# About This Reference

This is the *Hydra-16 Technical Reference*, **in progress**. Its first chapter, HyBASIC's keywords, is being
written letter by letter; the other chapters have one or two entries each so far, to show what they will be.

The *User's and Builder's Manual* teaches the Hydra, and its appendices list what each language has in a line
apiece. This book is where to look when a line isn't enough: when the question is what a keyword does with an
empty string, which errors a call can return, or what is left in a variable after a loop ends. It doesn't teach.
It assumes the manual has been read, and it says the same thing about every entry, in the same order.

## What it will hold

| Chapter | Entries | Written so far |
|---|---|---|
| 1. HyBASIC keywords | Every statement, function and operator | `ABS` to `CVN` (all of A to C); `FOR`...`NEXT`, `INPUT`, `OPEN` |
| 2. Commands | The shell's own commands, and the programs in `/bin` | `cp` |
| 3. System calls | Every call a program can make | `OPEN`, `READ` |
| 4. HyForth words | The core and every library | `>number` |
| 5. hylang forms | The special forms and the functions | `let` |

The other reference appendices of the manual (the 65C02's instructions, the character sets, the memory map, the
errors) will follow in the same way.

## How to read an entry

Every entry has the same parts, in this order. A part with nothing to say is left out.

Syntax
: Each form the thing can take. Words in `CAPITALS` (or, for a command or a HyForth word, as printed) are typed
  as they are; *italic* names stand for what you supply; `[ ]` encloses what can be left out; `{ | }` is a choice
  of one; `...` means the last item can repeat.

Arguments
: Each one: its type, its range, and what it means. For a system call, the registers in.

Result
: What a function returns, a word leaves on the stack, or a call leaves in the registers.

Description
: What happens, in the order it happens: what is evaluated and when, what is changed, where it stops.

Errors
: Each error it can raise, with its number and its message as the Hydra prints it, and what causes it.

Notes
: Limits, edge cases, and behaviour that differs from what another system's user would expect.

See also
: Related entries.

Example
: A transcript, taken from the Hydra. Every one in this book is replayed in the emulator when the book is built,
  so what is printed here is what the ROM does.

## How it was written

Each entry was written from the interpreter's, the program's or the kernel's source, and each statement of
behaviour in it was then tried on the system. Where the source and the trial disagreed with what one would expect
of the language elsewhere (QuickBASIC, the Forth standard), the entry says what the Hydra does.

Section and chapter numbers such as "section 13.2" and "Appendix G" are the *User's and Builder's Manual*'s.
