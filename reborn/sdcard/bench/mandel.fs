\ mandel.fs - after BBC BASIC's "FP + Strings" Mandelbrot benchmark: HyForth's 16-bit cells, / and /mod rounding
\ toward zero (as the BASIC's integer division), the same picture.  forth mandel.fs
require hydra.fl                                  \ sys-ticks (200 a second)
decimal

: z$ ( -- c-addr u ) s" .,'~=+:;*%&$OXB#@ " ;
50 constant f
variable a  variable b  variable c  variable d  variable n  variable q  variable s  variable t0

\ The character for one point: x and y its place on the grid
: point ( x y -- char )
  416 * 100 / d !  229 * 100 / c !
  c @ a !  d @ b !  0 n !
  begin
    b @ f /mod q ! s !                            \ q = b / f, s = b mod f
    a @ dup * b @ dup * - f / c @ +               ( t )
    a @ q @ *  a @ s @ * f / +  2*  d @ +  b !
    a !
    a @ f / dup *  b @ f / dup * +  5 <           \ not yet escaped?
  while
    1 n +!  n @ 16 = if bl exit then
  repeat
  z$ drop n @ + c@ ;

\ The time, added up a row at a time (sys-ticks is read in 15 bits, which wrap every 163.84 s)
variable last  variable secs  variable ticks
: clock0 ( -- )  sys-ticks last !  0 secs !  0 ticks ! ;
: clock ( -- )                                    \ the ticks since the last reading added
  sys-ticks dup last @ - 32767 and ticks +!  last !
  begin ticks @ 199 > while  -200 ticks +!  1 secs +!  repeat ;
: secs. ( -- )                                    \ shown as seconds, two places
  secs @ 0 .r  [char] . emit  ticks @ 2/ s>d <# # # #> type ;

: mandel ( -- )
  ." Mandelbrot - HyForth - integers + strings" cr
  ." Start" cr
  clock0
  13 -12 do
    30 -49 do  i j point emit  loop
    cr  clock
  loop
  ." Finished" cr
  ." Time: " secs. ."  secs." cr ;

mandel bye
