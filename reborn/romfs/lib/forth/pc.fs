\ pc.fs - /pc, a folder on the PC, through the serial port (#P: the PC tool, ../sim/tools/hydrapc.js): lib pc.
\ hylang's pc library's word.  /pc's files are files, for everything else (open-file, read-file ...).  Only the words
\ forth starts with.

: pc? ( -- flag )                                 \ The PC tool answers? (None: a second, then false)
  s" /pc" r/o open-file if drop false else close-file drop true then ;
