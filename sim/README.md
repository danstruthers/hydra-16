## **hydrasim**

A minimal Hydra-16 emulator for debugging the OS ROM without the hardware. It boots the real ROM images
built by `os_rom/makeC02.bat` (`os_rom/bin/os_rom_C02.bin` and `os_rom/bin/paged_rom_C02.bin`).

Requires [Node.js](https://nodejs.org). No other dependencies.

### **Usage**

```
node hydrasim.js [options]
```

| Option | Description |
| :----- | :---------- |
| `--rom DIR` | ROM images directory (default: `../os_rom/bin`) |
| `--cycles N` | CPU cycles to run (default 20,000,000; about 5.6 seconds at 3.58 MHz) |
| `--input TEXT` | Serial input to type after a short delay; `\r` = CR, `\xNN` = the byte NN (e.g. `\x03` = Ctrl-C), `\w` = wait about 2M cycles before the next key (booting to the HyForth prompt takes about 12 of them) |
| `--modules N` | RAM modules installed: banks `$00` to `N*16-1` (default 3) |
| `--shared-u N` | Shared RAM installed for `U` macro-pages 0 to N-1 (default 16; each 512K chip is 4 macro-pages) |
| `--acia-line N` | IRQ line the ACIA interrupts on (default 1) |
| `--acia rockwell\|wdc` | The ACIA chip: the Rockwell R65C51 (default), or the WDC W65C51N with its transmitter bug (TDRE always reads 1, no TX interrupt; for a ROM built with `SER_ACIA = SER_ACIA_WDC`, which paces sending with VIA timer 2).  In WDC mode the emulator counts bytes written while one is still being sent (they'd be garbled on the chip) and reports them at the end.  (VIA timer 2 is modelled too: one-shot) |
| `--stuck-irq N` | Hold IRQ line N active the whole time |
| `--sd [N:]FILE` | An SD card (SDHC) on SPI device N (0-7, the board's SPI headers J18-J25; default 0), backed by the image FILE (512-byte blocks; writes go to the file).  Up to 8 cards, one per device, e.g. `--sd card0.img --sd 3:C:/images/card3.img`.  Models the VIA's port B SPI bit by bit (device select as the board's 74HC138 does it), and the SD commands the ROM uses (CMD0, 8, 16, 17, 24, 55, 58, ACMD41) |
| `--ram-fault BANK:An:high\|low` | Address line An (0-12) stuck high or low on the RAM chip holding BANK (a shared chip holds 4 bank IDs, e.g. `F0-F3`; a task RAM module 16 banks), e.g. `F0:A0:high`.  The POST `RAM` line should report it |
| `--model M` | Hardware what-ifs: `sharedlow`, `nostack`, `zponly`, `noshared` |
| `--raw` | Print serial output as-is (by default ESC shows as `<ESC>`) |
| `--trace N` | Show the last N instructions (default 25) |
| `--dump ADDR[:LEN][@TASK]` | Hex dump task RAM after the run, e.g. `--dump 7D90:16@1` |
| `--watch ADDR[@TASK]` | Report every write to a task RAM address: the old and new value, and the PC that wrote it |
| `--mark TEXT` | Report the cycle each time the serial output ends with `TEXT` (`\r` = CR), e.g. `--mark "HF>"` to time a command from prompt to prompt |
| `--profile N` | From cycle `N` on, count the instructions each task runs in each routine (named from the build's debug info, `os_rom/obj/os_rom_C02.dbg`), and report the top 30, e.g. `--profile 2800000 --input '\wwords \| wc . . .\r'` |
| `--pc [PAGE:]ADDR` | Report the registers each time the PC reaches `ADDR` (on BIOS ROM page `PAGE`, if given); addresses are in `os_rom/obj/os_rom_C02.lbl` |

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
* VIA timer 1 (one-shot and free-running, latches, interrupt flag and enable registers) on IRQ line 0: the
  scheduler's tick.  The other VIA registers are plain storage (no timer 2, ports or shift register).
* YM2151 status always reads "not busy".
* RAM and the pseudo-registers power up with random values, like the hardware.

It is a model, not the hardware: anything it doesn't simulate (timers, the SPI bus, sound, card slots,
exact timing) can still behave differently on the board.
