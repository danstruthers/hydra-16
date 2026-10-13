# Lab 13. Pictures

> **THIS LAB:** Manual: Chapter 16, sections 16.1 to 16.4. About an hour. You need a Hydra with a Vera X. The web
> emulator has one unless it's been turned off in Setup; for the terminal emulator, start it with `--vera --view`.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Look at the Vera X's screen. ✎ What's on it, before you do anything? How is it different from the serial
   console?

<div class="write l2"></div>

2. Read the screen's settings: `cat /dev/vid/ctl`. ✎ Copy two of its lines, and say what you think each means.

<div class="write l2"></div>

3. Turn on a **bitmap**, a picture made of dots, under the text, and draw on it. Each line is a command to the
   screen's `draw` file.

```text
/> echo bitmap 320 8 >/dev/vid/ctl
/> echo clear 0 >/dev/vid/draw
/> echo pen 12 >/dev/vid/draw
/> echo circle 160 110 60 >/dev/vid/draw
/> echo pen 10 >/dev/vid/draw
/> echo box 40 30 280 190 >/dev/vid/draw
/> echo pen 11 >/dev/vid/draw
/> echo line 40 30 280 190 >/dev/vid/draw
```

   ✎ Sketch what's on the screen, and label each shape with the command that drew it.

<div class="space s2"></div>

4. ✎ The bitmap is 320 dots wide and 240 high. Where is the dot (0, 0)? Where is (319, 239)? Which way does *y*
   go?

<div class="write l2"></div>

5. ✎ Work out, on paper, the commands to draw a house: a square for the walls, a triangle of three lines for the
   roof, a filled door (`bar`), and a round window (`disc`). Write them here, then type them in. Fix them until
   it looks like a house.

<div class="write l8"></div>

6. Typing `echo ... >/dev/vid/draw` every time is slow. Put your house's commands in a file, one to a line,
   without the `echo`, and send the whole file at once:

```text
/> edit /ram/house.txt
/> cat /ram/house.txt >/dev/vid/draw
```

   ✎ Why does that work?

<div class="write l2"></div>

7. Write your name on the picture with `text`, and then turn the bitmap off with `echo bitmap off >/dev/vid/ctl`.

### Questions

1. ✎ What does the Vera X add to a Hydra? Name four things.

<div class="write l2"></div>

2. ✎ What's a pixel (a dot)? How many are there in a 320 by 240 bitmap?

<div class="write"></div>

3. ✎ `bitmap 320 8` means 320 dots across, and 8 bits for each dot. How many colours can 8 bits choose between?
   How many could 4 bits?

<div class="write l2"></div>

4. ✎ What's the difference between `box` and `bar`? Between `circle` and `disc`?

<div class="write"></div>

5. ✎ The picture is drawn by writing text to a file. What does that make easy that a special drawing program
   wouldn't?

<div class="write l2"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | turn a bitmap on and off |
| ☐ | find a point from its *x* and *y* |
| ☐ | draw lines, boxes and circles, in the colours I choose |
| ☐ | keep a drawing as a file of commands, and draw it again |

> **CHALLENGE:** ★ Write an rc script that draws ten circles, each bigger than the last, each in a different
> colour. You'll need a `for` loop, and `calc` to work out each radius. Then make them all share a centre... or
> not.
