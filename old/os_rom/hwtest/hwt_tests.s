; ****************************************************************************
; The hardware test's tests, in the order "all tests" runs them: each is its key on the menu, its routine,
; its name, and flags (HWT_NOT_ALL: only on its own).  (Included by hwtest.s, inside `.scope HWTEST`.)

HWT_TEST_SIZE       = 6
HWT_NOT_ALL         = $80

.macro HWT_TEST key, routine, name, flags
            .byte       key
            .word       routine
            .word       name
            .byte       flags
.endmacro

HWT_TESTS:
            HWT_TEST    '1', HWT_T_CPU,     HWT_N_CPU,      0
            HWT_TEST    '2', HWT_T_REGS,    HWT_N_REGS,     0
            HWT_TEST    '3', HWT_T_SHARED,  HWT_N_SHARED,   0
            HWT_TEST    '4', HWT_T_BANKREG, HWT_N_BANKREG,  0
            HWT_TEST    '5', HWT_T_TASKRAM, HWT_N_TASKRAM,  0
            HWT_TEST    '6', HWT_T_MODULES, HWT_N_MODULES,  0
            HWT_TEST    '7', HWT_T_BIOS,    HWT_N_BIOS,     0
            HWT_TEST    '8', HWT_T_PAGED,   HWT_N_PAGED,    0
            HWT_TEST    'I', HWT_T_IRQS,    HWT_N_IRQS,     0
            HWT_TEST    'V', HWT_T_VIA,     HWT_N_VIA,      0
            HWT_TEST    'Y', HWT_T_YM,      HWT_N_YM,       0
            HWT_TEST    'K', HWT_T_CLOCK,   HWT_N_CLOCK,    0
            HWT_TEST    'S', HWT_T_ACIA,    HWT_N_ACIA,     0
            HWT_TEST    'P', HWT_T_SPI,     HWT_N_SPI,      0
            HWT_TEST    'C', HWT_T_I2C,     HWT_N_I2C,      0
            HWT_TEST    'X', HWT_T_SLOTS,   HWT_N_SLOTS,    0
            HWT_TEST    'H', HWT_T_HOLD,    HWT_N_HOLD,     HWT_NOT_ALL
            .byte       0

HWT_N_CPU:      .byte   "CPU", 0
HWT_N_REGS:     .byte   "T U V W registers", 0
HWT_N_SHARED:   .byte   "shared RAM", 0
HWT_N_BANKREG:  .byte   "RAM bank registers", 0
HWT_N_TASKRAM:  .byte   "task RAM", 0
HWT_N_MODULES:  .byte   "RAM modules", 0
HWT_N_BIOS:     .byte   "BIOS ROM", 0
HWT_N_PAGED:    .byte   "paged ROM", 0
HWT_N_IRQS:     .byte   "interrupts", 0
HWT_N_VIA:      .byte   "VIA", 0
HWT_N_YM:       .byte   "sound chip (YM2151)", 0
HWT_N_CLOCK:    .byte   "CPU clock", 0
HWT_N_ACIA:     .byte   "serial port (ACIA)", 0
HWT_N_SPI:      .byte   "SPI devices", 0
HWT_N_I2C:      .byte   "I2C bus", 0
HWT_N_SLOTS:    .byte   "slot cards", 0
HWT_N_HOLD:     .byte   "hold a task (probe)", 0

.include "hwt_regs.s"
.include "hwt_mem.s"
.include "hwt_rom.s"
.include "hwt_dev.s"
