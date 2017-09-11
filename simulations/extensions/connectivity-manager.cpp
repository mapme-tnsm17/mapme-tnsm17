/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/* This module handles synchronizes ICN faces with the L2 status, appropriately
 * creating and removing faces to match connectivity. In addition, it eventually
 * manages routing adjacencies through GlobalRouter (if installed on nodes).
 *
 * Note:
 *  - wired emulation: Ethernet mobility emulation done by having multiple
 *    interfaces and orchestrating up/down to simulate mobility.
 *
 * TAGS:
 *   L2DEPENDENT : marks sections of the code that are Layer-2 dependent (only
 *   WiFi is currently implemented + wired emulation)
 */

#include "connectivity-manager.hpp"

#include "ns3/ndnSIM/model/ndn-global-router.hpp"
#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/mac48-address.h"

#include "ns3/ndnSIM/NFD/daemon/table/fib.hpp"

// For wifi specific mobility
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"

// For wired
#include "ns3/point-to-point-module.h"

// EthernetNetDeviceFace

#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
//for hop counting
#include "../utils/ndn-fw-hop-count-tag.hpp"

#include "ns3/point-to-point-net-device.h"

#define COMBINE(x,y) ((x)*1000+(y))

#define WIRED_HYSTERESIS 2


namespace ns3 {
namespace ndn {

NS_LOG_COMPONENT_DEFINE("ConnectivityManager");

/*------------------------------------------------------------------------------
 * EthernetNetDeviceFace
 *
 * Subclass of NetDeviceFace created dynamically by ConnectivityManager to
 * handle L2-specific MAC address.
 *
 *----------------------------------------------------------------------------*/

NS_OBJECT_ENSURE_REGISTERED(EthernetNetDeviceFace);

// XXX We cannot have two log components in the same file
//NS_LOG_COMPONENT_DEFINE("EthernetNetDeviceFace");

EthernetNetDeviceFace::EthernetNetDeviceFace(Ptr<Node> node, const Ptr<NetDevice>& netDevice, Mac48Address destAddress)
  : NetDeviceFace(node, netDevice)
  , m_destAddress(destAddress)
{
  ostringstream desc;
  desc << "Face<node=" << node->GetId() << ", mac=XXX" << ", id=" << getId() << ">";
  setDescription(desc.str());
} 

TypeId 
EthernetNetDeviceFace::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EthernetNetDeviceFace")
    .SetParent<Object> ()
    //.AddConstructor<EthernetNetDeviceFace>()
    ;
  return tid;
}

Mac48Address
EthernetNetDeviceFace::GetDestAddress() const
{
  return m_destAddress;
}

void
EthernetNetDeviceFace::SetDestAddress(Mac48Address destAddress)
{
  m_destAddress = destAddress;
}

void
EthernetNetDeviceFace::send(Ptr<Packet> packet)
{
  NS_ASSERT_MSG(packet->GetSize() <= m_netDevice->GetMtu(),
                "Packet size " << packet->GetSize() << " exceeds device MTU "
                               << m_netDevice->GetMtu());
  NS_LOG_FUNCTION(this<<packet);

	FwHopCountTag tag;
	packet->RemovePacketTag(tag);
	tag.Increment();
	packet->AddPacketTag(tag);

	m_netDevice->Send(packet, m_destAddress, L3Protocol::ETHERNET_FRAME_TYPE);
}

void
EthernetNetDeviceFace::receiveFromNetDevice(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol,
                       const Address& from, const Address& to, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION(device << p << protocol << from << to << packetType);
  NS_LOG_DEBUG("EthernetFace received packet on face " << getDescription());

  /* A wifiNetDevice on an AP can be mapped to several faces corresponding to
   *  each station. A received packet on such a NetDevice will be sent to all
   *  faces, so we need to add some filtering at the face level. (remember here
   *  that m_destAddress is the remote address).
   *
   * NOTE: this code does not support for multicast addresses
   *
   * XXX Need some comments about the role of each of the two following tests.
   */

  if (m_destAddress != Mac48Address::GetBroadcast() && (from != m_destAddress || to != m_netDevice->GetAddress()))
    return;
  if (m_destAddress == Mac48Address::GetBroadcast() && (to != m_netDevice->GetAddress()))
    return;

  NetDeviceFace::receiveFromNetDevice(device, p, protocol, from, to, packetType);

}
/*----------------------------------------------------------------------------*/

//DEPRECATED| XXX Anchor support 
//DEPRECATED| const std::string PREFIX_ANNOUNCEMENT = std::string("/announcement");

NS_OBJECT_ENSURE_REGISTERED (ConnectivityManager);

std::map<Mac48Address, std::pair<Ptr<Node>, Ptr<NetDevice>>> ConnectivityManager::m_ap_mac2node;
std::unordered_map<int, Ptr<Node>> ConnectivityManager::m_ap_maps;
uint32_t ConnectivityManager::m_max_x_indx=0;
uint32_t ConnectivityManager::m_max_y_indx=0;



//DEPRECATED| bool ConnectivityManager::m_isNodeSet=false;
//DEPRECATED| std::map<Mac48Address, Ptr<Node>> ConnectivityManager::m_map_apmac_apnode=std::map<Mac48Address, Ptr<Node>>();

TypeId 
ConnectivityManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConnectivityManager")
    .SetParent<Object> ()
    .AddConstructor<ConnectivityManager>()

    // BEGIN WIFI
    // XXX ConnectivityManager offers two callbacks. Document the usage to
    // justify keeping this code
    .AddTraceSource ("MobilityEvent", "MobilityEvent",  MakeTraceSourceAccessor(&ConnectivityManager::m_mobilityEvent))
    // END WIFI

    ;
  return tid;
}

//------------------------for wired link emulation-------------------------------
void 
ConnectivityManager::setApMaps(NodeContainer leaves, Options& options)
{
  m_ap_maps=getApMaps(leaves, options);
}

double
ConnectivityManager::getBase(double x){
    uint32_t x_uint=x+0.5;
    return x_uint-0.5;
}


