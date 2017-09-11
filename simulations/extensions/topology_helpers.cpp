#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/constant-position-mobility-model.h"

#include "topology_helpers.hpp"
#include "util.hpp"
#include "options.hpp"

#include <sstream>

//#define SQRT2 1.41421356237

namespace ns3 {
namespace ndn {

NetDeviceContainer
setupWired(NodeContainer & AP, NodeContainer & Clients, Options & options)
{
    NetDeviceContainer ndc;
for(uint32_t j=0; j< Clients.GetN();j++){
    for (uint32_t i = 0; i < AP.GetN(); i++)
    {
      NodeContainer nodes;
      nodes.Add(AP.Get(i));
      nodes.Add(Clients.Get(j));
//       std::cout<<"add link between "<<AP.Get(i)->GetId()<<" and "<<Clients.Get(j)->GetId()<<"\n";

      PointToPointHelper pointToPoint;
      pointToPoint.SetDeviceAttribute ("DataRate", StringValue (options.capacity_s));
      pointToPoint.SetChannelAttribute ("Delay", StringValue ("0.1ms"));//neglible with respect to wired link delay
      pointToPoint.Install (nodes);
//       std::cout<<"after install node="<<Clients.Get(j)->GetId()<<", has N netdevices="<<Clients.Get(j)->GetNDevices();
    }
}
    return ndc; // Useless
}

NetDeviceContainer
setupVhtWifi(NodeContainer & AP, NodeContainer & Clients, Options & options)
{
    NetDeviceContainer devices;
    
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    // AP range has to be lower than half the distance between APs
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(options.cell_size / 2));

    //set physical layer channel model
    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    
    // Set physical layer channel guard interval(long(false) or short(true))
    bool k=true;
    wifiPhy.Set ("ShortGuardEnabled", BooleanValue (k));
    
    //create wifi helper, set standard to 80211ac
    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetStandard (WIFI_PHY_STANDARD_80211ac);
    
    //create vht wifi mac layer helper
    VhtWifiMacHelper wifiMac = VhtWifiMacHelper::Default ();
    
    //set remote station manager
    int i=9;//set MCS=9, fastest
    StringValue DataRate = VhtWifiMacHelper::DataRateForMcs (i);
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
        "DataMode", DataRate,
        "ControlMode", DataRate,
        "RtsCtsThreshold", UintegerValue (10000), 
        "IsLowLatency" , BooleanValue(false),
        "NonUnicastMode" , DataRate
        );
    
    //ssid
    Ssid ssid = Ssid ("ns3-80211ac");
    
    //configure mac layer for clients
    wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue(ssid),
            "ActiveProbing", BooleanValue(false), "MaxMissedBeacons", UintegerValue(2));
    
    //install wifi on client nodes
    devices=wifi.Install (wifiPhy, wifiMac, Clients);
    
    //configure mac layer for AP
    wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    
    //install wifi on AP node
    devices.Add(wifi.Install (wifiPhy, wifiMac, AP));
    
    // Set channel width
    int j=40;//channel width could be 20, 40, 80, 160 MHz
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (j));
    
    return devices;
}


              
              
NetDeviceContainer
setupWifi_new_radio(NodeContainer & AP, NodeContainer & Clients, Options & options)
{//following IUT-R M1225 and IUT-R P.1411 https://www.itu.int/rec/R-REC-P.1411/en
              
              double frequency=5.0;
              //for 802.11n 2-LEVEL data frame aggregation: 
              int nMpdu=7;
              int nMsdu=4;
              int payloadSize=BITS_TO_BYTES(options.chunk_size);
              int interestSize=64;
              
              double exponent;
              double referenceLoss; 
              double slow_fading_variance;
              
              double fading_margin=40; //dBm
              
              if (options.radio_model=="slos"){ // Suburban LOS
                  slow_fading_variance=5.06*5.06;
                  exponent=2.12;
                  referenceLoss = 29.2+10*2.11*0.7;//log10(5)=0.7
                  fading_margin=30+5.06; //dBm

              } else if (options.radio_model=="unlos"){ // urban No LOS also default one
                  slow_fading_variance=7.6*7.6;
                  exponent=4;
                  referenceLoss = 10.2+10*2.36*0.7;//log10(5)=0.7
                  fading_margin=30+7.6; //dBm

              } else {
                  std::cout<<options.radio_model <<" radio model not defined\n";
                  exit(1);
            }
              

              NetDeviceContainer devices;

              //YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
              YansWifiChannelHelper channel
//               =YansWifiChannelHelper::Default()
              ;
              
              channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
              // AP range has to be lower than half the distance between APs
             // channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(options.cell_size * 0.707));
//             //  channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
//                                      "m0", DoubleValue(1.0),
//                                      "m1", DoubleValue(1.0),
//                                      "m2", DoubleValue(1.0));
              
              //path loss:
               channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                                     "Exponent", DoubleValue(exponent),
                                     "ReferenceLoss", DoubleValue(referenceLoss));
               
               //slow fading:
               std::ostringstream slowfading_s;
               slowfading_s<<"ns3::NormalRandomVariable[Mean=0|Variance="<<slow_fading_variance<<"]";

               
//                channel.AddPropagationLoss("ns3::RandomPropagationLossModel",
//                                           "Variable",StringValue(slowfading_s.str()) //indoor=12db, outdoor=10db
//                                          );
              
               
               //fast fading:
                channel.AddPropagationLoss("ns3::JakesPropagationLossModel");
              
              YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

              phy.SetChannel (channel.Create ());
              

              // Set guard interval
              phy.Set ("ShortGuardEnabled", BooleanValue (1));
              
              
              //for fading margin:
//               phy.Set ("TxPowerStart", DoubleValue(16.0206+fading_margin));
//               phy.Set ("TxPowerEnd", DoubleValue(16.0206+fading_margin));

              WifiHelper wifi = WifiHelper::Default ();
              
              if (frequency == 5.0)
                {
                  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
                }
              else if (frequency == 2.4)
                {
                  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
                  Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
                }
              else
                {
                  std::cout<<"Wrong frequency value!"<<std::endl;
                  return NetDeviceContainer();
                }
                
             HtWifiMacHelper mac = HtWifiMacHelper::Default ();
             


             /* no rate adaptation:
              StringValue DataRate = HtWifiMacHelper::DataRateForMcs (i);
              wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", DataRate
                                           );
              */

              StringValue HtMcs0Rate = HtWifiMacHelper::DataRateForMcs (0);
             /* 802.11n minstrel rate adaptation */
             wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager"
                           //                  ,"LookAroundRate",DoubleValue(10),"SampleColumn",DoubleValue(30)
                           //                   ,"NonUnicastMode",HtMcs0Rate //to change beacon sending rate to lowest HT rate
                                               );
                
             Ssid ssid = Ssid ("wifi-default");      
             
             /*for station: */     
             // -------------frame aggregation(aggregate data packets at producer)----------
             mac.SetMpduAggregatorForAc (AC_BE, "ns3::MpduStandardAggregator"
             ,"MaxAmpduSize", UintegerValue (nMpdu * (nMsdu * (payloadSize + 100)))
              );
              mac.SetMsduAggregatorForAc (AC_BE, "ns3::MsduStandardAggregator"
              ,"MaxAmsduSize", UintegerValue (nMsdu * (payloadSize + 100))
              );
              /* NOTE: the block ack mechanism in current ns3 release is not robust,
               * if runtime error occurs in the simulation, try to remove the following 
               * line of code, 
               * without block ack we can still achieve a maximum
               * throughput of 70Mbits/s in 802.11n with NDN 
               */
              mac.SetBlockAckThresholdForAc (AC_BE, 2);
              mac.SetBlockAckInactivityTimeoutForAc (AC_BE, 400);
              
              mac.SetType ("ns3::StaWifiMac"
                           ,"Ssid", SsidValue (ssid)
                           ,"ActiveProbing", BooleanValue (true)
                           ,"MaxMissedBeacons", UintegerValue(3)
                           ,"AssocRequestTimeout",TimeValue (Seconds (0.05))
                           ,"ProbeRequestTimeout",TimeValue (Seconds (0.05))
                          );
              NetDeviceContainer staDevice;
              staDevice = wifi.Install (phy, mac, Clients);                   
              
              /*for AP */
              // -------------frame aggregation(aggregate interest packets at consumer)-------
              mac.SetMpduAggregatorForAc (AC_BE, "ns3::MpduStandardAggregator"
             ,"MaxAmpduSize", UintegerValue (nMpdu * (nMsdu * (interestSize + 10)))
              );
              mac.SetMsduAggregatorForAc (AC_BE, "ns3::MsduStandardAggregator"
              ,"MaxAmsduSize", UintegerValue (nMsdu * (interestSize + 10))
              );
              //------------------------------------------------------------------------------
              mac.SetType ("ns3::ApWifiMac"
                           ,"Ssid", SsidValue (ssid)
                           ,"EnableBeaconJitter",BooleanValue (true)
                         //  ,"BeaconInterval", TimeValue (MicroSeconds (51200))
                          );
              NetDeviceContainer apDevice;
              apDevice = wifi.Install (phy, mac, AP);

              // Set channel width
              Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));

              devices.Add(apDevice);
              devices.Add(staDevice);
               
              return devices;
}

