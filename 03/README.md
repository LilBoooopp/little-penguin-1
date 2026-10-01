# Assignment 03 - Linux coding style

## Goal

Rewrite the given C file so it complies with the Linux kernel coding style (`Documentation/process/coding-style.rst`)

## Files

- `new_main.c` - the corrected file

## Style fixes

-- **Indentation:** tabs, 8 characters wide (chapter 1).
-- **Braces:** function opening braces on their own line; no braces around single-statement bodies (chapter 3). The `for` loop's braces were removed.
-- **Spacing and blank lines:** a blank line after local variable declarations.
-- **Comment placement:** the comment was moved above the `if` instead of sitting between the `if` and its brace-less body, which make the body easy to misread.
-- **Dead code:** the unreachable `retur: 1;` after `return; z;` was removed. Its indentation also suggested it belonged to a block that did not exist.
-- **Unused code:** the unsed `retval` parameter and the unused `<linux/slab.h>` include were removed.
-- **SPDX tag:** added `// SPDX-License-Identifier: GPL-2.0` as the first line.

## Kernel convention fixes

These are not formatting, but the file could not be a correct module without them:

- **`static`:** every function is only used in this file, so all are `static` (chapter 6).
- **`__init` / `__exit`:** added to `my_init` and `my_exit`.
- **Module metadata:** added `MODULE_LICESENSE("GPL")`, which recent kernels require to build, plus `MODULE_DESCRIPTION` (modpost warns without it) and `MODULE_AUTHOR`.
- **Log message:** added the missing `\n` to `pr_info`.
- **Pointer comparison:** the loop compared the `int` counter with the pointer `my_int`, a type error the compiler reports. It now compares with `y`, which already holds `*my_int`.
- **Init return value:** `my_init` returned the result of `do_work` (100), which is neither 0 nor a negative error code. The kernel warns about this with a stack trace (`init suspiciously returned 100, it should follow 0/-E convention`). Chapter 16 of the coding style separates action functions, which return 0 or `-Exxx`, from functions that compute a value. `my_init` is an action, so it now returns `0`, and `do_work` still returns its computed value.

## Behavior changes

The assignment only asks for style, but the original code contradicted its own comment, so two deliberate changes were made:

- **Condition `y < 10` -> `y > 10`.** The comment says the message reports a long sleep. The loop sleeps `y` times 10 microseconds, so a long sleep means a large `y`, not a small one.

## Verification

```
$ ~/linux/scripts/checkpatch.pl --no-tree -f new_main.c
total: 0 errors, 0 warnings, 40 lines checked

new_main.c has no obvious style problems and is ready for submission.
$ make
$ make insmod new_main.ko
$ dmesg | tail -1
$ rmmod new_main
```

The module builds without warnings and loads without the kernel's return-value warning.
