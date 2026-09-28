.debuginfo

; ****************************************************************************
; BIOS ROM page 3 (W = 3): storage (SPI, the SD card, its server, and later the FAT32 server).
;
;   This file is included inside `.scope PAGE3` (see all.s), before the rest of page 3, so the gate labels
;   below take precedence over the page 0 routines of the same name for all page 3 code.  Page 3 code
;   runs with W = 3; it's entered through the compact gates in storage.s (page 0).

.segment "GATES_P3"

; Gates from page 3 to page 0 routines (they run in the current task: the storage task)
FAR_GATE_INLINE     IO_SRV_MAP,     ::IO_SRV_MAP,           0
FAR_GATE_INLINE     IO_SRV_UNMAP,   ::IO_SRV_UNMAP,         0
FAR_GATE_INLINE     MM_ALLOC,       ::MM_ALLOC,             0
FAR_GATE_INLINE     MM_LOCK,        ::MM_LOCK,              0
