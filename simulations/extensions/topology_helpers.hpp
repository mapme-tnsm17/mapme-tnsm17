#ifndef TOPOLOGY_HELPERS_HPP
#define TOPOLOGY_HELPERS_HPP

#include "ns3/core-module.h"

#include "options.hpp"

namespace ns3 {
namespace ndn {

NetDeviceContainer
setupWired(NodeContainer & AP, NodeContainer & Clients, Options & options);

// for 802.11ac
NetDeviceContainer
setupVhtWifi(NodeContainer & AP, NodeContainer & Clients, Options & options);

NetDeviceContainer
setupWifi(NodeContainer & AP, NodeContainer & Clients, Options & options);

NetDeviceContainer
setupWifiInTheWild(NodeContainer & AP, NodeContainer & Clients);

NetDeviceContainer
setup80211nWifi(NodeContainer & AP, NodeContainer & Clients, Options & options);
  
int
setInternetPoAPosition(std::string file, NodeContainer &internetPoA);

void
addLink(Ptr<Node> n1, Ptr<Node> n2);




NetDeviceContainer
setupWifi_new_radio(NodeContainer & AP, NodeContainer & Clients, Options & options);
  
/**
 * \brief Setup LTE links
 */
void
setupLTE();

} // namespace ndn
} // namespace ns3

#endif // TOPOLOGY_HELPERS_HPP
