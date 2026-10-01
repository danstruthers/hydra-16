#!/usr/bin/env node
// ****************************************************************************
// hydrasim.js - a Hydra-16 emulator, for debugging the OS ROM without the hardware: the command line (its options,
// the image files, the report, the interactive terminal).  The machine is in lib/ (machine.js and its devices,
// with no Node.js in them: docs/tools/emulator.md, "Inside the emulator").
//
// Models:
//   * W65C02S CPU (STZ, BRA, PHX/PLY, TSB/TRB, BBR/BBS, RMB/SMB, WAI, STP, (zp) mode, the 1-byte NOPs, ...),
//     cycle-counted from WDC's table: +1 for an indexed read crossing a page, +1 / +2 for a branch taken
//     (to another page), +1 for ADC/SBC in decimal mode; 7 for an interrupt.  No wait states (the board's
//     RDY only has a pull-up), so every access is at full speed.  I/O accesses happen at the instruction's
//     last cycle (the devices are brought up to it); WAI sleeps until the next device event.
//   * T (task) register: each task has its own $0000-$7FFF (ZP, stack, task RAM) and $00/$01 bank registers
//   * RAM bank window $8000-$9FFF: banks $00-$EF per task (only the installed modules; others float),
//     banks $F0-$FF shared, 16 macro-pages selected by U
//   * Paged ROM $A000-$DFFF (paged_rom_C02.bin, 16K banks by $01), with the A13 half-swap of the board
//   * BIOS ROM $E000-$FFFF (os_rom_C02.bin, 8K pages by W); I/O at $FF00-$FFEF; T/U/V/W at $FFF0-$FFF3
//   * IRQ vector RAM ($FFFE/F): written at index V[0..3]; read at index IRQ_NUMBER(n) = n ^ 7 of the lowest
//     active IRQ line, or V[0..3] when no line is active (and for BRK)
//   * Rockwell 65C51 ACIA at $FF10 (IRQ line 1): TX/RX with IRQs, output captured, input from --input; a
//     character takes the time its baud rate, word length, parity and stop bits give (its 1.790 MHz clock)
//   * VIA timer 1 (one-shot / free-running, IFR/IER) on IRQ line 0: the scheduler's tick; timer 2
//     (one-shot); the shift register's timing and flag; port B's SPI; port A's inputs read high (the I2C
//     pull-ups); other VIA registers are plain storage
//   * YM2151: busy for 64 of its clocks (3.58 MHz) after each data write (writes while it's busy are
//     counted: lost on the chip); key-ons (register $08) are counted and reported; timers A and B, their
//     status flags and IRQ line 4
//   RAM and the pseudo-registers power up random, like the hardware.
//
// Usage: node hydrasim.js [options]
//   -i, --interactive   Use the Hydra from this terminal: it runs in real time, the keys typed go to its
//                       serial port and its output comes straight back (no report at the end; runs until
//                       quit, or --cycles).  Ctrl-A is the emulator's prefix: Ctrl-A x quits, Ctrl-A r
//                       presses the reset button, Ctrl-A s shows the state, Ctrl-A h lists them, Ctrl-A
//                       Ctrl-A types a Ctrl-A.  With piped input, it stops 3 s after the input runs out.
//                       E.g. node hydrasim.js -i --sd card.img
//   --speed N           Interactive: N times real time (default 1; 0 = as fast as the PC can go)
//   --rom DIR           ROM images directory (default: ../os_rom/bin next to this script)
//   --cycles N          CPU cycles to run (default 20000000; ~5.6 s at 3.58 MHz)
//   --clock 3.58|7.16   The CPU clock in MHz, as the ROM was built for (CPU_CLOCK_MULT; default 3.58): it
//                       sets the ACIA's and the YM2151's timing in CPU cycles, and the report's seconds
//   --input TEXT        Serial input to type, from cycle 200000, a key every 20000 cycles; "\r" = CR,
//                       "\xNN" = byte NN, e.g. a control key; "\w" = wait 2M cycles before the next key; "\p" = wait
//                       for a prompt: until the output (grown since) ends with "> ", or WOZMON's ">" (e.g. --input
//                       "1 2 + .\r", "cat\rhi\x04", "inf\r\w\x03", "\pls\r\pcd /rom\r\p")
//   --stop-after-input N   Stop N cycles after the last key of --input is typed (and its waits are done), or at
//                       --cycles, whichever comes first
//   --modules N         RAM modules installed: banks $00 - N*16-1 (default 3)
//   --shared-u N        Shared RAM installed for U macro-pages 0 - N-1 (default 16; 4 per 512K chip)
//   --acia-line N       IRQ line the ACIA interrupts on (default 1)
//   --acia rockwell|wdc The ACIA: Rockwell R65C51 (default: TDRE status and TX interrupt), or WDC W65C51N
//                       (its bug: TDRE always reads 1, no TX interrupt; bytes written while one is still
//                       being sent are counted: they'd be garbled on the chip)
//   --stuck-irq N       Hold IRQ line N active all the time
//   --ram-fault BANK:An:high|low   Address line An (0-12) stuck high/low on the RAM chip holding BANK (a
//                       shared chip holds 4 bank IDs, e.g. F0-F3; a task RAM module 16 banks), e.g. F0:A0:high
//   --u7-fault An:high|low   Task RAM line An (15-18: T0-T3, which task's 32K) stuck high/low at U7, e.g. A17:low
//                       (tasks that differ in that bit share their RAM; here their bank registers too, which the board keeps apart)
//   --model M           Hardware what-ifs: sharedlow (T doesn't switch $0000-$7FFF), nostack (stack page
//                       not per task), zponly (only ZP per task), noshared (no shared RAM)
//   --sd [N:]FILE[@B]   An SD card (SDHC) on SPI device N (0-7; default 0), backed by the image FILE
//                       (512-byte blocks; writes go to the file).  Up to 8, one per device
//                       (e.g. --sd card0.img --sd 3:C:/images/card3.img).  @B: the card says it has B
//                       blocks, more than the file (a big card from a small file: blocks past the file's end
//                       read as zeros, and a write there makes the file longer)
//   --rtc TIME|now|stopped|unset   A DS1747 in U7 (a 512K task RAM with a clock): its clock registers are
//                       task F's $7FF8-$7FFF.  TIME (YYYY-MM-DDThh:mm[:ss]) or now (this PC's time): the time it has
//                       at power-up, running; stopped: its oscillator off (OSC set), at 2000-01-01; unset: its
//                       registers hold junk, as a part never set may.  Without it, U7 is a plain HM628512
//   --rtc-battery-low   The DS1747's battery flag (BF) reads 0: its battery is flat
//   --paste             Type the --input (and interactive input) at the ACIA's full line rate, back to back like a
//                       paste, whether the ROM keeps up or not: bytes arriving while the last is still unread are
//                       lost, as on the chip, and counted in the report (default: the next key waits for it)
//   --raw               Print serial output as-is (default shows ESC as <ESC>)
//   --trace N           Show the last N instructions (default 25)
//   --dump ADDR[:LEN][@TASK]   Hex dump task RAM after the run (e.g. --dump 7D90:16@1)
//   --watch ADDR[@TASK] Report every write to task RAM address ADDR (value, and the PC that wrote it)
//   --pc [PAGE:]ADDR    Report the registers every time the PC reaches ADDR (on ROM page PAGE, if given)
//   --mark TEXT         Report the cycle each time the serial output ends with TEXT ("\r" = CR), e.g. a prompt
//   --seed N            Power up RAM and the pseudo-registers from random number seed N (default: a new
//                       random power-up each run), so a run can be repeated exactly
//   --ym-log            List every YM2151 key-on (channel and cycle), not just the first 8
//   --ym-dump           Show the YM2151's registers at the end (as the chip has them: its levels with the volumes)
//   --ym-vgm FILE       Write what the ROM wrote to the YM2151 as a VGM file (with the time between writes), to
//                       hear it in any VGM player (VGMPlay, foobar2000 with its VGM plugin ...)
//   --profile N         From cycle N on, count the instructions run in each routine (named from the
//                       build's debug info, ../os_rom/obj/os_rom_C02.dbg) and in each task, and report them
//
// Output: serial output, the last instructions (W T PC A X Y S P), the hottest PCs (useful to find a
// loop the code is stuck in), and final state.
// ****************************************************************************
'use strict';
const fs = require('fs');
const path = require('path');
const { createMachine } = require('./lib/machine.js');

