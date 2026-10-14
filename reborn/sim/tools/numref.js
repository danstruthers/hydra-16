#!/usr/bin/env node
// numref.js: the number system in JavaScript, danlang's (its Numbers.cs and NumberParser.cs, feature/numbers), the
// reference the numbers library is checked against (docs/design/plans/NUMBERS.md, step 2), as numfmt.js is for its
// stored format.  The numbers are numfmt.js's: an integer a BigInt, a fixed decimal { fix, places }, a rational
// { num, den }, a complex number { re, im }.  What's here:
//   the tower:     add, sub, mul, div, neg, abs, cmp, sign, kind, idiv, gcd, pow
//   conversions:   truncate, floor, round, toFixed, toRational, numerator, denominator, complex, part, fromInt, toInt
//   bits:          bits (and, or, xor, not, shl, shr, bit?)
//   others:        fib, the random generator (hylang's: a 16-bit xorshift, 7 9 8)
//   text:          NumberFormat.of(spec), display(x, format, prefix), parse(text, format, source): every base
//   math:          sqrt, exp, log, trig (sin, cos, tan), atan, pi, rpow; getDigits, setDigits (danlang's NumMath.cs)
// A failure is a NumError with its code (the library's NE_ name: BIG, DIV0, NOTNUM, DOMAIN, BASE, INT, REAL).
//
// Usage: node numref.js EXPRESSION        a small calculator for trying it: numbers and + - * / in danlang's
//                                         decimal forms, left to right (node numref.js 1/3 + 0.5)
// From Node: require('./numref.js')
'use strict';
const { norm, encode, decode, show } = require('./numfmt.js');

class NumError extends Error { constructor(code, msg) { super(msg || code); this.code = code; } }

// ---- Kinds

const isInt = x => typeof x === 'bigint';
const isFix = x => x !== null && typeof x === 'object' && 'fix' in x;
const isRat = x => x !== null && typeof x === 'object' && 'num' in x;
const isCpx = x => x !== null && typeof x === 'object' && 're' in x;
const isNum = x => isInt(x) || isFix(x) || isRat(x) || isCpx(x);
const KIND = { INT: 0, FIXED: 1, RATIONAL: 2, COMPLEX: 3 };
const kind = x => isCpx(x) ? KIND.COMPLEX : isRat(x) ? KIND.RATIONAL : isFix(x) ? KIND.FIXED : KIND.INT;
const babs = v => v < 0n ? -v : v;
const bgcd = (a, b) => { a = babs(a); b = babs(b); while (b) [a, b] = [b, a % b]; return a; };
const pow10 = n => 10n ** BigInt(n);

const isZero = x => isInt(x) ? x === 0n : isFix(x) ? x.fix === 0n : isRat(x) ? x.num === 0n : isZero(x.re) && isZero(x.im);

// A real number as a fraction { n, d } (d positive, lowest terms): danlang's Rat.ToRat
function frac(x) {
  if (isInt(x)) return { n: x, d: 1n };
  if (isFix(x)) { const g = bgcd(x.fix, pow10(x.places)) || 1n; return { n: x.fix / g, d: pow10(x.places) / g }; }
  if (isRat(x)) return { n: x.num, d: x.den };
  throw new NumError('REAL', 'A real number is needed');
}
const ratOf = (n, d) => norm({ num: n, den: d });          // (a whole one an integer: Num.Norm)
const fixOf = x => isFix(x) ? x : { fix: x, places: 0 };  // (an integer as a fixed decimal: Fix.Of)

// ---- The tower: a result is complex if either is; else rational if either is; else fixed (decimal) if either is;
// else an integer.  A quotient is exact: a rational (an integer when whole), complex or not

function cpxOf(x) { return isCpx(x) ? x : { re: x, im: 0n }; }

function add(x, y) {
  if (isInt(x) && isInt(y)) return x + y;
  if (isCpx(x) || isCpx(y)) { const a = cpxOf(x), b = cpxOf(y); return norm({ re: add(a.re, b.re), im: add(a.im, b.im) }); }
  if (isRat(x) || isRat(y)) { const a = frac(x), b = frac(y); return ratOf(a.n * b.d + b.n * a.d, a.d * b.d); }
  const f = fixOf(x), g = fixOf(y), p = Math.max(f.places, g.places);
  return norm({ fix: f.fix * pow10(p - f.places) + g.fix * pow10(p - g.places), places: p });
}

function neg(x) {
  if (isInt(x)) return -x;
  if (isFix(x)) return { fix: -x.fix, places: x.places };
  if (isRat(x)) return { num: -x.num, den: x.den };
  return { re: neg(x.re), im: neg(x.im) };
}

const sub = (x, y) => add(x, neg(y));

function mul(x, y) {
  if (isInt(x) && isInt(y)) return x * y;
  if (isCpx(x) || isCpx(y)) {
    const a = cpxOf(x), b = cpxOf(y);
    return norm({ re: sub(mul(a.re, b.re), mul(a.im, b.im)), im: add(mul(a.re, b.im), mul(a.im, b.re)) });
  }
  if (isRat(x) || isRat(y)) { const a = frac(x), b = frac(y); return ratOf(a.n * b.n, a.d * b.d); }
  const f = fixOf(x), g = fixOf(y);
  return norm({ fix: f.fix * g.fix, places: f.places + g.places });
}

function div(x, y) {
  if (isZero(y)) throw new NumError('DIV0', 'Division by zero.');
  if (isCpx(x) || isCpx(y)) {
    const a = cpxOf(x), b = cpxOf(y), d = add(mul(b.re, b.re), mul(b.im, b.im));
    return norm({ re: div(add(mul(a.re, b.re), mul(a.im, b.im)), d), im: div(sub(mul(a.im, b.re), mul(a.re, b.im)), d) });
  }
  const a = frac(x), b = frac(y);
  return ratOf(a.n * b.d, a.d * b.n);
}

// The order: by value whatever the kinds; complex numbers by real part, then imaginary part
function cmp(x, y) {
  if (isCpx(x) || isCpx(y)) { const a = cpxOf(x), b = cpxOf(y); return cmp(a.re, b.re) || cmp(a.im, b.im); }
  const a = frac(x), b = frac(y), l = a.n * b.d, r = b.n * a.d;
  return l < r ? -1 : l > r ? 1 : 0;
}
const sign = x => cmp(isCpx(x) ? x.re : x, 0n);

function abs(x) {
  if (isCpx(x)) throw new NumError('REAL', 'A real number is needed');
  return sign(x) < 0 ? neg(x) : x;
}

// ---- Integers

// An integer's value, or a number equal to one (danlang's IntOf); else INT
function intOf(x) {
  if (isInt(x)) return x;
  if (isCpx(x)) throw new NumError('INT', 'An integer is needed');
  const f = frac(x);
  if (f.d !== 1n) throw new NumError('INT', 'An integer is needed');
  return f.n;
}

// The quotient toward zero, and the remainder (its sign the dividend's)
function idiv(x, y) {
  const a = intOf(x), b = intOf(y);
  if (b === 0n) throw new NumError('DIV0', 'Division by zero.');
  return { q: a / b, r: a % b };
}
const gcd = (x, y) => bgcd(intOf(x), intOf(y));

