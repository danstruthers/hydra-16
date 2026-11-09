// hubgen.js - the hub's numbers for the other languages, made by the build from the C headers as the compiler sees
// them (ENGINE-CRITIQUE.md's §2, step 3's first stage: nothing written twice).  The headers (hub.h and those it
// includes: rs.h, level.h, ../dq0w/world.h, items.h, theme.h; and dq0.h, the UI's, and life.h, for the command tools)
// are the one place a number is stated; build.js calls generate() before it builds the games (hubNumbers), and
// everything else reads what it writes:
//   - obj/gen/actions.h: the actions' enum (VB_NONE, the catalogue's actions in its order, NACTIONS), from
//     commands.actions, the one list of them (the C's #include "actions.h", found there: build.js's -I obj/gen);
//   - obj/gen/dq0hub.json: every #define that's a number and every enum's member (C: NAME -> value; enums: each
//     enum's members in order), every named struct's layout (S: NAME -> { size, f: PATH -> [offset, size, then each
//     array dimension's element size] }, PATH its fields and theirs, "tb.at", "door.rel": an array's first element),
//     a few function-like macros' values (EXTRA), and the writers' table (own: hub.h's owners block, its regions'
//     bytes, for sim/lib/hubown.js).  load() reads it: { C, enums, off (struct, path), size (struct, path), dims,
//     words (an enum's members as words), value (an ASM spec), own, tasks }; the make.js files (dq0's theme compiler,
//     dq0w's, dq0d's, dq0s's), sim/lib's recorder and writers' check, tests.js's offsets (hubown.evaluate) and the
//     command tools (commandwords.js) use it;
//   - obj/gen/dq0hub_NAME.inc: an assembly file's names (ASM below: each consumer the names it uses, each a number
//     of the headers', a field's offset or a size), .included where its hand-typed copies were.
// The values come from cc65 itself: a C file of `const long vN = (long) (EXPR);`, one for each name, field and size,
// compiled once and read back from its assembly (an expression cc65 can't make a constant of, a #define of code, is
// left out: its line dropped and the file compiled again).  Made again only when an input changes (its key: the
// headers', the catalogue's and this file's hash).
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { execFileSync } = require('child_process');

const ROOT = path.join(__dirname, '..', '..');                  // (reborn/)
const DQ0 = __dirname;
const GEN = path.join(ROOT, 'obj', 'gen');
const JSON_OUT = path.join(GEN, 'dq0hub.json');
const HEADERS = ['hub.h', 'rs.h', 'level.h', path.join('..', 'dq0w', 'world.h'), 'items.h', 'theme.h', 'dq0.h', 'life.h'];
const EXTRA = ['ITEM_BANK (0)', 'CACHE_BANK (0)', 'CHANGE_BANK (0)'];   // (Function-like macros' values the others want)

