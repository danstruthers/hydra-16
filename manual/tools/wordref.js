#!/usr/bin/env node
// wordref.js - the Technical Reference's chapter of HyForth's words: an entry for every word the manual's tables list
// (Appendix A, and Chapter 20's: each row a word or a family of words, its stack picture and what it does), in
// alphabetical order, each with the word set or library it belongs to.  An entry written by hand
// (manual/reference/forth-own.md: "## words" and what follows, to the next "## ") takes the place of the one made
// here for the same first word, or adds one; the chapter's own start is that file's text before its first "## ".
// Usage: node manual/tools/wordref.js [--weak]     (writes manual/reference/r4-forth.md; --weak: the rows with no
//        stack picture or no description of their own, that want an entry by hand, listed)
'use strict';
const fs = require('fs');
const path = require('path');
const ROOT = path.join(__dirname, '..', '..');
const read = f => fs.readFileSync(path.join(ROOT, f), 'utf8').replace(/\r\n?/g, '\n');
const NL = '\n';

// Where each library is, and when it's there
const AT_START = ['coreext', 'exception', 'file', 'tools'];
function where(lib) {
  if (!lib) return 'in the ROM core, always there';
  if (AT_START.includes(lib)) return 'in the `' + lib + '` library, which is loaded as HyForth starts';
  if (lib === 'shell') return 'in the `shell` library, which is loaded when HyForth starts as a shell';
  return 'in the `' + lib + '` library (`lib ' + lib + '`)';
}

// ---- The rows: | `word` [, `word`] [(I)] [(C)] | ( stack ) | what it does |
// (The words of the sections that hold several libraries' words)
const OWN_LIB = { allocate: 'memory', free: 'memory', resize: 'memory', '{:': 'locals', 'locals|': 'locals', block: 'block', update: 'block', blk: 'block',
  load: 'block', list: 'block', 'open-blocks': 'block', tbit: 'bits', sbit: 'bits', rand: 'random', random: 'random', rseed: 'random', disasm: 'disasm' };
const rows = [], devLib = new Map();
function tables(file, libOf) {
  let sec = '', lib = '';
  for (const line of read(file).split(NL)) {
    let m;
    if ((m = /^## (?:[A-Z]|\d+)\.\d+ (.*)$/.exec(line))) { sec = m[1].replace(/\s*\(`[^`]+`\)/, ''); lib = libOf(m[1]); continue; }
    if (!/^\| `/.test(line)) continue;
    const cells = line.split(/(?<!\\)\|/).slice(1, -1).map(s => s.trim().replace(/\\\|/g, '|'));
    if (cells.length < 3) {                                   // (A library and its words: the device libraries')
      if (/device libraries/.test(sec)) for (const w of (cells[1].match(/`[^`]+`/g) || [])) devLib.set(w.slice(1, -1), cells[0].replace(/`/g, ''));
      continue;
    }
    const words = (cells[0].match(/`[^`]+`/g) || []).map(w => w.slice(1, -1));
    if (!words.length) continue;
    rows.push({ file, sec, lib, words, imm: /\(I\)/.test(cells[0]), conly: /\(C\)/.test(cells[0]), stack: cells[1], does: cells[cells.length - 1] });
  }
}
tables('manual/app-a-forth.md', h => { const m = /\(`([^`]+)`\)/.exec(h); return m ? m[1] : /^The core/.test(h.replace(/^A\.\d+ /, '')) ? '' : null; });
const CH20 = { 'Running commands': 'hydra', 'Files': 'file', 'The terminal': 'facility', 'The devices': null, 'Sound': 'sound', 'The screen': 'video', 'Numbers': 'numbers' };
tables('manual/20-forth-hydra.md', h => { const k = Object.keys(CH20).find(k => h.startsWith(k)); return k ? CH20[k] : null; });

// One entry a first word: Appendix A's row, or Chapter 20's if it says more (a stack picture where the other has none)
const by = new Map();
for (const r of rows) {
  const k = r.words[0];
  const old = by.get(k);
  if (!old || (!old.stack && r.stack) || (old.stack && r.stack && r.does.length > old.does.length && old.file !== 'manual/app-a-forth.md')) by.set(k, Object.assign({}, r, { lib: old && old.lib != null ? old.lib : r.lib, sec: old ? old.sec : r.sec }));
}
// (A word that is only inside another row's family isn't an entry of its own)
const sortKey = w => w.toLowerCase().replace(/^[^a-z0-9]+/, '') + '\u0001' + w;

const ownText = read('manual/reference/forth-own.md');
const firstOwn = ownText.search(/^## /m);
const intro = (firstOwn < 0 ? ownText : ownText.slice(0, firstOwn)).replace(/\s+$/, '') + NL;
const own = new Map();
if (firstOwn >= 0) for (const part of ownText.slice(firstOwn).split(/^(?=## )/m)) {
  const m = /^## (.*)$/m.exec(part);
  if (m) own.set(m[1].split(',')[0].trim(), part.replace(/\s+$/, '') + NL);
}
const stop = s => /[.!?)]$/.test(s) ? s : s + '.';
function entry(r) {
  let s = '## ' + r.words.join(', ') + NL + NL;
  s += (r.sec === 'The core' ? 'Core word set' : r.sec) + '; ' + (r.lib === null && OWN_LIB[r.words[0]] ? where(OWN_LIB[r.words[0]]) : r.lib === null ? (devLib.has(r.words[0]) ? 'in the `' + devLib.get(r.words[0]) + '` library, a file of Forth in `/lib/forth` (`lib ' + devLib.get(r.words[0]) + '`)' : 'in a library') : where(r.lib)) + '.' +
    (r.imm ? ' Immediate.' : '') + (r.conly ? ' Compile only.' : '') + NL + NL;
  if (r.stack) s += '#### Stack' + NL + NL + r.stack + NL + NL;
  s += '#### Description' + NL + NL + stop(r.does) + NL;
  return s;
}
const keys = new Set([...by.keys(), ...own.keys()]);
const weak = [];
let out = intro + NL, n = 0, hand = 0;
for (const k of [...keys].sort((a, b) => sortKey(a) < sortKey(b) ? -1 : 1)) {
  if (own.has(k)) { out += own.get(k) + NL; hand++; }
  else {
    const r = by.get(k);
    out += entry(r) + NL;
    if (!r.stack || /^(The same|As )/.test(r.does)) weak.push(r.words.join(', ') + '  [' + r.sec + ']  ' + r.does.slice(0, 80));
  }
  n++;
}
fs.writeFileSync(path.join(ROOT, 'manual/reference/r4-forth.md'), out);
console.log('r4-forth.md: ' + n + ' entries (' + hand + ' written by hand), ' + weak.length + ' want one');
if (process.argv.includes('--weak')) console.log(weak.join(NL));
