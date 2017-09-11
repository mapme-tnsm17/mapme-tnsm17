/*
 * ndn-consumer-elastic-traffic.cpp
 *
 *  Created on: Apr 13, 2015
 *      Author: muscariello
 *              zeng xuan
 */

#ifdef RAAQM

#include "ndn-dynamic-arrivals.hpp"
#include "ndn-flow-stats.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include <fstream>
#include <iomanip>

NS_LOG_COMPONENT_DEFINE("ndn.DynamicArrivals");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(DynamicArrivals);

TypeId
DynamicArrivals::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::DynamicArrivals")
      .SetGroupName("Ndn")
      .SetParent<Application>()
      .AddConstructor<DynamicArrivals>()
           
          .AddAttribute("Prefix", "common prefix used by all the downdloaders, to be appended by object rank number", StringValue("/"),
                    MakeNameAccessor(&DynamicArrivals::m_prefix),
			MakeNameChecker())

	  .AddAttribute("ArrivalRate", "File download rate [files/second]",
			     StringValue("1.0"), MakeDoubleAccessor(&DynamicArrivals::SetRate),
				 MakeDoubleChecker<double>())

	  .AddAttribute("CatalogSize", "Number of the objects in the catalog", StringValue("100"),
			        MakeUintegerAccessor(&DynamicArrivals::SetCatalogSize),
					MakeUintegerChecker<uint32_t>())

	  .AddAttribute("DynamicArrivalProcessType", "Arrivals: exponential, uniform",
				                    StringValue("exponential"),
				                    MakeStringAccessor(&DynamicArrivals::SetArrivalType),
				                    MakeStringChecker())
	  
	  .AddAttribute("DynamicPopularityProcessType", "Popularity: uinform, zipf, weibull",
				                    StringValue("zipf"),
				                    MakeStringAccessor(&DynamicArrivals::SetPopularityType),
				                    MakeStringChecker())

	   .AddAttribute("s", "parameter of power", 
			                            StringValue("1.0"),
                                                    MakeDoubleAccessor(&DynamicArrivals::SetS),
                                                    MakeDoubleChecker<double>())
	   .AddAttribute("MaxFreeAppSize", "the pool size of the apps, by default is 150", 
			                            IntegerValue(MAX_FREE_APP_SIZE2),
				                    MakeIntegerAccessor(&DynamicArrivals::m_maxFreeAppSize), 
				                    MakeIntegerChecker<int32_t>())

     .AddTraceSource("FlowStats", "Provide flow statistics of all download apps",
         MakeTraceSourceAccessor(&DynamicArrivals::m_flowStatsTrace));

       ;
  return tid;
}

DynamicArrivals::DynamicArrivals()
 /// XXX by default use the ConsumerWindowRAAQM as the download application and use the default configuration for that application as configuration
: m_downloadAppHelper("ns3::ndn::ConsumerStreamingCbr")
, m_active(false)
, m_windowTraceFile("")
, m_flowStatsTraceFile("")
, m_maxFreeAppSize(MAX_FREE_APP_SIZE2)
, m_nLauchedApps(0)
, m_flowsInProgress(0)
{
}

DynamicArrivals::DynamicArrivals(AppHelper downloadAppHelper)
: m_downloadAppHelper(downloadAppHelper)
, m_active(false)
, m_windowTraceFile("") 
, m_flowStatsTraceFile("")
, m_maxFreeAppSize(MAX_FREE_APP_SIZE2)
, m_nLauchedApps(0)
, m_flowsInProgress(0)
{
}


DynamicArrivals::~DynamicArrivals()
{
  Simulator::Remove(m_lauchEvent);
}

void
DynamicArrivals::onDownloadStop(Ptr<Application> stoppedApp)
{
  m_flowsInProgress--;
  //std::cout << "FLOWS IN PROGRESS: " << m_flowsInProgress << std::endl;
  m_freeApps.push(stoppedApp);
}