// Each assembly file's names: NAME: what it is, a header's name (its value), STRUCT.PATH (a field's offset), sizeof
// STRUCT or sizeof STRUCT.PATH (a size).  The file obj/gen/dq0hub_KEY.inc
const ASM = {
  dq0c: {                                                       // games/dq0c/dq0c.s: the hub's places it reads, the
    HUB_WINDOW: 'HUB_WINDOW', HUB_MAGIC: 'HUB_MAGIC', HUB_VERSION: 'HUB_VERSION',                     //   request's
    HUBO_SEM: 'hub.sem', HUBO_TASK: 'hub.task', HUBO_BEAT: 'hub.beat', HUBO_CQ: 'hub.cq', HUBO_QUIT: 'hub.quit',
    RQ_STATE: 'rqh.state', RQ_KIND: 'rqh.kind', RQ_FROM: 'rqh.from', RQ_SEQ: 'rqh.seq',
    CQO_BANK: 'cq.bank', CQO_LEN: 'cq.len', CQO_RES: 'cq.res', CQO_FACE: 'cq.face',
    CQ_WORDS: 'CQ_WORDS', CQ_CMD: 'CQ_CMD', CMD_SIZE: 'sizeof cmd',
    REQ_FREE: 'REQ_FREE', REQ_POSTED: 'REQ_POSTED', REQ_DONE: 'REQ_DONE', CQ_NONE: 'CQ_NONE', CQ_MATCH: 'CQ_MATCH', CQ_WORLD: 'CQ_WORLD',
  },
  cmdmean: {                                                    // games/dq0c/cmdmean.s: struct cmd, struct cthing
    CM_ACTION: 'cmd.action', CM_FLAGS: 'cmd.flags', CM_DIR: 'cmd.dir', CM_HOW: 'cmd.how', CM_PREP: 'cmd.prep', CM_AUX: 'cmd.aux',
    CM_MANNER: 'cmd.manner', CM_ORDER: 'cmd.order', CM_DFLT: 'cmd.dflt', CM_SKILL: 'cmd.skill', CM_TASK: 'cmd.task', CM_N: 'cmd.n',
    CM_OBJ: 'cmd.obj', CM_OTHER: 'cmd.other', CM_SIZE: 'sizeof cmd',
    T_KIND: 'cthing.kind', T_SEL: 'cthing.sel', T_N: 'cthing.n', T_LIES: 'cthing.lies', T_WORD: 'cthing.word', T_WORDSIZE: 'sizeof cthing.word',
    CF_NOVERB: 'CF_NOVERB', CF_HALF: 'CF_HALF', CF_UNSAID: 'CF_UNSAID', CF_STRANGE: 'CF_STRANGE', CQ_REL: 'CQ_REL',
  },
  line: {                                                       // games/dq0w/line.inc: the squares, a portal
    T_WALL: 'T_WALL', T_DOOR: 'T_DOOR', T_WINDOW: 'T_WINDOW', P_KIND: 'portal.kind', P_STATE: 'portal.state', P_PARTS: 'portal.parts',
    PK_WINDOW: 'PK_WINDOW', PK_PORTCULLIS: 'PK_PORTCULLIS', PK_GRATE: 'PK_GRATE', PK_SLIT: 'PK_SLIT',
    PP_GLASS: 'PP_GLASS', PP_SHUTTERS: 'PP_SHUTTERS', PS_OPEN: 'PS_OPEN', PS_WRECKED: 'PS_WRECKED',
  },
  items: {                                                      // games/dq0w/items.s: the item store
    REC: 'sizeof item', NITEMS: 'NITEMS', BANK0: 'ITEM_BANK (0)', ITEM_BANKS: 'ITEM_BANKS',
    I_KIND: 'item.kind', I_FLAGS: 'item.flags', I_WHERE: 'item.where', I_X: 'item.x',
  },
  letter: { NLET: 'NLET', LT_REC: 'LT_REC', CIX: 'sizeof cix', CIX_LET: 'cix.let' },   // games/dq0w/letter.s
  pulse: { SND_GRID: 'SND_GRID', SND_W: 'SND_W' },              // games/dq0p/pulse.s: the sound's grid
  at: { CW: 'CW' },                                             // games/dq0w/at.s: a chunk's width
  lbsaid: { TOLD: 'TOLD' },                                     // games/dq0/lbsaid.s: a look's sentences kept (dq0.h's)
};

