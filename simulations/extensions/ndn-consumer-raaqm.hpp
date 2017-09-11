/*
 * ndn-consumer-window-raaqm.hpp
 *
 *  Created on: Oct 15, 2015
 *      Author: Natalya Rozhnova
 *      		Zeng Xuan
 *
 */

#ifndef SRC_NDNSIM_APPS_NDN_CONSUMER_RAAQM_HPP_
#define SRC_NDNSIM_APPS_NDN_CONSUMER_RAAQM_HPP_

#ifdef RAAQM

#include <sys/time.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <ctime>
#include <unistd.h>

//NOTE: include ndn-xx/face.hpp header file before boost header files to avoid bugs in NDN release >=0.3.0
#include <ndn-cxx/face.hpp>
#include <queue>
#include <boost/iterator/iterator_concepts.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/identity.hpp>
#include<boost/unordered_map.hpp>
#include<string>
//#include <ndn-cxx/face.hpp>
#include <boost/foreach.hpp>


#include "ns3/ndnSIM/apps/ndn-consumer.hpp"

// We have to include this struct here before the circular inclusion of
// ndn-consumer-elastic-traffic.hpp
namespace ns3 {
namespace ndn {
typedef struct {
    double end_time;
    std::string id;
    std::string interest_name;
    intmax_t pkts_delivered;
    intmax_t pkts_delivered_bytes;
    intmax_t raw_data_delivered_bytes;
    double elapsed;
    double rate;
    double goodput;
    intmax_t num_timeouts;
    intmax_t num_holes;
} flow_statistics_t;
}
}

//#include "ns3/ndnSIM/apps/ndn-consumer-elastic-traffic.hpp"

//---Constants and defaults---------------------------------------------

#define RECEPTION_BUFFER 128000 // in packets
#define DEFAULT_PATH_ID "default_path"
#define MAX_WINDOW  (RECEPTION_BUFFER-1)
#define MAX_SEQUENCE_NUMBER 100
#define P_MIN 0.00001
#define P_MIN_STRING "0.00001"
#define DROP_FACTOR 0.02
#define DROP_FACTOR_STRING "0.02"
#define BETA 0.5
#define BETA_STRING "0.5"
#define GAMMA 1
#define SAMPLES 30 // Size of RTT sample queue. RAQM window adaptation starts only if this queue is full. Until it is full, Interests are sent one per Data packet.
#define INTEREST_LIFETIME "1s"
#define ALLOW_STALE false

//------------------------------------------------

namespace ns3 {
namespace ndn {

//forward declaration
class DataPath;

class ConsumerElasticTraffic;

class PendingInterests {

public:
	PendingInterests();

	const Time&
	GetSendTime();

	void
	SetSendTime(const Time& now);

	std::size_t
	GetRawDataSize();

	std::size_t
	GetPacketSize();

	void
	MarkAsReceived(bool val);

	bool
	CheckIfDataIsReceived();

	void
	AddRawData(const Data& data);

	void
	RemoveRawData();

private:
	std::size_t raw_data_size;    /* its size (plus 1) in bytes */
	std::size_t packet_size;      /* its size (plus 1) in bytes */
	bool isReceived;
	Time send_time;
};//end class InFlightInterests


//application definition:
class ConsumerRAAQM: public ns3::ndn::Consumer {
public:
	static TypeId
	GetTypeId();
	ConsumerRAAQM();
	virtual ~ConsumerRAAQM();
public:

	/**
	 * \brief override the sendPacket() function in Consumenr class.
	 *
	 *  the ending of the application(m_seq=m_seqMax) is no more checked
	 *  in this function, it is the responsiblity of the caller to check
	 *  if another interest should be sent or not.
	 */
	void
	SendPacket();

	/**
	 * \brief override the Ondata() function in Consumer class
	 * main block for calculating window size
	 */
	virtual void
	OnData(shared_ptr<const Data> contentObject);

#ifdef MLDR
	virtual void
	OnInterest(shared_ptr<const Interest> interest);
#endif
	/**
	 * \brief override the OnTimeout(uint32_t sequenceNumber)
	 *  function in Consumenr class.
	 */
	virtual void
	OnTimeout(uint32_t sequenceNumber);

	void
	ControlStatsReporter();

	void
	setSystemTimeout(int i);

	void
	SetStartTimeToNow();

	void
	SetLastTimeoutToNow();

	inline void
	EnableOutput() {
		m_isOutputEnabled = true;
	}