// x to the whole power n, exactly (a negative n: 1 / x^-n)
function pow(x, n) {
  n = intOf(n);
  if (n < 0n) return div(1n, pow(x, -n));
  let r = 1n, b = x;
  for (; n > 0n; n >>= 1n) { if (n & 1n) r = mul(r, b); if (n > 1n) b = mul(b, b); }
  return r;
}

// ---- Conversions

const truncate = x => { const f = frac(x); return f.n / f.d; };   // (BigInt division truncates toward zero)
function floor(x) { const f = frac(x), q = f.n / f.d; return (f.n % f.d !== 0n && f.n < 0n) ? q - 1n : q; }
// The nearest integer, a half to the even one
function round(x) {
  const f = frac(x), q = floor(x), r2 = 2n * (f.n - q * f.d);
  return r2 > f.d || (r2 === f.d && (q & 1n)) ? q + 1n : q;
}
// x as a fixed decimal of that many places (10), cut short: danlang's Rat.ToFix (Truncate)
function toFixed(x, places = 10) {
  const f = frac(x), mult = f.n > 0n ? 1n : -1n, n = f.n * mult;
  let w = n / f.d, r = n % f.d, dec = 0;
  while (r > 0n && dec < places) { ++dec; r *= 10n; w = w * 10n + r / f.d; r %= f.d; }
  return norm({ fix: w * mult, places: dec });
}
const toRational = x => { const f = frac(x); return ratOf(f.n, f.d); };
const numerator = x => frac(x).n;
const denominator = x => frac(x).d;
function complex(re, im) {
  if (isCpx(re) || isCpx(im)) throw new NumError('REAL', 'A real number is needed');
  return norm({ re, im });
}
const part = (x, which) => which ? cpxOf(x).im : cpxOf(x).re;
const fromInt = v => BigInt(v);
// An integer as a machine integer: its low 32 bits, and 0 if it fits 32 bits signed, 1 unsigned only, 2 neither
function toInt(x) {
  const v = intOf(x), low = v & 0xFFFFFFFFn;
  return { low, fits: v >= -(2n ** 31n) && v < 2n ** 31n ? 0 : v >= 0n && v < 2n ** 32n ? 1 : 2 };
}

// ---- Bits: integers of any size, in two's complement (BigInt's own)
function bits(op, x, y) {
  const a = intOf(x), b = y === undefined ? 0n : intOf(y);
  switch (op) {
    case 'and': return a & b;
    case 'or': return a | b;
    case 'xor': return a ^ b;
    case 'not': return ~a;
    case 'shl': return a << b;
    case 'shr': return a >> b;
    case 'bit': return (a >> b) & 1n;
  }
  throw new Error('no such operation ' + op);
}

function fib(n) {
  n = intOf(n);
  if (n < 0n) throw new NumError('INT', 'n must be 0 or more');
  let a = 0n, b = 1n;
  for (let k = 0n; k < n; k++) [a, b] = [b, a + b];
  return a;
}

// ---- Random numbers: hylang's generator, a 16-bit xorshift (7 9 8), a byte at a time
class Random {
  constructor(seed = 1) { this.x = seed & 0xFFFF || 1; }
  // The next byte: hylang's rn_next, step for step (lsr x.hi; ror x.lo; eor x.hi -> x.hi; ror; eor x.lo -> x.lo;
  // eor x.hi -> x.hi)
  next() {
    let lo = this.x & 255, hi = this.x >> 8;
    let c = hi & 1;                                   // lsr rn_x+1 (the carry)
    let a = lo;                                       // lda rn_x; ror
    let c2 = a & 1; a = (a >> 1) | (c << 7); c = c2;
    a ^= hi; hi = a;                                  // eor rn_x+1; sta rn_x+1
    c2 = a & 1; a = (a >> 1) | (c << 7); c = c2;      // ror
    a ^= lo; lo = a;                                  // eor rn_x; sta rn_x
    a ^= hi; hi = a;                                  // eor rn_x+1; sta rn_x+1
    this.x = lo | (hi << 8);
    return a;
  }
  // An integer from 0 below n: as many bytes as n has, the top one masked to n's top byte's bits, drawn again while
  // it's n or more
  below(n) {
    n = intOf(n);
    if (n <= 0n) throw new NumError('INT', 'n must be 1 or more');
    const len = (n.toString(16).length + 1) >> 1, top = Number(n >> BigInt(8 * (len - 1)));
    let mask = 0xFF;                                  // (0xFF less a bit for each 0 at the top byte's top)
    for (let t = top; t && !(t & 0x80); t = (t << 1) & 0xFF) mask >>= 1;
    for (;;) {
      let v = 0n;
      const b = [];
      for (let k = 0; k < len; k++) b.push(this.next());
      b[len - 1] &= mask;
      for (let k = len - 1; k >= 0; k--) v = (v << 8n) | BigInt(b[k]);
      if (v < n) return v;
    }
  }
}

// ---- The math functions (danlang's NumMath.cs): exact when the answer is, else a fixed decimal of DIGITS significant
// digits, correctly rounded: worked in binary fixed point with a bound on its error, again with more bits till all the
// bound allows rounds the same (Ziv's way), so any right way of working it (the math library's) gives these digits

let DIGITS = 12;
const MAX_DIGITS = 100, EXP_MOST = 20000n;
const getDigits = () => DIGITS;
function setDigits(n) { if (n < 1 || n > MAX_DIGITS) throw new NumError('DOMAIN', 'digits: 1 to ' + MAX_DIGITS); DIGITS = n; }

const bitLen = v => { v = babs(v); return v === 0n ? 0 : v.toString(2).length; };
function isqrt(n) {
  if (n <= 0n) return 0n;
  let x = 1n << BigInt((bitLen(n) + 1) >> 1);
  for (;;) { const y = (x + n / x) >> 1n; if (y >= x) return x; x = y; }
}
function iroot(n, k) {                                     // (the floor of n's kth root; k a BigInt, 1 or more)
  if (n <= 0n) return 0n;
  if (k === 1n) return n;
  if (k >= BigInt(bitLen(n))) return 1n;
  const kk = Number(k);
  let x = 1n << BigInt(Math.ceil(bitLen(n) / kk));
  for (;;) { const y = (BigInt(kk - 1) * x + n / x ** BigInt(kk - 1)) / k; if (y >= x) return x; x = y; }
}
const floorDiv = (a, b) => { const q = a / b; return (a % b !== 0n && (a < 0n) !== (b < 0n)) ? q - 1n : q; };
const fixedAt = (f, b) => (f.n << BigInt(b)) / f.d;        // (a fraction { n, d } at b bits, cut toward 0)