std::unordered_map<int, Ptr<Node> >
ConnectivityManager::getApMaps(NodeContainer leaves, Options& options){
std::unordered_map<int, Ptr<Node> > ap_maps;
m_max_x_indx=0;
m_max_y_indx=0;
    for(unsigned i=0;i<leaves.GetN();i++){
        Ptr <MobilityModel> mobilityModel = leaves.Get(i)->GetObject<MobilityModel> ();
        NS_ASSERT(mobilityModel!=0);
        uint32_t x_indx =mobilityModel->GetPosition ().x/options.cell_size;
        uint32_t y_indx= mobilityModel->GetPosition ().y/options.cell_size;
        ap_maps[COMBINE(x_indx,y_indx)] = leaves.Get(i);
        if(x_indx>m_max_x_indx)
            m_max_x_indx=x_indx;
        if(y_indx>m_max_y_indx)
            m_max_y_indx=y_indx;

    }
    return ap_maps;
}

void 
ConnectivityManager::checkHandover(Ptr<Node> mobile, uint32_t cell_size, std::unordered_map<int, Ptr<Node>>& maps)
{
    Ptr <MobilityModel> mobilityModel = mobile->GetObject<MobilityModel> ();
    NS_ASSERT(mobilityModel!=0);
      double x =mobilityModel->GetPosition ().x;
      double y= mobilityModel->GetPosition ().y;
      
      x=x/cell_size;
      y=y/cell_size;
      
      double base_x=getBase(x);
      double base_y=getBase(y);
      
      Ptr<Node> ap=getCurrentAp();
//       if(!ap){
//                     std::cout<<"first attach from "<< mobile->GetId()<<"to " <<NodeList::GetNode(2)->GetId() <<"\n";
// 
//           onAttachTo(NodeList::GetNode(2));
//           Simulator::Schedule(Seconds(0.1),&ConnectivityManager::checkHandover,this,  mobile, cell_size,maps);   
//           return;
//     }
//     ap=getCurrentAp();
//       NS_ASSERT_MSG(ap!=0, "current ap is empty");
      
      double x_ap;
      double y_ap;
if(ap){
      mobilityModel= ap->GetObject<MobilityModel> ();
      x_ap =mobilityModel->GetPosition ().x/cell_size;
      y_ap= mobilityModel->GetPosition ().y/cell_size;
}else{
    x_ap=16;//infinite
    y_ap=16;
}

      
      double minDist2=(x-x_ap)*(x-x_ap) + (y-y_ap)*(y-y_ap);
      double dist=4;
      double newAp_x=0;
      double newAp_y=0;
      for(int i=0; i<4;i++){
          double tmp_newAp_x=base_x+(i/2);
          
          if(tmp_newAp_x<0 || tmp_newAp_x>m_max_x_indx+1) continue;
          
          double tmp_newAp_y=base_y+(i%2);
          
           if(tmp_newAp_y<0 || tmp_newAp_y>m_max_y_indx+1) continue;

          double newdist = (tmp_newAp_x - x)*(tmp_newAp_x - x) + (tmp_newAp_y - y)*(tmp_newAp_y - y);
          if(newdist < dist){
              dist=newdist;
              newAp_x=tmp_newAp_x;
              newAp_y=tmp_newAp_y;
          }
  
      }
      
       if(dist< minDist2/WIRED_HYSTERESIS)
       {
           uint32_t new_ap_indx=newAp_x;
           uint32_t new_ap_indy=newAp_y;
           
//            std::cout<<"ap_base_x"<<"\tap_base_y"<<"\told_ap_x"<<"\told_ap_y\tnew_ap_x\tnew_ap_y\tx\ty\n";
//            std::cout<<"\t"<<base_x<<"\t"<<base_y<<"\t"<<x_ap<<"\t"<<y_ap<<"\t"<<newAp_x<<"\t"<<newAp_x<<"\t"<<x<<"\t"<<y<<"\n";
// std::cout<<"old ap="<<(x_ap-0.5)*10+y_ap-0.5<<"\n";
           if(ap)//not first attachment
              onDeattachFromCurrent();
//            std::cout<<"nw_ap_indx="<<new_ap_indx*10+new_ap_indy<<"\n";
           onAttachTo(maps[COMBINE(new_ap_indx,new_ap_indy)]);
       }
      
      Simulator::Schedule(Seconds(0.1),&ConnectivityManager::checkHandover, this, mobile, cell_size,maps);
      
}


bool 
ConnectivityManager::isWired()
{
    return m_wired;
}
  
void 
ConnectivityManager::setWired(bool isWired, double cell_size=-1)
{
    m_wired=isWired;
    if(m_wired){
        NS_ASSERT_MSG(m_ap_maps.size()!=0,"check ap coordinate map is set correctly before enabling wired");
        NS_ASSERT_MSG(cell_size!=-1,"check cell size is passed accordinglly for enabling wired");
        Simulator::Schedule(Seconds(GetObject<Node>()->GetId()/100.0),&ConnectivityManager::checkHandover,this, GetObject<Node>(), cell_size,m_ap_maps);
    }
}

Ptr<Node> 
ConnectivityManager::getCurrentAp()
{
    return m_currentAP;
}

