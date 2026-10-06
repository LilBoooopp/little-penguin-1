# Assignmnet 06 - Building and booting linux-next

## Goal

Download the latest linux-next kernel, build it and boot it.

## Files

- `boot.log` - kernel boot log of the linux-next kernel

## What linux-next is

Documented in `Documentation/process/2.Process.rst`.

Kernel development runs in cycles of a few months. A two-week **merge window** opens each cycle, during which Linux pulls new features from subsystem maintainers' trees. After that, weekly release candidates (`-rc1`, `-rc2`, ...) accept only fuxes, until the final release.

While the rc releases are stabilizing, maintainers already queue features for the *next* merge window in their own trees. **linux-next** merges all of those queued branches on top of Linus's latest tree, every day. It is roughly what main line should look like after the next merge window.

Its purpose is to catch integrations problems early: two subsystem trees can each work on their own and still conflict or break each other once combined. Building abd booting linux-next is how those problems are found weeks before they would reach Linus's tree.

linux-next is **rebuilt from scratch every day**, not extended. Each day's release (tagged `next-YYYYMMDD`) has new merge commits and is not a decvendant of the previous day's, so its history is effectively rewritteen daily. It must be used by checking out a specific tag, never by `git pull`, which would try to merge two diverging histories.

## Getting the source

linux-next shares most of its history with Linus's tree, so it was added as a remote to the existing repository instead of cloned again:

```
cd ~/linux
git remote add linux-next https://git.kernel.org/pub/scm/linux/kernel/git/next/linux-next.git
git fetch linux-next --tags
git tag -l 'next-*' | sort | tail -1
```

The latest release was `next-20261005`.

### A separate worktree

Checking out linux-next in `~/linux` would change the build tree that the ASsignment 02 kernel's `/lib/modules/<version>/build` symlink points to, giving modules built for that kernel the wrong vermagic. Instead, a **git worktree** creates a secondary working directory that shares the repository's objects but has its own files aand built outpout:

```
git worktree add /mnt/storage/linux-next next-20261005
```

The worktree is on a separate partition (the old Debian partition from LFS host system, reformatted as ext4 and mounted at `/mnt/storage` through `/etc/fstab`), because the root partition did not have enough space for a second kernel build.

## Build and install

Same process as Assignment 00, starting from the existing working config:

```
cd /mnt/storage/linux-next
cp ~/linux/.config .config
make olddefconfig
make -j$(nproc)
make INSTALL_MOD_STRIP=1 modules_install
KVER=$(make -s kernelrelease)
cp arch/x86/boot/bzImage /boot/vmlinuz-$KVER
cp System.map /boot/System.map-$KVER
cp .config /boot/config-$KVER
```

`INSTALL_MOD_STRIP=1` strips debug symbols from the installed modules to save space on the root partition. A GRUB entry was then added for the new kernel.

## The version string

```
7.3.0-rc6-next-20261005-cbopp
```

| Part | Comes from |
|---|---|
| `7.3.0-rc6` | Makefile: version and `EXTRAVERSION` of the Linus tree linux-next is based on |
| `-next-20261005` | the file `localversion-next` in the top of the tree |
| `-cbopp` | `.config`: `CONFIG_LOCALVERSION` |

Any `localversion*` file in the top of the source tree has its contents appended to the version. linux-next ships on so every build identifies the day's release.

There is no `-g<hash>` suffix even though `CONFIG_LOCALVERSION_AUTO=y`. When a `localversion` file exists, `scripts/setlocalversion` uses its contents (`next-20261005`) as the tag to describe from. HEAD is exactly at that tag, and at a tagged commit the version is already unambiguous, so nothign is added.

## Proof

```
$ uname -r
7.3.0-rc6-next-20261005-cbopp
$ head -n 1 boot.log
[    0.000000] Linux version 7.3.0-rc6-next-20261005-cbopp (root@cbopp) (gcc (GCC) 15.2.0, GNU ld (GNU Binutils) 2.45) #1 SMP PREEMPT_DYNAMIC Tue Oct  6 10:49:32 CEST 2026
```
