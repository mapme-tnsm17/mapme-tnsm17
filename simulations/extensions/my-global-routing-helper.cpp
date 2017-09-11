/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif

#define SETX2 1

#include "my-global-routing-helper.hpp"
#include "connectivity-manager.hpp"
#include "ap-manager.hpp"

#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/model/ndn-net-device-face.hpp"
#include "ns3/ndnSIM/model/ndn-global-router.hpp"

#include "daemon/table/fib.hpp"
#include "daemon/fw/forwarder.hpp"
#include "daemon/table/fib-entry.hpp"
#include "daemon/table/fib-nexthop.hpp"

#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/net-device.h"
#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/channel-list.h"
#include "ns3/object-factory.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/concept/assert.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "ns3/ndnSIM/helper/boost-graph-ndn-global-routing-helper.hpp"

#include <math.h>

#ifdef STRECH_N
#define COMBINE(x, y) (((x)<(y))?((x)*1000+(y)):((y)*1000+(x)))
//#define RECORD_AP_SPT_DISTANCE_ONLY 1
#endif

NS_LOG_COMPONENT_DEFINE("ndn.MyGlobalRoutingHelper");

namespace ns3 {
namespace ndn {
#ifdef STRECH_N
    std::unordered_map<uint32_t, uint32_t> MyGlobalRoutingHelper::m_ap_distance_map;
#endif    
inline bool
is_ap(Ptr<Node> node)
{
    /* We recognize an AP thanks to the aggregated (dummy) APManager object */
    return (node->GetObject<APManager>()) ? true : false;
}

void
MyGlobalRoutingHelper::Install(Ptr<Node> node)
{
  NS_LOG_LOGIC("Node: " << node->GetId());

  /* Install the Global router if needed or clear it... */
  Ptr<GlobalRouter> gr = node->GetObject<GlobalRouter>();
  if (gr) {
//      std::cout << "Clearing incidency of node " << node->GetId() << std::endl;
      gr->ClearIncidency();
  } else {
      gr = CreateObject<GlobalRouter>();
      node->AggregateObject(gr);
  }
  /* ... then proceed to update */
  Ptr<L3Protocol> ndn = node->GetObject<L3Protocol>();
  NS_ASSERT_MSG(ndn != 0, "Cannot install MyGlobalRoutingHelper before Ndn is installed on a node");

  for (auto& i : ndn->getForwarder()->getFaceTable()) {
    shared_ptr<NetDeviceFace> face = std::dynamic_pointer_cast<NetDeviceFace>(i);
    if (face == 0) {
      //NS_LOG_DEBUG("Skipping non-netdevice face");
      continue;
    }

    Ptr<NetDevice> nd = face->GetNetDevice();
    if (nd == 0) {
      NS_LOG_DEBUG("Not a NetDevice associated with NetDeviceFace");
      continue;
    }

    // https://github.com/NDN-Routing/ndnSIM/issues/67
    if(!face->isUp())
    {
      NS_LOG_DEBUG ("Skipping inactive netdevice face. Most wifi faces should be inactive."); // change  on 22/9/2014
      continue;
    }

    Ptr<Channel> ch = nd->GetChannel();

    if (ch == 0) {
      NS_LOG_DEBUG("Channel is not associated with NetDevice");
      continue;
    }

    if (ch->GetNDevices() == 2) // e.g., point-to-point channel
    {
      /* We should see this distinction as an optimization for topologies mostly
       * composed of p2p channels, since it avoids creating a GlobalRouter for
       * just 2 nodes, when we are sure they won't evolve. This will not work
       * for wireless channels, since during a simulation, the number of
       * connected devices might increase dynamically (this might also be the
       * case for switches...) */
      for (uint32_t deviceId = 0; deviceId < ch->GetNDevices(); deviceId++) {
        Ptr<NetDevice> otherSide = ch->GetDevice(deviceId);
        if (nd == otherSide)
          continue;

        Ptr<Node> otherNode = otherSide->GetNode();
        NS_ASSERT(otherNode != 0);

        Ptr<GlobalRouter> otherGr = otherNode->GetObject<GlobalRouter>();
        if (otherGr == 0) {
          Install(otherNode);
        }
        otherGr = otherNode->GetObject<GlobalRouter>();
        NS_ASSERT(otherGr != 0);

        /* We add an incidency but for X2 links */
        if (!(is_ap(node) && is_ap(otherNode))) {
          gr->AddIncidency(face, otherGr);
#ifdef SETX2
        } else {
          /* We mark the faces as X2 for MapMe use */
          face->setX2();

          Ptr<L3Protocol> other_l3 = otherNode->GetObject<L3Protocol>();
          assert(other_l3);
          shared_ptr<NetDeviceFace> other_face = std::dynamic_pointer_cast<NetDeviceFace>(other_l3->getFaceByNetDevice(otherSide));    
          assert(other_face);
          other_face->setX2();
#endif // SETX2
        }
      }
    }
// DEPRECATED|    else {
// DEPRECATED|      /* We would usually create a GlobalRouter per channel to avoid the NxN
// DEPRECATED|       * connection problem. Though, in practise, not all devices are connected
// DEPRECATED|       * together. We will rely on nodes having a ConnectivityManager to know
// DEPRECATED|       * to which AP they are associated.
// DEPRECATED|       *
// DEPRECATED|       * Ptr<GlobalRouter> grChannel = ch->GetObject<GlobalRouter>();
// DEPRECATED|       * //if (grChannel == 0) {
// DEPRECATED|       *   Install(ch);
// DEPRECATED|       * //}
// DEPRECATED|       * grChannel = ch->GetObject<GlobalRouter>();
// DEPRECATED|       * gr->AddIncidency(face, grChannel);
// DEPRECATED|       */
// DEPRECATED|
// DEPRECATED|      Ptr<ConnectivityManager> cm = node->GetObject<ConnectivityManager>();
// DEPRECATED|      if (!cm) {//it must be an AP node because only AP has a non-p2p channel, but does not have cm aggregated on it, now we need to add incidency for this active face
// DEPRECATED|        continue;
// DEPRECATED|      }
// DEPRECATED|
// DEPRECATED|      Ptr<Node> PoA = cm->GetPoA();
// DEPRECATED|      if (!PoA)
// DEPRECATED|        continue;
// DEPRECATED|//      std::cout << "Producer component manager has a PoA " << PoA->GetId() << std::endl;
// DEPRECATED|//      Install(PoA);
// DEPRECATED|      shared_ptr<Face> PoAFace = cm->GetPoAFace();
// DEPRECATED|      NS_ASSERT(PoAFace != 0);
// DEPRECATED|
// DEPRECATED|      Ptr<GlobalRouter> grPoA = PoA->GetObject<GlobalRouter>();
// DEPRECATED|      if (grPoA == 0) {
// DEPRECATED|        Install(PoA);
// DEPRECATED|      }
// DEPRECATED|      grPoA = PoA->GetObject<GlobalRouter>();
// DEPRECATED|      NS_ASSERT(grPoA != 0);
// DEPRECATED|
// DEPRECATED|//      std::cout << "WIRELESS: Adding incidencies between node " << node->GetId() << " and its BS " << PoA->GetId() << std::endl;
// DEPRECATED|      gr->AddIncidency(face, grPoA);
// DEPRECATED|      grPoA->AddIncidency(PoAFace, gr);
// DEPRECATED|    }
  }
}

void
MyGlobalRoutingHelper::Install(const NodeContainer& nodes)
{
  for (NodeContainer::Iterator node = nodes.Begin(); node != nodes.End(); node++) {
    Install(*node);
  }
}

void
MyGlobalRoutingHelper::InstallAll()
{
  Install(NodeContainer::GetGlobal());
}

void
MyGlobalRoutingHelper::AddOrigin(const std::string& prefix, Ptr<Node> node)
{
  Ptr<GlobalRouter> gr = node->GetObject<GlobalRouter>();
  NS_ASSERT_MSG(gr != 0, "GlobalRouter is not installed on the node");

  auto name = make_shared<Name>(prefix);
  gr->AddLocalPrefix(name);
}

void
MyGlobalRoutingHelper::ClearOrigins(Ptr<Node> node)
{
  Ptr<GlobalRouter> gr = node->GetObject<GlobalRouter>();
  NS_ASSERT_MSG(gr != 0, "GlobalRouter is not installed on the node");

  gr->ClearLocalPrefixes();
}

void
MyGlobalRoutingHelper::AddOrigins(const std::string& prefix, const NodeContainer& nodes)
{
  for (NodeContainer::Iterator node = nodes.Begin(); node != nodes.End(); node++) {
    AddOrigin(prefix, *node);
  }
}

void
MyGlobalRoutingHelper::AddOrigin(const std::string& prefix, const std::string& nodeName)
{
  Ptr<Node> node = Names::Find<Node>(nodeName);
  NS_ASSERT_MSG(node != 0, nodeName << "is not a Node");

  AddOrigin(prefix, node);
}

void
MyGlobalRoutingHelper::AddOriginsForAll()
{
  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
    Ptr<GlobalRouter> gr = (*node)->GetObject<GlobalRouter>();
    std::string name = Names::FindName(*node);

    if (gr != 0 && !name.empty()) {
      AddOrigin("/" + name, *node);
    }
  }
}

void
MyGlobalRoutingHelper::CalculateRoutes()
{
  /**
   * Implementation of route calculation is heavily based on Boost Graph Library
   * See http://www.boost.org/doc/libs/1_49_0/libs/graph/doc/table_of_contents.html for more details
   */
  
  //NOTE: we are adding a little extra overhead in this function to compute the spt distance between each pair of APs and store it in an ap distance map
  //for later use of computing path stretch:

  BOOST_CONCEPT_ASSERT((boost::VertexListGraphConcept<boost::NdnGlobalRouterGraph>));
  BOOST_CONCEPT_ASSERT((boost::IncidenceGraphConcept<boost::NdnGlobalRouterGraph>));

  boost::NdnGlobalRouterGraph graph;
  // typedef graph_traits < NdnGlobalRouterGraph >::vertex_descriptor vertex_descriptor;

  // For now we doing Dijkstra for every node.  Can be replaced with Bellman-Ford or Floyd-Warshall.
  // Other algorithms should be faster, but they need additional EdgeListGraph concept provided by
  // the graph, which
  // is not obviously how implement in an efficient manner
  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
    Ptr<GlobalRouter> source = (*node)->GetObject<GlobalRouter>();
    if (source == 0) {
      NS_LOG_DEBUG("Node " << (*node)->GetId() << " does not export GlobalRouter interface");
      continue;
    }

    boost::DistancesMap distances;

    dijkstra_shortest_paths(graph, source,
                            // predecessor_map (boost::ref(predecessors))
                            // .
                            distance_map(boost::ref(distances))
                              .distance_inf(boost::WeightInf)
                              .distance_zero(boost::WeightZero)
                              .distance_compare(boost::WeightCompare())
                              .distance_combine(boost::WeightCombine()));

    // NS_LOG_DEBUG (predecessors.size () << ", " << distances.size ());

    Ptr<L3Protocol> L3protocol = (*node)->GetObject<L3Protocol>();
    shared_ptr<nfd::Forwarder> forwarder = L3protocol->getForwarder();

    NS_LOG_DEBUG("Reachability from Node: " << source->GetObject<Node>()->GetId());
    for (const auto& dist : distances) {
      if (dist.first == source){
#ifdef STRECH_N
#ifdef RECORD_AP_SPT_DISTANCE_ONLY
        if(is_ap(*node)){ // we are an AP to itself, so if their distance is not recorded yet, record it:
#endif
            uint32_t sourceId=(*node)->GetId ();
            uint32_t destId=sourceId;
            if(m_ap_distance_map.find(COMBINE(sourceId, destId)) == m_ap_distance_map.end()){
                m_ap_distance_map[COMBINE(sourceId, destId)]= 0;
            }
#ifdef RECORD_AP_SPT_DISTANCE_ONLY            
        }    
#endif
#endif
        continue;
      }else {
        //std::cout << "  Node " << dist.first->GetObject<Node> ()->GetId ();
        if (std::get<0>(dist.second) == 0) {
          //std::cout << " is unreachable" << std::endl;
        }
        else {
#ifdef STRECH_N
#ifdef RECORD_AP_SPT_DISTANCE_ONLY
         if(is_ap(*node) && is_ap(dist.first->GetObject<Node> ())){ // we are encountering 2 APs, so if their distance is not recorded yet, record it:
#endif
            uint32_t sourceId=(*node)->GetId ();
            uint32_t destId=dist.first->GetObject<Node> ()->GetId ();
            if(m_ap_distance_map.find(COMBINE(sourceId, destId)) == m_ap_distance_map.end()){
                m_ap_distance_map[COMBINE(sourceId, destId)]= std::get<1>(dist.second);
            }
#ifdef RECORD_AP_SPT_DISTANCE_ONLY          
         }    
#endif
#endif
            
          for (const auto& prefix : dist.first->GetLocalPrefixes()) {
            NS_LOG_DEBUG(" prefix " << prefix->toUri() << " reachable via face " << *std::get<0>(dist.second)
                         << " with distance " << std::get<1>(dist.second) << " with delay "
                         << std::get<2>(dist.second));

            FibHelper::AddRoute(*node, *prefix, std::get<0>(dist.second),
                                std::get<1>(dist.second));
          }
        }
      }
    }
  }
}

