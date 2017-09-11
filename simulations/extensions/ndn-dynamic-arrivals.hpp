#ifndef NDN_DYNAMIC_ARRIVALS_HPP
#define NDN_DYNAMIC_ARRIVALS_HPP

#ifdef RAAQM

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ndn-flow-stats.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include <boost/unordered_map.hpp>
#include "ns3/application.h"
#include "ns3/ndnSIM-module.h"

#include <queue>

#define MAX_FREE_APP_SIZE2 15000000
/**
 * 
 *  \brief  traffic modeling:
 *   usage example (put in scenario.cpp the following code):
      
   #include "ns3/ndnSIM/apps/ndn-consumer-elastic-traffic.hpp"
    ....
   //first, configure the downloader to be used together, by default it is the raaqm downloader with default parameter
   ndn::AppHelper consumerHelper("ns3::ndn::ConsumerWindowRAAQM");
   consumerHelper.SetAttribute("MaxSeq", IntegerValue(100)); 
   consumerHelper.SetAttribute("EnablePathReport", BooleanValue(true));
   //consumerHelper.SetAttribute("EnableAbortMode",BooleanValue(true)); 
   consumerHelper.SetAttribute("NewRetxTimer", StringValue("100ms"));
 
   //second, configure the traffic modelling application:
   ndn::AppHelper elasticTrafficHelper("ns3::ndn::DynamicArrivals");
   elasticTrafficHelper.SetPrefix(prefix);

   //third, install the traffic modelling application on the node:
    ApplicationContainer myApps=elasticTrafficHelper.Install(consumerNodes);

   //finally, add the configuration of the donwloader to the traffic modelling application, by default it is the raaqm downloader with default parameter
   ndn::DynamicArrivals *myApp=dynamic_cast<ndn::DynamicArrivals*>(myApps.Get(0).operator->());
   myApp->SetDownloadApplication(consumerHelper);
   ...
*/


namespace ns3 {
namespace ndn {

//***************** arrival and popularity class**************************

/** \brief arrival and popularity process class
 */
class DynamicArrivalProcess{
public:
	DynamicArrivalProcess();
	
	double
	GetTime();

	void
	SetRate(double rate);

	void
	SetRandomize(const std::string& value);

	std::string
	GetRandomize() const;

private:
        double m_lambda; 
	bool m_firstTime;
	Ptr<RandomVariableStream> m_random;
	std::string m_randomType;

	           // Frequency in file downloads/sec

};

class DynamicPopularityProcess{
public:
	DynamicPopularityProcess();
	
	uint32_t
	GetRank();
	
	void
	SetCatalogSize(uint32_t size);

	void
	SetRandomize(const std::string& value);

	std::string
	GetRandomize() const;
	
	void
	SetQ(double q);
	
	void
	SetS(double s);

private:
	
	uint32_t m_catalog_size;
	double m_param_1;
	double m_param_2;
	double m_param_3;
	Ptr<RandomVariableStream> m_random;
	std::string m_randomType;
};  
  

class ConsumerWindowRAAQM;


//********************* DynamicArrivals class ****************************
/**
 * \brief application to simulate the traffic model
 */
class DynamicArrivals: public Application  {
public:
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
         static TypeId GetTypeId (void);
        //constructor:
        DynamicArrivals();
	DynamicArrivals(AppHelper downloadAppHelper);
	
	virtual ~DynamicArrivals();

// 	virtual void
// 	SendPacket();

	void
	SetRate(double rate);

	void
	SetCatalogSize(uint32_t size);

	void
	SetArrivalType(const std::string& value_arrivals);
	
	void
	SetPopularityType(const std::string& value_popularity);
					 
	void
	SetDynamicArrivalProcess(const DynamicArrivalProcess& newDynamicArrivalProcess);
	
	const DynamicArrivalProcess&
	GetMyDynamicArrivalProcess() const;
	
	void
	SetDynamicPopularityProcess(DynamicPopularityProcess& newDynamicPopularityProcess);
	
	const DynamicPopularityProcess&
	GetDynamicPopularityProcess() const;
	
	void
	SetDownloadApplication(const AppHelper& downloadAppHelper);
	
	void
	SetQ(double q);
	
	void
	SetS(double s);
	
	void
	onDownloadStop(Ptr<Application> stoppedApp);
  
  void
  onFlowStatsTrace(FlowStats & fs);
  //onFlowStatsTrace(FlowStats & fs);
	
// 	/**
//    * schedule the Installations of many instances of downlaod applications
//    * on each node of the input container configured with all the attributes
//    * set with SetAttribute and the arrival process and popularity process
//    *
//    * \param c NodeContainer of the set of nodes on which an NdnConsumer
//    * will be installed.
//    * 
//    */
//   void
//   ScheduleAllDowndloads(NodeContainer c);
// 
//   /**
//    * schedule the Installations of many instances of downlaod applications
//    * on the node configured with all the attributes set with SetAttribute 
//    * and the arrival process and popularity process
//    *
//    * \param node The node on which an NdnConsumer will be installed.
//    */
//   void
//   ScheduleAllDowndloads(Ptr<Node> node);
// 
// /**
//    * schedule the Installations of many instances of downlaod applications
//    * on the node configured with all the attributes set with SetAttribute 
//    * and the arrival process and popularity process
//    *
//    * \param nodeName The node on which an NdnConsumer will be installed.
//    * \returns Container of Ptr to the applications installed.
//    */
//   void
//   ScheduleAllDowndloads(std::string nodeName);

protected:
	void
	LaunchNextDownload();
		
	// inherited from Application base class. Originally they were private, now they are public
	virtual void
	StartApplication(); ///< @brief Called at time specified by Start

	virtual void
	StopApplication(); ///< @brief Called at time specified by Stop
	
	AppHelper m_downloadAppHelper; ///< @brief the app helper that contains all the configurations for the download application
	
	bool
	openTraceFile();

	bool
	openFlowStatsTraceFile();
	
	void
	printHeader();

	void
	printFlowStatsHeader();
	
	void
	printWindowTrace(uint32_t appId, double curwindow, unsigned winpending);

	void
	printFlowStatsTrace(FlowStats & );
	
	ApplicationContainer m_downloadApps;///< @brief contains pointers to all the previous launched applications.
	EventId m_lauchEvent; ///< @brief EventId of "to lauch a new download application" event
	
	bool m_active; ///< @brief Flag to indicate that application is active (set by StartApplication)
	Name m_prefix;

	std::string m_windowTraceFile;
  std::string m_flowStatsTraceFile;
	shared_ptr<std::ostream> m_os;
	shared_ptr<std::ostream> m_fs_os;

private:
  DynamicArrivalProcess m_arrivals;
  DynamicPopularityProcess m_popularity;
  
  std::queue<Ptr<Application> > m_freeApps;
  unsigned int m_maxFreeAppSize;
  unsigned int m_nLauchedApps;

  unsigned int m_flowsInProgress;

  TracedCallback<FlowStats &> m_flowStatsTrace;
};





} /* namespace ndn */
} /* namespace ns3 */

#endif // RAAQM

#endif // NDN_DYNAMIC_ARRIVALS_HPP
