# Unit 2. Soldering the Main Board

> **THIS STAGE:** Manual: section 4.2, with Chapter 3 open beside it. Seven steps of soldering and one of checking:
> 216 parts and 2,234 joints, in about ten sessions.

For each row: tick **Fitted** when every part in the row is soldered in, and **Checked** when you've looked at
every one of its joints with a magnifier (section 3.6). At the end of each step, get someone else to look too. A
second pair of eyes finds what yours have stopped seeing.

## Step 1: The shared RAM chips

128 joints. Section 3.7 shows how to solder surface-mount chips. If your board came with these already fitted,
tick them as checked, and go on.

| Fitted | Checked | Part | What to check |
|---|---|---|---|
| ☐ | ☐ | U25 | Pin 1 at the dot; no bridges between legs; every leg down on its pad |
| ☐ | ☐ | U27 | The same |
| ☐ | ☐ | U28 | The same |
| ☐ | ☐ | U29 | The same |

<p class="fields">Step 1 finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>

## Step 2: The resistors and resistor networks

82 joints. Resistors go in either way round. A resistor network has a dot at pin 1, which goes to the square pad.

<!-- generated: Step 2/1 -->

| Fitted | Checked | Value | Code | Parts | How many |
|---|---|---|---|---|---|
| ☐ | ☐ | 22 Ω | 220 | R23, R24, R25, R26, R27 | 5 |
| ☐ | ☐ | 33 Ω | 330 | R30, R31 | 2 |
| ☐ | ☐ | 100 Ω | 101 | R3 | 1 |
| ☐ | ☐ | 330 Ω | 331 | R22 | 1 |
| ☐ | ☐ | 470 Ω | 471 | R2 | 1 |
| ☐ | ☐ | 10 kΩ | 103 | R4, R5, R6, R7, R13, R19, R20, R21, R28, R32, R33 | 11 |
| ☐ | ☐ | 1 MΩ | 105 | R1 | 1 |
| ☐ | ☐ | 0 Ω | 0 | R29 | 1 |

<!-- end generated: Step 2/1 -->

<!-- generated: Step 2/2 -->

| Fitted | Checked | Value | Code | Parts | How many |
|---|---|---|---|---|---|
| ☐ | ☐ | 3.3 kΩ | 332 | RN1, RN2, RN3 | 3 |
| ☐ | ☐ | 10 kΩ | 103 | RN4 | 1 |

<!-- end generated: Step 2/2 -->

<p class="fields">Step 2 finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>

## Step 3: The ceramic capacitors

156 joints. They go in either way round. Most of them are the same value, so do the odd ones first, while they're
easy to tell apart.

<!-- generated: Step 3/1 -->

| Fitted | Checked | Value | Code | Parts | How many |
|---|---|---|---|---|---|
| ☐ | ☐ | 33 pF | 330 | C47, C48 | 2 |
| ☐ | ☐ | 3.3 nF | 332 | C50, C52 | 2 |
| ☐ | ☐ | 100 nF | 104 | C1, C2, C3, C5, C6, C12 to C46, C57 to C64, C66 to C85 | 68 |
| ☐ | ☐ | 1 µF | 105 | C4, C7, C8, C9, C10 | 5 |
| ☐ | ☐ | 4.7 µF | 475 | C49 | 1 |

<!-- end generated: Step 3/1 -->

<p class="fields">Step 3 finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>

## Step 4: The chip sockets

1,282 joints: more than half of the board. Every socket has a notch at one end, which must match the notch in its
outline on the board. Solder one corner pin, check that the socket is flat and the right way round, then the
opposite corner, then the rest.

<!-- generated: Step 4/1 -->

| Fitted | Checked | Socket | How many | Places |
|---|---|---|---|---|
| ☐ | ☐ | 8 pins | 1 | IC9 |
| ☐ | ☐ | 14 pins | 15 | U8, U9, U10, U11, U12, U13, U14, U15, U16, U39, U42, U44, U56, U57, U61 |
| ☐ | ☐ | 16 pins | 19 | IC1 to IC8, U4, U5, U17, U18, U20, U24, U26, U40, U41, U45, J28 |
| ☐ | ☐ | 20 pins | 13 | U21, U22, U48, U49, U50, U51, U52, U53, U54, U55, U58, U59, U60 |
| ☐ | ☐ | 24 pins (wide) | 3 | U19, U23, U38 |
| ☐ | ☐ | 28 pins (wide) | 1 | U3 |
| ☐ | ☐ | 32 pins (wide) | 9 | U7, U30 to U37 |
| ☐ | ☐ | 40 pins (wide) | 2 | U1, U2 |
| ☐ | ☐ | PLCC 32 (square) | 1 | U6 |

