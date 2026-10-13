# Lab 3. First Steps

> **THIS LAB:** Manual: Chapter 6. About an hour. You need a Hydra at its `/>` prompt.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Type each command, and write down what the Hydra says.

| I typed | The Hydra said |
|---|---|
| `date` | |
| `echo Hello, Hydra!` | |
| `ls /sd/0` | |
| `2 3 + .` | |
| `free` | |

2. Now make these mistakes on purpose. Write down each message, and what it's telling you.

| I typed | The Hydra said | Which means |
|---|---|---|
| `dat` | | |
| `LS` | | |
| `2 3 plus .` | | |
| `2 3+ .` | | |
| `cat /ram/nothing` | | |

3. Type `sleep 100`. ✎ How do you get the prompt back without waiting?

<div class="write"></div>

4. Type `cat`, with nothing after it. Type a few lines. ✎ What does it do with them? How do you make it stop?

<div class="write l2"></div>

5. Start each program, look at its prompt, and leave it again.

| To start it | Its prompt | To leave it |
|---|---|---|
| `rc` | | |
| `basic` | | |
| `hylang` | | |
| `: test` (don't finish it!) | | |

6. Make a file of your own with the screen editor: `edit /ram/me.txt`. Type three lines about yourself. Save it,
   and leave the editor. ✎ Which keys saved it? Which left the editor?

<div class="write"></div>

7. Show your file with `cat`. Then keep it: make a directory for your work, and copy the file there.

```text
/> mkdir /sd/0/work
/> cp /ram/me.txt /sd/0/work/me.txt
/> ls -l /sd/0/work
```

   ✎ What does `ls -l` tell you about your file?

<div class="write l2"></div>

8. Run `/sd/0/sample/c/chorus`. ✎ What goes wrong the first time, and what puts it right?

<div class="write l3"></div>

### Questions

1. ✎ What is a prompt? What does the Hydra's prompt tell you, apart from "I'm ready"?

<div class="write l2"></div>

2. ✎ `echo one >/ram/f` and then `echo two >/ram/f`. What's in the file? What if the second had been `>>`?

<div class="write l2"></div>

3. ✎ Why is `/ram` a bad place to keep your work? Where's a good place, on your Hydra?

<div class="write l2"></div>

4. ✎ You type a command, and the Hydra says `not found`. Give three things that might be wrong.

<div class="write l3"></div>

5. ✎ Where would you look to find out what the command `wc` does?

<div class="write"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | type a command with arguments, and fix a line before I send it |
| ☐ | bring back the last line with <kbd>↑</kbd>, and change it |
| ☐ | stop a program, and end my typing |
| ☐ | tell from the prompt which program is listening, and leave it |
| ☐ | make a file with `edit`, look at it, copy it and remove it |
| ☐ | keep my work on the SD card |

> **CHALLENGE:** ★ In HyForth, define a word that prints your name: `: me ." ..." ;`. In HyBASIC, write a
> two-line program that prints your name five times. In hylang, make it print your name with `(print ...)`.
> Which felt most natural?
