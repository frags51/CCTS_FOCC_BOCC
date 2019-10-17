//
// Created by supreets51 on 15/10/19.
//

#include <unordered_set>

#include "FOCC_OTA.h"
#include "DataItem.h"


FOCC_OTA::FOCC_OTA(int m): M(m){
    counter = 0;

    dataItems = std::vector<DataItem*>();
    for(int i=0; i<M; i++) dataItems.push_back(new DataItem(DataItem::INIT));

    transactions = std::map<int, Transaction*>();

}

// todo: new Transaction() here! maybe need to add lock
int FOCC_OTA::begin_trans() {
    scheduler_lock.lock();

    //if(transactions.size()>FOCC_OTA_GARB_THRESH) garbageCollect();

    int id = counter;
    transactions[id] = new Transaction(id, M);
    counter++;
    scheduler_lock.unlock();

    return id;
}

// return -2 if the current transaction was aborted by someone else.
int FOCC_OTA::read(int tid, int dIdx, int *store) {
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

int FOCC_OTA::write(int tid, int dIdx, const int value) {
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
char FOCC_OTA::tryC(const int tid) {
    std::set<int> conf_trans{};

    scheduler_lock.lock_shared();

    // lock all the data items, get conflicting transactions.
    for(auto e: transactions[tid]->writeSet){
        // lock each data item.
        dataItems[e]->lock.lock();

        // todo: Possibility of these transactions (in the readList) being deleted?
        // Seems to be 0, becauase we have acquired a shared_lock on the scheduler.
        // add all the conflicting and still running transactions here, they will be aborted.
        for(auto g: dataItems[e]->read_list){
            if(g->getStatus()=='s') conf_trans.insert(g->id);
        }
    }

    // this transaction is also conflicting!
    conf_trans.insert(tid);

    for(auto e: conf_trans){
        transactions[e]->t_status_mtx.lock();
    }

    // Check if someone aborted you!
    if(transactions[tid]->getStatus()=='a'){
        // unlock other transactions!
        for(auto e: conf_trans){
            transactions[e]->t_status_mtx.unlock();
        }
        for(auto e: transactions[tid]->writeSet){
            dataItems[e]->lock.unlock();
        }
        scheduler_lock.unlock_shared();

        return 'a';
    }
    else{ // can go ahead and abort everyone else!
        // remove current transaction from the conflicting transactions!
        conf_trans.erase(tid);

        // write these values!
        for(auto e: transactions[tid]->writeSet){
            dataItems[e]->value = transactions[tid]->local_write_store[e];
        }

        // Unlock everything!
        for(auto e: conf_trans){
            transactions[e]->setStatus('a');
            transactions[e]->t_status_mtx.unlock();
        }
        // need to unlock this too!
        transactions[tid]->setStatus('c');
        transactions[tid]->t_status_mtx.unlock();

        for(auto e: transactions[tid]->writeSet){
            dataItems[e]->lock.unlock();
        }

        scheduler_lock.unlock_shared();

        return 'c';
    }
}

// todo: Some trans may not be cleaned!
// Use this AFTER obtaining exclusive lock on the scheduler!
void FOCC_OTA::garbageCollect() {
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

FOCC_OTA::~FOCC_OTA() {
    garbageCollect();
    for(auto e: dataItems){
        delete(e);
    }
}