void
DynamicArrivals::SetArrivalType(const std::string& value_arrivals)
{
  m_arrivals.SetRandomize(value_arrivals);
}

void
DynamicArrivals::SetPopularityType(const std::string& value_popularity)
{
  	m_popularity.SetRandomize(value_popularity);

}
					 

void
DynamicArrivals::SetRate(double rate)
{
	m_arrivals.SetRate(rate);
}

void
DynamicArrivals::SetCatalogSize(uint32_t size)
{
	m_popularity.SetCatalogSize(size);
}

void
DynamicArrivals::SetQ(double q)
{
  m_popularity.SetQ(q);
}

void
DynamicArrivals::SetS(double s)
{
  m_popularity.SetS(s);
}

void
DynamicArrivals::StartApplication()
{
    NS_LOG_FUNCTION_NOARGS();

  NS_LOG_INFO("Starting Dynamic arrivals");

  NS_ASSERT(m_active != true);
  m_active = true;

  NS_ASSERT_MSG(GetNode()->GetObject<L3Protocol>() != 0,
                "Ndn stack should be installed on the node " << GetNode());

  m_lauchEvent=Simulator::Schedule(Seconds(m_arrivals.GetTime()),
 	    			 &DynamicArrivals::LaunchNextDownload, this);
}

void
DynamicArrivals::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  if (!m_active)
    return; // don't assert here, just return

  m_active = false;
  m_downloadApps.Stop(Time("0s"));
  Simulator::Remove(m_lauchEvent);
  

}

void
DynamicArrivals::onFlowStatsTrace(FlowStats & fs)
{
  m_flowStatsTrace(fs);
}

void
DynamicArrivals::LaunchNextDownload()
{
  if(!m_active) 
    return;
// 	shared_ptr<Download> newDownload = make_shared<Download>();
// 	m_downloads_table[++m_download_id] = newDownload;
// 
// 	shared_ptr<Name> nameWithRank = make_shared<Name>(m_interestName);
// 	nameWithRank->appendSequenceNumber(m_popularity.GetRank());
// 	newDownload->m_interestName.Name(nameWithRank);
// 
// 	if (!m_sendEvent.IsRunning())
// 	    m_sendEvent = Simulator::Schedule(Seconds(m_arrivals.GetTime()),
// 	    			 &DynamicArrivals::ScheduleNextDownload, this);
  
    
  //lauch a new download application: 
  uint32_t rank=m_popularity.GetRank();
  std::string nextPrefix=(m_prefix.toUri()+"/OBJNUM"+ std::to_string(rank));
  NS_LOG_DEBUG("install application for prefix " << nextPrefix );

  m_flowsInProgress++;
  //std::cout << "FLOWS IN PROGRESS: " << m_flowsInProgress << std::endl;
  
  ApplicationContainer newDownloadApp;
  
#if 0
  if(m_freeApps.size()!=0)//need and we can take from pool now
  {
    newDownloadApp.Add(m_freeApps.front());
    m_freeApps.pop();
    std::cout << "E: app not restarted" << std::endl;
//    DynamicCast<ConsumerWindowRAAQM>(newDownloadApp.Get(0))->ReStart(nextPrefix);
  }
  else 
#endif
  if(m_nLauchedApps<m_maxFreeAppSize)//need to create and lauch a new application container
  {
    //NS_LOG_DEBUG("opening a new face");
    m_downloadAppHelper.SetPrefix(nextPrefix); ///< @brief set the prefix the download according to popularity process
 
    Ptr<Node> m_node=GetNode();
    newDownloadApp=m_downloadAppHelper.Install(m_node);
    // XXX RAAQM
    // set stop callback is passed an instance of us
    // calls onDownloadStop, used to free apps, so very important !!!!
    m_nLauchedApps++;
    m_downloadApps.Add(newDownloadApp);
    m_downloadApps.Stop(m_stopTime-m_startTime);
    
    if (newDownloadApp.GetN()!=0) {
      // XXX better but has typing issues
      //newDownloadApp.Get(0)->SetStopCallback(MakeCallback(&DynamicArrivals::onDownloadStop, this));
      newDownloadApp.Get(0)->TraceConnectWithoutContext("StopCallback", MakeCallback(&DynamicArrivals::onDownloadStop, this));
      newDownloadApp.Get(0)->TraceConnectWithoutContext("FlowStats", MakeCallback(&DynamicArrivals::onFlowStatsTrace, this));
    }
    //NS_LOG_DEBUG("No. of launched app is "<<m_nLauchedApps);
  } else {
    NS_LOG_DEBUG("the pool of free app containers is used up");
    std::cout << "the pool of free app containers is used up" << std::endl;
  }

 
  
  NS_LOG_DEBUG("No. of free app is "<<m_freeApps.size()<<", No. of launched app is "<<m_nLauchedApps);
 
  
//NS_LOG_DEBUG("***********current number of applications on the node is" << m_node->GetNApplications());
  //schedule the next:
  if(m_lauchEvent.IsRunning())
  {
        Simulator::Remove(m_lauchEvent);

	NS_LOG_DEBUG("last launch event is still running");
  }
  
  m_lauchEvent=Simulator::Schedule(Seconds(m_arrivals.GetTime()),
 	    			 &DynamicArrivals::LaunchNextDownload, this);
}

