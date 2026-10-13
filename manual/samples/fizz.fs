\ fizz.fs - the game of FizzBuzz: count, but say Fizz for the threes,
\ Buzz for the fives, and FizzBuzz for both

: fizz? ( n -- flag ) 3 mod 0= ;
: buzz? ( n -- flag ) 5 mod 0= ;

: say ( n -- )
  dup fizz? over buzz? and if drop ." FizzBuzz " exit then
  dup fizz? if drop ." Fizz " exit then
  dup buzz? if drop ." Buzz " exit then
  . ;

: fizzbuzz ( n -- ) 1+ 1 do i say loop cr ;
