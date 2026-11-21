#!/usr/bin/env node
// ****************************************************************************
// fnumxcheck.js - HyForth's number words (numbers.fl) against another build of them, case by case: random operands of
// every size and kind (integers of both signs, 0, the edges of 7, 8, 16 and 32 bits, integers of 5 to 8 bytes and
// big ones, fixed decimals, fractions, complex numbers) through the words numbers.fl does itself for small integers
// (n+ n- n* n/ n/mod n/trunc, the comparisons, n0= n0<, nnegate nabs, truncate nfloor nround, s>n d>n, n@ n! and
// nvalue's TO), each result's bytes in the stored format (nbytes) and every THROW; the errors (the stack empty, a
// division by 0, digits wrong) and the edges of the stack's room (64 numbers, its 8K: each case with 0 to 12 bytes
// left after its operands).  The two builds run the same program, each from a card with its numbers.fl, and their
// outputs are compared line by line; and each result of the arithmetic is checked against numref.js too.  The base
// build's n/trunc is n/ truncate (a word it hasn't).
//
// Usage: node sim/tools/fnumxcheck.js [--base FL] [--fl FL] [-n N] [--seed S] [--keep DIR]
//   --base FL  the other build's numbers.fl (none: numref.js's check alone)
//   --fl FL    this build's (default: obj/forthlib/numbers.fl)
//   -n N       the random cases (default 2000); --seed S their seed (default 1)
//   --keep DIR the program and the outputs written to DIR
// As a module: gen(seed, n) gives back { program(trunc), expect } (expect: each line's text by numref.js, or null),
// run(program, fl) the output's lines.
'use strict';
const fs = require('fs');
const path = require('path');
const os = require('os');
const R = require('./numref.js');
const TREE = path.join(__dirname, '..', '..');

