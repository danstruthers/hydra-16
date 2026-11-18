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
const CHUNK_SIZE = 0xAA0, CK_TILE = 443, CK_ROOM = 8;
const T_ROCK = 0, T_FLOOR = 1, T_WALL = 2, T_CORR = 4, T_WINDOW = 6;
// The hub's chunk cache's banks (world.h's CACHE_BANK: 5 on, 8 of them), from its bank 0's (its magic "DQ") on: none
// of the other banks taken for chunks by their looks (the UI bank's map's layers would pass, slice 4.2's)
function cacheBanks(m) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51);
  return hub === undefined ? [] : [0, 1, 2, 3, 4, 5, 6, 7].map(i => banks[hub + 5 + i]).filter(b => b);
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
  const out = [], C = { room: 8, zone: 56, portal: 219, tile: 443, elev: 1467, pceil: 2555 };
  for (const b of cacheBanks(m)) {
    for (let k = 0; k < 3; k++) {
      const o = k * 0xAA0, nr = b[o + 5], nz = b[o + 6], np = b[o + C.portal - 1];
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
// read from its cache bank (world.h's CACHE_BANK: 5 + k / 3, CHUNK_AT: (k % 3) * 0xAA0), its 2,707 bytes (struct
// chunk's), by its place: { 'cx,cy': bytes }; the hub's banks the shared ones from its bank 0's (its magic "DQ") on
const CHUNK_BYTES = 2707, CHUNK_KEPT = 2556, WORLD_BANK_N = 21, CSLOT_AT = 8 * 79;
function liveChunks(m, sn) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51), out = {};
  if (hub === undefined || !banks[hub + WORLD_BANK_N]) return out;
  const w = banks[hub + WORLD_BANK_N];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * 8, b = banks[hub + 5 + Math.floor(k / 3)];
    if (!w[s] || w[s + 1] !== sn || !b) continue;
    const o = (k % 3) * 0xAA0;
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
    const r = w.subarray(i * 79, i * 79 + 79), u16 = k => r[k] | r[k + 1] << 8;
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
// floors from 1467; traps from 2513, 7 bytes each, their state the 5th): the squares' seen, visible and drawn bits
// (T_SEEN, T_VIS, T_DRAWN) and the rooms' visited (R_VISITED) aside, each byte that differs by what it is: portal, trap, square,
// opening, or other (none should be: the maker's own)
function chunkDiff(a, b) {
  const d = { portal: 0, trap: 0, square: 0, opening: 0, other: [] };
  for (let i = 0; i < CHUNK_BYTES; i++) {
    let x = a[i], y = b[i];
    if (i >= CHUNK_KEPT && i < CHUNK_KEPT + 9) continue;      // (The world's memory of it, the maker's none)
    if (i >= 443 && i < 1467) { x &= 0x2F; y &= 0x2F; }
    if (i >= 8 && i < 56 && (i - 8) % 8 === 4) { x &= ~1; y &= ~1; }
    if (x === y) continue;
    if (i >= 219 && i < 443 && [3, 8, 13].includes((i - 219) % 14)) d.portal++;
    else if (i >= 2513 && i < 2555 && (i - 2513) % 7 === 4) d.trap++;
    else if (i >= 443 && i < 1467) d.square++;
    else if (i >= 1467 && i < 2491 && (a[i - 1024] & 0x0F) !== (b[i - 1024] & 0x0F)) d.square++;   // (A changed square's floor)
    else if (i === 7 || (i >= 188 && i < 218)) d.opening++;
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
// twice, its trap twice: the look told it twice).  sim/test.js's dq0 check, in every dq0 test (HUBOWN=0 too)
const DQ0_REPEATS = [
  { what: /^You reach for the [a-z ]+\.$/, why: 'a thing handled, each command of a line (or typed ahead) its own first word: dq0firstword holds it to once a command' },
  { what: /^(Put down\.|You fumble for the sword\.|You see none of those here\.|You see no [a-z ]+ here\.|You carry none of those\.|You have no [a-z ]+\.|Taken\.)/, tests: /^dq0engage$/, why: 'its lines typed ahead, one answer' },
  { what: /^The [a-z ]+ (unslings|draws) (?:its|their) [a-z ]+\.$/,
    why: 'several of a kind readying their weapons in one answer, each one\'s told (slice 4.7\'s: three elves unslinging their hartbows)' },
  { what: /^Your hold gives way, \d+ feet up: you fall\.$/, why: 'a climb\'s holds given way and caught, each told as it comes, the climb on from the same height (dq0rope\'s grapple, 2026-11-06)' },
  { what: /^\(\d+ feet to go\.\)/, why: 'a dig\'s spells, each told as it ends (dig down: many spells to an answer)' },
  { what: /^(?:The work goes (?:well|slowly)|Your tool slips and turns; little comes of that spell of work|The rotten rock splits over you, and part of the roof falls into the hole)\.$/,
    why: 'a dig\'s spells, each told as it ends (a long dig: hours of spells between one stir and the next, some the same)' },
  { what: /^You (?:climb on|slip back a little|find no hold yet) \(\d+ feet to go\)\.$/,
    why: 'a climb\'s rounds, each told as it ends (a long climb: many rounds to an answer, some the same)' },
  { what: /^(?:(The|A) .* |Something )(bites|claws|strikes|swings|lunges|thrusts|shoots|throws)\b.*\bat you\b|^You (strike|swing|thrust|punch|kick|lunge|shoot|throw)\b|^(?:Your|Its|Their) [a-z]+ strikes .* instead\b/,
    why: 'a fight\'s rounds, each blow told as it falls (attack: the rounds till it ends, the same words some of them; in the dark "Something", unseen: dq0traces\' rats)' },
  { what: /^(?:You get to your feet|You fall|Everything goes black again|You hear something fall|The [a-z ]+ (?:gets to (?:its|their) feet|scrambles to its feet|coils itself up again|rights itself|flutters up again|rises again|falls|goes down|is thrown down|is flipped onto its back|drops to the floor|sinks to the floor))\.$/,
    why: 'a fight\'s falls and risings, each fall told as it falls and each rise as it gets up (a long fight: a creature down twice, a stumble\'s, a leg\'s wound\'s, up each time; dq0missile\'s rat; every fall told since 2026-10-30; the character knocked senseless a third time in one answer, "again" again: blows.c\'s harm_told, dq0antidote\'s adders since 2026-11-09)' },
  { what: /^The [a-z ]+ (?:struggles|scrabbles|writhes|flails its legs|flaps|wavers), but can't (?:get up|right itself|rise)\.$/,
    why: 'creatures down: each told once as it lies there (2026-11-02), so only two of a name in one answer, or one wounded between (dq0downonce holds it to that)' },
  { what: /^(?:You grapple with .+, but (?:it twists|they twist) free|You choke .+|(?:The [a-z ]+|Something) (?:struggles in your grip|resists your hold))\.$/,
    why: 'a grapple fought on (HOUSE.md\'s grappling, 2026-11-09): each round\'s try told as it comes, a hold missed, a choke, a held one\'s struggle, the same words some of them' },
  { what: /^You turn to (?:something|the [a-z ]+)\.$/,
    why: 'a fight\'s turns to a foe, each told as it turns (fighting on: struck from another side again and again; in the dark "something": dq0scent\'s rats, their dice by who asks, 2026-10-30)' },
  { what: /^The [a-z ]+ closes with you\.$/,
    why: 'each denizen\'s attack begun, told as it closes with the character (the user\'s, 2026-11-17: its first words, "The bandit closes with you."): two of a name closing in one answer, the same words (dq0fightverbs\' charge, both rats), or one coming back to the fight' },
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
  { what: /^(?:You go under!|You come up, gasping\.)$/,
    why: 'a swim\'s minutes as a walk goes on: under, up and under again, each told as it comes (the soak\'s seed 192)' },
  { what: /^(?:It falls senseless|They fall senseless|It flees|They flee|The .+ falls dead|You finish the .+ where (?:it lies|they lie)|The .+ gets to (?:its|their) feet|You get to your feet|You turn to the .+|You fight back against the .+)\.$/,
    why: 'fighting on (the user\'s, 2026-10-20): one attack fights its foes in turn, each falling, fleeing, getting up, fought back or turned to told as it comes (two of a name, the same words)' },
  { what: /^(?:You drop your .+|(?:The .+|[A-Z][a-z]+) drops (?:its|their) .+|The .+ snatches up (?:its|their) .+)\.$/,
    why: 'a fight\'s fumbles (slice 4.1b\'s hands), each told as it comes: a thing let fall at each fumble (two torches held, two fumbles), a denizen\'s weapon dropped and snatched up again its next turn, twice in a long answer\'s rounds (the soak\'s seed 85, 2026-10-27)' },
  { what: /^(?:You stop fighting|You fumble for the .+)\.$/,
    why: 'commands typed ahead as the character fights, each taking the fight over in turn, fought on between them (fighting on), their answers one' },
  { what: /^Something stirs\.$/, why: 'each stir\'s first words: one for each time what it sees of the denizens changes (a long command\'s answer has several)' },
  { what: /^(?:You strike a spark|It doesn't catch)\.$/, why: 'a fire-maker\'s tries, each told as it goes (the user\'s, 2026-11-02: "continue until successful, reporting as it goes along")' },
  { what: /^You keep at it\.$/,
    why: 'a battering\'s beat every fourth blow once its blows tell (the user\'s, 2026-10-22: told as it goes), twice or more in a dozen' },
  { what: /^Everything goes black\.$/, tests: /^dq0(?:finished|strike|helpless|venom|morale)$/,
    why: 'knocked senseless, come round in shock a round later, struck senseless again, all in one wait\'s answer (the UI waiting it out): two black-outs, each told (dq0strike\'s rats, fought with a bar, three; dq0morale\'s small orcs, a knife in the hand of each)' },
  { what: /^Everything goes black again\.$/, tests: /^dq0soak-/,
    why: 'as "Everything goes black." above, its second time and after, in a soak\'s fight' },
  { what: /^The [a-z]+ [a-z ]+ to your [a-z ]+ is gone\.$/, tests: /^dq0soak-/,
    why: 'the time between\'s healing: two wounds alike on one place, each told' },
  { what: /^(?:Quicker, you strike at .+|Something coils itself up again\.)$/, tests: /^dq0soak-/,
    why: 'a fight\'s long answer, a soak\'s: the same blow come out the same way two rounds of it, an adder out of sight coiling up twice' },
  { what: /^Enter takes /, tests: /^dq0soak-/,
    why: 'the time between\'s question asked again, a soak\'s answers refused (its echo in the sentence: dq0acct\'s prompt no game\'s)' },
  { what: /^You stop walking\.$/, tests: /^dq0soak-/,
    why: 'one walk stopped as the line came, a walk set off by the line stopped by a shot at it, each told (the fight soak\'s seed 85, 2026-11-04)' },
  { what: /^Something gets to its feet\.$/, tests: /^dq0soak-/,
    why: 'a fight\'s falls and risings, as the named, one out of sight down and up twice in a long answer (the fight soak\'s seed 66, 2026-11-04)' },
  { what: /^You feel your way [a-z]+\.$/, tests: /^dq0soak-/,
    why: 'a walk over to a thing in the dark, each leg of it told (the first stopped by what it felt)' },
  { what: /^The bite burns: there's venom in it\.$/,
    why: 'a fight\'s long answer: each venomous bite that gets through told as it falls, a dose more (the fight soak\'s seed 3, 2026-11-04)' },
  { what: /^You struggle, but can't get up\.$/,
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
// The hub's live chunks (world.h's struct chunk: portals from 219, 14 bytes each, their count at 218; traps from 2513,
// 7 bytes each, their count at 2512), each with two portals or two traps on one square: 'sn:cx,cy: 2 portals at x,y'
function dq0Doubled(m) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b), f = [];
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51);
  if (hub === undefined || !banks[hub + WORLD_BANK_N]) return f;
  const w = banks[hub + WORLD_BANK_N];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * 8, b = banks[hub + 5 + Math.floor(k / 3)];
    if (!w[s] || !b) continue;
    const o = (k % 3) * 0xAA0, at = w[s + 1] + ':' + (w[s + 2] | w[s + 3] << 8) + ',' + (w[s + 4] | w[s + 5] << 8);
    for (const [what, n, from, size, max] of [['portals', b[o + 218], 219, 14, 16], ['traps', b[o + 2512], 2513, 7, 6]]) {
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
function dq0Check(name, m) {
  return [...dq0Repeats(name, m.out).map(r => 'told ' + r.n + ' times in one answer (> ' + r.cmd + '): ' + JSON.stringify(r.sentence)),
    ...dq0Doubled(m).map(s => 'a chunk\'s fixtures doubled: ' + s), ...dq0Lost(name, m),
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
  const bare = l => !/^dq0( |$)/.test(l) ? l : [/ -R \d+$/, / -E$/, / -F$/, / -K$/, / -N$/, / -J$/, / -I$/, / -a$/, / -r \S+$/, / -R \d+$/].reduce((x, re) => x.replace(re, ''), l);
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
// What the character perceives of the place (the senses' record, struct sight), the denizens aside (they go their own
// ways), as one string: the same place seen the same, compared
const dq0View = s => !s || !s.sight ? '' : JSON.stringify(Object.assign({}, s.sight, { seq: 0, dens: [], first: 0 }));
const DQ0_I = { LIT: 0x01, HELD: 0x02, WORN: 0x04, HIDDEN: 0x08 };   // (items.h's I_*)
// The place the character's in come to see again n times (its passings, the map's: the torch put out five minutes,
// lit again; dq0bearing's)
const passings = n => Array(n).fill(['douse the torch', 'wait 5 minutes', 'light the torch']).flat();

// A dq0 test's lines typed, each at its prompt (rc's "% ", the game's "> ")
const dq0TypedAsk = lines => lines.map(l => '\u0101' + l + '\r').join('');
const dq0Typed = lines => dq0TypedAsk(lines.some(l => /^dq0 .*-d /.test(l)) ? ['dq0auto=1', ...lines] : lines);

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
    const r = w.subarray(i * 79, i * 79 + 79), u16 = k => r[k] | r[k + 1] << 8;
    if (r[0] && r[1] === sn) S = { x0: u16(11), y0: u16(13) };
  }
  if (!S) return null;
  const [PORTAL, PSIZE, PSTATE, NPORT, PKIND] = dq0PortalOff || (dq0PortalOff = require('../../sim/lib/hubown.js').evaluate([
    'offsetof (struct chunk, portal)', 'sizeof (struct portal)', 'offsetof (struct portal, state)', 'offsetof (struct chunk, nportals)',
    'offsetof (struct portal, kind)']));
  const cx = (x - S.x0) >> 5, cy = (y - S.y0) >> 5, lx = (x - S.x0) & 31, ly = (y - S.y0) & 31;
  for (let k = 0; k < 24; k++) {
    const sl = CSLOT_AT + k * 8;
    if (!w[sl] || w[sl + 1] !== sn || (w[sl + 2] | w[sl + 3] << 8) !== cx || (w[sl + 4] | w[sl + 5] << 8) !== cy) continue;
    const b = banks[hub + 5 + Math.floor(k / 3)], o = (k % 3) * 0xAA0;
    for (let i = 0; i < b[o + NPORT]; i++) {
      const q = o + PORTAL + i * PSIZE;
      if (b[q] === lx && b[q + 1] === ly) return { b, q, state: q + PSTATE, kind: b[q + PKIND] };
    }
  }
  return null;
}

let dq0RecPb = 0;
function dq0Rec(m) {
  const h = m.dq0.bank(0), t = h[12 + 2], lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
  const pb = dq0RecPb || (dq0RecPb = parseInt(/^al ([0-9A-F]+) \._pb$/m.exec(lbl())[1], 16)), mem = m.taskBankMem(t, m.taskRam[t][pb]) || new Uint8Array(0x2000);
  return { mem, at: i => 330 + 64 * 17 + 30 * i, FLAGS: 1, SN: 2, ID: 4, X: 8, Y: 10, ATTITUDE: 16, SERVE: 26, CONTACT: 27, ATE: 28, DRANK: 29 };
}

// Every denizen near but actor keep sent far off, friendly, and not free again for some five minutes: a scene with one foe in it (a dq0Poke's call)
function dq0Alone(m, h, L, K, at, keep) {
  const R = dq0Rec(m), far = 3000 + (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16);
  let n = 0;
  for (let k = 0; k < L.nactors; k++) if (k !== keep && h[at(k) + L.aName] && h[at(k) + L.aState]) {
    const o = R.at(h[at(k) + L.aDen]), x = 32706 + (n & 3), y = 32784 + (n++ >> 2);
    h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8;
    h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
    h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_FRIENDLY;
    h[at(k) + L.aDoing] = K.DO_ROAM;
    for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = far >>> (8 * i) & 255;
  }
  h[L.denSeq] = h[L.denSeq] + 1 & 255;
}

function dq0Poke(m, pokes) {
  const a = m.acia, read = a.read;
  let typed = '', g = 0, seen = {};
  m.hearts = [];
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (/^dq0 /.test(typed)) {
          ++g;
          seen = {};
        }
        seen[typed] = (seen[typed] || 0) + 1;
        const h = m.dq0 && m.dq0.bank(0);
        if (h) {
          const { L, K } = m.dq0.layout(), at = k => L.actor + k * L.actorSize, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const off = f => L['a' + f[0].toUpperCase() + f.slice(1)];   // (An actor's field by its name: dq0meaning's layout)
          const named = k => { let n = ''; for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]); return n; };
          for (const p of pokes) if (p.g === g && p.line === typed && p.n === seen[typed]) {
            for (const [f, v] of Object.entries(p.hub || {})) h[L[f]] = v;   // (The hub's own bytes, by the layout's names: pcSkew)
            for (const [kn, v] of Object.entries(p.fuel || {})) {   // (A thing's fuel left, minutes, by its kind's name: the
              const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);   //   first in the store, 2026-10-31)
              for (let i = 0; cat && i < L.nitems; i++) {
                const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
                if (!b || !b[o + L.iKind]) continue;
                let n = '';
                for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
                if (n === kn) { b[o + L.iFuel] = v & 255; b[o + L.iFuel + 1] = v >> 8 & 255; break; }
              }
            }
            for (const [kn, v] of Object.entries(p.wear || {})) {   // (A thing's wear, by its kind's name: the first in the store, 2026-11-02)
              const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);
              for (let i = 0; cat && i < L.nitems; i++) {
                const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
                if (!b || !b[o + L.iKind]) continue;
                let n = '';
                for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
                if (n === kn) { b[o + L.iWear] = v; break; }
              }
            }
            for (const [kn, v] of Object.entries(p.item || {})) {   // (A thing, by its kind's name: the first in the store, its flags and
              const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);   //   its hiding set, a scene's, 2026-11-02)
              for (let i = 0; cat && i < L.nitems; i++) {
                const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
                if (!b || !b[o + L.iKind]) continue;
                let n = '';
                for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
                if (n === kn) { b[o + L.iFlags] = b[o + L.iFlags] & ~(v.clear || 0) | (v.or || 0); if (v.hide !== undefined) b[o + L.iHide] = v.hide; if (v.size !== undefined) b[o + L.iSize] = v.size; if (v.quality !== undefined) b[o + L.iQuality] = v.quality; if (v.qty !== undefined) b[o + L.iQty] = v.qty; break; }
              }
            }
            if (p.traps) {                      // (The cached chunks' hidden traps' hiding set, a scene's, 2026-11-02; and on its own
              const w = m.dq0.bank(WORLD_BANK_N);   //   chunk's room's stops (a room searched whole's, but its own) a plate of an alarm, hidden,
                                                    //   and its doors made secret)
              for (let c = 0; c < 24; c++) {
                const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3));
                if (!w[sl] || !b) continue;
                const o = (c % 3) * 0xAA0;
                if (p.traps.hide !== undefined) for (let i = 0; i < b[o + 2512] && i < 6; i++) if (b[o + 2513 + i * 7 + 4] & 0x20) b[o + 2513 + i * 7 + 5] = p.traps.hide;
                const px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8, st = stratumRec(m, h[L.pcSn]), ox = px - (st ? st.x0 : 0), oy = py - (st ? st.y0 : 0);
                if (p.traps.plates && w[sl + 1] === h[L.pcSn] && (w[sl + 2] | w[sl + 3] << 8) === ox >> 5 && (w[sl + 4] | w[sl + 5] << 8) === oy >> 5) {
                  const lx = ox & 31, ly = oy & 31;
                  for (let r = 0; r < b[o + 5]; r++) {
                    const a = o + 8 + r * 8, rx = b[a], ry = b[a + 1], rw = b[a + 2], rh = b[a + 3];
                    if (lx < rx || ly < ry || lx >= rx + rw || ly >= ry + rh) continue;
                    for (let j = 0; j < rh; j += 3) for (let i = 0; i < rw; i += 3) {
                      const n = b[o + 2512], q = o + 2513 + n * 7;
                      if (n >= 6 || (lx - rx >> 0) - i >= 0 && lx - rx - i < 3 && ly - ry - j >= 0 && ly - ry - j < 3) continue;
                      b[q] = rx + i + 1; b[q + 1] = ry + j + 1; b[q + 2] = K.TK_ALARM; b[q + 3] = K.TT_PLATE; b[q + 4] = 0x20; b[q + 5] = p.traps.hide || 0; b[q + 6] = 1;
                      b[o + 2512] = n + 1;
                    }
                  }
                  for (let i = 0; i < b[o + 218]; i++) b[o + 219 + i * 14 + 3] |= 0x20;
                }
              }
            }
            if (p.call) p.call(h, L, K, at, named);   // (A scene of its own, 2026-11-03)
            const k = p.name === undefined ? p.k : [...Array(L.nactors).keys()].find(j => named(j) === p.name);   // (By its name: the first)
            if (k === undefined) continue;
            for (const [f, v] of Object.entries(p.set || {})) if (f === 'x' || f === 'y') w16(at(k) + off(f), v); else h[at(k) + off(f)] = v;
            if (p.beside !== undefined || p.engage) {   // (Put beside the character, engaged: the hub's and its record's)
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
            if (p.senseless) {                  // (Senseless, for the tenths given, as the referee has it and the world: down, prone, 2026-11-03)
              const S = dq0Sheet(m), o = S.at(h[at(k) + L.aDen]), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16 | h[L.now + 3] << 24, till = (now + p.senseless) >>> 0, rd = L.actor + k * L.actorSize + L.aReady;
              S.mem[o + S.STATE] = S.mem[o + S.STATE] & ~3 | 1;
              for (let i = 0; i < 4; i++) S.mem[o + S.OUT + i] = h[rd + i] = till >>> (8 * i) & 255;
              h[at(k) + L.aDoing] = K.DO_DOWN;
              h[at(k) + L.aPos] = K.POS_PRONE;
            }
            if (p.hurt) {                       // (Hurt, as the referee has it: a minor injury of the levels given, every test of its the worse by five points a level: its Dodge within reach of a blow, its courage shaken, 2026-11-04)
              const S = dq0Sheet(m), o = S.at(h[at(k) + L.aDen]);
              S.mem[o + 9] = 1;                 // (Its ninj, its inj[0] at 10: a location, an aspect, HARN_MINOR, its level)
              S.mem.set([0, 0, 1, p.hurt, 0, 0], o + 10);
            }
            if (p.lame) {                       // (Prone, seriously wounded and Agility 1, as the referee has it: its Rise an Agility test failed, 2026-11-03)
              const S = dq0Sheet(m), o = S.at(h[at(k) + L.aDen]);
              S.mem[o + 4] = 1;                 // (struct beast's att[3])
              S.mem[o + 9] = 1;                 // (Its ninj, its inj[0] at 10: a location, an aspect, HARN_SERIOUS, its level 2)
              S.mem.set([0, 0, 2, 2, 0, 0], o + 10);
              h[at(k) + L.aPos] = K.POS_PRONE;
            }
            if (p.rec) {
              const R = dq0Rec(m), o = R.at(h[at(k) + L.aDen]);
              if (p.rec.or) R.mem[o + R.FLAGS] |= p.rec.or;
              if (p.rec.serve !== undefined) R.mem[o + R.SERVE] = p.rec.serve;
              if (p.rec.contact !== undefined) R.mem[o + R.CONTACT] = p.rec.contact;
            }
          }
          const acts = [];
          for (let k = 0; k < L.nactors; k++) {
            let n = '';
            for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
            if (n) {
              const R = dq0Rec(m), o = R.at(h[at(k) + L.aDen]);
              acts.push({ k, name: n, heart: (h[at(k) + L.aHeart] << 24) >> 24, lead: h[at(k) + L.aLead], power: h[at(k) + L.aPower], mood: h[at(k) + L.aMood], bonds: h[at(k) + L.aBonds], doing: h[at(k) + L.aDoing],
                rank: R.mem[o + R.FLAGS] >> 5 & 3, prom: !!(R.mem[o + R.FLAGS] & 0x80) });
            }
          }
          m.hearts.push({ g, line: typed, n: seen[typed], acts });
        }
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}

