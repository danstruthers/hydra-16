# Project 6. A Traffic Light

> **THIS PROJECT:** rc, and the I/O header. After Lab 14. Two sittings. You need a real Hydra, three LEDs (red,
> amber and green), three 2.2K resistors, and wire. ⚠ Read the caution in section 17.1, and wire with the power off.
> (No real Hydra? See the last line of this page.)

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The circuit

Each LED goes from +5 V, through its own resistor, through the LED, to a pin: red to PA2, amber to PA3, green to
PA4. Wired this way round, an LED lights when its pin is **0**.

✎ Draw your circuit. Mark the header's pin numbers (the table in section 17.1), and which leg of each LED goes
which way.

<div class="space s3"></div>

| ☐ | Before the power goes on |
|---|---|
| ☐ | Each LED has its own resistor |
| ☐ | Nothing touches pins 3 and 4 of the header (PA0 and PA1: they're the I²C bus) |
| ☐ | Someone else has checked the wiring against the drawing |

### The starting point

Type this into `/ram/lights`, and run it with `rc /ram/lights`. <kbd>CTRL</kbd>-<kbd>C</kbd> stops it.

<!-- listing samples/lights -->

```text
#!/bin/rc
# lights - a traffic light on pins 2 (red), 3 (amber) and 4 (green).  An LED lights when its pin is 0
fn on { echo 0 >/dev/gpio/$1 }
fn off { echo 1 >/dev/gpio/$1 }
for(p in 2 3 4) { echo out $p >/dev/gpio/ctl; off $p }
while(~ 1 1) {
	on 2; sleep 4
	on 3; sleep 1
	off 2; off 3; on 4; sleep 4
	off 4; on 3; sleep 1
	off 3
}
```

### Understand it first

1. ✎ `on` and `off` are functions. What does `on 2` do? Why does `on` write a 0?

<div class="write l2"></div>

2. ✎ What does the `for` line do, before the lights start? Why is it needed?

<div class="write l2"></div>

3. ✎ Write out the order of the lights, with how long each lasts, from the `while` loop.

<div class="write l3"></div>

### Make it yours

| ☐ | Change |
|---|---|
| ☐ | Use the timings and the order of the traffic lights where you live |
| ☐ | When the script is stopped, some lights stay on. Write `lightsoff`, to put them all out |
| ☐ | Add a push button on CA1 (section 17.2): the lights stay green until it's pressed, then go through red |
| ☐ | Add a second set of lights, on PA5, PA6 and PA7, for the road that crosses |
| ☐ | Run the lights in the background, and control them from the prompt |
| ☐ | Rewrite it in HyForth, hylang or HyBASIC, with that language's pin words |

✎ Write your finished script here.

<div class="write l8"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | It runs for five minutes without a mistake in the order |
| ☐ | You can explain why each LED needs a resistor, and why it lights on a 0 |
| ☐ | The lights are never green both ways at once (if you built the crossroads) |

> **CHALLENGE:** ★ Everything about the lights' order and timing is buried in the `while` loop. Rewrite the script
> so that the order comes from a list at the top: each item a set of pins to light, and a number of seconds. Then
> a new sequence needs no new code. (No real Hydra? Do the whole project on the screen: draw three circles on the
> Vera X's bitmap, or print three coloured blocks with escape sequences, in place of the LEDs.)
