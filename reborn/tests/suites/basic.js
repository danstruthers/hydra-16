// ****************************************************************************
// basic.js - BASIC's tests.  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { card, CARD_DIR, OLD_CARDS, imageCard, shellCard, typed } = require('../lib/common.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

// BASIC's suite (the bsuite test's card): tests/basic's files, each as itself: the programs that check themselves
// (NAME.bas: its checks counted, a FAIL line for each one wrong, then "NAME: n checks, m failed"; BSUITE_PROGS, each
// one's count), the scripts piped into basic (NAME.txt) and what they print (NAME.out; BSUITE_SCRIPTS).  hydra.bas
// reads rc's $greet (hi) and its arguments (one two); files.bas and errors.bas write their files on the card;
// procs.bas INCLUDEs inc.bas, prompt.txt LOADs lab.bas; asm.bas's ASM blocks .include /lib/as's hydra.inc
const BASIC_DIR = path.join(TESTS, 'basic');
const BSUITE_PROGS = { arith: 66, funcs: 82, logic: 64, strings: 55, arrays: 37, flow: 33, procs: 28, records: 23, data: 27,
  errors: 30, files: 24, hydra: 32, asm: 23 };
const BSUITE_SCRIPTS = ['errors', 'print', 'prompt', 'input', 'asm'];
const bsuiteLine = n => (n === 'hydra' ? 'greet=hi; ' : '') + 'basic ' + n + '.bas' + (n === 'hydra' ? ' one two' : '');
function basicCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'basic0.img');
  fs.rmSync(f, { force: true });
  hydrafs.mkfs(f, 8, 'BASIC', undefined, true);
  const v = new hydrafs.Volume(f);
  for (const n of fs.readdirSync(BASIC_DIR)) v.put(n, fs.readFileSync(path.join(BASIC_DIR, n)));
  v.close();
  return [imageCard(0, f, 16384)];
}

