\ mandel-ex.fs - after BBC BASIC's "FP + Strings" Mandelbrot benchmark: HyForth's exact numbers (the numbers
\ library's number stack: n/ gives a fraction, truncate makes it whole), the same picture as mandel.fs, which keeps
\ to 16-bit cells.  forth mandel-ex.fs
require hydra.fl                                  \ sys-ticks (200 a second)
require numbers.fl                                \ the number stack: n+ n- n* n/ truncate n< nvariable
decimal

: z$ ( -- c-addr u ) s" .,'~=+:;*%&$OXB#@ " ;
: f ( -- ) ( N: -- 50 )  50 s>n ;
nvariable a  nvariable b  nvariable c  nvariable d  nvariable q  nvariable s
variable k  variable t0

\ The character for one point: x and y its place on the grid
: point ( x y -- char )
  s>n 416 s>n n* 100 s>n n/ truncate d n!
  s>n 229 s>n n* 100 s>n n/ truncate c n!
  c n@ a n!  d n@ b n!  0 k !
  begin
    b n@ f n/ truncate q n!                                    \ q = b / f, whole
    b n@ q n@ f n* n- s n!                                     \ s = b - q * f
    a n@ ndup n* b n@ ndup n* n- f n/ truncate c n@ n+          ( N: t )
    a n@ q n@ n*  a n@ s n@ n* f n/ truncate n+  2 s>n n*  d n@ n+  b n!
    a n!
    a n@ f n/ truncate ndup n*  b n@ f n/ truncate ndup n* n+  5 s>n n<   \ not yet escaped?
  while
    1 k +!  k @ 16 = if bl exit then
  repeat
  z$ drop k @ + c@ ;

\ The time, added up a row at a time (sys-ticks is read in 15 bits, which wrap every 163.84 s)
variable last  variable secs  variable ticks
: clock0 ( -- )  sys-ticks last !  0 secs !  0 ticks ! ;
: clock ( -- )                                    \ the ticks since the last reading added
  sys-ticks dup last @ - 32767 and ticks +!  last !
  begin ticks @ 199 > while  -200 ticks +!  1 secs +!  repeat ;
: secs. ( -- )                                    \ shown as seconds, two places
  secs @ 0 .r  [char] . emit  ticks @ 2/ s>d <# # # #> type ;

: mandel ( -- )
  ." Mandelbrot - HyForth - exact numbers + strings" cr
  ." Start" cr
  clock0
  13 -12 do
    30 -49 do  i j point emit  loop
    cr  clock
  loop
  ." Finished" cr
  ." Time: " secs. ."  secs." cr ;

mandel bye
