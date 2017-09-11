/*
 * ndn-consumer-window-raaqm.cpp
 *
 *  Created on: Mar 18, 2015
 *      Author: Natalya Rozhnova
 *              Zeng Xuan
 */

//#ifndef PATH_LABELLING
//#error "RAAQM requires PATH_LABELLING"
//#else

#ifdef RAAQM

#include "ndn-consumer-raaqm.hpp"

#include <sys/time.h>
#include <assert.h>
//#include <time.h>
#include <stdlib.h>
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

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/rtt-tag.hpp"

#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>

#include <iomanip>
#include <fstream>
#include <iostream>

#include "model/ndn-l3-protocol.hpp"

#define PATHID
NS_LOG_COMPONENT_DEFINE("ndn.ConsumerRAAQM");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerRAAQM);

TypeId
ConsumerRAAQM::GetTypeId(void)
{
	static TypeId tid =
			TypeId("ns3::ndn::ConsumerRAAQM")
			.SetGroupName("Ndn")
			.SetParent<Consumer>()
			.AddConstructor<ConsumerRAAQM>()
			.AddAttribute("AllowStale", "Allow to receive staled/expired data",
						   BooleanValue(ALLOW_STALE),
						   MakeBooleanAccessor(&ConsumerRAAQM::m_allow_stale),
						   MakeBooleanChecker())

			.AddAttribute("MaxWindowSize", "Maximum allowed window size in the download process, should be less than buffer size (RECEPTION_BUFFER)",
						   IntegerValue(MAX_WINDOW),
						   MakeIntegerAccessor(&ConsumerRAAQM::SetMaxWindow),
						   MakeIntegerChecker<int32_t>())

			.AddAttribute("DefaultInterestLifeTime", "Default interest LifeTime for each interest (an override). It is also the retransmission timer value.",
		    			   StringValue(INTEREST_LIFETIME),
		        		   MakeTimeAccessor(&ConsumerRAAQM::m_defaultInterestLifeTime),
		        		   MakeTimeChecker())

			.AddAttribute("LookupInterval","To check for expired Interests.",
		                   StringValue("50ms"),
		                   MakeTimeAccessor(&ConsumerRAAQM::GetLookupInterval, &ConsumerRAAQM::SetLookupIntervalForExpiredInterests),
		                   MakeTimeChecker())

			.AddAttribute("DropFactor", "the drop factor used to compute window size drop probability",
				      	   StringValue(DROP_FACTOR_STRING),
				      	   MakeDoubleAccessor(&ConsumerRAAQM::m_dropFactor),
				      	   MakeDoubleChecker<double>())

			.AddAttribute("Pmin", "minimum drop probability used to compute window size drop probability",
				      	   StringValue(P_MIN_STRING),
				      	   MakeDoubleAccessor(&ConsumerRAAQM::m_Pmin),
				      	   MakeDoubleChecker<double>())

			.AddAttribute("Beta", "Multiplifier for window size decrease, when necessary to decrease the window size. It should be between 0 and 1",
				      	   StringValue(BETA_STRING),
				      	   MakeDoubleAccessor(&ConsumerRAAQM::m_BETA),
				      	   MakeDoubleChecker<double>())

			.AddAttribute("Gamma", "the increment that is added to the window size, when necessary to increase window size. It should be a positvie integer",
				      	   IntegerValue(GAMMA),
				      	   MakeIntegerAccessor(&ConsumerRAAQM::m_GAMMA),
				      	   MakeIntegerChecker<int32_t>())

			.AddAttribute("NumberOfRTTSamples", "The maximum rtt queue size for each individual path",
						   IntegerValue(SAMPLES),
						   MakeIntegerAccessor(&ConsumerRAAQM::m_Samples),
						   MakeIntegerChecker<int32_t>())

			.AddAttribute("MaxSeq", "Maximum sequence number to request",
				      	   IntegerValue(MAX_SEQUENCE_NUMBER),
				      	   MakeIntegerAccessor(&ConsumerRAAQM::m_seqMax),
				      	   MakeIntegerChecker<uint32_t>())

			.AddAttribute("EnableOutput", "to output the content each time a data packet is received",
				       	   BooleanValue(false),
				       	   MakeBooleanAccessor(&ConsumerRAAQM::m_isOutputEnabled),
				       	   MakeBooleanChecker())

			.AddAttribute("EnablePathReport", "to ouput all the paths and statistics on each path",
				       	   BooleanValue(false),
				       	   MakeBooleanAccessor(&ConsumerRAAQM::m_isPathReportEnabled),
				       	   MakeBooleanChecker())

			.AddAttribute("EnableAbortMode", "when a full window of interest is timedout, stop the application",
						   BooleanValue(false),
				           MakeBooleanAccessor(&ConsumerRAAQM::m_isAbortModeEnabled),
				           MakeBooleanChecker())

			.AddAttribute("RetrxLimit", "set retransmission limit, such that when the limit is reached, the application will mark the data as received and move on, must be >0. RetrxLimit=n means maximum n-1 retransmissions are allowed",
						   StringValue("0"),
				           MakeUintegerAccessor(&ConsumerRAAQM::m_reTxLimit),
				           MakeUintegerChecker<uint32_t>())

			.AddAttribute("SeedValue", "Set a seed value for random window drop",
						   UintegerValue(30102015),
						   MakeUintegerAccessor(&ConsumerRAAQM::m_seed),
						   MakeUintegerChecker<unsigned int>())

			.AddAttribute("InitialWindow", "Set initial window size",
						   DoubleValue(1.0),
						   MakeDoubleAccessor(&ConsumerRAAQM::m_curwindow),
						   MakeDoubleChecker<double>())

			.AddAttribute("UseRTTreduction", "Activate RTT reduction mechanism",
						   BooleanValue(false),
						   MakeBooleanAccessor(&ConsumerRAAQM::m_rtt_red),
						   MakeBooleanChecker())

			.AddAttribute("AlphaHistory", "[Tracing] Alpha for moving average",
						   DoubleValue(0.7),
						   MakeDoubleAccessor(&ConsumerRAAQM::m_alpha_history),
						   MakeDoubleChecker<double>())

			.AddAttribute("ObservationPeriod", "[Tracing] Set observation period for tracing",
						   StringValue("1.0"),
						   MakeDoubleAccessor(&ConsumerRAAQM::m_observation_period),
						   MakeDoubleChecker<double>())

			.AddAttribute("EnableTracing", "[Tracing] Enable tracing",
						   BooleanValue(false),
						   MakeBooleanAccessor(&ConsumerRAAQM::m_enable_tracing),
						   MakeBooleanChecker())

			.AddAttribute("PathForTrace", "[Tracing] The path to a folder where you want to keep your trace, default [.]",
						   StringValue("."), MakeStringAccessor(&ConsumerRAAQM::m_filepath),
						   MakeStringChecker())
/*
			.AddAttribute("TraceFileName", "The name of trace file",
						   StringValue("none"), MakeStringAccessor(&ConsumerRAAQM::m_filename),
						   MakeStringChecker())
*/

			.AddTraceSource("WindowTrace", "[Tracing] Window trace file name, by default window traces are not logged, if you don't specify a valid file name",
		                    MakeTraceSourceAccessor(&ConsumerRAAQM::WindowTrace))

			.AddTraceSource("FlowStatsTrace", "[Tracing] Flow stats trace file name, by default flow stats traces are not logged, if you don't specify a valid file name",
							MakeTraceSourceAccessor(&ConsumerRAAQM::FlowStatsTrace))
#ifdef MLDR
			.AddTraceSource("InterestSatisfactionTime", "[Tracing] Get the Interest satisfaction time",
							MakeTraceSourceAccessor(&ConsumerRAAQM::m_satisfactionTime))
			.AddAttribute("EnableEln", "Enable Eln activity",
						   BooleanValue(false),
						   MakeBooleanAccessor(&ConsumerRAAQM::m_eln_activated),
						   MakeBooleanChecker())
#endif
							;

 return tid;
}

