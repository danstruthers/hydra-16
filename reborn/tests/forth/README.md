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
| `exceptiontest.fth` | Exception | Gerry Jackson's: public domain |
| `facilitytest.fth` | Facility (its structures) | Gerry Jackson's: public domain |
| `filetest.fth` | File Access | Gerry Jackson's: public domain |
| `required-helper1.fth`, `required-helper2.fth` | What `filetest.fth` INCLUDEs and REQUIREs | Gerry Jackson's: public domain |
| `toolstest.fth` | Programming-Tools | Gerry Jackson's: public domain |
| `searchordertest.fth` | Search-Order | Gerry Jackson's: public domain |
| `stringtest.fth` | String | Gerry Jackson's: public domain |

They're bytes, LF line ends (`.gitattributes`: not text to Git).  The test puts them on an emulated SD card, each as
itself, with a `run.fs` of its own that INCLUDEs them in turn, as `runtests.fth` does (and has a line for `core.fr`'s
ACCEPT test after it: ACCEPT reads stdin); then it runs `cd /sd/0; forth <run.fs`.  `filetest.fth` makes its files
there too.
