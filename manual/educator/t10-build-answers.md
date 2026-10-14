# Topic 10. The Build Log

Most of the build log is a record, not a test: ticks, readings and dates. These are the answers to the questions
it does ask, and what to look for in the rest.

## Unit 1. Before You Build

**Which solder, and what does that change?** Lead-free: a hotter iron (about 370 °C), joints that look a little
duller even when they're good, and slightly slower wetting. Solder with lead: an easier joint, and stricter
hygiene: no food or drink, and hands washed without fail. Either answer is right if it matches what's on the
bench.

**Iron temperature.** About 350 °C for tin-lead solder, about 370 °C for lead-free. A student who doesn't know
hasn't looked.

**A good joint and a bad one.** Good: a small, shiny cone or volcano, rising smoothly from the pad up the lead,
wetting both. Bad: a ball sitting on the pad (the pad wasn't hot); a dull, grainy lump (a cold joint: it moved
as it cooled, or wasn't hot enough); a blob joining two pads (a bridge); a hole round the lead (too little
solder). Look for a side view, and for *both* the pad and the lead being mentioned.

**Brown, black, orange, gold.** 1, 0, then three zeros: 10,000 ohms, or 10 kΩ. Gold is the tolerance, 5%.

**104, and 330.** Two digits, then a count of zeros, in picofarads. 104 is 10 and four zeros: 100,000 pF, which is
100 nF. 330 is 33 and no zeros: 33 pF. (A student who says 330 pF has read it as a plain number: the commonest
mistake.)

**Pin 1.** On a chip: beside the notch at one end, or marked by a dot; pins are counted anticlockwise from there,
seen from above. On a socket: at the notched end. On a header: the square pad on the board.

**Parts that must go the right way round.** Electrolytic capacitors (except the two non-polarized ones); every
chip and its socket; resistor networks; the reset chip, U43; LEDs; and the keyed connectors (the power connector,
by its latch). Resistors, ceramic capacitors and the crystal go either way.

**The kit count** has no right answer but the kit's own. The point is that it's done before the iron is on, and
that anything missing is written down.

**The plan.** Look for sessions short enough to be real (one or two hours), a total somewhere near twenty hours,
and Step 4 spread over several sessions. Come back to this table at the end: the comparison of "planned" and
"took" is one of the more useful things the build teaches.

## Unit 2. Soldering the Main Board

The ticks are the record. The questions:

**Where I stopped (Step 4).** Any note that would let someone else carry on: "sockets done to U24; U26 has one
corner tacked".

**What did the inspection find?** Something, always. An empty answer means it wasn't done. Typical finds on a
first board: three to ten unsoldered pins (most often on the long sockets and the slots), one or two bridges, a
few dull joints to reflow, and now and then a socket the wrong way round. A student who lists what they found,
and what they did about each, has done this page properly.

## Unit 3. Power, ROMs and Chips

**The resistance from +5 V to ground.** More than about 100 ohms, usually a few hundred to a few thousand, and
climbing as the meter charges the capacitors. Near zero is a short: almost always a solder bridge, sometimes an
electrolytic capacitor in backwards.

**The voltages.** +4.75 to +5.25 V on U1's pin 8; about +12 V and −12 V on U42's pins 4 and 11. A missing ±12 V
with a good +5 V means the supply's own rails, or the joints at J11: the Hydra will still start without them
(they feed only the sound circuit and the slots), which makes it a fault that hides.

**Which programmer, and when were the images built?** A record, for the day the ROMs are programmed again. What
matters is that it's filled in.

**DS1747 or HM628512? R65C51 or W65C51N?** Whatever is fitted. It matters later: without a DS1747 the date
starts at 2000 every time; and a WDC W65C51N needs its own kernel ROM, `bios_wdc.bin` (the manual's section 4.4).

**The drawing of J10.** Two rows of six, pin 1 at the square pad; the switch across 11 and 9; the reset button
across 7 and 5; the LED from 12 (its longer leg) to 10.

## Unit 4. The Memory Card

**How J1 and J2 were kept straight.** The manual's method: push the headers' long pins into the main board's own
J1 and J2 sockets, lower the card over their short ends, and solder from the top. Any method works that ends
with the pins square to the card, but only this one guarantees they fit the sockets.

**The module numbers.** 0, for a first card. With more than one module, all different.

**What `RAM modules:` will print.** `01`, for one module; in hex, so eleven would be `0B`.

## Unit 5. First Switch-On

**The start-up lines.** For a board that works, with one module:

```text
Hydra-16: kernel 0.1, ABI 1
POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0
RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000
POST ok
RAM modules: 01
```

Zeros everywhere (and `SH:S`) mean no faults. The manual's section 7.4 explains every field, and section 5.7 says
which chip each one points at.

**`date`.** `2000-01-01` and a few seconds, without a clock chip.

**`free`.** `128 KB a task (1 module)` for one module, 256 KB for two, 384 KB for three; and
`shared  1024 KB, 512 KB in segments (1), 512 KB free`.

**First joint to first prompt, against the plan.** Nearly every build takes longer than planned, mostly in Step 4
and in fault-finding. The good answers say *where* the plan was wrong.

**When it finally worked: what was the fault, and how could you have caught it sooner?** The most valuable
answer in the build log. "A chip in backwards; I'd have caught it if I'd checked every notch before the power
went on" is the kind of thing to hope for. Read these aloud to next year's class.

**The hardware test.** Every line `ok`, with the CPU clock near 3.58 MHz, the SD card listed if there is one, and
"no devices" and "all empty" for the I²C bus and the slots on a bare Hydra. The manual's section 5.7 shows a
whole passing run.
