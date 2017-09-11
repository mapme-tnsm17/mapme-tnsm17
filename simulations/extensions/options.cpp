#include <iostream>
#include "util.hpp"
#include "options.hpp"
#include "boost/lexical_cast.hpp"
#include <sys/stat.h>

#include "ns3/ndnSIM/NFD/daemon/fw/forwarder-mapme.hpp"

namespace ns3 {
namespace ndn {

Options::Options()
  : data_path("data")
  , duration(S(100))

  , capacity(MBITS_S(300))
  , buffer_size(400)
  , delay(MS(1))
  , cell_size(80) //in presence of fading (LATEST)
// , cell_size(50)
// , cell_size(100) // no presence of fading

  , rho(0.5)
  , rho_fraction_out(100)
  , rho_fraction_elastic(100)

  , num_permanent_streaming(0)
  , num_permanent_elastic(0)

  , num_cp_couples(1)
  , static_consumer(false)

  , chunk_size(BYTES(1024))

  , sigma(100)

  , streaming_rate(KBITS_S(100))
  , streaming_duration(S(30))

  , alpha(0.8)
  , catalog_size(1000)
  , cs_fraction(1)

  , mobility_scheme("gr")
  , mobility_model("rwp")
  , topology("fat-tree")
  , mobility_trace_file("../synthetic/trace_based_mobility/car_mobility")
  , aps_trace_file("../synthetic/trace_based_mobility/internetPoAPositionWiFi")
  , min_producer_time(240)
  //for wireless setting
  , radio_model("rfm") //Releigh Fading Model 
// DEPRECATED|#ifdef RANDOM_WAYPOINT
  , speed(50)
  , pause(0)
// DEPRECATED|#else
// DEPRECATED|  , mean_sleep(1)
// DEPRECATED|#endif
#ifdef MAPME
  , tu(MAPME_DEFAULT_TU)
#endif // MAPME

  , rho_fraction_in(0)
  , rho_fraction_streaming(0)
//  , mobility_scheme_id(0)
  , cs_size(0)

