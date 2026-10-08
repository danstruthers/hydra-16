#!/usr/bin/env node
// ****************************************************************************
// consbench.js - the console's costs (docs/design/plans/WINDOWS.md, W1's spike): commands typed at rc, each timed from its
// Enter to the next prompt, in the emulator's cycles; and from those, a window's output a byte (into a window not
// shown, less the same reading to /dev/null), a scroll (a file of short lines into it), and the window shown's (on
// the serial port, paced by the line).  --vs TREE runs the same in another tree's build (another branch's worktree,
// built), a column of its.
//
// Usage: node sim/consbench.js [--vs TREE] [-v]
// Build first (node build.js).
'use strict';
const fs = require('fs');
const path = require('path');

const KB = 8192;                                              // The files: 8K of text in 80-column lines, and of empty ones
const LONG = Array.from({ length: Math.ceil(KB / 80) }, (v, i) => (String(i).padStart(4, '0') + ' ').padEnd(79, 'abcdefghij')).join('\n').slice(0, KB - 1) + '\n';
const SHORT = 'x\n'.repeat(KB / 2);
const STEPS = [
  ['read8k', 'cat /pc/long >/dev/null', 'reading the 8K file (the baseline)'],
  ['hidden8k', "echo new >/dev/wctl; cat /pc/long >'#c1/cons'", 'the 8K into a window not shown'],
  ['readlines', 'cat /pc/short >/dev/null', 'reading 4096 short lines (the baseline)'],
  ['hiddenlines', "echo new >/dev/wctl; cat /pc/short >'#c1/cons'", '4096 short lines into a window not shown (each a scroll)'],
  ['shown8k', 'cat /pc/long', 'the 8K into the window shown (the serial port at 115200)'],
];

function run(root, verbose) {
  const { boot } = require(path.join(root, 'sim', 'run.js'));
  const { image } = require(path.join(root, 'sim', 'test.js'));
  const { createPcHost } = require(path.join(root, 'sim', 'lib', 'pchost.js'));
  const dir = path.join(root, 'obj', 'pc', 'consbench');
  fs.rmSync(dir, { recursive: true, force: true });
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, 'long'), LONG);
  fs.writeFileSync(path.join(dir, 'short'), SHORT);
  const input = ['echo b115200 >/dev/serctl', ...STEPS.map(s => s[1])].map(l => 'ā' + l + '\r').join('');
  const m = boot({ prom: image({ init: 't_rc' }), input, pcHost: createPcHost({ dir }), log: verbose ? s => console.log('  [sim] ' + s) : () => {} });
  const times = {};
  let step = 0, start = 0;
  while (step < STEPS.length && m.cpu.cyc < 3e9 && !m.cpu.halted) {
    m.run(m.cpu.cyc + 10000);
    const out = m.out.replace(/\r/g, '');
    const echo = '% ' + STEPS[step][1] + '\n';
    const at = out.lastIndexOf(echo);
    if (at < 0) continue;
    if (!start) start = m.cpu.cyc;
    if (out.indexOf('\n%', at + echo.length - 1) >= 0 || out.endsWith('\n% ') || out.slice(at + echo.length).includes('% ')) {
      times[STEPS[step][0]] = m.cpu.cyc - start;
      step++;
      start = 0;
    }
  }
  if (verbose) console.log(m.out.replace(/\r/g, '').slice(-2000));
  return times;
}

function main(argv) {
  const vs = argv.includes('--vs') ? argv[argv.indexOf('--vs') + 1] : null, verbose = argv.includes('-v');
  const here = run(path.join(__dirname, '..'), verbose), there = vs ? run(path.resolve(vs, fs.existsSync(path.join(vs, 'reborn')) ? 'reborn' : ''), verbose) : null;
  const row = (what, f) => console.log(what.padEnd(58) + String(f(here)).padStart(12) + (there ? String(f(there)).padStart(12) : ''));
  console.log(''.padEnd(58) + 'this tree'.padStart(12) + (there ? 'the other'.padStart(12) : ''));
  for (const [k, , what] of STEPS) row(what + ' (cycles)', t => t[k] === undefined ? '-' : t[k]);
  row('a byte into a window not shown (cycles)', t => ((t.hidden8k - t.read8k) / KB).toFixed(0));
  row('a short line into it, a scroll (cycles)', t => ((t.hiddenlines - t.readlines) / (KB / 2)).toFixed(0));
  row('a byte into the window shown (cycles; the line\'s 311)', t => ((t.shown8k - t.read8k) / KB).toFixed(0));
}

main(process.argv.slice(2));
