#!/usr/bin/env node
// ****************************************************************************
// hydrasim.js - a minimal Hydra-16 emulator, for debugging the OS ROM without the hardware.
//
// Models:
//   * 65C02 CPU (WDC/Rockwell: STZ, BRA, PHX/PLY, TSB/TRB, BBR/BBS, RMB/SMB, WAI, STP, (zp) mode, ...)
//   * T (task) register: each task has its own $0000-$7FFF (ZP, stack, task RAM) and $00/$01 bank registers
//   * RAM bank window $8000-$9FFF: banks $00-$EF per task (only the installed modules; others float),
//     banks $F0-$FF shared, 16 macro-pages selected by U
//   * Paged ROM $A000-$DFFF (paged_rom_C02.bin, 16K banks by $01), with the A13 half-swap of the board
//   * BIOS ROM $E000-$FFFF (os_rom_C02.bin, 8K pages by W); I/O at $FF00-$FFEF; T/U/V/W at $FFF0-$FFF3
//   * IRQ vector RAM ($FFFE/F): written at index V[0..3]; read at index IRQ_NUMBER(n) = n ^ 7 of the lowest
//     active IRQ line, or V[0..3] when no line is active (and for BRK)
//   * Rockwell 65C51 ACIA at $FF10 (IRQ line 1): TX/RX with IRQs, output captured, input from --input
//   * VIA timer 1 (one-shot / free-running, IFR/IER) on IRQ line 0: the scheduler's tick; other VIA
//     registers are plain storage
//   * YM2151 status always "not busy"; key-ons (register $08) are counted and reported
//   RAM and the pseudo-registers power up random, like the hardware.
//
// Usage: node hydrasim.js [options]
//   --rom DIR           ROM images directory (default: ../os_rom/bin next to this script)
//   --cycles N          CPU cycles to run (default 20000000; ~5.6 s at 3.58 MHz)
//   --input TEXT        Serial input to type, after a short delay; "\r" = CR, "\xNN" = byte NN, e.g. a
//                       control key; "\w" = wait ~2M cycles before the next key (e.g. --input
//                       "1 2 + .\r", "cat\rhi\x04", "inf\r\w\x03")
//   --modules N         RAM modules installed: banks $00 - N*16-1 (default 3)
//   --shared-u N        Shared RAM installed for U macro-pages 0 - N-1 (default 16; 4 per 512K chip)
//   --acia-line N       IRQ line the ACIA interrupts on (default 1)
//   --acia rockwell|wdc The ACIA: Rockwell R65C51 (default: TDRE status and TX interrupt), or WDC W65C51N
//                       (its bug: TDRE always reads 1, no TX interrupt; bytes written while one is still
//                       being sent are counted: they'd be garbled on the chip)
//   --stuck-irq N       Hold IRQ line N active all the time
//   --ram-fault BANK:An:high|low   Address line An (0-12) stuck high/low on the RAM chip holding BANK (a
//                       shared chip holds 4 bank IDs, e.g. F0-F3; a task RAM module 16 banks), e.g. F0:A0:high
//   --model M           Hardware what-ifs: sharedlow (T doesn't switch $0000-$7FFF), nostack (stack page
//                       not per task), zponly (only ZP per task), noshared (no shared RAM)
//   --sd [N:]FILE       An SD card (SDHC) on SPI device N (0-7; default 0), backed by the image FILE
//                       (512-byte blocks; writes go to the file).  Up to 8, one per device
//                       (e.g. --sd card0.img --sd 3:C:/images/card3.img)
//   --raw               Print serial output as-is (default shows ESC as <ESC>)
//   --trace N           Show the last N instructions (default 25)
//   --dump ADDR[:LEN][@TASK]   Hex dump task RAM after the run (e.g. --dump 7D90:16@1)
//   --watch ADDR[@TASK] Report every write to task RAM address ADDR (value, and the PC that wrote it)
//   --pc [PAGE:]ADDR    Report the registers every time the PC reaches ADDR (on ROM page PAGE, if given)
//   --mark TEXT         Report the cycle each time the serial output ends with TEXT ("\r" = CR), e.g. a prompt
//   --profile N         From cycle N on, count the instructions run in each routine (named from the
//                       build's debug info, ../os_rom/obj/os_rom_C02.dbg) and in each task, and report them
//
// Output: serial output, the last instructions (W T PC A X Y S P), the hottest PCs (useful to find a
// loop the code is stuck in), and final state.
// ****************************************************************************
'use strict';
const fs = require('fs');
const path = require('path');

// ---- options
const opt = { rom: path.join(__dirname, '..', 'os_rom', 'bin'), cycles: 20000000, input: '', modules: 3,
  aciaLine: 1, stuckIrq: -1, model: '', raw: false, trace: 25, dumps: [], watches: [], pcWatches: [], sharedU: 16, ramFault: null, sds: [], acia: 'rockwell', marks: [], profile: -1 };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const a = argv[i], next = () => argv[++i];
  switch (a) {
    case '--rom': opt.rom = next(); break;
    case '--cycles': opt.cycles = +next(); break;
    case '--input': opt.input = next().replace(/\\r/g, '\r').replace(/\\n/g, '\n').replace(/\\x([0-9A-Fa-f]{2})/g, (m, h) => String.fromCharCode(parseInt(h, 16))).replace(/\\w/g, '\u0100'); break;
    case '--modules': opt.modules = +next(); break;
    case '--shared-u': opt.sharedU = +next(); break;
    case '--acia-line': opt.aciaLine = +next(); break;
    case '--acia': opt.acia = next().toLowerCase(); if (!/^(rockwell|wdc)$/.test(opt.acia)) { console.error('--acia rockwell|wdc'); process.exit(1); } break;
    case '--stuck-irq': opt.stuckIrq = +next(); break;
    case '--model': opt.model = next(); break;
    case '--ram-fault': { const m = /^([0-9A-Fa-f]{1,2}):A(\d+):(high|low)$/i.exec(next()); opt.ramFault = { bank: parseInt(m[1], 16), mask: 1 << +m[2], high: m[3].toLowerCase() === 'high' }; break; }
    case '--sd': { const f = next(), m = /^([0-7]):(.+)$/.exec(f), dev = m ? +m[1] : 0;
      if (opt.sds.some(c => c.dev === dev)) { console.error('Two SD cards on device ' + dev); process.exit(1); }
      opt.sds.push({ dev, file: m ? m[2] : f }); break; }
    case '--raw': opt.raw = true; break;
    case '--trace': opt.trace = +next(); break;
    case '--dump': opt.dumps.push(next()); break;
    case '--pc': { const m = /^(?:([0-9A-Fa-f]):)?([0-9A-Fa-f]+)$/.exec(next()); opt.pcWatches.push({ pc: parseInt(m[2], 16), page: m[1] === undefined ? -1 : parseInt(m[1], 16) }); break; }
    case '--mark': opt.marks.push(next().replace(/\\r/g, '\r').replace(/\\n/g, '\n')); break;
    case '--profile': opt.profile = +next(); break;
    case '--watch': { const m = /^([0-9A-Fa-f]+)(?:@([0-9A-Fa-f]))?$/.exec(next()); opt.watches.push({ addr: parseInt(m[1], 16), task: m[2] === undefined ? -1 : parseInt(m[2], 16) }); break; }
    default: console.error('Unknown option: ' + a + ' (see the header of hydrasim.js)'); process.exit(1);
  }
}
const osrom = fs.readFileSync(path.join(opt.rom, 'os_rom_C02.bin'));
const pagedrom = fs.readFileSync(path.join(opt.rom, 'paged_rom_C02.bin'));
const rnd = n => (Math.random() * n) | 0;
const hx = (v, n = 2) => v.toString(16).toUpperCase().padStart(n, '0');

