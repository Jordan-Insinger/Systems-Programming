
obj-m := assignment5_d3.o

KERNEL_DIR = /usr/src/linux-headers-$(shell uname -r)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) modules
	
app: 
	gcc -o app1 deadlock1.c
	gcc -o app2 deadlock2.c
	gcc -o app3 deadlock3.c

clean:
	rm -rf *.o *.ko *.mod.* *.symvers *.order devuserapp
