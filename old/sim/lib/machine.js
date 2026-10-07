// machine.js - the Hydra-16 itself, for hydrasim.js or anything else (no Node.js: it runs in a browser too):
//   * the W65C02S (cpu65c02.js);
//   * T (task) register: each task has its own $0000-$7FFF (ZP, stack, task RAM) and $00/$01 bank registers;
//   * RAM bank window $8000-$9FFF: banks $00-$EF per task (only the installed modules; others float), banks
//     $F0-$FF shared, 16 macro-pages selected by U;
//   * paged ROM $A000-$DFFF (16K banks by $01), with the A13 half-swap of the board, and the V1 board's swap of the
//     bank number's bits 2 and 3, and 6 and 7;
//   * BIOS ROM $E000-$FFFF (8K pages by W); I/O at $FF00-$FFEF; T/U/V/W at $FFF0-$FFF3;
//   * IRQ vector RAM ($FFFE/F): written at index V[0..3]; read at index IRQ_NUMBER(n) = n ^ 7 of the lowest active
//     IRQ line, or V[0..3] when no line is active (and for BRK);
//   * the devices: the ACIA (port 1, acia.js), the VIA (port 0, via.js) with SD cards on its SPI port (sd.js), the
//     YM2151 (port 4, ym2151.js), a DS1747 in U7 (ds1747.js).
// RAM and the pseudo-registers power up random, like the hardware (seeded: opt.seed >= 0, the same each time).
//
// createMachine(opt): opt.osrom, opt.pagedrom (the images, Uint8Arrays) and the options hydrasim.js documents
// (modules, sharedU, model, ramFault, u7Fault, aciaLine, stuckIrq, acia, paste, input, sd: block devices, rtc,
// rtcBatteryLow, clock, trace, watches, pcWatches, marks, profile, ymLog), and opt.log(text) for the watches and
// marks.  opt.pcHost: what the serial port sends goes through its push(byte, cycle), which gives back the bytes that
// are the console's (the rest are /pc's frames: hydrasim.js --pc-dir).  run(limit) runs to a cycle; the rest is its
// state, for a report.
'use strict';
const { createCpu, FLAGS } = require('./cpu65c02.js');
const { createAcia } = require('./acia.js');
const { createVia } = require('./via.js');
const { createSpi } = require('./sd.js');
const { createYm } = require('./ym2151.js');
const { createRtc, RTC_REGS, RTC_TASK } = require('./ds1747.js');

const hx = (v, n = 2) => v.toString(16).toUpperCase().padStart(n, '0');
// The paged ROM bank $01 selects, as the V1 board wires it: bits 2 and 3, and 6 and 7, trade places before they
// reach the chips (so the image holds bank b at bank swap(b)'s place: sim/tools/mkromdisk.js, os_rom/tools/romsum.js)
const romBank = b => (b & 0x33) | ((b & 0x04) << 1) | ((b & 0x08) >> 1) | ((b & 0x40) << 1) | ((b & 0x80) >> 1);

