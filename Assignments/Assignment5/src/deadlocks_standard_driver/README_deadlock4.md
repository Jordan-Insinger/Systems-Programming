# Deadlock Scenario 4:

This last scenario showcases how a deadlock can happen if a process hangs while using the device. In this scenario I used the sleep function to simulate a process hanging. The result being that it holds its resources and the second process is unable to acquire them, resulting in a deadlock. 

## Thread Sequence:
1. Thread 1 opens the device (Mode = 1, count1 = 1, sem2 held).
2. Thread 1 calls ioctl, switches mode to 2 (Mode = 2, count 1 = 0, count2 = 1, sem2 released).
3. Thread 2 opens device (count2 = 2).
4. Thread 1 hangs (sleeps indefinitely).
5. Thread 2 calls ioctl, switching to mode 1.
6. Thread 2 deadlocks in wait loop. LINE 177.

## Relevant code:
```c
    if (devc->count2 > 1) {
        while (devc->count2 > 1) {
        up(&devc->sem1);
        wait_event_interruptible(devc->queue2, (devc->count2 == 1));
        down_interruptible(&devc->sem1);
        }
    }
```

## Note:
No modified driver is needed, deadlock4.c is used for this test, compiled as deadlock4_app.