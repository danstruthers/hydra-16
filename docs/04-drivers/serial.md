# The serial console

The serial port is the Hydra's console: the terminal everything in this book is typed at.  Its driver runs in **task F**
and owns the ACIA and VIA timer 2.  There are two:

* **`ser`**, the base's (`base/modules/ser/ser.s`): the serial port as a console, as bare as one can be.
* **`cons`**, HydraOS's (`reborn/modules/cons/`): the same files and more: **windows**, each a whole console, a VT100
  screen kept for each, the Vera X's screen as a second terminal, and `/pc`, a folder on the PC over the same line.

Both are built on the same serial layer (`base/lib/serial.inc`: the rates, the send ring, timer 2's pacing), and
`ser`'s files are a part of `cons`'s, so a program written for the base runs on HydraOS unchanged.

## Contents

1. [The serial line](#the-serial-line)
2. [The console's files](#the-consoles-files)
3. [Reading a line: cooked mode](#reading-a-line-cooked-mode)
4. [Raw mode and keys](#raw-mode-and-keys)
5. [Notes from the keyboard](#notes-from-the-keyboard)
6. [Windows](#windows): groups, the keys, tiles, the scrollback, the cut buffer
7. [The chrome: the bar, headers and footers](#the-chrome-the-bar-headers-and-footers)
8. [A window's screen, and its size](#a-windows-screen-and-its-size)
9. [Two terminals: the serial port and the screen](#two-terminals-the-serial-port-and-the-screen)
10. [/pc: a folder on the PC](#pc-a-folder-on-the-pc)
11. [How it works](#how-it-works)

---

## The serial line

* **9600 baud, 8 data bits, no parity, 1 stop bit**, with RTS/CTS, as the system starts.  The rates are 300, 600, 1200,
  2400, 4800, 9600, 19200 and 115200 ([the ACIA](../02-hardware/devices.md#the-acia-65c51-and-the-serial-port)):
  `echo b115200 >/dev/serctl`, then set the terminal to match.  `cat /dev/serctl` says the rate and the flow control
  (`x1`, `x0`), then two lines of counts for chasing an unreliable line: `/pc`'s requests, and how many of their
  replies were cut short, unanswered or damaged, how many requests the PC had damaged (its NAK), and how often the PC
  was given up; and the ACIA's receive **overruns** (the system was late taking a byte: another came first) and
  **framing errors** (noise, or a rate the terminal doesn't match), and the bytes **dropped** because the receive ring
  was full (nothing was reading, and the terminal didn't stop).  The PC tool's Ctrl-A s has the PC's side: the
  frames from the Hydra that came damaged or stopped part-way, and the replies it was asked for again.  An
  `unanswered` here with a reply `asked for again` there is a reply that was lost or late on its way to the Hydra;
  with a frame `stopped part-way` there, a request cut short on its way to the PC.
* **Every byte sent is paced** by VIA timer 2: a character's time and a margin (two idle bits at 115200, one at the
  other rates), so the board's slightly slow clock and the MAX232's edges never lose a character.
* **Bytes received** go into a 256-byte ring as they come (the ACIA's interrupt).  A byte that arrives when the ring is
  full is dropped.
* **Any terminal** works: a VT100 or xterm emulation, PuTTY, Tera Term, `screen`, `minicom`.  The PC tool
  (`reborn/sim/tools/hydrapc.js`) is a terminal that also serves `/pc` ([below](#pc-a-folder-on-the-pc)).

---

## The console's files

`#c`, bound at `/dev`:

| File | `ser` | `cons` | What it is |
| :--- | :---: | :----: | :--------- |
| `cons` | yes | yes | The console: a read gets a line (or keys, raw), a write goes to the terminal |
| `consctl` | yes | yes | Its mode: `rawon`, `rawoff`, `group`; `cons` adds the key modes, scrolling, the terminals, the size |
| `ser` | yes | yes | The serial port, raw: bytes in and out as they are |
| `serctl` | yes | yes | The rate: `b300` ... `b115200`; the flow control: `x1`, `x0`.  Reads as both, and the counts (above) |
| `wctl` | | yes | The windows: new ones, which is shown, their chrome, layouts, the keys |
| `label` | | yes | The window's title |
| `wnew` | | yes | A read waits for Ctrl-] c, makes a window, and gives its number (init's) |
| `kbdin` | | yes | A write is typed into the window as keys |
| `kbin` | | yes | The console's keyboard: the Vera X's keys, written by `kdev` |
| `text` | | yes | The window's scrollback and screen, as text |
| `snarf` | | yes | The cut buffer, shared by all the windows |

In `cons`, each window has its own: `#c2/cons` is window 2's console, `#c/cons` window 0's.  A shell in window 2 has `#c2`
bound at `/dev`, so its `/dev/cons` is its own window's.

---

## Reading a line: cooked mode

A read of `cons` gets **a line**, edited at the console before the program sees it:

| Key | What it does |
| :-- | :----------- |
| Backspace, Delete | Rub out a character |
| Left, Right, Home, End (Ctrl-A, Ctrl-E) | Move in the line (`cons`) |
| Up, Down | The lines typed before (`cons`) |
| Ctrl-U | Rub out the line |
| Enter | Send the line (it ends with an LF; CR, LF or CR LF from the terminal are one end) |
| Ctrl-D | On an empty line: the end of the input (a read of 0) |

Each key is echoed as it's typed.  A line longer than the window's row wraps onto the next rows.  A write to `cons` goes
to the terminal, each LF as CR LF.

---

## Raw mode and keys

Writing **`rawon`** to `consctl` turns line editing off: a read gets each key as it comes, with no echo.  Raw mode lasts
until the last fd open on `consctl` closes (Plan 9's way), so a program opens `consctl`, writes `rawon`, and keeps it
open while it wants raw keys.  `rawoff` turns it off.

`cons` can give a raw reader's keys two ways (a `consctl` command):

* **`keys hydra`** (as it starts): the cursor and function keys as **one code each** (`KEY_UP`, `KEY_F1` ...: the
  constants are in `hydra.inc`), and `KEY_RESIZE` when the window's size changes.  An Escape alone is a key of its own
  once a moment passes with nothing after it.
* **`keys vt`**: the keys as a VT100 sends them, escape sequences and all, following the window's modes (cursor keys,
  keypad, VT52).  For programs that parse them themselves.
* **`keys mods`**: as `hydra`, plus a key the terminal sent with modifiers as `KEY_MOD`, the modifiers (1 Shift, 2 Alt,
  4 Ctrl), then the key.

The window's screen answers a program's queries into its keys, as a VT100 does (where the cursor is, what the terminal
is, its size: `CSI 18 t`).  With **`?1000`** set (and `?1006`), the mouse's presses and releases come as xterm's reports
too, at the window's cells.

---

## Notes from the keyboard

In either mode, at any time:

* **Ctrl-C** sends the `interrupt` note to the shown window's note group: the program running stops, unless it catches it.
* **Ctrl-\\** sends `kill`, which nothing catches.

`consctl`'s **`group`** makes the window's notes go to the writer's note group: a shell does this for each command it
runs in the foreground.

---

## Windows

`cons` gives the one terminal **several consoles**: windows, Plan 9's way (rio's, on a text terminal), not Unix's job
control.  Each window is a whole console with its own `cons`, line editor, raw mode, note group and screen.  One window
is **shown**, and gets the keys; the others run on, their output kept on their screens, their reads waiting for keys.
Showing a window paints it on the terminal exactly as it was.

* **Up to 16 windows**, as the console's RAM banks allow.  **Window 0** is init's, and the login shell's.  A window goes away when its program ends (its last `cons` closes),
  except window 0.
* **Groups**: each window belongs to a group, a shell session.  Ctrl-] c makes a new group with a shell in it; a window
  made by a program (`new-window`) joins that program's group.  Each group remembers the window it last showed.
* **`new-window [-g] [command]`** makes a window and shows it, running the command (or a shell); `-g` puts it in a group
  of its own.

### The keys

Every window key is **Ctrl-]** and then a key:

| Keys | What they do |
| :--- | :----------- |
| Ctrl-] 0 ... 9 | Show that window (windows 10 and up: from the list, Ctrl-] w) |
| Ctrl-] c | A new group, with a shell |
| Ctrl-] n, Ctrl-] p | The next or previous group |
| Ctrl-] Tab, Ctrl-Tab | The group's next window; Ctrl-] Shift-Tab, Ctrl-Shift-Tab, the previous |
| Ctrl-] and an arrow | The group's previous (up, left) or next (down, right) window |
| Ctrl-] w | The list of windows: a window's number, or the arrows and Enter, shows it |
| Ctrl-] s, Ctrl-] v | Split: a new window with a shell, tiled below or beside |
| Ctrl-] z | Zoom: a tiled group's focus alone, or tiled again |
| Ctrl-] \[, Shift-PgUp | The scrollback ([below](#the-scrollback-and-the-cut-buffer)) |
| Ctrl-] y | Paste the cut buffer |
| Ctrl-] h | Hold the window's output (the VT100's No Scroll), until Ctrl-] h again |
| Ctrl-] x | A hangup note to the window's group |
| Ctrl-] ? | The keys, in a popup |
| Ctrl-] Ctrl-] | A Ctrl-] typed into the window |

