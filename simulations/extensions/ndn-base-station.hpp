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

#ifndef NDN_BASE_STATION_H
#define NDN_BASE_STATION_H

#ifdef ANCHOR

#include <boost/unordered_map.hpp>

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-anchor.hpp"

#define BUFFER 20
#define SIG_TLV 255

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief an application to implement the packet processing(redirect packets to
 * the new positon of producer) on the anchor node in the "ANCHOR BASED"
 * approach for producer mobility
 */
class BaseStation : public App {
public:
  static TypeId
  GetTypeId();

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

  std::string &
  GetLocator() { return m_locator; }

  virtual void
  PreInitialize();

protected:

  virtual void
  StartApplication();
  
private:
  // XXX used for ????
  uint32_t m_signature;

  std::string m_locator;

};

} // namespace ndn
} // namespace ns3

#endif // ANCHOR

#endif
