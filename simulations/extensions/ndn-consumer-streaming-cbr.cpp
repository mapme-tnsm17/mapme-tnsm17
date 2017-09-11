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

/*
 * NOTES:
 *  - supports multiple prefixes (could be refined to avoid LPM conflicts in the
 *  corresponding producer's search)
 *  - statistic about handovers support both consumer and producer mobility
 *  tracking
 */

#include "ndn-consumer-streaming-cbr.hpp"
#include "ns3/ndnSIM/apps/ndn-producer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/ndnSIM/utils/ndn-ns3-packet-tag.hpp"

#include "ns3/ndnSIM/model/ndn-app-face.hpp"

#include "connectivity-manager.hpp"

// XXX Handover and anchor specific code should be moved outside of the application
// An orchestrator should learn about handovers and query the application for
// statistics to dump.
#ifdef ANCHOR
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-anchor.hpp"
#endif // ANCHOR


//#define STRECH_C 1

#ifdef STRECH_C
#include "ns3/ndnSIM/utils/ndn-fw-from-tag.hpp"
#include "ns3/ndnSIM/model/ndn-global-router.hpp"
#include "my-global-routing-helper.hpp"
#endif

#define MAX_HANDOVER 50000

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerStreamingCbr");

// XXX stopCallback
// XXX statistics

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerStreamingCbr);

TypeId
ConsumerStreamingCbr::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerStreamingCbr")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<ConsumerStreamingCbr>()

      .AddAttribute("Frequency", "Frequency of interest packets", StringValue("1.0"),
                    MakeDoubleAccessor(&ConsumerStreamingCbr::m_frequency), MakeDoubleChecker<double>())

      .AddAttribute("Randomize",
                    "Type of send time randomization: none (default), uniform, exponential",
                    StringValue("none"),
                    MakeStringAccessor(&ConsumerStreamingCbr::SetRandomize, &ConsumerStreamingCbr::GetRandomize),
                    MakeStringChecker())

      .AddAttribute("MaxSeq", "Maximum sequence number to request",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&ConsumerStreamingCbr::m_seqMax), MakeIntegerChecker<uint32_t>())

      .AddTraceSource("FlowStats", "Flow stats trace file name, by default flow stats traces are not logged, if you don't specify a valid file name",
          MakeTraceSourceAccessor(&ConsumerStreamingCbr::m_FlowStatsTrace))

      .AddTraceSource("HandoverFlowStats", "Flow stats trace file name, by default flow stats traces are not logged, if you don't specify a valid file name",
          MakeTraceSourceAccessor(&ConsumerStreamingCbr::m_HandoverFlowStatsTrace))

      .AddTraceSource("StopCallback", "Callback called when application finishes", 
          MakeTraceSourceAccessor(&ConsumerStreamingCbr::m_StopCallback))
    ;

  return tid;
}

/*----------------------------------------------------------------------------- 
 * Callbacks
 *----------------------------------------------------------------------------*/

void
ConsumerStreamingCbr::onMobilityEvent(/* char who, */ double time, char type, Ptr<Node> sta, Ptr<Node> ap)
{
  // who : 'C' for consumer 'P' for producer
  // currently, only producer is tracked
  // let's count l3 handover delay for producer mobility only...
  
  std::cout << Simulator::Now ().ToDouble (Time::S) << " " << time << " EVENT " << GetNode()->GetId() << " " << type << " " << sta->GetId() << " " << ap->GetId() << std::endl;
  
  FlowStatsStreaming fs;

  switch (type) {
    case 'A':
      if (!m_active)
        return;

#ifdef HANDOFF_DELAYS
      m_l2_handoff_delay = Simulator::Now().ToDouble(Time::S) - m_last_handover_time;
      m_isAssociated = true;
#endif // HANDOFF_DELAYS

      m_num_handover++;
    
      if (m_num_handover > MAX_HANDOVER) {
        std::cout << "Aborting flow after " << MAX_HANDOVER << " handovers" << std::endl;
        m_active = false;
        onFlowTermination();
        Simulator::Stop();
        return;
      }
    
      // Flow stats are passed to the callback
      fs.end_time = Simulator::Now().ToDouble(Time::S);
      fs.id = GetId();
      fs.interest_name = "DUMMY";//m_interestName.toUri();
      fs.pkts_delivered = m_handover_recv_packets;
      fs.pkts_delivered_bytes = 0; //pkts_delivered_bytes;
      fs.raw_data_delivered_bytes = 0; //raw_data_delivered_bytes;
      fs.elapsed = fs.end_time - m_last_handover_time;
      fs.num_sent_packets = m_handover_sent_packets;
      fs.num_recv_packets = m_handover_recv_packets;
      fs.num_lost_packets = m_handover_lost_packets;
      fs.delay = m_handover_mean_delay;
      fs.loss_rate = m_handover_lost_packets / m_handover_sent_packets;
      fs.num_handover = m_num_handover;
      fs.hop_count = m_handover_mean_hop_count;
#ifdef HANDOFF_DELAYS
      fs.l2_delay = m_l2_handoff_delay;
      fs.l3_delay = m_l3_handoff_delay;
      fs.app_handoff_delay = m_app_handoff_delay;
      m_isAssociated = false;
#endif // HANDOFF_DELAYS
#ifdef STRECH_C
      fs.stretch = m_handover_sum_stretch / m_handover_recv_packets;
#endif
      
      m_HandoverFlowStatsTrace(fs);
    
      m_last_handover_time	    =	Simulator::Now().ToDouble(Time::S);

      m_handover_mean_delay     = 0;
      m_handover_sent_packets   = 0;
      m_handover_recv_packets   = 0;
      m_handover_lost_packets   = 0;
      m_handover_mean_hop_count = 0;
      
#ifdef HANDOFF_DELAYS
      m_l2_handoff_delay = 0;
      m_l3_handoff_delay = 0;
      m_app_handoff_delay = 0;
#endif // HANDOFF_DELAYS

#ifdef STRECH_C
      m_handover_sum_stretch = 0;
#endif

      break;

    case 'D':
      break;

    default:
      std::cerr << "Internal error" << std::endl;
      throw 0;
  }
}

