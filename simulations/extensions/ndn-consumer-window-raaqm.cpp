/*
 * ndn-consumer-window-raaqm.cpp
 *
 *  Created on: Mar 18, 2015
 *      Author: muscariello
 *              Zeng Xuan
 */

//variables arribute statstics

#ifdef RAAQM

#ifndef PATH_LABELLING
#error "RAAQM requires PATH_LABELLING"
#endif // !PATH_LABELLING

#include "ndn-consumer-window-raaqm.hpp"

#include <sys/time.h>
#include <assert.h>
#include <stdio.h>

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/core-module.h"

#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>

#include <iomanip>

#include "model/ndn-l3-protocol.hpp"

// hopCount accounting
#include "ns3/ndnSIM/utils/ndn-ns3-packet-tag.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerWindowRAAQM");

namespace ns3 {
namespace ndn {  
    
NS_OBJECT_ENSURE_REGISTERED(ConsumerWindowRAAQM);

TypeId
ConsumerWindowRAAQM::GetTypeId(void)
{
	static TypeId tid =
			TypeId("ns3::ndn::ConsumerWindowRAAQM")
			.SetGroupName("Ndn")
			.SetParent<Consumer>()
			.AddConstructor<ConsumerWindowRAAQM>()

      .AddAttribute("AllowStale", "Allow to receive staled/expired data",
          BooleanValue(ALLOW_STALE),
          MakeBooleanAccessor(&ConsumerWindowRAAQM::m_allow_stale),
          MakeBooleanChecker())

      .AddAttribute("MaxWindowSize", 
          "maximum allowed window size in the download process, should be less than 128, by default it's 127", 
          IntegerValue(INITIAL_MAX_WINDOW),
          MakeIntegerAccessor(&ConsumerWindowRAAQM::setMaxWindow), 
          MakeIntegerChecker<int32_t>())

      //override the arributes defined in consumer class, so that the default value change to 1s.
      .AddAttribute("DefaultInterestLifeTime", 
          "default interest LifeTime for each interest (an override)", 
          StringValue(INTEREST_LIFETIME),
          MakeTimeAccessor(&ConsumerWindowRAAQM::m_defaultInterestLifeTime), 
          MakeTimeChecker())

      //override the definition of RetxTimer, such that the timeout of interest is equal to that of the interest lifetime. 
      .AddAttribute("NewRetxTimer",
          "Timeout defining how frequent retransmission timeouts should be checked",
          StringValue("50ms"),
          MakeTimeAccessor(&ConsumerWindowRAAQM::GetRetxTimer, &ConsumerWindowRAAQM::SetRetxTimer),
          MakeTimeChecker())

      .AddAttribute("DropFactor", "the drop factor used to compute window size drop probability", 
          StringValue(DROP_FACTOR_STRING),
          MakeDoubleAccessor(&ConsumerWindowRAAQM::m_dropFactor),
          MakeDoubleChecker<double>())

      .AddAttribute("Pmin", 
          "minimum drop probability used to compute window size drop probability", 
          StringValue(P_MIN_STRING),
          MakeDoubleAccessor(&ConsumerWindowRAAQM::m_Pmin),
          MakeDoubleChecker<double>())

      .AddAttribute("Beta",
          "Multiplifier for window size decrease, when necessary to decrease the window size. It should be between 0 and 1", 
          StringValue(BETA_STRING),
          MakeDoubleAccessor(&ConsumerWindowRAAQM::m_BETA),
          MakeDoubleChecker<double>())

      .AddAttribute("Gamma", 
          "the increment that is added to the window size, when necessary to increase window size. It should be a positvie integer", 
          IntegerValue(GAMMA),
          MakeIntegerAccessor(&ConsumerWindowRAAQM::m_GAMMA), 
          MakeIntegerChecker<int32_t>())

      .AddAttribute("EST_LEN", "The maximum rtt queue size for each individual path", 
          IntegerValue(EST_LEN),
          MakeIntegerAccessor(&ConsumerWindowRAAQM::m_EST_LEN), 
          MakeIntegerChecker<int32_t>())

      .AddAttribute("MaxSeq", "Maximum sequence number to request",
          IntegerValue(MAX_SEQUENCE_NUMBER),// NOTE: to be changed if necessary
          MakeIntegerAccessor(&ConsumerWindowRAAQM::m_seqMax),
          MakeIntegerChecker<uint32_t>())
			 
			.AddAttribute("EnableOutput", "to ouput the content each time a data packet is received",
          BooleanValue(false),
          MakeBooleanAccessor(&ConsumerWindowRAAQM::m_isOutputEnabled),
          MakeBooleanChecker())

      .AddAttribute("EnablePathReport", "to ouput all the paths and statistics on each path",
          BooleanValue(false),
          MakeBooleanAccessor(&ConsumerWindowRAAQM::m_isPathReportEnabled),
          MakeBooleanChecker())
			
			.AddAttribute("EnableAbortMode", "when a full window of interest is timedout, stop the application",
          BooleanValue(false),
          MakeBooleanAccessor(&ConsumerWindowRAAQM::m_isAbortModeEnabled),
          MakeBooleanChecker())
			
			.AddAttribute("RetrxLimit",
          "set retransmission limit, such that when the limit is reached, the application will mark the data as received and move on, must be >0. RetrxLimit=n means maximum n-1 retransmit is allowed", 
          StringValue("0"),
          MakeUintegerAccessor(&ConsumerWindowRAAQM::m_reTxLimit),
          MakeUintegerChecker<uint32_t>())

			.AddAttribute("initialWindowSize", "initial window size ,by default=1", 
          IntegerValue(1),
          MakeIntegerAccessor(&ConsumerWindowRAAQM::m_initialWindowSize), 
          MakeIntegerChecker<int32_t>())

			.AddAttribute("GoodputHandoverTracefile", "the name of the file used to store flow statistics of all download apps",
          StringValue(""),
          MakeStringAccessor(&ConsumerWindowRAAQM::m_goodputHandoverTracefile),
          MakeStringChecker())
			
			.AddTraceSource("WindowTrace",
          "Window trace file name, by default window traces are not logged, if you don't specify a valid file name",
          MakeTraceSourceAccessor(&ConsumerWindowRAAQM::WindowTrace))

			.AddTraceSource("FlowStats",
          "Flow stats trace file name, by default flow stats traces are not logged, if you don't specify a valid file name",
          MakeTraceSourceAccessor(&ConsumerWindowRAAQM::m_FlowStatsTrace))

      .AddTraceSource("StopCallback", "Callback called when application finishes", 
          MakeTraceSourceAccessor(&ConsumerWindowRAAQM::m_StopCallback))
		                      ;
				      
 return tid;
}

ConsumerWindowRAAQM::ConsumerWindowRAAQM()
    : maxwindow(INITIAL_MAX_WINDOW)
    , m_dropFactor(DROP_FACTOR)
    , m_Pmin(P_MIN)
    , m_EST_LEN(EST_LEN)
   // , m_seqMax(nchunks)//originally finalchunk, it is initialized in parent class
    , m_GAMMA(GAMMA)
    , m_BETA(BETA)
    , m_allow_stale(ALLOW_STALE)
  
