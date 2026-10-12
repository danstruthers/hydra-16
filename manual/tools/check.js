#!/usr/bin/env node
// check.js - the book's examples held against the Hydra: each ```session block run again in the emulator
// (capture.js) and compared with what the book says; --fix puts what the Hydra printed in the book's place.
//
// Usage: node manual/tools/check.js [--fix] [--list] [FILE.md[:LINE] ...]   (no files: book.txt's chapters)
//   A block's lines after a prompt are typed, one by one; the rest is what the Hydra printed.  An HTML comment just
//   before the block (no blank line in it) can say more:
//     <!-- session OPTIONS            capture.js's options for it (--boot --modules 1 --rtc ...); and its lines
//     setup line ...                  typed first, unseen: their part of the transcript isn't the block's
//     -->
//     <!-- session-script OPTIONS     the whole of what's typed (capture.js's script: !raw, !wait ...), the block
//     line ...                        being the transcript from its start (or, with --boot, the boot's)
//     -->
//   A line "!end TEXT" in the comment ends the block at the line with TEXT in it (the rest isn't the book's).
//   A block's fence can name more prompts (```session * for ed's); a «...» line is a note, not output; a line
//   "» text" is typed with no prompt before it (the lines ed is given to add, a program's input).
//   --list shows each block's script, without running it.
'use strict';
const fs = require('fs');
const path = require('path');
const { capture } = require('./capture.js');

const BOOK = path.join(__dirname, '..');
const PROMPTS = [/^(\/[^ ]*> )/, /^(% )/, /^(> )/, /^(hylang> )/, /^(\]\s?)/, /^(\* )/, /^(db> )/, /^( {8})(?=\S)/];   // (8 spaces: the tab, HyForth's and rc's second prompt)
const BARE = /^(\/[^ ]*>|%|>|hylang>|\]|\*|db>)\s*$/;           // A prompt alone: the session's end

// The blocks in a chapter: { start, end (the fence lines), lines, prompts, opts, setup, script }
function blocks(text) {
  const L = text.replace(/\r\n?/g, '\n').split('\n'), out = [];
  for (let i = 0; i < L.length; i++) {
    const m = /^```session(?:\s+(.*))?$/.exec(L[i]);
    if (!m) continue;
    let j = i + 1;
    while (j < L.length && L[j] !== '```') j++;
    const b = { start: i, end: j, lines: L.slice(i + 1, j), opts: '', setup: [], script: null };
    b.prompts = (m[1] ? m[1].split(/\s+/).map(p => new RegExp('^(' + p.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace(/_/g, ' ') + ')')) : []).concat(PROMPTS);
    // The comment before it (straight before, or a blank line between)
    let k = i - 1;
    if (k >= 0 && !L[k].trim()) k--;
    if (k >= 0 && L[k].trim() === '-->') {
      let s = k;
      while (s >= 0 && !L[s].startsWith('<!--')) s--;
      const head = /^<!--\s*(session-script|session)\b(.*)$/.exec(L[s] || '');
      if (head) {
        b.opts = head[2].trim();
        let body = L.slice(s + 1, k);
        const end = body.find(l => l.startsWith('!end '));
        if (end) { b.endText = end.slice(5); body = body.filter(l => l !== end); }
        if (head[1] === 'session-script') b.script = body; else b.setup = body;
      }
    }
    out.push(b);
    i = j;
  }
  return out;
}

// What a block types: its lines after a prompt, and its \u00bb lines (typed with no prompt: !line)
function typed(b) {
  const t = [];
  for (const l of b.lines) {
    if (l.startsWith('\u00ab')) continue;
    if (l.startsWith('\u00bb ')) { t.push('!line ' + l.slice(2)); continue; }
    for (const p of b.prompts) { const m = p.exec(l); if (m) { t.push(l.slice(m[1].length)); break; } }
  }
  return t;
}

// The transcript's parts, a part a prompt's line and what follows it
function parts(lines, prompts) {
  const out = [];
  for (const l of lines) {
    if (prompts.some(p => p.test(l)) || BARE.test(l)) out.push([l]);
    else if (out.length) out[out.length - 1].push(l);
    else out.push([l]);
  }
  return out;
}

