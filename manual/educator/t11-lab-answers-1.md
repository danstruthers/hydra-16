# Topic 11. Labs 1 to 9

## Lab 1. Meet the Computer

**1. The parts.**

| Part | Reference | Its job |
|---|---|---|
| The processor | U1 | Follows the instructions: everything else serves it |
| The crystal | Y1 | The steady beat that the Hydra's clocks are made from |
| The task RAM | U7 | 32K of memory for each of the sixteen tasks |
| The kernel ROM | U6 | Holds the kernel, the heart of HydraOS |
| The VIA | U2 | Timers (one makes the tick), the SPI bus, and the I/O header's pins |
| The serial chip | U3 | Sends and receives characters, a bit at a time, for the terminal |
| The sound chip | U38 | The YM2151: eight voices of music |
| The socket for the Vera X | J12 (slot 0) | Where the video card connects |
| Where the power comes in | J11 | The ATX power supply's plug |

**2. The conversions.**

| Decimal | Binary | Hex |
|---|---|---|
| 10 | 0000 1010 | $0A |
| 65 | 0100 0001 | $41 |
| 200 | 1100 1000 | $C8 |
| 16 | 0001 0000 | $10 |
| 255 | 1111 1111 | $FF |
| 42 | 0010 1010 | $2A |
| 128 | 1000 0000 | $80 |

**3. The Hydra's check.** (`calc` doesn't print leading zeros.)

```session
/> calc -b b 65
1000001
/> calc -b x 200
C8
/> calc '#x2A'
42
/> calc '#b11111111'
255
```

**Questions.**

