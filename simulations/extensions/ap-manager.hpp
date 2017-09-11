/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef AP_MANAGER_H
#define AP_MANAGER_H

#include <map>
#include "ns3/core-module.h"
//#include "ns3/network-module.h"
//#include "ns3/ndnSIM-module.h"

namespace ns3 {
namespace ndn {

/**
 * \ingroup connectivity
 * \brief manages the network connectivity of a node.
 *
 */
class APManager : public Object
{
public:
  static TypeId GetTypeId (void);
  APManager ();

};

} // namespace ndn
} // namespace ns3

#endif /* AP_MANAGER_H */
