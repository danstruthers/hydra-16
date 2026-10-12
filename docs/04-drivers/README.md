# 4. The drivers

A **driver** is a module of the paged ROM that serves a device as files, in a task of its own.  This chapter is each of
them, from the serial port, which everything else depends on, to the Vera X: what files each serves, what you can do
with them from the shell, and how each works inside.  [Chapter 3](../03-base/files.md) explained the file model they
all follow; [servers and drivers](../09-programming/servers.md) is how to write one.

## Contents

1. [The serial console](serial.md): `ser` (the base's) and `cons` (HydraOS's): the console, its line editor and raw
   mode, the windows, the serial port, `/pc`.
2. [The kernel's devices](kdev.md): `kdev`: the modules, `/proc`, pipes, the environment, shared segments' names, the
   clock.
3. [Storage](storage.md): `storage`: the SPI bus, SD cards, the RAM disks, the ROM disk, and HydraFS.
4. [Sound](sound.md): `snd`: the YM2151 and the Vera X's PSG as one synthesizer, and the PCM.
5. [GPIO, I2C and SPI](gpio.md): `gpio`: the VIA's port A, its pins, the I2C bus; the SPI devices as files.
6. [Video](video.md): `vid` and `input`: the Vera X's screen, its files, drawing, claiming the chip, the keyboard and
   the mouse.

## The drivers at boot

The boot starts each boot driver in a task of its own, from task F down, in the order of the paged ROM's module
directory ([the boot](../03-base/boot.md#step-4-the-boot-drivers)):

| Task | Base | HydraOS | Devices | Bound at |
| :--- | :--- | :------ | :------ | :------- |
| F | `ser` | `cons` | `#c` the console (and `#P`, `/pc`, in HydraOS) | `/dev`, `/pc` |
| E | `kdev` | `storage` | `#S` SPI, `#d` disks, `#f` HydraFS | `/dev/spi`, `/dev/sd`, `/sd`, `/rom`, `/ram`, `/sram` |
| D | | `kdev` | `#/`, `#n`, `#t`, `#m`, `#p`, `#\|`, `#e`, `#s`, `#r` | `/`, `/dev`, `/proc`, `/env`, `/dev/seg` |
| C | | `snd` | `#a` sound | `/dev` |
| B | | `gpio` | `#g` GPIO, `#i` I2C | `/dev/gpio`, `/dev/i2c` |
| A | | `vid` | `#v` the Vera X (none without a card: it ends) | `/dev/vid` |

`input`, the Vera X's keyboard and mouse, is a program rather than a boot driver: init starts it, and it ends at once
with no input controller.

`ps` shows them (the tasks in decimal; this is the emulator without a Vera X, so `vid` has ended):

```
/> ps
task  state   parent     cpu group  name
   0  ready   -          2.9     0  kernel
   1  wait    -          0.1     1  init
   2  wait     1         0.2     2  forth
   3  event    1         0.0     1  wstart
   4  ready    2         0.0     2  ps
  11  idle    -          0.0    11  gpio
  12  idle    -          0.0    12  snd
  13  idle    -          0.1    13  kdev
  14  idle    -          1.3    14  storage
  15  idle    -          0.5    15  cons
```

## How a driver works

Every driver has the same shape ([servers and drivers](../09-programming/servers.md)):

* **`init`** runs once as it starts: it registers its device letters (`SRV_REGISTER`), owns its interrupt lines
  (`IRQ_OWN`), and sets its hardware up.
* **`serve`** runs for each request a client makes, in the driver's own task.  Nearly every driver uses srvlib's, with a
  table of its files and a routine for each.  A driver **never waits**: when it can't answer yet it says so
  (`E_AGAIN`), and adds to its event count when things change ([waiting](../03-base/files.md#waiting)).
* **`irq`** runs for its interrupt lines, in its own task, with interrupts off and about 85 cycles to spend.
* Its state is in its own RAM and banks; nothing else touches its hardware.  **One driver owns each piece of hardware**:
  the serial port is the console driver's, the SPI bus and every disk the storage driver's, the YM2151 the sound
  driver's, port A the GPIO driver's, the VERA the video driver's.  That's why there are no locks: two requests never
  meet on a chip.

**Drivers rarely call each other**, and only three ways, so none can wait on another: the console rings the sound
driver's bell (`#a/bell`) and writes the screen (`#v/term`), and the sound driver writes the PSG's registers through
the video driver (`#v/psg`).  `vid` calls nobody.
