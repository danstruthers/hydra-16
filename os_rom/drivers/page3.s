.debuginfo

; ****************************************************************************
; BIOS ROM page 3 (W = 3): storage (SPI, the SD card, /dev/sd).  The HydraFS server, in the same task, is on
; page 6 (io/page6.s).
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
FAR_GATE_INLINE     MM_SET_FLOOR,   ::MM_SET_FLOOR,         0   ; (HydraFS's scratch page)

; Gates to the HydraFS server's routines /dev/sd uses (page 6; the aliases are in all.s)
FAR_GATE_INLINE     HFS_FORGET,     ::HFS_FORGET_P6,        6   ; (A card started again)
FAR_GATE_INLINE     HFS_META_NEW,   ::HFS_META_NEW_P6,      6   ; (Format and label: hfs_format.s)
FAR_GATE_INLINE     HFS_META_AT,    ::HFS_META_AT_P6,       6
FAR_GATE_INLINE     HFS_META_CHANGED, ::HFS_META_CHANGED_P6, 6
FAR_GATE_INLINE     HFS_FINISH,     ::HFS_FINISH_P6,        6
FAR_GATE_INLINE     HFS_SHR,        ::HFS_SHR_P6,           6
FAR_GATE_INLINE     HFS_VOLUME,     ::HFS_VOLUME_P6,        6
FAR_GATE_INLINE     HFS_SB_GET,     ::HFS_SB_GET_P6,        6
FAR_GATE_INLINE     HFS_PG_START,   ::HFS_PG_START_P6,      6   ; (A full format's progress)
FAR_GATE_INLINE     HFS_PG_TICK,    ::HFS_PG_TICK_P6,       6
FAR_GATE_INLINE     HFS_PG_END,     ::HFS_PG_END_P6,        6
FAR_GATE_INLINE     HFS_CHECK,      ::HFS_CHECK_P6,         6   ; ("check")
FAR_GATE_INLINE     HFS_CTL_LINES,  ::HFS_CTL_LINES_P6,     6   ; (The ctl file's text: HydraFS's lines)