<!-- end generated: Step 4/1 -->

✎ This step takes several sessions. Keep a note of where you stopped each time.

<div class="write l3"></div>

<p class="fields">Step 4 finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>

## Step 5: The crystal, the reset chip, the headers and the pin sockets

148 joints.

| Fitted | Checked | Parts | What to check |
|---|---|---|---|
| ☐ | ☐ | Y1, the crystal | Soldered quickly; its can not touching any pad |
| ☐ | ☐ | U43, the reset chip | Its flat side matches the outline |
| ☐ | ☐ | J5, J6, J7, J8: the clock jumpers' headers | Standing straight |
| ☐ | ☐ | J9, J29 (2 pins each) and J30 (3 pins) | Standing straight |
| ☐ | ☐ | J10, the front panel, and J27, the I/O header (2 rows of 6 each) | Standing straight; the short pins through the board |
| ☐ | ☐ | J18 to J25: the eight SPI pin sockets | Flat on the board |
| ☐ | ☐ | J1 (32 holes) and J2 (24 holes): the memory card's sockets | Perfectly straight and flat: the card plugs into both at once |

<p class="fields">Step 5 finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>

## Step 6: The electrolytic capacitors

26 joints. ⚠ **These must go the right way round**, except the two marked NP. The longer leg is plus; the stripe on
the can marks minus. One fitted backwards can burst.

<!-- generated: Step 6/1 -->

| Fitted | Checked | Value | Code | Parts | Polarity | How many |
|---|---|---|---|---|---|---|
| ☐ | ☐ | 10 µF | 106 | C90, C91 | Yes | 2 |
| ☐ | ☐ | 47 µF | 476 | C86, C87, C88, C89 | Yes | 4 |
| ☐ | ☐ | 100 µF | 107 | C51, C53, C54, C55, C56 | Yes | 5 |
| ☐ | ☐ | 220 µF non-polarized | 227 NP | C92, C93 | No: either way | 2 |

<!-- end generated: Step 6/1 -->

| ☐ | Before soldering each one |
|---|---|
| ☐ | I checked every polarized capacitor's plus leg against the + on the board, twice. |

<p class="fields">Step 6 finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>

## Step 7: The big connectors

412 joints, 372 of them in the six slots.

| Fitted | Checked | Part | What to check |
|---|---|---|---|
| ☐ | ☐ | J3, the serial connector | Its two big lugs soldered, as well as its nine pins |
| ☐ | ☐ | J26, the audio jack | Flat on the board |
| ☐ | ☐ | J12, slot 0 | Its two end pins first; flat and straight; then all 62 |
| ☐ | ☐ | J13, slot 1 | The same |
| ☐ | ☐ | J14, slot 2 | The same |
| ☐ | ☐ | J15, slot 3 | The same |
| ☐ | ☐ | J16, slot 4 | The same |
| ☐ | ☐ | J17, slot 5 | The same |
| ☐ | ☐ | J11, the power connector | Its latch matches the outline; every joint full and shiny |

<p class="fields">Step 7 finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>

## Step 8: Inspect and clean

Go over the whole back of the board, slowly, with a magnifier and a good light. Work in strips, and tick each strip
when it's done, so that no part of the board is missed.

| ☐ | Strip of the board | Joints I repaired |
|---|---|---|
| ☐ | The top quarter | |
| ☐ | The second quarter | |
| ☐ | The third quarter | |
| ☐ | The bottom quarter | |
| ☐ | Every electrolytic capacitor's polarity, from the front | |
| ☐ | Every socket's notch, from the front | |
| ☐ | The flux cleaned off, and the board dry | |

✎ What did the inspection find? (Everybody finds something.)

<div class="write l4"></div>

> **SIGNED:** The board has been inspected, front and back, by its builder and by one other person. &nbsp; Builder:
> ________________ &nbsp; Second pair of eyes: ________________ &nbsp; Date: __________
