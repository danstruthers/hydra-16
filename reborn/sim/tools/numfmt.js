#!/usr/bin/env node
// numfmt.js: the Hydra's stored format of a number (docs/design/plans/NUMBERS.md, "The format numbers are stored
// in"), in JavaScript: the reference the numbers library and its tests are checked against.  A number is a tag byte,
// then what the tag says; integers' bytes least first:
//   $00-$7F            an integer, -64 to 63: the tag itself (7-bit two's complement)
//   $80-$8F, $90-$9F   a positive or negative integer of n bytes, n - 1 the tag's low 4 bits (1 to 16): its magnitude
//   $A0, $A1           a positive or negative integer of 17 to 255 bytes: a length byte, then its magnitude
//   $B0-$BF            a fixed decimal of 0 to 15 places (the tag's low 4 bits): its digits, an integer
//   $C0                a fixed decimal of 16 to 65535 places: the places (2 bytes), then its digits, an integer
//   $C1                a rational: its numerator, then its denominator (above 1), integers in lowest terms
//   $C2                a complex number: its real part, then its imaginary part (not 0), real numbers
//   $FF                never a number: free for a program's own use
// Each number has one form, its shortest (a fixed decimal's digits don't end in 0, but 0.0's): decode takes no other.
// The numbers are danlang's (and hylang's): integers of any size, fixed decimals (digits / 10^places, the kind kept
// even with no places: 1.0), rationals, complex numbers.  Here an integer is a BigInt, and the rest objects:
//   { fix: digits, places }   { num, den }   { re, im }
//
// Usage: node numfmt.js NUMBER ...            each number (danlang's decimal forms: 42, -1.5, 2/3, 1+2i, 2i) and its
//                                             bytes, in hexadecimal
//        node numfmt.js --decode HEX ...      the number those bytes are (B2 81 3A 01, or B2813A01)
//        node numfmt.js --test                its own checks
// From Node: { encode, decode, parse, show, norm } = require('./numfmt.js')
'use strict';

// ---- The numbers

const isInt = x => typeof x === 'bigint';
const isFix = x => x !== null && typeof x === 'object' && 'fix' in x;
const isRat = x => x !== null && typeof x === 'object' && 'num' in x;
const isCpx = x => x !== null && typeof x === 'object' && 're' in x;
const abs = v => v < 0n ? -v : v;
const gcd = (a, b) => { a = abs(a); b = abs(b); while (b) [a, b] = [b, a % b]; return a; };

// A number made its one form, as danlang keeps it: a fixed decimal without 0s at its digits' end; a rational in
// lowest terms, its denominator positive (a whole one an integer); a complex number with an imaginary part (else real)
function norm(x) {
  if (isFix(x)) {
    let { fix: d, places: p } = x;
    while (p > 0 && d % 10n === 0n) { d /= 10n; --p; }
    return { fix: d, places: p };
  }
  if (isRat(x)) {
    let { num: n, den: d } = x;
    if (d === 0n) throw new RangeError('Division by zero');
    if (d < 0n) { n = -n; d = -d; }
    const g = gcd(n, d) || 1n;
    n /= g; d /= g;
    return d === 1n ? n : { num: n, den: d };
  }
  if (isCpx(x)) {
    const re = norm(x.re), im = norm(x.im);
    return isZero(im) ? re : { re, im };
  }
  return x;
}
const isZero = x => isInt(x) ? x === 0n : isFix(x) ? x.fix === 0n : isRat(x) ? x.num === 0n : false;

// ---- The format

function encode(x) {
  const out = [];
  x = norm(x);
  if (isCpx(x)) { out.push(0xC2); putReal(out, x.re); putReal(out, x.im); }
  else putReal(out, x);
  return out;
}

function putReal(out, x) {
  if (isRat(x)) { out.push(0xC1); putInt(out, x.num); putInt(out, x.den); }
  else if (isFix(x)) {
    if (x.places < 16) out.push(0xB0 + x.places);
    else {
      if (x.places > 65535) throw new RangeError('Too many places for the stored format');
      out.push(0xC0, x.places & 255, x.places >> 8);
    }
    putInt(out, x.fix);
  }
  else putInt(out, x);
}

function putInt(out, v) {
  if (v >= -64n && v <= 63n) { out.push(Number(v) & 0x7F); return; }
  const neg = v < 0n, m = [];
  for (let a = abs(v); a > 0n; a >>= 8n) m.push(Number(a & 255n));
  if (m.length > 255) throw new RangeError('Too big for the stored format');
  if (m.length <= 16) out.push((neg ? 0x90 : 0x80) + m.length - 1);
  else out.push(neg ? 0xA1 : 0xA0, m.length);
  out.push(...m);
}

