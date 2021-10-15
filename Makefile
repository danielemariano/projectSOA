obj-m += usctm.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mount:
	sudo insmod usctm.ko

remove:
	sudo rmmod usctm.ko
