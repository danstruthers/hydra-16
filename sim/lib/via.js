// via.js - the 65C22 VIA (port 0, IRQ line 0): timer 1 (one-shot or free-running, IFR/IER: the scheduler's tick),
// timer 2 (one-shot), the shift register's timing and flag, port B (the SPI bus: env.portB(v) is told its output
// bits, env.miso() gives PB7), port A's inputs read high (the I2C pull-ups); the other registers are plain storage.
'use strict';

function createVia(env) {
  const v = {
    r: new Uint8Array(16),
    t1: 0xFFFF, t1Latch: 0xFFFF, t1On: false, ifr: 0, ier: 0,
    t2: 0xFFFF, t2LatchL: 0xFF, t2On: false,
    // The shift register: shifting (cycles until its 8 bits are done: its IFR flag), or -1.  CB1/CB2 aren't
    // brought out: modes 3 and 7 (CB1's clock) never finish, and shifting in reads 1s
    srLeft: -1,
  };
  const r = v.r;
  const portB = () => env.portB((r[0] & r[2]) | (~r[2] & 0x7F));
  // A shift register access: it clears the flag, and starts 8 shifts (mode 4, free-running, never sets it)
  function srStart() {
    const m = (r[0x0B] >> 2) & 7;
    v.ifr &= ~0x04;
    v.srLeft = m === 2 || m === 6 ? 16 : m === 1 || m === 5 ? 16 * (v.t2LatchL + 2) : -1;
    if (m === 1 || m === 2) r[0x0A] = 0xFF;                   // (Shifted in: CB2 floats high)
  }
  v.read = n => {
    if (n === 0) { const ddr = r[2], pins = 0x7F | (env.miso() << 7); return (r[0] & ddr) | (pins & ~ddr); }
    if (n === 1 || n === 0x0F) return (r[1] & r[3]) | (~r[3] & 0xFF);   // Port A: its inputs read high (pull-ups)
    if (n === 0x0A) { srStart(); return r[0x0A]; }
    if (n === 4) { v.ifr &= ~0x40; return v.t1 & 0xFF; }     // T1C-L: clears the T1 flag
    if (n === 5) return v.t1 >> 8;
    if (n === 8) { v.ifr &= ~0x20; return v.t2 & 0xFF; }     // T2C-L: clears the T2 flag
    if (n === 9) return (v.t2 >> 8) & 0xFF;
    if (n === 6) return v.t1Latch & 0xFF;
    if (n === 7) return v.t1Latch >> 8;
    if (n === 0x0D) return v.ifr | ((v.ifr & v.ier & 0x7F) ? 0x80 : 0);
    if (n === 0x0E) return v.ier | 0x80;
    return r[n];
  };
  v.write = (n, b) => {
    if (n === 4 || n === 6) v.t1Latch = (v.t1Latch & 0xFF00) | b;
    else if (n === 5) { v.t1Latch = (v.t1Latch & 0xFF) | (b << 8); v.t1 = v.t1Latch; v.t1On = true; v.ifr &= ~0x40; }
    else if (n === 7) { v.t1Latch = (v.t1Latch & 0xFF) | (b << 8); v.ifr &= ~0x40; }
    else if (n === 8) v.t2LatchL = b;
    else if (n === 9) { v.t2 = (b << 8) | v.t2LatchL; v.t2On = true; v.ifr &= ~0x20; }   // Load and start
    else if (n === 0x0D) v.ifr &= ~(b & 0x7F);
    else if (n === 0x0E) { if (b & 0x80) v.ier |= b & 0x7F; else v.ier &= ~(b & 0x7F); }
    else if (n === 0x0A) { r[0x0A] = b; srStart(); }
    else if (n === 0x0F) r[1] = b;
    else if (n === 0x0B) { r[0x0B] = b; if (!(b & 0x1C)) v.srLeft = -1; }
    else { r[n] = b; if (n === 0 || n === 2) portB(); }
  };
  v.tick = d => {                                            // (d can span several T1 periods: a WAI skipped ahead)
    if (v.srLeft >= 0 && (v.srLeft -= d) < 0) v.ifr |= 0x04;
    if (v.t2On) { v.t2 -= d; if (v.t2 < 0) { v.ifr |= 0x20; v.t2On = false; v.t2 &= 0xFFFF; } }
    if (!v.t1On) return;
    v.t1 -= d;
    while (v.t1 < 0) {
      v.ifr |= 0x40;
      if (r[0x0B] & 0x40) v.t1 += v.t1Latch + 2; else { v.t1 = 0xFFFF; v.t1On = false; }
    }
  };
  v.irqActive = () => !!(v.ifr & v.ier & 0x7F);
  v.nextEvent = () => {
    let n = Infinity;
    if (v.t1On) n = Math.min(n, v.t1 + 1);
    if (v.t2On) n = Math.min(n, v.t2 + 1);
    if (v.srLeft >= 0) n = Math.min(n, v.srLeft + 1);
    return n;
  };
  v.reset = () => { r.fill(0); v.ifr = 0; v.ier = 0; v.t1On = false; v.t2On = false; v.srLeft = -1; portB(); };   // (Port B: all inputs)
  return v;
}

module.exports = { createVia };
