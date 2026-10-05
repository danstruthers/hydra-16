# hylang's tests: danlang's regression suite

These are danlang's regression suite, `tests/regress/` from <https://github.com/SNSTRUTHERS/danlang>, as it has them
(copied whole, line ends LF).  danlang, the C# interpreter, is hylang's reference, and `reference.md` there its
specification: the suite is that specification's executable form, so hylang passes it unchanged.  A change to the
language goes into danlang first, with checks here, and is copied over.  The `.hl` files are hylang's own (the
phases' scaffolding till the suite runs whole).

The old hylang (phase 7 as first built, to commit `a0973eb`) is gone: hylang is being written again from scratch, to
the plan "danlang: review and 65C02 plan" (`docs/hylang.md` has its design).  Each phase of it runs the files it makes
pass, from an emulated card, and none that passed may fail after:

| File | What | Runs from |
| :--- | :--- | :--- |
| `harness.dl` | The checks: `check`, `check-error`, the count of each and of the failures | 3 |
| `prelude.hl` | hylang's own: what the files want of `globals.dl` that hylang doesn't load yet (`unpack`, `cond`, `case`) | 3, till phase 8 loads `globals.hl` |
| `run3.hl` | hylang's own: phase 3's `run.dl` (the files it runs, then a tail loop of 50,000 steps); the `hysuite` test's card makes eval.dl's numbers past a fixnum smaller | 3, till `run.dl` runs whole |
| `run.dl` | Loads `harness.dl`, then each file, and prints the count; its status is 1 if a check failed | 7 (it wants `args`, `last-index-of`; `run3.hl` till then) |
| `reader.dl` | The reader: numbers in every base, strings, here strings, characters, atoms, symbols, comments, `[...]`, the shorthand, bytes, `read` | 7 (the reader is phase 2's, checked by the emulator's `hylang` test till then; the file needs `read`, and its numbers phase 5) |
| `eval.dl` | Calls, partial application, extra arguments, too many, tail calls (50,000 deep), nesting | 3 (its numbers past a fixnum made smaller); 5 whole |
| `scope.dl` | Lexical scope, closures, `set!`, a Q-expression run where it was written, `let`, fexprs | 3 |
| `control.dl` | `if`, `and`, `or`, `<=>`, `while`, `each`, `dotimes`, `range`, `try` | 3 |
| `errors.dl` | Errors as values: through calls and built-ins, `try`, the error texts | 3 |
| `lists.dl` | The list built-ins, `sort` | 4 |
| `types.dl` | `type-of`, the type tests, symbols and atoms | 4 |
| `library.dl` | The library's built-ins and `globals.dl`'s functions | 4 (its numbers 5) |
| `numbers.dl` | The tower, every base, the conversions | 5 |
| `bits.dl` | Bits and bytes | 5 |
| `strings.dl` | Strings and characters | 6 |
| `hashes.dl` | Hashes, tags, methods | 6 |
| `io.dl` | Streams, `load`, `save`, `read` | 7 |
| `system.dl` | Files, programs, the environment, the clock | 7 |
