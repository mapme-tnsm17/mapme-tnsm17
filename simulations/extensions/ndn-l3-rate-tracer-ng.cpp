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

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

#include "ndn-l3-rate-tracer-ng.hpp"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/config.h"
#include "ns3/callback.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/node-list.h"

#include "daemon/table/pit-entry.hpp"

#include <fstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.L3RateTracerNg");

namespace ns3 {
namespace ndn {

static std::list<std::tuple<shared_ptr<std::ostream>, std::list<Ptr<L3RateTracerNg>>>>
  g_tracers;

void
L3RateTracerNg::Destroy()
{
  g_tracers.clear();
}

void
L3RateTracerNg::InstallAll(const std::string& file, Time averagingPeriod /* = Seconds (0.5)*/)
{
  std::list<Ptr<L3RateTracerNg>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
    Ptr<L3RateTracerNg> trace = Install(*node, outputStream, averagingPeriod);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
    *outputStream << "\n";
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
L3RateTracerNg::Install(const NodeContainer& nodes, const std::string& file,
                      Time averagingPeriod /* = Seconds (0.5)*/)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<L3RateTracerNg>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  for (NodeContainer::Iterator node = nodes.Begin(); node != nodes.End(); node++) {
    Ptr<L3RateTracerNg> trace = Install(*node, outputStream, averagingPeriod);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
    *outputStream << "\n";
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
L3RateTracerNg::Install(Ptr<Node> node, const std::string& file,
                      Time averagingPeriod /* = Seconds (0.5)*/)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<L3RateTracerNg>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  Ptr<L3RateTracerNg> trace = Install(node, outputStream, averagingPeriod);
  tracers.push_back(trace);

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
    *outputStream << "\n";
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

Ptr<L3RateTracerNg>
L3RateTracerNg::Install(Ptr<Node> node, shared_ptr<std::ostream> outputStream,
                      Time averagingPeriod /* = Seconds (0.5)*/)
{
  NS_LOG_DEBUG("Node: " << node->GetId());

  Ptr<L3RateTracerNg> trace = Create<L3RateTracerNg>(outputStream, node);
  trace->SetAveragingPeriod(averagingPeriod);

  return trace;
}

L3RateTracerNg::L3RateTracerNg(shared_ptr<std::ostream> os, Ptr<Node> node)
  : L3Tracer(node)
  , m_os(os)
{
  SetAveragingPeriod(Seconds(1.0));
}

L3RateTracerNg::L3RateTracerNg(shared_ptr<std::ostream> os, const std::string& node)
  : L3Tracer(node)
  , m_os(os)
{
  SetAveragingPeriod(Seconds(1.0));
}

L3RateTracerNg::~L3RateTracerNg()
{
  m_printEvent.Cancel();
}

void
L3RateTracerNg::SetAveragingPeriod(const Time& period)
{
  m_period = period;
  m_printEvent.Cancel();
  m_printEvent = Simulator::Schedule(m_period, &L3RateTracerNg::PeriodicPrinter, this);
}

void
L3RateTracerNg::PeriodicPrinter()
{
  Print(*m_os);
  Reset();

  m_printEvent = Simulator::Schedule(m_period, &L3RateTracerNg::PeriodicPrinter, this);
}

void
L3RateTracerNg::PrintHeader(std::ostream& os) const
{
  os << "Time"
     << "\t"

     << "Node"
     << "\t"
     << "FaceId"
     << "\t"
     << "FaceDescr"
     << "\t"
     << "OtherNode"
     << "\t"

     << "Type"
     << "\t"
     << "Packets"
     << "\t"
     << "Kilobytes"
     << "\t"
     << "PacketRaw"
     << "\t"
     << "KilobytesRaw";
}

void
L3RateTracerNg::Reset()
{
  for (auto& stats : m_stats) {
    std::get<0>(stats.second).Reset();
    std::get<1>(stats.second).Reset();
  }
}

const double alpha = 0.8;

#define STATS(INDEX) std::get<INDEX>(stats.second)
#define RATE(INDEX, fieldName) STATS(INDEX).fieldName / m_period.ToDouble(Time::S)

#define PRINTER(printName, fieldName) do {                                                         \
  STATS(2).fieldName =                                                                             \
    /*new value*/ alpha * RATE(0, fieldName) + /*old value*/ (1 - alpha) * STATS(2).fieldName;     \
  STATS(3).fieldName = /*new value*/ alpha * RATE(1, fieldName) / 1024.0                           \
                       + /*old value*/ (1 - alpha) * STATS(3).fieldName;                           \
                                                                                                   \
  Ptr<Node> otherNode;                                                                             \
  Ptr<L3Protocol> ap_l3 = m_nodePtr->GetObject<L3Protocol>();                                      \
  for (unsigned int i = 0; i < m_nodePtr->GetNDevices(); i++) {                                    \
    Ptr<NetDevice> d = m_nodePtr->GetDevice(i);                                                    \
    Ptr<PointToPointNetDevice> pd = DynamicCast<PointToPointNetDevice>(d);                         \
    shared_ptr<Face> tmp_face = ap_l3->getFaceByNetDevice(pd);                                     \
    if (tmp_face != stats.first) continue;                                                         \
    if (!pd) continue;                                                                             \
    Ptr<Channel> ch = pd->GetChannel();                                                            \
    assert(ch->GetNDevices() == 2);                                                                \
    for (uint32_t deviceId = 0; deviceId < ch->GetNDevices(); deviceId++) {                        \
      Ptr<NetDevice> otherSide = ch->GetDevice(deviceId);                                          \
      if (pd == otherSide) continue;                                                               \
      otherNode = otherSide->GetNode();                                                            \
    }                                                                                              \
  }                                                                                                \
  if (!otherNode) continue;                                                                        \
  os << time.ToDouble(Time::S) << "\t" << m_node << "\t"                                           \
     << stats.first->getId() << "\t" << stats.first->getLocalUri() << "\t"                         \
     << otherNode->GetId() << "\t"                                                                  \
     << printName << "\t" << STATS(2).fieldName << "\t" << STATS(3).fieldName << "\t"              \
     << STATS(0).fieldName << "\t" << STATS(1).fieldName / 1024.0 << "\n";                         \
} while(0)

void
L3RateTracerNg::Print(std::ostream& os) const
{
  Time time = Simulator::Now();

  for (auto& stats : m_stats) {
    if ((!stats.first) || (stats.first->isLocal()))
      continue;

    PRINTER("InInterests", m_inInterests);
    PRINTER("OutInterests", m_outInterests);

    PRINTER("InData", m_inData);
    PRINTER("OutData", m_outData);

    PRINTER("InSatisfiedInterests", m_satisfiedInterests);
    PRINTER("InTimedOutInterests", m_timedOutInterests);

    PRINTER("OutSatisfiedInterests", m_outSatisfiedInterests);
    PRINTER("OutTimedOutInterests", m_outTimedOutInterests);
  }

#if 0
  {
    auto i = m_stats.find(nullptr);
    if (i != m_stats.end()) {
      auto& stats = *i;
      PRINTER("SatisfiedInterests", m_satisfiedInterests);
      PRINTER("TimedOutInterests", m_timedOutInterests);
    }
  }
#endif
}

void
L3RateTracerNg::OutInterests(const Interest& interest, const Face& face)
{
  std::get<0>(m_stats[face.shared_from_this()]).m_outInterests++;
  if (interest.hasWire()) {
    std::get<1>(m_stats[face.shared_from_this()]).m_outInterests +=
      interest.wireEncode().size();
  }
}

void
L3RateTracerNg::InInterests(const Interest& interest, const Face& face)
{
  std::get<0>(m_stats[face.shared_from_this()]).m_inInterests++;
  if (interest.hasWire()) {
    std::get<1>(m_stats[face.shared_from_this()]).m_inInterests +=
      interest.wireEncode().size();
  }
}

void
L3RateTracerNg::OutData(const Data& data, const Face& face)
{
  std::get<0>(m_stats[face.shared_from_this()]).m_outData++;
  if (data.hasWire()) {
    std::get<1>(m_stats[face.shared_from_this()]).m_outData +=
      data.wireEncode().size();
  }
}

void
L3RateTracerNg::InData(const Data& data, const Face& face)
{
  std::get<0>(m_stats[face.shared_from_this()]).m_inData++;
  if (data.hasWire()) {
    std::get<1>(m_stats[face.shared_from_this()]).m_inData +=
      data.wireEncode().size();
  }
}

void
L3RateTracerNg::SatisfiedInterests(const nfd::pit::Entry& entry, const Face&, const Data&)
{
  std::get<0>(m_stats[nullptr]).m_satisfiedInterests++;
  // no "size" stats

  for (const auto& in : entry.getInRecords()) {
    std::get<0>(m_stats[in.getFace()]).m_satisfiedInterests ++;
  }

  for (const auto& out : entry.getOutRecords()) {
    std::get<0>(m_stats[out.getFace()]).m_outSatisfiedInterests ++;
  }
}

void
L3RateTracerNg::TimedOutInterests(const nfd::pit::Entry& entry)
{
  std::get<0>(m_stats[nullptr]).m_timedOutInterests++;
  // no "size" stats

  for (const auto& in : entry.getInRecords()) {
    std::get<0>(m_stats[in.getFace()]).m_timedOutInterests++;
  }

  for (const auto& out : entry.getOutRecords()) {
    std::get<0>(m_stats[out.getFace()]).m_outTimedOutInterests++;
  }
}

} // namespace ndn
} // namespace ns3
