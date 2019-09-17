#include "chimbuko/chimbuko.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

class ADTest : public ::testing::Test
{
protected:
    int world_rank, world_size;

    virtual void SetUp()
    {
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    }

    virtual void TearDown()
    {

    }

    template <typename K, typename V>
    void show_map(const std::unordered_map<K, V>& m)
    {
        for (const auto& it : m)
        {
            std::cout << it.first << " : " << it.second << std::endl;
        }
    }    

    chimbuko::CallListMap_p_t* generate_callList(unsigned long max_calls) {
        using namespace chimbuko;

        const unsigned long N_FUNC = 10;
        const unsigned long N_EVENT = 3;
        const unsigned long N_TAGS  = 5;
        const unsigned long LABEL_STEP = 10;
        const unsigned long PARENT_STEP = 20;
        const unsigned long rid = (unsigned long)world_rank;

        CallListMap_p_t * callList = new CallListMap_p_t;

        for (unsigned long ts=0; ts < max_calls; ts++)
        {
            std::string fid = "Function_" + std::to_string(ts%N_FUNC);
            unsigned long entry_d[] = {0, rid, 0, ts%N_EVENT, ts%N_FUNC, ts};
            unsigned long exit_d[]  = {0, rid, 0, ts%N_EVENT, ts%N_FUNC, ts*10 + 1};

            Event_t entry_ev(entry_d, EventDataType::FUNC, FUNC_IDX_TS, fid);
            Event_t exit_ev(exit_d, EventDataType::FUNC, FUNC_IDX_TS, fid);
            ExecData_t exec(entry_ev);
            exec.update_exit(exit_ev);
            exec.set_funcname(fid + "_funcname");

            for (unsigned long i = 0; i < 5; i++) {
                // exec.add_child(fid + "_child_" + std::to_string(i));

                unsigned long comm_d[] = {0, rid, 0, ts%(N_EVENT+2), ts%N_TAGS, (i+rid)%world_size, 10+i*1024, ts+1+i};
                Event_t comm_ev(comm_d, EventDataType::COMM, COMM_IDX_TS);
                CommData_t comm(comm_ev, ((i%2==0)? "SEND": "RECV"));
                exec.add_message(comm);
            }

            if (ts%PARENT_STEP == 0) exec.set_parent(fid + "_parent");
            if (ts%LABEL_STEP == 0) exec.set_label(-1);

            (*callList)[0][world_rank][0].push_back(exec);
        }

        // std::cout << std::endl << (*callList)[0][world_rank][0].front() << std::endl;
        return callList;
    }
};

TEST_F(ADTest, EnvTest)
{
    EXPECT_EQ(4, world_size);
}

TEST_F(ADTest, BpfileTest)
{
    using namespace chimbuko;

    //const int N_RANKS = 4;
    const std::vector<int> N_STEPS{6, 6, 6, 6};
    const std::vector<std::vector<size_t>> N_FUNC{
        {57077, 51845, 60561, 63278, 64628, 66484, 42233},
        {41215, 51375, 56620, 57683, 58940, 61010, 41963},
        {41108, 50820, 57237, 56590, 63458, 63931, 42486},
        {40581, 51127, 59175, 58158, 60465, 62516, 41238}
    }; 
    const std::vector<std::vector<size_t>> N_COMM{
        {89349, 107207, 121558, 123381, 128682, 131611, 83326},
        {78250, 94855 , 106301, 107222, 111581, 114273, 77817},
        {77020, 93355 , 105238, 106608, 114192, 118530, 79135},
        {77332, 93027 , 107856, 107628, 112019, 116468, 74517}
    };
    const std::vector<std::vector<unsigned long>> N_OUTLIERS{
        {134, 65, 83, 73, 77, 68, 34},
        { 89, 44, 62, 58, 54, 47, 39},
        { 99, 56, 59, 64, 66, 58, 43},
        {106, 58, 79, 49, 54, 66, 48}
    };


    std::string data_dir = "./data";
    std::string output_dir = "./data";
    std::string inputFile = "tau-metrics-" + std::to_string(world_rank) + ".bp";
    std::string engineType = "BPFile";

    double sigma = 6.0;

    Chimbuko driver;
    int step;
    unsigned long n_outliers = 0, frames = 0;
    unsigned long long n_func_events = 0, n_comm_events = 0;

    driver.init_io(
        world_rank, output_dir, 
        {{"rank", world_rank}, {"algorithm", 0}, {"nparam", 1}, {"winsz", 5}},
        IOMode::Off);
    driver.init_parser(data_dir, inputFile, engineType);
    driver.init_event();
    driver.init_outlier(world_rank, sigma);

    while ( driver.get_status() )
    {
        n_func_events = 0;
        n_comm_events = 0;
        n_outliers = 0;

        driver.run(world_rank, n_func_events, n_comm_events, n_outliers, frames, true);

        if (driver.get_step() == -1)
            break;

        step = driver.get_step();

        EXPECT_EQ(N_FUNC[world_rank][step], n_func_events);
        EXPECT_EQ(N_COMM[world_rank][step], n_comm_events);
        EXPECT_EQ(N_OUTLIERS[world_rank][step], n_outliers);
    }
    EXPECT_EQ(N_STEPS[world_rank], step);

    MPI_Barrier(MPI_COMM_WORLD);
    driver.finalize();
}