void
DynamicArrivals::SetDynamicArrivalProcess(const DynamicArrivalProcess& newDynamicArrivalProcess)
{
  m_arrivals=newDynamicArrivalProcess;
}

const DynamicArrivalProcess&
DynamicArrivals::GetMyDynamicArrivalProcess() const
{
  return m_arrivals;
}

void
DynamicArrivals::SetDynamicPopularityProcess(DynamicPopularityProcess& newDynamicPopularityProcess)
{
  m_popularity=newDynamicPopularityProcess;
}

const DynamicPopularityProcess&
DynamicArrivals::GetDynamicPopularityProcess() const
{
  return m_popularity;
}

void
DynamicArrivals::SetDownloadApplication(const AppHelper& downloadAppHelper)
{
  m_downloadAppHelper=downloadAppHelper;
}


//DEPRECATED bool
//DEPRECATED DynamicArrivals::openTraceFile()
//DEPRECATED {
//DEPRECATED     using namespace boost;
//DEPRECATED     using namespace std;
//DEPRECATED 
//DEPRECATED   shared_ptr<std::ostream> outputStream;
//DEPRECATED   if (m_windowTraceFile != "") {
//DEPRECATED     shared_ptr<std::ofstream> os(new std::ofstream());
//DEPRECATED     os->open(m_windowTraceFile.c_str(), std::ios_base::out | std::ios_base::trunc);
//DEPRECATED 
//DEPRECATED     if (!os->is_open()) {
//DEPRECATED       NS_LOG_ERROR("File " << m_windowTraceFile << " cannot be opened for writing. Tracing disabled");
//DEPRECATED       return false;
//DEPRECATED     }
//DEPRECATED 
//DEPRECATED     m_os = os;
//DEPRECATED     return true;
//DEPRECATED   }
//DEPRECATED   else {
//DEPRECATED     return false;
//DEPRECATED   }
//DEPRECATED }
//DEPRECATED 
//DEPRECATED bool DynamicArrivals::openFlowStatsTraceFile()
//DEPRECATED {
//DEPRECATED   using namespace boost;
//DEPRECATED   using namespace std;
//DEPRECATED 
//DEPRECATED 
//DEPRECATED   shared_ptr<std::ostream> outputStream;
//DEPRECATED   if (m_flowStatsTraceFile != "") {
//DEPRECATED     std::ostringstream fs_fn;
//DEPRECATED     fs_fn << m_flowStatsTraceFile << "-" << this->GetNode()->GetId();
//DEPRECATED     shared_ptr<std::ofstream> os(new std::ofstream());
//DEPRECATED     os->open(fs_fn.str().c_str(), std::ios_base::out | std::ios_base::trunc);
//DEPRECATED 
//DEPRECATED     if (!os->is_open()) {
//DEPRECATED       NS_LOG_ERROR("File " << m_flowStatsTraceFile << " cannot be opened for writing. Tracing disabled");
//DEPRECATED       return false;
//DEPRECATED     }
//DEPRECATED 
//DEPRECATED     m_fs_os = os;
//DEPRECATED     return true;
//DEPRECATED   }
//DEPRECATED   else {
//DEPRECATED     return false;
//DEPRECATED   }
//DEPRECATED }
//DEPRECATED 
//DEPRECATED void
//DEPRECATED DynamicArrivals::printHeader()
//DEPRECATED {
//DEPRECATED   if(m_os!=NULL)
//DEPRECATED   {
//DEPRECATED     *m_os << "NodeId="<<GetNode()->GetId()
//DEPRECATED      << "\n"
//DEPRECATED      << "Time"
//DEPRECATED      << "\t"
//DEPRECATED      << "AppId" 
//DEPRECATED      << "\t"
//DEPRECATED      << "currentWindow"
//DEPRECATED       << "\t"
//DEPRECATED      << "pendingWindow"
//DEPRECATED      << "\n";
//DEPRECATED   }
//DEPRECATED   
//DEPRECATED }
//DEPRECATED 
//DEPRECATED void DynamicArrivals::printFlowStatsHeader()
//DEPRECATED {
//DEPRECATED   if(m_fs_os!=NULL)
//DEPRECATED   {
//DEPRECATED     *m_fs_os << "end_time"
//DEPRECATED //        << "\t" << "id"
//DEPRECATED         << "\t" << "interest_name"
//DEPRECATED         << "\t" << "pkts_delivered"
//DEPRECATED         << "\t" << "pkts_delivered_bytes"
//DEPRECATED         << "\t" << "raw_data_delivered_bytes"
//DEPRECATED         << "\t" << "elapsed"
//DEPRECATED         << "\t" << "rate"
//DEPRECATED         << "\t" << "goodput"
//DEPRECATED         << "\t" << "num_timeouts"
//DEPRECATED         << "\t" << "num_holes"
//DEPRECATED         << std::endl;
//DEPRECATED   }
//DEPRECATED }
//DEPRECATED 
//DEPRECATED void
//DEPRECATED DynamicArrivals::printWindowTrace(uint32_t appId, double curwindow, unsigned winpending)
//DEPRECATED {
//DEPRECATED     *m_os << std::fixed<< std::setprecision(6)<<Simulator::Now().ToDouble(Time::S) << "\t" /*<< m_node << "\t" */<< appId 
//DEPRECATED         <<"\t" <<curwindow<<"\t" <<winpending<<"\n";
//DEPRECATED }
//DEPRECATED 
//DEPRECATED void
//DEPRECATED DynamicArrivals::printFlowStatsTrace(flow_stats_t *fs)
//DEPRECATED {
//DEPRECATED     *m_fs_os << fs->end_time
//DEPRECATED //        << "\t" << fs->id
//DEPRECATED         << "\t" << fs->interest_name
//DEPRECATED         << "\t" << fs->pkts_delivered
//DEPRECATED         << "\t" << fs->pkts_delivered_bytes
//DEPRECATED         << "\t" << fs->raw_data_delivered_bytes
//DEPRECATED         << "\t" << fs->elapsed
//DEPRECATED         << "\t" << fs->rate
//DEPRECATED         << "\t" << fs->goodput
//DEPRECATED         << "\t" << fs->num_timeouts
//DEPRECATED         << "\t" << fs->num_holes
//DEPRECATED         << std::endl;
//DEPRECATED }

