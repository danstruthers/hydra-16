.debuginfo

; ****************************************************************************
; BIOS ROM page 6 (W = 6): the HydraFS server (hfs_srv.s, hfs_write.s).  It runs in the storage task, on
; the SD card's block layer, which stays on page 3 with SPI and /dev/sd: a block's far call costs about
; 120 cycles, next to about 150,000 to move the block.
;
;   This file is included inside `.scope PAGE6` (see all.s), before the rest of page 6, so the gate labels
;   below take precedence over the routines of the same name on other pages.  Page 3 reaches page 6 through
;   the global aliases after the scope (HFS_FORGET_P6, ...): PAGE3 is assembled first, and a scope can't be
;   named before it's been seen.

.segment "GATES_P6"

; Gates from page 6 to the block layer (page 3)
FAR_GATE_INLINE     SD_CACHE_LOAD,  PAGE3::SD_CACHE_LOAD,   3
FAR_GATE_INLINE     SD_READ_BLOCK,  PAGE3::SD_READ_BLOCK,   3
FAR_GATE_INLINE     SD_WRITE_BLOCK, PAGE3::SD_WRITE_BLOCK,  3
FAR_GATE_INLINE     SD_START,       PAGE3::SD_START,        3
FAR_GATE_INLINE     SD_CARD_SIZE,   PAGE3::SD_CARD_SIZE,    3

; ... and to page 0 (they run in the current task: the storage task)
FAR_GATE_INLINE     IO_SRV_MAP,     ::IO_SRV_MAP,           0
FAR_GATE_INLINE     IO_SRV_UNMAP,   ::IO_SRV_UNMAP,         0
FAR_GATE_INLINE     MM_ALLOC,       ::MM_ALLOC,             0   ; (The check's buffer)
FAR_GATE_INLINE     MM_LOCK,        ::MM_LOCK,              0