// The creatures' bodies in the referee's bank (dq0harn's fb: tire.c's struct bbody, 8 bytes by the sheet's number, a
// denizen's record's: 2026-10-31, every actor's tiring): { mem, at(n) }, its levels at 0, wfl at 1, ex at 2, at (the clock)
// at 4
let dq0BodyFb = 0;
function dq0Body(m) {
  const h = m.dq0.bank(0), t = h[12 + 3], lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0harn', 'dq0harn.lbl'), 'latin1');
  const fb = dq0BodyFb || (dq0BodyFb = parseInt(/^al ([0-9A-F]+) \._fb$/m.exec(lbl())[1], 16)), mem = m.taskBankMem(t, m.taskRam[t][fb]) || new Uint8Array(0x2000);
  return { mem, at: n => 8 * n };
}
// The creatures' sheets in the referee's bank (dq0harn's sb: dq0harn.h's struct beast, 32 bytes by the sheet's number): { mem, at(n) }, its
// state at 7, its out (senseless till then, the clock's tenths, 4 bytes) at 28.  (A scene's: one senseless as the referee has it)
let dq0SheetSb = 0;
function dq0Sheet(m) {
  const h = m.dq0.bank(0), t = h[12 + 3], lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0harn', 'dq0harn.lbl'), 'latin1');
  const sb = dq0SheetSb || (dq0SheetSb = parseInt(/^al ([0-9A-F]+) ._sb$/m.exec(lbl())[1], 16)), mem = m.taskBankMem(t, m.taskRam[t][sb]) || new Uint8Array(0x2000);
  return { mem, at: n => 32 * n, STATE: 7, OUT: 28 };
}
// The character senseless for the tenths given and come round in shock (a scene's, the referee's pc and pc_out: harn.h's struct harn_char,
// dq0harn's own): its shock rolls the book's dice, in shock or not as they fall (2026-11-03)
function dq0PcShock(m, tenths) {
  const h = m.dq0.bank(0), { L } = m.dq0.layout(), ram = m.taskRam[h[12 + 3]], root = path.join(TESTS, '..');
  const lbl = fs.readFileSync(path.join(root, 'obj', 'games', 'dq0harn', 'dq0harn.lbl'), 'latin1'), ad = n => parseInt(new RegExp('^al ([0-9A-F]+) \\._' + n + '$', 'm').exec(lbl)[1], 16);
  const H = fs.readFileSync(path.join(root, 'sdk', 'c', 'include', 'harn.h'), 'latin1'), def = n => +new RegExp('#define\\s+' + n + '\\s+(\\d+)').exec(H)[1];
  const STATE = 2 * 24 + 10 + def('HARN_MEDMAX') + 2 * def('HARN_PSYMAX') + 1 + def('HARN_MAXSKILLS') + 3, now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16 | h[L.now + 3] << 24, till = (now + tenths) >>> 0;
  ram[ad('pc') + STATE] = 2;                    // (HARN_SHOCK)
  ram[ad('pc') + STATE + 1] = 5;                // (Its shockhr, as the library's)
  for (let i = 0; i < 4; i++) ram[ad('pc_out') + i] = till >>> (8 * i) & 255;
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
        else if (seen && !done && typed === line && m.dq0) {
          const { L } = m.dq0.layout(), h = m.dq0.bank(0);
          for (let k = 0; h && k < L.nactors; k++) {
            const o = L.actor + k * L.actorSize;
            let n = '';
            for (let i = 0; h[o + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[o + L.aName + i]);
            if (n === actor) { h[o + L.aDoing] = 1; done = true; }
          }
        }
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
    const s = CSLOT_AT + k * 8, b = banks[hub + 5 + Math.floor(k / 3)];
    if (!w[s] || w[s + 1] !== sn || !b || (w[s + 2] | w[s + 3] << 8) !== cx || (w[s + 4] | w[s + 5] << 8) !== cy) continue;
    const c = (k % 3) * 0xAA0;
    for (let i = 0; i < b[c + 218]; i++) {
      const o = c + 219 + i * 14;
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
        a.rxQueue.unshift(...armed.text);
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
      const h = m.dq0 && m.dq0.bank(0), ui = h ? h[12] : 0;
      return ui && m.taskRam[ui] ? m.taskRam[ui][busyAt] : null;
    } catch (e) { return null; }
  };
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {                    // (Each CR's, and the words typed before it)
      const c = a.rx;
      if (c === 13) { enters.push({ t: a.rxAt, pos: out.length, busy: busy(), line: typed }); typed = ''; }
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
            (c.want === '*' ? anyWords(story, c.line, story.length) >= 0 : c.want.split('|').some(x => story.includes(x))) && prompt < 0) prompt = idx.length;
          i += q[0].length - 1;
          continue;
        }
      }
      if (rev || out[i] < ' ') continue;
      story += out[i];
      idx.push(i);
    }
    const w = c.want === '*' ? anyWords(story, c.line, prompt < 0 ? story.length : prompt) :
      c.want.split('|').map(x => story.indexOf(x)).filter(x => x >= 0).reduce((a, b) => a < 0 || b < a ? b : a, -1);   // (Words the answer may begin with, | between)
    let last = prompt < 0 ? idx.length - 1 : prompt - 1;
    while (last > 0 && story[last] === ' ') last--;
    let own = last, pause = null, pauseAt = '';  // (Its own words: up to the world's news told after its first letter)
    if (w >= 0) {
      const k = story.slice(w + 1, last + 1).search(c.news || NEWS);
      if (k >= 0) for (own = w + k; own > w && story[own] === ' '; own--);
      for (let j = w + 1, p = -1; j <= own; j++) {
        const g = at[idx[j]] - at[idx[j - 1]];
        if (g > p) { p = pause = g; pauseAt = story.slice(Math.max(w, j - 24), j) + ' | ' + story.slice(j, j + 24); }
      }
      if (pause === null && own >= w) pause = 0;
    }
    return { line: c.line, first: w < 0 ? null : at[idx[w]] - e.t, whole: w < 0 || last < w ? null : at[idx[last]] - e.t, pause, pauseAt, busy: e.busy };
  }
}

