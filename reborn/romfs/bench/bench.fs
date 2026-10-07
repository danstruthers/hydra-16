\ bench.fs - HyForth's side of the hylang/HyForth benchmarks (bench.hl is hylang's: the same algorithms, the same
\ sizes, the same results).  forth bench.fs [reps [q|f [name...]]]: each benchmark run reps times (1), and a line for
\ each,
\   bench forth NAME RESULT TICKS REPS
\ its value, and the ticks the reps took (200 a second).  q: smaller sizes (the regression test's); names: those
\ alone.  Each is written the language's own way: DO LOOP or BEGIN WHILE REPEAT here, a tail call, while, dotimes
\ or each in hylang; an array and EXECUTE here where hylang has lists, map, filter and foldl.  Every value stays
\ under 16,384, so a cell never overflows and hylang's integers are its fixnums.  sim/bench.js runs both and
\ compares them.

require hydra.fl                                  \ sys-ticks, argc, arg

decimal
: arg>n ( n -- u ) >r 0. r> arg >number 2drop drop ;
: s= ( a1 u1 a2 u2 -- f )                         \ the same characters
  rot over <> if 2drop drop false exit then
  0 ?do over i + c@ over i + c@ <> if 2drop unloop false exit then loop 2drop true ;
variable reps   argc 1 > [if] 1 arg>n [else] 1 [then] reps !
: q? ( -- f ) argc 2 > if 2 arg s" q" s= else false then ;
variable quick  q? quick !
: size ( full quick -- n ) quick @ if nip else drop then ;
: chosen? ( c-addr u -- f )                       \ named after q|f, or none named
  argc 4 < if 2drop true exit then
  argc 3 ?do 2dup i arg s= if 2drop unloop true exit then loop 2drop false ;

\ A benchmark: xt's value, run reps times, and the ticks they took, a line (if it's chosen)
variable t0   variable res
: bench ( xt c-addr u -- )
  2dup chosen? 0= if 2drop drop exit then
  2>r sys-ticks t0 !
  reps @ 0 ?do dup execute res ! loop drop
  sys-ticks t0 @ - 32767 and
  ." bench forth " 2r> type space res @ . . reps @ . cr ;

\ ---- Calls

: add2 ( a b -- a+b ) + ;
: b-calls ( n -- n ) 0 swap 0 ?do 1 add2 loop ;

: b-fib ( n -- f ) dup 2 < if exit then dup 1- recurse swap 2 - recurse + ;

\ tak: Takeuchi's function, three arguments, each call's three calls' values its arguments: tak(9, 6, 3), 293 calls
\ (as deep as the data stack, 32 cells, takes it), k times, summed
: b-tak ( x y z -- r )
  over 3 pick < 0= if nip nip exit then
  2 pick 1- 2 pick 2 pick recurse >r
  over 1- over 4 pick recurse >r
  dup 1- 3 pick 3 pick recurse
  nip nip nip r> r> swap rot recurse ;
: b-taks ( k -- s ) 0 swap 0 ?do 9 6 3 b-tak + loop ;

\ ack: Ackermann's function, its calls nested deep: ack(2, 9), 230 calls 22 deep, k times, summed
: b-ack ( m n -- r )
  over 0= if nip 1+ exit then
  dup 0= if drop 1- 1 recurse exit then
  over swap 1- recurse swap 1- swap recurse ;
: b-acks ( k -- s ) 0 swap 0 ?do 2 9 b-ack + loop ;

\ ---- Loops

: b-loop ( n -- n ) 0 swap 0 ?do 1+ loop ;

\ while: the sum of i & 3 for i below n, by BEGIN WHILE REPEAT over the stack
: b-while ( n -- s ) 0 0 begin dup 3 pick < while dup 3 and rot + swap 1+ repeat drop nip ;

\ dotimes: the same sum, by DO LOOP
: b-dotimes ( n -- s ) 0 swap 0 ?do i 3 and + loop ;

\ nested: the pairs i, j below n whose i xor j is even, by a DO LOOP in a DO LOOP
variable sn
: b-nested ( n -- c ) dup sn ! 0 swap 0 ?do sn @ 0 ?do i j xor 1 and 0= if 1+ then loop loop ;

\ ---- Arithmetic

: b-gcd ( a b -- g ) begin 2dup <> while 2dup > if swap over - swap else over - then repeat drop ;
: b-gcds ( m -- sum ) dup sn !  0 swap 1+ 1 ?do sn @ 1+ 1 ?do j i b-gcd + loop loop ;

\ collatz: the steps to 1 of each n from 1 to m (halved if even, else 3n + 1), summed
: b-cz ( x t -- t )
  begin over 1 > while over 1 and if over 3 * 1+ else over 2/ then rot drop swap 1+ repeat nip ;
: b-collatz ( m -- t ) 0 swap 1+ 1 ?do i swap b-cz loop ;

\ hash: h = ((h & 255) * 31 + i) & 4095 for i below n: a multiplication a step
: b-hash ( n -- h ) 0 swap 0 ?do 255 and 31 * i + 4095 and loop ;

\ ---- Bytes

\ sieve: the primes below n, a byte each, a prime's multiples marked from its double
create flags 1024 allot
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

