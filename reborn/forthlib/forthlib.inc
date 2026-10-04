; ****************************************************************************
; forthlib.inc - a HyForth library's definitions (forthlib/NAME.s includes it first): the system's, forth's
; (fdefs.inc: HEADER and the rest), and the core's labels (forthcore.inc, tools/forthlib.js's: its routines, its
; variables, its zero page, all at fixed addresses).  A library is the core's form of a word set: its headers in a
; chain (HEADER's hdr_N, the first linked to the word list's last as it loads), each word's code after its header,
; its own variables in its BSS.  lib_init, if it has one, runs once it's loaded.  It runs from the dictionary (RAM),
; relocated to wherever HERE was.

.include "hydra.inc"
.include "hw.inc"
.include "macros.inc"
.include "fdefs.inc"
.include "forthcore.inc"

.code
