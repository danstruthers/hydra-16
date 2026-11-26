// ****************************************************************************
// dq0.js - Dingus Quest Zero's tests' shared helpers: the lines typed (dq0Typed), what happened by meaning (dq0Turns, dq0Ev), the scenes poked (dq0Poke), the checks every dq0 test has (dq0Check).
'use strict';
const fs = require('fs');
const path = require('path');
const { VT, DEC_ASCII } = require('../../../base/sim/lib/vt.js');
const { typed } = require('./common.js');
const { trend, target, D17 } = require('./trends.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

// A natural stratum's chunks (dq0m/caves.c), read from the hub's cache in the shared segments' banks (world.h's struct
// chunk, three to a bank, CHUNK_SIZE apart): its chambers ragged (none its bounds filled, some with their rows'
// ends uneven), its tunnels bending, every open square reached from every other, and a chunk's edges' open squares
// met by its neighbours' (those in the cache); n chunks of stratum sn at least (CAVEDUMP=1: each one drawn)
// (struct chunk's places, the cache's, a stratum's record: the build's layout, games/dq0/hubgen.js's, scene1, 2026-11-24)
const HL = require('../../games/dq0/hubgen.js').load(), HO = (s, f) => HL.off(s, f);
const CHUNK_SIZE = HL.C.CHUNK_SIZE, CK_TILE = HO('chunk', 'tile'), CK_ROOM = HO('chunk', 'room'), CK_ZONE = HO('chunk', 'zone'), CK_ELEV = HO('chunk', 'elev');
const CK_NPORT = HO('chunk', 'nportals'), CK_PORTAL = HO('chunk', 'portal'), PORTAL_SIZE = HL.size('portal'), CK_NOPEN = HO('chunk', 'nopen'), CK_OPEN = HO('chunk', 'open');
const CACHE0 = HL.C['CACHE_BANK (0)'], STRATUM_SIZE = HL.size('stratum'), CSLOT_SIZE = HL.size('cslot');
const T_ROCK = 0, T_FLOOR = 1, T_WALL = 2, T_CORR = 4, T_WINDOW = 6;
// The hub's chunk cache's banks (world.h's CACHE_BANK: 5 on, 8 of them), from its bank 0's (its magic "DQ") on: none
// of the other banks taken for chunks by their looks (the UI bank's map's layers would pass, slice 4.2's)
function cacheBanks(m) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51);
  return hub === undefined ? [] : [0, 1, 2, 3, 4, 5, 6, 7].map(i => banks[hub + CACHE0 + i]).filter(b => b);
}
function cacheChunks(m, sn) {
  const out = new Map();
  for (const b of cacheBanks(m)) {
    for (let k = 0; k < 3; k++) {
      const o = k * CHUNK_SIZE, nrooms = b[o + 5], tile = b.subarray(o + CK_TILE, o + CK_TILE + 1024);
      if (b[o] !== sn || !nrooms || nrooms > 6 || b[o + 6] > 12 || b[o + 7] > 6 || tile.some(t => (t & 0x0F) > 11)) continue;   // (A kind past T_CUT, 11: the diagonal ways', 2026-10-31)
      const rooms = [];
      for (let r = 0; r < nrooms; r++) { const p = o + CK_ROOM + r * 8; rooms.push({ x: b[p], y: b[p + 1], w: b[p + 2], h: b[p + 3], flags: b[p + 4], kind: b[p + 5] }); }
      out.set((b[o + 1] | b[o + 2] << 8) + ',' + (b[o + 3] | b[o + 4] << 8), { cx: b[o + 1] | b[o + 2] << 8, cy: b[o + 3] | b[o + 4] << 8, rooms, tile: Array.from(tile, t => t & 0x0F) });
    }
  }
  return out;
}

function caveReport(m, sn, n) {
  const f = [], ks = cacheChunks(m, sn), open = t => t !== T_ROCK && t !== T_WALL && t !== T_WINDOW;
  if (ks.size < n) f.push('stratum ' + sn + ': ' + ks.size + ' chunk(s) in the cache, not ' + n);
  let ragged = 0, bends = 0;
  for (const [at, c] of ks) {
    const T = (x, y) => x < 0 || y < 0 || x > 31 || y > 31 ? T_ROCK : c.tile[y * 32 + x];
    for (const r of c.rooms) {
      if (r.kind !== 6) { f.push(at + ': a room not a cave'); continue; }
      let fl = 0; const starts = new Set();
      for (let y = r.y; y < r.y + r.h; y++) {
        let s = -1;
        for (let x = r.x; x < r.x + r.w; x++) if (T(x, y) === T_FLOOR) { fl++; if (s < 0) s = x; }
        starts.add(s);
      }
      if (r.w > 2 && r.h > 2 && fl === r.w * r.h) f.push(at + ': a chamber a rectangle (' + r.w + ' by ' + r.h + ')');
      if (starts.size >= 3 && fl * 10 < r.w * r.h * 9) ragged++;
    }
    for (let y = 0; y < 32; y++) for (let x = 0; x < 32; x++) {
      if (T(x, y) !== T_CORR) continue;
      const nb = [open(T(x, y - 1)), open(T(x + 1, y)), open(T(x, y + 1)), open(T(x - 1, y))];
      if (nb.filter(Boolean).length === 2 && !(nb[0] && nb[2]) && !(nb[1] && nb[3])) bends++;
    }
    const seen = new Set(), all = [];
    for (let i = 0; i < 1024; i++) if (open(c.tile[i])) all.push(i);
    const q = all.slice(0, 1);
    seen.add(q[0]);
    while (q.length) {
      const i = q.pop(), x = i & 31, y = i >> 5;
      for (const [dx, dy] of [[0, -1], [1, 0], [0, 1], [-1, 0]]) {
        const j = (y + dy) * 32 + x + dx;
        if (x + dx >= 0 && x + dx < 32 && y + dy >= 0 && y + dy < 32 && !seen.has(j) && open(c.tile[j])) { seen.add(j); q.push(j); }
      }
    }
    if (seen.size !== all.length) f.push(at + ': ' + (all.length - seen.size) + ' open square(s) not reached from the rest');
    for (const [dx, dy] of [[1, 0], [0, 1]]) {
      const d = ks.get((c.cx + dx) + ',' + (c.cy + dy));
      if (!d) continue;
      for (let k = 0; k < 32; k++) {
        const a = dx ? c.tile[k * 32 + 31] : c.tile[31 * 32 + k], b = dx ? d.tile[k * 32] : d.tile[k];
        if (open(a) !== open(b)) f.push(at + ': its ' + (dx ? 'east' : 'south') + ' edge\'s square ' + k + ' not met by its neighbour\'s');
      }
    }
  }
  if (process.env.CAVEDUMP) for (const [at, c] of ks) { console.log(at, JSON.stringify(c.rooms)); for (let y = 0; y < 32; y++) console.log(c.tile.slice(y * 32, y * 32 + 32).map(t => ' .#+,="<>O'[t]).join('')); }
  if (!ragged) f.push('stratum ' + sn + ': no chamber ragged');
  if (bends < 4 * ks.size) f.push('stratum ' + sn + ': ' + bends + ' bend(s) in its tunnels, fewer than 4 a chunk');
  return f;
}

const PK_PORTCULLIS = 2, T_DOOR = 3, T_STEPS = 5;           // (T_FLOOR, T_CORR: the caves' above)
// The bytes xd said after the line it was typed on (cmd: its command line), up to the prompt: each line "OFFSET  hex
// ...  text"; and a theme's text as themegen said it (its command line typed, up to the next prompt), its CRs gone
function xdBytes(out, cmd) {
  const o = out.replace(/\r/g, ''), at = o.indexOf('% ' + cmd + '\n'), end = o.indexOf('\n% ', at + 1);
  const bytes = [];
  if (at < 0) return Buffer.alloc(0);
  for (const l of o.slice(at + cmd.length + 3, end < 0 ? undefined : end).split('\n')) {
    const m = /^([0-9a-f]{7})  ((?:[0-9a-f]{2} ?)+)/.exec(l);
    if (m) for (const h of m[2].trim().split(' ')) bytes.push(parseInt(h, 16));
  }
  return Buffer.from(bytes);
}
function saidTheme(out, cmd) {
  const o = out.replace(/\r/g, ''), at = o.indexOf('% ' + cmd + '\n'), end = o.indexOf('\n% ', at + 1);
  return at < 0 ? '' : o.slice(at + cmd.length + 3, end < 0 ? undefined : end + 1);
}
const DQ0_MAKE = () => require('../../games/dq0/make.js');
// A life's record (life.dat: records.def's struct life) from its bytes (xdBytes'): the build's decoder's { h: its head's
// fields, d: [each dungeon's], ok: its magic and version }, or null short of the record; dq0Life.size () its bytes,
// dq0Life.C (NAME) a number of the headers' (records.def's LF_*, LD_*; life.h's LO_*)
const dq0Hub = () => require('../../games/dq0/hubgen.js').load();
function dq0Life(b) { const H = dq0Hub(); return b.length >= H.size('life') ? H.record('life', b) : null; }
dq0Life.size = () => dq0Hub().size('life');
dq0Life.C = name => { const v = dq0Hub().C[name]; if (v === undefined) throw new Error('dq0Life.C: no ' + name); return v; };
// A theme's text compiled and checked by the game's one theme compiler, themec, in the emulator (tools/themec.js): each
// of themes ({ name, text }) its { status, said, data, words }
const THEMEC = themes => require('../../tools/themec.js').compile(themes);
function builtChunks(m, sn) {
  const out = [], C = { room: CK_ROOM, zone: CK_ZONE, portal: CK_PORTAL, tile: CK_TILE, elev: CK_ELEV, pceil: CH_PCEIL };
  for (const b of cacheBanks(m)) {
    for (let k = 0; k < 3; k++) {
      const o = k * CHUNK_SIZE, nr = b[o + 5], nz = b[o + 6], np = b[o + CK_NPORT];
      const tile = b.subarray(o + C.tile, o + C.tile + 1024);
      if (b[o] !== sn || !nr || nr > 6 || nz > 12 || b[o + 7] > 6 || np > 16 || tile.some(t => (t & 0x0F) > 11)) continue;   // (T_CUT, 11, the diagonal ways')
      const rooms = [], zones = [], portals = [];
      for (let r = 0; r < nr; r++) {
        const p = o + C.room + r * 8;
        if (b[p + 2] < 1 || b[p] + b[p + 2] > 31 || b[p + 1] + b[p + 3] > 31) { rooms.length = 0; break; }
        rooms.push({ x: b[p], y: b[p + 1], w: b[p + 2], h: b[p + 3], flags: b[p + 4], kind: b[p + 5] });
      }
      if (!rooms.length) continue;
      for (let z = 0; z < nz; z++) {
        const p = o + C.zone + z * 11;
        zones.push({ room: b[p + 4], ceil: b[p + 8], cmid: b[p + 9], cwall: b[p + 10] });
      }
      for (let i = 0; i < np; i++) { const p = o + C.portal + i * 14; portals.push({ x: b[p], y: b[p + 1], kind: b[p + 2] }); }
      out.push({ cx: b[o + 1] | b[o + 2] << 8, cy: b[o + 3] | b[o + 4] << 8, rooms, zones, portals, pceil: b[o + C.pceil],
        tile: Array.from(tile, t => t & 0x0F), elev: Array.from(b.subarray(o + C.elev, o + C.elev + 1024), e => e << 24 >> 24) });
    }
  }
  return out;
}

// The hub's live chunks of stratum sn (dq0pure's): its world bank's cslot table (hub.h's struct wrec: the strata's
// records, 8 of 79 bytes, then the slots, 8 bytes each: used, sn, cx, cy, stamp), each slot used holding a chunk of sn
// read from its cache bank (world.h's CACHE_BANK: CACHE_BANK (0) + k / 3, CHUNK_AT: (k % 3) * CHUNK_SIZE), its 2,707 bytes (struct
// chunk's), by its place: { 'cx,cy': bytes }; the hub's banks the shared ones from its bank 0's (its magic "DQ") on
// (struct chunk's traps and what's after them, world.h's by the build's layout: a trap's knowers, 2026-11-23, made it 8
// bytes, pceil and kept the later; CH_NTRAPS its count, CH_TRAP the first, TR_SIZE each, TR_STATE, TR_HIDE in it)
const [CH_NTRAPS, CH_TRAP, TR_SIZE, TR_STATE, TR_HIDE, CH_PCEIL, CHUNK_KEPT, CHUNK_BYTES] = require('../../sim/lib/hubown.js').evaluate([
  'offsetof (struct chunk, ntraps)', 'offsetof (struct chunk, trap)', 'sizeof (struct trap)', 'offsetof (struct trap, state)',
  'offsetof (struct trap, hide)', 'offsetof (struct chunk, pceil)', 'offsetof (struct chunk, kept)', 'sizeof (struct chunk)']);
const WORLD_BANK_N = HL.C.WORLD_BANK, CSLOT_AT = HO('wrec', 'cslot');
function liveChunks(m, sn) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51), out = {};
  if (hub === undefined || !banks[hub + WORLD_BANK_N]) return out;
  const w = banks[hub + WORLD_BANK_N];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * CSLOT_SIZE, b = banks[hub + CACHE0 + Math.floor(k / 3)];
    if (!w[s] || w[s + 1] !== sn || !b) continue;
    const o = (k % 3) * CHUNK_SIZE;
    out[(w[s + 2] | w[s + 3] << 8) + ',' + (w[s + 4] | w[s + 5] << 8)] = Buffer.from(b.subarray(o, o + CHUNK_BYTES));
  }
  return out;
}

// A stratum's record in the hub's world bank (hub.h's struct wrec: the strata's records first, 8 of 79 bytes, world.h's
// struct stratum: made, n, ..., x0 at 11, y0, w, h, ..., nlinks at 22, its links from 23, 7 bytes each: kind, to, x,
// y, dug): stratum sn's bounds and links ({ x0, y0, w, h, links: [{ kind, to, x, y }] }), or null
const O_WAY_OUT = 7, O_NOWHERE = 0xFF;
function stratumRec(m, sn) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51), w = hub === undefined ? null : banks[hub + WORLD_BANK_N];
  for (let i = 0; w && i < 8; i++) {
    const r = w.subarray(i * STRATUM_SIZE, i * STRATUM_SIZE + STRATUM_SIZE), u16 = k => r[k] | r[k + 1] << 8;
    if (!r[0] || r[1] !== sn) continue;
    const links = [];
    for (let j = 0; j < Math.min(r[22], 8); j++) links.push({ kind: r[23 + j * 7], to: r[24 + j * 7], x: u16(25 + j * 7), y: u16(27 + j * 7), dug: r[29 + j * 7] });
    return { x0: u16(11), y0: u16(13), w: u16(15), h: u16(17), links };
  }
  return null;
}

