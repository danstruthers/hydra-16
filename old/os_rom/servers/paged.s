.debuginfo

; ****************************************************************************
; BIOS ROM page D (W = $D): /dev/spi, the SPI devices as files (spi_srv.s), served in the storage task;
; /dev/gpio, the VIA's port A (gpio_srv.s), served in its client's task; and /pc, a folder on the PC over the
; serial port (pc_srv.s), served in the serial task; and / and /dev as directories (root_srv.s), served in their
; client's task.  Their gates: the IO layer's server
; calls and the MMU (page 0), and the storage page's SPI loops (page 3: spi.s).
;
;   This file is included inside `.scope PAGED` (see all.s), before the rest of page D, so the gate labels
;   below take precedence over the routines of the same name on other pages.

.segment "GATES_PD"

; Gates to page 0
FAR_GATE_INLINE     IO_SRV_MAP,     ::IO_SRV_MAP,           0
FAR_GATE_INLINE     IO_SRV_UNMAP,   ::IO_SRV_UNMAP,         0
FAR_GATE_INLINE     MM_ALLOC,       ::MM_ALLOC,             0
FAR_GATE_INLINE     MM_LOCK,        ::MM_LOCK,              0
FAR_GATE_INLINE     TICKS_GET,      ::TICKS_GET,            0   ; (/pc: pc_srv.s)
FAR_GATE_INLINE     DEV_REGISTER,   ::DEV_REGISTER_FAR,     0   ; (It reads the name on the caller's page)

; ... and to the SPI bus (page 3), a request's bytes at a time
FAR_GATE_INLINE     SPI_XFER_N,     PAGE3::SPI_XFER_N,      3
FAR_GATE_INLINE     SPI_RECV_N,     PAGE3::SPI_RECV_N,      3
