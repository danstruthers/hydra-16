\ bench.fs - HyForth's side of the hylang/HyForth benchmarks (bench.hl is hylang's: the same algorithms, the same
\ sizes, the same results).  forth bench.fs [reps [quick]]: each benchmark run reps times (1), and a line for each,
\   bench forth NAME RESULT TICKS REPS
\ its value, and the ticks the reps took (200 a second).  quick: smaller sizes (the regression test's).  Each loop is
\ the language's own way: DO LOOP or BEGIN WHILE REPEAT here, a tail call in hylang.  Every value stays under 16,384,
\ so a cell never overflows and hylang's integers are its fixnums.  sim/bench.js runs both and compares them.

require hydra.fl                                  \ sys-ticks, argc, arg

decimal
: arg>n ( n -- u ) >r 0. r> arg >number 2drop drop ;
variable reps   argc 1 > [if] 1 arg>n [else] 1 [then] reps !
variable quick  argc 2 > quick !
: size ( full quick -- n ) quick @ if nip else drop then ;

\ A benchmark: xt's value, run reps times, and the ticks they took, a line
variable t0   variable res
: bench ( xt c-addr u -- )
  2>r sys-ticks t0 !
  reps @ 0 ?do dup execute res ! loop drop
  sys-ticks t0 @ - 32767 and
  ." bench forth " 2r> type space res @ . . reps @ . cr ;

\ loop: n steps of a counting loop
: b-loop ( n -- n ) 0 swap 0 ?do 1+ loop ;

\ calls: n calls of a word of two arguments
: add2 ( a b -- a+b ) + ;
: b-calls ( n -- n ) 0 swap 0 ?do 1 add2 loop ;

\ fib: Fibonacci, recursively
: b-fib ( n -- f ) dup 2 < if exit then dup 1- recurse swap 2 - recurse + ;

\ sieve: the primes below n, a byte each, a prime's multiples marked from its double
create flags 1024 allot   variable sn
: b-sieve ( n -- count )
  dup sn !  flags swap 1 fill
  0  sn @ 2 ?do
    flags i + c@ if
      1+  i i + begin dup sn @ < while 0 over flags + c! i + repeat drop
    then
  loop ;

\ sort: n bytes (x' = 13x + 7, mod 256, from 1) sorted by insertion; the first, the middle and the last added
create sbuf 256 allot
: b-sort-fill ( n -- ) 1 swap 0 ?do dup sbuf i + c! 13 * 7 + 255 and loop drop ;
: b-sort-ins ( v j -- )
  begin dup 0< 0= if 2dup sbuf + c@ < else false then while
    dup sbuf + c@ over sbuf + 1+ c! 1-
  repeat sbuf + 1+ c! ;
: b-sort ( n -- r )
  dup b-sort-fill
  dup 1 ?do i sbuf + c@ i 1- b-sort-ins loop
  sbuf c@ over 1- sbuf + c@ + swap 2/ sbuf + c@ + ;

\ gcd: the sum of gcd(i, j) for i and j 1 to m, each by subtraction
: b-gcd ( a b -- g ) begin 2dup <> while 2dup > if swap over - swap else over - then repeat drop ;
: b-gcds ( m -- sum ) dup sn !  0 swap 1+ 1 ?do sn @ 1+ 1 ?do j i b-gcd + loop loop ;

: run-loop ( -- n ) 4000 1000 size b-loop ;
: run-calls ( -- n ) 2000 500 size b-calls ;
: run-fib ( -- n ) 16 12 size b-fib ;
: run-sieve ( -- n ) 1024 512 size b-sieve ;
: run-sort ( -- n ) 100 40 size b-sort ;
: run-gcd ( -- n ) 20 10 size b-gcds ;

' run-loop s" loop" bench
' run-calls s" calls" bench
' run-fib s" fib" bench
' run-sieve s" sieve" bench
' run-sort s" sort" bench
' run-gcd s" gcd" bench
.( bench forth done) cr