// ---- memory and devices
const taskRam = []; for (let i = 0; i < 16; i++) taskRam.push(new Uint8Array(0x8000).map(() => rnd(256)));
const taskBank = {}, sharedBank = {};
const vecRam = new Uint16Array(16).map(() => rnd(65536));
let T = 0, U = rnd(16), V = rnd(256), W = rnd(16);
let out = '';
let ymReg = 0; const ymKeyOns = [];                          // YM2151: the register selected, and the key-ons written
let aciaCmd = 0, aciaCtrl = 0, aciaTdre = 1, aciaTxTimer = 0, aciaIrq = 0, aciaRdrf = 0, aciaRx = 0, aciaOverruns = 0;
const aciaWdc = opt.acia === 'wdc';
const rxQueue = [...opt.input]; let rxDelay = 200000;
const via = new Uint8Array(16);
// VIA timer 1 (the scheduler's tick): counter, latch, IFR/IER; one-shot or free-running (ACR bit 6)
let viaT1 = 0xFFFF, viaT1Latch = 0xFFFF, viaT1On = false, viaIFR = 0, viaIER = 0;
let viaT2 = 0xFFFF, viaT2LatchL = 0xFF, viaT2On = false;         // Timer 2: one-shot
// SPI on VIA port B (PB0 SCLK, PB1 /CS enable, PB2 MOSI, PB3-PB5 device 0-7, PB6 = 0 for the board's
// devices, PB7 MISO; mode 0), with up to 8 SD cards (SPI mode, SDHC: block addresses) on devices 0-7,
// each backed by an image file (--sd [N:]FILE)
const sdCards = [];                                             // By device: a card, or undefined
for (const { dev, file } of opt.sds) {
  const fd = fs.openSync(file, 'r+');
  sdCards[dev] = { dev, fd, blocks: Math.floor(fs.fstatSync(fd).size / 512), bit: 0, inB: 0, cur: 0xFF, miso: 1,
    q: [], cmd: [], idle: true, app: false, acmd41: 0, writeAt: -1, wr: null };
}
let spiSel = null, spiClk = 0;                                  // The selected card (null: none), SCLK
function sdReset(sd) {                                          // Deselected: it forgets the transfer
  sd.bit = 0; sd.cmd = []; sd.q = []; sd.wr = null; sd.writeAt = -1; sd.cur = 0xFF;
}
function sdCommand(sd, c) {                                     // One 6-byte command: its answer bytes
  const idx = c[0] & 0x3F, arg = ((c[1] << 24) | (c[2] << 16) | (c[3] << 8) | c[4]) >>> 0, idle = sd.idle ? 1 : 0;
  if (sd.app) {
    sd.app = false;
    if (idx === 41) { if (++sd.acmd41 >= 3) sd.idle = false; return [sd.idle ? 1 : 0]; }
    return [0x04 | idle];
  }
  switch (idx) {
    case 0: sd.idle = true; sd.acmd41 = 0; return [0x01];
    case 8: return [idle, 0x00, 0x00, c[3] & 0x0F, c[4]];        // R7: voltage accepted, the pattern back
    case 55: sd.app = true; return [idle];
    case 58: return [idle, 0xC0, 0xFF, 0x80, 0x00];              // OCR: powered up, CCS (SDHC)
    case 16: return [idle];
    case 17: {
      if (sd.idle || arg >= sd.blocks) return [0x40 | idle];     // (Parameter error)
      const b = Buffer.alloc(512); fs.readSync(sd.fd, b, 0, 512, arg * 512);
      return [0x00, 0xFF, 0xFF, 0xFE, ...b, 0x12, 0x34];         // R1, a wait, the token, data, CRC
    }
    case 24:
      if (sd.idle || arg >= sd.blocks) return [0x40 | idle];
      sd.writeAt = arg; return [0x00];                           // Then: the data block
    default: return [0x04 | idle];                               // Illegal command
  }
}
function sdByte(sd, b) {                                        // Byte b came in: the next byte out
  if (sd.wr) {                                                  // A block for CMD24: token, 512 bytes, CRC
    if (sd.wr.data.length === 0 && b !== 0xFE) return 0xFF;
    sd.wr.data.push(b);
    if (sd.wr.data.length === 515) {
      fs.writeSync(sd.fd, Buffer.from(sd.wr.data.slice(1, 513)), 0, 512, sd.wr.at * 512);
      sd.wr = null; sd.q = [0x00, 0x00, 0x00, 0xFF];             // (Busy a while, then ready)
      return 0x05;                                               // Data accepted
    }
    return 0xFF;
  }
  if (sd.cmd.length || (b & 0xC0) === 0x40) {
    sd.cmd.push(b);
    if (sd.cmd.length === 6) { sd.q = [0xFF, ...sdCommand(sd, sd.cmd)]; sd.cmd = []; return sd.q.shift(); }
    return 0xFF;
  }
  if (sd.q.length) return sd.q.shift();
  if (sd.writeAt >= 0) { sd.wr = { at: sd.writeAt, data: [] }; sd.writeAt = -1; }   // (R1 out: data next)
  return 0xFF;
}
function spiPortB(v) {                                          // Port B's output bits changed
  const card = !(v & 0x02) && !(v & 0x40) ? sdCards[(v >> 3) & 7] || null : null;
  if (card !== spiSel) { if (spiSel) sdReset(spiSel); spiSel = card; }
  const clk = v & 1;
  if (clk && !spiClk && card) {                                 // Rising edge: both sides sample
    card.miso = (card.cur >> (7 - card.bit)) & 1;
    card.inB = ((card.inB << 1) | ((v >> 2) & 1)) & 0xFF;
    if (++card.bit === 8) { card.bit = 0; card.cur = sdByte(card, card.inB); }
  }
  spiClk = clk;
}
function viaRead(r) {
  if (r === 0) { const ddr = via[2], pins = 0x7F | ((spiSel ? spiSel.miso : 1) << 7); return (via[0] & ddr) | (pins & ~ddr); }
  if (r === 4) { viaIFR &= ~0x40; return viaT1 & 0xFF; }       // T1C-L: clears the T1 flag
  if (r === 5) return viaT1 >> 8;
  if (r === 8) { viaIFR &= ~0x20; return viaT2 & 0xFF; }       // T2C-L: clears the T2 flag
  if (r === 9) return (viaT2 >> 8) & 0xFF;
  if (r === 6) return viaT1Latch & 0xFF;
  if (r === 7) return viaT1Latch >> 8;
  if (r === 0x0D) return viaIFR | ((viaIFR & viaIER & 0x7F) ? 0x80 : 0);
  if (r === 0x0E) return viaIER | 0x80;
  return via[r];
}
function viaWrite(r, v) {
  if (r === 4 || r === 6) viaT1Latch = (viaT1Latch & 0xFF00) | v;
  else if (r === 5) { viaT1Latch = (viaT1Latch & 0xFF) | (v << 8); viaT1 = viaT1Latch; viaT1On = true; viaIFR &= ~0x40; }
  else if (r === 7) { viaT1Latch = (viaT1Latch & 0xFF) | (v << 8); viaIFR &= ~0x40; }
  else if (r === 8) viaT2LatchL = v;
  else if (r === 9) { viaT2 = (v << 8) | viaT2LatchL; viaT2On = true; viaIFR &= ~0x20; }   // Load and start
  else if (r === 0x0D) viaIFR &= ~(v & 0x7F);
  else if (r === 0x0E) { if (v & 0x80) viaIER |= v & 0x7F; else viaIER &= ~(v & 0x7F); }
  else { via[r] = v; if (r === 0 || r === 2) spiPortB((via[0] & via[2]) | (~via[2] & 0x7F)); }
}
function viaTick(n) {
  if (viaT2On) { viaT2 -= n; if (viaT2 < 0) { viaIFR |= 0x20; viaT2On = false; viaT2 &= 0xFFFF; } }
  if (!viaT1On) return;
  viaT1 -= n;
  if (viaT1 < 0) {
    viaIFR |= 0x40;
    if (via[0x0B] & 0x40) viaT1 += viaT1Latch + 2; else { viaT1 = 0xFFFF; viaT1On = false; }
  }
}
const ACIA_TX_CYCLES = 1860;                                // ~one character at 19200 baud, 3.58 MHz

