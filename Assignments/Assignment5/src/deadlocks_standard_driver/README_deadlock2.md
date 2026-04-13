# Deadlock Scenario 2:

This scenarios showcases a deadlock than can occur when two threads attempt to switch the device from Mode2 back to Mode 1 concurrently. The ioctl mode 1 path will wait until count2 drops to 1, but both threads will get stuck in this wait loop without being able to decrement the counter. 

## Thread Sequence:
1. Thread 1 opens the device (Mode = 1, count1 = 1, sem2 held).
2. Thread 1 calls E2_IOCMODE2 (Mode = 2, count2 = 1, count1 = 0, sem2 released).
3. Thread 2 opens the device (Mode = 2, count2 = 2)
4. Thread 2 calls E2_IOCMODE1
5. Thread 2 waits on line 177 for count2 to decrement.
6. Thread 1 calles E2_IOCMODE1 
7. Thread 1 waits on line 177 for count2 to decrement.

Relevant code segments: 
```c
       case E2_IOCMODE1:
          down_interruptible(&devc->sem1);
          if (devc->mode == MODE1) {
             up(&devc->sem1);
             break;
          }
          if (devc->count2 > 1) {
             while (devc->count2 > 1) {
                up(&devc->sem1);
                wait_event_interruptible(devc->queue2, (devc->count2 == 1));
                down_interruptible(&devc->sem1);
             }
          }
```

## Note:
No modifications needed to driver code. Test used: deadlock2.c, compiled as deadlock2_app.