function gen(seed = 1, n = 2000) {
  let st = seed >>> 0 || 1;
  const rnd = k => { st = (st * 1103515245 + 12345) >>> 0; return (st >>> 8) % k; };
  const pick = a => a[rnd(a.length)];
  const big = bytes => { let v = 0n; for (let k = 0; k < bytes; k++) v = v << 8n | BigInt(k === 0 ? 1 + rnd(255) : rnd(256)); return v; };
  const sg = v => rnd(2) ? -v : v;
  const EDGES = [0n, 1n, 63n, 64n, 65n, 127n, 128n, 255n, 256n, 16383n, 16384n, 32767n, 32768n, 65535n, 65536n,
    (1n << 24n) - 1n, 1n << 24n, (1n << 31n) - 1n, 1n << 31n, (1n << 32n) - 1n, 1n << 32n, (1n << 32n) + 1n, 1n << 40n];
  const digs = k => { let s = String(1 + rnd(9)); while (--k > 0) s += rnd(10); return s; };
  function int() {
    switch (rnd(8)) {
      case 0: case 1: return sg(BigInt(rnd(141))) ;
      case 2: case 3: return sg(pick(EDGES));
      case 4: case 5: return sg(big(1 + rnd(4)));
      case 6: return sg(big(5 + rnd(4)));
      default: return sg(big(9 + rnd(30)));
    }
  }
  function num() {
    switch (rnd(10)) {
      case 0: return (rnd(2) ? '-' : '') + (rnd(3) ? String(rnd(1000)) : digs(15)) + '.' + digs(1 + rnd(8));
      case 1: return (rnd(2) ? '-' : '') + (rnd(3) ? String(1 + rnd(99)) : digs(20)) + '/' + (rnd(3) ? String(2 + rnd(98)) : digs(20));
      case 2: if (rnd(2)) { const im = String(int() || 1n); return String(int()) + (im[0] === '-' ? '' : '+') + im + 'i'; }
      // falls through
      default: return String(int());
    }
  }
  const norm = t => { const v = R.parse(t); return v; };
  const B = v => R.encode(v).join(' ') + ' ;';
  const P = t => 'S" ' + t + '" >N DROP ';
  const lines = [], expect = [];
  const add = (src, exp) => { lines.push(src); expect.push(exp === undefined ? null : exp); };
  const tryv = f => { try { return f(); } catch (e) { if (e instanceof R.NumError) return e; throw e; } };
  const res = f => { const r = tryv(f); return r instanceof R.NumError ? null : r; };   // (An error: the base build's to judge)
  const bin = { 'N+': R.add, 'N-': R.sub, 'N*': R.mul, 'N/': R.div };
  const cmpw = { 'N=': c => c === 0, 'N<': c => c < 0, 'N>': c => c > 0 };
  const flag = b => (b ? -1 : 0) + ' |';
  // The cases: two numbers and a word, or one
  for (let k = 0; k < n; k++) {
    const a = num(), b = rnd(4) ? num() : String(int());
    const x = norm(a), y = norm(b), cpx = R.kind(x) === R.KIND.COMPLEX || R.kind(y) === R.KIND.COMPLEX;
    const w = pick(['N+', 'N-', 'N*', 'N/', 'N/MOD', 'NT', 'N=', 'N<', 'N>', 'NCOMPARE', 'NNEGATE', 'NABS', 'TRUNCATE',
      'NFLOOR', 'NROUND', 'N0=', 'N0<', 'N+', 'N*', 'NT', 'N/']);
    const one = ['NNEGATE', 'NABS', 'TRUNCATE', 'NFLOOR', 'NROUND', 'N0=', 'N0<'].includes(w);
    const src = P(a) + (one ? '' : P(b)) + "' " + w + ' R';
    let e = null;
    if (bin[w]) { const r = res(() => bin[w](x, y)); e = typeof r === 'string' || r === null ? r : '|' + B(r); }
    else if (w === 'NT') { const r = res(() => { const q = R.div(x, y); return R.truncate(q); }); e = typeof r === 'string' || r === null ? r : '|' + B(r); }
    else if (w === 'N/MOD') { const r = res(() => R.idiv(x, y)); e = typeof r === 'string' || r === null ? r : '|' + B(r.q) + B(r.r); }
    else if (cpx && /^N(=|<|>|COMPARE|0=|0<)$/.test(w)) e = null;
    else if (cmpw[w]) { const r = res(() => R.cmp(x, y)); e = typeof r === 'string' || r === null ? r : flag(cmpw[w](r)); }
    else if (w === 'NCOMPARE') { const r = res(() => R.cmp(x, y)); e = typeof r === 'string' || r === null ? r : r + ' |'; }
    else if (w === 'NNEGATE') e = '|' + B(R.neg(x));
    else if (w === 'N0=') { const r = res(() => R.cmp(x, 0n)); e = typeof r === 'string' || r === null ? r : flag(r === 0); }
    else if (w === 'N0<') { const r = res(() => R.cmp(x, 0n)); e = typeof r === 'string' || r === null ? r : flag(r < 0); }
    else if (w === 'TRUNCATE' || w === 'NFLOOR' || w === 'NROUND') { const r = res(() => ({ TRUNCATE: R.truncate, NFLOOR: R.floor, NROUND: R.round })[w](x)); e = typeof r === 'string' || r === null ? r : '|' + B(r); }
    // (NABS: the math library's, a complex number's modulus: not numref's here)
    else if (w === 'NABS' && typeof x === 'bigint') e = '|' + B(x < 0n ? -x : x);
    add(src, e);
  }
  // Cells and doubles
  const cells = [0, 1, -1, 63, 64, -64, -65, 127, 128, 255, 256, 16383, 16384, -16383, -16384, 32767, -32768, 65535, -32767];
  for (let k = 0; k < 120; k++) {
    const c = k < cells.length ? cells[k] : rnd(65536) - 32768;
    add(c + " ' S>N R", '|' + B(BigInt.asIntN(16, BigInt(c))));
    const d = k < cells.length ? BigInt(cells[k]) * (k & 1 ? 65536n : 1n) : BigInt(rnd(65536)) << 16n | BigInt(rnd(65536));
    const dv = BigInt.asIntN(32, d), lo = Number(BigInt.asUintN(16, dv)), hi = Number(BigInt.asIntN(16, dv >> 16n));
    add(lo + ' ' + hi + " ' D>N R", '|' + B(dv));
  }
  for (const d of [0x7FFFFFFFn, -0x80000000n, -1n, 0xFFFFn, -0x10000n]) {
    const lo = Number(BigInt.asUintN(16, d)), hi = Number(BigInt.asIntN(16, d >> 16n));
    add(lo + ' ' + hi + " ' D>N R", '|' + B(d));
  }
  // Variables and values: stores of every size into four of each, each read back
  for (let k = 0; k < 300; k++) {
    const t = rnd(5) ? String(rnd(3) ? sg(BigInt(rnd(20000))) : int()) : num(), v = rnd(4);
    if (rnd(2)) {
      add(P(t) + 'V' + v + " ' N! R", '|');
      add('V' + v + " ' N@ R", '|' + B(norm(t)));
    } else {
      add(P(t) + "' TW" + v + ' R', '|');
      add("' W" + v + ' R', '|' + B(norm(t)));
    }
  }
  for (let v = 0; v < 4; v++) add('V' + v + " ' N@ R", null);
  // The errors: the stack empty or one short; a division by 0; digits wrong
  for (const w of ['N+', 'N-', 'N*', 'N/', 'N/MOD', 'NT', 'N=', 'N<', 'N>', 'NCOMPARE', 'NNEGATE', 'NABS', 'TRUNCATE',
    'NFLOOR', 'NROUND', 'N0=', 'N0<', 'N!', 'N@']) {
    const va = w === 'N!' || w === 'N@' ? 'V0 ' : '';   // (No address left to show: the dictionary's differ)
    add('NEMPTY ' + va + "' " + w + ' R', null);
    add('NEMPTY ' + P('5') + va + "' " + w + ' R', null);
    add('NEMPTY ' + P('100000000000000000000') + va + "' " + w + ' R', null);
  }
  add('NEMPTY .( -) CR', null);
  for (const a of ['0', '7', '-7', '4294967295', '100000000000000000000', '1/3', '1.5', '1+2i'])
    for (const w of ['N/', 'N/MOD', 'NT']) add(P(a) + P('0') + "' " + w + ' R', null);
  for (const a of ['5', '-5', '1/3']) add('0 DIGITS ! ' + P(a) + "' NABS R 12 DIGITS !", null);
  // The stack's 64 numbers: each word with 63 or 64 there
  for (const w of ['N+', 'N*', 'NT', 'N/MOD', 'NNEGATE', 'TRUNCATE', 'N0=', 'N0<', 'N=', 'S>N', 'N@', 'NDUP']) {
    for (const d of [62, 63, 64]) {
      const ops = ['NNEGATE', 'TRUNCATE', 'N0=', 'N0<', 'NDUP'].includes(w) ? 1 : ['S>N', 'N@'].includes(w) ? 0 : 2;
      add('NEMPTY ' + (d - ops) + ' NFILL ' + (ops ? P('-3') : '') + (ops > 1 ? P('5') : '') +
        (w === 'S>N' ? '9 ' : w === 'N@' ? 'V1 ' : '') + "' " + w + ' R NEMPTY', null);
    }
  }
  // The stack's 8K: a filler of 8,132 bytes (41 numbers), then each case's operands after a number that leaves 0 to
  // 12 bytes after them
  const len = v => R.encode(v).length;
  add('FILL8K .( -) CR', null);
  const adj = L => L + ' ADJ ';
  const small = () => String(pick([0n, 1n, -1n, 63n, 64n, -64n, -65n, 255n, 256n, 65535n, -65536n, (1n << 32n) - 1n, -((1n << 32n) - 1n), 1n << 31n, 12345678n]));
  // (Not n/trunc: a word of its own, its room its own; n/ truncate's is n/'s fraction's)
  for (const w of ['N+', 'N-', 'N*', 'N/', 'N/MOD', 'NNEGATE', 'NABS', 'TRUNCATE', 'N0=', 'N<', 'S>N', 'N@', 'D>N']) {
    for (let r = 0; r <= 12; r++) {
      const ops = ['NNEGATE', 'NABS', 'TRUNCATE', 'N0='].includes(w) ? 1 : ['S>N', 'N@', 'D>N'].includes(w) ? 0 : 2;
      const a = small(), b = w === 'N/' || w === 'N/MOD' || w === 'NT' ? pick(['1', '-1', '3', '7', '256', '-65536']) : small();
      const used = (ops ? len(norm(a)) : 0) + (ops > 1 ? len(norm(b)) : 0);
      const L = 60 - used - r;
      add(adj(L) + (ops ? P(a) : '') + (ops > 1 ? P(b) : '') + (w === 'S>N' ? '-300 ' : w === 'N@' ? 'V2 ' : w === 'D>N' ? '0 1 ' : '') +
        "' " + w + ' R NCUT', null);
    }
  }
  add('0 NMARK ! NEMPTY .( -) CR', null);
  const program = trunc => [
    'DECIMAL',
    ': NEMPTY ( -- ) BEGIN NDEPTH WHILE NDROP REPEAT ;',
    'VARIABLE NMARK  0 NMARK !',
    ': NCUT ( -- ) BEGIN NDEPTH NMARK @ > WHILE NDROP REPEAT ;',
    ': .B ( -- ) ( N: x -- ) NBYTES OVER + SWAP ?DO I C@ . LOOP ;',
    ': R ( i*x xt -- ) CATCH ?DUP IF ." E" . DEPTH 0 ?DO DROP LOOP ELSE DEPTH 0 ?DO . LOOP THEN ." |"',
    '  BEGIN NDEPTH NMARK @ > WHILE .B ." ;" REPEAT CR ;',
    ': NFILL ( n -- ) 0 ?DO I 7 MOD S>N LOOP ;',
    ': ADJ ( u -- ) ( N: -- x ) >R PAD R@ ERASE 160 PAD C! R@ 2 - PAD 1+ C! 1 PAD R@ + 1- C! PAD R> NFROM-BYTES ;',
    ': FILL8K ( -- ) NEMPTY 40 0 DO 1 S>N 1600 NLSHIFT LOOP 1 S>N 80 NLSHIFT NDEPTH NMARK ! ;',
    trunc ? ': NT ( N: x y -- q ) N/TRUNC ;' : ': NT ( N: x y -- q ) N/ TRUNCATE ;',
    'NVARIABLE V0  NVARIABLE V1  NVARIABLE V2  NVARIABLE V3',
    '0 S>N NVALUE W0  1/2 NVALUE W1  70000 NVALUE W2  -5 S>N NVALUE W3',
    ': TW0 TO W0 ;  : TW1 TO W1 ;  : TW2 TO W2 ;  : TW3 TO W3 ;',
    'CR .( ==start) CR',
    ...lines,
    '.( ==end) CR BYE', ''].join('\n');
  return { program, expect, lines };
}

