//
// Created by supreets51 on 17/10/19.
//

/**
 * This algorithm aborts itself if there are conflicting transactions with this when it tries to commit.
 */

// In case of BOCC, read_list of DataItem is actually a write list: stores all transactions that are writing to that
// dataitem.

#ifndef CCTS_BOCC_CTA_H
#define CCTS_BOCC_CTA_H

#include <vector>
#include <mutex>
#include <map>
#include <shared_mutex>
#include <unordered_map>

#include "DataItem.h"

#define FOCC_CTA_GARB_THRESH 100

// A class defining an FOCC_OTA scheduler.
class BOCC_CTA{
private:
    int counter;

    // number of transactions to store before triggering garbage collection

    std::vector<DataItem*> dataItems;

    std::unordered_map<int, Transaction*> transactions;

    /**
     * Use this lock (in shared/exclusive mode) before any scheduler operation!
     */
    std::shared_mutex scheduler_lock;
public:
    // number of data_items
    int M;

    // constructors
    explicit BOCC_CTA(int m) ;
    ~BOCC_CTA();

    // returns a transaction ID.
    int begin_trans();

    // tid performs read of dataItem indexed by dIdx onto variable store.
    int read(int tid, int dIdx, int *store);

    int write(int tid, int dIdx, const int value);

    char tryC(const int tid);

    void garbageCollect();
};

#endif //CCTS_BOCC_CTA_H
