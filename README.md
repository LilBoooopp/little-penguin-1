# Little Penguin

Linux kernel development exercises from the 42 curriculum: building and booting kernels, writing modules, and working the way kernel developers do: with Git, patches, coding style and the kernel's own documentation.

Each assigment lives in its own folder (`00` to `09`) with its own README explaining what was done and why.

## Inspiration: The Eudyptula Challenge

Little Penguin is adapted from the **Eudyptula Challenge**, a free, self-paced series of twenty Linux kernel programming tasks that started in early 2014. The name comes from *Eudyptula minor*, the little penguin, a nod to Tux. The challenge was run by email by a pseudonymous "Little Penguin" ("Little" for short). Each participant got one task at a time and could only move on once their solution had been reviewed and accepted. The tasks started with a "Hello World" module and built up to locking, memory allocation, kernel threads, networking and filesystems. The format was inspired by the Matasano Crypto Challenge.

The challenge became very popular: thousands signed up within months, and it later stopped accepting new participants. 42 turned its early tasks into this project, keeping the spirit: terse instructions, the kernel's own documentation as the main reference, and a strong emphasis on doing things the way kernel developers expect them to be done. The subject's references to `Documentation/CodingStyle`, `Documentation/SubmittingPatches` and *Linux Device Drivers, 3rd Edition* come straight from that tradition.

## Prerequisite: ft_linux

This project follows **ft_linux**, where I built a Linux system from source following *Linux From Scratch* (LFS 12.4). All work here was done on that system, running in a VirtualBox VM. That shaped several assigmnents:

- **No initramfs.** Anything needed to mount the root filesystem has to be built into the kernel (`=y`), not as a module. This mattered for every kernel built here and explains the `rootfs` entry found in Assignment 09.
- **Hand-writted boot configuration.** Kernels are installed by copying `bzImage`, `System.map` and `.config` into `/boot` and adding entries to `grub.cfg` by hand, since LFS has no `installkernel` or GRUB generator.
- **Minimal userspace.** debugfs isn't mounted by default, there is no kernel-headers package, and building modules relies on the full source tree that `/lib/modules/<version>/build` points to.

## Assignments

| | Topic | What it's about |
|---|---|---|
| [00](00/) | Building a kernel | Clone Linus's tree, build, install and boot it with `CONFIG_LOCALVERSION_AUTO` |
| [01](01/) | Hello World module | Module entry points, kernel logging, kbuild and a Makefile that works on any system |
| [02](02/) | Patches | Change `EXTRAVERSION`, commit it in the kernel's format and produce a patch with `git format-patch` |
| [03](03/) | Coding style | Fix a file to follow the kernel coding style, and the conventions `checkpatch.pl` can't see |
| [04](04/) | Hotplug | Load a module automatically when a USB keyboard is plugged in, through device tables, modaliases and udev |
| [05](05/) | Misc device | A character device, `/dev/fortytwo`, with read and write operations |
| [06](06/) | linux-next | What linux-next is, and building and booting it |
| [07](07/) | debugfs | Three debugs files, including one with shared state protected by a mutex |
| [08](08/) | Code review | Fix the style and the behavior of a deliverately broken misc device driver |
| [09](09/) | Mount points | List mount points in `/proc/mymounts` by walking the kernel's own mount structures |

## Kernels built

| Kernel | Assignment | Built in |
|---|---|---|
| `7.3.0-rc5-cbopp-00011-g6f8319e3e9a4` | 00 | `~/linux` (Linus's tree) |
| `7.3.0-rc5-thor_kernel-cbopp-gfb0de51d305e` | 02 | `~/linux`, with the `EXTRAVERSION` commit |
| `7.3.0-rc6-next-20261005-cbopp` | 06 | `/mnt/storage/linux-next`, a Git worktree |

The version strings themselves are a recurring theme. Each part comes from a different place: the Makefile, `CONFIG_LOCALVERSION`, `localversion*` files in the tree, and the Git state read by `scripts/setlocalversion`. Assignments 00, 02 and 06 each explain one piece of it.

## Things learned along the way

A few lessons came up across several assignments:

- **A module belongs to one kernel.** Every module is stamped with the version of the kernel it was built against (its *vermagic*), and a mismatched module is refused with `Invalid module format`. The stamp comes from the current state of the built tree, not the directory name, so rebuilding a kernel in the same tree changes it for every module built afterwards. This is why linux-next was built in a seperate worktree.
- **The kernel enforces conventions the compiler doesn't.** An init function returning something other than 0 or `-Exxx`, a missing license, a missing `\n`: none of these stop the build, but each has a visible effect at runtime.
- **Userspace memory is never trusted.** Data crosses the boundary only through `copy_to_user` / `copy_from_user` or helpers built on them, and every size that comes from userspace is checked before it is used. Assignment 08's original code is a collection of what happens otherwise.
- **Shared state needs a lock, and the right kings.** A mutex rather than a spinlock whenever the critical section can sleep, which copying to or from userspace can.
- **The subject can be older than the kernel.** Paths like `Documentation/CodingStyle` have moved to `Documentation/process/`, and the mount list from Assignment 09's hint is now a red-black tree. Checking the actual source tree was always more reliable than the subject's description of it.

## References

- `Documentation/process/` in the kernel tree, also at [docs.kernel.org](https://docs.kernel.org/process/), in particular `coding-style.rst`, `submitting-patches.rst` and `2.Process.rst`
- `Documentation/kbuild/modules.rst`: building external modules
- `Documentation/filesystems/seq_file.rst`: the seq_file interface
- *Linux Device Drivers, 3rd Edition* (Corbet, Rubini, Kroah-Hartman), freely available online, chapter 14 on the device model and hotplug
- [The Eudyptula Challenge](https://lwn.net/Articles/599231/), LWN.net, 2014