TEST_F(ADTest, BpfileWithNetTest)
{
    using namespace chimbuko;

    //const int N_RANKS = 4;
    const std::vector<int> N_STEPS{6, 6, 6, 6};
    const std::vector<std::vector<size_t>> N_FUNC{
        {57077, 51845, 60561, 63278, 64628, 66484, 42233},
        {41215, 51375, 56620, 57683, 58940, 61010, 41963},
        {41108, 50820, 57237, 56590, 63458, 63931, 42486},
        {40581, 51127, 59175, 58158, 60465, 62516, 41238}
    }; 
    const std::vector<std::vector<size_t>> N_COMM{
        {89349, 107207, 121558, 123381, 128682, 131611, 83326},
        {78250, 94855 , 106301, 107222, 111581, 114273, 77817},
        {77020, 93355 , 105238, 106608, 114192, 118530, 79135},
        {77332, 93027 , 107856, 107628, 112019, 116468, 74517}
    };
    const std::vector<std::vector<unsigned long>> N_OUTLIERS{
        {134, 50, 65, 54, 62, 54, 31},
        { 75, 20, 33, 42, 32, 47, 44},
        { 72, 34, 27, 25, 28, 28, 35},
        { 76, 32, 47, 26, 26, 32, 26}
    };


    std::string data_dir = "./data";
    std::string output_dir = "./data";
    std::string inputFile = "tau-metrics-" + std::to_string(world_rank) + ".bp";
    std::string engineType = "BPFile";

    double sigma = 6.0;

    Chimbuko driver;

    int step;
    unsigned long n_outliers = 0, frames = 0;
    unsigned long long n_func_events = 0, n_comm_events = 0;

    driver.init_io(
        world_rank, output_dir, 
        {{"rank", world_rank}, {"algorithm", 0}, {"nparam", 1}, {"winsz", 5}},
        IOMode::Off);
    driver.init_parser(data_dir, inputFile, engineType);
    driver.init_event();
    driver.init_outlier(world_rank, sigma, "tcp://localhost:5559");

    step = -1;
    while ( driver.get_status() )
    {
        n_func_events = 0;
        n_comm_events = 0;
        n_outliers = 0;

        // for the test purpose, we serialize the excution for run()
        int token = 1;
        if (world_rank != 0) {
            MPI_Recv(&token, 1, MPI_INT, world_rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        driver.run(world_rank, n_func_events, n_comm_events, n_outliers, frames, true);
        
        MPI_Send(&token, 1, MPI_INT, (world_rank+1)%world_size, 0, MPI_COMM_WORLD);
        if (world_rank == 0) {
            MPI_Recv(&token, 1, MPI_INT, world_size-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        if (driver.get_step() == -1)
            break;

        step = driver.get_step();

        // std::cout << "Rank: " << world_rank 
        //     << ", Step: " << step 
        //     << ", # func: " << n_func_events
        //     << ", # comm: " << n_comm_events
        //     << ", # outliers: " <<  n_outliers
        //     << std::endl;

        EXPECT_EQ(N_FUNC[world_rank][step], n_func_events);
        EXPECT_EQ(N_COMM[world_rank][step], n_comm_events);
        EXPECT_EQ(N_OUTLIERS[world_rank][step], n_outliers);
    }
    // std::cout << "Final, Rank: " << world_rank 
    //     << ", Step: " << step 
    //     << std::endl;
    EXPECT_EQ(N_STEPS[world_rank], step);

    MPI_Barrier(MPI_COMM_WORLD);
    driver.finalize();
}

TEST_F(ADTest, ioReadWriteBinaryTest)
{
    using namespace chimbuko;
    const long long MAX_STEPS = 10;
    const unsigned long MAX_CALLS = 200;

    // Write
    ADio * ioW;
    ioW = new ADio();

    ioW->setDispatcher();
    ioW->setHeader({
        {"rank", world_rank}, {"algorithm", 0}, {"nparam", 1}, {"winsz", 3}
    });
    ioW->setLimit(400000); // 400k
    ioW->open("./temp/testexec." + std::to_string(world_rank), IOOpenMode::Write);

    for (long long step = 0; step < MAX_STEPS; step++) {
        CallListMap_p_t* callList = generate_callList(MAX_CALLS);
        ioW->write(callList, step);
    }

    EXPECT_EQ(3, ioW->getWinSize());

    // wait until all io jobs are finished
    for (int itry=0; itry<10; itry++) {
        while (ioW->getNumIOJobs()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EXPECT_EQ(3, ioW->getDataFileIndex()+1);
    delete ioW;
    //MPI_Barrier(MPI_COMM_WORLD);

    // Read
    ADio * ioR;
    ioR = new ADio();

    ioR->setDispatcher(); // currently dispatcher can't be used for reading
    ioR->open("./temp/testexec." + std::to_string(world_rank), IOOpenMode::Read);
    EXPECT_EQ(world_rank, ioR->getHeader().get_rank());
    EXPECT_EQ((unsigned int)MAX_STEPS, ioR->getHeader().get_nframes());
    EXPECT_EQ(0, ioR->getHeader().get_algorithm());
    EXPECT_EQ(3, ioR->getHeader().get_winsize());
    EXPECT_EQ(1, ioR->getHeader().get_nparam());
    EXPECT_EQ((size_t)MAX_STEPS, ioR->getHeader().get_datapos().size());

    CallListMap_p_t* callList = generate_callList(MAX_CALLS);
    CallList_t& cl_ref = (*callList)[0][world_rank][0];
    IOError err;
    for (size_t iframe = 0; iframe < ioR->getHeader().get_nframes(); iframe++) {
        //long long iframe = world_rank*2;
        CallList_t cl;
        err = ioR->read(cl, (long long)iframe);
        if (err != IOError::OK)
            FAIL() << "Fail to get a frame.";

        EXPECT_EQ(137, cl.size());

        //EXPECT_TRUE(cl_ref.front().is_same(cl.front()));
        CallListIterator_t it = cl.begin(), it_ref = cl_ref.begin();
        while (it_ref != cl_ref.end() && it != cl.end()) {
            // the first one must be same
            ASSERT_TRUE(it_ref->is_same(*it));
            it_ref++;
            it++;

            if (it_ref == cl_ref.end()) break;
            if (it == cl.end()) break;

            // next 3 items are also same (winsz == 3) (right side)
            for (int i = 0; i < 3 && it_ref != cl_ref.end() && it != cl.end(); i++) {
                ASSERT_TRUE(it_ref->is_same(*it));
                it_ref++;
                it++;
            }

            if (it_ref == cl_ref.end()) break;
            if (it == cl.end()) break;

            // next 3 items were skipped.
            for (int i = 0; i < 3 && it_ref != cl_ref.end() && it != cl.end(); i++) {
                ASSERT_FALSE(it_ref->is_same(*it));
                it_ref++;
            }
            if (it_ref == cl_ref.end()) break;
            if (it == cl.end()) break;

            // next 3 items are also same (winsz == 3) (left side)
            for (int i = 0; i < 3  && it_ref != cl_ref.end() && it != cl.end(); i++) {                    
                ASSERT_TRUE(it_ref->is_same(*it));
                it_ref++;
                it++;
            }
        }
        ASSERT_TRUE(it == cl.end());

        int remained = 0;
        while (it_ref != cl_ref.end()) {
            it_ref++;
            remained++;
        }
        EXPECT_EQ(6, remained);
    }

    delete callList;
    delete ioR;
}
