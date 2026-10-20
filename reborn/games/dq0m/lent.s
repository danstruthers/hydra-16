; ****************************************************************************
; lent.s - (dq0m) what the maker lends its bank program, stratagen (bankprog.h): cc65's runtime's helpers it uses,
; each named here so that it's linked and in the maker's exports (the build's dq0m.exp: a bank program's imports
; found there).  The build names what a bank program uses that its caller hasn't: it goes here.
; ****************************************************************************

            .forceimport    utsteax, incax3, ldeaxidx, pushwidx
