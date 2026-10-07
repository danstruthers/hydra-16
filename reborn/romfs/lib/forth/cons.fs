\ cons.fs - the console's windows, by their files (#c, /dev): lib cons.  hylang's cons library's words.  /dev/wctl:
\ new (a window made, and shown), current N (window N shown); it reads as the windows, a line each (* the one shown).
\ $window is this shell's window (/env/window; none: 0); window-label, window-status and window-ctl its chrome (W4:
\ /dev/label, wctl's status and any wctl line; a line each, 55 characters).  The bell is facility's beep; the keys' raw
\ mode is key's and ekey's (they set it, and a line read ends it).  A failure THROWs.

create cons-buf 256 allot                         \ What's read, or a line written
variable cons-n

: cons-to ( c-addr u c-addr2 u2 -- )               \ Text to file c-addr2 u2, one write
  w/o open-file throw >r r@ write-file r> close-file throw throw ;
: cons-wctl ( c-addr u -- ) s" /dev/wctl" cons-to ;
: cons+ ( c-addr u -- ) dup >r cons-buf cons-n @ + swap move r> cons-n +! ;
: cons-line ( c-addr u c-addr2 u2 -- c-addr3 u3 )  \ c-addr2 u2, then c-addr u, then a line end
  0 cons-n ! cons+ cons+ 10 cons-buf cons-n @ + c! 1 cons-n +! cons-buf cons-n @ ;

: window ( -- n )                                 \ This one's ($window; none: 0)
  s" /env/window" r/o open-file if drop 0 exit then
  >r cons-buf 8 r@ read-file r> close-file throw throw  cons-buf swap 0. 2swap >number 2drop drop ;
: windows ( -- c-addr u )                         \ The windows, a line each (* the one shown)
  s" /dev/wctl" r/o open-file throw >r cons-buf 256 r@ read-file r> close-file throw throw cons-buf swap ;
: new-window ( -- ) s" new" cons-wctl ;          \ A window made, and shown (its shell: wstart's)
: show-window ( n -- )                            \ Window n shown
  base @ >r decimal 0 <# #s s" current " holds #> r> base ! cons-wctl ;
: window-label ( c-addr u -- )                    \ Its title (OSC 2's too; empty: its program's name again)
  s" " cons-line s" /dev/label" cons-to ;
: window-status ( c-addr u -- ) s" status " cons-line cons-wctl ;    \ Its status line (the footer's %s)
: window-ctl ( c-addr u -- ) s" " cons-line cons-wctl ;              \ A line to its wctl (chrome screen off ...)
