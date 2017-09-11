/* synthetic: evaluation of mobility proposal with synthetic mobility */

#include <cmath>
#include <iomanip>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/random-variable-stream.h"

//for random 2d mobility model:
#include "ns3/random-walk-2d-mobility-model.h"

// Use topology with AP at the border too
//#define WITH_BORDER_AP 1

// for adding links -> topology
#include "ns3/point-to-point-module.h"
//#include "ns3/point-to-point-layout-module.h"


#ifdef MAPME
//for logging of signaling overhead
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-mapme.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-anchor.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-kite.hpp"
#endif

//for anchor based approach setup
#include "boost/lexical_cast.hpp"

// For dynamic face creation in WiFi
#include "ns3/wifi-net-device.h"

#ifdef NS3_AS_LIB
#define INC(x) x
#else
#define INC(x) ns3/x
#endif

#include INC("ap-manager.hpp")
#include INC("connectivity-manager.hpp")
#include INC("markov-mobility-model.hpp")
#include INC("my-global-routing-helper.hpp")
#include INC("options.hpp")
#include INC("ndn-base-station.hpp")
#include INC("ndn-anchor.hpp")
#include INC("ndn-l3-rate-tracer-ng.hpp")
#include INC("topology_helpers.hpp")
#include INC("traffic_helpers.hpp")
#include INC("util.hpp")

#ifdef WITH_BORDER_AP
#include INC("mobile_fat_tree_single_path_with_border.hpp")
#else
#include INC("mobile_fat_tree.hpp")
#endif

#define WITH_PRODUCER_HANDOVER_LOGGING 1
#define TOPOLOGY_SCALE 25
#define TOPOLOGY_PATH "topologies/out/"

//for rocket fuel topology:
#include "ns3/node-list.h"
#include "ns3/names.h"


/* TODO
 *  - in optimized more, logging is disabled, let's redefine logging macros to
 *  only display INFO messages on std::cout.
 */
NS_LOG_COMPONENT_DEFINE("synthetic");

static const std::set<std::string> topologies = {
  "cycle",
  "grid-2d",
  "hypercube",
  "expander",
  "erdos-renyi",
  "regular",
  "watts-strogatz",
  "small-world",
  "barabasi-albert",
  "rocketfuel-1755",
  "rocketfuel-3257",
  "rocketfuel-6461"
};

static const std::set<std::string> markov_models = {
  "markov-uniform",
  "markov-rw",
  "markov-rwp"
};

const std::string PREFIX = "/prefix";

//#define DEFAULT_STRATEGY "/localhost/nfd/strategy/load-balance"
#define DEFAULT_STRATEGY "/localhost/nfd/strategy/best-route"


/////Trace based simulation settings
#define TRACE_BASED_MAP_SIZE 2100
#define TRACE_BASED_NUM_LEAVES 729
//#define TRACE_BASED_MIN_PRODUCER_LIFETIME 240  //580 //240 //min time spent by a node inside the map to be eligible as producer  todo TRACE_BASED

#ifdef WITH_BORDER_AP
#define NUM_LEAVES 36
#else
#define NUM_LEAVES 16
#endif

// setupWifi: 802.11g
// setup80211nWifi: 802.11n (best)
// setupVhtWifi : 802.11ac (not working)
#define SETUP_WIFI setup80211nWifi

namespace ns3 {
namespace ndn {

/*------------------------------------------------------------------------------
 * Mobility helpers
 *
 * TODO migrate to a separate extension
 *----------------------------------------------------------------------------*/
struct MobilityItem
{
  double x;
  double y;
  double z;
  double time;
};

std::string getSpeedString(Options & options)
{
     std::ostringstream speed_s;
     speed_s << "ns3::ConstantRandomVariable[Constant=" << options.speed << "]";
     return speed_s.str();
}

Ptr<RandomRectanglePositionAllocator> getPositionAllocator(Options & options)
{
  Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
  Config::SetDefault ("ns3::UniformRandomVariable::Min", DoubleValue (0));
  Config::SetDefault ("ns3::UniformRandomVariable::Max", DoubleValue (4 * options.cell_size));
  Ptr<UniformRandomVariable> rv_x = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> rv_y = CreateObject<UniformRandomVariable> ();
  positionAlloc->SetX (rv_x);
  positionAlloc->SetY (rv_y);
  return positionAlloc;
}

void setupRandomWaypointMobility(NodeContainer & nodes, Options & options)
{
  std::cout << "Creating mobility with random waypoint..." << std::endl;

  MobilityHelper mobility;

  Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator> ();
  Config::SetDefault ("ns3::UniformRandomVariable::Min", DoubleValue (0));
  Config::SetDefault ("ns3::UniformRandomVariable::Max", DoubleValue (4 * options.cell_size));
  Ptr<UniformRandomVariable> rv_x = CreateObject<UniformRandomVariable> ();
  Ptr<UniformRandomVariable> rv_y = CreateObject<UniformRandomVariable> ();
  positionAlloc->SetX (rv_x);
  positionAlloc->SetY (rv_y);

  std::ostringstream speed_s, pause_s;
  speed_s << "ns3::ConstantRandomVariable[Constant=" << options.speed << "]";
  pause_s << "ns3::ConstantRandomVariable[Constant=" << options.pause << "]";

  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
      "Speed", StringValue (speed_s.str()),
      "Pause", StringValue (pause_s.str()),
      "PositionAllocator", PointerValue (positionAlloc));

  mobility.SetPositionAllocator(positionAlloc);

  mobility.Install(nodes);
}



//walk in smooth trajectory: no sudden changes in directions
void
setupGaussionMarkovMobilityModel(NodeContainer & nodes, Options & options){
       MobilityHelper mobility;

       double max_range=4 * options.cell_size;

       std::ostringstream normal_direction_s;
       normal_direction_s << "ns3::NormalRandomVariable[Mean=0.0|Variance="<< 0.08*options.speed<< "|Bound="<<1.6*options.speed  << "]";

       Ptr<RandomRectanglePositionAllocator> positionAlloc= getPositionAllocator(options);

       mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
      "Bounds", BoxValue (Box (0, max_range, 0, max_range, 0, 0)),
      "TimeStep", TimeValue (Seconds (0.5)),
      "Alpha", DoubleValue (0.85),
      "MeanVelocity", StringValue (getSpeedString(options)),
      "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
      "MeanPitch", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"),
       "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=1]"), //no speed variations
       "NormalDirection",StringValue (normal_direction_s.str()),
      "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=1]")
      );

      mobility.SetPositionAllocator(positionAlloc);

      mobility.Install (nodes);
}

