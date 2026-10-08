#!/usr/bin/env node
// numxcheck.js: numref.js (the number system's reference) against danlang, expression by expression: the tower's
// arithmetic and order, the conversions, the bits, numbers written in every base and read back, texts that may not
// be numbers, format's placeholders, fib, the math functions at precisions from 1 to 35 digits; some 12,000 cases
// from a fixed seed, run by danlang in one program.
//
// Usage: node sim/tools/numxcheck.js [DANLANG]     danlang's program (default: $DANLANG, or the Release build of
//                                                 ../danlang); its branch with the numbers (feature/numbers on)
'use strict';
const { execFileSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');
const R = require('./numref.js');
const dl = process.argv[2] || process.env.DANLANG || path.join(__dirname, '..', '..', '..', '..', 'danlang', 'bin', 'Release', 'net6.0', process.platform === 'win32' ? 'danlang.exe' : 'danlang');
const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'numx'));
let seed = 4242;
const rnd = n => { seed = (seed * 1103515245 + 12345) >>> 0; return (seed >>> 16) % n; };
const pick = a => a[rnd(a.length)];
const digits = (max) => { let s = String(1 + rnd(9)); for (let k = rnd(max); k > 0; k--) s += rnd(10); return s; };
const sgn = () => rnd(3) ? '' : '-';
function real() {
  switch (rnd(5)) {
    case 0: return sgn() + String(rnd(300));
    case 1: return sgn() + digits(40);
    case 2: return sgn() + (rnd(2) ? '0' : digits(6)) + '.' + (rnd(2) ? String(rnd(100)) : digits(12));
    default: return sgn() + digits(rnd(2) ? 3 : 20) + '/' + digits(rnd(2) ? 3 : 20);
  }
}
function num() {
  if (rnd(6)) return real();
  const re = real(), im = real();
  return '(complex ' + re + ' ' + im + ')';
}
const BASES = ['c', 'e', 'g', 'i', 'j', 'm', 'b', 't', 'q', 'v', 'f', 's', 'o', 'n', 'd', 'x', 'z', 'k', 'y',
  '#c', '#x', '#b', '#m', '#k', '#y', '2r', '#16r', '#80r', '#36r', '[01]', '#[abc]', '#=[abc]', '#=16r', '<x', '#<x', '>c', '#-x', '#+m', '#-d', '=7r'];

// The cases: each a danlang expression whose value is a string, and the JS that gives the same string
const cases = [];
const val = t => R.parse(t.replace(/^\(complex (\S+) (\S+)\)$/, (m, a, b) => '0'), R.DECIMAL) ;
function N(t) {   // a number's text (or a (complex re im) form) as a value
  const m = /^\(complex (\S+) (\S+)\)$/.exec(t);
  return m ? R.complex(R.parse(m[1]), R.parse(m[2])) : R.parse(t);
}
const S = x => R.show(x);
function js(f) { try { return f(); } catch (e) { if (e instanceof R.NumError) return 'ERR'; throw e; } }
const dlStr = e => '(try ' + e + ' "ERR")';
for (let k = 0; k < 3000; k++) {
  const a = num(), b = num();
  const op = pick(['+', '-', '*', '/']);
  cases.push([dlStr('(to-str (' + op + ' ' + a + ' ' + b + '))'), () => js(() => S({ '+': R.add, '-': R.sub, '*': R.mul, '/': R.div }[op](N(a), N(b))))]);
}
for (let k = 0; k < 600; k++) {
  const a = num(), b = num();
  cases.push([dlStr('(to-str (cmp ' + a + ' ' + b + '))'), () => js(() => String(R.cmp(N(a), N(b))))]);
}
for (let k = 0; k < 1500; k++) {
  const a = real();
  const fn = pick(['truncate', 'to-rational', 'rational.n', 'rational.d', 'abs', 'to-fixed', 'neg']);
  if (fn === 'to-fixed') { const p = rnd(25); cases.push([dlStr('(to-str (to-fixed ' + a + ' ' + p + '))'), () => js(() => S(R.toFixed(N(a), p)))]); }
  else if (fn === 'neg') cases.push([dlStr('(to-str (- ' + a + '))'), () => js(() => S(R.neg(N(a))))]);
  else cases.push([dlStr('(to-str (' + fn + ' ' + a + '))'), () => js(() => S({ truncate: R.truncate, 'to-rational': R.toRational, 'rational.n': R.numerator, 'rational.d': R.denominator, abs: R.abs }[fn](N(a))))]);
}
for (let k = 0; k < 800; k++) {
  const a = sgn() + digits(rnd(2) ? 4 : 30), b = sgn() + digits(rnd(2) ? 2 : 30);
  const fn = pick(['bit-and', 'bit-or', 'bit-xor', 'bit-not', 'shl', 'shr']);
  const s2 = fn === 'shl' || fn === 'shr' ? String(rnd(70)) : b;
  if (fn === 'bit-not') cases.push([dlStr('(to-str (bit-not ' + a + '))'), () => js(() => S(R.bits('not', N(a))))]);
  else cases.push([dlStr('(to-str (' + fn + ' ' + a + ' ' + s2 + '))'), () => js(() => S(R.bits(fn.replace('bit-', ''), N(a), N(s2))))]);
}
for (let k = 0; k < 2500; k++) {
  const a = num(), b = pick(BASES);
  cases.push([dlStr('(to-str ' + a + ' "' + b + '")'), () => js(() => R.display(N(a), R.NumberFormat.of(b)))]);
}
// reading: what danlang writes in a base, read back by both (the text made by JS, the same as danlang's above)
for (let k = 0; k < 2500; k++) {
  const a = num(), b = pick(BASES);
  let t;
  try { t = R.display(N(a), R.NumberFormat.of(b.startsWith('#') ? b : '#' + b)); } catch (e) { continue; }
  cases.push([dlStr('(to-str (val "' + t + '"))'), () => js(() => { const v = R.parse(t); return v === null ? 'ERR' : S(v); })]);
}
// reading texts that may not be numbers
const junk = ['1+', '1/x', '#xFG', '#q1', '1..2', '.5', '5.', '#c+-0', '-#x10', '1_000', '0.5-1/3i', '2i', 'i', '1+i', '#d1+#d2i', '#[0101]1', '#81r1', '#1r1', '1/0', '#x1.8', '#b0.1', '+5', '--5', '#=[abc]ab', '#<x01'];
for (const t of junk) cases.push([dlStr('(to-str (val "' + t + '"))'), () => js(() => { const v = R.parse(t); return v === null ? 'ERR' : S(v); })]);
// format's placeholders
for (let k = 0; k < 300; k++) {
  const a = real(), b = pick(BASES);
  cases.push([dlStr('(format "<{' + b + '}|{}>" ' + a + ' ' + a + ')'), () => js(() => '<' + R.display(N(a), R.NumberFormat.of(b)) + '|' + S(N(a)) + '>')]);
}
for (let n = 0; n < 120; n += 7) cases.push([dlStr('(to-str (fib ' + n + '))'), () => S(R.fib(BigInt(n)))]);
// the math functions, at a precision (its digits set first in each: (digits n), R.setDigits)
const MATH = { sqrt: x => R.sqrt(x), exp: x => R.exp(x), log: x => R.log(x), sin: x => R.trig(x, 0), cos: x => R.trig(x, 1),
  tan: x => R.trig(x, 2), atan: x => R.atan(x) };
