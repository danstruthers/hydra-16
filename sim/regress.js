#!/usr/bin/env node
// ****************************************************************************
// regress.js - the OS ROM's regression tests: boots the ROM images in hydrasim.js with typed input, and
// checks the serial output (and the emulator's report) for what each test expects.
//
// Usage: node regress.js [options] [NAME ...]
//   NAME                Run only the tests whose names contain NAME (default: all of them)
//   --rom DIR           ROM images directory (default: hydrasim.js's, ../os_rom/bin)
//   --seed N            Power-up random number seed for every run (default 1: runs repeat exactly)
//   --random            A new random power-up each run (finds code that depends on uninitialised RAM)
//   --jobs N            Emulators run at once (default: the number of CPUs)
//   --verbose           Show each test's serial output, not just the failing ones'
//   --list              List the tests
//
// Exit code 0 when every test passes, 1 otherwise.  Each failure shows what was missing (or found when it
// shouldn't be), the command that reproduces it, and the serial output.
//
// A test: name, what it checks (about), the emulator options (args: --cycles, --input, ...), and:
//   expect  text or regular expressions that must appear in the serial output, in this order (each is
//           looked for after the previous one's match).  Line ends are "\n" (the ROM's CR LF, and ESC
//           shows as <ESC>)
//   forbid  text or regular expressions that must not appear anywhere in the serial output (every test
//           also forbids a driver's boot FAIL, and the emulator halting: a BRK to nowhere, an STP, ...)
//   check   function (serial output, the emulator's whole report, the test's files) returning an error
//           message, or nothing when it's good
//   sd      true: a blank 1 MB SD card image on device 0 (files.sd = its path); or a list of cards: { dev (0-7),
//           mb (default 1), sdsc (true: standard capacity), fill (a function given the image to fill in),
//           label, blocks (a HydraFS of that many blocks: fewer, and the image is cut to it; more, and the
//           image stays mb, for a card that claims more than the test needs) and hfs (a function given a
//           HydraFS volume made on the card, and the hydrafs module, to put files in it) }
//           (files.sds[dev] = each one's path)
//
// Every test also fails if a task's stack came within STACK_MARGIN bytes of its bottom (the emulator reports
// each task's lowest stack pointer); the summary shows the deepest stack of the whole run.
// ****************************************************************************
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFile } = require('child_process');
const hydrafs = require('./tools/hydrafs.js');            // For the tests that want a HydraFS card

const SIM = path.join(__dirname, 'hydrasim.js');
const STACK_MARGIN = 32;                                        // Free stack bytes a task must keep
const W = n => '\\w'.repeat(n);                                 // Wait n * ~2M cycles before the next key
const BOOT = W(1);                                              // Before the first key: to the HyForth prompt
const TO_MON = BOOT + 'bye\\r' + W(1);                          // To WOZMON

// A Forth number as "." prints it: " 0003"
const num = n => ' ' + n.toString(16).toUpperCase().padStart(4, '0');

// The CPU cycle test's program: at $E000 on every BIOS page (W powers up random), then STP.  Each
// instruction with its W65C02S cycles (WDC's table and extras); the test checks the total.
function cycleTestRom() {
  const code = [], at = n => 0xE000 + code.length + n;
  let cycles = 0;
  const op = (c, ...bytes) => { code.push(...bytes); cycles += c; };
  op(2, 0xA2, 0xFF);                                            // LDX #$FF
  op(2, 0xA0, 0x01);                                            // LDY #$01
  op(5, 0xBD, 0xF0, 0x10);                                      // LDA $10F0,X: crosses a page, +1
  op(4, 0xB9, 0x00, 0x10);                                      // LDA $1000,Y: doesn't
  op(5, 0x9D, 0xF0, 0x10);                                      // STA $10F0,X: a store, always 5
  op(7, 0x1E, 0xF0, 0x10);                                      // ASL $10F0,X: 6, +1 across a page (65C02)
  op(6, 0x1E, 0x00, 0x10);                                      // ASL $1000,X
  op(7, 0xFE, 0x00, 0x10);                                      // INC $1000,X: always 7
  op(2, 0xF8);                                                  // SED
  op(3, 0x69, 0x01);                                            // ADC #1: +1 in decimal mode
  op(2, 0xD8);                                                  // CLD
  op(2, 0xA0, 0x01);                                            // LDY #1 (Z = 0)
  op(3, 0xD0, 0x00);                                            // BNE: taken, +1
  op(2, 0xF0, 0x00);                                            // BEQ: not taken
  const jsr = code.length; op(6 + 6, 0x20, 0, 0);               // JSR sub (and its RTS)
  op(3, 0x64, 0x10);                                            // STZ $10
  op(6, 0x0F, 0x10, 0x00);                                      // BBR0 $10: 5, taken +1
  op(3, 0x4C, 0xFC, 0xE0);                                      // JMP $E0FC
  const sub = at(0); code.push(0x60);                           // sub: RTS
  code[jsr + 1] = sub & 0xFF; code[jsr + 2] = sub >> 8;
  while (code.length < 0xFC) code.push(0xEA);
  op(4, 0xD0, 0x10);                                            // $E0FC BNE $E10E: taken, to another page, +2
  while (code.length < 0x10E) code.push(0xEA);
  op(3, 0xDB);                                                  // $E10E STP
  const page = Buffer.alloc(0x2000, 0xEA); Buffer.from(code).copy(page);
  page[0x1FFC] = 0x00; page[0x1FFD] = 0xE0;                     // RESET: $E000
  return { bios: Buffer.concat(Array(16).fill(page)), cycles };
}

