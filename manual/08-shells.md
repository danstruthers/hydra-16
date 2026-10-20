# Chapter 8. The Shells

A **shell** is the program that reads your commands and carries them out. It's called a shell because it's the
outside of the system: the part you touch. This chapter shows how HydraOS's shells work, and teaches you **rc**, the
Hydra's command language.

> **IN THIS CHAPTER:** You'll learn how HyForth and rc share the keyboard, and all of rc's language: variables,
> quotes, sending input and output to files, pipes, patterns, tests, loops, functions and scripts. **You need:**
> Chapter 6.

## 8.1 Two shells in one

When the Hydra starts, its shell is **HyForth**. HyForth is a programming language (Part Four teaches it), but it's
also a shell, because it hands every line that isn't Forth to a second shell, **rc**. The rule is simple:

* If the line's first word is a **Forth word** that HyForth knows, or a **number**, the line is Forth.
* Otherwise, the line is a **command** for rc.

```session
/> 2 3 + .
5
/> echo Hello
Hello
```

`2` is a number, so the first line is Forth. `echo` isn't a Forth word, so the second line is a command. (A plain
command like this one, just a program and its words, HyForth runs itself, in its own task, as rc would; anything more,
such as the `|` and `;` you'll meet below, it hands to rc.) You can type commands without thinking about Forth at
all.

Sometimes a program has the same name as a Forth word. To make sure rc gets a line, start it with `%` and a space:

```session
/> % echo This line is for rc
This line is for rc
```

You can also start rc itself, as a shell of its own, by typing `rc`. Its prompt is `%`. Type `exit` to leave it and
go back to HyForth:

```session
/> rc
% echo hello
hello
% exit
/>
```

The rest of this chapter's rc examples are typed at rc's `%` prompt, but they work just the same at HyForth's `/>`
prompt.

## 8.2 Commands

A command is a **program's name**, then its **arguments**: words that tell it what to do. The words are separated
by spaces:

```session
% echo one two three
one two three
```

Many programs take **options**: arguments starting with `-`, which change what they do. `ls -l` asks `ls` for a
long listing, with the files' sizes. Chapter 11 lists every program and its options.

**Where programs come from.** When you type a program's name, rc looks for it in the directories listed in the
variable `$path`:

```session
% echo $path
. /bin
```

`.` is the directory you're in, and `/bin` is where the Hydra's programs are. (`/bin` is really several directories
joined together: the ROM's programs, the ROM disk's, the SD card's and the RAM disks'. Chapter 9 explains how.) A name
with a `/` in it, like `/ram/myprog`, is found by its path, not by `$path`.

**Stopping a program.** <kbd>CTRL</kbd>-<kbd>C</kbd> stops the program that's running, and brings back the prompt.

## 8.3 rc: words and quotes

rc splits a line into words at spaces and tabs. Some characters have special jobs, and end a word too:

| Characters | Their job |
|---|---|
| `;` | Ends a command, so another can follow on the same line |
| `&` | Runs a command in the background |
| `|` | A pipe: one program's output into another's input |
| `< >` | Redirection: input from, or output to, a file |
| `^` | Joins two words into one |
| `$` | A variable's value |
| `` ` `` | A command's output, as words |
| `{ } ( )` | Group commands, or words |
| `=` | Sets a variable |
| `#` | A comment: rc ignores the rest of the line |

To use any of them in a word, **quote** the word with single quotes, `'...'`. Inside quotes, every character is
itself. A quote mark inside quotes is written twice:

```session
% echo 'it''s quoted: $x | ;'
it's quoted: $x | ;
```

A `\` at the very end of a line joins the next line to it, so a long command can be typed on two lines.

## 8.4 Variables

A **variable** is a name for some words. Set it with `=` (no spaces around the `=`), and use its value with `$`:

```session
% x=(apple banana cherry)
% echo $x
apple banana cherry
```

In rc, every variable is a **list** of words. The parentheses make a list of several words; a single word needs
none (`y=hello`). The list can be empty: `x=()`.

| Written | Gives |
|---|---|
| `$x` | All the words in `x` |
| `$#x` | How many words are in `x` |
| `$x(2)` | The second word (`$x(1 3)`: the first and third) |
| `$"x` | All the words, joined into one, with spaces between |

