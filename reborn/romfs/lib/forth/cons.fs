\ cons.fs - the console's windows, by their files (#c, /dev): lib cons.  hylang's cons library's words.  /dev/wctl:
\ new (a window made, and shown), current N (window N shown); it reads as the windows, a line each (* the one shown).
\ $window is this shell's window (/env/window; none: 0).  The bell is facility's beep; the keys' raw mode is key's
\ and ekey's (they set it, and a line read ends it).  A failure THROWs.  Only the words forth starts with.

create cons-buf 256 allot                         \ What's read

: cons-wctl ( c-addr u -- ) s" /dev/wctl" w/o open-file throw >r r@ write-file r> close-file throw throw ;

: window ( -- n )                                 \ This one's ($window; none: 0)
  s" /env/window" r/o open-file if drop 0 exit then
  >r cons-buf 8 r@ read-file r> close-file throw throw  cons-buf swap 0. 2swap >number 2drop drop ;
: windows ( -- c-addr u )                         \ The windows, a line each (* the one shown)
  s" /dev/wctl" r/o open-file throw >r cons-buf 256 r@ read-file r> close-file throw throw cons-buf swap ;
: new-window ( -- ) s" new" cons-wctl ;          \ A window made, and shown (its shell: wstart's)
: show-window ( n -- )                            \ Window n shown
  base @ >r decimal 0 <# #s s" current " holds #> r> base ! cons-wctl ;
