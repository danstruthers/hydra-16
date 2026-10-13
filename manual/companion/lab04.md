# Lab 4. From Power to Prompt

> **THIS LAB:** Manual: Chapter 7 (sections 7.1 to 7.3 and 7.5; dip into 7.4 for the messages). About 45 minutes.
> You need a Hydra at its prompt.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. The Hydra keeps a copy of everything it printed as it started. Read it again:

```text
/> cat /dev/kmesg
```

   ✎ Copy the line that starts `POST`, and the line that says how many RAM modules were found.

<div class="write l2"></div>

2. Type `ps`, and fill in the table for every task it lists.

| Task | Name | State | What it's for (sections 7.4 and 7.5) |
|---|---|---|---|
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |

3. ✎ One of the tasks in your table is `ps` itself. Why is it there? Will it be there when `ps` has finished?

<div class="write l2"></div>

4. Start a program that takes a while, and leave it running "in the background", by putting `&` after it. Then look
   for it, and stop it. (Use the task number that *your* `ps` shows for `sleep`.)

```text
/> sleep 60 &
/> ps
/> kill 4
/> ps
```

   ✎ What state was `sleep` in? Which task was its parent? What happened to it after `kill`?

<div class="write l2"></div>

5. Type `free`. ✎ How much memory can each task have? How many RAM modules does that mean?

<div class="write"></div>

6. Type `cat /dev/ticks`, twice, a few seconds apart. ✎ What are the two numbers? About how many ticks is a second?

<div class="write"></div>

### Questions

1. ✎ The 65C02 can only see 64K at once, but the Hydra has megabytes. How does it reach the rest?

<div class="write l2"></div>

2. ✎ What's a task? How does the Hydra keep sixteen of them from getting in each other's way?

<div class="write l3"></div>

3. ✎ Put these in the order they happen when the Hydra starts: the shell starts; the drivers start; the power-on
   self test; `init` starts; the processor reads the reset vector.

<div class="write l3"></div>

4. ✎ What's a driver? Name three, and the device each looks after.

<div class="write l2"></div>

5. ✎ What does `init` do if the shell ends? Why is that a good idea?

<div class="write l2"></div>

6. ✎ What's the kernel's "tick", and what happens on each one?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | read the start-up messages, and say what the main ones mean |
| ☐ | list the tasks, and say which are drivers |
| ☐ | start a program in the background, find it, and stop it |
| ☐ | explain, roughly, how one processor runs many programs |

> **CHALLENGE:** ★ `mods` lists the pieces of HydraOS that live in the paged ROM, and where each is. How many are
> there (`mods | wc -l` counts the lines)? Find `forth`, `hylang` and `basic` in the list. Which of the three
> languages takes the most banks? What might that tell you?