module.exports = {
  tests: [
    {
      name: 'basic', what: 'BASIC (docs/06-languages/hybasic.md) at the console: its prompt, PRINT, exact numbers, keywords in either case, a program typed by its line numbers (LIST, RUN; GOSUB, DATA, READ, INPUT, DIM, a FUNCTION), Ctrl-C (Break in) in a GOTO\'s loop and CONT, INKEY$\'s key (a string of one character), errors (at the prompt; a typed program\'s by its line\'s number), BYE (code 0); a pipeline into it (no prompt; an error\'s line ended, its end at stdin\'s); in /ram: SAVE, LOAD, RUN "name", a file not there (by its name), the text cat; scripts (basic file, #!/bin/basic: status 0 and 1, a file not there); files: OPEN\'s modes, PRINT #, INPUT #, LINE INPUT #, EOF, APPEND, CLOSE, the cat; a file not open, open, not there; INPUT\'s Redo from start; sound: SOUND\'s notes (a patch, a volume, off), SLEEP between them (timed), BEEP (the bell), a line for /dev/sndctl (the volume kept; the driver\'s error), a channel past 23, a patch past 127; SYS: calls by name (GETPID, TICKS, BANKS_ALLOC; one not there) and RREG; memory: FRE, an array of 10001, 301 strings; the shell (basic -l at rc\'s prompt): BASIC\'s lines and rc\'s by the rule, cd and the prompt, $status, %, a usage, ENV$, a program line, exit',
      init: 't_rc', cycles: 900e6,
      machine: {
        input: '\u0101basic\r' + '\u0101PRINT "Hello, World"; 2 + 3 * 4; 10 / 4; 2 ^ 70\r' +
          '\u0101print not 0; 5 and 3; 5 or 2; left$("abcd", 2); chr$(65); sqr(-4)\r' +
          '\u010110 FOR i = 1 TO 3: GOSUB 100: NEXT: PRINT "done"\r' + '\u010120 END\r' +
          '\u0101100 PRINT i; i * i;: RETURN\r' + '\u0101LIST\r' + '\u0101RUN\r' + '\u0101NEW\r' +
          '\u010110 DATA 3, "two": READ a, b$: PRINT a; b$\r' + '\u010120 INPUT "Name"; n$: PRINT "Hi "; n$\r' +
          '\u010130 DIM x(9): x(9) = 7: PRINT twice(x(9))\r' +
          '\u010140 FUNCTION twice (z): twice = z * 2: END FUNCTION\r' + '\u0101RUN\r' + '\u0100Ann\r' +
          '\u0101NEW\r' + '\u010110 i = i + 1: GOTO 10\r' + '\u0101RUN\r\u0100\u0003' + '\u0101CONT\r\u0100\u0003' +
          '\u0101PRINT i > 100\r' + '\u0101NEW\r' + '\u010110 DO: k$ = INKEY$: LOOP UNTIL k$ <> ""\r' +
          '\u010120 PRINT "key "; k$; ASC(k$); LEN(k$); k$ = "k"\r' + '\u0101RUN\r\u0100k' + '\u0101PRINT 1 / 0\r' + '\u0101x\r' +
          '\u0101NEW\r' + '\u010110 PRINT "a"\r' + '\u010120 PRINT 1 / 0\r' + '\u0101RUN\r' + '\u0101BYE\r' +
          '\u0101echo $status\r' +
          '\u0101{echo \'10 FOR i=1 TO 3\'; echo \'20 ? i*10\'; echo \'30 NEXT\'; echo RUN; echo \'? 1/0\'; echo \'? "end"\'} | basic; echo st $status\r' +
          '\u0101cd /ram\r' + '\u0101basic\r' + '\u010110 FOR i = 1 TO 3: PRINT "line"; i: NEXT\r' +
          '\u010120 PRINT "Done": END\r' + '\u0101SAVE "p.bas"\r' + '\u0101NEW\r' + '\u0101LOAD "p.bas"\r' +
          '\u0101LIST\r' + '\u0101NEW\r' + '\u0101RUN "p.bas"\r' + '\u0101LOAD "nofile"\r' + '\u0101BYE\r' +
          '\u0101cat p.bas\r' +
          '\u0101echo \'#!/bin/basic\' >s; echo \'PRINT "script"; 6 * 7\' >>s; echo \'x = 1 / 0\' >>s\r' +
          '\u0101basic p.bas; echo status $status\r' + '\u0101./s; echo status $status\r' +
          '\u0101basic none.bas; echo status $status\r' + '\u0101basic\r' +
          '\u010110 OPEN "d.txt" FOR OUTPUT AS #1: FOR i = 1 TO 3: PRINT #1, i; ","; i * i: NEXT: PRINT #1, "end": CLOSE #1\r' +
          '\u010120 OPEN "d.txt" FOR INPUT AS #2: FOR i = 1 TO 3: INPUT #2, a, b: PRINT a; b: NEXT\r' +
          '\u010130 LINE INPUT #2, s$: PRINT s$; EOF(2): CLOSE #2\r' +
          '\u010140 OPEN "d.txt" FOR APPEND AS #3: PRINT #3, "more": CLOSE #3\r' + '\u0101RUN\r' +
          '\u0101PRINT #2, 5\r' + '\u0101OPEN "x" FOR OUTPUT AS #1: OPEN "y" FOR OUTPUT AS #1\r' + '\u0101CLOSE\r' +
          '\u0101OPEN "nope" FOR INPUT AS #4\r' + '\u0101NEW\r' + '\u010110 INPUT x: PRINT x * 2\r' + '\u0101RUN\r' +
          '\u0100abc\r' + '\u01005\r' + '\u0101BYE\r' + '\u0101cat d.txt\r' + '\u0101basic\r' +
          '\u0101SOUND "volume 150"\r' + '\u010110 SOUND 2, 60, 0, 100: SLEEP 0.5: SOUND 2, 64: SLEEP 0.1: SOUND 2\r' +
          '\u010120 BEEP: SOUND 1, 67\r' + '\u0101RUN\r' + '\u0101SOUND 24, 60\r' + '\u0101SOUND 0, 60, 163\r' +
          '\u0101SOUND "frob"\r' + '\u0101BYE\r' + '\u0101cat /dev/sndctl\r' + '\u0101basic\r' +
          '\u0101SYS "getpid": RREG a: PRINT a > 0\r' + '\u0101SYS "Ticks": RREG l, h: PRINT h * 256 + l > 0\r' +
          '\u0101SYS "nosuch"\r' + '\u0101SYS "banks_alloc", 1: RREG b, , , p: PRINT p AND 1\r' +
          '\u0101PRINT FRE() > 10000\r' + '\u0101DIM x(10000): x(10000) = 7: PRINT x(10000)\r' +
          '\u0101DIM s$(300): FOR i = 0 TO 300: s$(i) = STR$(i) + "abcdefghijklmnopqrstuvwxyz": NEXT: PRINT s$(300)\r' +
          '\u0101BYE\r' + '\u0101basic -l\r' + '\u0101print 1 + 1\r' + '\u0101echo hello from rc\r' + '\u0101x = 5\r' +
          '\u0101? x * 2\r' + '\u0101ls /rom/lib/basic\r' + '\u0101cd /rom\r' + '\u0101echo $status\r' +
          '\u0101cd /none\r' + '\u0101echo s=$status\r' + '\u0101%echo forced\r' + '\u0101bind\r' +
          '\u0101? ENV$("window") = "0"\r' + '\u010110 print "prog"\r' + '\u0101run\r' + '\u0101exit\r' +
          '\u0101echo $status\r',
      },
      expect: ['% basic\nHyBASIC (QuickBASIC\'s kind), BYE to end\n> PRINT "Hello, World"; 2 + 3 * 4; 10 / 4; 2 ^ 70\nHello, World 14  5/2  1180591620717411303424 \n> ',
        'sqr(-4)\n-1  1  7 abA 2i \n> ',
        '> LIST\n10 FOR i = 1 TO 3: GOSUB 100: NEXT: PRINT "done"\n20 END\n100 PRINT i; i * i;: RETURN\n> RUN\n 1  1  2  4  3  9 done\n> ',
        '> RUN\n 3 two\nName? Ann\nHi Ann\n 14 \n> ',
        '> 10 i = i + 1: GOTO 10\n> RUN\nBreak in line 10\n> CONT\nBreak in line 10\n> PRINT i > 100\n-1 \n> ',
        '> RUN\nkey k 107  1 -1 \n> PRINT 1 / 0\ndivision by zero\n> x\nsyntax error\n> ',
        '> RUN\na\nline 20: division by zero\n> BYE\n',
        '% echo $status\n\n%',
        '| basic; echo st $status\n 10 \n 20 \n 30 \ndivision by zero\nend\nst\n%',
        '> LOAD "p.bas"\n> LIST\n10 FOR i = 1 TO 3: PRINT "line"; i: NEXT\n20 PRINT "Done": END\n> NEW\n> RUN "p.bas"\nline 1 \nline 2 \nline 3 \nDone\n> LOAD "nofile"\nnofile: not found\n> BYE\n',
        '% cat p.bas\n10 FOR i = 1 TO 3: PRINT "line"; i: NEXT\n20 PRINT "Done": END\n%',
        '% basic p.bas; echo status $status\nline 1 \nline 2 \nline 3 \nDone\nstatus\n%',
        '% ./s; echo status $status\nscript 42 \n./s:3: division by zero\nstatus 1\n%',
        '% basic none.bas; echo status $status\nnone.bas: not found\nstatus 1\n%',
        '> RUN\n 1  1 \n 2  4 \n 3  9 \nend-1 \n> PRINT #2, 5\nbad file number\n> OPEN "x" FOR OUTPUT AS #1: OPEN "y" FOR OUTPUT AS #1\nfile already open\n> CLOSE\n> OPEN "nope" FOR INPUT AS #4\nfile not found\n> ',
        '> RUN\n? abc\n?Redo from start\n? 5\n 10 \n> BYE\n',
        '% cat d.txt\n 1 , 1 \n 2 , 4 \n 3 , 9 \nend\nmore\n%',
        '> SOUND 24, 60\nillegal function call\n> SOUND 0, 60, 163\nillegal function call\n> SOUND "frob"\ninvalid argument\n> BYE\n',
        '% cat /dev/sndctl\nvolume 150\nchannels 8\nclaimed\n%',
        'RREG a: PRINT a > 0\n-1 \n',
        'PRINT h * 256 + l > 0\n-1 \n',
        '> SYS "nosuch"\nno such call\n',
        'PRINT p AND 1\n 0 \n',
        '> PRINT FRE() > 10000\n-1 \n',
        'PRINT x(10000)\n 7 \n',
        'PRINT s$(300)\n 300abcdefghijklmnopqrstuvwxyz\n> BYE\n',
        '% basic -l\nHyBASIC (QuickBASIC\'s kind), BYE to end\n/ram> print 1 + 1\n 2 \n/ram> echo hello from rc\nhello from rc\n/ram> x = 5\n/ram> ? x * 2\n 10 \n/ram> ls /rom/lib/basic\nprofile.bas\n/ram> cd /rom\n/rom> ',
        '/rom> echo $status\n0\n/rom> cd /none\n/none: not found\n/rom> echo s=$status\ns=not found\n/rom> %echo forced\nforced\n',
        '/rom> bind\nusage: bind [-a|-b] [-c] new old\n/rom> ? ENV$("window") = "0"\n-1 \n/rom> 10 print "prog"\n/rom> run\nprog\n/rom> exit\n% echo $status\n1\n%'],
      check(m) {
        // (SOUND's notes on the YM2151, and SLEEP .5 between two: 0.5 s at 3.58 MHz; BEEP: the bell, channel 7)
        const f = [], on = ch => m.ym.keyOns.filter(k => k.startsWith('ch ' + ch + ' ')).map(k => +k.match(/at cycle (\d+)/)[1]);
        const two = on(2), keys = m.ym.keyOns.join(', ');
        if (two.length !== 2) f.push('SOUND: ' + two.length + ' key-ons on channel 2, not 2: ' + keys);
        else if (Math.abs((two[1] - two[0]) / 3579545 - 0.5) > 0.02) f.push('SLEEP .5: ' + ((two[1] - two[0]) / 3579545).toFixed(3) + ' s between the notes');
        if (!on(7).length) f.push('BEEP: no bell (no key-on on channel 7): ' + keys);
        if (!on(1).length) f.push('SOUND 1,67: no key-on on channel 1: ' + keys);
        return f;
      },
    },
    {
      name: 'bsuite', what: 'BASIC\'s suite (tests/basic, docs/06-languages/hybasic.md), from a card: programs that check themselves, each its checks and none failed (arithmetic: the operators and their order, exact numbers, complex numbers, literals: E notation, &H, the # forms, .5; suffixes; a base; the number functions: exact when they can be, else DIGITS digits, RND, fractions\' and complex numbers\' parts, GCD, FIB, shifts, VAL, STR$ in a base, MKN$; comparisons, the bitwise operators on big integers, IF\'s forms, SELECT CASE; strings: their functions, MID$ =, fixed lengths, past 255, the garbage collector; arrays: TO, dimensions, used before DIM, REDIM PRESERVE, ERASE, LBOUND, SWAP; FOR\'s exact steps, DO, WHILE, EXIT, GOTO, GOSUB, RETURN label, ON, line numbers, _; SUBs and FUNCTIONs: by reference and by value, recursion, STATIC, SHARED, CONST, a call among a call\'s arguments, one calling another, INCLUDE; TYPE: nested records, arrays of them, copies, a field by reference; DATA, READ, RESTORE, CONST, OPTION BASE, DEFSTR; ON ERROR, RESUME\'s forms, ERR, ERL, ERR$, the system\'s errors; files: OPEN\'s modes, PRINT #, WRITE #, INPUT #, LINE INPUT #, INPUT$, LOF, LOC, SEEK, GET, PUT, FREEFILE, directories; the Hydra\'s: SYS and RREG, a bank of its own, machine code, FRE, SLEEP, TIMER, DATE$, TIME$, ENV$, ARG$, COMMAND$, SHELL, SHELL$, STATUS; inline assembly: ASM blocks, CALL ASM\'s labels and registers, the program\'s variables and CONSTs by name, data, a block in a SUB, a later block\'s label, a macro, .if, cheap and unnamed labels, .include and a system call, the blocks\' own bank and .bss); scripts piped into basic, their output tests/basic\'s: the errors\' messages (at the prompt, a typed program\'s by its line numbers; ASM\'s: the assembler\'s at its line, a label not there, a block not ended, past 8K, ASM at the prompt; CALL ASM at the prompt), PRINT\'s layout (zones, TAB, SPC, POS, numbers\' forms, PRINT USING\'s pictures and {} fields, WRITE, a base), the prompt (lines typed, replaced, taken out; LIST\'s ranges, labels; DELETE, CLEAR, SAVE, LOAD, RUN "f", STOP, CONT, SYSTEM), INPUT\'s answers (Redo from start, quotes, an empty line, LINE INPUT, INPUT ;)',
      init: 't_rc', cycles: 780e6,
      get machine() {
        return { sd: basicCard(), input: 'ācd /sd/0\r' + Object.keys(BSUITE_PROGS).map(n => 'ā' + bsuiteLine(n) + '\r').join('') +
          BSUITE_SCRIPTS.map(n => 'ābasic <' + n + '.txt\r').join('') };
      },
      get expect() {
        return [...Object.entries(BSUITE_PROGS).map(([n, c]) => '% ' + bsuiteLine(n) + '\n' + n + ': ' + c + ' checks, 0 failed\n%'),
          ...BSUITE_SCRIPTS.map(n => '% basic <' + n + '.txt\n' + fs.readFileSync(path.join(BASIC_DIR, n + '.out'), 'latin1') + '% ')];
      },
    },
    {
      name: 'bgfx', what: 'BASIC\'s graphics on a Vera X (tests/basic/gfx.bas, from a card: it checks itself, by POINT): SCREEN 13, PSET, LINE (a line, B a box, BF a filled one), CIRCLE (and F a disc); the same through a WINDOW, which kept none of their kinds (every LINE a filled box, every CIRCLE a disc: the scale\'s slot took the statement\'s scratch), and GPRINT\'s whole text through one (4 characters of it)',
      init: 't_rc', cycles: 400e6,
      get machine() { return { vera: true, sd: basicCard(), input: 'ācd /sd/0\r' + 'ābasic gfx.bas\r' }; },
      expect: ['gfx: 21 checks, 0 failed'],
    },
    {
      name: 'bdivs', what: 'BASIC\'s division and product of integers (tests/basic/divs.bas and divs.dat, from a card: it checks itself): FIX(a / b), INT(a / b), a / b, a * b, a \\ b and a MOD b over 876 pairs (every pair of 26 integers: 0, 1, -1, the bytes\' edges, -2^31, 2^31 and past 32 bits; and 200 random pairs of every kind: integers to 30 digits, fixed decimals, fractions, complex numbers), each result\'s bytes (MKN$) as the numbers library gave them before the interpreter took the integers\' cases (7dfff75a), division by zero\'s error and FIX\'s of a complex number too',
      init: 't_rc', cycles: 1200e6,
      get machine() { return { sd: basicCard(), input: 'ācd /sd/0\r' + 'ābasic divs.bas\r' }; },
      expect: ['divs: 5256 checks, 0 failed'],
    },
    {
      name: 'bplay', what: 'BASIC\'s PLAY: a line of MML on channel 0 and on another (play -m), a score file (play name.mml), its time waited for (the program goes on after); play\'s error (its message as BASIC\'s, in its line; BASIC\'s status 1), a channel past 23 (illegal function call); SOUND\'s text commands (sndctl\'s: a note, a level)',
      init: 't_rc', cycles: 160e6, ymLog: true,
      get machine() {
        return { ymLog: true, input: ['echo patch 0 0 >/dev/sndctl; echo patch 1 0 >/dev/sndctl', 'echo \'@p { gm 0 }\' >/ram/s.mml; echo \'B @p o3 g\' >>/ram/s.mml',
          'echo \'10 play "t240 o4 l16 c d e"\' >/ram/p.bas', 'echo \'20 play 2, "t240 o5 l16 c": print "on"\' >>/ram/p.bas',
          'echo \'30 play "/ram/s.mml": print "after"\' >>/ram/p.bas', 'echo \'40 play "c Z"\' >>/ram/p.bas', 'basic /ram/p.bas; echo status $status',
          'echo \'10 play 24, "c"\' >/ram/q.bas; basic /ram/q.bas', 'echo \'10 sound "note 3 72": sound "level 3 90"\' >/ram/r.bas; basic /ram/r.bas; echo r $status'
        ].map(l => '\u0101' + l + '\r').join('') };
      },
      expect: ['basic /ram/p.bas; echo status $status\non\nafter\nplay: c Z: channel 0: what is Z\n/ram/p.bas:4: channel 0: what is Z\nstatus 1\n%',
        'basic /ram/q.bas\n/ram/q.bas:1: illegal function call\n%', 'echo r $status\nr\n%'],
      // (The key-ons, each with its channel's key code then: C4 D4 E4 on 0 a 16th apart at 240 (12.5 ticks), C5 on
      // 2, the score's G3 on 1 (B), SOUND's C5 on 3)
      check(m) {
        const kc = [], ons = [];
        for (const [, r, v] of m.ym.writes) { if (r >= 0x28 && r < 0x30) kc[r & 7] = v; if (r === 0x08 && (v & 0x78)) ons.push((v & 7) + ':' + (kc[v & 7] || 0).toString(16)); }
        const want = ['0:3e', '0:41', '0:44', '2:4e', '1:38', '3:4e'], got = ons.slice(-want.length);
        return got.join(' ') === want.join(' ') ? [] : ['the key-ons (channel:key code): ' + got.join(' ') + ', not ' + want.join(' ')];
      },
    },
    {
      name: 'bawin', what: 'BASIC as a window\'s shell: a card\'s /lib/shell naming /bin/basic -l, init\'s in window 0 and in a window made (Ctrl-] c: $window, ENV$): the prompt, a BASIC line and an rc line in each',
      init: 'init', cycles: 200e6,
      get machine() {
        return { sd: shellCard('/bin/basic -l', 'shellbas'), input: '\u0101? 1+2\r' + '\u0101echo $window\r' + '\u0101\x1dc' +
          '\u0101echo $window\r' + '\u0101? env$("window")\r' + '\u0101x=2: ? x*21\r' };
      },
      expect: ['/> ? 1+2\n 3 \n/> echo $window\n\n/> ', '/> echo $window\n1\n/> ? env$("window")\n1\n/> x=2: ? x*21\n 42 \n/> '],
    },
  ],
};