//walk in straight line until a specific distance is reached, then choose new random direction
void
setupRandom2DWalkMobilityModel(NodeContainer & nodes, Options & options){
       MobilityHelper mobility;

       double max_range=4 * options.cell_size;
       double walk_distance= options.cell_size; //walking a distance more than diameter of the moving area before changing directions. This increase the chance of walking a long chain

       Ptr<RandomRectanglePositionAllocator> positionAlloc= getPositionAllocator(options);

       mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
      "Bounds", RectangleValue (Rectangle (0, max_range, 0, max_range)),
      //"Time", TimeValue (Seconds (0.5)),
      "Distance",DoubleValue (walk_distance),
      "Mode", EnumValue (RandomWalk2dMobilityModel::MODE_DISTANCE), //OR MODE_TIME
      "Speed", StringValue (getSpeedString(options)) // "Speed",  StringValue ("ns3::UniformRandomVariable[Min=5.0|Max=15.0]")
       );

      mobility.SetPositionAllocator(positionAlloc);

      mobility.Install (nodes);
}

//walk in straight line until boundary is reached, then choose new random direction
void
setupRandom2DDirectionMobilityModel(NodeContainer & nodes, Options & options){
       MobilityHelper mobility;

       double max_range=4 * options.cell_size;

       Ptr<RandomRectanglePositionAllocator> positionAlloc= getPositionAllocator(options);



       mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
      "Bounds", RectangleValue (Rectangle (0, max_range, 0, max_range)),
       "Speed", StringValue (getSpeedString(options)),
      "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]")
       );


      mobility.SetPositionAllocator(positionAlloc);

      mobility.Install (nodes);
}

// the return value is: <min time in the trace, max time in the trace>
std::pair<double, double> readMobilityFile(std::string mobFile, std::map<uint32_t, std::list<MobilityItem> > &mobileNodes, std::vector<uint32_t> &availableMobileNodes) throw()
{
  std::ifstream mobileInput;
  mobileInput.open(mobFile.c_str(), std::ios::in);
  if (!mobileInput.is_open()) {
    NS_LOG_ERROR("Invalid mobility file. The path (" << mobFile << " is wrong. Please check it again");
    throw;
  }

  uint32_t nodeNo = 0;
  double time = 0;
  double x = 0, y = 0, z = 0;
  double minTime = -1;
  double maxTime = -1;
  while (mobileInput >> nodeNo) {
    mobileInput >> time >> x >> y >> z;

    MobilityItem item;
    item.x = x;
    item.y = y;
    item.z = z;
    item.time = time;
    if(mobileNodes.find(nodeNo)==mobileNodes.end()){
      availableMobileNodes.push_back(nodeNo);
    }
    mobileNodes[nodeNo].push_back(item);

    if(minTime > time || minTime==-1){
      minTime = time;
    }
    if(maxTime < time || maxTime==-1){
      maxTime = time;
    }
  }

  ///mobility statistics
  double averageTime = 0;
  std::vector<double> lifetimeSamples;
  for(std::map<uint32_t, std::list<MobilityItem> >::iterator it=mobileNodes.begin(); it != mobileNodes.end(); it++){
    double time = it->second.back().time - it->second.front().time;

    if(time>0){
      lifetimeSamples.push_back(time);
      averageTime+=time;
    }
  }
  std::sort(lifetimeSamples.begin(), lifetimeSamples.end());
  NS_LOG_INFO("Lifetime 95th: " << lifetimeSamples[ (int) 95*lifetimeSamples.size()/100]);

  NS_LOG_INFO("Lifetime average: " << averageTime/lifetimeSamples.size());
  //end statistics

  return std::pair<double, double> (minTime, maxTime);
}

int pickApplicationAndInstallMobility(std::string mobFile, uint32_t numberOfProducer, double minProducerLife, NodeContainer &mobileUserC)
{

  MobilityHelper mobileNodesMobility;
  mobileNodesMobility.SetMobilityModel ("ns3::WaypointMobilityModel");
  mobileNodesMobility.Install (mobileUserC);

  int timeOffset = 0;

  std::map<uint32_t, std::list<MobilityItem> > mobileNodes;
  std::vector<uint32_t> availableMobileNodes;

  double startMobility = 0;
  double endMobility = 0;

  try{
    std::pair<double, double> res = readMobilityFile(mobFile,mobileNodes,availableMobileNodes);
    startMobility = res.first; //useless, but it helps reading the code
    endMobility = res.second;
  } catch(...){
    NS_LOG_ERROR("Something went wrong while reading the mobility file");
    return -1;
  }

  //selecting the portion of mobility we want to simulate
  Ptr<UniformRandomVariable> mobilityPortion=CreateObject<UniformRandomVariable>();
  int mobilityInferiorLimit = mobilityPortion->GetInteger(startMobility+minProducerLife, endMobility-minProducerLife) - minProducerLife ;

  int mobilitySuperiorLimit = mobilityInferiorLimit + minProducerLife;
  NS_LOG_INFO("Inferior mobility limit " << mobilityInferiorLimit << " Superior mobility limit: " << mobilitySuperiorLimit);

  //selecting the producers
  std::list<uint32_t> producers;
  Ptr<UniformRandomVariable> producerRandom=CreateObject<UniformRandomVariable>();
  double producerStartTime = 0;
  double producerEndTime = 0;

  //todo we shouldn't do this. it's a quick and dirty ack
  int debugIndex = 0;
  int secondIndex = 0;
  while(producers.size()< numberOfProducer){
    uint32_t randomNum = producerRandom->GetInteger(0,availableMobileNodes.size()-1);
    uint32_t newNum = availableMobileNodes[randomNum];
    debugIndex++;
    if(debugIndex>10000){

      if(secondIndex>100){
        NS_LOG_INFO("Failed - We had problem selecting producers, so far we selected " << producers.size() << " producers");
        return -1;
      } else {
        NS_LOG_INFO("Try again - We had problem selecting producers, so far we selected " << producers.size() << " producers");
        producers.clear();
        producerStartTime=0;
        producerEndTime=0;
        debugIndex=0;
        timeOffset=0;
        secondIndex++;

        mobilityInferiorLimit = mobilityPortion->GetInteger(startMobility+minProducerLife, endMobility-minProducerLife) - minProducerLife ;

        mobilitySuperiorLimit = mobilityInferiorLimit + minProducerLife;
        NS_LOG_INFO("Inferior mobility limit " << mobilityInferiorLimit << " Superior mobility limit: " << mobilitySuperiorLimit);
      }
    }

    if(std::find(producers.begin(), producers.end(), newNum)!=producers.end()){
      continue;
    }

    //reducing the size of the mobility
    if ((mobileNodes[newNum].front().time<mobilityInferiorLimit
         && mobileNodes[newNum].back().time < mobilitySuperiorLimit) ||
        (mobileNodes[newNum].front().time> mobilityInferiorLimit
         && mobileNodes[newNum].back().time > mobilitySuperiorLimit)){
      continue;
    }


    //checking if the producer stays enough in the map
    if(mobileNodes[newNum].back().time -
       mobileNodes[newNum].front().time > minProducerLife){
      //ok
      NS_LOG_DEBUG("New producer: " << newNum  << " time: " << mobileNodes[newNum].front().time << " - " << mobileNodes[newNum].back().time );

      producers.push_back(newNum);
      //availableMobileNodes.erase(availableMobileNodes.begin()+randomNum);
      if(producerStartTime==0 || producerStartTime> mobileNodes[newNum].front().time){
        producerStartTime = mobileNodes[newNum].front().time;
        timeOffset = producerStartTime;
      }

      if(producerEndTime==0||producerEndTime<mobileNodes[newNum].back().time){
        producerEndTime = mobileNodes[newNum].back().time;
      }
      std::cout<<"New producer: " << newNum  << "time: " << mobileNodes[newNum].front().time << " - " << mobileNodes[newNum].back().time<<"\n";
    }

  }
  timeOffset = mobilityInferiorLimit;

  int simulationEndTime = mobilitySuperiorLimit - mobilityInferiorLimit;//producerEndTime - timeOffset;

  uint32_t nNodes = mobileUserC.GetN();
  Ptr<WaypointMobilityModel> wayMobility[nNodes];
  for (uint32_t i = 0; i < nNodes; i++) {
    wayMobility[i] = mobileUserC.Get(i)->GetObject<WaypointMobilityModel>();
  }

  //installing mobility on the producers
  double sinkX = 100000;
  double sinkY = 100000;
  double sinkZ = 0;
  int counter = 0;
  for (std::list<uint32_t>::iterator it = producers.begin(); it!=producers.end(); it++){

    //setting sink point at the beggining of the simulation
    Simulator::Schedule(Seconds(0.1), &WaypointMobilityModel::SetPosition, wayMobility[counter], Vector3D(sinkX + (counter * 10000), sinkY + (counter * 10000), sinkZ));

    for(std::list<MobilityItem>::iterator mobilityIt = mobileNodes[*it].begin();
        mobilityIt!=mobileNodes[*it].end(); mobilityIt++){

      Waypoint waypoint(Seconds(mobilityIt->time - timeOffset), Vector3D(mobilityIt->x, mobilityIt->y, mobilityIt->z));

      wayMobility[counter]->AddWaypoint(waypoint);

      //NS_LOG_DEBUG("Producer pos: " << mobilityIt->time << " " << mobilityIt->x << " " <<mobilityIt->y);
    }
    counter++;
  }
  return simulationEndTime;
}