    , m_isOutputEnabled(false)//by default dont print received data content
    , m_isPathReportEnabled(false)//by default dont print path status
       
    , winrecvd(0)
    , winpending(0)
    , curwindow(1)///< \brief note: by default current window is initialized to 1 on start
    , ooo_base(0)
    , ooo_count(0)
   // , m_seq(0)//originally ooo_next, it is initialized in parent class
    , finalslot(~0)//invalid number
    
    , interests_sent(0)
    , pkts_recvd(0)
    , pkts_bytes_recvd(0)
    , raw_data_bytes_recvd(0)//size_t m_totalSize;
    , pkts_delivered(0)
    , raw_data_delivered_bytes(0)
    , pkts_delivered_bytes(0)
    , nTimeouts(0)//number of interest timeout events    
    
   // , m_timeout(0)//system timeout, default to run forever
    
    //new feature:
    , m_isAbortModeEnabled(false)
    , winretrans(0)
    , m_reTxLimit(0)
    , m_nHoles(0)
    , m_initialWindowSize(1)
    , m_lastHandover_raw_data_delivered_bytes(0)
    , m_maxnumHandover(100)
    , m_currentNumHandover(0)
    , m_goodputHandoverTracefile("")
    , m_os(shared_ptr<std::ostream>())
    , m_recv_packets(0)
    , m_mean_hop_count(0)
    , m_handover_recv_packets(0)
    , m_handover_mean_hop_count(0)

{
  m_seqMax=MAX_SEQUENCE_NUMBER;
 

}

ConsumerWindowRAAQM::~ConsumerWindowRAAQM() {
	Simulator::Remove(m_newRetxEvent);
	
}

void
ConsumerWindowRAAQM::control_stats_reporter()
{
	Time now=Simulator::Now();
	NS_LOG_INFO(std::fixed<<std::setprecision(6)
			<<"[control stats report]:"
			<<"at time "<<now.GetSeconds()<<"sec ndn-icp-download: "
			<<"interests sent "<<interests_sent<<", recvd "<<pkts_recvd<<", t/o "<<nTimeouts<<", "
			<<"curwin "<<curwindow<<", winpending "<<winpending<<", recvd "<<winrecvd
			);
}

void
ConsumerWindowRAAQM::setCurrentPath(shared_ptr<Path> path)
{
  cur_path=path;
}

void
ConsumerWindowRAAQM::print_summary()
{
  const char *expid;
  const char *dlm = " ";
  expid = getenv("NDN_EXPERIMENT_ID");
  if (expid == NULL)
        expid = dlm = "";
  double elapsed = 0.0;
  double rate = 0.0;
  double goodput = 0.0;

  stop_tv=Simulator::Now();
  elapsed = stop_tv.GetSeconds()-start_tv.GetSeconds();

  if (elapsed > 0.00001)
  {
	  rate = pkts_delivered_bytes*8/elapsed/1000000;
	  goodput = raw_data_delivered_bytes*8/elapsed/1000000;
  }
  NS_LOG_INFO(std::fixed << std::setprecision(6)
  	  	  	  <<stop_tv.GetSeconds() <<" ndn-icp-download[" <<GetId()<<"], prefix="<<m_interestName<<" : "<<expid<<dlm
			  <<pkts_delivered_bytes<<" bytes transferred (filesize: "<<raw_data_delivered_bytes<<" ) in "<<elapsed<<" "
			  <<"seconds rate: "<<rate<<" goodput: "<<goodput<<" Mbps t/o:"<<nTimeouts<<" "
			  <<"missing chunks: "<<m_nHoles<<" missRatio: "<< m_nHoles/(double)(pkts_delivered)
			  );
     
  // Write flow statistics
  FlowStatsElastic fs;
  fs.end_time = stop_tv.GetSeconds();
  fs.id = GetId();
  fs.interest_name = m_interestName.toUri();
  fs.pkts_delivered = pkts_delivered;
  fs.pkts_delivered_bytes = pkts_delivered_bytes;
  fs.raw_data_delivered_bytes = raw_data_delivered_bytes;
  fs.elapsed = elapsed;
  fs.rate = rate;
  fs.goodput = goodput;
  fs.num_timeouts = nTimeouts;
  fs.num_holes = m_nHoles;

  m_FlowStatsTrace(fs);
  if(m_isPathReportEnabled)
  {
	  //print number of paths in the transmission process,
	  //we need to exclude the default path
// 	  std::cout<<"number of paths in the path table is(exclude the default path): "
// 			  	  <<(PathTable.size()-1)<<std::endl;
	  NS_LOG_INFO("number of paths in the path table is(exclude the default path): "
			  	  <<(PathTable.size()-1));
	  int i=0;
	  BOOST_FOREACH(HashTableForPath::value_type kv, PathTable)
	  {
		  if(kv.first.length()<=1)
		  {
			  i++;
// 			  std::cout<<"[path "<< i <<"]\n" <<"ID : "
// 					  <<(int) *(reinterpret_cast<const unsigned char*>(kv.first.data()))<<"\n";
			  NS_LOG_INFO("[path "<< i <<"]\n" <<"ID : "
 					  <<(int) *(reinterpret_cast<const unsigned char*>(kv.first.data())));
			  kv.second->path_reporter();
		  }
	  }
  }
}

void
ConsumerWindowRAAQM::update_RTT_stats(unsigned slot)
{
	if(!cur_path)
	{
		//std::cout << "ERROR: no current path found, exit" << std::endl;
		NS_LOG_ERROR("no current path found, exit");
		//exit(1);
		StopApplication();
		return;
	}
	else
	{
		double rtt;
		Time now;
		now=Simulator::Now();
		const Time& sendTime=Window[slot].getSendTime();
		rtt = now.GetMicroSeconds()-sendTime.GetMicroSeconds();
		//if(Window[slot].isRetrans()){
		cur_path->addRTT(rtt);
		cur_path->smoothTimer();
		//}
	}
}

void
ConsumerWindowRAAQM::wininc()
{
	if (winrecvd == (unsigned)curwindow)
	{
		if ((unsigned)curwindow < maxwindow)
			curwindow += (double)m_GAMMA;

		winrecvd = 0;
    }
}

void
ConsumerWindowRAAQM::RAQM()
{
   if(!cur_path)
  {
    //std::cout << "ERROR: no current path found, exit" << std::endl;
    NS_LOG_ERROR("no current path found, exit");
    //exit(1);
    StopApplication();
    return;

  }
  else
  {
    //change drop probability according to RTT statistics
    cur_path->resetDropProb();
    if (rand() % 10000 <= cur_path->getDropProb() * 10000) {
	curwindow = curwindow * m_BETA;
	if (curwindow < 1)
		curwindow = 1;
	winrecvd = 0;
    }
  }
}

void
ConsumerWindowRAAQM::npkt_received( unsigned slot)
{
	//update win counters
	winpending--;
	//winpendingTrace(GetId(), winpending);//trace the change in winpending
	winrecvd++;
	wininc();
	//update RTT stats for the current path
	update_RTT_stats(slot);
	//set drop probablility and window size accordingly
	RAQM();
	//optional to report path stats here
	//cur_path->path_reporter();
	//optional to report control stats
	//control_stats_reporter();
	WindowTrace(GetId(),curwindow,winpending);
}

void
ConsumerWindowRAAQM::SendPacket()
{	
	if (!m_active)
		return;

//NS_LOG_FUNCTION_NOARGS();
	uint32_t seq;
	unsigned slot;
	if(m_retxSeqs.size()>0) //if timedout interest is in the queque, they are sent in priority
	{
	  seq = *m_retxSeqs.begin();
	  m_retxSeqs.erase(m_retxSeqs.begin());
	  slot= seq % BUF_DIM;
	  NS_LOG_DEBUG("App="<<GetId()<<" > retransmit interest for " << seq);
//  Window[slot].setRetrans(true);
	}
	else//send a new interest
	{
	  seq=m_seq++;
	  slot= seq % BUF_DIM;
	//  Window[slot].setRetrans(false);
	  NS_LOG_DEBUG("App="<<GetId()<<" > interest for " << seq);
	}
	NS_ASSERT(!Window[slot].isDataReceived());
	Time now;
	now=Simulator::Now();
	Window[slot].setSendTime(now);
	//make a proper interest:
	//make interest name
	shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
	nameWithSequence->appendSequenceNumber(seq);
        //make interest:
	shared_ptr<Interest> interest = make_shared<Interest>();
	interest->setName(*nameWithSequence);
	time::milliseconds interestLifeTime(m_defaultInterestLifeTime.GetMilliSeconds());
	interest->setInterestLifetime(interestLifeTime);
	interest->setMustBeFresh(m_allow_stale);
	interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
	
	// NS_LOG_DEBUG("App="<<GetId()<<" > interest for " << seq);
	 //log statistics
	 WillSendOutInterest(seq);
	 m_transmittedInterests(interest, this, m_face);
	 m_face->onReceiveInterest(*interest);
	//TODO schedule a timeout after DEFAULT_INTEREST_TIMEOUT(or the one given by option for timer),bound to onTimeout
	 interests_sent++;
	
	//update muber of interests in flight
	 winpending++;
	//winpendingTrace(GetId(), winpending);
	 NS_ASSERT(ooo_count < BUF_DIM);
	
	/** NOTE: due to the fact that in our application when we plan to send an interest
	 * , we will always schedule to send it immediately,
	 * instead of scheduling to send an interest immediately every time, we call
	 * sendPacket() function directly each time. So the following call is not needed.
	 */
	//ScheduleNextPacket();
}

void
ConsumerWindowRAAQM::ScheduleNextPacket()
{
  SendPacket();
}

void
ConsumerWindowRAAQM::StartApplication()
{
	//NS_LOG_FUNCTION_NOARGS();
	Simulator::Remove(m_retxEvent);
	
	//set the interest timeout checking
	SetRetxTimer(m_newRetxTimer);
	shared_ptr<Path> initPath=make_shared<Path>(m_dropFactor,m_Pmin,m_defaultInterestLifeTime.GetMicroSeconds(),m_EST_LEN);
	insertToPathTable(DEFAULT_PATH_ID,initPath);
	setCurrentPath(initPath);
	setStartTimetoNow();
	setLastTimeOutoNow();
	m_lastHandoverTime=Simulator::Now();
	curwindow=m_initialWindowSize;
	
	bool isOK=openTraceFile();
  if(isOK)
  {
    NS_LOG_DEBUG("goodput traces are logged");
     printHeader();
      //connect to callback:
    ns3::Config::ConnectWithoutContext("/NodeList/*/$ns3::ConnectivityManager/OnWiFiDeassociation", MakeCallback(&ConsumerWindowRAAQM::printGoodputHandoverTrace,this));
  }
  else
    NS_LOG_DEBUG("goodput traces are NOT logged");
  
   
    Consumer::StartApplication();
    NS_LOG_INFO("App=" <<GetId()<<", prefix="<<m_interestName);
}

void
ConsumerWindowRAAQM::ReStart(std::string nextPrefix)
{
	reInitialize();
	m_interestName=Name(nextPrefix);
	shared_ptr<Path> initPath=make_shared<Path>(m_dropFactor,m_Pmin,m_defaultInterestLifeTime.GetMicroSeconds(),m_EST_LEN);
	insertToPathTable(DEFAULT_PATH_ID,initPath);
	setCurrentPath(initPath);
	setStartTimetoNow();
	setLastTimeOutoNow();
	m_lastHandoverTime=Simulator::Now();

	NS_ASSERT(m_active != true);
	m_active = true;

	NS_ASSERT_MSG(GetNode()->GetObject<L3Protocol>() != 0,
                "Ndn stack should be installed on the node " << GetNode());
	ScheduleNextPacket();
	NS_LOG_INFO("restarted, App=" <<GetId()<<", prefix="<<m_interestName);
}

void
ConsumerWindowRAAQM::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  print_summary();
  control_stats_reporter();
  
