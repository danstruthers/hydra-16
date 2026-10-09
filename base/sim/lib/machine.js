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
//   * the devices: the ACIA (port 1, acia.js), the VIA (port 0, via.js) with SD cards on its SPI port (sd.js) and
//     an I2C bus on port A (i2c.js: opt.i2c, its devices), the YM2151 (port 4, ym2151.js), a DS1747 in U7
//     (ds1747.js), a Vera X card in slot 0 (vera.js: opt.vera, true or its options; ports 2 and 3, IRQ line 2), and
//     its input controller, the X16's SMC on the I2C bus at $42 (smc.js: opt.smc, true or its options; keys typed at
//     it from the input's \u0102 to its \u0103, acia.js).
// RAM and the pseudo-registers power up random, like the hardware (seeded: opt.seed >= 0, the same each time).
//
// createMachine(opt): opt.osrom, opt.pagedrom (the images, Uint8Arrays) and the options hydrasim.js documents
// (modules, sharedU, model, ramFault, u7Fault, aciaLine, stuckIrq, acia, paste, input, sd: block devices, rtc,
// rtcBatteryLow, clock, trace, watches, pcWatches, marks, profile, ymLog, vera), and opt.log(text) for the watches and
// marks.  opt.sound: the chips' sound made (audio.js: m.audio, its listeners given what each run made).  opt.pcHost: what the serial port sends goes through its push(byte, cycle), which gives back the bytes that
// are the console's (the rest are /pc's frames: pchost.js, run.js --pc-dir), and its send(bytes) is what the PC
// sends.  opt.pcHist: count the instructions run at each page:PC (pcHist).  run(limit) runs to a cycle; the rest is
// its state, for a report.  For a debugger: opt.breaks ({ pc, page, bank, banks }: -1 for any; bank, the first of
// banks of the task's paged ROM bank): run stops before the instruction at one, m.breakHit says which, and m.skipBreak = true goes past it the next
// time; opt.readWatches ({ addr, task }), as opt.watches are for writes; opt.calls (a Map: a jump table slot's
// address -> its call's name), opt.errNames (code -> name) and opt.callFilter (a Set of names and "tN", or none):
// each call a program makes logged, with its registers, and what it gives back as it returns.  The loop that runs each instruction allocates nothing, so the emulator runs as fast as the
// host can (about 20 MHz of the Hydra's cycles on a 2019 desktop): keep it that way.
'use strict';
const { createCpu, FLAGS } = require('./cpu65c02.js');
const { createAcia } = require('./acia.js');
const { createVia } = require('./via.js');
const { createI2c } = require('./i2c.js');
const { createSmc } = require('./smc.js');
const { createSpi } = require('./sd.js');
const { createYm } = require('./ym2151.js');
const { createRtc, RTC_REGS, RTC_TASK } = require('./ds1747.js');
const { createVera } = require('./vera.js');
const { createAudio } = require('./audio.js');

