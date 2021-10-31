obj-m += syscall_filler.o driver.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	sudo insmod syscall_filler.ko
	sudo insmod driver.ko

remove:
	sudo rmmod syscall_filler.ko
	sudo rmmod driver.ko