void 
ConnectivityManager::onAttachTo(Ptr<Node> ap_node)
{
  Ptr<Node> sta_node = GetObject<Node>();
//   Ptr<NetDevice> sta_nd = GetNetDeviceFromContext(context);
//   Mac48Address sta_mac = Mac48Address::ConvertFrom(sta_nd->GetAddress());
// 
//   std::pair<Ptr<Node>, Ptr<NetDevice>> ap_node_nd = GetNodeNetDeviceFromMac(ap_mac);
// 
//   Ptr<NetDevice> ap_nd = ap_node_nd.second;
  
  NS_LOG_INFO("Associating node " << sta_node->GetId() << " to new AP " << ap_node->GetId() );
//   std::cout<<Simulator::Now()<<"ssociating node " << sta_node->GetId() << " to new AP " << ap_node->GetId()<<"\n";

  /* Dual objective:
   *  - Face management : L2 abstraction using EthernetNetDeviceFace
   *  - Routing : update adjacencies and recompute routes at least at first
   *  attachment
   *
   * We need to at least update GlobalRouting once at the first attachment to
   * populate the FIBs
   *
   * Issue: we need to recompute routes before the face is created since this
   * triggers the transmission of an IU which will update the FIBs (not yet
   * created). For this, we need to update incidencies, which requires face
   * information (not yet created)... This requires that we delay a bit the
   * sending of the IU.
   *
   * So we create the face on AP first, so that the producer is reachable, so
   * that we can recompute routing, and the STA face in the end, since it will
   * trigger the IU. We thus need to mix both face and routing management in
   * the code.
   *
   */

  Ptr<GlobalRouter> ap_gr = ap_node->GetObject<GlobalRouter>();
  Ptr<GlobalRouter> sta_gr = sta_node->GetObject<GlobalRouter>();
  //assert(sta_gr && ap_gr);
  
  Ptr<NetDevice> fromdevice=GetPointToPointNetDevice(ap_node,sta_node);//ap's device
  
  Ptr<PointToPointNetDevice> pd = DynamicCast<PointToPointNetDevice>(fromdevice);
  NS_ASSERT(pd!=0);
  Ptr<Channel> ch = pd->GetChannel();                                                            
  assert(ch->GetNDevices() == 2);
    
  Ptr<NetDevice> todevice=0;  
  if(ch->GetDevice(0)==fromdevice)
      todevice=ch->GetDevice(1);
  else
      todevice=ch->GetDevice(0);

  // Manage AP first
  //
  // Since there is no callback in ap-wifi-mac.*, let's also trigger face
  // creation on the AP side here. For this, we first need to retrieve the
  // NetDevice involved in the association from the context.

  shared_ptr<Face> ap_face = CreateWiredFace(ap_node, sta_node,fromdevice);
#ifdef MLDR
  ns3::ndn::EthernetNetDeviceFace* ap_ndf = static_cast<ns3::ndn::EthernetNetDeviceFace*>((ap_face.get()));
  ap_ndf->SetDestAddress(sta_mac);
#endif
  
    if (m_firstAttach) {//clean up redundant adjacencies and start from scratch at edges

        if (sta_gr){  
//               std::cout<<"first attach clean for "<<sta_node->GetId()<<"adj no.="<<sta_gr->GetIncidencies().size() <<"\n";
              GlobalRouter::IncidencyList& incidencies = sta_gr->GetIncidencies();
              for(GlobalRouter::IncidencyList::iterator i = incidencies.begin() ; i != incidencies.end(); ++i) {
    
                Ptr<GlobalRouter> one_neighbour = std::get<2>(*i);
                sta_gr->GetL3Protocol()->removeFace(std::get<1>(*i));//remove face on sta node
                //remove face and incidency on ap:
                RemoveIncidencyAndFace(one_neighbour,sta_gr);
              }  
              sta_gr->ClearIncidency();
        }
    }
  
  // NOTE: the face is passed as a second argument because it is used by the
  // GlobalRoutingHelper to populate the FIBs after Dijkstra has run.
  if (sta_gr && ap_gr)
    ap_gr->AddIncidency(ap_face, sta_gr);

  // Handling first attachment
  if (m_firstAttach) {
    RecomputeRoutes();
    m_firstAttach = false;
  }

  // Create the face on the station
  shared_ptr<Face> sta_face = CreateWiredFace(sta_node, ap_node,todevice);
#ifdef MLDR
  ns3::ndn::EthernetNetDeviceFace* sta_ndf = static_cast<ns3::ndn::EthernetNetDeviceFace*>((sta_face.get()));
  sta_ndf->SetDestAddress(ap_mac);
#endif
  
  // Add default route
  FibHelper::AddRoute(sta_node, "/", sta_face, 0);

  if (sta_gr && ap_gr)
    sta_gr->AddIncidency(sta_face, ap_gr);

#ifdef MLDR

  if(this->GlobalRoutingActivated())
  {
          if(m_firstAttachFinished)
          {
                  this->RecomputeRoutes();
                  ApplyMLDROnFaceCreation(sta_node, sta_face);
                  ApplyMLDROnFaceCreation(ap_node, ap_face);
          }
  }
  m_firstAttachFinished = true;

  Ptr<L3Protocol> l3 = sta_node->GetObject<L3Protocol>();
  bool wifi_reassoc = l3->getForwarder()->getFwPlugin().WifiReassocActivated();
  if(wifi_reassoc)
  {
          sta_face->AllowStaOperations(); // Practically this procedure is useful on a producer node
          l3->getForwarder()->getFwPlugin().getWifiReassoc().SendReassocMsg(*sta_face.get());
  }
  if(this->NS3ReassocActivated())
  {
          Mac48Address oldApMac = m_ns3Reassoc.GetPrevApMac();
          bool destroy = m_ns3Reassoc.ShouldDestroy(ap_mac);
          if(destroy)
          {
                  std::pair<Ptr<Node>, Ptr<NetDevice>> old_ap_node_nd = GetNodeNetDeviceFromMac(oldApMac);
                  Ptr<Node> old_ap_node = old_ap_node_nd.first;
                  Ptr<NetDevice> old_ap_nd = old_ap_node_nd.second;
                  this->DestroyFace(old_ap_node, old_ap_nd, sta_mac);
          }
  }
#endif

  m_currentAP=ap_node;


  m_mobilityEvent(Simulator::Now().ToDouble(Time::S), 'A', sta_node, ap_node);
 
  
}

