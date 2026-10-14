# Project 4. Your Own Command

> **THIS PROJECT:** rc, the shell's language. After Labs 5 and 6. One or two sittings. You'll write a script with
> arguments, keep it where the shell finds it, and so add a command to your Hydra.

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The starting point

`jot` is a notebook. `jot` and some words adds a line, with the date and time in front. `jot` alone shows the
notebook. Type this into `/ram/jot` with `edit`. (It keeps the notebook in `/sd/0/work`: make that directory
first, if you haven't got one.)

<!-- listing samples/jot -->

```text
#!/bin/rc
# jot - a notebook.  jot words ... adds a line, with the date; jot alone shows them all
book=/sd/0/work/notebook
if(~ $#* 0) {
	cat $book
	exit
}
echo `{date} $* >>$book
```

Try it, as a script:

<!-- session
mkdir /sd/0/work
cd /ram
!file samples/jot
-->

```session
/ram> rc jot Feed the cat
/ram> rc jot Solder step 4
/ram> rc jot
2000-01-01 00:00:35 Feed the cat
2000-01-01 00:00:37 Solder step 4
```

Then make it a command, by copying it into `/bin`. Now it runs by its name, from any directory:

```text
/ram> cp jot /bin/jot
/ram> cd /
/> jot Buy solder
/> jot
```

### Understand it first

1. ✎ What do `$*` and `$#*` stand for? What's `$#*` when you type just `jot`?

<div class="write l2"></div>

2. ✎ What does `` `{date} `` do in the last line? What does `>>` do, and why not `>`?

<div class="write l2"></div>

3. ✎ You copied the script to `/bin`. Which directory is it really in? Will it be there tomorrow? Where should it
   go, so that it is? (Sections 9.7 and 14.2.)

<div class="write l3"></div>

### Make it yours

| ☐ | Change |
|---|---|
| ☐ | Keep the command for good: put it where it will still be after the power's been off |
| ☐ | Say something sensible when there's no notebook yet, in place of `cat`'s error |
| ☐ | `jot -n` shows how many entries there are (`wc -l`) |
| ☐ | `jot -f word` shows only the entries with that word in them (`grep`) |
| ☐ | `jot -l` shows only the last five (`tail`) |
| ☐ | `jot -x` empties the notebook, but only after asking "Are you sure?" |
| ☐ | A wrong option prints a usage line, as the Hydra's own commands do |

✎ Write your finished script here.

<div class="write l8"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | It runs by its name, from any directory, after a reset |
| ☐ | Its first comment says what it does, and how to use it |
| ☐ | Someone else has used it without being told how, just from its usage line |

> **CHALLENGE:** ★ Write a second command of your own invention. Some ideas: `backup`, which copies everything in
> `/sd/0/work` into a directory named after today's date; `where`, which says which directory a command's program
> is in; `count`, which counts the files in each directory it's given.
