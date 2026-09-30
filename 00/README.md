# Assignment 00

## Goal

Clone Linus Torvals' mainline Git tree, built it with `CONFIG_LOCALVERSION_AUTO=y`, install it on my LFS system and boot it.

## Files
- `boot.org` - kernel boot log (`dmesg` output captured right after booting)
- `.config` - the configuration used to build the kernel

## What I did

**1. Got the source.** Cloned the mainline tree from git.kernel.org:

```bash
git clone git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
```

**2. Configured it.** Started from the config of my working ft_linux kernel (6.16.1) instead of a genergic default, so the options my system needs to boot were kept: devtmpfs, and teh storage/filesystem drivers built in (`=y`) since LFS has no initramfs.

```bash
cp /boot/config-6.16.1 ~/linux/.config
cd linux/
make olddefconfig
make menuconfig
```

`olddefconfig` keeps my existing choices and gives default values to options added in newer kernels. In `menuconfig`, I enabled **General setup -> Automatically append version information to the version string** (`CONFIG_LOCALVERSION_AUTO=y`).

**3. Build it.**

```bash
make -j$(nproc) 2>&1 | tee build.log
```

**4. Installed it** (as root). LFS has no `installkernel`, so the kernel files were copied by hand:

```
make modules_install
KVER=$(make -s kernelrelease)
cp arch/x86/boot/bzImage /boot/vmlinuz-$KVER
cp System.map /boot/System.map-$KVER
cp .config /boot/config-$KVER
```

Moduels go into `/lib/modules/$KVER/`, so they don't touch the old kernel's modules.

**5. Added a GRUB entry.** Added a new `menuentry` to `/boot/grub/grub.cfg`, copied from the existing one, keeping the old kernel as the default in case the new one failed to boot. Since `/boot` is a separate partition, the kernel path is relative to that partition (`/vmlinux-...`).

**6. Booted and verified.**

```bash
$ uname -r
7.3.0-rc5-cbopp-00011-g6f8319e3e9a4
$ dmesg > boot.log
```

## Reading the version string

`7.3.0-rc5-cbopp-00011-g6f8319e3e9a4`

| Part | Meaning |
|---|---|
| `7.3.0-rc5` | Nearest tag in Linus's tree (release candidate 5) |
| `-cbopp` | My `CONFIG_LOCALVERSION`, kept from the LFS config |
| `-00011` | 11 commits past that tag |
| `-g6f8319e3e9a4` | Git commit hash, added by `CONFIG_LOCALVERSION_AUTO` |

The hash proves the kernel was built from a specific commit of the Git tree, not a release tarball. No `-dirty` suffix means the tree had no uncommited changes when it was built.
