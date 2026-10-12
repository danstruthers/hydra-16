# Lab 10. Devices Are Files

> **THIS LAB:** Manual: Chapter 12, and sections 13.4, 13.6 and 13.7. About 45 minutes. You need a Hydra at its
> `/>` prompt. You'll type a few lines of HyForth exactly as they're printed: you don't need to know Forth yet.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Type `ls /dev`. ✎ Pick four names, and say which device you think each belongs to. (The table in section 12.3
   will tell you whether you're right.)

<div class="write l3"></div>

2. A control file can be read, to see a device's settings. ✎ What does each of these say?

| I typed | It said |
|---|---|
| `cat /dev/serctl` | |
| `cat /dev/consctl` | |
| `cat /dev/sd/0/ctl` | |
| `cat /dev/time` | |

3. A control file can be written, to give the device a command. First, a command it doesn't know:

```text
/> echo fly >/dev/gpio/ctl
```

   ✎ What did the Hydra say? Who said it: `echo`, or the driver?

<div class="write l2"></div>

4. Now a command it does know. This one makes pin 2 of the I/O header an output, and switches it on and off:

```text
/> echo out 2 >/dev/gpio/ctl
/> echo 1 >/dev/gpio/2
/> cat /dev/gpio/2
/> echo 0 >/dev/gpio/2
/> cat /dev/gpio/2
```

   ✎ What did the two `cat`s show?

<div class="write"></div>

5. The screen is a device too, and it takes its commands *inside* the text sent to it, as **escape sequences**
   (section 13.7). Type these three lines of HyForth, and then run them:

```text
/> : esc 27 emit ;
/> : colours esc ." [31mred " esc ." [32mgreen " esc ." [34mblue " esc ." [0m" cr ;
/> : flag esc ." [1;33;44m Hello, Hydra! " esc ." [0m" cr ;
/> colours
/> flag
```

   ✎ What did `colours` print, and in which colours? What does `ESC [0m` do? (Try leaving it off the end.)

<div class="write l2"></div>

6. ✎ Using the tables in section 13.7, write the escape sequence that does each job.

| The job | The sequence |
|---|---|
| Clear the whole screen | |
| Move the cursor to row 5, column 10 | |
| Text in bright green | |
| Text reversed (dark on light) | |

### Questions

1. ✎ What does a driver do? Why don't programs just talk to the chips themselves?

<div class="write l3"></div>

2. ✎ A control file "works in two directions". What are they?

<div class="write l2"></div>

3. ✎ Every device is a file, so commands that work on files work on devices. Give one example you've used.

<div class="write l2"></div>

4. ✎ What do these error messages mean, when a device gives them: `invalid argument`, `busy`, `no such device`?

<div class="write l3"></div>

5. ✎ Section 5.5 sped up the serial line with `echo b115200 >/dev/serctl`. What else has to change when you do
   that, and why?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | find a device's files in `/dev` |
| ☐ | read a control file, and write a command to one |
| ☐ | tell a driver's error from a typing mistake |
| ☐ | change the screen's colours with an escape sequence |

> **CHALLENGE:** ★ Write a HyForth word `banner` that clears the screen, moves to the middle, and prints your name
> in yellow on blue. Then write an rc script that does the same. (In rc, you'll need a real <kbd>ESC</kbd>
> character: section 13.7 and the entry for `echo` in Chapter 11 will help.)
