# Topic 6. Running a Build

A Hydra is 2,234 solder joints on the main board, 240 on a memory card, 59 chips to seat, and a day or two of
fault-finding. One determined adult can do it in a week of evenings. A class can do it in a term. This topic is
about the term.

## Is the class ready?

Don't begin on the Hydra. Begin on something that doesn't matter.

| Before the Hydra | Why |
|---|---|
| Every student has made twenty joints on scrap, to the standard of the manual's Figure 3-3 | The Hydra's board is not for learning on |
| Every student has cleared a bridge, and removed a part, on scrap | They will need to, and the first time shouldn't be on a 62-pin slot |
| Every student has signed the Companion's safety rules (Unit 1), and so has an adult | Topic 4 |
| You have built a Hydra yourself, or at least soldered a hundred of its joints | You can't supervise what you haven't done |

An inexpensive "learn to solder" kit each, built in the week before, does all of the first three rows, and gives
every student something to take home.

## Sharing one board

A board has room for one iron at a time. So a build shared by a class is a relay, and the question is how to
make the relay fair, and how to make the board come out working.

**The team.** Two or three students to a board is ideal. With one board for a class, run it as shifts of three:
one soldering, one placing parts and holding the board, one reading the manual aloud and ticking the Companion.
Rotate the three jobs every fifteen minutes, on a timer.

**The reader matters as much as the solderer.** The commonest cause of a dead Hydra isn't a bad joint: it's a
wrong part in the right place. The student with the manual, checking each value against the table before it
goes in, is the quality control. Say so, often.

**One log for the board.** If several groups share one build, the Companion's Units 2 to 5 are kept as *one*
copy, which lives with the board: who fitted what, on which day, checked by whom. Each student still keeps their
own logbook.

## The order of work, with a class

The manual's order (lowest parts first) is right. Here it is as sessions of 90 minutes, with what to watch at
each. The bench time is about 20 hours for a first build; with a relay, allow twelve sessions.

| Session | What gets done (the Companion's Unit 2) | Joints | Watch for |
|---|---|---|---|
| 0 | Practice kits; safety; the kit counted (Unit 1) | | The kit's count: find missing parts *now* |
| 1 | Step 1, by an adult: the four surface-mount RAM chips. Step 2: resistors | 210 | Resistor values: have every one read aloud, and measured if in doubt |
| 2 | Step 3: the 78 ceramic capacitors | 156 | Tedium. This is the session where joints get careless. Short shifts |
| 3 to 6 | Step 4: the 64 sockets | 1,282 | Orientation, every single socket. A socket in backwards still works, but the chip then goes in backwards. Unsoldered pins: a 40-pin socket with 39 joints looks finished |
| 7 | Step 5: the crystal, the reset chip, headers and pin sockets | 148 | J1 and J2 dead straight. U43 the right way round |
| 8 | Step 6: the electrolytic capacitors. Step 7 begun: J3, J26, J11 | 66 | **Polarity.** Stop the room, and check all eleven polarized capacitors together before any is soldered |
| 9 | Step 7: the six slots | 372 | Slots sitting flat. Bridges between the slots' close pins |
| 10 | Step 8: inspect and clean. The first power test (Unit 3) | | Everyone inspects. You do the power test, with glasses on and faces back |
| 11 | The memory card (Unit 4). Chips in (Unit 3) | 240 | The card's J1 and J2 upside down, soldered in place on the main board. Chip families: 74F is not 74LS |
| 12 | First switch-on (Unit 5) | | See below |

## The surface-mount chips

The four shared RAM chips, U25 and U27 to U29, have 32 legs each at a spacing that needs flux, a clean fine tip,
magnification, and a steady hand. They're also Step 1, when the class has had the least practice.

Do them yourself, before the first session, or have them done. It isn't cheating. It's the difference between a
board that starts and a board that teaches despair. A confident older student can do the fourth one, watched,
once the first three are down.

## The first switch-on

Make an occasion of it. Everyone who worked on the board should be in the room.

1. **Before the power:** the Companion's "Before the switch" list, read aloud, each item answered by a named
   student who goes and looks.
2. **The terminal first,** on the projector, at 9600 baud. Then the power.
3. **If it starts:** let the students read out the messages. Type `free`. Then hand the keyboard to the class.
4. **If it doesn't,** and it very often doesn't: that's the next two lessons, and they're good ones. Say out loud,
   right away, that this is normal and expected, and that now the interesting part starts.

## Fault-finding as a lesson

A board that doesn't start is the best teaching material you'll have all year. Treat it that way.

* **One fault diary, on the board** (the Companion's Unit 5 has one; copy it large). Every check that anyone makes
  is written up, with what was found. Nothing is tried twice. Nothing is tried without being written down first.
* **One change at a time.** The student who reseats six chips "to be safe" has destroyed the evidence. Make the
  rule explicit.
* **Divide the problem.** The manual's section 5.7 is in the right order: power, then the serial line, then the
  chips the Hydra needs to start, then the address decoder, then joints. Have the class say what each check
  *rules out*.
* **The usual suspects**, roughly in the order they turn out to be the answer: a chip in the wrong socket or the
  wrong way round; a bent leg folded under a chip; a socket pin never soldered; a solder bridge on a slot or a
  socket; the wrong serial adapter (TTL instead of RS-232) or the wrong baud rate; a clock jumper missing or
  doubled; a ROM chip not programmed, or in the wrong socket.
* **When it starts but `POST` reports faults,** the numbers on the `POST` and `RAM` lines point at a chip: the
  manual's section 5.7 has the table, and section 7.4 explains every field. This is where the students who read
  Chapter 7 properly get to be the experts.
* **Then the hardware test** (`hwtest`, or <kbd>T</kbd> held at reset), which tests each part in turn and names
  what fails.

Budget two sessions for faults. If the board works first time, spend them on the SD card reader and Lab 14.

## What the students keep

A single class Hydra belongs to the room. That's a pity, and worth softening:

* A photograph of each student with the board, at the first prompt.
* Their name on a label under the board, or in a file on its SD card that says who built it.
* The Companion's signed "this Hydra started on" line.
* Their own SD card, or card image, with their work: the same files run on the emulator at home.

## A build without a class

For a club (Topic 2's Shape E) or a single group, drop the sessions and keep the rules: practise first; one
person reads while another solders; tick the Companion as you go; stop fifteen minutes before you have to; inspect
with someone else's eyes; and write the fault diary even when you're sure you'll remember.
