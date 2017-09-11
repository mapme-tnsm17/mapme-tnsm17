/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef CONNECTIVITY_MANAGER_H
#define CONNECTIVITY_MANAGER_H

/* From extensions since BEACON_MISSING needs to be public */
#include "sta-wifi-mac.h"

#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM/model/ndn-global-router.hpp"
#include "ns3/mac48-address.h"


#include "options.hpp"
#include "my-global-routing-helper.hpp"

//for simulating wired links
#include <unordered_map>

#define EMPTY ""

#define MAPME_DEFAULT_TN                  5000 /* ms */
#define MAPME_DEFAULT_TU                  5000 /* ms */
#define MAPME_DEFAULT_TR                200000 /* ms */
#define MAPME_DEFAULT_T_RETRANSMISSION      50 /* ms */

#include "ns3/ndnSIM/NFD/core/scheduler.hpp"

// For the EthernetNetDeviceFace
#include "ns3/ndnSIM/model/ndn-net-device-face.hpp"
#include "ns3/mac48-address.h"
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
//#include "ndn-ns3.hpp"

#ifdef MLDR
#include "ns3/ndnSIM/NFD/daemon/fw/ns3-reassociation-procedure.hpp"
#endif

namespace ns3 {
namespace ndn {

/*----------------------------------------------------------------------------*/

class EthernetNetDeviceFace : public NetDeviceFace {
public:
  static TypeId GetTypeId (void);

  EthernetNetDeviceFace(Ptr<Node> node, const Ptr<NetDevice>& netDevice, Mac48Address destAddress);

  Mac48Address
  GetDestAddress() const;

  void
  SetDestAddress(Mac48Address destAddress);

private:
  virtual void
  send(Ptr<Packet> packet);

  virtual void
  receiveFromNetDevice(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol,
                       const Address& from, const Address& to, NetDevice::PacketType packetType);

	Mac48Address m_destAddress;

};

/*----------------------------------------------------------------------------*/

/**
 * \ingroup connectivity
 * \brief manages the network connectivity of a node.
 *
 */
class ConnectivityManager : public Object
{
public:
  static TypeId GetTypeId (void);

  ConnectivityManager ();
  ~ConnectivityManager () {};

  void NotifyNewAggregate();

public:
  void SetPrefix(std::string prefix);

  //wired:
  bool isWired();
  
  void setWired(bool isWired, double cell_size);
  
  static void setApMaps(NodeContainer leaves, Options& options);
  
  virtual void onAttachTo(Ptr<Node> newBs);
  virtual void onDeattachFromCurrent();
  
  Ptr<Node> getCurrentAp();
  //end
  

  TracedCallback<double, char, Ptr<Node>, Ptr<Node>> m_mobilityEvent;

#ifdef WLDR
  void enable_wldr();
#endif

#ifdef MLDR
  void ActivateNs3Reassoc()
  {
	  m_ns3Reassoc_activated = true;
  }
  bool
  NS3ReassocActivated()
  {
	  return m_ns3Reassoc_activated;
  }
  void
  DestroyKnownFace(Ptr<Node> node, shared_ptr<Face> face);

  void
  ActivateGlobalRouting()
  {
	  this->m_global_routing = true;
  }
  bool
  GlobalRoutingActivated() const
  {
	  return this->m_global_routing;
  }
  void
  ApplyMLDROnFaceCreation(Ptr<Node> node, shared_ptr<Face> face);

  void
  ApplyMLDROnFaceDestruction(Ptr<Node> node, shared_ptr<Face> face);

#endif

  void
  BeforeMobility();

  void
  AfterMobility(Ptr<Node> current_leaf);

protected:  

  //unsigned int getNodeId(const std::string & path);

  Ptr<StaWifiMac> GetNodeMac();

  shared_ptr<Face>
  CreateFace(Ptr<Node> node, Ptr<NetDevice> netDevice, Mac48Address mac);
  
  shared_ptr<Face>
  CreateWiredFace(Ptr<Node> from, Ptr<Node> to, Ptr<NetDevice> fromdevice);
  
  void
  DestroyFace(Ptr<Node> node, Ptr<NetDevice> netDevice, Mac48Address mac);
  
  void
  DestroyWiredFace(Ptr<Node> from, Ptr<Node> to);

	void
	ClearFIB(Ptr<Node> node, Name const & prefix);

	void
	ClearFIBs(Name const & prefix);

  void
  RecomputeRoutes();

  /**
   * \brief Handle WiFi deassociation events
   */
  virtual void OnWifiDeassoc(std::string context, Mac48Address ap);

  /**
   * \brief Handle WiFi association events
   */
  virtual void OnWifiAssoc(std::string context, Mac48Address ap);
  
  Ptr<Node> m_currentAP;
  
  Ptr<NetDevice> GetPointToPointNetDevice(Ptr<Node> from, Ptr<Node> to);
  

private:
  std::vector<std::string> 
  GetElementsFromContext (std::string context);

  Ptr<NetDevice>
  GetNetDeviceFromContext (std::string context);

  std::pair<Ptr<Node>, Ptr<NetDevice>>
  GetNodeNetDeviceFromMac(Mac48Address mac);

  shared_ptr<Face> 
  CreateFace(Ptr<Node> node, NetDevice & nd, Mac48Address mac);

  void
  RemoveIncidency(Ptr<GlobalRouter> gr, Ptr<GlobalRouter> remote_gr);
  
  void
  RemoveIncidencyAndFace(Ptr<GlobalRouter> gr, Ptr<GlobalRouter> remote_gr);

//for wired  
  double getBase(double x);
  
  void checkHandover(Ptr<Node> mobile, uint32_t cell_size, std::unordered_map<int, Ptr<Node>>& maps);
  
  static std::unordered_map<int, Ptr<Node> > getApMaps(NodeContainer leaves, Options& options);
  
  static std::unordered_map<int, Ptr<Node>> m_ap_maps;
  static uint32_t m_max_x_indx;
  static uint32_t m_max_y_indx;

//end

  /**
   * Map : AP BSSID (mac48address)  -> AP node
   *
   * Used for handling WiFi events
   * NOTE: this information is shared by all connectivity manager
   */
  static std::map<Mac48Address, std::pair<Ptr<Node>, Ptr<NetDevice>>> m_ap_mac2node;

  bool m_registered;
  bool m_wired;
  bool m_firstAttach;

#ifdef WLDR
  bool m_wldr;
#endif

#ifdef MLDR
  bool m_ns3Reassoc_activated;
  Ns3ReassociationProcedure m_ns3Reassoc;
  bool m_global_routing;
  bool m_firstAttachFinished;
#endif
};

/*-----------------------------------------------------------------------------
 * ConnectivityManagerGR
 *----------------------------------------------------------------------------*/

class ConnectivityManagerGR : public ConnectivityManager
{
protected:
  virtual void
  OnWifiAssoc(std::string context, Mac48Address ap);

  virtual void 
  OnWifiDeassoc(std::string context, Mac48Address ap);
  
  
  virtual void onAttachTo(Ptr<Node> newBs);
  virtual void onDeattachFromCurrent();
};



} // namespace ndn
} // namespace ns3

#endif /* CONNECTIVITY_MANAGER_H */
