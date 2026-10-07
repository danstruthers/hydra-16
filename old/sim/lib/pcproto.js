// pcproto.js - /pc's line protocol: the frames the Hydra and the PC tool send each other on the serial port,
// between the console's own bytes (docs/plans/PC.md).  No Node.js in it: the emulator (hydrasim.js --pc-dir) and
// the PC tool (tools/hydrapc.js) both use it, with the file server (tools/pcfs.js).
//
// A frame: MARK, then its body with MARK and ESC stuffed (each sent as ESC, the byte ^ $20): the type, the tag,
// the payload's length (2 bytes, low first), the payload, and a CRC-16 (CCITT: $1021, from $FFFF; low byte first)
// of everything from the type to the payload's end.  A MARK always starts a frame: one cut short is dropped.
// From the PC, MARK then ESC (where the type would be) is a MARK typed on the console (Ctrl-^); and the PC stuffs
// Ctrl-C, Ctrl-\ and Ctrl-] too, the keys reborn's console acts on as they come in, so its frames never have one (a
// reader takes any byte after ESC as the byte ^ $20, so both versions of the Hydra take them).
//
// Two versions of what the frames carry, the attach's payload naming the Hydra's: 1, the old system's (a request's
// first REQ_HDR bytes of its H9 block, a reply's REPLY_HDR bytes: status, value, count); 2, reborn's (the whole
// request block, appendix B of docs/reimplementation-from-scratch.md, REQ_HDR2 bytes; a reply's REPLY_HDR2: status,
// fid, count, qid type).  The PC tool's file server (tools/pcfs.js) speaks both.
'use strict';

const MARK = 0x1E, ESC = 0x1F;
const STUFFED = new Set([MARK, ESC, 0x03, 0x1C, 0x1D]);    // (Ctrl-C, Ctrl-\, Ctrl-]: the PC's frames only)
const T_ATTACH = 0x41, T_REQ = 0x51;                       // 'A', 'Q': the Hydra's
const T_REPLY = 0x52, T_NAK = 0x4E;                        // 'R', 'N': the PC's
const REQ_HDR = 16, REPLY_HDR = 4;                         // A request's copy of the request block; a reply's
const REQ_HDR2 = 32, REPLY_HDR2 = 5;                       //   status, value and count (then the data): 1, and 2
const MAX_PAYLOAD = REQ_HDR2 + 256;                        // (A request's data: 256 bytes at most)
const MAX_REPLY = REPLY_HDR2 + 256;                        // (The Hydra drops a frame longer than it takes)

function crc16(bytes, crc = 0xFFFF) {
  for (const b of bytes) {
    crc ^= b << 8;
    for (let i = 0; i < 8; i++) crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xFFFF : (crc << 1) & 0xFFFF;
  }
  return crc;
}

// A frame as the bytes the PC sends
function encode(type, tag, payload = []) {
  const body = [type, tag & 0xFF, payload.length & 0xFF, payload.length >> 8, ...payload];
  const crc = crc16(body);
  body.push(crc & 0xFF, crc >> 8);
  const out = [MARK];
  for (const b of body) if (STUFFED.has(b)) out.push(ESC, b ^ 0x20); else out.push(b);
  return Uint8Array.from(out);
}

// Frames out of a byte stream that has other bytes between them (the PC tool reads the Hydra's console output,
// the emulator its serial port).  push(byte) returns the bytes that aren't a frame's, as they're known to be: a
// MARK starts a frame, held until it's whole and its CRC is right (then onFrame({ type, tag, payload })), or
// until it can't be one (a type the reader doesn't take, a length beyond max, or flush()): then its bytes as they
// came are the stream's.  A frame whose CRC is wrong but looked right otherwise is dropped, and goes to
// onBad({ type, tag }) (the PC asks for it again).
function createReader({ types, max = MAX_PAYLOAD, onFrame, onBad = () => {} }) {
  let raw = null, body = null, esc = false;
  const r = {};
  const giveUp = () => { const out = raw; raw = null; body = null; esc = false; return out; };
  r.inFrame = () => raw !== null;
  r.flush = () => raw ? giveUp() : [];
  r.push = b => {
    if (b === MARK) { const out = raw ? giveUp() : []; raw = [b]; body = []; return out; }
    if (!raw) return [b];
    raw.push(b);
    if (b === ESC && !esc) { esc = true; return []; }
    if (esc) { b ^= 0x20; esc = false; }
    body.push(b);
    if (body.length === 1 && !types.includes(b)) return giveUp();
    if (body.length < 4) return [];
    const len = body[2] | body[3] << 8;
    if (len > max) return giveUp();
    if (body.length < 4 + len + 2) return [];
    const crc = crc16(body.slice(0, 4 + len)), got = body[4 + len] | body[5 + len] << 8;
    if (crc !== got) { onBad({ type: body[0], tag: body[1] }); giveUp(); return []; }   // (A frame, damaged: not shown)
    const f = { type: body[0], tag: body[1], payload: Uint8Array.from(body.slice(4, 4 + len)) };
    raw = null; body = null;
    onFrame(f);
    return [];
  };
  return r;
}

module.exports = { MARK, ESC, T_ATTACH, T_REQ, T_REPLY, T_NAK, REQ_HDR, REPLY_HDR, REQ_HDR2, REPLY_HDR2, MAX_PAYLOAD, MAX_REPLY, crc16, encode,
  createReader };
