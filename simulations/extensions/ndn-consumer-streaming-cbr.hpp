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

#ifndef NDN_CONSUMER_STREAMING_CBR_H
#define NDN_CONSUMER_STREAMING_CBR_H

#include "ns3/ndnSIM/apps/ndn-consumer.hpp"
#include "ndn-flow-stats.hpp"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class ConsumerStreamingCbr : public Consumer {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  ConsumerStreamingCbr();

protected:

  virtual void
  StartApplication();

  /**
   * @brief Actually send packet
   */
  virtual void
  SendPacket();

  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();

  /**
   * @brief Set type of frequency randomization
   * @param value Either 'none', 'uniform', or 'exponential'
   */
  void
  SetRandomize(const std::string& value);

  /**
   * @brief Get type of frequency randomization
   * @returns either 'none', 'uniform', or 'exponential'
   */
  std::string
  GetRandomize() const;

  void
  OnData(shared_ptr<const Data> data);

  void
  OnTimeout(uint32_t sequenceNumber);

private:
  void
  onMobilityEvent(/*char who, */ double time, char type, Ptr<Node> sta, Ptr<Node> ap);

  void
  onFlowTermination();

protected:
  double m_frequency; // Frequency of interest packets (in hertz)
  bool m_firstTime;
  Ptr<RandomVariableStream> m_random;
  std::string m_randomType;

  TracedCallback<Ptr<Application>> m_StopCallback;
  TracedCallback<FlowStats &> m_FlowStatsTrace;
  TracedCallback<FlowStats &> m_HandoverFlowStatsTrace;

private:

  double m_start_time;
  double m_mean_delay;
  double m_sent_packets;
  double m_recv_packets;
  double m_lost_packets;
  double m_mean_hop_count;

  double m_last_handover_time;
  double m_handover_mean_delay;
  double m_handover_sent_packets;
  double m_handover_recv_packets;
  double m_handover_lost_packets;
  double m_handover_mean_hop_count;
  double m_num_handover;
  
#ifdef HANDOFF_DELAYS
  double m_l3_handoff_delay;
  double m_l2_handoff_delay;
  bool m_isAssociated;
  double m_last_received_data_time;
  double m_app_handoff_delay;
#endif // HANDOFF_DELAYS
  
#ifdef STRECH_C
  double m_handover_sum_stretch;
  
#endif
};

} // namespace ndn
} // namespace ns3

#endif // NDN_CONSUMER_STREAMING_CBR_H
