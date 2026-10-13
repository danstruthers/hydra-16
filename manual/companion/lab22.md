# Lab 22. Your Language Meets the Hydra

> **THIS LAB:** Manual: the second chapter of your language: Chapter 20 (HyForth), Chapter 22 (hylang) or Chapter 24
> (HyBASIC). About an hour. You need a Hydra, with sound; the last step needs a Vera X. Choose **one** language for
> the whole lab.

<p class="fields">Name <span></span> Date <span class="short"></span> My language <span class="short"></span></p>

Each step is a job to do *from your language*. The manual's chapter has what you need; your job is to find it,
try it, and write down the line (or lines) that worked.

### Do

1. **Run a command.** Make your language run the shell command `date`, and show what it printed.

<div class="write l2"></div>

2. **Write a file.** Write three lines of text into `/ram/log.txt`. Leave your language, and check with `cat`.

<div class="write l3"></div>

3. **Read it back.** Read `/ram/log.txt` from your language, and print each line with its line number in front.

<div class="write l4"></div>

4. **The screen.** Clear the screen, and print your name near the middle of it, in a colour.

<div class="write l3"></div>

5. **Sound.** Play three notes, one after another: C, E and G.

<div class="write l3"></div>

6. **Time.** Print "Ready...", wait two seconds, and print "Go!".

<div class="write l2"></div>

7. **A picture** (with a Vera X). Turn on a bitmap, and draw a box with a circle inside it.

<div class="write l4"></div>

8. ✎ Put three of these together into one small program, in a file in `/sd/0/work`. Say what it does.

<div class="write l3"></div>

### Questions

1. ✎ Which job was easiest in your language? Which was hardest? Why, do you think?

<div class="write l3"></div>

2. ✎ Did your language need a **library** for any of these? Which, and how did you load it?

<div class="write l2"></div>

3. ✎ In Lab 12 you played notes by writing to `/dev/sndctl`, and in Lab 13 you drew by writing to
   `/dev/vid/draw`. Could your language have done these jobs *that* way, with nothing but its file words? What does
   that tell you about how the libraries work?

<div class="write l3"></div>

4. ✎ What happens when your program asks for something that isn't there (a file that doesn't exist, or a picture
   with no Vera X)? How can the program find out, and carry on?

<div class="write l2"></div>

### Check yourself

| ☐ | In my language, I can |
|---|---|
| ☐ | run another program, and use what it prints |
| ☐ | write a file, and read one |
| ☐ | control the screen: clear it, move the cursor, change colour |
| ☐ | make a sound |
| ☐ | wait |

> **CHALLENGE:** ★ Do steps 1, 2 and 5 again in one of the *other* two languages. Then fill in a table of your
> own: for each job, the line in each language. Which language's line would you rather explain to a friend?
