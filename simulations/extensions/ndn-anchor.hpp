/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015  Zeng Xuan   SystemX insitute of Technology
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

#ifndef NDN_ANCHOR_H
#define NDN_ANCHOR_H

/* Example deployment for a producer serving /prefix
 * 
 * - /prefix announced by ANCHOR to the routing layer. Thus, forwarding for
 *   /prefix converges to ANCHOR.
 *
 * 1) update process
 *
 * PRODUCER     BASE_STATION                    ANCHOR
 *
 *   ----------------->
 *   /announcement/1/prefix
 *
 *                      --------------------------->
 *                      /prefix/LocationUpdate/1
 *                      /LocationUpdate/router1/served/prefix ?????????
 *
 * Data packets serve as acknowlegements
 *
 *
 * 2) regular interest forwarding
 *
 * PRODUCER     BASE_STATION                    ANCHOR
 */

#ifdef ANCHOR

#include<boost/unordered_map.hpp>

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-anchor.hpp"

#define BUFFER 20
#define SIG_TLV 255

namespace ns3 {
namespace ndn {

typedef boost::unordered_map<std::string /*Real name prefix*/, Name /*name prefix of router or location*/ > BindingCache;

/**
 * @ingroup ndn-apps
 * @brief an application to implement the packet processing(redirect packets to
 * the new positon of producer) on the anchor node in the "ANCHOR BASED"
 * approach for producer mobility
 */
class Anchor : public App {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  Anchor();

  virtual ~Anchor() {}

   /**
   * @brief Method that will be called every time new Interest arrives
   * @param interest Interest header
   * @param packet "Payload" of the interests packet. The actual payload should
   * be zero, but packet itself may be useful to get packet tags
   */
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  /**
   * @brief Method that will be called every time new Data arrives
   * @param contentObject Data header
   * @param payload payload (potentially virtual) of the Data packet (may include packet tags of
   * original packet)
   */
  virtual void
  OnData(shared_ptr<const Data> data);
  
protected:

  virtual void
  StartApplication();

private:
  BindingCache m_bindingCache;

  uint32_t m_signature;

  TracedCallback<double, uint32_t> m_producerSignalingOverhead;
  TracedCallback<uint32_t> m_forwarderSignalingOverhead;

  bool m_kiteEnabled;

};

} // namespace ndn
} // namespace ns3

#endif // ANCHOR

#endif
