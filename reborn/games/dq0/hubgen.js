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
//   - obj/gen/dq0rec.h: the save folder's records the programs between dungeons pass to each other (records.def: each
//     one's struct, its file, its magic and version: between.dat, life.dat, clues.dat; the structs they're made of),
//     their layouts in dq0hub.json too (load ().record: the tests' decoder);
//   - obj/gen/dq0hub_NAME.inc: an assembly file's names (ASM below: each consumer the names it uses, each a number
//     of the headers', a field's offset or a size), .included where its hand-typed copies were.
// The values come from cc65 itself: a C file of `const long vN = (long) (EXPR);`, one for each name, field and size,
// compiled once and read back from its assembly (an expression cc65 can't make a constant of, a #define of code, is
// left out: its line dropped and the file compiled again).  Made again only when an input changes (its key: the
// headers', the catalogue's, SAVED's structs' and this file's hash).
//   - obj/gen/dq0ver.h: HUB_VERSION and SAVE_VER, hashes of the layouts (versions, below), not numbers bumped by hand.
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { execFileSync } = require('child_process');

const ROOT = path.join(__dirname, '..', '..');                  // (reborn/)
const DQ0 = __dirname;
const GEN = path.join(ROOT, 'obj', 'gen');
const JSON_OUT = path.join(GEN, 'dq0hub.json');
const MSG_H = path.join(GEN, 'dq0msg.h'), MSGS = path.join(DQ0, 'msgs.def');
const REC_H = path.join(GEN, 'dq0rec.h'), RECORDS = path.join(DQ0, 'records.def');
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
    CM_OBJ: 'cmd.obj', CM_OTHER: 'cmd.other', CM_HAND: 'cmd.hand', CM_QUOTE: 'cmd.quote', CM_SIZE: 'sizeof cmd',
    T_KIND: 'cthing.kind', T_SEL: 'cthing.sel', T_N: 'cthing.n', T_LIES: 'cthing.lies', T_WORD: 'cthing.word', T_WORDSIZE: 'sizeof cthing.word',
    CF_NOVERB: 'CF_NOVERB', CF_HALF: 'CF_HALF', CF_UNSAID: 'CF_UNSAID', CF_STRANGE: 'CF_STRANGE', CF_WORN: 'CF_WORN', CF_LIST: 'CF_LIST',
    CQ_REL: 'CQ_REL', SL_OTHER: 'SL_OTHER', HN_LEFT: 'HN_LEFT', HN_RIGHT: 'HN_RIGHT', HN_BOTH: 'HN_BOTH', HN_OTHER: 'HN_OTHER',
    HN_OFF: 'HN_OFF', HN_MAIN: 'HN_MAIN',
  },
  line: {                                                       // modules/dq0lib/line.s: the squares, a portal
    T_WALL: 'T_WALL', T_DOOR: 'T_DOOR', T_WINDOW: 'T_WINDOW', P_KIND: 'portal.kind', P_STATE: 'portal.state', P_PARTS: 'portal.parts',
    PK_WINDOW: 'PK_WINDOW', PK_PORTCULLIS: 'PK_PORTCULLIS', PK_GRATE: 'PK_GRATE', PK_SLIT: 'PK_SLIT',
    PP_GLASS: 'PP_GLASS', PP_SHUTTERS: 'PP_SHUTTERS', PS_OPEN: 'PS_OPEN', PS_WRECKED: 'PS_WRECKED',
  },
  items: {                                                      // games/dq0w/items.s: the item store
    REC: 'sizeof item', NITEMS: 'NITEMS', BANK0: 'ITEM_BANK (0)', ITEM_BANKS: 'ITEM_BANKS',
    I_KIND: 'item.kind', I_FLAGS: 'item.flags', I_WHERE: 'item.where', I_X: 'item.x',
    I_OWNER: 'item.owner', I_HIDE: 'item.hide', IW_DEN: 'IW_DEN', A_DID: 'actor.did',     // (den_draw's: the world's)
  },
  letter: { NLET: 'NLET', LT_REC: 'LT_REC', CIX: 'sizeof cix', CIX_LET: 'cix.let' },   // games/dq0w/letter.s
  pulse: { SND_GRID: 'SND_GRID', SND_W: 'SND_W' },              // games/dq0p/pulse.s: the sound's grid
  at: { CW: 'CW', M_CHUNK: 'M_CHUNK' },                         // games/dq0w/at.s: a chunk's width, the maker's chunk
  hz: {                                                         // games/dq0w/hz.s: body bd's step, its hazards (den_hz)
    CH_NTRAPS: 'chunk.ntraps', CH_TRAP: 'chunk.trap', TR_SIZE: 'sizeof trap', TR_X: 'trap.x', TR_Y: 'trap.y', TR_STATE: 'trap.state',
    TR_TRIGGER: 'trap.trigger', TT_DOOR: 'TT_DOOR', TS_SPRUNG: 'TS_SPRUNG', TS_DISARMED: 'TS_DISARMED', TS_BROKEN: 'TS_BROKEN',
    T_PIT: 'T_PIT', A_STATE: 'actor.state', A_SN: 'actor.sn', A_X: 'actor.x', A_Y: 'actor.y',
    A_BODY: 'actor.body', BD_WINGS: 'BD_WINGS', TR_HIDE: 'trap.hide', TR_KNOW: 'trap.know',   // (dq0d/ktrap.s's too)
  },
  sq: {                                                         // modules/dq0lib/sq.s: the chunks' cache, a stratum, a chunk
    SS: 'sizeof stratum', SS_X0: 'stratum.x0', SS_Y0: 'stratum.y0', SS_W: 'stratum.w', SS_H: 'stratum.h', WORLD_BANK: 'WORLD_BANK',
    PLAN_BANKS: 'PLAN_BANKS', NCSLOTS: 'NCSLOTS', CS_SIZE: 'sizeof cslot', CS_STAMP: 'cslot.stamp', W_CSLOT: 'wrec.cslot',
    W_STAMP: 'wrec.stamp', W_INUSE: 'wrec.inuse', HUB_WINDOW: 'HUB_WINDOW', CHUNK_SIZE: 'CHUNK_SIZE', CH_TILE: 'chunk.tile',
    CH_NPORTALS: 'chunk.nportals', CH_PORTAL: 'chunk.portal', PO_SIZE: 'sizeof portal', PO_X: 'portal.x', PO_Y: 'portal.y', CW: 'CW',
  },
  walk: { CW: 'CW', T_DOOR: 'T_DOOR', T_STEPS: 'T_STEPS', STEP_MAX: 'STEP_MAX', STAIR_MAX: 'STAIR_MAX', PS_HIDDEN: 'PS_HIDDEN', PS_OPEN: 'PS_OPEN' },   // modules/dq0lib/walk.s
  lbsaid: { TOLD: 'TOLD' },
  gw: {                                                         // modules/dq0lib/gw.s: the one give-way
    HUB_WINDOW: 'HUB_WINDOW', HUBO_PACE: 'hub.pace', HUBO_TPR: 'hub.tpr', HUBO_CMD: 'hub.ui_cmd', HUBO_KEYS: 'hub.ui_keys',
    HUBO_CQ: 'hub.cq', RQ_SEQ: 'rqh.seq', GW_CMD: 'GW_CMD', GW_KEYS: 'GW_KEYS', GW_PACE: 'GW_PACE', GW_NOKEYS: 'GW_NOKEYS',
    GW_LOOK: 'GW_LOOK', GW_ALL: 'GW_ALL', GW_REST: 'GW_REST', HUBO_WWST: 'hub.way_want.st', HUBO_ENTER: 'hub.ui_enter',
  },                                     // games/dq0/lbsaid.s: a look's sentences kept (dq0.h's)
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
// null }]; a member that's a macro alone (RQH) its #define's text; an anonymous struct's or union's members its
// parent's, as C has them (the hub's bodies, actor 0's old names: hub.h's union, offsetof's place of each by cc65)
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
      if (!list.length) { out.push(...members(sub, defs, named)); continue; }   // (Anonymous: its members the parent's)
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
  const head = '#include <stddef.h>\n#include "dq0.h"\n#include "hub.h"\n#include "dq0rec.h"\n', skip = new Set();   // (main.c's order;
                                                                                                    //   the records')
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
        for (const m of text.matchAll(/hub\.c[:(](\d+)\)?:\s*Error/g)) bad.push(+m[1] - 5);   // (The file's line: its
        if (!bad.length || pass > 100) {                                                       //   expression's, after four)
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

// ---- The messages' payloads (ENGINE-CRITIQUE.md's §2, step 3's second stage): msgs.def, a kind's fields where they
// sit in its struct (struct msg's slots a, b, w1 ...; x[2] an array's element), packed bits as bit ranges; from it
// obj/gen/dq0msg.h (an accessor a field, a packer a packed slot, a builder a kind: plain macros over the struct, hub.h
// includes it) and the JSON's msgs (each kind's fields, by offset, for the tests' decoders: load ()'s decode).  Its
// lines (msgs.def's head says it at length):
//   struct NAME                                    the struct the kinds after it travel in
//   KIND[:FORM]  [< KIND[:FORM];] SLOT FIELD ...; SLOT FIELD ...      a kind's payload (a line indented goes on; an
//                                                  included field its own fields cover, within its bits, theirs: the
//                                                  outcome events' E_OUTCOME, their kinds' names for it; the JSON's
//                                                  outcome, the kinds that include it, saidgen.js's)
// a FIELD name (the slot whole), name:N (bit N) or name:N-M (bits N to M), and @FIRST, an enum's first member, for
// its value's name in the tests (a name of one bit, a boolean there); then (the events') ! a signed byte, +K stored
// plus K, ==CONST[,CONST] the form's selector (the form a message is read by, its field's value one of them).  The
// referee's requests' (struct req) besides: a SLOT x[4-7], an array's elements spanned (two bytes a word, four a long:
// the accessor a cast of its first byte's address; else, or a FIELD name[], its bytes, the address), x the array
// whole; a SLOT alone, its field its own name (steps, grade); a request's reply after its ->, a form of its own
// (KIND:reply, KIND:FORM_reply); and no builder for a request (filled slot by slot, as the code computes it)
const TYPE_BITS = { 'unsigned char': 8, 'signed char': 8, 'char': 8, 'unsigned': 16, 'unsigned int': 16, 'int': 16,
  'signed': 16, 'unsigned short': 16, 'short': 16, 'unsigned long': 32, 'long': 32, 'signed long': 32 };

// A struct's slots' widths, by its declarations' types: { slot: { bits, path (the C's), n (an array's length), sg
// (signed) } }
function slotWidths(body, defs) {
  const out = {};
  for (const decl of split(body.replace(/^[ \t]*#.*$/gm, ''), ';')) {
    const parts = split(decl, ','), first = parts[0].match(/^((?:\w+\s+)*?\w+)\s+(\w+\s*(?:\[[^\]]*\])?)$/);
    if (!first) continue;
    const type = first[1].replace(/^(?:volatile|const)\s+/, '').replace(/\s+/g, ' '), bits = TYPE_BITS[type];
    if (!bits) continue;
    for (const x of [first[2], ...parts.slice(1)]) {
      const m = x.trim().match(/^(\w+)\s*(?:\[([^\]]*)\])?$/);
      if (!m) continue;
      out[m[1]] = { bits, n: m[2] === undefined ? 0 : +(defs[m[2]] !== undefined ? defs[m[2]] : m[2]), sg: /^(signed|int|short|long)/.test(type) };
    }
  }
  return out;
}

// msgs.def read: { forms: [{ kind, form, struct, inc, fields: [{ name, slot, lo, hi, en }], line }] }
function msgsRead(text) {
  const forms = [];
  let struct = null, cur = null;
  const lines = text.split(/\r?\n/);
  const entry = (s, line) => {
    const r = s.indexOf('->');                                 // (A request's reply after its ->: a form of its own,
    if (r >= 0) {                                              //   FORM_reply, or reply; none asked, none before it)
      const head = s.match(/^(\w+)(?::(\w+))?/);
      if (!head) throw new Error('msgs.def:' + line + ': ' + s);
      if (s.slice(head[0].length, r).trim()) entry(s.slice(0, r), line);
      entry(head[1] + ':' + (head[2] ? head[2] + '_' : '') + 'reply ' + s.slice(r + 2), line);
      return;
    }
    const parts = s.split(';').map(p => p.trim()), head = parts[0].match(/^(\w+)(?::(\w+))?\s*(.*)$/);
    if (!head) throw new Error('msgs.def:' + line + ': ' + s);
    if (!struct) throw new Error('msgs.def:' + line + ': no struct line before it');
    const f = { kind: head[1], form: head[2] || '', struct, inc: [], fields: [], aliases: [], line };
    parts[0] = head[3].trim();
    for (const p of parts) {
      if (!p || p === '-') continue;
      if (p[0] === '<') { f.inc.push(p.slice(1).trim()); continue; }
      const w = p.split(/\s+/), sl = w.shift().match(/^(\w+(?:\[\d+(?:-\d+)?\])?)(?:\(([a-z]\w*)\))?$/);
      if (!sl) throw new Error('msgs.def:' + line + ': "' + p + '", a slot?');
      const slot = sl[1];
      if (sl[2]) f.aliases.push({ name: sl[2], slot });         // (A packed slot's whole, by a name: SLOT(name))
      if (!sl[2] && !w.length && /^[a-z]\w*$/.test(slot)) w.push(slot);   // (A slot alone, its field its name: steps)
      for (const t of w) {
        const m = t.match(/^([a-z]\w*)(\[\])?(?::(\d+)(?:-(\d+))?)?(!)?(?:\+(\d+))?(?:==(\w+(?:,\w+)*)|=(\w+))?(?:@(\w+(?:,\w+)*|\w+(?:\/\w+)+))?$/);
        if (!m) throw new Error('msgs.def:' + line + ': "' + t + '", a field?');
        f.fields.push({ name: m[1], slot, lo: m[3] === undefined ? -1 : +m[3], hi: m[3] === undefined ? -1 : +(m[4] === undefined ? m[3] : m[4]), bit: m[8] || '', en: m[9] || '',
          signed: !!m[5], plus: m[6] === undefined ? 0 : +m[6], sel: m[7] ? m[7].split(',') : null, bytes: !!m[2] });   // (The events': a signed byte,
                                                                                                         //   a value stored plus K, a form's selector; a
                                                                                                         //   request's span's bytes)
      }
    }
    forms.push(f);
  };
  lines.forEach((raw, i) => {
    const l = raw.replace(/#.*$/, '');
    if (!l.trim()) return;
    if (/^\s/.test(l)) { if (!cur) throw new Error('msgs.def:' + (i + 1) + ': goes on from nothing'); cur.s += ' ' + l.trim(); return; }
    if (cur) entry(cur.s, cur.line);
    cur = null;
    const st = l.match(/^struct\s+(\w+)\s*$/);
    if (st) { struct = st[1]; return; }
    cur = { s: l.trim(), line: i + 1 };
  });
  if (cur) entry(cur.s, cur.line);
  return forms;
}

// The forms resolved (their < included), checked against their structs (each slot one of its, each field in its
// slot's bits, none overlapping another of its form), and dq0msg.h's text
function msgsMake(forms, named, defs, names) {
  const byKey = new Map(forms.map(f => [f.kind + (f.form ? ':' + f.form : ''), f]));
  const widths = {}, slotOf = (f, slot) => {
    if (!widths[f.struct]) {
      if (named[f.struct] === undefined) throw new Error('msgs.def:' + f.line + ': no struct ' + f.struct);
      widths[f.struct] = slotWidths(named[f.struct], defs);
    }
    const m = slot.match(/^(\w+)(?:\[(\d+)(?:-(\d+))?\])?$/), w = widths[f.struct][m[1]];
    if (!w || (m[2] !== undefined && !w.n) || (w.n && +(m[3] || m[2] || 0) >= w.n) || +m[3] <= +m[2]) throw new Error('msgs.def:' + f.line + ': no slot ' + slot + ' in struct ' + f.struct);
    const ord = Object.keys(widths[f.struct]).indexOf(m[1]) * 64 + (+m[2] || 0);
    if (!w.n || (m[2] !== undefined && m[3] === undefined))     // (A slot, or an array's element)
      return { ord, bits: w.bits, sg: w.sg, c: m[2] === undefined ? m[1] : m[1] + '[' + m[2] + ']', up: (m[1] + (m[2] || '')).toUpperCase(),
        base: m[1], first: +m[2] || 0, last: +m[2] || 0, at: m[2] !== undefined };
    // (An array's elements spanned, x[4-7], or the array whole, x: two bytes a word, four a long, by its first byte's
    // address, an lvalue; else its first byte's address, a pointer)
    const first = m[2] === undefined ? 0 : +m[2], last = m[2] === undefined ? w.n - 1 : +m[3], n = (last - first + 1) * w.bits / 8;
    const p = first ? '((m).' + m[1] + ' + ' + first + ')' : '(m).' + m[1];
    return { ord, bits: (last - first + 1) * w.bits, c: m[1] + '[' + first + ']', up: (m[1] + (m[2] === undefined ? '' : first + '_' + last)).toUpperCase(),
      base: m[1], first, last, at: m[2] !== undefined, span: true, ptr: p,
      acc: n === 2 ? '(*(unsigned*) ' + p + ')' : n === 4 ? '(*(unsigned long*) ' + p + ')' : p };
  };
  const all = f => {                                           // (A form's fields, its included forms' first; one of
    const out = [];                                            //   theirs its own fields take the place of (the outcome
    for (const k of f.inc) {                                   //   events' head's: E_DIG's how its outcome), left out,
      const g = byKey.get(k);                                  //   each of its own within that one's bits)
      if (!g) throw new Error('msgs.def:' + f.line + ': no ' + k + ' to include');
      out.push(...all(g));
    }
    const bits = x => { const w = slotOf(f, x.slot).bits; return [x.lo < 0 ? 0 : x.lo, x.hi < 0 ? w - 1 : x.hi]; };
    const kept = out.filter(y => {
      const [ylo, yhi] = bits(y), mine = f.fields.filter(x => x.slot === y.slot && bits(x)[0] <= yhi && ylo <= bits(x)[1]);
      for (const x of mine) if (bits(x)[0] < ylo || bits(x)[1] > yhi) throw new Error('msgs.def:' + f.line + ': ' + x.name + ' over ' + y.name + ', not within it');
      return !mine.length;
    });
    return [...kept, ...f.fields];
  };
  const macros = new Map(), out = [];
  const def = (name, text, line) => {
    if (names.has(name)) throw new Error('msgs.def:' + line + ': ' + name + ' is a name of the headers\' already');
    if (macros.has(name) && macros.get(name) !== text) throw new Error('msgs.def:' + line + ': ' + name + ' made twice, unlike');
    if (!macros.has(name)) { macros.set(name, text); out.push(text); }
  };
  const pad = (s, n) => s + ' '.repeat(Math.max(1, n - s.length));
  const hex = v => '0x' + v.toString(16).toUpperCase();
  const res = [];
  for (const f of forms) {
    const fs_ = all(f), P = f.kind + (f.form ? '_' + f.form.toUpperCase() : ''), used = {};
    const fields = fs_.map(x => {
      const s = slotOf(f, x.slot), lo = x.lo < 0 ? 0 : x.lo, hi = x.hi < 0 ? s.bits - 1 : x.hi;
      if (hi >= s.bits || lo > hi || (s.span && x.lo >= 0 && (s.bits > 32 || x.bytes)) || (x.bytes && !s.span)) throw new Error('msgs.def:' + f.line + ': ' + x.name + ' past ' + x.slot + '\'s ' + s.bits + ' bits');
      const mask = (2 ** (hi - lo + 1) - 1) * 2 ** lo;
      for (const [n, u] of Object.entries(used)) if (u.slot === x.slot ? u.mask & mask : u.base === s.base && u.first <= s.last && s.first <= u.last)
        throw new Error('msgs.def:' + f.line + ': ' + x.name + ' over ' + n);
      if (used[x.name]) throw new Error('msgs.def:' + f.line + ': ' + x.name + ' twice');
      used[x.name] = { slot: x.slot, mask, base: s.base, first: s.first, last: s.last };
      if (x.signed && (lo || hi !== 7)) throw new Error('msgs.def:' + f.line + ': ' + x.name + ' signed, not bits 0-7');
      if (x.sel && f.fields.indexOf(x) < 0) throw new Error('msgs.def:' + f.line + ': ' + x.name + '\'s selector an included form\'s');
      for (const c of x.sel || []) if (!names.has(c)) throw new Error('msgs.def:' + f.line + ': ' + c + ', not a name of the headers\'');
      return { ...x, lo, hi, s, whole: lo === 0 && hi === s.bits - 1, top: hi === s.bits - 1, fixed: x.sel && x.sel.length === 1 ? x.sel[0] : '' };
    });
    const sels = fields.filter(x => x.sel);                    // (A form chosen by a field's value: one field, and the
    if (sels.length > 1 || (sels.length && !f.form)) throw new Error('msgs.def:' + f.line + ': a selector ' + (f.form ? 'twice' : 'of a kind\'s own form'));   //   kind's own form, none)
    out.push('', '/* ' + f.kind + (f.form ? ':' + f.form : '') + ' (msgs.def:' + f.line + ') */');
    const bit = x => x.bit || hex(2 ** x.lo);                  // (A one-bit field's bit: its constant's name, or its value)
    const kinds = x => byKey.get(f.kind) !== f && all(byKey.get(f.kind) || { inc: [], fields: [] }).some(y => y.name === x.name &&
      y.slot === x.slot && (y.lo < 0 ? 0 : y.lo) === x.lo && (y.hi < 0 ? x.s.bits - 1 : y.hi) === x.hi && y.signed === x.signed && y.plus === x.plus);
    for (const x of fields) {                                 // (Its accessors: the slot whole an lvalue; one bit, the
      const v = x.bytes ? x.s.ptr : x.s.acc || '(m).' + x.s.c, body = x.signed ? '(signed char) ' + v : x.whole ? v : x.lo === x.hi ? v + ' & ' + bit(x) :    //   bit in its place;
        !x.lo && x.hi === 7 ? '(unsigned char) ' + v :       //   a word's low byte, the byte (cc65's shorter code);
        (x.lo ? v + ' >> ' + x.lo : v) + (x.top ? '' : ' & ' + hex(2 ** (x.hi - x.lo + 1) - 1));   //   else its bits)
      const A = (sels.length && !x.sel && !kinds(x) ? P : f.kind) + '_' + x.name.toUpperCase();   // (A chosen form's own:
                                                              //   KIND_FORM_FIELD, unless the kind's own form has it the same)
      def(A, pad('#define ' + A + '(m)', 40) + '(' + body + ')', f.line);
    }
    for (const x of f.aliases) {                               // (A packed slot's whole: an lvalue, its packer's to set)
      if (used[x.name]) throw new Error('msgs.def:' + f.line + ': ' + x.name + ' a field\'s name and a slot\'s');
      const A = (sels.length ? P : f.kind) + '_' + x.name.toUpperCase(), s = slotOf(f, x.slot);
      def(A, pad('#define ' + A + '(m)', 40) + '(' + (s.acc || '(m).' + s.c) + ')', f.line);
    }
    const slots = [...new Set(fields.map(x => x.slot))].sort((p, q) => fields.find(x => x.slot === p).s.ord - fields.find(x => x.slot === q).s.ord);
    const arg = x => x.fixed ? '' : x.name + '_';             // (A form's selector of one value: not an argument, its
    const val = x => {                                         //   constant in its place.  A field's part of its slot's value: one bit, its bit
      const a = '(' + (x.fixed || x.name + '_') + ')';         //   or none; the bottom one as given, the top one cut by
      if (x.lo === x.hi && !x.whole) return '(' + a + ' ? ' + bit(x) + ' : 0)';   //   the slot, one between masked)
      if (!x.lo) return a;
      return (x.top ? (x.s.bits > 8 ? '(unsigned' + (x.s.bits > 16 ? ' long' : '') + ') ' : '') + a : '(' + a + ' & ' + hex(2 ** (x.hi - x.lo + 1) - 1) + ')') + ' << ' + x.lo;
    };
    for (const sl of slots) {                                  // (A packed slot's packer: its fields, in their order)
      const xs = fields.filter(x => x.slot === sl);
      if (xs.length === 1 && xs[0].whole) continue;
      def(P + '_' + xs[0].s.up, pad('#define ' + P + '_' + xs[0].s.up + '(' + xs.map(arg).filter(Boolean).join(', ') + ')', 40) + '(' + xs.map(val).join(' | ') + ')', f.line);
    }
    const sorted = slots.map(sl => fields.filter(x => x.slot === sl));   // (Its builder: kind, who, each slot in turn;
                                                                         //   a request none, a struct with no who)
    const args = sorted.flat().map(arg).filter(Boolean), sets = ['(m).kind = ' + f.kind, '(m).who = (who_)'];
    for (const xs of sorted) sets.push((xs[0].s.span ? xs[0].s.acc : '(m).' + xs[0].s.c) + ' = ' + (xs.length === 1 && xs[0].whole ? '(' + (xs[0].fixed || xs[0].name + '_') + ')' : P + '_' + xs[0].s.up + ' (' + xs.map(arg).filter(Boolean).join(', ') + ')'));
    if (names.has(f.kind) && slotWidths(named[f.struct] || '', defs).who) def(P + '_SET', pad('#define ' + P + '_SET(' + ['m', 'who_', ...args].join(', ') + ')', 40) + '(' + sets.join(', ') + ')', f.line);
    res.push({ kind: f.kind, form: f.form, struct: f.struct, sel: sels.length ? { name: sels[0].name, values: sels[0].sel } : null,
      fields: fields.map(x => ({ name: x.name, slot: x.slot, lo: x.lo, hi: x.hi, bits: x.s.bits, en: x.en, bit: x.bit, signed: x.signed, plus: x.plus,
      at: x.s.base + (x.s.at || x.s.span ? '[' + x.s.first + ']' : ''), n: x.s.last - x.s.first + 1, sg: x.bytes ? 2 : x.signed || (x.whole && x.s.sg) ? 1 : 0 })) });
  }
  const text = ['/* dq0msg.h - made by the build (games/dq0/hubgen.js) from games/dq0/msgs.def: not to be edited.  Each message',
    '** kind\'s fields where they sit in its struct (hub.h\'s struct msg, struct req): KIND_FIELD (m) a field of message m (a struct, not',
    '** a pointer: *p for one), an lvalue if it\'s its slot whole (a packed slot\'s name too, msgs.def\'s SLOT(name));',
    '** KIND[_FORM]_SLOT (fields ...) a packed slot\'s value; and',
    '** KIND[_FORM]_SET (m, who, fields ...) a message made, every slot its payload names set, in order */', '',
    '#ifndef _DQ0MSG_H', '#define _DQ0MSG_H', ...out, '', '#endif', ''].join('\r\n');
  // (The outcome events: the kinds that include the head, E_OUTCOME: said.txt's, the UI's phrase table's)
  const outcome = [...new Set(forms.filter(f => f.inc.includes('E_OUTCOME')).map(f => f.kind))];
  return { text, forms: res, outcome };
}

// ---- The versions (obj/gen/dq0ver.h, the build's: names1, 2026-11-24; both were bumped by hand, and two branches bumped
// them alike).  HUB_VERSION (hub.h's: the tasks attach to a hub of their own build's) 1 to 255, a hash of everything
// dq0hub.json says, the versions apart; SAVE_VER (life.h's: a save of another version isn't carried on) 1 to 65,535, a
// hash of SAVE_FORMAT (life.h's, the hand part: a change the layouts don't show) and SAVED's layouts.
// SAVED: what reaches the save's files (game.dat, world.dat, items.dat, the region files, dens.dat, seen.dat,
// names.dat, the clues', the traces', kit.dat): layout, the structs these headers lay out (dq0hub.json's S: each
// field's offset and size; "NAME:first-last" those fields alone, offsets from the first's); counts, the numbers that
// size a saved array; source, a task's own structs, each by its declaration's text (its file's, the comments and the
// spacing apart: what's in it, not what its dimensions' names come to).  A struct written to a save that isn't here
// fails the build (savedChecked)
const SAVED = {
  layout: ['save', 'quest', 'wsave', 'stratum', 'item', 'change', 'vfh', 'kept', 'tname', 'talias', 'clue', 'uirec:low-cap'],
  counts: ['TNAMES', 'NITEMS', 'ITEM_BANKS', 'MAXSTRATA', 'NLET'],
  source: [['dq0d/dq0d.c', 'den'], ['dq0d/dq0d.c', 'lair'], ['dq0m/changes.c', 'seen'], ['dq0w/outfit.h', 'kitrec'],
    ['dq0w/things.h', 'igroup'], ['dq0w/things.h', 'ires'], ['dq0w/tstore.h', 'trace'], ['dq0w/tstore.h', 'tgroup']],
};
const VER_H = path.join(GEN, 'dq0ver.h'), VERSIONS = ['HUB_VERSION', 'SAVE_VER', 'SAVE_FORMAT'];
const sha = x => parseInt(crypto.createHash('sha1').update(typeof x === 'string' ? x : JSON.stringify(x)).digest('hex').slice(0, 8), 16);

// SAVED's source structs' texts, each its declaration's, its comments and spacing taken out
function savedSources() {
  return SAVED.source.map(([f, s]) => {
    const t = clean(fs.readFileSync(path.join(DQ0, '..', f), 'latin1')), m = new RegExp('\\bstruct\\s+' + s + '\\s*\\{').exec(t);
    if (!m) throw new Error('hubgen: SAVED\'s struct ' + s + ' not in games/' + f + ' (moved? hubgen.js\'s SAVED)');
    let d = 1, i = m.index + m[0].length;
    for (; i < t.length && d; i++) if (t[i] === '{') d++; else if (t[i] === '}') d--;
    return s + ' ' + t.slice(m.index, i).replace(/\s+/g, ' ');
  });
}

// The versions, from a dq0hub.json's C, enums, S and records (theirs less the versions) and SAVED's sources
function versions(j, sources) {
  const C = Object.fromEntries(Object.entries(j.C).filter(([n]) => !VERSIONS.includes(n)));
  const hub = 1 + sha({ C, enums: j.enums, S: j.S, records: j.records }) % 255;
  const lay = SAVED.layout.map(spec => {
    const [s, span] = spec.split(':'), S = j.S[s];
    if (!S) throw new Error('hubgen: SAVED\'s struct ' + s + ' not laid out (hubgen.js\'s SAVED)');
    if (!span) return [s, S];
    const [a, b] = span.split('-'), fa = S.f[a], fb = S.f[b];
    if (!fa || !fb) throw new Error('hubgen: SAVED\'s ' + spec + ': no field ' + (fa ? b : a));
    return [spec, Object.entries(S.f).filter(([, f]) => f[0] >= fa[0] && f[0] < fb[0] + fb[1]).map(([p, f]) => [p, f[0] - fa[0], ...f.slice(1)])];
  });
  const counts = SAVED.counts.map(n => { if (j.C[n] === undefined) throw new Error('hubgen: SAVED\'s ' + n + ': no such number'); return [n, j.C[n]]; });
  if (j.C.SAVE_FORMAT === undefined) throw new Error('hubgen: no SAVE_FORMAT (life.h)');
  const save = 1 + sha({ format: j.C.SAVE_FORMAT, lay, counts, sources }) % 65535;
  return { HUB_VERSION: hub, SAVE_VER: save };
}

// dq0ver.h's text
function verHeader(v) {
  return ['/* dq0ver.h - made by the build (games/dq0/hubgen.js) from the headers\' layouts: not to be edited.  HUB_VERSION (hub.h\'s)',
    '** a hash of every number and layout of the hub\'s headers; SAVE_VER (life.h\'s) one of SAVE_FORMAT and the saved structs\'',
    '** layouts (hubgen.js\'s SAVED) */', '#ifndef _DQ0VER_H', '#define _DQ0VER_H',
    '#define HUB_VERSION     ' + v.HUB_VERSION, '#define SAVE_VER        ' + v.SAVE_VER + 'u', '#endif', ''].join('\r\n');
}

// Every struct a dq0 task writes whole to a file (write (fd, &x, sizeof x), write (fd, b, n * sizeof (struct x)): x a
// struct's, declared in its file) in SAVED (its layout's, or its source's), so a change in one's layout is a new
// SAVE_VER; or in NOT_SAVED, written but not to the save.  OUT: the structs found
const NOT_SAVED = new Set([]);
function savedChecked() {
  const known = new Set([...SAVED.layout.map(s => s.split(':')[0]), ...SAVED.source.map(([, s]) => s), ...NOT_SAVED]), faults = [], found = new Set();
  for (const t of ['dq0', 'dq0w', 'dq0d', 'dq0m', 'dq0p', 'dq0harn']) {
    const dir = path.join(DQ0, '..', t);
    for (const f of fs.readdirSync(dir).filter(f => f.endsWith('.c'))) {
      const text = clean(fs.readFileSync(path.join(dir, f), 'latin1'));
      for (const m of text.matchAll(/\bwrite\s*\(\s*\w+\s*,\s*&?\s*(\w+)\s*,\s*([^;]*?)\)\s*;/g)) {
        let s = (/sizeof\s*\(\s*struct\s+(\w+)\s*\)/.exec(m[2]) || [])[1];
        if (!s && new RegExp('sizeof\\s*\\(?\\s*' + m[1] + '\\b').test(m[2])) {
          const d = new RegExp('\\bstruct\\s+(\\w+)\\s+\\**\\s*' + m[1] + '\\s*[;=\\[]').exec(text);
          s = d && d[1];
        }
        if (!s) continue;
        found.add(s);
        if (!known.has(s)) faults.push(t + '/' + f + ': struct ' + s + ' written (' + m[0].replace(/\s+/g, ' ') + ')');
      }
    }
  }
  if (faults.length) throw new Error('hubgen: a struct written to a file that SAVE_VER doesn\'t follow (hubgen.js\'s SAVED, or NOT_SAVED):\n  ' + faults.join('\n  '));
  return found;
}

// ---- The build's step: OPT { cc65: { bin, flags: [cc65's -t, --cpu, its include folders] }, say }.  OUT: what it
// made (load ()'s)
function generate(opt) {
  fs.mkdirSync(GEN, { recursive: true });
  const act = actionsHeader(), actFile = path.join(GEN, 'actions.h');
  if (!fs.existsSync(actFile) || fs.readFileSync(actFile, 'latin1') !== act) fs.writeFileSync(actFile, act, 'latin1');
  const raw = HEADERS.map(h => fs.readFileSync(path.join(DQ0, h), 'latin1'));
  const hash = crypto.createHash('sha1');
  const msgsText = fs.readFileSync(MSGS, 'latin1');
  const recs = recordsRead(fs.readFileSync(RECORDS, 'latin1')), recText = recordsHeader(recs);   // (The records' header:
  if (!fs.existsSync(REC_H) || fs.readFileSync(REC_H, 'latin1') !== recText) fs.writeFileSync(REC_H, recText, 'latin1');   //   before cc65)
  const sources = savedSources();                       // (SAVED's own structs: SAVE_VER's, in the key)
  savedChecked();
  if (!fs.existsSync(VER_H)) fs.writeFileSync(VER_H, verHeader({ HUB_VERSION: 0, SAVE_VER: 0 }), 'latin1');   // (Till the
  for (const t of [...raw, act, msgsText, recText, ...sources, fs.readFileSync(__filename, 'latin1')]) hash.update(t);   //   layouts are
  const key = hash.digest('hex'), incs = Object.keys(ASM).map(k => path.join(GEN, 'dq0hub_' + k + '.inc'));   //   known: hub.h
  try {                                                                                                      //   includes it)
    const j = JSON.parse(fs.readFileSync(JSON_OUT, 'utf8'));
    if (j.key === key && [...incs, MSG_H, REC_H].every(f => fs.existsSync(f)) && fs.readFileSync(VER_H, 'latin1') === verHeader(j.C))
      return (LOADED = wrap(j));
  } catch (e) { /* (Made again) */ }
  const texts = [...raw.map(clean), clean(act), clean(recText)];
  const { defs, enums } = scan(texts), named = structs(texts);
  const exprs = [], at = new Map(), idx = e => { if (!at.has(e)) { at.set(e, exprs.length); exprs.push(e); } return at.get(e); };
  // (The names: each #define's that isn't code or words, each enum's member)
  const names = [];
  for (const [n, b] of Object.entries(defs)) if (b && !/["{;]/.test(b) && !/^_\w+_H$/.test(n)) names.push(n);
  for (const e of enums) names.push(...e);
  const nameIdx = names.map(n => idx(n)), extraIdx = EXTRA.map(e => idx(e));
  // (The messages' payloads: dq0msg.h written before cc65 reads hub.h, which includes it)
  const msgs = msgsMake(msgsRead(msgsText), named, defs, new Set(names));
  if (!fs.existsSync(MSG_H) || fs.readFileSync(MSG_H, 'latin1') !== msgs.text) fs.writeFileSync(MSG_H, msgs.text, 'latin1');
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
  const j = { key, C, enums: enums.filter(e => e.every(n => C[n] !== undefined)), S, tasks, outcome: msgs.outcome,
    own: ownSpans(own, { at: v, itemSize: v[itemSize], taskOff: v[taskOff] }, C), records: {} };
  for (const r of recs.records) {                              // (The records: each one's file, magic, version and its
    if (!S[r.name]) throw new Error('hubgen: records.def: struct ' + r.name + ' not laid out');   //   fields' types, for
    j.records[r.name] = { file: r.file, magic: r.magic, version: r.version, fields: r.fields.map(f => [f.name, f.type]) };   //   the decoder)
  }
  Object.assign(C, versions(j, sources));               // (The versions, from the layouts: dq0ver.h, hub.h's and
  if (!fs.existsSync(VER_H) || fs.readFileSync(VER_H, 'latin1') !== verHeader(C))   //   life.h's; dq0hub.json's C, the
    fs.writeFileSync(VER_H, verHeader(C), 'latin1');                                 //   assembly's)
  let H = wrap(j);
  // (The payloads by offset, for the tests' decoders: each kind's form, KIND or KIND:FORM, its fields [name, offset,
  // bytes, lo, hi, enum]; each slot's width held to cc65's)
  j.msgs = {};
  for (const f of msgs.forms) j.msgs[f.kind + (f.form ? ':' + f.form : '')] = { struct: f.struct, fields: f.fields.map(x => {
    const n = H.size(f.struct, x.at) * x.n;                   // (A span's bytes its elements')
    if (n * 8 !== x.bits) throw new Error('hubgen: msgs.def: ' + f.struct + '.' + x.slot + ' is ' + n + ' bytes, not ' + x.bits + ' bits');
    if (x.bit && (x.lo !== x.hi || C[x.bit] !== 2 ** x.lo)) throw new Error('hubgen: msgs.def: ' + f.kind + '\'s ' + x.name + ': ' + x.bit + ' is ' + C[x.bit] + ', not bit ' + x.lo);
    return x.sg || x.plus ? [x.name, H.off(f.struct, x.at), n, x.lo, x.hi, x.en, x.sg, x.plus] : [x.name, H.off(f.struct, x.at), n, x.lo, x.hi, x.en];
  }) };
  for (const f of msgs.forms) if (f.sel) {                    // (A form chosen by a field's value: its values, by number)
    const v = f.sel.values.map(c => C[c]), s = f.fields.find(x => x.name === f.sel.name);
    if (v.some(x => x === undefined || x >= 2 ** (s.hi - s.lo + 1))) throw new Error('hubgen: msgs.def: ' + f.kind + ':' + f.form + '\'s ' + f.sel.values + '?');
    j.msgs[f.kind + ':' + f.form].sel = [f.sel.name, v];
  }
  H = wrap(j);
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

// ---- records.def read: { defines: [{ name, value, what }], records: [{ name, file, magic, version, what: [its
// comment's lines], fields: [{ type, name, dims (the C's: "[16]"), what: [lines], before: [a comment's lines] }] }] }:
// a record's and a struct's both, in the file's order (a struct's file null), a field's type a struct's ("struct
// life_head") only one declared before it
function recordsRead(text) {
  const defines = [], records = [], TYPES = ['char', 'signed char', 'unsigned char', 'unsigned', 'int', 'unsigned long', 'long'];
  let rec = null, field = null, note = [], what = [];
  const head = r => {                                          // (A record's head: its first two fields its magic and
    const f = r.fields;                                        //   version, or its first a struct's whose are)
    if (f.length && f[0].type.startsWith('struct ')) return head(records.find(x => 'struct ' + x.name === f[0].type));
    return f.length >= 2 && f[0].name === 'magic' && f[1].name === 'version';
  };
  text.split(/\r?\n/).forEach((raw, i) => {
    const at = 'records.def:' + (i + 1) + ': ', c = raw.indexOf('#'), body = (c < 0 ? raw : raw.slice(0, c)).trim();
    const com = c < 0 ? '' : raw.slice(c + 1).trim();
    let m;
    if (!body) {                                               // (A comment alone: at the left, a record's words to
      if (c === 0) what = com ? [...what, com] : [];           //   come (a blank one ends them); indented, a field's
      else if (c > 0 && rec && field && /^#\s{3}/.test(raw.slice(c))) field.what.push(com);   //   going on (#, three
      else if (c > 0 && rec) { field = null; note.push(com); } //   spaces), or words before the next field
      else if (c < 0) { what = []; if (!/^\s/.test(raw)) rec = null; }
      return;
    }
    if ((m = body.match(/^define\s+([A-Z_]\w*)\s+(\d+|0x[0-9A-Fa-f]+)$/))) {
      defines.push({ name: m[1], value: m[2], what: com });
      rec = null; what = [];
      return;
    }
    if ((m = body.match(/^record\s+([a-z]\w*)\s+"([^"]+)"(?:\s+(0x[0-9A-Fa-f]+|\d+)\s+(\d+))?$/)) || (m = body.match(/^struct\s+([a-z]\w*)$/))) {
      if (rec && rec.magic !== null && !head(rec)) throw new Error('hubgen: records.def: ' + rec.name + '\'s first fields not its magic and its version');
      if (records.some(r => r.name === m[1])) throw new Error('hubgen: ' + at + m[1] + ' declared twice');
      records.push(rec = { name: m[1], file: m[2] || null, magic: m[3] ? Number(m[3]) : null, version: m[3] ? +m[4] : null, what, fields: [] });
      what = []; field = null; note = [];
      return;
    }
    if (!rec || !/^\s/.test(raw)) throw new Error('hubgen: ' + at + raw.trim() + ': a define, a record, a struct or a field?');
    m = body.match(/^((?:(?:un)?signed\s+)?(?:char|long|int)|unsigned|struct\s+[a-z]\w*)\s+([a-z_]\w*)((?:\[(?:\d+|[A-Z_]\w*)\])*)$/);
    const type = m && m[1].replace(/\s+/g, ' ');
    if (!m || !(TYPES.includes(type) || records.some(r => !r.file && 'struct ' + r.name === type)))
      throw new Error('hubgen: ' + at + raw.trim() + ': a field (a type, or a struct declared before it; a name, its dimensions)?');
    rec.fields.push(field = { type, name: m[2], dims: m[3], what: com ? [com] : [], before: note });
    note = [];
  });
  if (rec && rec.magic !== null && !head(rec)) throw new Error('hubgen: records.def: ' + rec.name + '\'s first fields not its magic and its version');
  return { defines, records };
}

// dq0rec.h's text, from records.def's
function recordsHeader({ defines, records }) {
  const pad = (s, n) => s + ' '.repeat(Math.max(1, n - s.length)), L = [];
  L.push('/*', '** dq0rec.h - the records in a save\'s folder the programs between dungeons pass to each other: made by the build',
    '** (games/dq0/hubgen.js) from games/dq0/records.def, not to be edited', '*/', '', '#ifndef _DQ0REC_H', '#define _DQ0REC_H', '');
  for (const d of defines) L.push(pad(pad('#define ' + d.name, 32) + d.value, 48) + (d.what ? '/* ' + d.what + ' */' : ''));
  for (const r of records) {
    const up = r.name.toUpperCase();
    L.push('');
    if (r.what.length) L.push('/* ' + r.what.join('\r\n** ') + ' */');
    if (r.file) L.push(pad('#define ' + up + '_FILE', 32) + '"' + r.file + '"');
    if (r.magic !== null) L.push(pad('#define ' + up + '_MAGIC', 32) + '0x' + r.magic.toString(16).toUpperCase(), pad('#define ' + up + '_VER', 32) + r.version);
    L.push('struct ' + r.name + ' {');
    for (const f of r.fields) {
      for (const n of f.before) L.push('    /* ' + n + ' */');
      L.push(pad('    ' + pad(f.type, 16) + f.name + f.dims + ';', 48) + (f.what.length ? '/* ' + f.what.join(' ') + ' */' : '').trimEnd());
    }
    L.push('};');
  }
  L.push('', '#endif', '');
  return L.join('\r\n').replace(/[ \t]+\r\n/g, '\r\n');
}

// ---- The numbers read: load () (the build's, obj/gen/dq0hub.json).  H.C.NAME a name's value; H.off (STRUCT, PATH)
// a field's offset (PATH's indices numbers or names: "door[2].rel", "mreq[ASK_UI]"; a STRUCT "hub" or "struct hub"),
// H.size (STRUCT) its size, H.size (STRUCT, PATH) a field's (less its indices given: "door" all, "door[0]" one);
// H.dims (STRUCT, PATH) its dimensions' element sizes; H.value (SPEC) ASM's specs'
function wrap(j) {
  const sels = {};                                             // (Each kind's forms chosen by a field's value)
  for (const [k, q] of Object.entries(j.msgs || {})) if (q.sel) (sels[k.split(':')[0]] = sels[k.split(':')[0]] || []).push(q);
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
    C: j.C, enums: j.enums, own: j.own, tasks: j.tasks, S: j.S, msgs: j.msgs || {}, outcome: j.outcome || [],
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
    // A message's payload (msgs.def's): payload (KIND, FORM) its fields, the form's or else the kind's, or null;
    // decode (BYTES, O, KIND, FORM) the fields of the message at BYTES[O] as { name: value } (a value of an enum's its
    // member's name, one bit's a boolean, a signed one's signed, a span's past four bytes or a name[]'s its bytes), {}
    // for a kind with none declared.  With no FORM of its own (or none of that name), the form its selector chooses, if
    // one does (the events': E_TASK's by its TE_*), else the kind's; a request's reply its KIND:reply form's
    // A save folder's record (records.def's): record (NAME, BYTES, O) the record at BYTES[O] as { field: value } (a char
    // array's its string, to its first 0, an array of them its strings; another array its numbers; a signed type's
    // signed), and ok: whether its magic and version are its record's
    // A field of a struct of records.def's its own { field: value } (an array of them, its objects), and a record's
    // magic and version, ok's, its head's if its first field is a struct (life's h)
    record: (name, b, o = 0) => {
      const r = j.records && j.records[name];
      if (!r || !r.file) throw new Error('dq0hub: no record ' + name);
      const num = (at, n, sg) => {
        let v = 0;
        for (let i = n - 1; i >= 0; i--) v = v * 256 + b[at + i];
        return sg && v >= 2 ** (8 * n - 1) ? v - 2 ** (8 * n) : v;
      };
      const str = (at, n) => { let s = ''; for (let i = 0; i < n && b[at + i]; i++) s += String.fromCharCode(b[at + i]); return s; };
      const struct = (s, base) => {
        const out = {};
        for (const [f, type] of j.records[s].fields) {
          const [off, size, ...dims] = j.S[s].f[f], at = base + off, sg = !/unsigned/.test(type) && type !== 'char';
          const sub = type.startsWith('struct ') && type.slice(7);
          const one = (a, n) => sub ? struct(sub, a) : num(a, n, sg);
          const each = (a, n, d) => Array.from({ length: n / d[0] }, (_, i) =>   // (An array's elements: each its own
            d.length > 1 ? each(a + i * d[0], d[0], d.slice(1)) : type === 'char' ? str(a + i * d[0], d[0]) : one(a + i * d[0], d[0]));   //   array's, or
          out[f] = !dims.length ? one(at, size) : type === 'char' ? (dims.length === 1 ? str(at, size) : each(at, size, dims.slice(0, -1)))   //   its
            : each(at, size, dims);                                                                                                      //   value)
        }
        return out;
      };
      const out = struct(name, o);
      let h = out;
      if (r.magic === null) return Object.assign(out, { ok: true });   // (A file of records, no head: clues.dat)
      for (let s = name; j.records[s].fields[0][1].startsWith('struct ');) { h = h[j.records[s].fields[0][0]]; s = j.records[s].fields[0][1].slice(7); }
      out.ok = h.magic === r.magic && h.version === r.version;
      return out;
    },
    payload: (kind, form) => (form && j.msgs && j.msgs[kind + ':' + form]) || (j.msgs && j.msgs[kind]) || null,
    decode: (b, o, kind, form) => {
      const field = ([, off, n, lo, hi]) => {
        let v = 0;
        for (let i = n - 1; i >= 0; i--) v = v * 256 + b[o + off + i];
        return Math.floor(v / 2 ** lo) % 2 ** (hi - lo + 1);
      };
      let p = form && j.msgs && j.msgs[kind + ':' + form];
      if (!p && sels[kind]) p = sels[kind].find(q => q.sel[1].includes(field(q.fields.find(x => x[0] === q.sel[0]))));
      p = p || H.payload(kind, '');
      const out = {};
      if (!p) return out;
      for (const x of p.fields) {
        const [name, off, n, lo, hi, en, signed, plus] = x;    // (signed: 1 a signed one, 2 a span's bytes)
        if (n > 4 || signed === 2) { out[name] = Array.from({ length: n }, (_, i) => b[o + off + i]); continue; }
        let v = field(x);
        if (signed === 1 && v >= 2 ** (hi - lo)) v -= 2 ** (hi - lo + 1);
        if (plus) v -= plus;
        if (en) {                                              // (Its names: an enum's (its member's), or constants, or
          let m;                                               //   words, a/b/c, for 0, 1, 2 ...)
          if (en.includes('/')) m = en.split('/')[v];
          else for (const c of en.split(',')) {
            const e = j.enums.find(y => y.includes(c));
            m = e ? e.find(y => j.C[y] === v) : j.C[c] === v ? c : undefined;
            if (m !== undefined) break;
          }
          out[name] = m === undefined ? v : m;
        } else out[name] = hi === lo ? !!v : v;
      }
      return out;
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