  //cleanup memory:
  Simulator::Remove(m_newRetxEvent);
  PathTable.clear();
  cur_path=NULL;
  
  // XXX Currently, we can never stop an application, it is "doomed" to be
  // reused ! :) This is due to the use of TracedCallback... 
  /*
  if (OnStop.IsNull())
  {
    Consumer::StopApplication();
  } else {}
  */
   Simulator::Cancel(m_sendEvent);
   if (!m_active)
      return; // don't assert here, just return

   m_active = false;
   m_StopCallback(this);
}

void
ConsumerWindowRAAQM::reInitialize()
{
	m_seq=0;
	winrecvd=0;
	winpending=0;
	curwindow=m_initialWindowSize;///< \brief note: by default current window is initialized to 1 on start
	ooo_base=0;
	ooo_count=0;
	finalslot=~0;//invalid number
	interests_sent=0;
	pkts_recvd=0;
	pkts_bytes_recvd=0;
	raw_data_bytes_recvd=0;//size_t m_totalSize;
	pkts_delivered=0;
	raw_data_delivered_bytes=0;
	pkts_delivered_bytes=0;
	nTimeouts=0;//number of interest timeout events
	winretrans=0;
	m_nHoles=0;
	m_currentNumHandover=0;//current handover
	SetRetxTimer(m_newRetxTimer);
	for(int i=0;i<BUF_DIM;i++)
	{
	  Window[i]=InFlightInterests();
	}
}

// TODO|void
// TODO|ConsumerWindowRAAQM::SetStopCallback(Callback<void, Ptr<App>> cb)
// TODO|{
// TODO|	m_StopCallback = db;
// TODO|}

bool
ConsumerWindowRAAQM::openTraceFile()
{
    using namespace boost;
    using namespace std;

  shared_ptr<std::ostream> outputStream;
  if (m_goodputHandoverTracefile != "") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(m_goodputHandoverTracefile.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << m_goodputHandoverTracefile << " cannot be opened for writing. Tracing disabled");
      return false;
    }