const TESTS = [
  {
    name: 'cpu-cycles', about: 'the W65C02S cycle counts: page crossing, branches, decimal mode, RMW abs,X, JSR/RTS, BBR',
    romImage: cycleTestRom,
    args: ['--cycles', '1000'],
    halts: /STP at [0-9A-F]:E10E/,                              // (On whichever page W powered up as)
    check: (out, report, files) => {
      const c = +/--- cycles (\d+)/.exec(report)[1];
      if (c !== files.expectCycles) return 'the program took ' + c + ' cycles, not ' + files.expectCycles;
    },
  },
  {
    name: 'boot', about: 'POST (task mapping, shared RAM, ROM page 1, RAM lines), drivers, HyForth banner',
    args: ['--cycles', '30000000'],
    expect: ['POST ZP:T ST:T LO:T 7D:T SH:S P1:4C\n',
      /RAM U:0( [0-9A-F]{2}:0\/00\/0000)+\n/,
      'Welcome to the HYDRA-16!', /HyForth \d/, 'HF>'],
  },
  {
    name: 'selftest', about: 'the ROM self tests from WOZMON: MMU (F833), scheduler (F869), IO (F88A)',
    args: ['--cycles', '60000000', '--input', TO_MON + 'F833R\\r' + W(2) + 'F869R\\r' + W(4) + 'F88AR\\r' + W(1)],
    expect: ['T1 00:00>F833R', 'MMU test: ok',
      'Sched test:\n', /^(?=[abm]*a)(?=[abm]*b)(?=[abm]*m)[abm]+\n/m,  // 1. a, b and m interleave
      /^m*\[c{10}\]m+\n/m,                                      // 2. NO_PREEMPT: the c's all together
      'wW\n', 'done',                                           // 3. TASK_WAIT, IO_WAKE
      'IO test: cons ok'],
    forbid: ['FAIL'],
  },
  {
    name: 'selftest-small', about: 'the self tests with 1 RAM module and 1 shared RAM macro-page',
    args: ['--cycles', '60000000', '--modules', '1', '--shared-u', '1',
      '--input', BOOT + 'mmtest\\r' + W(1) + 'bye\\r' + W(1) + 'F869R\\r' + W(4) + 'F88AR\\r' + W(1)],
    expect: [/RAM U:F F0:0\/00\/0000 F4:0\/00\/0000 F8:0\/00\/0000 FC:0\/00\/0000 00:0\/00\/0000\n/,
      'MMU test: ok', 'Sched test:', 'done', 'IO test: cons ok'],
    forbid: ['FAIL'],
  },
  {
    name: 'post-ram-fault', about: 'POST reports a stuck address line (A0 high on the chip holding shared bank $F0)',
    args: ['--cycles', '8000000', '--ram-fault', 'F0:A0:high'],
    expect: [/RAM U:0 F0:0\/00\/0001 F4:0\/00\/0000 /],
  },
  {
    name: 'post-no-shared', about: 'POST with no shared RAM: SH:X, and the drivers that need it fail',
    args: ['--cycles', '30000000', '--model', 'noshared'],
    expect: ['SH:X', 'SOUND FAIL', 'HF>'],
    bootFailOk: true,
  },
  {
    name: 'forth', about: 'HyForth: arithmetic (decimal in, hex out), negatives, $ and % prefixes, typed wc (Ctrl-D ends it)',
    args: ['--cycles', '60000000', '--input', BOOT + '1 2 + .\\r1000 24 - .\\r-1 . -2 . -9 . -10 . $B . $1F . %101 .\\rwc\\rab c\\r\\x04. . .\\r'],
    expect: ['HF>1 2 + .\n' + num(3) + '\n', num(1000 - 24),
      ' FFFF FFFE FFF7 FFF6' + num(0xB) + num(0x1F) + num(5) + '\n',
      'HF>. . .\n' + num(5) + num(2) + num(1) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'pipes', about: 'pipelines: words | wc, and through cat (a copy of the shell in the middle) gives the same',
    args: ['--cycles', '150000000', '--input', BOOT + 'words | wc . . .\\rwords | cat | wc . . .\\rwords | cat | cat | wc . . .\\r1 2 + .\\r'],
    expect: [/HF>words \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/, /HF>words \| cat \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/,
      /HF>words \| cat \| cat \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/, 'HF>1 2 + .\n' + num(3)],
    forbid: ['!DS PTR ERROR!', '!IO ERR!'],
    check: out => {
      const counts = [...out.matchAll(/wc \. \. \.\n((?: [0-9A-F]{4}){3})\n/g)].map(m => m[1]);
      if (counts.length !== 3 || counts.some(c => c !== counts[0])) return 'the pipelines counted differently: ' + counts.join(' /');
      if (/ 0000 0000 0000/.test(counts[0])) return 'wc counted nothing';
    },
  },
  {
    name: 'io', about: 'open/read /dev/zero, a missing file (ERR 70), mount and ns, /dev/proc, a server\'s not found (ERR 70)',
    args: ['--cycles', '90000000', '--input', BOOT +
      'q^/dev/zero^ 1 open here @ 5 read . ioerr .\\r' +
      'q^/dev/nothere^ 1 open\\rioerr .\\r' +
      'q^/z^ q^zero^ mount ns\\rq^/z^ 1 open here @ 3 read .\\r' +
      'q^/dev/proc^ 1 open here @ 100 read .\\r' +
      'q^/dev/proc/z^ 1 open\\rioerr .\\r'],
    expect: ['read . ioerr .\n' + num(5) + num(0) + '\n',
      '!IO ERR!', 'HF>ioerr .\n' + num(0x70) + '\n',
      '/z -> zero\n', 'read .\n' + num(3) + '\n',
      /\/dev\/proc\^ 1 open here @ 100 read \.\n 00[1-9A-F][0-9A-F]\n/,
      '!IO ERR!', 'HF>ioerr .\n' + num(0x70) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'tasks', about: 'another shell: ps, Ctrl-] to switch the console, kill; Ctrl-C breaks a read',
    args: ['--cycles', '150000000', '--input', BOOT + 'shell\\r' + W(1) + 'ps\\r' + W(1) + '\\x1dB' + W(1) + '\\r1 2 + .\\r' + W(1) + '\\x1d1' + W(1) +
      '\\r11 kill\\rps\\rcat\\r' + W(1) + '\\x03' + W(1) + '3 4 + .\\r'],
    expect: ['HF>ps\n0 R -\n1 R 0 *\nB W 1\n', '[B]', 'HF>1 2 + .\n' + num(3), '[1]',
      'HF>ps\n0 R -\n1 R 0 *\nC D -\n', 'HF>cat\n', '!BREAK!', 'HF>3 4 + .\n' + num(7)],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'sound', about: 'sndtest plays in a task of its own while the shell runs; sndstop ends it; the bell (Ctrl-G) first',
    args: ['--cycles', '90000000', '--input', BOOT + '\\x07\\r' + W(1) + 'sndtest\\r' + W(1) + 'ps\\r' + W(2) + 'sndstop\\rps\\r'],
    expect: ['HF>ps\n0 R -\n1 R 0 *\nB W E\n', 'HF>sndstop\n', 'HF>ps\n0 R -\n1 R 0 *\nC D -\n'],  // (B W: it sleeps between notes)
    check: (out, report) => {
      const m = /--- YM2151 key-ons: (\d+) \((.*)\)/.exec(report);
      if (!m || +m[1] < 5) return 'the tune played ' + (m ? m[1] : 'no') + ' notes';
      if (!/^ch 7 at/.test(m[2])) return 'no bell (the first key-on, on channel 7)';
    },
  },
  {
    name: 'serial', about: 'serial settings: 9600 8N1 at boot; stty (/dev/ser/ctl), a refused format, IO_CTL rate and format; the ACIA\'s registers',
    args: ['--cycles', '60000000', '--input', BOOT + 'stty?\\rq^b19200 l7 pe s2^ stty stty?\\rq^l8 pe s2^ stty\\rioerr .\\r' +
      '1 2 6 ioctl stty?\\r1 3 11 ioctl stty?\\r'],
    expect: ['HF>stty?\nb9600 l8 pn s1\n', 'stty stty?\nb19200 l7 pe s2\n', '!IO ERR!', 'HF>ioerr .\n' + num(0x78) + '\n',
      '6 ioctl stty?\nb4800 l7 pe s2\n', '11 ioctl stty?\nb4800 l8 pe s1\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {                                   // 4800 ($0C), 8 bits, 1 stop; even parity ($60) on DTR + IRQs ($05)
      if (!/ACIA control 1C command 65,/.test(report)) return 'the ACIA isn\'t at 4800 8E1: ' + (/ACIA control \w+ command \w+/.exec(report) || ['?'])[0];
    },
  },
  {
    name: 'paste', about: 'serial input at the full line rate (--paste): 1000 characters into wc at 57600, none lost',
    args: ['--cycles', '60000000', '--paste', '--input', BOOT + 'q^b57600^ stty\\r' + W(1) + 'wc . . .\\r' +
      Array(20).fill('the quick brown fox jumps over the lazy dog 0123 \\r').join('') + '\\x04'],
    expect: [' 03E8 00C8 0014\n'],                                  // 1000 characters, 200 words, 20 lines
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {
      const m = /ACIA: (\d+) received byte\(s\) lost/.exec(report);
      if (!m || +m[1]) return (m ? m[1] : '?') + ' received byte(s) lost';
    },
  },
  {
    name: 'fast-output', about: 'console output at 115200: words (3.5K characters) in well under a second (the fast paths)',
    args: ['--cycles', '40000000', '--mark', 'HF>words', '--mark', 'HF>', '--input', BOOT + 'q^b115200^ stty\\r' + W(1) + 'words\\r'],
    expect: ['HF>words\n'],
    check: (out, report) => {                                   // (The wire alone: about 1.1M cycles; the old IO path: 3.6M)
      const at = +/mark: "HF>words" at cycle (\d+)/.exec(report)[1];
      const took = [...report.matchAll(/mark: "HF>" at cycle (\d+)/g)].map(m => +m[1]).find(c => c > at) - at;
      if (!(took < 2500000)) return 'words took ' + took + ' cycles at 115200, not under 2.5M';
    },
  },
  {
    name: 'irqs-off', about: 'no long stretch with IRQs off after boot (tasks starting and ending, a pipeline, sound, files): a serial byte can\'t wait long',
    args: ['--cycles', '90000000', '--input', BOOT + 'words | wc . . .\\rq^/dev/zero^ 1 open here @ 16 read .\\r3 close\\r' +
      'sndtest\\r' + W(2) + 'sndstop\\rshell .\\r' + W(1) + 'mmtest\\r'],
    expect: ['MMU test: ok'],
    check: (out, report) => {
      const m = /longest with IRQs off.*?at cycle\): (\d+): (\S+) -> (\S+)/.exec(report);
      if (!m) return 'no IRQs-off report';
      if (+m[1] > 5000) return 'IRQs were off for ' + m[1] + ' cycles (from ' + m[2] + ' to ' + m[3] + ')';
    },
  },
  {
    name: 'sd', about: '/dev/sd/0/data: write a byte at offset 512, read it back, and it\'s in the card image',
    sd: true,
    args: ['--cycles', '90000000', '--input', BOOT + 'q^/dev/sd/0/data^ 3 open .\\r90 here @ c!\\r' +
      '3 512 0 seek 3 here @ 1 write . ioerr .\\r91 here @ c!\\r3 512 0 seek 3 here @ 1 read . here @ c@ .\\r3 close\\r'],
    expect: ['open .\n' + num(3) + '\n', 'write . ioerr .\n' + num(1) + num(0) + '\n',
      'read . here @ c@ .\n' + num(1) + num(90) + '\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const img = fs.readFileSync(files.sd);
      if (img[512] !== 90) return 'the card image has $' + img[512].toString(16) + ' at 512, not $5A';
      if (img.some((b, i) => b && i !== 512)) return 'the card image changed somewhere else too';
    },
  },
  {
    name: 'sd-speed', about: 'SD read throughput: 4K in 256-byte reads, inside a cycle budget (the bit-banged SPI is most of it)',
    sd: true,
    args: ['--cycles', '200000000', '--mark', 'HF>go', '--mark', 'HF>', '--input', BOOT +
      'ftrain autoload\\rq^/dev/sd/0/data^ 1 open .\\r' +
      ': go lit [ 16 , ] 0 do 3 here @ lit [ 256 , ] read drop loop ;\\r' + W(1) + 'go\\r' + W(2)],
    expect: ['open .\n' + num(3) + '\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {                                   // 298 cycles/byte now; SPI_RECV is about 60% of it
      const at = +/mark: "HF>go" at cycle (\d+)/.exec(report)[1];
      const took = [...report.matchAll(/mark: "HF>" at cycle (\d+)/g)].map(m => +m[1]).find(c => c > at) - at;
      const per = Math.round(took / 4096);
      if (per > 330) return 'an SD read took ' + per + ' cycles a byte, over the 330 budget';
    },
  },
  {
    name: 'sd-cards', about: 'SD cards on devices 0 (SDHC) and 1 (SDSC, CSD v1), none on 3: ctl files, SDSC data, init, bad names',
    sd: [{ dev: 0 }, { dev: 1, mb: 3, sdsc: true, fill: img => img.write('SDSC-BLK1', 512) }],
    args: ['--cycles', '150000000', '--input', BOOT +
      'q^/dev/sd/0/ctl^ 1 open 0 fdup2 cat | cat\\rq^/dev/sd/1/ctl^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/dev/sd/3/ctl^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/dev/sd/1/data^ 3 open .\\r3 512 0 seek 3 here @ 9 read . here @ 5 + c@ .\\r' +
      '66 here @ c! 3 1024 0 seek 3 here @ 1 write .\\r3 close\\r' +
      'q^/dev/sd/9/data^ 1 open\\rioerr .\\rq^/dev/sd/0/nope^ 1 open\\rioerr .\\rq^/dev/sd/3/data^ 1 open\\rioerr .\\r' +
      'q^/dev/sd/1/ctl^ 3 open .\\r105 here @ c! 110 here @ 1 + c! 105 here @ 2 + c! 116 here @ 3 + c!\\r' +
      '3 here @ 4 write .\\r3 here @ 2 write\\rioerr .\\r3 close\\r'],
    expect: ['| cat\nsdhc 1 MB 2048 blocks\n', '| cat\nsdsc 3 MB 6144 blocks\n', '| cat\nnone\n',
      'open .\n' + num(3) + '\n', 'c@ .\n' + num(9) + num(0x42) + '\n', 'write .\n' + num(1) + '\n',
      '!IO ERR!', 'HF>ioerr .\n' + num(0x70) + '\n', '!IO ERR!', 'HF>ioerr .\n' + num(0x70) + '\n',
      '!IO ERR!', 'HF>ioerr .\n' + num(0x79) + '\n',
      'open .\n' + num(3) + '\n', '4 write .\n' + num(4) + '\n', '!IO ERR!', 'HF>ioerr .\n' + num(0x78) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const img = fs.readFileSync(files.sds[1]);
      if (img[1024] !== 66) return 'the SDSC card image has $' + img[1024].toString(16) + ' at 1024, not $42';
    },
  },
  {
    name: 'hydrafs', about: 'HydraFS reading: /sd/N, walking, "." and "..", file and directory reads, stat records, errors',
    sd: [{ dev: 0, mb: 2, label: 'TESTS', hfs: v => {
      v.put('hello.txt', Buffer.from('hello hydra\r\n'));
      v.mkdir('games');
      v.put('games/star.frt', Buffer.from(': star 42 . ;\r\n'));
      v.put('big.bin', Buffer.from(Array.from({ length: 5000 }, (_, i) => i & 0xFF)));
      v.mkdir('many');                                          // More entries than one block holds (8)
      for (let i = 0; i < 10; i++) v.put('many/f' + i, Buffer.from(String(i)));
      for (const n of 'abcdefg') v.put(n, Buffer.alloc(4096, n.charCodeAt(0)));
      for (const n of 'bdf') v.remove(n);                       // So "a" grows into three separate runs:
      v.write(v.walk('a'), 4096, Buffer.alloc(12288, 0x5A));    //   three extents, one in an extent block
    } }, { dev: 1 }],                                           // And a card with no HydraFS on it
    args: ['--cycles', '400000000', '--input', BOOT +
      'q^/sd/0^ 1 open 0 fdup2 cat | cat\\rq^/sd/0/many^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/sd/0/./games/../hello.txt^ 1 open 0 fdup2 cat | cat\\rq^/sd/0/..^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/sd/0/a^ 1 open .\\r3 0 0 seek 3 here @ 1 read . here @ c@ .\\r' +
      '3 4096 0 seek 3 here @ 1 read . here @ c@ .\r3 16383 0 seek 3 here @ 4 read . here @ c@ .\r3 close\r' +
      'q^/sd/0/big.bin^ 1 open .\\r3 4095 0 seek 3 here @ 4 read . here @ c@ . here @ 1 + c@ .\\r3 close\\r' +
      'q^/sd/0/many^ 5 open .\\r3 here @ 48 read . here @ c@ . here @ 32 + c@ . here @ 40 + @ .\\r3 close\\r' +
      'q^/sd/1/x^ 1 open\\rioerr .\\rq^/sd/3/x^ 1 open\\rioerr .\\rq^/sd/8/x^ 1 open\\rioerr .\\r' +
      'q^/sd^ 1 open\\rioerr .\\rq^/sd/0/games^ 2 open\\rioerr .\\rq^/sd/0/hello.txt/x^ 1 open\\rioerr .\\r'],
    expect: [
      '| cat\nhello.txt 13\ngames/\nbig.bin 5000\nmany/\na 16384\nc 4096\ne 4096\ng 4096\n',
      '| cat\nf0 1\nf1 1\nf2 1\nf3 1\nf4 1\nf5 1\nf6 1\nf7 1\nf8 1\nf9 1\n',
      '| cat\nhello hydra\n',                                   // "." and ".." on the way
      '| cat\nhello.txt 13\n',                                  // ".." at the root stays there
      'open .\n' + num(3) + '\n',                               // The three-extent file, a byte at a time
      'c@ .\n' + num(1) + num(0x61) + '\n',                     // Its first extent ("a")
      'c@ .\n' + num(1) + num(0x5A) + '\n',                     // The second (what was written at 4096)
      'c@ .\n' + num(1) + num(0x5A) + '\n',                     // The third, and 1 byte at the end of file
      'open .\n' + num(3) + '\n',                               // big.bin, over a cluster boundary
      '1 + c@ .\n' + num(4) + num(0xFF) + num(0) + '\n',
      'open .\n' + num(3) + '\n',                               // A directory as stat records (IO_MODE_STAT)
      '@ .\n' + num(48) + num(0x66) + num(0) + num(1) + '\n',   // "f0": 48 bytes, mode 0, size 1
      'HF>ioerr .\n' + num(0x80) + '\n',                        // No HydraFS on the card (ERR_IO_NOT_FS)
      'HF>ioerr .\n' + num(0x79) + '\n',                        // No card at all
      'HF>ioerr .\n' + num(0x70) + '\n',                        // Card 8
      'HF>ioerr .\n' + num(0x70) + '\n',                        // /sd, with no card in it
      'HF>ioerr .\n' + num(0x72) + '\n',                        // A directory opened for writing
      'HF>ioerr .\n' + num(0x70) + '\n',                        // A path through a file
    ],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const v = new hydrafs.Volume(files.sds[0]);               // Nothing on the card changed
      try { const p = v.check(); if (p.length) return 'the card is damaged: ' + p[0]; } finally { v.close(); }
    },
  },
  {
    name: 'hydrafs-write', about: 'HydraFS writing: a file grown into pieces (an extent block), truncate, append-only, create, mkdir, remove, rename, errors, format, a full card',
    sd: [{ dev: 0, mb: 2, label: 'WRITES', hfs: v => {
      for (const n of 'abcdefg') v.put(n, Buffer.alloc(4096, n.charCodeAt(0)));
      const at = v.extents(v.walk('b'))[0].start;
      for (const n of 'bdf') v.remove(n);
      v.hint = at;                                              // So the Hydra's clusters fill the holes first:
    } },                                                        //   a file in four pieces, one in an extent block
    { dev: 1 },                                                 // Blank: the Hydra formats it
    { dev: 2, label: 'TINY', blocks: 34, hfs: () => {} }],      // 4 clusters: it fills up
    args: ['--cycles', '220000000', '--input', BOOT + 'ftrain autoload\\r' + [
      ': wr lit [ 80 , ] 0 do 3 here @ lit [ 256 , ] write drop loop ;\\r' +
      ': fl lit [ 60 , ] 0 do 3 here @ lit [ 256 , ] write drop loop ;\\r' + W(1),
      'q^/sd/0/big^ 0 create .\\rwr 3 close\\r' + W(18) + 'q^/sd/0^ ls\\r',        // (20 KB: type-ahead would overflow)
      'q^/sd/0/t^ 0 create .\\r3 here @ 200 write . 3 close\\rq^/sd/0/t^ 10 open . 3 close\\r',
      'q^/sd/0/log^ 64 create .\\r3 here @ 4 write . 3 0 0 seek 3 here @ 4 write . 3 close\\r',
      'q^/sd/0/t^ 2 open .\\r3 100 0 seek 3 here @ 4 write\\rioerr .\\r3 close\\r',
      'q^/sd/0/ro^ 1 create . 3 here @ 4 write . 3 close\\rq^/sd/0/ro^ 2 open\\rioerr .\\r',
      'q^/sd/0/log^ mkdir\\rioerr .\\rq^/sd/0/sub^ mkdir\\rq^/sd/0/sub^ 0 create\\rioerr .\\r',
      'q^/sd/0/log^ 1 open .\\rq^/sd/0/log^ remove\\rioerr .\\r3 close\\r',
      'q^/sd/0/ro^ q^log^ rename\\rioerr .\\rq^/sd/0/log^ q^log2^ rename\\r',
      'q^/sd/0/sub/x^ 0 create . 3 close\\rq^/sd/0/sub^ remove\\rioerr .\\rq^/sd/0/c^ remove\\rq^/sd/0^ ls\\r',
      'q^/dev/sd/1/ctl^ 2 open .\\r3 q^format TEST^ @ 3 + 11 write . 3 close\\r' + W(1),
      'q^/sd/1/hi^ 0 create .\\r3 here @ 5 write . 3 close\\rq^/sd/1^ ls\\r',
      'q^/sd/2/fill^ 0 create .\\rfl\\r' + W(14) + 'ioerr .\\r3 close\\rq^/sd/2^ ls\\r'].join(W(2))],
    expect: [
      'ls\na 4096\nbig 20480\nc 4096\ne 4096\ng 4096\n',     // 20 KB, in the holes and after
      '200 write . 3 close\n' + num(200) + '\n',
      '4 write . 3 close\n' + num(4) + num(4) + '\n',          // Append-only: the second write after the first
      'HF>ioerr .\n' + num(0x78) + '\n',                        // A write past the end (no holes)
      'HF>ioerr .\n' + num(0x72) + '\n',                        // A read-only file (written as it was made)
      'HF>ioerr .\n' + num(0x82) + '\n',                        // mkdir where there's a file
      'HF>ioerr .\n' + num(0x82) + '\n',                        // A file where there's a directory
      'HF>ioerr .\n' + num(0x84) + '\n',                        // Removing an open file
      'HF>ioerr .\n' + num(0x82) + '\n',                        // Renaming to a name that's taken
      'HF>ioerr .\n' + num(0x83) + '\n',                        // Removing a directory with a file in it
      'ls\na 4096\nbig 20480\nt 0\ne 4096\nlog2 8\ng 4096\nro 4\nsub/\n',   // (Free entries used first)
      '11 write . 3 close\n' + num(11) + '\n',                  // format TEST
      'ls\nhi 5\n',
      'HF>ioerr .\n' + num(0x81) + '\n',                        // The card is full
      'ls\nfill 12288\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {                            // What the host tool finds on the cards
      const open = (dev, f) => { const v = new hydrafs.Volume(files.sds[dev]); try { return f(v); } finally { v.close(); } };
      for (const dev of [0, 1, 2]) {
        const p = open(dev, v => v.check());
        if (p.length) return 'card ' + dev + ': ' + p[0];
      }
      const e = open(0, v => {
        const big = v.walk('big'), d = v.read(big), ext = v.extents(big);
        if (ext.length < 3 || !big.extBlock) return 'big is in ' + ext.length + ' extents, with no extent block';
        for (let i = 256; i < d.length; i += 256) if (!d.subarray(i, i + 256).equals(d.subarray(0, 256))) return 'big\'s data is wrong at ' + i;
        if (v.walk('log2').size !== 8 || v.walk('t').size !== 0 || v.walk('ro').mode !== hydrafs.MODE_RO) return 'log2, t or ro is wrong';
      });
      if (e) return e;
      if (open(1, v => v.label) !== 'TEST') return 'card 1\'s label isn\'t TEST';
      if (open(2, v => v.freeCount) !== 0) return 'card 2 isn\'t full';
    },
  },
  {
    name: 'hydrafs-check', about: 'HydraFS check on /dev/sd/0/ctl: lost, unmarked and doubly used clusters over two passes, check fix, the ctl file\'s lines',
    sd: [{ dev: 0, label: 'CHECK', blocks: 600000, hfs: v => {    // 74997 clusters (two passes): a 1 MB image
      v.put('a', Buffer.alloc(100, 1)); v.put('b', Buffer.alloc(100, 2)); v.put('c', Buffer.alloc(5000, 3));
      v.mkdir('d'); v.put('d/e', Buffer.alloc(10, 4));
      v.setUsed(v.extents(v.walk('c'))[0].start + 1, false);  // Unmarked: c's second cluster
      const a = v.walk('a'), b = v.walk('b');
      b.setExt(0, a.ext(0)); v.writeEntry(b);                 // Twice: b uses a's cluster (b's own is lost)
      v.setUsed(70000, true);                                 // Lost, in the second pass
      const e = v.walk('d/e');
      e.setExt(0, { start: 70001, len: 1 }); v.writeEntry(e); // Unmarked in the second pass (e's own is lost)
      v.freeCount = 7;                                        // And the free count is wrong
    } }],
    args: ['--cycles', '120000000', '--input', BOOT + ['ls\\r', 'q^check^ ctl\\r' + W(3), 'ls\\r',
      'q^check fix^ ctl\\r' + W(3), 'ls\\r', 'q^check^ ctl\\r' + W(3), 'ls\\r'].map(s => 'q^/dev/sd/0/ctl^ ' + s).join('')],
    expect: ['ls\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 28 KB of 299988 KB\n',
      'ls\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 299960 KB of 299988 KB\ncheck: lost 3, unmarked 2, twice 1\n',
      'ls\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 299964 KB of 299988 KB\ncheck: lost 3, unmarked 2, twice 1, fixed\n',
      'ls\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 299964 KB of 299988 KB\ncheck: lost 0, unmarked 0, twice 1\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {                            // The host tool agrees: only the shared cluster
      const v = new hydrafs.Volume(files.sds[0]);               //   is wrong (it needs a person), and the free
      try {                                                     //   count is right
        const p = v.check();
        if (p.length !== 1 || !/ too$/.test(p[0])) return 'the card: ' + JSON.stringify(p);
      } finally { v.close(); }
    },
  },
  {
    name: 'sleep', about: 'TASK_SLEEP (HyForth sleep): 400 ticks take 2 s, and Ctrl-C ends a long one',
    args: ['--cycles', '40000000', '--mark', '400 sleep', '--mark', 'HF>', '--input', BOOT + '400 sleep\\r' + W(5) + '30000 sleep\\r' + W(1) + '\\x03' + W(1) + '1 2 + .\\r'],
    expect: ['HF>400 sleep\n', 'HF>30000 sleep\n', '!BREAK!', 'HF>1 2 + .\n' + num(3)],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {                                   // The line typed to the prompt: 2 s is 7.16M cycles at 3.58 MHz
      const typed = +/mark: "400 sleep" at cycle (\d+)/.exec(report)[1];
      const took = [...report.matchAll(/mark: "HF>" at cycle (\d+)/g)].map(m => +m[1]).find(c => c > typed) - typed;
      if (!(took > 7100000 && took < 7400000)) return '400 sleep took ' + took + ' cycles, not about 7.16M';
    },
  },
  {
    name: 'sd-shared', about: 'two shells read /dev/sd at once: the storage server is switched out mid-request, and the other waits for it',
    sd: true,
    args: ['--cycles', '90000000', '--input', BOOT + 'shell\\r' + W(1) + '\\x1dB' + W(1) + '\\rq^/dev/sd/0/data^ 1 open .\\r' + W(1) +
      '6 here @ 600 read '.repeat(12) + '\\r\\x1d1q^/dev/sd/0/data^ 1 open .\\r' + '3 here @ 600 read . '.repeat(6) + '\\r' + W(12) +
      '\\x1dB' + W(1) + '\\r' + '+ '.repeat(11) + '.\\r'],             // (B's 12 counts, added up: 7200 = $1C20)
    expect: ['HF>q^/dev/sd/0/data^ 1 open .\n' + num(6), '[1]q^/dev/sd/0/data^ 1 open .\n' + num(3), '[B]', '+ .\n' + num(7200) + '\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: out => {                                             // Shell 1's 6 reads (B's prompt can come out among them)
      const n = (out.slice(out.indexOf('[1]'), out.lastIndexOf('[B]')).match(/0258/g) || []).length;
      if (n !== 6) return 'shell 1 read 600 bytes ($0258) ' + n + ' times, not 6';
    },
  },
];

// ---- options
const opt = { rom: null, seed: 1, jobs: os.cpus().length, verbose: false, list: false, names: [] };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const a = argv[i], next = () => argv[++i];
  switch (a) {
    case '--rom': opt.rom = next(); break;
    case '--seed': opt.seed = +next(); break;
    case '--random': opt.seed = -1; break;
    case '--jobs': opt.jobs = Math.max(1, +next()); break;
    case '--verbose': opt.verbose = true; break;
    case '--list': opt.list = true; break;
    default:
      if (a.startsWith('--')) { console.error('unknown option ' + a); process.exit(2); }
      opt.names.push(a);
  }
}
const tests = TESTS.filter(t => !opt.names.length || opt.names.some(n => t.name.includes(n)));
if (opt.list) { for (const t of TESTS) console.log(t.name.padEnd(16) + t.about); process.exit(0); }
if (!tests.length) { console.error('no tests match ' + opt.names.join(' ')); process.exit(2); }

