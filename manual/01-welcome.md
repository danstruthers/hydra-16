# Chapter 1. Welcome to the Hydra-16

Before you pick up a soldering iron or type your first command, let's look at what a computer *is*, and at the
particular computer you are about to meet. By the end of this chapter you will know the names of the Hydra's main
parts, what each one does, and how to read the strange numbers, like `$FF20`, that computer people love.

> **IN THIS CHAPTER:** You'll learn what a computer is made of, meet the Hydra-16's parts and its software, and learn
> to read binary and hexadecimal numbers. **You need:** nothing. This is where everybody starts.

## 1.1 What is a computer?

A computer is a machine that **follows instructions**. That's all. A list of instructions is called a **program**,
and the computer works through a program one instruction at a time, very quickly, without getting bored or making
mistakes. Each instruction is tiny: "add these two numbers," "copy this number over there," "if this number is zero,
skip ahead." But a computer can follow millions of them every second, and from millions of tiny steps come games,
music, drawings, and everything else computers do.

Every computer, from a pocket calculator to a supercomputer, does the same four jobs:

| Job | What it means | On the Hydra |
|---|---|---|
| **Input** | Getting information in | The keyboard, files on a card, a mouse |
| **Processing** | Working on it | The 65C02 processor chip |
| **Memory** | Remembering it | RAM and ROM chips |
| **Output** | Showing the results | Text on a screen, sound, files |

Think of a computer as a cook in a kitchen. The **program** is the recipe. The **processor** is the cook, who reads
one step at a time and does it. The **memory** is the kitchen counter and the recipe book. And **input** and
**output** are the doors the groceries come in through and the plates go out through.

## 1.2 The parts of a computer

Open up almost any computer and you will find the same kinds of parts. The Hydra has all of them, and on the Hydra
they are big enough to see and touch.

### The processor

The **processor**, or **CPU** (central processing unit), is the chip that follows the instructions. The Hydra's
processor is a **W65C02S**, made by the Western Design Center. It is a modern version of the 6502, the processor
that powered many famous home computers and game machines of the late 1970s and 1980s. The 65C02 is small and
simple enough that a person can learn every single instruction it knows (there are about seventy), which is one of
the reasons the Hydra uses it. Part Five of this book teaches you to program it directly.

### Memory

A computer needs somewhere to keep its programs and the information they work on. That's **memory**, and it comes
in two kinds.

**RAM** (random-access memory) is like a whiteboard: the computer can write on it, read it back, and wipe it clean,
as often as it likes. RAM forgets everything when the power goes off.

**ROM** (read-only memory) is like a printed book: the computer can read it, but not change it, and it remembers
everything even with the power off. The Hydra keeps its operating system, its languages and its tools in ROM, so
they are ready the moment you switch on. (The Hydra's ROM chips are actually *flash* memory, a kind of ROM that a
special programmer can rewrite. That's how you put the software in them. See Chapter 4.)

Memory is divided into **bytes**. A byte holds one small number, from 0 to 255: enough for one letter of text, one
colour, or one piece of a bigger number. Each byte has an **address**, a number that says where it is, just as
each house on a street has a number. When the processor wants a byte, it sends out the address, and the memory
answers with the byte that lives there.

### Input and output

Input and output, often written **I/O**, are how the computer connects to the world. Special chips handle each
kind of connection: one sends and receives text over a cable, another makes sound, another reads the keyboard. To
the processor, each I/O chip looks like a few bytes of memory at special addresses. Writing a byte to the sound
chip's address plays a note; reading the serial chip's address gives you the key someone just typed.

### The clock

Inside every computer is a **clock**, not one that tells the time of day, but a steady beat, like a drummer
keeping time for a band. On each beat the processor does one small step of its work. The Hydra's clock beats
3,579,545 times every second, written **3.58 MHz** (megahertz: millions of beats per second). A little jumper on the
board can make it beat more slowly, for testing.

### The bus

All these parts are connected by the **bus**: dozens of copper paths, called *traces*, that run between the chips.
The address bus carries addresses from the processor, the data bus carries bytes back and forth, and the control
lines say what's happening ("I'm reading," "I'm writing," "now!"). On the Hydra's circuit board you can see the bus
as the many thin lines running in parallel between the chips.

## 1.3 Meet the Hydra-16

The Hydra-16 is a computer you build yourself. It is a single large circuit board, about the size of a sheet of
paper, covered in chips that you put in their places one by one. Its name comes from the Hydra of Greek legend, a
monster with many heads. The Hydra-16 has sixteen "heads": it can run up to **sixteen programs at the same time**,
each in its own private part of the memory, without any of them getting in the others' way. (HydraOS keeps five of
them for itself, and the shell you type at takes one, so ten are free for the programs you start.)