    m_os = os;
    return true;
  }
  else {
    return false;
  }
}

void
ConsumerWindowRAAQM::printHeader()
{
  if(m_os!=NULL)
  {
    *m_os << "handover_time"
     << "\t" << "goodput"
     << "\t" << "num_handover"
     << "\t" << "hop_count"
     << "\n";
  }
}

void
ConsumerWindowRAAQM::printGoodputHandoverTrace()
{
  if(!m_active)
    return;
  //compute:
  double elapsed = Simulator::Now().ToDouble(Time::S) - m_lastHandoverTime.GetSeconds();
  intmax_t tmp_raw_data_delivered_bytes=raw_data_delivered_bytes-m_lastHandover_raw_data_delivered_bytes;
  double goodput = tmp_raw_data_delivered_bytes*8/elapsed/1000000;
  if(m_currentNumHandover > 30) {
    std::cout << "Aborting flow after 30 handovers" << std::endl;
    Simulator::Stop(Seconds(0.0));
  }
  m_currentNumHandover++;

  //change state:
  m_lastHandoverTime=Simulator::Now();
  m_lastHandover_raw_data_delivered_bytes=raw_data_delivered_bytes;
  
  //print:
  *m_os << std::fixed<< std::setprecision(6)<<Simulator::Now().ToDouble(Time::S) 
  << "\t" << goodput
  << "\t" << m_currentNumHandover
  << "\t" << m_mean_hop_count
  <<"\n";
  m_os->flush();
  
  m_handover_recv_packets = 0;
  m_handover_mean_hop_count = 0;
}