void printPosition(Ptr<const MobilityModel> mobility) //DEBUG purpose
{
  Simulator::Schedule(Seconds(1), &printPosition, mobility);
  NS_LOG_INFO("Car "<<  mobility->GetObject<Node>()->GetId() << " is at: " <<mobility->GetPosition());

}

void DownloadFailure(uint32_t appId, double totalDownloadByte, double bytePerSec)
{
  NS_LOG_INFO("App " << appId <<
              " stopped playing. Total downloaded MBytes: " << totalDownloadByte/1000000 <<
              " Bytes/sec: " << bytePerSec);
  //std::cout<<Simulator::Now() << ": App " << appId <<
  //            " stopped playing. Total downloaded MBytes: " << totalDownloadByte/1000000 <<
  //            " Bytes/sec: " << bytePerSec<<"\n";
}

void DownloadSuccess(uint32_t appId, double totalDownloadByte, double bytePerSec){
  NS_LOG_INFO("App " << appId <<
              "played correctly the entire stream. Total downloaded MBytes: " << totalDownloadByte/1000000 <<
              " Bytes/sec: " << bytePerSec);
}

void setupMarkovMobility(NodeContainer & nodes, Options & options, NodeContainer & Leaves)
{
  for (unsigned int i = 0; i < nodes.GetN(); i++) {
    Ptr<Node> node = nodes.Get(i);
    // Use Markov Mobility Model : pause == mean_sleep
    Ptr<MarkovMobilityModel> mmm = CreateObject<MarkovMobilityModel>(); //Leaves, options.pause);
    std::cout << "MM set nodes " << Leaves.GetN() << std::endl;
    mmm->SetNodes(&Leaves);
    mmm->SetMeanSleep(options.pause);
    mmm->SetModel(options.mobility_model);
    node->AggregateObject(mmm);
  }
}

/*------------------------------------------------------------------------------
 * DummyNetDeviceFace
 *----------------------------------------------------------------------------*/

class DummyNetDeviceFace : public NetDeviceFace {
public:
  DummyNetDeviceFace(Ptr<Node> node, const Ptr<NetDevice>& netDevice)
  : NetDeviceFace(node, netDevice)
  {}

private:
  virtual void send(Ptr<Packet> packet) {}

  virtual void receiveFromNetDevice(Ptr<NetDevice> device, Ptr<const Packet> p,
      uint16_t protocol, const Address& from, const Address& to,
      NetDevice::PacketType packetType) {}

};

shared_ptr<NetDeviceFace>
dummyNetDeviceFaceCallBack (Ptr<Node> node, Ptr<ns3::ndn::L3Protocol> ndn, Ptr<ns3::NetDevice> netDevice)
{
  return std::make_shared<DummyNetDeviceFace>(node, netDevice);
}

/*------------------------------------------------------------------------------
 * Statistics output
 *----------------------------------------------------------------------------*/

void
onHandover(Ptr<OutputStreamWrapper> stream, double time, char type, Ptr<Node> sta, Ptr<Node> ap)
{
  static uint64_t id = 0;
  *stream->GetStream () << std::fixed << std::setprecision(6)
    << time << "\t" << ++id << "\t" << type << "\t" << sta->GetId() << "\t"
    << ap->GetId() << std::endl;
}

void
onSpecialInterest(Ptr<OutputStreamWrapper> stream, uint64_t nodeId, std::string prefix, uint64_t seq, uint64_t ttl, uint64_t num_retx)
{
  *stream->GetStream () << Simulator::Now().ToDouble(Time::S) << "\t" << nodeId
    << "\t" << prefix << "\t" << seq << "\t" << ttl << "\t" << num_retx << std::endl;
}

// XXX This should be replaced by onSpecialInterest
void
onSignalingOverhead(Ptr<OutputStreamWrapper> stream, uint32_t num_routers)
{
  *stream->GetStream () << 0 << "\t" << num_routers << std::endl;
}

void
onProcessing(Ptr<OutputStreamWrapper> stream, uint64_t nodeId, std::string prefix, uint64_t seq, uint64_t ttl, uint64_t num_retx)
{
  *stream->GetStream () << Simulator::Now().ToDouble(Time::S) << "\t" << nodeId
    << "\t" << prefix << "\t" << seq << "\t" << ttl << "\t" << num_retx << std::endl;
}

#ifdef WITH_PRODUCER_HANDOVER_LOGGING
void
onHandoverStats(Ptr<OutputStreamWrapper> stream, producerHandoverStats & fs)
{
  *stream->GetStream () << fs.toString() << std::endl;
}
#endif