// ---- options
const opt = { rom: path.join(__dirname, '..', 'os_rom', 'bin'), cycles: 20000000, input: '', modules: 3,
  aciaLine: 1, stuckIrq: -1, model: '', raw: false, trace: 25, dumps: [], watches: [], pcWatches: [], sharedU: 16, ramFault: null, sds: [], sdsc: [], interactive: false, speed: 1, acia: 'rockwell', marks: [], profile: -1, seed: -1, ymLog: false, clock: 3.579545 };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const a = argv[i], next = () => argv[++i];
  switch (a) {
    case '--rom': opt.rom = next(); break;
    case '--cycles': opt.cycles = +next(); opt.cyclesSet = true; break;
    case '-i': case '--interactive': opt.interactive = true; break;
    case '--speed': opt.speed = +next(); if (!(opt.speed >= 0)) { console.error('--speed N (0 = as fast as it goes)'); process.exit(1); } break;
    case '--input': opt.input = next().replace(/\\r/g, '\r').replace(/\\n/g, '\n').replace(/\\x([0-9A-Fa-f]{2})/g, (m, h) => String.fromCharCode(parseInt(h, 16))).replace(/\\w/g, '\u0100').replace(/\\p/g, '\u0101'); break;
    case '--stop-after-input': opt.stopAfterInput = +next(); break;
    case '--modules': opt.modules = +next(); break;
    case '--shared-u': opt.sharedU = +next(); break;
    case '--acia-line': opt.aciaLine = +next(); break;
    case '--acia': opt.acia = next().toLowerCase(); if (!/^(rockwell|wdc)$/.test(opt.acia)) { console.error('--acia rockwell|wdc'); process.exit(1); } break;
    case '--stuck-irq': opt.stuckIrq = +next(); break;
    case '--u7-fault': { const m = /^A(1[5-8]):(high|low)$/i.exec(next());
      if (!m) { console.error('--u7-fault A15|A16|A17|A18:high|low'); process.exit(1); }
      opt.u7Fault = { mask: 1 << (+m[1] - 15), high: m[2].toLowerCase() === 'high' }; break; }
    case '--model': opt.model = next(); break;
    case '--ram-fault': { const m = /^([0-9A-Fa-f]{1,2}):A(\d+):(high|low)$/i.exec(next()); opt.ramFault = { bank: parseInt(m[1], 16), mask: 1 << +m[2], high: m[3].toLowerCase() === 'high' }; break; }
    case '--sd': { let f = next(); const b = /^(.+)@(\d+)$/.exec(f); if (b) f = b[1];
      const m = /^([0-7]):(.+)$/.exec(f), dev = m ? +m[1] : 0;
      if (opt.sds.some(c => c.dev === dev)) { console.error('Two SD cards on device ' + dev); process.exit(1); }
      opt.sds.push({ dev, file: m ? m[2] : f, blocks: b ? +b[2] : 0 }); break; }
    case '--sdsc': opt.sdsc.push(+next()); break;
    case '--raw': opt.raw = true; break;
    case '--rtc': { const s = next(), m = /^(\d{4})-(\d\d)-(\d\d)[T ](\d\d):(\d\d)(?::(\d\d))?$/.exec(s);
      if (!m && !/^(now|stopped|unset)$/.test(s)) { console.error('--rtc YYYY-MM-DDThh:mm[:ss] | now | stopped | unset'); process.exit(1); }
      opt.rtc = m ? Date.UTC(+m[1], m[2] - 1, +m[3], +m[4], +m[5], +(m[6] || 0)) / 1000 : s; break; }
    case '--rtc-battery-low': opt.rtcBatteryLow = true; break;
    case '--paste': opt.paste = true; break;
    case '--trace': opt.trace = +next(); break;
    case '--dump': opt.dumps.push(next()); break;
    case '--pc': { const m = /^(?:([0-9A-Fa-f]):)?([0-9A-Fa-f]+)$/.exec(next()); opt.pcWatches.push({ pc: parseInt(m[2], 16), page: m[1] === undefined ? -1 : parseInt(m[1], 16) }); break; }
    case '--mark': opt.marks.push(next().replace(/\\r/g, '\r').replace(/\\n/g, '\n')); break;
    case '--profile': opt.profile = +next(); break;
    case '--seed': opt.seed = +next() >>> 0; break;
    case '--ym-log': opt.ymLog = true; break;
    case '--ym-dump': opt.ymDump = true; break;
    case '--ym-vgm': opt.ymVgm = next(); break;
    case '--clock': { const c = next(); opt.clock = /^7/.test(c) ? 7.15909 : 3.579545; if (!/^(3\.58|7\.16)$/.test(c)) { console.error('--clock 3.58|7.16'); process.exit(1); } break; }
    case '--watch': { const m = /^([0-9A-Fa-f]+)(?:@([0-9A-Fa-f]))?$/.exec(next()); opt.watches.push({ addr: parseInt(m[1], 16), task: m[2] === undefined ? -1 : parseInt(m[2], 16) }); break; }
    default: console.error('Unknown option: ' + a + ' (see the header of hydrasim.js)'); process.exit(1);
  }
}
const hx = (v, n = 2) => v.toString(16).toUpperCase().padStart(n, '0');

