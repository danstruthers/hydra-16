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
const B = require('../../tools/dq0build.js'), H = B.hub(), C = H.C;
const { THEME_SIZE } = C;                                           // theme.h's
const HEAD = H.size('theme');                                       // (struct theme's bytes, its words after)
const T = p => H.off('theme', p), LEN = (s, p) => H.size(s, p);

// A theme's file read: its lines, each its key and its values (a line begun with a space goes on with the last: its
// last value, or with a bar first, more values: ../../tools/dq0build.js's keyLines)
const lines = (text, f) => B.keyLines(text, f);

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

// The dispatch table's coverage report (COMMANDS-CRITIQUE.md's §4.4, step L's last, 2026-11-15): every verb's cell for
// each kind of thing a command may name, read from its row (actions.c's ROW, ROWR, ROWP): a thing by its word, each part
// of the place (OB_*, dq0.h).  A cell is the row's handler's (H, the verb does it: its row's takes,
// masks[], say which parts), refused by the row (U_*, refuse.c's words, before the handler's asked), or the handler's
// own (-: the table says nothing, the handler answers as it will; the cells the table doesn't cover yet).  Written to
// obj/games/dq0cover.txt, a line a verb; the build's log the sums.  Nothing fails on it: a report
function coverage(dir, objdir) {
  const src = fs.readFileSync(path.join(dir, 'actions.c'), 'latin1').replace(/\/\*[\s\S]*?\*\//g, ' ');
  const OB = H.enums.find(e => e[0] === 'OB_NONE'), U = H.enums.find(e => e[0] === 'U_VERB');
  const bits = t => /^0x/i.test(t.trim()) ? parseInt(t, 16) : [...t.matchAll(/BIT\s*\(\s*(OB_\w+)\s*\)/g)].reduce((m, x) => m | 1 << OB.indexOf(x[1]), 0);
  const defs = {};
  for (const m of src.matchAll(/#define\s+(\w+)\s+(\(BIT[^\n]*\)|0x[0-9A-Fa-f]+)u?/g)) defs[m[1]] = bits(m[2]);
  const masks = /masks\[\]\s*=\s*\{([^}]*)\}/.exec(src)[1].split(',').map(t => t.trim()).map(t => t in defs ? defs[t] : bits(t));
  const M = /enum\s*\{\s*(M_NONE[^}]*)\}/.exec(src)[1].split(',').map(t => t.trim());
  const W = { W_CLIMB: 0x80, W_PART: 0x40 };
  const val = t => { t = (t || '0').trim(); let v = 0;   // (A row's field: names or'd, a refusal shifted)
    for (const p of t.split('|')) { const q = p.replace(/[()]/g, '').trim(), sh = /^(\w+)\s*>>\s*(\d+)$/.exec(q);
      v |= sh ? (W[sh[1]] || 0) >> +sh[2] : /^\d+$/.test(q) ? +q : U.includes(q) ? U.indexOf(q) : M.includes(q) ? M.indexOf(q) : 0; }
    return v; };
  const rows = [...src.matchAll(/\bROW[RP]?\s*\((.*)\)[\s,]*$/gm)].map(m => m[1].split(',').map(t => t.trim())).filter(r => /^VB_/.test(r[0]));
  const names = ['word', ...OB.slice(1).map(n => n.slice(3).toLowerCase())], u = n => U[n].slice(2).toLowerCase();
  const sum = { H: 0, U: 0, '-': 0 }, out = ['# The dispatch table\'s cells (games/dq0/make.js\'s coverage): a verb a line, its handler, then each kind',
    '# of thing named: H its handler\'s; yet, cant, note refused by its row (U_YET, U_CANT, U_NOTE); - the handler\'s own',
    '# ' + names.join(' ')];
  for (const [a, h, t, w, k, m] of rows) {
    const cells = [];
    if (h === 'H_POWERS' || h === 'H_NOTHING') {   // (Refused whatever's named; or seen to before the table)
      out.push(a + ' ' + h + ' ' + (h === 'H_POWERS' ? 'refused, whatever\'s named' : 'seen to before the table'));
      continue;
    }
    const wv = val(w) & 3, kv = val(k), mask = masks[val(m)] || 0;
    cells.push(wv ? u(wv) : '-');
    for (let o = 1; o < names.length; ++o) cells.push(mask >> o & 1 ? 'H' : kv ? u(kv) : '-');
    for (const c of cells) sum[c === 'H' || c === '-' ? c : 'U']++;
    out.push(a + ' ' + h + ' ' + cells.join(' '));
  }
  fs.writeFileSync(path.join(objdir, '..', 'dq0cover.txt'), out.join('\n') + '\n');
  return 'the dispatch table: ' + rows.length + ' verbs, ' + (sum.H + sum.U + sum['-']) + ' cells: ' + sum.H + ' its handlers\', ' + sum.U +
    ' refused by their rows, ' + sum['-'] + ' the handlers\' own (obj/games/dq0cover.txt)';
}

function build({ dir, objdir, say = () => {} }) {
  rowsSame(dir);
  say(coverage(dir, objdir));
  return { overlays: B.pack(objdir, 'dq0') };
}

// The stand-in themes, by themec in the emulator (tools/themec.js: once every game's built, themec too), each with its
// file's name (for popgen to read it: theme.h's file) and the count (the file's first's)
function after({ dir, objdir, bios, say = () => {} }) {
  const files = fs.readdirSync(path.join(dir, 'themes')).filter(t => t.endsWith('.txt')).sort();
  const made = B.themes(files.map(t => ({ name: t, text: fs.readFileSync(path.join(dir, 'themes', t), 'latin1') })), { bios }, THEME_SIZE);
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
