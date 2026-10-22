// commandwords.js - what a command means to Dingus Quest Zero: its words (games/dq0/commands.words) and its verbs
// (commands.verbs: each verb's words, the action it makes by the phrase's shape, its default, its own value) made the
// game's command (hub.h's struct cmd: an action of actions.h's, VB_*, and its roles), as the parser's task does it
// (games/dq0c, cmdmean.s, from the line's tokens: each as typed and as its alias's root), so the host has the
// reference the 65C02's is held to; and their tables, for dq0c.dat.
//   node sim/tools/commandwords.js "a line" ...     each line's struct cmd (facing north)
// From Node: load() the words and verbs; tokens(line) the line's tokens as dq0c makes them; scan(toks, words) its
// struct cmd as dq0c gives it (ways as the character faces kept relative: REL | n); finish(c, face) as the UI makes it
// of that (the ways made the compass's, a half turn: main.c's cq_cmd); encode(words, entries) the tables for dq0c.dat
// (cmdmean.s's); actionName(n) an action's name.
'use strict';
const fs = require('fs');
const path = require('path');

const GAME = path.join(__dirname, '..', '..', 'games', 'dq0');
const REL = 0x10, PART = 0x80, NONE = 0xFF;
const CF_NOVERB = 1, CF_HALF = 2, CF_UNSAID = 4, CF_STRANGE = 8;   // (hub.h's struct cmd's flags)
const RF_ORDER = 1, RF_ASPECTS = 2;                           // (A verb's own flags: an ally's request, its order read;
                                                              //   a thing's aspects taken, commands.words' [aspects])

