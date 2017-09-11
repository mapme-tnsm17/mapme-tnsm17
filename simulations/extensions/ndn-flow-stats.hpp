#ifndef FLOW_STATS_HPP
#define FLOW_STATS_HPP

#include <string>

// Experimental extension by X2 to measure misc handover delays as (partly?)
// defined in the KITE paper. Modifications by JA to account for the real
// handovers of the flow and thus support many consumer/producer pairs.
// TO BE VALIDATED, AND IN FACT DELETED
//#define HANDOFF_DELAYS 1

//for computing path stretch
#define STRECH_C 1

namespace ns3 {
namespace ndn {

class FlowStats {
public:
    FlowStats() {};
    virtual std::string toString() = 0;
public:
    double end_time;
    std::string id;
    std::string interest_name;
    intmax_t pkts_delivered;
    intmax_t pkts_delivered_bytes;
    intmax_t raw_data_delivered_bytes;
    double elapsed;
};

class FlowStatsElastic: public FlowStats
{
public:
    static std::string getHeader();
    std::string toString();
public:
    double rate;
    double goodput;
    intmax_t num_timeouts;
    intmax_t num_holes;
};

class FlowStatsStreaming: public FlowStats
{
public:
    static std::string getHeader();
    std::string toString();
public:
    double num_sent_packets;
    double num_recv_packets;
    double num_lost_packets;
    double delay;
    double loss_rate;
    double hop_count;
    double num_handover;
#ifdef HANDOFF_DELAYS
    double l2_delay;
    double l3_delay;
    double app_handoff_delay;
#endif // HANDOFF_DELAYS
    
#ifdef STRECH_C
    double stretch;
#endif
 
};

} // ndn
} // ns3

#endif
