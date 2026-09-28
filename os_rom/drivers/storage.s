.debuginfo

.segment "IO_P0"

; ****************************************************************************
; The storage task (STORAGE_TASK_NUM): a Resident task, started at boot like the drivers, that owns the
; SPI bus and serves /dev/sd (and later the HydraFS filesystem).  Its code is on BIOS ROM page 3 (spi.s,
; sd.s, sd_srv.s); this is the page 0 part: its DriverInfo, init, and the gates into page 3.

STORAGE_DRIVER:
            .word       STORAGE_INIT                        ; DriverInfo::init
            .word       PIPE_STOP                           ; DriverInfo::stop (nothing to do)
            .word       STORAGE_DNAME                       ; DriverInfo::name
NamedHString STORAGE_DNAME, "STORAGE"
SD_NAME:    .byte   "sd", 0

FAR_GATE_INLINE     STORAGE_INIT3,  PAGE3::STORAGE_INIT3,   3
FAR_GATE_INLINE     SD_SERVE,       PAGE3::SD_SERVE,        3

; Runs in the storage task: the block cache and SPI (page 3), then register /dev/sd.  (The card isn't
; touched until /dev/sd is opened.)  OUT: C = 0; or C = 1, .A = error
STORAGE_INIT:
            jsr         STORAGE_INIT3
            bcs         @done
            LOAD_ADDR   SD_SERVE, ZP_TC_VEC
            lda         #<SD_NAME
            ldy         #>SD_NAME
            ldx         #STORAGE_TASK_NUM
            jmp         DEV_REGISTER

@done:
            rts
