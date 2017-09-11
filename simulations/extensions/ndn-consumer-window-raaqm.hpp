/*
 * ndn-consumer-window-raaqm.hpp
 *
 *  Created on: Mar 18, 2015
 *      Author: muscariello
 *              Zeng Xuan
 */

#ifndef SRC_NDNSIM_APPS_NDN_CONSUMER_WINDOW_RAAQM_HPP_
#define SRC_NDNSIM_APPS_NDN_CONSUMER_WINDOW_RAAQM_HPP_

#ifdef RAAQM

#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
#include "ndn-flow-stats.hpp"

#define BUF_DIM 12800
//#define GOT_HERE() fprintf(stderr, "LINE %d\n", __LINE__)
#define GOT_HERE() ((void)(__LINE__))
#define TRUE 1
#define FALSE 0
#define DEFAULT_INTEREST_TIMEOUT 1000000 //microsecond (us)
#define MAX_INTEREST_LIFETIME_MS 10000 //millisecond (ms)
#define CHUNK_SIZE 1024
#define RATE_SAMPLES 4096  //number of packets on which rate is calculated
#define PATH_REP_INTVL 2000000
#define TIMEOUT_SMOOTHER 0.1
#define QUEUE_CAP 200
#define THRESHOLD 0.9
#define TIMEOUT_RATIO 10

#define DEFAULT_PATH_ID "default_path"


//relevent default system configuration parameters
#define INITIAL_MAX_WINDOW  (BUF_DIM-1)

#define MAX_SEQUENCE_NUMBER 100

#define P_MIN 0.00001
#define P_MIN_STRING "0.00001"
  
#define DROP_FACTOR 0.02
#define DROP_FACTOR_STRING "0.02" 

//0.5?
#define BETA 0.5
#define BETA_STRING "0.5"
  
#define GAMMA 1

//20?
#define EST_LEN 30

//one second
#define INTEREST_LIFETIME "1s" 
#define  ALLOW_STALE false
//one millisecond
#define RETX_TIMER "1ms" 

namespace ns3 {
namespace ndn {

//forward declaration
class Path;

class InFlightInterests {

public:
	InFlightInterests();
	
	const Time&
	getSendTime();

	void
	setSendTime(const Time& now);

	std::size_t
	getRawDataSize();

	std::size_t
	getPacketSize();

	void
	markAsReceived();
	
	bool
	isDataReceived();

	void
	addRawData(const Data& data);

	void
	removeRawData();
private:
	//real operations for buffering are canceled in the implementation, for convinience of simulations
	//unsigned char *raw_data;      /* content that has arrived out-of-order */
	std::size_t raw_data_size;    /* its size (plus 1) in bytes */
	std::size_t packet_size;      /* its size (plus 1) in bytes */
	//	bool retrans; /* not needed, already defined in Consumer class*/
	//NOTE: new feature, a flag to indicate if the data is received.
	bool isReceived;
	Time send_time;
};//end class InFlightInterests



//application definition:
class ConsumerWindowRAAQM: public ns3::ndn::Consumer {
public:
	static TypeId
	GetTypeId();
	ConsumerWindowRAAQM();
	virtual ~ConsumerWindowRAAQM();
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
	
	/**
	 * \brief override the OnTimeout(uint32_t sequenceNumber)
	 *  function in Consumenr class.
	 */
	virtual void
	OnTimeout(uint32_t sequenceNumber);

	void
	control_stats_reporter();

	void
	setSystemTimeout(int i);

	void
	setStartTimetoNow();

	void
	setLastTimeOutoNow();

	inline void
	enableOutput() {
		m_isOutputEnabled = true;
	}

	inline void
	enablePathReport() {
		m_isPathReportEnabled = true;
	}

	void
	setCurrentPath(shared_ptr<Path> path);
	
	void
	setMaxWindow(unsigned int NewMaxWindow);
	


// 	void
// 	run();

	void
	insertToPathTable(std::string key,shared_ptr<Path> path);
	
	
	void
	ReStart(std::string nextPrefix);
	
// TODO|	void
// TODO|	SetStopCallback(Callback<void, Ptr<App>> cb);
	
protected:
  
       /** 
	 * \brief override the ScheduleNextPacket() in Consumer class.
	 * simply forward call to sendPacket().
	 *
	 * because in this application  whenever
	 * it becomes necessary to send an interest, the interest is  always 
	 * scheduled to now and sent immediately
	 * 
	 */
	virtual void
	ScheduleNextPacket();
	