// The number the bytes are, all of them, in its one form; an error (a FormatError) otherwise
class FormatError extends Error {}
function decode(bytes) {
  const b = Array.from(bytes), at = { i: 0 };
  let x;
  if (b[0] === 0xC2) {
    at.i = 1;
    const re = getReal(b, at), im = getReal(b, at);
    if (isZero(im)) throw new FormatError("a complex number's imaginary part is 0");
    x = { re, im };
  }
  else x = getReal(b, at);
  if (at.i !== b.length) throw new FormatError((b.length - at.i) + ' bytes after the number');
  return x;
}

function byte(b, at) {
  if (at.i >= b.length) throw new FormatError('the bytes end in the middle of a number');
  return b[at.i++];
}

function getReal(b, at) {
  const t = byte(b, at);
  if (t < 0xB0) { --at.i; return getInt(b, at); }
  if (t <= 0xBF || t === 0xC0) {
    let places = t - 0xB0;
    if (t === 0xC0) {
      places = byte(b, at) + 256 * byte(b, at);
      if (places < 16) throw new FormatError('a fixed decimal of fewer than 16 places has a tag of its own');
    }
    const digits = getInt(b, at);
    if (places > 0 && digits % 10n === 0n) throw new FormatError("a fixed decimal's digits end in 0");
    return { fix: digits, places };
  }
  if (t === 0xC1) {
    const num = getInt(b, at), den = getInt(b, at);
    if (den <= 1n) throw new FormatError("a rational's denominator is above 1");
    if (num === 0n || gcd(num, den) !== 1n) throw new FormatError("a rational isn't in its lowest terms");
    return { num, den };
  }
  throw new FormatError('$' + hex2(t) + " isn't a real number's tag");
}

function getInt(b, at) {
  const t = byte(b, at);
  if (t < 0x80) return BigInt(t & 0x40 ? t - 128 : t);
  let neg, len;
  if (t <= 0x9F) { neg = t >= 0x90; len = (t & 0x0F) + 1; }
  else if (t === 0xA0 || t === 0xA1) {
    neg = t === 0xA1;
    len = byte(b, at);
    if (len < 17) throw new FormatError('an integer of fewer than 17 bytes has a tag of its own');
  }
  else throw new FormatError('$' + hex2(t) + " isn't an integer's tag");
  let v = 0n;
  const m = [];
  for (let k = 0; k < len; k++) m.push(byte(b, at));
  if (m[len - 1] === 0) throw new FormatError("an integer's magnitude ends in a 0 byte");
  for (let k = len - 1; k >= 0; k--) v = (v << 8n) | BigInt(m[k]);
  if (neg) v = -v;
  if (v >= -64n && v <= 63n) throw new FormatError('an integer of -64 to 63 is its tag alone');
  return v;
}

const hex2 = v => v.toString(16).toUpperCase().padStart(2, '0');

// ---- Text: danlang's decimal forms (the libraries read every base; this is for the tests)

// A real number's text: an integer, a fixed decimal (digits after the point), or a rational of two integers
function parseReal(s) {
  let m;
  if ((m = /^([+-]?)([0-9]+)$/.exec(s))) return BigInt(m[1] + m[2]);
  if ((m = /^([+-]?)([0-9]+)\.([0-9]+)$/.exec(s))) return norm({ fix: BigInt(m[1] + m[2] + m[3]), places: m[3].length });
  if ((m = /^([+-]?[0-9]+)\/([+-]?[0-9]+)$/.exec(s))) return norm({ num: BigInt(m[1]), den: BigInt(m[2]) });
  return null;
}

// A number's text, danlang's way: a real number, or a complex one (1+2i, 0.5-1/3i, 2i); null if it isn't one
function parse(s) {
  s = String(s).trim().replace(/_/g, '');
  const r = parseReal(s);
  if (r !== null || !s.endsWith('i')) return r;
  const body = s.slice(0, -1);
  const im = parseReal(body);
  if (im !== null) return norm({ re: 0n, im });
  for (let p = body.length - 1; p > 0; --p) {
    if (body[p] !== '+' && body[p] !== '-') continue;
    const re = parseReal(body.slice(0, p)), ip = parseReal(body.slice(p));
    if (re !== null && ip !== null) return norm({ re, im: ip });
  }
  return null;
}

