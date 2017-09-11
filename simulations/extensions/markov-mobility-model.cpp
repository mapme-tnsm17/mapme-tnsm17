/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "markov-mobility-model.hpp"
#include "connectivity-manager.hpp"

#include "ns3/constant-position-mobility-model.h"
#include "ns3/simulator.h"

// For graph neighbours
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include "ns3/ndnSIM/helper/boost-graph-ndn-global-routing-helper.hpp"
#include "ns3/ndnSIM/model/ndn-global-router.hpp"

namespace ns3 {
namespace ndn {

#define MARKOV_MOBILITY_NO_LEAF (std::numeric_limits<uint32_t>::max())

// Distance to AP has to be lower than coverage
#define MARKOV_MOBILITY_DISTANCE 20 /* m */

#define MARKOV_MOBILITY_MEAN_SLEEP 5

NS_OBJECT_ENSURE_REGISTERED (MarkovMobilityModel);

TypeId MarkovMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::MarkovMobilityModel")
    .SetParent<MobilityModel> ()
    .AddConstructor<MarkovMobilityModel>()
    ;
  return tid;
}

MarkovMobilityModel::MarkovMobilityModel()
  : m_uniform(NULL)
  , m_sleep_rv(NULL)
  , m_model("")
  , m_current_leaf_id(MARKOV_MOBILITY_NO_LEAF)
{
  SetMeanSleep(MARKOV_MOBILITY_MEAN_SLEEP);
}
/*
MarkovMobilityModel::MarkovMobilityModel (NodeContainer * Nodes, double mean_sleep)
  : m_current_leaf_id(MARKOV_MOBILITY_NO_LEAF)
  , m_uniform(NULL)
  , m_sleep_rv(NULL)
{
  SetMeanSleep(mean_sleep);
  SetNodes(Nodes);
}
*/
void
MarkovMobilityModel::SetMeanSleep(double mean_sleep)
{
  std::cout << "mean sleep = " <<  mean_sleep << std::endl;
  m_sleep_rv = CreateObjectWithAttributes<ExponentialRandomVariable>(
      "Mean", DoubleValue(mean_sleep)
  );
}

void
MarkovMobilityModel::SetNodes(NodeContainer * Nodes)
{

  m_nodes = Nodes;
  m_uniform = CreateObjectWithAttributes<UniformRandomVariable>(
      "Min", DoubleValue(0.0),
      "Max", DoubleValue((double)(m_nodes->GetN()))
  );
 
  // Schedule first event
  Simulator::Schedule(Seconds(0), &ns3::ndn::MarkovMobilityModel::Jump, this);
}

void
MarkovMobilityModel::SetModel(std::string model)
{
  m_model = model;
}

void
MarkovMobilityModel::GetShortestPath(uint32_t dst_id, uint32_t src_id)
{
    boost::NdnGlobalRouterGraph graph;

    Ptr<GlobalRouter> src = m_nodes->Get(src_id)->GetObject<GlobalRouter>();
    if (src == 0) {
        std::cerr << "Node " << m_nodes->Get(src_id)->GetId() << " does not export GlobalRouter interface" << std::endl;
        return;
    }
    Ptr<GlobalRouter> dst = m_nodes->Get(dst_id)->GetObject<GlobalRouter>();
    if (dst == 0) {
        std::cerr << "Node " << m_nodes->Get(dst_id) << " does not export GlobalRouter interface" << std::endl;
        return;
    }

    boost::PredecessorsMap predecessors;

    dijkstra_shortest_paths(graph, src,
                            predecessor_map(boost::ref(predecessors))
                              .distance_inf(boost::WeightInf)
                              .distance_zero(boost::WeightZero)
                              .distance_compare(boost::WeightCompare())
                              .distance_combine(boost::WeightCombine()));
    /*
    for(auto elem : predecessors)
    {
        Ptr<GlobalRouter> gr1 = elem.first;
        Ptr<GlobalRouter> gr2 = elem.second;
        std::cout << gr1->GetObject<Node>()->GetId() << " -> " << gr2->GetObject<Node>()->GetId() << std::endl;
    }
    */

    m_path.clear();
    // Store waypoint path in m_path
    for(Ptr<GlobalRouter> cur = dst; cur != src; cur = predecessors[cur]) {
        if (cur == dst)
            continue;
        m_path.push_back(GetLeafIdFromNodeId(cur->GetObject<Node>()->GetId()));
    }
}

uint32_t
MarkovMobilityModel::GetUniformNodeId(uint32_t num, uint32_t avoid)
{
    uint32_t leaf_id = avoid;
    while(leaf_id == avoid)
        leaf_id = m_uniform->GetInteger(0, num - 1);
    return leaf_id;
}

uint32_t
MarkovMobilityModel::GetLeafIdFromNodeId(uint32_t node_id)
{
    uint32_t leaf_id = MARKOV_MOBILITY_NO_LEAF;
    uint32_t i = 0;
    for (auto it = m_nodes->begin(); it != m_nodes->end(); it++) {
        Ptr<Node> leaf_node = *it;
        if (leaf_node->GetId() == node_id) {
            leaf_id = i;
            break;
        }
        i++;
    }
    return leaf_id;
}

void
MarkovMobilityModel::Jump()
{
  Ptr<Node> m_current_leaf;
  double delay;

  Ptr<ConnectivityManager> cm = GetObject<ConnectivityManager>();

  if ((m_current_leaf_id != MARKOV_MOBILITY_NO_LEAF) && cm)
      cm->BeforeMobility();

  if ((m_model == "markov-uniform") || (m_current_leaf_id == MARKOV_MOBILITY_NO_LEAF)) {
      // Select the new position uniformly at random among m_nodes, avoiding the
      // current location
      m_current_leaf_id = GetUniformNodeId(m_nodes->GetN(), m_current_leaf_id);

  } else if (m_model == "markov-rw") {
      // Select the position among the neighbours of the node
      // XXX does not work for the tree topology = graph is not connected
      m_current_leaf = m_nodes->Get(m_current_leaf_id);
      uint32_t node_id = m_current_leaf->GetId();
      Ptr<GlobalRouter> gr = m_current_leaf->GetObject<GlobalRouter>();
      GlobalRouter::IncidencyList & list = gr->GetIncidencies();
      Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable>();
      uint32_t pos = random->GetInteger(0, list.size()-1);
      GlobalRouter::IncidencyList::iterator it = list.begin();
      std::advance(it, pos);
      Ptr<GlobalRouter> gr2 = std::get<2>(*it);
      Ptr<Node> node = gr2->GetObject<Node>();
      node_id = node->GetId();

      m_current_leaf_id = GetLeafIdFromNodeId(node_id);

  } else if (m_model == "markov-rwp") {
      // Did we reach the waypoint ? Yes, draw a new one uniformly and compute
      // shortest path. Otherwise, follow shortest path.
      if (m_path.empty()) {
          uint16_t waypoint = GetUniformNodeId(m_nodes->GetN(), m_current_leaf_id);
          GetShortestPath(waypoint, m_current_leaf_id);
      }
      m_current_leaf_id = m_path.back();
      m_path.pop_back();
  } else {
      std::cerr << "Unexpected condition: wrong mobility model" << std::endl;
      exit(-1);
  }

  if (m_current_leaf_id == MARKOV_MOBILITY_NO_LEAF) {
      std::cerr << "Unexpected condition: cannot find next hop" << std::endl;
      exit(-1);
  }

  m_current_leaf = m_nodes->Get(m_current_leaf_id);

  /* Move node position to a predefined distance from chosen BS node */
  Ptr<ConstantPositionMobilityModel> leaf_loc;
  leaf_loc = m_current_leaf->GetObject<ConstantPositionMobilityModel>();
  Vector leaf_vec = leaf_loc->GetPosition();
  Vector node_vec (leaf_vec.x, leaf_vec.y + MARKOV_MOBILITY_DISTANCE, 0); 
  SetPosition(node_vec);

  if (cm)
    cm->AfterMobility(m_current_leaf);

  // Schedule next jump event
  delay = m_sleep_rv->GetValue();
  std::cout << Simulator::Now ().ToDouble (Time::S) << " JUMP " << GetObject<Node>()->GetId() << " " << m_current_leaf_id << " " << delay << std::endl;
  Simulator::Schedule(Seconds(delay), &ns3::ndn::MarkovMobilityModel::Jump, this);
}


#if 0
  //connect mobile to basestation, then install producer and calculate fib
  
    
    //connect to a basestation
    uint32_t node_id;
    node_id = m_current_leaf_id;
    while(node_id == m_current_leaf_id)
    node_id = m_uniform->GetInteger(0, m_nodes->GetN() - 1);
    m_current_leaf_id = node_id;
    m_current_leaf = m_nodes->Get(m_current_leaf_id);
    Attach(m_mobile_node, m_current_leaf);
#endif


Vector
MarkovMobilityModel::DoGetPosition (void) const
{
  return m_position;
}
void
MarkovMobilityModel::DoSetPosition (const Vector &position)
{
  m_position = position;
  NotifyCourseChange ();
}
Vector
MarkovMobilityModel::DoGetVelocity (void) const
{
  return Vector (0.0, 0.0, 0.0);
}


} // namespace ndn
} // namespace ns3