void 
ConnectivityManager::onDeattachFromCurrent()
{
  Ptr<Node> sta_node = GetObject<Node>();
//   Ptr<NetDevice> sta_nd = GetNetDeviceFromContext(context);
//   Mac48Address sta_mac = Mac48Address::ConvertFrom(sta_nd->GetAddress());

//   std::pair<Ptr<Node>, Ptr<NetDevice>> ap_node_nd = GetNodeNetDeviceFromMac(ap_mac);
  Ptr<Node> ap_node = m_currentAP;
  NS_ASSERT_MSG(ap_node!=NULL, "current_ap is empty");
//   Ptr<NetDevice> ap_nd = ap_node_nd.second;

//    std::cout<<"Deassociating node " << sta_node->GetId() << " from AP " << ap_node->GetId() << " \n";

  /* Face management : L2 abstraction using EthernetNetDeviceFace */

  // destroy the face on the station
  DestroyWiredFace(sta_node, ap_node);

#ifdef MLDR
  Ptr<L3Protocol> l3 = sta_node->GetObject<L3Protocol>();
  if(!NS3ReassocActivated() && !l3->getForwarder()->getFwPlugin().WifiReassocActivated())
  {
#endif
          // Since there is no callback in ap-wifi-mac.*, let's also trigger face
          // creation on the AP side here. For this, we first need to retrieve the
          // NetDevice involved in the association from the context.
          DestroyWiredFace(ap_node, sta_node);
#ifdef MLDR
  }
#endif

  /* Routing : if GlobalRouter present in both nodes, update adjacencies */
  Ptr<GlobalRouter> sta_gr = sta_node->GetObject<GlobalRouter>();
  Ptr<GlobalRouter> ap_gr = ap_node->GetObject<GlobalRouter>();
  // XXX face as second argument ?
  if (sta_gr && ap_gr) {
    RemoveIncidency(sta_gr, ap_gr);
    RemoveIncidency(ap_gr, sta_gr);
  }
//#ifdef MLDR
//  RecomputeRoutes();
//#endif

  m_currentAP=NULL;
  m_mobilityEvent(Simulator::Now().ToDouble(Time::S), 'D', sta_node, ap_node);
}
//------------------------wired emulation functionality end---------------------









/*------------------------------------------------------------------------------
 * Constructor and aggregation handler
 *----------------------------------------------------------------------------*/

ConnectivityManager::ConnectivityManager()
  : m_registered(false)
  , m_wired(false)
  , m_firstAttach(true)
#ifdef WLDR
  , m_wldr(false)
#endif
#ifdef MLDR
  , m_ns3Reassoc_activated(false)
  , m_ns3Reassoc()
  , m_global_routing(false)
  , m_firstAttachFinished(false)
#endif
{}

void
ConnectivityManager::NotifyNewAggregate()
{
  if (m_registered)
    return;
  m_registered = true;

  Ptr<Node> node;
  // No need to hook any signal for wired emulation
  if (m_wired) {
    NS_LOG_INFO("CM aggregated to node with wired emulation enabled");
    goto default_notify;
  }

  /* ConnectivityManager might be associated to a new Node, let's register
   * callbacks to handle WiFi association/deassociation events.
   *
   * Notice that we don't have any callback on the Access Point, and we need to
   * hook everything from there.
   * TODO integrate path from NR ?œ
   *
   * Implementation notes:
   *
   * This callback triggers for every device, we could specialize the callback
   * for every device but this would require to handle new NetDevices. Here we
   * process the context string to extract the corresponding device id (the node
   * is also present but we have it in the parameters... in fact we might have a
   * single connectivity manager for the whole experiment, but this is different
   * from what we have on a real system, so let's stick to the current
   * implementation unless it has performance issues.
   */
  node = GetObject<Node>();
  if (node) {
    ostringstream path1, path2;
    std::string id = boost::lexical_cast<std::string>(node->GetId());

    NS_LOG_INFO("CM aggregated to node " << id << ", registering WiFi callbacks");

    path1 << "/NodeList/" + id + "/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/DeAssoc";
    Config::Connect(path1.str(), MakeCallback(&ConnectivityManager::OnWifiDeassoc, this));
    path2 << "/NodeList/" + id + "/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/Assoc";
    Config::Connect(path2.str(), MakeCallback(&ConnectivityManager::OnWifiAssoc, this));
  }

default_notify:
  // Default behaviour 
  Object::NotifyNewAggregate();
}

#ifdef WLDR
void ConnectivityManager::enable_wldr(){
  m_wldr =  true;
}
#endif

/*-----------------------------------------------------------------------------
 * Helper functions
 *---------------------------------------------------------------------------*/

// The two following functions are from
// https://www.nsnam.org/bugzilla/attachment.cgi?id=1182&action=diff&context=patch&collapsed=&headers=1&format=raw

std::vector<std::string> 
ConnectivityManager::GetElementsFromContext (std::string context)
{
  vector <std::string> elements;
  size_t pos1=0, pos2;
  while (pos1 != context.npos)
  {
    pos1 = context.find ("/",pos1);
    pos2 = context.find ("/",pos1+1);
    elements.push_back (context.substr (pos1+1,pos2-(pos1+1)));
    pos1 = pos2; 
    pos2 = context.npos;
  }
  return elements;
}

Ptr<NetDevice>
ConnectivityManager::GetNetDeviceFromContext (std::string context)
{
  // Use "NodeList/*/DeviceList/*/ as reference
  // where element [1] is the Node Id
  // element [2] is the NetDevice Id

  vector <std::string> elements = GetElementsFromContext (context);
  Ptr <Node> n = NodeList::GetNode (atoi (elements[1].c_str ()));
  NS_ASSERT (n);
  return n->GetDevice (atoi (elements[3].c_str ()));
}

/**
 * Build a map of APs for detecting association/deassociation to nodes and use
 * it to retrieve the AP node based on the Mac48Address present in the
 * association callback.
 *
 * Implementation note:
 *  - This function is L2 dependent (L2DEPENDENT)
 *  - We assume the BSSID of the access point is equal to the MAC address (which
 *  is likely the case in ns3)
 */

