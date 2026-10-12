# Lab 14. Pins, and the System's Own Files

> **THIS LAB:** Manual: Chapter 17, sections 17.1, 17.2, 17.5, 17.7 and 17.8. About an hour. Part A needs a real
> Hydra, an LED, a 2.2K resistor and three wires; on an emulator, do its steps 1 and 2 only. Part B works anywhere.

<p class="fields">Name <span></span> Date <span class="short"></span></p>

### Do: Part A, a light on a pin

1. ⚠ Read the caution in section 17.1 first. Then make pin PA2 an output, and switch it on and off:

```text
/> echo out 2 >/dev/gpio/ctl
/> echo 1 >/dev/gpio/2
/> cat /dev/gpio/2
/> echo 0 >/dev/gpio/2
/> cat /dev/gpio/ctl
```

   ✎ What does the `ctl` file tell you about the eight pins?

<div class="write l2"></div>

2. ✎ On the header J27, which pin number is PA2? Which is +5 V, and which is ground? (The table in section 17.1.)

<div class="write"></div>

3. With the power **off**, wire the LED as section 17.1 says: from +5 V, through the 2.2K resistor, through the
   LED, to PA2. ✎ Draw your circuit. Which leg of the LED goes towards +5 V?

<div class="space s2"></div>

4. Switch on, make PA2 an output again, and write 0 and then 1 to it. ✎ Which one lights the LED? Why is it "the
   wrong way round"?

<div class="write l2"></div>

5. Make it blink. In rc (type `rc` first):

```text
% while(~ 1 1) { echo 0 >/dev/gpio/2; sleep 1; echo 1 >/dev/gpio/2; sleep 1 }
```

   (`~ 1 1` is a test that's always true, so the loop goes round for ever.) ✎ How do you stop it? How would you
   make it blink faster?

<div class="write"></div>

### Do: Part B, the system as files

6. The kernel shows the tasks as files. Type `ls /proc`, and then look at your own shell (use the task number
   that `ps` shows for `forth`):

```text
/> ps
/> cat /proc/2/status
/> cat /proc/2/fd
```

   ✎ What does `status` tell you? What are the files your shell has open (`fd`)?

<div class="write l3"></div>

7. The environment is a directory of variables. ✎ What's in it? What does `cat /env/prompt` show?

<div class="write l2"></div>

8. Set the clock. (Use today's date and the time now, in exactly this form.)

```text
/> echo 2026-10-12 09:41:00 >/dev/time
/> date
/> cat /dev/rtc
```

   ✎ What did `/dev/rtc` say? Will your Hydra still know the date tomorrow?

<div class="write l2"></div>

### Questions

1. ✎ A pin can be an input or an output. What's the difference? Give one thing you could connect to each.

<div class="write l2"></div>

2. ✎ Why does the LED need a resistor?

<div class="write"></div>

3. ✎ PA0 and PA1 have a second job. What is it, and what does that mean for using them as ordinary pins?

<div class="write l2"></div>

4. ✎ `/proc` isn't on any disk. Where do its "files" come from? What reads them, besides you? (Section 29.11 has a
   surprise.)

<div class="write l2"></div>

5. ✎ You've now seen pins, the clock, tasks and variables, all as files. What's the advantage of making everything
   a file?

<div class="write l3"></div>

### Check yourself

| ☐ | I can |
|---|---|
| ☐ | make a pin an output, and set it; or an input, and read it |
| ☐ | wire an LED to a pin safely |
| ☐ | look at a task through `/proc` |
| ☐ | read and set the clock |

> **CHALLENGE:** ★ Wire a push button from PA3 to ground. An input with nothing connected reads 1 (section 17.2),
> so PA3 reads 1 until the button is pressed, and 0 while it's held. Write a loop that lights the LED only while
> the button is down. (No real Hydra? Then write a script that prints the name of every task, by reading `/proc`.)
