// commandcompile.js - the reference command compiler for Dingus Quest Zero (host side, in node).
// It reads games/dq0/commands.ebnf, parses a typed line by backtracking over the grammar, turns the parse into an
// abstract action (games/dq0/commands.actions) with its roles, and checks the corpus (commands.corpus).
//   node sim/tools/commandcompile.js               check the corpus; prints each failure and a summary
//   node sim/tools/commandcompile.js look north    compile one line with no context
// The 65C02 compiler will be built from this design; this file is the behaviour to match.
'use strict';
const fs = require('fs');
const path = require('path');

const GAME = path.join(__dirname, '..', '..', 'games', 'dq0');

// The default aliases (commands.aliases): each word and the verb it means, added to every player's list.
function loadAliases(file) {
  const CR = String.fromCharCode(13), NL = String.fromCharCode(10);
  const m = new Map();
  DEFAULT_ANYWHERE.clear();
  for (const line of fs.readFileSync(file, 'utf8').split(CR).join('').split(NL)) {
    const t = line.trim();
    if (!t || t.startsWith('#')) continue;
    let parts = t.split(' ');
    // a leading * is an alias that stands anywhere in a line (a direction word), not only at the start of a command
    const anywhere = parts[0].charAt(0) === '*';
    if (anywhere) parts[0] = parts[0].slice(1);
    m.set(parts.slice(0, -1).join(' '), parts[parts.length - 1]);
    if (anywhere) DEFAULT_ANYWHERE.add(parts.slice(0, -1).join(' '));
  }
  return m;
}
const DEFAULT_ANYWHERE = new Set();
const DEFAULT_ALIASES = loadAliases(path.join(GAME, 'commands.aliases'));

// ---- the grammar ----------------------------------------------------------------------------------------------------

// The rules of commands.ebnf as trees: alt / seq / opt / rep / nt (a rule or open name) / term (words) / quote.
function loadRules(file) {
  const text = fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n').replace(/\(\*[\s\S]*?\*\)/g, ' ');
  const bodies = new Map();
  let cur = null;
  for (const line of text.split('\n')) {
    const m = line.match(/^\s*<([^>]+)>\s*::=(.*)$/);
    if (m) { cur = m[1]; bodies.set(cur, m[2]); }
    else if (cur && line.trim() && /^\s/.test(line)) bodies.set(cur, bodies.get(cur) + ' ' + line);
    else if (!line.trim()) cur = null;
  }
  const rules = new Map();
  for (const [name, body] of bodies) rules.set(name, parseBody(body, name));
  return rules;
}

function parseBody(s, name) {
  const toks = s.match(/"[^"]*"|'[^']*'|<[^>]+>|[\[\]{}()|]/g) || [];
  let i = 0;
  const alt = () => {
    const opts = [seq()];
    while (toks[i] === '|') { i++; opts.push(seq()); }
    return opts.length === 1 ? opts[0] : { t: 'alt', opts };
  };
  const seq = () => {
    const items = [];
    while (i < toks.length && !['|', ')', ']', '}'].includes(toks[i])) items.push(item());
    return { t: 'seq', items };
  };
  const item = () => {
    const k = toks[i++];
    if (k === '[') { const a = alt(); i++; return { t: 'opt', x: a }; }
    if (k === '{') { const a = alt(); i++; return { t: 'rep', x: a }; }
    if (k === '(') { const a = alt(); i++; return a; }
    if (k[0] === '<') return { t: 'nt', name: k.slice(1, -1) };
    if (k === "'\"'") return { t: 'quote' };
    if (k[0] === '"') return { t: 'term', words: k.slice(1, -1).toLowerCase().split(/\s+/).filter(Boolean) };
    throw new Error(`${name}: bad token ${k}`);
  };
  return alt();
}

// Every word the grammar writes as a literal: these are reserved, so they can't be names of things.
function literalWords(rules) {
  const out = new Set();
  const walk = n => {
    if (!n) return;
    if (n.t === 'term') n.words.forEach(w => out.add(w));
    if (n.items) n.items.forEach(walk);
    if (n.opts) n.opts.forEach(walk);
    if (n.x) walk(n.x);
  };
  for (const r of rules.values()) walk(r);
  return out;
}

// ---- the tokens of a line -------------------------------------------------------------------------------------------

// Words (lower case), "!" and "=" as their own tokens, and quoted text: {k:'q', text, vol} with vol 'w' or 'y' when
// the quote is attached to a w or y prefix (w"..." whispers, y"..." yells).
// (A word may begin with #: a thing the player named, its id, as names.c makes the line: "get #12")
function tokenize(line) {
  const out = [];
  const re = /([wy]?)"([^"]*)"|(#[A-Za-z0-9'\-]*|[A-Za-z0-9][A-Za-z0-9'\-]*)|(!)|(=)/g;
  let m;
  while ((m = re.exec(line))) {
    if (m[2] !== undefined) out.push({ k: 'q', text: m[2], vol: m[1] || null });
    else if (m[3]) out.push({ k: 'w', v: m[3].toLowerCase() });
    else out.push({ k: 'w', v: m[4] || m[5] });
  }
  return out;
}

// ---- the tables -----------------------------------------------------------------------------------------------------

const own = (o, k) => (Object.prototype.hasOwnProperty.call(o, k) ? o[k] : undefined);
const LIMIT = 200;
// Open names: their words come from the game's data; openOK says which words fit each one.
const OPEN = new Set(['deity-name', 'pretense-word', 'name', 'noun', 'alias', 'adj',
  'number', 'ordinal', 'task-name', 'meta-topic', 'name-word', 'spell-word', 'text', 'quoted-text']);
const ADJ = new Set(['right', 'far', 'wooden', 'iron', 'large', 'small', 'closed', 'open', 'old', 'rusty',
  'bottom', 'top', 'heavy', 'near', 'left']);
const NUMWORDS = { one: 1, two: 2, three: 3, four: 4, five: 5, six: 6, seven: 7, eight: 8, nine: 9, ten: 10 };
const ORDINALS = ['first', 'second', 'third', 'fourth', 'fifth'];
const TASKS = ['digging', 'crawling', 'waiting', 'searching', 'climbing', 'sleeping', 'resting', 'working', 'pretending'];
// Words no object may be: the player is "me", never "you" (question 14).
const NOT_OBJECT = new Set(['you', 'character', 'yourself']);
const SPELL_STOP = new Set(['at', 'toward', 'towards', 'to', 'spell', 'the', 'a', 'an', 'my', 'this', 'that', 'these', 'those']);
const MOVE_WORDS = new Set(['move', 'mv', 'walk', 'trundle', 'mosey', 'saunter', 'run', 'crawl', 'go', 'head']);
const SPEECH_WORDS = new Set(['say', 'yell', 'scream', 'shout', 'whisper']);
// These take no object: "wait the sword" is a role error, not a parse error.
const INTRANSITIVE = new Set(['wait', 'rest', 'sleep', 'stop']);
const PACE = { crawl: 'crawl', 'slow walk': 'slowwalk', walk: 'walk', trot: 'trot', run: 'run', sprint: 'sprint' };
const META = { status: 'VB_META_STATUS', help: 'VB_META_HELP', inventory: 'VB_META_INVENTORY', i: 'VB_META_INVENTORY',
  map: 'VB_META_MAP', save: 'VB_META_SAVE', quit: 'VB_META_QUIT', q: 'VB_META_QUIT', sheet: 'VB_META_SHEET',
  pause: 'VB_META_PAUSE' };
const ATTACK_MANNER = { slash: 'edge', slice: 'edge', chop: 'edge', swing: 'edge', bash: 'blunt', bludgeon: 'blunt',
  thump: 'blunt', club: 'blunt', stab: 'point', poke: 'point', pierce: 'point', punch: 'unarmed', kick: 'unarmed',
  headbutt: 'unarmed', bite: 'unarmed', elbow: 'unarmed', knee: 'unarmed', slap: 'unarmed', butt: 'unarmed' };
const POSTURE = { SIT: 'VB_SIT', STAND: 'VB_STAND', 'LIE-DOWN': 'VB_LIE_DOWN', CROUCH: 'VB_CROUCH' };
const GRAPPLE_MANEUVER = { grapple: 'grapple', wrestle: 'wrestle', tackle: 'tackle', pin: 'pin', choke: 'choke',
  hold: 'hold', grab: 'grab', put: 'headlock', toss: 'throw', throw: 'throw', chuck: 'throw', hurl: 'throw', sling: 'throw', t: 'throw' };