\ matrix: two n by n matrices of bytes (a: (i + j) & 3, b: i * j & 3) multiplied, the product's elements summed
create ma 144 allot   create mb 144 allot   variable mi   variable mj
: b-matrix ( n -- s )
  sn !
  sn @ 0 ?do sn @ 0 ?do
    j i + 3 and  j sn @ * i + ma + c!
    j i * 3 and  j sn @ * i + mb + c!
  loop loop
  0  sn @ 0 ?do i mi !  sn @ 0 ?do i mj !
    0  sn @ 0 ?do  mi @ sn @ * i + ma + c@  i sn @ * mj @ + mb + c@  * +  loop
    +
  loop loop ;

\ queens: the ways n queens can stand on an n by n board, none taking another, counted by backtracking (a byte each
\ for the columns and the two ways of diagonals)
create qcol 16 allot   create qd1 32 allot   create qd2 32 allot
: b-q ( r -- count )
  dup sn @ = if drop 1 exit then
  0 swap
  sn @ 0 ?do
    i qcol + c@ 0=  over i + qd1 + c@ 0= and  over i - sn @ + qd2 + c@ 0= and
    if
      1 i qcol + c!  1 over i + qd1 + c!  1 over i - sn @ + qd2 + c!
      dup 1+ recurse rot + swap
      0 i qcol + c!  0 over i + qd1 + c!  0 over i - sn @ + qd2 + c!
    then
  loop drop ;
: b-queens ( n -- count ) sn !  qcol 16 0 fill  qd1 32 0 fill  qd2 32 0 fill  0 b-q ;

\ ---- Arrays and EXECUTE (hylang: lists, map, filter, foldl, each)

\ mapf: the sum of the squares of the evens below n, a word for each EXECUTEd, k times
: b-sq ( x -- x*x ) dup * ;
: b-even? ( x -- f ) 1 and 0= ;
: b-mapf ( n k -- r )
  0 swap 0 ?do
    drop 0  over 0 ?do i ['] b-even? execute if i ['] b-sq execute + then loop
  loop nip ;

\ fold: a = (3a + x) & 1023 over 0 to n - 1, a word EXECUTEd for each, k times
: b-f3 ( a x -- a' ) swap 3 * + 1023 and ;
: b-fold ( n k -- r ) 0 swap 0 ?do drop 0  over 0 ?do i ['] b-f3 execute loop loop nip ;

\ each: the sum of x & 7 over an array of 0 to n - 1, k times
create elist 256 cells allot
: b-each ( n k -- r )
  over 0 ?do i elist i cells + ! loop
  0 swap 0 ?do drop 0  over 0 ?do elist i cells + @ 7 and + loop loop nip ;

\ ---- Text

\ chars: the a's in a string of 64 characters, k times
: b-text ( -- c-addr u ) s" the quick brown fox jumps over a lazy dog and a cat at the gate." ;
: b-chars ( k -- c )
  0 swap 0 ?do drop 0 b-text 0 ?do dup i + c@ 97 = if swap 1+ swap then loop drop loop ;

\ digits: the numbers below n written out (<# #S #>), their lengths summed
: b-digits ( n -- s ) 0 swap 0 ?do i 0 <# #s #> nip + loop ;

: run-calls ( -- n ) 2000 500 size b-calls ;
: run-fib ( -- n ) 16 12 size b-fib ;
: run-tak ( -- n ) 6 2 size b-taks ;
: run-ack ( -- n ) 8 2 size b-acks ;
: run-loop ( -- n ) 4000 1000 size b-loop ;
: run-while ( -- n ) 4000 1000 size b-while ;
: run-dotimes ( -- n ) 4000 1000 size b-dotimes ;
: run-nested ( -- n ) 60 30 size b-nested ;
: run-gcd ( -- n ) 20 10 size b-gcds ;
: run-collatz ( -- n ) 60 30 size b-collatz ;
: run-hash ( -- n ) 2000 500 size b-hash ;
: run-sieve ( -- n ) 1024 512 size b-sieve ;
: run-sort ( -- n ) 100 40 size b-sort ;
: run-matrix ( -- n ) 10 6 size b-matrix ;
: run-queens ( -- n ) 7 6 size b-queens ;
: run-mapf ( -- n ) 40 20 5 size b-mapf ;
: run-fold ( -- n ) 200 10 3 size b-fold ;
: run-each ( -- n ) 200 10 3 size b-each ;
: run-chars ( -- n ) 40 10 size b-chars ;
: run-digits ( -- n ) 1000 300 size b-digits ;

' run-calls s" calls" bench
' run-fib s" fib" bench
' run-tak s" tak" bench
' run-ack s" ack" bench
' run-loop s" loop" bench
' run-while s" while" bench
' run-dotimes s" dotimes" bench
' run-nested s" nested" bench
' run-gcd s" gcd" bench
' run-collatz s" collatz" bench
' run-hash s" hash" bench
' run-sieve s" sieve" bench
' run-sort s" sort" bench
' run-matrix s" matrix" bench
' run-queens s" queens" bench
' run-mapf s" mapf" bench
' run-fold s" fold" bench
' run-each s" each" bench
' run-chars s" chars" bench
' run-digits s" digits" bench
.( bench forth done) cr