That ability, called **multitasking**, is something the home computers of the 1980s could not do. A Commodore 64
or an Apple II ran one program at a time. The Hydra is built from the same kind of chips as those machines, but it
adds a few clever extra chips that give every program its own memory. That lets its operating system, **HydraOS**,
juggle many programs at once, the way modern computers do.

Here is what's inside:

| Part | The Hydra-16 has |
|---|---|
| Processor | One W65C02S, at 3.58 MHz (or 1.79 or 0.89 MHz) |
| Programs at once | Up to 16 *tasks*, each with its own 32K of memory |
| RAM | 512K for the tasks, 2 MB shared between them, and 2 MB more for each RAM module on the memory cards (up to 15 modules) |
| ROM | 128K for the kernel (the heart of HydraOS), and up to 4 MB for the rest of the system |
| Serial port | A DE-9 connector for a PC or terminal: the Hydra's keyboard and screen |
| Sound | A Yamaha YM2151 FM sound chip, with a stereo headphone/line output |
| Storage | Up to eight SD card readers, and RAM disks |
| Connectors | Six expansion slots, eight SPI device headers, a 12-pin I/O header (six I/O pins, two handshake pins and the I²C bus) |
| Video (optional) | The Vera X card: a colour screen, sprites, a keyboard and mouse |
| Power | A standard ATX computer power supply |

> **NOTE:** "K" means 1,024 bytes, and "MB" (megabyte) means 1,024 K, or 1,048,576 bytes. Computers count in
> powers of two, so 1,024 (that is, 2 × 2 × 2 × 2 × 2 × 2 × 2 × 2 × 2 × 2) is a "round number" to a computer.
> The Hydra's 32K for each task is 32,768 bytes.

## 1.4 A tour of the board

Figure 1-1 shows the Hydra-16's circuit board from above, with its main parts labelled. Every part on the board
has a name printed next to it in white letters: a letter for the kind of part and a number. **U** is a chip
(integrated circuit), **R** a resistor, **C** a capacitor, **J** a connector, **Y** the crystal. You will use
these names all through the building chapters.

<figure>
<img src="figures/board-tour.svg" alt="The Hydra-16 board, with its main parts labelled">
<figcaption>Figure 1-1. The Hydra-16's circuit board and its main parts.</figcaption>
</figure>

Let's take a walk around the board.

**The processor (U1).** The big 40-pin chip near the top of the board, next to the expansion slots, is the W65C02S.
Everything else on the board is there to help it.

**The clock (Y1, U39, U40).** The little silver can marked *14.31818MHz* is the **crystal**, which vibrates
14,318,180 times a second. Two chips divide that beat down into the clocks the computer uses. Four jumpers,
J5 to J8, choose the processor's speed.

**The task RAM (U7).** One 512K RAM chip, near the bottom left, holds sixteen separate 32K blocks, one for each
task. When the Hydra switches from one program to another, it simply switches which block the processor sees.

**The shared RAM (U25, U27, U28, U29).** Four small surface-mounted RAM chips along the bottom edge hold 2 MB that
any program can use, a piece at a time.

**The memory cards (J1, J2).** Two long sockets on the left take the **memory cards**. A card holds one or two
**RAM modules** of 2 MB each, and every module gives every task 128K more memory of its own.

**The kernel ROM (U6).** The square chip in the middle of the board, in a socket with pins on all four sides, holds
the **kernel**: the heart of HydraOS, the part that runs the other programs.

**The paged ROM (U30 to U37).** Eight sockets along the bottom right hold up to eight more ROM chips, 512K each.
They hold the rest of HydraOS: its drivers, shells, languages and tools, and a whole disk's worth of files.