const LOOK_PREP_ACTION = { through: 'VB_OBSERVE_THROUGH', around: 'VB_OBSERVE_AROUND', for: 'VB_FIND_OBJECT' };
const numWord = v => own(NUMWORDS, v);

// Default world: the names of things the game knows. A word not here is "no such thing"; one here but not perceived is
// "not perceived".
const DEFAULT_WORLD = ['dragon', 'pit', 'torch', 'spell', 'bank', 'grating', 'smoke', 'rats', 'lamp',
  'hook', 'helm', 'apple', 'sword', 'stone', 'chest', 'lid', 'corner', 'bow', 'crossbow', 'club', 'dagger', 'key',
  'door', 'box', 'window', 'wall', 'table', 'goblin', 'rat', 'rations', 'waterskin', 'potion', 'room', 'linda',
  'rope', 'lever', 'stairs', 'ladder', 'sign', 'shield', 'gate', 'bed', 'stream', 'spring'];

function openOK(name, tk, env) {
  if (name === 'quoted-text') return tk.k === 'q';
  if (tk.k !== 'w') return false;
  const v = tk.v;
  switch (name) {
    case 'number': return /^\d+$/.test(v) || numWord(v) !== undefined;
    case 'ordinal': return ORDINALS.includes(v);
    case 'adj': return ADJ.has(v);
    case 'task-name': return TASKS.includes(v);
    case 'spell-word': return !SPELL_STOP.has(v) && !INVOCATION_WORDS.includes(v);   // "learn divination" is an invocation
    case 'pretense-word': return !env.reserved.has(v);
    case 'deity-name': return env.deities.has(v);
    case 'text': case 'meta-topic': return true;
    case 'name-word': return !env.reserved.has(v) || env.world.has(v);
    case 'noun':                               // (A name's word before its last: no adjective, number or ordinal)
      return !NOT_OBJECT.has(v) && (!env.reserved.has(v) || env.world.has(v) || env.names.has(v)) && !ADJ.has(v) &&
        !/^[0-9]+$/.test(v) && numWord(v) === undefined && !ORDINALS.includes(v);
    default: // unique-name, kind-name, fixture-name, place-name, known-alias, alias: a name of a thing
      return !NOT_OBJECT.has(v) && (!env.reserved.has(v) || env.world.has(v) || env.names.has(v));
  }
}

// ---- matching -------------------------------------------------------------------------------------------------------

function matchNode(node, pos, env) {
  switch (node.t) {
    case 'term': {
      let p = pos;
      for (const w of node.words) {
        const tk = env.toks[p];
        if (!tk || tk.k !== 'w' || tk.v !== w) return [];
        p++;
      }
      return [{ end: p, kids: [] }];
    }
    case 'quote': return env.toks[pos] && env.toks[pos].k === 'q' ? [{ end: pos + 1, kids: [] }] : [];
    case 'nt': return matchRule(node.name, pos, env);
    case 'seq': {
      let st = [{ end: pos, kids: [] }];
      for (const it of node.items) {
        const nx = [];
        for (const s of st) for (const r of matchNode(it, s.end, env)) nx.push({ end: r.end, kids: s.kids.concat(r.kids) });
        st = nx.slice(0, LIMIT);
        if (!st.length) return [];
      }
      return st;
    }
    case 'alt': {
      let out = [];
      for (const o of node.opts) out = out.concat(matchNode(o, pos, env));
      return out.slice(0, LIMIT);
    }
    case 'opt': return [{ end: pos, kids: [] }].concat(matchNode(node.x, pos, env));
    case 'rep': {
      let out = [{ end: pos, kids: [] }];
      let frontier = out.slice();
      for (let g = 0; g < 12 && frontier.length; g++) {
        const nx = [];
        for (const s of frontier) for (const r of matchNode(node.x, s.end, env)) {
          if (r.end !== s.end) nx.push({ end: r.end, kids: s.kids.concat(r.kids) });
        }
        frontier = nx.slice(0, LIMIT);
        out = out.concat(frontier);
      }
      return out.slice(0, LIMIT);
    }
    default: throw new Error('bad node ' + node.t);
  }
}

// A rule (or open name) at a position. Results are {end, kids:[node]}; a node is {rule, start, end, kids}.
function matchRule(name, pos, env) {
  const key = name + '@' + pos;
  if (env.memo.has(key)) return env.memo.get(key);
  env.memo.set(key, []);
  let res;
  if (OPEN.has(name)) {
    const tk = env.toks[pos];
    res = tk && openOK(name, tk, env) ? [{ end: pos + 1, kids: [{ rule: name, start: pos, end: pos + 1, kids: [] }] }] : [];
  } else {
    const body = env.rules.get(name);
    if (!body) throw new Error('no rule <' + name + '>');
    res = matchNode(body, pos, env).map(r => ({ end: r.end, kids: [{ rule: name, start: pos, end: r.end, kids: r.kids }] }));
  }
  env.memo.set(key, res);
  return res;
}

// ---- the tree -------------------------------------------------------------------------------------------------------

function findFirst(node, rule) {
  if (!node) return null;
  for (const k of node.kids) {
    if (k.rule === rule) return k;
    const f = findFirst(k, rule);
    if (f) return f;
  }
  return null;
}
const kidsOf = (node, rule) => (node ? node.kids.flatMap(k => k.rule === rule ? [k] :            // (A thing in the place:
  k.rule === 'thing' ? k.kids.filter(x => x.rule === rule) : []) : []);                         //   its object)
const countRule = (node, rule) => node.kids.reduce((n, k) => n + (k.rule === rule ? 1 : 0) + countRule(k, rule), 0);
const wordsOf = (node, env) => (node ? env.toks.slice(node.start, node.end).filter(t => t.k === 'w').map(t => t.v) : []);
const textOf = (node, env) => wordsOf(node, env).join(' ');
const quoteOf = (node, env) => env.toks[node.start].text;

// Direction in the player's frame: compass letters, or F B L R U D; "around the corner" is a throw's direction.
function dirCode(node, env) {
  const w = new Set(wordsOf(node, env));
  const diag = ['ne', 'nw', 'se', 'sw'].find(x => w.has(x));
  if (diag) return diag.toUpperCase();
  const ns = w.has('north') || w.has('n') ? 'N' : w.has('south') || w.has('s') ? 'S' : '';
  const ew = w.has('east') || w.has('e') ? 'E' : w.has('west') || w.has('w') ? 'W' : '';
  if (ns || ew) return ns + ew;
  if (w.has('around') && w.has('corner')) return 'around-corner';
  const fb = ['f', 'fwd', 'forward', 'ahead', 'front'].some(x => w.has(x)) ? 'F' : ['b', 'back', 'behind'].some(x => w.has(x)) ? 'B' : '';
  const lr = ['l', 'left'].some(x => w.has(x)) ? 'L' : ['r', 'right'].some(x => w.has(x)) ? 'R' : '';
  if (fb || lr) return fb + lr;                 // ("ahead to my left": FL)
  if (['u', 'up', 'above'].some(x => w.has(x)) || (w.has('over') && w.has('head'))) return 'U';
  if (['d', 'down', 'below', 'under', 'underneath'].some(x => w.has(x))) return 'D';
  return '?';
}

const paceOf = (node, env) => own(PACE, textOf(node, env)) || null;

function numberOf(node, env) {
  if (!node) return null;
  const w = wordsOf(node, env)[0];
  if (w === undefined) return null;
  if (/^\d+$/.test(w)) return Number(w);
  return numWord(w) ?? null;
}

// Minutes of a duration: "10 minutes", "2 hours", "an hour", "a minute".
function minutesOf(node, env) {
  const t = textOf(node, env);
  if (t === 'an hour') return 60;
  if (t === 'a minute') return 1;
  const n = numberOf(node, env) ?? 1;
  const unit = wordsOf(node, env).find(w => /^(hour|minute)/.test(w));
  return n * (unit && unit.startsWith('hour') ? 60 : 1);
}

// ---- the context ----------------------------------------------------------------------------------------------------

