// ****************************************************************************
// map.js - Dingus Quest Zero's tests: the map, the dungeon's making, digging and travel.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { typed } = require('../../lib/common.js');
const {
  CHUNK_SIZE, CK_TILE, CK_ROOM, T_ROCK, T_FLOOR, T_WALL, T_CORR, T_WINDOW, cacheBanks, cacheChunks, caveReport,
  PK_PORTCULLIS, T_DOOR, T_STEPS, builtChunks, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N, CSLOT_AT, liveChunks,
  O_WAY_OUT, O_NOWHERE, stratumRec, dq0Made, chunkDiff, dq0StatusRows, dq0Story, inTurn, dq0Answers, dq0Turns, dq0Ev,
  dq0Rep, dq0Evs, dq0Need, dq0Never, dq0Carried, dq0Here, dq0Named, dq0Thing, dq0View, passings, dq0Typed, dq0Portal,
  dq0Rec, dq0Poke, dq0OnWords
} = require('../../lib/dq0.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')

// The chunks of stratum sn in the hub's cache, as cacheChunks, with their floors' elevations (elev, signed) too
// (dq0cavern's: a great cavern's parts, slice 4.10's)
function cavernChunks(m, sn) {
  const out = cacheChunks(m, sn);
  for (const b of cacheBanks(m)) for (let k = 0; k < 3; k++) {
    const o = k * CHUNK_SIZE, c = out.get((b[o + 1] | b[o + 2] << 8) + ',' + (b[o + 3] | b[o + 4] << 8));
    if (b[o] === sn && c && !c.elev) c.elev = Array.from(b.subarray(o + CK_TILE + 1024, o + CK_TILE + 2048), e => e << 24 >> 24);
  }
  return out;
}

// The character's square as each line is typed (dq0cavern's): t.where, a { line, sn, x, y } for each line typed in a
// game, the hub's as it stands then (what the line before left); on(line) called for each, if it's given
function dq0Where(t, m, on) {
  const a = m.acia, read = a.read;
  let typed = '';
  t.where = [];
  a.read = (r, x) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (m.dq0 && m.dq0.games.length) {
          const { L } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
          t.where.push({ line: typed, sn: h[L.pcSn], x: u16(L.pcX), y: u16(L.pcY) });
          if (on) on(typed);
        }
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, x);
  };
}
// A built stratum's chunks (dq0m/built.c, uses.c), read from the hub's cache in the shared segments' banks (world.h's
// struct chunk, three to a bank, 0xAA0 apart): each one's place, rooms (their uses, world.h's U_*), zones (their
// ceilings), portals, squares and floors, and its passages' ceiling (pceil); those of stratum sn that look whole
const U_HALL = 1, U_CELL = 2, U_CRYPT = 4, U_STORE = 7, U_KITCHEN = 8, U_PANTRY = 9, U_QUARTERS = 13, U_GATEHOUSE = 17,
  U_GUARDROOM = 18;