1. Input: the serial port (the terminal's keyboard), or the pins of the I/O header. Processing: the W65C02S, U1.
   Memory: the RAM chips and the ROM chips. Output: the serial port again (the terminal's screen), the sound chip,
   the Vera X. Accept any sensible part for each.
2. Its sixteen tasks: it can run up to sixteen programs at once, each in memory of its own.
3. 32 × 1,024 = 32,768 bytes. Sixteen of them: 512K (524,288 bytes), which is exactly the size of the task RAM
   chip, U7.
4. 256, from 0 to 255. Eight bits, each 0 or 1: 2 × 2 × 2 × 2 × 2 × 2 × 2 × 2.
5. RAM can be written as well as read, and forgets everything when the power goes off. ROM keeps what it holds,
   and a program can't change it. HydraOS is in ROM so that it's there the instant the Hydra is switched on (and
   so that no program can damage it).
6. Four bits. So a byte is two hex digits, and a 16-bit address is four.

**Challenge.** For example, 2012 is 0111 1101 1100, or $7DC. $FFFF is 65,535. With five hex digits, $FFFFF:
1,048,575, which is one less than a megabyte.

## Lab 2. A Hydra on Your PC

**1.** `HyForth (Forth 2012), bye to end`, and then the prompt, `/>`.

**2.** `5`, and then the twelve names at the top of the file tree, from `bin/` to `tmp/`, each with a `/`.

**3.** **Pause** stops the Hydra dead: nothing typed appears until it's pressed again. **As fast as it goes** makes
everything quicker (the clock at the top right shows how much). The **size** menu changes the console's columns
and rows, and the Hydra's windows follow. **Copy text** puts the console's whole text, scrolled-off lines too, on
the PC's clipboard. **Sound** turns the sound on (a browser won't make a sound until the user asks).

**4. The experiment.**

| After | `/ram/a.txt` | `/sd/0/a.txt` |
|---|---|---|
| Reset | gone | kept |
| Power cycle | gone | kept |
| Closing and reopening the page | gone | kept |

The RAM disk is made afresh every time HydraOS starts, whatever the reason. The SD card is kept by the browser.
(Students who report that the SD card's file went too have usually pressed **New** or **Samples** in Setup, or
are on a browser that clears its storage.)

**5.** As it comes: a Vera X card in slot 0, without its keyboard and mouse; a clock chip; three RAM modules; the
sample SD card in reader 0, and no card in the Vera X's own slot.

**6.** With the Vera X, `ps` has one more line: task 10, `vid`, the screen's driver. Without a Vera X, `vid` starts,
finds nothing to drive, and ends.

**Questions.**

1. A program that makes one computer behave like another, by imitating its chips. The same: the processor, the
   memory, the devices, HydraOS itself, and so every program. Different: no soldering, no real pins or sockets;
   a perfect "serial cable"; SD cards that are files; and it can run faster than real.
2. On the SD card, `/sd/0`: it's the only place that survived all three.
3. Reset presses the reset button: HydraOS starts again. Power cycle switches off and on: the same, and everything
   in memory is lost too. (To a student, the visible difference is small: both empty the RAM disks. That's the
   point.)
4. **Save card**, which gives a file holding the whole card. **Load…** on the other computer brings it back.

**Challenge.** <kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>S</kbd> shows every task and what it's doing.
<kbd>CTRL</kbd>-<kbd>A</kbd> <kbd>B</kbd> stops the processor between two instructions; `r` shows its registers at
that instant, and `c` lets it carry on.

## Lab 3. First Steps

**1.**

```session
/> date
2000-01-01 00:00:04
/> echo Hello, Hydra!
Hello, Hydra!
/> ls /sd/0
bench/
sample/
songs/
/> 2 3 + .
5
/> free
ram     384 KB a task (3 modules)
shared  1024 KB, 512 KB in segments (1), 512 KB free
```

**2.**

```session
/> dat
rc: dat: not found
/> LS
rc: LS: not found
/> 2 3 plus .
plus ?
/> 2 3+ .
3+ ?
/> cat /ram/nothing
cat: /ram/nothing: not found
```

| The message | Which means |
|---|---|
| `rc: dat: not found` | There's no program called `dat` |
| `rc: LS: not found` | Capitals count: the program is `ls` |
| `plus ?` | The line began with a number, so it's Forth; and Forth has no word `plus` |
| `3+ ?` | Without a space, `3+` is one word, and Forth doesn't know it |
| `cat: /ram/nothing: not found` | The program `cat` ran, and couldn't find that file |

**3.** <kbd>CTRL</kbd>-<kbd>C</kbd>.

**4.** It prints each line back, as soon as <kbd>RETURN</kbd> is pressed. <kbd>CTRL</kbd>-<kbd>D</kbd>, at the start
of a line, ends it.

**5.**

| To start it | Its prompt | To leave it |
|---|---|---|
| `rc` | `%` | `exit` |
| `basic` | `>` | `SYSTEM` |
| `hylang` | `hylang>` | `(exit)` |
| `: test` | Spaces (no prompt at all) | `;` |

**6.** <kbd>CTRL</kbd>-<kbd>O</kbd> and then <kbd>RETURN</kbd> saved it; <kbd>CTRL</kbd>-<kbd>X</kbd> left.

**7.** Its mode (who may read and write it), which disk it's on (`f0`: HydraFS, card 0), its length in bytes, when
it was last changed, and its name.

**8.** The first time, four tasks each print a line of a song a letter at a time, with nothing to keep them
apart, and their letters are shuffled together. With a lock (a mutex) held for a whole line, each line comes out
whole, though in whatever order the tasks happened to run. With a baton passed from one to the next, they come
out whole and in order.

**Questions.**

1. The program's sign that it's ready for a line. The Hydra's also shows the current directory.
2. Just `two`: `>` starts the file afresh each time. With `>>`, both lines: `one`, then `two`.
3. It's made of memory, and is emptied at every reset and power-off. `/sd/0`, the SD card (or `/pc`, with no card
   reader).
4. The name is misspelled; a capital letter where a small one should be; the file is in another directory (so
   it needs its path); or the first word was meant as Forth and isn't a Forth word.
5. Chapter 11 of the manual, under W: the commands are in alphabetical order.

**Challenge.** For example:

```session
/> : me ." My name is Sam" cr ;
/> me
My name is Sam
```

```session
> 10 FOR I = 1 TO 5
> 20 PRINT "Sam"
> 30 NEXT I
> RUN
Sam
Sam
Sam
Sam
Sam
```

```session
hylang> (print "Sam")
Sam
=> NIL
```

## Lab 4. From Power to Prompt

**1.**

```session
/> cat /dev/kmesg | head -6

Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000 20:0/00/0000
POST ok
RAM modules: 03
```

