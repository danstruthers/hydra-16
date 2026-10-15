# Topic 3. The Room, the Kit and the Emulator

There are three ways to have a Hydra in the room, and most courses end up using two of them at once.

| | Emulator only | One built Hydra, and emulators | A Hydra for each group |
|---|---|---|---|
| What it costs | Nothing but the computers you have | One kit, one set of tools | A kit for every two or three students; tools for each bench |
| Before the first lesson | An hour | The build: 20 hours of yours, or a group's | A term |
| What students miss | The build; real pins, real sound, a real fault | Hands on the hardware, for most of them | Nothing |
| Labs you can't do | 14 Part A; Project 6 | None, in turns | None |
| Best for | Shapes A and B; a first year | Shape C; a first build | Shapes D and E |

The middle column is the one to start with. One real Hydra on the front bench, wired to a projector, changes how a
class sees thirty emulators: they stop being a web page, and become *copies of that*.

## The emulator

### Getting it onto every computer

The web emulator is one file, `hydra-16.html`, of about a megabyte. It needs no installation, no network, no
account and no plug-in: any browser of the last few years will run it straight from a disk.

1. Take it from the project's `reborn/bin` folder (on its GitHub page: open `reborn/bin/hydra-16.html`, then
   *Download raw file*). It's made with every build of HydraOS, so it always matches the ROM images beside it.
2. Copy `hydra-16.html` to a shared drive, or onto each computer, or onto a memory stick for each student.
3. Open it once on a student computer, with the student's login, *before* the first lesson. School filters and
   locked-down browsers are the commonest reason for a lost first lesson: find out a week early.

> **WATCH FOR:** The emulator keeps each student's SD card inside the browser's own storage, on that one computer,
> under that one login, for that one copy of the file. A student who sits somewhere else next week, or whose
> computer "resets itself" overnight (many school computers do), finds an empty card. Decide on day one which of
> these you'll do about it, and tell the class:
>
> * **Save card** to a file at the end of every lesson, and **Load…** at the start of the next. This is the
>   robust way, and it's a good habit: make it part of packing up.
> * Fixed seats, on computers that keep their browser storage. Simplest, when it's true.
> * A memory stick each, with the emulator and the card file on it.

### Making the students' Hydras match the book

As it comes, the web emulator's Hydra has a Vera X video card and a clock chip, and the manual's Hydra has
neither (the manual's section "This book's Hydra"). The differences are small: a few more start-up lines, today's
date, and one more task in `ps`. For Labs 12 and 13 you want the Vera X; for the rest it doesn't matter. Either
tell the class to expect it, or have them clear the two boxes in **Setup…** once.

### Giving every student the same starting card

For a lab that needs files of yours on the card (a half-written program, a data file), make a card image once
and hand it out: in the terminal emulator's tools, `hydrafs.js` builds one from a folder on your PC (the manual's
section 2.6). Students bring it in with **Load…**. The same image, written to a real SD card with `sdwrite.js`,
serves a real Hydra.

### The projector

The terminal emulator (`node reborn/sim/run.js -i`) in a large-font terminal window projects better than the web
page, and it has two things worth having at the front: `--pc-dir`, so that you can prepare files in your own
editor and have them appear on the Hydra as `/pc`; and <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>S</kbd>, which shows what
every task is doing, and is the best single picture of multitasking there is.

## The kit

A kit is planned. Until there is one, this is what one Hydra is made of: price it from the manual's Appendix I,
which lists every part with its value and its package, and section 3.9, which says where the parts come from.

| What | How many | Notes |
|---|---|---|
| The main circuit board | 1 | Revision 1.8D |
| The parts soldered to it | 216 | 64 chip sockets; 27 resistors and networks; 91 capacitors; headers, connectors, a crystal |
| The four shared RAM chips | (of the 216) | Surface-mount: ask whether they come fitted. If not, an adult fits them (Topic 6) |
| The chips for the sockets | 59 | Five of the ROM sockets stay empty |
| A memory card, with its parts | 1 or more | The through-hole card: 13 parts, 4 RAM chips |
| The four ROM chips, programmed | (of the 59) | Programmed already, or a flash programmer and an hour of your time (section 4.4) |
| An ATX power supply | 1 | Any desktop PC's; second-hand ones are fine |
| A power switch, a reset button, an LED | 1 each | The switch must be one that stays on |
| A USB to RS-232 adapter | 1 | With a 9-pin plug: *not* a "TTL" adapter (section 5.1) |
| An SD card module, and a small SD card | 1 each | A 5-volt module. Not strictly needed, but without it nothing can be saved on the Hydra itself |
| A PC, to be the terminal | 1 | Anything with a USB port |
| For Lab 14 and Project 6 | | LEDs, 2.2K resistors, hook-up wire, a push button |
| A Vera X video card | Optional | Not part of the kit; the emulator has one |

**Spares.** For a class build, hold spares of whatever is cheap and easy to damage: sockets of each size, a few of
each resistor and capacitor value, one of each of the common logic chips, and a second set of ROM chips. A whole
lesson lost to one bent pin costs more than the spare.

## A bench for soldering

For each pair of students at a bench:

| What | Notes |
|---|---|
| A temperature-controlled iron, with a stand and a tip cleaner | Not a bare "pencil" iron: the difference in the students' joints is enormous |
| Lead-free solder, 0.5 to 0.8 mm | Topic 4 says why lead-free |
| Safety glasses, for each person | Not optional |
| A heat-proof mat | |
| Flush cutters, needle-nose pliers, tweezers | |
| A magnifier, with a light | One between two is enough |
| Desoldering braid | And a solder sucker for the room |
| Fume extraction | A small bench extractor each, or good cross-ventilation: Topic 4 |

And for the room: two multimeters; an anti-static wrist strap for whoever is fitting chips; isopropyl alcohol
and brushes, kept by you; a first-aid kit that someone knows how to use; and running cold water within a few
steps.

## The room for keyboard lessons

* **One computer each.** Pairs work for labs if they swap the keyboard at every numbered step; they don't work
  for projects.
* **Headphones** for Labs 12 and 22, and Project 3. Thirty Hydras playing "Frère Jacques" out of step is a round
  nobody wants.
* **Paper.** The Companion is meant to be written in, with a pencil. Students who are asked to *predict* on paper
  before they type learn far more from the same lab than students who just type. If you can't print a Companion
  each, print the lab sheets as you need them.
* **The manual within reach.** On paper if you can: a student with the manual open beside the keyboard looks
  things up, and a student who must switch windows to find it doesn't.