**The registers and mirrors.** The rows of smaller chips in the middle of the board are the Hydra's special trick.
Four of them, labelled **T**, **U**, **V** and **W Register**, choose which task's memory the processor sees, which
shared memory, which interrupt, and which page of the kernel ROM. Others, the **mirror RAMs**, remember which bank
of memory each task has chosen. (A **bank** is one piece of a memory bigger than the processor can see at once:
the processor looks at one bank at a time, and can choose another. An **interrupt** is a chip's signal that it
needs attention now.) Chapter 7 tells you how they work together.

**The VIA (U2).** The R65C22 *Versatile Interface Adapter* has two timers and two 8-bit ports. One timer ticks the
Hydra's clock for multitasking. One port runs the **SPI bus**, which talks to SD cards and other gadgets through
the eight **SPI device headers** (J18 to J25). The other port goes to the **I/O header** (J27), where you can
connect switches, lights and I²C sensors.

**The serial port (U3, U5, J3).** The R65C51 *ACIA* chip sends and receives characters one bit at a time, and the
MAX232 chip changes them into the voltages of a standard serial cable. The DE-9 connector at the top, J3, is where
your PC plugs in. This cable is how you type to the Hydra and see its answers.

**The sound (U38, U41, U42, IC9, J26).** The Yamaha **YM2151** is an FM synthesizer, the same kind of chip that made
the music in many 1980s arcade games. It has eight voices. The YM3012 turns its numbers into sound waves, and the
mixer adds in the sound from slot 0 and the *Audio In* header. Plug headphones or a speaker into the jack, J26.

**The expansion slots (J12 to J17).** Six long card-edge sockets let you plug in extra boards. **Slot 0** is
special: it's the one for the **Vera X**, a video card that gives the Hydra a colour screen, a keyboard and a mouse.

**The power (J11, J10, U43).** The big 24-pin connector, J11, takes a standard ATX computer power supply. The
front-panel header, J10, is for the power switch, the reset button and the power light, and U43 makes sure the
computer starts cleanly every time the power comes on or the reset button is pressed.

## 1.5 HydraOS, the Hydra's software

A computer without software is just a pile of chips. The Hydra's software is called **HydraOS**, and it lives in
the ROM chips, so it starts the moment you switch on. HydraOS is an **operating system**: the program that runs
the other programs. It decides which program gets the processor next, gives each one its memory, and lets them use
the files and devices without fighting over them.

HydraOS gives you:

* **Shells**: programs that read the commands you type and do them. The Hydra greets you with **HyForth**, a shell
  that understands both commands and the Forth programming language, and also has **rc**, a classic command shell.
* **Files**: a tree of folders and files on SD cards, in ROM, and in RAM disks, all in one tree.
* **Windows**: up to sixteen text windows on one screen, each with its own program, that you switch between with a
  couple of keys.
* **About fifty commands**: for listing, copying and editing files, watching the programs that are running,
  calculating, playing music, and much more.
* **Three programming languages**: **HyForth**, **hylang** and **HyBASIC**, plus an **assembler**, a
  **disassembler** and a **debugger** for programming the 65C02 itself.

Part Two of this book is all about using HydraOS. Part Four teaches the languages.

## 1.6 Decimal, binary and hexadecimal

Sooner or later every computer book shows you a number like **$FF20** or `01000001`. These are ordinary numbers,
written in a different way. This section shows you how to read them. Take it slowly: the rest of the book leans on
it, and it is much easier than it looks.

### Decimal: the way you already count

Take the number 3,405. You read it without thinking, but look at what each digit is doing:

| Place | Thousands | Hundreds | Tens | Ones |
|---|---|---|---|---|
| Worth | 1,000 | 100 | 10 | 1 |
| Digit | 3 | 4 | 0 | 5 |

Three thousands, four hundreds, no tens and five ones: 3,000 + 400 + 0 + 5. A digit's **place** says what it's
worth, and each place is worth *ten times* the one to its right.

That's why there are ten digits, 0 to 9. When you've counted up to 9 in a place you've run out of digits, so you
put that place back to 0 and add 1 to the place on its left. That's how 9 is followed by 10, and 99 by 100.

This way of writing numbers is called **decimal**, or **base 10**. There's nothing special about ten, except that
people have ten fingers. Any number can be the base, and computers use two others all the time.

### Binary: counting with two digits

Everything inside a computer is kept in millions of tiny switches, and a switch can only be *off* or *on*. So a
computer has just two digits to count with: **0** for off, and **1** for on. Counting with two digits is called
**binary**, or **base 2**.

