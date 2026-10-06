\ gpio.fs - the VIA's port A on header J27, by its files (#g, /dev/gpio): lib gpio.  hylang's gpio library's words,
\ its order (the pin first).  Its files: 0-7 (a pin: reads as 0 or 1; a write of 0 or 1 sets it, the pin an
\ output), port (all 8: a byte), ctl (in N, out N, ddr N, ca1 rise or fall, ca2 0, 1 or in; it reads as the pins and
\ the handshake lines), ca1 (a read waits for CA1's next edge, then gives the edges counted).  A failure THROWs.
\ Only the words forth starts with (File Access's), so it's an example of driving the pins by hand too.

create gpio-buf 64 allot   variable gpio-n          \ A name or a line being made
create gpio-text 128 allot                          \ What's read

: gpio{ ( -- ) 0 gpio-n ! ;
: gpio+ ( c-addr u -- ) tuck gpio-buf gpio-n @ + swap move gpio-n +! ;
: gpio+n ( n -- ) base @ >r decimal 0 <# #s #> gpio+ r> base ! ;
: gpio} ( -- c-addr u ) gpio-buf gpio-n @ ;
: gpio-file ( c-addr u -- c-addr2 u2 ) gpio{ s" /dev/gpio/" gpio+ gpio+ gpio} ;
: gpio-pin ( pin -- c-addr u ) gpio{ s" /dev/gpio/" gpio+ gpio+n gpio} ;
: gpio-read ( c-addr u -- c-addr2 u2 )             \ A file's text (128 bytes at most)
  r/o open-file throw >r gpio-text 128 r@ read-file r> close-file throw throw gpio-text swap ;
: gpio-write ( c-addr u c-addr2 u2 -- )            \ Text c-addr u written to the file c-addr2 u2
  w/o open-file throw >r r@ write-file r> close-file throw throw ;
: gpio-ctl ( c-addr u -- ) s" /dev/gpio/ctl" gpio-write ;
: gpio-num ( c-addr u -- n ) 0. 2swap >number 2drop drop ;

: gpio ( pin -- level ) gpio-pin gpio-read gpio-num ;               \ Its level, 0 or 1
: gpio! ( pin level -- ) 0<> 1 and [char] 0 + gpio-text c! gpio-text 1 rot gpio-pin gpio-write ;   \ The pin an output, set
: gpio-in ( pin -- ) >r gpio{ s" in " gpio+ r> gpio+n gpio} gpio-ctl ;          \ The pin an input
: gpio-out ( pin -- ) >r gpio{ s" out " gpio+ r> gpio+n gpio} gpio-ctl ;        \ An output
: gpio-port ( -- byte )                             \ All 8 pins' levels
  s" port" gpio-file r/o bin open-file throw >r gpio-text 1 r@ read-file r> close-file throw throw drop gpio-text c@ ;
: gpio-port! ( byte -- )                            \ The outputs' levels (the inputs keep theirs)
  gpio-text c! gpio-text 1 s" port" gpio-file w/o bin open-file throw >r r@ write-file r> close-file throw throw ;
: gpio-ddr! ( byte -- ) >r gpio{ s" ddr " gpio+ r> gpio+n gpio} gpio-ctl ;      \ Bit n 1: pin n an output
: gpio-ca1! ( rise? -- ) if s" ca1 rise" else s" ca1 fall" then gpio-ctl ;    \ CA1's active edge
: gpio-ca2! ( n -- )                                \ CA2: 0 low, 1 high, -1 an input
  dup 0< if drop s" ca2 in" gpio-ctl exit then
  >r gpio{ s" ca2 " gpio+ r> 0<> 1 and gpio+n gpio} gpio-ctl ;
: gpio-wait ( -- count ) s" ca1" gpio-file gpio-read gpio-num ;    \ CA1's next edge: the edges counted
: gpio-state ( -- c-addr u ) s" ctl" gpio-file gpio-read ;          \ The pins and lines, a line each
