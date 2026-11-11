// ****************************************************************************
// system.js - the system's tests: the kernel, POST, the devices, the shell and its tools, the ROM, the programs.  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { MODULES, card, CARD_DIR, OLD_CARDS, imageCard, RC_LINES, shellCard, typed } = require('../lib/common.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

const S1_BYTES = 2000;

const DISK_CARDS = [card(0, 2048, false, (n, i) => n * 7 + i), card(1, 4096, true, (n, i) => n * 13 + i + 1)];

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
  const f = path.join(CARD_DIR, 'load0.img'), prog = n => fs.readFileSync(path.join(TESTS, '..', 'obj', 'tests', n + '.hyx'));
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

// The kernel's bring-up console's test's program (kputc): fd 1 closed, so its PUTC is the kernel's, straight to the
// serial port, polled (as the boot's last line is, the console driver up): 1,024 dots
const KPUT_S = ['.include "hydra.inc"', '.include "hyx2.inc"', '', '            HYX2_PROGRAM "kput", main', '', '.code',
  'main:       lda #1', '            jsr CLOSE', '            ldy #4', '            ldx #0', "@dot:       lda #'.'", '            jsr PUTC',
  '            dex', '            bne @dot', '            dey', '            bne @dot', '            rts', ''].join('\n');

// The step test's card: in bin, t_steppee (tests/ram), the program t_step steps
function stepCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'step0.img');
  hydrafs.mkfs(f, 8, 'STEP', undefined, true);
  const v = new hydrafs.Volume(f);
  v.mkdir('bin');
  v.put('bin/t_steppee', fs.readFileSync(path.join(TESTS, '..', 'obj', 'tests', 't_steppee.hyx')));
  v.close();
  return [imageCard(0, f, 16384)];
}

const BIG_LENGTH = () => fs.statSync(path.join(TESTS, '..', 'obj', 'tests', 't_big.hyx')).size;

