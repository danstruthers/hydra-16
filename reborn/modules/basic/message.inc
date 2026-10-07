; global messages: "error", "in", "ready", "break" (a new line is LF alone, as the system has it)

.segment "CODE"

QT_ERROR:
        .byte   " ERROR"
        .byte   0

QT_IN:                                  ; (The number after it has its sign's space)
        .byte   " IN"
        .byte   $00

QT_OK:
		.byte   LF,"OK",LF
		.byte	0
QT_BREAK:
        .byte LF,"BREAK",$00