function mreal() {
  switch (rnd(6)) {
    case 0: return sgn() + String(rnd(60));
    case 1: return sgn() + '0.' + '0'.repeat(rnd(6)) + digits(8);
    case 2: return sgn() + digits(3) + '/' + digits(3);
    case 3: return pick(['3.14159265358979', '1.5707963267949', '0.99999999', '1.00000001', '6.28318530717959', '-3.1415926535898']);
    default: return sgn() + digits(rnd(2) ? 2 : 6) + '.' + digits(6);
  }
}
const mathCase = (dl, js) => {
  const d = pick([1, 3, 12, 12, 12, 20, 35]);
  cases.push(['(do (digits ' + d + ') ' + dlStr('(to-str ' + dl + ')') + ')', () => { R.setDigits(d); try { return js(); } finally { R.setDigits(12); } }]);
};
for (let k = 0; k < 1200; k++) {
  const fn = pick([...Object.keys(MATH), 'pow', 'pow', 'pi']);
  if (fn === 'pi') { mathCase('(pi)', () => S(R.pi())); continue; }
  if (fn === 'pow') {
    const a = rnd(4) ? mreal().replace(/^-/, '') : mreal(), b = pick([mreal(), '1/2', '1/3', '2/3', '-1/2', '0.25', '1.5', String(rnd(9) - 4)]);
    mathCase('(pow ' + a + ' ' + b + ')', () => js(() => S(R.rpow(N(a), N(b)))));
    continue;
  }
  const a = fn === 'exp' ? pick([mreal(), String(rnd(400) - 200) + '.' + digits(4)]) : mreal();
  mathCase('(' + fn + ' ' + a + ')', () => js(() => S(MATH[fn](N(a)))));
}
for (const t of ['9/4', '2.25', '16', '0.09', '1.0', '1/9', '0.0001', '123456789/100']) mathCase('(sqrt ' + t + ')', () => S(R.sqrt(N(t))));

const prog = cases.map(c => '(print ' + c[0] + ')').join('\n') + '\n';
const f = path.join(dir, 'xref.dl');
fs.writeFileSync(f, prog);
const out = execFileSync(dl, [f], { maxBuffer: 1 << 28 }).toString().replace(/\r/g, '').split('\n');
let bad = 0;
cases.forEach((c, i) => {
  const want = c[1]();
  if (out[i] !== want) { if (bad++ < 15) console.log('DIFF ' + c[0] + '\n  danlang ' + out[i] + '\n  js      ' + want); }
});
console.log(cases.length + ' cases: ' + bad + ' differ');
