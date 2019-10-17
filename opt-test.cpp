/**
 * opt-test
 * Testing function for the various schedulers!
 * The algorithms have been toaken from the book Transaction Information Systems (by Vossen)
 *
 * As per the assignment's opt-test, each transaction does both read and write operations on each data items it accesses.
 *  In case that is not so, i.e. each transaction can either do a read or a write, then need to modify code slightly -
 *  in each algo's tryCommit, lock dataitems from both readSet and writeSet (combine them into an ordered set, and lock
 *  in that order)
 */

#include <iostream>
#include <cstdio>               /* for fprintf() */
#include <cstdlib>              /* for rand() */
#include <unistd.h>             /* for usleep() */
#include <chrono>
#include <thread>
#include <random>
#include <fstream>

#include "FOCC_OTA.h"
#include "FOCC_CTA.h"
#include "BOCC_CTA.h"

// Global variables
namespace glb{
    int numTrans = 0;
    int M = 10;
    // writes are random numbers less than this number but >= 0
    int constVal = 100;
    // labmda is in milliseconds
    int lambda = 10;
    int N = 2;

    // just for convenience, need to be changed in scheduler separately.
    const char TRANS_LIVE = 's';
    const char TRANS_COMMIT = 'c';
    const char TRANS_ABORT = 'a';

    std::vector<uint64_t> total_aborts;
    std::vector<uint64_t> total_delay ;

    std::default_random_engine generator;
    std::exponential_distribution<double> dist(lambda);

    FOCC_OTA *foccOta;
    FILE *log_file;
}

void updtMem(FOCC_OTA* foccOta, int mTid);
void updtMem2(FOCC_CTA* foccOta, int mTid);
void updtMem3(BOCC_CTA* foccOta, int mTid);

//void updtMem(int mTid);

using namespace std::chrono;

int main() {
    srand(time(nullptr));

    std::ifstream inFile{"inp-params.txt", std::ifstream::in};
    if(inFile.fail()){
        std::cerr<<"inp-params.txt NOT FOUND!\n";
        exit(1);
    }
    inFile>>glb::N>>glb::M>>glb::numTrans>>glb::constVal>>glb::lambda;

    // initialization.
    for(int i=0; i<glb::N; i++) {
        glb::total_aborts.push_back(0);
        glb::total_delay.push_back(0);
    }

    std::cout<<"Input Params: N="<<glb::N<<" M = "<<glb::M<<" numTrans = "<<glb::numTrans<<"\n";

    glb::dist = std::exponential_distribution<double>(glb::lambda);

    // -- FOCC OTA ----
    std::cout<<"***** FOCC OTA ******\n";
    glb::foccOta = new FOCC_OTA(glb::M);

    std::thread tids[glb::N];

    glb::log_file = fopen("FOCC-OTA-log.txt", "w");
    for(int i=0; i<glb::N; i++) tids[i] =  std::thread(updtMem, glb::foccOta,i);
    for(int i=0; i<glb::N; i++) tids[i].join();
    delete glb::foccOta;

    uint64_t res = 0;
    for(auto e: glb::total_delay) res+=e;
    std::cout<<"Avg Delay:"<<(((double)res)/(glb::N*glb::numTrans))<<std::endl;
    res = 0;
    for(auto e: glb::total_aborts) res+=e;
    //std::cout<<"tota bors: "<<res<<"\n";
    std::cout<<"Avg Aborts:"<<(((double)res)/(glb::N*glb::numTrans))<<std::endl;
    fclose(glb::log_file);


    // -- FOCC CTA ----
    for(int i=0; i<glb::N; i++) {
        glb::total_aborts.push_back(0);
        glb::total_delay.push_back(0);
    }

    std::cout<<"\n***** FOCC CTA ******\n";

    FOCC_CTA * foccCta = new FOCC_CTA(glb::M);


    glb::log_file = fopen("FOCC-CTA-log.txt", "w");
    for(int i=0; i<glb::N; i++) tids[i] =  std::thread(updtMem2, foccCta,i);
    for(int i=0; i<glb::N; i++) tids[i].join();

    delete foccCta;

    res = 0;
    for(auto e: glb::total_delay) res+=e;
    std::cout<<"Avg Delay:"<<(((double)res)/(glb::N*glb::numTrans))<<std::endl;
    res = 0;
    for(auto e: glb::total_aborts) res+=e;
    //std::cout<<"tota bors: "<<res<<"\n";
    std::cout<<"Avg Aborts:"<<(((double)res)/(glb::N*glb::numTrans))<<std::endl;

    // -- FOCC CTA ----
    for(int i=0; i<glb::N; i++) {
        glb::total_aborts.push_back(0);
        glb::total_delay.push_back(0);
    }

    std::cout<<"\n***** BOCC CTA ******\n";

    BOCC_CTA * boccCta = new BOCC_CTA(glb::M);


    glb::log_file = fopen("BOCC-CTA-log.txt", "w");
    for(int i=0; i<glb::N; i++) tids[i] =  std::thread(updtMem3, boccCta,i);
    for(int i=0; i<glb::N; i++) tids[i].join();

    delete boccCta;

    res = 0;
    for(auto e: glb::total_delay) res+=e;
    std::cout<<"Avg Delay:"<<(((double)res)/(glb::N*glb::numTrans))<<std::endl;
    res = 0;
    for(auto e: glb::total_aborts) res+=e;
    //std::cout<<"tota bors: "<<res<<"\n";
    std::cout<<"Avg Aborts:"<<(((double)res)/(glb::N*glb::numTrans))<<std::endl;

    return 0;
}

