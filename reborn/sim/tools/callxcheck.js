#!/usr/bin/env node
// ****************************************************************************
// callxcheck.js - BASIC's and hylang's + - * / and order over random operands of every size and kind (integers of
// 0 to 100 bytes and their edges (7, 8, 16, 31, 32, 63, 64 bits), fixed decimals of 1-21 places, fractions, complex
// numbers), run in this build and another and compared line by line: BASIC's each result's bytes (MKN$) or its
// error, from literals read by VAL (so VT_INT, VT_FIX and VT_NUM operands, in the RAM and the heap's banks); hylang's
// each result printed.  The numbers library's ARITH and its quick operands (NUMSPEED.md, "A cheaper call") are
// judged by it; HyForth's words by fnumxcheck.js.
//
// Usage: node sim/tools/callxcheck.js --base TREE [-n N] [--seed S] [--keep DIR] [--only bas|hl]
//   --base TREE  the other build (its reborn folder, built)
//   -n N         the pairs of operands (default 180: BASIC's program must fit its memory); --seed S (default 1)
//   --keep DIR   the programs and the outputs written to DIR; --only: one language
'use strict';
const fs = require('fs'), path = require('path'), os = require('os');
const argv = process.argv.slice(2);
const opt = (n, d) => { const i = argv.indexOf(n); return i < 0 ? d : argv[i + 1]; };
const A = path.join(__dirname, '..', '..'), B = path.resolve(opt('--base', '') || '.');
const N = +opt('-n', 180), SEED = +opt('--seed', 1), KEEP = opt('--keep', null), ONLY = opt('--only', null);
let st = SEED >>> 0 || 1;
const rnd = k => { st = (st * 1103515245 + 12345) >>> 0; return (st >>> 8) % k; };
const pick = a => a[rnd(a.length)];
const big = bytes => { let v = 0n; for (let k = 0; k < bytes; k++) v = v << 8n | BigInt(k === 0 ? 1 + rnd(255) : rnd(256)); return v; };
const sg = v => rnd(2) ? -v : v;
const P = k => 1n << BigInt(k);
const EDGES = [0n, 1n, 63n, 64n, 65n, 127n, 128n, 255n, 256n, 32767n, 32768n, 65535n, 65536n, P(31) - 1n, P(31), P(32) - 1n,
  P(32), P(32) + 1n, P(40), P(56) - 1n, P(56), P(63) - 1n, P(63), P(64) - 1n, P(64), P(64) + 1n, P(72), P(120)];
function int() {
  switch (rnd(10)) {
    case 0: return sg(BigInt(rnd(141)));
    case 1: case 2: return sg(pick(EDGES));
    case 3: case 4: case 5: return sg(big(1 + rnd(8)));
    case 6: case 7: return sg(big(5 + rnd(6)));
    case 8: return sg(big(9 + rnd(24)));
    default: return sg(big(rnd(4) ? 40 + rnd(40) : 95 + rnd(5)));
  }
}
function fixed() {          // a fixed decimal: its digits an integer, its places
  const d = rnd(3) ? sg(big(1 + rnd(9))) : int();
  const p = rnd(8) ? 1 + rnd(15) : 16 + rnd(6);
  let s = (d < 0n ? -d : d).toString().padStart(p + 1, '0');
  return (d < 0n ? '-' : '') + s.slice(0, s.length - p) + '.' + s.slice(s.length - p);
}
function frac() {
  let n = rnd(3) ? sg(big(1 + rnd(3))) : sg(big(1 + rnd(40))), d = rnd(3) ? big(1 + rnd(3)) : big(1 + rnd(12));
  if (n === 0n) n = 1n;
  if (d === 0n || d === 1n) d = 7n;
  return n.toString() + '/' + d.toString();
}
function cplx() { const re = rnd(2) ? sg(big(1 + rnd(12))).toString() : fixed(); const im = big(1 + rnd(3)); return re + (rnd(2) ? '+' : '-') + im + 'i'; }
function val() { const k = rnd(20); return k < 8 ? int().toString() : k < 13 ? fixed() : k < 18 ? frac() : cplx(); }
const pairs = [];
for (let i = 0; i < N; i++) pairs.push([val(), val()]);