**2.** (The task numbers, and `ps`'s own, may differ.)

```session
/> ps
task  state   parent     cpu group  name
   0  ready   -          1.5     0  kernel
   1  wait    -          0.1     1  init
   2  wait     1         0.3     2  forth
   3  event    1         0.0     1  wstart
   4  ready    2         0.0     2  ps
  11  idle    -          0.0    11  gpio
  12  idle    -          0.0    12  snd
  13  idle    -          0.1    13  kdev
  14  idle    -          1.6    14  storage
  15  idle    -          0.5    15  cons
```

| Task | Name | What it's for |
|---|---|---|
| 0 | `kernel` | The kernel's own task: it also runs when nothing else can |
| 1 | `init` | Set the system up, and now keeps the shell running |
| 2 | `forth` | The shell you're typing at |
| 3 | `wstart` | Waits to open a new window when you ask |
| 4 | `ps` | The command itself |
| 11 to 15 | `gpio`, `snd`, `kdev`, `storage`, `cons` | The drivers: the pins, the sound, the kernel's devices, the disks, the console |

**3.** `ps` is a program, so while it runs, it's a task: it lists itself. When it finishes, its task is free
again, for the next program that starts.

**4.**

<!-- session-script
sleep 60 &
ps
kill 4
ps
-->

```session
/> sleep 60 &
/> ps
task  state   parent     cpu group  name
   0  ready   -          2.0     0  kernel
   1  wait    -          0.1     1  init
   2  wait     1         0.3     2  forth
   3  event    1         0.0     1  wstart
   4  sleep    2         0.0     4  sleep
   5  ready    2         0.0     2  ps
  11  idle    -          0.0    11  gpio
  12  idle    -          0.0    12  snd
  13  idle    -          0.1    13  kdev
  14  idle    -          1.7    14  storage
  15  idle    -          0.6    15  cons
/> kill 4
/> ps
task  state   parent     cpu group  name
   0  ready   -          3.6     0  kernel
   1  wait    -          0.1     1  init
   2  wait     1         0.4     2  forth
   3  event    1         0.0     1  wstart
   5  ready    2         0.0     2  ps
  11  idle    -          0.0    11  gpio
  12  idle    -          0.0    12  snd
  13  idle    -          0.1    13  kdev
  14  idle    -          1.8    14  storage
  15  idle    -          0.9    15  cons
```

`sleep` was in the state `sleep`, with the shell (task 2) as its parent. After `kill`, it's gone from the list.

**5.** 384 KB for each task, on the manual's Hydra: three RAM modules, at 128 KB each.

```session
/> free
ram     384 KB a task (3 modules)
shared  1024 KB, 512 KB in segments (1), 512 KB free
```

**6.** Two numbers a few hundred apart: there are 200 ticks in a second.

**Questions.**

1. Through "windows": parts of the 64K that show a piece of a bigger memory, with a register choosing which
   piece. $8000 to $9FFF shows one 8K RAM bank at a time, and $A000 to $DFFF one 16K bank of ROM.
2. A program that's running, with memory of its own. The T register chooses which task's 32K the processor sees,
   so a task simply can't address another task's memory; and only the kernel changes T.
3. The processor reads the reset vector; the power-on self test; the drivers start; `init` starts; the shell
   starts.
4. A program that looks after a device, so that other programs don't touch the chips themselves. `cons`: the
   serial port and the windows. `storage`: the SD cards and the other disks. `snd`: the sound chip. (Also `gpio`:
   the I/O header; `kdev`: the clock, the tasks and the environment; `vid`: the Vera X.)
5. It starts another, so there's always a shell to type at. Without that, one mistyped `bye` would leave the
   Hydra with no way in.
6. A timer in the VIA interrupts the processor 200 times a second. On each tick, the kernel can stop the task
   that's running and give another its turn, and it counts the time.

**Challenge.** `mods | wc -l` says 17: a heading, and sixteen modules. `forth` is in 1 bank (bank 30), `basic` in
5 (banks 9 to 13), and `hylang` in 8 (banks 14 to 21). hylang is by far the biggest language to *implement*, and
Forth the smallest: one reason Forth was so popular on small machines.

## Lab 5. Talking to the Shell

**1.**

```session
% pets=(cat dog fish)
% echo $pets
cat dog fish
% echo $#pets
3
% echo $pets(2)
dog
% echo I have a $pets(1)
I have a cat
% echo 'I have a $pets(1)'
I have a $pets(1)
```

**2.** `3`. The `<` makes `wc` read the file as its input, as if it had been typed.

<!-- session
echo one >/ram/n.txt
echo two >>/ram/n.txt
echo three >>/ram/n.txt
-->

```session
% cat /ram/n.txt
one
two
three
% wc -l </ram/n.txt
      3
```

**3.** 47: the programs on the ROM disk. 6: the names in `/sd/0/sample`. And the three words, in alphabetical
order.

<!-- session
echo one >/ram/n.txt
echo two >>/ram/n.txt
echo three >>/ram/n.txt
-->

```session
% ls /rom/bin | wc -l
     47
% ls /sd/0/sample | wc -l
      6
% cat /ram/n.txt | sort
one
three
two
```

**4.** Two words: the date, and the time.

```session
% echo today is `{date}
today is 2000-01-01 00:00:05
% now=`{date}
% echo $#now
2
```

**5.** Empty after the `ls` that worked, and `1` after the one that failed. `||` runs the second command only if
the first failed; `&&` only if it worked.

```session
% ls /rom >/dev/null
% echo $status

% ls /nowhere
ls: /nowhere: not found
% echo $status
1
% ls /nowhere >[2]/dev/null || echo not there
not there
% ls /rom >/dev/null && echo there
there
```

**6.**

```session
% ls /rom/bin/s*
/rom/bin/scom
/rom/bin/slay
/rom/bin/sleep
/rom/bin/sort
% ls /rom/bin/[x-z]*
/rom/bin/xd
/rom/bin/xmodem
```

`/rom/bin/??` lists the thirteen programs with two-letter names: `as`, `cp`, `db`, `df`, `du`, `ed`, `ls`, `mv`,
`ns`, `ps`, `rm`, `wc` and `xd`.

**Questions.**

1. By the line's first word: if it's a Forth word that HyForth knows, or a number, the line is Forth; otherwise
   it's rc's. A `%` and a space at the start sends a line to rc whatever it begins with; or type `rc` and work
   there.
2. The first prints the variable's words; the second prints the six characters `$pets`. Quotes are for when a
   word has spaces or special characters in it that the shell should leave alone.
3. To `/dev/null`, a file that throws away whatever is written to it. `2` is the number of the **standard
   error** file: programs write their error messages there, and their results to file 1, so that the two can be
   sent to different places.
4. It looks in each directory in `$path`, in order: first `.`, the current directory, then `/bin`. `whatis ls`
   says `/bin/ls`.
5. `ls /rom/bin/s* | wc -l`, or `ls /rom/bin | grep '^s' | wc -l`. Both say 4.

**Challenge.**

```session
% ls /rom/bin | sort -r | head -3
xmodem
xd
wstart
% ls /rom/bin | grep e | wc -l
     15
```

Six, because each program in a pipeline is a task: the Hydra has sixteen, and ten are in use at rc's prompt (the
kernel, init, five drivers, wstart, HyForth and rc: `ps` shows them, and itself), which leaves six. For a job that
needs seven programs, send the half-way result to a file in `/ram`, and start a second pipeline from that file.

