// ****************************************************************************
// dq0build.js - what Dingus Quest Zero's build shares (ENGINE-CRITIQUE.md's sections 1, 2 and 9, step 11): the readers
// of its data files' formats, and the one place its generators are driven from, by build.js and the make.js files
// alike (each generator's own work in its file: games/dq0/hubgen.js, saidgen.js, parts.js, pack.js, ovlpack.js,
// tools/themec.js).
//
// The readers:
//   keyLines (text, f, extra)  a "key | value | value" file's lines (the theme format's, themegen's history.txt and
//                              goals.txt, dq0d's kinds and factions): [{ key, v (its values, trimmed), n (its line),
//                              ...extra }]; a blank line or one begun with # left out; a line begun with a space goes
//                              on with the last: its last value, or, begun with a bar, more values.  Throws ("f: line
//                              n: goes on with nothing") at one with no line before it
//   blocks (file, first)       a file's keyLines in blocks, each begun by a line of key first: [{ n (its first line),
//                              f: { key: values } }] (dq0d's kinds.txt and the others)
//   rows (file, enc)           a file's lines but the blank ones and the notes, each its values between bars, trimmed
// The generators:
//   generate ({ say, cc65 })   the build's step before the games (build.js's hubNumbers): the headers' numbers, the
//                              messages' and the records' headers (hubgen.js: obj/gen's actions.h, dq0hub.json,
//                              dq0msg.h, dq0rec.h, dq0hub_*.inc), then the phrase table (saidgen.js: dq0said.h) and the
//                              styles file (styles, below: before the games, whose themes themec compiles with it).
//                              OUT: hub ()'s
//   styles (H, say)            the styles' proportions, games/dq0/styles.txt's sets, made obj/games/styles.dat (the card's
//                              lib/dq0/styles.dat: the maker's to read, the set its theme names; themec's, a set's name
//                              found): its head (the sets, sizeof struct style, the styles a set (level.h's ST_*), a
//                              name's bytes), each set's name (themec's, read alone), then each set's four struct styles
//                              (theme.h's, laid out as cc65 has it: H's).  A set begun from another (its line's third value) a copy of it,
//                              but for the rows it gives; one begun from none gives every field (but slant, the
//                              chunk's, 0).  OUT: the sets' names
//   hub ()                     the headers' numbers, as generate made them (hubgen.js's load: obj/gen/dq0hub.json)
//   swappedParts (dir, objdir, extra, b)
//                              a task's C with swapped parts (its dir has a parts.txt) compiled and placed: build.js's
//                              cprog's, its tools b = { cc65 (args), od65 (args): its words, compileC (files, objdir,
//                              extra, asExtra), csources (dir), flags (cflags ()'s) }.  Its C compiled (cc65), its calls
//                              routed by part (pack.js's route: a call into another part its stub's, one inside a part
//                              straight), assembled with its assembly but bank.s; each part's bytes (od65's); a part
//                              past the swap area (its config's __SWAPSIZE__) has pieces moved out to parts with room
//                              (pack.js's plan, apply: the moves listed in OBJDIR/moved.txt, for the room table), its
//                              files assembled again; then its parts.inc and config (parts.js's make: the parts in
//                              banks by their sizes) and bank.s, its .include of parts.inc found in objdir.  OUT: the
//                              objects, in the sources' order
//   pack (objdir, name)        a task's overlays' file packed (ovlpack.js: obj/games/NAME.ovp, ld65's cut after the last
//                              overlay that has code, each bank's fill left out).  OUT: its banks
//   packOnly (name)            a make.js whose build packs the task's overlays' file and nothing more: { build }
//   themes (list, opt, size)   themes ({ name, text }) compiled and checked by the game's one theme compiler, themec, in
//                              the emulator (tools/themec.js's compile, opt its: bios, out); throws with each that
//                              failed and why (its status not 0; with size, its record not size bytes).  OUT: compile's
'use strict';
const fs = require('fs');
const path = require('path');

