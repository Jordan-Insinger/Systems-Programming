# Deadlock Scenario 3:

This test demonstrates a dedlock which can happen when a thread is cancelled while in the driver's open function. If the thread is cancelled before it increments the count, the release function will be called and decrement the counter to -1. This can cause a bug where threads will get stuck in a circular wait, where it otherwise would have been caught by a count check.

## Thread Sequence:
1. Thread 1 opens the device, sleeps before count increments (Mode = 1, count1 = 0, sem2 held).
2. Userspace process kills thread 1 (release called, count2 = -1).
3. Thread 2 opens the device (Mode = 1, count1 = 0, sem2 held).
4. Thread 3 opens device (Mode = 1, count1 = 1, blocked on sem2).
4. Thread 2 calls E2_IOCMODE2 (sem2 released, count2 = 1, count1 = 0)
5. Thread 3 acquires sem2.
6. Thread 2 calls E2_IOCMODE1 (sem1 held, mode = 1, count2 = 0, count1 = 1)
7. Thread 2 is blocked trying to acquire sem2.
8. Thread 3 calls read.
9. Thread 3 is blocked trying to acquire sem1.
---------------------------------
Thread 2 Deadlocked on line 206.
Thread 3 Deadlocked on line 92.

## Relevant code segments: 
Multiple functions are used here, for reference see read, ioctl, and open.

## Note:
A modifed version of the driver code is used which inserts a sleep call in the open function to allow the test code to cancel the process. The modified code is called assignment5_d3.c. The test code is deadlock3.c, compiled as deadlock3_app.