## Lab 6. Scripts

**1.**

```session
% for(i in 1 2 3) echo number $i
number 1
number 2
number 3
% for(f in /sd/0/sample/*) echo I found $f
I found /sd/0/sample/as
I found /sd/0/sample/c
I found /sd/0/sample/hi
I found /sd/0/sample/nsum
I found /sd/0/sample/tick
I found /sd/0/sample/upper
```

**2.** The first, the second and the fourth. `Sam` matches `Sam`, and it matches `S*`. It doesn't match `Ann`, so
that `if` prints nothing, and the `if not` that follows it does.

```session
% name=Sam
% if(~ $name Sam) echo hello Sam
hello Sam
% if(~ $name S*) echo starts with S
starts with S
% if(~ $name Ann) echo hello Ann
% if not echo you are not Ann
you are not Ann
```

**3.**

```session
% fn shout { for(w in $*) echo $w! }
% shout one two three
one!
two!
three!
```

**4.** `#!/bin/rc` says which program should run the file: rc. The `#` line is a comment, for people.

<!-- session
cd /ram
!file samples/tour
-->

```session
/ram> rc /ram/tour
In /rom there are:
README
bin/
doc/
lib/
songs/
In /ram there are:
bin/
lib/
tour
In /sd/0 there are:
bench/
sample/
songs/
```