void
ConsumerWindowRAAQM::onReachRetxLimit(uint32_t seq)
{
	//pretend the packet is received
	unsigned slot = seq % BUF_DIM;
	NS_LOG_DEBUG("App=" << GetId()<<" > interest for " << seq << ", retxLimit reached");
	if(m_isAbortModeEnabled)
	{
		std::map<uint32_t, uint32_t>::iterator it=m_seqRetxCounts.find(seq);
		if(winretrans>0 && it!=m_seqRetxCounts.end() && it->second>=2)
			winretrans--;
	}
	
	//perform operation defined in consumer class
	int hopCount = -1;
	SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
	if (entry != m_seqLastDelay.end())
	{
		m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
	}
	entry = m_seqFullDelay.find(seq);
	if (entry != m_seqFullDelay.end())
	{
		m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
	}
	m_seqRetxCounts.erase(seq);
	m_seqFullDelay.erase(seq);
	m_seqLastDelay.erase(seq);
	m_seqTimeouts.erase(seq);
	m_retxSeqs.erase(seq);
	m_rtt->AckSeq(SequenceNumber32(seq));
	if(seq==m_seqMax-1)
	{
		finalslot=slot;
		GOT_HERE();
	}
	//out of order data received, save it for later.
	if(slot != ooo_base)
	{
		if (!Window[slot].isDataReceived())
		{//check if packet is received before
			GOT_HERE();
			ooo_count++;
			Window[slot].markAsReceived();
			npkt_received(slot);
		}
		//TODO maybe needed uncomment it, this could be used in the future.
		//else
		//cur_path->dups++;
	}
	else //in oder data arrived
	{
		NS_ASSERT(!Window[slot].isDataReceived());
		m_nHoles++;
		if (slot == finalslot)
		{
			//npkt_received(slot);
			//old: m_face.shutdown();
			//new:
			StopApplication();
			return;
		}
		npkt_received(slot);
		slot = (slot + 1) % BUF_DIM;
		/**consume out-of-order pkts already received until there is a hole*/
		while ((ooo_count > 0) && (Window[slot].isDataReceived()))
		{
			if(Window[slot].getRawDataSize()!=0)
			{
				pkts_delivered++;
				raw_data_delivered_bytes += Window[slot].getRawDataSize();//maybe need to assign DataSize-1?
				pkts_delivered_bytes += Window[slot].getPacketSize();//maybe size-1?
			}
			else
			{
				m_nHoles++;
			}
			if (slot == finalslot)
			{
				GOT_HERE();
				//m_face.shutdown();
				StopApplication();
				return;
			}
			Window[slot].removeRawData();
			slot = (slot + 1) % BUF_DIM;
			ooo_count--;
		}
		ooo_base = slot;
	}
	/** Ask for the next one or two */
	while ( (winpending < (unsigned)curwindow) && (m_seq < m_seqMax) )
	{
		//ask_more(ooo_next++);
		//ooo_next++;
		ScheduleNextPacket();
	}
	GOT_HERE();
}

void
ConsumerWindowRAAQM::SetRetxTimer(Time retxTimer)
{
  m_newRetxTimer = retxTimer;
  if (m_newRetxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_newRetxEvent); // slower, but better for memory

  }
  // schedule even with new timeout
  m_newRetxEvent = Simulator::Schedule(m_newRetxTimer, &ConsumerWindowRAAQM::CheckRetxTimeout, this);
}

Time
ConsumerWindowRAAQM::GetRetxTimer() const
{
  return m_newRetxTimer;
}
	
