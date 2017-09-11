#include "traffic_helpers.hpp"

#include <limits>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"

#include "boost/lexical_cast.hpp"

#include "util.hpp"
#include "ndn-flow-stats.hpp"
#include "ndn-dynamic-arrivals.hpp"
#include "ndn-consumer-streaming-cbr.hpp"

// XXX To be removed !
#ifdef ANCHOR
#include "ndn-anchor.hpp"
#include "ndn-base-station.hpp"
#endif // ANCHOR

namespace ns3 {
namespace ndn {

/*-----------------------------------------------------------------------------
 * Helpers
 *
 * XXX we should not have code related to mobility management (anchor) here
 *----------------------------------------------------------------------------*/

void
onFlowStats(Ptr<OutputStreamWrapper> stream, FlowStats & fs)
{
  *stream->GetStream () << fs.toString() << std::endl;
}

Ptr<Producer>
setupProducer(Ptr<Node> node, const std::string & prefix, Options & options)
{
    std::string chunk_size_s = boost::lexical_cast<std::string>(BITS_TO_BYTES(options.chunk_size));

    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    producerHelper.SetPrefix(prefix);
    producerHelper.SetAttribute("PayloadSize", StringValue(chunk_size_s));
    producerHelper.Install(node);

    return node->GetApplication(0)->GetObject<Producer>();
}

// DEPRECATED|#ifdef ANCHOR
// DEPRECATED|void
// DEPRECATED|setupAnchor(Ptr<Node> node,const std::string & prefix, Options &options, std::string result_path)
// DEPRECATED|{
// DEPRECATED|  ndn::AppHelper anchorHelper("ns3::ndn::Anchor");
// DEPRECATED|  anchorHelper.SetPrefix(prefix);
// DEPRECATED|  anchorHelper.Install(node);
// DEPRECATED|
// DEPRECATED|  if (!result_path.empty()) {
// DEPRECATED|    Ptr<Application> app;
// DEPRECATED|    AsciiTraceHelper asciiTraceHelper;
// DEPRECATED|    for (unsigned int i = 0; i < node->GetNApplications(); i++) {
// DEPRECATED|      app = node->GetApplication(i);
// DEPRECATED|      if (app->GetInstanceTypeId ().GetName() == "ns3::ndn::Anchor")
// DEPRECATED|        break;
// DEPRECATED|    }
// DEPRECATED|    Ptr<Anchor> anchor = DynamicCast<Anchor>(app);
// DEPRECATED|    if (!anchor)
// DEPRECATED|        std::cout << "not anchor" << std::endl;
// DEPRECATED|
// DEPRECATED|    std::ostringstream fo_fn, po_fn;
// DEPRECATED|    fo_fn << result_path << "forwarderOverhead.txt";
// DEPRECATED|    Ptr<OutputStreamWrapper> fo_stream = asciiTraceHelper.CreateFileStream (fo_fn.str());
// DEPRECATED|    anchor->TraceConnectWithoutContext("ForwarderSignalingOverhead", MakeBoundCallback(&onForwarderSignalingOverhead, fo_stream));
// DEPRECATED|
// DEPRECATED|    po_fn << result_path << "producerOverhead.txt";
// DEPRECATED|    Ptr<OutputStreamWrapper> po_stream = asciiTraceHelper.CreateFileStream (po_fn.str());
// DEPRECATED|    anchor->TraceConnectWithoutContext("ProducerSignalingOverhead", MakeBoundCallback(&onProducerSignalingOverhead, po_stream));
// DEPRECATED|  }
// DEPRECATED|
// DEPRECATED|}
// DEPRECATED|#endif // ANCHOR

/*-----------------------------------------------------------------------------
 * Common applications (RAAQM, CBR), traffic patterns (Poisson) 
 * & their parameters
 *----------------------------------------------------------------------------*/

// Flow statistics collection with DynamicArrivals
#define FLOW_STATS(app, flags, id) do {                          \
  if (!result_path.empty()) {                                   \
    std::ostringstream fs_fn;                                   \
    AsciiTraceHelper asciiTraceHelper;                          \
    Ptr<OutputStreamWrapper> stream;                            \
                                                                \
    fs_fn << result_path << "flow_stats_" << ((flags & FT_ELASTIC) ? "elastic" : "streaming"); \
    if (!suffix.empty())                                        \
        fs_fn << "-" << suffix;                                 \
    fs_fn << id;                                                \
    stream = asciiTraceHelper.CreateFileStream (fs_fn.str());   \
    *stream->GetStream () << FlowStatsStreaming::getHeader() << \
        std::endl;                                              \
    app->TraceConnectWithoutContext("FlowStats", MakeBoundCallback(&onFlowStats, stream)); \
  }                                                             \
} while(0)

#ifdef RAAQM
AppHelper
setupConsumer(Options &options, int flags, std::string prefix = "")
{
  assert ((flags & FT_PERMANENT) ^ (flags & FT_FINITE));

  double max_seq;
  if (flags & FT_PERMANENT) {
    max_seq = std::numeric_limits<int>::max();
  } else {
    max_seq = (flags & FT_ELASTIC) ? options.sigma : options.streaming_rate * options.streaming_duration / options.chunk_size;
  }

  std::string catalog_size_s = boost::lexical_cast<std::string>(options.catalog_size);
  std::string alpha_s = boost::lexical_cast<std::string>(options.alpha);

  std::string cls = (flags & FT_ELASTIC) ? "ns3::ndn::ConsumerWindowRAAQM" : "ns3::ndn::ConsumerStreamingCbr";
  ndn::AppHelper consumerHelper(cls);
  if (!prefix.empty()) {
    consumerHelper.SetPrefix(prefix);
  }
  consumerHelper.SetAttribute("MaxSeq",            IntegerValue(max_seq)); 
  if (flags & FT_ELASTIC) {
    consumerHelper.SetAttribute("EnableAbortMode",   BooleanValue(false));  // Continue despite losses
    consumerHelper.SetAttribute("EnablePathReport",  BooleanValue(true));   // Needed to allow trace output
    consumerHelper.SetAttribute("DropFactor",        StringValue("0.01"));  // 0.01 0.005 seems the best
    consumerHelper.SetAttribute("Beta",              StringValue("0.9"));   // 0.9 avoids too many degradation
    consumerHelper.SetAttribute("Gamma",             IntegerValue(1));      // Default is the best
    consumerHelper.SetAttribute("initialWindowSize", IntegerValue(30));     // Default was 1
/*
    consumerHelper.SetAttribute("RetrxLimit",        StringValue("4"));
    consumerHelper.SetAttribute("Pmin",              StringValue("0.00001"));
*/
  } else {
    consumerHelper.SetAttribute("Frequency", DoubleValue(options.streaming_rate / options.chunk_size)); // interest/s
  }
  return consumerHelper;
}
#endif // RAAQM

AppHelper
setupDynamicArrivals(NodeContainer & node, AppHelper & consumerHelper, Options &options, int flags, std::string prefix, std::string result_path, std::string suffix)
{
  assert((flags & FT_STREAMING) ^ (flags & FT_ELASTIC));

  AppHelper app_dyn("ns3::ndn::DynamicArrivals");
  
    // Content arrival rate (s^-1)
  double lambda = options.rho * options.capacity;
  if (flags & FT_ELASTIC)
    lambda /= (options.sigma * options.chunk_size);
  else
    lambda /= (options.streaming_duration * options.streaming_rate);

  if (flags & FT_INTERNAL)
    lambda *= options.rho_fraction_in * options.num_cp_couples;
  else
    lambda *= options.rho_fraction_out;

  if (flags & FT_ELASTIC) 
    lambda *= options.rho_fraction_elastic;
  else
    lambda *= options.rho_fraction_streaming;
  
  if (lambda <= 0)
    return app_dyn;

  std::cout << "Setting up dynamic " << ((flags & FT_ELASTIC) ? "elastic" : "streaming") << " flows - rho=" << options.rho << " lambda=" << lambda << std::endl;

  std::string lambda_s = boost::lexical_cast<std::string>(lambda);
  std::string alpha_s  = boost::lexical_cast<std::string>(options.alpha);
  std::string catalog_size_s  = boost::lexical_cast<std::string>(options.catalog_size);

  app_dyn.SetPrefix(prefix);
  app_dyn.SetAttribute("ArrivalRate", StringValue(lambda_s));
  app_dyn.SetAttribute("CatalogSize", StringValue(catalog_size_s)); // XXX not taken into account
  app_dyn.SetAttribute("s",           StringValue(alpha_s));

  // XXX To simplify (use object aggregation ?) -- JA
  ApplicationContainer appc = app_dyn.Install(node);
  for (uint32_t i = 0; i < appc.GetN(); i++) {
    DynamicArrivals *app = dynamic_cast<DynamicArrivals*>(appc.Get(i).operator->());
    app->SetDownloadApplication(consumerHelper);
    app->SetPopularityType("zipf");

    // Collect statistics in separate files
    FLOW_STATS(app, flags, i);
  }

  return app_dyn;
}

/*-----------------------------------------------------------------------------
 * Workload definition
 *----------------------------------------------------------------------------*/

void
setupDynamicConsumer(NodeContainer & node, const std::string & prefix, Options &options, 
    std::string result_path, int flags, std::string suffix)
{
  assert((flags & FT_INTERNAL) ^ (flags & FT_EXTERNAL));
  assert((flags & FT_STREAMING) ^ (flags & FT_ELASTIC));

  // NOTE : the helper might be created for nothing if rho / fraction / etc. // = 0
  AppHelper consumerHelper = setupConsumer(options, flags | FT_FINITE);
  AppHelper app_dyn = setupDynamicArrivals(node, consumerHelper, options, flags, prefix, result_path, suffix);

}

