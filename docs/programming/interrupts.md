## **Interrupts**

How the OS dispatches interrupts, and how a driver handles one.  Sources: `os_rom/kernel/irq.s`, `os_rom/kernel/common.s`.  The hardware side (16 prioritised lines, the vector RAM, the `n ^ 7` numbering) is in the [Hardware Reference](../hardware.md#interrupts).  Part of the [Programmer's Guide](README.md).

### **IRQ lines**

| IRQ | Source | Registered by |
| ---: | :----- | :------------ |
| 0 | VIA: timer 1 (the scheduler's tick), timer 2 | The system task (tick); the serial driver (timer 2: sending, in WDC ACIA builds, and at 115200) |
| 1 | ACIA (serial) | The serial driver |
| 2, 3 | Slot 0, A and B | |
| 4 | YM2151 | Timer B: the sound clock, a song player's tick (a fast handler, `YM_IRQ_FAST`: below).  The sound driver's registered handler is a placeholder |
| 5-9 | Slots 1-5, A | |
| 10-14 | Slots 1-5, B | |
| 15 | Software interrupts | `SWI_REGISTER` |

Line 0 has the highest priority.  In code, name a line with `IRQ_NUMBER(n)`, or with the names in `include/hw.inc` (`IRQ_NUMBER_ONBOARD_VIA`, `IRQ_NUMBER_SLOT_1_L`, ...).  `IRQ_NUMBER(n)` is `n ^ 7`: the vector RAM entry the hardware uses for line n.

### **How an interrupt is handled**

1. **The stub.**  At boot, `IRQ_INIT` points the vectors at the IRQ stubs in the COMMON block (all but the VIA's and the ACIA's: [below](#the-fast-handlers-the-tick-the-serial-port-and-the-sound-clock)) (`$FD00`, the same on every page).  Each stub loads its line's number, saves `W` and switches to ROM page 0, wherever the CPU was.
2. **The dispatcher** (`IRQ_DISPATCH`, page 0) looks the line up in the registration table.  It runs each registered handler **in the task that registered it** (`TASK_CALL`): on that task's stack, with its zero page, RAM bank and MMU area.  So a driver's handler sees the driver's own state, whichever task was interrupted.
3. **Claiming.**  A handler returns C = 1 if it claimed the interrupt, which stops the chain.  An IRQ that none of its handlers claims is counted in the interrupted task's unclaimed-IRQ counters (`$7D90-$7D9F`, by line).  It's then offered to every registered handler, so a misrouted interrupt still gets cleared.
4. **Returning.**  The dispatcher switches back to the interrupted task, or switches task if the tick asked for one.  It restores `W` and returns with `rti` from the COMMON block.

The registration tables live in the task system page (`$7D00`) and are copied into all 16 tasks.  So the dispatcher reads them from whichever task was interrupted, without switching.  Registering updates all 16 copies.

### **The fast handlers: the tick, the serial port and the sound clock**

The dispatcher and its `TASK_CALL` cost about 650 cycles per interrupt, far too much for the serial port at high rates (at 115200 baud a byte arrives every 320 cycles, and the 65C51 holds only one).  So the busiest interrupts bypass it (`servers/serfast.s`, `sound/ymfast.s`, BIOS page 2):
* **Their vectors:** `IRQ_INIT` points the VIA's (line 0), the ACIA's (line 1) and the YM2151's (line 4) vectors at `VIA_IRQ_STUB`, `SER_IRQ_STUB` and `YM_IRQ_STUB` in the COMMON block, which switch to page 2 (`IRQ_FAST_P2`).
* **No stack switch:** instead of running in the driver's task, a fast handler briefly switches `T` to it, a "quick look": its zero page and RAM, with no stack use until `T` is back.
* **`SER_IRQ_FAST`** moves the received byte into the receive ring and the next byte from the transmit ring to the ACIA, and wakes the tasks waiting to read or write.
  * **Cost:** about 60–90 cycles per byte.
  * **The rest goes through the dispatcher:** the break and kill keys, console commands and the bell are recorded in `SER_PEND` for the serial driver's own handler (`SER_DO_PENDING`).
* **`VIA_IRQ_FAST`** counts the tick and wakes the sleepers due, in the system task's zero page.
  * **Then** it asks the dispatcher for a task switch (`IRQ_TICK`).
  * **Timer 2** paces sending: always with a WDC ACIA (its transmitter status doesn't work), and at 115200 with the Rockwell.  `SER_T2_FAST` sends the next byte from the transmit ring as `SER_IRQ_FAST` would (`SER_TX_STEP`), so either chip sends at the wire's rate.
  * **Other VIA sources** go to the registered handlers as before.
* **`YM_IRQ_FAST`** is the sound clock: timer B, run by the sound driver for a song player at the song's rate (`SND_CTL_CLOCK`, [io.md](io.md#sound-devsnd)).  In the sound task's zero page, it resets timer B's flag, sets the period after the next (K or K + 1 units, as a 16-bit fraction carries, so the rate is exact on average), counts the tick (`SND_CLK`), and, when the waiting player's time has come (its `ZSM_AT`, a quick look into its zero page), wakes it and asks for a task switch (`IRQ_TICK`).
  * **Cost:** about 150-300 cycles: up to two register writes to the chip, each of which may wait up to 64 cycles for it (busy after the sound driver's last write).
* **The registered handlers** for lines 0 and 1 (`VIA_IRQ_HANDLER`, `SERIAL_IRQ_HANDLER`) are still there, and are what the dispatcher calls for that rare work.

### **Registering a handler**

| Call | In | Out |
| :--- | :- | :-- |
| `IRQ_REGISTER` | `.X` = `IRQ_NUMBER(n)`, `.A.Y` = handler (page 0) | C = 0; or C = 1, `ERR_IRQ_CHAIN_FULL` (2 handlers per line) |
| `IRQ_UNREGISTER` | `.X` = `IRQ_NUMBER(n)`, `.A.Y` = handler | C = 0; or C = 1, `ERR_IRQ_NOT_FOUND` |
| `SWI_REGISTER` | `.X` = software interrupt number (`$0-$F`), `.A.Y` = handler | as above (one handler per number) |
| `SWI_UNREGISTER` | `.X` = number, `.A.Y` = handler | |

The handler runs in the **calling task**, so call these from the driver's own task: from its `init` (which `DRV_START` runs there).  A task's handlers are removed when it ends.

### **Writing a handler**

```
; IN: .A = the line (0-14), or the software interrupt number; I flag set
; OUT: C = 1 claimed, C = 0 not mine
MY_IRQ_HANDLER:
            lda     MY_DEVICE_STATUS
            bpl     @not_mine                   ; (e.g. bit 7 = this device interrupted)
            ...                                 ; clear the cause, move the data
            sec
            rts
@not_mine:
            clc
            rts
```

**Rules:**
* **Page 0.**  It must be a page 0 address: the dispatcher calls it on page 0.  A driver whose code is on another page puts a small page 0 handler in front.
* **Clear the cause** before returning; the IRQ input is level-triggered.
* **Don't re-enable interrupts**, and don't use far gates, `YIELD`, IO calls or anything else that can wait.
* **Keep it short:** at 19200 baud a byte arrives about every 1,860 cycles (at 115200, about 320), and the ACIA holds only one.
* **What it may touch:** `.A`, `.X`, `.Y`.  It may call `IO_WAKE` to wake a task waiting on the device.
* **Bank registers:** preserve `$00` and `U` if it changes them.  The dispatcher gives it its own task's `$00`, but `U` is global.
* **Asking for a task switch:** a handler returns C = 1 with `.A` = `SCHED_RESCHED_A` and `.Y` = `SCHED_RESCHED_Y`.  The tick handler does this.

### **Software interrupts**

A software interrupt is a `BRK` with `V` pointing at IRQ 15's vector.  The number (`$0-$F`) goes in `V`'s upper nibble:

```
            lda     #3                          ; software interrupt 3
            jsr     SW_INT                      ; saves and restores V, runs the handler, returns
```

`SW_INT` disables interrupts between setting `V` and its `brk`, so another task can't change `V` in between.  The dispatcher reads the number from `V` and runs the handler registered with `SWI_REGISTER`, in its task.

### **NMI**

The NMI vector points at the COMMON block's `NMI_ENTRY`: it switches to page 0 and calls `NMI_HANDLER`, which does nothing yet.  Nothing on the board drives NMI; a slot card can.

### **Timing notes**

* The dispatcher and the `TASK_CALL` into the handler's task cost about 650 cycles per interrupt; the fast handlers about 60–90.
* **How often a driver's interrupt can come:** through the dispatcher, up to a few hundred a second (650 cycles each: 60 a second, a VERA's frames, is about 1% of the CPU; 1,000 a second would be a fifth of it).  Faster than that (a raster line, a byte at a time from a fast port) needs a fast handler: a stub in COMMON, as the VIA's, the ACIA's and the YM2151's have, and its code on its page, with no dispatcher.  COMMON has little room (the build's `rom_space.js` report shows how much), so plan for it.
* **Keep sections with interrupts off short.**  Anything longer than a character's time holds off the serial port and loses input: about 320 cycles at 115200 baud, 620 at 57600, 3,700 at 9600.
  * **Instead:** use `NO_PREEMPT` when only a task switch must be prevented, as the MMU calls do.
  * **For long loops that need interrupts off:** open a moment for them between iterations (`cli`, `nop`, `sei`), as `FP_COPY`, `SH_RESET_TASK` and `SCHED_PICK` do.
  * **Checked by a test:** the `irqs-off` regression test fails if any stretch after boot exceeds 5,000 cycles, and the emulator's report lists the longest.