NetDeviceContainer
setupWifi(NodeContainer & AP, NodeContainer & Clients, Options & options)
{
    NetDeviceContainer devices;

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default (); 
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default (); 
    phy.SetChannel (channel.Create ()); 

    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetStandard (WIFI_PHY_STANDARD_80211g); 
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();

    /* XXX This lead to many losses XXX
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("ErpOfdmRate54Mbps"));
    */
    wifi.SetRemoteStationManager 
      ("ns3::ConstantRateWifiManager","DataMode", StringValue 
       ("ErpOfdmRate54Mbps"), "ControlMode", StringValue ("ErpOfdmRate54Mbps"), 
       "RtsCtsThreshold", UintegerValue (10000), "IsLowLatency" , BooleanValue 
       (false) , "NonUnicastMode" , StringValue ("ErpOfdmRate54Mbps") );

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    // AP range has to be lower than half the distance between APs
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(options.cell_size / 2));

    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());

    Ssid ssid = Ssid ("wifi-default");
    wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    devices = wifi.Install (wifiPhy, wifiMac, AP);

    wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue(ssid),
            "ActiveProbing", BooleanValue(false), "MaxMissedBeacons", UintegerValue(2));
    devices.Add(wifi.Install (wifiPhy, wifiMac, Clients));

    /* 
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    positionAlloc->Add (Vector (5.0, 0.0, 0.0));
    positionAlloc->Add (Vector (0.0, 5.0, 0.0));
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (Clients);
    */

    return devices;
}

