# Appendix A. HyForth Reference

This appendix lists every word in HyForth, library by library. HyForth follows the Forth 2012 standard; the words of
the standard's word sets mean what the standard says, and the Hydra's own words are described in full.

## A.1 How to read the entries

Each word has a **stack diagram**: what it takes, `--`, and what it leaves, with the top of the stack on the right.

| Abbreviation | Is |
|---|---|
| *n* | A signed number: one cell, -32,768 to 32,767 |
| *u* | An unsigned number: 0 to 65,535 |
| *x* | Any cell |
| *flag* | A flag: 0 false, -1 true (any value but 0 counts as true) |
| *char*, *c* | A character's code |
| *addr*, *c-addr* | An address; the address of characters |
| *c-addr u* | A string: its address and its length |
| *d*, *ud* | A double-cell number (32 bits), signed and unsigned |
| *xt* | An **execution token**: a word, as a value (`'` gives one) |
| *nt*, *wid* | A word's name token; a word list |
| *fileid*, *fam*, *ior* | A file's id; a file access method; an I/O result (0, or an error) |
| *"name"* | A word taken from the input, after this one |
| `N:` | The number stack (the numbers library's) |
| `R:` | The return stack |

**(I)** marks an **immediate** word, which works while a definition is being compiled; **(C)** a word that can only
be used inside a definition. Words load with their library: the core is always there; `coreext`, `exception`,
`file`, `tools` and (at the shell) `shell` load as HyForth starts; the rest with `lib` *name*.

## A.2 The core

**The stack**

| Word | Stack | Does |
|---|---|---|
| `dup` | ( *x* -- *x x* ) | Copies the top |
| `?dup` | ( *x* -- 0 \| *x x* ) | Copies the top if it isn't 0 |
| `drop` | ( *x* -- ) | Drops the top |
| `swap` | ( *x1 x2* -- *x2 x1* ) | Swaps the top two |
| `over` | ( *x1 x2* -- *x1 x2 x1* ) | Copies the second |
| `rot` | ( *x1 x2 x3* -- *x2 x3 x1* ) | Brings the third to the top |
| `2dup`, `2drop`, `2swap`, `2over` | | The same, for pairs of cells |
| `depth` | ( -- *u* ) | How many cells are on the stack |
| `>r`, `r>`, `r@` (C) | ( *x* -- ) R: ( -- *x* ), ( -- *x* ) R: ( *x* -- ), ( -- *x* ) | To, from, a copy of, the return stack |

**Arithmetic**

| Word | Stack | Does |
|---|---|---|
| `+`, `-`, `*` | ( *n1 n2* -- *n3* ) | Add, subtract, multiply |
| `/`, `mod`, `/mod` | ( *n1 n2* -- *n3* ), ( -- *rem* ), ( -- *rem quot* ) | Divide (rounding toward zero), the remainder, both |
| `*/`, `*/mod` | ( *n1 n2 n3* -- *n4* ), ( -- *rem quot* ) | *n1* × *n2* ÷ *n3*, with a 32-bit product in between |
| `1+`, `1-`, `2*`, `2/` | ( *n1* -- *n2* ) | Add 1, take 1, double, halve |
| `negate`, `abs` | ( *n1* -- *n2* ) | Change the sign; the size |
| `min`, `max` | ( *n1 n2* -- *n3* ) | The smaller; the larger |
| `um*`, `m*` | ( *u1 u2* -- *ud* ), ( *n1 n2* -- *d* ) | A double-cell product |
| `um/mod`, `fm/mod`, `sm/rem` | ( *ud u1* -- *u2 u3* ), ( *d n1* -- *n2 n3* ) | Divide a double by a cell: unsigned, floored, symmetric |
| `s>d` | ( *n* -- *d* ) | A cell to a double |

**Logic and comparisons**

| Word | Stack | Does |
|---|---|---|
| `and`, `or`, `xor` | ( *x1 x2* -- *x3* ) | Bit by bit |
| `invert` | ( *x1* -- *x2* ) | Every bit flipped |
| `lshift`, `rshift` | ( *x1 u* -- *x2* ) | Shift the bits *u* places left, right |
| `=`, `<`, `>`, `u<` | ( *x1 x2* -- *flag* ) | Equal; less, more (signed); less (unsigned) |
| `0=`, `0<` | ( *x* -- *flag* ) | Zero; below zero |

**Memory**

| Word | Stack | Does |
|---|---|---|
| `@`, `!` | ( *addr* -- *x* ), ( *x addr* -- ) | Fetch, store a cell |
| `c@`, `c!` | ( *addr* -- *char* ), ( *char addr* -- ) | Fetch, store a byte |
| `2@`, `2!` | ( *addr* -- *x1 x2* ), ( *x1 x2 addr* -- ) | Fetch, store two cells |
| `+!` | ( *n addr* -- ) | Add to a cell |
| `move` | ( *addr1 addr2 u* -- ) | Copy *u* bytes |
| `fill` | ( *c-addr u char* -- ) | Fill with a byte |
| `count` | ( *c-addr1* -- *c-addr2 u* ) | A counted string (its length first) to a string |
| `cells`, `cell+`, `chars`, `char+` | ( *n1* -- *n2* ) | Sizes: a cell is 2 bytes, a character 1 |
| `align`, `aligned` | ( -- ), ( *addr* -- *addr* ) | (Nothing on the 65C02: any address is aligned) |
| `here` | ( -- *addr* ) | The dictionary's next free byte |
| `allot` | ( *n* -- ) | Reserve *n* bytes there |
| `,`, `c,` | ( *x* -- ), ( *char* -- ) | Put a cell, a byte, there |

**Text and numbers**

| Word | Stack | Does |
|---|---|---|
| `emit` | ( *char* -- ) | Print a character |
| `type` | ( *c-addr u* -- ) | Print a string |
| `cr`, `space`, `spaces` | ( -- ), ( -- ), ( *n* -- ) | A new line; a space; *n* spaces |
| `.`, `u.` | ( *n* -- ), ( *u* -- ) | Print a number, and a space |
| `."` *text*`"` (I C) | ( -- ) | Print the text |
| `s"` *text*`"` (I) | ( -- *c-addr u* ) | A string |
| `key` | ( -- *char* ) | Wait for a key |
| `accept` | ( *c-addr n1* -- *n2* ) | Read a line into a buffer |
| `char`, `[char]` (I) | ( "name" -- *char* ) | The first character of the next word |
| `bl` | ( -- *char* ) | A space's code (32) |
| `<#`, `#`, `#s`, `hold`, `sign`, `#>` | | Pictured numbers: build a number's text, digit by digit |
| `>number` | ( *ud1 c-addr1 u1* -- *ud2 c-addr2 u2* ) | Read digits into a double |
| `base` | ( -- *addr* ) | The variable holding the base |
| `decimal` | ( -- ) | Base 10 |
| `(` | ( "text)" -- ) | A comment, to `)` |
| `\` | ( "text" -- ) | A comment, to the end of the line |
| `word`, `source`, `>in` | | The input: a word of it; the line; the place in it |

**Definitions and control**

| Word | Stack | Does |
|---|---|---|
| `:` | ( "name" -- ) | Begin a definition |
| `;` (I) | ( -- ) | End it |
| `immediate` | ( -- ) | Make the last definition immediate |
| `recurse` (I C) | | Call the definition being made |
| `[`, `]` | ( -- ) | Stop compiling; start again |
| `literal` (I C) | ( *x* -- ) | Compile a number |
| `'`, `[']` (I) | ( "name" -- *xt* ) | A word's execution token |
| `execute` | ( *xt* -- ) | Run a word |
| `postpone` (I C) | ( "name" -- ) | Compile a word's compiling behaviour |
| `if` ... `else` ... `then` (I C) | ( *flag* -- ) | Decisions |
| `begin` ... `until` (I C) | ( *flag* -- ) | Loop until true |
| `begin` ... `while` ... `repeat` (I C) | ( *flag* -- ) | Loop while true |
| `do` ... `loop`, `+loop` (I C) | ( *limit start* -- ), ( *n* -- ) | Counted loops |
| `i`, `j` (C) | ( -- *n* ) | The loop's count; the outer loop's |
| `leave`, `unloop` (C) | ( -- ) | Leave the loop; drop its count (before `exit`) |
| `exit` (C) | ( -- ) | Return from the definition |
| `create` | ( "name" -- ) | A word that gives the address of what follows |
| `does>` (I C) | | What a `create`d word does |
| `>body` | ( *xt* -- *addr* ) | A `create`d word's data |
| `variable`, `constant` | ( "name" -- ), ( *x* "name" -- ) | A variable; a constant |
| `state` | ( -- *addr* ) | True while compiling |
| `find` | ( *c-addr* -- *c-addr* 0 \| *xt* 1 \| *xt* -1 ) | Look a word up |
| `evaluate` | ( *c-addr u* -- ) | Run a string as Forth |
| `quit`, `abort`, `abort"` | | Back to the prompt; with a message if a flag is true |
| `bye` | ( -- ) | Leave HyForth |
| `environment?` | ( *c-addr u* -- false \| *x true* ) | Ask about the system |
| `included`, `include` | ( *c-addr u* -- ), ( "file" -- ) | Run a file of Forth |
| `required`, `require` | ( *c-addr u* -- ), ( "file" -- ) | The same, once only |

