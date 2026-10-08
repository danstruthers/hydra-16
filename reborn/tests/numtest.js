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
const RESULT = 1, IN_BANK = 2, SECOND = 4;                    // (A record's flags)
const ARG_ROOM = [2560, 2560, 512, 512, 512], BANK_ARG_ROOM = [0x800, 0x800, 0x400, 0x400, 0x400];   // (r0, r1, r4-r6)
const PLACE2 = { place2: true };                              // (r5: the second result's place)
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

// POW: numref.js's loop, its products the library's (lop)
function lpow(x, n) {
  if (n < 0n) { const p = lpow(x, -n); if (!isCpx(p) && R.isZero(p)) throw new R.NumError('DIV0'); return lop('div', 1n, p); }
  let r = 1n, b = x;
  for (; n > 0n; n >>= 1n) { if (n & 1n) r = lop('mul', r, b); if (n > 1n) b = lop('mul', b, b); }
  return r;
}
// TO_FIXED of a rational: hylang's digit at a time, its registers checked
function lToFixed(x, places) {
  if (isCpx(x)) throw new R.NumError('REAL');
  if (!isRat(x)) return R.toFixed(x, places);
  const neg = x.num < 0n, n = neg ? -x.num : x.num, d = x.den;
  let w = n / d, r = n % d, dec = 0;
  while (r > 0n && dec < places) { ++dec; r = reg(r * 10n); w = reg(reg(w * 10n) + r / d); r %= d; }
  return numfmt.norm({ fix: neg ? -w : w, places: dec });
}
// TO_RATIONAL, NUMERATOR, DENOMINATOR: a fixed decimal through 10^places
function lfrac(x) {
  if (isCpx(x)) throw new R.NumError('REAL');
  if (isFix(x)) rpow10(x.places);
  return R.toRational(x);
}
// BITS: and, or, xor work a byte past the longer (255 bytes: too big); shl past 2040 bits; counts 16 bits
const word = v => v > 32767n ? 32767n : v < -32768n ? -32768n : v;
const intOf = x => R.bits('and', x, -1n);                    // (numref.js's: NE_INT for a number not equal to an integer)
function lbits(op, x, y) {
  const ia = intOf(x);
  if (op === 'not') return reg(~ia);
  const ib = intOf(y);
  if (['and', 'or', 'xor'].includes(op)) { if (Math.max(nbytes(ia), nbytes(ib)) === 255) throw TOO_BIG(); return R.bits(op, ia, ib); }
  let k = word(ib), dir = op === 'shl' ? 1 : -1;
  if (k < 0n) { k = -k; dir = -dir; }
  if (dir > 0) { if (ia !== 0n && ia.toString(2).replace('-', '').length + Number(k) > 2040) throw TOO_BIG(); return ia << k; }
  return ia >> k;
}
// FIB: hylang's doubling, its registers checked
function lfib(n) {
  n = intOf(n);
  if (n < 0n) throw new R.NumError('INT');
  let m = Number(word(n)), a = 0n, b = 1n;
  for (let k = 0; k < 16; k++) {
    const t = rmul(a, reg(reg(b + b) - a)), aa = rmul(a, a), bb = rmul(b, b);
    b = reg(aa + bb); a = t;
    if (m & 0x8000) { const c = reg(a + b); a = b; b = c; }
    m = (m << 1) & 0xFFFF;
  }
  return a;
}

