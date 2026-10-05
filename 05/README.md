# Assignment 05 - Misc character device

## Goal

Turn the Assignment 01 module into a misc character device driver that creates `/dev/fortytwo`. Reading it returns my student login; writing to it succeeds if the data matches the login and fails with "invalid value" otherwise.

## Files

- `main.c` - the driver
- `Makefile` - same portable Makefile as Assignmnet 01

## Character devices and misc devices

A file in `/dev` holds no data. Calls like `open()`, `read()` and `write()` on it are passed by the kernel to a driver. A character device's node shows two numbers instead of a size: the **major** number selects the driver, the **minor** number selects which of that driver's devices was opened.

All **misc** devices share major 10. The misc core looks up the minor number to find the right driver, so a misc driver only needs a minor number and no registration of its own. The device uses `MISC_DYNAMIC_MINOR`, so the kernel assigns any free minor. When the device is registered with the name `fortytwo`, devtmpfs creates `/dev/fortytwo` automatically; unregistering removes it.

```
$ ls -l /dev/fortytwo
crw------- 1 root root 10, 258 Oct  5 22:08 /dev/fortytwo
$ grep fortytwo /proc/misc
258 fortytwo
```

## Code structure

The pieces point to each other: the `miscdevice` points to the `file_operations` table, which points to the read and write functions.

- `struct file_operations fortytwo_fops`: `.owner = THIS_MODULE` (prevents unloading while the device is open), `.read` and `.write`.
- `struct miscdevice fortytwo_dev`: dynamic minor, name `"fortytwo"`, and `.fops = &fortytwo_fops`.
- `misc_register()` in init (its error is returned if it fials) and `misc_deregister()` in exit.

### Read

The read function is one call to `simple_read_from_buffer()`, which:

- copies the login to userspace with `copy_to_user()` (the user buffer is `__user` memory and must never be accessed directly);
- tracks the file position `*ppos`, so it returns 0 (end of file) after the whole login has been read. Without this, `cat` would print the login forever, since it reads until it gets 0;
- handles reads smaller than the login.

### Write

1. If `count` is not the login's length, return `-EINVAL`. This also guarantees the copy fits in the buffer.
2. Copy the data into fixed-size local buffer with `copy_from_user()`. It returns the number of bytes **not** copied, so any nonzero result means a bad user pointer: return `-EFAULT`.
3. Compare with `memcmp()` (user data has no terminating `\0`, so not `strcmp`).
4. Return `count` on a match (all bytes accepted), `-EINVAL` otherwise.

The buffer has a size fixed at compile time (`sizeof(login - 1`). Variable-length arrawys are banned in the kernel, which has a very small stack.

## Design choice: the trailing newline

The login is stored as `"cbopp\n"`, including the newline. This keeps read and write symmetric (what `cat` returns is exactly what the device accepts back), makes `cat` output end cleanly, and makes the natural command `echo cbopp` work.
As a consequence, `echo -n cbopp`, which sends no newline, is rejected.

## Proof

```
$ make
$ insmod main.ko
$ cat /dev/fortytwo
cbopp
$ echo cbopp > /dev/fortytwo; echo $?
0
$ echo -n cbopp > /dev/fortytwo; echo $?
echo: write error: invalid argument
1
$ rmmod main
$ ls /dev/fortytwo
ls: cannot access '/dev/fortytwo': No such file or directory
```