## A.3 Core Extension (`coreext`)

| Word | Stack | Does |
|---|---|---|
| `nip`, `tuck` | ( *x1 x2* -- *x2* ), ( *x1 x2* -- *x2 x1 x2* ) | Drop the second; copy the top under the second |
| `pick` | ( *xu ... x0 u* -- *xu ... x0 xu* ) | Copy the *u*th cell to the top |
| `roll` | ( *xu ... x0 u* -- *xu-1 ... x0 xu* ) | Move the *u*th cell to the top |
| `<>`, `u>`, `0<>`, `0>` | ( ... -- *flag* ) | Not equal; more (unsigned); not zero; above zero |
| `within` | ( *n lo hi* -- *flag* ) | *lo* ≤ *n* < *hi* |
| `true`, `false` | ( -- *flag* ) | -1, 0 |
| `2>r`, `2r>`, `2r@` (C) | | Pairs to and from the return stack |
| `erase` | ( *addr u* -- ) | Fill with zeros |
| `pad` | ( -- *c-addr* ) | A scratch buffer |
| `unused` | ( -- *u* ) | The dictionary's free bytes |
| `hex` | ( -- ) | Base 16 |
| `:noname` | ( -- *xt* ) | A definition without a name |
| `compile,` | ( *xt* -- ) | Compile a word |
| `[compile]` (I) | ( "name" -- ) | Compile an immediate word |
| `again` (I C) | | `begin` ... `again`: loop for ever |
| `?do` (I C) | ( *limit start* -- ) | `do`, but skipped if they're equal |
| `case`, `of`, `endof`, `endcase` (I C) | | Choose by value |
| `value`, `to` | ( *x* "name" -- ), ( *x* "name" -- ) | A value; change it |
| `buffer:` | ( *u* "name" -- ) | A named buffer of *u* bytes |
| `defer`, `is`, `defer!`, `defer@`, `action-of` | | A word whose action is set later |
| `marker` | ( "name" -- ) | *name* forgets everything defined after it |
| `parse`, `parse-name` | ( *char* "text" -- *c-addr u* ), ( "name" -- *c-addr u* ) | Take text from the input |
| `refill`, `source-id`, `save-input`, `restore-input` | | The input source |
| `.(` (I) | ( "text)" -- ) | Print the text now |
| `.r`, `u.r` | ( *n w* -- ), ( *u w* -- ) | Print a number right-aligned in *w* characters |
| `holds` | ( *c-addr u* -- ) | Pictured numbers: put a string in |
| `c"` (I C) | ( "text"" -- *c-addr* ) | A counted string |
| `s\"` (I) | ( "text"" -- *c-addr u* ) | A string with escapes: `\n`, `\t`, `\"` ... |

## A.4 Exception (`exception`)

| Word | Stack | Does |
|---|---|---|
| `catch` | ( *i*x xt* -- *j*x* 0 \| *i*x n* ) | Run *xt*; 0 if it worked, or the error it threw |
| `throw` | ( *k*x n* -- *k*x* \| *i*x n* ) | Throw error *n* (nothing if it's 0) |

## A.5 File Access (`file`)

| Word | Stack | Does |
|---|---|---|
| `r/o`, `w/o`, `r/w` | ( -- *fam* ) | Read only, write only, both |
| `bin` | ( *fam1* -- *fam2* ) | Bytes, as they are |
| `open-file`, `create-file` | ( *c-addr u fam* -- *fileid ior* ) | Open; make (or empty) and open |
| `close-file` | ( *fileid* -- *ior* ) | Close |
| `read-file` | ( *c-addr u1 fileid* -- *u2 ior* ) | Read bytes |
| `read-line` | ( *c-addr u1 fileid* -- *u2 flag ior* ) | Read a line (*flag* false at the end) |
| `write-file`, `write-line` | ( *c-addr u fileid* -- *ior* ) | Write bytes; a line |
| `file-position`, `reposition-file` | ( *fileid* -- *ud ior* ), ( *ud fileid* -- *ior* ) | The place in the file |
| `file-size`, `resize-file` | ( *fileid* -- *ud ior* ), ( *ud fileid* -- *ior* ) | Its length |
| `file-status` | ( *c-addr u* -- *x ior* ) | Is it there? |
| `flush-file` | ( *fileid* -- *ior* ) | Write what's waiting |
| `delete-file`, `rename-file` | ( *c-addr u* -- *ior* ), ( *c-addr1 u1 c-addr2 u2* -- *ior* ) | Remove; rename |
| `include-file` | ( *fileid* -- ) | Run an open file as Forth |

An ior from HyForth's file words is 0, or -512 less the system's error code (Appendix G), so `throw` reports it.

## A.6 Programming-Tools (`tools`)

| Word | Stack | Does |
|---|---|---|
| `.s` | ( -- ) | Show the stack |
| `?` | ( *addr* -- ) | Print the cell at *addr* |
| `dump` | ( *addr u* -- ) | Show memory, 8 bytes a line, in hex and as text |
| `see` | ( "name" -- ) | Show a word's definition |
| `words` | ( -- ) | List the words (the newest first), with their addresses and kinds |
| `libs` | ( -- ) | List the libraries loaded |
| `lib` | ( "name" -- ) | Load a library (*name*`.fl`, or *name*`.fs`), or show its words again |
| `-lib` | ( "name" -- ) | Hide a library's words |
| `[if]`, `[else]`, `[then]` (I) | ( *flag* -- ) | Choose which lines to compile |
| `[defined]`, `[undefined]` (I) | ( "name" -- *flag* ) | Is a word there? |
| `ahead`, `cs-pick`, `cs-roll` (I C) | | For building control words |
| `n>r`, `nr>` | ( *i*x n* -- ) R: ( -- *i*x n* ) | Several cells to and from the return stack |
| `synonym` | ( "new" "old" -- ) | Another name for a word |
| `traverse-wordlist` | ( *i*x xt wid* -- *j*x* ) | Run *xt* for each word of a word list |
| `name>string`, `name>interpret`, `name>compile` | ( *nt* -- ... ) | A word's name, and how it runs |

## A.7 Facility (`facility`)

| Word | Stack | Does |
|---|---|---|
| `key?` | ( -- *flag* ) | Is a key waiting? |
| `ekey`, `ekey?` | ( -- *u* ), ( -- *flag* ) | A key, special keys too; is one waiting? |
| `ekey>char`, `ekey>fkey` | ( *u* -- *u* false \| *char* true ), ( *u* -- *u* false \| *x* true ) | Is it a character? A special key? |
| `k-up` `k-down` `k-left` `k-right` `k-home` `k-end` `k-prior` `k-next` `k-insert` `k-delete` `k-f1` ... `k-f12` | ( -- *x* ) | The special keys, as `ekey>fkey` gives them |
| `k-resize`, `k-focus` | ( -- *x* ) | (Not keys: the window was resized; another window of the group was chosen) |
| `k-shift-mask`, `k-ctrl-mask`, `k-alt-mask` | ( -- *x* ) | Added to a key typed with them held |
| `emit?` | ( -- *flag* ) | Can `emit` go on? (always) |
| `ms` | ( *u* -- ) | Wait *u* milliseconds |
| `time&date` | ( -- *sec min hour day month year* ) | The date and time |
| `page` | ( -- ) | Clear the screen |
| `at-xy` | ( *u1 u2* -- ) | Cursor to column *u1*, row *u2* (from 0) |
| `form` | ( -- *rows cols* ) | The window's size |
| `clear-line`, `clear-below` | ( -- ) | Clear the rest of the line, of the screen |
| `cursor-up`, `cursor-down`, `cursor-right`, `cursor-left` | ( *n* -- ) | Move the cursor |
| `cursor-save`, `cursor-restore`, `cursor-off`, `cursor-on` | ( -- ) | Save, restore, hide, show the cursor |
| `color`, `bgcolor` | ( *c* -- ) | The text's, the background's colour (0-15) |
| `black` `red` `green` `yellow` `blue` `magenta` `cyan` `white` | ( -- *c* ) | The colours 0 to 7 |
| `bright` | ( *c1* -- *c2* ) | A colour's bright version (8-15) |
| `plain`, `bold`, `dim`, `underline`, `blink`, `reverse` | ( -- ) | Styles (`plain`: all off) |
| `sgr` | ( *n* -- ) | Any `ESC [`*n*`m` |
| `beep` | ( -- ) | Ring the bell |
| `begin-structure`, `end-structure` | ( "name" -- *addr* 0 ), ( *addr n* -- ) | A structure's start and end |
| `+field`, `field:`, `cfield:` | ( *n1 n2* "name" -- *n3* ), ( *n1* "name" -- *n2* ) | Its fields |

## A.8 String (`string`)

| Word | Stack | Does |
|---|---|---|
| `cmove`, `cmove>` | ( *c-addr1 c-addr2 u* -- ) | Copy bytes, from the start up; from the end down |
| `-trailing` | ( *c-addr u1* -- *c-addr u2* ) | Without the spaces at the end |
| `/string` | ( *c-addr u n* -- *c-addr+n u-n* ) | Skip *n* characters |
| `blank` | ( *c-addr u* -- ) | Fill with spaces |
| `compare` | ( *c-addr1 u1 c-addr2 u2* -- *n* ) | -1, 0 or 1, as they sort |
| `search` | ( *c-addr1 u1 c-addr2 u2* -- *c-addr3 u3 flag* ) | Find the second string in the first |
| `sliteral` (I C) | ( *c-addr u* -- ) | Compile a string |
| `replaces`, `substitute`, `unescape` | | Fill in `%name%`s in a string |

## A.9 Search-Order (`search`)

| Word | Stack | Does |
|---|---|---|
| `forth-wordlist` | ( -- *wid* ) | The FORTH word list |
| `wordlist` | ( -- *wid* ) | A new, empty word list |
| `get-current`, `set-current` | ( -- *wid* ), ( *wid* -- ) | Where new words go |
| `definitions` | ( -- ) | New words go in the first word list of the order |
| `search-wordlist` | ( *c-addr u wid* -- 0 \| *xt* 1 \| *xt* -1 ) | Look a word up in a word list |
| `get-order`, `set-order` | ( -- *widn ... wid1 n* ), ( *widn ... wid1 n* -- ) | The search order |
| `only`, `also`, `previous`, `forth`, `order` | | Change, and show, the order |
| `library` | ( "name" -- *wid* ) | Start a library of your own: a new word list, first in the order |
| `end-library` | ( *wid* -- ) | End it |

## A.10 Double-Number (`double`)

`2constant`, `2variable`, `2value`, `2literal` (I), `d+`, `d-`, `m+`, `dnegate`, `dabs`, `d0=`, `d0<`, `d2*`,
`d2/`, `d=`, `d<`, `du<`, `dmax`, `dmin`, `d>s`, `d.`, `d.r`, `m*/` and `2rot`, as the standard has them: the
double-cell (32-bit) versions of the cell words.

## A.11 Memory-Allocation, Locals and Block

| Word | Stack | Does |
|---|---|---|
| `allocate` | ( *u* -- *addr ior* ) | A block of *u* bytes, from the heap (`memory`) |
| `free` | ( *addr* -- *ior* ) | Give one back |
| `resize` | ( *addr1 u* -- *addr2 ior* ) | Change one's size |
| `{:` *args* `| ` *vals* `--` *comment* `:}` (I C) | | Local variables, from the stack (`locals`) |
| `locals|` *names* `|` (I C), `(local)` | | The older way to make locals |
| `block`, `buffer` | ( *u* -- *addr* ) | Block *u*'s buffer, read in; a buffer for it (`block`) |
| `update`, `save-buffers`, `flush`, `empty-buffers` | ( -- ) | Mark changed; write; write and free; free |
| `blk`, `scr` | ( -- *addr* ) | The block being run; the block listed |
| `load`, `thru` | ( *u* -- ), ( *u1 u2* -- ) | Run blocks |
| `list` | ( *u* -- ) | Show a block |
| `open-blocks` | ( *c-addr u* -- ) | Use a file of blocks |

## A.12 The shell (`shell`)

| Word | Stack | Does |
|---|---|---|
| `%` | ( "line" -- ) | The rest of the line is rc's |
| `cd` | ( "dir" -- ) | Change directory |
| `bind`, `mount`, `unmount`, `newns` | ( "args" -- ) | The namespace, as rc's (section 7.14) |
| `prompt`, `prompt2` | ( *c-addr u* -- ) | The prompt's format; the second prompt's |
| `status` | ( -- *n* ) | The last command's status |
| `exit` (I) | | At the prompt: leave HyForth; in a definition, the standard's `exit` |
| `exits` | ( *n* -- ) | Leave HyForth with status *n* |
| `wait` | ( *task* -- ) | Wait for a background task |
| `getenv`, `setenv`, `unsetenv` | ( *c-addr u* -- *c-addr2 u2* ), ( *c-addr1 u1 c-addr2 u2* -- ), ( *c-addr u* -- ) | Environment variables |
| `spawn` | ( *c-addr u* -- *task* ) | Start a command, not waiting |
| `sh-out` | ( *c-addr u* -- *c-addr2 u2* ) | A command's output |
| `output-of` | ( *xt* -- *c-addr u* ) | What a word prints |
| `|` | ( *xt* "line" -- ) | What a word prints, into a command |
| `piped` | ( *xt c-addr u* -- ) | The same, the command a string |
| `send` | ( "n line" -- ) | Type a line into window *n* |

## A.13 The Hydra (`hydra`)

| Word | Stack | Does |
|---|---|---|
| `sh` | ( *c-addr u* -- *status* ) | Run a command line with rc, and wait |
| `run` | ( *c-addr u* -- *status* ) | Run a program and its arguments, and wait |
| `ctl` | ( *c-addr1 u1 c-addr2 u2* -- ) | Write text *c-addr2 u2* to file *c-addr1 u1*, in one write |
| `>z`, `zcount` | ( *c-addr u* -- *z-addr* ), ( *z-addr* -- *c-addr u* ) | To and from a zero-ended string |
| `argc`, `arg` | ( -- *n* ), ( *n* -- *c-addr u* ) | A script's arguments (0: its name) |
| `get-dir`, `set-dir` | ( *c-addr1 u1* -- *c-addr2 u2* ), ( *c-addr u* -- *ior* ) | The current directory |
| `open-dir`, `read-dir`, `close-dir` | | A directory's names, one at a time |
| `=mkdir` | ( *c-addr u mode* -- *ior* ) | Make a directory |
| `bank-window` | ( -- *addr* ) | $8000, where a RAM bank is seen |
| `bank!`, `bank@` | ( *bank* -- ), ( -- *bank* ) | Choose, and read, the bank there |
| `seg-bank!` | ( *seg n* -- *ior* ) | A shared segment's bank there |
| `code-banks` | ( *flag* -- ) | Compile definitions into banks (true, as it starts) or the dictionary (false) |
| `note`, `note-group` | ( *task n* -- ), ( *group n* -- ) | Send a note |
| `on-note` | ( *xt* -- ) | *xt* ( *n* -- *flag* ) handles notes (0: none) |
| `pause` | ( -- ) | Let the other tasks have a turn |
| `ior>text` | ( *ior* -- *c-addr u* ) | An error's message |
| `sys` | ( *addr a x y* -- *a x y p* ) | Call machine code, with registers |
| `sys-`*name* | | Each system call (Appendix K): its inputs, in order, the first deepest; its outputs, and an ior if it can fail |

The `sys-` words are: `sys-sysinfo` `sys-errstr` `sys-kmesg` `sys-reboot` `sys-exits` `sys-wait` `sys-spawn`
`sys-getpid` `sys-yield` `sys-sleep` `sys-sleep-until` `sys-ticks` `sys-preempt-off` `sys-preempt-on` `sys-taskinfo`
`sys-note` `sys-modinfo` `sys-env-get` `sys-env-put` `sys-env-del` `sys-env-name` `sys-taskread` `sys-getppid`
`sys-sem-new` `sys-sem-acquire` `sys-sem-try` `sys-sem-release` `sys-sem-free` `sys-exec` `sys-break`
`sys-pages-alloc` `sys-pages-free` `sys-banks` `sys-banks-alloc` `sys-banks-free` `sys-seg-create` `sys-seg-attach`
`sys-seg-detach` `sys-seg-map` `sys-seginfo` `sys-banks-alloc-in` `sys-seg-create-in` `sys-open` `sys-create`
`sys-close` `sys-read` `sys-write` `sys-seek` `sys-stat` `sys-fstat` `sys-wstat` `sys-fwstat` `sys-remove` `sys-dup`
`sys-dup2` `sys-chdir` `sys-getcwd` `sys-pipe` `sys-fd2path` `sys-bind` `sys-mount` `sys-unmount` `sys-nsinfo`
`sys-putc` `sys-puts` `sys-getc` `sys-puthex` `sys-time` `sys-time-set` and `sys-rtc`.

## A.14 Numbers (`numbers`)

The number stack's words; `N:` shows it. Any number Forth's cells can't hold goes on the number stack.

| Word | Stack | Does |
|---|---|---|
| `ndepth` | ( -- *u* ) | How many numbers |
| `ndrop`, `ndup`, `nover`, `nswap`, `nrot` | N: | As the cell words |
| `n+`, `n-`, `n*`, `n/` | N: ( *x y* -- *z* ) | Exactly (`n/` gives fractions) |
| `n/mod` | N: ( *x y* -- *rem quot* ) | Whole-number division |
| `nnegate`, `nabs`, `ngcd`, `npow` | N: | Negate; size; greatest common divisor; a power |
| `ncompare` | ( -- -1 \| 0 \| 1 ) N: ( *x y* -- ) | Compare |
| `n=`, `n<`, `n>`, `n0=`, `n0<` | ( -- *flag* ) N: ( ... -- ) | Tests |
| `int?`, `fixed?`, `rational?`, `complex?` | ( -- *flag* ) N: ( *x* -- ) | Its kind |
| `s>n`, `d>n` | ( *n* -- ) N: ( -- *x* ), ( *d* -- ) N: ( -- *x* ) | From Forth's stack |
| `n>s`, `n>d` | ( -- *n* ) N: ( *x* -- ), ( -- *d* ) N: ( *x* -- ) | To Forth's stack |
| `truncate`, `nfloor`, `nround` | N: ( *x* -- *n* ) | To a whole number |
| `to-fixed` | ( *u* -- ) N: ( *x* -- *y* ) | A fixed decimal of *u* places |
| `to-rational`, `rational.n`, `rational.d` | N: | A fraction; its top; its bottom |
| `complex` | N: ( *re im* -- *z* ) | A complex number |
| `nrandom`, `nseed` | N: ( *n* -- *r* ), ( *u* -- ) | Random numbers |
| `nfib` | N: ( *n* -- *f* ) | A Fibonacci number |
| `nand`, `nor`, `nxor`, `ninvert` | N: | Bits |
| `nlshift`, `nrshift` | ( *u* -- ) N: ( *x* -- *y* ) | Shift |
| `nbit?` | ( *u* -- *flag* ) N: ( *x* -- ) | A bit |
| `nbytes`, `nfrom-bytes` | ( -- *c-addr u* ) N: ( *x* -- ), ( *c-addr u* -- ) N: ( -- *x* ) | The stored form's bytes |
| `n.`, `n.base` | N: ( *x* -- ), ( *c-addr u* -- ) N: ( *x* -- ) | Print in the base; in a base named |
| `n>str` | ( -- *c-addr u* ) N: ( *x* -- ) | As a string |
| `n.s` | ( -- ) | Show the number stack |
| `>n` | ( *c-addr u* -- *flag* ) N: ( -- *x* \| ) | Read a number from a string |
| `set-base`, `get-base` | ( *c-addr u* -- ), ( -- *c-addr u* ) | The base (any of Chapter 17's) |
| `nformat` | ( *c-addr u* -- ) N: ( *x1 ... xn* -- ) | Print a format, each `{}` a number |
| `nsqrt` `nexp` `nlog` `nsin` `ncos` `ntan` `natan` `npi` | N: | The math functions |
| `digits` | ( -- *addr* ) | The variable holding their precision (12) |
| `nliteral` (I C), `nconstant`, `nvariable`, `nvalue`, `n@`, `n!` | | Numbers in definitions, constants, variables |

## A.15 Sound (`sound`)

`snd-reset`, `snd-volume` ( *v* -- ), `snd-claim` `snd-release` `snd-claim-psg` `snd-release-psg` ( *mask* -- ),
`snd-reg` ( *reg val* -- ), `snd-patch` ( *ch p* -- ), `snd-note` ( *ch n* -- ), `snd-off` ( *ch* -- ), `snd-level`
( *ch v* -- ), `snd-pan` ( *ch pan* -- ), `snd-bend` `snd-glide` `snd-drum` ( *ch n* -- ), `snd-freq` ( *ch hz* -- ),
`snd-sens` ( *ch pms ams* -- ), `snd-noise` ( *n* -- ), `snd-wave` ( *ch w width* -- ), `snd-lfo` ( *rate pmd amd wave*
-- ), `snd-regs` ( *c-addr* -- ), `snd-play` ( *c-addr u times* -- *status* ), `snd-mml` ( *ch c-addr u* -- *status* ),
`snd-chord` ( *ch c-addr u* -- *status* ), `note-of` ( *c-addr u* -- *n* ), `tune` ( *c-addr u ch tempo* -- ): section
19.6.

## A.16 Bits, random numbers and the disassembler

| Word | Stack | Does |
|---|---|---|
| `tbit` | ( *n b* -- *n flag* ) | Is bit *b* of *n* set? (`bits`) |
| `sbit`, `cbit` | ( *n b* -- *n'* ) | Set, clear bit *b* |
| `rand`, `rand32` | ( -- *u* ), ( -- *ud* ) | A random number (`random`) |
| `random` | ( *u1* -- *u2* ) | One from 0 to *u1* - 1 |
| `rseed` | ( *ud* -- ) | Start the random numbers |
| `disasm` | ( *addr u* -- ) | Disassemble *u* instructions from *addr* (`disasm`: Chapter 26) |

## A.17 The device libraries

These are Forth source files, in `/lib/forth`. Their words are described in Chapter 19:

| Library | Words |
|---|---|
| `gpio` | `gpio` `gpio!` `gpio-in` `gpio-out` `gpio-port` `gpio-port!` `gpio-ddr!` `gpio-ca1!` `gpio-ca2!` `gpio-wait` `gpio-state` |
| `i2c` | `i2c-read` `i2c-write` `i2c-speed` `i2c-reg-size` `i2c-devices` `i2c?` |
| `spi` | `spi` `spi-read` `spi-mode` |
| `clock` | `set-date` `rtc` |
| `disk` | `disk-ctl` `disk-cmd` `disk-start` `disk-stop` `cards` |
| `cons` | `window` `windows` `new-window` `new-group` `show-window` `window-label` `window-status` `window-ctl` |
| `proc` | `task-args` `task-cwd` `task-env` `task-ns` `task-regs` `task-mem` `task-ram` |
| `pc` | `pc?` |
| `video` | `bitmap` `bitmap-off` `border` `pen` `plot` `line` `box` `bar` `circle` `disc` `clear` `text` `home` `cs` `pu` `pd` `rt` `lt` `heading` `seth` `fd` `bk` `vpoke` `vpeek` `vram!` `vram@` `palette!` `sprite!` `sprite-at` `sprite-off` `vsync` `mouse` `mouse-wait` |

## A.18 Errors

HyForth's errors are the standard's **throw codes**; these are the ones you'll see most:

| Code | Message |
|---|---|
| -3, -4 | Stack overflow, stack underflow |
| -9 | Invalid memory address |
| -10 | Division by zero |
| -11 | Result out of range |
| -13 | Undefined word (shown as the word and a `?`) |
| -14 | Interpreting a compile-only word |
| -24 | Invalid numeric argument |
| -28 | User interrupt (<kbd>CTRL</kbd>-<kbd>C</kbd>) |
| -512 - *e* | A system error, *e* from Appendix G (as the file words' iors are) |