void
ConsumerWindowRAAQM::CheckRetxTimeout()
{
  //NS_LOG_FUNCTION_NOARGS();
  Time now = Simulator::Now();

  Time rto = m_defaultInterestLifeTime;
   //NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  while (!m_seqTimeouts.empty()) {
    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
      m_seqTimeouts.get<i_timestamp>().begin();
    if (entry->time + rto <= now) // timeout expired?
    {
      uint32_t seqNo = entry->seq;
      m_seqTimeouts.get<i_timestamp>().erase(entry);
      OnTimeout(seqNo);
      if(!m_active)
	break;
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  if(m_active)
  m_newRetxEvent = Simulator::Schedule(m_newRetxTimer, &ConsumerWindowRAAQM::CheckRetxTimeout, this);
}


void
ConsumerWindowRAAQM::OnData(shared_ptr<const Data> data)
{
	NS_LOG_FUNCTION_NOARGS();
	if(!m_active)
	  return;
	const Block& content = data->getContent();
	const Name& name = data->getName();
	size_t dataSize=content.value_size();
	size_t packetSize=data->wireEncode().size();
	uint64_t seq = name.at(-1).toSequenceNumber();
	unsigned slot = seq % BUF_DIM;
	
	 //NS_LOG_DEBUG("App="<<GetId()<<" < DATA for " << seq);
	Consumer::OnData(data);///<forward call to consumer class to make statistics and basic operations.

  // hopCount accounting
  int hopCount = -1;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) {
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }


  m_mean_hop_count *= m_recv_packets;
  m_recv_packets++;
  m_mean_hop_count += hopCount;
  m_mean_hop_count /= m_recv_packets;

  m_handover_mean_hop_count *= m_handover_recv_packets;
  m_handover_recv_packets++;
  m_handover_mean_hop_count += hopCount;
  m_handover_mean_hop_count /= m_handover_recv_packets;

	if(m_isAbortModeEnabled)
	{
	  std::map<uint32_t, uint32_t>::iterator it=m_seqRetxCounts.find(seq);
	  if(winretrans>0 && it!=m_seqRetxCounts.end() && it->second>=2)
		  winretrans--;
	}
	if (m_isOutputEnabled)
    {
		//optional ot print out packet size
		//std::cout << "dataSize is " << dataSize << std::endl;
		//std::cout << "packetSize is " << packetSize << std::endl;
		//std::cout << "data received, seq number #" << seq << "\n";
		NS_LOG_INFO( "data received, seq number #" << seq);
		std::cout.write(reinterpret_cast<const char*>(content.value()), content.value_size());
    }
      /* 1.get the pathID from packet content,
         2.then find(or also store if a new path) it in a hash table
         3.if it is a new path, initialize the path with default parameters
         4. point cur_path to the found or created path
     */
	//solved
	GOT_HERE();
#if 1
	ndn::Block pathIdBlock=data->getPathId();
	if(pathIdBlock.empty())
	{
		//std::cout<< "ERROR:path ID lost in the transmission.";
		NS_LOG_ERROR("path ID lost in the transmission.");
		//exit(1);
		StopApplication();
		return;
	}
	unsigned char pathId=*(data->getPathId().value());
#else
	unsigned char pathId = 0;
#endif
	NS_LOG_DEBUG("App="<<GetId()<<" < DATA for " << seq<<", pathID="<<static_cast<int>(pathId));
	std::string pathIdSring(1,pathId);
	if(PathTable.find(pathIdSring)==PathTable.end())
	{
		if(cur_path)
		{
			//create a new path with some default param
			if(PathTable.empty())
			{
 			//	std::cout<<"no path initialized for path table, "
 			//			"error could be in default path initialization.\n";
			  NS_LOG_ERROR("no path initialized for path table, error could be in default path initialization.");
				//exit(1);
				StopApplication();
				return;
			}
			else
			{
				//initiate the new path default param
				shared_ptr<Path> newPath=make_shared<Path>(*(PathTable.at(DEFAULT_PATH_ID)));
				//insert the new path into hash table
				PathTable[pathIdSring]=newPath;
			}
		}
		else
		{
			//std::cout<< "UNEXPECTED ERROR: when running,current path not found.\n" ;
		        NS_LOG_ERROR("when running,current path not found.");
			//exit(1);
			StopApplication();
			return;
		}
	}
	cur_path=PathTable[pathIdSring];
	//update measurements
	pkts_recvd++;
	raw_data_bytes_recvd += dataSize;
	pkts_bytes_recvd += packetSize; //data wont be encoded again since its already encoded before received

	//update measurements for path
	cur_path->update_received_stats(packetSize,dataSize);
	//TODO important check if the packet(slot) is out of window
	//from my point of view, this is not going to happen guaranteed by logic
// 	unsigned int nextSlot=(m_seq%BUF_DIM);
// 	if(nextSlot>ooo_base &&(slot>nextSlot ||slot<ooo_base))
// 	{
// 		//std::cout<<"out of window data received at #"<<slot<<std::endl;
// 	        NS_LOG_DEBUG("out of window data received at #"<<seq);
// 		return;
// 	}
// 	if(nextSlot<ooo_base && (slot>nextSlot && slot<ooo_base))
// 	{
// 		//std::cout<<"out of window data received at #"<<slot<<std::endl;
// 	        NS_LOG_DEBUG("out of window data received at #"<<seq);	
// 	        return;
// 	}
//for stability use >= , instead of == condition here
	if(seq>=m_seqMax-1)
	{
		finalslot=slot;
		GOT_HERE();
	}
	if(slot != ooo_base)//out of order data received, save it for later.
	{
		if (!Window[slot].isDataReceived())
		{//check if packet is received before
			GOT_HERE();
			ooo_count++;
			Window[slot].addRawData(*data);
			npkt_received(slot);
		}
		//TODO maybe needed uncomment it, this could be used in the future.
		//else
		//cur_path->dups++;
	}
	else //in oder data arrived
	{
	  NS_ASSERT(!Window[slot].isDataReceived());
	  pkts_delivered++;
	  raw_data_delivered_bytes += dataSize;
	  pkts_delivered_bytes += packetSize;

	  if (slot == finalslot)
	  {
		  //npkt_received(slot);
		  //old: m_face.shutdown();
		 //new:
		 StopApplication();
		 return;
	  }
	  npkt_received(slot);
	  slot = (slot + 1) % BUF_DIM;
	  /**consume out-of-order pkts already received until there is a hole*/
	  while ((ooo_count > 0) && (Window[slot].isDataReceived()))
	  {
		  if(Window[slot].getRawDataSize()!=0)
		  {
			  pkts_delivered++;
			  raw_data_delivered_bytes += Window[slot].getRawDataSize();//maybe need to assign DataSize-1?
			  pkts_delivered_bytes += Window[slot].getPacketSize();
			  //maybe size-1?
		  }
		  else
		  {
			  m_nHoles++;
		  }
		  if (slot == finalslot)
		  {
			  GOT_HERE();
			  //m_face.shutdown();
			  StopApplication();
			  return;
		  }
    	  Window[slot].removeRawData();
    	  slot = (slot + 1) % BUF_DIM;
    	  ooo_count--;
	  }
	  ooo_base = slot;
	}
	/** Ask for the next one or two */
	while ( (winpending < (unsigned)curwindow) &&
    		  (m_seq < m_seqMax) )
	{
	  //ask_more(ooo_next++);
      //ooo_next++;
	  ScheduleNextPacket();
	}
	GOT_HERE();
}

