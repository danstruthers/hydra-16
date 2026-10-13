# Chapter 3. System Calls

A system call is a `jsr` to a fixed address in the jump table at $F800. Its arguments go in A, X, Y and the call
registers r0 to r15 (two bytes each, in the zero page from $02); its results come back in the same places, a
16-bit result in A (low byte) and X (high byte). On return, C = 0 means success. C = 1 means failure, and A
holds an error code (Appendix G). A call may change A, X, Y, the flags and r0 to r15, including registers it
doesn't use for a result; it never changes the program's own zero page, $22 to $7F.

A call that *can wait* may suspend the task until something happens elsewhere: a key, a disk, another task. A
note delivered to the task ends such a wait, and the call returns `E_INTR`.

Each call's name is defined in `hydra.inc` (`/lib/as`), with the constants and error codes used here. The
constants and the error codes are listed at the end of the chapter.

**How the entries are made.** The kernel is built from a specification that gives, for every call, its
registers in and out, the errors it can return and why, whether it can wait, and what it does. The entries of
this chapter are made from that same specification, so they describe the calls the kernel has, in the kernel's
own terms. Names in `THIS FORM` are constants or other calls. `OPEN` and `READ`, which most programs begin
with, are written out at more length.

**From the languages.** A call that a program can use, as opposed to one meant for drivers or the debugger,
can be made from each language:

* **hylang** has a function for it, `sys-` and the call's name in small letters. Each entry gives its
  arguments. A call that fails is an error carrying the system's message.
* **HyForth**, after `lib hydra`, has a word of the same name. It takes the call's inputs from the stack in the
  order the entry lists them, the first deepest, and leaves the results, with an *ior* on top if the call can
  fail. A path is passed as the address of a zero-terminated string, which `>z` makes (section 20.9).
* **HyBASIC** has `SYS "NAME" [, a [, x [, y]]]`, which sets A, X and Y. Arguments that go in the call
  registers must be put there with `POKE` first (r0 is at addresses 2 and 3, r1 at 4 and 5, and so on), and the
  results are read with `RREG` and `PEEK`.

## OPEN

System call $F8C0: opens a file that exists, and returns a file descriptor for it. It can wait.

#### Syntax

```text
            LDR r0, path        ; r0 = the address of the path
            lda #mode
            jsr OPEN
            bcs failed          ; A = the error code
            sta fd              ; A = the file descriptor
```

#### Arguments

r0
: The address of the path: its bytes, then a zero byte.

A
: The mode: one of `O_READ` (0), `O_WRITE` (1) or `O_RDWR` (2), to which can be added `O_TRUNC` ($10), to empty
  the file as it is opened, and `O_NONBLOCK` ($40), to make reads and writes of this descriptor return `E_AGAIN`
  where they would otherwise wait.

#### Result

C = 0, and A = the file descriptor: a number from 0 to 15, the lowest one the task has free.

#### Description

1. The path is made absolute and clean. One that doesn't start with `/` (or with `#`, a device's own name) has
   the task's current directory put before it; then `.` elements and doubled slashes are removed, and each `..`
   removes the element before it. The result may be at most 63 bytes (`PATH_MAX`).
2. The path is looked up in the task's namespace (section 9.5). Where a directory is a union of several (section 9.7), each
   member is tried in turn, and the first that has the name is the one opened. A member whose device has gone is
   passed over as if it didn't have the name.
3. The device's driver is asked to open the file in the mode given. If it refuses, its error is the call's.
4. The descriptor is set to the file, at offset 0.

A descriptor has an offset of its own, which `READ`, `WRITE` and `SEEK` move. Descriptors made from it by `DUP`,
and those a child task is given by `SPAWN`, share that offset; a second `OPEN` of the same file does not.

Opening a directory with `O_READ` succeeds: it reads as a sequence of 64-byte stat records, one for each file in
it. When the directory is a union, the records of all its members follow one another.

`OPEN` never makes a file. `CREATE` does, and opens it.

#### Errors

| Code | Name | Cause |
|---|---|---|
| $20 | `E_NOENT` | No such file, in any member of a union |
| $2B | `E_NODEV` | The path names a device that isn't there |
| $0A | `E_NAMETOOLONG` | The path, made absolute, is longer than 63 bytes |
| $26 | `E_MFILE` | The task has all 16 of its descriptors in use |
| $27 | `E_NFILE` | All 48 of the system's channels are in use (every open file in every task takes one; `DUP`s share) |
| $05 | `E_INTR` | A note arrived while the call was waiting for the driver |

