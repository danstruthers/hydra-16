## **hydrasim**

A minimal Hydra-16 emulator for debugging the OS ROM without the hardware. It boots the real ROM images
built by `os_rom/makeC02.bat` (`os_rom/tmp/os_rom_C02.bin` and `os_rom/tmp/paged_rom_C02.bin`).

Requires [Node.js](https://nodejs.org). No other dependencies.

### **Usage**

```
node hydrasim.js [options]
```

| Option | Description |
| :----- | :---------- |
| `--rom DIR` | ROM images directory (default: `../os_rom/tmp`) |
| `--cycles N` | CPU cycles to run (default 20,000,000; about 5.6 seconds at 3.58 MHz) |
| `--input TEXT` | Serial input to type after a short delay; `\r` = CR |
| `--modules N` | RAM modules installed: banks `$00` to `N*16-1` (default 3) |
| `--acia-line N` | IRQ line the ACIA interrupts on (default 1) |
| `--stuck-irq N` | Hold IRQ line N active the whole time |
| `--model M` | Hardware what-ifs: `sharedlow`, `nostack`, `zponly`, `noshared` |
| `--raw` | Print serial output as-is (by default ESC shows as `<ESC>`) |
| `--trace N` | Show the last N instructions (default 25) |
| `--dump ADDR[:LEN][@TASK]` | Hex dump task RAM after the run, e.g. `--dump 7D90:16@1` |

Example: boot to Forth and run a command (Forth starts after `COPYTORAM`, so allow plenty of cycles):

```
node hydrasim.js --cycles 60000000 --input "1 2 + .\r"
```

The report shows the serial output, the last instructions executed (`W T PC A X Y S P`), the hottest PCs
(a stuck loop shows up at the top), and the final pseudo-register and vector RAM state.

### **What it models**

* 65C02 CPU, including the WDC/Rockwell additions the ROM uses (`STZ`, `BRA`, `PHX`/`PLY`, `TSB`/`TRB`,
  `BBR`/`BBS`, `RMB`/`SMB`, `WAI`, `STP`, `(zp)` addressing). Cycle counts are approximate.
* `T`: each task has its own `$0000-$7FFF` (zero page, stack, task RAM) and `$00`/`$01` bank registers.
* RAM bank window `$8000-$9FFF`: banks `$00-$EF` per task (only the installed modules; others read back
  floating-bus values), banks `$F0-$FF` shared, 16 macro-pages selected by `U`.
* Paged ROM `$A000-$DFFF` (16K banks selected by `$01`), including the board's A13 half-swap: `$A000` reads
  ROM offset `$2000`, `$C000` reads ROM offset `$0000`.
* BIOS ROM `$E000-$FFFF` in 8K pages selected by `W`; I/O at `$FF00-$FFEF`; `T`/`U`/`V`/`W` at `$FFF0-$FFF3`.
* IRQ vector RAM at `$FFFE`/`$FFFF`: written at index `V[0..3]`, read at index `IRQ_NUMBER(n)` (`n ^ 7`)
  of the lowest active IRQ line, or `V[0..3]` when no line is active (and for `BRK`).
* Rockwell 65C51 ACIA at `$FF10` on IRQ line 1: transmit and receive with interrupts, output captured,
  input from `--input`.
* YM2151 status always reads "not busy". VIA registers are plain storage (no timers or interrupts).
* RAM and the pseudo-registers power up with random values, like the hardware.

It is a model, not the hardware: anything it doesn't simulate (timers, the SPI bus, sound, card slots,
exact timing) can still behave differently on the board.
