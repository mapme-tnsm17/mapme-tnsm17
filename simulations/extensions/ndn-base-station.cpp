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

#include "ndn-base-station.hpp"

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-app-face.hpp"
#include "model/ndn-ns3.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

// DEPRECATED|#include "ndn-anchor.hpp"
// DEPRECATED|#include "ns3/ndnSIM/NFD/daemon/fw/forwarder.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-anchor.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.BaseStation");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(BaseStation);

/*-----------------------------------------------------------------------------
 * Constructor and application management
 *----------------------------------------------------------------------------*/

TypeId
BaseStation::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::BaseStation")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<BaseStation>()
      .AddAttribute("Signature", 
          "Fake signature, 0 valid signature (default), other values application-specific",
          UintegerValue(0), MakeUintegerAccessor(&BaseStation::m_signature),
          MakeUintegerChecker<uint32_t>())
      ;
  return tid;
}

void
BaseStation::PreInitialize()
{
  /* Initialize locator and make it known to routing: ANCHOR_LOCATOR_STR + <NODE_ID>
   *
   * NOTE: we get node id when application starts, since it is not possible in
   * the constructor
   */
  m_locator = ANCHOR_LOCATOR_STR + boost::lexical_cast<std::string>(GetNode()->GetId());
}

void
BaseStation::StartApplication()
{
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_locator, m_face, 0);

  /* Register for announcement prefixes */
  FibHelper::AddRoute(GetNode(), Name(ANCHOR_ANNOUNCE_STR), m_face, 0);
}

/*-----------------------------------------------------------------------------
 * Interest / Data processing
 *----------------------------------------------------------------------------*/

void
BaseStation::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside


  Name announcePrefix = Name(ANCHOR_ANNOUNCE_STR);

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  const Name& locatorName = interest->getName();

  /* Prefix announcement : /ANNOUNCE/<seq>/<prefix> */
  if (announcePrefix.isPrefixOf(locatorName)) {
    /* 1) Upon reception of a prefix announcement, send a binding update message */
    NS_LOG_DEBUG("Base station received announcement");

    /* Get the name of the prefix which is announced:
     *
     * /ANNOUNCE/<seq>/name/of/prefix
     *                 \______ _____/
     *                        V
     *                    <prefix>
     */
    const Name& seq    = locatorName.getSubName(announcePrefix.size(), 1);
    const Name& prefix = locatorName.getSubName(announcePrefix.size() + 1);

    /*
     * 2) Create binding update:
     *
     * /UPDATE/<seq>/<locator>/<prefix>
     *
     * <locator> is the locator name of the base station
     */
    Name updateName(ANCHOR_UPDATE_STR);
    updateName.append(seq).append(m_locator).append(prefix);
    std::cout << "seq" << seq << " - prefix " << prefix << " - BS UPDATE NAME " << updateName.toUri() << std::endl;
   
    shared_ptr<Interest> updateInterest = make_shared<Interest>(updateName, time::milliseconds(ANCHOR_DEFAULT_RETX));
    
    m_transmittedInterests(updateInterest, this, m_face);
    m_face->onReceiveInterest(*updateInterest);
    NS_LOG_DEBUG("Base station sending update interest " << updateName);
    
    /* Updating local FIB to point to the mobile producer
     *
     * PIT is used to discover the incoming face
     */
    Ptr<L3Protocol> l3 = GetNode()->GetObject<L3Protocol>();
    NS_ASSERT(l3);
    nfd::Pit & pit = l3->getForwarder()->getPit();

    std::pair<shared_ptr<nfd::pit::Entry>, bool> pitEntryPair = pit.insert(*interest);
    if (pitEntryPair.second) {
      NS_LOG_DEBUG("PIT Entry for announce interest not found");
      pit.erase(pitEntryPair.first);
    } else {
      const nfd::pit::InRecordCollection& inrecords = pitEntryPair.first->getInRecords();
      // for (nfd::pit::InRecordCollection::const_iterator it = inrecords.begin(); it!= inrecords.end(); ++it)
      for (auto it : inrecords) {
        shared_ptr<Face> inFace = it.getFace();
        Name originName(ANCHOR_ORIGIN_STR);
        originName.append(Name(prefix));
        FibHelper::AddRoute(GetNode(), originName, inFace, 0);
        NS_LOG_DEBUG("Base station added FIB entry for prefix " << originName << ", face=" << inFace->getId());
      }
    }

    /* 3) Send ack */
    auto data = make_shared<Data>();
    data->setName(locatorName);
    data->setFreshnessPeriod(::ndn::time::milliseconds(0)); // BUGFIX : don't cache // ::ndn::time::milliseconds(ANCHOR_DEFAULT_RETX));
    data->setContent(make_shared<::ndn::Buffer>(BUFFER));

    Signature signature;
    SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(SIG_TLV));
    signature.setInfo(signatureInfo);
    signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));
    data->setSignature(signature);

    data->wireEncode();

    std::cout << "BS send ack" << std::endl;
    m_transmittedDatas(data, this, m_face);
    m_face->onReceiveData(*data);

    return;
  }

  /* Redirected consumer interest : /<locator>/<prefix> */
  if (Name(m_locator).isPrefixOf(locatorName)) {
    NS_LOG_DEBUG("Base station received regular interest " << locatorName);

    /* LOCATOR/prefix -> ORIGIN/prefix */
    const Name basePrefix(locatorName.getSubName(Name(m_locator).size()));
    Name originName(ANCHOR_ORIGIN_STR);
    originName.append(basePrefix);

    shared_ptr<Interest> modifiedInterest = make_shared<Interest>(*interest);
    modifiedInterest->setName(originName);
    NS_LOG_DEBUG("Base station removed location prefix and resent interest " << originName);

    /* ... and send it through local face. This will be routed by the local FIB
     * entry
     */
    m_transmittedInterests(modifiedInterest, this, m_face);
    m_face->onReceiveInterest(*modifiedInterest);
  }
}

void
BaseStation::OnData(shared_ptr<const Data> data)
{
  NS_LOG_DEBUG("Base station received consumer data " << data->getName());

  const Name& originName = data->getName();

  /* ORIGIN/prefix -> LOCATOR/prefix */
  const Name basePrefix(originName.getSubName(Name(ANCHOR_ORIGIN_STR).size()));
  Name locatorName(m_locator);
  locatorName.append(basePrefix);

  /* Copy and make new data with locator name */
  shared_ptr<Data> receivedData = make_shared<Data>(*data);
  receivedData->setName(locatorName);
  
  /* Re-sign the data packet */
  Signature signature;
  SignatureInfo signatureInfo(static_cast<::ndn::tlv::SignatureTypeValue>(SIG_TLV));
  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));
  receivedData->setSignature(signature);

  NS_LOG_DEBUG("Base station added prefix and resent data " << receivedData->getName());
  receivedData->wireEncode();

  m_transmittedDatas(receivedData, this, m_face);
  m_face->onReceiveData(*receivedData);
}

} // namespace ndn
} // namespace ns3

#endif // ANCHOR
