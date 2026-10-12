# Appendix G. Error Codes

When a system call fails, HydraOS gives an **error code**: a number that says what went wrong. Programs show it as
its **message**, usually after the name of the file or the thing it was about:

```session
/> cat nothing
cat: nothing: not found
```

## G.1 HydraOS's errors

| Code | Hex | Name | Message | What it usually means |
|---|---|---|---|---|
| 1 | $01 | `E_PERM` | not allowed | Not allowed: something only HydraOS or a driver may do, or a task that can't be debugged |
| 2 | $02 | `E_INVAL` | invalid argument | A call was given something it can't use: a bad number, a bad name, a wrong command in a ctl file |
| 3 | $03 | `E_NOSYS` | no such call | There is no such system call, or a device doesn't do that |
| 4 | $04 | `E_AGAIN` | not yet: try again | Not ready yet: nothing to read, with waiting turned off. Try again later |
| 5 | $05 | `E_INTR` | interrupted | A note (<kbd>CTRL</kbd>-<kbd>C</kbd>, say) stopped a call while it waited |
| 6 | $06 | `E_NOMEM` | out of memory | No memory left: no free RAM bank, no room in a program's memory |
| 7 | $07 | `E_BUSY` | busy | Something else is using it: a device opened by another program |
| 8 | $08 | `E_RANGE` | out of range | A number outside the range allowed |
| 9 | $09 | `E_FAULT` | bad address | An address a call can't use |
| 10 | $0A | `E_NAMETOOLONG` | name too long | A path longer than 63 characters, or a name longer than 31 |
| 11 | $0B | `E_TOOBIG` | too big | Too big: a program's arguments longer than 176 bytes in all, or a program too big for its task's memory |
| 32 | $20 | `E_NOENT` | not found | There's no file (or directory, or program, or variable) of that name: check the spelling, and `ls` |
| 33 | $21 | `E_EXIST` | already exists | Something of that name is there already |
| 34 | $22 | `E_NOTDIR` | not a directory | A part of the path isn't a directory |
| 35 | $23 | `E_ISDIR` | is a directory | It's a directory, and the call wanted a file |
| 36 | $24 | `E_NOTEMPTY` | directory not empty | A directory to be removed still has files in it |
| 37 | $25 | `E_BADF` | bad file descriptor | A file descriptor that isn't open (or isn't open for that: writing a file opened to read) |
| 38 | $26 | `E_MFILE` | no free file descriptor | The program has 16 files open already |
| 39 | $27 | `E_NFILE` | too many open files | Too many files are open in the whole Hydra (or in one device): close some |
| 40 | $28 | `E_NOSPC` | disk full | The disk is full |
| 41 | $29 | `E_ROFS` | read-only | The disk can only be read: the ROM disk |
| 42 | $2A | `E_IO` | i/o error | The hardware failed: an SD card that didn't answer, a bad read |
| 43 | $2B | `E_NODEV` | no such device | There's no such device, or its hardware isn't there (a Vera X, a card) |
| 44 | $2C | `E_PIPE` | broken pipe | Writing to a pipe whose reader has gone |
| 45 | $2D | `E_NOEXEC` | not a program | The file isn't a program the Hydra can run |
| 46 | $2E | `E_EOF` | end of file | The end of a file came too soon |
| 47 | $2F | `E_NOTFS` | no file system | The disk has no HydraFS file system on it: `mkfs` makes one |
| 48 | $30 | `E_MEDIA` | medium error | The disk (the card) has a fault, or was changed |
| 64 | $40 | `E_NOTASK` | no free task | All 16 tasks are in use: end a program or two |
| 65 | $41 | `E_SRCH` | no such task | There's no task of that number |
| 66 | $42 | `E_CHILD` | no such child | Waiting for a task that wasn't started by this one |
| 80 | $50 | `E_NSFULL` | namespace full | The namespace has too many binds and mounts |
| 81 | $51 | `E_NSLOOP` | too many binds | Binds that lead round in a circle |

## G.2 In each language

Each language shows these errors in its own way:

| Where | How an error shows |
|---|---|
| rc | The program's message; `$status` holds it (section 8.9) |
| Assembly language | C = 1 after the call, and the code in A; `ERRSTR` gives the message (section 26.13) |
| HyForth | An **ior** of -512 less the code (so `not found`, $20, is -544), which `throw` reports with its message; `ior>text` gives the message |
| hylang | An error whose message is "*name*: *message*", and whose code (`&code`, `(error-code e)`) is the name in small letters without its `E_`: `:noent` |
| HyBASIC | Error number 256 plus the code (so `not found` is 288), with its message (`ERR`, `ERR$`, `ERL`) |
| C | `errno` set to the nearest C error, and `_oserror` to the code |

The languages have errors of their own too: HyForth's `throw` codes (Appendix A), HyBASIC's errors (Appendix C), and
hylang's (Appendix B).

## G.3 The numbers library's errors

The numbers library (Chapter 18) has its own codes, which the languages show as their own errors:

| Code | What it means |
|---|---|
| 1 | Too big: an integer of more than 255 bytes, or a fixed decimal of more than 65,535 places |
| 2 | Division by zero |
| 3 | Not a number |
| 4 | No room for the result |
| 5 | Outside the function's domain (like the square root of a negative number, for a real result) |
| 6 | Not a base: a base string that names none (section 18.3) |
| 7 | An integer is needed |
| 8 | A real number is needed, not a complex one |
| 9 | The library wasn't set up (in assembly language: `NUM_INIT` first) |
| 10 | A format's placeholders and its values don't match |