std::pair<Ptr<Node>, Ptr<NetDevice>>
ConnectivityManager::GetNodeNetDeviceFromMac(Mac48Address mac)
{
  std::map<Mac48Address, std::pair<Ptr<Node>, Ptr<NetDevice>>>::iterator i_node_nd;
  i_node_nd = m_ap_mac2node.find(mac);
  if (i_node_nd != m_ap_mac2node.end())
    return i_node_nd->second;

  // Mac address not found in local map: new AP. Loop over nodes to find it.
  // This is useful in case of dynamically added nodes.
  Ptr<NetDevice> nd = NULL;
  Ptr<WifiNetDevice> wnd = NULL;

  for (NodeList::Iterator inode = NodeList::Begin(); inode != NodeList::End(); inode++) {
    Ptr<Node> node = *inode;

    for (unsigned int i = 0; i < node->GetNDevices(); i++) {
      nd = node->GetDevice(i);

      if (nd->GetInstanceTypeId ().GetName() == "ns3::WifiNetDevice") {
				std::pair<Ptr<Node>, Ptr<NetDevice>> node_nd(node, nd);

        wnd = DynamicCast<WifiNetDevice>(nd);

        // Only access points are concerned...
        Ptr<ApWifiMac> awm = DynamicCast<ApWifiMac>(wnd->GetMac());
        if (awm)
          m_ap_mac2node[awm->GetBssid()] = node_nd;
      }
    }
  }
  return m_ap_mac2node.find(mac)->second;
}
void
ConnectivityManager::ClearFIB(Ptr<Node> node, Name const & prefix)
{
  Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();

  /* Since we don't know the face, we will try to remove the next hop for all
   * faces */
  for (auto& i : l3->getForwarder()->getFaceTable()) {
    std::shared_ptr<Face> face = std::dynamic_pointer_cast<Face>(i);
    if (!face->isLocal())
      FibHelper::RemoveRoute(node, prefix, face);
  }
}

void
ConnectivityManager::ClearFIBs(Name const & prefix)
{
  NFD_LOG_INFO("Clearing FIBs for prefix" << prefix);
  for (NodeList::Iterator inode = NodeList::Begin(); inode != NodeList::End(); inode++) {
    Ptr<Node> node = *inode;
    ClearFIB(node, prefix);
  }
}

void
ConnectivityManager::RecomputeRoutes()
{
  NS_LOG_INFO("Recomputing global routing");

  Ptr<Node> node = GetObject<Node>();
  assert(node);

#if 0
  /* Using the FIB is not correct in cases when the application starts at time t
   * > 0, which means at the time the producer associates, the application face
   * is not in the FIB and no route is computed. We thus rely on GlobalRouter's
   * origins.
   */

  Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();
  assert(l3);

	/* Loop over all prefixes that have a local next hop in the FIB = for which
	 * the node is a producer, and recompute routes. Nothing will happen if the
   * node is a pure consumer.
	 * 
	 * XXX This does not work for multiple producers with the same prefix ! We
   * need to selectively clear the FIB
   */
  //
  for (auto & iEntry : l3->getForwarder()->getFib()) {
    for (auto & iNextHop : iEntry.getNextHops()) {
      if(iNextHop.getFace()->getLocalUri() == FaceUri("appFace://")) {
				ClearFIBs(iEntry.getPrefix());
				MyGlobalRoutingHelper::CalculateRoutesForPrefix(iEntry.getPrefix());
        break;
      }
    }
  }
#endif 

  Ptr<GlobalRouter> gr = node->GetObject<GlobalRouter>();
  if (gr) {
    for (auto name: gr->GetLocalPrefixes()) {
			ClearFIBs(*name);
			MyGlobalRoutingHelper::CalculateRoutesForPrefix(*name);
    }
  }
}

shared_ptr<Face> 
ConnectivityManager::CreateFace(Ptr<Node> node, Ptr<NetDevice> netDevice, Mac48Address mac)
{
  ostringstream desc;
  Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();
  if (!l3) {
    std::cerr << "ERROR: No NDN stack found on the node, cannot create face" << std::endl;
    return 0;
  }   

  shared_ptr<NetDeviceFace> face = std::make_shared<EthernetNetDeviceFace>(node, netDevice, mac);
  l3->addFace(face);

  desc << "EthernetNetDeviceFace<node=" << node->GetId() << ", mac=" << mac 
    << ", id=" << face->getId() << ">";
  face->setDescription(desc.str());

  NS_LOG_INFO("Created face " << face->getDescription());

#ifdef WLDR
  if(m_wldr){
	face->SetFaceAsWireless();
	face->enable_wldr_scheme();
#ifdef NDNSIM
    Ptr<WifiNetDevice> wifi_net_dev = StaticCast<WifiNetDevice>(netDevice); 
    Ptr<RegularWifiMac> wifi_mac = StaticCast<RegularWifiMac>(wifi_net_dev->GetMac());
    ns3::PointerValue ptr;
    wifi_mac->GetAttribute ("BE_EdcaTxopN",ptr);
    face->set_wldr_queue(ptr.Get<ns3::EdcaTxopN>()->GetEdcaQueue());
#endif
  }
#endif

#ifdef MLDR
  	face->setUp(true);
	face->SetFaceAsWireless();
  if(!GlobalRoutingActivated())
    {
	  ApplyMLDROnFaceCreation(node, face);
    }
#endif

  return face;
}

shared_ptr<Face> 
ConnectivityManager::CreateWiredFace(Ptr<Node> from, Ptr<Node> to, Ptr<NetDevice> fromDevice)
{
  ostringstream desc;
  Ptr<L3Protocol> l3 = from->GetObject<L3Protocol>();
  if (!l3) {
    std::cerr << "ERROR: No NDN stack found on the node, cannot create face" << std::endl;
    return 0;
  }   

  //TODO: find netdevice based on node:
  Ptr<NetDevice> device=fromDevice;
  NS_ASSERT_MSG(device!=0,"cannot find p2p device on node");
  
  shared_ptr<NetDeviceFace> face = std::make_shared<NetDeviceFace>(from, device);
  l3->addFace(face);

  desc << "PointToPointNetDeviceFace<node=" << from->GetId() << ", mac=" << 0
    << ", id=" << face->getId() << ">";
  face->setDescription(desc.str());

  NS_LOG_INFO("Created face " << face->getDescription());

#ifdef WLDR
  if(m_wldr){
        face->SetFaceAsWireless();
        face->enable_wldr_scheme();
#ifdef NDNSIM
    Ptr<WifiNetDevice> wifi_net_dev = StaticCast<WifiNetDevice>(netDevice); 
    Ptr<RegularWifiMac> wifi_mac = StaticCast<RegularWifiMac>(wifi_net_dev->GetMac());
    ns3::PointerValue ptr;
    wifi_mac->GetAttribute ("BE_EdcaTxopN",ptr);
    face->set_wldr_queue(ptr.Get<ns3::EdcaTxopN>()->GetEdcaQueue());
#endif
  }
#endif

#ifdef MLDR
        face->setUp(true);
        face->SetFaceAsWireless();
  if(!GlobalRoutingActivated())
    {
          ApplyMLDROnFaceCreation(node, face);
    }
#endif

  return face;
}