function parsePending(v) {
  const parts = v.split(':');
  if (parts[0] === 'which') return { kind: 'which', verb: parts[1], choices: parts[2].split(',') };
  if (parts[0] === 'distance') return { kind: 'distance', max: Number(parts[2].replace('max=', '')) };
  if (parts[0] === 'overwrite') return { kind: 'overwrite', alias: parts[1], current: parts[2], next: parts[3] };
  return { kind: parts[0] };
}

// The context: what is perceived, the last things named, the target and pace, the aliases, the names given, the
// pending question. Keys: near last facing pending distances names target pace; any other key is an alias.
function buildContext(spec, world) {
  const ctx = { near: [], last: [], facing: null, pending: null, target: null, pace: 'walk', names: new Map(),
    dist: new Map(), aliases: new Map(DEFAULT_ALIASES), world: new Set(world), engaged: [], deities: new Set(), person: null };
  const addName = (key, id) => {
    const k = key.toLowerCase();
    if (!ctx.names.has(k)) ctx.names.set(k, []);
    if (!ctx.names.get(k).includes(id)) ctx.names.get(k).push(id);
  };
  let distances = null;
  for (const pair of (!spec || spec === '-' ? [] : spec.split(';'))) {
    const eq = pair.indexOf('=');
    if (eq < 0) continue;
    const k = pair.slice(0, eq), v = pair.slice(eq + 1);
    if (k === 'near') {
      ctx.near = v.split(',');
      ctx.near.forEach(id => { addName(id, id); addName(id.replace(/\d+$/, ''), id); ctx.world.add(id.replace(/\d+$/, '')); });
    } else if (k === 'names') v.split(',').forEach(n => addName(n, n));
    else if (k === 'last') ctx.last = v.split(',');
    else if (k === 'facing') ctx.facing = v;
    else if (k === 'pending') ctx.pending = parsePending(v);
    else if (k === 'distances') distances = v.split(',').map(Number);
    else if (k === 'engaged') ctx.engaged = v.split(',');
    else if (k === 'deities') ctx.deities = new Set(v.split(','));
    else if (k === 'person') ctx.person = v;
    else if (k === 'target') ctx.target = v;
    else if (k === 'pace') ctx.pace = own(PACE, v) || v;
    else ctx.aliases.set(k, v);
  }
  if (distances) {
    const list = ctx.pending && ctx.pending.kind === 'which' ? ctx.pending.choices : ctx.near;
    list.forEach((id, i) => ctx.dist.set(id, distances[i]));
  }
  return ctx;
}

// ---- resolving objects ----------------------------------------------------------------------------------------------

// Returns {value} or {error: CODE} or {which: [ids]}. A remote object (in the inventory, or known by name) need not be
// perceived: its word is its value.
function resolveObject(node, env, ctx, remote) {
  if (!node) return { value: undefined };
  const pron = findFirst(node, 'pronoun');
  if (pron) {
    // he, she, him, her: the last person named (context "person="); it, them, that, this, one: the last thing of any kind
    if (['he', 'she', 'him', 'her'].indexOf(wordsOf(pron, env)[0]) >= 0) return ctx.person ? { value: ctx.person } : { error: 'NOTHING_HERE' };
    if (!ctx.last.length) return { error: 'NOTHING_HERE' };
    return { value: ctx.last[0] };
  }
  const mine = findFirst(node, 'me-phrase');
  if (mine && ['me', 'myself'].includes(wordsOf(mine, env)[0])) return { value: 'me' };
  if (findFirst(node, 'everything')) return { value: 'all-near' };
  if (findFirst(node, 'way-out')) return { value: 'way out' };   // (The theme's way out)
  const head = findFirst(node, 'head');
  if (!head) return { error: 'NOT_UNDERSTOOD' };
  const hw = wordsOf(head, env), h = hw[hw.length - 1];   // (Its name: the nouns before it say which, "the brass key")
  if (h === 'fire') return { value: 'fire' };
  const sel = textOf(findFirst(node, 'selector'), env);
  const cands = ctx.names.get(h) || [];
  if (!cands.length) {
    if (remote) return { value: h };
    return { error: ctx.world.has(h) ? 'NOT_PERCEIVED' : 'NO_SUCH_THING' };
  }
  if (sel === 'closest' || sel === 'nearest') {
    return { value: cands.slice().sort((a, b) => (ctx.dist.get(a) ?? 0) - (ctx.dist.get(b) ?? 0))[0] };
  }
  if (['all', 'every', 'each', 'any'].includes(sel)) return { value: 'all-' + h };
  if (cands.length > 1 && !sel) return { which: cands };
  const ord = ORDINALS.indexOf(sel);
  const selNum = numWord(sel) ?? (/^\d+$/.test(sel) ? Number(sel) : null);
  if (ord >= 0 && cands[ord]) return { value: cands[ord] };
  if (selNum && cands[selNum - 1]) return { value: cands[selNum - 1] };
  return { value: cands[0] };
}

// ---- results --------------------------------------------------------------------------------------------------------

function clean(roles) {
  const o = {};
  for (const k of Object.keys(roles || {})) if (roles[k] !== undefined && roles[k] !== null) o[k] = String(roles[k]);
  return o;
}
const act = (action, roles) => ({ action, roles: clean(roles) });
const err = (code, roles) => ({ error: code, roles: clean(roles) });
const question = (kind, roles) => ({ question: kind, roles: clean(roles) });

// An object role: its value, or the result (an error or a which-question) that stops the command.
function objRole(node, env, ctx, remote) {
  const r = resolveObject(node, env, ctx, remote);
  if (r.error) return { stop: err(r.error) };
  if (r.which) return { stop: question('WHICH', { choices: r.which.join(',') }) };
  return { value: r.value };
}

// The target of a look, listen or smell: a locale, a direction, or an object with its preposition.
function targetOf(tgt, env, ctx, remote) {
  if (!tgt) return { kind: 'locale' };
  if (findFirst(tgt, 'locale-ref')) return { kind: 'locale' };
  const dir = findFirst(tgt, 'direction');
  if (dir && !findFirst(dir, 'object')) return { kind: 'direction', direction: dirCode(dir, env) };
  const o = objRole(findFirst(tgt, 'object'), env, ctx, remote);
  if (o.stop) return { stop: o.stop };
  return { kind: 'object', prep: wordsOf(tgt, env)[0], object: o.value };
}

const lookAction = prep => own(LOOK_PREP_ACTION, prep) || 'VB_OBSERVE_OBJECT';

// The adjuncts of a command (manner, with, from, at, locative, direction) fill roles the phrase left empty.
function withAdjuncts(res, cmd, env, ctx) {
  if (!res.action || !res.action.startsWith('VB_')) return res;
  const put = (key, v) => { if (v !== undefined && res.roles[key] === undefined) res.roles[key] = String(v); };
  const ROLE_OF = { 'with-phrase': 'instrument', 'from-phrase': 'source', 'at-phrase': 'target', locative: 'container' };
  for (const adj of kidsOf(cmd, 'adjunct')) {
    const inner = adj.kids[0];
    if (inner.rule === 'manner') put('manner', wordsOf(inner, env)[0]);
    else if (inner.rule === 'direction') put('direction', dirCode(inner, env));
    else if (own(ROLE_OF, inner.rule)) {
      const o = objRole(findFirst(inner, 'object'), env, ctx);
      if (o.stop) return o.stop;
      put(ROLE_OF[inner.rule], o.value);
    }
  }
  return res;
}

// An object list ("the sword and the shield") is the same phrase once for each object, in order. Only the first list
// in a phrase is expanded here; a second list in the same phrase is expanded the same way on the next pass.
function expandLists(spec) {
  const listed = kidsOf(spec, 'object').find(o => kidsOf(o, 'object-one').length > 1);
  if (!listed) return [spec];
  const out = [];
  for (const part of kidsOf(listed, 'object-one')) {
    const single = Object.assign({}, listed, { kids: [part] });
    const swap = k => (k === listed ? single : k.rule === 'thing' && k.kids.includes(listed) ?   // (A thing's list: in it)
      Object.assign({}, k, { kids: k.kids.map(x => (x === listed ? single : x)) }) : k);
    const copy = Object.assign({}, spec, { kids: spec.kids.map(swap) });
    out.push(...expandLists(copy));
  }
  return out;
}

