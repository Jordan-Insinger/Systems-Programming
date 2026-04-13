# Deadlock Scenario 1:

This test demonstrates a deadlock which happens when two threads open the device in mode 1. Then the first thread calls the ioctl function to switch to mode 2. 

## Thread Sequence:
1. Thread 1 opens the device (Mode = 1, count1 = 1, sem2 held).
2. Thread 2 opens the device (Mode = 1, count1 = 2).
3. Thread 2 blocks on line 49 waiting to acquire sem2.
3. Thread 1 calls E2_IOCMODE2 (sem1 held)
4. Thread 2 blocks on line 158, waiting for count1 to drop to 1.

Relevant code segments: 
```c
int e2_open(struct inode *inode, struct file *filp)                             
    {                                                                               
        struct e2_dev *devc = container_of(inode->i_cdev, struct e2_dev, cdev);     
        filp->private_data = devc;                                                  
        down_interruptible(&devc->sem1);                                            
        if (devc->mode == MODE1) {                                                  
            devc->count1++;                                                         
            up(&devc->sem1);                                                        
            down_interruptible(&devc->sem2);                                        
            return 0;                                                               
        }                                                                           
        else if (devc->mode == MODE2) {                                             
            devc->count2++;                                                         
        }                                                                           
        up(&devc->sem1);                                                            
        return 0;                                                                   
    }

// static long e2_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
switch(cmd) {                                                               
        case E2_IOCMODE2:                                                        
           down_interruptible(&(devc->sem1));                                    
           if (devc->mode == MODE2) {                                            
              up(&devc->sem1);                                                   
              break;                                                             
           }                                                                     
           if (devc->count1 > 1) {                                               
              while (devc->count1 > 1) {                                         
                 up(&devc->sem1);                                                
                 wait_event_interruptible(devc->queue1, (devc->count1 == 1));    
                 down_interruptible(&devc->sem1);                                
              }                                                                  
           }           
        }
```

## Note:
No modifications needed to driver code. Test used: deadlock1.c, compiled as deadlock1_app.