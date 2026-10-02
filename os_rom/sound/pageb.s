.debuginfo

; ****************************************************************************
; BIOS ROM page B (W = $B): sound.  The YM2151's library (ym.s), the sound driver's file server (/dev/snd:
; snd_srv.s), the test tune (snd_test.s) and the console bell (beep.s).  The sound driver's page 0 part
; (drivers/sound.s) and the serial driver (the bell) reach it through page 0's gates.
;
;   This file is included inside `.scope PAGEB` (see all.s), before the rest of page B, so the gate labels
;   below take precedence over the routines of the same name on other pages.

.segment "GATES_PB"

; Gates to page 0
FAR_GATE_INLINE     IO_SRV_MAP,     ::IO_SRV_MAP,           0
FAR_GATE_INLINE     IO_SRV_UNMAP,   ::IO_SRV_UNMAP,         0
FAR_GATE_INLINE     TASK_RUN,       ::TASK_RUN,             0   ; (The test tune's player)
FAR_GATE_INLINE     TASK_SIGNAL,    ::TASK_SIGNAL,          0
FAR_GATE_INLINE     TASK_SLEEP,     ::TASK_SLEEP,           0   ; (The test tune's timing)
FAR_GATE_INLINE     TICKS_GET,      ::TICKS_GET,            0   ; (The sound clock's numbers: SND_READ)
FAR_GATE_INLINE     IRQ_REGISTER,   ::IRQ_REGISTER,         0   ; (SOUND_INIT: the sound chip's handler)

; ... and to the IO layer (page 2)
FAR_GATE_INLINE     IO_SRV_COUNT,   PAGE2::IO_SRV_COUNT,    2
FAR_GATE_INLINE     STAT_ZERO,      PAGE2::STAT_ZERO,       2
