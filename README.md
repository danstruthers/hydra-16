## **Hydra 16**

Project to create a multi-tasking 6502-based computer and basic operating system.

The code is built with **cc65** (https://cc65.github.io/).  The board schematics and PCB layouts are done in **KiCAD 9.0** (https://www.kicad.org).

### **Memory Map**
* PER-TASK memory map (each task has its own copy of this memory space, except for shared RAM pages, as discussed below)

| Start | End  | Description |
| :---- | :--- | :---------- |
| $00 | $00 | RAM Page selection register (Pages `$00-$EF` are task-specific.  Pages `$F0-$FF` are shared between all tasks, and are further indexed using the U register, below) |
| $01  | $01 | ROM Page selection register |
| $02 | $0F | Reserved Zero-page entries for future use |
| $10 | $FF | Remaining Zero-page |
| $0100 | $01FF | Hardware Stack |
| $0200 | $7EFF | Availabe Task RAM space |
| $7F00 | $7FFF | Serial input buffer (256 bytes) |
| $8000 | $9FFF | Paged RAM (8K pages; task-specific and shared pages all show up here) |
| $A000 | $DFFF | Paged ROM (16K pages; ROMs are shared between all tasks, but the page selection is per-task, see `$01` above) |

* SHARED memory map (all tasks see the following areas the same)

| Start | End  | Description |
| :---- | :--- | :---------- |
| $E000 | $FFFF | BIOS/OS ROM paged area (indexed by the W register, below) |
| $E000 | $FEFF | Effective BIOS paged area |
| $E000 | | RESET Vector entry point, replicated on each BIOS page.  Code saves `W` register to `ZP_W_SAVE` and then resets W to zero. |
|  | | **I/O Ports** `$00-$0E` |
| $FF00 | $FF0F | Onboard VIA (65C22) |
| $FF10 | FF13 | Onboard ACIA (65C51) Serial |
| $FF14 | $FF1F | Unused |
| $FF20 | $FF3F | Reserved for future Video |
| $FF40 | $FF41 | Onboard YM2151 Sound generator |
| $FF42 | $FF4F | Unused |
| $FF50 | $FFEF | Unused (future I/O ports, expansion cards) |
|  | | **Pseudo-registers** |
| $FFF0 | | T Register (current task selector) |
| $FFF1 | | U Register (current shared memory macro-page) |
| $FFF2 | | V Register (interrupt vector selector) |
| $FFF3 | | W Register (BOIS page selection register) |
| $FFF4 | $FFF9 | Unused (future pseudo-register expansion) |
| $FFFA | $FFFB | NMI Interrupt handler vector |
| $FFFC | $FFFD | Reset Vector (Set to `$E000`) |
| $FFFE | $FFFF | Interrupt Vector (16-entry pseudo-register indexed by either `V` register bits 0-3 if no hardware interrupt is active when `BRK` is called, or indexed by lowest numbered active interrupt request line if one or more is active)

### **Interrupts**

| IRQ # | Description |
| --- | --- |
| 0 | On-board VIA |
| 1 | On-board ACIA (Serial) |
| 2 | Card Slot 0 (low) |
| 3 | Card Slot 0 (high) |
| 4 | On-board Sound (YM 2151) |
| 5 | Card Slot 1 (low) |
| 6 | Card Slot 2 (low) |
| 7 | Card Slot 3 (low) |
| 8 | Card Slot 4 (low) |
| 9 | Card Slot 5 (low) |
| 10 | Card Slot 1 (high) |
| 11 | Card Slot 2 (high) |
| 12 | Card Slot 3 (high) |
| 13 | Card Slot 4 (high) |
| 14 | Card Slot 5 (high) |
| 15 | S/W interrupt (Set Register `V[0..3]` = `$F`, Set `V[4..7]` = S/W Interrupt number)\* |

\* Call `jsr SW_INT` after loading S/W interrupt number ($0-F) into A

Have fun!