// A dq0 test's start(m) for what each game made and loaded: as "> quit" is typed (the game still as it stands), the
// chunks of the top stratum in the hub's cache (builtChunks) and the strata's records (stratumRec, 0-7), t.games[k]
// for the test's game k (0 on: the hubs made, m.dq0's); and each file a program OPENs (the kernel's call, by its jump
// table slot: base/obj/gen/api.json's), its task and the hubs made by then (a game's themegen runs before its hub's
// made, its maker after), in t.opens
function dq0Made(t, m) {
  const kl = require('../../../base/sim/run.js').labels(), cpu = m.cpu, step = cpu.step.bind(cpu);  // (The kernel's own OPEN:
  const K_OPEN = kl.byName.get('K_OPEN'), K_OPEN_PAGE = kl.pageOf.get('K_OPEN');                //   its loader, BANK_LOAD, opens a
  const K_STAT = kl.byName.get('K_STAT'), K_STAT_PAGE = kl.pageOf.get('K_STAT');                //   child itself; a STAT's open,
  const games = t.games = [], opens = t.opens = [], statting = new Set();                       //   its cache's check, not counted)
  let out = m.out, qpos = 0;
  cpu.step = v => {
    if (cpu.PC === K_STAT && m.W === K_STAT_PAGE) statting.add(m.T);
    else if (cpu.PC === K_OPEN && m.W === K_OPEN_PAGE && statting.has(m.T)) statting.delete(m.T);
    else if (cpu.PC === K_OPEN && m.W === K_OPEN_PAGE) {
      const r = m.taskRam[m.T], a = r[2] | r[3] << 8;
      let s = '';
      for (let i = 0; i < 64 && a + i < 0x8000 && r[a + i]; i++) s += String.fromCharCode(r[a + i]);
      opens.push({ T: m.T, path: s, hubs: m.dq0 ? m.dq0.games.length : 0 });
    }
    return step(v);
  };
  Object.defineProperty(m, 'out', { get: () => out, set: v => {
    let i;
    while ((i = v.indexOf('> quit', qpos)) >= 0) {
      if (m.dq0 && m.dq0.games.length) games[m.dq0.games.length - 1] = { chunks: builtChunks(m, 0), strata: [0, 1, 2, 3, 4, 5, 6, 7].map(sn => stratumRec(m, sn)) };
      qpos = i + 6;
    }
    out = v;
  } });
}
// Two chunks' bytes compared (struct chunk's: rooms from 8, 8 bytes each, their flags the 5th; openings 188-217, their
// count at 7; portals from 219, 14 bytes each, their state, parts, harm the 4th, 9th and 14th; squares from 443, their
// floors from 1467; traps from CH_TRAP, TR_SIZE bytes each, their state at TR_STATE): the squares' seen, visible and drawn bits
// (T_SEEN, T_VIS, T_DRAWN) and the rooms' visited (R_VISITED) aside, each byte that differs by what it is: portal, trap, square,
// opening, or other (none should be: the maker's own)
function chunkDiff(a, b) {
  const d = { portal: 0, trap: 0, square: 0, opening: 0, other: [] };
  for (let i = 0; i < CHUNK_BYTES; i++) {
    let x = a[i], y = b[i];
    if (i >= CHUNK_KEPT && i < CHUNK_KEPT + 9) continue;      // (The world's memory of it, the maker's none)
    if (i >= CK_TILE && i < CK_ELEV) { x &= 0x2F; y &= 0x2F; }
    if (i >= CK_ROOM && i < CK_ZONE && (i - CK_ROOM) % 8 === 4) { x &= ~1; y &= ~1; }
    if (x === y) continue;
    if (i >= CK_PORTAL && i < CK_TILE && [3, 8, 13].includes((i - CK_PORTAL) % PORTAL_SIZE)) d.portal++;
    else if (i >= CH_TRAP && i < CH_TRAP + 6 * TR_SIZE && (i - CH_TRAP) % TR_SIZE === TR_STATE) d.trap++;
    else if (i >= CK_TILE && i < CK_ELEV) d.square++;
    else if (i >= CK_ELEV && i < CK_ELEV + 1024 && (a[i - 1024] & 0x0F) !== (b[i - 1024] & 0x0F)) d.square++;   // (A changed square's floor)
    else if (i === CK_NOPEN || (i >= CK_OPEN && i < CK_NPORT)) d.opening++;
    else d.other.push(i);
  }
  return d;
}