void
ConnectivityManager::DestroyFace(Ptr<Node> node, Ptr<NetDevice> netDevice, Mac48Address mac)
{

  Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();
  if (!l3) {
    std::cerr << "ERROR: No NDN stack found on the node, no face to remove" << std::endl;
    return;
  }   

  for (auto& i : l3->getForwarder()->getFaceTable())
  {
    shared_ptr<EthernetNetDeviceFace> face = std::dynamic_pointer_cast<EthernetNetDeviceFace>(i);
    if (!face || face->GetDestAddress() != mac)
      continue;
#ifdef MLDR
    ApplyMLDROnFaceDestruction(node, face);
#endif

    NS_LOG_INFO("Destroyed face " << face->getDescription());
    l3->removeFace(face);
    return; // to avoid iterator issues
  }
}

void
ConnectivityManager::DestroyWiredFace(Ptr<Node> from, Ptr<Node> to)
{

  Ptr<L3Protocol> l3 = from->GetObject<L3Protocol>();
  if (!l3) {
    std::cerr << "ERROR: No NDN stack found on the node, no face to remove" << std::endl;
    return;
  }   

  for (auto& i : l3->getForwarder()->getFaceTable())
  {
    shared_ptr<NetDeviceFace> face = std::dynamic_pointer_cast<NetDeviceFace>(i);
    if(!face) continue;
    Ptr<NetDevice> fromDevice = face->GetNetDevice();
    Ptr<PointToPointNetDevice> pd = DynamicCast<PointToPointNetDevice>(fromDevice);
    if(!pd) continue;
    
    Ptr<Channel> ch = pd->GetChannel();                                                            
    assert(ch->GetNDevices() == 2);
    
    //compute other side of this p2p device
    Ptr<Node> otherNode;
    for (uint32_t deviceId = 0; deviceId < ch->GetNDevices(); deviceId++) {                        
      Ptr<NetDevice> otherSide = ch->GetDevice(deviceId);                                          
      if (pd == otherSide) continue;                                                               
      otherNode = otherSide->GetNode();                                                            
    }
    
    
    if (!otherNode || otherNode->GetId()!=to->GetId())
      continue;
#ifdef MLDR
    ApplyMLDROnFaceDestruction(node, face);
#endif

    NS_LOG_INFO("Destroyed face " << face->getDescription());
//     std::cout<<"Destroyed face " << face->getDescription()<<"\n";
    l3->removeFace(face);
    return; // to avoid iterator issues
  }
}


Ptr<NetDevice> 
ConnectivityManager::GetPointToPointNetDevice(Ptr<Node> from, Ptr<Node> to)
{
  Ptr<L3Protocol> l3 = from->GetObject<L3Protocol>();
  if (!l3) {
    std::cerr << "ERROR: No NDN stack found on the node, no face to remove" << std::endl;
    return NULL;
  }   
//   std::cout<<"+++++++++++from="<<from->GetId()<<",to="<<to->GetId()<<" device NO. =" <<from->GetNDevices()<<"\n";

      for (unsigned i=0;i<from->GetNDevices();i++)
  {
    Ptr<NetDevice> fromDevice = from->GetDevice(i);
    Ptr<PointToPointNetDevice> pd = DynamicCast<PointToPointNetDevice>(fromDevice);
    if(!pd) continue;
    
    Ptr<Channel> ch = pd->GetChannel();                                                            
    assert(ch->GetNDevices() == 2);
    
    //compute other side of this p2p device
    Ptr<Node> otherNode;
    for (uint32_t deviceId = 0; deviceId < ch->GetNDevices(); deviceId++) {                        
      Ptr<NetDevice> otherSide = ch->GetDevice(deviceId);                                          
      if (pd == otherSide) continue;                                                               
      otherNode = otherSide->GetNode();                                                            
    }
    
    
    if (!otherNode || otherNode->GetId()!=to->GetId())
      continue;

    return fromDevice;
  }
  
  return NULL;
}

#ifdef MLDR
void
ConnectivityManager::DestroyKnownFace(Ptr<Node> node, shared_ptr<Face> face)
{

	Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();
	if (!l3) {
		std::cerr << "ERROR: No NDN stack found on the node, no face to remove" << std::endl;
		return;
	}
	// #ifdef MLDR
	ApplyMLDROnFaceDestruction(node, face);
	// #endif
	NS_LOG_INFO("Destroyed face " << face->getDescription());
	l3->removeFace(face);
	return;
}
void
ConnectivityManager::ApplyMLDROnFaceCreation(Ptr<Node> node, shared_ptr<Face> face)
{
	Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();
	if (!l3) {
		std::cerr << "ERROR: No NDN stack found on the node, cannot create face" << std::endl;
		return;
	}
	face->setUp(true);
	face->SetFaceAsWireless();
	bool mldr = l3->getForwarder()->getFwPlugin().MldrActivated();
	bool eln = l3->getForwarder()->getFwPlugin().ElnActivated();
	if(mldr || eln)
		l3->getForwarder()->getFwPlugin().OnFaceAvailable(*face.get());
}
void
ConnectivityManager::ApplyMLDROnFaceDestruction(Ptr<Node> node, shared_ptr<Face> face)
{
	Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();
		  if (!l3) {
		    std::cerr << "ERROR: No NDN stack found on the node, cannot create face" << std::endl;
		    return;
		  }
	face->setUp(false);
	l3->getForwarder()->getFwPlugin().getWifiReassoc().ResetTimerOnFaceDestruction();
	bool mldr = l3->getForwarder()->getFwPlugin().MldrActivated();
	bool eln = l3->getForwarder()->getFwPlugin().ElnActivated();
	if(mldr || eln)
		l3->getForwarder()->getFwPlugin().OnFaceDestroyed(*face.get());
}
#endif
  