	 /** \brief override StartApplication() in Consumer class.
	  *  perform extra path initialization
	  */
	virtual void
	StartApplication();
	
	/** \brief override StopApplication() in Consumer class.
	 *  print summary of status stopping application
	 */
	virtual void
	StopApplication();
	
	void
	onReachRetxLimit(uint32_t sequenceNumber);

	/**
	 * \brief override SetRetxTimer() function in Consumer class
	 *  to make the timeout of an interest constant and equal to the lifetime of interest
	 */
	void
	SetRetxTimer(Time retxTimer);
	
	/**
	 * \brief override GetRetxTimer() function in Consumer class, even if it is indeed the same
	 *  to make the timeout of an interest constant and equal to the lifetime of interest
	 */
	Time
	GetRetxTimer() const;
	
	/**
	 * \brief override CheckRetxTimeout() function in Consumer class
	 *  to make the timeout of an interest constant and equal to the lifetime of interest
	 */
	void
	CheckRetxTimeout();
	
	 Time m_newRetxTimer;
	 EventId m_newRetxEvent;
	 
	 TracedCallback<uint32_t /* appId */, double /* curwindow */, unsigned /* winpending */>WindowTrace;

   TracedCallback<FlowStats &> m_FlowStatsTrace;
	 
// TODO|	 //for memory: recycle this app
// TODO|	 Callback<void, Ptr<App>> m_StopCallback;

  TracedCallback<Ptr<Application>> m_StopCallback;
	 
	 void
	 reInitialize();
	
private:
	void
	print_summary();
	/* @brief update the RTT status after a data packet at slot is received. also update the timer for the current path
	 * @param slot  the corresponding slot of the received data packet.
	 *
	 */
	void
	update_RTT_stats(unsigned slot);


	// update the window size
	void
	wininc();

	//update drop probability and modify curwindow accordingly.
	void
	RAQM();

	//processing after each packet received: check timeout,update RTT update windows RAQM
	void
	npkt_received( unsigned slot);

	//send one more new interest with sequence number to be seq, interest_sent++, winpending++
// 	void
// 	ask_more(uint64_t seq);
	
	bool
	openTraceFile();
	
	void
	printHeader();
	
	void
	printGoodputHandoverTrace();

private:
	//Name m_dataName;//use m_interestName inherited from Consumer class instead
	unsigned maxwindow;//corresponds to n pipeline slot in initialization
	
	//configuration parameters that are used by each path, a copy is also stored in the application for configuration(TypeId definition) purposes:
	double m_dropFactor;
	double m_Pmin;
	unsigned int m_EST_LEN;
	
	
	// unsigned finalchunk;
	// finalchunk has been defined in Consumer class as ****m_seqMax****
	
	unsigned int m_GAMMA;
	double m_BETA;
	bool m_allow_stale;//bool m_mustBeFresh; changed to bool and set to false by default
	
	
	//const time::milliseconds m_defaulInterestLifeTime; //default interest lifetime for all interests
	//m_defaulInterestLifeTime gas been defined in Consumer class as ****m_interestLifeTime****
	
	int m_scope;
	bool m_isOutputEnabled;  // set to false by default
	bool m_isPathReportEnabled; //set to false by default
	
	//not useful for the moment
	//int use_decimal;
	
	unsigned winrecvd;
	unsigned winpending;
	double curwindow;//size_t m_pipeSize;
	

	//point to the "slot"(important) that the base of the sliding window corresponds to
	unsigned ooo_base;
	unsigned ooo_count;

	//point to the next segment number
	//unsigned ooo_next;
	// ooo_next has been defined in Consumer classas as ****m_seq**** 
	
	unsigned finalslot;
	intmax_t interests_sent;
	intmax_t pkts_recvd;
	intmax_t pkts_bytes_recvd;
	intmax_t raw_data_bytes_recvd;
	//size_t m_totalSize;
	intmax_t pkts_delivered;
	intmax_t raw_data_delivered_bytes;
	intmax_t pkts_delivered_bytes;
	//not useful for the moment
	// intmax_t holes;

	intmax_t nTimeouts;//number of timeout events
	shared_ptr<Path> cur_path;