void
ConsumerStreamingCbr::onFlowTermination()
{
  // Flow stats are passed to the callback
  FlowStatsStreaming fs;
  fs.end_time = Simulator::Now().ToDouble(Time::S);
  fs.id = GetId();
  fs.interest_name = "DUMMY";//m_interestName.toUri();
  fs.pkts_delivered = m_recv_packets;
  fs.pkts_delivered_bytes = 0; //pkts_delivered_bytes;
  fs.raw_data_delivered_bytes = 0; //raw_data_delivered_bytes;
  fs.elapsed = fs.end_time - m_start_time;
  fs.num_sent_packets = m_sent_packets;
  fs.num_recv_packets = m_recv_packets;
  fs.num_lost_packets = m_lost_packets;
  fs.delay = m_mean_delay;
  fs.loss_rate = m_lost_packets / m_sent_packets;
  fs.num_handover = m_num_handover;
  fs.hop_count = m_mean_hop_count;
#ifdef HANDOFF_DELAYS
  fs.l2_delay = m_l2_handoff_delay;
  fs.l3_delay = m_l3_handoff_delay;
  fs.app_handoff_delay = m_app_handoff_delay;
  m_isAssociated = false;
#endif // HANDOFF_DELAYS
  m_FlowStatsTrace(fs);
}

/*----------------------------------------------------------------------------- 
 * Constructor & application start
 *----------------------------------------------------------------------------*/

ConsumerStreamingCbr::ConsumerStreamingCbr()
  : m_frequency(1.0)
  , m_firstTime(true)
  , m_random(0)
  , m_start_time(0)
  , m_mean_delay(0)
  , m_sent_packets(0)
  , m_recv_packets(0)
  , m_lost_packets(0)
  , m_mean_hop_count(0)
  , m_last_handover_time(0)
  , m_handover_mean_delay(0)
  , m_handover_sent_packets(0)
  , m_handover_recv_packets(0)
  , m_handover_lost_packets(0)
  , m_handover_mean_hop_count(0)
  , m_num_handover(0)
#ifdef HANDOFF_DELAYS
  , m_l3_handoff_delay(0)
  , m_l2_handoff_delay(0)
  , m_isAssociated(false)
  , m_last_received_data_time(0)
  , m_app_handoff_delay(0)
#endif // HANDOFF_DELAYS
#ifdef STRECH_C
  , m_handover_sum_stretch(0)
#endif

{
  NS_LOG_FUNCTION_NOARGS();
  m_seqMax = std::numeric_limits<uint32_t>::max();
  m_start_time = Simulator::Now().ToDouble(Time::S);
  m_last_handover_time = m_start_time;
}