// A sequence of results: a step that failed stops the line with its error; a step that is itself a sequence is
// flattened into the list.
function sequenceOf(results) {
  const steps = [];
  for (const r of results) {
    if (r.error || r.question) return r;
    if (r.action === 'VB_SEQUENCE') steps.push(...r.steps);
    else steps.push(r);
  }
  return { action: 'VB_SEQUENCE', roles: { steps: steps.map(x => x.action).join(',') }, steps };
}

// The phrase's own meaning: spec is the rule named by its kind (look, get, move, ...).
function phraseSem(spec, env, ctx) {
  const r = spec.rule;
  const direct = kidsOf(spec, 'object');
  const objOf = (i, remote) => objRole(direct[i], env, ctx, remote);
  const thing = kidsOf(spec, 'thing')[0] || kidsOf(kidsOf(spec, 'move-dir')[0], 'thing')[0];
  const lies = thing && kidsOf(thing, 'lies')[0] ? dirCode(kidsOf(thing, 'lies')[0], env) : undefined;   // (Where it lies)
  const withObj = (action, extra, remote) => {
    const o = objOf(0, remote);
    if (o.stop) return o.stop;
    return act(action, Object.assign({ object: o.value, lies }, extra || {}));
  };
  const optObj = action => (direct[0] ? withObj(action) : act(action, {}));   // (A verb alone: what's in front)
  // The verbs whose one object is a target, not an object.
  const onTarget = (action, remote) => {
    const o = objOf(0, remote);
    if (o.stop) return o.stop;
    return act(action, { target: o.value });
  };
  const manner = (node, e) => (kidsOf(node, 'manner')[0] ? wordsOf(kidsOf(node, 'manner')[0], e)[0] : undefined);
  const adjOf = () => (findFirst(spec, 'adj') ? wordsOf(findFirst(spec, 'adj'), env)[0] : undefined);
  switch (r) {
    case 'look': {
      const t = targetOf(kidsOf(spec, 'look-target')[0], env, ctx, false);
      if (t.stop) return t.stop;
      if (t.kind === 'locale') return act('VB_OBSERVE_LOCALE', { place: 'here' });
      if (t.kind === 'direction') return act('VB_OBSERVE_DIRECTION', { direction: t.direction });
      return act(lookAction(t.prep), { object: t.object });
    }
    case 'examine': return direct[0] ? withObj('VB_EXAMINE_OBJECT') : act('VB_EXAMINE_LOCALE', { place: 'here' });
    case 'move': {
      const md = kidsOf(spec, 'move-dir')[0];
      const pw = kidsOf(spec, 'pace-word')[0];
      const first = wordsOf(spec, env)[0];
      const pace = pw ? paceOf(pw, env) : (['run', 'crawl'].includes(first) ? first : undefined);
      if (md && findFirst(md, 'object') && ['to', 'towards', 'toward', 'in', 'into', 'onto', 'past'].includes(wordsOf(md, env)[0])) {
        const o = objRole(findFirst(md, 'object'), env, ctx);
        if (o.stop) return o.stop;
        return act('VB_MOVE_TOWARD', { object: o.value });
      }
      return act('VB_MOVE_DIRECTION', { direction: dirCode(findFirst(md, 'direction') || md, env), pace,
        distance: numberOf(kidsOf(spec, 'number')[0], env) });
    }
    case 'face': return act('VB_FACE_DIRECTION', { direction: wordsOf(spec, env).includes('around') ? 'B' : dirCode(findFirst(spec, 'direction'), env),
      half: wordsOf(spec, env).includes('half') ? 1 : undefined });   // ("turn half left": an eighth)
    case 'get': {
      let value;
      if (!direct[0]) {
        if (ctx.last.length) value = ctx.last[0];
        else if (ctx.near.length) return question('WHICH', { choices: ctx.near.join(',') });
        else return err('NOTHING_HERE');
      } else {
        const o = objOf(0);
        if (o.stop) return o.stop;
        value = o.value;
      }
      const from = kidsOf(spec, 'from-phrase')[0] || kidsOf(spec, 'off-phrase')[0];
      let source;
      if (from) {
        const s = objRole(findFirst(from, 'object'), env, ctx);
        if (s.stop) return s.stop;
        source = s.value;
      }
      if (ctx.engaged.includes(value)) return err('ENGAGED');
      return act('VB_RETRIEVE_OBJECT', { object: value, source });
    }
    case 'drop': return withObj('VB_DROP_OBJECT');
    case 'put': {
      const a = objOf(0), b = objOf(1);
      if (a.stop) return a.stop;
      if (b.stop) return b.stop;
      return act('VB_PUT_OBJECT', { object: a.value, container: b.value });
    }
    case 'use': {
      const a = objOf(0), b = objOf(1);
      if (a.stop) return a.stop;
      if (b.stop) return b.stop;
      return act('VB_USE_OBJECT', { object: a.value, target: b.value });
    }
    case 'equip': return withObj('VB_EQUIP_OBJECT');
    case 'unequip': return withObj('VB_UNEQUIP_OBJECT', null, true);
    case 'open': return direct[0] ? withObj('VB_OPEN_OBJECT', { adj: adjOf(), manner: manner(spec, env) }) : act('VB_OPEN_OBJECT', { manner: manner(spec, env) });
    case 'close': return direct[0] ? withObj('VB_CLOSE_OBJECT', { adj: adjOf(), manner: manner(spec, env) }) : act('VB_CLOSE_OBJECT', { manner: manner(spec, env) });
    case 'attack': {
      const a = objOf(0);
      if (a.stop) return a.stop;
      let instrument;
      if (direct[1]) {
        const b = objOf(1);
        if (b.stop) return b.stop;
        instrument = b.value;
      }
      const verb = wordsOf(spec, env)[0];
      return act('VB_ATTACK_OBJECT', { object: a.value, instrument, manner: own(ATTACK_MANNER, verb),
        blow: own(ATTACK_MANNER, verb) === 'unarmed' ? verb : undefined, engage: engageFlag(a.value, ctx) });
    }
    case 'throw': {
      const td = kidsOf(spec, 'throw-dir')[0];
      const at = kidsOf(spec, 'at-phrase')[0];
      const mn = kidsOf(spec, 'manner')[0];
      const manner = mn ? textOf(mn, env) : undefined;
      const instrument = wordsOf(spec, env)[0] === 'sling' ? 'sling' : undefined;
      if (td) {
        const o = objOf(0);
        if (o.stop) return o.stop;
        return question('DISTANCE', { object: o.value, direction: dirCode(td, env), manner, instrument });
      }
      if (at) {
        const o = objOf(0);
        if (o.stop) return o.stop;
        const t = objRole(findFirst(at, 'object'), env, ctx);
        if (t.stop) return t.stop;
        return act('VB_THROW_OBJECT', { object: o.value, target: t.value, manner, instrument });
      }
      if (ctx.near.length) return question('WHICH', { choices: ctx.near.join(',') });
      return err('NOTHING_HERE');
    }
    case 'shoot': {
      const at = kidsOf(spec, 'at-phrase')[0];
      const o = objOf(0);
      if (o.stop) return o.stop;
      if (!at) return act('VB_SHOOT_OBJECT', { target: o.value });
      const t = objRole(findFirst(at, 'object'), env, ctx);
      if (t.stop) return t.stop;
      return act('VB_SHOOT_OBJECT', { instrument: o.value, target: t.value });
    }
    case 'load': return withObj('VB_LOAD_OBJECT');
    case 'jump': {
      const d = kidsOf(spec, 'direction')[0];
      if (d) return act('VB_JUMP', { direction: dirCode(d, env) });
      if (direct[0]) return withObj('VB_JUMP');
      return act('VB_JUMP', {});
    }
    case 'say': return sayRoles(spec, env, wordsOf(spec, env)[0]);
    case 'tell': {
      const a = objOf(0);
      if (a.stop) return a.stop;
      if (wordsOf(spec, env).indexOf('that') >= 0 && !kidsOf(spec, 'quoted-text').length) {
        const ws = wordsOf(spec, env);
        return act('VB_TELL_OBJECT', { object: a.value, topic: ws.slice(ws.indexOf('that') + 1).join(' ') });
      }
      let topic;
      if (direct[1]) {
        const b = objOf(1);
        if (b.stop) return b.stop;
        topic = b.value;
      }
      const ph = kidsOf(spec, 'phrase')[0];
      if (ph) topic = textOf(ph, env);
      const q = findFirst(spec, 'quoted-text');
      return act('VB_TELL_OBJECT', { object: a.value, text: q ? quoteOf(q, env) : undefined, topic });
    }
    case 'cast': {
      const sw = findFirst(spec, 'spell-word');
      const at = kidsOf(spec, 'at-phrase')[0];
      const t = at ? objRole(findFirst(at, 'object'), env, ctx) : { value: undefined };
      if (t.stop) return t.stop;
      return act('VB_CAST_SPELL', { spell: sw ? textOf(sw, env) : undefined, target: t.value });
    }
    case 'eat': return withObj('VB_EAT_OBJECT');
    case 'drink': return direct[0] ? withObj('VB_DRINK') : act('VB_DRINK', {});
    case 'swim': {
      const md = kidsOf(spec, 'move-dir')[0];
      if (!md) return act('VB_SWIM', {});
      if (findFirst(md, 'object')) {
        const o = objRole(findFirst(md, 'object'), env, ctx);
        if (o.stop) return o.stop;
        return act('VB_SWIM', { object: o.value });
      }
      return act('VB_SWIM', { direction: dirCode(findFirst(md, 'direction') || md, env) });
    }
    case 'find': return withObj('VB_FIND_OBJECT', null, true);
    case 'search': return direct[0] ? withObj('VB_SEARCH') : act('VB_SEARCH', {});
    case 'listen': case 'smell': {
      const t = targetOf(kidsOf(spec, 'look-target')[0], env, ctx, r === 'smell');
      if (t.stop) return t.stop;
      const action = r === 'listen' ? 'VB_LISTEN' : 'VB_SMELL';
      if (t.kind === 'direction') return act(action, { direction: t.direction });
      if (t.kind === 'object') return act(action, { object: t.object });
      return act(action, {});
    }
    case 'track': return direct[0] ? withObj('VB_TRACK', null, true) : act('VB_TRACK', {});
    case 'target': {
      const words = wordsOf(spec, env);
      if (words.includes('clear') || words.includes('untarget')) return act('VB_TARGET_CLEAR', {});
      if (direct[0]) return withObj('VB_TARGET_SET');
      return act('VB_TARGET_SHOW', { target: ctx.target || 'none' });
    }
    case 'wait': {
      const d = kidsOf(spec, 'duration')[0];
      return act('VB_WAIT', { duration: (d ? minutesOf(d, env) : 1) + 'min' });
    }
    case 'rest':
    case 'sleep': {
      const d = kidsOf(spec, 'duration')[0];
      return act('VB_SLEEP', { short: wordsOf(spec, env).includes('short') ? 'yes' : 'no',
        duration: d ? minutesOf(d, env) + 'min' : undefined });
    }
    case 'stop': {
      const t = findFirst(spec, 'task-name');
      return act('VB_STOP', { task: t ? wordsOf(t, env)[0] : 'current' });
    }
    case 'naming': {
      const words = wordsOf(spec, env);
      const name = words[words.length - 1];
      if (words.includes('room') || words.includes('place')) return act('VB_NAME_PLACE', { place: 'room', name });
      return withObj('VB_NAME_OBJECT', { name });
    }
    case 'meta': {
      const topic = findFirst(spec, 'meta-topic');
      return act(own(META, wordsOf(spec, env)[0]), topic ? { topic: wordsOf(topic, env)[0] } : {});
    }
    case 'step': return act('VB_STEP_DIRECTION', { direction: dirCode(findFirst(spec, 'direction'), env) });
    case 'pace-set': {
      if (wordsOf(spec, env)[0] === 'double') return act('VB_SET_PACE', { pace: 'run' });
      const pw = kidsOf(spec, 'pace-word')[0];
      if (pw) return act('VB_SET_PACE', { pace: paceOf(pw, env) });
      if (wordsOf(spec, env).includes('walk')) return act('VB_SET_PACE', { pace: 'walk' });
      return act('VB_PACE_SHOW', { pace: ctx.pace || 'walk' });
    }
    case 'charge': {
      if (!ctx.near.length) return err('NO_TARGET');
      const fast = wordsOf(spec, env).includes('fast');
      return act('VB_CHARGE', { direction: 'facing', pace: fast ? 'sprint' : 'run', text: 'Charge!' });
    }
    case 'light': case 'extinguish': {
      const action = r === 'light' ? 'VB_LIGHT_OBJECT' : 'VB_EXTINGUISH_OBJECT';
      if (!direct[0]) return act(action, { object: 'fire' });
      return withObj(action, null, true);
    }
    case 'enter': return direct[0] ? withObj('VB_ENTER') : act('VB_ENTER', {});
    case 'leave': {
      const d = kidsOf(spec, 'direction')[0];
      if (d) return act('VB_LEAVE', { direction: dirCode(d, env) });
      return direct[0] ? withObj('VB_LEAVE') : act('VB_LEAVE', {});
    }
    case 'climb': case 'dig': {
      const d = kidsOf(spec, 'direction')[0];
      const action = r === 'climb' ? 'VB_CLIMB' : 'VB_DIG';
      if (!direct[0]) return act(action, { direction: d ? dirCode(d, env) : undefined });
      return withObj(action, { direction: d ? dirCode(d, env) : undefined });
    }
    case 'wake': return direct[0] ? withObj('VB_WAKE') : act('VB_WAKE', {});
    case 'hide': {
      const loc = kidsOf(spec, 'locative')[0];
      const c = loc ? objRole(findFirst(loc, 'object'), env, ctx) : { value: undefined };
      if (c.stop) return c.stop;
      const o = direct[0] ? objOf(0) : { value: undefined };   // (A thing hidden: "hide the pouch")
      if (o.stop) return o.stop;
      return act('VB_HIDE', { object: o.value, container: c.value });
    }
    case 'sneak': case 'flee': {
      const d = kidsOf(spec, 'direction')[0];
      const action = r === 'sneak' ? 'VB_SNEAK' : 'VB_FLEE';
      return act(action, { direction: d ? dirCode(d, env) : undefined });
    }
    case 'knock': return withObj('VB_KNOCK_OBJECT');
    case 'push': return withObj('VB_PUSH_OBJECT');
    case 'pull': return withObj('VB_PULL_OBJECT');
    case 'lock': return withObj('VB_LOCK_OBJECT');
    case 'read': return withObj('VB_READ_OBJECT');
    case 'taste': return withObj('VB_TASTE_OBJECT');
    case 'touch': return withObj('VB_TOUCH_OBJECT');
    case 'follow': return withObj('VB_FOLLOW_OBJECT');
    case 'give': {
      const a = objOf(0), b = objOf(1);
      if (a.stop) return a.stop;
      if (b.stop) return b.stop;
      return act('VB_GIVE_OBJECT', { object: a.value, target: b.value });
    }
    case 'unlock': {
      const a = objOf(0), b = objOf(1);
      if (a.stop) return a.stop;
      if (b.stop) return b.stop;
      return act('VB_UNLOCK_OBJECT', { object: a.value, instrument: b.value });
    }
    case 'fill': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      const from = kidsOf(spec, 'from-phrase')[0];
      const s = from ? objRole(findFirst(from, 'object'), env, ctx) : { value: undefined };
      if (s.stop) return s.stop;
      return act('VB_FILL_OBJECT', { object: o.value, source: s.value });
    }
    case 'defend': { const w = wordsOf(spec, env); return act('VB_DEFEND', { defence: w[0] === 'defend' ? w[1] || 'defend' : w[0] }); }
    case 'where': return act('VB_WHERE', {});
    case 'again': return act('VB_REPEAT', {});
    case 'knock-out': {
      const a = objOf(0);
      if (a.stop) return a.stop;
      return act('VB_ATTACK_OBJECT', { object: a.value, manner: 'unarmed', blow: 'knock out', engage: engageFlag(a.value, ctx) });
    }
    case 'grapple': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      if (!ctx.engaged.includes(o.value)) return err('NOT_ENGAGED');
      return act('VB_GRAPPLE_OBJECT', { object: o.value, maneuver: own(GRAPPLE_MANEUVER, wordsOf(spec, env)[0]) });
    }
    // grab and hold: a grapple on an engaged object; otherwise a get, or a carry for hold (the grammar has one phrase).
    case 'grab': {
      const word = wordsOf(spec, env)[0];
      const o = objOf(0);
      if (o.stop) return o.stop;
      if (ctx.engaged.includes(o.value)) return act('VB_GRAPPLE_OBJECT', { object: o.value, maneuver: word });
      if (word === 'hold') return act('VB_HOLD_OBJECT', { object: o.value });
      const from = kidsOf(spec, 'from-phrase')[0];
      const src = from ? objRole(findFirst(from, 'object'), env, ctx) : { value: undefined };
      if (src.stop) return src.stop;
      return act('VB_RETRIEVE_OBJECT', { object: o.value, source: src.value });
    }
    case 'engage': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      if (ctx.engaged.includes(o.value)) return err('ALREADY_ENGAGED');
      return act('VB_ENGAGE', { object: o.value });
    }
    case 'disengage': {
      if (!ctx.engaged.length) return err('NOT_ENGAGED');
      const d = kidsOf(spec, 'direction')[0];
      return act('VB_DISENGAGE', { direction: d ? dirCode(d, env) : undefined });
    }
    // Pass forfeits the turn: only when engaged. "pass the time" is a wait, in the wait rule.
    case 'pass': return ctx.engaged.length ? act('VB_PASS', {}) : err('NOT_ENGAGED');
    // Religion: prayer, intervention, invocations (performed, not cast); baptism is the invocation that converts.
    case 'pray': {
      const t = topicAfter(spec, env, ['for']);
      return t ? act('VB_PRAY_FOR', { task: t }) : act('VB_PRAY', {});
    }
    case 'intervene': return act('VB_CALL_INTERVENTION', { deity: textOf(findFirst(spec, 'DEITY'), env), intent: topicAfter(spec, env, ['for']) });
    case 'invoke': case 'baptise': {
      const o = direct[0] ? objOf(0) : { value: undefined };
      if (o.stop) return o.stop;
      if (r === 'baptise') return act('VB_INVOKE', { invocation: 'baptism', target: o.value, deity: textOf(findFirst(spec, 'DEITY'), env) || undefined });
      return act('VB_INVOKE', { invocation: textOf(findFirst(spec, 'INVOCATION'), env), target: o.value, deity: textOf(findFirst(spec, 'DEITY'), env) || undefined });
    }
    case 'learn-invocation': {
      const o = direct[0] ? objOf(0) : { value: undefined };
      if (o.stop) return o.stop;
      return act('VB_LEARN_INVOCATION', { invocation: textOf(findFirst(spec, 'INVOCATION'), env), source: o.value });
    }
    case 'fasting': return act('VB_FASTING', {});
    // Magic: spells are memorized, learned, researched, countered; dispelling and releasing are the item cases.
    case 'memorize': return act('VB_MEMORIZE', { spell: textOf(findFirst(spec, 'spell-word'), env) });
    case 'unmemorize': return act('VB_UNMEMORIZE', { spell: textOf(findFirst(spec, 'spell-word'), env) });
    case 'research': return act('VB_RESEARCH', { spell: textOf(findFirst(spec, 'spell-word'), env) });
    case 'counter': return act('VB_COUNTER', { spell: textOf(findFirst(spec, 'spell-word'), env) });
    case 'learn-spell': {
      const spell = textOf(findFirst(spec, 'spell-word'), env);
      if (INVOCATION_WORDS.includes(spell)) return err('NOT_UNDERSTOOD');   // "learn divination" is an invocation
      const o = direct[0] ? objOf(0) : { value: undefined };
      if (o.stop) return o.stop;
      return act('VB_LEARN_SPELL', { spell, source: o.value });
    }
    case 'dispel': return onTarget('VB_DISPEL', true);
    case 'release': return withObj('VB_RELEASE_PRINCIPLE');
    // Psionics: a talent is used on a target; mental conflict and artifacts.
    case 'talent': {
      const o = direct[0] ? objOf(0) : { value: undefined };
      if (o.stop) return o.stop;
      return act('VB_USE_TALENT', { talent: textOf(findFirst(spec, 'TALENT'), env), target: o.value });
    }
    case 'possess': return onTarget('VB_POSSESS');
    case 'link-mind': return onTarget('VB_LINK_MIND');
    case 'activate': return withObj('VB_ACTIVATE');
    // Influence: Rhetoric and its specialties. Taunt and fast-talk are Rhetoric: persuade to attack, or to believe you.
    case 'intimidate': return onTarget('VB_INTIMIDATE');
    case 'order': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      return act('VB_ORDER', { target: o.value, action: topicAfter(spec, env, ['to']) });
    }
    case 'persuade': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      return act('VB_PERSUADE', { target: o.value, topic: topicAfter(spec, env, ['to']) });
    }
    case 'orate': {
      const o = direct[0] ? objOf(0) : { value: undefined };
      if (o.stop) return o.stop;
      return act('VB_ORATE', { audience: o.value, topic: topicAfter(spec, env, ['about']) });
    }
    case 'negotiate': return direct[0] ? onTarget('VB_NEGOTIATE') : act('VB_NEGOTIATE', {});
    case 'fast-talk': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      return act('VB_FAST_TALK', { target: o.value, topic: topicAfter(spec, env, ['about']) });
    }
    case 'taunt': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      return act('VB_TAUNT', { target: o.value, topic: topicAfter(spec, env, ['to']) });
    }
    case 'judge': return onTarget('VB_JUDGE_SINCERITY');
    case 'talk': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      return act('VB_TELL_OBJECT', { object: o.value, topic: topicAfter(spec, env, ['about']) });
    }
    case 'sit-on': return act('VB_SIT', {});
    case 'posture': return act(own(POSTURE, spec.kids[0].rule), {});
    // The game's own (the old parser's commands: commands.words says what the game does with each)
    case 'break': return optObj('VB_BREAK_OBJECT');
    case 'raise': return optObj('VB_RAISE_OBJECT');
    case 'lower': return optObj('VB_LOWER_OBJECT');
    case 'bend': return optObj('VB_BEND_OBJECT');
    case 'force': return optObj('VB_FORCE_OBJECT');
    case 'disarm': return optObj('VB_DISARM_OBJECT');
    case 'rebar': return optObj('VB_BAR_OBJECT');
    case 'approach': return withObj('VB_APPROACH_OBJECT');
    case 'hang': case 'combine': {
      const o = objOf(0);
      if (o.stop) return o.stop;
      const p = direct[1] ? objOf(1) : { value: undefined };
      if (p.stop) return p.stop;
      return r === 'hang' ? act('VB_HANG_OBJECT', { object: o.value, support: p.value }) : act('VB_COMBINE_OBJECT', { object: o.value, other: p.value });
    }
    case 'deceive': return onTarget('VB_DECEIVE');
    case 'threaten': return onTarget('VB_THREATEN');
    case 'calm': return onTarget('VB_CALM');
    case 'befriend': return onTarget('VB_BEFRIEND');
    case 'ally': {
      const w = wordsOf(spec, env)[0];
      if (w === 'dismiss') return onTarget('VB_ALLY_DISMISS');
      return act(w === 'come' || w === 'heel' ? 'VB_ALLY_COME' : 'VB_ALLY_STAY', {});
    }
    case 'pretend': return act('VB_PRETEND', { state: textOf(findFirst(spec, 'pretense'), env) });
    default: return err('NOT_UNDERSTOOD');
  }
}

