# Assignment 07 - debugfs

## Goal

Modify the ASsignment 01 module to create a debugfs directory `fortytwo` containing three files:

| File | Mode | Behavior |
|---|---|---|
| `id` | `0666` | Same as ASsignment 05: read returns the login, write succeeds only if it matches |
| `jiffies` | `0444` | Read returns the current value of the kernel's `jiffies` counter |
| `foo` | `0644` | Stores up to one page of written data; read returns it. Protected by a lock |

## Files

- `debugfs.c` - the module
- `Makefile` - same portable Makefile as Assignment 01

## debugfs

A misc device in `/dev` is part of the kernel's ABI: once programs rely on it, it must never change. debugfs is an in-memory filesystem for developers to expose internal information, with no stability rules. It needs no device numbers or registration: the module creates files directly.

`CONFIG_DEBUG_FS=y` is enabled in the kernel. LFS does not mount debugfs by default, so it is mounted through `/etc/fstab`:

```
debugfs /sys/kernel/debug debugfs mode=0755 0 0
```

### Permissions of the debugfs directory

`/sys/kernel/debug` is `drwx-------` (root only) by default, because debugfs exposes kernel internals. Opening a file requries execute (search) permission on every directory in its path, so without changing this, a normal user could never reach `fortytwo/id`, whatever its own mode. The subject says to use `chown`, but what is needed is read and execute permission for others, which is a mode change: `chmod 755 /sys/kernel/debug`. Since debugfs lives in memory, that resets at every boot; the `mode=0755` mount option makes it permanent. The `fortytwo` firectory created by the module is `0755` by default.

## Code strucutre

```c
fortytwo_dir = debugfs_create_dir("fortytwo", NULL);
debugfs_create_file("id", 0666, fortytwo_dir, NULL, &id_fops);
...
```

`debugfs_create_file()` takes the name, mode, parent directory, an optional data pointer and a `file_operations` table. The return values of the debugfs functions are deliverately not checked: the API is designed so a failure can be ignored safely, because a debugging feature should never prevent a driver from loading.

On unload, `debugfs_remove recursive(fortytwo_dir)` removes the directory and all its files, then the buffer for `foo` is freed. The order matters: removing the files first guarantees nobody can still be using the buffer when it is freed.

### id

The read and write functions from ASsignment 05, unchanged, including teh trailing newline choice: `echo cbopp` is accepted, `echo -n cbopp` is not.

### jiffies

`jiffies` counts timer interrupts since boot, incremented `CONFIG_HZ` times per second (1000 on this kernel). It starts five minutes before a 32-bit wraparound, so wraparound bugs only show up early. The read function formats it as text with `snprintf("%lu\n")` into a local buffer and passes the resulting length to `simple_read_from_buffer()`.

### foo

- **Storage:** a one-page buffer (`PAGE_SIZE`) allocated in init with `kzalloc()`, plus the stored length. `kzalloc` zeros the memory. With `kmalloc`, a write at an offset past the current end would leave a gap of old kernel memory that any user could read back. The allocation happens before any file is created, so if it fails, init returns `-ENOMEM` with nothing to undo.
- **Write:** `simple_write_to_buffer()` copies the data at the file position. On success, the stored length becomes the new position, so a shorter write replaces a longer one. Writes that do not fit in the page are rejected with `-ENOSPC` *before* the helper is called, because the helper returns 0 when tehre is no room, and a write returning 0 makes programs like `echo` retry forever. The check is written as `count > PAGE_SIZE - *ppos` rather than `*ppos + count > PAGE_SIZE`, so a huge `count` from userspace cannot overflow the addition.
- **Read:** `simple_read_from_buffer()` with the buffer and stored length.

### Locking

The buffer and its length must always be read and updated together. Without a lock, a read could see new data with the old length (or vice versa), and two writers could leave a mixture of both.

A **mutex** is used, not a spinlokc: the critical sections call `copy_to_user` / `copy_from_user`, which can sleep (for example if the user's memory has to be paged in), and a spinlock holder must never sleep. Both read and write take the lock; the size check in write happens before locking, so no errro path returns while holding it.

## Proof

```
$ insmod debugfs.ko
$ ls -l /sys/kernel/debug/fortytwo
total 0
-rw-r--r-- 1 root root 0 Oct  6 13:54 foo
-rw-rw-rw- 1 root root 0 Oct  6 13:50 id
-r--r--r-- 1 root root 0 Oct  6 13:45 jiffies
```

### As root

```
# cat jiffies; sleep 5; cat jiffies
4307221946
4307227629
# echo hello > foo; cat foo
hello
# echo hi > foo; cat foo
hi
# head -c 5000 /dev/zero > foo; echo $?
head: write error: No space left on device
1
```


### Concurrency

Two loops overwrite `foo` with strings of different lengths while a third process reads it 2000 times:

```
# while true; do echo aaaaaaaaaaaaaaaaa > foo; done &
# while true; do echo bbbb > foo; done &
# for i in $(seq 2000); do dd if=foo bs=4096 count=1 2>/dev/null; done | sort | uniq -c
    874 aaaaaaaaaaaaaaaaa
   1126 bbbb
# kill %1 %2
```

Each `dd` makes exactly one `read()` call. All 2000 reads return one complete string: the buffer and its length are always consistent.

The same test with `cat` gives 2275 lines, including a string never written (`aaaaaaaaaaaa`, 12 a's):

```
    275 aaaaaaaaaaaa
    909 aaaaaaaaaaaaaaaaa
   1091 bbbb
```

This is not a locking failure. `cat` calls `read()` repeatedly until it gets 0. If its first read returns `bbbb\n` (position now 5), and a writer stores the 18-byte string before its second read, that second read returns bytes 5 to 17 of the new contents: twelve `a`s and a newline. The mutex makes each `read()` call consistent; it cannot make several sepaate system calls consistent, since the lock is released between them.