  // XXX How to distinguish many apps on the same node // esp. for statistics
void
setupPermanentConsumer(NodeContainer & node, const std::string & prefix, Options &options,
    std::string result_path, int flags, std::string suffix)
{
  assert(node.GetN() == 1);

  uint32_t num = (flags & FT_ELASTIC) ? options.num_permanent_elastic : options.num_permanent_streaming;
  if (num <= 0) {
    std::cout << "NUM <= 0" << std::endl;
    return;
  }

  assert(flags & FT_STREAMING); // XXX BUG BELOW

  std::string type = (flags & FT_ELASTIC) ? "elastic" : "streaming";
  std::cout << "Setting up " << num << " permanent " << type << " flow(s)" << std::endl;

  for (uint32_t id = 0; id < num; id++) {
    AppHelper consumerHelper = setupConsumer(options, flags | FT_PERMANENT, prefix);
    ApplicationContainer myApps = consumerHelper.Install(node);

    for (uint32_t i = 0; i < myApps.GetN(); i++) {
      // XXX ISSUE HERE WITH ELASTIC TRAFFIC
      // GetObject would be better
      ConsumerStreamingCbr *myApp = dynamic_cast<ConsumerStreamingCbr*>(myApps.Get(i).operator->());

      // Flow statistics
      if (!result_path.empty()) {
        std::ostringstream fs_fn;
        std::string hdr = (flags & FT_ELASTIC) ? FlowStatsElastic::getHeader() : FlowStatsStreaming::getHeader();

        fs_fn << result_path << "flow_stats_" << type << "_handover-" << suffix << "-" << myApps.Get(i)->GetNode ()->GetId();
        AsciiTraceHelper asciiTraceHelper;
        Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fs_fn.str());
        *stream->GetStream () << hdr << std::endl;
        myApp->TraceConnectWithoutContext("HandoverFlowStats", MakeBoundCallback(&onFlowStats, stream));
      }
    }
  }
}