// A bare verb (no phrase): the class it belongs to decides the action.
function bareVerb(node, env, ctx) {
  const cls = node.kids[0].rule;
  switch (cls) {
    case 'GET': return phraseSem({ rule: 'get', start: 0, end: 0, kids: [] }, env, ctx);
    case 'LOOK': return act('VB_OBSERVE_LOCALE', { place: 'here' });
    case 'SEARCH': return act('VB_SEARCH', {});
    case 'LISTEN': return act('VB_LISTEN', {});
    case 'TRACK': return act('VB_TRACK', {});
    case 'WAIT': return act('VB_WAIT', { duration: '1min' });
    case 'REST': case 'SLEEP': return act('VB_SLEEP', { short: 'no' });
    case 'STOP': return act('VB_STOP', { task: 'current' });
    case 'META-WORD': return act(own(META, wordsOf(node, env)[0]), {});
    case 'AGAIN': return act('VB_REPEAT', {});
    default: return err('NOT_UNDERSTOOD');
  }
}

// Volume: the w or y prefix, a trailing "!", or yell/scream/whisper.
function sayRoles(node, env, verbWord) {
  const q = node.rule === 'quoted-text' ? node : findFirst(node, 'quoted-text');
  const tk = env.toks[q.start];
  let volume = 'normal';
  if (tk.vol === 'w') volume = 'quiet';
  else if (tk.vol === 'y') volume = 'loud';
  else if (tk.text.endsWith('!') || (node.rule === 'quoted-speech' && env.toks[node.end - 1].k === 'w' && env.toks[node.end - 1].v === '!')) volume = 'loud';
  if (verbWord === 'whisper') volume = 'quiet';
  else if (['yell', 'scream', 'shout'].includes(verbWord)) volume = 'loud';
  return act('VB_SAY', { text: tk.text, volume });
}

