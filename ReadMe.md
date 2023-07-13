# Lock_Manager
The program implements a lock manager with the following capabilities:
1. Lock a resource in either shared or exclusive mode. 
2. Unlock a resource held by a transaction.

Compilation:
make

Run:
.\main.exe

Upon execution, the program provides instructions on how to use the lock manager.
The logic of the code is explained via comments contained in the program code.


NOTES:
1. If a resource has been locked once, it stays in the lock table forever (until the program terminates). 
    - I haven't deleted the resource entry from the lock table when the linked list of lock records it maintains becomes empty assuming that, if it was locked once, it may be locked again.
    - I didn't find it in the scope of this program to also delete resources from the lock table not used for a certain time.
