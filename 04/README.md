# Assignment 04 - Loading a module when a USB keyboard is plugged in

## Goal

Modify the Assignment 01 module so it is loaded automatically whenever any USB keyboard is plugged in, triggered by the userspace hotplug tools (udev and kmod on this LFS system).

## Files

- `usb.c` - the module, now with a USB device ID table
- `Makefile` - same portable Makefile as Assignment 01
- `99-usb-keyboard.rules` - udev rule, installed in `/etc/udev/rules.d/`

## How hotplug loading works

1. **The kernel detects the device.** The USB core reads the device's descriptors, including the class, usbclass and protocol of each interface.
2. **The kernels sens a uevent to userspace.** It contains `ACTION=add`, `SUBSYSTEM=usb`, `DEVTYPE=usb_interface` and a `MODALIAS` string encoding all descriptor fields. For the keyboard used for testing:

```
   usb:v3434p02A0d0100dc00dsc00dp00ic03isc01ip01in00
```

    `ic03 isc01 ip01` means interface class HID, subclass boot, protocol keyboard.
3. **udev receives the event** and checks it against its rules files.
4. **modprobe loads the module.** It looks up the modalias in `/lib/modules/$(uname -r)/modules.alias`, which `depmod` generates from the aliases declared inside every installed module.

The device level of a USB keyboard reports class `0/0/0` ("defined per interface"), so matching happens on the interface events, not the device events.

## The code: declaring a device table

```c
static const struct usb_device_kdb_id_table[] = {
    { USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                    USB_INTERFACE_SUBCLASS_BOOT,
                    USB_INTERFACE_PROTOCOL_KEYBOARD) },
    { }
};
MODULE_DEVICE_TABLE(usb, usb,kdb_id_table);
```

`USB_INTERFACE_INFO` matches the interface class/subclass/protocol from any vendor. `USB_DEVICE(vendor, product)` would only match one specific keyboard model, which would not satisfy "any USB keyboard". The protocol field matters: protocol 2 under the same class and subclass is a boot mouse.

`MODULE_DEVICE_TABLE` must be at file scope. The build turns it into an alias stored in the `.ko`, with wildcards for every field not specified:

```
$ modinfo usb.ko | grep alias
alias:          usb:v*p*d*dc*dsc*dp*ic03isc01ip01in*
```

The module does not need to be a real driver; the alias alone is enough for the hotplug tools to load it.

## Installing the module

modprobe only finds modules under `/lib/modules/$(uname -r)/`, so the module is installed there and the alias database regenerated:

```
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules_install
depmod -a
```

Check that the alias is registered and resolves to the module:

```
$ grep ic03isc01ip01 /lib/modules/$(uname -r)/modules.alias
alias usb:v*p*d*dc*dsc*dp*ic03isc01ip01in* usb
$ modprobe -n -v 'usb:v3434p02A0d0100dc00dsc00dp00ic03isc01ip01in00'
insmod /lib/modules/7.3.0-rc5-thor_kernel-cbopp-gfb0de51d305e/updates/usb.ko
```

## The udev rule

```
ACTION=="add", SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_interface", ATTR{bInterfaceClass}=="03", ATTR{bInterfaceSubClass}=="01", ATTR{bInterfaceProtocol}=="01", RUN+="/sbin/modprobe usb"
```

- `ACTION`, `SUBSYSTEM`: a USB device being added.
- `ENV{DEVTYPE}`: only the interface events, where the keyboard identity lives.
- `ATTR{...}`: the interface's sysfs attributes, compared as stringsd (two-digit hex).
- `RUN`: modprobe takes a module name, not a file name, and needs a full path since udev does not use the shell's `PATH`.

After installing the rule: `udevadm control --reload`.

### Two mechanisms

The standard `/usr/lib/udev/rules.d/80-drivers.rules` already loads modules for any event with a `MODALIAS`, which is enough on its own once the device table is in place. The custom rule loads the module by name. Both fire on the same event; whichever runs first loads the module, and the other finds it already loaded.

`udevadm test` shows the custom rule matching the keyboard's interface:

```
$ udevadm test /sys/bus/usb/devices/1-2:1.0 2>&1 | grep -i modprobe
1-2:1.0: /etc/udev/rules.d/99-usb-keyboard.rules:3 RUN '/sbin/modprobe usb'
  RUN{program} : /sbin/modprobe usb
```

## Proof

The keyboard is attached to the VirtualBox CM through **Devices -> USB**, which produces a real hot-plug event in the guest.

```
$ rmmod usb
$ lsmod | grep usb
$ dmesg -w
(keyboard attached)
[ ... ] USB module running!
```

The module is not unloaded when the keyboard is removed, which is normal Linux behavior: modprobe only loads modules. Before each test the module is removed with `rmmod`, since an already loaded module is not loaded again.