	// a hash table for path, each entry is composed of a path ID(key) and
	// a pointer to a specific path object(mapped)
	typedef boost::unordered_map<std::string, shared_ptr<Path> > HashTableForPath;
	HashTableForPath PathTable;

	/**
	 *  @brief currently the following is not needed
	 */
//	time::milliseconds m_timeout;//system timeout,default to 0 (run forever)

	//not interested for the moment
	//intmax_t unverified;

	Time start_tv;
	Time stop_tv;
	Time last_timeout;
	InFlightInterests Window[BUF_DIM];
	//InFlightInterests* Window;
	// uint32_t lifetime; //use timer of path instead to set lifetime of interest
	//not interested for the moment
	//uint32_t disable_cache;
	//Face m_face;
	
	//NOTE: new featture:
	bool m_isAbortModeEnabled;
	// number of slots whose coresponind interests have been retransmitted in the curent window, 
	//once it reaches "curwindow", the application will stop.
	unsigned winretrans;	
	//0 means no limit
	unsigned m_reTxLimit; 
	unsigned m_nHoles;
	
	int32_t m_initialWindowSize;
	intmax_t m_lastHandover_raw_data_delivered_bytes;
	int m_maxnumHandover;
	int m_currentNumHandover;
	
	std::string m_goodputHandoverTracefile;
	shared_ptr<std::ostream> m_os;
	
	Time m_defaultInterestLifeTime;
	Time m_lastHandoverTime;
  
  // hopCount accounting
  double m_recv_packets;
  double m_mean_hop_count;
  double m_handover_recv_packets;
  double m_handover_mean_hop_count;

	

};

class Path {
public:
	Path(double dropFactor
  ,double Pmin
  ,unsigned newTimer
  ,unsigned int ESTLEN
  ,unsigned NewRtt=1000
  ,unsigned NewRttMin=1000
  ,unsigned NewRttMax=1000
  );
public:
	//print path status
	void
	path_reporter();

	//add a new RTT to the RTT queue of the path, check if RTT queue is full, and thus need overwrite.
	//also it maintains the vailidity of min and max of RTT.
	void
	addRTT(unsigned newRTT);

	//update received bytes
	void
	update_received_stats(std::size_t paketSize, std::size_t dataSize );

	double
	getMyDropFactor();

	double
	getDropProb();

	void
	setDropProb(double dropProb);

	double
	getMyPmin();

	unsigned
	getTimer();

	void
	smoothTimer();

	unsigned
	getRTT();

	unsigned
	getRTTMax();

	unsigned
	getRTTMin();

	unsigned
	getMyEstLen();

	unsigned
	getRTTQueueSize();

	//change drop probability according to RTT statistics
	//invoked in RAQM(), before control window size update.
	void
	resetDropProb();
private:
	double m_drop_factor;
	double m_P_MIN;
	unsigned timer;//timer in milliseconds
	unsigned int m_EST_LEN;
	unsigned rtt,rtt_min, rtt_max;
	double drop_prob;

	//for the moment not interested in
	//intmax_t dups;
	intmax_t pkts_recvd;
	intmax_t last_pkts_recvd;
	intmax_t pkts_bytes_recvd;  	 /** Total number of bytes received including ccnxheader*/
	intmax_t last_pkts_bytes_recvd;  /** pkts_bytes_recvd at the last path summary computation */
	intmax_t raw_data_bytes_recvd;   /** Total number of raw data bytes received (excluding ccnxheader)*/
	intmax_t last_raw_data_bytes_recvd;/** raw_data_bytes_recvd at the last throughput computation */

	class byArrival;
	class byOrder;

	typedef boost::multi_index_container<
	unsigned,
	boost::multi_index::indexed_by<
		// by arrival (FIFO)
		boost::multi_index::sequenced<
			boost::multi_index::tag<byArrival>
		>,
		// index by ascending order
		boost::multi_index::ordered_non_unique<
			boost::multi_index::tag<byOrder>,
			boost::multi_index::identity<unsigned>
			>
		>
	> RTTQueue;

	RTTQueue sam_queue; //double ended queue for rtt sample queue
	Time last_rate_cal;
};//end class Path



} // namespace ndn
} // namespace ns3

#endif // RAAQM

#endif /* SRC_NDNSIM_APPS_NDN_CONSUMER_WINDOW_RAAQM_HPP_ */
