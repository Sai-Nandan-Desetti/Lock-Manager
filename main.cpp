/**
 * @file main.cpp
 * @author nandand1998@gmail.com
 * @brief A lock manager application
 * @version 0.4 
 * 
 */

/**
 * @mainpage
 * Goal : Build a lock manager.
 * \n
 * - The lock manager should support the following capabilities:
 *      1. Lock a resource in either shared or exclusive mode.
 *      2. Unlock a resource held by a transaction.
 * - A resource will be identified by a string.
 * - A resource is locked in a 'mode' by a 'transaction'.
 * - The lock request may be granted or put on wait based on a lock compatibility matrix.
 * 
 */

#include <unordered_map>
#include <list>
#include <cassert>
#include <cstdint>
#include <iostream>

using namespace std;

// A structure that maintains information about a lock a txn has (on a resource)
class lock_record
{
private:
    uint32_t txn_id_;
    uint8_t lock_type_;   // SHARED, EXCLUSIVE
    uint8_t lock_status_; // GRANTED, WAITING

public:
    lock_record(uint32_t txn_id, uint8_t lock_type, uint8_t lock_status) : txn_id_(txn_id), lock_type_(lock_type), lock_status_(lock_status) {}

    uint32_t getTxnId()
    {
        return txn_id_;
    }
    uint8_t getLockType()
    {
        return lock_type_;
    }
    uint8_t getLockStatus()
    {
        return lock_status_;
    }

    bool setLockStatus(uint8_t st)
    {
        lock_status_ = st;
        return true;
    }
    bool setLockType(uint8_t lt)
    {
        lock_type_ = lt;
        return true;
    }
};

enum lockType
{
    SHARED,
    EXCLUSIVE
};

enum lockStatus
{
    GRANTED,
    WAITING
};

bool lock(std::string resource_name,
          std::uint32_t txn_id,
          uint8_t lock_type);

bool unlock(std::string resource_name,
            std::uint32_t txn_id);

bool isTxnWaitingOnResource(std::string resource_name,
                            std::uint32_t txn_id);
void print_locktable();

unordered_map<std::string, list<lock_record *> *> lock_table;

int main()
{
    uint8_t option, lock_type;
    uint32_t txn_id;
    string resource_name;

    cout << "\nLOCK MANAGER IMPLEMENTATION" << endl;
    do
    {
        cout << "Enter the option:" << endl;
        cout << "0. Lock" << endl;
        cout << "1. Unlock" << endl;        
        cin >> option;

        cout << "RESOURCE: ";
        cin >> resource_name;
        cout << "Txn Id: ";
        cin >> txn_id;

        if (isTxnWaitingOnResource(resource_name, txn_id))
        {
            cerr << "A transaction cannot make a lock/unlock request while it's still waiting for the resource!\n";
            goto GET_OPTION;
        }

        switch (option)
        {
        case '0':
            do
            {
                cout << "Lock type (S/X): ";
                cin >> lock_type;
                if (lock_type == 'S')
                {
                    lock_type = lockType::SHARED;
                    break;
                }
                else if (lock_type == 'X')
                {
                    lock_type = lockType::EXCLUSIVE;
                    break;
                }
                else
                    cerr << "Please enter a valid lock type! (S/X)\n";
            } while (true);

            if (!lock(resource_name, txn_id, lock_type))
                cerr << "Lock error!" << endl;
            break;

        case '1':
            if (!unlock(resource_name, txn_id))
                cerr << "Unlock error!" << endl;
            break;

        default:
            cerr << "Invalid option!" << endl;
        }

        print_locktable();

        GET_OPTION:
        cout << "\nDo you want to continue? [No(0)/Yes(1)]: " << endl;
        cin >> option;

    } while (option == '1');

    return 0;
}

// FUNCTIONS //

bool isTxnWaitingOnResource(std::string resource_name,
                            std::uint32_t txn_id)
{
    // Check if the data item exists in the lock table, or if it exists, check if it holds a non-empty list of lock records.
    if (lock_table.find(resource_name) != lock_table.end() && lock_table[resource_name]->size() != 0)
    {
        // Check if the requesting transaction is already waiting on the resource
        list<lock_record *>::iterator lr_iter;    
        for (lr_iter = lock_table[resource_name]->begin(); lr_iter != lock_table[resource_name]->end(); lr_iter++)
        {
            if ((*lr_iter)->getTxnId() == txn_id && (*lr_iter)->getLockStatus() == lockStatus::WAITING)
                return true;
        }
    }
    
    return false;
}