It works exactly like decimal, except that you run out of digits much sooner. After 0 comes 1, and then you've
run out already: so back to 0, and add 1 to the place on the left. That gives `10`, which is two. Then `11`, three;
and now both places have run out, so the next number is `100`, four.

| Decimal | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|---|
| Binary | `0` | `1` | `10` | `11` | `100` | `101` | `110` | `111` | `1000` |

In decimal each place is worth ten times the one to its right. In binary each place is worth *twice* the one to
its right: 1, 2, 4, 8, 16, 32, 64, 128, and so on.

One binary digit is called a **bit** (short for "binary digit"). Eight bits in a row make a **byte**, and a byte is
the size of thing the Hydra's processor works with.

**To read a binary number**, write the place values over its bits, and add up the ones with a 1 under them:

| Worth | 128 | 64 | 32 | 16 | 8 | 4 | 2 | 1 |
|---|---|---|---|---|---|---|---|---|
| `01000001` | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 1 |

64 + 1 = 65. So the byte `01000001` is the number 65. (It is also the code for the letter **A**: a computer keeps
letters as numbers too. Appendix E has the whole list.)

**To write a number in binary**, go the other way. Start at the biggest place value, and at each one ask: does it
fit into what's left? If it does, write a 1 and take it away. If it doesn't, write a 0. Here is 200:

| Worth | 128 | 64 | 32 | 16 | 8 | 4 | 2 | 1 |
|---|---|---|---|---|---|---|---|---|
| Does it fit? | yes | yes | no | no | yes | no | no | no |
| What's left | 72 | 8 | 8 | 8 | 0 | 0 | 0 | 0 |
| Bit | 1 | 1 | 0 | 0 | 1 | 0 | 0 | 0 |

128 fits into 200, leaving 72. 64 fits into 72, leaving 8. 32 and 16 don't fit into 8. 8 fits, leaving nothing. So
200 is `11001000`.

The smallest byte is `00000000`, which is 0. The biggest is `11111111`, which is 128 + 64 + 32 + 16 + 8 + 4 + 2 + 1
= 255. So a byte can hold any whole number from 0 to 255, which is 256 different values. Each bit you add doubles
that:

| Bits | How many values | The biggest number |
|---|---|---|
| 1 | 2 | 1 |
| 4 | 16 | 15 |
| 8 (one byte) | 256 | 255 |
| 16 (two bytes) | 65,536 | 65,535 |

### Hexadecimal: binary made shorter

Binary is what the computer uses, but it's hard on people. `1111111100100000` is easy to copy wrongly and
impossible to say aloud. So programmers write binary numbers in a shorter way, called **hexadecimal**, or **hex**.
Hex is **base 16**: each place is worth *sixteen times* the one to its right.

Base 16 needs sixteen digits, and there are only ten. So after 9, hex carries on with letters: **A** is ten, **B**
eleven, **C** twelve, **D** thirteen, **E** fourteen and **F** fifteen.

| Decimal | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Hex | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B | C | D | E | F |
| Binary | `0000` | `0001` | `0010` | `0011` | `0100` | `0101` | `0110` | `0111` | `1000` | `1001` | `1010` | `1011` | `1100` | `1101` | `1110` | `1111` |

Look at the bottom row. Four bits make sixteen different patterns, and there are sixteen hex digits, so **each hex
digit stands for exactly four bits**. That's the whole reason hex is used. To turn binary into hex, split it into
groups of four bits from the right, and write each group's digit:

| Binary | `1100` | `1000` |
|---|---|---|
| Hex | C | 8 |

So `11001000` is **$C8**. To turn hex into binary, do the opposite: write the four bits for each digit. A byte is
always exactly two hex digits, from **$00** to **$FF**.

**To read a hex number** as decimal, use the place values again. In a two-digit number the left digit counts
sixteens and the right one counts ones:

| Hex | Worked out | Decimal |
|---|---|---|
| $C8 | 12 × 16 + 8 | 200 (the same 200 as before) |
| $FF | 15 × 16 + 15 | 255 |
| $10 | 1 × 16 + 0 | 16 (not ten!) |

With four digits, the places are worth 4,096, 256, 16 and 1:

| Worth | 4,096 | 256 | 16 | 1 |
|---|---|---|---|---|
| $FF20 | F (15) | F (15) | 2 | 0 |

15 × 4,096 + 15 × 256 + 2 × 16 + 0 = 61,440 + 3,840 + 32 = 65,312.

**To write a number in hex**, divide it by 16. The answer is the left digit and the remainder is the right one:
200 ÷ 16 is 12, remainder 8, and 12 is C, so 200 is $C8.

You will rarely need to do these sums. What matters is to recognise hex when you see it, to know that two hex
digits are one byte, and to remember a handful of sizes.

### How this book writes numbers

* A plain number, like 200, is decimal.
* A number with a **$** in front, like **$C8**, is hexadecimal, as it was on many computers of the 1980s. So
  **$10** is sixteen, not ten.
* A binary number is written out as its bits, like `11001000`, and the words around it say that it's binary.

Each of the Hydra's languages has its own way to write a hex or a binary number in a program. Their chapters
explain them; here they are side by side, for the number 200:

| Where | Hex | Binary |
|---|---|---|
| The assembler and HyForth | `$C8` | `%11001000` |
| HyBASIC | `&HC8` | `&B11001000` |
| hylang and `calc` | `#xC8` | `#b11001000` |

### Sizes worth knowing

| Decimal | Hex | What it is |
|---|---|---|
| 255 | $FF | The biggest number in a byte |
| 256 | $100 | How many values a byte can have |
| 1,024 | $400 | 1K: a kilobyte of memory |
| 8,192 | $2000 | 8K: one of the Hydra's RAM banks |
| 65,535 | $FFFF | The biggest number in 16 bits |
| 65,536 | $10000 | 64K: all the memory the processor can see at once |

The Hydra numbers each byte of its memory, and that number is the byte's **address**. An address is 16 bits long,
so addresses run from **$0000** to **$FFFF**: always four hex digits.

### Let the Hydra do it

Once you have a Hydra running (or the emulator, in the next chapter), the `calc` command converts for you. `-b b`
asks for the answer in binary, and `-b x` in hex. A hex number is typed with `#x` in front and a binary one with
`#b`, inside single quotes:

```session
/> calc -b b 200
11001000
/> calc -b x 200
C8
/> calc '#xFF20'
65312
/> calc '#b01000001'
65
```

> **TRY THIS:** Work these out on paper. What is `00101010` in decimal? What is 100 in binary? What is $20 in
> decimal? What is $7F in binary? Then check your answers with `calc` when your Hydra is running. (They are 42,
> `01100100`, 32 and `01111111`.)

Chapter 18 has much more about the Hydra's numbers: other bases, fractions, and numbers too big for any
calculator.

## 1.7 What you need

**To use the Hydra on a PC**, all you need is a web browser and one file: the emulator's page, `hydra-16.html`
(it's in the project's `reborn/bin` folder, ready to use; or someone may give it to you).
Chapter 2 shows you where it comes from, and how to use it.

**To build and use a real Hydra-16**, you will need:

* The Hydra-16 kit: the main circuit board, its parts, and at least one memory card. (A kit is planned; until there
  is one, section 3.9 says how to get a board and the parts.)
* A soldering iron and a few other tools (Chapter 3 lists them).
* An **ATX power supply**, the kind used in desktop PCs, with a 24-pin connector.
* A **power switch** (a toggle switch that stays on), and if you like, a reset button and a power light.
* A PC (Windows, Mac or Linux) and a **USB-to-serial adapter** with a DE-9 plug. It must be an "RS-232" adapter,
  the kind with a 9-pin connector, not a bare "TTL" cable.
* A programmer for flash ROM chips (such as a TL866 or T48), to put HydraOS into the ROM chips, unless your kit
  came with them already programmed.
* An **SD card reader module** for the SPI headers, and an SD card. A Hydra works without one, but it's where your
  files are kept: without it, you can only save your work on your PC, through the serial cable.
* Optional: headphones or speakers, and the Vera X video card.

## 1.8 Where to go next

* If you want to try the Hydra right away, turn to **Chapter 2** and run it on your PC.
* If you are building a Hydra, read **Chapters 3, 4 and 5** in order, carefully, before you start.
* If your Hydra is already built and working, skip ahead to **Part Two** and learn your way around HydraOS.