A driver adds its own: `E_ROFS` ($29) for a write mode on a read-only disk, and `E_ISDIR` ($23) for a write mode on
a directory.

#### Notes

Descriptors 0, 1 and 2 are standard input, output and error, already open when a program starts. A task that
closes one of them will get that number from its next `OPEN`.

A descriptor that isn't closed is closed when the task ends.

#### From the languages

| Language | Form |
|---|---|
| HyForth (`lib hydra`) | `sys-open` ( *z-addr mode* -- *fd ior* ), the path zero-terminated (`>z` makes one) |
| hylang | `(sys-open path [mode])`: the descriptor. A failure is an error, its text the path and the system's message |
| HyBASIC | `SYS "OPEN"`; or, for most purposes, the `OPEN` statement (Chapter 1) |

#### See also

`CREATE`, `CLOSE`, `READ`, `WRITE`, `SEEK`, `DUP`, `FSTAT`.

#### Example

From hylang. Descriptors 0 to 2 are in use, so the first file opened is 3:

```session
hylang> (sys-open "/dev/time" 0)
=> 3
hylang> (sys-open "/dev/nothing" 0)
=> Error: /dev/nothing: not found
hylang> (sys-open "/rom/bin/ls" 1)
=> Error: /rom/bin/ls: read-only
hylang> (sys-open "/ram" 1)
=> Error: /ram: is a directory
hylang> (sys-open "#q/x" 0)
=> Error: #q/x: no such device
```

## READ

System call $F8C9: reads bytes from an open file into memory. It can wait.

#### Syntax

```text
            LDR r0, buf         ; r0 = where the bytes go
            LDR r1, count       ; r1 = how many, at most
            lda fd
            jsr READ
            bcs failed          ; A = the error code
                                ; A/X = how many were read; 0 is the end of the file
```

#### Arguments

A
: A file descriptor, open with `O_READ` or `O_RDWR`.

r0
: The address of a buffer in the task's memory.

r1
: The most bytes to read: 0 to 65,535. The buffer must have room for them.

#### Result

C = 0, and A/X = the number of bytes read, which were put in the buffer from its start. The descriptor's offset
has moved on by that many.

#### Description

The kernel asks the file's driver for the bytes at the descriptor's offset, at most 512 at a time (`IO_UNIT`),
and repeats until one of three things happens:

* **The count is reached.** The result equals r1.
* **The file ends.** The result is what there was before the end; a read that starts at the end returns 0. A
  result of 0 is how the end of a file is recognised, and it is not an error.
* **The driver returns fewer bytes than it was asked for.** The kernel stops there and returns them. This is how a
  device says "that is all for now": the console returns one line, however large the buffer.

A result smaller than r1 therefore does not by itself mean the end of the file. A program that needs exactly *n*
bytes must call `READ` again for the remainder until it has them or gets 0.

When the driver has nothing yet (no key typed, an empty pipe), the task waits. With `O_NONBLOCK` set on the
descriptor, the call returns `E_AGAIN` instead.

A directory reads as 64-byte stat records (`SR_SIZE`), one for each file in it: ask for a multiple of 64.

#### Errors

| Code | Name | Cause |
|---|---|---|
| $25 | `E_BADF` | A isn't an open descriptor, or it was opened with `O_WRITE` |
| $04 | `E_AGAIN` | Nothing to read yet, and the descriptor is non-blocking |
| $05 | `E_INTR` | A note arrived while the call was waiting |

A driver adds its own: a disk that fails answers `E_IO` ($2A).

#### From the languages

| Language | Form |
|---|---|
| HyForth (`lib hydra`) | `sys-read` ( *fd c-addr u* -- *u2 ior* ): *u2* bytes were read |
| hylang | `(sys-read fd count)`: the bytes read, as a string |
| HyBASIC | `SYS "READ"`; or `INPUT #`, `LINE INPUT #`, `INPUT$` and `GET` |

#### See also

`OPEN`, `WRITE`, `SEEK`, `CLOSE`, `PIPE`.

#### Example

The offset moving on, a count cut short by the end of the file, and 0 at the end:

<!-- session
echo hello world >/ram/t
hylang
-->

```session
hylang> (set {fd} (sys-open "/ram/t" 0))
=> NIL
hylang> (sys-read fd 5)
=> "hello"
hylang> (sys-read fd 100)
=> " world\n"
hylang> (sys-read fd 100)
=> ""
hylang> (sys-close fd)
=> NIL
```