//gives what to do after an interest being timed-out:   rexpress the interest
void
ConsumerWindowRAAQM::OnTimeout(uint32_t sequenceNumber)
{

  NS_LOG_FUNCTION(sequenceNumber);
  
  //std::cerr << "TIMEOUT: last interest sent for segment #" << (ooo_next - 1) << std::endl;
  if(!cur_path)
  {
    //std::cout << "ERROR: when timed-out no current path found, exit" << std::endl;
    NS_LOG_ERROR("when timed-out no current path found, exit");
    //exit(1);
    StopApplication();
    return;
  }

  unsigned slot=sequenceNumber%BUF_DIM;

  //check if data is received
  if(Window[slot].isDataReceived()) return;
  
  std::map<uint32_t, uint32_t>::iterator it=m_seqRetxCounts.find(sequenceNumber);
  
  if(m_reTxLimit!=0 && it!=m_seqRetxCounts.end() && it->second>=m_reTxLimit ) //limit is set and limit is reached
  {
    onReachRetxLimit(sequenceNumber);
    return;
  }
  
  //NOTE: update number of outstanding interest:
  winpending--;
  //winpendingTrace(GetId(),winpending);
  
  if(m_isAbortModeEnabled)
  {
    //update number of slots whose interests has been retransmitted.
    if(it!=m_seqRetxCounts.end() && it->second==1)
      winretrans++;
  
    if(winretrans>=curwindow)//we need to abort
    {
      NS_LOG_INFO("> full window of interests timedout, application aborted");
      StopApplication();
      return;
    }
  }
  NS_LOG_DEBUG("App="<<GetId()<<" : timeout on "<<static_cast<uint64_t>(sequenceNumber));
  //forward call to consumer class' OnTimeout() to resend the interest and do some statistics.
  Consumer::OnTimeout(sequenceNumber);
  last_timeout=Simulator::Now();
  nTimeouts++;
  //fprintf(stdout,"%6f ndn-icp-download: timeout on %ld\n", last_timeout.GetSeconds(), static_cast<uint64_t>(sequenceNumber));
  //TODO schedule a timeout after DEFAULT_INTEREST_TIMEOUT and bound to this function itself
}

void
ConsumerWindowRAAQM::insertToPathTable(std::string key,shared_ptr<Path> path)
{
  if(PathTable.find(key)==PathTable.end())
  {
    PathTable[key]=path;
  }
  else
  {
    //std::cout<<"ERROR:failed to insert path to path table, the path entry already exists\n";
    NS_LOG_ERROR("ERROR:failed to insert path to path table, the path entry already exists");
  }
}

void
ConsumerWindowRAAQM::setStartTimetoNow()
{
	start_tv=Simulator::Now();
}

void
ConsumerWindowRAAQM::setLastTimeOutoNow()
{
	last_timeout=Simulator::Now();
}

void
ConsumerWindowRAAQM::setMaxWindow(unsigned int NewMaxWindow)
{
  if(NewMaxWindow<128 && NewMaxWindow>0)
    maxwindow=NewMaxWindow;
    
}

InFlightInterests::InFlightInterests()
  : raw_data_size(0)
  , packet_size(0)
  //, retrans(false)
  ,isReceived(false)
{
}

const Time&
InFlightInterests::getSendTime()
{
	return send_time;
}

void
InFlightInterests::setSendTime(const Time& now)
{
	send_time=now;
}