// ---- lines ----------------------------------------------------------------------------------------------------------

const countAdjuncts = p => countRule(p.kids[0], 'adjunct');

function numberOfTok(tk) {
  if (!tk || tk.k !== 'w') return null;
  if (/^\d+$/.test(tk.v)) return Number(tk.v);
  return numWord(tk.v) ?? null;
}

// An attack on an enemy not yet engaged is an Engage option: a half-move in, then the blow (Combat 7).
const engageFlag = (target, ctx) => (ctx.engaged.includes(target) ? undefined : 'yes');

// While engaged, a character moves only by disengaging, and rests or sits only when unengaged (Combat 6-7).
const NEEDS_FREE = new Set(['VB_MOVE_DIRECTION', 'VB_MOVE_TOWARD', 'VB_STEP_DIRECTION', 'VB_CHARGE', 'VB_SLEEP',
  'VB_SNEAK', 'VB_ENTER', 'VB_LEAVE', 'VB_CLIMB', 'VB_SWIM', 'VB_JUMP', 'VB_HIDE', 'VB_SIT', 'VB_LIE_DOWN', 'VB_CROUCH', 'VB_WAIT']);
function engagementCheck(res, ctx) {
  if (res.error || res.question || !ctx.engaged.length) return res;
  if (res.action === 'VB_FLEE') return act('VB_DISENGAGE', { direction: res.roles.direction, pace: 'run' });
  return NEEDS_FREE.has(res.action) ? err('ENGAGED') : res;
}

