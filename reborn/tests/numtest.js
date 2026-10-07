// ****************************************************************************
// numtest.js - the numbers test's calls (tests.js, numbers): the card t_num reads them from (tests/mod/t_num: num.in,
// a record for each call), and the check of what each gave back (num.out) against the reference (sim/tools/numfmt.js;
// numref.js as the library grows).  The entries' slots and the constants are spec/numbers.def's.
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../sim/tools/hydrafs.js');
const numfmt = require('../sim/tools/numfmt.js');
const { readNumbers } = require('../tools/apigen.js');

const ROOT = path.join(__dirname, '..');
const NUMS = readNumbers(path.join(ROOT, 'spec', 'numbers.def'));
const K = Object.fromEntries(NUMS.consts.map(k => [k.name, k.value]));
const SLOT = Object.fromEntries(NUMS.libs.find(l => l.name === 'numbers').entries.map((e, i) => [e.name, i]));
const ERR = Object.fromEntries(NUMS.consts.filter(k => k.name.startsWith('NE_')).map(k => [k.value, k.name]));
const RESULT = 1, IN_BANK = 2;                                // (A record's flags)
const ARG_ROOM = [2560, 2560, 512], BANK_ARG_ROOM = [0x800, 0x800, 0x400];   // (t_num's room for r0, r1, r4)
const BANK_RES_ROOM = 0xC00;

const str = s => [...Buffer.from(s + '\0', 'latin1')];
const hex = b => Array.from(b, v => v.toString(16).toUpperCase().padStart(2, '0')).join(' ');

// A generator of its own, so the calls are the same each run
function rng(seed) {
  return n => { seed = (Math.imul(seed, 1103515245) + 12345) >>> 0; return (seed >>> 16) % n; };
}

// A number of every kind and size the format has: integers of 0 to 255 bytes, fixed decimals of 0 to 65535 places,
// rationals, complex numbers of them
function randomNumber(rnd) {
  const big = n => { let v = 0n; for (let k = 0; k < n; k++) v = (v << 8n) | BigInt(rnd(256)); return v; };
  const int = () => {
    const k = rnd(10), v = k < 3 ? BigInt(rnd(200)) : k < 8 ? big(1 + rnd(16)) : big(17 + rnd(239));
    return rnd(2) ? -v : v;
  };
  const real = () => {
    switch (rnd(3)) {
      case 0: return int();
      case 1: return { fix: int(), places: rnd(4) ? rnd(16) : rnd(3) ? 16 + rnd(400) : 65535 - rnd(10) };
      default: { const den = int(); return { num: int(), den: den === 0n ? 7n : den }; }
    }
  };
  for (;;) {
    try {
      const x = numfmt.norm(rnd(5) ? real() : { re: real(), im: real() });
      if (numfmt.encode(x).length <= 1100) return x;
    } catch (e) { if (!(e instanceof RangeError)) throw e; }
  }
}