/*------------------------------------------------------------------------------
 *  Topology helpers
 *----------------------------------------------------------------------------*/

void
addLink(Ptr<Node> n1, Ptr<Node> n2, std::string bandwidth, std::string delay)
{
    NodeContainer nodes;
    nodes.Add(n1);
    nodes.Add(n2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
    pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));
    pointToPoint.Install (nodes);

}

void
addx2Link(Ptr<Node> n1, Ptr<Node> n2, std::string bandwidth, std::string delay)
{
    NodeContainer nodes;
    nodes.Add(n1);
    nodes.Add(n2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
    pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));
    pointToPoint.Install (nodes);

}

/*------------------------------------------------------------------------------
 * Simulation code
 *----------------------------------------------------------------------------*/

int
main(int argc, char* argv[])
{
    /*--------------------------------------------------------------------------
     * Command line parsing; progress report; results path
     *------------------------------------------------------------------------*/

    Options options;
    options.ParseCmdline(argc, argv);
    options.PrintSummary();

    // https://www.nsnam.org/docs/manual/html/random-variables.html
    //ns3::RngSeedManager::SetSeed()
    //ns3::RngSeedManager::SetRun(options.run)

    std::string result_path = options.ManageResultPath();

    // Save PID to file
    ofstream pidfile;
    pidfile.open(result_path + "_pid");
    pidfile << ::getpid() << std::endl;
    pidfile.close();

#ifndef RAAQM
    if (options.rho_fraction_elastic) {
      std::cout << "RAAQM needed to generate elastic traffic" << std::endl;
      exit(EXIT_FAILURE);
    }
#endif // ! RAAQM

    TrackProgress(options.duration, result_path + "_status", options.quiet);

    /*--------------------------------------------------------------------------
     * Topology
     *
     * We need Topology, Leaves and RootNodes
     *------------------------------------------------------------------------*/

    NodeContainer Topology;
    NodeContainer Leaves;
    NodeContainer RootNodes;
    NodeContainer grNodes;

    uint32_t numLeaves = NUM_LEAVES;
    if (options.mobility_model == "trace")
      numLeaves = TRACE_BASED_NUM_LEAVES;

    std::map<int, Ptr<Node> > ap_maps;

      std::cout << "Creating leaves..." << std::endl;
      Leaves.Create(numLeaves);
#ifdef MAPME
      bool has_X2 = (options.mobility_scheme_id == MS_MAPME);
#else
      bool has_X2 = false;
#endif

      std::cout << "Creating topology..." << std::endl;

    std::string bw = boost::lexical_cast<std::string>(options.capacity / 1e6) + "Mbps";

    if ((options.topology == "fat-tree") || (options.topology == "tree")) {

#ifdef WITH_BORDER_AP
        MobileFatTreeTopologySinglePathWithBorder MFTTopology(Leaves, options, has_X2);
#else
        int mapSize = -1;
        if (options.mobility_model == "trace") {
          if (setInternetPoAPosition(options.aps_trace_file, Leaves)==-1) {
            NS_LOG_ERROR("Failed to place Internet PoA in the map");
            return -1;
          }
          mapSize = TRACE_BASED_MAP_SIZE;
        }
        MobileFatTreeTopology MFTTopology(Leaves, options, has_X2, mapSize);
#endif

        MFTTopology.GetRootNodes(RootNodes);

        grNodes.Add(MFTTopology);
        grNodes.Add(Leaves);

        // Adds an APManager to the leaves (this is basically a dummy manager to
        // detect X2 links)
        // NOTE: myGlobalRoutingHelper does not create routes towards links
        // having an AP Manager on both sides = X2 links (weak)
        for (uint32_t i = 0; i < Leaves.GetN(); i++) {
            Ptr<APManager> apm  = CreateObject<APManager>();
            Leaves.Get(i)->AggregateObject(apm);
        }
        Topology.Add(MFTTopology);

    } else if (topologies.find(options.topology) != topologies.end()) {
        AnnotatedTopologyReader topologyReader("", TOPOLOGY_SCALE);
        std::stringstream fn;
        fn << TOPOLOGY_PATH << options.topology << ".txt";
        topologyReader.SetFileName(fn.str());
        Topology.Add(topologyReader.Read());
        RootNodes.Add(Topology);

        NodeContainer AvailableLeaves;

        if(options.topology ==  "rocketfuel-3257" ||
           options.topology == "rocketfuel-6461" ||
           options.topology == "rocketfuel-1755"
          ) {
            AvailableLeaves.Add(Topology);

            // TODO: use topology, not all nodes !
           for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
               std::string  nodeName = ns3::Names::FindName (*node);
               if (nodeName.find("leaf-") == 0){ // it is a leaf in rocketfuel
                   AvailableLeaves.Add(*node);
               }
           }
        } else {
          AvailableLeaves.Add(Topology);
        }

        // XXX ensure we have enough AvailableLeaves
        std::random_shuffle(AvailableLeaves.begin(), AvailableLeaves.end()); // shuffle STL container

//        if (options.mobility_model == "rwp" ||
//            options.mobility_model == "rw2" ||
//            options.mobility_model == "rd2" ||
//            options.mobility_model == "rgm"
//           ) {
            // Add 16 random nodes to act as leaves, and we will use a random
            // waypoint
            // XXX need to setup right coordinates
            double x, y;


          // position the leaf and create a wired link towards it
          for (unsigned int i = 0; i < 16; i++) {
              Ptr<Node> leaf = Leaves.Get(i);

              Ptr<ConstantPositionMobilityModel> loc = leaf->GetObject<ConstantPositionMobilityModel> ();
              if (loc == 0) {
                  loc = CreateObject<ConstantPositionMobilityModel> ();
                  leaf->AggregateObject (loc);
              }
              switch(i) {
                case  0: x = 0 * options.cell_size + options.cell_size / 2; y = 0 * options.cell_size + options.cell_size / 2; break; 
                case  1: x = 1 * options.cell_size + options.cell_size / 2; y = 0 * options.cell_size + options.cell_size / 2; break; 
                case  2: x = 0 * options.cell_size + options.cell_size / 2; y = 1 * options.cell_size + options.cell_size / 2; break; 
                case  3: x = 1 * options.cell_size + options.cell_size / 2; y = 1 * options.cell_size + options.cell_size / 2; break; 
                case  4: x = 2 * options.cell_size + options.cell_size / 2; y = 0 * options.cell_size + options.cell_size / 2; break; 
                case  5: x = 3 * options.cell_size + options.cell_size / 2; y = 0 * options.cell_size + options.cell_size / 2; break; 
                case  6: x = 2 * options.cell_size + options.cell_size / 2; y = 1 * options.cell_size + options.cell_size / 2; break; 
                case  7: x = 3 * options.cell_size + options.cell_size / 2; y = 1 * options.cell_size + options.cell_size / 2; break; 
                case  8: x = 0 * options.cell_size + options.cell_size / 2; y = 2 * options.cell_size + options.cell_size / 2; break; 
                case  9: x = 1 * options.cell_size + options.cell_size / 2; y = 2 * options.cell_size + options.cell_size / 2; break; 
                case 10: x = 0 * options.cell_size + options.cell_size / 2; y = 3 * options.cell_size + options.cell_size / 2; break; 
                case 11: x = 1 * options.cell_size + options.cell_size / 2; y = 3 * options.cell_size + options.cell_size / 2; break; 
                case 12: x = 2 * options.cell_size + options.cell_size / 2; y = 2 * options.cell_size + options.cell_size / 2; break; 
                case 13: x = 3 * options.cell_size + options.cell_size / 2; y = 2 * options.cell_size + options.cell_size / 2; break; 
                case 14: x = 2 * options.cell_size + options.cell_size / 2; y = 3 * options.cell_size + options.cell_size / 2; break; 
                case 15: x = 3 * options.cell_size + options.cell_size / 2; y = 3 * options.cell_size + options.cell_size / 2; break; 
              }
              std::cout << leaf->GetId() << " x= " << x << ", y = " << y << std::endl;
              Vector pos (x, y, 0);
              loc->SetPosition(pos);
              Leaves.Add(leaf);
              Topology.Add(leaf);

              Ptr<Node> node = AvailableLeaves.Get(i);

              /* link between leaf and node*/
              addLink(node, leaf, bw, options.delay_s);

          }

        grNodes.Add(Topology);

        // code duplicated with MFT too !
        if (has_X2) {
            addx2Link(Leaves.Get(0),  Leaves.Get(1),  bw, "1ms");
            addx2Link(Leaves.Get(0),  Leaves.Get(3),  bw, "1ms");
            addx2Link(Leaves.Get(0),  Leaves.Get(2),  bw, "1ms");
            addx2Link(Leaves.Get(1),  Leaves.Get(2),  bw, "1ms");

            addx2Link(Leaves.Get(1),  Leaves.Get(4),  bw, "1ms");
            addx2Link(Leaves.Get(1),  Leaves.Get(6),  bw, "1ms");
            addx2Link(Leaves.Get(1),  Leaves.Get(3),  bw, "1ms");
            addx2Link(Leaves.Get(4),  Leaves.Get(3),  bw, "1ms");

            addx2Link(Leaves.Get(4),  Leaves.Get(5),  bw, "1ms");
            addx2Link(Leaves.Get(4),  Leaves.Get(7),  bw, "1ms");
            addx2Link(Leaves.Get(4),  Leaves.Get(6),  bw, "1ms");
            addx2Link(Leaves.Get(5),  Leaves.Get(6),  bw, "1ms");

            addx2Link(Leaves.Get(5),  Leaves.Get(7),  bw, "1ms");

            addx2Link(Leaves.Get(2),  Leaves.Get(3),  bw, "1ms");
            addx2Link(Leaves.Get(2),  Leaves.Get(9),  bw, "1ms");
            addx2Link(Leaves.Get(2),  Leaves.Get(8),  bw, "1ms");
            addx2Link(Leaves.Get(3),  Leaves.Get(8),  bw, "1ms");

            addx2Link(Leaves.Get(3),  Leaves.Get(6),  bw, "1ms");
            addx2Link(Leaves.Get(3),  Leaves.Get(12), bw, "1ms");
            addx2Link(Leaves.Get(3),  Leaves.Get(9),  bw, "1ms");
            addx2Link(Leaves.Get(6),  Leaves.Get(9),  bw, "1ms");

            addx2Link(Leaves.Get(6),  Leaves.Get(7),  bw, "1ms");
            addx2Link(Leaves.Get(6),  Leaves.Get(13), bw, "1ms");
            addx2Link(Leaves.Get(6),  Leaves.Get(12), bw, "1ms");
            addx2Link(Leaves.Get(7),  Leaves.Get(12), bw, "1ms");

            addx2Link(Leaves.Get(7),  Leaves.Get(13), bw, "1ms");

            addx2Link(Leaves.Get(8),  Leaves.Get(9),  bw, "1ms");
            addx2Link(Leaves.Get(8),  Leaves.Get(11), bw, "1ms");
            addx2Link(Leaves.Get(8),  Leaves.Get(10), bw, "1ms");
            addx2Link(Leaves.Get(9),  Leaves.Get(10), bw, "1ms");

            addx2Link(Leaves.Get(9),  Leaves.Get(12), bw, "1ms");
            addx2Link(Leaves.Get(9),  Leaves.Get(14), bw, "1ms");
            addx2Link(Leaves.Get(9),  Leaves.Get(11), bw, "1ms");
            addx2Link(Leaves.Get(12), Leaves.Get(11), bw, "1ms");

            addx2Link(Leaves.Get(12), Leaves.Get(13), bw, "1ms");
            addx2Link(Leaves.Get(12), Leaves.Get(15), bw, "1ms");
            addx2Link(Leaves.Get(12), Leaves.Get(14), bw, "1ms");
            addx2Link(Leaves.Get(13), Leaves.Get(14), bw, "1ms");

            addx2Link(Leaves.Get(13), Leaves.Get(15), bw, "1ms");

            addx2Link(Leaves.Get(10), Leaves.Get(11), bw, "1ms");
            addx2Link(Leaves.Get(11), Leaves.Get(14), bw, "1ms");
            addx2Link(Leaves.Get(14), Leaves.Get(15), bw, "1ms");
        }
    }


    std::cout << "***** topo size " << Topology.GetN() << std::endl;


    NodeContainer consumerNodes;
    if (options.mobility_model == "trace")
      consumerNodes.Create(options.num_cp_couples*2); //in the mobility trace scenario, we have 2 consumers per producer at the moment
    else
      consumerNodes.Create(options.num_cp_couples);
    NodeContainer producerNodes;
    producerNodes.Create(options.num_cp_couples);

    NodeContainer mobileNodes;
    mobileNodes.Add(producerNodes);

    grNodes.Add(producerNodes);
    std::cout << "GLOBAL ROUTING SIZE = " << grNodes.size() << std::endl; 

    if (options.static_consumer){ //picking randon leaves and connecting (wired link) to consumers
        Ptr<UniformRandomVariable> random=CreateObject<UniformRandomVariable>();
        for(unsigned int i = 0; i< consumerNodes.size(); i++){
            uint32_t randomNum = random->GetInteger(0,Leaves.size()-1);
            addLink(Leaves.Get(randomNum),consumerNodes.Get(i));
        }
    } else {
        mobileNodes.Add(consumerNodes);
    }

    /* Nodes to involve in GlobalRouting
     *
     * Note that consumer nodes do not need routing as they always have / as
     * default route.
     */

    // XXX TODO Producer corresponding to consumer at root

    /*--------------------------------------------------------------------------
     * NDN stack parameters (incl. mobility management)
     *
     * DEPS: all nodes
     *------------------------------------------------------------------------*/

    /* NDN stack / no cache */
    std::cout << "Creating NDN stack..." << std::endl;
    ndn::StackHelper ndnHelper;
    ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "1");