// The words after the first marker word ("for", "to", "about"): a prayer's endeavour, an order's action, a topic.
function topicAfter(spec, env, markers) {
  const words = wordsOf(spec, env);
  const i = words.findIndex(w => markers.includes(w));
  return i < 0 || i === words.length - 1 ? undefined : words.slice(i + 1).join(' ');
}

// The named invocations (HarnMaster Religion); "learn" with one of these is an invocation, not a spell.
const INVOCATION_WORDS = ['divination', 'baptism', 'passage', 'consecration', 'exorcism', 'curse', 'blessing', 'balefire'];

// The phrase with its first object replaced: the verb is kept, the object is new.
function replaceObject(spec, obj) {
  let done = false;
  return Object.assign({}, spec, { kids: spec.kids.map(k => {
    if (!done && k.rule === 'object') { done = true; return obj; }
    if (!done && k.rule === 'thing') { done = true; return Object.assign({}, k, { kids: [obj] }); }   // (A thing in the place: its object new, where it lies with the old)
    return k;
  }) });
}

// A single-word alias at the start of a command, or of a part after "then", stands for its verb (in place).
// A default or player alias at the start of a command, or of a part after "then", stands for its verb (in place);
// a direction alias (DEFAULT_ANYWHERE) stands for its short form anywhere.
// The longest alias whose words start here wins; a key may be several words ("look closely").
function segmentAliases(toks, ctx) {
  for (let i = 0; i < toks.length; i++) {
    const segStart = i === 0 || (toks[i - 1].k === 'w' && toks[i - 1].v === 'then');
    let best = null;
    for (const key of ctx.aliases.keys()) {
      // a command's verb is aliased at its start; a direction word (DEFAULT_ANYWHERE) anywhere
      if (!segStart && !DEFAULT_ANYWHERE.has(key)) continue;
      const kw = key.split(' ');
      if (['alias', 'unalias', 'forget'].indexOf(kw[0]) >= 0 || i + kw.length > toks.length) continue;
      if (!kw.every((w, j) => toks[i + j].k === 'w' && toks[i + j].v === w)) continue;
      if (!best || kw.length > best.n) best = { key, n: kw.length };
    }
    if (!best) continue;
    const verb = ctx.aliases.get(best.key);
    if (verb.indexOf(' ') < 0) toks.splice(i, best.n, { k: 'w', v: verb });
  }
  return toks;
}

// The text of some tokens, as a line would type them (words, and quoted text with its w or y).
function tokensText(ts) {
  return ts.map(t => (t.k === 'w' ? t.v : (t.vol || '') + '"' + t.text + '"')).join(' ');
}

class Compiler {
  constructor(rules) {
    this.rules = rules;
    this.reserved = literalWords(rules);
  }

  envFor(toks, ctx) {
    return { rules: this.rules, toks, memo: new Map(), reserved: this.reserved, world: ctx.world, names: ctx.names, deities: ctx.deities };
  }

  // Compile one line in a context: returns {action, roles} | {question, roles} | {error, roles}.
  compile(line, ctx) {
    const toks = tokenize(line);
    if (!toks.length) return err('NOT_UNDERSTOOD');
    // A pending question is answered first: "n" answers a yes-or-no question, it is not north.
    if (ctx.pending) {
      const a = this.answer(toks, ctx);
      if (a) return a;
    }
    // A word that stands for several words expands once, with the rest of the line added (the rest keeps its aliases).
    // A single word is handled by segmentAliases below.
    if (toks[0].k === 'w' && ctx.aliases.has(toks[0].v) && ctx.aliases.get(toks[0].v).indexOf(' ') >= 0 && ['alias', 'unalias', 'forget'].indexOf(toks[0].v) < 0) {
      const key = toks[0].v;
      const expanded = ctx.aliases.get(key) + ' ' + toks.slice(1).map(t => (t.k === 'w' ? t.v : '"' + t.text + '"')).join(' ');
      return this.compile(expanded.trim(), Object.assign({}, ctx, { aliases: new Map([...ctx.aliases].filter(([k]) => k !== key)) }));
    }
    segmentAliases(toks, ctx);
    const aliasResult = this.alias(toks, ctx);
    if (aliasResult) return aliasResult;
    const env = this.envFor(toks, ctx);
    const parses = matchRule('line', 0, env).filter(r => r.end === toks.length);
    if (!parses.length) return this.verbAnd(toks, ctx) || this.fallback(toks, env);
    // A parse that puts fewer words in adjuncts is the reading the grammar means: "look at the wall" is a look at a
    // thing, not a look with an "at the wall" adjunct.
    parses.sort((a, b) => countAdjuncts(a) - countAdjuncts(b));
    const results = parses.map(p => this.semantics(p.kids[0], env, ctx));
    return results.find(r => !r.error) || results[0];
  }

  // Two verbs with one object: "take and stow the sword" is "take the sword", then "stow the sword" (a sequence).
  verbAnd(toks, ctx) {
    // The verbs are the words at even places, joined by "and"; the object follows them all, once.
    let groups = 0;
    while (2 * groups + 2 < toks.length && toks[2 * groups + 1].k === 'w' && toks[2 * groups + 1].v === 'and' && toks[2 * groups + 2].k === 'w') groups++;
    for (let k = groups; k >= 1; k--) {
      if (toks.length <= 2 * k + 1) continue;
      const rest = tokensText(toks.slice(2 * k + 1));
      const steps = [];
      for (let i = 0; i <= k; i++) steps.push(this.compile(toks[2 * i].v + " " + rest, ctx));
      if (steps.every(r => !r.error && !r.question)) return sequenceOf(steps);
    }
    return null;
  }

  // Lines with no parse: the few that have a reason of their own.
  fallback(toks, env) {
    const w = toks.map(t => (t.k === 'w' ? t.v : null));
    if (toks.length === 1 && MOVE_WORDS.has(w[0])) return question('DIRECTION', {});
    if (SPEECH_WORDS.has(w[0]) && !toks.some(t => t.k === 'q')) return err('QUOTES_REQUIRED');
    if (INTRANSITIVE.has(w[0]) && toks.length > 1) {
      const rest = Object.assign({}, env, { toks: toks.slice(1), memo: new Map() });
      if (matchRule('object', 0, rest).some(r => r.end === toks.length - 1)) return err('BAD_ROLE');
    }
    return err('NOT_UNDERSTOOD');
  }

