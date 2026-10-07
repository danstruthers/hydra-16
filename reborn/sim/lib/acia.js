// acia.js - the 65C51 ACIA (port 1): Rockwell R65C51 (TDRE status and the TX interrupt) or WDC W65C51N (its bug:
// TDRE always reads 1, no TX interrupt; bytes written while one is still being sent are counted: they'd be
// garbled on the chip).  A character takes the time its baud rate, word length, parity and stop bits give, from
// its 1.790 MHz clock.  What it sends goes to env.onTx; what it receives comes from its input queue (keys, and
// '\u0100': wait 2M cycles before the next one; '\u0101': wait for a prompt, the output (grown since) ending with
// "> " or ">" (the old system's), "% " (rc's) or " <" (hylang's for more lines), and quiet for a while; '\u0102' to
// '\u0103': typed at the keyboard instead, env.keyboard's (the input controller's: smc.js), a key every KBD_DELAY
// cycles), a key every 20000 cycles from cycle 200000, or as fast as the line goes (env.paste: then a byte
// arriving while the last is unread is lost, as on the chip).  send(bytes): what the PC sends on its own (a /pc reply: hydrasim.js
// --pc-dir), back to back at the line's rate, ahead of the keys (a byte arriving while the last is unread is lost, and counted).
'use strict';

// The rates (the control register's low 4 bits: 0 = the external clock / 16), named for 1.8432 MHz; the ACIA's
// clock (SER_CLK) is the board's 14.318 MHz crystal / 8, 1.790 MHz, so every rate is 2.9% slow, as on the board
const ACIA_BAUD = [1843200 / 16, 50, 75, 109.92, 134.58, 150, 300, 600, 1200, 1800, 2400, 3600, 4800, 7200, 9600, 19200]
  .map(r => r * (14318180 / 8) / 1843200);

const KBD_DELAY = 150000;                                    // (A key at the keyboard: 24 a second; the SMC holds 15 codes)

