NAME = matrix_driver
MAKECMD = make -C /lib/modules/$(shell uname -r)/build M=$(PWD)
obj-m += $(NAME).o

matrix_driver-objs += matrix_drv.o matrix_thread.o

$(NAME).ko:
	$(MAKECMD) modules

load: $(NAME).ko
	-unload
	sudo insmod $(NAME).ko

unload:
	sudo rmmod $(NAME).ko

clean:
	$(MAKECMD) clean