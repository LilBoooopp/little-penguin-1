# Assignment 02 - Changing the kernel version with a patch

## Goal

Modify `EXTRAVERSION` in the kernel's top-level Makefile so the running kernel's version string includes `-thor_kernel`, and submit the change as a patch following the kernel's submission standards.

## Files

- `boot.log` - kernel boot log of the modified kernel
- `0001-kbuild-add-thor_kernel-to-EXTRAVERSION.patch` - the patch to the Makefile

## How the version string is built

```
7.3.0-rc5-thor_kernel-cbopp-gfb0de51d305e
```

| Part | Comes from |
|---|---|
| `7.3.0` | Makefile: `VERSION.PATCHLEVEL.SUBLEVEL` |
| `-rc5-thor_kernel` | Makefile: `EXTRAVERSION` |
| `-cbopp` | `.config`: `CONFIG_LOCALVERSION` |
| `-gfb0de51d305e` | Git commit hash, added by `CONFIG_LOCALVERSION_AUTO` |

The subject asks for the MAkefile, not `CONFIG_LOCALVERSION`: `EXTRAVERSION` is part of the source tree, so the change can be comitted and turned into a patch, whereas `CONFIG_LOCALVERSION` is a local build setting.

## The change

```
-EXTRAVERSION = -rc5
+EXTRAVERSION = -rc5-thor_kernel
```

`-rc5` was kept rather than replaced. Replacing it would produce `7.3.0-thor_kernel`, which looks like a build of the final 7.3 release, while the code is actually release candidate 5.

## Why the commit count disappeared

In Assignment 00 the version was `7.3.0-rc5-cbopp-00011-g6f8319e3e9a4`, where `-00011` meant 11 commits past the `v7.3-rc5` tag. After this change it is only `-g<hash>`.

`scripts/setlocalversion` derives the tag name to count from using the Makefile's version. With the new `EXTRAVERSION` it looks for a tag called `v7.3-rc5-thor_kernel`, which does not exist, so it cannot ocunt commits and prints only the commit hash. The hash still identifies exactly which commit the kernel was built from.

## Making the patch

The change was comitted with a message in the format described in `Documentation/process/submitting-patches.rst`:

- a subject line with a subsystem prefix (`kbuild:`, matching other commits to the Makefile) and an imperative summary:
- a body explaining what the change does and why, wrapped at 75 characters;
- a `Signed-off-by:` line (added with `git commit -s`), certifying the right to submit the code under the kernel's license.

```
git commit -s
git format-patch -1
scripts/checkpatch.pl 0001-kbuild-add-thor_kernel-to-EXTRAVERSION.patch
```

`git format-patch` turns the commit into an email-formatted patch file. `checkpatch.pl` reports no error or warnings.

## Rebuild and boot

Same process as Assignment 00: build, `make modules_install`, copy `bzImage`, `System.map` and `.config` to `/boot` under the new version name, add a GRUB entry, reboot.

```
$ uname -r
7.3.0-rc5-thor_kernel-cbopp-gfb0de51d305e
$ head -n 1 boot.log
[    0.000000] Linux version 7.3.0-rc5-thor_kernel-cbopp-gfb0de51d305e (root@cbopp) (gcc (GCC) 15.2.0, GNU ld (GNU Binutils) 2.45) #2 SMP PREEMPT_DYNAMIC Wed Sep 30 16:58:59 CEST 2026
```