void
ConsumerStreamingCbr::StartApplication()
{
  Consumer::StartApplication();

  /* Track both consumer and producer(s) mobility events */
  // XXX This should be generic for any app and not just CBR
  Ptr<ConnectivityManager> cm = GetNode()->GetObject<ConnectivityManager>();
//DISABLED|  if (cm)
//DISABLED|    cm->TraceConnectWithoutContext("MobilityEvent", MakeCallback(&ConsumerStreamingCbr::onMobilityEvent, this, 'C'));

  for (NodeList::Iterator inode = NodeList::Begin(); inode != NodeList::End(); inode++) {
    Ptr<Node> node = *inode;

    /* To check whether the node is a producer for this content, we search for
     * FIB entries that serves a larger prefix locally.
     *
     * NOTE:
     *  - With wireless producers since the prefix will be advertized and
     *  inserted in the FIB only after the node is connected through WiFi, we
     *  cannot look at the FIB...
     *  - Let's fall back on GlobalRouter then... but, in case of Anchor, this
     *  is not necessarily the producer, but it might be the anchor...
     *  - In fact, a producer for /prefix has an app of type producer serving /prefix
     *
     *
     * TODO 
     *  - We might improve this by checking that there is no longest prefix
     * match with no local face on the node.
     */

    for (uint32_t i = 0; i < node->GetNApplications(); i++) {
      Ptr<Producer> p = DynamicCast<Producer>(node->GetApplication(i));
      if (!p) 
        continue;
#ifdef ANCHOR
      Name prefixedInterestName(ANCHOR_ORIGIN_STR);
      prefixedInterestName.append(m_interestName);
      if ((p->GetPrefix().isPrefixOf(m_interestName)) || (p->GetPrefix().isPrefixOf(prefixedInterestName))) {
#else
      if (p->GetPrefix().isPrefixOf(m_interestName)) {
#endif // ANCHOR
          Ptr<ConnectivityManager> p_cm = node->GetObject<ConnectivityManager>();
          if (p_cm) {
            p_cm->TraceConnectWithoutContext("MobilityEvent", MakeCallback(&ConsumerStreamingCbr::onMobilityEvent, this));
          }
          break;
      }
    }

#if 0
    Ptr<GlobalRouter> gr = node->GetObject<GlobalRouter>();
    assert(gr);
    for (auto &prefix : gr->GetLocalPrefixes()) {
      std::cout << "?" << *prefix << " vs " << m_interestName << std::endl;
      if (prefix->isPrefixOf(m_interestName)) {
          std::cout << "OK" << std::endl;
          Ptr<ConnectivityManager> p_cm = node->GetObject<ConnectivityManager>();
          if (p_cm) {
            std::cout << "registering handover on the producer side -----------------" << std::endl;
            p_cm->TraceConnectWithoutContext("MobilityEvent", MakeCallback(&ConsumerStreamingCbr::onMobilityEvent, this));
          } else {
            std::cout << "no cm" << std::endl;
          }
          break;
      }
    }
#endif

#if 0
    Ptr<L3Protocol> l3 = node->GetObject<L3Protocol>();
    assert(l3);
    for (auto & iEntry : l3->getForwarder()->getFib()) {
      std::cout << iEntry.getPrefix() << " is prefix of " << m_interestName << std::endl;
      if (!iEntry.getPrefix().isPrefixOf(m_interestName)) // eg. prefix != /prefix/OBJ22/3 
        continue;
      for (auto & iNextHop : iEntry.getNextHops()) {
        if(iNextHop.getFace()->getLocalUri() == FaceUri("appFace://")) {
          // The node is a producer
          Ptr<ConnectivityManager> p_cm = node->GetObject<ConnectivityManager>();
          if (p_cm) {
            std::cout << "registering handover on the producer side -----------------" << std::endl;
            p_cm->TraceConnectWithoutContext("MobilityEvent", MakeCallback(&ConsumerStreamingCbr::onMobilityEvent, this));
          }
          break;
        }
      }
    }
#endif       
  }
}

/*----------------------------------------------------------------------------- 
 * Interest/Data management
 *----------------------------------------------------------------------------*/

void
ConsumerStreamingCbr::SendPacket()
{
  if (!m_active){
    std::cout << "********* inactive **********" << std::endl;
    return;
  }
  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  while (m_retxSeqs.size()) {
    std::cout << "RET: SHOULD NOT HAPPEN" << std::endl;
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) {
    // No retransmissions
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      // Finite flow
      if (m_seq >= m_seqMax) {
        onFlowTermination();
        m_StopCallback(this);
        return; // we are totally done
      }
    }

    seq = m_seq++;
  } else {
    std::cout << "RETRANSMIT: SHOULD NOT HAPPEN" << std::endl;
  }

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);

  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << seq);
  
  WillSendOutInterest(seq);
  m_sent_packets++;
  m_handover_sent_packets++;

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  ScheduleNextPacket();
}

void
ConsumerStreamingCbr::ScheduleNextPacket()
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";

  if (m_firstTime) {
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &ConsumerStreamingCbr::SendPacket, this);
    m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning())
    m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
                                                      : Seconds(m_random->GetValue()),
                                      &ConsumerStreamingCbr::SendPacket, this);
}

