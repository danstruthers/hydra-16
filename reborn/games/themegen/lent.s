; ****************************************************************************
; lent.s - (themegen) what themegen lends its bank programs (histgen, goalgen, linegen, namegen: bankprog.h): its
; own that they use (themegen.h's, data.h's) and cc65's runtime's helpers and the C library's functions they use,
; each named here so that it's linked and in themegen's exports (the build's themegen.exp: a bank program's imports
; found there).  The build names what a bank program uses that its caller hasn't: it goes here.
; ****************************************************************************

            .forceimport    _D, _Dt, _W, _among, _beast_of, _begin_line, _climate, _data, _dids, _emit, _end_line
            .forceimport    _endings, _er, _era_now, _fa, _fill, _find_word, _gA, _gB, _gM, _ga, _ga_line, _gb, _gb_line
            .forceimport    _ge, _geu, _gg, _given, _given_each, _gk, _gmission, _goal_line, _gs, _gu, _land, _lives
            .forceimport    _name_of, _nbeasts, _ne, _nf, _noun, _ns, _pick, _purpose_of, _purpose_words, _rand32
            .forceimport    _rname, _rock, _seed, _sl, _st, _tg_count, _tg_find, _tg_get, _tg_key, _tg_lines, _title
            .forceimport    _val, _valn, _water, _work_of, _works, _wused, _beasts, _erp, _stp, _fap, _beastp, _Dp
            .forceimport    _strncat, _strncmp, _ultoa, ___bzero, _strlen, _strcpy, _strcat, _strcmp, _strchr, _strstr
            .forceimport    _strncpy, _memcpy, _memset, _atoi, _write
            .forceimport    bnegax, bnegeax, decax2, decsp7, incax4, incax5, incax7, incax8, inceaxy, laddeq1, ldaxidx
            .forceimport    ldeaxi, leaaxsp, regsave, shleax1, shleax4, shreax1, tosadda0, tosdiva0, tosicmp0
            .forceimport    tosmoda0, tosmulax, tosudiva0, tosumoda0, tosumulax, axlong, tosumuleax, shlax1
            .forceimport    utsteax, incax3, ldeaxidx, pushwidx
