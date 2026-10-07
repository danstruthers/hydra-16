; ****************************************************************************
; toollib.inc - toollib.s's constants and zero page: included at the top of a tool's source (after hydra.inc),
; so the tool's own code addresses them as zero page; toollib.s goes at its end.

TL_DEPTH        = 16            ; tl_walk's directories, one in another, at most

.pushseg

.zeropage
tl_arg:     .res        2                                   ; The argument (main's list: tl_next on)
tl_pp:      .res        2                                   ; A path (tl_pcat, tl_pcut)
tl_q:       .res        2                                   ; (A pointer of toollib's own)

.popseg