function createAcia(env) {
  const wdc = !!env.wdc, paste = !!env.paste, clock = env.clock;
  const a = {
    cmd: 0, ctrl: 0, tdre: 1, txTimer: 0, irq: 0, rdrf: 0, rx: 0, overruns: 0,
    // The line's idle time between characters sent: when the last one ended, and the shortest gap (in bits, at
    // the rate of the character after it; 0 = back to back or overlapping), counted once 8 have gone at the rate
    // last set
    txEnd: -Infinity, txSent: 0, gapMin: Infinity,
    rxQueue: [...(env.input || '')], rxDelay: 200000, rxLost: 0, typedAt: -1, typedLast: -1, wdc, kbd: false,
    pcQueue: [], pcDelay: 0, pcLost: 0,                       // (What the PC sends on its own: send)
    sent: 0, tail: '', sentAt: 0, promptFrom: -1,             // (Bytes sent, the last two, when: for a wait for a prompt)
    rxAt: 0, rxLat: { min: Infinity, max: 0, sum: 0, n: 0 },  // (Each byte received: the cycles till the CPU read it)
  };
  // A prompt: sent since the wait began, and nothing after it for PROMPT_QUIET cycles (lines typed ahead are still
  // running until the output goes quiet)
  const PROMPT_QUIET = 300000;
  const prompted = t => a.sent > a.promptFrom && (a.tail.endsWith('> ') || a.tail.endsWith('>') || a.tail.endsWith('% ') || a.tail.endsWith(' <')) && t - a.sentAt >= PROMPT_QUIET;
  // The character time in CPU cycles: the baud rate, the word length and stop bits, and parity
  a.charCycles = () => {
    const bits = 1 + (8 - ((a.ctrl >> 5) & 3)) + ((a.cmd & 0x20) ? 1 : 0) + ((a.ctrl & 0x80) ? 2 : 1);
    return Math.round(bits * clock * 1e6 / ACIA_BAUD[a.ctrl & 15]);
  };
  a.read = (r, t) => {
    if (r === 0) {
      if (a.rdrf && t !== undefined) { const l = a.rxLat, n = t - a.rxAt; l.min = Math.min(l.min, n); l.max = Math.max(l.max, n); l.sum += n; l.n++; }
      a.rdrf = 0; return a.rx;
    }
    if (r === 1) { const s = (a.irq ? 0x80 : 0) | (a.tdre || wdc ? 0x10 : 0) | (a.rdrf ? 0x08 : 0); a.irq = 0; return s; }
    return r === 2 ? a.cmd : a.ctrl;
  };
  a.write = (r, v, t) => {
    if (r === 0) {
      if (a.txTimer > 0) a.overruns++;
      if (++a.txSent > 8) a.gapMin = Math.min(a.gapMin, a.txTimer > 0 ? 0 : (t - a.txEnd) * ACIA_BAUD[a.ctrl & 15] / (clock * 1e6));
      if (!env.consoleOnly) a.shown(v, t);
      env.onTx(v, t); a.tdre = 0; a.txTimer = a.charCycles();
    } else if (r === 1) { a.cmd &= 0xE0; a.irq = 0; }       // Programmed reset
    else if (r === 2) a.cmd = v;                              // (The TX interrupt comes as TDRE goes on, as on the
                                                              //   board: turning it on with TDRE on is nothing)
    else { a.ctrl = v; a.txSent = 0; a.gapMin = Infinity; }   // (A new rate: the gaps from here)
  };
  // d cycles have gone, to cycle t: a character sent, a key in
  a.tick = (d, t) => {
    if (a.txTimer > 0 && (a.txTimer -= d) <= 0) {             // (A character's time)
      a.txEnd = t + a.txTimer; a.txTimer = 0; a.tdre = 1; if (!wdc && (a.cmd & 0x0C) === 0x04) a.irq = 1;
    }
    if (a.pcQueue.length) {                                   // The PC's own bytes: back to back, the keys wait
      if ((a.pcDelay -= d) > 0) return;
      const c = a.pcQueue.shift();
      if (a.rdrf) a.pcLost++;
      else { a.rx = c; a.rdrf = 1; a.rxAt = t; if (!(a.cmd & 2)) a.irq = 1; }
      a.pcDelay = a.charCycles();
      if (!a.pcQueue.length && a.rxDelay < a.pcDelay) a.rxDelay = a.pcDelay;   // (A key after them: a character's time on)
      return;
    }
    if (a.rxQueue[0] === '\u0101') {                           // \p: a prompt (one sent since the wait began), then on
      if (a.promptFrom < 0) a.promptFrom = a.sent;
      if (!prompted(t)) return;
      a.rxQueue.shift(); a.promptFrom = -1; a.rxDelay = 20000; if (!a.rxQueue.length) a.typedLast = t;
    }
    if (a.rxQueue.length && (a.rxDelay -= d) <= 0 && (!a.rdrf || paste || a.kbd || a.rxQueue[0] === '\u0102')) {
      const c = a.rxQueue.shift();
      if (!a.rxQueue.length) a.typedLast = t;                  // (The input's end: --stop-after-input counts from here)
      if (c === '\u0102' || c === '\u0103') { a.kbd = c === '\u0102' && !!env.keyboard; a.rxDelay = a.kbd ? KBD_DELAY : 20000; }
      else if (a.kbd && c !== '\u0100') { env.keyboard(c); a.rxDelay = KBD_DELAY; }
      else if (c === '\u0100') { a.rxDelay = 2000000; if (!a.rxQueue.length) a.typedLast = t + a.rxDelay; }   // \w: wait before the next key
      else if (a.rdrf) { a.rxLost++; a.rxDelay = a.charCycles(); }   // (--paste: an overrun: the ACIA keeps the old byte)
      else { if (a.typedAt < 0) a.typedAt = t; a.rx = c.charCodeAt(0); a.rdrf = 1; a.rxAt = t; if (!(a.cmd & 2)) a.irq = 1; a.rxDelay = paste ? a.charCycles() : 20000; }
    }
  };
  a.irqActive = () => !!(a.irq && ((!wdc && (a.cmd & 0x0C) === 0x04 && a.tdre) || (!(a.cmd & 2) && a.rdrf)));
  a.nextEvent = () => {
    let n = Infinity;
    if (a.txTimer > 0) n = Math.min(n, a.txTimer);
    if (a.pcQueue.length) n = Math.min(n, Math.max(1, a.pcDelay));
    if (a.rxQueue.length && a.rxQueue[0] !== '\u0101') n = Math.min(n, Math.max(1, a.rxDelay));   // (A wait for a prompt: the output wakes it)
    return n;
  };
  // A byte of the console's output (for a wait for a prompt): every byte sent, or with env.consoleOnly (a /pc host
  // takes its frames out first: machine.js) only the ones its caller says are the console's
  a.shown = (v, t) => { a.sent++; a.tail = (a.tail + String.fromCharCode(v)).slice(-2); a.sentAt = t; };
  a.type = key => a.rxQueue.push(key);                        // A key typed (interactive)
  a.send = bytes => { if (!a.pcQueue.length) a.pcDelay = a.charCycles(); a.pcQueue.push(...bytes); };   // The PC's own bytes
  a.reset = () => { a.cmd = 0; a.ctrl = 0; a.tdre = 1; a.txTimer = 0; a.irq = 0; a.rdrf = 0; };
  return a;
}

module.exports = { createAcia, ACIA_BAUD };
