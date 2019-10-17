//
// Created by supreets51 on 15/10/19.
//

/**
 * This algorithm aborts ALL other transactions that are conflicting with this when it tries to commit.
 */

#ifndef CCTS_FOCC_OTA_H
#define CCTS_FOCC_OTA_H

#include <vector>
#include <mutex>
#include <map>
#include <shared_mutex>
#include <unordered_map>
#include "DataItem.h"

#define FOCC_OTA_GARB_THRESH 100

// A class defining an FOCC_OTA scheduler.
class FOCC_OTA{
private:
    int counter;

    /// The shared memory.
    std::vector<DataItem*> dataItems;

    /// Contains all transactions!
    std::map<int, Transaction*> transactions;

    /**
     * Use this lock (in shared/exclusive mode) before any scheduler operation!
     */
    mutable std::shared_mutex scheduler_lock;
public:
    // number of data_items
    int M;

    // constructors
    explicit FOCC_OTA(int m) ;
    ~FOCC_OTA();

    // returns a transaction ID.
    int begin_trans();

    // tid performs read of dataItem indexed by dIdx onto variable store.
    int read(int tid, int dIdx, int *store);

    int write(int tid, int dIdx, const int value);

    char tryC(const int tid);

    void garbageCollect();
};

#endif //CCTS_FOCC_OTA_H