NetDeviceContainer
setup80211nWifi(NodeContainer & AP, NodeContainer & Clients, Options & options)
{
    	      //parameters for 802.11n wifi
              double frequency=5.0;
              //for 802.11n 2-LEVEL data frame aggregation: 
	      int nMpdu=7;
	      int nMsdu=4;
	      int payloadSize=BITS_TO_BYTES(options.chunk_size);
	      int interestSize=64;

              NetDeviceContainer devices;

              
              //YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	      YansWifiChannelHelper channel
	      =YansWifiChannelHelper::Default()
	      ;
	      
	      //channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	      // AP range has to be lower than half the distance between APs
	     // channel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(options.cell_size * 0.707));
  	      channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
                                     "m0", DoubleValue(1.0),
                                     "m1", DoubleValue(1.0),
                                     "m2", DoubleValue(1.0));
              YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

              phy.SetChannel (channel.Create ());
	      

              // Set guard interval
              phy.Set ("ShortGuardEnabled", BooleanValue (1));

              WifiHelper wifi = WifiHelper::Default ();
	      
	      if (frequency == 5.0)
                {
                  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
                }
              else if (frequency == 2.4)
                {
                  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
                  Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
                }
              else
                {
                  std::cout<<"Wrong frequency value!"<<std::endl;
                  return NetDeviceContainer();
                }
                
             HtWifiMacHelper mac = HtWifiMacHelper::Default ();
	     


	     /* no rate adaptation:
              StringValue DataRate = HtWifiMacHelper::DataRateForMcs (i);
              wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", DataRate
					   );
	      */

	      StringValue HtMcs0Rate = HtWifiMacHelper::DataRateForMcs (0);
	     /* 802.11n minstrel rate adaptation */
	     wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager"
                           //                  ,"LookAroundRate",DoubleValue(10),"SampleColumn",DoubleValue(30)
	                   //                   ,"NonUnicastMode",HtMcs0Rate //to change beacon sending rate to lowest HT rate
                                               );
                
	     Ssid ssid = Ssid ("wifi-default");	     
	     
	     /*for station: */     
	     // -------------frame aggregation(aggregate data packets at producer)----------
	     mac.SetMpduAggregatorForAc (AC_BE, "ns3::MpduStandardAggregator"
	     ,"MaxAmpduSize", UintegerValue (nMpdu * (nMsdu * (payloadSize + 100)))
	      );
              mac.SetMsduAggregatorForAc (AC_BE, "ns3::MsduStandardAggregator"
	      ,"MaxAmsduSize", UintegerValue (nMsdu * (payloadSize + 100))
	      );
	      /* NOTE: the block ack mechanism in current ns3 release is not robust,
	       * if runtime error occurs in the simulation, try to remove the following 
	       * line of code, 
	       * without block ack we can still achieve a maximum
	       * throughput of 70Mbits/s in 802.11n with NDN 
	       */
 	      mac.SetBlockAckThresholdForAc (AC_BE, 2);
	      mac.SetBlockAckInactivityTimeoutForAc (AC_BE, 400);
              
              mac.SetType ("ns3::StaWifiMac"
                           ,"Ssid", SsidValue (ssid)
                           ,"ActiveProbing", BooleanValue (true)
			   ,"MaxMissedBeacons", UintegerValue(3)
			   ,"AssocRequestTimeout",TimeValue (Seconds (0.05))
			   ,"ProbeRequestTimeout",TimeValue (Seconds (0.05))
			  );
              NetDeviceContainer staDevice;
              staDevice = wifi.Install (phy, mac, Clients);	      	      
	      
	      /*for AP */
	      // -------------frame aggregation(aggregate interest packets at consumer)-------
	      mac.SetMpduAggregatorForAc (AC_BE, "ns3::MpduStandardAggregator"
	     ,"MaxAmpduSize", UintegerValue (nMpdu * (nMsdu * (interestSize + 10)))
	      );
              mac.SetMsduAggregatorForAc (AC_BE, "ns3::MsduStandardAggregator"
	      ,"MaxAmsduSize", UintegerValue (nMsdu * (interestSize + 10))
	      );
	      //------------------------------------------------------------------------------
              mac.SetType ("ns3::ApWifiMac"
                           ,"Ssid", SsidValue (ssid)
			   ,"EnableBeaconJitter",BooleanValue (true)
			 //  ,"BeaconInterval", TimeValue (MicroSeconds (51200))
			  );
              NetDeviceContainer apDevice;
              apDevice = wifi.Install (phy, mac, AP);

              // Set channel width
              Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (40));

              devices.Add(apDevice);
	      devices.Add(staDevice);
               
              return devices;
}