#ifdef CONF_FILE
    if (options.mobility_scheme_id == MS_GR) {
      ndnHelper.SetStackAttributes("mobility_scheme", "vanilla");
#ifdef MAPME
    } else if (options.mobility_scheme_id == MS_MAPME) {
      ndnHelper.SetStackAttributes("mobility_scheme", "mapme");
      ndnHelper.SetStackAttributes("Tu", boost::lexical_cast<std::string>(options.tu));
    } else if (options.mobility_scheme_id == MS_MAPME_IU) {
      ndnHelper.SetStackAttributes("mobility_scheme", "mapme");
      ndnHelper.SetStackAttributes("Tu", "0");
    } else {
      ndnHelper.SetStackAttributes("mobility_scheme", options.mobility_scheme);
#endif // MAPME

    }

#else
    if (options.mobility_scheme_id != MS_GR) {
      std::cout << "Cannot set mobility scheme without conffile" << std::endl;
      exit(EXIT_FAILURE);
    }
#endif // CONF_FILE

    /* At this stage, a NetDeviceFace is created for every NetDevice. This
     * should not be the case for WiFi because we rely on the connectivity
     * managers for this.
     *
     * NOTE:
     *  - the face cannot be NULL, otherwise default callback will be used
     *  (see. helper/ndn-stack-helper.cpp)
     *  - with default routes, / will be associated to the facze
     *
     * TODO This code should be moved to the ConnectivityManager extension.
     */
    ndnHelper.AddNetDeviceFaceCreateCallback(ns3::WifiNetDevice::GetTypeId(),
        MakeCallback(&dummyNetDeviceFaceCallBack));

    //ndnHelper.SetDefaultRoutes (true);

    ndnHelper.InstallAll();
    ndn::StrategyChoiceHelper::InstallAll("/", DEFAULT_STRATEGY);

    /*--------------------------------------------------------------------------
     * Routing (1/2)
     *
     * DEPS: ndnStack
     *------------------------------------------------------------------------*/

    std::cout << "Setting up ConnectivityManager & GlobalRoutingHelper..." << std::endl;
    /* Routing:
     * we use myGlobalRoutingHelper which is a variant of the
     * GlobalRoutingHelper supporting WiFi channels (takes associations into
     * account), and APManager to ignore X2 links between two base stations for
     * route calculation. Be sure that APmanager has been installed beforehand.
     *
     * NOTE: we don't install GR on consumerNodes to allow them to use the
     * default route / even when MapMe would leave FIB entries without next
     * hops, preventing the correct LPM match.
     */
    MyGlobalRoutingHelper grHelper;
    grHelper.Install(grNodes);


    /*--------------------------------------------------------------------------
     * Mobility management
     *
     * NOTES:
     *  - The signalling to anchor will be done at first producer association A
     *  fixed producer should be known to routing.
     *  - The same applies for KITE
     *------------------------------------------------------------------------*/

    /* Anchor-based mobility: setup anchor and base-station applications */
