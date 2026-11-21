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

: secs. ( ticks -- )                              \ shown as seconds, two places
  2/ s>d <# # # [char] . hold #s #> type ;

: mandel ( -- )
  ." Mandelbrot - HyForth - integers + strings" cr
  ." Start" cr
  sys-ticks t0 !
  13 -12 do
    30 -49 do  i j point emit  loop
    cr
  loop
  sys-ticks t0 @ - 32767 and
  ." Finished" cr
  ." Time: " secs. ."  secs." cr ;

mandel bye
