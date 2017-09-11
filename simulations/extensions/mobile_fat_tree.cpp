#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"

#include "boost/lexical_cast.hpp"
#include "mobile_fat_tree.hpp"

/* Nodes = 1 core + 4 aggregation + 8 edge */
#define NUM_NODES 13 

// DEPRECATED|#define RANDOM_WAYPOINT 1

namespace ns3 {
namespace ndn {


MobileFatTreeTopology::MobileFatTreeTopology(NodeContainer & Leaves, Options & options, bool has_meshed_leaves, double mapSize)
  : m_has_meshed_leaves(has_meshed_leaves)
  , m_options(options)
{
    std::cout << "Low bandwidth core links" << std::endl;
    std::string bw_core_aggr   = boost::lexical_cast<std::string>(MFT_BW_CORE_AGGR   * options.capacity / 1e6) + "Mbps";
    std::string bw_aggr_edge   = boost::lexical_cast<std::string>(MFT_BW_AGGR_EDGE   * options.capacity / 1e6) + "Mbps";
    std::string bw_edge_leaves = boost::lexical_cast<std::string>(MFT_BW_EDGE_LEAVES * options.capacity / 1e6) + "Mbps";
    std::string bw_x2          = boost::lexical_cast<std::string>(MFT_BW_X2          * options.capacity / 1e6) + "Mbps";
    std::string bw_aggr        = boost::lexical_cast<std::string>(MFT_BW_AGGR        * options.capacity / 1e6) + "Mbps";
    std::string bw_edge        = boost::lexical_cast<std::string>(MFT_BW_EDGE        * options.capacity / 1e6) + "Mbps";

    Create(NUM_NODES);

    // Assign position to nodes TODO
    uint32_t x, y;
    for (uint32_t i = 0; i < 13; i++) {
        if (i == 0) {
            // core
            x = 16; // num_leaves * 2 / 2
            y = Y_CORE;
        } else if (i > 0 && i < 5) {
            // Aggregation
            x = 4 + (i-1) * 8;
            y = Y_AGGREGATION;
        } else {
            // Edge
            x = 2 + (i-5) * 4;
            y = Y_EDGE;
        }
        // assign coordinates
        Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
        Vector vec (x * options.cell_size / 5, options.cell_size * 4 + y * options.cell_size / 2, 0);
        loc->SetPosition(vec);
        Get(i)->AggregateObject(loc);
    }

    // Connect core node to aggregation nodes
    Ptr<Node> core = Get(0);
    for (uint32_t aggr = 1; aggr < 5; aggr++) {
        addLink(core, Get(aggr), bw_core_aggr);
    }

    // Connect aggregation nodes to edge nodes
    for (uint32_t pod = 0; pod < 2; pod++) {
        for (uint32_t edge = 5 + pod * 4; edge < 9 + pod * 4; edge++) {
            addLink(Get(1 + pod * 2 ), Get(edge), bw_aggr_edge);
            addLink(Get(1 + pod * 2 + 1), Get(edge), bw_aggr_edge);
        }
    }

    if (options.topology == "fat-tree") {
      // Add transversal links at edge level...
      addLink(Get(1), Get(4), bw_edge);
      addLink(Get(2), Get(3), bw_edge);
      // ... and aggregation level
      addLink(Get(5), Get(6), bw_aggr);
      addLink(Get(7), Get(8), bw_aggr);
      addLink(Get(9), Get(10), bw_aggr);
      addLink(Get(11), Get(12), bw_aggr);
    }

    
    if (Leaves.GetN()==16){
        // Connect edge nodes to leaves
        for (uint32_t leaf = 0; leaf < Leaves.GetN(); leaf++) {
            addLink(Leaves.Get(leaf), Get(5 + leaf / 2), bw_edge_leaves);
            
            // DEPRECATED|#ifndef RANDOM_WAYPOINT
            // DEPRECATED|        // Assign position to leaves
            // DEPRECATED|        Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
            // DEPRECATED|        x = 2 * leaf + 1;
            // DEPRECATED|        Vector vec (x * options.cell_size / 5, options.cell_size * 4 + Y_LEAVES * options.cell_size / 2, 0);
            // DEPRECATED|        loc->SetPosition(vec);
            // DEPRECATED|        Leaves.Get(leaf)->AggregateObject(loc);
            // DEPRECATED|#endif
        }
        
        // DEPRECATED|#ifdef RANDOM_WAYPOINT
        LeafSetPosition(Leaves.Get( 0),  0 * options.cell_size + options.cell_size / 2,  0 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 1),  1 * options.cell_size + options.cell_size / 2,  0 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 2),  0 * options.cell_size + options.cell_size / 2,  1 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 3),  1 * options.cell_size + options.cell_size / 2,  1 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 4),  3 * options.cell_size - options.cell_size / 2,  0 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 5),  4 * options.cell_size - options.cell_size / 2,  0 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 6),  3 * options.cell_size - options.cell_size / 2,  1 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 7),  4 * options.cell_size - options.cell_size / 2,  1 * options.cell_size + options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 8),  0 * options.cell_size + options.cell_size / 2,  3 * options.cell_size - options.cell_size / 2);
        LeafSetPosition(Leaves.Get( 9),  1 * options.cell_size + options.cell_size / 2,  3 * options.cell_size - options.cell_size / 2);
        LeafSetPosition(Leaves.Get(10),  0 * options.cell_size + options.cell_size / 2,  4 * options.cell_size - options.cell_size / 2);
        LeafSetPosition(Leaves.Get(11),  1 * options.cell_size + options.cell_size / 2,  4 * options.cell_size - options.cell_size / 2);
        LeafSetPosition(Leaves.Get(12),  3 * options.cell_size - options.cell_size / 2,  3 * options.cell_size - options.cell_size / 2);
        LeafSetPosition(Leaves.Get(13),  4 * options.cell_size - options.cell_size / 2,  3 * options.cell_size - options.cell_size / 2);
        LeafSetPosition(Leaves.Get(14),  3 * options.cell_size - options.cell_size / 2,  4 * options.cell_size - options.cell_size / 2);
        LeafSetPosition(Leaves.Get(15),  4 * options.cell_size - options.cell_size / 2,  4 * options.cell_size - options.cell_size / 2);
        // DEPRECATED|#endif

        
        if (has_meshed_leaves) {
            // DEPRECATED|#ifdef RANDOM_WAYPOINT
            addx2Link(Leaves.Get(0),  Leaves.Get(1),  bw_x2);
            addx2Link(Leaves.Get(0),  Leaves.Get(3),  bw_x2);
            addx2Link(Leaves.Get(0),  Leaves.Get(2),  bw_x2);
            addx2Link(Leaves.Get(1),  Leaves.Get(2),  bw_x2);
            
            addx2Link(Leaves.Get(1),  Leaves.Get(4),  bw_x2);
            addx2Link(Leaves.Get(1),  Leaves.Get(6),  bw_x2);
            addx2Link(Leaves.Get(1),  Leaves.Get(3),  bw_x2);
            addx2Link(Leaves.Get(4),  Leaves.Get(3),  bw_x2);
            
            addx2Link(Leaves.Get(4),  Leaves.Get(5),  bw_x2);
            addx2Link(Leaves.Get(4),  Leaves.Get(7),  bw_x2);
            addx2Link(Leaves.Get(4),  Leaves.Get(6),  bw_x2);
            addx2Link(Leaves.Get(5),  Leaves.Get(6),  bw_x2);
            
            addx2Link(Leaves.Get(5),  Leaves.Get(7),  bw_x2);
            
            addx2Link(Leaves.Get(2),  Leaves.Get(3),  bw_x2);
            addx2Link(Leaves.Get(2),  Leaves.Get(9),  bw_x2);
            addx2Link(Leaves.Get(2),  Leaves.Get(8),  bw_x2);
            addx2Link(Leaves.Get(3),  Leaves.Get(8),  bw_x2);
            
            addx2Link(Leaves.Get(3),  Leaves.Get(6),  bw_x2);
            addx2Link(Leaves.Get(3),  Leaves.Get(12), bw_x2);
            addx2Link(Leaves.Get(3),  Leaves.Get(9),  bw_x2);
            addx2Link(Leaves.Get(6),  Leaves.Get(9),  bw_x2);
            
            addx2Link(Leaves.Get(6),  Leaves.Get(7),  bw_x2);
            addx2Link(Leaves.Get(6),  Leaves.Get(13), bw_x2);
            addx2Link(Leaves.Get(6),  Leaves.Get(12), bw_x2);
            addx2Link(Leaves.Get(7),  Leaves.Get(12), bw_x2);
            
            addx2Link(Leaves.Get(7),  Leaves.Get(13), bw_x2);
            
            addx2Link(Leaves.Get(8),  Leaves.Get(9),  bw_x2);
            addx2Link(Leaves.Get(8),  Leaves.Get(11), bw_x2);
            addx2Link(Leaves.Get(8),  Leaves.Get(10), bw_x2);
            addx2Link(Leaves.Get(9),  Leaves.Get(10), bw_x2);
            
            addx2Link(Leaves.Get(9),  Leaves.Get(12), bw_x2);
            addx2Link(Leaves.Get(9),  Leaves.Get(14), bw_x2);
            addx2Link(Leaves.Get(9),  Leaves.Get(11), bw_x2);
            addx2Link(Leaves.Get(12), Leaves.Get(11), bw_x2);
            
            addx2Link(Leaves.Get(12), Leaves.Get(13), bw_x2);
            addx2Link(Leaves.Get(12), Leaves.Get(15), bw_x2);
            addx2Link(Leaves.Get(12), Leaves.Get(14), bw_x2);
            addx2Link(Leaves.Get(13), Leaves.Get(14), bw_x2);
            
            addx2Link(Leaves.Get(13), Leaves.Get(15), bw_x2);
            
            addx2Link(Leaves.Get(10), Leaves.Get(11), bw_x2);
            addx2Link(Leaves.Get(11), Leaves.Get(14), bw_x2);
            addx2Link(Leaves.Get(14), Leaves.Get(15), bw_x2);
            // DEPRECATED|#else
            // DEPRECATED|        // Add a full mesh between leaves to represent X2 links
            // DEPRECATED|        for (uint32_t i = 0; i < Leaves.GetN(); ++i) {
            // DEPRECATED|            for (uint32_t j = 0; j < Leaves.GetN(); ++j) {
            // DEPRECATED|                if (i == j)
            // DEPRECATED|                    continue;
            // DEPRECATED|                addLink(Leaves.Get(i), Leaves.Get(j), bw_x2);
            // DEPRECATED|            }
            // DEPRECATED|        }
            // DEPRECATED|#endif
        }
    } else {
        // Connect edge nodes to leaves - we are assuming we are dealing with a square
        // 0 1 2 3
        // 0 1 2 3
        // 4 5 6 7
        // 4 5 6 7
        double rowSize = mapSize/2;
        double columnSize = mapSize/4;
        for (uint32_t leaf = 0; leaf < Leaves.GetN(); leaf++) {
            //we are assuming leaves have already a position in the map
            Ptr<ConstantPositionMobilityModel> loc = Leaves.Get(leaf)->GetObject<ConstantPositionMobilityModel>();
            uint32_t column = (uint32_t) floor(loc->GetPosition().x/columnSize);
            uint32_t row =(uint32_t) floor(loc->GetPosition().y/rowSize);
            addLink(Leaves.Get(leaf), Get(5+column + row*4), bw_edge_leaves);
            //std::cout<< (column+row*4) << " is with " << leaf << " at " << loc->GetPosition() << std::endl;
        }
        if (has_meshed_leaves) {
            for (uint32_t leaf = 0; leaf < Leaves.GetN(); leaf++) { //not the efficient way
                Vector loc = Leaves.Get(leaf)->GetObject<ConstantPositionMobilityModel>()->GetPosition();
                for (uint32_t index = leaf+1; index < Leaves.GetN(); index++) {
                    Vector apLoc = Leaves.Get(index)->GetObject<ConstantPositionMobilityModel>()->GetPosition();
                    if (CalculateDistance(apLoc, loc) < options.cell_size*1.5){ //*1.5 so that we connect APs on the diagonal (of the grid)
                        addx2Link(Leaves.Get(leaf),  Leaves.Get(index),  bw_x2);
                        //std::cout << " adding link among " << leaf << " at " << loc <<  " and " << index << " at " << apLoc << " distance: " << CalculateDistance(apLoc, loc) << std::endl;
                    }
                }
            }
        }

    }
    std::cout << "Done initializing topology" << std::endl;
}

Ptr<Node>
MobileFatTreeTopology::GetRoot()
{
    return Get(0);
}

void
MobileFatTreeTopology::GetRootNodes(NodeContainer & n)
{
    n.Add(GetRoot());
}

void
MobileFatTreeTopology::LeafSetPosition(Ptr<Node> leaf, uint32_t x, uint32_t y)
{
    Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
    Vector vec (x, y, 0);
    loc->SetPosition(vec);
    leaf->AggregateObject(loc);
}


void
MobileFatTreeTopology::addLink(Ptr<Node> n1, Ptr<Node> n2, std::string bandwidth)
{
    NodeContainer nodes;
    nodes.Add(n1);
    nodes.Add(n2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
    pointToPoint.SetChannelAttribute ("Delay", StringValue (m_options.delay_s));
    pointToPoint.Install (nodes);

}

void
MobileFatTreeTopology::addx2Link(Ptr<Node> n1, Ptr<Node> n2, std::string bandwidth)
{
    NodeContainer nodes;
    nodes.Add(n1);
    nodes.Add(n2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
    pointToPoint.Install (nodes);

}

} // namespace ndn
} // namespace ns3