// The dq0 tests' story: the output with its escapes and the lines' wrapping taken out; and its words looked for each
// in its turn (after the one before), what isn't found pushed on f (its name first)
// The status line as the screen showed it after each of its draws (the game draws only from its first column changed:
// "torch 15" of "torch 15m" when the m was there already), one a line, for a check of what it said
const dq0StatusRows = out => { const v = new VT({ onlcr: true }); return out.split('\x1b[0m').map(c => v.write(c + '\x1b[0m').lines()[0]).join('\n'); };
const dq0Story = out => out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' ');
// The story's prose alone: the status line's draws left out (drawn in reverse video on its own row, wherever they fall
// among an answer's words, as dq0Timer's story has it), for a check of what an answer said in order (2026-11-18:
// dq0follow's "[######....] torch 59m UP 0 facing W" between a follow's two sentences, its long task's bar drawn while it
// was under way; the story itself keeps them, the checks of what the status line said read it there)
const dq0Prose = out => dq0Story(out.replace(/\x1b\[7m[\s\S]*?\x1b\[0?m/g, ' '));
function inTurn(o, want, f, name = '') {
  let at = 0;
  for (const w of want) {
    const i = o.indexOf(w, at);
    if (i < 0) { f.push(name + 'not found (in its turn): ' + w); return; }
    at = i + w.length;
  }
}

// Nothing told twice in one answer (the user's, 2026-10-17: "some messages are duplicated, like when describing a
// trap, it is shown twice in the 'look' results"): every dq0 test's answers (dq0Answers: the story between one line
// entered, the game's "> " or rc's "% ", and the next; the status line (reverse video), the line typed as it's drawn
// and the escapes out, its lines' wrapping undone), each sentence in one of them once (dq0Repeats: those said more,
// { test, cmd, sentence, n }), but those DQ0_REPEATS allows, each by its words (and the tests it's in, if only some),
// with its reason; and the hub's live chunks (dq0Doubled) with no two portals or traps on one square (a door made
// twice, its trap twice: the look told it twice).  sim/test.js's dq0 check, in every dq0 test (HUBOWN=0 too).  One list
// with sim/lib/dq0inv.js's repeat invariant (fix3, 2026-11-25: its own list had none of the fight's words, "It falls
// senseless.", allowed here): an entry marked goes is told as it goes, each time, as often as it happens (the user's
// decisions: a fight's blows, a dig's spells, a fire-maker's tries); the rest three times in one reply at most there
const DQ0_REPEATS = [
  { goes: true, what: /^You reach for the [a-z ]+\.$/, why: 'a thing handled, each command of a line (or typed ahead) its own first word: dq0firstword holds it to once a command' },
  { what: /^(Put down\.|You fumble for the sword\.|You see none of those here\.|You see no [a-z ]+ here\.|You carry none of those\.|You have no [a-z ]+\.|Taken\.)/, tests: /^dq0engage$/, why: 'its lines typed ahead, one answer' },
  { what: /^The [a-z ]+ (unslings|draws) (?:its|their) [a-z ]+\.$/,
    why: 'several of a kind readying their weapons in one answer, each one\'s told (slice 4.7\'s: three elves unslinging their hartbows)' },
  { what: /^Your hold gives way, \d+ feet up: you fall\.$/, why: 'a climb\'s holds given way and caught, each told as it comes, the climb on from the same height (dq0rope\'s grapple, 2026-11-06)' },
  { goes: true, what: /^\(\d+ feet to go\.\)/, why: 'a dig\'s spells, each told as it ends (dig down: many spells to an answer)' },
  { goes: true, what: /^(?:The work goes (?:well|slowly)|Your tool slips and turns; little comes of that spell of work|The rotten rock splits over you, and part of the roof falls into the hole)\.$/,
    why: 'a dig\'s spells, each told as it ends (a long dig: hours of spells between one stir and the next, some the same)' },
  { goes: true, what: /^You (?:climb on|slip back a little|find no hold yet) \((?:\d+ feet|a foot) to go\)\.$/,
    why: 'a climb\'s rounds, each told as it ends (a long climb: many rounds to an answer, some the same)' },
  { goes: true, what: /^(?:(The|A) .* |Something )(bites|claws|strikes|swings|lunges|thrusts|shoots|throws)\b.*\bat you\b|^You (strike|swing|thrust|punch|kick|lunge|shoot|throw)\b|^(?:Your|Its|Their) [a-z]+ strikes .* instead\b/,
    why: 'a fight\'s rounds, each blow told as it falls (attack: the rounds till it ends, the same words some of them; in the dark "Something", unseen: dq0traces\' rats)' },
  { goes: true, what: /^(?:You get to your feet|You fall|Everything goes black again|You hear something fall|The [a-z ]+ (?:gets to (?:its|their) feet|scrambles to its feet|coils itself up again|rights itself|flutters up again|rises again|falls|goes down|is thrown down|is flipped onto its back|drops to the floor|sinks to the floor))\.$/,
    why: 'a fight\'s falls and risings, each fall told as it falls and each rise as it gets up (a long fight: a creature down twice, a stumble\'s, a leg\'s wound\'s, up each time; dq0missile\'s rat; every fall told since 2026-10-30; the character knocked senseless a third time in one answer, "again" again: blows.c\'s harm_told, dq0antidote\'s adders since 2026-11-09)' },
  { goes: true, what: /^The [a-z ]+ (?:struggles|scrabbles|writhes|flails its legs|flaps|wavers), but can't (?:get up|right itself|rise)\.$/,
    why: 'creatures down: each told once as it lies there (2026-11-02), so only two of a name in one answer, or one wounded between (dq0downonce holds it to that)' },
  { goes: true, what: /^(?:You grapple with .+, but (?:it twists|they twist) free|You choke .+|(?:The [a-z ]+|Something) (?:struggles in your grip|resists your hold))\.$/,
    why: 'a grapple fought on (HOUSE.md\'s grappling, 2026-11-09): each round\'s try told as it comes, a hold missed, a choke, a held one\'s struggle, the same words some of them' },
  { goes: true, what: /^You turn to (?:something|the [a-z ]+)\.$/,
    why: 'a fight\'s turns to a foe, each told as it turns (fighting on: struck from another side again and again; in the dark "something": dq0scent\'s rats, their dice by who asks, 2026-10-30)' },
  { goes: true, what: /^The [a-z ]+ closes with you\.$/,
    why: 'each denizen\'s attack begun, told as it closes with the character (the user\'s, 2026-11-17: its first words, "The bandit closes with you."): two of a name closing in one answer, the same words (dq0fightverbs\' charge, both rats), or one coming back to the fight' },
  { goes: true, what: /^The [a-z ]+ takes aim at you\.$/,
    why: 'each shot at the character begun, told as the shooter takes aim (the user\'s, 2026-11-18: one path, the character\'s "You take aim at ..." the other way round): a shot a round, several in one wait' },
  { what: /^(?:You (?:look over|poke through|go through|feel over|feel through) the .+|You find nothing in the .+|Your hands find nothing in the .+|There's nothing (?:in|on) the .+|It's closed: you go over the outside of it|(?:It's|They're) made of .+|You stop short: just ahead is a trap, .+)\.$/,
    why: 'a room searched whole, each thing in it in turn (HOUSE.md\'s, 2026-11-02): two heaps of straw, two iron pots, told each as it\'s gone through' },
  { what: /^You find (?:a trap|a door)[^.]*\.$/, why: 'a room searched whole\'s finds, each told as it\'s found: two plates, two doors' },
  { what: /^You hear /, why: 'a sound heard each time it\'s made (main.c\'s heard: half a minute apart the same way, five seconds any; a search\'s or a wait\'s minute hears several)' },
  { what: /^You go over to the .+\.$/, why: '"take all"\'s things, walked over to each in turn: two heaps of potsherds, each its walk (the soak\'s seed 21)' },
  { what: /^(?:The [a-z ]+|[A-Z][a-z]+) (?:snores|says something to (?:its|their) fellow|rallies (?:its|their) fellows|raises the alarm)\.$/,
    why: 'a sound made each time, seen as it\'s made as one heard is told each time ("You hear": main.c\'s heard weighs both, half a minute apart the same way): a sleeper seen snoring twice in a long check (the soak\'s seed 11, 2026-11-06)' },
  { what: /^(?:The wall to your (?:left|right) ends|A wall begins to your (?:left|right)|You feel a door to your (?:left|right)|You feel your way on|The way bends|The way turns (?:left|right))\.$/,
    why: 'a walk by feel, its beats as it goes: a wall can end more than once along a winding way, more than one door felt, each its own (the soak\'s seeds 149, 200, by feel: told three times)' },
  { what: /^(The passage turns (left|right)|The passage bends away [a-z-]+|You go through a doorway|You go on down the passage|The way (rises|falls away))\.$/,
    why: 'a walk\'s beats as it goes (slice 4.0): a winding passage turns or bends the same way more than once (a bend\'s heading, 2026-11-01: the soak\'s seed 200), has more than one doorway, goes on quietly every 30 feet' },
  { goes: true, what: /^The way (?:rises|falls away)\.$/, why: 'a walk\'s beats as it goes (slice 4.0\'s and 4.10\'s): an incline told four feet of rise at a time, dq0incline\'s two or more (the passage\'s other beats above, three at most in a reply: sim/lib/dq0inv.js\'s repeat)' },
  { what: /^(?:You go under!|You come up, gasping\.)$/,
    why: 'a swim\'s minutes as a walk goes on: under, up and under again, each told as it comes (the soak\'s seed 192)' },
  { goes: true, what: /^(?:It falls senseless|They fall senseless|It flees|They flee|The .+ falls dead|You finish the .+ where (?:it lies|they lie)|The .+ gets to (?:its|their) feet|You get to your feet|You turn to the .+|You fight back against the .+)\.$/,
    why: 'fighting on (the user\'s, 2026-10-20): one attack fights its foes in turn, each falling, fleeing, getting up, fought back or turned to told as it comes (two of a name, the same words)' },
  { goes: true, what: /^(?:You drop your .+|(?:The .+|[A-Z][a-z]+) drops (?:its|their) .+|The .+ snatches up (?:its|their) .+)\.$/,
    why: 'a fight\'s fumbles (slice 4.1b\'s hands), each told as it comes: a thing let fall at each fumble (two torches held, two fumbles), a denizen\'s weapon dropped and snatched up again its next turn, twice in a long answer\'s rounds (the soak\'s seed 85, 2026-10-27)' },
  { goes: true, what: /^(?:You stop fighting|You fumble for the .+)\.$/,
    why: 'commands typed ahead as the character fights, each taking the fight over in turn, fought on between them (fighting on), their answers one' },
  { goes: true, what: /^Something stirs\.$/, why: 'each stir\'s first words: one for each time what it sees of the denizens changes (a long command\'s answer has several)' },
  { goes: true, what: /^Nothing stirs in sight now\.$/, why: 'each stir\'s words when what it saw is gone, a stir\'s telling of its own as "Something stirs." is (dq0Repeats tells the stirs apart; the repeat invariant counts a reply: a rat at the edge of sight, four stirs in dq0torchwords\' dip of a torch, fix3)' },
  { goes: true, what: /^(?:You strike a spark|It doesn't catch)\.$/, why: 'a fire-maker\'s tries, each told as it goes (the user\'s, 2026-11-02: "continue until successful, reporting as it goes along")' },
  { goes: true, what: /^You keep at it\.$/,
    why: 'a battering\'s beat every fourth blow once its blows tell (the user\'s, 2026-10-22: told as it goes), twice or more in a dozen' },
  { what: /^Everything goes black\.$/, tests: /^dq0(?:finished|strike|helpless|venom|morale)$/,
    why: 'knocked senseless, come round in shock a round later, struck senseless again, all in one wait\'s answer (the UI waiting it out): two black-outs, each told (dq0strike\'s rats, fought with a bar, three; dq0morale\'s small orcs, a knife in the hand of each)' },
  { what: /^Everything goes black again\.$/, tests: /^dq0soak-/,
    why: 'as "Everything goes black." above, its second time and after, in a soak\'s fight' },
  { what: /^The [a-z]+ [a-z ]+ to your [a-z ]+ is gone\.$/, tests: /^dq0soak-/,
    why: 'the time between\'s healing: two wounds alike on one place, each told' },
  { goes: true, what: /^(?:Quicker, you strike at .+|Something coils itself up again\.)$/, tests: /^dq0soak-/,
    why: 'a fight\'s long answer, a soak\'s: the same blow come out the same way two rounds of it, an adder out of sight coiling up twice' },
  { what: /^Enter takes /, tests: /^dq0soak-/,
    why: 'the time between\'s question asked again, a soak\'s answers refused (its echo in the sentence: dq0acct\'s prompt no game\'s)' },
  { what: /^You stop walking\.$/, tests: /^dq0soak-/,
    why: 'one walk stopped as the line came, a walk set off by the line stopped by a shot at it, each told (the fight soak\'s seed 85, 2026-11-04)' },
  { goes: true, what: /^Something gets to its feet\.$/, tests: /^dq0soak-/,
    why: 'a fight\'s falls and risings, as the named, one out of sight down and up twice in a long answer (the fight soak\'s seed 66, 2026-11-04)' },
  { what: /^You feel your way [a-z]+\.$/, tests: /^dq0soak-/,
    why: 'a walk over to a thing in the dark, each leg of it told (the first stopped by what it felt)' },
  { what: /^The bite burns: there's venom in it\.$/,
    why: 'a fight\'s long answer: each venomous bite that gets through told as it falls, a dose more (the fight soak\'s seed 3, 2026-11-04)' },
  { goes: true, what: /^You struggle, but can't get up\.$/,
    why: 'the character\'s own Rise tried each round and failed (the book\'s test for one seriously or grievously wounded, slice 4.1b\'s), each try told' },
  { what: /^(?:The passage (?:forks here|ends here; it goes back behind you|comes to a door; it goes back behind you)|You are in a passage|It goes .+)\.$/,
    tests: /^dq0travel$/,
    why: 'a journey\'s legs (slice 4.2\'s travel), each place it comes to told whole as it gets there (the UI\'s told begun again each leg, a look of its own): one fork or stretch of passage much like another in words' },
];
function dq0Answers(out) {
  const s = out.replace(/\r/g, '').replace(/\x1b\[7m[\s\S]*?\x1b\[0m/g, '')
    .replace(/\x1b\[\d+;1H\x1b\[K(?:\x1b\[\?25h)?[^\x1b\n]*/g, '').replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '');
  const res = [];
  let cmd = '', ans = [];
  for (const l of s.split('\n')) {
    const p = l.match(/^[>%] (.*)$/);
    if (p) { res.push({ cmd, text: ans.join(' ').replace(/\s+/g, ' ').trim() }); cmd = p[1].trim(); ans = []; continue; }
    ans.push(l);
  }
  res.push({ cmd, text: ans.join(' ').replace(/\s+/g, ' ').trim() });
  return res;
}
function dq0Repeats(name, out) {
  const found = [];
  for (const a of dq0Answers(out)) {
    const parts = Math.max(a.cmd.split(/ and |,/).length,   // (A list's parts each told, "Put down." "Put down.": games/dq0/parts.c; no oftener than it has parts;
      /^\S+(?: [a-zA-Z]{1,2}){2,}$/.test(a.cmd) ? a.cmd.split(' ').length - 1 : 1);   //   several letters a line its commands too, "drop h i": letters.c's, each its reach's steps, the game as played)
    // (Each stir a telling of its own: a denizen's place told again after a change between, "two rats" then "a rat",
    // is what changed, not a repeat; within a telling, a look's or a stir's, nothing twice)
    for (const part of a.text.split(/(?=Something stirs\.)/)) {
      const n = new Map();
      for (const t of part.split(/(?<=[.!?])\s+/)) if (t.length > 8 && /[a-z]{2}/i.test(t)) n.set(t, (n.get(t) || 0) + 1);
      for (const [sentence, c] of n) {
        if (c < 2 || (parts > 1 && c <= parts) || DQ0_REPEATS.some(r => r.what.test(sentence) && (!r.tests || r.tests.test(name)))) continue;
        found.push({ test: name, cmd: a.cmd, sentence, n: c });
      }
    }
  }
  return found;
}
// The hub's live chunks (world.h's struct chunk: portals from 219, 14 bytes each, their count at 218; traps from CH_TRAP,
// TR_SIZE bytes each, their count at CH_NTRAPS), each with two portals or two traps on one square: 'sn:cx,cy: 2 portals at x,y'
function dq0Doubled(m) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b), f = [];
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51);
  if (hub === undefined || !banks[hub + WORLD_BANK_N]) return f;
  const w = banks[hub + WORLD_BANK_N];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * CSLOT_SIZE, b = banks[hub + CACHE0 + Math.floor(k / 3)];
    if (!w[s] || !b) continue;
    const o = (k % 3) * CHUNK_SIZE, at = w[s + 1] + ':' + (w[s + 2] | w[s + 3] << 8) + ',' + (w[s + 4] | w[s + 5] << 8);
    for (const [what, n, from, size, max] of [['portals', b[o + CK_NPORT], CK_PORTAL, PORTAL_SIZE, 16], ['traps', b[o + CH_NTRAPS], CH_TRAP, TR_SIZE, 6]]) {
      const on = new Map();
      for (let i = 0; i < n && i < max; i++) { const p = o + from + i * size, sq = b[p] + ',' + b[p + 1]; on.set(sq, (on.get(sq) || 0) + 1); }
      for (const [sq, c] of on) if (c > 1) f.push(at + ': ' + c + ' ' + what + ' at ' + sq);
    }
  }
  return f;
}
// (A game over early, its lines typed to rc: a test's own checks may pass on what it never ran, 2026-11-03: dq0lair's look and search; a
// test whose game's meant to end names it, lostLines: true)
function dq0Lost(name, m) {
  const t = require('../tests.js').tests.find(x => x.name === name.replace(/@\d+$/, ''));   // (The index's: loaded by then)
  if ((t && t.lostLines) || /^dq0soak-/.test(name)) return [];   // (The soak's scripted lines outlast a death, as they may)
  const e = String.fromCharCode(27), o = (m.out || '').split(e + '[').map((s, i) => i ? s.replace(/^[0-9;?]*[A-Za-z]/, '') : s).join('');
  const lost = [...o.matchAll(/^rc: ([a-z0-9]+): not found/gm)].map(x => x[1]).filter(w => !/^(?:dq0|hmchar|quit)$/.test(w));   // (A last quit: nothing lost by it)
  return lost.length ? ['lines typed to rc, the game over before them: ' + lost.slice(0, 6).join(', ')] : [];
}
// The scenes posted (dq0Scene's, dq0SceneNow's) not put in place: each one's name and why
function dq0ScenesLost(m) {
  const e = m.dq0Scenes;
  return !e ? [] : [...e.lost, ...[e.cur, ...e.queue].filter(Boolean).map(x => x.why + ': not in place by the test\'s end')];
}
function dq0Check(name, m) {
  return [...dq0Repeats(name, m.out).map(r => 'told ' + r.n + ' times in one answer (> ' + r.cmd + '): ' + JSON.stringify(r.sentence)),
    ...dq0Doubled(m).map(s => 'a chunk\'s fixtures doubled: ' + s), ...dq0Lost(name, m),
    ...dq0ScenesLost(m).map(s => 'a scene not in place: ' + s),
    ];
}

// What a dq0 test's lines did, by their meaning (Phase 3A: tests cut to meaning, not words): sim/lib/dq0meaning.js's
// record (m.dq0, in every dq0 test: the world's events, the denizens', the requests' replies, the state each line
// found as it was entered), a line at a time.  dq0Turns(m): the lines typed (as the serial port gave them, each to its
// CR), each { k, line, game (the game its events are in, 0 on), events, dens, replies, intents (the UI's and the
// denizens': ring, kind, who and their payloads' fields by name, games/dq0/msgs.def's: e.tenths, e.task), at (the state
// the line found), after (the next line's, or the test's end's) }; T.of(line, from): the first turn of that line from
// turn from on (or
// one of nothing, its k -1), T.after(turn, line): the first of that line after it.  A meaning test (meaning: true)
// checks only these, and nothing of the ruleset's own (its numbers, its dice's outcomes, its words): the null ruleset
// runs them too (dq0null's agent's)
function dq0Turns(m) {
  const r = m.dq0 || { games: [], lines: [], final: () => null }, all = { events: [], dens: [], replies: [], snaps: [], intents: [] };   // (None: sim/dl's)
  r.games.forEach((g, gi) => { for (const k of Object.keys(all)) for (const e of g[k] || []) all[k].push(Object.assign({ game: gi }, e)); });
  const fin = r.final(), of = (list, k) => list.filter(e => e.line === k + 1);
  // (A dq0 line as the test wrote it: sim/test.js's -F, -K, -N, -I and -a, RULES=NAME's -r NAME, and the dice's offset,
  // -R N (DQ0_ROLLS's, or a test's own: not a game's command), taken off: each once, the last added first, as sim/test.js
  // adds them, so a test's own flag before them is kept (dq0travel's -N; a test's own -R last of its own, taken off)
  const bare = l => !/^dq0( |$)/.test(l) ? l : [/ -R \d+$/, / -E$/, / -F$/, / -K$/, / -N$/, / -J$/, / -I$/, / -a$/, / -r \S+$/, / -R \d+(?=(?: -S \S+)?$)/].reduce((x, re) => x.replace(re, ''), l);   // (A test's own -R before its scene's -S too)
  const T = r.lines.map(bare).map((line, k) => ({ k, line, events: of(all.events, k), dens: of(all.dens, k), replies: of(all.replies, k), intents: of(all.intents, k),
    at: all.snaps.find(s => s.line === k + 1) || null }));
  T.forEach((t, k) => {
    t.after = k + 1 < T.length ? T[k + 1].at || fin : fin;
    t.game = t.events.length ? t.events[0].game : t.after ? t.after.game : -1;
  });
  const none = { k: -1, line: '', events: [], dens: [], replies: [], intents: [], at: null, after: null, game: -1 };
  T.of = (line, from = 0) => T.find(t => t.k >= from && t.line === line) || Object.assign({}, none, { line });
  T.after = (t, line) => T.of(line, t.k + 1);
  // (A turn and the one after, as one: a quit with a save folder has its end's events, its save, as the next line's
  // typed, the game's "> " back before it's done)
  T.and = t => { const u = T[t.k + 1]; return t.k < 0 || !u ? t : Object.assign({}, t, { events: [...t.events, ...u.events], dens: [...t.dens, ...u.dens],
    replies: [...t.replies, ...u.replies], intents: [...t.intents, ...u.intents], after: u.after }); };
  return T;
}
// A turn's events (the world's and the denizens') of a kind, with the fields given (each a value, or a test of it)
const dq0Ev = (t, kind, want = {}) => [...t.events, ...t.dens].filter(e => e.kind === kind &&
  Object.entries(want).every(([k, v]) => (typeof v === 'function' ? v(e[k]) : e[k] === v)));
const dq0Rep = (t, req, want = {}) => t.replies.filter(e => e.req === req && Object.entries(want).every(([k, v]) => (typeof v === 'function' ? v(e[k]) : e[k] === v)));
// A turn's events as words, for a failure's
function dq0Evs(t) {
  if (t.k < 0) return '"' + t.line + '" not typed';
  const one = e => e.kind + ['how', 'why', 'po', 'hc', 'band', 'to', 'tk', 'ir', 'de', 'out'].filter(k => e[k] !== undefined && e[k] !== false).map(k => ' ' + e[k]).join('');
  return '> ' + t.line + ': ' + ([...t.events, ...t.dens.filter(e => e.kind === 'E_FIGHT')].map(one).join(', ') || 'no events');
}
// A check: event kind (want's fields) in turn t, or (never) none; what the failure says
function dq0Need(f, t, kind, want, what) { if (!dq0Ev(t, kind, want).length) f.push(what + ' (' + dq0Evs(t) + ')'); }
function dq0Never(f, t, kind, want, what) { if (t.k >= 0 && dq0Ev(t, kind, want).length) f.push(what + ' (' + dq0Evs(t) + ')'); }
// The things carried (the item store's: on the character, or in what it carries), by kind's name: [{ name, flags, ... }]
function dq0Carried(s) {
  if (!s) return [];
  const on = new Set(), by = new Map(s.items.map(i => [i.n, i]));
  const mine = i => { for (let d = 0; i && d < 8; d++) { if (i.where === 'IW_ACTOR') return i.x === 0; if (i.where !== 'IW_ITEM') return false; i = by.get(i.x); } return false; };
  return s.items.filter(i => mine(i) && !on.has(i.n) && on.add(i.n));
}
// The things lying on the character's square (its stratum's), by name
const dq0Here = s => s ? s.items.filter(i => i.where === 'IW_SQUARE' && i.sn === s.pc.sn && i.x === s.pc.x && i.y === s.pc.y) : [];
const dq0Named = (list, name) => list.filter(i => i.name === name);
// A thing by its kind's name: the one carried, else the one on the character's square or in (or on) one there, else
// any (null: none); and what holds it (the item it's in or on, or null)
function dq0Thing(s, name) {
  if (!s) return null;
  const by = new Map(s.items.map(i => [i.n, i])), all = s.items.filter(i => i.name === name), carried = new Set(dq0Carried(s).map(i => i.n));
  const here = i => { for (let d = 0; i && d < 8; d++) { if (i.where === 'IW_SQUARE') return i.sn === s.pc.sn && i.x === s.pc.x && i.y === s.pc.y; if (i.where !== 'IW_ITEM') return false; i = by.get(i.x); } return false; };
  return all.find(i => carried.has(i.n)) || all.find(here) || all[0] || null;
}
const dq0Holder = (s, i) => s && i && i.where === 'IW_ITEM' ? s.items.find(j => j.n === i.x) || null : null;
// (What holds thing i at the last, in the snapshot s: itself if it's in nothing; its holder's holder ... the things' own
//   home, an actor's IW_DEN or the character's IW_ACTOR: the one-path plan's stage 2, things, 2026-11-23)
const dq0Root = (s, i) => { for (let n = 0; i && i.where === 'IW_ITEM' && n < 8; n++) i = dq0Holder(s, i) || i; return i || {}; };
// What the character perceives of the place (the senses' record, struct sight), the denizens aside (they go their own
// ways), as one string: the same place seen the same, compared
const dq0View = s => !s || !s.sight ? '' : JSON.stringify(Object.assign({}, s.sight, { seq: 0, dens: [], first: 0 }));
const DQ0_I = { LIT: 0x01, HELD: 0x02, WORN: 0x04, HIDDEN: 0x08 };   // (items.h's I_*)
// The place the character's in come to see again n times (its passings, the map's: the torch put out five minutes,
// lit again; dq0bearing's)
const passings = n => Array(n).fill(['douse the torch', 'wait 5 minutes', 'light the torch']).flat();

// A dq0 test's lines typed, each at its prompt (rc's "% ", the game's "> ")
const dq0TypedAsk = lines => lines.map(l => '\u0101' + l + '\r').join('');
// (And every dq0 line after a scene's start written, echo K N > /ram/s.sc, given -S /ram/s.sc: dq0 -S's, the environment's
// dq0hour ... till 2026-11-24; a line given it already as it is)
const dq0Scened = lines => { let on = false; return lines.map(l => { if (/^echo [a-z]+ \d+ >>? \/ram\/s\.sc$/.test(l)) on = true; return on && /^dq0( |$)/.test(l) && !/ -S /.test(l) ? l + ' -S /ram/s.sc' : l; }); };
const dq0Typed = lines => dq0TypedAsk(dq0Scened(lines.some(l => /^dq0 .*-d /.test(l)) ? ['dq0auto=1', ...lines] : lines));
// A dq0 test's shell lines, the prelude every test types written once (the tests' scan: 381 copies of it): the card's
// games bound, the character made (char: its hmchar line, the usual hunter's by default; false none), the test's own
// shell lines (pre: its skills, its folders), a theme (theme: its text, or [folder, its text]: the folder made, its
// theme.txt written; /ram/g by default), a scene's start (scene: { hour, sky, air, well, furn }, games/dq0/hub.h's
// sc_*: its file /ram/s.sc written, and every dq0 line given -S /ram/s.sc), the game's line (game: its text, or
// { seed, pace, T, kit, flags }: dq0 -p PACE -s SEED -c /ram/c.hm ...), then its lines (lines, and every dq0 line
// among them given the scene's -S too), and the end (end: false none; else quit and echo $status)
const DQ0_CHAR = 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm';
function dq0Game(o = {}) {
  const out = ['bind -a /sd/0/games /bin'], S = ' -S /ram/s.sc';
  if (o.char !== false) out.push(o.char || DQ0_CHAR);
  out.push(...(o.pre || []));
  if (o.theme) {
    const [dir, text] = Array.isArray(o.theme) ? o.theme : ['/ram/g', o.theme];
    out.push('mkdir ' + dir, "echo '" + text + "' > " + dir + '/theme.txt');
  }
  const sc = Object.entries(o.scene || {});
  sc.forEach(([k, v], i) => out.push('echo ' + k + ' ' + v + (i ? ' >> ' : ' > ') + '/ram/s.sc'));
  const g = o.game === undefined ? null : typeof o.game === 'string' ? o.game : 'dq0 -p ' + (o.game.pace || 't') + ' -s ' + (o.game.seed === undefined ? 3 : o.game.seed) +
    ' -c /ram/c.hm' + (o.game.T === false ? '' : ' -T') + (o.game.kit ? ' -k ' + (/[ ,]/.test(o.game.kit) ? "'" + o.game.kit + "'" : o.game.kit) : '') + (o.game.flags ? ' ' + o.game.flags : '');
  const withS = l => sc.length && /^dq0( |$)/.test(l) ? l + S : l;
  if (g) out.push(withS(g));
  out.push(...(o.lines || []).map(withS));
  if (o.end !== false && g) out.push('quit', 'echo $status');
  return out;
}

// A door's (or any portal's) record in the chunks' cache, square x, y of stratum sn (its chunk cached): { b its bank,
// q its offset there, state its state's offset, kind its kind } or null; the offsets world.h's, by cc65 (hubown's
// evaluate), as the headers stand.  A test's scene poked through it (a door broken down, left open: dq0builder, dq0mine)
let dq0PortalOff = null;
function dq0Portal(m, sn, x, y) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b), h0 = m.dq0.bank(0);
  // (This game's hub, not an earlier game's left in its banks: its bank the recorder's)
  const hub = keys.find(k => banks[k] === h0 || (banks[k].buffer === h0.buffer && banks[k].byteOffset === h0.byteOffset)), w = hub === undefined ? null : banks[hub + WORLD_BANK_N];
  if (!w) return null;
  let S = null;
  for (let i = 0; i < 8 && !S; i++) {
    const r = w.subarray(i * STRATUM_SIZE, i * STRATUM_SIZE + STRATUM_SIZE), u16 = k => r[k] | r[k + 1] << 8;
    if (r[0] && r[1] === sn) S = { x0: u16(11), y0: u16(13) };
  }
  if (!S) return null;
  const [PORTAL, PSIZE, PSTATE, NPORT, PKIND] = dq0PortalOff || (dq0PortalOff = require('../../sim/lib/hubown.js').evaluate([
    'offsetof (struct chunk, portal)', 'sizeof (struct portal)', 'offsetof (struct portal, state)', 'offsetof (struct chunk, nportals)',
    'offsetof (struct portal, kind)']));
  const cx = (x - S.x0) >> 5, cy = (y - S.y0) >> 5, lx = (x - S.x0) & 31, ly = (y - S.y0) & 31;
  for (let k = 0; k < 24; k++) {
    const sl = CSLOT_AT + k * CSLOT_SIZE;
    if (!w[sl] || w[sl + 1] !== sn || (w[sl + 2] | w[sl + 3] << 8) !== cx || (w[sl + 4] | w[sl + 5] << 8) !== cy) continue;
    const b = banks[hub + CACHE0 + Math.floor(k / 3)], o = (k % 3) * CHUNK_SIZE;
    for (let i = 0; i < b[o + NPORT]; i++) {
      const q = o + PORTAL + i * PSIZE;
      if (b[q] === lx && b[q + 1] === ly) return { b, q, state: q + PSTATE, kind: b[q + PKIND] };
    }
  }
  return null;
}

// ---- The scene builder (phase C, scene1, 2026-11-24: games/dq0/hub.h's scene, in place of the pokes): what a test sets up
// said to the game as ops, each fact done by its owner between its intents, so no copy of another task's writes it back
// (OPEN.md's G18, X1).  A scene's built by a test's own code as the pokes were, on copies: the hub's banks (m.dq0.bank,
// m.sharedBanks: each one read is a copy while it's built) and the minds' records (dq0Rec's mem); then each byte changed
// is an op, its owner's by the hub's table of writers (hub.h's own lines, sim/lib/hubown.js's map): the world's SO_HUB,
// the minds' SO_DHUB, the records' SO_REC; the referee's (a sheet, a body, the character out) and an intent posted, said
// by name (sc.out, sc.hurt ... sc.intent, sc.due).  Posted at SCENE_AT in CMD_BANK and the hub's scene; in place as the
// minds set scene_d (the UI's line waits for that); then what's to be read after it (m.hearts, the line's snapshot,
// m.dq0's at, taken again).  Every offset the build's (games/dq0/hubgen.js's layout, den.h's records)
let SCN = null;
function sceneLayout() {
  if (SCN) return SCN;
  const H = dq0Hub(), C = H.C, M = require('../../sim/lib/hubown.js').map();
  return (SCN = { H, C, M, world: H.tasks.indexOf('world'), dens: H.tasks.indexOf('denizens'), maker: H.tasks.indexOf('maker'), referee: H.tasks.indexOf('referee'),
    scene: H.off('hub', 'scene'), sceneW: H.off('hub', 'scene_w'), sceneD: H.off('hub', 'scene_d'), task: H.off('hub', 'task'),
    popDen: H.off('pop', 'den'), denSize: H.size('den') });
}
let SCTX = null;                                // (A scene being built: its copies and its ops by name)

// The minds' records (dq0d's: den.h's struct pop at $8000 of its bank pb, the label's), each denizen's by its number (the
// hub's actor's den): { mem, at(i), and its fields' offsets by name, den.h's }; a scene's copy as one's built
let dq0RecPb = 0;
function dq0Rec(m) {
  const S = sceneLayout(), h = m.dq0.bank(0), t = h[S.task + S.dens], lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
  const pb = dq0RecPb || (dq0RecPb = parseInt(/^al ([0-9A-F]+) \._pb$/m.exec(lbl())[1], 16));
  let mem = m.taskBankMem(t, m.taskRam[t][pb]) || new Uint8Array(0x2000);
  if (SCTX) {                                   // (A scene's: its copy, the bytes changed its ops)
    if (!SCTX.rec) SCTX.rec = { live: mem, copy: Uint8Array.from(mem) };
    mem = SCTX.rec.copy;
  }
  const F = f => S.H.off('den', f);
  return { mem, at: i => S.popDen + S.denSize * i, FLAGS: F('flags'), SN: F('sn'), ID: F('id'), X: F('x'), Y: F('y'), ATTITUDE: F('attitude'),
    SERVE: F('serve'), CONTACT: F('contact'), ATE: F('ate'), DRANK: F('drank') };
}

// Every denizen near but actor keep (or those of a list) sent far off, friendly, and not free again for some five minutes: a
// scene's (its call's)
function dq0Alone(m, h, L, K, at, keep) {
  const R = dq0Rec(m), far = 3000 + (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16), ks = [].concat(keep);
  let n = 0;
  for (let k = 0; k < L.nactors; k++) if (!ks.includes(k) && h[at(k) + L.aName] && h[at(k) + L.aState]) {
    const o = R.at(h[at(k) + L.aDen]), x = 32706 + (n & 3), y = 32784 + (n++ >> 2);
    h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8;
    h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
    h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_FRIENDLY;
    h[at(k) + L.aDoing] = K.DO_ROAM;
    for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = far >>> (8 * i) & 255;
  }
  h[L.denSeq] = h[L.denSeq] + 1 & 255;
}

// An intent posted for actor k, as its mind would (a scene's: the minds post it, SO_INTENT): kind (K.I_*), its fields { a,
// b, w1, w2, t, x: [4] } as struct msg lays them out; I_DEN's doing and face (x[0], x[1]) 0xFF unless given.  The one-path
// plan's stage 2, things, 2026-11-23
function dq0Intent(h, L, kind, k, f = {}) {
  if (!SCTX) throw new Error('dq0Intent: only in a scene (dq0Scene\'s call)');
  const x = f.x || [0xFF, 0xFF, 0, 0], b = new Array(L.msgSize).fill(0), w = (o, v, n) => { for (let i = 0; i < n; i++) b[o + i] = v >>> (8 * i) & 255; };
  b[L.mKind] = kind; b[L.mWho] = k + 1; b[L.mA] = f.a || 0; b[L.mB] = f.b || 0;
  w(L.mW1, f.w1 || 0, 2); w(L.mW2, f.w2 || 0, 2); w(L.mT, f.t || 0, 4); x.forEach((v, i) => { b[L.mX + i] = v; });
  SCTX.ops.push([sceneLayout().C.SO_INTENT, ...b]);
}

// A scene built (fn (sc): the test's own code, its writes to the copies; sc its ops by name) and its ops: [[kind, ...payload]]
function sceneBuild(m, fn) {
  const S = sceneLayout(), C = S.C, rbank = m.dq0.bank, rshared = m.sharedBanks, live = rshared(), copies = new Map();
  const made = new Set(), cp = b => { if (!b || !(b instanceof Uint8Array) || made.has(b)) return b; let c = copies.get(b); if (!c) { copies.set(b, c = Uint8Array.from(b)); made.add(c); } return c; };
  const ctx = SCTX = { ops: [], rec: null };
  const rs = (k, what, o = {}) => ctx.ops.push([C.SO_RS, k, what, o.level || 0, o.which || 0, (o.ex || 0) & 255, (o.ex || 0) >> 8 & 255,
    ...[0, 8, 16, 24].map(i => (o.till || 0) >>> i & 255)]);
  // (The referee's ops by name, by the actor's entry, k (0 on; -1 the character): rs.h's SCN_*)
  const sc = {
    rs, out: (k, till) => rs(k + 1, C.SCN_OUT, { till }), well: k => rs(k + 1, C.SCN_WELL), hurt: (k, level) => rs(k + 1, C.SCN_HURT, { level }),
    lame: k => rs(k + 1, C.SCN_LAME), att: (k, which, level) => rs(k + 1, C.SCN_ATT, { which, level }),
    tire: (k, o) => rs(k + 1, C.SCN_TIRE, { level: o.lv, ex: o.ex, till: o.at }),
    due: what => ctx.ops.push([C.SO_DUE, { burn: C.DU_BURN, trace: C.DU_TRACE, hold: C.DU_HOLD }[what]]),
    rig: list => sceneRig(S, m.dq0.bank(0), list),
  };
  m.dq0.bank = i => cp(rbank.call(m.dq0, i));
  // (A task's own main RAM, the world's, the minds' or the referee's (by its label: a test's own reading of the build's): a
  // copy too, its bytes changed each its owner's op, SO_WRAM, SO_DRAM, the referee's RS_SCENE SCN_RAM)
  const rram = m.taskRam, h0 = rbank.call(m.dq0, 0), rams = new Map();
  if (h0) for (const role of [S.world, S.dens, S.referee]) rams.set(h0[S.task + role], role);
  const rcp = new Map();
  m.taskRam = new Proxy(rram, { get: (t, p) => {
    if (typeof p !== 'string' || !/^\d+$/.test(p) || !rams.has(+p) || !t[p]) return t[p];
    if (!rcp.has(+p)) rcp.set(+p, { live: t[p], copy: Uint8Array.from(t[p]), role: rams.get(+p) });
    return rcp.get(+p).copy;
  } });
  m.sharedBanks = () => new Proxy(live, { get: (t, p) => typeof p === 'string' && /^\d+$/.test(p) ? cp(t[p]) : t[p] });
  try { fn(sc); } finally { m.dq0.bank = rbank; m.sharedBanks = rshared; m.taskRam = rram; SCTX = null; }
  // (The bytes changed, in runs of one owner each, 64 bytes at most)
  const hub = new Map(), w = [], d = [];
  for (let i = 0; i < C.HUB_BANKS; i++) { const b = rbank.call(m.dq0, i); if (b) hub.set(b, i); }
  const refRun = (what, o, run) => { for (let i = 0; i < run.length; i += 4) { const x = run.slice(i, i + 4); rs(0, what, { which: x.length, ex: o + i }); ctx.ops[ctx.ops.length - 1].splice(7, x.length, ...x); } };
  const runs = (a, c, route, put) => {
    for (let o = 0; o < a.length;) {
      if (a[o] === c[o]) { o++; continue; }
      const r = route(o), run = [], o0 = o;
      while (o < a.length && a[o] !== c[o] && run.length < 64 && route(o) === r) run.push(c[o++]);
      put(r, o0, run);
    }
  };
  for (const [b, c] of copies) {
    const bi = hub.get(b);
    if (bi === undefined) { if (b.some((v, o) => v !== c[o])) throw new Error('dq0Scene: a bank written that isn\'t the hub\'s'); continue; }
    runs(b, c, o => {
      const g = S.M.regions[S.M.at[bi][o]];
      if (g.allowed & 1 << S.world) return 'w';
      if (g.allowed & 1 << S.dens) return 'd';
      if (g.allowed & 1 << S.maker) return 'm';
      if (bi === 0 && (g.allowed & 1 << S.referee)) return 'r';
      throw new Error('dq0Scene: the hub\'s ' + g.name + ' (bank ' + bi + ', ' + o + ') written, its owner ' + (S.H.tasks[g.owner] || 'none') + ': not the world\'s, the maker\'s nor the minds\'');
    }, (r, o, run) => r === 'r' ? refRun(C.SCN_HUB, o, run) : (r === 'd' ? d : w).push([{ w: C.SO_HUB, d: C.SO_DHUB, m: C.SO_MHUB }[r], bi, o & 255, o >> 8, ...run]));
  }
  for (const { live, copy, role } of rcp.values()) runs(live, copy, () => role, (r, o, run) => r === S.referee ? refRun(C.SCN_RAM, o, run) :
    (r === S.world ? w : d).push([r === S.world ? C.SO_WRAM : C.SO_DRAM, o & 255, o >> 8, ...run]));
  if (ctx.rec) runs(ctx.rec.live, ctx.rec.copy, () => 'r', (r, o, run) => d.push([C.SO_REC, o & 255, o >> 8, ...run]));
  return [...w, ...ctx.ops.filter(o => o[0] === C.SO_RS || o[0] === C.SO_DUE), ...d, ...ctx.ops.filter(o => o[0] === C.SO_INTENT)];
}

// A test's rigged dice (dice2, 2026-11-24: hub.h's struct rig, the referee's, dq0harn's; its scene's SCN_HUB), the table
// written whole on the hub's copy h, its uses counted afresh: each entry { kind (rs.h's RS_ less its RS_: 'TEST',
// 'ATTACK' ...), a (a purpose's name, 'P_DIG', or a number; none any), actor (an actor's entry, -1 the character; none
// any), uses (none every time), faces [the first dice's, four at most; [] the dice as they fall: a request kept from
// the entries after] or grade (HarnMaster's d100 test, its first die: 'well' 5, 'pass' 1, 'fail' 99, 'badly' 100,
// whatever the mastery) }: the first that matches a request has its first dice show its faces, the rest the key's.
// kind 'WORLD': the world's own rolls (dq0w/wdice.c: a its WD_* name, 'WD_SHOT', 'WD_FALL', 'WD_DROP'; no actor, no uses:
// every time), its faces each roll's value plus 1 in turn (a missile's stray: [1] it strays, then [1, k + 1] the one
// beside it struck)
const RIG_FACE = { well: 5, pass: 1, fail: 99, badly: 100 };
function sceneRig(S, h, list) {
  const H = S.H, C = S.C, o = H.off('hub', 'rig'), e0 = o + H.off('rig', 'e'), es = H.size('rig_e'), F = f => H.off('rig_e', f);
  if (list.length > C.RIG_N) throw new Error('sc.rig: ' + list.length + ' entries, ' + C.RIG_N + ' at most');
  for (let i = 0; i < C.RIG_N; i++) {
    const p = e0 + i * es, e = list[i];
    for (let z = 0; z < es; z++) h[p + z] = 0;
    if (!e) continue;
    const world = e.kind === 'WORLD', kind = world ? C.RIG_WORLD : C['RS_' + e.kind],
      a = e.a === undefined ? C.RIG_ANY : typeof e.a === 'string' ? (world ? C[e.a] >> 12 : C[e.a]) : e.a, faces = e.faces || [RIG_FACE[e.grade]];
    if (kind === undefined || a === undefined || faces.length > 4 || faces.some(x => !(x > 0 && x < 256))) throw new Error('sc.rig: ' + JSON.stringify(e));
    h[p + F('kind')] = kind; h[p + F('a')] = a; h[p + F('actor')] = e.actor === undefined ? C.RIG_ANY : e.actor + 1;
    h[p + F('uses')] = e.uses === undefined ? C.RIG_ANY : e.uses;
    faces.forEach((x, j) => { h[p + F('face') + j] = x; });
  }
}

// The machine's scenes: posted one at a time, the next when the last's in place (its after () then)
function sceneOf(m) {
  if (m.dq0Scenes) return m.dq0Scenes;
  const e = m.dq0Scenes = { cur: null, queue: [], lost: [], sent: 0 };
  return e;
}
function sceneDone(m, e) {
  const cur = e.cur;
  if (cur.cr !== undefined) (m.dq0SceneAt || (m.dq0SceneAt = new Map())).set(cur.cr, m.cpu.cyc);   // (Its line's Enter's: dq0Timer's from then)
  const h0 = m.dq0 && m.dq0.bank(0), S0 = sceneLayout();   // (At the turns pace the line waits on past it for the world's quiet
  if (cur.cr !== undefined && h0 && (h0[S0.H.off('hub', 'pace')] & 3) === 3) m.dq0SceneTake = cur.cr;   //   after it, lock2's:
  //   dq0Timer's from the line taken then, the UI's ui_cmd, the world's reaction to the test's setting up its own too)
  if (process.env.DQ0SCENE) console.log('scene ' + cur.seq + ' in place at ' + m.cpu.cyc + ' (' + (m.cpu.cyc - cur.at) + ' after): ' + cur.why);
  e.cur = null;
  if (cur.after) cur.after();
  if (e.queue.length) sceneSend(m, e, e.queue.shift());
}
function sceneSend(m, e, item) {
  const S = sceneLayout(), h = m.dq0.bank(0), cmd = m.dq0.bank(S.C.CMD_BANK);
  if (!h || !cmd) { e.lost.push(item.why + ': no game'); return; }
  let o = S.C.SCENE_AT;
  for (const op of item.ops) {
    if (o + op.length + 1 > S.C.SCENE_AT + S.C.SCENE_SIZE - 2) throw new Error('dq0Scene: ' + item.why + ': its ops past SCENE_SIZE');
    cmd[o] = op[0]; cmd[o + 1] = op.length - 1; cmd.set(op.slice(1), o + 2); o += op.length + 1;
  }
  cmd[o] = cmd[o + 1] = 0;
  const r = m.dq0;                              // (In place as the minds set scene_d: the recorder's hook sees the write, before it's made)
  if (r && r.hook && !r.dq0SceneHook) {
    const h0 = r.hook, k0 = () => { const b = m.sharedBanks(), b0 = m.dq0.bank(0); return Object.keys(b).map(Number).find(k => b[k] === b0); };
    r.hook = (k, off, v) => {
      h0(k, off, v);
      const s = m.dq0Scenes;
      if (process.env.DQ0SCENE && s && s.cur && off === S.sceneW && k === k0()) console.log('scene ' + v + ': the world\'s part at ' + m.cpu.cyc + ' (' + (m.cpu.cyc - s.cur.at) + ' after)');
      if (s && s.cur && off === S.sceneD && v === s.cur.seq && k === k0()) sceneDone(m, s);
      if (m.dq0SceneTake !== undefined && off === S.H.off('hub', 'ui_cmd') && v === 1 && k === k0()) {   // (The line taken, past
        m.dq0SceneAt.set(m.dq0SceneTake, m.cpu.cyc);   //   the scene's quiet: sceneDone's)
        m.dq0SceneTake = undefined;
      }
    };
    r.dq0SceneHook = true;
  }
  item.seq = ((h[S.scene] & 0x7F) + 1 & 0x7F) | (item.cr !== undefined ? S.C.SCENE_LINE : 0);   // (A line's Enter's: the line waits)
  item.at = m.cpu.cyc;
  if (process.env.DQ0SCENE) console.log('scene ' + item.seq + ' posted at ' + item.at + ': ' + item.why + ' ' + JSON.stringify(item.ops));
  e.cur = item;
  e.sent++;
  if (process.env.DQ0SCENE_PROF) require(process.env.DQ0SCENE_PROF)(m, S.task, +process.env.DQ0SCENE_ROLE, process.env.DQ0SCENE_PROG, path.join(TESTS, '..'));   // (Debugging: a role's PCs as it's pending)
  h[S.scene] = item.seq;
}
// A scene posted now: fn (h, L, K, at, named, sc), the test's own setting of it (h the hub's header's copy); after (), what's
// read once it's in place.  why: its name, for a scene lost
function dq0SceneNow(m, fn, after, why = 'a scene', enter = false) {
  const e = sceneOf(m), h = m.dq0 && m.dq0.bank(0);
  if (!h) return;
  const { L, K } = m.dq0.layout(), at = k => L.actor + k * L.actorSize;
  const ops = sceneBuild(m, sc => {
    const c = m.dq0.bank(0), named = k => { let n = ''; for (let i = 0; c[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(c[at(k) + L.aName + i]); return n; };
    fn(c, L, K, at, named, sc);
  });
  if (!ops.length) { if (after) after(); return; }
  // (The minds' own facts first, a scene of their own: an actor's name, its attitude, its record in place before the
  // world's bodies move and its den_seq bids the senses look again, as a poke had them all at once; then the rest, the
  // world's, the maker's, the referee's and the intents posted)
  const C = sceneLayout().C, mind = o => o[0] === C.SO_DHUB || o[0] === C.SO_REC || o[0] === C.SO_DRAM;
  const cr = enter ? m.acia.rxAt : undefined, a = ops.filter(mind), b = ops.filter(o => !mind(o));
  const items = [a.length && b.length ? { ops: a, why: why + ' (the minds\' first)', cr } : null, { ops: b.length ? b : a, after, why, cr }].filter(Boolean);
  for (const item of items) if (e.cur) e.queue.push(item); else sceneSend(m, e, item);
}

// A test's scene set as lines are typed (each p: g its game, 1 on; line, n: its nth time typed in it), the pokes' specs:
// hub { NAME: v } the hub's bytes by the layout's names; fuel, wear { KIND: v }, item { KIND: { clear, or, hide, size,
// quality, qty } } the first thing of that kind in the store; traps { hide, plates }; call (h, L, K, at, named, sc), a
// scene of its own; and actor k (or name, the first so named): set { FIELD: v }, beside (a heading, or 'face'), engage,
// senseless (tenths), hurt (levels), lame, rec { or, serve, contact }; rig [...] (its dice rigged: sceneRig's).  Each line's scene posted at its Enter; then (in
// place) the actors' moods and doings, m.hearts, and the line's own snapshot (m.dq0's) taken again
function dq0Scene(m, pokes) {
  const a = m.acia, read = a.read;
  let typed = '', g = 0, seen = {};
  m.hearts = [];
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx, e = sceneOf(m);
      if (process.env.DQ0SCENE && e.cur && m.dq0 && m.dq0.bank(0)) { const h = m.dq0.bank(0), S = sceneLayout(); console.log('scene bytes at ' + m.cpu.cyc + ': ' + [h[S.scene], h[S.sceneW], h[S.H.off('hub', 'scene_m')], h[S.sceneD]]); }
      if (e.cur && m.dq0 && m.dq0.bank(0) && m.dq0.bank(0)[sceneLayout().sceneD] === e.cur.seq) sceneDone(m, e);   // (In place, unseen: HUBOWN=0's hook)
      if (c === 13) {
        if (/^dq0 /.test(typed)) {
          ++g;
          seen = {};
          if (e.cur || e.queue.length) e.lost.push(...[e.cur, ...e.queue].filter(Boolean).map(x => x.why + ': its game over before it was in place'));
          e.cur = null; e.queue = [];
        }
        const line = m.dq0Orig && m.dq0Orig.length && m.dq0Orig[0].to === typed ? m.dq0Orig[0].from : typed;   // (A played game's
                                                //   line in its own letters, dq0PlayLetters': the test's line, its scene the same)
        seen[line] = (seen[line] || 0) + 1;
        const h = m.dq0 && m.dq0.bank(0);
        if (h && g) {
          const ln = line, gn = g, nn = seen[line], rec = m.dq0, game = rec.games && rec.games[rec.games.length - 1], snapAt = rec.lines ? rec.lines.length + 1 : 0;
          const mine = pokes.filter(p => p.g === gn && p.line === ln && p.n === nn);
          const hearts = () => {
            const h = m.dq0.bank(0), { L } = m.dq0.layout(), at = k => L.actor + k * L.actorSize, acts = [], R = dq0Rec(m);
            for (let k = 0; k < L.nactors; k++) {
              let n = '';
              for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
              if (n) {
                const o = R.at(h[at(k) + L.aDen]);
                acts.push({ k, name: n, heart: (h[at(k) + L.aHeart] << 24) >> 24, lead: h[at(k) + L.aLead], power: h[at(k) + L.aPower], mood: h[at(k) + L.aMood], bonds: h[at(k) + L.aBonds], doing: h[at(k) + L.aDoing],
                  rank: R.mem[o + R.FLAGS] >> 5 & 3, prom: !!(R.mem[o + R.FLAGS] & 0x80) });
              }
            }
            m.hearts.push({ g: gn, line: ln, n: nn, acts });
          };
          const resnap = () => {                // (The line's snapshot as its command finds it: the scene in place)
            if (!game || !rec.final) return;
            const i = game.snaps.findIndex(s => s.line === snapAt);
            if (i >= 0) { const s = rec.final(); if (s) { s.line = snapAt; game.snaps[i] = s; } }
          };
          if (!mine.length) hearts();
          else dq0SceneNow(m, (h, L, K, at, named, sc) => { for (const p of mine) scenePoke(m, p, h, L, K, at, named, sc); }, () => { hearts(); resnap(); },
            'game ' + gn + '\'s "' + ln + '" #' + nn, true);
        }
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}
// A thing's place in the store, the first of its kind's name (the catalogue's): its bank and offset, or null
function sceneThing(m, L, kn) {
  const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);
  for (let i = 0; cat && i < L.nitems; i++) {
    const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
    if (!b || !b[o + L.iKind]) continue;
    let n = '';
    for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
    if (n === kn) return { b, o };
  }
  return null;
}
// One spec of dq0Scene's set, on the scene's copies
function scenePoke(m, p, h, L, K, at, named, sc) {
  const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; }, off = f => L['a' + f[0].toUpperCase() + f.slice(1)];   // (An actor's field by its name)
  for (const [f, v] of Object.entries(p.hub || {})) h[L[f]] = v;   // (The hub's own bytes, by the layout's names: pcSkew)
  for (const [kn, v] of Object.entries(p.fuel || {})) { const t = sceneThing(m, L, kn); if (t) { t.b[t.o + L.iFuel] = v & 255; t.b[t.o + L.iFuel + 1] = v >> 8 & 255; } }
  for (const [kn, v] of Object.entries(p.wear || {})) { const t = sceneThing(m, L, kn); if (t) t.b[t.o + L.iWear] = v; }
  for (const [kn, v] of Object.entries(p.item || {})) {
    const t = sceneThing(m, L, kn);
    if (!t) continue;
    const { b, o } = t;
    b[o + L.iFlags] = b[o + L.iFlags] & ~(v.clear || 0) | (v.or || 0);
    if (v.hide !== undefined) b[o + L.iHide] = v.hide;
    if (v.size !== undefined) b[o + L.iSize] = v.size;
    if (v.quality !== undefined) b[o + L.iQuality] = v.quality;
    if (v.qty !== undefined) b[o + L.iQty] = v.qty;
  }
  if (p.traps) sceneTraps(m, p.traps, h, L, K);
  if (p.call) p.call(h, L, K, at, named, sc);   // (A scene of its own, 2026-11-03)
  if (p.rig) sc.rig(p.rig);                     // (Its dice rigged: sceneRig's, dice2)
  const k = p.name === undefined ? p.k : [...Array(L.nactors).keys()].find(j => named(j) === p.name);   // (By its name: the first)
  if (k === undefined) return;
  for (const [f, v] of Object.entries(p.set || {})) if (f === 'x' || f === 'y') w16(at(k) + off(f), v); else h[at(k) + off(f)] = v;
  if (p.beside !== undefined || p.engage) {     // (Put beside the character, engaged: the hub's and its record's)
    const R = dq0Rec(m), o = R.at(h[at(k) + L.aDen]), r16 = (q, v) => { R.mem[o + q] = v & 255; R.mem[o + q + 1] = v >> 8 & 255; };
    if (p.beside !== undefined) {
      const d = p.beside === 'face' ? h[L.pcFace] : p.beside, x = (h[L.pcX] | h[L.pcX + 1] << 8) + [0, 1, 1, 1, 0, -1, -1, -1][d & 7],
        y = (h[L.pcY] | h[L.pcY + 1] << 8) + [-1, -1, 0, 1, 1, 1, 0, -1][d & 7];
      w16(at(k) + L.aX, x);
      w16(at(k) + L.aY, y);
      h[at(k) + L.aSn] = h[L.pcSn];
      h[at(k) + L.aPos] = K.POS_STANDING;
      r16(R.X, x);
      r16(R.Y, y);
      R.mem[o + R.SN] = h[L.pcSn];
    }
    if (p.engage) {
      h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_HOSTILE;
      h[at(k) + L.aDoing] = K.DO_FIGHT;
    }
    h[L.denSeq] = h[L.denSeq] + 1 & 255;
  }
  if (p.senseless) {                            // (Senseless, for the tenths given, as the referee has it and the world: down, prone, 2026-11-03)
    const now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16 | h[L.now + 3] << 24, till = (now + p.senseless) >>> 0;
    sc.out(k, till);
    for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = till >>> (8 * i) & 255;
    h[at(k) + L.aDoing] = K.DO_DOWN;
    h[at(k) + L.aPos] = K.POS_PRONE;
  }
  if (p.hurt) sc.hurt(k, p.hurt);               // (Hurt, as the referee has it: a minor injury of the levels given, every test of its the worse by five points a level, 2026-11-04)
  if (p.lame) {                                 // (Prone, seriously wounded and Agility 1, as the referee has it: its Rise an Agility test failed, 2026-11-03)
    sc.lame(k);
    h[at(k) + L.aPos] = K.POS_PRONE;
  }
  if (p.rec) {
    const R = dq0Rec(m), o = R.at(h[at(k) + L.aDen]);
    if (p.rec.or) R.mem[o + R.FLAGS] |= p.rec.or;
    if (p.rec.serve !== undefined) R.mem[o + R.SERVE] = p.rec.serve;
    if (p.rec.contact !== undefined) R.mem[o + R.CONTACT] = p.rec.contact;
  }
}
// The cached chunks' hidden traps' hiding set (hide), a scene's, 2026-11-02; and (plates) on the character's own chunk's room's
// stops (a room searched whole's, but its own) a plate of an alarm, hidden, and its doors made secret: on the scene's copies,
// struct chunk's and struct trap's offsets the build's
function sceneTraps(m, T, h, L, K) {
  const H = dq0Hub(), C = H.C, w = m.dq0.bank(C.WORLD_BANK), slot0 = H.off('wrec', 'cslot'), ss = H.size('cslot');
  const [NT, TR, TS, TST, THD, NP, PO, PS, PST] = [H.off('chunk', 'ntraps'), H.off('chunk', 'trap'), H.size('trap'), H.off('trap', 'state'), H.off('trap', 'hide'),
    H.off('chunk', 'nportals'), H.off('chunk', 'portal'), H.size('portal'), H.off('portal', 'state')];
  const ntr = C.NTRAPS, rm = H.off('chunk', 'room'), rs = H.size('room');
  for (let c = 0; c < C.NCSLOTS; c++) {
    const sl = slot0 + c * ss, b = m.dq0.bank(C['CACHE_BANK (0)'] + Math.floor(c / 3));
    if (!w[sl + H.off('cslot', 'used')] || !b) continue;
    const o = (c % 3) * C.CHUNK_SIZE, r16 = q => w[sl + q] | w[sl + q + 1] << 8;
    if (T.hide !== undefined) for (let i = 0; i < b[o + NT] && i < ntr; i++) if (b[o + TR + i * TS + TST] & C.TS_HIDDEN) b[o + TR + i * TS + THD] = T.hide;
    const px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8, st = stratumRec(m, h[L.pcSn]), ox = px - (st ? st.x0 : 0), oy = py - (st ? st.y0 : 0);
    if (T.plates && w[sl + H.off('cslot', 'sn')] === h[L.pcSn] && r16(H.off('cslot', 'cx')) === ox >> 5 && r16(H.off('cslot', 'cy')) === oy >> 5) {
      const lx = ox & 31, ly = oy & 31;
      for (let r = 0; r < b[o + H.off('chunk', 'nrooms')]; r++) {
        const a = o + rm + r * rs, rx = b[a + H.off('room', 'x')], ry = b[a + H.off('room', 'y')], rw = b[a + H.off('room', 'w')], rh = b[a + H.off('room', 'h')];
        if (lx < rx || ly < ry || lx >= rx + rw || ly >= ry + rh) continue;
        for (let j = 0; j < rh; j += 3) for (let i = 0; i < rw; i += 3) {
          const n = b[o + NT], q = o + TR + n * TS, f = x => H.off('trap', x);
          if (n >= ntr || (lx - rx >> 0) - i >= 0 && lx - rx - i < 3 && ly - ry - j >= 0 && ly - ry - j < 3) continue;
          for (let z = 0; z < TS; z++) b[q + z] = 0;
          b[q + f('x')] = rx + i + 1; b[q + f('y')] = ry + j + 1; b[q + f('kind')] = K.TK_ALARM; b[q + f('trigger')] = K.TT_PLATE; b[q + f('state')] = C.TS_HIDDEN;
          b[q + f('hide')] = T.hide || 0; b[q + f('power')] = 1;
          b[o + NT] = n + 1;
        }
      }
      for (let i = 0; i < b[o + NP]; i++) b[o + PO + i * PS + PST] |= C.PS_HIDDEN;
    }
  }
}

// The creatures' bodies in the referee's bank (dq0harn's fb: tire.c's struct bbody, 8 bytes by the sheet's number, a
// denizen's record's: 2026-10-31, every actor's tiring), to read: { mem, at(n) }, its levels at 0, wfl at 1, ex at 2, at
// (the clock) at 4.  (A scene sets one by sc.tire)
let dq0BodyFb = 0;
function dq0Body(m) {
  const S = sceneLayout(), h = m.dq0.bank(0), t = h[S.task + S.referee], lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0harn', 'dq0harn.lbl'), 'latin1');
  const fb = dq0BodyFb || (dq0BodyFb = parseInt(/^al ([0-9A-F]+) \._fb$/m.exec(lbl())[1], 16)), mem = m.taskBankMem(t, m.taskRam[t][fb]) || new Uint8Array(0x2000);
  return { mem, at: n => 8 * n };
}
// The character senseless for the tenths given and come round in shock (a scene's: the referee's, RS_SCENE's SCN_OUT for
// the character, its shock rolls the book's dice, in shock or not as they fall, 2026-11-03).  RULES=null: the null
// referee's out, and the world's pc_out and pc_ready as a blow's knockout sets them (no shock: up as its time's come)
function dq0PcShock(m, tenths) {
  if (!SCTX) throw new Error('dq0PcShock: only in a scene (dq0Scene\'s call)');
  const h = m.dq0.bank(0), { L } = m.dq0.layout(), C = sceneLayout().C;
  const till = ((h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16 | h[L.now + 3] << 24) + tenths) >>> 0;
  SCTX.ops.push([C.SO_RS, 0, C.SCN_OUT, 0, 0, 0, 0, ...[0, 8, 16, 24].map(i => till >>> i & 255)]);
  if (process.env.RULES === 'null') {
    h[L.pcOut] = 1;                             // (And the world's, as a blow's knockout leaves it: wfight.c's, its busy time's end)
    for (let i = 0; i < 4; i++) h[L.pcReady + i] = till >>> (8 * i) & 255;
  }
}

// Each skill's growth in the referee's bank (skills.c's: past the bodies, fb; 2026-11-03), at each line's Enter (m.grow: { g, line,
// n, sk(k): its rolls earned, its routine uses }), after any pokes it's given
function dq0Grow(m) {
  const a = m.acia, read = a.read;
  let typed = '', g = 0, seen = {};
  m.grow = [];
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (/^dq0 /.test(typed)) { ++g; seen = {}; }
        seen[typed] = (seen[typed] || 0) + 1;
        const h = m.dq0 && m.dq0.bank(0);
        if (h && g) {
          const B = dq0Body(m), v = Array.from(B.mem.subarray(0x800, 0x800 + 100));
          m.grow.push({ g, line: typed, n: seen[typed], rolls: k => v[k] >> 4, uses: k => v[k] & 7 });
        }
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}

// A denizen made aware of the character as a line is typed (a test's own setting up: the line's Enter read, the
// actor named its doing DO_WATCH in the hub, as if it had noticed the character; slice 4.5's, an unaware one engages
// no one): the first time the line's typed after the one given before it, if given
function dq0Aware(m, line, actor, after) {
  const a = m.acia, read = a.read;
  let typed = '', seen = !after, done = false;
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (!seen && typed === after) seen = true;
        else if (seen && !done && typed === line && m.dq0) dq0SceneNow(m, (h, L, K, at, named) => {   // (A scene's: the world's, its doing)
          for (let k = 0; k < L.nactors; k++) if (named(k) === actor) { h[at(k) + L.aDoing] = K.DO_WATCH; done = true; }
        }, null, 'dq0Aware\'s "' + line + '"', true);
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}

// A portal's record in the live chunk cache (struct chunk's portal[], 14 bytes each: x, y, kind, state, lock ...): { b
// its bank, o its offset } of stratum sn's square x, y, or null (the scene's pokes: dq0denkey's lock)
function dq0PortalAt(m, sn, x, y) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51), S = stratumRec(m, sn);
  if (hub === undefined || !S || !banks[hub + WORLD_BANK_N]) return null;
  const w = banks[hub + WORLD_BANK_N], cx = (x - S.x0) >> 5, cy = (y - S.y0) >> 5;
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * CSLOT_SIZE, b = banks[hub + CACHE0 + Math.floor(k / 3)];
    if (!w[s] || w[s + 1] !== sn || !b || (w[s + 2] | w[s + 3] << 8) !== cx || (w[s + 4] | w[s + 5] << 8) !== cy) continue;
    const c = (k % 3) * CHUNK_SIZE;
    for (let i = 0; i < b[c + CK_NPORT]; i++) {
      const o = c + CK_PORTAL + i * PORTAL_SIZE;
      if (b[o] === ((x - S.x0) & 31) && b[o + 1] === ((y - S.y0) & 31)) return { b, o };
    }
  }
  return null;
}

