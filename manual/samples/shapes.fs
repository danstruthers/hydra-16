\ shapes.fs - turtle pictures (it needs a Vera X)
lib video

: polygon ( size sides -- )     \ A shape with that many sides, each that long
  360 over / swap               \ ( size turn sides )
  0 do over fd dup rt loop 2drop ;

: square   ( size -- ) 4 polygon ;
: triangle ( size -- ) 3 polygon ;

: flower ( -- )                 \ 36 squares, each turned a little, each a new colour
  36 0 do  i 15 mod 1+ pen  60 square  10 rt  loop ;

320 8 bitmap  cs
flower