// ---- the machine (lib/machine.js), with the ROM images and SD card images from files
const sd = opt.sds.map(({ dev, file, blocks }) => {             // A card's blocks: its image file (512 bytes a block)
  const fd = fs.openSync(file, 'r+');
  return { dev, sdsc: opt.sdsc.includes(dev), blocks: blocks || Math.floor(fs.fstatSync(fd).size / 512),
    read: n => { const b = Buffer.alloc(512); fs.readSync(fd, b, 0, 512, n * 512); return b; },
    write: (n, data) => { fs.writeSync(fd, data, 0, 512, n * 512); } };
});
const m = createMachine(Object.assign({}, opt, {
  osrom: fs.readFileSync(path.join(opt.rom, 'os_rom_C02.bin')),
  pagedrom: fs.readFileSync(path.join(opt.rom, 'paged_rom_C02.bin')),
  sd, ymLog: !!opt.ymVgm, log: t => console.log(t),
}));
const { cpu, acia, ym, rtc } = m;

// ---- report
function report() {
console.log('--- serial output ---\n' + (opt.raw ? m.out : m.out.replace(/\x1b/g, '<ESC>')));
if (cpu.halted) console.log('--- halted: ' + cpu.halted);
console.log('--- last instructions (W T PC   A  X  Y  S  P) ---');
for (const [w, t, pc, a, x, y, s, p] of m.trace) console.log(hx(w, 1), hx(t, 1), hx(pc, 4), hx(a), hx(x), hx(y), hx(s), hx(p));
if (m.iOffTop.length) console.log('--- longest with IRQs off, from the first key typed (cycles: from -> to, at cycle): ' +
  m.iOffTop.map(([n, a, b, at]) => n + ': ' + a + ' -> ' + b + ' at ' + at).join(', '));
if (opt.paste) console.log('--- ACIA: ' + acia.rxLost + ' received byte(s) lost (they arrived while the last one was still unread)');
if (acia.gapMin < Infinity) console.log('--- ACIA: shortest idle between characters sent: ' + acia.gapMin.toFixed(2) + ' bits');
if (acia.wdc) console.log('--- WDC ACIA: ' + acia.overruns + ' byte(s) written while one was still being sent (garbled on the chip)');
console.log('--- hottest PCs (W:PC count) ---');
for (const [k, c] of [...m.pcHist.entries()].sort((a, b) => b[1] - a[1]).slice(0, 10)) console.log(hx(k >> 16, 1) + ':' + hx(k & 0xFFFF, 4), c);
const keyOns = ym.keyOns;
if (keyOns.length) console.log('--- YM2151 key-ons: ' + keyOns.length + ' (' + (opt.ymLog ? keyOns : keyOns.slice(0, 8)).join(', ') + (keyOns.length > 8 && !opt.ymLog ? ', ...' : '') + ')');
if (keyOns.length > 1) { const t = keyOns.map(k => +k.split(' ').pop()); let g = 0, at = 0;   // (Timing: a late note shows as a long gap)
  for (let i = 1; i < t.length; i++) if (t[i] - t[i - 1] > g) { g = t[i] - t[i - 1]; at = t[i - 1]; }
  console.log('--- YM2151 longest gap between key-ons: ' + g + ' cycles, after cycle ' + at + '; first to last: ' + (t[t.length - 1] - t[0]) + ' cycles'); }
if (opt.profile >= 0) profileReport();
console.log('--- lowest stack pointer by task (free bytes; W:PC at the time): ' + m.stackLow.map((v, t) => v > 0xFF ? null : hx(t, 1) + ':' + hx(v) + ' (' + (v + 1) + '; ' + hx(m.stackLowAt[t][0], 1) + ':' + hx(m.stackLowAt[t][1], 4) + ')').filter(x => x).join(', '));
if (ym.lost) console.log('--- YM2151: ' + ym.lost + ' data write(s) while it was busy (lost on the chip)');
if (opt.ymDump) {                                               // --ym-dump: the chip's registers, 16 a line
  console.log('--- YM2151 registers ---');
  for (let r = 0; r < 256; r += 16) console.log(hx(r) + ': ' + [...ym.regs.slice(r, r + 16)].map(v => hx(v)).join(' '));
}
if (opt.ymVgm) ymVgm(opt.ymVgm);
if (rtc) {                                                      // --rtc: the DS1747's registers as they are
  const r = rtc.regs(), h = n => hx(r[n]);
  console.log('--- DS1747: ' + (rtc.junk ? 'junk ' + r.map(v => hx(v)).join(' ') : hx(r[0] & 0x3F) + h(7) + '-' + h(6) + '-' + h(5) + ' ' + h(3) + ':'
    + h(2) + ':' + hx(r[1] & 0x7F) + ' day ' + r[4]) + (rtc.osc ? '' : ', stopped (OSC)') + (rtc.ctl ? ', control bits ' + hx(rtc.ctl) + ' left set' : ''));
}
console.log('--- cycles ' + cpu.cyc + ' (' + (cpu.cyc / (opt.clock * 1e6)).toFixed(3) + ' s at ' + opt.clock.toFixed(2) + ' MHz), T=' + hx(m.T, 1) + ' U=' + hx(m.U, 1) + ' V=' + hx(m.V) + ' W=' + hx(m.W, 1) + ', ACIA control ' + hx(acia.ctrl) + ' command ' + hx(acia.cmd) + ', vector RAM: ' + [...m.vecRam].map(v => hx(v, 4)).join(' '));
for (const d of opt.dumps) {
  const r = /^([0-9A-Fa-f]+)(?::(\d+))?(?:@([0-9A-Fa-f]))?$/.exec(d);
  if (!r) { console.log('bad --dump ' + d); continue; }
  const start = parseInt(r[1], 16), len = +(r[2] || 16), task = parseInt(r[3] || '0', 16);
  console.log('--- task ' + hx(task, 1) + ' $' + hx(start, 4) + ':');
  for (let a = start; a < start + len; a += 16)
    console.log(hx(a, 4) + ': ' + [...m.taskRam[task].slice(a, Math.min(a + 16, start + len, 0x8000))].map(v => hx(v)).join(' '));
}
}