void
setUpPeriscopeConsumer(NodeContainer & node, const std::string & prefix, std::string result_path)
{
  ndn::AppHelper consumerHelper = ndn::AppHelper("ns3::ndn::ConsumerStreaming");
  consumerHelper.SetAttribute("MaxSeq", UintegerValue(1000000));
  consumerHelper.SetAttribute("MaxRTO", StringValue("0.5s"));
  consumerHelper.SetAttribute("LifeTime", StringValue("0.5s"));
  consumerHelper.SetAttribute("SecondsToBuffer", StringValue("5s"));
  consumerHelper.SetAttribute("PlayBitRate", StringValue("1000000"));
  consumerHelper.SetPrefix(prefix);

  consumerHelper.Install(node);
  //todogg add tracing for failures stats

}
  
void
setUpCBRConsumer(NodeContainer & node, const std::string & prefix, std::string result_path)
{
  ndn::AppHelper consumerHelper = ndn::AppHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("PathForTrace", StringValue(result_path));
  consumerHelper.SetAttribute("Frequency", DoubleValue(120)); //120 int per sec -> 1Mbit/sec of data
  consumerHelper.SetPrefix(prefix);
  consumerHelper.Install(node);
  
}

} // namespace ndn
} // namespace ns3


/*-----------------------------------------------------------------------------
 * Deprecated code (to delete as soon as all function integrated in new code)
 *----------------------------------------------------------------------------*/

