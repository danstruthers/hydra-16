// ****************************************************************************
// dq0build.js - what Dingus Quest Zero's make.js files share (ENGINE-CRITIQUE.md's section 9, step 11): the readers of
// its data files' formats, and a task's make.js that only packs its overlays.
//
//   keyLines (text, f, extra)  a "key | value | value" file's lines (the theme format's, themegen's history.txt and
//                              goals.txt, dq0d's kinds and factions): [{ key, v (its values, trimmed), n (its line),
//                              ...extra }]; a blank line or one begun with # left out; a line begun with a space goes
//                              on with the last: its last value, or, begun with a bar, more values.  Throws ("f: line
//                              n: goes on with nothing") at one with no line before it
//   blocks (file, first)       a file's keyLines in blocks, each begun by a line of key first: [{ n (its first line),
//                              f: { key: values } }] (dq0d's kinds.txt and the others)
//   rows (file, enc)           a file's lines but the blank ones and the notes, each its values between bars, trimmed
//   packOnly (name)            a make.js whose build packs the task's overlays' file (obj/games/NAME.ovp: games/dq0/
//                              ovlpack.js's) and nothing more: { build }
'use strict';
const fs = require('fs');
const path = require('path');

function keyLines(text, f, extra = {}) {
  const out = [];
  text.split(/\r?\n/).forEach((l, i) => {
    if (!l.trim() || l.startsWith('#')) return;
    if (/^\s/.test(l)) {
      if (!out.length) throw new Error(`${f}: line ${i + 1}: goes on with nothing`);
      const v = out[out.length - 1].v, t = l.trim();
      if (t.startsWith('|')) v.push(...t.slice(1).split('|').map(s => s.trim()));   // (More values)
      else v[v.length - 1] += ' ' + t;
      return;
    }
    const v = l.split('|').map(s => s.trim());
    out.push(Object.assign({ key: v.shift(), v, n: i + 1 }, extra));
  });
  return out;
}

function blocks(file, first) {
  const out = [];
  for (const l of keyLines(fs.readFileSync(file, 'latin1'), file)) {
    if (l.key === first) out.push({ n: l.n, f: {} });
    else if (!out.length) throw new Error(`${file}: line ${l.n}: ${l.key} before the first ${first}`);
    out[out.length - 1].f[l.key] = l.v;
  }
  return out;
}

const rows = (file, enc = 'latin1') => fs.readFileSync(file, enc).split(/\r?\n/).filter(l => l.trim() && !l.startsWith('#'))
  .map(l => l.split('|').map(s => s.trim()));

const packOnly = name => ({ build: ({ objdir }) => ({ overlays: require(path.join(__dirname, '..', 'games', 'dq0', 'ovlpack.js')).pack(objdir, name).banks }) });

module.exports = { keyLines, blocks, rows, packOnly };
