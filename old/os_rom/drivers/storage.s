.debuginfo

.segment "IO_P0"

; ****************************************************************************
; The storage task (STORAGE_TASK_NUM): a Resident task, started at boot like the drivers, that owns the
; SPI bus and serves /dev/sd and the HydraFS filesystem (/sd/N/...).  Its code is on BIOS ROM page 3
; (spi.s, sd.s, sd_srv.s) and page 6 (hfs_srv.s, hfs_write.s); this is the page 0 part: its DriverInfo,
; init, and the gates into pages 3 and 6.

STORAGE_DRIVER:
            .word       STORAGE_INIT                        ; DriverInfo::init
            .word       PIPE_STOP                           ; DriverInfo::stop (nothing to do)
            .word       STORAGE_DNAME                       ; DriverInfo::name
NamedHString STORAGE_DNAME, "STORAGE"
SD_NAME:    .byte   "sd", 0
HFS_NAME:   .byte   "hfs", 0

FAR_GATE_INLINE     STORAGE_INIT3,  PAGE3::STORAGE_INIT3,   3
FAR_GATE_INLINE     SD_SERVE,       PAGE3::SD_SERVE,        3
FAR_GATE_INLINE     HFS_SERVE,      PAGE6::HFS_SERVE,       6

; Runs in the storage task: the block cache and SPI (page 3), then register /dev/sd, /dev/spi and the HydraFS
; server (hfs; the shell mounts it at /sd).  (A card isn't touched until one of them is opened.)
; OUT: C = 0; or C = 1, .A = error
STORAGE_INIT:
            jsr         STORAGE_INIT3
            bcs         @done
            jsr         STORAGE_SPI
            bcs         @done
            LOAD_ADDR   SD_SERVE, ZP_TC_VEC
            lda         #<SD_NAME
            ldy         #>SD_NAME
            ldx         #STORAGE_TASK_NUM
            jsr         DEV_REGISTER
            bcs         @done
            LOAD_ADDR   HFS_SERVE, ZP_TC_VEC
            lda         #<HFS_NAME
            ldy         #>HFS_NAME
            ldx         #STORAGE_TASK_NUM
            jmp         DEV_REGISTER

@done:
            rts

; Register /dev/spi (page D: spi_srv.s), served in the storage task.  (After the thunks, where page 0 has room.)
.pushseg
.segment "BIOS"
SPI_NAME:   .byte   "spi", 0
FAR_GATE_INLINE     SPI_SERVE,      PAGED::SPI_SERVE,       $D

STORAGE_SPI:
            LOAD_ADDR   SPI_SERVE, ZP_TC_VEC
            lda         #<SPI_NAME
            ldy         #>SPI_NAME
            ldx         #STORAGE_TASK_NUM
            jmp         DEV_REGISTER
.popseg