const hx = (v, n = 2) => v.toString(16).toUpperCase().padStart(n, '0');
const LINES = 16;                                             // The IRQ lines (0 the highest priority)
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
  const smc = opt.smc ? createSmc(Object.assign({ log }, opt.smc === true ? {} : opt.smc)) : null;
  const acia = createAcia({ clock: opt.clock, wdc: opt.acia === 'wdc', paste: opt.paste, input: opt.input, consoleOnly: !!opt.pcHost,
    keyboard: smc ? c => smc.type(c) : null,
    onTx: (v, t) => { for (const b of opt.pcHost ? opt.pcHost.push(v, t) : [v]) out(b, t); } });
  function out(v, t) { if (opt.pcHost) acia.shown(v, t); m.out += String.fromCharCode(v); for (const k of opt.marks || []) if (m.out.endsWith(k)) log('mark: ' + JSON.stringify(k) + ' at cycle ' + t); }
  if (opt.pcHost) opt.pcHost.send = bytes => acia.send(bytes);   // (The PC's replies: on the line, at its rate)
  const spi = createSpi(opt.sd || [], opt.spiEcho || []);
  const i2c = opt.i2c || smc ? createI2c({ devices: Object.assign({}, opt.i2c, smc ? { 0x42: smc } : {}) }) : null;   // (opt.i2c: { address: size }, memories)
  const via = createVia({ portB: spi.portB, miso: spi.miso, portAIn: opt.gpioIn, i2c });
  // CA1's pulses (opt.ca1: cycles): low at each, high again 500 cycles on (its edges, in order)
  const ca1Edges = [];
  for (const t of (opt.ca1 || []).slice().sort((a, b) => a - b)) ca1Edges.push([t, 0], [t + 500, 1]);
  let ca1At = 0;
  const ym = createYm({ clock: opt.clock, log: !!opt.ymLog, resetDelay: opt.ymResetDelay || 0 });
  const vera = opt.vera ? createVera(Object.assign({ clock: opt.clock, rnd }, opt.vera === true ? {} : opt.vera)) : null;   // (Its VRAM: random)
  const audio = opt.sound ? createAudio({ ym, vera }) : null;   // (The sound: what each run makes, mixed)

  // Which task's copy of $0000-$7FFF an access uses (the model what-ifs change this)
  const model = opt.model || '', u7 = opt.u7Fault, ramFault = opt.ramFault, plain = !model && !u7;
  const tsel = a => plain ? T : model === 'sharedlow' ? 0 : (model === 'zponly' && a >= 0x200) ? 0
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
  const readWatches = opt.readWatches || [], rWatch = readWatches.length > 0;
  function rd(a) {
    if (a < 0x8000) {
      if (rWatch) for (const w of readWatches) if (w.addr === a && (w.task < 0 || w.task === tsel(a)))
        log('read: $' + hx(a, 4) + ' (task ' + hx(tsel(a), 1) + ') ' + hx(taskRam[tsel(a)][a]) + ' by ' + hx(W, 1) + ':' + hx(cpu.PC, 4) + ' at cycle ' + cpu.cyc);
      return rtc && a >= RTC_REGS && tsel(a) === RTC_TASK ? rtc.read(a - RTC_REGS) : taskRam[tsel(a)][a];
    }
    if (a < 0xA000) { const b = taskRam[tsel(0)][0]; return bankInstalled(b) ? bankMem(b)[ramOfs(b, a)] : (a >> 8); }  // floating bus
    if (a < 0xE000) { const off = romBank(taskRam[tsel(1)][1]) * 0x4000 + ((a - 0xA000) ^ 0x2000); return off < pagedrom.length ? pagedrom[off] : 0xFF; }
    if (a >= 0xFF00 && a < 0xFFF0) {
      const t = cpu.ioAt;
      sync(t);                                                  // (The devices, as of this access's cycle)
      if (a >= 0xFF10 && a < 0xFF14) return acia.read(a - 0xFF10, t);
      if (a < 0xFF10) return via.read(a - 0xFF00);
      if (a === 0xFF41) return ym.readStatus(t);
      if (vera && a >= 0xFF20 && a < 0xFF40) { const b = vera.read(a - 0xFF20, t); return b < 0 ? 0xFF : b; }   // (Configuring: floating)
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
    if (vera && a >= 0xFF20 && a < 0xFF40) { vera.write(a - 0xFF20, v, t); return; }
    if (a === 0xFFF0) { regT = v; T = v & 15; return; } if (a === 0xFFF1) { regU = v; U = v & 15; return; }
    if (a === 0xFFF2) { V = v; return; } if (a === 0xFFF3) { regW = v; W = v & 15; return; }
    if (a === 0xFFFE) { vecRam[V & 15] = (vecRam[V & 15] & 0xFF00) | v; return; }
    if (a === 0xFFFF) { vecRam[V & 15] = (vecRam[V & 15] & 0xFF) | (v << 8); return; }
  }
  // Lowest numbered active IRQ line, or -1
  function irqLine() {                                          // (Asked before every instruction: no allocation)
    let n = LINES;
    if (acia.irqActive() && opt.aciaLine < n) n = opt.aciaLine;
    if (via.irqActive()) n = 0;                                 // VIA: IRQ line 0
    if (vera && vera.irqActive() && 2 < n) n = 2;              // The VERA: line 2 (slot 0's IRQ A)
    if (ym.irqActive() && 4 < n) n = 4;                         // YM2151: line 4
    if (opt.stuckIrq >= 0 && opt.stuckIrq < n) n = opt.stuckIrq;
    return n < LINES ? n : -1;
  }
  const irqVector = () => { const n = irqLine(); return vecRam[n >= 0 ? (n ^ 7) : (V & 15)]; };

  // ---- the CPU, and what's watched as it runs
  const stackLow = new Array(16).fill(0x100), stackLowAt = new Array(16).fill(null);    // Per task: lowest S, and where (W:PC, cycle)
  cpu = createCpu({ rd, wr, where: pc => hx(W, 1) + ':' + hx(pc, 4) + ' (task ' + T + ')',
    pushed: s => { if (s < stackLow[T & 15]) { stackLow[T & 15] = s; stackLowAt[T & 15] = [W, cpu.lastPC, cpu.cyc]; } } });
  // The longest stretches with IRQs off (the I flag set), from the first key typed: [cycles, from, to, at, from's key]
  // (the places as page:PC; noted as keys, page << 16 | PC, and written out only for a stretch kept)
  let iOffAt = -1, iOffFrom = 0;
  const iOffTop = [], place = k => hx(k >> 16, 1) + ':' + hx(k & 0xFFFF, 4);
  function iOffNote(n, from, to, at) {
    const k = iOffTop.findIndex(e => e[4] === from);           // (One entry per starting place)
    if (k >= 0) { if (iOffTop[k][0] >= n) return; iOffTop[k] = [n, place(from), place(to), at, from]; }
    else if (iOffTop.length >= 8 && iOffTop[7][0] >= n) return;
    else iOffTop.push([n, place(from), place(to), at, from]);
    iOffTop.sort((a, b) => b[0] - a[0]); if (iOffTop.length > 8) iOffTop.pop();
  }
  const pcHist = new Map(), profHist = new Map(), profCyc = new Map(), profTask = new Array(16).fill(0);
  let profCount = 0, profCycles = 0;
  cpu.PC = rd(0xFFFC) | (rd(0xFFFD) << 8); cpu.P |= FLAGS.I;  // RESET

  // The devices, brought up to cycle t (at the start of each instruction, and at each I/O access: the access's cycle)
  let devCyc = 0;
  function sync(t) {
    const d = t - devCyc; if (d <= 0) return; devCyc = t;
    via.tick(d);
    while (ca1At < ca1Edges.length && ca1Edges[ca1At][0] <= t) via.ca1(ca1Edges[ca1At++][1]);
    ym.tick(t);
    if (vera) vera.tick(t);
    acia.tick(d, t);
  }
  // Cycles to the next device event (for a WAI: the CPU sleeps until then)
  const nextEvent = () => Math.min(via.nextEvent(), ym.nextEvent(devCyc), acia.nextEvent(), vera ? vera.nextEvent(devCyc) : Infinity,
    ca1At < ca1Edges.length ? Math.max(1, ca1Edges[ca1At][0] - devCyc) : Infinity);

  // Run until cycle limit (or a halt)
  const traceLen = opt.trace === undefined ? 25 : opt.trace, pcWatches = opt.pcWatches || [], profileFrom = opt.profile === undefined ? -1 : opt.profile,
    profileTo = opt.profileTo === undefined ? Infinity : opt.profileTo;
  const I = FLAGS.I;
  const ring = new Uint16Array(Math.max(1, traceLen) * 8);    // The trace: the last traceLen instructions, a ring
  let ringAt = 0, ringN = 0;                                  //   (W T PC A X Y S P each), m.trace's list
  const breaks = opt.breaks || [];
  m.breakHit = null; m.skipBreak = false;
  // The call trace: a call when the PC reaches a jump table slot (page 0), its return when the PC is back at the
  // caller's (its task, its stack as it was)
  const calls = opt.calls || null, errNames = opt.errNames || new Map(), callFilter = opt.callFilter || null;
  const pend = new Array(64).fill(null);
  let pendN = 0, pendAt = 0;
  const stringAt = (t, a) => {                                  // (A string in task t's view, as it runs: its RAM, its
    let str = '';                                             //   banks, the paged ROM; if a is one: printable, ended)
    for (let i = 0; i < 48 && a + i < 0xE000; i++) {
      const c = a + i < 0x8000 ? taskRam[t][a + i] : rd(a + i);
      if (c === 0) return str.length ? str : null;
      if (c < 0x20 || c > 0x7E) return null;
      str += String.fromCharCode(c);
    }
    return null;
  };
  function callTrace(PC) {
    if (pendN) for (let i = 0; i < pend.length; i++) {
      const p = pend[i];
      if (p && p.task === T && p.ret === PC && p.s === cpu.S) {
        log('  ' + p.name + ' T' + hx(T, 1) + ': ' + (cpu.P & FLAGS.C ? (errNames.get(cpu.A) || 'error $' + hx(cpu.A)) :
          'ok A=' + hx(cpu.A) + ' X=' + hx(cpu.X) + ' Y=' + hx(cpu.Y)) + ' at cycle ' + cpu.cyc);
        pend[i] = null; pendN--;
      }
    }
    if (W !== 0 || PC < 0xF800 || PC >= 0xFA40) return;
    const name = calls.get(PC);
    if (!name || callFilter && !callFilter.has(name) && !callFilter.has('t' + T)) return;
    const r = taskRam[T], sp = cpu.S;
    const ret = ((r[0x100 + ((sp + 2) & 0xFF)] << 8 | r[0x100 + ((sp + 1) & 0xFF)]) + 1) & 0xFFFF;
    let text = 'call T' + hx(T, 1) + ' ' + name + ' A=' + hx(cpu.A) + ' X=' + hx(cpu.X) + ' Y=' + hx(cpu.Y);
    for (let k = 0; k < 4; k++) text += ' r' + k + '=' + hx(r[2 + 2 * k] | r[3 + 2 * k] << 8, 4);
    const str = stringAt(T, r[2] | r[3] << 8);
    log(text + (str ? ' (r0: "' + str + '")' : '') + ', from ' + hx(ret, 4) + ' at cycle ' + cpu.cyc);
    if (pend[pendAt]) pendN--;                                  // (The oldest given up: a call that never returns, EXITS)
    pend[pendAt] = { task: T, ret, s: (sp + 2) & 0xFF, name };
    pendAt = (pendAt + 1) % pend.length; pendN++;
  }
  function run(limit) {
    runTo(limit);
    if (audio) audio.pump(cpu.cyc);
  }
  function runTo(limit) {
    while (cpu.cyc < limit && !cpu.halted) {
      sync(cpu.cyc);
      if (irqLine() >= 0) {
        cpu.waiting = false;
        if (!(cpu.P & I)) {                                     // (Taken: an IRQs-off stretch ends here, and the
          if (iOffAt >= 0) { iOffNote(cpu.cyc - iOffAt, iOffFrom, W << 16 | cpu.lastPC, iOffAt); iOffAt = -1; }   //   service starts its own)
          cpu.interrupt(irqVector()); continue;
        }
      }
      if (cpu.waiting) { cpu.cyc += Math.max(1, Math.min(nextEvent(), limit - cpu.cyc)); continue; }
      const PC = cpu.PC, P = cpu.P;
      if (breaks.length) {
        let hit = null;
        for (const b of breaks) if (b.pc === PC && (b.page < 0 || b.page === W) && (b.bank < 0 || taskRam[T][1] - b.bank >>> 0 < (b.banks || 1))) hit = b;
        if (hit) { if (m.skipBreak) m.skipBreak = false; else { m.breakHit = hit; return; } }
      }
      if (calls) callTrace(PC);
      if (acia.typedAt >= 0) {                                  // IRQs-off stretches, from the first key typed
        if (P & I) { if (iOffAt < 0) { iOffAt = cpu.cyc; iOffFrom = W << 16 | PC; } }
        else if (iOffAt >= 0) { iOffNote(cpu.cyc - iOffAt, iOffFrom, W << 16 | cpu.lastPC, iOffAt); iOffAt = -1; }
      }
      if (traceLen) {
        const o = ringAt * 8;
        ring[o] = W; ring[o + 1] = T; ring[o + 2] = PC; ring[o + 3] = cpu.A; ring[o + 4] = cpu.X; ring[o + 5] = cpu.Y; ring[o + 6] = cpu.S; ring[o + 7] = P;
        ringAt = (ringAt + 1) % traceLen; if (ringN < traceLen) ringN++;
      }
      if (pcWatches.length) for (const w of pcWatches) if (w.pc === PC && (w.page < 0 || w.page === W))
        log('pc: ' + hx(W, 1) + ':' + hx(PC, 4) + ' T=' + hx(T, 1) + ' A=' + hx(cpu.A) + ' X=' + hx(cpu.X) + ' Y=' + hx(cpu.Y) + ' S=' + hx(cpu.S) + ' P=' + hx(P) + ' at cycle ' + cpu.cyc);
      const pT = T, pW = W, c0 = cpu.cyc;                      // (The profile's: the instruction's task, page and cycles)
      cpu.step(irqVector);
      if (opt.pcHist) { const k = W * 65536 + cpu.PC; pcHist.set(k, (pcHist.get(k) || 0) + 1); }
      if (profileFrom >= 0 && c0 >= profileFrom && c0 < profileTo) {
        const pk = pT * 1048576 + pW * 65536 + cpu.lastPC, dc = cpu.cyc - c0;
        profHist.set(pk, (profHist.get(pk) || 0) + 1); profCyc.set(pk, (profCyc.get(pk) || 0) + dc); profTask[pT]++; profCount++; profCycles += dc;
      }
    }
  }

  // The reset button (RESB): the CPU, the VIA, the ACIA and the YM2151 reset, and the VERA configures itself again;
  // RAM, the pseudo-registers (plain latches), VRAM and the SD cards keep their state, as on the board
  function hwReset() {
    via.reset();
    acia.reset();
    ym.reset();
    if (vera) vera.reset(cpu.cyc);
    cpu.reset();
  }

  // (A task's RAM bank b, as it is: undefined if it was never written; tools/hysnap.js reads hylang's heap with it)
  const taskBankMem = (t, b) => taskBank[t * 256 + b];
  Object.assign(m, { cpu, acia, via, i2c, smc, ym, vera, audio, rtc, taskRam, vecRam, pcHist, iOffTop, stackLow, stackLowAt, profHist, profCyc, profTask, run, hwReset, rd, taskBankMem });
  Object.defineProperties(m, {                                // (The pseudo-registers, the trace and the profile's count, as they are now)
    trace: { get: () => {                                     // (The ring, oldest first: [W, T, PC, A, X, Y, S, P] each)
      const out = [];
      for (let i = ringN; i > 0; i--) { const o = ((ringAt - i + traceLen) % traceLen) * 8; out.push(Array.from(ring.subarray(o, o + 8))); }
      return out;
    } },
    T: { get: () => T }, U: { get: () => U }, V: { get: () => V }, W: { get: () => W }, profCount: { get: () => profCount }, profCycles: { get: () => profCycles },
  });
  return m;
}

module.exports = { createMachine, romBank };