**5.** Into `/ram/bin`. `/bin` is a union, and new files made in a union go into the first of its directories
that allows it: the RAM disk's `bin`. (`ls /sd/0/bin` says `not found`: the sample card has no `bin` directory.)

**6.** One way:

```text
#!/bin/rc
# tour - say what's on each disk, or in each directory named
dirs=$*
if(~ $#dirs 0) dirs=(/rom /ram /sd/0)
for(d in $dirs) {
	echo In $d there are:
	ls $d
}
```

**Questions.**

1. A function lives in the shell's memory, and is gone when that shell ends. A script is a file: it's there
   tomorrow, and any shell can run it. A function for something you want for the next ten minutes; a script for
   anything you want to keep.
2. The shell. It replaces the pattern with the list of matching names *before* `for` or any command sees it.
3. It's lost, with the rest of the RAM disk. A script to keep goes in `/sd/0/bin` (make the directory; it joins
   `/bin` the next time the namespace is built: after a reset, or the command `newns`).
4. "While it is not true that `$i` matches `0`": `~` tests a match, `!` turns success into failure and back. So:
   carry on until `$i` is 0.

**Challenge.** One way:

```text
#!/bin/rc
# countdown - count down from a number, a second at a time
i=$1
while(! ~ $i 0) {
	echo $i
	sleep 1
	i=`{calc $i - 1}
}
echo Lift off!
```

## Lab 7. The Tree of Files

**1.**

```session
/> cd /rom/lib/forth
/rom/lib/forth> cd ..
/rom/lib> cd ../songs
/rom/songs> cd /
```

**2.** `/` at the top; `rom` under it; `lib` and `songs` side by side under `rom`; `forth` under `lib`.

**3.** A directory's line starts with `d`. The only file there is `README`, of 1,955 bytes; the directories show
a length of 0.

```session
/> ls -l /rom
--r--r--r-- fx     1955 2000-01-01 00:00 README
d-r--r--r-- fx        0 2000-01-01 00:00 bin
d-r--r--r-- fx        0 2000-01-01 00:00 doc
d-r--r--r-- fx        0 2000-01-01 00:00 lib
d-r--r--r-- fx        0 2000-01-01 00:00 songs
```

**4.**

```session
/> df
disk  kind   size        free        label
0     sdhc   16380 KB    15956 KB    HYDRA-16
x     rom    623 KB      10 KB       ROM
r     ram    255 KB      252 KB      RAM
s     sram   511 KB      510 KB      SRAM
```

Disk `0` is the SD card, at `/sd/0`; `x` the ROM disk, at `/rom`; `r` the RAM disk, at `/ram`; `s` the shared RAM
disk, at `/sram`.

**5.** The date and time; a count of ticks, which is different each time; and 28 names (the manual's Hydra has
seven directories and twenty-one files in `/dev`).

**6.** Nothing; then `scom.zsm`; then nothing again. Nothing was copied: for a while, `/mnt` was another *name*
for the directory `/rom/songs`.

```session
/> ls /mnt
/> bind /rom/songs /mnt
/> ls /mnt
scom.zsm
/> unmount /mnt
/> ls /mnt
```

**7.** Four lines: so `/bin` is four directories seen as one.

```text
bind -c '#fr/2/bin' /bin
bind -a '#fs/bin' /bin
bind -a '#fx/bin' /bin
bind -a '#m/bin' /bin
```

They are the RAM disk's `bin` (where new files go: that's the `-c`), the shared RAM disk's, the ROM disk's, and
the programs in the paged ROM itself. A card with a `bin` directory adds a fifth.

**Questions.**

1. The list of directories that leads to a file from the root. `/ram/notes` starts at the root, so it means the
   same thing everywhere; `notes` starts from the current directory, so it depends on where you are.
2. `.` is the directory you're in; `..` is the one above it.
3. `/rom`, `/ram`, `/sram`, and `/sd/0` (and `/sd/1` and so on, for more cards).
4. Any two of the four above. The one that comes first in the union: the directories are searched in order.
5. No. A namespace belongs to a task (and the tasks it starts). Section 9.5 does the experiment: a `bind` made
   inside an `rc` isn't there after `exit`.

**Challenge.** Yes. `-a` adds the new directory *after* the ones already in the union, so it's searched last;
`-b` would put it *before*, so a script of yours with the same name as a command would win.

## Lab 8. Windows

**1.** A list of the window keys. Any key puts it away.

**2.** The screen clears, and a new shell says hello: a new window, in a new group. It's window 1, and the bar at
the top shows it.

**3.** <kbd>0</kbd>: window 0, just as it was left. <kbd>1</kbd>: the second window, with its line still there.
<kbd>W</kbd>: a list of the windows, each with its number, its group and the program running in it. <kbd>N</kbd>:
the next group's window.

**4.** Yes: `ps`, typed in the second window, lists `sleep`. A window isn't a program; it's a screen and a
keyboard for one. The programs in the windows you aren't looking at carry on.

**5.** The screen is split into two columns, one window in each. <kbd>CTRL</kbd>-<kbd>]</kbd> and an arrow key
moves the keyboard between them.

**6.** `columns` or `rows`: the editor in one, and a shell in the other.

**7.** In the window's heading, at the top.

**8.** The shell ends, and at once a new one starts in the same window: `init` sees to it. Window 0 never goes
away.

**Questions.**

1. Each window has its own screen, kept by the console even while it isn't shown, and its own shell. You choose
   which one the terminal shows, and which gets the keys.
2. A window is one screen with one program at its keyboard. A group is a set of windows that belong to one shell
   session, and can be shown together, tiled.
3. Press <kbd>CTRL</kbd>-<kbd>]</kbd> twice.
4. A file served by the console driver, which stands for this window's label. Likewise `/dev/cons`, the window's
   keyboard and screen; `/dev/consctl`, its settings; and `/dev/wctl`, its group's layout.

## Lab 9. A Command Hunt

**1.** `wc`; `head`; `tail`; `grep`; `sort`; `more`; `xd`; `cmp`; `df`; `top`; `slay`; `sleep`.

**2.**

```session
/> wc -l /sd/0/sample/as/hi.s
     84 /sd/0/sample/as/hi.s