function atanInv(m, b) {
  let t = (1n << BigInt(b)) / BigInt(m), sum = 0n;
  const m2 = BigInt(m * m);
  for (let k = 0; t !== 0n; k++) { const term = t / BigInt(2 * k + 1); sum = (k & 1) ? sum - term : sum + term; t /= m2; }
  return sum;
}
function atanhInv(m, b) {
  let t = (1n << BigInt(b)) / BigInt(m), sum = 0n;
  const m2 = BigInt(m * m);
  for (let k = 0; t !== 0n; k++) { sum += t / BigInt(2 * k + 1); t /= m2; }
  return sum;
}
const guard = b => bitLen(BigInt(b)) + 10;
const atanInvExact = (m, b) => atanInv(m, b + 64) >> 64n;   // (atan(1/m) at b bits, cut toward 0: the tables')
function piFixed(b) { const g = guard(b); return (16n * atanInv(5, b + g) - 4n * atanInv(239, b + g)) >> BigInt(g); }
function ln2Fixed(b) { const g = guard(b); return (2n * atanhInv(3, b + g)) >> BigInt(g); }
function ln10Fixed(b) { const g = guard(b); return (6n * atanhInv(3, b + g) + 2n * atanhInv(9, b + g)) >> BigInt(g); }

function roundScaled(x, b, s) {
  let num = x, den = 1n << BigInt(b);
  if (s >= 0) num *= pow10(s); else den *= pow10(-s);
  return (2n * num + den) / (2n * den);
}
function sig(x, b, k10, n) {
  let p = n - 1 - (Math.floor((bitLen(x) - 1 - b) * 0.30102999566398120) + k10);
  const low = pow10(n - 1), high = low * 10n;
  for (;;) {
    const R = roundScaled(x, b, k10 + p);
    if (R >= high) p--; else if (R < low) p++; else return [R, p];
  }
}
// The number in [(A - E) / 2^b, (A + E) / 2^b] times 10^k10, rounded to n significant digits, if all of it rounds
// the same; else null
function roundSig(A, E, b, k10, n) {
  const neg = A < 0n, a = babs(A);
  if (a - E <= 0n) return null;
  const [r1, p1] = sig(a - E, b, k10, n), [r2, p2] = sig(a + E, b, k10, n);
  if (r1 !== r2 || p1 !== p2) return null;
  const r = neg ? -r1 : r1;
  return norm(p1 >= 0 ? { fix: r, places: p1 } : { fix: r * pow10(-p1), places: 0 });
}
function ziv(f, n = DIGITS) {
  const need = Math.floor(n * 3.33) + 16;
  let b = need + 8;
  for (;;) {
    const x = f(b), r = roundSig(x.A, x.E, x.B, x.K, n);
    if (r !== null) return r;
    b += Math.max(b >> 1, need - (bitLen(x.A) - bitLen(x.E)) + 8);
  }
}

// exp z, z within Ez / 2^bz of Z / 2^bz (bz at least b + 48): 10^k exp r, r = z - k log 10
function expCore(Z, Ez, bz, b) {
  if (babs(Z) > (EXP_MOST << BigInt(bz))) throw new NumError('DOMAIN', 'out of range');
  const w = b + 40, W = BigInt(w);
  const k = floorDiv(Z, ln10Fixed(bz));
  const kb = bitLen(k) + 4;
  const R = ((Z << BigInt(kb)) - k * ln10Fixed(bz + kb)) >> BigInt(kb + bz - w);
  const s = 12, t = R >> BigInt(s), one = 1n << W;
  let sum = one, term = one, n = 0;
  for (let i = 1n; term !== 0n; i++) { term = ((term * t) >> W) / i; sum += term; n++; }
  for (let i = 0; i < s; i++) sum = (sum * sum) >> W;
  let Ew = BigInt(10 * (3 * n + 6)) << BigInt(s + 1);
  Ew += ((sum * Ez) >> BigInt(bz - 4)) + 1n;
  return { A: sum >> BigInt(w - b), E: (Ew >> BigInt(w - b)) + 2n, B: b, K: Number(k) };
}
const scale2 = (f, t) => t >= 0 ? { n: f.n << BigInt(t), d: f.d } : { n: f.n, d: f.d << BigInt(-t) };
const fcmp = (f, g) => { const c = f.n * g.d - g.n * f.d; return c < 0n ? -1 : c > 0n ? 1 : 0; };
// log x (x above 0, not 1): t log 2 + 2 atanh((f - 1) / (f + 1)), x = 2^t f, f from 3/4 to 3/2
function logCore(x, b) {
  const w = b + 24, W = BigInt(w);
  let t = bitLen(x.n) - bitLen(x.d), f = scale2(x, -t);
  if (fcmp(f, { n: 3n, d: 4n }) < 0) { t--; f = scale2(x, -t); }
  else if (fcmp(f, { n: 3n, d: 2n }) >= 0) { t++; f = scale2(x, -t); }
  const zneg = f.n < f.d;
  const Z = fixedAt({ n: babs(f.n - f.d), d: f.n + f.d }, w), Z2 = (Z * Z) >> W;
  let sum = 0n, term = Z, n = 0;
  for (let k = 0n; term !== 0n; k++) { sum += term / (2n * k + 1n); term = (term * Z2) >> W; n++; }
  sum *= zneg ? -2n : 2n;
  const tb = bitLen(BigInt(t)) + 2;
  sum += (BigInt(t) * ln2Fixed(w + tb)) >> BigInt(tb);
  return { A: sum >> BigInt(w - b), E: (BigInt(4 * n + 8) >> BigInt(w - b)) + 2n, B: b, K: 0 };
}
// sin, cos or tan (which 0, 1, 2) of x (not 0): r = x - k pi/2
function trigCore(x, which, b) {
  const w = b + 16, W = BigInt(w);
  const xb = Math.max(bitLen(x.n) - bitLen(x.d), 0);
  const p0 = piFixed(xb + 64);
  const k = floorDiv(2n * fixedAt(x, xb + 64) + (p0 >> 1n), p0);
  const kb = bitLen(k) + 4;
  const R = (fixedAt(x, w + kb) - ((k * piFixed(w + kb)) >> 1n)) >> BigInt(kb), R2 = (R * R) >> W;
  let s = R, c = 1n << W, ts = R, tc = c, n = 0;
  for (let i = 1n; ts !== 0n || tc !== 0n; i++) {
    ts = -((ts * R2) >> W) / ((2n * i) * (2n * i + 1n));
    tc = -((tc * R2) >> W) / ((2n * i - 1n) * (2n * i));
    s += ts; c += tc; n++;
  }
  const Es = BigInt(3 * n + 6), sh = BigInt(w - b);
  switch (Number(((k % 4n) + 4n) % 4n)) {
    case 1: [s, c] = [c, -s]; break;
    case 2: [s, c] = [-s, -c]; break;
    case 3: [s, c] = [-c, s]; break;
  }
  if (which === 0) return { A: s >> sh, E: (Es >> sh) + 2n, B: b, K: 0 };
  if (which === 1) return { A: c >> sh, E: (Es >> sh) + 2n, B: b, K: 0 };
  const ca = babs(c);
  if (ca <= Es) return { A: 0n, E: 1n << BigInt(b), B: b, K: 0 };
  const T = (s << W) / c;
  const Et = (((2n * Es + ((babs(T) * 2n * Es) >> W)) << W) / (ca - Es)) + 2n;
  return { A: T >> sh, E: (Et >> sh) + 2n, B: b, K: 0 };
}
// atan x (x not 0): three halvings, then the series; past 1, pi/2 - atan 1/x
function atanCore(x, b) {
  const w = b + 20, W = BigInt(w);
  const neg = x.n < 0n;
  let y = { n: babs(x.n), d: x.d };
  const inv = y.n > y.d;
  if (inv) y = { n: y.d, d: y.n };
  const one = 1n << W;
  let Y = fixedAt(y, w);
  for (let i = 0; i < 3; i++) Y = (Y << W) / (one + isqrt((one << W) + Y * Y));
  const Y2 = (Y * Y) >> W;
  let sum = 0n, term = Y, n = 0;
  for (let k = 0n; term !== 0n; k++) { const q = term / (2n * k + 1n); sum = (k & 1n) ? sum - q : sum + q; term = (term * Y2) >> W; n++; }
  sum <<= 3n;
  if (inv) sum = (piFixed(w) >> 1n) - sum;
  if (neg) sum = -sum;
  return { A: sum >> BigInt(w - b), E: (BigInt(8 * (2 * n + 8) + 4) >> BigInt(w - b)) + 2n, B: b, K: 0 };
}

