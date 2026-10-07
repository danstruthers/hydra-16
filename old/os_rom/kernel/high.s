.debuginfo

; ****************************************************************************
; Each BIOS ROM page's room above COMMON: $FE00-$FEFF, below the I/O space ($FF00).  A segment for each
; page (os_rom_C02.cfg: HIGH_P0 ... HIGH_PF; page 1's is FORTH_TOP, HyForth's), declared here so the link
; knows them while they're empty.  For code or data that fits nowhere else on its page: .pushseg, .segment
; "HIGH_Pn", .popseg.  (Page 0's held WOZMON, which is on page 4 now.)
.segment "HIGH_P0"
.segment "HIGH_P2"
.segment "HIGH_P3"
.segment "HIGH_P4"
.segment "HIGH_P5"
.segment "HIGH_P6"
.segment "HIGH_P7"
.segment "HIGH_P8"
.segment "HIGH_P9"
.segment "HIGH_PA"
.segment "HIGH_PB"
.segment "HIGH_PC"
.segment "HIGH_PD"
.segment "HIGH_PE"
.segment "HIGH_PF"
