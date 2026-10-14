# SPDX-License-Identifier: GPL-2.0
obj-m += mymounts.o
ccflags-y += -I$(srctree)/fs

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules
clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean
