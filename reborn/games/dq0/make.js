// make.js - Dingus Quest Zero's data files, after its build (build.js calls build): its overlays' file packed
// (obj/games/dq0.ovp, ovlpack.js's: ld65's cut after the last overlay that has code, each bank's fill left out), so
// the game reads no more than it needs as it starts; and once every game's built (build.js calls after), its stand-in
// themes (themes/*.txt, in their files' order: the format's in the first) made obj/games/dq0t.dat, THEME_SIZE bytes a
// theme (theme.h's struct theme, then its words), each one compiled and checked by the game's one theme compiler,
// themec (games/themec, C: the theme checker's every goal reachable, every faction placed), in the emulator
// (tools/themec.js).  A game's own theme, its history's, is themegen's (games/themegen), made as the game begins in
// the same format and compiled by the same themec into the same record.
'use strict';
const fs = require('fs');
const path = require('path');

// The headers' numbers (the build's, from the C as cc65 lays it out: hubgen.js), and struct theme's layout (its
// fields' offsets: T ('file'))
const H = require('./hubgen.js').load(), C = H.C;
const { THEME_SIZE } = C;                                           // theme.h's
const HEAD = H.size('theme');                                       // (struct theme's bytes, its words after)
const T = p => H.off('theme', p), LEN = (s, p) => H.size(s, p);

// A theme's file read: its lines, each its key and its values (a line begun with a space goes on with the last: its
// last value, or with a bar first, more values: ../../tools/dq0build.js's keyLines)
const lines = (text, f) => require('../../tools/dq0build.js').keyLines(text, f);

// The dispatch table's rows (actions.c's ROW (VB_*, ...)) held to actions.h's actions (the build's, from
// commands.actions): one a verb, in its order
function rowsSame(dir) {
  const want = H.enums.find(e => e[0] === 'VB_NONE').filter(n => n !== 'NACTIONS');
  const c = fs.readFileSync(path.join(dir, 'actions.c'), 'latin1').replace(/\/\*[\s\S]*?\*\//g, ' ');
  const have = [...c.matchAll(/\bROW[RP]?\s*\(\s*(VB_\w+)/g)].map(m => m[1]);   // (ROWR: a row with its refusals, stage 2;
                                                                                //   ROWP: and its request, stage 3)
  const at = want.findIndex((n, k) => have[k] !== n);
  if (at >= 0 || have.length !== want.length) throw new Error('games/dq0/actions.c\'s rows and actions.h differ' +
    (at >= 0 ? ': ' + want[at] + ' (actions.c has ' + (have[at] || 'none') + ')' : ': ' + have.length + ' rows'));
}

function build({ dir, objdir, say = () => {} }) {
  rowsSame(dir);
  return { overlays: require('./ovlpack.js').pack(objdir, 'dq0').banks };
}

// The stand-in themes, by themec in the emulator (tools/themec.js: once every game's built, themec too), each with its
// file's name (for popgen to read it: theme.h's file) and the count (the file's first's)
function after({ dir, objdir, bios, say = () => {} }) {
  const files = fs.readdirSync(path.join(dir, 'themes')).filter(t => t.endsWith('.txt')).sort();
  const made = require('../../tools/themec.js').compile(files.map(t => ({ name: t, text: fs.readFileSync(path.join(dir, 'themes', t), 'latin1') })), { bios });
  const bad = made.filter(t => t.status || !t.data || t.data.length !== THEME_SIZE);
  if (bad.length) throw new Error('the theme compiler (themec):\n  ' + bad.map(t => t.name + ': ' + (t.said || 'no record')).join('\n  '));
  made.forEach((t, i) => {
    if (files[i].length >= LEN('theme', 'file')) throw new Error(files[i] + ': a stand-in\'s name, 15 letters at most');
    t.data[T('count')] = files.length;
    t.data.write(files[i], T('file'), 'latin1');
  });
  fs.writeFileSync(path.join(objdir, '..', 'dq0t.dat'), Buffer.concat(made.map(t => t.data)));
  say('dq0t.dat: ' + files.length + ' stand-in themes, by themec in the emulator (' + made.map(t => t.words).join(', ') + ' bytes of words)');
  return { themes: files.length, words: made.map(t => t.words) };
}

// The stand-ins as themec makes each alone (dq0t.dat's records, their count 1 and no file's name: the tests', to hold
// a theme compiled on the Hydra to the build's)
function standIns(objdir) {
  const all = fs.readFileSync(path.join(objdir, 'dq0t.dat')), out = [];
  for (let k = 0; k < all.length; k += THEME_SIZE) {
    const r = Buffer.from(all.subarray(k, k + THEME_SIZE));
    r[T('count')] = 1;
    r.fill(0, T('file'), T('file') + LEN('theme', 'file'));
    out.push(r);
  }
  return out;
}

module.exports = { build, after, standIns, lines, THEME_SIZE, HEAD };