#ifdef ANCHOR // XXX KITE also
    if ((options.mobility_scheme_id == MS_ANCHOR) ) {
      ApplicationContainer appc;

      /* Set up anchor support in anchor point(now it is at root) and make it
       * the default gateway for all traffic (or traffic in a namespace).
       * Setup traces.
       */
      ndn::AppHelper anchorHelper("ns3::ndn::Anchor");
      anchorHelper.Install(Topology.Get(0));
      grHelper.AddOrigin(ANCHOR_UPDATE_STR, Topology.Get(0) );
    }

#endif // ANCHOR KITE

#ifdef ANCHOR
    if (options.mobility_scheme_id == MS_ANCHOR) {
      ApplicationContainer appc;

      /* Setup Base Station for Anchor management and make sure the BS locator is routed */
      ndn::AppHelper BaseStationHelper("ns3::ndn::BaseStation");
      for (uint32_t i = 0; i < Leaves.GetN(); i++) {
        ApplicationContainer appc = BaseStationHelper.Install(Leaves.Get(i));
        for (uint32_t j = 0; j < appc.GetN(); j++) {
          BaseStation *app = dynamic_cast<BaseStation*>(appc.Get(j).operator->());
          app->PreInitialize();
          grHelper.AddOrigin(app->GetLocator(), Leaves.Get(i));
        }
      }

    }
#endif // ANCHOR

    /*--------------------------------------------------------------------------
     * Workload
     *------------------------------------------------------------------------*/

    std::cout << "Creating traffic..." << std::endl;

    /* TODO num_streaming and num_elastic used here, not for mobile consumers
     * which rely on num_cp_couples already...
     */
    /*
    setupPermanentConsumer(RootNodes, PREFIX, options, result_path, FT_EXTERNAL | FT_ELASTIC, "root");
    setupPermanentConsumer(RootNodes, PREFIX, options, result_path, FT_EXTERNAL | FT_STREAMING, "root");
    setupDynamicConsumer(RootNodes, PREFIX, options, result_path, FT_EXTERNAL | FT_ELASTIC, "root");
    setupDynamicConsumer(RootNodes, PREFIX, options, result_path, FT_EXTERNAL | FT_STREAMING, "root");
    */

// created only if consumer at root, not part of a pair
#if 0
    NodeContainer ProducerC;
    ProducerC.Create(1);
    Ptr<Node> ProducerNode = ProducerC.Get(0);

#ifdef ANCHOR
      if (options.mobility_scheme_id == MS_ANCHOR) {
        /* Because we use the Global Router independently of the mobility scheme,
         * at least to initially populate the FIBs, we need to inform each node
         * that it serves the prefix.
         *
         * TODO This might be done automatically by a GR extension.
         */
        grHelper.AddOrigin(curPrefix, Topology.Get(0) ); // Topology.GetRootNodes ?
      } else {
#endif // ANCHOR
        grHelper.AddOrigin(PREFIX, Producer.Get(0));
#ifdef ANCHOR
      }
#endif // ANCHOR

      /* Producer setup: traffic model + mobility model + mobility management */
      Ptr<Producer> Producer = setupProducer(ProducerNode, (options.mobility_scheme_id == MS_ANCHOR) ? ANCHOR_ORIGIN_STR + PREFIX : PREFIX, options);
#endif

