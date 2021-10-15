obj-m += syscall_hacking.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	sudo insmod syscall_hacking.ko

remove:
	sudo rmmod syscall_hacking.ko
