
obj-m := assignment5.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) modules
	
app: 
	gcc -o deadlock3_app deadlock3.c

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order devuserapp