void updtMem(FOCC_OTA* foccOta, int mTid){
    int abortCnt = 0;
    char status = glb::TRANS_ABORT;

    char buf[40];
    // each thread invokes numTrans Transactions!
    for(int curTrans=0; curTrans<glb::numTrans; curTrans++){
        abortCnt = 0;
        auto start = high_resolution_clock::now();
        do {
            int id = glb::foccOta->begin_trans();
            int nItr = rand()%glb::M + 1;

            int locVal;

            for(int i=0; i<nItr; i++){
                // rnadom index to be updated.
                int randInd = rand()%glb::M;
                int randVal = rand()%glb::constVal;

                glb::foccOta->read(id, randInd, &locVal);

                auto _reqTime = std::chrono::system_clock::now();
                time_t reqTime = std::chrono::system_clock::to_time_t(_reqTime);
                struct tm* tinfo= localtime(&reqTime);
                strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);
                fprintf(glb::log_file,"Thread ID %ld Transaction %d reads from %d a value %d at time %s\n", std::this_thread::get_id()
                        , id, randInd, locVal, buf);

                locVal+=randVal;

                glb::foccOta->write(id, randInd, locVal);

                auto _enterTime = std::chrono::system_clock::now(); //the timepoint
                auto enterTime = std::chrono::system_clock::to_time_t(_enterTime);
                tinfo= localtime(&enterTime);
                strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);

                fprintf(glb::log_file,"Thread ID %ld Transaction %d writes to %d a value %d at time %s\n", std::this_thread::get_id()
                        , id, randInd, locVal, buf);

                usleep(1000*glb::dist(glb::generator));
            }

            status = glb::foccOta->tryC(id);

            auto _exitTime = std::chrono::system_clock::now(); //the timepoint
            auto exitTime = std::chrono::system_clock::to_time_t(_exitTime);
            struct tm* tinfo= localtime(&exitTime);
            strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);
            fprintf(glb::log_file,"Transaction %d tryCommits with result %c at time %s\n\n", id, status, buf);
            //fprintf(stdout,"Transaction %d tryCommits with result %c\n", id, status);

            if(status == glb::TRANS_ABORT) {abortCnt++;}

        } while(status!=glb::TRANS_COMMIT);

        auto time_to_commit = duration_cast<milliseconds>(high_resolution_clock::now() - start);
        glb::total_delay[mTid]+=time_to_commit.count();
        glb::total_aborts[mTid]+=abortCnt;
    } // for
}

