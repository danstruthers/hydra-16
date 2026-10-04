# hylang's tests: danlang's regression suite

The hylang test (`tests/tests.js`) runs danlang's own regression suite on hylang: the files of `tests/regress/` from
danlang's master (<https://github.com/SNSTRUTHERS/danlang>), as it has them, each as itself.  danlang is the
reference (`docs/hylang.md`), and each step of the plan's phase 7 runs the files it makes pass; none that passed may
fail after.

| File | What | Runs from |
| :--- | :--- | :--- |
| `harness.dl` | The checks: `check`, `check-error`, the count of each and of the failures | 7.2 |
| `reader.dl` | The reader: numbers, strings and their escapes, here strings, characters, atoms, symbols, comments, the shorthands, `read` (its two checks of hashes, from 7.4a) | 7.3 |
| `scope.dl` | Lexical scope: closures, `set!` on a closure's variables, a Q-expression run where it was written, `let`, fexprs | 7.2 |
| `control.dl` | `if`, `and`, `or`, `<=>`, `while`, `each`, `dotimes`, `range`, `try`, `error-message` | 7.2 |
| `errors.dl` | Errors as values: through built-ins, functions, `do` and `let`; `try`; the common ones | 7.2 |
| `lists.dl` | `list`, `head`, `tail`, `init`, `end`, `join`, `len`, `item-at`, `subset`, `reverse`, `sort`, `index-of` in a list; equality | 7.4a |
| `strings.dl` | `+` of strings, `substring`, `char-at`, `str-split`, `index-of`, the `str-` functions, `format`, character codes and tests, comparison, `to-str`, `repr`, `print`, `write` | 7.4a |
| `numbers.dl` | Integers of any size, fixed decimals, rationals, complex numbers: arithmetic, comparison, conversion, bases, `random`, `fib` | 7.3 |
| `hashes.dl` | Keys, `hash-get`, `hash-put`, `hash-remove`, `len`, keys and values, `hash-clone`, methods (`&0`), tags (private, locked, read-only, not-nil), `to#` and `from#`, equality | 7.4a |
| `types.dl` | `type-of`, the type tests, `to-sym`, `to-atom`, `gensym`, `defined?` (its check of `stdout`'s type fails till 7.4b has streams, and the test expects it to) | 7.4a |
| `core.hl` | hylang's own (not danlang's): what `eval.dl` checks, at sizes the test has time for; equality and order of lists; `output-of` nested | 7.2 |

`eval.dl` (calls, `fn`, partial application, `&N`, `def`, tail calls, the call depth) passes too, but isn't in the test:
its tail calls loop 50,000 times, 4,675M cycles in all (22 minutes at 3.58 MHz), so `core.hl` has its checks at
smaller sizes.

The suite's files are bytes, LF line ends (`.gitattributes`: not text to Git), danlang's terms (GPLv3: Daniel and
Simon Struthers).  The test puts them on an emulated SD card with a `run.hl` of its own that loads `harness.dl`, then
each file in turn (a file that stops with an error counts as a failure, as danlang's `run.dl` has it), and prints
the count; then it runs `cd /sd/0; hylang` and types `(load "run.hl")`.
