## **Text windows: screens in RAM banks, a whole VT100, window groups and their chrome**

A plan for the rebuilt system's (`reborn/`) console windows, October 2026.  It has three parts.  First, each window keeps its screen as cells in the console driver's RAM banks, not as its last 2K of output.  Second, each window becomes a complete VT100 (and VT102) terminal.  Third, on those two, a consistent text window system: windows in groups, Ctrl-Tab to move between a group's windows (Ctrl-] Tab where a terminal can't send Ctrl-Tab), and headers, footers and a bar that the user lays out.  It extends what's there: the plan's §14.2 windows (Plan 9's way, not job control) and the screen console (phase 8.3).  The screen editor (phase 9, `edit`) is its first big client.  The user answered the plan's questions on 2026-10-07 ([Decisions](#decisions)), and the work is on the branch `reborn-text-windows`.

### **Contents**
1. [What the user sees](#what-the-user-sees)
2. [What exists now](#what-exists-now)
3. [The design in one page](#the-design-in-one-page)
4. [Screens in the console's banks](#screens-in-the-consoles-banks)
5. [A whole VT100](#a-whole-vt100)
6. [Drawing the terminals: follow, else paint](#drawing-the-terminals-follow-else-paint)
7. [Windows, groups and the keys](#windows-groups-and-the-keys)
8. [Headers, footers and the bar](#headers-footers-and-the-bar)
9. [Sizes](#sizes)
10. [Scrollback, text and snarf](#scrollback-text-and-snarf)
11. [Layouts: tabs, tiles, popups](#layouts-tabs-tiles-popups)
12. [Two seats: the screen and the serial port](#two-seats-the-screen-and-the-serial-port)
13. [The files, and the names in each language](#the-files-and-the-names-in-each-language)
14. [The editor](#the-editor)
15. [Costs and budgets](#costs-and-budgets)
16. [The order of work](#the-order-of-work)
17. [Where this departs from the request](#where-this-departs-from-the-request)
18. [Decisions](#decisions)

---

### **What the user sees**

```
 0 forth  1 forth  2 edit!  3 top                                Tue 6 Oct 20:41
 2 edit: kdev.s                                       [2 kdev.s]  5 notes  6 rc
 ; ****************************************************************************
 ; kdev - the kernel's devices: #/ #p #e #| #t #m ...
 ...
 line 120/4410  col 17  changed                               ^G help  ^X exit
```

The top row is the bar: the groups, by the label of each one's focused window (`!` a bell in a group not shown), and the time.  The second is window 2's header: its number and label, then its group's windows (2, 5 and 6, with 2 focused).  The last is its footer: the editor's own status line.  The rows between are the editor's screen, 80 x 57 on the Vera X's 80 x 60.

* **Every window keeps its whole screen.**  Showing a window again paints it exactly as it was, whatever ran there: `top`, the editor, `db`, a shell.  Windows that aren't shown run on, as now.
* **A group is a set of windows that belong together**: a shell's, and the windows its programs open.  Ctrl-Tab goes to the group's next window and Ctrl-Shift-Tab to the previous.  Ctrl-] n and Ctrl-] p go to the next and previous group.  Ctrl-] c starts a new group with a shell, as it starts a window now.
* **The bar, a window's header and its footer each show a format that the user writes.**  A format can hold the window's number, its title, its program, the group's windows, the time, a program's own status line, and colours.  Programs set their window's title as they do on xterm (OSC 2), and their status line as they do on a VT320 (its host-writable status line), or through a file.  Each program can turn its window's chrome on or off, on the screen and on the serial port separately: by default the screen shows all of it and the serial port none.
* **Any program that writes ANSI (VT100) sequences runs in any window**: of any size, shown or hidden, tiled or not.  It's told when its window's size changes.

---

### **What exists now**

* **`cons`** (task F, `reborn/modules/cons`) serves `#c`: 4 windows, each with its own `cons` and `consctl`, line editor and history, raw mode, note group and keys.  Each also has its **text**: its last 2K of output, kept as a byte stream.  Ctrl-] and a digit shows a window, Ctrl-] n the next, Ctrl-] c asks for a new one (`wnew`, which wstart reads to start a shell there).
* **Showing a window replays its text**: `CSI r`, then the window's last 24 lines (the serial port) or 60 (the screen).  That works for a shell's scrolling output.  It fails for anything cursor-addressed: replaying `top`'s or the editor's last 2K draws a jumble.  And nothing can read back what a hidden window's screen looks like.
* **Output**: the shown window's text goes to the serial port paced (the writer waits for room in the send ring), and to `#v/term`, vid's ANSI terminal, from a place of its own in the text.  cons doesn't interpret what it passes on, but for LF (sent as CR LF) and BEL.
* **vid's terminal** has the subset the system's programs send (`reborn/docs/programming/video.md`): the cursor moves and CUP, ED and EL, SGR (bold shown bright, reverse, 16 colours), DECSC and DECRC, `?25`.  Since 188e313 (for the editor) it also has DECSTBM, IND, NEL and RI: a region's scroll copies rows in VRAM, some 1,500 cycles a row.  It has no insert or delete of lines or characters, tab stops, origin mode, character sets or reports.
* **The editor** (`programs/edit`, being built in another session) sends CUP, EL, ED 2, SGR 7 and 27, DECSTBM, LF and RI.  It takes its size from `$COLUMNS` x `$LINES` (through conio) and keeps its own shadow of the screen in a bank.  Its layout is a title line, a message line and two help lines.
* **Sizes**: conio, HyForth's `form` and hylang read `$COLUMNS` and `$LINES` (80 x 24 by default).  Nothing tells a program its window's size, or that it changed.
* **Room**: cons's code is about 6.7K of its 16K bank, and its RAM 13.1K of task F's 31K (8K of that is the four texts).  **It uses none of task F's RAM banks**: 32 on the emulator's two modules, 48 on the user's board with three.
* **The keys**: in raw mode cons decodes the terminal's sequences to one code each (`KEY_*`, `$80-$95`), and drops their modifiers.  Ctrl-Tab never reaches it: most terminals send a plain Tab for it, and Windows Terminal keeps it for its own tabs.

---

### **The design in one page**

```
  program --write--> cons: window N's VT engine --cells--> window N's screen: 3 of task F's banks
                                  |                                       |
                                  | follow: the output passed on,         | paint: the damage,
                                  v   moved to the window's place         v   from the cells
                     a back end for each terminal: the serial port (a VT102 or better), the screen (#v/term)

  keys: the serial port; later the input controller --> one decoder --> the console's keys (Ctrl-Tab, Ctrl-] ...)
                                                                     --> the focused window's queue
```

1. **A window is a virtual terminal.**  Its state (cursor, modes, margins, character sets, rendition, tab stops) is in cons's RAM.  Its screen is cells in three of task F's banks, and its scrollback is more rows in the same banks.
2. **The terminals show only what the console draws.**  While a terminal is up to date with the window it shows, the window's output is passed on as it's parsed (*follow*): moved to the window's place on the terminal, and turned into what that terminal can do.  Whatever can't be passed on becomes *damage*, painted from the cells (*paint*).  Showing another window, catching up after a pause, and a change of chrome are all paints.
3. **Windows, groups and layout are the console's state, reached through files** (`#cN/...`, Plan 9's way, with rio's names where rio has one).  The console's own keys (the prefix, Ctrl-Tab) are bindings the user can change.
4. **One name for each thing, in every language.**  A window's size comes from its `consctl`, its title from its `label`; C, HyForth, hylang and rc use the same words for both.

---

### **Screens in the console's banks**

**Whose banks: the console's, not each program's.**  The windows are cons's (task F's), and their screens go in its banks.  A program's own banks are the wrong place, for four reasons:
* A window outlives the programs that write it: a shell, then each command, then a pipeline's stages all write the same window.  Several tasks can write it at once.
* A task's banks are its program's, to use as it likes.  The kernel keeps only the bookkeeping of `BANKS_ALLOC`, and the editor's text, hylang's heap and HyForth's code all take banks.
* Reaching another task's banks from cons would cost a kcopy for every byte (35 cycles), outside the parser's reach.
* Task F's banks are idle and its own, and cons reaches them with a write to `$00`.

**A plane is a bank: 64 rows of 128 cells** (the shape of the VERA's text map, 128 x 64).  A window has three planes:
* characters;
* colours: the foreground in the low nibble, the background in the high;
* rendition flags: bold, dim, underline, blink, reverse, invisible, protected (DECSCA).

A cell's three bytes are at the same offset in the three banks: row *r*, column *c* is `$8000 + 128r + c`.

**The rows are a pool, and the screen is a map into it.**  The window's 64 physical rows are a pool.  Its screen is a map from screen rows to pool rows: 64 bytes, in RAM.
* **A scroll rotates the map.**  Every kind of scroll (the whole screen's, a region's under DECSTBM, IL, DL, IND, RI, SU, SD) moves the map's entries and blanks the one row that comes in: some 64 bytes moved and one row cleared, not the rows copied.
* **Scrollback costs nothing to keep.**  A row that scrolls off the top of the whole screen joins the scrollback as it is (its pool row).  The oldest scrollback row is taken, blanked, and becomes the new bottom row.  So a 24-row window has 40 rows of scrollback in its three banks, and `history N` adds three banks (64 rows) at a time.
* **The alternate screen** (`?1049`) takes its rows from the pool too, with three more banks when there aren't enough.

**Each row** also has, in RAM:
* its line attribute: single, double width, or double height (top or bottom);
* its damage on each terminal that shows it: the first and last column changed (a span).

**DEC graphics** (the VT100's line drawing) are character codes `$00-$1F` in the character plane (the 32 of DEC Special Graphics).  Nothing printable lands there, so such a cell is known to be a DEC graphic, and each back end maps it:
* the serial port sends `ESC ( 0` and the DEC character;
* the screen shows the font's glyph.  The console's ISO-8859-15 font gets the DEC set in its first 32 glyphs, which no printable byte reaches now.  cp437 has those shapes at its own codes, mapped through a 32-byte table that goes with the font.

**Colours**: there are 16, conio's 0-15 (the VERA's text has 16).  xterm's 256-colour and RGB SGRs are taken and mapped to the nearest of the 16.  The default colours (SGR 39 and 49) are the window's own, set by `colours FG BG` in its `wctl`.

**How many windows**: three banks a window, so task F's banks / 3, and 16 at most (numbers 0-f, a hex digit as the SPI devices' are).  That's 10 on the emulator's two modules and 16 on the board's three.  If that's too few, two windows of 32 rows or fewer could share three banks; that can come later, if it's wanted.

**RAM**: the four 2K texts go (8K).  Each window's state is about 120 bytes: the cursor and the saved cursor, the modes, the margins, G0-G3 and the shift state, the rendition, the tab stops (a 16-byte bitmap) and the row map (64).  At 16 windows, with their key queues, lines and histories, cons's RAM comes to about 16K of 31K.

**Two banks of code.**  The VT engine, the back ends and the chrome take cons past one 16K bank.  The first bank keeps the serial line, the keys, the line editor, `/pc` and srvlib; the second takes the rest (FAR2, as rc's and storage's second banks are reached).  A third bank can come if tiles and popups need it (ROM is plentiful).  One rule must change for this: `hyx2.inc` says a module of several banks owns no IRQ line, because its irq entry might find another bank at `$A000`.  cons owns two lines (the ACIA's and timer 2's), so **its irq entry and the ring routines it calls move into its RAM** (its DATA, where the FAR trampolines already are, about 300 bytes).  An interrupt then finds them whichever bank is in.  The IRQs-off time is unchanged: code runs as fast from RAM.

---

### **A whole VT100**

**Target: every control function of the VT100 and the VT102** (their user guides, chapter 3), in ANSI mode and in VT52 mode.  On top of those come the later functions that the system's programs and today's terminal programs use (from the VT220, the VT320 and xterm).

**The parser** is Paul Williams' DEC-compatible state machine (vt100.net's "A parser for DEC's ANSI-compatible video terminals"), with its states: ground, escape, CSI entry, parameter, intermediate and ignore, DCS passthrough and ignore, OSC string, and SOS/PM/APC string.  It handles the cases a simpler parser gets wrong:
* C0 controls act even in the middle of a sequence;
* CAN and SUB abort a sequence;
* a sequence that isn't known is consumed whole and dropped, so none of it reaches the screen.

In practice, that last point is most of what handling "all escape commands" means.

| Group | Functions | Notes |
| :---- | :-------- | :---- |
| C0 controls | NUL, ENQ, BEL, BS, HT, LF, VT, FF, CR, SO, SI, DC1/DC3, CAN, SUB, DEL | ENQ sends the window's answerback (`answerback TEXT` in its `wctl`).  BEL rings the bell; a hidden window's marks the bar with `!`.  VT and FF act as LF.  SUB aborts and shows the error glyph |
| Cursor | CUU, CUD, CUF, CUB, CUP, HVP, IND, NEL, RI, DECSC, DECRC | DECSC saves the cursor, rendition, character sets, origin mode and the wrap flag.  Also ECMA-48's CHA, VPA, HPA, CNL, CPL (the system's programs use G and d) |
| Editing | ED, EL; the VT102's IL, DL, DCH and IRM (insert mode); the VT220's ICH, ECH, DECSED, DECSEL and DECSCA; xterm's REP | |
| Margins and scrolling | DECSTBM, DECOM, DECAWM (with the VT100's last-column flag), SU, SD, DECSCLM | DECSCLM chooses smooth scroll (the writer waits for the terminal) or jump scroll (it doesn't) |
| Tabs | HTS, TBC (0 and 3), CHT, CBT | A stop every 8 columns to start with |
| Rendition | SGR 0, 1, 2, 4, 5, 7, 8, 22, 24, 25, 27, 28, 30-37, 39, 40-47, 49, 90-97, 100-107 | 38 and 48 (`;5;n`, `;2;r;g;b`) mapped to the 16 colours |
| Character sets | SCS for G0 and G1 (G2 and G3 too): `B` ASCII, `A` UK (`#` as £), `0` DEC Special Graphics, `1` and `2` (the alternate ROM, taken as `B` and `0`); SO, SI; the VT220's SS2, SS3, LS2, LS3 | |
| Lines | DECDHL (top, bottom), DECDWL, DECSWL, DECALN (the screen filled with E) | The serial port shows double lines as they are.  The screen (one 8 x 8 font, no scaling for a single row) shows them single width, with a space after each character so the columns still line up |
| Modes | DECCKM, DECANM (VT52), DECCOLM, DECSCLM, DECSCNM (reverse screen), DECOM, DECAWM, DECARM, DECINLM, LNM, IRM, KAM, DECTCEM (`?25`); DECKPAM, DECKPNM; RIS, DECSTR (soft reset) | DECCOLM: no 132 columns, so it does what xterm does without them: the screen cleared and the margins reset.  DECARM and DECINLM are kept and reported, with nothing to act on.  xterm's `?1049`, `?1047` and `?47` (the alternate screen) and `?2004` (bracketed paste, for snarf).  `?1000` and `?1006` (the mouse) come later |
| Reports | DA (as a VT102: `CSI ? 6 c`), secondary DA, DECID, DSR 5, DSR 6 (CPR, relative under DECOM), DECREQTPARM (DECREPTPARM), DECRQM, xterm's `CSI 18 t` (the window's size), ENQ | **The console answers these, never a terminal**: the answer goes into the window's keys, as a real terminal's answer would |
| Titles and status lines | OSC 0 and 2 (the window's label); the VT320's DECSSDT and DECSASD | The program's status line becomes the window's footer ([below](#headers-footers-and-the-bar)) |
| Kept, with nothing to act on | DECLL (the VT100's four LEDs: shown in the bar as `%L`), DECTST (no tests to run), MC (the VT102's printer: there's none, and the bytes of printer controller mode are dropped, not shown), DCS strings, DECSCUSR (the cursor's shape: the screen's cursor sprite) | Consumed whole, so nothing leaks onto the screen |
| VT52 mode | `ESC A B C D F G H I J K Y Z = > <` | `ESC Z` answers `ESC / Z` |

**Line feeds**: a write's LF still goes out as CR LF, the tty's `onlcr`, which a VT100 has as LNM.  A program that wants the VT100's bare LF can turn that off in its `consctl`.

**Keys, from a program's side.**  Raw mode gives `KEY_*` codes, as now (`keys hydra`).  `keys vt` in `consctl` gives what a VT100 would send instead, following DECCKM and DECKPAM: `ESC [ A` or `ESC O A`, and the keypad's application codes.  That's for programs brought from elsewhere and for terminal programs.  Cooked mode is as now.

---

### **Drawing the terminals: follow, else paint**

Each terminal (the serial port, the screen) has a back end with:
* its capabilities: what it can do;
* its own damage: rows and spans, in its own coordinates;
* a state: *follow* or *paint*.

**Follow.**  The terminal shows exactly what it should, so the window's output is passed on as it's parsed:
* a printable run is sent as it is;
* a control is sent;
* a sequence is sent again: as it was written when the window fills the terminal, or with its rows and columns moved to the window's place when it doesn't (its scrolling region kept inside the window's rows).

What the terminal can't do becomes damage instead, painted afterwards.  For example, vid has no IL or DCH, and a scroll in a window that has another beside it needs left and right margins.  The queries and modes that are the console's own are never passed on: DA, DSR, ENQ, DECCKM, the alternate screen, titles.

For a shell's output in a window that fills the terminal, follow sends the same bytes as today.  So the PC terminal's own scrollback and copying go on working, and the tests that read the serial port see what they see now.

**Paint.**  A terminal falls behind when:
* a window was just shown;
* the layout or the chrome changed;
* the line was `/ser`'s (xmodem);
* the chip was claimed (vid);
* (jump scroll) the window wrote faster than the line could carry.

The back end then sends its damage from the cells, one row's span at a time: CUP, any changes of SGR, the characters (DEC graphics between `ESC ( 0` and `ESC ( B`), and EL for a blank tail.  A scroll of a region that's whole and up to date on the terminal is sent as DECSTBM and LF or RI rather than as a repaint.  Then the cursor goes to its place, and the back end follows again.  Paint uses the smallest set: CUP, SGR, EL, ED, DECSTBM, IND, RI, SCS and DECTCEM.  Every VT100 has them, and so does vid's terminal now.

**The serial port's pacing stays.**  The back end fills the send ring as there's room, at each request's end (SRV_POST, where `pump` is now).
* **Smooth scroll** (the default): a shown window's writer waits for the ring, as now.
* **Jump scroll** (DECSCLM reset, or `scroll jump` in its `wctl`): its bytes go into the cells at once, and the terminal paints when it can, coalescing.  `cat` of a big file then ends in the time it takes to parse it, not to send it, and the scrollback has whatever the terminal skipped.

**The screen** has the same kind of back end, writing to `#v/term` (the call from one driver to another that 8.3 made).  vid gains two things:
* `ESC ( 0`, with the DEC glyphs in its font;
* while the chip is claimed, it answers E_BUSY instead of keeping 1K of text.  The console paints the screen at the release, since the cells have everything.

Painting a whole 80 x 60 screen through `#v/term` is estimated at 0.2-0.3 s: about 10K bytes, each kcopied and parsed.  If that's too slow for a window switch, there's a faster cell path: a shared segment (`SEG_CREATE`) holding the screen's cells in the VERA map's layout, which vid copies to VRAM when cons asks.  That's some 25 cycles a cell instead of about 110.  Measure first.

**Each terminal's capabilities can be set.**  The screen's are vid's.  The serial port's are a VT102's with ANSI colours by default (the PC tool's terminal has at least that).  `terminal vt100`, `vt102` or `xterm` in `consctl` changes them; with `xterm`, left and right margins too, so tiles side by side scroll in follow.

---

### **Windows, groups and the keys**

**A window** is as now: `#cN`, with its own `cons`, `consctl`, line editor, raw mode, note group and keys.  Now it also has its screen, size, label, header and footer.  Windows are numbered 0-15 (as the banks allow), and a window goes when its last `cons` closes (but window 0).

**A group** is a set of windows shown together, one at a time (tabs) by default, or tiled.
* **How windows join one.**  A window made by writing `new` to a window's `wctl` joins that window's group; `new group` makes a group for it.  Ctrl-] c (`wnew`, wstart's shell) makes a new group.  So a shell's group holds the windows that it and its programs make, and Ctrl-Tab goes round them.
* **When it goes**: with its last window.
* **Numbers**: groups have their own (0-15), shown in the bar.

**Starting a program in a new window**: `new-window [-g] [cmd ...]` makes a window in this group (with `-g`, in a new group).  It starts the command there, or the shell, with its fds and `$window`, as wstart does for Ctrl-] c.  Writing `new` to `wctl` answers like a Plan 9 clone file: a read on the fid that wrote it gives the new window's number.  (Plan 9's command is `window`, but here `window` is the languages' word for this window's number, so the command takes their `new-window`.)

**Focus**: one window has the keys: the focused window of the group shown.  Ctrl-C and Ctrl-\ go to its note group, as now.

**The keys**: these are the defaults; `key` lines in `wctl` change them.

| Keys | Do |
| :--- | :- |
| Ctrl-Tab, Ctrl-] Tab | The group's next window |
| Ctrl-Shift-Tab, Ctrl-] Shift-Tab | Its previous window |
| Ctrl-] n, Ctrl-] p | The next or previous group |
| Ctrl-] 0-9 | Window N (as now) |
| Ctrl-] c | A new group, with a shell (as now) |
| Ctrl-] w | The windows, as a list to choose from |
| Ctrl-] [, Shift-PgUp | The scrollback: view it, select, copy |
| Ctrl-] y | Paste the snarf buffer |
| Ctrl-] h, Scroll Lock (the keyboard's) | Hold: the window's writers wait (the VT100's No Scroll) |
| Ctrl-] x | Close: a hangup note (NOTE_HANGUP) to the window's note group, as rio's Delete does |
| Ctrl-] s, Ctrl-] v, Ctrl-] arrows, Ctrl-] z | Tile below or beside, move the focus, zoom ([tiles](#layouts-tabs-tiles-popups)) |
| Ctrl-] ? | The keys, in a popup |
| Ctrl-] Ctrl-] | A Ctrl-] (as now) |