bool lock(std::string resource_name,
          std::uint32_t txn_id,
          uint8_t lock_type)
{
    try
    {
        // Check if the data item exists in the lock table, or if it exists, check if it holds a non-empty list of lock records.
        if (lock_table.find(resource_name) == lock_table.end() || lock_table[resource_name]->size() == 0)
        {
            // If the record does NOT exist in the lock table, then (the data item has no lock)
            //  - a list with a lock record has to be created
            //  - the lock table should be updated with the list.

            // Create a `lock_record`
            // items not (currently) locked are always granted a lock request
            lock_record *lr = new lock_record(txn_id, lock_type, lockStatus::GRANTED);

            // Create a list to store the lock records
            list<lock_record *> *lst = new list<lock_record *>;
            // and append the (newly constructed) lock record to the list
            lst->emplace_back(lr);

            // Update the `lock_table`
            lock_table[resource_name] = lst;
        }
        else
        {
            // The data item maintains some lock(s)

            // Check if the requesting transaction already has a lock record in the list
            list<lock_record *>::iterator lr_iter;
            bool txn_prereq = false;
            for (lr_iter = lock_table[resource_name]->begin(); lr_iter != lock_table[resource_name]->end(); lr_iter++)
            {
                if ((*lr_iter)->getTxnId() == txn_id)
                {
                    txn_prereq = true;
                    break;
                }
            }

            // Determine whether to grant the requested lock or not
            // A lock is granted if
            //     1. the requested lock is compatible with all the locks held by the data item
            //     2. if earlier requests are all granted

            uint8_t lock_status = lockStatus::WAITING;

            // If a txn requests a shared lock...
            if (lock_type == lockType::SHARED)
            {
                lock_record *record;

                // If all prior txns were granted shared locks...
                for (lr_iter = lock_table[resource_name]->begin(), record = *lr_iter;
                     lr_iter != lock_table[resource_name]->end() &&
                     record->getLockType() == lockType::SHARED &&
                     record->getLockStatus() == lockStatus::GRANTED;
                     lr_iter++, record = *lr_iter)
                    ;

                // ...then, the current txn too is granted the lock
                if (lr_iter == lock_table[resource_name]->end())
                    lock_status = lockStatus::GRANTED;
            }

            // `lock_status` remains in waiting state if
            //     1. the request is for an exclusive lock, or
            //     2. despite requesting for a shared lock, there's a prior txn that's still waiting
            lock_record *lr = new lock_record(txn_id, lock_type, lock_status);
            lock_table[resource_name]->emplace_back(lr);

            // unlock the previous lock issued by the txn
            if (txn_prereq)
                unlock(resource_name, txn_id);
        }
    }
    // catch memory allocation exceptions
    catch (bad_alloc &e)
    {
        cerr << "Memory allocation failure: " << e.what() << endl;
        return false;
    }
    return true;
}

bool unlock(std::string resource_name,
            std::uint32_t txn_id)
{
    // Check if the data item exists in the lock table, or if it exists, check if it holds a non-empty list of lock records
    if (lock_table.find(resource_name) == lock_table.end() || lock_table[resource_name]->size() == 0)
    {
        // Cannot unlock a data item that has no locks!
        cerr << "Unlocking on data that has no locks!" << endl;
        return false;
    }

    lock_record *record;
    list<lock_record *>::iterator lr_iter = lock_table[resource_name]->begin();
    list<lock_record *>::iterator del_iter;
    bool all_shared = true;

    int i;
    // Search for the txn that requested the unlock message
    for (i = 0, record = *lr_iter;
         lr_iter != lock_table[resource_name]->end() && txn_id != record->getTxnId();
         lr_iter++, i++, record = *lr_iter)
    {
        all_shared = all_shared ? (record->getLockType() == lockType::SHARED) : all_shared;
    }
    // Invalid txn id error
    if (lr_iter == lock_table[resource_name]->end())
    {
        cerr << "Invalid Txn Id!" << endl;
        return false;
    }
    // Store the iterator position that points to the record to be deleted
    // (In order to use the same iterator (`lr_iter`), we delete the record after the possible status updates.)
    del_iter = lr_iter;
    lr_iter++;

    // If the transaction is still waiting for the resource, then it makes no sense
    // for the transaction to request to unlock the resource!
    // record = *del_iter;
    // if (record->getLockStatus() == lockStatus::WAITING)
    // {
    //     cerr << "Cannot unlock for a transaction that is still waiting on the resource!\n";
    //     return false;
    // }

    // A lock's status is updated (granted) only if the previous locks are all shared
    // (The second operand here handles the case when the list has just one record (prior to deletion))
    if (all_shared && lr_iter != lock_table[resource_name]->end())
    {
        record = *lr_iter;
        if (record->getLockType() == lockType::EXCLUSIVE)
        {
            if (lr_iter == ++lock_table[resource_name]->begin()) // => `all_shared` is trivially true, and the exclusive record is now the first one in the list
                record->setLockStatus(lockStatus::GRANTED);
        }
        else
        {
            // Grant all shared locks that were waiting
            for (; lr_iter != lock_table[resource_name]->end() && record->getLockType() == lockType::SHARED && record->getLockStatus() == lockStatus::WAITING;
                 lr_iter++, record = *lr_iter)
            {
                record->setLockStatus(lockStatus::GRANTED);
            }
        }
    }
    // Delete the record from the record list
    lock_table[resource_name]->erase(del_iter);
    return true;
}

void print_locktable()
{
    cout << "\n----------------------------LOCK TABLE-----------------------------------" << endl;

    unordered_map<std::string, list<lock_record *> *>::iterator lt_iter;
    list<lock_record *>::iterator lr_iter;
    lock_record *record;

    for (lt_iter = lock_table.begin(); lt_iter != lock_table.end(); lt_iter++)
    {
        cout << "RESOURCE: " << lt_iter->first << endl;
        lr_iter = lock_table[lt_iter->first]->begin();
        for (; lr_iter != lock_table[lt_iter->first]->end(); lr_iter++)
        {
            record = *lr_iter;
            cout << "\t Txn_Id: " << record->getTxnId() << "\t";
            if (record->getLockType() == lockType::EXCLUSIVE)
                cout << "LockType: Exclusive";
            else
                cout << "LockType: Shared";
            cout << "\t";
            if (record->getLockStatus() == lockStatus::WAITING)
                cout << "Status: Waiting";
            else
                cout << "Status: Granted";
            cout << endl;
        }
    }
    cout << "-------------------------------------------------------------------------" << endl;
}
