;
;  training data
;
ftrain_0:
.byte ": 2* dup + ;"
.byte 0
.byte ": ?branch bool not rp @ @ @ 2 - and rp @ @ + 2 + rp @ ! ;  : branch rp @ @ dup @ + rp @ ! ;"
.byte 0
.byte ": if lit ?branch , here @ 0 , ; I"
.byte 0
.byte ": then dup here @ swap - swap ! ; I"
.byte 0
.byte ": else lit branch , here @ 0 , swap dup here @ swap - swap ! ; I"
.byte 0
.byte ": begin here @ ; I"
.byte 0
.byte ": again lit branch , here @ - , ; I"
.byte 0
.byte ": until lit ?branch , here @ - , ; I"
.byte 0
.byte ": while lit ?branch , here @ 0 , ; I"
.byte 0
.byte ": repeat swap lit branch , here @ - , dup here @ swap - swap ! ; I"
.byte 0
.byte ": do here @ lit >r , lit >r , ; I"
.byte 0
.byte ": loop lit r> , lit r> , lit lit , 1 , lit + , lit 2dup , lit = , lit ?branch , here @ - , lit 2drop , ; I"
.byte 0
.byte ": i rp @ 4 + @ ;  : j rp @ 8 + @ ;  : k rp @ lit [ 12 , ] + @ ;"
.byte 0
.byte ": mdump memptr 2 + @ 1 dump ;"
.byte 0
.byte ": decsz! xdrv 6 4 malloc decs! ;"
.byte 0
.byte ": | lit [ 256 , ] / ;"
.byte 0, 0                               ; to mark end?
ftrain_end0:
