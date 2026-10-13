# Unit 4. The Memory Card

> **THIS STAGE:** Manual: section 4.7. The through-hole card: 13 parts and 240 joints, an evening's work. The
> Hydra needs at least one memory card (section 1.7): it's where each task's extra memory is.

## The through-hole card

| Fitted | Checked | Part | What it is | What to check |
|---|---|---|---|---|
| ☐ | ☐ | C1 to C5 | 100 nF ceramic capacitors (104) | Either way round |
| ☐ | ☐ | U1 | 74LS139 (or its 16-pin socket) | Its notch matches the outline |
| ☐ | ☐ | IC1 to IC4 | Four 32-pin wide sockets | Their notches match the outlines |
| ☐ | ☐ | J3 | Pin header, 2 rows of 15 | Long pins standing *up* |
| ☐ | ☐ | J1 | Pin header, 1 row of 32 | Long pins pointing *down*: soldered with the card sitting on the main board |
| ☐ | ☐ | J2 | Pin header, 1 row of 24 | The same |
| ☐ | ☐ | The four HM628512 RAM chips | In IC1 to IC4 | Notches matching; no legs bent |

✎ J1 and J2 go in upside down. In your own words, how did you keep them straight while you soldered them?

<div class="write l3"></div>

## The module number

| ☐ | Check |
|---|---|
| ☐ | A jumper is on J3, across its two rows, in column 0 (pins 1 and 2): this is module 0. |
| ☐ | If there's a second card or module, it has a *different* number. |

✎ The module numbers on your cards:

<div class="write"></div>

## Plugging it in

| ☐ | Check |
|---|---|
| ☐ | The power is off and unplugged. |
| ☐ | The card's J1 is over the main board's J1, and its J2 over J2. |
| ☐ | The card is pushed down evenly, all the way. |

✎ When the Hydra starts, it prints `RAM modules:` and a number, in hex. What do you expect yours to print?

<div class="write"></div>

<p class="fields">Card finished on <span class="short"></span> Hours <span class="short"></span> Looked over by <span></span></p>
