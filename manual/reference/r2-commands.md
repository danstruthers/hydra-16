# Chapter 2. Commands

A command is a program in `/bin`, or one of the shell's own. Each ends with an **exit status**, which rc keeps in
`$status`: empty for success, and a short text for failure. Most commands fail with the status `1`, and with
`usage` when their arguments make no sense. A command's messages go to standard error, as `name: what: why`, so
they aren't mixed into a pipe's data.

A path in an argument can be at most 63 characters long once it has been made absolute.

## cp

A program (`/bin/cp`): copies files, and with `-r` directories.

#### Syntax

```text
cp [-r] from to
cp [-r] from ... dir
```

#### Arguments

`-r`
: Copy directories too: each one's whole tree.

*from*
: A file to copy, or with `-r` a directory. There can be several if the last name is a directory.

*to*
: The copy's name. If it is an existing directory, the copy goes into it under *from*'s own name.

*dir*
: An existing directory. Each *from* is copied into it under its own name.

#### Description

Each *from* is handled in turn, and one that fails doesn't stop the rest.

A file is copied by opening it for reading, creating the destination, and reading and writing 512 bytes at a
time until a read returns nothing. Creating the destination empties a file that is already there. The copy is a
new file: it has its own date, the time of copying, and is not read-only even if the original is.

Before copying, `cp` compares the two files' device, disk and file identifier. If they are the same file, by
whatever two names, it refuses to copy it onto itself, which would otherwise empty it.

Anything that can be opened and read can be a *from*, a device included: `cp /dev/time now` copies what one read
of the clock gives.

Without `-r`, a *from* that is a directory is reported and skipped. With `-r`, the directory is made at the
destination (one already there is used as it is), and then everything in it is copied in the same way, down to
16 directories deep.

#### Exit status

Empty if every file was copied. `1` if any was not. `usage`, after a line saying how `cp` is used, if there are
fewer than two names or an option other than `-r`.

#### Errors

Each is printed as `cp: name: reason`, where *name* is the file that could not be read or written.

| Reason | Cause |
|---|---|
| `not found` | *from* isn't there, or a directory in *to*'s path isn't |
| `is a directory` | *from* is a directory, and there's no `-r` |
| `not a directory` | Several *from*s, and the last name isn't a directory |
| `the same file` | *from* and *to* are one file |
| `read-only` | The destination's disk can't be written (`/rom`) |
| `name too long` | The destination's path would pass 63 characters |

Any other reason is the system's own error message for what failed (Appendix G): a full disk, for one.

#### Notes

If a read or a write fails part-way, the destination is left holding what was written up to then.

`cp` doesn't ask before it overwrites.

#### See also

`mv`, `rm`, `mkdir`, `cat`, `cmp`.

#### Example

<!-- session
rc
cd /ram
-->

```session
% echo hello >a.txt
% cp a.txt b.txt; cat b.txt
hello
% mkdir d; cp a.txt b.txt d; ls d
a.txt
b.txt
% cp d e; echo $status
cp: d: is a directory
1
% cp -r d e; ls e
a.txt
b.txt
% cp a.txt a.txt
cp: a.txt: the same file
% cp a.txt /rom/x
cp: /rom/x: read-only
% cp a.txt; echo $status
usage: cp [-r] from to, or cp [-r] from ... dir
usage
```