void
ConsumerStreamingCbr::SetRandomize(const std::string& value)
{

  if (value == "uniform") {
    m_random = CreateObjectWithAttributes<UniformRandomVariable>(
        "Min", DoubleValue(0.0),
        "Max", DoubleValue(2 * 1.0 / m_frequency)
    );
  }
  else if (value == "exponential") {
    m_random = CreateObjectWithAttributes<ExponentialRandomVariable>(
        "Mean" , DoubleValue( 1.0 / m_frequency),
        "Bound", DoubleValue(50.0 / m_frequency)
    );
  }
  else {
    m_random = 0;
  }

  m_randomType = value;
}

std::string
ConsumerStreamingCbr::GetRandomize() const
{
  return m_randomType;
}

void
ConsumerStreamingCbr::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  // This could be a problem......
  uint32_t seq = data->getName().at(-1).toSequenceNumber();

#ifdef HANDOFF_DELAYS
  if (m_isAssociated && m_l3_handoff_delay == 0) {
    m_l3_handoff_delay = Simulator::Now().ToDouble(Time::S) - m_last_handover_time - m_l2_handoff_delay;
    m_app_handoff_delay = Simulator::Now().ToDouble(Time::S) - m_last_received_data_time;
  }
  m_last_received_data_time = Simulator::Now().ToDouble(Time::S);
#endif // HANDOFF_DELAYS
  
  // hopCount accounting
  int hopCount = -1;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) {
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    } else {
      std::cerr << "Missing hop count tag info" << std::endl;
    }
  } else {
      std::cerr << "Missing hop count tag" << std::endl;
  } 

  SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
  if (entry != m_seqLastDelay.end()) {
    m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
  }


  entry = m_seqFullDelay.find(seq);
  if (entry != m_seqFullDelay.end()) {
    m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
  }

#ifdef STRECH_C
  double path_stretch =-1;
  if (ns3PacketTag != nullptr) {
    FwFromTag fromHopTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(fromHopTag)) {
      //get from hop id:
      uint32_t fromHopId = fromHopTag.Get();

      

      //NOTE: this is coupled with mobility event, so not reliable:
      Ptr<ConnectivityManager> cm = GetNode()->GetObject<ConnectivityManager>();
      NS_ASSERT_MSG(cm!=0, "consumer node has not connectivity manager");

      Ptr<Node> ap= cm->getCurrentAp();
      NS_ASSERT_MSG(ap!=0, "consumer is connected to multiple base stations, not able to determine the base station");
          if(ap){ // in normal state
               
                //get to hop id:
                uint32_t toHopId = ap->GetId();
      
                int spt_distance_aps = MyGlobalRoutingHelper::getSPTdistanceBetweenAp(fromHopId,toHopId);
                if(hopCount!=-1 && spt_distance_aps>=0)
                    path_stretch = (double)hopCount/(spt_distance_aps+2.0);
          }

  
    }
  }

    
#endif


  m_mean_delay *= m_recv_packets;
  m_mean_hop_count *= m_recv_packets;
  m_recv_packets++;
  m_mean_delay += (Simulator::Now() - entry->time).ToDouble(Time::S);
  m_mean_hop_count += hopCount;
  m_mean_delay /= m_recv_packets;
  m_mean_hop_count /= m_recv_packets;

  m_handover_mean_delay *= m_handover_recv_packets;
  m_handover_mean_hop_count *= m_handover_recv_packets;
  m_handover_recv_packets++;
  m_handover_mean_delay += (Simulator::Now() - entry->time).ToDouble(Time::S);
  m_handover_mean_hop_count += hopCount;
  m_handover_mean_delay /= m_handover_recv_packets;
  m_handover_mean_hop_count /= m_handover_recv_packets;
  
#ifdef STRECH_C
  m_handover_sum_stretch += path_stretch;
#endif

  m_seqRetxCounts.erase(seq);
  m_seqFullDelay.erase(seq);
  m_seqLastDelay.erase(seq);

  m_seqTimeouts.erase(seq);
  m_retxSeqs.erase(seq);

  m_rtt->AckSeq(SequenceNumber32(seq));
}

void
ConsumerStreamingCbr::OnTimeout(uint32_t sequenceNumber)
{
// DISABLED|  std::cout << GetId() << " ON TIMEOUT " << Simulator::Now().ToDouble(Time::S) << " " << Simulator::Now().ToDouble(Time::S) - m_start_time << " " << sequenceNumber << std::endl;
  NS_LOG_FUNCTION(sequenceNumber);
  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  // XXX XXX m_rtt->IncreaseMultiplier(); // Double the next RTO
  m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
                 1); // make sure to disable RTT calculation for this sample
  //m_retxSeqs.insert(sequenceNumber);
  m_lost_packets++;
  m_handover_lost_packets++;
  if (m_active)
    ScheduleNextPacket();
}

} // namespace ndn
} // namespace ns3