// Which task's copy of $0000-$7FFF an access uses (the --model what-ifs change this)
const tsel = a => opt.model === 'sharedlow' ? 0 : (opt.model === 'zponly' && a >= 0x200) ? 0
  : (opt.model === 'nostack' && a >= 0x100 && a < 0x200) ? 0 : T;
const bankInstalled = b => b >= 0xF0 ? (opt.model !== 'noshared' && U < opt.sharedU) : b < opt.modules * 16;
function bankMem(b) {
  if (b >= 0xF0) { const k = U * 16 + (b & 15); return sharedBank[k] || (sharedBank[k] = new Uint8Array(0x2000)); }
  const k = T * 256 + b; return taskBank[k] || (taskBank[k] = new Uint8Array(0x2000));
}
// --ram-fault: the offset in the window that the chip holding bank b actually sees
const sameChip = (b, f) => b >= 0xF0 ? f >= 0xF0 && ((b ^ f) & 0x0C) === 0 : f < 0xF0 && (b >> 4) === (f >> 4);
function ramOfs(b, a) {
  const o = a - 0x8000, f = opt.ramFault;
  if (!f || !sameChip(b, f.bank)) return o;
  return f.high ? o | f.mask : o & ~f.mask;
}
function rd(a) {
  if (a < 0x8000) return taskRam[tsel(a)][a];
  if (a < 0xA000) { const b = taskRam[tsel(0)][0]; return bankInstalled(b) ? bankMem(b)[ramOfs(b, a)] : (a >> 8); }  // floating bus
  if (a < 0xE000) { const off = taskRam[tsel(1)][1] * 0x4000 + ((a - 0xA000) ^ 0x2000); return off < pagedrom.length ? pagedrom[off] : 0xFF; }
  if (a >= 0xFF00 && a < 0xFFF0) {
    if (a >= 0xFF10 && a < 0xFF14) {
      const r = a - 0xFF10;
      if (r === 0) { aciaRdrf = 0; return aciaRx; }
      if (r === 1) { const s = (aciaIrq ? 0x80 : 0) | (aciaTdre || aciaWdc ? 0x10 : 0) | (aciaRdrf ? 0x08 : 0); aciaIrq = 0; return s; }
      return r === 2 ? aciaCmd : aciaCtrl;
    }
    if (a < 0xFF10) return viaRead(a - 0xFF00);
    if (a === 0xFF41) return 0x00;                          // YM2151 status: not busy
    return 0xFF;
  }
  if (a === 0xFFF0) return T; if (a === 0xFFF1) return U; if (a === 0xFFF2) return V; if (a === 0xFFF3) return W;
  if (a === 0xFFFE || a === 0xFFFF) { const v = vecRam[V & 15]; return a === 0xFFFE ? v & 0xFF : v >> 8; }
  return osrom[W * 0x2000 + (a - 0xE000)];
}
function wr(a, v) {
  v &= 0xFF;
  if (a < 0x8000) {
    for (const w of opt.watches) if (w.addr === a && (w.task < 0 || w.task === tsel(a)))
      console.log('watch: $' + hx(a, 4) + ' (task ' + hx(tsel(a), 1) + ') ' + hx(taskRam[tsel(a)][a]) + ' -> ' + hx(v) + ' by ' + hx(W, 1) + ':' + hx(lastPC, 4) + ' at cycle ' + cyc);
    taskRam[tsel(a)][a] = v; return;
  }
  if (a < 0xA000) { const b = taskRam[tsel(0)][0]; if (bankInstalled(b)) bankMem(b)[ramOfs(b, a)] = v; return; }
  if (a < 0xFF00) return;
  if (a >= 0xFF10 && a < 0xFF14) {
    const r = a - 0xFF10;
    if (r === 0) { if (aciaTxTimer > 0) aciaOverruns++; out += String.fromCharCode(v); for (const m of opt.marks) if (out.endsWith(m)) console.log('mark: ' + JSON.stringify(m) + ' at cycle ' + cyc); aciaTdre = 0; aciaTxTimer = ACIA_TX_CYCLES; }
    else if (r === 1) { aciaCmd &= 0xE0; aciaIrq = 0; }                         // programmed reset
    else if (r === 2) { aciaCmd = v; if (!aciaWdc && (v & 0x0C) === 0x04 && aciaTdre) aciaIrq = 1; }
    else aciaCtrl = v;
    return;
  }
  if (a < 0xFF10) { viaWrite(a - 0xFF00, v); return; }
  if (a === 0xFF40) { ymReg = v; return; }                          // YM2151: register, then data
  if (a === 0xFF41) { if (ymReg === 0x08 && (v & 0x78)) ymKeyOns.push('ch ' + (v & 7) + ' at cycle ' + cyc); return; }
  if (a === 0xFFF0) { T = v & 15; return; } if (a === 0xFFF1) { U = v & 15; return; }
  if (a === 0xFFF2) { V = v; return; } if (a === 0xFFF3) { W = v & 15; return; }
  if (a === 0xFFFE) { vecRam[V & 15] = (vecRam[V & 15] & 0xFF00) | v; return; }
  if (a === 0xFFFF) { vecRam[V & 15] = (vecRam[V & 15] & 0xFF) | (v << 8); return; }
}
// Lowest numbered active IRQ line, or -1
function irqLine() {
  const acia = aciaIrq && ((!aciaWdc && (aciaCmd & 0x0C) === 0x04 && aciaTdre) || (!(aciaCmd & 2) && aciaRdrf));
  const lines = [];
  if (acia) lines.push(opt.aciaLine);
  if (viaIFR & viaIER & 0x7F) lines.push(0);                      // VIA: IRQ line 0
  if (opt.stuckIrq >= 0) lines.push(opt.stuckIrq);
  return lines.length ? Math.min(...lines) : -1;
}
const irqVector = () => { const n = irqLine(); return vecRam[n >= 0 ? (n ^ 7) : (V & 15)]; };

