#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"

#include "boost/lexical_cast.hpp"
#include "mobile_fat_tree_with_border.hpp"

/* Nodes = 1 core + 4 aggregation + 12 edge */
#define NUM_NODES 17 

#define RANDOM_WAYPOINT 1

#define XSHIFT 400 //this is used to move nodes in the wired part and see then in a better way in the visulizer

namespace ns3 {
namespace ndn {

MobileFatTreeTopologyWithBorder::MobileFatTreeTopologyWithBorder(NodeContainer & Leaves, Options & options, bool has_meshed_leaves)
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

    //core
    Ptr<ConstantPositionMobilityModel> loc_core = CreateObject<ConstantPositionMobilityModel> ();
    Vector vec_core (XSHIFT + 165, -100, 0);
	loc_core->SetPosition(vec_core);
    Get(0)->AggregateObject(loc_core);

    //aggregation
    for(uint32_t i = 1; i <= 4; i++){
        Ptr<ConstantPositionMobilityModel> loc_aggr = CreateObject<ConstantPositionMobilityModel> ();
        Vector vec_aggr (XSHIFT + 30 + (i-1)*90, -80, 0);
	    loc_aggr->SetPosition(vec_aggr);
        Get(i)->AggregateObject(loc_aggr);
    }

    //edge
    for(uint32_t i = 5; i < 17; i++){
        Ptr<ConstantPositionMobilityModel> loc_edge = CreateObject<ConstantPositionMobilityModel> ();
        Vector vec_edge (XSHIFT + (i-5)*30, -60, 0);
	    loc_edge->SetPosition(vec_edge);
        Get(i)->AggregateObject(loc_edge);
    }

    // Connect core node to aggregation nodes
    Ptr<Node> core = Get(0);
    for (uint32_t aggr = 1; aggr <=4; aggr++) {
      addLink(core, Get(aggr), bw_core_aggr);
    }

    // Connect aggregation nodes to edge nodes
    uint32_t edge_node = 5;
    for(uint32_t aggr=1; aggr <= 4; aggr++){
        addLink(Get(aggr),Get(edge_node),bw_aggr_edge);
        addLink(Get(aggr),Get(edge_node+1),bw_aggr_edge);
        addLink(Get(aggr),Get(edge_node+2),bw_aggr_edge);

        if((aggr%2) != 0){
            addLink(Get(aggr),Get(edge_node+3),bw_aggr_edge);
            addLink(Get(aggr),Get(edge_node+4),bw_aggr_edge);
            addLink(Get(aggr),Get(edge_node+5),bw_aggr_edge);   
        }else{
            addLink(Get(aggr),Get(edge_node-1),bw_aggr_edge);
            addLink(Get(aggr),Get(edge_node-2),bw_aggr_edge);
            addLink(Get(aggr),Get(edge_node-3),bw_aggr_edge);
        }        
        edge_node+=3;
    }

     

    addLink(Get(5), Get(6), bw_aggr);
    addLink(Get(6), Get(7), bw_aggr);

    addLink(Get(8), Get(9), bw_aggr);
    addLink(Get(9), Get(10), bw_aggr);

    addLink(Get(11), Get(12), bw_aggr);
    addLink(Get(12), Get(13), bw_aggr);

    addLink(Get(14), Get(15), bw_aggr);
    addLink(Get(15), Get(16), bw_aggr);

    // Connect edge nodes to leaves
    uint32_t leaf_node = 0;
    for (uint32_t edge=5; edge<=16; edge++){
        addLink(Get(edge), Leaves.Get(leaf_node), bw_edge_leaves);
        addLink(Get(edge), Leaves.Get(leaf_node+1), bw_edge_leaves);
        addLink(Get(edge), Leaves.Get(leaf_node+2), bw_edge_leaves);
        leaf_node+=3;
    }

    //leaf positioning

    for (uint32_t l = 0; l < 36; l++){
        LeafSetPosition(Leaves.Get(l),  (l/6) * options.cell_size, (l%6) * options.cell_size);   
    }

    if(has_meshed_leaves){  
        for(uint32_t l=0; l <= 34; l++){
            for(uint32_t l_in = l+1; l_in <=35; l_in++){
                try_to_add_x2_link(Leaves.Get(l), Leaves.Get(l_in), bw_x2);
            }    
        }
    }

/*     if (has_meshed_leaves) {
#ifdef RANDOM_WAYPOINT
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
#else
        // Add a full mesh between leaves to represent X2 links
        for (uint32_t i = 0; i < Leaves.GetN(); ++i) {
            for (uint32_t j = 0; j < Leaves.GetN(); ++j) {
                if (i == j)
                    continue;
                addLink(Leaves.Get(i), Leaves.Get(j), bw_x2);
            }
        }
#endif
    }*/
    std::cout << "Done initializing topology" << std::endl;


	for(int i = 0; i < 16; i++){
		Ptr<MobilityModel> mobModel = Leaves.Get(i)->GetObject<MobilityModel> ();
  		Vector3D pos = mobModel->GetPosition ();
		std::cout << "pos node " << i << "(" << pos.x << "," << pos.y << "," << pos.z
			<< ")" << std::endl; 
	}
}

Ptr<Node>
MobileFatTreeTopologyWithBorder::GetRoot()
{
    return Get(0);
}

void
MobileFatTreeTopologyWithBorder::GetRootNodes(NodeContainer & n)
{
    n.Add(GetRoot());
}

void
MobileFatTreeTopologyWithBorder::LeafSetPosition(Ptr<Node> leaf, uint32_t x, uint32_t y)
{
    std::cout << "Node id " << leaf->GetId() << " X " << x << " Y " << y << std::endl;
    Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
    Vector vec (x, y, 0);
    loc->SetPosition(vec);
    leaf->AggregateObject(loc);
}


void
MobileFatTreeTopologyWithBorder::addLink(Ptr<Node> n1, Ptr<Node> n2, std::string bandwidth)
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
MobileFatTreeTopologyWithBorder::addx2Link(Ptr<Node> n1, Ptr<Node> n2, std::string bandwidth)
{
    NodeContainer nodes;
    nodes.Add(n1);
    nodes.Add(n2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
    pointToPoint.Install (nodes);

}

void MobileFatTreeTopologyWithBorder::try_to_add_x2_link(Ptr<Node> n1, Ptr<Node> n2, std::string bandwidth){
    Ptr<MobilityModel> mob_mod_n1 = n1->GetObject<MobilityModel> ();
	Vector3D pos_n1 = mob_mod_n1->GetPosition ();
    
    Ptr<MobilityModel> mob_mod_n2 = n2->GetObject<MobilityModel> ();
	Vector3D pos_n2 = mob_mod_n2->GetPosition ();

    double diff_x = pos_n2.x - pos_n1.x;
    double diff_y = pos_n2.y - pos_n1.y;
    double distance = sqrt(diff_x*diff_x + diff_y*diff_y);
    
    //ATTENTION: this works only with AP at 80m
    if(distance < 114){
        std::cout << "x2 " << n1->GetId() << " " << n2->GetId() << std::endl;  
        NodeContainer nodes;
        nodes.Add(n1);
        nodes.Add(n2);

        PointToPointHelper pointToPoint;
        pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
        pointToPoint.SetChannelAttribute ("Delay", StringValue ("0.1ms"));
        pointToPoint.Install (nodes);
    }
}

} // namespace ndn
} // namespace ns3