// The names' values: each enum's names in order (dq0.h, hub.h, level.h, items.h, rs.h, actions.h), and the #defines
// that are numbers
function names() {
  const v = {};
  for (const f of ['dq0.h', 'hub.h', 'level.h', 'items.h', 'rs.h', 'actions.h']) {
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
    for (const [, k, x] of t.matchAll(/^#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)\s*$/gm)) if (v[k] === undefined) v[k] = +x;
  }
  return v;
}

// The actions' names by number (actions.h's enum: VB_NONE first)
let ACTIONS = null;
function actionName(n) {
  if (!ACTIONS) {
    const V = names();
    ACTIONS = [];
    for (const k of Object.keys(V)) if (k.startsWith('VB_')) ACTIONS[V[k]] = k;
  }
  return ACTIONS[n] || String(n);
}

const UNSAID = { none: 'DF_NONE', place: 'DF_PLACE', last: 'DF_LAST', near: 'DF_NEAR', front: 'DF_FRONT', target: 'DF_TARGET',
  hand: 'DF_HAND' };
const FIELDS = new Set(['alone', 'thing', 'way', 'unsaid', 'aux', 'skill', 'task']);

// The words and the verbs: commands.words' sections (a word or words = its meaning, in order), commands.verbs' verbs
// (rows: 1 on, 0 none) and their words
function load(file = path.join(GAME, 'commands.words'), vfile = path.join(GAME, 'commands.verbs')) {
  const V = names();
  const num = (s, what) => {
    const n = /^\d+$/.test(s) ? +s : V[s];
    if (n === undefined) throw new Error(what + ': no value for ' + s);
    return n;
  };
  const val = s => {
    s = s.trim();
    let flag = 0;
    if (s.startsWith('REL ')) { flag = REL; s = s.slice(4); }
    if (s.endsWith(' PART')) { flag = PART; s = s.slice(0, -5); }
    const m = s.match(/^(SL_NTH) (\d)$/);                      // (The second: SL_NTH, 2 in the top four bits)
    if (m) return num(m[1], 'commands.words') | +m[2] << 4;
    const n = /^\d+$/.test(s) ? +s : V[s];
    if (n === undefined && !/^[A-Z_]+$/.test(s)) throw new Error('commands.words: no value for ' + s);
    return n === undefined ? s : n | flag;
  };
  const w = { verbs: [], rows: [null], rowOf: new Map(), orders: new Map(), dirs: new Map(), things: new Map(),
    preps: new Map(), how: new Map(), special: new Map(), selectors: new Map(), numbers: new Map(), manners: new Map(),
    aspects: new Map(), little: new Set(), V };
  let sec = null;
  for (const raw of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
    const l = raw.replace(/#.*$/, '').trim();
    if (!l) continue;
    const s = l.match(/^\[(\w+)\]$/);
    if (s) { sec = s[1]; continue; }
    if (sec === 'little') { w.little.add(l); continue; }
    const [k, x] = l.split('=').map(t => t.trim());
    if (!k || x === undefined) throw new Error('commands.words: ' + l);
    if (w[sec] instanceof Map) w[sec].set(k, val(x));
    else throw new Error('commands.words: no section ' + sec);
  }
  sec = null;
  for (const raw of fs.readFileSync(vfile, 'utf8').split(/\r?\n/)) {
    const l = raw.replace(/#.*$/, '').trim();
    if (!l) continue;
    const s = l.match(/^\[(\w+)\]$/);
    if (s) { sec = s[1]; continue; }
    if (sec === 'verbs') {
      const [name, ...fs_] = l.split(/\s+/);
      const r = { name, alone: 0, thing: 0, way: 0, unsaid: 0, aux: 0, skill: 0, task: 0, flags: 0, preps: new Map() };
      for (const f of fs_) {
        const [k, x] = f.split('=');
        if (x === undefined) throw new Error('commands.verbs: ' + name + ': ' + f);
        if (k === 'unsaid') { if (!UNSAID[x]) throw new Error('commands.verbs: ' + name + ': unsaid=' + x); r.unsaid = num(UNSAID[x], 'commands.verbs'); }
        else if (k === 'aspects') r.flags |= RF_ASPECTS;     // (A thing's aspects said after it: "examine the value of")
        else if (k === 'skill') r.skill = num(x, 'commands.verbs') + 1;
        else if (FIELDS.has(k)) r[k] = num(x, 'commands.verbs');
        else r.preps.set(k, num(x, 'commands.verbs'));          // (A preposition's own action for it)
        if (k === 'aux' && x === 'SW_ORDER') r.flags |= RF_ORDER;
      }
      if (!r.alone && !r.thing && !r.way) throw new Error('commands.verbs: ' + name + ' makes no action');
      if (w.rowOf.has(name)) throw new Error('commands.verbs: ' + name + ' twice');
      w.rowOf.set(name, w.rows.length);
      w.rows.push(r);
    } else if (sec === 'words') {
      const [k, x] = l.split('=').map(t => t.trim());
      if (!w.rowOf.has(x)) throw new Error('commands.verbs: ' + k + ' = ' + x + ', no such verb');
      w.verbs.push({ words: k.split(/ +/), v: w.rowOf.get(x) });
    } else throw new Error('commands.verbs: no section ' + sec);
  }
  w.verbs.sort((a, b) => (b.words.length > 1) - (a.words.length > 1));   // (Several words first, each's order kept)
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

const thing = () => ({ kind: 0, sel: 0, n: 0, lies: NONE, x: 0, y: 0, word: '' });

// A line's struct cmd from its tokens: the verb (its first words), the words after it (the old parser's rules, o_parse:
// parse.c's, gone), then the action its verb makes of the shape (commands.verbs: a thing said, and the word straight
// after the verb; a way; nothing), its default, its own value.  The ways as the character faces are kept REL | n, but
// where one by the compass is with one: face, then.  dq0c gives the same
function scan(toks, w, face = 0) {
  const V = w.V, R = w.rowOf;
  const ws = toks.filter(t => t.k === 'w' && t.typed !== '!' && t.typed !== '=');
  const c = { action: 0, actor: 0, flags: 0, dir: NONE, how: 0, prep: 0, aux: 0, manner: 0, order: 0, dflt: 0, skill: 0, task: 0,
    n: 0, obj: thing(), other: thing() };
  if (toks.length && toks[0].k === 'q') { c.action = V.VB_SAY; c.flags |= CF_NOVERB; }   // (Quoted text alone: said aloud)
  if (!ws.length) return c;
  const text = t => t.multi ? t.root : t.typed;
  const get = (t, m) => { const a = m.get(text(t)); return a !== undefined || t.multi ? a : m.get(t.root); };
  const sp = (i, k) => get(ws[i], w.special) === k;
  const little = t => w.little.has(text(t)) || (!t.multi && w.little.has(t.root));
  const two = (i, m) => i + 1 < ws.length ? m.get(text(ws[i]) + ' ' + text(ws[i + 1])) : undefined;
  let i = 0, dir = NONE, k, verb = 0, around = 0, me = 0;
  let word = '', word2 = '';
  for (const e of w.verbs) {                   // The verb, if the line starts with one: several words first
    if (e.words.length <= ws.length && e.words.every((x, j) => text(ws[j]) === x)) { verb = e.v; i = e.words.length; break; }
  }
  if (!verb && !ws[0].multi) {                 // (Its alias's root: an alias the player made)
    const e = w.verbs.find(e => e.words.length === 1 && e.words[0] === ws[0].root);
    if (e) { verb = e.v; i = 1; }
  }
  const order = () => (w.rows[verb].flags & RF_ORDER) !== 0;
  const head = () => !c.prep && !order() && verb !== R.get('name') && verb !== R.get('combine');   // (A later word the head)
  if (verb) c.order = order() ? get(ws[0], w.orders) || 0 : 0;
  else { verb = R.get('move'); c.flags |= CF_NOVERB; }   // (A way, or a thing, alone: going there)
  const first = i;                             // (The word straight after the verb)
  let aspect = 0;                              // (A thing's aspects said: EX_*, the verb's own if none)
  for (; i < ws.length; ++i) {
    const t = ws[i];
    const th = word && c.prep ? c.other : c.obj;   // (The thing being said: the other, after its preposition)
    const said = word || c.obj.sel;              // (A thing said: a word, or one chosen, "it")
    const other = () => {                        // (A word for the other thing: a name given the last)
      if (verb === R.get('name') || ((c.prep || order() || verb === R.get('combine')) && !word2)) word2 = t.typed.slice(0, 15);
    };
    if (order() && (k = get(t, w.orders))) c.order = c.order || k;
    if ((w.rows[verb].flags & RF_ASPECTS) && (k = get(t, w.aspects)) !== undefined) { aspect |= k; continue; }
    if (sp(i, 'WITH')) { c.prep = V.PR_WITH; continue; }
    if (two(i, w.special) === 'WAY_OUT') { c.obj.kind = V.OB_WAY_OUT; ++i; continue; }
    if (said && !c.prep && (k = get(t, w.preps)) !== undefined) { c.prep = k; continue; }
    if (verb === R.get('put') && sp(i, 'AWAY')) { verb = R.get('stow'); continue; }
    if (verb === R.get('put') && sp(i, 'BACK')) continue;
    if ((k = get(t, w.how)) !== undefined) { c.how = k; continue; }
    if (sp(i, 'HALF')) { c.flags |= CF_HALF; continue; }
    if (sp(i, 'AROUND')) { around = 1; continue; }
    if ((k = get(t, w.selectors)) !== undefined) {
      if ((k & 15) === V.SL_ME) me = 1;            // ("me": the character, if nothing else is said)
      else { th.sel = k & 15; th.n = k >> 4; }
      continue;
    }
    if ((k = get(t, w.manners)) !== undefined) { c.manner |= k; continue; }
    if (little(t)) continue;
    if ((/^[0-9]+$/.test(t.typed) || get(t, w.numbers) !== undefined) && !c.n) {   // (A number: squares ...)
      c.n = Math.min(255, /^[0-9]+$/.test(t.typed) ? +t.typed : get(t, w.numbers));
      continue;
    }
    if ((k = get(t, w.dirs)) !== undefined) {
      const vert = d => d === V.GO_UP || d === V.GO_DOWN;
      dir = dir === NONE || vert(k) || vert(dir) ? k : between(dir, k, face);
      continue;
    }
    if ((k = get(t, w.things)) !== undefined) {
      if ((k & PART) && c.prep === V.PR_WITH) { other(); continue; }   // (A part after "with": what it's done with, "hit the
                                                //   door with the bar", not the door's)
      c.obj.kind = k & ~PART;
      if ((k & PART) && (!word || head())) word = t.typed.slice(0, 15);
      continue;
    }
    if (i === 0 && (c.flags & CF_NOVERB)) c.flags |= CF_STRANGE;   // (The line's first, with no verb: a verb unknown)
    if (c.prep !== V.PR_WITH && (word ? head() : !c.prep || !c.obj.sel)) word = t.typed.slice(0, 15);   // (A word not
                                                // understood: the thing's, its last before a preposition its head;
                                                // after "it" and one, the other's)
    else other();
  }
  // The old parser's rules, on what the words made
  if (c.obj.kind) c.obj.lies = dir; else c.dir = dir;   // (With a thing, its way is where it lies)
  if (verb === R.get('doff') && !c.obj.kind && dir !== NONE) verb = R.get('move');
  if (verb === R.get('hang')) c.prep = V.PR_ON;
  if (verb === R.get('stow') && word2) verb = R.get('put');
  if (verb === R.get('put') && !word2 && dir === V.GO_DOWN) verb = R.get('drop');
  if (verb === R.get('put') && w.special.get(word) === 'BAR' && c.obj.kind === V.OB_DOOR && !word2) verb = R.get('bar');
  if (verb === R.get('doff') && c.prep === V.PR_FROM && word2) verb = R.get('get');
  else if (verb === R.get('get') && c.prep === V.PR_FROM && !word2) verb = R.get('doff');
  const said = word || c.obj.sel;
  if (verb === R.get('turn') && said && !word2 && c.prep === V.PR_ON) { verb = R.get('light'); c.prep = 0; }
  else if (verb === R.get('turn') && said && !word2 && c.prep === V.PR_FROM) { verb = R.get('douse'); c.prep = 0; }
  if (verb === R.get('put') && said && !word2 && c.prep === V.PR_FROM) { verb = R.get('douse'); c.prep = 0; }
  if (verb === R.get('get') && said && c.dir === V.GO_UP) { verb = R.get('get'); c.dir = NONE; }   // ("pick it up")
  if ((verb === R.get('turn') || verb === R.get('face')) && around && dir === NONE) c.dir = REL | 4;
  if (verb === R.get('name') && !word2) { word2 = word; word = ''; }    // ("call it Grub")
  if (verb === R.get('move') && (c.obj.lies === V.GO_UP || c.obj.lies === V.GO_DOWN) &&   // ("go down the stairs": a way
      [V.OB_STAIRS, V.OB_LADDER, V.OB_SHAFT].includes(c.obj.kind)) c.dir = c.obj.lies;    //   down them, not to them)
  if (c.prep === V.PR_WITH && verb !== R.get('combine') && !word) { word = word2; word2 = ''; }
  c.obj.word = word;
  c.other.word = word2;
  if (verb === R.get('throw') && c.how === V.HOW_TO && !c.prep && (c.obj.word || c.obj.sel) && !c.other.word) {
    c.other = c.obj;                           // ("throw at the rat": the mark; the thing thrown what's in hand)
    c.obj = thing();
    c.prep = V.PR_AT;
  }
  if (me && !c.obj.word && !c.obj.kind && !c.obj.sel && c.dir === NONE) c.obj.sel = V.SL_ME;   // ("examine me")
  // The action its verb makes of the shape: a thing said (a preposition straight after the verb its own), a way, none
  const r = w.rows[verb];
  c.aux = aspect || r.aux;                     // (The aspects said, else the verb's own)
  c.skill = r.skill;
  c.task = r.task;
  c.dflt = r.unsaid;
  let p;                                       // (The verb's own preposition straight after it, the little words passed)
  for (let j = first; j < ws.length && p === undefined; ++j) {
    p = r.preps.get(text(ws[j]));
    if (p === undefined && !little(ws[j])) break;
  }
  if (c.dir !== NONE && verb === R.get('move') && c.obj.kind) c.action = r.way;   // (Up or down them: the way's)
  else if (c.obj.word || c.obj.kind || c.obj.sel) {
    c.action = p !== undefined ? p : r.thing || r.alone || r.way;
  } else if (c.dir !== NONE) c.action = r.way || r.thing || r.alone;
  else if (r.alone) c.action = r.alone;
  else { c.action = r.thing || r.way; c.flags |= CF_UNSAID; }   // (Its thing unsaid: its default the UI's to fill)
  return c;
}

// The tables dq0c's meaning reads (cmdmean.s), its part of dq0c.dat (in a bank of dq0c's own): magic "DQ0W", version
// 3; the constants it needs (CONSTANTS' values, a byte each: a verb's its number); the dictionary's entry count (the
// index's, u16), and a byte an entry: its record (0: none, no word of the game's); the records' count (record 0 the
// none's among them), and their fields, a column each, a byte a record (COLUMNS: 0 in each the none's; the way's
// + 1, so $FF's none is 0 too); the verbs of several words, in their order (a count, the entries, the verb), all
// before any of one word's; the pairs (two entries, 1 a special's, the value); the verbs (a count, then each one's
// ROW bytes: its actions alone, with a thing, with a way, its unsaid, aux, skill, task and flags), and the
// prepositions' own actions (a count, then each: the verb, the word's entry, the action)
const CONSTANTS = ['move', 'put', 'stow', 'doff', 'hang', 'drop', 'bar', 'get', 'turn', 'face', 'light', 'douse', 'unlock',
  'name', 'combine', 'PR_WITH', 'PR_ON', 'PR_FROM', 'OB_WAY_OUT', 'OB_DOOR', 'GO_UP', 'GO_DOWN', 'SL_ME', 'VB_SAY', 'OB_STAIRS',
  'OB_LADDER', 'OB_SHAFT', 'throw', 'HOW_TO', 'PR_AT'];
const SPECIAL = { WITH: 1, AWAY: 2, BACK: 3, HALF: 4, AROUND: 5, BAR: 6, WAY_OUT: 7 };   // (cmdmean.s's S_*)
const COLUMNS = ['verb', 'dir', 'thing', 'prep', 'how', 'special', 'order', 'sel', 'num', 'manner', 'little', 'aspect'];   // (cmdmean.s's C_*)
const ROW = ['alone', 'thing', 'way', 'unsaid', 'aux', 'skill', 'task', 'flags'];            // (cmdmean.s's R_*)
const tokenWord = x => /^[a-z0-9][a-z0-9'-]*$/.test(x);     // (A word dq0c's tokenizer can make: not "?")

// Every word of the game's (each of a key's, and the verbs' prepositions), for the index's dictionary
function gameWords(w) {
  const out = new Set();
  const add = k => k.split(' ').filter(tokenWord).forEach(x => out.add(x));
  w.verbs.forEach(e => e.words.forEach(add));
  w.rows.slice(1).forEach(r => [...r.preps.keys()].forEach(add));
  for (const m of [w.orders, w.dirs, w.things, w.preps, w.how, w.special, w.selectors, w.numbers, w.manners, w.aspects]) for (const k of m.keys()) add(k);
  w.little.forEach(add);
  return [...out];
}

function encode(w, entries) {
  const V = w.V, entryOf = new Map(entries.map((x, i) => [x, i]));
  const ent = x => { const e = entryOf.get(x); if (e === undefined) throw new Error('dq0c.dat: ' + x + ' is in no entry of the dictionary'); return e; };
  const u16 = n => [n & 255, n >> 8];
  const byte = (n, max, what) => { if (!(n >= 0 && n <= max)) throw new Error('dq0c.dat: ' + what + ' ' + n + ' out of range'); return n; };
  const rec = new Map();
  const r = x => { if (!rec.has(x)) rec.set(x, { verb: 0, dir: NONE, thing: 0, prep: 0, how: 0, little: 0, special: 0, order: 0, sel: 0, num: 0, manner: 0, aspect: 0 }); return rec.get(x); };
  const multi = [], two = [];
  let single = false;
  for (const e of w.verbs) {
    if (!e.words.every(tokenWord)) continue;
    if (e.words.length > 1) {
      if (single) throw new Error('commands.verbs: ' + e.words.join(' ') + ', a verb of several words, after one of one word');
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
      else if (ks.length === 2 && pair) two.push([...u16(ent(ks[0])), ...u16(ent(ks[1])), pair, SPECIAL[v]]);
      else throw new Error('commands.words: ' + k + ', a key dq0c cannot hold');
    }
  };
  each(w.dirs, (x, v) => { x.dir = byte(v, 254, 'way'); });
  each(w.things, (x, v) => { x.thing = byte(v, 255, 'thing'); });
  each(w.preps, (x, v) => { x.prep = byte(v, 7, 'prep'); });
  each(w.how, (x, v) => { x.how = byte(v, 3, 'how'); });
  each(w.orders, (x, v) => { x.order = byte(v, 7, 'order'); });
  each(w.special, (x, v) => { x.special = byte(SPECIAL[v], 7, 'special ' + v); }, 1);
  each(w.selectors, (x, v) => { x.sel = byte(v, 255, 'selector'); });
  each(w.numbers, (x, v) => { x.num = byte(v, 255, 'number'); });
  each(w.manners, (x, v) => { x.manner = byte(v, 255, 'manner'); });
  each(w.aspects, (x, v) => { x.aspect = byte(v, 255, 'aspect'); });
  for (const k of w.little) if (tokenWord(k)) r(k).little = 1;
  const none = COLUMNS.map(() => 0), recs = [none], recOf = new Map([[none.join(','), 0]]), gwi = new Array(entries.length).fill(0);
  for (const [k, x] of rec) {
    const b = COLUMNS.map(c => (c === 'dir' ? (x.dir + 1) & 255 : x[c]));
    const key = b.join(',');
    if (!recOf.has(key)) { recOf.set(key, recs.length); recs.push(b); }
    gwi[ent(k)] = recOf.get(key);
  }
  if (recs.length > 255 || multi.length > 255 || two.length > 42 || w.rows.length > 255) throw new Error("dq0c.dat: too many of the game's words");
  const k = CONSTANTS.map(n => {
    const x = w.rowOf.has(n) ? w.rowOf.get(n) : V[n];
    if (x === undefined) throw new Error('dq0c.dat: no value for ' + n);
    return byte(x, 255, n);
  });
  const rows = w.rows.slice(1).map(x => ROW.map(f => byte(x[f], 255, x.name + "'s " + f)));
  const preps = [];
  w.rows.forEach((x, n) => { if (x) for (const [p, a] of x.preps) preps.push([n, ...u16(ent(p)), byte(a, 255, 'action')]); });
  if (preps.length > 63) throw new Error("dq0c.dat: too many of the verbs' prepositions");
  return Buffer.from([0x44, 0x51, 0x30, 0x57, 3, ...k, ...u16(entries.length), ...gwi,
    recs.length, ...COLUMNS.flatMap((c, i) => recs.map(b => b[i])), multi.length, ...multi.flat(), two.length, ...two.flat(),
    w.rows.length - 1, ...rows.flat(), preps.length, ...preps.flat()]);
}

// What the UI makes of it as the command's carried out: the ways as the character faces made the compass's, and a half
// turn (an eighth: half left, half right)
function finish(c, face, V) {
  const res = d => d === NONE || !(d & REL) ? d : (face + (d & 7)) & 7;
  c = Object.assign({}, c, { dir: res(c.dir), obj: Object.assign({}, c.obj, { lies: res(c.obj.lies) }) });
  if (c.action === V.VB_FACE_DIRECTION && (c.flags & CF_HALF) && c.dir < V.GO_UP)
    c.dir = ((c.dir - face) & 7) === 6 ? (face + 7) & 7 : ((c.dir - face) & 7) === 2 ? (face + 1) & 7 : c.dir;
  return c;
}

// A line's struct cmd, as the UI has it (facing face): its tokens (the default aliases), the meaning, finished
function meaning(line, w, face = 0) {
  const { DEFAULT_ALIASES, DEFAULT_ANYWHERE } = require('./commandcompile.js');
  return finish(scan(tokens(line, DEFAULT_ALIASES, DEFAULT_ANYWHERE), w, face), face, w.V);
}

// A struct cmd in a line of words (its fields that aren't empty)
function show(c) {
  const t = (x, n) => [x.kind && 'kind ' + x.kind, x.sel && 'sel ' + x.sel + (x.n ? '/' + x.n : ''), x.lies !== NONE && 'lies ' + x.lies,
    x.word && JSON.stringify(x.word)].filter(Boolean).map(s => n + ' ' + s);
  return [actionName(c.action), c.flags && 'flags ' + c.flags, c.dir !== NONE && 'dir ' + c.dir, c.how && 'how ' + c.how,
    c.prep && 'prep ' + c.prep, c.aux && 'aux ' + c.aux, c.manner && 'manner ' + c.manner, c.order && 'order ' + c.order, c.n && 'n ' + c.n,
    ...t(c.obj, 'obj'), ...t(c.other, 'other')].filter(Boolean).join(', ');
}

module.exports = { load, tokens, scan, finish, between, meaning, names, gameWords, encode, actionName, show, CONSTANTS, SPECIAL,
  COLUMNS, ROW, REL, PART, NONE, CF_NOVERB, CF_HALF, CF_UNSAID, CF_STRANGE };

if (require.main === module) {
  const w = load();
  for (const l of process.argv.slice(2)) console.log(JSON.stringify(l) + ' ' + show(meaning(l, w)));
}
