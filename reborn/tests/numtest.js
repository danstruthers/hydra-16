// ****************************************************************************
// numtest.js - the numbers test's calls (tests.js, numbers): the card t_num reads them from (tests/mod/t_num: num.in,
// a record for each call), and the check of what each gave back (num.out) against the references (sim/tools/numfmt.js,
// numref.js).  The entries' slots and the constants are spec/numbers.def's.
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../sim/tools/hydrafs.js');
const numfmt = require('../sim/tools/numfmt.js');
const R = require('../sim/tools/numref.js');
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
// rationals, complex numbers of them.  small: integers to 16 bytes (most to 6), to 40 places (an arithmetic's
// answers seldom too big, and quick)
function randomNumber(rnd, small = false) {
  const big = n => { let v = 0n; for (let k = 0; k < n; k++) v = (v << 8n) | BigInt(rnd(256)); return v; };
  const int = () => {
    const k = rnd(10), v = k < 3 ? BigInt(rnd(200)) : small ? big(k < 9 ? 1 + rnd(6) : 7 + rnd(10)) : k < 8 ? big(1 + rnd(16)) : big(17 + rnd(239));
    return rnd(2) ? -v : v;
  };
  const real = () => {
    switch (rnd(3)) {
      case 0: return int();
      case 1: return { fix: int(), places: rnd(4) ? rnd(16) : small ? 16 + rnd(25) : rnd(3) ? 16 + rnd(400) : 65535 - rnd(10) };
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

// ---- The library's way (nmval.inc), for where it's too big: numref.js's integers have no end, the library's registers
// 255 bytes.  Integers and fixed decimals are worked on their digits (scaled to the same places for a sum, a
// difference or an order; a product's places both's); with a rational, or for a quotient, each is loaded as a
// fraction (an integer's denominator 1, a fixed decimal's 10^places).  Each register's value is checked as it's made
// (a product's operands' lengths 256 at most together, as r_mul has it), and the result is written the kind the
// tower's rules give; a complex number by its parts, as the machine's programs have it
const TOO_BIG = () => new R.NumError('BIG');
const nbytes = v => { let a = v < 0n ? -v : v, n = 0; while (a > 0n) { a >>= 8n; n++; } return n; };
const reg = v => { if (nbytes(v) > 255) throw TOO_BIG(); return v; };
const rmul = (a, b) => { if (a === 0n || b === 0n) return 0n; if (nbytes(a) + nbytes(b) > 256) throw TOO_BIG(); return reg(a * b); };
const rpow10 = n => { let v = 1n; for (let k = 0; k < n; k++) v = reg(v * 10n); return v; };
const isCpx = x => typeof x === 'object' && 're' in x, isRat = x => typeof x === 'object' && 'num' in x;
const isFix = x => typeof x === 'object' && 'fix' in x;
function lload(x) {
  if (isFix(x)) return { n: x.fix, d: rpow10(x.places), pl: x.places, rat: false };
  if (isRat(x)) return { n: x.num, d: x.den, pl: 0, rat: true };
  return { n: x, d: 1n, pl: 0, rat: false };
}
function lrat(n, d) {
  if (d === 0n) throw new R.NumError('DIV0');
  return numfmt.norm({ num: n, den: d });
}
const digits = x => isFix(x) ? { n: x.fix, pl: x.places } : { n: x, pl: 0 };
function scaled(x, y) {                                       // (n_scaled: the digits at the more places)
  const a = digits(x), b = digits(y), pl = Math.max(a.pl, b.pl);
  return { m: reg(a.n * 10n ** BigInt(pl - a.pl)), n: reg(b.n * 10n ** BigInt(pl - b.pl)), pl };
}
function lreal(op, x, y) {
  if (typeof x === 'bigint' && typeof y === 'bigint' && op !== 'div')
    return reg(op === 'add' ? x + y : op === 'sub' ? x - y : rmul(x, y));
  if (op !== 'div' && !isRat(x) && !isRat(y)) {
    if (op === 'mul') {
      const a = digits(x), b = digits(y), pl = a.pl + b.pl;
      if (pl > 65535) throw TOO_BIG();
      return numfmt.norm({ fix: rmul(a.n, b.n), places: pl });
    }
    const t = scaled(x, y);
    return numfmt.norm({ fix: reg(op === 'add' ? t.m + t.n : t.m - t.n), places: t.pl });
  }
  const a = lload(x), b = lload(y);
  let n, d, pl;
  if (op === 'add' || op === 'sub') {
    const t1 = rmul(a.n, b.d), t2 = rmul(b.n, a.d);
    n = reg(op === 'add' ? t1 + t2 : t1 - t2); d = rmul(a.d, b.d); pl = Math.max(a.pl, b.pl);
  } else if (op === 'mul') {
    n = rmul(a.n, b.n); d = rmul(a.d, b.d); pl = a.pl + b.pl;
    if (pl > 65535) throw TOO_BIG();
  } else {
    n = rmul(a.n, b.d); d = rmul(a.d, b.n);
    return d < 0n ? lrat(-n, -d) : lrat(n, d);
  }
  return lrat(n, d);
}
const lparts = x => isCpx(x) ? [x.re, x.im] : [x, 0n];
const lcpx = (re, im) => R.isZero(im) ? re : { re, im };
function lop(op, x, y) {
  if (op === 'div' && !isCpx(y) && R.isZero(y)) throw new R.NumError('DIV0');
  if (!isCpx(x) && !isCpx(y)) return lreal(op, x, y);
  const [a, b] = lparts(x), [c, e] = lparts(y), r = lreal;
  switch (op) {
    case 'add': return lcpx(r('add', a, c), r('add', b, e));
    case 'sub': return lcpx(r('sub', a, c), r('sub', b, e));
    case 'mul': return lcpx(r('sub', r('mul', a, c), r('mul', b, e)), r('add', r('mul', a, e), r('mul', b, c)));
    default: {
      const n = r('add', r('mul', c, c), r('mul', e, e));
      return lcpx(r('div', r('add', r('mul', a, c), r('mul', b, e)), n), r('div', r('sub', r('mul', b, c), r('mul', a, e)), n));
    }
  }
}
function lrcmp(x, y) {
  if (typeof x === 'bigint' && typeof y === 'bigint') return x < y ? -1 : x > y ? 1 : 0;
  if (!isRat(x) && !isRat(y)) { const t = scaled(x, y); return t.m < t.n ? -1 : t.m > t.n ? 1 : 0; }
  const a = lload(x), b = lload(y), l = rmul(a.n, b.d), m = rmul(b.n, a.d);
  return l < m ? -1 : l > m ? 1 : 0;
}
function lcmp(x, y) {
  if (!isCpx(x) && !isCpx(y)) return lrcmp(x, y);
  const [a, b] = lparts(x), [c, e] = lparts(y);
  return lrcmp(a, c) || lrcmp(b, e);
}

// What a call gives back, from the library's way (and checked against numref.js where it isn't too big)
function want(f) {
  try { return f(); }
  catch (e) { if (e instanceof R.NumError) return { err: K['NE_' + e.code] }; throw e; }
}
const nbytesOf = x => { const b = numfmt.encode(x); return { bytes: b }; };

// The calls, in order: each { op, flags, a, x, y, room, r0, r1, r4, want, what }.  An argument is a value, or
// { data, bank } (its bytes, in this task's RAM or in the bank at $8000).  want: { err } (C = 1, .A the error), { ok }
// (C = 0), { bytes } (C = 0 and the result those bytes), or { a, x } (C = 0, and .A and .X those)
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

  // ---- The arithmetic: ADD, SUB, MUL, DIV (each checked against numref.js too), NEG, ABS, CMP, KIND
  const OPS = { ADD: ['add', R.add], SUB: ['sub', R.sub], MUL: ['mul', R.mul], DIV: ['div', R.div] };
  const show = x => numfmt.show(x).slice(0, 40);
  const operand = (small = true) => numfmt.encode(randomNumber(rnd, small));
  const arith = (op, x, y, what) => {
    const [lo, ref] = OPS[op], w = want(() => {
      const v = lop(lo, x, y), r = ref(x, y);
      if (!Buffer.from(numfmt.encode(v)).equals(Buffer.from(numfmt.encode(r)))) throw new Error(op + ': the library\'s way and numref.js differ: ' + show(x) + ', ' + show(y));
      return nbytesOf(v);
    });
    call(op, { flags: RESULT | (rnd(2) ? IN_BANK : 0), room: BANK_RES_ROOM, r0: where(numfmt.encode(x)), r1: where(numfmt.encode(y)) }, w,
      what || op + ' ' + show(x) + ', ' + show(y));
  };
  const n = t => numfmt.parse(t);
  for (const [op, a, b] of [['ADD', '1', '2'], ['ADD', '0.1', '0.2'], ['SUB', '1/3', '1/3'], ['MUL', '1.5', '2'], ['DIV', '1', '3'],
    ['DIV', '6', '3'], ['DIV', '1', '0'], ['DIV', '1', '0.0'], ['DIV', '1+2i', '3-4i'], ['MUL', '2i', '2i'], ['ADD', '1+2i', '1-2i'],
    ['SUB', '-64', '1'], ['ADD', '63', '1'], ['MUL', '1.25', '0.8'], ['DIV', '2.5', '0.5'], ['ADD', '1/2', '0.5'], ['SUB', '0.10', '0.1']])
    arith(op, n(a), n(b), op + ' ' + a + ', ' + b);
  const max = (1n << 2040n) - 1n;
  arith('ADD', max, 1n, 'ADD the greatest integer, 1: too big');
  arith('SUB', -max, 1n, 'SUB -the greatest integer, 1: too big');
  arith('ADD', max, -1n, 'ADD the greatest integer, -1');
  arith('MUL', (1n << 1024n), (1n << 1015n), 'MUL two halves, 255 bytes');
  arith('MUL', (1n << 1024n), (1n << 1016n), 'MUL two halves, 256 bytes: too big');
  for (let k = 0; k < 800; k++) { const op = ['ADD', 'SUB', 'MUL', 'DIV'][k & 3]; arith(op, numfmt.decode(operand()), numfmt.decode(operand())); }
  for (let k = 0; k < 40; k++) { const op = ['ADD', 'SUB', 'MUL', 'DIV'][k & 3]; arith(op, numfmt.decode(operand(false)), numfmt.decode(operand(!(k & 4)))); }
  for (let k = 0; k < 300; k++) {
    const x = numfmt.decode(operand()), y = rnd(8) ? numfmt.decode(operand()) : x;
    call('CMP', { r0: where(numfmt.encode(x)), r1: where(numfmt.encode(y)) },
      want(() => { const c = lcmp(x, y); if (c !== R.cmp(x, y)) throw new Error('CMP: the library\'s way and numref.js differ'); return { a: c & 255, x: 0 }; }),
      'CMP ' + show(x) + ', ' + show(y));
  }
  for (let k = 0; k < 450; k++) {
    const x = numfmt.decode(operand()), op = ['NEG', 'ABS', 'KIND'][k % 3];
    const w = op === 'NEG' ? want(() => nbytesOf(R.neg(x))) : op === 'ABS' ? want(() => nbytesOf(R.abs(x)))
      : { a: R.kind(x), x: R.sign(isCpx(x) ? x.re : x) & 255 };
    call(op, { flags: op === 'KIND' ? 0 : RESULT | (rnd(2) ? IN_BANK : 0), room: BANK_RES_ROOM, r0: where(numfmt.encode(x)) }, w, op + ' ' + show(x));
  }
  call('ADD', { flags: RESULT, room: 0, r0: { data: [1] }, r1: { data: [2] } }, { err: K.NE_ROOM }, 'ADD 1, 2, no room: NE_ROOM');
  call('ADD', { flags: RESULT, room: 1, r0: { data: [1] }, r1: { data: [2] } }, { bytes: [3] }, 'ADD 1, 2, room for it');
  call('ADD', { flags: RESULT, room: 16, r0: { data: [0xC3] }, r1: { data: [2] } }, { err: K.NE_NOTNUM }, 'ADD a tag $C3: NE_NOTNUM');
  call('DIV', { flags: RESULT, room: 16, r0: { data: [1] }, r1: { data: [0xC1, 0, 3] } }, { err: K.NE_DIV0 }, 'DIV by 0/3 (not its one form): NE_DIV0');
  call('DIV', { flags: RESULT, room: 16, r0: { data: [1] }, r1: { data: [0xB2, 0] } }, { err: K.NE_DIV0 }, 'DIV by 0.00 (not its one form): NE_DIV0');
  call('ADD', { flags: RESULT, room: 16, r0: { data: [0xC1, 1, 0] }, r1: { data: [2] } }, { err: K.NE_DIV0 }, 'ADD 1/0 (not a number): NE_DIV0');

  // ---- The entries not written yet
  for (const e of NUMS.libs.find(l => l.name === 'numbers').entries)
    if (!['INIT', 'SET_BASE', 'GET_BASE', 'SEED', 'BYTES', 'ADD', 'SUB', 'MUL', 'DIV', 'NEG', 'ABS', 'CMP', 'KIND'].includes(e.name))
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
    else if (w.a !== undefined && (a !== w.a || x !== w.x)) say(c, '.A, .X = $' + hex([a, x]) + ', not $' + hex([w.a, w.x]));
  }
  if (!f.length && at !== out.length) f.push('num.out: ' + (out.length - at) + ' bytes after the last call\'s');
  return f;
}

module.exports = { calls, card, check, K, SLOT };