function realOf(x) { if (isCpx(x)) throw new NumError('REAL', 'a real number is needed'); return frac(x); }
// v (a fraction, exact) as x's kind: an integer's an integer, a fixed decimal's a fixed decimal, a rational's a rational
function kindOf(x, v) {
  if (isFix(x)) {
    let places = 0;
    while (pow10(places) % v.d !== 0n) places++;
    return norm({ fix: v.n * (pow10(places) / v.d), places });
  }
  return ratOf(v.n, v.d);
}
function log10Floor(v) {
  const c10 = e => e >= 0 ? (v.n - v.d * pow10(e)) : (v.n * pow10(-e) - v.d);
  let e = Math.floor((bitLen(v.n) - bitLen(v.d)) * 0.30102999566398120);
  for (;;) { if (c10(e) < 0n) { e--; continue; } if (c10(e + 1) >= 0n) { e++; continue; } return e; }
}

const mpi = () => ziv(b => ({ A: piFixed(b), E: 2n, B: b, K: 0 }));
function msqrt(x) {
  if (isCpx(x)) return csqrt(x);
  const v = realOf(x);
  if (v.n < 0n) return norm({ re: 0n, im: msqrt(kindOf(x, { n: -v.n, d: v.d })) });
  if (v.n === 0n) return x;
  const ra = isqrt(v.n), rb = isqrt(v.d);
  if (ra * ra === v.n && rb * rb === v.d) return kindOf(x, { n: ra, d: rb });
  const n = DIGITS, e = log10Floor(v);
  let p = n - 1 - Math.floor(e / 2);
  for (;;) {
    let P = v.n, Q = v.d;
    if (p >= 0) P *= pow10(2 * p); else Q *= pow10(-2 * p);
    const R = (isqrt(4n * P * Q) + Q) / (2n * Q);
    if (R >= pow10(n)) { p--; continue; }
    if (R < pow10(n - 1)) { p++; continue; }
    return norm(p >= 0 ? { fix: R, places: p } : { fix: R * pow10(-p), places: 0 });
  }
}
function mexp(x) {
  if (isCpx(x)) return cexp(x);
  const v = realOf(x);
  if (v.n === 0n) return 1n;
  return ziv(b => expCore(fixedAt(v, b + 48), 1n, b + 48, b));
}
function mlog(x) {
  if (isCpx(x)) return clog(x);
  const v = realOf(x);
  if (v.n === 0n) throw new NumError('DOMAIN', 'the logarithm of 0');
  if (v.n < 0n) {
    const a = { n: -v.n, d: v.d };
    const re = a.n === a.d ? 0n : ziv(b => logCore(a, b));
    return norm({ re, im: mpi() });
  }
  if (v.n === v.d) return 0n;
  return ziv(b => logCore(v, b));
}
function mtrig(x, which) {
  if (which < 0 || which > 2) throw new NumError('DOMAIN', 'sin, cos or tan');
  if (isCpx(x)) return ctrig(x, which);
  const v = realOf(x);
  if (v.n === 0n) return which === 1 ? 1n : 0n;
  return ziv(b => trigCore(v, which, b));
}
function matan(x) {
  if (isCpx(x)) return catan(x);
  const v = realOf(x);
  if (v.n === 0n) return 0n;
  return ziv(b => atanCore(v, b));
}
function perfectRoot(a, q) {                               // (a above 0: its qth root if it's a perfect qth power, else null)
  const r = iroot(a, q);
  if (r === 1n) return a === 1n ? r : null;
  return r ** q === a ? r : null;
}
// x to the power y: a whole y exact (any x); else for x real and 0 or more, exact when its root is rational, else
// exp (y log x); a complex x or y, or a negative x, cpow's (the principal value)
function rpow(x, y) {
  if (isCpx(y)) return cpow(x, y);
  const yr = frac(y);
  if (yr.d === 1n) return pow(x, yr.n);
  if (isCpx(x)) return cpow(x, y);
  const v = frac(x);
  if (v.n < 0n) return cpow(x, y);
  if (v.n === 0n) { if (yr.n < 0n) throw new NumError('DIV0', 'Division by zero.'); return 0n; }
  const ra = perfectRoot(v.n, yr.d), rb = ra === null ? null : perfectRoot(v.d, yr.d);
  if (ra !== null && rb !== null) return pow(kindOf(x, { n: ra, d: rb }), yr.n);
  return ziv(b => {
    const yb = Math.max(bitLen(yr.n) - bitLen(yr.d), 0) + 1, bl = b + 52 + yb;
    const L = logCore(v, bl);
    const Z = (L.A * yr.n) / yr.d;
    const Ez = ((babs(L.E * yr.n) + yr.d - 1n) / yr.d) + 1n;
    return expCore(Z, Ez, bl, b);
  });
}

// ---- Complex arguments: the functions as mathematics defines them (their principal values: the argument of a number
// from above -pi to pi), each part correctly rounded as a real result is.  A part is worked in binary fixed point with
// a bound on its error (an approximation X = { A, E, B, K }: within E / 2^B of A / 2^B, times 10^K), again with more
// bits till it rounds surely (zivC).  A part that's exactly 0 is known from the argument (a modulus of 1, an angle of
// whole quarter turns, a power with a 0 part), never worked out: Ziv's way can't decide a 0 (and gives 0, past 1,024
// bits, for one too small to tell from it).

