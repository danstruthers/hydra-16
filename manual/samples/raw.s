; A routine on its own, at $1000: no header (as -b)
.org $1000
double:     asl a               ; .A times 2 ...
            bcc :+
            inx                 ;   (its carry into .X)
:           rts
