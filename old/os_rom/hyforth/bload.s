;
;  BLOAD test code
;
;  ALL binary libraries loadable with 'bload' MUST be Forth words,
;   (not jsr subroutines), fully relocatable.  That means NO jumps
;   or branches to labels!  Calculate all jumps and branches accordingly
;   and use numercal offsets.  jsr's and jmp's to core primitive labels
;   (next, keeps, this, etc) or ROM locations will work fine.
;
;  Format below must be rigidly followed, with length of name followed
;   by that many bytes for the name.  'bit #0' ($89 $00) must be
;   the first instruction in any native code word loaded past
;   'exit'.  Termination must be $00 $00 for each word, with "END"
;   to designate the end of a 'library'.
;
test00:
.byte 6
.byte "BLtest"
     SKIPNEXT
     .byte 0
     PRINT_CRLF
     PRINT_CHAR #'T', #'E', #'S', #'T'
     PRINT_CRLF
     jmp next
.byte 0, 0
.byte 7
.byte "BLtest2"
     SKIPNEXT
     .byte 0
     PRINT_CRLF
     PRINT_CHAR #'T', #'E', #'S', #'T', #'Y'
     PRINT_CRLF
     jmp next
.byte 0, 0
.byte "END"
bload0_end: