1 REM bench.bas - BASIC's side of the benchmarks (bench.hl is hylang's, bench.fs HyForth's: the same algorithms,
2 REM the same sizes, the same results).  basic bench.bas [reps [q|f]]: each benchmark run reps times (1), and a
3 REM line for each,  bench basic NAME RESULT TICKS REPS:  its value, and the ticks the reps took (200 a second).
4 REM q: smaller sizes (the regression test's).  Each is BASIC's own way: FOR and NEXT, GOSUB for a call (its
5 REM arguments and its result in variables), a stack of its own for Fibonacci's recursion (GOSUB keeps no
6 REM locals), integer arrays for bytes.  The benchmarks come first, as a GOTO or GOSUB back looks for its line
7 REM from the program's start; their variables are made first (line 9), as a variable is looked for among them in
8 REM the order they were made.  sim/bench.js runs the three languages' and compares them.
9 R=0:I=0:J=0:N=0:A=0:B=0:C=0:P=0:X=0:V=0:GOTO 900
100 REM loop: n steps of a counting loop
110 R=0:FOR I=1 TO N:R=R+1:NEXT:RETURN
200 REM calls: n calls of a subroutine of two arguments (A and B, its result R)
210 C=0:FOR I=1 TO N:A=C:B=1:GOSUB 230:C=R:NEXT:R=C:RETURN
230 R=A+B:RETURN
300 REM fib: Fibonacci, recursively: N's on a stack of its own (K, P its top), the result R
310 IF N<2 THEN R=N:RETURN
320 P=P+1:K(P)=N:N=N-1:GOSUB 310
330 N=K(P)-2:K(P)=R:GOSUB 310
340 R=R+K(P):P=P-1:RETURN
400 REM sieve: the primes below n, a flag each, a prime's multiples marked from its double
410 FOR I=0 TO N-1:F%(I)=1:NEXT:R=0
420 FOR I=2 TO N-1:IF F%(I)=0 THEN 440
430 R=R+1:IF I+I<N THEN FOR J=I+I TO N-1 STEP I:F%(J)=0:NEXT J
440 NEXT I:RETURN
500 REM sort: n bytes (x' = 13x + 7, mod 256, from 1) sorted by insertion; the first, the middle and the last added
510 X=1:FOR I=0 TO N-1:B%(I)=X:X=(X*13+7) AND 255:NEXT
520 FOR I=1 TO N-1:V=B%(I):J=I-1
530 IF J>=0 THEN IF B%(J)>V THEN B%(J+1)=B%(J):J=J-1:GOTO 530
540 B%(J+1)=V:NEXT
550 R=B%(0)+B%(N-1)+B%(INT(N/2)):RETURN
600 REM gcd: the sum of gcd(i, j) for i and j 1 to m, each by subtraction
610 R=0:FOR I=1 TO N:FOR J=1 TO N:A=I:B=J
620 IF A>B THEN A=A-B:GOTO 620
630 IF A<B THEN B=B-A:GOTO 620
640 R=R+A:NEXT J,I:RETURN
900 REM The runs: reps and quick from the arguments; each benchmark (its size N, kept in M) run RP times.  Its
901 REM loops are GOTO's, so that the stack is the benchmark's (fib's GOSUBs 15 deep, 7 bytes each)
910 RP=VAL(ARG$(1)):IF RP<1 THEN RP=1
920 Q=ARG$(2)="q":DIM F%(1023),B%(255),K(20)
930 READ N$,NF,NQ:IF N$="done" THEN PRINT "bench basic done":END
940 N=NF:IF Q THEN N=NQ
950 BN=BN+1:GOSUB 990:T0=T:RR=0
960 M=N:P=0:ON BN GOSUB 110,210,310,410,510,610:N=M:RR=RR+1:IF RR<RP THEN 960
970 GOSUB 990:D=T-T0:IF D<0 THEN D=D+65536
980 D=D-INT(D/32768)*32768:PRINT "bench basic ";N$;STR$(R);STR$(D);STR$(RP):GOTO 930
990 SYS "TICKS":RREG L,H:T=H*256+L:RETURN
1000 DATA loop,4000,1000,calls,2000,500,fib,16,12,sieve,1024,512,sort,100,40,gcd,20,10,done,0,0
