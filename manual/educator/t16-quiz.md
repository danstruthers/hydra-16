# Topic 16. A Bank of Questions

Sixty short-answer questions, in six sets of ten, with answers. Each set goes with a stretch of the course, and
runs from recall (the first few) to explanation (the last few). Use them as a starter, an exit ticket, a homework
or a test: Topic 7 says how they fit.

The answers are models. Any answer that shows the same understanding is right.

## Set 1. The machine (Chapters 1 and 7; Labs 1 and 4)

| | Question | Answer |
|---|---|---|
| 1 | What is the Hydra's processor called? | The W65C02S (a 65C02). |
| 2 | How many bits are in a byte, and how many values can a byte hold? | Eight; 256 (0 to 255). |
| 3 | Write 255 in hex, and $10 in decimal. | $FF; 16. |
| 4 | What's the difference between RAM and ROM? | RAM can be written and forgets when the power goes off; ROM keeps its contents and can't be changed by a program. |
| 5 | How many tasks can the Hydra run at once, and how much memory of its own does each have? | Sixteen; 32K each (plus RAM banks from the memory cards). |
| 6 | What is the bus? | The paths that join the processor to memory and to the other chips: addresses, data and control lines. |
| 7 | The 65C02 can address 64K. How does the Hydra give it megabytes? | Parts of the 64K are "windows" onto bigger memories; a register chooses which piece (bank) shows through each. |
| 8 | What is the kernel, and where is it kept? | The heart of HydraOS, which runs the other programs and shares the machine between them; in the kernel ROM, U6. |
| 9 | What does the first program, `init`, do? | Sets the system up (the console, the RAM disks, the namespace), starts the shell, and starts it again if it ends. |
| 10 | What stops one task from spoiling another's memory? | Each task's 32K is chosen by the T register, which only the kernel changes; a task has no address that reaches another task's RAM. |

## Set 2. Using HydraOS (Chapters 6 and 8 to 11; Labs 3 and 5 to 9)

| | Question | Answer |
|---|---|---|
| 1 | What does the prompt `/>` tell you? | HyForth, the shell, is ready for a line; and the current directory is `/`. |
| 2 | Which key stops a running program? Which says "my typing has ended"? | <kbd>CTRL</kbd>-<kbd>C</kbd>; <kbd>CTRL</kbd>-<kbd>D</kbd>. |
| 3 | Why is `/ram` a poor place to keep work? Where is a good one? | It's memory, emptied at every reset and power-off. The SD card, `/sd/0`. |
| 4 | What's the difference between `>` and `>>`? | `>` sends output to a file, replacing what was there; `>>` adds it to the file's end. |
| 5 | What does `ls /rom/bin \| wc -l` do? | Counts the names in `/rom/bin`: `ls`'s output is piped into `wc -l`, which counts lines. |
| 6 | In rc, `x=(a b c)`. What are `$#x` and `$x(2)`? | 3; `b`. |
| 7 | How does the shell decide whether a line is HyForth's or rc's? | By its first word: a Forth word or a number makes it Forth; anything else goes to rc. |
| 8 | What's a path? Give a full one and a relative one. | The directories that lead to a file. `/sd/0/work/a.txt` starts from the root; `work/a.txt` from the current directory. |
| 9 | What does `sleep 60 &` do, and how would you stop it? | Starts `sleep` in the background, so the prompt comes straight back. Find its task with `ps`, and `kill` that number. |
| 10 | What's a script, and how do you make one into a command? | A file of commands. Start it with `#!/bin/rc`, and put it in a directory that's part of `/bin` (`/sd/0/bin`, to keep it). |

## Set 3. Devices (Chapters 12 to 17; Labs 10 to 14)

| | Question | Answer |
|---|---|---|
| 1 | What is a driver? | A program that looks after a device, and offers it to other programs as files. |
| 2 | What are the two things you can do with a control file? | Read it, to see the device's settings; write a line to it, to give the device a command. |
| 3 | What does `invalid argument` mean, from a device? | The driver didn't understand the command, or a number was out of range. |
| 4 | Name the Hydra's three kinds of disk. Which keeps its files when the power is off? | SD cards, RAM disks and the ROM disk. The SD cards (and the ROM disk, which can't be changed). |
| 5 | What's a block? What's HydraFS? | 512 bytes: the unit a disk is read and written in. The Hydra's file system: its way of arranging files on a disk. |
| 6 | What note is number 60? How much higher is note 72? | Middle C. An octave. |
| 7 | In MML, what do `o4`, `l8` and `r` mean? | Octave 4; notes are eighth notes unless they say otherwise; a rest. |
| 8 | On the Vera X's bitmap, where is the dot (0, 0)? Which way does *y* go? | The top left. Down. |
| 9 | Why does an LED on a pin need a resistor, and why is it wired from +5 V to the pin? | To limit the current. Because a pin can take in more current than it can give out, so the LED lights when the pin is 0. |
| 10 | "On the Hydra, everything is a file." What does that buy? | One set of commands and one set of file words reach everything: disks, devices, even the tasks. Nothing new to learn for each device. |

