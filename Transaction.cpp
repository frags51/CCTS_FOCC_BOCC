//
// Created by supreets51 on 15/10/19.
//

#include "Transaction.h"


Transaction::Transaction(int i, int m_): id(i), m(m_), status('s') {
    writeSet = std::set<int>();
    readSet = std::set<int>();

    // initialize
    // Initially stores -1 for each item.
    local_write_store = std::vector<int>();
    for(int j=0; j<m; j++) local_write_store.push_back(-1);

}

char Transaction::getStatus() const {
    return this->status;
}

// Just inserts the DataItem index into the readSet. Do rest of the stuff inside the scheduler's read function
int Transaction::read(const int data_item) {
    readSet.insert(data_item);
    return 0;

}

int Transaction::write(const int data_item, int value) {
    local_write_store[data_item] = value;
    writeSet.insert(data_item);
    return 0;

}

void Transaction::setStatus(char a) {
    status = a;
}