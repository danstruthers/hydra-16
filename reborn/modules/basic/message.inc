; global messages: "error", "in", "ready", "break"

.segment "CODE"

QT_ERROR:
        .byte   " ERROR"
        .byte   0

QT_IN:
        .byte   " IN "
        .byte   $00

QT_OK:
		.byte   CR,LF,"OK",CR,LF
		.byte	0
QT_BREAK:
        .byte CR,LF,"BREAK",$00