## Set 4. Numbers and programming (Chapters 18 to 25; Labs 15 to 23)

| | Question | Answer |
|---|---|---|
| 1 | What does `calc '10 / 4'` print, and why isn't it 2.5? | `5/2`. The Hydra keeps fractions exactly, as fractions. |
| 2 | Write 10 in binary and in hex. | 1010; A. |
| 3 | In HyForth, what does `2 3 + 4 * .` print? | 20. |
| 4 | What do `dup`, `swap` and `drop` do? | Copy the top of the stack; exchange the top two; throw the top away. |
| 5 | In hylang, what's the value of `(* 2 (+ 3 4))`? | 14. |
| 6 | In hylang, what's the difference between `{+ 1 2}` and `(+ 1 2)`? | The first is a list, left as it is; the second is a call, worked out: 3. |
| 7 | In HyBASIC, what does `A = A + 1` mean? | Work out `A + 1`, and put the answer back in `A`: add one to `A`. |
| 8 | What's the difference between a `FOR` loop and a `DO` loop? | `FOR` counts a known number of times; `DO` goes on while (or until) a test is true. |
| 9 | A variable, a loop and a decision: write one line of each, in any one of the three languages. | For example, in HyBASIC: `N = 5`; `FOR I = 1 TO 10 : PRINT I : NEXT`; `IF N > 3 THEN PRINT "big"`. |
| 10 | HyForth counts to ten thousand more than ten times faster than HyBASIC. Why, and what does it cost? | Its numbers are 16-bit cells, which the processor adds directly; HyBASIC's can be any size, and are handled by a library. The cost: no big numbers or fractions without extra work. |

## Set 5. Machine language (Chapters 26 to 29; Labs 24 to 26)

| | Question | Answer |
|---|---|---|
| 1 | Name the 65C02's three 8-bit registers for data. | A, X and Y. |
| 2 | What does the program counter hold? | The address of the next instruction. |
| 3 | What's the difference between `lda #$10` and `lda $10`? | The first loads the number $10; the second loads the byte stored at address $10. |
| 4 | $FF + $01, in one byte: what's the result, and what's the carry? | $00, with a carry of 1. |
| 5 | What does `bne loop` do? | Goes to `loop` if the last result wasn't zero (the Z flag is 0); otherwise carries on. |
| 6 | What do `jsr` and `rts` do? | Call a subroutine (remembering where to come back to, on the stack); return from one. |
| 7 | What's the zero page, and why is it special? | Addresses $0000 to $00FF. Instructions that use it are shorter and quicker, and pointers must be kept there. |
| 8 | What does an assembler do? And a disassembler? | Turns assembly language into machine code; turns machine code back into assembly language. |
| 9 | In `db`, what do `s`, `b` and `c` do? | Step one instruction; set a breakpoint; continue to the next breakpoint or the end. |
| 10 | A program assembles with no errors, and prints the wrong thing. What does that tell you, and what do you do? | The assembler only checks that the program is written correctly, not that it's right. Step through it in `db`, comparing each register with what you expected. |

## Set 6. Building (Chapters 3 to 5; the build log)

| | Question | Answer |
|---|---|---|
| 1 | Name three safety rules for soldering. | Any three: glasses on; the iron in its stand; don't breathe the fumes; no food or drink; wash your hands; unplug the power before touching the board. |
| 2 | What do you do for a burn? | Cool running water, at once, for at least ten minutes; tell an adult. |
| 3 | A resistor's bands are brown, black, orange. What's its value? | 10 kΩ (1, 0, and three zeros). |
| 4 | A capacitor is marked 104. What's its value? | 100 nF (10, and four zeros, in picofarads). |
| 5 | Which parts must go in the right way round? | Electrolytic capacitors, chips and their sockets, resistor networks, the reset chip, LEDs. |
| 6 | What does a good solder joint look like? | A small, shiny cone that has flowed onto both the pad and the lead. |
| 7 | Why is the power tested before any chip goes in? | A power fault found then harms nothing; with the chips in, it could destroy them. |
| 8 | What are the terminal's settings for a Hydra? | 9600 baud, 8 data bits, no parity, 1 stop bit, flow control XON/XOFF. |
| 9 | The power light is on, and nothing appears on the terminal. Name the first three things to check. | The port and the speed; the cable (RS-232, not TTL; not null-modem); press reset. Then the jumpers and the chips. |
| 10 | Why change only one thing at a time when finding a fault? | So that you know which change made the difference. Change two things, and whatever happens, you've learned nothing. |