// --ym-vgm: the YM2151's writes as a VGM 1.51 file (YM2151 at 3,579,545 Hz; waits in 44,100ths of a second)
function ymVgm(file) {
  const data = [], writes = ym.writes;
  let at = writes.length ? writes[0][0] : 0, samples = 0, owed = 0;
  const hz = opt.clock * 1e6;
  for (const [c, reg, val] of writes) {
    owed += (c - at) * 44100 / hz; at = c;
    let n = Math.floor(owed); owed -= n; samples += n;
    while (n > 0) { const w = Math.min(n, 65535); data.push(0x61, w & 255, w >> 8); n -= w; }
    data.push(0x54, reg, val);
  }
  data.push(0x66);
  const b = Buffer.alloc(0x100 + data.length);
  b.write('Vgm ', 0, 'latin1');
  b.writeUInt32LE(b.length - 4, 0x04);
  b.writeUInt32LE(0x151, 0x08);
  b.writeUInt32LE(samples, 0x18);
  b.writeUInt32LE(3579545, 0x30);
  b.writeUInt32LE(0x100 - 0x34, 0x34);
  Buffer.from(data).copy(b, 0x100);
  fs.writeFileSync(file, b);
  console.log('--- YM2151: ' + writes.length + ' writes, ' + (samples / 44100).toFixed(2) + ' s, to ' + file);
}