// template<std::string atrributeName, typename T>
// DynamicArrivals::SetAttribute(T attributeValue)
// {
//   m_downloadAppHelper.SetAttribute();
// }

DynamicArrivalProcess::DynamicArrivalProcess()
  : m_lambda(1.0)
  , m_firstTime(true)
  , m_random(0)
{
  NS_LOG_FUNCTION_NOARGS();
}

double
DynamicArrivalProcess::GetTime()
{
	return m_random->GetValue();
}

void
DynamicArrivalProcess::SetRate(double rate)
{
	m_lambda = rate;
}

void
DynamicArrivalProcess::SetRandomize(const std::string& value)
{
  if (value == "uniform") {
    m_random = CreateObjectWithAttributes<UniformRandomVariable>(
        "Min", DoubleValue(0.0),
        "Max", DoubleValue(2.0 / m_lambda)
    );
  }
  else if (value == "exponential") {
    m_random = CreateObjectWithAttributes<ExponentialRandomVariable>(
        "Mean" , DoubleValue( 1.0 / m_lambda),
        "Bound", DoubleValue(50.0 / m_lambda)
    );
  }
  else
    m_random = 0;

  m_randomType = value;
}

std::string
DynamicArrivalProcess::GetRandomize() const
{
  return m_randomType;
}