// (A chunk's main room: its largest, the first of the largest; the uses' plan's)
const mainRoom = c => c.rooms.reduce((a, r) => r.w * r.h > a.w * a.h ? r : a, c.rooms[0]);
// The maker's map in a snapshot (slice 4.2's: dq0meaning's map and mapLay): each square shown by its layer (ML_*: seen
// now, remembered, drawn on the map carried), and the @'s place: { now, kept, drawn, all: [{ r, i, c }], pc: { r, i } }
const DQ0_ML = ['now', 'kept', 'drawn', 'pc', 'compass'];
// (MAPDUMP=1: a snapshot's map, each square's layer after its row: n now, k remembered, d drawn, @ the character)
const dq0MapDump = s => !s || !s.map ? '(no map)' : s.map.map((row, r) => row.padEnd(64) + ' ' + Array.from(row, (c, i) => c === ' ' ? ' ' : 'nkd@c'[s.mapLay[r][i]]).join('')).filter(l => l.trim()).join('\n');
function dq0MapLayers(s) {
  const out = { now: [], kept: [], drawn: [], all: [], pc: null, none: !s || !s.map || !s.mapLay };
  if (out.none) return out;
  s.map.forEach((row, r) => {
    for (let i = 0; i < row.length; i++) {
      if (row[i] === ' ') continue;
      const q = { r, i, c: row[i] }, l = DQ0_ML[s.mapLay[r][i]];
      out.all.push(q);
      if (l === 'pc') out.pc = { r, i };
      else if (out[l]) out[l].push(q);
    }
  });
  return out;
}
// A staircase of short steps made in the rock (a test's scene, HOUSE.md's a passage's heading): after the Enter of a line
// typed ({ g: the game, 1 on, line, n: its nth time typed in it }), in the chunk of the hub's cache with stratum g's
// stairway up in it (the character come down to it: dq0 -T, d), from a square of the cave's wall (at: its place from
// the stairs, the cave's floor north of it) on, each step one of steps (DIR_*: 1 east, 2 south): passages' squares (T_CORR) at the
// floor's height, each through rock or a wall with nothing open beside it but the steps before and after; the senses'
// grid made again (dq0p's lgen).  Gives { done, why }
function dq0CarveStairs(m, p) {
  const a = m.acia, read = a.read, r = { done: false, why: 'its line never typed' };
  let typed = '', g = 0, seen = {};
  const carve = () => {
    const h = m.dq0 && m.dq0.bank(0);
    if (!h) return 'no hub';
    for (const b of cacheBanks(m)) for (let k = 0; k < 3; k++) {
      const o = k * CHUNK_SIZE, T = (x, y) => o + CK_TILE + y * 32 + x, E = (x, y) => o + CK_TILE + 1024 + y * 32 + x;
      if (b[o] !== p.sn || !b[o + 5] || b[o + 5] > 6) continue;
      let u = -1;
      for (let i = 0; i < 1024 && u < 0; i++) if ((b[o + CK_TILE + i] & 15) === 7) u = i;
      if (u < 0) continue;
      let x = (u & 31) + p.at[0], y = (u >> 5) + p.at[1];
      if ((b[T(x, y)] & 15) !== T_WALL || (b[T(x, y - 1)] & 15) !== T_FLOOR) return 'no wall there, south of the floor';
      const z = b[E(x, y - 1)], sq = [[x, y]];
      for (const d of p.steps) { x += d === 1 ? 1 : 0; y += d === 2 ? 1 : 0; sq.push([x, y]); }
      const on = new Set(sq.map(([i, j]) => i + ',' + j));
      for (let n = 1; n < sq.length; n++) {
        const [i, j] = sq[n];
        if (i < 1 || i > 30 || j < 1 || j > 30) return 'off the chunk';
        for (const [di, dj] of [[0, -1], [1, 0], [0, 1], [-1, 0], [0, 0]]) {
          const t = b[T(i + di, j + dj)] & 15;
          if (t !== 0 && t !== T_WALL && !on.has((i + di) + ',' + (j + dj))) return 'open beside ' + i + ',' + j;
        }
      }
      for (const [i, j] of sq) { b[T(i, j)] = (b[T(i, j)] & 0xF0) | T_CORR; b[E(i, j)] = z; }
      const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0p', 'dq0p.lbl'), 'latin1');
      const lg = parseInt(/^al ([0-9A-F]+) \._lgen$/m.exec(lbl)[1], 16), ram = m.taskRam[h[12 + 6]];
      ram[lg] = (ram[lg] + 1) & 255;
      return '';
    }
    return 'no chunk with the stairway up';
  };
  a.read = (rr, t) => {
    if (rr === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (/^dq0 /.test(typed)) { ++g; seen = {}; }
        seen[typed] = (seen[typed] || 0) + 1;
        if (!r.done && g === p.g && typed === p.line && seen[typed] === p.n) { const w = carve(); r.done = !w; r.why = w; }
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(rr, t);
  };
  return r;
}

module.exports = {
  tests: [
    {
      name: 'dq0', meaning: true, what: 'Dingus Quest Zero (games/dq0) at rc, its turns pace, by what happens (the hub\'s events and state: sim/lib/dq0meaning.js, tests.js\'s dq0Turns): a character from hmchar\'s file (-c) begins in a room by the way out; commands typed ahead (each waits for its prompt): the way out shut, a shut door in the way, a door opened (turned to first), walked through to a passage, another opened and a room come into the first time, crossed, no stairs there, back to the first room; west and go west the same (no way there); the map, the help, a word the game doesn\'t know and a look taking no time, a wait a round; quit, the game\'s end (the ledges: dq0harm; the game running hmchar itself: dq0veraserial)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'look', 'u', 'go through the door behind me on my right', 'turn around',
        'open the door behind me on my right', 'go through it', 'open', 'go through it', 'e', 'u', 'go back to where I came in', 'w', 'go west', 'map', 'help', 'foo', 'look', 'wait', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], g = T.of('dq0 -p t -s 3 -c /ram/c.hm'), s = g.after;
        dq0Need(f, g, 'E_ARRIVE', { how: 'A_START' }, 'the game not begun');
        if (!s || !s.sight || s.sight.where !== 0 || !s.sight.open.some(o => o.kind === 'O_WAY_OUT')) f.push('not begun in a room by the way out');
        const u = T.of('u'), n = T.of('go through the door behind me on my right'), o = T.of('open the door behind me on my right');
        dq0Need(f, u, 'E_NOWAY', { why: 'N_SHUT' }, 'the way out not shut');
        dq0Need(f, n, 'E_NOWAY', { why: 'N_DOOR' }, 'no shut door in the way');
        dq0Need(f, o, 'E_PORTAL', { po: 'PO_OPENED' }, 'the door behind not opened');
        if (o.at && o.after && o.at.pc.face === o.after.pc.face) f.push('not turned to the door behind');
        const n2 = T.after(o, 'go through it'), op = T.after(n2, 'open'), n3 = T.after(op, 'go through it'), e = T.after(n3, 'e'), u2 = T.after(e, 'u'), s2 = T.after(u2, 'go back to where I came in');
        dq0Need(f, n2, 'E_ARRIVE', {}, 'not through the door');
        if (n2.at && n2.after && !(n2.after.pc.y < n2.at.pc.y)) f.push('not gone north');
        dq0Need(f, op, 'E_PORTAL', { po: 'PO_OPENED' }, 'the next door not opened');
        dq0Need(f, n3, 'E_ARRIVE', { first: true }, 'no room come into the first time');
        dq0Need(f, e, 'E_ARRIVE', {}, 'the room not crossed');
        if (e.at && e.after && !(e.after.pc.x > e.at.pc.x)) f.push('not gone east');
        dq0Need(f, u2, 'E_NOWAY', { why: 'N_NO_STAIRS' }, 'up with no stairs not refused');
        dq0Need(f, s2, 'E_ARRIVE', {}, 'not back south');
        if (!s2.after || !s2.after.sight || !s2.after.sight.open.some(o => o.kind === 'O_WAY_OUT')) f.push('not back by the way out');
        for (const l of ['w', 'go west']) dq0Need(f, T.after(s2, l), 'E_NOWAY', { why: 'N_NO_EXIT' }, l + ': not refused (no way)');
        for (const l of ['map', 'help', 'foo', 'look']) {
          const t = T.after(s2, l);
          if (t.k < 0 || t.events.length || !t.at || !t.after || t.after.now !== t.at.now) f.push(l + ': not a moment\'s (' + dq0Evs(t) + ')');
        }
        const w = T.of('wait');
        dq0Need(f, w, 'E_TIME', {}, 'no time waited');
        if (w.at && w.after && !(w.after.now > w.at.now)) f.push('the clock not on after a wait');
        dq0Need(f, T.of('quit'), 'E_BYE', {}, 'the game not ended');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0deep', meaning: true, what: 'Dingus Quest Zero down a way to the stratum below (dq0 -T: the character begins at the first way down of the top stratum), by the world\'s events, what the character perceives and the hub\'s cache: the stratum below grown in outline as the character first takes the way, its first chunk made (natural, at this seed: caves), come into the first time, mapped (the maker\'s map), and along a tunnel; the caves irregular (the user\'s, 2026-10-16: "not straight tunnels, not rectangular rooms"): the cache\'s chunks read, their chambers ragged, their tunnels bending, every open square reached, their edges met',
      init: 't_rc', cycles: 500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'd', 'map', 'sw', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], d = T.of('d'), mp = T.of('map'), sw = T.of('sw');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN', first: true }, 'not down to the stratum below, the first time');
        if (!d.after || d.after.pc.sn !== 1 || !d.after.sight || d.after.sight.stratum !== 1) f.push('not on the stratum below');
        if (!mp.after || !mp.after.map.some(r => r.trim())) f.push('no map drawn');
        dq0Need(f, sw, 'E_ARRIVE', {}, 'not along the tunnel');
        return [...f, ...caveReport(m, 1, 1)];
      },
      sdcard: true
    },
    {
      name: 'dq0fog', meaning: true, what: 'Dingus Quest Zero\'s map as the character knows the place (slice 4.2, DUNGEON.md\'s the map kept; the user\'s, 2026-10-16 and 2026-10-23: only what it sees now plain, the rest obscured, a mental map fading by its Intelligence and Will, the better kept where it\'s often been, a map drawn on paper while mapping as it goes), by the maker\'s map (the UI bank\'s rows and their layers, ML_*): at seed 3 (dq0 -T, a human hunter: INT 10, WIL 13) what it sees now with its heights, what it saw facing the other way faint (a floor a comma); its torch put out, all of it remembered, none seen now; an hour later its squares put off where they truly are (faded a tenth: it doesn\'t know north underground); three hours on, gone; the same place come to again four times (down the stairway and five minutes there each time) still on the map after three hours in the dark; with paper and charcoal (dq0 -k: the charcoal, in the pack, put in the pouch on its belt to be handy, HOUSE.md\'s handy tools) mapping as it goes, a map begun ("You begin a map."; Drawing opened by it the referee\'s, dq0rules\'), what it drew shown as drawn while it carries the map (its torch out, nothing seen now), not once it\'s put down, again as it\'s taken up',
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'turn around', 'map', 'douse the torch', 'map', 'wait 1 hour', 'map', 'wait 2 hours', 'map', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'd', 'wait 5 minutes', 'u', 'd', 'wait 5 minutes', 'u', 'd', 'wait 5 minutes', 'u', 'douse the torch',
        'wait 3 hours', 'map', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k paper,charcoal', 'put the charcoal in the pouch', 'map as I go', 'turn around',
        'turn around', 'douse the torch', 'map',
        'light the torch', 'drop the map', 'douse the torch', 'map', 'light the torch', 'take the map', 'douse the torch', 'map', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), maps = T.filter(t => t.line === 'map');
        if (maps.length !== 8) return ['the eight maps not typed: ' + maps.length];
        const [m1, m2, m3, m4, p1, d1, d2, d3] = maps.map(t => dq0MapLayers(t.after));
        if (process.env.MAPDUMP) maps.forEach(t => console.log(t.k, dq0MapDump(t.after)));
        if (m1.none || !m1.now.length || !m1.kept.length) f.push('facing the other way: ' + m1.now.length + ' squares seen now, ' + m1.kept.length + ' remembered (both wanted)');
        if (m1.kept.some(q => q.c === '.' || /[1-9a-i]/.test(q.c))) f.push('a remembered square drawn as one seen now (a floor not faint, or its height)');
        if (!m1.now.some(q => q.c === '.') || m1.now.some(q => q.c === ',')) f.push('the floor it sees now not plain');
        if (m2.now.length || !m2.kept.length) f.push('its torch out: ' + m2.now.length + ' squares seen now (none wanted), ' + m2.kept.length + ' remembered');
        const at = l => new Set(l.map(q => q.r + ',' + q.i));
        const k2 = at(m2.kept);
        if (!m3.kept.some(q => !k2.has(q.r + ',' + q.i))) f.push('an hour on, nothing put off where it was (a human, no north sense)');
        if (m4.kept.length) f.push('three hours on, ' + m4.kept.length + ' squares still remembered (seen once: gone)');
        if (!p1.kept.length) f.push('come to four times: nothing remembered after three hours');
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k paper,charcoal'), mg = T.after(g, 'map as I go');
        if (!mg.after || !mg.after.pc.map || !dq0Thing(mg.after, 'map') || dq0Thing(mg.after, 'paper')) f.push('mapping as it goes with paper: no map begun (pc_map ' + (mg.after && mg.after.pc.map) + ')');
        inTurn(s, ['> map as I go', 'You begin a map.'], f, 'the map begun: ');
        if (!d1.drawn.length) f.push('its map carried: nothing shown as drawn');
        if (d2.drawn.length) f.push('its map put down: ' + d2.drawn.length + ' squares still shown as drawn');
        if (!d3.drawn.length) f.push('its map taken up again: nothing shown as drawn');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0mapturn', what: 'Dingus Quest Zero\'s map turned as the character faces (slice 4.2, DUNGEON.md\'s bearings underground; the user\'s, 2026-10-16: a dwarf\'s north up, anyone else\'s the way it faces up), by the maker\'s map and the words: at seed 3 (dq0 -T) a human facing north, the map north up, the character in its middle, a compass in its top right corner (N above +); facing east, east up (the rows turned a quarter: what\'s to its south to its right), the compass\'s N to the left of its +, "Facing E" beside it and no square given (its coordinates would give the true north away); a dwarf (hmchar -p k) at the same place facing east, north up (the window kept in the stratum, no compass; the window by the room\'s door where the human\'s north-up map has it, the human\'s facing east turned a quarter), its square given; its torch put out an hour, fewer squares remembered but none put off (a dwarf knows north)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'hmchar -q -s 9 -p k -o /ram/d.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'face north', 'map', 'face east', 'map', 'quit',
        'dq0 -p t -s 3 -c /ram/d.hm -T', 'face east', 'map', 'douse the torch', 'map', 'wait 1 hour', 'map', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], maps = T.filter(t => t.line === 'map');
        if (maps.length !== 5) return ['the five maps not typed: ' + maps.length];
        const [hn, he, de, dk, dw] = maps.map(t => dq0MapLayers(t.after));
        if (process.env.MAPDUMP) maps.forEach(t => console.log(t.k, dq0MapDump(t.after)));
        for (const [l, x] of [['facing north', hn], ['facing east', he]]) if (!x.pc || x.pc.r !== 10 || x.pc.i !== 32) f.push('a human ' + l + ': the @ not in the middle (' + JSON.stringify(x.pc) + ')');
        // (The window by the room's door: where the dwarf's north-up map has it, the human's has it turned)
        const st = x => x.all.filter(q => q.c === '"').map(q => ({ dx: q.i - x.pc.i, dy: q.r - x.pc.r }));
        const ds = de.pc ? st(de) : [];
        if (!ds.length) f.push('the dwarf\'s map: no window near it');
        else {
          const { dx, dy } = ds[0];
          if (!st(hn).some(q => q.dx === dx && q.dy === dy)) f.push('facing north: the window not where north up has it (' + dx + ',' + dy + ')');
          if (!st(he).some(q => q.dx === dy && q.dy === -dx)) f.push('facing east: the window not turned a quarter (' + dy + ',' + -dx + ' wanted; ' + JSON.stringify(st(he)) + ')');
        }
        // (The compass, the UI's: the map's lines as they were said, its first three rows' last columns)
        const plain = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '').split(/\r?\n/);
        const rows = (k, n) => { let c = 0; const i = plain.findIndex(l => l.startsWith('The map (you face') && c++ === k); return i < 0 ? [] : plain.slice(i + 1, i + 1 + n); };
        const rn = rows(0, 3), re = rows(1, 3), rd = rows(2, 3);
        if (!rn[0] || rn[0][62] !== 'N' || !rn[1] || rn[1][62] !== '+') f.push('facing north: no compass, N above + (' + JSON.stringify(rn.map(l => l.slice(60, 64))) + ')');
        if (!re[1] || re[1].slice(61, 63) !== 'N+') f.push('facing east: no compass, N left of + (' + JSON.stringify(re.map(l => l.slice(60, 64))) + ')');
        if (rd.some(l => l.slice(61, 64).includes('N'))) f.push('the dwarf\'s map: a compass');
        const s = dq0Story(out);
        inTurn(s, ['> face east', '> map', 'The map (you face east', 'Facing E'], f, 'the human: ');
        if (/Sq -?\d+,-?\d+/.test(s.slice(s.indexOf('> face north'), s.indexOf('% dq0 -p t -s 3 -c /ram/d.hm')))) f.push('the human\'s map: its square given');
        if (!/Sq -?\d+,-?\d+/.test(s.slice(s.indexOf('% dq0 -p t -s 3 -c /ram/d.hm')))) f.push('the dwarf\'s map: no square given');
        if (!dk.kept.length || !(dw.kept.length < dk.kept.length)) f.push('the dwarf an hour in the dark: ' + dw.kept.length + ' squares remembered of ' + dk.kept.length + ' (fewer wanted)');
        const kk = new Set(dk.kept.map(q => q.r + ',' + q.i));
        if (dw.kept.some(q => !kk.has(q.r + ',' + q.i))) f.push('the dwarf\'s memory put off where it was (it knows north)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dig', meaning: true, what: 'Dingus Quest Zero digging (the referee\'s: the tools equipped, P_DIG by what\'s dug, Mining tests a spell at a time, HarnMaster\'s; the maker\'s materials: a room\'s floor, then the stratum\'s rock), by the world\'s events: a skilled miner (Mining 95, hmchar\'s file) with a pick (dq0 -k pick -T: the tool the referee takes) digs a shaft down from a room by its stairway, spell by spell, by a lantern\'s light (slice 4.1; the scene set, slice 4.10\'s, as the shaft\'s rock now differs by the referee\'s pace: its oil two hours\' as it\'s first lit, dq0Poke\'s, the shaft longer under either referee, and twelve hours\' as it\'s lit again, the shaft through by it; the light gone, in the dark, the dig stopped, the lantern filled and lit again, and dug on from where it was left), through to the stratum below (tiring as it goes, strenuous work: two fatigue levels a spell, the referee\'s, its Mining tests the harder for them, so more spells), and climbs down it (grown round it then); the giant rats by the stairway living through every spell of it (the turns pace: hours of it, the denizens kept up a slice at a time); a shovel digs seed 1\'s hard-packed earth floor (untested: the tool\'s rate, a foot and a half a spell) till the rock under it stops it (the user: the floor, then the rock); a sword won\'t dig seed 5\'s flagged floor at all (only a pick lifts flags); bare hands (the dagger in the kit, a knife that would dig it, put down first) make no progress in seed 1\'s hard-packed earth (the user\'s, 2026-10-20: hands only in soft soil) (dq0earth\'s and dq0tools\' digs, these together)',
      init: 't_rc', cycles: 3800e6,                // (3600e6 till slice 4.10; 2026-10-31 3415M under HarnMaster's referee, 2721M the null's)
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/m.hm', 'echo ml min 95 >> /ram/m.hm', 'echo ml cli 95 >> /ram/m.hm',
        'dq0 -p t -s 3 -c /ram/m.hm -k pick,lantern -T', 'light the lantern', 'dig down', 'fill the lantern', 'light the lantern', 'dig down', 'd', 'quit', 'dq0 -p t -s 1 -c /ram/c.hm -k shovel -T', 'dig down', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T', 'dig down', 'quit', 'dq0 -p t -s 1 -c /ram/c.hm -T', 'drop the dagger', 'dig down', 'quit', 'echo $status']) },
      start(m) {                                  // (The scene set, slice 4.10's: its lantern's oil two hours' as it's first lit,
        dq0Poke(m, [{ g: 1, line: 'light the lantern', n: 1, fuel: { lantern: 120 } },   //   the light out in the dig's
          { g: 1, line: 'light the lantern', n: 2, fuel: { lantern: 720 } }]);   //   first part, under either referee; filled, twelve
      },                                          //   hours', the shaft through by its light: the rock harder in parts and
                                                  //   poor rock coming down into it, its spells as many as they come)
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], digs = T.filter(t => t.line === 'dig down');
        if (digs.length !== 5) return ['the five digs not typed'];
        const spells = t => dq0Ev(t, 'E_DIG', { how: h => ['D_GOOD', 'D_POOR', 'D_SLIP'].includes(h) }), dug = t => spells(t).filter(e => e.how !== 'D_SLIP').length;
        // (The tools the referee took for the dig's spells, the hub's dig_tool: the one carried among them)
        const tool = (t, n) => { if (!t.after || !t.after.digTool.includes(n)) f.push(t.k + ': the referee\'s tools ' + (t.after && JSON.stringify(t.after.digTool)) + ', not the ' + n); };
        const [p0, p, s1, sw, h1] = digs;
        dq0Need(f, p0, 'E_DIG', { how: 'D_BEGUN' }, 'the pick\'s dig not begun');
        if (!dug(p0)) f.push('the pick dug nothing');
        dq0Need(f, p0, 'E_TASK', { te: 'TE_OUT', task: 'LT_DIG' }, 'the dig not stopped as its light went out');
        dq0Need(f, T.after(p0, 'fill the lantern'), 'E_READY', {}, 'the lantern not filled');
        dq0Need(f, p, 'E_DIG', { how: 'D_AGAIN' }, 'the shaft not dug on from where it was left');
        dq0Need(f, p, 'E_DIG', { how: 'D_THROUGH' }, 'the shaft not through');
        tool(p, 'pick');
        const rats = s => s ? s.actors.filter(a => a.state && a.name === 'giant rat').length : 0;
        if (!rats(p0.at) || rats(p.after) !== rats(p0.at)) f.push('the rats by the stairway not all living through the dig: ' + rats(p0.at) + ', then ' + rats(p.after));
        const d = T.after(p, 'd');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN', first: true }, 'not down the shaft to the stratum below');
        if (!d.after || d.after.pc.sn !== 1) f.push('not on the stratum below');
        if (dug(s1) !== 2) f.push('the shovel dug the earth floor\'s two feet in ' + dug(s1) + ' spells, not 2 (a foot and a half a spell)');
        dq0Need(f, s1, 'E_DIG', { how: 'D_NO_TOOL', into: 1 + 1 }, 'the shovel not stopped by the rock (MAT_STONE)');
        tool(s1, 'shovel');
        dq0Need(f, sw, 'E_DIG', { how: 'D_NO_TOOL' }, 'the sword not refused by the flags');
        if (spells(sw).length) f.push('the sword dug at the flags');
        if (spells(h1).length) f.push('bare hands dug at the hard-packed earth');
        dq0Need(f, h1, 'E_DIG', { how: 'D_NO_TOOL', into: 0 + 1 }, 'bare hands not refused by the hard-packed earth (MAT_EARTH)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dug', meaning: true, what: 'Dingus Quest Zero\'s holes dug part way kept (slice 4.0, HOUSE.md\'s what stopping costs: "a hole half dug stays half dug"; the world\'s change records, DUG: a hole\'s half-feet dug and the way it\'s dug, looked up as a dig begins on its square), by the world\'s events, its change records and its long task: at seed 3 by the stairway (dq0 -T -d, a pick, the character no miner) the wall to the south dug a while and stopped ("stop": its feet dug a DUG record, heading south), then dug again, on from where it was left (E_DIG\'s D_AGAIN, its feet to go the last spell\'s), through; a shaft begun beside the stairway, dug a while, stopped (a DUG record, down); the game kept (quit) and carried on (dq0 -d: the record read back from its region\'s file), the shaft dug again on from where it was left, its bar too (the hub\'s pc_task: its part done the record\'s and more)',
      init: 't_rc', cycles: 1500e6,
      // (A "stop" typed as the dig goes, after a wait: \u0100's 2M cycles each, a few spells of the shaft's, the stratum
      // below grown first; the wall's as its first spell is told, dq0OnWords: its time moved with the code's layout under
      // RULES=null, 2026-10-30, the stop typed before it)
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick,lantern -T -d /ram/dug',
        'light the lantern', 'dig south', 'dig south', 'dig down', '!' + '\u0100'.repeat(30) + 'stop', 'quit',
        'dq0 -d /ram/dug', 'dig down', '!' + '\u0100'.repeat(15) + 'stop', 'quit', 'echo $status']
        .map(l => (l[0] === '!' ? l.slice(1) : '\u0101' + l) + '\r').join('') },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'dig south', n: 1, words: 'You dig with your', text: 'stop\r' }]);   // (Typed as it's told)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], digs = T.filter(t => /^dig /.test(t.line)), stops = T.filter(t => t.line === 'stop');
        if (digs.length !== 4 || stops.length !== 3) return ['the digs and stops not typed: ' + digs.length + ' and ' + stops.length];
        // (A dig and the stop after it as one turn (the wall's second, to its end, its own); its E_DIG events; its
        // spells; the feet to go after its last spell; the hole's record, DUG, after a stop: at the square dug, dy south
        // of where the character stands, dug way d, level.h's DIR_S 2 or DIR_DOWN 5)
        const [s1, s2, d1, d2] = digs.map((t, i) => i === 1 ? t : T.and(t)), [x1, x2, x3] = stops;
        const ev = t => dq0Ev(t, 'E_DIG', {}), says = t => ev(t).map(e => e.how + ' ' + e.feet).join(', ');
        const spells = t => ev(t).filter(e => ['D_GOOD', 'D_POOR', 'D_SLIP'].includes(e.how));
        const togo = t => { const s = spells(t); return s.length ? s[s.length - 1].feet : -1; };
        const hole = (t, dy, d) => (t.after ? t.after.changes : []).filter(c => c.what === 'DUG' && c.sn === t.after.pc.sn &&
          c.x === t.after.pc.x && c.y === t.after.pc.y + dy && c.elev === d)[0];
        const part = (t, x, d, dy, what) => {             // (Dug a while and stopped: its record, the half-feet dug,
                                                          //   the last spell's feet to go)
          const b = ev(t).find(e => e.how === 'D_BEGUN' || e.how === 'D_AGAIN');
          if (!spells(t).some(e => e.how !== 'D_SLIP')) f.push(what + ' dug nothing before it was stopped (' + says(t) + ')');
          if (ev(t).some(e => e.how === 'D_THROUGH')) f.push(what + ' through before it was stopped');
          dq0Need(f, x, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_DIG' }, what + ' not stopped');
          const h = hole(x, dy, d);
          if (!h || !h.tile) { f.push(what + ': no record of its hole after the stop (' + JSON.stringify(x.after && x.after.changes.filter(c => c.what === 'DUG')) + ')'); return 0; }
          if (b && b.how === 'D_BEGUN' && (2 * b.feet - h.tile + 1) >> 1 !== togo(t)) f.push(what + ': its record\'s ' + h.tile + ' half-feet not the ' + togo(t) + ' feet to go of ' + b.feet + ' (' + says(t) + ')');
          return h.tile;
        };
        // (The wall to the south: two feet, dug a while, stopped; dug again, on from where it was left, through)
        dq0Need(f, s1, 'E_DIG', { how: 'D_BEGUN', feet: 2 }, 'the wall\'s dig not begun (two feet)');
        part(s1, x1, 2, 1, 'the wall');
        dq0Need(f, s2, 'E_DIG', { how: 'D_AGAIN', feet: togo(s1) }, 'the wall not dug again from where it was left, ' + togo(s1) + ' feet to go');
        dq0Never(f, s2, 'E_DIG', { how: 'D_BEGUN' }, 'the wall\'s dig begun afresh');
        dq0Need(f, s2, 'E_DIG', { how: 'D_THROUGH' }, 'the wall not dug through');
        // (The shaft: dug a while, stopped, the game kept; carried on, dug again from where it was left, its bar too)
        const b = ev(d1).find(e => e.how === 'D_BEGUN'), h = part(d1, x2, 5, 0, 'the shaft');
        if (!b) f.push('the shaft not begun (' + says(d1) + ')');
        dq0Need(f, d2, 'E_DIG', { how: 'D_AGAIN', feet: togo(d1) }, 'the shaft not dug again from where it was left, carried on (dq0 -d), ' + togo(d1) + ' feet to go');
        dq0Never(f, d2, 'E_DIG', { how: 'D_BEGUN' }, 'the shaft begun afresh, carried on');
        const k = x3.at && x3.at.task;
        if (b && h && (!k || k.kind !== 'LT_DIG' || k.len !== 2 * b.feet || k.done < h)) f.push('the shaft\'s bar not on from where it was left: ' + JSON.stringify(k) + ', its record ' + h + ' half-feet of ' + 2 * b.feet);
        // (The UI's words for it, both times: the UI's own, whatever the referee)
        const again = dq0Story(out).match(/You set to digging again where you left off\. \((?:A foot|\d+ feet) to go\.\)/g) || [];
        if (again.length !== 2) f.push('"You set to digging again where you left off." said ' + again.length + ' times, not 2');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rock', near: ['map'], what: 'Dingus Quest Zero\'s rock (slice 4.10: DUNGEON.md\'s changing the dungeon, HOUSE.md\'s rock\'s kinds and hardness; dq0/rock.h, the world\'s dig_rock, the UI\'s words), by the words told: a skilled miner (Mining 95) with a pick at seed 3 by the stairway (dq0 -T) digs a shaft down, through the floor\'s flags (a spell) into the rock; the rock\'s told as the dig comes to it, after the floor ("You\'re into flaky rock", its looks one of the rocks\'), named only after a spell that went well ("...: shale." with "The work goes well."); stopped as it\'s told ("stop"), then quit',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml min 95 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -k pick,lantern -T', 'light the lantern', 'dig down', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'dig down', n: 1, words: 'You\'re into', text: 'stop\r' }]);   // (Typed as it's told)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out), first = s.indexOf('You\'re into'), begun = s.lastIndexOf('You set to digging', first);
        const looks = { shale: 'flaky', sandstone: 'buff', limestone: 'pale grey', slate: 'blue-grey', marble: 'white', granite: 'speckled grey',
          basalt: 'black', quartz: 'white-veined', ironstone: 'rusty red', 'copper ore': 'green-stained' };
        const told = [...s.matchAll(/You're into ([a-z -]+?) rock(?:: ([a-z ]+))?\. (?:You dig with your [a-z]+\. )?(The work goes well\.|The work goes slowly\.|Your tool slips|The rotten rock)/g)];
        if (!told.length) return ['the rock not told as the dig came to it: ' + (s.match(/dig down.{0,600}/) || [''])[0]];
        for (const [all, look, name, how] of told) {
          if (!Object.values(looks).includes(look)) f.push('"' + all + '": not a rock\'s looks');
          if (name && looks[name] !== look) f.push('"' + all + '": ' + name + ' doesn\'t look ' + look);
          if (!!name !== (how === 'The work goes well.')) f.push('"' + all + '": named ' + (name ? '' : 'not ') + 'after a spell that went ' + (name ? 'badly' : 'well'));
        }
        if (begun < 0) f.push('the rock told before the dig began');
        else if (!/The work goes|Your tool slips/.test(s.slice(begun, first))) f.push('the rock told on the dig\'s first spell, the floor\'s flags not dug first: ' + s.slice(begun, first + 80));
        if (!/You dig with your pick\./.test(s.slice(begun, first + 200))) f.push('the pick not named on the first spell');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rubble', near: ['map'], what: 'Dingus Quest Zero\'s age\'s collapse (slice 4.10: DUNGEON.md\'s rock and old places, HOUSE.md\'s rock\'s kinds and hardness; the maker\'s rubble, caves.c\'s, by the stratum\'s age, twice where the rock is poor; world.h\'s T_RUBBLE), by the hub\'s cache: at seed 3 down the stairway (dq0 -T, d), the chunks of both strata read: rubble in old passages (seed 3\'s nine squares: the top stratum\'s chunk 3,1 and the caves\' 18,11 and 18,12), every square of it a passage\'s, in no room\'s bounds, every square beside it a passage\'s, the rock\'s or rubble (no doorway, no room\'s floor), never on a chunk\'s edge',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'd', 'map', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const f = [], ok = new Set([T_CORR, T_ROCK, T_WALL, 10]);
        let n = 0;
        for (const sn of [0, 1]) for (const [at, c] of cacheChunks(m, sn)) for (let i = 0; i < 1024; i++) {
          if (c.tile[i] !== 10) continue;
          n++;
          const x = i & 31, y = i >> 5;
          if (!x || !y || x === 31 || y === 31) f.push(sn + ':' + at + ': rubble on the chunk\'s edge at ' + x + ',' + y);
          else if (![-32, 1, 32, -1].every(d => ok.has(c.tile[i + d]))) f.push(sn + ':' + at + ': rubble at ' + x + ',' + y + ' beside a room\'s square or a doorway');
          if (c.rooms.some(r => x + 1 >= r.x && x <= r.x + r.w && y + 1 >= r.y && y <= r.y + r.h)) f.push(sn + ':' + at + ': rubble at ' + x + ',' + y + ' in a room\'s bounds');
        }
        if (n < 5) f.push('only ' + n + ' squares of rubble in the cache (seed 3\'s: 9)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0mixed', near: ['map'], what: 'Dingus Quest Zero\'s mixed strata (slice 4.10: DUNGEON.md\'s growing a chunk, HOUSE.md\'s rock\'s kinds and hardness; the maker\'s cstyle), by the hub\'s cache: at seed 3 with every built stratum made mixed (dq0 -M, the tests\'), by the stairway (dq0 -T) and down it, the top stratum\'s chunks read: part natural (seed 3\'s 3,0 and 3,1: caves, every room a cave\'s chamber) and part built (2,0 and 2,1, the stairway\'s: no cave in them), and where a natural chunk meets a built one their edges\' open squares meet; the stratum below (natural) all caves',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -M', 'd', 'map', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const f = [], ks = cacheChunks(m, 0), open = t => t !== T_ROCK && t !== T_WALL && t !== T_WINDOW;
        const natural = c => c.rooms.length > 0 && c.rooms.every(r => r.kind === 6), built = c => !c.rooms.some(r => r.kind === 6);
        const nat = [...ks].filter(([, c]) => natural(c)).map(([at]) => at), bui = [...ks].filter(([, c]) => built(c)).map(([at]) => at);
        if (!nat.length || !bui.length) f.push('the top stratum not part natural, part built: natural ' + nat.join(' ') + ', built ' + bui.join(' '));
        for (const [at, c] of ks) for (const [dx, dy] of [[1, 0], [0, 1]]) {
          const d = ks.get((c.cx + dx) + ',' + (c.cy + dy));
          if (!d || natural(c) === natural(d)) continue;
          for (let k = 0; k < 32; k++) {
            const a = dx ? c.tile[k * 32 + 31] : c.tile[31 * 32 + k], b = dx ? d.tile[k * 32] : d.tile[k];
            if (open(a) !== open(b)) f.push(at + ': its ' + (dx ? 'east' : 'south') + ' edge\'s square ' + k + ' not met by its neighbour\'s (natural and built)');
          }
        }
        if (![...cacheChunks(m, 1).values()].every(natural)) f.push('the stratum below (natural) not all caves');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0angles', near: ['map'], what: 'Dingus Quest Zero\'s diagonal ways (the user\'s, 2026-10-31: passages off the compass, step 1; HOUSE.md\'s diagonal ways), by the words told and the map: at seed 3 in the cell by the stairway (dq0 -T) a skilled miner (Mining 95) with a pick faces north, goes to the far right corner and digs northeast (the cell\'s corner wall, two feet: "You break through."), the square corner to corner a passage, its two corners cut (T_CUT); "go northeast" walks into it (a diagonal way from a room\'s floor: across\'s mouth), its end told ("The passage ends here; it goes back behind you."); the map draws the two cut corners as strokes along the way ("/" beside the @ and below it, no "\\"); "look southwest" sees back along the diagonal way into the cell (a look one way\'s, way.c\'s seen_on: "Down the passage ahead you see into the cell", 2026-11-01); "go southwest" walks back into the cell',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/m.hm', 'echo ml min 95 >> /ram/m.hm',
        'dq0 -p t -s 3 -c /ram/m.hm -k pick -T', 'face north', 'go to the far right corner', 'dig northeast', 'go northeast', 'map', 'look', 'look southwest', 'go southwest', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        const at = l => s.indexOf('> ' + l + ' > ' + l);
        const seg = (l, n) => at(l) >= 0 ? s.slice(at(l), at(n) > at(l) ? at(n) : undefined) : '';
        if (!seg('dig northeast', 'go northeast').includes('You break through.')) f.push('the dig northeast not through');
        const go = seg('go northeast', 'map');
        if (!go.includes('The passage ends here; it goes back behind you.')) f.push('"go northeast" not into the dug square: ' + go.slice(0, 200));
        const raw = out.replace(/\x1b\[[0-9;]*m/g, ''), m0 = raw.indexOf('The map (you face northeast');
        const map = m0 < 0 ? '' : raw.slice(m0, raw.indexOf('> look', m0)), rows = map.split('\n'), i = rows.findIndex(r => r.includes('@'));
        if (i < 0) f.push('no map');
        else {
          const c = rows[i].indexOf('@');
          if (rows[i][c - 1] !== '/' || !rows[i + 1] || rows[i + 1][c] !== '/') f.push('the cut corners not drawn "/" beside the @ and below it: ' + rows.slice(i - 1, i + 2).join(' | '));
          if (map.includes('\\')) f.push('a "\\" stroke on the map');
        }
        if (!seg('look southwest', 'go southwest').includes('Down the passage ahead you see into the cell')) f.push('"look southwest" not along the diagonal way into the cell (way.c seen_on): ' + seg('look southwest', 'go southwest').slice(0, 200));
        if (!seg('go southwest', 'quit').includes('You can see part of a cell')) f.push('"go southwest" not back into the cell');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0heading', near: ['map'], what: 'Dingus Quest Zero\'s passages told by their real heading (the user\'s, 2026-10-31: ways off the compass, step 2; HOUSE.md\'s a passage\'s heading), by the words told and the map: at seed 3 down the stairway (dq0 -T, the caves dry and unfallen: -W -U) a staircase of short steps (south, then east and south by turns, eight squares: one square wide, not cut) made in the rock from the cave\'s wall two squares south and two west of the stairs as "look" is typed (dq0CarveStairs: the chunk cache\'s squares, the senses\' grid made again); two steps west, "go south" walks down it to its end without a turn told at every step (one bend at most, "The passage bends away southeast.", and no "The passage turns"); "look" at its end tells the way back by its shape, behind and to the left (its run\'s bearing as far as it\'s seen, northwest, the steps by turns, as it faces east: "it goes back behind you to your left"); the map draws what it shows of it as a slant (strokes of one kind, two or more, as the map\'s turned); "turn around" and "go ahead" walk back up it to the cave',
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -W -U', 'd', 'look', 'face west', 'step forward', 'step forward', 'go south', 'look', 'map', 'turn around', 'go ahead', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) { this.carve = dq0CarveStairs(m, { g: 1, sn: 1, line: 'look', n: 1, at: [-2, 2], steps: [2, 2, 1, 2, 1, 2, 1, 2, 1] }); },
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        if (!this.carve.done) return ['the staircase not made: ' + this.carve.why];
        const at = l => s.indexOf('> ' + l + ' > ' + l);
        const seg = (l, n) => at(l) >= 0 ? s.slice(at(l), at(n) > at(l) ? at(n) : undefined) : '';
        const go = seg('go south', 'look');
        if (/The passage turns/.test(go)) f.push('"go south" told a turn at its steps: ' + go.slice(0, 300));
        if ((go.match(/The passage bends away/g) || []).length > 1) f.push('"go south" told more than one bend: ' + go.slice(0, 300));
        if (/bends away/.test(go) && !go.includes('The passage bends away southeast.')) f.push('its bend not southeast: ' + go.slice(0, 300));
        if (!/The passage ends here; it goes back/.test(go)) f.push('"go south" not to the staircase\'s end: ' + go.slice(0, 300));
        const lk = s.slice(s.lastIndexOf('> look > look'), at('map'));
        if (!lk.includes('The passage ends here; it goes back behind you to your left.')) f.push('the look at its end not "it goes back behind you to your left": ' + lk.slice(0, 200));
        const raw = out.replace(/\x1b\[[0-9;]*m/g, ''), m0 = raw.indexOf('The map (you face');
        const map = m0 < 0 ? '' : raw.slice(m0, raw.indexOf('> turn around', m0)), rows = map.split('\n').map(r => r.slice(0, 64));
        const all = rows.join(''), back = all.split('\\').length - 1, fore = all.split('/').length - 1;
        if (Math.max(back, fore) < 2 || Math.min(back, fore) > 0) f.push('the staircase not drawn as a slant, two strokes or more of one kind (' + back + ' "\\", ' + fore + ' "/"): ' + rows.filter(r => r.trim()).slice(1, 14).join(' | '));
        if (!seg('go ahead', 'quit').includes('cave')) f.push('"go ahead" not back up into the cave: ' + seg('go ahead', 'quit').slice(0, 200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0slant', near: ['map'], what: 'Dingus Quest Zero\'s built passages on the diagonal (the user\'s, 2026-10-31: ways off the compass, step 3; HOUSE.md\'s built passages on the diagonal), by the chunks made and the words told: at seed 3 every built stratum\'s passages on the diagonal (dq0 -A, the tests\'), the dungeon made and entered as ever (its first look told); the chunks the cache has of its top stratum have rooms and passages with cut corners (T_CUT, three or more), each between two of a passage\'s squares that share only a corner with it on either side (a diagonal way\'s, chunk.c\'s slant: a staircase cut as the caves\' are, caves.c\'s slants), none in a room\'s bounds; "go east" from the kitchen by the way in walks a passage on the diagonal, told as it bends ("The passage bends away northeast.", HOUSE.md\'s a passage\'s heading), to its end; the map draws a cut corner ("/" or "\\")',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -A', 'look', 'go through the door to the east', 'map', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        if (!/You (can see|are in)/.test(s)) f.push('no first look told');
        const i = s.indexOf('> go through the door to the east > go through the door to the east'), go = i < 0 ? '' : s.slice(i, s.indexOf('> map', i));
        if (!go.includes('The passage bends away northeast.') || !/The passage (comes to a door|ends here|forks here)|You are in a passage/.test(go)) f.push('"go east" not along a passage on the diagonal to its end ("The passage bends away northeast."): ' + go.slice(0, 300));
        const raw = out.replace(/\x1b\[[0-9;]*m/g, ''), m0 = raw.indexOf('The map (you face');
        if (m0 < 0 || !/[/\\]/.test(raw.slice(m0, raw.indexOf('> quit', m0)))) f.push('no cut corner drawn on the map ("/" or "\\")');
        let cuts = 0, bad = 0;
        for (const c of cacheChunks(m, 0).values()) {
          if (!c.rooms.length) continue;
          const t = (x, y) => x < 0 || y < 0 || x > 31 || y > 31 ? 0 : c.tile[y * 32 + x];
          const pass = v => v === T_CORR || v === 3 || v === 5 || v === 10;
          for (let y = 0; y < 32; y++) for (let x = 0; x < 32; x++) {
            if (t(x, y) !== 11) continue;
            cuts++;
            const inRoom = c.rooms.some(r => x >= r.x - 1 && x <= r.x + r.w && y >= r.y - 1 && y <= r.y + r.h);
            const diag = (pass(t(x - 1, y)) && pass(t(x, y + 1))) || (pass(t(x + 1, y)) && pass(t(x, y - 1))) ||
              (pass(t(x - 1, y)) && pass(t(x, y - 1))) || (pass(t(x + 1, y)) && pass(t(x, y + 1)));
            if (inRoom || !diag) bad++;
          }
        }
        if (cuts < 3) f.push('fewer than three cut corners in the top stratum\'s built chunks: ' + cuts);
        if (bad) f.push(bad + ' cut corner(s) in a room\'s bounds or not between two of a passage\'s squares');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0cavern', near: ['map'], what: 'Dingus Quest Zero\'s great caverns (slice 4.10\'s rest, 2026-11-02: DUNGEON.md\'s caverns across many chunks, HOUSE.md\'s great caverns; the maker\'s caves.c cavern, overlay 8), by the hub\'s cache and the words told: at seed 3 with every natural region a cavern (dq0 -V, the tests\'), down the stairway (dq0 -T) into the caves, the stairs\' foot in the cavern (its middle by the region\'s way): its parts in three or more of the cached chunks (world.h\'s R_CAVERN rooms), and where a part\'s floor is at a chunk\'s edge the next chunk\'s square open (its floor, a step\'s rise or fall at most) or a wall, never the bare rock (the parts grown from the seed alone, meeting: seed 3\'s at 8 squares or more); told "You are somewhere in a vast cavern, its walls beyond your light." as it comes in, and "You are in a vast cavern, by the light of your torch" with its walls in sight; "go north" and "go east" walk across it over the chunks\' edges (seed 3\'s 19,12 to 19,11, 19,11 to 20,11); the map drawn; the same game again, walked the other way about: every part in both the same, square for square and floor for floor (made from the seed alone)',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -V', 'd', 'look', 'map', 'go north', 'look', 'go east', 'look', 'map', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -V', 'd', 'go east', 'go north', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) { this.snaps = []; dq0Where(this, m, l => { if (l === 'quit') this.snaps.push(cavernChunks(m, 1)); }); },
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        const S = stratumRec(m, 1), ks = this.snaps[0] || new Map(), CAV = 0x10, open = t => t !== T_ROCK && t !== T_WALL && t !== T_WINDOW;
        const parts = [...ks.values()].filter(c => c.rooms.some(r => r.flags & CAV));
        const floorOf = (c, x, y) => c.tile[y * 32 + x] === T_FLOOR && c.rooms.some(r => (r.flags & CAV) && x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
        if (parts.length < 3) f.push('a great cavern\'s parts in ' + parts.length + ' of the cached chunks of the stratum below, not 3 or more');
        let met = 0;
        for (const c of parts) for (const [dx, dy] of [[1, 0], [0, 1], [-1, 0], [0, -1]]) {
          const d = ks.get((c.cx + dx) + ',' + (c.cy + dy));
          if (!d || !d.elev || !c.elev) continue;
          for (let k = 0; k < 32; k++) {
            const ax = dx > 0 ? 31 : dx < 0 ? 0 : k, ay = dy > 0 ? 31 : dy < 0 ? 0 : k, bx = dx > 0 ? 0 : dx < 0 ? 31 : k, by = dy > 0 ? 0 : dy < 0 ? 31 : k;
            const a = ax + ay * 32, b = bx + by * 32;
            if (floorOf(c, ax, ay)) {
              if (open(d.tile[b])) ++met;
              if (d.tile[b] === T_ROCK) f.push(c.cx + ',' + c.cy + ': its cavern floor at the edge (' + ax + ',' + ay + ') meets the bare rock in ' + d.cx + ',' + d.cy);
              else if (open(d.tile[b]) && Math.abs(c.elev[a] - d.elev[b]) > 2) f.push(c.cx + ',' + c.cy + ': a ledge at the edge (' + ax + ',' + ay + '): ' + c.elev[a] + ' to ' + d.elev[b]);
            } else if (c.tile[a] === T_ROCK && floorOf(d, bx, by)) f.push(c.cx + ',' + c.cy + ': rock, not a wall, at the edge (' + ax + ',' + ay + ') beside ' + d.cx + ',' + d.cy + '\'s cavern floor');
          }
        }
        if (met < 8) f.push('the cavern\'s parts meet at only ' + met + ' squares of the chunks\' edges');
        let same = 0;
        for (const [at, c] of ks) {
          const d = this.snaps[1] && this.snaps[1].get(at);
          if (!d || !d.elev || !c.elev || !c.rooms.some(r => r.flags & CAV)) continue;
          ++same;
          if (c.tile.join() !== d.tile.join() || c.elev.join() !== d.elev.join()) f.push(at + ': its part not the same, made in another order');
        }
        if (same < 2) f.push('only ' + same + ' of the cavern\'s parts in both games');
        const at = l => s.indexOf('> ' + l + ' > ' + l);
        const seg = (l, n) => at(l) >= 0 ? s.slice(at(l), at(n) > at(l) ? at(n) : undefined) : '';
        if (!seg('d', 'look').includes('You are somewhere in a vast cavern, its walls beyond your light.')) f.push('not "somewhere in a vast cavern" as it comes in: ' + seg('d', 'look').slice(0, 300));
        if (!s.includes('You are in a vast cavern, by the light of your torch')) f.push('never "You are in a vast cavern, by the light of your torch" with its walls in sight');
        const W = this.where || [], ch = w => S ? ((w.x - S.x0) >> 5) + ',' + ((w.y - S.y0) >> 5) : '?';
        for (const [l, from, to] of [['go north', '19,12', '19,11'], ['go east', '19,11', '20,11']]) {
          const i = W.findIndex(w => w.line === l);
          if (i < 0 || !W[i + 1] || ch(W[i]) !== from || ch(W[i + 1]) !== to) f.push('"' + l + '" not across the cavern from chunk ' + from + ' to ' + to + ': ' + (i < 0 || !W[i + 1] ? 'no squares' : ch(W[i]) + ' to ' + ch(W[i + 1])) + ': ' + seg(l, 'look').slice(0, 200));
        }
        const raw = out.replace(/\x1b\[[0-9;]*m/g, '');
        if (raw.indexOf('The map (you face') < 0) f.push('no map drawn');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0ramp', near: ['map'], what: 'Dingus Quest Zero\'s long slopes between strata (slice 4.10\'s rest, 2026-11-02: raises and inclines; DUNGEON.md\'s openings and ways between strata, HOUSE.md\'s long slopes; level.h\'s O_RAMP_DOWN, O_RAMP_UP), by the words told and the strata\'s records: at seed 3 with every stairway down a long slope (dq0 -P, the tests\') by the top stratum\'s way down (dq0 -T): "look" tells "a long slope leading down" beside it; "d" walks down it ("You walk down the long slope"), into the stratum below at its foot (a long slope leading up, the same square: the top stratum\'s link down O_RAMP_DOWN, the one below\'s up O_RAMP_UP); "u" walks back up it ("You trudge up the long slope")',
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -P', 'look', 'd', 'look', 'map', 'u', 'look', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        const at = l => s.indexOf('> ' + l + ' > ' + l);
        const seg = (l, n) => at(l) >= 0 ? s.slice(at(l), at(n) > at(l) ? at(n) : undefined) : '';
        if (!seg('look', 'd').includes('a long slope leading down')) f.push('"look" not by "a long slope leading down": ' + seg('look', 'd').slice(0, 300));
        if (!seg('d', 'look').includes('You walk down the long slope')) f.push('"d" not "You walk down the long slope": ' + seg('d', 'look').slice(0, 300));
        if (!s.includes('a long slope leading up')) f.push('below, never "a long slope leading up"');
        if (!seg('u', 'look').includes('You trudge up the long slope')) f.push('"u" not "You trudge up the long slope": ' + seg('u', 'look').slice(0, 300));
        const O_RAMP_UP = 8, O_RAMP_DOWN = 9, a = stratumRec(m, 0), b = stratumRec(m, 1);
        const dn = a && a.links.filter(l => l.kind === O_RAMP_DOWN && l.to === 1), up = b && b.links.filter(l => l.kind === O_RAMP_UP && l.to === 0);
        if (!dn || !dn.length) f.push('no long slope down in the top stratum\'s links: ' + JSON.stringify(a && a.links));
        else if (!up || !dn.every(d => up.some(u => u.x === d.x && u.y === d.y))) f.push('the long slopes down not each over a long slope up below: ' + JSON.stringify(b && b.links));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0diamond', near: ['map'], what: 'Dingus Quest Zero\'s rooms at 45 degrees (the user\'s, 2026-10-31: ways off the compass, step 3\'s rest; HOUSE.md\'s rooms on the diagonal; the maker\'s caves.c diamonds, overlay 8), by the chunks made and the words told: at seed 3 with every built stratum\'s rooms on the diagonal (dq0 -D, the tests\'), the dungeon made and entered as ever (its first look told); in the chunks the cache has of the top stratum, every room four squares or more each way that no way between strata comes out in cut to its diamond (no floor in its box\'s four corners, its walls there), its floor still there (half its box or more of the diamond\'s), every doorway in its old walls beside its floor still; "go east" walks out of the room it begins in',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -D', 'look', 'go through the door to the east', 'map', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[[.#]{10}\] /g, ''), S = stratumRec(m, 0), T_DOOR = 3;
        if (!/You (can see|are in)/.test(s)) f.push('no first look told');
        let cut = 0, corners = 0;
        for (const c of cacheChunks(m, 0).values()) {
          const t = (x, y) => x < 0 || y < 0 || x > 31 || y > 31 ? 0 : c.tile[y * 32 + x];
          for (const r of c.rooms) {
            if (r.w < 4 || r.h < 4 || r.kind === 6) continue;
            const landing = S && S.links.some(l => { const x = l.x - S.x0 - c.cx * 32, y = l.y - S.y0 - c.cy * 32; return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; });
            if (landing) continue;
            const at = c.cx + ',' + c.cy + ' room ' + r.x + ',' + r.y + ' ' + r.w + 'x' + r.h;
            for (const [x, y] of [[r.x, r.y], [r.x + r.w - 1, r.y], [r.x, r.y + r.h - 1], [r.x + r.w - 1, r.y + r.h - 1]]) { ++corners; cut += t(x, y) !== T_FLOOR; }
            let fl = 0;
            for (let y = r.y; y < r.y + r.h; y++) for (let x = r.x; x < r.x + r.w; x++) fl += t(x, y) === T_FLOOR;
            if (fl * 3 < r.w * r.h) f.push(at + ': only ' + fl + ' squares of floor');
            for (let x = r.x; x < r.x + r.w; x++) for (const [y, dy] of [[r.y - 1, 1], [r.y + r.h, -1]]) if (t(x, y) === T_DOOR && t(x, y + dy) !== T_FLOOR && t(x, y + dy) !== 5) f.push(at + ': its door at ' + x + ',' + y + ' not beside its floor');
            for (let y = r.y; y < r.y + r.h; y++) for (const [x, dx] of [[r.x - 1, 1], [r.x + r.w, -1]]) if (t(x, y) === T_DOOR && t(x + dx, y) !== T_FLOOR && t(x + dx, y) !== 5) f.push(at + ': its door at ' + x + ',' + y + ' not beside its floor');
          }
        }
        if (!corners || cut * 4 < corners * 3) f.push('of the top stratum\'s cached rooms\' box corners only ' + cut + ' of ' + corners + ' cut (three in four or more: those an opening\'s kept, not)');
        const i = s.indexOf('> go through the door to the east > go through the door to the east'), go = i < 0 ? '' : s.slice(i, s.indexOf('> map', i));
        if (!/You go through a doorway|You cross the room|passage|You are in/.test(go)) f.push('"go east" not out of its room: ' + go.slice(0, 300));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0incline', near: ['map'], what: 'Dingus Quest Zero\'s sloping tunnels (slice 4.10\'s rest, 2026-11-02: raises and inclines; HOUSE.md\'s moving and tiring; the maker\'s caves.c incline, overlay 8), by the chunks made and the words told: at seed 3 with every chunk that has room given one (dq0 -P, the tests\'; dry and unfallen, -W -U), down the stairway (dq0 -T) into the caves, the chunks\' sloping tunnels a straight way of 10 to 16 squares of passage, a foot a square, off a passage, a chamber of its own at its end (seed 3\'s 18,12: rising south), every step of them a walk\'s, the character set at its foot (the hub\'s square, the scene\'s) as "look" is typed; "go south" walks up it, told "The way rises." (two or more, a 4-foot rise at a time), its walk not stopped by a ledge, into the chamber ("cave"); "go north" walks back down it, told "The way falls away."',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -P -W -U', 'd', 'wait', 'look', 'go south', 'look', 'map', 'go north', 'look', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        this.runs = [];
        let done = false;
        const runs = () => {
          const out = [], S = stratumRec(m, 1);
          for (const c of cavernChunks(m, 1).values()) {
            const t = (x, y) => x < 0 || y < 0 || x > 31 || y > 31 ? 0 : c.tile[y * 32 + x], e = (x, y) => c.elev[y * 32 + x];
            for (let y = 0; y < 32; y++) for (let x = 0; x < 32; x++) for (const [dx, dy] of [[1, 0], [0, 1], [-1, 0], [0, -1]]) {
              if (t(x, y) !== T_CORR || t(x + dx, y + dy) !== T_CORR) continue;
              const d = e(x + dx, y + dy) - e(x, y);
              if (Math.abs(d) !== 1 || (t(x - dx, y - dy) === T_CORR && e(x, y) - e(x - dx, y - dy) === d)) continue;
              let n = 1;
              while (t(x + (n + 1) * dx, y + (n + 1) * dy) === T_CORR && e(x + (n + 1) * dx, y + (n + 1) * dy) - e(x + n * dx, y + n * dy) === d) n++;
              if (n >= 9) out.push({ at: c.cx + ',' + c.cy, x: S.x0 + c.cx * 32 + x, y: S.y0 + c.cy * 32 + y, dx, dy, d, n });
            }
          }
          return out;
        };
        dq0Where(this, m, l => {
          if (l !== 'look' || done) return;
          done = true;
          this.runs = runs();
          const r = this.runs.find(q => q.dy === 1 && q.d === 1);
          if (!r) return;
          const { L } = m.dq0.layout(), h = m.dq0.bank(0), w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          w16(L.pcX, r.x);
          w16(L.pcY, r.y);
        });
      },
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        if (!this.runs.some(r => r.n >= 10 && r.n <= 16)) f.push('no sloping tunnel of 10 to 16 squares in the stratum\'s cached chunks: ' + JSON.stringify(this.runs));
        const at = (l, from = 0) => s.indexOf('> ' + l + ' > ' + l, from), i = at('go south'), j = at('go north');
        const up = i < 0 ? '' : s.slice(i, at('look', i)), down = j < 0 ? '' : s.slice(j, at('look', j));
        if ((up.match(/The way rises\./g) || []).length < 2) f.push('"go south" not told "The way rises." twice or more: ' + up.slice(0, 400));
        if (/ledge|falls away/.test(up)) f.push('"go south" not walked: ' + up.slice(0, 400));
        if (!/cave/.test(s.slice(i, j))) f.push('"go south" not into its chamber: ' + s.slice(i, j).slice(0, 500));
        if (!/The way falls away\./.test(down)) f.push('"go north" not told "The way falls away.": ' + down.slice(0, 400));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rockwall', near: ['map'], what: 'Dingus Quest Zero\'s rock looked at (slice 4.10: "examine the wall", the world\'s wall_look, I_TRACE\'s TRC_WALL; HOUSE.md\'s rock\'s kinds and hardness), by the words told: at seed 3 by the stairway (dq0 -T) a cell\'s wall ahead masonry ("The wall\'s masonry"); down the stairway (natural caves) a skilled miner (Mining 95) examines the wall ahead and round it (the last "look at the wall", the verbs\' walllook), the bare rock, how it looks, and named (its Mining test, the appraisal\'s AP_ROCK: "The wall\'s the bare rock, buff: sandstone."), the name always the looks\' rock\'s',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml min 95 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'examine the wall', 'd', 'examine the wall', 'turn left', 'examine the wall', 'turn left', 'look at the wall', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out);
        const looks = { shale: 'flaky', sandstone: 'buff', limestone: 'pale grey', slate: 'blue-grey', marble: 'white', granite: 'speckled grey',
          basalt: 'black', quartz: 'white-veined', ironstone: 'rusty red', 'copper ore': 'green-stained' };
        const said = [...s.matchAll(/(?:examine|look at) the wall (?:You look at the wall\. )?(The wall's masonry|The wall's the bare rock, ([a-z -]+?)(?:: ([a-z ]+))?\.|It's too dark|There's no wall)/g)];
        if (said.length !== 4) return ['the walls not told, four times: ' + said.map(x => x[1]).join(' / ')];
        if (said[0][1] !== 'The wall\'s masonry') f.push('the cell\'s wall not masonry: ' + said[0][0]);
        for (const [all, w, look, name] of said.slice(1)) {
          if (!look) { f.push('a cave\'s wall not the bare rock: ' + all); continue; }
          if (!Object.values(looks).includes(look)) f.push('"' + all + '": not a rock\'s looks');
          if (name && looks[name] !== look) f.push('"' + all + '": ' + name + ' doesn\'t look ' + look);
        }
        if (!said.slice(1).some(x => x[3])) f.push('the miner never named the rock');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0nodig', outfit: true, meaning: true, near: ['map', 'gear'], what: "Dingus Quest Zero's digs told why (the user's playtest, 2026-11-02, DECISIONS.md; HOUSE.md's digging, for review), by poked scenes at seed 374's cell, a closed door to the east of the start (the character by a stairway, facing south).  A hidden door (its portal's state poked in the chunks' cache, PS_HIDDEN, as its first look's typed): the dig takes it for a wall.  With no tool that bites (a knife; then none): told at once, no time spent, by what it carries (E_DIG D_NO_TOOL; 'Your knife only scratches', 'You need better tools'), the door still hidden, no refusal saying there's nothing to dig.  A chisel alone: 'You'd need a hammer to drive the chisel.'  A pick: begun as a wall's two feet, and a spell or so on the facing's off, the door found (D_FOUND: 'strikes wood', its PORTAL_STATE record no longer hidden), seen in the look after.  A hammer and a chisel: the wall west of the start worked at slowly, a stone's half foot a spell (D_GOOD or D_POOR, or asked).  A door in sight: D_PORTAL 'That's a door'; a portcullis (seed 1's guardroom, dq0tunnel's) 'Those are iron bars'.  A door hidden by magic: a pick makes no mark, and after two spells with nothing to show it's asked, soft ('This isn't getting anywhere'), no door told",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 374 -c /ram/c.hm -T', 'look', 'dig east', 'drop the knife', 'dig east', 'quit',
        'dq0 -p t -s 374 -c /ram/c.hm -k chisel -T', 'look', 'dig east', 'quit',
        'dq0 -p t -s 374 -c /ram/c.hm -k pick -T', 'look', 'wait', 'dig east', 'look', 'dig east', 'quit',
        'dq0 -p t -s 374 -c /ram/c.hm -k chisel,hammer -T', 'drop the torch', 'hold the chisel', 'hold the hammer', 'dig north', 'quit',
        'dq0 -p t -s 374 -c /ram/c.hm -k pick -T', 'look', 'dig east', 'quit',
        'mkdir /ram/g', "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -k pick -T -d /ram/g -R 7',
        'open the door', 'south', 'turn around', 'close the door', 'dig north', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 5, line: 'dig east', n: 1, words: 'anywhere', text: 'stop\r' }, { g: 4, line: 'dig north', n: 1, words: 'You dig with your chisel', text: 'stop\r' }]);   // (Typed as it's told; the chisel's first spell enough)
        const a = m.acia, read = a.read;
        let typed = '', g = 0;
        m.door = {};
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (/^dq0 /.test(typed)) ++g;
              if (typed === 'look' && m.dq0 && !m.door[g]) {   // (The door east of the start hidden, by magic in the last game)
                const { L } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
                const x = u16(L.pcX) + 1, y = u16(L.pcY), p = dq0Portal(m, h[L.pcSn], x, y);
                if (p) { p.b[p.state] = g === 5 ? 0x60 : 0x20; m.door[g] = { x, y, sn: h[L.pcSn] }; }
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), digs = T.filter(t => /^dig /.test(t.line));
        if ([1, 2, 3, 5].some(g => !m.door[g])) return ['the scenes not set: ' + JSON.stringify(m.door)];
        const looks = T.filter(t => t.line === 'look');
        const [d1, d2, d3, d4, d5, d6, d7] = digs;   // (Knife, hands; chisel; pick, the door found, then a door in sight; chisel and hammer; magic)
        const t0 = t => t.at && t.after ? t.after.now - t.at.now : -1;
        dq0Need(f, d1, 'E_DIG', { how: 'D_NO_TOOL' }, 'a knife on the wall not refused');
        dq0Need(f, d2, 'E_DIG', { how: 'D_NO_TOOL' }, 'bare hands on stone not refused');
        dq0Need(f, d3, 'E_DIG', { how: 'D_NO_TOOL' }, 'a chisel with no hammer not refused');
        for (const d of [d1, d2, d3]) {
          dq0Never(f, d, 'E_NOWAY', {}, 'a refusal (nothing there to dig) telling the wall a door');
          if (dq0Ev(d, 'E_DIG', { how: 'D_BEGUN' }).length) f.push('a spell begun with no tool that bites: ' + d.line);
        }
        if (!/Your knife only scratches at the/.test(s)) f.push('no knife\'s words');
        if (!/You need better tools: your hands make no mark on the/.test(s)) f.push('no hands\' words');
        if (!/You'd need a hammer to drive the chisel./.test(s)) f.push('no chisel\'s words');
        dq0Need(f, d4, 'E_DIG', { how: 'D_BEGUN' }, 'the seeming wall not dug at');
        dq0Need(f, d4, 'E_DIG', { how: 'D_FOUND' }, 'the hidden door not found by the pick');
        if (!/Your pick strikes [a-z ,-]*: a door, behind the facing/.test(s)) f.push('no door found told');
        const rec = T.of('quit', d5.k).at && T.of('quit', d5.k).at.changes.find(c => c.what === 'PORTAL_STATE' && c.x === m.door[3].x && c.y === m.door[3].y);
        if (!rec || rec.tile & 0x20) f.push('the door still hidden after: ' + JSON.stringify(rec));
        else if (!(rec.elev & 0x80)) f.push('the door found not marked handled as a search finds it (PP_KNOWN): ' + JSON.stringify(rec));
        dq0Need(f, d5, 'E_DIG', { how: 'D_PORTAL' }, 'a door in sight not told so');
        if (!/That's a door: open it/.test(s)) f.push('no door\'s words');
        if (!dq0Ev(d6, 'E_DIG', { how: h => h === 'D_GOOD' || h === 'D_POOR' || h === 'D_SLIP' || h === 'D_THROUGH' }).length) f.push('a hammer and chisel not at work on the wall: ' + dq0Evs(d6));
        if (dq0Ev(d7, 'E_DIG', { how: 'D_FOUND' }).length) f.push('a door hidden by magic found by digging');
        if (!/This isn't getting anywhere/.test(s)) f.push('nothing told after two spells with nothing to show');
        dq0Need(f, T.of('dig north', d7.k), 'E_DIG', { how: 'D_PORTAL' }, 'a portcullis not told so');
        if (!/Those are iron bars/.test(s)) f.push("no bars' words");
        dq0Need(f, d7, 'E_TASK', { te: 'TE_ASK_SHORT', task: 'LT_DIG' }, 'the hopeless dig not asked about');
        if (f.length) f.push(JSON.stringify(dq0Ev(d7, 'E_TASK').map(e => e.te + ' ' + e.task)), s.slice(s.indexOf('-k pick'), s.indexOf('-k pick') + 2600));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rope', reach: true, near: ['map', 'gear'], what: "Dingus Quest Zero's rope and grapples (the user's A16, 2026-11-03: \"Queue rope rules\"; HOUSE.md's rope and grapples, for review), by the words, the item store and the referee's tests, each rule a scene: no drop, refused; a drop made beside the character by a poke (a ledge fifteen feet down), a rope got out by the hands routine (its steps told), a spike driven with a hammer (a minute's long task, its noise 7, the spike there for good), let down: a tied rope at its top, a hanging rope at its foot, seen; climbing down it by its help (every Climbing test on the rope 3 steps the better, the book's), at the foot the knot out of reach, climbing back up by it, untied: coiled at its feet, the hanging rope gone, the spike still driven, not taken, the rope taken; a thing that won't hold named, a chest that will, a pit's drop climbed into and out of by the rope; the second game every room a shaft (dq0sky=1): a mouth too high to reach, a grapple thrown up (a Throwing test at its band, the noise heard, told), the rope seen and looked at (a grapple's, how far), the grating (A18) barring the top as before, the grapple shaken loose from below, coiled at its feet, thrown again, its way out opened: climbed out by it; a grapple giving under the climber's weight (its record's RK_SLIP set: a marginal success's, one in four), its rope at its feet, the mouth too high again; the third game a rope let down a shaft from an iron ring (the maker's), the game saved and carried on: the rope still there, at its top and its foot, used to go down, still aided, its end poked out of reach from the foot: told, the shaft climbed unaided; a sack's tie, closed and opened",
      init: 't_rc', cycles: 4500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml thr 95 >> /ram/c.hm', 'echo ml cli 40 >> /ram/c.hm', 'dq0sky=0', 'dq0hour=128',
        "dq0 -p t -s 5 -c /ram/c.hm -T -k 'rope,iron spike,hammer'", 'tie the rope', 'look', 'tie the rope', 'look', 'look at the tied rope', 'd', 'look', 'untie the rope', 'u', 'untie the rope', 'look', 'take the driven spike', 'take the rope', 'inventory', 'quit',
        "dq0 -p t -s 5 -c /ram/c.hm -T -k 'rope,chest,sack'", 'drop the sack', 'tie the rope to the sack', 'tie the sack', 'untie the sack', 'drop the chest', 'wait', 'tie the rope to the chest', 'd', 'look', 'u', 'quit',
        'echo ml cli 90 >> /ram/c.hm', 'dq0sky=1', 'dq0hour=12', "dq0 -p t -s 2 -c /ram/c.hm -T -k 'rope,grapple'", 'go to the shaft', 'u', 'throw the grapple up', 'throw the grapple up', 'look', 'examine the hanging rope', 'untie the rope', 'untie the rope', 'untie the rope', 'look', 'take the rope', 'throw the rope up', 'throw the rope up', 'u',
        "dq0 -p t -s 2 -c /ram/c.hm -T -k 'rope,grapple'", 'go to the shaft', 'throw the grapple up', 'throw the grapple up', 'u', 'look', 'u', 'quit',
        "dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/s -k 'rope'", 'go to the shaft', 'tie the rope', 'quit', 'dq0 -d /ram/s', 'look', 'd', 'look', 'u', 'quit', 'echo $status']) },
      expect: ['% echo $status'], sdcard: true,
      start(m) {
        const drop = (ft, pit) => () => {                 // (A floor square beside the character made a drop, or a pit)
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), w = m.dq0.bank(WORLD_BANK_N), st = stratumRec(m, h[L.pcSn]);
          const ox = (h[L.pcX] | h[L.pcX + 1] << 8) - st.x0, oy = (h[L.pcY] | h[L.pcY + 1] << 8) - st.y0;
          const [TILE, ELEV] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, tile)', 'offsetof (struct chunk, elev)']);
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (!w[sl] || !b || w[sl + 1] !== h[L.pcSn] || (w[sl + 2] | w[sl + 3] << 8) !== ox >> 5 || (w[sl + 4] | w[sl + 5] << 8) !== oy >> 5) continue;
            for (const [dx, dy] of [[0, -1], [1, 0], [0, 1], [-1, 0]]) {
              const x = (ox & 31) + dx, y = (oy & 31) + dy;
              if (x < 0 || y < 0 || x > 31 || y > 31 || (b[o + TILE + y * 32 + x] & 15) !== 1) continue;
              if (pit) b[o + TILE + y * 32 + x] = b[o + TILE + y * 32 + x] & 0xF0 | 9;
              else b[o + ELEV + y * 32 + x] = (b[o + ELEV + y * 32 + x] - ft) & 255;
              m.dropAt = [st.x0 + x + (ox & ~31), st.y0 + y + (oy & ~31)];
              return;
            }
          }
        };
        const K_HUNG = (cat, L) => { for (let k = 1; k < 206; k++) { let n = ''; for (let j = 0; j < 14 && cat[k * L.kindSize + j]; j++) n += String.fromCharCode(cat[k * L.kindSize + j]); if (n === 'hanging rope') return k; } return -1; };
        dq0Poke(m, [{ g: 1, line: 'tie the rope', n: 2, call: drop(15) }, { g: 2, line: 'tie the rope to the chest', n: 1, call: drop(0, 1) },
          { g: 3, line: 'look', n: 1, fuel: { 'hanging rope': 0x0B04 } }, { g: 3, line: 'u', n: 2, fuel: { 'hanging rope': 0x0B04 } }, { g: 4, line: 'u', n: 1, fuel: { 'hanging rope': 0x0B84 } },
          { g: 6, line: 'u', n: 1, call: () => {          // (Its end twenty feet up: a shorter rope's, out of reach from the floor)
            const { L } = m.dq0.layout(), cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);
            for (let i = 0; cat && i < L.nitems; i++) {
              const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
              if (b && b[o + L.iKind] === K_HUNG(cat, L)) b[o + L.iOwner] = 20;
            }
          } }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], K = m.dq0.layout().K, o = dq0Story(out);
        const say = (t, re, what) => { if (!re.test(o)) f.push('no words: ' + what); };
        const g1 = T.of("dq0 -p t -s 5 -c /ram/c.hm -T -k 'rope,iron spike,hammer'");
        const climbs = t => t.replies.filter(r => /^slot/.test(r.req) && r.a === K.P_CLIMB).map(r => r.steps);
        // A rope, tied off
        say(0, /tie the rope You lift the rope\. There's no drop here to let a rope down\./, 'no drop');
        const t1 = T.of('tie the rope', g1.k), t2 = T.of('tie the rope', t1.k + 1), d1 = T.of('d', g1.k), n1 = T.after(d1, 'look');
        say(0, /You get out the hammer\./, 'the hammer got out, the hands routine');
        say(0, /You set to tying the rope\./, 'its first words');
        say(0, /You tie the rope to the driven spike and let it down over the edge: it reaches the foot\./, 'tied to a spike, let down');
        const look = T.after(t2, 'look');
        const at = (s, nm) => s ? s.items.filter(i => i.name === nm && i.where === 'IW_SQUARE') : [];
        if (!T.filter(t => t.k >= t2.k && t.k <= look.k).some(t => dq0Ev(t, 'E_NOISE', { a: 7 }).length)) f.push('the spike\'s driving not heard');
        if (at(look.at, 'tied rope').length !== 1 || at(look.at, 'hanging rope').length !== 1 || at(look.at, 'driven spike').length !== 1) f.push('a rope in place not its two records and a spike: ' + JSON.stringify((look.at || { items: [] }).items.filter(i => /rope|spike/.test(i.name))));
        if (!/feet of the climb/.test(o)) f.push('the rope looked at: how far it goes not told');
        // Climbed, aided
        const dn = climbs(d1);
        if (!dn.length || dn.some(s => s !== 3)) f.push('the climb down not 3 steps the better: ' + dn);
        if (!/You take hold of the rope\./.test(o)) f.push('no hold of the rope told');
        const foot = n1.at;
        if (!foot || !m.dropAt || foot.pc.x === d1.at.pc.x && foot.pc.y === d1.at.pc.y) f.push('not at the foot');
        say(0, /The knot's far above you, out of reach\./, 'the knot from below');
        const u1 = T.after(n1, 'u');
        if (climbs(u1).some(s => s !== 3) || !climbs(u1).length) f.push('the climb up not aided: ' + climbs(u1));
        const un = T.after(u1, 'untie the rope');
        say(0, /You untie the rope, draw it up and coil it at your feet\./, 'untied');
        const lk2 = T.after(un, 'look');
        if (at(lk2.at, 'hanging rope').length || at(lk2.at, 'tied rope').length || !dq0Named(dq0Here(lk2.at), 'rope').length) f.push('the rope not back, coiled at its feet: ' + JSON.stringify((lk2.at || { items: [] }).items.filter(i => /rope/.test(i.name))));
        say(0, /driven spike is fixed where it stands/, 'the spike taken');
        const tk = T.after(lk2, 'take the rope'), inv = T.after(tk, 'inventory');
        if (!dq0Named(dq0Carried(inv.at), 'rope').length) f.push('the rope not taken up');
        // What holds; a pit
        say(0, /tie the rope to the sack .{0,80}That won't hold a rope\./, 'a sack as an anchor');
        if (!/You tie the rope to the chest and let it down over the edge/.test(o)) f.push('tied to the chest, a pit\'s edge');
        const g2 = T.of("dq0 -p t -s 5 -c /ram/c.hm -T -k 'rope,chest,sack'");
        const d2 = T.of('d', g2.k), u2 = T.after(d2, 'u');
        if (!climbs(d2).length || climbs(d2).some(s => s !== 3)) f.push('down into the pit not aided: ' + climbs(d2));
        if (!climbs(u2).length) f.push('out of the pit not climbed');
        if (u2.at && !(u2.at.pc.x === m.dropAt[0] && u2.at.pc.y === m.dropAt[1])) f.push('not in the pit: ' + JSON.stringify([u2.at.pc.x, u2.at.pc.y, m.dropAt]));
        // A grapple thrown up
        say(0, /The shaft's mouth is too high to reach\./, 'the mouth too high');
        const g3 = T.of("dq0 -p t -s 2 -c /ram/c.hm -T -k 'rope,grapple'");
        const th = T.of('throw the grapple up', g3.k);
        const P = th.replies.filter(r => /^slot/.test(r.req) && r.a === K.P_THROW);
        if (!P.length || P[0].steps !== -4) f.push('its test not Throwing at its band: ' + JSON.stringify(P.map(r => [r.a, r.steps])));
        if (!dq0Ev(th, 'E_NOISE', {}).length) f.push('a throw not heard');
        say(0, /The grapple flies up and catches, with a clang\./, 'caught');
        say(0, /It's tied to the grapple, in the mouth of the shaft above: \d+ feet of the climb\./, 'the rope looked at');
        say(0, /You shake the rope, and the grapple comes loose and clatters down\./, 'shaken loose');
        say(0, /You take hold of the rope\. You set to climbing: \d+ feet of it\./, 'up the rope');
        if (!T.filter(t => t.k >= th.k).some(t => dq0Ev(t, 'E_LEFT', {}).length)) f.push('not climbed out by the rope');
        // Its grapple giving
        say(0, /You put your weight on the rope: the grapple tears loose and comes clattering down\. The shaft's mouth is too high to reach\./, 'the grapple giving');
        // Kept
        const g5 = T.of("dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/s -k 'rope'");
        const c5 = T.of('dq0 -d /ram/s', g5.k), d5 = T.of('d', c5.k);
        if (!/You tie the rope to the iron ring and let it down the shaft/.test(o)) f.push('not tied to the maker\'s ring');
        if (!at(d5.at, 'tied rope').length) f.push('the rope not there carried on');
        if (!climbs(d5).length || climbs(d5).some(s => s !== 3)) f.push('the rope, carried on, no help: ' + climbs(d5));
        const lk5 = T.after(d5, 'look');
        if (!at(lk5.at, 'hanging rope').length) f.push('its foot not there, carried on');
        const u5 = T.after(lk5, 'u');
        if (!/The rope's end hangs out of reach\./.test(o)) f.push('a rope\'s end out of reach not told');
        if (climbs(u5).some(s => s !== 0)) f.push('a rope out of reach, aided: ' + climbs(u5));
        if (/understand that/.test(o)) f.push('a line not understood');
        return f;
      },
    },
    {
      name: 'dq0foundkey', near: ['map', 'gear'], what: "Dingus Quest Zero's found keys (the user's A17: \"fit something nearby\"; HOUSE.md's keys that fit, for review): by the item store and the chunks: in three seeds' first chunks a key the maker put down is numbered for a door in its own chunk, that door locked, mechanical; none has no lock; the same game the same number; the key given the character (a poke), put beside its door: \"You unlock it with its key.\", no Picklock test asked",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0hour=128', 'dq0sky=0',
        'dq0 -p t -s 2 -c /ram/c.hm -T', 'look', 'wait', 'wait', 'unlock the door ahead', 'quit', 'dq0 -p t -s 5 -c /ram/c.hm -T', 'quit', 'dq0 -p t -s 6 -c /ram/c.hm -T', 'quit', 'dq0 -p t -s 2 -c /ram/c.hm -T', 'quit', 'echo $status']) },
      expect: ['% echo $status'], sdcard: true,
      start(m) {
        const doors = () => {                             // (Every cached chunk's doors: their squares, states)
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), w = m.dq0.bank(WORLD_BANK_N), r = [];
          const [PORTAL, PSIZE, NPORT, KIND, STATE, LOCK] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, portal)', 'sizeof (struct portal)', 'offsetof (struct chunk, nportals)', 'offsetof (struct portal, kind)', 'offsetof (struct portal, state)', 'offsetof (struct portal, lock)']);
          const st = stratumRec(m, h[L.pcSn]);
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (!w[sl] || !b || w[sl + 1] !== h[L.pcSn]) continue;
            const cx = w[sl + 2] | w[sl + 3] << 8, cy = w[sl + 4] | w[sl + 5] << 8;
            for (let i = 0; i < b[o + NPORT]; i++) {
              const q = o + PORTAL + i * PSIZE;
              r.push({ x: st.x0 + (cx << 5) + b[q], y: st.y0 + (cy << 5) + b[q + 1], kind: b[q + KIND], state: b[q + STATE], lock: b[q + LOCK] });
            }
          }
          return { st, r };
        };
        const id = (sn, x, y) => 0x8000 | ((x * 37 + y * 101 + sn * 4099) & 0x7FFF);
        const keys = () => {
          const { L } = m.dq0.layout(), cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), out = [];
          for (let i = 0; cat && i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (!b || !b[o + L.iKind]) continue;
            let n = '';
            for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
            if (n === 'iron key' && b[o + 2] === 0) out.push({ x: b[o + 4] | b[o + 5] << 8, y: b[o + 6] | b[o + 7] << 8, fits: b[o + L.iSize] | b[o + L.iSize + 1] << 8 });
          }
          return out;
        };
        m.found = {};
        const note = g => () => {
          const { st, r } = doors(), ks = keys(), h = m.dq0.bank(0), { L } = m.dq0.layout();
          m.found[g] = ks.map(k => {
            const d = r.find(q => id(h[L.pcSn], q.x, q.y) === k.fits);
            return { key: k, door: d, same: d ? (d.x - st.x0 >> 5) === (k.x - st.x0 >> 5) && (d.y - st.y0 >> 5) === (k.y - st.y0 >> 5) : false };
          });
        };
        const put = (x, y) => { const h = m.dq0.bank(0), { L } = m.dq0.layout(); h[L.pcX] = x & 255; h[L.pcX + 1] = x >> 8; h[L.pcY] = y & 255; h[L.pcY + 1] = y >> 8; };
        dq0Poke(m, [{ g: 1, line: 'look', n: 1, call: note(1) },
          { g: 1, line: 'wait', n: 1, call: () => {       // (It's found: in the character's hands, as if picked up; a search and a take are their own tests')
            const { L } = m.dq0.layout(), cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), k = m.found[1][0];
            for (let i = 0; cat && k && i < L.nitems; i++) {
              const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
              if (!b || !b[o + L.iKind] || b[o + L.iWhere] !== 0 || (b[o + L.iX] | b[o + L.iX + 1] << 8) !== k.key.x || (b[o + L.iSize] | b[o + L.iThick] << 8) !== k.key.fits) continue;
              b[o + L.iWhere] = 1;
              b[o + L.iX] = b[o + L.iX + 1] = 0;
              b[o + L.iFlags] = b[o + L.iFlags] & ~8 | 0x20;
            }
          } },
          { g: 1, line: 'wait', n: 2, call: () => {     // (Beside its door, on a floor: a look, then it faced)
            const k = m.found[1][0], h = m.dq0.bank(0), { L } = m.dq0.layout(), w = m.dq0.bank(WORLD_BANK_N), st = stratumRec(m, h[L.pcSn]);
            const [TILE] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, tile)']);
            if (!k || !k.door) return;
            const ox = k.door.x - st.x0, oy = k.door.y - st.y0;
            for (let c = 0; c < 24; c++) {
              const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
              if (!w[sl] || !b || w[sl + 1] !== h[L.pcSn] || (w[sl + 2] | w[sl + 3] << 8) !== ox >> 5 || (w[sl + 4] | w[sl + 5] << 8) !== oy >> 5) continue;
              for (const [dx, dy, face] of [[0, -1, 4], [1, 0, 6], [0, 1, 0], [-1, 0, 2]]) {
                const t = b[o + TILE + ((oy & 31) + dy) * 32 + (ox & 31) + dx] & 15;
                if (t === 1 || t === 4) { put(k.door.x + dx, k.door.y + dy); h[L.pcFace] = face; m.doorAt = [dx, dy, face, ox & 31, oy & 31, t]; return; }
              }
            }
          } },
          { g: 2, line: 'quit', n: 1, call: note(2) }, { g: 3, line: 'quit', n: 1, call: note(3) }, { g: 4, line: 'quit', n: 1, call: note(4) }]);
      },
      check(m, out) {
        const f = [], o = dq0Story(out);
        let n = 0;
        for (const g of [1, 2, 3, 4]) for (const e of m.found[g] || []) {
          ++n;
          if (!e.door || !e.same) f.push('game ' + g + ': a found key fits no door in its chunk: ' + JSON.stringify(e));
          else if (!(e.door.state & 2) || !e.door.lock) f.push('game ' + g + ': its door not locked: ' + JSON.stringify(e));
        }
        if (!n) f.push('no found key in the games');
        if (JSON.stringify(m.found[1].map(e => e.key)) !== JSON.stringify(m.found[4].map(e => e.key))) f.push('the same game, other keys');
        const T = dq0Turns(m), K = m.dq0.layout().K, g1 = T.of('dq0 -p t -s 2 -c /ram/c.hm -T'), u = T.of('unlock the door ahead', g1.k);
        if (!/You unlock the door with its key/.test(o)) f.push('its door not unlocked by it: ' + o.slice(o.indexOf('> unlock'), o.indexOf('> unlock') + 200));
        if (u.replies.some(r => /^slot/.test(r.req) && r.a === K.P_PICKLOCK)) f.push('a Picklock test asked');
        return f;
      },
    },
    {
      name: 'dq0vast', meaning: true, what: 'Dingus Quest Zero\'s plans past PLAN_MAX regions (move 2.8: no fixed size of a stratum\'s plan), by what the character perceives and the maker\'s map: dq0 -T -T -T -T (the stratum below the top 27 regions each way, 729 of them, its plan paged to the save folder by the maker, plSS0000.dat, its pages of 42 regions in a bank of its own) and -C (16 of those pages, made way for as it grows) and -d: down the stairway, the stratum below grown, mapped; quit and carried on (its plan grown again, the same): seen the same, the map the same',
      init: 't_rc', cycles: 3600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -T -T -T -C -d /ram/kept', 'd',
        'map', 'quit', 'ls /ram/kept/d1', 'dq0 -C -d /ram/kept', 'map', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], d = T.of('d'), c = T.of('dq0 -C -d /ram/kept'), m1 = T.of('map'), m2 = T.after(c, 'map');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (!d.after || d.after.pc.sn !== 1 || !c.after || c.after.pc.sn !== 1 || dq0View(c.after) !== dq0View(d.after))
          f.push('the vast stratum not seen the same carried on: ' + dq0View(c.after) + ', first ' + dq0View(d.after));
        if (!m1.after || !m2.after || !m1.after.map.some(r => r.trim()) || m1.after.map.join('\n') !== m2.after.map.join('\n')) f.push('the map not the same carried on');
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' ');
        if (!/pl10000\.dat/.test(o.slice(o.indexOf('% ls /ram/kept/d1')))) f.push('no plan paged to the save folder (pl10000.dat)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0soak', meaning: true, what: 'Dingus Quest Zero\'s stores paged to the save folder (move 2.8, DUNGEON.md: no fixed size; the user, 2026-10-16: "It should be as big as it needs to be, given there is enough disk space to hold it"), eviction and write-back, a soak, by what the character perceives (the senses\' record) and the stores\' records: dq0 -C (the caches small: the chunks\' twelve slots, a change page, 24 of the item store\'s records, two of the denizens\' regions, two of the strata\'s slots, two seen maps) and -T -T -T (three strata grown below the top, the character on the second): the dagger put down by the stairway there, up (the strata\'s slots made way in: the top\'s record read back into a slot), the rations put down by the stairway on the top, down and up again and again, every cache made way in each time (the chunks made again from their seeds, the change records, the things and the denizens read back from their files, the seen maps too): each stratum seen each time as it was first (the dagger, the rations where they were put down), the giant rats by the stairway seen; the map; the folder\'s region files (rg, it, dn, sm, st); quit and carried on: where it was, seen the same, the map as it was, up again, the rations there; then the game\'s game.dat gone and a new game begun in the folder (seed 5): the old game\'s region files of its stratum below gone, none of its things lying about (dq0page\'s and dq0strata\'s, these together)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -T -T -C -d /ram/kept',
        'drop the dagger', 'u', 'drop the rations', 'd', 'u', 'd', 'u', 'd', 'map', 'quit', 'ls /ram/kept/d1', 'dq0 -C -d /ram/kept', 'map', 'u', 'quit',
        'rm /ram/kept/d1/game.dat', 'dq0 -p t -s 5 -c /ram/c.hm -T -C -d /ram/kept', 'quit', 'ls /ram/kept/d1', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], at = (s, n) => s && dq0Here(s).some(i => i.name === n);
        const d0 = T.of('drop the dagger'), r0 = T.of('drop the rations');
        if (!at(d0.after, 'dagger') || !d0.after || d0.after.pc.sn !== 1) f.push('the dagger not put down on the second stratum');
        if (!at(r0.after, 'rations') || !r0.after || r0.after.pc.sn !== 0) f.push('the rations not put down on the top');
        const view = { 1: dq0View(d0.after), 0: dq0View(r0.after) };
        let rats = 0;
        for (const t of T.filter(t => (t.line === 'u' || t.line === 'd') && t.game === 0 && t.k > r0.k)) {     // (Each way, after both were put down)
          const s = t.after, sn = t.line === 'u' ? 0 : 1;
          dq0Need(f, t, 'E_ARRIVE', { how: t.line === 'u' ? 'A_UP' : 'A_DOWN' }, 'not ' + (t.line === 'u' ? 'up' : 'down') + ' the stairway');
          if (!s || s.pc.sn !== sn) { f.push('> ' + t.line + ' (line ' + t.k + '): not on stratum ' + sn); continue; }
          if (dq0View(s) !== view[sn]) f.push('stratum ' + sn + ' not as it was (line ' + t.k + '): ' + dq0View(s) + ', first ' + view[sn]);
          if (s.sight.dens.some(d => d.name === 'giant rat')) rats++;
        }
        if (!rats) f.push('the giant rats by the stairways never seen (their regions\' records read back)');
        const mp = T.of('map'), q = T.of('quit'), c = T.of('dq0 -C -d /ram/kept'), mp2 = T.after(c, 'map'), u2 = T.after(c, 'u');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (!c.after || !q.at || c.after.pc.sn !== q.at.pc.sn || c.after.pc.x !== q.at.pc.x || c.after.pc.y !== q.at.pc.y || dq0View(c.after) !== view[1])
          f.push('not carried on where it was, seen the same');
        if (!mp.after || !mp2.after || !mp.after.map.some(r => r.trim()) || mp.after.map.join('\n') !== mp2.after.map.join('\n')) f.push('the map not as it was, carried on');
        if (!u2.after || dq0View(u2.after) !== view[0]) f.push('the top not as it was, up again once carried on (the rations)');
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' ');
        const ls1 = o.slice(o.indexOf('% ls /ram/kept/d1'), o.indexOf('% dq0 -C')), ls2 = o.slice(o.lastIndexOf('% ls /ram/kept/d1'), o.indexOf('% echo $status'));
        for (const p of ['rg', 'it', 'dn', 'sm', 'st']) if (!new RegExp(p + '[0-9a-f]{5,6}\\.da').test(ls1)) f.push('no ' + p + '... region files');
        if (/rg1\w{4}\.da/.test(ls2)) f.push('the old game\'s region files left in the folder for the new game (its stratum below\'s)');
        const g = T.of('dq0 -p t -s 5 -c /ram/c.hm -T -C -d /ram/kept');
        dq0Need(f, g, 'E_ARRIVE', { how: 'A_START' }, 'the new game not begun');
        if (!g.after || g.after.game !== 2 || g.after.items.some(i => i.where === 'IW_SQUARE' && (i.name === 'dagger' || i.name === 'rations')))
          f.push('the old game\'s things lying about in the new game');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stream', meaning: true, near: ['map'], what: "Dingus Quest Zero's streams (the user's, 2026-11-03: some run deep enough to swim, so a current can happen; HOUSE.md's the water in the dungeon): the maker's, by the seed alone: seed 17's stratum below has a stretch of stream six feet deep in the chunks cached, seed 2's a foot deep",
      init: 't_rc', cycles: 1200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 17 -c /ram/c.hm -T', 'd', 'quit', 'dq0 -p t -s 2 -c /ram/c.hm -T', 'd', 'quit', 'echo $status']) },
      start(m) {
        m.runs = [];
        const scan = () => { const r = []; for (let c = 0; c < 24; c++) { const b = m.dq0.bank(5 + Math.floor(c / 3)); if (!b) continue; const o = (c % 3) * 0xAA0; if (b[o + 0xA8C] & 0x40) r.push(b[o + 0xA8B]); } m.runs.push(r); };
        dq0Poke(m, [{ g: 1, line: 'quit', n: 1, call: scan }, { g: 2, line: 'quit', n: 1, call: scan }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [];
        this.notes = [JSON.stringify(m.runs)];
        if (!m.runs[0] || !m.runs[0].includes(6)) f.push('seed 17: no stretch six feet deep: ' + JSON.stringify(m.runs[0]));
        if (!m.runs[1] || !m.runs[1].length || m.runs[1].some(d => d !== 1)) f.push('seed 2: its stream not a foot deep: ' + JSON.stringify(m.runs[1]));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0streamswim', near: ['map', 'body'], what: "Dingus Quest Zero's deep stream swum in play (the user's, 2026-11-03: some run deep enough to swim; HOUSE.md's the water in the dungeon, swimming; dq0stream's the maker's alone), by the hub and the words: seed 17's stream six feet deep along a passage on the cave's north side, the rats there held off (dq0Poke's): north from the landing into it, out of its depth (\"You're out of your depth: you swim.\"), its torch out, its swim told by the referee's grade, the hub's pc_wet six feet, pc_swim its squares a minute; along it east and back west, each a swimmer's square, longer than a walked one, still swimming; south out of it (\"You wade out of the water.\"), dry",
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 17 -c /ram/c.hm -T', 'd', 'n', 'step east', 'step west', 'step west', 'step south', 'step south', 'quit', 'echo $status']) },
      start(m) {
        m.wet = [];
        const off = (h, L, K) => {
          const at = k => L.actor + k * L.actorSize, R = dq0Rec(m), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, due = now + 3000;
          let n = 0;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            const o = R.at(h[at(k) + L.aName + 12]), x = 32706 + (n & 3), y = 32784 + (n++ >> 2);
            h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8;
            h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_FRIENDLY;
            h[at(k) + L.aDoing] = K.DO_ROAM;
            for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = due >>> (8 * i) & 255;
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        const seen = line => (h, L) => m.wet.push({ line, wet: h[L.pcWet], swim: h[L.pcSwim], breath: h[L.pcBreath] });
        dq0Poke(m, [{ g: 1, line: 'n', n: 1, call: off }, { g: 1, line: 'step east', n: 1, call: seen('n') }, { g: 1, line: 'step west', n: 1, call: seen('east') },
          { g: 1, line: 'step west', n: 2, call: seen('west') }, { g: 1, line: 'step south', n: 1, call: seen('west 2') }, { g: 1, line: 'step south', n: 2, call: seen('south') }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out), w = l => (m.wet || []).find(e => e.line === l) || {}, t = (l, n) => T.of(l, n === 2 ? T.of(l).k + 1 : 0);
        inTurn(o, ["You're out of your depth: you swim.", 'The water puts out your light.'], f, 'into the stream: ');
        if (!/You swim strongly\.|You struggle to keep afloat\./.test(o)) f.push('its swimming not told (the referee\'s test)');
        for (const l of ['n', 'east', 'west', 'west 2']) if ((w(l).wet & 0x7F) !== 6 || !w(l).swim) f.push('not swimming six feet down after ' + l + ': ' + JSON.stringify(w(l)));
        const e = t('step east'), we = t('step west'), wt = t('step west', 2), so = t('step south'), so2 = t('step south', 2);
        for (const [u, dx, l] of [[e, 1, 'east'], [we, -1, 'west'], [wt, -1, 'west again']]) {
          if (!u.after || u.after.pc.x - u.at.pc.x !== dx || u.after.pc.y !== u.at.pc.y) f.push('a stroke ' + l + ' not its square along the stream');
        }
        if (!so.after || so.after.pc.y !== so.at.pc.y + 1) f.push('south out of the stream not gone');
        if (!/You wade out of the water\./.test(o)) f.push('coming out not told');
        if (w('south').wet & 0x7F) f.push('dry floor, still wet: ' + JSON.stringify(w('south')));
        if (so2.after && e.after && !(e.after.now - e.at.now > so2.after.now - so2.at.now)) f.push('a swimmer\'s square no longer than a walked one: ' + (e.after.now - e.at.now) + ' tenths, ' + (so2.after.now - so2.at.now));
        if (e.after && w('n').swim && e.after.now - e.at.now < Math.floor(600 / w('n').swim)) f.push('a stroke not its squares a minute\'s time: ' + (e.after.now - e.at.now) + ' tenths at ' + w('n').swim + ' a minute');
        this.notes = [JSON.stringify(m.wet)];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rockname', near: ['map'], what: "Dingus Quest Zero's rock named (the user's, 2026-11-03: \"Naming the rock\": HOUSE.md's told as it's come to): a middling miner (Mining 40, dq0 -R 2's dice) at seed 3 digs a shaft down: into the rock, a spell that fails tells only its looks, and the first good spell after it names it, though it's no first spell",
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml min 40 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k pick -a -R 2', 'dig down', 'stop', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out), told = [...s.matchAll(/You're into ([a-z -]+?) rock(: [a-z ]+)?\./g)];
        this.notes = [told.map(x => x[0]).join(' ')];
        if (told.length !== 2 || told[0][2] || !told[1][2] || told[0][1] !== told[1][1]) f.push('not its looks, then named at the first good spell: ' + told.map(x => x[0]).join(' '));
        else if (!/rock: [a-z ]+\. The work goes well\./.test(s)) f.push('named but not with a spell that went well');
        return f;
      },
      sdcard: true
    },
    {
      // (Seed 7, dq0glow's dungeon: the kitchen the character begins in, lit by its sconce's torch (lit first: since slice
      // 4.1 nobody keeps it lit, its torch cold), its south door opened
      // (its dart trap set off) and down the straight passage south to a dark hall; the kitchen's floor ends at row
      // 32840, its door at 32841, the hall's north door at 32850, and an iron pot lies on the kitchen's floor at 32785,
      // 32839, straight up the passage from the hall's doorway square, 32785,32851, and its far side, 32785,32855; the
      // hall's east door, at its far side's 32787,32855, closed.  A character of Eyesight 5: HarnMaster's sight 1, a
      // glance's reach 6 squares, a look one way's 12; the null ruleset's sight 5, 14 and 28)
      // (dq0sky=0, 2026-11-03, a scene's: seed 7's kitchen's skylight takes the middle of its floor, its iron pot not stocked there; its window lit, the doorway's looks other; no fault)
      name: 'dq0lookway', meaning: true, what: 'Dingus Quest Zero\'s looking one way, listening and sniffing one way (HOUSE.md\'s slice 4.0, the user\'s: "look north", "look at the door", "listen at the door"; told by the facing alone), by the hub\'s events and the senses\' record (what the character perceives, and a look one way\'s own: WAY_SEEN): at seed 7 in the dark hall below the lit kitchen (lit by its sconce\'s torch, the character\'s first: nobody keeps it lit, slice 4.1\'s), a glance and then a look north from its doorway and from its far side: a look one way reaches twice a glance\'s squares (the ruleset\'s sight: 4 + 2 sight) in the quarter ahead, so the kitchen\'s iron pot straight up the passage is in the glance\'s record only within its reach and in the look one way\'s within twice it, and one of the two looks sees it where the glance doesn\'t; the look one way\'s own record what\'s beyond the hall\'s open door, the kitchen, lit (a room\'s kind, U_KITCHEN), or only the dark past the look\'s reach; each look one way the world\'s I_LOOK, E_VIEW V_LOOKED, 3 seconds; looking east at the closed door, turned to it first (V_FACING), not seen past (WB_SHUT); listening at the door, turned to it already, 3 seconds and no look, the watchfulness not set (not "listen as I go"); listening about and sniffing, 3 seconds each',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 5 -u feudal -c clothier -o /ram/c.hm', 'cp /ram/c.hm /ram/e.hm', 'echo att eye 5 >> /ram/e.hm',
        'dq0 -p t -s 7 -c /ram/e.hm', 'light the torch in the sconce', 'face north', 'open the door behind me', 's', 'turn around', 'look north', 'move south 4 squares', 'face north',
        'look north', 'look east', 'listen at the door', 'listen', 'sniff', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], at = (t, what) => t.k >= 0 && t.at && t.after && t.after.sight && t.after.way ? true : (f.push(what + ' not typed, or no record'), false);
        const ln = T.filter(t => t.line === 'look north'), glance = [T.of('turn around'), T.of('face north')];
        if (ln.length !== 2 || !glance.every((g, i) => at(g, 'the glance ' + i)) || !ln.every((t, i) => at(t, 'look north ' + i))) return f.concat(['the looks not typed']);
        const pot = (ln[1].after.items || []).find(i => i.name === 'iron pot' && i.x === 32785 && i.where === 'IW_SQUARE');
        if (!pot) return ['the kitchen\'s iron pot not found at 32785'];
        const has = s => s.items.some(i => i.name === 'iron pot' && i.rel === 0);
        let more = 0;
        ln.forEach((t, i) => {
          const r = 4 + 2 * Math.min(t.at.sum.sight, 9), w = Math.min(2 * r, 31), g = glance[i].after, s = t.after.sight, d = t.after.pc.y - pot.y, k = t.after.pc.y - 32840;
          const what = 'look north ' + i + ' (' + d + ' squares from the pot, a glance\'s reach ' + r + ')';
          if (t.after.pc.face !== 0 || t.after.pc.x !== 32785) f.push(what + ': not facing north up the passage');
          if (has(g.sight) !== (d <= r)) f.push(what + ': the glance ' + (d <= r ? 'doesn\'t see' : 'sees') + ' the pot');
          if (has(s) !== (d <= w)) f.push(what + ': the look one way ' + (d <= w ? 'doesn\'t see' : 'sees') + ' the pot');
          more += !has(g.sight) && has(s);
          dq0Need(f, t, 'E_VIEW', { why: 'V_LOOKED' }, what + ': no V_LOOKED');
          dq0Never(f, t, 'E_VIEW', { why: 'V_FACING' }, what + ': turned, facing north already');
          if (t.after.now - t.at.now !== 30) f.push(what + ': ' + (t.after.now - t.at.now) + ' tenths, not 30');
          const b = t.after.way.valid ? t.after.way.beyond.find(e => e.rel === 0 && e.noun === 'BN_OPEN') : null;
          if (!b) f.push(what + ': nothing beyond the open door ahead in the look one way\'s own: ' + JSON.stringify(t.after.way));
          else if (k <= w ? b.what !== 'WB_ROOM' || b.kind !== 8 || !b.lit : b.what !== 'WB_DARK') f.push(what + ': beyond the door ' + JSON.stringify(b) + ', the kitchen ' + k + ' squares off');
          if (has(s) && !t.after.way.items.some(e => e.name === 'iron pot')) f.push(what + ': the pot not among the things ahead');
        });
        if (!more) f.push('no look one way saw the pot where the glance didn\'t');
        const e = T.of('look east'), l = T.of('listen at the door');
        if (at(e, 'look east')) {
          dq0Need(f, e, 'E_VIEW', { why: 'V_FACING' }, 'look east: not turned first');
          dq0Need(f, e, 'E_VIEW', { why: 'V_LOOKED' }, 'look east: no V_LOOKED');
          if (e.after.pc.face !== 2) f.push('look east: facing ' + e.after.pc.face);
          if (!e.after.way.valid || !e.after.way.beyond.some(b => b.rel === 0 && b.noun === 'BN_DOOR' && b.what === 'WB_SHUT')) f.push('look east: the closed door not shut to the look: ' + JSON.stringify(e.after.way));
        }
        for (const [x, face] of [[l, 2], [T.of('listen'), 2], [T.of('sniff'), 2]]) {
          if (!at(x, x.line)) continue;
          dq0Never(f, x, 'E_VIEW', { why: 'V_LOOKED' }, x.line + ': a look');
          dq0Never(f, x, 'E_VIEW', { why: 'V_FACING' }, x.line + ': turned');
          if (x.after.pc.face !== face) f.push(x.line + ': facing ' + x.after.pc.face);
          if (x.after.now - x.at.now !== 30) f.push(x.line + ': ' + (x.after.now - x.at.now) + ' tenths, not 30');
          if (x.after.pc.watch & 2) f.push(x.line + ': listening as it goes set (WA_LISTEN)');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0portc', meaning: true, what: 'Dingus Quest Zero\'s portcullises (DUNGEON.md: portals; the world works them, dq0w/wwin.c), by the world\'s events and what the character perceives: at seed 514 the kobold warren\'s gatehouse (its every way out a portcullis, raised), the one ahead lowered by its winch (walked up to first), seen down, never walked through while it\'s down, raised again by the winch, seen up, and gone through (broken, bent and the windows: dq0break)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 514 -c /ram/c.hm', 'lower portcullis', 'n',
        'raise portcullis', 'n', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], seen = (t, open) => t.after && t.after.sight && t.after.sight.doors.some(d => d.rel === 0 && d.kind === 'PK_PORTCULLIS' && (d.state & 1) === open);
        const s0 = T.of('dq0 -p t -s 514 -c /ram/c.hm');
        if (!seen(s0, 1)) f.push('no raised portcullis ahead to begin with');
        const lo = T.of('lower portcullis'), n = T.of('n'), r = T.of('raise portcullis'), n2 = T.after(r, 'n');
        dq0Need(f, lo, 'E_PORTAL', { po: 'PO_LOWERED', pk: 'PK_PORTCULLIS' }, 'the portcullis not lowered');
        if (!seen(lo, 0)) f.push('the portcullis ahead not seen down');
        dq0Need(f, n, 'E_NOWAY', { why: 'N_CLOSED' }, 'walked through the portcullis down');
        if (!n.at || !n.after || n.after.pc.x !== n.at.pc.x || n.after.pc.y !== n.at.pc.y) f.push('moved, the portcullis down');
        dq0Need(f, r, 'E_PORTAL', { po: 'PO_RAISED', pk: 'PK_PORTCULLIS' }, 'the portcullis not raised');
        if (!seen(r, 1)) f.push('the portcullis ahead not seen up');
        dq0Need(f, n2, 'E_ARRIVE', {}, 'not gone through the portcullis raised');
        if (!n2.at || !n2.after || !(n2.after.pc.y < n2.at.pc.y)) f.push('not gone north through it');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0portcopen', meaning: true, near: ['map', 'parser'], what: 'Dingus Quest Zero\'s portcullis opened and closed as it\'s raised and lowered (the user\'s, 2026-11-14: "open portcullis doesn\'t actually raise it ... raise portcullis does"; DECISIONS.md\'s "Opening a portcullis"), by the world\'s events, what the character perceives and the words: at seed 514 the kobold warren\'s gatehouse (dq0portc\'s), a strong character (Strength 18); from its winch\'s side the portcullis ahead lowered, opened (wound up, then up and seen up: the same words as raised), closed (let down, seen down: the same words as lowered), raised, and gone through (north, along the passage beyond); from the far side (walked back to it, let down meanwhile: its state poked in the chunks\' cache) heaved up by "open", neither closed nor lowered (its winch on the other side, the same words for both), let down again (poked) and heaved up by "raise", the same words as "open"; and gone back through',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/s.hm', 'echo att str 18 >> /ram/s.hm',
        'dq0 -p t -s 514 -c /ram/s.hm', 'lower portcullis', 'open the portcullis', 'close the portcullis', 'raise the portcullis', 'n', 's',
        'open the portcullis', 'close the portcullis', 'lower the portcullis', 'raise the portcullis', 's', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        let n = 0, r = 0;
        this.poked = [];
        dq0Where(this, m, l => {   // (The portcullis's square: north of where the character stands to close it; let down
          // (its state poked) as the character walks back to it from the far side, and before "raise" there)
          const w = this.where[this.where.length - 1];
          if (l === 'close the portcullis' && !this.pq) this.pq = [w.sn, w.x, w.y - 1];
          if ((l === 's' && ++n === 1) || (l === 'raise the portcullis' && ++r === 2)) {
            const p = this.pq && dq0Portal(m, ...this.pq), ok = !!p && p.kind === PK_PORTCULLIS && (p.b[p.state] & 1) === 1;
            if (ok) p.b[p.state] &= ~1;
            this.poked.push(ok);
          }
        });
      },
      check(m) {
        const T = dq0Turns(m), f = [], s = m.out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\[[ .]*\]|torch \d+(h\d+)?m?|UP \d+|facing [NSEW]+/g, ' ').replace(/\s+/g, ' ');
        const seen = (t, open) => t.after && t.after.sight && t.after.sight.doors.some(d => d.kind === 'PK_PORTCULLIS' && (d.state & 1) === open);
        const lo = T.of('lower portcullis'), op = T.after(lo, 'open the portcullis'), cl = T.after(op, 'close the portcullis'),
          ra = T.after(cl, 'raise the portcullis'), n = T.after(ra, 'n'), back = T.after(n, 's'), op2 = T.after(back, 'open the portcullis'),
          cl2 = T.after(op2, 'close the portcullis'), lo2 = T.after(cl2, 'lower the portcullis'), ra2 = T.after(lo2, 'raise the portcullis'), s2 = T.after(ra2, 's');
        // (What each line's told: the screen's text cut at the prompts, each line's echoes and what follows them, in turn)
        const chunks = s.split('> ').slice(1), told = new Map();
        for (let i = T.indexOf(lo), c = 0; i >= 0 && i < T.length; i++) {
          while (c < chunks.length && !chunks[c].startsWith(T[i].line)) c++;
          let w = '';
          for (; c < chunks.length && chunks[c].startsWith(T[i].line); c++) w += ' ' + chunks[c].slice(T[i].line.length);
          told.set(T[i].k, w.trim());
        }
        const said = t => told.get(t.k) || '';
        dq0Need(f, lo, 'E_PORTAL', { po: 'PO_LOWERED', pk: 'PK_PORTCULLIS' }, 'the portcullis not lowered first');
        for (const [t, w] of [[op, 'opened'], [ra, 'raised']]) {
          dq0Need(f, t, 'E_TASK', { te: 'TE_BEGUN' }, 'its winding not begun, ' + w);
          dq0Need(f, t, 'E_PORTAL', { po: 'PO_RAISED', pk: 'PK_PORTCULLIS' }, 'the portcullis not ' + w + ' by its winch');
          if (!seen(t, 1)) f.push('the portcullis not seen up, ' + w);
        }
        if (!/You set to winding the winch\. You raise the portcullis: it grinds up/.test(said(op))) f.push('"open": not wound up and told raised: ' + JSON.stringify(said(op)));
        if (said(op) !== said(ra)) f.push('"open" and "raise" told differently: ' + JSON.stringify(said(op)) + ' / ' + JSON.stringify(said(ra)));
        dq0Need(f, cl, 'E_PORTAL', { po: 'PO_LOWERED', pk: 'PK_PORTCULLIS' }, 'the portcullis not closed (let down)');
        if (!seen(cl, 0)) f.push('the portcullis not seen down, closed');
        const loose = x => x.replace(/^.*?(You loose)/, '$1');   // (The lowering's words, after the walk up's first)
        if (!/^You loose the winch: the portcullis crashes down\.$/.test(loose(said(cl))) || loose(said(cl)) !== loose(said(lo))) f.push('"close" and "lower" told differently: ' + JSON.stringify(said(cl)) + ' / ' + JSON.stringify(said(lo)));
        dq0Need(f, n, 'E_ARRIVE', {}, 'not gone through the portcullis raised');
        if (!n.after || !this.pq || !(n.after.pc.y < this.pq[2])) f.push('not gone north through it');
        if (this.poked.length !== 2 || !this.poked.every(x => x)) f.push('the portcullis not let down from the far side (the poke): ' + JSON.stringify(this.poked));
        if (!back.after || !this.pq || back.after.pc.x !== this.pq[1] || back.after.pc.y !== this.pq[2] - 1) f.push('not walked back to the portcullis, down, from the far side');
        for (const [t, w] of [[op2, 'opened'], [ra2, 'raised']]) {
          dq0Need(f, t, 'E_PORTAL', { po: 'PO_HEAVED', pk: 'PK_PORTCULLIS' }, 'the portcullis not heaved up from the far side, ' + w);
          if (!t.after || !this.pq || !(t.after.pc.y < this.pq[2])) f.push('not on the far side, ' + w);
          if (!seen(t, 1)) f.push('the portcullis not seen up from the far side, ' + w);
        }
        if (!/^You look at the portcullis\. You set to heaving at the portcullis\. You raise the portcullis: it rises, and catches/.test(said(op2)) || said(op2) !== said(ra2)) f.push('"open" and "raise" told differently from the far side: ' + JSON.stringify(said(op2)) + ' / ' + JSON.stringify(said(ra2)));
        for (const t of [cl2, lo2]) dq0Need(f, t, 'E_PORTAL', { po: 'PO_NO_WINCH', pk: 'PK_PORTCULLIS' }, '"' + t.line + '" from the far side: not its winch on the other side');
        if (said(cl2) !== said(lo2) || !/Its winch is on the other side/.test(said(cl2))) f.push('"close" and "lower" told differently from the far side: ' + JSON.stringify(said(cl2)) + ' / ' + JSON.stringify(said(lo2)));
        dq0Need(f, s2, 'E_ARRIVE', {}, 'not gone back through the portcullis heaved up');
        if (!s2.after || !this.pq || !(s2.after.pc.y > this.pq[2])) f.push('not back south through it');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0doorfar', near: ['map', 'parser'], what: "Dingus Quest Zero's door beyond the room's walls walked over to (HOUSE.md's out of reach, walked to first: \"close the door\", \"hit the door\", \"kick the door\" not beside it by the four ways, nor on the walls of the room it stands in: dq0's door_go; 2026-11-03, the check of the document), by the words and the hub: at seed 3 out through the cell's doorway into the passage (the rats held off, the scene's), the doors on the cell's wall asked about, one chosen: \"You head for the door.\", walked to beside it, then the command: the open door closed, the closed one battered with the torch, and kicked",
      init: 't_rc', cycles: 1400e6,
      machine: { input: (() => {
        const g = d => ['dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/' + d, 'step west', 's'];
        return dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/h', 'mkdir /ram/i', ...['g', 'h', 'i'].map(d => 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/' + d + '/theme.txt'),
          ...g('g'), 'close the door', '2', 'look', 'quit', ...g('h'), 'hit the door', '1', 'stop', 'quit', ...g('i'), 'kick the door', '1', 'stop', 'quit', 'echo $status']);
      })() },
      start(m) {
        const away = (h, L, K, at) => {
          const R = dq0Rec(m), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, due = now + 6000; let n = 0;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            const o = R.at(h[at(k) + L.aName + 12]), x = 32719 + n++, y = 32721;
            h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8; h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_FRIENDLY; h[at(k) + L.aDoing] = K.DO_ROAM;
            for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = due >>> (8 * i) & 255;
          }
        };
        dq0Poke(m, [1, 2, 3].flatMap(g => [{ g, line: 'step west', n: 1, call: away }, { g, line: 's', n: 1, call: away }, { g, line: g === 1 ? '2' : '1', n: 1, call: away }]));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out), G = o.split('% dq0 ').slice(1);
        if (G.length !== 3) return ['not three games: ' + G.length];
        const ans = T.filter(t => t.line === '2' || t.line === '1'), want = [['close the door', 32723, 'PO_CLOSED', /You close the door\./], ['hit the door', 32720, 'TE_BEGUN', /You set to battering the door with the torch\./], ['kick the door', 32720, 'TE_BEGUN', /You set to kicking the door\./]];
        if (ans.length !== 3) return ['the doors not chosen: ' + ans.length];
        want.forEach(([l, x, ev, re], i) => {
          const a = ans[i];
          if (!/Which door\? \(1\) the door ahead to your right \(closed\), \(2\) the one behind you to your right \(open\)/.test(G[i])) f.push(l + ': not asked which');
          if (!/You head for the door\./.test(G[i])) f.push(l + ': not told it heads for the door');
          if (!a.at || a.at.pc.x !== 32722 || a.at.pc.y !== 32724) f.push(l + ': not from the passage, off its wall');
          if (!a.after || a.after.pc.x !== x || a.after.pc.y !== 32724) f.push(l + ': not walked to beside it: ' + (a.after && [a.after.pc.x, a.after.pc.y]));
          if (!re.test(G[i])) f.push(l + ': the command not done there');
          if (!a.events.some(e => e.po === ev || e.te === ev)) f.push(l + ': no ' + ev);
        });
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0kept', meaning: true, what: "Dingus Quest Zero's lights kept (DUNGEON.md: light kept; the user's, 2026-10-16: \"why would all of the torches in the wall sconces be lit if no one is there to light them?\"): the kobold warren's Dung Tunnels (a mine, dq0 -T -T -T: four strata grown, the character on the second) at seed 527, the kobolds' keeping (their lair's chunk and their range, the denizens' places kept, KEPT_AT) lighting the room the character comes in to: lit by its sconce (\"On the wall to your right is a sconce, a lit torch in it.\"), the torch lit and tended (I_TENDED), the room's light kept (ROOM_LIT); the torch taken from it (a step nearer first), the room dark, a torch put back, the room's light as that torch is; at seed 535, a storeroom away from the kobolds dark, its sconce's torch cold, \"a torch in it\", lit there (light the torch in the sconce: the room lit by it, not tended), put out (dark again), lit again, kept in the save (-d) and carried on lit; every sconce in the item store at each lit and tended if and only if a keeping whose keepers live reaches it, each of those rooms' light kept; and the same seed again the same sconces, the same torches, the same places kept (a chunk always stocked the same: the world waits for a stratum's populations)",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 527 -c /ram/c.hm -T -T -T', 'step forward', 'take the torch from the sconce', 'put the torch in the sconce', 'quit',
        'dq0 -p t -s 535 -c /ram/c.hm -d /ram/kp -T -T -T', 'light the torch in the sconce', 'put out the torch in the sconce',
        'light the torch in the sconce', 'quit', 'dq0 -d /ram/kp', 'look', 'quit',
        'dq0 -p t -s 535 -c /ram/c.hm -T -T -T', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], LIT = 1, TENDED = 0x80;
        const sconces = s => s ? s.items.filter(i => i.name === 'sconce') : [];
        const torchIn = (s, c) => s.items.find(i => i.where === 'IW_ITEM' && i.x === c.n) || null;
        const roomLit = (s, c) => { const r = s.changes.filter(x => x.what === 'ROOM_LIT' && x.sn === c.sn && x.x === c.x && x.y === c.y); return r.length ? r[r.length - 1].tile : -1; };
        const kept = (s, c) => { const st = s.slots.find(x => x.n === c.sn); return !!st && s.kept.at.some(q => q.sn === c.sn && q.live &&
          Math.abs(c.x - (st.x0 + q.cx * 32 + 16)) <= q.range && Math.abs(c.y - (st.y0 + q.cy * 32 + 16)) <= q.range); };
        const near = (s) => sconces(s).filter(c => c.sn === s.pc.sn).sort((a, b) => Math.max(Math.abs(a.x - s.pc.x), Math.abs(a.y - s.pc.y)) - Math.max(Math.abs(b.x - s.pc.x), Math.abs(b.y - s.pc.y)))[0] || null;
        // (Each sconce: lit and tended where a live keeping reaches it, its room's light kept; else cold or none)
        const every = (s, what) => {
          if (!s || !s.kept) return f.push(what + ': no state');
          if (!s.kept.at.some(q => q.sn === 1 && q.live)) f.push(what + ': no keeping on the Dung Tunnels: ' + JSON.stringify(s.kept));
          for (const c of sconces(s)) {
            const t = torchIn(s, c), k = kept(s, c), lit = t && (t.flags & (LIT | TENDED)) === (LIT | TENDED);
            if (k !== !!lit) f.push(what + ': the sconce at ' + c.x + ',' + c.y + (k ? ' kept, its torch not lit and tended: ' : ' not kept, its torch lit and tended: ') + JSON.stringify(t));
            if (k && roomLit(s, c) !== 2) f.push(what + ': the kept sconce at ' + c.x + ',' + c.y + ' its room\'s light not kept (ROOM_LIT)');
          }
          if (!sconces(s).length) f.push(what + ': no sconces');
        };
        const g1 = T.of('dq0 -p t -s 527 -c /ram/c.hm -T -T -T'), a = g1.after;
        every(a, 'seed 527');
        const c1 = a && near(a), t1 = c1 && torchIn(a, c1);
        if (!a || !a.sight || !a.sight.lit) f.push('seed 527: the kept room not lit: ' + JSON.stringify(a && a.sight && { lit: a.sight.lit, dark: a.sight.dark }));
        if (!c1 || !t1 || (t1.flags & (LIT | TENDED)) !== (LIT | TENDED)) f.push('seed 527: the room\'s sconce\'s torch not lit and tended: ' + JSON.stringify(t1));
        const story = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, ''), w1 = story.indexOf('lit by smoky tallow lamps'), w2 = story.indexOf('is a sconce, a lit torch in it.');
        if (w1 < 0 || w2 < 0) f.push('seed 527: the kept room\'s words: ' + JSON.stringify(story.slice(story.indexOf('Type help'), story.indexOf('Type help') + 600)));
        const tk = T.of('take the torch from the sconce'), b = tk.after;
        if (!b || !b.sight || b.sight.lit || !c1 || roomLit(b, c1) !== 0) f.push('seed 527: the torch taken, the room still lit: ' + JSON.stringify(b && b.sight && b.sight.lit) + ' ' + (c1 && b ? roomLit(b, c1) : ''));
        if (!b || !c1 || torchIn(b, c1)) f.push('seed 527: the torch not taken from the sconce');
        const pt = T.of('put the torch in the sconce'), p = pt.after;
        const t4 = p && c1 && torchIn(p, c1), l4 = !!t4 && !!(t4.flags & LIT);   // (Whichever torch it put back: lit, the room lit; cold, dark)
        if (!t4 || !p.sight || !!p.sight.lit !== l4 || roomLit(p, c1) !== (l4 ? 2 : 0)) f.push('seed 527: a torch put back, the room\'s light not as its torch is: ' + JSON.stringify(t4) + ' ' + (p && p.sight && p.sight.lit));
        const g2 = T.of('dq0 -p t -s 535 -c /ram/c.hm -d /ram/kp -T -T -T'), d = g2.after;
        every(d, 'seed 535');
        const c2 = d && near(d), t2 = c2 && torchIn(d, c2);
        if (!d || !d.sight || d.sight.lit || d.sight.dark) f.push('seed 535: the storeroom away from the kobolds not dark (lit by the character\'s own torch alone): ' + JSON.stringify(d && d.sight && { lit: d.sight.lit, dark: d.sight.dark }));
        if (!c2 || !t2 || t2.flags & LIT || (d && kept(d, c2))) f.push('seed 535: the storeroom\'s sconce\'s torch not cold: ' + JSON.stringify(t2));
        if (story.indexOf('by the light of your torch. A barrel vault arches far overhead. There are two doors to your right (a doorway). You stand by a stairway leading up. On the wall to your left is a sconce, a torch in it.') < 0)
          f.push('seed 535: the storeroom\'s words: not dark, by the character\'s own torch, its sconce\'s torch cold');
        const l1 = T.of('light the torch in the sconce'), e = l1.after, t3 = e && c2 && torchIn(e, c2);
        if (!e || !e.sight || !e.sight.lit || !c2 || roomLit(e, c2) !== 2) f.push('seed 535: the sconce\'s torch lit, the room not lit by it');
        if (!t3 || (t3.flags & (LIT | TENDED)) !== LIT) f.push('seed 535: the torch lit in the sconce not lit, or tended: ' + JSON.stringify(t3));
        if (story.indexOf('You light the torch. You can see part of a storeroom, lit by smoky tallow lamps.') < 0 || story.indexOf('On the wall beside you is a sconce, a lit torch in it.') < 0)
          f.push('seed 535: the sconce\'s torch lit not told so');
        const po = T.of('put out the torch in the sconce'), o = po.after;
        if (!o || !o.sight || o.sight.lit || !c2 || roomLit(o, c2) !== 0) f.push('seed 535: the sconce\'s torch put out, the room still lit');
        const l2 = T.of('light the torch in the sconce', po.k + 1), q = l2.after;
        if (!q || !q.sight || !q.sight.lit) f.push('seed 535: lit again, the room not lit');
        const rs = T.of('dq0 -d /ram/kp'), r = rs.after;
        dq0Need(f, rs, 'E_ARRIVE', { how: 'A_RESUMED' }, 'seed 535: the game not carried on');
        if (!r || !r.kept || !d || JSON.stringify(r.kept) !== JSON.stringify(d.kept)) f.push('seed 535: the places kept not kept with the game: ' + JSON.stringify(r && r.kept));
        const c5 = r && c2 && sconces(r).find(c => c.sn === c2.sn && c.x === c2.x && c.y === c2.y), t5 = c5 && torchIn(r, c5);
        if (!r || !r.sight || !r.sight.lit || !t5 || (t5.flags & (LIT | TENDED)) !== LIT) f.push('seed 535: carried on, the storeroom lit by its sconce no longer: ' + JSON.stringify(t5));
        const g3 = T.of('dq0 -p t -s 535 -c /ram/c.hm -T -T -T', rs.k + 1), h = g3.after;
        const list = s => s ? JSON.stringify(s.items.filter(i => i.name === 'sconce' || (i.name === 'torch' && i.where === 'IW_ITEM')).map(i => [i.name, i.flags, i.sn, i.x, i.y, i.where === 'IW_ITEM' ? 0 : 1])) : '';
        if (!h || list(h) !== list(d)) f.push('seed 535 again: not the same sconces and torches: ' + list(h) + ' against ' + list(d));
        if (!h || !h.kept || JSON.stringify(h.kept) !== JSON.stringify(d.kept)) f.push('seed 535 again: not the same places kept: ' + JSON.stringify(h && h.kept));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0bearing', what: 'Dingus Quest Zero\'s bearings underground (slice 4.2, the user\'s, 2026-10-16: "Dwarves or other denizens that live underground don\'t have this problem, but humans would, so instead of direction heading, a check would need to be made"; 2026-10-23: a start below, come round from a fall, disoriented; HOUSE.md\'s bearings underground), by the hub\'s bearings (pc_skew, pc_lost), the referee\'s P_BEARING tests and the words: at seed 3 a human of little Awareness (hmchar\'s file, its Awareness 5) walks the passage east of the way in, which bends twice: tested, failed, its north drifted an eighth unknown to it (the status line\'s "facing SE", truly east); "turn left" truly north, told as it believes ("You turn to face northeast."); "face north" its believed north (truly northwest), "face northwest" truly west; forward back along the passage into the kitchen it began in, a landmark, its bearings set right ("You know this place: your bearings come back to you.", no test); east again and back twice, the third time east a critical failure: a quarter off and known ("You\'ve lost your bearings ...", "facing S?"), the status command\'s "Facing: south, you think"; kept with the game (dq0 -d) and carried on with it; a dwarf (hmchar -p dwarf, its Awareness as poor) the same way never tested, "You turn to face north."; a theme whose character comes round below from a fall (a hand-written theme\'s "start | fell", themegen\'s to fill out): the human tested as it begins (ten steps the harder), its north astray, the dwarf not; knocked senseless by the hand-written theme\'s rats (dq0senseless\'s), tested as it\'s out; and the kobold warren (seed 517: its story\'s fall through a ruin\'s floor, its start fallen in), tested as it begins and again as it jumps down its gatehouse\'s ledge, ten feet; and a way walked often the easier (the user\'s, 2026-10-23: by the map\'s passings, the senses\' count of the times it\'s come to see a chunk again; its torch put out five minutes and lit again a passing): the bending passage east tested at 2 steps new, 6 with its end come to four times, untested at twelve',
      bearings: true, init: 't_rc', cycles: 5000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml awa 5 >> /ram/c.hm',
        'hmchar -q -s 5 -p dwarf -o /ram/d.hm', 'echo ml awa 5 >> /ram/d.hm',
        'dq0 -p t -s 3 -c /ram/c.hm', 'go through the door to the east', 'turn left', 'face north', 'face northwest', 'f', 'go through the door to the east', 'turn around', 'f', 'go through the door to the east', 'turn around', 'f', 'go through the door to the east', 'status',
        'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -d /ram/k', 'go through the door to the east', 'quit', 'dq0 -p t -d /ram/k', 'look', 'quit',
        'dq0 -p t -s 3 -c /ram/d.hm', 'go through the door to the east', 'turn left', 'quit',
        'echo \'start    | fell\' > /ram/t.txt', 'dq0 -p t -s 4 -c /ram/c.hm -t /ram/t.txt', 'look', 'quit',
        'dq0 -p t -s 4 -c /ram/d.hm -t /ram/t.txt', 'look', 'quit',
        'cp /ram/c.hm /ram/s.hm', 'echo ml una 5 >> /ram/s.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/s.hm -T -d /ram/g', 'defend none', ...Array(12).fill('wait'), 'quit',
        'dq0 -p t -s 517 -c /ram/c.hm', 'turn half left', 'f', 'turn half right', 'f', 'u', 'u', 'turn around', 'jump down', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -d /ram/f', 'go through the door to the east', ...passings(3), 'turn around', 'f', 'go through the door to the east', ...passings(8), 'turn around', 'f', 'go through the door to the east', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), K = m.dq0.layout().K;
        const tests = t => t.replies.filter(q => q.req === 'slot0' && q.kind === K.RS_TEST && q.a === K.P_BEARING && q.steps < 64);   // (Not the time kept's: its steps from rs.h's TIME_STEPS on, 2026-10-31)
        const bear = (t, w) => dq0Ev(t, 'E_BEARING', { brg: w }).length;
        const at = (t, skew, lost, what) => { if (!t.after || t.after.pc.skew !== skew || t.after.pc.lost !== lost) f.push(what + ': its bearings ' + (t.after && [t.after.pc.skew, t.after.pc.lost]) + ', not ' + [skew, lost]); };
        const face = (t, h, what) => { if (!t.after || t.after.pc.face !== h) f.push(what + ': truly facing ' + (t.after && t.after.pc.face) + ', not ' + h); };
        // The human at seed 3
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm'), e1 = T.after(g, 'go through the door to the east'), tl = T.after(e1, 'turn left'), fn = T.after(tl, 'face north');
        const fw = T.after(fn, 'face northwest'), b1 = T.after(fw, 'f'), e2 = T.after(b1, 'go through the door to the east'), b2 = T.after(e2, 'f'), e3 = T.after(b2, 'go through the door to the east');
        const b3 = T.after(e3, 'f'), e4 = T.after(b3, 'go through the door to the east'), st = T.after(e4, 'status');
        if (!g.after || g.after.pc.skew || g.after.pc.lost) f.push('a new game, walked in by the way in, not knowing north');
        if (tests(e1).length !== 1 || tests(e1)[0].grade !== -1) f.push('the bending passage east: not one test, failed (' + JSON.stringify(tests(e1)) + ')');
        at(e1, 1, 0, 'the walk east, a marginal failure');
        face(e1, 2, 'the walk east');
        face(tl, 0, '"turn left"');
        face(fn, 7, '"face north", the north it believes in');
        face(fw, 6, '"face northwest"');
        if (tests(b1).length || !bear(b1, 'BRG_KNOWN')) f.push('back into the kitchen: not a landmark, told, untested (' + dq0Evs(b1) + ')');
        at(b1, 0, 0, 'back in the kitchen');
        if (tests(e2).length !== 1 || tests(e3).length !== 1 || tests(e4).length !== 1) f.push('east again: not tested each time');
        if (!bear(e4, 'BRG_LOST')) f.push('the third time east: not lost, known (' + dq0Evs(e4) + ')');
        at(e4, 2, 1, 'the third time east, a critical failure');
        inTurn(s, ['> turn left', 'You turn left, to face northeast.', '> face north', 'You turn to face north.',
          '> face northwest', 'You turn to face northwest.', '> f', 'You know this place: your bearings come back to you.',
          'You\'ve lost your bearings: you\'re no longer sure which way is north.', '> status', 'Facing: south, you think'], f, 'the words: ');
        inTurn(dq0StatusRows(out), ['facing SE', 'facing S?'], f, 'the status line: ');
        // Kept with the game, and carried on
        const k1 = T.of('dq0 -p t -s 3 -c /ram/c.hm -d /ram/k'), k2 = T.after(k1, 'go through the door to the east'), k3 = T.after(k2, 'dq0 -p t -d /ram/k'), k4 = T.after(k3, 'look');
        at(k2, 1, 0, 'the kept game\'s walk east');
        if (!k4.at || k4.at.pc.skew !== 1) f.push('the game carried on: its bearings not as kept (' + (k4.at && k4.at.pc.skew) + ')');
        // The dwarf
        const d = T.of('dq0 -p t -s 3 -c /ram/d.hm'), d1 = T.after(d, 'go through the door to the east'), d2 = T.after(d1, 'turn left');
        if (!d1.after || !d1.after.sum.northsense) f.push('the dwarf\'s summary not knowing north');
        if (tests(d1).length || !d1.after || d1.after.pc.skew) f.push('the dwarf tested, or astray (' + JSON.stringify(tests(d1)) + ')');
        inTurn(s.slice(s.indexOf('-c /ram/d.hm')), ['> turn left', 'You turn left, to face north.'], f, 'the dwarf\'s words: ');
        // Fallen in
        const h = T.of('dq0 -p t -s 4 -c /ram/c.hm -t /ram/t.txt'), hd = T.of('dq0 -p t -s 4 -c /ram/d.hm -t /ram/t.txt');
        if (tests(h).length !== 1) f.push('come round below from a fall: the human not tested as it begins (' + JSON.stringify(tests(h)) + ')');
        if (!h.after || !h.after.pc.skew) f.push('come round below from a fall: the human\'s north not astray (' + (h.after && h.after.pc.skew) + ')');
        if (tests(hd).length || !hd.after || hd.after.pc.skew) f.push('come round below from a fall: the dwarf tested, or astray');
        // Knocked senseless (the hand-written theme's rats, seed 3: dq0senseless's): tested as it's out, astray
        const o = T.of('dq0 -p t -s 3 -c /ram/s.hm -T -d /ram/g'), out1 = T.slice(o.k).find(t => t.after && (t.after.sum.cond & 2));
        if (!out1) f.push('never knocked senseless');
        else if (tests(out1).length !== 1) f.push('knocked senseless (' + out1.line + '): not tested as it\'s out (' + JSON.stringify(tests(out1)) + ')');
        // The kobold warren (seed 517): fallen in, tested as it begins; a jump down its ledge (10 feet), tested
        const w = T.of('dq0 -p t -s 517 -c /ram/c.hm'), j = T.after(w, 'jump down');
        if (tests(w).length !== 1) f.push('the warren\'s start, fallen in: not tested (' + JSON.stringify(tests(w)) + ')');
        if (tests(j).length !== 1) f.push('the jump down 10 feet: not tested (' + JSON.stringify(tests(j)) + ')');
        // Familiar ways (the map's passings of the passage's end, its chunk's: 4 four steps the easier, 12 untested)
        const v = T.of('dq0 -p t -s 3 -c /ram/c.hm -d /ram/f'), v1 = T.after(v, 'go through the door to the east'), v2 = T.after(T.after(v1, 'f'), 'go through the door to the east');
        const v3 = T.after(T.after(v2, 'f'), 'go through the door to the east');
        if (tests(v1).length !== 1 || tests(v1)[0].steps !== 2) f.push('the passage east, new: not tested at 2 steps (' + JSON.stringify(tests(v1)) + ')');
        if (tests(v2).length !== 1 || tests(v2)[0].steps !== 6) f.push('the passage east, its end come to 4 times: not tested at 6 steps (' + JSON.stringify(tests(v2)) + ')');
        if (tests(v3).length) f.push('the passage east, its end come to 12 times: tested (' + JSON.stringify(tests(v3)) + ')');
        this.notes = [e1, e2, e3, e4, h, ...(out1 ? [out1] : []), w, j, v1, v2, v3].map(t => t.line + ': ' + JSON.stringify(tests(t).map(q => [q.grade, q.result])) + ' ' + (t.after && [t.after.pc.skew, t.after.pc.lost]));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0travel', what: 'Dingus Quest Zero\'s travel to a place it knows (slice 4.2, the user\'s, 2026-10-16: "name this room "The Mausoleum" and then you could say "I want to go back to the mausoleum" and you would be on your way, and the game would move you at your current rate of speed back toward that named place, as long as your direction sense/orientation checks held up.  Orientation checks would be aided if good maps are used"; HOUSE.md\'s travel), by the world\'s events (E_TASK\'s LT_TRAVEL, the legs\' E_ARRIVE), where the character ends, the chunks\' memory (the cache\'s, each chunk\'s kept) and the words: at seed 3\'s caves (dq0 -T, knowing north: -N) the cave by the stairs named the Grotto, three walks west out of its chunk, "go back to the grotto" there a leg at a time (TE_BEGUN, two legs or more, TE_THERE under way: in its room), again there already, "return to the grotto" the same, then the same walks out and "go back to the grotto" again, stopped by a command typed as it begins ("turn around" typed as "You head back toward the Grotto." is told, as a player would: TE_STOPPED, not there); the same walks and two hours\' waits twice (its torch out between), its memory of the Grotto\'s chunk under a tenth: refused (TE_FORGOT, no walk), and again in the light of its spare torch (the chunk it\'s in seen, not the Grotto\'s); a map drawn as it went (paper, charcoal, a lantern lit), its memory gone after four hours\' waits but its map carried showing the Grotto, in light to read it: on its way by the map (begun, walked, not refused: there, or stopped by a giant rat in its way); a wrong belief taking it astray (its bearings tested, the hunter\'s Awareness: a long way walked, its north an eighth off since it last saw the Grotto, not knowing it) its first leg another way than the same walks\' knowing north (-N) take; at seed 4 the kitchen past the barred door named the Gallery, "go back to where I came in" stopped by a shut door it hasn\'t opened, the way nearest the bedchamber (E_NOWAY N_DOOR, no walk); "go back to the kitchen" (a kind of room, the theme\'s word) there already (its caves dry and unfallen, dq0 -W -U: the walks and the journeys as they were before the water, slice 4.9\'s, and before the rubble, slice 4.10\'s: at seed 3 the caves\' rubble made the astray walk\'s first leg the one knowing north takes, so -U keeps the scene the comparison was made for; the test is about travel, not wading or clambering)',
      bearings: true, init: 't_rc', cycles: 7000e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -W -U -N', 'd', 'call this room the Grotto', 'n', 'go through the passage on the left', 'w', 'w', 'go back to the grotto', 'go back to the grotto',
        'return to the grotto', 'n', 'go through the passage on the left', 'w', 'w', 'go back to the grotto', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -W -U -N -k torch', 'd', 'call this room the Grotto', 'n', 'go through the passage on the left', 'w', 'w', 'wait 2 hours', 'wait 2 hours',
        'go back to the grotto', 'light the torch', 'go back to the grotto', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -W -U -N -k paper,charcoal,lantern', 'douse the torch', 'drop the torch', 'light the lantern', 'put the charcoal in the pouch', 'map as I go', 'd',
        'call this room the Grotto', 'n', 'go through the passage on the left', 'w', 'w', 'stop mapping', 'wait 100 minutes', 'wait 100 minutes', 'wait 1 hour', 'go back to the grotto', 'quit',
        ...['', ' -N'].flatMap(n => ['dq0 -p t -s 3 -c /ram/c.hm -T -W -U' + n, 'd', 'call this room the Grotto', 'n', 'go through the passage on the left', 'f', 'f', 'f', 'l', 'f', 'r', 'f', 'l', 'f',
          'r', 'f', 'f', 'l', 'f', 'go back to the grotto', 'quit']),
        'dq0 -p t -s 4 -c /ram/c.hm -N', 'take the bar', 'open', 'n', 'e', 'n', 'call this room the Gallery', 'go back to where I came in',
        'go back to the kitchen', 'quit', 'echo $status'].map(l => 'ā' + l + '\r').join('') },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'go back to the grotto', n: 3, words: 'You head back toward the Grotto.', text: 'turn around\r' }]);   // (Typed as it's told)
        dq0Poke(m, [{ g: 4, line: 'go back to the grotto', n: 1, hub: { pcSkew: 1 } }]);   // (Astray: its north an eighth off as it sets out, the scene set: the cave's diagonal ways, 2026-10-31, left its long walk's bearings tests true)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out);
        const tr = (t, te) => dq0Ev(t, 'E_TASK', { task: 'LT_TRAVEL', te });
        const legs = t => dq0Ev(t, 'E_ARRIVE').length;
        // (The named room's chunk and room in a state, and the character in it; what it keeps of that chunk now, 0-100: the
        // world's kept_left, the map's rule)
        const named = (st, nm) => st && st.names.find(q => q.name.toLowerCase() === nm.toLowerCase());
        const chunkOf = (st, sn, x, y) => { const sl = st.slots.find(q => q.n === sn); if (!sl) return null;
          const cx = (x - sl.x0) >> 5, cy = (y - sl.y0) >> 5; return { c: st.chunks.find(q => q.sn === sn && q.cx === cx && q.cy === cy), ix: (x - sl.x0) & 31, iy: (y - sl.y0) & 31, cx, cy }; };
        const inRoom = (st, nm) => { const r = named(st, nm); if (!r || st.pc.sn !== r.sn) return false;
          const a = chunkOf(st, r.sn, r.x, r.y), b = chunkOf(st, st.pc.sn, st.pc.x, st.pc.y);
          if (!a || !b || !a.c || a.cx !== b.cx || a.cy !== b.cy) return false;
          return a.c.rooms.some(q => a.ix >= q.x && a.ix < q.x + q.w && a.iy >= q.y && a.iy < q.y + q.h && b.ix >= q.x && b.ix < q.x + q.w && b.iy >= q.y && b.iy < q.y + q.h); };
        const left = (st, c) => { let age = ((Math.floor(st.pc.now / 256) & 0xFFFF) - c.m) & 0xFFFF, mf = st.sum.mapfade, u = (mf << 1) + (mf >> 2) + (mf >> 4) + 1, q = c.q;
          if (c.n >= 4) u <<= 1; if (c.n >= 12) u <<= 1; while (age >= u && q) { age -= u; q = q > 10 ? q - 10 : 0; } return q; };
        const kept = (st, nm) => { const r = named(st, nm), a = r && chunkOf(st, r.sn, r.x, r.y); return a && a.c; };
        const at = (t, what) => { if (!t.at || !t.after) { f.push(what + ': no state'); return false; } return true; };
        // The Grotto gone back to, knowing north
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -W -U -N'), b1 = T.after(g, 'go back to the grotto'), b2 = T.after(b1, 'go back to the grotto');
        const b3 = T.after(b2, 'return to the grotto');
        if (at(b1, 'the first journey')) {
          if (!tr(b1, 'TE_BEGUN').length || legs(b1) < 2 || !tr(b1, 'TE_THERE').filter(e => e.b & 0x40).length) f.push('"go back to the grotto": not begun, two legs or more, there under way (' + dq0Evs(b1) + ')');
          if (!inRoom(b1.after, 'the Grotto')) f.push('"go back to the grotto": not in the Grotto at its end: ' + JSON.stringify(b1.after.pc && [b1.after.pc.x, b1.after.pc.y]));
          if (inRoom(b1.at, 'the Grotto')) f.push('the first journey begun in the Grotto');
        }
        for (const [t, l] of [[b2, 'again'], [b3, '"return to the grotto"']]) {
          if (!tr(t, 'TE_THERE').filter(e => !(e.b & 0x40)).length || legs(t)) f.push(l + ': not there already, no walk (' + dq0Evs(t) + ')');
        }
        // Stopped by a command typed as it begins, the same walks out again
        const b4 = T.after(b3, 'go back to the grotto'), bt = T.after(b4, 'turn around'), stops = [...tr(b4, 'TE_STOPPED'), ...tr(bt, 'TE_STOPPED')];
        if (bt.k < 0 || bt.game !== b4.game) f.push('"turn around" not typed as the journey began');
        else if (!tr(b4, 'TE_BEGUN').length || !stops.length || tr(b4, 'TE_THERE').length || tr(bt, 'TE_THERE').length) f.push('a command typed as it goes: the journey not stopped (' + dq0Evs(b4) + ' ' + dq0Evs(bt) + ')');
        inTurn(s, ['> go back to the grotto', 'You head back toward the Grotto.', 'You\'re back in the Grotto.', '> go back to the grotto',
          'You\'re in the Grotto already.', '> return to the grotto', 'You\'re in the Grotto already.', '> go back to the grotto',
          'You head back toward the Grotto.', '> turn around', 'You stop travelling.'], f, 'the words: ');
        // Forgotten: two hours' waits twice, out of the Grotto's chunk
        const h = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -W -U -N -k torch'), h1 = T.after(h, 'go back to the grotto'), h2 = T.after(h1, 'go back to the grotto');
        for (const [t, l] of [[h1, 'in the dark'], [h2, 'in its torch\'s light']]) {
          if (!at(t, 'forgotten ' + l)) continue;
          const c = kept(t.at, 'the Grotto');
          if (!c || left(t.at, c) >= 10) f.push('forgotten ' + l + ': the Grotto\'s chunk kept still: ' + (c ? left(t.at, c) : 'no chunk'));
          if (!tr(t, 'TE_FORGOT').length || legs(t) || t.after.pc.x !== t.at.pc.x || t.after.pc.y !== t.at.pc.y) f.push('forgotten ' + l + ': not refused, or walked (' + dq0Evs(t) + ')');
        }
        if (h2.at && !h2.at.pc.light) f.push('the spare torch not lit');
        inTurn(s.slice(s.indexOf('-k torch')), ['> go back to the grotto', 'You don\'t remember the way to the Grotto.'], f, 'forgotten\'s words: ');
        // A map carried after the memory's gone
        const p = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -W -U -N -k paper,charcoal,lantern'), p1 = T.after(p, 'go back to the grotto');
        if (at(p1, 'the map\'s journey')) {
          const c = kept(p1.at, 'the Grotto');
          if (!c || left(p1.at, c) >= 10) f.push('the map\'s: the Grotto still remembered: ' + (c ? left(p1.at, c) : 'no chunk'));
          if (!c || !p1.at.pc.map || c.map !== p1.at.pc.map) f.push('the map\'s: the Grotto\'s chunk not drawn on the map carried: ' + JSON.stringify(c && [c.map, p1.at.pc.map]));
          if (!p1.at.pc.light) f.push('the map\'s: no light to read it by');
          if (!tr(p1, 'TE_BEGUN').length || !legs(p1) || tr(p1, 'TE_FORGOT').length ||
            !((tr(p1, 'TE_THERE').length && inRoom(p1.after, 'the Grotto')) || dq0Ev(p1, 'E_NOWAY', { why: 'N_DEN' }).length))
            f.push('the map\'s: not on its way there by the map (there, or a denizen in its way: ' + dq0Evs(p1) + ')');
        }
        // Astray, by a belief gone wrong since it saw the Grotto: its first leg another way than knowing north takes
        const a = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -W -U'), a1 = T.after(a, 'go back to the grotto');
        const n = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -W -U -N', g.k + 1), n1 = T.after(n, 'go back to the grotto');
        if (at(a1, 'astray') && at(n1, 'knowing north')) {
          const c = kept(a1.at, 'the Grotto');
          if (!c || a1.at.pc.lost || !((a1.at.pc.skew - c.sk) & 7)) f.push('astray: its belief not off since it saw the Grotto, or known lost: ' + JSON.stringify(c && [a1.at.pc.skew, c.sk, a1.at.pc.lost]));
          if (a1.at.pc.x !== n1.at.pc.x || a1.at.pc.y !== n1.at.pc.y) f.push('astray: not set out from where the walks knowing north did');
          const first = t => (dq0Ev(t, 'E_ARRIVE')[0] || {}).t;
          if (!tr(a1, 'TE_BEGUN').length || !first(a1) || first(a1) === first(n1)) f.push('astray: its first leg the one knowing north takes (' + dq0Evs(a1) + ' | ' + dq0Evs(n1) + ')');
        }
        // A shut door it hasn't opened, the way nearest; a kind of room
        const d = T.of('dq0 -p t -s 4 -c /ram/c.hm -N'), d1 = T.after(d, 'go back to where I came in'), d2 = T.after(d1, 'go back to the kitchen');
        if (at(d1, 'the shut door')) {
          if (!inRoom(d1.at, 'the Gallery')) f.push('the shut door: not set out from the Gallery');
          if (!dq0Ev(d1, 'E_NOWAY', { why: 'N_DOOR' }).length || tr(d1, 'TE_THERE').length) f.push('the shut door: not stopped by it, walked up to it (' + dq0Evs(d1) + ')');
        }
        if (!tr(d2, 'TE_THERE').filter(e => e.b === (0x80 | 8)).length) f.push('"go back to the kitchen": not there already, the kitchen\'s kind (' + dq0Evs(d2) + ')');
        inTurn(s.slice(s.indexOf('-s 4')), ['> go back to where I came in', 'You head back toward where you came in.', 'The door is shut: open it first.',
          '> go back to the kitchen', 'You\'re in the kitchen already.'], f, 'the shut door\'s words: ');
        this.notes = [b1, b4, h1, h2, p1, a1, n1, d1].map(t => t.line + ': ' + dq0Evs(t).slice(0, 300));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0traps', what: 'Dingus Quest Zero\'s traps (DUNGEON.md: traps; the world\'s walk stops on one and sets it off, the referee judges it), by the world\'s events (the referee\'s dice the commands\', in their order, at the turns pace: HarnMaster\'s at these seeds): a door\'s blade, set off as the door\'s forced open (seed 16, dq0 -T); the next door bursting at the first try (its load within its free pounds by Strength, D13\'s: a step the stronger; it held at the first before), open at the second; then a plate in a storeroom\'s floor walked onto (the walk stopped on it), its blade striking (HC_STRIKE: a wound), the trap seen after, sprung; then (seed 7, a thief, hmchar -s 1 -u feudal -c thief, Lockcraft 95) a hidden dart trap on a door found by the third search, the door left shut while it\'s known, the trap made out (its mechanism assessed), then disarmed (Lockcraft: two checks, the user\'s), and the door opened',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 16 -c /ram/c.hm -T', 'face east', 'force', 'e',
        'force', 'force', 'e', 'e', 'quit', 'hmchar -q -s 1 -u feudal -c thief -o /ram/t.hm', 'echo ml lck 95 >> /ram/t.hm', 'dq0 -p t -s 7 -c /ram/t.hm', 'search', 'search',
        'search', 'face south', 'open', 'disarm', 'disarm', 'open', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], fo = T.of('force');
        dq0Need(f, fo, 'E_PORTAL', { po: 'PO_FORCED' }, 'the door not forced open');
        dq0Need(f, fo, 'E_TRAP', { tk: 'TK_BLADE', tt: 'TT_DOOR' }, 'the door\'s blade not set off as it opened');
        const f2 = T.after(T.after(fo, 'e'), 'force'), f3 = T.after(f2, 'force');
        dq0Need(f, f2, 'E_PORTAL', { po: 'PO_FORCED' }, 'the next door not bursting at the first try');
        dq0Need(f, f3, 'E_PORTAL', { po: 'PO_IS_OPEN' }, 'the next door not open at the second');
        const e2 = T.after(T.after(f3, 'e'), 'e');
        dq0Need(f, e2, 'E_ARRIVE', { how: 'A_STOPPED' }, 'the walk not stopped on the plate');
        dq0Need(f, e2, 'E_TRAP', { tk: 'TK_BLADE', tt: 'TT_PLATE', to: 'TO_SPRUNG' }, 'the plate\'s blade not set off');
        dq0Need(f, e2, 'E_HARM', { hc: 'HC_STRIKE', band: b => b > 0 }, 'the blade not a wound');
        if (!e2.after || !e2.after.sight || !e2.after.sight.traps.some(p => p.kind === 'TK_BLADE' && (p.state & 1))) f.push('the trap not seen, sprung');
        const g = T.of('dq0 -p t -s 7 -c /ram/t.hm'), s1 = T.after(g, 'search'), s2 = T.after(s1, 'search'), s3 = T.after(s2, 'search');
        for (const s of [s1, s2]) dq0Need(f, s, 'E_PORTAL', { po: 'PO_NOTHING' }, 'something found by the first searches');
        dq0Need(f, s3, 'E_TRAP', { tk: 'TK_DARTS', tt: 'TT_DOOR', to: 'TO_FOUND' }, 'the dart trap not found by the third search');
        const o = T.after(s3, 'open'), d1 = T.after(o, 'disarm'), d2 = T.after(d1, 'disarm'), o2 = T.after(d2, 'open');
        dq0Need(f, o, 'E_PORTAL', { po: 'PO_TRAPPED' }, 'the door opened with its known trap on it');
        dq0Need(f, d1, 'E_TRAP', { to: 'TO_ASSESSED' }, 'the mechanism not made out first');
        dq0Need(f, d2, 'E_TRAP', { to: 'TO_DISARMED' }, 'the trap not disarmed');
        dq0Need(f, o2, 'E_PORTAL', { po: 'PO_OPENED' }, 'the door not opened, disarmed');
        dq0Never(f, o2, 'E_TRAP', {}, 'the trap went off, disarmed');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pure', what: 'Dingus Quest Zero\'s maker pure (move 2.2 of Phase 2, "one authority on state": generation only, the same seed always the same chunk; the change records the world\'s, dq0w/store.c, laid over a chunk the maker makes again): at seed 16 (dq0 -T -k pick) the chunks the maker made at once, read from the hub\'s cache; then a game played and kept (dq0 -d: a door forced open, the blade trap on it set off, the rock dug through beside the passage), and carried on (its chunks made again by a fresh maker from the seed, the records laid over them from the save\'s region files): each chunk made again the same as it was live before the game was kept, and the same as the maker first made it but for what the records say (a portal\'s state, a trap\'s, a square\'s kind and floor, an opening), and the records saying each of them',
      meaning: true,
      init: 't_rc', cycles: 2500e6,
      machine: { input: '\u0101bind -a /sd/0/games /bin\r\u0101hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\r\u0101dq0 -p t -s 16 -c /ram/c.hm -T -k pick\r' +
        ['look', 'quit', 'echo pure-a'].map(l => '\u0101' + l + '\r').join('') + '\u0101dq0 -p t -s 16 -c /ram/c.hm -T -k pick -d /ram/p\r' +
        ['face east', 'force', 'e', 'dig north', 'quit', 'echo pure-b'].map(l => '\u0101' + l + '\r').join('') + '\u0101dq0 -p t -d /ram/p\r' +
        ['look', 'quit', 'echo pure-c', 'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['% echo $status'],
      // (The hub's live chunks taken as each game has ended, its echo typed: the maker's first (a), the game's as it
      // was kept (b), the ones made again as it's carried on (c))
      start(m) {
        const run = m.run, snaps = this.snaps = {};
        m.run = lim => {
          run(lim);
          for (const k of ['a', 'b', 'c']) if (!snaps[k] && m.out.includes('pure-' + k)) snaps[k] = liveChunks(m, 0);
        };
      },
      check(m, out) {
        const f = [], { a, b, c } = this.snaps;
        if (!a || !b || !c) return ['the chunks not read as each game ended: ' + ['a', 'b', 'c'].filter(k => !this.snaps[k]).join(', ')];
        const sum = { portal: 0, trap: 0, square: 0, opening: 0 };
        let same = 0, made = 0;
        for (const [at, k] of Object.entries(c)) {
          if (b[at]) {
            const d = chunkDiff(b[at], k), n = d.portal + d.trap + d.square + d.opening + d.other.length;
            if (n) f.push(at + ': made again not as it was kept: ' + JSON.stringify(d));
            else same++;
          }
          if (a[at]) {
            const d = chunkDiff(a[at], k);
            if (d.other.length) f.push(at + ': made again, its bytes ' + d.other.slice(0, 8).join(', ') + ' not the maker\'s first');
            for (const w of Object.keys(sum)) sum[w] += d[w];
            made++;
          }
        }
        if (!same) f.push('no chunk both live as the game was kept and made again');
        if (!made) f.push('no chunk both made first and made again');
        if (!sum.portal) f.push('no portal\'s state changed (the door forced)');
        if (!sum.trap) f.push('no trap\'s state changed (the blade set off)');
        if (!sum.square) f.push('no square changed (the rock dug)');
        this.notes = [Object.keys(c).length + ' chunk(s) made again, ' + same + ' as kept, ' + made + ' against the maker\'s first: ' + JSON.stringify(sum)];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stairtrap', meaning: true, what: 'Dingus Quest Zero\'s walks to stairs and ladders checking their way for traps, as the other walks do (the user\'s, 2026-10-16), by the world\'s events: at seed 38 the character goes round to the kitchen\'s far door, clear of the covered pit hidden between it and the stairs up to the way out (not noticed in passing: the referee\'s, HarnMaster\'s at this seed), then up (u): the walk back to the stairs stops on the pit, which gives way under it (a fall, a climb out begun, a slip ending it at its floor: D13\'s, slice 4.4\'s), the pit seen open; climbed out again (climb up: a slip again, then out); then the stairs again, the pit it knows of gone round, and the way out reached (shut fast), no second fall',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 38 -c /ram/c.hm', 'se', 'walk to the door east',
        'u', 'climb up', 'climb up', 'u', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], w = T.of('walk to the door east'), u1 = T.of('u'), u2 = T.after(u1, 'u');
        dq0Need(f, w, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'not gone round to the far door');
        dq0Never(f, w, 'E_TRAP', {}, 'a trap set off going round');
        dq0Need(f, u1, 'E_ARRIVE', { how: 'A_STOPPED' }, 'the walk to the stairs not stopped on the pit');
        dq0Need(f, u1, 'E_TRAP', { tk: 'TK_COVERED', to: 'TO_SPRUNG' }, 'the covered pit didn\'t give way');
        dq0Need(f, u1, 'E_HARM', { hc: 'HC_FALL' }, 'no fall into the pit');
        dq0Need(f, u1, 'E_CLIMB', { how: 'K_BEGUN' }, 'no climb out begun');
        dq0Need(f, u1, 'E_CLIMB', { how: 'K_SLID' }, 'the climb out not ended by its slip (D13\'s: slid back to the pit\'s floor)');
        const c1 = T.of('climb up'), up = [[], []];   // (Out at the first climb up or the second, as the ruleset's dice fall)
        dq0Need(up[0], c1, 'E_CLIMB', { how: 'K_UP' }, '');
        dq0Need(up[1], T.after(c1, 'climb up'), 'E_CLIMB', { how: 'K_UP' }, '');
        if (up[0].length && up[1].length) f.push('not climbed out of the pit again');
        if (!u1.after || !u1.after.sight || !u1.after.sight.traps.some(p => p.kind === 'TK_COVERED' && (p.state & 1))) f.push('the pit not seen open');
        dq0Need(f, u2, 'E_NOWAY', { why: 'N_SHUT' }, 'the way out not reached, the pit gone round');
        dq0Never(f, u2, 'E_TRAP', {}, 'the pit gave way again (not gone round)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rebar', meaning: true, what: 'Dingus Quest Zero\'s doors barred again (the user\'s, 2026-10-16), by the world\'s events and the bar\'s record (a portal\'s part, IW_PORTAL, on the square of its side, or a thing carried or lying): at seed 4 the door\'s bar lifted out of its brackets and taken, then put back (bar the door: from its side, the door shut): out of the pack, in its brackets again, the door barred (opened: the bar lifted out again, at its feet); not while the door stands open; put back from the floor (put the bar back in its brackets); kept (dq0 -d), the door still barred and the bar in it as the game carries on (their words: dq0wordsfix)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 4 -c /ram/c.hm -d /ram/g', 'take the bar',
        'bar the door', 'open', 'bar the door', 'close', 'put the bar back in its brackets', 'quit', 'dq0 -p t -d /ram/g', 'open', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        // (The bar: in its brackets, IW_PORTAL on the square of its side; carried; or lying where the character stands)
        const bar = s => { const b = s && s.items.filter(i => i.name === 'bar'); if (!b) return '?';
          if (dq0Carried(s).some(i => i.name === 'bar')) return 'carried';
          return b.some(i => i.where === 'IW_PORTAL' && i.sn === s.pc.sn && i.x === s.pc.x && i.y === s.pc.y) ? 'brackets' :
            dq0Here(s).some(i => i.name === 'bar') ? 'floor' : 'gone'; };
        const want = (l, po, where, from = 0) => {
          const t = T.of(l, from);
          dq0Need(f, t, 'E_PORTAL', { po }, l + ': not ' + po);
          if (bar(t.after) !== where) f.push(l + ': the bar ' + bar(t.after) + ', not ' + where);
          return t;
        };
        const s0 = T.of('dq0 -p t -s 4 -c /ram/c.hm -d /ram/g').after;      // (Its bar's side a square off, the door's)
        if (!s0 || !s0.items.some(i => i.name === 'bar' && i.where === 'IW_PORTAL' && Math.abs(i.x - s0.pc.x) + Math.abs(i.y - s0.pc.y) <= 1)) f.push('the door by it not barred to begin with');
        want('take the bar', 'PO_LIFTED', 'carried');
        const b = want('bar the door', 'PO_REBARRED', 'brackets');
        const o = want('open', 'PO_UNBARRED', 'floor');
        if (!o.after || !o.after.sight || !o.after.sight.doors.some(d => d.rel === 0 && (d.state & 1))) f.push('the door ahead not open');
        want('bar the door', 'PO_SHUT_FIRST', 'floor', o.k + 1);
        want('close', 'PO_CLOSED', 'floor');
        const p = want('put the bar back in its brackets', 'PO_REBARRED', 'brackets');
        const c = T.of('dq0 -p t -d /ram/g');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (bar(c.after) !== 'brackets') f.push('the bar not in its brackets as the game carries on: ' + bar(c.after));
        want('open', 'PO_UNBARRED', 'floor', c.k + 1);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0doorlies', meaning: true, near: ['map', 'parser'], what: 'Dingus Quest Zero\'s "the door on the left" (the user\'s playtest, 2026-11-02: "if you say walk to the door on the left it doesn\'t always do what you expect"; HOUSE.md\'s going to anything): at seed 5, in a hall of four doors, from the centre and two corners, facing each of the four ways, each way a door can be said to lie: it means what the look\'s words mean by the facing at that moment (the record\'s rel its bearing as it faces, doors remembered too); the one that lies exactly so is walked up to, the nearest ahead of several; none exactly so, the one on that side (half the room), of several the question; none on that side, "You haven\'t seen a door that way." and no step',
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm', 'go to the far left corner', 'go to the far right corner', 'go to the near left corner',
        ...['go to the centre', 'go to the near left corner', 'go to the far right corner'].flatMap(p => ['face north', 'face east', 'face south', 'face west'].flatMap(q =>
          ['on the left', 'on the right', 'ahead', 'behind me', 'ahead to my left', 'behind me on my right'].flatMap(w => [p, q, 'walk to the door ' + w]))),
        'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], ans = dq0Answers(out), WANT = { 'on the left': 6, 'on the right': 2, 'ahead': 0, 'behind me': 4, 'ahead to my left': 7, 'behind me on my right': 3 };
        const HX = [0, 1, 1, 1, 0, -1, -1, -1], HY = [-1, -1, 0, 1, 1, 1, 0, -1], seen = {}, kinds = { one: 0, side: 0, asked: 0, none: 0 };
        const bearing = (dx, dy) => { const ax = Math.abs(dx), ay = Math.abs(dy); return ax * 5 < ay * 2 ? (dy < 0 ? 0 : 4) : ay * 5 < ax * 2 ? (dx > 0 ? 2 : 6) : dx > 0 ? (dy < 0 ? 1 : 3) : dy > 0 ? 5 : 7; };
        const apart = (a, b) => Math.max(Math.abs(a.x - b.x), Math.abs(a.y - b.y));
        for (const t of T) {
          const w = /^walk to the door (.*)$/.exec(t.line);
          if (!w) continue;
          const n = seen[t.line] = (seen[t.line] || 0) + 1, text = (ans.filter(y => y.cmd === t.line)[n - 1] || { text: '' }).text, pc = t.at && t.at.pc, s = t.at && t.at.sight;
          if (!pc || !s || !t.after) { f.push(t.line + ': not played'); continue; }
          const doors = [...s.doors, ...s.known].filter(d => !(d.rel & 8) && d.kind === 'PK_DOOR'), to = (d) => (bearing(d.x - pc.x, d.y - pc.y) - pc.face) & 7, want = WANT[w[1]], h = (pc.face + want) & 7, at = 'at ' + pc.x + ',' + pc.y + ' facing ' + pc.face + ': ' + t.line;
          for (const d of doors) if ((d.rel & 7) !== to(d)) f.push(at + ': the look\'s record has a door at ' + (d.rel & 7) + ', its bearing as it faces ' + to(d));
          const exact = doors.filter(d => to(d) === want), side = doors.filter(d => (d.x - pc.x) * HX[h] + (d.y - pc.y) * HY[h] > 0), cand = exact.length ? exact : side;
          const ahead = cand.filter(d => to(d) === 0).sort((a, b) => apart(a, pc) - apart(b, pc)), one = cand.length === 1 ? cand[0] : ahead[0];
          if (!cand.length) {
            ++kinds.none;
            if (!text.includes('You haven\'t seen a door that way.') || apart(t.after.pc, pc)) f.push(at + ': none that way, but: ' + text.slice(0, 120));
          } else if (one) {
            ++kinds[exact.length ? 'one' : 'side'];
            if (apart(t.after.pc, one) > 1) f.push(at + ': not walked up to the door at ' + one.x + ',' + one.y + ' (at ' + t.after.pc.x + ',' + t.after.pc.y + '): ' + text.slice(0, 120));
          } else {
            ++kinds.asked;
            if (!text.includes('Which door') || apart(t.after.pc, pc)) f.push(at + ': several that way, not asked: ' + text.slice(0, 120));
          }
        }
        if (!kinds.one || !kinds.side || !kinds.asked || !kinds.none) f.push('not every kind of answer met: ' + JSON.stringify(kinds));
        return f;
      },
      sdcard: true
    },
    {
      // (dq0sky=0, 2026-11-03, a scene's: seed 7's second pot not stocked under the skylight, "head for the pot" nothing to ask)
      name: 'dq0moving', meaning: true, near: ['map', 'parser'], what: 'Dingus Quest Zero\'s moving (the user\'s playtest, 2026-11-02: DECISIONS.md; HOUSE.md\'s straight walks, going to anything, crossing, inspecting walks over first), by the words told and the hub: at seed 8 a way word walks straight: north to the wall, told; south to a closed door, told; again, refused beside it; a door off to the side never walked to; a shut door it has handled opened as it goes, and on through; at seed 7 go to the centre (its sum near $8000), cross the room, go across the room, approach, walk to (no pace words), walk over to, head for, which asked of two pots, beside it not on it, "You haven\'t seen a rat here."; examine, assess out of reach walked over to, their words short; a plain look at from where it stands; things and doors remembered in the dark, named, walked to; leave, which asked; a walked-to take told short, no look, no "at your feet", and with hands and pack full refused as it gets there; a denizen gone up to with no blow, and where it was last seen (seed 2)',
      init: 't_rc', cycles: 5000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 8 -c /ram/c.hm', 'n', 's', 's', 'e', 'go through the door to the south', 'open', 'close', 'n', 's', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm', 'go to the centre', 'cross the room', 'go across the room', 'leave', 'go outside', 'go out', 'go to the rat', 'go to the far right corner', 'approach the pot', 'go to the far right corner', 'walk to the pot', 'go to the far right corner', 'head for the pot', '1',
        'go to the far left corner', 'walk over to the sconce', 'go to the far left corner', 'examine the sconce', 'go to the far left corner', 'assess the sconce', 'drop the backpack', 'go to the far left corner', 'take the pot', 'go to the far right corner', 'take the pot', 'drop the pot',
        'douse the torch', 'go to the door', 'go to the far left corner', 'go to the sconce', 'quit',
        'dq0 -p t -s 2 -c /ram/c.hm', 'go to the closest rat', 'douse the torch', 'go to the window', 'go to the rat', 'quit',
        'dq0 -p t -s 15 -c /ram/c.hm', 'bar the door', 'look at the door ahead to my left', 'go to the door ahead to my left', 'unbar the door', 'unbolt the door', 'bar the door', 'take the bar off', 'lift the bar', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm', 'go through the door to the east', 'open', 'look', 'look at the door', 'quit',
        'dq0 -p t -s 16 -c /ram/c.hm -T', 'go to the pit', 'go to the ledge', 'face east', 'force', 'e', 'force', 'force', 'e', 'e', 'go west', 'go to the trap', 'go west', 'go to the pit', 'quit',
        'dq0 -p t -s 517 -c /ram/c.hm', 'ne', 'e', 'go to the ledge', 'quit',
        'dq0 -p t -s 2 -c /ram/c.hm -T', 'read the door', 'read the sconce', 'douse the torch', 'read the sconce', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'step forward', 'examine the door ahead', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], ans = dq0Answers(out), T = dq0Turns(m), g = {};
        const said = (line, want, not) => {           // (Each line's answer, the nth of it in turn)
          const n = g[line] = (g[line] || 0) + 1, a = ans.filter(y => y.cmd === line)[n - 1], x = a ? a.text : '';
          for (const w of [].concat(want)) if (!x.includes(w)) f.push(line + ' (' + n + '): not "' + w + '": ' + x.slice(0, 200));
          for (const w of [].concat(not || [])) if (x.includes(w)) f.push(line + ' (' + n + '): "' + w + '" told: ' + x.slice(0, 200));
        };
        said('n', 'You come to the wall.');
        said('s', 'You come to a closed door.');
        said('s', 'The door is shut: open it first.');
        said('e', 'You come to a closed door.', 'You go through');    // (The door to its south, off to its side, not walked to)
        said('go through the door to the south', 'You come to a closed door.');
        said('n', 'You come to the wall.');
        said('s', ['You open the door.', 'You go through a doorway.']);
        said('go to the centre', 'You are in the middle of a kitchen');
        said('cross the room', 'You are');
        said('go across the room', ['You']);
        said('leave', 'Which exit?');
        said('go outside', 'Which exit?');            // (A plain phrasing, the rules': 2026-11-02)
        said('go out', 'Which exit?');
        said('go to the rat', 'You haven\'t seen a rat here.');
        said('approach the pot', 'You go over to the pot.');
        said('walk to the pot', 'You go over to the iron pot.', ['Pace:', 'You go over to it.']);
        said('head for the pot', 'Which one?');
        said('1', 'The iron pot is beside you.');
        said('walk over to the sconce', 'You go over to the sconce.');
        said('examine the sconce', ['You go over to the sconce.', 'The sconce is'], ['A barrel vault', 'by the light of', 'On the iron pot lies']);
        said('assess the sconce', ['You go over to the sconce.', 'You judge it'], ['A barrel vault', 'by the light of']);
        said('take the pot', ['You go over to the pot.', 'Taken.'], ['A barrel vault', 'by the light of', 'at your feet', 'At your feet', 'lies an iron pot']);
        said('take the pot', ['You go over to the pot.', 'Your hands are full, and there\'s nowhere to put it.'], ['A barrel vault', 'by the light of', 'at your feet', 'At your feet', 'lies an iron pot']);
        said('go to the door', 'You head for the door.');
        said('go to the sconce', 'You go over to the sconce.');
        said('go to the window', 'You haven\'t seen a window here.');
        said('go to the rat', 'You go over to the rat.', 'You haven\'t seen');
        said('bar the door', 'Which door? (1) the door ahead to your left (closed, barred)');    // (Its bar plain on this side, the look's words too: the user's)
        said('look at the door ahead to my left', 'it\'s barred');
        said('go to the door ahead to my left', 'There is a door ahead (closed, barred).');
        said('unbar the door', 'You lift the bar out of its brackets');
        said('unbolt the door', 'It isn\'t barred.');
        said('bar the door', 'the door is barred');
        said('take the bar off', 'You lift the bar out of its brackets');
        said('lift the bar', 'It isn\'t barred.');
        said('open', 'You open the door');
        said('open', 'It won\'t open: it\'s locked.');   // (A lock not seen till it's tried: then told)
        said('look', 'There is a door ahead (closed, locked).');
        said('look at the door', 'it\'s locked');
        said('go to the pit', 'You haven\'t seen a trap here.');
        said('go to the trap', 'It\'s beside you.');   // (The nearest it knows of: the door's blade, sprung)
        said('go to the pit', 'You go over to the trap.', ['You go over to it.', 'You can\'t go that way.']);
        said('go to the ledge', 'You haven\'t seen a ledge here.');
        said('go to the ledge', 'at a ledge ahead');
        said('read the door', 'There\'s nothing written on the door.');
        said('read the sconce', ['You go over to the sconce.', 'There\'s nothing written on the sconce.']);   // (Walked over to first, as any inspecting)
        said('read the sconce', 'You can\'t see to read.', 'You go over');   // (In the dark, nothing's read: where it lies or not)
        said('examine the door ahead', 'You head for the door.', 'You haven\'t seen');   // (The side said from where it was: gone to, it's that one; or a denizen in the way, the null referee's dice)
        if (!/A door of |can't get near enough/.test((ans.filter(y => y.cmd === 'examine the door ahead')[0] || { text: '' }).text)) f.push('examine the door ahead: not examined, gone to');
        const r = T.of('go to the closest rat');
        if (r.k < 0) f.push('no rat gone to');
        dq0Never(f, r, 'E_FIGHT', { who: 0, def: -1 }, 'a blow struck, going up to a rat');
        if (r.events.some(e => e.kind === 'E_FIGHT' && e.who === 0)) f.push('the character struck, only going up to a rat');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0steps', meaning: true, what: 'Dingus Quest Zero\'s small moves (the user\'s, 2026-10-16: "move left one space"; a square 5 feet, the game\'s), by where the character stands and faces (the hub\'s): at seed 12\'s first room (a small storeroom), facing northeast, a step forward (a square that way, its facing kept), a move to the right three spaces stopped by the wall (not moved), backing up 10 feet (the room\'s one square behind it, still facing northeast), a sidestep left into the wall; turned about, "run 2 squares east" (its pace set to a run, east one square to the wall, facing east) (a step stopped by a denizen in the way: dq0denizens; the look as a run ends, brief: dq0wordsthings)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 12 -c /ram/c.hm', 'step forward',
        'move right 3 spaces', 'back up 10 feet', 'sidestep left', 'turn around', 'run 2 squares east', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], p0 = (T.of('dq0 -p t -s 12 -c /ram/c.hm').after || {}).pc;
        if (!p0 || p0.face !== 1) return ['not begun facing northeast: ' + JSON.stringify(p0)];
        const at = (l, dx, dy, face, why) => {
          const t = T.of(l), p = t.after && t.after.pc;
          if (!p || p.x !== p0.x + dx || p.y !== p0.y + dy || p.face !== face)
            f.push(l + ': at ' + (p ? (p.x - p0.x) + ',' + (p.y - p0.y) + ' facing ' + p.face : '?') + ', not ' + dx + ',' + dy + ' facing ' + face);
          if (why) dq0Need(f, t, 'E_NOWAY', { why }, l + ': not refused');
          return t;
        };
        at('step forward', 1, -1, 1);
        at('move right 3 spaces', 1, -1, 1, 'N_NO_EXIT');
        at('back up 10 feet', 0, 0, 1);
        at('sidestep left', 0, 0, 1, 'N_NO_EXIT');
        at('turn around', 0, 0, 5);
        const r = at('run 2 squares east', 1, 0, 2);
        if (!r.after || r.after.pc.gait !== 4) f.push('the pace not set to a run (GA_RUN): ' + (r.after && r.after.pc.gait));
        return f;
      },
      sdcard: true
    },
    {
      scene: false,   // (Its watchers count the loads as the game begins: none taken up, sim/lib/scenes.js)
      name: 'dq0layers', meaning: true, what: 'Dingus Quest Zero\'s dungeon made from its history (Phase 3D, games/dq0/HISTORY.md; D12, the user\'s: "A temple could sit over the ruin of an older keep, say.  I just want the things on the same strata to make sense"): at seed 70018 (a region of its own, themegen\'s) the top stratum the tombs the hill-men built, fallen in, and the temple the sun-priests made of them after (its layers: built, ruined, reused): by the way in (those who came after came in that way) the chunks the maker made the temple\'s (a kitchen or a pantry among them, no main room a crypt), and by the way down (dq0 -T, the far side) the old tombs\' (each main room a crypt, no kitchen nor pantry); stratagen (bank/stratagen, a bank program: a stratum\'s layers chunk by chunk) loaded by themegen (its checker) and by the maker, once for all the maker\'s chunks, and with dq0 -C (the bank programs given up after each request) again and again; then at seed 70008 a keep the conquerors made of an older temple: in a guardroom of the keep\'s by the way in, the temple\'s chapel\'s candle stubs (the older layer\'s mark: a guardroom\'s own use leaves none)',
      init: 't_rc', cycles: 1300e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 70018 -c /ram/c.hm', 'quit',
        'dq0 -p t -s 70018 -c /ram/c.hm -T -C', 'quit', 'dq0 -p t -s 70008 -c /ram/c.hm', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) { dq0Made(this, m); },
      check(m) {
        const T = dq0Turns(m), f = [], g = this.games || [], opens = this.opens || [];
        if (!g[0] || !g[1] || !g[2]) return ['the three games\' quits not all seen'];
        const has = (c, u) => c.rooms.some(r => r.kind === u), at = c => c.cx + ',' + c.cy, uses = c => at(c) + ' ' + c.rooms.map(r => r.kind).join('/');
        const a = g[0].chunks, b = g[1].chunks;
        if (!a.length || !b.length) return ['no chunks of the top stratum made (by the way in: ' + a.length + ', by the way down: ' + b.length + ')'];
        if (!a.some(c => has(c, U_KITCHEN) || has(c, U_PANTRY))) f.push('by the way in no kitchen nor pantry, the temple\'s (' + a.map(uses).join('; ') + ')');
        for (const c of a) if (mainRoom(c).kind === U_CRYPT) f.push('by the way in, chunk ' + at(c) + '\'s main room a crypt, the old tombs\'');
        for (const c of b) {
          if (mainRoom(c).kind !== U_CRYPT) f.push('by the way down, chunk ' + at(c) + '\'s main room not a crypt (' + uses(c) + ')');
          if (has(c, U_KITCHEN) || has(c, U_PANTRY)) f.push('by the way down, chunk ' + at(c) + ' the temple\'s (' + uses(c) + ')');
        }
        // (stratagen's loads by game, by its caller: the task that read themegen's exports, before game k's hub was made,
        // or the maker's, after it)
        const loads = (k, exp) => {
          const h = exp === 'themegen.exp' ? k : k + 1, here = opens.filter(o => o.hubs === h);
          const ts = new Set(here.filter(o => o.path.endsWith('/' + exp)).map(o => o.T));
          return here.filter(o => ts.has(o.T) && o.path.endsWith('/stratagen.bp')).length;
        };
        for (const k of [0, 1, 2]) if (loads(k, 'themegen.exp') !== 1) f.push('game ' + (k + 1) + ': stratagen loaded by themegen ' + loads(k, 'themegen.exp') + ' times, not once');
        if (loads(0, 'dq0m.exp') !== 1) f.push('stratagen loaded by the maker ' + loads(0, 'dq0m.exp') + ' times for its ' + a.length + ' chunks, not once (kept)');
        if (loads(1, 'dq0m.exp') < 2) f.push('with dq0 -C, stratagen loaded by the maker ' + loads(1, 'dq0m.exp') + ' times, not again after it was given up');
        // (Seed 70008: the candle stubs in a guardroom of a chunk by the way in)
        const s = T.of('quit', T.of('dq0 -p t -s 70008 -c /ram/c.hm').k).at, st = g[2].strata[0];
        if (!s || !st) return f.concat(['seed 70008: no state at its quit']);
        const room = i => {
          const c = g[2].chunks.find(c => c.cx === (i.x - st.x0) >> 5 && c.cy === (i.y - st.y0) >> 5), x = (i.x - st.x0) & 31, y = (i.y - st.y0) & 31;
          return c && c.rooms.find(r => x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
        };
        const stubs = s.items.filter(i => i.name === 'candle stubs' && i.where === 'IW_SQUARE' && i.sn === 0);
        if (!g[2].chunks.some(c => has(c, U_GATEHOUSE))) f.push('seed 70008: no gatehouse by the way in, the keep\'s');
        if (!stubs.some(i => (room(i) || {}).kind === U_GUARDROOM)) f.push('seed 70008: no candle stubs in a guardroom (the temple\'s chapel\'s, under the keep\'s): ' +
          stubs.map(i => (room(i) || { kind: '-' }).kind).join(' '));
        this.notes = [a.length + ' chunks by the way in, ' + b.length + ' by the way down; the maker\'s stratagen loads with -C: ' + loads(1, 'dq0m.exp')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0plan', meaning: true, what: 'Dingus Quest Zero\'s built strata built to a plan (DUNGEON.md, the user\'s, 2026-10-16: "feast halls near kitchens near pantries and storage"; "Larger rooms tend to have higher ceilings, and corridors and smaller rooms tend to have uniform ceiling heights"; "Prior usage can also inform ... what kinds of detritus"): at seed 3 the crypt\'s Upper Halls (their purpose halls: a domestic suite by the way in), the character beginning in a kitchen, its iron pot lying there (what its use left), and its well (there\'s water); the chunks made, read from the hub\'s cache (world.h\'s struct chunk in the shared segment\'s banks): a hall joined to a kitchen by a passage of its own, through no other room; every hall\'s ceiling higher than the passages\' (the chunk\'s pceil), every storeroom\'s, pantry\'s, cell\'s and bedchamber\'s flat at the passages\' height; every room\'s floor flat, at the stratum\'s one level (but a hall\'s dais, two feet up)',
      init: 't_rc', cycles: 500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const f = [], s = dq0Turns(m).of('dq0 -p t -s 3 -c /ram/c.hm').after, seen = s && s.sight;
        if (!seen || seen.kind !== U_KITCHEN) f.push('not begun in a kitchen (the room\'s use: ' + (seen && seen.kind) + ')');
        if (!seen || !seen.items.some(i => i.name === 'iron pot')) f.push('no iron pot in the kitchen (what its use left)');
        if (!seen || !seen.items.some(i => i.name === 'well')) f.push('no well in the kitchen');
        const ks = builtChunks(m, 0);
        if (!ks.length) return ['no chunks of stratum 0 in the cache'];
        let joined = 0, halls = 0, level = null;
        for (const c of ks) {
          const at = c.cx + ',' + c.cy, inRoom = (x, y) => c.rooms.findIndex(r => x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h);
          for (const [r, p] of c.rooms.entries()) {
            const zs = c.zones.filter(z => z.room === r);
            if (p.kind === U_HALL) {
              halls++;
              for (const z of zs) if (z.cmid <= c.pceil) f.push(at + ': a hall\'s ceiling ' + z.cmid + ' feet, the passages\' ' + c.pceil);
            } else if ([U_STORE, U_PANTRY, U_CELL, U_QUARTERS].includes(p.kind)) {
              for (const z of zs) if (z.ceil !== 0 || z.cmid !== c.pceil) f.push(at + ': a small room\'s ceiling not flat at the passages\' height');
            }
            for (let y = p.y; y < p.y + p.h; y++) for (let x = p.x; x < p.x + p.w; x++) {
              const e = c.elev[y * 32 + x];
              if (level === null) level = e;
              if (e !== level && !(p.kind === U_HALL && e > level && e <= level + 2)) f.push(at + ': room ' + r + ' not flat at ' + x + ',' + y);
            }
          }
          // (A hall to a kitchen: from the hall's floor through doors and passages only, into the kitchen's)
          for (const [h, H] of c.rooms.entries()) {
            if (H.kind !== U_HALL) continue;
            const seen = new Set(), q = [];
            for (let y = H.y; y < H.y + H.h; y++) for (let x = H.x; x < H.x + H.w; x++) { seen.add(y * 32 + x); q.push(y * 32 + x); }
            while (q.length && !joined) {
              const i = q.pop(), x = i & 31, y = i >> 5;
              for (const [dx, dy] of [[0, -1], [1, 0], [0, 1], [-1, 0]]) {
                const nx = x + dx, ny = y + dy, j = ny * 32 + nx;
                if (nx < 0 || ny < 0 || nx > 31 || ny > 31 || seen.has(j)) continue;
                const t = c.tile[j], r = inRoom(nx, ny);
                if (r >= 0 && c.rooms[r].kind === U_KITCHEN && (t === T_FLOOR || t === T_STEPS)) { joined++; break; }
                if (r < 0 && (t === T_DOOR || t === T_CORR || t === T_STEPS)) { seen.add(j); q.push(j); }
              }
            }
          }
        }
        if (!halls) f.push('no hall in the chunks made');
        if (!joined) f.push('no hall joined to a kitchen by a passage of its own');
        this.notes = [ks.length + ' chunk(s) of the Upper Halls in the cache, ' + halls + ' hall(s), ' + joined + ' joined to a kitchen'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0gate', meaning: true, what: 'Dingus Quest Zero\'s fortified strata (DUNGEON.md: built to a plan; "Fortified versus not are built differently"): at seed 514 the kobold warren\'s Warren Mouth (its purpose a keep, fortified: theme.h\'s SP_FORT), the character beginning in its gatehouse (the room the way in comes out in), told so, a portcullis raised and an arrow slit in its walls; read from the hub\'s cache: the gatehouse\'s every way out a portcullis (none a door, none a bare doorway), murder holes in its vault (R_MURDER), its vault higher than the passages\'',
      init: 't_rc', cycles: 500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 514 -c /ram/c.hm', 'x portcullis', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], s = T.of('dq0 -p t -s 514 -c /ram/c.hm').after, seen = s && s.sight;
        if (!seen || seen.kind !== U_GATEHOUSE) f.push('not begun in the gatehouse (the room\'s use: ' + (seen && seen.kind) + ')');
        if (!seen || !seen.doors.some(d => d.kind === 'PK_PORTCULLIS' && (d.state & 1))) f.push('no raised portcullis seen');
        if (!seen || !seen.doors.some(d => d.kind === 'PK_SLIT')) f.push('no arrow slit seen');
        if (!dq0Rep(T.of('x portcullis'), 'wreq', { kind: 'WQ_EXAMINE', res: 0 }).length) f.push('the portcullis not examined');
        let gates = 0;
        for (const c of builtChunks(m, 0)) {
          for (const p of c.rooms) {
            if (p.kind !== U_GATEHOUSE) continue;
            gates++;
            if (!(p.flags & 0x04)) f.push('the gatehouse without its murder holes');
            for (const z of c.zones.filter(z => c.rooms[z.room] === p)) if (z.cmid <= c.pceil) f.push('the gatehouse\'s vault no higher than the passages\'');
            for (let x = p.x - 1; x <= p.x + p.w; x++) for (let y = p.y - 1; y <= p.y + p.h; y++) {
              if (x >= p.x && x < p.x + p.w && y >= p.y && y < p.y + p.h) continue;
              if (x < 0 || y < 0 || x > 31 || y > 31 || c.tile[y * 32 + x] !== T_DOOR) continue;
              const q = c.portals.find(q => q.x === x && q.y === y);
              if (!q || q.kind !== PK_PORTCULLIS) f.push('the gatehouse\'s way out at ' + x + ',' + y + ' ' + (q ? 'a door' : 'a bare doorway'));
            }
          }
        }
        if (!gates) f.push('no gatehouse in the chunks made');
        this.notes = [gates + ' gatehouse(s) in the cache\'s chunks'];
        return f;
      },
      sdcard: true
    },
    // dq0start0, dq0start1: twelve of the 38 seeds (of 1-200) whose start was a dead end before the links were kept off
    // their chunks' edges (each start in a room, by the stairs up to the way out, with a way on), a third of them, six
    // a test (the rest left to the soak's 200 seeds: each game's start a game start's cycles, some 195M)
    ...[[1, 30, 66, 93, 111, 122], [138, 152, 161, 171, 189, 197]].map((seeds, k) => ({
      name: 'dq0start' + k, meaning: true, what: 'Dingus Quest Zero\'s starts (dq0m/plan.c\'s links and the world\'s way out off their chunks\' edges, world.h\'s INSET: room for the room each comes out in, built.c\'s tries all failing else, a fifth of the starts a dead end of a passage), by what the character perceives (the senses\' record): seeds ' + seeds.join(', ') + ' (some of those that began at a dead end), each beginning in a room, by the stairs up to the way out, with a door, a portcullis or a passage on (seen ahead, or turned round, or as it crosses the room and back by its own light: the rooms dark since slice 4.1, nobody keeping their lights)',
      init: 't_rc', cycles: seeds.length * 360e6,      // (A timeout: each game started, turned round and quit; 360M
      // a game since 2026-10-29, the sound layer's pulses as it walks (320M since 2026-10-27, 300M before: the outfits'
      // and the denizens' gear's bigger world and referee to load))
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        ...seeds.flatMap(s => ['dq0 -p t -s ' + s + ' -c /ram/c.hm', 'turn around', 'go forward', 'turn around', 'go forward', 'quit']), 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        for (const s of seeds) {
          const g = T.of('dq0 -p t -s ' + s + ' -c /ram/c.hm'), a = g.after, b = T.after(g, 'turn around').after;
          if (!a || !a.sight || !b || !b.sight) { f.push('seed ' + s + ': not begun'); continue; }
          // (A way on: a door, a doorway, a passage seen from the stairs or as it crosses the room and back, by its own
          // light, the rooms dark since slice 4.1, nobody keeping their lights; or the room left by one)
          const went = [g.k + 1, g.k + 2, g.k + 3, g.k + 4].map(k => T[k] && T[k].after).filter(x => x && x.sight);
          if (a.sight.where !== 0) f.push('seed ' + s + ': not in a room (a dead end of a passage?)');
          else if (!a.sight.open.some(o => o.kind === 'O_WAY_OUT' && o.rel === 255)) f.push('seed ' + s + ': not by the way out');
          else if (![a, ...went].some(x => x.sight.doors.length || x.sight.nways || x.sight.where !== 0)) f.push('seed ' + s + ': no way on seen');
        }
        return f;
      },
      sdcard: true
    })),
  ],
};
