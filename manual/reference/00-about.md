# About This Reference

This is the *Hydra-16 Technical Reference*. Its five chapters cover HyBASIC's keywords, the commands, the system
calls, HyForth's words and hylang's forms: every one has an entry.

The chapters are not all made in the same way. HyBASIC's keywords and the commands were each written out by
hand. The system calls are made from the kernel's own specification. For HyForth and hylang, the parts where a
language's exact behaviour most needs saying were written by hand (what the Forth standard leaves to the
system; the word families with several stack pictures; hylang's special forms), and the entries for the rest,
the standard's words and the plain functions, give the word's or the function's form and what it does in a
sentence, from the manual's tables.

The *User's and Builder's Manual* teaches the Hydra, and its appendices list what each language has in a line
apiece. This book is where to look when a line isn't enough: when the question is what a keyword does with an
empty string, which errors a call can return, or what is left in a variable after a loop ends. It doesn't teach.
It assumes the manual has been read, and it says the same thing about every entry, in the same order.

## What it holds

| Chapter | Entries |
|---|---|
| 1. HyBASIC keywords | Every statement, function and operator: 175 |
| 2. Commands | Every program in `/bin`, the system's own, and the shell's built-in commands |
| 3. System calls | All 90, with every constant and error code |
| 4. HyForth words | The core and every library: 300 entries |
| 5. hylang forms | The special forms and the functions: 145 entries |

The manual's other reference appendices (the 65C02's instructions, the character sets, the memory map) are
not repeated here.

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