// ---- run
const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'hydra-regress-'));
const quote = s => /^[\w\/.:=-]+$/.test(s) ? s : "'" + s + "'";

function runTest(t) {
  const files = {};
  const args = [SIM, ...t.args];
  if (t.romImage) {                                             // A ROM image of its own
    const { bios, cycles } = t.romImage(), dir = path.join(tmpDir, t.name);
    fs.mkdirSync(dir);
    fs.writeFileSync(path.join(dir, 'os_rom_C02.bin'), bios);
    fs.writeFileSync(path.join(dir, 'paged_rom_C02.bin'), Buffer.alloc(0x10000, 0xFF));
    files.expectCycles = cycles;
    args.push('--rom', dir);
  } else if (opt.rom) args.push('--rom', opt.rom);
  if (opt.seed >= 0) args.push('--seed', String(opt.seed));
  files.sds = [];
  for (const c of t.sd === true ? [{ dev: 0 }] : t.sd || []) {    // The SD cards
    const f = path.join(tmpDir, t.name + '-' + c.dev + '.img'), img = Buffer.alloc((c.mb || 1) << 20);
    if (c.fill) c.fill(img);
    fs.writeFileSync(f, img);
    if (c.hfs) {                                                // A HydraFS on it, made with the host tool
      hydrafs.mkfs(f, c.mb || 1, c.label || '', c.blocks);    // (blocks: a smaller filesystem)
      const v = new hydrafs.Volume(f);
      try { c.hfs(v, hydrafs); } finally { v.close(); }
      if (fs.statSync(f).size > (c.mb || 1) << 20) fs.truncateSync(f, (c.mb || 1) << 20);   // (A HydraFS bigger than
    }                                                           //   the card: only its first blocks are used)
    files.sds[c.dev] = f;
    if (!files.sd) files.sd = f;
    args.push('--sd', c.dev + ':' + f);
    if (c.sdsc) args.push('--sdsc', String(c.dev));
  }
  let cmd = 'node hydrasim.js ' + args.slice(1).map(quote).join(' ');
  for (const f of files.sds) if (f) cmd = cmd.split(f).join(path.basename(f));
  return new Promise(resolve => execFile(process.execPath, args, { maxBuffer: 64 << 20 }, (err, stdout, stderr) => {
    const report = stdout.replace(/\r/g, '');
    const m = /--- serial output ---\n([\s\S]*?)\n--- last instructions/.exec(report);
    const out = m ? m[1] : '';
    const errors = [];
    if (err) errors.push('the emulator failed: ' + (stderr.trim().split('\n').pop() || err.message));
    else if (!m) errors.push('no serial output in the emulator\'s report');
    const halted = /--- halted: (.*)/.exec(report);
    if (t.halts) { if (!halted || !t.halts.test(halted[1])) errors.push('the emulator didn\'t halt as expected (' + t.halts + '): ' + (halted ? halted[1] : 'it ran on')); }
    else if (halted) errors.push('the emulator halted: ' + halted[1]);
    if (!t.bootFailOk && / FAIL [0-9A-F]{2}\n/.test(out)) errors.push('a driver failed to start: ' + / (\S+ FAIL [0-9A-F]{2})\n/.exec(out)[1]);
    let at = 0;
    for (const e of t.expect || []) {
      let found = -1, len = 0;
      if (e instanceof RegExp) {
        const re = new RegExp(e.source, e.flags.replace('g', '') + 'g'); re.lastIndex = at;
        const r = re.exec(out); if (r) { found = r.index; len = r[0].length; }
      } else { found = out.indexOf(e, at); len = e.length; }
      if (found < 0) { errors.push('missing (in order): ' + show(e)); break; }
      at = found + len;
    }
    for (const f of t.forbid || []) if (f instanceof RegExp ? f.test(out) : out.includes(f)) errors.push('found: ' + show(f));
    const stacks = [];                                          // [task, free bytes, W:PC]
    const sl = /--- lowest stack pointer by task .*?\): (.*)/.exec(report);
    if (sl) for (const e of sl[1].matchAll(/([0-9A-F]):[0-9A-F]{2} \((\d+); ([0-9A-F]:[0-9A-F]{4})\)/g)) stacks.push([e[1], +e[2], e[3]]);
    for (const [task, free, at] of stacks) if (free < STACK_MARGIN) errors.push('task ' + task + '\'s stack got down to ' + free + ' free bytes (at ' + at + ')');
    if (t.check && !errors.length) { const e = t.check(out, report, files); if (e) errors.push(e); }
    resolve({ t, errors, out, cmd, stacks });
  }));
}
const show = e => e instanceof RegExp ? String(e) : JSON.stringify(e);