// (dq0Timer's want '*': the answer's own words, whatever they are, 2026-11-17: the first letter in story s, before lim,
// after the line's own echo ("> " and line) and past the world's news told before them, a stir's, a sound's (NEWS, and
// what a stir's telling lists: "(A) A giant rat ...", "... is gone from sight."); -1 none)
function anyWords(s, line, lim) {
  let j = s.indexOf('> ' + line);
  j = j >= 0 && j < 8 ? j + line.length + 2 : 0;
  for (let news = 0; ; news = 1) {               // (A stir's list after its news: news too; one first, the answer's)
    while (j < lim && s[j] === ' ') j++;
    if (j >= lim) return -1;
    const rest = s.slice(j, lim);
    if (!/^(?:Something stirs|Nothing stirs in sight|You hear (?!nothing))/.test(rest) && (!news || !/^(?:\(|[^.!?]* gone from sight[.!?])/.test(rest))) return j;
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
// Each line of a D17 test's typings timed (dq0Timer's game: the test's own first, then dq0Paces' menacing and played),
// its first letter held to D17.first (tests/lib/trends.js's target: f the failures, notes; their names the test's own
// first, the others' ", menacing" and ", played" after the line); the first typing's words its want, the others' '*'
// (their answers their own: the kit, the hour, the questions of a game without the switches).  gs the three games'
// places among those typed (0 on)
function dq0HeldPaces(t, f, notes, timer, gs, cmds) {
  [['', cmds], [', menacing', null], [', played', null]].forEach(([sfx, cs], i) => {
    for (const r of timer.game(gs[i], cs || cmds.map(c => ({ line: c.line, want: '*' })))) {
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
  ['look', 'You', 'read whole'], ['look ahead', 'You peer ahead|You turn', 'read'], ['go up', 'There\'s no way up or down here.'], ['go to the far side', 'You'], ['walk to the window', 'You head for'], ['take the bones', 'You look for the bones|You go over to the bones.'], ['look', 'You', 'read'],
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

module.exports = {
  CHUNK_SIZE, CK_TILE, CK_ROOM, T_ROCK, T_FLOOR, T_WALL, T_CORR, T_WINDOW, cacheBanks, cacheChunks, caveReport,
  PK_PORTCULLIS, T_DOOR, T_STEPS, xdBytes, saidTheme, DQ0_MAKE, dq0Life, THEMEC, builtChunks, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N,
  CSLOT_AT, liveChunks, O_WAY_OUT, O_NOWHERE, stratumRec, dq0Made, chunkDiff, dq0StatusRows, dq0Story, inTurn,
  DQ0_REPEATS, dq0Answers, dq0Repeats, dq0Doubled, dq0Lost, dq0Check, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need,
  dq0Never, dq0Carried, dq0Here, dq0Named, dq0Thing, dq0Holder, dq0View, DQ0_I, passings, dq0TypedAsk, dq0Typed,
  dq0PortalOff, dq0Portal, dq0RecPb, dq0Rec, dq0Alone, dq0Poke, dq0BodyFb, dq0Body, dq0SheetSb, dq0Sheet, dq0PcShock,
  dq0Grow, dq0Aware, dq0PortalAt, dq0OnWords, dq0Timer, dq0Median, dq0Worst, dq0Waits, dq0Held, DQ0_ANSWERS, dq0AnswerTest, WORN,
  DQ0_PLAYED, dq0Paces, dq0Own, dq0Asks, dq0HeldPaces,
};
