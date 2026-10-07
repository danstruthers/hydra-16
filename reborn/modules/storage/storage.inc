; ****************************************************************************
; storage.inc - the storage driver's constants, and what its two objects share: storage.s (its first bank: SPI, the
; disks, #S and #d, srvlib) and hfs.s (its second: HydraFS, #f).  Each includes it: a name .global here is one
; object's, used by the other.  A routine in the other bank is called through FAR1 or FAR2 (hyx2.inc); data in
; RAM is either's.

SPI_DEVS        = 16
DISKS           = 20                                        ; The disks: 0-15 the SPI devices' cards ...
DISK_X          = 16                                        ;   the ROM disk ...
DISK_R          = 17                                        ;   the RAM disk ...
DISK_S          = 18                                        ;   the shared one ...
DISK_V          = 19                                        ;   and the Vera X's card (on the VERA's SPI)
DS_SDSC         = 1                                         ; A disk's state (0: not started): a card, standard
DS_SDHC         = 2                                         ;   capacity or high (SDHC, SDXC) ...
DS_ROM          = 3                                         ;   the ROM disk ...
DS_RAM          = 4                                         ;   a RAM disk ...
DS_SRAM         = 5                                         ;   the shared one
BLOCK           = 512

; ---- storage.s's, for HydraFS
.globalzp   dk, lba, bufp, src, dst, n, pos, done           ; (HydraFS's SD_DEV, SD_LBA, SD_BUF ...)
.global     blk, c_ok, d_state, d_blocks, d_open
.global     blk_get, blk_read, blk_write, disk_start        ; (FAR1)
.global     blk_part, blk_plen                              ; (A data write's part of its block: blk_write's)
.globalzp   srv_tlen, srv_argn                              ; srvlib's: a ctl file's text, and a ctl command's
.global     srv_text, srv_ctl, srv_argp                     ;   words
.globalzp   hyx2_to, hyx2_a                                 ; The trampolines between the banks (hyx2.inc)
.global     hyx2_far1, hyx2_far2

; ---- hfs.s's, for storage.s (FAR2)
.global     hfs_init, hfs_serve, hfs_forget, hfs_in_use, hfs_ctl_lines, hfs_format, hfs_format_ram, hfs_label
.global     hfs_check
