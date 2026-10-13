# Lab 6. Scripts

> **THIS LAB:** Manual: Chapter 8, sections 8.11 to 8.14, and section 9.7. About an hour. You need a Hydra, and
> Lab 5 done. Type `rc` first: this lab is typed at rc's `%` prompt.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. A `for` loop does something for each word in a list. ✎ What does each of these print?

```text
% for(i in 1 2 3) echo number $i
% for(f in /sd/0/sample/*) echo I found $f
```

<div class="write l3"></div>

2. `if` runs a command when another succeeds. `~` is the command that tests whether a word matches a pattern.

```text
% name=Sam
% if(~ $name Sam) echo hello Sam
% if(~ $name S*) echo starts with S
% if(~ $name Ann) echo hello Ann
% if not echo you are not Ann
```

   ✎ Which lines printed something?

<div class="write l2"></div>

3. A function is a new command, made of others. `$1` is its first argument, and `$*` all of them.

```text
% fn greet { echo Hello, $1! }
% greet Sam
% fn shout { for(w in $*) echo $w! }
% shout one two three
```

   ✎ What did `shout` print?

<div class="write l2"></div>

4. A **script** is a file of commands. Make this one with `edit /ram/tour`:

```text
#!/bin/rc
# tour - say what's on each disk
for(d in /rom /ram /sd/0) {
	echo In $d there are:
	ls $d
}
```

   Run it with `rc /ram/tour`. ✎ What does the first line of the script do? What's the `#` line for?

<div class="write l2"></div>

5. Make your script a command of its own, by copying it into `/bin`. Then run it by its name, from anywhere:

```text
% cp /ram/tour /bin/tour
% cd /rom
% tour
```

   ✎ `/bin` is a union of several directories (section 9.7). Which real directory did your script go into? (Look
   with `ls /ram/bin`. The sample card has no `bin` directory yet: `mkdir /sd/0/bin` makes one, and it joins `/bin`
   the next time the Hydra starts. `newns` would join it at once, but it also empties your `/ram`, your script
   with it: section 9.8.)

<div class="write"></div>

6. Scripts take arguments too. Change `tour` so that it tours the directories you name, and all three if you name
   none. (`$#*` is the number of arguments, and `$*` is the list of them.) ✎ Write your new script here.

<div class="write l6"></div>

### Questions

1. ✎ What's the difference between a function and a script? When would you choose each?

<div class="write l2"></div>

2. ✎ In `for(f in /sd/0/sample/*)`, who turned the `*` into a list of names: `for`, `echo`, or the shell?

<div class="write"></div>

3. ✎ Your script is in `/ram/bin`. What happens to it when the Hydra is switched off? Where should a script you
   want to keep go?

<div class="write l2"></div>

4. ✎ What does `while(! ~ $i 0)` mean, word by word? (Section 8.11.)

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | write a `for` loop over a list, or over files |
| ☐ | test a word with `~`, and act on the answer |
| ☐ | write a function with arguments |
| ☐ | write a script, run it, and make it a command |

> **CHALLENGE:** ★ Write a script `countdown` that counts down from the number it's given to 1, and then prints
> "Lift off!". You'll need `while`, and `calc` to take 1 away: `` i=`{calc $i - 1} ``. Then make it wait a second
> between numbers.