void
MyGlobalRoutingHelper::CalculateRoutesForPrefix(Name const & prefix)
{
  /**
   * Implementation of route calculation is heavily based on Boost Graph Library
   * See http://www.boost.org/doc/libs/1_49_0/libs/graph/doc/table_of_contents.html for more details
   */

  BOOST_CONCEPT_ASSERT((boost::VertexListGraphConcept<boost::NdnGlobalRouterGraph>));
  BOOST_CONCEPT_ASSERT((boost::IncidenceGraphConcept<boost::NdnGlobalRouterGraph>));

  boost::NdnGlobalRouterGraph graph;
  // typedef graph_traits < NdnGlobalRouterGraph >::vertex_descriptor vertex_descriptor;

  // For now we doing Dijkstra for every node.  Can be replaced with Bellman-Ford or Floyd-Warshall.
  // Other algorithms should be faster, but they need additional EdgeListGraph concept provided by
  // the graph, which
  // is not obviously how implement in an efficient manner
  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
    Ptr<GlobalRouter> source = (*node)->GetObject<GlobalRouter>();
    if (source == 0) {
      NS_LOG_DEBUG("Node " << (*node)->GetId() << " does not export GlobalRouter interface");
      continue;
    }

    boost::DistancesMap distances;

    dijkstra_shortest_paths(graph, source,
                            // predecessor_map (boost::ref(predecessors))
                            // .
                            distance_map(boost::ref(distances))
                              .distance_inf(boost::WeightInf)
                              .distance_zero(boost::WeightZero)
                              .distance_compare(boost::WeightCompare())
                              .distance_combine(boost::WeightCombine()));

    // NS_LOG_DEBUG (predecessors.size () << ", " << distances.size ());

    Ptr<L3Protocol> L3protocol = (*node)->GetObject<L3Protocol>();
    shared_ptr<nfd::Forwarder> forwarder = L3protocol->getForwarder();

    NS_LOG_DEBUG("Reachability from Node: " << source->GetObject<Node>()->GetId());
    for (const auto& dist : distances) {
      if (dist.first == source)
        continue;
      else {
        //std::cout << "  Node " << dist.first->GetObject<Node> ()->GetId ();
        if (std::get<0>(dist.second) == 0) {
          //std::cout << " is unreachable" << std::endl;
        }
        else {
          for (const auto& ithPrefix : dist.first->GetLocalPrefixes()) {
            if(*ithPrefix != prefix)
              continue;
	    
            NS_LOG_DEBUG(" prefix " << prefix.toUri() << " reachable via face " << *std::get<0>(dist.second)
                         << " with distance " << std::get<1>(dist.second) << " with delay "
                         << std::get<2>(dist.second));

            FibHelper::AddRoute(*node, prefix, std::get<0>(dist.second),
                                std::get<1>(dist.second));
          }
        }
      }
    }
  }
}