const ax = (v, b) => ({ A: fixedAt(v, b), E: 1n, B: b, K: 0 });                   // (an exact fraction)
const aOne = b => ({ A: 1n << BigInt(b), E: 0n, B: b, K: 0 });
const aPi = b => ({ A: piFixed(b), E: 2n, B: b, K: 0 });
function aK(X, K) {                                          // (X as a multiple of 10^K)
  if (X.K === K) return X;
  const d = X.K - K;
  if (d > 0) { const p = pow10(d); return { A: X.A * p, E: X.E * p, B: X.B, K }; }
  const p = pow10(-d); return { A: X.A / p, E: X.E / p + 1n, B: X.B, K };
}
function aAdd(X, Y) { const K = Math.max(X.K, Y.K); X = aK(X, K); Y = aK(Y, K); return { A: X.A + Y.A, E: X.E + Y.E, B: X.B, K }; }
const aNeg = X => ({ A: -X.A, E: X.E, B: X.B, K: X.K });
const aSub = (X, Y) => aAdd(X, aNeg(Y));
const aShr = (X, s) => ({ A: X.A >> BigInt(s), E: (X.E >> BigInt(s)) + 1n, B: X.B, K: X.K });
function aMul(X, Y) {
  const B = BigInt(X.B);
  return { A: (X.A * Y.A) >> B, E: ((babs(X.A) * Y.E + babs(Y.A) * X.E + X.E * Y.E) >> B) + 2n, B: X.B, K: X.K + Y.K };
}
function aMulQ(X, f) {                                       // (X times an exact fraction)
  return { A: (X.A * f.n) / f.d, E: (X.E * babs(f.n) + f.d - 1n) / f.d + 1n, B: X.B, K: X.K };
}
function aDiv(X, Y) {
  const B = BigInt(X.B), ya = babs(Y.A), K = X.K - Y.K;
  if (ya <= Y.E) return { A: 0n, E: 1n << BigInt(2 * X.B), B: X.B, K };            // (Y not known from 0: more bits)
  return { A: (X.A << B) / Y.A, E: (((X.E * ya + babs(X.A) * Y.E) << B) / ((ya - Y.E) * ya)) + 2n, B: X.B, K };
}
function aSqrt(X) {                                          // (X at or above 0)
  if (X.K & 1) X = aK(X, X.K - 1);
  const B = BigInt(X.B), lo = X.A - X.E, K = X.K / 2;
  if (lo <= 0n) return { A: 0n, E: isqrt((X.A + X.E) << B) + 2n, B: X.B, K };
  return { A: isqrt(X.A << B), E: ((X.E << B) / (2n * isqrt(lo << B))) + 2n, B: X.B, K };
}
const aToB = (X, b) => { const s = BigInt(X.B - b); return { A: X.A >> s, E: (X.E >> s) + 1n, B: b, K: X.K }; };
function aTrig(Z, which, b) {                                // (sin or cos of an approximation: 1 its slope at most)
  const T = Z.A === 0n ? (which ? aOne(b) : { A: 0n, E: 0n, B: b, K: 0 })
                       : trigCore({ n: Z.A, d: 1n << BigInt(Z.B) }, which, b);
  return { A: T.A, E: T.E + (Z.E >> BigInt(Z.B - b)) + 1n, B: b, K: 0 };
}
function zivC(f, n = DIGITS) {
  const need = Math.floor(n * 3.33) + 16;
  let b = need + 8;
  for (;;) {
    const x = f(b), r = roundSig(x.A, x.E, x.B, x.K, n);
    if (r !== null) return r;
    if (b > 1024 && babs(x.A) <= x.E) return 0n;
    b += Math.max(b >> 1, need - (bitLen(x.A) - bitLen(x.E)) + 8);
  }
}
const W = b => b + 24;                                       // (A part's working bits)
const fsgn = f => f.n < 0n ? -1 : f.n > 0n ? 1 : 0;
const fneg = f => ({ n: -f.n, d: f.d });
const fdiv = (f, g) => g.n < 0n ? { n: -f.n * g.d, d: f.d * -g.n } : { n: f.n * g.d, d: f.d * g.n };
const modsq = x => { const c = cpxOf(x); return add(mul(c.re, c.re), mul(c.im, c.im)); };   // (|x|^2, exactly)
const halfOf = c => (isFix(c.re) || isFix(c.im)) ? { fix: 5n, places: 1 } : { num: 1n, den: 2n };
function sqrtExact(x) {                                      // (x's square root if it's rational, else null)
  const v = frac(x);
  if (v.n < 0n) return null;
  const ra = isqrt(v.n), rb = isqrt(v.d);
  return ra * ra === v.n && rb * rb === v.d ? kindOf(x, { n: ra, d: rb }) : null;
}

// The argument (angle) of a + y i (not 0), at w bits
function aArg(a, y, w) {
  if (a.n === 0n) { const h = aShr(aPi(w), 1); return y.n < 0n ? aNeg(h) : h; }
  const t = y.n === 0n ? { A: 0n, E: 0n, B: w, K: 0 } : atanCore(fdiv(y, a), w);
  if (a.n > 0n) return t;
  return y.n < 0n ? aSub(t, aPi(w)) : aAdd(t, aPi(w));
}

// |x|: a real number's, or a complex number's modulus (the square root of re^2 + im^2)
const cabs = x => isCpx(x) ? msqrt(modsq(x)) : abs(x);

// The square root (its real part 0 or more): exact when it's rational
function csqrt(x) {
  const c = cpxOf(x), a = frac(c.re), y = frac(c.im), m2 = modsq(x), h = halfOf(c);
  const r = sqrtExact(m2);
  if (r !== null) {
    const re = msqrt(mul(add(r, c.re), h)), im = msqrt(mul(sub(r, c.re), h));
    return norm({ re, im: y.n < 0n ? neg(im) : im });
  }
  const parts = w => {                                       // (the part not cancelling first: then y over twice it)
    const R = aSqrt(ax(frac(m2), w)), P = aSqrt(aShr(aAdd(R, ax(a.n < 0n ? fneg(a) : a, w)), 1));
    const Q = aDiv(ax(y.n < 0n && a.n < 0n ? fneg(y) : y, w), aMulQ(P, { n: 2n, d: 1n }));
    if (a.n >= 0n) return [P, Q];
    return [Q, y.n < 0n ? aNeg(P) : P];
  };
  return norm({ re: zivC(b => aToB(parts(W(b))[0], b)), im: zivC(b => aToB(parts(W(b))[1], b)) });
}

// e to the x: e^a (cos y + i sin y)
function cexp(x) {
  const c = cpxOf(x), a = frac(c.re), y = frac(c.im);
  const part = k => zivC(b => {
    const w = W(b), T = trigCore(y, k ? 0 : 1, w);
    if (a.n === 0n) return aToB(T, b);
    return aToB(aMul(expCore(fixedAt(a, w + 48), 1n, w + 48, w), T), b);
  });
  return norm({ re: part(0), im: part(1) });
}

// The natural logarithm: log |x| + i arg x
function clog(x) {
  const c = cpxOf(x), a = frac(c.re), y = frac(c.im), m2 = frac(modsq(x));
  const re = m2.n === m2.d ? 0n : zivC(b => aToB(aShr(logCore(m2, W(b)), 1), b));
  return norm({ re, im: zivC(b => aToB(aArg(a, y, W(b)), b)) });
}

