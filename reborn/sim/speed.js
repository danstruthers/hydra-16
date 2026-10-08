#!/usr/bin/env node
// ****************************************************************************
// speed.js - the numbers' speed (docs/design/plans/NUMSPEED.md): tests/speed's programs run in the emulator, each
// stretch they mark timed (sim/tools/nprof.js), one table: BASIC's number work (ms a stretch), its single operations
// (cycles a pass of a loop of 200), the math functions (cycles a call), hylang's number work (ms) and single
// operations (cycles a pass).  Every stretch's time is the machine's, all tasks' (the program's and the system's).
//
// Usage: node sim/speed.js [--build TREE] [--vs TREE | --base FILE] [--prev FILE] [--save FILE] [--only NAME,...]
//   --build TREE   that tree's build (its reborn folder), not this one's
//   --vs TREE      another tree's build beside it (the baseline), and the ratio of the two
//   --base FILE    a table saved before (--save) as the baseline
//   --prev FILE    another table saved before (the step before), its ratio too
//   --save FILE    this table saved (JSON)
//   --only         those programs alone (numb, small, math, numh)
'use strict';
const fs = require('fs');
const path = require('path');
const { profile } = require('./tools/nprof.js');

const ROOT = path.join(__dirname, '..');
const SPEED = path.join(ROOT, 'tests', 'speed');
// Each program: its stretches, each [name, how it's shown: 'ms' the whole, or n: cycles a pass (or a call) of n]
const PROGS = [
  { name: 'numb', file: 'numb.bas', cmd: 'basic numb.bas', mark: 0x6F, segs: [['2000 integer additions', 'ms'],
    ['500 additions of 0.1', 'ms'], ['300 of i / 7 summed', 'ms'], ['300 of * 1.5, / 1.5', 'ms'], ['2 doubled 100 times', 'ms'],
    ['100 SQR', 'ms'], ['50 SIN', 'ms'], ['50 EXP', 'ms'], ['50 LOG', 'ms'], ['300 STR$', 'ms'], ['300 VAL', 'ms']] },
  { name: 'small', file: 'small.bas', cmd: 'basic small.bas', mark: 0x6F, segs: [['an empty FOR pass', 200], ['X = I', 200],
    ['X = I + G', 200], ['X = A + B (1.5 + 2.25)', 200], ['X = A * B', 200], ['X = I / G (3)', 200], ['X = C + D (1/3 + 1/7)', 200],
    ['X = E * F (12 digits)', 200], ['IF A < B', 200], ['X = INT(A)', 200], ['X = E + F (12 digits)', 200], ['X = I \\ G', 200],
    ['AR(I) = I', 200], ['X = A', 200], ['FOR X = 0 TO 20 STEP 0.1', 201]] },
  { name: 'math', file: 'math.bas', cmd: 'basic math.bas', mark: 0x6F, segs: [['the first SIN', 1], ['the first EXP', 1],
    ['EXP', 20], ['SIN', 20], ['LOG', 20], ['SQR', 20], ['ATN', 20], ['I ^ 0.37', 20], ['SIN, 30 digits', 20]] },
  { name: 'numh', file: 'numh.hl', cmd: 'hylang numh.hl', mark: 0x7FFF, segs: [['60! twenty times', 'ms'],
    ['300 big integers\' additions', 'ms'], ['100 products of 30 digits', 'ms'], ['100 big divisions', 'ms'],
    ['1/1 to 1/20 summed, 10 times', 'ms'], ['300 of i/3 + i/7 summed', 'ms'], ['300 additions of 0.1', 'ms'],
    ['200 products 1.25 * 3.5', 'ms'], ['100 to-str of 2^100', 'ms'], ['100 val of 30 digits', 'ms'], ['sin', 20],
    ['(+ 1.5 2.25)', 200], ['(/ i 3)', 200], ['(+ 1/3 1/7)', 200], ['(< 1.5 2.25)', 200]] },
];

function measure(tree, only) {
  const res = {};
  for (const p of PROGS) {
    if (only && !only.includes(p.name)) continue;
    const r = profile({ tree, files: { [p.file]: fs.readFileSync(path.join(SPEED, p.file)) }, cmd: p.cmd, mark: p.mark,
      segs: p.segs.map(s => s[0]), labels: false });
    for (const [n, how] of p.segs) {
      const s = r.segs.get(n);
      res[p.name + ': ' + n] = !s ? null : how === 'ms' ? Math.round(s.total / 3579.545) : Math.round(s.total / how);
    }
    res[p.name + ': (output)'] = r.out.trim().split('\n').slice(-2).join(' | ');
  }
  return res;
}

if (require.main === module) {
  const args = process.argv.slice(2);
  const opt = n => { const i = args.indexOf(n); return i < 0 ? null : args[i + 1]; };
  const only = opt('--only') ? opt('--only').split(',') : null;
  const tree = opt('--build') ? path.resolve(opt('--build')) : ROOT;
  const t0 = Date.now();
  const cur = measure(tree, only);
  const vs = opt('--vs') ? measure(path.resolve(opt('--vs')), only) : opt('--base') ? JSON.parse(fs.readFileSync(opt('--base'), 'utf8')) : null;
  const prev = opt('--prev') ? JSON.parse(fs.readFileSync(opt('--prev'), 'utf8')) : null;
  if (opt('--save')) fs.writeFileSync(opt('--save'), JSON.stringify(cur, null, 1));
  const unit = k => { const [pn, sn] = k.split(': '); const p = PROGS.find(q => q.name === pn), s = p && p.segs.find(x => x[0] === sn); return !s ? '' : s[1] === 'ms' ? 'ms' : p.name === 'math' || sn === 'sin' ? 'cycles a call' : 'cycles a pass'; };
  let prog = '';
  for (const k of Object.keys(cur)) {
    const [pn, sn] = k.split(': ');
    if (sn === '(output)') continue;
    if (pn !== prog) { prog = pn; console.log('\n' + pn.padEnd(34) + (vs ? '  original' : '') + (prev ? '      prev' : '') + '        now' + (vs ? '  vs orig.' : '') + (prev ? '  vs prev' : '')); }
    const a = vs ? vs[k] : null, p = prev ? prev[k] : null, b = cur[k];
    const f = v => v === null || v === undefined ? '-' : v.toLocaleString('en-US');
    const x = (u, w) => u && w ? (u / w).toFixed(2) + 'x' : '-';
    console.log('  ' + sn.padEnd(32) + (vs ? f(a).padStart(10) : '') + (prev ? f(p).padStart(10) : '') + f(b).padStart(11) + (vs ? x(a, b).padStart(10) : '') +
      (prev ? x(p, b).padStart(9) : '') + '  ' + unit(k));
  }
  for (const k of Object.keys(cur).filter(k => k.endsWith('(output)'))) if (vs && vs[k] !== undefined && vs[k] !== cur[k]) console.log('OUTPUT DIFFERS ' + k + ': ' + vs[k] + ' / ' + cur[k]);
  console.log('\n(' + ((Date.now() - t0) / 1000).toFixed(0) + ' s)');
}
module.exports = { measure, PROGS };
