// ****************************************************************************
// tests.js - the tests sim/test.js runs, in order.  Each boots a paged ROM of the system's modules and its own
// (tests/mod), with one of them as init, and runs till init says "NAME: PASS" or "NAME: FAIL" (or the cycles run
// out).  A test module's lines are "ok - ..." or "not ok - ..." (tests/mod/testlib.inc).
//
//   name, what       the test
//   init             the module started as init (a test module, or the system's init)
//   modules          more modules for the image (the system's are always there, but those in without)
//   without          system modules left out (a test that owns the serial port itself, or needs its driver as
//                    task F)
//   cycles           at most this many (3.58 MHz: 3579545 a second)
//   expect           lines the output must have (for a test without "PASS")
//   machine          the emulator's options for it (sim/lib/machine.js): faults, keys typed (input), modules
//   send             { after: a mark, bytes: [...] }: the PC sends them, back to back, when the mark is out
//   pc               { files, readOnly, damage }: a folder at /pc, the emulator the PC tool (sim/test.js: pcFolder);
//                    check(m) has it as m.pc ({ dir, host })
//   budgets          [{ what, from, to, minus, per, max }]: the cycles between two marks (less those between the
//                    two marks in minus, a baseline), divided by per, at most max (a number, or a function of the
//                    build's options: { clock, acia }, obj/build.json)
//   check(m, out)    more checks on the machine afterwards: gives a list of failures
//   start(m)         the machine as it's made, before it runs (to listen to its sound, say)
// Every test also checks the longest IRQs-off stretch after the boot (IRQ_OFF_MAX).
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../sim/tools/hydrafs.js');
const { createXmodemPeer } = require('../sim/lib/xmpeer.js');
const numtest = require('./numtest.js');
const { VT, DEC_ASCII } = require('../../base/sim/lib/vt.js');
const { createWin32Input } = require('../../base/sim/lib/win32in.js');

const IRQ_OFF_MAX = 200;                                      // (docs/design/reimplementation-from-scratch.md, §8: 115200)
const S1_BYTES = 2000;

// An SD card for the emulator (sim/lib/sd.js) on SPI device dev, its blocks in memory: byte i of block n is fill(n, i)
function card(dev, blocks, sdsc, fill) {
  const data = new Uint8Array(blocks * 512);
  for (let n = 0; n < blocks; n++) for (let i = 0; i < 512; i++) data[n * 512 + i] = fill(n, i) & 0xFF;
  return { dev, blocks, sdsc, data, read: n => data.slice(n * 512, n * 512 + 512), write: (n, b) => data.set(b, n * 512) };
}
const DISK_CARDS = [card(0, 2048, false, (n, i) => n * 7 + i), card(1, 4096, true, (n, i) => n * 13 + i + 1)];

// An SD card on SPI device dev from an image file, claiming blocks (those past the file's end read as zeros); its
// writes kept, and save() puts them in the file (for the PC tool to look at)
const CARD_DIR = path.join(__dirname, '..', 'obj', 'cards'), OLD_CARDS = path.join(__dirname, '..', '..', 'old', 'sim', 'cards');
// The SD card's image (bin/sdcard.img: the samples, the songs, the benchmarks: tools/sdcard.js), for the tests that run a sample
// (sdcard: true): in SD device 0, its writes kept apart (imageCard's), so the image stays as it was built
const SDCARD = path.join(__dirname, '..', 'bin', 'sdcard.img');
function sdcardOf(t) {
  const d = Object.getOwnPropertyDescriptor(t, 'machine');
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if ((m.sd || []).some(c => c.dev === 0)) throw new Error(t.name + ': its own card in SD device 0, and sdcard');
    return Object.assign({}, m, { sd: [imageCard(0, SDCARD, fs.statSync(SDCARD).size / 512), ...(m.sd || [])] });
  } });
}

function imageCard(dev, file, blocks) {
  const base = fs.readFileSync(file), written = new Map();
  return { dev, blocks, file,
    read: n => written.get(n) || Buffer.concat([n * 512 < base.length ? base.subarray(n * 512, n * 512 + 512) : Buffer.alloc(0)], 512),
    write: (n, b) => written.set(n, Buffer.from(b)),
    save() { const fd = fs.openSync(file, 'r+'); for (const [n, b] of written) fs.writeSync(fd, b, 0, 512, n * 512); fs.closeSync(fd); } };
}

// The fs test's cards: 0, a HydraFS made by the PC tool (hello.txt, games/star.txt, big.bin: byte i is i * 7); 1, blank;
// 2 and 3, the old system's fixture cards (sim/cards: a version 1 volume, a version 2 one), copies; 4, partitioned (a FAT
// partition first, the PC's, then the HydraFS: part.txt); 5, the version 1 card with a cluster marked in use that
// nothing uses (a lost one, for the check to find); 6, a blank 1 GB card
function fsCards() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f0 = path.join(CARD_DIR, 'fs0.img'), f1 = path.join(CARD_DIR, 'fs1.img');
  const f2 = path.join(CARD_DIR, 'fs2.img'), f3 = path.join(CARD_DIR, 'fs3.img');
  hydrafs.mkfs(f0, 8, 'TESTS', undefined, true);
  const v = new hydrafs.Volume(f0);
  v.put('hello.txt', Buffer.from('hello, hydrafs\n'));
  v.mkdir('games');
  v.put('games/star.txt', Buffer.from('a star\n'));
  v.put('big.bin', Buffer.from(Array.from({ length: 9000 }, (_, i) => (i * 7) & 0xFF)));
  v.close();
  fs.writeFileSync(f1, Buffer.alloc(512));
  fs.copyFileSync(path.join(OLD_CARDS, 'tests-v1.img'), f2);
  fs.copyFileSync(path.join(OLD_CARDS, 'quick-v2.img'), f3);
  const f4 = path.join(CARD_DIR, 'fs4.img'), f5 = path.join(CARD_DIR, 'fs5.img'), f6 = path.join(CARD_DIR, 'fs6.img');
  hydrafs.mkfs(f4, 8, 'PART', undefined, true, 1);
  const v4 = new hydrafs.Volume(f4);
  v4.put('part.txt', Buffer.from('in a partition\n'));
  v4.close();
  fs.copyFileSync(path.join(OLD_CARDS, 'tests-v1.img'), f5);
  const v5 = new hydrafs.Volume(f5);
  let lost = 1000;
  while (v5.used(lost)) lost++;
  v5.setUsed(lost, true);
  v5.freeCount = v5.freeCount - 1;
  v5.close();
  fs.writeFileSync(f6, Buffer.alloc(512));
  return [imageCard(0, f0, 16384), imageCard(1, f1, 8192), imageCard(2, f2, 131072), imageCard(3, f3, 131072),
    imageCard(4, f4, 16384), imageCard(5, f5, 131072), imageCard(6, f6, 2097152)];
}

// The load test's card: hello.txt, and in bin the test RAM programs as built (tests/ram: obj/tests/NAME.hyx): t_ram,
// t_big, t_short (t_ram cut short: its header whole) and t_low (t_ram, its header saying it loads at $0400)
function loadCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'load0.img'), prog = n => fs.readFileSync(path.join(__dirname, '..', 'obj', 'tests', n + '.hyx'));
  const ram = prog('t_ram'), low = Buffer.from(ram);
  low.writeUInt16LE(0x0400, 8);
  hydrafs.mkfs(f, 8, 'LOAD', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('hello.txt', Buffer.from('hello, hydrafs\n'));
  v.mkdir('bin');
  v.put('bin/t_ram', ram);
  v.put('bin/t_big', prog('t_big'));
  v.put('bin/t_short', ram.subarray(0, 300));
  v.put('bin/t_low', low);
  v.close();
  return [imageCard(0, f, 16384)];
}
// The debugger's step-over test's program (dbover): a subroutine that calls itself, then a call of the kernel's
// that takes two seconds, for ever
const DBOVER_S = ['.include "hydra.inc"', '.include "hyx2.inc"', '', '            HYX2_PROGRAM "over", main', '', '.code',
  'main:       lda #3', '            jsr down', "            lda #'!'", '            jsr PUTC', '@nap:       lda #<(TICK_HZ * 2)',
  '            ldx #>(TICK_HZ * 2)', '            jsr SLEEP', "            lda #'.'", '            jsr PUTC', '            bra @nap', '',
  'down:       pha', "            ora #'0'", '            jsr PUTC', '            pla', '            dec a', '            beq @end',
  '            jsr down', '@end:       rts', ''].join('\n');

// The step test's card: in bin, t_steppee (tests/ram), the program t_step steps
function stepCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'step0.img');
  hydrafs.mkfs(f, 8, 'STEP', undefined, true);
  const v = new hydrafs.Volume(f);
  v.mkdir('bin');
  v.put('bin/t_steppee', fs.readFileSync(path.join(__dirname, '..', 'obj', 'tests', 't_steppee.hyx')));
  v.close();
  return [imageCard(0, f, 16384)];
}

const BIG_LENGTH = () => fs.statSync(path.join(__dirname, '..', 'obj', 'tests', 't_big.hyx')).size;

// /bin's programs in the rc and tools tests: the ROM's program modules (their headers' type, HT_PROGRAM), the ROM
// disk's bin, and t_rc (init)
function BIN_COUNT() {
  const root = path.join(__dirname, '..'), { readManifest } = require('../build.js');
  const mods = readManifest(path.join(root, 'modules', 'rom.txt')).modules
    .filter(n => fs.readFileSync(path.join(root, 'obj', 'modules', n + '.bin'))[5] === 1);
  const disk = fs.readFileSync(path.join(root, 'romfs', 'romfs.txt'), 'latin1').split(/\r?\n/).filter(l => /^bin\//.test(l));
  return mods.length + disk.length + 1;
}

// The ROM disk's bin (/rom/bin): its programs' names, as ls lists them
function ROM_BIN() {
  return fs.readFileSync(path.join(__dirname, '..', 'romfs', 'romfs.txt'), 'latin1').split(/\r?\n/).filter(l => /^bin\//.test(l))
    .map(l => l.split(/\s+/)[0].slice(4)).sort();
}

// The rc test's lines, and what each says
const RC_LINES = [
  ["echo hello","hello"],
  ["cat /env/prog; echo","/bin/cat"],
  ["x=(a b c); echo $x $#x $x(2)","a b c 3 b"],
  ["echo 'a  b' 'it''s'","a  b it's"],
  ["echo $\"x","a b c"],
  ["echo x^$x $x^1 $x^$x","xa xb xc a1 b1 c1 aa bb cc"],
  ["y=1 echo $y; echo $#y","1\n0"],
  ["echo $x(2-) $x(1-2)","b c a b"],
  ["echo one >/ram/f; echo two >>/ram/f; cat /ram/f","one\ntwo"],
  ["cat </ram/f >[2=1]","one\ntwo"],
  ["wc <[0=]","wc: -: bad file descriptor\n      0       0       0"],
  ["{echo b >[1=3]} >[3]/ram/y >/dev/null; cat /ram/y","b"],
  ["echo piped | cat","piped"],
  ["echo a b | cat | cat","a b"],
  ["echo a b c d | cat | cat | cat | wc -w","      4"],
  ["rc -c 'i=(); while() {i=($i x); echo $#i; ~ $#i 3 && exit}'","1\n2\n3"],
  ["if() echo always","always"],
  ["if(~ a a) echo yes; if not echo no","yes"],
  ["if(~ a b) echo yes; if not echo no","no"],
  ["for(i in 1 2 3) echo $i","1\n2\n3"],
  ["n=(); while(! ~ $#n 3) n=($n x); echo $#n","3"],
  ["switch(b){case a; echo A; case b c; echo B}","B"],
  ["fn greet {echo hi $1 $#*}; greet you there","hi you 2"],
  ["echo `{echo inner} after","inner after"],
  ["~ a a && echo and; ~ a b || echo or","and\nor"],
  ["cat /nothing; echo status $status","cat: /nothing: not found\nstatus 1"],
  ["echo /rom/lib/n*","/rom/lib/namespace"],
  ["echo /rom/lib/*","/rom/lib/as /rom/lib/bank /rom/lib/basic /rom/lib/edit /rom/lib/font /rom/lib/forth /rom/lib/hylang /rom/lib/namespace /rom/lib/profile /rom/lib/shell /rom/lib/windows"],
  ["echo 'no*match'*","no*match*"],
  ["cd /rom/lib; pwd; cd","/rom/lib"],
  ["rc -c 'echo sub $x'","sub a b c"],
  ["{echo in a block} >/ram/g; cat /ram/g","in a block"],
  ["whatis greet","fn greet {echo hi $1 $#*}"],
  ["{echo back >/ram/bg} & wait; cat /ram/bg; echo $#apid","back\n1"],
  ["echo 'echo script $1 $0' >/ram/s; rc /ram/s arg","script arg /ram/s"],
  ["echo 'z=sourced' >/ram/d; . /ram/d; echo $z","sourced"],
  ["echo x >/dev/sd/x/data; whatis status","echo: write error: read-only\nstatus='write error'"],
  ["eval echo evaled $x(1)","evaled a"],
  ["fn sh {shift; echo $*}; sh a b c","b c"],
  ["rc -c 'exit oops'; echo $status","oops"],
  ["echo (a","rc: syntax error"],
  ["whatis echo x; q=('it''s' '' a.b); whatis q","/bin/echo\nx=(a b c)\nq=('it''s' '' a.b)"],
  ["bind '#n' /mnt; ls /mnt","null\nzero\nkmesg"],
  ["{mount /mnt}","rc: usage: mount [-abc] '#x' old [spec]"],
  ["{bind /mnt}","rc: usage: bind [-abc] new old"],
  ["ls /rom/lib","as/\nbank/\nbasic/\nedit/\nfont/\nforth/\nhylang/\nnamespace\nprofile\nshell\nwindows"],
  ["cat /bin/echo >/ram/hi; cd /ram; hi from dot; cd","from dot"],
  ["cat /nothing >[2]/ram/e; cat /ram/e","cat: /nothing: not found"],
  ["cat /nothing |[2] cat >/ram/p; echo -n 'p: '; cat /ram/p","p: cat: /nothing: not found"],
  ["echo $task $#path $path # a comment","2 2 . /bin"],
  ["path=(); ls; path=(. /bin); ls /rom/lib","rc: ls: not found\nas/\nbank/\nbasic/\nedit/\nfont/\nforth/\nhylang/\nnamespace\nprofile\nshell\nwindows"],
  ["! ~ a b && echo not; echo $status","not\n"],
];

// hylang as the shell (hysh): the rc test's lines that stand alone at hylang -l's prompt (each runs in an rc of its own:
// not those that use what an earlier line set, $x or greet, nor $task, rc's own, nor a block first, which is hylang's);
// then the shell's own: hylang's lines, cd (the prompt follows), $status and status, bind and unmount in hylang's own
// namespace (an rc line after sees it), a usage, rc's not found, & and $apid, Ctrl-C to cat (rc's), exit
const HYSH_RC = RC_LINES.filter(([l]) => l[0] !== '{' &&
  !/^(echo \$"x|echo x\^|echo \$x\(2-\)|rc -c 'echo sub|whatis greet|whatis echo x|eval echo evaled|echo \$task)/.test(l));
const HYSH_LINES = [
  ['(+ 1 2)', '=> 3'], ['(map (fn {x} {* x x}) {1 2 3})', '=> {1 4 9}'], ['cd /rom/lib', null, '/rom/lib'], ['pwd', '/rom/lib'],
  ['ls | wc -l', '     11'], ['cmp namespace profile >/dev/null', null], ['(+ status 0)', '=> 1'], ['echo $status', '1'],
  ['cd /none', '/none: not found'], ['bind -x a b', 'usage: bind [-abc] new old'], ["bind -a '#n' /mnt", null], ['ls /mnt', 'null\nzero\nkmesg'],
  ['unmount /mnt', null], ['ls /mnt', null], ['nosuch', 'rc: nosuch: not found'], ['sleep 1 &', null], ['echo $#apid', '1'],
  ['cd', null, '/'],
];

const TOOL_LINES = [
  ["mkdir /ram/t /ram/t/a; ls /ram/t","a/"],
  ["mkdir /ram/t; echo $status","mkdir: /ram/t: already exists\n1"],
  ["mkdir -p /ram/t/b/c/d; ls /ram/t/b/c","d/"],
  ["touch /ram/t/f /ram/t/a/g; ls /ram/t","a/\nb/\nf"],
  ["echo hi >/ram/t/f; cp /ram/t/f /ram/t/f2; cat /ram/t/f2","hi"],
  ["cp /ram/t/f /ram/t/f; echo $status","cp: /ram/t/f: the same file\n1"],
  ["cp -r /ram/t /ram/u; ls /ram/u /ram/u/a /ram/u/b/c","a/\nb/\nf\nf2\ng\nd/"],
  ["mv /ram/t/f2 /ram/t/f3; ls /ram/t","a/\nb/\nf\nf3"],
  ["mv /ram/t/f3 /ram/u; ls /ram/u","a/\nb/\nf\nf2\nf3"],
  ["mv /ram/u/f3 /sram/f3; cat /sram/f3; ls /ram/u","hi\na/\nb/\nf\nf2"],
  ["rm /ram/t/a; echo $status","rm: /ram/t/a: directory not empty\n1"],
  ["rm -r /ram/t; ls /ram","bin/\nlib/\nu/"],
  ["rmdir /ram/u/b/c/d; ls /ram/u/b/c",null],
  ["rmdir /ram/u/f; echo $status","rmdir: /ram/u/f: not a directory\n1"],
  ["ls -ld /rom/lib /rom", [
    "d-r--r--r-- fx        0 2000-01-01 00:00 /rom/lib",
    "d-r--r--r-- fx        0 2000-01-01 00:00 /rom",
  ].join('\n')],
  ["ls -d /rom /ram","/rom/\n/ram/"],
  ["rm -f /nothing; echo $status",""],
  ["ls -x; echo $status","usage: ls [-ld] [name ...]\nusage"],
  ["du /ram/u; du -a /ram/u/a","0\t/ram/u/a\n0\t/ram/u/b/c\n0\t/ram/u/b\n2\t/ram/u\n0\t/ram/u/a/g\n0\t/ram/u/a"],
  ["df","disk  kind   size        free        label\n0     sdhc   16380 KB    ",true],
  ["cat /proc/$task/args; cd /ram/u; cat /proc/$task/cwd; cd","-l\n/ram/u"],
  ["ns", [
    "bind '#/' /",
    "bind '#/dev' /dev",
    "bind -a '#c' /dev",
    "bind -a '#n' /dev",
    "bind -a '#t' /dev",
    "bind -a '#a' /dev",
    "bind '#g' /dev/gpio",
    "bind '#i' /dev/i2c",
    "bind '#d' /dev/sd",
    "bind '#S' /dev/spi",
    "bind '#m' /dev/mod",
    "bind '#s' /dev/seg",
    "bind '#e' /env",
    "bind '#p' /proc",
    "bind '#f' /sd",
    "mount '#f' /rom x",
    "mount '#f' /sram s",
    "mount -c '#f' /ram r/2",
    "bind -c '#fr/2/bin' /bin",
    "bind -a '#fs/bin' /bin",
    "bind -a '#fx/bin' /bin",
    "bind -a '#m/bin' /bin",
    "bind -c '#fr/2/lib' /lib",
    "bind -a '#fs/lib' /lib",
    "bind -a '#fx/lib' /lib",
    "bind '#P' /pc",
  ].join('\n')],
  ["ps -a","task  state   parent     cpu group  name\n   0  ready   -",true],
  ["mods", [
    "bank   type     name",
    "  4+33 program  init",
    "  2- 4 boot     cons",
    "  5- 6 boot     storage",
    " 32    boot     kdev",
    "  4+0A boot     snd",
    "",
  ].join('\n'), true],
  ["free","ram     256 KB a task (2 modules)\nshared  1024 KB, 256 KB in segments (1), 768 KB free"],
  ["sleep 30 & sleep 30 & kill $apid; slay sleep; wait; ps","task  state",true],
  ["kill 8; kill x; echo $status","kill: 8: no such task\nkill: x: invalid argument\n1"],
  ["sleep 1; echo slept","slept"],
  ["ls /rom/bin; whatis mkfs", [...ROM_BIN(), '/bin/mkfs'].join('\n')],
  ["label s; label s Shared Disk; label s","SRAM\nShared Disk"],
  ["fsck s","hydrafs label=Shared Disk\nfree 253 KB of 255 KB\ncheck: lost 0, unmarked 0, twice 0\nsegment 15"],
  ["mkfs s Fresh; ls /sram; label s; echo $status","Fresh\n"],
  ["mkfs; label nodisk","usage: mkfs [-fp] disk [label ...]\nlabel: nodisk: not found"],
  ["echo one two >/ram/w; echo three >>/ram/w; wc /ram/w; wc -l /ram/w /ram/w; echo a b | wc -w", [
    "      2       3      14 /ram/w",
    "      2 /ram/w",
    "      2 /ram/w",
    "      4 total",
    "      2",
  ].join('\n')],
  ["for(i in 1 2 3 4 5 6 7 8 9 10 11 12) echo $i >>/ram/n; head -3 /ram/n; tail -2 /ram/n; head /ram/n | tail -1", [
    "1",
    "2",
    "3",
    "11",
    "12",
    "10",
  ].join('\n')],
  ["echo hi | tee /ram/t1 /ram/t2; cat /ram/t2; echo more | tee -a /ram/t2 >/dev/null; cat /ram/t2", [
    "hi",
    "hi",
    "hi",
    "more",
  ].join('\n')],
  ["for(i in a a b b b c a) echo $i >>/ram/q; uniq /ram/q; uniq -c /ram/q", [
    "a",
    "b",
    "c",
    "a",
    "      2 a",
    "      3 b",
    "      1 c",
    "      1 a",
  ].join('\n')],
  ["echo hello there | xd","0000000  68 65 6c 6c 6f 20 74 68 65 72 65 0a              hello there."],
  ["cmp /ram/t1 /ram/t1; echo $status; cmp /ram/t1 /ram/t2; cmp /ram/w /ram/t1", [
    "",
    "cmp: end of /ram/t1",
    "/ram/w /ram/t1 differ: byte 1",
  ].join('\n')],
  ["/sd/0/sample/hi Ann Bob","Hello, Ann!\nHello, Bob!\nI'm task 3, in /, in window 0."],
  ["echo Hello there | /sd/0/sample/upper","HELLO THERE"],
  ["cat '#k/count'; echo add 5 >'#k/ctl'; echo add 2 >'#k/ctl'; cat '#k/count' '#k/ctl'; echo reset >'#k/ctl'; cat '#k/ctl'", [
    "0",
    "7",
    "7",
    "0",
  ].join('\n')],
  ["echo frob >'#k/ctl'; echo add >'#k/ctl'", [
    "echo: write error: invalid argument",
    "echo: write error: invalid argument",
  ].join('\n')],
  ["cat /dev/rtc; date","none\n2000-01-01 00:0",true],
];

// The C test's lines (as the tools test's): the C SDK's samples, the library's test (ctest: its "ok" lines), and the
// tools in C (sort and grep)
// The chorus sample's lines (sdk/c/samples/chorus), and the round sample's tune: each note's length, in eighths
const CHORUS = ['Row, row, row your boat,', 'Gently down the stream.', 'Merrily, merrily, merrily, merrily,', 'Life is but a dream.'];
const ROUND_LENS = [3, 3, 2, 1, 3, 2, 1, 2, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 6];

const C_LINES = [
  ["/sd/0/sample/c/hello world","hello from C, world"],
  ["echo hello there | /sd/0/sample/c/upper","HELLO THERE"],
  ["/sd/0/sample/c/code 3; echo $status","3"],
  ["/sd/0/sample/c/code oops; echo $status","oops"],
  ["/sd/0/sample/c/harn 99 | head -6", [
    "Human Female, born 28 Nolus (Ahnu); Feudal Serf",
    "2 of 4; Fostered, Favorite; clanhead: Distant Relation",
    "71\", Medium, 165 lb (size 6); Attractive 13; Medium, Black hair, Hazel eyes",
    "Strength 15, Stamina 11, Dexterity 7, Agility 6, Eyesight 9, Hearing 7, Smell 9, Voice 6",
    "Intelligence 14, Aura 16, Will 11, Morality 10",
    "Endurance 12, Move 6, Piety 19; medical: Parasites Hemophilia",
  ].join('\n')],
  ["/sd/0/sample/c/harn 99 | grep 'Automatic|broad|best|D&D'", [
    "Automatic skills (SB/ML): Climbing 9/36 Condition 12/60 Jumping 9/36 Stealth 8/24 Throwing 10/40 Awareness 8/32 Intrigue 14/42 Oratory 11/22 Rhetoric 10/30 Singing 6/18 Language 10/60 Initiative 9/36 Unarmed 9/36 Dodge 6/30",
    "A broadsword: A*1 on the left calf, effective impact 11: serious, 2 levels; UP 2",
    "the best 3 of 5 d6: 15 (6 5 4)",
    "D&D attributes: 14 (5 5 4 2) 14 (6 4 4 4) 13 (6 5 2 1) 13 (5 4 4 3) 12 (5 4 3 1) 9 (4 3 2 1)",
  ].join('\n')],
  ["cd /ram; /sd/0/sample/c/ctest a 'b c'","ok - arguments",true],
  ["bind -a /sd/0/games /bin", null],
  ["hmchar -q -s 5 -u feudal -c hunter -o /ram/t.hm | head -2", "\nTordan, Human Male, born 28 Peonu (Aralius); Feudal Serf, Hunter/Trapper"],
  ["grep -c '^ml ' /ram/t.hm; grep 'trk|bow|occupation|^fam' /ram/t.hm", "24\nfam 2 1 1 4 3 3 1\nml bow 36\nml trk 80\noccupation Hunter/Trapper"],
  ["for(w in pear apple fig Apple banana 10 9 07) echo $w >>/ram/s; sort /ram/s", [
    "07",
    "10",
    "9",
    "Apple",
    "apple",
    "banana",
    "fig",
    "pear",
  ].join('\n')],
  ["sort -n /ram/s; sort -rf /ram/s | head -4; sort -fu /ram/s | wc -l", [
    "Apple",
    "apple",
    "banana",
    "fig",
    "pear",
    "07",
    "9",
    "10",
    "pear",
    "fig",
    "banana",
    "apple",
    "      7",
  ].join('\n')],
  ["grep an /ram/s; grep -n '^[a-f]' /ram/s; grep -i -c apple /ram/s","banana\n2:apple\n3:fig\n5:banana\n2"],
  ["grep -v 'e|a' /ram/s; grep -c '^-?[0-9]+$' /ram/s","fig\n10\n9\n07\n3"],
  ["echo pineapple >/ram/s2; grep 'p+le$' /ram/s /ram/s2; grep -l fig /ram/s /ram/s2","/ram/s:apple\n/ram/s:Apple\n/ram/s2:pineapple\n/ram/s"],
  ["grep zzz /ram/s; echo $status; grep '(ab' /ram/s; echo $status; grep x /ram/nosuch; echo $status", [
    "no matches",
    "grep: bad expression: no )",
    "bad expression",
    "grep: /ram/nosuch: not found",
    "1",
  ].join('\n')],
  ["sort -x; echo $status; grep; echo $status", [
    "usage: sort [-bfnru] [file ...]",
    "usage",
    "usage: grep [-chilnsv] [-e] pattern [file ...]",
    "usage",
  ].join('\n')],
];

// The numbers in C (cnum's lines, as the C test's): num.h's test (ntest: its "ok" lines), calc at rc (rc's own
// characters quoted: ^ * ( ) #), and the assembly sample nsum (numbers.inc's macros, numlib.s)
const CNUM_LINES = [
  ["/sd/0/sample/c/ntest","ok - num_init: the libraries",true],
  ["calc 2/3 + 0.5; calc sqrt 2; calc -b x 255","7/6\n1.41421356237\nFF"],
  ["calc '2^100'; calc -d 30 pi; calc -b '#b' 0.75","1267650600228229401496703205376\n3.14159265358979323846264338328\n#b0.11"],
  ["calc '(1+2)*3'; calc 'log(e)^2'; calc 'sqrt 2^2'; calc 'gcd(12, 18)'","9\n1\n2\n6"],
  ["calc -17 % 5; calc 'fixed(1/3, 5)'; calc '#xFF + 1'; calc sqrt -4; calc -b c 5","-2\n0.33333\n256\n2i\n--+"],
  ["calc '4/3 - 8/3i'; calc '3.9 + 1/2i'; calc 1/2i; calc '1/(2i)'; calc '4/3+3.7i'","4/3-8/3i\n3.9+1/2i\n1/2i\n-1/2i\n4/3+3.7i"],
  ["calc 1/0; echo $status; calc foo; calc 2 +; calc -b zz 5", [
    "calc: division by zero",
    "division by zero",
    "calc: foo: unknown",
    "calc: an operand is missing",
    "calc: zz: not a base",
  ].join('\n')],
  ["echo 1/3+1/6 >/ram/e; echo x >>/ram/e; echo '0.1 + 0.2' >>/ram/e; calc </ram/e; echo $status","1/2\ncalc: x: unknown\n0.3\nx: unknown"],
  ["calc 'fib 2000' | wc -c; calc -b b '2^1000' | wc -c","    419\n   1002"],
  ["/sd/0/sample/nsum 1/3 0.5 2; /sd/0/sample/nsum 1 x; echo $status","17/6\nsqrt 1.68325082306\nnot a number"],
];

// The console on the Vera X's screen (phase 8: cons's second terminal, vid's /term), at rc: /dev/vid; consctl's
// terminal; the serial port alone (the screen left as it was: a regexp that doesn't match itself, z[z]z, counts
// zzz on the screen), then both (the screen repainted from the window's text); colours (SGR, a file on the PC);
// a font from the ROM disk; a bad command
const SCREEN_LINES = [
  ["ls /dev/vid", "ctl\nterm\nvram\npal\nsprites\nfont\nframe\npsg\npcm\npcmctl\nmouse\nmousein\nmousectl\ndraw"],
  ["cat /dev/vid/ctl", "vera 47.0.2\nmode 80x60\ncursor blink\nborder 0\nbitmap off\noutput vga\nclaimed"],
  ["grep terminal /dev/consctl", "terminal both"],
  ["echo serial >/dev/consctl; echo z^zz; grep -c 'z[z]z' /dev/vid/term; echo both >/dev/consctl", "zzz\n0"],
  ["grep -c 'z[z]z' /dev/vid/term", "1"],
  ["cat /lib/font/cp437 >/dev/vid/font", null],
  ["echo flash >/dev/vid/ctl", "echo: write error: invalid argument"],
  ["echo output ntsc mono 240p >/dev/vid/ctl; grep output /dev/vid/ctl; echo output vga >/dev/vid/ctl", "output ntsc mono 240p"],
  ["echo output vga 240p >/dev/vid/ctl; grep output /dev/vid/ctl", "echo: write error: invalid argument\noutput vga"],
  ["cat /pc/box", "\x1b(0lqk\x1b(B"],
  ["cat /pc/colours", "\x1b[31;44mR\x1b[0mn\x1b[1;32mG\x1b[0;7mV\x1b[m"],
  ["cat /pc/reverse", "\x1b[?5h", true],
];
const SCREEN_COLOURS = '\x1b[31;44mR\x1b[0mn\x1b[1;32mG\x1b[0;7mV\x1b[m\n';

// The console's VT100 (docs/design/plans/WINDOWS.md, W1): what a program writes, each into a window not shown (window 1),
// its /text read back (the vt test); and a window painted on both terminals (vtpaint).  Each expected screen is
// sim/lib/vt.js's; xterm.js's headless terminal, if it's installed (npm install, in reborn/), is held against vt.js
// too, for each fixture but those it differs on by design (false: SUB's error character, DECCOLM's clearing, which
// xterm.js leaves out)
const VT_FIXTURES = [
  ['text', 'Hello, world.\r\nsecond line\nthird\tTAB\tTAB2\x08X\r\n' + 'A'.repeat(85) + '\ndone'],
  ['moves', '\x1b[5;10Hfive-ten\x1b[2Aup2\x1b[3Bdown3\x1b[4Cright4\x1b[20Dleft20\x1b[2Ecnl\x1b[1Fcpl\x1b[30Gcha30' +
    '\x1b[12dvpa12\x1b[40`hpa40\x1b[3aR\x1b[2eE\x1b[99;75Hcorner\x1b[1;1H\x1b[0;0fhome'],
  ['erase', 'aaaaaaaaaa\r\nbbbbbbbbbb\r\ncccccccccc\r\ndddddddddd\r\neeeeeeeeee\r\nffffffffff\r\ngggggggggg' +
    '\x1b[2;5H\x1b[K\x1b[3;5H\x1b[1K\x1b[4;5H\x1b[2K\x1b[1;3H\x1b[4X\x1b[6;3H\x1b[J'],
  ['erase1', 'aaaaaaaaaa\r\nbbbbbbbbbb\r\ncccccccccc\x1b[2;4H\x1b[1Jz'],
  ['insdel', Array.from({ length: 10 }, (v, i) => 'line ' + (i + 1)).join('\r\n') +
    '\x1b[3;1H\x1b[2L\x1b[7;1H\x1b[1M\x1b[1;3H\x1b[2@\x1b[2;2H\x1b[3Pafter'],
  ['region', 'top\r\nsecond\x1b[3;8r\x1b[8;1Hr1\nr2\nr3\n\x1b[3;1H\x1bMri\x1b[5;1H\x1bDind\x1bEnel\x1b[2S\x1b[1T\x1b[rafter'],
  ['scrollback', Array.from({ length: 30 }, (v, i) => 'L' + (i + 1)).join('\n') + '\x1b[1;20r\x1b[20;1H\n\n\nin region'],
  ['tabs', '\tA\tB\x1b[3g\x1b[1;5H\x1bH\x1b[1;13H\x1bH\r\n\tC\tD\tE\x1b[1;30H\x1b[2ZF\x1b[3;1H\x1b[2IG\x1b[0gH'],
  ['wrap', 'X'.repeat(80) + '\rY\n\x1b[?7l' + 'Z'.repeat(85) + '\x1b[?7h\r\n' + 'W'.repeat(80) + 'V'],
  ['insert', 'abcdef\r\x1b[4hXY\x1b[4l\r\nghijkl\x1b[2;3H\x1b[4h12\x1b[4l'],
  ['charsets', '\x1b(0lqqqqk\r\nx    x\r\nmqqqqj\x1b(B\r\n\x1b)0A\x0eqqq\x0fB\r\n\x1b(A#1 pound\x1b(B #2'],
  ['savecursor', '\x1b[3;5Hhere\x1b7\x1b[10;10Hthere\x1b8!\x1b[s\x1b[12;1Hscosc\x1b[u?' +
    '\x1b[5;10r\x1b[?6h\x1b[1;1Horigin\x1b[2;3Hom\x1b[?6l\x1b[r'],
  ['align', 'garbage\x1b#8\x1b[12;35Hcentre'],
  ['ris', 'old text\x1b[5;10r\x1b[4h\r\nmore\x1bcnew'],
  ['rep', 'ab\x1b[5bc\r\n\x1b[1;79H' + 'x\x1b[3b'],
  ['junk', 'A\x1b[?1234hB\x1b[5xC\x1b]0;title\x07D\x1bPq#0;1\x1b\\E\x1b[1\x18F\x1b[2\x1b[1;9HG' + 'y'.repeat(240) +
    '\x1b[31mred\x1b[0m'],
  ['scrolls', Array.from({ length: 30 }, (v, i) => 'S' + (i + 1)).join('\n') + '\x1b[2S\x1b[1;1H\x1b[2Mtop'],
  ['ris2', Array.from({ length: 30 }, (v, i) => 'R' + (i + 1)).join('\n') + '\x1bcnew'],
  ['ed3', Array.from({ length: 30 }, (v, i) => 'E' + (i + 1)).join('\n') + '\x1b[3Jkept'],
  ['alt', Array.from({ length: 28 }, (v, i) => 'M' + (i + 1)).join('\n') + '\x1b[?1049halt text\x1b[5;5Hthere\n\n\n' +
    Array.from({ length: 30 }, (v, i) => 'A' + i).join('\n') + '\x1b[?1049lback'],
  ['alt47', 'main\x1b[?47hon the alternate\x1b[?47l\x1b[2;1Hmain again'],
  ['sgr', '\x1b[1;31mbold red\x1b[0m \x1b[38;5;196mx256\x1b[48;2;0;0;255mrgb\x1b[m \x1b[7mrev\x1b[27m end'],
  ['sub', 'one\x1b[2\x1athree', false],
  ['dwide', 'single\r\n\x1b#6double width, long enough to wrap past forty columns\r\n\x1b#3top\r\n\x1b#4bottom\r\nx\x1b[70G\x1b#6y\r\n' +
    '\x1b#6z\x1b#5back to single', false],
  ['vt52', 'a\x1b[?2lb\x1bAc\x1bBd\x1bY(#xy\x1bFlqk\x1bG\x1bH\x1bIrv\x1b<\x1b[3;1Hansi', false],
  ['colm', 'text\x1b[?3hafter', false],
];
const VT_PAINT = '\x1b[1;31mRed bold\x1b[0m plain \x1b[7mrev\x1b[27m\r\n\x1b(0lqqqk\x1b(B box\r\n' + 'W'.repeat(100) +
  '\r\n\x1b[44;33m blue bg \x1b[0m\r\n\x1b[10;5Hmiddle\x1b[12;1H\x1b#6wide\x1b[3;20r\x1b[15;7Hend';
// (vtpaint's steps: window 1 made, written, shown (a read of its /text a request at a time paints the serial port,
// as a reader waiting for keys would), the screen read; then window 0 shown, and the screen's rows printed)
const VT_PAINT_RC = [
  'echo new >/dev/wctl',
  '{',
  '  cat /pc/vt/paint >[1=3]',
  '  echo current 1 >/dev/wctl',
  '  for(i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16) cat \'#c1/text\' >/dev/null',
  '  cat /dev/vid/term >/ram/scr',
  '  echo current 0 >/dev/wctl',
  '} >[3]\'#c1/cons\'',
  'echo painted',
  'cat /ram/scr',
  'echo done',
].join('\n') + '\n';
// (vtsize's: 30 lines into window 1, every third longer than 60 columns, the cursor then on row 5; then its steps,
// each the bytes written first, the console's size set (terminal size: both terminals on, the screen's bigger), and a
// mark written at the cursor; its /text after each, held against sim/lib/vt.js's resize)
const VT_SIZE = Array.from({ length: 30 }, (v, i) => 'line ' + String(i + 1).padStart(2, '0') + (i % 3 ? '' : ' ' + 'abcdefghij'.repeat(7))).join('\n') + '\n\x1b[5;1H';
const VT_SIZE_STEPS = [
  ['', 80, 10, '<a>'],                                       // shorter, the cursor high: the bottom rows dropped
  ['', 80, 30, '<b>'],                                       // taller: the scrollback's rows down, then blank ones
  ['', 60, 30, '<c>'],                                       // narrower: the long lines cut
  ['', 80, 24, '<d>'],                                       // wider again (what was cut stays gone), shorter
  ['\x1b[?1049h' + Array.from({ length: 20 }, (v, i) => 'alt ' + (i + 1)).join('\n'), 80, 12, '<e>'], // the alternate screen
  ['\x1b[?1049l', 80, 12, '<f>'],                            // the main one again: its cursor where it was saved
  ['', 80, 24, '<g>'],
];
const VT_SIZE_RC = [
  'echo new >/dev/wctl',
  '{',
  '  cat /pc/vt/size >[1=3]',
  ...VT_SIZE_STEPS.flatMap(([pre, c, r, mark], i) => [
    ...(pre ? ['  cat /pc/vt/size' + i + ' >[1=3]'] : []),
    '  echo terminal size ' + c + ' ' + r + ' >/dev/consctl',
    "  echo -n '" + mark + "' >[1=3]",
    "  echo '[" + i + "]'; cat '#c1/text'; echo '[/" + i + "]'",
  ]),
  "} >[3]'#c1/cons'",
  'echo terminal size 80 24 >/dev/consctl',
  'echo done',
].join('\n') + '\n';
const vtSizeFiles = () => {
  const files = { 'vt/size': vtFile(VT_SIZE), 'vt/size.rc': VT_SIZE_RC };
  VT_SIZE_STEPS.forEach(([pre], i) => { if (pre) files['vt/size' + i] = vtFile(pre); });
  return files;
};
// (vtmode's: the screen alone, vid's mode changed under the console; its sizes, and the screen as painted at 40x30)
const VT_MODE_RC = [
  'echo screen >/dev/consctl',
  'grep size /dev/consctl >/ram/s1',
  'echo mode 40x30 >/dev/vid/ctl',
  'echo after 40x30',
  'grep size /dev/consctl >/ram/s2',
  'cat /dev/vid/term >/ram/t2',
  'echo mode 80x30 >/dev/vid/ctl',
  'echo after 80x30',
  'grep size /dev/consctl >/ram/s3',
  'echo both >/dev/consctl',
  'grep size /dev/consctl >/ram/s4',
  'cat /ram/s1 /ram/s2 /ram/s3 /ram/s4',
  "echo '[t2]'",
  'cat /ram/t2',
  "echo '[/t2]'",
  'echo mode 80x60 >/dev/vid/ctl',
  'echo done',
].join('\n') + '\n';
// (vtedit's: two lines edited in a window 20 columns wide, each longer than its row: moved over (Left across the
// rows), inserted into, deleted from, Home and End; then the first recalled (Up) and cut back; a third made 30 wide
// as it's typed (the terminal's report, typed).  edSim: what the line editor makes of keys, the lines before for Up)
const VT_EDIT = [
  'echo 0123456789abcdefghijklmnopqrstuvwxyz' + '\x1b[D'.repeat(25) + 'XY' + '\x01' + '\x1b[C'.repeat(7) + '\x1b[3~\x1b[3~' + '\x05' + '!',
  '\x1b[A' + '\x7f'.repeat(30) + ' ok',
  'echo abcdefghijklmnopqrstuvwxyz0123' + '\x1b[8;24;30t' + '\x1b[D'.repeat(3) + 'Z',
];
function edSim(keys, hist) {
  let s = '', p = 0;
  for (const k of keys.match(/\x1b\[8;24;30t|\x1b\[3~|\x1b\[[A-D]|[\s\S]/g)) {
    if (k === '\x1b[D') { if (p > 0) p--; }
    else if (k === '\x1b[C') { if (p < s.length) p++; }
    else if (k === '\x01') p = 0;
    else if (k === '\x05') p = s.length;
    else if (k === '\x7f') { if (p > 0) { s = s.slice(0, p - 1) + s.slice(p); p--; } }
    else if (k === '\x1b[3~') { if (p < s.length) s = s.slice(0, p) + s.slice(p + 1); }
    else if (k === '\x1b[A') { s = hist[hist.length - 1]; p = s.length; }
    else if (k === '\x1b[8;24;30t') { }
    else { s = s.slice(0, p) + k + s.slice(p); p++; }
  }
  return s;
}
// (winchrome's: the chrome on the screen alone, its rows read back from /dev/vid/term at each step: the bar's and
// header's (the screen's first two), the footer's (its last, or with the bar at the bottom the two last))
const CHROME_RC = [
  'echo screen >/dev/consctl',
  'echo -n mywin >/dev/label',
  'cat /dev/label >/ram/l0',
  'echo status hello there >/dev/wctl',
  'cat /dev/vid/term >/ram/s1',
  'cat /pc/vt/sasd',
  "echo 'footer %[7]%s%=%c x %r %m' >/dev/wctl",
  "echo 'header [%p] %l%=%n' >/dev/wctl",
  'cat /dev/vid/term >/ram/s2',
  'echo bar bottom >/dev/wctl',
  'cat /dev/vid/term >/ram/s3',
  'echo chrome screen off header >/dev/wctl',
  'grep size /dev/consctl >/ram/z1',
  'echo bar off >/dev/wctl',
  'grep size /dev/consctl >>/ram/z1',
  'echo chrome screen off >/dev/wctl',
  'grep size /dev/consctl >>/ram/z1',
  'echo chrome screen on >/dev/wctl',
  'echo bar top >/dev/wctl',
  'grep size /dev/consctl >>/ram/z1',
  'cat /pc/vt/title',
  'cat /dev/label >/ram/l1',
  'echo new >/dev/wctl',
  '{',
  "  echo monitor on >'#c1/wctl'",
  '  echo hidden >[1=3]',
  '  cat /dev/vid/term >/ram/s4',
  '  cat /pc/vt/bel >[1=3]',
  '  cat /dev/vid/term >/ram/s5',
  "} >[3]'#c1/cons'",
  'echo >/dev/label',
  'cat /dev/vid/term >/ram/s6',
  'echo both >/dev/consctl',
  "echo '[l]'; cat /ram/l0; echo; cat /ram/l1; echo",
  "echo '[z]'; cat /ram/z1",
  "for (n in 1 2 4 5 6) { echo '[s'^$n^']'; head -2 /ram/s^$n; tail -1 /ram/s^$n }",
  "echo '[s3]'; head -1 /ram/s3; tail -2 /ram/s3",
  'echo done',
].join('\n') + '\n';
// (vtjump's: the ROM disk's api.md, 38K, cat to the window shown with scroll jump)
const vtJump = () => fs.readFileSync(path.join(__dirname, '..', 'obj', 'gen', 'api.md'), 'latin1');
const vtModel = bytes => new VT({ onlcr: true }).write(bytes);
const vtText = bytes => vtModel(bytes).text().replace(/\n$/, '');
const vtFile = bytes => () => Buffer.from(bytes, 'latin1');
const VT_LINES = VT_FIXTURES.map(([n, b]) => ["echo new >/dev/wctl; {cat /pc/vt/" + n + " >[1=3]; cat '#c1/text'} >[3]'#c1/cons'", vtText(b)]);
// xterm.js's headless terminal's screen for bytes (as a window's /text: its scrollback, then its screen; the DEC
// graphics as the console's ASCII), or null if it isn't installed
const XTERM_DEC = '◆▒␉␌␍␊°±␤␋┘┐┌└┼' +
  '⎺⎻─⎼⎽├┤┴┬│≤≥π≠£·';
function xtermText(bytes) {
  let Terminal;
  try { ({ Terminal } = require('@xterm/headless')); } catch (e) { return null; }
  const t = new Terminal({ cols: 80, rows: 24, scrollback: 40, convertEol: true, allowProposedApi: true });
  const warn = console.warn, log = console.log;
  console.warn = console.log = () => {};                    // (writeSync's warning: it's a test's, not a program's)
  try { t._core.writeSync(bytes); } finally { console.warn = warn; console.log = log; }
  const b = t.buffer.active, lines = [];
  for (let i = 0; i < b.length; i++) {
    lines.push([...b.getLine(i).translateToString(true)].map(ch => {
      const k = XTERM_DEC.indexOf(ch);
      return k >= 0 && ch !== '£' && ch !== '°' && ch !== '±' && ch !== '·' ? DEC_ASCII[k + 1] : ch;
    }).join('').replace(/ +$/, ''));
  }
  t.dispose();
  return lines.join('\n');
}
// The vt test's check: vt.js held against xterm.js (if it's there), fixture by fixture
function vtXterm() {
  const f = [];
  let checked = 0;
  for (const [n, b, xt] of VT_FIXTURES) {
    if (xt === false) continue;
    const x = xtermText(b);
    if (x === null) return { f, note: 'xterm.js not installed (npm install in reborn/): vt.js not checked against it' };
    checked++;
    const v = vtText(b);
    if (x !== v) {
      const xl = x.split('\n'), vl = v.split('\n');
      const i = xl.findIndex((l, k) => l !== vl[k]);
      f.push('vt.js and xterm.js differ on ' + n + ', line ' + (i + 1) + ': ' + JSON.stringify(vl[i]) + ' and ' + JSON.stringify(xl[i]));
    }
  }
  return { f, note: 'vt.js held against xterm.js: ' + checked + ' fixtures' };
}

// The sound test's lines (as the tools test's): #a's files, the volume, claims (one another program holds), its
// errors, the shadow, the C sample tones, and the bell (no Vera X: 8 channels, the PSG's E_NODEV)
const SND_LINES = [
  ["ls /dev | grep snd; cat /dev/sndctl","snd\nsndctl\nvolume 100\nchannels 8\nclaimed"],
  ["echo volume 150 >/dev/sndctl; cat /dev/sndctl; echo volume 100 >/dev/sndctl","volume 150\nchannels 8\nclaimed"],
  ["{echo claim 5 >[1=3]; cat /dev/sndctl} >[3]/dev/sndctl; cat /dev/sndctl","volume 100\nchannels 8\nclaimed 0 2\nvolume 100\nchannels 8\nclaimed"],
  ["echo frob >/dev/sndctl; echo claim >/dev/sndctl", [
    "echo: write error: invalid argument",
    "echo: write error: invalid argument",
  ].join('\n')],
  ["wc -c /dev/snd","    256 /dev/snd"],
  ["/sd/0/sample/c/tones 0 & sleep 1; echo claim 1 >/dev/sndctl; echo note 0 60 >/dev/sndctl; wait",
    "echo: write error: busy\necho: write error: busy\ntones: patch 0, $20 C4, $28 4C, C#4 61, the tune played, 1000 Hz 5D, mml 0, chord 0"],
  ["echo x >/dev/bell",null],
  // (The channel commands as text: channel 4 a note bent down half a semitone, on the left; 5 a drum on the right;
  // two registers of 6's; their effects on the chip checked in check)
  ["echo patch 4 0 >/dev/sndctl; echo note 4 69 >/dev/sndctl; echo level 4 100 >/dev/sndctl; echo vol 4 90 >/dev/sndctl",null],
  ["echo pan 4 left >/dev/sndctl; echo bend 4 -32 >/dev/sndctl; echo off 4 >/dev/sndctl",null],
  ["echo pan 5 2 >/dev/sndctl; echo drum 5 38 >/dev/sndctl; echo reg 46 74 54 252 >/dev/sndctl",null],
  ["echo note 24 60 >/dev/sndctl; echo patch 0 163 >/dev/sndctl; echo pan 0 up >/dev/sndctl; echo bend 0 128 >/dev/sndctl",
    Array(4).fill('echo: write error: invalid argument').join('\n')],
  ["echo note 8 60 >/dev/sndctl; echo wave 23 saw >/dev/sndctl", Array(2).fill('echo: write error: no such device').join('\n')],
  ["echo bend 0 -129 >/dev/sndctl; echo note 0 >/dev/sndctl; echo reg 46 >/dev/sndctl; echo reg 46 256 >/dev/sndctl",
    Array(4).fill('echo: write error: invalid argument').join('\n')],
  // (Step 3's: a frequency (1000 Hz: B5 and 13 64ths), a glide (C5, no attack), the LFO, a channel's sensitivity
  // to it, the noise; checked in check)
  ["echo freq 2 1000 >/dev/sndctl; echo freq 3 440 >/dev/sndctl; echo glide 3 72 >/dev/sndctl",null],
  ["echo lfo 200 10 20 2 >/dev/sndctl; echo sens 3 5 2 >/dev/sndctl; echo noise 7 >/dev/sndctl",null],
  ["echo noise off >/dev/sndctl; echo noise 9 >/dev/sndctl",null],
  ["echo freq 99 440 >/dev/sndctl; echo lfo 1 2 3 >/dev/sndctl; echo lfo 1 128 0 0 >/dev/sndctl",
    Array(3).fill('echo: write error: invalid argument').join('\n')],
  ["echo sens 0 8 0 >/dev/sndctl; echo noise 32 >/dev/sndctl; echo noise loud >/dev/sndctl",
    Array(3).fill('echo: write error: invalid argument').join('\n')],
];

// The PSG test's lines (with a Vera X: channels 8-23): the state; notes, a waveform, speakers, a level, a frequency,
// a bend, a note off (their registers on the chip checked in check); claims of the PSG's channels (given back
// keyed off: 8's note ends); the master volume on them; errors; /psg and vid's; a note played while the VERA's
// claimed (only kept), on the chip when the claim ends (the claimer's task in the state, then none); hylang's and HyForth's words (snd-wave; claims of the
// PSG's channels: hylang's by number, HyForth's snd-claim-psg); a song of the PSG's alone (play: its PSG writes,
// its PSG channels claimed while it plays)
const PSG_LINES = [
  ["ls /dev | grep psg; ls /dev/vid | grep psg; cat /dev/sndctl", "psg\npsg\nvolume 100\nchannels 24\nclaimed"],
  ["echo note 8 69 >/dev/sndctl; echo wave 9 saw >/dev/sndctl; echo note 9 60 >/dev/sndctl", null],
  ["echo pan 10 left >/dev/sndctl; echo level 10 64 >/dev/sndctl; echo note 10 72 >/dev/sndctl", null],
  ["echo freq 11 1000 >/dev/sndctl; echo wave 12 2 31 >/dev/sndctl; echo patch 13 3 >/dev/sndctl", null],
  ["echo bend 8 -64 >/dev/sndctl; echo off 9 >/dev/sndctl", null],
  ["{echo claim 0 3 >[1=3]; cat /dev/sndctl} >[3]/dev/sndctl; cat /dev/sndctl",
    "volume 100\nchannels 24\nclaimed 8 9\nvolume 100\nchannels 24\nclaimed"],
  ["{echo claim 1 32768 >[1=3]; cat /dev/sndctl} >[3]/dev/sndctl", "volume 100\nchannels 24\nclaimed 0 23"],
  ["echo wave 3 pulse >/dev/sndctl; echo wave 8 pulse 64 >/dev/sndctl", Array(2).fill('echo: write error: invalid argument').join('\n')],
  ["echo wave 8 square >/dev/sndctl; echo sens 8 1 1 >/dev/sndctl", Array(2).fill('echo: write error: invalid argument').join('\n')],
  ["echo volume 50 >/dev/sndctl", null],
  ["wc -c /dev/psg /dev/vid/psg", "     64 /dev/psg\n     64 /dev/vid/psg\n    128 total"],
  ["echo '(do (use \"snd\") (snd-wave 15 :triangle 20) (snd-note 15 69))' | hylang >/dev/null", null],
  ["echo 'lib sound 16 1 40 snd-wave 16 60 snd-note' | forth >/dev/null", null],
  ["echo '(do (use \"snd\") (snd-claim 0 17 23) (run \"cat\" \"/dev/sndctl\"))' | hylang | grep claimed", "claimed 0 17 23"],
  ["echo 'lib sound lib hydra 3 snd-claim-psg s\" cat /dev/sndctl\" sh drop' | forth | grep claimed", "claimed 8 9"],
  ["{echo claim >[1=3]; echo note 14 69 >/dev/sndctl; grep claimed /dev/vid/ctl} >[3]/dev/vid/ctl", null, true],
  ["grep claimed /dev/vid/ctl", "claimed"],
  ["play /sd/0/p.zsm & sleep 1; cat /dev/sndctl; wait", "volume 50\nchannels 24\nclaimed 8 9"],
];

// The psg test's song (PSG_SONG: a ZSM of the PSG's writes alone, its voices 0 and 1 claimed): A4 on voice 0, C4 on
// a sawtooth on voice 1 half a second later (30 ticks at 60 Hz), voice 0 off half a second after that, then two
// seconds more; and its card
function PSG_SONG() {
  const hdr = [0x7A, 0x6D, 1, 0, 0, 0, 0, 0, 0, 0x00, 0x03, 0x00, 60, 0, 0, 0];
  return Buffer.from([...hdr, 0, 0x9D, 1, 0x04, 3, 0x3F, 2, 0xFF, 0x80 | 30, 4, 0xBE, 5, 0x02, 7, 0x7F, 6, 0xFF, 0x80 | 30, 2, 0xC0,
    0x80 | 120, 0x80]);
}
function psgCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'psg0.img');
  hydrafs.mkfs(f, 8, 'SONGS', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('p.zsm', PSG_SONG());
  v.close();
  return [imageCard(0, f, 16384)];
}

// The ramw test's lines, at the login shell (HyForth): a file of 70 writes of 10 bytes, then written over in its
// first block, across its first block's end and at its end (one byte more after it), on the RAM disk, the shared one
// and a card, each read back; then 100 writes of 16 bytes to /ram, and to a card, timed (the file made before them and
// closed after; the marks [rw A] and [rw B] made by emit and .(, so the line typed isn't one); then two files on the
// card left open, each written twice (the first write allocating: its data goes to the card before the map does), the
// one's second write synced, the other's left kept back (nothing after it reaches the storage driver); and its card
// (blank HydraFS)
const RAMW_LINES = ['variable fd', ': w ( a u -- ) w/o create-file throw fd ! ;', ': p ( a u -- ) fd @ write-file throw ;',
  ': at ( n -- ) s>d fd @ reposition-file throw ;', ': c fd @ close-file throw ;',
  ': t ( a u -- ) w 70 0 do s" 0123456789" p loop 5 at s" abc" p 508 at s" XYZWV" p 699 at s" !+" p c ;',
  ': n ( -- ) 100 0 do s" 0123456789abcdef" p loop ;',
  's" /ram/w" t', 's" /sram/w" t', 's" /sd/0/w" t', 'cat /ram/w; echo; cat /sram/w; echo; cat /sd/0/w; echo',
  ': h ( a u -- ) w s" ab" p 3000 at s" Z" p c ;', 's" /ram/h" h s" /sram/h" h s" /sd/0/h" h', 'ls -l /ram/h /sram/h /sd/0/h',
  's" /ram/n" w', '91 emit .( b0 A])', '91 emit .( b0 B])', '91 emit .( rw A])', 'n', '91 emit .( rw B])', 'c',
  's" /sd/0/n" w', '91 emit .( cw A])', 'n', '91 emit .( cw B])', 'c', 'ls -l /ram/n /sd/0/n',
  's" /sd/0/k" w s" first part, written. " p s" then synced" p', 'echo sync >/dev/sd/0/ctl',
  's" /sd/0/u" w s" first part, written. " p s" second part: kept back" p', '91 emit .( ramw done.)'];
const RAMW_TEXT = (() => { const b = [...'0123456789'.repeat(70)]; b.splice(5, 3, ...'abc'); b.splice(508, 5, ...'XYZWV'); b.splice(699, 1, '!', '+'); return b.join(''); })();
function ramwCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const img = path.join(CARD_DIR, 'ramw.img');
  fs.rmSync(img, { force: true });
  hydrafs.mkfs(img, 8, 'RAMW', undefined, true);
  return imageCard(0, img, Math.floor(fs.statSync(img).size / 512));
}

// The psgmml test's card: scores on the PSG's channels (I-X) and hysong.js's ZSMs of them: the ROM disk's
// songs/vera.mml (both chips, each waveform and envelope) as v.mml and vpc.zsm, tests/scores/edges.mml and edges2.mml
// as e and f; and scores that are wrong (an instrument on the other chip's channel, both ways; x on the PSG; y past
// its registers; three instruments it can't read; a note before an instrument on channel 23)
const PSG_SCORES = { v: path.join(__dirname, '..', 'sdcard', 'songs', 'vera.mml'), e: path.join(__dirname, 'scores', 'edges.mml'), f: path.join(__dirname, 'scores', 'edges2.mml') };
const PSG_BAD = ['@w { wave saw }\nA @w c\n', '@g { gm 0 }\nI @g c\n', '@w { wave saw }\nI @w x36\n', '@w { wave saw }\nJ @w y 64,1 c\n',
  '@w { wave square }\nI @w c\n', '@w { env 1 2 64 3 }\nI @w c\n', '@w { wave saw alg 3 }\nI @w c\n', '@w { wave saw }\nX c\n'];
function psgScoreCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'psgmml0.img');
  fs.rmSync(f, { force: true });
  hydrafs.mkfs(f, 8, 'SCORES', undefined, true);
  const v = new hydrafs.Volume(f);
  for (const [n, score] of Object.entries(PSG_SCORES)) {
    const zsm = path.join(CARD_DIR, 'psgmml-' + n + '.zsm');
    require('child_process').execFileSync(process.execPath, [path.join(__dirname, '..', 'sim', 'tools', 'hysong.js'), score, zsm, '--quiet']);
    v.put(n + '.mml', fs.readFileSync(score));
    v.put(n + 'pc.zsm', fs.readFileSync(zsm));
  }
  PSG_BAD.forEach((t, i) => v.put('b' + (i + 1) + '.mml', Buffer.from(t)));
  v.close();
  return [imageCard(0, f, 16384)];
}
const PSG_MML_LINES = [
  ['cd /sd/0; play -o v.mml /ram/v.zsm; cmp /ram/v.zsm vpc.zsm && echo same', 'same'],
  ['play -o e.mml /ram/e.zsm; cmp /ram/e.zsm epc.zsm && echo same', 'same'],
  ['play -o f.mml /ram/f.zsm; cmp /ram/f.zsm fpc.zsm && echo same', 'same'],
  ["echo '[b0' 'A]'", '[b0 A]'], ["echo '[b0' 'B]'", '[b0 B]'], ["echo '[po' 'A]'", '[po A]'], ['play -o v.mml /ram/p.zsm', null],
  ["echo '[po' 'B]'", '[po B]'],
  ['play b1.mml; play b2.mml; play b3.mml; play b4.mml', 'play: b1.mml: channel 0: the other chip\'s instrument\nplay: b2.mml: channel 8: the other chip\'s instrument\nplay: b3.mml: channel 8: the YM2151\'s only\nplay: b4.mml: channel 9: a PSG register is 0-63'],
  ['play b5.mml; play b6.mml; play b7.mml; play b8.mml', 'play: b5.mml: an instrument it can\'t read\nplay: b6.mml: an instrument it can\'t read\nplay: b7.mml: an instrument it can\'t read\nplay: b8.mml: channel 23: a note before an instrument'],
  ['echo reset >/dev/sndctl; play v.mml; echo played', 'played'],
  ['play -m 8 o4 l8 c d e; play -x -m 9 I128 V40 O5 L4 C; play -c 13 o4 l4 c e g; play -c 21 c d e f', 'play: c: more notes than channels'],
];
// A ZSM's PSG voices' starts (a volume from 0 to more, a speaker on: vera.js's psgOns), each voice's count and its
// first one's tick
function zsmPsgOns(b) {
  const vol = new Array(16).fill(0), n = new Array(16).fill(0), first = new Array(16).fill(-1);
  let i = 16, t = 0;
  while (i < b.length) {
    const c = b[i++];
    if (c < 0x40) {
      const v = b[i++];
      if ((c & 3) === 2) {
        const k = c >> 2;
        if (!(vol[k] & 0x3F) && (v & 0x3F) && (v & 0xC0)) { n[k]++; if (first[k] < 0) first[k] = t; }
        vol[k] = v;
      }
    } else if (c === 0x40) i += b[i++] & 0x3F;
    else if (c < 0x80) i += 2 * (c & 0x3F);
    else if (c === 0x80) break;
    else t += c & 0x7F;
  }
  return { n, first };
}

// The PCM test's lines (with a Vera X: vid's /pcm and /pcmctl): its files and state; the rate (the VERA's nearest)
// and volume; raw samples from a card, drained; bad commands; /pcm one task's (another's pcmctl command: busy);
// WAV files played (8 bits mono, made signed; 16 bits stereo, past a chunk of an odd size; a float one, not a song);
// a ZSM's PCM instruments (one, then a looped one, stopped by the FIFO emptied: from RAM), its claim of the PCM as
// it plays, the state it leaves; one too big for RAM (from the file), stopped after half a second.  (A redirection is
// its rc's: another task's command comes through rc -c.)  The FIFO's bytes, and its runs dry, checked in check
const PCM_LINES = [
  ["ls /dev/vid | grep pcm; cat /dev/vid/pcmctl", "pcm\npcmctl\nrate 0\nbits 8\nmono\nvolume 15\nclaimed"],
  ["echo rate 22050 >/dev/vid/pcmctl; echo volume 12 >/dev/vid/pcmctl; grep -v claimed /dev/vid/pcmctl", "rate 22126\nbits 8\nmono\nvolume 12"],
  ["echo rate 3800 >/dev/vid/pcmctl; grep rate /dev/vid/pcmctl", "rate 3815"],
  ["cat /sd/0/tone.raw >/dev/vid/pcm; echo drain >/dev/vid/pcmctl; echo drained", "drained"],
  ["echo bits 12 >/dev/vid/pcmctl; echo volume 16 >/dev/vid/pcmctl; echo rate fast >/dev/vid/pcmctl",
    Array(3).fill('echo: write error: invalid argument').join('\n')],
  ["{rc -c 'echo bits 16 >/dev/vid/pcmctl'; grep -c 'claimed [0-9]' /dev/vid/pcmctl} >[3]/dev/vid/pcm; grep claimed /dev/vid/pcmctl",
    "echo: write error: busy\n1\nclaimed"],
  ["play /sd/0/t.wav; echo $status", ""],
  ["play /sd/0/s.wav; grep bits /dev/vid/pcmctl", "bits 16"],
  ["play /sd/0/f.wav", "play: /sd/0/f.wav: not a song"],
  ["play /sd/0/p.zsm & sleep 1; grep -c 'claimed [0-9]' /dev/vid/pcmctl; wait", "1"],
  ["cat /dev/vid/pcmctl", "rate 3815\nbits 8\nmono\nvolume 10\nclaimed"],
  ["play /sd/0/q.zsm; grep -c claimed /dev/vid/pcmctl", "1"],
];

// The pcm test's samples (bytes a generator's, so the parts can't be mistaken for each other), its WAV files and its
// ZSMs: p.zsm two PCM instruments (8 bits mono), the second looped from 100; the stream sets the rate (AUDIO_RATE 10:
// 3,815 Hz) and volume (10), starts the first, 30 ticks on the second, 60 ticks on empties the FIFO (its volume 10
// again).  q.zsm one of 17,000 bytes (past play's RAM for them, 16K), started, 30 ticks on stopped
const pcmBytes = (n, seed) => { const b = Buffer.alloc(n); let x = seed >>> 0; for (let i = 0; i < n; i++) { x = (Math.imul(x, 1103515245) + 12345) >>> 0; b[i] = x >>> 24; } return b; };
const PCM_DATA = { tone: pcmBytes(6000, 1), t: pcmBytes(4000, 2), s: pcmBytes(2400, 3), i0: pcmBytes(1500, 4), i1: pcmBytes(600, 5),
  big: pcmBytes(17000, 6) };
function wavFile(rate, channels, bits, data, opt = {}) {
  const fmt = Buffer.alloc(16);
  fmt.writeUInt16LE(opt.tag || 1, 0); fmt.writeUInt16LE(channels, 2); fmt.writeUInt32LE(rate, 4);
  fmt.writeUInt32LE(rate * channels * bits / 8, 8); fmt.writeUInt16LE(channels * bits / 8, 12); fmt.writeUInt16LE(bits, 14);
  const chunk = (id, b) => { const h = Buffer.alloc(8); h.write(id, 0, 'latin1'); h.writeUInt32LE(b.length, 4); return Buffer.concat([h, b, Buffer.alloc(b.length & 1)]); };
  const body = Buffer.concat([Buffer.from('WAVE', 'latin1'), chunk('fmt ', fmt), ...(opt.list ? [chunk('LIST', Buffer.from('hydra', 'latin1'))] : []), chunk('data', data)]);
  const h = Buffer.alloc(8); h.write('RIFF', 0, 'latin1'); h.writeUInt32LE(body.length, 4);
  return Buffer.concat([h, body]);
}
function pcmSong(stream, insts, data) {
  const at = 16 + stream.length;
  const hdr = [0x7A, 0x6D, 1, 0, 0, 0, at & 255, at >> 8, 0, 0, 0, 0, 60, 0, 0, 0];
  const inst = ([idx, off, len, loop, lp]) => {
    const b = Buffer.alloc(16); b[0] = idx; b.writeUIntLE(off, 2, 3); b.writeUIntLE(len, 5, 3); b[8] = loop ? 0x80 : 0; b.writeUIntLE(lp, 9, 3); return b;
  };
  return Buffer.concat([Buffer.from(hdr), Buffer.from(stream), Buffer.from('PCM', 'latin1'), Buffer.from([insts.length - 1]), ...insts.map(inst), ...data]);
}
function PCM_SONG() {
  const { i0, i1 } = PCM_DATA;
  return pcmSong([0x40, 0x04, 1, 10, 0, 10, 0x40, 0x02, 2, 0, 0x80 | 30, 0x40, 0x02, 2, 1, 0x80 | 60, 0x40, 0x02, 0, 0x8A, 0x80 | 10, 0x80],
    [[0, 0, i0.length, false, 0], [1, i0.length, i1.length, true, 100]], [i0, i1]);
}
function PCM_BIG() {
  return pcmSong([0x40, 0x04, 1, 10, 2, 0, 0x80 | 30, 0x40, 0x02, 0, 0x8A, 0x80], [[0, 0, PCM_DATA.big.length, false, 0]], [PCM_DATA.big]);
}
function pcmCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'pcm0.img');
  hydrafs.mkfs(f, 8, 'PCM', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('tone.raw', PCM_DATA.tone);
  v.put('t.wav', wavFile(3819, 1, 8, PCM_DATA.t));
  v.put('s.wav', wavFile(1144, 2, 16, PCM_DATA.s, { list: true }));
  v.put('f.wav', wavFile(3819, 1, 8, PCM_DATA.t, { tag: 3 }));
  v.put('p.zsm', PCM_SONG());
  v.put('q.zsm', PCM_BIG());
  v.close();
  return [imageCard(0, f, 16384)];
}

// The song player's test lines (as the tools test's): play's errors; a file run by its name that isn't a program
// (no #!); a song from a card (PLAY_SONG, at 60 Hz: its key-ons timed in check) with its channels claimed while it
// plays and given back when it's stopped; scom, a song that runs by its name (an rc script); the C sample jukebox
// (snd_play)
const PLAY_LINES = [
  ["play; echo $status","usage: play [-l] song [n]; play -o score.mml song.zsm; play [-lx] -m|-c ch mml\nusage"],
  ["/rom/README; whatis scom","rc: /rom/README: not a program\n/bin/scom"],
  ["play /rom/README; echo $status","play: /rom/README: not a song\nnot a song"],
  ["play /rom/nosuch; echo $status","play: /rom/nosuch: not found\n1"],
  ["play /sd/0/t.zsm & sleep 4; cat /dev/sndctl; kill $apid; wait; cat /dev/sndctl", [
    "volume 100",
    "channels 8",
    "claimed 0 1 2 3 4 5",
    "volume 100",
    "channels 8",
    "claimed",
  ].join('\n')],
  ["scom & sleep 1; cat /dev/sndctl; slay play; wait; cat /dev/sndctl","volume 100\nchannels 8\nclaimed 0 1\nvolume 100\nchannels 8\nclaimed"],
  ["/sd/0/sample/c/jukebox /rom/songs/scom.zsm 2","2\n1\nstopped: 137"],
];
// The play test's song: 60 Hz, six FM channels (a voice each, set up in tick 0), then a note every 2 ticks, round
// the channels, 240 of them (as dense as the X16's tunes: songs come from cards); and its card, SD device 0
function PLAY_SONG() {
  const fm = pairs => {                                       // (63 pairs a command at most)
    const out = [];
    for (let i = 0; i < pairs.length; i += 126) out.push(0x40 | pairs.slice(i, i + 126).length / 2, ...pairs.slice(i, i + 126));
    return out;
  };
  const voices = [];
  for (let ch = 0; ch < 6; ch++) {
    voices.push(0x20 + ch, 0xC7, 0x38 + ch, 0x00);
    for (const op of [0x00, 0x08, 0x10, 0x18]) voices.push(0x40 + op + ch, 0x01, 0x60 + op + ch, 0x10, 0x80 + op + ch, 0x1F, 0xA0 + op + ch, 0x00,
      0xC0 + op + ch, 0x00, 0xE0 + op + ch, 0x0F);
  }
  const notes = [];
  for (let k = 0; k < 240; k++) { const ch = k % 6; notes.push(...fm([0x08, ch, 0x28 + ch, 0x30 + k % 12, 0x08, 0x78 | ch]), 0x82); }
  const hdr = [0x7A, 0x6D, 1, 0, 0, 0, 0, 0, 0, 0x3F, 0, 0, 60, 0, 0, 0];
  return Buffer.from([...hdr, ...fm(voices), 0x81, ...notes, 0x80]);
}
function playCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'play0.img');
  hydrafs.mkfs(f, 8, 'SONGS', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('t.zsm', PLAY_SONG());
  v.mkdir('sample/c');                                        // (jukebox, as the SD card has it: bin/sdcard.img's)
  v.put('sample/c/jukebox', fs.readFileSync(path.join(__dirname, '..', 'obj', 'samples', 'c', 'jukebox.hyx')));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The mml test's card: scores (play's: modules/play/mml.inc, hysong.js's language) and hysong.js's ZSMs of them (the
// PC's compiler, sim/tools/hysong.js): the old system's test song (os_rom/songs/test.mml: all 8 channels and
// algorithms, the LFO's 4 waveforms, noise, slides, legato, drums, repeats, the timers) as t.mml and tpc.zsm, the
// riff scom (programs/songs) as s.mml and spc.zsm; and two that are wrong (a note before an instrument, a line
// that isn't one)
const MML_SCORES = { t: path.join(__dirname, '..', '..', 'old', 'os_rom', 'songs', 'test.mml'), s: path.join(__dirname, '..', '..', 'old', 'programs', 'songs', 'scom.mml') };
function mmlCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'mml0.img');
  fs.rmSync(f, { force: true });
  hydrafs.mkfs(f, 8, 'SCORES', undefined, true);
  const v = new hydrafs.Volume(f);
  for (const [n, score] of Object.entries(MML_SCORES)) {
    const zsm = path.join(CARD_DIR, 'mml-' + n + '.zsm');
    require('child_process').execFileSync(process.execPath, [path.join(__dirname, '..', 'sim', 'tools', 'hysong.js'), score, zsm, '--quiet']);
    v.put(n + '.mml', fs.readFileSync(score));
    v.put(n + 'pc.zsm', fs.readFileSync(zsm));
  }
  v.put('bad.mml', Buffer.from('#tempo 100\nA o4 c d e\n'));
  v.put('bad2.mml', Buffer.from('@p { gm 0 }\nA @p c\nZ c d e\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The GPIO and I2C test's lines (as the tools test's): CA1's edges (the machine's pulses, at cycles 30 and 40
// million: the first line is waiting for them by then), the files, the pins' levels (the machine's: $A5, PA0 and
// PA1 the I2C bus's, high), outputs and the ctl commands, their errors; the I2C bus (a 256-byte memory at $50, a
// 16-byte one at $68): its devices listed, a write and a read through a register address, a device that isn't there
const GPIO_LINES = [
  ["head -1 /dev/gpio/ca1; head -1 /dev/gpio/ca1","1\n2"],
  ["ls /dev/gpio; ls /dev/i2c","0\n1\n2\n3\n4\n5\n6\n7\nport\nctl\nca1\nctl\n50\n68"],
  ["cat /dev/gpio/2 /dev/gpio/3; xd /dev/gpio/port","1\n0\n0000000  a7                                               ."],
  ["echo 1 >/dev/gpio/4; echo out 6 >/dev/gpio/ctl; echo ca2 1 >/dev/gpio/ctl; echo ca1 rise >/dev/gpio/ctl; cat /dev/gpio/ctl", [
    "0 in 1",
    "1 in 1",
    "2 in 1",
    "3 in 0",
    "4 out 1",
    "5 in 1",
    "6 out 0",
    "7 in 1",
    "ca1 rise 2",
    "ca2 1",
  ].join('\n')],
  ["echo ddr 0 >/dev/gpio/ctl; echo frob >/dev/gpio/ctl; echo in 9 >/dev/gpio/ctl; echo 2 >/dev/gpio/5; grep out /dev/gpio/ctl", [
    "echo: write error: invalid argument",
    "echo: write error: invalid argument",
    "echo: write error: invalid argument",
  ].join('\n')],
  ["echo subaddress 1 >/dev/i2c/ctl; echo hello >/dev/i2c/50; head -1 /dev/i2c/50; cat /dev/i2c/ctl","hello\nspeed 40\nsubaddress 1"],
  ["echo x >/dev/i2c/51; echo speed 0 >/dev/i2c/ctl; echo speed 100 >/dev/i2c/ctl; grep speed /dev/i2c/ctl", [
    "echo: write error: i/o error",
    "echo: write error: invalid argument",
    "speed 100",
  ].join('\n')],
];

// The clock test's lines (as the tools test's; a third element true: how its output starts): a DS1747 set to
// 2026-10-03 15:04:05 (the machine's), the clock from it; the time set across the end of a month (2030: no leap
// year), of February in 2100 (no leap year either) and a leap day; the time's errors; a file's stamp
const CLOCK_LINES = [
  ["cat /dev/rtc; date","running\n2026-10-03 15:04:",true],
  ["echo 2030-02-28 23:59:58 >/dev/time; sleep 3; date","2030-03-01 00:00:0",true],
  ["echo 2100-02-28 23:59:59 >/dev/time; sleep 1; date","2100-03-01 00:00:0",true],
  ["echo 2023-02-29 12:00:00 >/dev/time; echo junk >/dev/time; date x", [
    "echo: write error: invalid argument",
    "echo: write error: invalid argument",
    "usage: date [-n]",
  ].join('\n')],
  ["echo 2024-02-29 12:00:00 >/dev/time; date -n; touch /ram/f; ls -l /ram/f","76252320",true],
];

// A ZSM song's key-ons (a file, or its bytes): { rate, ticks: [the song tick of each] }
function ZSM_KEYONS(song) {
  const b = Buffer.isBuffer(song) ? song : fs.readFileSync(song), ticks = [];
  let i = 16, tick = 0;
  while (i < b.length) {
    const c = b[i++];
    if (c < 0x40) i++;
    else if (c === 0x40) i += 1 + (b[i] & 0x3F);
    else if (c < 0x80) { for (let k = 0; k < (c & 0x3F); k++, i += 2) if (b[i] === 8 && (b[i + 1] & 0x78)) ticks.push(tick); }
    else if (c === 0x80) break;
    else tick += c & 0x7F;
  }
  return { rate: (b[12] | b[13] << 8) || 60, ticks };
}

// The YM2151's key-ons in a machine's log (machine: { ymLog: true }), each its channel and its speakers as it came
// on (the register $20 + ch's RL bits): '4R', '5B', '6L' (B both, - none) in order
function YM_KEYPANS(m) {
  const r = new Uint8Array(256), ons = [];
  for (const [, reg, v] of m.ym.writes) {
    r[reg] = v;
    if (reg === 0x08 && (v & 0x78)) ons.push((v & 7) + '-LRB'[r[0x20 + (v & 7)] >> 6]);
  }
  return ons;
}

// The /pc tests' folder (the emulator as the PC tool: sim/lib/pchost.js), and the pc test's lines (as the tools
// test's): a listing, a file read, a file made by a redirect, a copy to the PC compared (its 512-byte writes go in
// frames of 128 bytes: the rest of each in the same request block, a fid other than the first's, as another /pc
// file is open), a directory made, a rename, a program run from the PC (by its path, and by its name through a
// bind), cd into it and a relative name, a directory and a file removed, missing names, the folder's files
const PC_FILES = () => ({ 'hello.txt': 'Hello from the PC\nline two\n', 'sub/x': 'x', 'sub/deep/': '',
  'bin/hi': fs.readFileSync(path.join(__dirname, '..', 'obj', 'samples', 'hi.hyx')) });
const PC_LINES = [
  ["ls /pc","bin/\nhello.txt\nsub/"],
  ["cat /pc/hello.txt","Hello from the PC\nline two"],
  ["ls -l /pc/sub","d-rw-rw-rw- P-        0 2026-10-03 15:04 deep\n--rw-rw-rw- P-        1 2026-10-03 15:04 x"],
  ["echo hi >/pc/new.txt; cat /pc/new.txt","hi"],
  ["{cp /rom/README /pc/r} </pc/hello.txt; cmp /rom/README /pc/r; echo $status",""],
  ["mkdir /pc/d; mv /pc/r /pc/rr; ls /pc","bin/\nd/\nhello.txt\nnew.txt\nrr\nsub/"],
  ["/pc/bin/hi Ann","Hello, Ann!",true],
  ["bind -a /pc/bin /bin; hi Bob","Hello, Bob!",true],
  ["cd /pc/sub; pwd; cat ../hello.txt | wc -l; cd","/pc/sub\n      2"],
  ["rmdir /pc/d; rm /pc/sub/x; cat /pc/nope; rm /pc/nope","cat: /pc/nope: not found\nrm: /pc/nope: not found"],
  ["ls /pc /pc/sub","bin/\nhello.txt\nnew.txt\nrr\nsub/\ndeep/"],
];
// The pc-img test's lines: /pc a HydraFS image on the PC, not a folder (the card's own order in a directory, and
// its stamps: the test's, for what's written too)
const PC_IMG_LINES = [
  ["echo b115200 >/dev/serctl",null],
  ["ls /pc","big\nhello.txt\nsub/\nbin/"],
  ["cat /pc/hello.txt","Hello from the PC\nline two"],
  ["ls -l /pc/sub","--rw-rw-rw- P-        1 2026-10-03 15:04 x\nd-rw-rw-rw- P-        0 2026-10-03 15:04 deep"],
  ["echo hi >/pc/new.txt; cat /pc/new.txt; ls -l /pc/new.txt","hi\n--rw-rw-rw- P-        3 2026-10-03 15:04 /pc/new.txt"],
  ["{cp /rom/README /pc/r} </pc/hello.txt; cmp /rom/README /pc/r; echo $status",""],
  ["cp /pc/big /pc/copy; cmp /pc/big /pc/copy; echo $status",""],
  ["mkdir /pc/d; mv /pc/r /pc/rr; ls /pc","big\nhello.txt\nsub/\nbin/\nnew.txt\nrr\ncopy\nd/"],
  ["/pc/bin/hi Ann","Hello, Ann!",true],
  ["echo shorter >/pc/copy; cat /pc/copy; ls -l /pc/copy","shorter\n--rw-rw-rw- P-        8 2026-10-03 15:04 /pc/copy"],
  ["echo more >>/pc/hello.txt; cat /pc/hello.txt | wc -l","      3"],
  ["mkdir /pc/d; echo no >/pc/sub; cat /pc/hello.txt/x","mkdir: /pc/d: already exists\nrc: /pc/sub: is a directory\ncat: /pc/hello.txt/x: not a directory"],
  ["rmdir /pc/sub; rmdir /pc/d; rm /pc/sub/x; cat /pc/nope; rm /pc/nope","rmdir: /pc/sub: directory not empty\ncat: /pc/nope: not found\nrm: /pc/nope: not found"],
  ["ls /pc /pc/sub","big\nhello.txt\nsub/\nbin/\nnew.txt\nrr\ncopy\ndeep/"],
];
// hydrafs.js's Volume, what serving an image needs of it beyond what the Hydra's commands ask in pc-img: a file's
// length set (longer: zeros; cut: its clusters given back, mid-cluster and to nothing), a write into a hole (the
// hole's part gets clusters, the rest stays one), a rename (and one refused: the name taken), the read-only bit;
// its errors' codes; and the free map right after all of it.  OUT: what's wrong
function hfsVolumeChecks(dir) {
  const f = [], img = path.join(dir, 'vol.img');
  fs.mkdirSync(dir, { recursive: true });
  hydrafs.mkfs(img, 1, 'VOL', undefined, true);
  const v = new hydrafs.Volume(img, { now: 0x2000 });
  const code = fn => { try { fn(); return 'none'; } catch (e) { return e.code || e.message; } };
  try {
    const C = v.cluster, a = v.create(v.root, 'a', 0);
    v.write(a, 0, Buffer.alloc(C + 100, 0x41));
    v.resize(v.lookup(v.root, 'a'), 3 * C + 7);                           // Longer: zeros
    let e = v.lookup(v.root, 'a'), d = v.read(e);
    if (e.size !== 3 * C + 7 || d[C + 99] !== 0x41 || d[C + 100] !== 0 || d[3 * C + 6] !== 0) f.push('a made longer: ' + e.size + ' bytes, or not zeros past its old end');
    v.resize(e, C + 50);                                                 // Cut, in a cluster
    e = v.lookup(v.root, 'a'); d = v.read(e);
    if (e.size !== C + 50 || d.length !== C + 50 || d[C + 49] !== 0x41 || v.extents(e).reduce((s, x) => s + x.len, 0) !== 2) f.push('a cut to a cluster and 50 bytes: ' + e.size + ' bytes in ' + v.extents(e).reduce((s, x) => s + x.len, 0) + ' clusters');
    v.resize(e, 2 * C); d = v.read(v.lookup(v.root, 'a'));               // Longer again: zeros where its cut bytes were
    if (d[C + 49] !== 0x41 || d[C + 50] !== 0 || d[C + 99] !== 0) f.push('a longer again: its cut bytes are back');
    v.resize(v.lookup(v.root, 'a'), 0);
    if (v.extents(v.lookup(v.root, 'a')).length) f.push('a cut to nothing still has clusters');
    // A hole: h's second to fourth clusters (as the Hydra's HydraFS leaves one), then a write into the third
    const h = v.create(v.root, 'h', 0);
    v.write(h, 0, Buffer.alloc(C, 0x42));
    const x0 = v.extents(v.lookup(v.root, 'h'))[0];
    let he = v.lookup(v.root, 'h');
    v.setExtents(he, [x0, { start: hydrafs.HOLE, len: 3 }]); he.size = 4 * C; v.writeEntry(he);
    v.write(v.lookup(v.root, 'h'), 2 * C + 10, Buffer.from('in the hole'));
    he = v.lookup(v.root, 'h'); d = v.read(he);
    const kinds = v.extents(he).map(x => (x.start === hydrafs.HOLE ? 'hole' : 'clusters') + ' ' + x.len).join(', ');
    if (kinds !== 'clusters 1, hole 1, clusters 1, hole 1') f.push('h\'s extents after a write into its hole: ' + kinds);
    if (d.length !== 4 * C || d[C - 1] !== 0x42 || d[C] !== 0 || d.toString('latin1', 2 * C + 10, 2 * C + 21) !== 'in the hole' || d[2 * C + 9] !== 0 || d[3 * C] !== 0) f.push('h read back after the write into its hole isn\'t right');
    v.resize(v.lookup(v.root, 'h'), 2 * C + 5);                           // Cut in its written cluster: the hole after it goes
    // Names
    v.rename(v.root, v.lookup(v.root, 'h'), 'hole.bin');
    if (!v.lookup(v.root, 'hole.bin') || v.lookup(v.root, 'h')) f.push('h renamed hole.bin: not so');
    const errs = [code(() => v.rename(v.root, v.lookup(v.root, 'a'), 'hole.bin')), code(() => v.rename(v.root, v.lookup(v.root, 'a'), 'x'.repeat(32))),
      code(() => v.create(v.root, 'a', 0)), code(() => v.walk('nope')), code(() => v.walk('a/b')), code(() => v.remove('/')),
      code(() => { v.mkdir('dd'); v.put('dd/f', Buffer.from('f')); v.remove('dd'); })].join(' ');
    if (errs !== 'EEXIST ENAMETOOLONG EEXIST ENOENT ENOTDIR EPERM ENOTEMPTY') f.push('the errors\' codes: ' + errs);
    v.setReadOnly(v.lookup(v.root, 'a'), true);
    if (!(v.lookup(v.root, 'a').mode & hydrafs.MODE_RO)) f.push('a read-only: its bit isn\'t set');
    v.setReadOnly(v.lookup(v.root, 'a'), false);
    if (v.lookup(v.root, 'a').mode & hydrafs.MODE_RO) f.push('a not read-only: its bit is set');
    if (v.lookup(v.root, 'hole.bin').stamp !== 0x2000) f.push('the volume\'s own stamp (now) isn\'t on its files');
    v.flush();
    const problems = v.check();
    if (problems.length) f.push('the free map after it all: ' + problems.slice(0, 3).join('; '));
  } catch (e) { f.push('hydrafs.js\'s Volume: ' + e.message); } finally { v.close(); }
  return f;
}
// The pc-ro test's lines: the folder served read-only
const PC_RO_LINES = [
  ["cat /pc/hello.txt","Hello"],
  ["echo no >/pc/x","rc: /pc/x: read-only"],
  ["echo no >/pc/hello.txt","rc: /pc/hello.txt: read-only"],
  ["rm /pc/hello.txt; mkdir /pc/d; mv /pc/hello.txt /pc/h","rm: /pc/hello.txt: read-only\nmkdir: /pc/d: read-only\nmv: /pc/hello.txt: read-only"],
  ["ls -l /pc","--r--r--r-- P-        6 2026-10-03 15:04 hello.txt"],
];
// A file of 120 lines (pc-two's), and a song (pc-song's: a note, then its loop, another, 0.6 s apart at 60 Hz)
const PC_BIG = () => Buffer.from([...Array(120)].map((_, i) => 'line ' + i + ' of the big file on the PC\n').join(''));
function PC_SONG() {
  const fm = pairs => [0x40 | pairs.length / 2, ...pairs];
  const voice = [0x20, 0xC7, 0x38, 0x00];
  for (const op of [0x00, 0x08, 0x10, 0x18]) voice.push(0x40 + op, 0x01, 0x60 + op, 0x10, 0x80 + op, 0x1F, 0xA0 + op, 0x00, 0xC0 + op, 0x00, 0xE0 + op, 0x0F);
  const note = kc => [...fm([0x28, kc, 0x30, 0x00, 0x08, 0x78]), 0x80 + 30, ...fm([0x08, 0x00]), 0x80 + 6];
  const intro = [...fm(voice), 0x05, 0x3F, 0x40, 0x82, 0x12, 0x34, ...note(0x3E)];    // (A PSG write; an extension)
  const loop = [...note(0x44)];
  const loopAt = 16 + intro.length;
  const hdr = [0x7A, 0x6D, 1, loopAt & 255, loopAt >> 8 & 255, loopAt >> 16, 0, 0, 0, 0x01, 0, 0, 60, 0, 0, 0];
  return Buffer.from([...hdr, ...intro, ...loop, 0x80]);
}
// The xmodem test's file: 3000 bytes, every value (Ctrl-C, Ctrl-], the frame marks ... too), its second 1K all SUB
// (the padding's byte: held back, then written, as data comes after it), its last byte not SUB
const XM_DATA = () => Buffer.from(Array.from({ length: 3000 }, (_, i) => i >= 1024 && i < 2048 ? 0x1A : i === 2999 ? 0x41 : (i * 7 + (i >> 8)) & 0xFF));

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
  for (const n of [...FORTH_SUITE, ...FORTH_HELPERS]) v.put(n, fs.readFileSync(path.join(__dirname, 'forth', n)));
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
  for (const n of ['tester.fr', 'numberstest.fth']) v.put(n, fs.readFileSync(path.join(__dirname, 'forth', n)));
  v.put('nums.fs', Buffer.from('REQUIRE string.fl\nREQUIRE hydra.fl\nREQUIRE numbers.fl\nS" tester.fr" INCLUDED\n' +
    'S" numberstest.fth" INCLUDED\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The lshell test's card: /lib/shell, HyForth as the shell (or the shell given: the hywin test's, hylang)
function shellCard(line = '/bin/forth -l', name = 'shell0') {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, name + '.img');
  hydrafs.mkfs(f, 8, 'SHELL', undefined, true);
  const v = new hydrafs.Volume(f);
  v.mkdir('lib');
  v.put('lib/shell', Buffer.from(line + '\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// hylang's card (the hysuite and hyhydra tests'): danlang's suite's files (tests/hylang), hylang's own checks
// (tests/hyhydra), and the test's own files ({ name: text }); its library (danlang's) is the ROM disk's, /lib/hylang.
// The image is the test's own (obj/cards/hylang-TEST.img), as sim/test.js -j runs tests side by side
function hylangCard(test, files = {}) {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'hylang-' + test + '.img');
  fs.rmSync(f, { force: true });
  hydrafs.mkfs(f, 8, 'HYLANG', undefined, true);
  const v = new hydrafs.Volume(f);
  const dir = path.join(__dirname, 'hylang');
  const put = (from, to) => v.put(to, fs.readFileSync(from));
  for (const n of fs.readdirSync(dir).filter(n => /\.(dl|hl)$/.test(n))) put(path.join(dir, n), n);
  const own = path.join(__dirname, 'hyhydra');                // (hylang's own checks: hydra.hl)
  for (const n of fs.readdirSync(own).filter(n => /\.hl$/.test(n))) put(path.join(own, n), n);
  for (const [n, text] of Object.entries(files)) v.put(n, Buffer.from(text, 'latin1'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// BASIC's suite (the bsuite test's card): tests/basic's files, each as itself: the programs that check themselves
// (NAME.bas: its checks counted, a FAIL line for each one wrong, then "NAME: n checks, m failed"; BSUITE_PROGS, each
// one's count), the scripts piped into basic (NAME.txt) and what they print (NAME.out; BSUITE_SCRIPTS).  hydra.bas
// reads rc's $greet (hi) and its arguments (one two); files.bas and errors.bas write their files on the card;
// procs.bas INCLUDEs inc.bas, prompt.txt LOADs lab.bas; asm.bas's ASM blocks .include /lib/as's hydra.inc
const BASIC_DIR = path.join(__dirname, 'basic');
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

// danlang's suite in parts (the hysuite tests), so that -j runs them side by side: each part is run.dl itself with
// its list of the suite's files cut to the part's (its harness, its counting and its status are run.dl's own), and
// the parts' files together are run.dl's, in its order.  Nearly all the suite's time is eval.dl's tail loops (50,000
// steps each, 0.2-0.8 billion cycles apiece), so eval.dl is cut too, on the card (the file itself unchanged): eval1.dl
// up to its first cut, then a piece from each cut (a line that starts with it, found once) to the next.  checks: a
// part's count (the suite's: 1,337); about: more of what it checks
const HYSUITE_CUTS = { eval: ['(fun {ev-loop n}', '(fun {ev-loop-do n}', '(fun {ev-loop-let n}', '(fun {ev-loop-eval n}', '(fun {ev-sum-to n acc}'] };
const HYSUITE_PARTS = [
  { files: ['reader', 'eval1', 'eval2'], checks: 187, about: 'run.dl a script, args its name, its status; with its harness, loaded from a card: load reads a file an item at a time, refilled as it goes; a load nested in another' },
  { files: ['eval3'], checks: 1 },
  { files: ['eval4'], checks: 1 },
  { files: ['eval5'], checks: 3 },
  { files: ['eval6', 'scope', 'control', 'errors', 'lists', 'strings', 'numbers', 'math', 'hashes', 'types', 'io', 'system', 'bits', 'buffers', 'library'], checks: 1348,
    about: 'danlang\'s library, hylang\'s from its snapshot (globals.dl) and the ROM disk\'s /lib/hylang (dice.dl and screen.dl, where load finds a bare name, and use); files written on the card, programs run, the clock a DS1747\'s' },
];
// A part's own files for the card: part.dl (run.dl with the part's list) and the pieces it names of a file that's cut
function hysuiteFiles(part) {
  const dir = path.join(__dirname, 'hylang'), run = fs.readFileSync(path.join(dir, 'run.dl'), 'latin1'), list = /\{"reader" [^}]*\}/;
  if (!list.test(run)) throw new Error('tests/hylang/run.dl: its list of files ({"reader" ...}) not found');
  const all = run.match(list)[0].slice(1, -1).split(' ').map(s => s.replace(/"/g, ''));
  const pieces = {};                                          // ('eval3': its text)
  for (const [f, cuts] of Object.entries(HYSUITE_CUTS)) {
    const lines = fs.readFileSync(path.join(dir, f + '.dl'), 'latin1').split('\n');
    const at = cuts.map(c => {
      const k = lines.map((l, i) => l.startsWith(c) ? i : -1).filter(i => i >= 0);
      if (k.length !== 1) throw new Error('tests/hylang/' + f + '.dl: the cut "' + c + '" found ' + k.length + ' times, not once');
      return k[0];
    });
    if (at.some((k, i) => i && k <= at[i - 1])) throw new Error('HYSUITE_CUTS.' + f + ': not in the file\'s order');
    [0, ...at, lines.length].forEach((k, i, a) => { if (i < a.length - 1) pieces[f + (i + 1)] = lines.slice(k, a[i + 1]).join('\n'); });
  }
  const named = HYSUITE_PARTS.flatMap(p => p.files).map(n => n.replace(/\d+$/, ''));
  const whole = named.filter((n, i) => n !== named[i - 1]);
  if (whole.join(' ') !== all.join(' ') || HYSUITE_PARTS.flatMap(p => p.files).filter(n => /\d$/.test(n)).join(' ') !== Object.keys(pieces).join(' '))
    throw new Error('HYSUITE_PARTS: not run.dl\'s files (' + all.join(' ') + ') in its order, each cut file\'s pieces in theirs');
  const files = { 'part.dl': run.replace(list, '{' + part.files.map(f => '"' + f + '"').join(' ') + '}') };
  for (const n of part.files) if (pieces[n] !== undefined) files[n + '.dl'] = pieces[n];
  return files;
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

// hylang's lines (the hylang test's): each typed at its prompt, what it prints (=> ...; none: it wants more), and its
// prompt if it isn't hylang> (the closers wanted)
const HYLANG_LINES = [
  ['42', '42'], ['', 'NIL'], ['1 2 3', 'Error: S-Expression starts with incorrect type. Got Number, Expected Function.'],
  ['(+ 1 2)', '3'], ['+ 1 2', '3'],
  ['{a B :C T nil exit () {} [] [1 2]}', '{a b :c T NIL exit NIL NIL (list) (list 1 2)}'],
  ['{"a\\nb" "\\e[1m\\x01\\x7f" """x"y""" "" """""" "\\x41\\x4a2" "tab\\there" "\\\\\\""}',
    '{"a\\nb" "\\e[1m\\x01\\x7F" "x\\"y" "" "" "AJ2" "tab\\there" "\\\\\\""}'],
  ['{\\a \\A \\space \\( \\] \\lf \\LineFeed \\line-feed \\null \\\\ \\" \\; \\escape \\del \\x}',
    '{\\a \\A \\space \\lparen \\rbracket \\lf \\lf \\lf \\null \\backslash \\quote \\semicolon \\escape \\delete \\x}'],
  ['{?(c a b) ?{c} =(x 1) :(y 2) #(z) @({x} {x}) .(f l) ~("s") ?x a:b :}',
    '{(if c a b) {if c} (set x 1) (def y 2) (hash-create z) (fn {x} {x}) (unpack f l) (format "s") ?x a:b :}'],
  ['{$HOME $Mixed_Case $}', '{(env "HOME") (env "Mixed_Case") $}'],
  ['{1 -2 +3 16383 -16384 1_000 1_ +_1 007 -0 - + 1+ -_ 1a _1 #x10 1.5}', '{1 -2 3 16383 -16384 1000 1 1 7 0 - + 1+ -_ 1a _1 16 1.5}'],
  ['{a ; a comment'], ['b}', '{a b}', '\t} <'], ['(list 1'], ['{b', undefined, '\t) <'], ['c})', '{1 {b c}}', '\t)} <'],
  ['"""x'], ['y"""', '"x\\ny"', '\t""" <'],
  ['(1 2]', 'Error: Closed a list without opening: )'], [')', 'Error: Closed a SExpr without opening: '],
  ['{a)', 'Error: Closed a SExpr without opening: }'], ['[a}', 'Error: Closed a QExpr without opening: ]'],
  ['f(x)', 'Error: \'f\' touches \'(\': put a space between them'], ['x[1]', 'Error: \'x\' touches \'[\': put a space between them'],
  ['+#(a)', 'Error: \'+#\' touches \'(\': put a space between them'],
  ['$(x)', 'Error: \'$(\' isn\'t danlang: $name is the environment\'s variable'], ['"\\q"', 'Error: Unknown escape sequence \\q'],
  ['"abc', 'Error: Newlines are not allowed in regular strings'], ['"\\x"', 'Error: \\x needs a hex digit'],
  ['\\zzz', 'Error: Unknown character name \\zzz'], ['\\', 'Error: A character needs a name'],
  ['70000', '70000'], ['{1/2 6/4 -#b101 #[01]11 #16rff 0.50 1/x #<x01 #c+-}', '{1/2 3/2 -5 3 255 0.5 1/x 16 -2}'],
  ['1/0', 'Error: Division by zero: 1/0'],
  ['('.repeat(100)], ['('.repeat(100), undefined, '\t' + ')'.repeat(100) + ' <'],
  ['('.repeat(55) + '1' + ')'.repeat(55), undefined, '\t' + ')'.repeat(200) + ' <'],
  [')'.repeat(100), undefined, '\t' + ')'.repeat(200) + ' <'], [')'.repeat(100), '1', '\t' + ')'.repeat(100) + ' <'],
  ['('.repeat(100)], ['('.repeat(100), undefined, '\t' + ')'.repeat(100) + ' <'],
  ['('.repeat(56), 'Error: Too deep: more than 255 brackets open', '\t' + ')'.repeat(200) + ' <'],
  ['(def {x} 10)', 'NIL'], ['(* x x)', '100'],
  ['(fun {fact n} {if (zero? n) 1 (* n (fact (- n 1)))})', 'NIL'], ['(fact 7)', '5040'],
  ['(fun {loop n} ?{(zero? n) :done (loop (- n 1))})', 'NIL'], ['(loop 16000)', ':done'],
  ['(fun {deep n} ?{(zero? n) 0 (+ 1 (deep (- n 1)))})', 'NIL'], ['(deep 1000)', '1000'],
  ['(deep 3000)', 'Error: Too deep: more than 2500 calls nested'],
  ['undefined-thing', 'Error: Unbound Symbol \'undefined-thing\''],
  ['((fn {a b c} {+ a b c}) 1)', '<function>(fn {b c} {+ a b c})'], ['(((fn {a b c} {+ a b c}) 1) 2 3)', '6'],
  ['((eq 1) 1)', 'T'], ['(repr (eq 1))', '"<function>(eq 1)"'], ['eq', '<function>(eq)'],
  ['(len {1} 2)', 'Error: \'len\' takes 1 argument, not 2'], ['((fn {x} {x}) 1 2)', 'Error: The function takes 1 argument, not 2'],
  ['((fn {x} {&_}) 1 2 3)', '{2 3}'], ['((fn {} {&2}) :a :b)', ':b'],
  ['(let {{a 1} {b (+ a 1)}} (+ a b))', '3'], ['(do (def {wi} 0) (while (< wi 5) =(wi (+ wi 1))) wi)', '5'],
  ['(output-of (dotimes {i 3} (write i)) (each print {:a :b}))', '"012:a\\n:b\\n"'],
  ['(try (error "x" :e) (list &err &code))', '{"x" :e}'], ['(try (+ 1 2) 0)', '3'],
  ['(map (fn {x} {* x x}) (range 5))', '{0 1 4 9 16}'], ['(format "{} + {} = {}" 1 2 (+ 1 2))', '"1 + 2 = 3"'],
  ['(+ "n=" 5 \\space :a)', '"n=5 :a"'], ['(cmp {1 2} {1 3})', '-1'], ['(eq {1 "a" (b)} {1 "a" (b)})', 'T'],
  ['(fun {mk n} {fn {x} {+ x n}})', 'NIL'], ['((mk 5) 1)', '6'],
  ['(def {my-if} (fexpr {c a b} {if (eval c) (eval a) (eval b)}))', 'NIL'], ['(my-if NIL (error "no") 2)', '2'],
  ['(/ 7 0)', 'Error: Division by zero.'], ['(* 200 200)', '40000'],
  ['(fun {inf} {inf})', 'NIL'],
  ['(filter (fn {x} {> x 1}) {1 2 3})', '{2 3}'], ['(foldr - 0 {1 2 3})', '2'], ['((foldl +) 0 {1 2})', '3'],
  ['(list (any? neg? {1 -1}) (all? pos? {}) (find neg? {1 -2}) (count neg? {-1 2 -3}) (sum {1 2 3}) (product {2 3}))', '{T T -2 2 6 6}'],
  ['(map 5 {1})', 'Error: S-Expression starts with incorrect type. Got Number, Expected Function.'],
  ['(sort {T {2} :c 1 {1 2} {1} x "s"})', '{1 "s" :c x {1} {1 2} {2} T}'], ['(sort (range 12) >)', '{11 10 9 8 7 6 5 4 3 2 1 0}'],
  ['(sort {{1 :a} {0 :x} {1 :b}} (fn {x y} {< (fst x) (fst y)}))', '{{0 :x} {1 :a} {1 :b}}'], ['(sort {1 2} (fn {x y} {error "cmp"}))', 'Error: cmp'],
  ['(list (subset {1 2 3 4} 1 2) (index-of "hello" \\l) (last-index-of {1 2 3 2} 2))', '{{2 3} 2 3}'],
  ['(list (gensym) (gensym "TMP") (to-atom "AB") (to-atom 5) (< (random 6) 6))', '{g__1 tmp__2 :ab :5 T}'],
  ['(* 99999999999 99999999999)', '9999999999800000000001'],
  ['(list (/ 7 2) (* 1.5 2) (+ 1/2 0.5) (- 0 12345678901234567890) (* (complex 0 1) (complex 0 1)))', '{7/2 3 1 -12345678901234567890 -1}'],
  ['(list (to-str 255 "x") (to-str -7 "#m") (val "#zHYDRA") (to-fixed 2/3) (truncate -7/2) (fib 100))', '{"FF" "#mst" 30157606 0.6666666666 -3 354224848179261915075}'],
  ['(list (shl 1 40) (bit-and -1 #xffff) (hex 255 4) (bin 5) (lo -1) (word 52 18))', '{1099511627776 65535 "00FF" "101" 255 4660}'],
  ['(error-code (bit-and 1.5 1))', ':inval'], ['(< (random 100000000000000000000000) 100000000000000000000000)', 'T'],
  ['(list (str-split "a,b;;c" {"," ";"}) (str-upper "hi") (char-at "abc" 1) (str-pad-left "7" 3 "0") (code-char 66))',
    '{{"a" "b" "" "c"} "HI" \\b "007" \\B}'],
  ['(list (str-trim " x ") (str-replace "a-b" "-" "+") (str-join {1 "a" :b} ",") (str-repeat "ab" 2) (alpha? "ab"))',
    '{"x" "a+b" "1,a,:b" "abab" T}'],
  ['(def {h} (hash-create {{:a 1} {"s" (+ 1 1)} {2 :two} :t}))', 'NIL'], ['h', '<hash>{{:a 1} {"s" 2} {2 :two} :t}'],
  ['(list (h "s") (hash-get h 2.0) (hash-keys h) (len h) (from# (hash-clone h {:b 3})))',
    '{2 :two {:a "s" 2} 3 {{:a 1} {"s" 2} {2 :two} {:b 3} :t}}'],
  ['(def {o} (to# {{:n 1} {:inc (fn {} {hash-put &0 {:n (+ (&0 :n) 1)}})} {:k 7 :__private}}))', 'NIL'],
  ['(list (o :inc) (hash-call o :inc) (o :n) (eq o (hash-clone o)))', '{1 2 3 T}'],
  ['(o :k)', 'Error: hash-get error: cannot access private hash entry'],
  ['(do (hash-lock h) (hash-put h {:new 1}))', 'Error: hash-put error: cannot add new entries to a locked hash'],
  ['(list (read "1 {2} (+ 3)") (platform) (hydra?) (tick-rate) (date 0) (seconds-of 2024 2 29))',
    '{{1 {2} (+ 3)} :hydra T 200 "2000-01-01 00:00:00" 762480000}'],
  ['(list (sh-out "echo hi there") (sh "exit 3") (sh-out "sort" "b\\na\\n"))', '{"hi there\\n" 3 "a\\nb\\n"}'],
  ['(output-of (print-to stdout "a" 1) (write-to stdout "b") (save {1 "s" {} #({{:k 2}})}))',
    '"a 1\\nb{1 \\"s\\" {} (hash-create {{:k 2}})}\\n"'],
  ['(do (setenv :hyt "v") (list (env "hyt") $hyt (unsetenv "hyt") (env "hyt")))', '{"v" "v" NIL NIL}'],
  ['(list (error-code (read-file "/nope")) (error-message (open "/nope")) (type-of stdin))', '{:noent "/nope: not found" :stream}'],
];
// hylang -g's lines (a collection before every allocation)
const HYLANG_G = [
  ['{"a" "b" (c d) [e f] $G :h \\i 123 "c\\x41"}', '{"a" "b" (c d) (list e f) (env "G") :h \\i 123 "cA"}'],
  ['(fun {fact n} {if (zero? n) 1 (* n (fact (- n 1)))})', 'NIL'], ['(fact 7)', '5040'],
  ['(map (fn {x} {* x x}) (range 5))', '{0 1 4 9 16}'], ['(let {{a 1} {b (+ a 1)}} (list a b))', '{1 2}'],
  ['(output-of (each {c "ab"} (write c ".")))', '"a.b."'], ['(try (error "x") &err)', '"x"'],
  ['(sort {3 1 2} >)', '{3 2 1}'], ['(filter (fn {x} {> x 1}) {1 2 3})', '{2 3}'], ['(foldr cons {} {1 2})', '{1 2}'],
  ['(list (subset {1 2 3} 1) (to-atom "x"))', '{{2 3} :x}'],
  ['(list (* 99999999999 99999999999) (/ 7 2) (+ 0.5 0.25) (/ (complex 1 2) (complex 3 4)) (to-str 1/2 "#b"))', '{9999999999800000000001 7/2 0.75 11/25+2/25i "#b0.1"}'],
  ['(list (val "#[01]101") (fib 100) (shl -3 70) (to-fixed 1/3 5))', '{5 354224848179261915075 -3541774862152233910272 0.33333}'],
  ['(def {g} (to# {{:a "x"} {:b {1 2}} :t}))', 'NIL'],
  ['(list (g :b) (from# (hash-clone g {:c 3})) (str-split "a b" " ") (str-upper \\q))', '{{1 2} {{:a "x"} {:b {1 2}} {:c 3} :t} {"a" "b"} \\Q}'],
  ['(do (def {bg} (buffer "abc")) (buffer-copy bg 1 (buffer {9 8})) (bytes bg))', '{97 9 8}'],
  ['(list (from-bytes (buffer {104 105})) (bg 2) (buffer 2 7))', '{"hi" 8 <buffer>{7 7}}'],
];
// hyspeed's: functions to time, then pairs of lines, each timed from its echo to its value (the same length), so
// the REPL's work and the serial line's drop out of their difference
const HYBUDGET_SETUP = ['(fun {two a b} {a})', '(fun {id x} {x})', '(fun {tl n} {if (zero? n) :done (tl (- n 1))})',
  '(fun {c2 n} {if (zero? n) :done (do (two 1 2) (c2 (- n 1)))})', '(fun {c0 n} {if (zero? n) :done (do 1 (c0 (- n 1)))})',
  '(fun {pl n} {if (zero? n) :done (do n n n n n n n n n n (pl (- n 1)))})',
  '(fun {pc n} {if (zero? n) :done (do 1 1 1 1 1 1 1 1 1 1 (pc (- n 1)))})', '(def {l1k} (range 1000))', '(def {l10} (range 10))'];
// (:w2 an untimed call of c2 just before its pair: its first, which may need a page of frames of two (a collection),
// so the pair's c2 is a call's cost alone; :wm, an untimed map of l1k before map's pair, for the same: the bytecode
// machine makes no frames, so its collections come at other lines)
const HYBUDGET_LINES = [['(list :t0 (tl 100))', '{:t0 :done}'], ['(list :t1 (tl 1100))', '{:t1 :done}'],
  ['(list :w2 (c2 500))', '{:w2 :done}'], ['(list :c0 (c0 500))', '{:c0 :done}'], ['(list :c2 (c2 500))', '{:c2 :done}'],
  ['(list :p0 (pc 500))', '{:p0 :done}'], ['(list :p1 (pl 500))', '{:p1 :done}'], ['(list :wm (zero? (len (map id l1k))))', '{:wm NIL}'],
  ['(list :m0 (zero? (len (map id l10))))', '{:m0 NIL}'], ['(list :m1 (zero? (len (map id l1k))))', '{:m1 NIL}']];
const hyBudget = (what, k0, k1, per, max) => ({ what, from: HYBUDGET_LINES[k1][0], to: '=> ' + HYBUDGET_LINES[k1][1],
  minus: [HYBUDGET_LINES[k0][0], '=> ' + HYBUDGET_LINES[k0][1]], per, max });
// hyhydra's lines with a collection before every allocation (hylang -g): each Hydra built-in that makes values, and
// sys- functions (bound as they're looked up, their values made); a third item, what's written before the value
const HYHYDRA_G = [
  ['(list ((sysinfo) :abi) (errstr :noent))', '{1 "not found"}'],
  ['(list ((task-info (pid)) :name) (any? (fn {x} {== (x :name) "cons"}) (ps)))', '{"hylang" T}'],
  ['(do (def {b} (bank-alloc 1)) (bank-write b 0 {1 2 3}) (bank-read b 0 3))', '{1 2 3}'],
  ['(list ((find (fn {e} {== (e :old) "/rom"}) (ns)) :new) ((free) :ram) (hold (list 1 2)))', '{"#fx/" 256 {1 2}}'],
  ['(list (== (sys-getpid) (pid)) (sys-env-put 255 "hy-g" "gv") (sys-env-get 255 "hy-g") ((sys-stat "hydra.hl") :name) sys-ticks)',
    '{T NIL {2 "gv"} "hydra.hl" <function>(sys :ticks)}'],
  ['(do (sys-puts "put, ") (sys-putc \\s) (sys-puthex 171) (sys-putc 10))', 'NIL', 'put, sAB\n'],
  ['(sys-write 1 {104 105 10})', '3', 'hi\n'],
];
// hytext's lines: hylang without its snapshot, its library loaded as text (globals.dl's definitions), a tail loop
const HYTEXT_LINES = [
  ['(list (square 7) (cube 3) (xor t nil) (flip - 1 10))', '{49 27 T 9}'], ['math.e', '2.71828182845904523536028747135266249775724709369995'],
  ['(map square {1 2 3})', '{1 4 9}'], ['(fun {hy-tail n} {if (zero? n) :done (hy-tail (- n 1))})', 'NIL'], ['(hy-tail 50000)', ':done'],
];

// A test's lines typed, each at its prompt, and its expect (as the tools test's)
const typed = lines => lines.map(l => 'ā' + l[0] + '\r').join('');
const expected = lines => lines.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%'));
// The PC host's report (its note), checked: attaches, damaged frames, repeats
function pcReport(m, attaches, naks, repeats) {
  const h = m.pc.host, f = [];
  if (h.attaches !== attaches) f.push('/pc: ' + h.attaches + ' attach(es), not ' + attaches);
  if (h.naks !== naks) f.push('/pc: ' + h.naks + ' damaged, not ' + naks);
  if (h.fsrv.stats.repeats !== repeats) f.push('/pc: ' + h.fsrv.stats.repeats + ' repeated, not ' + repeats);
  return f;
}
// A natural stratum's chunks (dq0m/caves.c), read from the hub's cache in the shared segments' banks (world.h's struct
// chunk, three to a bank, CHUNK_SIZE apart): its chambers ragged (none its bounds filled, some with their rows'
// ends uneven), its tunnels bending, every open square reached from every other, and a chunk's edges' open squares
// met by its neighbours' (those in the cache); n chunks of stratum sn at least (CAVEDUMP=1: each one drawn)
const CHUNK_SIZE = 0xAA0, CK_TILE = 443, CK_ROOM = 8;
const T_ROCK = 0, T_FLOOR = 1, T_WALL = 2, T_CORR = 4, T_WINDOW = 6;
function cacheChunks(m, sn) {
  const out = new Map();
  for (const b of Object.values(m.sharedBanks())) {
    for (let k = 0; k < 3; k++) {
      const o = k * CHUNK_SIZE, nrooms = b[o + 5], tile = b.subarray(o + CK_TILE, o + CK_TILE + 1024);
      if (b[o] !== sn || !nrooms || nrooms > 6 || b[o + 6] > 12 || b[o + 7] > 6 || tile.some(t => (t & 0x1F) > 9)) continue;
      const rooms = [];
      for (let r = 0; r < nrooms; r++) { const p = o + CK_ROOM + r * 8; rooms.push({ x: b[p], y: b[p + 1], w: b[p + 2], h: b[p + 3], kind: b[p + 5] }); }
      out.set((b[o + 1] | b[o + 2] << 8) + ',' + (b[o + 3] | b[o + 4] << 8), { cx: b[o + 1] | b[o + 2] << 8, cy: b[o + 3] | b[o + 4] << 8, rooms, tile: Array.from(tile, t => t & 0x1F) });
    }
  }
  return out;
}
function caveReport(m, sn, n) {
  const f = [], ks = cacheChunks(m, sn), open = t => t !== T_ROCK && t !== T_WALL && t !== T_WINDOW;
  if (ks.size < n) f.push('stratum ' + sn + ': ' + ks.size + ' chunk(s) in the cache, not ' + n);
  let ragged = 0, bends = 0;
  for (const [at, c] of ks) {
    const T = (x, y) => x < 0 || y < 0 || x > 31 || y > 31 ? T_ROCK : c.tile[y * 32 + x];
    for (const r of c.rooms) {
      if (r.kind !== 6) { f.push(at + ': a room not a cave'); continue; }
      let fl = 0; const starts = new Set();
      for (let y = r.y; y < r.y + r.h; y++) {
        let s = -1;
        for (let x = r.x; x < r.x + r.w; x++) if (T(x, y) === T_FLOOR) { fl++; if (s < 0) s = x; }
        starts.add(s);
      }
      if (r.w > 2 && r.h > 2 && fl === r.w * r.h) f.push(at + ': a chamber a rectangle (' + r.w + ' by ' + r.h + ')');
      if (starts.size >= 3 && fl * 10 < r.w * r.h * 9) ragged++;
    }
    for (let y = 0; y < 32; y++) for (let x = 0; x < 32; x++) {
      if (T(x, y) !== T_CORR) continue;
      const nb = [open(T(x, y - 1)), open(T(x + 1, y)), open(T(x, y + 1)), open(T(x - 1, y))];
      if (nb.filter(Boolean).length === 2 && !(nb[0] && nb[2]) && !(nb[1] && nb[3])) bends++;
    }
    const seen = new Set(), all = [];
    for (let i = 0; i < 1024; i++) if (open(c.tile[i])) all.push(i);
    const q = all.slice(0, 1);
    seen.add(q[0]);
    while (q.length) {
      const i = q.pop(), x = i & 31, y = i >> 5;
      for (const [dx, dy] of [[0, -1], [1, 0], [0, 1], [-1, 0]]) {
        const j = (y + dy) * 32 + x + dx;
        if (x + dx >= 0 && x + dx < 32 && y + dy >= 0 && y + dy < 32 && !seen.has(j) && open(c.tile[j])) { seen.add(j); q.push(j); }
      }
    }
    if (seen.size !== all.length) f.push(at + ': ' + (all.length - seen.size) + ' open square(s) not reached from the rest');
    for (const [dx, dy] of [[1, 0], [0, 1]]) {
      const d = ks.get((c.cx + dx) + ',' + (c.cy + dy));
      if (!d) continue;
      for (let k = 0; k < 32; k++) {
        const a = dx ? c.tile[k * 32 + 31] : c.tile[31 * 32 + k], b = dx ? d.tile[k * 32] : d.tile[k];
        if (open(a) !== open(b)) f.push(at + ': its ' + (dx ? 'east' : 'south') + ' edge\'s square ' + k + ' not met by its neighbour\'s');
      }
    }
  }
  if (process.env.CAVEDUMP) for (const [at, c] of ks) { console.log(at, JSON.stringify(c.rooms)); for (let y = 0; y < 32; y++) console.log(c.tile.slice(y * 32, y * 32 + 32).map(t => ' .#+,="<>O'[t]).join('')); }
  if (!ragged) f.push('stratum ' + sn + ': no chamber ragged');
  if (bends < 4 * ks.size) f.push('stratum ' + sn + ': ' + bends + ' bend(s) in its tunnels, fewer than 4 a chunk');
  return f;
}

// The draw test's lines at HyForth (lib video) and hylang (video.hl), and what each prints ('': nothing): vid's
// /dev/vid/draw on the bitmap at each depth, read back by vpeek; the turtle; rc's lines to the file
const DRAW_FORTH = [
  ['echo serial >/dev/consctl', ''], ['lib video', ''], ['320 8 bitmap  0 pen clear  5 pen 10 20 plot  20 320 * 10 + 0 vpeek .', '5 '],
  ['0 0 9 0 line  4 0 vpeek .', '5 '], ['20 20 29 29 box  20 320 * 25 + 0 vpeek .  25 320 * 25 + 0 vpeek .', '5 0 '],
  ['40 40 44 42 bar  41 320 * 42 + 0 vpeek .  43 320 * 42 + 0 vpeek .', '5 0 '],
  ['100 100 10 circle  100 320 * 110 + 0 vpeek .  100 320 * 100 + 0 vpeek .', '5 0 '],
  ['200 100 5 disc  100 320 * 203 + 0 vpeek .', '5 '], ['-5 -5 plot  5000 0 plot', 'plot: invalid argument'],
  ['320 4 bitmap  0 pen clear  7 pen 1 0 plot  0 0 vpeek .  12 pen 0 0 plot  0 0 vpeek .', '7 199 '],
  ['320 2 bitmap  0 pen clear  2 pen 2 0 plot  0 0 vpeek .', '8 '],
  ['640 1 bitmap  0 pen clear  1 pen 3 0 plot  0 0 vpeek .  0 479 639 479 line  479 80 * 0 vpeek .', '16 255 '],
  ['640 4 bitmap', 'invalid argument'], ['320 8 bitmap  cs 7 pen 50 fd 90 rt 40 fd heading .', '90 '],
  ['95 320 * 160 + 0 vpeek .  70 320 * 180 + 0 vpeek .', '7 7 '], ['200 $F00 palette!  2 100 50 sprite-at', ''],
  ['0 pen clear  6 pen 0 0 s" Hi" text  1 0 vpeek .  0 0 vpeek .  3 320 * 3 + 0 vpeek .', '6 0 6 '],
  ['echo pen 4 >/dev/vid/draw; cat /dev/vid/draw', 'pen 4'],
];
const DRAW_HY = [['(use "video")', 'NIL'], ['(pen 9)', 'NIL'], ['(plot 30 30)', 'NIL'], ['(vpeek (+ (* 30 320) 30))', '9'], ['(cs)', 'NIL'],
  ['(pen 11)', 'NIL'], ['(fd 30)', 'NIL'], ['(vpeek (+ (* 95 320) 160))', '11'], ['(heading)', '0'],
  ['(text 0 10 "H")', 'NIL'], ['(vpeek (+ (* 10 320) 1))', '11']];

// The vsd test's card: a HydraFS volume (hello.txt) on the VERA's own SD port, its writes kept; and its lines
function veraCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  const img = path.join(CARD_DIR, 'vsd.img');
  hydrafs.setNow(0x1000);
  hydrafs.mkfs(img, 8, 'VERASD', undefined, true);
  const v = new hydrafs.Volume(img);
  v.put('hello.txt', Buffer.from('hello from the vera\n'));
  v.close();
  return imageCard(0, img, Math.floor(fs.statSync(img).size / 512));
}
const VSD_LINES = [
  ['cat /dev/sd/v/ctl', 'sdhc 8 MB 16384 blocks\nhydrafs label=VERASD\nfree 8180 KB of 8188 KB'],
  ['cat /sd/v/hello.txt', 'hello from the vera'],
  ['echo written >/sd/v/new.txt; cat /sd/v/new.txt', 'written'],
  ['ls /sd', 'v/'],
];

// The hmchar tests' checks (hmcharhl, hmcharfs: games/hmchar's hylang and HyForth generators, C's hmchar's
// companions): the last profile printed, and the file written (harnfile.c's lines), each as it should be and the two
// agreeing (the skills' names, keys and which are automatic from harn.hl's Skill Data table, a line each): a list of
// failures
const HM_SKILLS = fs.readFileSync(path.join(__dirname, '..', 'romfs', 'lib', 'hylang', 'harn.hl'), 'latin1').split(/\r?\n/)
  .filter(l => l.length === 41 && /^[a-z]{3,4} +[A-Z]/.test(l) && l[27] === ' ' && l[36] === ' ')
  .map(l => ({ key: l.slice(0, 5).trim(), name: l.slice(5, 24).trim(), auto: l[38] === '1' }));
const HM_NAMES = {
  sp: ['Human', 'Sindarin', 'Khuzdul', 'Other'], sx: ['Female', 'Male'],
  month: ['Nuzyael', 'Peonu', 'Kelen', 'Nolus', 'Larane', 'Agrazhar', 'Azura', 'Halane', 'Savor', 'Ilvin', 'Navek', 'Morgat'],
  culture: ['Tribal', 'Viking', 'Feudal', 'Imperial', 'Khuzan', 'Sindarin'], social: ['Slave', 'Serf', 'Unguilded', 'Guilded', 'Noble'],
  parent: ['Offspring', 'Fostered', 'Adopted', 'Bastard', 'Orphan'], estrange: ['Outcast', 'Unpopular', 'Average', 'Popular', 'Favorite'],
  clanhead: ['Distant Relation', 'Aunt or Uncle', 'Cousin', 'Father or Mother'], fr: ['Scant', 'Light', 'Medium', 'Heavy', 'Massive'],
  complexion: ['Fair', 'Medium', 'Dark'], hair: ['Brown', 'Black', 'Red', 'Silver', 'Blond'], eyes: ['Hazel', 'Gray', 'Violet', 'Green', 'Blue'],
};
const HM_DEITIES = ['Agrik', 'Halea', 'Ilvir', 'Larani', 'Morgath', 'Naveh', 'Peoni', 'Sarajin', 'Save-K' + "'" + 'nor', 'Siem'];
const HM_ATTRS = ['sp', 'sx', 'bd', 'ss', 'ht', 'fr', 'owt', 'wt', 'cml', 'str', 'sta', 'dex', 'agl', 'eye', 'hrg', 'sml', 'voi',
  'int', 'aur', 'wil', 'mor', 'dty', 'pty'];
const HM_OCCS = fs.readFileSync(path.join(__dirname, '..', 'games', 'hmchar', 'occupations.txt'), 'latin1').split(/\r?\n/)
  .filter(l => /^[^;[\s][^|]*\|/.test(l)).map(l => l.split('|')[0].trim()).filter(n => n !== 'all');
function hmcharCheck(out, file) {
  const f = [], end = out.lastIndexOf('\nWritten to ');
  if (HM_SKILLS.length !== 91) return ['harn.hl\'s Skill Data table: ' + HM_SKILLS.length + ' skills read, not 91'];
  if (end < 0) return ['no "Written to"'];
  const lines = out.slice(0, end).split('\n'), at = lines.length - 1 - [...lines].reverse().findIndex(l => / born \d+ /.test(l));
  const p = lines.slice(at), fl = file.split('\n').filter(l => l);
  const m1 = p[0].match(/^(.+), (\w+) (\w+), born (\d+) (\w+) \((.+)\); (\w+) (\w+), (.+)$/);
  const m2 = (p[1] || '').match(/^(\d+) of (\d+); (\w+), (\w+); clanhead: (.+)$/);
  const m3 = (p[2] || '').match(/^(\d+)", (\w+), (\d+) lb \(size (\d+)\); (\w+) (\d+); (\w+), (\w+) hair, (\w+) eyes$/);
  const m6 = (p[5] || '').match(/^Endurance (\d+), Move (\d+), Piety (\d+)(?: \((.+)\))?; medical:(.*)$/);
  if (!m1 || !m2 || !m3 || !m6) return ['the profile isn\'t as hmchar prints it: ' + JSON.stringify(p.slice(0, 7))];
  // The file: its lines in harnfile.c's order
  const a = {};
  if (fl[0] !== 'harn 1') f.push('the file\'s first line: ' + fl[0]);
  HM_ATTRS.forEach((k, i) => { const m = (fl[1 + i] || '').match(/^att (\w+) (\d+)$/); if (!m || m[1] !== k) f.push('att ' + k + ': ' + fl[1 + i]); else a[k] = +m[2]; });
  const nums = (l, key, n) => { const w = (l || '').split(' '); if (w[0] !== key || w.length - 1 < n) { f.push(key + ': ' + l); return []; } return w.slice(1).map(Number); };
  const fam = nums(fl[24], 'fam', 7), look = nums(fl[25], 'look', 3), med = nums(fl[26], 'med', 0);
  let k = 27;
  while (/^psy /.test(fl[k] || '')) k++;
  if (fl[k++] !== 'sexuality 0') f.push('sexuality: ' + fl[k - 1]);
  const ml = new Map();
  while (/^ml /.test(fl[k] || '')) { const w = fl[k++].split(' '); ml.set(w[1], +w[2]); }
  if (fl[k++] !== 'vol 0 0 0 0 0') f.push('vol: ' + fl[k - 1]);
  if (fl[k++] !== 'name ' + m1[1]) f.push('name: ' + fl[k - 1] + ', the profile\'s ' + m1[1]);
  if (fl[k++] !== 'occupation ' + m1[9]) f.push('occupation: ' + fl[k - 1] + ', the profile\'s ' + m1[9]);
  if (fl.length !== k) f.push('the file\'s lines after occupation: ' + JSON.stringify(fl.slice(k)));
  if (f.length) return f;
  // The profile agreeing with it
  const eq = (what, x, y) => { if (x !== y) f.push(what + ': ' + x + ' printed, ' + y + ' in the file'); };
  eq('species', m1[2], HM_NAMES.sp[a.sp]); eq('sex', m1[3], HM_NAMES.sx[a.sx]);
  eq('birthday', +m1[4], a.bd & 255); eq('month', m1[5], HM_NAMES.month[(a.bd >> 8) - 1]);
  eq('culture', m1[7], HM_NAMES.culture[fam[0]]); eq('class', m1[8], HM_NAMES.social[fam[1]]);
  if (!HM_OCCS.includes(m1[9])) f.push('occupation ' + m1[9]);
  eq('sibling rank', +m2[1], fam[2]); eq('family', +m2[2], fam[3]); eq('parent', m2[3], HM_NAMES.parent[fam[4]]);
  eq('estrangement', m2[4], HM_NAMES.estrange[fam[5]]); eq('clanhead', m2[5], HM_NAMES.clanhead[fam[6]]);
  eq('comeliness', m3[5], ['Ugly', 'Plain', 'Average', 'Attractive', 'Handsome'][a.cml <= 5 ? 0 : a.cml <= 8 ? 1 : a.cml <= 12 ? 2 : a.cml <= 15 ? 3 : 4]);
  eq('height', +m3[1], a.ht); eq('frame', m3[2], HM_NAMES.fr[a.fr]); eq('weight', +m3[3], a.wt); eq('comeliness', +m3[6], a.cml);
  eq('complexion', m3[7], HM_NAMES.complexion[look[0]]); eq('hair', m3[8], HM_NAMES.hair[look[1]]); eq('eyes', m3[9], HM_NAMES.eyes[look[2]]);
  eq('physical', p[3], `Strength ${a.str}, Stamina ${a.sta}, Dexterity ${a.dex}, Agility ${a.agl}, Eyesight ${a.eye}, Hearing ${a.hrg}, Smell ${a.sml}, Voice ${a.voi}`);
  eq('personality', p[4], `Intelligence ${a.int}, Aura ${a.aur}, Will ${a.wil}, Morality ${a.mor}`);
  eq('endurance', +m6[1], Math.floor(((a.str + a.sta + a.wil) * 2 + 3) / 6)); eq('move', +m6[2], a.agl); eq('piety', +m6[3], a.pty);
  const church = HM_DEITIES.findIndex(d => m1[9] === 'Cleric (' + d + ')');     // (A cleric's deity its church's; else the
  if (church >= 0 ? a.dty !== church : !(a.dty <= 10)) f.push('a deity: ' + a.dty + ', ' + m1[9]);   //   player's, or none)
  eq('deity', m6[4] || 'none', a.dty === 10 ? 'none' : HM_DEITIES[a.dty]);
  if (a.dty < 10 && !ml.has(HM_SKILLS[21 + a.dty].key)) f.push('the deity\'s ritual not open: ' + HM_SKILLS[21 + a.dty].name);
  eq('medical traits', m6[5] === ' none' ? 0 : m6[5].trim().split(' ').length, med.length);
  const sk = (p.slice(6).join('\n').match(/^Skills \(ML\):([^]*)$/) || [])[1];
  if (!sk) return [...f, 'no skills line'];
  const shown = new Map([...sk.matchAll(/ ([A-Z][A-Za-z':\- ]*?) (\d+)(?= |\n|$)/g)].map(m => [m[1].trim(), +m[2]]));
  for (const s of HM_SKILLS) {
    if (s.auto && !ml.has(s.key)) f.push('an automatic skill not open: ' + s.name);
    if (ml.get(s.key) !== shown.get(s.name)) f.push(s.name + ': ' + shown.get(s.name) + ' printed, ' + ml.get(s.key) + ' in the file');
  }
  if (ml.size !== shown.size) f.push(ml.size + ' skills in the file, ' + shown.size + ' printed');
  return f;
}

// The hmcharhl test's runs: hmchar.hl -q, its file; then as a player answers: a roll again, kept; a deity (a beginning
// that isn't enough, one there isn't, then Peoni); an occupation out of
// range, the Gladiator's skills shown (? and a beginning of its name), then it chosen so; weapons (a beginning, one that isn't enough, one twice); option points (Climbing three times: the second and third refused, two points a skill at most,
// a skill there isn't, Enter); a name; its file
const HM_PLAY = 'r\rk\rs\rxyz\rpeo\ru\r99\r?glad\rglad\rsw\rs\rshield\rshield\rdag\rclimb\rclimb\rclimb\rzzz\r\rDorn\r';
const HM_DEITY_SAID = ['Which (a beginning enough; Enter alone, none)? s\nWhich? More than one begins so.\n',
  'Which (a beginning enough; Enter alone, none)? xyz\nThere\'s no such deity.\n', 'Which (a beginning enough; Enter alone, none)? peo\nRitual: Peoni is '];
// The hmcharfs test's runs as a player answers, each C's and HyForth's the same (the book's limits, the user's,
// 2026-10-20): seed 64, a Feudal noblewoman: the nobles listed (the Herald, the Lady), the Lady's skills shown, then
// her chosen; seed 91, a Feudal nobleman: the nobles listed (the Herald alone), no Lady, a knight a beginning that
// isn't enough, then the Knight MH; seed 5, a Feudal serf (Tordan): no nobles, the Herald and the Lady not his, a
// beginning (g) that's the unguilded Gladiator's and the soldiers' guardsmen, no knight, the Clothier not his (a
// guilded occupation), then a farmer
const HM_LIMITS = [
  ['64', 'k\r\rn\r?lady\rlady\r\rAyla\r', 'Lady'],
  ['91', 'k\r\rn\rlady\rknight\rknight mh\r\rBran\r', 'Knight MH'],
  ['5', 'k\r\rn\rherald\rlady\rg\rknight\rclothier\rfarmer\r\rTor\r', 'Farmer'],
];
const HM_FILE = f => 'echo ==file; cat ' + f + '; echo ==end';
// A run's part of the output (from its command line), and the file it cat'd (between ==file and ==end)
function hmRun(out, cmd) {
  const a = out.indexOf('% ' + cmd + '\n'), b = out.indexOf('\n==file\n', a), e = out.indexOf('==end\n', b);
  return a < 0 || b < 0 || e < 0 ? null : { out: out.slice(a, b + 1), file: out.slice(b + 8, e) };
}

// A built stratum's chunks (dq0m/built.c, uses.c), read from the hub's cache in the shared segments' banks (world.h's
// struct chunk, three to a bank, 0xAA0 apart): each one's place, rooms (their uses, world.h's U_*), zones (their
// ceilings), portals, squares and floors, and its passages' ceiling (pceil); those of stratum sn that look whole
const U_HALL = 1, U_CELL = 2, U_CRYPT = 4, U_STORE = 7, U_KITCHEN = 8, U_PANTRY = 9, U_QUARTERS = 13, U_GATEHOUSE = 17,
  U_GUARDROOM = 18;
const PK_PORTCULLIS = 2, T_DOOR = 3, T_STEPS = 5;           // (T_FLOOR, T_CORR: the caves' above)
// The bytes xd said after the line it was typed on (cmd: its command line), up to the prompt: each line "OFFSET  hex
// ...  text"; and a theme's text as themegen said it (its command line typed, up to the next prompt), its CRs gone
function xdBytes(out, cmd) {
  const o = out.replace(/\r/g, ''), at = o.indexOf('% ' + cmd + '\n'), end = o.indexOf('\n% ', at + 1);
  const bytes = [];
  if (at < 0) return Buffer.alloc(0);
  for (const l of o.slice(at + cmd.length + 3, end < 0 ? undefined : end).split('\n')) {
    const m = /^([0-9a-f]{7})  ((?:[0-9a-f]{2} ?)+)/.exec(l);
    if (m) for (const h of m[2].trim().split(' ')) bytes.push(parseInt(h, 16));
  }
  return Buffer.from(bytes);
}
function saidTheme(out, cmd) {
  const o = out.replace(/\r/g, ''), at = o.indexOf('% ' + cmd + '\n'), end = o.indexOf('\n% ', at + 1);
  return at < 0 ? '' : o.slice(at + cmd.length + 3, end < 0 ? undefined : end + 1);
}
const DQ0_MAKE = () => require('../games/dq0/make.js');
const DQ0_BEASTS = () => DQ0_MAKE().bestiary(path.join(__dirname, '..', 'games', 'dq0'));

function builtChunks(m, sn) {
  const out = [], C = { room: 8, zone: 56, portal: 219, tile: 443, elev: 1467, pceil: 2555 };
  for (const b of Object.values(m.sharedBanks())) {
    for (let k = 0; k < 3; k++) {
      const o = k * 0xAA0, nr = b[o + 5], nz = b[o + 6], np = b[o + C.portal - 1];
      const tile = b.subarray(o + C.tile, o + C.tile + 1024);
      if (b[o] !== sn || !nr || nr > 6 || nz > 12 || b[o + 7] > 6 || np > 16 || tile.some(t => (t & 0x1F) > 9)) continue;
      const rooms = [], zones = [], portals = [];
      for (let r = 0; r < nr; r++) {
        const p = o + C.room + r * 8;
        if (b[p + 2] < 1 || b[p] + b[p + 2] > 31 || b[p + 1] + b[p + 3] > 31) { rooms.length = 0; break; }
        rooms.push({ x: b[p], y: b[p + 1], w: b[p + 2], h: b[p + 3], flags: b[p + 4], kind: b[p + 5] });
      }
      if (!rooms.length) continue;
      for (let z = 0; z < nz; z++) {
        const p = o + C.zone + z * 11;
        zones.push({ room: b[p + 4], ceil: b[p + 8], cmid: b[p + 9], cwall: b[p + 10] });
      }
      for (let i = 0; i < np; i++) { const p = o + C.portal + i * 14; portals.push({ x: b[p], y: b[p + 1], kind: b[p + 2] }); }
      out.push({ cx: b[o + 1] | b[o + 2] << 8, cy: b[o + 3] | b[o + 4] << 8, rooms, zones, portals, pceil: b[o + C.pceil],
        tile: Array.from(tile, t => t & 0x1F), elev: Array.from(b.subarray(o + C.elev, o + C.elev + 1024), e => e << 24 >> 24) });
    }
  }
  return out;
}

// The hub's live chunks of stratum sn (dq0pure's): its world bank's cslot table (hub.h's struct wrec: the strata's
// records, 8 of 79 bytes, then the slots, 8 bytes each: used, sn, cx, cy, stamp), each slot used holding a chunk of sn
// read from its cache bank (world.h's CACHE_BANK: 5 + k / 3, CHUNK_AT: (k % 3) * 0xAA0), its 2,556 bytes (struct
// chunk's), by its place: { 'cx,cy': bytes }; the hub's banks the shared ones from its bank 0's (its magic "DQ") on
const CHUNK_BYTES = 2556, WORLD_BANK_N = 21, CSLOT_AT = 8 * 79;
function liveChunks(m, sn) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51), out = {};
  if (hub === undefined || !banks[hub + WORLD_BANK_N]) return out;
  const w = banks[hub + WORLD_BANK_N];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * 8, b = banks[hub + 5 + Math.floor(k / 3)];
    if (!w[s] || w[s + 1] !== sn || !b) continue;
    const o = (k % 3) * 0xAA0;
    out[(w[s + 2] | w[s + 3] << 8) + ',' + (w[s + 4] | w[s + 5] << 8)] = Buffer.from(b.subarray(o, o + CHUNK_BYTES));
  }
  return out;
}

// A stratum's record in the hub's world bank (hub.h's struct wrec: the strata's records first, 8 of 79 bytes, world.h's
// struct stratum: made, n, ..., x0 at 11, y0, w, h, ..., nlinks at 22, its links from 23, 7 bytes each: kind, to, x,
// y, dug): stratum sn's bounds and links ({ x0, y0, w, h, links: [{ kind, to, x, y }] }), or null
const O_WAY_OUT = 7, O_NOWHERE = 0xFF;
function stratumRec(m, sn) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51), w = hub === undefined ? null : banks[hub + WORLD_BANK_N];
  for (let i = 0; w && i < 8; i++) {
    const r = w.subarray(i * 79, i * 79 + 79), u16 = k => r[k] | r[k + 1] << 8;
    if (!r[0] || r[1] !== sn) continue;
    const links = [];
    for (let j = 0; j < Math.min(r[22], 8); j++) links.push({ kind: r[23 + j * 7], to: r[24 + j * 7], x: u16(25 + j * 7), y: u16(27 + j * 7) });
    return { x0: u16(11), y0: u16(13), w: u16(15), h: u16(17), links };
  }
  return null;
}

// A dq0 test's start(m) for what each game made and loaded: as "> quit" is typed (the game still as it stands), the
// chunks of the top stratum in the hub's cache (builtChunks) and the strata's records (stratumRec, 0-7), t.games[k]
// for the test's game k (0 on: the hubs made, m.dq0's); and each file a program OPENs (the kernel's call, by its jump
// table slot: base/obj/gen/api.json's), its task and the hubs made by then (a game's themegen runs before its hub's
// made, its maker after), in t.opens
function dq0Made(t, m) {
  const kl = require('../../base/sim/run.js').labels(), cpu = m.cpu, step = cpu.step.bind(cpu);  // (The kernel's own OPEN:
  const K_OPEN = kl.byName.get('K_OPEN'), K_OPEN_PAGE = kl.pageOf.get('K_OPEN');                //   its loader, BANK_LOAD, opens a
  const K_STAT = kl.byName.get('K_STAT'), K_STAT_PAGE = kl.pageOf.get('K_STAT');                //   child itself; a STAT's open,
  const games = t.games = [], opens = t.opens = [], statting = new Set();                       //   its cache's check, not counted)
  let out = m.out, qpos = 0;
  cpu.step = v => {
    if (cpu.PC === K_STAT && m.W === K_STAT_PAGE) statting.add(m.T);
    else if (cpu.PC === K_OPEN && m.W === K_OPEN_PAGE && statting.has(m.T)) statting.delete(m.T);
    else if (cpu.PC === K_OPEN && m.W === K_OPEN_PAGE) {
      const r = m.taskRam[m.T], a = r[2] | r[3] << 8;
      let s = '';
      for (let i = 0; i < 64 && a + i < 0x8000 && r[a + i]; i++) s += String.fromCharCode(r[a + i]);
      opens.push({ T: m.T, path: s, hubs: m.dq0 ? m.dq0.games.length : 0 });
    }
    return step(v);
  };
  Object.defineProperty(m, 'out', { get: () => out, set: v => {
    let i;
    while ((i = v.indexOf('> quit', qpos)) >= 0) {
      if (m.dq0 && m.dq0.games.length) games[m.dq0.games.length - 1] = { chunks: builtChunks(m, 0), strata: [0, 1, 2, 3, 4, 5, 6, 7].map(sn => stratumRec(m, sn)) };
      qpos = i + 6;
    }
    out = v;
  } });
}
// (A chunk's main room: its largest, the first of the largest; the uses' plan's)
const mainRoom = c => c.rooms.reduce((a, r) => r.w * r.h > a.w * a.h ? r : a, c.rooms[0]);
// Two chunks' bytes compared (struct chunk's: rooms from 8, 8 bytes each, their flags the 5th; openings 188-217, their
// count at 7; portals from 219, 14 bytes each, their state, parts, harm the 4th, 9th and 14th; squares from 443, their
// floors from 1467; traps from 2513, 7 bytes each, their state the 5th): the squares' seen and visible bits (T_SEEN,
// T_VIS) and the rooms' visited (R_VISITED) aside, each byte that differs by what it is: portal, trap, square,
// opening, or other (none should be: the maker's own)
function chunkDiff(a, b) {
  const d = { portal: 0, trap: 0, square: 0, opening: 0, other: [] };
  for (let i = 0; i < CHUNK_BYTES; i++) {
    let x = a[i], y = b[i];
    if (i >= 443 && i < 1467) { x &= 0x3F; y &= 0x3F; }
    if (i >= 8 && i < 56 && (i - 8) % 8 === 4) { x &= ~1; y &= ~1; }
    if (x === y) continue;
    if (i >= 219 && i < 443 && [3, 8, 13].includes((i - 219) % 14)) d.portal++;
    else if (i >= 2513 && i < 2555 && (i - 2513) % 7 === 4) d.trap++;
    else if (i >= 443 && i < 1467) d.square++;
    else if (i >= 1467 && i < 2491 && (a[i - 1024] & 0x1F) !== (b[i - 1024] & 0x1F)) d.square++;   // (A changed square's floor)
    else if (i === 7 || (i >= 188 && i < 218)) d.opening++;
    else d.other.push(i);
  }
  return d;
}

// The senses' looks (dq0looks's): at each reply the senses give (dq0p's hub_reply: a look's, a denizen's senses'), the
// request (its first 16 bytes), the senses' record (SIGHT_BANK's first 2K) and each live chunk's seen and visible marks
// (its squares' T_SEEN, T_VIS, its rooms' R_VISITED; a chunk with none left out), hashed: a line each, "game request
// its-hash sight-hash sn:cx,cy=hash ...", game the games' count of "> quit" told before it
const LOOKS_REF = path.join(__dirname, 'dq0', 'looks.ref');
const lookHash = b => require('crypto').createHash('sha1').update(b).digest('hex').slice(0, 8);
function lookSnap(m, ptr, game) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51);
  if (hub === undefined) return null;
  const w = banks[hub + WORLD_BANK_N], ch = [];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * 8, b = banks[hub + 5 + Math.floor(k / 3)];
    if (!w[s] || !b) continue;
    const o = (k % 3) * CHUNK_SIZE, t = Buffer.alloc(1024 + 6);
    for (let i = 0; i < 1024; i++) t[i] = b[o + CK_TILE + i] & 0xC0;
    for (let r = 0; r < 6; r++) t[1024 + r] = b[o + CK_ROOM + r * 8 + 4] & 1;
    if (t.some(v => v)) ch.push(w[s + 1] + ':' + (w[s + 2] | w[s + 3] << 8) + ',' + (w[s + 4] | w[s + 5] << 8) + '=' + lookHash(t));
  }
  return [game, ptr.toString(16), lookHash(banks[hub].subarray(ptr - 0x8000, ptr - 0x8000 + 16)), lookHash(banks[hub + 19].subarray(0, 0x800)),
    ...ch.sort()].join(' ');
}

// The dq0 tests' story: the output with its escapes and the lines' wrapping taken out; and its words looked for each
// in its turn (after the one before), what isn't found pushed on f (its name first)
const dq0Story = out => out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' ');
function inTurn(o, want, f, name = '') {
  let at = 0;
  for (const w of want) {
    const i = o.indexOf(w, at);
    if (i < 0) { f.push(name + 'not found (in its turn): ' + w); return; }
    at = i + w.length;
  }
}

// Nothing told twice in one answer (the user's, 2026-10-17: "some messages are duplicated, like when describing a
// trap, it is shown twice in the 'look' results"): every dq0 test's answers (dq0Answers: the story between one line
// entered, the game's "> " or rc's "% ", and the next; the status line (reverse video), the line typed as it's drawn
// and the escapes out, its lines' wrapping undone), each sentence in one of them once (dq0Repeats: those said more,
// { test, cmd, sentence, n }), but those DQ0_REPEATS allows, each by its words (and the tests it's in, if only some),
// with its reason; and the hub's live chunks (dq0Doubled) with no two portals or traps on one square (a door made
// twice, its trap twice: the look told it twice).  sim/test.js's dq0 check, in every dq0 test (HUBOWN=0 too)
const DQ0_REPEATS = [
  { what: /^\(\d+ feet to go\.\)/, why: 'a dig\'s spells, each told as it ends (dig down: many spells to an answer)' },
  { what: /^You (?:climb on|slip back a little|find no hold yet) \(\d+ feet to go\)\.$/,
    why: 'a climb\'s rounds, each told as it ends (a long climb: many rounds to an answer, some the same)' },
  { what: /^(The|A) .* (bites|claws|strikes|swings|lunges|thrusts)\b.*\bat you\b|^You (strike|swing|thrust|punch|kick|lunge)\b/,
    why: 'a fight\'s rounds, each blow told as it falls (attack: the rounds till it ends, the same words some of them)' },
  { what: /^You hear /, why: 'a sound heard each time it\'s made (main.c\'s heard: half a minute apart the same way, five seconds any; a search\'s or a wait\'s minute hears several)' },
  { what: /^(The passage turns (left|right)|You go through a doorway|You go on down the passage|The way (rises|falls away))\.$/,
    why: 'a walk\'s beats as it goes (slice 4.0): a winding passage turns the same way more than once, has more than one doorway, goes on quietly every 30 feet' },
  { what: /^Something stirs\.$/, why: 'each stir\'s first words: one for each time what it sees of the denizens changes (a long command\'s answer has several)' },
];
function dq0Answers(out) {
  const s = out.replace(/\r/g, '').replace(/\x1b\[7m[\s\S]*?\x1b\[0m/g, '')
    .replace(/\x1b\[\d+;1H\x1b\[K(?:\x1b\[\?25h)?[^\x1b\n]*/g, '').replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '');
  const res = [];
  let cmd = '', ans = [];
  for (const l of s.split('\n')) {
    const p = l.match(/^[>%] (.*)$/);
    if (p) { res.push({ cmd, text: ans.join(' ').replace(/\s+/g, ' ').trim() }); cmd = p[1].trim(); ans = []; continue; }
    ans.push(l);
  }
  res.push({ cmd, text: ans.join(' ').replace(/\s+/g, ' ').trim() });
  return res;
}
function dq0Repeats(name, out) {
  const found = [];
  for (const a of dq0Answers(out)) {
    // (Each stir a telling of its own: a denizen's place told again after a change between, "two rats" then "a rat",
    // is what changed, not a repeat; within a telling, a look's or a stir's, nothing twice)
    for (const part of a.text.split(/(?=Something stirs\.)/)) {
      const n = new Map();
      for (const t of part.split(/(?<=[.!?])\s+/)) if (t.length > 8 && /[a-z]{2}/i.test(t)) n.set(t, (n.get(t) || 0) + 1);
      for (const [sentence, c] of n) {
        if (c < 2 || DQ0_REPEATS.some(r => r.what.test(sentence) && (!r.tests || r.tests.test(name)))) continue;
        found.push({ test: name, cmd: a.cmd, sentence, n: c });
      }
    }
  }
  return found;
}
// The hub's live chunks (world.h's struct chunk: portals from 219, 14 bytes each, their count at 218; traps from 2513,
// 7 bytes each, their count at 2512), each with two portals or two traps on one square: 'sn:cx,cy: 2 portals at x,y'
function dq0Doubled(m) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b), f = [];
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51);
  if (hub === undefined || !banks[hub + WORLD_BANK_N]) return f;
  const w = banks[hub + WORLD_BANK_N];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * 8, b = banks[hub + 5 + Math.floor(k / 3)];
    if (!w[s] || !b) continue;
    const o = (k % 3) * 0xAA0, at = w[s + 1] + ':' + (w[s + 2] | w[s + 3] << 8) + ',' + (w[s + 4] | w[s + 5] << 8);
    for (const [what, n, from, size, max] of [['portals', b[o + 218], 219, 14, 16], ['traps', b[o + 2512], 2513, 7, 6]]) {
      const on = new Map();
      for (let i = 0; i < n && i < max; i++) { const p = o + from + i * size, sq = b[p] + ',' + b[p + 1]; on.set(sq, (on.get(sq) || 0) + 1); }
      for (const [sq, c] of on) if (c > 1) f.push(at + ': ' + c + ' ' + what + ' at ' + sq);
    }
  }
  return f;
}
function dq0Check(name, m) {
  return [...dq0Repeats(name, m.out).map(r => 'told ' + r.n + ' times in one answer (> ' + r.cmd + '): ' + JSON.stringify(r.sentence)),
    ...dq0Doubled(m).map(s => 'a chunk\'s fixtures doubled: ' + s)];
}

// What a dq0 test's lines did, by their meaning (Phase 3A: tests cut to meaning, not words): sim/lib/dq0meaning.js's
// record (m.dq0, in every dq0 test: the world's events, the denizens', the requests' replies, the state each line
// found as it was entered), a line at a time.  dq0Turns(m): the lines typed (as the serial port gave them, each to its
// CR), each { k, line, game (the game its events are in, 0 on), events, dens, replies, at (the state the line found),
// after (the next line's, or the test's end's) }; T.of(line, from): the first turn of that line from turn from on (or
// one of nothing, its k -1), T.after(turn, line): the first of that line after it.  A meaning test (meaning: true)
// checks only these, and nothing of the ruleset's own (its numbers, its dice's outcomes, its words): the null ruleset
// runs them too (dq0null's agent's)
function dq0Turns(m) {
  const r = m.dq0 || { games: [], lines: [], final: () => null }, all = { events: [], dens: [], replies: [], snaps: [] };   // (None: sim/dl's)
  r.games.forEach((g, gi) => { for (const k of Object.keys(all)) for (const e of g[k]) all[k].push(Object.assign({ game: gi }, e)); });
  const fin = r.final(), of = (list, k) => list.filter(e => e.line === k + 1);
  // (A dq0 line as the test wrote it: sim/test.js's -a, and RULES=NAME's -r NAME, taken off)
  const T = r.lines.map(l => l.replace(/^(dq0(?: .*)?) -a$/, '$1').replace(/^(dq0(?: .*)?) -r \S+$/, '$1').replace(/^(dq0(?: .*)?) -a$/, '$1')).map((line, k) => ({ k, line, events: of(all.events, k), dens: of(all.dens, k), replies: of(all.replies, k),
    at: all.snaps.find(s => s.line === k + 1) || null }));
  T.forEach((t, k) => {
    t.after = k + 1 < T.length ? T[k + 1].at || fin : fin;
    t.game = t.events.length ? t.events[0].game : t.after ? t.after.game : -1;
  });
  const none = { k: -1, line: '', events: [], dens: [], replies: [], at: null, after: null, game: -1 };
  T.of = (line, from = 0) => T.find(t => t.k >= from && t.line === line) || Object.assign({}, none, { line });
  T.after = (t, line) => T.of(line, t.k + 1);
  // (A turn and the one after, as one: a quit with a save folder has its end's events, its save, as the next line's
  // typed, the game's "> " back before it's done)
  T.and = t => { const u = T[t.k + 1]; return t.k < 0 || !u ? t : Object.assign({}, t, { events: [...t.events, ...u.events], dens: [...t.dens, ...u.dens],
    replies: [...t.replies, ...u.replies], after: u.after }); };
  return T;
}
// A turn's events (the world's and the denizens') of a kind, with the fields given (each a value, or a test of it)
const dq0Ev = (t, kind, want = {}) => [...t.events, ...t.dens].filter(e => e.kind === kind &&
  Object.entries(want).every(([k, v]) => (typeof v === 'function' ? v(e[k]) : e[k] === v)));
const dq0Rep = (t, req, want = {}) => t.replies.filter(e => e.req === req && Object.entries(want).every(([k, v]) => (typeof v === 'function' ? v(e[k]) : e[k] === v)));
// A turn's events as words, for a failure's
function dq0Evs(t) {
  if (t.k < 0) return '"' + t.line + '" not typed';
  const one = e => e.kind + ['how', 'why', 'po', 'hc', 'band', 'to', 'tk', 'ir', 'de', 'out'].filter(k => e[k] !== undefined && e[k] !== false).map(k => ' ' + e[k]).join('');
  return '> ' + t.line + ': ' + ([...t.events, ...t.dens.filter(e => e.kind === 'E_FIGHT')].map(one).join(', ') || 'no events');
}
// A check: event kind (want's fields) in turn t, or (never) none; what the failure says
function dq0Need(f, t, kind, want, what) { if (!dq0Ev(t, kind, want).length) f.push(what + ' (' + dq0Evs(t) + ')'); }
function dq0Never(f, t, kind, want, what) { if (t.k >= 0 && dq0Ev(t, kind, want).length) f.push(what + ' (' + dq0Evs(t) + ')'); }
// The things carried (the item store's: on the character, or in what it carries), by kind's name: [{ name, flags, ... }]
function dq0Carried(s) {
  if (!s) return [];
  const on = new Set(), by = new Map(s.items.map(i => [i.n, i]));
  const mine = i => { for (let d = 0; i && d < 8; d++) { if (i.where === 'IW_ACTOR') return i.x === 0; if (i.where !== 'IW_ITEM') return false; i = by.get(i.x); } return false; };
  return s.items.filter(i => mine(i) && !on.has(i.n) && on.add(i.n));
}
// The things lying on the character's square (its stratum's), by name
const dq0Here = s => s ? s.items.filter(i => i.where === 'IW_SQUARE' && i.sn === s.pc.sn && i.x === s.pc.x && i.y === s.pc.y) : [];
const dq0Named = (list, name) => list.filter(i => i.name === name);
// A thing by its kind's name: the one carried, else the one on the character's square or in (or on) one there, else
// any (null: none); and what holds it (the item it's in or on, or null)
function dq0Thing(s, name) {
  if (!s) return null;
  const by = new Map(s.items.map(i => [i.n, i])), all = s.items.filter(i => i.name === name), carried = new Set(dq0Carried(s).map(i => i.n));
  const here = i => { for (let d = 0; i && d < 8; d++) { if (i.where === 'IW_SQUARE') return i.sn === s.pc.sn && i.x === s.pc.x && i.y === s.pc.y; if (i.where !== 'IW_ITEM') return false; i = by.get(i.x); } return false; };
  return all.find(i => carried.has(i.n)) || all.find(here) || all[0] || null;
}
const dq0Holder = (s, i) => s && i && i.where === 'IW_ITEM' ? s.items.find(j => j.n === i.x) || null : null;
// What the character perceives of the place (the senses' record, struct sight), the denizens aside (they go their own
// ways), as one string: the same place seen the same, compared
const dq0View = s => !s || !s.sight ? '' : JSON.stringify(Object.assign({}, s.sight, { seq: 0, dens: [], first: 0 }));
const DQ0_I = { LIT: 0x01, HELD: 0x02, WORN: 0x04, HIDDEN: 0x08 };   // (items.h's I_*)
// A dq0 test's lines typed, each at its prompt (rc's "% ", the game's "> ")
const dq0Typed = lines => lines.map(l => '\u0101' + l + '\r').join('');

// Dingus Quest Zero's answers timed (dq0answer's): each command's Enter (the CR as the ACIA has it) to its answer's first
// letter in the story (want: the words it begins with), and to the story's last letter before the prompt ("> " on
// the line typed's row) comes back; the status line (reverse video) and the escapes not the story's.  dq0Timer(m),
// as the machine's made, listens (each byte's cycle, each CR's), and gives a function of the lines typed and of cmds
// ({ line, want }: each the line's first time typed after the one before) giving { line, first, whole } for each
function dq0Timer(m) {
  const at = [], enters = [];
  let out = m.out;
  for (let i = 0; i < out.length; i++) at.push(0);
  Object.defineProperty(m, 'out', { get: () => out, set: v => { const c = m.cpu.cyc; for (let i = out.length; i < v.length; i++) at.push(c); out = v; } });
  const a = m.acia, read = a.read;
  a.read = (r, t) => { if (r === 0 && a.rdrf && a.rx === 13) enters.push({ t: a.rxAt, pos: out.length }); return read(r, t); };
  return (lines, cmds) => {
    const res = [];
    let li = 0;
    for (const c of cmds) {
      while (li < lines.length && lines[li] !== c.line) li++;
      const e = enters[li], e2 = enters[li + 1];
      li++;
      if (!e) { res.push({ line: c.line, first: null, whole: null }); continue; }
      const end = e2 ? e2.pos : out.length;
      let story = '', idx = [], rev = 0, prompt = -1;
      for (let i = e.pos; i < end; i++) {
        if (out[i] === '\x1b') {
          const q = /^\x1b\[([0-9;?]*)([A-Za-z])/.exec(out.slice(i, i + 12));
          if (q) {
            if (q[2] === 'm') rev = q[1] === '7';
            if (q[2] === 'H' && /;1$/.test(q[1]) && q[1] !== '1;1' && /^\x1b\[K(\x1b\[\?25h)?> /.test(out.slice(i + q[0].length, i + q[0].length + 12)) &&
              story.includes(c.want) && prompt < 0) prompt = idx.length;
            i += q[0].length - 1;
            continue;
          }
        }
        if (rev || out[i] < ' ') continue;
        story += out[i];
        idx.push(i);
      }
      const w = story.indexOf(c.want);
      let last = prompt < 0 ? idx.length - 1 : prompt - 1;
      while (last > 0 && story[last] === ' ') last--;
      res.push({ line: c.line, first: w < 0 ? null : at[idx[w]] - e.t, whole: w < 0 || last < w ? null : at[idx[last]] - e.t });
    }
    return res;
  };
}

// dq0answer's commands: at seed 3 (-T: by the way down), the turns pace, then the normal pace; each a line, the words
// its answer begins with, and at 115,200 baud its budgets (the user's targets, D17), its first letter's and its
// whole's, from Enter: a turn's first words (as it begins, V_FACING) and a look's under half a second, 1.79M; what's
// carried, a door examined and a step that can't be taken a fifth, 720,000.  (Not a step's or a walk's, their answer
// the look as they get there: at the turns pace the denizens given their turn a slice at a time through the time the
// move takes, at the normal pace the move taking its own time, a step a second, then the senses' look)
const DQ0_ANSWERS = [['turn left', 'You turn to face', 1790000], ['turn right', 'You turn to face', 1790000],
  ['step forward', 'You'], ['inventory', 'You carry', 720000], ['examine the door', 'A door', 720000],
  ['look', 'You', 1790000, 1790000], ['go up', 'There\'s no way up or down here.', 720000],
  ['walk to the door ahead to my left', 'You']];
function dq0AnswerTest(baud) {
  const wait = +(process.env.DQ0ANS_WAIT || 0);   // (Each line typed after 0-2 waits of 2M cycles: the timing moved, for an average)
  const games = ['dq0 -p t -s 3 -c /ram/c.hm -T', 'dq0 -p n -s 3 -c /ram/c.hm -T'];
  const lines = ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', ...(baud === 115200 ? ['echo b115200 >/dev/serctl'] : []),
    ...games.flatMap(g => [g, ...DQ0_ANSWERS.map(c => c[0]), 'quit']), 'echo $status'];
  return {
    name: baud === 115200 ? 'dq0answer' : 'dq0answer96', what: 'Dingus Quest Zero\'s answers timed at ' + baud + ' baud (the user\'s D17: the responsiveness targets at 115,200; a command answered in a fifth of a second, 720,000 cycles, a look in half, 1.79M): from Enter to the answer\'s first letter, and to its last, for a turn left and right at seed 3, a step, what the character carries, a door examined, a look, a step that can\'t be taken and a walk, at the turns pace and the normal pace',
    meaning: true,
    init: 't_rc', cycles: 1500e6,
    machine: { input: lines.map((l, i) => '\u0101' + '\u0100'.repeat(wait * (i + 1) % 3) + l + '\r').join('') },
    expect: ['% echo $status'],
    start(m) { this.timer = dq0Timer(m); },
    check(m, out) {
      const f = [], notes = this.notes = [], li = lines.indexOf(games[1]);
      const over = (r, name, what, v, max) => {
        if (baud === 115200 && max && !(v !== null && v <= max)) f.push(name + ' ' + r.line + ': ' + what + ' ' + v + ' cycles from Enter (budget ' + max + ')');
      };
      const pace = (from, to, name) => this.timer(lines.slice(0, to).map((l, i) => i < from ? '' : l), DQ0_ANSWERS.map(c => ({ line: c[0], want: c[1] }))).forEach((r, k) => {
        notes.push(name + ' ' + r.line + ': first letter ' + r.first + ', whole ' + r.whole);
        over(r, name, 'its first letter', r.first, DQ0_ANSWERS[k][2]);
        over(r, name, 'its whole', r.whole, DQ0_ANSWERS[k][3]);
      });
      pace(0, li, 'turns');
      pace(li, lines.length, 'normal');
      return f;
    },
    sdcard: true
  };
}

module.exports = {
  IRQ_OFF_MAX,
  tests: [
    {
      name: 'boot', what: 'the kernel boots, POST finds nothing wrong; init runs hello and waits for it',
      init: 'init', cycles: 20e6,
      expect: ['Hydra-16: kernel 0.1, ABI 1', 'HydraOS 1.0 for the Hydra-16', 'POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0',
        'RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000', 'POST ok', 'RAM modules: 02',
        'task F: cons', 'task 1: init', 'init: up in task 01', 'hello, from init', 'init: hello ended: code $07 (bye)'],
    },
    {
      name: 'ser', what: 'the base\'s console driver (../base/modules/ser) in cons\'s place: init\'s console; rc\'s lines on it, cooked (Backspace, Enter; Ctrl-D the end of cat\'s input); Ctrl-C a note to the console\'s group; consctl\'s and serctl\'s states; /ser written: what a program written for the base sees is a part of cons\'s',
      init: 'init', without: ['cons'], modules: ['ser'], cycles: 200e6,
      // (ā: wait for a prompt; Ā: 2M cycles, sleep running; \b twice takes xo back)
      machine: { input: 'ārc\r' + 'āsleep 30\rĀ\x03' + 'āecxo\b\bho edited\r' + 'ācat\rline one\r\x04' + 'ācat /dev/consctl /dev/serctl\r' + 'āecho to ser >/dev/ser\r' },
      expect: ['hello, from init', '% sleep 30\n', '\nedited\n%', '% cat\nline one\nline one\n%', '\nrawoff\ngroup 1\nb9600\n%', '\nto ser\n%'],
    },
    {
      name: 'init', what: 'init from files (rc the shell, a card\'s /lib/shell naming it): the RAM disks started, the namespace file run, each shell\'s own namespace and /ram (a window\'s too); the shared RAM disk stopped, /bin\'s union still there',
      init: 'init', modules: ['t_child'], cycles: 250e6,
      // (ā: wait for a prompt; '#fr' quoted, as # starts a comment; \x1d c: Ctrl-] c, a window made, init started
      // there by the kernel (INIT_RUN) and rc in its place.  /bin: the RAM disks' caches (empty), then #m/bin, whose
      // t_child runs by its name.  t_child f makes /ram/mark: in window 0's rc's area, 1 (init's task: rc is in its
      // place), and not in window 1's rc's, 2)
      get machine() { return { sd: shellCard('/bin/rc -l', 'shellrc'), input: 'āls \'#fr\'\r' + 'āls /ram\r' + 'āls /bin\r' + 'āt_child f\r' + 'āls \'#fr\'/1\r' + 'ācat /rom/lib/profile\r' +
        'ācat /dev/sd/s/ctl\r' + 'āecho $window\r' + 'ā\x1dc' + 'āecho $window\r' + 'āls \'#fr\'\r' + 'āls /ram\r' + 'āls /dev\r' +
        'āecho stop >>\'#d/s/ctl\'; echo still; cat /sram/x\r' }; },
      expect: ['% ls \'#fr\'\n1/\n%', '% ls /ram\nbin/\nlib/\n%',
        '% ls /bin\n' + ROM_BIN().join('\n') + '\ninit\nrc\nplay\n', 't_child\n% t_child f\n', '% ls \'#fr\'/1\nbin/\nlib/\nmark\n%',
        'prompt=(', '% cat /dev/sd/s/ctl\nsram 512 KB 1024 blocks\nhydrafs label=SRAM\n', '% echo $window\n0\n%',
        '% echo $window\n1\n%', '% ls \'#fr\'\n1/\n2/\n%', '% ls /ram\nbin/\nlib/\n%', '\ncons\nconsctl\nwctl\nwnew\nser\nserctl\nkbdin\ntext\nlabel\nsnarf\nkbin\n%',
        '% echo stop >>\'#d/s/ctl\'; echo still; cat /sram/x\nstill\ncat: /sram/x: no such device\n%'],
    },
    {
      name: 'newns', what: 'the default namespace\'s library (nslib): an old area emptied, a namespace file run (quotes, comments, $task, flags, bad lines); the whole namespace built twice: the area emptied while it isn\'t /ram, kept once it is (a shell\'s newns lost its /ram\'s files)',
      init: 't_newns', cycles: 60e6,
      check(m, out) {
        const f = [];
        for (const l of ['newns: frob /dev: invalid argument', 'newns: bind /tmp: invalid argument'])
          if (!out.includes(l + '\n')) f.push('not said: ' + l);
        if (/newns: .*(null zero|mount|bind -b|bind '#n')/.test(out)) f.push('a good line said as a bad one');
        return f;
      },
    },
    {
      name: 'post-t', what: 'POST: a T line stuck low (U7)',
      init: 'init', cycles: 3e6, machine: { u7Fault: { mask: 4, high: false } },
      expect: ['POST ZP:4 ST:4 OS:4 HI:4 SH:S W:0', 'POST found faults: T lines'],
    },
    {
      name: 'post-zp', what: 'POST: only the zero page and stack per task (a decoding fault)',
      init: 'init', cycles: 3e6, machine: { model: 'zponly' },
      expect: ['POST ZP:0 ST:0 OS:F HI:F', 'POST found faults: T lines'],
    },
    {
      name: 'post-ram', what: 'POST: an address line of RAM module 1 stuck low; the module is left unused',
      init: 'init', cycles: 3e6, machine: { ramFault: { bank: 0x10, mask: 0x100, high: false } },
      expect: [' 00:0/00/0000 10:0/00/0100', 'POST found faults: RAM (left unused)'],
    },
    {
      name: 'post-sh', what: 'POST: no shared RAM',
      init: 'init', cycles: 3e6, machine: { model: 'noshared' },
      expect: ['SH:X', 'POST found faults: shared RAM RAM (left unused)'],
    },
    {
      name: 'hwtest', what: 'a T typed during POST starts the hardware test (paged ROM bank 1)',
      init: 'init', cycles: 3e6, machine: { input: 'T' },
      expect: ['Hydra-16 hardware test'],
    },
    {
      name: 'hwtool', what: 'hwtest at rc: the system starts again (REBOOT), and POST takes REBOOT_HWTEST\'s word as a T',
      init: 'init', cycles: 60e6, get machine() { return { sd: shellCard('/bin/rc -l', 'shellrc2'), input: 'āhwtest\r' }; },
      expect: ['% hwtest\nhwtest: the system starts again, into the hardware test\n', 'HydraOS 1.0', 'Hydra-16 hardware test'],
    },
    {
      name: 'task', what: 'tasks and the scheduler: SPAWN, EXITS, WAIT, SLEEP, preemption, PAUSE and WAKE, orphans',
      init: 't_task', modules: ['t_child'], without: ['cons', 'storage', 'snd', 'vid'], cycles: 60e6,
    },
    {
      name: 'dev', what: 'the kernel\'s devices (kdev): #/, #n, #t, #m, #p; pipes; a union keeping what was there',
      init: 't_dev', modules: ['t_child'], cycles: 40e6,
    },
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
      expect: ['% basic\n> PRINT "Hello, World"; 2 + 3 * 4; 10 / 4; 2 ^ 70\nHello, World 14  5/2  1180591620717411303424 \n> ',
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
        '% basic -l\n/ram> print 1 + 1\n 2 \n/ram> echo hello from rc\nhello from rc\n/ram> x = 5\n/ram> ? x * 2\n 10 \n/ram> ls /rom/lib/basic\nprofile.bas\n/ram> cd /rom\n/rom> ',
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
        const f = pcReport(m, 1, 0, 0), mult = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
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
      name: 'bootsize', what: 'the boot with a terminal that answers the size asked at once (ESC [ 18 t: the PC tool does), not 80 x 24: the console\'s repaint for it waits till init has the console (the kernel\'s own "task 1: init" is still going out when the answer comes), so no byte is written over another on the serial chip, the kernel\'s line is whole, and the screen is cleared before HydraOS\'s first line',
      init: 'init', cycles: 60e6,
      machine: { termSize: [30, 100], input: 'āecho booted\r' },
      expect: ['\x1b[18ttask 1: init\n', '\x1b[H\x1b[2JHydraOS 1.0 for the Hydra-16\n', '\nbooted\n'],
      check(m) { return m.acia.overruns || m.acia.txLost ? [m.acia.overruns + ' bytes written while another was still being sent (' + m.acia.txLost + ' lost on a WDC chip)'] : []; },
    },
    {
      name: 'standing', what: 'the tasks kept standing: at the login shell\'s prompt the kernel, the shell in init\'s task (init EXECs it) and the drivers, no init, no window starter, no input program, no gpio driver; the console waiting for its own events (its init\'s loop); a program left behind when its shell ends is nobody\'s; the login shell ended is started again by the kernel, in task 1, window 0\'s; a window made while the shell waits for a program (Ctrl-] c: the console\'s own loop makes it, the kernel starts init there); newns twice, the namespace the same, and a file in /ram still there after it',
      init: 'init', cycles: 520e6,
      machine: { input: 'āps\r' + 'āecho mine >/ram/kept\r' + 'āns | wc -l\r' + 'ānewns\r' + 'āns | wc -l\r' + 'ācat /ram/kept\r' + 'āsleep 200 &\r' + 'ābye\r' + 'āps\r' + 'āecho $window\r' + 'āsleep 8\rĀĀ\x1dc' + 'āecho $window\r' + 'āps\r' + 'āecho all done\r' },
      expect: ['\nall done\n', '/> ns | wc -l\n     26\n/> newns\n/> ns | wc -l\n     26\n/> cat /ram/kept\nmine\n', '/> bye\nHyForth (Forth 2012), bye to end\n/> ps\n', '/> echo $window\n0\n', '/> echo $window\n1\n'],
      check(m, out) {
        const f = [], text = out.replace(/\r/g, ''), lists = text.split('task  state   parent     cpu group  name\n').slice(1).map(t => t.split('\n/> ')[0].split('\n').map(l => l.trim().split(/\s+/)));
        const names = l => l.map(r => r[0] + ':' + r[5] + ':' + r[2]).join(' ');
        if (lists.length !== 3) return ['ps listed the tasks ' + lists.length + ' times, not 3'];
        if (names(lists[0]) !== '0:kernel:- 1:forth:- 12:snd:- 13:kdev:- 14:storage:- 15:cons:-') f.push('at the first prompt: ' + names(lists[0]));   // (ps run in the shell's task: RUN)
        if (lists[0].find(r => r[5] === 'cons')[1] !== 'event') f.push('cons isn\'t waiting for its events: ' + lists[0].find(r => r[5] === 'cons')[1]);
        if (names(lists[1]) !== '0:kernel:- 1:forth:- 2:sleep:- 12:snd:- 13:kdev:- 14:storage:- 15:cons:-') f.push('after the login shell ended: ' + names(lists[1]));
        if (!/ 1:forth:- 2:sleep:- 3:forth:- 12:/.test(' ' + names(lists[2]) + ' ')) f.push('in the window made: ' + names(lists[2]));   // (sleep 8 in task 1's, ps in 3's)
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
    {
      name: 'spi', what: 'SPI and #S (storage): transactions, kept bytes, modes 0 and 3, one open at a time, the time a byte takes',
      init: 't_spi', cycles: 30e6, machine: { spiEcho: [3, 9] },
      // (The bit loops: 18 cycles a bit in, 33 out; the rest is the request and the copy to or from the client.  At
      // 7.16 MHz the receive loop is padded, 8 cycles a bit, to keep SCLK under an SD card's 400 kHz)
      budgets: [{ what: 'SPI, 256 bytes clocked in (READ), a byte', from: '<rx', to: 'rx>', minus: ['<b0', 'b0>'], per: 256,
        max: o => o.clock === 2 ? 280 + 64 : 280 },
        { what: 'SPI, 256 bytes sent (WRITE), a byte', from: '<tx', to: 'tx>', minus: ['<b0', 'b0>'], per: 256, max: 430 }],
    },
    {
      // (A card: the bit loops, 144 cycles a byte in; the old system's was 298 cycles a byte, in 256-byte reads)
      name: 'disk', what: 'the disks (storage): #d, the ROM disk, SD cards (SDHC and SDSC), RAM disks, their ctl files, the time a byte takes',
      init: 't_disk', cycles: 60e6, machine: { sd: DISK_CARDS, spiEcho: [3] },
      // (A card's block read is kept in the storage driver's cache too: about 16 cycles a byte more)
      budgets: [{ what: 'a card, 4096 bytes read (8 blocks), a byte', from: '<card', to: 'card>', minus: ['<b0', 'b0>'], per: 4096,
        max: o => o.clock === 2 ? 300 + 64 : 300 },
        { what: 'the same again, from the cache, a byte', from: '<hit', to: 'hit>', minus: ['<b0', 'b0>'], per: 4096, max: 80 },
        { what: 'a RAM disk, 4096 bytes read, a byte', from: '<ram', to: 'ram>', minus: ['<b0', 'b0>'], per: 4096, max: 70 }],
      check() {
        const f = [], c0 = DISK_CARDS[0].data, c1 = DISK_CARDS[1].data;
        const has = (d, at, text) => [...text].every((ch, i) => d[at + i] === ch.charCodeAt(0));
        if (!has(c0, 508, '0123456789')) f.push('card 0: not 0123456789 at 508 (across blocks 0 and 1)');
        if (!c0.subarray(4096, 4608).every(b => b === 0x5A)) f.push('card 0: block 8 not all Z');
        if (!has(c0, 2048 * 512 - 2, 'en')) f.push('card 0: not "en" at its end');
        if (c0[512 + 10] !== ((7 + 10) & 0xFF)) f.push('card 0: block 1 changed past the write');
        if (!has(c1, 5 * 512 + 10, 'sdsc')) f.push('card 1: not sdsc at byte 10 of block 5');
        return f;
      },
    },
    {
      name: 'fs', what: 'HydraFS (#f): files and directories, create, write, holes, remove, rename, a length, format, label, check, old cards, mounts',
      init: 't_fs', cycles: 600e6,
      get machine() { this.cards = fsCards(); return { sd: this.cards }; },
      budgets: [{ what: 'a HydraFS file, 8192 bytes read from a card (512 a READ), a byte', from: '<file', to: 'file>', minus: ['<b0', 'b0>'],
        per: 8192, max: o => o.clock === 2 ? 290 + 64 : 290 }],
      check() {
        const f = [];
        for (const c of this.cards) c.save();
        const text = (v, p) => { const e = v.tryWalk(p); return e ? v.read(e).toString('latin1') : null; };
        const each = (i, what) => { const v = new hydrafs.Volume(this.cards[i].file); for (const p of v.check()) f.push('card ' + i + ': ' + p); what(v); v.close(); };
        each(0, v => {
          if (text(v, 'renamed.txt') !== 'trunc') f.push('card 0: renamed.txt isn\'t "trunc"');
          if (v.tryWalk('new.txt') || v.tryWalk('dir')) f.push('card 0: new.txt or dir is still there');
          if (text(v, 'hello.txt') !== 'hello, hydrafs\n') f.push('card 0: hello.txt changed');
        });
        each(1, v => {
          if (v.label !== 'NEW NAME') f.push('card 1: its label is "' + v.label + '"');
          if (v.version !== 1) f.push('card 1: version ' + v.version + ' (a full format makes version 1)');
          if (text(v, 'a.txt') !== 'on card 1') f.push('card 1: a.txt isn\'t "on card 1"');
        });
        for (const i of [2, 3]) each(i, v => { if (text(v, 'hydra.txt') !== 'from reborn') f.push('card ' + i + ': hydra.txt isn\'t "from reborn"'); });
        each(4, () => {});
        each(5, () => {});
        each(6, v => {
          if (v.label !== 'BIG' || v.version !== 2) f.push('card 6: label "' + v.label + '", version ' + v.version + ' (BIG, 2)');
          if (text(v, 'big.txt') !== 'on a big card') f.push('card 6: big.txt isn\'t "on a big card"');
        });
        return f;
      },
    },
    {
      name: 'rom', what: 'the ROM disk: /rom (#f, spec x) walked on the Hydra, every file read back against its source (romfs/romfs.txt)',
      init: 't_rom', cycles: 300e6,                         // (the walk takes some 240M)
      check(m, out) {
        const romfs = require('../tools/romfs.js'), { crc16 } = require('../tools/romimg.js');
        const files = romfs.manifest(path.join(__dirname, '..', 'romfs', 'romfs.txt')), seen = new Map(), f = [];
        for (const l of out.split('\n')) {
          const k = l.match(/^rom: (\S+) ([0-9A-F]{4}) ([0-9A-F]{4})$/);
          if (k) seen.set(k[1], { size: parseInt(k[2], 16), crc: parseInt(k[3], 16) });
        }
        for (const x of files) {
          const p = '/rom' + x.path, s = seen.get(p), crc = crc16(i => x.data[i], x.data.length);
          if (!s) { f.push(p + ': not found on the Hydra'); continue; }
          seen.delete(p);
          if (s.size !== (x.data.length & 0xFFFF) || s.crc !== crc)
            f.push(p + ': read back as ' + s.size + ' bytes, CRC ' + s.crc.toString(16) + '; its source (' + x.src + ') is ' + x.data.length + ', ' + crc.toString(16));
        }
        for (const p of seen.keys()) f.push(p + ': on the Hydra, but not in the manifest');
        this.notes = ['the ROM disk: ' + files.length + ' files read back on the Hydra, each as its source'];
        return f;
      },
    },
    {
      name: 'load', what: 'SPAWN by path and the loader: modules in place (#m/bin), RAM programs from a card (arguments, fd maps), errors',
      init: 't_load', modules: ['t_child'], cycles: 200e6,
      get machine() { return { sd: loadCard() }; },
      // (A name through /bin looks in the card's bin first: its directories come from the storage driver's cache)
      get budgets() {
        const n = BIG_LENGTH();
        return [{ what: 'a RAM program loaded from a card (t_big, ' + n + ' bytes: SPAWN to its first instruction), a byte', from: '<big', to: 'big>',
          per: n, max: o => o.clock === 2 ? 320 + 64 : 320 },
        { what: 'the same from the RAM disk, a byte', from: '<rbig', to: 'rbig>', per: n, max: 90 },
        // (A mark is seen as its text goes out on the line, at the boot's 9600: so a time here moves in steps of a
        // character's, some 3,700 cycles, as the work before it shifts by a few.  SPAWN's own: about 45,000.  It grows
        // with the ROM's modules, a hundred cycles or more each (October 2026: basic 169, as 106; the PSG's bigger snd,
        // vid and play 27, from reborn's 48,991 to 49,018), so 50,000)
        { what: 'SPAWN of a module in place (#m/t_child), the caller\'s time', from: '<msp', to: 'msp>', minus: ['<b0', 'b0>'],
          per: 1, max: 50000 },
        { what: 'the same by /bin/t_child (the card\'s bin first, then #m/bin)', from: '<sp', to: 'sp>', minus: ['<b0', 'b0>'], per: 1,
          max: 110000 }];
      },
    },
    {
      name: 'rc', what: 'rc: quoting, lists, redirections, pipelines, if, for, while, switch, functions, globs, scripts, Ctrl-C, its start',
      init: 't_rc', cycles: 400e6,
      // (Each line typed at its prompt: its output, then the next prompt.  Then a command of three lines, each after
      // the one before has been read (\u0100: a moment), and cat, waiting for input, interrupted by Ctrl-C)
      get machine() {
        return { input: RC_LINES.map(l => '\u0101' + l[0] + '\r').join('') + '\u0101if(~ a a){\r\u0100echo multi\r\u0100}\r' +
          '\u0101cat\r\u0100\x03\u0101echo $status\r' };
      },
      get expect() {
        return [...RC_LINES.map(l => '% ' + l[0] + '\n' + l[1] + '\n%'), '% if(~ a a){\n\techo multi\n\t}\nmulti\n%',
          '\n% echo $status\ninterrupt\n%'];
      },
      // (t_rc's, before rc -l starts: rc's own start and end, and ls /bin through its union (the RAM disks' empty
      // caches, the ROM disk's bin, then #m/bin), into #n/null: for each program it shows, as it grows with them)
      get budgets() {
        const n = BIN_COUNT();
        return [{ what: 'rc -c \'x=1\': SPAWN to its end', from: '<rc', to: 'rc>', minus: ['<b0', 'b0>'], per: 1, max: 145000 },
          { what: 'ls /bin (the caches, /rom/bin, #m/bin: ' + n + ' programs): SPAWN to its end, a program', from: '<ls', to: 'ls>',
            minus: ['<b0', 'b0>'], per: n, max: 27000 }];   // (Most from the ROM disk's bin: a little more each)
      },
    },
    {
      name: 'tools', what: 'the core tools at rc: files, text, tasks, the disks\' (/rom/bin); /proc\'s args, cwd, ns; the SDK\'s samples',
      init: 't_rc', modules: ['counter'], cycles: 400e6,
      // (Each line typed at its prompt: its output (null: none; a third element true: how it starts), then the next
      // prompt.  Then top, for 2 seconds or so, and Ctrl-C; more, its --more-- answered with Enter; and the SDK's tick, and
      // Ctrl-C: its handler's, not the default)
      get machine() {
        return { input: TOOL_LINES.map(l => 'ā' + l[0] + '\r').join('') + 'ātop\rĀĀĀĀ\x03' +
          'āecho $status\r' + 'ācat /ram/n /ram/n /ram/n | more\rĀĀ\r' + 'ā/sd/0/sample/tick\rĀĀĀĀĀ\x03' + 'āecho $status\r' };
      },
      get expect() {
        return [...TOOL_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')),
          '\x1b[H\x1b[2Jtask  state    cpu  name\n', '% echo $status\ninterrupt\n%',
          '\n9\n10\n--more--\n11\n12\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n%', '% /sd/0/sample/tick\n.', ' seconds\n\n% echo $status\n\n%'];
      },
      sdcard: true
    },
    {
      name: 'c', what: 'the C target (cc65): its samples at rc, the library\'s test (ctest), conio\'s raw keys (an Escape alone too; and raw ended with the program)',
      init: 't_rc', cycles: 220e6,
      // (Each line typed at its prompt, as the tools test's.  Then keys: three keys and q; and again, ended by Ctrl-C,
      // its window cooked again for rc)
      get machine() {
        return { input: C_LINES.map(l => 'ā' + l[0] + '\r').join('') + 'ā/sd/0/sample/c/keys\rĀĀab\x1b[A\x1bĀq' +
          'ā/sd/0/sample/c/keys\rĀĀ\x03' + 'āecho $status\r' };
      },
      get expect() {
        return [...C_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')),
          '\nctest: 0 failed\n%', 'codes:\x1b[27m 61 62 80 1B\nended at 18,2\n%', '% echo $status\ninterrupt\n%'];
      },
      sdcard: true
    },
    {
      name: 'dq0', meaning: true, what: 'Dingus Quest Zero (games/dq0) at rc, its turns pace, by what happens (the hub\'s events and state: sim/lib/dq0meaning.js, tests.js\'s dq0Turns): a character from hmchar\'s file (-c) begins in a room by the way out; commands typed ahead (each waits for its prompt): the way out shut, a shut door in the way, a door opened (turned to first), walked through to a passage, another opened and a room come into the first time, crossed, no stairs there, back to the first room; west and go west the same (no way there); the map, the help, a word the game doesn\'t know and a look taking no time, a wait a round; quit, the game\'s end (the ledges: dq0harm; the game running hmchar itself: dq0veraserial)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'look', 'u', 'n',
        'open the door behind me on my right', 'n', 'open', 'n', 'e', 'u', 's', 'w', 'go west', 'map', 'help', 'foo', 'look', 'wait', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], g = T.of('dq0 -p t -s 3 -c /ram/c.hm'), s = g.after;
        dq0Need(f, g, 'E_ARRIVE', { how: 'A_START' }, 'the game not begun');
        if (!s || !s.sight || s.sight.where !== 0 || !s.sight.open.some(o => o.kind === 'O_WAY_OUT')) f.push('not begun in a room by the way out');
        const u = T.of('u'), n = T.of('n'), o = T.of('open the door behind me on my right');
        dq0Need(f, u, 'E_NOWAY', { why: 'N_SHUT' }, 'the way out not shut');
        dq0Need(f, n, 'E_NOWAY', { why: 'N_DOOR' }, 'no shut door in the way');
        dq0Need(f, o, 'E_PORTAL', { po: 'PO_OPENED' }, 'the door behind not opened');
        if (o.at && o.after && o.at.pc.face === o.after.pc.face) f.push('not turned to the door behind');
        const n2 = T.after(o, 'n'), op = T.after(n2, 'open'), n3 = T.after(op, 'n'), e = T.after(n3, 'e'), u2 = T.after(e, 'u'), s2 = T.after(u2, 's');
        dq0Need(f, n2, 'E_ARRIVE', {}, 'not through the door');
        if (n2.at && n2.after && !(n2.after.pc.y < n2.at.pc.y)) f.push('not gone north');
        dq0Need(f, op, 'E_PORTAL', { po: 'PO_OPENED' }, 'the next door not opened');
        dq0Need(f, n3, 'E_ARRIVE', { first: true }, 'no room come into the first time');
        dq0Need(f, e, 'E_ARRIVE', {}, 'the room not crossed');
        if (e.at && e.after && !(e.after.pc.x > e.at.pc.x)) f.push('not gone east');
        dq0Need(f, u2, 'E_NOWAY', { why: 'N_NO_STAIRS' }, 'up with no stairs not refused');
        dq0Need(f, s2, 'E_ARRIVE', {}, 'not back south');
        if (!s2.after || !s2.after.sight || !s2.after.sight.open.some(o => o.kind === 'O_WAY_OUT')) f.push('not back by the way out');
        for (const l of ['w', 'go west']) dq0Need(f, T.after(s2, l), 'E_NOWAY', { why: 'N_NO_EXIT' }, l + ': not refused (no way)');
        for (const l of ['map', 'help', 'foo', 'look']) {
          const t = T.after(s2, l);
          if (t.k < 0 || t.events.length || !t.at || !t.after || t.after.now !== t.at.now) f.push(l + ': not a moment\'s (' + dq0Evs(t) + ')');
        }
        const w = T.of('wait');
        dq0Need(f, w, 'E_TIME', {}, 'no time waited');
        if (w.at && w.after && !(w.after.now > w.at.now)) f.push('the clock not on after a wait');
        dq0Need(f, T.of('quit'), 'E_BYE', {}, 'the game not ended');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0deep', meaning: true, what: 'Dingus Quest Zero down a way to the stratum below (dq0 -T: the character begins at the first way down of the top stratum), by the world\'s events, what the character perceives and the hub\'s cache: the stratum below grown in outline as the character first takes the way, its first chunk made (natural, at this seed: caves), come into the first time, mapped (the maker\'s map), and along a tunnel; the caves irregular (the user\'s, 2026-10-16: "not straight tunnels, not rectangular rooms"): the cache\'s chunks read, their chambers ragged, their tunnels bending, every open square reached, their edges met',
      init: 't_rc', cycles: 500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'd', 'map', 'sw', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], d = T.of('d'), mp = T.of('map'), sw = T.of('sw');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN', first: true }, 'not down to the stratum below, the first time');
        if (!d.after || d.after.pc.sn !== 1 || !d.after.sight || d.after.sight.stratum !== 1) f.push('not on the stratum below');
        if (!mp.after || !mp.after.map.some(r => r.trim())) f.push('no map drawn');
        dq0Need(f, sw, 'E_ARRIVE', {}, 'not along the tunnel');
        return [...f, ...caveReport(m, 1, 1)];
      },
      sdcard: true
    },
    {
      name: 'dq0dig', meaning: true, what: 'Dingus Quest Zero digging (the referee\'s: the tools equipped, P_DIG by what\'s dug, Mining tests a spell at a time, HarnMaster\'s; the maker\'s materials: a room\'s floor, then the stratum\'s rock), by the world\'s events: a skilled miner (Mining 95, hmchar\'s file) with a pick (dq0 -k pick -T: the tool the referee takes) digs a shaft down from a room by its stairway, spell by spell, through to the stratum below, and climbs down it (grown round it then); the giant rats by the stairway living through every spell of it (the turns pace: hours of it, the denizens kept up a slice at a time); a shovel digs seed 1\'s hard-packed earth floor (untested: the tool\'s rate, a foot and a half a spell) till the rock under it stops it (the user: the floor, then the rock); a sword won\'t dig seed 5\'s flagged floor at all (only a pick lifts flags); bare hands (the dagger in the kit, a knife that would dig it, put down first) make no progress in seed 1\'s hard-packed earth (the user\'s, 2026-10-20: hands only in soft soil) (dq0earth\'s and dq0tools\' digs, these together)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/m.hm', 'echo ml min 95 >> /ram/m.hm',
        'dq0 -p t -s 3 -c /ram/m.hm -k pick -T', 'dig down', 'd', 'quit', 'dq0 -p t -s 1 -c /ram/c.hm -k shovel -T', 'dig down', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T', 'dig down', 'quit', 'dq0 -p t -s 1 -c /ram/c.hm -T', 'drop the dagger', 'dig down', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], digs = T.filter(t => t.line === 'dig down');
        if (digs.length !== 4) return ['the four digs not typed'];
        const spells = t => dq0Ev(t, 'E_DIG', { how: h => ['D_GOOD', 'D_POOR', 'D_SLIP'].includes(h) }), dug = t => spells(t).filter(e => e.how !== 'D_SLIP').length;
        // (The tools the referee took for the dig's spells, the hub's dig_tool: the one carried among them)
        const tool = (t, n) => { if (!t.after || !t.after.digTool.includes(n)) f.push(t.k + ': the referee\'s tools ' + (t.after && JSON.stringify(t.after.digTool)) + ', not the ' + n); };
        const [p, s1, sw, h1] = digs;
        dq0Need(f, p, 'E_DIG', { how: 'D_BEGUN' }, 'the pick\'s dig not begun');
        if (!dug(p)) f.push('the pick dug nothing');
        dq0Need(f, p, 'E_DIG', { how: 'D_THROUGH' }, 'the shaft not through');
        tool(p, 'pick');
        const rats = s => s ? s.actors.filter(a => a.state && a.name === 'giant rat').length : 0;
        if (!rats(p.at) || rats(p.after) !== rats(p.at)) f.push('the rats by the stairway not all living through the dig: ' + rats(p.at) + ', then ' + rats(p.after));
        const d = T.after(p, 'd');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN', first: true }, 'not down the shaft to the stratum below');
        if (!d.after || d.after.pc.sn !== 1) f.push('not on the stratum below');
        if (dug(s1) !== 2) f.push('the shovel dug the earth floor\'s two feet in ' + dug(s1) + ' spells, not 2 (a foot and a half a spell)');
        dq0Need(f, s1, 'E_DIG', { how: 'D_NO_TOOL', feet: 1 + 1 }, 'the shovel not stopped by the rock (MAT_STONE)');
        tool(s1, 'shovel');
        dq0Need(f, sw, 'E_DIG', { how: 'D_NO_TOOL' }, 'the sword not refused by the flags');
        if (spells(sw).length) f.push('the sword dug at the flags');
        if (spells(h1).length) f.push('bare hands dug at the hard-packed earth');
        dq0Need(f, h1, 'E_DIG', { how: 'D_NO_TOOL', feet: 0 + 1 }, 'bare hands not refused by the hard-packed earth (MAT_EARTH)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dug', meaning: true, what: 'Dingus Quest Zero\'s holes dug part way kept (slice 4.0, HOUSE.md\'s what stopping costs: "a hole half dug stays half dug"; the world\'s change records, DUG: a hole\'s half-feet dug and the way it\'s dug, looked up as a dig begins on its square), by the world\'s events, its change records and its long task: at seed 3 by the stairway (dq0 -T -d, a pick, the character no miner) the wall to the south dug a while and stopped ("stop": its feet dug a DUG record, heading south), then dug again, on from where it was left (E_DIG\'s D_AGAIN, its feet to go the last spell\'s), through; a shaft begun beside the stairway, dug a while, stopped (a DUG record, down); the game kept (quit) and carried on (dq0 -d: the record read back from its region\'s file), the shaft dug again on from where it was left, its bar too (the hub\'s pc_task: its part done the record\'s and more)',
      init: 't_rc', cycles: 1500e6,
      // (A "stop" typed as the dig goes, after a wait: \u0100's 2M cycles each, a spell or so of the wall's, a few of the
      // shaft's, the stratum below grown first)
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T -d /ram/dug',
        'dig south', '!' + '\u0100'.repeat(12) + 'stop', 'dig south', 'dig down', '!' + '\u0100'.repeat(30) + 'stop', 'quit',
        'dq0 -d /ram/dug', 'dig down', '!' + '\u0100'.repeat(15) + 'stop', 'quit', 'echo $status']
        .map(l => (l[0] === '!' ? l.slice(1) : '\u0101' + l) + '\r').join('') },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], digs = T.filter(t => /^dig /.test(t.line)), stops = T.filter(t => t.line === 'stop');
        if (digs.length !== 4 || stops.length !== 3) return ['the digs and stops not typed: ' + digs.length + ' and ' + stops.length];
        // (A dig and the stop after it as one turn (the wall's second, to its end, its own); its E_DIG events; its
        // spells; the feet to go after its last spell; the hole's record, DUG, after a stop: at the square dug, dy south
        // of where the character stands, dug way d, level.h's DIR_S 2 or DIR_DOWN 5)
        const [s1, s2, d1, d2] = digs.map((t, i) => i === 1 ? t : T.and(t)), [x1, x2, x3] = stops;
        const ev = t => dq0Ev(t, 'E_DIG', {}), says = t => ev(t).map(e => e.how + ' ' + e.feet).join(', ');
        const spells = t => ev(t).filter(e => ['D_GOOD', 'D_POOR', 'D_SLIP'].includes(e.how));
        const togo = t => { const s = spells(t); return s.length ? s[s.length - 1].feet : -1; };
        const hole = (t, dy, d) => (t.after ? t.after.changes : []).filter(c => c.what === 'DUG' && c.sn === t.after.pc.sn &&
          c.x === t.after.pc.x && c.y === t.after.pc.y + dy && c.elev === d)[0];
        const part = (t, x, d, dy, what) => {             // (Dug a while and stopped: its record, the half-feet dug,
                                                          //   the last spell's feet to go)
          const b = ev(t).find(e => e.how === 'D_BEGUN' || e.how === 'D_AGAIN');
          if (!spells(t).some(e => e.how !== 'D_SLIP')) f.push(what + ' dug nothing before it was stopped (' + says(t) + ')');
          if (ev(t).some(e => e.how === 'D_THROUGH')) f.push(what + ' through before it was stopped');
          dq0Need(f, x, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_DIG' }, what + ' not stopped');
          const h = hole(x, dy, d);
          if (!h || !h.tile) { f.push(what + ': no record of its hole after the stop (' + JSON.stringify(x.after && x.after.changes.filter(c => c.what === 'DUG')) + ')'); return 0; }
          if (b && b.how === 'D_BEGUN' && (2 * b.feet - h.tile + 1) >> 1 !== togo(t)) f.push(what + ': its record\'s ' + h.tile + ' half-feet not the ' + togo(t) + ' feet to go of ' + b.feet + ' (' + says(t) + ')');
          return h.tile;
        };
        // (The wall to the south: two feet, dug a while, stopped; dug again, on from where it was left, through)
        dq0Need(f, s1, 'E_DIG', { how: 'D_BEGUN', feet: 2 }, 'the wall\'s dig not begun (two feet)');
        part(s1, x1, 2, 1, 'the wall');
        dq0Need(f, s2, 'E_DIG', { how: 'D_AGAIN', feet: togo(s1) }, 'the wall not dug again from where it was left, ' + togo(s1) + ' feet to go');
        dq0Never(f, s2, 'E_DIG', { how: 'D_BEGUN' }, 'the wall\'s dig begun afresh');
        dq0Need(f, s2, 'E_DIG', { how: 'D_THROUGH' }, 'the wall not dug through');
        // (The shaft: dug a while, stopped, the game kept; carried on, dug again from where it was left, its bar too)
        const b = ev(d1).find(e => e.how === 'D_BEGUN'), h = part(d1, x2, 5, 0, 'the shaft');
        if (!b) f.push('the shaft not begun (' + says(d1) + ')');
        dq0Need(f, d2, 'E_DIG', { how: 'D_AGAIN', feet: togo(d1) }, 'the shaft not dug again from where it was left, carried on (dq0 -d), ' + togo(d1) + ' feet to go');
        dq0Never(f, d2, 'E_DIG', { how: 'D_BEGUN' }, 'the shaft begun afresh, carried on');
        const k = x3.at && x3.at.task;
        if (b && h && (!k || k.kind !== 'LT_DIG' || k.len !== 2 * b.feet || k.done < h)) f.push('the shaft\'s bar not on from where it was left: ' + JSON.stringify(k) + ', its record ' + h + ' half-feet of ' + 2 * b.feet);
        // (The UI's words for it, both times: the UI's own, whatever the referee)
        const again = dq0Story(out).match(/You set to digging again where you left off\. \((?:A foot|\d+ feet) to go\.\)/g) || [];
        if (again.length !== 2) f.push('"You set to digging again where you left off." said ' + again.length + ' times, not 2');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tasks', asks: true, what: 'Dingus Quest Zero\'s long tasks (slice 4.0, HOUSE.md\'s: the world\'s, the hub\'s pc_task; the user\'s choices: a bar and words, hard stops, soft asks, all wounds hard), by the world\'s events: at seed 3 "stop" with nothing to stop, then a walk east down a passage a square at a time, its beats as it goes (it turns, a quiet stretch) and its bar on the status line; at seed 3 by the way down a dig begun, "stop walking" refused (it\'s a dig), "stop digging" stops it; at seed 21 below, a long wait asked about at a sound heard ("Keep waiting?": the UI\'s, I_ASK), answered yes (it goes on), no (it stops), and with a command (it stops, and the command\'s carried out); and with the hand-written theme\'s hostile rats (inhabit.txt) a long wait stopped hard by a bite',
      init: 't_rc', cycles: 3500e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm', 'stop', 'e', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', 'dig down', '!stop walking', '!stop digging', 'quit',
        ...['yes', 'no', 'look'].flatMap(ans => ['dq0 -p t -s 21 -c /ram/c.hm -T', 'd', 'wait 30 minutes', ans, 'quit']),
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'wait 10 minutes', 'quit', 'echo done']
        .map(l => (l[0] === '!' ? l.slice(1) : 'ā' + l) + '\r').join('') },
      expect: ['% echo done'],
      check(m, out) {
        const T = dq0Turns(m), f = [], ev = (t, k, want) => dq0Ev(t, k, want);
        const games = [], g = [];
        for (const t of T) { if (/^dq0 /.test(t.line)) games.push(g.length ? g.splice(0) : []); g.push(t); }
        games.push(g.splice(0)); games.shift();
        if (games.length !== 6) return ['the six games not played: ' + games.length];
        const all = gm => gm.flatMap(t => t.events), has = (gm, want, what) => { if (!all(gm).some(e => e.kind === 'E_TASK' && Object.entries(want).every(([k, v]) => e[k] === v))) f.push(what + ' (' + all(gm).filter(e => e.kind === 'E_TASK').map(e => e.te + ':' + e.task).join(', ') + ')'); };
        // (A walk: stop with none, then east down the passage, its beats and its bar)
        const [w] = [games[0]], stop = w.find(t => t.line === 'stop'), east = w.find(t => t.line === 'e');
        if (!ev(stop, 'E_TASK', { te: 'TE_IDLE' }).length) f.push('"stop" with nothing to stop not told so');
        const beats = ev(east, 'E_BEAT', {});
        if (!beats.some(e => e.beat === 'WB_TURN') || !beats.some(e => e.beat === 'WB_QUIET')) f.push('the walk east not told as it went: ' + beats.map(e => e.beat + ':' + e.side).join(', '));
        if (!ev(east, 'E_ARRIVE', {}).length) f.push('the walk east not ended');
        if (!/\[#+\.*\]/.test(out)) f.push('no long task\'s bar on the status line');
        // (A dig: a stop that names another task refused, then the dig stopped)
        has(games[1], { te: 'TE_NOT', task: 'LT_DIG', named: 'LT_WALK' }, '"stop walking" not refused as the dig went on');
        has(games[1], { te: 'TE_STOPPED', task: 'LT_DIG' }, '"stop digging" not the dig stopped');
        // (A long wait at a sound heard: asked, answered yes, no, and with a command)
        for (const [k, ans] of [[2, 'yes'], [3, 'no'], [4, 'look']]) {
          has(games[k], { te: 'TE_ASK_HEARD', task: 'LT_WAIT' }, 'the wait (' + ans + ') not asked about at a sound');
          if (ans === 'yes') has(games[k], { te: 'TE_ON', task: 'LT_WAIT' }, 'the wait not gone on at yes');
          else has(games[k], { te: 'TE_STOPPED', task: 'LT_WAIT' }, 'the wait not stopped at ' + ans);
        }
        if (!/> look[\s\S]{0,40}\n?[\s\S]*?You stop waiting\.[\s\S]*?You can see part of a cave/.test(dq0Story(out).replace(/> /g, '\n> '))) f.push('the look typed for an answer not carried out after the wait stopped');
        // (Hostile rats: the wait stopped hard by a bite)
        if (!all(games[5]).length || !games[5].flatMap(t => t.dens).some(e => e.kind === 'E_FIGHT' && e.def === 0)) f.push('no rat bit at the character as it waited');
        has(games[5], { te: 'TE_HIT', task: 'LT_WAIT' }, 'the wait not stopped hard by the bite');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0cmd', what: 'Dingus Quest Zero\'s command interpreter (Phase 3C, the user\'s DesignIdeas.md): at seed 3 (-T), three giant rats near, the target set by a choice ("target the rat": which, in their order from the left round to the right, two the same way and as far told apart by their side, answered by number) and shown; a dagger thrown hard at the closest of them (the world\'s throw: its reach, the range bands, the referee\'s Throwing; it misses, and lands beside it); a bare take, the last thing named carried, asking which of the things near, answered by a name, then by a selector; take all (walked over to); a bare drop, the last thing named; it; a throw with no mark, the target lost from sight; a bare attack, out of reach; the target cleared; a look one way (turned to), a look round the corner, N alone going north',
      init: 't_rc', cycles: 1500e6,
      machine: { input: '\u0101bind -a /sd/0/games /bin\r\u0101hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\r\u0101dq0 -p t -s 3 -c /ram/c.hm -k pick -T\r' +
        ['target the rat', '1', 'target', 'throw the dagger hard at the closest rat', 'drop pick', 'examine the torch', 'take', 'the bones', 'take',
          'the closest', 'take all', 'i', 'drop', 'take it', 'throw the dagger', 'attack', 'clear target', 'target', 'look north',
          'look around the corner', 'n', 'quit', 'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['% echo $status'],
      // (Its words checked with the screen's codes and the lines' wrapping taken out, each in its turn)
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        const want = ['Which do you mean: 1. the giant rat ahead to your left, on the right, 2. the giant rat ahead to your left, on the ' +
          'left, or 3. the giant rat ahead?', 'Your target: the giant rat.', 'Your target: the giant rat ahead to your left.',
          'You throw the dagger hard at the giant rat, and miss: it lands beside it.', 'Put down.',
          'Which do you mean: 1. the dagger ahead, 2. the bones ahead to your right, or 3. the pick where you stand?', 'Taken.',
          'Which do you mean: 1. the dagger ahead, or 2. the pick where you stand?', 'Taken.', 'You go over to it.', 'Taken.',
          'a dagger, a pick, bones', 'Put down.', 'Taken.', 'Throw it at what?', 'It\'s out of reach', 'You have no target now.',
          'You have no target.', 'You turn to face north.', 'You cross the room.', '% echo $status'];
        let at = 0;
        for (const w of want) {
          const i = o.indexOf(w, at);
          if (i < 0) { f.push('not found (in its turn): ' + w); break; }
          at = i + w.length;
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0talk', meaning: true, what: 'Dingus Quest Zero understanding the player (the command parser, dq0c: the grammar and the game\'s words, after the user\'s Language.txt), by what each command did: synonyms (take a gander: a look, no time), several words to a verb, turns as the character faces (half left) and two directions together (north and east), a door behind walked up to (asked which: the second), the little words passed over (off the verb\'s in "take off my coat": nothing worn refused; a filler in "saunter off to the left": a move to its left), a thing not there refused, a word it doesn\'t know doing nothing (their words: dq0wordsthings)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'take a gander', 'turn half left',
        'face north and east', 'walk to the door behind me', '2', 'pick up the sword', 'take off my coat', 'saunter off to the left', 'foo', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], still = t => t.k >= 0 && !t.events.length && t.at && t.after && t.after.now === t.at.now && t.after.pc.x === t.at.pc.x && t.after.pc.y === t.at.pc.y;
        const face = (l, h) => { const t = T.of(l); if (!t.after || t.after.pc.face !== h) f.push(l + ': facing ' + (t.after && t.after.pc.face) + ', not ' + h); return t; };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T').after;
        if (!s0 || s0.pc.face !== 6) return ['not begun facing west'];
        if (!still(T.of('take a gander'))) f.push('take a gander not a look (' + dq0Evs(T.of('take a gander')) + ')');
        face('turn half left', 5);
        face('face north and east', 1);
        const w = T.of('walk to the door behind me'), c = T.of('2');
        if (!still(w)) f.push('walk to the door behind me not asked which (' + dq0Evs(w) + ')');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'the second door behind not walked up to');
        if (!c.after || c.after.pc.face !== 4) f.push('not facing the door behind it (south)');
        if (!dq0Rep(T.of('pick up the sword'), 'ireq', { kind: 'IO_GET', result: 'IR_NONE' }).length) f.push('a sword not there not refused');
        if (!dq0Rep(T.of('take off my coat'), 'ireq', { kind: 'IO_DOFF', result: 'IR_NO' }).length) f.push('take off my coat not a doff refused');
        const s = T.of('saunter off to the left');
        dq0Need(f, s, 'E_ARRIVE', {}, 'saunter off to the left not a move');
        if (!s.at || !s.after || !(s.after.pc.x > s.at.pc.x) || s.after.pc.face !== 2) f.push('not moved to its left (east, facing south)');
        if (!still(T.of('foo'))) f.push('a word it doesn\'t know did something (' + dq0Evs(T.of('foo')) + ')');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0items', meaning: true, what: 'Dingus Quest Zero\'s things (items.h, the world\'s item store), by where each one is (the store\'s records) and what the world said to each request (ireq\'s result): the kit a character begins with (dq0 -k pick: a pick more), its torch lit in hand, its pack and belt worn; what it carries listed; a thing examined, one there isn\'t refused; a thing put down (on its square, seen at its feet), taken up again; one out of reach walked over to first (the world\'s I_TAKE); one it hasn\'t refused; a thing hidden where it stands (on its square, hidden, seen so), taken up again (their words: dq0wordsthings)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', 'inventory',
        'examine the dagger', 'examine the moon', 'drop the dagger', 'look', 'get dagger', 'drop pick', 'walk to the door ahead', 'turn around',
        'take the pick', 'drop rocks', 'hide the pouch', 'look', 'take pouch', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], I = DQ0_I, mine = (s, n) => dq0Carried(s).some(i => i.name === n), here = (s, n) => dq0Here(s).find(i => i.name === n);
        const rep = (l, kind, result) => { const t = T.of(l); if (!dq0Rep(t, 'ireq', { kind, result }).length) f.push(l + ': no ' + kind + ' ' + result + ' ' + JSON.stringify(t.replies.filter(r => r.req === 'ireq'))); return t; };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -k pick -T').after;
        for (const n of ['torch', 'backpack', 'rations', 'tinderbox', 'belt', 'pouch', 'pennies', 'dagger', 'waterskin', 'pick'])
          if (!mine(s0, n)) f.push('the kit without its ' + n);
        const torch = dq0Thing(s0, 'torch');
        if (!torch || (torch.flags & (I.LIT | I.HELD)) !== (I.LIT | I.HELD)) f.push('the torch not lit in hand');
        for (const n of ['backpack', 'belt']) { const i = dq0Thing(s0, n); if (!i || !(i.flags & I.WORN)) f.push('the ' + n + ' not worn'); }
        rep('inventory', 'IO_LIST', 'IR_OK');
        rep('examine the dagger', 'IO_DESCRIBE', 'IR_OK');
        rep('examine the moon', 'IO_DESCRIBE', 'IR_NONE');
        const d = rep('drop the dagger', 'IO_DROP', 'IR_OK');
        if (!here(d.after, 'dagger') || mine(d.after, 'dagger')) f.push('the dagger not put down where it stands');
        const lk = T.after(d, 'look');
        if (!lk.after || !lk.after.sight || !lk.after.sight.items.some(i => i.name === 'dagger' && i.rel === 255)) f.push('the dagger not seen at its feet');
        if (!mine(rep('get dagger', 'IO_GET', 'IR_OK').after, 'dagger')) f.push('the dagger not taken up again');
        const p = rep('drop pick', 'IO_DROP', 'IR_OK'), at = p.after && p.after.pc;
        if (!here(p.after, 'pick')) f.push('the pick not put down');
        const w = T.of('walk to the door ahead');
        dq0Need(f, w, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'not walked to the door');
        if (at && w.after && Math.max(Math.abs(w.after.pc.x - at.x), Math.abs(w.after.pc.y - at.y)) < 2) f.push('the pick not out of reach after the walk');
        const t = rep('take the pick', 'IO_GET', 'IR_NOT_HERE');
        dq0Need(f, t, 'E_TAKEN', { ir: 'IR_OK' }, 'the pick out of reach not walked over to and taken');
        if (!mine(t.after, 'pick') || !at || t.after.pc.x !== at.x || t.after.pc.y !== at.y) f.push('not at the pick\'s square with it');
        const r = rep('drop rocks', 'IO_DROP', 'IR_NONE');
        if (r.at && r.after && dq0Carried(r.at).length !== dq0Carried(r.after).length) f.push('something put down for rocks it hasn\'t');
        const h = rep('hide the pouch', 'IO_HIDE', 'IR_OK'), hp = here(h.after, 'pouch');
        if (!hp || !(hp.flags & I.HIDDEN) || mine(h.after, 'pouch')) f.push('the pouch not hidden where it stands');
        const lk2 = T.after(h, 'look');
        if (!lk2.after || !lk2.after.sight || !lk2.after.sight.items.some(i => i.name === 'pouch' && (i.hid & 1))) f.push('the pouch not seen hidden at its feet');
        const tp = rep('take pouch', 'IO_GET', 'IR_OK'), pp = dq0Thing(tp.after, 'pouch');
        if (!mine(tp.after, 'pouch') || (pp.flags & I.HIDDEN)) f.push('the pouch not taken up again');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0equip', meaning: true, what: 'Dingus Quest Zero\'s things handled (the world\'s item store, dq0w/things.c), by where each one is and its state (held, worn, lit: the store\'s records) and what the world said to each (ireq\'s result: done or refused): a kit of things more (dq0 -k, commas between: a table held in both hands, the rest in the pack); a table put down and a candle put on it, taken from it again; a cloak worn and taken off; a basket held that won\'t keep sand, a waterskin that won\'t take a dagger, a quiver hung on the belt and arrows put in it, a candle that won\'t hang, a dagger not taken in hand with the hands full, the pack not into the basket that\'s in it; then down to a dark place: the torch put out (dark), a candle lit (out of the pack into a hand: light), the torch lit, put down lit (its light lies about it, the room still seen with the candle out), taken up again (their words: dq0wordsthings)',
      init: 't_rc', cycles: 1200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -k table,basket,sand,cloak,candle,quiver,arrows',
        'drop table', 'put the candle on the table', 'look', 'take the candle from the table', 'wear cloak', 'hold basket', 'put sand in basket',
        'put the dagger in the waterskin', 'hang the quiver on my belt', 'put arrows in quiver', 'hang the candle', 'wield the dagger', 'stow basket',
        'put the backpack in the basket', 'take the cloak off', 'stow cloak', 'd', 'douse the torch', 'light the candle', 'light torch',
        'drop the torch', 'douse candle', 'look', 'get torch', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], I = DQ0_I;
        const put = (l, kind, result) => { const t = T.of(l); if (!dq0Rep(t, 'ireq', { kind, result }).length) f.push(l + ': no ' + kind + ' ' + result + ' ' + JSON.stringify(t.replies.filter(r => r.req === 'ireq').map(r => r.kind + ' ' + r.result))); return t; };
        const thing = (t, n) => dq0Thing(t.after, n), holder = (t, n) => { const h = dq0Holder(t.after, thing(t, n)); return h ? h.name : ''; };
        const flags = (t, n, on, off, what) => { const i = thing(t, n); if (!i || (i.flags & on) !== on || (i.flags & off)) f.push(t.line + ': the ' + n + ' not ' + what + ' (flags ' + (i && i.flags) + ')'); };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k table,basket,sand,cloak,candle,quiver,arrows');
        flags(s0, 'table', I.HELD, 0, 'held');
        for (const n of ['basket', 'heap of sand', 'cloak', 'candle', 'quiver', 'arrows']) if (holder(s0, n) !== 'backpack') f.push('the kit\'s ' + n + ' not in the pack');
        const dt = put('drop table', 'IO_DROP', 'IR_OK');
        if (!dq0Here(dt.after).some(i => i.name === 'table')) f.push('the table not put down');
        if (holder(put('put the candle on the table', 'IO_PUT', 'IR_OK'), 'candle') !== 'table') f.push('the candle not on the table');
        if (holder(put('take the candle from the table', 'IO_TAKE', 'IR_OK'), 'candle') !== 'backpack') f.push('the candle not taken from the table into the pack');
        flags(put('wear cloak', 'IO_DON', 'IR_OK'), 'cloak', I.WORN, 0, 'worn');
        flags(put('hold basket', 'IO_HOLD', 'IR_OK'), 'basket', I.HELD, 0, 'held');
        if (holder(put('put sand in basket', 'IO_PUT', 'IR_NO'), 'heap of sand') !== 'backpack') f.push('the sand moved into the basket');
        if (holder(put('put the dagger in the waterskin', 'IO_PUT', 'IR_NO'), 'dagger') === 'waterskin') f.push('the dagger in the waterskin');
        if (holder(put('hang the quiver on my belt', 'IO_PUT', 'IR_OK'), 'quiver') !== 'belt') f.push('the quiver not hung on the belt');
        if (holder(put('put arrows in quiver', 'IO_PUT', 'IR_OK'), 'arrows') !== 'quiver') f.push('the arrows not in the quiver');
        if (holder(put('hang the candle', 'IO_PUT', 'IR_NO'), 'candle') !== 'backpack') f.push('the candle moved, hung');
        flags(put('wield the dagger', 'IO_HOLD', 'IR_NO'), 'dagger', 0, I.HELD, 'left alone (the hands full)');
        if (holder(put('stow basket', 'IO_STOW', 'IR_OK'), 'basket') !== 'backpack') f.push('the basket not stowed');
        flags(put('put the backpack in the basket', 'IO_PUT', 'IR_NO'), 'backpack', I.WORN, 0, 'still worn');
        flags(put('take the cloak off', 'IO_DOFF', 'IR_OK'), 'cloak', I.HELD, I.WORN, 'taken off into a hand');
        if (holder(put('stow cloak', 'IO_STOW', 'IR_OK'), 'cloak') !== 'backpack') f.push('the cloak not stowed');
        dq0Need(f, T.of('d'), 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        const dark = (t, d, what) => { if (!t.after || !t.after.sight || t.after.sight.dark !== d) f.push(t.line + ': ' + what); };
        const dz = put('douse the torch', 'IO_DOUSE', 'IR_OK');
        flags(dz, 'torch', 0, I.LIT, 'put out');
        dark(dz, 1, 'not dark');
        const lc = put('light the candle', 'IO_LIGHT', 'IR_OK');
        flags(lc, 'candle', I.LIT | I.HELD, 0, 'lit in a hand');
        dark(lc, 0, 'dark still, the candle lit');
        flags(put('light torch', 'IO_LIGHT', 'IR_OK'), 'torch', I.LIT, 0, 'lit');
        const dr = put('drop the torch', 'IO_DROP', 'IR_OK'), tr = dq0Here(dr.after).find(i => i.name === 'torch');
        if (!tr || !(tr.flags & I.LIT)) f.push('the torch not put down lit');
        flags(put('douse candle', 'IO_DOUSE', 'IR_OK'), 'candle', 0, I.LIT, 'put out');
        dark(T.after(dr, 'look'), 0, 'dark, the torch lit on the floor');
        flags(put('get torch', 'IO_GET', 'IR_OK'), 'torch', I.LIT, 0, 'taken up lit');
        if (!dq0Carried(T.of('get torch').after).some(i => i.name === 'torch')) f.push('the torch not carried again');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0harm', what: 'Dingus Quest Zero\'s climbing and harm (the referee\'s: climbing tests, RS_HARM\'s falling, Skills 8, and its injuries, Combat 13-14: HarnMaster\'s at this seed), by the world\'s events and the referee\'s summary: at seed 517 the kobold warren\'s gatehouse (a built stratum\'s floors flat), a ledge 10 feet up met; climbed (u, its words\' though the way out\'s in the room): the climb\'s test at its foot a critical failure, no hold found yet (nothing to fall: the soak\'s, 2026-10-19, "you fall 0 feet" no more), then up; u again the way out\'s, shut; then jumped down it, a fall of 10 feet onto paving, an injury (the character hurt), and kept in the save (pc.hm\'s inj line, hmchar\'s format); a creep, listening as it goes, kept with the sheet too (pc.hm\'s pace line, the referee\'s: dq0pace\'s before)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 517 -c /ram/c.hm -d /ram/hurt', 'ne', 'e', 'u', 'u',
        'turn around', 'jump down', 'creep', 'listen as I go', 'quit', 'cat /ram/hurt/pc.hm', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], e = T.of('e'), u = T.of('u'), u2 = T.after(u, 'u'), j = T.of('jump down');
        dq0Need(f, e, 'E_LEDGE', { rise: 10 }, 'no ledge 10 feet up met');
        dq0Need(f, u, 'E_CLIMB', { how: 'K_BEGUN' }, 'the climb not begun');
        dq0Need(f, u, 'E_CLIMB', { how: 'K_NOHOLD' }, 'the critical failure at its foot not a hold not found');
        dq0Never(f, u, 'E_CLIMB', { how: 'K_FELL' }, 'a fall from the ledge\'s foot');
        dq0Never(f, u, 'E_HARM', {}, 'harm from the ledge\'s foot');
        dq0Need(f, u, 'E_CLIMB', { how: 'K_UP' }, 'not up the ledge');
        dq0Need(f, u2, 'E_NOWAY', { why: 'N_SHUT' }, 'the second u not the way out\'s, shut');
        dq0Need(f, j, 'E_CLIMB', { how: 'K_JUMPED', feet: 10 }, 'not jumped down 10 feet');
        dq0Need(f, j, 'E_HARM', { hc: 'HC_FALL', band: b => b > 0 }, 'the fall did no harm');
        if (!j.after || !(j.after.sum.health > 0)) f.push('the character not hurt by the fall');
        const pc = out.slice(out.indexOf('% cat /ram/hurt/pc.hm'));
        if (!/\ninj /.test(pc)) f.push('the injury not kept in the save (pc.hm\'s inj line)');
        if (!/\npace 0 2 /.test(pc)) f.push('the pace and watchfulness not kept with the sheet (pc.hm\'s pace line: a creep, listening)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rules', what: 'Dingus Quest Zero\'s referee\'s unit tests (Phase 3A: tests/ram/t_rules, its cases in rules.txt beside it): the HarnMaster referee (dq0harn) asked rs.h\'s requests by a program playing the world\'s part, its dice fixed (RS_DICE: dice.h\'s hook), each answer held to the book\'s rules and HOUSE.md\'s, and the dice each rolled to the book\'s: the tests\' grades at their edges (critical and marginal, the 5-95 clamp, the penalty each skill takes), noticing by pace and state, falls by height and ground (a landing on the feet, a hold grabbed), injuries by impact, aspect, location and armour (Combat 13-14), the kill, amputation, shock and stumble rolls, senselessness and coming round, bleeding, encumbrance by load, fatigue by exertion and rest, digging and appraising',
      init: 't_rc', cycles: 1100e6,
      pc: { files: () => ({ 't_rules.hyx': fs.readFileSync(path.join(__dirname, '..', 'obj', 'tests', 't_rules.hyx')),
        'rules.txt': fs.readFileSync(path.join(__dirname, 'ram', 't_rules', 'rules.txt')) }) },
      machine: { input: '\u0101cp /pc/t_rules.hyx /ram/t_rules.hyx; cp /pc/rules.txt /ram/rules.txt\r\u0101/ram/t_rules.hyx /ram/rules.txt\r' },
      expect: ['t_rules: PASS'],
      check(m, out) {
        const t = out.slice(out.indexOf('% /ram/t_rules.hyx'));
        this.notes = [(t.match(/^ok - /gm) || []).length + ' cases held'];
        return [...t.matchAll(/^not ok - ([^\r\n]*)/gm)].map(k => k[1]);
      },
      sdcard: true
    },
    {
      name: 'dq0save', meaning: true, what: 'Dingus Quest Zero kept (dq0 -d: the save on a card, here /ram), by the game\'s state: a thing put down, down a stairway, quit (kept as it ends: the world\'s save), the folder listed (the UI\'s game, the things, the world\'s change records, the denizens\', the maker\'s seen maps), then the game carried on: in the stratum below, where it stood and as it faced, seeing what it saw, carrying what it carried, each thing where it was (worn, held, in what)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/kept', 'drop the dagger',
        'd', 'quit', 'ls /ram/kept', 'dq0 -d /ram/kept', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], dr = T.of('drop the dagger'), d = T.of('d'), q = T.and(T.of('quit')), c = T.of('dq0 -d /ram/kept');
        if (!dq0Here(dr.after).some(i => i.name === 'dagger')) f.push('the dagger not put down');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        dq0Need(f, q, 'E_BYE', {}, 'the game not ended');
        if (!dq0Rep(q, 'wreq', { kind: 'WQ_SAVE' }).length) f.push('the game not kept as it ended (the world\'s WQ_SAVE)');
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), ls = o.slice(o.indexOf('% ls /ram/kept'), o.indexOf('% dq0 -d'));
        for (const n of ['changes.dat', 'dens.dat', 'game.dat', 'items.dat', 'seen.dat']) if (!ls.includes(n)) f.push('no ' + n + ' in the save folder');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        const a = d.after, b = c.after;
        if (!a || !b || a.pc.sn !== 1 || b.pc.sn !== a.pc.sn || b.pc.x !== a.pc.x || b.pc.y !== a.pc.y || b.pc.face !== a.pc.face)
          f.push('not carried on where it stood, as it faced: ' + JSON.stringify(b && b.pc) + ', kept ' + JSON.stringify(a && a.pc));
        if (dq0View(b) !== dq0View(a)) f.push('not seeing what it saw: ' + dq0View(b) + ', kept ' + dq0View(a));
        const kit = s => dq0Carried(s).map(i => i.name + '/' + (dq0Holder(s, i) || { name: '' }).name + '/' + (i.flags & 7)).sort().join(', ');
        if (!kit(b) || kit(b) !== kit(a)) f.push('not carrying what it carried: ' + kit(b) + ', kept ' + kit(a));
        if (/dagger\//.test(kit(b))) f.push('the dagger carried, put down before');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0vast', meaning: true, what: 'Dingus Quest Zero\'s plans past PLAN_MAX regions (move 2.8: no fixed size of a stratum\'s plan), by what the character perceives and the maker\'s map: dq0 -T -T -T -T (the stratum below the top 27 regions each way, 729 of them, its plan paged to the save folder by the maker, plSS0000.dat, its pages of 42 regions in a bank of its own) and -C (16 of those pages, made way for as it grows) and -d: down the stairway, the stratum below grown, mapped; quit and carried on (its plan grown again, the same): seen the same, the map the same',
      init: 't_rc', cycles: 3600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -T -T -T -C -d /ram/kept', 'd',
        'map', 'quit', 'ls /ram/kept', 'dq0 -C -d /ram/kept', 'map', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], d = T.of('d'), c = T.of('dq0 -C -d /ram/kept'), m1 = T.of('map'), m2 = T.after(c, 'map');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (!d.after || d.after.pc.sn !== 1 || !c.after || c.after.pc.sn !== 1 || dq0View(c.after) !== dq0View(d.after))
          f.push('the vast stratum not seen the same carried on: ' + dq0View(c.after) + ', first ' + dq0View(d.after));
        if (!m1.after || !m2.after || !m1.after.map.some(r => r.trim()) || m1.after.map.join('\n') !== m2.after.map.join('\n')) f.push('the map not the same carried on');
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' ');
        if (!/pl10000\.dat/.test(o.slice(o.indexOf('% ls /ram/kept')))) f.push('no plan paged to the save folder (pl10000.dat)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0soak', meaning: true, what: 'Dingus Quest Zero\'s stores paged to the save folder (move 2.8, DUNGEON.md: no fixed size; the user, 2026-10-16: "It should be as big as it needs to be, given there is enough disk space to hold it"), eviction and write-back, a soak, by what the character perceives (the senses\' record) and the stores\' records: dq0 -C (the caches small: the chunks\' twelve slots, a change page, 24 of the item store\'s records, two of the denizens\' regions, two of the strata\'s slots, two seen maps) and -T -T -T (three strata grown below the top, the character on the second): the dagger put down by the stairway there, up (the strata\'s slots made way in: the top\'s record read back into a slot), the rations put down by the stairway on the top, down and up again and again, every cache made way in each time (the chunks made again from their seeds, the change records, the things and the denizens read back from their files, the seen maps too): each stratum seen each time as it was first (the dagger, the rations where they were put down), the giant rats by the stairway seen; the map; the folder\'s region files (rg, it, dn, sm, st); quit and carried on: where it was, seen the same, the map as it was, up again, the rations there; then the game\'s game.dat gone and a new game begun in the folder (seed 5): the old game\'s region files of its stratum below gone, none of its things lying about (dq0page\'s and dq0strata\'s, these together)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -T -T -C -d /ram/kept',
        'drop the dagger', 'u', 'drop the rations', 'd', 'u', 'd', 'u', 'd', 'map', 'quit', 'ls /ram/kept', 'dq0 -C -d /ram/kept', 'map', 'u', 'quit',
        'rm /ram/kept/game.dat', 'dq0 -p t -s 5 -c /ram/c.hm -T -C -d /ram/kept', 'quit', 'ls /ram/kept', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], at = (s, n) => s && dq0Here(s).some(i => i.name === n);
        const d0 = T.of('drop the dagger'), r0 = T.of('drop the rations');
        if (!at(d0.after, 'dagger') || !d0.after || d0.after.pc.sn !== 1) f.push('the dagger not put down on the second stratum');
        if (!at(r0.after, 'rations') || !r0.after || r0.after.pc.sn !== 0) f.push('the rations not put down on the top');
        const view = { 1: dq0View(d0.after), 0: dq0View(r0.after) };
        let rats = 0;
        for (const t of T.filter(t => (t.line === 'u' || t.line === 'd') && t.game === 0 && t.k > r0.k)) {     // (Each way, after both were put down)
          const s = t.after, sn = t.line === 'u' ? 0 : 1;
          dq0Need(f, t, 'E_ARRIVE', { how: t.line === 'u' ? 'A_UP' : 'A_DOWN' }, 'not ' + (t.line === 'u' ? 'up' : 'down') + ' the stairway');
          if (!s || s.pc.sn !== sn) { f.push('> ' + t.line + ' (line ' + t.k + '): not on stratum ' + sn); continue; }
          if (dq0View(s) !== view[sn]) f.push('stratum ' + sn + ' not as it was (line ' + t.k + '): ' + dq0View(s) + ', first ' + view[sn]);
          if (s.sight.dens.some(d => d.name === 'giant rat')) rats++;
        }
        if (!rats) f.push('the giant rats by the stairways never seen (their regions\' records read back)');
        const mp = T.of('map'), q = T.of('quit'), c = T.of('dq0 -C -d /ram/kept'), mp2 = T.after(c, 'map'), u2 = T.after(c, 'u');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (!c.after || !q.at || c.after.pc.sn !== q.at.pc.sn || c.after.pc.x !== q.at.pc.x || c.after.pc.y !== q.at.pc.y || dq0View(c.after) !== view[1])
          f.push('not carried on where it was, seen the same');
        if (!mp.after || !mp2.after || !mp.after.map.some(r => r.trim()) || mp.after.map.join('\n') !== mp2.after.map.join('\n')) f.push('the map not as it was, carried on');
        if (!u2.after || dq0View(u2.after) !== view[0]) f.push('the top not as it was, up again once carried on (the rations)');
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' ');
        const ls1 = o.slice(o.indexOf('% ls /ram/kept'), o.indexOf('% dq0 -C')), ls2 = o.slice(o.lastIndexOf('% ls /ram/kept'), o.indexOf('% echo $status'));
        for (const p of ['rg', 'it', 'dn', 'sm', 'st']) if (!new RegExp(p + '[0-9a-f]{5,6}\\.da').test(ls1)) f.push('no ' + p + '... region files');
        if (/rg1\w{4}\.da/.test(ls2)) f.push('the old game\'s region files left in the folder for the new game (its stratum below\'s)');
        const g = T.of('dq0 -p t -s 5 -c /ram/c.hm -T -C -d /ram/kept');
        dq0Need(f, g, 'E_ARRIVE', { how: 'A_START' }, 'the new game not begun');
        if (!g.after || g.after.game !== 2 || g.after.items.some(i => i.where === 'IW_SQUARE' && (i.name === 'dagger' || i.name === 'rations')))
          f.push('the old game\'s things lying about in the new game');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0names', meaning: true, what: 'Dingus Quest Zero\'s names (the user, 2026-10-16: "I would like to be able to reference items by aliases, as long as the alias doesn\'t clash with something else"; names.c), by the names table (hub.h\'s struct tname, NAMES_AT) and the things\' records: a sword named Linda (dq0 -k sword: its record marked named), names that clash refused (another name, a kind of thing\'s, a kind of creature\'s, a word the parser needs: no name made), Linda used in commands (dropped, seen lying there, taken up, examined, taken in hand to strike a rat with), the room named the Mausoleum; the game kept and carried on, the names with it; then a name forgotten (its record no longer named) (the names\' words: dq0wordsthings)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -k sword -d /ram/kept',
        'name my sword Linda', 'alias the dagger as linda', 'name the dagger Torch', 'name the dagger kobold', 'name the dagger north', 'drop Linda',
        'look', 'take Linda', 'examine Linda', 'attack the rat with Linda', 'call this room the mausoleum', 'quit', 'dq0 -d /ram/kept',
        'forget the name Linda', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], NAMED = 0x40;
        const names = t => (t.after ? t.after.names : []).map(n => n.what + ' ' + n.name).join(', ');
        const sword = t => dq0Thing(t.after, 'sword');
        const n1 = T.of('name my sword Linda');
        if (names(n1) !== 'NM_ITEM Linda') f.push('the sword not named Linda: ' + names(n1));
        if (!sword(n1) || !(sword(n1).flags & NAMED)) f.push('the sword\'s record not marked named');
        for (const l of ['alias the dagger as linda', 'name the dagger Torch', 'name the dagger kobold', 'name the dagger north'])
          if (names(T.of(l)) !== 'NM_ITEM Linda') f.push(l + ': a name that clashes made: ' + names(T.of(l)));
        const d = T.of('drop Linda');
        if (!dq0Here(d.after).some(i => i.name === 'sword')) f.push('Linda not put down');
        const lk = T.after(d, 'look');
        if (!lk.after || !lk.after.sight || !lk.after.sight.items.some(i => i.name === 'sword' && i.rel === 255)) f.push('Linda not seen lying there');
        if (!dq0Carried(T.of('take Linda').after).some(i => i.name === 'sword')) f.push('Linda not taken up');
        if (!dq0Rep(T.of('examine Linda'), 'ireq', { kind: 'IO_DESCRIBE', result: 'IR_OK', name: 'sword' }).length) f.push('Linda not examined');
        const a = T.of('attack the rat with Linda');
        if (!sword(a) || !(sword(a).flags & DQ0_I.HELD)) f.push('Linda not taken in hand to strike with');
        const r = T.of('call this room the mausoleum');
        if (names(r) !== 'NM_ITEM Linda, NM_ROOM the Mausoleum') f.push('the room not named the Mausoleum: ' + names(r));
        const c = T.of('dq0 -d /ram/kept');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (names(c) !== 'NM_ITEM Linda, NM_ROOM the Mausoleum') f.push('the names not kept with the game: ' + names(c));
        const g = T.of('forget the name Linda');
        if (names(g) !== 'NM_ROOM the Mausoleum') f.push('Linda not forgotten: ' + names(g));
        if (!sword(g) || (sword(g).flags & NAMED)) f.push('the sword\'s record still marked named');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denizens', meaning: true, what: 'Dingus Quest Zero\'s denizens (dq0d: the populations decided as a stratum is grown, from a stand-in theme\'s kinds, kinds.txt), by the actors\' records, their events and what the character perceives (their kinds\' records the referee\'s, HarnMaster\'s): the giant rats that lair by the top stratum\'s ways placed round the way down as the character starts there (dq0 -T), seen (the world\'s actors, in what it perceives), and moving about as time passes (their own task deciding): at the turns pace through all the time a minute\'s search takes (the clock on a slice at a time as they keep up), their stirring told (the world\'s E_VIEW) as what the character sees of them changes (gone out of sight), not again when nothing has; then (seed 5, down its ladder) the rats there seen, cornered, and in the character\'s way: the world stops it short of them, and won\'t let it walk or step onto a rat\'s square (dq0steps\' rat in the way, a step, in it)',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'search', 'search', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'd', 'go right', 'go forward', 'step forward', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T'), s0 = g.after;
        const rats = s => s ? s.actors.filter(a => a.state && a.name === 'giant rat') : [];
        const near = (s, n) => rats(s).filter(a => a.sn === s.pc.sn && Math.max(Math.abs(a.x - s.pc.x), Math.abs(a.y - s.pc.y)) <= n);
        if (near(s0, 4).length < 2) f.push('the giant rats not placed round the way down: ' + JSON.stringify(rats(s0)));
        if (!s0 || !s0.sight || !s0.sight.dens.some(d => d.name === 'giant rat')) f.push('the rats not seen as it starts');
        const s1 = T.of('search'), s2 = T.after(s1, 'search'), at = r => r.x + ',' + r.y;
        if (!dq0Ev(s1, 'E_DEN', { de: 'DE_MOVED' }).length || rats(s1.after).map(at).join(' ') === rats(s0).map(at).join(' ')) f.push('the rats not moving about as the search went on');
        dq0Need(f, s1, 'E_VIEW', { why: 'V_STIR' }, 'the rats\' stirring not told as they went out of sight');
        if (!s1.after || !s1.after.sight || s1.after.sight.dens.length) f.push('the rats not gone out of sight');
        dq0Never(f, s2, 'E_VIEW', { why: 'V_STIR' }, 'the rats told stirring again, with nothing seen changed');
        const d = T.of('d');
        if (!d.after || !d.after.sight || d.after.sight.dens.filter(x => x.name === 'giant rat').length < 2) f.push('the rats by the ladder not seen as the character comes down');
        const r = T.of('go right'), gf = T.of('go forward'), sf = T.of('step forward');
        dq0Need(f, r, 'E_NOWAY', { why: 'N_DEN' }, 'not stopped short of the rats');
        for (const t of [gf, sf]) {
          dq0Need(f, t, 'E_NOWAY', { why: 'N_DEN' }, t.line + ': a rat not in the way');
          if (!t.at || !t.after || t.after.pc.x !== t.at.pc.x || t.after.pc.y !== t.at.pc.y) f.push(t.line + ': moved onto a rat\'s square');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0popgen', what: 'Dingus Quest Zero\'s population generator (Phase 3D: popgen, a bank program, bank/popgen; games/dq0d/pop.h) from the shell (bankrun): a theme file\'s inhabitants written by hand (the card\'s games/lib/dq0/inhabit.txt: four factions, one\'s line going on with the next), each faction read (its kinds, its strata, its number, its leader, its lair\'s stratum, its attitude, how it stands with the others, the worse of the two ways it\'s said), and each stratum\'s lairs: each faction\'s number spread over its strata, the most on its lair\'s, shared among its kinds (the first twice each other\'s share) in groups of its kind\'s size, its leader in its home; the same again (its own die, the stratum\'s seed)',
      init: 't_rc', cycles: 600e6,
      machine: { input: ['bankrun /sd/0/games/lib/dq0/popgen.bp /sd/0/games/lib/dq0 /sd/0/games/lib/dq0/inhabit.txt 3 >/ram/p1',
        'bankrun /sd/0/games/lib/dq0/popgen.bp /sd/0/games/lib/dq0 /sd/0/games/lib/dq0/inhabit.txt 3 >/ram/p2',
        'cat /ram/p1', 'cmp /ram/p1 /ram/p2 && echo same', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['popgen: 4 factions, 17 kinds\n',
        'Gnawers (rat bat): 40 on strata 0-3, their lair on 0; hostile to you; hostile to Diggers, wary of Brigands\n',
        'Diggers (kobold rat): 30 on strata 1-4, led by a koblord, their lair on 2; unfriendly to you; hostile to Gnawers, hostile to Brigands, hostile to Hollow\n',
        'Hollow (skel wight): 24 on strata 2-5, led by a shade, their lair on 4; hostile to you; hostile to Diggers\n',
        'stratum 0: rat 5 home, rat 4, rat 2, bat 5 (Gnawers); bandit 3 home, banlord 1 leader, bandit 3 (Brigands)\n',
        'stratum 4: kobold 4, rat 1 (Diggers); skel 4 home, shade 1 leader, skel 2, wight 2 (Hollow)\nstratum 5: skel 4, wight 2 (Hollow)\n',
        'echo same\nsame', '% echo done'],
      sdcard: true
    },
    {
      name: 'dq0inhab', what: 'Dingus Quest Zero\'s denizens from a theme\'s inhabitants (Phase 3D: the theme file in the save folder, theme.txt, its faction lines: games/dq0d/pop.h; written by hand here, the card\'s inhabit.txt, copied there before the game\'s begun): popgen (bank/popgen, a bank program of the denizens\' task\'s) places the theme\'s factions as each stratum is grown, in place of the stand-in\'s (kinds.txt\'s, factions.txt\'s): at seed 3 its Gnawers\' home, five giant rats (the stand-in\'s: two to five), by the top stratum\'s way down where the character starts (dq0 -T), and hostile to it (the theme\'s; the stand-in\'s vermin are wary), so they come at it while it searches; the game kept and carried on (dq0 -d), the rat there hostile still (the theme read again, its records the save\'s).  And the world\'s deities\' names (deities.txt\'s, the card\'s lib/dq0/dq0g.txt) the command parser\'s words for them (CQ_WORLD\'s third list, after the things\' words and the names given: dq0c keeps them)',
      init: 't_rc', cycles: 1200e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'look', 'examine the rat', 'search', 'quit', 'dq0 -d /ram/g', 'examine the rat', 'quit',
        'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['Two giant rats are ahead to your left, 15 feet away.', 'The giant rat seems hostile.', 'The giant rat bites at you',
        '% dq0 -d /ram/g', 'The giant rat seems hostile.', '% echo $status'],
      // (The theme's five by the way down, its home: two of them in sight as the game begins, the rest round the
      // corner; the stand-in's at seed 3 are wary, and keep their distance)
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['> examine the rat', 'The giant rat seems hostile.', '> search', 'The giant rat bites at you', '> quit',
          '% dq0 -d /ram/g', '> examine the rat', 'The giant rat seems hostile.', '> quit'], f);
        const want = Buffer.from(' \0\0' + fs.readFileSync(path.join(__dirname, '..', 'obj', 'games', 'dq0g.txt'), 'latin1') + '\0', 'latin1');
        if (!m.taskRam.some(r => Buffer.from(r.buffer, r.byteOffset, r.length).includes(want)))
          f.push('the deities\' names not the command parser\'s words (no task\'s RAM has them as CQ_WORLD\'s third list)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0reach', meaning: true, what: 'Dingus Quest Zero\'s things out of reach walked to first, places in a room, a blow\'s close-in and engagement (slice 4.0; HOUSE.md: out of reach, walked to first, going places, engagement; the user\'s, 2026-10-20), by the hub\'s events, the world\'s replies and where the character ends (the referee\'s dice at these seeds): at seed 3 the pick put down and walked away from, then "stow the pick" walks over to it (the things\' IR_NOT_HERE, I_TAKE\'s walk alone) and stows it as it gets there (the command carried out again: IR_OK); the room\'s centre, its far left corner, its near side and, walking forward, its other side gone to by the facing (I_GOTO: each square the room\'s own); at seed 7 the ghoul down the stairway attacked from two squares off: closed with (I_CLOSE: a walk), its blow back as the character comes beside it not a stop, and the character\'s blow struck; in another game the walk toward the far side stopped as it comes into the ghoul\'s zone (TE_ENGAGED) and, begun in it, after a square (TE_ENGAGED, engaged still)',
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T',
        'drop pick', 'walk to the door ahead', 'turn around', 'stow the pick', 'go to the centre of the room', 'go to the far left corner',
        'go to the near side', 'walk forward to the other side', 'quit', 'echo ml swd 80 >> /ram/c.hm',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'attack the ghoul', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'go to the far side',
        'go to the far side', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const st = T.of('stow the pick');
        if (!dq0Rep(st, 'ireq', { result: 'IR_NOT_HERE' }).length || !dq0Rep(st, 'ireq', { result: 'IR_OK' }).length) f.push('the pick not walked over to and stowed (' + st.replies.map(r => r.req + ' ' + (r.result || '')).join(', ') + ')');
        dq0Need(f, st, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'no walk over to the pick');
        const pk = dq0Thing(st.after, 'pick');
        if (!pk || pk.where !== 'IW_ITEM') f.push('the pick not in the pack: ' + (pk && pk.where));
        // (The places: the room's floor rx, ry, rw by rh, the character facing north as it ends each)
        const at = (t, what, want) => {
          const s = t.after, r = s && s.sight;
          if (!r || r.where !== 0) { f.push(t.line + ': not in a room'); return; }
          const w = want(r);
          if (s.pc.x !== w[0] || s.pc.y !== w[1]) f.push(t.line + ': at ' + s.pc.x + ',' + s.pc.y + ', not ' + what + ' ' + w.join(',') + ' (' + dq0Evs(t) + ')');
        };
        at(T.of('go to the centre of the room'), 'the centre', r => [r.rx + (r.rw - 1 >> 1), r.ry + (r.rh - 1 >> 1)]);
        at(T.of('go to the far left corner'), 'the far left corner', r => [r.rx, r.ry]);
        at(T.of('go to the near side'), 'the near side', r => [r.rx, r.ry + r.rh - 1]);
        at(T.of('walk forward to the other side'), 'the other side', r => [r.rx, r.ry]);
        const a = T.of('attack the ghoul'), gh = a.at && a.at.actors.find(x => x.name === 'ghoul');
        if (!gh) return [...f, 'no ghoul'];
        dq0Need(f, a, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'the ghoul not closed with');
        if (!dq0Ev(a, 'E_FIGHT', { who: 0, def: gh.k + 1 }).length) f.push('no blow struck at the ghoul as the character came up to it (' + dq0Evs(a) + ')');
        const g = T.filter(t => t.line === 'go to the far side');
        if (g.length !== 2) return [...f, 'the walks not typed'];
        if (!dq0Ev(g[0], 'E_TASK', { te: 'TE_ENGAGED', still: false }).length) f.push('the walk not stopped by the ghoul\'s zone (' + dq0Evs(g[0]) + ')');
        if (!dq0Ev(g[1], 'E_TASK', { te: 'TE_ENGAGED', still: true }).length) f.push('the walk begun engaged not stopped after a square (' + dq0Evs(g[1]) + ')');
        const p0 = g[0].after && g[0].after.pc, p1 = g[1].after && g[1].after.pc;
        if (p0 && p1 && Math.max(Math.abs(p1.x - p0.x), Math.abs(p1.y - p0.y)) > 1) f.push('more than a square walked engaged');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0sway', what: 'Dingus Quest Zero\'s attitudes and influence (DENIZENS.md; the user: "skills like animal husbandry could influence animals encountered"), by the denizens\' replies to the character\'s ways with them (the hub\'s sway: the result, the attitude before and after) and the actors\' records (the referee\'s Animalcraft tests in their order at this seed: HarnMaster\'s): a character good with animals (Animalcraft 90, hmchar\'s file) at seed 21\'s giant rats (wary, the vermin faction\'s), words refused (an animal: persuasion doesn\'t reach it), how one seems (examined: wary), befriended a test at a time (indifferent, then won over: an ally, following; then asked no more), named Grub, asked to wait and seen waiting as the character turns round, then to follow, following it across the room (seen beside it as it turns round again) and down the stairway (come along to the stratum below); the game kept (dq0 -d) and carried on, Grub its ally still; then a fresh game, a blow at a rat out of reach not struck, the rat as wary as it was',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', 'echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -d /ram/kept',
        'persuade the rat', 'examine the rat', 'befriend the rat', 'befriend the rat', 'befriend the rat', 'name the rat grub', 'tell grub to wait', 'turn around',
        'follow me', 'go east', 'turn around', 'd', 'quit', 'dq0 -d /ram/kept', 'examine grub', 'quit', 'dq0 -p t -s 3 -c /ram/a.hm -T', 'attack the rat',
        'examine the rat', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const sw = (t, want, what) => { if (!dq0Rep(t, 'sway', want).length) f.push(what + ' (' + JSON.stringify(t.replies.filter(r => r.req === 'sway')) + ')'); };
        sw(T.of('persuade the rat'), { kind: 'SW_PERSUADE', result: 'SR_BEAST' }, 'words not refused for an animal');
        sw(T.of('examine the rat'), { kind: 'SW_LOOK', result: 'SR_OK', now: 'AT_WARY' }, 'the rat not seen wary');
        const b = T.filter(t => t.line === 'befriend the rat');
        sw(b[0], { kind: 'SW_BEFRIEND', result: 'SR_OK', was: 'AT_WARY', now: 'AT_INDIFFERENT' }, 'the first coaxing not swaying it to indifferent');
        sw(b[1], { kind: 'SW_BEFRIEND', result: 'SR_OK', was: 'AT_INDIFFERENT', now: 'AT_ALLIED' }, 'the second not winning it over');
        sw(b[2], { kind: 'SW_BEFRIEND', result: 'SR_AS_FAR' }, 'the third not refused (asked no more)');
        const grub = s => s ? s.actors.find(a => a.name === 'Grub') : null, near = s => { const g = grub(s); return g && g.sn === s.pc.sn && Math.max(Math.abs(g.x - s.pc.x), Math.abs(g.y - s.pc.y)) <= 2; };
        const seen = (t, doing) => t.after && t.after.sight && t.after.sight.dens.some(d => d.name === 'Grub' && d.doing === doing);
        const nm = T.of('name the rat grub');
        sw(nm, { kind: 'SW_NAME', result: 'SR_OK' }, 'the rat not named');
        if (!grub(nm.after) || grub(nm.after).attitude !== 'AT_ALLIED') f.push('no ally Grub');
        const w = T.of('tell grub to wait'), ta = T.after(w, 'turn around'), fm = T.of('follow me'), ge = T.of('go east'), ta2 = T.after(ge, 'turn around');
        sw(w, { kind: 'SW_ORDER', order: 'OR_WAIT', result: 'SR_OK' }, 'Grub not asked to wait');
        if (!seen(ta, 'DO_WAIT')) f.push('Grub not seen waiting as the character turned round');
        sw(fm, { kind: 'SW_ORDER', order: 'OR_FOLLOW', result: 'SR_OK' }, 'Grub not asked to follow');
        dq0Need(f, ge, 'E_ARRIVE', {}, 'the room not crossed');
        if (!seen(ta2, 'DO_FOLLOW') || !near(ta2.after)) f.push('Grub not seen following across the room, beside the character');
        const d = T.of('d');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        if (!d.after || d.after.pc.sn !== 1 || !near(d.after)) f.push('Grub not come along down the stairway');
        const c = T.of('dq0 -d /ram/kept');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        sw(T.of('examine grub'), { kind: 'SW_LOOK', result: 'SR_OK', now: 'AT_ALLIED' }, 'Grub not an ally still, carried on');
        const a = T.of('attack the rat');
        if (dq0Ev(a, 'E_FIGHT', { who: 0 }).length) f.push('a blow struck at a rat out of reach');
        sw(T.after(a, 'examine the rat'), { kind: 'SW_LOOK', now: 'AT_WARY' }, 'the rat not as wary as it was');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0prompt', what: 'Dingus Quest Zero answering at once at the normal pace while the denizens act (DENIZENS.md: events; the user: "the UI is slow to respond"): the giant rats by the way down moving about on their own (their task\'s, in the background), a while waited, then a turn and what the character carries asked for at once, while the senses look about after the turn (the character still busy): told within a fifth of a second of Enter, before the look\'s done (the senses answer it between the look\'s rows, the UI a command that wants nothing of the character at once), and a step that can\'t be taken (up, with no way up) told as quickly (the world\'s answer); the status line drawn only where it changed, a second at most (the user: the cursor flickered), the cursor hidden while it\'s there',
      meaning: true,
      init: 't_rc', cycles: 900e6,
      machine: { input: 'ābind -a /sd/0/games /bin\rāhmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\rādq0 -p n -s 3 -c /ram/c.hm -T\r' +
        'ĀĀāturn left\rinventory\rāgo up\rĀĀĀĀāquit\rāecho $status\r' },
      expect: ['You carry', 'There\'s no way up or down here.', '% echo $status'],
      // (The inventory's budget a guard against a large slowdown at 9,600 baud, not a target: 1,410,000 to 1,530,000
      // cycles measured, as the rats' stirring falls on the line before it; the user's targets are 115,200's, D17:
      // dq0answer's)
      budgets: [{ what: 'inventory answered (Enter to the answer)', from: '> inv', to: 'You carry', per: 1, max: 2000000 },
        { what: 'a step that can\'t be taken answered (the world\'s: from the line told into the story)', from: '\n> go up',
          to: 'There\'s no way up or down here.', per: 1, max: 600000 }],
      // (Each a mark in what the serial line sends: the inventory's from the line typed at the bottom, a fifth of a
      // second, the user's; the step's from the line entered, as it's told into the story (on a line of its own: the
      // line typed's row, 24, after it, or the status line's clock first, as the second falls): about 285,000 cycles to "go up"'s answer since the world sleeps on its
      // doorbell (the hub's protocol, SEM_WAIT: the UI's intent wakes it at once; 354,000 when it napped up to 4 ticks
      // between looks): some 150,000 the world's (its climbing part swapped in for it), 130,000 the UI's words, a rat's
      // step about then or not.  From the line typed it was
      // 380,000 or 600,000 cycles, as that echo went out at once or waited behind the words still going out on the
      // line: when Enter fell against what the line was sending, moved by any task's code, not the answer's speed)
      // (While nothing happens, between the step and quitting, 8M cycles, 2.2 s: the status line's clock, a move up to
      // it a second at most, the cursor hidden for it)
      // (Move 2.5: the inventory's mark its typed line's first letters, '> inv': '> inventory' met the line told into
      // the story, just before the answer, only when the status line's clock split the typed echo (87,000 cycles then);
      // from the line typed it was some 3.3M cycles since move 2.3 (the world's answer, its swapped part's words,
      // about 1.1M of it, the UI's parsing and words the rest), 2.6M now, the senses' look giving way to the command
      // in hand at the real paces: a fifth of a second (1.6M) not met yet, the world's words for it to quicken)
      // (The speed pass: 690,000 to 900,000 now, as the code lies (a fifth of a second met in some builds, not all): the
      // UI's loop looking at the keys every four ticks, not two; the world's list in one pass of the store, part 11
      // alone; the line a command in hand (the look giving way) from its first parse, at_once's, which command takes as
      // it is; the verbs' and the paces' words looked for by their first letters; the look giving way at more of its
      // stages.  What's left: the line typed while the UI still says the turn (its words wait for the serial line,
      // full of the rats' stirring at 9,600 baud), the typed echo's last letters with the answer, 600,000 cycles after
      // its first; and the world's list, 200,000 of its own)
      // (reborn-game-stream: these at 9,600 baud, the user's D17 (the targets are 115,200's: dq0answer).  The turn's first
      // words told as it begins (V_FACING), ahead of what it carries now, and the cursor's moves to the line typed and
      // back round them, some 60 characters at 3,730 cycles each: the inventory 1,157,000 from its typed letters (887,000
      // before), a budget of 1,450,000; and the line entered told into the story as it's entered, not with the answer,
      // so from it the step's answer has the UI's parse and the world's climb in it: 456,000, a budget of 600,000 (from
      // Enter it's quicker: dq0answer96's go up, some 560,000, 575,000 before).  What it carries is still told before
      // the turn's look, its words after its first)
      check(m, out) {
        const a = out.indexOf('There\'s no way up or down here.'), b = out.indexOf('> quit', a), seg = a < 0 || b < 0 ? '' : out.slice(a, b);
        const top = (seg.match(/\x1b\[1;\d+H/g) || []).length, f = [];
        if (top > 4) f.push(top + ' moves up to the status line while nothing happened (a second at most each)');
        const turned = out.indexOf('You turn to face'), looked = turned < 0 ? -1 : turned + out.slice(turned).search(/You (can see|are in)/);
        if (looked < turned || out.indexOf('You carry') > looked) f.push('what it carries told only after the turn\'s look');
        for (const s of seg.matchAll(/\x1b\[1;\d+H/g)) {      // (Hidden as it moved there: hidden last, here or before)
          const at = a + s.index;
          if (out.lastIndexOf('\x1b[?25l', at) <= out.lastIndexOf('\x1b[?25h', at)) { f.push('the cursor not hidden while the status line was drawn'); break; }
        }
        return f;
      },
      sdcard: true
    },
    dq0AnswerTest(115200),
    // (dq0answer96, the same at 9,600 baud: its times noted, no budget of its own (the user's targets, D17, are 115,200's):
    // only with DQ0ANS96=1, to measure)
    ...(process.env.DQ0ANS96 ? [dq0AnswerTest(9600)] : []),
    {
      name: 'dq0looks', what: 'Dingus Quest Zero\'s looks the same as they were (the look quickened, reborn-game-look: its grid kept and moved, its marks put only where they reach, the store gone through once): eight games at the turns pace (seeds 3, 5, 7, 12, 4, 517: turns, steps, walks, a walk\'s chosen door, a stairway down, a torch put out and lit again, one put down and taken up, a portcullis lowered and raised, a window broken and climbed through, a weak-eyed character), every reply the senses give (a look\'s, the denizens\' senses\') with the same request, the same record of what the character perceives and the same squares seen and visible in every live chunk as tests/dq0/looks.ref, the old code\'s (DQ0LOOKS=write: written afresh from this run)',
      init: 't_rc', cycles: 2500e6,
      machine: { input: 'ābind -a /sd/0/games /bin\rāhmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\rācp /ram/c.hm /ram/w.hm\rāecho att eye 4 >> /ram/w.hm\r' +
        [['dq0 -p t -s 3 -c /ram/c.hm -T -k sword',['look','turn left','turn right','walk to the door ahead to my left','2','look','turn around','turn left','turn right','map','d','look','sw','look']],['dq0 -p t -s 5 -c /ram/c.hm -T',['d','wait','go right','turn left','look','turn around','wait','look']],['dq0 -p t -s 7 -c /ram/w.hm',['open the door behind me','s','move south 4 squares','face north','look','douse the torch','z','z','z','look','light the torch','z','look','turn around','n']],['dq0 -p t -s 3 -c /ram/c.hm -T',['drop the torch','turn around','step forward','look','turn around','step forward','get torch','look']],['dq0 -p t -s 12 -c /ram/c.hm -T',['e','wait','e','wait','e','turn around','lower portcullis','w','look','raise portcullis','w','turn left']],['dq0 -p t -s 12 -c /ram/c.hm',['n','turn around','break window','climb through window','turn around','look','turn right']],['dq0 -p t -s 4 -c /ram/c.hm',['open door','3','look','go through the door','look','turn left']],['dq0 -p t -s 517 -c /ram/c.hm',['look','turn left','turn right','turn around','look']]].map(([g, c]) => 'ā' + g + '\r' + c.concat('quit').map(l => 'ā' + l + '\r').join('')).join('') + 'āecho $status\r' },
      expect: ['% echo $status'],
      // (The senses' task found by its code, dq0p.hyx's at its main; its hub_reply's calls snapped, as the game it's in
      // the quits told so far)
      start(m) {
        const dir = path.join(__dirname, '..', 'obj', 'games'), hyx = fs.readFileSync(path.join(dir, 'dq0p.hyx'));
        const lbl = fs.readFileSync(path.join(dir, 'dq0p', 'dq0p.lbl'), 'latin1'), at = n => parseInt(new RegExp('^al ([0-9A-F]+) \\.' + n + '$', 'm').exec(lbl)[1], 16);
        const main = at('_main'), reply = at('_hub_reply'), lines = this.lines = [], cpu = m.cpu, step = cpu.step.bind(cpu);
        let lastT = -1, isP = false, game = 0, qpos = 0, out = m.out;
        cpu.step = v => {
          const T = m.T;
          if (T !== lastT) { lastT = T; const r = m.taskRam[T]; isP = true; for (let i = 0; i < 24; i++) if (r[main + i] !== hyx[main - 0x800 + i]) { isP = false; break; } }
          if (isP && cpu.PC === reply) { const l = lookSnap(m, cpu.A | cpu.X << 8, game); if (l) lines.push(l); }
          step(v);
        };
        Object.defineProperty(m, 'out', { get: () => out, set: v => { let i; while ((i = v.indexOf('> quit', qpos)) >= 0) { game++; qpos = i + 6; } out = v; } });
      },
      check(m, out) {
        const now = this.lines || [];
        if (process.env.DQ0LOOKS === 'write') { fs.writeFileSync(LOOKS_REF, now.join('\n') + '\n'); return []; }
        const ref = fs.readFileSync(LOOKS_REF, 'utf8').split(/\r?\n/).filter(l => l), f = [];
        if (now.length !== ref.length) f.push(now.length + ' replies, ' + ref.length + ' in tests/dq0/looks.ref');
        for (let i = 0; i < Math.min(now.length, ref.length) && f.length < 4; i++) {
          if (now[i] !== ref[i]) f.push('reply ' + i + ': ' + now[i].slice(0, 120) + ' (looks.ref: ' + ref[i].slice(0, 120) + ')');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0heard', what: 'Dingus Quest Zero\'s denizens\' events perceived by some and not others (DENIZENS.md: events), by the denizens\' events, what the character perceives and the YM2151\'s key-ons: down the ladder at seed 5, the torch put out: in the dark the giant rats move about unseen (their steps told, none on a square the character sees; none seen, nothing told as stirring) but heard, near enough for the character\'s hearing (the referee\'s), by the way they lie (SOUND.md: the UI\'s "You hear a scurrying to your right."), and played as they\'re heard (the sound task\'s: the YM2151\'s channels 4-6, on the side they lie: a rat to the right on the right speaker); the turns pace, the denizens kept up a slice at a time; its cave, seed 5\'s small stratum, irregular, every square reached',
      init: 't_rc', cycles: 700e6,
      machine: { ymLog: true, input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -T', 'd',
        'douse the torch', 'wait', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], x = T.of('douse the torch'), w = T.of('wait'), ons = YM_KEYPANS(m);
        if (!x.after || !x.after.sight || !x.after.sight.dark) f.push('not dark, the torch put out');
        const dark = [x, w], steps = dark.flatMap(t => dq0Ev(t, 'E_DEN', { de: 'DE_MOVED' }));
        if (!steps.length) f.push('the rats not moving about in the dark');
        if (steps.some(e => e.seen)) f.push('a rat\'s step on a square the character sees, in the dark');
        for (const t of dark) {
          if (t.after && t.after.sight && t.after.sight.dens.length) f.push(t.line + ': a rat seen in the dark');
          dq0Never(f, t, 'E_VIEW', { why: 'V_STIR' }, t.line + ': a stirring told in the dark');
        }
        const o = dq0Story(out), seg = o.slice(o.indexOf('> douse the torch'), o.indexOf('> quit'));
        if (!/You hear [^.]* to your right\./.test(seg)) f.push('the rats not heard to the right');
        if (o.includes('You hear something')) f.push('a sound told without its words (the sound task\'s table not in the hub)');
        if (!ons.some(k => k === '4R' || k === '5R' || k === '6R')) f.push('no rat played on the right: ' + ons.join(' '));
        if (ons.some(k => !/^[456]/.test(k))) f.push('a key-on off the sound task\'s channels: ' + ons.join(' '));
        this.notes = ['the YM2151\'s key-ons: ' + ons.join(' ')];
        f.push(...caveReport(m, 1, 1));
        return f;
      },
      sdcard: true
    },
    {
      // (Seed 7: the kitchen the character begins in lit by its own torches; its south door's dart trap set off as
      // it's opened, then down the passage into a dark hall and to its far side, 14 squares from the kitchen's door,
      // facing back up the passage: a weak-eyed character's look reaches 4)
      name: 'dq0glow', what: 'Dingus Quest Zero\'s lights seen far off and eyes adapting to the dark (DUNGEON.md: perception as layers; the user\'s, 2026-10-16: "if I am standing in the dark, I should be able to see other light sources from much further away"; eyes take minutes to the dark), by what the character perceives (the senses\' record: the lights far off, the eyes dazzled): at seed 7, a weak-eyed character (Eyesight 4: its look reaches 4 squares) in a dark hall 14 squares from the lit kitchen\'s open door, up a straight passage: holding its torch, no light far off (its eyes used to the torch\'s light); its torch put out: at once nothing far off (its eyes still used to the torch), after 2 minutes still nothing, after 3 minutes and 20 seconds a faint glow far off ahead, after 5 minutes a light (16 b / (4 + a): 19 squares, its eyes used to the dark; the eyes the senses\', a 16th of a light level each 1.9 s darker); its torch lit again, dazzled a moment, and the light lost (dq0dark\'s, in it)',
      init: 't_rc', cycles: 700e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/w.hm', 'echo att eye 4 >> /ram/w.hm',
        'dq0 -p t -s 7 -c /ram/w.hm', 'open the door behind me', 's', 'move south 4 squares', 'face north', 'look', 'douse the torch', 'look',
        ...Array(12).fill('z'), 'look', ...Array(8).fill('z'), 'look', ...Array(10).fill('z'), 'look', 'light the torch', 'z', 'look', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], looks = T.filter(t => t.line === 'look'), x = T.of('douse the torch'), lt = T.of('light the torch');
        if (looks.length !== 6) return ['the looks not typed'];
        const glows = t => t.after && t.after.sight ? t.after.sight.glows : [{ how: '?' }], ahead = (t, how) => glows(t).some(g => g.rel === 0 && g.how === how);
        const at = ['holding the torch', 'at once in the dark', 'after 2 minutes'];
        looks.slice(0, 3).forEach((t, i) => { if (glows(t).length) f.push('a light far off seen ' + at[i] + ': ' + JSON.stringify(glows(t))); });
        if (!x.after || !x.after.sight || !x.after.sight.dark) f.push('not dark, the torch put out');
        if (!ahead(looks[3], 'GL_GLOW')) f.push('no faint glow ahead after 3 minutes 20: ' + JSON.stringify(glows(looks[3])));
        if (!ahead(looks[4], 'GL_LIGHT')) f.push('no light far off ahead after 5 minutes: ' + JSON.stringify(glows(looks[4])));
        if (!lt.after || !lt.after.sight || !lt.after.sight.dazzled) f.push('not dazzled, the torch lit again');
        if (glows(lt).length || glows(looks[5]).length) f.push('the light far off still seen, the torch lit');
        if (!looks[5].after || looks[5].after.sight.dazzled) f.push('still dazzled a round after');
        return f;
      },
      sdcard: true
    },
    {
      // (Seed 7, dq0glow's dungeon: the kitchen the character begins in, lit by its own torches, its south door opened
      // (its dart trap set off) and down the straight passage south to a dark hall; the kitchen's floor ends at row
      // 32840, its door at 32841, the hall's north door at 32850, and an iron pot lies on the kitchen's floor at 32785,
      // 32839, straight up the passage from the hall's doorway square, 32785,32851, and its far side, 32785,32855; the
      // hall's east door, at its far side's 32787,32855, closed.  A character of Eyesight 5: HarnMaster's sight 1, a
      // glance's reach 6 squares, a look one way's 12; the null ruleset's sight 5, 14 and 28)
      name: 'dq0lookway', meaning: true, what: 'Dingus Quest Zero\'s looking one way, listening and sniffing one way (HOUSE.md\'s slice 4.0, the user\'s: "look north", "look at the door", "listen at the door"; told by the facing alone), by the hub\'s events and the senses\' record (what the character perceives, and a look one way\'s own: WAY_SEEN): at seed 7 in the dark hall below the lit kitchen, a glance and then a look north from its doorway and from its far side: a look one way reaches twice a glance\'s squares (the ruleset\'s sight: 4 + 2 sight) in the quarter ahead, so the kitchen\'s iron pot straight up the passage is in the glance\'s record only within its reach and in the look one way\'s within twice it, and one of the two looks sees it where the glance doesn\'t; the look one way\'s own record what\'s beyond the hall\'s open door, the kitchen, lit (a room\'s kind, U_KITCHEN), or only the dark past the look\'s reach; each look one way the world\'s I_LOOK, E_VIEW V_LOOKED, 3 seconds; looking east at the closed door, turned to it first (V_FACING), not seen past (WB_SHUT); listening at the door, turned to it already, 3 seconds and no look, the watchfulness not set (not "listen as I go"); listening about and sniffing, 3 seconds each',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c clothier -o /ram/c.hm', 'cp /ram/c.hm /ram/e.hm', 'echo att eye 5 >> /ram/e.hm',
        'dq0 -p t -s 7 -c /ram/e.hm', 'open the door behind me', 's', 'turn around', 'look north', 'move south 4 squares', 'face north',
        'look north', 'look east', 'listen at the door', 'listen', 'sniff', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], at = (t, what) => t.k >= 0 && t.at && t.after && t.after.sight && t.after.way ? true : (f.push(what + ' not typed, or no record'), false);
        const ln = T.filter(t => t.line === 'look north'), glance = [T.of('turn around'), T.of('face north')];
        if (ln.length !== 2 || !glance.every((g, i) => at(g, 'the glance ' + i)) || !ln.every((t, i) => at(t, 'look north ' + i))) return f.concat(['the looks not typed']);
        const pot = (ln[1].after.items || []).find(i => i.name === 'iron pot' && i.x === 32785 && i.where === 'IW_SQUARE');
        if (!pot) return ['the kitchen\'s iron pot not found at 32785'];
        const has = s => s.items.some(i => i.name === 'iron pot' && i.rel === 0);
        let more = 0;
        ln.forEach((t, i) => {
          const r = 4 + 2 * Math.min(t.at.sum.sight, 9), w = Math.min(2 * r, 31), g = glance[i].after, s = t.after.sight, d = t.after.pc.y - pot.y, k = t.after.pc.y - 32840;
          const what = 'look north ' + i + ' (' + d + ' squares from the pot, a glance\'s reach ' + r + ')';
          if (t.after.pc.face !== 0 || t.after.pc.x !== 32785) f.push(what + ': not facing north up the passage');
          if (has(g.sight) !== (d <= r)) f.push(what + ': the glance ' + (d <= r ? 'doesn\'t see' : 'sees') + ' the pot');
          if (has(s) !== (d <= w)) f.push(what + ': the look one way ' + (d <= w ? 'doesn\'t see' : 'sees') + ' the pot');
          more += !has(g.sight) && has(s);
          dq0Need(f, t, 'E_VIEW', { why: 'V_LOOKED' }, what + ': no V_LOOKED');
          dq0Never(f, t, 'E_VIEW', { why: 'V_FACING' }, what + ': turned, facing north already');
          if (t.after.now - t.at.now !== 30) f.push(what + ': ' + (t.after.now - t.at.now) + ' tenths, not 30');
          const b = t.after.way.valid ? t.after.way.beyond.find(e => e.rel === 0 && e.noun === 'BN_OPEN') : null;
          if (!b) f.push(what + ': nothing beyond the open door ahead in the look one way\'s own: ' + JSON.stringify(t.after.way));
          else if (k <= w ? b.what !== 'WB_ROOM' || b.kind !== 8 || !b.lit : b.what !== 'WB_DARK') f.push(what + ': beyond the door ' + JSON.stringify(b) + ', the kitchen ' + k + ' squares off');
          if (has(s) && !t.after.way.items.some(e => e.name === 'iron pot')) f.push(what + ': the pot not among the things ahead');
        });
        if (!more) f.push('no look one way saw the pot where the glance didn\'t');
        const e = T.of('look east'), l = T.of('listen at the door');
        if (at(e, 'look east')) {
          dq0Need(f, e, 'E_VIEW', { why: 'V_FACING' }, 'look east: not turned first');
          dq0Need(f, e, 'E_VIEW', { why: 'V_LOOKED' }, 'look east: no V_LOOKED');
          if (e.after.pc.face !== 2) f.push('look east: facing ' + e.after.pc.face);
          if (!e.after.way.valid || !e.after.way.beyond.some(b => b.rel === 0 && b.noun === 'BN_DOOR' && b.what === 'WB_SHUT')) f.push('look east: the closed door not shut to the look: ' + JSON.stringify(e.after.way));
        }
        for (const [x, face] of [[l, 2], [T.of('listen'), 2], [T.of('sniff'), 2]]) {
          if (!at(x, x.line)) continue;
          dq0Never(f, x, 'E_VIEW', { why: 'V_LOOKED' }, x.line + ': a look');
          dq0Never(f, x, 'E_VIEW', { why: 'V_FACING' }, x.line + ': turned');
          if (x.after.pc.face !== face) f.push(x.line + ': facing ' + x.after.pc.face);
          if (x.after.now - x.at.now !== 30) f.push(x.line + ': ' + (x.after.now - x.at.now) + ' tenths, not 30');
          if (x.after.pc.watch & 2) f.push(x.line + ': listening as it goes set (WA_LISTEN)');
        }
        return f;
      },
      sdcard: true
    },
    {
      // (The sound task's scene, dq0s -t: the character faces north, its hearing 5; then the YM2151's key-ons, each
      // with its speakers: the rat's three hits on the right, the squeak on the left, the growl on both, the
      // yapping on the right, the bear's tread in its place on the left; nothing on 0-3 or 7, the music's and bell's)
      name: 'dq0sound', what: 'Dingus Quest Zero\'s sound (SOUND.md, the user: "if I am facing N and the sound is coming from the E, then I hear it in my right ear"; dq0s -t, its scene): a giant rat\'s step two squares east of a character facing north played on the right speaker (the YM2151\'s channel 4, its RL bits); then five sources at once, more than the three channels: the three loudest played (the squeak to the left on the left, the growl ahead on both, the yapping behind a shut door muffled, on the right), the two quietest not; and a bear\'s tread beside it, louder, in the quietest\'s place; the music\'s and the bell\'s channels left alone',
      meaning: true,
      init: 't_rc', cycles: 200e6,
      machine: { ymLog: true, input: 'ā/sd/0/games/lib/dq0/dq0s -t /sd/0/games/lib/dq0\rāecho $status\r' },
      expect: ['1 a scurrying: channel 4, right, level 48\n3 a squeak: channel 4, left, level 76\n' +
        '2 a low growl: channel 5, both, level 64\n6 a yapping: channel 6, right, muffled, level 52\n' +
        '4 a shuffling tread: not played, level 39\n5 a dry clatter: not played, level 36\n' +
        '7 a heavy tread: channel 6, left, in place of 6, level 97\n%', '% echo $status\n\n%'],
      check(m) {
        const ons = YM_KEYPANS(m), f = [], want = ['4R', '4L', '5B', '6R', '6L'];
        let i = 0;
        for (const o of ons) if (o === want[i]) i++;
        if (ons[0] !== '4R') f.push('the rat to the east not first, on the right: ' + ons.join(' '));
        if (i < want.length) f.push('the key-ons not ' + want.join(', ') + ' in turn: ' + ons.join(' '));
        if (ons.some(o => !/^[456]/.test(o))) f.push('a key-on off the sound task\'s channels: ' + ons.join(' '));
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        this.notes = ['the YM2151\'s key-ons: ' + ons.join(' ')];
        return f;
      },
      sdcard: true
    },
    {
      // (With a Vera X: the noises on the PSG's voices first (20-23: voices 12-15), so all five played and the
      // bear's tread on a voice free; the voices' speakers as they were last: the rat east's 12, then the
      // tread's behind (both); the clatter's (east: right) on 13, the bear's tread (west: left) on 14)
      name: 'dq0soundx', what: 'Dingus Quest Zero\'s sound with a Vera X (dq0s -t): the PSG\'s voices 12-15 the sound task\'s too, seven channels: a rat\'s scurrying to the east on voice 12 on the right, the five sources at once all played (the noises on the PSG, the tones on the YM2151), the bear\'s tread on a voice free, on the left',
      meaning: true,
      init: 't_rc', cycles: 200e6,
      machine: { ymLog: true, vera: true, input: 'ā/sd/0/games/lib/dq0/dq0s -t /sd/0/games/lib/dq0\rāecho $status\r' },
      expect: ['1 a scurrying: channel 20, right, level 48\n3 a squeak: channel 4, left, level 76\n' +
        '2 a low growl: channel 5, both, level 64\n6 a yapping: channel 6, right, muffled, level 52\n' +
        '4 a shuffling tread: channel 20, both, level 39\n5 a dry clatter: channel 21, right, level 36\n' +
        '7 a heavy tread: channel 22, left, level 97\n%', '% echo $status\n\n%'],
      check(m) {
        const f = [], lr = v => '-LRB'[m.vera.psg[4 * v + 2] >> 6], ons = v => m.vera.psgOns.filter(k => k.startsWith('voice ' + v + ' ')).length;
        for (const [v, n, s] of [[12, 4, 'B'], [13, 3, 'R'], [14, 2, 'L']]) {
          if (ons(v) < n) f.push('PSG voice ' + v + ' on ' + ons(v) + ' times, not ' + n);
          if (lr(v) !== s) f.push('PSG voice ' + v + '\'s speakers ' + lr(v) + ', not ' + s);
        }
        if (ons(15)) f.push('PSG voice 15 on: nothing played there');
        this.notes = ['the PSG\'s voices on: ' + m.vera.psgOns.map(k => k.split(' ')[1]).join(' ')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0portc', meaning: true, what: 'Dingus Quest Zero\'s portcullises (DUNGEON.md: portals; the world works them, dq0w/wwin.c), by the world\'s events and what the character perceives: at seed 514 the kobold warren\'s gatehouse (its every way out a portcullis, raised), the one ahead lowered by its winch (walked up to first), seen down, never walked through while it\'s down, raised again by the winch, seen up, and gone through (broken, bent and the windows: dq0break)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 514 -c /ram/c.hm', 'lower portcullis', 'n',
        'raise portcullis', 'n', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], seen = (t, open) => t.after && t.after.sight && t.after.sight.doors.some(d => d.rel === 0 && d.kind === 'PK_PORTCULLIS' && (d.state & 1) === open);
        const s0 = T.of('dq0 -p t -s 514 -c /ram/c.hm');
        if (!seen(s0, 1)) f.push('no raised portcullis ahead to begin with');
        const lo = T.of('lower portcullis'), n = T.of('n'), r = T.of('raise portcullis'), n2 = T.after(r, 'n');
        dq0Need(f, lo, 'E_PORTAL', { po: 'PO_LOWERED', pk: 'PK_PORTCULLIS' }, 'the portcullis not lowered');
        if (!seen(lo, 0)) f.push('the portcullis ahead not seen down');
        dq0Need(f, n, 'E_NOWAY', { why: 'N_CLOSED' }, 'walked through the portcullis down');
        if (!n.at || !n.after || n.after.pc.x !== n.at.pc.x || n.after.pc.y !== n.at.pc.y) f.push('moved, the portcullis down');
        dq0Need(f, r, 'E_PORTAL', { po: 'PO_RAISED', pk: 'PK_PORTCULLIS' }, 'the portcullis not raised');
        if (!seen(r, 1)) f.push('the portcullis ahead not seen up');
        dq0Need(f, n2, 'E_ARRIVE', {}, 'not gone through the portcullis raised');
        if (!n2.at || !n2.after || !(n2.after.pc.y < n2.at.pc.y)) f.push('not gone north through it');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0break', what: 'Dingus Quest Zero\'s portcullises and windows broken (the user\'s, 2026-10-16; DUNGEON.md: portals), by the world\'s events (the referee\'s blows and Strength: HarnMaster\'s, at these seeds): at seed 514\'s gatehouse a portcullis lowered by its winch, then battered with a pick (dq0 -k pick: blows at its iron bars off their body, as a door\'s) till they break, a gap through for good (battered again, jammed; examined; it won\'t lower), and gone through; then a strong character (Strength 18) bending its bars apart (P_FORCE), and through; then at seed 12 a glazed window not climbed through while its glass is in it, the glass broken (the referee\'s blow at a thing, HC_BLOW at RS_THING), climbed through (its sill, P_CLIMB) into the passage beyond, the window seen from there, its glass gone (dq0portcbreak\'s and dq0window\'s, these together)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 514 -c /ram/c.hm -k pick', 'lower portcullis',
        'break portcullis', 'break', 'x portcullis', 'lower portcullis', 'n', 'quit', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/s.hm', 'echo att str 18 >> /ram/s.hm',
        'dq0 -p t -s 514 -c /ram/s.hm', 'lower portcullis', 'bend the bars', 'bend bars', 'n', 'quit',
        'dq0 -p t -s 12 -c /ram/c.hm', 'n', 'turn around', 'climb through window', 'break window', 'climb through window', 'turn around', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], g1 = T.of('dq0 -p t -s 514 -c /ram/c.hm -k pick'), g2 = T.of('dq0 -p t -s 514 -c /ram/s.hm');
        const lo = T.after(g1, 'lower portcullis'), b = T.of('break portcullis'), b2 = T.of('break'), x = T.of('x portcullis'), lo2 = T.after(x, 'lower portcullis');
        dq0Need(f, lo, 'E_PORTAL', { po: 'PO_LOWERED' }, 'the portcullis not lowered');
        dq0Need(f, b, 'E_PORTAL', { po: 'PO_GAP' }, 'its bars not broken through with the pick');
        dq0Need(f, b2, 'E_PORTAL', { po: 'PO_JAMMED' }, 'battered again, not jammed for good');
        if (!dq0Rep(x, 'wreq', { kind: 'WQ_EXAMINE', res: 0 }).length) f.push('the portcullis not examined');
        dq0Need(f, lo2, 'E_PORTAL', { po: 'PO_JAMMED' }, 'lowered again, not jammed');
        dq0Need(f, T.after(lo2, 'n'), 'E_ARRIVE', {}, 'not through the gap');
        const lo3 = T.after(g2, 'lower portcullis'), bend = [T.of('bend the bars'), T.of('bend bars')];
        dq0Need(f, lo3, 'E_PORTAL', { po: 'PO_LOWERED' }, 'the portcullis not lowered (the strong character)');
        if (!bend.some(t => dq0Ev(t, 'E_PORTAL', { po: 'PO_BENT' }).length)) f.push('its bars not bent apart (' + bend.map(dq0Evs).join('; ') + ')');
        dq0Need(f, T.after(bend[1], 'n'), 'E_ARRIVE', {}, 'not through the bent bars');
        const g3 = T.of('dq0 -p t -s 12 -c /ram/c.hm'), c1 = T.after(g3, 'climb through window'), bw = T.of('break window'), c2 = T.after(bw, 'climb through window');
        dq0Need(f, c1, 'E_PORTAL', { po: 'PO_IN_WAY', pk: 'PK_WINDOW' }, 'climbed through the glass');
        dq0Need(f, bw, 'E_PORTAL', { po: 'PO_SHATTERED', pk: 'PK_WINDOW' }, 'the glass not broken');
        dq0Need(f, c2, 'E_PORTAL', { po: 'PO_CLIMBED' }, 'not climbed through the window');
        dq0Need(f, c2, 'E_ARRIVE', { how: 'A_CLIMBED' }, 'not come through the window');
        const ta = T.after(c2, 'turn around');
        if (!ta.after || !ta.after.sight || ta.after.sight.where !== 1 || !ta.after.sight.doors.some(d => d.kind === 'PK_WINDOW' && !(d.parts & 1)))
          f.push('the window, its glass gone, not seen from the passage beyond');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0words', what: 'Dingus Quest Zero\'s words, a test about wording alone (the others check meaning: dq0words, dq0wordsfix and dq0wordsthings are where the game\'s sentences are held to): rooms and what\'s seen as the user asked (2026-10-15 and 16): at seed 3 a room told in general words at a glance (a small kitchen, a barrel vault high overhead), the door behind not told ("descriptions often tell you what lies behind you, which you can\'t really see"), its figures (its size in feet, its ceiling\'s height) only on a close look (look closely, x room); the door behind told as it turns round, examined in full (its make, size and thickness); at seed 5 a close look at a hall whose far side is dark, no figures, its far side lost in the dark; at seed 11 a door across the pantry examined, only its look from afar (tall, of wood, how far, go closer), and by a weak-eyed character (Eyesight 4: "Eyesight score should affect the amount of detail") not even its wood (dq0doors\' and dq0sense\'s words)',
      init: 't_rc', cycles: 1200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/w.hm', 'echo att eye 4 >> /ram/w.hm',
        'dq0 -p t -s 3 -c /ram/c.hm', 'look', 'look closely', 'x room', 'turn around', 'examine the door ahead to my left', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm', 'x room', 'quit', 'dq0 -p t -s 11 -c /ram/c.hm', 'examine the door ahead to my left', 'quit',
        'dq0 -p t -s 11 -c /ram/w.hm', 'examine the door ahead to my left', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['% dq0 -p t -s 3', '> look You are in the middle of a small kitchen', 'A barrel vault arches high overhead. There is a door ahead to your left (open).',
          '> look closely', 'It is 10 feet by 10.', 'A barrel vault rises to 13 feet, 9 at the walls.', '> x room', 'It is 10 feet by 10.', '> turn around',
          'There is a door ahead to your left (closed).', '> examine the door ahead to my left', 'A door of iron-bound wood, 3 feet wide and 7 high, 3 inches thick.',
          '% dq0 -p t -s 5', '> x room You can see part of a hall, by the light of your torch. Its far side is lost in the dark.',
          '% dq0 -p t -s 11 -c /ram/c.hm', 'A tall door of wood, some 20 feet off. You\'d have to go closer to make out more.',
          '% dq0 -p t -s 11 -c /ram/w.hm', '> examine the door ahead to my left A tall door, some 20 feet off.', '% echo $status'], f);
        const g = o.slice(o.indexOf('> look You are'), o.indexOf('> look closely'));
        if (/feet up|feet by/.test(g)) f.push('the glance gives figures: ' + g);
        if (/behind you/.test(g)) f.push('the glance tells what\'s behind: ' + g);
        const r = o.slice(o.indexOf('> x room You can see part'), o.indexOf('lost in the dark'));
        if (/feet by/.test(r)) f.push('the dark room\'s figures given: ' + r);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wordsfix', what: 'Dingus Quest Zero\'s words, a test about wording alone: the fixtures\' (the doors, their bars, the windows) as the user asked (2026-10-15 and 16): at seed 4, three doors none of them ahead, open door listing them, numbered, from the character\'s left round (each where it lies, its state), 3 turning to and opening the one behind it to its right, the room\'s doors told again; its door\'s bar lifted out of its brackets and taken, carried; at seed 12 a window examined (its make, its pane judged), its pane looked at closely, not climbed through glazed, broken (shards at its feet), climbed through into a passage, the window seen from there; at seed 15 a barred door examined (its bar in its brackets), the bar looked at closely, the door battered with a pick till it gives way, broken down for good: it won\'t shut (dq0doors\', dq0choose\'s and dq0window\'s words)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 4 -c /ram/c.hm', 'open door', '3', 'look', 'take the bar',
        'i', 'quit', 'dq0 -p t -s 12 -c /ram/c.hm', 'n', 'examine the window', 'x glass', 'climb through window', 'break window', 'look', 'climb through window',
        'turn around', 'quit', 'dq0 -p t -s 15 -c /ram/c.hm -k pick', 'examine the door ahead to my left', 'examine bar', 'break the door ahead to my left', 'look',
        'close', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['% dq0 -p t -s 4', '> open door Which door: 1. the door ahead to your right (open), 2. the one to your right (closed), or 3. the one behind you to your right (closed)?',
          '> 3', 'You turn to face north.', 'You open the door.', '> look You are on the right side of a long, narrow bedchamber',
          'There is a door ahead (open), one to your left (open) and one ahead to your left (closed).', '> take the bar',
          'You lift the bar out of its brackets, and set it down.', 'Taken.', '> i', 'a tinderbox, a bar)', '% dq0 -p t -s 12', '> examine the window',
          'A window, 2 feet wide and 3 high; its shutters of wood, 2 inches thick. It\'s glazed', 'There is a glazed window ahead (its shutters open).', '> x glass',
          'It\'s 2 by 3 feet, a quarter inch thick.', '> climb through window', 'You can\'t climb through: it\'s shut, glazed or barred.', '> break window',
          'The glass shatters with a crash.', '> look', 'There is a shuttered window ahead (its shutters open). At your feet lie some glass shards.',
          '> climb through window', 'You climb up onto the sill, and through. You are in a passage.', '> turn around', 'There is a shuttered window ahead (its shutters open).',
          '% dq0 -p t -s 15', '> examine the door ahead to my left', 'Brackets for a bar are fixed on this side, the bar in them.', '> examine bar',
          'It\'s 4 feet long, 3 inches thick.', '> break the door ahead to my left', 'You batter at it till it gives way, and it crashes open.', '> look',
          'There is a door ahead (broken down).', '> close', 'It\'s broken: it won\'t shut.', '% echo $status'], f);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wordsthings', what: 'Dingus Quest Zero\'s words, a test about wording alone: the things\', the names\', the pace\'s and the help\'s, in one game (seed 3, dq0 -T, a kit of things more: dq0 -k): a portcullis and a door behind, told apart by how far off each is; what\'s carried listed (worn, held in both hands, in and on what, the load); a table put down and a candle put on it, taken from it; a cloak worn and taken off, a basket held, the sand it won\'t keep, the waterskin that won\'t take a dagger, a quiver hung and arrows put in it, a candle that won\'t hang, hands full, the pack not into the basket in it; a sword named Linda (shown so in what\'s carried, no mark of its number), names that clash refused with why; a thing that isn\'t there, a coat not worn, a word the game doesn\'t know; the status (pace, watching, condition, light); a jog, held to a slow walk while it searches; a run set as it goes, its look as it ends a brief one (no ceiling), going on at it unsaid; the help (dq0equip\'s, dq0items\', dq0names\', dq0talk\'s, dq0choose\'s, dq0pace\'s and dq0steps\' words)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -k table,basket,sand,cloak,candle,quiver,arrows,sword',
        'face north and east', 'walk to the door behind me', '1', 'i', 'drop table', 'put the candle on the table', 'look', 'take the candle from the table',
        'wear cloak', 'hold basket', 'put sand in basket', 'put the dagger in the waterskin', 'hang the quiver on my belt', 'put arrows in quiver', 'hang the candle',
        'wield the dagger', 'stow basket', 'put the backpack in the basket', 'take the cloak off', 'stow cloak', 'name my sword Linda', 'alias the dagger as linda',
        'name the dagger Torch', 'name the dagger kobold', 'name the dagger north', 'i', 'examine the moon', 'take off my coat', 'foo', 'status', 'jog',
        'search for traps as I go', 'stop searching', 'run north', 'run south', 'help', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['> walk to the door behind me Which door: 1. the portcullis behind you, 10 feet off (raised), or 2. the door behind you, 5 feet off (open)?',
          '> 1', 'You go over to it.', '> i', 'You carry a torch (lit), in your hand; a backpack, worn (in it: 3 rations, a tinderbox, a basket, a heap of sand,',
          'a belt, worn (on it: a pouch (in it: 12 pennies), a dagger, a waterskin); a table, in both hands. In all, 56 lb.', '> drop table', 'Put down.',
          '> put the candle on the table', 'You put the candle on the table.', '> look', 'At your feet lies a table. On the table lies a candle.',
          '> take the candle from the table', 'You take the candle from the table. It\'s in your backpack.', '> wear cloak', 'You put on the cloak.',
          '> hold basket', 'You take the basket in your hand.', '> put sand in basket', 'The basket won\'t keep the heap of sand: it would run out.',
          '> put the dagger in the waterskin', 'The dagger won\'t go in the waterskin.', '> hang the quiver on my belt', 'You hang the quiver from your belt.',
          '> put arrows in quiver', 'You put the arrows in the quiver.', '> hang the candle', 'The candle won\'t hang from a belt.', '> wield the dagger',
          'Your hands are full: stow something, or put it down.', '> stow basket', 'You stow the basket in your backpack.', '> put the backpack in the basket',
          'The backpack won\'t go inside itself.', '> take the cloak off', 'You take off the cloak. It\'s in your hand.', '> stow cloak',
          'You stow the cloak in your backpack.', '> name my sword Linda', 'You name your sword Linda.', '> alias the dagger as linda', 'That name is taken by your sword.',
          '> name the dagger Torch', 'That name is taken by a kind of thing (torch).', '> name the dagger kobold', 'That name is taken by a kind of creature (kobold).',
          '> name the dagger north', 'That name is taken: it\'s a word your commands need.', '> i', 'a candle, Linda, your sword);',
          'a waterskin, a quiver (in it: arrows)). In all, 16 lb.', '> examine the moon', 'You see no moon here.', '> take off my coat', 'You aren\'t wearing any coat.',
          '> foo', 'I don\'t know how to foo.', '> status', 'Pace: a walk (40 feet a round). Watching: nothing in particular. Condition: unhurt; a little burdened (encumbrance 1); penalties: universal 0, physical 1. Light: your torch (20 feet).',
          '> jog', 'Pace: a jog (80 feet a round). Watching: nothing in particular.', '> search for traps as I go',
          'Pace: a jog, held to a slow walk while you search as you go (20 feet a round). Watching: for traps and hidden doors as you go.', '> stop searching',
          'Pace: a jog (80 feet a round).', '> run north', 'Pace: a run (160 feet a round).', 'You cross the room. You are in the middle of a long, narrow cell, by the light of your torch.',
          '> run south', '> help', 'Commands: north, northeast, east', 'Pace: creep (sneak), walk slowly, walk, jog, run, sprint', '19200 or 115200 baud', '% echo $status'], f);
        if (/#\d/.test(o)) f.push('a thing\'s mark ("#12") said');
        const rn = o.slice(o.indexOf('> run north'), o.indexOf('> run south')), rs = o.slice(o.indexOf('> run south'), o.indexOf('> help'));
        if (/ceiling|vault/.test(rn.slice(rn.indexOf('You cross')))) f.push('the look as the run ended not brief: ' + rn);
        if (/Pace:/.test(rs)) f.push('the pace said again going on as it was: ' + rs);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0choose', meaning: true, what: 'Dingus Quest Zero asking which, of several (the user\'s, 2026-10-16: "If I say open door and there are three closed doors in the room, it should give me a numbered list of choices if one is not directly in front of me"), by what each answer did: at seed 4, three doors on the bedchamber\'s north and west walls, none ahead: open door asks (nothing done), and 3 opens the one behind it to its right (turned to: facing north); then a fresh game, examine door asked again, answered with a way (right: the one to its right, examined); go through the door asked, a command not a choice (look) letting it go (1 then a word it doesn\'t know: nothing done), asked again and 1 going through the one ahead to its right; at seed 3 (dq0 -T) a portcullis and a door behind it, the first, the portcullis, walked up to (the lists\' words: dq0wordsfix, dq0wordsthings)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 4 -c /ram/c.hm', 'open door', '3', 'quit',
        'dq0 -p t -s 4 -c /ram/c.hm', 'examine door', 'right', 'go through the door', 'look', '1', 'go through the door', '1', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'face north and east', 'walk to the door behind me', '1', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], asked = t => t.k >= 0 && !t.events.length && !t.replies.some(r => r.req === 'wreq' && r.kind === 'WQ_EXAMINE');
        const o = T.of('open door'), c3 = T.of('3');
        if (!asked(o)) f.push('open door not asked which (' + dq0Evs(o) + ')');
        dq0Need(f, c3, 'E_PORTAL', { po: 'PO_OPENED' }, '3 didn\'t open the door');
        if (!c3.after || c3.after.pc.face !== 0) f.push('not turned to the door behind to its right (north)');
        const x = T.of('examine door'), r = T.of('right');
        if (!asked(x)) f.push('examine door not asked which');
        if (!dq0Rep(r, 'wreq', { kind: 'WQ_EXAMINE', res: 0 }).length) f.push('right not answered with the door to its right examined');
        const g1 = T.of('go through the door'), lk = T.after(g1, 'look'), n1 = T.after(lk, '1'), g2 = T.after(n1, 'go through the door'), n2 = T.after(g2, '1');
        if (!asked(g1) || !asked(g2)) f.push('go through the door not asked which');
        if (n1.k < 0 || n1.events.length) f.push('1 after a look (the question let go) did something (' + dq0Evs(n1) + ')');
        dq0Need(f, n2, 'E_ARRIVE', {}, '1 not gone through the door');
        const w = T.of('walk to the door behind me'), p = T.after(w, '1');
        if (!asked(w)) f.push('walk to the door behind me not asked which');
        dq0Need(f, p, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'the portcullis not walked up to');
        if (!p.at || !p.after || p.at.pc.x - p.after.pc.x !== 2) f.push('not gone the portcullis\'s 10 feet');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0vera',what: 'Dingus Quest Zero on the Vera X (the user\'s, 2026-10-15: "If the Vera is available, this should be running there, and keyboard would come from the SMC"; 2026-10-16: "Full screen is fine"): started with the console\'s terminals both, the game takes the screen alone (consctl\'s terminal screen: its window 57 rows) and puts them back as it ends; typed at the SMC\'s keyboard: the status line reversed on the window\'s first row, the line typed on its last, the map, Backspace, Escape pausing and going on, the cursor key up recalling the last line, the help\'s word on the serial port\'s rates',
      init: 't_rc', cycles: 1400e6,
      // (Snapshots of the screen as the keyboard's typing comes to SNAP: smc.type's, wrapped; P a wait, W a prompt: the
      // serial port's, so W only while it shows the window)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push({ text: m.vera.text(), cells: m.vera.cells() }) : type(ch);
      },
      get machine() {
        const W = 'ā', P = 'Ā', SNAP = 'Ѐ', UP = String.fromCharCode(0x200 + 83), C = P.repeat(30);
        return { vera: true, smc: true, input: W + 'bind -a /sd/0/games /bin\r' + W + 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\r' + 'Ă' +
          W + 'dq0 -p t -s 3 -c /ram/c.hm\r' + P.repeat(150) + 'look\r' + C + 'mapx\b\r' + C + SNAP + '\x1b' + P + SNAP + '\x1b' + P +
          'lo' + UP + '\r' + C + SNAP + 'help\r' + C + SNAP + 'quit\r' + P.repeat(20) + 'ă' + 'cat /dev/consctl\r' + W + 'echo do^ne\r' };
      },
      expect: ['\ndone\n', 'terminal both'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 4 || s.some(p => !p.cells)) return ['the screen snapshots: ' + s.length + ' (4 wanted, each with a text layer)'];
        const row = (p, r) => p.text[r] || '', rev = (p, r) => Array.from(p.cells.attrs.subarray(r * 80, r * 80 + 80)).every(a => a === 0x70);
        const [map, paused, again, help] = s;
        // (The screen alone, as the game made it: the window rows 2-58 below the bar and the header, its footer row 59;
        // the status line its first, the line typed its last)
        if (!/^ Tordan +UP 0 +facing /.test(row(map, 2))) f.push('the status line (row 2): ' + JSON.stringify(row(map, 2)));
        if (!rev(map, 2)) f.push('the status line isn' + Q + 't reversed all the way across');
        if (row(map, 58) !== '>') f.push('the line typed (row 58): ' + JSON.stringify(row(map, 58)));
        if (!map.text.some(l => l.startsWith('The map (you face'))) f.push('no map on the screen');
        if (!map.text.some(l => l.startsWith('> map')) || map.text.some(l => l.startsWith('> mapx'))) f.push('Backspace: the line entered isn' + Q + 't "> map"');
        if (!/PAUSED/.test(row(paused, 2))) f.push('Escape: the status line doesn' + Q + 't say PAUSED: ' + JSON.stringify(row(paused, 2)));
        if (/PAUSED/.test(row(again, 2))) f.push('Escape again: still PAUSED');
        if (again.text.filter(l => l.startsWith('The map (you face')).length < 2) f.push('the cursor key up: the last line ("map") not entered again');
        const h = help.text.join(' ').replace(/ +/g, ' ');
        if (!h.includes('19200 or 115200 baud')) f.push('the help doesn' + Q + 't give the serial port' + Q + 's rates');
        if (!help.text.slice(27, 58).some(l => l.trim())) f.push('the screen alone: nothing below row 26 (the window not 57 rows)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0veraplain', what: 'Dingus Quest Zero on the Vera X as the browser page has it with its keyboard on (the user\'s, 2026-10-16: plain dq0 typed at the Vera X\'s keyboard "switched (on its own) back to the serial terminal"): the terminals chosen once as dq0 starts (terminal both, and consctl\'s keyboard screen: the screen alone) and kept through hmchar\'s questions and the game, put back (terminal both) only as it ends; typed at the SMC\'s keyboard, the normal pace',
      meaning: true,
      init: 't_rc', cycles: 2000e6,
      // (SNAP: the screen and the serial port's output so far, as the keyboard's typing comes to it: smc.type's, wrapped)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push({ text: m.vera.text(), out: m.out }) : type(ch);
      },
      get machine() {
        const P = 'Ā', SNAP = 'Ѐ', hm = ['k', '', 'glad', 'sw', 'shield', 'dag', 'climb', '', 'Dorn'];
        return { vera: true, smc: true, input: 'ābind -a /sd/0/games /bin\rāĂdq0 -s 3\r' + P.repeat(60) + 'n\r' + P.repeat(60) +
          SNAP + hm.map(l => l + '\r' + P.repeat(30)).join('') + P.repeat(150) + SNAP + 'look\r' + P.repeat(40) + SNAP + 'quit\r' + P.repeat(40) +
          'ăcat /dev/consctl\rāecho do^ne\r' };
      },
      expect: ['\ndone\n', '\nterminal both\n'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 3) return ['the snapshots: ' + s.length + ' (3 wanted)'];
        const [hm, game, last] = s, at = out.indexOf('dq0 -s 3');
        if (at < 0) return ['no "dq0 -s 3" on the serial port'];
        // (hmchar's questions on the screen alone: none of them on the serial port, as it asks or after)
        if (!hm.text.some(l => l.startsWith('Keep this character'))) f.push('hmchar\'s first question isn\'t on the screen');
        const ser = last.out.slice(at);
        if (/Keep this character|An occupation|Three weapon skills/.test(ser)) f.push('hmchar\'s questions went to the serial port (the terminals both again)');
        if (/UP 0 +facing|> look/.test(ser)) f.push('the game went to the serial port (the terminals both again)');
        // (The game on the screen alone: the window 57 rows, its status line the first, the line typed its last)
        if (!/^ \S+ +UP 0 +facing /.test(game.text[2] || '')) f.push('the status line (row 2): ' + JSON.stringify(game.text[2]));
        if ((game.text[58] || '') !== '>') f.push('the line typed (row 58), the screen alone: ' + JSON.stringify(game.text[58]));
        // (Put back as it ends: the serial port's terminal painted again, consctl's terminal both)
        if (!out.slice(at).includes('\nterminal both\n')) f.push('the terminals not both after the game');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0veraserial', what: 'Dingus Quest Zero on the Vera X with no keyboard there (the browser page\'s defaults: the screen, no SMC), typed on the serial port: plain dq0 (hmchar inside it, the character it makes the one the game plays: dq0\'s, before) stays on both terminals (no consctl keyboard screen: the serial terminal, the only keyboard, isn\'t left blind), hmchar\'s questions and the game on the serial port and the screen, the terminals both as it ends',
      init: 't_rc', cycles: 2000e6,
      // (When the game's first question and hmchar's first came out on the serial port: the first as it was asked, not
      // painted at once with what the window had as the serial port's terminal was turned on again for hmchar)
      start(m) {
        const run = m.run, at = this.at = {};
        m.run = limit => {
          run(limit);
          if (!at.pace && m.out.includes('How fast does the dungeon move')) at.pace = m.cpu.cyc;
          if (!at.hm && m.out.includes('Keep this character')) at.hm = m.cpu.cyc;
        };
      },
      get machine() {
        const P = 'Ā', hm = ['k', '', 'glad', 'sw', 'shield', 'dag', 'climb', '', 'Dorn'];
        return { vera: true, input: 'ābind -a /sd/0/games /bin\rādq0 -s 3\r' + P.repeat(60) + 'n\r' + P.repeat(60) +
          hm.map(l => l + '\r' + P.repeat(30)).join('') + P.repeat(150) + 'look\r' + P.repeat(40) + 'quit\r' + P.repeat(40) +
          'cat /dev/consctl\rāecho do^ne\r' };
      },
      expect: ['\ndone\n', '\nterminal both\n'],
      check(m, out) {
        const f = [], at = out.indexOf('dq0 -s 3'), text = m.vera.text();
        if (at < 0) return ['no "dq0 -s 3" on the serial port'];
        const ser = out.slice(at);
        const a = this.at || {};
        if (!a.pace || !a.hm || a.hm - a.pace < 50e6) f.push('the game\'s first question not on the serial port as it was asked (the screen taken alone): at ' + a.pace + ', hmchar\'s at ' + a.hm);
        if (!ser.includes('Keep this character')) f.push('hmchar\'s questions not on the serial port');
        if (!/UP 0 +facing/.test(ser)) f.push('the game\'s status line not on the serial port');
        if (!/> look/.test(ser)) f.push('the line typed not on the serial port');
        if (!/Dorn, [A-Za-z/]+, enters the dungeon\./.test(ser.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '))) f.push('the character hmchar made in the game (Dorn) not the one it plays (dq0\'s, before)');
        if (/keyboard screen/.test(ser)) f.push('consctl said keyboard screen with no keyboard');
        if (!text.some(l => l.startsWith('> look'))) f.push('the game not on the screen too');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0batter', what: 'Dingus Quest Zero\'s doors battered (the user\'s, 2026-10-15 and 16: "model it after unarmed combat": a blow at a thing an attack it ignores, by Unarmed, its impact less the door\'s resistance off its body; the striker hurt as a blow goes wrong, less with skill and armour; boots for a kick, a helm for a head butt; the tool to hand that hits hardest), by the world\'s events (the referee\'s, HarnMaster\'s, at these seeds; each game\'s first blow the same roll): at seed 14\'s sound iron-bound door, bare fists making no mark, the hand hurt seriously, the blows stopped as the character\'s too hurt; a kick in knee boots and a head butt in an iron helm hurt only a little; at seed 19\'s rotten door one of Unarmed 95 (hmchar\'s file) breaking it down unhurt; at seed 15 a pick battering a door till it gives way, broken down for good: it won\'t shut (dq0hands\' and dq0kick\'s, and dq0doors\' blows, these together; the injuries\' words the referee\'s)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/u.hm', 'echo ml una 95 >> /ram/u.hm',
        'dq0 -p t -s 14 -c /ram/c.hm', 'punch the door', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -k \'knee boots\'', 'wear the knee boots', 'kick the door', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm -k \'iron helm\'', 'wear the iron helm', 'head butt the door', 'quit', 'dq0 -p t -s 19 -c /ram/u.hm', 'punch the door', 'quit',
        'dq0 -p t -s 15 -c /ram/c.hm -k pick', 'break the door ahead to my left', 'close', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], hurt = t => Math.max(0, ...dq0Ev(t, 'E_HARM', { hc: 'HC_STRUCK' }).map(e => e.band));
        const p = T.of('punch the door'), k = T.of('kick the door'), h = T.of('head butt the door'), u = T.after(p, 'punch the door');
        dq0Need(f, p, 'E_PORTAL', { po: 'PO_NO_MARK' }, 'bare fists marked the sound door');
        if (hurt(p) < 2) f.push('the bare hand not hurt seriously (' + dq0Evs(p) + ')');
        dq0Need(f, p, 'E_HARM', { hc: 'HC_STRUCK', stopped: true }, 'the blows not stopped, the character too hurt');
        if (!(hurt(k) >= 1 && hurt(k) < hurt(p))) f.push('the booted foot not spared (' + dq0Evs(k) + ')');
        if (!(hurt(h) >= 1 && hurt(h) < hurt(p))) f.push('the helmed head not spared (' + dq0Evs(h) + ')');
        dq0Need(f, u, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'the skilled fists didn\'t break the rotten door down');
        if (hurt(u)) f.push('the skilled fists hurt');
        const b = T.of('break the door ahead to my left');
        dq0Need(f, b, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'the pick didn\'t break the door down');
        dq0Need(f, T.after(b, 'close'), 'E_PORTAL', { po: 'PO_WRECKED' }, 'the broken door shut');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0battering', what: 'Dingus Quest Zero\'s battering a long task (slice 4.0, HOUSE.md\'s long tasks: a blow at a time as the clock goes, its bar the blows struck, stopped by the player or hard, the harm done kept), by the world\'s events and the long task\'s part done as it went (the hub\'s pc_task: the referee\'s, HarnMaster\'s, at these seeds): at seed 14 a pick battering the sound door a dozen blows, one at a time (LT_BATTER, its length a dozen), leaving it giving, and battered again, broken down; then the same door battered and "stop digging" refused as it goes (it\'s a battering), "stop battering" stopping it part way, how the door stands told, and battered again, broken down in as many blows in all as without the stop (the harm done kept, each blow its own roll as before); with the hand-written theme\'s hostile rats (inhabit.txt) a battering stopped hard by a rat\'s blow',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 14 -c /ram/c.hm -k pick', 'break the door', 'break', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -k pick', 'break the door']) +
        'ĀĀstop digging\rstop battering\r' + dq0Typed(['break', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k pick', 'break the door ahead', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], tasks = m.dq0.games.flatMap(g => g.tasks);
        // (A run's part done as it went, from turn t's line to turn u's: the blows struck, one at a time from none)
        const run = (t, u, what) => {
          const d = tasks.filter(e => e.kind === 'LT_BATTER' && e.line > t.k && e.line <= u.k).map(e => e.done);
          if (!d.length || !d.every((v, i) => v === i)) f.push(what + ': its part done not the blows struck one at a time (' + d.join(' ') + ')');
          return d.length - 1;
        };
        const g1 = T.of('dq0 -p t -s 14 -c /ram/c.hm -k pick'), b1 = T.after(g1, 'break the door'), r1 = T.after(b1, 'break'), q1 = T.after(r1, 'quit');
        const n1 = run(b1, r1, 'the whole run'), m1 = run(r1, q1, 'the run after it');
        if (n1 !== 12) f.push('the whole run not a dozen blows: ' + n1);
        if (!tasks.some(e => e.kind === 'LT_BATTER' && e.len === 12)) f.push('the long task\'s length not a dozen blows');
        dq0Need(f, b1, 'E_PORTAL', { po: 'PO_GIVING' }, 'the pick\'s dozen blows didn\'t leave the door giving');
        dq0Need(f, r1, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'battered again, the door not broken down');
        const g2 = T.after(q1, 'dq0 -p t -s 14 -c /ram/c.hm -k pick'), b2 = T.after(g2, 'break the door'), sd = T.after(b2, 'stop digging');
        const sb = T.after(b2, 'stop battering'), r2 = T.after(sb, 'break'), q2 = T.after(r2, 'quit');
        dq0Need(f, T.and(sd), 'E_TASK', { te: 'TE_NOT', task: 'LT_BATTER', named: 'LT_DIG' }, '"stop digging" not refused as it battered');
        dq0Need(f, sb, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_BATTER' }, '"stop battering" didn\'t stop it');
        const n2 = run(b2, r2, 'the run stopped'), m2 = run(r2, q2, 'the run after the stop');
        if (!(n2 >= 1 && n2 < 12)) f.push('not stopped part way: ' + n2 + ' blows');
        dq0Need(f, sb, 'E_PORTAL', { po: p => p === 'PO_HOLDING' || p === 'PO_GIVING' }, 'how the door stood not told as it stopped');
        dq0Need(f, r2, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'battered again after the stop, the door not broken down');
        if (n2 + m2 !== n1 + m1) f.push('the harm done before the stop not kept: ' + n2 + ' blows, then ' + m2 + ', not ' + (n1 + m1) + ' in all as without it');
        const b3 = T.after(T.of('dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k pick'), 'break the door ahead');
        dq0Need(f, b3, 'E_TASK', { te: 'TE_HIT', task: 'LT_BATTER' }, 'the battering not stopped hard by the rat\'s blow');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fight', what: 'Dingus Quest Zero\'s fights (DENIZENS.md: fights; the referee\'s melee, RS_ATTACK, dq0harn/fight.c), by the blows told (the denizens\' events, E_FIGHT: who struck whom, how it came out, what it left them in; the referee\'s dice the commands\', in their order, at the turns pace: HarnMaster\'s at this seed): a fight with seed 5\'s giant rats won (down its ladder, a sword in hand, dq0 -k sword; the character\'s Sword 80, hmchar\'s file): the rat struck at, and its lair\'s kin, turn on the character, both striking at it; three blows dodged, the fourth knocking the rat senseless; then the other struck at by the way it lies (the rat ahead to the right) and knocked senseless too; each finished where it lies, their bodies left (seen as the world looks about again); the character hurt by their blows',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml swd 80 >> /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -k sword -T',
        'd', 'wait', 'go right', ...Array(4).fill('attack the rat'), 'attack the rat ahead right', 'attack the rat ahead right', 'attack the rat', 'look', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], FF_D_DOWN = 4, FF_D_DEAD = 8;
        const mine = t => dq0Ev(t, 'E_FIGHT', { who: 0 }), theirs = t => dq0Ev(t, 'E_FIGHT', { def: 0, who: w => w > 0 });
        const a = T.filter(t => t.line === 'attack the rat'), r = T.filter(t => t.line === 'attack the rat ahead right');
        if (a.length !== 5 || r.length !== 2) return ['the attacks not typed'];
        const first = mine(a[0])[0];
        if (!first) return ['no blow struck at the rat (' + dq0Evs(a[0]) + ')'];
        if (new Set(a.slice(0, 3).flatMap(theirs).map(e => e.who)).size < 2) f.push('the rat and its kin not both striking at the character');
        // (Which rat "the rat" is, as each blow's struck, the one in front then: since slice 4.0's walks a square at a
        // time, the first blow may fall on its kin; the rat the fourth knocks senseless the one finished last)
        for (const t of a.slice(0, 3)) if (!mine(t).some(e => e.out === 'O_DODGED')) f.push(t.k + ': the blow not dodged (' + dq0Evs(t) + ')');
        const down = mine(a[3]).find(e => e.out === 'O_HIT' && (e.ff & FF_D_DOWN));
        if (!down) f.push('the fourth blow didn\'t knock a rat senseless (' + dq0Evs(a[3]) + ')');
        const other = mine(r[0])[0];
        if (!other || (down && other.def === down.def) || other.out !== 'O_HIT' || !(other.ff & FF_D_DOWN)) f.push('the rat ahead to the right not struck senseless (' + dq0Evs(r[0]) + ')');
        if (!mine(r[1]).some(e => other && e.def === other.def && e.out === 'O_DISPATCH' && (e.ff & FF_D_DEAD))) f.push('the rat ahead to the right not finished');
        if (!mine(a[4]).some(e => down && e.def === down.def && e.out === 'O_DISPATCH' && (e.ff & FF_D_DEAD))) f.push('the first rat down not finished');
        const lk = T.of('look');
        if (!lk.after || !lk.after.sight || lk.after.sight.items.filter(i => i.name === 'body').length < 2) f.push('their bodies not seen');
        if (!lk.after || !(lk.after.sum.health > 0)) f.push('the character not hurt by their blows');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0armour', what: 'Dingus Quest Zero\'s denizens fighting and armour (DENIZENS.md: fights; the catalogue\'s armour kinds, worn: the things tell the referee what\'s worn and how well made, RS_EQUIP\'s marks; HarnMaster\'s protection by location, the layers summed, Combat 4-5), by the blows told and the character\'s state (the referee\'s dice at this seed): a gambeson under a mail hauberk (dq0 -k) worn; down the stairway at seed 7 a ghoul (its faction hostile to the character) seen as the character comes down, coming up to it and clawing at it a round at a time as it waits (its own turns: the referee\'s round); the character dodging (defend dodge: its stance), a claw dodged, one turned by the armour; then standing undefended (defend none), wounded, worse, knocked down, then clawed till it\'s killed: the character\'s death, the game\'s end (dq0ghoul\'s, in it)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 7 -c /ram/c.hm -k \'gambeson,mail hauberk\' -T',
        'wear the gambeson', 'wear the mail hauberk', 'd', 'defend dodge', 'wait', 'wait', 'defend none', ...Array(18).fill('wait'), 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], FF_D_DOWN = 4, FF_D_DEAD = 8, FF_STOPPED = 0x40;
        const worn = (t, n) => { const i = dq0Thing(t.after, n); if (!i || !(i.flags & DQ0_I.WORN)) f.push(n + ' not worn'); };
        worn(T.of('wear the gambeson'), 'gambeson');
        worn(T.of('wear the mail hauberk'), 'mail hauberk');
        const d = T.of('d'), ghoul = d.after && d.after.actors.find(a => a.name === 'ghoul');
        if (!d.after || !d.after.sight || !d.after.sight.dens.some(x => x.name === 'ghoul')) f.push('the ghoul not seen as the character came down');
        const claws = t => dq0Ev(t, 'E_FIGHT', { def: 0, who: w => ghoul && w === ghoul.k + 1 });
        if (!ghoul) return [...f, 'no ghoul'];
        const dd = T.of('defend dodge'), dn = T.of('defend none'), w = T.filter(t => t.line === 'wait');
        if (!dd.after || dd.after.pc.stance !== 'D_DODGE') f.push('the stance not dodging: ' + (dd.after && dd.after.pc.stance));
        if (!dn.after || dn.after.pc.stance !== 'D_NONE') f.push('the stance not none: ' + (dn.after && dn.after.pc.stance));
        if (w.length !== 20) return [...f, 'the waits not typed'];
        if (!claws(w[0]).some(e => e.out === 'O_DODGED')) f.push('the first claw not dodged (' + dq0Evs(w[0]) + ')');
        if (!claws(w[1]).some(e => e.ff & FF_STOPPED)) f.push('the second claw not turned by the armour (' + dq0Evs(w[1]) + ')');
        const h = t => t.after ? t.after.sum.health : -1;
        if (!(claws(w[2]).some(e => e.out === 'O_HIT') && h(w[2]) > h(w[1]))) f.push('undefended, not wounded');
        if (!(claws(w[3]).some(e => e.out === 'O_HIT') && h(w[3]) > h(w[2]))) f.push('undefended, not wounded worse');
        if (!claws(w[4]).some(e => e.ff & FF_D_DOWN)) f.push('the third undefended claw not knocking the character down (' + dq0Evs(w[4]) + ')');
        // (Then down, senseless, clawed a round at a time while it's out, till a blow kills it: the referee's, some
        // fifteen rounds at this seed; since slice 4.0 the character busy till it comes round, so the death comes in the
        // knock's own turn, the waits after it the shell's)
        const k = w.slice(4).find(t => dq0Ev(t, 'E_FIGHT', { ff: x => x & FF_D_DEAD }).length);
        if (!k) f.push('the character, down, not killed by the ghoul\'s claws');
        else {
          if (!k.after || !(k.after.sum.cond & 4)) f.push('the character not dead (C_DEAD)');
          dq0Need(f, k, 'E_BYE', {}, 'the game not ended by the death');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0flee', what: 'Dingus Quest Zero\'s unarmed blows and morale (the user\'s, 2026-10-16: punching, kicking, head butts, the referee\'s gear.c, by Unarmed; RS_MORALE, HarnMaster\'s Initiative test, Combat 18), by the blows told and the denizens\' state (the referee\'s dice at this seed): at seed 5\'s giant rats (bare fists: the character\'s Unarmed 80) a kick, a head butt and a punch (the hub\'s pc_strike each), the rat and its kin turning on the character, a punch felling it; its lair\'s kin seeing it fall (the book\'s friend slain: a bad situation) shaken and, shy, fleeing (fighting no more), the one down finished; the other gone out of reach, a blow at it not struck (dq0unarmed\'s, in it)',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml una 80 >> /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -T', 'd',
        'wait', 'go right', 'kick the rat', 'head butt the rat', 'punch the rat', 'punch the rat', 'punch the rat', 'punch the rat', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], FF_D_DOWN = 4, FF_D_DEAD = 8, mine = t => dq0Ev(t, 'E_FIGHT', { who: 0 });
        const k = T.of('kick the rat'), b = T.of('head butt the rat'), p = T.filter(t => t.line === 'punch the rat');
        if (p.length !== 4) return ['the punches not typed'];
        for (const [t, sk] of [[k, 'SK_KICK'], [b, 'SK_HEAD'], [p[0], 'SK_PUNCH']]) {
          if (!mine(t).length) f.push(t.line + ': no blow struck (' + dq0Evs(t) + ')');
          if (!t.after || t.after.pc.strike !== sk) f.push(t.line + ': struck with ' + (t.after && t.after.pc.strike) + ', not ' + sk);
        }
        // (The rat the second punch fells: "the rat" the one in front as each blow's struck, since slice 4.0's walks a
        // square at a time the kick's maybe its kin)
        const felled = mine(p[1]).find(e => e.ff & FF_D_DOWN), rat = felled ? felled.def : (mine(k)[0] || {}).def;
        if (!k.after || k.after.actors.filter(a => a.doing === 'DO_FIGHT').length < 2) f.push('the rat and its kin not turned on the character');
        if (!felled) f.push('the second punch didn\'t fell a rat (' + dq0Evs(p[1]) + ')');
        const kin = p[1].after ? p[1].after.actors.filter(a => a.k + 1 !== rat) : [];
        if (!kin.length || kin.some(a => a.doing !== 'DO_FLEE')) f.push('its kin not fleeing: ' + kin.map(a => a.doing).join(', '));
        if (!mine(p[2]).some(e => e.def === rat && e.out === 'O_DISPATCH' && (e.ff & FF_D_DEAD))) f.push('the one down not finished');
        if (mine(p[3]).length) f.push('a blow struck at the rat fled out of reach');
        if (p[3].after && p[3].after.sight && p[3].after.sight.dens.some(d => d.doing === 'DO_FIGHT')) f.push('a rat seen fighting still');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0traps', what: 'Dingus Quest Zero\'s traps (DUNGEON.md: traps; the world\'s walk stops on one and sets it off, the referee judges it), by the world\'s events (the referee\'s dice the commands\', in their order, at the turns pace: HarnMaster\'s at these seeds): a door\'s blade, set off as the door\'s forced open (seed 16, dq0 -T); the next door holding at the first try, bursting at the second; then a plate in a storeroom\'s floor walked onto (the walk stopped on it), its blade striking (HC_STRIKE: a wound), the trap seen after, sprung; then (seed 7, a thief, hmchar -s 1 -u feudal -c thief, Lockcraft 95) a hidden dart trap on a door found by the third search, the door left shut while it\'s known, the trap made out (its mechanism assessed), then disarmed (Lockcraft: two checks, the user\'s), and the door opened',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 16 -c /ram/c.hm -T', 'face east', 'force', 'e',
        'force', 'force', 'e', 'e', 'quit', 'hmchar -q -s 1 -u feudal -c thief -o /ram/t.hm', 'echo ml lck 95 >> /ram/t.hm', 'dq0 -p t -s 7 -c /ram/t.hm', 'search', 'search',
        'search', 'face south', 'open', 'disarm', 'disarm', 'open', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], fo = T.of('force');
        dq0Need(f, fo, 'E_PORTAL', { po: 'PO_FORCED' }, 'the door not forced open');
        dq0Need(f, fo, 'E_TRAP', { tk: 'TK_BLADE', tt: 'TT_DOOR' }, 'the door\'s blade not set off as it opened');
        const f2 = T.after(T.after(fo, 'e'), 'force'), f3 = T.after(f2, 'force');
        dq0Need(f, f2, 'E_PORTAL', { po: 'PO_HELD' }, 'the next door not holding at the first try');
        dq0Need(f, f3, 'E_PORTAL', { po: 'PO_FORCED' }, 'the next door not bursting at the second');
        const e2 = T.after(T.after(f3, 'e'), 'e');
        dq0Need(f, e2, 'E_ARRIVE', { how: 'A_STOPPED' }, 'the walk not stopped on the plate');
        dq0Need(f, e2, 'E_TRAP', { tk: 'TK_BLADE', tt: 'TT_PLATE', to: 'TO_SPRUNG' }, 'the plate\'s blade not set off');
        dq0Need(f, e2, 'E_HARM', { hc: 'HC_STRIKE', band: b => b > 0 }, 'the blade not a wound');
        if (!e2.after || !e2.after.sight || !e2.after.sight.traps.some(p => p.kind === 'TK_BLADE' && (p.state & 1))) f.push('the trap not seen, sprung');
        const g = T.of('dq0 -p t -s 7 -c /ram/t.hm'), s1 = T.after(g, 'search'), s2 = T.after(s1, 'search'), s3 = T.after(s2, 'search');
        for (const s of [s1, s2]) dq0Need(f, s, 'E_PORTAL', { po: 'PO_NOTHING' }, 'something found by the first searches');
        dq0Need(f, s3, 'E_TRAP', { tk: 'TK_DARTS', tt: 'TT_DOOR', to: 'TO_FOUND' }, 'the dart trap not found by the third search');
        const o = T.after(s3, 'open'), d1 = T.after(o, 'disarm'), d2 = T.after(d1, 'disarm'), o2 = T.after(d2, 'open');
        dq0Need(f, o, 'E_PORTAL', { po: 'PO_TRAPPED' }, 'the door opened with its known trap on it');
        dq0Need(f, d1, 'E_TRAP', { to: 'TO_ASSESSED' }, 'the mechanism not made out first');
        dq0Need(f, d2, 'E_TRAP', { to: 'TO_DISARMED' }, 'the trap not disarmed');
        dq0Need(f, o2, 'E_PORTAL', { po: 'PO_OPENED' }, 'the door not opened, disarmed');
        dq0Never(f, o2, 'E_TRAP', {}, 'the trap went off, disarmed');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pure', what: 'Dingus Quest Zero\'s maker pure (move 2.2 of Phase 2, "one authority on state": generation only, the same seed always the same chunk; the change records the world\'s, dq0w/store.c, laid over a chunk the maker makes again): at seed 16 (dq0 -T -k pick) the chunks the maker made at once, read from the hub\'s cache; then a game played and kept (dq0 -d: a door forced open, the blade trap on it set off, the rock dug through beside the passage), and carried on (its chunks made again by a fresh maker from the seed, the records laid over them from the save\'s region files): each chunk made again the same as it was live before the game was kept, and the same as the maker first made it but for what the records say (a portal\'s state, a trap\'s, a square\'s kind and floor, an opening), and the records saying each of them',
      meaning: true,
      init: 't_rc', cycles: 2500e6,
      machine: { input: '\u0101bind -a /sd/0/games /bin\r\u0101hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\r\u0101dq0 -p t -s 16 -c /ram/c.hm -T -k pick\r' +
        ['look', 'quit', 'echo pure-a'].map(l => '\u0101' + l + '\r').join('') + '\u0101dq0 -p t -s 16 -c /ram/c.hm -T -k pick -d /ram/p\r' +
        ['face east', 'force', 'e', 'dig north', 'quit', 'echo pure-b'].map(l => '\u0101' + l + '\r').join('') + '\u0101dq0 -p t -d /ram/p\r' +
        ['look', 'quit', 'echo pure-c', 'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['% echo $status'],
      // (The hub's live chunks taken as each game has ended, its echo typed: the maker's first (a), the game's as it
      // was kept (b), the ones made again as it's carried on (c))
      start(m) {
        const run = m.run, snaps = this.snaps = {};
        m.run = lim => {
          run(lim);
          for (const k of ['a', 'b', 'c']) if (!snaps[k] && m.out.includes('pure-' + k)) snaps[k] = liveChunks(m, 0);
        };
      },
      check(m, out) {
        const f = [], { a, b, c } = this.snaps;
        if (!a || !b || !c) return ['the chunks not read as each game ended: ' + ['a', 'b', 'c'].filter(k => !this.snaps[k]).join(', ')];
        const sum = { portal: 0, trap: 0, square: 0, opening: 0 };
        let same = 0, made = 0;
        for (const [at, k] of Object.entries(c)) {
          if (b[at]) {
            const d = chunkDiff(b[at], k), n = d.portal + d.trap + d.square + d.opening + d.other.length;
            if (n) f.push(at + ': made again not as it was kept: ' + JSON.stringify(d));
            else same++;
          }
          if (a[at]) {
            const d = chunkDiff(a[at], k);
            if (d.other.length) f.push(at + ': made again, its bytes ' + d.other.slice(0, 8).join(', ') + ' not the maker\'s first');
            for (const w of Object.keys(sum)) sum[w] += d[w];
            made++;
          }
        }
        if (!same) f.push('no chunk both live as the game was kept and made again');
        if (!made) f.push('no chunk both made first and made again');
        if (!sum.portal) f.push('no portal\'s state changed (the door forced)');
        if (!sum.trap) f.push('no trap\'s state changed (the blade set off)');
        if (!sum.square) f.push('no square changed (the rock dug)');
        this.notes = [Object.keys(c).length + ' chunk(s) made again, ' + same + ' as kept, ' + made + ' against the maker\'s first: ' + JSON.stringify(sum)];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stairtrap', meaning: true, what: 'Dingus Quest Zero\'s walks to stairs and ladders checking their way for traps, as the other walks do (the user\'s, 2026-10-16), by the world\'s events: at seed 38 the character goes round to the kitchen\'s far door, clear of the covered pit hidden between it and the stairs up to the way out (not noticed in passing: the referee\'s, HarnMaster\'s at this seed), then up (u): the walk back to the stairs stops on the pit, which gives way under it (a fall, a climb out begun), the pit seen open; then the stairs again, the pit it knows of gone round, and the way out reached (shut fast), no second fall',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 38 -c /ram/c.hm', 'se', 'walk to the door east',
        'u', 'u', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], w = T.of('walk to the door east'), u1 = T.of('u'), u2 = T.after(u1, 'u');
        dq0Need(f, w, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'not gone round to the far door');
        dq0Never(f, w, 'E_TRAP', {}, 'a trap set off going round');
        dq0Need(f, u1, 'E_ARRIVE', { how: 'A_STOPPED' }, 'the walk to the stairs not stopped on the pit');
        dq0Need(f, u1, 'E_TRAP', { tk: 'TK_COVERED', to: 'TO_SPRUNG' }, 'the covered pit didn\'t give way');
        dq0Need(f, u1, 'E_HARM', { hc: 'HC_FALL' }, 'no fall into the pit');
        dq0Need(f, u1, 'E_CLIMB', { how: 'K_BEGUN' }, 'no climb out begun');
        if (!u1.after || !u1.after.sight || !u1.after.sight.traps.some(p => p.kind === 'TK_COVERED' && (p.state & 1))) f.push('the pit not seen open');
        dq0Need(f, u2, 'E_NOWAY', { why: 'N_SHUT' }, 'the way out not reached, the pit gone round');
        dq0Never(f, u2, 'E_TRAP', {}, 'the pit gave way again (not gone round)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rebar', meaning: true, what: 'Dingus Quest Zero\'s doors barred again (the user\'s, 2026-10-16), by the world\'s events and the bar\'s record (a portal\'s part, IW_PORTAL, on the square of its side, or a thing carried or lying): at seed 4 the door\'s bar lifted out of its brackets and taken, then put back (bar the door: from its side, the door shut): out of the pack, in its brackets again, the door barred (opened: the bar lifted out again, at its feet); not while the door stands open; put back from the floor (put the bar back in its brackets); kept (dq0 -d), the door still barred and the bar in it as the game carries on (their words: dq0wordsfix)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 4 -c /ram/c.hm -d /ram/g', 'take the bar',
        'bar the door', 'open', 'bar the door', 'close', 'put the bar back in its brackets', 'quit', 'dq0 -p t -d /ram/g', 'open', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        // (The bar: in its brackets, IW_PORTAL on the square of its side; carried; or lying where the character stands)
        const bar = s => { const b = s && s.items.filter(i => i.name === 'bar'); if (!b) return '?';
          if (dq0Carried(s).some(i => i.name === 'bar')) return 'carried';
          return b.some(i => i.where === 'IW_PORTAL' && i.sn === s.pc.sn && i.x === s.pc.x && i.y === s.pc.y) ? 'brackets' :
            dq0Here(s).some(i => i.name === 'bar') ? 'floor' : 'gone'; };
        const want = (l, po, where, from = 0) => {
          const t = T.of(l, from);
          dq0Need(f, t, 'E_PORTAL', { po }, l + ': not ' + po);
          if (bar(t.after) !== where) f.push(l + ': the bar ' + bar(t.after) + ', not ' + where);
          return t;
        };
        const s0 = T.of('dq0 -p t -s 4 -c /ram/c.hm -d /ram/g').after;      // (Its bar's side a square off, the door's)
        if (!s0 || !s0.items.some(i => i.name === 'bar' && i.where === 'IW_PORTAL' && Math.abs(i.x - s0.pc.x) + Math.abs(i.y - s0.pc.y) <= 1)) f.push('the door by it not barred to begin with');
        want('take the bar', 'PO_LIFTED', 'carried');
        const b = want('bar the door', 'PO_REBARRED', 'brackets');
        const o = want('open', 'PO_UNBARRED', 'floor');
        if (!o.after || !o.after.sight || !o.after.sight.doors.some(d => d.rel === 0 && (d.state & 1))) f.push('the door ahead not open');
        want('bar the door', 'PO_SHUT_FIRST', 'floor', o.k + 1);
        want('close', 'PO_CLOSED', 'floor');
        const p = want('put the bar back in its brackets', 'PO_REBARRED', 'brackets');
        const c = T.of('dq0 -p t -d /ram/g');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (bar(c.after) !== 'brackets') f.push('the bar not in its brackets as the game carries on: ' + bar(c.after));
        want('open', 'PO_UNBARRED', 'floor', c.k + 1);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0load', what: 'Dingus Quest Zero\'s load (the user\'s, 2026-10-16), by the things\' weights (the catalogue\'s, all the character carries) and the referee\'s summary (its load: HarnMaster\'s encumbrance, the load over its endurance, as the game\'s bands): the kit alone 7 lb, a little burdened; with a pick, a shovel, a rope, a sword and bones (dq0 -k) 33 lb, heavily burdened; some put down, 15 lb, a little burdened again (the sheet\'s penalties the referee\'s, its unit tests\'; the words: dq0wordsthings)',
      init: 't_rc', cycles: 700e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -k pick,shovel,rope,sword,bones', 'drop rope', 'drop shovel', 'drop sword', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        // (A thing's weight its kind's, in quarter pounds, times how many but a plural kind's (K_PLURAL: 3 rations one lot):
        // the catalogue's, as the things weigh them)
        const T = dq0Turns(m), f = [], lb = s => dq0Carried(s).reduce((w, i) => w + i.weight * (i.kflags & 0x40 ? 1 : Math.max(1, i.qty)), 0) / 4, load = s => s ? s.sum.load : -1;
        const k = T.of('dq0 -p t -s 3 -c /ram/c.hm').after, h = T.of('dq0 -p t -s 3 -c /ram/c.hm -k pick,shovel,rope,sword,bones').after, d = T.of('drop sword').after;
        for (const [s, w, l, what] of [[k, 7, 1, 'the kit alone'], [h, 33, 2, 'with the things more'], [d, 15, 1, 'some put down']]) {
          if (!s || lb(s) !== w) f.push(what + ': ' + (s && lb(s)) + ' lb carried, not ' + w);
          if (load(s) !== l) f.push(what + ': load ' + load(s) + ', not ' + l + ' (1 a little burdened, 2 heavily)');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pace', meaning: true, what: 'Dingus Quest Zero\'s pace and watchfulness (the user\'s, 2026-10-16: DUNGEON.md, moving), by the world\'s state of them (the hub\'s gait and watch: the UI\'s requests, WQ_SET): a walk to begin with; a jog set, then searching for traps as it goes, listening too, each stopped, mapping as it goes and stopped; a sprint set, a slow walk; "run north" setting the run as it goes (the move the command\'s), "run south" going on at it; a creep, listening as it goes; kept (dq0 -d: the world\'s world.dat) and the game carried on at them (its words: dq0wordsthings; the sheet\'s pace line, the referee\'s: dq0harm)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -d /ram/p', 'status', 'jog',
        'search for traps as I go', 'listen as I go', 'stop searching', 'stop listening', 'map as I go', 'stop mapping', 'set pace to sprint',
        'walk slowly', 'run north', 'run south', 'creep', 'listen as I go', 'quit', 'dq0 -p t -d /ram/p', 'status', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        // (rs.h's GA_CREEP 0 ... GA_SPRINT 5; hub.h's WA_TRAPS 1, WA_LISTEN 2, WA_MAP 4)
        const T = dq0Turns(m), f = [];
        const want = [['status', 2, 0], ['jog', 3, 0], ['search for traps as I go', 3, 1], ['listen as I go', 3, 3], ['stop searching', 3, 2],
          ['stop listening', 3, 0], ['map as I go', 3, 4], ['stop mapping', 3, 0], ['set pace to sprint', 5, 0], ['walk slowly', 1, 0],
          ['run north', 4, 0], ['run south', 4, 0], ['creep', 0, 0], ['listen as I go', 0, 2], ['dq0 -p t -d /ram/p', 0, 2], ['status', 0, 2]];
        let k = 0;
        for (const [l, gait, watch] of want) {
          const t = T.of(l, k), s = t.after;
          k = t.k + 1;
          if (t.k < 0 || !s || s.pc.gait !== gait || s.pc.watch !== watch) { f.push(l + ': pace ' + (s && s.pc.gait) + ', watching ' + (s && s.pc.watch) + ', not ' + gait + ', ' + watch); if (t.k < 0) break; }
        }
        dq0Need(f, T.of('dq0 -p t -d /ram/p'), 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0steps', meaning: true, what: 'Dingus Quest Zero\'s small moves (the user\'s, 2026-10-16: "move left one space"; a square 5 feet, the game\'s), by where the character stands and faces (the hub\'s): at seed 12\'s first room (a small storeroom), facing northeast, a step forward (a square that way, its facing kept), a move to the right three spaces stopped by the wall (not moved), backing up 10 feet (the room\'s one square behind it, still facing northeast), a sidestep left into the wall; turned about, "run 2 squares east" (its pace set to a run, east one square to the wall, facing east) (a step stopped by a denizen in the way: dq0denizens; the look as a run ends, brief: dq0wordsthings)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 12 -c /ram/c.hm', 'step forward',
        'move right 3 spaces', 'back up 10 feet', 'sidestep left', 'turn around', 'run 2 squares east', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], p0 = (T.of('dq0 -p t -s 12 -c /ram/c.hm').after || {}).pc;
        if (!p0 || p0.face !== 1) return ['not begun facing northeast: ' + JSON.stringify(p0)];
        const at = (l, dx, dy, face, why) => {
          const t = T.of(l), p = t.after && t.after.pc;
          if (!p || p.x !== p0.x + dx || p.y !== p0.y + dy || p.face !== face)
            f.push(l + ': at ' + (p ? (p.x - p0.x) + ',' + (p.y - p0.y) + ' facing ' + p.face : '?') + ', not ' + dx + ',' + dy + ' facing ' + face);
          if (why) dq0Need(f, t, 'E_NOWAY', { why }, l + ': not refused');
          return t;
        };
        at('step forward', 1, -1, 1);
        at('move right 3 spaces', 1, -1, 1, 'N_NO_EXIT');
        at('back up 10 feet', 0, 0, 1);
        at('sidestep left', 0, 0, 1, 'N_NO_EXIT');
        at('turn around', 0, 0, 5);
        const r = at('run 2 squares east', 1, 0, 2);
        if (!r.after || r.after.pc.gait !== 4) f.push('the pace not set to a run (GA_RUN): ' + (r.after && r.after.pc.gait));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0watch', what: 'Dingus Quest Zero\'s watchfulness and fatigue (the user\'s, 2026-10-16), by the world\'s events and the referee\'s summary (HarnMaster\'s fatigue, the book\'s): a keen-eyed character (Awareness 95, hmchar\'s file) at seed 16 (dq0 -T) searching for traps as it goes, the search where it stops finding the cave\'s hidden plate first (dq0traps\' walk onto it, not searching, sets it off), and the walk going round it; then a character a level tired and nearly another (pc.hm\'s vol and pace lines: the pace a run) running down a passage, tired a level more (its universal penalty one more); one two levels tired resting ten minutes (61 rounds waited), a level gone',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/r.hm', 'cp /ram/c.hm /ram/t.hm', 'echo ml awa 95 >> /ram/c.hm',
        'dq0 -p t -s 16 -c /ram/c.hm -T', 'face east', 'force', 'e', 'force', 'force', 'search for traps as I go', 'e', 'e', 'quit',
        'echo vol 1 0 0 0 0 >> /ram/t.hm', 'echo pace 4 0 5990 >> /ram/t.hm', 'dq0 -p t -s 16 -c /ram/t.hm -T', 'face east', 'force', 'e', 'quit',
        'echo vol 2 0 0 0 0 >> /ram/r.hm', 'dq0 -p t -s 16 -c /ram/r.hm -T', ...Array(61).fill('z'), 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], s = T.of('search for traps as I go'), e1 = T.after(s, 'e'), e2 = T.after(e1, 'e');
        if (!s.after || !(s.after.pc.watch & 1)) f.push('not searching for traps as it goes');
        dq0Need(f, e1, 'E_TRAP', { tk: 'TK_BLADE', tt: 'TT_PLATE', to: 'TO_FOUND' }, 'the plate not found by the search as it went');
        for (const t of [e1, e2]) dq0Never(f, t, 'E_TRAP', { to: 'TO_SPRUNG' }, 'the plate set off, searching');
        dq0Need(f, e2, 'E_ARRIVE', {}, 'the walk not gone on round the plate');
        // (The referee's universal penalty, the status line's "UP n": fatigue's here, no wounds)
        const up = x => x ? +((x.status.match(/UP (\d+)/) || [])[1]) : NaN;
        const t0 = T.of('dq0 -p t -s 16 -c /ram/t.hm -T'), te = T.after(t0, 'e');
        if (!t0.after || t0.after.pc.gait !== 4) f.push('the tired character\'s pace not its file\'s run');
        if (!(up(t0.after) === 1 && up(te.after) === 2)) f.push('running not tiring a level more: UP ' + up(t0.after) + ', then ' + up(te.after));
        const r0 = T.of('dq0 -p t -s 16 -c /ram/r.hm -T'), rz = T.of('quit', r0.k + 1);
        if (!(up(r0.after) === 2 && up(rz.at) === 1)) f.push('resting ten minutes not a level less tired: UP ' + up(r0.after) + ', then ' + up(rz.at));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0theme', meaning: true, what: 'Dingus Quest Zero\'s themes (theme.h: data, games/dq0/themes, made dq0t.dat), by the theme bank, the world\'s events and the things\' records: the theme picked by the seed (its second byte: seed 259 the second theme, the drowned temple; seed 3 the first, the crypt), kept in the save and the game carried on in it, its strata named by it (the senses\' record\'s stratum, the theme\'s names); the way out shut by its lock; then the crypt\'s puzzle\'s pieces (dq0 -k: the sunstone and the moonstone to hand) combined into the king\'s seal (its rule\'s: the stones gone, the seal carried), the seal used on the way out (it opens: the world\'s way_open), and taken: the game ended (escaped, E_LEFT), and the save marked done (a new game begun in its folder, not carried on) (its words, the theme\'s: the themes\' own data)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 259 -c /ram/c.hm -d /ram/t', 'u', 'quit', 'dq0 -d /ram/t',
        'map', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -d /ram/q', 'u', 'combine the sunstone with the moonstone', 'use the seal on the way out',
        'u', 'dq0 -p t -c /ram/c.hm -d /ram/q', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], theme = t => t.after ? t.after.theme : '?';
        const g0 = T.of('dq0 -p t -s 259 -c /ram/c.hm -d /ram/t'), g1 = T.of('dq0 -d /ram/t'), g2 = T.of('dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -d /ram/q');
        if (theme(g0) !== 'the Drowned Temple') f.push('seed 259 not the second theme: ' + theme(g0));
        dq0Need(f, T.after(g0, 'u'), 'E_NOWAY', { why: 'N_SHUT' }, 'the way out not shut by its lock');
        dq0Need(f, g1, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (theme(g1) !== 'the Drowned Temple') f.push('the theme not kept with the game: ' + theme(g1));
        const mp = T.after(g1, 'map');
        const st = mp.after && mp.after.sight ? mp.after.strata[mp.after.sight.sname] : '?';
        if (st !== 'Temple Steps') f.push('the stratum not named by the theme: ' + st);
        if (theme(g2) !== 'the Crypt of the Dead King') f.push('seed 3 not the first theme: ' + theme(g2));
        dq0Need(f, T.after(g2, 'u'), 'E_NOWAY', { why: 'N_SHUT' }, 'the crypt\'s way out not shut');
        const c = T.of('combine the sunstone with the moonstone'), has = (t, n) => dq0Carried(t.after).some(i => i.name === n);
        if (!dq0Rep(c, 'ireq', { kind: 'IO_COMBINE', result: 'IR_OK' }).length) f.push('the stones not combined');
        if (!has(c, 'king\'s seal') || has(c, 'sunstone') || has(c, 'moonstone')) f.push('the stones not made the king\'s seal');
        const u = T.of('use the seal on the way out');
        if (!u.after || !u.after.wayOpen || (u.at && u.at.wayOpen)) f.push('the way out not opened by the seal');
        const l = T.after(u, 'u');
        dq0Need(f, l, 'E_LEFT', {}, 'the way out not taken (escaped)');
        dq0Need(f, T.and(l), 'E_BYE', {}, 'the game not ended');
        dq0Need(f, T.of('dq0 -p t -c /ram/c.hm -d /ram/q'), 'E_ARRIVE', { how: 'A_START' }, 'the done save carried on, not a new game begun');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pieces', meaning: true, what: 'Dingus Quest Zero\'s puzzle\'s pieces placed with the plan (theme.h: each one\'s stratum from the seed, its chunk in the stratum\'s halls as its plan is grown, put down as the chunk is made, by the maker\'s stock), by what the character perceives and the things\' records and replies: the crypt\'s stone tablet on the top stratum (dq0 -T -T: the character begins on it), seen at its feet, taken and read (the parser\'s read: a piece of the puzzle found to hand), a dagger not (nothing written on it); then the crown and the throne (dq0 -k), the throne put down and not taken up again (fixed where it stands: refused), the crown put on it: the place ruled, the game ended (the theme\'s ending; its words, the theme\'s data)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -T', 'take the tablet', 'read the tablet',
        'read the dagger', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -k crown,throne', 'drop the throne', 'take the throne', 'put the crown on the throne', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], rep = (l, kind, result, what) => { if (!dq0Rep(T.of(l), 'ireq', { kind, result }).length) f.push(what + ' (' + JSON.stringify(T.of(l).replies.filter(r => r.req === 'ireq').map(r => r.kind + ' ' + r.result)) + ')'); };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -T').after;
        if (!s0 || !s0.sight || !s0.sight.items.some(i => i.name === 'stone tablet' && i.rel === 255)) f.push('the stone tablet not seen at its feet as it begins');
        rep('take the tablet', 'IO_GET', 'IR_OK', 'the tablet not taken');
        if (!dq0Carried(T.of('take the tablet').after).some(i => i.name === 'stone tablet')) f.push('the tablet not carried');
        rep('read the tablet', 'IO_FIND', 'IR_OK', 'the tablet not read (a piece found to hand)');
        rep('read the dagger', 'IO_FIND', 'IR_NONE', 'the dagger read as a piece');
        const d = T.of('drop the throne');
        if (!dq0Here(d.after).some(i => i.name === 'throne')) f.push('the throne not put down');
        rep('take the throne', 'IO_GET', 'IR_NO', 'the throne taken up again (not fixed where it stands)');
        const p = T.of('put the crown on the throne');
        rep('put the crown on the throne', 'IO_PUT', 'IR_OK', 'the crown not put on the throne');
        dq0Need(f, p, 'E_BYE', {}, 'the game not ended, the place ruled');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0themegen', what: 'Dingus Quest Zero\'s theme generator (Phase 3D; games/themegen, its bank programs bank/histgen, goalgen, linegen and namegen: HISTORY.md): "themegen -s 3" says a region\'s history and its theme (the region, its peoples and eras, the strata as layers, the inhabitants, the goal, its clues, the story), the same each time (made twice into folders, the files the same, cmp); its record (themec, run in its place, theme.dat) held to games/dq0/make.js\'s compile of the theme it said, byte for byte, and that theme to the theme checker (every goal reachable, every faction placed)',
      init: 't_rc', cycles: 900e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'themegen -s 3', 'themegen -q -s 3 -o /ram/a', 'echo $status',
        'themegen -q -s 3 -o /ram/b', 'cmp /ram/a/theme.txt /ram/b/theme.txt', 'cmp /ram/a/theme.dat /ram/b/theme.dat', 'echo $status',
        'xd /ram/a/theme.dat', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['seed     | 3', 'region   | the ', 'era      | ', 'layer    | ', 'faction  | ', 'goal     | ', 'story    | Under the',
        '% cmp /ram/a/theme.dat /ram/b/theme.dat\n% echo $status\n\n', '% echo done\ndone\n'],
      check(m, out) {
        const f = [], text = saidTheme(out, 'themegen -s 3'), dq0 = DQ0_MAKE(), beasts = DQ0_BEASTS();
        let r;
        try { r = dq0.compile(text, 'themegen -s 3', beasts, 1); } catch (e) { return ['its theme isn\'t one: ' + e.message]; }
        f.push(...dq0.check(r.t, beasts, 'themegen -s 3'));
        const dat = xdBytes(out, 'xd /ram/a/theme.dat');
        if (dat.length !== dq0.THEME_SIZE) f.push('theme.dat: ' + dat.length + ' bytes, not ' + dq0.THEME_SIZE);
        else for (let i = 0; i < dat.length; i++) if (dat[i] !== r.data[i]) { f.push('theme.dat: byte ' + i + ' is ' + dat[i] + ', make.js\'s ' + r.data[i]); break; }
        if (/^\s+\S/m.test(text.split('\n').filter(l => /^[a-z]/.test(l)).join('\n'))) f.push('a line not in the theme format');
        this.notes = [(text.match(/^theme +\| (.*)/m) || [])[1] + ': ' + text.split('\n').filter(l => /^[a-z]/.test(l)).length + ' lines, ' + (r.used - dq0.HEAD) + ' bytes of words'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0goals', what: 'Dingus Quest Zero\'s goal kinds (G5: data, games/themegen/goals.txt): themegen\'s histories for seeds 1-12, each passing the theme checker (themec, in themegen\'s place: its status), all four kinds among them (an artifact found, the evil cleared out, a prisoner rescued, a far exit won through to), each goal\'s stratum one of its history\'s and its guardians a faction there',
      init: 't_rc', cycles: 3000e6,
      machine: { input: ['bind -a /sd/0/games /bin', ...[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12].flatMap(s => ['themegen -q -s ' + s + ' -o /ram/g',
        'echo seed ' + s + ' status $status', 'grep goal /ram/g/theme.txt']), 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['seed 12 status\n', '% echo done\ndone\n'],
      check(m, out) {
        const o = out.replace(/\r/g, ''), f = [], kinds = new Set();
        for (let s = 1; s <= 12; s++) {
          if (!o.includes('seed ' + s + ' status\n')) f.push('seed ' + s + ': the checker refused it (or it wasn\'t made)');
          const g = new RegExp('% grep goal /ram/g/theme.txt\\n(goal +\\|.*)').exec(o.slice(o.indexOf('seed ' + s + ' status')));
          if (!g) f.push('seed ' + s + ': no goal line'); else kinds.add(g[1].split('|')[1].trim());
        }
        for (const k of ['artifact', 'evil', 'prisoner', 'through']) if (!kinds.has(k)) f.push('no ' + k + ' goal among seeds 1-12');
        this.notes = ['goal kinds: ' + [...kinds].join(', ')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0themec', what: 'Dingus Quest Zero\'s theme compiler (games/themec): the stand-in themes (the card\'s lib/dq0/themes) compiled on the Hydra into the same records as make.js makes them at build time (dq0t.dat\'s, but for their count), byte for byte; and the theme checker refusing a theme whose way out can\'t be opened (no rule opens it) and whose faction can\'t live where it\'s said to (kobolds on a built stratum), said why',
      init: 't_rc', cycles: 900e6,
      machine: { input: ['bind -a /sd/0/games /bin', ...['1crypt', '2temple', '3warren'].flatMap(n => ['themec -o /ram/' + n + '.dat /sd/0/games/lib/dq0/themes/' + n + '.txt',
        'xd /ram/' + n + '.dat']), 'echo \'theme    | the Bad Place\' >/ram/bad.txt', 'echo \'mission  | escape\' >>/ram/bad.txt',
        'echo \'stratum  | Top | b | halls\' >>/ram/bad.txt', 'echo \'faction  | kobolds | kobold | 0-0 | 9 | - | 0 | u | -\' >>/ram/bad.txt',
        'themec /ram/bad.txt', 'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['the Crypt of the Dead King: ', 'the Drowned Temple: ', 'the Kobold Warren: ', 'the theme checker: the way out can\'t be opened',
        'the theme checker: kobolds: none of its kinds lives in its strata', '% echo $status\n1\n'],
      check(m, out) {
        const f = [], dq0 = DQ0_MAKE(), beasts = DQ0_BEASTS();
        for (const n of ['1crypt', '2temple', '3warren']) {
          const r = dq0.compile(fs.readFileSync(path.join(__dirname, '..', 'games', 'dq0', 'themes', n + '.txt'), 'latin1'), n, beasts, 1).data;
          const dat = xdBytes(out, 'xd /ram/' + n + '.dat');
          if (dat.length !== r.length) { f.push(n + ': ' + dat.length + ' bytes, not ' + r.length); continue; }
          for (let i = 0; i < r.length; i++) if (dat[i] !== r[i]) { f.push(n + ': byte ' + i + ' is ' + dat[i] + ', make.js\'s ' + r[i]); break; }
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0history', what: 'Dingus Quest Zero\'s own theme (theme.h): a game whose seed is past the stand-ins\' (65,536 and on, as a clock\'s is) has its region\'s history and its theme made as it begins (themegen, run by the UI before the world: "The place\'s history is being made"), into its save\'s folder (theme.txt, theme.dat), its story told the theme\'s word for word (its name, then its story: the region, its eras, who has it now, the goal), the top stratum named by it; quit and carried on (dq0 -d), the same theme read back from the folder, not made again',
      init: 't_rc', cycles: 1500e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 70001 -c /ram/c.hm -d /ram/h', 'map', 'quit',
        'ls /ram/h', 'cat /ram/h/theme.txt', 'dq0 -d /ram/h', 'quit', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['The place\'s history is being made', 'Under the ', 'theme.dat', 'theme.txt', 'takes up where they left off.', '% echo done\ndone\n'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), f = [], dq0 = DQ0_MAKE();
        const text = saidTheme(out, 'cat /ram/h/theme.txt');
        let r;
        try { r = dq0.compile(text, 'theme.txt', DQ0_BEASTS(), 1); } catch (e) { return ['its theme.txt isn\'t a theme: ' + e.message]; }
        const words = s => s.replace(/\s+/g, ' ').trim(), name = (text.match(/^theme +\| (.*)/m) || [])[1] || '?';
        const story = words(r.data.toString('latin1', r.data.readUInt16LE(776), r.data.indexOf(0, r.data.readUInt16LE(776))));
        const game = words(o.slice(o.indexOf('history is being made'), o.indexOf('enters the dungeon.')));
        const title = name[0].toUpperCase() + name.slice(1) + '.';
        if (!game.includes(title)) f.push('its name not told: ' + title);
        if (!game.includes(story)) f.push('its story not told word for word: ' + story.slice(0, 80) + ' ...');
        const top = (text.match(/^stratum +\| ([^|]*)\|/m) || [])[1].trim();
        if (!o.includes(top.slice(0, 14))) f.push('the top stratum not named by it: ' + top);
        const again = o.slice(o.indexOf('% dq0 -d /ram/h'));
        if (again.includes('history is being made')) f.push('carried on, its history made again');
        if (!again.includes(title)) f.push('carried on, its name not told: ' + title);
        this.notes = [name + ': ' + text.split('\n').filter(l => /^[a-z]/.test(l)).length + ' lines; ' + (story.length) + ' letters of story'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0layers', meaning: true, what: 'Dingus Quest Zero\'s dungeon made from its history (Phase 3D, games/dq0/HISTORY.md; D12, the user\'s: "A temple could sit over the ruin of an older keep, say.  I just want the things on the same strata to make sense"): at seed 70018 (a region of its own, themegen\'s) the top stratum the tombs the hill-men built, fallen in, and the temple the sun-priests made of them after (its layers: built, ruined, reused): by the way in (those who came after came in that way) the chunks the maker made the temple\'s (a kitchen or a pantry among them, no main room a crypt), and by the way down (dq0 -T, the far side) the old tombs\' (each main room a crypt, no kitchen nor pantry); stratagen (bank/stratagen, a bank program: a stratum\'s layers chunk by chunk) loaded by themegen (its checker) and by the maker, once for all the maker\'s chunks, and with dq0 -C (the bank programs given up after each request) again and again; then at seed 70008 a keep the conquerors made of an older temple: in a guardroom of the keep\'s by the way in, the temple\'s chapel\'s candle stubs (the older layer\'s mark: a guardroom\'s own use leaves none)',
      init: 't_rc', cycles: 1300e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 70018 -c /ram/c.hm', 'quit',
        'dq0 -p t -s 70018 -c /ram/c.hm -T -C', 'quit', 'dq0 -p t -s 70008 -c /ram/c.hm', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) { dq0Made(this, m); },
      check(m) {
        const T = dq0Turns(m), f = [], g = this.games || [], opens = this.opens || [];
        if (!g[0] || !g[1] || !g[2]) return ['the three games\' quits not all seen'];
        const has = (c, u) => c.rooms.some(r => r.kind === u), at = c => c.cx + ',' + c.cy, uses = c => at(c) + ' ' + c.rooms.map(r => r.kind).join('/');
        const a = g[0].chunks, b = g[1].chunks;
        if (!a.length || !b.length) return ['no chunks of the top stratum made (by the way in: ' + a.length + ', by the way down: ' + b.length + ')'];
        if (!a.some(c => has(c, U_KITCHEN) || has(c, U_PANTRY))) f.push('by the way in no kitchen nor pantry, the temple\'s (' + a.map(uses).join('; ') + ')');
        for (const c of a) if (mainRoom(c).kind === U_CRYPT) f.push('by the way in, chunk ' + at(c) + '\'s main room a crypt, the old tombs\'');
        for (const c of b) {
          if (mainRoom(c).kind !== U_CRYPT) f.push('by the way down, chunk ' + at(c) + '\'s main room not a crypt (' + uses(c) + ')');
          if (has(c, U_KITCHEN) || has(c, U_PANTRY)) f.push('by the way down, chunk ' + at(c) + ' the temple\'s (' + uses(c) + ')');
        }
        // (stratagen's loads by game, by its caller: the task that read themegen's exports, before game k's hub was made,
        // or the maker's, after it)
        const loads = (k, exp) => {
          const h = exp === 'themegen.exp' ? k : k + 1, here = opens.filter(o => o.hubs === h);
          const ts = new Set(here.filter(o => o.path.endsWith('/' + exp)).map(o => o.T));
          return here.filter(o => ts.has(o.T) && o.path.endsWith('/stratagen.bp')).length;
        };
        for (const k of [0, 1, 2]) if (loads(k, 'themegen.exp') !== 1) f.push('game ' + (k + 1) + ': stratagen loaded by themegen ' + loads(k, 'themegen.exp') + ' times, not once');
        if (loads(0, 'dq0m.exp') !== 1) f.push('stratagen loaded by the maker ' + loads(0, 'dq0m.exp') + ' times for its ' + a.length + ' chunks, not once (kept)');
        if (loads(1, 'dq0m.exp') < 2) f.push('with dq0 -C, stratagen loaded by the maker ' + loads(1, 'dq0m.exp') + ' times, not again after it was given up');
        // (Seed 70008: the candle stubs in a guardroom of a chunk by the way in)
        const s = T.of('quit', T.of('dq0 -p t -s 70008 -c /ram/c.hm').k).at, st = g[2].strata[0];
        if (!s || !st) return f.concat(['seed 70008: no state at its quit']);
        const room = i => {
          const c = g[2].chunks.find(c => c.cx === (i.x - st.x0) >> 5 && c.cy === (i.y - st.y0) >> 5), x = (i.x - st.x0) & 31, y = (i.y - st.y0) & 31;
          return c && c.rooms.find(r => x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
        };
        const stubs = s.items.filter(i => i.name === 'candle stubs' && i.where === 'IW_SQUARE' && i.sn === 0);
        if (!g[2].chunks.some(c => has(c, U_GATEHOUSE))) f.push('seed 70008: no gatehouse by the way in, the keep\'s');
        if (!stubs.some(i => (room(i) || {}).kind === U_GUARDROOM)) f.push('seed 70008: no candle stubs in a guardroom (the temple\'s chapel\'s, under the keep\'s): ' +
          stubs.map(i => (room(i) || { kind: '-' }).kind).join(' '));
        this.notes = [a.length + ' chunks by the way in, ' + b.length + ' by the way down; the maker\'s stratagen loads with -C: ' + loads(1, 'dq0m.exp')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0quest', meaning: true, what: 'Dingus Quest Zero\'s goals from a region\'s history played to their ends (Phase 3D: G5, the user\'s: "Each dungeon needs a goal, and it may be to find a specific artifact, clean out the evil denizens, rescue a prisoner, or just find a way through to a distant exit"; themegen\'s goals.txt kinds): at seed 70000 the Cup of Rowon (an artifact): dq0 -T -T begins by the goal\'s own thing, the jade cup lying hidden where the history put it; with it to hand (dq0 -k) and the seeker\'s journal read, the way out shut, the cup held up to it (open), and up it: escaped; at seed 70007 the curse of Yiskeek (an evil): by the bone idol (dq0 -T -T), the silver bell and the old scroll to hand, the scroll read, the bell laid on the idol: the curse broken (ruled, the game ended); at seed 70012 Voven, the kobolds\' prisoner: the shackle key put to the chained woman, the way out open (hers), and up it: escaped; at seed 70008 the Far Door (a way through): dq0 -T -T -T (four strata grown below the top) the goal\'s stratum (the fourth) with a way out of its own, a far exit, and none on the strata between',
      init: 't_rc', cycles: 2200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 70000 -c /ram/c.hm -T -T', 'quit',
        'dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\'', 'read the journal', 'u', 'use the cup on the way out', 'u',
        'dq0 -p t -s 70007 -c /ram/c.hm -T -T -k \'silver bell,old scroll\'', 'read the scroll', 'put the bell on the idol',
        'dq0 -p t -s 70012 -c /ram/c.hm -k \'shackle key,chained woman\'', 'drop the woman', 'put the key on the woman', 'u',
        'dq0 -p t -s 70008 -c /ram/c.hm -T -T -T', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) { dq0Made(this, m); },
      check(m) {
        const T = dq0Turns(m), f = [], has = (t, n) => dq0Carried(t.after).some(i => i.name === n);
        const g1 = T.of('dq0 -p t -s 70000 -c /ram/c.hm -T -T');
        if (!g1.after || !dq0Here(g1.after).some(i => i.name === 'jade cup' && (i.flags & DQ0_I.HIDDEN))) f.push('dq0 -T -T not begun by the jade cup, the goal\'s, hidden');
        const g2 = T.of('dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\'');
        if (!has(g2, 'jade cup') || !has(g2, 'journal')) f.push('the cup and the journal not to hand (dq0 -k)');
        if (!dq0Rep(T.after(g2, 'read the journal'), 'ireq', { kind: 'IO_FIND', result: 'IR_OK' }).length) f.push('the journal not read (a piece found to hand)');
        dq0Need(f, T.after(g2, 'u'), 'E_NOWAY', { why: 'N_SHUT' }, 'the way out not shut');
        const u = T.after(g2, 'use the cup on the way out');
        if (!u.after || !u.after.wayOpen || (u.at && u.at.wayOpen)) f.push('the way out not opened by the cup');
        const l = T.after(u, 'u');
        dq0Need(f, l, 'E_LEFT', {}, 'the way out not taken (escaped)');
        dq0Need(f, T.and(l), 'E_BYE', {}, 'the artifact\'s game not ended');
        const g3 = T.of('dq0 -p t -s 70007 -c /ram/c.hm -T -T -k \'silver bell,old scroll\'');
        if (!g3.after || !dq0Here(g3.after).some(i => i.name === 'bone idol')) f.push('dq0 -T -T not begun by the bone idol, the curse\'s');
        if (!dq0Rep(T.after(g3, 'read the scroll'), 'ireq', { kind: 'IO_FIND', result: 'IR_OK' }).length) f.push('the old scroll not read (a piece found to hand)');
        const p = T.after(g3, 'put the bell on the idol');
        if (!dq0Rep(p, 'ireq', { kind: 'IO_PUT', result: 'IR_OK' }).length) f.push('the bell not laid on the idol');
        dq0Need(f, T.and(p), 'E_BYE', {}, 'the curse not broken (the game not ended, ruled)');
        const g4 = T.of('dq0 -p t -s 70012 -c /ram/c.hm -k \'shackle key,chained woman\''), k = T.after(g4, 'put the key on the woman');
        if (!k.after || !k.after.wayOpen || (k.at && k.at.wayOpen)) f.push('the prisoner\'s way out not opened by the key');
        const l4 = T.after(k, 'u');
        dq0Need(f, l4, 'E_LEFT', {}, 'the prisoner\'s way out not taken (escaped)');
        dq0Need(f, T.and(l4), 'E_BYE', {}, 'the prisoner\'s game not ended');
        // (Seed 70008, the strata's records at its quit: the goal's stratum, the fourth, with a far exit)
        const st = (this.games || [])[4] ? this.games[4].strata : [];
        const outs = sn => st[sn] ? st[sn].links.filter(l => l.kind === O_WAY_OUT && l.to === O_NOWHERE).length : -1;
        if (outs(3) !== 1) f.push('the Far Door\'s stratum (the fourth) with ' + outs(3) + ' ways out of its own, not one');
        for (const sn of [1, 2]) if (outs(sn) !== 0) f.push('stratum ' + sn + ' with ' + outs(sn) + ' ways out, not the goal\'s');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0handtheme', meaning: true, what: 'Dingus Quest Zero\'s hand-written theme (dq0 -t FILE: games/dq0/themes/1crypt.txt\'s format, its history\'s groups): a theme giving its name, its region and its two strata, made into a game\'s own by themegen (its seed\'s history filling the rest: the head\'s mission and story made, line by line; the eras, the inhabitants, the goal and its puzzle), written into the save\'s folder and compiled (theme.txt, theme.dat), even at a small seed (5: a stand-in\'s, but for -t); the game in it: the theme\'s name, its top stratum the one given',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'echo \'theme    | the Pit of Echoes\' > /ram/t.txt', 'echo \'region   | the Grey Fells | hills | granite | springs | cold\' >> /ram/t.txt',
        'echo \'stratum  | the Upper Pit | bx | keep\' >> /ram/t.txt', 'echo \'stratum  | the Echoing Deep | n\' >> /ram/t.txt',
        'dq0 -p t -s 5 -c /ram/c.hm -t /ram/t.txt -d /ram/y', 'quit', 'cat /ram/y/theme.txt', 'echo done']) },
      expect: ['% echo done\ndone\n'],
      check(m, out) {
        const T = dq0Turns(m), f = [], text = saidTheme(out, 'cat /ram/y/theme.txt'), keyed = k => text.split('\n').filter(l => l.startsWith(k.padEnd(9) + '|'));
        try { DQ0_MAKE().compile(text, 'theme.txt', DQ0_BEASTS(), 1); } catch (e) { f.push('its theme.txt isn\'t a theme: ' + e.message); }
        const want = { theme: ['theme    | the Pit of Echoes'], region: ['region   | the Grey Fells | hills | granite | springs | cold'],
          stratum: ['stratum  | the Upper Pit | bx | keep', 'stratum  | the Echoing Deep | n'] };
        for (const [k, w] of Object.entries(want)) if (keyed(k).join('\n') !== w.join('\n')) f.push('its ' + k + ' lines not the ones given: ' + keyed(k).join(' / '));
        for (const k of ['mission', 'story', 'era', 'faction', 'goal', 'piece']) if (!keyed(k).length) f.push('no ' + k + ' line made');
        const s = T.of('dq0 -p t -s 5 -c /ram/c.hm -t /ram/t.txt -d /ram/y').after;
        if (!s || s.theme !== 'the Pit of Echoes') f.push('the game not in the hand-written theme: ' + (s && s.theme));
        if (!s || !s.strata || s.strata[0] !== 'the Upper Pit') f.push('the top stratum not the one given: ' + (s && s.strata && s.strata[0]));
        this.notes = [text.split('\n').filter(l => /^[a-z]/.test(l)).length + ' lines, ' + keyed('era').length + ' eras, ' + keyed('faction').length + ' factions made'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0plan', meaning: true, what: 'Dingus Quest Zero\'s built strata built to a plan (DUNGEON.md, the user\'s, 2026-10-16: "feast halls near kitchens near pantries and storage"; "Larger rooms tend to have higher ceilings, and corridors and smaller rooms tend to have uniform ceiling heights"; "Prior usage can also inform ... what kinds of detritus"): at seed 3 the crypt\'s Upper Halls (their purpose halls: a domestic suite by the way in), the character beginning in a kitchen, its iron pot lying there (what its use left), and its well (there\'s water); the chunks made, read from the hub\'s cache (world.h\'s struct chunk in the shared segment\'s banks): a hall joined to a kitchen by a passage of its own, through no other room; every hall\'s ceiling higher than the passages\' (the chunk\'s pceil), every storeroom\'s, pantry\'s, cell\'s and bedchamber\'s flat at the passages\' height; every room\'s floor flat, at the stratum\'s one level (but a hall\'s dais, two feet up)',
      init: 't_rc', cycles: 500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const f = [], s = dq0Turns(m).of('dq0 -p t -s 3 -c /ram/c.hm').after, seen = s && s.sight;
        if (!seen || seen.kind !== U_KITCHEN) f.push('not begun in a kitchen (the room\'s use: ' + (seen && seen.kind) + ')');
        if (!seen || !seen.items.some(i => i.name === 'iron pot')) f.push('no iron pot in the kitchen (what its use left)');
        if (!seen || !seen.items.some(i => i.name === 'well')) f.push('no well in the kitchen');
        const ks = builtChunks(m, 0);
        if (!ks.length) return ['no chunks of stratum 0 in the cache'];
        let joined = 0, halls = 0, level = null;
        for (const c of ks) {
          const at = c.cx + ',' + c.cy, inRoom = (x, y) => c.rooms.findIndex(r => x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
          for (const [r, p] of c.rooms.entries()) {
            const zs = c.zones.filter(z => z.room === r);
            if (p.kind === U_HALL) {
              halls++;
              for (const z of zs) if (z.cmid <= c.pceil) f.push(at + ': a hall\'s ceiling ' + z.cmid + ' feet, the passages\' ' + c.pceil);
            } else if ([U_STORE, U_PANTRY, U_CELL, U_QUARTERS].includes(p.kind)) {
              for (const z of zs) if (z.ceil !== 0 || z.cmid !== c.pceil) f.push(at + ': a small room\'s ceiling not flat at the passages\' height');
            }
            for (let y = p.y; y < p.y + p.h; y++) for (let x = p.x; x < p.x + p.w; x++) {
              const e = c.elev[y * 32 + x];
              if (level === null) level = e;
              if (e !== level && !(p.kind === U_HALL && e > level && e <= level + 2)) f.push(at + ': room ' + r + ' not flat at ' + x + ',' + y);
            }
          }
          // (A hall to a kitchen: from the hall's floor through doors and passages only, into the kitchen's)
          for (const [h, H] of c.rooms.entries()) {
            if (H.kind !== U_HALL) continue;
            const seen = new Set(), q = [];
            for (let y = H.y; y < H.y + H.h; y++) for (let x = H.x; x < H.x + H.w; x++) { seen.add(y * 32 + x); q.push(y * 32 + x); }
            while (q.length && !joined) {
              const i = q.pop(), x = i & 31, y = i >> 5;
              for (const [dx, dy] of [[0, -1], [1, 0], [0, 1], [-1, 0]]) {
                const nx = x + dx, ny = y + dy, j = ny * 32 + nx;
                if (nx < 0 || ny < 0 || nx > 31 || ny > 31 || seen.has(j)) continue;
                const t = c.tile[j], r = inRoom(nx, ny);
                if (r >= 0 && c.rooms[r].kind === U_KITCHEN && (t === T_FLOOR || t === T_STEPS)) { joined++; break; }
                if (r < 0 && (t === T_DOOR || t === T_CORR || t === T_STEPS)) { seen.add(j); q.push(j); }
              }
            }
          }
        }
        if (!halls) f.push('no hall in the chunks made');
        if (!joined) f.push('no hall joined to a kitchen by a passage of its own');
        this.notes = [ks.length + ' chunk(s) of the Upper Halls in the cache, ' + halls + ' hall(s), ' + joined + ' joined to a kitchen'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0gate', meaning: true, what: 'Dingus Quest Zero\'s fortified strata (DUNGEON.md: built to a plan; "Fortified versus not are built differently"): at seed 514 the kobold warren\'s Warren Mouth (its purpose a keep, fortified: theme.h\'s P_FORT), the character beginning in its gatehouse (the room the way in comes out in), told so, a portcullis raised and an arrow slit in its walls; read from the hub\'s cache: the gatehouse\'s every way out a portcullis (none a door, none a bare doorway), murder holes in its vault (R_MURDER), its vault higher than the passages\'',
      init: 't_rc', cycles: 500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 514 -c /ram/c.hm', 'x portcullis', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], s = T.of('dq0 -p t -s 514 -c /ram/c.hm').after, seen = s && s.sight;
        if (!seen || seen.kind !== U_GATEHOUSE) f.push('not begun in the gatehouse (the room\'s use: ' + (seen && seen.kind) + ')');
        if (!seen || !seen.doors.some(d => d.kind === 'PK_PORTCULLIS' && (d.state & 1))) f.push('no raised portcullis seen');
        if (!seen || !seen.doors.some(d => d.kind === 'PK_SLIT')) f.push('no arrow slit seen');
        if (!dq0Rep(T.of('x portcullis'), 'wreq', { kind: 'WQ_EXAMINE', res: 0 }).length) f.push('the portcullis not examined');
        let gates = 0;
        for (const c of builtChunks(m, 0)) {
          for (const p of c.rooms) {
            if (p.kind !== U_GATEHOUSE) continue;
            gates++;
            if (!(p.flags & 0x04)) f.push('the gatehouse without its murder holes');
            for (const z of c.zones.filter(z => c.rooms[z.room] === p)) if (z.cmid <= c.pceil) f.push('the gatehouse\'s vault no higher than the passages\'');
            for (let x = p.x - 1; x <= p.x + p.w; x++) for (let y = p.y - 1; y <= p.y + p.h; y++) {
              if (x >= p.x && x < p.x + p.w && y >= p.y && y < p.y + p.h) continue;
              if (x < 0 || y < 0 || x > 31 || y > 31 || c.tile[y * 32 + x] !== T_DOOR) continue;
              const q = c.portals.find(q => q.x === x && q.y === y);
              if (!q || q.kind !== PK_PORTCULLIS) f.push('the gatehouse\'s way out at ' + x + ',' + y + ' ' + (q ? 'a door' : 'a bare doorway'));
            }
          }
        }
        if (!gates) f.push('no gatehouse in the chunks made');
        this.notes = [gates + ' gatehouse(s) in the cache\'s chunks'];
        return f;
      },
      sdcard: true
    },
    // dq0start0, dq0start1: twelve of the 38 seeds (of 1-200) whose start was a dead end before the links were kept off
    // their chunks' edges (each start in a room, by the stairs up to the way out, with a way on), a third of them, six
    // a test (the rest left to the soak's 200 seeds: each game's start a game start's cycles, some 195M)
    ...[[1, 30, 66, 93, 111, 122], [138, 152, 161, 171, 189, 197]].map((seeds, k) => ({
      name: 'dq0start' + k, meaning: true, what: 'Dingus Quest Zero\'s starts (dq0m/plan.c\'s links and the world\'s way out off their chunks\' edges, world.h\'s INSET: room for the room each comes out in, built.c\'s tries all failing else, a fifth of the starts a dead end of a passage), by what the character perceives (the senses\' record): seeds ' + seeds.join(', ') + ' (some of those that began at a dead end), each beginning in a room, by the stairs up to the way out, with a door, a portcullis or a passage on (seen ahead, or turned round)',
      init: 't_rc', cycles: seeds.length * 300e6,      // (A timeout: each game started, turned round and quit)
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        ...seeds.flatMap(s => ['dq0 -p t -s ' + s + ' -c /ram/c.hm', 'turn around', 'quit']), 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        for (const s of seeds) {
          const g = T.of('dq0 -p t -s ' + s + ' -c /ram/c.hm'), a = g.after, b = T.after(g, 'turn around').after;
          if (!a || !a.sight || !b || !b.sight) { f.push('seed ' + s + ': not begun'); continue; }
          if (a.sight.where !== 0) f.push('seed ' + s + ': not in a room (a dead end of a passage?)');
          else if (!a.sight.open.some(o => o.kind === 'O_WAY_OUT' && o.rel === 255)) f.push('seed ' + s + ': not by the way out');
          else if (!a.sight.doors.length && !a.sight.nways && !b.sight.doors.length && !b.sight.nways) f.push('seed ' + s + ': no way on seen');
        }
        return f;
      },
      sdcard: true
    })),


    {
      name: 'cnum', what: 'the numbers in C and assembly: num.h\'s test (ntest: the number libraries from C, printf\'s and scanf\'s %N and %{base}), calc at rc (its operators, functions, bases, digits, errors, its input a line at a time, long results), the assembly sample nsum (numbers.inc, numlib.s)',
      init: 't_rc', cycles: 150e6,
      // (Each line typed at its prompt, as the C test's)
      machine: { input: CNUM_LINES.map(l => 'ā' + l[0] + '\r').join('') },
      expect: [...CNUM_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : l[1] + '\n%')), '\nntest: 0 failed\n%'],
      sdcard: true
    },
    {
      name: 'hmcharhl', what: 'hmchar.hl (games/hmchar: hmchar in hylang, harn.hl in the ROM disk\'s /lib/hylang) at rc: -q, its profile and its file (harnfile.c\'s lines) agreeing, the automatic skills open; then as a player answers (a roll again, a deity, Peoni, chosen after a beginning that isn\'t enough and one there isn\'t, its ritual opened; an occupation out of range, weapons by a beginning of their names, one ambiguous, one twice, an option point\'s second improvement and a third, a skill there isn\'t), the skills it says as they should be; -s refused (hylang\'s random can\'t be seeded); harntest.hl\'s tests of the character it read back; -p elf, a Sindarin (Character 2: not rolled), its profile and file as they should be; -u viking, a human of the Viking culture (not rolled: the user\'s, 2026-10-20)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: 'ābind -a /sd/0/games /bin\r' + 'ācd /ram; hmchar.hl -q -o q.hm; ' + HM_FILE('q.hm') + '\r' + 'āhmchar.hl\r' + HM_PLAY + 'ā' + HM_FILE('Dorn.hm') + '\r' +
        'ācd /ram; hmchar.hl -q -p elf -o e.hm; ' + HM_FILE('e.hm') + '\r' + 'ācd /ram; hmchar.hl -q -p h -u viking -o v.hm; ' + HM_FILE('v.hm') + '\r' +
        'āhmchar.hl -s 3; echo $status\r' + 'āecho \'(do (use "harntest") (def {c} (harn.read "Dorn.hm")) (list (c :other) (harn.eml c 1)))\' | hylang\r' },
      expect: ['Written to q.hm\n', ...HM_DEITY_SAID, 'Which? 99\nThere\'s no such occupation.\nWhich? ?glad\nGladiator, 4 years: Initiative/6, weapons 3/2, Unarmed/6, Physician/2\nWhich? glad\n', 'The second: s\nWhich? More than one begins so.\n',
        'The third: shield\nThat one\'s chosen already.\n', 'Points left: 4. A skill: climb\nNo more than two points in a skill: that one\'s had its share.\n',
        'A skill: zzz\nThere\'s no such skill.\n', 'Written to Dorn.hm\n',
        'hmchar.hl: -s: hylang\'s random can\'t be seeded (hmchar.fs and C\'s hmchar have -s)\nusage: hmchar.hl [-o file] [-q] [-p species] [-c occupation] [-u culture]\n1\n%',
        '=> {{"name Dorn" "occupation Gladiator"} ', 'hylang> => exit'],
      check(m, out) {
        const f = [], q = hmRun(out, 'cd /ram; hmchar.hl -q -o q.hm; ' + HM_FILE('q.hm')), d = hmRun(out, HM_FILE('Dorn.hm'));
        if (!q) return ['-q: its output or its file not found'];
        f.push(...hmcharCheck(q.out, q.file).map(e => '-q: ' + e));
        const play = out.slice(out.indexOf('% hmchar.hl\n'), out.indexOf('% ' + HM_FILE('Dorn.hm')));
        if (!d) return [...f, 'Dorn.hm not found'];
        f.push(...hmcharCheck(play, d.file).map(e => 'played: ' + e));
        // (Climbing: the kept roll's, then improved by its SB once, the second and third refused: two points a skill
        // at most; the Gladiator's weapons; the eml in harntest's test the file's Climbing; Peoni the deity, its ritual
        // open)
        const kept = play.slice(0, play.indexOf('roll again (k or r)? k')), c0 = +((kept.match(/Climbing (\d+)/g) || []).pop() || ' 0').split(' ')[1];
        const c = [...play.matchAll(/Climbing is (\d+)/g)].map(m => +m[1]);
        if (c.length !== 1 || c[0] - c0 <= 0) f.push('Climbing: ' + c0 + ', then ' + c.join(', ') + ': not improved by its SB once');
        if (!/occupation Gladiator\n/.test(d.file) || !['ml dag ', 'ml shd ', 'ml swd '].every(k => d.file.includes(k))) f.push('Dorn.hm: not a Gladiator with Dagger, Shield and Sword');
        if (!/^att dty 6$/m.test(d.file) || !/^ml rpeo \d+$/m.test(d.file) || !/, Piety \d+ \(Peoni\);/.test(play)) f.push('Dorn.hm: Peoni not the deity, or Ritual: Peoni not open');
        const eml = out.match(/=> \{\{"name Dorn" "occupation Gladiator"\} (\d+)\}/);
        if (!eml || +eml[1] !== c[0]) f.push('harntest.hl\'s eml of Climbing: ' + (eml && eml[1]) + ', not ' + c[0]);
        const e = hmRun(out, 'cd /ram; hmchar.hl -q -p elf -o e.hm; ' + HM_FILE('e.hm'));
        if (!e) return [...f, '-p elf: its output or its file not found'];
        f.push(...hmcharCheck(e.out, e.file).map(e => '-p elf: ' + e));
        if (!/, Sindarin (Male|Female), .*; Sindarin /.test(e.out) || !/^att sp 1$/m.test(e.file)) f.push('-p elf: not a Sindarin of the Sindarin culture');
        const v = hmRun(out, 'cd /ram; hmchar.hl -q -p h -u viking -o v.hm; ' + HM_FILE('v.hm'));
        if (!v) return [...f, '-u viking: its output or its file not found'];
        f.push(...hmcharCheck(v.out, v.file).map(e => '-u viking: ' + e));
        if (!/, Human (Male|Female), .*; Viking /.test(v.out) || !/^fam 1 /m.test(v.file)) f.push('-u viking: not a human of the Viking culture');
        this.notes = ['-q: ' + q.out.split('\n')[2]];
        return f;
      },
      sdcard: true
    },
    {
      name: 'hmcharfs', what: 'hmchar.fs (games/hmchar: hmchar in HyForth, harn.fs in the ROM disk\'s /lib/forth) at rc, against C\'s hmchar: -q from a seed, its profile and its file the same as C\'s from it (the dice in the same order, the culture rolled too), the two agreeing and as they should be; then the same seed answered as a player would (a roll again, a deity chosen, an occupation out of range, weapons by a beginning, one ambiguous, one twice, an option point\'s improvements, a skill there isn\'t), its questions, profile and file the same as C\'s; a bad option; harn.fs\'s harn-read of the file, harn-eml and harn-end; a dwarf (-p: a species not rolled, Character 2), the same from C and HyForth, a Khuzdul of the Khuzan culture, its file as it should be; -p refused a species there isn\'t; -u viking (the culture not rolled), the same from both; the occupations by class and sex (the user\'s, 2026-10-20: the book\'s limits), a noblewoman\'s, a nobleman\'s and a serf\'s, the same from both: the Lady hers alone, the nobles and the guilded not the serf\'s, nor a knight',
      init: 't_rc', cycles: 2400e6,
      machine: { input: 'ābind -a /sd/0/games /bin\r' + 'ācd /ram; hmchar.fs -q -s 5 -o x.hm >f.out; cp x.hm f.hm\r' +
        'āhmchar -q -s 5 -o x.hm >c.out; cmp f.out c.out && cmp f.hm x.hm && echo same\r' +
        'ācat f.out; ' + HM_FILE('f.hm') + '\r' +
        'āhmchar.fs -s 5 -o x.hm >fi.out\r' + HM_PLAY + 'ācp x.hm fi.hm\r' +
        'āhmchar -s 5 -o x.hm >ci.out\r' + HM_PLAY + 'ācmp fi.out ci.out && cmp fi.hm x.hm && echo the same\r' +
        'ācat fi.out; ' + HM_FILE('fi.hm') + '\r' + 'āhmchar.fs -s; echo $status\r' +
        'āecho \'require harn.fs s" fi.hm" r/o open-file throw harn-read . 1 harn-eml . harn-end .\' | forth\r' +
        'āhmchar.fs -q -s 9 -p dwarf -o x.hm >fd.out; cp x.hm fd.hm\r' +
        'āhmchar -q -s 9 -p k -o x.hm >cd.out; cmp fd.out cd.out && cmp fd.hm x.hm && echo the dwarf the same\r' +
        'ācat cd.out; ' + HM_FILE('fd.hm') + '\r' + 'āhmchar -p z; echo $status\r' +
        'āhmchar.fs -q -s 9 -u vik -o x.hm >fu.out; cp x.hm fu.hm\r' +
        'āhmchar -q -s 9 -u viking -o x.hm >cu.out; cmp fu.out cu.out && cmp fu.hm x.hm && echo the viking the same\r' +
        'ācat cu.out; ' + HM_FILE('fu.hm') + '\r' + 'āhmchar.fs -u zz; echo $status\r' +
        HM_LIMITS.map(([s, a]) => 'āhmchar.fs -s ' + s + ' -u feudal -o x.hm >f' + s + '.out\r' + a + 'ācp x.hm f' + s + '.hm\r' +
          'āhmchar -s ' + s + ' -u feudal -o x.hm >c' + s + '.out\r' + a + 'ācmp f' + s + '.out c' + s + '.out && cmp f' + s + '.hm x.hm && echo seed ' + s + ' the same\r' +
          'ācat c' + s + '.out; ' + HM_FILE('f' + s + '.hm') + '\r').join('') +
        'āecho end of test\r' },
      expect: ['\nsame\n%', '\nthe same\n%', 'usage: hmchar.fs [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]\n1\n%', '\nthe dwarf the same\n%',
        'usage: hmchar [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]\n1\n%', '\nthe viking the same\n%',
        '% hmchar.fs -u zz; echo $status\nusage: hmchar.fs [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]\n1\n%',
        ...HM_LIMITS.map(([s]) => '\nseed ' + s + ' the same\n%'), '% echo end of test\nend of test\n'],
      check(m, out) {
        const f = [], q = hmRun(out, 'cat f.out; ' + HM_FILE('f.hm')), d = hmRun(out, 'cat fi.out; ' + HM_FILE('fi.hm'));
        if (!q || !d) return ['the runs\' output or files not found'];
        f.push(...hmcharCheck(q.out, q.file).map(e => '-q: ' + e), ...hmcharCheck(d.out, d.file).map(e => 'played: ' + e));
        for (const t of ['Which? More than one begins so.', 'That one\'s chosen already.', 'No more than two points in a skill: that one\'s had its share.', 'There\'s no such skill.'])
          if (!d.out.includes(t)) f.push('played: "' + t + '" not said');
        const DQ = 'Which (a beginning enough; Enter alone, none)? ';     // (Its answers not in its output, a file)
        if (!d.out.includes(DQ + 'Which? More than one begins so.\n' + DQ + 'There\'s no such deity.\n' + DQ + 'Ritual: Peoni is ')) f.push('played: the questions of the deity not as they should be');
        if (!/occupation Gladiator\n/.test(d.file) || !['ml dag ', 'ml shd ', 'ml swd '].every(k => d.file.includes(k))) f.push('fi.hm: not a Gladiator with Dagger, Shield and Sword');
        if (!/^att dty 6$/m.test(d.file) || !/^ml rpeo \d+$/m.test(d.file)) f.push('fi.hm: Peoni not the deity, or Ritual: Peoni not open');
        // (harn-read's ior, then Climbing's EML: its ML, no penalty; Endurance: the profile's)
        const r = out.match(/\| forth\n0 (\d+) (\d+) /), cli = d.file.match(/^ml cli (\d+)$/m), end = d.out.match(/.*Endurance (\d+),/s);
        if (!r || !cli || !end || r[1] !== cli[1] || r[2] !== end[1]) f.push('harn-read, harn-eml, harn-end: ' + (r ? r.slice(1).join(' ') : 'nothing') + ', not ' + (cli && cli[1]) + ' ' + (end && end[1]));
        const w = hmRun(out, 'cat cd.out; ' + HM_FILE('fd.hm'));
        if (!w) return [...f, '-p dwarf: its output or its file not found'];
        f.push(...hmcharCheck(w.out, w.file).map(e => '-p dwarf: ' + e));
        if (!/, Khuzdul (Male|Female), .*; Khuzan /.test(w.out) || !/^att sp 2$/m.test(w.file)) f.push('-p dwarf: not a Khuzdul of the Khuzan culture');
        const u = hmRun(out, 'cat cu.out; ' + HM_FILE('fu.hm'));
        if (!u) return [...f, '-u viking: its output or its file not found'];
        f.push(...hmcharCheck(u.out, u.file).map(e => '-u viking: ' + e));
        if (!/, Human (Male|Female), .*; Viking /.test(u.out) || !/^fam 1 /m.test(u.file)) f.push('-u viking: not a human of the Viking culture');
        // (The limits: each run's menu, its answers' replies, the occupation taken)
        const W = 'Which? ', NO = W + 'There\'s no such occupation.\n', TWO = W + 'Which? More than one begins so.\n';
        const NOBLES = W + 'Noble (Character 15; a knight is a soldier):\n';
        for (const [s, , occ] of HM_LIMITS) {
          const l = hmRun(out, 'cat c' + s + '.out; ' + HM_FILE('f' + s + '.hm'));
          if (!l) { f.push('seed ' + s + ': its output or its file not found'); continue; }
          f.push(...hmcharCheck(l.out, l.file).map(e => 'seed ' + s + ': ' + e));
          if (!l.file.includes('\noccupation ' + occ + '\n')) f.push('seed ' + s + ': not a ' + occ);
          const noble = /^  noble \d+/m.test(l.out), guilded = /^  guilded \d+/m.test(l.out), at = l.out.indexOf(NOBLES), list = l.out.slice(at, l.out.indexOf('\n' + W, at) + 1);
          if (s === '64' && !(noble && guilded && /^  \d+ Herald +\d+ Lady\n$/m.test(list) && l.out.includes(W + 'Lady, 7 years:') &&
            /, Human Female, .*; Feudal Noble, Lady\n/.test(l.out))) f.push('seed 64: a noblewoman not offered the nobles, the guilded and the Lady');
          if (s === '91' && !(noble && guilded && /^  \d+ Herald\n$/m.test(list) && l.out.includes(list + NO + TWO + W + 'Every soldier:') && l.out.includes('Knight MH, 7 years:') &&
            /, Human Male, .*; Feudal Noble, Knight MH\n/.test(l.out))) f.push('seed 91: a nobleman not offered the nobles and a knight, or offered the Lady');
          if (s === '5' && !(!noble && !guilded && at < 0 && l.out.includes(NO + NO + NO + TWO + NO + NO + W + 'Farmer, 4 years:') &&
            /, Human Male, .*; Feudal Serf, Farmer\n/.test(l.out))) f.push('seed 5: a serf offered a noble\'s, a guilded or a knight\'s occupation');
        }
        this.notes = ['-s 5 -q: ' + q.out.split('\n')[2], '-s 9 -p dwarf: ' + w.out.split('\n')[2], '-s 9 -u viking: ' + u.out.split('\n')[2]];
        return f;
      },
      sdcard: true
    },
    {
      name: 'race', what: 'the race sample (sdk/c/samples/race: tasks sharing a segment) at rc: four tasks each add 1 to a counter 300 times (a read, some work, a write); with nothing to keep them apart adds are lost, with a mutex none; the barrier (ready and go)',
      init: 't_rc', cycles: 200e6,
      machine: { input: 'ā/sd/0/sample/c/race 4 300\r' },
      expect: ['the counter: 1200 of 1200: none lost', 'wait for it, using no CPU.\n'],
      check(m, out) {
        const f = [], lost = out.match(/the counter: (\d+) of 1200: (\d+) adds lost/);
        if (!lost) f.push('with nothing to keep them apart, no adds lost (no task switched between a read and its write?)');
        const full = (out.match(/\x1b\[\d+;64H  300/g) || []).length;           // (Each task's count, 300 at the end)
        if (full !== 8) f.push(full + ' tasks\' counts reached 300 in the two races, not 8');
        this.notes = lost ? ['without a lock: ' + lost[2] + ' adds of 1200 lost'] : [];
        return f;
      },
      sdcard: true
    },
    {
      name: 'chorus', what: 'the chorus sample (sdk/c/samples/chorus: the console shared) at rc: four tasks sing a line each, a letter at a time; with nothing between them the letters tangle; with a mutex each line is whole (in the order they took it); with a baton (a semaphore each, passed round) whole and in turn',
      init: 't_rc', cycles: 200e6,
      machine: { input: 'ā/sd/0/sample/c/chorus\r' },
      expect: ['With a baton passed round, a semaphore each (wait for yours, sing, pass it on):\n' + CHORUS.join('\n') + '\n'],
      check(m, out) {
        const f = [], a = out.indexOf('With a mutex, held for a whole line:\n'), b = out.indexOf('\n\nWith a baton');
        const mutex = out.slice(a, b).split('\n').slice(1);
        if (JSON.stringify([...mutex].sort()) !== JSON.stringify([...CHORUS].sort())) f.push('with a mutex, not the four lines whole: ' + JSON.stringify(mutex));
        const tangled = out.slice(out.indexOf('With nothing to keep them apart:\n'), a);
        if (CHORUS.every(l => tangled.includes(l))) f.push('with nothing between them, the lines came out whole');
        this.notes = ['with a mutex, in the order ' + mutex.map(l => CHORUS.indexOf(l) + 1).join(' ')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'philo', what: 'the philo sample (sdk/c/samples/philo: the dining philosophers, each fork a mutex, a shared segment) at rc: five tasks eat three meals each, the lower-numbered fork first, none in two hands at once; then -d, each its left fork first: a deadlock, seen, and ended (the tasks killed, their mutexes given back); then under rc -c, Ctrl-C: philo\'s handler ends it, rc -c waiting for it (then ending, before its next command)',
      init: 't_rc', cycles: 400e6,
      machine: { input: 'ā/sd/0/sample/c/philo -n 3\rā/sd/0/sample/c/philo -d; echo $status\r' +
        'ārc -c \'/sd/0/sample/c/philo; echo after\'\r' + 'Ā'.repeat(14) + '\x03āecho $status\r' },   // (Read from the card: a moment more.  12 waits came as philo set its handler: a ROM's change of a few cycles put it before)
      expect: ['5 philosophers ate 15 meals (3 to 3 each); a fork was in two hands 0 times.\n',
        'Deadlock: each holds their left fork and waits for their right one', 'a fork was in two hands 0 times.\ndeadlock\n',
        'a fork was in two hands 0 times.\n\n% echo $status\ninterrupted\n%'],
      check(m, out) {
        return out.slice(out.lastIndexOf('rc -c \'/sd/0/sample/c/philo')).includes('\nafter') ? ['rc -c went on after Ctrl-C'] : [];
      },
      sdcard: true
    },
    {
      name: 'prodcons', what: 'the prodcons sample (sdk/c/samples/prodcons: counting semaphores and a mutex, a ring in a shared segment) at rc: two producers make 20 items each, two consumers use them, each once; the producers waited for room, and the consumers for items',
      init: 't_rc', cycles: 300e6,
      machine: { input: 'ā/sd/0/sample/c/prodcons -n 20\r' },
      expect: ['Made 40 items (their sum 60420), used 40 (their sum 60420): each once.\n', ' times.\n%'],
      check(m, out) {
        const w = out.match(/The producers waited for room (\d+) times; the consumers for an item (\d+) times/);
        if (!w) return ['no waits line'];
        this.notes = ['the producers waited ' + w[1] + ' times, the consumers ' + w[2]];
        return +w[1] && +w[2] ? [] : ['the ' + (+w[1] ? 'consumers' : 'producers') + ' never waited'];
      },
      sdcard: true
    },
    {
      name: 'bankprog', what: 'bank programs (Phase 3B: sdk/c/include/bankprog.h, sdk/c/lib/bankprog.s; the user\'s design, 2026-10-18): tests/ram/t_bank, a C program, loads a child (tests/bank: o65, linked at $8000) into a RAM bank of its own and calls it, calls it again (still loaded: its count goes on), 200 calls timed; its words in the caller\'s RAM, three arguments; BANK_MAX more loaded, the first given up for them and loaded again as it\'s called; and refused with their reasons: too much zero page, an import its caller hasn\'t (named), no such entry, no such file',
      init: 't_rc', cycles: 600e6,
      pc: { files: () => {
        const o = n => fs.readFileSync(path.join(__dirname, '..', 'obj', 'tests', n)), c = o('count.bp');
        return Object.assign({ 't_bank.hyx': o('t_bank.hyx'), 't_bank.exp': o('t_bank.exp'), 'count.bp': c, 'fill.bp': o('fill.bp'),
          'bigzp.bp': o('bigzp.bp'), 'nosuch.bp': o('nosuch.bp'), 'b1.bp': o('big.bp'), 'b2.bp': o('big.bp') });
      } },
      machine: { input: '\u0101for (f in `{ls /pc}) cp /pc/$f /ram/$f\r\u0101/ram/t_bank.hyx\r' },
      expect: ['t_bank: PASS']
    },
    {
      name: 'bankrun', what: 'bankrun (Phase 3B): a bank program run from the shell, the ROM disk\'s /rom/lib/bank/hello.bp (bank/hello: printf its caller\'s), its arguments said back and its value the status; called 50 times, still loaded, the calls timed; one with no file refused, its reason said',
      init: 't_rc', cycles: 400e6,
      machine: { input: ['bankrun /rom/lib/bank/hello.bp one two; echo status $status', 'bankrun -n 50 /rom/lib/bank/hello.bp >/dev/null',
        'bankrun /rom/lib/bank/none.bp', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['hello from a bank program (/rom/lib/bank/hello.bp) one two\nstatus 2', 'bankrun: /rom/lib/bank/none.bp: no such file',
        '% echo done']
    },
    {
      name: 'round', what: 'the round sample (sdk/c/samples/round: a barrier, then each task its own time) at rc, as a copy named /ram/rnd (the build\'s, through /pc: only $prog can find it): four tasks sing a round on YM2151 channels 0-3, an eighth 12 ticks; each voice\'s 27 notes in time (hy_sleep_until: within 5 ticks), each voice two bars after the one before',
      init: 't_rc', cycles: 220e6,
      pc: { files: () => ({ 'round.hyx': fs.readFileSync(path.join(__dirname, '..', 'obj', 'samples', 'c', 'round.hyx')) }) },
      machine: { input: 'ācp /pc/round.hyx /ram/rnd; /ram/rnd 1 12\r' },
      expect: ['Each voice sang it 1 time, its latest note late by', ' ticks.\n%'],
      check(m, out) {
        // (Each channel's key-ons against the tune's: its note lengths, an eighth 12 ticks, from voice 0's first,
        // voice v's 2 bars (12 eighths) on: each within 5 ticks, a task woken waiting its turn for the CPU)
        const f = [], tick = 3579545 / 200, lens = ROUND_LENS, on = [];
        for (let v = 0; v < 4; v++) on.push(m.ym.keyOns.filter(k => k.startsWith('ch ' + v + ' ')).map(k => +k.match(/at cycle (\d+)/)[1]));
        if (on.some(o => o.length !== lens.length)) return ['key-ons on channels 0-3: ' + on.map(o => o.length).join(', ') + ', not ' + lens.length + ' each'];
        let worst = 0;
        for (let v = 0; v < 4; v++) {
          let at = on[0][0] + v * 12 * 12 * tick;
          lens.forEach((len, k) => {
            const off = (on[v][k] - at) / tick;
            worst = Math.max(worst, Math.abs(off));
            if (Math.abs(off) > 5) f.push('voice ' + (v + 1) + '\'s note ' + (k + 1) + ': ' + off.toFixed(1) + ' ticks from its time');
            at += len * 12 * tick;
          });
        }
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        this.notes = ['the notes within ' + worst.toFixed(1) + ' ticks of their times'];
        return f.slice(0, 5);
      },
      sdcard: true
    },
    {
      name: 'ed', what: 'ed, the line editor: a file made, printed, changed and written; its errors; q twice; Ctrl-C at its prompt; w name',
      init: 't_rc', cycles: 80e6,
      // (rc's prompt waited for, then each session typed ahead: the console keeps the keys till ed reads its lines)
      machine: { input: '\u0101ed /ram/e\r' + 'a\rone\rtwo\rthree\r.\r2p\ri 1\rzero\r.\rp\r2,3d\rc 2\rTHREE\r.\rp\rw\rq\r' +
        '\u0101cat /ram/e\r' + '\u0101ed /ram/e\r' + '9p\rx\rd\ra\rfour\r.\rq\rq\r' +
        '\u0101ed /ram/e\r\u0100\x03\u0100' + 'Q\r' + '\u0101echo $status\r' +
        '\u0101ed\r' + 'a\rx\r.\rw\rw /ram/f\r1,$p\r0a\rfirst\r.\r$p\rh\rQ\r' + '\u0101cat /ram/f; ed a b; echo $status\r' },
      expect: [
        '% ed /ram/e\n/ram/e: new file\n*a\none\ntwo\nthree\n.\n*2p\n   2 two\n*i 1\nzero\n.\n*p\n   1 zero\n   2 one\n' +
          '   3 two\n   4 three\n*2,3d\n*c 2\nTHREE\n.\n*p\n   1 zero\n   2 THREE\n*w\n/ram/e: 11 bytes\n*q\n%',
        '% cat /ram/e\nzero\nTHREE\n%',
        '% ed /ram/e\n/ram/e: 2 lines\n*9p\n? no such line\n*x\n? h: help\n*d\n? which lines?\n*a\nfour\n.\n*q\n' +
          '? not written: q again to quit anyway\n*q\n%',
        '% ed /ram/e\n/ram/e: 2 lines\n*\n?\n*Q\n',
        '% echo $status\n\n%',
        '% ed\n*a\nx\n.\n*w\n? no file name (w name)\n*w /ram/f\n/ram/f: 2 bytes\n*1,$p\n   1 x\n*0a\nfirst\n.\n*$p\n   2 x\n' +
          '*h\np [a[,b]]  print (all)       a [n]      add after n (the last)\n',
          'n: a number, or $ (the last).  Lines typed after a, i or c end with a .\n*Q\n%',
        '% cat /ram/f; ed a b; echo $status\nx\nusage: ed [file]\nusage\n%',
      ],
    },
    {
      name: 'edit', what: 'edit, the screen editor (/rom/bin/edit, its text in RAM banks): a file typed and saved; a line cut and pasted; o replaced with 0, all; a cut undone; a line copied into a second file; a CR LF file kept so; a 20K file (several blocks) cut, pasted and saved',
      init: 't_rc', cycles: 300e6,
      pc: { files: () => ({ 'dos.txt': 'a\r\nb\r\n', 'big.txt': Array.from({ length: 2000 }, (_, i) => 'line ' + String(i + 1).padStart(4, '0') + '\n').join('') }) },
      // (Each session typed ahead, waits (Ā: 2M cycles) where edit reads, writes or starts; M- is Esc then the key)
      get machine() {
        const W = 'Ā', P = 'ā', C = c => String.fromCharCode(c.charCodeAt(0) & 0x1F), M = k => '\x1b' + k;
        return { input: [P, 'echo b115200 >/dev/serctl\r',
          P, 'edit /ram/e.txt\r', W.repeat(4), 'hello\rworld\r', C('O'), W, '\r', W, C('X'),
          P, 'cat /ram/e.txt\r',
          P, 'edit /ram/e.txt\r', W.repeat(4), M('\\'), C('K'), M('/'), C('U'), C('S'), W, C('X'),
          P, 'cat /ram/e.txt\r',
          P, 'edit /ram/e.txt\r', W.repeat(4), M('r'), W, 'o\r', W, '0\r', W, 'a', W, C('S'), W, C('X'),
          P, 'cat /ram/e.txt\r',
          P, 'edit /ram/e.txt\r', W.repeat(4), C('K'), W, M('u'), W, C('X'), W, 'n',
          P, 'cat /ram/e.txt\r',
          P, 'edit /ram/e.txt /ram/g.txt\r', W.repeat(4), M('6'), M('.'), W, C('U'), C('S'), W, C('X'), W, C('X'),
          P, 'cat /ram/g.txt\r',
          P, 'edit /pc/dos.txt\r', W.repeat(4), 'x', C('S'), W.repeat(2), C('X'),
          P, 'wc /pc/dos.txt\r',
          P, 'edit /pc/big.txt\r', W.repeat(8), M('/'), 'end\r', M('g'), W, '1000\r', W, C('K'), C('K'), M('\\'), C('U'),
          C('S'), W.repeat(8), C('X'),
          P, 'wc /pc/big.txt; head -3 /pc/big.txt\r'].join('') };
      },
      expect: ['% cat /ram/e.txt\nhello\nworld\n%', '% cat /ram/e.txt\nworld\nhello\n%', '% cat /ram/e.txt\nw0rld\nhell0\n%',
        '% cat /ram/e.txt\nw0rld\nhell0\n%', '% cat /ram/g.txt\nw0rld\n%', '% wc /pc/dos.txt\n      2       2       7 /pc/dos.txt\n%',
        '% wc /pc/big.txt; head -3 /pc/big.txt\n   2001    4001   20004 /pc/big.txt\nline 1000\nline 1001\nline 0001\n%'],
    },
    {
      name: 'as', what: 'as, the assembler (the module as): the SDK\'s hi from /lib/as, with its include files there, the same bytes as the build\'s (ca65 and ld65), run, its labels file (-l); nsum (numbers.inc\'s macros, numlib.s) the same; t_asall (tests/ram: every opcode in each mode, directives, expressions, labels, macros, conditionals, segments, .include, .incbin) the same as the build\'s; -b with .org; errors with their files and lines; a warning; - for the source (a pipe, a < file; the output - too unless named) and for the output (stdout); -l with - refused',
      init: 't_rc', cycles: 300e6,
      pc: {
        files: () => {
          const t = n => fs.readFileSync(path.join(__dirname, 'ram', 't_asall', n));
          return { 't_asall.s': t('t_asall.s'), 'inc1.inc': t('inc1.inc'), 'data.bin': t('data.bin'),
            't_asall.hyx': fs.readFileSync(path.join(__dirname, '..', 'obj', 'tests', 't_asall.hyx')),
            'raw.s': '.org $C000\nstart:      jmp         start\n            .word       start, * - start\n',
            'bad.s': '; bad.s\n            frob        #1\n            .error      "stop"\n            lda         (1 +\n',
            'warn.s': '; warn.s\n            .warning    "careful"\n            rts\n' };
        },
      },
      machine: {
        input: ['echo b115200 >/dev/serctl', 'as -l /sd/0/sample/as/hi.s /ram/hi; echo $status', 'cmp /ram/hi /sd/0/sample/hi; echo $status', '/ram/hi Ann',
          'grep main /ram/hi.lbl', 'as /sd/0/sample/as/nsum.s /ram/nsum; cmp /ram/nsum /sd/0/sample/nsum; echo $status', 'as /pc/t_asall.s /ram/t_asall; cmp /ram/t_asall /pc/t_asall.hyx; echo $status', 'as -b /pc/raw.s /ram/raw; xd /ram/raw',
          'as /pc/bad.s /ram/bad; echo $status', 'as /pc/warn.s /ram/warn; echo $status; xd /ram/warn',
          'cat /sd/0/sample/as/hi.s | as - /ram/hi2; cmp /ram/hi2 /ram/hi; echo $status', 'as /sd/0/sample/as/hi.s - >/ram/hi3; cmp /ram/hi3 /ram/hi; echo $status',
          'dis /ram/hi | as - >/ram/hi4; cmp /ram/hi4 /ram/hi; echo $status', 'as -l - /ram/hi5 </sd/0/sample/as/hi.s; cmp /ram/hi5 /ram/hi; echo $status; grep main /ram/hi5.lbl',
          'as -l - </sd/0/sample/as/hi.s; echo $status', 'as'].map(l => 'ā' + l + '\r').join(''),
      },
      expect: ['% as -l /sd/0/sample/as/hi.s /ram/hi; echo $status\n\n%', '% cmp /ram/hi /sd/0/sample/hi; echo $status\n\n%', '% /ram/hi Ann\nHello, Ann!\nI\'m task ',
        '% grep main /ram/hi.lbl\nal 000830 .main\n%', '% as /sd/0/sample/as/nsum.s /ram/nsum; cmp /ram/nsum /sd/0/sample/nsum; echo $status\n\n%',
        '% as /pc/t_asall.s /ram/t_asall; cmp /ram/t_asall /pc/t_asall.hyx; echo $status\n\n%',
        '% as -b /pc/raw.s /ram/raw; xd /ram/raw\n0000000  4c 00 c0 00 c0 05 00 ',
        '% as /pc/bad.s /ram/bad; echo $status\nas: /pc/bad.s:2: not an instruction, directive or macro: frob\nas: /pc/bad.s:3: stop\nas: /pc/bad.s:4: a bad expression\n1\n%',
        '% as /pc/warn.s /ram/warn; echo $status; xd /ram/warn\nas: /pc/warn.s:2: warning: careful\n\n0000000  60 ',
        '% cat /sd/0/sample/as/hi.s | as - /ram/hi2; cmp /ram/hi2 /ram/hi; echo $status\n\n%', '% as /sd/0/sample/as/hi.s - >/ram/hi3; cmp /ram/hi3 /ram/hi; echo $status\n\n%',
        '% dis /ram/hi | as - >/ram/hi4; cmp /ram/hi4 /ram/hi; echo $status\n\n%', '% as -l - /ram/hi5 </sd/0/sample/as/hi.s; cmp /ram/hi5 /ram/hi; echo $status; grep main /ram/hi5.lbl\n\nal 000830 .main\n%',
        '% as -l - </sd/0/sample/as/hi.s; echo $status\nas: -l needs the output\'s name (as -l - out): - is stdout\n1\n%', '% as\nusage: as [-bl] file.s|- [out|-]\n%'],
      sdcard: true
    },
    {
      name: 'dis', what: 'dis, the disassembler (/rom/bin/dis: as\'s inverse, the asm library\'s instructions): the SDK\'s tick as as\'s source (its code followed from main, its labels, hydra.inc\'s calls and registers, its data and BSS); sources that as assembles to the same bytes: hi with its labels (as -l), upper in the SDK\'s columns (-w), db (C, 20K) with ld65\'s labels, every opcode (three operands each) raw from an address (-c -o); a bank program (/rom/bin/echo: its text as linked, at $8000); its usage and a file not a program',
      init: 't_rc', cycles: 2500e6,
      pc: {
        files: () => {
          const ops = [];
          for (const [x, y] of [[0x12, 0x34], [0x12, 0x00], [0xFE, 0x7F]]) for (let op = 0; op < 256; op++) ops.push(op, x, y);
          return { 'db.lbl': fs.readFileSync(path.join(__dirname, '..', 'obj', 'programs', 'db', 'db.lbl')), 'ops.bin': Buffer.from(ops) };
        },
      },
      machine: {
        input: ['dis /sd/0/sample/tick', 'as -l /sd/0/sample/as/hi.s /ram/hi; dis -l /ram/hi.lbl /ram/hi >/ram/h.s',
          'as /ram/h.s /ram/h2; cmp /ram/hi /ram/h2; echo $status', "dis -w /sd/0/sample/upper >/ram/u.s; grep -c '^            jsr ' /ram/u.s",
          'as /ram/u.s /ram/u2; cmp /sd/0/sample/upper /ram/u2; echo $status', 'dis -l /pc/db.lbl /rom/bin/db >/ram/db.s; grep -c _exit /ram/db.s',
          'as /ram/db.s /ram/db2; cmp /rom/bin/db /ram/db2; echo $status', 'dis -c -o 1000 /pc/ops.bin >/ram/o.s; as -b /ram/o.s /ram/o2',
          'cmp /pc/ops.bin /ram/o2; echo $status', 'dis /rom/bin/echo | head -12', 'dis',
          'dis /rom/doc/api.md'].map(l => 'ā' + l + '\r').join(''),
      },
      expect: ['% dis /sd/0/sample/tick\n; /sd/0/sample/tick, as dis read it: as assembles this to its bytes again\n.include "hydra.inc"\n\n' +
        '.include "hyx2.inc"\n\n\tHYX2_PROGRAM "tick", main\n.code\nmain:\n\tstz B0895\n\tstz B0896\n\tlda #$86\n\tsta r0\n\tlda #$08\n\tsta r0+1\n' +
        '\tjsr NOTIFY\nL0841:\n\tlda #$C8\n\tldx #$00\n\tjsr SLEEP\n\tlda B0895\n\tbne L0857\n',
        '\tora #$30\n\tjsr PUTC\n', '\trts\n\t.byte $8D, $95, $08, $18, "` seconds", $0A, $00\n.bss\nB0895:\n\t.res 1\nB0896:\n\t.res 1\n%',
        '% as /ram/h.s /ram/h2; cmp /ram/hi /ram/h2; echo $status\n\n%', '% as /ram/u.s /ram/u2; cmp /sd/0/sample/upper /ram/u2; echo $status\n\n%',
        '% as /ram/db.s /ram/db2; cmp /rom/bin/db /ram/db2; echo $status\n\n%', '% cmp /pc/ops.bin /ram/o2; echo $status\n\n%',
        '% dis /rom/bin/echo | head -12\n; /rom/bin/echo, a bank program, as dis read it: its text as linked, at $8000\n.include "hydra.inc"\n\n' +
        '.include "hyx2.inc"\n\n\tHYX2_PROGRAM "echo", main\n.code\nmain:\n\tlda r0\n\tsta $22\n\tlda r0+1\n\tsta $23\n%',
        '% dis\nusage: dis [-cnw] [-l labels] [-o addr] file\n%',
        '% dis /rom/doc/api.md\ndis: /rom/doc/api.md: not a HYX2 RAM program nor a bank program (-o addr: raw bytes from addr)\n%'],
      check(m, out) {
        const f = [], n = out.match(/grep -c '\^            jsr ' \/ram\/u\.s\n(\d+)\n/), k = out.match(/grep -c _exit \/ram\/db\.s\n(\d+)\n/);
        if (!n || +n[1] < 20) f.push('dis -w: upper\'s jsr lines not in the SDK\'s columns (' + (n && n[1]) + ')');
        if (!k || +k[1] < 1) f.push('dis -l: db\'s _exit not named');
        return f;
      },
      sdcard: true
    },
    {
      name: 'sound', what: 'the simulator\'s sound (sim/lib/audio.js: run.js --sound, --wav), at rc: the YM2151\'s (opm.js, ymfm\'s) A4 on channel 4, then the Vera X\'s PSG\'s A5 on a sawtooth (channel 8), each heard at its pitch; the stream 48,000 samples a second of the Hydra\'s time',
      init: 't_rc', cycles: 60e6, jsOnly: 'the danlang emulator has no sound',
      machine: {
        vera: true, sound: true,
        input: ['echo patch 4 0 >/dev/sndctl; echo note 4 69 >/dev/sndctl', 'sleep 1; echo off 4 >/dev/sndctl; echo wave 8 saw >/dev/sndctl; echo note 8 81 >/dev/sndctl',
          'sleep 1; echo off 8 >/dev/sndctl'].map(l => 'ā' + l + '\r').join(''),
      },
      start(m) { this.heard = []; m.audio.on(s => this.heard.push(s)); },
      expect: ['% sleep 1; echo off 8 >/dev/sndctl\n%'],
      check(m) {
        const f = [], n = this.heard.reduce((k, s) => k + s.length / 2, 0), L = new Float64Array(n);
        let k = 0;
        for (const s of this.heard) for (let i = 0; i < s.length; i += 2) L[k++] = s[i];
        // Each 0.1 s that sounds: its pitch, the shortest period (60-2000 Hz) the samples repeat at (a correlation
        // over 0.9 at a local peak)
        const pitches = [];
        for (let s = 0; s + 4800 <= n; s += 4800) {
          let e = 0;
          for (let i = s; i < s + 4800; i++) e += L[i] * L[i];
          if (Math.sqrt(e / 4800) < 300) continue;
          const corr = lag => { let c = 0, e1 = 0, e2 = 0; for (let i = s; i < s + 2400; i++) { c += L[i] * L[i + lag]; e1 += L[i] * L[i]; e2 += L[i + lag] * L[i + lag]; } return c / Math.sqrt(e1 * e2); };
          for (let lag = 24, prev = corr(23); lag < 800; lag++) {
            const c = corr(lag);
            if (c > 0.9 && c >= prev && c >= corr(lag + 1)) { pitches.push(48000 / lag); break; }
            prev = c;
          }
        }
        const near = hz => pitches.filter(p => Math.abs(p - hz) < hz * 0.01).length;
        if (near(440) < 3) f.push('the YM2151\'s A4 not heard: ' + pitches.map(p => p.toFixed(1)).join(', '));
        if (near(880) < 3) f.push('the PSG\'s A5 not heard: ' + pitches.map(p => p.toFixed(1)).join(', '));
        const want = m.cpu.cyc / 3.579545e6 * 48000;
        if (Math.abs(m.audio.made - want) > 16) f.push('the stream: ' + m.audio.made + ' samples for ' + Math.round(want) + ' of the Hydra\'s time');
        this.notes = [n + ' samples heard; 0.1 s pitches near A4 ' + near(440) + ', near A5 ' + near(880)];
        return f;
      },
    },
    {
      name: 'snd', what: 'sound (#a): snd, sndctl and bell; the volume, claims (one another program holds), the shadow, tones (C, snd.h); sndctl\'s channel commands as text (patch, note, level and vol, pan by word and number, bend below 0, off, drum, reg: their registers on the chip; a channel another program has; numbers out of range, or missing); freq (a note by its frequency, its 64ths), glide, lfo, sens, noise',
      init: 't_rc', cycles: 140e6,
      get machine() { return { input: SND_LINES.map(l => '\u0101' + l[0] + '\r').join('') }; },
      get expect() { return SND_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')); },
      // (Channel 4: A4 bent down 32 64ths, G#4 and a half: key code $49 (octave 4, G#), fraction $80; on the left
      // (RL 01).  5: on the right (RL 10), a drum's patch kept it so.  6: $2E and $36 as written)
      check(m) {
        const f = [], keys = m.ym.keyOns.join(', '), r = m.ym.regs, hx = v => '$' + v.toString(16).toUpperCase();
        for (const ch of [0, 1, 2, 3, 4, 5, 7]) if (!m.ym.keyOns.some(k => k.startsWith('ch ' + ch + ' '))) f.push('no key-on on channel ' + ch + ': ' + keys);
        for (const [reg, want, mask, what] of [[0x2C, 0x49, 0xFF, 'note 4 69, bend 4 -32: its key code'], [0x34, 0x80, 0xFC, 'its fraction'],
          [0x24, 0x40, 0xC0, 'pan 4 left'], [0x25, 0x80, 0xC0, 'pan 5 2 (a drum after it)'], [0x2E, 0x4A, 0xFF, 'reg 46 74'], [0x36, 0xFC, 0xFF, 'reg 54 252'],
          [0x2A, 0x5D, 0xFF, 'freq 2 1000: its key code (B5)'], [0x32, 0x34, 0xFC, 'its fraction (13 64ths)'], [0x2B, 0x4E, 0xFF, 'glide 3 72 (C5)'],
          [0x33, 0x00, 0xFC, 'its fraction'], [0x18, 200, 0xFF, 'lfo: the rate'], [0x19, 20, 0xFF, 'its amplitude depth (the last written)'],
          [0x1B, 2, 0x03, 'its waveform'], [0x3B, 0x52, 0xFF, 'sens 3 5 2'], [0x0F, 0x89, 0xFF, 'noise 9']])
          if ((r[reg] & mask) !== want) f.push(what + ': register ' + hx(reg) + ' is ' + hx(r[reg]) + ', not ' + hx(want) + (mask !== 0xFF ? ' (mask ' + hx(mask) + ')' : ''));
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        this.notes = ['the YM2151: ' + m.ym.keyOns.length + ' key-ons'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'mml', what: 'scores (play\'s, modules/play/mml.inc: hysong.js\'s language compiled as it plays): play -o\'s ZSM of the old test song (every channel, algorithm and LFO waveform, noise, slides, legato, drums, repeats, the timers) and of scom, each the same as hysong.js\'s byte for byte; scom played as a score and as hysong.js\'s ZSM, the chip\'s writes the same, in the same order, and in time; play -m (a line on a channel, its own instrument), -c (a chord, a note a channel), -x (the X16\'s MML: T, upper-case notes, S0 legato, K), I (a patch by number); a score\'s errors, the lines\'',
      init: 't_rc', cycles: 700e6,
      get machine() {
        return { sd: mmlCard(), ymLog: true, input: ['cd /sd/0', 'play -o t.mml /ram/t.zsm; cmp /ram/t.zsm tpc.zsm && echo same',
          'play -o s.mml /ram/s.zsm; cmp /ram/s.zsm spc.zsm && echo same', 'play bad.mml; echo $status', 'play bad2.mml',
          'echo reset >/dev/sndctl; play spc.zsm; echo reset >/dev/sndctl; play s.mml; echo played',
          'echo patch 0 0 >/dev/sndctl; echo patch 1 0 >/dev/sndctl; play -m 0 o4 l8 c d e; play -c 0 o4 l2 I0 c e g',
          'play -x -m 1 T240 O4 L8 CDE S0 CD K E; echo lines', 'play -m 24 c; play -c 0 c d e f g a b c d',
          'play -m 0 I0 c t100; play -x -m 0 I0 c Z'].map(l => '\u0101' + l + '\r').join('') };
      },
      expect: ['cmp /ram/t.zsm tpc.zsm && echo same\nsame\n%', 'cmp /ram/s.zsm spc.zsm && echo same\nsame\n%',
        'play bad.mml; echo $status\nplay: bad.mml: channel 0: a note before an instrument\nchannel 0: a note before an ins\n%',
        'play bad2.mml\nplay: bad2.mml: what is this line?\n%', 'echo played\nplayed\n%', 'echo lines\nlines\n%',
        'play -c 0 c d e f g a b c d\nusage: play [-l] song [n]; play -o score.mml song.zsm; play [-lx] -m|-c ch mml\nplay: c: more notes than channels\n%',
        'play -x -m 0 I0 c Z\nplay: I0: channel 0: t is a tempo, before any notes\nplay: I0: channel 0: what is Z\n%'],
      // (The lines' key-ons, the last 11, each with its channel's key code then: -m's C4 D4 E4 an eighth apart (50
      // ticks at 120), -c's C4 E4 G4 on 0-2 at once, -x's C D E an eighth apart at 240 (25 ticks), then after S0 a
      // C keyed, the D after it not (legato), and K's E)
      lineKeys: [[0, 0x3E, 0], [0, 0x41, 50], [0, 0x44, 100], [0, 0x3E], [1, 0x44], [2, 0x48], [1, 0x3E, 0], [1, 0x41, 25], [1, 0x44, 50], [1, 0x3E, 75], [1, 0x44, 125]],
      // (The two plays' writes: each from the reset before it (its $14, then $01-$FF and the channels' $20s), the
      // song's after it; the same registers and values in the same order, each one's time from the song's first
      // within 4 ticks of the other's)
      check(m) {
        const w = m.ym.writes, starts = [];
        for (let i = 0; i < w.length; i++) if (w[i][1] === 0x14 && w[i][2] === 0x30 && w[i + 1] && w[i + 1][1] === 0x01) starts.push(i);
        if (starts.length < 2) return ['the resets before the two plays: ' + starts.length + ' found'];
        const RESET = 256 + 8, s0 = starts[starts.length - 2], s1 = starts[starts.length - 1];
        const a = w.slice(s0 + RESET, s1), b = w.slice(s1 + RESET, s1 + RESET + a.length), tick = 3579545 * (JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')).clock || 1) / 200;
        if (!a.length || a.length !== b.length) return ['the ZSM played ' + a.length + ' writes, the score ' + b.length];
        let worst = 0;
        for (let i = 0; i < a.length; i++) {
          if (a[i][1] !== b[i][1] || a[i][2] !== b[i][2]) return ['write ' + i + ': the ZSM\'s $' + a[i][1].toString(16) + ' = ' + a[i][2] + ', the score\'s $' + b[i][1].toString(16) + ' = ' + b[i][2]];
          worst = Math.max(worst, Math.abs((b[i][0] - b[0][0]) - (a[i][0] - a[0][0])) / tick);
        }
        this.notes = ['scom: ' + a.length + ' writes, the same both ways; their times within ' + worst.toFixed(2) + ' ticks of each other'];
        if (worst > 4) return ['the score\'s writes ' + worst.toFixed(2) + ' ticks from the ZSM\'s'];
        const kc = [], ons = [];
        for (const [t, r, v] of w) { if (r >= 0x28 && r < 0x30) kc[r & 7] = v; if (r === 0x08 && (v & 0x78)) ons.push([v & 7, kc[v & 7], t]); }
        const last = ons.slice(-this.lineKeys.length), f = [];
        let at0 = 0;
        this.lineKeys.forEach(([ch, code, ticks], i) => {
          const [c, k, t] = last[i] || [];
          if (ticks === 0) at0 = t;
          if (c !== ch || k !== code) f.push('the lines\' key-on ' + i + ': channel ' + c + ', key code $' + (k || 0).toString(16) + ' (not ' + ch + ', $' + code.toString(16) + ')');
          else if (ticks !== undefined && Math.abs((t - at0) / tick - ticks) > 1) f.push('the lines\' key-on ' + i + ': at tick ' + ((t - at0) / tick).toFixed(1) + ', not ' + ticks);
        });
        return f;
      },
    },
    {
      name: 'play', what: 'the song player: its errors; a song timed (its key-ons against its stream), its channels claimed and given back; scom; jukebox',
      init: 't_rc', cycles: 150e6,
      get machine() { return { input: PLAY_LINES.map(l => '\u0101' + l[0] + '\r').join(''), sd: playCard() }; },
      get expect() { return PLAY_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')); },
      // (The card's song's key-ons from its 20th song tick to its 60th key-on, each against its song tick: they keep
      // time to within two system ticks, so the tempo neither drifts nor jitters more.  The first ticks are left out:
      // tick 0 sets six voices up, hundreds of writes, and its key-ons go out late)
      check(m) {
        const f = [], mult = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const song = ZSM_KEYONS(PLAY_SONG());
        const cyc = m.ym.keyOns.map(k => +k.match(/at cycle (\d+)/)[1]), N = 60, K0 = song.ticks.findIndex(t => t >= 20);
        const perTick = 3579545 * mult / song.rate, slack = 2 * 3579545 * mult / 200;
        if (cyc.length < N) return ['the song: ' + cyc.length + ' key-ons (' + N + ' wanted)'];
        const e = [];
        for (let k = K0; k < N; k++) e.push(cyc[k] - song.ticks[k] * perTick);
        const spread = Math.max(...e) - Math.min(...e), span = (song.ticks[N - 1] - song.ticks[K0]) * perTick;
        this.notes = ['the song (60 Hz) timed: key-ons ' + K0 + '-' + (N - 1) + ' over ' + Math.round(span) + ' cycles, off their times by ' +
          Math.round(spread) + ' cycles at most from each other (at most ' + Math.round(slack) + ': two system ticks)'];
        if (spread > slack) f.push('the song: its key-ons ' + Math.round(spread) + ' cycles apart from their times (two system ticks: ' + Math.round(slack) + ')');
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        return f;
      },
    },
    {
      name: 'gpio', what: 'GPIO (#g) and I2C (#i): CA1\'s edges (its own line), pins, the port, ctl; the I2C bus, a memory written and read',
      init: 't_rc', cycles: 120e6,
      get machine() {
        return { input: GPIO_LINES.map(l => '\u0101' + l[0] + '\r').join(''), gpioIn: 0xA5, ca1: [30e6, 40e6], i2c: { 0x50: 256, 0x68: 16 } };
      },
      get expect() { return GPIO_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')); },
      check(m) {
        const f = [], mem = Buffer.from(m.i2c.devices.get(0x50).mem.subarray(0, 6)).toString('latin1');
        if (mem !== 'hello\n') f.push('the I2C memory at $50: ' + JSON.stringify(mem) + ', not "hello\\n"');
        if (m.via.ier & 0x02) f.push('CA1\'s interrupt on with /dev/gpio/ca1 closed');
        if ((m.via.r[0x0C] & 0x0F) !== 0x0F) f.push('PCR: $' + m.via.r[0x0C].toString(16) + ' (CA1 rising, CA2 high wanted)');
        this.notes = ['the I2C bus: ' + m.i2c.stats.starts + ' starts, ' + m.i2c.stats.taken + ' bytes taken, ' + m.i2c.stats.given + ' given'];
        return f;
      },
    },
    {
      name: 'clock', what: 'the clock: from a DS1747 at its start, /dev/time and date, the calendar (month ends, leap years), the chip set, stamps',
      init: 't_rc', cycles: 80e6,
      get machine() {
        return { input: CLOCK_LINES.map(l => '\u0101' + l[0] + '\r').join(''), rtc: Date.UTC(2026, 9, 3, 15, 4, 5) / 1000 };
      },
      get expect() {
        return [...CLOCK_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')),
          ' 2024-02-29 12:00 /ram/f\n%'];
      },
      // (The chip as the last time set it: 2024-02-29 12:00, a Thursday: day 5 from Sunday)
      check(m) {
        const r = m.rtc.regs(), f = [], hex = a => a.map(b => b.toString(16).padStart(2, '0')).join(' ');
        if ((r[0] & 0x3F) !== 0x20 || r[7] !== 0x24 || (r[6] & 0x1F) !== 0x02 || (r[5] & 0x3F) !== 0x29 || (r[3] & 0x3F) !== 0x12 ||
          (r[4] & 7) !== 5 || (r[1] & 0x80)) f.push('the DS1747: ' + hex(r) + ' (2024-02-29 12:00, day 5, running wanted)');
        return f;
      },
    },
    {
      name: 'pc', what: '/pc (#P, the console driver\'s): a folder on the PC through the serial port, its frames between the console\'s bytes; files read, made, copied, renamed, removed; a program run from it',
      init: 't_rc', cycles: 250e6, pc: { files: PC_FILES },
      get machine() { return { input: typed(PC_LINES) }; },
      get expect() { return expected(PC_LINES); },
      check(m) {
        const f = pcReport(m, 1, 0, 0), at = n => path.join(m.pc.dir, n);
        if (fs.readFileSync(at('new.txt'), 'latin1') !== 'hi\n') f.push('new.txt on the PC: ' + JSON.stringify(fs.readFileSync(at('new.txt'), 'latin1')));
        if (!fs.readFileSync(at('rr')).equals(fs.readFileSync(path.join(__dirname, '..', 'romfs', 'README')))) f.push('rr on the PC isn\'t /rom/README');
        for (const n of ['d', 'r', 'sub/x']) if (fs.existsSync(at(n))) f.push(n + ' is still on the PC');
        return f;
      },
    },
    {
      name: 'pc-img', what: '/pc a HydraFS image file on the PC (the PC tool\'s folder can be an image: sim/tools/pcfs.js, hydrafs.js\'s Volume): its files read, a program run from it; files made, written, added to, made again shorter, copied (17K: several clusters), renamed, removed, a directory made and removed; its errors; then the image on the PC: the files as the Hydra left them, and its free map right (hydrafs.js check).  And the Volume\'s own: a length set, a write into a hole, a rename, the read-only bit, its errors\' codes',
      init: 't_rc', cycles: 400e6,
      pc: { image: { mb: 1, label: 'PCIMG' }, files: () => Object.assign({ big: Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]) }, PC_FILES()) },
      get machine() { return { input: typed(PC_IMG_LINES) }; },
      get expect() { return expected(PC_IMG_LINES); },
      check(m) {
        const f = pcReport(m, 1, 0, 0), v = new hydrafs.Volume(m.pc.image, { readOnly: true });
        try {
          const bytes = n => { const e = v.tryWalk(n); return e && !e.isDir ? v.read(e) : Buffer.from('(no such file)'); };
          const text = n => bytes(n).toString('latin1'), big = Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]);
          if (text('new.txt') !== 'hi\n') f.push('new.txt in the image: ' + JSON.stringify(text('new.txt')));
          if (text('copy') !== 'shorter\n') f.push('copy in the image: ' + JSON.stringify(text('copy').slice(0, 40)));
          if (text('hello.txt') !== 'Hello from the PC\nline two\nmore\n') f.push('hello.txt in the image: ' + JSON.stringify(text('hello.txt')));
          if (!bytes('big').equals(big)) f.push('big in the image isn\'t as it was');
          if (!bytes('rr').equals(fs.readFileSync(path.join(__dirname, '..', 'romfs', 'README')))) f.push('rr in the image isn\'t /rom/README');
          for (const n of ['d', 'r', 'sub/x']) if (v.tryWalk(n)) f.push(n + ' is still in the image');
          const problems = v.check();
          if (problems.length) f.push('the image\'s free map: ' + problems.slice(0, 3).join('; '));
          this.notes = [v.freeCount + ' of ' + v.clusters + ' clusters free'];
        } finally { v.close(); }
        f.push(...hfsVolumeChecks(path.join(path.dirname(m.pc.image), 'pc-img-vol')));
        return f;
      },
    },
    {
      name: 'pc-two', what: '/pc from two tasks at once (a pipeline: one reads a file, the other writes its copy): one request at a time, the other waiting its turn',
      init: 't_rc', cycles: 400e6, pc: { files: { big: PC_BIG } },
      machine: { input: 'ācat /pc/big | cat >/pc/copy; cmp /pc/big /pc/copy; echo $status\r' },
      expect: ['% cat /pc/big | cat >/pc/copy; cmp /pc/big /pc/copy; echo $status\n\n%'],
      check(m) {
        const f = pcReport(m, 1, 0, 0);
        if (!fs.readFileSync(path.join(m.pc.dir, 'copy')).equals(PC_BIG())) f.push('the copy on the PC isn\'t big');
        return f;
      },
    },
    {
      name: 'pc-fast', what: '/pc at 115200: a file copied on the PC through the Hydra and compared, no reply\'s byte lost (none asked for again: the ACIA\'s interrupt goes ahead of the tick\'s and timer 2\'s)',
      // (17K: some 530 requests.  Before, about one in 22 lost a byte and went again)
      init: 't_rc', cycles: 300e6, pc: { files: { big: () => Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]) } },
      machine: { input: 'āecho b115200 >/dev/serctl\r' + 'ācp /pc/big /pc/copy; cmp /pc/big /pc/copy; echo $status\r' },
      expect: ['% cp /pc/big /pc/copy; cmp /pc/big /pc/copy; echo $status\n\n%'],
      check(m) {
        const f = pcReport(m, 1, 0, 0), big = Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]);
        if (m.acia.pcLost) f.push(m.acia.pcLost + ' reply byte(s) lost (they came while the last was unread)');
        if (!fs.readFileSync(path.join(m.pc.dir, 'copy')).equals(big)) f.push('the copy on the PC isn\'t big');
        return f;
      },
    },
    {
      name: 'pc-song', what: 'a song played from /pc (play /pc/t.zsm 2), its loop twice more, in time: a read on the line doesn\'t hold a note up',
      init: 't_rc', cycles: 120e6, pc: { files: { 't.zsm': PC_SONG } },
      machine: { input: 'āplay /pc/t.zsm 2; echo $status\r' },
      expect: ['% play /pc/t.zsm 2; echo $status\n\n%'],
      check(m) {
        const f = pcReport(m, 1, 0, 0), mult = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const on = m.ym.keyOns.map(k => +k.match(/at cycle (\d+)/)[1]), want = 0.6 * 3579545 * mult, slack = 2 * 3579545 * mult / 200;
        if (on.length !== 4) return [...f, 'key-ons: ' + on.length + ', not 4'];
        for (let k = 1; k < 4; k++) if (Math.abs(on[k] - on[k - 1] - want) > slack)
          f.push('key-on ' + k + ' came ' + (on[k] - on[k - 1]) + ' cycles after the last, not 0.6 s (' + Math.round(want) + ', give or take two system ticks)');
        return f;
      },
    },
    {
      name: 'pc-ro', what: '/pc served read-only (the PC tool\'s --read-only): files read; a write, a create, a remove, a mkdir, a rename refused; the folder as it was',
      init: 't_rc', cycles: 120e6, pc: { files: { 'hello.txt': 'Hello\n' }, readOnly: true },
      get machine() { return { input: typed(PC_RO_LINES) }; },
      get expect() { return expected(PC_RO_LINES); },
      check(m) {
        const f = pcReport(m, 1, 0, 0);
        if (fs.readdirSync(m.pc.dir).join() !== 'hello.txt' || fs.readFileSync(path.join(m.pc.dir, 'hello.txt'), 'latin1') !== 'Hello\n') f.push('the folder changed: ' + fs.readdirSync(m.pc.dir).join());
        return f;
      },
    },
    {
      name: 'pc-none', what: '/pc with no PC tool: the attach (its bytes on the terminal) unanswered, an error a second on, each time; the console goes on',
      init: 't_rc', cycles: 80e6,
      machine: { input: 'āls /pc\rācat /pc/x\rāecho still here\r' },
      expect: ['% ls /pc\n\x1eA', 'ls: /pc: i/o error\n%', '% cat /pc/x\n\x1eA', 'cat: /pc/x: i/o error\n%', '% echo still here\nstill here\n%'],
    },
    {
      name: 'pc-damage', what: '/pc\'s frames damaged on the line: requests (the PC tool asks for them again: its NAK), a reply (the Hydra asks again, and the PC tool answers from its last reply); the answers right, nothing damaged shown; serctl counts them (2 nak, 1 damaged)',
      init: 't_rc', cycles: 140e6, pc: { files: { 'hello.txt': 'Hello from the PC\n' }, damage: ['q3', 'r4', 'q6'] },
      machine: { input: 'āls /pc\rācat /pc/hello.txt\rācat /pc/hello.txt\rācat /dev/serctl\r' },
      expect: ['% ls /pc\nhello.txt\n%', '% cat /pc/hello.txt\nHello from the PC\n% cat /pc/hello.txt\nHello from the PC\n%', ' req, 0 cut short, 0 unanswered, 1 damaged, 2 nak, 0 gone\nline 0 overruns, 0 framing errors, 0 dropped\n%'],
      check: m => pcReport(m, 1, 2, 1),
    },
    {
      name: 'xmodem', what: 'xmodem: a file received (1K blocks, a CRC; one damaged, one sent twice) and sent back (128-byte blocks, a checksum, one NAKed; 1K ones), the same; Ctrl-C at its start; the PC cancelling; 115200',
      init: 't_rc', cycles: 200e6,
      // (The emulator the PC's end: sim/lib/xmpeer.js.  The -s sessions' start, NAK or C, typed as a key.  At 115200,
      // 1K blocks received: one comes faster than it can be taken, what's behind waiting in the console's receive ring)
      get machine() {
        this.peer = createXmodemPeer([
          { trigger: 'xmodem -r /ram/x\r\n', role: 'send', data: XM_DATA(), k: true, damage: [2], again: [1] },
          { trigger: 'xmodem -s /ram/x\r\n', role: 'receive', crc: false, nak: [3] },
          { trigger: 'xmodem -s -k /ram/x\r\n', role: 'receive', crc: true },
          { trigger: 'xmodem -r /ram/y\r\n', role: 'send', data: XM_DATA(), cancel: 2 },
          { trigger: 'xmodem -r /ram/w\r\n', role: 'send', data: XM_DATA(), k: true },
          { trigger: 'xmodem -s -k /ram/w\r\n', role: 'receive', crc: true },
        ]);
        // (flow 'none': the PC's end takes every byte, XON and XOFF too, as a terminal does in a binary transfer)
        return { pcHost: this.peer, flow: 'none', input: 'āxmodem -r /ram/x\r' + 'āxmodem -s /ram/x\rĀ\x15' + 'āxmodem -s -k /ram/x\rĀC' +
          'āxmodem -r /ram/z\rĀ\x03' + 'āxmodem -r /ram/y\r' + 'āecho $status; xmodem /ram/x\r' +
          'āecho b115200 >/dev/serctl; xmodem -r /ram/w\r' + 'āxmodem -s -k /ram/w\rĀC' };
      },
      expect: ['% xmodem -r /ram/x\n/ram/x: 3000 bytes\n%', '% xmodem -s /ram/x\n/ram/x: 3000 bytes\n%', '% xmodem -s -k /ram/x\n/ram/x: 3000 bytes\n%',
        'xmodem: /ram/z: cancelled\n%', '% xmodem -r /ram/y\nxmodem: /ram/y: cancelled\n%',
        '% echo $status; xmodem /ram/x\ncancelled\nusage: xmodem -r file | -s [-k] file\n%',
        '% echo b115200 >/dev/serctl; xmodem -r /ram/w\n/ram/w: 3000 bytes\n%', '% xmodem -s -k /ram/w\n/ram/w: 3000 bytes\n%'],
      // (Each session's data back as the file, and SUBs to its blocks' end: 128-byte blocks, and 1K ones)
      check() {
        const f = [], [rx, tx, tk, can, rx115, tk115] = this.peer.sessions, data = XM_DATA();
        this.notes = this.peer.sessions.map((s, i) => 'session ' + (i + 1) + ': ' + (s.log || ['never armed']).join(', '));
        if (rx.done !== true || !rx.log.includes('damaged 2') || !rx.log.includes('again 1')) f.push('receiving: ' + (rx.log || []).join(', '));
        if (tx.done !== true || !tx.log.includes('nak 3')) f.push('sending back: ' + (tx.log || []).join(', '));
        if (tk.done !== true || tk.log.filter(l => / 1K$/.test(l)).length !== 3) f.push('sending back, 1K: ' + (tk.log || []).join(', '));
        if (rx115.done !== true) f.push('receiving at 115200: ' + (rx115.log || []).join(', '));
        for (const [s, block] of [[tx, 128], [tk, 1024], [tk115, 1024]]) {
          const back = Buffer.from(s.got || []), pad = Math.ceil(data.length / block) * block;
          if (back.length !== pad || !back.subarray(0, data.length).equals(data) || back.subarray(data.length).some(b => b !== 0x1A))
            f.push('sent back in ' + block + '-byte blocks: ' + back.length + ' bytes, not the file (' + data.length + ') and SUBs to ' + pad);
        }
        if (can.done !== 'cancelled') f.push('the PC\'s cancel: ' + (can.log || []).join(', '));
        return f;
      },
    },
    {
      name: 'cons', what: 'the console: lines, editing, history, raw keys, its answers (DA, CPR, DECRQM, the size, DECREPTPARM), the window\'s size (the terminal\'s report typed, terminal size, KEY_RESIZE, the terminal asked), keys vt (DECCKM, VT52 mode), Ctrl-C, windows (shown, repainted, made, gone), groups (Ctrl-] c\'s, new\'s, new group\'s; Ctrl-] Tab, Ctrl-Tab, Ctrl-Shift-Tab, Ctrl-] n and p; KEY_FOCUS), 115200, the bell',
      init: 't_cons', modules: ['t_child'], cycles: 160e6,
      // (ā: wait for a prompt, "N> ")
      machine: { input: 'āhello\r' + 'āabX\x08c\r' + 'āac\x1b[Db\r' + 'ābc\x1b[Ha\x1b[Fd\r' +
        'āxyz\x15ok\r' + 'āabXc\x1b[D\x1b[D\x1b[3~\r' + 'ā\x1b[A\x1b[A\r' + 'ā\x04' + 'āparts\r' +
        'āx\x1b[A' + 'ā\x1b[8;40;100t' + 'ā\x1b[A\x1b[A\x1b[15~\x1b[A' + 'ā\x03' +
        'ā\x1d1z\r\x1d0' + 'ā\x1d1\x03\x1d0' + 'ā\x1dc' + 'ā\x1d\t' + 'ā\x1b[9;5u' + 'ā\x1dn' + 'ā\x1b[27;6;9~' + 'ā\x1dp' },
      check(m, out) {
        const f = [], a = m.acia, want = a.wdc ? 1 : 2;
        if (!out.includes('\x1b[2J') || !out.includes('w1 hidden text')) f.push('window 1 shown: no repaint of its text');
        if (out.split('\x1b[18t').length < 3) f.push('the terminal not asked its size twice (ESC [ 1 8 t: as the console starts, and terminal size)');
        this.notes = ['at 115200, the shortest idle time between characters sent: ' + a.gapMin.toFixed(2) + ' bits (at least ' + want + ')'];
        if (!(a.gapMin >= want - 0.05)) f.push('at 115200, characters ' + a.gapMin.toFixed(2) + ' bits apart: less than ' + want);
        if (a.overruns) f.push(a.overruns + ' bytes written to the ACIA while it was still sending');
        if (!m.ym.keyOns.some(k => k.startsWith('ch 7 '))) f.push('a BEL printed: no bell (no key-on on channel 7)');
        return f;
      },
    },
    {
      name: 'numbers', what: 'the numbers library (modules/numbers): its calls as a card\'s file has them (tests/numtest.js), each checked against the reference (sim/tools/numfmt.js), and to keep the caller\'s bank, zero page and r0-r3',
      init: 't_num', cycles: 3000e6,
      get machine() { this.calls = numtest.calls(); this.file = numtest.card(this.calls); this.sd = [imageCard(0, this.file, 16384)]; return { sd: this.sd }; },
      check() {
        this.sd[0].save();
        const v = new hydrafs.Volume(this.file), out = v.read(v.walk('num.out'));
        v.close();
        this.notes = [this.calls.length + ' calls'];
        return numtest.check(this.calls, out);
      },
    },
    {
      name: 'step', what: 'the debugger\'s steps (TASKSTEP, /proc/N/ctl): a program started stopped (SPAWN_STOPPED), each kind of instruction a step at a time (out of line, or on its frame), a JSR stepped over, a breakpoint, a program\'s own BRK; refused steps',
      init: 't_step', modules: ['t_child'], cycles: 60e6,
      get machine() { return { sd: stepCard() }; },
    },
    {
      name: 'db', what: 'the debugger at rc (/rom/bin/db): the SDK\'s hi started stopped, its labels from /pc (ld65\'s) and hydra.inc\'s calls and registers, registers, steps, a disassembly (as as writes it: the asm library\'s, symbols in the operands), a breakpoint hit twice, a JSR to the kernel stepped over, until, memory read and written, and on to its end',
      init: 't_rc', cycles: 300e6,
      pc: { files: () => ({ 'hi.lbl': fs.readFileSync(path.join(__dirname, '..', 'obj', 'samples', 'hi', 'hi.lbl')) }) },
      machine: {
        input: ['db /sd/0/sample/hi Ann Bob', 'l /pc/hi.lbl', 'l /lib/as/hydra.inc', 'r', 's 3', 'd main 6', 'b main+14', 'b', 'c', 'n 4', 'u main+2C', 'c',
          'm s_you 4', 'w s_you 59 4F 55', 'm s_you 4', 'x', 'c', 'echo $status'].map(l => 'ā' + l + '\r').join(''),
      },
      // (Its registers as the loader left them aren't checked: A, X and Y at its entry point)
      expect: ['% db /sd/0/sample/hi Ann Bob\ntask ', '\n0830  A5 02     lda $02\ndb> l /pc/hi.lbl\n8 symbols\ndb> l /lib/as/hydra.inc\n',
        ' symbols\ndb> r\nPC=0830 ', '\n0830  A5 02     lda r0                   ; main\n',
        'db> s 3\n0832  85 22     sta arg                  ; main+2\n0834  A5 03     lda r0+1                 ; main+4\nPC=0836 A=03 ',
        '0838  B2 22     lda (arg)                ; main+8\n083A  D0 08     bne main+14              ; main+A\ndb> b main+14\n',
        'db> b\n1 0844  A9 52     lda #$52                 ; main+14\ndb> c\nbreakpoint 1\nPC=0844 ',
        '084C  20 53 F9  jsr PUTS                 ; main+1C\ndb> u main+2C\nHello, PC=085C ', 'db> c\nAnn!\nbreakpoint 1\nPC=0844 A=42 ',
        'db> m s_you 4\n0934  79 6F 75 00              you.\ndb> w s_you 59 4F 55\ndb> m s_you 4\n0934  59 4F 55 00              YOU.\n',
        'db> x\ndb> c\nHello, Bob!\nI\'m task ', ' ended: code 0\n% echo $status\n\n% '],
      sdcard: true
    },
    {
      name: 'typeahead', what: 'keys typed ahead of a program that reads nothing (three lines, 190 keys, while sleep runs; then again while ls writes): none dropped at the window\'s queue of 63 (the serial port\'s keys held in the receive ring till the reader has taken some), each line read whole in its turn',
      init: 't_rc', cycles: 200e6,
      machine: {
        input: 'āsleep 1\r' + 'echo ' + '1234567890'.repeat(7) + '\recho ' + 'abcdefghij'.repeat(7) + '\recho after one\r' +
          'āls /rom/bin; sleep 1\r' + 'echo ' + '0987654321'.repeat(7) + '\recho ' + 'jihgfedcba'.repeat(7) + '\recho after two\r',
      },
      expect: ['% sleep 1\n% echo ' + '1234567890'.repeat(7) + '\n' + '1234567890'.repeat(7) + '\n% echo ' + 'abcdefghij'.repeat(7) + '\n' + 'abcdefghij'.repeat(7) + '\n% echo after one\nafter one\n% ',
        '\n% echo ' + '0987654321'.repeat(7) + '\n' + '0987654321'.repeat(7) + '\n% echo ' + 'jihgfedcba'.repeat(7) + '\n' + 'jihgfedcba'.repeat(7) + '\n% echo after two\nafter two\n% '],
    },
    {
      name: 'kcopy2', what: 'two copies between tasks at once, with the kernel task the far end of both: a loop reads /dev/kmesg (kdev copies from the kernel task\'s memory) while the shell starts thirty programs (SPAWN\'s path and arguments: the kernel task\'s own copies); a copy holds the CPU till it\'s done, so neither finds the other\'s partner at its far end (kdev did, switched out between two bursts: it went on in another task\'s stack, and ended with a BRK\'s note; init and the shells after it)',
      init: 't_rc', cycles: 600e6,
      machine: {
        input: ['rc -c \'while() cat /dev/kmesg >/dev/null\' &', 'for(i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30) calc $i + 100 >/dev/null',
          'cat /dev/kmesg | head -2', 'echo still here'].map(l => 'ā' + l + '\r').join(''),
      },
      expect: ['% cat /dev/kmesg | head -2\n\nHydra-16: kernel ', '% echo still here\nstill here\n% '],
    },
    {
      name: 'pasteahead', what: 'with no flow control (serctl\'s x0), seven lines (399 keys) pasted at the line\'s rate while ps writes four times and nothing reads: more than the window\'s queue and the receive ring hold (63 + 255); the newest are lost (the irq entry\'s, with the ring full: serctl counts them, dropped), and what came first is whole: the first five lines read and run in their turn',
      init: 't_rc', cycles: 260e6,
      machine: {
        paste: true,
        input: 'āecho x0 >/dev/serctl; cat /dev/serctl\r' + 'āps; ps; ps; ps; sleep 1\r' + [1, 2, 3, 4, 5, 6, 7].map(i => 'echo ' + i + ' ' + 'abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\r').join('') +
          'Ā'.repeat(14) + '\r' + 'ĀĀ' + 'grep dropped /dev/serctl; echo counted\r',   // (Once the lines kept have run: what's left of the line cut short, run; then serctl's count of the lost)
      },
      check(m, out) { return /\nline 0 overruns, 0 framing errors, [1-9]\d* dropped\n/.test(out) ? [] : ['serctl doesn\'t count the bytes dropped with the ring full: ' + JSON.stringify((out.match(/\nline .*\n/g) || []).pop())]; },
      expect: ['\ncounted\n', '% echo x0 >/dev/serctl; cat /dev/serctl\nb9600\nx0\npc 0 req, 0 cut short, 0 unanswered, 0 damaged, 0 nak, 0 gone\nline 0 overruns, 0 framing errors, 0 dropped\n% ',
        '\n% ' + [1, 2, 3, 4, 5].map(i => 'echo ' + i + ' abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\n' + i + ' abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\n% ').join('')],
    },
    {
      name: 'flow', what: 'flow control (XON/XOFF: serctl\'s x1, as the console starts): sixty lines (3,400 keys) pasted at the line\'s rate while ps writes and nothing reads, ten times what the queue and the ring hold: the console tells the terminal to stop (XOFF, with 96 unread) and to go on (XON, at 32), and every line is read and run whole, in its turn; then a file of XONs and XOFFs copied through /pc, its frames\' bytes of them stuffed; x0 and x1 again',
      init: 't_rc', cycles: 900e6,
      pc: { files: () => ({ 'xs': Buffer.from(Array.from({ length: 600 }, (_, i) => [0x11, 0x13, 0x1E, 0x1F, 0x41 + i % 26][i % 5])) }) },
      machine: {
        paste: true,
        // (Two seconds with nothing read: less than the watch's five, after which the terminal is let go: flowlet)
        input: 'ācat /dev/serctl\r' + 'āps; ps; sleep 1\r' +
          Array.from({ length: 60 }, (_, i) => 'echo ' + (i + 1) + ' ' + 'abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\r').join('') +
          'echo pasted\r' + 'ācp /pc/xs /pc/xs2; cmp /pc/xs /pc/xs2; echo copied $status\r' + 'āecho x0 >/dev/serctl; echo x1 >/dev/serctl; cat /dev/serctl\r' + 'āecho the end\r',
      },
      expect: ['% cat /dev/serctl\nb9600\nx1\npc 0 req, 0 cut short, 0 unanswered, 0 damaged, 0 nak, 0 gone\nline 0 overruns, 0 framing errors, 0 dropped\n% ',
        '\n% ' + Array.from({ length: 60 }, (_, i) => 'echo ' + (i + 1) + ' abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\n' + (i + 1) + ' abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\n% ').join(''),
        '; echo copied $status\ncopied\n% ', '; cat /dev/serctl\nb9600\nx1\npc ', ' req, 0 cut short, 0 unanswered, 0 damaged, 0 nak, 0 gone\nline 0 overruns, 0 framing errors, 0 dropped\n% echo the end\nthe end\n% '],
      check(m) {
        const f = [];
        if (m.acia.xoffs < 5 || m.acia.xons < 5) f.push('the console told the terminal to stop ' + m.acia.xoffs + ' times, and to go on ' + m.acia.xons + ' (5 each at least, for 3,400 keys)');
        if (String(m.pc.host.attach) !== '2,1') f.push('/pc\'s attach: ' + m.pc.host.attach + ' (2, the version, and 1: its frames stuff XON and XOFF)');
        if (m.acia.rxLost) f.push(m.acia.rxLost + ' bytes came while the last was unread');
        this.notes = ['XOFF ' + m.acia.xoffs + ' times, XON ' + m.acia.xons];
        return f;
      },
    },
    {
      name: 'flowlet', what: 'flow control\'s watch: eight lines (456 keys) pasted while sleep 30 runs and nothing reads; the console tells the terminal to stop (XOFF), and the terminal holds the rest, and the Ctrl-C typed after them; five seconds on the console lets it go (XON), though nothing has been read: the rest comes (the ring fills, the newest lost), then the Ctrl-C, which ends sleep; the first five lines are read whole.  (HydraOS as it boots: wstart\'s waiting read brings the driver back to look at the time)',
      init: 'init', cycles: 80e6,
      machine: {
        paste: true,
        input: 'āsleep 30\r' + [1, 2, 3, 4, 5, 6, 7, 8].map(i => 'echo ' + i + ' ' + 'abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\r').join('') + '\x03' + 'ĀĀĀ' + '\recho back\r',
      },
      expect: ['/> sleep 30\n', ...[1, 2, 3, 4, 5].map(i => '/> echo ' + i + ' abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\n' + i + ' abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLM\n'),
        '/> echo back\nback\n/> '],
      check(m) {
        const f = [];
        if (m.acia.xoffs < 1) f.push('the console never told the terminal to stop');
        if (m.acia.xons < 2) f.push('the console told the terminal to go on ' + m.acia.xons + ' times (2 at least: as it starts, and its watch)');
        this.notes = ['XOFF ' + m.acia.xoffs + ' times, XON ' + m.acia.xons];
        return f;
      },
    },
    {
      name: 'dbover', what: 'the debugger\'s steps over a subroutine (n): a breakpoint in the subroutine stepped over, then a step and a continue from it (no step of the kernel\'s is out); a subroutine that calls itself stepped over at its second level, and returned from (u); Ctrl-C in a call of the kernel\'s stepped over, then continue runs on past the call\'s end; and stopped in the next, a step is refused',
      init: 't_rc', cycles: 400e6,
      pc: { files: () => ({ 'over.s': DBOVER_S }) },
      machine: {
        input: ['as /pc/over.s /ram/over', 'db /ram/over', 'b 084E', 'n 2', 's', 'c', 'x', 'u 0852', 'n', 'u 0835', 'u 083E'].map(l => 'ā' + l + '\r').join('') +
          'ān\rĀ\x03' + 'āc\rĀĀĀĀĀ\x03' + 'ās\r' + 'āq\r' + 'āecho $status\r',
      },
      expect: ['db> n 2\n0832  20 48 08  jsr $0848\n3breakpoint 1\nPC=084E A=33 ', 'db> s\nPC=084F A=03 ', 'db> c\n2breakpoint 1\nPC=084E A=32 ',
        'db> u 0852\nPC=0852 A=01 ', 'db> n\n1PC=0855 A=00 ', 'db> u 0835\nPC=0835 A=00 ', 'db> u 083E\n!PC=083E A=90 X=01 ',
        'db> n\nPC=E', 'db> c\n.PC=E', 'db> s\nit can\'t be stepped here: in a call, or a step over still running (c runs it on)\n',
        'db> q\n\n% echo $status\n\n% '],
    },
    {
      name: 'scall', what: 'spike S3: calls into a driver\'s task, its errors, a busy driver, the round trip',
      init: 't_scall', modules: ['t_child', 't_drv'], without: ['cons', 'storage', 'snd', 'vid'], cycles: 40e6,
      budgets: [{ what: 'SCALL round trip (DBG_SCALL, less the same loop calling the code in place)', from: '<scall', to: 'scall>',
        minus: ['<base', 'base>'], per: 1000, max: 200 }],
    },
    {
      name: 'heap', what: 'hylang\'s runtime (modules/hylang/heap.inc, phase 1): values and fixnums, cells, symbols and atoms, strings; the collector (a list kept while garbage is taken back, a structure deeper than the mark stack, blobs dropped and the rest moved down); the heap growing, a million cells made and dropped with none lost, and its end (E_NOMEM)',
      init: 't_heap', cycles: 2000e6,
      budgets: [{ what: 'a cons made and put on a list (the fixnum made, cell_alloc, its words set)', from: '<cons', to: 'cons>', per: 1000, max: 550 },
        { what: 'a collection, 1000 cells live, a cell', from: '<gc1k', to: 'gc1k>', per: 1000, max: 310 },
        { what: 'a collection, 9000 cells live, a cell', from: '<gc9k', to: 'gc9k>', per: 9000, max: 255 }],
    },
    {
      name: 'hylang', what: 'hylang\'s REPL, evaluator, built-ins, numbers, strings, hashes, streams and system library (phases 3 to 7): the reader\'s every form (in Q-expressions, printed as they\'re read) and its errors, an expression over lines, 255 brackets open; lines evaluated: def, fn, fun, recursion 1,000 deep (2,500 the most: deeper, an error), a tail loop, errors, partial application, too many arguments, &_, let, the loops, output-of, try, map, format, + of strings, cmp, closures, fexprs; numbers past a fixnum, fractions, fixed decimals and complex numbers, read in bases and written in them, to-fixed, truncate, fib, random, the bits; the string built-ins; hashes (made with their values evaluated, called, a method with &0, a private entry, a locked hash, cloned, listed); read, the clock, rc\'s lines run (their output, their exit status), print-to and write-to stdout, save, the environment, a system error\'s code; filter, the folds, any?, all?, find, count, sum, product, sort (by cmp, by a function, its error), subset, index-of, gensym, to-atom, random; Ctrl-C at the prompt and in a loop; (exit 3); stdin a pipe, its end; hylang -g (a collection before every allocation)',
      init: 't_rc', cycles: 1500e6,
      // (Each line typed at a prompt: hylang> and, for more lines, the closers it wants then " <")
      get machine() {
        const lines = HYLANG_LINES.map(l => l[0]);
        return { input: '\u0101hylang\r' + lines.map(l => '\u0101' + l + '\r').join('') +
          '\u0101(list 1\r\u0101\x03' + '\u0101(inf)\r\u0100\x03' + '\u0101(exit 3)\r' + '\u0101echo $status\r' +
          '\u0101echo \'(1 2) $x {a\' | hylang; echo \'(* 6 7)\' | hylang; echo $status\r' +
          '\u0101hylang -g\r' + HYLANG_G.map(l => '\u0101' + l[0] + '\r').join('') + '\u0101(list 1 (list 2 (list 3)) {5\r\u0101"""6\r\u01017"""})\r' +
          '\u0101exit\r' + '\u0101echo $status\r' };
      },
      expect: ['hylang (danlang on the Hydra-16)\nType \'exit\' to Exit\n\n',
        HYLANG_LINES.map(l => (l[2] || 'hylang> ') + l[0] + '\n' + (l[1] === undefined ? '' : '=> ' + l[1] + '\n')).join(''),
        'hylang> (list 1\n\t) <\nhylang> (inf)\n=> Error: interrupted\nhylang> (exit 3)\n\n% echo $status\n3\n%',
        'hylang> \t} <=> Error: missing }\n', 'hylang> => 42\nhylang> => exit\n\n%',
        HYLANG_G.map(l => 'hylang> ' + l[0] + '\n=> ' + l[1] + '\n').join('') +
          'hylang> (list 1 (list 2 (list 3)) {5\n\t)} <"""6\n\t)}""" <7"""})\n=> {1 {2 {3}} {5 "6\\n7"}}\n' +
          'hylang> exit\n=> exit\n% echo $status\n\n%'],
    },
    ...HYSUITE_PARTS.map((p, i) => ({
      name: 'hysuite' + (i + 1), what: 'hylang\'s suite, part ' + (i + 1) + ' of ' + HYSUITE_PARTS.length + ': danlang\'s run.dl with ' +
        p.files.map(f => f + '.dl').join(', ') + (p.about ? ' (' + p.about + ')' : ''),
      init: 't_rc', cycles: 3000e6,
      get machine() { return { sd: hylangCard(this.name, hysuiteFiles(p)), rtc: Date.UTC(2026, 9, 5, 12, 0, 0) / 1000, input: '\u0101cd /sd/0; hylang part.dl; echo status $status\r' }; },
      expect: [(p.checks === undefined ? '' : p.checks) + ' checks, 0 failed\nstatus\n%'],
    })),
    {
      name: 'hytext', what: 'hylang without its snapshot (a ROM without the module hysnap): its library loaded as text as it starts (/lib/hylang/globals.hl, the ROM disk\'s), the same banner, the library\'s definitions there; a tail loop of 50,000 steps',
      init: 't_rc', without: ['hysnap'], cycles: 1200e6,
      get machine() {
        return { input: '\u0101hylang\r' + HYTEXT_LINES.map(l => '\u0101' + l[0] + '\r').join('') + '\u0101exit\r' };
      },
      expect: ['hylang (danlang on the Hydra-16)\nType \'exit\' to Exit\n\n' +
        HYTEXT_LINES.map(l => 'hylang> ' + l[0] + '\n=> ' + l[1] + '\n').join('') + 'hylang> exit\n=> exit\n%'],
    },
    {
      name: 'hyspeed', what: 'hylang\'s budgets (phase 8\'s, at 3.58 MHz, its library loaded): a parameter looked up, a call of a function of two arguments, a tail loop\'s step (if, zero?, -, the call), map with a function of one argument, an item; each the difference of two lines\' times, from the echo to the value',
      init: 't_rc', cycles: 200e6,
      get machine() {
        return { input: 'āhylang\r' + [...HYBUDGET_SETUP, ...HYBUDGET_LINES.map(l => l[0])].map(l => 'ā' + l + '\r').join('') + 'āexit\r' };
      },
      expect: [HYBUDGET_LINES.map(l => 'hylang> ' + l[0] + '\n=> ' + l[1] + '\n').join('') + 'hylang> exit\n=> exit\n%'],
      budgets: [hyBudget('hylang, a parameter looked up', 5, 6, 5000, 300), hyBudget('hylang, a call of a function of two arguments', 3, 4, 500, 4000),
        hyBudget('hylang, a tail loop\'s step', 0, 1, 1000, 6500), hyBudget('hylang, map with a function of one argument, an item', 8, 9, 990, 4000)],
    },
    {
      name: 'hyhydra', what: 'hylang\'s Hydra built-ins and system calls (the plan\'s phases 9 and 10): hydra.hl as a script (sysinfo, mods, errstr; ps, task-info, yield, sleep-until; peek and poke, the task\'s banks, a shared segment, free; bind, mount, unmount, ns, newns; note, on-note; hold; key?; sys- functions of each group of calls, and their errors), and again with a collection before every allocation (sys- names bound, the calls\' values made, puts and putc); at the prompt, raw keys (key: a character, the terminal\'s up key; key?) and Ctrl-C given to on-note\'s function',
      init: 't_rc', cycles: 2400e6,
      get machine() {
        return { sd: hylangCard(this.name), input: '\u0101cd /sd/0; hylang hydra.hl; echo status $status\r' +
          '\u0101hylang -g\r' + HYHYDRA_G.map(l => '\u0101' + l[0] + '\r').join('') + '\u0101exit\r' +
          '\u0101hylang\r' + '\u0101(key)\r\u0100q' + '\u0101(key)\r\u0100\x1b[A' + '\u0101(list (key?) (key))\r\u0100z' +
          '\u0101(on-note (fn {n} {do (print n) T}))\r' + '\u0101(fun {hy-loop n} {if (zero? n) :done (hy-loop (- n 1))})\r' +
          '\u0101(hy-loop 30000)\r\u0100\x03' + '\u0101exit\r' };
      },
      expect: ['127 checks, 0 failed\nstatus\n%', HYHYDRA_G.map(l => 'hylang> ' + l[0] + '\n' + (l[2] || '') + '=> ' + l[1] + '\n').join('') + 'hylang> exit\n=> exit\n%',
        'hylang> (key)\n=> \\q\nhylang> (key)\n=> :up\nhylang> (list (key?) (key))\n=> {NIL \\z}\n',
        'hylang> (hy-loop 30000)\n:interrupt\n=> :done\nhylang> exit\n=> exit\n\n%'],          // (rc had the Ctrl-C too: a new line)
    },
    {
      name: 'hysh', what: 'hylang as the shell (the plan\'s phase 12: hylang -l, login.hl, profile.hl, shell.hl): the rc test\'s lines that stand alone, each an rc line at hylang\'s prompt (rc -c), as at rc\'s; hylang\'s lines by their first character; cd and the prompt; $status and status; bind and unmount in hylang\'s namespace; a usage; & and $apid; Ctrl-C to cat, rc\'s; exit',
      init: 't_rc', cycles: 400e6,
      machine: {
        input: '\u0101hylang -l\r' + HYSH_RC.map(l => '\u0101' + l[0] + '\r').join('') + HYSH_LINES.map(l => '\u0101' + l[0] + '\r').join('') +
          '\u0101cat\r\u0100\x03' + '\u0101echo $status\r' + '\u0101exit\r',
      },
      get expect() {
        let at = '/', out = ['hylang (danlang on the Hydra-16)\nType \'exit\' to Exit\n\n/> '];
        for (const [l, o] of HYSH_RC) out.push('/> ' + l + '\n' + o + '\n/> ');
        for (const [l, o, cd] of HYSH_LINES) { const p = at + '> '; if (cd) at = cd; out.push(p + l + '\n' + (o === null ? '' : o + '\n') + at + '> '); }
        out.push('/> cat\n\n/> echo $status\ninterrupt\n/> exit\n');
        return out;
      },
    },
    {
      name: 'hywin', what: 'hylang as a window\'s shell: a card\'s /lib/shell naming /bin/hylang -l, init\'s in window 0 and in a window made (Ctrl-] c: $window, cons.hl\'s window)',
      init: 'init', cycles: 200e6,
      get machine() {
        return { sd: shellCard('/bin/hylang -l', 'shellhy'), input: '\u0101(+ 1 2)\r' + '\u0101echo $window\r' + '\u0101\x1dc' +
          '\u0101echo $window\r' + '\u0101(use "cons")\r' + '\u0101(window)\r' };
      },
      expect: ['/> (+ 1 2)\n=> 3\n/> echo $window\n\n/> ', 'hylang (danlang on the Hydra-16)\nType \'exit\' to Exit\n\n/> echo $window\n1\n' +
        '/> (use "cons")\n=> NIL\n/> (window)\n=> 1\n/> '],
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
    {
      name: 'bench', what: 'hylang\'s, HyForth\'s and BASIC\'s benchmarks (sdcard/bench, on the SD card: bench.hl and hl/NAME.hl, bench.fs, bench.bas, and benchasm.bas, BASIC\'s inline assembly: all twenty in each; sim/bench.js times them against each other) at their quick sizes, all of hylang\'s in one hylang: each language\'s result of each the same (calls, fib, tak, ack; loop, while, dotimes, nested; gcd, collatz, hash; sieve, sort, matrix, queens; mapf, fold, each; chars, digits)',
      init: 't_rc', cycles: 360e6,
      machine: { input: '\u0101hylang /sd/0/bench/bench.hl 1 q\r\u0101forth /sd/0/bench/bench.fs 1 q\r\u0101basic /sd/0/bench/bench.bas 1 q\r' +
        '\u0101basic /sd/0/bench/benchasm.bas 1 q\r' },
      get expect() {
        const r = [['calls', 500], ['fib', 144], ['tak', 12], ['ack', 42], ['loop', 1000], ['while', 1500], ['dotimes', 1500],
          ['nested', 450], ['gcd', 189], ['collatz', 441], ['hash', 1274], ['sieve', 97], ['sort', 404], ['matrix', 273], ['queens', 4],
          ['mapf', 9880], ['fold', 964], ['each', 700], ['chars', 7], ['digits', 790]];
        return [...['hylang', 'forth'].flatMap(l => r.map(([n, v]) => 'bench ' + l + ' ' + n + ' ' + v + ' ')), 'bench hylang done', 'bench forth done',
          ...r.map(([n, v]) => 'bench basic ' + n + ' ' + v + ' '), 'bench basic done', ...r.map(([n, v]) => 'bench basm ' + n + ' ' + v + ' '),
          'bench basm done'];
      },
      sdcard: true
    },
    {
      name: 'hydev', what: 'hylang\'s device libraries (the plan\'s phase 11: /lib/hylang\'s, loaded by use, over the devices\' files), devices.hl as a script: gpio (pins, the port, ctl as a hash, CA1\'s edge), i2c (a memory written and read at a register, the devices, one that doesn\'t answer), spi (an echo device\'s transactions, mode 3), cons (the window, the windows, the bell), proc (a task\'s args, cwd, regs, memory, banks; its environment, its namespace), clock (the chip, the time set), disk (the disks, the cards: this one and one on SPI device 5; the ROM disk\'s room), pc (the PC tool answers; a file of its read), snd (note-of; a tune, its notes on the YM2151 in time; a channel\'s settings; the registers read back: a bent note\'s key code and fraction; a frequency, a glide, the LFO, a sensitivity, the noise; a line of MML and a chord, by play)',
      init: 't_rc', cycles: 400e6, pc: { files: { 'hi.txt': 'hi from the PC\n' } },
      get machine() {
        return { gpioIn: 0xA5, ca1: Array.from({ length: 60 }, (_, i) => 100e6 + i * 50e6), i2c: { 0x50: 256, 0x68: 16 }, spiEcho: [3],
          sd: [...hylangCard(this.name), card(5, 2048, false, () => 0)], rtc: Date.UTC(2026, 9, 3, 15, 4, 5) / 1000,
          input: '\u0101cd /sd/0; hylang devices.hl; echo status $status\r' };
      },
      expect: ['72 checks, 0 failed\nstatus\n%'],
      // (The tune: C4, E4 a beat on (a tenth of a second at 600 a minute), a rest, G4 two beats after E4)
      check(m) {
        const f = pcReport(m, 1, 0, 0), mult = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const on = m.ym.keyOns.filter(k => k.startsWith('ch 0 ')).map(k => +k.match(/at cycle (\d+)/)[1]);
        const beat = 0.1 * 3579545 * mult, slack = 2 * 3579545 * mult / 200;
        if (on.length !== 3) return [...f, 'tune: ' + on.length + ' key-ons on channel 0, not 3: ' + m.ym.keyOns.join(', ')];
        [1, 2].forEach((beats, k) => { if (Math.abs(on[k + 1] - on[k] - beats * beat) > slack)
          f.push('tune: key-on ' + (k + 1) + ' came ' + (on[k + 1] - on[k]) + ' cycles after the last, not ' + beats + ' beat(s) (' + Math.round(beats * beat) + ')'); });
        return f;
      },
    },
    {
      name: 'irq', what: 'spike S1: 115200 received by an irq entry while tasks spin',
      init: 't_irq', modules: ['t_child'], without: ['cons', 'storage', 'snd', 'vid'], cycles: 30e6,
      send: { after: 'ready>', bytes: Array.from({ length: S1_BYTES }, (_, i) => (3 + 7 * i) & 0xFF) },
      check(m) {
        const f = [], l = m.acia.rxLat, char = m.acia.charCycles();
        if (m.acia.pcLost) f.push(m.acia.pcLost + ' bytes lost by the ACIA (one arrived before the last was read)');
        if (l.n) this.notes = ['receive latency (a byte in to its read): ' + l.min + '-' + l.max + ' cycles, ' + Math.round(l.sum / l.n) +
          ' on average, over ' + l.n + ' bytes; the limit is a character: ' + char + ' cycles'];
        if (l.max > char) f.push('a byte waited ' + l.max + ' cycles to be read: over a character\'s time (' + char + ')');
        return f;
      },
    },
    // ---- Phase 8: the Vera X (the emulator's VERA: sim/lib/vera.js; the driver: modules/vid)
    {
      name: 'vera', what: 'the emulator\'s Vera X (sim/lib/vera.js), the chip as a program sees it (no vid): the version register; ADDR0 and ADDR1, their steps, a data port\'s byte fetched ahead; the display\'s registers at the start; VSYNC (59.5 a second), LINE and SCANLINE (bit 8 too); sprites colliding; the PCM FIFO (empty, full, AFLOW and its interrupt\'s time); a PSG voice; the SPI port with no card; FX (the cache\'s writes and fill, transparency, the multiplier, the line helper, the fill length); CTRL\'s reset',
      init: 't_vera', without: ['vid'], cycles: 40e6, machine: { vera: true },
      check(m) {
        const f = [];
        if (!m.vera.psgOns.some(k => k.startsWith('voice 0 '))) f.push('PSG voice 0 never came on');
        if (m.vera.pcmOut < 977) f.push('the PCM FIFO drained ' + m.vera.pcmOut + ' bytes (977 at least)');
        this.notes = ['the VERA: ' + m.vera.frames + ' frames, ' + m.vera.pcmIn + ' PCM bytes in, ' + m.vera.pcmOut + ' out, ' + m.vera.pcmLost + ' lost (full)'];
        return f;
      },
    },
    {
      name: 'vid', what: 'the Vera X\'s driver (vid: #v), through its files: ctl\'s state; the terminal (/term): text written and read back, a CSI move, a line erased, wrapping, BS and TAB, 70 lines scrolled, SGR\'s colours (in the map\'s cells), the cursor\'s sprite; /frame (a frame a read, 59.5 a second); /vram, /pal, /font, the files\' lengths; ctl\'s commands (mode, cursor, border, bitmap, bad ones); claims: a write to the terminal E_BUSY meanwhile, shown after the release; claim all (the font back); another task\'s (E_BUSY), ended by its end',
      init: 't_vid', cycles: 80e6, machine: { vera: true },
    },
    {
      name: 'vid-none', what: 'vid with no card: its init looks for DETECT_TICKS, then ends; no #v (E_NODEV)',
      init: 't_vid', cycles: 30e6, expect: ['ok - no card: #v isn\'t there (E_NODEV)', 't_vid: PASS'],
    },
    {
      name: 'screen', what: 'the console on the Vera X\'s screen (cons\'s second terminal: vid\'s /term), at rc: /dev/vid; consctl\'s terminal both, serial (the screen left as it was), both again (repainted); the boot\'s lines whole (the console\'s ESC [ 18 t not mixed into the kernel\'s next); a font written to /dev/vid/font; ctl\'s output (NTSC, mono, 240p; VGA has neither); colours from a file (SGR, in the cells); DECSCNM (every cell reversed); the DEC graphics (ESC ( 0) as the font\'s glyphs; what rc shows, on the screen as on the serial port',
      init: 't_rc', cycles: 150e6, pc: { files: { colours: SCREEN_COLOURS, box: '\x1b(0lqk\x1b(B\n', reverse: '\x1b[?5h' } },
      get machine() { return { input: typed(SCREEN_LINES), vera: true }; },
      get expect() { return expected(SCREEN_LINES); },
      check(m, out) {
        const f = [], c = m.vera.cells(), text = m.vera.text();
        if (!c) return ['no text layer on the screen'];
        // (The console's question to the terminal, ESC [ 18 t, sent whole before the kernel's next line: cons's init waits for it)
        if (!out.includes('\x1b[18ttask 1: t_rc')) f.push('the boot\'s "task 1: t_rc" isn\'t whole after the console\'s ESC [ 18 t');
        // (The window shown is the smaller terminal's size, the serial port's 80 x 24: its rows below the screen's
        // chrome, the bar and its header, then its footer: W4; rc's line looked for one its last 24 rows still have)
        if (!text.some(l => l.startsWith('% echo flash >/dev/vid/ctl'))) f.push('the screen lacks rc\'s line "% echo flash >/dev/vid/ctl"');
        if (text.slice(27).some(l => l)) f.push('the screen has text below the window\'s 24 rows and its footer');
        if (!/^ 0 \S+ .* \d\d:\d\d$/.test(text[0] || '')) f.push('the screen\'s bar (row 1) isn\'t " 0 label ... HH:MM": ' + JSON.stringify(text[0]));
        if (!/^0 \S+ .* 0 \S+$/.test(text[1] || '')) f.push('the window\'s header (row 2) isn\'t "0 label ... 0 label": ' + JSON.stringify(text[1]));
        if (!text.some(l => l === 'terminal both')) f.push('the screen lacks consctl\'s "terminal both"');
        // (The DEC graphics, ESC ( 0's lqk, as the font's glyphs $0D $12 $0C: tools/decfont.js's)
        let box = false;
        for (let i = 0; i + 2 < c.chars.length; i++) if (c.chars[i] === 0x0D && c.chars[i + 1] === 0x12 && c.chars[i + 2] === 0x0C) box = true;
        if (!box) f.push('the screen lacks the DEC graphics\' corner, line and corner (glyphs $0D $12 $0C)');
        const row = text.findIndex(l => l === 'RnGV');
        if (row < 0) f.push('the screen lacks the colours\' line RnGV');
        else {
          const at = row * c.cols, attrs = Array.from(c.attrs.subarray(at, at + 4)).map(a => '$' + a.toString(16).toUpperCase().padStart(2, '0')).join(' ');
          // (DECSCNM set at the end: each cell's colours reversed; V's, reversed itself, plain)
          if (attrs !== '$14 $70 $A0 $07') f.push('the colours\' cells, the screen reversed: ' + attrs + ' ($14 $70 $A0 $07 wanted)');
        }
        const font = fs.readFileSync(path.join(__dirname, '..', 'romfs', 'lib', 'font', 'cp437'));
        if (!Buffer.from(m.vera.vram.subarray(0x1F000, 0x1F800)).equals(font)) f.push('VRAM\'s font isn\'t /lib/font/cp437');
        this.notes = ['the screen\'s last rows: ' + JSON.stringify(text.filter(l => l).slice(-3))];
        return f;
      },
    },
    {
      name: 'kbinclock', what: 'the console\'s keyboard (the Vera X\'s, the SMC\'s: kdev\'s input writing #c/kbin) typed at slowly as the bar\'s clock turns a minute (the user\'s, 2026-10-16, in the browser: "only printed about 3/4 of the text and then froze"): the chrome drawn again in the pump after a /kbin write read #t/time, a call to kdev, busy with its look and waiting for that write\'s answer, the two waiting for each other for good; now the console makes the time from the kernel\'s clock (TIME), no call to kdev.  And consctl\'s keyboard screen once the keyboard\'s typed',
      init: 't_rc', cycles: 260e6,
      // (The clock at 15:04:52 as it boots: its minute turns as the keys come, one each 2M cycles)
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2026, 9, 16, 15, 4, 52) / 1000,
        input: 'ācat /dev/consctl\rāĂecho ' + 'kĀ'.repeat(48) + '\răācat /dev/consctl\rāecho do^ne\r' }; },
      expect: ['\n' + 'k'.repeat(48) + '\n', '\nkeyboard screen\n', '\ndone\n'],
      check(m, out) {
        const f = [], first = out.slice(0, out.indexOf('echo k'));
        if (first.includes('keyboard screen')) f.push('consctl said keyboard screen before the keyboard had typed');
        if (!/ 15:0[5-9]$/.test(m.vera.text()[0] || '')) f.push('the bar\'s clock didn\'t turn a minute: ' + JSON.stringify(m.vera.text()[0]));
        return f;
      },
    },
    {
      name: 'kbinstatus', what: 'the console\'s keyboard (kdev\'s input writing #c/kbin) typed at while a loop writes the window\'s status (wctl\'s status, its footer %s %t: each write\'s pump draws the chrome again, the time with it): the console, serving rc\'s write, read #t/time (a call to kdev) while kdev\'s input, busy with its look, waited to write /kbin to it, each waiting for the other for good (the second freeze, 2026-10-16: dq0\'s consctl write as hmchar began; the first, kbinclock\'s, the pump after the /kbin write itself); the time the kernel\'s now (TIME), no call to kdev at all',
      init: 't_rc', cycles: 400e6,
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2026, 9, 16, 15, 4, 0) / 1000,
        input: '\u0101echo footer %s %t >/dev/wctl\r\u0101d=(0 1 2 3 4 5 6 7 8 9); for(i in 0 1 2) for(j in $d) for(k in $d) echo status $i$j$k >/dev/wctl\r' +
          '\u0102echo ' + 'k\u0100'.repeat(40) + '\r\u0103' + '\u0100'.repeat(20) + 'echo do^ne\r' }; },
      expect: ['\n' + 'k'.repeat(40) + '\n', '\ndone\n'],
      check(m) {
        const foot = m.vera.text().filter(l => l).pop() || '';
        return /^299 15:0\d$/.test(foot) ? [] : ['the footer, the loop done: ' + JSON.stringify(foot) + ' ("299 15:0N" wanted)'];
      },
    },
    {
      name: 'chrometime', what: 'the bar\'s date and time (%d %t) made by the console from the kernel\'s clock (TIME), not #t/time (a call to kdev, which could wait for good on kdev\'s input writing #c/kbin): its calendar kdev\'s, a leap day\'s last seconds and the next day\'s first, as /dev/time has them',
      init: 't_rc', cycles: 120e6,
      // (SNAP: the screen's bar, as the keyboard's typing comes to it; nothing typed)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push(m.vera.text()[0]) : type(ch);
      },
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2028, 1, 29, 23, 59, 50) / 1000,
        input: 'āecho bar %d %t >/dev/wctl\rācat /dev/time\rāĂЀă' + 'Ā'.repeat(20) + 'cat /dev/time\rāĂЀăecho do^ne\r' }; },
      expect: ['\n2028-02-29 23:59:5', '\n2028-03-01 00:00:', '\ndone\n'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 2) return ['the snapshots: ' + s.length + ' (2 wanted)'];
        if (!/^2028-02-29 23:59 *$/.test(s[0])) f.push('the bar before midnight: ' + JSON.stringify(s[0]));
        if (!/^2028-03-01 00:00 *$/.test(s[1])) f.push('the bar after midnight: ' + JSON.stringify(s[1]));
        return f;
      },
    },
    {
      name: 'chrometime2', what: 'the bar\'s date and time (chrometime\'s) in 2100, not a leap year (a century): its February\'s last seconds and March\'s first; the date and time (%d %t) made by the console from the kernel\'s clock (TIME), not #t/time (a call to kdev, which could wait for good on kdev\'s input writing #c/kbin): its calendar kdev\'s, a leap day\'s last seconds and the next day\'s first, as /dev/time has them',
      init: 't_rc', cycles: 120e6,
      // (SNAP: the screen's bar, as the keyboard's typing comes to it; nothing typed)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push(m.vera.text()[0]) : type(ch);
      },
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2100, 1, 28, 23, 59, 50) / 1000,
        input: 'āecho bar %d %t >/dev/wctl\rācat /dev/time\rāĂЀă' + 'Ā'.repeat(20) + 'cat /dev/time\rāĂЀăecho do^ne\r' }; },
      expect: ['\n2100-02-28 23:59:5', '\n2100-03-01 00:00:', '\ndone\n'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 2) return ['the snapshots: ' + s.length + ' (2 wanted)'];
        if (!/^2100-02-28 23:59 *$/.test(s[0])) f.push('the bar before midnight: ' + JSON.stringify(s[0]));
        if (!/^2100-03-01 00:00 *$/.test(s[1])) f.push('the bar after midnight: ' + JSON.stringify(s[1]));
        return f;
      },
    },
    {
      name: 'vt', what: 'the console\'s VT100 (W1): sequences into a window not shown, its /text read back: text, the cursor\'s moves, erasing, inserting and deleting, the scrolling region, the scrollback, tabs, autowrap, insert mode, the character sets, DECSC and origin mode, DECALN, RIS, REP, SGR, VT52 mode, the alternate screen, double width and height, sequences dropped, cancelled and split; each as sim/lib/vt.js has it, and vt.js as xterm.js has it (if it\'s installed)',
      init: 't_rc', cycles: 600e6,
      pc: { files: () => Object.fromEntries(VT_FIXTURES.map(([n, b]) => ['vt/' + n, vtFile(b)])) },
      get machine() { return { input: typed(VT_LINES) }; },
      get expect() { return expected(VT_LINES); },
      check() { const r = vtXterm(); this.notes = [r.note]; return r.f; },
    },
    {
      name: 'vtpaint', what: 'a window painted (W1): text in colours, a box in DEC graphics, a line autowrapped, a double-width row, a region and the cursor, written into a window not shown, which is then shown: what the serial port\'s terminal shows (sim/lib/vt.js: its characters and colours) and what the screen shows, as the window has it',
      init: 't_rc', cycles: 200e6, pc: { files: { 'vt/paint': vtFile(VT_PAINT), 'vt/paint.rc': VT_PAINT_RC } },
      get machine() {
        return { vera: true, input: typed([['rc /pc/vt/paint.rc']]) };
      },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], want = vtModel(VT_PAINT), raw = m.out;
        const clear = '\x1b[0m\x1b(B\x1b)B\x0f', at = raw.indexOf('rc /pc/vt/paint.rc');
        const a = raw.indexOf(clear, at), b = raw.indexOf(clear, a + 1);
        if (a < 0 || b < 0) return ['the serial port: no paint of window 1 (and of window 0 after it)'];
        const ser = new VT().write(raw.slice(a, b));
        for (let r = 0; r < 24; r++) {
          for (let c = 0; c < 80; c++) {
            const x = want.screen[r][c], y = ser.screen[r][c];
            if (x.c !== y.c || x.a !== y.a || x.f !== y.f) { f.push('the serial port\'s terminal: row ' + (r + 1) + ', column ' + (c + 1) + ': ' + JSON.stringify(y) + ', not ' + JSON.stringify(x)); break; }
          }
          if (f.length) break;
        }
        if (ser.y !== want.y || ser.x !== want.x) f.push('the serial port\'s cursor at ' + (ser.y + 1) + ';' + (ser.x + 1) + ', not ' + (want.y + 1) + ';' + (want.x + 1));
        if (ser.top !== want.top || ser.bot !== want.bot) f.push('the serial port\'s region ' + (ser.top + 1) + '-' + (ser.bot + 1) + ', not ' + (want.top + 1) + '-' + (want.bot + 1));
        const out = raw.replace(/\r/g, ''), s0 = out.indexOf('\npainted\n');
        const scr = s0 < 0 ? [] : out.slice(s0 + 9).split('\n').map(l => l.replace(/ +$/, ''));
        const rows = want.screen.map(r => r.dw ? [...VT.rowText(r)].map(c => c + ' ').join('').replace(/ +$/, '') : VT.rowText(r));
        for (let r = 0; r < 24; r++) if (scr[r + 2] !== rows[r]) { f.push('the screen\'s row ' + (r + 3) + ' (the window\'s ' + (r + 1) + ', below the bar and its header): ' + JSON.stringify(scr[r + 2]) + ', not ' + JSON.stringify(rows[r])); break; }
        return f;
      },
    },
    {
      name: 'vtjump', what: 'scroll jump (consctl): the ROM disk\'s api.md (38K) cat to the window shown, its writer not waiting for the serial line, the terminal painted as it can: far fewer bytes sent than written, the file\'s last line shown at the end; scroll smooth again',
      init: 't_rc', cycles: 300e6,
      get machine() { return { input: typed([['echo scroll jump >/dev/consctl; cat /rom/doc/api.md; echo scroll smooth >/dev/consctl; echo after']]) }; },
      expect: ['\nafter\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), at = out.indexOf('echo after\n'), end = out.indexOf('\nafter\n', at);
        if (at < 0 || end < 0) return ['no output between the command and its end'];
        const big = vtJump(), sent = end - at, last = big.trim().split('\n').pop();
        this.notes = ['sent ' + sent + ' bytes for the file\'s ' + big.length];
        if (sent > big.length * 0.6) f.push('scroll jump sent ' + sent + ' bytes of the file\'s ' + big.length + ' (more than 60%)');
        if (!out.slice(at, end).includes(last)) f.push('the file\'s last line not shown: ' + last);
        return f;
      },
    },
    {
      name: 'vtsize', what: 'a window\'s size (W3: terminal size, both terminals on): a window not shown with 30 lines in it, made shorter (the cursor high: rows dropped at the bottom), taller (the scrollback\'s rows down onto it, then blank ones), narrower (the long lines cut), wider; its alternate screen in use, shorter (rows off its top dropped), then the main one again (the saved cursor where its row went); its /text after each, a mark at its cursor, as sim/lib/vt.js\'s resize has it',
      init: 't_rc', cycles: 150e6, pc: { files: vtSizeFiles() },
      get machine() { return { input: typed([['rc /pc/vt/size.rc']]) }; },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), want = new VT({ onlcr: true }).write(VT_SIZE);
        VT_SIZE_STEPS.forEach(([pre, c, r, mark], i) => {
          want.write(pre).resize(c, r).write(mark);
          const a = out.indexOf('[' + i + ']\n'), b = out.indexOf('[/' + i + ']', a);
          if (a < 0 || b < 0) { f.push('step ' + i + ': no /text read'); return; }
          const got = out.slice(a + 3 + String(i).length, b).replace(/\n$/, ''), exp = want.text().replace(/\n$/, '');
          if (got !== exp) {
            const g = got.split('\n'), e = exp.split('\n'), k = e.findIndex((l, j) => l !== g[j]);
            f.push('step ' + i + ' (' + c + ' x ' + r + '): /text row ' + (k < 0 ? g.length : k + 1) + ' ' + JSON.stringify(g[k < 0 ? e.length : k]) + ', not ' + JSON.stringify(e[k]) + ' (' + g.length + ' rows, not ' + e.length + ')');
          }
        });
        return f;
      },
    },
    {
      name: 'vtmode', what: 'a window\'s size from the screen\'s (W3): the screen alone (80 x 60, less its chrome\'s 3 rows: the bar, the header, the footer), vid\'s mode changed under the console (40x30: its next write refused once, the size looked at, the windows resized and the screen painted again; 80x30), then both terminals (the smaller: the serial port\'s 80 x 24)',
      init: 't_rc', cycles: 150e6, pc: { files: { 'vt/mode.rc': VT_MODE_RC } },
      get machine() { return { vera: true, input: typed([['rc /pc/vt/mode.rc']]) }; },
      expect: ['size 80 57\nsize 40 27\nsize 80 27\nsize 80 24\n', '\ndone\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), a = out.indexOf('[t2]\n'), b = out.indexOf('[/t2]', a);
        if (a < 0 || b < 0) return ['no screen read at 40x30'];
        const rows = out.slice(a + 5, b).replace(/\n$/, '').split('\n');
        if (rows.length !== 30 || rows.some(r => r.length !== 40)) f.push('the screen at 40x30 read as ' + rows.length + ' rows of ' + [...new Set(rows.map(r => r.length))].join(', ') + ' columns');
        if (!rows.some(r => r.startsWith('after 40x30'))) f.push('the screen at 40x30: not painted again (no "after 40x30" on it)');
        return f;
      },
    },
    {
      name: 'vtedit', what: 'the line editor at the window\'s width (W3): rc\'s lines typed in a window 20 columns wide (terminal size 20 24), each longer than a row: Left back across the rows, characters inserted, Home, Right, Delete, End; then that line again (Up), cut back with Backspace across the rows; a third made 30 columns wide as it\'s typed (the terminal\'s report among the keys: the line drawn again), then edited: what rc read, and what the serial port\'s terminal shows of each line (sim/lib/vt.js, 20 and 30 columns), as typed, the rest of its last row blank',
      init: 't_rc', cycles: 150e6,
      get machine() { return { input: typed([['echo terminal size 20 24 >/dev/consctl'], [VT_EDIT[0]], [VT_EDIT[1]], [VT_EDIT[2]], ['echo terminal size 80 24 >/dev/consctl'], ['echo done']]) }; },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], raw = m.out, out = raw.replace(/\r/g, ''), hist = [];
        const lines = VT_EDIT.map(k => { const l = edSim(k, hist); hist.push(l); return l; });
        lines.forEach((l, i) => { if (!out.includes('\n' + l.slice(5) + '\n')) f.push('line ' + (i + 1) + ': rc didn\'t echo ' + JSON.stringify(l.slice(5))); });
        const clear = '\x1b[0m\x1b(B\x1b)B', a = raw.indexOf(clear, raw.indexOf('size 20 24')), b = raw.indexOf(clear, a + 1), c = raw.indexOf(clear, b + 1);
        if (a < 0 || b < 0 || c < 0) return f.concat(['no paints at 20 columns, then 30, then 80']);
        const shownAt = (seg, cols, start, l, i) => {             // (The line on the terminal, and the rest of its last row)
          const rows = new VT({ cols, rows: 24 }).write(seg).lines().map(r => r.padEnd(cols)), k = rows.findIndex(r => r.startsWith(start));
          const n = Math.ceil((l.length + 2) / cols) * cols, shown = k < 0 ? '' : rows.slice(k).join('').slice(0, n);
          if (shown !== ('% ' + l).padEnd(n)) f.push('line ' + (i + 1) + ' on the terminal (' + cols + ' columns): ' + JSON.stringify(shown) + ', not ' + JSON.stringify(('% ' + l).padEnd(n)));
        };
        const seg20 = raw.slice(a, b), mid = seg20.indexOf(lines[0].slice(5) + '\r\n');
        shownAt(seg20, 20, '% echo 01', lines[0], 0);
        shownAt(seg20.slice(mid), 20, '% echo 01', lines[1], 1);
        shownAt(raw.slice(b, c), 30, '% echo abc', lines[2], 2);
        this.notes = lines.map((l, i) => 'line ' + (i + 1) + ': ' + l);
        return f;
      },
    },
    {
      name: 'winsize', what: 'a window\'s size in each language (W3): terminal size 100 30, then rc (consctl\'s size line), HyForth (form, k-resize), hylang (cons.hl\'s window-size), C (conio: the keys sample\'s screensize, and CH_RESIZE as the terminal\'s report is typed, 120 x 40); the editor drawn again as its window changes (its help lines on the new last rows)',
      init: 't_rc', cycles: 120e6,
      get machine() {
        return { input: 'āecho terminal size 100 30 >/dev/consctl; grep size /dev/consctl\r' +
          'āforth\rĀĀ' + 'lib facility form . . k-resize .\rĀ' + 'bye\r' +
          'āhylang\rĀĀ' + '(use "cons")\r' + 'ā(window-size)\r' + 'ā(exit)\r' +
          'ā/sd/0/sample/c/keys\rĀĀ' + '\x1b[8;40;120t' + 'Ā' + 'q' + 'āgrep size /dev/consctl\r' +
          'āedit /ram/w\rĀĀ' + '\x1b[8;30;100t' + 'ĀĀ' + '\x18' + 'āecho terminal size 80 24 >/dev/consctl\r' + 'āecho done\r' };
      },
      expect: ['/dev/consctl\nsize 100 30\n%', 'k-resize .\n100 30 150  ok', '(window-size)\n=> {100 30}', 'keys: a 100x30 screen', ' 96\nended at',
        '% grep size /dev/consctl\nsize 120 40\n%', '\ndone\n%'],
      check(m) {
        const out = m.out, a = out.indexOf('edit /ram/w'), b = out.indexOf('echo terminal size 80 24', a);
        if (a < 0 || b < 0) return ['the editor: no output'];
        const ed = out.slice(a, b), r = ed.indexOf('\x1b[0m\x1b(B\x1b)B');            // (The resize's paint)
        return ed.lastIndexOf('\x1b[30;1H') > r && r > 0 ? [] : ['the editor: not drawn again at 100 x 30 (no help line on row 30 after the resize\'s paint)'];
      },
      sdcard: true
    },
    {
      name: 'winchrome', what: 'the chrome on the screen (W4): its label (#c0/label, OSC 2, empty: its program\'s name), its status line (wctl\'s status, and DECSASD\'s, after DECSSDT 2), the header\'s and footer\'s formats (%p, %l, %n, %s, %c, %r, %m, %[7], %=), the bar (its defaults: the windows, the time; at the bottom; off), a window\'s chrome rows turned off (its size grows by each), activity in a window not shown (monitor on: +; a bell: !); read back from vid\'s screen',
      init: 't_rc', cycles: 260e6,
      pc: { files: { 'vt/chrome.rc': CHROME_RC, 'vt/sasd': '\x1b[2$~\x1b[1$}\x1b[2Kfrom vt\x1b[0$}', 'vt/title': '\x1b]2;titled\x07', 'vt/bel': '\x07' } },
      get machine() { return { vera: true, input: typed([['rc /pc/vt/chrome.rc']]) }; },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), part = n => { const a = out.indexOf('[' + n + ']\n'); return a < 0 ? [] : out.slice(a + n.length + 3).split('\n'); };
        const row = (l, at, want) => { if (!want.test(l[at] || '')) f.push(at + ': ' + JSON.stringify(l[at]) + ' isn\'t ' + want); };
        const l = part('l'), z = part('z');
        if (l[0] !== 'mywin' || l[1] !== 'titled') f.push('the label read back: ' + JSON.stringify(l.slice(0, 2)) + ', not mywin, titled');
        if (z.slice(0, 4).join('|') !== 'size 80 58|size 80 59|size 80 60|size 80 57') f.push('the sizes as the chrome went: ' + JSON.stringify(z.slice(0, 4)));
        const s1 = part('s1'), s2 = part('s2'), s3 = part('s3'), s4 = part('s4'), s5 = part('s5'), s6 = part('s6');
        row(s1, 0, /^ 0 mywin +00:00$/); row(s1, 1, /^0 mywin +0 mywin$/); row(s1, 2, /^hello there *$/);
        row(s2, 1, /^\[rc\] mywin +0$/); row(s2, 2, /^from vt +80 x 57 cooked$/);
        row(s3, 0, /^\[rc\] mywin +0$/); row(s3, 1, /^from vt +80 x 57 cooked$/); row(s3, 2, /^ 0 mywin +00:00$/);
        row(s4, 0, /^ 0\+ titled +00:00$/); row(s5, 0, /^ 0! titled +00:00$/); row(s6, 0, /^ 0 rc +00:00$/);
        return f.map(x => 'winchrome: ' + x);
      },
    },
    {
      name: 'winser', what: 'the chrome on the serial port (W4b: chrome serial on): its bar, header and footer drawn there, the window 80 x 21 below the bar and header (its margins sent offset); 25 lines scrolling the window\'s rows alone; a clear (ED 2) then the chrome drawn again; a CUP and DECSTBM from a program offset; chrome serial off (80 x 24 again): what the PC\'s terminal shows (sim/lib/vt.js)',
      init: 't_rc', cycles: 260e6,
      // (One script: a chrome redraw after a prompt (the status changed) would hide it from the harness's wait)
      pc: { files: { clr: '\x1b[2J\x1b[Hcleared\n\x1b[5;10Hat 5,10\x1b[2;4r\x1b[20;1H', 'ser.rc': ['echo chrome serial on >/dev/wctl', 'grep size /dev/consctl',
        'echo -n sertitle >/dev/label', 'echo status st1 >/dev/wctl', 'for (i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25) echo line $i',
        'cat /pc/clr', 'echo chrome serial off >/dev/wctl', 'grep size /dev/consctl', 'echo done', ''].join('\n') } },
      get machine() { return { input: typed([['rc /pc/ser.rc']]) }; },
      expect: ['size 80 21\n', 'size 80 24\n', '\ndone\n%'],
      check(m) {
        const f = [], raw = m.out, clear = '\x1b[0m\x1b(B\x1b)B';
        const a = raw.indexOf(clear, raw.indexOf('rc /pc/ser.rc')), b = raw.indexOf(clear, raw.indexOf('at 5,10'));
        if (a < 0 || b < 0) return ['no paints for chrome serial on and off'];
        const look = (upto, what, rows) => {                     // (The terminal's rows, as far as upto)
          const t = new VT({ cols: 80, rows: 24 }).write(raw.slice(a, upto)), l = t.lines();
          for (const [r, re] of rows) if (!re.test(l[r] || '')) f.push(what + ': row ' + (r + 1) + ' ' + JSON.stringify(l[r]) + ' isn\'t ' + re);
          return t;
        };
        const lines = raw.indexOf('\x1b[2J', raw.indexOf('line 25'));
        look(lines, 'after 25 lines', [[0, /^ 0 sertitle +00:00$/], [1, /^0 sertitle +0 sertitle$/], [2, /^line 6$/], [21, /^line 25$/], [22, /^$/], [23, /^st1$/]]);
        const t = look(b, 'after the clear', [[0, /^ 0 sertitle +00:00$/], [1, /^0 sertitle +0 sertitle$/], [2, /^cleared$/], [6, /^ +at 5,10$/], [23, /^st1$/]]);
        if (t.top !== 3 || t.bot !== 5) f.push('the program\'s region (2;4) on the terminal: rows ' + (t.top + 1) + '-' + (t.bot + 1) + ', not 4-6');
        return f;
      },
    },
    {
      name: 'winwords', what: 'the window\'s chrome in HyForth and hylang (W4c; C\'s are ctest\'s): window-label (read back from /dev/label; hylang\'s read too), window-status (the footer\'s %s, on the screen\'s footer row: both terminals on, the window 80 x 24 below the bar and header), window-ctl',
      init: 't_rc', cycles: 200e6,
      get machine() {
        return { vera: true, input: 'āforth\rĀĀ' + 'lib cons s" fth" window-label s" st-fth" window-status s" monitor off" window-ctl\rĀ' + 'bye\r' +
          'ācat /dev/label; echo; head -27 /dev/vid/term | tail -1\r' +
          'āhylang\rĀĀ' + '(use "cons")\r' + 'ā(window-label "hyl")\r' + 'ā(window-label)\r' + 'ā(window-status "st-hyl")\r' + 'ā(window-ctl "monitor off")\r' + 'ā(exit)\r' +
          'āhead -27 /dev/vid/term | tail -1; echo done\r' };
      },
      expect: ['monitor off" window-ctl\n ok', 'tail -1\nfth\nst-fth', '(window-label)\n=> "hyl"', '(window-ctl "monitor off")\n=> NIL', 'echo done\nst-hyl', '\ndone\n%'],
    },
    {
      name: 'newwin', what: 'new-window (W5c): a command run in a window made and shown, in this one\'s group ($window its number; the window gone when it ends, this one shown again), -g\'s in a group of its own, the shell (/lib/shell\'s: HyForth) with none; HyForth\'s new-window and new-group, hylang\'s',
      init: 't_rc', cycles: 200e6,
      // (Each window shown as it's made: its output on the terminal then, the window it came from painted again
      // when it goes, the prompt last.  The shell's window: a line typed there, then bye)
      get machine() {
        return { input: 'ānew-window \'echo hi from $window; sleep 1\'\r' + 'ānew-window -g sleep 2; cat /dev/wctl\r' +
          'ānew-window\r' + 'āecho shell in $window; cat /dev/wctl\r' + 'ābye\r' +
          'āforth -l\r' + 'ālib cons s" echo fth $window; sleep 1" new-window\r' + 'ās" echo fgrp; cat /dev/wctl; sleep 1" new-group\r' + 'ābye\r' +
          'āhylang\r' + 'ā(use "cons")\r' + 'ā(new-window "echo hyl $window; sleep 1")\r' + 'ā(new-group "echo hgrp; cat /dev/wctl; sleep 1")\r' + 'ā(exit)\r' +
          'āecho done\r' };
      },
      expect: ['hi from 1\n', 'cat /dev/wctl\n0 0 80 24\n1 1 80 24 *\n', 'shell in $window; cat /dev/wctl\nshell in 1\n0 0 80 24\n1 0 80 24 *\n',
        'fth 1', 'fgrp\n0 0 80 24\n1 1 80 24 *', 'hyl 1', 'hgrp\n0 0 80 24\n1 1 80 24 *', 'echo done\ndone\n%'],
    },
    {
      name: 'winkeys', what: 'the windows\' keys (W5d): wctl\'s key lines (the prefix Ctrl-A, keys after it, Ctrl-Tab\'s action; a digit, an unknown action, Ctrl-C as the prefix refused); Ctrl-] w\'s list (a line a window: its key, number, label and group; chosen with the arrows and Enter, a key, q, Escape twice; the window shown each time it opens marked; activity, +, as monitor on and off say); Ctrl-Tab and Ctrl-Shift-Tab as Windows Terminal\'s win32-input-mode sends them, made into CSI u\'s by sim/lib/win32in.js (the PC tool\'s --win32-input); keys mods (HyForth\'s ekey: Ctrl-Up, Up, Shift-F2, the k- masks or\'d in)',
      init: 't_rc', cycles: 200e6,
      // (Windows 0, 1 in its group, 2 in a group of its own, each held by a sleep; monitor on in 1, on then off in 2,
      // each written to: the first list marks 1's activity, +, not 2's.  The lists, in order, with the window shown before each
      // marked: 0; 2 (the first's down, down, Enter); 1 (its 0, then Ctrl-Tab); 0 (its q, then Ctrl-Shift-Tab); 2
      // (its Escape twice, then Ctrl-Tab bound to next-group; the list by the key g, its 1, then Ctrl-A 0)
      get machine() {
        const w32 = s => { const o = []; const d = createWin32Input(b => o.push(...b)); for (const c of Buffer.from(s, 'latin1')) d.push(c); d.flush(); return String.fromCharCode(...o); };
        const rec = (vk, uc, cs) => '\x1b[' + vk + ';15;' + uc + ';1;' + cs + ';1_';
        const CTAB = w32(rec(9, 9, 8)), CSTAB = w32(rec(9, 9, 0x18)), L = '\u0100', W = '\x01w' + L;
        return { input: 'āecho b115200 >/dev/serctl\r' + 'āecho new >/dev/wctl; echo new group >/dev/wctl\r' + 'āecho key prefix ctrl-a >/dev/wctl\r' +
          'āsleep 60 >\'#c1/cons\' &\r' + 'āsleep 60 >\'#c2/cons\' &\r' + 'āecho monitor on >\'#c1/wctl\'\r' +
          'āecho monitor on >\'#c2/wctl\'\r' + 'āecho monitor off >\'#c2/wctl\'\r' + 'āecho x >\'#c1/cons\'; echo x >\'#c2/cons\'\r' +
          'āecho key 5 list >/dev/wctl\r' + 'āecho key z bogus >/dev/wctl\r' + 'āecho key prefix ctrl-c >/dev/wctl\r' +
          'ā' + W + '\x1b[B' + L + '\x1b[B' + L + '\r' + L + W + '0' +
          'ā' + CTAB + L + W + 'q' + L + CSTAB + L + W + '\x1b\x1b' +
          'āecho key ctrl-tab next-group >/dev/wctl\r' + 'āecho key g list >/dev/wctl\r' + 'ā' + CTAB + L + '\x01g' + L + '1' + L + '\x010' +
          'ācat /dev/wctl\r' + 'āforth\r' + L + L + 'require facility.fl\r' + L +
          ': t s" /dev/consctl" w/o open-file throw >r s" keys mods" r@ write-file throw\r' + L +
          '  ekey . ekey . ekey . ekey . r> close-file throw ;\r' + L + 't\r' + L + '\x1b[1;5A' + L + '\x1b[A' + L + '\x1b[1;2Q' + L + 'x' + L +
          'bye\r' + 'āecho done\r' };
      },
      expect: ['5 list >/dev/wctl\necho: write error: invalid argument', 'z bogus >/dev/wctl\necho: write error: invalid argument',
        'prefix ctrl-c >/dev/wctl\necho: write error: invalid argument', '> 0  0 rc  (group 0)', '  1  1+   (group 0)', '  2  2   (group 1)',
        'cat /dev/wctl\n0 0 80 24 *\n1 0 80 24\n2 1 80 24\n', '640 128 395 120  ok', 'echo done\ndone\n%'],
      check(m) {
        const out = m.out.replace(/\r/g, ''), marked = [];
        for (const part of out.split('A window\'s key, or the arrows').slice(1)) { const k = part.match(/> ([0-9a-f])  \d/); marked.push(k ? k[1] : '?'); }
        return marked.join(' ') === '0 2 1 0 2' ? [] : ['the lists marked ' + marked.join(' ') + ', not 0 2 1 0 2'];
      },
    },
    {
      name: 'snarf', what: '/dev/snarf (W6a): written and read back; a file through it, the same (cmp); 8K at most (past them, disk full); Ctrl-] y\'s paste into rc as its keys (two lines: each LF a CR); hylang\'s snarf! and snarf; bracketed paste (?2004) to a keys vt reader, HyForth\'s ekey: CSI 200 ~, the text, CSI 201 ~',
      init: 't_rc', cycles: 120e6,
      get machine() {
        const L = '\u0100';
        return { input: 'āecho hello snarf >/dev/snarf; cat /dev/snarf\r' +
          'ācat /rom/lib/windows >/dev/snarf; cmp /rom/lib/windows /dev/snarf; echo cmp $status\r' +
          'ācat /rom/doc/api.md >/dev/snarf; wc -c /dev/snarf\r' + 'ā{echo echo one; echo echo two} >/dev/snarf\r' + 'ā\x1dy' +
          'āhylang\r' + 'ā(use "cons")\r' + 'ā(snarf! "from hylang")\r' + 'ā(snarf)\r' + 'ā(exit)\r' +
          'āecho -n ab >/dev/snarf\r' + 'āforth\r' + L + L + 'require facility.fl\r' + L +
          ': t s" /dev/consctl" w/o open-file throw >r s" keys vt" r@ write-file throw\r' + L +
          '  27 emit ." [?2004h" 14 0 do ekey . loop r> close-file throw ;\r' + L + 't\r' + L + '\x1dy' + L + L + 'bye\r' +
          'āecho done\r' };
      },
      expect: ['cat /dev/snarf\nhello snarf\n', 'echo cmp $status\ncmp\n', 'cat: write error: disk full\n   8192 /dev/snarf\n',
        '% echo one\none\n% echo two\ntwo\n', '(snarf)\n=> "from hylang"', '27 91 50 48 48 126 97 98 27 91 50 48 49 126  ok',
        'echo done\ndone\n%'],
    },
    {
      name: 'scrollview', what: 'the scrollback\'s view (W6b): Ctrl-] [ (its end), PgUp, the arrows, Space\'s mark and Enter\'s copy (the lines marked to the cursor\'s, into /dev/snarf); Enter alone (the cursor\'s line); Escape twice; Shift-PgUp (a page up) while a program writes on (its output there after: the view left with q), Home and End; its footer\'s %y on the serial port (default chrome serial on)',
      init: 't_rc', cycles: 200e6,
      // (30 lines and the command's two rows: the view a page up from its end, its top the command's first row; up
      // from the last row (line 22) to line 21, marked, down twice (the top a line down): lines 21-23 copied.  Then
      // the view at the end, up twice: the line two above the prompt's, line 22, copied alone)
      get machine() {
        const L = '\u0100', UP = '\x1b[A', DOWN = '\x1b[B', PGUP = '\x1b[5~';
        return { input: 'āfor(i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30) echo line $i\r' +
          'ā\x1d[' + L + PGUP + L + UP + L + ' ' + L + DOWN + L + DOWN + L + '\r' + 'ācat /dev/snarf\r' +
          'ā\x1d[' + L + UP + L + UP + L + '\r' + 'ācat /dev/snarf\r' + 'ā\x1d[' + L + '\x1b\x1b' +
          'āsleep 1; echo after the view\r' + '\x1b[5;2~' + L + '\x1b[H' + L + '\x1b[F' + L + L + 'q' +
          'āecho default chrome serial on >/dev/wctl\r' + 'ā\x1b[5;2~' + L + 'q' + 'āecho done\r' };
      },
      expect: ['% cat /dev/snarf\nline 21\nline 22\nline 23\n%', '% cat /dev/snarf\nline 22\n%', 'after the view\n%',
        ' 1-21/', '  Space: mark, Enter: copy, q: leave', 'echo done\ndone\n%'],
    },
    {
      name: 'history', what: 'a window\'s history (W6c: wctl\'s history N): 64 rows past its scrollback\'s, a hundred lines and more kept (/dev/text has them all: 105 lines, 64 without), the view\'s Home the oldest of them (copied into /dev/snarf, then a page down and back); history 0 (its rows gone: 64 lines again); history 200 refused',
      init: 't_rc', cycles: 200e6,
      get machine() {
        const L = '\u0100';
        return { input: 'āecho history 64 >/dev/wctl\r' + 'āfor(i in 1 2 3 4 5 6 7 8 9 10) for(j in 0 1 2 3 4 5 6 7 8 9) echo n $i$j\r' +
          'āgrep -c n /dev/text; wc -l /dev/text\r' + 'ā\x1d[' + L + '\x1b[H' + L + '\x1b[6~' + L + '\x1b[5~' + L + '\r' + 'ācat /dev/snarf\r' +
          'āecho history 0 >/dev/wctl; grep -c n /dev/text; wc -l /dev/text\r' + 'āecho history 200 >/dev/wctl\r' + 'āecho done\r' };
      },
      expect: ['wc -l /dev/text\n102\n    105 /dev/text\n', 'cat /dev/snarf\n% echo history 64 >/dev/wctl\n%',
        'wc -l /dev/text\n60\n     64 /dev/text\n', 'history 200 >/dev/wctl\necho: write error: invalid argument', 'echo done\ndone\n%'],
    },
    {
      name: 'tiles', what: 'tiles (W7a: wctl\'s layout): three windows of a group in rows (80 x 7 each on the serial port\'s 80 x 24, a header row each), columns (26 x 23, a border between), a grid (40 x 11, 39 x 11, the third 80 x 11), zoomed and back (Ctrl-] z: the focus alone, 80 x 24, the others keeping theirs), tabs; then rows again, the focus moved by Ctrl-] and the arrows and Ctrl-] Shift-Tab (back to window 0); the screen at the end: the three tiles, their headers (the focused one\'s reversed), each window\'s text in its tile',
      init: 't_rc', cycles: 300e6,
      // (Windows 1 and 2 held open by sleeps, a line written to each.  The sizes read from each window's consctl.  The
      // waits are a time's, not the prompt's: a tile painted again ends with its blanks)
      get machine() {
        const L = '\u0100', W = L + L + L;
        const size = 'grep size /dev/consctl; grep size \'#c1/consctl\'; grep size \'#c2/consctl\'\r';
        return { input: 'āecho b115200 >/dev/serctl\r' + 'āecho new >/dev/wctl; echo new >/dev/wctl\r' + 'āsleep 1000 >\'#c1/cons\' &\r' + 'āsleep 1000 >\'#c2/cons\' &\r' +
          'āecho one >\'#c1/cons\'; echo two >\'#c2/cons\'\r' + 'āecho layout rows >/dev/wctl\r' + W + size + W +
          'echo layout columns >/dev/wctl\r' + W + size + W + 'echo layout grid >/dev/wctl\r' + W + size + W +
          '\x1dz' + W + 'grep size /dev/consctl\r' + W + '\x1dz' + W + 'echo layout tabs >/dev/wctl\r' + W + 'grep size /dev/consctl\r' + W +
          'echo layout rows >/dev/wctl\r' + W + '\x1d\x1b[B' + W + '\x1d\x1b[Z' + W + '\x1d\x1b[C' + W + '\x1d\x1b[D' + W +
          'echo done\r' + W };
      },
      expect: ['size 80 7\n', 'size 26 23', 'size 40 11', 'size 39 11', 'size 80 11', 'done\n%'],
      check(m) {
        const f = [], out = m.out;
        const sizes = [...new Set(out.replace(/\x1b\[[0-9;]*[A-Za-z]/g, '\n').match(/size \d+ \d+/g) || [])];
        const want = ['size 80 7', 'size 26 23', 'size 40 11', 'size 39 11', 'size 80 11', 'size 80 24'];
        if (sizes.join(',') !== want.join(',')) f.push('the sizes read (each the first time): ' + sizes.join(', ') + '; not ' + want.join(', '));
        const t = new VT({ cols: 80, rows: 24 }).write(out);
        const lines = t.lines();
        const rev = r => t.screen[r][0].f & 16;
        if (!/^0 rc/.test(lines[0]) || !/^1/.test(lines[8]) || !/^2/.test(lines[16])) f.push('the headers at rows 1, 9, 17: ' + [lines[0], lines[8], lines[16]].join(' | '));
        if (!rev(0) || rev(8) || rev(16)) f.push('the focused tile\'s header (window 0\'s) reversed, the others not');
        if (!lines.slice(9, 16).some(l => l.startsWith('one'))) f.push('window 1\'s line (one) in its tile, rows 10-16');
        if (!lines.slice(17, 24).some(l => l.startsWith('two'))) f.push('window 2\'s line (two) in its tile, rows 18-24');
        if (!lines.slice(1, 8).some(l => l === 'done')) f.push('window 0\'s last line (done) in its tile, rows 2-8');
        return f;
      },
    },
    {
      name: 'tilesplit', what: 'splits (W7a): Ctrl-] s, a window with a shell (init\'s, HyForth) below in the group, rows; a Forth line there; Ctrl-] Up back to window 0, its wctl (two windows, each 80 x 11: the serial port\'s tiles, the smaller); Ctrl-] v, a third; the Vera X\'s screen read back: the bar, then window 0\'s tile\'s header',
      init: 'init', cycles: 600e6,
      get machine() {
        const L = '\u0100', W = L + L + L + L + L;
        return { vera: true, input: 'ā\x1ds' + W + W + '2 3 + .\r' + W + '\x1d\x1b[A' + W + 'cat /dev/wctl\r' + W + '\x1dv' + W + W +
          '\x1d\x1b[A\x1d\x1b[A' + W + 'head -2 /dev/vid/term | tail -1\r' + W };
      },
      expect: ['2 3 + .\n5 ', 'cat /dev/wctl\n0 0 80 11 *\n1 0 80 11\n', 'tail -1\n0 forth'],
    },
    {
      name: 'popups', what: 'popups (W7b: wctl\'s float X Y C R): window 1 floating at 20, 6 (30 x 8: its consctl\'s size), boxed, shown over window 0 as twelve lines are written there (painted around the box); Ctrl-] 0 and window 0 alone again; Ctrl-] ? (the keys, a popup: the bindings) and q; then the popup over the group\'s two tiles (rows: windows 0 and 2; window 1, floating, no tile), the screen at the end checked: the tiles\' headers, the box, the text in each',
      init: 't_rc', cycles: 300e6,
      get machine() {
        const L = '\u0100', W = L + L + L;
        return { input: 'āecho b115200 >/dev/serctl\r' + 'āecho new >/dev/wctl\r' + 'āsleep 1000 >\'#c1/cons\' &\r' +
          'āecho float 20 6 30 8 >\'#c1/wctl\'; echo popup text >\'#c1/cons\'\r' + 'āgrep size \'#c1/consctl\'\r' +
          'ā{sleep 2; for(i in 1 2 3 4 5 6 7 8 9 10 11 12) echo under the popup, a long line, $i} &\r' +
          'āecho current 1 >/dev/wctl\r' + W + W + W + '\x1d0' + W + 'echo keys\r' + W + '\x1d?' + W + 'q' + W +
          'echo new >/dev/wctl\r' + W + 'sleep 1000 >\'#c2/cons\' &\r' + W + 'echo two >\'#c2/cons\'; echo layout rows >/dev/wctl\r' + W +
          '{sleep 3; echo fin^ish >\'#c1/cons\'} &\r' + W + 'echo current 1 >/dev/wctl\r' + W + W + W };
      },
      expect: ['size 30 8', ' w', 'the windows', ' ?', 'these keys', 'finish'],
      check(m) {
        const f = [];
        const t = new VT({ cols: 80, rows: 24 }).write(m.out);
        const lines = t.lines();
        if (!/^0 rc/.test(lines[0]) || !/^2/.test(lines[12])) f.push('the tiles\' headers at rows 1 and 13: ' + lines[0] + ' | ' + lines[12]);
        if (lines[5].slice(19, 21) !== '+-' || lines[5][50] !== '+' || lines[14][19] !== '+' || lines[14][50] !== '+')
          f.push('the box from row 6, column 20 to row 15, column 51: ' + lines[5] + ' | ' + lines[14]);
        if (!lines[6].slice(20).startsWith('popup text') || lines[6][19] !== '|') f.push('the popup\'s text in its box: ' + lines[6]);
        if (!lines[7].slice(20).startsWith('finish')) f.push('the line written into it last: ' + lines[7]);
        if (!lines.slice(13, 24).some(l => l.startsWith('two'))) f.push('window 2\'s text in its tile, rows 14-24');
        return f;
      },
    },
    {
      name: 'seats', what: 'seats (W8a: consctl\'s seats): the screen and the serial port each a seat; Ctrl-] c at the keyboard, a group of its own (init\'s shell) shown on the screen alone, sized to it, the serial port still showing window 0 (both marked in wctl); the keyboard\'s keys to it (its text read back from the serial port), its Ctrl-C a note to its shell, not window 0\'s; consctl reads terminal seats; both, one seat again (the screen showing window 0)',
      init: 'init', cycles: 400e6,
      get machine() {
        const P = '\u0100', W = P + P + P, CRB = '\u033A\u021C\u03BA';   // (Ctrl down, ], Ctrl up: Ctrl-] at the keyboard)
        return { vera: true, smc: true, input: 'āecho seats >/dev/consctl\r' + 'āgrep terminal /dev/consctl\r' + 'ā' +
          '\u0102' + CRB + 'c' + W + W + W + W + '.( on the scr^een) cr\r' + W + ': spin begin again ;\r' + W + 'spin\r' + W + '\x03' + W + '\u0103' +
          W + 'cat /dev/wctl\r' + 'ācat \'#c1/text\'\r' + 'āhead -2 /dev/vid/term | tail -1\r' + 'āecho both >/dev/consctl\r' + 'ācat /dev/wctl\r' +
          'āhead -3 /dev/vid/term\r' + 'ā' };
      },
      expect: ['terminal seats', 'cat /dev/wctl\n0 0 80 24 *\n1 1 80 57 *\n', 'on the scr^een\n', 'spin\ninterrupt\n', 'tail -1\n1 forth',
        'cat /dev/wctl\n0 0 80 24 *\n1 1 80 24\n', 'head -3 /dev/vid/term\n 0 forth 1 forth', '\n0 forth'],
      check(m) {                                              // (The serial port not painted again till both: its window
        const o = m.out, a = o.indexOf('terminal seats'), b = o.indexOf('echo both');   //   the same throughout)
        return a < 0 || b < 0 || o.slice(a, b).includes('\x1b[2J') ? ['the serial port cleared while the keyboard\'s seat had its window'] : [];
      },
    },
    {
      name: 'winmouse', what: 'the mouse in the windows (W8b): the input controller\'s reports (kdev\'s) (CSI < B ; X ; Y M and m) to a program that asks for them (?1000, ?1006: HyForth\'s ekey, keys vt), at its own cell; a click in a tile not focused (Ctrl-] s\'s rows: window 0\'s, above) focuses it; the wheel over a window that doesn\'t ask, its scrollback\'s view (Enter: its cursor\'s line into /dev/snarf, the view left); the PC terminal\'s reports from the serial port (the console\'s ?1000 and ?1006 sent it as its window asks), its tile\'s cell (row 3, its header row 1: the window\'s 2)',
      init: 'init', cycles: 600e6,
      get machine() {
        const P = '\u0100', W = P + P + P, M = '\u0400';
        return { vera: true, smc: { moves: [[0, -160, 0], [0, 0, 1], [0, 0, 0], [0, 0, 1], [0, 0, 0], [0, 0, 0, -1]] },
          input: 'ārequire facility.fl\r' + 'ā: t s" /dev/consctl" w/o open-file throw >r s" keys vt" r@ write-file throw\r' +
            W + '  27 emit ." [?1000h" 27 emit ." [?1006h" 0 do ekey . loop\r' + W + '  27 emit ." [?1000l" r> close-file throw ;\r' +
            W + '20 t\r' + W + '\u0102' + M + P + M + P + M + P + '\u0103' + W + '\x1ds' + W + W + W + W + 'cat /dev/wctl\r' + W +
            '\u0102' + M + P + M + P + '\u0103' + W + 'cat /dev/wctl\r' + W + '\u0102' + M + P + '\u0103' + W + '\r' + W +
            'cat /dev/snarf\r' + W + '18 t\r' + W + '\x1b[<0;5;3M\x1b[<0;5;3m' + W };
      },
      // (The pointer from the screen's middle up 160: its row 10, the window's 9 below the bar and header; its column 41)
      expect: ['/> 20 t\n\x1b[?1000;1006h27 91 60 48 59 52 49 59 57 77 27 91 60 48 59 52 49 59 57 109 \x1b[?1000l\n',
        'cat /dev/wctl\n0 0 80 11\n1 0 80 11 *\n', 'cat /dev/wctl\n0 0 80 11 *\n1 0 80 11\n',
        '18 t\n\x1b[?1000;1006h', '27 91 60 48 59 53 59 50 77 27 91 60 48 59 53 59 50 109 \x1b[?1000l'],
      // (/dev/snarf's line: the boot's task table's cons line (its state 08: waiting for its own events, its
      // init's loop), its CPU column the boot's own, then the prompt)
      check(m) {
        const out = m.out.replace(/\r/g, ''), line = (out.match(/^F 08 01 FF [0-9A-F]{6} cons$/m) || [])[0];
        return line && out.includes(line + '\n/> ') ? [] : ['cat /dev/snarf: not the task table\'s cons line' + (line ? ' (' + line + ')' : '')];
      },
    },
    {
      name: 'pcm', what: 'the Vera X\'s PCM (vid\'s /pcm and /pcmctl), at rc: its files and state; the rate (the VERA\'s nearest) and volume; raw samples from a card into the FIFO, drained; bad commands; /pcm one task\'s (another\'s pcmctl: busy); WAV files played (8 bits mono, made signed; 16 bits stereo past an odd chunk; a float one, not a song); a ZSM\'s PCM instruments (one, then one looped, stopped by the FIFO emptied: from RAM) and its claim of the PCM; one too big for RAM (from the file); the FIFO\'s bytes in order, none lost, its runs dry only at the ends',
      init: 't_rc', cycles: 150e6,
      get machine() { return { input: typed(PCM_LINES), vera: { pcmLog: true }, sd: pcmCard() }; },
      get expect() { return expected(PCM_LINES); },
      // (The FIFO's bytes: the raw samples, the 8-bit WAV file's made signed, the 16-bit one's as they are, the first
      // instrument's, then the looped one's: whole, then its loop (bytes 100-599) again and again, a second's worth and
      // the FIFO's at most; then the big one's first, half a second's and the FIFO's.  It runs dry four times: at the
      // raw samples' end, each WAV file's, and the first instrument's; the FIFO emptied isn't one)
      check(m) {
        const f = [], log = m.vera.pcmLog, d = PCM_DATA;
        const want = Buffer.concat([d.tone, Buffer.from(d.t.map(b => b ^ 0x80)), d.s, d.i0]);
        const got = Buffer.from(log.slice(0, want.length));
        if (!got.equals(want)) {
          const at = [...want].findIndex((b, i) => got[i] !== b);
          f.push('the FIFO\'s bytes differ from byte ' + at + ' (of ' + want.length + ': the raw 6000, the WAV files\' 4000 and 2400, the first instrument\'s 1500)');
        }
        const rest = log.slice(want.length), i1 = d.i1, big = d.big;
        let k = 0;                                            // (The looped one's, till the big one's start)
        while (k < rest.length && rest[k] === (k < 600 ? i1[k] : i1[100 + (k - 600) % 500]) && !(k >= 600 && rest[k] === big[0] && rest[k + 1] === big[1] && rest[k + 2] === big[2])) k++;
        if (k < 3000 || k > 9000) f.push('the looped instrument: ' + k + ' bytes to the FIFO (3000-9000 wanted)');
        const tail = rest.slice(k);
        if (!Buffer.from(tail).equals(big.subarray(0, tail.length))) f.push('the big instrument\'s bytes (' + tail.length + ') aren\'t its first');
        if (tail.length < 1500 || tail.length > 7000) f.push('the big instrument: ' + tail.length + ' bytes to the FIFO (1500-7000 wanted)');
        if (m.vera.pcmLost) f.push(m.vera.pcmLost + ' bytes written to a full FIFO');
        if (m.vera.pcmUnderruns !== 4) f.push('the FIFO ran dry ' + m.vera.pcmUnderruns + ' times (4 wanted: the ends)');
        this.notes = ['the FIFO: ' + log.length + ' bytes taken, ' + m.vera.pcmOut + ' played; the looped instrument ' + k + ', the big one ' + tail.length + '; dry ' + m.vera.pcmUnderruns + ' times'];
        return f;
      },
    },
    {
      name: 'mouse', what: 'the mouse (VIDEO.md step 6): vid\'s /mouse, /mousein and /mousectl (its state; /mouse\'s first read at once, Plan 9\'s 49 bytes; a non-blocking read\'s E_AGAIN; moves, the pointer\'s sprite at them, kept on the screen; swap; the buttons\' changes queued and read in turn; the pointer off and on; a write to /mouse; bad lines; a claim and its release; mode 40x30\'s size), then the input controller (kdev\'s, on the emulator\'s SMC): a move, a click and the wheel from its PS/2 packets',
      init: 't_mouse', cycles: 45e6,
      // (The SMC's mouse: a move, then the left button pressed and let go, then the wheel up, each 2M cycles apart,
      // from 12M cycles on: by then kdev has asked for the mouse's mode and read it)
      machine: { vera: true, smc: { moves: [[30, 20, 0], [0, 0, 1], [0, 0, 0], [0, 0, 0, -1]] },
        input: '\u0102' + '\u0100'.repeat(6) + '\u0400\u0100\u0400\u0100\u0400\u0100\u0400\u0103' },
      check(m) {
        const s = m.smc, f = [];
        if (s.mouseId !== 3) f.push('the SMC\'s mouse in mode ' + s.mouseId + ' (3, a wheel\'s, asked for)');
        if (s.mouseLost) f.push(s.mouseLost + ' packets lost on the SMC');
        this.notes = ['the SMC: ' + s.reads + ' reads, ' + s.nacks + ' with nothing (unanswered); the I2C bus: ' + m.i2c.stats.starts + ' starts'];
        return f;
      },
    },
    {
      name: 'kbd', what: 'the keyboard (VIDEO.md step 6): the input controller (kdev\'s), the emulator\'s SMC typed at, the keys into the console (#c/kbin) as a PC terminal sends them, at the login shell (HyForth): Shift and punctuation; Left to edit a line; Up, its history; Caps Lock; the keypad\'s digits, and its cursor keys with Num Lock off; Ctrl-C, a note to the window\'s shell; the keyboard\'s LEDs following the locks',
      init: 'init', cycles: 120e6,
      get machine() {
        const tap = n => String.fromCharCode(0x200 + n), W = '\u0101';  // (smc.js's key n pressed and let go; a prompt)
        const LEFT = tap(79), UP = tap(83), CAPS = tap(30), NUM = tap(90), KP1 = tap(93), KP2 = tap(98), KP4 = tap(92);
        return { vera: true, smc: true, input: '\u0102' + W + '.( Hello, World!) cr\r' + W + '.( ac)' + LEFT + LEFT + 'b\r' + W + UP + '\r' +
          W + CAPS + '.( shout)' + CAPS + '\r' + W + '.( ' + KP1 + KP2 + ')\r' + W + '.( xz)' + NUM + KP4 + KP4 + 'y' + NUM + '\r' +
          W + ': spin begin again ;\r' + W + 'spin\r\u0100\x03' + W + '.( back)\r' + '\u0103' };
      },
      expect: ['/> .( Hello, World!) cr\nHello, World!\n/> ', '\nabc\n/> .( abc)\nabc\n/> .( SHOUT)\nSHOUT\n/> .( 12)\n12\n/> .( xz)',
        '\nxyz\n/> : spin begin again ;\n/> spin\ninterrupt\n/> .( back)\nback\n/> '],
      check(m) {
        const s = m.smc, f = [], leds = s.commands.filter(c => c[0] === 0xED).map(c => c[1]).join(' ');
        if (leds !== '2 6 2 0 2') f.push('the LEDs: ' + leds + ' (2 6 2 0 2 wanted: Num Lock, Caps Lock on and off, Num Lock off and on)');
        if (s.lost) f.push(s.lost + ' key codes lost on the SMC');
        if (s.keys.length) f.push(s.keys.length + ' key codes left unread');
        return f;
      },
    },
    {
      name: 'draw', what: 'the graphics words (VIDEO.md step 5): vid\'s /dev/vid/draw (pen, plot, line, box, bar, circle, disc, clear) on the bitmap at 8, 4, 2 and 1 bits a pixel (640 across), what falls off it, bad numbers, a bitmap too big (the console on the serial port alone meanwhile: a bitmap 320 across makes the screen\'s text 40 columns, and the windows with it); HyForth\'s lib video (the drawing, vpeek, the turtle, text, the palette, a sprite) and rc\'s lines to the file; hylang\'s video.hl (the drawing, vpeek, the turtle, text); the C SDK\'s shapes sample (cc65\'s TGI on hydra_tgi: a line, a bar, a circle, an ellipse and text, read back) and sketch (vera.h, drawing after the emulator\'s mouse till a key)',
      init: 'init', cycles: 260e6,
      // (The mouse for sketch: to the top left, onto the strip's colour 2 and clicked, then to (150, 120), pressed,
      // dragged 20 right and 20 down, let go; then a key, x, ends it, and a while after it (not a prompt: rc's may come
      // out before the keyboard's input has ended, as the test runner sees it), echo $status)
      get machine() {
        const W = '\u0101', M = '\u0400', P = '\u0100';
        return { vera: true, smc: { moves: [[-400, -400, 0], [50, 5, 0], [0, 0, 1], [0, 0, 0], [100, 115, 0], [0, 0, 1], [20, 0, 1], [0, 20, 1], [0, 0, 0]] },
          input: DRAW_FORTH.map(l => W + l[0] + '\r').join('') + W + 'hylang\r' + DRAW_HY.map(l => W + l[0] + '\r').join('') + W + 'exit\r' +
            W + '/sd/0/sample/c/shapes\r' + W + '/sd/0/sample/c/sketch\r' + '\u0102' + P + P + (M + P).repeat(9) + 'x\u0103' + P + 'echo $status\r' };
      },
      get expect() {
        return [DRAW_FORTH.map(l => '/> ' + l[0] + '\n' + (l[1] ? l[1] + '\n' : '')).join('') + '/> hylang\n',
          DRAW_HY.map(l => 'hylang> ' + l[0] + '\n=> ' + l[1] + '\n').join(''), '/> /sd/0/sample/c/shapes\n320x240, 256 colours: 4 4 2 14 11, text 165 dots\n/> /sd/0/sample/c/sketch\n/> echo $status\n0\n'];
      },
      check(m) {
        const v = m.vera.vram, f = [];
        if (v[0x1FA00 + 400] !== 0 || v[0x1FA00 + 401] !== 15) f.push('palette entry 200 isn\'t $F00 (red)');
        const sp = Array.from(v.subarray(0x1FC00 + 18, 0x1FC00 + 22)).join(' ');
        if (sp !== '100 0 50 0') f.push('sprite 2 at ' + sp + ' (100 0 50 0 wanted: 100, 50)');
        let picked = 0;                                       // (sketch's lines, in the strip's colour 2)
        for (let y = 10; y < 240; y++) for (let x = 0; x < 320; x++) if (v[y * 320 + x] === 2) picked++;
        if (picked < 40 || v[120 * 320 + 160] !== 2 || v[130 * 320 + 170] !== 2) f.push('sketch\'s drag: ' + picked + ' pixels in colour 2 (40 or more wanted, through (160, 120) and (170, 130))');
        if (m.smc.lost || m.smc.mouseLost) f.push('codes lost on the SMC');
        return f;
      },
      sdcard: true
    },
    {
      name: 'ramw', what: 'small writes (HydraFS on a RAM disk writes back only the part of a block a write changed; a card\'s block is kept back): a file of 70 writes, written over in its first block, across a block\'s end and at its end, on /ram, /sram and a card, read back; 100 writes of 16 bytes to /ram and to a card, a write\'s time; a card\'s block kept back (its file open: not on the card) and synced (on it)',
      init: 'init', cycles: 120e6,
      get machine() { this.card = ramwCard(); return { sd: [this.card], input: RAMW_LINES.map(l => '\u0101' + l + '\r').join('') + '\u0101' }; },
      expect: ['[ramw done.'],
      // (October 2026: 15,400, from 24,200 when a RAM disk wrote back the whole block)
      budgets: [{ what: 'a write of 16 bytes to /ram (HyForth\'s write-file, 100 of them: its request to the storage driver, HydraFS, the RAM disk)',
        from: '[rw A]', to: '[rw B]', minus: ['[b0 A]', '[b0 B]'], per: 100, max: 18000 },
        // (October 2026: 25,000 with a card's block kept back, from some 225,000 when each write wrote its block)
        { what: 'a write of 16 bytes to a card (the same: its block kept back, written as the next is wanted)',
          from: '[cw A]', to: '[cw B]', minus: ['[b0 A]', '[b0 B]'], per: 100, max: 40000 }],
      check(m, out) {
        const f = [], n = out.split(RAMW_TEXT).length - 1;
        if (n !== 3) f.push('the file as written, read back ' + n + ' times (3 wanted: /ram, /sram, the card)');
        for (const p of ['/ram/n', '/sd/0/n']) if (!new RegExp('-rw\\S*\\s.*\\b1600\\b.*' + p).test(out.replace(/\r/g, ''))) f.push(p + ' isn\'t 1600 bytes long');
        // (A write past a file's end: the file that long, with a hole.  A RAM disk's clusters aren't a card's 4K: its
        // file came out too long, and the write failed, end of file)
        for (const p of ['/ram/h', '/sram/h', '/sd/0/h']) if (!new RegExp('-rw\\S*\\s.*\\b3001\\b.*' + p).test(out.replace(/\r/g, ''))) f.push(p + ', written at 3000 past its 2 bytes, isn\'t 3001 bytes long');
        this.card.save();
        const raw = fs.readFileSync(this.card.file, 'latin1');
        if (!raw.includes('then synced')) f.push('the card: a block synced, not on it');
        if (raw.includes('second part: kept back')) f.push('the card: a block kept back (its file open, not synced) on it already');
        const v = new hydrafs.Volume(this.card.file), e = v.tryWalk('w'), got = e ? v.read(e).toString('latin1') : '';
        for (const p of v.check()) f.push('the card: ' + p);
        v.close();
        if (got !== RAMW_TEXT) f.push('the card\'s w: ' + got.length + ' bytes, not as written');
        return f;
      },
    },
    {
      name: 'vsd', what: 'the Vera X\'s SD card (the storage driver\'s disk v, on the VERA\'s own SPI controller: the emulator\'s card there), at the login shell: its ctl (an SDHC card, its HydraFS), a file read, one written, /sd listing it',
      init: 'init', cycles: 120e6,
      get machine() { this.card = veraCard(); return { vera: { sd: this.card }, input: VSD_LINES.map(l => '\u0101' + l[0] + '\r').join('') + '\u0101' }; },
      get expect() { return VSD_LINES.map(l => '/> ' + l[0] + '\n' + l[1] + '\n/> '); },
      check() {
        this.card.save();
        const v = new hydrafs.Volume(this.card.file);
        const e = v.tryWalk('new.txt'), got = e ? v.read(e).toString('latin1') : '';
        v.close();
        return got === 'written\n' ? [] : ['the card\'s new.txt: ' + JSON.stringify(got) + ' ("written\\n" wanted)'];
      },
    },
    {
      name: 'psg', what: 'the Vera X\'s PSG as sound channels 8-23 (snd, through vid\'s /psg), at rc: sndctl\'s state (24 channels); a note (its frequency word), a waveform by name, by number and as a patch, speakers, a level, a frequency, a bend, a note off; claims of the PSG\'s channels (sndctl\'s second mask); the master volume on their volumes; errors; /psg and /dev/vid/psg; a note while the VERA\'s claimed, on the chip as the claim ends; hylang\'s and HyForth\'s snd-wave and claims of the PSG\'s channels; a ZSM of PSG writes played (play: its PSG channels claimed, its voices in time)',
      init: 't_rc', cycles: 180e6,
      get machine() { return { input: typed(PSG_LINES), vera: true, sd: psgCard() }; },
      get expect() {
        const claim = PSG_LINES.findIndex(l => l[2]);
        return expected(PSG_LINES.slice(0, claim)).concat(['% ' + PSG_LINES[claim][0] + '\nclaimed '], expected(PSG_LINES.slice(claim + 1)));
      },
      // (Each voice's four registers on the chip: its frequency word (Hz * 2^17 / 48,828.125), its speakers and
      // volume (63 less its attenuation * 1.5: level 64's 16 TL steps, the master volume 50's 17), its waveform and
      // width.  0: A4, the song's (bent down a semitone before, then off as its claim ended), off; 1: C4 on a sawtooth,
      // the song's too, off as it ended; 2: C5
      // on the left, at level 64; 3: 1000 Hz (B5 and 13 64ths); 4: a triangle of width 31; 5: patch 3, noise; 6: A4,
      // played while the chip was claimed; 7: hylang's A4 on a triangle of width 20; 8: HyForth's C4 on a sawtooth of
      // width 40)
      voices: [[1181, 0xC0, 0x3F], [702, 0xC0, 0x7F], [1405, 0x40 | 14, 0x3F], [2683, 0xC0 | 38, 0x3F], [0, 0xC0, 0x9F], [0, 0xC0, 0xFF], [1181, 0xC0 | 38, 0x3F], [1181, 0xC0 | 38, 0x94], [702, 0xC0 | 38, 0x68]],
      check(m) {
        const f = [], p = m.vera.psg, hx = v => '$' + v.toString(16).toUpperCase();
        this.notes = ['the PSG\'s voices on: ' + m.vera.psgOns.join(', ')];
        this.voices.forEach(([word, vol, wave], v) => {
          const w = p[v * 4] | p[v * 4 + 1] << 8;
          if (w !== word) f.push('voice ' + v + ': frequency word ' + w + ', not ' + word);
          if (p[v * 4 + 2] !== vol) f.push('voice ' + v + ': speakers and volume ' + hx(p[v * 4 + 2]) + ', not ' + hx(vol));
          if (p[v * 4 + 3] !== wave) f.push('voice ' + v + ': waveform and width ' + hx(p[v * 4 + 3]) + ', not ' + hx(wave));
        });
        for (let v = this.voices.length; v < 16; v++) if (p[v * 4 + 2] & 0x3F) f.push('voice ' + v + ': its volume ' + (p[v * 4 + 2] & 0x3F) + ' (none played)');
        // (The song's voices on: its voice 1 30 ticks after its voice 0, within two system ticks)
        const mult = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const at = v => { const o = m.vera.psgOns.filter(k => k.startsWith('voice ' + v + ' ')).pop(); return o ? +o.match(/at cycle (\d+)/)[1] : NaN; };
        const gap = (at(1) - at(0)) / (3579545 * mult / 60);
        if (!(Math.abs(gap - 30) <= 2 * 60 / 200)) f.push('the song: its voice 1 on ' + gap.toFixed(2) + ' ticks after its voice 0 (30 wanted)');
        this.notes.push('the song: its voice 1 on ' + gap.toFixed(2) + ' song ticks after its voice 0 (30)');
        return f;
      },
    },
    {
      name: 'psgmml', what: 'the PSG in scores (play\'s, mml.inc, and hysong.js\'s: channels I-X, sound channels 8-23): play -o\'s ZSMs of the ROM disk\'s songs/vera.mml (both chips; each waveform; envelopes; slides, legato, triplets, ties; I, y, k, D, v, q, p) and of two scores of edge cases, each the same as hysong.js\'s byte for byte; the errors (the other chip\'s instrument, both ways; x; y past 63; instruments it can\'t read; channel 23\'s number); vera.mml played on the Vera X (its voices\' starts as many as its ZSM\'s, the lead\'s first 751 song ticks after the hats\'); a line (-m 8), the X16\'s (-x: I as the waveform register, V), a chord (-c 13), one with more notes than the PSG\'s channels left',
      init: 't_rc', cycles: 600e6,
      get machine() { return { input: typed(PSG_MML_LINES), vera: true, sd: psgScoreCard() }; },
      get expect() { return expected(PSG_MML_LINES); },
      // (October 2026: 36M, from 129M when play -o wrote what each step of the score made, a byte or two at a time)
      budgets: [{ what: 'play -o of songs/vera.mml into /ram (10.5K, 256 bytes a write)', from: '[po A]', to: '[po B]', minus: ['[b0 A]', '[b0 B]'],
        per: 1, max: 45e6 }],
      // (Each voice's registers at the end (its frequency word, its speakers and volume, its waveform; null: any).
      // 0: -m 8's E4 (MIDI 64), on the song's lead's pulse of width 24, off; 1: -x's C5 on its triangle of width 0
      // (I128), off; 2, 3, 4: the song's hats (noise), pad (saw) and chirp (a square), off; 5-7: the chord's C4 E4 G4,
      // off; 12: the chirp's y 51,191 (its waveform register))
      voices: [[64, 0xC0, 0x18], [72, 0xC0, 0x80], [null, 0xC0, 0xFF], [null, 0xC0, 0x7F], [null, 0xC0, 0x3F], [60, 0xC0, null], [64, 0xC0, null], [67, 0xC0, null],
        [null, null, null], [null, null, null], [null, null, null], [null, null, null], [null, null, 0xBF]],
      check(m) {
        const f = [], p = m.vera.psg, hx = v => '$' + v.toString(16).toUpperCase();
        const { psgWord } = require('../sim/tools/hysong.js');
        this.voices.forEach(([note, vol, wave], v) => {
          const w = p[v * 4] | p[v * 4 + 1] << 8;
          if (note !== null && w !== psgWord(note * 64)) f.push('voice ' + v + ': frequency word ' + w + ', not ' + psgWord(note * 64));
          if (vol !== null && p[v * 4 + 2] !== vol) f.push('voice ' + v + ': speakers and volume ' + hx(p[v * 4 + 2]) + ', not ' + hx(vol));
          if (wave !== null && p[v * 4 + 3] !== wave) f.push('voice ' + v + ': waveform and width ' + hx(p[v * 4 + 3]) + ', not ' + hx(wave));
        });
        for (let v = 0; v < 16; v++) if (p[v * 4 + 2] & 0x3F) f.push('voice ' + v + ': its volume ' + (p[v * 4 + 2] & 0x3F) + ' at the end');
        // (The song's voices' starts: the ZSM's, and the lines' after it (-m's 3 on voice 0, -x's 1 on voice 1, the
        // chord's on 5-7); the lead's first (voice 0) 751 song ticks after the hats' (voice 2), within two system ticks)
        const z = zsmPsgOns(fs.readFileSync(path.join(CARD_DIR, 'psgmml-v.zsm'))), ons = new Array(16).fill(0), at = new Array(16).fill(-1);
        for (const o of m.vera.psgOns) { const [, v, c] = o.match(/voice (\d+) at cycle (\d+)/).map(Number); ons[v]++; if (at[v] < 0) at[v] = c; }
        const lines = [3, 1, 0, 0, 0, 1, 1, 1];
        for (let v = 0; v < 16; v++) if (ons[v] !== z.n[v] + (lines[v] || 0)) f.push('voice ' + v + ': ' + ons[v] + ' starts, not ' + (z.n[v] + (lines[v] || 0)));
        const mult = JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const gap = (at[0] - at[2]) / (3579545 * mult / 200);
        if (!(Math.abs(gap - (z.first[0] - z.first[2])) <= 2 * 200 / 200)) f.push('the song: its lead on ' + gap.toFixed(2) + ' ticks after its hats (' + (z.first[0] - z.first[2]) + ' wanted)');
        this.notes = ['the song: ' + z.n.slice(0, 5).join(', ') + ' starts on voices 0-4; its lead on ' + gap.toFixed(2) + ' song ticks after its hats (' + (z.first[0] - z.first[2]) + ')'];
        return f;
      },
    },
  ],
};

module.exports.dq0Check = dq0Check;
module.exports.sdcardOf = sdcardOf;                 // (sim/tools/soak.js's games: the SD card's in device 0)

// The tests that run a sample, or play a song of the SD card's: its image in SD device 0
for (const t of module.exports.tests) if (t.sdcard) sdcardOf(t);