std::size_t
InFlightInterests::getRawDataSize()
{
	return raw_data_size;
}

std::size_t
InFlightInterests::getPacketSize()
{
	return packet_size;
}

void
InFlightInterests::markAsReceived()
{
  isReceived=true;
}

bool
InFlightInterests::isDataReceived()
{
  return isReceived;
}

void
InFlightInterests::addRawData(const Data& data)
{
	const Block& content = data.getContent();
	raw_data_size = content.value_size();//or I need to assign content.value_size()+1?
	packet_size = data.wireEncode().size();//or I need to assign data.wireEncode().size() + 1 ?
	isReceived=true;
 }

void
InFlightInterests::removeRawData()
{
	raw_data_size = 0;
	packet_size = 0;
	isReceived=false;
}

Path::Path(double dropFactor
  ,double Pmin
  ,unsigned newTimer
  ,unsigned int ESTLEN
  ,unsigned NewRtt
  ,unsigned NewRttMin
  ,unsigned NewRttMax
  )
:m_drop_factor(dropFactor)
  ,m_P_MIN(Pmin)
  ,timer(newTimer)
  ,m_EST_LEN(ESTLEN)
  ,rtt(NewRtt)
  ,rtt_min(NewRttMin)
  ,rtt_max(NewRttMax)

  
  ,drop_prob(0)
  ,pkts_recvd(0)
  ,last_pkts_recvd(0)
  ,pkts_bytes_recvd(0)  	   /** Total number of bytes received including ccnxheader*/ 
  ,last_pkts_bytes_recvd(0)  	   /** pkts_bytes_recvd at the last path summary computation */ 
  ,raw_data_bytes_recvd(0)  	   /** Total number of raw data bytes received (excluding ccnxheader)*/ 
  ,last_raw_data_bytes_recvd(0)  
{
  last_rate_cal=Simulator::Now();
}

//print path status
void
Path::path_reporter()
{
	Time now;
	now=Simulator::Now();
	double rate, delta_t;

	delta_t = now.GetMicroSeconds()-last_rate_cal.GetMicroSeconds();
	rate = (pkts_bytes_recvd - last_pkts_bytes_recvd)*8 / delta_t; // MB/s
	NS_LOG_INFO(std::fixed<<std::setprecision(6)
			<<"path status report:"
			<<"at time "<<now.GetSeconds()<<"sec ndn-icp-download:"
			<<" "<<(void*) this<<" path , pkts_recvd "<<(pkts_recvd-last_pkts_recvd)<<", delta_t "<<delta_t<<", rate "<<rate<<" Mbps, "
			<<"RTT "<<rtt<<", RTT_max "<<rtt_max<<", RTT_min "<<rtt_min
			);
	last_pkts_recvd = pkts_recvd;
	last_pkts_bytes_recvd = pkts_bytes_recvd;
	last_rate_cal=Simulator::Now();
 }

//add a new RTT to the RTT queue of the path, check if RTT queue is full, and thus need overwrite.
//also it maintains the vailidity of min and max of RTT.
void
Path::addRTT(unsigned newRTT)
{
	// std::cout <<"RTT added\n";
	rtt = newRTT;
	sam_queue.get<byArrival>().push_back(newRTT);
	if(sam_queue.get<byArrival>().size()>m_EST_LEN)
		sam_queue.get<byArrival>().pop_front();
	rtt_max=*(sam_queue.get<byOrder>().rbegin());
	rtt_min=*(sam_queue.get<byOrder>().begin());
}

//update received bytes
void
Path::update_received_stats(std::size_t paketSize, std::size_t dataSize)
{
	pkts_recvd++;
	pkts_bytes_recvd += paketSize;
	raw_data_bytes_recvd += dataSize;
}

double
Path::getMyDropFactor()
{
	return m_drop_factor;
}

double
Path::getDropProb()
{
	return drop_prob;
}

void
Path::setDropProb(double dropProb)
{
	drop_prob = dropProb;
}

double
Path::getMyPmin()
{
	return m_P_MIN;
}

unsigned
Path::getTimer()
{
	return timer;
}

void
Path::smoothTimer()
{
	timer = (1 - TIMEOUT_SMOOTHER)* timer + (TIMEOUT_SMOOTHER) * rtt * (TIMEOUT_RATIO);
}

unsigned
Path::getRTT()
{
	return rtt;
}

unsigned
Path::getRTTMax()
{
  return rtt_max;
}

unsigned
Path::getRTTMin()
{
	return rtt_min;
}

unsigned
Path::getMyEstLen()
{
	return m_EST_LEN;
}

unsigned
Path::getRTTQueueSize()
{
	return sam_queue.get<byArrival>().size();
}

/* unnecessary for the moment
bool
Path::isRTTQueueFull()
{
	return (sam_queue.size()==m_EST_LEN);
}
*/

//change drop probability according to RTT statistics
//invoked in RAQM(), before control window size update.
void
Path::resetDropProb()
{
	drop_prob=0.0;
	if (getMyEstLen()==getRTTQueueSize())
	{
		if(rtt_max==rtt_min)
		{
			drop_prob=m_P_MIN;
		}
		else
		{
			drop_prob=m_P_MIN + m_drop_factor*(rtt -rtt_min)/(rtt_max-rtt_min);
		}
	}
}

} /* namespace ndn */
} /* namespace ns3 */

#endif // RAAQM
