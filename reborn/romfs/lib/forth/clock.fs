\ clock.fs - the clock and the DS1747, by their files (#t, /dev): lib clock.  hylang's clock library's words.
\ /dev/time reads as the time (2026-10-03 15:04:05), and a write of that sets it (the chip too); /dev/rtc reads as
\ what the boot found: running (and battery low), stopped (set the time), or none.  The time as numbers is
\ facility's time&date; date is the program.  A failure THROWs.  Only the words forth starts with.

create clock-buf 64 allot

: set-date ( c-addr u -- )                        \ The clock (and the chip) set: "2026-10-04 12:00:00"
  s" /dev/time" w/o open-file throw >r r@ write-file r> close-file throw throw ;
: rtc ( -- c-addr u )                             \ The chip: running, stopped or none (and battery low)
  s" /dev/rtc" r/o open-file throw >r clock-buf 64 r@ read-file r> close-file throw throw clock-buf swap ;
