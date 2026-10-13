#!/usr/bin/env node
// callref.js - the Technical Reference's chapter of system calls, from their one source (base/spec/api.def, with the
// error codes of base/spec/errors.def): an entry for every call, in alphabetical order (its address, whether it can
// wait, its registers in and out, what it does, its errors with their codes and causes, how the languages reach it),
// then the constants and the error codes.  An entry written by hand (manual/reference/calls-own.md: "## NAME" and
// what follows, to the next "## ") takes the place of the one made here; the chapter's own start is that file's
// text before its first "## ".
// Usage: node manual/tools/callref.js          (writes manual/reference/r3-calls.md)
'use strict';
const fs = require('fs');
const path = require('path');
const ROOT = path.join(__dirname, '..', '..');
const read = f => fs.readFileSync(path.join(ROOT, f), 'utf8').replace(/\r\n?/g, '\n');
const NL = '\n';

const GROUPS = { system: 'the system', task: 'tasks', memory: 'memory', file: 'files', name: 'namespaces', cons: 'the console',
  time: 'the clock', server: 'drivers', dbg: 'debugging' };

// ---- api.def: the groups, the calls (each its fields' lines joined), the constants
const groups = {}, calls = [], consts = [];
let cur = null;
for (const line of read('base/spec/api.def').split(NL)) {
  let m;
  if ((m = /^group\s+(\w+)\s+\$([0-9A-F]+)\s+(\d+)\s+"(.*)"/.exec(line))) groups[m[1]] = { base: parseInt(m[2], 16), next: 0, what: m[4] };
  else if ((m = /^call\s+(\w+)\s+(\w+)\s+impl=(\w+)/.exec(line))) {
    const g = groups[m[2]];
    cur = { name: m[1], group: m[2], addr: g.base + 3 * g.next++, f: {} };
    calls.push(cur);
  } else if ((m = /^  (\w+):\s*(.*)$/.exec(line)) && cur) cur.f[m[1]] = (cur.f[m[1]] ? cur.f[m[1]] + ' ' : '') + m[2].trim();
  else if ((m = /^const\s+(\w+)\s+(\S+)(?:\s+"(.*)")?/.exec(line))) { consts.push({ name: m[1], value: m[2], what: m[3] || '' }); cur = null; }
  else if (/^\S/.test(line)) cur = null;
}
const errors = {};
for (const line of read('base/spec/errors.def').split(NL)) {
  const m = /^error\s+(\w+)\s+\$([0-9A-F]+)\s+"(.*?)"/.exec(line);
  if (m) errors[m[1]] = { code: m[2], text: m[3] };
}
const callNames = new Set(calls.map(c => c.name));
const constNames = new Set(consts.map(c => c.name));

