# Project 5. A Program in the Background

> **THIS PROJECT:** rc, and the Hydra's tasks. After Labs 4 and 6. One or two sittings. You'll start a program
> that keeps running while you do other things: the thing the Hydra-16 is named for.

<p class="fields">Name <span></span> Started <span class="short"></span> Finished <span class="short"></span></p>

### The starting point

`ticker` writes the time into a file every five seconds, for ever. Type it into `/ram/ticker`.

<!-- listing samples/ticker -->

```text
#!/bin/rc
# ticker - write the time into a file every five seconds, until it's stopped
while(~ 1 1) {
	date >>/ram/ticker.log
	sleep 5
}
```

A `&` after a command starts it **in the background**: the shell doesn't wait for it, and you get your prompt back
at once.

```text
/ram> rc ticker &
/ram> ps
/ram> cat /ram/ticker.log
/ram> ls -l /ram
/ram> cat /ram/ticker.log
```

✎ Copy the lines of `ps` that belong to your ticker. There are two. What's each one doing?

<div class="write l3"></div>

Stop it with `kill` and the number of its `rc` task, and check with `ps` that both lines have gone.

### Understand it first

1. ✎ While `ticker` runs, you can still type commands. How can one processor do both?

<div class="write l2"></div>

2. ✎ Most of the time, your ticker's tasks are in the state `sleep` or `wait`. How much of the processor's time do
   you think they use? Look at the `cpu` column of `ps` to check.

<div class="write l2"></div>

3. ✎ What happens to the ticker if you press <kbd>CTRL</kbd>-<kbd>C</kbd> at the prompt? If you reset the Hydra?

<div class="write l2"></div>

### Make it yours

| ☐ | Change |
|---|---|
| ☐ | Take the delay as an argument: `ticker 2` ticks every two seconds |
| ☐ | Make it stop by itself after a number of ticks |
| ☐ | Watch it from a second window (Lab 8), with `tail /ram/ticker.log` |
| ☐ | Make it record something more interesting each time: the free space (`df`), or how many tasks are running |
| ☐ | Run two tickers at once, writing to the same file. Do their lines ever get mixed up? |
| ☐ | Write an alarm: `alarm 60 Tea is ready` waits that many seconds, then rings the bell and prints the message |

✎ What did you find when two tasks wrote to one file? (The sample program `chorus`, from section 6.8, is about
exactly this.)

<div class="write l4"></div>

### Show it

| ☐ | Before you call it finished |
|---|---|
| ☐ | You can start it, find it with `ps`, and stop it, without looking anything up |
| ☐ | It doesn't fill the disk if it's left running (how do you know?) |
| ☐ | You can explain to someone what `&` does, and what a task is |

> **CHALLENGE:** ★ Run `/sd/0/sample/c/race`. Four tasks add to one counter, and lose count. Explain, in your own
> words and with a drawing, how two tasks can both add 1 and the counter only go up by 1. Then explain what the
> lock changes.
