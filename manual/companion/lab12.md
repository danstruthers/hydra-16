# Lab 12. Sound

> **THIS LAB:** Manual: Chapter 15, sections 15.1, 15.2, 15.4, 15.6 and 15.7. About an hour. You need a Hydra
> with headphones or a speaker; in the web emulator, press **Sound** first, and set the speed to **Real time**.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do

1. Play one note, by hand. Each line is a command to the sound driver.

```text
/> echo patch 0 0 >/dev/sndctl
/> echo note 0 60 >/dev/sndctl
/> echo off 0 >/dev/sndctl
```

   ✎ What did each of the three lines do? What are the two numbers after `note`?

<div class="write l2"></div>

2. Note 60 is middle C. ✎ Using the table in section 15.2, what are the numbers of these notes?

| Note | Its number |
|---|---|
| The D above middle C | |
| The A that an orchestra tunes to | |
| The C an octave above middle C | |
| The C an octave below | |

3. Change the instrument: `patch 0 40` is a violin. ✎ Try five more patch numbers (section 15.4 lists them), and
   write down which you tried and what you heard.

<div class="write l3"></div>

4. `play -m` plays music written as text, in **MML**. Try these:

```text
/> play -m 0 'o4 l8 c d e f g a b > c'
/> play -m 0 'I40 o5 l4 c c g g a a g2'
/> play -c 0 c e g
```

   ✎ What does each of these mean in the first line: `o4`, `l8`, `>`? What did the third line play?

<div class="write l3"></div>

5. ✎ Write a tune of your own in MML: at least eight notes, with at least two different lengths and one rest.
   Play it, change it until you like it, and copy it here.

<div class="write l2"></div>

6. A **score** is a file with a tune for several channels at once. Make this one with `edit /ram/round.mml`:

```text
#tempo 120
A I0 o4 l4 c d e c | c d e c | e f g2 | e f g2
B I40 o3 l2 c g | c g | c g | c g
```

   Then play it: `play /ram/round.mml`. ✎ What are the `A` and `B` at the start of the lines? What does `#tempo`
   set?

<div class="write l2"></div>

7. Play the song that comes with HydraOS: `play /rom/songs/scom.zsm`. Press <kbd>CTRL</kbd>-<kbd>C</kbd> to stop it.

### Questions

1. ✎ The YM2151 is an "FM synthesizer" with eight voices. What does "eight voices" let it do?

<div class="write l2"></div>

2. ✎ What's a patch? What's a channel?

<div class="write l2"></div>

3. ✎ Note 69 sounds at 440 Hz. What does "440 Hz" mean? What note would sound at 880 Hz?

<div class="write l2"></div>

4. ✎ A `.mml` file is text that people can read. A `.zsm` file isn't. What's in a ZSM file, and why does the Hydra
   have both?

<div class="write l2"></div>

5. ✎ In the web emulator, why should the speed be **Real time** when you play music?

<div class="write"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | play a note, and stop it, by writing to `/dev/sndctl` |
| ☐ | choose an instrument |
| ☐ | write a tune in MML, with lengths, octaves and rests |
| ☐ | write a score for two channels, and play it |

> **CHALLENGE:** ★ Add a third line to the score for a drum (section 15.6: `x` plays a General MIDI drum, and
> section 15.4 lists them). Then turn your score into a ZSM file with `play -o`, and play that. Which file is
> bigger?