<!-- session
x=(apple banana cherry)
-->

```session
% echo $#x
3
% echo $x(2)
banana
```

Every variable is part of the **environment**, which the programs you run can read too. Some variables have
special jobs:

| Variable | What it holds |
|---|---|
| `$path` | Where programs are found: `. /bin` |
| `$prompt` | The prompt: its first word, and its second, which is shown when a command goes on to another line |
| `$status` | How the last command ended: empty or `0` for success; anything else is a failure |
| `$task` | This shell's task number |
| `$apid` | The task number of the last command run in the background |
| `$window` | The number of this shell's window (Chapter 10) |
| `$home` | Where `cd` goes with no directory |
| `$*` | A script's or function's arguments; `$1`, `$2` ... are each one, and `$0` is the script's name |

One more goes to each program rc starts, in its environment (`/env/prog`, or `getenv` in C), though it isn't a
variable of rc's own: `prog`, the path the program was started from (`/bin/ls`, `/ram/round`). A program that
starts copies of itself finds its own file by it.

## 8.5 Joining words

`^` joins two words into one. If either side is a list, it joins each word in turn:

```session
% y=hello; echo $y^.txt
hello.txt
% echo a^(1 2 3)
a1 a2 a3
```

When a variable is right next to other text, rc puts in the `^` for you: `$y.txt` means the same as `$y^.txt`.

> **NOTE:** Joining with an empty list is an error: if `$z` is empty, `$z^.txt` makes rc say
> `null list`.

## 8.6 Input and output

Every program has three **standard files**, numbered 0, 1 and 2:

| Number | Name | Normally |
|---|---|---|
| 0 | Standard input | Your keyboard |
| 1 | Standard output | Your screen |
| 2 | Standard error | Your screen: for error messages |

A **redirection** connects one of them to a file instead:

| Redirection | What it does |
|---|---|
| `>file` | Standard output into the file (made empty first) |
| `>>file` | Standard output added to the end of the file |
| `<file` | Standard input from the file |
| `>[2]file` | Standard error into the file (any number works: `>[1]` is the same as `>`) |
| `>[2=1]` | Standard error to wherever standard output goes |
| `>[2=]` | Standard error closed |

```session
% echo one >/ram/t.txt
% echo two >>/ram/t.txt
% cat /ram/t.txt
one
two
% wc -l </ram/t.txt
      2
```

The file `/dev/null` swallows whatever is written to it, which is handy for messages you don't want to see:

```session
% ls /nothing
ls: /nothing: not found
% ls /nothing >[2]/dev/null
```

## 8.7 Pipes

A **pipe**, `|`, connects one program's standard output to the next one's standard input, so the first program's
output becomes the second one's input. `wc -l` counts lines, so this counts the files on the ROM disk:

```session
% ls /rom | wc -l
      5
```

A line can pipe several programs together. This one lists the ROM disk's programs, keeps the names with an `e` in
them, sorts those backwards, and shows the first three:

```session
% ls /rom/bin | grep e | sort -r | head -3
xmodem
tee
sleep
```

And `|[2]` pipes standard error instead of standard output.

> **NOTE:** Each program in a pipeline is a task, and the Hydra has sixteen tasks for everything (`ps` shows
> them). On the book's Hydra that leaves room for nine programs in one pipeline. A tenth isn't started: rc says
> `no free task` for it, and the others run without it. For a longer job, put the half-way result in a file in
> `/ram`, and carry on from there.

## 8.8 A command's output as words

`` `{command} `` runs the command, and puts its output into the line as words:

```session
% echo today is `{date}
today is 2000-01-01 00:00:05
```

## 8.9 Success and failure

Every program ends with a **status**: success, or a number that says what went wrong. rc keeps it in `$status`:

```session
% ls /nothing
ls: /nothing: not found
% echo $status
1
```

Commands can be joined by what happened:

| Written | Means |
|---|---|
| `a && b` | Run `b` only if `a` succeeded |
| `a || b` | Run `b` only if `a` failed |
| `! a` | Run `a`, and turn success into failure, and failure into success |

```session
% ls /nothing >[2]/dev/null || echo not there
not there
```

## 8.10 Patterns

In a word that isn't quoted, `*`, `?` and `[` make a **pattern**, which rc replaces with the names of the files that
match it:

| Pattern | Matches |
|---|---|
| `*` | Any letters, or none |
| `?` | Any one letter |
| `[abc]` | One of the letters a, b or c |
| `[a-z]` | One letter from a to z |
| `[~a-z]` | One letter that isn't from a to z |

```session
% ls /rom/lib/[ns]*
/rom/lib/namespace
/rom/lib/shell
```

If no file matches, the word is left as it was.

The `~` command matches words against patterns, without looking at files: `~ word patterns` succeeds if the word
matches any of the patterns.

## 8.11 Decisions and loops

**if** runs a command if another succeeds. **if not** runs a command if the last `if`'s test failed:

<!-- session
x=(apple banana cherry)
-->

```session
% if(~ $x(1) apple) echo yes
yes
% if not echo no
```

**for** runs a command once for each word in a list, setting a variable to each word in turn:

```session
% for(i in 1 2 3) echo number $i
number 1
number 2
number 3
```