NetDeviceContainer
setupWifiInTheWild(NodeContainer & AP, NodeContainer & Clients)
{
    NetDeviceContainer devices;
        
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());
        
    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
        
    /* XXX This lead to many losses XXX
     wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("ErpOfdmRate54Mbps"));
     */
    wifi.SetRemoteStationManager
    ("ns3::ConstantRateWifiManager","DataMode", StringValue
    ("OfdmRate24Mbps"), "ControlMode", StringValue ("OfdmRate24Mbps"),
    "RtsCtsThreshold", UintegerValue (0), "IsLowLatency" , BooleanValue
    (false) , "NonUnicastMode" , StringValue ("OfdmRate24Mbps") );
    //wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
        
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
    // AP range has to be lower than half the distance between APs
    wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(150));
    wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
                                   "m0", DoubleValue(1.0),
                                   "m1", DoubleValue(1.0),
                                   "m2", DoubleValue(1.0));

    
    
    YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
    wifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-93));  //dBm, default value is -96dBm

    wifiPhy.SetChannel (wifiChannel.Create ());
        
    Ssid ssid = Ssid ("wifi-default");
    wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    devices = wifi.Install (wifiPhy, wifiMac, AP);
    
    wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(true));
    devices.Add(wifi.Install (wifiPhy, wifiMac, Clients));
        
    return devices;
}

    
    
/**
 * \brief Setup LTE links
 */
void
setupLTE()
{
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  // Create Node objects for the eNB(s) and the UEs
  NodeContainer enbNodes;
  enbNodes.Create (1);
  NodeContainer ueNodes;
  ueNodes.Create (2);

  // Configure the Mobility model for all the nodes:
  // The above will place all nodes at the coordinates (0,0,0). Please refer to
  // the documentation of the ns-3 mobility model for how to set your own
  // position or configure node movement.
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ueNodes);

  // Install an LTE protocol stack on the eNB(s):
  NetDeviceContainer enbDevs;
  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  // Install an LTE protocol stack on the UEs:
  NetDeviceContainer ueDevs;
  ueDevs = lteHelper->InstallUeDevice (ueNodes);

  // Attach the UEs to an eNB. This will configure each UE according to the eNB
  // configuration, and create an RRC connection between them:
  lteHelper->Attach (ueDevs, enbDevs.Get (0));

  // Activate a data radio bearer between each UE and the eNB it is attached to:
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);


}
  
/**
 * \brief Sets Internet Point of Access positions in the map, reading their positions from file
 */
int
setInternetPoAPosition(std::string file, NodeContainer &internetPoA)
{
  MobilityHelper staticMobility;
  staticMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticMobility.Install (internetPoA);
  
  
  
  std::ifstream fileInput;
  fileInput.open(file.c_str(), std::ios::in);
  if (!fileInput.is_open()) {
    //NS_LOG_ERROR("Invalid internet PoA map. The path (" << file << " is wrong. Please check it again");
    return -1;
  }
  
  uint32_t counter=0;
  double x = 0, y = 0, z = 0;
  while (fileInput >> x) {
    fileInput >> y >> z;
    if(counter<internetPoA.size()){
       internetPoA.Get(counter)->GetObject<ConstantPositionMobilityModel>()->SetPosition (Vector (x,y,z));
       counter++;
     } else {
       //NS_LOG_ERROR("The number of IPoA is too high, it should be " << internetPoA.size());
       return -1;
     }
  }
  if(counter != internetPoA.size()){
    //NS_LOG_ERROR("The number of IPoA " << counter << " is too low, it should be " << internetPoA.size());
    return -1;
  }
  return 0;
}
  
void
addLink(Ptr<Node> n1, Ptr<Node> n2)
{
  NodeContainer nodes;
  nodes.Add(n1);
  nodes.Add(n2);
    
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
  pointToPoint.Install (nodes);
}


} // namespace ndn
} // namespace ns3
