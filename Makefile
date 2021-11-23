obj-m += syscall_filler.o driver.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	sudo insmod syscall_hacking.ko
	sudo insmod device_driver.ko

remove:
	sudo rmmod syscall_hacking.ko
	sudo rmmod device_driver.ko