// ovlpack.js - a Dingus Quest Zero task's overlays' file packed for its start (each task's make.js calls it): ld65's
// file (obj/games/NAME.hyx.ovl: each overlay filled out to 8K with $FF) cut after the last overlay that has code, each
// bank's fill at its end left out and the rest packed (LZ4's block format, a bank on its own): obj/games/NAME.ovp, a
// bank after another, each its 4 bytes' header (L, its bytes, 0-8192; C, their packed length, 0 if they're stored as
// they are: two bytes each, low first) then its C bytes (or its L).  The unpack library (modules/unpack: the format's
// told there) reads a bank's packed bytes to its end and unpacks them in place from its start, then fills the rest
// with $FF, so the bank's the same as ld65 made it; a bank whose unpacking would write over a byte it hasn't read yet
// (or that doesn't pack smaller) is stored.  The card's read the fewer bytes (the start's time is the card's, some
// 270 cycles a byte; unpacking about 40): 2026-10-31.  Each file's checked here as it's made: unpacked as the library
// does it, in place, every bank the same as ld65's.  The card's file is lib/dq0/NAME.ovl (sdcard/sdcard.txt).
// A match's offset is sixteen bits back from where it goes, and the library's pointers plain addresses: so a match may
// reach below the window into the task's own main RAM, its code and read-only data, there and unchanged as the overlays
// are read (each task's first act; only its DATA changes before it, measured: 2026-11-03, the margins' pass): some 10%
// fewer bytes off the card, for no more code or time unpacking.  dq0starttime compares every bank in the emulator.
// OUT: { banks, bytes, raw } (the packed file's banks and bytes, the bytes of the banks unpacked)
'use strict';
const fs = require('fs');
const path = require('path');

const BANK = 0x2000, MIN = 4, MAXOFF = 0xFFFF, WINDOW = 0x8000, LOAD = 0x0800, HEADER = 0x30;

// LZ4's block (the library's form) of data, at the window's start: greedy, with a look one byte ahead for a longer match;
// its last sequence's literals end it (no match after them).  dict: { at, bytes }, what's in the main RAM below the window
// for a match to reach, or none
function compress(data, dict) {
  const n = data.length, out = [], heads = new Map(), end = WINDOW + n, all = Buffer.alloc(end), prev = new Int32Array(end).fill(-1);
  const dend = dict ? dict.at + dict.bytes.length : 0;
  data.copy(all, WINDOW);
  const key = i => all[i] | all[i + 1] << 8 | all[i + 2] << 16 | all[i + 3] << 24;
  const insert = i => { if (i + MIN <= end && prev[i] === -1 && heads.get(key(i)) !== i) { const k = key(i); prev[i] = heads.has(k) ? heads.get(k) : -1; heads.set(k, i); } };
  if (dict) {
    dict.bytes.copy(all, dict.at);
    for (let i = dict.at; i + MIN <= dend; i++) insert(i);
  }
  const best = j => {
    const i = WINDOW + j;
    if (i + MIN > end) return { len: 0, off: 0 };
    let p = heads.has(key(i)) ? heads.get(key(i)) : -1, len = 0, off = 0, tries = 256;
    while (p >= 0 && tries-- > 0 && i - p <= MAXOFF) {
      const top = p < WINDOW ? dend - p : Infinity;       // (Not past the main RAM's part it may reach)
      let l = 0;
      while (l < top && i + l < end && all[p + l] === all[i + l]) l++;
      if (l > len) { len = l; off = i - p; }
      p = prev[p];
    }
    return len >= MIN ? { len, off } : { len: 0, off: 0 };
  };
  const ext = v => { while (v >= 255) { out.push(255); v -= 255; } out.push(v); };
  const emit = (litFrom, litTo, m) => {
    const ll = litTo - litFrom, ml = m ? m.len - MIN : 0;
    out.push((Math.min(ll, 15) << 4) | (m ? Math.min(ml, 15) : 0));
    if (ll >= 15) ext(ll - 15);
    for (let i = litFrom; i < litTo; i++) out.push(data[i]);
    if (m) {
      out.push(m.off & 255, m.off >> 8);
      if (ml >= 15) ext(ml - 15);
    }
  };
  let i = 0, lit = 0;
  while (i < n) {
    const m = best(i);
    insert(WINDOW + i);
    if (m.len) {
      const m2 = best(i + 1);
      if (m2.len > m.len + 1) { i++; continue; }      // (A longer one a byte on: this byte a literal)
      emit(lit, i, m);
      for (let k = i + 1; k < i + m.len; k++) insert(WINDOW + k);
      i += m.len;
      lit = i;
    } else {
      i++;
    }
  }
  emit(lit, n, null);
  return Buffer.from(out);
}