ConsumerRAAQM::ConsumerRAAQM()
    : m_maxwindow(MAX_WINDOW)
    , m_dropFactor(DROP_FACTOR)
    , m_Pmin(P_MIN)
    , m_Samples(SAMPLES)
    , m_GAMMA(GAMMA)
    , m_BETA(BETA)
    , m_allow_stale(ALLOW_STALE)
    , m_isOutputEnabled(false)		//by default dont print received data content
    , m_isPathReportEnabled(false)	//by default dont print path status
    , m_winrecvd(0)
    , m_winpending(0)
//    , m_curwindow(1)				//by default current window is initialized to 1 on start
    , m_firstInFlight(0)
    , m_ooo_count(0)
    , m_finalslot(~0)				//invalid number
    , m_interests_sent(0)
    , m_pkts_recvd(0)
    , m_pkts_bytes_recvd(0)
    , m_raw_data_bytes_recvd(0)		//size_t m_totalSize;
    , m_pkts_delivered(0)
    , m_raw_data_delivered_bytes(0)
    , m_pkts_delivered_bytes(0)
    , m_nTimeouts(0)				//number of interest timeout events
    , m_isAbortModeEnabled(false)
    , m_winretrans(0)
    , m_reTxLimit(0)
    , m_nHoles(0)
	, m_stop_tracing(false)
	, m_old_npackets(0)
    , m_old_rate(0.0)
	, m_old_interests(0.0)
	, m_old_data_inpackets(0)
	, m_interests_inpackets(0)
	, m_old_interests_inpackets(0)
	, m_isSending(false)
    , m_isRetransmission(false)
	, m_RawDataReceived(0)
#ifdef MLDR
	, m_eln_activated(false)
#endif
{
  m_seqMax=MAX_SEQUENCE_NUMBER;
}

ConsumerRAAQM::~ConsumerRAAQM() {
	Simulator::Remove(m_newRetxEvent);
}

void
ConsumerRAAQM::StartApplication()
{
	//m_seed = std::time(NULL);
	srand(m_seed);
	Simulator::Remove(m_retxEvent);
	//set the interest timeout checking
	SetLookupIntervalForExpiredInterests(m_lookupInterval);

	shared_ptr<DataPath> initPath = make_shared<DataPath>(m_dropFactor, m_Pmin, m_defaultInterestLifeTime.GetMicroSeconds(), m_Samples);
	InsertToPathTable(DEFAULT_PATH_ID, initPath);
	SetCurrentPath(initPath);
	SetStartTimeToNow();
	m_start_period = Simulator::Now(); // for tracing
	SetLastTimeoutToNow();

	Consumer::StartApplication();

	if(this->m_enable_tracing)
		SetTraceFileName(m_filepath, "ConsumerRAAQM.plotme");

    NS_LOG_INFO("App=" <<GetId()<<", prefix="<<m_interestName);

}

