# Assignment 01 - Hello World kernel module

## Goal

Write a kernel module that logs `Hello world!` when it is loaded and `Cleaning up module.` when it is unloaded, with a Makefile that builds it on any system.

## Files

- `main.c` - the module source
- `Makefile` - builds the module against the running kernel

## How the module works

A module is a kernel code compiled in a `.ko` file and inserted into the running kernel without rebooting. It has no `main()` and no libc. Instead it has two entry points, registered with `module_init()` and `module_exit()`:

- `main_init()` runs on `insmod`. It logs `Hello world!` with `pr_info` and returns `0`, which tells the kernel loading succeeded. A negative error code would make the kernel refuse to load the module.
- `main_exit()` runs on `rmmode` and logs `Cleaning up module.`. It returns `void` because unloading cannot fail.

Messages go to the kernel log buffer, which `dmesg` reads. `MODULE_LICENSE("GPL")` declares the license: without it the kernel is tainted and GPL-only kernel functions are unavailable. The SPDX tag on the first line is the kernel's machine-readable license identifier for source files.

## Why the Makefile works on any system

A module must be compiled with the exact headers, configuration and flags of the kernel it will be loaded into, so it is built by the kernel's own build system (kbuild) rather than by calling `gcc` directly.

- `obj-m += main.o` tells kbuild to build `main.c` into `main.ko`.
- `make -C /lib/modules/$(uname -r)/build M=$(CURDIR) modules` switches into the build tree of the **running** kernel and builds the module in the current directory.

Nothing is hardcoded: `uname -r` gives the running kernel's version, and `/lib/modules/<version>/build` is the standard location of that kernel's build tree, created by `make modules_install`. kbuild stamps the module with that version (its *vermagic*), and the kernel refuses to load a module built for a different version, so the module must be rebuilt for each kernel it runs on.

## Build and test

```bash
$ make
$ insmod main.ko
$ dmesg | tail -1
[  126.934016] Hello world!
$ rmmod main
$ dmesg | tail -1
[  126.934016] Hello world!
```

`modinfo main.ko` shows the module's author, description, license and vermagic. `make clean` removes all build output.

The source passes `~/linux/scripts/checkpatch.pl --no-tree -f main.c` with no errors or warnings.