// DEPRECATED|// kept for statistics code until it is integrated into
// DEPRECATED|// setupPermanentElasticConsumer
// DEPRECATED|void
// DEPRECATED|setupOneDownloadConsumer(NodeContainer & node, const std::string & prefix, Options &options, std::string result_path, double fraction, std::string suffix)
// DEPRECATED|{
// DEPRECATED|    if (fraction <= 0)
// DEPRECATED|      return;
// DEPRECATED|
// DEPRECATED|    AppHelper consumerHelper = setupRAAQMConsumer(node, options, std::numeric_limits<int>::max()), fraction);
// DEPRECATED|
// DEPRECATED|    std::string lambda_s = boost::lexical_cast<std::string>(options.lambda * fraction / node.GetN()); // num_catalogs ?
// DEPRECATED|    std::cout << "lambda_s " << lambda_s << std::endl;
// DEPRECATED|    std::string catalog_size_s = boost::lexical_cast<std::string>(options.catalog_size);
// DEPRECATED|    std::string alpha_s = boost::lexical_cast<std::string>(options.alpha);
// DEPRECATED|    std::string sigma_s = boost::lexical_cast<std::string>(options.sigma);
// DEPRECATED|
// DEPRECATED|    //*********set flow length to inifinit*********************************
// DEPRECATED|    consumerHelper.SetAttribute("MaxSeq", IntegerValue(std::numeric_limits<int>::max())); 
// DEPRECATED|    consumerHelper.SetAttribute("EnableAbortMode",BooleanValue(false)); // Continue despite losses
// DEPRECATED|    consumerHelper.SetAttribute("EnablePathReport", BooleanValue(true)); // Needed to allow trace output
// DEPRECATED|//    consumerHelper.SetAttribute("RetrxLimit", StringValue("4"));
// DEPRECATED|    consumerHelper.SetAttribute("DropFactor", StringValue("0.01")); // 0.01 0.005 seems the best
// DEPRECATED|    consumerHelper.SetAttribute("Beta", StringValue("0.9")); // 0.9 avoids too many degradation
// DEPRECATED|    consumerHelper.SetAttribute("Gamma", IntegerValue(1)); // Default is the best
// DEPRECATED|//    consumerHelper.SetAttribute("Pmin", StringValue("0.00001"));
// DEPRECATED|    
// DEPRECATED|    //------------to modify the initial window size, change the following line--------------------------------
// DEPRECATED|    consumerHelper.SetAttribute("initialWindowSize",IntegerValue(30));
// DEPRECATED|
// DEPRECATED|    // XXX XXX XXX XXX XXX XXX XXX
// DEPRECATED|    // Why do we need dynamic arrivals when we have one single flow !! --JA
// DEPRECATED|    // XXX XXX XXX XXX XXX XXX XXX
// DEPRECATED|
// DEPRECATED|    ndn::AppHelper elasticTrafficConsumer("ns3::ndn::ConsumerElasticTraffic");
// DEPRECATED|    elasticTrafficConsumer.SetPrefix(prefix); // XXX
// DEPRECATED|    elasticTrafficConsumer.SetAttribute("ArrivalRate", StringValue(lambda_s)); // 100 i/s
// DEPRECATED|    elasticTrafficConsumer.SetAttribute("CatalogSize", StringValue(catalog_size_s));
// DEPRECATED|    elasticTrafficConsumer.SetAttribute("s", StringValue(alpha_s));
// DEPRECATED|    
// DEPRECATED|    //***************************here set to use one single flow*************************
// DEPRECATED|    elasticTrafficConsumer.SetAttribute("useOneSingleFlow",BooleanValue(true));
// DEPRECATED|
// DEPRECATED|    // Statistics (XXX factor code)
// DEPRECATED|    if (!result_path.empty()) {
// DEPRECATED|        std::ostringstream cwnd_fn, fs_fn, ps_fn, gp_fn;
// DEPRECATED|
// DEPRECATED|        /* Window statistics */
// DEPRECATED|        cwnd_fn << result_path << "cwnd";
// DEPRECATED|        if (!suffix.empty())
// DEPRECATED|            cwnd_fn << "-" << suffix;
// DEPRECATED|        cwnd_fn << ".out";
// DEPRECATED|        elasticTrafficConsumer.SetAttribute("WindowTraceFile", StringValue(cwnd_fn.str()));
// DEPRECATED|
// DEPRECATED|        /* Flow statistics */
// DEPRECATED|        fs_fn << result_path << "flow_stats_elastic";
// DEPRECATED|        if (!suffix.empty())
// DEPRECATED|            fs_fn << "-" << suffix;
// DEPRECATED|        elasticTrafficConsumer.SetAttribute("FlowStatsTraceFile", StringValue(fs_fn.str()));
// DEPRECATED|
// DEPRECATED|//         /* Path statistics */
// DEPRECATED|//         ps_fn << result_path << "path_stats";
// DEPRECATED|//         if (!suffix.empty())
// DEPRECATED|//             ps_fn << "-" << suffix;
// DEPRECATED|//         elasticTrafficConsumer.SetAttribute("PathStatsTraceFile", StringValue(ps_fn.str()));
// DEPRECATED|  
// DEPRECATED|  gp_fn <<result_path<<"goodput_handover";
// DEPRECATED|  consumerHelper.SetAttribute("GoodputHandoverTracefile", StringValue(gp_fn.str()));
// DEPRECATED|    }
// DEPRECATED|    ApplicationContainer myApps = elasticTrafficConsumer.Install(node);
// DEPRECATED|
// DEPRECATED|    // XXX I think that can be simplified with GetObject
// DEPRECATED|    for (uint32_t i = 0; i < myApps.GetN(); i++) {
// DEPRECATED|        ndn::ConsumerElasticTraffic *myApp=dynamic_cast<ndn::ConsumerElasticTraffic*>(myApps.Get(i).operator->());
// DEPRECATED|        myApp->SetDownloadApplication(consumerHelper);
// DEPRECATED|        myApp->SetPopularityType("zipf");
// DEPRECATED|    }
// DEPRECATED|    
// DEPRECATED|    myApps.Start(Time("0.1s"));
// DEPRECATED|}

/*
  // Customer requesting the disappearing prefix
  ndn::AppHelper h("ns3::ndn::ConsumerZipf");
 std::string alpha_s = boost::lexical_cast<std::string>(options.alpha);

 h.SetPrefix(prefix);
 h.SetAttribute("Frequency", StringValue(lambda_s)); // 100 i/s
 h.SetAttribute("Randomize", StringValue("exponential"));
 h.SetAttribute("NumberOfContents", StringValue(catalog_size_s));
 h.SetAttribute("s", StringValue(alpha_s));
 h.Install(node);
*/