void
ConsumerRAAQM::ReStart(std::string nextPrefix)
{
	reInitialize();
	m_interestName = Name(nextPrefix);
	shared_ptr<DataPath> initPath = make_shared<DataPath>(m_dropFactor, m_Pmin, m_defaultInterestLifeTime.GetMicroSeconds(), m_Samples);
	InsertToPathTable(DEFAULT_PATH_ID,initPath);
	SetCurrentPath(initPath);
	SetStartTimeToNow();
	SetLastTimeoutToNow();

	NS_ASSERT(m_active != true);
	m_active = true;

	NS_ASSERT_MSG(GetNode()->GetObject<L3Protocol>() != 0,
                "Ndn stack should be installed on the node " << GetNode());

	ScheduleNextPacket();

	NS_LOG_INFO("restarted, App=" <<GetId()<<", prefix="<<m_interestName);
}

void
ConsumerRAAQM::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  PrintSummary();
  ControlStatsReporter();

  WriteConsumerTrace(false); //if you want to see the last line
  m_stop_tracing = true;

#ifdef MLDR
  m_tsMap.clear();
#endif

  //cleanup memory:
  Simulator::Remove(m_newRetxEvent);
  m_PathTable.clear();
  m_curr_path=NULL;

  if(OnStop.IsNull())
  {
    Consumer::StopApplication();
  }
  else
  {
     Simulator::Cancel(m_sendEvent);
     if (!m_active)
        return; // don't assert here, just return

     m_active = false;
     OnStop(this);
  }
}

void
ConsumerRAAQM::reInitialize()
{
	m_seq = 0;
	m_winrecvd = 0;
	m_winpending = 0;
	m_curwindow = 1.0;			// by default current window is initialized to 1 on start
	m_firstInFlight = 0;
	m_ooo_count = 0;
	m_finalslot = ~0;
	m_interests_sent = 0;
	m_pkts_recvd = 0;
	m_pkts_bytes_recvd = 0;
	m_raw_data_bytes_recvd = 0;
	m_RawDataReceived = 0;
	m_pkts_delivered = 0;
	m_raw_data_delivered_bytes = 0;
	m_pkts_delivered_bytes = 0;
	m_nTimeouts = 0;
	m_winretrans = 0;
	m_nHoles = 0;
	SetLookupIntervalForExpiredInterests(m_lookupInterval);
	m_isSending = false;
    m_isRetransmission = false;

	for(int i=0; i < RECEPTION_BUFFER; i++)
	{
	  m_inFlightInterests[i] = PendingInterests();
	}
#ifdef MLDR
	m_RawDataReceived = 0;
#endif
}

//void
//ConsumerRAAQM::SetStopcallBack(ConsumerElasticTraffic* MyElasticTraffic)
//{
//	if(MyElasticTraffic==NULL)
//		return;
//	OnStop = MakeCallback (&ConsumerElasticTraffic::OnDownloadStopRAAQM, MyElasticTraffic);
//}

void
ConsumerRAAQM::ScheduleNextPacket()
{
	if( (m_winpending < (unsigned)m_curwindow) && (m_seq < m_seqMax) && !m_isSending)
	{
		m_isRetransmission = false;
		SendPacket();
	}
	else if(m_seq >= m_seqMax && m_isRetransmission && !m_isSending)
	{
		m_isRetransmission = false;
		SendPacket();
	}
	else
	{
//		std::cout<<"Can't send anymore, m_isSending = "<<m_isSending<<std::endl;
	}
}

void
ConsumerRAAQM::SendPacket()
{
	m_isSending = true;

	if (!m_active)
		return;

	uint64_t seq;
	unsigned slot;

	if(m_retxSeqs.size() > 0) //if timed out interest is in the queue, it is sent in priority
	{
		seq = *m_retxSeqs.begin();
		m_retxSeqs.erase(m_retxSeqs.begin());
		slot = seq % RECEPTION_BUFFER;
		NS_LOG_DEBUG("App="<<GetId()<<" > retransmit interest for " << seq);
		RTXset::iterator sit(this->m_ListOfRetransmitted.find(seq));
		if(sit == this->m_ListOfRetransmitted.end())
		{
			this->m_ListOfRetransmitted.insert(seq);
		}
	}
	else	//generate a new interest
	{
		seq = m_seq++;
		slot = seq % RECEPTION_BUFFER;
		NS_LOG_DEBUG("App= "<<GetId()<<" > interest for " << seq);
#ifdef MLDR
		m_tsMap[seq] = Simulator::Now().GetMicroSeconds();
#endif
	}

	NS_ASSERT(!m_inFlightInterests[slot].CheckIfDataIsReceived());
	m_inFlightInterests[slot].SetSendTime(Simulator::Now());

	shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
	nameWithSequence->appendSequenceNumber(seq);

	//make interest:
	shared_ptr<Interest> interest = make_shared<Interest>();
	interest->setName(*nameWithSequence);
	time::milliseconds interestLifeTime(m_defaultInterestLifeTime.GetMilliSeconds());
	interest->setInterestLifetime(interestLifeTime);
	interest->setMustBeFresh(m_allow_stale);
	interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
	
	//std::cout << "send int " << interest->getName() << std::endl;

#ifdef KITE_ENABLED
	//***********to set tracing name, such that it could reach the producer**********************
	interest->setTraceName(Name("/anchor/tracingInterest"));
	//traceonly:
	interest->setTraceForwardingFlag(2);
#endif

	//log statistics
	WillSendOutInterest(seq);
	m_transmittedInterests(interest, this, m_face);

	// Practically send Interest
	m_face->onReceiveInterest(*interest);

	m_interests_inpackets++;
	m_interests_sent++;

	//update number of interests in flight
	m_winpending++;

	NS_ASSERT(m_ooo_count < RECEPTION_BUFFER);

	m_isSending = false;

	ScheduleNextPacket();
}


void
ConsumerRAAQM::SetCurrentPath(shared_ptr<DataPath> path)
{
	m_curr_path=path;
}

