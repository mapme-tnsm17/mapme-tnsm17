#include <string>
#include <sstream>

#include "ndn-flow-stats.hpp"

namespace ns3 {
namespace ndn {

std::string
FlowStatsElastic::getHeader()
{
    std::ostringstream oss;
    oss <<         "end_time"
        << "\t" << "interest_name"
        << "\t" << "pkts_delivered"
        << "\t" << "pkts_delivered_bytes"
        << "\t" << "raw_data_delivered_bytes"
        << "\t" << "elapsed"
        << "\t" << "rate"
        << "\t" << "goodput"
        << "\t" << "num_timeouts"
        << "\t" << "num_holes";
    return oss.str();

}

std::string
FlowStatsElastic::toString()
{   
    std::ostringstream oss;
        
    oss << end_time
        << "\t" << interest_name
        << "\t" << pkts_delivered
        << "\t" << pkts_delivered_bytes
        << "\t" << raw_data_delivered_bytes
        << "\t" << elapsed
        << "\t" << rate
        << "\t" << goodput
        << "\t" << num_timeouts
        << "\t" << num_holes;

    return oss.str();
}


std::string
FlowStatsStreaming::getHeader()
{
    std::ostringstream oss;
    oss <<         "end_time"
        << "\t" << "interest_name"
        << "\t" << "pkts_delivered"
        << "\t" << "pkts_delivered_bytes"
        << "\t" << "raw_data_delivered_bytes"
        << "\t" << "elapsed"
        << "\t" << "num_sent_packets"
        << "\t" << "num_recv_packets"
        << "\t" << "num_lost_packets"
        << "\t" << "delay"
        << "\t" << "loss_rate"
        << "\t" << "hop_count"
        << "\t" << "num_handover"
	
#ifdef HANDOFF_DELAYS
        << "\t" << "l2_delay"
        << "\t" << "l3_delay"
       	<< "\t" << "app_handoff_delay"
#endif // HANDOFF_DELAYS
        
#ifdef STRECH_C
        << "\t" << "stretch"        
#endif
	;
    return oss.str();
}

std::string
FlowStatsStreaming::toString()
{
    std::ostringstream oss;
    
    oss << end_time
        << "\t" << interest_name
        << "\t" << pkts_delivered
        << "\t" << pkts_delivered_bytes
        << "\t" << raw_data_delivered_bytes
        << "\t" << elapsed
        << "\t" << num_sent_packets
        << "\t" << num_recv_packets
        << "\t" << num_lost_packets
        << "\t" << delay
        << "\t" << loss_rate
        << "\t" << hop_count
        << "\t" << num_handover
        
#ifdef HANDOFF_DELAYS
        << "\t" << l2_delay
        << "\t" << l3_delay
      	<< "\t" << app_handoff_delay
#endif // HANDOFF_DELAYS

#ifdef STRECH_C
        << "\t" << stretch
#endif
	;
    return oss.str();
}


} // ndn
} // ns3
