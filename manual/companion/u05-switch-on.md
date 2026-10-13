# Unit 5. First Switch-On

> **THIS STAGE:** Manual: section 4.9 and Chapter 5. The moment of truth. If the prompt appears, congratulations.
> If it doesn't, you're in good company: most home-built computers don't work the first time, and finding out why
> is where builders learn the most.

## Before the switch

| ☐ | Check (section 4.9) |
|---|---|
| ☐ | Every chip is in the right socket, the right way round, with no legs bent. |
| ☐ | The programmed ROM chips are in U6, U31, U32 and U34. |
| ☐ | One clock jumper, on J7. The J30 jumper on pins 1 and 2. |
| ☐ | The memory card is plugged in, with its module jumper set. |
| ☐ | The power supply is plugged into J11, and the power switch into J10. |
| ☐ | The SD card module, if there is one, is wired to J18 (section 14.2), with a card in it. |

## The serial line

| ☐ | Check (sections 5.1 and 5.2) |
|---|---|
| ☐ | The adapter says RS-232, and has a 9-pin plug. It isn't a "TTL" adapter, and there's no null-modem cable. |
| ☐ | I know the port's name on my PC. |
| ☐ | The terminal program is set to 9600 baud, 8 data bits, no parity, 1 stop bit, flow control XON/XOFF. |
| ☐ | The terminal program is running *before* I switch on. |

✎ The port's name, and the terminal program you're using:

<div class="write"></div>

## Switching on

Switch on, and copy down what the Hydra says. The lines below are the ones that matter (section 5.3).

| What the Hydra printed | Mine said |
|---|---|
| `Hydra-16: kernel` ... | |
| `POST ZP:` ... | |
| `RAM U:` ... | |
| `POST ok`, or `POST found faults:` ... | |
| `RAM modules:` ... | |
| `HydraOS 1.0 for the Hydra-16` | |
| The prompt, `/>` | |

Then two commands (section 5.4):

| Command | What mine said |
|---|---|
| `date` | |
| `free` | |

✎ How long was it from your first joint to your first prompt? How does that compare with your plan in Unit 1?

<div class="write l2"></div>

> **SIGNED:** This Hydra started on &nbsp; __________ &nbsp; Builder: ________________ &nbsp; Witness: ________________

## If it doesn't start: the fault-finding chart

Always switch off, and unplug, before you touch the board. Change **one thing at a time**, and write down what you
changed, in the fault diary on the next page. Section 5.7 has the details of every step here.

| What you see | Check, in this order | Tick |
|---|---|---|
| **No power light** | The supply's own switch, and its plug in J11 | ☐ |
| | The power switch on J10, pins 11 and 9: is it a switch that stays on? | ☐ |
| | The voltages, as in Unit 3. No +5 V: unplug, and look for a short | ☐ |
| **Light on, but nothing in the terminal** | The right port, at 9600 baud. Unplug the adapter, plug it in again, and see which port appears | ☐ |
| | The cable: straight-through, not null-modem; an RS-232 adapter, not a TTL one | ☐ |
| | Press reset. Do the messages come now? | ☐ |
| | Move the J30 jumper to pins 2 and 3 | ☐ |
| | Exactly one clock jumper, on J7 | ☐ |
| | The chips it needs to start: U1, U6, U3, U5, U39, U40, U43. Right socket? Right way round? No bent legs? | ☐ |
| | The address decoder chips: U4, U8 to U16, U19 to U22. Read each number against the board | ☐ |
| | The joints around those chips, and around J3 | ☐ |
| **Letters, but nonsense** | The terminal's speed: 9600 baud, 8N1 | ☐ |
| | The clock jumper: on J5 or J6, the serial port runs slow too | ☐ |
| **`POST found faults`** | Write down the whole `POST` and `RAM` lines. Each number that isn't 0 points at a chip: the table in section 5.7 says which | ☐ |
| **`RAM modules: 00`** | The card pushed fully into J1 and J2; a jumper on its J3; its RAM chips the right way round | ☐ |
| **It starts, but something doesn't work** | The hardware test: hold <kbd>T</kbd> and press reset, or type `hwtest`; then <kbd>A</kbd> for all the tests | ☐ |

## The fault diary

One line for each thing you try. Yes, even the ones that didn't help: they're how you know what the fault *isn't*.

| Date | What was wrong | What I checked or changed | What happened |
|---|---|---|---|
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |
| | | | |

✎ When it finally worked: what was the fault, and how could you have caught it sooner?

<div class="write l4"></div>

## The hardware test's results

Run the hardware test once your Hydra works, and keep the results. If something stops working months from now,
you'll know what "good" looked like (section 5.7).

| Test | Result | Test | Result |
|---|---|---|---|
| CPU | | BIOS ROM | |
| T U V W registers | | Paged ROM | |
| Shared RAM | | Interrupts | |
| RAM bank registers | | VIA | |
| Task RAM | | Sound chip | |
| RAM modules | | CPU clock | |
| Serial port | | SPI devices | |
| I2C bus | | Slot cards | |