/> tail -1 /sd/0/sample/as/hi.s
s_end:      .byte       ".", LF, 0
/> grep -c PRINT /sd/0/sample/as/hi.s
9
```

`head -3` shows its first three lines, which are comments; and `grep -n window` finds the word on lines 3, 8, 21,
64, 65, 73, 74, 75 and 83.

**3.**

<!-- session
echo pear >/ram/fruit
echo apple >>/ram/fruit
echo pear >>/ram/fruit
echo fig >>/ram/fruit
-->

```session
/> sort /ram/fruit
apple
fig
pear
pear
/> sort -r /ram/fruit
pear
pear
fig
apple
/> sort -u /ram/fruit
apple
fig
pear
/> wc -c /ram/fruit
     20 /ram/fruit
/> xd /ram/fruit
0000000  70 65 61 72 0a 61 70 70 6c 65 0a 70 65 61 72 0a  pear.apple.pear.
0000010  66 69 67 0a                                      fig.
```

(`sort /ram/fruit | uniq` does the same as `sort -u`.)

**4.** `p` is $70. Each line ends with $0A, the new-line character.

**5.** Square brackets mean "you can leave this out". `...` means "as many as you like". The pattern must always
be given.

**Questions.**

1. A command built into the shell. The current directory belongs to a task. A separate program would be a
   separate task: it would change its *own* directory, and then end, leaving the shell's where it was.
2. `wc`, `sort`, `grep`, `head` and `uniq` all do.
3. `kill` takes a task's number; `slay` takes a program's name, and stops every task running it.
4. Any three, with reasons. Expect `ls`, `cat`, `edit`, `cp` and `ps`.

**Challenge.** `d` and `m`, with five each (`date`, `db`, `df`, `dis`, `du`; and `mkdir`, `mkfs`, `mods`, `more`,
`mv`). One way, trying the likely letters:

```session
% for(l in c d m s t) echo $l `{ls /rom/bin/$l^* | wc -l}
c 4
d 5
m 5
s 4
t 4
```

The `^` matters: `$l*` would be read as a variable called `l*`, which is empty (rc says `null list in
concatenation`). `$l^*` joins the letter to the `*` (section 8.5).
