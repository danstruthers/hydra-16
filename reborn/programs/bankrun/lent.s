; ****************************************************************************
; lent.s - what bankrun lends the children it runs (bankprog.h): cc65's runtime's helpers, every one, linked whether
; bankrun uses it or not, so they're in its exports (the build's bankrun.exp) for a child's imports
; ****************************************************************************

            .forceimport    addeq0sp, addeqysp, addysp, addysp1, along, aslax1, aslax2, aslax3, aslax4, aslax7, aslaxy, asleax1, asleax2
            .forceimport    asleax3, asleax4, asrax1, asrax2, asrax3, asrax4, asrax7, asraxy, asreax1, asreax2, asreax3, asreax4, aulong
            .forceimport    axlong, axulong, bcasta, bcastax, bcasteax, bnega, bnegax, bnegeax, booleq, boolge, boolgt, boolle, boollt
            .forceimport    boolne, booluge, boolugt, boolule, boolult, bpushbsp, bpushbysp, callax, callptr4, complax, compleax, decax1
            .forceimport    decax2, decax3, decax4, decax5, decax6, decax7, decax8, decaxy, deceaxy, decsp1, decsp2, decsp3, decsp4, decsp5
            .forceimport    decsp6, decsp7, decsp8, enter, getlop, idiv32by16r16, imul16x16r32, imul8x8r16, imul8x8r16m, incax1, incax2
            .forceimport    incax3, incax4, incax5, incax6, incax7, incax8, incaxy, inceaxy, incsp1, incsp2, incsp3, incsp4, incsp5, incsp6
            .forceimport    incsp7, incsp8, jmpvec, laddeq, laddeq0sp, laddeq1, laddeqa, laddeqysp, ldaidx, ldau00sp, ldau0ysp, ldaui0sp
            .forceimport    ldauidx, ldauiysp, ldax0sp, ldaxi, ldaxidx, ldaxysp, ldeax0sp, ldeaxi, ldeaxidx, ldeaxysp, ldptr10sp
            .forceimport    ldptr1ysp, leaa0sp, leaaxsp, leave, leave0, leave00, leavey, leavey0, leavey00, lsubeq, lsubeq0sp, lsubeq1
            .forceimport    lsubeqa, lsubeqysp, mul8x16, mul8x16a, mulax10, mulax3, mulax5, mulax6, mulax7, mulax9, negax, negeax, popa
            .forceimport    popax, popeax, poplsargs, popptr1, popsargsudiv16, popsreg, push0, push0ax, push1, push2, push3, push4, push5
            .forceimport    push6, push7, pusha, pusha0, pusha0sp, pushaFF, pushax, pushaysp, pushb, pushbidx, pushbsp, pushbysp, pushc0
            .forceimport    pushc1, pushc2, pusheax, pushl0, pushlysp, pushptr1, pushptr1idx, pushw, pushw0sp, pushwidx, pushwysp, regswap
            .forceimport    regswap1, regswap2, resteax, return0, return1, returnFFFF, saveeax, shlax1, shlax2, shlax3, shlax4, shlax7
            .forceimport    shlaxy, shleax1, shleax2, shleax3, shleax4, shrax1, shrax2, shrax3, shrax4, shrax7, shraxy, shreax1, shreax2
            .forceimport    shreax3, shreax4, staspidx, stax0sp, staxspidx, staxysp, steax0sp, steaxspidx, steaxysp, subeq0sp, subeqysp
            .forceimport    subysp, swapstk, tosadd0ax, tosadda0, tosaddax, tosaddeax, tosand0ax, tosanda0, tosandax, tosandeax, tosaslax
            .forceimport    tosasleax, tosasrax, tosasreax, tosdiv0ax, tosdiva0, tosdivax, tosdiveax, toseq00, toseqa0, toseqax, toseqeax
            .forceimport    tosge00, tosgea0, tosgeax, tosgeeax, tosgt00, tosgta0, tosgtax, tosgteax, tosicmp, tosicmp0, tosint, toslcmp
            .forceimport    tosle00, toslea0, tosleax, tosleeax, toslong, toslt00, toslta0, tosltax, toslteax, tosmod0ax, tosmoda0
            .forceimport    tosmodax, tosmodeax, tosmul0ax, tosmula0, tosmulax, tosmuleax, tosne00, tosnea0, tosneax, tosneeax, tosor0ax
            .forceimport    tosora0, tosorax, tosoreax, tosrsub0ax, tosrsuba0, tosrsubax, tosrsubeax, tosshlax, tosshleax, tosshrax
            .forceimport    tosshreax, tossub0ax, tossuba0, tossubax, tossubeax, tosudiv0ax, tosudiva0, tosudivax, tosudiveax, tosuge00
            .forceimport    tosugea0, tosugeax, tosugeeax, tosugt00, tosugta0, tosugtax, tosugteax, tosule00, tosulea0, tosuleax
            .forceimport    tosuleeax, tosulong, tosult00, tosulta0, tosultax, tosulteax, tosumod0ax, tosumoda0, tosumodax, tosumodeax
            .forceimport    tosumul0ax, tosumula0, tosumulax, tosumuleax, tosxor0ax, tosxora0, tosxorax, tosxoreax, tsteax, udiv16
            .forceimport    udiv32, udiv32by16r16, udiv32by16r16m, umul16x16r16, umul16x16r16m, umul16x16r32, umul16x16r32m
            .forceimport    umul8x16r16, umul8x16r16m, umul8x16r24, umul8x16r24m, umul8x8r16, umul8x8r16m, utsteax
