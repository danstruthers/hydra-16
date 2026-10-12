# Lab 2. A Hydra on Your PC

> **THIS LAB:** Manual: Chapter 2, sections 2.1 and 2.2. About 45 minutes. You need a PC with a web browser, and
> the emulator's page, `hydra-16.html`.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Open `hydra-16.html`. Watch the Hydra start. ✎ What's the last line it prints before it waits for you?

<div class="write"></div>

2. Click in the serial console, and type these two lines. ✎ What does each print?

```text
/> 2 3 + .
/> ls /
```

<div class="write l2"></div>

3. Find each control, try it, and say what happened.

| Control | What happened when I used it |
|---|---|
| **Pause** | |
| The speed menu: **As fast as it goes** | |
| The **size** menu | |
| **Copy text** | |
| **Sound** | |

4. An experiment: where does a file survive? Make one file on the RAM disk and one on the SD card:

```text
/> echo hello >/ram/a.txt
/> echo hello >/sd/0/a.txt
```

   Then do each thing in the table, and look with `ls /ram` and `ls /sd/0`. Write **kept** or **gone**. (After each
   one, make the files again if they've gone.)

| After | `/ram/a.txt` | `/sd/0/a.txt` |
|---|---|---|
| Pressing **Reset** | | |
| Pressing **Power cycle** | | |
| Closing the page, and opening it again | | |

5. Open **Setup…**. ✎ What hardware does this Hydra have?

<div class="write l2"></div>

6. In Setup, clear the box for the Vera X card, and press **Power cycle with these**. Type `ps`. Then put the Vera X
   back, power cycle, and type `ps` again. ✎ What's different?

<div class="write l2"></div>

### Questions

1. ✎ What is an emulator? In what way is it the *same* as a real Hydra, and in what ways is it different?

<div class="write l3"></div>

2. ✎ From your experiment: where should you keep work that matters? Why?

<div class="write l2"></div>

3. ✎ What's the difference between **Reset** and **Power cycle**?

<div class="write l2"></div>

4. ✎ You want to take your work home, to another computer. Which button do you need, and what does it give you?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | start the emulator, and type a command into it |
| ☐ | reset it, power cycle it, and change its hardware in Setup |
| ☐ | say where a file is safe, and where it isn't |
| ☐ | save my SD card to a file, and load it again |

> **CHALLENGE:** ★ If you have the Hydra's files and Node.js (section 2.3), start the terminal emulator with
> `node reborn/sim/run.js -i`. Press <kbd>CTRL</kbd>-<kbd>A</kbd> and then <kbd>S</kbd>. What does it show? Then
> <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>B</kbd>, type `r`, and then `c`. What were you looking at?