// ---- CPU
let A = 0, X = 0, Y = 0, S = 0xFD, P = 0x34, PC = 0, lastPC = 0, cyc = 0, waiting = false, halted = '';
const C = 1, Z = 2, I = 4, D = 8, B = 0x10, Vf = 0x40, N = 0x80;
const setNZ = v => { P = (P & ~(N | Z)) | (v & 0x80) | (v ? 0 : Z); return v; };
const push = v => { wr(0x100 + S, v); S = (S - 1) & 0xFF; };
const pull = () => { S = (S + 1) & 0xFF; return rd(0x100 + S); };
const rd16 = a => rd(a) | (rd((a + 1) & 0xFFFF) << 8);
const zp16 = a => rd(a & 0xFF) | (rd((a + 1) & 0xFF) << 8);
const fetch = () => { const v = rd(PC); PC = (PC + 1) & 0xFFFF; return v; };
const fetch16 = () => { const v = rd16(PC); PC = (PC + 2) & 0xFFFF; return v; };
function adc(v) {
  const c = P & C; let r = A + v + c;
  P = (P & ~(C | Vf)) | (r > 0xFF ? C : 0) | ((~(A ^ v) & (A ^ r) & 0x80) ? Vf : 0);
  if (P & D) {
    let lo = (A & 15) + (v & 15) + c, hi = (A >> 4) + (v >> 4);
    if (lo > 9) { lo += 6; hi++; } if (hi > 9) hi += 6;
    r = (hi << 4) | (lo & 15); P = (P & ~C) | (hi > 15 ? C : 0);
  }
  A = setNZ(r & 0xFF);
}
function sbc(v) {
  if (!(P & D)) { adc(v ^ 0xFF); return; }
  const b = 1 - (P & C), r = A - v - b; let lo = (A & 15) - (v & 15) - b, hi = (A >> 4) - (v >> 4);
  if (lo < 0) { lo -= 6; hi--; } if (hi < 0) hi -= 6;
  P = (P & ~C) | (r >= 0 ? C : 0); A = setNZ(((hi << 4) | (lo & 15)) & 0xFF);
}
const cmp = (r, v) => { const t = r - v; P = (P & ~C) | (t >= 0 ? C : 0); setNZ(t & 0xFF); };
function interrupt(vec, brk) { push(PC >> 8); push(PC & 0xFF); push((P | 0x20) & (brk ? 0xFF : ~B)); P = (P | I) & ~D; PC = vec; }