// DISPLAY: where it's too big (t_real): a fixed decimal of 615 places or more in a base not decimal (10^places); a
// radix point's n * (base^k / d) past 255 bytes (as r_mul has it)
function ldisplay(x, f, pre = null) {
  const p = pre === null ? f.prefix : pre;
  if (!(f.isDecimal && !p)) {
    let g = f;
    if (isCpx(x) && (f.hasDigit('+') || f.hasDigit('-') || f.hasDigit('i'))) g = R.DECIMAL;
    if (!g.isDecimal) for (const r of isCpx(x) ? [x.re, x.im] : [x]) {
      if (isFix(r)) rpow10(r.places);
      const q = R.toRational(r);
      const n = isRat(q) ? q.num : q, d = isRat(q) ? q.den : 1n;
      const b = BigInt(g.chars.length);
      let dd = d, c = 1n, ok = true;
      while (dd !== 1n) { let a = dd % b, gg = b; while (a) [gg, a] = [a, gg % a]; if (gg === 1n) { ok = false; break; } dd /= gg; c = reg(c * (b / gg)); }
      if (ok && d !== 1n) rmul(n, c);
    }
  }
  return R.display(x, f, pre);
}
// PARSE: its text's longest start that's a number (its first 255 characters), as numref.js's parse reads it
function lparse(text, fmt, source) {
  const t = text.slice(0, 255);
  for (let L = t.length; L > 0; L--) {
    let v;
    try { v = R.parse(t.slice(0, L), fmt, source); }
    catch (e) { if (e instanceof R.NumError) return { err: K['NE_' + e.code] }; throw e; }
    if (v !== null) {
      try { return { bytes: numfmt.encode(v), used: L }; }
      catch (e) { if (e instanceof RangeError) return { err: K.NE_BIG }; throw e; }
    }
  }
  return { err: K.NE_NOTNUM };
}
// PARSE with .Y's bit 1: the whole text (255 characters at most) or nothing, as numref.js's parse reads it
function lwhole(text, fmt, source) {
  if (text.length > 255) return { err: K.NE_NOTNUM };
  let v;
  try { v = R.parse(text, fmt, source); }
  catch (e) { if (e instanceof R.NumError) return { err: K['NE_' + e.code] }; throw e; }
  if (v === null) return { err: K.NE_NOTNUM };
  try { return { bytes: numfmt.encode(v), used: text.length }; }
  catch (e) { if (e instanceof RangeError) return { err: K.NE_BIG }; throw e; }
}
// FORMAT: danlang's format; args: { num } or { str }; cur the base for {}
function lformat(fmt, args, cur) {
  let out = '', ai = 0;
  for (let i = 0; i < fmt.length; i++) {
    const next = fmt[i + 1];
    if (fmt[i] === '{' && next === '{') { out += '{'; i++; }
    else if (fmt[i] === '}' && next === '}') { out += '}'; i++; }
    else if (fmt[i] === '{' && fmt.indexOf('}', i + 1) > i) {
      const close = fmt.indexOf('}', i + 1), spec = fmt.slice(i + 1, close);
      if (ai >= args.length) throw new R.NumError('FORMAT');
      const a = args[ai++];
      if (spec === '') out += a.str !== undefined ? a.str : ldisplay(a.num, cur);
      else { if (a.str !== undefined) throw new R.NumError('FORMAT'); out += ldisplay(a.num, R.NumberFormat.of(spec)); }
      i = close;
    } else out += fmt[i];
  }
  if (ai < args.length) throw new R.NumError('FORMAT');
  return out;
}
// t_num's buffers' addresses (its labels: obj/tests/t_num/t_num.lbl), for FORMAT's arguments' table
function tnumLabel(name) {
  const m = new RegExp('^al ([0-9A-F]+) \\.' + name + '\\s*$', 'm').exec(fs.readFileSync(path.join(ROOT, 'obj', 'tests', 't_num', 't_num.lbl'), 'latin1'));
  if (!m) throw new Error('t_num.lbl: no ' + name);
  return parseInt(m[1], 16);
}
const BANK_ARG1 = 0x8800;

// What a call gives back, from the library's way (and checked against numref.js where it isn't too big)
function want(f) {
  try { return f(); }
  catch (e) { if (e instanceof R.NumError) return { err: K['NE_' + e.code] }; throw e; }
}
const nbytesOf = x => { const b = numfmt.encode(x); return { bytes: b }; };

