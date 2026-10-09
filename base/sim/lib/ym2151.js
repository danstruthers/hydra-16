// ym2151.js - the YM2151 (port 4, IRQ line 4): $FF40 selects a register, $FF41 writes it and reads the status.
// Busy for 64 of its clocks (SND_CLK, 3.58 MHz) after each data write; a write while it's busy is lost on the chip
// (counted).  Its timers: A (10 bits, registers $10/$11: 64 x (1024 - NA) of its clocks) and B ($12: 1024 x
// (256 - NB)); register $14 loads (starts) them, enables their flags, resets the flags.  An enabled timer's
// overflow sets its status flag (bit 0 or 1), and a flag holds IRQ line 4 until it's reset: as on the chip, a
// reset takes effect as the write's busy time ends (and env.resetDelay cycles after: a slower chip, as some boards'
// seem to be), so the line is still held just after the write.  Key-ons (register
// $08) are noted, and every write when env.log is set (for a VGM file).
//   Its sound, if it's asked for (sound(fn): audio.js's): opm.js's samples, one every 64 of its clocks (55,930 a
// second), each given to fn(left, right) as it's made: up to the cycle each register write comes at (before the
// write changes what it sounds), and up to soundTo(t)'s.  The writes lost while it's busy are lost to it too.
'use strict';
const { createOpm } = require('./opm.js');

function createYm(env) {
  const clock = env.clock;
  const BUSY = Math.round(64 * clock / 3.579545);
  const y = { regs: new Uint8Array(256), reg: 0, busyUntil: 0, lost: 0, status: 0, aNext: -1, bNext: -1, keyOns: [], writes: [],
    resetAt: -1, resetMask: 0 };
  const aPeriod = () => Math.round(64 * (1024 - ((y.regs[0x10] << 2) | (y.regs[0x11] & 3))) * clock / 3.579545);
  const bPeriod = () => Math.round(1024 * (256 - y.regs[0x12]) * clock / 3.579545);
  function timers(v, t) {                                     // Register $14 written
    if (v & 1) { if (y.aNext < 0) y.aNext = t + aPeriod(); } else y.aNext = -1;
    if (v & 2) { if (y.bNext < 0) y.bNext = t + bPeriod(); } else y.bNext = -1;
    const m = (v & 0x10 ? 1 : 0) | (v & 0x20 ? 2 : 0);          // Flags reset: once the write's busy time ends
    if (m) { y.resetMask |= m; y.resetAt = y.busyUntil + (env.resetDelay || 0); }
  }
  y.tick = t => {
    if (y.resetAt >= 0 && t >= y.resetAt) { y.status &= ~y.resetMask; y.resetMask = 0; y.resetAt = -1; }
    if (y.aNext >= 0) while (t >= y.aNext) { if (y.regs[0x14] & 4) y.status |= 1; y.aNext += aPeriod(); }
    if (y.bNext >= 0) while (t >= y.bNext) { if (y.regs[0x14] & 8) y.status |= 2; y.bNext += bPeriod(); }
  };
  y.readStatus = t => { y.tick(t); return (t < y.busyUntil ? 0x80 : 0x00) | y.status; };   // Busy after a data write, the timer flags   // Busy after a data write, the timer flags
  // The sound (sound(fn)): opm.js, the samples made so far, and a sample's length in CPU cycles
  let opm = null, sink = null, made = 0;
  const perSample = 64 * clock / 3.579545, so = [0, 0];
  y.sound = fn => { sink = fn; opm = createOpm(); made = 0; };
  y.soundTo = t => {
    if (!opm) return;
    const n = Math.floor(t / perSample);
    for (; made < n; made++) { opm.sample(so); sink(so[0], so[1]); }
  };
  y.select = v => { y.reg = v; };
  y.write = (v, t) => {
    if (t < y.busyUntil) { y.lost++; return; }
    y.busyUntil = t + BUSY;
    y.regs[y.reg] = v;
    if (opm) { y.soundTo(t); opm.write(y.reg, v); }
    if (env.log) y.writes.push([t, y.reg, v]);
    if (y.reg === 0x14) timers(v, t);
    if (y.reg === 0x08 && (v & 0x78)) y.keyOns.push('ch ' + (v & 7) + ' at cycle ' + t);
  };
  y.irqActive = () => !!(y.status & 3);
  y.nextEvent = devCyc => {
    let n = Infinity;
    if (y.aNext >= 0 && (y.regs[0x14] & 4)) n = Math.min(n, Math.max(1, y.aNext - devCyc));
    if (y.bNext >= 0 && (y.regs[0x14] & 8)) n = Math.min(n, Math.max(1, y.bNext - devCyc));
    if (y.resetAt >= 0) n = Math.min(n, Math.max(1, y.resetAt - devCyc));
    return n;
  };
  y.reset = () => { y.busyUntil = 0; y.regs.fill(0); y.status = 0; y.aNext = -1; y.bNext = -1; y.resetAt = -1; y.resetMask = 0; if (opm) opm.reset(); };
  return y;
}

module.exports = { createYm };
