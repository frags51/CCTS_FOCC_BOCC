//
// Created by supreets51 on 15/10/19.
//

#include <unordered_set>

#include "FOCC_CTA.h"
#include "DataItem.h"


FOCC_CTA::FOCC_CTA(int m): M(m){
    counter = 0;

    dataItems = std::vector<DataItem*>();
    for(int i=0; i<M; i++) dataItems.push_back(new DataItem(DataItem::INIT));

    transactions = std::map<int, Transaction*>();

}

// todo: new Transaction() here! maybe need to add lock
int FOCC_CTA::begin_trans() {
    scheduler_lock.lock();

    //if(transactions.size()>FOCC_OTA_GARB_THRESH) garbageCollect();

    int id = counter;
    transactions[id] = new Transaction(id, M);
    counter++;
    scheduler_lock.unlock();

    return id;
}

// return -2 if the current transaction was aborted by someone else.
int FOCC_CTA::read(int tid, int dIdx, int *store) {
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
    curItem->read_list.insert(cur);
    *store = curItem->value;
    // This function just adds dIdx to the readSet!
    cur->read(dIdx);

    cur->t_status_mtx.unlock();
    dataItems[dIdx]->lock.unlock();
    scheduler_lock.unlock_shared();

    return 0;
}

int FOCC_CTA::write(int tid, int dIdx, const int value) {
    scheduler_lock.lock_shared();
    transactions[tid]->write(dIdx, value);
    scheduler_lock.unlock_shared();
    return 0;
}

/**
 * Algorithm as per book.
 * @param tid The transcation ID trying to commit
 * @return 'c' if commit, 'a' if aborted.
 */
char FOCC_CTA::tryC(const int tid) {
    std::set<int> conf_trans{};

    scheduler_lock.lock_shared();

    // lock all the data items, get conflicting transactions.
    for(auto e: transactions[tid]->writeSet){
        // lock each data item.
        dataItems[e]->lock.lock();

        // todo: Possibility of these transactions (in the readList) being deleted?
        // todo: --Might--(or might not) need a lock on these transactions' status, if each transaction doesnt
        //  read AND write each data item it accesses (since multiple transactions can be in try commit at
        //  the same time - fine grained locking!
        //  Ans: No, because they are already in try-commit (and not read phase)
        // Seems to be 0, becauase we have acquired a shared_lock on the scheduler.
        // add all the conflicting and still running transactions here, they will be aborted.
        for(auto g: dataItems[e]->read_list){
            if(g->getStatus()=='s') conf_trans.insert(g->id);
        }
    }
    conf_trans.erase(tid);
    // abort this transaction if conflicting transactions are present!
    if(!conf_trans.empty()){
        transactions[tid]->setStatus('a');
        for(auto e: transactions[tid]->writeSet){
            dataItems[e]->read_list.erase(transactions[tid]);
            dataItems[e]->lock.unlock();
        }
        scheduler_lock.unlock_shared();
        return 'a';
    }
    else{
        transactions[tid]->setStatus('c');
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
void FOCC_CTA::garbageCollect() {
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

FOCC_CTA::~FOCC_CTA() {
    garbageCollect();
    for(auto e: dataItems){
        delete(e);
    }
}