// ---- profile report: instructions per routine (the nearest label at or below the PC, on its ROM page)
function profileReport() {
  const dbgFile = path.join(opt.rom, '..', 'obj', 'os_rom_C02.dbg');
  const lists = {};                                             // 'P0'-'PF' (BIOS ROM pages), 'A' (paged ROM), 'R' (RAM)
  if (fs.existsSync(dbgFile)) {
    const segs = {};
    for (const line of fs.readFileSync(dbgFile, 'utf8').split(/\r?\n/)) {
      const f = {}; for (const r of line.matchAll(/(\w+)=("[^"]*"|[^,\t]*)/g)) f[r[1]] = r[2].replace(/"/g, '');
      if (line.startsWith('seg\t')) {
        let key = null;
        if (/os_rom_C02\.bin$/.test(f.oname || '')) key = 'P' + hx(Math.floor(+f.ooffs / 0x2000), 1);
        else if (f.name === 'FORTH_CODE') key = 'R';
        else if (f.name === 'FORTH_PAGED_ROM') key = 'A';
        segs[f.id] = key;
      } else if (line.startsWith('sym\t') && f.type === 'lab' && f.seg !== undefined && !/^@/.test(f.name)) {
        const key = segs[f.seg]; if (!key) continue;
        (lists[key] = lists[key] || []).push([parseInt(f.val, 16), f.name]);
      }
    }
    for (const k in lists) lists[k].sort((a, b) => a[0] - b[0]);
  } else console.log('(no ' + dbgFile + ': routines by address)');
  const nameOf = (w, pc) => {
    if (pc >= 0xFD00 && pc < 0xFE00 && w !== 0) return hx(w, 1) + ':' + nameOf(0, pc).slice(2);   // (COMMON: page 0's labels)
    const key = pc >= 0xE000 ? 'P' + hx(w, 1) : pc >= 0xA000 ? 'A' : pc < 0x8000 ? 'R' : null;
    const list = key && lists[key];
    let best = null;
    if (list) for (const [v, n] of list) { if (v <= pc) best = n; else break; }
    return (key === 'R' ? 'RAM:' : key === 'A' ? 'PROM:' : hx(w, 1) + ':') + (best || hx(pc, 4));
  };
  const byName = new Map();
  for (const [k, c] of m.profHist) { const n = "T" + hx(Math.floor(k / 1048576), 1) + " " + nameOf((k >> 16) & 15, k & 0xFFFF); byName.set(n, (byName.get(n) || 0) + c); }
  console.log('--- profile: ' + m.profCount + ' instructions from cycle ' + opt.profile + ' (by task: ' +
    m.profTask.map((c, t) => c ? hx(t, 1) + ' ' + (100 * c / m.profCount).toFixed(1) + '%' : '').filter(x => x).join(', ') + ') ---');
  for (const [n, c] of [...byName.entries()].sort((a, b) => b[1] - a[1]).slice(0, 30))
    console.log((100 * c / m.profCount).toFixed(1).padStart(5) + '%  ' + String(c).padStart(9) + '  ' + n);
}

// ---- run: in one go with a report (batch), or live on the terminal (--interactive)
if (!opt.interactive) {
  if (opt.stopAfterInput >= 0) {                                // (In steps: stop once the input's done, and that long after)
    while (cpu.cyc < opt.cycles && !cpu.halted && (acia.rxQueue.length || acia.typedLast < 0)) m.run(Math.min(opt.cycles, cpu.cyc + 100000));
    m.run(Math.min(opt.cycles, Math.max(cpu.cyc, acia.typedLast + opt.stopAfterInput)));
  } else m.run(opt.cycles);
  report();
}
else interactive();

// Interactive: the terminal is the Hydra's serial terminal.  The machine runs in real time (--speed), keys
// go to the ACIA as they're typed, and its output goes straight to the terminal.  Ctrl-A is the emulator's
// own prefix key (as in QEMU or screen): Ctrl-A x quits, Ctrl-A r presses the reset button, Ctrl-A s shows
// the machine's state, Ctrl-A h lists them, Ctrl-A Ctrl-A types a Ctrl-A.
function interactive() {
  const cps = opt.clock * 1e6, stdin = process.stdin, stdout = process.stdout, tty = stdin.isTTY;
  const now = () => Number(process.hrtime.bigint()) / 1e9;
  if (!opt.cyclesSet) opt.cycles = Infinity;
  let sent = 0, prefix = false, quit = '', eof = false, stopAt = Infinity;
  let baseT = now(), baseC = cpu.cyc;
  const say = t => stdout.write('\r\n[hydrasim] ' + t + '\r\n');
  const status = () => 'cycle ' + cpu.cyc + ' (' + (cpu.cyc / cps).toFixed(1) + ' s at ' + opt.clock.toFixed(2) + ' MHz' +
    (opt.speed ? (opt.speed !== 1 ? ', ' + opt.speed + 'x real time' : '') : ', as fast as it goes') + '), task ' + hx(m.T, 1) +
    ', ROM page ' + hx(m.W, 1) + ', PC ' + hx(cpu.PC, 4) + (cpu.waiting ? ' (WAI: idle)' : '') +
    (opt.sds.length ? ', SD: ' + opt.sds.map(c => c.dev + ':' + path.basename(c.file)).join(' ') : '');
  const help = () => say('Ctrl-A then: x quit, r reset (the reset button), s status, h this help, Ctrl-A a Ctrl-A.  ' +
    'Everything else goes to the Hydra (Ctrl-C breaks, Ctrl-] switches tasks, Ctrl-D ends input).');

  function onKey(b) {
    if (prefix) {
      prefix = false;
      const k = String.fromCharCode(b).toLowerCase();
      if (b === 1) acia.type('\x01');
      else if (k === 'x' || k === 'q') quit = 'quit (Ctrl-A x)';
      else if (k === 'r') { m.hwReset(); say('reset'); }
      else if (k === 's') say(status());
      else help();
      return;
    }
    if (b === 1) { prefix = true; return; }
    if (!tty && b === 0x0A) b = 0x0D;                           // (Piped text: a line ends in CR, as Enter sends)
    acia.type(String.fromCharCode(b));
  }
  if (tty) stdin.setRawMode(true);
  stdin.on('data', buf => { for (const b of buf) if (!(!tty && b === 0x0D)) onKey(b); });
  stdin.on('end', () => { eof = true; });
  stdin.resume();

  function flush() {
    if (sent < m.out.length) { stdout.write(m.out.slice(sent)); sent = m.out.length; }
    if (m.out.length > 1 << 16) { m.out = m.out.slice(-1024); sent = m.out.length; }   // (Keep only a tail for --mark)
  }
  function finish(why) {
    flush();
    say('stopped: ' + why + '; ' + status());
    if (tty) stdin.setRawMode(false);
    process.exit(cpu.halted ? 1 : 0);
  }
  function tick() {
    const t = now();
    if (opt.speed > 0) {
      let target = baseC + (t - baseT) * cps * opt.speed;
      if (target - cpu.cyc > cps * opt.speed * 0.25) { baseT = t; baseC = cpu.cyc; target = cpu.cyc + cps * opt.speed * 0.01; }   // (A slow host: don't race to catch up)
      m.run(Math.min(target, opt.cycles, stopAt));
    } else while (now() - t < 0.02 && !cpu.halted && cpu.cyc < Math.min(opt.cycles, stopAt)) m.run(Math.min(cpu.cyc + 200000, opt.cycles, stopAt));
    flush();
    if (eof && !acia.rxQueue.length && stopAt === Infinity) stopAt = cpu.cyc + cps * 3;   // Piped input used up: 3 s more, then stop
    if (cpu.halted) return finish('halted: ' + cpu.halted);
    if (quit) return finish(quit);
    if (cpu.cyc >= opt.cycles) return finish('--cycles reached');
    if (cpu.cyc >= stopAt) return finish('end of input');
    setTimeout(tick, opt.speed > 0 ? 4 : 0);
  }
  say('interactive: the Hydra\'s serial console.  Ctrl-A x quits, Ctrl-A h for help.');
  tick();
}
