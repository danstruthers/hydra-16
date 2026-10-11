#!/usr/bin/env node
// ****************************************************************************
// callref.js - the book's system call reference, from the specification (base/spec/api.def, base/spec/errors.def):
// each group's calls in a table, then each call under a heading of its own (its address, registers in and out,
// errors, whether it waits, its HyForth word and hylang function, what it does), the error codes, and the constants
// hydra.inc has.  It reads the spec with base/tools/apigen.js, and names the languages' forms as reborn/tools/apigen.js
// makes them, so the reference says what the system does.  The text goes into the chapter between its markers,
// <!-- calls:begin --> and <!-- calls:end -->, so what's written around it stays.
//
// Usage: node docs/build/callref.js [--check] [CHAPTER.md]
//   --check       don't write: say if the chapter is out of date (exit 1)
//   CHAPTER.md    the chapter (default docs/09-programming/calls-reference.md)
'use strict';
const fs = require('fs');
const path = require('path');

const REPO = path.join(__dirname, '..', '..');
const base = require(path.join(REPO, 'base', 'tools', 'apigen.js'));
const reborn = require(path.join(REPO, 'reborn', 'tools', 'apigen.js'));

const hx = (v, n) => '$' + v.toString(16).toUpperCase().padStart(n, '0');
const esc = t => String(t).replace(/\|/g, '\\|');
// A call's doc's first sentence (or its first clause, to a colon), not counting what's in brackets
function first(doc) {
  const t = doc.join(' ');
  let depth = 0;
  for (let i = 0; i < t.length; i++) {
    const ch = t[i];
    if (ch === '(') depth++;
    else if (ch === ')') depth = Math.max(0, depth - 1);
    else if (depth === 0 && (ch === '.' || ch === ':') && (i + 1 === t.length || t[i + 1] === ' ')) return t.slice(0, i) + '.';
  }
  return t;
}

function reference() {
  const api = base.readApi(path.join(REPO, 'base', 'spec', 'api.def'));
  const errors = base.readErrors(path.join(REPO, 'base', 'spec', 'errors.def'));
  const fc = reborn.forthCalls(api);
  const out = [];
  out.push('', 'Made from `base/spec/api.def` and `base/spec/errors.def` by `node docs/build/callref.js`: ' + api.calls.length +
    ' calls in ' + api.groups.length + ' groups.', '');
  out.push('**The groups:** ' + api.groups.map(g => '[' + g.name + '](#' + g.name + '-' + g.base.toString(16) + ')').join(', ') +
    '; [the error codes](#the-error-codes); [the constants](#the-constants).');
  for (const g of api.groups) {
    out.push('', '---', '', '## ' + g.name + ' (`' + hx(g.base, 4) + '`)', '', g.doc + ': ' + g.slots + ' slots from `' + hx(g.base, 4) + '`, ' +
      g.calls.length + ' used.', '');
    if (!g.calls.length) { out.push('No calls yet.'); continue; }
    out.push('| Call | Address | What it does |', '| :--- | :------ | :----------- |');
    for (const c of g.calls) out.push('| [`' + c.name + '`](#' + c.name.toLowerCase() + ') | `' + hx(c.addr, 4) + '` | ' + esc(first(c.doc)) + ' |');
    for (const c of g.calls) {
      out.push('', '### ' + c.name, '', '`' + hx(c.addr, 4) + '`' + (c.far ? ', a far call (its routine on another BIOS page)' : '') + '.', '');
      out.push('| | |', '| :-- | :-- |');
      out.push('| **In** | ' + esc(c.in.join(' ') || '-') + ' |');
      out.push('| **Out** | ' + esc(c.out.join(' ') || '-') + ' |');
      out.push('| **Errors** | ' + esc(c.errorsText || '-') + ' |');
      out.push('| **Waits** | ' + (c.blocks || '-') + ' |');
      if (fc.includes(c)) {
        out.push('| **HyForth** | `' + esc(reborn.forthName(c) + ' ' + reborn.forthEffect(c)) + '` |');
        out.push('| **hylang** | `' + esc(reborn.hlName(c) + ' ' + c.hl) + '` |');
      }
      out.push('', c.doc.join(' '));
    }
  }
  out.push('', '---', '', '## The error codes', '', 'A call that fails returns C = 1 and one of these in `.A`.  `ERRSTR` gives the text; ' +
    'the C library sets `errno` to the code\'s POSIX equivalent (and `_oserror` to the code itself).', '',
    '| Code | Name | Text |', '| :--- | :--- | :--- |');
  for (const e of errors) out.push('| `' + hx(e.code, 2) + '` | `' + e.name + '` | ' + esc(e.text) + ' |');
  out.push('', '---', '', '## The constants', '', 'What `hydra.inc` defines besides the calls and the error codes (`hydracalls.h` has them for C, with `HY_` before each name).', '',
    '| Name | Value | What it is |', '| :--- | :---- | :--------- |');
  for (const k of api.consts) out.push('| `' + k.name + '` | `' + esc(k.text) + '` | ' + esc(k.doc) + ' |');
  return out.join('\n');
}

function update(chapter) {
  let text = fs.readFileSync(chapter, 'utf8');
  const nl = text.includes('\r\n') ? '\r\n' : '\n';
  text = text.replace(/\r\n/g, '\n');
  const begin = '<!-- calls:begin -->', end = '<!-- calls:end -->';
  const i = text.indexOf(begin), j = text.indexOf(end);
  if (i < 0 || j < i) throw new Error(chapter + ': no ' + begin + ' ... ' + end);
  text = text.slice(0, i + begin.length) + '\n' + reference() + '\n\n' + text.slice(j);
  return text.replace(/\n/g, nl);
}

if (require.main === module) {
  const a = process.argv.slice(2);
  let check = false, chapter = path.join(REPO, 'docs', '09-programming', 'calls-reference.md');
  for (const x of a) if (x === '--check') check = true; else chapter = path.resolve(x);
  try {
    const text = update(chapter), old = fs.readFileSync(chapter, 'utf8'), name = path.relative(REPO, chapter);
    if (check) { if (text !== old) { console.error('callref: ' + name + ' is out of date (node docs/build/callref.js)'); process.exit(1); } console.log('callref: ' + name + ' as it was'); }
    else if (text !== old) { fs.writeFileSync(chapter, text); console.log('callref: ' + name + ' written'); }
    else console.log('callref: ' + name + ' as it was');
  } catch (e) { console.error('callref: ' + e.message); process.exit(1); }
}
module.exports = { reference };
