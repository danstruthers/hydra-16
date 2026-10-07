\ spi.fs - the SPI devices, by their files (#S, /dev/spi): lib spi.  hylang's spi library's words, its order (the
\ device first: 0-f, 0-7 the board's headers, 8-f the slots').  A device's files: data (a write: its bytes sent with
\ the device selected, a transaction, the bytes it sent back meanwhile kept; a read: the bytes kept, or with none,
\ as many clocked in, sending $FF) and ctl (mode 0 or mode 3).  Open by one at a time, and not while it's a card's
\ (E_BUSY).  A failure THROWs.  Only the words forth starts with (File Access's).

create spi-buf 32 allot   variable spi-n          \ A name or a line being made

: spi{ ( -- ) 0 spi-n ! ;
: spi+ ( c-addr u -- ) tuck spi-buf spi-n @ + swap move spi-n +! ;
: spi+c ( char -- ) spi-buf spi-n @ + c! 1 spi-n +! ;
: spi-file ( dev c-addr u -- c-addr2 u2 )         \ Device dev's file
  rot spi{ s" /dev/spi/" spi+ 15 and s" 0123456789abcdef" drop + c@ spi+c [char] / spi+c spi+ spi-buf spi-n @ ;
: spi-data ( dev fam -- fid ) >r s" data" spi-file r> bin open-file throw ;

: spi ( dev c-addr u -- )                         \ A transaction: the bytes sent, and those the device sent back
  rot r/w spi-data >r 2dup r@ write-file throw r@ read-file throw drop r> close-file throw ;   \   in their place
: spi-read ( dev c-addr u -- )                    \ u bytes clocked in ($FF sent)
  rot r/o spi-data >r r@ read-file throw drop r> close-file throw ;
: spi-mode ( dev mode -- )                        \ 0 (SCLK idles low) or 3 (high)
  swap s" ctl" spi-file w/o open-file throw >r
  base @ >r decimal 0 <# #s s" mode " holds #> r> base ! r@ write-file throw r> close-file throw ;