**Ctrl-Tab has no byte of its own.**  A terminal sends it as xterm's `CSI 27;5;9~` (modifyOtherKeys) or as the newer `CSI 9;5u`, and Ctrl-Shift-Tab with 6 for 5.
* **cons's decoder learns both forms**, and the modifiers it drops now (`CSI 1;5A` is Ctrl-Up, and so on).
* **The input controller's firmware** (VIDEO.md step 6) should send its keys as the same sequences a PC terminal sends, so there's one decoder for both sources.
* **The PC tool** (hydrapc.js in Windows Terminal) is still to be tried.  Windows Terminal keeps Ctrl-Tab for its own tabs unless that binding is removed, and then sends a Tab.  The PC tool can ask for win32-input-mode (`CSI ?9001h`) and send `CSI 9;5u` itself.

Ctrl-] Tab works on every terminal.

**Programs with several windows** (the editor's files, say) need two key codes:
* **`KEY_FOCUS`**: when the focus moves within a group, each raw reader in the group gets `KEY_FOCUS` and the number of the window now focused.  So a program can follow the user to its other windows.
* **`KEY_RESIZE`**: when a window's size changes ([Sizes](#sizes)).

These are key codes rather than notes because a note a program doesn't catch ends it.

**Activity**: a hidden window that rings the bell, or that writes while `monitor` is on for it, is marked in the bar and in its group's list (`!` and `+`), as screen and tmux do.

---

### **Headers, footers and the bar**

* **The bar** is one row of the terminal's, at the top or the bottom (`bar top`, `bar bottom`, `bar off` in `wctl`).  It's console-wide: the groups, the time, whatever its format says.
* **A window's header and footer** are a row each, above and below its screen, set by `header FORMAT`, `footer FORMAT` and `header off` in its `wctl`.  A new window takes the defaults (`default header FORMAT`).  The program's screen is what's left ([Sizes](#sizes)).
* **Formats** (tmux's and screen's idea, kept short):

| Code | Shows |
| :--- | :---- |
| `%n`, `%g` | The window's number, its group's |
| `%l` | Its label: the title (OSC 2, or `#cN/label`).  With no label, the name of the program reading the window (cons knows its reader's task), as tmux's names are automatic |
| `%p` | The program reading the window (its task's name) |
| `%s` | Its status line (DECSASD, or `status` in its `wctl`) |
| `%w` | Its group's windows: the focused one reversed, `!` and `+` for activity |
| `%G` | The groups |
| `%c`, `%r` | Its columns, its rows |
| `%m` | Its modes: raw or cooked, `keys vt`, held, scrolled back |
| `%y` | Where the scrollback view is |
| `%t`, `%d` | The time, the date.  While a bar shows the clock it's drawn again each minute: timer 2 runs on in rounds, as it does for `/pc`'s replies |
| `%L` | The DECLL LEDs |
| `%=` | The rest of the row: what follows goes to the right |
| `%[...]` | Rendition, as SGR's numbers: `%[7]`, `%[1;33;44]`, `%[0]` |
| `%%` | A `%` |

* **The defaults** are in `/rom/lib/windows`, a file of `wctl` lines that init writes to `#c/wctl` at boot (as `/rom/lib/namespace` is run).  A card's `/lib` can override it through the union.  For example:

```
bar bottom
bar %[7] %G%=%t
default header %[1]%n %l%=%w
default footer %s
default chrome screen on
default chrome serial off
```

* **The program's status line** is the VT320's host-writable status line.  DECSSDT 2 (`CSI 2 $ ~`) turns it on; DECSASD 1 (`CSI 1 $ }`) sends the program's output there until DECSASD 0 (`CSI 0 $ }`).  The footer shows it through `%s`.  Scripts write `status TEXT` to `wctl` instead; it's the same state either way.
* **Chrome on each terminal, for each window.**  A window's chrome is on or off for each terminal separately, and its program can change it by writing to its `wctl`: `chrome screen on`, `chrome serial off`, or one part at a time (`chrome serial on header`).  While a window is shown, the bar follows its setting too, so a program can have the whole of either terminal.  By default the screen shows everything (the bar, the header, the footer) and the serial port nothing: `default chrome screen on` and `default chrome serial off` in `/rom/lib/windows`.
* **A label** that's written stays until it's written again.  Written empty, it's automatic again (the reading program's name).
* **Borders** (for tiles and popups) are drawn in DEC line drawing.  A tile's header is its border with the tile above.

---

### **Sizes**

* **A window's size** is its place in the layout, less its chrome on that terminal.  In tabs, that's the terminal's size less its bar, header and footer there; tiled, it's its tile's.  With both terminals showing it, it's the smaller of the two, each less its own chrome (by default, the serial port's 80 x 24 whole against the screen's 80 x 60 less three rows).  It's 128 x 64 at most (a plane).
* **`consctl` reads with `size C R`**: the one name for a window's size.  conio's `screensize`, HyForth's `form` and hylang's size word read it.  They read `$COLUMNS` and `$LINES` only when there's no console (a file, a pipe).  xterm's `CSI 18 t` and the CPR trick (`CSI 999;999 H`, then `CSI 6 n`) answer with it too.
* **A change** (the layout, a terminal's size, a header turned on) puts `KEY_RESIZE` into a raw reader's keys, as curses has it.  A cooked reader, such as a shell, reads the new size the next time it asks.  The line editor wraps at the window's width.
* **The terminals' sizes**:
  * the screen's comes from vid's mode (80 x 60, 80 x 30, 40 x 30);
  * the serial port's is 80 x 24 unless something sets it.  It can be set by hand (`terminal size C R` in `consctl`), asked of the terminal (`CSI 18 t`, at the first paint), or told by the PC tool: it sends its window's size whenever that changes (`CSI 8 ; R ; C t`, the form of xterm's answer, unasked), so resizing the PC's window resizes the Hydra's.

---

### **Scrollback, text and snarf**

* **Scrollback** is the pool's spare rows ([above](#screens-in-the-consoles-banks)), plus whatever `history N` adds.  Ctrl-] [ or Shift-PgUp shows it:
  * PgUp, PgDn, the arrows, Home and End move the view;
  * `/` finds text (later);
  * Space starts a selection, and Enter copies it to snarf;
  * Esc or `q` leaves.

  The program goes on meanwhile, its output going into the cells while the view stays put; the footer's `%y` says where the view is.
* **`#cN/text`** (rio's): the window's text, its scrollback and then its screen, a line for each row with trailing blanks dropped.  It's for scripts (to grep a window's output) and for tests.
* **`#c/snarf`** (rio's, at `/dev/snarf`): the console's one cut buffer, up to 8K, in a bank.  Ctrl-] y pastes it into the focused window as keys, as `kbdin` does: between `CSI 200~` and `CSI 201~` if the program asked for bracketed paste.  Programs read and write it, so the editor's cut and paste go through it and reach every window.

---

### **Layouts: tabs, tiles, popups**

* **Tabs**: one window of the group at a time.  This is what there is now, and it stays the default.
* **Tiles**: `layout rows`, `layout columns` or `layout grid` in `wctl` for the group, or Ctrl-] s and Ctrl-] v to split the focused window.  Ctrl-] and an arrow moves the focus; Ctrl-] z zooms one tile to the whole.  Each tile's header serves as its border.  The 80 x 60 screen has room for this; an 80 x 24 terminal, for two tiles at most.
* **Popups**: a window that floats over the layout (`float X Y C R` in its `wctl`), boxed, above the rest until it's closed.  The console uses them itself (the window list, the keys), and so can a program (a dialog in a window of its own).

