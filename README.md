## **Hydra 16**

Project to create a multi-tasking 6502-based computer and basic operating system.

The code is built with **cc65** (https://cc65.github.io/).  The board schematics and PCB layouts are done in **KiCAD 9.0** (https://www.kicad.org).

### **Memory Map**
* PER-TASK memory map (each task has its own copy of this memory space, except for shared RAM pages, as discussed below)

| Start | End  | Description |
| :---- | :--- | :---------- |
| $00 | | RAM Page selection register (Pages `$00-$EF` are task-specific.  Pages `$F0-$FF` are shared between all tasks, and are further indexed using the U register, below) |
| $01  | | ROM Page selection register |
| $02 | $0F | Reserved Zero-page entries for future use |
| $10 | $FF | Remaining Zero-page |
| $0100 | $01FF | Hardware Stack |
| $0200 | $7DFF | Availabe Task RAM space |
| $7E00 | $7EFF | Monitor input buffer (256 bytes) |
| $7F00 | $7FFF | Onboard serial driver input buffer (256 bytes) |
| $8000 | $9FFF | Paged RAM (8K pages; task-specific and shared pages all show up here) |
| $A000 | $DFFF | Paged ROM (16K pages; ROMs are shared between all tasks, but the page selection is per-task, see `$01` above) |

* SHARED memory map (all tasks see the following areas the same)

| Start | End  | Description |
| :---- | :--- | :---------- |
| $E000 | $FFFF | BIOS/OS ROM paged area (indexed by the W register; see below) |
| $E000 | $E009 | RESET Vector entry point. Code saves `W` register to `ZP_W_SAVE` and then resets W to zero. This is replicated at the beginning of each BIOS page so that arbitrary W register values at startup/RESET result in the correct entry point being executed. |
| $E00A | $FEFF | Effective BIOS paged area.  Compiler segments (pages) `BIOS_P1 - BIOS_PF` are available for BIOS implementers to add more BIOS calls, corresponding to `W` register values of `$01 - $0F`, respectively. |

#### **I/O Ports**

There are 15 shared I/O ports on the Hydra, 16 1-byte registers per port, located from $FFF0-$FFEF.  Some ports are taken by the on-board devices.  Others are reserved for specific add-on cards (ports 2 & 3 for video, for example).  Still others are assigned to card slots, usually to correspond with the assigned IRQ numbers, with two I/O ports per slot.  I/O port assignment currently matches IRQ assignment for devices, but that is not a requirement, but it does make things easier.

The area from $FFF0 to $FFFF (that would have been reserved for I/O port 15) is the System port, where pseudo-registers T-W ($FFF0-$FFF3) and the interrupt vector addresses ($FFFA-$FFFF) live.  There are 6 unused bytes ($FFF4-$FFF9) that are reserved for future System expansion.

| Start | End  | Description |
| :---- | :--- | :---------- |
|  | | **I/O Ports** `$00-$0E` |
| $FF00 | $FF0F | Onboard VIA (65C22) |
| $FF10 | FF13 | Onboard ACIA (65C51) Serial |
| $FF14 | $FF1F | Unused |
| $FF20 | $FF3F | Reserved for future Video |
| $FF40 | $FF41 | Onboard YM2151 Sound generator |
| $FF42 | $FF4F | Unused |
| $FF50 | $FFEF | Unused (future I/O ports, expansion cards) |
|  | | **Pseudo-registers** |
| $FFF0 | | `T` Register (current task selector) |
| $FFF1 | | `U` Register (current shared memory macro-page) |
| $FFF2 | | `V` Register (interrupt vector selector) |
| $FFF3 | | `W` Register (BOIS page selection register) |
| $FFF4 | $FFF9 | Unused (future pseudo-register expansion) |
| | | **Vectors** (replicated on each BIOS page) |
| $FFFA | $FFFB | NMI Interrupt handler vector |
| $FFFC | $FFFD | Reset Vector (Set to `$E000`) |
| $FFFE | $FFFF | Interrupt Vector (see below) |


### **Interrupts**

Interrupt priority is lowest number == highest priority, so the S/W interrupt vector (#15) is the lowest priority.  
The interrupt vector (`$FFFE & $FFFF`) is actually a 16-entry pseudo-register indexed by either a) `V` register bits 0-3 if no hardware interrupt is active when a `BRK` instruction is executed, or b) the lowest numbered active interrupt request line (via the IRQ priority decoder circuit) if one or more H/W IRQs is active.  It is also indexed on write by `V` register bits 0-3, which is how the interrupt vectors are set by driver initialization functions.  
Hardware interrupts ignore `V` register bits 4-7, but sub-functions could be S/W triggered by setting those bits and calling `BRK`, and then checking them in the H/W interrupt vector, similar to how the S/W vector _will eventually_ work.  
An unused H/W interrupt could be used by S/W to add another S/W interrupt handler, giving another 16 S/W interrupts per IRQ, so long as those are not used by other H/W; _see IRQ Slot assignments, below_.  
**_All_** interrupts can be called via the S/W interrupt mechanism by setting `V` to the IRQ #, and then calling `BRK`.  Just remember that `V` is a shared, pseudo-register, so should be saved and restored (preferrably to `ZP_V_SAVE`) by each task whenever used.

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
