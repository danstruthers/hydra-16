; ****************************************************************************
; zeropage.inc - BASIC's variables (included by basic.s).  In the program's zero page ($22-$7F) those it reads
; through ((zp),y), names as zero-page addresses (ldx #FAC, lda #<ARG and ldy #0) or indexes as one block (REASON's
; TEMP1 to FAC, the floating point's TMPEXP to SERLEN, the descriptors' TEMPST): Microsoft's block, in its order, less
; four that went to the BSS; CHRGET's TXTPTR, which was in its code; and hyio.inc's pointer.  The rest in the BSS.

.zeropage
TXTPTR:
	.res 2
LINNUM:
TXPSV:
	.res 2
TEMPPT:
	.res 1
LASTPT:
	.res 2
TEMPST:
	.res 9
INDEX:
	.res 2
DEST:
	.res 2
RESULT:
	.res BYTES_FP
RESULT_LAST = RESULT + BYTES_FP-1
TXTTAB:
	.res 2
VARTAB:
	.res 2
ARYTAB:
	.res 2
STREND:
	.res 2
FRETOP:
	.res 2
FRESPC:
	.res 2
MEMSIZ:
	.res 2
DATPTR:
	.res 2
INPTR:
	.res 2
VARNAM:
	.res 2
VARPNT:
	.res 2
FORPNT:
	.res 2
LASTOP:
	.res 2
CPRTYP:
	.res 1
FNCNAM:
TEMP3:
	.res 2
DSCPTR:
		.res 3
DSCLEN:
	.res 2
JMPADRS			:= DSCLEN + 1
Z52:
	.res 1
ARGEXTENSION:
	.res 1
TEMP1:
	.res 1
HIGHDS:
	.res 2
HIGHTR:
	.res 2
TEMP2:
	.res 1
INDX:
TMPEXP:
	.res 1
EXPON:
	.res 1
LOWTR:
LOWTRX:
	.res 1
EXPSGN:
	.res 1
FAC:
	.res BYTES_FP
FAC_LAST = FAC + BYTES_FP-1
FACSIGN:
	.res 1
SERLEN:
	.res 1
SHIFTSIGNEXT:
	.res 1
ARG:
	.res BYTES_FP
ARG_LAST = ARG + BYTES_FP-1
ARGSIGN:
	.res 1
STRNG1:
	.res 2
SGNCPR = STRNG1
FACEXTENSION = STRNG1+1
STRNG2:
	.res 2
IOPTR:                                      ; (hyio.inc's: a string printed, a file's name)
	.res 2
KWPTR:                                      ; (The tokenizer's and LIST's: a keyword in the table)
	.res 2

.bss
GORESTART:                                  ; (jmp RESTART)
	.res 3
GOSTROUT:                                   ; (jmp STROUT)
	.res 3
GOAYINT:
	.res 2
GOGIVEAYF:
	.res 2
POSX:
	.res 1
Z17:
	.res 1
Z18:
	.res 1
CHARAC:
	.res 1
ENDCHR:
	.res 1
EOLPNTR:
	.res 1
DIMFLG:
	.res 1
VALTYP:
	.res 2
DATAFLG:
	.res 1
SUBFLG:
	.res 1
INPUTFLG:
	.res 1
CPRMASK:
	.res 1
Z14:
	.res 1
CURLIN:
	.res 2
OLDLIN:
	.res 2
OLDTEXT:
	.res 2
Z8C:
	.res 2
RNDSEED:                                    ; (RND's seed: in CHRGET's code, which was in the zero page)
	.res BYTES_FP

.segment "LINEBUF"                          ; (A page of its own: basic.cfg)
	.res 8
INPUTBUFFER:
	.res INBUF_MAX + 8
.code
