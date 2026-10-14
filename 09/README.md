# Assignment 09 - Listing mount points in /proc/mymounts

## Goal

Write a module that creates `/proc/mymounts`, listing every mount point on the system with an associated name:

```
$ cat /proc/mymounts
root       /
dev        /dev
run        /run
proc       /proc
sys        /sys
...
```

## Files

- `mymounts.c` - the module
- `Makefile` - portable module Makefile, with the kernel's private `fs/` directory added to the include path

## Creating the /proc file

`proc_create("mymounts", 0444, NULL, &mymounts_ops)` creates the file, readble by everyone, and `proc_remove()` deletes it on unload. procfs files take a `struct proc_ops` (fields prefixed `proc_`) instead of a `file_operations`. Unlike debugfs, procfs is a stable interface, so init checks the return value and fails with `-ENOMEM` if creation fails.

### seq_file

The output is generated, not stored: its length depends on how many mounts exist, and could exceed a page. The `seq_file` interface handles this. The module provides a *show* function that prints with `seq_printf()`; seq_file buffers the output, grows the buffer as needed, and handles file positions, partial reads and end of file.

`single_open()` is the simple form of seq_file: the show function prints the whole output in one call. The `proc_ops` uses the generic helpers `seq_read`, `seq_lseek` and `single_release`.

## Finding the mounts

### The data structures are private

Each process belongs to a **mount namespace** (`current->nsproxy->mnt_ns`), the set of mounts it can see. Each mount is a `struct mount`. Both `stuct mnt_namespace` and `struct mount` are defined in **`fs/mount.h`**, a header private to the VFS, not under `include/`. kbuild does not put `fs/` on a module's include path, and the VFS developers make nos tability promises about these structs.

The Makefile adds it explicitly:

```make
ccflags-y += -I$(srctree)/fs
```

`$(srctree)` is the source tree of the running kernel's build directory, so this stays portable across kernels, as long as that build directory is a full source tree. A distribution's kernel-headers package usually does not ship `fs/mount.h`, so this module would not build there.

### Not a linked list anymore

The subject hints at "linked list traversal": older kernels kept a `list_head` of mounts in `struct mnt_namespace`, walked with `list_for_each_entry`. In this kernel (7.3), the mounts are stored in a **red-black tree**, sorted by mount ID:

```c
struct mnt_namespace {}
	...
	struct rb_root mounts; // Protected by namespace_sem
	...
};
```

The module walks it with the same way the kernel's own `/proc/mounts` iterator does (`m_next` in `fs/namespace.`): `rb_first()` for the first node, `rb_next()` for each following one, and `rb_entry(node, struct mount, mnt_node)` to get from the tree node back to the `struct mount` containing it. Mount IDs are assigned in creation order, so the output comes out in the same order as `/proc/mounts`.

### Known limitation: the walk is unlocked

The tree is protected by `namespace_sem`, which is declared `static` in `fs/namespace.c`. It is not exported, so a module cannot take it, and none of the functions `fs/namespace.c` exports enumerate the mounts of a namespace.

As a result, if a filesystem is mounted or unmounted while `/proc/mymounts` is being read, the tree can be rebalanced or a `struct mount` freed under the walk, which can produce a wrong listing or a use-after-free. On a quiet system this practically never happens, but it is a real race that cannot be fixed from a module.

The fully safe alternative would be to read `/proc/self/mounts` from the kernel with the exported `filp_open()` and `kernel_read()` and reformat it, since the kernel's own iterator holds the lock. That only reformats an existing file instead of traversing the kernel's data structures, which is what this assignmnent is about, so the tree walk was kept.

## Building each line

For each mount, the module builds a `struct path` (the mount's embedded `strucutre vfsmount` plus its root dentry) and converts it to a full path with `d_path()`, which is exported. `d_path` writes from the end of the buffer, so it returns a pointer into the buffer, or an error pointer checked with `IS_ERR()`.

The **name** is the last component of that path: `/sys` -> `sys`, `/sys/kernel/debug` -> `debug`. Comparing with `/proc/mounts` shows the subject's names are not device names (`sysfs`, `tmpfs`, `devtmpfs`). The root mount `/` has no name component, so it is printed as `root` as in the subject.

## rootfs

The first version listed `root /` twice. The extra entry is **rootfs**: at the boot, before any disk is mounted, the kernel creates a small in-memory filesystem as the root of the first mount namespace. This happens even without an initramfs. The real root partition (`root=` on the kernel command line) is then mounted on top of it at `/`.

rootfs stays in the namespace as its base mount (`ns->root`), but nothing can reach it since the real root covers it completely. `/proc/mounts` hides it because it prints paths relative to the reader's root directory and skips uncheabale mounts (`SEQ_SKIP` in `show_vfsmnt`, `fs/proc_namespace.c`). The module does the same by skipping it explicitly:

```c
if (mnt == ns->root)
	continue;
```

## Proof

```
$ insmod mymounts.ko
$ cat /proc/mymounts
root       /
dev        /dev
run        /run
proc       /proc
sys        /sys
shm        /dev/shm
cgroup     /sys/fs/cgroup
boot       /boot
pts        /dev/pts
storage    /mnt/storage
debug      /sys/kernel/debug
```

SAme mounts, in the same order,  as `/proc/mounts`:

```
$ cat /proc/mounts
/dev/root / ext4 rw,relatime 0 0
devtmpfs /dev devtmpfs rw,relatime,size=10050532k,nr_inodes=2512633,mode=755 0 0
tmpfs /run tmpfs rw,relatime 0 0
proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0
sysfs /sys sysfs rw,nosuid,nodev,noexec,relatime 0 0
tmpfs /dev/shm tmpfs rw,nosuid,nodev,relatime 0 0
cgroup2 /sys/fs/cgroup cgroup2 rw,nosuid,nodev,noexec,relatime 0 0
/dev/sda3 /boot ext4 rw,relatime 0 0
devpts /dev/pts devpts rw,relatime,gid=5,mode=620,ptmxmode=000 0 0
/dev/sda1 /mnt/storage ext4 rw,relatime 0 0
debugfs /sys/kernel/debug debugfs rw,relatime,mode=755 0 0
```

The list is live: a new mount appears immediately.

```
$ mkdir -p /tmp/t && mount -t tmpfs none /tmp/t && cat /proc/mymounts
...
debug      /sys/kernel/debug
t          /tmp/t
$ umount /tmp/t
```

The file is readable by normal users (mode `0444`), and removed on unload:

```
$ rmmod mymounts
$ ls /proc/mymounts
ls: cannot access '/proc/mymounts': No such file or directory
```