// ---- The spec's words as the book writes them
const hex4 = n => '$' + n.toString(16).toUpperCase().padStart(4, '0');
// Registers: .A as A; names of calls, constants and errors in the code face (a family too: SR_*, O_*)
function prose(s) {
  return s.replace(/  +/g, ' ').replace(/\.([AXY])\b/g, '$1')
    .replace(/\b(?:[A-Z][A-Z0-9]*_[A-Z0-9_]*\*?|[A-Z]{2,}[0-9]*_\*)(?![A-Za-z0-9_])|\b[A-Z][A-Z0-9_]{2,}\b/g, w => {
      const base = w.replace(/\*$/, '');
      if (callNames.has(w) || constNames.has(w) || errors[w] || /_\*$/.test(w) || /^(E|O|SR|DM|NOTE|SPAWN|TM|TR|TS|TI|TF|ME|NI|RQ|R|HX|HF|SEM|LINE|REBOOT|TASK|PROG|ENV|KMESG|IO|PATH|FD|ARGS|IRQ|BIND|MREPL|MBEFORE|MAFTER|MCREATE)_/.test(base) || /^M(REPL|BEFORE|AFTER|CREATE)$/.test(base)) return '`' + w + '`';
      return w;
    })
    .replace(/(^|[\s(])(\/(?:dev|proc|env|bin|rom|ram|sd|pc|lib)(?:\/[\w.#|-]+)*)/g, (m0, a, b) => a + '`' + b + '`')
    .replace(/(^|[\s(])(#[a-z|\/][\w\/]*)/g, (m0, a, b) => a + '`' + b + '`');
}
const cap = s => s.charAt(0).toUpperCase() + s.slice(1);
const stop = s => /[.!?)]$/.test(s) ? s : s + '.';
// A field's items: at each "; " that isn't inside parentheses
function items(s) {
  const out = [];
  let depth = 0, at = 0;
  for (let i = 0; i < s.length; i++) {
    if (s[i] === '(') depth++;
    else if (s[i] === ')') depth--;
    else if (s[i] === ';' && depth === 0) { out.push(s.slice(at, i).trim()); at = i + 1; }
  }
  out.push(s.slice(at).trim());
  return out.filter(Boolean);
}
// "REG = what it is" as a definition list's item; anything else a paragraph
function regList(text, none) {
  if (!text || text === '-') return none + NL;
  const out = [], paras = [];
  for (const it of items(text)) {
    const m = /^((?:\.[AXY]|r\d+|C)(?:(?:\/|, | and )(?:\.[AXY]|r\d+))*(?: \([^)]*\))?) = (.*)$/.exec(it);
    if (m) out.push(prose(m[1]) + NL + ': ' + stop(cap(prose(m[2]))));
    else paras.push(stop(cap(prose(it))));
  }
  return (out.length ? out.join(NL + NL) + NL : '') + (out.length && paras.length ? NL : '') + (paras.length ? paras.join(' ') + NL : '');
}
// The errors: each E_NAME and the cause in parentheses after it; what's left over said after the table
function errorTable(text) {
  if (!text || text === '-') return 'None: the call always succeeds.' + NL;
  const rows = [], rest = [];
  const re = /(E_[A-Z0-9]+)(?:\s*\(([^()]*(?:\([^()]*\)[^()]*)*)\))?|\(([^()]*(?:\([^()]*\)[^()]*)*)\)/g;
  let m;
  while ((m = re.exec(text))) {
    if (m[1]) { if (m[2] && /^and /.test(m[2])) { rows.push([m[1], '']); rest.push(m[2]); } else rows.push([m[1], m[2] || '']); }
    else if (rows.length && !rows[rows.length - 1][1] && !/^and /.test(m[3])) rows[rows.length - 1][1] = m[3];
    else rest.push(m[3]);
  }
  let out = '| Code | Name | Message | Cause |' + NL + '|---|---|---|---|' + NL;
  for (const [name, why] of rows) {
    const e = errors[name] || { code: '??', text: '' };
    out += '| $' + e.code + ' | `' + name + '` | ' + e.text + ' | ' + (why ? cap(prose(why)) : '') + ' |' + NL;
  }
  for (const r of rest) out += NL + stop(cap(prose(r.replace(/^and /, 'Also '))) ) + NL;
  return out;
}
// hylang's form, from the hl: line: REG=KIND ... -> RESULT ...
const KIND = { n: 'a number', 'n?': 'a number, or left out', i: 'a number of 32 bits', s: 'a string', 's?': 'a string, or `NIL`',
  'args?': 'a list of strings, or `NIL`', 'map?': 'a list of bytes, or `NIL`', stat: 'a hash, as `stat` gives' };
function languages(c) {
  const lname = 'sys-' + c.name.toLowerCase().replace(/_/g, '-');
  if (c.f.hl === undefined) return 'Not in the languages: it is a call for drivers, or for the debugger.' + NL;
  const [ins, outs] = c.f.hl.split('->').map(s => s.trim());
  const args = [], notes = [];
  for (const a of ins.split(/\s+/).filter(Boolean)) {
    const m = /^([^=]+)=(.*)$/.exec(a);
    if (!m) continue;
    const reg = prose(m[1]), kind = m[2];
    if (/^buf/.test(kind)) { notes.push(reg + ' is a buffer the function makes itself'); continue; }
    if (kind === 'count' || kind === 'size') { args.push(['*' + kind + '*', 'the most to take: ' + reg]); continue; }
    const bm = /^bytes\((.*)\)$/.exec(kind), im = /^io\((.*)\)$/.exec(kind);
    args.push(['*' + reg + '*', bm ? 'a string or a list of bytes (its length goes in ' + prose(bm[1]) + ')' : im ? 'a string of ' + im[1] + ' bytes, or `NIL` for zeros' : KIND[kind] || kind]);
  }
  const res = (outs || '').split(/\s+/).filter(Boolean).map(o => {
    const m = /^buf=(.*)$/.exec(o);
    if (o === 'buf') return 'the buffer, as a string';
    if (m) return m[1] === 'z' ? "the buffer's text" : m[1] === 'stat' ? 'the stat record, as a hash' : 'the first ' + prose(m[1]) + ' bytes of the buffer, as a string';
    return prose(o);
  });
  let out = 'hylang: `(' + lname + (args.length ? ' ' + args.map(a => a[0].replace(/\*/g, '')).join(' ') : '') + ')`' +
    (args.length ? ', where ' + args.map(a => a[0] + ' is ' + a[1]).join('; ') : '') + '. ' +
    (res.length === 0 ? 'It returns `NIL`' : res.length === 1 ? 'It returns ' + res[0] : 'It returns a list: ' + res.join(', ')) + '.' +
    (notes.length ? ' (Here ' + notes.join('; ') + '.)' : '') + NL + NL;
  out += 'HyForth: `' + lname + '`. HyBASIC: `SYS "' + c.name + '"`.' + NL;
  return out;
}

// ---- The entries
const ownText = read('manual/reference/calls-own.md');
const firstOwn = ownText.search(/^## /m);
const intro = (firstOwn < 0 ? ownText : ownText.slice(0, firstOwn)).replace(/\s+$/, '') + NL;
const own = {};
if (firstOwn >= 0) for (const part of ownText.slice(firstOwn).split(/^(?=## )/m)) {
  const m = /^## (\S+)/.exec(part);
  if (m) own[m[1]] = part.replace(/\s+$/, '') + NL;
}
function entry(c) {
  if (own[c.name]) return own[c.name];
  const g = GROUPS[c.group] || c.group;
  let s = '## ' + c.name + NL + NL;
  s += 'System call ' + hex4(c.addr) + ', one of those for ' + g + '. ' + (c.f.blocks === 'yes' ? 'It can wait.' : 'It never waits.') + NL + NL;
  s += '#### Arguments' + NL + NL + regList(c.f.in, 'None.') + NL;
  s += '#### Result' + NL + NL + regList(c.f.out, 'Nothing, but the carry: C = 0 for success.') + NL;
  if (c.f.doc) s += '#### Description' + NL + NL + stop(prose(c.f.doc)) + NL + NL;
  s += '#### Errors' + NL + NL + errorTable(c.f.errors) + NL;
  s += '#### From the languages' + NL + NL + languages(c) + NL;
  const mates = calls.filter(o => o.group === c.group && o !== c).map(o => '`' + o.name + '`');
  if (mates.length) s += '#### See also' + NL + NL + 'The other calls for ' + g + ': ' + mates.join(', ') + '.' + NL;
  return s.replace(/\s+$/, '') + NL;
}

let out = intro + NL;
out += '## The calls by group' + NL + NL + '| Group | From | Calls |' + NL + '|---|---|---|' + NL;
for (const [g, v] of Object.entries(groups)) {
  const mine = calls.filter(c => c.group === g);
  if (mine.length) out += '| ' + cap(GROUPS[g] || g) + ' | ' + hex4(v.base) + ' | ' + mine.map(c => '`' + c.name + '`').join(' ') + ' |' + NL;
}
out += NL;
for (const c of calls.slice().sort((a, b) => a.name < b.name ? -1 : 1)) out += entry(c) + NL;

// ---- The constants, by family (the letters before their first _), and the error codes
out += '## Constants' + NL + NL + 'Every constant that `hydra.inc` defines for programs, by family. A value with a `$` is hexadecimal.' + NL + NL;
const fam = new Map();
for (const c of consts) {
  const k = /^([A-Z0-9]+)_/.exec(c.name) ? RegExp.$1 : c.name;
  if (!fam.has(k)) fam.set(k, []);
  fam.get(k).push(c);
}
out += '| Name | Value | Meaning |' + NL + '|---|---|---|' + NL;
let lastWhat = '';
for (const [, list] of fam) for (const c of list) {
  const what = c.what || '';
  out += '| `' + c.name + '` | ' + c.value + ' | ' + prose(what).replace(/\|/g, '\\|') + ' |' + NL;
  lastWhat = what;
}
out += NL + '## Error codes' + NL + NL + 'A call that fails returns with C = 1 and one of these in A. `ERRSTR` gives the message.' + NL + NL;
out += '| Code | Name | Message |' + NL + '|---|---|---|' + NL;
for (const [name, e] of Object.entries(errors)) out += '| $' + e.code + ' | `' + name + '` | ' + e.text + ' |' + NL;

fs.writeFileSync(path.join(ROOT, 'manual/reference/r3-calls.md'), out);
console.log('r3-calls.md: ' + calls.length + ' calls (' + Object.keys(own).length + ' written by hand), ' + consts.length + ' constants, ' + Object.keys(errors).length + ' error codes');
