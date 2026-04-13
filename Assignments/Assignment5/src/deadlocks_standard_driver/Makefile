
obj-m := assignment5.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) modules
	
app: 
	gcc -o deadlock1_app deadlock1.c
	gcc -o deadlock2_app deadlock2.c
	gcc -o deadlock4_app deadlock4.c

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order devuserapp