// A bank unpacked as the library does it: its packed bytes p at the bank's end, unpacked from its start, in place.
// OUT: the bank (BANK bytes, its fill $FF), or null if a byte's written over one not yet read
function unpackInPlace(p, len, dict) {
  const b = Buffer.alloc(BANK, 0);
  let src = BANK - p.length, dst = 0;
  p.copy(b, src);
  const byte = () => b[src++];
  const more = v => { let x; do { x = byte(); v += x; } while (x === 255); return v; };
  for (;;) {
    const tok = byte();
    let ll = tok >> 4;
    if (ll === 15) ll = more(ll);
    for (let k = 0; k < ll; k++) { if (dst >= src) return null; b[dst++] = byte(); }
    if (src >= BANK) break;
    const off = byte() | byte() << 8;
    let ml = tok & 15;
    if (ml === 15) ml = more(ml);
    ml += MIN;
    for (let k = 0; k < ml; k++) {
      if (dst >= src) return null;
      if (dst >= off) b[dst] = b[dst - off];
      else {                                            // (Below the window: the task's main RAM)
        const at = WINDOW + dst - off - (dict ? dict.at : 0);
        if (!dict || at < 0 || at >= dict.bytes.length) return null;
        b[dst] = dict.bytes[at];
      }
      dst++;
    }
  }
  if (dst !== len) return null;
  b.fill(0xFF, dst);
  return b;
}

// The task's main RAM as a match may reach it: its program's image, past its header, up to its DATA (which its start
// changes before the overlays are read): { at, bytes }
function mainRam(objdir, name) {
  const lbl = /^al ([0-9A-F]+) \.__DATA_RUN__$/m.exec(fs.readFileSync(path.join(objdir, name + '.lbl'), 'latin1'));
  if (!lbl) throw new Error('ovlpack: ' + name + ': no __DATA_RUN__ in its labels');
  const hyx = fs.readFileSync(path.join(objdir, '..', name + '.hyx')), top = parseInt(lbl[1], 16) - LOAD;
  if (top <= HEADER || top > hyx.length) throw new Error('ovlpack: ' + name + ': its DATA not in its image');
  return { at: LOAD + HEADER, bytes: hyx.subarray(HEADER, top) };
}

function pack(objdir, name) {
  const data = fs.readFileSync(path.join(objdir, '..', name + '.hyx.ovl'));
  const bank = k => {                                   // (ld65's bank, filled out to 8K)
    const b = Buffer.alloc(BANK, 0xFF);
    data.copy(b, 0, k * BANK, Math.min(data.length, (k + 1) * BANK));
    return b;
  };
  const dict = mainRam(objdir, name);
  let n = Math.ceil(data.length / BANK);
  while (n > 1 && bank(n - 1).every(b => b === 0xFF)) n--;
  const parts = [];
  let raw = 0;
  for (let k = 0; k < n; k++) {
    const b = bank(k);
    let e = BANK;
    while (e > 0 && b[e - 1] === 0xFF) e--;
    const body = b.subarray(0, e);
    let p = e ? compress(body, dict) : Buffer.alloc(0);
    if (p.length >= e || !unpackInPlace(p, e, dict)) p = null;     // (Stored: it doesn't pack smaller, or can't in place)
    const head = Buffer.alloc(4);
    head.writeUInt16LE(e, 0);
    head.writeUInt16LE(p ? p.length : 0, 2);
    parts.push(head, p || body);
    raw += e;
  }
  const out = Buffer.concat(parts);
  check(out, n, bank, name, dict);
  fs.writeFileSync(path.join(objdir, '..', name + '.ovp'), out);
  return { banks: n, bytes: out.length, raw };
}

// The packed file read back as the library reads it, each bank the same as ld65's, or the build fails
function check(f, n, bank, name, dict) {
  let at = 0, k = 0;
  while (at < f.length) {
    const len = f.readUInt16LE(at), c = f.readUInt16LE(at + 2);
    at += 4;
    let b;
    if (c) b = unpackInPlace(f.subarray(at, at + c), len, dict);
    else { b = Buffer.alloc(BANK, 0xFF); f.copy(b, 0, at, at + len); }
    at += c || len;
    if (!b || !b.equals(bank(k))) throw new Error('ovlpack: ' + name + '\'s bank ' + (k + 1) + ' doesn\'t unpack as ld65 made it');
    k++;
  }
  if (k !== n) throw new Error('ovlpack: ' + name + ': ' + k + ' banks read back, not ' + n);
}

module.exports = { pack, compress, unpackInPlace, mainRam };
