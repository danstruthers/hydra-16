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
// Every test also checks the longest IRQs-off stretch after the boot (IRQ_OFF_MAX).
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { createXmodemPeer } = require('../sim/lib/xmpeer.js');

const IRQ_OFF_MAX = 200;                                      // (docs/reimplementation-from-scratch.md, §8: 115200)
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
const CARD_DIR = path.join(__dirname, '..', 'obj', 'cards'), OLD_CARDS = path.join(__dirname, '..', '..', 'sim', 'cards');
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

// The rc test's lines, and what each says
const RC_LINES = [
  ["echo hello","hello"],
  ["x=(a b c); echo $x $#x $x(2)","a b c 3 b"],
  ["echo 'a  b' 'it''s'","a  b it's"],
  ["echo $\"x","a b c"],
  ["echo x^$x $x^1 $x^$x","xa xb xc a1 b1 c1 aa bb cc"],
  ["y=1 echo $y; echo $#y","1\n0"],
  ["echo $x(2-) $x(1-2)","b c a b"],
  ["echo one >/ram/f; echo two >>/ram/f; cat /ram/f","one\ntwo"],
  ["cat </ram/f >[2=1]","one\ntwo"],
  ["{echo b >[1=3]} >[3]/ram/y >/dev/null; cat /ram/y","b"],
  ["echo piped | cat","piped"],
  ["echo a b | cat | cat","a b"],
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
  ["echo /rom/lib/*","/rom/lib/forth /rom/lib/namespace /rom/lib/profile"],
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
  ["bind '#n' /mnt; ls /mnt","null\nzero"],
  ["ls /rom/lib","forth/\nnamespace\nprofile"],
  ["cat /bin/echo >/ram/hi; cd /ram; hi from dot; cd","from dot"],
  ["cat /nothing >[2]/ram/e; cat /ram/e","cat: /nothing: not found"],
  ["cat /nothing |[2] cat >/ram/p; echo -n 'p: '; cat /ram/p","p: cat: /nothing: not found"],
  ["echo $task $#path $path # a comment","2 2 . /bin"],
  ["path=(); ls; path=(. /bin); ls /rom/lib","rc: ls: not found\nforth/\nnamespace\nprofile"],
  ["! ~ a b && echo not; echo $status","not\n"],
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
  ["df","disk  kind   size        free        label\nx     rom    ",true],
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
    "  2    program  init",
    "  3    program  hello",
    "  4    boot     cons",
    "  5- 6 boot     storage",
    "",
  ].join('\n'), true],
  ["free","ram     256 KB a task (2 modules)\nshared  1024 KB, 256 KB in segments (1), 768 KB free"],
  ["sleep 30 & sleep 30 & kill $apid; slay sleep; wait; ps","task  state",true],
  ["kill 9; kill x; echo $status","kill: 9: no such task\nkill: x: invalid argument\n1"],
  ["sleep 1; echo slept","slept"],
  ["ls /rom/bin; whatis mkfs","fsck\ngrep\nlabel\nmkfs\nscom\nsort\n/bin/mkfs"],
  ["label s; label s Shared Disk; label s","SRAM\nShared Disk"],
  ["fsck s","hydrafs label=Shared Disk\nfree 244 KB of 252 KB\ncheck: lost 0, unmarked 0, twice 0"],
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
  ["/rom/sample/hi Ann Bob","Hello, Ann!\nHello, Bob!\nI'm task 3, in /, in window 0."],
  ["echo Hello there | /rom/sample/upper","HELLO THERE"],
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
const C_LINES = [
  ["/rom/sample/c/hello world","hello from C, world"],
  ["echo hello there | /rom/sample/c/upper","HELLO THERE"],
  ["/rom/sample/c/code 3; echo $status","3"],
  ["/rom/sample/c/code oops; echo $status","oops"],
  ["cd /ram; /rom/sample/c/ctest a 'b c'","ok - arguments",true],
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

// The sound test's lines (as the tools test's): #a's files, the volume, claims (one another program holds), its
// errors, the shadow, the C sample tones, and the bell
const SND_LINES = [
  ["ls /dev | grep snd; cat /dev/sndctl","snd\nsndctl\nvolume 100\nclaimed"],
  ["echo volume 150 >/dev/sndctl; cat /dev/sndctl; echo volume 100 >/dev/sndctl","volume 150\nclaimed"],
  ["{echo claim 5 >[1=3]; cat /dev/sndctl} >[3]/dev/sndctl; cat /dev/sndctl","volume 100\nclaimed 0 2\nvolume 100\nclaimed"],
  ["echo frob >/dev/sndctl; echo claim >/dev/sndctl", [
    "echo: write error: invalid argument",
    "echo: write error: invalid argument",
  ].join('\n')],
  ["wc -c /dev/snd","    256 /dev/snd"],
  ["/rom/sample/c/tones 0 & sleep 1; echo claim 1 >/dev/sndctl; wait","echo: write error: busy\ntones: patch 0, $20 C4, $28 4C"],
  ["echo x >/dev/bell",null],
];

// The song player's test lines (as the tools test's): play's errors; a file run by its name that isn't a program
// (no #!); a song from a card (PLAY_SONG, at 60 Hz: its key-ons timed in check) with its channels claimed while it
// plays and given back when it's stopped; scom, a song that runs by its name (an rc script); the C sample jukebox
// (snd_play)
const PLAY_LINES = [
  ["play; echo $status","usage: play [-l] song [n]\nusage"],
  ["/rom/README; whatis scom","rc: /rom/README: not a program\n/bin/scom"],
  ["play /rom/README; echo $status","play: /rom/README: not a song\nnot a song"],
  ["play /rom/nosuch; echo $status","play: /rom/nosuch: not found\n1"],
  ["play /sd/0/t.zsm & sleep 4; cat /dev/sndctl; kill $apid; wait; cat /dev/sndctl", [
    "volume 100",
    "claimed 0 1 2 3 4 5",
    "volume 100",
    "claimed",
  ].join('\n')],
  ["scom & sleep 1; cat /dev/sndctl; slay play; wait; cat /dev/sndctl","volume 100\nclaimed 0 1\nvolume 100\nclaimed"],
  ["/rom/sample/c/jukebox /rom/songs/scom.zsm 2","2\n1\nstopped: 137"],
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

// The forth test's card: the Forth 2012 test suite's files (tests/forth), each as itself; run.fs and run2.fs, which
// INCLUDE them in the suite's own order (runtests.fth's, those of the word sets HyForth has) in two sessions (the
// dictionary hasn't room for them all), each REQUIRing the libraries (.fl) its word sets are beyond startup.fs's
// (filetest.fth uses String's /STRING and coreexttest.fth's SI_INC; toolstest.fth, the Search-Order words), run.fs
// with a line for core.fr's ACCEPT test after it (stdin's next line); bad.fs, a file with an error in it; and args.fs,
// a script (#!/bin/forth: its arguments, the Hydra library, the constants library, a library of its own)
const FORTH_RUNS = [['prelimtest.fth', 'tester.fr', 'core.fr', 'coreplustest.fth', 'utilities.fth', 'errorreport.fth',
  'coreexttest.fth', 'exceptiontest.fth', 'string.fl', 'filetest.fth'],
  ['facility.fl', 'tools.fl', 'search.fl', 'string.fl', 'double.fl', 'tester.fr', 'utilities.fth', 'errorreport.fth',
  'facilitytest.fth', 'toolstest.fth', 'searchordertest.fth', 'stringtest.fth']];
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
  v.close();
  return [imageCard(0, f, 16384)];
}

// The lshell test's card: /lib/shell, HyForth as the shell
function shellCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'shell0.img');
  hydrafs.mkfs(f, 8, 'SHELL', undefined, true);
  const v = new hydrafs.Volume(f);
  v.mkdir('lib');
  v.put('lib/shell', Buffer.from('/bin/forth -l\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

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

module.exports = {
  IRQ_OFF_MAX,
  tests: [
    {
      name: 'boot', what: 'the kernel boots, POST finds nothing wrong; init runs hello and waits for it',
      init: 'init', cycles: 20e6,
      expect: ['Hydra-16 reborn: kernel 0.1, ABI 1', 'POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0',
        'RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000', 'POST ok', 'RAM modules: 02',
        'task F: cons', 'task 1: init', 'init: up in task 01', 'hello, from init', 'init: hello ended: code $07 (bye)'],
    },
    {
      name: 'init', what: 'init from files: the RAM disks started, the namespace file run, each shell\'s own namespace and /ram (a window\'s too)',
      init: 'init', modules: ['t_child'], cycles: 250e6,
      // (ā: wait for a prompt; '#fr' quoted, as # starts a comment; \x1d c: Ctrl-] c, a window made, wstart's rc
      // started there.  /bin: the RAM disks' caches (empty), then #m/bin, whose t_child runs by its name.  t_child f
      // makes /ram/mark: in window 0's rc's area, 2, and not in window 1's rc's, 4 (wstart, 3, has none))
      machine: { input: 'āls \'#fr\'\r' + 'āls /ram\r' + 'āls /bin\r' + 'āt_child f\r' + 'āls \'#fr\'/2\r' + 'ācat /rom/lib/profile\r' +
        'ācat /dev/sd/s/ctl\r' + 'āecho $window\r' + 'ā\x1dc' + 'āecho $window\r' + 'āls \'#fr\'\r' + 'āls /ram\r' + 'āls /dev\r' },
      expect: ['% ls \'#fr\'\n1/\n2/\n%', '% ls /ram\nbin/\nlib/\n%',
        '% ls /bin\nfsck\ngrep\nlabel\nmkfs\nscom\nsort\ninit\nhello\nrc\nwstart\n', 't_child\n% t_child f\n', '% ls \'#fr\'/2\nbin/\nlib/\nmark\n%',
        'prompt=(', '% cat /dev/sd/s/ctl\nsram 512 KB 1024 blocks\nhydrafs label=SRAM\n', '% echo $window\n0\n%',
        '% echo $window\n1\n%', '% ls \'#fr\'\n1/\n2/\n4/\n%', '% ls /ram\nbin/\nlib/\n%', '\ncons\nconsctl\nwctl\nwnew\nser\nserctl\nkbdin\n%'],
    },
    {
      name: 'newns', what: 'the default namespace\'s library (nslib): an old area emptied, a namespace file run (quotes, comments, $task, flags, bad lines)',
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
      name: 'task', what: 'tasks and the scheduler: SPAWN, EXITS, WAIT, SLEEP, preemption, PAUSE and WAKE, orphans',
      init: 't_task', modules: ['t_child'], without: ['cons', 'storage', 'snd', 'gpio'], cycles: 60e6,
    },
    {
      name: 'note', what: 'notes: the defaults, handlers, a note to oneself, WAIT ended by one, note groups',
      init: 't_note', modules: ['t_child'], cycles: 40e6,
    },
    {
      name: 'file', what: 'files and servers: OPEN, READ, WRITE, SEEK, STAT, DUP; text, ctl, data, directories; waiting',
      init: 't_file', modules: ['t_child', 't_srv'], cycles: 40e6,
    },
    {
      name: 'ns', what: 'namespaces: BIND, MOUNT, UNMOUNT, unions and union directories, CHDIR, clean names, inheritance',
      init: 't_ns', modules: ['t_child', 't_srv'], cycles: 40e6,
    },
    {
      name: 'dev', what: 'the kernel\'s devices (kdev): #/, #n, #t, #m, #p; pipes; a union keeping what was there',
      init: 't_dev', modules: ['t_child'], cycles: 40e6,
    },
    {
      name: 'proc', what: '/proc/N\'s mem (its RAM, bank, ROMs, the I/O area), ram (its banks), regs, env and note; the kernel task\'s and a driver\'s refused',
      init: 't_proc', modules: ['t_child'], cycles: 40e6,
    },
    {
      name: 'forth', what: 'HyForth (Forth 2012): the test suite (Core, Core Extension, Exception, Facility, File Access, Programming-Tools, Search-Order, String) in two sessions, its files INCLUDED from a card, the word sets\' libraries REQUIREd from /lib/forth; scripts (forth file.fs, #!/bin/forth: arguments, REQUIRE from /lib/forth, a library, an error, a pipeline); at the console: startup.fs\'s Programming-Tools (.S), libraries REQUIREd (and again after a MARKER), a definition, KEY? and KEY, errors (a file\'s, the system\'s), SH, RUN, a sys- word, a bank, the constants library, Ctrl-C, BYE',
      init: 't_rc', cycles: 900e6,
      // (The console's lines: each a moment after the last, as forth's prompt is its ok; w waits for a key, z, in raw
      // mode, not echoed, and the line after it is cooked again; l loops till Ctrl-C, which rc gets too: its prompt
      // on a new line after forth ends.  hydra.fs is 171 lines compiled, three searches of the dictionary each, so
      // the line after it waits long enough: the window keeps 64 keys typed ahead, and that line is longer)
      get machine() {
        return { sd: forthCard(), input: 'ācd /sd/0; forth <run.fs; forth <run2.fs; echo $status\r' +
          'āforth args.fs a b; echo $status\r' + 'ā./args.fs x; echo $status\r' + 'āforth bad.fs; echo $status\r' +
          'āforth args.fs a b | wc\r' +
          'āforth\rĀ1 2 .s 2drop\rĀrequire facility.fl require hydra.fl\rĀĀĀ: sq dup * ; 7 sq .\rĀ' + 'key? . cr\rĀ' + ': w begin key? until key ; w\rĀzĀ' + 'emit cr 1 2 + .\rĀ' +
          '1 0 /\rĀ' + 'foo\rĀ' + 'include bad.fs\rĀ' + 's" none.fs" included\rĀ' + 's" echo hi" sh .\rĀ' +
          's" echo there" run .\rĀ' + 's" /none" >z pad sys-stat .\rĀ' + '1 sys-banks-alloc throw bank! 1234 bank-window ! bank-window @ .\rĀ' +
          'require hydra.fs O_RDWR . E_NOENT .\rĀĀĀĀĀĀĀĀĀĀĀĀ' + 'marker m require double.fl m require double.fl -5 s>d dabs drop .\rĀĀ' +
          ': l begin again ; l\rĀ\u0003Ā' + '-5 3 mod . bye\r' + 'āecho $status\r' };
      },
      expect: ['0 tests failed out of 57 additional tests', 'End of Core word set tests', 'End of additional Core tests',
        'End of Core Extension word tests', 'End of Exception word tests', 'End of Facility word tests',
        'End of File-Access word set tests', 'End of Programming Tools word tests', 'End of Search Order word tests',
        'End of String word tests', forthReport('Core', 'Core extension', 'Exception', 'File-access'),
        forthReport('Core', 'Facility', 'Programming-tools', 'Search-order', 'String'),
        '% forth args.fs a b; echo $status\n3 args.fs a b\n2 \n42 \n\n%', '% ./args.fs x; echo $status\n2 ./args.fs x \n2 \n42 \n\n%',
        '% forth bad.fs; echo $status\n1 bad.fs:3: foo ?\n1\n%', '% forth args.fs a b | wc\n      3       6      21\n%',
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
      name: 'hyforth', what: 'HyForth\'s additions (docs/hyforth.md): names in lower case; words (each word\'s xt, and whether it\'s a literal, immediate, assembly or Forth); the libraries loaded (libs), one not searched (-lib) and searched again (lib, where it was), the one with lib refused, a .fs one, one a MARKER takes out; disasm (the modes, the Rockwell opcodes, a jsr to a word), see of a code word (with disasm.fl, and without), sys, the bit words, random\'s numbers; the terminal\'s sequences, form, ekey and the keys (an arrow key, a character); the sound words (notes on the YM2151, a claim, the volume); ctl (and its error)',
      init: 't_rc', cycles: 150e6,
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
          's" cat /dev/sndctl" sh drop\rĀĀ' + 's" /dev/sndctl" s" volume 100" ctl s" /dev/sndctl" s" frob" ctl\rĀĀ' +
          'lib greet words\rĀĀĀĀĀĀ' + 'bye\r',
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
        's" cat /dev/sndctl" sh drop\nvolume 150\nclaimed 0 2\n ok\n', 's" frob" ctl\n/dev/sndctl: invalid argument\n',
        'lib greet words\n ', 'bye\n'],
      check(m, out) {
        const f = [], first = out.split('libs\n')[0], last = out.slice(out.lastIndexOf('lib greet words'));
        // (disasm: a Rockwell branch to itself, the indirect and indexed modes, a jsr to a word; see of a code word)
        if (!/^ ([0-9A-F]{4})  0F 12 FD  bbr0 \$12, \$\1\n [0-9A-F]{4}  B2 22     lda \(\$22\)\n [0-9A-F]{4}  7C 34 12  jmp \(\$1234,x\)\n [0-9A-F]{4}  B1 10     lda \(\$10\),y\n [0-9A-F]{4}  A1 10     lda \(\$10,x\)\n [0-9A-F]{4}  BE 00 80  ldx \$8000,y\n [0-9A-F]{4}  B6 10     ldx \$10,y\n [0-9A-F]{4}  87 20     smb0 \$20\n [0-9A-F]{4}  0A        asl\n [0-9A-F]{4}  CB        wai\n [0-9A-F]{4}  20 [0-9A-F]{2} [0-9A-F]{2}  jsr \$[0-9A-F]{4}  \\ dup \n/m.test(out))
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
      name: 'fshell', what: 'HyForth as a shell (forth -l, shell.fl): its namespace and profile; a line Forth\'s or rc\'s by its first word (a number, a word, a pipeline, a redirection), or rc\'s by % (a program a word shadows); cd and the prompt (its format); a definition over lines (the second prompt); status and $status; & ($apid) and wait; programs as values: sh-out, output-of, a word\'s output a program\'s input (|, piped: one that ends first, Ctrl-C), spawn; Ctrl-C ending a program; errors, a usage; -lib shell and lib shell; exit',
      init: 't_rc', cycles: 400e6,
      // (Each line typed at the shell's prompt (ā: "> " or "% "), but those it has none for: a definition's second line
      // (its prompt a tab), and the lines after -lib shell, a moment after the one before (Ā).  cat, waiting for input,
      // stopped by Ctrl-C; and a word that loops, its output into cat, stopped by Ctrl-C.  head -c: a usage, so head
      // ends before the word's output has: forth goes on)
      machine: {
        input: 'āecho b115200 >/dev/serctl\r' + 'āforth -l\r' + 'ā2 3 + .\r' + 'āls /ram\r' + 'ācd /rom/lib/forth\r' + 'āpwd\r' +
          'āls startup.fs profile.fs | wc -l\r' + 'ā: twice\rĀ2 * ;\r' + 'ā3 twice .\r' + 'ācmp startup.fs profile.fs >/dev/null\r' +
          'āstatus .\r' + 'āecho $status\r' + 'ās" [%p] %% " prompt\r' + 'āsleep 1 &\r' + 'ās" apid" getenv evaluate wait status .\r' +
          'ā: free 1 ;\r' + 'āfree .\r' + 'ā% free\r' + 'ās" ls startup.fs" sh-out type status .\r' + 'ās" exit 3" sh-out nip . status .\r' +
          'ā: hi ." hello there" cr ;\r' + 'ā\' hi | wc -w\r' + 'ā\' hi s" wc -c" piped status .\r' + 'ā\' hi output-of type\r' +
          'ā: lots 300 0 do i . loop ;\r' + 'ā\' lots | head -c 20\r' + 'ā\' lots output-of nip .\r' + 'ās" sleep 1" spawn wait status .\r' +
          'ā: forever begin 1 . again ;\r' + 'ā\' forever | cat >/dev/null\rĀĀ\x03' +
          'ācat\rĀ\x03' + 'āecho $status\r' + 'ācd /none\r' + 'ābind -x a b\r' + 'ānosuch\r' + 'ā-lib shell\rĀ' + 'ls\rĀ' +
          'lib shell\r' + 'āecho back\r' + 'āexit\r' + 'āecho $status\r',
      },
      expect: ['% forth -l\nHyForth (Forth 2012), bye to end\n/> 2 3 + .\n5 \n/> ls /ram\nbin/\nlib/\n/> cd /rom/lib/forth\n' +
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
      name: 'lshell', what: 'the shell /lib/shell names (a card\'s: /bin/forth -l): init\'s in window 0, wstart\'s in a window made (Ctrl-] c: $window); send, a line typed in another window (#cN/kbdin), run there',
      init: 'init', cycles: 300e6,
      // (Window 1 made and shown (\x1d c), its shell sends window 0 a line; window 0 shown again (\x1d 0): its text,
      // the line run there)
      get machine() {
        return { sd: shellCard(), input: 'ā2 3 + .\r' + 'āecho $window\r' + 'ā\x1dc' + 'āecho $window\r' + 'āsend 0 echo hi from 1\r' +
          'ā\x1d0' + 'āecho back in 0\r' };
      },
      expect: ['HyForth (Forth 2012), bye to end\n/> 2 3 + .\n5 \n/> echo $window\n\n/> ', '/> echo $window\n1\n/> send 0 echo hi from 1\n/> ',
        '/> echo hi from 1\nhi from 1\n/> echo back in 0\nback in 0\n/> '],
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
      init: 't_rom', cycles: 200e6,
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
        { what: 'SPAWN of a module in place (#m/t_child), the caller\'s time', from: '<msp', to: 'msp>', minus: ['<b0', 'b0>'],
          per: 1, max: 45000 },
        { what: 'the same by /bin/t_child (the card\'s bin first, then #m/bin)', from: '<sp', to: 'sp>', minus: ['<b0', 'b0>'], per: 1,
          max: 110000 }];
      },
    },
    {
      name: 'env', what: 'environments: ENV_GET, ENV_PUT, ENV_DEL, ENV_NAME, a child\'s copy, #e (/env) as files',
      init: 't_env', modules: ['t_child'], cycles: 40e6,
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
            minus: ['<b0', 'b0>'], per: n, max: 25000 }];
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
          'āecho $status\r' + 'ācat /ram/n /ram/n /ram/n | more\rĀĀ\r' + 'ā/rom/sample/tick\rĀĀĀĀĀ\x03' + 'āecho $status\r' };
      },
      get expect() {
        return [...TOOL_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')),
          '\x1b[H\x1b[2Jtask  state    cpu  name\n', '% echo $status\ninterrupt\n%',
          '\n9\n10\n--more--\n11\n12\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n%', '% /rom/sample/tick\n.', ' seconds\n\n% echo $status\n\n%'];
      },
    },
    {
      name: 'c', what: 'the C target (cc65): its samples at rc, the library\'s test (ctest), conio\'s raw keys (and raw ended with the program)',
      init: 't_rc', cycles: 150e6,
      // (Each line typed at its prompt, as the tools test's.  Then keys: three keys and q; and again, ended by Ctrl-C,
      // its window cooked again for rc)
      get machine() {
        return { input: C_LINES.map(l => 'ā' + l[0] + '\r').join('') + 'ā/rom/sample/c/keys\rĀĀab\x1b[Aq' +
          'ā/rom/sample/c/keys\rĀĀ\x03' + 'āecho $status\r' };
      },
      get expect() {
        return [...C_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')),
          '\nctest: 0 failed\n%', 'codes:\x1b[27m 61 62 80\nended at 15,2\n%', '% echo $status\ninterrupt\n%'];
      },
    },
    {
      name: 'edit', what: 'edit, the line editor: a file made, printed, changed and written; its errors; q twice; Ctrl-C at its prompt; w name',
      init: 't_rc', cycles: 80e6,
      // (rc's prompt waited for, then each session typed ahead: the console keeps the keys till edit reads its lines)
      machine: { input: '\u0101edit /ram/e\r' + 'a\rone\rtwo\rthree\r.\r2p\ri 1\rzero\r.\rp\r2,3d\rc 2\rTHREE\r.\rp\rw\rq\r' +
        '\u0101cat /ram/e\r' + '\u0101edit /ram/e\r' + '9p\rx\rd\ra\rfour\r.\rq\rq\r' +
        '\u0101edit /ram/e\r\u0100\x03\u0100' + 'Q\r' + '\u0101echo $status\r' +
        '\u0101edit\r' + 'a\rx\r.\rw\rw /ram/f\r1,$p\r0a\rfirst\r.\r$p\rh\rQ\r' + '\u0101cat /ram/f; edit a b; echo $status\r' },
      expect: [
        '% edit /ram/e\n/ram/e: new file\n*a\none\ntwo\nthree\n.\n*2p\n   2 two\n*i 1\nzero\n.\n*p\n   1 zero\n   2 one\n' +
          '   3 two\n   4 three\n*2,3d\n*c 2\nTHREE\n.\n*p\n   1 zero\n   2 THREE\n*w\n/ram/e: 11 bytes\n*q\n%',
        '% cat /ram/e\nzero\nTHREE\n%',
        '% edit /ram/e\n/ram/e: 2 lines\n*9p\n? no such line\n*x\n? h: help\n*d\n? which lines?\n*a\nfour\n.\n*q\n' +
          '? not written: q again to quit anyway\n*q\n%',
        '% edit /ram/e\n/ram/e: 2 lines\n*\n?\n*Q\n',
        '% echo $status\n\n%',
        '% edit\n*a\nx\n.\n*w\n? no file name (w name)\n*w /ram/f\n/ram/f: 2 bytes\n*1,$p\n   1 x\n*0a\nfirst\n.\n*$p\n   2 x\n' +
          '*h\np [a[,b]]  print (all)       a [n]      add after n (the last)\n',
          'n: a number, or $ (the last).  Lines typed after a, i or c end with a .\n*Q\n%',
        '% cat /ram/f; edit a b; echo $status\nx\nusage: edit [file]\nusage\n%',
      ],
    },
    {
      name: 'snd', what: 'sound (#a): snd, sndctl and bell; the volume, claims (one another program holds), the shadow, tones (C, snd.h)',
      init: 't_rc', cycles: 120e6,
      get machine() { return { input: SND_LINES.map(l => '\u0101' + l[0] + '\r').join('') }; },
      get expect() { return SND_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')); },
      check(m) {
        const f = [], keys = m.ym.keyOns.join(', ');
        for (const ch of [0, 1, 2, 3, 7]) if (!m.ym.keyOns.some(k => k.startsWith('ch ' + ch + ' '))) f.push('no key-on on channel ' + ch + ': ' + keys);
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        this.notes = ['the YM2151: ' + m.ym.keyOns.length + ' key-ons'];
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
      name: 'pc-damage', what: '/pc\'s frames damaged on the line: requests (the PC tool asks for them again: its NAK), a reply (the Hydra asks again, and the PC tool answers from its last reply); the answers right, nothing damaged shown',
      init: 't_rc', cycles: 120e6, pc: { files: { 'hello.txt': 'Hello from the PC\n' }, damage: ['q3', 'r4', 'q6'] },
      machine: { input: 'āls /pc\rācat /pc/hello.txt\rācat /pc/hello.txt\r' },
      expect: ['% ls /pc\nhello.txt\n%', '% cat /pc/hello.txt\nHello from the PC\n% cat /pc/hello.txt\nHello from the PC\n%'],
      check: m => pcReport(m, 1, 2, 1),
    },
    {
      name: 'xmodem', what: 'xmodem: a file received (1K blocks, a CRC; one damaged, one sent twice) and sent back (128-byte blocks, a checksum, one NAKed; 1K ones), the same; Ctrl-C at its start; the PC cancelling; 115200',
      init: 't_rc', cycles: 200e6,
      // (The emulator the PC's end: sim/lib/xmpeer.js.  The -s sessions' start, NAK or C, typed as a key.  At 115200,
      // 128-byte blocks received: a 1K block comes faster than it can be taken, as the console's receive ring holds 255)
      get machine() {
        this.peer = createXmodemPeer([
          { trigger: 'xmodem -r /ram/x\r\n', role: 'send', data: XM_DATA(), k: true, damage: [2], again: [1] },
          { trigger: 'xmodem -s /ram/x\r\n', role: 'receive', crc: false, nak: [3] },
          { trigger: 'xmodem -s -k /ram/x\r\n', role: 'receive', crc: true },
          { trigger: 'xmodem -r /ram/y\r\n', role: 'send', data: XM_DATA(), cancel: 2 },
          { trigger: 'xmodem -r /ram/w\r\n', role: 'send', data: XM_DATA() },
          { trigger: 'xmodem -s -k /ram/w\r\n', role: 'receive', crc: true },
        ]);
        return { pcHost: this.peer, input: 'āxmodem -r /ram/x\r' + 'āxmodem -s /ram/x\rĀ\x15' + 'āxmodem -s -k /ram/x\rĀC' +
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
      name: 'cons', what: 'the console: lines, editing, history, raw keys, Ctrl-C, windows (shown, repainted, made, gone), 115200, the bell',
      init: 't_cons', modules: ['t_child'], cycles: 80e6,
      // (ā: wait for a prompt, "N> ")
      machine: { input: 'āhello\r' + 'āabX\x08c\r' + 'āac\x1b[Db\r' + 'ābc\x1b[Ha\x1b[Fd\r' +
        'āxyz\x15ok\r' + 'āabXc\x1b[D\x1b[D\x1b[3~\r' + 'ā\x1b[A\x1b[A\r' + 'ā\x04' + 'āparts\r' +
        'āx\x1b[A' + 'ā\x03' +
        'ā\x1d1z\r\x1d0' + 'ā\x1d1\x03\x1d0' + 'ā\x1dc' },
      check(m, out) {
        const f = [], a = m.acia, want = a.wdc ? 1 : 2;
        if (!out.includes('\x1b[2J') || !out.includes('w1 hidden text')) f.push('window 1 shown: no repaint of its text');
        this.notes = ['at 115200, the shortest idle time between characters sent: ' + a.gapMin.toFixed(2) + ' bits (at least ' + want + ')'];
        if (!(a.gapMin >= want - 0.05)) f.push('at 115200, characters ' + a.gapMin.toFixed(2) + ' bits apart: less than ' + want);
        if (a.overruns) f.push(a.overruns + ' bytes written to the ACIA while it was still sending');
        if (!m.ym.keyOns.some(k => k.startsWith('ch 7 '))) f.push('a BEL printed: no bell (no key-on on channel 7)');
        return f;
      },
    },
    {
      name: 'mem', what: 'memory: BREAK, pages, banks, a shared segment between tasks (and kcopy from it)',
      init: 't_mem', modules: ['t_child'], cycles: 30e6,
    },
    {
      name: 'banks', what: 'a module of two banks: calls between them (FAR2, FAR1), registers and C, each bank\'s data',
      init: 't_bank2', cycles: 10e6,
    },
    {
      name: 'banks3', what: 'a module of three banks: calls from any bank to any (FARN), registers and C, each bank\'s data, each bank set again',
      init: 't_bank3', cycles: 10e6,
    },
    {
      name: 'scall', what: 'spike S3: calls into a driver\'s task, its errors, a busy driver, the round trip',
      init: 't_scall', modules: ['t_child', 't_drv'], without: ['cons', 'storage', 'snd', 'gpio'], cycles: 40e6,
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
      name: 'hylang', what: 'hylang\'s reader, printer and REPL (phase 2: what a line reads to, printed as danlang\'s REPL prints a value): atoms, symbols, T, NIL, exit, lists of three kinds, strings and here strings with their escapes, characters by name, the shorthand, $name, decimal fixnums; an expression over lines, a comment, a here string; the reader\'s errors; 255 brackets open; Ctrl-C at the prompt; exit; stdin a pipe, its end; hylang -g (a collection before every allocation)',
      init: 't_rc', cycles: 400e6,
      // (Each line typed at a prompt: hylang> and, for more lines, the closers it wants then " <")
      get machine() {
        const lines = ['42', '', '1 2 3', '(+ 1 2)', '{a B :C T nil exit () {} [] [1 2]}',
          '{"a\\nb" "\\e[1m\\x01\\x7f" """x"y""" "" """""" "\\x41\\x4a2" "tab\\there" "\\\\\\""}',
          '{\\a \\A \\space \\( \\] \\lf \\LineFeed \\line-feed \\null \\\\ \\" \\; \\escape \\del \\x}',
          '{?(c a b) ?{c} =(x 1) :(y 2) #(z) @({x} {x}) .(f l) ~("s") ?x a:b :}', '$HOME $Mixed_Case $',
          '{1 -2 +3 16383 -16384 1_000 1_ +_1 007 -0 - + 1+ -_ 1a _1 #x10 1.5}',
          '{a ; a comment', 'b}', '(a', '{b', 'c})', '"""x', 'y"""',
          '(1 2]', ')', '{a)', '[a}', 'f(x)', 'x[1]', '+#(a)', '$(x)', '"\\q"', '"abc', '"\\x"', '\\zzz', '\\', '70000', '16384',
          '('.repeat(100), '('.repeat(100), '('.repeat(55) + '1' + ')'.repeat(55), ')'.repeat(100), ')'.repeat(100),
          '('.repeat(100), '('.repeat(100), '('.repeat(56)];
        return { input: '\u0101hylang\r' + lines.map(l => '\u0101' + l + '\r').join('') + '\u0101(a\r\u0101\x03' +
          '\u0101exit\r' + '\u0101echo $status\r' +
          '\u0101echo \'(1 2) $x {a\' | hylang; echo \'42\' | hylang; echo $status\r' +
          '\u0101hylang -g\r' + '\u0101{"a" "b" (c d) [e f] $G :h \\i 123 "c\\x41"}\r' + '\u0101(1 (2 (3 (4))) {5\r\u0101"""6\r\u01017"""})\r' +
          '\u0101(1 2]\r' + '\u0101exit\r' + '\u0101echo $status\r' };
      },
      expect: ['hylang (danlang on the Hydra-16), phase 2: it reads, and prints what it read\nType \'exit\' to Exit\n\n' +
        'hylang> 42\n=> 42\nhylang> \n=> NIL\nhylang> 1 2 3\n=> (1 2 3)\nhylang> (+ 1 2)\n=> (+ 1 2)\n' +
        'hylang> {a B :C T nil exit () {} [] [1 2]}\n=> {a b :c T NIL exit NIL NIL (list) (list 1 2)}\n',
        '=> {"a\\nb" "\\e[1m\\x01\\x7F" "x\\"y" "" "" "AJ2" "tab\\there" "\\\\\\""}\n',
        '=> {\\a \\A \\space \\lparen \\rbracket \\lf \\lf \\lf \\null \\backslash \\quote \\semicolon \\escape \\delete \\x}\n',
        '=> {(if c a b) {if c} (set x 1) (def y 2) (hash-create z) (fn {x} {x}) (unpack f l) (format "s") ?x a:b :}\n',
        '=> ((env "HOME") (env "Mixed_Case") $)\n',
        '=> {1 -2 3 16383 -16384 1000 1 1 7 0 - + 1+ -_ 1a _1 #x10 1.5}\n',
        'hylang> {a ; a comment\n\t} <b}\n=> {a b}\nhylang> (a\n\t) <{b\n\t)} <c})\n=> (a {b c})\n' +
          'hylang> """x\n\t""" <y"""\n=> "x\\ny"\n',
        'hylang> (1 2]\n=> Error: Closed a list without opening: )\nhylang> )\n=> Error: Closed a SExpr without opening: \n' +
          'hylang> {a)\n=> Error: Closed a SExpr without opening: }\nhylang> [a}\n=> Error: Closed a QExpr without opening: ]\n' +
          'hylang> f(x)\n=> Error: \'f\' touches \'(\': put a space between them\n' +
          'hylang> x[1]\n=> Error: \'x\' touches \'[\': put a space between them\n' +
          'hylang> +#(a)\n=> Error: \'+#\' touches \'(\': put a space between them\n' +
          'hylang> $(x)\n=> Error: \'$(\' isn\'t danlang: $name is the environment\'s variable\n' +
          'hylang> "\\q"\n=> Error: Unknown escape sequence \\q\nhylang> "abc\n=> Error: Newlines are not allowed in regular strings\n' +
          'hylang> "\\x"\n=> Error: \\x needs a hex digit\nhylang> \\zzz\n=> Error: Unknown character name \\zzz\n' +
          'hylang> \\\n=> Error: A character needs a name\n' +
          'hylang> 70000\n=> Error: Not yet: an integer past a fixnum (phase 5)\nhylang> 16384\n=> Error: Not yet: an integer past a fixnum (phase 5)\n',
        ' <' + ')'.repeat(100) + '\n=> ' + '('.repeat(255) + '1' + ')'.repeat(255) + '\n',
        ' <' + '('.repeat(56) + '\n=> Error: Too deep: more than 255 brackets open\nhylang> ',
        '\nhylang> exit\n=> exit\n\n% echo $status\n\n%',
        'hylang> \t} <=> Error: missing }\n', 'hylang> => 42\nhylang> => exit\n\n%',
        'hylang> {"a" "b" (c d) [e f] $G :h \\i 123 "c\\x41"}\n=> {"a" "b" (c d) (list e f) (env "G") :h \\i 123 "cA"}\n' +
          'hylang> (1 (2 (3 (4))) {5\n\t)} <"""6\n\t)}""" <7"""})\n=> (1 (2 (3 (4))) {5 "6\\n7"})\n' +
          'hylang> (1 2]\n=> Error: Closed a list without opening: )\nhylang> exit\n=> exit\n% echo $status\n\n%'],
    },
    {
      name: 'kcopy', what: 'spike S2: copying between tasks',
      init: 't_kcopy', cycles: 40e6,
      budgets: [{ what: 'kcopy, 4096 bytes (DBG_KCOPY)', from: '<kc', to: 'kc>', per: 4096, max: 40 }],
    },
    {
      name: 'irq', what: 'spike S1: 115200 received by an irq entry while tasks spin',
      init: 't_irq', modules: ['t_child'], without: ['cons', 'storage', 'snd', 'gpio'], cycles: 30e6,
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
  ],
};