They're **bindings**: `wctl`'s `key` lines change them (`key prefix ctrl-a` makes Ctrl-A the prefix; `key t new` makes
Ctrl-] t a new group).  Init writes `/lib/windows`'s lines to `wctl` as it starts, and a card's `/lib/windows` comes
first, so a card can carry your own.

### Tiles and floating windows

A group's windows are shown one at a time (`layout tabs`), or **tiled** together: `echo layout rows >/dev/wctl` (or
`columns`, `grid`).  A window can **float** over its group's in a box: `echo float 40 10 >/dev/wctl` (40 x 10, centred),
`float X Y C R` at a place, `float off`.

### The scrollback and the cut buffer

Each window keeps rows past its screen: its **scrollback**, 64 by default (`wctl`'s `history N`, up to 128).
**Ctrl-] [** (or Shift-PgUp) shows it, while the program goes on: the arrows, PgUp, PgDn, Home and End move; Space marks
a line, and the lines from it to the cursor are the selection; Enter copies them to **`/dev/snarf`**, the cut buffer
(rio's), and leaves; `q`, or Escape twice, leaves.  **Ctrl-] y** pastes the cut buffer into the window shown as its keys,
between bracketed-paste marks for a program that asked (`?2004`).  `/dev/snarf` is a file: `cat /dev/snarf`, or `echo
text >/dev/snarf` to replace it (8K at most).

`/dev/text` reads the window's scrollback and screen as text, a line a row.

---

## The chrome: the bar, headers and footers

A terminal can show **chrome** around the window: the **bar** (a row for the whole console, at the top or bottom: the
groups, the time), and the window's **header** and **footer** (a row each, above and below it).  By default they're on
the screen (with a Vera X), and off on the serial port.

Each is drawn from a **format**: `%n` the window's number, `%l` its label, `%p` its program, `%s` its status line, `%w`
the group's windows, `%G` the groups, `%c` `%r` its size, `%m` its modes, `%t` `%d` the time and date, `%L` the keyboard's
LEDs, `%y` the scrollback's place, `%=` the rest at the right, `%[1;33]` SGR's rendition, `%%` a `%`.  The defaults,
`/lib/windows`:

```
bar top
bar %[7] %G%=%t
default header %[1]%n %l%=%w
default footer %s
default chrome screen on
default chrome serial off
```

`wctl` takes the same lines, for the window that writes them: `echo chrome serial on >/dev/wctl` shows it on the serial
port too; `echo header %n %p >/dev/wctl`; `echo status compiling >/dev/wctl` sets the footer's `%s`.  The window's
**label** is `/dev/label` (`echo build >/dev/label`), which OSC 0 and 2 set too; empty, it's the program's name.

---

## A window's screen, and its size

Each window's screen is kept in the console driver's RAM banks, written by a **VT100**: the VT100's and VT102's
sequences and reports, VT52 mode, the alternate screen, double width and height, the character sets.  So a full-screen
program (`edit`, `top`, `more`) works in any window, and a window shown again is painted exactly as it was.

**A window's size** is the smaller of the terminals it's shown on, less their chrome: 80 x 24 on a serial terminal
until it says otherwise; 80 x 57 on the Vera X's screen alone.  `consctl` reads with a line `size C R`; a raw reader with
`keys hydra` gets `KEY_RESIZE` when it changes.  The serial terminal's size is asked as the console starts (`CSI 18 t`)
and can be set: `echo terminal size 132 43 >/dev/consctl`.  The PC tool tells the Hydra its window's size as it changes.

---

## Two terminals: the serial port and the screen

With a Vera X, the console has a second terminal, the screen (`vid`'s `#v/term`), and keys from its keyboard
(`#c/kbin`, written by `kdev`).  `consctl` chooses how they're used:

| Command | What it does |
| :------ | :----------- |
| `both` | The window shown on both (as it starts) |
| `screen`, `serial` | The windows on one alone.  With `screen` alone, output isn't held back by the serial line |
| `seats` | Both on, each a **seat** of its own: its own window shown, its own keys (the keyboard's are the screen's), its own Ctrl-C.  A group's windows are sized to the terminals showing it |

`cat /dev/consctl` shows which (`terminal both`).

---

## /pc: a folder on the PC

`/pc` is a folder on the PC, served over the **same serial line** by the PC tool, so files go between the Hydra and the
PC with `cp`:

```
$ node sim/tools/hydrapc.js COM3 work               # the PC: a terminal, and the folder work as /pc
/> ls /pc
/> cp /pc/hello.hyx /ram/bin/hello
```

* The PC tool needs Node's `serialport` package (`npm install` in `reborn/sim`); `--list` shows the PC's serial
  ports, `--baud N` sets the rate, `--read-only` keeps the folder from being changed.  Its own keys are Ctrl-A and a
  key: Ctrl-A x quits.
* The PC tool is the Hydra's terminal too; its requests go in **frames** between the console's bytes, with a CRC, so a
  byte lost on the line means a frame sent again, never a wrong file.
* One request at a time; with no PC tool on the line, a request fails (`E_IO`) after a moment.
* The emulator plays the PC tool's part with `--pc-dir DIR` ([the emulator](../08-emulator.md)).
* While a program has `/dev/ser` open for reading (`xmodem`), the line is its: `/pc` and the windows' output wait.

`xmodem -r file` and `xmodem -s file` move files with any terminal program instead ([the tools](../07-tools.md)).

---

## How it works

* **Receiving**: the ACIA's interrupt puts each byte into the receive ring and adds 1 to the driver's event count, so
  the clients waiting look again.  Before each request, the driver hands the keys to the windows' queues, acting on
  Ctrl-] and the key after it there.  Ctrl-C and Ctrl-\\ are sent as notes from the interrupt entry itself
  (`NOTE_QUEUE`), so they work even when the system's busy.
* **Sending**: the shown window's output goes into a send ring as there's room; VIA timer 2 runs a character's time,
  and its interrupt sends the next byte.  The interrupts' work is a few dozen cycles each.
* **The screens**: `vt.s` (the driver's second bank) is the VT100: it writes each window's cells into the driver's RAM
  banks (a pool of 64 rows of 128 cells, three planes: characters, attributes, colours), and the terminal follows the
  window shown as its output is parsed.  Showing another window paints it from its cells.
* **The irq entry is in RAM**: `cons` is two banks, so its interrupt entry (and what that calls) lives in its RAM, as
  the rule for a module of several banks says ([assembly](../09-programming/assembly.md)).
* **`ser`** is the same without windows, screens or `/pc`: a receive ring, a send ring, the line editor, and the four
  files.
