//
// Created by supreets51 on 17/10/19.
//

#include <unordered_set>
#include <chrono>

#include "BOCC_CTA.h"
#include "DataItem.h"


BOCC_CTA::BOCC_CTA(int m): M(m){
    counter = 0;

    dataItems = std::vector<DataItem*>();
    for(int i=0; i<M; i++) dataItems.push_back(new DataItem(DataItem::INIT));

    transactions = std::unordered_map<int, Transaction*>();

}

// todo: new Transaction() here! maybe need to add lock
int BOCC_CTA::begin_trans() {
    scheduler_lock.lock();

    //if(transactions.size()>FOCC_OTA_GARB_THRESH) garbageCollect();

    int id = counter;
    transactions[id] = new Transaction(id, M);
    transactions[id]->start_time = std::chrono::high_resolution_clock::now();
    counter++;
    scheduler_lock.unlock();

    return id;
}

// return -2 if the current transaction was aborted by someone else.
int BOCC_CTA::read(int tid, int dIdx, int *store) {
    scheduler_lock.lock_shared();
    dataItems[dIdx]->lock.lock();

    Transaction *cur = transactions[tid];
    cur->t_status_mtx.lock();

    if(cur->getStatus()=='a'){

        // remove this transaction from the read list of this data item.
        dataItems[dIdx]->read_list.erase(cur);

        cur->t_status_mtx.unlock();
        dataItems[dIdx]->lock.unlock();
        scheduler_lock.unlock_shared();

        return -2;
    }

    DataItem* curItem = dataItems[dIdx];
    //curItem->read_list.insert(cur);
    *store = curItem->value;
    // This function just adds dIdx to the readSet!
    cur->read(dIdx);

    cur->t_status_mtx.unlock();
    dataItems[dIdx]->lock.unlock();
    scheduler_lock.unlock_shared();

    return 0;
}

int BOCC_CTA::write(int tid, int dIdx, const int value) {
    scheduler_lock.lock_shared();
    transactions[tid]->write(dIdx, value);
    dataItems[dIdx]->lock.lock();

    dataItems[dIdx]->read_list.insert(transactions[tid]);

    dataItems[dIdx]->lock.unlock();
    scheduler_lock.unlock_shared();
    return 0;
}

/**
 * Algorithm as per book.
 * @param tid The transcation ID trying to commit
 * @return 'c' if commit, 'a' if aborted.
 */
char BOCC_CTA::tryC(const int tid) {
    std::set<int> conf_trans{};

    scheduler_lock.lock_shared();

    // lock all the data items, get conflicting transactions.
    for(auto e: transactions[tid]->readSet){
        // lock each data item.
        dataItems[e]->lock.lock();


        // todo: Might (or might not) need a lock on these transactions' status, if each transaction doesnt
        //  read AND write each data item it accesses (since multiple transactions can be in try commit at
        //  the same time - fine grained locking!
        //  Solution: Lock all items in the current transaction's writeSet along with those in readSet!
        // todo: Possibility of these transactions (in the readList) being deleted?
        // Seems to be 0, becauase we have acquired a shared_lock on the scheduler.
        // add all the conflicting and still running transactions here, they will be aborted.
        for(auto g: dataItems[e]->read_list){
            if(g->getStatus()=='c' && (transactions[tid]->start_time < g->end_time)) conf_trans.insert(g->id);
        }
    }
    conf_trans.erase(tid);
    // abort this transaction if conflicting transactions are present!
    if(!conf_trans.empty()){
        transactions[tid]->setStatus('a');
        transactions[tid]->end_time = std::chrono::high_resolution_clock::now();
        for(auto e: transactions[tid]->readSet){
            dataItems[e]->read_list.erase(transactions[tid]);
            dataItems[e]->lock.unlock();
        }
        scheduler_lock.unlock_shared();
        return 'a';
    }
    else{
        transactions[tid]->setStatus('c');
        transactions[tid]->end_time = std::chrono::high_resolution_clock::now();
        for(auto e: transactions[tid]->writeSet){
            dataItems[e]->value = transactions[tid]->local_write_store[e];
            dataItems[e]->lock.unlock();
        }
        scheduler_lock.unlock_shared();
        return 'c';

    }
}

// todo: Some trans may not be cleaned!
// Use this AFTER obtaining exclusive lock on the scheduler!
void BOCC_CTA::garbageCollect() {
    std::unordered_set<int> freeables{};

    for(auto& [k, v]: transactions){
        // dont need to process still ongoing transactions!
        if(v->getStatus()=='s') continue;

        // remove this transaction from any read_lists!
        for(auto i: v->readSet){
            dataItems[i]->read_list.erase(v);
        }

        // remove this transaction from the hashmap.
        //transactions.erase(k);
        freeables.insert(k);
    }

    for(auto e: freeables) {delete transactions[e];transactions.erase(e);}
}

BOCC_CTA::~BOCC_CTA() {
    garbageCollect();
    for(auto e: dataItems){
        delete(e);
    }
}