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

## How this book is organized

The book has five parts, plus appendices at the back for looking things up.

**Part One, Meet the Hydra**, introduces the computer, shows you how to run it on a PC, and takes you through
building and switching on your own.

**Part Two, Inside HydraOS**, follows the computer from the moment it is switched on, and teaches you to use it:
its shells, its files, its windows, and every command it knows.

**Part Three, Devices**, explains how the Hydra talks to the world: the screen and keyboard, disks, sound, video
and the connectors on the board, and the *control files* that let you command each one.

**Part Four, Programming Languages**, teaches you to program in HyForth, hylang and HyBASIC, starting with the way
the Hydra handles numbers.

**Part Five, Machine Language**, goes all the way down to the 65C02 chip at the heart of the Hydra, with the
assembler, disassembler and debugger that let you program it directly.

**The appendices** hold the complete language references, the 65C02's instructions, the character codes, the memory
and I/O map, error messages, the emulator's options, the parts list and board diagrams, a glossary of computer words,
and every system call. The index, at the very end, finds the page where a word is explained.

You can read the book from front to back, and if you are building a Hydra you should read Part One that way. After
that, feel free to jump to whatever interests you most. Each chapter tells you what it expects you to know already.

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

## Boxes

> **NOTE:** A note gives you extra information that is useful but not essential.

> **TRY THIS:** A "try this" box suggests something to do at the keyboard right now.

> **CAUTION:** A caution warns you about something that could hurt you, damage a part, or lose your work. Always
> read these, especially in the building chapters.

> **HOW IT WORKS:** These boxes explain what is going on inside, for readers who want to know more. You can skip
> them the first time through.

## About the Hydra-16

The Hydra-16 was designed by Daniel Struthers; his name is on the circuit board, next to the board's revision
number, **1.8D**. This book describes revision 1.8D of the board and version 1.0 of its operating system,
**HydraOS**.

Every example in this book was typed into a Hydra running HydraOS 1.0, and the answers shown are the ones it gave.
If your Hydra answers a little differently, check that you typed exactly what the book shows. Spaces matter!
