// cpu65c02.js - the W65C02S: its instructions (STZ, BRA, PHX/PLY, TSB/TRB, BBR/BBS, RMB/SMB, WAI, STP, (zp) mode,
// the 1-byte NOPs ...), cycle-counted from WDC's table: +1 for an indexed read crossing a page, +1 / +2 for a branch
// taken (to another page), +1 for ADC/SBC in decimal mode; 7 for an interrupt.
//
// createCpu(bus): bus.rd(a), bus.wr(a, v) are the memory; bus.pushed(S) is told each push (for the stack watch).
// The CPU keeps the cycle count (cyc) and, for the instruction being run, the cycle of its data access (ioAt:
// its last cycle, near enough), which the bus brings the devices up to for an I/O access.  step() runs one
// instruction; interrupt(vec) takes an interrupt.  No Node.js: it runs anywhere.
'use strict';

// W65C02S cycles per opcode (WDC data sheet), before the extras: +1 for a page crossed by an indexed read
// (PAGE_X), +1 for a branch taken and +1 more if it lands on another page, +1 for ADC/SBC in decimal mode.
// (BRA is 2 + 1 taken.)  The 1-byte NOPs (xxx3, xxxB) take 1 cycle.
const CYC = ('7621535532216465' + '2551546524216465' + '6621335542214465' + '2551446524214465' +
             '6621335532213465' + '2551446524318465' + '6621335542216465' + '2551446524416465' +
             '2621333522214445' + '2651444525214555' + '2621333522214445' + '2551444524214445' +
             '2621335522234465' + '2551446524334475' + '2621335522214465' + '2551446524414475').split('').map(Number);
const PAGE_X = new Uint8Array(256);
for (const o of [0x11, 0x19, 0x1D, 0x31, 0x39, 0x3D, 0x51, 0x59, 0x5D, 0x71, 0x79, 0x7D, 0xB1, 0xB9, 0xBD, 0xD1, 0xD9, 0xDD,
  0xF1, 0xF9, 0xFD, 0xBC, 0xBE, 0x3C, 0x1E, 0x3E, 0x5E, 0x7E]) PAGE_X[o] = 1;   // (Shifts abs,X too, on the 65C02)
const DECIMAL_X = new Uint8Array(256);
for (const o of [0x61, 0x65, 0x69, 0x6D, 0x71, 0x72, 0x75, 0x79, 0x7D, 0xE1, 0xE5, 0xE9, 0xED, 0xF1, 0xF2, 0xF5, 0xF9, 0xFD]) DECIMAL_X[o] = 1;
const C = 1, Z = 2, I = 4, D = 8, B = 0x10, Vf = 0x40, N = 0x80;
const hx = (v, n = 2) => v.toString(16).toUpperCase().padStart(n, '0');