// The calls, in order: each { op, flags, a, x, y, room, r0, r1, r4, want, what }.  An argument is a value, or
// { data, bank } (its bytes, in this task's RAM or in the bank at $8000).  want: { err } (C = 1, .A the error), { ok }
// (C = 0), or { bytes } (C = 0 and the result those bytes)
function calls(seed = 1066) {
  const rnd = rng(seed), list = [];
  const call = (op, o, want, what) => list.push({ op, flags: 0, room: 0, ...o, want, what });
  const where = d => ({ data: d, bank: !!rnd(2) });

  // ---- The state: INIT first, the base
  call('GET_BASE', { flags: RESULT, room: 16 }, { err: K.NE_INIT }, 'GET_BASE before INIT: NE_INIT');
  call('BYTES', { flags: RESULT, room: 16, r0: { data: [5] }, r1: 1 }, { err: K.NE_INIT }, 'BYTES before INIT: NE_INIT');
  call('INIT', {}, { ok: true }, 'INIT');
  call('GET_BASE', { flags: RESULT, room: 16 }, { bytes: [...Buffer.from('d')] }, 'the base at the start: d');
  call('GET_BASE', { flags: RESULT, room: 1 }, { err: K.NE_ROOM }, 'GET_BASE, room for 1 byte: NE_ROOM');
  call('GET_BASE', { flags: RESULT | IN_BANK, room: 2 }, { bytes: [...Buffer.from('d')] }, 'GET_BASE to the bank at $8000');
  call('SET_BASE', { r0: { data: str('#x') } }, { ok: true }, 'SET_BASE #x');
  call('GET_BASE', { flags: RESULT, room: 3 }, { bytes: [...Buffer.from('#x')] }, 'the base: #x');
  call('SET_BASE', { r0: { data: str('#16r'), bank: true } }, { ok: true }, 'SET_BASE #16r, from the bank at $8000');
  call('GET_BASE', { flags: RESULT | IN_BANK, room: 5 }, { bytes: [...Buffer.from('#16r')] }, 'the base: #16r');
  call('SET_BASE', { r0: { data: str('') } }, { err: K.NE_BASE }, 'SET_BASE "": NE_BASE');
  const longest = '#<=-[' + '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-=+`~!@#$%^&*,;:|?' + ']';
  call('SET_BASE', { r0: { data: str('x'.repeat(96)) } }, { err: K.NE_BASE }, 'SET_BASE of 96 characters: NE_BASE');
  call('GET_BASE', { flags: RESULT, room: 5 }, { bytes: [...Buffer.from('#16r')] }, 'the base still #16r');
  call('SET_BASE', { r0: { data: str(longest) } }, { ok: true }, 'SET_BASE, the longest base');
  call('GET_BASE', { flags: RESULT, room: 100 }, { bytes: [...Buffer.from(longest)] }, 'the base: the longest');
  call('SET_BASE', { r0: { data: str('d') } }, { ok: true }, 'SET_BASE d');
  call('SEED', { r0: 0x1234 }, { ok: true }, 'SEED $1234');
  call('SEED', { r0: 0 }, { ok: true }, 'SEED 0: from the clock');

  // ---- BYTES: the format's one form
  const bytes = (b, what, bad) => {
    let want = { err: K.NE_NOTNUM };
    if (!bad) {
      try { numfmt.decode(b); want = { bytes: b }; }
      catch (e) {
        if (!(e instanceof numfmt.FormatError)) throw e;
        if (/lowest terms/.test(e.message)) return;           // (Lowest terms: not till GCD's written)
      }
    }
    call('BYTES', { flags: RESULT | (rnd(2) ? IN_BANK : 0), room: BANK_RES_ROOM, r0: where(b), r1: b.length }, want,
      what + ' [' + hex(b.slice(0, 24)) + (b.length > 24 ? ' ...' : '') + ']');
  };
  const forms = [
    [[0x00], '0'], [[0x3F], '63'], [[0x40], '-64'], [[0x7F], '-1'],
    [[0x80, 0x40], '64'], [[0x80, 0x3F], '63, a byte'], [[0x90, 0x41], '-65'], [[0x90, 0x40], '-64, a byte'],
    [[0x81, 0x10, 0x00], 'a top byte of 0'], [[0x8F, ...Array(15).fill(1), 2], '16 bytes'],
    [[0xA0, 16, ...Array(16).fill(1)], '16 bytes, the long tag'], [[0xA0, 17, ...Array(17).fill(1)], '17 bytes'],
    [[0xA1, 255, ...Array(255).fill(0xEE)], '-255 bytes'], [[0xA0, 17, ...Array(16).fill(1), 0], '17 bytes, the top 0'],
    [[0xA2, 17, ...Array(17).fill(1)], 'the tag $A2'],
    [[0xB0, 0x00], '0.0'], [[0xB0, 0x0A], '10.0'], [[0xB1, 0x0A], '1.0'], [[0xB1, 0x00], '0 with a place'],
    [[0xB1, 0x0B], '1.1'], [[0xB2, 0x80, 0xC8], '2.00'], [[0xB2, 0x80, 0xC9], '2.01'], [[0xB1, 0x76], '-1.0'],
    [[0xB1, 0x77], '-0.9'], [[0xB3, 0x81, 0x10, 0x27], '10.000'], [[0xB3, 0x81, 0x11, 0x27], '10.001'],
    [[0xC0, 15, 0, 7], '15 places, the long tag'], [[0xC0, 16, 0, 7], '16 places'], [[0xC0, 0, 1, 7], '256 places'],
    [[0xC0, 0xFF, 0xFF, 7], '65535 places'], [[0xC0, 16, 0, 0x80, 100], '16 places, digits ending in 0'],
    [[0xC1, 1, 2], '1/2'], [[0xC1, 0, 3], '0/3', true], [[0xC1, 3, 1], '3/1'], [[0xC1, 3, 0], '3/0'], [[0xC1, 1, 0x7E], '1/-2'],
    [[0xC1, 0x7F, 2], '-1/2'], [[0xC1, 1, 0x80, 0x40], '1/64'], [[0xC1, 1, 0x90, 0x41], '1/-65'],
    [[0xC1, 1, 0xA0, 17, ...Array(17).fill(3)], '1/a 17-byte number'], [[0xC1, 1, 0xA1, 17, ...Array(17).fill(3)], '1/-a 17-byte number'],
    [[0xC1, 0xB1, 3, 2], 'a rational of a fixed decimal'],
    [[0xC2, 1, 2], '1+2i'], [[0xC2, 0, 1], 'i'], [[0xC2, 1, 0], '1+0i'], [[0xC2, 1, 0xB0, 0], '1+0.0i'],
    [[0xC2, 1, 0xB1, 3], '1+0.3i'], [[0xC2, 0xC2, 1, 2, 3], 'a complex number in one'], [[0xC2, 1, 0xC1, 1, 2], '1+1/2i'],
    [[0xC3], 'the tag $C3'], [[0xFF], 'the tag $FF'], [[], 'no bytes'], [[0x80], 'a byte short'], [[0x05, 0x00], 'a byte over'],
    [[0xC1, 1], 'a rational without its denominator'], [[0xC2, 1], 'a complex number without its imaginary part'],
  ];
  for (const [b, what, bad] of forms) bytes(b, what, bad);
  const big = numfmt.encode({ re: { num: (1n << 2039n) - 1n, den: (1n << 2040n) - 3n }, im: { num: -((1n << 2039n) + 1n), den: (1n << 2039n) + 3n } });
  bytes(big, 'the longest number (' + big.length + ' bytes)');
  for (let k = 0; k < 600; k++) {                             // (Numbers of every kind, and the same changed a little)
    const b = numfmt.encode(randomNumber(rnd));
    bytes(b, 'a number');
    const m = b.slice();
    switch (rnd(5)) {
      case 0: m[rnd(m.length)] = rnd(256); break;
      case 1: m.push(rnd(256)); break;
      case 2: m.pop(); break;
      case 3: m[0] = 0xA0 + rnd(0x30); break;
      default: { const i = rnd(m.length); m[i] = m[i] ^ (1 << rnd(8)); }
    }
    bytes(m, 'a number changed');
  }
  const two = numfmt.encode(123456789n);
  call('BYTES', { flags: RESULT, room: two.length - 1, r0: { data: two }, r1: two.length }, { err: K.NE_ROOM }, 'BYTES, room for one byte less: NE_ROOM');
  call('BYTES', { flags: RESULT, room: two.length, r0: { data: two }, r1: two.length }, { bytes: two }, 'BYTES, room for it exactly');
  call('BYTES', { flags: RESULT, room: 64, r0: { data: two }, r1: two.length - 1 }, { err: K.NE_NOTNUM }, 'BYTES, a count short of the number: NE_NOTNUM');
  call('BYTES', { flags: RESULT, room: 64, r0: { data: two }, r1: two.length + 1 }, { err: K.NE_NOTNUM }, 'BYTES, a count past it: NE_NOTNUM');

  // ---- The entries not written yet
  for (const e of NUMS.libs.find(l => l.name === 'numbers').entries)
    if (!['INIT', 'SET_BASE', 'GET_BASE', 'SEED', 'BYTES'].includes(e.name))
      call(e.name, { flags: RESULT, room: 64, r0: { data: [5] }, r1: { data: [6] } }, { err: K.NE_TODO }, e.name + ': not written yet');
  return list;
}

