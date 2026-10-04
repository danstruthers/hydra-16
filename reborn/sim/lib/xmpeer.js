// xmpeer.js - the PC's end of XMODEM, played in the emulator for a test (through the machine's pcHost hook: what
// the serial port sends goes through push, and send puts the PC's bytes on the line, at its rate).  Sessions in
// turn, each armed when its trigger shows in the console's output; the Hydra's bytes are the session's from then
// till its end, and the rest are the console's.
//   { trigger, role: 'send', data, k, damage, again, cancel }   the PC sends (the Hydra's xmodem -r): on the
//       Hydra's first ask, 'C' (a CRC) or NAK (a checksum); 128-byte blocks, or 1K ones with k (and a CRC); the
//       blocks numbered in damage go damaged the first time (a data byte's bit 6 flipped: it NAKs), those in again
//       twice (their ACK as good as lost: it ACKs the second and keeps one), and at block cancel, CAN CAN instead
//   { trigger, role: 'receive', crc, nak }   the PC receives (the Hydra's xmodem -s): the test types its start ('C'
//       with crc, else NAK: a key, as the emulator can't time one here); each block checked (crc: a CRC, else the
//       sum), ACKed, or NAKed (the blocks numbered in nak, the first time, too); its data in got, padding and all
// createXmodemPeer(sessions) gives { push(byte) -> the console's bytes, flush(), send, sessions }; each session ends
// with done (true, or 'cancelled'), and log: what happened, in short ("blk 1", "nak 2", "eot" ...).
'use strict';

const SOH = 1, STX = 2, EOT = 4, ACK = 6, NAK = 0x15, CAN = 0x18, SUB = 0x1A;

function crc16(bytes) {
  let c = 0;
  for (const b of bytes) {
    c ^= b << 8;
    for (let i = 0; i < 8; i++) c = c & 0x8000 ? ((c << 1) ^ 0x1021) & 0xFFFF : (c << 1) & 0xFFFF;
  }
  return c;
}
const sum8 = bytes => bytes.reduce((a, b) => (a + b) & 0xFF, 0);

function createXmodemPeer(sessions) {
  const p = { send: null, sessions };
  let tail = '', s = null, next = 0;
  let crc = true, blk = 1, ofs = 0, size = 128, state = '', hurt = new Set(), twice = new Set(), buf = [];

  function arm() {
    s = sessions[next++];
    s.log = []; s.got = []; s.done = false;
    state = 'start'; blk = 1; ofs = 0; buf = []; hurt = new Set(); twice = new Set();
  }
  function end(how) { s.done = how; s.log.push(how === true ? 'done' : how); s = null; tail = ''; }
  const out = bytes => p.send(Uint8Array.from(bytes));

  // The PC sends: block blk from ofs (or EOT, past the data's end)
  function sendBlock() {
    if (s.cancel === blk) { out([CAN, CAN]); end('cancelled'); return; }
    if (ofs >= s.data.length) { state = 'eot'; s.log.push('eot'); out([EOT]); return; }
    size = s.k && crc ? 1024 : 128;
    const data = Array.from({ length: size }, (_, i) => ofs + i < s.data.length ? s.data[ofs + i] : SUB);
    const check = crc ? [crc16(data) >> 8, crc16(data) & 0xFF] : [sum8(data)];
    const bytes = [size === 1024 ? STX : SOH, blk & 0xFF, 255 - (blk & 0xFF), ...data, ...check];
    if ((s.damage || []).includes(blk) && !hurt.has(blk)) { hurt.add(blk); bytes[3 + 10] ^= 0x40; s.log.push('damaged ' + blk); }
    else s.log.push('blk ' + blk);
    state = 'block';
    out(bytes);
  }
  function sender(b) {
    if (state === 'start') {
      if (b === 0x43 || b === NAK) { crc = b === 0x43; s.log.push(crc ? 'crc' : 'sum'); sendBlock(); }
    } else if (state === 'block') {
      if (b === NAK) sendBlock();
      else if (b === ACK) {
        if ((s.again || []).includes(blk) && !twice.has(blk)) { twice.add(blk); s.log.push('again ' + blk); sendBlock(); return; }
        ofs += size; blk++; sendBlock();
      }
    } else if (state === 'eot') {
      if (b === ACK) end(true);
      else if (b === NAK) out([EOT]);
    }
  }

  // The PC receives: a block's bytes gathered, then checked
  function receiver(b) {
    if (!buf.length) {
      if (b === EOT) { out([ACK]); end(true); return; }
      if (b === CAN) { if (state === 'can') end('cancelled'); else state = 'can'; return; }
      state = 'start';
      if (b !== SOH && b !== STX) return;                     // (Anything else: not a block's)
    }
    buf.push(b);
    const len = (buf[0] === STX ? 1024 : 128) + 3 + (s.crc ? 2 : 1);
    if (buf.length < len) return;
    const n = buf[1], k = s.crc ? 2 : 1, data = buf.slice(3, len - k), check = buf.slice(len - k);
    const ok = buf[2] === 255 - n && (s.crc ? (check[0] << 8 | check[1]) === crc16(data) : check[0] === sum8(data));
    buf = [];
    if (!ok) { s.log.push('bad ' + n); out([NAK]); return; }
    if ((s.nak || []).includes(n) && !hurt.has(n)) { hurt.add(n); s.log.push('nak ' + n); out([NAK]); return; }
    if (n === (blk & 0xFF)) { s.got.push(...data); s.log.push('blk ' + n + (data.length === 1024 ? ' 1K' : '')); blk++; }
    else s.log.push('again ' + n);
    out([ACK]);
  }

  p.push = b => {
    if (s) { (s.role === 'send' ? sender : receiver)(b); return []; }
    tail = (tail + String.fromCharCode(b)).slice(-200);
    if (next < sessions.length && tail.includes(sessions[next].trigger)) arm();
    return [b];
  };
  p.flush = () => [];
  return p;
}

module.exports = { createXmodemPeer, crc16 };
