#!/usr/bin/env node
// renumber.js - make room for a chapter, or close a gap: the manual's chapters from FROM on move by BY, and so does
// everything that names them: the files' names (07-shells.md), book.txt, the headings ("# Chapter 7.", "## 7.4"),
// the captions ("Figure 7-1."), and every reference in the three books and in the tools' comments (as xref.js
// reads them).  It then checks itself: each reference must point at the same title as before.
//
// Usage: node manual/tools/renumber.js FROM BY [--dry]
//   node manual/tools/renumber.js 6 1      chapters 6, 7, 8 ... become 7, 8, 9 ... (then write the new 06-*.md)
//   --dry    say what would change, and change nothing
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const x = require('./xref.js');

const args = process.argv.slice(2).filter(a => !a.startsWith('--'));
const dry = process.argv.includes('--dry');
const FROM = +args[0], BY = +args[1];
if (!(FROM > 0) || !BY) { console.error('usage: renumber.js FROM BY [--dry]'); process.exit(2); }

const move = n => (n >= FROM ? n + BY : n);
// A number as it's written ("7", "7.4", "7-1", "A.2"), moved
const moved = num => num.replace(/^\d+/, d => String(move(+d)));
const pad = n => String(n).padStart(2, '0');

// Every place a number is written in a text: the references and the definitions, once each
function places(text, glossary) {
  const all = x.refs(text, glossary).concat(x.defs(text).map(d => ({ kind: d.kind, num: d.num, at: d.at })));
  all.sort((a, b) => a.at - b.at);
  return all.filter((p, i) => i === 0 || p.at !== all[i - 1].at);
}
function rewrite(text, glossary) {
  let out = text, n = 0;
  for (const p of places(text, glossary).reverse()) {
    if (text.substr(p.at, p.num.length) !== p.num) throw new Error('lost my place at ' + p.at + ': ' + p.num);
    const to = moved(p.num);
    if (to !== p.num) { out = out.slice(0, p.at) + to + out.slice(p.at + p.num.length); n++; }
  }
  return { out, n };
}

// The titles the references of a set of files point at, in order
function resolved(files, texts, map) {
  const out = [];
  files.forEach((f, i) => { for (const r of x.refs(texts[i], x.isGlossary(f))) out.push((map.get(r.kind + ' ' + r.num) || { title: '?' }).title); });
  return out;
}
function defMap(files, texts) {
  const map = new Map();
  files.forEach((f, i) => { for (const d of x.defs(texts[i])) map.set(d.kind + ' ' + d.num, { title: d.title }); });
  return map;
}

const manualFiles = x.bookFiles(x.MANUAL);
const bookFiles = [].concat(...x.allBooks().map(x.bookFiles));
const tools = path.join(x.MANUAL, 'tools');
const others = fs.readdirSync(tools).filter(f => /\.(js|txt|sh)$/.test(f) && f !== 'renumber.js' && f !== 'xref.js').map(f => path.join(tools, f))
  .concat([path.join(x.MANUAL, 'build.js')]);

const before = bookFiles.map(f => fs.readFileSync(f, 'utf8'));
const titlesBefore = resolved(bookFiles, before, defMap(manualFiles, manualFiles.map(f => before[bookFiles.indexOf(f)])));

// The files' new names
const rename = f => {
  const m = /^(\d\d)(-.*\.md)$/.exec(path.basename(f));
  return m && path.dirname(f) === x.MANUAL ? path.join(x.MANUAL, pad(move(+m[1])) + m[2]) : f;
};
let changes = 0;
const after = bookFiles.map((f, i) => { const r = rewrite(before[i], x.isGlossary(f)); changes += r.n; if (r.n) console.log(path.basename(f) + ': ' + r.n); return r.out; });
const newNames = bookFiles.map(rename);
const titlesAfter = resolved(newNames, after, defMap(manualFiles.map(rename), manualFiles.map(f => after[bookFiles.indexOf(f)])));
let wrong = 0;
if (titlesAfter.length !== titlesBefore.length) { console.log('references: ' + titlesBefore.length + ' before, ' + titlesAfter.length + ' after'); wrong++; }
titlesBefore.forEach((t, i) => { if (t !== titlesAfter[i]) { console.log('reference ' + i + ': "' + t + '" became "' + titlesAfter[i] + '"'); wrong++; } });
if (wrong) { console.error('renumber.js: ' + wrong + ' references would point somewhere else; nothing changed'); process.exit(1); }
console.log(changes + ' numbers in the books, ' + titlesBefore.length + ' references still point where they did');
if (dry) process.exit(0);

// The books, under their new names (git moves them, so their history follows); highest first when moving up
const order = bookFiles.map((f, i) => i).sort((a, b) => (BY > 0 ? -1 : 1) * path.basename(bookFiles[a]).localeCompare(path.basename(bookFiles[b])));
for (const i of order) {
  if (newNames[i] !== bookFiles[i]) execFileSync('git', ['mv', bookFiles[i], newNames[i]], { cwd: x.MANUAL });
  if (after[i] !== before[i]) fs.writeFileSync(newNames[i], after[i]);
}
// book.txt
const listFile = path.join(x.MANUAL, 'book.txt');
fs.writeFileSync(listFile, fs.readFileSync(listFile, 'utf8').replace(/^(\d\d)(-.*\.md)$/gm, (m0, n, rest) => pad(move(+n)) + rest));
// index.txt: "term: 6.6, 9.2, 3" (sections, and chapters)
const indexFile = path.join(x.MANUAL, 'index.txt');
if (fs.existsSync(indexFile)) {
  fs.writeFileSync(indexFile, fs.readFileSync(indexFile, 'utf8').split('\n').map(line => {
    const m = /^([^#:][^:]*:)(.*)$/.exec(line);
    return m ? m[1] + m[2].replace(/\d+(\.\d+)?/g, moved) : line;
  }).join('\n'));
}
// The tools' comments and strings
for (const f of others) {
  const text = fs.readFileSync(f, 'utf8');
  const r = rewrite(text, false);
  if (r.n) { fs.writeFileSync(f, r.out); console.log(path.relative(x.MANUAL, f) + ': ' + r.n); }
}
console.log('done: now look at the tables of bare chapter numbers (grep "| [0-9, ]* |$")');
