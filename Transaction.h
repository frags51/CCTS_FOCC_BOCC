//
// Created by supreets51 on 15/10/19.
//
/**
 * A data structure for storing information related to a transaction.
 */

#ifndef CCTS_TRANSACTION_H
#define CCTS_TRANSACTION_H

#include <vector>
#include <set>
#include <mutex>

#include <chrono>

class Transaction {
private:
    // 'a': Abort, 'c": Commited, 's': Neither commit nor abort
    char status;

    // The number of different data items.
    const int m;


public:
    /// The transaction ID
    const int id;

    /// Lock for status.
    mutable std::mutex t_status_mtx;

    /// ids of data items are stored here.
    /// Using a set, because a set stores in sorted order!
    std::set<int> writeSet;
    std::set<int> readSet;

    // for storing writes until val-write phase.
    // Could use a map?
    std::vector<int> local_write_store;

    // For BOCC
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time; // commit/abort time

    /// Constructor
    Transaction(int i, int m_);

    /// Status getter/setter
    char getStatus() const;
    void setStatus(char a);

    /// the index of data_item and the value to write.
    int write(const int data_item, int value);

    /// Does only book-keeping, not an actual read.
    int read(const int data_item);

};


#endif //CCTS_TRANSACTION_H
