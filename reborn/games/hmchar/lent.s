; ****************************************************************************
; lent.s - (hmchar) what hmchar lends its bank program (hmocc, the occupation: bankprog.h): its own that it uses
; (hmchar.h's) and cc65's runtime's helpers and the C library's functions it uses, each named here so that it's
; linked and in hmchar's exports (the build's hmchar.exp: a bank program's imports found there).  The build names
; what a bank program uses that its caller hasn't: it goes here.
; ****************************************************************************

            .forceimport    _h, _quiet, _cname, _occname, _libdir, _ab, _col, _ch, _s, _n, _named, _err, _ask
            .forceimport    _skill_named, _spend
            .forceimport    _atoi, _harn_deity_name, bnegax
