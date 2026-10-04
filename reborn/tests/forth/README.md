# The Forth 2012 test suite

The forth test (`tests/tests.js`) runs these on HyForth: the files from Gerry Jackson's Forth 2012 test suite
(<https://github.com/gerryjackson/forth2012-test-suite>, `src/`), as they came, in its own order (`runtests.fth`'s):

| File | What | Terms (each file's notice) |
| :--- | :--- | :--- |
| `prelimtest.fth` | The preliminary tests, before the tester | Gerry Jackson's: public domain |
| `tester.fr` | John Hayes's tester (`T{ ... -> ... }T`) | (C) 1995 Johns Hopkins University / Applied Physics Laboratory: may be distributed freely as long as the notice remains |
| `core.fr` | The Core word set's tests | The same as `tester.fr` |
| `coreplustest.fth` | More Core tests | Gerry Jackson's: public domain |
| `utilities.fth`, `errorreport.fth` | What the optional word sets' tests use | Gerry Jackson's: public domain |
| `coreexttest.fth` | The Core Extension word set's tests | Gerry Jackson's: public domain |

They're bytes, LF line ends (`.gitattributes`: not text to Git).  The test puts them on an emulated SD card, one
after another, and runs `forth </sd/0/suite.fs`.