void
ConsumerRAAQM::UpdateRTT(unsigned slot)
{
	if(!m_curr_path)
	{
		NS_LOG_ERROR("no current path found, exit");
		StopApplication();
		return;
	}
	else
	{
		uint64_t rtt;
		const Time& sendTime = m_inFlightInterests[slot].GetSendTime();
		rtt = Simulator::Now().GetMicroSeconds()-sendTime.GetMicroSeconds();
		m_curr_path->InsertNewRTT(rtt);
	}
}

void
ConsumerRAAQM::IncreaseWindow()
{
	if (m_winrecvd == (unsigned)m_curwindow)
	{
		if ((unsigned)m_curwindow < m_maxwindow)
			m_curwindow += (double)m_GAMMA;

		m_winrecvd = 0;
    }
}

void
ConsumerRAAQM::RAQM()
{
	if(!m_curr_path)
	{
		NS_LOG_ERROR("no current path found, exit");
		StopApplication();
		return;

	}
	else
	{
		//change drop probability according to RTT statistics
		m_curr_path->UpdateDropProb();

		if (rand() % 10000 <= m_curr_path->GetDropProb() * 10000)
		{
			m_curwindow = m_curwindow * m_BETA;

			if (m_curwindow < 1)
				m_curwindow = 1;

			m_winrecvd = 0;
		}
	}
}

void
ConsumerRAAQM::AfterDataReceptionWithReducedRTT(unsigned slot, uint64_t rtt_val)
{
	m_winpending--;
	m_winrecvd++;

	IncreaseWindow();
	if(!m_rtt_red)
		rtt_val = 0;
	UpdateRTTWithoutWireless(slot, rtt_val);
	RAQM();

	//optional to report path stats here
	//m_curr_path->PathReporter();
	//optional to report control stats
	//ControlStatsReporter();
	WindowTrace(GetId(), m_curwindow, m_winpending);

}

void
ConsumerRAAQM::UpdateRTTWithoutWireless(unsigned slot, uint64_t rtt_val)
{
	if(!m_curr_path)
	{
		NS_LOG_ERROR("no current path found, exit");
		StopApplication();
		return;
	}
	else
	{
		uint64_t rtt;
		const Time& sendTime = m_inFlightInterests[slot].GetSendTime();
		rtt = Simulator::Now().GetMicroSeconds()-sendTime.GetMicroSeconds() - rtt_val;

		m_curr_path->InsertNewRTT(rtt);
	}
}

// Used for expired packets
void
ConsumerRAAQM::AfterVirtualDataReception( unsigned slot)
{
	m_winpending--;
	m_winrecvd++;

//	IncreaseWindow();

//	UpdateRTT(slot);
//	RAQM();

	WindowTrace(GetId(), m_curwindow, m_winpending);
}


void
ConsumerRAAQM::OnReachRetxLimit(uint32_t seq)
{
	m_nHoles++;
	//Suppose that the packet has been received
	unsigned slot = seq % RECEPTION_BUFFER;
	NS_LOG_DEBUG("App=" << GetId()<<" > interest for " << seq << ", retxLimit reached");

	if(m_isAbortModeEnabled)
	{
		std::map<uint32_t, uint32_t>::iterator it = m_seqRetxCounts.find(seq);

		if(m_winretrans>0 && it!=m_seqRetxCounts.end() && it->second>=2)
			m_winretrans--;
	}

	// For tracing defined in Consumer class
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
	m_rtt->AckSeq(SequenceNumber32(seq)); // ???????????

	if(seq == m_seqMax-1)
	{
		m_finalslot = slot;
	}

	//whether expired packet is out of order
	if(slot != m_firstInFlight)
	{
		if (!m_inFlightInterests[slot].CheckIfDataIsReceived())
		{
			m_ooo_count++;
			m_inFlightInterests[slot].MarkAsReceived(true);
			AfterVirtualDataReception(slot);
		}
	}
	else // in order -> deliver to application all till hole
	{
		NS_ASSERT(!m_inFlightInterests[slot].CheckIfDataIsReceived());

		if (slot == m_finalslot)
		{
			AfterVirtualDataReception(slot);
			StopApplication();
			return;
		}

		AfterVirtualDataReception(slot);

		slot = (slot + 1) % RECEPTION_BUFFER;

		//Deliver to the application already received packets until there is a hole
		while ((m_ooo_count > 0) && (m_inFlightInterests[slot].CheckIfDataIsReceived()))
		{
			if(m_inFlightInterests[slot].GetRawDataSize() != 0)
			{
				m_pkts_delivered++;
				m_raw_data_delivered_bytes += m_inFlightInterests[slot].GetRawDataSize();
				m_pkts_delivered_bytes += m_inFlightInterests[slot].GetPacketSize();
			}
			if (slot == m_finalslot)
			{
				StopApplication();
				return;
			}

			m_inFlightInterests[slot].RemoveRawData();
			m_inFlightInterests[slot].MarkAsReceived(false);

			slot = (slot + 1) % RECEPTION_BUFFER;
			m_ooo_count--;
		}
		m_firstInFlight = slot;
	}

	// Why we send them here ??? Because the window is reduced now and we have a place to send a new packet
		ScheduleNextPacket();
}

void
ConsumerRAAQM::SetLookupIntervalForExpiredInterests(Time interval)
{
	m_lookupInterval = interval;

	if (m_newRetxEvent.IsRunning())
	{
		Simulator::Remove(m_newRetxEvent); // slower, but better for memory
	}

	m_newRetxEvent = Simulator::Schedule(m_lookupInterval, &ConsumerRAAQM::CheckRetxTimeout, this);
}