// sin, cos or tan of a + y i: with cosh y and sinh y (from e^y and e^-y)
function ctrig(x, which) {
  const c = cpxOf(x), a = frac(c.re), y = frac(c.im);
  const hyp = (v, w) => {
    const bz = w + 48, P = expCore(fixedAt(v, bz), 1n, bz, w), M = expCore(fixedAt(fneg(v), bz), 1n, bz, w);
    return [aShr(aAdd(P, M), 1), aShr(aSub(P, M), 1)];
  };
  const sinA = (v, w) => trigCore(v, 0, w), cosA = (v, w) => v.n === 0n ? aOne(w) : trigCore(v, 1, w);
  let f, zero;
  if (which === 2) {                                         // (tan: (sin 2a + i sinh 2y) / (cos 2a + cosh 2y))
    const a2 = { n: 2n * a.n, d: a.d }, y2 = { n: 2n * y.n, d: y.d };
    f = k => b => { const w = W(b), [C, S] = hyp(y2, w); return aToB(aDiv(k ? S : sinA(a2, w), aAdd(cosA(a2, w), C)), b); };
    zero = 0;
  } else if (which === 0) {                                  // (sin a cosh y + i cos a sinh y)
    f = k => b => { const w = W(b), [C, S] = hyp(y, w); return aToB(k ? aMul(cosA(a, w), S) : aMul(sinA(a, w), C), b); };
    zero = 0;
  } else {                                                   // (cos a cosh y - i sin a sinh y)
    f = k => b => { const w = W(b), [C, S] = hyp(y, w); return aToB(k ? aNeg(aMul(sinA(a, w), S)) : aMul(cosA(a, w), C), b); };
    zero = 1;
  }
  const part = k => (a.n === 0n && k === zero) ? 0n : zivC(f(k));
  return norm({ re: part(0), im: part(1) });
}

// The arc tangent: (1/2) arg(1 - a^2 - y^2 + 2a i) + (i/4) log((a^2 + (y + 1)^2) / (a^2 + (y - 1)^2)), and on the
// imaginary axis past i and -i, a quarter turn (atan is odd); of i and -i, NE_DOMAIN
function catan(x) {
  const c = cpxOf(x), a = frac(c.re), y = frac(c.im);
  if (a.n === 0n && babs(y.n) === y.d) throw new NumError('DOMAIN', 'the arc tangent of i or -i');
  const aa = mul(c.re, c.re);
  const N = frac(add(aa, mul(add(c.im, 1n), add(c.im, 1n)))), D = frac(add(aa, mul(sub(c.im, 1n), sub(c.im, 1n))));
  const im = zivC(b => aToB(aShr(logCore(fdiv(N, D), W(b)), 2), b));
  let re;
  if (a.n === 0n) re = babs(y.n) < y.d ? 0n : zivC(b => { const h = aShr(aPi(W(b)), 1); return aToB(y.n < 0n ? aNeg(h) : h, b); });
  else {
    const den = frac(sub(sub(1n, aa), mul(c.im, c.im))), num = { n: 2n * a.n, d: a.d };
    re = zivC(b => aToB(aShr(aArg(den, num, W(b)), 1), b));
  }
  return norm({ re, im });
}

// The square root of x if it's exact (a complex number, or a negative real number), else null
function csqrtExact(x) {
  if (!isCpx(x)) { const r = sqrtExact(neg(x)); return r === null ? null : norm({ re: 0n, im: r }); }
  const c = x, r = sqrtExact(modsq(x)), h = halfOf(c);
  if (r === null) return null;
  const re = sqrtExact(mul(add(r, c.re), h)), im = sqrtExact(mul(sub(r, c.re), h));
  if (re === null || im === null) return null;
  return norm({ re, im: frac(c.im).n < 0n ? neg(im) : im });
}

// x to the power y, the principal value: exp(y log x).  A whole y exact; y = n/2 exact when x's square root is; an
// angle that's a whole number of quarter turns exact, the parts with it
function cpow(x, y) {
  const w_ = cpxOf(y), wr = frac(w_.re), wi = frac(w_.im);
  if (isZero(x)) {
    if (fsgn(wr) > 0) return 0n;
    throw isCpx(y) ? new NumError('DOMAIN', '0 to a power whose real part is 0 or less') : new NumError('DIV0', 'Division by zero.');
  }
  if (!isCpx(y) && wr.d === 2n) { const s = csqrtExact(x); if (s !== null) return pow(s, wr.n); }
  const c = cpxOf(x), a = frac(c.re), v = frac(c.im), m2 = frac(modsq(x));
  const lre0 = m2.n === m2.d;                                // (log |x| is 0)
  let q = null;                                              // (arg x, when it's q pi)
  if (v.n === 0n) q = a.n > 0n ? { n: 0n, d: 1n } : { n: 1n, d: 1n };
  else if (a.n === 0n) q = { n: v.n > 0n ? 1n : -1n, d: 2n };
  else if (a.n * v.d === v.n * a.d || a.n * v.d === -v.n * a.d) q = { n: (a.n > 0n ? 1n : 3n) * (v.n > 0n ? 1n : -1n), d: 4n };
  const arg0 = q !== null && q.n === 0n;
  const t1 = wr.n !== 0n && !lre0, t2 = wi.n !== 0n && !arg0;            // (Tre = wr log|x| - wi arg x)
  const t3 = wr.n !== 0n && !arg0, t4 = wi.n !== 0n && !lre0;            // (Tim = wr arg x + wi log|x|)
  let p = null;                                              // (Tim = p pi, when it's that: arg x q pi, or 0)
  if (!t4) { if (!t3) p = { n: 0n, d: 1n }; else if (q !== null) p = { n: wr.n * q.n, d: wr.d * q.d }; }
  const bits = f => Math.max(bitLen(f.n) - bitLen(f.d), 0);
  const bzOf = w => w + 48 + bits(wr) + bits(wi);
  const Lre = bz => aShr(logCore(m2, bz), 1);
  const Lim = bz => q !== null ? aMulQ(aPi(bz), q) : aArg(a, v, bz);
  const Tre = bz => {
    let T = { A: 0n, E: 0n, B: bz, K: 0 };
    if (t1) T = aAdd(T, aMulQ(Lre(bz), wr));
    if (t2) T = aSub(T, aMulQ(Lim(bz), wi));
    return T;
  };
  const Tim = bz => {
    if (p !== null) return aMulQ(aPi(bz), p);
    let T = { A: 0n, E: 0n, B: bz, K: 0 };
    if (t3) T = aAdd(T, aMulQ(Lim(bz), wr));
    if (t4) T = aAdd(T, aMulQ(Lre(bz), wi));
    return T;
  };
  const mag0 = !t1 && !t2;                                   // (|x^y| is 1)
  const mag = w => { if (mag0) return aOne(w); const bz = bzOf(w), Z = Tre(bz); return expCore(Z.A, Z.E, bz, w); };
  let cs = null;                                             // (cos and sin of Tim, when they're 0 and 1s)
  if (p !== null) {
    const r = { n: ((p.n % (2n * p.d)) + 2n * p.d) % (2n * p.d), d: p.d };  // (p mod 2)
    if ((2n * r.n) % r.d === 0n) cs = [[1n, 0n], [0n, 1n], [-1n, 0n], [0n, -1n]][Number((2n * r.n) / r.d)];
  }
  const part = k => {
    if (cs !== null) {
      if (cs[k] === 0n) return 0n;
      if (mag0) return cs[k];
      const m = zivC(b => aToB(mag(W(b)), b));
      return cs[k] < 0n ? neg(m) : m;
    }
    return zivC(b => { const w = W(b), bz = bzOf(w); return aToB(aMul(mag(w), aTrig(Tim(bz), k ? 0 : 1, w)), b); });
  };
  return norm({ re: part(0), im: part(1) });
}

