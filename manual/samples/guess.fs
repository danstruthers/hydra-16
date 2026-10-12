\ guess.fs - guess the number, in HyForth
lib random

variable secret
variable tries

: ask ( -- n )                  \ A number, typed at the keyboard
  ." Your guess? " pad 8 accept
  0 0 rot pad swap >number 2drop drop ;

: judge ( n -- flag )           \ Say how a guess did; true if it's right
  dup secret @ < if drop ." Too small." cr false exit then
      secret @ > if      ." Too big." cr false exit then
  true ;

: play ( -- )
  100 random 1+ secret !  0 tries !
  ." I'm thinking of a number from 1 to 100." cr
  begin  ask  1 tries +!  judge  until
  ." Right! You took " tries @ . ." guesses." cr ;

play