Time
ConsumerRAAQM::GetLookupInterval() const
{
	return m_lookupInterval;
}

void
ConsumerRAAQM::CheckRetxTimeout()
{

	Time now = Simulator::Now();
	Time rto = m_defaultInterestLifeTime;

	while (!m_seqTimeouts.empty())
	{
		SeqTimeoutsContainer::index<i_timestamp>::type::iterator
			entry = m_seqTimeouts.get<i_timestamp>().begin();

		if (entry->time + rto <= now) // timeout expired?
		{
			uint32_t seqNo = entry->seq;
			m_seqTimeouts.get<i_timestamp>().erase(entry);
			OnTimeout(seqNo);

			if(!m_active)
				break;
		}
		else
			break; // Next packets don't need to be retransmitted
	}

	if(m_active)
		m_newRetxEvent = Simulator::Schedule(m_lookupInterval, &ConsumerRAAQM::CheckRetxTimeout, this);
}


void
ConsumerRAAQM::OnData(shared_ptr<const Data> data)
{
	NS_LOG_FUNCTION_NOARGS();
	m_received_data += data->wireEncode().size();

	if(!m_active)
		return;

    uint64_t rtt_val = 0;
#ifdef MLDR
	rtt_val = data->getRttReduction();
#endif

	const Block& content = data->getContent();
	const Name& name = data->getName();
	uint64_t seq = name.at(-1).toSequenceNumber();

	size_t dataSize=content.value_size();
	RTXset::iterator sit(this->m_ListOfRetransmitted.find(seq));
		if(sit == this->m_ListOfRetransmitted.end())
		{
//			std::cout<<"This Data packet is not a retransmission, set size "<<this->m_ListOfRetransmitted.size()<<" m_retxSeqs size "<<m_retxSeqs.size()<<std::endl;
			m_RawDataReceived += dataSize;
		}
		else
		{
//			std::cout<<"This Data packet is a retransmission, don't count, set size "<<this->m_ListOfRetransmitted.size()<<" m_retxSeqs size "<<m_retxSeqs.size()<<std::endl;
			this->m_ListOfRetransmitted.erase(sit);
		}

#ifdef MLDR
		NackSet::iterator nack_sit(this->m_nackSet.find(seq));
		if(nack_sit != this->m_nackSet.end())
		{
			this->m_nackSet.erase(nack_sit);
		}

		TimestampMap::iterator ts_mit(m_tsMap.find(seq));
		if(ts_mit != m_tsMap.end())
		{
			uint64_t ts = ts_mit->second;
			uint64_t ist = Simulator::Now().GetMicroSeconds() - ts;
			// To trace only retransmitted
			if(rtt_val != 0)
				m_satisfactionTime(m_interestName, Simulator::Now().GetSeconds(), seq, ts, ist);
			m_tsMap.erase(ts_mit);
		}
#endif

	size_t packetSize=data->wireEncode().size();
	unsigned slot = seq % RECEPTION_BUFFER;

	Consumer::OnData(data);

	if(m_isAbortModeEnabled)
	{
		std::map<uint32_t, uint32_t>::iterator it=m_seqRetxCounts.find(seq);
		if(m_winretrans>0 && it!=m_seqRetxCounts.end() && it->second>=2)
			m_winretrans--;
	}
	// Tracing?
	if (m_isOutputEnabled)
	{
		NS_LOG_INFO( "data received, seq number #" << seq);
		std::cout.write(reinterpret_cast<const char*>(content.value()), content.value_size());
	}
	/*
	 * 1.get the pathID from packet content,
	 * 2.then find(or also store if a new path) it in a hash table
	 * 3.if it is a new path, initialize the path with default parameters
	 * 4. point m_curr_path to the found or created path
	 */

#ifdef PATHID
	ndn::Block pathIdBlock = data->getPathId();

	if(pathIdBlock.empty())
	{
		NS_LOG_ERROR("path ID lost in the transmission.");
		StopApplication();
		return;
	}
	unsigned char pathId = *(data->getPathId().value());

#else
	unsigned char pathId = 0;
#endif

	NS_LOG_DEBUG("App="<<GetId()<<" < DATA for " << seq<<", pathID="<<static_cast<int>(pathId));

	std::string pathIdSring(1, pathId);

	if(m_PathTable.find(pathIdSring) == m_PathTable.end())
	{
		if(m_curr_path) // ??? keep until craches if delete
		{
			//create a new path with some default param
			if(m_PathTable.empty())
			{
				NS_LOG_ERROR("no path initialized for path table, error could be in default path initialization.");
				StopApplication();
				return;
			}
			else
			{
				//initiate the new path default param
				shared_ptr<DataPath> newPath = make_shared<DataPath>(*(m_PathTable.at(DEFAULT_PATH_ID)));
				//insert the new path into hash table
				m_PathTable[pathIdSring] = newPath;
			}
		}
		else
		{
			NS_LOG_ERROR("when running,current path not found.");
			StopApplication();
			return;
		}
	}

	m_curr_path = m_PathTable[pathIdSring];
	//update measurements
	m_pkts_recvd++;
	m_pkts_bytes_recvd += packetSize; //data wont be encoded again since its already encoded before received

	//update measurements for path
	m_curr_path->UpdateReceivedStats(packetSize, dataSize);

	if(seq >= m_seqMax-1)
	{
		m_finalslot = slot;
	}

	if(slot != m_firstInFlight)	//out of order data
	{
		if (!m_inFlightInterests[slot].CheckIfDataIsReceived())
		{
			m_ooo_count++;
			m_inFlightInterests[slot].AddRawData(*data);
			m_inFlightInterests[slot].MarkAsReceived(true);
			AfterDataReceptionWithReducedRTT(slot, rtt_val);
		}
	}
	else //in oder data arrived
	{
		NS_ASSERT(!m_inFlightInterests[slot].CheckIfDataIsReceived());
		m_pkts_delivered++;
		m_raw_data_delivered_bytes += dataSize;
		m_pkts_delivered_bytes += packetSize;

		if (slot == m_finalslot)
		{
			StopApplication();
			return;
		}

		AfterDataReceptionWithReducedRTT(slot, rtt_val);

		slot = (slot + 1) % RECEPTION_BUFFER;

		// Deliver all data to the application until we meet a hole
		while ((m_ooo_count > 0) && (m_inFlightInterests[slot].CheckIfDataIsReceived()))
		{
			if(m_inFlightInterests[slot].GetRawDataSize()!=0)
			{
				m_pkts_delivered++;
				m_raw_data_delivered_bytes += m_inFlightInterests[slot].GetRawDataSize();
				m_pkts_delivered_bytes += m_inFlightInterests[slot].GetPacketSize();
			}
			if (slot == m_finalslot) // curr_seq == MaxSeq
			{
				StopApplication();
				return;
			}
			m_inFlightInterests[slot].RemoveRawData();
			m_inFlightInterests[slot].MarkAsReceived(false);

			slot = (slot + 1) % RECEPTION_BUFFER;
			m_ooo_count--;
		}
		m_firstInFlight = slot;
	}

		ScheduleNextPacket();
}
#ifdef MLDR
void
ConsumerRAAQM::OnInterest(shared_ptr<const Interest> interest)
{
	bool mobility = interest->get_MobilityLossFlag();
	std::cout<<"Consumer got a Nack "<<mobility<<std::endl;
	if(mobility)
	{
		uint64_t seq = interest->getName().at(-1).toSequenceNumber();
		NackSet::iterator sit(this->m_nackSet.find(seq));
		if(sit == this->m_nackSet.end())
		{
			this->m_nackSet.insert(seq);

			if(m_eln_activated)
			{
				m_seqTimeouts.erase(seq);
				OnTimeout(seq);
			}
		}
	}
//	if(m_eln_activated)
//		m_retxSeqs.insert(seq);
//do nothing for the moment
}
#endif

