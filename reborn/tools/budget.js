#!/usr/bin/env node
// ****************************************************************************
// budget.js - what the build used, and what's left (docs/reimplementation-from-scratch.md, §16): BIOS ROM page 0
// (the kernel, its jump table, the COMMON block), and each module's ROM and RAM.  The time budgets (IRQ latency,
// SCALL, kcopy) are measured by the tests: sim/test.js.
//
// Usage: node tools/budget.js          (after a build)
// From Node: report(root, { modules, tests, entries }) gives { text, page0Free, ... }.
'use strict';
const fs = require('fs');
const path = require('path');

const hx = (v, n = 4) => '$' + v.toString(16).toUpperCase().padStart(n, '0');

// The map's segment list: { NAME: { start, end, size } }
function segments(map) {
  const out = {}, text = fs.readFileSync(map, 'latin1');
  const list = text.slice(text.indexOf('Segment list:'));
  for (const m of list.matchAll(/^(\w+)\s+([0-9A-F]{6})\s+([0-9A-F]{6})\s+([0-9A-F]{6})/gm))
    out[m[1]] = { start: parseInt(m[2], 16), end: parseInt(m[3], 16), size: parseInt(m[4], 16) };
  return out;
}

function report(root, built = {}) {
  const lines = [], seg = segments(path.join(root, 'obj', 'kernel', 'bios.map'));
  const kernelEnd = Math.max(...['RESET_P0', 'KCODE', 'KRODATA'].filter(s => seg[s]).map(s => seg[s].end + 1));
  const page0Free = (seg.JUMPTABLE ? seg.JUMPTABLE.start : 0xF800) - kernelEnd;
  const jt = seg.JUMPTABLE ? seg.JUMPTABLE.size : 0;
  lines.push('BIOS ROM page 0:');
  lines.push('  kernel        ' + hx(0xE000) + '-' + hx(kernelEnd - 1) + '  ' + String(kernelEnd - 0xE000).padStart(5) + ' bytes (code ' +
    (seg.KCODE ? seg.KCODE.size : 0) + ', data ' + (seg.KRODATA ? seg.KRODATA.size : 0) + '); ' + page0Free + ' free before the jump table');
  lines.push('  jump table    ' + hx(0xF800) + '-' + hx(0xF800 + jt - 1) + '  ' + String(jt).padStart(5) + ' bytes (' + jt / 3 + ' slots)');
  if (seg.COMMON_P0) lines.push('  COMMON block  ' + hx(seg.COMMON_P0.start) + '-' + hx(seg.COMMON_P0.end) + '  ' + String(seg.COMMON_P0.size).padStart(5) + ' bytes, on every page');
  for (const [p, what] of [[1, 'tasks, memory, notes'], [2, 'files, names, pipes'], [3, 'namespaces'], [4, 'POST']]) {
    const code = seg['KCODE_P' + p], data = seg['KRODATA_P' + p];
    if (!code && !data) continue;
    const used = (code ? code.size : 0) + (data ? data.size : 0);
    lines.push('BIOS ROM page ' + p + ' (' + what + '): ' + used + ' bytes (code ' + (code ? code.size : 0) + ', data ' + (data ? data.size : 0) + '); ' +
      (0x1D00 - 3 - used) + ' free below the COMMON block');
  }
  const mods = (title, set, dir) => {
    const names = Object.keys(set || {}).sort();
    if (!names.length) return;
    lines.push(title);
    for (const n of names) {
      const d = set[n], top = d.readUInt16LE(22), data = d.readUInt16LE(16), bss = d.readUInt16LE(20);
      lines.push('  ' + n.padEnd(12) + String(d.length).padStart(6) + ' bytes ROM (' + Math.round(d.length * 100 / 0x4000) + '% of a bank)' +
        (top > 0x0400 ? '   RAM ' + hx(0x0400) + '-' + hx(top - 1) + ' (data ' + data + ', BSS ' + bss + ')' : '   no RAM'));
    }
  };
  mods('Modules:', built.modules);
  mods('Test modules:', built.tests);
  if (page0Free < 512) lines.push('WARNING: under 512 bytes left on BIOS ROM page 0');
  return { text: lines.join('\n'), page0Free, kernelEnd, segments: seg };
}

if (require.main === module) console.log(report(path.join(__dirname, '..')).text);
module.exports = { report, segments };
