\ i2c.fs - the I2C bus on port A's PA0 and PA1, by its files (#i, /dev/i2c): lib i2c.  hylang's i2c library's words,
\ its order (the device's address first).  Its files: NN (the device at address NN, two hex digits: a write sends its
\ bytes, a read reads, a transaction each, 64 bytes at most; with a subaddress, the file's offset is the device's
\ register: written first, then the data, a repeated start before a read), and ctl (speed N in kHz, subaddress 0, 1
\ or 2).  The directory lists the devices that answer.  A failure THROWs (a device that doesn't answer: E_IO).  Only
\ the words forth starts with (File Access's).

create i2c-buf 32 allot   variable i2c-n         \ A name or a line being made
create i2c-rec 64 allot                           \ A stat record (the directory's)

: i2c{ ( -- ) 0 i2c-n ! ;
: i2c+ ( c-addr u -- ) tuck i2c-buf i2c-n @ + swap move i2c-n +! ;
: i2c+c ( char -- ) i2c-buf i2c-n @ + c! 1 i2c-n +! ;
: i2c+n ( n -- ) base @ >r decimal 0 <# #s #> i2c+ r> base ! ;
: i2c+x ( addr -- ) dup 4 rshift 15 and s" 0123456789abcdef" drop + c@ i2c+c   \ Two hex digits (the driver's)
  15 and s" 0123456789abcdef" drop + c@ i2c+c ;
: i2c} ( -- c-addr u ) i2c-buf i2c-n @ ;
: i2c-ctl ( c-addr u -- ) s" /dev/i2c/ctl" w/o open-file throw >r r@ write-file r> close-file throw throw ;
: i2c-open ( addr reg fam -- fid )                \ Device addr's file, at its register reg
  rot i2c{ s" /dev/i2c/" i2c+ i2c+x i2c} rot bin open-file throw >r 0 r@ reposition-file throw r> ;
: i2c-name ( -- c-addr u )                        \ The stat record's name
  i2c-rec 0 begin dup 32 < while 2dup + c@ while 1+ repeat then ;

: i2c-read ( addr reg c-addr u -- )              \ u bytes (64 at most) from device addr, from its register reg
  2swap r/o i2c-open >r r@ read-file throw drop r> close-file throw ;
: i2c-write ( addr reg c-addr u -- )             \ u bytes to device addr, to its register reg
  2swap w/o i2c-open >r r@ write-file throw r> close-file throw ;
: i2c-speed ( khz -- ) >r i2c{ s" speed " i2c+ r> i2c+n i2c} i2c-ctl ;          \ 1-100
: i2c-reg-size ( n -- ) >r i2c{ s" subaddress " i2c+ r> i2c+n i2c} i2c-ctl ;    \ The register's bytes: 0, 1, 2
: i2c-devices ( -- )                              \ The addresses that answer, in hex
  s" /dev/i2c" r/o open-file throw >r
  begin i2c-rec 64 r@ read-file throw 64 = while
    i2c-name dup 2 = if type space else 2drop then
  repeat r> close-file throw ;
: i2c? ( addr -- flag )                           \ A device at addr answers?
  i2c{ i2c+x  s" /dev/i2c" r/o open-file throw >r 0
  begin i2c-rec 64 r@ read-file throw 64 = while
    i2c-name 2 = if dup c@ i2c-buf c@ = swap 1+ c@ i2c-buf 1+ c@ = and or else drop then
  repeat r> close-file throw ;