function run(b) {
  const o = { boot: false, vera: false, smc: false, rtc: false, modules: 3, sd: true, cycles: 4000, raw: false, file: '-' };
  // A block typed at rc's prompt starts rc first (unless its setup does)
  if (!b.script && b.lines.length && b.lines[0].startsWith('% ') && !b.setup.some(l => /^rc\b/.test(l))) b.setup = ['rc'].concat(b.setup);
  const script = ['#! ' + b.opts].concat(b.script || b.setup.concat(typed(b))).join('\n');
  let lines = capture(script, Object.assign({}, o)).split('\n');
  while (lines.length && !lines[0].trim()) lines.shift();
  if (!b.script && b.setup.length) {
    // Drop the setup's parts: one a line typed, from the first prompt's
    const ps = parts(lines, b.prompts);
    const first = typed(b)[0];
    let k = 0, n = 0;
    while (k < ps.length && n < b.setup.length) { if (b.prompts.some(p => p.test(ps[k][0]))) n++; k++; }
    while (k < ps.length && first !== undefined && !ps[k][0].endsWith(first)) k++;
    lines = [].concat(...ps.slice(k));
  }
  // The block's end: at !end's text, if it has one; a bare prompt last only if the book's has one
  if (b.endText) { const e = lines.findIndex(l => l.includes(b.endText)); if (e >= 0) lines = lines.slice(0, e + 1); }
  const hasBare = b.lines.length && BARE.test(b.lines[b.lines.length - 1]);
  while (lines.length && !lines[lines.length - 1].trim()) lines.pop();
  if (!hasBare && lines.length && BARE.test(lines[lines.length - 1])) lines.pop();
  return lines.map(l => l.replace(/\s+$/, ''));
}

function main() {
  const argv = process.argv.slice(2);
  const fix = argv.includes('--fix'), list = argv.includes('--list');
  let files = argv.filter(a => !a.startsWith('--'));
  if (!files.length) files = fs.readFileSync(path.join(BOOK, 'book.txt'), 'utf8').split('\n')
    .map(l => l.trim()).filter(l => l && !l.startsWith('#') && l.endsWith('.md'));
  let bad = 0, n = 0;
  for (const spec of files) {
    const [f, at] = spec.split(':');
    const file = path.isAbsolute(f) ? f : fs.existsSync(f) ? f : path.join(BOOK, f);
    let text = fs.readFileSync(file, 'utf8');
    const bs = blocks(text).filter(b => !at || b.start + 1 <= +at && +at <= b.end + 1);
    const fixes = [];
    for (const b of bs) {
      n++;
      const where = path.basename(file) + ':' + (b.start + 1);
      if (list) { console.log(where + '  ' + (b.opts || '') + '\n  ' + (b.script || b.setup.concat(typed(b))).join('\n  ')); continue; }
      const want = b.lines.filter(l => !l.startsWith('\u00ab')).map(l => l.replace(/^\u00bb /, '').replace(/\s+$/, ''));
      const got = run(b);
      if (want.join('\n') === got.join('\n')) { console.log(where + '  ok'); continue; }
      bad++;
      console.log(where + '  DIFFERS');
      const max = Math.max(want.length, got.length);
      for (let i = 0; i < max; i++) if (want[i] !== got[i]) console.log('    book:  ' + (want[i] ?? '(none)') + '\n    hydra: ' + (got[i] ?? '(none)'));
      // (The » lines marked again, in order)
      const raws = b.lines.filter(l => l.startsWith('» ')).map(l => l.slice(2).replace(/\s+$/, ''));
      fixes.push({ b, got: got.map(l => raws.length && l === raws[0] ? '» ' + raws.shift() : l) });
    }
    if (fix && fixes.length) {
      const L = text.replace(/\r\n?/g, '\n').split('\n');
      for (const { b, got } of fixes.reverse()) L.splice(b.start + 1, b.end - b.start - 1, ...got);
      fs.writeFileSync(file, L.join('\n'));
      console.log('fixed ' + fixes.length + ' in ' + path.basename(file));
    }
  }
  if (!list) console.log(n + ' blocks, ' + bad + ' differ' + (fix && bad ? ' (fixed)' : ''));
  process.exitCode = bad && !fix ? 1 : 0;
}

if (require.main === module) main();
module.exports = { blocks, typed };
