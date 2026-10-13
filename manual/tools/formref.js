#!/usr/bin/env node
// formref.js - the Technical Reference's chapter of hylang's forms and functions: an entry for every row of the
// manual's Appendix B from its special forms on (each row a form, or several of a kind, and what it does), in
// alphabetical order of its first name, each with the kind of thing it is and the forms it's written in.  An entry
// written by hand (manual/reference/hylang-own.md: "## names" and what follows, to the next "## ") takes the place
// of the one made here for the same first name, or adds one; the chapter's own start is that file's text before
// its first "## ".
// Usage: node manual/tools/formref.js      (writes manual/reference/r5-hylang.md)
'use strict';
const fs = require('fs');
const path = require('path');
const ROOT = path.join(__dirname, '..', '..');
const read = f => fs.readFileSync(path.join(ROOT, f), 'utf8').replace(/\r\n?/g, '\n');
const NL = '\n';

// A form's name: "(len l)" is len, "name" (a variable, a constant) itself; none for a piece of syntax
function nameOf(code) {
  let w = code.trim();
  if (w.startsWith('(')) w = w.slice(1);
  w = w.split(/[\s()]/)[0];
  return w && !/^[{\[]/.test(w) ? w : null;
}
const rows = [];
let sec = '', on = false;
for (const line of read('manual/app-b-hylang.md').split(NL)) {
  let m;
  if ((m = /^## B\.\d+ (.*)$/.exec(line))) { sec = m[1]; on = !/^Syntax/.test(sec); continue; }
  if (!on || !/^\| `/.test(line)) continue;
  const cells = line.split(/(?<!\\)\|/).slice(1, -1).map(s => s.trim().replace(/\\\|/g, '|'));
  if (cells.length < 2) continue;
  const forms = (cells[0].match(/`[^`]+`/g) || []).map(w => w.slice(1, -1));
  const names = [];
  for (const f of forms) { const n = nameOf(f); if (n && !names.includes(n)) names.push(n); }
  if (!names.length) continue;
  rows.push({ sec, forms, names, does: cells[cells.length - 1] });
}
const by = new Map();
for (const r of rows) if (!by.has(r.names[0])) by.set(r.names[0], r);
const sortKey = w => w.toLowerCase().replace(/^[^a-z0-9]+/, '') + ' ' + w;

const ownText = read('manual/reference/hylang-own.md');
const firstOwn = ownText.search(/^## /m);
const intro = (firstOwn < 0 ? ownText : ownText.slice(0, firstOwn)).replace(/\s+$/, '') + NL;
// The entries written by hand: hylang-own.md's, and those of the files beside it (hylang-own-lists.md ...: a kind
// of function each).  An entry's every name (its heading's, between commas) takes the made entry of that first name
// out, so a row of the appendix can be written as several entries
const own = new Map(), taken = new Set();
const ownFiles = fs.readdirSync(path.join(ROOT, 'manual/reference')).filter(f => /^hylang-own-.*\.md$/.test(f)).sort();
for (const text of [firstOwn < 0 ? '' : ownText.slice(firstOwn), ...ownFiles.map(f => read('manual/reference/' + f))])
  for (const part of text.split(/^(?=## )/m)) {
    const m = /^## (.*)$/m.exec(part);
    if (!m) continue;
    const names = m[1].split(', ').map(s => s.trim());
    if (own.has(names[0])) throw new Error('two entries for ' + names[0]);
    own.set(names[0], part.replace(/\s+$/, '') + NL);
    for (const n of names) taken.add(n);
  }
const KIND = { 'Special forms': 'A special form', 'The library': 'A function of the library, written in hylang' };
const stop = s => /[.!?)]$/.test(s) ? s : s + '.';
function entry(r) {
  let s = '## ' + r.names.join(', ') + NL + NL;
  s += (KIND[r.sec] || 'A function') + (KIND[r.sec] ? '' : ': ' + r.sec.charAt(0).toLowerCase() + r.sec.slice(1)) + '.' + NL + NL;
  s += '#### Syntax' + NL + NL + '```text' + NL + r.forms.join(NL) + NL + '```' + NL + NL;
  s += '#### Description' + NL + NL + stop(r.does) + NL;
  return s;
}
for (const [k, r] of [...by]) if (!own.has(k) && r.names.some(n => taken.has(n))) by.delete(k);
const keys = new Set([...by.keys(), ...own.keys()]);
let out = intro + NL, n = 0, hand = 0;
for (const k of [...keys].sort((a, b) => sortKey(a) < sortKey(b) ? -1 : 1)) {
  if (own.has(k)) { out += own.get(k) + NL; hand++; } else out += entry(by.get(k)) + NL;
  n++;
}
fs.writeFileSync(path.join(ROOT, 'manual/reference/r5-hylang.md'), out);
console.log('r5-hylang.md: ' + n + ' entries (' + hand + ' written by hand), from ' + rows.length + ' rows');
