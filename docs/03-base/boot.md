# The boot

What happens from the moment the reset line goes high to the first prompt.  The code is `base/kernel/reset.s`, with
POST in `post.s` and the drivers' start in `task.s`.

## Contents

1. [The reset stub](#the-reset-stub)
2. [Step 1: every task's state](#step-1-every-tasks-state)
3. [Step 2: the console, the banner and POST](#step-2-the-console-the-banner-and-post)
4. [Step 3: the interrupts](#step-3-the-interrupts)
5. [Step 4: the boot drivers](#step-4-the-boot-drivers)
6. [Step 5: the tick, init, and the idle loop](#step-5-the-tick-init-and-the-idle-loop)
7. [The module directory](#the-module-directory)
8. [After the boot: the kernel's messages](#after-the-boot-the-kernels-messages)
9. [Rebooting, and the hardware test](#rebooting-and-the-hardware-test)

---

## The reset stub

When the reset line goes high, the W65C02S reads the RESET vector at `$FFFC` and jumps there.  But `$FFFC` is in the
BIOS ROM, whose page is chosen by `W`, and `W` isn't reset: it powers up random and keeps its value through a reset
([the quirks](../02-hardware/quirks.md#registers-that-power-up-random)).  So the CPU may start on any of the 16 pages.

The kernel deals with it by putting the same thing on every page:

* Every page's RESET vector points to `$E000`.
* Every page's first instruction, at `$E000`, is `stz W_REGISTER`: it selects page 0.  The CPU fetches the next
  instruction from page 0, at `$E003`.
* Page 0 goes on from `$E003`: interrupts off (`sei`), decimal mode off, `T` = 0 (the kernel task's RAM and stack), the
  stack pointer at `$FF`, then `BOOT`.

From here the boot runs in **task 0, the kernel task**, with interrupts off, in five steps.

---

## Step 1: every task's state

Nothing in RAM can be trusted after power-up, and the bank registers are random too.  For each task, 15 down to 0, the
boot sets `T` to the task and then, in that task's RAM:

* its bank registers `$00` and `$01` to 0;
* its OS zero page (`$80-$FF`): the task free (`ST_FREE`), no flags, no preemption hold, no notes, its saved stack
  pointer;
* its OS area: no irq entry and no serve entry, every interrupt line unowned in its copy of the owners' table, all 16
  fds closed.

It can't use the stack while it does this: changing `T` changes the stack page under it.  Then, back in task 0:

* the kernel task is made ready, and never preemptible (its `TK_PREEMPT` is 1 for ever);
* `U` = 0, and `V` = line 15's vector entry, the one `BRK` uses;
* the kernel task's tables are cleared: the parents, the tasks' banks, semaphores, exit records, CPU times, shared
  segments, the note groups (each task its own), the channel table (no open files), the device table (no devices), the
  namespaces;
* the kernel task's serve entry is set to the KCALL dispatcher, and its irq entry to the tick's handler.

---

## Step 2: the console, the banner and POST

* **The bring-up console**: the serial port, polled, at 9600 8N1 (`cons.s`).  Everything the kernel prints until a
  console driver takes the port over goes out this way, and it's also kept in the kernel's message buffer
  ([below](#after-the-boot-the-kernels-messages)).
* **The banner**: `Hydra-16: kernel 0.1, ABI 1`.  The ABI is the system calls' version: a program built for ABI 1 runs on
  any kernel that says 1.
* **POST** (BIOS page 4, `post.s`): it checks that the tasks' RAM is kept apart (the `T` lines), that the shared RAM
  is shared, the `W` lines, and every RAM bank chip's lines, and finds the memory cards.  [Building
  it](../02-hardware/building.md#post-the-power-on-self-test) explains its report.  Bad RAM is recorded and left
  unused.  A `T` typed during POST starts the hardware test instead of the system.
* **The environments' banks**: each task's environment is a RAM bank of the kernel task's, from the first good memory
  card; they're emptied.  With no memory card, there are no environments.
* `RAM modules: NN`: how many memory cards POST found good.

---

## Step 3: the interrupts

`IRQ_INIT` (`irq.s`) writes all 16 entries of the vector RAM: each line's entry points to its stub in the COMMON block,
on every page the same.  It sets every line's owner to nobody, then gives the **VIA's line, 0, to the kernel**: the VIA's
timer 1 is the system's tick.  The VIA's line also carries timer 2 and CA1, which the kernel passes on as lines of
their own (16 and 17) to the drivers that own them ([the kernel](kernel.md#interrupts)).

---

## Step 4: the boot drivers

`K_TASK_BOOT` (`task.s`, BIOS page 1) reads the paged ROM's **module directory** and sets up a task for each module
that's a **driver** with the flag **`HF_BOOT`**, in the directory's order: the first in **task F**, the next in **task E**,
and down.  Each is set up as `SPAWN` would set it up: its bank at `$A000`, its data copied into its RAM, its BSS
cleared, and its first instruction its `init` entry.  None runs yet: interrupts are still off, and the kernel task
holds the CPU.

The base's directory has two boot drivers: `ser` (task F) and `kdev` (task E).  HydraOS's has six: `cons` (F),
`storage` (E), `kdev` (D), `snd` (C), `gpio` (B), `vid` (A).  Each prints a line as it's set up: `task F: ser`.

---

## Step 5: the tick, init, and the idle loop

* **The tick**: VIA timer 1 is set free-running at **200 interrupts a second** (`TICK_HZ`), and its interrupt enabled.
* **Interrupts on.**  From now on the scheduler runs, and the boot drivers start: each runs its `init` (registering its
  device letters, `SRV_REGISTER`, and owning its interrupt lines, `IRQ_OWN`), then waits for requests.  While a driver's
  init runs, it's busy: a request to it waits.
* **The boot waits** for every boot driver's init to end, two seconds at most.  A driver whose init fails (it returns
  C = 1) ends; the system goes on without it.
* **Init**: the module the directory names as init starts in **task 1**, as a program, with an empty namespace of its
  own and no fds.  It builds both: it opens the console (fds 0, 1 and 2) and binds what the system's namespace needs.
  **Init is the system**: in the base it's `wozmon`, the monitor; in HydraOS it's `init`, which reads the namespace file,
  starts the windows and the login shell.
* **The idle loop**: task 0 becomes the idle task.  It runs only when no other task can, and then waits for an interrupt
  with `WAI`, which stops the CPU's clock until one comes.  It also takes a turn when an interrupt handler has queued
  notes to a note group for it to send.

The base reaches the monitor's prompt in under a second, most of it spent printing at 9600 baud.

---

## The module directory

The paged ROM's bank 0 holds the **module directory**, at `$A200` (written by `base/tools/romimg.js` from a
`rom.txt`):

* a signature, `HYMD`, and its version;
* how many modules there are (127 at most), and which is init;
* whether bank 1 holds the hardware test;
* 16 bytes for each module: its first bank, how many banks it spans, its type (program, driver, library), its flags,
  and its name (12 characters).

`MODINFO` reads an entry, and kdev serves the directory as files: `#m/NAME` is a module's image, which `SPAWN` reads;
`#m/bin` lists the programs, and is bound at `/bin`.

A `rom.txt` names the modules in order, and init:

```
init        wozmon
module      ser         ; (task F: the console's driver)
module      kdev        ; (task E: the kernel's devices)
module      wozmon      ; (task 1: the monitor)
```

Bank 0's first 512 bytes hold a signature line and the ROM disk's partition table (HydraOS's `/rom`); bank 1 is the
hardware test with the checksums of every ROM; the modules start at bank 2, each at `$A000` of its own bank.

---

## After the boot: the kernel's messages

Everything the kernel prints on the bring-up console, from the banner on, is also kept: its last 4K, in the kernel
task's RAM.  The `KMESG` call reads them, and kdev serves them as `#n/kmesg`, bound at **`/dev/kmesg`** (Plan 9's name):
`cat /dev/kmesg` shows the boot's lines long after they've scrolled away.  A driver's own messages from its init go
there too.

---

## Rebooting, and the hardware test

`REBOOT` starts the system again, as the reset button does: it quiets the VIA's and the ACIA's interrupts (only the
button resets the chips), then jumps to the reset stub.  With `.A = REBOOT_HWTEST`, POST starts the hardware test
instead of the system, as a `T` typed during POST does: HydraOS's `hwtest` command is this.  Every task ends where it
is; files written are on their disks already, since the disks write through.
