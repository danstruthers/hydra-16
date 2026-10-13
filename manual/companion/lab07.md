# Lab 7. The Tree of Files

> **THIS LAB:** Manual: Chapter 9. About 45 minutes. You need a Hydra at its `/>` prompt.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Walk around the tree. After each line, write where you are (the prompt tells you, and so does `pwd`).

| I typed | Where I am now |
|---|---|
| `cd /rom/lib/forth` | |
| `cd ..` | |
| `cd ../songs` | |
| `cd /` | |

2. ✎ Draw the part of the tree you just walked through: `/`, `rom`, `lib`, `forth` and `songs`.

<div class="space s2"></div>

3. Type `ls -l /rom`. ✎ How can you tell a directory from a file in a long listing? Which is the biggest file there?

<div class="write l2"></div>

4. Type `df`, and fill in the table.

| Disk | Where it appears | Its size | How much is free |
|---|---|---|---|
| | | | |
| | | | |
| | | | |
| | | | |

5. Devices look like files. ✎ What does each of these show?

| I typed | It showed |
|---|---|
| `cat /dev/time` | |
| `cat /dev/ticks` | |
| `ls /dev` (how many names?) | |

6. `bind` makes one directory appear in another place. Try it, and undo it:

```text
/> ls /mnt
/> bind /rom/songs /mnt
/> ls /mnt
/> unmount /mnt
/> ls /mnt
```

   ✎ What was in `/mnt` before, during and after? Were any files copied?

<div class="write l2"></div>

7. Type `ns`. It prints the recipe that built your whole tree. ✎ Copy the lines that end in `/bin`. How many
   directories is `/bin` made of?

<div class="write l4"></div>

### Questions

1. ✎ What's a path? What's the difference between `/ram/notes` and `notes`?

<div class="write l2"></div>

2. ✎ What do `.` and `..` mean?

<div class="write"></div>

3. ✎ The Hydra has several disks, but only one tree. Which directory is each disk seen at?

<div class="write l2"></div>

4. ✎ `/bin` is a **union**: several directories seen as one. Name two of them. If two of them have a program with
   the same name, which one runs?

<div class="write l2"></div>

5. ✎ Each task has a **namespace** of its own. You typed `bind` at one prompt. Would a program in another window
   see `/mnt` change? (Section 9.5 has the experiment.)

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | move around the tree with `cd`, with full paths and with `..` |
| ☐ | read a long listing |
| ☐ | say which disk a path is on, and how full the disks are |
| ☐ | read a device through its file |
| ☐ | bind one directory onto another, and take it away again |

> **CHALLENGE:** ★ Make a directory `/ram/mybin`, put a script in it, and bind it *after* `/bin` with
> `bind -a /ram/mybin /bin`. Can you now run the script by its name alone? Use `ns` to see your change, and
> explain what `-a` did. What would `-b` have done?