function createMachine(opt) {
  const log = opt.log || (() => {});
  const osrom = opt.osrom, pagedrom = opt.pagedrom;
  let seed = opt.seed === undefined ? -1 : opt.seed;          // A repeatable power-up (mulberry32)
  const random = seed < 0 ? Math.random : () => { seed = (seed + 0x6D2B79F5) | 0; let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296; };
  const rnd = n => (random() * n) | 0;

  // ---- memory, the pseudo-registers, the devices (in this order: the power-up's random numbers)
  const taskRam = []; for (let i = 0; i < 16; i++) taskRam.push(new Uint8Array(0x8000).map(() => rnd(256)));
  let cpu = null;
  const rtc = opt.rtc !== undefined ? createRtc({ time: opt.rtc, batteryLow: opt.rtcBatteryLow, clock: opt.clock, now: () => cpu.cyc, rnd }) : null;
  const taskBank = {}, sharedBank = {};
  const vecRam = new Uint16Array(16).map(() => rnd(65536));
  // T/U/V/W are 8-bit latches (74F573) read back through a 74F541: a read gives the whole byte written.  Only
  // T0-T3 select the task, U0-U3 the shared macro-page and W0-W3 the BIOS ROM page (the socket wires W0-W5
  // for up to a 512K chip; with a 128K image, W4-W5 fold back, as on a 39SF010)
  let regT = rnd(256), regU = rnd(256), V = rnd(256), regW = rnd(256);
  let T = regT & 15, U = regU & 15, W = regW & 15;
  const m = { out: '' };                                      // The serial output (the ACIA's)
  const acia = createAcia({ clock: opt.clock, wdc: opt.acia === 'wdc', paste: opt.paste, input: opt.input, consoleOnly: !!opt.pcHost,
    onTx: (v, t) => { for (const b of opt.pcHost ? opt.pcHost.push(v, t) : [v]) out(b, t); } });
  function out(v, t) { if (opt.pcHost) acia.shown(v, t); m.out += String.fromCharCode(v); for (const k of opt.marks || []) if (m.out.endsWith(k)) log('mark: ' + JSON.stringify(k) + ' at cycle ' + t); }
  const spi = createSpi(opt.sd || [], opt.spiEcho || []);
  const via = createVia({ portB: spi.portB, miso: spi.miso, portAIn: opt.gpioIn });
  // CA1's pulses (opt.ca1: cycles): low at each, high again 500 cycles on (its edges, in order)
  const ca1Edges = [];
  for (const t of (opt.ca1 || []).slice().sort((a, b) => a - b)) ca1Edges.push([t, 0], [t + 500, 1]);
  let ca1At = 0;
  const ym = createYm({ clock: opt.clock, log: !!opt.ymLog, resetDelay: opt.ymResetDelay || 0 });

  // Which task's copy of $0000-$7FFF an access uses (the model what-ifs change this)
  const model = opt.model || '', u7 = opt.u7Fault, ramFault = opt.ramFault;
  const tsel = a => model === 'sharedlow' ? 0 : (model === 'zponly' && a >= 0x200) ? 0
    : (model === 'nostack' && a >= 0x100 && a < 0x200) ? 0
    : u7 ? (u7.high ? T | u7.mask : T & ~u7.mask) : T;
  const bankInstalled = b => b >= 0xF0 ? (model !== 'noshared' && U < opt.sharedU) : b < opt.modules * 16;
  function bankMem(b) {
    if (b >= 0xF0) { const k = U * 16 + (b & 15); return sharedBank[k] || (sharedBank[k] = new Uint8Array(0x2000)); }
    const k = T * 256 + b; return taskBank[k] || (taskBank[k] = new Uint8Array(0x2000));
  }
  // A RAM fault: the offset in the window that the chip holding bank b actually sees
  const sameChip = (b, f) => b >= 0xF0 ? f >= 0xF0 && ((b ^ f) & 0x0C) === 0 : f < 0xF0 && (b >> 4) === (f >> 4);
  function ramOfs(b, a) {
    const o = a - 0x8000, f = ramFault;
    if (!f || !sameChip(b, f.bank)) return o;
    return f.high ? o | f.mask : o & ~f.mask;
  }
  function rd(a) {
    if (a < 0x8000) return rtc && a >= RTC_REGS && tsel(a) === RTC_TASK ? rtc.read(a - RTC_REGS) : taskRam[tsel(a)][a];
    if (a < 0xA000) { const b = taskRam[tsel(0)][0]; return bankInstalled(b) ? bankMem(b)[ramOfs(b, a)] : (a >> 8); }  // floating bus
    if (a < 0xE000) { const off = romBank(taskRam[tsel(1)][1]) * 0x4000 + ((a - 0xA000) ^ 0x2000); return off < pagedrom.length ? pagedrom[off] : 0xFF; }
    if (a >= 0xFF00 && a < 0xFFF0) {
      const t = cpu.ioAt;
      sync(t);                                                  // (The devices, as of this access's cycle)
      if (a >= 0xFF10 && a < 0xFF14) return acia.read(a - 0xFF10);
      if (a < 0xFF10) return via.read(a - 0xFF00);
      if (a === 0xFF41) return ym.readStatus(t);
      return 0xFF;
    }
    if (a === 0xFFF0) return regT; if (a === 0xFFF1) return regU; if (a === 0xFFF2) return V; if (a === 0xFFF3) return regW;
    if (a === 0xFFFE || a === 0xFFFF) { const v = vecRam[V & 15]; return a === 0xFFFE ? v & 0xFF : v >> 8; }
    return osrom[W * 0x2000 + (a - 0xE000)];
  }
  function wr(a, v) {
    v &= 0xFF;
    if (a < 0x8000) {
      for (const w of opt.watches || []) if (w.addr === a && (w.task < 0 || w.task === tsel(a)))
        log('watch: $' + hx(a, 4) + ' (task ' + hx(tsel(a), 1) + ') ' + hx(taskRam[tsel(a)][a]) + ' -> ' + hx(v) + ' by ' + hx(W, 1) + ':' + hx(cpu.lastPC, 4) + ' at cycle ' + cpu.cyc);
      if (rtc && a >= RTC_REGS && tsel(a) === RTC_TASK) return rtc.write(a - RTC_REGS, v);
      taskRam[tsel(a)][a] = v; return;
    }
    if (a < 0xA000) { const b = taskRam[tsel(0)][0]; if (bankInstalled(b)) bankMem(b)[ramOfs(b, a)] = v; return; }
    if (a < 0xFF00) return;
    const t = cpu.ioAt;
    if (a < 0xFFF0) sync(t);                                    // (The devices, as of this access's cycle)
    if (a >= 0xFF10 && a < 0xFF14) { acia.write(a - 0xFF10, v, t); return; }
    if (a < 0xFF10) { via.write(a - 0xFF00, v); return; }
    if (a === 0xFF40) { ym.select(v); return; }                 // YM2151: register, then data
    if (a === 0xFF41) { ym.write(v, t); return; }
    if (a === 0xFFF0) { regT = v; T = v & 15; return; } if (a === 0xFFF1) { regU = v; U = v & 15; return; }
    if (a === 0xFFF2) { V = v; return; } if (a === 0xFFF3) { regW = v; W = v & 15; return; }
    if (a === 0xFFFE) { vecRam[V & 15] = (vecRam[V & 15] & 0xFF00) | v; return; }
    if (a === 0xFFFF) { vecRam[V & 15] = (vecRam[V & 15] & 0xFF) | (v << 8); return; }
  }
  // Lowest numbered active IRQ line, or -1
  function irqLine() {
    const lines = [];
    if (acia.irqActive()) lines.push(opt.aciaLine);
    if (via.irqActive()) lines.push(0);                         // VIA: IRQ line 0
    if (ym.irqActive()) lines.push(4);                          // YM2151: line 4
    if (opt.stuckIrq >= 0) lines.push(opt.stuckIrq);
    return lines.length ? Math.min(...lines) : -1;
  }
  const irqVector = () => { const n = irqLine(); return vecRam[n >= 0 ? (n ^ 7) : (V & 15)]; };

  // ---- the CPU, and what's watched as it runs
  const stackLow = new Array(16).fill(0x100), stackLowAt = new Array(16).fill(null);    // Per task: lowest S, and where (W:PC, cycle)
  cpu = createCpu({ rd, wr, where: pc => hx(W, 1) + ':' + hx(pc, 4) + ' (task ' + T + ')',
    pushed: s => { if (s < stackLow[T & 15]) { stackLow[T & 15] = s; stackLowAt[T & 15] = [W, cpu.lastPC, cpu.cyc]; } } });
  // The longest stretches with IRQs off (the I flag set), from the first key typed: [cycles, from, to, at]
  let iOffAt = -1, iOffFrom = '';
  const iOffTop = [];
  function iOffNote(n, from, to, at) {
    const k = iOffTop.findIndex(e => e[1] === from);           // (One entry per starting place)
    if (k >= 0) { if (iOffTop[k][0] < n) iOffTop[k] = [n, from, to, at]; }
    else iOffTop.push([n, from, to, at]);
    iOffTop.sort((a, b) => b[0] - a[0]); if (iOffTop.length > 8) iOffTop.pop();
  }
  const trace = [], pcHist = new Map(), profHist = new Map(), profCyc = new Map(), profTask = new Array(16).fill(0);
  let profCount = 0, profCycles = 0;
  cpu.PC = rd(0xFFFC) | (rd(0xFFFD) << 8); cpu.P |= FLAGS.I;  // RESET

  // The devices, brought up to cycle t (at the start of each instruction, and at each I/O access: the access's cycle)
  let devCyc = 0;
  function sync(t) {
    const d = t - devCyc; if (d <= 0) return; devCyc = t;
    via.tick(d);
    while (ca1At < ca1Edges.length && ca1Edges[ca1At][0] <= t) via.ca1(ca1Edges[ca1At++][1]);
    ym.tick(t);
    acia.tick(d, t);
  }
  // Cycles to the next device event (for a WAI: the CPU sleeps until then)
  const nextEvent = () => Math.min(via.nextEvent(), ym.nextEvent(devCyc), acia.nextEvent(),
    ca1At < ca1Edges.length ? Math.max(1, ca1Edges[ca1At][0] - devCyc) : Infinity);

  // Run until cycle limit (or a halt)
  const traceLen = opt.trace === undefined ? 25 : opt.trace, pcWatches = opt.pcWatches || [], profileFrom = opt.profile === undefined ? -1 : opt.profile,
    profileTo = opt.profileTo === undefined ? Infinity : opt.profileTo;
  const I = FLAGS.I;
  function run(limit) {
    while (cpu.cyc < limit && !cpu.halted) {
      sync(cpu.cyc);
      if (irqLine() >= 0) { cpu.waiting = false; if (!(cpu.P & I)) { cpu.interrupt(irqVector()); continue; } }
      if (cpu.waiting) { cpu.cyc += Math.max(1, Math.min(nextEvent(), limit - cpu.cyc)); continue; }
      const PC = cpu.PC, P = cpu.P;
      if (acia.typedAt >= 0) {                                  // IRQs-off stretches, from the first key typed
        if (P & I) { if (iOffAt < 0) { iOffAt = cpu.cyc; iOffFrom = hx(W, 1) + ':' + hx(PC, 4); } }
        else if (iOffAt >= 0) { iOffNote(cpu.cyc - iOffAt, iOffFrom, hx(W, 1) + ':' + hx(cpu.lastPC, 4), iOffAt); iOffAt = -1; }
      }
      trace.push([W, T, PC, cpu.A, cpu.X, cpu.Y, cpu.S, P]); if (trace.length > traceLen) trace.shift();
      for (const w of pcWatches) if (w.pc === PC && (w.page < 0 || w.page === W))
        log('pc: ' + hx(W, 1) + ':' + hx(PC, 4) + ' T=' + hx(T, 1) + ' A=' + hx(cpu.A) + ' X=' + hx(cpu.X) + ' Y=' + hx(cpu.Y) + ' S=' + hx(cpu.S) + ' P=' + hx(P) + ' at cycle ' + cpu.cyc);
      const pT = T, pW = W, c0 = cpu.cyc;                      // (The profile's: the instruction's task, page and cycles)
      cpu.step(irqVector);
      const k = W * 65536 + cpu.PC; pcHist.set(k, (pcHist.get(k) || 0) + 1);
      if (profileFrom >= 0 && c0 >= profileFrom && c0 < profileTo) {
        const pk = pT * 1048576 + pW * 65536 + cpu.lastPC, dc = cpu.cyc - c0;
        profHist.set(pk, (profHist.get(pk) || 0) + 1); profCyc.set(pk, (profCyc.get(pk) || 0) + dc); profTask[pT]++; profCount++; profCycles += dc;
      }
    }
  }

  // The reset button (RESB): the CPU, the VIA, the ACIA and the YM2151 reset; RAM, the pseudo-registers
  // (plain latches) and the SD cards keep their state, as on the board
  function hwReset() {
    via.reset();
    acia.reset();
    ym.reset();
    cpu.reset();
  }

  Object.assign(m, { cpu, acia, via, ym, rtc, taskRam, vecRam, trace, pcHist, iOffTop, stackLow, stackLowAt, profHist, profCyc, profTask, run, hwReset, rd });
  Object.defineProperties(m, {                                // (The pseudo-registers and the profile's count, as they are now)
    T: { get: () => T }, U: { get: () => U }, V: { get: () => V }, W: { get: () => W }, profCount: { get: () => profCount }, profCycles: { get: () => profCycles },
  });
  return m;
}

module.exports = { createMachine, romBank };