// num.in: the calls' records (tests/mod/t_num/t_num.s), then $FF
function record(c) {
  const b = [SLOT[c.op], c.flags, c.a | 0, c.x | 0, c.y | 0, c.room & 255, c.room >> 8];
  [c.r0, c.r1, c.r4].forEach((g, i) => {
    if (g === undefined || typeof g === 'number') b.push(0, (g | 0) & 255, ((g | 0) >> 8) & 255);
    else {
      if (g.data.length > (g.bank ? BANK_ARG_ROOM : ARG_ROOM)[i]) throw new Error(c.what + ': its data is too long for t_num');
      b.push(g.bank ? 2 : 1, g.data.length & 255, g.data.length >> 8, ...g.data);
    }
  });
  if ((c.flags & IN_BANK) && c.room > BANK_RES_ROOM) throw new Error(c.what + ': room past the bank\'s for a result');
  return b;
}

// The test's card: num.in, and num.out empty (t_num writes it)
const CARD_DIR = path.join(ROOT, 'obj', 'cards');
function card(list) {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'num0.img');
  hydrafs.mkfs(f, 8, 'NUM', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('num.in', Buffer.from([...list.flatMap(record), 0xFF]));
  v.put('num.out', Buffer.alloc(0));
  v.close();
  return f;
}

// What the calls gave back (num.out), against what they should: a list of failures
function check(list, out) {
  const f = [];
  let at = 0;
  const say = (c, why) => { if (f.length < 20) f.push(c.what + ': ' + why); else if (f.length === 20) f.push('...'); };
  for (const c of list) {
    if (at + 11 > out.length) { f.push('num.out ends at call ' + list.indexOf(c) + ' of ' + list.length + ' (' + c.what + ')'); break; }
    const [carry, a, x] = out.subarray(at, at + 3), kept = out[at + 10];
    at += 11;
    let got = null;
    if (!carry && (c.flags & RESULT)) { got = out.subarray(at, at + a + 256 * x); at += a + 256 * x; }
    if (kept) say(c, 'it didn\'t keep ' + ['the bank at $8000', '$78-$7F', 'r0-r3'].filter((w, i) => kept & (1 << i)).join(', '));
    const w = c.want, err = carry ? (ERR[a] || '$' + a.toString(16)) : null;
    if (w.err !== undefined) { if (!carry || a !== w.err) say(c, (carry ? err : 'no error') + ', not ' + ERR[w.err]); }
    else if (carry) say(c, err);
    else if (w.bytes && !Buffer.from(w.bytes).equals(got)) say(c, '[' + hex(got) + '], not [' + hex(w.bytes) + ']');
  }
  if (!f.length && at !== out.length) f.push('num.out: ' + (out.length - at) + ' bytes after the last call\'s');
  return f;
}

module.exports = { calls, card, check, K, SLOT };
