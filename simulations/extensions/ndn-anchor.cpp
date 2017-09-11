/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 ZENG XUAN systemX institute
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

#ifdef ANCHOR

#include <memory>

#include "ndn-anchor.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/boolean.h"

#include "model/ndn-app-face.hpp"
#include "model/ndn-ns3.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"


// for signaling overhead
#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.Anchor");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Anchor);

// DEPRECATED|//const Name Anchor::LOCATION_UPDATE_PREFIX("/LocationUpdate");

/*-----------------------------------------------------------------------------
 * Constructor and application management
 *----------------------------------------------------------------------------*/

TypeId
Anchor::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::Anchor")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<Anchor>()
      .AddAttribute("Signature",
          "Fake signature, 0 valid signature (default), other values application-specific",
          UintegerValue(0),
          MakeUintegerAccessor(&Anchor::m_signature),
          MakeUintegerChecker<uint32_t>())
      .AddAttribute("EnableKite", "to kite mobility management",
               BooleanValue(false),
               MakeBooleanAccessor(&Anchor::m_kiteEnabled),
               MakeBooleanChecker())
      .AddTraceSource("ProducerSignalingOverhead", "Producer signaling overhead",
                    MakeTraceSourceAccessor(&Anchor::m_producerSignalingOverhead))
      .AddTraceSource("ForwarderSignalingOverhead", "Producer signaling overhead",
                    MakeTraceSourceAccessor(&Anchor::m_forwarderSignalingOverhead))

      ;
  return tid;
}

Anchor::Anchor()
: m_kiteEnabled(false)

{
  NS_LOG_FUNCTION_NOARGS();
}

void
Anchor::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StartApplication();

  if (m_kiteEnabled)
    return;

  FibHelper::AddRoute(GetNode(), Name(ANCHOR_UPDATE_STR), m_face, 0);
}

/*-----------------------------------------------------------------------------
 * Interest / Data processing
 *----------------------------------------------------------------------------*/

void
Anchor::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  const Name& originalName = interest->getName();
  size_t nComponents = originalName.size();
  
  // Fix hopCount + Signaling overhead statistics
  int hopCount = -1;
  auto ns3PacketTag = interest->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) {
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  if (m_kiteEnabled) {
    // XXX KITE SPECIFIC CODE... TEMPORARY IMPLEMENTATION BY XZ XXX
    // assume only producers are talking to anchor
    // we don't account for traffic lost on the road !!!! and thus underestimate
    // overhead 
    //
    // XXX be sure to only count traced interests ! for now we only receive such
    // interests but it might change in the future and produce wrong overhead
    // results
    //
    // XXX also we miss lost signalization packets 
    //
    m_producerSignalingOverhead(Simulator::Now().ToDouble(Time::S), 1);
    m_forwarderSignalingOverhead(hopCount);
    return;
  }

  Name updateName = Name(ANCHOR_UPDATE_STR);

  /* Binding update message */
  if (updateName.isPrefixOf(originalName)) {
    NS_LOG_DEBUG("Anchor received update " << originalName << ", updating binding cache");

    /* Binding update:
     *
     * /UPDATE/<seq>/<locator>/name/of/prefix
     *                        \______ ______/
     *                               V
     *                            <prefix>
     *
     * BindingCache : <prefix> -> <locator>
     */
    std::string key = originalName.getSubName(updateName.size() + 2).toUri();
    m_bindingCache[key] = originalName.getSubName(updateName.size() + 1, 1); // locator
    
    // XXX only for new entries !!!
    FibHelper::AddRoute(GetNode(), key, m_face, 0);
    
    // TODO send binding update ack  
    
    /* Signaling overhead statistics */
    m_producerSignalingOverhead(Simulator::Now().ToDouble(Time::S), 1);
    m_forwarderSignalingOverhead(hopCount);

    return;
  }
  
  NS_LOG_DEBUG("Anchor received regular interest " << originalName << ", LPM on binding cache");
  BindingCache::iterator it;

  for (size_t i = nComponents - 1; i > 0; i--) {
    it = m_bindingCache.find(originalName.getPrefix(i).toUri());
    if (it != m_bindingCache.end()) {
      // Match in binding cache
      Name newName = it->second;
      shared_ptr<Interest> modifiedInterest = make_shared<Interest>(newName.append(originalName));
      modifiedInterest->setInterestLifetime(interest->getInterestLifetime());

      NS_LOG_DEBUG("Found match in binding cache, modified and resent interest " << modifiedInterest->getName());
      m_transmittedInterests(modifiedInterest, this, m_face);
      m_face->onReceiveInterest(*modifiedInterest);
      return;
    }
  }

  NS_LOG_DEBUG("Binding cache not found, dropped the interest");
}

void
Anchor::OnData(shared_ptr<const Data> data)
{
  NS_LOG_DEBUG("Anchor received data " << data->getName());
  shared_ptr<Data> receivedData = make_shared<Data>(*data);
  receivedData->setName(data->getName().getSubName(1));
  
  /* Resign the data packet */
  Signature signature;
  SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(SIG_TLV));
  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));
  receivedData->setSignature(signature);
  
// This seems not to be neededanymore
// DISABLED|  /* Fix hopCount + Signaling overhead statistics */
// DISABLED|  int hopCount = -1;
// DISABLED|  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
// DISABLED|  if (ns3PacketTag != nullptr) {
// DISABLED|    FwHopCountTag hopCountTag;
// DISABLED|    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
// DISABLED|      hopCount = hopCountTag.Get();
// DISABLED|      NS_LOG_DEBUG("Hop count: " << hopCount);
// DISABLED|    }
// DISABLED|  }
// DISABLED|  
// DISABLED|  /* Fix hopCount for measuring path stretch */
// DISABLED|  ns3PacketTag = receivedData->getTag<Ns3PacketTag>();
// DISABLED|  FwHopCountTag hopCountTag;
// DISABLED|  Packet * pk = const_cast<Packet *>(PeekPointer(ns3PacketTag->getPacket()));
// DISABLED|  pk->RemovePacketTag(hopCountTag);
// DISABLED|  std::cout << "HOPCOUNT" << hopCount << std::endl;
// DISABLED|  for (int i = 0; i < hopCount; i++)
// DISABLED|    hopCountTag.Increment();
// DISABLED|  ns3PacketTag->getPacket()->AddPacketTag(hopCountTag);
// DISABLED|  std::cout << "HOPCOUNT COPY" << hopCountTag.Get() << std::endl;

  NS_LOG_DEBUG("Anchor modified and resent data " << receivedData->getName());
  receivedData->wireEncode();
  m_transmittedDatas(receivedData, this, m_face);
  m_face->onReceiveData(*receivedData);
}


} // namespace ndn
} // namespace ns3

#endif // ANCHOR