**while** runs a command as long as a test succeeds. `{ }` groups several commands into one. (`calc` is the
Hydra's calculator, Chapter 11.)

```session
% i=3; while(! ~ $i 0) { echo $i; i=`{calc $i - 1} }
3
2
1
```

A `while()` with nothing in its parentheses has no test to fail, so it goes round for ever, until
<kbd>CTRL</kbd>-<kbd>C</kbd> or an `exit` stops it. (An `if()` with nothing in them always runs its command.)

**switch** compares a word with patterns, and runs the commands after the first `case` that matches:

<!-- session
y=hello
-->

```session
% switch($y){ case h*; echo starts with h; case *; echo other }
starts with h
```

## 8.12 Functions

A **function** is a command you make yourself, from other commands. Inside it, `$*` is its arguments, and `$1`,
`$2` ... are each one:

```session
% fn greet { echo Hello, $1! }
% greet Sam
Hello, Sam!
% fn count { echo $#* things: $* }
% count a b c
3 things: a b c
```

`whatis` shows what a name is to rc: a function, a variable, a built-in, or a program, and where it is:

<!-- session
x=(apple banana cherry)
fn greet { echo Hello, $1! }
-->

```session
% whatis greet
fn greet { echo Hello, $1! }
% whatis ls cd x
/bin/ls
builtin cd
x=(apple banana cherry)
```

`fn greet`, with no `{ }`, removes the function again.

## 8.13 Scripts

A **script** is a file of commands. Its first line can be `#!/bin/rc`, which tells HydraOS to run the file with rc,
so you can run the script by its name, like a program. Here's a two-line script, made with `echo`:

```session
% echo '#!/bin/rc' >/ram/hi
% echo 'echo hello from $0, you said $*' >>/ram/hi
% rc /ram/hi one two
hello from /ram/hi, you said one two
```

You can run a script three ways:

| Typed | What happens |
|---|---|
| `rc file args` | A new rc runs the file's commands |
| `file args` | The same, if the file's first line is `#!/bin/rc` |
| `. file args` | This rc runs the file's commands itself, so the variables and functions the file sets stay set |

## 8.14 rc's built-in commands

A few commands change the shell itself (its directory, its variables, its namespace), so they can't be programs:
they're built into rc.

| Command | What it does |
|---|---|
| `cd [dir]` | Go to directory *dir* (with none: `$home`, or `/`) |
| `exit [status]` | End rc, with a status |
| `wait [task]` | Wait for the commands running in the background (or just one task) to end |
| `eval words` | Run the words as a command line |
| `. file [args]` | Run the file's commands in this shell |
| `builtin cmd args` | Run *cmd* as a built-in or a program, even if there's a function called *cmd* |
| `whatis names` | Say what each name is |
| `shift [n]` | Remove the first *n* (or 1) words from `$*` |
| `bind [-a|-b] [-c] new old` | Add a directory to the namespace (Chapter 9) |
| `mount [-a|-b] [-c] '#x' old [spec]` | Add a device's files to the namespace (Chapter 9) |
| `unmount [new] old` | Take something back out of the namespace (Chapter 9) |
| `newns` | Build the namespace again, as it was when the shell started (Chapter 9) |
| `exec cmd args` | Run *cmd* in the shell's place: the shell ends, and the program takes over its task |

rc itself can be started with options:

| Typed | What it does |
|---|---|
| `rc` | Reads commands from the keyboard, with a prompt |
| `rc file args` | Runs the file's commands, then ends |
| `rc -c 'commands'` | Runs the commands, then ends |
| `rc -l` | A **login shell**: builds the namespace and runs `/rom/lib/profile` first |
| `rc -i` | Reads commands with a prompt, even from a file |

## 8.15 HyForth's shell words

A command that rc runs, runs in a task of its own, so it can't change HyForth's own directory or namespace. So
HyForth has its own words for those jobs, and a few more. You type them as you'd type commands:

| Typed | What it does |
|---|---|
| `% command` | The rest of the line is rc's |
| `cd dir` | Go to a directory (with none: `$home`) |
| `bind`, `mount`, `unmount`, `newns` | The same as rc's (section 8.14) |
| `status .` | Print the status of the last command (0: success) |
| `exit`, or `bye` | Leave HyForth |
| `send n line` | Type a line into window *n*'s shell, as if you'd typed it there |

```session
/> ls /nothing
ls: /nothing: not found
/> status .
1
```

HyForth has more shell words, for programs to use: to run a command and keep what it printed, to start one in the
background, to read and set the environment. They take their values from Forth's stack, so they're in section 20.2,
once Chapter 19 has explained the stack.

**The prompt.** HyForth's prompt is made from a **format**: its letters are shown as they are, except for these:

| Code | Shows |
|---|---|
| `%d` | The directory you're in; on an SD card, the path on the card |
| `%v` | On an SD card, its number and a colon: `0:` |
| `%p` | The directory's whole path |
| `%t` | The shell's task number |
| `%w` | The window's number |
| `%%` | A `%` |

The usual format is `%v%d> `, which shows `/rom/lib> ` in `/rom/lib`, and `0:/games> ` in `/sd/0/games`. To use
another, type it between `s" ` and `"`, then the word `prompt`:

```session 1_/rom/lib>_
/> cd /rom/lib
/rom/lib> s" %t %p> " prompt
1 /rom/lib> s" %v%d> " prompt
/rom/lib>
```

## 8.16 Choosing your shell

Any of the Hydra's three languages can be your shell, as well as rc itself. Each language follows the same idea: a
line in the language is the language's, and any other line is rc's.

| Shell | Start it | Its lines are |
|---|---|---|
| HyForth | `forth -l` | Lines starting with a Forth word or a number |
| hylang | `hylang -l` | Lines starting with `(`, `{` or `[` |
| HyBASIC | `basic -l` | BASIC's statements |
| rc | `rc -l` | Everything |

The `-l` makes each one a **login shell**: it builds its namespace, and runs its **profile**, a file of commands
for it to run as it starts. Each has its own:

| Shell | Its profile |
|---|---|
| HyForth | `/lib/forth/profile.fs` |
| hylang | `/lib/hylang/profile.hl` |
| HyBASIC | `/lib/basic/profile.bas` |
| rc | `/rom/lib/profile` |

The shell init becomes in each window is named in the file `/lib/shell`. On the ROM disk it says:

```text
/bin/forth -l
```

To choose another, put a file called `shell` in the `lib` directory of your SD card (`/sd/0/lib/shell`), with one
line: `/bin/hylang -l`, say, or `rc -l`. `/lib` is a **union**: several directories seen as one, the card's
before the ROM disk's (Chapter 9 shows how it's made). So the card's file is found first. init reads the file again each time it starts a shell, so the simplest way to make the change is to
reset the Hydra.