// If Interest is timed out, we immediately send it again
void
ConsumerRAAQM::OnTimeout(uint32_t sequenceNumber)
{
	NS_LOG_FUNCTION(sequenceNumber);

	if(!m_curr_path)
	{
		NS_LOG_ERROR("when timed-out no current path found, exit");
		StopApplication();
		return;
	}
	bool rtx_by_eln = false;
#ifdef MLDR
	NackSet::iterator sit(this->m_nackSet.find(sequenceNumber));
	if(sit == this->m_nackSet.end())
	{
#endif
		m_curwindow = m_curwindow * m_BETA;

		if(m_curwindow < 1)
			m_curwindow = 1;
#ifdef MLDR
	}
	else
	{
		// ==========================================================================
		this->m_nackSet.erase(sit);

		// XXX FOR ELN SUPPORT: to force a retransmission even if the limit is achieved. (e.g. in case where the rtx by timer are desactivated)
		if(m_eln_activated)
			rtx_by_eln = true;
		// ==========================================================================
	}
#endif

	unsigned slot = sequenceNumber%RECEPTION_BUFFER;

	//check if data is received
	if(m_inFlightInterests[slot].CheckIfDataIsReceived())
		return;

	std::map<uint32_t, uint32_t>::iterator it = m_seqRetxCounts.find(sequenceNumber);

	// Check whether we reached the retransmission limit
	if(m_reTxLimit != 0 && it != m_seqRetxCounts.end() && it->second >= m_reTxLimit && !rtx_by_eln)
	{
		OnReachRetxLimit(sequenceNumber);
		return;
	}

	// Decrease the window because the timeout happened
	m_winpending--;

	if(m_isAbortModeEnabled)
	{
		if(it != m_seqRetxCounts.end() && it->second == 1)
			m_winretrans++;

		if(m_winretrans >= m_curwindow)
		{
			NS_LOG_INFO("> full window of interests timed out, application aborted");
			StopApplication();
			return;
		}
	}
	NS_LOG_DEBUG("App="<<GetId()<<" : timeout on "<<static_cast<uint64_t>(sequenceNumber));

	//Retransmit this interest and do some statistics
	this->m_isRetransmission = true;
	Consumer::OnTimeout(sequenceNumber);

	SetLastTimeoutToNow();
	m_nTimeouts++;
}

void
ConsumerRAAQM::InsertToPathTable(std::string key, shared_ptr<DataPath> path)
{
	if(m_PathTable.find(key) == m_PathTable.end())
	{
		m_PathTable[key] = path;
	}
	else
	{
		NS_LOG_ERROR("ERROR:failed to insert path to path table, the path entry already exists");
	}
}

void
ConsumerRAAQM::SetStartTimeToNow()
{
	m_start_time = Simulator::Now();
}

void
ConsumerRAAQM::SetLastTimeoutToNow()
{
	m_last_timeout = Simulator::Now();
}

void
ConsumerRAAQM::SetMaxWindow(unsigned int NewMaxWindow)
{
	if(NewMaxWindow<128 && NewMaxWindow>0)
		m_maxwindow=NewMaxWindow;
}

PendingInterests::PendingInterests()
  : raw_data_size(0)
  , packet_size(0)
  ,isReceived(false)
{
}

const Time&
PendingInterests::GetSendTime()
{
	return send_time;
}

void
PendingInterests::SetSendTime(const Time& now)
{
	send_time = now;
}

