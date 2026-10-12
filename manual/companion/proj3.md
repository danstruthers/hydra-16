# Project 3. A Round for Two Voices

> **THIS PROJECT:** Music, written as text. After Lab 12. Two sittings. You'll use MML and a score file (section
> 15.6), and you need sound: headphones, or the emulator's **Sound** button and **Real time** speed.

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The starting point

A **round** is a tune that fits with itself: a second voice starts the same tune a little later, and the two
sound well together. Type this into `/sd/0/work/frere.mml`, and play it with `play /sd/0/work/frere.mml`.

<!-- listing samples/frere.mml -->

```text
; frere.mml - Frere Jacques, as a round for two voices
#tempo 120
#title Frere Jacques
A I73 o4 l4 [c d e c]2 [e f g2]2 [g8 a8 g8 f8 e c]2 [c <g> c2]2
B I71 o4 l4 r1 r1 [c d e c]2 [e f g2]2 [g8 a8 g8 f8 e c]2 [c <g> c2]2
```

### Understand it first

1. ✎ Lines `A` and `B` are almost the same. What's different, and what does that difference do?

<div class="write l2"></div>

2. ✎ What do these mean: `l4`, `g2`, `g8`, `[c d e c]2`, `<g>`, `r1`?

<div class="write l3"></div>

3. ✎ Each bar of this tune is four beats. Count the beats inside `[g8 a8 g8 f8 e c]`. Is it a whole bar?

<div class="write"></div>

4. ✎ `I73` and `I71` choose the instruments. Which are they? (Section 15.4.) Try two others.

<div class="write l2"></div>

### Make it yours

| ☐ | Change |
|---|---|
| ☐ | Add a third voice, on line `C`, starting two bars after the second |
| ☐ | Make it faster, then slower (`#tempo`) |
| ☐ | Put one voice in the left speaker and one in the right (`p l` and `p r`) |
| ☐ | Make the second voice quieter (`v`) |
| ☐ | Add a drum on every beat, on another line (`x`, and section 15.4's list of drums) |
| ☐ | Write out a different round: "Row, Row, Row Your Boat", or "London's Burning" |
| ☐ | Write a tune of your own, at least eight bars, for two voices |

✎ Write your own tune's lines here.

<div class="write l6"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | Every bar adds up to the same number of beats, in every voice |
| ☐ | The file has a title, and a comment saying what it is |
| ☐ | Someone has listened to it all the way through |
| ☐ | It's saved in `/sd/0/work` |

> **CHALLENGE:** ★ Turn your score into a song file with `play -o`, and write a HyBASIC or hylang program that
> plays it in the background while it prints the words, a line at a time, in step with the music. (Chapter 24 or
> 22 has what you need; the sample program `jukebox` on the SD card does something like it, in C.)
