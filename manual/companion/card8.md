# Card 8. Numbers and Memory

### Hex digits (section 1.6)

| Decimal | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Hex | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B | C | D | E | F |
| Binary | 0000 | 0001 | 0010 | 0011 | 0100 | 0101 | 0110 | 0111 | 1000 | 1001 | 1010 | 1011 | 1100 | 1101 | 1110 | 1111 |

A byte is two hex digits: `$A5` is `1010 0101`, which is 10 × 16 + 5 = 165.

### Powers of two

| Power | Decimal | Hex | Worth knowing |
|---|---|---|---|
| 2^4 | 16 | $10 | One hex digit's worth |
| 2^7 | 128 | $80 | A byte's top bit |
| 2^8 | 256 | $100 | The values a byte can hold; a page |
| 2^10 | 1,024 | $400 | 1K |
| 2^13 | 8,192 | $2000 | 8K: a RAM bank |
| 2^14 | 16,384 | $4000 | 16K: a paged ROM bank |
| 2^15 | 32,768 | $8000 | 32K: a task's own memory |
| 2^16 | 65,536 | $10000 | 64K: all the 65C02 can see at once |
| 2^20 | 1,048,576 | $100000 | 1 MB |

### Writing a number in a base (Chapter 18)

| Written | Where | Means |
|---|---|---|
| `$FF` | This book, assembly language, HyForth | Hex |
| `#xFF`, `#b1010`, `#o17` | `calc`, hylang, HyBASIC, HyForth's numbers library | Hex; binary; octal |
| `%1010` | HyForth, assembly language | Binary |
| `&HFF` | HyBASIC | Hex |
| `calc -b x 255` | The shell | Print 255 in hex |

### Characters (Appendix E has them all)

| Code | Is | Code | Is | Code | Is |
|---|---|---|---|---|---|
| 10 ($0A) | A new line | 48 to 57 ($30-$39) | `0` to `9` | 65 to 90 ($41-$5A) | `A` to `Z` |
| 27 ($1B) | <kbd>ESC</kbd> | 32 ($20) | A space | 97 to 122 ($61-$7A) | `a` to `z` |

A small letter's code is its capital's plus 32. A digit's code is the digit plus 48.

### The Hydra's memory, as the 65C02 sees it (section 7.2)

| Addresses | What's there | Chosen by |
|---|---|---|
| $0000-$7FFF | The running task's own 32K of RAM | The T register |
| $8000-$9FFF | A RAM bank: 8K of a memory card, or of the shared RAM | The RAM bank register, at $00 |
| $A000-$DFFF | A paged ROM bank: 16K | The ROM bank register, at $01 |
| $E000-$FEFF | A page of the kernel ROM: 8K | The W register |
| $FF00-$FFEF | The I/O ports: the chips that talk to the world | |
| $FFF0-$FFF3 | The registers T, U, V and W | |
| $FFFE-$FFFF | The vector RAM: where interrupts go | |

### Inside a task's 32K

| Addresses | What's there |
|---|---|
| $00, $01 | The RAM and ROM bank registers |
| $02-$21 | The call registers, r0 to r15: two bytes each, for system calls |
| $22-$7F | The program's own zero page: 94 bytes for its variables |
| $80-$FF | HydraOS's zero page for the task |
| $0100-$01FF | The stack |
| $0200-$03FF | HydraOS's notes on the task: its open files, its name, its arguments |
| $0800 on | A program: its header (48 bytes), then its code, its data and its free memory |

### Sizes

| Thing | Size |
|---|---|
| A bit | One 0 or 1 |
| A byte | 8 bits: 0 to 255 |
| A HyForth cell | 16 bits: 0 to 65,535, or -32,768 to 32,767 |
| An address | 16 bits |
| 1K | 1,024 bytes |
| 1 MB | 1,024 K |
| A disk block | 512 bytes |
| A tick | 1/200 of a second |
