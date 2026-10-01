// ym2151.js - the YM2151 (port 4, IRQ line 4): $FF40 selects a register, $FF41 writes it and reads the status.
// Busy for 64 of its clocks (SND_CLK, 3.58 MHz) after each data write; a write while it's busy is lost on the chip
// (counted).  Its timers: A (10 bits, registers $10/$11: 64 x (1024 - NA) of its clocks) and B ($12: 1024 x
// (256 - NB)); register $14 loads (starts) them, enables their flags, resets the flags.  An enabled timer's
// overflow sets its status flag (bit 0 or 1), and a flag holds IRQ line 4 until it's reset.  Key-ons (register
// $08) are noted, and every write when env.log is set (for a VGM file).
'use strict';

function createYm(env) {
  const clock = env.clock;
  const BUSY = Math.round(64 * clock / 3.579545);
  const y = { regs: new Uint8Array(256), reg: 0, busyUntil: 0, lost: 0, status: 0, aNext: -1, bNext: -1, keyOns: [], writes: [] };
  const aPeriod = () => Math.round(64 * (1024 - ((y.regs[0x10] << 2) | (y.regs[0x11] & 3))) * clock / 3.579545);
  const bPeriod = () => Math.round(1024 * (256 - y.regs[0x12]) * clock / 3.579545);
  function timers(v, t) {                                     // Register $14 written
    if (v & 1) { if (y.aNext < 0) y.aNext = t + aPeriod(); } else y.aNext = -1;
    if (v & 2) { if (y.bNext < 0) y.bNext = t + bPeriod(); } else y.bNext = -1;
    if (v & 0x10) y.status &= ~1;
    if (v & 0x20) y.status &= ~2;
  }
  y.tick = t => {
    if (y.aNext >= 0) while (t >= y.aNext) { if (y.regs[0x14] & 4) y.status |= 1; y.aNext += aPeriod(); }
    if (y.bNext >= 0) while (t >= y.bNext) { if (y.regs[0x14] & 8) y.status |= 2; y.bNext += bPeriod(); }
  };
  y.readStatus = t => (t < y.busyUntil ? 0x80 : 0x00) | y.status;   // Busy after a data write, the timer flags
  y.select = v => { y.reg = v; };
  y.write = (v, t) => {
    if (t < y.busyUntil) { y.lost++; return; }
    y.busyUntil = t + BUSY;
    y.regs[y.reg] = v;
    if (env.log) y.writes.push([t, y.reg, v]);
    if (y.reg === 0x14) timers(v, t);
    if (y.reg === 0x08 && (v & 0x78)) y.keyOns.push('ch ' + (v & 7) + ' at cycle ' + t);
  };
  y.irqActive = () => !!(y.status & 3);
  y.nextEvent = devCyc => {
    let n = Infinity;
    if (y.aNext >= 0 && (y.regs[0x14] & 4)) n = Math.min(n, Math.max(1, y.aNext - devCyc));
    if (y.bNext >= 0 && (y.regs[0x14] & 8)) n = Math.min(n, Math.max(1, y.bNext - devCyc));
    return n;
  };
  y.reset = () => { y.busyUntil = 0; y.regs.fill(0); y.status = 0; y.aNext = -1; y.bNext = -1; };
  return y;
}

module.exports = { createYm };
