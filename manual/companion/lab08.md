# Lab 8. Windows

> **THIS LAB:** Manual: Chapter 10, sections 10.1 to 10.4, and 10.8. About 45 minutes. You need a Hydra, on a
> terminal at least 80 columns wide. The **window key** is <kbd>CTRL</kbd>-<kbd>]</kbd>: press it, let go, and then
> press the next key.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Press <kbd>CTRL</kbd>-<kbd>]</kbd> and then <kbd>?</kbd>. ✎ What appears? How do you make it go away?

<div class="write"></div>

2. Press <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>C</kbd>. ✎ What happened? Which window are you in now? (The bar at the top
   says.)

<div class="write l2"></div>

3. In the new window, type `echo I am the second window`. Then press each of these, and write what you see.

| Keys | What I saw |
|---|---|
| <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>0</kbd> | |
| <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>1</kbd> | |
| <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>W</kbd> | |
| <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>N</kbd> | |

4. In window 0, start something slow: `sleep 30`. While it's running, go to the other window and type `ps`.
   ✎ Is `sleep` still running? What does that tell you about windows and tasks?

<div class="write l2"></div>

5. A window's **group** can have several windows, shown together. In one window, type:

```text
/> new-window
/> echo layout columns >/dev/wctl
```

   ✎ What does the screen look like now? How do you move the keyboard from one to the other?

<div class="write l2"></div>

6. Try the other layouts: `rows`, `grid`, and `tabs` to go back to one at a time. ✎ Which would you use for writing
   a program in one window and running it in another?

<div class="write"></div>

7. Give your window a name, and read it back:

```text
/> echo my window >/dev/label
/> cat /dev/label; echo
```

   ✎ Where on the screen did the name appear?

<div class="write"></div>

8. Close the extra windows: type `bye` in each one's shell (or press <kbd>CTRL</kbd>-<kbd>]</kbd> <kbd>X</kbd>).
   ✎ What happens if you type `bye` in window 0?

<div class="write"></div>

### Questions

1. ✎ The Hydra can run sixteen programs, but you have one screen. How do windows solve that?

<div class="write l2"></div>

2. ✎ What's the difference between a window and a group?

<div class="write l2"></div>

3. ✎ You're in a full-screen program and need to send it a real <kbd>CTRL</kbd>-<kbd>]</kbd>. How?

<div class="write"></div>

4. ✎ `/dev/label` isn't a file on any disk. What is it? What else in `/dev` belongs to your window?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | open a new window, and go between windows |
| ☐ | list the windows, and close one |
| ☐ | show two windows side by side |
| ☐ | explain that each window has its own shell, and they all run at once |

> **CHALLENGE:** ★ Set up a "workbench": `edit` in one window and a shell in another, side by side. Write a
> three-line rc script in the first, save it (without leaving the editor), and run it in the second. Change it,
> save it, run it again. How many keys does one round trip take?