  // The answer to a pending question, or null when the line is a fresh command.
  answer(toks, ctx) {
    const p = ctx.pending;
    const text = toks.map(t => (t.k === 'w' ? t.v : t.text)).join(' ');
    const n = toks.length === 1 ? numberOfTok(toks[0]) : null;
    if (p.kind === 'overwrite') {
      if (['yes', 'y'].includes(text)) return act('ALIAS:REPLACE', { alias: p.alias, substitution: p.next });
      if (['no', 'n'].includes(text)) return act('ALIAS:KEEP', { alias: p.alias });
      return null;
    }
    if (p.kind === 'distance') return n === null ? null : act('VB_THROW_DIRECTION', { distance: n });
    if (p.kind === 'which') {
      let pick = null;
      if (n !== null) pick = p.choices[n - 1];
      else if (/^(the )?(closest|nearest)$/.test(text)) {
        pick = p.choices.slice().sort((a, b) => (ctx.dist.get(a) ?? 0) - (ctx.dist.get(b) ?? 0))[0];
      } else pick = p.choices.find(c => c.toLowerCase() === text);
      return pick ? act('VB_ATTACK_OBJECT', { object: pick }) : null;
    }
    return null;
  }

  // Alias commands: define (is or =), show, list, remove; the built-in and use-a-name refusals.
  alias(toks, ctx) {
    const w = toks.map(t => (t.k === 'w' ? t.v : null));
    if (w[0] === 'unalias' || (w[0] === 'forget' && w[1] === 'alias')) {
      return act('ALIAS:REMOVE', { alias: w[0] === 'unalias' ? w[1] : w[2] });
    }
    if (w[0] !== 'alias') return null;
    if (toks.length === 1) return act('ALIAS:LIST', Object.fromEntries(ctx.aliases));
    const sep = w.findIndex((x, i) => i >= 2 && (x === 'is' || x === '='));
    if (sep < 0 && w.includes('as')) return err('USE_NAME');
    if (sep < 0) {
      if (this.reserved.has(w[1]) && !ctx.world.has(w[1])) return err('BUILT_IN_WORD', { alias: w[1] });
      return act('ALIAS:SHOW', { alias: w[1], substitution: ctx.aliases.get(w[1]) });
    }
    const name = w[1];
    if (this.reserved.has(name) && !ctx.world.has(name)) return err('BUILT_IN_WORD', { alias: name });
    const substitution0 = toks.slice(sep + 1).map(t => (t.k === 'w' ? t.v : `"${t.text}"`)).join(' ');
    // An alias of an alias points at the core word, as it is now: "alias yyy is x" means what x means.
    const first = substitution0.split(' ')[0];
    const substitution = ctx.aliases.has(first) && first !== name
      ? [ctx.aliases.get(first), substitution0.slice(first.length + 1)].filter(Boolean).join(' ') : substitution0;
    if (ctx.aliases.has(name)) return question('OVERWRITE', { alias: name, current: ctx.aliases.get(name), new: substitution });
    return act('ALIAS:DEFINE', { alias: name, substitution });
  }

  // A sequence: each part is a whole action, in order; a part that does not compile stops the line with its error.
  sequenceSem(node, env, ctx) {
    const results = [];
    let prev = null;
    for (const p of kidsOf(node, 'seq-part')) {
      const part = p.kids[0];
      if (part.rule === 'object') {
        // An object alone repeats the verb of the part before it: "attack the rat then the goblin".
        if (!prev) return err('NOT_UNDERSTOOD');
        const steps = expandLists(replaceObject(prev, part)).map(x => engagementCheck(phraseSem(x, env, ctx), ctx));
        results.push(sequenceOf(steps));
      } else {
        prev = part.rule === 'command' ? part.kids[0].kids[0] : null;
        results.push(this.nodeSem(part, env, ctx));
      }
    }
    return sequenceOf(results);
  }

  semantics(line, env, ctx) {
    return this.nodeSem(line.kids[0], env, ctx);
  }

  nodeSem(child, env, ctx) {
    return engagementCheck(this.rawSem(child, env, ctx), ctx);
  }

  rawSem(child, env, ctx) {
    switch (child.rule) {
      case 'command': {
        const spec = child.kids[0].kids[0];
        const steps = expandLists(spec).map(s => withAdjuncts(phraseSem(s, env, ctx), child, env, ctx));
        if (steps.length === 1) return steps[0];
        // A list of attacks is one attack a turn, split between the targets (Combat 9); other lists are sequences.
        if (spec.rule === 'attack' || spec.rule === 'knock-out') {
          const bad = steps.find(r => r.error || r.question);
          return bad || act('VB_ATTACK_SPLIT', { targets: steps.map(r => r.roles.object).join(',') });
        }
        return sequenceOf(steps);
      }
      case 'bare-direction': return act('VB_MOVE_DIRECTION', { direction: dirCode(findFirst(child, 'direction'), env),
        distance: numberOf(findFirst(child, 'number'), env) });
      case 'bare-verb': return bareVerb(child, env, ctx);
      case 'quoted-speech': return sayRoles(child, env, null);
      case 'sequence': return this.sequenceSem(child, env, ctx);
      default: return err('NOT_UNDERSTOOD');
    }
  }
}

// ---- the corpus -----------------------------------------------------------------------------------------------------

function kv(s) {
  const o = {};
  if (!s || s === '-') return o;
  for (const pair of s.split(';')) {
    const eq = pair.indexOf('=');
    if (eq > 0) o[pair.slice(0, eq)] = pair.slice(eq + 1);
  }
  return o;
}

// Each corpus case: the input, its expected code, the roles it must have (others are not checked), and the context.
function checkCorpus(compiler, corpusFile, log) {
  const lines = fs.readFileSync(corpusFile, 'utf8').replace(/\r\n/g, '\n').split('\n');
  let pass = 0, total = 0;
  for (const raw of lines) {
    if (!raw.trim() || raw.startsWith('#')) continue;
    const [input, expect, roles, context] = raw.split('\t');
    total++;
    const got = compiler.compile(input, buildContext(context || '-', DEFAULT_WORLD));
    const gotCode = got.error ? 'ERROR:' + got.error : got.question ? 'QUESTION:' + got.question : got.action;
    // A sequence's roles are also checked per step: 2.object is the object of the second step.
    const have = Object.assign({}, got.roles || {});
    (got.steps || []).forEach((st, i) => Object.entries(st.roles).forEach(([k, v]) => { have[`${i + 1}.${k}`] = v; }));
    const bad = [];
    if (gotCode !== expect) bad.push(`action ${gotCode} (want ${expect})`);
    for (const [k, want] of Object.entries(kv(roles))) {
      if (want === '-' || k === 'choices') continue;
      const h = have[k] === undefined ? '' : String(have[k]).toLowerCase();
      if (h !== want.toLowerCase()) bad.push(`${k}=${h || '(none)'} (want ${want})`);
    }
    if (!bad.length) pass++;
    else log(`${input}  ->  ${bad.join('; ')}`);
  }
  return { pass, total };
}

// The open names' word lists, for the 65C02 matcher's index table (commandtables.js's lexicon)
const LEXICON = { adj: [...ADJ], tasks: TASKS, ordinals: ORDINALS, numbers: Object.keys(NUMWORDS), spellStop: [...SPELL_STOP],
  invocations: INVOCATION_WORDS, notObject: [...NOT_OBJECT] };

module.exports = { Compiler, loadRules, buildContext, tokenize, DEFAULT_WORLD, checkCorpus, matchRule, countRule, DEFAULT_ALIASES, DEFAULT_ANYWHERE, segmentAliases, LEXICON };

if (require.main === module) {
  const compiler = new Compiler(loadRules(path.join(GAME, 'commands.ebnf')));
  if (process.argv.length > 2) {
    console.log(JSON.stringify(compiler.compile(process.argv.slice(2).join(' '), buildContext('-', DEFAULT_WORLD))));
  } else {
    const { pass, total } = checkCorpus(compiler, path.join(GAME, 'commands.corpus'), console.log);
    console.log(`corpus: ${pass}/${total} cases pass`);
    process.exit(pass === total ? 0 : 1);
  }
}