/*------------------------------------------------------------------------------
 * Mobility triggers from outside
 *
 * NOTES:
 *  - could be dropped except it is needed by wired emulation
 *  - used for instance by a forced random waypoint that determines when to
 *  deattach from a station and reattach to a new one.
 *
 * TODO:
 *  - XXX This is broken in case of MULTIHOMING or with different L2 because of
 *  GetNodeMac()
 *
 *----------------------------------------------------------------------------*/

void
ConnectivityManager::BeforeMobility()
{
  NS_LOG_DEBUG("BeforeMobility");

  /*
  if (m_wired) {
    // For wired emulation, we cannot rely on any callback
    if (!m_first_jump)
      UpdateNDNStation();
    return;
  }
  */
  if(m_wired){
      if(m_currentAP){
          onDeattachFromCurrent();    
      }
      return;
  }

  /* Force detach from current AP */
  // XXX a node might have several physical interfaces
  GetNodeMac()->SetState(StaWifiMac::BEACON_MISSED);

}

void
ConnectivityManager::AfterMobility(Ptr<Node> current_leaf)
{
  NS_LOG_DEBUG("AfterMobility");

  /*
  if (m_wired) {
    // For wired emulation, we cannot rely on any callback
    assert (current_leaf != NULL);
    UpdateNDNStation(current_leaf);
    return;
  }
  */
  

  if(m_wired){
      onAttachTo(current_leaf);
      return;
  }
  // Trigger attachment to new AP
  // XXX a node might have several physical interfaces
  GetNodeMac()->TryToEnsureAssociated ();
}

/*
unsigned int 
ConnectivityManager::getNodeId(const std::string & path) 
{
  //the path is in the form /NodeList/node-id/...
  //node-id is between end of "/NodeList/" and the next occurrence of "/"
  std::string no_nid = path.substr(10);
  int pos = no_nid.find('/');
  istringstream s(no_nid.substr(0, pos));
  unsigned int ret;
  s >> ret;
  return ret;
}
*/

Ptr<StaWifiMac>
ConnectivityManager::GetNodeMac()
{
  Ptr<Node> node = GetObject<Node>();
  Ptr<NetDevice> d = node->GetDevice(0);
  Ptr<WifiNetDevice> wd =  DynamicCast<WifiNetDevice>(d);
  return DynamicCast<StaWifiMac> (wd->GetMac () );
}

/*------------------------------------------------------------------------------
 * Callbacks from layer 2 issued by *stations*
 *
 * Implementation note:
 *  - This code will have to be moved in a separate function is we ever
 *  generalize L2.
 *
 *----------------------------------------------------------------------------*/

void
ConnectivityManager::OnWifiAssoc(std::string context, Mac48Address ap_mac) 
{
  Ptr<Node> sta_node = GetObject<Node>();
  Ptr<NetDevice> sta_nd = GetNetDeviceFromContext(context);
  Mac48Address sta_mac = Mac48Address::ConvertFrom(sta_nd->GetAddress());

  std::pair<Ptr<Node>, Ptr<NetDevice>> ap_node_nd = GetNodeNetDeviceFromMac(ap_mac);
  Ptr<Node> ap_node = ap_node_nd.first;
  Ptr<NetDevice> ap_nd = ap_node_nd.second;
  
  NS_LOG_INFO("Associating node " << sta_node->GetId() << " to new AP " << ap_node->GetId() << " (" << ap_mac << ")");

  /* Dual objective:
   *  - Face management : L2 abstraction using EthernetNetDeviceFace
   *  - Routing : update adjacencies and recompute routes at least at first
   *  attachment
   *
   * We need to at least update GlobalRouting once at the first attachment to
   * populate the FIBs
   *
   * Issue: we need to recompute routes before the face is created since this
   * triggers the transmission of an IU which will update the FIBs (not yet
   * created). For this, we need to update incidencies, which requires face
   * information (not yet created)... This requires that we delay a bit the
   * sending of the IU.
   *
   * So we create the face on AP first, so that the producer is reachable, so
   * that we can recompute routing, and the STA face in the end, since it will
   * trigger the IU. We thus need to mix both face and routing management in
   * the code.
   *
   */

  Ptr<GlobalRouter> ap_gr = ap_node->GetObject<GlobalRouter>();
  Ptr<GlobalRouter> sta_gr = sta_node->GetObject<GlobalRouter>();
  //assert(sta_gr && ap_gr);

  // Manage AP first
  //
  // Since there is no callback in ap-wifi-mac.*, let's also trigger face
  // creation on the AP side here. For this, we first need to retrieve the
  // NetDevice involved in the association from the context.

  shared_ptr<Face> ap_face = CreateFace(ap_node, ap_nd, sta_mac);
#ifdef MLDR
  ns3::ndn::EthernetNetDeviceFace* ap_ndf = static_cast<ns3::ndn::EthernetNetDeviceFace*>((ap_face.get()));
  ap_ndf->SetDestAddress(sta_mac);
#endif
  
  // NOTE: the face is passed as a second argument because it is used by the
  // GlobalRoutingHelper to populate the FIBs after Dijkstra has run.
  if (sta_gr && ap_gr)
    ap_gr->AddIncidency(ap_face, sta_gr);

  // Handling first attachment
  if (m_firstAttach) {
    RecomputeRoutes();
    m_firstAttach = false;
  }

  // Create the face on the station
  shared_ptr<Face> sta_face = CreateFace(sta_node, sta_nd, ap_mac);
#ifdef MLDR
  ns3::ndn::EthernetNetDeviceFace* sta_ndf = static_cast<ns3::ndn::EthernetNetDeviceFace*>((sta_face.get()));
  sta_ndf->SetDestAddress(ap_mac);
#endif
  
  // Add default route
  FibHelper::AddRoute(sta_node, "/", sta_face, 0);

  if (sta_gr && ap_gr)
    sta_gr->AddIncidency(sta_face, ap_gr);

#ifdef MLDR

  if(this->GlobalRoutingActivated())
  {
	  if(m_firstAttachFinished)
	  {
		  this->RecomputeRoutes();
		  ApplyMLDROnFaceCreation(sta_node, sta_face);
		  ApplyMLDROnFaceCreation(ap_node, ap_face);
	  }
  }
  m_firstAttachFinished = true;

  Ptr<L3Protocol> l3 = sta_node->GetObject<L3Protocol>();
  bool wifi_reassoc = l3->getForwarder()->getFwPlugin().WifiReassocActivated();
  if(wifi_reassoc)
  {
	  sta_face->AllowStaOperations(); // Practically this procedure is useful on a producer node
	  l3->getForwarder()->getFwPlugin().getWifiReassoc().SendReassocMsg(*sta_face.get());
  }
  if(this->NS3ReassocActivated())
  {
	  Mac48Address oldApMac = m_ns3Reassoc.GetPrevApMac();
	  bool destroy = m_ns3Reassoc.ShouldDestroy(ap_mac);
	  if(destroy)
	  {
		  std::pair<Ptr<Node>, Ptr<NetDevice>> old_ap_node_nd = GetNodeNetDeviceFromMac(oldApMac);
		  Ptr<Node> old_ap_node = old_ap_node_nd.first;
		  Ptr<NetDevice> old_ap_nd = old_ap_node_nd.second;
		  this->DestroyFace(old_ap_node, old_ap_nd, sta_mac);
	  }
  }
#endif

  m_currentAP=ap_node;

  m_mobilityEvent(Simulator::Now().ToDouble(Time::S), 'A', sta_node, ap_node);
}