// The calls, in order: each { op, flags, a, x, y, room, r0, r1, r4, r5, r6, want, what }.  An argument is a value,
// { data, bank } (its bytes, in this task's RAM or in the bank at $8000), or PLACE2 (the second result's place).
// want: { err } (C = 1, .A the error), or C = 0 and: { ok }; { bytes } (the result those bytes; bytes2 the second
// result's; used: r5 those characters, PARSE's); { a, x } (.A and .X those); r4, r5 (those registers after the call)
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
  for (let k = 0; k < 300; k++) {                             // (Numbers of every kind, and the same changed a little)
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

  // ---- Integers, conversions, bits, random numbers, Fibonacci
  const ints = () => numfmt.decode(numfmt.encode(BigInt.asIntN(8 * (1 + rnd(6)), BigInt(rnd(1 << 30)) * BigInt(rnd(1 << 30)) + BigInt(rnd(1 << 20)))));
  const smallInt = () => BigInt(rnd(41) - 20);
  const enc = x => numfmt.encode(x);
  const real = () => { for (;;) { const x = numfmt.decode(operand()); if (!isCpx(x)) return x; } };
  const anyNum = () => numfmt.decode(operand());
  for (let k = 0; k < 120; k++) {
    const x = k % 10 ? ints() : anyNum(), y = k % 7 ? (rnd(4) ? ints() : smallInt()) : rnd(3) ? 0n : anyNum();
    const w = want(() => { const r = R.idiv(x, y); return { bytes: enc(r.q), bytes2: enc(r.r) }; });
    call('IDIV', { flags: RESULT | SECOND, room: 64, r0: where(enc(x)), r1: where(enc(y)), r5: PLACE2, r6: 64 }, w, 'IDIV ' + show(x) + ', ' + show(y));
  }
  call('IDIV', { flags: RESULT | SECOND, room: 64, r0: { data: enc(1000n) }, r1: { data: enc(7n) }, r5: PLACE2, r6: 0 }, { err: K.NE_ROOM }, 'IDIV 1000, 7, no room for the remainder: NE_ROOM');
  for (let k = 0; k < 80; k++) {
    const x = k % 9 ? ints() : 0n, y = k % 11 ? ints() * BigInt(1 + rnd(5)) : k % 2 ? 0n : 5n;
    call('GCD', { flags: RESULT, room: 64, r0: where(enc(x)), r1: where(enc(y)) }, want(() => nbytesOf(R.gcd(x, y))), 'GCD ' + show(x) + ', ' + show(y));
  }
  for (let k = 0; k < 120; k++) {
    const x = k % 8 ? numfmt.decode(numfmt.encode(randomNumber(rnd, true))) : [0n, 1n, -1n, { re: 0n, im: 1n }][k % 4];
    const n = k % 13 ? BigInt(rnd(14) - 4) : k % 2 ? (1n << 20n) + BigInt(rnd(4)) : 3n;
    const big = n;
    call('POW', { flags: RESULT, room: BANK_RES_ROOM, r0: where(enc(x)), r1: where(enc(big)) }, want(() => nbytesOf(lpow(x, big))), 'POW ' + show(x) + ', ' + show(big));
  }
  call('POW', { flags: RESULT, room: 64, r0: { data: enc(2n) }, r1: { data: enc({ fix: 15n, places: 1 }) } }, { err: K.NE_INT }, 'POW 2, 1.5: NE_INT');
  call('POW', { flags: RESULT, room: 64, r0: { data: enc(2n) }, r1: { data: [0xB0, 0x03] } }, { bytes: enc(8n) }, 'POW 2, 3.0 (equal to an integer)');
  for (const op of ['TRUNCATE', 'FLOOR', 'ROUND']) {
    const ref = { TRUNCATE: R.truncate, FLOOR: R.floor, ROUND: R.round }[op];
    for (const t of ['2.5', '-2.5', '3.5', '-3.5', '1/2', '-1/2', '3/2', '7', '-7/3', '0.0', '2i'])
      call(op, { flags: RESULT, room: 64, r0: { data: enc(numfmt.parse(t)) } }, want(() => nbytesOf(ref(numfmt.parse(t)))), op + ' ' + t);
    for (let k = 0; k < 60; k++) {
      const x = k % 15 ? anyNum() : { fix: BigInt(rnd(1 << 30)) * (rnd(2) ? 1n : -1n), places: 615 + rnd(2000) };
      call(op, { flags: RESULT | (rnd(2) ? IN_BANK : 0), room: BANK_RES_ROOM, r0: where(enc(x)) }, want(() => nbytesOf(ref(x))), op + ' ' + show(x));
    }
  }
  for (let k = 0; k < 150; k++) {
    const x = anyNum(), p = k % 10 ? rnd(25) : [0, 0xFFFF, 10000, 10001, 40][(k / 10) % 5];
    const w = p > 10000 && p !== 0xFFFF ? { err: K.NE_DOMAIN } : want(() => nbytesOf(lToFixed(x, p === 0xFFFF ? 10 : p)));
    call('TO_FIXED', { flags: RESULT, room: BANK_RES_ROOM, a: p & 255, x: p >> 8, r0: where(enc(x)) }, w, 'TO_FIXED ' + show(x) + ', ' + p);
  }
  for (const [op, f] of [['TO_RATIONAL', x => lfrac(x)], ['NUMERATOR', x => R.numerator(lfrac(x))], ['DENOMINATOR', x => R.denominator(lfrac(x))]])
    for (let k = 0; k < 60; k++) {
      const x = k % 12 ? anyNum() : { fix: 1n + BigInt(rnd(1000)), places: 600 + rnd(40) };
      call(op, { flags: RESULT, room: BANK_RES_ROOM, r0: where(enc(x)) }, want(() => nbytesOf(f(x))), op + ' ' + show(x));
    }
  for (let k = 0; k < 60; k++) {
    const x = k % 6 ? real() : anyNum(), y = k % 5 ? real() : k % 2 ? 0n : anyNum();
    call('COMPLEX', { flags: RESULT, room: BANK_RES_ROOM, r0: where(enc(x)), r1: where(enc(y)) }, want(() => nbytesOf(R.complex(x, y))), 'COMPLEX ' + show(x) + ', ' + show(y));
  }
  for (let k = 0; k < 60; k++) {
    const x = anyNum(), y = k & 1;
    call('PART', { flags: RESULT, room: BANK_RES_ROOM, y, r0: where(enc(x)) }, want(() => nbytesOf(R.part(x, y))), 'PART ' + show(x) + ', ' + y);
  }
  for (let k = 0; k < 60; k++) {
    const v = [0, 1, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF, 0x12345678][k % 6] ^ (k > 6 ? rnd(1 << 30) : 0), signed = k & 1;
    const u = v >>> 0, val = signed ? BigInt(u | 0) : BigInt(u);
    call('FROM_INT', { flags: RESULT, room: 64, y: signed, r0: u & 0xFFFF, r1: u >>> 16 }, { bytes: enc(val) }, 'FROM_INT $' + u.toString(16) + (signed ? ' signed' : ''));
  }
  for (let k = 0; k < 80; k++) {
    const x = k % 10 ? (k % 3 ? ints() : [2n ** 31n - 1n, -(2n ** 31n), 2n ** 31n, 2n ** 32n - 1n, 2n ** 32n, -(2n ** 31n) - 1n][k % 6]) : anyNum();
    call('TO_INT', { r0: where(enc(x)) }, want(() => { const t = R.toInt(x); return { a: t.fits, x: 0, r4: Number(t.low & 0xFFFFn), r5: Number(t.low >> 16n) }; }), 'TO_INT ' + show(x));
  }
  const BOPS = ['and', 'or', 'xor', 'not', 'shl', 'shr', 'bit'];
  for (let k = 0; k < 280; k++) {
    const op = BOPS[k % 7], x = k % 23 ? (k % 5 ? ints() : smallInt()) : k % 2 ? (1n << 2039n) : anyNum();
    const y = op === 'shl' || op === 'shr' || op === 'bit' ? (k % 17 ? BigInt(rnd(100) - 30) : k % 2 ? 2000n : (1n << 40n)) : (k % 4 ? ints() : smallInt());
    const w = want(() => op === 'bit' ? { a: Number(R.bits('bit', x, y)), x: 0 } : nbytesOf(lbits(op, x, y)));
    call('BITS', { flags: op === 'bit' ? 0 : RESULT, room: BANK_RES_ROOM, y: k % 7, r0: where(enc(x)), r1: where(enc(y)) }, w, 'BITS ' + op + ' ' + show(x) + ', ' + show(y));
  }
  call('BITS', { flags: RESULT, room: BANK_RES_ROOM, y: 0, r0: { data: enc((1n << 2039n) + 5n) }, r1: { data: enc(3n) } }, { err: K.NE_BIG }, 'BITS and of a 255-byte integer: NE_BIG');
  call('BITS', { flags: RESULT, room: BANK_RES_ROOM, y: 4, r0: { data: enc(1n) }, r1: { data: enc(2039n) } }, { bytes: enc(1n << 2039n) }, 'BITS shl 1, 2039: 255 bytes');
  call('BITS', { flags: RESULT, room: BANK_RES_ROOM, y: 4, r0: { data: enc(1n) }, r1: { data: enc(2040n) } }, { err: K.NE_BIG }, 'BITS shl 1, 2040: NE_BIG');
  // random numbers: SEED, then RANDOM's draws, as numref.js's generator makes them
  for (const seed of [1, 0x1234, 0xBEEF]) {
    call('SEED', { r0: seed }, { ok: true }, 'SEED $' + seed.toString(16));
    const g = new R.Random(seed);
    for (let k = 0; k < 30; k++) {
      const n = k % 10 === 9 ? 0n : [6n, 100n, 256n, 1n, 1000000n, 2n ** 70n, 255n, 3n, 65535n][k % 9];
      const w = n === 0n ? nbytesOf(numfmt.norm({ fix: g.below(10n ** 12n), places: 12 })) : nbytesOf(g.below(n));
      call('RANDOM', { flags: RESULT, room: 64, r0: where(enc(n)) }, w, 'RANDOM ' + n + ' (seed $' + seed.toString(16) + ')');
    }
  }
  call('RANDOM', { flags: RESULT, room: 64, r0: { data: enc(-5n) } }, { err: K.NE_INT }, 'RANDOM -5: NE_INT');
  call('RANDOM', { flags: RESULT, room: 64, r0: { data: enc({ fix: 15n, places: 1 }) } }, { err: K.NE_INT }, 'RANDOM 1.5: NE_INT');
  for (const n of [0n, 1n, 2n, 3n, 10n, 50n, 93n, 94n, 100n, 186n, 1000n, 2000n, 2920n, 2930n, 2935n, 2938n, 2939n, 2940n, 2941n, 2950n, 3000n, 40000n, 1n << 40n, -1n])
    call('FIB', { flags: RESULT, room: BANK_RES_ROOM, r0: { data: enc(n) } }, want(() => { lfib(n); return nbytesOf(R.fib(n)); }), 'FIB ' + n);
  call('FIB', { flags: RESULT, room: 64, r0: { data: enc({ fix: 15n, places: 1 }) } }, { err: K.NE_INT }, 'FIB 1.5: NE_INT');

  // ---- Text: bases (SET_BASE), DISPLAY, PARSE, FORMAT, against numref.js
  const txt = t => [...Buffer.from(t, 'latin1')];
  const BASES = ['c', 'e', 'g', 'i', 'j', 'm', 'b', 't', 'q', 'v', 'f', 's', 'o', 'n', 'd', 'x', 'z', 'k', 'y', 'C', 'X', 'Y',
    '#c', '#x', '#b', '#m', '#k', '#y', '#d', '2r', '#16r', '#80r', '#36r', '#37r', '[01]', '#[abc]', '#=[abc]', '#=16r', '<x', '#<x',
    '>c', '#-x', '#+m', '#-d', '=7r', '#=7r', '#<-3r', '[0123456789]', '#10r', '#[-0+]', '#[Daniel]', '#<=-[0123456789]'];
  const BAD = ['', '#', 'w', 'a', '=x', '#=d', '1r', '81r', '123r', '0r', '[0]', '[00]', '[0 1]', '[a.b]', '[a/b]', 'x ', ' x', '#xx',
    '<<x', '[01', '01]', '#[abc', 'xr', '#2', '##x', '-', '[' + '0123456789'.repeat(9) + ']'];
  for (const b of [...BASES, ...BAD]) {
    let ok = true, fb = null;
    try { fb = R.NumberFormat.of(b); } catch (e) { ok = false; }
    call('SET_BASE', { r0: where(str(b)) }, ok ? { a: (fb.prefix ? 2 : 0) | (fb.isDecimal ? 1 : 0), x: 0 } : { err: K.NE_BASE },
      'SET_BASE "' + b + '"' + (ok ? ': decimal ' + fb.isDecimal + ', its prefix ' + fb.prefix : ''));
    if (ok) call('GET_BASE', { flags: RESULT, room: 100 }, { bytes: txt(b) }, 'GET_BASE "' + b + '"');
  }
  call('SET_BASE', { r0: { data: str('d') } }, { ok: true }, 'SET_BASE d');
  const disp = (x, b, how) => {
    const fm = R.NumberFormat.of(b);
    call('DISPLAY', { flags: RESULT | (rnd(2) ? IN_BANK : 0), room: BANK_RES_ROOM, r0: where(enc(x)), r4: how === 'state' ? 0 : where(str(b)) },
      want(() => ({ bytes: txt(ldisplay(x, fm)) })), 'DISPLAY ' + show(x) + ' "' + b + '"' + (how === 'state' ? ' (the base)' : ''));
  };
  for (const t of ['0', '1', '-1', '255', '-255', '0.5', '-1.25', '1/3', '-2/3', '3/4', '1+2i', '-2i', '0.5-1/3i', '1.0', '100'])
    for (const b of ['x', '#x', 'b', '#c', 'm', '#<b', '#d', '#[abc]', 'z', '#=16r', '#-x', 'e'])
      disp(numfmt.parse(t), b);
  for (let k = 0; k < 300; k++) disp(numfmt.decode(operand(!!(k % 8))), BASES[rnd(BASES.length)]);
  for (const b of ['x', '#x', 'b', '#c', 'd']) {                 // (r4 0: the base SET_BASE set)
    call('SET_BASE', { r0: { data: str(b) } }, { ok: true }, 'SET_BASE "' + b + '"');
    for (let k = 0; k < 8; k++) disp(numfmt.decode(operand()), b, 'state');
  }
  disp((1n << 2040n) - 1n, 'b');
  disp(-((1n << 2040n) - 1n), '#<-b');
  disp({ fix: 7n, places: 700 }, 'x');
  disp({ num: 1n, den: 3n ** 300n }, '#3r');
  call('DISPLAY', { flags: RESULT, room: 5, r0: { data: enc(123456789n) }, r4: { data: str('d') } }, { err: K.NE_ROOM }, 'DISPLAY 123456789, room for 5: NE_ROOM');
  call('DISPLAY', { flags: RESULT, room: 9, r0: { data: enc(123456789n) }, r4: { data: str('d') } }, { bytes: txt('123456789') }, 'DISPLAY 123456789, room for 9');
  call('DISPLAY', { flags: RESULT, room: 64, r0: { data: enc(5n) }, r4: { data: str('w') } }, { err: K.NE_BASE }, 'DISPLAY in "w": NE_BASE');
  const parse = (t, b, source, what) => {
    const fm = b === null ? R.DECIMAL : R.NumberFormat.of(b);
    call('PARSE', { flags: RESULT | (rnd(2) ? IN_BANK : 0), room: BANK_RES_ROOM, y: source ? 1 : 0, r0: where(txt(t)), r1: t.length, r4: b === null ? 0 : where(str(b)) },
      lparse(t, fm, !!source), what || 'PARSE "' + t.slice(0, 40) + '"' + (b === null ? '' : ' in "' + b + '"') + (source ? ', a program\'s' : ''));
  };
  for (let k = 0; k < 200; k++) {                               // (Read back as written: with #, then bare in the base)
    const x = numfmt.decode(operand(!!(k % 6))), b = BASES[rnd(BASES.length)];
    if ([x, x.re, x.im].some(r => r && isFix(r) && r.places > 600)) continue;  // (numref.js's 10^places: slow)
    let t;
    try { t = R.display(x, R.NumberFormat.of(b.startsWith('#') ? b : '#' + b)); } catch (e) { continue; }
    parse(t, null, k % 5 === 0);
    try { t = R.display(x, R.NumberFormat.of(b.replace(/^#/, ''))); } catch (e) { continue; }
    parse(t, b.replace(/^#/, ''), k % 7 === 0);
  }
  for (const t of ['1+', '1/x', '#xFG', '#q1', '1..2', '.5', '5.', '#c+-0', '-#x10', '1_000', '0.5-1/3i', '2i', 'i', '1+i', '#d1+#d2i', '#[0101]1',
    '#81r1', '#1r1', '1/0', '#x1.8', '#b0.1', '+5', '--5', '#=[abc]ab', '#<x01', '12+X', '12 + 3', '  42', '42  ', '1/2)', '#xFF zz', '3.14.15', '1e5', '_1_',
    '1 _', '0FF', 'FF', '-0', '+0+', '#k0A', '#Y0z', '#y0z', 'DanielStphrus', '1/-2', '-1/2', '1/2/3', '2/4', '1.5/2', '#x-1', '-#-x10', '1+2i+3i', '2.5i',
    '-0.5i', '#x10+#x2i', '1+#xAi', '1/0+2i', '#e=-0+#', '\t5\n', '7' + ' '.repeat(300), 'x'.repeat(300)])
    parse(t, null, false);
  for (const [t, b] of [['FF', 'x'], ['ff', 'x'], ['-10', 'x'], ['+-0', 'c'], ['-0+', 'c'], ['1.8', 'x'], ['0.1', 'b'], ['zz', 'z'], ['-zz', 'z'],
    ['1+2i', 'x'], ['A+Bi', 'x'], ['10', 'b'], ['#xFF', 'b'], ['1.1', 'd'], ['12', '[012]'], ['1.5', '#d']])
    for (const src of [false, true]) parse(t, b, src);
  parse('#xFF', null, false, 'PARSE "#xFF", the base at its start');
  // PARSE, .Y's bit 1: the whole text, or not a number (a reader's word, val's text)
  const whole = (t, b, source) => {
    const fm = b === null ? R.DECIMAL : R.NumberFormat.of(b);
    call('PARSE', { flags: RESULT | (rnd(2) ? IN_BANK : 0), room: BANK_RES_ROOM, y: 2 | (source ? 1 : 0), r0: where(txt(t)), r1: t.length, r4: b === null ? 0 : where(str(b)) },
      lwhole(t, fm, !!source), 'PARSE "' + t.slice(0, 40) + '", the whole text' + (b === null ? '' : ' in "' + b + '"') + (source ? ', a program\'s' : ''));
  };
  for (const t of ['1+', '1/x', '1/0x', '1/0', '12+X', '12 + 3', '  42', '42  ', '  42  ', '1/2)', '#xFF zz', '3.14.15', '1_000', '1 _', '-0', '2/4',
    '1+2i', '1+2i+3i', '#x10+#x2i', '0.5-1/3i', '1.5/2', '#d1+#d2i', '#=[abc]ab', '', ' ', '7' + ' '.repeat(248), '7' + ' '.repeat(249),
    '7' + ' '.repeat(300), '1'.repeat(255), '1'.repeat(256), 'x'.repeat(300)])
    for (const src of [false, true]) whole(t, null, src);
  for (const [t, b] of [['FF', 'x'], ['FFG', 'x'], ['1.8', 'x'], ['+-0', 'c'], ['A+Bi', 'x'], ['10 ', 'b'], ['102', 'b']])
    for (const src of [false, true]) whole(t, b, src);
  for (let k = 0; k < 60; k++) {                                // (Written, then read back whole)
    const x = numfmt.decode(operand(!!(k % 4))), b = BASES[rnd(BASES.length)];
    if ([x, x.re, x.im].some(r => r && isFix(r) && r.places > 600)) continue;
    let t;
    try { t = R.display(x, R.NumberFormat.of(b.replace(/^#/, ''))); } catch (e) { continue; }
    whole(t, b.replace(/^#/, ''), k % 3 === 0);
  }
  // FORMAT: its arguments in r1's buffer (t_num's arg1, or the bank's at $8800), the table first
  const fmt = (fstr, args, inBank) => {
    const at0 = inBank ? BANK_ARG1 : tnumLabel('arg1'), blob = [], table = [];
    let off = 3 * args.length + 1;
    const datas = args.map(a => a.str !== undefined ? txt(a.str + '\0') : enc(a.num));
    datas.forEach((d, i) => { table.push(args[i].str !== undefined ? 1 : 0, (at0 + off) & 255, (at0 + off) >> 8); off += d.length; });
    blob.push(...table, 0xFF); datas.forEach(d => blob.push(...d));
    call('FORMAT', { flags: RESULT, room: BANK_RES_ROOM, r0: where(str(fstr)), r1: { data: blob, bank: inBank }, r4: at0 },
      want(() => ({ bytes: txt(lformat(fstr, args, R.DECIMAL)) })), 'FORMAT "' + fstr + '" (' + args.length + ')');
  };
  fmt('<{}|{x}|{#b}>', [{ num: 255n }, { num: 255n }, { num: 5n }], false);
  fmt('{} and {}', [{ str: 'cats' }, { num: { num: 1n, den: 3n } }], true);
  fmt('{{}} {{{}}} }', [{ num: 7n }], false);
  fmt('a{b', [], false);
  fmt('{}{}', [{ num: 1n }], false);
  fmt('{}', [{ num: 1n }, { num: 2n }], true);
  fmt('{x}', [{ str: 'no' }], false);
  fmt('{w}', [{ num: 1n }], false);
  fmt('', [], true);
  fmt('{#=16r} {c} {[01]}', [{ num: -300n }, { num: { fix: 5n, places: 1 } }, { num: { re: 1n, im: 2n } }], true);
  for (let k = 0; k < 40; k++) {
    const n = rnd(4), args = [], parts = [];
    for (let j = 0; j < n; j++) {
      const b = rnd(3) ? BASES[rnd(BASES.length)] : '';
      if (b === '' && !rnd(3)) args.push({ str: 'str' + j }); else args.push({ num: numfmt.decode(operand()) });
      parts.push('<{' + b + '}>');
    }
    fmt(parts.join(rnd(2) ? ' ' : '{{}}'), args, !!(k & 1));
  }

  return list;
}

// num.in: the calls' records (tests/mod/t_num/t_num.s), then $FF
function record(c) {
  const b = [SLOT[c.op], c.flags, c.a | 0, c.x | 0, c.y | 0, c.room & 255, c.room >> 8];
  [c.r0, c.r1, c.r4, c.r5, c.r6].forEach((g, i) => {
    if (g === undefined || typeof g === 'number') b.push(0, (g | 0) & 255, ((g | 0) >> 8) & 255);
    else if (g.place2) b.push(3);
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
    const r4 = out.readUInt16LE(at + 4), r5 = out.readUInt16LE(at + 6), r6 = out.readUInt16LE(at + 8);
    at += 11;
    let got = null, got2 = null;
    if (!carry && (c.flags & RESULT)) { got = out.subarray(at, at + a + 256 * x); at += a + 256 * x; }
    if (!carry && (c.flags & SECOND)) { got2 = out.subarray(at, at + r6); at += r6; }
    if (kept) say(c, 'it didn\'t keep ' + ['the bank at $8000', '$78-$7F', 'r0-r3'].filter((w, i) => kept & (1 << i)).join(', '));
    const w = c.want, err = carry ? (ERR[a] || '$' + a.toString(16)) : null;
    if (w.err !== undefined) { if (!carry || a !== w.err) say(c, (carry ? err : 'no error') + ', not ' + ERR[w.err]); }
    else if (carry) say(c, err);
    else if (w.bytes && !Buffer.from(w.bytes).equals(got)) say(c, '[' + hex(got) + '], not [' + hex(w.bytes) + ']');
    else if (w.a !== undefined && (a !== w.a || x !== w.x)) say(c, '.A, .X = $' + hex([a, x]) + ', not $' + hex([w.a, w.x]));
    else if (w.bytes2 && !Buffer.from(w.bytes2).equals(got2)) say(c, 'second [' + hex(got2) + '], not [' + hex(w.bytes2) + ']');
    else if (w.used !== undefined && r5 !== w.used) say(c, r5 + ' characters, not ' + w.used);
    else if (w.r4 !== undefined && (r4 !== w.r4 || r5 !== w.r5)) say(c, 'r4, r5 = $' + r4.toString(16) + ', $' + r5.toString(16) + ', not $' + w.r4.toString(16) + ', $' + w.r5.toString(16));
  }
  if (!f.length && at !== out.length) f.push('num.out: ' + (out.length - at) + ' bytes after the last call\'s');
  return f;
}

module.exports = { calls, card, check, K, SLOT };
