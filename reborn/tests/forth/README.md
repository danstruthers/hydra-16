# The Forth 2012 test suite

The forth test (`tests/tests.js`) runs these on HyForth: the files from Gerry Jackson's Forth 2012 test suite
(<https://github.com/gerryjackson/forth2012-test-suite>, `src/`), as they came, in its own order (`runtests.fth`'s, for
the word sets HyForth has):

| File | What | Terms (each file's notice) |
| :--- | :--- | :--- |
| `prelimtest.fth` | The preliminary tests, before the tester | Gerry Jackson's: public domain |
| `tester.fr` | John Hayes's tester (`T{ ... -> ... }T`) | (C) 1995 Johns Hopkins University / Applied Physics Laboratory: may be distributed freely as long as the notice remains |
| `core.fr` | The Core word set's tests | The same as `tester.fr` |
| `coreplustest.fth` | More Core tests | Gerry Jackson's: public domain |
| `utilities.fth`, `errorreport.fth` | What the optional word sets' tests use (`REPORT-ERRORS`: the table at the end) | Gerry Jackson's: public domain |
| `coreexttest.fth` | The Core Extension word set's tests | Gerry Jackson's: public domain |
| `blocktest.fth` | Block (and its extension's words) | Steve Palmer's (with others' contributions): public domain |
| `doubletest.fth` | Double-Number (and its extension's 2ROT, 2VALUE, DU<) | Gerry Jackson's: public domain |
| `exceptiontest.fth` | Exception | Gerry Jackson's: public domain |
| `facilitytest.fth` | Facility (its structures) | Gerry Jackson's: public domain |
| `filetest.fth` | File Access | Gerry Jackson's: public domain |
| `required-helper1.fth`, `required-helper2.fth` | What `filetest.fth` INCLUDEs and REQUIREs | Gerry Jackson's: public domain |
| `localstest.fth` | Locals | Gerry Jackson's: public domain |
| `memorytest.fth` | Memory-Allocation | Gerry Jackson's: public domain |
| `toolstest.fth` | Programming-Tools | Gerry Jackson's: public domain |
| `searchordertest.fth` | Search-Order | Gerry Jackson's: public domain |
| `stringtest.fth` | String | Gerry Jackson's: public domain |

They're bytes, LF line ends (`.gitattributes`: not text to Git).  The test puts them on an emulated SD card, each as
itself, with a `run.fs` of its own that INCLUDEs them in turn, as `runtests.fth` does (and has a line for `core.fr`'s
ACCEPT test after it: ACCEPT reads stdin); then it runs `cd /sd/0; forth <run.fs`.  `filetest.fth` makes its files
there too.

`numberstest.fth` is HyForth's own, not the suite's: the number words (`lib numbers`: `numbers.fl`, on the numbers
and math libraries) checked in the suite's way, with `tester.fr`'s `T{ ... -> ... }T`, each number by its text
(`N>STR`); its last lines are typed (`N.`, `N.BASE`, `N.S`, `NFORMAT`, cells in bases that aren't a radix) and the
count of its errors.  The `fnumbers` test puts it and `tester.fr` on a card of its own, with a `nums.fs` that
REQUIREs `string.fl`, `hydra.fl` and `numbers.fl` and INCLUDEs them, and compares what's typed.
