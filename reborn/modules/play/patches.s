; ****************************************************************************
; patches.s - the sound driver's patches, drum map and volume curve (modules/snd/patches.s, the X16's), for play's
; scores: the instruments a score names (gm N), its drums (x N) and its volumes (v N), as hysong.js has them.  The
; patches and the drum map are in play's second bank (RODATA2: mml.inc's pt_copy and pt_drum read them), the volume
; curve in its first.

PATCHES_FAR     = 1
.include "snd/patches.s"
