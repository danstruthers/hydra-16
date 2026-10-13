# GPIO, I2C and SPI

**kdev** serves the VIA's port A on header J27 (`reborn/modules/kdev/gpio.inc`, in HydraOS's `kdev`: it was a driver
of its own, `gpio`, and took a task): its eight pins and its handshake lines as `#g`, and the I2C bus on two of them
as `#i`.  One driver for both, so their changes to port A never meet.  The SPI devices are the storage driver's
([storage](storage.md#spi-devices-as-files)), since SD cards share the bus.

## Contents

1. [The pins: /dev/gpio](#the-pins-devgpio)
2. [CA1: waiting for an edge](#ca1-waiting-for-an-edge)
3. [The I2C bus: /dev/i2c](#the-i2c-bus-devi2c)
4. [SPI devices: /dev/spi](#spi-devices-devspi)
5. [From each language](#from-each-language)
6. [An example: a button and an LED](#an-example-a-button-and-an-led)

---

## The pins: /dev/gpio

`#g`, bound at `/dev/gpio`.  The pins are on J27 ([the VIA](../02-hardware/devices.md#the-via-65c22)): PA0-PA7 at pins
3-10, CA1 and CA2 at 11 and 12, with +5 V and ground.  PA0 and PA1 are the I2C bus's while `/dev/i2c` is used.

| File | Read | Write |
| :--- | :--- | :---- |
| `0` ... `7` | The pin's level, `0` or `1` | `0` or `1`: sets it, and makes the pin an output |
| `port` | All 8 pins' levels, a byte | A byte: sets the outputs (the inputs keep theirs) |
| `ctl` | A line a pin (`2 out 1`), then `ca1 fall 3` (CA1's edge, and the edges counted) and `ca2 in` | `in N`, `out N` (a pin's direction), `ddr N` (all 8: bit n pin n, 1 an output), `ca1 rise`, `ca1 fall` (CA1's active edge), `ca2 0`, `ca2 1` (CA2 an output, low or high), `ca2 in` |
| `ca1` | Waits for CA1's next active edge, then the edges counted so far | |

```
% echo 1 >/dev/gpio/2                  # PA2 an output, high
% echo in 3 >/dev/gpio/ctl              # PA3 an input
% cat /dev/gpio/3
1
% cat /dev/gpio/ctl
```

The pins are 5 V CMOS, through the SP720 ESD array (J28).  A VIA output can drive an LED through a resistor (1K) but not
much more; use a transistor for anything bigger.

---

## CA1: waiting for an edge

A read of `/dev/gpio/ca1` **waits** for CA1's next active edge (`ca1 fall` or `ca1 rise` in `ctl`) since that fd last read
it, then gives the count of edges so far.  A non-blocking fd gets `E_AGAIN` instead.  CA1's interrupt is on only while a
`ca1` file is open (it's the kernel's line 17, owned then by `kdev`), so a floating CA1 can't flood the system.  A
switch's bounce counts as edges too: debounce in hardware, or by time.

```
% cat /dev/gpio/ca1                     # waits for a press
1
```

---

## The I2C bus: /dev/i2c

`#i`, bound at `/dev/i2c`: an I2C master, bit-banged on PA0 (SCL) and PA1 (SDA), which are pulled up on the board and
go to J27 and every slot.

| File | What it is |
| :--- | :--------- |
| `NN` | The device at address NN (7 bits, two hex digits, `08`-`77`): a write sends its bytes to it, a read reads from it, one transaction a request (64 bytes at most).  A device that doesn't answer: `E_IO` |
| `ctl` | `speed N` (kHz, 1-100; it tops out near 40 kHz at 3.58 MHz), `subaddress N` (0, 1 or 2) |

**Listing `/dev/i2c`** shows the devices that answer: `ls /dev/i2c` is a bus scan.

**Registers**: most I2C parts (EEPROMs, sensors, the Vera X's input controller) have registers.  With `subaddress 1`
(or 2), the file's offset is the register: a read writes the register's number (a byte, or two, high first), then a
repeated start and the read; a write writes the register's number then the data.  So `/dev/i2c/50`, with `subaddress 1`,
reads a 24C02 EEPROM as a file of 256 bytes.

```
% ls /dev/i2c
ctl
42
% echo subaddress 1 >/dev/i2c/ctl
```

`$42` is the Vera X's input controller, which `kdev` itself reads ([video](video.md#the-keyboard-and-the-mouse)).
Nothing else on the board uses the bus.  As the system starts `kdev` looks for the controller three times, a second
apart; with none, PA0 and PA1 aren't touched again unless something uses `/dev/i2c`.

---

## SPI devices: /dev/spi

The storage driver serves the SPI bus as `#S`, at `/dev/spi`: `N/data` (a write is a transaction; a read gives the
bytes the device sent back, or clocks new ones in) and `N/ctl` (`mode 0` or `mode 3`).  Devices 0-7 are headers
J18-J25; 8-15 are for cards in the slots.  [Storage](storage.md#spi-devices-as-files) has the details.

---

## From each language

Each language has a library over these files, with the same words:

| What | HyForth (`lib gpio`, `lib i2c`, `lib spi`) | hylang (`(use "gpio")` ...) |
| :--- | :--- | :--- |
| A pin's level | `gpio ( pin -- level )` | `(gpio pin)` |
| Set a pin (an output) | `gpio! ( pin level -- )` | `(gpio! pin level)` |
| A pin's direction | `gpio-in ( pin -- )`, `gpio-out ( pin -- )` | `(gpio-in pin)`, `(gpio-out pin)` |
| All 8 pins | `gpio-port ( -- byte )`, `gpio-port! ( byte -- )` | `(gpio-port)`, `(gpio-port! byte)` |
| I2C read, write | `i2c-read ( addr reg c-addr u -- )`, `i2c-write ( addr reg c-addr u -- )` | `(i2c-read addr n)`, `(i2c-write addr bytes)` (after `(i2c-reg-size n)`, the first bytes are the register) |
| I2C devices | `i2c-devices`, `i2c? ( addr -- flag )` | `(i2c-devices)` |
| An SPI transaction | `spi ( dev c-addr u -- )`, `spi-read ( dev c-addr u -- )`, `spi-mode ( dev mode -- )` | `(spi dev bytes)`, `(spi-read dev n)` |

In C, they're files: `open ("/dev/gpio/2", O_WRONLY)` and `write`.  HyBASIC's `OPEN` works the same way.  The libraries
are sources on the ROM disk (`/rom/lib/forth/gpio.fs`, `/rom/lib/hylang/gpio.hl`), and short: they're examples of
driving a device through its files.

---

## An example: a button and an LED

An LED (through a 1K resistor) from PA2 to ground, and a push button from CA1 to ground (with `ca1 fall`): each press
turns the LED over.  In rc:

```
echo ca1 fall >/dev/gpio/ctl
led=0
while () {
    cat /dev/gpio/ca1 >/dev/null        # wait for a press
    if (~ $led 0) led=1
    if not led=0
    echo $led >/dev/gpio/2
}
```

In HyForth (`lib gpio` loads `/lib/forth/gpio.fs`):

```
lib gpio
: toggle  ( -- )  0  begin  s" /dev/gpio/ca1" gpio-read 2drop  1 xor  dup 2 swap gpio!  again ;
```