// The program run by HyForth (forth /sd/0/t.fs), numbers.fl from the card: its output's lines from ==start to ==end
function run(program, fl, tree = TREE) {
  const hydrafs = require(path.join(tree, 'sim', 'tools', 'hydrafs.js'));
  const { boot } = require(path.join(tree, 'sim', 'run.js'));
  const img = path.join(os.tmpdir(), 'fnumx-' + process.pid + '-' + Date.now() + '.img');
  hydrafs.setNow(0x1000);
  hydrafs.mkfs(img, 8, 'FNX', undefined, true);
  const v = new hydrafs.Volume(img);
  v.put('t.fs', Buffer.from('S" /sd/0/nb.fl" INCLUDED\n' + program, 'latin1'));
  v.put('nb.fl', fs.readFileSync(fl));
  v.close();
  const base = fs.readFileSync(img), written = new Map();
  fs.rmSync(img, { force: true });
  const card = { dev: 0, blocks: 16384, file: img,
    read: k => written.get(k) || Buffer.concat([k * 512 < base.length ? base.subarray(k * 512, k * 512 + 512) : Buffer.alloc(0)], 512),
    write: (k, b) => written.set(k, Buffer.from(b)) };
  const m = boot({ seed: 1, sd: [card], input: '\u0101forth /sd/0/t.fs\r\u0101echo %%END%%\r' });
  const t0 = Date.now();
  while (Date.now() - t0 < 1800e3) {
    m.run(m.cpu.cyc + 5e6);
    if (/^%%END%%$/m.test(m.out.slice(-300).replace(/\r/g, ''))) break;
  }
  const out = m.out.replace(/\r/g, '');
  const a = out.indexOf('==start\n'), b = out.indexOf('==end');
  if (a < 0 || b < 0) return { lines: null, out: out.slice(-3000), cycles: m.cpu.cyc };
  return { lines: out.slice(a + 8, b).split('\n').filter(l => l !== ''), out, cycles: m.cpu.cyc };
}