// BASIC: DATA lines; each result's bytes (MKN$) or its error
function basProg() {
  let s = 'DIM SHARED e$\nFUNCTION hx$ (x)\n m$ = MKN$(x): h$ = ""\n FOR k = 1 TO LEN(m$): h$ = h$ + RIGHT$("0" + HEX$(ASC(MID$(m$, k, 1))), 2): NEXT\n hx$ = h$\nEND FUNCTION\n';
  s += 'ON ERROR GOTO oops\nDO\n READ a$: IF a$ = "end" THEN EXIT DO\n READ b$\n a = VAL(a$): b = VAL(b$)\n';
  s += ' r$ = "E": r$ = hx$(a + b): PRINT r$;" ";\n r$ = "E": r$ = hx$(a - b): PRINT r$;" ";\n r$ = "E": r$ = hx$(a * b): PRINT r$;" ";\n';
  s += ' r$ = "E": r$ = hx$(a / b): PRINT r$;" ";\n r$ = "E": x = 0: IF a < b THEN x = 1\n IF a = b THEN x = x + 2\n IF a > b THEN x = x + 4\n IF a <> b THEN x = x + 8\n PRINT x\n';
  s += ' c = a: c = c + b: c = c * a: PRINT hx$(c)\nLOOP\nPRINT "done"\nEND\noops:\n PRINT "E"; ERR;: RESUME NEXT\n';
  for (const [a, b] of pairs) s += 'DATA "' + a + '"\nDATA "' + b + '"\n';
  return s + 'DATA "end"\n';
}
// hylang: each result printed
function hlLit(v) { return v.replace(/^(-?[0-9.]+)([+-])([0-9]+)i$/, '$1$2$3i'); }
function hlProg() {
  let s = '';
  for (const [a, b] of pairs) {
    const x = hlLit(a), y = hlLit(b);
    const zero = /^-?0(\.0*)?$/.test(b);
    s += `(print (+ ${x} ${y}) (- ${x} ${y}) (* ${x} ${y}) ${zero ? '"z"' : `(/ ${x} ${y})`} (cmp ${x} ${y}))\n`;
  }
  return s + '(print "done")\n';
}
function run(tree, file, text, cmd) {
  const hydrafs = require(path.join(tree, 'sim', 'tools', 'hydrafs.js'));
  const { boot } = require(path.join(tree, 'sim', 'run.js'));
  const { image } = require(path.join(tree, 'sim', 'test.js'));
  const dir = fs.mkdtempSync(path.join(KEEP || os.tmpdir(), 'xc'));
  const img = path.join(dir, 'card.img');
  hydrafs.setNow(0x1000);
  hydrafs.mkfs(img, 8, 'XC', undefined, true);
  const v = new hydrafs.Volume(img); v.put(file, Buffer.from(text, 'latin1')); v.close();
  const base = fs.readFileSync(img), written = new Map();
  const card = { dev: 0, blocks: 16384, file: img,
    read: n => written.get(n) || Buffer.concat([n * 512 < base.length ? base.subarray(n * 512, n * 512 + 512) : Buffer.alloc(0)], 512),
    write: (n, b) => written.set(n, Buffer.from(b)) };
  const m = boot({ prom: image({ init: 't_rc' }), seed: 1, marks: [], log: () => {}, trace: 0, sd: [card],
    input: '\u0101cd /sd/0; ' + cmd + ' ' + file + '; echo xc-end\r' });
  while (m.cpu.cyc < 80e9 && !m.cpu.halted) { m.run(m.cpu.cyc + 5e6); if (/\nxc-end\n/.test(m.out.replace(/\r/g, ''))) break; }
  fs.rmSync(dir, { recursive: true, force: true });
  return { out: m.out.replace(/\r/g, ''), cyc: m.cpu.cyc };
}
let bad = 0;
for (const [name, file, cmd, gen] of [['bas', 'x.bas', 'basic', basProg], ['hl', 'x.hl', 'hylang', hlProg]]) {
  if (ONLY && ONLY !== name) continue;
  const prog = gen();
  const ra = run(A, file, prog, cmd), rb = run(B, file, prog, cmd);
  if (KEEP) { fs.writeFileSync(path.join(KEEP, name + '.prog'), prog); fs.writeFileSync(path.join(KEEP, name + '-a.out'), ra.out); fs.writeFileSync(path.join(KEEP, name + '-b.out'), rb.out); }
  const la = ra.out.split('\n'), lb = rb.out.split('\n');
  let d = 0;
  for (let i = 0; i < Math.max(la.length, lb.length); i++) if (la[i] !== lb[i]) { if (d++ < 8) console.log(name + ' line ' + i + '\n  A ' + la[i] + '\n  B ' + lb[i]); }
  const done = /\ndone\n/.test(ra.out) && /\ndone\n/.test(rb.out);
  console.log(`${name}: ${pairs.length} pairs, ${la.length} lines, ${d} different${done ? '' : ' (NOT FINISHED)'}; cycles A ${(ra.cyc / 1e6).toFixed(0)}M B ${(rb.cyc / 1e6).toFixed(0)}M`);
  bad += d + (done ? 0 : 1);
}
process.exit(bad ? 1 : 0);