#ifdef WITH_PRODUCER_HANDOVER_LOGGING
ApplicationContainer producerApps;
#endif
    /* Traffic at edge : consumer producer pairs */
    for (uint64_t id = 0; id < options.num_cp_couples; id++) {
      std::string curPrefix(PREFIX + boost::lexical_cast<std::string>(id));

      /* Producer : make a function setupProducer which does a bit more... */

      std::cout << "setting producer up with prefix" << curPrefix << "id=" << id << std::endl;
      Ptr<Producer> Producer = setupProducer(producerNodes.Get(id), (options.mobility_scheme_id == MS_ANCHOR) ? ANCHOR_ORIGIN_STR + curPrefix : curPrefix, options);
#ifdef WITH_PRODUCER_HANDOVER_LOGGING
producerApps.Add(Producer);
#endif

      // Set prefix origin for Global Routing
#ifdef ANCHOR
      if ((options.mobility_scheme_id == MS_ANCHOR) || (options.mobility_scheme_id == MS_KITE)) {
        /* Because we use the Global Router independently of the mobility scheme,
         * at least to initially populate the FIBs, we need to inform each node
         * that it serves the prefix.
         *
         * TODO This might be done automatically by a GR extension.
         */
        std::cout << "ANCHOR OR KITE origin" << std::endl;
        grHelper.AddOrigin(curPrefix, Topology.Get(0) ); // Topology.GetRootNodes ?
      } else {
#endif // ANCHOR
        std::cout << "Origin to producer node=" << producerNodes.Get(id) << std::endl;
        grHelper.AddOrigin(curPrefix, producerNodes.Get(id));
#ifdef ANCHOR
      }
#endif // ANCHOR


      /* Setup workload
       *
       * Non-used traffic should not be installed since fraction would be 0
       *
       * Need to choose where traffic is installed: a single leaf.
       */
      NodeContainer consumerNode;
      consumerNode.Add(consumerNodes.Get(id));

      if (options.mobility_model == "trace") {
        //todogg for the moment, with trace_based sim we run either CBR or Periscope app. This should change in the future - merge with the other options
        //tmp ack: in the previous simulations we had 2 consumers for each producer, so we need one more
        uint32_t idBis = id + options.num_cp_couples;
        curPrefix = PREFIX + boost::lexical_cast<std::string>(id) + "/"+ boost::lexical_cast<std::string>(id);
        std::string curPrefixBis(PREFIX + boost::lexical_cast<std::string>(id) + "/"+boost::lexical_cast<std::string>(idBis));
        NodeContainer consumerNodeBis;
        consumerNodeBis.Add(consumerNodes.Get(idBis));

        if(options.rho_fraction_elastic>0){ //if true, consumer streaming (periscope) is up, otherwise CBR
          setUpPeriscopeConsumer(consumerNode, curPrefix, result_path);
          setUpPeriscopeConsumer(consumerNodeBis, curPrefixBis, result_path);
        } else {
          setUpCBRConsumer(consumerNode, curPrefix, result_path);
          setUpCBRConsumer(consumerNodeBis, curPrefixBis, result_path);
        }
      } else {
        setupPermanentConsumer(consumerNode, curPrefix, options, result_path, FT_INTERNAL | FT_ELASTIC, "leaf");
        setupPermanentConsumer(consumerNode, curPrefix, options, result_path, FT_INTERNAL | FT_STREAMING, "leaf");
        setupDynamicConsumer(consumerNode, curPrefix, options, result_path, FT_INTERNAL | FT_ELASTIC, "leaf");
        setupDynamicConsumer(consumerNode, curPrefix, options, result_path, FT_INTERNAL | FT_STREAMING, "leaf");
      }
    } // END LOOP CONSUMER/PRODUCER PAIR

    if (options.mobility_model == "trace") {
      Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ConsumerStreaming/DownloadFailure", MakeCallback(&DownloadFailure));
      Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ConsumerStreaming/DownloadSuccess", MakeCallback(&DownloadSuccess));
    }

    /*--------------------------------------------------------------------------
     * Mobility pattern
     *------------------------------------------------------------------------*/

    int endTime = -1; //used to set the end time of the simulation when the mobility is trace based
    if (options.mobility_model == "rwp") {
        setupRandomWaypointMobility(mobileNodes, options);
    } else if (options.mobility_model=="rw2") { // random walk 2D
        setupRandom2DWalkMobilityModel(mobileNodes, options);
    } else if (options.mobility_model == "rgm") { // random Gaussion Markov
        setupGaussionMarkovMobilityModel(mobileNodes, options);
    } else if (options.mobility_model == "rd2") { // random direction 2d
        setupRandom2DDirectionMobilityModel(mobileNodes, options);
    } else if (markov_models.find(options.mobility_model) != markov_models.end()) {
        std::cout << "DO MARKOV" << std::endl;
        setupMarkovMobility(mobileNodes, options, Leaves);
    } else if (options.mobility_model == "trace") { //trace based mobility (car, pedestrian)
      endTime = pickApplicationAndInstallMobility(options.mobility_trace_file, options.num_cp_couples, options.min_producer_time,producerNodes);
      if ( endTime ==-1){
        NS_LOG_ERROR("Something went wrong while installing mobility and selecting consumers/producers");
        return -1;
      }
      if (!options.static_consumer){
        setupRandomWaypointMobility(consumerNodes, options); //if consumers are mobile, we have to install mobility on them. todo: mobility should be   trace based, not randomWayPoint
      }
    } else {
        std::cerr << "Unknown mobility model: " << options.mobility_model << std::endl;
        return -1;
    }

    /*--------------------------------------------------------------------------
     * Radio access
     *------------------------------------------------------------------------*/

    /* Setup WiFi on mobile nodes */
    if (options.wired) {//implies options.radio_model is equal to "pwc" (= perfect wireless channel)
      setupWired(Leaves, mobileNodes, options);
    } else {
       if(options.radio_model=="rfm"){//releigh fading model
           SETUP_WIFI(Leaves, mobileNodes, options);
       } else if(options.radio_model=="slos" || //suburban with line of sight
                 options.radio_model=="unlos" //urban with NO line of sight
                ){
                    setupWifi_new_radio(Leaves, mobileNodes, options);
                }

    }


    if(options.wired){
        ConnectivityManager::setApMaps(Leaves,options);
    }

    /* All mobile nodes should have a ConnectivityManager, but for consumers.
     * This will prevent routing updates to populate consumer's FIB and thus
     * have empty FIBs preempt the default route of consumers... */
    for (uint32_t i = 0; i < mobileNodes.GetN(); i++) {
      Ptr<ConnectivityManager> cm;
      switch(options.mobility_scheme_id) {
        case MS_GR:
          cm  = CreateObject<ConnectivityManagerGR>();
          break;
        default: /* MapMe, Anchor */
          cm = CreateObject<ConnectivityManager>();
          break;
      }

      mobileNodes.Get(i)->AggregateObject(cm);
      if(options.wired){
          cm->setWired(options.wired,options.cell_size);
      }
    }

#ifdef WITH_PRODUCER_HANDOVER_LOGGING
    for(unsigned i=0; i< producerApps.GetN(); i++){
       Ptr<Producer> producerApp=DynamicCast<Producer>(producerApps.Get(i));
       producerApp->setlogging();
       std::ostringstream fs_fn;
           fs_fn << result_path << "producer_handover_stats" << "-" << producerApp->GetNode()->GetId();
       AsciiTraceHelper asciiTraceHelper;
             Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fs_fn.str());
       *stream->GetStream () << producerHandoverStats::getHeader() << std::endl;
       producerApp->TraceConnectWithoutContext("HandoverStatsTrace", MakeBoundCallback(&onHandoverStats, stream));
     }
