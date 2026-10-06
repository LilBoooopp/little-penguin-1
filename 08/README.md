# Assignment 08 - Fixing a broken module

## Goal

Fix the coding style and the behavior of the given module.

## Files

- `old_main.c` - the broken module
- `new_main.c` - the corrected module

## What the module does

It registers a misc device, `/dev/reverse`. Writing stores a string; reading returns it reversed.

```
$ echo hello > /dev/reverse
$ cat /dev/reverse
olleh
```

## Bugs in the original and their fixes

### Module conventions


| Original | Consequence | Fix |
|---|---|---|
| `MODULE_LICENSE("LICENSE")` | Not a recognized license: taints the kernel and blocks GPL-only kernel functions | `MODULE_LICENSE("GPL")`, plus an SPDX tag |
| Init ignored `misc_register()`'s result and returned `1` | A registration failure went unnoticed; a positive return triggers the kernel's "suspiciously returned" warning with a stack trace | Return `misc_register()`'s result |
| Exit was empty | `/dev/reverse` outlived the module, pointing at freed code: the next `open` after `rmmod` would crash the kernel | `misc_deregister()` in exit |

### Read

| Original | Consequence | Fix |
|---|---|---|
| `size_t; ... t >= 0; t--` | `size_t` is unsigned: `t--` at 0 wraps to the largest value, so the loop never ends and reads far outside `str`. An empty buffer wraps immediately (`strlen(str) - 1`) | Signed `int` index |
| `kmalloc(PAGE_SIZE * 2)` on every `read()`, never freed | Leaks 8 KB per call (`cat` calls `read()` at least twice), permannently until reboot | One buffer allocated in init, freed in exit |
| Allocation not checked | A failed `kmalloc` gives `NULL`, and the loop writes through it | Init returns `-ENOMEM` if the allocation fails |
| Global `tmp` shared by all readers | Concurrent readers overwrite each other's buffer | Protected by the mutex (see below) |

If `misc_register()` fails after the buffer was allocated, init frees it before returning the error, sinze exit never runs for a module that failed to load.

### Write

| Original | Consequence | Fix|
|---|---|---|
| `simple_write_to_buffer(str, size, ...)` | The buffer size given to the helper was the user's own `size`, so its bounds check always passed: writing more than 4096 bytes overflowed `str` into neighboring kernel memory | The size of `str` minus one byte reserved for the terminator |
| `+ 1` on the result | Reported one bbyte more than was written, and turned `-EFAULT` (-14) into `-EACCESS` (-13) | Return the helper's result unchanged |
| `str[size + 1] = 0x0` | Terminator one byte too far, leaving a stale byte; past the end of `str` for large writes | `str[*offs] = '\0'`, wrigh after the data written |
| No size check | A write that does not fit would make the helper return 0, and `echo` retries forever | Reject with `-ENOSPC` before writing |

The size check is written as `size > sizeof(str) - 1 - *offs` rather than an addition, so a huge `size` cannot overflow.

### Conccurency

`str` and `tmp` are shared by every reader and writer, with no protection. A **mutex** now covers the critical sections of both read and write. It is a mutex rather than a spinlock because `copy_to_user` / `copy_from_user` can sleep. The size check in write happens before locking, so no error path retusn while holding the lock.

### Style

Reformatted to the kernel coding style: tabs, brace placement, single spaces between types and names, `static` on all file-local functions and variables, `const` file operations, trailing commas in initializers, no `//` comments or joke comments, `'\0'` instead of `0x0`, and the redundant `&(*(&(myfd_device)))` reduced to `&myfd_device`. `checkpatch.pl` reports no errors or warnings.

## Desgin choice: the trailing newline

`echo hello` sends `hello\n`. Reversing every byte gives `\nolleh`: an empty line, then the string with the shell prompt stuck to it. The read function treats a trailing newline as a line ending rather than data: if the stored string ends with `\n`, the characters before it are reversed and the newline is kept at the end. Input without a newline is reversed as is, so each input keeps its own ending:

```
$ echo hello > /dev/reverse; cat /dev/reverse
olleh
$ echo -n hello > /dev/reverse; cat /dev/reverse
olleh$
```

## Proof

```
$ insmod new_main.ko
$ cat /dev/reverse
$ echo hello > /dev/reverse; cat /dev/reverse
olleh
$ head -c 5000 /dev/zero > /dev/reverse; echo $?
head: error writing 'standard output': No space left on device
1
$ scripts/checkpatch.pl --no-tree -f new_main.c
total: 0 errors, 0 warnings, 95 liens checked

new_main.c has no obvious style problems and is ready for submission.
```
