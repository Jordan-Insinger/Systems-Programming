# Race Condition Scenario 1

These two critical regions access the count2, and mode member variable from the device. At the time of entering the both critical regions semaphore 1 is held, and in the case of the second critical region (release funcion), semaphore 2 is held in addition to semaphore 1. 

In error-free execution there should be no race condition because both critical regions are protected by semaphore 1, requiring that only one process hold the semaphore at a time regardless of semaphore 2. However, there is no error handling on the deown interruptible call. Thus, it is possible that a process executing critical region 1 receives an error code from down_interruptible, and instead of aquiring the semaphore or blocking, it just proceedes in to the critical region without aquiring the lock. This would cause both processes to be reading/writing to the count2 variable within the critical region. 

I believe this would result in unpredicted behavior in the branch condition within critical region 2. 

Code: 

```c
// ---------------- Critical Region 1: open mode 2 -------------------------------//
    down_interruptible(&devc->sem1);
    else if (devc->mode == MODE2) {
        devc->count2++;
    }
    // -----------------------------------------------------------------// 

    // ------------ Critical Region 2: release mode 2 ----------------------------//
    down_interruptible(&devc->sem1);
    else if (devc->mode == MODE2) {
        devc->count2--;
        if (devc->count2 == 1)
            wake_up_interruptible(&(devc->queue2));
    }
    // -----------------------------------------------------------------// 
```

# Race Condition Scenario 2

The read and write functions both write to the ramdisk in mode 1 after releasing semaphore 1. Within those critical regions, mode1 and the ramdisk are accessed while holding only semaphore 2. 

The driver assumes that there will only be one process accessing the criticla regions at a time because semaphore 2 is held. However, as mentioned in scenario 1, since there is no error path for down_interruptible, its possible for a process to bypass acquiring semaphore 1. 

This would result in two processess entering the critical regions, where one is a writer and the other is reading, leading to possible data invalidation. For example, a process could read from the ramdisk, check some condition on the data, then before it can act on that condition, another process may write over the data. 

The read/write functions should maintain semaphore 1 while writing/reading to/from the ramdisk and also create an error path.

```c
    // ---------------- Critical Region 1: read ----------//
    down_interruptible(&devc->sem1);
    if (devc->mode == MODE1) {
       up(&devc->sem1);
       if (*f_pos + count > ramdisk_size) {
          printk("Trying to read past end of buffer!\n");
          return ret;
       }
       ret = count - copy_to_user(buf, devc->ramdisk, count);
       *f_pos += ret;
    }
    // --------------------------------------------------// 

    // ------------ Critical Region 2: write ------------//
    down_interruptible(&devc->sem1);
    if (devc->mode == MODE1) {
	up(&devc->sem1);
        if (*f_pos + count > ramdisk_size) {
            printk("Trying to read past end of buffer!\n");
            return ret;
        }
        ret = count - copy_from_user(devc->ramdisk, buf, count);
        *f_pos += ret;
    }
    // --------------------------------------------------// 
```

# Race Condition Scenario 3

In this scenario the two critical regions are within the relese and ioctl functions. These specific sections are concerned with waiting in queue2 and waking up processes from queue2. These two critical regions access shared device information, specifically the count2, and queue2. In both critical regions semaphore 1 is aquired before entering, and in the second critical region semaphore 2 is acquired prior to releasing semaphore 1. 

There should be no issue with queue2 because even if there was a missed wakeup, it would loop through again and check the condition.

With successful aquisition of semaphore 1, this is not a data race on count2 because both regions correctly protect the region with the semaphore. However, as in previous scenarios there is no fault handling on down_interruptible so the ioctl wait loop may be able to prematurely exit the loop by reading count2 before aquiring semaphore 1. 

Code: 

```c
    // --------- Critical Region 1: release Mode 2 ----//
    down_interruptible(&devc->sem1);
    else if (devc->mode == MODE2) {
        devc->count2--;
        if (devc->count2 == 1)
            wake_up_interruptible(&(devc->queue2));
    }
    up(&devc->sem1);
    // ----------------------------------------------//

    // --------- Critical Region 2: IOCTL Mode 1 -----//
    down_interruptible(&(devc->sem1));
    if (devc->count2 > 1) {
        while (devc->count2 > 1) {
        up(&devc->sem1);
        wait_event_interruptible(devc->queue2, (devc->count2 == 1));
        down_interruptible(&devc->sem1);
        }
    }
    devc->mode = MODE1;
    devc->count2--;
    devc->count1++;
    down_interruptible(&devc->sem2);
    up(&devc->sem1);
    // ------------------------------------------------//
```

# Race Condition Scenario 4

Here, the two critical regions are the wait loop inside of the ioctl function when switching to mode 2, and the release function when called in mode 1. 

Within these critical regions the shared data accessed are count1 and queue1. Also, in both regions semaphore 1 is acquired at the beginning, and semaphore 2 is held throughout. 

There should be no race condition here. Both execute in mode 1 which is protected by semaphore 2 which is acquired in the open function. Then before accessing the count variable semahpore 1 is acquired. 

```c
// ---------------- Critical Region 1: ioctl mode 2 -----//
down_interruptible(&(devc->sem1));
while (devc->count1 > 1) {
    up(&devc->sem1);
    wait_event_interruptible(devc->queue1, (devc->count1 == 1));
    down_interruptible(&devc->sem1);
}
// ------------------------------------------------------//

// --------- Critical Region 2: release mode 1 -------//
    down_interruptible(&devc->sem1);
    if (devc->mode == MODE1) {
        devc->count1--;
        if (devc->count1 == 1)
            wake_up_interruptible(&(devc->queue1));
	up(&devc->sem2);
    }
    up(&devc->sem1);
// --------------------------------------------------//
```

