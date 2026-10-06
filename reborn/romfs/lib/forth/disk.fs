\ disk.fs - the disks, by their files (#d, /dev/sd): lib disk.  hylang's disk library's words, a disk named by its
\ letter (0-f a card on that SPI device, x the ROM disk, r the RAM disk, s the shared one: [char] r).  A disk's ctl
\ reads as it ("sdhc 7580 MB 15523840 blocks", or "none"; its file system's lines after), and takes init (a card
\ started again), stop (a RAM disk's banks given back), format, label, check.  What's free on each is df, the
\ program.  A failure THROWs.  Only the words forth starts with.

create disk-buf 32 allot   variable disk-n
create disk-out 256 allot

: disk-file ( disk -- c-addr u )                  \ /dev/sd/disk/ctl
  0 disk-n ! s" /dev/sd/" tuck disk-buf swap move disk-n !
  disk-buf disk-n @ + c! 1 disk-n +!  s" /ctl" tuck disk-buf disk-n @ + swap move disk-n +!  disk-buf disk-n @ ;
: disk-cmd ( c-addr u disk -- )
  disk-file w/o open-file throw >r r@ write-file r> close-file throw throw ;

: disk-ctl ( disk -- c-addr u )                   \ Its ctl's text
  disk-file r/o open-file throw >r disk-out 256 r@ read-file r> close-file throw throw disk-out swap ;
: disk-start ( disk -- ) s" init" rot disk-cmd ; \ A card started again (as after it's changed)
: disk-stop ( disk -- ) s" stop" rot disk-cmd ;  \ A RAM disk stopped
: cards ( -- mask )                               \ The SD cards there: bit n, one on SPI device n
  0 16 0 do
    i s" 0123456789abcdef" drop + c@ disk-file r/o open-file 0= if
      >r disk-out 4 r@ read-file r> close-file drop 0= swap 3 > and
      disk-out c@ [char] n <> and if 1 i lshift or then
    else drop then
  loop ;
