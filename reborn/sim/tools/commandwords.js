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
const HUB = require('../../games/dq0/hubgen.js').load();     // (hub.h's numbers, the build's: obj/gen/dq0hub.json)
const REL = HUB.C.CQ_REL, PART = 0x80, NONE = 0xFF;           // (A way as the character faces: hub.h's CQ_REL)
const { CF_NOVERB, CF_HALF, CF_UNSAID, CF_STRANGE } = HUB.C;  // (hub.h's struct cmd's flags)
const CF_WORN = HUB.C.CF_WORN || 0x20, CF_LIST = HUB.C.CF_LIST || 0x40;   // (A place on the body; a list's later thing)
const RF_ORDER = 1, RF_ASPECTS = 2;                           // (A verb's own flags: an ally's request, its order read;
                                                              //   a thing's aspects taken, commands.words' [aspects])

// The names' values: the headers' enums' members and #defines that are numbers (dq0.h, hub.h and those it includes,
// actions.h), as the build's cc65 made them (games/dq0/hubgen.js: obj/gen/dq0hub.json)
function names() {
  return Object.assign({}, HUB.C);
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

// The words that make one kind another, each a name's word before its last (the catalogue's names, dq0w/kinds.c's; the
// compartments', its cnouns; the denizen kinds', dq0d/kinds.txt's): "iron" of "iron key", "stone" of "stone chisel",
// "kite", "black", "giant", "side", "secret" ...; and the old hand list's (nameis.s's, the themes' pieces' colours).  A
// name's word before its last is kept with it if it's one of these (the parser's "iron key"), else passed over ("the
// sleeping rat": a rat; HOUSE.md's 2026-11-04 rule)
const QUALS_OLD = ['black', 'brown', 'red', 'white', 'small', 'great', 'cave', 'giant', 'dwarf', 'kobold', 'iron', 'brass', 'broken'];
function qualWords(w) {
  const games = path.join(GAME, '..'), out = new Set(QUALS_OLD);
  const names = [];
  const kc = fs.readFileSync(path.join(games, 'dq0w', 'kinds.c'), 'latin1');
  for (const m of kc.slice(kc.indexOf('const struct kind kinds[')).matchAll(/^\s*\{ "([a-z ]*)"/gm)) names.push(m[1]);
  const cn = kc.match(/cnouns\[NCN\]\[CN_LEN\] = \{([^}]*)\}/);
  if (cn) for (const m of cn[1].matchAll(/"([a-z ]*)"/g)) names.push(m[1]);
  for (const l of fs.readFileSync(path.join(games, 'dq0d', 'kinds.txt'), 'latin1').split(/\r?\n/)) {
    if (!l.trim() || l.startsWith('#')) continue;
    const f = l.split('|');
    if (f.length > 5) names.push(f[0].trim());
  }
  for (const n of names) {
    const ws = n.split(' ').filter(x => x && !(w && w.little.has(x)));
    ws.slice(0, -1).forEach(x => out.add(x));
  }
  return [...out].filter(x => /^[a-z][a-z'-]*$/.test(x)).sort();
}

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
  w.quals = new Set(qualWords(w));
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
      out.push({ k: 'q', typed: '', root: '', multi: false, at: line.length - s.length + (c === '"' ? 1 : 2) });   // (at:
                                                // its first letter's place in the line)
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

// The roles a word may have in a line (the grammar's: commandcompile.js's ROLES, cmdmean.s's R_*); the hands said (struct
// cmd's hand: main.c's s_hand_said's)
const RO = { NONE: 0, OTHER: 1, TOOL: 2, HAND: 3, WORN: 4, LOC: 5, LIST: 6, SKIP: 7, MINE: 8 };
const HN = { LEFT: 1, RIGHT: 2, BOTH: 3, OTHER: 4, OFF: 5, MAIN: 6 };

// A line's struct cmd from its tokens and their roles (lab: the grammar's derivation's, commandcompile.js's rolesOf; none,
// a line that doesn't parse: each word the thing's): the verb (its first words); each word after it, by its role, the
// thing's or the other's (where it's put, what it's taken from or done with, ...), the hand said, a place on the body;
// each thing named by its last word and the one before it ("the iron key": "iron key"; "the side pocket"), a part of
// the place by its last ("the far side": the side, "far"); then the old parser's rules (o_parse: parse.c's, gone), and
// the action its verb makes of the shape (commands.verbs: a thing said, and the word straight after the verb; a way;
// nothing), its default, its own value.  The ways as the character faces are kept REL | n, but where one by the
// compass is with one: face, then.  dq0c gives the same
function scan(toks, w, face = 0, lab = null) {
  const V = w.V, R = w.rowOf;
  const ws = [];                               // (The words, each with its token's place: its role's)
  toks.forEach((t, j) => { if (t.k === 'w' && t.typed !== '!' && t.typed !== '=') ws.push(Object.assign({ ti: j }, t)); });
  const c = { action: 0, actor: 0, flags: 0, dir: NONE, how: 0, prep: 0, aux: 0, manner: 0, order: 0, dflt: 0, skill: 0, task: 0,
    n: 0, obj: thing(), other: thing(), hand: 0, quote: 0 };
  if (toks.length && toks[0].k === 'q') { c.action = V.VB_SAY; c.flags |= CF_NOVERB; }   // (Quoted text alone: said aloud)
  const q = toks.find(t => t.k === 'q');       // (Quoted text: where it is in the line, its first letter's place + 1)
  if (q) c.quote = Math.min(255, q.at + 1);
  if (!ws.length) return c;
  const text = t => t.multi ? t.root : t.typed;
  const get = (t, m) => { const a = m.get(text(t)); return a !== undefined || t.multi ? a : m.get(t.root); };
  const sp = (i, k) => get(ws[i], w.special) === k;
  const little = t => w.little.has(text(t)) || (!t.multi && w.little.has(t.root));
  const two = (i, m) => i + 1 < ws.length ? m.get(text(ws[i]) + ' ' + text(ws[i + 1])) : undefined;
  const role = i => lab ? lab[ws[i].ti] || 0 : 0;
  let i = 0, dir = NONE, k, verb = 0, around = 0, me = 0;
  const S = [{ th: c.obj, hd: -1, ql: -1, ro: 0 }, { th: c.other, hd: -1, ql: -1, ro: 0 }];   // (The two things: each one's
                                                //   last word and the one before it, and the role they were said in)
  let said = 0;                                // (A thing said: a word not a part of the place's, or one chosen, "it")
  for (const e of w.verbs) {                   // The verb, if the line starts with one: several words first
    if (e.words.length <= ws.length && e.words.every((x, j) => text(ws[j]) === x)) { verb = e.v; i = e.words.length; break; }
  }
  if (!verb && !ws[0].multi) {                 // (Its alias's root: an alias the player made)
    const e = w.verbs.find(e => e.words.length === 1 && e.words[0] === ws[0].root);
    if (e) { verb = e.v; i = 1; }
  }
  const order = () => (w.rows[verb].flags & RF_ORDER) !== 0;
  if (verb) c.order = order() ? get(ws[0], w.orders) || 0 : 0;
  else { verb = R.get('move'); c.flags |= CF_NOVERB; }   // (A way, or a thing, alone: going there)
  const first = i;                             // (The word straight after the verb)
  let aspect = 0;                              // (A thing's aspects said: EX_*, the verb's own if none)
  const content = (s, j, r) => {               // (A word of a thing's name: its last, the one before it kept; of a
    if (s.ro !== r) { s.hd = -1; s.ro = r; }   //   phrase said after another's, its own: "the dagger and the sword")
    s.ql = s.hd;
    s.hd = j;
    if (s === S[0] && (get(ws[j], w.things) === undefined || (get(ws[j], w.things) & PART))) said = 1;
  };
  for (; i < ws.length; ++i) {
    const t = ws[i], lb = role(i), r = lb & 7, mine = lb & RO.MINE;   // (Its role; mine, "my left arm")
    if (r === RO.SKIP) continue;
    if (r === RO.HAND) {                       // (The hand: "in my left hand", "in both hands", "in my off hand")
      const d = get(t, w.dirs), n = get(t, w.numbers), sl = get(t, w.selectors);
      k = d === (REL | 6) ? HN.LEFT : d === (REL | 2) ? HN.RIGHT : n === 2 ? HN.BOTH : sl !== undefined && (sl & 15) === V.SL_OTHER ?
        HN.OTHER : get(t, w.preps) === V.PR_FROM ? HN.OFF : sp(i, 'MAIN') ? HN.MAIN : 0;
      if (k) c.hand = k;
      continue;
    }
    const s = S[r ? 1 : 0], th = s.th;         // (The thing it's of: the other's, a role's)
    if (r === RO.WORN) c.flags |= CF_WORN;
    if (r === RO.LIST) c.flags |= CF_LIST;
    if (order() && !c.order && (k = get(t, w.orders))) { c.order = k; continue; }   // (A request's word, the first, not
                                                //   the other thing's: "tell Grub to fetch the key" the key, 2026-10-30)
    if ((w.rows[verb].flags & RF_ASPECTS) && (k = get(t, w.aspects)) !== undefined) { aspect |= k; continue; }
    if (sp(i, 'WITH')) { c.prep = V.PR_WITH; continue; }
    if (two(i, w.special) === 'WAY_OUT') { c.obj.kind = V.OB_WAY_OUT; ++i; continue; }
    if ((r ? r !== RO.LIST : said) && !c.prep && (k = get(t, w.preps)) !== undefined) { c.prep = k; continue; }   // (Its
                                                //   preposition: the first, after the thing said, or the other's own)
    if (r === RO.WORN || r === RO.LOC) {       // (A place on the body, "on my left hand"; or to hide, "behind the table":
      const d = get(t, w.dirs);                //   its side its name's, no way; its words of how and where no more)
      if (d === (REL | 6) || d === (REL | 2)) { content(s, i, r); continue; }
      if (d !== undefined || get(t, w.how) !== undefined || sp(i, 'AROUND') || little(t)) continue;
    }
    if (verb === R.get('put') && sp(i, 'AWAY')) { verb = R.get('stow'); continue; }
    if (verb === R.get('put') && sp(i, 'BACK')) continue;
    if ((k = get(t, w.how)) !== undefined) { c.how = c.how || k; continue; }   // (The first said: "go through the door to
                                                //   the east")
    if (sp(i, 'HALF')) { c.flags |= CF_HALF; continue; }
    if (sp(i, 'AROUND')) { around = 1; continue; }
    if ((k = get(t, w.selectors)) !== undefined) {
      if ((k & 15) === V.SL_ME) me = 1;            // ("me": the character, if nothing else is said)
      else { th.sel = k & 15; th.n = k >> 4; if (s === S[0]) said = 1; }
      continue;
    }
    if ((k = get(t, w.manners)) !== undefined) { c.manner |= k; continue; }
    if (little(t)) continue;
    if (/^[0-9]+$/.test(t.typed) || get(t, w.numbers) !== undefined) {   // (A number: squares ...; the first, the others
      if (!c.n) c.n = Math.min(255, /^[0-9]+$/.test(t.typed) ? +t.typed : get(t, w.numbers));   //   no name's word)
      continue;
    }
    if ((k = get(t, w.dirs)) === V.GO_DOWN && said && !c.prep && i + 1 < ws.length &&
      [R.get('drop'), R.get('put'), R.get('throw')].includes(verb)) { c.prep = V.PR_IN; continue; }   // ("Drop the dagger down the
                                                //   well": into it, a thing let fall down a shaft, a hole, 2026-11-06)
    if (k !== undefined && !(mine && (k === (REL | 6) || k === (REL | 2)))) {   // (A way; "my left": a side, the name's)
      const vert = d => d === V.GO_UP || d === V.GO_DOWN;
      dir = dir === NONE || vert(k) || vert(dir) ? k : between(dir, k, face);
      continue;
    }
    if ((k = get(t, w.things)) !== undefined && (k & ~PART) === V.OB_WAY_OUT) { c.obj.kind = V.OB_WAY_OUT; continue; }   // (The
                                                //   way out: the thing's, the puzzle's, "use the seal on the exit")
    if (k === undefined && get(t, w.preps) !== undefined) continue;   // (A preposition not the thing's: no name's word)
    if (k === undefined && i === 0 && (c.flags & CF_NOVERB)) c.flags |= CF_STRANGE;   // (The line's first, with no verb: a
                                                //   verb unknown)
    content(s, i, r);                          // (A word of its name, or a part of the place)
  }
  // Each thing's name: its last word and the one before it; a part of the place by its last (the kind), its word the
  // one before it ("the far side"), a part of a thing's its own ("the bars", "the iron bars"; "the window bars": the bars)
  let wrec = '';                               // (The thing's last word, for "bar")
  for (const s of S) {
    if (s.hd < 0) continue;
    const h = ws[s.hd], hk = get(h, w.things), qs = s.ql >= 0 ? ws[s.ql] : null;
    const both = qs && s.ql === s.hd - 1 && qs.typed.length + h.typed.length < 15 && (w.quals.has(qs.typed) ||
      get(qs, w.dirs) !== undefined) ? qs.typed + ' ' + h.typed : h.typed.slice(0, 15);   // (Its word just before it
                                                //   kept, if it makes one kind another or it's a side: "the iron key",
                                                //   "the side pocket", "my left arm"; "the sleeping rat" the rat, "the
                                                //   flask of oil" the oil, as the world names it)
    if (s === S[0]) wrec = text(h);
    if (hk === undefined || s.ro === RO.TOOL) s.th.word = both;   // (A thing's name; what it's done with a thing's,
                                                //   "with the bar", not the door's)
    else {
      s.th.kind = hk & ~PART;
      s.th.word = hk & PART ? h.typed.slice(0, 15) : qs ? qs.typed.slice(0, 15) : '';   // (A part of a thing by its own word:
                                                //   "the iron bars", "the window bars": "bars")
    }
  }
  let word = c.obj.word, word2 = c.other.word;
  // The old parser's rules, on what the words made
  if (c.obj.kind) c.obj.lies = dir; else c.dir = dir;   // (With a thing, its way is where it lies)
  if (verb === R.get('doff') && !c.obj.kind && dir !== NONE) verb = R.get('move');
  if (verb === R.get('hang')) c.prep = V.PR_ON;
  if (verb === R.get('stow') && word2) verb = R.get('put');
  if (verb === R.get('put') && !word2 && dir === V.GO_DOWN) verb = R.get('drop');
  if (verb === R.get('put') && w.special.get(wrec) === 'BAR' && c.obj.kind === V.OB_DOOR && !word2) verb = R.get('bar');
  if (verb === R.get('doff') && c.prep === V.PR_FROM && word2) verb = R.get('get');
  else if (verb === R.get('get') && c.prep === V.PR_FROM && !word2) verb = R.get('doff');
  said = word || c.obj.sel;
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
  let p, pw;                                   // (The verb's own preposition straight after it, the little words passed)
  for (let j = first; j < ws.length && p === undefined; ++j) {
    p = r.preps.get(text(ws[j]));
    pw = ws[j];
    if (p === undefined && !little(ws[j])) break;
  }
  if (c.dir !== NONE && verb === R.get('move') && c.obj.kind) c.action = r.way;   // (Up or down them: the way's)
  else if (c.obj.word || c.obj.kind || c.obj.sel) {
    c.action = p !== undefined ? p : r.thing || r.alone || r.way;
    if (p !== undefined && !c.prep && (k = get(pw, w.preps)) !== undefined) c.prep = k;   // ("look on the table": on it)
  } else if (c.dir !== NONE) c.action = r.way || r.thing || r.alone;
  else if (r.alone) c.action = r.alone;
  else { c.action = r.thing || r.way; c.flags |= CF_UNSAID; }   // (Its thing unsaid: its default the UI's to fill)
  return c;
}

// The tables dq0c's meaning reads (cmdmean.s), its part of dq0c.dat (in a bank of dq0c's own): magic "DQ0W", version
// 4; the constants it needs (CONSTANTS' values, a byte each: a verb's its number); the dictionary's entry count (the
// index's, u16), and a byte an entry: its record (0: none, no word of the game's); then a byte an entry: the verb it
// is, alone (0: none; not in its record, so each verb a record's no more: 2026-11-17, the records 255 of 255 before);
// the records' count (record 0 the none's among them), and their fields, a column each, a byte a record (COLUMNS: 0 in
// each the none's; the way's + 1, so $FF's none is 0 too); the verbs of several words, in their order (a count, the entries, the verb), all
// before any of one word's; the pairs (two entries, 1 a special's, the value); the verbs (a count, then each one's
// ROW bytes: its actions alone, with a thing, with a way, its unsaid, aux, skill, task and flags), and the
// prepositions' own actions (a count, then each: the verb, the word's entry, the action); then the words that make one
// kind another (qualWords: a count, each its length and letters)
const CONSTANTS = ['move', 'put', 'stow', 'doff', 'hang', 'drop', 'bar', 'get', 'turn', 'face', 'light', 'douse', 'unlock',
  'name', 'combine', 'PR_WITH', 'PR_ON', 'PR_FROM', 'OB_WAY_OUT', 'OB_DOOR', 'GO_UP', 'GO_DOWN', 'SL_ME', 'VB_SAY', 'OB_STAIRS',
  'OB_LADDER', 'OB_SHAFT', 'throw', 'HOW_TO', 'PR_AT', 'PR_IN'];
const SPECIAL = { WITH: 1, AWAY: 2, BACK: 3, HALF: 4, AROUND: 5, BAR: 6, WAY_OUT: 7, MAIN: 8 };   // (cmdmean.s's S_*)
const COLUMNS = ['dir', 'thing', 'prep', 'how', 'special', 'order', 'sel', 'num', 'manner', 'little', 'aspect'];   // (cmdmean.s's C_*)
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
  each(w.special, (x, v) => { x.special = byte(SPECIAL[v], 15, 'special ' + v); }, 1);
  each(w.selectors, (x, v) => { x.sel = byte(v, 255, 'selector'); });
  each(w.numbers, (x, v) => { x.num = byte(v, 255, 'number'); });
  each(w.manners, (x, v) => { x.manner = byte(v, 255, 'manner'); });
  each(w.aspects, (x, v) => { x.aspect = byte(v, 255, 'aspect'); });
  for (const k of w.little) if (tokenWord(k)) r(k).little = 1;
  const none = COLUMNS.map(() => 0), recs = [none], recOf = new Map([[none.join(','), 0]]), gwi = new Array(entries.length).fill(0);
  const gwv = new Array(entries.length).fill(0);
  for (const [k, x] of rec) {
    gwv[ent(k)] = x.verb;
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
  return Buffer.from([0x44, 0x51, 0x30, 0x57, 4, ...k, ...u16(entries.length), ...gwi, ...gwv,
    recs.length, ...COLUMNS.flatMap((c, i) => recs.map(b => b[i])), multi.length, ...multi.flat(), two.length, ...two.flat(),
    w.rows.length - 1, ...rows.flat(), preps.length, ...preps.flat(), byte(w.quals.size, 255, 'qualifiers'),
    ...[...w.quals].flatMap(q => [q.length, ...[...q].map(c => c.charCodeAt(0))])]);
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

// The world's words, as the UI tells dq0c them as the game starts (main.c's world_words: the catalogue's names' words
// the parser knows, each once, the build's: dq0w/make.js's, after the kinds in obj/games/dq0w.hyx.cat), or none, the
// catalogue not built
let WORLD = null;
function worldWords() {
  if (WORLD) return WORLD;
  WORLD = new Set();
  const f = path.join(GAME, '..', '..', 'obj', 'games', 'dq0w.hyx.cat');
  if (!fs.existsSync(f)) return WORLD;
  const cat = fs.readFileSync(f), at = HUB.C.NKINDS * HUB.size('kind'), end = cat.indexOf(0, at);
  for (const x of cat.subarray(at, end < 0 ? at : end).toString('latin1').split(' ')) if (x) WORLD.add(x);
  return WORLD;
}

// The roles of a line's tokens (the grammar's derivation, as dq0c's matcher makes it: commandcompile.js's rolesOf), the
// world's words those it's told (world: a Set, worldWords' if none's given; and the things' words, as dq0c marks them,
// always: cmdmean.s's cm_wthings); null if the line doesn't parse (or is too long for the matcher: MAXTOK)
let RULES = null;
function roles(toks, w, world = worldWords()) {
  const cc = require('./commandcompile.js');
  if (!RULES) {
    RULES = cc.loadRules(path.join(GAME, 'commands.ebnf'));
    RULES.reserved = cc.literalWords(RULES);
    RULES.things = new Set([...w.things.keys()].filter(k => !k.includes(' ')));
  }
  if (toks.length > 15) return null;
  const mt = toks.map(t => (t.k === 'q' ? { k: 'q', text: '' } : { k: 'w', v: t.root }));
  const env = { rules: RULES, toks: mt, memo: new Map(), reserved: RULES.reserved, world: new Set([...RULES.things, ...world]),
    names: new Map(), deities: new Set() };
  const lab = cc.rolesOf(env);
  return cc.endsOf({ t: 'nt', name: 'line' }, 0, env).has(mt.length) ? lab : null;
}

// A line's struct cmd, as the UI has it (facing face): its tokens (the default aliases), their roles, the meaning, finished
function meaning(line, w, face = 0) {
  const { DEFAULT_ALIASES, DEFAULT_ANYWHERE } = require('./commandcompile.js');
  const toks = tokens(line, DEFAULT_ALIASES, DEFAULT_ANYWHERE);
  return finish(scan(toks, w, face, roles(toks, w)), face, w.V);
}

// A struct cmd in a line of words (its fields that aren't empty)
function show(c) {
  const t = (x, n) => [x.kind && 'kind ' + x.kind, x.sel && 'sel ' + x.sel + (x.n ? '/' + x.n : ''), x.lies !== NONE && 'lies ' + x.lies,
    x.word && JSON.stringify(x.word)].filter(Boolean).map(s => n + ' ' + s);
  return [actionName(c.action), c.flags && 'flags ' + c.flags, c.hand && 'hand ' + c.hand, c.quote && 'quote ' + c.quote, c.dir !== NONE && 'dir ' + c.dir, c.how && 'how ' + c.how,
    c.prep && 'prep ' + c.prep, c.aux && 'aux ' + c.aux, c.manner && 'manner ' + c.manner, c.order && 'order ' + c.order, c.n && 'n ' + c.n,
    ...t(c.obj, 'obj'), ...t(c.other, 'other')].filter(Boolean).join(', ');
}

module.exports = { load, tokens, scan, finish, qualWords, between, meaning, roles, worldWords, RO, HN, CF_WORN, CF_LIST, names, gameWords, encode, actionName, show, CONSTANTS, SPECIAL,
  COLUMNS, ROW, REL, PART, NONE, CF_NOVERB, CF_HALF, CF_UNSAID, CF_STRANGE };

if (require.main === module) {
  const w = load();
  for (const l of process.argv.slice(2)) console.log(JSON.stringify(l) + ' ' + show(meaning(l, w)));
}