// ---- The headers read: their text less comments, a backslash's line joined to the next
function clean(text) {
  return text.replace(/\/\*[^]*?\*\//g, m => m.replace(/[^\n]/g, ' ')).replace(/\/\/.*$/gm, '').replace(/\\\r?\n/g, ' ');
}

// The #defines (object-like: { name: body }; a function-like one's left out: EXTRA has those wanted) and the enums
// (each its members)
function scan(texts) {
  const defs = {}, enums = [];
  for (const t of texts) {
    for (const m of t.matchAll(/^[ \t]*#[ \t]*define[ \t]+(\w+)(\(?)(.*)$/gm)) if (!m[2]) defs[m[1]] = m[3].trim();
    for (const m of t.matchAll(/\benum\b\s*\w*\s*\{([^}]*)\}/g))
      enums.push(m[1].split(',').map(s => (s.trim().match(/^(\w+)/) || [])[1]).filter(Boolean));
  }
  return { defs, enums };
}

// A body split at its top level's separator (';' or ','), brackets and braces kept whole
function split(s, sep) {
  const out = [];
  let d = 0, cur = '';
  for (const c of s) {
    if (c === sep && !d) { out.push(cur); cur = ''; continue; }
    if ('([{'.includes(c)) d++;
    if (')]}'.includes(c)) d--;
    cur += c;
  }
  out.push(cur);
  return out.map(x => x.trim()).filter(Boolean);
}

// The named structs: NAME -> its body's text
function structs(texts) {
  const out = {};
  for (const t of texts) for (const m of t.matchAll(/\bstruct\s+(\w+)\s*\{/g)) {
    let d = 1, i = m.index + m[0].length;
    for (; i < t.length && d; i++) if (t[i] === '{') d++; else if (t[i] === '}') d--;
    out[m[1]] = t.slice(m.index + m[0].length, i - 1);
  }
  return out;
}

// A body's members: [{ name, dims: [C expression], sub: a struct's body (a named struct's, or one of its own), or
// null }]; a member that's a macro alone (RQH) its #define's text
function members(body, defs, named) {
  const out = [];
  for (let decl of split(body.replace(/^[ \t]*#.*$/gm, ''), ';')) {     // (A #define among its fields left out)
    if (/^\w+$/.test(decl) && defs[decl] !== undefined) { out.push(...members(defs[decl], defs, named)); continue; }
    let sub = null, list;
    const b = decl.indexOf('{');
    if (b >= 0) {
      let d = 0, e = b;
      for (; e < decl.length; e++) if (decl[e] === '{') d++; else if (decl[e] === '}' && !--d) break;
      sub = decl.slice(b + 1, e);
      list = split(decl.slice(e + 1), ',').map(x => ({ x, ptr: x.includes('*') }));
    } else {
      const parts = split(decl, ','), first = parts[0].match(/^(.*?)([\s*]+)(\w+\s*(?:\[[^\]]*\]\s*)*)$/);
      if (!first) continue;
      const named_ = first[1].match(/^(?:const\s+|volatile\s+)*struct\s+(\w+)$/);
      if (named_ && named[named_[1]] !== undefined) sub = named[named_[1]];
      list = [{ x: first[3], ptr: first[2].includes('*') }, ...parts.slice(1).map(x => ({ x, ptr: x.includes('*') }))];
    }
    for (const { x, ptr } of list) {
      const m = x.replace(/^\**\s*/, '').match(/^(\w+)\s*((?:\[[^\]]*\]\s*)*)$/);
      if (!m) continue;
      out.push({ name: m[1], dims: [...m[2].matchAll(/\[([^\]]*)\]/g)].map(k => k[1]), sub: ptr ? null : sub });
    }
  }
  return out;
}

// A struct's fields, flattened: [{ path ("tb.at", "door.rel"), c (offsetof's: "door[0].rel"), dims }]
function fields(body, defs, named, pre = '', cpre = '', depth = 0) {
  const out = [];
  if (depth > 6) return out;
  for (const m of members(body, defs, named)) {
    const p = pre + m.name, c = cpre + m.name;
    out.push({ path: p, c, dims: m.dims.length });
    if (m.sub !== null) out.push(...fields(m.sub, defs, named, p + '.', c + '[0]'.repeat(m.dims.length) + '.', depth + 1));
  }
  return out;
}

// ---- The writers' table (hub.h's owners block, between its "own:" and "own." lines): its lines, each
// { region, owner, plus: [task], known: [{ move, task }], line }
const RECS = { hub: 'struct hub', sight: 'struct sight', uirec: 'struct uirec', wrec: 'struct wrec', chunk: 'struct chunk', item: 'struct item' };
function ownTable(text, tasks) {
  const out = [], lines = text.split(/\r?\n/);
  const a = lines.findIndex(l => /^\*\*\s+own:\s*$/.test(l)), b = lines.findIndex(l => /^\*\*\s+own\.\s*$/.test(l));
  if (a < 0 || b < a) throw new Error('games/dq0/hub.h: no writers\' table (its "own:" and "own." lines)');
  for (let i = a + 1; i < b; i++) {
    const l = lines[i];
    if (/^\*\*\s*$/.test(l) || /^\*\*\s{20,}\S/.test(l)) continue;   // (A line blank, or a line's words going on)
    const k = l.match(/^\*\*   own (\S.*)$/);
    if (!k) throw new Error('hub.h:' + (i + 1) + ': not an own line in the writers\' table: ' + l.trim());
    const words = k[1].replace(/\s+\(.*$/, '').trim().split(/\s+/), region = words.shift(), owner = words.shift();
    const role = t => { if (!tasks.includes(t) && t !== 'each' && t !== 'none') throw new Error('hub.h:' + (i + 1) + ': no task ' + t); return t; };
    const e = { region, owner: role(owner), plus: [], known: [], line: i + 1 };
    for (const w of words) {
      if (w.startsWith('+')) e.plus.push(role(w.slice(1)));
      else if (/^\d+\.\d+:/.test(w)) { const [move, ts] = w.split(':'); for (const t of ts.split(',')) e.known.push({ move, task: role(t) }); }
      else throw new Error('hub.h:' + (i + 1) + ': "' + w + '"');
    }
    out.push(e);
  }
  return out;
}

// A region's arguments, split at their own commas: bank(DEN_BANK,SND_AT,FIGHT_AT) -> ['DEN_BANK', 'SND_AT', 'FIGHT_AT']
const args = s => split(s, ',');

// The C expressions the table's regions want (each line's ex: its indices in exprs)
function ownExprs(lines, idx) {
  for (const l of lines) {
    const r = l.region, f = r.match(/^(\w+)\((.*)\)$/);
    if (f) {
      const a = args(f[2]);
      if (f[1] === 'bank' && a.length === 1) l.ex = { kind: 'bank', b: idx(a[0]), n: idx('1') };
      else if (f[1] === 'bank' && a.length === 3) l.ex = { kind: 'range', b: idx(a[0]), from: idx('(unsigned) (' + a[1] + ')'), to: idx('(unsigned) (' + a[2] + ')') };
      else if (f[1] === 'banks' && a.length === 2) l.ex = { kind: 'bank', b: idx(a[0]), n: idx(a[1]) };
      else throw new Error('hub.h:' + l.line + ': ' + r);
      continue;
    }
    const d = r.indexOf('.'), rec = d < 0 ? r : r.slice(0, d), field = d < 0 ? '' : r.slice(d + 1), st = RECS[rec];
    if (!st) throw new Error('hub.h:' + l.line + ': ' + r);
    const p = st + '*', m = field.match(/^(.*?)\[\*\](.*)$/);
    if (!field) l.ex = { kind: 'rec', rec, off: idx('0'), size: idx('sizeof (' + st + ')'), stride: idx('1'), count: idx('1') };
    else if (!m) l.ex = { kind: 'rec', rec, off: idx('offsetof (' + st + ', ' + field + ')'), size: idx('sizeof (((' + p + ') 0)->' + field + ')'), stride: idx('1'), count: idx('1') };
    else {
      const el = m[1] + '[0]' + m[2];
      l.ex = { kind: 'rec', rec, off: idx('offsetof (' + st + ', ' + el + ')'), size: idx('sizeof (((' + p + ') 0)->' + el + ')'),
        stride: idx('sizeof (((' + p + ') 0)->' + m[1] + '[0])'), count: idx('sizeof (((' + p + ') 0)->' + m[1] + ') / sizeof (((' + p + ') 0)->' + m[1] + '[0])') };
    }
  }
}

// The table's regions' bytes: each line's spans, [bank, from, to, element (-1: the whole)]
function ownSpans(lines, V, C) {
  const c = { banks: C.HUB_BANKS, ntasks: C.NTASKS, magic: C.HUB_MAGIC, cache0: C['CACHE_BANK (0)'], chunk: C.CHUNK_SIZE,
    slots: C.NCSLOTS, item0: C['ITEM_BANK (0)'], item: V.itemSize, items: C.NITEMS };
  const BANK = { hub: 0, sight: C.SIGHT_BANK, uirec: C.UI_BANK, wrec: C.WORLD_BANK };
  const places = rec => {                                       // (Each record's places: [bank, offset]; the records in
    if (rec === 'chunk') return Array.from({ length: c.slots }, (_, k) => [c.cache0 + Math.floor(k / 3), (k % 3) * c.chunk]);   //   a bank of their
    if (rec === 'item') { const per = Math.floor(0x2000 / c.item); return Array.from({ length: c.items }, (_, i) => [c.item0 + Math.floor(i / per), (i % per) * c.item]); }   //   own at its start)
    return [[BANK[rec], 0]];
  };
  const out = [];
  for (const l of lines) {
    const x = l.ex, spans = [], v = i => V.at[i];
    if (x.kind === 'bank') for (let b = 0; b < v(x.n); b++) spans.push([v(x.b) + b, 0, 0x2000, -1]);
    else if (x.kind === 'range') spans.push([v(x.b), v(x.from) - 0x8000, v(x.to) - 0x8000, -1]);
    else for (const [b, o] of places(x.rec)) for (let e = 0; e < v(x.count); e++) {
      const from = o + v(x.off) + e * v(x.stride);
      spans.push([b, from, from + v(x.size), v(x.count) > 1 ? e : -1]);
    }
    for (const s of spans) if (s[0] < 0 || s[0] >= c.banks || s[1] < 0 || s[2] > 0x2000 || s[1] > s[2])
      throw new Error('hub.h:' + l.line + ': ' + l.region + ' outside the hub: bank ' + s[0] + ' $' + s[1].toString(16) + '-$' + s[2].toString(16));
    out.push({ region: l.region, owner: l.owner, plus: l.plus, known: l.known, line: l.line, spans });
  }
  return { banks: c.banks, ntasks: c.ntasks, taskOff: V.taskOff, magic: c.magic, lines: out };
}

// ---- cc65: the expressions' values (a .c of constants, each a .dword in its .s), the lines it can't make a constant
// of left out.  OUT: [value or undefined]
function evaluate(exprs, cc65) {
  const dir = fs.mkdtempSync(path.join(require('os').tmpdir(), 'hubgen-'));
  const head = '#include <stddef.h>\n#include "dq0.h"\n#include "hub.h"\n', skip = new Set();   // (main.c's order)
  try {
    const c = path.join(dir, 'hub.c'), s = path.join(dir, 'hub.s');
    for (let pass = 0; ; pass++) {
      const lines = exprs.map((e, i) => skip.has(i) ? '\n' : 'const long v' + i + ' = (long) (' + e + ');\n');
      fs.writeFileSync(c, head + lines.join(''));
      try {
        execFileSync(cc65.bin, [...cc65.flags, '-o', s, c], { stdio: ['ignore', 'pipe', 'pipe'] });
        break;
      } catch (e) {
        const text = ((e.stderr || '') + (e.stdout || '')).toString(), bad = [];
        for (const m of text.matchAll(/hub\.c[:(](\d+)\)?:\s*Error/g)) bad.push(+m[1] - 4);   // (The file's line: its
        if (!bad.length || pass > 100) {                                                       //   expression's, after three)
          fs.copyFileSync(c, path.join(GEN, 'dq0hub_failed.c'));
          throw new Error('hubgen: cc65 (obj/gen/dq0hub_failed.c):\n' + text.trim().split('\n').slice(0, 20).join('\n'));
        }
        for (const i of bad) if (i >= 0 && i < exprs.length) skip.add(i);
      }
    }
    if (process.env.HUBGEN_DEBUG) for (const i of skip) console.log('hubgen: left out: ' + exprs[i]);
    const v = new Array(exprs.length);
    for (const k of fs.readFileSync(s, 'latin1').matchAll(/^_v(\d+):\s*\.dword\s+\$([0-9a-fA-F]+)/gm)) v[+k[1]] = parseInt(k[2], 16) | 0;
    return v;
  } finally { fs.rmSync(dir, { recursive: true, force: true }); }
}

// ---- actions.h, from the catalogue: VB_NONE, its [actions] in order (each its comment, the catalogue's (* *)), NACTIONS
function actionsHeader() {
  const out = [], cat = fs.readFileSync(path.join(DQ0, 'commands.actions'), 'latin1').split(/\r?\n/);
  let on = false;
  for (const l of cat) {
    if (l.startsWith('[')) { on = l.trim() === '[actions]'; continue; }
    const m = on && l.match(/^(VB_\w+)(.*)$/);
    if (!m) continue;
    const note = (m[2].match(/\(\*\s*(.*?)\s*\*\)/) || [])[1];
    out.push('    ' + m[1] + ',' + (note ? ' '.repeat(Math.max(1, 44 - m[1].length - 5)) + '/* (' + note.replace(/\*\//g, '* /') + ') */' : ''));
  }
  return ['/* actions.h - Dingus Quest Zero\'s actions (made by the build, games/dq0/hubgen.js, from games/dq0/commands.actions:',
    '** that\'s the one list of them; a new action is a line there).  What a command does (hub.h\'s struct cmd), whoever\'s',
    '** it is (a line the player typed, the parser\'s; a denizen\'s mind\'s), each one meaning in the game, in the',
    '** catalogue\'s order: VB_NONE, then each of its [actions], then NACTIONS */', '',
    '#ifndef _ACTIONS_H', '#define _ACTIONS_H', '', 'enum {', '    VB_NONE,', ...out, '    NACTIONS', '};', '', '#endif', ''].join('\r\n');
}

// ---- The build's step: OPT { cc65: { bin, flags: [cc65's -t, --cpu, its include folders] }, say }.  OUT: what it
// made (load ()'s)
function generate(opt) {
  fs.mkdirSync(GEN, { recursive: true });
  const act = actionsHeader(), actFile = path.join(GEN, 'actions.h');
  if (!fs.existsSync(actFile) || fs.readFileSync(actFile, 'latin1') !== act) fs.writeFileSync(actFile, act, 'latin1');
  const raw = HEADERS.map(h => fs.readFileSync(path.join(DQ0, h), 'latin1'));
  const hash = crypto.createHash('sha1');
  for (const t of [...raw, act, fs.readFileSync(__filename, 'latin1')]) hash.update(t);
  const key = hash.digest('hex'), incs = Object.keys(ASM).map(k => path.join(GEN, 'dq0hub_' + k + '.inc'));
  try {
    const j = JSON.parse(fs.readFileSync(JSON_OUT, 'utf8'));
    if (j.key === key && incs.every(f => fs.existsSync(f))) return (LOADED = wrap(j));
  } catch (e) { /* (Made again) */ }
  const texts = [...raw.map(clean), clean(act)];
  const { defs, enums } = scan(texts), named = structs(texts);
  const exprs = [], at = new Map(), idx = e => { if (!at.has(e)) { at.set(e, exprs.length); exprs.push(e); } return at.get(e); };
  // (The names: each #define's that isn't code or words, each enum's member)
  const names = [];
  for (const [n, b] of Object.entries(defs)) if (b && !/["{;]/.test(b) && !/^_\w+_H$/.test(n)) names.push(n);
  for (const e of enums) names.push(...e);
  const nameIdx = names.map(n => idx(n)), extraIdx = EXTRA.map(e => idx(e));
  // (The layouts: each named struct's size, each field's offset, size, its dimensions' elements' sizes)
  const lay = {};
  for (const [s, body] of Object.entries(named)) {
    const st = 'struct ' + s, p = '((' + st + '*) 0)->';
    lay[s] = { size: idx('sizeof (' + st + ')'), f: fields(body, defs, named).map(f => {
      const ix = [idx('offsetof (' + st + ', ' + f.c + ')'), idx('sizeof (' + p + f.c + ')')];
      for (let k = 1; k <= f.dims; k++) ix.push(idx('sizeof (' + p + f.c + '[0]'.repeat(k) + ')'));
      return [f.path, ix];
    }) };
  }
  // (The writers' table: its tasks hub.h's T_* (T_UI ... before NTASKS), lower case)
  const tasks = enums.find(e => e.includes('NTASKS')).filter(n => n !== 'NTASKS').map(n => n.slice(2).toLowerCase());
  const own = ownTable(raw[0], tasks);
  ownExprs(own, idx);
  const itemSize = idx('sizeof (struct item)'), taskOff = idx('offsetof (struct hub, task)');
  const v = evaluate(exprs, opt.cc65);
  // The JSON
  const C = {};
  names.forEach((n, i) => { if (v[nameIdx[i]] !== undefined) C[n] = v[nameIdx[i]]; });
  EXTRA.forEach((e, i) => { if (v[extraIdx[i]] === undefined) throw new Error('hubgen: ' + e + ': no value'); C[e] = v[extraIdx[i]]; });
  const S = {};
  for (const [s, l] of Object.entries(lay)) {
    if (v[l.size] === undefined) continue;                     // (A struct the compiler hasn't: an incomplete one)
    const f = {};
    for (const [p, ix] of l.f) if (ix.every(i => v[i] !== undefined)) f[p] = ix.map(i => v[i]);
    S[s] = { size: v[l.size], f };
  }
  for (const n of ['HUB_BANKS', 'NTASKS', 'HUB_MAGIC', 'CHUNK_SIZE', 'NCSLOTS', 'NITEMS', 'SIGHT_BANK', 'UI_BANK', 'WORLD_BANK'])
    if (C[n] === undefined) throw new Error('hubgen: no ' + n + ' in the headers');
  const j = { key, C, enums: enums.filter(e => e.every(n => C[n] !== undefined)), S, tasks,
    own: ownSpans(own, { at: v, itemSize: v[itemSize], taskOff: v[taskOff] }, C) };
  const H = wrap(j);
  // The assembly files
  for (const [k, list] of Object.entries(ASM)) {
    const rows = Object.entries(list).map(([n, spec]) => {
      const x = H.value(spec), hex = '$' + (x >>> 0).toString(16).toUpperCase();
      return n.padEnd(15) + ' = ' + (x > 255 ? hex : String(x)).padEnd(10) + ' ; (' + spec + ')';
    });
    const text = ['; dq0hub_' + k + '.inc - made by the build (games/dq0/hubgen.js) from the C headers, as cc65 lays them out: not to be',
      '; edited (a name here is in hubgen.js\'s ASM, its value the headers\')', ...rows, ''].join('\r\n');
    const f = path.join(GEN, 'dq0hub_' + k + '.inc');
    if (!fs.existsSync(f) || fs.readFileSync(f, 'latin1') !== text) fs.writeFileSync(f, text, 'latin1');
  }
  const t = JSON_OUT + '.' + process.pid;
  fs.writeFileSync(t, JSON.stringify(j));
  fs.renameSync(t, JSON_OUT);
  if (opt.say) opt.say('dq0hub.json: ' + Object.keys(C).length + ' names, ' + Object.keys(S).length + ' structs, ' + own.length + ' writers\' lines');
  return (LOADED = H);
}

// ---- The numbers read: load () (the build's, obj/gen/dq0hub.json).  H.C.NAME a name's value; H.off (STRUCT, PATH)
// a field's offset (PATH's indices numbers or names: "door[2].rel", "mreq[ASK_UI]"; a STRUCT "hub" or "struct hub"),
// H.size (STRUCT) its size, H.size (STRUCT, PATH) a field's (less its indices given: "door" all, "door[0]" one);
// H.dims (STRUCT, PATH) its dimensions' element sizes; H.value (SPEC) ASM's specs'
function wrap(j) {
  const rec = (s, p) => {
    const S = j.S[String(s).replace(/^struct\s+/, '')];
    if (!S) throw new Error('dq0hub: no struct ' + s);
    if (p === undefined) return { S };
    let off = 0, key = '', last = null, n = 0;
    for (const seg of String(p).split('.')) {
      const m = seg.match(/^(\w+)((?:\[[^\]]*\])*)$/);
      if (!m) throw new Error('dq0hub: ' + s + '.' + p + '?');
      key += (key ? '.' : '') + m[1];
      const f = S.f[key];
      if (!f) throw new Error('dq0hub: no field ' + key + ' in struct ' + s);
      off += last ? f[0] - last[0] : f[0];                   // (A field's offset from its parent's first element's)
      const ix = [...m[2].matchAll(/\[([^\]]*)\]/g)].map(k => k[1].trim());
      ix.forEach((x, d) => {
        const i = /^\d+$/.test(x) ? +x : j.C[x];
        if (i === undefined) throw new Error('dq0hub: ' + x + '?');
        off += i * f[2 + d];
      });
      last = f; n = ix.length;
    }
    return { S, f: last, off, n };
  };
  const H = {
    C: j.C, enums: j.enums, own: j.own, tasks: j.tasks, S: j.S,
    off: (s, p) => rec(s, p).off,
    size: (s, p) => { const r = rec(s, p); return p === undefined ? r.S.size : r.n ? r.f[1 + r.n] : r.f[1]; },
    dims: (s, p) => rec(s, p).f.slice(2),
    words: first => {                                          // (An enum's members as words, in order: words ('G_NONE')
      const e = j.enums.find(x => x[0] === first), pre = first.slice(0, first.indexOf('_') + 1);   //   -> none, artifact
      if (!e) throw new Error('dq0hub: no enum begun with ' + first);                              //   ...; its count, a
      const w = e.filter(n => n.startsWith(pre));                                                   //   name past them, not)
      w.forEach((n, i) => { if (j.C[n] !== i) throw new Error('dq0hub: ' + n + ' is ' + j.C[n] + ', not ' + i + ' (words (' + first + '))'); });
      return w.map(n => n.slice(pre.length).toLowerCase());
    },
    value: spec => {
      let m;
      if ((m = spec.match(/^sizeof\s+(\w+)(?:\.(.+))?$/))) return H.size(m[1], m[2]);
      if ((m = spec.match(/^(\w+)\.(.+)$/))) return H.off(m[1], m[2]);
      if (j.C[spec] === undefined) throw new Error('dq0hub: no ' + spec + ' in the headers');
      return j.C[spec];
    },
  };
  return H;
}

let LOADED = null;
function load() {
  if (LOADED) return LOADED;
  let j;
  try { j = JSON.parse(fs.readFileSync(JSON_OUT, 'utf8')); } catch (e) {
    throw new Error('obj/gen/dq0hub.json: not there (made by the build: node build.js)');
  }
  return (LOADED = wrap(j));
}

module.exports = { generate, load, ASM, HEADERS };
