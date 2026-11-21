// ****************************************************************************
// forth.js - HyForth's tests.  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { card, CARD_DIR, OLD_CARDS, imageCard, typed, expected, pcReport } = require('../lib/common.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

// The forth test's card: the Forth 2012 test suite's files (tests/forth), each as itself; run.fs, run2.fs and run3.fs,
// which INCLUDE them in the suite's own order (runtests.fth's, those of the word sets HyForth has) in three sessions
// (the dictionary hasn't room for them all), each REQUIRing the libraries (.fl) its word sets are beyond startup.fs's
// (filetest.fth uses String's /STRING and coreexttest.fth's SI_INC; doubletest.fth, core.fr's <TRUE>; localstest.fth
// and toolstest.fth, the Search-Order words; blocktest.fth makes blocks.fb on the card), run.fs
// with a line for core.fr's ACCEPT test after it (stdin's next line); bad.fs, a file with an error in it; and args.fs,
// a script (#!/bin/forth: its arguments, the Hydra library, the constants library, a library of its own)
const FORTH_RUNS = [['prelimtest.fth', 'tester.fr', 'core.fr', 'coreplustest.fth', 'utilities.fth', 'errorreport.fth',
  'coreexttest.fth', 'double.fl', 'doubletest.fth', 'exceptiontest.fth', 'string.fl', 'filetest.fth'],
  ['facility.fl', 'search.fl', 'string.fl', 'memory.fl', 'locals.fl', 'block.fl', 'tester.fr', 'utilities.fth',
  'errorreport.fth', 'blocktest.fth', 'facilitytest.fth', 'localstest.fth', 'memorytest.fth'],
  ['tools.fl', 'search.fl', 'string.fl', 'double.fl', 'tester.fr', 'utilities.fth', 'errorreport.fth', 'toolstest.fth',
  'searchordertest.fth', 'stringtest.fth']];
const FORTH_SUITE = [...new Set(FORTH_RUNS.flat().filter(n => !n.endsWith('.fl')))];
const FORTH_HELPERS = ['required-helper1.fth', 'required-helper2.fth'];
// The suite's error report (errorreport.fth's) for a session: the word sets it tested, each with no errors; the rest -
const FORTH_SETS = ['Core', 'Core extension', 'Block', 'Double number', 'Exception', 'Facility', 'File-access', 'Locals',
  'Memory-allocation', 'Programming-tools', 'Search-order', 'String'];
const forthReport = (...tested) => FORTH_SETS.map(s => s.padEnd(24) + (tested.includes(s) ? '0' : '-') + '\n').join('') +
  '---------------------------\nTotal                   0\n---------------------------\n';
function forthCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'forth0.img');
  hydrafs.mkfs(f, 8, 'FORTH', undefined, true);
  const v = new hydrafs.Volume(f);
  for (const n of [...FORTH_SUITE, ...FORTH_HELPERS]) v.put(n, fs.readFileSync(path.join(TESTS, 'forth', n)));
  FORTH_RUNS.forEach((r, i) => v.put(i ? 'run' + (i + 1) + '.fs' : 'run.fs', Buffer.from(r.map(n => n.endsWith('.fl') ? 'REQUIRE ' + n + '\n' :
    'S" ' + n + '" INCLUDED\n' + (n === 'core.fr' ? 'A line for ACCEPT\n' : '')).join('') + 'REPORT-ERRORS\n')));
  v.put('bad.fs', Buffer.from(': ok1 1 ;\nok1 .\nfoo\n.( not here)\n'));
  v.put('args.fs', Buffer.from('#!/bin/forth\nREQUIRE hydra.fl\nARGC . 0 ARG TYPE SPACE 1 ARG TYPE SPACE 2 ARG TYPE CR\nREQUIRE hydra.fs O_RDWR . CR\n' +
    'LIBRARY MINE  : TWICE 2 * ;  END-LIBRARY  21 TWICE . CR\n'));
  // (blk1.fs changes block 1 and lets it go to its bank, blocks 2 and 3 taking the buffers, with no flush; blk2.fs,
  // another forth, reads it from the file: written as the first ended)
  v.put('blk1.fs', Buffer.from('REQUIRE block.fl\ns" bx.fb" open-blocks\n1 block 1024 char A fill update 2 block drop 3 block drop\n'));
  v.put('blk2.fs', Buffer.from('REQUIRE block.fl\ns" bx.fb" open-blocks\n1 block c@ emit cr\n'));
  // (cbank.fs: colon definitions' code in the task's banks, a stub each (jsr, then its bank), 13K of definitions
  // made by EVALUATE, from a word in the first bank, into the next banks; then words of the first bank: an immediate
  // one compiling, a DOES> child, an S" string, a THROW to a CATCH, ABORT"; SEE; BANK! in one (THROW -21); a definition
  // with code-banks off (its rts); a MARKER giving the banks back, the next definition's code where the one after it was)
  v.put('cbank.fs', Buffer.from('REQUIRE hydra.fl\n: bank-of 3 + c@ ;\n: myif postpone if ; immediate\n: mythen postpone then ; immediate\n' +
    ': def create , does> @ 1+ ;\n: greet s" from the first bank" ;\n: oops abort" oops" ;\n: thrower 1 throw ;\n' +
    '\' greet c@ . \' greet bank-of \' thrower bank-of = . cr\n' +
    ': mk 0 do s" : zz 1 2 3 4 5 6 7 8 9 10 + + + + + + + + + ;" evaluate loop ;\n' +
    '60 mk \' zz bank-of \' greet bank-of <> . zz . cr\n: late 0 myif 1 else 2 mythen ; late .\n5 def x x .\n' +
    ': g2 greet type ; g2 cr\n: catcher [\'] thrower catch . 5 6 + . ; catcher\n: catch2 -1 [\'] oops catch . drop ; catch2 cr\n' +
    'see g2 see greet\n: bk 0 bank! ; \' bk catch .\nfalse code-banks : nb ; \' nb c@ . true code-banks cr\n' +
    'marker m1 : q1 ; \' q1 bank-of \' q1 4 + @\n60 mk m1 : q2 ; \' q2 4 + @ = . \' q2 bank-of = . cr\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The fload test's card: load.fs, its lines made for HyForth's read-ahead (512 bytes of a file at a time, from where
// a line starts): a line from 500 whose CR is the first buffer's last byte (its LF the next one's first), lines
// ended by CR LF and by CR alone, one of 130 characters (cut at 128: the rest, a tab and 6, the next), names between
// tabs, numbers with each prefix and in base 36, a double, and a last line ended by a CR and the file's end
function floadCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'fload0.img');
  hydrafs.mkfs(f, 8, 'FLOAD', undefined, true);
  const v = new hydrafs.Volume(f);
  let s = ': t1 1 ;\n';
  while (s.length < 500 - 40) s += '\\ ' + 'a'.repeat(30) + '\n';
  s += '\\' + ' '.repeat(500 - s.length - 2) + '\n';
  if (s.length !== 500) throw new Error('load.fs: its line at 500 is at ' + s.length);
  s += ': t2 2 ;   \r\n: t3 3 ;\r\n: t4 4 ;\r: t5 5 ;' + ' '.repeat(120) + '\t6\n';
  s += 't1\tt2 + t3 + t4 + t5 + . . %101 . #99 . $ff . \'A\' . 36 base ! z decimal . 65537. . . cr\r';
  v.put('load.fs', Buffer.from(s, 'latin1'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The fnumbers test's card: tester.fr and numberstest.fth (tests/forth), and nums.fs, which REQUIREs string.fl
// (COMPARE), hydra.fl (CODE-BANKS) and numbers.fl, then INCLUDEs them
function fnumCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'fnum0.img');
  hydrafs.mkfs(f, 8, 'FNUM', undefined, true);
  const v = new hydrafs.Volume(f);
  for (const n of ['tester.fr', 'numberstest.fth']) v.put(n, fs.readFileSync(path.join(TESTS, 'forth', n)));
  v.put('nums.fs', Buffer.from('REQUIRE string.fl\nREQUIRE hydra.fl\nREQUIRE numbers.fl\nS" tester.fr" INCLUDED\n' +
    'S" numberstest.fth" INCLUDED\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The fnumquick test's card: fq.fs, fnumxcheck.js's cases (seed 1, 400 random ones) after REQUIRE numbers.fl
const FNUMQ = require('../../sim/tools/fnumxcheck.js').gen(1, 400);
function fnumqCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'fnumq0.img');
  hydrafs.mkfs(f, 8, 'FNUMQ', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('fq.fs', Buffer.from('REQUIRE numbers.fl\n' + FNUMQ.program(true), 'latin1'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The lshell test's long line, sent to another window: 100 characters, more than its keys' queue holds (63)
const SEND_LONG = 'the quick brown fox jumps over the lazy dog, 0123456789, the quick brown fox jumps over the lazy cat';

// The wcache test's lines: names looked up (there or not), then what changes them (a create, a rename, a remove, a
// mkdir and rmdir, a rename of a directory, a create through /lib's union), and looked up again
const WC_LINES = [
  ["cat /ram/x", "cat: /ram/x: not found"],
  ["echo hi >/ram/x; cat /ram/x", "hi"],
  ["mv /ram/x /ram/y; cat /ram/x", "cat: /ram/x: not found"],
  ["cat /ram/y", "hi"],
  ["rm /ram/y; cat /ram/y", "cat: /ram/y: not found"],
  ["mkdir /ram/d /ram/d/e; echo a >/ram/d/e/f; cat /ram/d/e/f", "a"],
  ["rm /ram/d/e/f; rmdir /ram/d/e; cat /ram/d/e/f", "cat: /ram/d/e/f: not found"],
  ["mkdir /ram/d/e; echo b >/ram/d/e/f; cat /ram/d/e/f", "b"],
  ["mv /ram/d /ram/g; cat /ram/g/e/f", "b"],
  ["cat /ram/d/e/f", "cat: /ram/d/e/f: not found"],
  ["cat /lib/nothere", "cat: /lib/nothere: not found"],
  ["echo c >/lib/nothere; cat /lib/nothere /ram/lib/nothere", "c\nc"],
  ["rm /lib/nothere; cat /lib/nothere", "cat: /lib/nothere: not found"],
  ["ls /rom/lib/forth/gpio.fs /lib/forth/gpio.fs", "/rom/lib/forth/gpio.fs\n/lib/forth/gpio.fs"],
];

module.exports = {
  tests: [
    {
      name: 'forth', what: 'HyForth (Forth 2012): the test suite (Core, Core Extension, Double-Number, Exception, Facility, File Access, Locals, Memory-Allocation, Programming-Tools, Search-Order, String, Block) in three sessions, its files INCLUDED from a card, the word sets\' libraries REQUIREd from /lib/forth; scripts (forth file.fs, #!/bin/forth: arguments, REQUIRE from /lib/forth, a library, an error, a pipeline, code banks); at the console: startup.fs\'s Programming-Tools (.S), libraries REQUIREd (and again after a MARKER), a definition, KEY? and KEY, errors (a file\'s, the system\'s), SH, RUN, a sys- word, a bank, the constants library, Ctrl-C, BYE',
      init: 't_rc', cycles: 1200e6,
      // (The console's lines: each a moment after the last, as forth's prompt is its ok; w waits for a key, z, in raw
      // mode, not echoed, and the line after it is cooked again; l loops till Ctrl-C, which rc gets too: its prompt
      // on a new line after forth ends.  hydra.fs is 171 lines compiled, three searches of the dictionary each, so
      // the line after it waits long enough: the window keeps 64 keys typed ahead, and that line is longer)
      get machine() {
        return { sd: forthCard(), input: 'ācd /sd/0; forth <run.fs; forth <run2.fs; forth <run3.fs; echo $status\r' +
          'āforth args.fs a b; echo $status\r' + 'ā./args.fs x; echo $status\r' + 'āforth bad.fs; echo $status\r' +
          'āforth args.fs a b | wc\r' + 'āforth blk1.fs; forth blk2.fs\r' + 'āforth cbank.fs\r' +
          'āforth\rĀ1 2 .s 2drop\rĀrequire facility.fl require hydra.fl\rĀĀĀ: sq dup * ; 7 sq .\rĀ' + 'key? . cr\rĀ' + ': w begin key? until key ; w\rĀzĀ' + 'emit cr 1 2 + .\rĀ' +
          '1 0 /\rĀ' + 'foo\rĀ' + 'include bad.fs\rĀ' + 's" none.fs" included\rĀ' + 's" echo hi" sh .\rĀ' +
          's" echo there" run .\rĀ' + 's" /none" >z pad sys-stat .\rĀ' + '1 sys-banks-alloc throw bank! 1234 bank-window ! bank-window @ .\rĀ' +
          'require hydra.fs O_RDWR . E_NOENT .\rĀĀĀĀĀĀĀĀĀĀĀĀ' + 'marker m require double.fl m require double.fl -5 s>d dabs drop .\rĀĀ' +
          ': l begin again ; l\rĀ\u0003Ā' + '-5 3 mod . bye\r' + 'āecho $status\r' };
      },
      expect: ['0 tests failed out of 57 additional tests', 'End of Core word set tests', 'End of additional Core tests',
        'End of Core Extension word tests', 'End of Double-Number word tests', 'End of Exception word tests', 'End of File-Access word set tests',
        forthReport('Core', 'Core extension', 'Double number', 'Exception', 'File-access'), 'End of Block word tests',
        'End of Facility word tests', 'End of Locals word set tests', 'End of Memory-Allocation word tests',
        forthReport('Core', 'Block', 'Facility', 'Locals', 'Memory-allocation'), 'End of Programming Tools word tests',
        'End of Search Order word tests', 'End of String word tests', forthReport('Core', 'Programming-tools', 'Search-order', 'String'),
        '% forth args.fs a b; echo $status\n3 args.fs a b\n2 \n42 \n\n%', '% ./args.fs x; echo $status\n2 ./args.fs x \n2 \n42 \n\n%',
        '% forth bad.fs; echo $status\n1 bad.fs:3: foo ?\n1\n%', '% forth args.fs a b | wc\n      3       6      21\n%', '% forth blk1.fs; forth blk2.fs\nA\n%',
        '% forth cbank.fs\n32 -1 \n-1 55 \n2 6 from the first bank\n1 11 -2 \n: g2 greet type ;\n: greet s" from the first bank" ;\n-21 96 \n-1 -1 \n%',
        'HyForth (Forth 2012), bye to end\n1 2 .s 2drop\n<2> 1 2  ok\nrequire facility.fl require hydra.fl\n ok\n: sq dup * ; 7 sq .\n49  ok\nkey? . cr\n0 \n ok\n: w begin key? until key ; w\n ok\n' +
        'emit cr 1 2 + .\nz\n3  ok\n1 0 /\ndivision by zero\nfoo\nfoo ?\n' +
        'include bad.fs\n1 bad.fs:3: foo ?\ns" none.fs" included\nnone.fs: not found\ns" echo hi" sh .\nhi\n0  ok\n' +
        's" echo there" run .\nthere\n0  ok\ns" /none" >z pad sys-stat .\n-544  ok\n' +
        '1 sys-banks-alloc throw bank! 1234 bank-window ! bank-window @ .\n1234  ok\n' +
        'require hydra.fs O_RDWR . E_NOENT .\n2 32  ok\nmarker m require double.fl m require double.fl -5 s>d dabs drop .\n5  ok\n' +
        ': l begin again ; l\ninterrupt\n' +
        '-5 3 mod . bye\n-2 \n\n% echo $status\n\n%'],
      check(m, out) {
        const f = [];
        for (const bad of ['INCORRECT RESULT', 'WRONG NUMBER OF RESULTS', 'Error: #'])
          if (out.includes(bad)) f.push('the suite: ' + out.slice(out.indexOf(bad), out.indexOf(bad) + 100).replace(/\n/g, ' | '));
        const undef = /^(.*) \?$/m.exec(out.split('% forth args.fs')[0]);    // (The suite's)
        if (undef) f.push('an undefined word: ' + undef[1]);
        return f;
      },
    },
    {
      name: 'hyforth', what: 'HyForth\'s additions (docs/design/hyforth.md): names in lower case; words (each word\'s xt, and whether it\'s a literal, immediate, assembly or Forth); the libraries loaded (libs), one not searched (-lib) and searched again (lib, where it was), the one with lib refused, a .fs one, one a MARKER takes out; disasm (the modes, the Rockwell opcodes, a jsr to a word), see of a code word (with disasm.fl, and without), sys, the bit words, random\'s numbers; the terminal\'s sequences, form, ekey and the keys (an arrow key, a character); the sound words (notes on the YM2151, a claim, the volume; a channel\'s level, its old name; the registers read back: a note\'s key code and fraction; a song by play, its error; a note by its frequency, a glide, the LFO, a channel\'s sensitivity, the noise; a line of MML and a chord, by play); ctl (and its error); compile-only words typed (THROW -14: >r, if, .", a synonym of one, a library\'s) and compiled',
      init: 't_rc', cycles: 180e6,
      // (At 115200, so words's thousands of characters are out before the next line comes: the keys typed meanwhile
      // wait in the window's queue, which has room for a line or two.  greet.fs, in /ram, the current directory: lib
      // finds it there, as REQUIRED does)
      machine: {
        input: 'āecho b115200 >/dev/serctl; cd /ram; echo \': greet 7 . ;\' >greet.fs\r' + 'āforth\rĀĀ' +
          '5 constant five : twice 2 * ; : x 3 . ; immediate words\rĀĀĀ' + 'libs\rĀ' + '-lib tools\rĀ' +
          'lib string libs\rĀĀ' + '-lib string libs\rĀ' + 's" abc" s" abd" compare .\rĀ' + 'lib string s" abc" s" abd" compare .\rĀ' +
          'marker m lib double m libs\rĀĀ' + 'lib greet greet libs\rĀĀ' + '-lib greet greet\rĀ' +
          'see 2drop\rĀ' + 'lib disasm see 2drop\rĀĀ' +
          'create c $0F c, $12 c, $FD c, $B2 c, $22 c, $7C c, $34 c, $12 c, $B1 c, $10 c, $A1 c, $10 c,\rĀ' +
          '$BE c, 0 c, $80 c, $B6 c, $10 c, $87 c, $20 c, $0A c, $CB c, $20 c, \' dup , c 11 disasm\rĀĀ' +
          'lib hydra create s $A9 c, 7 c, $A2 c, 9 c, $A0 c, $0B c, $38 c, $60 c, s 0 0 0 sys .s\rĀĀ' +
          'lib bits 5 3 tbit . . 0 15 sbit . $FFFF 0 cbit .\rĀ' + 'lib random 12345. rseed rand . rand . 6 random . 1000 random .\rĀĀ' +
          'lib facility clear-line clear-below 3 cursor-up 0 cursor-down 2 cursor-right 1 cursor-left cursor-save\rĀĀ' +
          'cursor-restore cursor-off cursor-on red color blue bright bgcolor bold dim underline blink reverse plain\rĀ' +
          '38 sgr beep form . . 3 7 at-xy page\rĀ' + 'k-up . ekey ekey>fkey . . ekey ekey>char . .\rĀ\x1b[AĀxĀ' +
          'lib sound 0 0 snd-patch 0 60 snd-note 1 64 snd-note 1 snd-off 2 36 snd-drum 5 snd-claim 150 snd-volume\rĀĀ' +
          'create rb 256 allot 1 100 snd-level 1 90 snd-vol rb snd-regs rb $29 + c@ . rb $31 + c@ .\rĀĀ' +
          's" none.zsm" 2 snd-play .\rĀĀ' + '2 1000 snd-freq 3 72 snd-glide 200 10 20 2 snd-lfo 3 5 2 snd-sens 9 snd-noise\rĀĀ' +
          'rb snd-regs rb $2A + c@ . rb $32 + c@ . rb $3B + c@ . rb $0F + c@ .\rĀĀ' +
          '3 s" t240 o4 l16 c d" snd-mml . 3 s" t240 o4 l16 c e" snd-chord .\rĀĀĀ' +
          's" cat /dev/sndctl" sh drop\rĀĀ' + 's" /dev/sndctl" s" volume 100" ctl s" /dev/sndctl" s" frob" ctl\rĀĀ' +
          '1 >r 2 .\rĀ' + '3 . : t 1 >r 5 0 do i . loop r> . ; t\rĀ' + '1 if 2 then\rĀ' + '." hi"\rĀ' + 'synonym x >r x\rĀ' +
          ': u 7 x r> . ; u 2>r\rĀ' + 'lib greet words\rĀĀĀĀĀĀ' + 'bye\r',
      },
      expect: ['libs\nforth coreext exception file tools\n ok\n', '-lib tools\nunsupported operation\n',
        'lib string libs\nforth coreext exception file tools string\n ok\n',
        '-lib string libs\nforth coreext exception file tools (string)\n ok\n', 's" abc" s" abd" compare .\ncompare ?\n',
        'lib string s" abc" s" abd" compare .\n-1  ok\n', 'marker m lib double m libs\nforth coreext exception file tools string\n ok\n',
        'lib greet greet libs\n7 forth coreext exception file tools string greet\n ok\n', '-lib greet greet\ngreet ?\n',
        'see 2drop\n: 2drop drop drop ;\n', '<4> 7 9 11 49  ok\n', '5 3 tbit . . 0 15 sbit . $FFFF 0 cbit .\n0 5 -32768 -2  ok\n',
        '1000 random .\n29818 2479 3 257  ok\n',
        'cursor-save\n\x1b[K\x1b[J\x1b[3A\x1b[2C\x1b[1D\x1b7 ok\n', 'plain\n\x1b8\x1b[?25l\x1b[?25h\x1b[31m\x1b[104m\x1b[1m\x1b[2m\x1b[4m\x1b[5m\x1b[7m\x1b[0m ok\n',
        'at-xy page\n\x1b[38m\x0780 24 \x1b[8;4H\x1b[2J\x1b[H ok\n', 'ekey>char . .\n128 -1 128 -1 120  ok\n',
        'rb $29 + c@ . rb $31 + c@ .\n68 0  ok\n', 's" none.zsm" 2 snd-play .\nplay: none.zsm: not found\n1  ok\n', 'rb $3B + c@ . rb $0F + c@ .\n93 52 82 137  ok\n', 's" t240 o4 l16 c e" snd-chord .\n0 0  ok\n', 's" cat /dev/sndctl" sh drop\nvolume 150\nchannels 8\nclaimed 0 2\n ok\n', 's" frob" ctl\n/dev/sndctl: invalid argument\n',
        '1 >r 2 .\n>r: compile only\n', 'r> . ; t\n3 0 1 2 3 4 1  ok\n', '1 if 2 then\nif: compile only\n', '." hi"\n.": compile only\n',
        'synonym x >r x\nx: compile only\n', ': u 7 x r> . ; u 2>r\n7 2>r: compile only\n', 'lib greet words\n ', 'bye\n'],
      check(m, out) {
        const f = [], first = out.split('libs\n')[0], last = out.slice(out.lastIndexOf('lib greet words'));
        // (disasm: a Rockwell branch to itself, the indirect and indexed modes, a jsr to a word; see of a code word)
        if (!/^ ([0-9A-F]{4})  0F 12 FD  bbr0 \$12, \$\1\n [0-9A-F]{4}  B2 22     lda \(\$22\)\n [0-9A-F]{4}  7C 34 12  jmp \(\$1234,x\)\n [0-9A-F]{4}  B1 10     lda \(\$10\),y\n [0-9A-F]{4}  A1 10     lda \(\$10,x\)\n [0-9A-F]{4}  BE 00 80  ldx \$8000,y\n [0-9A-F]{4}  B6 10     ldx \$10,y\n [0-9A-F]{4}  87 20     smb0 \$20\n [0-9A-F]{4}  0A        asl a\n [0-9A-F]{4}  CB        wai\n [0-9A-F]{4}  20 [0-9A-F]{2} [0-9A-F]{2}  jsr \$[0-9A-F]{4}  \\ dup \n/m.test(out))
          f.push('disasm: not as it should be');
        if (!/lib disasm see 2drop\ncode 2drop \n [0-9A-F]{4}  E8        inx\n [0-9A-F]{4}  E8        inx\n [0-9A-F]{4}  60        rts\nend-code\n/.test(out))
          f.push('see of a code word (with disasm.fl): not as it should be');
        for (const ch of [0, 1, 2])                                                     // (The sound words' notes)
          if (!m.ym.keyOns.some(k => k.startsWith('ch ' + ch + ' '))) f.push('sound: no key-on on channel ' + ch + ': ' + m.ym.keyOns.join(', '));
        for (const [w, re] of [['five', /\b[0-9A-F]{4} l-f five /], ['twice', /\b[0-9A-F]{4} --f twice /], ['x', /\b[0-9A-F]{4} -if x /],
          ['bl', /\b[0-9A-F]{4} l-a bl /], ['dup', /\b[0-9A-F]{4} --a dup /], ['if', /\b[0-9A-F]{4} -ia if /], ['true', /\b[0-9A-F]{4} l-a true /]])
          if (!re.test(first)) f.push('words: ' + w + ' not shown as it should be');
        if (!/^lib greet words\n [^]* [0-9A-F]{4} --a rand [^]* [0-9A-F]{4} --f greet /.test(last))  // (Back where it was)
          f.push('words: greet (a .fs library\'s, searched again) not shown, or not as Forth, or not where it was');
        if (/[A-Z]{2}/.test(first.replace(/\b[0-9A-F]{4}\b/g, '').split('words\n')[1] || '')) f.push('words: a name not in lower case');
        return f;
      },
    },
    {
      name: 'flib', what: 'HyForth\'s lib and require after trouble: a bare name is tried in the current directory, then /lib/forth: with a directory of that name in the way (and in /pc with no PC tool) lib facility still loads; a library that fails to load (a bad .fl) is un-noted, so lib says so again rather than nothing; require of a file that isn\'t there, again',
      init: 't_rc', cycles: 160e6,
      machine: { input: 'āforth -l\r' + 'ācd /ram\r' + 'āmkdir facility.fl\r' + 'ālib facility\r' + 'ālibs\r' + 'āecho HYFLxx >bad.fl\r' + 'ālib bad\r' + 'ālib bad\r' + 'ārequire none.fs\r' + 'ārequire none.fs\r' + 'ācd /pc\r' + 'ālib string\r' + 'ālibs\r' + 'ābye\r' },
      expect: ['/ram> lib facility\n/ram> libs\nforth coreext exception file tools shell facility\n/ram> echo HYFLxx >bad.fl\n/ram> lib bad\nnot this forth\'s library\n/ram> lib bad\nnot this forth\'s library\n/ram> require none.fs\nnone.fs: not found\n/ram> require none.fs\nnone.fs: not found\n/ram> cd /pc\n',
        '/pc: i/o error\n/ram> lib string\n/ram> libs\nforth coreext exception file tools shell facility string\n/ram> bye\n'],
    },
    {
      name: 'fshell', what: 'HyForth as a shell (forth -l, shell.fl): its namespace and profile; a line Forth\'s or rc\'s by its first word (a number, one of the numbers library\'s, a word, a pipeline, a redirection); . on an empty stack, or rc\'s by % (a program a word shadows); cd and the prompt (its format); a definition over lines (the second prompt); status and $status; & ($apid) and wait; programs as values: sh-out, output-of, a word\'s output a program\'s input (|, piped: one that ends first, Ctrl-C), spawn; Ctrl-C ending a program; errors, a usage; -lib shell and lib shell; exit',
      init: 't_rc', cycles: 400e6,
      // (Each line typed at the shell's prompt (ā: "> " or "% "), but those it has none for: a definition's second line
      // (its prompt a tab), and the lines after -lib shell, a moment after the one before (Ā).  cat, waiting for input,
      // stopped by Ctrl-C; and a word that loops, its output into cat, stopped by Ctrl-C.  head -c: a usage, so head
      // ends before the word's output has: forth goes on)
      machine: {
        input: 'āecho b115200 >/dev/serctl\r' + 'āforth -l\r' + 'ā2 3 + .\r' + 'ā.\r' + 'ālib numbers\r' + 'ā2/3 1/6 n+ n. ndepth .\r' + 'āls /ram\r' + 'ācd /rom/lib/forth\r' + 'āpwd\r' +
          'āls startup.fs profile.fs | wc -l\r' + 'ā: twice\rĀ2 * ;\r' + 'ā3 twice .\r' + 'ācmp startup.fs profile.fs >/dev/null\r' +
          'āstatus .\r' + 'āecho $status\r' + 'ās" [%p] %% " prompt\r' + 'āsleep 1 &\r' + 'ās" apid" getenv evaluate wait status .\r' +
          'ā: free 1 ;\r' + 'āfree .\r' + 'ā% free\r' + 'ās" ls startup.fs" sh-out type status .\r' + 'ās" exit 3" sh-out nip . status .\r' +
          'ā: hi ." hello there" cr ;\r' + 'ā\' hi | wc -w\r' + 'ā\' hi s" wc -c" piped status .\r' + 'ā\' hi output-of type\r' +
          'ā: lots 300 0 do i . loop ;\r' + 'ā\' lots | head -c 20\r' + 'ā\' lots output-of nip .\r' + 'ās" sleep 1" spawn wait status .\r' +
          'ā: forever begin 1 . again ;\r' + 'ā\' forever | cat >/dev/null\rĀĀ\x03' +
          'ācat\rĀ\x03' + 'āecho $status\r' + 'ācd /none\r' + 'ābind -x a b\r' + 'ānosuch\r' + 'ā-lib shell\rĀ' + 'ls\rĀ' +
          'lib shell\r' + 'āecho back\r' + 'āexit\r' + 'āecho $status\r',
      },
      expect: ['% forth -l\nHyForth (Forth 2012), bye to end\n/> 2 3 + .\n5 \n/> .\nstack underflow\n/> lib numbers\n/> 2/3 1/6 n+ n. ndepth .\n5/6 0 \n/> ls /ram\nbin/\nlib/\n/> cd /rom/lib/forth\n' +
        '/rom/lib/forth> pwd\n/rom/lib/forth\n/rom/lib/forth> ls startup.fs profile.fs | wc -l\n      2\n' +
        '/rom/lib/forth> : twice\n\t2 * ;\n/rom/lib/forth> 3 twice .\n6 \n/rom/lib/forth> cmp startup.fs profile.fs >/dev/null\n' +
        '/rom/lib/forth> status .\n1 \n/rom/lib/forth> echo $status\n1\n/rom/lib/forth> s" [%p] %% " prompt\n[/rom/lib/forth] % sleep 1 &\n' +
        '[/rom/lib/forth] % s" apid" getenv evaluate wait status .\n0 \n[/rom/lib/forth] % : free 1 ;\n[/rom/lib/forth] % free .\n1 \n' +
        '[/rom/lib/forth] % % free\nram ',
        '[/rom/lib/forth] % s" ls startup.fs" sh-out type status .\nstartup.fs\n0 \n[/rom/lib/forth] % s" exit 3" sh-out nip . status .\n0 3 \n' +
        '[/rom/lib/forth] % : hi ." hello there" cr ;\n[/rom/lib/forth] % \' hi | wc -w\n      2\n' +
        '[/rom/lib/forth] % \' hi s" wc -c" piped status .\n     12\n0 \n[/rom/lib/forth] % \' hi output-of type\nhello there\n' +
        '[/rom/lib/forth] % : lots 300 0 do i . loop ;\n[/rom/lib/forth] % \' lots | head -c 20\nusage: head [-N] [file ...]\n' +
        '[/rom/lib/forth] % \' lots output-of nip .\n1090 \n[/rom/lib/forth] % s" sleep 1" spawn wait status .\n0 \n' +
        '[/rom/lib/forth] % : forever begin 1 . again ;\n[/rom/lib/forth] % \' forever | cat >/dev/null\ninterrupt\n[/rom/lib/forth] % cat\n',
        '\n[/rom/lib/forth] % echo $status\ninterrupt\n[/rom/lib/forth] % cd /none\n/none: not found\n' +
        '[/rom/lib/forth] % bind -x a b\nusage: bind [-a|-b] [-c] new old\n[/rom/lib/forth] % nosuch\nrc: nosuch: not found\n' +
        '[/rom/lib/forth] % -lib shell\n ok\nls\nls ?\nlib shell\n[/rom/lib/forth] % echo back\nback\n[/rom/lib/forth] % exit\n\n% echo $status\n\n%'],
    },
    {
      name: 'fhydra', what: 'HyForth\'s Hydra words (hylang\'s layer 2): argc under forth -l (0: -l isn\'t an argument); a directory read (open-dir, read-dir, close-dir), =mkdir, get-dir and set-dir (the prompt follows), ior>text; setenv, getenv, unsetenv; a note to itself taken by on-note\'s handler, between words and in a loop, and one it says no to (as Ctrl-C); pause',
      init: 't_rc', cycles: 200e6,
      machine: {
        input: ['echo b115200 >/dev/serctl', 'forth -l', 'require hydra.fl', 'argc .',
          ': ls-dir open-dir throw >r begin pad 64 r@ read-dir throw while pad swap type space repeat drop r> close-dir throw ;',
          's" /rom/lib" ls-dir', 's" /ram/newdir" 0 =mkdir . s" /ram" ls-dir', 's" /rom" set-dir . pad 64 get-dir type',
          's" /none" set-dir ior>text type', 's" foo" s" bar" setenv s" foo" getenv type s" foo" unsetenv s" foo" getenv nip .',
          ': h ." note " . true ;', '\' h on-note sys-getpid 16 note 7 .', ': lp 10 0 do i 5 = if sys-getpid 17 note then loop ." done" ;',
          'lp', ': h2 drop false ;', '\' h2 on-note sys-getpid 18 note 1 .', 'pause 2 .', 'exit'].map(l => 'ā' + l + '\r').join(''),
      },
      expect: ['/> argc .\n0 \n', '/> s" /rom/lib" ls-dir\nas bank basic edit font forth hylang namespace profile shell windows \n/> s" /ram/newdir" 0 =mkdir . s" /ram" ls-dir\n0 bin lib newdir \n' +
        '/> s" /rom" set-dir . pad 64 get-dir type\n0 /rom\n/rom> s" /none" set-dir ior>text type\nnot found\n' +
        '/rom> s" foo" s" bar" setenv s" foo" getenv type s" foo" unsetenv s" foo" getenv nip .\nbar0 \n' +
        '/rom> : h ." note " . true ;\n/rom> \' h on-note sys-getpid 16 note 7 .\nnote 16 7 \n' +
        '/rom> : lp 10 0 do i 5 = if sys-getpid 17 note then loop ." done" ;\n/rom> lp\nnote 17 done\n' +
        '/rom> : h2 drop false ;\n/rom> \' h2 on-note sys-getpid 18 note 1 .\ninterrupt\n/rom> pause 2 .\n2 \n/rom> exit\n'],
    },
    {
      name: 'fdev', what: 'HyForth\'s device libraries (hylang\'s layer 3, source over the devices\' files): gpio (pins, the port, ctl, CA1\'s edge), i2c (a memory written and read at a register, the devices), spi (an echo device\'s transactions, mode 3), cons (the window, the windows), proc (a task\'s args, cwd, regs, memory), clock (the chip, the time set), disk (a disk\'s ctl, the cards: one on SPI device 5), pc (the PC tool answers; a file of its read), and sound\'s note-of and tune (its notes on the YM2151, in time)',
      init: 't_rc', cycles: 320e6, pc: { files: { 'hi.txt': 'hi from the PC\n' } },
      get machine() {
        return { gpioIn: 0xA5, ca1: [200e6, 230e6, 260e6], i2c: { 0x50: 256, 0x68: 16 }, spiEcho: [3], sd: [card(5, 2048, false, () => 0)],
          rtc: Date.UTC(2026, 9, 3, 15, 4, 5) / 1000,
          input: ['forth -l', 'lib gpio lib i2c lib spi lib cons lib proc lib clock lib disk lib pc lib sound', 'libs',
            '2 gpio . 3 gpio . gpio-port .', '4 1 gpio! 6 gpio-out true gpio-ca1! 1 gpio-ca2! gpio-state type', 'gpio-wait 0> .',
            '1 i2c-reg-size $50 0 s" hello" i2c-write $50 0 pad 5 i2c-read pad 5 type', 'i2c-devices $50 i2c? . $51 i2c? .',
            'create b 1 c, 2 c, 3 c,', '3 b 3 spi b c@ . b 1+ c@ . b 2 + c@ .', '3 3 spi-mode 3 b 1 spi b c@ .',
            'window . windows type', 'variable t s" sleep 50" spawn t ! t @ task-args type t @ task-cwd type',
            't @ task-regs drop 3 type space t @ $E000 pad 2 task-mem pad @ $E000 @ = .',
            'rtc type', 's" 2030-01-02 03:04:05" set-date s" date" sh-out type', 'char x disk-ctl type', 'cards . char 5 disk-ctl type',
            'pc? .', 'variable f s" /pc/hi.txt" r/o open-file throw f !', 'pad 64 f @ read-file throw pad swap type', 'f @ close-file throw',
            's" C4" note-of . s" C#4" note-of . s" Db4" note-of . s" A4" note-of . s" B-1" note-of .', 's" C4 1 E4 1 - 1 G4 2" 0 600 tune',
            's" H4" note-of', 'exit'].map(l => 'ā' + l + '\r').join('') };
      },
      // (gpio: the pins $A5 and PA1 high, the I2C bus's pull-up; spi: the echo device's first byte $A0 in mode 0, $A3 in
      // mode 3, then each byte the one before; the ROM disk's ctl to its label, as the rest changes with its files)
      expect: ['/> libs\nforth coreext exception file tools shell gpio i2c spi hydra cons proc clock disk pc sound\n/> 2 gpio . 3 gpio . gpio-port .\n1 0 167 \n',
        '0 in 1\n1 in 1\n2 in 1\n3 in 0\n4 out 1\n5 in 1\n6 out 0\n7 in 1\nca1 rise 0\nca2 1\n/> gpio-wait 0> .\n-1 \n',
        'pad 5 type\nhello\n/> i2c-devices $50 i2c? . $51 i2c? .\n50 68 -1 0 \n',
        'b 2 + c@ .\n160 1 2 \n/> 3 3 spi-mode 3 b 1 spi b c@ .\n163 \n/> window . windows type\n0 0 0 80 24 *\n',
        'task-cwd type\n50\n/\n/> t @ task-regs drop 3 type space t @ $E000 pad 2 task-mem pad @ $E000 @ = .\nPC= -1 \n/> rtc type\nrunning\n',
        'sh-out type\n2030-01-02 03:04:0', '/> char x disk-ctl type\nrom 4 MB 8192 blocks\nhydrafs label=ROM\n', '/> cards . char 5 disk-ctl type\n32 sdhc 1 MB 2048 blocks\n/> pc? .\n-1 \n',
        'pad swap type\nhi from the PC\n/> f @ close-file throw\n',
        'note-of .\n60 61 61 69 11 \n/> s" C4 1 E4 1 - 1 G4 2" 0 600 tune\n/> s" H4" note-of\ninvalid numeric argument\n/> exit\n'],
      // (The tune: C4, E4 a beat on (a tenth of a second at 600 a minute), a rest, G4 two beats after E4)
      check(m) {
        const f = pcReport(m, 1, 0, 0), mult = JSON.parse(fs.readFileSync(path.join(TESTS, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const on = m.ym.keyOns.filter(k => k.startsWith('ch 0 ')).map(k => +k.match(/at cycle (\d+)/)[1]);
        const beat = 0.1 * 3579545 * mult, slack = 2 * 3579545 * mult / 200;
        if (on.length !== 3) return [...f, 'tune: ' + on.length + ' key-ons on channel 0, not 3: ' + m.ym.keyOns.join(', ')];
        [1, 2].forEach((beats, k) => { if (Math.abs(on[k + 1] - on[k] - beats * beat) > slack)
          f.push('tune: key-on ' + (k + 1) + ' came ' + (on[k + 1] - on[k]) + ' cycles after the last, not ' + beats + ' beat(s) (' + Math.round(beats * beat) + ')'); });
        return f;
      },
    },
    {
      name: 'findex', what: 'HyForth\'s index of the word lists (a bank\'s chains by a name\'s hash): a word redefined, a definition hidden till ;, a MARKER\'s words gone, MARKERs till the bank\'s full (started again), more word lists than records, EVALUATE of a bank\'s text (no index), the index\'s bank overwritten (started again); 500 searches of a name that isn\'t there in under 150 ticks (620 without the index)',
      init: 't_rc', cycles: 300e6,
      // (mk makes 100 words; cyc, 12 times: a MARKER, mk, the MARKER run, FORTH's chains made again each time, from
      // more nodes: the bank fills every few)
      machine: {
        input: ['echo b115200 >/dev/serctl', 'forth -l', ': dup 1 ; 5 dup . .', 'marker m : zz 7 ; zz . m zz', ': yy yy ;',
          ': mk 100 0 do s" create zz" evaluate loop ; : cyc 12 0 do s" marker m mk m" evaluate loop ; cyc zz',
          'lib search : wl9 10 0 do s" swap" wordlist search-wordlist . loop ; wl9 2 3 + .',
          'lib hydra 1 sys-banks-alloc throw bank! s" 2 3 + ." bank-window swap move bank-window 7 evaluate',
          'sys-banks 1- bank! 0 bank-window ! 6 7 + .', ': b sys-ticks 500 0 do c" nosuch" find 2drop loop sys-ticks swap - ; b 150 < .',
          'exit'].map(l => 'ā' + l + '\r').join(''),
      },
      expect: ['/> : dup 1 ; 5 dup . .\n1 5 \n', '/> marker m : zz 7 ; zz . m zz\n7 zz ?\n', '/> : yy yy ;\nyy ?\n',
        'cyc zz\nzz ?\n', 'wl9 2 3 + .\n0 0 0 0 0 0 0 0 0 0 5 \n', 'bank-window 7 evaluate\n5 \n', '0 bank-window ! 6 7 + .\n13 \n',
        'b 150 < .\n-1 \n/> exit\n'],
    },
    {
      name: 'wcache', what: 'HydraFS\'s walk cache (the names looked up, there or not, and the directories\' entries on the way): a name not there, then made, renamed, removed; directories made, removed, renamed under a name looked up; a name made through /lib\'s union after it wasn\'t there; each looked up again as it is now',
      init: 't_rc', cycles: 120e6,
      get machine() { return { input: typed(WC_LINES) }; },
      get expect() { return expected(WC_LINES); },
    },
    {
      name: 'fload', what: 'HyForth loading a file: its read-ahead (a CR LF across its 512-byte buffers, CR LF and CR line ends, a line of 130 cut at 128, a last line ended by a CR and the file\'s end), names between tabs, numbers with each prefix, in base 36, a double; hydra.fs REQUIREd in under 300 ticks (383 before 6.20)',
      init: 't_rc', cycles: 150e6,
      get machine() {
        return { sd: floadCard(), input: ['echo b115200 >/dev/serctl', 'forth -l', 'cd /sd/0', 'include load.fs', 'lib hydra',
          'sys-ticks require hydra.fs sys-ticks swap - 300 < .', 'exit'].map(l => 'ā' + l + '\r').join('') };
      },
      expect: ['include load.fs\n15 6 5 99 255 65 35 1 1 \n', 'swap - 300 < .\n-1 \n'],
    },
    {
      name: 'fnumbers', what: 'HyForth\'s numbers (numbers.fl, on the numbers and math libraries): numberstest.fth, each number word checked as the test suite checks (tester.fr\'s T{ ... -> ... }T): literals of every kind (and cells and doubles Forth\'s still), compiled with code banks and without, the number stack, the arithmetic, the tests, the conversions, the bits, text in the base and in others (set-base: cells read and shown by the library), the math functions, digits, the errors, nvariable, nconstant, nvalue; then n., n.base, n.s, nformat and cells in bases that aren\'t a radix, typed',
      init: 't_rc', cycles: 150e6,
      get machine() {
        return { sd: fnumCard(), input: 'ācd /sd/0; forth <nums.fs; echo $status\r' };
      },
      expect: ['Typed: 1/3 FF #b11111111 <3> 1 2 3 \n255 is FF and #b11111111, { and } {\n<-+> -+- --+  -0+00+ \n' +
        '#x255 -#x1 #xFFFF 255 \n0 errors in the number word tests\n\n%'],
    },
    {
      name: 'fnumquick', what: 'HyForth\'s numbers done in numbers.fl itself (small integers, of 4 bytes or fewer; n/trunc; a variable\'s small integer in its cell, its block used again): sim/tools/fnumxcheck.js\'s cases (seed 1), random operands of every size and kind through n+ n- n* n/ n/mod n/trunc, the comparisons, n0= n0<, nnegate nabs, truncate nfloor nround, s>n d>n, n! n@ and TO, each result\'s bytes (nbytes) as numref.js has them; the errors and the stack\'s edges (64 numbers, 0 to 12 bytes of its 8K left) run, as fnumxcheck.js --base compares with another build',
      init: 't_rc', cycles: 400e6,
      get machine() {
        return { sd: fnumqCard(), input: '\u0101cd /sd/0; forth fq.fs; echo $status\r' };
      },
      expect: ['==end\n'],
      check(m, out) {
        const f = [], a = out.indexOf('==start\n'), b = out.indexOf('==end');
        if (a < 0 || b < 0) return ['no ==start ... ==end'];
        const got = out.slice(a + 8, b).split('\n').filter(l => l !== '');
        if (got.length !== FNUMQ.lines.length) f.push(got.length + ' lines, not ' + FNUMQ.lines.length);
        FNUMQ.expect.forEach((e, k) => { if (e !== null && got[k] !== e && f.length < 10) f.push(FNUMQ.lines[k] + ': ' + got[k] + ', not ' + e); });
        return f;
      },
    },
    {
      name: 'lshell', what: 'the shell /lib/shell names (the ROM\'s: /bin/forth -l, HyForth the login shell): init\'s in window 0, and in a window made (Ctrl-] c: the kernel starts init there, $window); send, a line typed in another window (#cN/kbdin), run there, then one longer than its keys\' queue (the write waiting for room)',
      init: 'init', cycles: 300e6,
      // (Window 1 made and shown (\x1d c), its shell sends window 0 a line, then one longer than window 0's keys' queue
      // (63), as the first still runs there: its write waits for room; window 0 shown again (\x1d 0): its text, the
      // lines run there)
      get machine() {
        return { input: 'ā2 3 + .\r' + 'āecho $window\r' + 'ā\x1dc' + 'āecho $window\r' + 'āsend 0 echo hi from 1\r' +
          'āsend 0 echo ' + SEND_LONG + '\r' + 'ā\x1d0' + 'āecho back in 0\r' };
      },
      expect: ['HyForth (Forth 2012), bye to end\n/> 2 3 + .\n5 \n/> echo $window\n\n/> ', '/> echo $window\n1\n/> send 0 echo hi from 1\n/> ',
        '/> send 0 echo ' + SEND_LONG + '\n/> ',
        '/> echo hi from 1\nhi from 1\n/> echo ' + SEND_LONG + '\n' + SEND_LONG + '\n/>', 'echo back in 0\nback in 0\n/> '],
      // (Window 0's screen drawn again as it's shown: its prompt there already, the cursor put after it, or not yet)
    },
  ],
};