function createCpu(bus) {
  const { rd, wr } = bus;
  const pushed = bus.pushed || (() => {});
  let A = 0, X = 0, Y = 0, S = 0xFD, P = 0x34, PC = 0;
  const cpu = { cyc: 0, ioAt: 0, waiting: false, halted: '', lastPC: 0, where: bus.where || (() => '') };
  Object.defineProperties(cpu, {                              // (The registers, for a trace or a report)
    A: { get: () => A, set: v => { A = v; } }, X: { get: () => X, set: v => { X = v; } }, Y: { get: () => Y, set: v => { Y = v; } },
    S: { get: () => S, set: v => { S = v; } }, P: { get: () => P, set: v => { P = v; } }, PC: { get: () => PC, set: v => { PC = v; } },
  });
  const setNZ = v => { P = (P & ~(N | Z)) | (v & 0x80) | (v ? 0 : Z); return v; };
  const push = v => { wr(0x100 + S, v); S = (S - 1) & 0xFF; pushed(S); };
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
  function irq(vec, brk) { push(PC >> 8); push(PC & 0xFF); push((P | 0x20) & (brk ? 0xFF : ~B)); P = (P | I) & ~D; PC = vec; }

  // An interrupt (IRQ: 7 cycles)
  cpu.interrupt = vec => { irq(vec, false); cpu.cyc += 7; };
  // RESET: the vector at $FFFC, IRQs off
  cpu.reset = () => { P = (P | I) & ~D; PC = rd16(0xFFFC); cpu.waiting = false; };
  // The instruction being run: its opcode, a page crossed by its indexing, its extra cycles; and its addressing
  // modes and operations, made once here (a step that made them would make some thirty objects an instruction)
  let op = 0, crossed = 0, extra = 0;
  const idx = (b, i) => { const r = (b + i) & 0xFFFF; crossed = (b ^ r) >> 8; return r; };
  const zp = () => fetch(), zpx = () => (fetch() + X) & 0xFF, zpy = () => (fetch() + Y) & 0xFF;
  const abs = () => fetch16(), absx = () => idx(fetch16(), X), absy = () => idx(fetch16(), Y);
  const indx = () => zp16(fetch() + X), indy = () => idx(zp16(fetch()), Y), indz = () => zp16(fetch());
  const br = c => { const o = fetch(); if (c) { const t = (PC + ((o ^ 0x80) - 0x80)) & 0xFFFF; extra += (t ^ PC) >> 8 ? 2 : 1; PC = t; } };
  const rmw = (addr, f) => wr(addr, f(rd(addr)) & 0xFF);
  const asl = v => { P = (P & ~C) | (v >> 7); return setNZ((v << 1) & 0xFF); };
  const lsr = v => { P = (P & ~C) | (v & 1); return setNZ(v >> 1); };
  const rol = v => { const c = P & C; P = (P & ~C) | (v >> 7); return setNZ(((v << 1) | c) & 0xFF); };
  const ror = v => { const c = P & C; P = (P & ~C) | (v & 1); return setNZ((v >> 1) | (c << 7)); };
  const bit = v => { P = (P & ~(N | Vf | Z)) | (v & 0xC0) | ((A & v) ? 0 : Z); };
  const ALU = [v => A = setNZ(A | v), v => A = setNZ(A & v), v => A = setNZ(A ^ v), adc, null, null, v => cmp(A, v), sbc];
  const CC1_MODE = [indx, zp, () => -1, abs, indy, zpx, absy, absx];                  // (cc = 1: bbb's mode; -1 immediate)
  const CC2_MODE = [null, zp, null, abs, null, zpx, null, absx];                       // (cc = 2: the shifts, INC, DEC)
  const CC2_OP = [asl, rol, lsr, ror, null, null, v => setNZ((v - 1) & 0xFF), v => setNZ((v + 1) & 0xFF)];
  const bad = () => { cpu.halted = 'unimplemented opcode $' + hx(op) + ' at ' + cpu.where((PC - 1) & 0xFFFF); };

  // One instruction; .vector() gives BRK's vector
  cpu.step = vector => {
    cpu.lastPC = PC;
    op = fetch();
    crossed = 0; extra = DECIMAL_X[op] && (P & D) ? 1 : 0;
    cpu.ioAt = cpu.cyc + CYC[op] - 1;                         // (Its data access: the last cycle, near enough)
    let a, v, t;
    const aaa = op >> 5, bbb = (op >> 2) & 7, cc = op & 3;
    switch (op) {
      case 0x00: PC = (PC + 1) & 0xFFFF; irq(vector(), true); break;                   // BRK
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
      case 0xCB: cpu.waiting = true; break;                                             // WAI
      case 0xDB: cpu.halted = 'STP at ' + cpu.where((PC - 1) & 0xFFFF); break;         // STP
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
      case 0x03: case 0x13: case 0x23: case 0x33: case 0x43: case 0x53: case 0x63: case 0x73: case 0x83: case 0x93: case 0xA3: case 0xB3:
      case 0xC3: case 0xD3: case 0xE3: case 0xF3: case 0x0B: case 0x1B: case 0x2B: case 0x3B: case 0x4B: case 0x5B: case 0x6B: case 0x7B:
      case 0x8B: case 0x9B: case 0xAB: case 0xBB: case 0xEB: case 0xFB: break;                                   // 1-byte NOPs
      default:
        if ((op & 0x0F) === 0x0F) { const n = (op >> 4) & 7, z = fetch(); const set = (rd(z) >> n) & 1; br((op & 0x80) ? set : !set); break; }   // BBR/BBS
        if ((op & 0x0F) === 0x07) { const n = (op >> 4) & 7, z = fetch(); v = rd(z); wr(z, (op & 0x80) ? v | (1 << n) : v & ~(1 << n)); break; } // RMB/SMB
        if (cc === 1) {
          a = CC1_MODE[bbb]();
          if (aaa === 4) { if (a >= 0) wr(a, A); else fetch(); break; }                // STA (no immediate)
          v = a < 0 ? fetch() : rd(a);
          if (aaa === 5) A = setNZ(v); else ALU[aaa](v);
          break;
        }
        if (cc === 2) {
          const mode = CC2_MODE[bbb], f = CC2_OP[aaa];
          if (!mode || !f) { bad(); break; }
          rmw(mode(), f); break;
        }
        bad();
    }
    cpu.cyc += CYC[op] + extra + (crossed && PAGE_X[op] ? 1 : 0);
  };
  return cpu;
}

module.exports = { createCpu, FLAGS: { C, Z, I, D, B, V: Vf, N } };