void
MyGlobalRoutingHelper::CalculateAllPossibleRoutes()
{
  /**
   * Implementation of route calculation is heavily based on Boost Graph Library
   * See http://www.boost.org/doc/libs/1_49_0/libs/graph/doc/table_of_contents.html for more details
   */

  BOOST_CONCEPT_ASSERT((boost::VertexListGraphConcept<boost::NdnGlobalRouterGraph>));
  BOOST_CONCEPT_ASSERT((boost::IncidenceGraphConcept<boost::NdnGlobalRouterGraph>));

  boost::NdnGlobalRouterGraph graph;
  // typedef graph_traits < NdnGlobalRouterGraph >::vertex_descriptor vertex_descriptor;

  // For now we doing Dijkstra for every node.  Can be replaced with Bellman-Ford or Floyd-Warshall.
  // Other algorithms should be faster, but they need additional EdgeListGraph concept provided by
  // the graph, which
  // is not obviously how implement in an efficient manner
  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
    Ptr<GlobalRouter> source = (*node)->GetObject<GlobalRouter>();
    if (source == 0) {
      NS_LOG_DEBUG("Node " << (*node)->GetId() << " does not export GlobalRouter interface");
      continue;
    }

    Ptr<L3Protocol> L3protocol = (*node)->GetObject<L3Protocol>();
    shared_ptr<nfd::Forwarder> forwarder = L3protocol->getForwarder();

    NS_LOG_DEBUG("Reachability from Node: " << source->GetObject<Node>()->GetId() << " ("
                                            << Names::FindName(source->GetObject<Node>()) << ")");

    Ptr<L3Protocol> l3 = source->GetObject<L3Protocol>();
    NS_ASSERT(l3 != 0);

    // remember interface statuses
    int faceNumber = 0;
    std::vector<uint16_t> originalMetric(uint32_t(l3->getForwarder()->getFaceTable().size()));
    for (auto& i : l3->getForwarder()->getFaceTable()) {
      faceNumber++;
      shared_ptr<Face> nfdFace = std::dynamic_pointer_cast<Face>(i);
      originalMetric[uint32_t(faceNumber)] = nfdFace->getMetric();
      nfdFace->setMetric(std::numeric_limits<uint16_t>::max() - 1);
      // value std::numeric_limits<uint16_t>::max () MUST NOT be used (reserved)
    }

    faceNumber = 0;
    for (auto& k : l3->getForwarder()->getFaceTable()) {
      faceNumber++;
      shared_ptr<NetDeviceFace> face = std::dynamic_pointer_cast<NetDeviceFace>(k);
      if (face == 0) {
        NS_LOG_DEBUG("Skipping non-netdevice face");
        continue;
      }

      // enabling only faceId
      face->setMetric(originalMetric[uint32_t(faceNumber)]);

      boost::DistancesMap distances;

      NS_LOG_DEBUG("-----------");

      dijkstra_shortest_paths(graph, source,
                              // predecessor_map (boost::ref(predecessors))
                              // .
                              distance_map(boost::ref(distances))
                                .distance_inf(boost::WeightInf)
                                .distance_zero(boost::WeightZero)
                                .distance_compare(boost::WeightCompare())
                                .distance_combine(boost::WeightCombine()));

      // NS_LOG_DEBUG (predecessors.size () << ", " << distances.size ());

      for (const auto& dist : distances) {
        if (dist.first == source)
          continue;
        else {
          // cout << "  Node " << dist.first->GetObject<Node> ()->GetId ();
          if (std::get<0>(dist.second) == 0) {
            // cout << " is unreachable" << endl;
          }
          else {
            for (const auto& prefix : dist.first->GetLocalPrefixes()) {
              NS_LOG_DEBUG(" prefix " << *prefix << " reachable via face "
                           << *std::get<0>(dist.second)
                           << " with distance " << std::get<1>(dist.second)
                           << " with delay " << std::get<2>(dist.second));

              if (std::get<0>(dist.second)->getMetric() == std::numeric_limits<uint16_t>::max() - 1)
                continue;

              FibHelper::AddRoute(*node, *prefix, std::get<0>(dist.second),
                                  std::get<1>(dist.second));
            }
          }
        }
      }

      // disabling the face again
      face->setMetric(std::numeric_limits<uint16_t>::max() - 1);
    }

    // recover original interface statuses
    faceNumber = 0;
    for (auto& i : l3->getForwarder()->getFaceTable()) {
      faceNumber++;
      shared_ptr<Face> face = std::dynamic_pointer_cast<Face>(i);
      face->setMetric(originalMetric[faceNumber]);
    }
  }
}

int
MyGlobalRoutingHelper::getSPTdistanceBetweenAp(uint32_t fromNodeId, uint32_t toNodeId)
{
    if(m_ap_distance_map.find(COMBINE(fromNodeId,toNodeId))!= m_ap_distance_map.end()){
        return m_ap_distance_map[COMBINE(fromNodeId,toNodeId)];
    } else{
        return -1;

    }
}

} // namespace ndn
} // namespace ns3
