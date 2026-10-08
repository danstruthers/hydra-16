// smc.js - the input controller: the X16's SMC (its System Management Controller, an ATtiny861 with the X16
// community's firmware, x16-smc) on the I2C bus at $42, a PS/2 keyboard and mouse on it (docs/design/plans/VIDEO.md,
// step 6).  Its answers are the firmware's (x16-smc.ino's I2C_Receive and I2C_Send, smc_wire.cpp):
//   * a write's first byte is the register, its second (and third) the values; at the transaction's end, with a
//     value the command's done (and the register is the default request again), without one the register's kept
//     for the next read;
//   * a read's answer is made as its address comes, into a buffer: from the register (the default request after a
//     read or a command: $41 at power-up, $40 sets it); with nothing in it, the address isn't acknowledged, and past
//     its end the bytes are $FF;
//   * $07 and $41: a key code (0: none; $41's an empty buffer, then); $21 and $42: a mouse packet (0: none); $43: a
//     key code and a mouse packet (an empty buffer if neither);
//   * key codes are the IBM PC/AT's key numbers (keyboard.c's, x16-emulator's), bit 7 a release; a mouse packet is
//     the PS/2 mouse's (byte 0 its buttons, bit 3 set, the signs of x and y in bits 4 and 5; then x and y, y up; a
//     4th, the wheel, with the mouse in mode 3 or 4: $20 asks for it, $22 reads it);
//   * $30-$32 its version; $18 the keyboard's command status (0: none pending); $19 V and $1A V W, a command to the
//     keyboard ($ED W: its LEDs, W's bit 0 Scroll Lock, 1 Num Lock, 2 Caps Lock); the rest ($01, $02, $03: power,
//     reset, NMI) logged.
// The keyboard's buffer holds KBD_SIZE - 1 codes, the mouse's MSE_SIZE - 1 bytes; more are lost (counted).
// The host's side: key(n, down), type(ch) (a character as the keys a US keyboard types it with; \u0200 + n key n
// pressed and let go, \u0300 + n pressed, \u0380 + n let go; \u0400 the next of env.moves, a move's arguments),
// move(dx, dy, buttons, wheel) (the screen's way: y down; the wheel's down positive), and env.log(text) for the
// commands it ignores.
'use strict';

const KBD_SIZE = 16, MSE_SIZE = 16;
const VERSION = [47, 2, 3];
// The US keyboard: each printable character's key number, and whether it's shifted
const US = {};
const rows = [
  [1, '`1234567890-=', '~!@#$%^&*()_+'], [17, 'qwertyuiop[]\\', 'QWERTYUIOP{}|'],
  [31, 'asdfghjkl;\'', 'ASDFGHJKL:"'], [46, 'zxcvbnm,./', 'ZXCVBNM<>?'],
];
for (const [first, plain, shifted] of rows) {
  [...plain].forEach((c, i) => { US[c] = [first + i, false]; });
  [...shifted].forEach((c, i) => { US[c] = [first + i, true]; });
}
US[' '] = [61, false];
const K = { BACKSPACE: 15, TAB: 16, ENTER: 43, LSHIFT: 44, LCTRL: 58, ESC: 110 };