void
ConnectivityManager::RemoveIncidency(Ptr<GlobalRouter> gr, Ptr<GlobalRouter> remote_gr)
{
  GlobalRouter::IncidencyList& incidencies = gr->GetIncidencies();
  for(GlobalRouter::IncidencyList::iterator i = incidencies.begin() ; i != incidencies.end(); ++i) {
    if (std::get<2>(*i) == remote_gr) {
      incidencies.erase(i);
      break;
    }
  }
}

void
ConnectivityManager::RemoveIncidencyAndFace(Ptr<GlobalRouter> gr, Ptr<GlobalRouter> remote_gr)
{
  GlobalRouter::IncidencyList& incidencies = gr->GetIncidencies();
  for(GlobalRouter::IncidencyList::iterator i = incidencies.begin() ; i != incidencies.end(); ++i) {
    if (std::get<2>(*i) == remote_gr) {
      gr->GetL3Protocol()->removeFace(std::get<1>(*i));//remove face
      incidencies.erase(i);
//       std::cout<<"remove incidency and face\n";
      break;
    }
  }
}

void 
ConnectivityManager::OnWifiDeassoc(std::string context, Mac48Address ap_mac) 
{
  Ptr<Node> sta_node = GetObject<Node>();
  Ptr<NetDevice> sta_nd = GetNetDeviceFromContext(context);
  Mac48Address sta_mac = Mac48Address::ConvertFrom(sta_nd->GetAddress());

  std::pair<Ptr<Node>, Ptr<NetDevice>> ap_node_nd = GetNodeNetDeviceFromMac(ap_mac);
  Ptr<Node> ap_node = ap_node_nd.first;
  Ptr<NetDevice> ap_nd = ap_node_nd.second;

  NS_LOG_INFO("Deassociating node " << sta_node->GetId() << " from AP " << ap_node->GetId() << " (" << ap_mac << ")");

  /* Face management : L2 abstraction using EthernetNetDeviceFace */

  // Destroy the face on the station
  DestroyFace(sta_node, sta_nd, ap_mac);

#ifdef MLDR
  Ptr<L3Protocol> l3 = sta_node->GetObject<L3Protocol>();
  if(!NS3ReassocActivated() && !l3->getForwarder()->getFwPlugin().WifiReassocActivated())
  {
#endif
	  // Since there is no callback in ap-wifi-mac.*, let's also trigger face
	  // deletion on the AP side here. For this, we first need to retrieve the
	  // NetDevice involved in the association from the context.
	  DestroyFace(ap_node, ap_nd, sta_mac);
#ifdef MLDR
  }
#endif

  /* Routing : if GlobalRouter present in both nodes, update adjacencies */
  Ptr<GlobalRouter> sta_gr = sta_node->GetObject<GlobalRouter>();
  Ptr<GlobalRouter> ap_gr = ap_node->GetObject<GlobalRouter>();
  // XXX face as second argument ?
  if (sta_gr && ap_gr) {
    RemoveIncidency(sta_gr, ap_gr);
    RemoveIncidency(ap_gr, sta_gr);
  }
//#ifdef MLDR
//  RecomputeRoutes();
//#endif

  m_currentAP=NULL;

  m_mobilityEvent(Simulator::Now().ToDouble(Time::S), 'D', sta_node, ap_node);
}

/*-----------------------------------------------------------------------------
 * ConnectivityManagerGR
 *----------------------------------------------------------------------------*/

void
ConnectivityManagerGR::OnWifiAssoc(std::string context, Mac48Address ap_mac)
{

  ConnectivityManager::OnWifiAssoc(context, ap_mac);
  RecomputeRoutes();
}

void
ConnectivityManagerGR::OnWifiDeassoc(std::string context, Mac48Address ap_mac)
{
  ConnectivityManager::OnWifiDeassoc(context, ap_mac);
  RecomputeRoutes();
}

void 
ConnectivityManagerGR::onAttachTo(Ptr<Node> newBs)
{
    ConnectivityManager::onAttachTo(newBs);
    RecomputeRoutes();

}
void 
ConnectivityManagerGR::onDeattachFromCurrent()
{
    ConnectivityManager::onDeattachFromCurrent();
    RecomputeRoutes();

}

} // namespace ndn
} // namespace ns3
