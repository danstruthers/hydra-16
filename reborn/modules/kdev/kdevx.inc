; ****************************************************************************
; kdevx.inc - HydraOS's own devices in kdev (the base's kdev.s includes this, with KDEV_X): GPIO and I2C (gpio.inc)
; and the input controller (input.inc), each in a scope of its own (their names are theirs; srvlib's and the
; kernel's are everyone's).  What kdev.s wants of it is at the end: kx_init, kx_irq, kx_opened, kx_clunked, kx_stat,
; and the trees kdevxt.inc names.

.scope gpio
.include "gpio.inc"
.endscope

.scope input
.include "input.inc"
.endscope

.code

kx_tree_gpio    = gpio::tree_gpio
kx_tree_i2c     = gpio::tree_i2c
kx_irq          = gpio::irq                                 ; (CA1's line: #g's /ca1)
kx_opened       = gpio::opened                              ; (#g's alone: it looks at the request's device)
kx_clunked      = gpio::clunked

; A stat record made: a module's file's length (kdev's own), a pin's or /port's (#g's)
kx_stat:
            jsr         mod_stat
            jmp         gpio::stat

; The end of kdev's init: port A's (C = 1: kdev's init fails); then the input controller looked for and, if it's
; there, looked at for good (SRV_NAP between looks: kdev serves its calls meanwhile).  With none, kdev's init ends
kx_init:
            jsr         gpio::init
            bcs         :+
            jmp         input::main
:
            rts