  , output_only(false)
  , quiet(false)
  , wired(false)
  , wldr(false)
  , mldr(false)
  , ilf("500ms")
//  , NumberOfConsumers(4)
{ }

void
Options::ParseCmdline(int argc, char* argv[])
{
    /* ConfigStore and CommandLine parsing */
    CommandLine cmd;

    cmd.AddValue ("data_path", "Data path", data_path);
    cmd.AddValue ("duration", "Simulation duration", duration);

    cmd.AddValue ("capacity", "Link capacities", capacity);
    cmd.AddValue ("buffer_size", "Buffer size", buffer_size);
    cmd.AddValue ("delay", "Link delay", delay);

    cmd.AddValue ("cell_size", "Cell size (You need to adjust the #defined value in extensions/constant-velocity-jump-helper.cpp also)", cell_size);

    cmd.AddValue ("rho", "Producer load", rho);
    cmd.AddValue ("rho_fraction_out", "Fraction of customers outside the AS", rho_fraction_out);
    cmd.AddValue ("rho_fraction_elastic", "Fraction of elastic load", rho_fraction_elastic);

    cmd.AddValue ("num_permanent_streaming", "Number of permanent streaming flows", num_permanent_streaming);
    cmd.AddValue ("num_permanent_elastic", "Number of permanent elastic flows", num_permanent_elastic);

    cmd.AddValue ("num_cp_couples", "Number of consumer/producer couples", num_cp_couples);

    cmd.AddValue ("catalog_size", "Catalog size", catalog_size);
    cmd.AddValue ("cs_fraction", "Content store fraction", cs_fraction);
    cmd.AddValue ("alpha", "Generalized Zipf exponent", alpha);

    cmd.AddValue ("sigma", "Elastic Content size (in chunks)" , sigma);

    cmd.AddValue ("streaming_rate", "CBR streaming rate", streaming_rate);
    cmd.AddValue ("streaming_duration", "CBR mean duration time", streaming_duration);

// DEPRECATED|#ifdef RANDOM_WAYPOINT
    cmd.AddValue ("speed", "Speed (m/s)", speed);
    cmd.AddValue ("pause", "Pause time", pause);
// DEPRECATED|#else
// DEPRECATED|    cmd.AddValue ("mean_sleep", "Mean sleep time", mean_sleep);
// DEPRECATED|#endif

#ifdef MAPME
    cmd.AddValue ("tu", "Tu", tu);
#endif // MAPME

    cmd.AddValue ("output-only", "Only displays result path", output_only);
    cmd.AddValue ("mobility_scheme", "Mobility scheme", mobility_scheme);
    cmd.AddValue ("mobility_model", "Mobility model", mobility_model);
    cmd.AddValue ("mobility_trace_file", "Mobility trace file (only for trace based simulation)", mobility_trace_file);
    cmd.AddValue ("aps_trace_file", "APs location file (only for trace based simulation)", aps_trace_file);
    cmd.AddValue ("min_producer_time", "Mobility trace file (only for trace based simulation)", min_producer_time);
    cmd.AddValue ("quiet", "Quiet", quiet);
//    cmd.AddValue ("wired", "Wired", wired); //this is binded with radio model of pwc(perfect wireless channel)
    cmd.AddValue ("staticConsumer", "True if consumer are static", static_consumer);
    cmd.AddValue ("wldr", "Set to true to enable WLDR", wldr);
    cmd.AddValue ("mldr", "Set to true to enable MLDR", mldr);
    cmd.AddValue ("int_lifetime", "Interest lifetime", ilf);
    cmd.AddValue ("topology", "Topology", topology);
    //cmd.AddValue ("nConsumers", "number of mobile consumer nodes", NumberOfConsumers);
    //for radio model 
    cmd.AddValue ("radio_model", "radio model", radio_model);

    cmd.Parse(argc, argv);

    //ConfigStore inputConfig;
    //inputConfig.ConfigureDefaults ();
    //cmd.Parse (argc, argv);

    if (mobility_scheme == "gr") {
        mobility_scheme_id = MS_GR;
#ifdef MAPME
    } else if (mobility_scheme == "mapme") {
        mobility_scheme_id = MS_MAPME;
    } else if (mobility_scheme == "mapme-iu") {
        mobility_scheme_id = MS_MAPME_IU;
#endif // MAPME
#ifdef ANCHOR
    } else if (mobility_scheme == "anchor") {
        mobility_scheme_id = MS_ANCHOR;
#endif // ANCHOR
#ifdef KITE
    } else if (mobility_scheme == "kite") {
        mobility_scheme_id = MS_KITE;
#endif // KITE
    } else {
        std::cout << "Invalid mobility scheme" << std::endl;
        exit(EXIT_FAILURE);
    }


/* XXX BROKEN
    if(mobility_model !="rwp" && ////random way point
       mobility_model !="rw2" && //random walk 2D
       mobility_model !="rgm" &&//random gaussion markov
       mobility_model !="markov" &&
       mobility_model != "rd2" &&//random direction 2D
       mobility_model != "trace" //trace based mobility (eg. car, pedestrian)
       ) {
        std::cout<< mobility_model<< " Invalid mobility model" << std::endl;
        exit(EXIT_FAILURE);
    }
*/

    if(radio_model !="rfm" && //Releigh Fading Model+log distance
       radio_model !="slos" && //suburban with Line of sight
       radio_model !="unlos" && //urban without line of sight
       radio_model != "pwc" //Perfect Wireless Channel(=wired channel)
       ) {
        std::cout<<radio_model << " Invalid radio setting" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(radio_model=="pwc"){
        wired = true; //set option wired to true
    }

    // CS size proportional to the number of producers / catalogs !
    cs_size = catalog_size * num_cp_couples * sigma * cs_fraction / 100;
    /* Cache size needs to be at least 1 packet */
    if (cs_size == 0)
      cs_size = 1;
    rho_fraction_out /= 100;
    rho_fraction_in = 1 - rho_fraction_out;
    rho_fraction_elastic /= 100;
    rho_fraction_streaming = 1 - rho_fraction_elastic;

    // setting default parameters for PointToPoint links and channels
    std::string buffer_size_s = boost::lexical_cast<std::string>(buffer_size);
    delay_s = boost::lexical_cast<std::string>(delay) + "ms";
    capacity_s = boost::lexical_cast<std::string>(capacity/1e6) + "Mbps";
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue(buffer_size_s)); // XXX BUFFER SIZE

}

void Options::PrintSummary()
{
    if (quiet)
        return;
    /* Display simulation summary */
    std::cout << "Run          : " << RngSeedManager::GetRun() << std::endl;
    std::cout << "Capacity     : " << capacity_s   << std::endl;
    std::cout << "Rho          : " << rho          << std::endl;
    std::cout << "  - out      : " << rho_fraction_out * 100 << " %"        << std::endl;
    std::cout << "  - in       : " << rho_fraction_in * 100  << " %"        << std::endl;
    std::cout << "  - elastic  : " << rho_fraction_elastic * 100 << " %"    << std::endl;
    std::cout << "  - streaming: " << rho_fraction_streaming * 100 << " %" << std::endl;
    std::cout << "Catalog size : " << catalog_size << " contents"     << std::endl;
    std::cout << "CS fraction  : " << cs_fraction  << " % = " << cs_size << " chunks" << std::endl;
    std::cout << "Content size : " << sigma        << " chunks"       << std::endl;
    std::cout << "Chunk size   : " << chunk_size   << " bits"         << std::endl;
    std::cout << "Alpha        : " << alpha        << std::endl;
    std::cout << "Duration     : " << duration     << " s"            << std::endl;
// DEPRECATED|#ifdef RANDOM_WAYPOINT
    std::cout << "Mean speed   : " << speed   << " m/s"          << std::endl;
    std::cout << "Pause        : " << pause        << " s"            << std::endl;
// DEPRECATED|#else
// DEPRECATED|    std::cout << "Mean sleep   : " << mean_sleep   << " s"            << std::endl;
// DEPRECATED|#endif
    std::cout << "Buffer size  : " << buffer_size  << std::endl;
    std::cout << "Cell size    : " << cell_size    << std::endl;
    std::cout << "Mobility scheme : " << mobility_scheme << std::endl;
    std::cout << "Topology : " << topology << std::endl;
    std::cout << "Mobility pattern : " << mobility_model << std::endl;
    std::cout << "Radio model : " << radio_model << std::endl;
}

std::string Options::ManageResultPath(bool create)
{
    std::ostringstream result_path;
  
    std::string traceBasedInfo = "";
    if (mobility_model == "trace"){
      std::size_t found = mobility_trace_file.find_last_of("/");
      traceBasedInfo = "-TM" + mobility_trace_file.substr(found+1);  //+ "-AP"+aps_trace_file;
    }
  
    result_path << "./" << data_path << "/"
        << "synthetic"
        << "-r" << rho
        << "-f" << rho_fraction_elastic
        << "-F" << rho_fraction_out
        << "-S" << num_permanent_streaming
        << "-E" << num_permanent_elastic
        << "-N" << num_cp_couples
        << "-K" << catalog_size
        << "-C" << cs_fraction
        << "-s" << sigma
        << "-a" << alpha
        << "-d" << duration

// DEPRECATED|#ifdef RANDOM_WAYPOINT
        << "-e" << speed
        << "-p" << pause
// DEPRECATED|#else
// DEPRECATED|        << "-i" << mean_sleep
// DEPRECATED|#endif

        << "-b" << buffer_size
        << "-c" << cell_size

        << "-y" << delay
#ifdef MAPME
        << "-tu" << tu
#else // MAPME
        << "-tu" << 0
#endif // MAPME

//        << (wired ? "-wired" : "-wireless")
        << "-m" << mobility_scheme
        << "-T" << topology
        << "-M" << mobility_model
        << "-R" << radio_model
        << traceBasedInfo
        << "/";

    if (!quiet || output_only)
      std::cout << "OUTPUT " << result_path.str() << std::endl;
    if (output_only)
        exit(0);
    result_path << RngSeedManager::GetRun() << "/";
    if (!quiet || output_only)
      std::cout << "FULL PATH " << result_path.str() << std::endl;

    struct stat sb;
    if (stat(result_path.str().c_str(), &sb) == 0) {
        if (!S_ISDIR(sb.st_mode)) {
            printf("Not a directory");
            exit(1);
        }
    } else {
        if (create) {
          const int dir_err = _mkdir(result_path.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
          if (-1 == dir_err) {
              printf("Error creating directory!");
              exit(1);
          }
        }
    }

    return result_path.str();
}

} // namespace ndn
} // namespace ns3