const trace = [], pcHist = new Map(), profHist = new Map(), profTask = new Array(16).fill(0);
let profCount = 0;
PC = rd16(0xFFFC); P |= I;                                  // RESET
let lastCyc = 0;
while (cyc < opt.cycles && !halted) {
  const dCyc = cyc - lastCyc; lastCyc = cyc;
  viaTick(dCyc);
  if (aciaTxTimer > 0 && (aciaTxTimer -= dCyc) <= 0) {            // (In cycles: a character's time)
    aciaTxTimer = 0; aciaTdre = 1; if (!aciaWdc && (aciaCmd & 0x0C) === 0x04) aciaIrq = 1;
  }
  if (rxQueue.length && --rxDelay <= 0 && !aciaRdrf) {
    const c = rxQueue.shift();
    if (c === '\u0100') rxDelay = 2000000;                  // \w: wait before the next key
    else { aciaRx = c.charCodeAt(0); aciaRdrf = 1; if (!(aciaCmd & 2)) aciaIrq = 1; rxDelay = 20000; }
  }
  if (irqLine() >= 0) { waiting = false; if (!(P & I)) { interrupt(irqVector(), false); cyc += 7; continue; } }
  if (waiting) { cyc++; continue; }
  trace.push([W, T, PC, A, X, Y, S, P]); if (trace.length > opt.trace) trace.shift();
  lastPC = PC;
  for (const w of opt.pcWatches) if (w.pc === PC && (w.page < 0 || w.page === W))
    console.log('pc: ' + hx(W, 1) + ':' + hx(PC, 4) + ' T=' + hx(T, 1) + ' A=' + hx(A) + ' X=' + hx(X) + ' Y=' + hx(Y) + ' S=' + hx(S) + ' P=' + hx(P) + ' at cycle ' + cyc);
  const op = fetch(); cyc += 3;
  let a, v, t;
  const zp = () => fetch(), zpx = () => (fetch() + X) & 0xFF, zpy = () => (fetch() + Y) & 0xFF;
  const abs = () => fetch16(), absx = () => (fetch16() + X) & 0xFFFF, absy = () => (fetch16() + Y) & 0xFFFF;
  const indx = () => zp16(fetch() + X), indy = () => (zp16(fetch()) + Y) & 0xFFFF, indz = () => zp16(fetch());
  const br = c => { const o = fetch(); if (c) PC = (PC + ((o ^ 0x80) - 0x80)) & 0xFFFF; };
  const rmw = (addr, f) => wr(addr, f(rd(addr)) & 0xFF);
  const asl = v => { P = (P & ~C) | (v >> 7); return setNZ((v << 1) & 0xFF); };
  const lsr = v => { P = (P & ~C) | (v & 1); return setNZ(v >> 1); };
  const rol = v => { const c = P & C; P = (P & ~C) | (v >> 7); return setNZ(((v << 1) | c) & 0xFF); };
  const ror = v => { const c = P & C; P = (P & ~C) | (v & 1); return setNZ((v >> 1) | (c << 7)); };
  const bit = v => { P = (P & ~(N | Vf | Z)) | (v & 0xC0) | ((A & v) ? 0 : Z); };
  const ALU = [v => A = setNZ(A | v), v => A = setNZ(A & v), v => A = setNZ(A ^ v), adc, null, null, v => cmp(A, v), sbc];
  const aaa = op >> 5, bbb = (op >> 2) & 7, cc = op & 3;
  const bad = () => { halted = 'unimplemented opcode $' + hx(op) + ' at ' + hx(W, 1) + ':' + hx((PC - 1) & 0xFFFF, 4) + ' (task ' + T + ')'; };
  switch (op) {
    case 0x00: PC = (PC + 1) & 0xFFFF; interrupt(irqVector(), true); break;           // BRK
    case 0x40: P = pull() | 0x30; PC = pull(); PC |= pull() << 8; break;              // RTI
    case 0x60: PC = pull(); PC |= pull() << 8; PC = (PC + 1) & 0xFFFF; break;         // RTS
    case 0x20: a = fetch16(); t = (PC - 1) & 0xFFFF; push(t >> 8); push(t & 0xFF); PC = a; break;
    case 0x4C: PC = fetch16(); break;
    case 0x6C: PC = rd16(fetch16()); break;
    case 0x7C: PC = rd16(absx()); break;
    case 0x08: push(P | 0x30); break; case 0x28: P = pull() | 0x30; break;
    case 0x48: push(A); break; case 0x68: A = setNZ(pull()); break;
    case 0xDA: push(X); break; case 0xFA: X = setNZ(pull()); break;
    case 0x5A: push(Y); break; case 0x7A: Y = setNZ(pull()); break;
    case 0x18: P &= ~C; break; case 0x38: P |= C; break; case 0x58: P &= ~I; break; case 0x78: P |= I; break;
    case 0xB8: P &= ~Vf; break; case 0xD8: P &= ~D; break; case 0xF8: P |= D; break;
    case 0xAA: X = setNZ(A); break; case 0xA8: Y = setNZ(A); break; case 0x8A: A = setNZ(X); break; case 0x98: A = setNZ(Y); break;
    case 0xBA: X = setNZ(S); break; case 0x9A: S = X; break;
    case 0xE8: X = setNZ((X + 1) & 0xFF); break; case 0xCA: X = setNZ((X - 1) & 0xFF); break;
    case 0xC8: Y = setNZ((Y + 1) & 0xFF); break; case 0x88: Y = setNZ((Y - 1) & 0xFF); break;
    case 0x1A: A = setNZ((A + 1) & 0xFF); break; case 0x3A: A = setNZ((A - 1) & 0xFF); break;
    case 0xEA: break;
    case 0xCB: waiting = true; break;                                                 // WAI
    case 0xDB: halted = 'STP at ' + hx(W, 1) + ':' + hx((PC - 1) & 0xFFFF, 4); break;   // STP
    case 0x0A: A = asl(A); break; case 0x4A: A = lsr(A); break; case 0x2A: A = rol(A); break; case 0x6A: A = ror(A); break;
    case 0x10: br(!(P & N)); break; case 0x30: br(P & N); break; case 0x50: br(!(P & Vf)); break; case 0x70: br(P & Vf); break;
    case 0x90: br(!(P & C)); break; case 0xB0: br(P & C); break; case 0xD0: br(!(P & Z)); break; case 0xF0: br(P & Z); break;
    case 0x80: br(true); break;
    case 0x64: wr(zp(), 0); break; case 0x74: wr(zpx(), 0); break; case 0x9C: wr(abs(), 0); break; case 0x9E: wr(absx(), 0); break;
    case 0x89: v = fetch(); P = (P & ~Z) | ((A & v) ? 0 : Z); break;
    case 0x24: bit(rd(zp())); break; case 0x34: bit(rd(zpx())); break; case 0x2C: bit(rd(abs())); break; case 0x3C: bit(rd(absx())); break;
    case 0x04: case 0x0C: a = op === 0x04 ? zp() : abs(); v = rd(a); P = (P & ~Z) | ((A & v) ? 0 : Z); wr(a, v | A); break;   // TSB
    case 0x14: case 0x1C: a = op === 0x14 ? zp() : abs(); v = rd(a); P = (P & ~Z) | ((A & v) ? 0 : Z); wr(a, v & ~A); break;  // TRB
    case 0x92: wr(indz(), A); break; case 0xB2: A = setNZ(rd(indz())); break;
    case 0x12: ALU[0](rd(indz())); break; case 0x32: ALU[1](rd(indz())); break; case 0x52: ALU[2](rd(indz())); break;
    case 0x72: adc(rd(indz())); break; case 0xD2: cmp(A, rd(indz())); break; case 0xF2: sbc(rd(indz())); break;
    case 0xA2: X = setNZ(fetch()); break; case 0xA0: Y = setNZ(fetch()); break;
    case 0xA6: X = setNZ(rd(zp())); break; case 0xB6: X = setNZ(rd(zpy())); break; case 0xAE: X = setNZ(rd(abs())); break; case 0xBE: X = setNZ(rd(absy())); break;
    case 0xA4: Y = setNZ(rd(zp())); break; case 0xB4: Y = setNZ(rd(zpx())); break; case 0xAC: Y = setNZ(rd(abs())); break; case 0xBC: Y = setNZ(rd(absx())); break;
    case 0x86: wr(zp(), X); break; case 0x96: wr(zpy(), X); break; case 0x8E: wr(abs(), X); break;
    case 0x84: wr(zp(), Y); break; case 0x94: wr(zpx(), Y); break; case 0x8C: wr(abs(), Y); break;
    case 0xE0: cmp(X, fetch()); break; case 0xE4: cmp(X, rd(zp())); break; case 0xEC: cmp(X, rd(abs())); break;
    case 0xC0: cmp(Y, fetch()); break; case 0xC4: cmp(Y, rd(zp())); break; case 0xCC: cmp(Y, rd(abs())); break;
    case 0x02: case 0x22: case 0x42: case 0x62: case 0x82: case 0xC2: case 0xE2: case 0x44: case 0x54: case 0xD4: case 0xF4: fetch(); break;   // NOPs
    case 0x5C: case 0xDC: case 0xFC: fetch16(); break;
    default:
      if ((op & 0x0F) === 0x0F) { const n = (op >> 4) & 7, z = fetch(); const set = (rd(z) >> n) & 1; br((op & 0x80) ? set : !set); break; }   // BBR/BBS
      if ((op & 0x0F) === 0x07) { const n = (op >> 4) & 7, z = fetch(); v = rd(z); wr(z, (op & 0x80) ? v | (1 << n) : v & ~(1 << n)); break; } // RMB/SMB
      if (cc === 1) {
        a = [indx, zp, () => -1, abs, indy, zpx, absy, absx][bbb]();
        if (aaa === 4) { if (a >= 0) wr(a, A); else fetch(); break; }                // STA (no immediate)
        v = a < 0 ? fetch() : rd(a);
        if (aaa === 5) A = setNZ(v); else ALU[aaa](v);
        break;
      }
      if (cc === 2) {
        const mode = { 1: zp, 3: abs, 5: zpx, 7: absx }[bbb];
        const f = [asl, rol, lsr, ror, null, null, v => setNZ((v - 1) & 0xFF), v => setNZ((v + 1) & 0xFF)][aaa];
        if (!mode || !f) { bad(); break; }
        rmw(mode(), f); break;
      }
      bad();
  }
  const k = W * 65536 + PC; pcHist.set(k, (pcHist.get(k) || 0) + 1);
  if (opt.profile >= 0 && cyc >= opt.profile) { const pk = T * 1048576 + W * 65536 + lastPC; profHist.set(pk, (profHist.get(pk) || 0) + 1); profTask[T]++; profCount++; }
}