// /bin's programs in the rc and tools tests: the ROM's program modules (their headers' type, HT_PROGRAM), the ROM
// disk's bin, and t_rc (init)
function BIN_COUNT() {
  const root = path.join(TESTS, '..'), { readManifest } = require('../../build.js');
  const mods = readManifest(path.join(root, 'modules', 'rom.txt')).modules
    .filter(n => fs.readFileSync(path.join(root, 'obj', 'modules', n + '.bin'))[5] === 1);
  const disk = fs.readFileSync(path.join(root, 'romfs', 'romfs.txt'), 'latin1').split(/\r?\n/).filter(l => /^bin\//.test(l));
  return mods.length + disk.length + 1;
}

// The ROM disk's bin (/rom/bin): its programs' names, as ls lists them
function ROM_BIN() {
  return fs.readFileSync(path.join(TESTS, '..', 'romfs', 'romfs.txt'), 'latin1').split(/\r?\n/).filter(l => /^bin\//.test(l))
    .map(l => l.split(/\s+/)[0].slice(4)).sort();
}

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
  ["free","ram     " + MODULES * 128 + " KB a task (" + MODULES + " modules)\nshared  1024 KB, 256 KB in segments (1), 768 KB free"],
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

module.exports = {
  tests: [
    {
      name: 'boot', what: 'the kernel boots, POST finds nothing wrong; init runs hello and waits for it',
      init: 'init', cycles: 20e6,
      expect: ['Hydra-16: kernel 0.1, ABI 1', 'HydraOS 1.0 for the Hydra-16', 'POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0',
        'RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 ' + ['00', '10', '20'].slice(0, MODULES).map(b => b + ':0/00/0000').join(' '),
        'POST ok', 'RAM modules: 0' + MODULES,                // (Each module's first bank tested, and the count: 3, the board's)
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
      name: 'tiers', area: 'tiers', what: "the tests chosen by what changed (sim/test.js --changed, --near, --list: ../base/sim/select.js, tests/areas.js; TESTING.md's tiers): the tests files read as entries, sample paths to the tests they must run and mustn't (areas.js's CASES), tests.js's lines to their entries, their helpers' tests or every test, every area a rule names some test's, NEAR's names tests",
      init: 'init', cycles: 3e6, expect: ['POST ok'],
      check() { const r = require('../areas.js').selfCheck(); this.notes = r.notes; return r.failures; },
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
        const romfs = require('../../tools/romfs.js'), { crc16 } = require('../../tools/romimg.js');
        const files = romfs.manifest(path.join(TESTS, '..', 'romfs', 'romfs.txt')), seen = new Map(), f = [];
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
        // (A mark is seen as its text goes out on the line, at the boot's 9600, from the kernel's console (t_load has no
        // fds).  That waits a character's time after each byte, 5,120 cycles, once the console driver has the ACIA's
        // receive interrupt on (as the W65C51N's build always did: kernel/cons.s), so a time here is the work's own,
        // and one such wait more for each character its end mark has past the baseline's (msp> four, b0> three).  Till
        // 2026-10-22 the Rockwell's waited on the line itself, and a time moved in steps of a character's, some 3,700
        // cycles, as the work before it shifted (the W65C51N's build measured 50,028 here then).  SPAWN's own: about
        // 45,000, growing with the ROM's modules, a hundred cycles or more each (October 2026: basic 169, as 106; the
        // PSG's bigger snd, vid and play 27).  With the wait: 50,107 (2026-10-22), so 51,000)
        { what: 'SPAWN of a module in place (#m/t_child), the caller\'s time', from: '<msp', to: 'msp>', minus: ['<b0', 'b0>'],
          per: 1, max: 51000 },
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
        const o = n => fs.readFileSync(path.join(TESTS, '..', 'obj', 'tests', n)), c = o('count.bp');
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
          const t = n => fs.readFileSync(path.join(TESTS, 'ram', 't_asall', n));
          return { 't_asall.s': t('t_asall.s'), 'inc1.inc': t('inc1.inc'), 'data.bin': t('data.bin'),
            't_asall.hyx': fs.readFileSync(path.join(TESTS, '..', 'obj', 'tests', 't_asall.hyx')),
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
          return { 'db.lbl': fs.readFileSync(path.join(TESTS, '..', 'obj', 'programs', 'db', 'db.lbl')), 'ops.bin': Buffer.from(ops) };
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
      name: 'step', what: 'the debugger\'s steps (TASKSTEP, /proc/N/ctl): a program started stopped (SPAWN_STOPPED), each kind of instruction a step at a time (out of line, or on its frame), a JSR stepped over, a breakpoint, a program\'s own BRK; refused steps',
      init: 't_step', modules: ['t_child'], cycles: 60e6,
      get machine() { return { sd: stepCard() }; },
    },
    {
      name: 'db', what: 'the debugger at rc (/rom/bin/db): the SDK\'s hi started stopped, its labels from /pc (ld65\'s) and hydra.inc\'s calls and registers, registers, steps, a disassembly (as as writes it: the asm library\'s, symbols in the operands), a breakpoint hit twice, a JSR to the kernel stepped over, until, memory read and written, and on to its end',
      init: 't_rc', cycles: 300e6,
      pc: { files: () => ({ 'hi.lbl': fs.readFileSync(path.join(TESTS, '..', 'obj', 'samples', 'hi', 'hi.lbl')) }) },
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
      name: 'kputc', what: 'the kernel\'s bring-up console written to with the console driver up (PUTC with fd 1 closed: the serial port, polled, as the boot\'s last line is) while keys come at the line\'s rate: none of its waits reads the ACIA\'s status register, which would clear a receive interrupt the CPU hadn\'t taken yet (the byte left unread, and no interrupt again: the console deaf till a reset, the browser page\'s fault of 2026-10-22), so the line typed meanwhile is read and run after it, and the next',
      init: 't_rc', cycles: 160e6,
      pc: { files: () => ({ 'kput.s': KPUT_S }) },
      machine: {
        paste: true,
        input: 'āas /pc/kput.s /ram/kput\r' + 'ā/ram/kput\rĀ' + 'echo typed while the kernel wrote\r' + 'āecho after\r',
      },
      expect: ['.'.repeat(1024), 'echo typed while the kernel wrote\ntyped while the kernel wrote\n% echo after\nafter\n% '],
      check(m) { return m.acia.rxLost ? [m.acia.rxLost + ' byte(s) came while the last was unread (its receive interrupt lost: the console deaf)'] : []; },
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
  ],
};
