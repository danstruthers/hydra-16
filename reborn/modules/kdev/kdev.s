; ****************************************************************************
; kdev - HydraOS's: the base's kdev (../../../base/modules/kdev/kdev.s: the kernel's devices), and in the same driver,
; so they take no task of their own, HydraOS's small devices on the VIA's port A (kdevxt.inc: their letters; kdevx.inc):
;   #g      the GPIO pins, and #i, the I2C bus on two of them (gpio.inc: once a driver of its own, gpio)
;   the input controller, the Vera X's keyboard and mouse, on that bus (input.inc: once a program, input): looked at
;           from this driver's init, which doesn't end while there is one (SRV_NAP: a driver's own work between its
;           calls)

KDEV_X          = 1

.include "../../../base/modules/kdev/kdev.s"
