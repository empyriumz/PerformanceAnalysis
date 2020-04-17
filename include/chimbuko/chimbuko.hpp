#pragma once

#include "chimbuko/AD.hpp"

namespace chimbuko {

  class Chimbuko{
  public:
    Chimbuko();
    ~Chimbuko();

    void init_io(int rank, IOMode mode, std::string outputPath, 
		 std::string addr, unsigned int winSize=0);
    void init_parser(std::string data_dir, std::string inputFile, std::string engineType);
    void init_event(bool verbose=false);

    /**
     * @brief Initialize connection to the parameter server
     */
    void init_net_client(int rank, const std::string &pserver_addr = "");
    void init_outlier(double sigma);
    void init_counter();

    
    void finalize();

    bool use_ps() const { return m_outlier->use_ps(); }
    void show_status(bool verbose=false ) const { m_event->show_status(verbose); }
    bool get_status() const { return m_parser->getStatus(); }
    int get_step() const { return m_parser->getCurrentStep(); }

    /**
     * @brief Run the main Chimbuko analysis loop
     * @param rank MPI rank of main Chimbuko process
     * @param[out] n_func_events number of function events recorded
     * @param[out] n_comm_events number of comm events recorded
     * @param[out] n_counter_events number of counter events recorded
     * @param[out] n_outlier number of anomalous events recorded
     * @param[out] frames number of adios2 input steps
     * @param perf_outputpath Output path for performance data
     * @param perf_step How frequently the performance data is dumped (in steps) 
     * @param only_one_frame Force the analysis to occur only for the first input step
     * @param interval_msec Optionally add a wait between steps
     */
    void run(int rank, 
	     unsigned long long& n_func_events, 
	     unsigned long long& n_comm_events,
	     unsigned long long& n_counter_events,
	     unsigned long& n_outliers,
	     unsigned long& frames,
#ifdef _PERF_METRIC
	     std::string perf_outputpath="",
	     int         perf_step=10,
#endif
	     bool only_one_frame=false,
	     int interval_msec=0);

  private:
    /**
     * @brief Signal the parser to parse the adios2 timestep
     * @param[out] step index
     * @param[out] number of func events parsed
     * @param[out] number of comm events parsed
     * @param[out] number of counter events parsed
     * @return false if unsuccessful, true otherwise
     */
    bool parseInputStep(int &step, 
			unsigned long long& n_func_events, 
			unsigned long long& n_comm_events,
			unsigned long long& n_counter_event);


    /*
     * @brief Extract parsed events and insert into the event manager
     * @param rank The MPI rank of the process
     * @param step The adios2 stream step index
     */
    void extractEvents(int rank, int step);

    /*
     * @brief Extract parsed counters and insert into counter manager
     * @param rank The MPI rank of the process
     * @param step The adios2 stream step index
     */
    void extractCounters(int rank, int step);
    
    // int m_rank;
    // std::string m_engineType;  // BPFile or SST
    // std::string m_data_dir;    // *.bp location
    // std::string m_inputFile;   
    // std::string m_output_dir;

    ADParser * m_parser;       /**< adios2 input data stream parser */
    ADEvent * m_event;         /**< func/comm event manager */
    ADCounter * m_counter;     /**< counter event manager */
    ADOutlierSSTD * m_outlier; /**< outlier detection algorithm */
    ADio * m_io;               /**< output writer */
    ADNetClient * m_net_client; /** <client for comms with parameter server */
    
    // priority queue was used to mitigate a minor problem from TAU plug-in code
    // (e.g. pthread_create in wrong position). Actually, it doesn't casue 
    // call stack violation and the priority queue approach is not the ultimate
    // solution. So, it is deprecated and hope the problem is solved in TAU side.
    // PQUEUE pq;
  };

}