function dq0OnWords(m, list) {
  const a = m.acia, read = a.read, shown = a.shown, norm = s => s.replace(/\s+/g, ' ');
  let typed = '', g = 0, seen = {}, armed = null, story = '', esc = 0, escp = '';
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (/^dq0 /.test(typed)) { ++g; seen = {}; armed = null; }
        seen[typed] = (seen[typed] || 0) + 1;
        const p = list.find(q => q.g === g && q.line === typed && q.n === seen[typed]);
        if (p) { armed = p; story = ''; }
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
  a.shown = (v, t) => {
    shown(v, t);
    if (esc === 1) { esc = v === 0x5B ? 2 : 0; escp = ''; return; }
    if (esc === 2) {
      if (v < 0x40 || v > 0x7E) {
        escp += String.fromCharCode(v);
        return;
      }
      esc = 0;
      if (v === 0x72 && !escp) armed = null;    // (ESC [ r: the game's end)
      return;
    }
    if (v === 0x1B) { esc = 1; return; }
    if (!armed) return;
    story = norm(story + (v < 0x20 ? ' ' : String.fromCharCode(v))).slice(-200);
    if (story.includes(norm(armed.words))) {
      if (armed.call) armed.call();             // (A scene's own at those words, not a line typed, 2026-11-03)
      else {
        if (armed.drop && (typed || (a.rxQueue.length && a.rxQueue[0] >= ' ' && a.rxQueue[0] < '\x7f'))) {   // (drop: a line being typed, or due, as the words came, the rest of it let go
          const k = a.rxQueue.indexOf('\r');    //   and what's typed of it cleared, Ctrl-U, before the text: not typed to
          a.rxQueue.splice(0, k + 1);           //   rc after the game's end, cmd1's)
          a.rxQueue.unshift('\x15');
          typed = '';
        }
        a.rxQueue.unshift(...(armed.drop && a.rxQueue[0] === '\x15' ? [a.rxQueue.shift(), ...armed.text] : armed.text));
        a.promptFrom = -1;
      }
      armed = null;
    }
  };
}

