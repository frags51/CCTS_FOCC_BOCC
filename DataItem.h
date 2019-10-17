//
// Created by supreets51 on 15/10/19.
//

#ifndef CCTS_DATAITEM_H
#define CCTS_DATAITEM_H

#include <set>
#include "Transaction.h"
#include <mutex>

struct TransCompare{
    bool operator() (const Transaction* t1, const Transaction* t2) const{
        return t1->id < t2->id;
    }
};

//todo: Can make read_list unordered_set
class DataItem{
public:
    // initial value to be stored in the data item
    static const int INIT = -2;

    int value;

    // store the transactions that read this in a sorted order.
    // NOTE: This is not an id, it is a pointer to that transaction!
    std::set<Transaction*, TransCompare> read_list;

    mutable std::mutex lock;

    // Constructors
    DataItem() = default;
    DataItem(int v);
};

#endif //CCTS_DATAITEM_H
