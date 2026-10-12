# Card 4. HyForth

Chapter 19 teaches it, and Appendix A lists every word. The stack diagrams read ( *before* -- *after* ), with the
top of the stack on the right.

### The stack

| Word | Stack | Word | Stack |
|---|---|---|---|
| `dup` | ( *a* -- *a a* ) | `swap` | ( *a b* -- *b a* ) |
| `drop` | ( *a* -- ) | `over` | ( *a b* -- *a b a* ) |
| `rot` | ( *a b c* -- *b c a* ) | `nip` | ( *a b* -- *b* ) |
| `2dup` | ( *a b* -- *a b a b* ) | `2drop` | ( *a b* -- ) |
| `.s` | Shows the stack | `depth` | ( -- *how many* ) |

### Arithmetic and tests

| Words | What they do |
|---|---|
| `+` `-` `*` `/` `mod` | ( *a b* -- *c* ): `7 2 mod` is 1 |
| `/mod` | ( *a b* -- *remainder quotient* ) |
| `1+` `1-` `2*` `2/` `negate` `abs` | ( *a* -- *b* ) |
| `min` `max` | ( *a b* -- *c* ) |
| `=` `<>` `<` `>` | ( *a b* -- *flag* ): -1 is true, 0 is false |
| `0=` `0<` `0>` | ( *a* -- *flag* ) |
| `and` `or` `xor` `invert` | On flags, and on bits |

### Printing

| Word | What it does |
|---|---|
| `.` | ( *n* -- ) Print a number |
| `4 .r` | ( *n* -- ) Print it in a column 4 wide |
| `."` *text*`"` | Print the text (in a definition) |
| `cr`, `space`, `5 spaces` | A new line; a space; five |
| `emit` | ( *code* -- ) Print a character: `65 emit`, `[char] * emit` |
| `s"` *text*`"`, `type` | ( -- *addr len* ) A string; ( *addr len* -- ) print one |

### Definitions

```text
: square ( n -- n*n ) dup * ;         \ a new word: ( ... ) and \ are comments
variable score      10 score !   score @ .   5 score +!
42 constant answer
0 value lives       3 to lives
create row 10 , 20 , 30 ,             \ row 1 cells + @  is 20
```

### Decisions and loops (only inside a definition)

```text
: sign ( n -- ) 0< if ." negative" else ." not negative" then ;
: five ( -- ) 5 0 do i . loop ;                    \ 0 1 2 3 4
: evens ( -- ) 10 0 do i . 2 +loop ;               \ 0 2 4 6 8
: down ( n -- ) begin dup . 1- dup 0= until drop ; \ n ... 1
```

`?do` for a loop that might go round no times; `leave` to leave one early; `exit` to leave a word; `recurse` for a
word to call itself.

### Files, libraries, and looking around

| Word | What it does |
|---|---|
| `include` *file* | Read a file of Forth, as if it were typed |
| `lib` *name* | Load a library: `random`, `facility`, `numbers`, `video`, `sound`, `hydra` ... |
| `words` | List every word |
| `see` *word* | Show a word's definition |
| `forth`, `bye` | A plain HyForth, where every line is Forth; leave it |

### When it goes wrong (section 19.15)

| You see | It means |
|---|---|
| *word* `?` | HyForth doesn't know that word. A missing space? |
| `rc:` *word*`: not found` | The line's first word wasn't Forth, so rc had the line |
| A prompt of spaces | A `:` with no `;` yet |
| `stack underflow` | A word wanted more numbers than there were |
| `if: compile only` | That word only works inside a definition |