// Dingus Quest Zero's answers timed (dq0answer's, and the D17 targets' tests'): each command's Enter (the CR as the ACIA
// has it) to its answer's first letter in the story (want: the words it begins with), to the story's last letter before
// the prompt ("> " on the line typed's row) comes back, and the longest pause between two of its letters, from its first
// to the last of its own (before the prompt, and before any news of the world told after its first letter, NEWS: a
// stir, a sound, the denizens' doings told as they come, their wait the game's time and not the answer's); the status
// line (reverse video) and the escapes not the story's.  dq0Timer(m), as the machine's made, listens (each byte's
// cycle; each CR's, and whether the character was busy as it came: the UI's busy, a line typed then held till it's
// free, its answer the game's time), and gives a function of the lines typed and of cmds ({ line, want, news }: each the
// line's first time typed after the one before) giving { line, first, whole, pause, pauseAt, busy } for each (null: none).
// Its .game (n, cmds): the same of the n'th game typed (its dq0 line, 0 on), each command by its words as it was typed
// after the one before, to the game's end (the lines typed between them passed over: a soft question's answer, dq0Answer's)
const NEWS = /Something stirs|Nothing stirs in sight|You hear (?!nothing)/;
let busyAt = null;
function dq0Timer(m) {
  const at = [], enters = [];
  let out = m.out, typed = '';
  for (let i = 0; i < out.length; i++) at.push(0);
  Object.defineProperty(m, 'out', { get: () => out, set: v => { const c = m.cpu.cyc; for (let i = out.length; i < v.length; i++) at.push(c); out = v; } });
  const a = m.acia, read = a.read;
  const busy = () => {                          // (The UI's busy as a CR comes, the hub's T_UI its task; null with no game)
    try {
      if (busyAt === null) busyAt = parseInt(/^al ([0-9A-F]+) \._busy$/m.exec(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0', 'dq0.lbl'), 'latin1'))[1], 16);
      const h = m.dq0 && m.dq0.bank(0), ui = h ? h[HO('hub', 'task')] : 0;
      return ui && m.taskRam[ui] ? m.taskRam[ui][busyAt] : null;
    } catch (e) { return null; }
  };
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {                    // (Each CR's, and the words typed before it)
      const c = a.rx;
      if (c === 13) {                           // (A line typed in its own game's letters, dq0PlayLetters': the test's line)
        const o = m.dq0Orig && m.dq0Orig.length && m.dq0Orig[0].to === typed ? m.dq0Orig.shift().from : null;
        enters.push({ t: a.rxAt, pos: out.length, busy: busy(), line: o || typed, typed });
        typed = '';
      }
      else if (c === 0x15) typed = '';
      else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
  const none = c => ({ line: c.line, first: null, whole: null, pause: null, pauseAt: '', busy: null });
  const timer = (lines, cmds) => {
    const res = [];
    let li = 0;
    for (const c of cmds) {
      while (li < lines.length && lines[li] !== c.line) li++;
      const e = enters[li], e2 = enters[li + 1];
      li++;
      res.push(e ? measure(c, e, e2) : none(c));
    }
    return res;
  };
  const game = l => /^(?:\S*\/)?dq0( |$)/.test(l);
  timer.enters = enters;                        // (Each CR's: { t, pos, busy, line })
  timer.at = i => measure({ line: enters[i].line, want: '*' }, enters[i], enters[i + 1]);   // (The ith CR's line timed, its answer's own words: sim/tools/lineprof.js's)
  timer.game = (n, cmds) => {
    const res = [];
    let i = -1;
    for (let k = 0; k <= n; k++) for (i++; i < enters.length && !game(enters[i].line); i++);
    for (const c of cmds) {
      let j = i + 1;
      while (j < enters.length && enters[j].line !== c.line && !game(enters[j].line)) j++;
      if (j >= enters.length || enters[j].line !== c.line) { res.push(none(c)); continue; }
      res.push(measure(c, enters[j], enters[j + 1]));
      i = j;
    }
    return res;
  };
  return timer;
  function measure(c, e, e2) {
    const end = e2 ? e2.pos : out.length;
    let story = '', idx = [], rev = 0, prompt = -1;
    for (let i = e.pos; i < end; i++) {
      if (out[i] === '\x1b') {
        const q = /^\x1b\[([0-9;?]*)([A-Za-z])/.exec(out.slice(i, i + 12));
        if (q) {
          if (q[2] === 'm') rev = q[1] === '7';
          if (q[2] === 'H' && /;1$/.test(q[1]) && q[1] !== '1;1' && /^\x1b\[K(\x1b\[\?25h)?> /.test(out.slice(i + q[0].length, i + q[0].length + 12)) &&
            (c.want === '*' ? anyWords(story, e.typed || c.line, story.length) >= 0 : c.want.split('|').some(x => story.includes(x))) && prompt < 0) prompt = idx.length;
          i += q[0].length - 1;
          continue;
        }
      }
      if (rev) continue;
      if (out[i] === '\n' && story && story[story.length - 1] !== ' ') {   // (A line's end a space: a news sentence and
        story += ' ';                                                      //   the answer's own after it on the next line
        idx.push(i);                                                       //   told apart, "left.\nYou lift", 2026-11-18)
        continue;
      }
      if (out[i] < ' ') continue;
      story += out[i];
      idx.push(i);
    }
    const w = c.want === '*' ? anyWords(story, e.typed || c.line, prompt < 0 ? story.length : prompt) :
      c.want.split('|').map(x => story.indexOf(x)).filter(x => x >= 0).reduce((a, b) => a < 0 || b < a ? b : a, -1);   // (Words the answer may begin with, | between)
    let last = prompt < 0 ? idx.length - 1 : prompt - 1;
    while (last > 0 && story[last] === ' ') last--;
    let own = last, pause = null, pauseAt = '', pauseFrom = null;  // (Its own words: up to the world's news told after its first letter)
    if (w >= 0) {
      const k = story.slice(w + 1, last + 1).search(c.news || NEWS);
      if (k >= 0) for (own = w + k; own > w && story[own] === ' '; own--);
      for (let j = w + 1, p = -1; j <= own; j++) {
        const g = at[idx[j]] - at[idx[j - 1]];
        if (g > p) { p = pause = g; pauseFrom = at[idx[j - 1]]; pauseAt = story.slice(Math.max(w, j - 24), j) + ' | ' + story.slice(j, j + 24); }
      }
      if (pause === null && own >= w) pause = 0;
    }
    const t = Math.max(e.t, (m.dq0SceneAt && m.dq0SceneAt.get(e.t)) || 0);   // (From the scene in place, a test's set as it was typed:
                                                //   the UI's line waits for it, the test's own setting up, not the game's work;
                                                //   at the turns pace from the line taken at the world's quiet after it, its
                                                //   reaction to the setting up the setting up's, sceneDone's: lock2's)
    return { line: c.line, first: w < 0 ? null : at[idx[w]] - t, whole: w < 0 || last < w ? null : at[idx[last]] - t, pause, pauseAt, busy: e.busy, pos: e.pos,
      pauseFrom: pauseFrom === null ? null : pauseFrom - t };   // (Where its longest pause begins: lineprof.js's --pause)
  }
}

// (dq0Timer's want '*': the answer's own words, whatever they are, 2026-11-17: the first letter in story s, before lim,
// after the line's own echo ("> " and line) and past the world's news told before them, a stir's, a sound's (NEWS, and
// what a stir's telling lists: "(A) A giant rat ...", "... is gone from sight."); -1 none)
function anyWords(s, line, lim) {
  let j = s.indexOf('> ' + line);
  j = j >= 0 && j < 8 ? j + line.length + 2 : 0;
  for (let stir = 0; ; ) {                       // (A stir's list after its news: news too; one first, the answer's.
    while (j < lim && s[j] === ' ') j++;          //   After a sound's words no list: "You hear .... (E) a body,
    if (j >= lim) return -1;                      //   ahead" around's own, speed11's, 2026-11-22)
    const rest = s.slice(j, lim), sound = /^You hear (?!nothing)/.test(rest);
    if (!sound && !/^(?:Something stirs|Nothing stirs in sight)/.test(rest) && (!stir || !/^(?:\(|[^.!?]* gone from sight[.!?])/.test(rest))) return j;
    stir = !sound;
    const e = /[.!?](?= |$)/.exec(rest);
    if (!e) return -1;
    j += e.index + 1;
  }
}

// The game as played (the user's, 2026-11-17: the timing tests once without the test switches, where each step of a
// reach costs the world its own time): its dq0 line typed after a wait (DQ0_PLAYED before it), so the runner's switches
// aren't on it (sim/test.js's -a -I -J -N -K -F -E go on a dq0 line typed straight at its prompt); each soft question it
// asks answered yes as it's asked, as the player would (dq0Asks), the test's next line after it.  And dq0Paces: a D17
// test's game typed again at the menacing pace, and as played (its pace the normal), each its lines and a quit
const DQ0_PLAYED = 'Ā';
const dq0Paces = (game, lines) => [game.replace(/ -p [a-z]/, ' -p m'), ...lines, 'quit', DQ0_PLAYED + game.replace(/ -p [a-z]/, ' -p n'), ...lines, 'quit'];
const dq0Own = out => { const i = out.indexOf('dq0 -p m'); return i < 0 ? out : out.slice(0, i); };   // (The test's own games' output: dq0Paces' after it)
// The soft questions ("Keep walking?", "You're tiring.  Keep digging?", "... would you rather draw your sword?") of the
// games gs (their numbers, as the dq0 lines are typed: 1 on) answered "yes" as they're asked, before the test's next line
function dq0Asks(m, gs) {
  const a = m.acia, read = a.read, shown = a.shown;
  let typed = '', g = 0, tail = '', esc = 0;
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) { if (/^(?:\S*\/)?dq0( |$)/.test(typed)) ++g; typed = ''; }
      else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
  a.shown = (v, t) => {
    shown(v, t);
    if (esc === 1) { esc = v === 0x5B ? 2 : 0; return; }
    if (esc === 2) { if (v >= 0x40 && v <= 0x7E) esc = 0; return; }
    if (v === 0x1B) { esc = 1; return; }
    if (!gs.includes(g)) return;
    tail = (tail + (v < 0x20 ? ' ' : String.fromCharCode(v))).slice(-48);
    if (/(?:Keep [a-z ]+|would you rather [a-z ]+)\?$/.test(tail)) {
      a.rxQueue.unshift(...'yes\r');            // (Its answer, before the test's next line, that waiting for a prompt again)
      a.promptFrom = -1;
      tail = '';
    }
  };
}
// A player's pause between the lines typed mid-fight (the user's T3, 2026-11-22: "the band of lines typed mid-fight is
// judged with a player's pause between lines, not back to back"; speed11's).  A player reads an answer before typing
// the next line: PLAYER_PAUSE, a second at the board's 3.58 MHz, a quick player's (reading a line of the fight's words
// and starting to type; the keys after the first as fast as ever).  Only when the next line is typed changes: every
// line is timed from its Enter as before, to the same targets.  dq0FightPause(m): (1) a line typed at its prompt while
// the character's in a fight (its long task fighting on, LT_FIGHT, or an actor fighting, DO_FIGHT, as the prompt comes)
// has its first key PLAYER_PAUSE after the prompt (acia.js's linePause), not at once; (2) .ready(cyc), for a test's
// lines typed mid-round with no prompt waited for (dq0fightmid's MID_MARK): true once the line before has been answered
// (the prompt drawn on its row again after its Enter) and PLAYER_PAUSE has passed since.  Installed in the D17 tests
// that type lines mid-fight: dq0fightcmd, dq0fightmid, dq0felled, dq0letdens
const PLAYER_PAUSE = 3579545;
function dq0FightPause(m) {
  const a = m.acia, read = a.read, shown = a.shown;
  let enter = -1, prompt = -1;
  const fighting = () => {
    try {
      const h = m.dq0 && m.dq0.bank(0);
      if (!h) return false;
      const { L, K } = m.dq0.layout();
      if (h[L.pcTask + L.ltKind] === K.LT_FIGHT) return true;
      for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aDoing] === K.DO_FIGHT) return true;
      return false;
    } catch (e) { return false; }
  };
  a.linePause = () => fighting() ? PLAYER_PAUSE : 0;
  a.read = (r, t) => {
    if (r === 0 && a.rdrf && a.rx === 13) { enter = t === undefined ? m.cpu.cyc : t; prompt = -1; }
    return read(r, t);
  };
  let raw = '';                                 // (The prompt drawn on its row, dq0Timer's: ESC[K, perhaps the cursor
  a.shown = (v, t) => {                         //   shown, then "> ", or two spaces while the character's busy,
    shown(v, t);                                //   fighting on; the first after the line's Enter its answer's end)
    raw = (raw + String.fromCharCode(v)).slice(-8);
    if (prompt < 0 && enter >= 0 && /(\x1b\[K|\x1b\[\?25h)(> |  )$/.test(raw)) prompt = t === undefined ? m.cpu.cyc : t;
  };
  const ready = cyc => enter < 0 || (prompt >= 0 && cyc - prompt >= PLAYER_PAUSE);
  return { ready, fighting };
}
// Each line typed only once the character's free (the user's D17: a line typed while the character is busy, fighting
// or walking, fails the test; 2026-11-20, the coordinator's, dq0letdens' played game): a line come to its prompt held
// while the UI's busy (dq0Timer's busy) or the character's on a long task (fighting on, walking: the hub's pc_task,
// pc_going), its first key sent as it's free; a soft question's answer (dq0Asks', put
// before it) not held.  Installed after dq0Asks and dq0PlayLetters (the line as it's to be typed, its letters the game's)
function dq0WhenFree(m) {
  const a = m.acia, tick = a.tick, idleTo = a.idleTo;
  let held = 0;
  const busy = () => {
    try {
      if (busyAt === null) busyAt = parseInt(/^al ([0-9A-F]+) ._busy$/m.exec(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0', 'dq0.lbl'), 'latin1'))[1], 16);
      const h = m.dq0 && m.dq0.bank(0), ui = h ? h[HO('hub', 'task')] : 0;
      if (!h) return 0;
      const { L } = m.dq0.layout();             // (Fighting on, walking, any long task: the hub's pc_task, pc_going)
      return (ui && m.taskRam[ui] ? m.taskRam[ui][busyAt] : 0) || h[L.pcTask] || h[L.pcGoing];
    } catch (e) { return 0; }
  };
  a.tick = (d, t) => {
    held = a.rxQueue[0] === 'ā' && busy() ? 1 : 0;
    if (!held) return tick(d, t);
    const q = a.rxQueue;                        // (Its prompt not taken yet: the line kept back, the rest of the tick
    a.rxQueue = [];                             //   done; a soft question's answer put in meanwhile, before it)
    tick(d, t);
    a.rxQueue = a.rxQueue.concat(q);
  };
  a.idleTo = t => held ? Math.min(idleTo(t), t + 50000) : idleTo(t);   // (Looked at again, as it's held, every 50,000 cycles)
}

// The designators' letters in the game as played (the user's, 2026-11-19: the designator tests' played games type that
// game's own letters, so each plays the same commands as the others): its kit isn't the tests' (no -K), so a letter of
// the test's own game names another thing there, or none.  As each line of the test's own game (own: its number, the dq0
// lines typed, 1 on) is entered, each letter in it (a word of one letter, a count before it, "drop 1g"; not the new one of
// "letter h as t") is noted as the thing it names then: a carried thing's kind's name, where it is (held, worn, at the
// belt, or in what) and its state (lit, spent), its place among those alike (the UI bank's ix, in the store's order); a
// capital's thing's or denizen's kind and its place among those alike (the UI bank's cap).  As the same line comes to be
// typed in the game as played (played: its number), at its prompt, each letter is the one that game gives the same thing
// (alike as near as it can: its kind and place, then its kind), kept if there's none.  m.dq0Orig: the lines so changed,
// for dq0Timer (its line the test's, typed as its own)
function dq0LetterKeys(m) {
  if (!m.dq0) return null;
  const { L } = m.dq0.layout(), ui = m.dq0.bank(L.uiBank), cat = m.dq0.bank(L.catBank);
  if (!ui) return null;
  const str = (b, o, n) => { let t = ''; for (let i = 0; i < n && b[o + i]; i++) t += String.fromCharCode(b[o + i]); return t; };
  const ltr = (i, base) => i < 26 ? String.fromCharCode(base + i) : String.fromCharCode(base) + String.fromCharCode(base + i - 26);
  const kname = k => k === 0xFF ? '?' : str(ui, L.uIxks + k * L.cixkSize + L.ckName, 14);
  const ent = [];
  for (let i = 0; i < Math.min(ui[L.uIxn], 40); i++) {
    const o = L.uIx + i * L.cixSize;
    ent.push({ kind: ui[o + L.cxKind], up: ui[o + L.cxUp], flags: ui[o + L.cxFlags], let: ui[o + L.cxLet] });
  }
  const low = {}, cap = {}, seen = {};
  for (const e of ent) {
    if (!e.let) continue;
    const name = kname(e.kind), at = e.up && ent[e.up - 1] ? 'in ' + kname(ent[e.up - 1].kind) : e.flags & 1 ? 'held' : e.flags & 2 ? 'worn' : 'top';
    const k = name + '|' + at + '|' + (e.flags & 0x0C);
    low[ltr(e.let - 1, 97)] = { name, at, st: e.flags & 0x0C, rank: seen[k] = (seen[k] === undefined ? 0 : seen[k] + 1) };
  }
  for (let i = 0; i < L.nlet; i++) {
    const o = L.uCap + i * L.ucapSize, id = ui[o + L.ucId] | ui[o + L.ucId + 1] << 8;
    if (!id) continue;
    const what = ui[o + L.ucWhat], kind = ui[o + L.ucKind], name = what === 2 ? 'den ' + kind : cat ? str(cat, kind * L.kindSize, 14) : 'kind ' + kind;
    cap[ltr(i, 65)] = { name, at: '', st: 0, rank: seen['C' + name] = (seen['C' + name] === undefined ? 0 : seen['C' + name] + 1) };
  }
  return { low, cap };
}
function dq0PlayLetters(m, own, played) {
  const a = m.acia, read = a.read, tick = a.tick, ownLines = [];
  let typed = '', g = 0, n = 0;
  const tokens = l => l.split(' ').map((w, i, ws) => ({ w, i, m: /^(\d*)([A-Za-z])$/.exec(w), as: i > 0 && ws[i - 1] === 'as' && ws[0] === 'letter' }));
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (/^(?:\S*\/)?dq0( |$)/.test(typed)) { ++g; n = 0; }
        else if (g === own) {                   // (The test's own game: its letters' things as the line's entered)
          const K = dq0LetterKeys(m), keys = {};
          for (const tk of tokens(typed)) if (tk.m && !tk.as && K) keys[tk.i] = (tk.m[2] < 'a' ? K.cap : K.low)[tk.m[2]] || null;
          ownLines.push({ line: typed, keys });
        }
        typed = '';
      } else if (c === 0x15) typed = '';
      else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
  a.tick = (d, t) => {
    const head = a.rxQueue[0];
    tick(d, t);
    if (head !== '\u0101' || a.rxQueue[0] === '\u0101' || g !== played) return;
    let e = a.rxQueue.indexOf('\r');           // (Its prompt come: the line about to be typed)
    if (e < 0) return;
    const line = a.rxQueue.slice(0, e).join(''), o = ownLines[n++];
    if (!o || o.line !== line) return;
    const K = dq0LetterKeys(m);
    if (!K) return;
    const ws = line.split(' ');
    for (const tk of tokens(line)) {
      const want = o.keys[tk.i];
      if (!tk.m || tk.as || !want) continue;
      const here = tk.m[2] < 'a' ? K.cap : K.low, all = Object.entries(here);
      const pick = f => { const c = all.filter(([, v]) => f(v)); return c.length ? c[Math.min(want.rank, c.length - 1)][0] : null; };
      const to = pick(v => v.name === want.name && v.at === want.at && v.st === want.st) || pick(v => v.name === want.name && v.at === want.at) ||
        pick(v => v.name === want.name && v.st === want.st) || pick(v => v.name === want.name);
      if (to) ws[tk.i] = tk.m[1] + to;
    }
    const to = ws.join(' ');
    if (to === line) return;
    a.rxQueue.splice(0, e, ...to);
    (m.dq0Orig = m.dq0Orig || []).push({ from: line, to });
  };
}

// Each line of a D17 test's typings timed (dq0Timer's game: the test's own first, then dq0Paces' menacing and played),
// its first letter held to D17.first (tests/lib/trends.js's target: f the failures, notes; their names the test's own
// first, the others' ", menacing" and ", played" after the line); the first typing's words its want, the others' '*'
// (their answers their own: the kit, the hour, the questions of a game without the switches).  gs the three games'
// places among those typed (0 on)
function dq0HeldPaces(t, f, notes, timer, gs, cmds, after) {
  [['', cmds], [', menacing', null], [', played', null]].forEach(([sfx, cs], i) => {
    for (const r of timer.game(gs[i], cs || cmds.map(c => ({ line: c.line, want: '*' })))) {
      if (after && after(r, i)) {     // (Its Enter after the game's end began, the test's own: not a
        notes.push(r.line + sfx + ': typed after the game\'s end began, not held: first letter ' + r.first);   //   line at its prompt, cmd1's)
        continue;
      }
      notes.push(r.line + sfx + ': first letter ' + r.first + (r.busy ? ' (typed as it was busy)' : ''));
      if (target(t, r.line + sfx + ': first letter', r.first, D17.first)) f.push(r.line + sfx + ': its first letter ' + r.first + ' cycles from Enter (the user\'s 720,000)');
    }
  });
}

// The median of a measure over a test's typings (dq0answer's, dq0lookfar's: a null, none, counted the highest), and
// their worst
const dq0Median = vs => { const b = vs.map(v => v === null || v === undefined ? Infinity : v).sort((x, y) => x - y), v = b[(b.length - 1) >> 1]; return v === Infinity ? null : v; };
const dq0Worst = vs => vs.some(v => v === null || v === undefined) ? null : Math.max(...vs);

// The waits of 2M cycles before each line of a typing (dq0answer's and dq0lookfar's), its pattern k (0-2) and DQ0ANS_WAIT
// turning them on (the lead's tier's other phases): 0-2 before a command, j its place in the game; none before a dq0
// line itself (the runner's switches, -a and the rest, go on a dq0 line typed straight at its prompt: a soft question
// asked of a game without -a was answered by the next line, its first words "You stop walking.", 2026-11-17)
const dq0Waits = (k, j) => (k * (j + 1) + +(process.env.DQ0ANS_WAIT || 0)) % 3;

// A measure held to a D17 target over a test's typings (dq0answer's, dq0lookfar's): its median held (tests/lib/trends.js's
// target: f its failures), each typing's said in notes, the worst said when it's over the target and the median isn't,
// the median a trend (max 0: a trend only; undefined: said only, dq0answer96's at 9,600)
function dq0Held(t, f, notes, id, what, vs, max) {
  const K = v => v === null ? 'none' : Math.round(v).toLocaleString('en-US'), v = dq0Median(vs), w = dq0Worst(vs), many = vs.length > 1;
  notes.push(id + ': ' + what + ' ' + vs.map(K).join(', ') + (many ? ' (median ' + K(v) + ')' : ''));
  if (max === undefined) return;
  if (max && target(t, id + ': ' + what, v, max)) f.push(id + ': ' + what + ' ' + K(v) + ' cycles' + (many ? ', the median of ' + vs.map(K).join(', ') : '') + ' (the user\'s ' + K(max) + ')');
  else if (max && !(w <= max)) notes.push(id + ': ' + what + ': its worst typing ' + K(w) + ', over the user\'s ' + K(max) + ' (its median held)');
  const n = v === null ? null : trend(t, id + ': ' + what, v);
  if (n) notes.push(n);
}

// dq0answer's commands: each a line, the words its answer begins with, and its readings' marks: 'read' (a look, what's
// carried, a thing examined or assessed: its words' longest pause held to D17.pause), 'whole' (its whole a trend)
const DQ0_ANSWERS = [['turn left', 'You turn'], ['turn right', 'You turn'],
  ['step forward', 'You step forward'], ['inventory', 'You carry', 'read'], ['examine the torch', 'You look at the torch|You look for the torch', 'read'], ['walk to the door ahead to my left', 'You head for'], ['examine the door', 'You head for|A door|Which door'],
  ['open the door', 'You try the door|You turn|Which door'], ['close the door', 'You reach for the door|You turn|Which door'], ['listen', 'You listen'], ['bar the door', 'You look at the door|You turn|Which door'],
  ['look', 'You', 'read whole'], ['look ahead', 'You peer ahead|You turn', 'read'], ['go up', 'There\'s no way up here.'], ['go to the far side', 'You'], ['walk to the window', 'You head for'], ['take the bones', 'You look for the bones|You go over to the bones.'], ['look', 'You', 'read'],
  ['take the sack from the backpack', 'You reach for your backpack|You look for'], ['put the whetstone in the sack', 'You reach|You look for the whetstone'], ['take the whetstone from the sack', 'You reach|You look for'],
  ['drop the sack', 'You put down the|You reach|You look for'], ['take the sack', 'You reach for the sack|You look for the sack'], ['assess the sack', 'You look at the sack|You look for the sack', 'read'], ['sit', 'You sit down'], ['stand', 'You get to your feet'], ['wait 1 minute', 'You wait a minute'], ['dig down', 'You look at the floor']];   // (The first words as they begin, 2026-11-04: dq0firstword's; a walk over told as it begins, 2026-11-02: a door examined, beside it or out of reach; a thing walked to and taken.  "You stop", a line taken for a soft question's answer, gone: no question's asked with -a, 2026-11-17)

// dq0answer: the user's D17 targets (rethought 2026-11-17, games/dq0/built/2026-11-17-d17.md; tests/lib/trends.js), at
// seed 3 (-T: by the way down), each pace's game typed three times (the turns pace's, then the normal pace's), each line
// after 0-2 waits of 2M cycles by its typing's pattern (dq0Waits): where the background's work falls against an Enter
// moves an answer by 10-20% at the normal pace (the denizens' minds, the senses, the world's own work) and by some 5%
// at the turns pace (a sit 613,000-731,000 over eight phases), so each measure is the median of the three, the answer as
// it's typically had (the worst said in the notes).  Timed at 115,200 baud from Enter: every command's first letter held
// to D17.first (a look's too, as any command's: its first words come at once, "You peer ahead."); a reading's longest
// pause to D17.pause; each measure a trend besides (a rise of more than 5% said, not failed).  Each line typed only once
// the character's free (the prompt, the UI not busy: one typed as it's busy is held till it's free, its first letter
// the game's time), else the test fails.  And (the user's, 2026-11-17) typed once more at the menacing pace, and once
// as the game is played (the normal pace's, without the runner's switches: DQ0_PLAYED, its soft questions answered as
// they're asked, dq0Asks), each held to the same targets, its own measures ("menacing turn left", "played turn left").
// At 9,600 (dq0answer96) one typing a pace, its times said, no target held (the user's targets are 115,200's)
function dq0AnswerTest(baud) {
  const fast = baud === 115200, N = fast ? 3 : 1, game = p => 'dq0 -p ' + p + ' -s 3 -c /ram/c.hm -T';
  const plays = [...[['t', 'turns'], ['n', 'normal']].flatMap(([p, pace]) => [...Array(N).keys()].map(k => ({ g: game(p), pace, k }))),
    ...(fast ? [{ g: game('m'), pace: 'menacing', k: 0 }, { g: DQ0_PLAYED + game('n'), pace: 'played', k: 0 }] : [])];
  const pre = ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', ...(fast ? ['echo b115200 >/dev/serctl'] : [])];
  const typed = [...pre.map(l => [l, 0]), ...plays.flatMap(p => [[p.g, 0], ...DQ0_ANSWERS.map((c, j) => [c[0], dq0Waits(p.k, j)]), ['quit', 0]]), ['echo $status', 0]];
  return {
    name: fast ? 'dq0answer' : 'dq0answer96', what: 'Dingus Quest Zero\'s answers timed at ' + baud + ' baud (the user\'s D17, rethought 2026-11-17: a command\'s first letter within a fifth of a second, 720,000 cycles; a reading\'s words with no pause longer than half a second, 1.79M): from Enter to the answer\'s first letter, its longest pause and its last, for 28 commands at seed 3 (turns, a step, what the character carries, things examined, doors, a look about and ahead, a refused step, walks, the pack, sitting, waiting, digging), at the turns pace and the normal pace, each pace\'s game typed three times at different moments, the median of the three held' + (fast ? '; and once at the menacing pace, and once as the game is played (the normal pace without the tests\' switches, its soft questions answered), each held' : ''),
    meaning: true, outfit: true,                  // (The game as it's played: its outfit drawn, 2026-10-27)
    init: 't_rc', cycles: fast ? 7000e6 : 4000e6,
    machine: { input: typed.map(([l, w]) => '\u0101' + '\u0100'.repeat(w) + l + '\r').join('') },
    expect: ['% echo $status'],
    start(m) { this.timer = dq0Timer(m); dq0Asks(m, plays.map((p, i) => p.pace === 'played' ? i + 1 : 0).filter(Boolean)); },
    check(m, out) {
    const f = [], notes = this.notes = [];
    // (Each typing's answers, by its game's place among those typed; then each command's measures over its pace's typings)
    const runs = plays.map((p, i) => this.timer.game(i, DQ0_ANSWERS.map(c => ({ line: c[0], want: c[1] }))));
    const T = fast ? D17 : {};                 // (At 9,600 its times said, no target)
    for (const pace of ['turns', 'normal', 'menacing', 'played']) {
      const rs = runs.filter((r, i) => plays[i].pace === pace), seen = {};
      if (!rs.length) continue;
      DQ0_ANSWERS.forEach((c, k) => {
        const nth = seen[c[0]] = (seen[c[0]] || 0) + 1, id = pace + ' ' + c[0] + (nth > 1 ? ' #' + nth : ''), r = rs.map(x => x[k]);
        r.forEach((x, n) => { if (x.busy) f.push(id + ': typed in typing ' + (n + 1) + ' while the character was busy (its answer the game\'s time, not the command\'s)'); });
        dq0Held(this, f, notes, id, 'first letter', r.map(x => x.first), T.first);
        if (/read/.test(c[2] || '')) {
          dq0Held(this, f, notes, id, 'longest pause', r.map(x => x.pause), T.pause);
          notes.push(id + ': its longest pause where ' + r.map(x => JSON.stringify(x.pauseAt)).join(', '));
        }
        if (/whole/.test(c[2] || '')) dq0Held(this, f, notes, id, 'whole', r.map(x => x.whole), baud === 115200 ? 0 : undefined);
      });
    }
    return f;
    },
    sdcard: true
  };
}

const WORN = ['inventory', 'wear the ring on my left hand', 'wear the ring on my left hand', 'put on the ring on my left hand', 'don the ring on my left hand',
  'wear the ring on my left hand', 'wear the ring on my neck', 'wear the ring', 'inventory', 'put the scarf on my head', 'put the necklace around my neck', 'wear the earrings',
  'examine the scarf', 'inventory', 'take off the ring', 'right', 'inventory', 'take off the scarf', 'wear the scarf over my face', 'inventory'];   // (dq0worn's lines)

// The minds' places by name (dq0d.lbl's text): a label's (._NAME), else a field of the minds' one record per denizen
// (games/dq0d/dq0d.c's struct mind md, 2026-11-19: by field, each an array of NACTORS; its fields' order and sizes
// read from the source), the address of its first element
let dq0MindOff = null;
function dq0Ad(lbl) {
  const at = n => { const m = new RegExp('^al ([0-9A-F]+) \\._' + n + '$', 'm').exec(lbl); return m ? parseInt(m[1], 16) : null; };
  return n => {
    const v = at(n);
    if (v !== null) return v;
    if (!dq0MindOff) {
      const c = fs.readFileSync(path.join(__dirname, '..', '..', 'games', 'dq0d', 'dq0d.c'), 'latin1');
      const body = c.slice(c.indexOf('struct mind {'), c.indexOf('static struct mind md;')).replace(/\/\*[^]*?\*\//g, '');
      const N = dq0Hub().value('NACTORS');
      dq0MindOff = {};
      let o = 0;
      for (const m of body.matchAll(/^\s*unsigned( char| long)?\s+([^;]+);/gm)) {
        const size = m[1] === ' char' ? 1 : m[1] === ' long' ? 4 : 2;
        for (const f of m[2].split(',')) { dq0MindOff[f.trim().replace('[NACTORS]', '')] = o; o += size * N; }
      }
    }
    if (dq0MindOff[n] === undefined) throw new Error('dq0Ad: no ' + n + ' in dq0d.lbl or struct mind');
    return at('md') + dq0MindOff[n];
  };
}

module.exports = {
  dq0Ad,
  CHUNK_SIZE, CK_TILE, CK_ROOM, T_ROCK, T_FLOOR, T_WALL, T_CORR, T_WINDOW, cacheBanks, cacheChunks, caveReport,
  PK_PORTCULLIS, T_DOOR, T_STEPS, xdBytes, saidTheme, DQ0_MAKE, dq0Life, THEMEC, builtChunks, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N,
  CSLOT_AT, liveChunks, O_WAY_OUT, O_NOWHERE, stratumRec, dq0Made, chunkDiff, dq0StatusRows, dq0Story, dq0Prose, inTurn,
  DQ0_REPEATS, dq0Answers, dq0Repeats, dq0Doubled, dq0Lost, dq0Check, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need,
  dq0Never, dq0Carried, dq0Here, dq0Named, dq0Thing, dq0Holder, dq0Root, dq0Intent, dq0View, DQ0_I, passings, dq0TypedAsk, dq0Typed, dq0Game, DQ0_CHAR,
  dq0PortalOff, dq0Portal, dq0RecPb, dq0Rec, dq0Alone, dq0Scene, dq0SceneNow, dq0BodyFb, dq0Body, dq0PcShock,
  dq0Grow, dq0Aware, dq0PortalAt, dq0OnWords, dq0Timer, dq0Median, dq0Worst, dq0Waits, dq0Held, DQ0_ANSWERS, dq0AnswerTest, WORN,
  DQ0_PLAYED, dq0Paces, dq0Own, dq0Asks, dq0HeldPaces, dq0LetterKeys, dq0PlayLetters, dq0WhenFree, dq0FightPause, PLAYER_PAUSE,
};