module.exports = { gen, run };

if (require.main === module) {
  const args = process.argv.slice(2);
  const opt = (k, d) => { const i = args.indexOf(k); if (i < 0) return d; const v = args[i + 1]; args.splice(i, 2); return v; };
  const N = +opt('-n', 2000), seed = +opt('--seed', 1), keep = opt('--keep', ''), baseFl = opt('--base', '');
  const fl = opt('--fl', path.join(TREE, 'obj', 'forthlib', 'numbers.fl'));
  const g = gen(seed, N);
  const mine = run(g.program(true), fl);
  if (!mine.lines) { console.log('no output:\n' + mine.out); process.exit(1); }
  if (keep) { fs.mkdirSync(keep, { recursive: true }); fs.writeFileSync(path.join(keep, 't.fs'), g.program(true)); fs.writeFileSync(path.join(keep, 'new.out'), mine.lines.join('\n') + '\n'); }
  let bad = 0;
  const show = (k, what) => { if (++bad <= 30) console.log('case ' + k + ': ' + g.lines[k] + '\n  ' + what); };
  if (mine.lines.length !== g.lines.length) console.log('lines: ' + mine.lines.length + ', cases ' + g.lines.length);
  for (let k = 0; k < g.lines.length; k++) if (g.expect[k] !== null && mine.lines[k] !== g.expect[k]) show(k, 'got ' + mine.lines[k] + '\n  numref ' + g.expect[k]);
  console.log(g.lines.length + ' cases, ' + g.expect.filter(e => e !== null).length + ' checked against numref.js: ' + bad + ' different (' + (mine.cycles / 1e6).toFixed(0) + 'M cycles)');
  let bad2 = 0;
  if (baseFl) {
    const theirs = run(g.program(false), baseFl);
    if (!theirs.lines) { console.log('base: no output:\n' + theirs.out); process.exit(1); }
    if (keep) fs.writeFileSync(path.join(keep, 'base.out'), theirs.lines.join('\n') + '\n');
    for (let k = 0; k < Math.max(g.lines.length, theirs.lines.length); k++)
      if (mine.lines[k] !== theirs.lines[k] && ++bad2 <= 30) console.log('case ' + k + ': ' + g.lines[k] + '\n  this ' + mine.lines[k] + '\n  base ' + theirs.lines[k]);
    console.log('against the base build: ' + bad2 + ' lines different (' + (theirs.cycles / 1e6).toFixed(0) + 'M cycles)');
  }
  process.exit(bad || bad2 ? 1 : 0);
}