	inline void
	EnablePathReport() {
		m_isPathReportEnabled = true;
	}

	void
	SetCurrentPath(shared_ptr<DataPath> path);

	void
	SetMaxWindow(unsigned int NewMaxWindow);

	void
	InsertToPathTable(std::string key, shared_ptr<DataPath> path);

	//If ConsumerElasticTraffic is used
	void
	ReStart(std::string nextPrefix);

	void
	SetStopcallBack(ConsumerElasticTraffic* MyElasticTraffic);
	//

	// My tracing
	void
	WriteConsumerTrace(bool isInitialize);

	void
	SetTraceFileName(std::string path, std::string name);


protected:

	virtual void
	ScheduleNextPacket();

	/** \brief override StartApplication() in Consumer class
	 *  perform extra path initialization ???
	 */
	virtual void
	StartApplication();

	/** \brief override StopApplication() in Consumer class
	 *  print summary of status stopping application
	 */
	virtual void
	StopApplication();

	void
	OnReachRetxLimit(uint32_t sequenceNumber);

	/**
	 * \brief override SetRetxTimer() function in Consumer class
	 *  to make the timeout of an interest constant and equal to the lifetime of interest
	 */
	void
	SetLookupIntervalForExpiredInterests(Time interval);

	Time
	GetLookupInterval() const;

	void
	CheckRetxTimeout(); // ???

	Time m_lookupInterval;
	EventId m_newRetxEvent;

	TracedCallback<uint32_t /* appId */, double /* curwindow */, unsigned /* winpending */>WindowTrace;
	TracedCallback<flow_statistics_t*> FlowStatsTrace;
#ifdef MLDR
	TracedCallback<Name, double, uint64_t, uint64_t, uint64_t> m_satisfactionTime;
#endif
	//for memory: recycle this app
	Callback<void,Ptr<ConsumerRAAQM> > OnStop;

	void
	reInitialize();

private:
	void
	PrintSummary();

	void
	UpdateRTT(unsigned slot);

	// Update RTT and reduce its value by the wireless delay (if any)
	void
	UpdateRTTWithoutWireless(unsigned slot, uint64_t rtt_val);

	// Increase window size when a whole window is received
	void
	IncreaseWindow();

	//Update window size using RAAQM
	void
	RAQM();

	//Should be invoked every time a new Data packet is received
	void
	AfterVirtualDataReception( unsigned slot);

	void
	AfterDataReceptionWithReducedRTT( unsigned slot, uint64_t wireless_rtt);

private:

	unsigned m_maxwindow;

	// configuration parameters that are used by each path
	// a copy is also stored in the application for configuration(TypeId definition) purposes:
	double m_dropFactor;
	double m_Pmin;
	unsigned int m_Samples;

	unsigned int m_GAMMA;
	double m_BETA;
	bool m_allow_stale;						//	bool m_mustBeFresh; changed to bool and set to false by default

	int m_scope;
	bool m_isOutputEnabled;  				// 	set to false by default
	bool m_isPathReportEnabled; 			//	set to false by default

	unsigned m_winrecvd; 					// 	size
	unsigned m_winpending; 					//	size
	double m_curwindow; 					//	size


	unsigned m_firstInFlight; 				// expected data (first interest inflight)
	unsigned m_ooo_count;		 			// current number of out of order packets

	unsigned m_finalslot;					// max_seq_no ?
	intmax_t m_interests_sent;				// [Tracing] Interests sent by given application
	intmax_t m_pkts_recvd;					// [Tracing]
	intmax_t m_pkts_bytes_recvd;			// [Tracing]
	intmax_t m_raw_data_bytes_recvd;		// [Tracing]
	intmax_t m_pkts_delivered;				// [Tracing]
	intmax_t m_raw_data_delivered_bytes;	// [Tracing]
	intmax_t m_pkts_delivered_bytes;		// [Tracing]

	intmax_t m_nTimeouts;					//number of timeout events
	shared_ptr<DataPath> m_curr_path;

	// a hash table for path, each entry is composed of a path ID(key) and
	// a pointer to a specific path object(mapped)
	typedef boost::unordered_map<std::string, shared_ptr<DataPath> > HashTableForPath;
	HashTableForPath m_PathTable;

	Time m_start_time;
	Time m_stop_time;
	Time m_last_timeout;
	PendingInterests m_inFlightInterests[RECEPTION_BUFFER];

	bool m_isAbortModeEnabled;				// If activated: when a whole window is timed out it will abort the download

	unsigned m_winretrans;
	unsigned m_reTxLimit;					// 0 : no limit, 1: no retransmissions, i > 1: i-1 retransmissions
	unsigned m_nHoles;						// Number of lost packets detected by this Application

	Time m_defaultInterestLifeTime;

	std::stringstream m_filePlot;			// [Tracing]
	Time m_start_period;					// [Tracing]
	double m_observation_period;			// [Tracing]
	bool m_stop_tracing;					// [Tracing]
	intmax_t m_old_npackets;				// [Tracing] number of data sent during observation period
	double m_old_rate;						// [Tracing] the rate in previous observation period
	intmax_t m_sent_interests; 				// [Tracing] sent interests in bytes (total)
	intmax_t m_old_interests;				// [Tracing] amount in bytes of sent interests during observation period
	intmax_t m_old_data_inpackets;			// [Tracing] to compute the number of sent data during observation period
	intmax_t m_interests_inpackets;			// [Tracing] total number of sent interests
	intmax_t m_old_interests_inpackets;		// [Tracing] help to compute number of sent interests per observation period
	std::string m_filepath;					// [Tracing] Path to the output file

	bool m_isSending;						// Device is busy and is currently sending a packet
	bool m_isRetransmission;
	unsigned int m_seed;
	bool m_rtt_red;
	intmax_t m_RawDataReceived;

	typedef std::set<uint64_t> RTXset;
	RTXset m_ListOfRetransmitted; 			// [Tracing] For goodput computation don't consider the packets from this list

	bool m_enable_tracing;
	double m_alpha_history;
#ifdef MLDR
	typedef std::set<uint64_t> NackSet;
	NackSet m_nackSet; 						// Don't drop a window on timeout if the timeouted interest has a sequence numbers from this set
	typedef std::map<uint64_t, uint64_t> TimestampMap;
	TimestampMap m_tsMap;					// Keep the sending time of the FIRST original interest. Do not consider retransmissions. For computation of the Interest satisfaction time.
#endif
	unsigned int m_received_data = 0;		//[Tracing] received Data per observation period
#ifdef MLDR
	bool m_eln_activated;
#endif
};

class DataPath {
public:
	DataPath(double dropFactor
  ,double Pmin
  ,unsigned newTimer
  ,unsigned Samples
  ,uint64_t NewRtt=1000
  ,uint64_t NewRttMin=1000
  ,uint64_t NewRttMax=1000
  );

public:
	//print path status, for tracing
	void
	PathReporter();

	void
	InsertNewRTT(uint64_t newRTT);

	void
	UpdateReceivedStats(std::size_t paketSize, std::size_t dataSize );

	double
	GetMyDropFactor();

	double
	GetDropProb();

	void
	SetDropProb(double dropProb);

	double
	GetMyPmin();

	uint64_t
	GetRTT();

	uint64_t
	GetRTTMax();

	uint64_t
	GetRTTMin();

	unsigned
	GetMySampleValue();

	unsigned
	GetRTTQueueSize();

	void
	UpdateDropProb();

private:
	double m_drop_factor;
	double m_P_MIN;
	unsigned int m_Samples;
	uint64_t m_current_rtt;
	uint64_t m_rtt_min;
	uint64_t m_rtt_max;
	double m_drop_prob;

	intmax_t m_pkts_recvd;
	intmax_t m_last_pkts_recvd;
	intmax_t m_pkts_bytes_recvd;  	 			// Total number of bytes received including ccnxheader
	intmax_t m_last_pkts_bytes_recvd;  			// pkts_bytes_recvd at the last path summary computation
	intmax_t m_raw_data_bytes_recvd;   			// Total number of raw data bytes received (excluding ccnxheader)

	class byArrival;
	class byOrder;

	typedef boost::multi_index_container<
	uint64_t,
	boost::multi_index::indexed_by<
		// by arrival (FIFO)
		boost::multi_index::sequenced<
			boost::multi_index::tag<byArrival>
		>,
		// index by ascending order
		boost::multi_index::ordered_non_unique<
			boost::multi_index::tag<byOrder>,
			boost::multi_index::identity<uint64_t>
			>
		>
	> RTTQueue;

	RTTQueue m_RTTSamples;						//double ended queue for rtt sample queue
	Time m_PreviousCallOfPathReporter;
};//end class DataPath



} // namespace ndn
} // namespace ns3

#endif // RAAQM

#endif /* SRC_NDNSIM_APPS_NDN_CONSUMER_RAAQM_HPP_ */
