# cc65 for the Hydra-16

The build uses cc65 (`ca65`, `ld65`, `cc65`, `ar65`, `od65`, and cc65's `include`, `asminc` and `lib`) as this
folder has it: upstream cc65 at commit 6bd01e8a5 (October 2026) with four fixes of its code generator and optimizer,
each a patch here.

| | |
| :-- | :-- |
| `0001-...patch` | A post-increment or decrement lost before a call through a function pointer |
| `0002-...patch` | A ternary's result used in place when the expression's result is unused |
| `0003-...patch` | OptPtrStore1 and 2 taking .A and .X from before a label |
| `0004-...patch` | AdjustStackOffset inserting a .Y load before a label (an upstream regression that miscompiled dq0) |
| `win64_hydra/` | That cc65 built for 64-bit Windows: `bin`, `include`, `asminc`, `cfg`, and `lib` (`none.lib`, which the builds use, and the simulator's two); `VERSION.txt` says how it was built, `LICENSE` is cc65's |

On Windows nothing is to be installed: `base/build.js` (its `CC65_DIR`), `reborn/build.js` and `old/build.js` use
`win64_hydra` unless `CC65_BIN` or `CC65_HOME` names another cc65.  On Linux or macOS, build cc65 from its source with
the patches, as the CI does (`.github/workflows/build.yml`), and set `CC65_HOME` to its folder (or put its `bin` on the
PATH):

```
git clone https://github.com/cc65/cc65.git && cd cc65
git checkout 6bd01e8a5b0e86389734fe22c679792214c01dc6
git apply /path/to/hydra-16/base/tools/cc65/*.patch
make -j4
export CC65_HOME=$PWD
```

The images in Git are this cc65's: a different cc65 makes different ones (and stock cc65 2.19 miscompiles some of the
C, and its ca65 doesn't take WAI with `--cpu W65C02`).  To make `win64_hydra` again: in a clone of cc65 at 6bd01e8a5
with the patches applied (the user's clone has them as its branch `hydra-optfix`), build `src/cc65.sln` (Release, x64)
with Visual Studio, then `lib/none.lib` from `libsrc` with those tools (`cc65 -Or`, `ca65 -g`); copy `bin`, `include`,
`asminc`, `cfg` and `lib` here, and say so in `VERSION.txt`.