// ---- Text: bases (danlang's NumberParser)

const STD = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-=+`~!@#$%^&*,;:|?';
const INVALID = '<>[]() \t\r\n./_\\\'"';
// The named bases: their digits, least digit first, balanced, negative
const BASES = {
  c: ['-0+', true, true, false], e: ['=-0+#', true, true, false], g: ['~=-0+#*', true, true, false],
  i: ['UON', true, true, false], j: ['WUONM', true, true, false], m: ['DanielStphrus', true, true, true],
  b: ['01'], t: ['012'], q: ['0123'], v: ['01234'], f: ['012345'], s: ['0123456'], o: ['01234567'],
  n: ['012345678'], d: ['0123456789'], x: ['0123456789ABCDEF'], z: [STD.slice(0, 36)],
  k: ['ZYXWVUTSRQPON0ABCDEFGHIJKLM', false, true, false], y: ['zyxwvutsrqponmlkjihgfedcba0ABCDEFGHIJKLMNOPQRSTUVWXYZ', false, true, false],
};

const isUniqueSet = s => !!s && new Set(s).size === s.length && [...s].every(c => !INVALID.includes(c) && c.charCodeAt(0) > 32);
const isCaseUnique = s => isUniqueSet(s.toUpperCase());

// A digit set's parser (danlang's NumberParser): its digits, base (negative for a negative base), least first,
// balanced; it reads a value a character at a time
class Digits {
  constructor(neg, le, bal, chars) {
    this.caseSig = !isCaseUnique(chars);
    this.chars = this.caseSig ? chars : chars.toUpperCase();
    this.base = BigInt(chars.length) * (neg ? -1n : 1n);
    this.le = le; this.bal = bal;
    this.zero = bal ? Math.floor(chars.length / 2) : 0;
  }
  up(c) { return this.caseSig ? c : c.toUpperCase(); }
  isDigit(c) { return this.chars.indexOf(this.up(c)) >= 0; }
  val(c) { return BigInt(this.chars.indexOf(this.up(c)) - this.zero); }
  charFor(v) { return this.chars[Number(v) + this.zero]; }
  get minDigit() { return BigInt(-this.zero); }
  get maxDigit() { return BigInt(this.chars.length - 1 - this.zero); }   // (balanced, of an even size: one less above 0)
  // A value's text read (AddString): its digits, '_' passed over, one '.'; how many characters it took
  read(s) {
    this.v = 0n; this.den = null;
    let used = 0;
    for (const c of this.le ? [...s].reverse() : [...s]) {
      if (c === '_' || c === '.') {
        if (c === '.') { if (this.den !== null) break; this.den = 1n; }
        ++used; continue;
      }
      if (!this.isDigit(c)) break;
      if (this.den !== null) this.den *= this.base;
      this.v = this.v * this.base + this.val(c);
      ++used;
    }
    return used;
  }
  // The number read: an integer; a fraction in base 10 a fixed decimal, in any other a rational
  get num() {
    if (this.den === null || this.den === 1n) return this.v;
    if (this.base === 10n) {
      const p = this.den.toString().length - 1;
      return norm({ fix: this.den > 0n ? this.v : -this.v, places: p });
    }
    return ratOf(this.v, this.den);
  }
}

// A base, as a base string names it (danlang's NumberFormat)
class NumberFormat {
  static of(spec) {
    const m = /^(#?)([<>]?)(=?)([+-]?)(?:([0-9]{1,2})[rR]|\[([^\]]{2,80})\]|([A-Za-z]))$/.exec(spec || '');
    if (!m) throw new NumError('BASE', 'Invalid base specifier ' + spec);
    const f = new NumberFormat();
    f.spec = spec; f.prefix = m[1] === '#'; f.prefixText = '#' + spec.replace(/^#/, '');
    const [, , dir, bal, sgn, radix, chars, letter] = m;
    if (letter) {
      const b = BASES[letter.toLowerCase()];
      if (!b) throw new NumError('BASE', 'Invalid base specifier ' + spec);
      if (bal) throw new NumError('BASE', 'Invalid base specifier ' + spec + ': a named base is balanced or not already');
      f.chars = b[0]; f.bal = !!b[2];
      f.le = dir === '' ? !!b[1] : dir === '<';
      f.neg = sgn === '' ? !!b[3] : sgn === '-';
    }
    else {
      if (radix !== undefined) {
        const r = +radix;
        if (r < 2 || r > 80) throw new NumError('BASE', 'Invalid base specifier ' + spec + ': a radix is 2 to 80');
        f.chars = STD.slice(0, r);
      }
      else {
        f.chars = chars;
        if (!isUniqueSet(chars)) throw new NumError('BASE', 'Invalid base specifier ' + spec + ': each digit once');
      }
      f.bal = bal === '='; f.le = dir === '<'; f.neg = sgn === '-';
    }
    f.isDecimal = f.chars === '0123456789' && !f.bal && !f.le && !f.neg;
    return f;
  }
  digits() { return new Digits(this.neg, this.le, this.bal, this.chars); }
  hasDigit(c) { return this.digits().isDigit(c); }
}
const DECIMAL = NumberFormat.of('d');

// ---- Writing

function display(x, f = DECIMAL, prefix = null) {
  const pre = prefix === null ? f.prefix : prefix;
  if (f.isDecimal && !pre) return show(x);
  if (isCpx(x)) {
    let g = f, p = pre;
    if (f.hasDigit('+') || f.hasDigit('-') || f.hasDigit('i')) { g = DECIMAL; p = true; }
    const im = displayReal(x.im, g, p);
    if (isZero(x.re)) return im + 'i';
    return displayReal(x.re, g, p) + (im.startsWith('-') ? '' : '+') + im + 'i';
  }
  return displayReal(x, f, pre);
}

function displayReal(x, f, pre) {
  const p = pre ? f.prefixText : '';
  if (f.isDecimal) {
    if (isRat(x)) return displayReal(x.num, f, true) + '/' + displayReal(x.den, f, true);
    const s = show(x);
    return s.startsWith('-') ? '-' + p + s.slice(1) : p + s;
  }
  const q = frac(x);
  if (q.d === 1n) return digitsOf(q.n, f, p, 0);
  const k = placesIn(q.d, f);
  if (k < 0) return digitsOf(q.n, f, p, 0) + '/' + digitsOf(q.d, f, p, 0);
  return digitsOf(q.n * f.digits().base ** BigInt(k) / q.d, f, p, k);
}

// The places a fraction with this denominator ends in, in a base (the least k for which den divides base^k), or -1
function placesIn(den, f) {
  const b = babs(f.digits().base);
  let k = 0;
  while (den !== 1n) { const g = bgcd(den, b); if (g === 1n) return -1; den /= g; ++k; }
  return k;
}

function digitsOf(n, f, p, k) {
  const acc = f.digits();
  const neg = acc.base > 0n && !acc.bal && n < 0n;
  let be = bigEndianDigits(neg ? -n : n, acc);
  if (k > 0) {
    if (be.length <= k) be = acc.charFor(0n).repeat(k + 1 - be.length) + be;
    be = be.slice(0, be.length - k) + '.' + be.slice(be.length - k);
  }
  if (acc.le) be = [...be].reverse().join('');
  return (neg ? '-' : '') + p + be;
}

// An integer's digits, most significant first (danlang's ToBase loop; BigInteger's / and % truncate, as BigInt's)
function bigEndianDigits(scratch, acc) {
  let s = '', p = 1n, min = acc.minDigit, max = acc.maxDigit;
  while (scratch > max || scratch < min) {
    p = p * acc.base;
    min = min + p * (p > 0n ? acc.minDigit : acc.maxDigit);
    max = max + p * (p > 0n ? acc.maxDigit : acc.minDigit);
  }
  while (p !== 0n) {
    min = min - p * (p > 0n ? acc.minDigit : acc.maxDigit);
    max = max - p * (p > 0n ? acc.maxDigit : acc.minDigit);
    let d = scratch / p;
    scratch = scratch % p;
    const m = scratch > max ? (scratch > p ? -1n : 1n) : scratch < min ? (scratch < p ? -1n : 1n) : 0n;
    d = d + m;
    scratch = scratch - m * p;
    p = p / acc.base;
    s += acc.charFor(d);
  }
  return s;
}

// ---- Reading

const CUSTOM = /^#([<>]?)(=?)([+-]?)(?:([1-7]?[0-9]|80)[rR]|\[([0-9a-zA-Z`~!@#$%^&*\-=+|;:,?]{2,80})\])([0-9a-zA-Z`~!@#$%^&*\-=+|;:,?]+(?:\.[0-9a-zA-Z`~!@#$%^&*\-=+|;:,?]+)?)$/;
const esc = s => s.replace(/[-\\\]^]/g, '\\$&');
const NAMED = Object.entries(BASES).map(([c, b]) => [c, b, new RegExp('^#([<>])?([+-])?[' + c + c.toUpperCase() + '](' +
  '[' + esc(b[0]) + ']+(?:\\.[' + esc(b[0]) + ']+)?)$', isCaseUnique(b[0]) ? 'i' : '')]);