function createSmc(env = {}) {
  const log = env.log || (() => {});
  const s = {
    keys: [], mouse: [], reg: 0x41, dflt: 0x41, data: [], buf: [], at: 0, reading: false,
    mouseId: env.mouse === false ? -1 : 0, leds: 0, lost: 0, mouseLost: 0, reads: 0, nacks: 0,
    commands: [],                                             // (The keyboard's: [V, W] each, as $1A took them)
    moves: (env.moves || []).slice(),                         // (type's \u0400: the next, move's arguments)
  };
  const msize = () => (s.mouseId === 3 || s.mouseId === 4 ? 4 : 3);
  // The I2C side (i2c.js's device)
  s.addr = rw => {
    if (!rw) { s.data = []; s.reading = false; return true; }
    s.reading = true;
    s.buf = answer(s.reg);
    s.at = 0;
    s.reg = s.dflt;
    s.reads++;
    if (!s.buf.length) { s.nacks++; return false; }
    return true;
  };
  s.put = v => { if (s.data.length < 3) s.data.push(v); return true; };
  s.get = () => (s.at < s.buf.length ? s.buf[s.at++] : 0xFF);
  s.end = () => {
    if (s.reading) { s.reading = false; return; }
    const d = s.data;
    s.data = [];
    if (!d.length) return;
    s.reg = d[0];
    if (d.length < 2) return;                                 // (A register alone: the next read's)
    switch (d[0]) {
      case 0x40: s.dflt = d[1]; break;
      case 0x20: s.mouseId = s.mouseId < 0 ? -1 : (d[1] === 3 || d[1] === 4 ? d[1] : 0); s.mouse = []; break;
      case 0x1A: if (d.length >= 3) { s.commands.push([d[1], d[2]]); if (d[1] === 0xED) s.leds = d[2] & 7; } break;
      case 0x19: s.commands.push([d[1]]); break;             // (A one-byte command to the keyboard)
      default: log('smc: command $' + d[0].toString(16).padStart(2, '0') + ' $' + d[1].toString(16).padStart(2, '0'));
    }
    s.reg = s.dflt;
  };
  function keyCode(out, always) {
    if (s.keys.length) { out.push(s.keys.shift()); return true; }
    if (always) out.push(0);
    return false;
  }
  function packet(out) {
    const n = msize();
    if (s.mouseId >= 0 && s.mouse.length >= n) { for (let i = 0; i < n; i++) out.push(s.mouse.shift()); return true; }
    out.push(0);
    return false;
  }
  function answer(r) {
    const out = [];
    switch (r) {
      case 0x07: keyCode(out, true); break;
      case 0x41: if (!keyCode(out, true)) out.length = 0; break;
      case 0x21: packet(out); break;
      case 0x42: if (!packet(out)) out.length = 0; break;
      case 0x43: { const k = keyCode(out, true), p = packet(out); if (!k && !p) out.length = 0; break; }
      case 0x18: out.push(0); break;
      case 0x22: out.push(s.mouseId < 0 ? 0xFF : s.mouseId); break;
      case 0x30: case 0x31: case 0x32: out.push(VERSION[r - 0x30]); break;
    }
    return out;
  }
  // The host's side
  s.key = (n, down = true) => {
    if (s.keys.length >= KBD_SIZE - 1) { s.lost++; return; }
    s.keys.push((n & 0x7F) | (down ? 0 : 0x80));
  };
  const tap = n => { s.key(n, true); s.key(n, false); };
  s.type = ch => {
    const c = ch.charCodeAt(0);
    if (c === 0x400) { const mv = s.moves.shift(); if (mv) s.move(...mv); return; }
    if (c >= 0x380 && c < 0x400) return s.key(c - 0x380, false);
    if (c >= 0x300) return s.key(c - 0x300, true);
    if (c >= 0x200) return tap(c - 0x200);
    if (ch === '\r' || ch === '\n') return tap(K.ENTER);
    if (ch === '\t') return tap(K.TAB);
    if (ch === '\b' || c === 0x7F) return tap(K.BACKSPACE);
    if (c === 0x1B) return tap(K.ESC);
    if (c >= 1 && c <= 26) { s.key(K.LCTRL, true); tap(US[String.fromCharCode(c + 96)][0]); return s.key(K.LCTRL, false); }
    const k = US[ch];
    if (!k) return;
    if (k[1]) s.key(K.LSHIFT, true);
    tap(k[0]);
    if (k[1]) s.key(K.LSHIFT, false);
  };
  // A move (y down, as on the screen) and the buttons (bit 0 left, 1 right, 2 middle): packets of at most 255 a
  // step, as the mouse sends them
  s.move = (dx, dy, buttons = 0, wheel = 0) => {
    if (s.mouseId < 0) return;
    let x = dx, y = -dy;
    do {
      const sx = Math.max(-255, Math.min(255, x)), sy = Math.max(-255, Math.min(255, y));
      const p = [0x08 | (buttons & 7) | (sx < 0 ? 0x10 : 0) | (sy < 0 ? 0x20 : 0), sx & 0xFF, sy & 0xFF];
      if (msize() === 4) p.push(s.mouseId === 4 ? wheel & 0x0F : wheel & 0xFF);   // (Mode 4: 4 bits, mode 3: 8)
      if (s.mouse.length + p.length > MSE_SIZE - 1) { s.mouseLost++; return; }
      s.mouse.push(...p);
      x -= sx; y -= sy; wheel = 0;
    } while (x || y);
  };
  return s;
}

module.exports = { createSmc, US_KEYS: US, SMC_VERSION: VERSION };
