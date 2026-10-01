## **First steps with the Hydra-16**

A tutorial for a first hour: the Hydra in the emulator, HyForth at its prompt, files on a card, then a program of
your own in C and in assembly.  It needs no board; everything here works the same on one.  What it shows is
explained fully in the [HyForth guide](using/hyforth.md) and the [Programmer's Guide](programming/README.md).

You need [Node.js](https://nodejs.org) and a clone of the repository.  For the C and assembly parts, also
[cc65](https://cc65.github.io/) ([getting started](getting-started.md) says where the build looks for it).

### **1. Switch it on**

From the top of the repository:

```
node sim/hydrasim.js -i
```

The Hydra boots in your terminal: its power-on test, the welcome, and HyForth's prompt, `/ram/1>` (the current
directory: with no card, the shell's own area on the RAM disk, where files can be saved until a reset).  Your terminal is now the Hydra's serial console.  Ctrl-A then x quits the emulator, and Ctrl-A
then h lists its other keys.

### **2. HyForth**

HyForth is the Hydra's shell and its language.  Type a line, press Enter, and it runs each word in turn.  Numbers go
on a stack, and words take them from it:

```
/ram/1> 1 2 + .
 0003
```

`.` prints the top number in hex, 4 digits.  For decimal, say so:

```
/ram/1> decimal 12 12 * .
 144
/ram/1> hex
```

Make a word of your own with `:` and `;`.  It works like the built-in ones:

```
/ram/1> : sq dup * ;
/ram/1> 7 sq .
 0031
```

The Up arrow brings back the lines you typed (Down goes forward again), and Left, Right, Home and End move along
the line, so a mistake is quick to fix.  `words` lists every word there is.  The Hydra multitasks: Ctrl-] then a
task's number switches the console to it, and `ps` lists the tasks.

### **3. Files**

Even with no card, the Hydra has files: the ones built into its ROM, under `/rom`.

```
/ram/1> ls /rom/bin
code.hyx 2156
hello.hyx 8616
...
scom.zsm 838
/ram/1> cat /rom/README
```

A program runs by its name: `hello` runs `/rom/bin/hello.hyx` (a C program), and `scom` plays the song
`scom.zsm` on the YM2151 (the emulator times the music but makes no sound; `--ym-vgm FILE` saves it to listen to).
Words that fail say why:

```
/ram/1> cat /rom/nope

 !IO ERR! not found
```

### **4. A card**

Real work goes on an SD card.  The emulator's card is an image file, which `sim/tools/hydrafs.js` makes and fills
from the PC (Ctrl-A x first):

```
node sim/tools/hydrafs.js mkfs card.img 16 WORK          a new 16 MB card, labelled WORK
node sim/tools/hydrafs.js mkdir card.img bin
node sim/tools/hydrafs.js put card.img programs/c/bin/hello.hyx bin
node sim/hydrasim.js -i --sd card.img
```

The Hydra boots into the card: the prompt is `0:/>` (volume 0, its root directory).  `ls`, `cd`, `cat`, `cp`, `rm`
and `mkdir` work on it, and `edit notes.txt` is a small editor (`h` in it for help).  A program in the card's
`/bin` runs by name too, and gets the rest of the line as its arguments:

```
0:/> hello world
Hello from C on the Hydra-16!
1 arguments: [world]
```

Put the lines you want at every start in `boot.hys` at the card's root (a script: lines as you'd type them).  On
a real Hydra, the card is the same format: copy an image to it with a disk imager, or write files with the same
tool.

### **5. A program in C**

Save this as `greet.c`, anywhere:

```c
#include <stdio.h>

int main (int argc, char* argv[])
{
    int i;
    for (i = 1; i < argc; ++i) {
        printf ("Hello, %s!\n", argv[i]);
    }
    return argc > 1 ? 0 : 1;
}
```

Build it, put it on the card, and run it:

```
node build.js c                                   the C library, once (and after pulling changes to it)
node build.js prog greet.c                        your program: programs/c/bin/greet.hyx
node sim/tools/hydrafs.js put card.img programs/c/bin/greet.hyx bin
node sim/hydrasim.js -i --sd card.img
```

```
0:/> greet Hydra world
Hello, Hydra!
Hello, world!
0:/> status .
 0000
```

`main`'s value is the program's exit status: run `greet` with no names and `status .` gives 1.  The C library
has the standard calls (stdio, files and directories, the environment, `malloc`, `time`), and the Hydra's own
(tasks, semaphores, sound): the [C Programmer's Guide](programming/c.md) has them all, with a section on making
programs small and fast.

### **6. A program in assembly**

`programs/asm/samples/hello.s` is a complete program: a header (`HYX_HEADER`, from `programs/asm/hyx.inc`), then
code that calls the ROM's `WRITE_CHAR` thunk (`$F803`) to print.  Copy it to a name of your own in the same
folder, change the message, and build every sample there:

```
node build.js asm                                 programs/asm/samples/*.s -> programs/asm/bin/*.hyx
node sim/tools/hydrafs.js put card.img programs/asm/bin/hello.hyx bin
```

The calls a program can make are the thunks at `$F800` and up ([the API index](programming/rom-layout.md#api-index-the-thunks)):
the console, files, memory, tasks, the clock.  A program has its own task: 32K of RAM, its own zero page and
stack, and its own fds, so a crash in it doesn't take the shell down (Ctrl-C ends it).

### **7. Where next**

* **Try the rest of the system:** pipelines (`words | wc . . .`), background programs (`play /rom/bin/scom.zsm 0 &`),
  the environment (`echo /sd/0/bin > /env/PATH`), `/dev/proc` (`cat /dev/proc`): the [HyForth guide](using/hyforth.md).
* **See how it works:** tasks and the scheduler, interrupts, memory, the Plan 9-style IO, writing a device server:
  the [Programmer's Guide](programming/README.md).
* **Build the ROM itself:** `node build.js test` builds everything and runs the regression tests
  ([getting started](getting-started.md)); [the emulator](tools/emulator.md) has a debugger's tools (`--trace`,
  `--watch`, `--pc`, `--profile`).
* **The board:** the [hardware reference](hardware.md); what's planned: [plans/NEXT_STEPS.md](plans/NEXT_STEPS.md).