std::size_t
PendingInterests::GetRawDataSize()
{
	return raw_data_size;
}

std::size_t
PendingInterests::GetPacketSize()
{
	return packet_size;
}

void
PendingInterests::MarkAsReceived(bool val)
{
  isReceived = val;
}

bool
PendingInterests::CheckIfDataIsReceived()
{
  return isReceived;
}

void
PendingInterests::AddRawData(const Data& data)
{
	const Block& content = data.getContent();
	raw_data_size = content.value_size();
	packet_size = data.wireEncode().size();
}

void
PendingInterests::RemoveRawData()
{
	raw_data_size = 0;
	packet_size = 0;
}

DataPath::DataPath(double dropFactor
				  ,double Pmin
				  ,unsigned newTimer
				  ,unsigned int Samples
				  ,uint64_t NewRtt
				  ,uint64_t NewRttMin
				  ,uint64_t NewRttMax)
  :m_drop_factor(dropFactor)
  ,m_P_MIN(Pmin)
  ,m_Samples(Samples)
  ,m_current_rtt(NewRtt)
  ,m_rtt_min(NewRttMin)
  ,m_rtt_max(NewRttMax)
  ,m_drop_prob(0)
  ,m_pkts_recvd(0)
  ,m_last_pkts_recvd(0)
  ,m_pkts_bytes_recvd(0)  	   /** Total number of bytes received including ccnxheader*/
  ,m_last_pkts_bytes_recvd(0)  	   /** m_pkts_bytes_recvd at the last path summary computation */
  ,m_raw_data_bytes_recvd(0)  	   /** Total number of raw data bytes received (excluding ccnxheader)*/
{
	m_PreviousCallOfPathReporter = Simulator::Now();
}

//print path status
void
DataPath::PathReporter()
{
	Time now = Simulator::Now();
	double rate, delta_t;

	delta_t = now.GetMicroSeconds() - m_PreviousCallOfPathReporter.GetMicroSeconds();
	rate = (m_pkts_bytes_recvd - m_last_pkts_bytes_recvd)*8 / delta_t; // MB/s

	NS_LOG_INFO(std::fixed<<std::setprecision(6)
			<<"path status report:"
			<<"at time "<<now.GetSeconds()
			<<"sec ndn-icp-download: "<<(void*)this
			<<" path , m_pkts_recvd "<<(m_pkts_recvd-m_last_pkts_recvd)
			<<", delta_t "<<delta_t
			<<", rate "<<rate<<" Mbps, "
			<<"RTT "<<m_current_rtt
			<<", RTT_max "<<m_rtt_max
			<<", RTT_min "<<m_rtt_min
			);

	m_last_pkts_recvd = m_pkts_recvd;
	m_last_pkts_bytes_recvd = m_pkts_bytes_recvd;
	m_PreviousCallOfPathReporter = Simulator::Now();
}

//add a new RTT to the RTT queue of the path, check if RTT queue is full, and thus need overwrite
//also it maintains the validity of min and max of RTT
void
DataPath::InsertNewRTT(uint64_t newRTT)
{
	m_current_rtt = newRTT;
	m_RTTSamples.get<byArrival>().push_back(newRTT);

	if(m_RTTSamples.get<byArrival>().size()>m_Samples)
		m_RTTSamples.get<byArrival>().pop_front();

	m_rtt_max=*(m_RTTSamples.get<byOrder>().rbegin());
	m_rtt_min=*(m_RTTSamples.get<byOrder>().begin());
}

void
DataPath::UpdateReceivedStats(std::size_t paketSize, std::size_t dataSize)
{
	m_pkts_recvd++;
	m_pkts_bytes_recvd += paketSize;
	m_raw_data_bytes_recvd += dataSize;
}

double
DataPath::GetMyDropFactor()
{
	return m_drop_factor;
}

double
DataPath::GetDropProb()
{
	return m_drop_prob;
}

void
DataPath::SetDropProb(double dropProb)
{
	m_drop_prob = dropProb;
}

double
DataPath::GetMyPmin()
{
	return m_P_MIN;
}

uint64_t
DataPath::GetRTT()
{
	return m_current_rtt;
}

uint64_t
DataPath::GetRTTMax()
{
	return m_rtt_max;
}

uint64_t
DataPath::GetRTTMin()
{
	return m_rtt_min;
}

unsigned
DataPath::GetMySampleValue()
{
	return m_Samples;
}

unsigned
DataPath::GetRTTQueueSize()
{
	return m_RTTSamples.get<byArrival>().size();
}

void
DataPath::UpdateDropProb()
{
	m_drop_prob = 0.0;

	if (GetMySampleValue() == GetRTTQueueSize())
	{
		if(m_rtt_max == m_rtt_min)
		{
			m_drop_prob = m_P_MIN;
		}
		else
		{
			m_drop_prob = m_P_MIN + m_drop_factor*(m_current_rtt - m_rtt_min)/(m_rtt_max - m_rtt_min);
		}
	}
}