// ---- report
console.log('--- serial output ---\n' + (opt.raw ? out : out.replace(/\x1b/g, '<ESC>')));
if (halted) console.log('--- halted: ' + halted);
console.log('--- last instructions (W T PC   A  X  Y  S  P) ---');
for (const [w, t, pc, a, x, y, s, p] of trace) console.log(hx(w, 1), hx(t, 1), hx(pc, 4), hx(a), hx(x), hx(y), hx(s), hx(p));
if (aciaWdc) console.log('--- WDC ACIA: ' + aciaOverruns + ' byte(s) written while one was still being sent (garbled on the chip)');
console.log('--- hottest PCs (W:PC count) ---');
for (const [k, c] of [...pcHist.entries()].sort((a, b) => b[1] - a[1]).slice(0, 10)) console.log(hx(k >> 16, 1) + ':' + hx(k & 0xFFFF, 4), c);
if (ymKeyOns.length) console.log('--- YM2151 key-ons: ' + ymKeyOns.length + ' (' + ymKeyOns.slice(0, 8).join(', ') + (ymKeyOns.length > 8 ? ', ...' : '') + ')');
if (opt.profile >= 0) profileReport();
console.log('--- cycles ' + cyc + ', T=' + hx(T, 1) + ' U=' + hx(U, 1) + ' V=' + hx(V) + ' W=' + hx(W, 1) + ', vector RAM: ' + [...vecRam].map(v => hx(v, 4)).join(' '));
for (const d of opt.dumps) {
  const m = /^([0-9A-Fa-f]+)(?::(\d+))?(?:@([0-9A-Fa-f]))?$/.exec(d);
  if (!m) { console.log('bad --dump ' + d); continue; }
  const start = parseInt(m[1], 16), len = +(m[2] || 16), task = parseInt(m[3] || '0', 16);
  console.log('--- task ' + hx(task, 1) + ' $' + hx(start, 4) + ':');
  for (let a = start; a < start + len; a += 16)
    console.log(hx(a, 4) + ': ' + [...taskRam[task].slice(a, Math.min(a + 16, start + len, 0x8000))].map(v => hx(v)).join(' '));
}