const DQ0 = path.join(__dirname, '..', 'games', 'dq0');

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

// ---- The generators

function generate({ say = () => {}, cc65 }) {
  const H = require(path.join(DQ0, 'hubgen.js')).generate({ say, cc65 });
  require(path.join(DQ0, 'saidgen.js')).generate(H, say);
  styles(H, say);
  return H;
}

function styles(H, say = () => {}) {
  const file = path.join(DQ0, 'styles.txt'), out = path.join(DQ0, '..', '..', 'obj', 'games', 'styles.dat');
  const ST = H.words('ST_BUILT').length, SIZE = H.size('style'), NAME = H.C.STYLE_NAME, MAX = H.C.STYLE_SETS;   // (theme.h's)
  const F = Object.entries(H.S.style.f).filter(([k]) => !k.includes('.')).map(([k, f]) => ({ k, at: f[0], n: f[2] ? f[1] / f[2] : 1 }));
  const sets = [];
  for (const l of keyLines(fs.readFileSync(file, 'latin1'), file)) {
    const bad = what => { throw new Error(`games/dq0/styles.txt: line ${l.n}: ${what}`); };
    if (l.key === 'set') {
      const from = l.v[1] ? sets.find(s => s.name === l.v[1]) : null;
      if (!l.v[0] || l.v[0].length >= NAME || sets.some(s => s.name === l.v[0])) bad('a set\'s name, ' + (NAME - 1) + ' letters at most, its own');
      if (l.v[1] && !from) bad('no set ' + l.v[1] + ' before it');
      sets.push({ name: l.v[0], n: l.n, b: from ? Buffer.from(from.b) : Buffer.alloc(SIZE * ST), from, given: new Set() });
      continue;
    }
    const s = sets[sets.length - 1], f = F.find(x => x.k === l.key);
    if (!s) bad('a row before the first set');
    if (!f || f.k === 'slant') bad('no field ' + l.key + ' of struct style\'s (theme.h)');
    if (l.v.length !== ST) bad(ST + ' columns (built, natural, mine, mixed), not ' + l.v.length);
    l.v.forEach((col, st) => {
      const w = col.split(/\s+/).filter(Boolean);
      if (w.length !== f.n) bad(f.k + ': ' + f.n + ' value(s) a style, not ' + w.length);
      w.forEach((x, i) => {
        const v = /^\d+$/.test(x) ? +x : H.C['KI_' + x.toUpperCase()];
        if (v === undefined || v > 255) bad(f.k + ': ' + x + '?');
        s.b[st * SIZE + f.at + i] = v;
      });
    });
    s.given.add(f.k);
  }
  if (!sets.length || sets.length > MAX) throw new Error('games/dq0/styles.txt: ' + sets.length + ' sets, not 1 to ' + MAX);
  for (const s of sets) {
    const miss = s.from ? [] : F.filter(f => f.k !== 'slant' && !s.given.has(f.k)).map(f => f.k);
    if (miss.length) throw new Error(`games/dq0/styles.txt: line ${s.n}: set ${s.name}: no ${miss.join(', ')}`);
  }
  const names = sets.map(s => { const b = Buffer.alloc(NAME); b.write(s.name, 'latin1'); return b; });
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, Buffer.concat([Buffer.from([sets.length, SIZE, ST, NAME]), ...names, ...sets.map(s => s.b)]));
  say('styles.dat: ' + sets.length + ' set(s) of styles (' + sets.map(s => s.name).join(', ') + ')');
  return sets.map(s => s.name);
}

const hub = () => require(path.join(DQ0, 'hubgen.js')).load();

