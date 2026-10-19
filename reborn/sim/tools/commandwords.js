// commandwords.js - what a command means to Dingus Quest Zero: its words (games/dq0/commands.words) made the game's
// struct pcmd (dq0.h), as the parser's task does it (games/dq0c, dq0c.s's meaning, from the line's tokens: each as typed
// and as its alias's root), so the host has the reference the 65C02's is held to; and their tables, for dq0c.dat.
//   node sim/tools/commandwords.js "a line" ...     each line's struct pcmd (facing north)
// From Node: load() the words; tokens(line) the line's tokens as dq0c makes them; scan(toks, words) its struct pcmd as
// dq0c gives it (ways as the character faces kept relative: REL | n); finish(c, face) as the UI makes it of that (the
// ways made the compass's, a half turn: main.c's cq_cmd); encode(words, entries) the tables for dq0c.dat (cmdmean.s's).
'use strict';
const fs = require('fs');
const path = require('path');

const GAME = path.join(__dirname, '..', '..', 'games', 'dq0');
const REL = 0x10, PART = 0x80, NONE = 0xFF;

// The names' values: each enum's names in order (dq0.h, hub.h, level.h), and the #defines that are numbers
function names() {
  const v = {};
  for (const f of ['dq0.h', 'hub.h', 'level.h']) {
    const t = fs.readFileSync(path.join(GAME, f), 'latin1').replace(/\/\*[\s\S]*?\*\//g, ' ');
    for (const [, body] of t.matchAll(/enum\s*\{([^}]*)\}/g)) {
      let n = 0;
      for (let item of body.split(',')) {
        item = item.trim();
        if (!item) continue;
        const m = item.match(/^(\w+)\s*(?:=\s*(\w+))?$/);
        if (!m) continue;
        if (m[2] !== undefined) n = /^\d+$/.test(m[2]) ? +m[2] : (v[m[2]] !== undefined ? v[m[2]] : n);
        v[m[1]] = n++;
      }
    }
    for (const [, k, x] of t.matchAll(/^#define\s+(\w+)\s+(\d+)\s*$/gm)) if (v[k] === undefined) v[k] = +x;
  }
  return v;
}

// The words: each section's lines (a word or words = its meaning), in order
function load(file = path.join(GAME, 'commands.words')) {
  const V = names();
  const val = s => {
    s = s.trim();
    let flag = 0;
    if (s.startsWith('REL ')) { flag = REL; s = s.slice(4); }
    if (s.endsWith(' PART')) { flag = PART; s = s.slice(0, -5); }
    const n = /^\d+$/.test(s) ? +s : V[s];
    if (n === undefined && !/^[A-Z_]+$/.test(s)) throw new Error('commands.words: no value for ' + s);
    return n === undefined ? s : n | flag;
  };
  const w = { verbs: [], orders: new Map(), dirs: new Map(), things: new Map(), preps: new Map(), how: new Map(),
    special: new Map(), styles: new Map(), little: new Set(), V };
  let sec = null;
  for (const raw of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
    const l = raw.replace(/#.*$/, '').trim();
    if (!l) continue;
    const s = l.match(/^\[(\w+)\]$/);
    if (s) { sec = s[1]; continue; }
    if (sec === 'little') { w.little.add(l); continue; }
    const [k, x] = l.split('=').map(t => t.trim());
    if (!k || x === undefined) throw new Error('commands.words: ' + l);
    if (sec === 'verbs') w.verbs.push({ words: k.split(/ +/), v: val(x) });
    else if (w[sec] instanceof Map) w[sec].set(k, val(x));
    else throw new Error('commands.words: no section ' + sec);
  }
  return w;
}

// A line's tokens as dq0c's tokenizer makes them (cmdmatch.s): words, ! and =, quoted text; then the aliases (a verb's
// at a command's start, a way's anywhere; the longest key first): each token its word as typed, the root its alias
// gives, and whether that alias was several words (its root then the token's meaning: "look closely" is x)
function tokens(line, aliases, anywhere) {
  const out = [];
  let s = line;
  while (s.length) {
    const c = s[0];
    if (c === '"' || ((c === 'w' || c === 'y') && s[1] === '"')) {
      const b = s.indexOf('"', c === '"' ? 1 : 2);
      out.push({ k: 'q', typed: '', root: '', multi: false });
      s = b < 0 ? '' : s.slice(b + 1);
      continue;
    }
    const m = s.match(/^(?:#[A-Za-z0-9'-]*|[A-Za-z0-9][A-Za-z0-9'-]*)/);   // (# a thing named's id: "#12")
    if (m) { const t = m[0].toLowerCase(); out.push({ k: 'w', typed: t, root: t, multi: false }); s = s.slice(m[0].length); continue; }
    if (c === '!' || c === '=') out.push({ k: 'w', typed: c, root: c, multi: false });
    s = s.slice(1);
  }
  for (let t = 0; t < out.length; t++) {
    if (out[t].k !== 'w') continue;
    const seg = t === 0 || out[t - 1].root === 'then';
    let best = 0, verb = null;
    for (const [key, v] of aliases) {
      if (!seg && !anywhere.has(key)) continue;
      const kw = key.split(' ');
      if (kw.length <= best) continue;
      if (kw.every((x, j) => out[t + j] && out[t + j].k === 'w' && out[t + j].typed === x)) { best = kw.length; verb = v; }
    }
    if (!verb) continue;
    out[t].root = verb;
    out[t].multi = best > 1;
    out.splice(t + 1, best - 1);
  }
  return out;
}

// Two ways together: the way between them (a quarter apart: the eighth between), or the second.  One by the compass and
// one as it faces: that one made the compass's first (face)
function between(a, b, face) {
  if ((a & REL) !== (b & REL)) {
    if (a & REL) a = (face + (a & 7)) & 7;
    else b = (face + (b & 7)) & 7;
  }
  const f = a & b & REL, x = a & 7, y = b & 7;
  if (((y - x) & 7) === 2) return f | ((x + 1) & 7);
  if (((x - y) & 7) === 2) return f | ((y + 1) & 7);
  return f | y;
}

// A line's struct pcmd from its tokens (the old parser's rules, o_parse: parse.c's, gone, on the words; the ways as the
// character faces kept REL | n, but where one by the compass is with one: face, then).  dq0c gives the same
function scan(toks, w, face = 0) {
  const V = w.V;
  const ws = toks.filter(t => t.k === 'w' && t.typed !== '!' && t.typed !== '=');
  const c = { verb: 0, dir: NONE, obj: 0, orel: NONE, how: 0, half: 0, around: 0, word: '', prep: 0, word2: '', order: 0 };
  if (!ws.length) return c;
  const text = t => t.multi ? t.root : t.typed;
  const get = (t, m) => { const a = m.get(text(t)); return a !== undefined || t.multi ? a : m.get(t.root); };
  const sp = (i, k) => get(ws[i], w.special) === k;
  const two = (i, m) => i + 1 < ws.length ? m.get(text(ws[i]) + ' ' + text(ws[i + 1])) : undefined;
  let i = 0, dir = NONE, k;
  for (const e of w.verbs) {                   // The verb, if the line starts with one: several words first
    if (e.words.length <= ws.length && e.words.every((x, j) => text(ws[j]) === x)) { c.verb = e.v; i = e.words.length; break; }
  }
  if (!c.verb && !ws[0].multi) {               // (Its alias's root: an alias the player made)
    const e = w.verbs.find(e => e.words.length === 1 && e.words[0] === ws[0].root);
    if (e) { c.verb = e.v; i = 1; }
  }
  if (c.verb) c.order = c.verb === V.V_TELL ? get(ws[0], w.orders) || 0 : 0;
  else c.verb = V.V_MOVE;                      // (A way, or a thing, alone: going there)
  for (; i < ws.length; ++i) {
    const t = ws[i];
    if (c.verb === V.V_TELL && (k = get(t, w.orders))) c.order = c.order || k;
    if (sp(i, 'WITH')) { c.prep = V.PR_WITH; continue; }
    if (two(i, w.special) === 'WAY_OUT') { c.obj = V.OB_WAY_OUT; ++i; continue; }
    if (c.word && !c.prep && (k = get(t, w.preps)) !== undefined) { c.prep = k; continue; }
    if (c.verb === V.V_PUT && sp(i, 'AWAY')) { c.verb = V.V_STOW; continue; }
    if (c.verb === V.V_PUT && sp(i, 'BACK')) continue;
    if ((k = get(t, w.how)) !== undefined) { c.how = k; continue; }
    if (sp(i, 'HALF')) { c.half = 1; continue; }
    if (sp(i, 'AROUND')) { c.around = 1; continue; }
    if (w.little.has(text(t)) || (!t.multi && w.little.has(t.root))) continue;
    if ((k = get(t, w.dirs)) !== undefined) {
      const vert = d => d === V.GO_UP || d === V.GO_DOWN;
      dir = dir === NONE || vert(k) || vert(dir) ? k : between(dir, k, face);
      continue;
    }
    if ((k = get(t, w.things)) !== undefined) {
      c.obj = k & ~PART;
      if ((k & PART) && !c.word) c.word = t.typed.slice(0, 15);
      continue;
    }
    if (!c.word && c.prep !== V.PR_WITH) c.word = t.typed.slice(0, 15);   // (The first word not understood)
    else if (c.verb === V.V_NAME || ((c.prep || c.verb === V.V_TELL || c.verb === V.V_COMBINE) && !c.word2)) c.word2 = t.typed.slice(0, 15);
  }
  if (c.obj) c.orel = dir; else c.dir = dir;   // (With a thing, its way is where it lies)
  if (c.verb === V.V_DOFF && !c.obj && dir !== NONE) c.verb = V.V_MOVE;
  if (c.verb === V.V_HANG || (c.verb === V.V_STOW && c.word2)) { c.prep = c.verb === V.V_HANG ? V.PR_ON : c.prep; c.verb = V.V_PUT; }
  if (c.verb === V.V_PUT && !c.word2 && dir === V.GO_DOWN) c.verb = V.V_DROP;
  if (c.verb === V.V_PUT && w.special.get(c.word) === 'BAR' && c.obj === V.OB_DOOR && !c.word2) c.verb = V.V_BAR;
  if (c.verb === V.V_DOFF && c.prep === V.PR_FROM && c.word2) c.verb = V.V_GET;
  else if (c.verb === V.V_GET && c.prep === V.PR_FROM && !c.word2) c.verb = V.V_DOFF;
  if (c.verb === V.V_TURN && c.word && !c.word2 && c.prep === V.PR_ON) { c.verb = V.V_LIGHT; c.prep = 0; }       // (new: the
  else if (c.verb === V.V_TURN && c.word && !c.word2 && c.prep === V.PR_FROM) { c.verb = V.V_DOUSE; c.prep = 0; } //   grammar's
  if (c.verb === V.V_PUT && c.word && !c.word2 && c.prep === V.PR_FROM) { c.verb = V.V_DOUSE; c.prep = 0; }      //   particles)
  if (c.verb === V.V_UNLOCK && c.word && c.dir === V.GO_UP) { c.verb = V.V_GET; c.dir = NONE; }
  if ((c.verb === V.V_TURN || c.verb === V.V_FACE) && c.around && dir === NONE) c.dir = REL | 4;
  if (c.verb === V.V_NAME && !c.word2) { c.word2 = c.word; c.word = ''; }    // ("call it Grub")
  if (c.prep === V.PR_WITH && c.verb !== V.V_COMBINE && !c.word) { c.word = c.word2; c.word2 = ''; }
  if (c.verb === V.V_ATTACK) {                 // (A blow: struck with what its verb says)
    k = two(0, w.styles);
    c.half = k !== undefined ? k : get(ws[0], w.styles) !== undefined ? get(ws[0], w.styles) : V.SK_BEST;
  }
  return c;
}

// The tables dq0c's meaning reads (cmdmean.s), its part of dq0c.dat: magic "DQ0W", version 1; the constants it needs
// (CONSTANTS' values, a byte each); the dictionary's entry count (the index's, u16), and a byte an entry: its record
// (0: none, no word of the game's); the records' count (record 0 the none's among them), and their fields, a column
// each, a byte a record (COLUMNS: 0 in each the none's; the way's + 1, so $FF's none is 0 too); the verbs of several
// words, in their order (a count, the entries, the verb), all before any of one word's; and the pairs (two entries,
// 1 a special's or 2 a style's, the value)
const CONSTANTS = ['V_MOVE', 'V_TELL', 'V_PUT', 'V_STOW', 'V_DOFF', 'V_HANG', 'V_DROP', 'V_BAR', 'V_GET', 'V_TURN', 'V_FACE',
  'V_LIGHT', 'V_DOUSE', 'V_UNLOCK', 'V_NAME', 'V_COMBINE', 'V_ATTACK', 'PR_WITH', 'PR_ON', 'PR_FROM', 'OB_WAY_OUT', 'OB_DOOR',
  'GO_UP', 'GO_DOWN', 'SK_BEST'];
const SPECIAL = { WITH: 1, AWAY: 2, BACK: 3, HALF: 4, AROUND: 5, BAR: 6, WAY_OUT: 7 };   // (cmdmean.s's S_*)
const COLUMNS = ['verb', 'dir', 'thing', 'prep', 'how', 'special', 'order', 'style', 'little'];   // (cmdmean.s's C_*)
const tokenWord = x => /^[a-z0-9][a-z0-9'-]*$/.test(x);     // (A word dq0c's tokenizer can make: not "?")

// Every word of the game's (each of a key's), for the index's dictionary
function gameWords(w) {
  const out = new Set();
  const add = k => k.split(' ').filter(tokenWord).forEach(x => out.add(x));
  w.verbs.forEach(e => e.words.forEach(add));
  for (const m of [w.orders, w.dirs, w.things, w.preps, w.how, w.special, w.styles]) for (const k of m.keys()) add(k);
  w.little.forEach(add);
  return [...out];
}

function encode(w, entries) {
  const V = w.V, entryOf = new Map(entries.map((x, i) => [x, i]));
  const ent = x => { const e = entryOf.get(x); if (e === undefined) throw new Error('dq0c.dat: ' + x + ' is in no entry of the dictionary'); return e; };
  const u16 = n => [n & 255, n >> 8];
  const byte = (n, max, what) => { if (!(n >= 0 && n <= max)) throw new Error('dq0c.dat: ' + what + ' ' + n + ' out of range'); return n; };
  const rec = new Map();
  const r = x => { if (!rec.has(x)) rec.set(x, { verb: 0, dir: NONE, thing: 0, prep: 0, how: 0, little: 0, special: 0, order: 0, style: 0 }); return rec.get(x); };
  const multi = [], two = [];
  let single = false;
  for (const e of w.verbs) {
    if (!e.words.every(tokenWord)) continue;
    if (e.words.length > 1) {
      if (single) throw new Error('commands.words: ' + e.words.join(' ') + ', a verb of several words, after one of one word');
      multi.push([e.words.length, ...e.words.flatMap(x => u16(ent(x))), byte(e.v, 255, 'verb')]);
    } else {
      single = true;
      if (!r(e.words[0]).verb) r(e.words[0]).verb = byte(e.v, 255, 'verb');
    }
  }
  const each = (m, fn, pair) => {
    for (const [k, v] of m) {
      const ks = k.split(' ');
      if (!ks.every(tokenWord)) continue;
      if (ks.length === 1) fn(r(k), v);
      else if (ks.length === 2 && pair) two.push([...u16(ent(ks[0])), ...u16(ent(ks[1])), pair, pair === 1 ? SPECIAL[v] : byte(v, 3, 'style')]);
      else throw new Error('commands.words: ' + k + ', a key dq0c cannot hold');
    }
  };
  each(w.dirs, (x, v) => { x.dir = byte(v, 254, 'way'); });
  each(w.things, (x, v) => { x.thing = byte(v, 255, 'thing'); });
  each(w.preps, (x, v) => { x.prep = byte(v, 7, 'prep'); });
  each(w.how, (x, v) => { x.how = byte(v, 3, 'how'); });
  each(w.orders, (x, v) => { x.order = byte(v, 7, 'order'); });
  each(w.special, (x, v) => { x.special = byte(SPECIAL[v], 6, 'special ' + v); }, 1);
  each(w.styles, (x, v) => { x.style = byte(v, 3, 'style'); }, 2);
  for (const k of w.little) if (tokenWord(k)) r(k).little = 1;
  const none = COLUMNS.map(() => 0), recs = [none], recOf = new Map([[none.join(','), 0]]), gwi = new Array(entries.length).fill(0);
  for (const [k, x] of rec) {
    const b = COLUMNS.map(c => (c === 'dir' ? (x.dir + 1) & 255 : x[c]));
    const key = b.join(',');
    if (!recOf.has(key)) { recOf.set(key, recs.length); recs.push(b); }
    gwi[ent(k)] = recOf.get(key);
  }
  if (recs.length > 255 || multi.length > 255 || two.length > 42) throw new Error("dq0c.dat: too many of the game's words");
  const k = CONSTANTS.map(n => { if (V[n] === undefined) throw new Error('dq0c.dat: no value for ' + n); return byte(V[n], 255, n); });
  return Buffer.from([0x44, 0x51, 0x30, 0x57, 1, ...k, ...u16(entries.length), ...gwi,
    recs.length, ...COLUMNS.flatMap((c, i) => recs.map(b => b[i])), multi.length, ...multi.flat(), two.length, ...two.flat()]);
}

// What the UI makes of it as the command's carried out: the ways as the character faces made the compass's, and a half
// turn (an eighth: half left, half right)
function finish(c, face, V) {
  const res = d => d === NONE || !(d & REL) ? d : (face + (d & 7)) & 7;
  c = Object.assign({}, c, { dir: res(c.dir), orel: res(c.orel) });
  if (c.verb === V.V_TURN && c.half && c.dir < V.GO_UP)
    c.dir = ((c.dir - face) & 7) === 6 ? (face + 7) & 7 : ((c.dir - face) & 7) === 2 ? (face + 1) & 7 : c.dir;
  return c;
}

// A line's struct pcmd, as the UI has it (facing face): its tokens (the default aliases), the meaning, finished
function meaning(line, w, face = 0) {
  const { DEFAULT_ALIASES, DEFAULT_ANYWHERE } = require('./commandcompile.js');
  return finish(scan(tokens(line, DEFAULT_ALIASES, DEFAULT_ANYWHERE), w, face), face, w.V);
}

module.exports = { load, tokens, scan, finish, between, meaning, names, gameWords, encode, CONSTANTS, SPECIAL, COLUMNS, REL, PART, NONE };

if (require.main === module) {
  const w = load();
  for (const l of process.argv.slice(2)) console.log(JSON.stringify(l) + ' ' + JSON.stringify(meaning(l, w)));
}
