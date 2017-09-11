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

#include "ndn-stopping-producer.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.StoppingProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(StoppingProducer);

TypeId
StoppingProducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::StoppingProducer")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<StoppingProducer>();
  return tid;
}

StoppingProducer::StoppingProducer()
  : m_stopped(false)
{
  NS_LOG_FUNCTION_NOARGS();
}

void
StoppingProducer::Stop()
{
    m_stopped = true;
}

void
StoppingProducer::OnInterest(shared_ptr<const Interest> interest)
{
  if (m_stopped)
    App::OnInterest(interest); // tracing inside
  else
    Producer::OnInterest(interest); // tracing inside
}

} // namespace ndn
} // namespace ns3
