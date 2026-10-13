# Lab 5. Talking to the Shell

> **THIS LAB:** Manual: Chapter 8, sections 8.1 to 8.10. About an hour. You need a Hydra at its prompt. Type `rc`
> first: this lab is typed at rc's `%` prompt. (`exit` goes back.)

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. A variable holds a list of words. Type each line, and write what it prints.

| I typed | It printed |
|---|---|
| `pets=(cat dog fish)` | (nothing) |
| `echo $pets` | |
| `echo $#pets` | |
| `echo $pets(2)` | |
| `echo I have a $pets(1)` | |
| `echo 'I have a $pets(1)'` | |

2. Send output to a file, add to it, and count its lines:

```text
% echo one >/ram/n.txt
% echo two >>/ram/n.txt
% echo three >>/ram/n.txt
% cat /ram/n.txt
% wc -l </ram/n.txt
```

   ✎ What did `wc -l` say? What does the `<` do?

<div class="write l2"></div>

3. A pipe, `|`, sends one program's output straight into another. ✎ Write what each prints, and what it's counting.

| I typed | It printed | What that is |
|---|---|---|
| `ls /rom/bin \| wc -l` | | |
| `ls /sd/0/sample \| wc -l` | | |
| `cat /ram/n.txt \| sort` | | |

4. A command's output can become words in another command. Try it:

```text
% echo today is `{date}
% now=`{date}
% echo $#now
```

   ✎ How many words is `date`'s output? What are they?

<div class="write"></div>

5. Every command ends with a **status**: empty for success, something else for failure.

```text
% ls /rom
% echo $status
% ls /nowhere
% echo $status
% ls /nowhere >[2]/dev/null || echo not there
% ls /rom >/dev/null && echo there
```

   ✎ What was `$status` after each `ls`? What do `||` and `&&` do?

<div class="write l3"></div>

6. A pattern stands for all the names that match it: `*` is anything, `?` any one character, and `[a-c]` any one of
   those. ✎ What does each of these list?

| I typed | What it listed |
|---|---|
| `ls /rom/bin/s*` | |
| `ls /rom/bin/??` | |
| `ls /rom/bin/[x-z]*` | |

### Questions

1. ✎ The Hydra has two shells in one. How does it decide whether a line is for HyForth or for rc? How can you make
   sure a line goes to rc?

<div class="write l2"></div>

2. ✎ What's the difference between `echo $pets` and `echo '$pets'`? When would you want quotes?

<div class="write l2"></div>

3. ✎ `>[2]/dev/null` made an error message disappear. Where did it go? What's the `2`?

<div class="write l2"></div>

4. ✎ When you type `ls`, how does the shell find the program? (Try `echo $path`, and `whatis ls`.)

<div class="write l2"></div>

5. ✎ Write one line that counts how many of the programs in `/rom/bin` have a name starting with `s`.

<div class="write"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | set a variable, and use all of it, part of it, and its length |
| ☐ | send a command's output to a file, and read its input from one |
| ☐ | join commands with a pipe |
| ☐ | test whether a command worked |
| ☐ | use `*`, `?` and `[ ]` to name many files at once |

> **CHALLENGE:** ★ In one line: list the names in `/rom/bin`, sort them backwards (`sort -r`), and show the first
> three (`head -3`). What are they? Then, in another line: how many of the names have an `e` in them (`grep e`)?
> A pipeline can join six programs at most (section 8.7). Why six? And how would you do a job that needs seven?
