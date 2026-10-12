#!/usr/bin/env node
// buildlog.js - the Student Companion's build log, kept in step with the manual: each of Chapter 4's tables of
// parts (04-building.md) as a checklist, with a box to tick when a row's parts are fitted and one for when they've
// been checked, and how many parts the row has.
//
// Usage: node manual/tools/buildlog.js --update FILE.md
//   FILE.md has places marked  <!-- generated: TABLE -->  ...  <!-- end generated: TABLE -->  where TABLE names one
//   of Chapter 4's tables by the heading it's under and its number there: "Step 2/1" (the first table under
//   "### Step 2 ..."), "4.5/1" (the first under "## 4.5 ...").
'use strict';
const fs = require('fs');
const path = require('path');
const MANUAL = path.join(__dirname, '..');
const SOURCE = path.join(MANUAL, '04-building.md');

// Chapter 4's tables: "Step 2/1" -> its rows (each a list of cells), the header first
function tables() {
  const out = new Map();
  let under = '', n = 0, rows = null;
  for (const l of fs.readFileSync(SOURCE, 'utf8').split(/\r?\n/)) {
    const h = /^#{2,3} (Step \d+|\d+\.\d+)\b/.exec(l);
    if (h) { under = h[1]; n = 0; }
    if (/^\|/.test(l)) {
      if (!rows) { rows = []; n++; out.set(under + '/' + n, rows); }
      if (!/^\|[-| ]+\|$/.test(l)) rows.push(l.replace(/^\||\|$/g, '').split('|').map(c => c.trim()));
    } else rows = null;
  }
  return out;
}
// How many parts a cell names: "R23, R24", "C12 to C46", "IC1 to IC8, U4"
function count(cell) {
  let n = 0;
  for (const p of cell.replace(/\([^)]*\)/g, '').split(',')) {
    const r = /([A-Z]+)(\d+) to ([A-Z]+)(\d+)/.exec(p);
    if (r) n += +r[4] - +r[2] + 1;
    else if (/[A-Z]+\d+/.test(p)) n++;
  }
  return n;
}
function checklist(rows) {
  const head = rows[0];
  const parts = head.findIndex(c => /^(Parts|Places)$/.test(c));
  const many = head.includes('How many');
  const out = ['| Fitted | Checked | ' + head.join(' | ') + (many || parts < 0 ? '' : ' | How many') + ' |',
    '|---|---|' + head.map(() => '---|').join('') + (many || parts < 0 ? '' : '---|')];
  for (const r of rows.slice(1)) out.push('| ☐ | ☐ | ' + r.map(c => c.replace(/\s*\(\d+ in all\)/, '')).join(' | ') + (many || parts < 0 ? '' : ' | ' + count(r[parts])) + ' |');
  return out.join('\n');
}

function main() {
  const i = process.argv.indexOf('--update');
  const file = process.argv[i + 1];
  if (i < 0 || !file) { console.error('usage: buildlog.js --update FILE.md'); process.exit(2); }
  const T = tables();
  let n = 0;
  const text = fs.readFileSync(file, 'utf8').replace(/(<!-- generated: ([^>]*?) -->\n)[\s\S]*?(<!-- end generated: \2 -->)/g, (m0, a, tag, b) => {
    if (!T.has(tag)) throw new Error('buildlog.js: Chapter 4 has no table "' + tag + '" (it has ' + [...T.keys()].join(', ') + ')');
    n++;
    return a + '\n' + checklist(T.get(tag)) + '\n\n' + b;
  });
  fs.writeFileSync(file, text);
  console.log(n + ' tables in ' + path.basename(file));
}
main();
