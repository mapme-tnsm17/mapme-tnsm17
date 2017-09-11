#ifndef OPTIONS_HPP
#define OPTIONS_HPP

// DEPRECATED|#define RANDOM_WAYPOINT 1

#include "ns3/core-module.h"

namespace ns3 {
namespace ndn {

typedef enum {
  MS_GR,
#ifdef MAPME
  MS_MAPME,
  MS_MAPME_IU,
#endif //MAPME
#ifdef ANCHOR
  MS_ANCHOR,
#endif // ANCHOR
#ifdef KITE
  MS_KITE,
#endif // KITE
} mobility_scheme_t;

class Options {

public:
    Options();
    void PrintSummary();
    std::string ManageResultPath(bool create = true);
    void ParseCmdline(int argc, char* argv[]);

public:
    /* Simulation */
    std::string data_path;
    uint32_t duration;

    /* Output statistics */
    //trace per handover true | false

    /* Network & topology*/
    double capacity;        /* Bottleneck capacity: close to producer */
    double buffer_size;     /* Link buffer size */
    double delay;           /* Link delay */
    double cell_size;     /* For ideal wifi only */

    /* Radio access : deserves a separate section... TODO */

    /* Workload (resp. Dynamic and Permanent flows) */
    double rho;             /* Link load with respect to bottlenecked producer link */
    double rho_fraction_out;
    double rho_fraction_elastic;

    uint32_t num_permanent_streaming;
    uint32_t num_permanent_elastic;

    uint32_t num_cp_couples;
    bool static_consumer; //if true, consumers are static and connected to random leafs (with wired link)
    //double num_consumers;   /* Number of customers */
    //double num_catalogs;    /* Number of catalogs */

    /* Application (resp. Elastic & Streaming) */
    double chunk_size;

    double sigma;           /* Mean content size (in chunks) */

    double streaming_rate;
    double streaming_duration;

    /* Content */
    double alpha;           /* Zipf parameter */
    double catalog_size;
    double cs_fraction;     /* Content store size (in chunks) */

    /* Mobility */
    std::string mobility_scheme; /* String version of mobility scheme */
    std::string mobility_model; /* String version of mobility model */
    std::string topology;
    std::string mobility_trace_file; /* Path where to find mobility traces (only when mobility_model is "trace")*/
    std::string aps_trace_file; /* Path where to find APs locations (used only when mobility_model is "trace")*/
    int min_producer_time; /* nodes that stay in the map less than this won't be chosen as producer (used only when mobility_model is "trace")*/
  
    std::string radio_model; /*for wireless channel model*/

// DEPRECATED|#ifdef RANDOM_WAYPOINT
    double speed;
    double pause;
// DEPRECATED|#else
// DEPRECATED|    double mean_sleep;      /* Sleep time for producer mobility */
// DEPRECATED|#endif
#ifdef MAPME
    uint64_t tu;
#endif // MAPME

    /* Computed options */
    double rho_fraction_in;
    double rho_fraction_streaming;
    mobility_scheme_t mobility_scheme_id;
    double lambda;
    double lambda_streaming;
    uint32_t cs_size;
    std::string capacity_s;
    std::string delay_s;

    /* Control parameters */
    bool output_only;
    bool quiet;
    bool wired;
    bool wldr;
    bool mldr;
    std::string ilf;
    //uint32_t NumberOfConsumers;

};

} // namespace ndn
} // namespace ns3

#endif // OPTIONS_HPP
