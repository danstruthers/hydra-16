# hylang's tests: danlang's regression suite

The hylang test (`tests/tests.js`) runs danlang's own regression suite on hylang: the files of `tests/regress/` from
danlang's master (<https://github.com/SNSTRUTHERS/danlang>), as it has them, each as itself.  danlang is the
reference (`docs/hylang.md`), and each step of the plan's phase 7 runs the files it makes pass; none that passed may
fail after.

| File | What | Runs from |
| :--- | :--- | :--- |
| `harness.dl` | The checks: `check`, `check-error`, the count of each and of the failures | 7.2 |
| `scope.dl` | Lexical scope: closures, `set!` on a closure's variables, a Q-expression run where it was written, `let`, fexprs | 7.2 |
| `control.dl` | `if`, `and`, `or`, `<=>`, `while`, `each`, `dotimes`, `range`, `try`, `error-message` | 7.2 |
| `errors.dl` | Errors as values: through built-ins, functions, `do` and `let`; `try`; the common ones | 7.2 |
| `core.hl` | hylang's own (not danlang's): what `eval.dl` and `reader.dl` check, at sizes fixnums hold (those files read numbers past them, so they run from step 7.3); equality and order of lists; `output-of` nested | 7.2 |

The suite's files are bytes, LF line ends (`.gitattributes`: not text to Git), danlang's terms (GPLv3: Daniel and
Simon Struthers).  The test puts them on an emulated SD card with a `run.hl` of its own that loads `harness.dl`, then
each file in turn (a file that stops with an error counts as a failure, as danlang's `run.dl` has it), and prints
the count; then it runs `cd /sd/0; hylang` and types `(load "run.hl")`.