---

### **Two seats: the screen and the serial port**

* **Now**, both terminals show the same thing (`screen`, `serial` or `both` in `consctl`).
* **With the input controller**, the screen and its keyboard become a computer of their own, beside the PC's terminal.  So each terminal can be a **seat**, with its own group shown, focus, size and keys: the input controller's keys go to the screen's seat, and the serial port's to its own.
* **`both` stays the default** (the user's choice): one seat on both terminals, at the smaller size; independent seats are a `consctl` setting.  A window shown on both seats is sized to the smaller.
* **The mouse** (the input controller's): a click focuses a window, and xterm's mouse reports (`?1000`, `?1006`) go to programs that ask for them.

---

### **The files, and the names in each language**

| File | Now | Planned |
| :--- | :-- | :------ |
| `#cN/cons` | The window's console | As now |
| `#cN/consctl` | `rawon`, `rawoff`, `group`, `screen`, `serial`, `both`; reads as the state | Adds `keys hydra` and `keys vt`, `terminal ...`; reads with `size C R` too |
| `#cN/wctl` | `new`, `current N` (console-wide); reads as the windows | Also the window's own, as rio's is: `new [group]`, `close`, `chrome`, `header`, `footer`, `status`, `history N`, `scroll smooth` or `jump`, `monitor`, `answerback`, `colours`, `float`, `layout`.  Console-wide: `current N`, `group N`, `bar`, `default`, `key`.  Reads as a line for each window: `N`, its group, its columns and rows, and `*` for the one shown (its label stays out, so a `*` in a title can't confuse a reader) |
| `#cN/label` | (new) | The window's title (rio's) |
| `#cN/text` | (new) | Its scrollback and screen, as text (rio's) |
| `#c/snarf` | (new) | The cut buffer (rio's) |
| `#c/wnew` | Ctrl-] c's window | As now (now a new group) |

**The words in each language** are proposals, under the review's rule of one name for each thing everywhere:

| | C (`hydra.h`, conio) | HyForth (`cons.fs`, Facility) | hylang (`cons.hl`, `screen.hl`) | rc |
| :- | :- | :- | :- | :- |
| The window's size | `screensize ()` | `form ( -- rows cols )` | `(window-size)`, as `{cols rows}` | `cat /dev/consctl` |
| Its title | `hy_wlabel (s)` | `window-label ( c-addr u -- )` | `(window-label s)` | `echo -n title >/dev/label` |
| Its status line | `hy_wstatus (s)` | `window-status ( c-addr u -- )` | `(window-status s)` | `echo status text >/dev/wctl` |
| A new window | `hy_wnew (flags)` | `new-window` (there now) | `(new-window)` (there now) | `new-window [-g] [cmd]` |
| Show one | | `show-window` (there now) | `(show-window n)` (there now) | `echo current 3 >/dev/wctl` |
| The new keys | `CH_RESIZE`, `CH_FOCUS` | `k-resize`, `k-focus` | (raw keys) | |
| Snarf | the file | the file | `(snarf)`, `(snarf! s)` | `cat /dev/snarf` |
| Any `wctl` line (`chrome serial on` ...) | `hy_wctl (s)` | `window-ctl ( c-addr u -- )` | `(window-ctl s)` | `echo chrome serial on >/dev/wctl` |

`new-window` in HyForth and hylang changes to match the command: the shell, or a command, run in the window it makes.  Today it makes a window that nothing reads.

---

### **The editor**

The screen editor (phase 9, `edit`, now on `reborn`) is this plan's first big client.  Nothing here needs it changed to keep working: it writes ANSI sequences, and the console models them.  It gains, step by step:
* **W1**: a window switched away from the editor and back shows it exactly.  Its DECSTBM scrolls become rotations of the row map.
* **W3**: its size comes from `consctl` (conio's `screensize`, which it already calls), and `KEY_RESIZE` tells it to redraw.  This is the one change it should make early: it reads the size once now.
* **W4**: its title line can become the window's label (OSC 2) and header, and its message line the status line (the footer).  That's two more rows of text and less to draw.
* **W6**: its cut buffer can go through `/dev/snarf`.
* **Its shadows** (a bank holding what's on the screen, so that only changes are sent) stay worthwhile on the serial line, since follow passes on what it writes.
* **Its several files become a window each**, in its group (the user's choice): Ctrl-Tab goes from file to file.  That needs W5's `KEY_FOCUS` (a raw read of any of its windows says where the focus went), and the editor's input reworked to read the focused window's keys.  It comes after W5, in the editor's code.

---

### **Costs and budgets**

These are estimates; W1's first piece of work is a spike that measures them against what cons does now.

| | Planned |
| :- | :------ |
| A printable byte into a window | About 90-120 cycles (parsing, three planes written, the damage noted): 30,000-40,000 bytes a second.  The serial line carries 11,500 a second at 115200 |
| A scroll of the whole screen, or of a region | About 3,000 cycles (the map rotated, a row blanked), whatever the rows |
| A window shown on the serial port, 80 x 24 | A paint of about 2-2.5K bytes: 0.2 s at 115200, as the replay takes now |
| A window shown on the screen, 80 x 60 | 0.2-0.3 s through `#v/term`; about 0.06 s by the shared cell path |
| IRQs off | Unchanged.  Nothing new runs with IRQs off.  The planes are task F's own banks, and `$00` is per task, so an interrupt doesn't disturb them |
| cons's ROM | Two banks: about 7K in the first, 12-14K in the second |
| cons's RAM | About 16K at 16 windows (13.1K now) |
| Task F's banks | 3 a window, plus history and alternate screens: 10 windows on two modules, 16 on three |

---

### **The order of work**

1. **W1, the screens and the VT core.**  cons becomes two banks, its irq entry in its RAM.  It gets:
   * the parser, the planes, the row map and the scrollback pool;
   * the VT100's cursor, erasing, scrolling, margins, rendition, modes, tabs and character sets, and the VT102's insert and delete;
   * the back ends' follow and paint, with smooth scroll;
   * `#cN/text`, and the texts gone;
   * in vid: `ESC ( 0`, the DEC glyphs in the font, and E_BUSY while claimed (the 1K kept goes).

   Tests: the cons, screen and vid tests as they are (follow keeps their bytes; a window's repaint is checked as a screen).  A new vt test of each function: bytes in, then `#cN/text` and the serial port's view out.  A JS model of a VT100 in the emulator (`sim/lib/vt.js`), so that tests can check what the PC's terminal shows.  And xterm.js's headless terminal (`@xterm/headless`, a test-only dependency) as a cross-check of the engine: the same bytes into both, the screens compared; a test skips it when it isn't installed.  The spike's measurements go in status.md.
2. **W2, the rest of the VT100**: the reports, VT52 mode, `keys vt` (DECCKM, DECKPAM), double width and height, DECALN, DECSCNM, DECLL, the alternate screen, OSC titles, and the VT220's and xterm's additions; jump scroll; hold.  Then vttest's checks.
3. **W3, sizes**: `consctl`'s size, `KEY_RESIZE`, and the four languages reading it; the line editor at the window's width; the serial terminal's size set, asked or told (the PC tool).
4. **W4, chrome**: labels, formats, the bar, headers and footers, the status line (DECSSDT and DECSASD, `status`), `/rom/lib/windows`, activity.
5. **W5, groups and keys**: up to 16 windows (as the banks allow), groups, `new` into the writer's group, `new-window`, the decoder's modifiers and Ctrl-Tab, the bindings, Ctrl-] w's list, closing with a hangup, `KEY_FOCUS`; Ctrl-Tab from the PC tool.  Then the editor's files as windows.
6. **W6, scrollback and snarf**: history banks, the view and selection, `/dev/snarf`, paste (bracketed).
7. **W7, tiles and popups.**
8. **W8, seats, the keyboard and the mouse**, with the input controller (VIDEO.md step 6).

Each language's words come with the step that brings their file (W3's size, W4's label and status, W5's windows, W6's snarf), with tests in each language's suite.  Each step also updates the documentation: `reborn/docs/using` (the keys, `new-window`), `programming/files.md` (`#c`), `video.md` (the screen as a back end), and status.md.

**What could go wrong**:
* Every byte a program writes costs cons two or three times what it does now, shown or hidden.  Jump scroll and hidden windows give the line's time back, and the spike measures the rest.
* Follow re-sends a sequence in a canonical form when it moves it, so the bytes can differ from today's in tiled or chromed windows.  Tests of those read the screen, not the bytes.
* The emulator's two modules allow 10 windows.
* Ctrl-Tab from the PC's terminal is uncertain (Ctrl-] Tab isn't).
* vid's E_BUSY change is in a file another session is changing now: arrange it with that session.

---

### **Where this departs from the request**

* **"The task's paged RAM"** is taken as the console's task (F), not each program's task.  [Screens](#screens-in-the-consoles-banks) gives the reasons: a window outlives its programs and has several writers, and programs' banks are their own.
* **Ctrl-Tab can't be the only key**: over the serial line most terminals can't send it.  Ctrl-] Tab does the same everywhere, and Ctrl-Tab works where it arrives (xterm's and CSI u forms, the input controller).
* **"Multiple windows per task"** is taken as a group per shell session, holding the windows it and its programs make.  The user confirmed that ([Decisions](#decisions)).
* **"All escape commands"**: all of the VT100's and VT102's are handled.  A few have nothing on the Hydra to act on and are consumed and reported, not acted on: 132 columns, interlace, auto-repeat, the confidence tests and the printer.  On the screen, double width and height are shown single width.
* **The command is `new-window`, not Plan 9's `window`**, because `window` is already HyForth's and hylang's word for this window's number.

---

### **As built: W1**

October 2026, on `reborn-text-windows` (reborn's `docs/status.md`, "The text windows", has the whole note and the measurements).  Where it went otherwise than planned:

* **A row's last cell is its meta**, so a window is **127 columns** at most (the plan said 128): where its blank end starts and in what colours, and its attributes.  Measuring showed why: blanking each row as it scrolled in (240 bytes over three banks) made a short line 6,783 cycles, slower than the serial line; with the blank end a byte, 1,033.  The whole screen's scroll turns the map, a ring (`v_rbase`), rather than moving its 63 entries.
* **The serial port is painted as a stream of lines**, a CR and an LF a row (not a CUP a row), and the cursor reached by the least move, so the PC's terminal keeps the rows in its own scrollback, and a paint reads as the replay did to the tests that read the line.  A row an autowrap continued is painted on from the full row before it, the terminal wrapping it.
* **Paints are whole** in W1: no damage by rows yet.  What vid can't do (insert and delete, SU, SD, REP) has the screen painted whole at the request's end.  vid's cursor is tracked exactly (a CUP when it isn't where a character goes), rather than lowering sequences one by one.
* **A paint goes on as requests come** (as the replay did): a client of the console waiting (a shell waiting for keys, a writer) brings it on as the send ring empties.
* **As xterm.js has them** (the oracle's): SU's rows don't go into the scrollback, and RIS clears it.  Where xterm.js differs by design, the VT100's is kept: SUB shows the error character, DECCOLM clears the screen.
* **Not yet** (W2): vid's `E_BUSY` while claimed (it still keeps 1K and shows it at the release), the DEC graphics in the font (ASCII on the screen meanwhile), double width and height (passed on to the serial port, not kept).

### **As built: W2a**

* **The DEC graphics** are the fonts' first 32 glyphs (both the console's: ISO-8859-15's, and `/lib/font/cp437`, whose smileys there no byte reached), not a mapping table: any font for the console keeps them there (`tools/decfont.js`).  vid's terminal takes the character sets; a read of `/term` gives them as ASCII.
* **vid's E_BUSY while claimed** is done: the 1K of kept text is gone, and the console paints the window again after the release (trying once a request meanwhile).
* **The answers** go to a queue of their own for each window (32 bytes), given to a raw reader first and as they came, so a program in `KEY_*` mode can read a CPR too; a cooked read drops them (a line editor would have taken them as typing).  `keys vt` (W2b) is still to come.

### **As built: W2b**

* **VT52 mode is translated, not passed on**: each of its sequences becomes the ANSI one that does the same, which the engine does and sends to the serial port, so the PC's terminal never leaves ANSI mode.
* **`keys vt`** follows DECCKM and VT52 mode; the keypad's application mode (DECKPAM) is kept but can't be acted on, as the PC's terminal is never put in it and sends its keypad as digits.  `keys vt` ends with the window's last `consctl`, as raw mode does.

### **As built: W2c**

* **The alternate screen isn't the PC terminal's**: switching paints both terminals from the window's cells instead of passing `?1049` on, so a window shown always gets its own screen, whatever the terminal's buffers hold.  It costs a paint (some 0.2 s on the serial port) as a full-screen program starts and ends.
* **The maps moved out of the window's state** (into `vw_maps`, through `vmap`), as a second map wouldn't fit its page.
* **Double width and height on the screen** show the characters a space apart (the VERA can't scale one row); on the serial port they're the terminal's own.

### **As built: W2d**

* **Jump scroll is the console's setting** (`consctl`'s `scroll jump`), not DECSCLM's: programs' resets send `?4l` (jump), and a window shouldn't start skipping its output for that.  The plan's `wctl` `scroll` became `consctl`'s, beside raw mode and `keys`, as it's the window's own and needs no group.
* **Hold** is Ctrl-] h; the keyboard's Scroll Lock comes with the input controller (W8).

### **As built: W3a**

* **The screen's size is read from vid's `ctl`** (`mode 80x60`), the documented state, not worked out from `term`'s length (rows x (columns + 1) doesn't say which is which).  It's read as `#v/term` is opened and when a write to it is refused; vid refuses the first write after the screen changed under the console (a `mode`, a `bitmap`, a `reset`, a claim's end), once, as it does while the chip's claimed.  So there's no polling: the change is seen at the console's next write, which a shell's prompt is.
* **The serial port's terminal's report is watched for, not taken out**: `ESC [ 8 ; R ; C t` goes to the window shown's keys as the rest do, and its key decoder drops it (a sequence that isn't a key).  Holding the bytes back till the sequence is known would hold back an Escape typed alone too.  The terminal is asked once, as the console starts (W3c), rather than at a first paint; `terminal size` alone asks again, and the PC tool tells unasked.
* **A resize keeps the cursor's row, as xterm does without reflow**: taller, the scrollback's newest rows come down first; shorter, the rows above the cursor's go into the scrollback only as must, and the bottom's rows are dropped.  The cells past a narrower width are dropped (in the scrollback too), not kept to come back: rows aren't reflowed.  The margins become the whole screen.
* **`KEY_RESIZE` is `keys hydra`'s**: a `keys vt` reader expects what a VT100 sends, which has no such key; it reads `consctl`.  One from before a `rawon` isn't given (it isn't news to a program that's just read the size).

### **As built: W3b**

* **The line editor finds its line's start from the window's cursor** at the line's first key (the prompt's been written by then), and keeps the terminal's cursor as a place in the line and whether it's past a row's last column, as a terminal is after writing there.  Its moves are then a row and a column apart, so nothing but the window's width is asked of vt.s.  Output from elsewhere into a window while its line is typed still confuses it, as it did.
* **A resize draws the line again** rather than working out where its cut rows went: up to its first row as it was laid out, its rest erased (ED), then written at the new width.  The prompt isn't the editor's, so a prompt cut by a narrower window stays cut.

### **As built: W3c**

* **Each language reads `consctl`'s `size` line**, under the plan's names: conio's `screensize`, HyForth's `form`, hylang's `(window-size)`; `$COLUMNS` and `$LINES` only when there's no `/dev/consctl`.  conio asks again after it gives a `CH_RESIZE`; `form` and `(window-size)` ask each time.  The editor redraws on `CH_RESIZE`, as on `^L`, at the new size.
* **The PC tool answers the console's ask itself** (and doesn't pass it on), so a PC terminal that doesn't answer `ESC [ 18 t` still gets the right size; it also tells the size unasked as its window changes.  `run.js -i` does as it does.

### **As built: W4a**

* **Formats are rendered in cons's first bank, drawn by its second**: vt.s asks for a row (`FAR1 chr_render`) at a terminal's width and writes its cells, so the renderer reaches the windows' state where it is.  Formats are 63 characters at most (a ctl write's length), the status line 127.
* **Groups wait for W5**: until then each window is its own group, so `%g` is `%n`, `%w` the window alone and `%G` every window.
* **The bar's default is at the top**, as the plan's picture has it (the plan's sample file said bottom).  `header on` and `off` act on both terminals; `chrome` names one.
* **`default` commands reach the windows still as the defaults were**, so `/lib/windows`, written after window 0 is made, still sets window 0's chrome.  A command that changes nothing paints nothing, so the ROM's file (the console's own defaults) costs no repaint at boot.
* **The time follows the console's use**: a server runs only for requests, so the minute's redraw comes with the next one (a shell at its prompt has one waiting, which timer 2's naps bring back every 2 seconds).  A long program that never touches the console leaves the clock as it was till it does.  A label is cleared by an empty line (`echo >/dev/label`): a write of no bytes never reaches the server.

### **As built: W4b**

* **The serial port with chrome keeps following byte for byte** where the chrome changes nothing, and translates only what moves rows: cursor moves become one CUP from the model's cursor, DECSTBM is sent offset, DECOM stays the console's, ED and DECSTR have the chrome drawn again after, RIS and DECALN repaint.  So a program's output costs about what it did, and the model, not the terminal, decides where things go.
* **Its chrome is drawn a cell at a time** as the send ring has room (a row rendered again as it goes on), in a paint or alone (the rows, then the terminal's state again), the window's writers waiting meanwhile as they do for a paint.
* **A redraw can come just after a prompt** (a status line or label changed by the command before it): the cursor goes back to the prompt, so a terminal shows it right; the tests' harness, which waits for output ending in a prompt, drives such steps from a script.

### **As built: W4c**

* **The words are the plan's**, each a write of a line: `hy_wlabel`, `window-label`, `(window-label s)` and `/dev/label`; `hy_wstatus`, `window-status`, `(window-status s)` and `wctl`'s `status`; `hy_wctl`, `window-ctl`, `(window-ctl s)` for any `wctl` line.  hylang's `(window-label)` with no argument reads the title back.

### **As built: W5a and W5b**

* **Sixteen windows** cost cons some 17K of RAM (to $6FD3 of the task's 32K): the histories (512 bytes a window), the VT states (a page), the maps, the formats and the status lines.  They could move to a bank if the RAM's wanted.
* **Ctrl-Tab is watched for, not taken out**, as the terminal's size report is: its bytes go to the window shown first (whose decoder drops them: they aren't keys), then the console acts, so the sequence ends in the window it started in.
* **Ctrl-] Shift-Tab** is Ctrl-] then the terminal's back-tab (`ESC [ Z`).  The irq entry's quick Ctrl-] digit takes `0`-`9` alone now (the note group of the window a Ctrl-C right after goes to).
* **`KEY_FOCUS` carries the window's number as the next key**, as two bytes of a raw read (`keys hydra`'s); a reader that turns raw after it doesn't get one from before.  The latest focus wins: a reader that didn't read meanwhile gets the window focused now.
* **`new`'s answer is one read**: the fid's next read gives "N" and an LF, its reads after that the windows again.

### **Decisions**

The user's answers to the plan's questions, 2026-10-07:

1. **Groups**: a group is a shell session.  Ctrl-] c starts one; windows made from a window (by its programs, or `new-window`) join that window's group, and each goes as its program closes it.
2. **Ctrl-Tab** (and Ctrl-] Tab) goes round the group's windows; Ctrl-] n and Ctrl-] p go between groups.
3. **Chrome**: each program can turn its window's chrome on or off, on the screen and on the serial port separately.  By default the screen gets everything and the serial port nothing ([Headers, footers and the bar](#headers-footers-and-the-bar)).
4. **The editor**: a window for each file, in its group, with Ctrl-Tab between them.  That comes after W5's `KEY_FOCUS`, in the editor's code ([The editor](#the-editor)).
5. **Seats**: the two terminals mirrored, as now, by default; independent seats are an option in `consctl`.
6. **Tests**: our own `sim/lib/vt.js`, plus xterm.js's headless terminal (`@xterm/headless`, MIT) as a test-only dependency that cross-checks the engine.  A test skips that check when it isn't installed.
7. **The order**: now, before 8.4.  W8 (the seats, the keyboard, the mouse) waits for 8.4's input controller.