// A number from text in a base (a # number in its own); source: a program's text, where a bare number starts with a
// digit 0-9.  null if it isn't a number
function parse(s, f = DECIMAL, source = false) {
  s = String(s).trim().replace(/_/g, '');
  if (!s) return null;
  const real = parseReal(s, f, source);
  if (real !== null || !s.endsWith('i')) return real;
  const body = s.slice(0, -1);
  if (!body) return null;
  const im = parseReal(body, f, source);
  if (im !== null) return norm({ re: 0n, im });
  for (let p = body.length - 1; p > 0; --p) {
    if (body[p] !== '+' && body[p] !== '-') continue;
    const re = parseReal(body.slice(0, p), f, source);
    if (re === null) continue;
    const ip = parseReal(body.slice(p), f, false);
    if (ip !== null) return norm({ re, im: ip });
  }
  return null;
}

function parseReal(s, f, source) {
  if (s.includes('/')) {
    const sp = s.split('/');
    if (sp.length !== 2 || sp.some(x => !x || x.includes('.'))) return null;
    const n = parseReal(sp[0], f, source), d = parseReal(sp[1], f, false);
    if (n === null || d === null || !isInt(n) && !(isFix(n) && n.places === 0) || !isInt(d) && !(isFix(d) && d.places === 0)) return null;
    const nv = isInt(n) ? n : n.fix, dv = isInt(d) ? d : d.fix;
    if (dv === 0n) throw new NumError('DIV0', 'Division by zero');
    return ratOf(nv, dv);
  }
  let sgn = '+';
  const bare = !s.replace(/^[+-]+/, '').startsWith('#');
  if ('+-'.includes(s[0]) && !(bare && f.hasDigit(s[0]))) { sgn = s[0]; s = s.slice(1); }
  if (!s) return null;
  let v;
  if (s[0] === '#') v = parseBased(s);
  else {
    if (source && !(s[0] >= '0' && s[0] <= '9')) return null;
    if (f.isDecimal) {
      const m = /^([0-9]+)(?:\.([0-9]+))?$/.exec(s);
      if (!m) return null;
      v = m[2] !== undefined ? norm({ fix: BigInt(m[1] + m[2]), places: m[2].length }) : BigInt(m[1]);
    }
    else {
      const dot = s.indexOf('.');
      if (dot === 0 || dot === s.length - 1) return null;
      const d = f.digits();
      if (d.read(s) !== s.length) return null;
      v = d.num;
    }
  }
  if (v === null) return null;
  return sgn === '-' ? neg(v) : v;
}

function parseBased(s) {
  let m = CUSTOM.exec(s);
  if (m) {
    const [, dir, bal, sgn, radix, chars, value] = m;
    let set;
    if (radix !== undefined) { if (+radix < 2) return null; set = STD.slice(0, +radix); }
    else { if (!isUniqueSet(chars)) return null; set = chars; }
    const d = new Digits(sgn === '-', dir === '<', bal === '=', set);
    if (d.read(value) !== value.length) return null;
    return d.num;
  }
  for (const [, b, re] of NAMED) {
    if (!(m = re.exec(s))) continue;
    const [, dir, sgn, value] = m;
    const le = dir ? dir === '<' : !!b[1];
    let negb = !!b[3];
    if (sgn === '-') negb = true;
    if (sgn === '+') negb = false;
    const d = new Digits(negb, le, !!b[2], b[0]);
    if (d.read(value) !== value.length) return null;
    return d.num;
  }
  return null;
}

// ---- The calculator (trying it)
if (require.main === module) {
  const t = process.argv.slice(2);
  if (!t.length) { console.error('usage: node numref.js NUMBER [OP NUMBER] ...'); process.exit(2); }
  let x = parse(t[0]);
  for (let k = 1; k + 1 < t.length; k += 2) {
    const y = parse(t[k + 1]);
    x = { '+': add, '-': sub, '*': mul, '/': div }[t[k]](x, y);
  }
  console.log(show(x) + '\t' + encode(x).map(v => v.toString(16).toUpperCase().padStart(2, '0')).join(' '));
}

module.exports = {
  NumError, KIND, kind, isNum, isZero, norm, encode, decode, show,
  add, sub, mul, div, neg, abs, cmp, sign, idiv, gcd, pow,
  truncate, floor, round, toFixed, toRational, numerator, denominator, complex, part, fromInt, toInt,
  bits, fib, Random, NumberFormat, DECIMAL, display, parse,
  MAX_DIGITS, getDigits, setDigits, sqrt: msqrt, cabs, exp: mexp, log: mlog, trig: mtrig, atan: matan, pi: mpi, rpow,
  piFixed, ln2Fixed, ln10Fixed, atanInvExact,
};