(async () => {
  const start = Date.now(), results = [], queue = [...tests];
  await Promise.all(Array.from({ length: Math.min(opt.jobs, tests.length) }, async () => {
    for (let t; (t = queue.shift());) {
      const r = await runTest(t);
      results.push(r);
      console.log((r.errors.length ? 'FAIL ' : 'ok   ') + t.name.padEnd(16) + t.about);
    }
  }));
  const failed = tests.map(t => results.find(r => r.t === t)).filter(r => r.errors.length);
  for (const r of results) {
    if (!r.errors.length && !opt.verbose) continue;
    console.log('\n=== ' + r.t.name + (r.errors.length ? ': FAILED' : ''));
    for (const e of r.errors) console.log('  ' + e);
    console.log('  (cd sim; ' + r.cmd + ')\n--- serial output ---\n' + r.out.replace(/\.{8,}/g, '...'));
  }
  const deep = results.flatMap(r => r.stacks.map(s => [...s, r.t.name])).sort((a, b) => a[1] - b[1])[0];
  if (deep) console.log('\nDeepest stack: task ' + deep[0] + ', ' + deep[1] + ' bytes free (at ' + deep[2] + ', test ' + deep[3] + ')');
  fs.rmSync(tmpDir, { recursive: true, force: true });
  console.log('\n' + (tests.length - failed.length) + ' of ' + tests.length + ' tests passed (' +
    ((Date.now() - start) / 1000).toFixed(1) + ' s' + (opt.seed >= 0 ? ', seed ' + opt.seed : ', random power-up') + ')');
  process.exit(failed.length ? 1 : 0);
})();
