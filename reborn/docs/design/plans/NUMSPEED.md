## **The numbers' speed: where the time goes, and a plan to win it back**

A plan (October 2026) for the user's request: take the number system as it is now ([NUMBERS.md](NUMBERS.md), steps 1-8, merged into `reborn` at `b6c24ed`) as the final state of what it does, find out in depth where its time goes, and make it significantly faster.  **What it does doesn't change**: every result stays the same to the digit and the byte (every kind, every base, the math functions correctly rounded), and every entry and every language keeps its form; only the time does.  The numbers library's test, the cross-checks against danlang (`numxcheck.js`, `hyxcheck.js`), BASIC's suite and the 120 regression tests are the judge of every step.

### **Contents**
1. [How it was measured](#how-it-was-measured)
2. [What it costs now](#what-it-costs-now)
3. [Where the time goes](#where-the-time-goes)
4. [The plan](#the-plan)
5. [What to expect](#what-to-expect)
6. [The order of work](#the-order-of-work)
7. [Risks](#risks)
8. [Questions](#questions)

---

### **How it was measured**

All on the emulator, at 3.58 MHz, on `reborn` at `b6c24ed` (the build that's merged and on `main`).

* **A profiler by segment** (`nprof.js`, to be `sim/tools/nprof.js`: step 0): a program run at rc marks the stretches it wants measured (BASIC `POKE &H6F, k`, hylang `(poke 32767 k)`), and each stretch's cycles are counted by where they're spent: every module's labels by bank (from ld65's debug file of the module linked again), the kernel's, the RAM's.  Each library call (an `XCALL`) is timed from its call to its return, and inside it by label; and every routine's time is counted inclusive of what it calls (a shadow stack of the program's `JSR`s).
* **The workloads**: `numb.bas` and `numh.hl` (the before-and-after table in [../../status.md](../../status.md), "The numbers' cost, before and after"); small loops of 200 of one operation in BASIC and hylang, each operation in a loop of its own; the math functions 20 at a time; `bench.bas`'s twenty benchmarks, each a stretch of its own.

### **What it costs now**

**The math functions**, a call (cycles; BASIC's `SIN(I)` and so on, the library's time alone):

| Function | A call | ms a call | The first call of a program, more |
| :--- | ---: | ---: | :--- |
| `EXP` | 290,000 | 81 | 1,330,000 (log 10 made) |
| `SIN` | 185,000 | 52 | 790,000 (pi made) |
| `LOG` | 142,000 | 40 | 880,000 (log 2 made) |
| `SQR` | 127,000 | 35 | |
| `ATN` | 124,000 | 35 | 790,000 (pi) |

EhyBASIC's `SIN` took some 7 ms, in its 40-bit floating point (about 9 digits).

**The arithmetic on small numbers**, a call of the library (cycles), and BASIC's loop a pass (cycles, `FOR I = 1 TO 200: statement: NEXT`):

| Operation | BASIC: a pass | BASIC: the call | hylang: the call |
| :--- | ---: | ---: | ---: |
| An empty loop | 650 | | |
| `X = I` | 1,050 | | |
| `X = I + G` (integers: no call) | 1,450 | | |
| `X = A + B` (1.5 + 2.25) | 10,500 | 6,180 | 5,330 |
| `X = A * B` (1.5 * 2.25) | 10,450 | 6,100 | |
| `X = I / G` (a fraction, `I / 3`) | 13,200 | 10,060 | 9,830 |
| `X = C + D` (1/3 + 1/7) | 15,200 | 10,890 | 9,990 |
| `IF A < B` (1.5, 2.25) | 7,000 | 3,880 | 3,360 |
| `X = INT(A)` (1.5) | 6,600 | 4,480 | |
| `X = E * F` (two 12-digit integers) | 15,450 | 9,940 | 7,850 |
| `X = E + F` (two 12-digit integers) | 10,850 | 5,670 | |

**Big numbers and text**, a call (hylang's `numh.hl`):

| Call | Cycles |
| :--- | ---: |
| `*` of two 30-digit integers | 30,400 |
| `div` of a 60-digit integer by a 30-digit one | 224,600 |
| `to-str` of 2^100 (31 digits) | 45,400 |
| `val` of a 30-digit integer | 72,700 |
| BASIC's `VAL("123.25")` | 17,500 |

### **Where the time goes**

**1. A call's fixed cost.**  For small numbers, getting the operands in and the result out costs as much as the arithmetic:

* Each call saves the 16 bytes of the zero page it borrows ($70-$7F) and puts them back (about 350 cycles), whether the caller uses them or not (BASIC's zero page ends at $5F).
* Each operand is measured byte by byte (`nm_measure`, through `c_get`), then copied into the arena: about 550 cycles an operand even when it's two bytes.
* **BASIC's operands are in the scratch bank** (`NS_A`, `NS_B` at $8000), so the library reads each byte with a bank switch on either side (`c_get`'s slow way: about 65 cycles a byte, against some 16 for a straight copy below $8000), and writes the result the same way (`c_put`).  A fixed-decimal `NUM_ADD` from BASIC: 1,670 cycles to get its two operands in, about 500 to get the result out, about 500 for the call's start and end, against 3,230 for the work.
* **BASIC's own side** costs about 2,900 cycles more an operation: each operand copied from the heap (far memory, a byte at a time) into the scratch bank (590 each), and each result not an integer of 32 bits given a heap block of its own and copied there (1,370).
* `XCALL` itself is cheap: about 110 cycles.

**2. Small numbers go the general way.**  Every operand is loaded into a register (R0-R7), a sum of fixed decimals goes through the tower's scaling (`n_scaled`: 1,250 cycles to load two one-byte operands), and the result's trailing zeros are looked for by copying it and dividing the copy by 10 bit by bit (`n_fix0`: some 800 cycles, most often for nothing).  The work of `1.5 + 2.25` is 3,230 cycles; done in 32 bits it is under 400.  `1/3 + 1/7` spends 3,450 cycles in the general binary gcd (`n_gcd`: a shift of a register and its length kept at every step) to find that 10/21 is in lowest terms.  `INT(1.5)` divides 15 by 10 bit by bit (1,310 cycles) after building 10 as a register (`n_load`, 920).

**3. The integer primitives.**  `nmreg.inc`'s routines are general and correct, but slow at their core:

* **Division is a bit at a time** (`r_divmod`): for each quotient bit the remainder is shifted, compared and subtracted across the divisor's bytes.  60 digits by 30: 225,000 cycles.  It is 66% of `SQR` (Newton's square root divides once a step), 59% of hylang's big division, and it's under every fraction's reduction (`n_rdiv`), every `FLOOR`/`INT` of a fixed decimal, and the math functions' `m_fixed`.
* **Division by a byte is a bit at a time** too (`r_divsmall`, about 200 cycles a byte): the math series' terms (`m_div16`, `m_divpair`), `n_fix0`'s test, and `DISPLAY`'s every two digits (38% of `to-str`).
* **Multiplication is about 135 cycles a byte pair**: a `JSR` to `mul8` (the quarter squares, 85 cycles with the call) for each pair, then the carry and the add into the product.  It's 63% of `EXP`'s time, 40-45% of `SIN`'s and `LOG`'s.
* **Bookkeeping**: each `r_add` on two 10-byte registers takes 900 cycles, most of it zero-extending both to the same length (`r_maxlen`, `r_zext`), finding the pages again (`r_abc`, `r_page`: 17 cycles a call, thousands of calls) and normalizing; an `RMULW` multiplies in full and shifts the low half away (`m_shrw`: 870 cycles a time), so half of each product is made only to be thrown away.

**4. Text.**

* **Every `PARSE` and `DISPLAY` parses the base string again** (`b_state`, `b_parse`, `b_named`, then `b_finish`'s case check of every pair of digits, then an 88-byte copy, `b_cur`): about 7,000 cycles a call before a digit is read, 40% of `VAL("123.25")`.
* **`DISPLAY` divides the whole number by 100 for each two digits** with the bit-at-a-time small division: quadratic, with a large constant.
* **`PARSE` multiplies by 10 and adds a digit**, the whole number each time (`r_mulsmall`, `r_uadd`, `r_zext`): 72,700 cycles for 30 digits.

**5. The math functions.**

* **The constants are made on first use, in every program** (the library's bank is the program's): log 10 by series costs 1.33 million cycles, pi 0.79 million, log 2 0.88 million: the first `EXP` of a program takes 0.37 s more, the first `SIN` 0.22 s.
* **The rounding costs some 43,000 cycles a call** (`m_round`), whatever the function: two `m_sig`s (each 15,500), each making 10^s again by multiplications by 100 (`r_pow10`, 3,500 each), and 10^(n-1), 10^n again; and `m_log10x` multiplies by 77 by adding 77 times (2,800 cycles).  That is 23% of `SIN`, 30% of `LOG`, 34% of `ATN`.
* **The cores multiply at their full width**: `EXP` squares its sum eight times at the core's width (b + 24 bits), each multiply made in full and shifted down.
* **The square root** is Newton's from a power of 2 above it, each step a full division bit by bit: 72% of `SQR`.
* **`ATN` makes atan(1/m) by series in each call** (`m_atan_inv`: 46,000 of its 124,000).

**6. BASIC's own.**  These aren't the libraries', but the numbers work made them:

* **`\` and `MOD` on integers call the library** (`NUM_IDIV`, 4,940 cycles a call): 43% of `collatz`.
* **An array element's index** is multiplied by each dimension's count as it goes, the first too, where it multiplies 0 (`mul_cnt`, `mul16_t4`: 16 steps a bit at a time): 15% of `queens`, whose arrays have one dimension.
* **An empty `FOR` loop's pass is 650 cycles**: the slots looked up again for each of the add and the test (`slot_at` five times).

**7. hylang's own.**  Its collector takes 17-34% of number-heavy loops (`fact60` 34%, `bigadd` 27%): each number is a cell and a blob.  That's hylang's heap, not the numbers'; noted, and left to its own plan (step 9).

**8. Outside the numbers.**  The scheduler's task scan (`K_SCHED_PICK`) is about 5% of every profile, whatever runs: the kernel's, not this plan's.

### **The plan**

Nine steps, each measured before and after with the same workloads, each leaving every result as it was.

**Step 0: the tools.**  `nprof.js` into `sim/tools` (the profiler above, documented), and `tests/speed/`: the workloads above as fixed programs (`numb.bas`, `numh.hl`, the small loops, the math functions), with a script that runs them all and prints one table, cycles a call or a pass, in each language; the table goes in `status.md` after each step, so each step's gain is on record.

**Step 1: the quick wins.**  Each small, each in the libraries' banks as they are:

* **The constants from the ROM**: pi, log 2 and log 10 at 832 bits (the cache's size, `CONST_BYTES`), made by `numref.js` when the library is built and copied into the state by `INIT` (312 bytes).  The first call costs what every call does.
* **The rounding's own costs**: `m_log10x` by shifts (77 = 64 + 8 + 4 + 1); 10^(n-1) and 10^n kept in the RAM bank's state for the precision (made again when `DIGITS` changes), and the last 10^s `m_sig` made (its two calls in a rounding almost always want the same one).
* **The base kept parsed**: `SET_BASE` (and `INIT`) parse the base into the state once; `PARSE` and `DISPLAY` with `r4 = 0` (the state's base) use it as it is, and read `w_` through the parsed copy without the 88-byte copy.  A call's own base string (`r4`) is parsed as now.
* **`n_fix0`'s test without a division**: a number is divisible by 10 only if it's even and its bytes' sum is divisible by 5 (256 is 1 more than a multiple of 5); divide only then.
* **BASIC: `\` and `MOD` on integers in the interpreter** (as `+`, `-` and `*` are: 32 bits, the library past them), and an array's first index taken as it is (no multiplication of 0 by its count).

**Step 2: the room, and the call.**

* **A second bank for each library.**  `numbers` has 1,250 bytes left in its bank, `math` 450; the steps after need more (code, and 2-3K of tables).  The code seldom called moves to a second bank (`FORMAT`, the bits, `RANDOM`, the complex numbers' machine, `BYTES`), entered through a small trampoline in the library's RAM bank (put there by `INIT`) or through `XCALL` with r14 and r15 kept.  The hot path stays in the first bank.
* **Operands in by one pass**: an operand below $8000 measured and copied together, straight (`m_copy_in`'s way); one in the caller's bank copied in runs through the stack page, a bank switch a run, not two a byte.  The result out likewise.
* **The zero page kept only if the caller needs it**: an option in the library's state, set once by a program that leaves $70-$7F to the libraries (a new entry, `NUM_OPTIONS`, or a flag to `INIT`).  BASIC sets it (its zero page ends at $5F); hylang can't (its zero page is full); C and HyForth as their zero pages allow.  Without it, the library saves and restores the zero page as now.  (Not a flag in r13's high byte: `NUMCALL` sets only its low byte, so its high byte is whatever the caller left there.)
* **BASIC's operands and results below $8000**: a buffer of 64 bytes each for the two operands and the result in BASIC's RAM (192 bytes of the value stack's room; almost every number fits); the scratch bank only past that (the library's `NE_ROOM`, then again with the scratch bank's room).  An operand that is a `VT_INT` is already made below $8000 (`nia`, `nib`).

**Step 3: small numbers' own way, in the library.**  When both operands are integers or fixed decimals whose digits fit in 32 bits, the arithmetic is done there and then in 32-bit words in the zero page, without the registers, and the result written straight to the caller in the stored format:

* `ADD`, `SUB`: the places aligned (a times 10^k, from a table, its overflow checked), added, the trailing zeros dropped (the test of step 1);
* `MUL`: a 32 by 32 product (64 bits), the places added;
* `CMP`, `NEG`, `ABS`, `SIGN`, `FLOOR`, `CEILING`, `ROUND`, `TRUNCATE`: in place;
* `DIV` of two such numbers (a fixed decimal's places moved to the other side): whole (divisible) or a fraction, reduced by a binary gcd on 32-bit words;
* `ADD`, `SUB`, `MUL`, `CMP` of fractions whose numerators and denominators fit in 16 bits: in 32 bits, reduced the same way.

Anything past 32 bits on the way, or any other kind, goes the general way, as now.  The kinds and rules are the tower's (a sum of a fixed decimal and an integer is a fixed decimal; a quotient is exact): the same results, from shorter code.

**Step 4: the integer primitives** (`nmreg.inc`, both libraries).

* **Division a byte at a time** (Knuth's algorithm D): the divisor shifted so its top byte is 128 or more, each quotient byte estimated from the remainder's top two bytes (and checked with the divisor's second), the divisor times it taken off in one pass, added back in the rare case it was one too many.  A quotient byte costs about one row of a multiplication, not eight passes of shift, compare and subtract.
* **Multiplication a row at a time by table pointers**: for each byte of one factor, four zero-page pointers into the quarter-square tables (`sq_lo`, `sq_hi`, and two more, of f(255 - x), 1K) so that a byte pair's product is four `(zp),y` loads and two subtractions, and the product accumulated through a pointer: about 75 cycles a pair, from 135.  A square makes each cross product once and doubles it (55% of the pairs).
* **Division by a byte unrolled** (about twice as fast), and by 10 and 100 from tables (a byte of quotient and remainder from the remainder and the next byte: some 30 cycles a byte, from 200).
* **The bookkeeping trimmed**: `r_page` inlined; `r_add` and `r_sub` adding the shorter's bytes and then carrying, not zero-extending both; `r_copy` by pages.

**Step 5: the math functions' own.**

* **Truncated products**: an `RMULW` makes only the product's columns above the core's guard bytes (about 55% of the pairs); each core's error bound counted again with the truncation's error (a few ulps of w, under the guard bits), in `numref.js`'s mirror first.
* **The square root**: Newton's from a first guess good to 16 bits (from the top two bytes), so three or four steps, not eight, each with step 4's division; or, if it's quicker for the precision's sizes, the root found a bit at a time (no division at all).
* **`EXP` without its squarings**: exp(r) = exp(j/64) exp(r - j/64), j from a table of exp(j/64) (j to 147: up to log 10) at 128 bits (2.4K); the series then needs no squaring.  Past the table's bits (a precision past about 25 digits), the way it is now.
* **`ATN`'s atan(1/m)** (m 2 to 15) from a table at 128 bits, likewise.
* **One `m_sig`**: the digits rounded from the middle of the interval, then each end only compared with the rounding's limits (a multiplication each, not a rounding each).
* **The powers of 10 from a table** to 10^38 (`r_pow10` a copy, not multiplications by 100), in both libraries' first banks once step 2 has made the room.

**Step 6: text.**

* **`DISPLAY`** by 10,000 at a time (four digits for each pass over the number, with the table division of step 4 twice), and a short number (four bytes or less) in 32 bits without the registers.
* **`PARSE`** four digits at a time (the digits made a 16-bit number, the whole multiplied by 10^4 once), and a short number in 32 bits.
* The same for the digits of a fixed decimal and of a fraction.

**Step 7: BASIC's small decimals in the value itself** (a question: see below).  A fixed decimal whose digits fit in 32 bits held in its 5-byte value as an integer is (`VT_FIX`: its type byte says its places), not in the heap; `+`, `-`, `*` and the comparisons on two of them, or one and a `VT_INT`, done by the interpreter as it does integers, the library only past 32 bits; a `FOR` loop's `STEP 0.1` the same.  Its results are the library's, to the digit: what changes is where the number is kept.  `X = X + 0.1` would go from 10,000 cycles a pass to about 1,700, and no heap block for each result.

**Step 8: BASIC's interpreter** (a question: below).  The numbers work's own BASIC is 4.0 times hylang's time over the twenty benchmarks.  The plain ways: `FOR`/`NEXT` with its slots found once (an empty pass 650 cycles to about 350), `LDV`/`STV` of a global without the slot's test, the value stack's helpers inline in the hottest ops, `\` and `MOD` (step 1).

**Step 9: hylang's numbers' memory** (a question: below).  A number of up to 6 bytes in a cell of its own, no blob; fewer blobs, fewer and shorter collections.

### **What to expect**

Estimates from the cycles counted above and the new code's instruction counts; each step measures its own.  ms at 3.58 MHz; "after" is after steps 1-6, and step 7 for BASIC's decimals where it's marked.

`numb.bas` (BASIC):

| Work | Now | After 1-6 | With step 7 |
| :--- | ---: | ---: | ---: |
| 2,000 integer additions | 785 | 785 | 785 |
| 500 additions of 0.1 | 1,400 | 600 | 250 |
| 300 of i / 7 summed | 2,725 | 850 | 850 |
| 300 of * 1.5, / 1.5 | 1,865 | 800 | 550 |
| 100 `SQR` | 4,795 | 900 | 900 |
| 50 `SIN` | 3,020 | 1,100 | 1,100 |
| 50 `EXP` | 4,760 | 1,600 | 1,600 |
| 50 `LOG` | 2,465 | 950 | 950 |
| 300 `STR$` | 1,960 | 700 | 600 |
| 300 `VAL` | 1,840 | 500 | 500 |

`numh.hl` (hylang; its collector as it is):

| Work | Now | After 1-6 |
| :--- | ---: | ---: |
| 60! twenty times | 6,265 | 5,500 |
| 300 big integers' additions | 2,020 | 1,850 |
| 100 products of 30 digits | 1,485 | 1,150 |
| 100 big divisions | 8,910 | 2,900 |
| 1/1 to 1/20 summed, 10 times | 3,320 | 2,300 |
| 300 of i/3 + i/7 summed | 7,235 | 4,800 |
| 300 additions of 0.1 | 1,330 | 1,050 |
| 100 `to-str` of 2^100 | 1,960 | 1,050 |
| 100 `val` of 30 digits | 2,540 | 950 |

A call of the library alone, now and after:

| Call | Now | After 1-6 |
| :--- | ---: | ---: |
| `EXP` (12 digits) | 290,000 | 110,000 |
| `SIN` | 185,000 | 80,000 |
| `LOG` | 142,000 | 65,000 |
| `SQR` | 127,000 | 30,000 |
| `ATN` | 124,000 | 45,000 |
| `NUM_ADD` of 1.5 and 2.25 (BASIC's) | 6,180 | 1,300 |
| `NUM_DIV` of `I` by 3 (BASIC's) | 10,060 | 2,500 |
| `NUM_ADD` of 1/3 and 1/7 (BASIC's) | 10,890 | 2,500 |
| `NUM_MUL` of two 30-digit integers | 30,400 | 19,000 |
| `NUM_DIV` of 60 digits by 30 | 224,600 | 20,000 |
| `NUM_DISPLAY` of 2^100 | 45,400 | 12,000 |
| `NUM_PARSE` of 30 digits | 72,700 | 15,000 |
| `VAL("123.25")` (BASIC's call) | 17,500 | 4,000 |

So: the math functions 2 to 4 times as fast (the square root 4), and no 0.2-0.4 s on a program's first one; the library's calls on small numbers 4 to 5 times; a big division 10 times; text 4 to 5 times.  A whole BASIC program's decimal arithmetic about 2.5 times as fast (6 with step 7); hylang's programs less, as its own time (its collector above all) stays.  The first-call cost and `VAL`'s base come from step 1 alone.

### **The order of work**

On a branch, `reborn-numspeed`, merged into `reborn` when it's done (or after any step the user wants sooner).

| Step | What | Size |
| :--- | :--- | :--- |
| 0 | The profiler in `sim/tools`, `tests/speed/`, the table in `status.md` | small |
| 1 | Constants in ROM, the rounding's costs, the base kept parsed, `n_fix0`'s test, BASIC's `\` `MOD` and arrays | small, high value |
| 2 | Each library a second bank; operands in one pass; the zero page flag; BASIC's buffers below $8000 | medium |
| 3 | Small numbers' own way (32 bits) | medium |
| 4 | Division by bytes, multiplication by rows, division by a byte, the bookkeeping | medium, the most careful |
| 5 | Truncated products, the square root, `EXP`'s and `ATN`'s tables, one `m_sig` | medium |
| 6 | `DISPLAY` and `PARSE` four digits at a time | small |
| 7 | BASIC's `VT_FIX` (if the user wants it) | medium |
| 8 | BASIC's interpreter (if wanted) | medium |
| 9 | hylang's short numbers in cells (if wanted) | small to medium |

Each step: the numbers library's test (its calls checked against `numref.js`), `numxcheck.js` and `hyxcheck.js` (random numbers and expressions, and the math functions, against danlang), BASIC's suite (495 checks), all 120 tests, the browser emulator's check; then the speed table.  Step 4's primitives are checked first on their own, by a test of random operands of every length (1 to 255 bytes) against JavaScript's `BigInt`, before anything uses them.

### **Risks**

* **A result that differs.**  The point of the whole plan is that none may.  The fast ways (steps 3, 6, 7) each fall back to the general way at the first doubt (anything past 32 bits, any kind they don't handle), and each is cross-checked against the general way on random operands as well as against danlang.
* **The math functions' error bounds** (step 5): a truncated product is no longer exact, so each core's bound must be counted again; a bound too small would round wrongly, rarely and silently.  The bounds are worked in `numref.js` first and checked by running each core at the edge of its bound with random arguments, as step 4 of NUMBERS.md was.
* **The second banks** (step 2): a new way into a library's second bank (the trampoline in the RAM bank, or `XCALL`), and two libraries (`numbers`, `math`) that share the RAM bank's layout, so they must agree on where anything put there is.
* **The zero page option** is an addition to the libraries' interface: a program that doesn't set it gets what it gets now; one that sets it and then uses $70-$7F across a call would lose them, so it's the program's promise, as BASIC's zero page map makes it.
* **`VT_FIX`** (step 7) is a new kind of value in BASIC: everything that asks "is it a number" must say yes to it (13 places name `VT_NUM` now, most of them in `num.inc`), and arrays, records, files and `PRINT` must take it.

### **Questions**

1. **Step 7, BASIC's `VT_FIX`**: small fixed decimals kept in the value, worked by the interpreter (BASIC's decimals 6 times as fast as now, against 2 to 3 times without it).  Yes, or the libraries' gains alone?
2. **Steps 8 and 9**: BASIC's interpreter, and hylang's numbers in cells: in this plan, or each its own later?
3. **The tables in ROM**, about 5.5K in all, some in both libraries (step 4's quarter squares, 1K, and its division by 10 and 100, about 1.2K; step 5's exp(j/64) and atan(1/m), about 2.6K; the constants, 312 bytes; the powers of 10, about 400 bytes): fine as ROM space, or keep to code alone where the gain is smaller?
4. **The zero page option** (step 2): a new entry (or a flag to `INIT`) by which a program tells the libraries their zero page needn't be kept, or keep saving it always (about 350 cycles a call)?
5. **Merging**: once at the end, or after each step that's done (step 1's gains are the cheapest and among the most visible)?
