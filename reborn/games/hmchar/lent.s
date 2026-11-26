; ****************************************************************************
; lent.s - (hmchar) what hmchar lends its bank program (hmocc, the occupation: bankprog.h): its own that it uses
; (hmchar.h's) and cc65's runtime's helpers and the C library's functions it uses, each named here so that it's
; linked and in hmchar's exports (the build's hmchar.exp: a bank program's imports found there).  The build names
; what a bank program uses that its caller hasn't: it goes here.
; ****************************************************************************

            .forceimport    _h, _quiet, _cname, _uname, _occname, _libdir, _ab, _col, _ch, _s, _n, _named, _err, _ask
            .forceimport    _skill_named, _spend, _ntongues, _nscripts
            .forceimport    _atoi, _harn_deity_name, _harn_rand_class, bnegax
            .forceimport    _name, _strlen, _harn_species_name, _harn_sex_name, _harn_month_name, _harn_sunsign_name
            .forceimport    _harn_culture_name, _harn_class_name, _harn_parent_name, _harn_estrangement_name
            .forceimport    _harn_clanhead_name, _harn_frame_name, _harn_size, _harn_comeliness_name, _harn_complexion_name
            .forceimport    _harn_hair_name, _harn_eye_name, _harn_attr_name, _harn_end, _harn_move, _harn_medical_name
