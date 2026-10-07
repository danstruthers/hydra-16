## **Fixture card images**

Card images the regression tests start from (`regress.js`: a card's `image`), so they check that the ROM still reads, writes and checks cards made before, as well as the ones the tests make fresh with `tools/hydrafs.js`.  A test runs on a copy: these files never change.  They hold only the card's used blocks; the tests have the emulator's card claim its full size (`claim`, the emulator's `--sd FILE@BLOCKS`), and the rest reads as zeros.

| Image | Card | What's on it |
| :---- | :--- | :----------- |
| `tests-v1.img` | 64 MB (131072 blocks), HydraFS version 1 (the whole free map written), label TESTS | The first 198 blocks of the `sim/card.img` made for the `hydrafs` test's files: `hello.txt`, `games/star.frt`, `big.bin` (5000 bytes, 0-255 over and over), `many/` (10 files, more than a directory block holds), and `a` (16 KB in three extents, one in an extent block), `c`, `e`, `g` |
| `quick-v2.img` | 64 MB (131072 blocks), HydraFS version 2 (a quick format: one free map block written; the other three hold junk, $FF, which must be ignored), label QUICK | `note.txt`, `docs/list.txt`, `data.bin` (9000 bytes, byte i = i * 7 & $FF) |

The `cards` test reads them, writes a directory and a file on each, checks them with `fsck`, and checks the copies with the PC tool afterwards.  Keep these as they are: a new format gets a new image (and a new row here), so the old ones keep being tested.