// ---- profile report: instructions per routine (the nearest label at or below the PC, on its ROM page)
function profileReport() {
  const dbgFile = path.join(opt.rom, '..', 'obj', 'os_rom_C02.dbg');
  const lists = {};                                             // 'P0'-'PF' (BIOS ROM pages), 'A' (paged ROM), 'R' (RAM)
  if (fs.existsSync(dbgFile)) {
    const segs = {};
    for (const line of fs.readFileSync(dbgFile, 'utf8').split(/\r?\n/)) {
      const f = {}; for (const m of line.matchAll(/(\w+)=("[^"]*"|[^,\t]*)/g)) f[m[1]] = m[2].replace(/"/g, '');
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
  for (const [k, c] of profHist) { const n = "T" + hx(Math.floor(k / 1048576), 1) + " " + nameOf((k >> 16) & 15, k & 0xFFFF); byName.set(n, (byName.get(n) || 0) + c); }
  console.log('--- profile: ' + profCount + ' instructions from cycle ' + opt.profile + ' (by task: ' +
    profTask.map((c, t) => c ? hx(t, 1) + ' ' + (100 * c / profCount).toFixed(1) + '%' : '').filter(x => x).join(', ') + ') ---');
  for (const [n, c] of [...byName.entries()].sort((a, b) => b[1] - a[1]).slice(0, 30))
    console.log((100 * c / profCount).toFixed(1).padStart(5) + '%  ' + String(c).padStart(9) + '  ' + n);
}
