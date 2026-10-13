# Topic 1. What the Hydra Teaches

Most school computing starts at the top: an app, a web page, a block of Python. The machine underneath stays a
mystery, and so does the reason anything works. The Hydra starts at the bottom. A student who has soldered the
memory chips in, watched the self test count them, and then asked `free` how much there is, *knows* what memory is,
in a way no diagram can give.

And the Hydra is small enough to know completely. One processor with six registers. One board whose every chip has
a job you can name. An operating system whose start-up fits in eight steps. That completeness is the point: it's a
whole computer that one person can hold in their head.

## The big ideas

Everything in the three books serves one of these seven ideas. They're the answer to "what will my students
actually know?"

| The idea | Where the manual teaches it | The labs that make it real |
|---|---|---|
| **A computer is a machine that follows instructions**, made of a few kinds of part: a processor, memory, and ways in and out | Chapters 1, 26 | 1, 24 |
| **Everything is numbers**, and the same number can be written in many ways | Sections 1.6, 26.2; Chapter 18 | 1, 15, 24 |
| **An operating system shares one machine among many programs** | Chapters 7, 10 | 4, 8; Project 5 |
| **Naming things, and one tree for everything**: files, devices, even running programs | Chapters 9, 12, 17 | 7, 10, 14 |
| **Small tools join up**: a program does one thing, and pipes and scripts combine them | Chapters 8, 11 | 5, 6, 9; Project 4 |
| **A language is a way of thinking**: the same problem looks different in each | Chapters 19 to 25 | 16 to 23 |
| **Layers**: each level of a computer is built from the one below, and you can open any of them | Chapters 26 to 29 | 24 to 26; Project 7 |

And under all seven runs an eighth, which is not about computers: **when it doesn't work, there's a reason, and you
can find it**, by observing carefully, changing one thing at a time, and writing down what happened. The build log
and the fault diary exist to teach that. It may be the most valuable thing a student takes away.

## What students will be able to do

By the end of the whole course (few courses will do it all: Topic 2):

* **Build**: solder a board of 2,200 joints to a working standard; read component values; test for power faults;
  find and fix a fault methodically.
* **Operate**: use a command line fluently; manage files; run several programs at once; automate a job with a
  script.
* **Program**: write, test and debug programs of 20 to 50 lines in at least one of three languages; read simple
  programs in the other two; choose a language for a job, with reasons.
* **Explain**: how a processor runs a program; how memory is addressed; how one processor runs many tasks; what an
  operating system, a driver, a file system and a shell are for.
* **Go below**: follow machine code by hand; write, assemble, disassemble and single-step a small program.

## Where it fits

For mapping to a curriculum, here's what the Hydra covers under the headings most computing frameworks use.

| Strand | What the Hydra offers | Where |
|---|---|---|
| **Computing systems** (hardware, and how hardware and software work together) | Its strongest suit: every component named and handled; memory, the bus, the processor; drivers; fault finding | Chapters 1, 3 to 7, 12 to 17, 26 |
| **Data and its representation** | Binary and hex; character codes; exact and inexact numbers; files as bytes | Sections 1.6, 26.2; Chapter 18; Appendix E; Lab 9's hex dump |
| **Algorithms and programming** | Sequence, selection, iteration, variables, procedures, recursion, decomposition, testing and debugging, in three contrasting languages and in machine code | Chapters 19 to 29 |
| **Operating systems** (often missing below university) | Tasks and scheduling; memory protection; file systems; namespaces; devices as files; shared resources and locks | Chapters 7, 9, 10, 12, 14, 17; the `race`, `chorus` and `philo` samples |
| **Networks and the Internet** | Almost nothing: the Hydra has a serial line, not a network. Serial communication, baud and protocols (XMODEM) are the nearest it comes | Sections 5.1, 5.2, 13.6, 14.7 |
| **Impacts and history of computing** | The 1980s home computer, and why its design still matters; open designs; making versus buying | Chapters 1, 19, 21, 23, 26 (their openings) |
| **Electronics and making** (design and technology) | Soldering; reading parts; power; measuring; working safely and to a standard | Chapters 3 to 5; the Companion's Part One |
| **Music, art and mathematics** | Notes as numbers; music as text; coordinates; turtle geometry; exact fractions; bases | Chapters 15, 16, 18; Labs 12, 13, 15; Projects 2, 3 |

Be honest with yourself about the gap: **networks, the web, data science and security are not here.** The Hydra
complements a mainstream course. It doesn't replace one.

## Who it's for

| Students | What works |
|---|---|
| **Ages 12 to 14** | The emulator; Labs 1 to 9, 12, 13, 15, 20 and 21; HyBASIC as the language; Projects 1, 3 and 4. A group build, with the adults doing the surface-mount chips. |
| **Ages 14 to 18** | Everything. A build in pairs or threes. Two languages, and Part Five for those who want it. |
| **University, first or second year** | A lab companion to a computer organization or operating systems course: Chapters 7, 9, 12, 17 and 26 to 29, with the system's source code as the further reading. Topic 2 has an outline. |
| **Adults and hobbyists** | The manual on its own; the Companion's build log is worth having. |

## What students need first

* **To read fluently**, and to follow written instructions exactly. This matters more than any mathematics.
* **Arithmetic**: the four operations, and the idea of a remainder. Fractions help in Chapter 18. Nothing beyond
  that until Part Five, which wants powers of two.
* **To type**, even slowly. A student who hunts for every key will struggle with the labs' timings: allow half as
  long again.
* **No programming.** The manual assumes none.

> **WATCH FOR:** The students who have "done coding" in a block language are often the most thrown by a command
> line: nothing to click, and it's case-sensitive. Don't skip Lab 3 for them. They need it most.