function swappedParts(dir, objdir, extra = [], b) {
  const P = require(path.join(DQ0, 'parts.js')), K = require(path.join(DQ0, 'pack.js'));
  fs.mkdirSync(objdir, { recursive: true });
  K.planChecked();
  const src = b.csources(dir), cs = src.filter(f => f.endsWith('.c')), ss = src.filter(f => f.endsWith('.s'));
  const sOf = f => path.join(objdir, path.basename(f, '.c') + '.s'), o = new Map();
  for (const f of cs) b.cc65([...b.flags.cc, ...extra, '-o', sOf(f), f]);
  const compiled = cs.map(sOf), rest = ss.filter(f => path.basename(f) !== 'bank.s'), bank = ss.filter(f => path.basename(f) === 'bank.s');
  const asm = () => b.compileC(compiled, objdir).forEach((x, i) => o.set(cs[i], x));
  K.route(compiled);
  asm();
  b.compileC(rest, objdir, extra, ['-I', objdir]).forEach((x, i) => o.set(rest[i], x));
  const dump = () => b.od65(['--dump-segsize', ...o.values()]);
  let text = dump();
  const cfgText = fs.readFileSync(path.join(dir, path.basename(dir) + '.cfg'), 'latin1');
  const cap = parseInt((/__SWAPSIZE__:[^;]*value\s*=\s*\$([0-9A-Fa-f]+)/.exec(cfgText) || [])[1] || '0', 16);
  const list = P.read(path.join(dir, 'parts.txt')), bytes = new Map(list.parts.map(p => [p.n, 0]));
  const count = () => { for (const k of bytes.keys()) bytes.set(k, 0); for (const [s, z] of P.segsizes(text)) { const m = /^SW(\d+)(CODE|RODATA)$/.exec(s); if (m) bytes.set(+m[1], (bytes.get(+m[1]) || 0) + z); } };
  count();
  let moves = [];
  const squeeze = new Map((process.env.DQ0_PACK_SQUEEZE || '').split(';').map(t => t.split(':')).filter(([t]) => t === path.basename(dir))   // (A test's:
    .flatMap(([, l]) => (l || '').split(',').map(x => x.split('=').map(Number))));   //   DQ0_PACK_SQUEEZE=TASK:PART=BYTES,...;... parts given less room, the packer made to move)
  const room = k => Math.min(cap, squeeze.has(k) ? squeeze.get(k) : cap);
  if (cap && [...bytes].some(([k, n]) => n > room(k))) {
    const depth = +((/^SWDEPTH\s*=\s*(\d+)/m.exec(bank.length ? fs.readFileSync(bank[0], 'latin1') : '') || [])[1] || 8);
    const r = K.pieces({ lsts: compiled.map(f => f.replace(/\.s$/, '.lst')), asm: rest, sizes: K.objectSizes(text) });
    const bk = P.banks(cfgText, list, P.segsizes(text), dir);   // (The banks' room and files, the parts in them as they are)
    moves = K.plan(r, bytes, room, depth, new Set(bytes.keys()), bk);
    K.apply(moves, objdir);
    K.route(compiled);
    asm();
    text = dump();
    count();
  }
  fs.writeFileSync(path.join(objdir, 'moved.txt'), moves.map(m => m.piece.id.replace(/^_/, '') + ' ' + m.from + ' ' + m.to + ' ' + m.piece.bytes + '\n').join(''));
  P.make({ dir, objdir, own: ss, sizes: P.segsizes(text), compiled });
  b.compileC(bank, objdir, extra, ['-I', objdir]).forEach((x, i) => o.set(bank[i], x));
  return src.map(f => o.get(f));
}

const pack = (objdir, name) => require(path.join(DQ0, 'ovlpack.js')).pack(objdir, name).banks;

const packOnly = name => ({ build: ({ objdir }) => ({ overlays: pack(objdir, name) }) });

function themes(list, opt, size) {
  const made = require('./themec.js').compile(list, opt);
  const bad = made.filter(t => t.status || (size && (!t.data || t.data.length !== size)));
  if (bad.length) throw new Error('the theme compiler (themec):\n  ' + bad.map(t => t.name + ': ' + (t.said || 'no record')).join('\n  '));
  return made;
}

module.exports = { keyLines, blocks, rows, generate, styles, hub, swappedParts, pack, packOnly, themes };
