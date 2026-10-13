# About This Book

Welcome! You are holding the manual for the **Hydra-16**, a computer you build yourself, one part at a time, and
then learn to program from the inside out. This book is your guide for the whole trip: from an empty circuit board
and a bag of parts, to a working computer, to the programs you will write for it.

You don't need to know anything about electronics or computers to start. If you can follow a recipe, use a
screwdriver carefully, and type on a keyboard, you can build and use a Hydra. The first chapters explain everything
from the beginning. Later chapters go deeper, and by the end of the book you will be writing programs in four
different languages, one of them the language the computer's chip speaks itself.

## Who this book is for

This book was written for **students aged 12 and up**, for their **teachers**, and for anyone else who wants to
understand how a computer really works. Some readers will build a Hydra from a kit; others will use the Hydra
*emulator*, a program that makes a PC behave exactly like a Hydra. Everything in this book except the building
chapters works the same way on both.

> **FOR TEACHERS:** Boxes like this one have suggestions for classrooms: what to try together, what to watch for,
> and how much time an activity takes. A whole class can use the emulator (Chapter 2) at the same time, with one
> or two real Hydras built as a group project.

Two more books go with this one. The **Student Companion** is a workbook: a build log with a box to tick for every
part, lab sheets and exercises for each chapter, projects, and quick-reference cards. The **Educator's Course
Guide** is for whoever is running a class or a club: course plans, lesson by lesson, a safety policy, how to equip
a room, how to mark the work, and the answers. You don't need either to use this book. It's complete on its own.

## How this book is organized

The book has five parts, plus appendices at the back for looking things up.

**Part One, Meet the Hydra**, introduces the computer, shows you how to run it on a PC, and takes you through
building and switching on your own.

**Part Two, Inside HydraOS**, starts with your first hour at the keyboard. Then it follows the computer from the
moment it is switched on, and teaches you to use it: its shells, its files, its windows, and every command it knows.

**Part Three, Devices**, explains how the Hydra talks to the world: the screen and keyboard, disks, sound, video
and the connectors on the board, and the *control files* that let you command each one.

**Part Four, Programming Languages**, teaches you to program in HyForth, hylang and HyBASIC, starting with the way
the Hydra handles numbers.

**Part Five, Machine Language**, goes all the way down to the 65C02 chip at the heart of the Hydra, with the
assembler, disassembler and debugger that let you program it directly.

**The appendices** hold the complete language references, the 65C02's instructions, the character codes, the memory
and I/O map, error messages, the emulator's options, the parts list and board diagrams, a glossary of computer words,
and every system call. The index, at the very end, finds the page where a word is explained.

### Which way through?

You don't have to read the parts in order. After Chapter 6, every reader knows enough to go where they like:

| If you want to | Read |
|---|---|
| Try a Hydra today, on a PC | Chapters 1 and 2, then Chapter 6 |
| Build one | Chapters 1 to 5, in order and carefully, then Chapter 6 |
| Start programming as soon as you can | Chapters 2 and 6, then Part Four: Chapter 18, and the language you like |
| Understand how the machine and its system work | Parts Two and Three, in order |
| Make sounds and pictures | Chapter 6, then Chapters 15 and 16, and the second chapter of your language in Part Four |
| Program the processor itself | Chapter 6, a language from Part Four, then Part Five |

Each chapter opens with a box, **IN THIS CHAPTER**, that says what you'll be able to do by the end of it, and what
it expects you to know already.

**When you want every detail**, the Hydra's **reference book** has it: the folder `docs` in the Hydra's files (start
at `docs/README.md`) describes the architecture, every driver's files and commands, the expansion cards and the
quirks. This book explains; that one specifies.

## How to read the examples

Most chapters have examples you can try. In the examples, what **you type** is printed in **bold**, and what the
Hydra prints is in plain type:

```session
/> 2 3 + .
5
/> echo Hello, Hydra!
Hello, Hydra!
```

The text before your typing, like `/>` or `%`, is the **prompt**: the computer's way of saying "I'm ready, type
something." You type only the bold part, then press the <kbd>RETURN</kbd> key (it may be called <kbd>ENTER</kbd>
on your keyboard).

Keys you press are shown like this: <kbd>RETURN</kbd>, <kbd>ESC</kbd>, <kbd>TAB</kbd>. When two keys are joined by a
dash, hold down the first and tap the second: <kbd>CTRL</kbd>-<kbd>C</kbd> means hold the <kbd>CTRL</kbd> key and
tap <kbd>C</kbd>. Keys like <kbd>CTRL</kbd>-<kbd>]</kbd> work the same way.

Computer people often write numbers in *hexadecimal* (base 16). In this book a number written with a dollar sign,
like **$FF20**, is hexadecimal. Section 1.6 explains what that means.

## This book's Hydra

Every example in this book was typed into a Hydra running HydraOS 1.0, and the answers shown are the ones it gave.
It was always the same Hydra:

| | This book's Hydra has |
|---|---|
| Memory | Three RAM modules, on one or two memory cards (so `free` shows 384 KB for each task) |
| SD card | HydraOS's sample card, in the first reader: `/sd/0` |
| Clock chip | None. Its clock starts at midnight on January 1, 2000 each time it's switched on |
| Vera X video card | None, except in Chapter 16 and where an example says it has one |
| Processor speed | 3.58 MHz |

Your Hydra may not be the same, and then a few of its answers won't be either. That's not a mistake. Here's what to
expect:

| Your Hydra | What's different from the book |
|---|---|
| **The web emulator**, as it comes | It has a Vera X and a clock chip. The start-up messages have a few more lines, `date` shows today's date, and `ps` lists one more task, `vid`. |
| **The terminal emulator**, as it comes | It has two RAM modules, so `free` shows 256 KB for each task. |
| **A real Hydra** with one RAM module | `free` shows 128 KB for each task. |
| **A real Hydra** with no SD card reader | There's no `/sd/0`. Read section 6.7 before you save anything. |
| Any Hydra | The numbers that count time (the `CPU` column of `ps`, the seconds of `date`) are never quite the same twice. |

To make an emulator into this book's Hydra exactly: in the web emulator, press **Setup…**, clear the boxes for the
Vera X card and the clock chip, and press **Power cycle with these**; in the terminal emulator, add `--modules 3`
when you start it (section 2.4). You don't have to. Nothing in this book depends on it.

If your Hydra's answer is different in some other way, check that you typed exactly what the book shows. Spaces
matter!

## Boxes

> **IN THIS CHAPTER:** Each chapter starts with one of these: what you'll be able to do by its end, and what you
> need to know, or to have, before you begin.

> **NOTE:** A note gives you extra information that is useful but not essential.

> **TIP:** A tip is a shortcut, or a way to stay out of trouble.

> **TRY THIS:** A "try this" box suggests something to do at the keyboard right now.

> **CAUTION:** A caution warns you about something that could hurt you, damage a part, or lose your work. Always
> read these, especially in the building chapters.

> **HOW IT WORKS:** These boxes explain what is going on inside, for readers who want to know more. You can skip
> them the first time through.

## About the Hydra-16

The Hydra-16 was designed by Daniel Struthers; his name is on the circuit board, next to the board's revision
number, **1.8D**. This book describes revision 1.8D of the board and version 1.0 of its operating system,
**HydraOS**.