void
ConsumerRAAQM::PrintSummary()
{
	const char *expid;
	const char *dlm = " ";
	expid = getenv("NDN_EXPERIMENT_ID");

	if (expid == NULL)
		expid = dlm = "";

	double elapsed = 0.0;
	double rate = 0.0;
	double goodput = 0.0;

	m_stop_time=Simulator::Now();
	elapsed = m_stop_time.GetSeconds()-m_start_time.GetSeconds();

	if (elapsed > 0.00001)
	{
		rate = m_pkts_delivered_bytes*8/elapsed/1000000;
		goodput = m_raw_data_delivered_bytes*8/elapsed/1000000;
	}

	NS_LOG_INFO(std::fixed << std::setprecision(6)
	<<m_stop_time.GetSeconds() <<" ndn-icp-download["
	<<GetId()
	<<"], prefix=" << m_interestName
	<<" : " << expid << dlm << m_pkts_delivered_bytes
	<<" bytes transferred (filesize: "<<m_raw_data_delivered_bytes
	<<" ) in " << elapsed << " " << "seconds"
	<<" rate: " << rate
	<<" goodput: " << goodput << " Mbps"
	<<" t/o:" << m_nTimeouts
	<<" missing chunks: " << m_nHoles
	<<" missRatio: " << m_nHoles/(double)(m_pkts_delivered)
	);

	// Write flow statistics
	flow_statistics_t fs;
	fs.end_time = m_stop_time.GetSeconds();
	fs.id = GetId();
	fs.interest_name = m_interestName.toUri();
	fs.pkts_delivered = m_pkts_delivered;
	fs.pkts_delivered_bytes = m_pkts_delivered_bytes;
	fs.raw_data_delivered_bytes = m_raw_data_delivered_bytes;
	fs.elapsed = elapsed;
	fs.rate = rate;
	fs.goodput = goodput;
	fs.num_timeouts = m_nTimeouts;
	fs.num_holes = m_nHoles;

	FlowStatsTrace(&fs);
	if(m_isPathReportEnabled)
	{
		//print number of paths in the transmission process,
		//we need to exclude the default path
		NS_LOG_INFO("number of paths in the path table is(exclude the default path): "
				<<(m_PathTable.size()-1));

		int i=0;

		BOOST_FOREACH(HashTableForPath::value_type kv, m_PathTable)
		{
			if(kv.first.length()<=1)
			{
				i++;
				NS_LOG_INFO("[path "<< i <<"]\n"
						<<"ID : " << (int) *(reinterpret_cast<const unsigned char*>(kv.first.data())));

				kv.second->PathReporter();
			}
		}
	}
}

void
ConsumerRAAQM::ControlStatsReporter()
{
	Time now=Simulator::Now();
	NS_LOG_INFO(std::fixed<<std::setprecision(6)
						  <<"[control stats report]:"
						  <<"at time "<<now.GetSeconds()
						  <<"sec ndn-icp-download: "
						  <<"interests sent "<<m_interests_sent
						  <<", recvd "<<m_pkts_recvd
						  <<", t/o "<<m_nTimeouts<<", "
						  <<"currwin "<<m_curwindow
						  <<", m_winpending "<<m_winpending
						  <<", recvd "<<m_winrecvd
				);
}

void ConsumerRAAQM::SetTraceFileName(std::string path, std::string name)
{
	  char ss[20];
	  sprintf(ss,"%d", this->GetNode()->GetId());
	  name.insert (name.find("."), ss);
	  m_filePlot << path <<"/"<< name;// <<"_"<<this->GetId();
	  remove (m_filePlot.str ().c_str ());
	  WriteConsumerTrace(true);
}

void
ConsumerRAAQM::WriteConsumerTrace(bool isInitialize)
{
	if(!isInitialize && !m_stop_tracing)
	{
		double av_rate = 0.0,
				goodput = 0.0,
				lower_rate = 0.0;

		double total_elapsed = Simulator::Now().GetSeconds() - m_start_time.GetSeconds();//m_start_time.GetSeconds();

		goodput = this->m_RawDataReceived*8/total_elapsed/1000000;		// in Mb/s
		lower_rate = m_pkts_bytes_recvd*8/total_elapsed/1000000;
		av_rate = m_pkts_delivered_bytes*8.0/total_elapsed/1000000;	// in Mb/s

		std::ofstream fPlot (m_filePlot.str ().c_str (), std::ios::out|std::ios::app);
		fPlot << Simulator::Now ().GetSeconds ()
 										  <<" Name(2) "<<m_interestName.toUri()
 										  <<" -Instantrate(4) " << m_received_data*(1.0/m_observation_period)*8.0/1000000.0
 										  <<" -ADR(6) "<<av_rate //Average Data Rate in Mb/s
 										  <<" -ReceivedData(8) " << m_received_data
 										  <<" -TotD(10) "<<m_pkts_delivered_bytes	// Total number of delivered Chunks in Bytes
 										  <<" -TotDP(12) "<<m_pkts_delivered		// Total number of delivered Chunks in packets
 										  <<" -TotIP(14) "<<m_interests_inpackets	// Total number of sent interests in packets
 										  <<" -NTO(16) "<<m_nTimeouts	// Number of timeouts
 										  <<" -H(18) "<<m_nHoles // Missing chunks
 										  <<" -CW(20) "<<m_curwindow	// Current window
 										  <<" -WP(22) "<<m_winpending // Pending window
 										  <<" -LSeq(24) "<<m_seq	// The sequence number of the last sent interest
 										  <<" -GPut(26) "<<goodput
 										  <<" -LR(28) "<<lower_rate
 										  << std::endl;
		fPlot.close ();
	}

	m_received_data = 0;

	if(!m_stop_tracing)
	{
		m_start_period = Simulator::Now();
		Simulator::Schedule (Seconds (m_observation_period), &ConsumerRAAQM::WriteConsumerTrace, this, false);
		return;
	}
	else if(m_stop_tracing)
	{
		/*		std::ofstream fPlot (m_filePlot.str ().c_str (), std::ios::out|std::ios::app);
		fPlot<<"STOP TRACING"<<std::endl;
		fPlot.close();
		 */		return;
	}
}

} /* namespace ndn */
} /* namespace ns3 */
//#endif

#endif // RAAQM