DynamicPopularityProcess::DynamicPopularityProcess()
  : m_catalog_size(100),
	m_param_1(1.0),
	m_param_2(1.0),
	m_param_3(1.0),
	m_random(0)
{
  NS_LOG_FUNCTION_NOARGS();
}

uint32_t
DynamicPopularityProcess::GetRank()
{
	return m_random->GetInteger();
}

void
DynamicPopularityProcess::SetCatalogSize(uint32_t size)
{
	m_catalog_size = size;
}


void
DynamicPopularityProcess::SetRandomize(const std::string& value)
{
  if (value == "uniform")
  {
    m_random = CreateObjectWithAttributes<UniformRandomVariable>(
        "Min", DoubleValue(1.0),
        "Max", DoubleValue((double)m_catalog_size)
    );
  }
  else if (value == "zipf")
  {
      m_random = CreateObjectWithAttributes<ZipfRandomVariable>(
          "N", IntegerValue(m_catalog_size),
          "Alpha", DoubleValue(m_param_1)
      );
  }
  else if (value == "Zipf") // faster implementation of a zipf distribution log2 N vs N instead
  {
      m_random = CreateObjectWithAttributes<EmpiricalRandomVariable>();
      for (uint32_t i = 1; i <= m_catalog_size; i++)
      {
    	  m_param_2 += (1.0 / std::pow ((double)i, m_param_1));
      }
      m_param_2 = 1.0 / m_param_2;
      double sum_prob = 0;
      for (uint32_t i = 1; i <= m_catalog_size; i++)
      {
    	  sum_prob += m_param_2 / std::pow ((double)i, m_param_1);
    	  m_random->GetObject<EmpiricalRandomVariable>()->CDF(i, sum_prob);
      }

  }
  else if (value == "weibull")
  {
    m_random = CreateObjectWithAttributes<WeibullRandomVariable>(
        "Shape", DoubleValue(m_param_1),
        "Scale", DoubleValue(m_param_2),
        "Bound", DoubleValue(m_param_3)
    );
  }
  else
    m_random = 0;

  m_randomType = value;
}

std::string
DynamicPopularityProcess::GetRandomize() const
{
  return m_randomType;
}

void
DynamicPopularityProcess::SetQ(double q)
{
  m_param_2=q;
}

void
DynamicPopularityProcess::SetS(double s)
{
  m_param_1=s;
  SetRandomize(m_randomType);
    
}

} /* namespace ndn */
} /* namespace ns3 */

#endif // RAAQM