// A number as danlang prints it (in decimal)
function show(x) {
  if (isInt(x)) return x.toString();
  if (isFix(x)) {
    if (x.places === 0) return x.fix.toString();
    const neg = x.fix < 0n, d = abs(x.fix).toString().padStart(x.places + 1, '0');
    return (neg ? '-' : '') + d.slice(0, -x.places) + '.' + d.slice(-x.places);
  }
  if (isRat(x)) return x.num + '/' + x.den;
  if (isCpx(x)) {
    const im = show(x.im) + 'i';
    if (isZero(x.re)) return im;
    return show(x.re) + (im.startsWith('-') ? '' : '+') + im;
  }
  throw new TypeError('not a number');
}

// ---- Its checks

function selfTest() {
  const want = [
    ['0', '00'], ['63', '3F'], ['-64', '40'], ['-1', '7F'], ['64', '80 40'], ['100', '80 64'], ['-100', '90 64'],
    ['65535', '81 FF FF'], ['2147483648', '83 00 00 00 80'], ['0.5', 'B1 05'], ['-2.5', 'B1 67'],
    ['3.14', 'B2 81 3A 01'], ['1.0', 'B0 01'], ['1/3', 'C1 01 03'], ['2/3', 'C1 02 03'], ['1+2i', 'C2 01 02'],
    ['1i', 'C2 00 01'], ['3.14159265359', 'BB 84 4F F6 59 25 49'],
  ];
  let fails = 0;
  const say = m => { console.log('FAIL ' + m); ++fails; };
  for (const [t, h] of want) {
    const got = encode(parse(t)).map(hex2).join(' ');
    if (got !== h) say(t + ': ' + got + ', not ' + h);
    const back = show(decode(got.split(' ').map(v => parseInt(v, 16))));
    if (back !== show(parse(t))) say(h + ' decoded: ' + back);
  }
  // 10^20, a long integer (255 bytes), past it, 16 places and more
  if (encode(10n ** 20n).length !== 10) say('10^20');
  if (encode(2n ** 2032n).length !== 257) say('2^2032');
  try { encode(2n ** 2040n); say('2^2040 encoded'); } catch (e) { if (!(e instanceof RangeError)) throw e; }
  const p20 = { fix: 123456789n, places: 20 };
  if (show(decode(encode(p20))) !== show(norm(p20))) say('20 places');
  // what decode refuses: each number has one form
  for (const bad of [[0x80, 5], [0x81, 5, 0], [0xC1, 2, 4], [0xB1, 10], [1, 2], [0xC2, 1, 0], [0xC0, 5, 0, 1], [0xA0, 3, 1, 2, 3], [0xC3], [0xFF], [0xB1]]) {
    try { decode(bad); say('decoded ' + bad.map(hex2).join(' ')); } catch (e) { if (!(e instanceof FormatError)) throw e; }
  }
  // random numbers of every kind, encoded and decoded back
  let seed = 12345;
  const rnd = n => { seed = (seed * 1103515245 + 12345) >>> 0; return seed % n; };
  const rint = () => { let v = BigInt(rnd(1000)) - 500n; for (let k = rnd(30); k > 0; k--) v = v * 977n + BigInt(rnd(977)); return v; };
  for (let k = 0; k < 3000; k++) {
    const kind = rnd(4);
    let x = kind === 0 ? rint() : kind === 1 ? norm({ fix: rint(), places: rnd(40) }) :
      kind === 2 ? norm({ num: rint(), den: rint() || 1n }) : norm({ re: rint(), im: norm({ num: rint(), den: 7n }) });
    const back = decode(encode(x));
    if (show(back) !== show(x)) say('round trip ' + show(x) + ': ' + show(back));
  }
  console.log(fails ? fails + ' failed' : 'numfmt: all checks passed');
  return fails === 0;
}

if (require.main === module) {
  const a = process.argv.slice(2);
  if (a[0] === '--test') process.exit(selfTest() ? 0 : 1);
  else if (a[0] === '--decode') {
    const b = a.slice(1).join('').replace(/[^0-9A-Fa-f]/g, '').match(/../g).map(v => parseInt(v, 16));
    console.log(show(decode(b)));
  }
  else if (a.length) for (const t of a) {
    const x = parse(t);
    if (x === null) { console.error(t + ": isn't a number"); process.exitCode = 1; continue; }
    console.log(show(x) + '\t' + encode(x).map(hex2).join(' '));
  }
  else { console.error('usage: node numfmt.js NUMBER ... | --decode HEX ... | --test'); process.exit(2); }
}

module.exports = { encode, decode, parse, show, norm, FormatError };
