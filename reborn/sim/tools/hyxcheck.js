#!/usr/bin/env node
// hyxcheck.js: hylang against danlang, expression by expression: random number expressions (the tower's arithmetic
// and order, past a fixnum and back, the conversions, the bits, numbers written in every base and read back, texts
// that may not be numbers, format's placeholders, fib, powers, the stored format, the math functions at precisions
// from 1 to 30), each printed as a line by both, the lines compared byte for byte.  danlang runs them as one
// program; hylang runs the same file in the emulator, from a card (the reborn image as built: node build.js), its
// console's output kept.
//
// Usage: node sim/tools/hyxcheck.js [-n N] [--seed S] [--keep DIR] [DANLANG]
//   -n N       N expressions (default 2500)
//   --seed S   the generator's seed (default 2100)
//   --keep DIR the program and both outputs written to DIR (x.dl, danlang.out, hylang.out)
//   DANLANG    danlang's program (default: $DANLANG, or the Release build of ../danlang): its branch with the
//              numbers (feature/numbers on)
'use strict';
const { execFileSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');
const hydrafs = require('./hydrafs.js');
const { boot } = require('../run.js');
const { image } = require('../test.js');

const argv = process.argv.slice(2);
const opt = { n: 2500, seed: 2100, keep: null, dl: null };
for (let i = 0; i < argv.length; i++) {
  if (argv[i] === '-n') opt.n = +argv[++i];
  else if (argv[i] === '--seed') opt.seed = +argv[++i];
  else if (argv[i] === '--keep') opt.keep = argv[++i];
  else opt.dl = argv[i];
}
const dl = opt.dl || process.env.DANLANG || path.join(__dirname, '..', '..', '..', '..', 'danlang', 'bin', 'Release', 'net6.0', process.platform === 'win32' ? 'danlang.exe' : 'danlang');
const dir = opt.keep || fs.mkdtempSync(path.join(os.tmpdir(), 'hyx'));
fs.mkdirSync(dir, { recursive: true });

// ---- The expressions (numxcheck.js's kinds, and hylang's own edges: a fixnum's range, its quick ways)
let seed = opt.seed;
const rnd = n => { seed = (seed * 1103515245 + 12345) >>> 0; return (seed >>> 16) % n; };
const pick = a => a[rnd(a.length)];
const digits = (max) => { let s = String(1 + rnd(9)); for (let k = rnd(max); k > 0; k--) s += rnd(10); return s; };
const sgn = () => rnd(3) ? '' : '-';
const edge = () => pick(['16383', '-16384', '16384', '-16385', '8192', '-8192', '255', '256', '65535', '65536', '32767', '-32768', '0', '1', '-1', '127', '128']);
function real() {
  switch (rnd(6)) {
    case 0: return sgn() + String(rnd(300));
    case 1: return sgn() + digits(40);
    case 2: return sgn() + (rnd(2) ? '0' : digits(6)) + '.' + (rnd(2) ? String(rnd(100)) : digits(12));
    case 3: return edge();
    default: return sgn() + digits(rnd(2) ? 3 : 20) + '/' + digits(rnd(2) ? 3 : 20);
  }
}
function num() {
  if (rnd(6)) return real();
  return '(complex ' + real() + ' ' + real() + ')';
}
const int = () => rnd(3) ? sgn() + digits(rnd(2) ? 4 : 30) : edge();
const BASES = ['c', 'e', 'g', 'i', 'j', 'm', 'b', 't', 'q', 'v', 'f', 's', 'o', 'n', 'd', 'x', 'z', 'k', 'y',
  '#c', '#x', '#b', '#m', '#k', '#y', '2r', '#16r', '#80r', '#36r', '[01]', '#[abc]', '#=[abc]', '#=16r', '<x', '#<x', '>c', '#-x', '#+m', '#-d', '=7r'];
const T = e => '(try ' + e + ' "ERR")';
const S = e => T('(to-str ' + e + ')');
const kinds = [
  [30, () => S('(' + pick(['+', '-', '*', '/']) + ' ' + num() + ' ' + num() + ')')],
  [10, () => S('(' + pick(['+', '-', '*']) + ' ' + edge() + ' ' + edge() + (rnd(2) ? ' ' + edge() : '') + ')')],
  [6, () => S('(' + pick(['cmp', '<', '>', '<=', '>=', '==', 'eq']) + ' ' + (rnd(2) ? num() : real()) + ' ' + real() + ')')],
  [12, () => {
    const a = real(), fn = pick(['truncate', 'to-rational', 'rational.n', 'rational.d', 'abs', 'to-fixed', 'neg', '1+', '1-', 'zero?', 'neg?', 'pos?', 'int?', 'fixed?', 'rational?']);
    return fn === 'to-fixed' ? S('(to-fixed ' + a + ' ' + rnd(25) + ')') : fn === 'neg' ? S('(- ' + a + ')') : S('(' + fn + ' ' + a + ')');
  }],
  [8, () => {
    const fn = pick(['bit-and', 'bit-or', 'bit-xor', 'bit-not', 'shl', 'shr', 'hex', 'bin', 'lo', 'hi']);
    const a = int();
    if (fn === 'bit-not' || fn === 'lo' || fn === 'hi') return S('(' + fn + ' ' + a + ')');
    if (fn === 'hex' || fn === 'bin') return S('(' + fn + ' ' + a + (rnd(2) ? ' ' + rnd(12) : '') + ')');
    return S('(' + fn + ' ' + a + ' ' + (fn === 'shl' || fn === 'shr' ? String(rnd(70)) : int()) + ')');
  }],
  [14, () => T('(to-str ' + num() + ' "' + pick(BASES) + '")')],
  [8, () => { const b = pick(BASES); return T('(to-str (val (to-str ' + num() + ' "' + (b.startsWith('#') ? b : '#' + b) + '")))'); }],
  [2, () => S('(val "' + pick(['1+', '1/x', '#xFG', '#q1', '1..2', '.5', '5.', '#c+-0', '-#x10', '1_000', '0.5-1/3i', '2i', 'i', '1+i', '#d1+#d2i', '#[0101]1', '#81r1', '#1r1', '1/0', '#x1.8', '#b0.1', '+5', '--5', '#=[abc]ab', '#<x01', '16384', '-16385', '0.0', '-0']) + '")')],
  [4, () => { const a = real(); return T('(format "<{' + pick(BASES) + '}|{}>" ' + a + ' ' + a + ')'); }],
  [2, () => S('(fib ' + rnd(200) + ')')],
  [3, () => S('(pow ' + real() + ' ' + rnd(12) + ')')],
  [3, () => T('(to-str (number-bytes ' + num() + '))')],
  [2, () => S('(bytes-number (number-bytes ' + num() + '))')],
  [12, () => {                                              // (the math functions, at a precision set first)
    const d = pick([1, 3, 12, 12, 12, 20, 30]), fn = pick(['sqrt', 'exp', 'log', 'sin', 'cos', 'tan', 'atan', 'pow', 'pow', 'pi']);
    const m = () => pick([String(rnd(60)), sgn() + '0.' + digits(8), digits(2) + '/' + digits(3), '-' + digits(2) + '.' + digits(4), '3.14159265358979']);
    const call = fn === 'pi' ? '(pi)' : fn === 'pow' ? '(pow ' + m() + ' ' + pick([m(), '1/2', '-1/3', '0.25', '2.5']) + ')'
      : '(' + fn + ' ' + (fn === 'exp' ? sgn() + rnd(50) + '.' + digits(3) : m()) + ')';
    return T('(do (digits ' + d + ') (to-str ' + call + '))');
  }],
];
const total = kinds.reduce((a, k) => a + k[0], 0);
const exprs = [];
for (let i = 0; i < opt.n; i++) {
  let r = rnd(total), k = 0;
  while (r >= kinds[k][0]) r -= kinds[k++][0];
  exprs.push(kinds[k][1]());
}
const prog = exprs.map(e => '(print ' + e + ')').join('\n') + '\n';
fs.writeFileSync(path.join(dir, 'x.dl'), prog);

// ---- danlang
const dlOut = execFileSync(dl, [path.join(dir, 'x.dl')], { maxBuffer: 1 << 28 }).toString().replace(/\r/g, '');
fs.writeFileSync(path.join(dir, 'danlang.out'), dlOut);

// ---- hylang, in the emulator: x.dl on a card, run at rc
const img = path.join(dir, 'card.img');
fs.rmSync(img, { force: true });
hydrafs.setNow(0x1000);
hydrafs.mkfs(img, 8, 'HYX', undefined, true);
const v = new hydrafs.Volume(img);
v.put('x.dl', Buffer.from(prog, 'latin1'));
v.close();
const base = fs.readFileSync(img), written = new Map();
const card = { dev: 0, blocks: 16384, file: img,
  read: n => written.get(n) || Buffer.concat([n * 512 < base.length ? base.subarray(n * 512, n * 512 + 512) : Buffer.alloc(0)], 512),
  write: (n, b) => written.set(n, Buffer.from(b)) };
const m = boot({ prom: image({ init: 't_rc' }), seed: 1, marks: [], log: () => {}, trace: 0, sd: [card],
  input: 'ācd /sd/0; hylang x.dl; echo hyx-end\r' });
const MAX = 40e9;
const t0 = Date.now();
while (m.cpu.cyc < MAX && !m.cpu.halted) {
  m.run(m.cpu.cyc + 5e6);
  if (/\nhyx-end\n/.test(m.out.replace(/\r/g, ''))) break;
}
let hy = m.out.replace(/\r/g, '');
const at = hy.indexOf('hylang x.dl; echo hyx-end\n');
hy = at < 0 ? hy : hy.slice(at + 'hylang x.dl; echo hyx-end\n'.length);
hy = hy.replace(/hyx-end\n[^]*$/, '');
fs.writeFileSync(path.join(dir, 'hylang.out'), hy);

const a = dlOut.split('\n'), b = hy.split('\n');
let bad = 0;
exprs.forEach((e, i) => {
  if (a[i] !== b[i]) { if (bad++ < 20) console.log('DIFF ' + e + '\n  danlang ' + a[i] + '\n  hylang  ' + b[i]); }
});
console.log(exprs.length + ' expressions: ' + bad + ' differ (' + (m.cpu.cyc / 1e6).toFixed(0) + 'M cycles, ' +
  ((Date.now() - t0) / 1000).toFixed(0) + ' s)' + (opt.keep ? '' : ''));
if (!opt.keep) fs.rmSync(dir, { recursive: true, force: true });
process.exit(bad ? 1 : 0);
