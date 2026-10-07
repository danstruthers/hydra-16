# rc

rc is the Hydra's shell underneath everything: Plan 9's rc, small, in assembly, run in place from the paged ROM.
HyForth's and hylang's shells give it every line that isn't theirs (`rc -c`), C's `system()` runs it, and scripts
start with `#!/bin/rc`.  It's also a shell of its own: type `rc` at the HyForth prompt for an rc session (`exit`
or Ctrl-D ends it), or name `/bin/rc -l` in a card's `/lib/shell` to have it in every window.

This guide is a tour with examples (each run on the Hydra).  Plan 9's rc paper ("Rc — The Plan 9 Shell", Tom Duff)
is the full story; what's here is what the Hydra's has.

Contents: [Starting it](#starting-it) · [Commands](#commands) · [Words and quoting](#words-and-quoting) ·
[Variables](#variables) · [Redirection](#redirection) · [Pipelines and background](#pipelines-and-background) ·
[Control flow](#control-flow) · [Functions](#functions) · [Scripts](#scripts) · [Built-ins](#built-ins) ·
[At HyForth's prompt](#at-hyforths-prompt)

## Starting it

| Command | What runs |
| :------ | :-------- |
| `rc` | An rc session: a prompt (`% `), each line run, till `exit` or Ctrl-D |
| `rc -l` | The same as a login shell: its namespace made from `/rom/lib/namespace` (and a card's), then `/rom/lib/profile` |
| `rc -c 'line'` | The line run, and rc ends with its status |
| `rc file args` | The file's commands (`$0` the file, `$*` the args) |

At the prompt, the console edits the line for it: Backspace, Delete, Left and Right, Home and End (Ctrl-A, Ctrl-E),
Ctrl-U, and Up and Down through the last 8 lines.  Ctrl-C ends what's running (its note goes to the window's note
group), and rc goes on.  A command that goes on (an open `{`) gets the second prompt, a tab.

## Commands

A command is a program and its arguments, run in a task of its own and waited for.  `/bin` is searched for a bare
name (and `.`: `$path` is `(. /bin)`); a name with a `/` is used as it is.  A command's status is `$status`: its
exit message, or its code; empty or `0` is success.

| Form | Meaning |
| :--- | :------ |
| `a; b` | a, then b (a new line is the same) |
| `a & b` | a in the background (`$apid` its task), then b |
| `a && b`, `a \|\| b` | b only if a succeeds, or only if it fails |
| `! a` | a, its status turned over |
| `{ a; b }` | a group: one command |
| `# words` | a comment, to the line's end |

```
% ls /nosuch || echo it failed
ls: /nosuch: not found
it failed
% ! ~ a b && echo not the same
not the same
```

## Words and quoting

Words are separated by spaces and tabs.  `'...'` quotes: nothing inside is special, and `''` is a quote in it.
Globbing makes names from `*` (anything), `?` (one character) and `[a-z]` (one of a set; `[~a-z]` one not in
it), sorted, by reading the directories; a pattern that matches nothing stays as it is.

`^` joins words: two single words make one; a word and a list make a list, the word joined to each; two lists of the
same length join item by item.  A caret is put in for you between a word and a `$` right against it (`pre$x`).

```
% x=(apple banana cherry)
% echo $x^.txt
apple.txt banana.txt cherry.txt
% echo pre^$x
preapple prebanana precherry
% echo 'single quotes: $x stays'
single quotes: $x stays
```

## Variables

Every variable is a list of words: `x=word`, `x=(a b c)`, `x=()` (empty: as good as unset).

| Form | Value |
| :--- | :---- |
| `$x` | its words |
| `$#x` | how many |
| `$x(n)`, `$x(n-m)` | the nth word (from 1), or those from n to m |
| `$"x` | its words joined by spaces, one word |
| `` `{command} `` | the command's output, split into words at spaces, tabs and new lines |

```
% x=(apple banana cherry)
% echo $x(2) $#x
banana 3
% n=`{echo hello | wc -c}
% echo $n
6
```

Variables are the environment's: a program rc starts gets them (`/env/x` in its task, `getenv` in C, `$x` in
hylang), a list's words each with a zero byte after it.  `x=1 cmd` sets `x` for that command alone.

Some are rc's own: `$status`, `$apid` (the last background task), `$task` (rc's task), `$path`, `$prompt` (its
first word the prompt, its second the second prompt), `$*` and `$0` (a script's), `$window` (the shell's window, a
login shell's).

## Redirection

| Form | Meaning |
| :--- | :------ |
| `< file` | fd 0 from the file |
| `> file`, `>> file` | fd 1 to the file, made or emptied; or added to |
| `>[2] file` | fd 2 to the file (any fd: `>[3]`, `<[4]`) |
| `>[2=1]` | fd 2 made a copy of fd 1 |
| `>[2=]` | fd 2 closed |

```
% ls /nosuch >[2=1] | wc -l
      1
```

## Pipelines and background

`a | b` runs a and b at once, a's fd 1 into b's fd 0 (a pipe: `/dev/pipe`, 512 bytes held); `a |[2] b` is a's fd 2.
Each stage is a program; a stage that isn't one (a group, a function) runs as `rc -c` on its text.  The
pipeline's status is its last stage's.

`a &` runs a without waiting: `$apid` is its task, `wait` waits for the background tasks (`wait $apid` for one),
and `ps` shows them.

```
% ls /rom/bin | sort -r | head -3
sort
scom
mkfs
```

## Control flow

| Form | Meaning |
| :--- | :------ |
| `if(cmd) a` | a if cmd succeeds |
| `if not b` | right after an `if`: b if it didn't run its command |
| `for(x in words) a` | a for each word, `$x` it (`for(x) a`: `$*`'s) |
| `while(cmd) a` | a while cmd succeeds |
| `switch(word){ case pattern ... }` | the commands after the first `case` whose pattern matches |
| `~ subject pattern ...` | success if the subject matches one of the patterns (glob patterns, matched as words, not names) |

```
% for(f in $x) echo fruit: $f
fruit: apple
fruit: banana
fruit: cherry
% if(~ $x(1) apple) echo first is apple
first is apple
% switch($x(3)){
	case b*
	echo starts with b
	case c*
	echo starts with c
	}
starts with c
```

## Functions

`fn name { commands }` makes one; its arguments are `$*` and `$1` ...; `fn name` alone removes it.  Functions are
in the environment too (`fn#name`), so a child rc has them.  `whatis name` says what a name is: a function's text, a
variable's value, or a program's path.

```
% fn greet { echo hello, $1 }
% greet world
hello, world
% whatis greet
fn greet { echo hello, $1 }
```

## Scripts

A file of rc commands runs with `rc file args`, or by its own name if its first line is `#!/bin/rc` (rc runs a file
that isn't a program by the interpreter its `#!` line names: Plan 9's way).  `/rom/bin/scom` is one:

```
#!/bin/rc
# scom [-l | n]: the old riff ... a song: once; its loop n more times; or -l, till it's stopped
if(~ $1 -l) play -l /rom/songs/scom.zsm
if not play /rom/songs/scom.zsm $*
```

`. file` runs a file's commands in this rc (its variables and functions stay); `exit [status]` ends a script, or
rc.  `/rom/lib/profile` is what a login shell runs first; a card's `/lib/profile` comes after it.

Ctrl-C reaches a script's rc (and `rc -c`'s) as well as what it's running: rc waits for that to end, as at the
prompt (a program may take the note itself, finish up and say so), and then ends, with its status, rather than
going on to the next command.

## Built-ins

These change rc's own task, so they can't be programs:

| Built-in | What it does |
| :------- | :----------- |
| `cd [dir]` | the current directory (none: `$home`, else `/`) |
| `bind [-a \| -b] [-c] new old` | new at old in rc's namespace: in place of what's there, or `-a` after it / `-b` before it in a union; `-c` creates go there |
| `mount [-a \| -b] [-c] dev old [spec]` | a device's tree at old (`mount '#f' /mnt/c 1`: card 1's file system) |
| `unmount [new] old` | old's member new gone, or all of old |
| `newns` | the default namespace made again |
| `exit [status]`, `wait [task]`, `eval words`, `. file`, `builtin cmd`, `whatis name`, `shift [n]` | as Plan 9's |

`#` starts a comment, so a device's name is quoted: `ls '#c'`, `bind '#s' /dev/seg`.

## At HyForth's prompt

The login shell is HyForth (`forth -l`).  A line whose first word isn't a Forth word or a number is an rc line, run
whole by an rc of its own (`rc -c`) and waited for.  Pipes, redirection, globbing and quoting are rc's there, but each
line is a new rc: a variable or function set on one line is gone on the next (the environment's `$status` is kept
for the next line).  For an rc session, type `rc`; `%` before a line makes it rc's whatever its first word
(`% free` past a Forth word `free`).  `cd`, `bind`, `mount`, `unmount` and `newns` are Forth words there, taking
rc's arguments, so they change the shell's own directory and namespace.  [hyforth.md](hyforth.md) has the rest.