#endif

/*
#ifdef MAPME
    SignalingPrinter signalingPrinter(result_path);
    if(options.mobility_scheme == MS_MAPME || options.mobility_scheme == MS_MAPME_IU || options.mobility_scheme == MS_MAPME_OLD) {
      Ptr<L3Protocol> mobile_Node_l3 = ProducerNode->GetObject<L3Protocol>();
      shared_ptr<::nfd::ForwarderMapMe> fw = static_pointer_cast<::nfd::ForwarderMapMe>(mobile_Node_l3->getForwarder());
      ::nfd::MobilityManager & mm = fw->getMobilityManager();
      //tracing the signaling overhead at producer and router
      std::cout << "A" << std::endl;
      std::cout << "mob mgr " << mm << std::endl;
      mm.onPrintSignalingOverhead=MakeCallback (&SignalingPrinter::printForProducer,  &signalingPrinter);
      nfd::ForwarderMapMe::onIUdropped=MakeCallback (&SignalingPrinter::printForForwarder, &signalingPrinter);
    }
#endif // MAPME
*/


    /*--------------------------------------------------------------------------
     * Additional statistics
     *
     * TODO move this to a statistics extension
     *------------------------------------------------------------------------*/

    // Handovers
    AsciiTraceHelper ho_asciiTraceHelper;
    Ptr<OutputStreamWrapper> ho_stream;
    std::ostringstream ho_fn;
    ho_fn << result_path << "handovers";
    ho_stream = ho_asciiTraceHelper.CreateFileStream (ho_fn.str());
    Config::ConnectWithoutContext("/NodeList/*/$ns3::ConnectivityManager/MobilityEvent", MakeBoundCallback(&onHandover, ho_stream));

    // Signaling overhead
    AsciiTraceHelper so_asciiTraceHelper;
    Ptr<OutputStreamWrapper> so_stream;
    std::ostringstream so_fn;
    so_fn << result_path << "signaling_overhead";
    so_stream = so_asciiTraceHelper.CreateFileStream (so_fn.str());
    *so_stream->GetStream() << "time\tnode_id\tprefix\tseq\tttl\tnum_retx" << std::endl;

    AsciiTraceHelper po_asciiTraceHelper;
    Ptr<OutputStreamWrapper> po_stream;
    std::ostringstream po_fn;
    po_fn << result_path << "processing_overhead";
    po_stream = po_asciiTraceHelper.CreateFileStream (po_fn.str());
    *po_stream->GetStream() << "time\tnode_id\tprefix\tseq\tttl\tnum_retx" << std::endl;
    switch (options.mobility_scheme_id) {
      case MS_GR:
        // This is equal to the number of nodes in the network
        // No statistics to collect
        // OR
        // XXX for each Node and each forwarder (eventually that is not a C /
        // P), write 1
        break;

#ifdef MAPME
      case MS_MAPME:
      case MS_MAPME_IU:
        /* We instrument the MapMe forwarder on each node, to set up a callback
         * for every sent special interest
         */
        for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
          Ptr<L3Protocol> l3 = (*node)->GetObject<L3Protocol>();
          shared_ptr<::nfd::ForwarderMapMe> fw = static_pointer_cast<::nfd::ForwarderMapMe>(l3->getForwarder());
          fw->m_onSpecialInterest = MakeBoundCallback (&onSpecialInterest, so_stream, (*node)->GetId());
          fw->m_onProcessing = MakeBoundCallback (&onProcessing, po_stream, (*node)->GetId());
        }
        break;
#endif // MAPME
#ifdef ANCHOR
      case MS_ANCHOR:
        for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
          Ptr<L3Protocol> l3 = (*node)->GetObject<L3Protocol>();
          shared_ptr<::nfd::ForwarderAnchor> fw = static_pointer_cast<::nfd::ForwarderAnchor>(l3->getForwarder());
          fw->m_onSpecialInterest = MakeBoundCallback (&onSpecialInterest, so_stream, (*node)->GetId());
          fw->m_onProcessing = MakeBoundCallback (&onProcessing, po_stream, (*node)->GetId());
        }
        break;
#endif // ANCHOR
#ifdef KITE
      case MS_KITE:
        for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
          Ptr<L3Protocol> l3 = (*node)->GetObject<L3Protocol>();
          shared_ptr<::nfd::ForwarderKite> fw = static_pointer_cast<::nfd::ForwarderKite>(l3->getForwarder());
          fw->m_onSpecialInterest = MakeBoundCallback (&onSpecialInterest, so_stream, (*node)->GetId());
          fw->m_onProcessing = MakeBoundCallback (&onProcessing, po_stream, (*node)->GetId());
        }
        break;
#endif // KITE

    }

    // Rate
    // XXX XZ: disable tracing by default
    //for logging througput every deltaT=0.5s

    /*--------------------------------------------------------------------------
     * Routing (2/2)
     *
     * NOTE:
     *  - This is only required if there are static prefixes (eg. anchors).
     *  - This is done after the m_onSpecialInterest is set otherwise segfault
     *  upon updating the trace in KITE. XXX
     *------------------------------------------------------------------------*/

    std::cout << "Computing routes..." << std::endl;
    MyGlobalRoutingHelper::CalculateRoutes();

    /*--------------------------------------------------------------------------
     * Main
     *------------------------------------------------------------------------*/

    /* Add a description to all faces for easier debug
     *
     * NOTE: application faces dynamically created when Applications start won't
     * have a description
     */
    for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
      Ptr<L3Protocol> l3 = (*node)->GetObject<L3Protocol>();
      for (auto& i : l3->getForwarder()->getFaceTable()) {
        std::shared_ptr<Face> face = std::dynamic_pointer_cast<Face>(i);
        if (face->isLocal())
          continue;
        ostringstream desc;
        desc << "Face<node=" << (*node)->GetId() << ", mac=XXX"
          << ", id=" << face->getId() << ">";
        face->setDescription(desc.str());
      }
    }


    if (endTime==-1)
      Simulator::Stop(Seconds(options.duration));
    else
      Simulator::Stop(Seconds(endTime));

    /* Rate tracer : not for artificial topologies */
    if (topologies.find(options.topology) == topologies.end()) {
      L3RateTracerNg::InstallAll(result_path + "rate-trace.txt", Seconds(1.0));
    }

    std::cout << "Starting simulation..." << std::endl;
    Simulator::Run();

    TrackProgress(0, result_path + "_status", options.quiet);
    std::cout << "Stopping simulation..." << std::endl;

    Simulator::Destroy();

    return 0;
}

} // namesapce ndn
} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::ndn::main(argc, argv);
}