void updtMem2(FOCC_CTA* foccOta, int mTid){
    int abortCnt = 0;
    char status = glb::TRANS_ABORT;

    char buf[40];
    // each thread invokes numTrans Transactions!
    for(int curTrans=0; curTrans<glb::numTrans; curTrans++){
        abortCnt = 0;
        auto start = high_resolution_clock::now();
        do {
            int id = foccOta->begin_trans();
            int nItr = rand()%glb::M + 1;

            int locVal;

            for(int i=0; i<nItr; i++){
                // rnadom index to be updated.
                int randInd = rand()%glb::M;
                int randVal = rand()%glb::constVal;

                foccOta->read(id, randInd, &locVal);

                auto _reqTime = std::chrono::system_clock::now();
                time_t reqTime = std::chrono::system_clock::to_time_t(_reqTime);
                struct tm* tinfo= localtime(&reqTime);
                strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);
                fprintf(glb::log_file,"Thread ID %ld Transaction %d reads from %d a value %d at time %s\n", std::this_thread::get_id()
                        , id, randInd, locVal, buf);

                locVal+=randVal;

                foccOta->write(id, randInd, locVal);

                auto _enterTime = std::chrono::system_clock::now(); //the timepoint
                auto enterTime = std::chrono::system_clock::to_time_t(_enterTime);
                tinfo= localtime(&enterTime);
                strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);

                fprintf(glb::log_file,"Thread ID %ld Transaction %d writes to %d a value %d at time %s\n", std::this_thread::get_id()
                        , id, randInd, locVal, buf);

                usleep(1000*glb::dist(glb::generator));
            }

            status = foccOta->tryC(id);

            auto _exitTime = std::chrono::system_clock::now(); //the timepoint
            auto exitTime = std::chrono::system_clock::to_time_t(_exitTime);
            struct tm* tinfo= localtime(&exitTime);
            strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);
            fprintf(glb::log_file,"Transaction %d tryCommits with result %c at time %s\n\n", id, status, buf);
            //fprintf(stdout,"Transaction %d tryCommits with result %c\n", id, status);

            if(status == glb::TRANS_ABORT) {abortCnt++;}

        } while(status!=glb::TRANS_COMMIT);

        auto time_to_commit = duration_cast<milliseconds>(high_resolution_clock::now() - start);
        glb::total_delay[mTid]+=time_to_commit.count();
        glb::total_aborts[mTid]+=abortCnt;
    } // for
}

void updtMem3(BOCC_CTA* foccOta, int mTid){
    int abortCnt = 0;
    char status = glb::TRANS_ABORT;

    char buf[40];
    // each thread invokes numTrans Transactions!
    for(int curTrans=0; curTrans<glb::numTrans; curTrans++){
        abortCnt = 0;
        auto start = high_resolution_clock::now();
        do {
            int id = foccOta->begin_trans();
            int nItr = rand()%glb::M + 1;

            int locVal;

            for(int i=0; i<nItr; i++){
                // rnadom index to be updated.
                int randInd = rand()%glb::M;
                int randVal = rand()%glb::constVal;

                foccOta->read(id, randInd, &locVal);

                auto _reqTime = std::chrono::system_clock::now();
                time_t reqTime = std::chrono::system_clock::to_time_t(_reqTime);
                struct tm* tinfo= localtime(&reqTime);
                strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);
                fprintf(glb::log_file,"Thread ID %ld Transaction %d reads from %d a value %d at time %s\n", std::this_thread::get_id()
                        , id, randInd, locVal, buf);

                locVal+=randVal;

                foccOta->write(id, randInd, locVal);

                auto _enterTime = std::chrono::system_clock::now(); //the timepoint
                auto enterTime = std::chrono::system_clock::to_time_t(_enterTime);
                tinfo= localtime(&enterTime);
                strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);

                fprintf(glb::log_file,"Thread ID %ld Transaction %d writes to %d a value %d at time %s\n", std::this_thread::get_id()
                        , id, randInd, locVal, buf);

                usleep(1000*glb::dist(glb::generator));
            }

            status = foccOta->tryC(id);

            auto _exitTime = std::chrono::system_clock::now(); //the timepoint
            auto exitTime = std::chrono::system_clock::to_time_t(_exitTime);
            struct tm* tinfo= localtime(&exitTime);
            strftime (buf, sizeof (buf), "%H:%M:%S", tinfo);
            fprintf(glb::log_file,"Transaction %d tryCommits with result %c at time %s\n\n", id, status, buf);
            //fprintf(stdout,"Transaction %d tryCommits with result %c\n", id, status);

            if(status == glb::TRANS_ABORT) {abortCnt++;}

        } while(status!=glb::TRANS_COMMIT);

        auto time_to_commit = duration_cast<milliseconds>(high_resolution_clock::now() - start);
        glb::total_delay[mTid]+=time_to_commit.count();
        glb::total_aborts[mTid]+=abortCnt;
    } // for
}