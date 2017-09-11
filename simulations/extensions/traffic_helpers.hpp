#ifndef TRAFFIC_HELPERS_HPP
#define TRAFFIC_HELPERS_HPP

#ifndef RAAQM
#error "This module requires RAAQM extensions"
#endif

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM/apps/ndn-producer.hpp"

#include "options.hpp"
#include "ndn-flow-stats.hpp"

// Flow types
#define FT_INTERNAL  1 << 0
#define FT_EXTERNAL  1 << 1
#define FT_ELASTIC   1 << 2
#define FT_STREAMING 1 << 3
#define FT_FINITE    1 << 4
#define FT_PERMANENT 1 << 5

namespace ns3 {
namespace ndn {

Ptr<Producer>
setupProducer(Ptr<Node> node, const std::string & prefix, Options & options);

// XXX Should not be in this file
#ifdef ANCHOR
/** @brief to set up anchor support at desired anchor location
 *  currently we are forced to set the prefix argument to "/" to get things work.
 */
void
setupAnchor(Ptr<Node> node,const std::string & prefix, Options &options, std::string result_path);

/** @brief set a BS manager
 */
void
setupBS(Ptr<Node> BS, const std::string & BSname, std::string prefixServedByAnchor, Options &options);
#endif // ANCHOR

void
setupDynamicConsumer(NodeContainer & node, const std::string & prefix, Options &options, 
    std::string result_path, int flags, std::string suffix = "");

void
setupPermanentConsumer(NodeContainer & node, const std::string & prefix, Options &options,
    std::string result_path, int flags, std::string suffix = "");

void
setUpPeriscopeConsumer(NodeContainer & node, const std::string & prefix, std::string result_path);

void
setUpCBRConsumer(NodeContainer & node, const std::string & prefix, std::string result_path);


} // namespace ndn
} // namespace ns3

#endif // TRAFFIC_HELPERS_HPP
