//
// Created by supreets51 on 16/10/19.
//

/**
 * This algorithm aborts itself if there are conflicting transactions with this when it tries to commit.
 */

#ifndef CCTS_FOCC_CTA_H
#define CCTS_FOCC_CTA_H
#include <vector>
#include <mutex>
#include <map>
#include <unordered_map>
#include <shared_mutex>

#include "DataItem.h"

#define FOCC_CTA_GARB_THRESH 100

// A class defining an FOCC_OTA scheduler.
class FOCC_CTA{
private:
    int counter;

    // number of transactions to store before triggering garbage collection

    std::vector<DataItem*> dataItems;

    std::map<int, Transaction*> transactions;

    /**
     * Use this lock (in shared/exclusive mode) before any scheduler operation!
     */
    std::shared_mutex scheduler_lock;
public:
    // number of data_items
    int M;

    // constructors
    explicit FOCC_CTA(int m) ;
    ~FOCC_CTA();

    // returns a transaction ID.
    int begin_trans();

    // tid performs read of dataItem indexed by dIdx onto variable store.
    int read(int tid, int dIdx, int *store);

    int write(int tid, int dIdx, const int value);

    char tryC(const int tid);

    void garbageCollect();
};
#endif //CCTS_FOCC_CTA_H
