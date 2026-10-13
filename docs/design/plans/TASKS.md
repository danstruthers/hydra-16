## **Fewer standing tasks**

The Hydra has sixteen tasks, and no more can be had without a new board: the `T` register is four bits.  At a shell's
prompt HydraOS used eleven of them with a Vera X (ten without): the kernel, init, the shell, wstart, input, and six
drivers.  That left five for programs, pipelines and more windows.  This is what was done about it (October 2026),
and what could still be.

### **What was standing, and what is**

| Task | Before | Now |
| :--- | :----- | :-- |
| 0 | the kernel | the kernel |
| 1 | `init`, waiting for its children | **the shell**: init `EXEC`s it |
| 2 | the shell | free |
| 3 | `wstart`, waiting for Ctrl-] c | free |
| 4 | `input` (with a Vera X's keyboard) | free |
| B | `gpio` | `vid` (with a Vera X), else free |
| C | `snd` | `snd` |
| D | `kdev` | `kdev`, with GPIO, I2C and the input controller in it |
| E | `storage` | `storage` |
| F | `cons` | `cons` |

Six standing without a Vera X, seven with one: ten or nine free, where there were seven or five.

### **How**

**1. init becomes the shell.**  init sets the system up, then `EXEC`s the shell `/lib/shell` names (HyForth) in its
own task.  Two things a waiting init did are the kernel's now:

* *Orphans.*  A task whose parent has ended is nobody's (it used to be init's): the kernel lets it go as it ends, its
  exit record with it.  Nothing has to wait for it.
* *The login shell again.*  The kernel remembers the task the boot's init is in.  If that task has `EXEC`ed and then
  ends, the kernel task starts init again from its idle loop, with `0` as its argument, in the lowest free task (task
  1 again, as a rule).  An init that ends without an `EXEC` (the base's monitor, a test's init) is left ended.

init with an argument does only what a new shell needs: its window's console as fds 0-2, `$window`, its namespace
(to find `/lib/shell` and the shell), then the `EXEC`.  The RAM disks, the windows' chrome, the task list and hello
are the boot's alone.

**2. No window starter.**  wstart waited on `#c/wnew` for Ctrl-] c, then started a shell in the window made.  Now
the console makes the window itself when the key comes and nobody has `/wnew` open, and asks the kernel for init
there: **`INIT_RUN`**, `.A` the window.  The kernel task starts init with that number as its argument, as the boot
starts it (no parent, an empty namespace and environment, a note group of its own), and init becomes that window's
shell.  `/wnew` still works for a program that wants to start the windows' shells itself.

**3. A driver's own work: `SRV_NAP`.**  A driver ran only for requests and interrupts, so anything that polled (the
input controller) or that had to happen with no client asking (the console's keys, with the shell waiting for a
program) needed a task: `input` polled, and wstart's waiting read was, by accident, what kept the console turning
over.  Now a driver's `init` may not return, and wait in `SRV_NAP` between turns of its own work: for some ticks, or
(0 ticks) till its own event count isn't the one it last saw.  While it waits it serves its requests as any driver
(the call puts its base thread's state back when it's done); when it runs it's busy, so its work and its requests
never meet.  The boot counts it as started once it's not busy.  Two drivers use it:

* `kdev` looks at the Vera X's input controller: what `input` did, with `SRV_NAP` for its `SLEEP`.
* `cons` hands the keys to the windows and paints the terminals whenever its event count changes, as it does before
  and after each request.  So Ctrl-] c makes a window while the shell waits for a program, and flow control's watch
  runs with nothing reading.

**4. GPIO and I2C in kdev.**  `gpio` was a quarter of a bank and 600 bytes of RAM in a task of its own.  It's part of
HydraOS's `kdev` now: `reborn/modules/kdev/kdev.s` is the base's `kdev.s` with `KDEV_X` defined, which includes
`kdevxt.inc` (the letters `g` and `i` in its table of devices) and `kdevx.inc` (`gpio.inc` and `input.inc`, each in a
scope of its own).  The base's `kdev` is as it was.  The input controller's reads use `gpio.inc`'s I2C routines
directly: a request to its own driver isn't one a driver can make.

**5. An empty namespace.**  A shell that init `EXEC`s inherits init's namespace, and its `-l` builds the default one
again on top: every union twice.  So `UNMOUNT` with no old (`r1 = 0`) is *everything*: the caller has an empty
namespace of its own (one it shared stays the others', as a bind in a shared one copies it first).  nslib's
`ns_default` does that first, as Plan 9's newns starts from an empty namespace: `newns` twice is the same as once.

### **What it cost**

* The kernel: 80 bytes of page 0 (`SRV_NAP`, `INIT_RUN`), 270 of page 1.  Two calls in the server group, so no
  language's bindings grew.
* `kdev` is 14.5K of its bank (it shared one before), so the paged ROM is 71 banks, not 70.
* rc's bank was full: three of its messages are shorter (`args too long`, `bad [ pattern`).
* The Vera X's keyboard and mouse are read by `kdev`, so a request to `kdev` (a pipe, `/proc`, `/env`) waits while a
  look at the controller is going on: a few milliseconds at most, 67 times a second while keys are coming.
* A window's shell is started by init, which builds a namespace only to find the shell, which then builds its own.

### **What could still go**

* **`snd`** when nothing plays: a driver started on its device's first open and ended after its last close would
  free its task most of the time.  The kernel would have to know which module serves which letter.
* **`kdev` itself** in the kernel task: its 16K of RAM and its serve entry would be the kernel task's.  Not looked
  into.
* **`storage`** and **`cons`** are what the system is: they stay.
