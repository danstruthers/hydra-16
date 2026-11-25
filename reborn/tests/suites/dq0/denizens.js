// ****************************************************************************
// denizens.js - Dingus Quest Zero's tests: the denizens, their minds and their places.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { card, typed } = require('../../lib/common.js');
const {
  T_ROCK, T_FLOOR, T_WALL, T_CORR, T_WINDOW, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N, CSLOT_AT, liveChunks, stratumRec,
  dq0Story, dq0Prose, inTurn, dq0Answers, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need, dq0Never, dq0Carried, dq0Named, DQ0_I,
  dq0Typed, dq0Portal, dq0Rec, dq0Scene, dq0SceneNow, dq0PcShock, dq0Aware, dq0PortalAt, dq0OnWords, dq0Timer, dq0Alone, dq0Paces, dq0Own, dq0WhenFree,
  dq0Asks, dq0HeldPaces, dq0PlayLetters, dq0FightPause,
  dq0Ad, dq0Root, dq0Holder, dq0Intent, dq0Game
} = require('../../lib/dq0.js');
const { target, D17 } = require('../../lib/trends.js');   // (The user's targets, D17, at 115,200 baud: each measure kept, those over and past 90% listed after the tests)
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')
// The world's act table's rows by name (games/dq0w/acts.txt, acts.js's read: the one-path plan's stage 1, 2026-11-19): each
// act's time, test, noise (loud; nz the character's E_NOISE's what, de an actor's E_DEN's), the same for every actor
const dq0Acts = () => Object.fromEntries(require(path.join(TESTS, '..', 'games', 'dq0w', 'acts.js')).read(require(path.join(TESTS, '..', 'games', 'dq0', 'hubgen.js')).load()).map(r => [r.name, r]));
// (Climbing and links, lights: the one-path plan's stage 2, 2026-11-23) A floor square (a room's or a passage's) at d
// squares from the character on its stratum, no one on it: the way h (a heading) if it's given, else the first round
// it from the north; [x, y] or null.  A stratum's datum, its floor's height (world.h's struct stratum's, at 5: the
// world bank's record).  A flight's crossing, tenths (acts.h's FLIGHT_T: link kind k, step st, the drop in feet)
function dq0FloorBy(m, h, L, d, way) {
  const u16 = o => h[o] | h[o + 1] << 8, px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], S = stratumRec(m, sn), C = S && liveChunks(m, sn);
  const tile = (x, y) => { const b = C && C[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)]; return b ? b[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1; };
  const taken = (x, y) => [...Array(L.nactors).keys()].some(k => { const o = L.actor + k * L.actorSize; return h[o + L.aState] && h[o + L.aSn] === sn && u16(o + L.aX) === x && u16(o + L.aY) === y; });
  const dx = [0, 1, 1, 1, 0, -1, -1, -1], dy = [-1, -1, 0, 1, 1, 1, 0, -1];
  for (const w of way === undefined ? [0, 1, 2, 3, 4, 5, 6, 7] : [way]) {
    const x = px + d * dx[w], y = py + d * dy[w];
    if ([T_FLOOR, T_CORR].includes(tile(x, y)) && !taken(x, y)) return [x, y];
  }
  return null;
}
function dq0Datum(m, sn) {
  const banks = m.sharedBanks(), h0 = m.dq0.bank(0), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k] === h0 || (banks[k].buffer === h0.buffer && banks[k].byteOffset === h0.byteOffset)), w = hub === undefined ? null : banks[hub + WORLD_BANK_N];
  for (let i = 0; w && i < 8; i++) if (w[i * 79] && w[i * 79 + 1] === sn) return (w[i * 79 + 5] | w[i * 79 + 6] << 8) << 16 >> 16;
  return null;
}
const dq0Flight = (k, st, feet, up) => k >= 8 ? Math.floor((feet + 1) * st * (up ? 13 : 8) / 10) :
  Math.floor((feet + 1) * st * (k === 2 || k === 3 ? 2 : k === 5 || k === 6 ? 3 : 1) / (up ? 1 : 2));

// A task's own main RAM in a scene's call (the minds', dq0d's: struct mind md and its statics, by its label, dq0Ad's; the
// world's grip), task its role (2 the minds, 1 the world): the scene's copy of it, its bytes changed the task's own ops
// (tests/lib/dq0.js's sceneBuild: SO_DRAM, SO_WRAM), made by the task itself as the scene's put in place (scene1, 2026-11-24)
function dq0LaterRam(m, task = 2) {
  return m.taskRam[m.dq0.bank(0)[m.dq0.layout().L.task + task]];
}

// Dingus Quest Zero's morale's own (dq0morale): at each line's Enter (the CR as the ACIA has it, the game still as it
// stands), the pokes made (each { g: the game, 1 on, by the dq0 lines typed, line, n: its nth time typed in it, 1 on, k:
// the actor's entry, or name: its name, the first so named, set: { field: value }; hub: { the layout's name: a byte },
// the hub's own, pcSkew say) and the actors' hearts kept after them (m.hearts: { g, line, n, acts: [{ k,
// name, heart, doing }] }): the hub's actor's heart,
// lead, power, mood, serve, contact and bonds are its struct's last seven bytes (hub.h's, 2026-10-30), attitude,
// faction, doing, pos, face, state and sn (bytes) and x, y (words) the layout's; and its record's (rec: { or: flags
// or'd in, serve, contact }: dq0Rec's).  A denizen put somewhere (2026-10-31): beside: a heading (H_N 0 ... H_NW 7,
// or 'face', the way the character faces), the actor on the square beside the character that way, on its stratum, on
// its feet; hurt: levels, a minor injury on its sheet; engage: true, hostile and fighting it (AT_HOSTILE, DO_FIGHT); either written to its record too (its square,
// its stratum, its attitude: dq0d's, so the world and the minds agree) and the world's den_seq bumped (what the
// character sees of them looked at again).  The test's own setting up, as dq0Aware's
// The denizens' records (dq0d's pb bank, its label's: the task the hub's task[T_DENIZENS]): { mem, at(i) the record
// i's offset, and its fields' }: dq0d.c's struct pop (its head 330 bytes since slice 4.8's regard, 64 lairs of 17), struct den (30 bytes since its want, 2026-10-31: its
// flags at 1, its stratum at 2, its id at 4, its square at 8 and 10, its attitude at 16, its serve and contact at 26 and 27)
// rock.h's rock_vein (the miners', slice 4.11's rest): the vein across the region of square x, y where it crosses row
// y, from the game's seed (its low 16 bits): { kind (RK_QUARTZ 8 ...), vx its middle (the frame's), vw the squares its
// middle moves a row down }, or null (none, or not in the region on that row)
function dq0Vein(seed, x, y) {
  let h = 0;
  const next = () => { h = (h ^ (h << 4)) & 0xFFFF; h ^= h >> 3; h = (h ^ (h << 8)) & 0xFFFF; return (h ^ (h >> 8)) & 255; };
  h = ((seed ^ ((x >> 8) * 31) ^ ((y >> 8) * 977) ^ 7919) & 0xFFFF) | 1;
  next(); next();
  const k = next() & 15;
  if (k >= 5) return null;
  const vw = (next() & 7) - 4, x0 = next(), c = x0 + vw * ((y & 255) - next());
  if (c < 0 || c > 255) return null;
  return { kind: k < 3 ? 8 : k < 4 ? 9 : 10, vx: (x & 0xFF00) | c, vw };
}

// The day's hour poked (a test's own setting up: the denizens' hour0, dq0d.c's part 9, the hour of the day the game's
// clock began at; 0x80, no day, every one about as before the day, 2026-10-31): at a line's Enter, each { g: the game,
// 1 on, by the dq0 lines typed, line, n: its nth time typed in it, 1 on, hour }.  A game's whole (from its start):
// the environment's dq0hour (a line dq0hour=128 before it, rc's), dq0d's first look at its day
function dq0Hour(m, list) {
  const a = m.acia, read = a.read;
  let typed = '', g = 0, seen = {};
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (/^dq0 /.test(typed)) { ++g; seen = {}; }
        seen[typed] = (seen[typed] || 0) + 1;
        const h = m.dq0 && m.dq0.bank(0), p = list.find(q => q.g === g && q.line === typed && q.n === seen[typed]);
        if (h && p) dq0SceneNow(m, () => {      // (The minds' own: dq0LaterRam's)
          const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
          dq0LaterRam(m)[parseInt(/^al ([0-9A-F]+) \._hour0$/m.exec(lbl)[1], 16)] = p.hour;
        }, null, 'dq0Hour\'s "' + typed + '"', true);
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}

// A line typed as the game's words come, not at a cycle (a test's own: what a player reacting to them types, whatever
// the code's speed): each { g: the game, 1 on, by the dq0 lines typed, line, n: its nth time typed in it, 1 on, words,
// text, or call: a function run then }: after that line's Enter, the first time the story shows the words (the escapes
// left out, spaces and line ends as one), the text typed next, before the input still to come (its waits for a prompt
// from then on).  One its words never came for let go as its game ends (the screen's scrolling whole again, the game's
// t_away: ESC [ r) or another begins: never typed into the next (2026-10-31)
// The scene-setter shared by the door tests (Phase 3C's): on the typed line `on` (its first), the actor named `who`
// put on the square beyond the door the character faces (the door beside it), hostile, drawn on to the character's
// square (the minds' drawn and last-sensed square), its record moved with it; m.door = { k, door, beyond, pc, tile,
// state } after.  `how`: extra pokes (h, o, L, K) for the scene
function dq0DoorScene(m, on, who, how) {
  const a = m.acia, read = a.read;
  let typed = '', done = false;
  m.door = null;
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (typed === on && !done && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K) => {   // (A scene: the hub's copy, its records', the minds' own later)
          const u16 = o => h[o] | h[o + 1] << 8;
          const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], S = stratumRec(m, sn);
          const chunk = (x, y) => S && liveChunks(m, sn)[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)];
          const tile = (x, y) => { const b = chunk(x, y); return b ? b[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1; };
          const state = (x, y) => {   // (The door's state, its portal record's: struct chunk's portal[], 14 bytes each)
            const b = chunk(x, y);
            if (!b) return -1;
            for (let i = 0; i < b[218]; i++) if (b[219 + i * 14] === ((x - S.x0) & 31) && b[220 + i * 14] === ((y - S.y0) & 31)) return b[222 + i * 14];
            return -1;
          };
          const at = k => L.actor + k * L.actorSize, named = k => {
            let n = '';
            for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
            return n;
          };
          const f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === who);
          m.doorWhy = { names: [...Array(L.nactors).keys()].map(named), face: f, ahead: tile(px + dx, py + dy), beyond: tile(px + 2 * dx, py + 2 * dy) };
          if (k !== undefined && tile(px + dx, py + dy) === 3 && [1, 4].includes(tile(px + 2 * dx, py + 2 * dy))) {
            const x = px + 2 * dx, y = py + 2 * dy, o = at(k);
            w16(o + L.aX, x);
            w16(o + L.aY, y);
            h[o + L.aSn] = sn;
            h[o + L.aAttitude] = K.AT_HOSTILE;
            h[o + L.aDoing] = 0;
            const R = dq0Rec(m), q = R.at(h[o + L.aDen]);
            R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
            R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
            R.mem[q + R.SN] = sn;
            R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
            const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
            const ad = dq0Ad(lbl), ram = dq0LaterRam(m);
            ram[ad('drawn') + k] = 8;
            ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
            ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
            if (how) how(h, o, L, K);
            h[L.denSeq] = h[L.denSeq] + 1 & 255;
            m.door = { k, door: [px + dx, py + dy], beyond: [x, y], pc: [px, py], tile, state };
          }
          done = true;
        }, null, 'dq0DoorScene\'s "' + on + '"', true);
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}

// The sounds the sound task plays (a machine's ymLog: each YM2151 key-on on its channels, 4-6, the source and the sound
// its channel's playing then, dq0s.c's struct ch, 44 bytes, who then snd: its cs[] by its label), m.played: [{ who, snd }]
// (who an actor's entry plus 1, 0 the character; snd 2 * SND_KINDS + WS_*, the world's: 64 dig, 65 open, 66 shut, 67 blow,
// 68 smash, 69 chains; 73 click, 74 bar, quiet: a key's, a pick's, a bar's, 2026-11-17)
function dq0Played(m) {
  const w = m.ym.writes, push = w.push.bind(w), lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0s', 'dq0s.lbl'), 'latin1');
  const cs = parseInt(/^al ([0-9A-F]+) \._cs$/m.exec(lbl)[1], 16);
  m.played = [];
  w.push = e => {
    const [, reg, v] = e, k = [4, 5, 6].indexOf(v & 7), h = reg === 0x08 && (v & 0x78) && k >= 0 && m.dq0 && m.dq0.bank(0), ram = h && m.taskRam[h[12 + 4]];
    if (ram) m.played.push({ who: ram[cs + 44 * k], snd: ram[cs + 44 * k + 1] });
    return push(e);
  };
}
const WSND = { dig: 64, open: 65, shut: 66, blow: 67, smash: 68, chains: 69, click: 73, bar: 74 };

// The one-path plan's stage 2 parity scenes (closures, 2026-11-21: ONE-PATH.md's 2.7): dq0allydoor's (seed 1's
// bandits, the door ahead opened and shut, a bandit put beyond it as 'look' is typed, dq0DoorScene's, and made the
// character's ally), how its own pokes then (dq0DoorScene's how); asks: the lines it's kept allied at (dq0Poke's)
function dq0ParScene(m, asks, how) {
  dq0Scene(m, [{ g: 1, line: 'look', n: 1, name: 'bandit', set: { attitude: 5, doing: 0 } }, ...asks.map(line => ({ g: 1, line, n: 1, name: 'bandit', set: { attitude: 5 } }))]);
  dq0DoorScene(m, 'look', 'bandit', how);
}

// Every denizen's lockpicks made a rope (a scene's: none to pick a lock with, the one-path plan's tool rule, 2026-11-21): how many
function dq0NoPicks(m) {
  const { L, K } = m.dq0.layout(), per = Math.floor(0x2000 / L.itemSize);
  let n = 0;
  for (let i = 1; i < L.nitems; i++) {
    const b = m.dq0.bank(L.item0 + Math.floor(i / per)), q = (i % per) * L.itemSize;
    if (b && b[q + L.iKind] === K.KI_LOCKPICKS && b[q + L.iWhere] !== K.IW_ACTOR && b[q + L.iWhere] !== K.IW_SQUARE) { b[q + L.iKind] = K.KI_ROPE; n++; }
  }
  return n;
}

// A scene's own at a line's Enter, each time it's typed: fn (h, L, K), the hub's bank 0's copy and the layout's (a scene
// built of its writes, dq0SceneNow's)
function dq0OnLine(m, line, fn) {
  const a = m.acia, read = a.read;
  let typed = '';
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (typed === line && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K) => fn(h, L, K), null, 'dq0OnLine\'s "' + line + '"', true);   // (A scene: fn's writes the copies')
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}

const LETDENS = ['look', 'around', 'look at B', 'talk to C', 'attack C', 'attack A', 'around'];   // (dq0letdens' lines)

module.exports = {
  tests: [
    {
      name: 'dq0tunnel', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's tunnellers (slice 4.11, 2026-11-01: DENIZENS.md's tunnellers and builders, HOUSE.md's tunnellers, for review): a hand-written theme's dwarf miners (seed 1, south of the start, wary; a miner digs with the tools it carries, kinds.txt's ways m: its pick, outfits.txt's [role khuzmin]), the character in their guardroom, the portcullis it came in by let down behind it ('close the door'); one of them put in the passage beyond it, hostile, drawn on to where it last sensed the character (the scene set as the first wait's typed: the hub's actor and its record, the minds' drawn and last-sensed square poked).  A portcullis it can't raise, no way nearer: it digs its way round it (dq0d's dig_way, a diagonal round the door: nearer as the crow flies; I_KIT's KT_DIG, the world's wdig.c a spell of ten minutes at a time by the character's rules: a pick's rate on the wall, a Strength test by its sheet), its first spell seen through the portcullis (E_DEN's DE_DIG: \"The dwarf miner digs into the wall.\", or heard, a scraping in rock), the hole kept as it goes (DUG), and through (DE_DUG: the square a passage, told seen, \"A fresh tunnel opens ...\", or heard, rock falling); the dwarf come through it after; both referees",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7 -S /ram/s.sc',
        'open the door', 'go through it', 'turn around', 'close the door ahead', 'wait', 'wait 2 hours', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'wait 2 hours', n: 1, words: 'A fresh tunnel opens', text: 'stop\r' }]);   // (Stopped as it's told: 2026-11-03)
        const others = (h, L) => { for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName] && (!m.tun || k !== m.tun.k)) h[L.actor + k * L.actorSize + L.aAttitude] = 3; };
        dq0Scene(m, ['wait 2 hours', 'stop'].map(line => ({ g: 1, line, n: 1, call: others })));   // (The other miners let be: this one's the scene)
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.tun = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K) => {   // (The scene: one dwarf miner beyond the shut door ahead, hostile, drawn on)
                const u16 = o => h[o] | h[o + 1] << 8;
                const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
                const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], S = stratumRec(m, sn), ch = liveChunks(m, sn);
                const tile = (x, y) => {
                  const b = S && liveChunks(m, sn)[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)];
                  return b ? b[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1;
                };
                const at = k => L.actor + k * L.actorSize, named = k => {
                  let n = '';
                  for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
                  return n;
                };
                const f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
                const k = [...Array(L.nactors).keys()].find(j => named(j) === 'dwarf miner');
                if (k !== undefined && ch && tile(px + dx, py + dy) === 3 && [1, 4].includes(tile(px + 2 * dx, py + 2 * dy))) {
                  const x = px + 2 * dx, y = py + 2 * dy, o = at(k);
                  w16(o + L.aX, x);
                  w16(o + L.aY, y);
                  h[o + L.aSn] = sn;
                  h[o + L.aAttitude] = K.AT_HOSTILE;
                  h[o + L.aDoing] = 0;
                  const R = dq0Rec(m), q = R.at(h[o + L.aDen]);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  R.mem[q + R.SN] = sn;
                  R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  const ad = dq0Ad(lbl), ram = dq0LaterRam(m);
                  ram[ad('drawn') + k] = 8;
                  if (!m.portc) ram[ad('atry') + k] = ((f >> 1) + 2 & 3) | 3 << 2;   // (Its tries at the portcullis given up already (the minds'
                  // atry: its way, stage 3), 2026-11-02: so it digs round; dq0portc's the portcullis worked)
                  ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
                  ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.tun = { k, door: [px + dx, py + dy], beyond: [x, y], pc: [px, py], tile };
                }
                done = true;
              }, null, 'the dwarf beyond the door', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), DE_DIG = 24, DE_DUG = 25;
        if (!m.tun) return ['the scene not set: no dwarf miner near, or no portcullis ahead with a floor beyond it'];
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.tun.k + 1);
        const digs = dens.filter(e => e.a === DE_DIG), dug = dens.filter(e => e.a === DE_DUG);
        if (!digs.length) f.push('no spell of digging begun (E_DEN DE_DIG)');
        if (!dug.length) f.push('never through (E_DEN DE_DUG)');
        for (const e of dug) {
          const t = m.tun.tile(e.w1, e.w2);
          if (t !== 4) f.push('a square dug through not a passage now: ' + [e.w1, e.w2] + ', tile ' + t);
          if (Math.max(Math.abs(e.w1 - m.tun.pc[0]), Math.abs(e.w2 - m.tun.pc[1])) > 3) f.push('dug far from the way round the door: ' + [e.w1, e.w2]);
        }
        if (m.tun.tile(m.tun.door[0], m.tun.door[1]) !== 3) f.push('the door itself dug');
        if (!/scraping in rock|The dwarf miner sets to digging/.test(s)) f.push('its digging neither seen nor heard');
        if (!/A fresh tunnel opens|rock falling/.test(s)) f.push('the way through not told');
        const fin = T.of('quit').at, me = fin && fin.actors.find(a => a.k === m.tun.k);
        if (!me || (me.x === m.tun.beyond[0] && me.y === m.tun.beyond[1])) f.push('the dwarf never came on from beyond the door: ' + JSON.stringify(me));
        if (f.length) f.push(JSON.stringify(dens.filter(e => e.a).map(e => [e.a, e.w1, e.w2, e.t])) + ' ' + s.slice(s.indexOf('> close the door'), s.indexOf('> close the door') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0mine', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's miners at their work (slice 4.11's rest, 2026-11-02: DENIZENS.md's tunnellers and builders, HOUSE.md's tunnellers, for review): a hand-written theme's dwarf miners (seed 374: their home south-west of the start, a copper seam crossing their chunk eight squares east of it, rock.h's; wary; a miner digs with the pick it carries, kinds.txt's ways m, outfits.txt's [role khuzmin]), the hour 8 in the morning (dq0hour: a diurnal miner's working hours, part 9's day), the character waiting in its cell six hours, out of their sight; the portcullis on their mine's way across raised (the scene set as the first long wait's typed: its state in the chunks' cache poked; a lowered one ends a mine's way, the minds raising none yet).  At its work (by the clock, part 9's SC_WORK: never polled) a miner digs its faction's mine (dq0d's work_dig, part 15's): from its lair's home across to its region's vein (rock.h's rock_vein: a seam of ore or quartz), then along it, the open squares and doorways on it walked through, its stand the square before its first wall, rock or rubble; a spell at a time, the world's (wdig.c, I_KIT's KT_DIG, by the character's rules: its pick's rate, its Strength test by its sheet), heard (E_DEN DE_DIG, 'a scraping in rock'), each square through a passage (DE_DUG) on the mine's line (the home's row between the home and the vein, or the vein's own squares at their row), none of them hostile",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 8 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 374 -c /ram/c.hm -T -d /ram/g -R 1 -S /ram/s.sc',
        'wait 2 hours', 'wait 2 hours', 'wait 2 hours', 'quit', 'echo $status'] })) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '';
        m.mine = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 2 hours' && !m.opened && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L) => {   // (The scene: the portcullis on the mine's way across raised)
                const u16 = o => h[o] | h[o + 1] << 8;
                const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ram = m.taskRam[h[12 + 2]];
                const ad = dq0Ad(lbl), sn = h[L.pcSn];
                const hx = ram[ad('ahx')] | ram[ad('ahx') + 1] << 8, hy = ram[ad('ahy')] | ram[ad('ahy') + 1] << 8, v = dq0Vein(u16(L.seed), hx, hy);
                m.opened = [];
                for (let x = hx; v && x !== v.vx; x += Math.sign(v.vx - hx)) {
                  const p = dq0Portal(m, sn, x, hy);
                  if (p) {
                    m.opened.push([x, p.kind]);
                    p.b[p.state] = (p.b[p.state] & ~0x06) | 1;   // (Open (a portcullis raised), unlocked, unbarred)
                  }
                }
              }, null, 'the portcullis raised', true);
              if (typed === 'quit' && !m.mine && m.dq0) {   // (The miners' homes, the actors': the minds' ahx, ahy)
                const { L } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
                const at = k => L.actor + k * L.actorSize, named = k => {
                  let n = '';
                  for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
                  return n;
                };
                const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                const ad = dq0Ad(lbl), ram = m.taskRam[h[12 + 2]];
                const miners = [...Array(L.nactors).keys()].filter(k => named(k) === 'dwarf miner').map(k => ({ k,
                  hx: ram[ad('ahx') + 2 * k] | ram[ad('ahx') + 2 * k + 1] << 8, hy: ram[ad('ahy') + 2 * k] | ram[ad('ahy') + 2 * k + 1] << 8,
                  attitude: h[at(k) + L.aAttitude], act: ram[ad('aact') + k], tx: ram[ad('atx') + 2 * k] | ram[ad('atx') + 2 * k + 1] << 8,
                  ty: ram[ad('aty') + 2 * k] | ram[ad('aty') + 2 * k + 1] << 8, sres: ram[ad('sres') + k], agear: h[L.agear + k] }));
                const S = stratumRec(m, h[L.pcSn]);
                const sn = h[L.pcSn], ch = liveChunks(m, sn), tile = (x, y) => { const c = S && ch[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)]; return c ? c[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1; };
                const row = miners.length ? [...Array(16).keys()].map(i => tile(miners[0].hx - 2 + i, miners[0].hy)) : [];
                m.mine = { seed: u16(L.seed), miners, x0: S && S.x0, y0: S && S.y0, pc: [u16(L.pcX), u16(L.pcY)], opened: m.opened, row,
                  at: [...Array(L.nactors).keys()].map(k => [u16(at(k) + L.aX), u16(at(k) + L.aY), h[at(k) + L.aDoing]]) };
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), DE_DIG = 24, DE_DUG = 25, DE_DIGS = 26;
        if (!m.mine || !m.mine.miners.length) return ['no dwarf miner an actor at the end: ' + JSON.stringify(m.mine)];
        const ks = new Set(m.mine.miners.map(q => q.k + 1));
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && ks.has(e.who));
        const digs = dens.filter(e => e.a === DE_DIG || e.a === DE_DIGS), dug = dens.filter(e => e.a === DE_DUG);
        if (!digs.length) f.push('no miner dug at its work (E_DEN DE_DIG)');
        if (!dug.length) f.push('no square of the mine dug through (E_DEN DE_DUG)');
        const q = m.mine.miners[0], v = dq0Vein(m.mine.seed, q.hx, q.hy);
        if (!v) f.push('no vein near the miners: ' + JSON.stringify(q));
        for (const e of dug) {
          const w = v && dq0Vein(m.mine.seed, e.w1, e.w2), across = e.w2 === q.hy && (e.w1 - q.hx) * (v ? v.vx - e.w1 : 0) >= 0;
          if (!across && !(w && Math.abs(e.w1 - w.vx) <= Math.abs(w.vw))) f.push('a square dug off the mine: ' + [e.w1, e.w2]);
        }
        if (m.mine.miners.some(x => x.attitude === 0)) f.push('a miner hostile');
        if (!/scraping in rock|sets to digging/.test(s)) f.push('the digging neither seen nor heard');
        if (f.length) f.push(JSON.stringify({ mine: m.mine, v, dens: dens.filter(e => e.a).map(e => [e.who, e.a, e.w1, e.w2]) }));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0burrow', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's burrowers at their work (slice 4.11's rest, 2026-11-02; the user's, 2026-11-03, \"Soft rock only\": claws dig no rock, shale neither, the one rule at work and at prey: DENIZENS.md's tunnellers and builders, HOUSE.md's a burrower's claws): a hand-written theme's giant rats on the stratum below, wary (a burrower digs with its claws, kinds.txt's ways d), the game begun at dusk (their waking hours); the character goes down the stairway, comes back up and waits six hours, the rats away from it (records, coarse); the first square of rock or wall on each lair's burrow line (its home, one of the four ways by its lair's number, eight squares) made rubble as it begins (the scene): a rat at its work (part 9's SC_WORK, by the clock) digs that rubble through and nothing else: rock it never touches, the change records' squares none but those",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g',
        "echo 'faction | Vermin | rat | 1-1 | 30 | - | 1 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s ' + 3 + ' -c /ram/c.hm -T -d /ram/g',
        'd', 'u', 'wait 3 hours', 'wait 3 hours', 'quit', 'echo $status'] })) },
      start(m) {
        m.burrow = { homes: [], rubble: [] };
        const HX = [0, 1, 0, -1], HY = [-1, 0, 1, 0];
        dq0Scene(m, [{ g: 1, line: 'wait 3 hours', n: 1, call: (h, L, K) => {   // (Their lairs placed by now: their records' numbers and homes)
          const R = dq0Rec(m), u = o => R.mem[o] | R.mem[o + 1] << 8, homes = new Map(), w = m.dq0.bank(WORLD_BANK_N);
          for (let i = 0; i < 224; i++) {
            const q = R.at(i);
            if (R.mem[q + R.FLAGS] & 1) homes.set(u(q + 6), { lid: u(q + 6), sn: R.mem[q + 18], hx: u(q + 21), hy: u(q + 23) });
          }
          m.burrow.homes = [...homes.values()];
          for (const o of m.burrow.homes) {
            const st = stratumRec(m, o.sn), d = o.lid & 3;
            for (let n = 1; st && n <= 8; n++) {
              const x = o.hx + n * HX[d], y = o.hy + n * HY[d], cx = (x - st.x0) >> 5, cy = (y - st.y0) >> 5;
              let c = 0;
              for (; c < 24; c++) { const sl = CSLOT_AT + c * 8; if (w[sl] && w[sl + 1] === o.sn && (w[sl + 2] | w[sl + 3] << 8) === cx && (w[sl + 4] | w[sl + 5] << 8) === cy) break; }
              if (c === 24) break;
              const b = m.dq0.bank(5 + Math.floor(c / 3)), at = (c % 3) * 0xAA0 + 0x1BB + ((y - st.y0) & 31) * 32 + ((x - st.x0) & 31), t = b[at] & 15;
              if (t === 0 || t === 2) { b[at] = (b[at] & 0xF0) | 10; m.burrow.rubble.push(x + ',' + y); break; }   // (T_ROCK, T_WALL: T_RUBBLE)
              if (t !== 1 && t !== 4 && t !== 5) break;
            }
          }
        } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], fin = T.of('quit').at;
        this.notes = [JSON.stringify(m.burrow)];
        if (!m.burrow.rubble.length) return ['no burrow line to make rubble on: ' + JSON.stringify(m.burrow)];
        const dug = (fin ? fin.changes : []).filter(c => c.what === 'DUG' || (c.what === 'SQUARE' && c.tile === 4));
        if (!dug.length) f.push('the rubble not dug');
        for (const c of dug) if (!m.burrow.rubble.includes(c.x + ',' + c.y)) f.push('a square dug that was no rubble: ' + [c.sn, c.x, c.y, c.what, c.tile]);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0builder', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's builders (slice 4.11's rest, 2026-11-02: DENIZENS.md's tunnellers and builders, HOUSE.md's builders, for review): a hand-written theme's dwarf miners (seed 374, their home south-west of the start, wary), the hour 8 in the morning (dq0hour: their working hours).  The closed door beside the character, in the miners' home's chunk, broken down, and the portcullis on their mine's way across raised (the scene set as the first wait's typed: their portals' states in the chunks' cache poked, PS_WRECKED, PS_OPEN); the character goes down the stairway and waits four hours (a lair that found no dig to do asks again as the clock's high word moves, part 9's nowork: near enough two hours), the miners away from it (records, moved coarsely by the clock): at their work a miner, its faction's builder, puts the door up again (dq0d's work_dig, part 15's: the door its work before its mine; its spell coarse, I_KIT's KT_BUILD who 0: the world's wdig.c), by the door's own change records (PORTAL_STATE: shut and whole; PORTAL_HARM: none); back up, the character finds the door shut ('one to your left (closed)'); and the others meanwhile dig their mine away from the character, coarse (I_KIT's KT_DIG who 0: a spell at once, untested), its holes and its ways through (DUG, a square's change records) on its line (the home's row across to the copper seam, or the seam's own squares)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 8 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 374 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'wait', 'look', 'd', 'wait 2 hours', 'wait 2 hours', 'u', 'look', 'quit', 'echo $status'] })) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.door = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L) => {   // (The scene: the closed door beside the character broken down, in the cache)
                const u16 = o => h[o] | h[o + 1] << 8;
                const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn];
                for (let d = 0; d < 8 && !m.door; d += 2) {
                  const x = px + [0, 1, 1, 1, 0, -1, -1, -1][d], y = py + [-1, -1, 0, 1, 1, 1, 0, -1][d], p = dq0Portal(m, sn, x, y);
                  if (p && p.kind === 0 && !(p.b[p.state] & 1)) {
                    p.b[p.state] = 0x81;   // (PS_WRECKED | PS_OPEN: broken down)
                    m.door = { x, y, sn, seed: u16(L.seed) };
                  }
                }
                const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ram = m.taskRam[h[12 + 2]];
                const ad = dq0Ad(lbl);
                if (m.door) m.door.home = [ram[ad('ahx')] | ram[ad('ahx') + 1] << 8, ram[ad('ahy')] | ram[ad('ahy') + 1] << 8];
                const v = m.door && dq0Vein(m.door.seed, m.door.home[0], m.door.home[1]);   // (The portcullis on the mine's way across raised, as dq0mine's)
                for (let x = m.door ? m.door.home[0] : 0; v && x !== v.vx; x += Math.sign(v.vx - m.door.home[0])) {
                  const p = dq0Portal(m, sn, x, m.door.home[1]);
                  if (p && p.kind) p.b[p.state] = (p.b[p.state] & ~0x06) | 1;
                }
                done = true;
              }, null, 'the door broken down, the portcullis raised', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        if (!m.door) return ['the scene not set: no closed door beside the character'];
        const fin = T.of('quit').at, ch = (fin ? fin.changes : []).filter(c => c.sn === m.door.sn && c.x === m.door.x && c.y === m.door.y);
        const st = ch.filter(c => c.what === 'PORTAL_STATE');
        if (!st.length || st[st.length - 1].tile !== 0) f.push('the door not put up again (its PORTAL_STATE record): ' + JSON.stringify(ch));
        if (T.of('d').after && T.of('d').after.pc.sn === m.door.sn) f.push('the character never went down');
        const [hx, hy] = m.door.home, v = dq0Vein(m.door.seed, hx, hy), all = fin ? fin.changes : [];   // (The mine, dug coarse meanwhile)
        const mine = all.filter(c => c.sn === m.door.sn && (c.what === 'DUG' || (c.what === 'SQUARE' && c.tile === 4)));
        if (!v) f.push('no vein near the miners: ' + [hx, hy]);
        else if (!mine.length) f.push('nothing of the mine dug away from the character');
        for (const c of mine) {
          const w = dq0Vein(m.door.seed, c.x, c.y), across = c.y === hy && (c.x - hx) * (v.vx - c.x) >= 0;
          if (!across && !(w && Math.abs(c.x - w.vx) <= Math.abs(w.vw))) f.push('a square dug off the mine: ' + [c.x, c.y, c.what]);
        }
        const back = s.slice(s.lastIndexOf('> look'), s.lastIndexOf('> quit'));
        if (!/door[^.]*[(]closed[)]|one [^.]*[(]closed[)]/.test(back)) f.push('the door not seen shut, back up: ' + back);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wallup', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's builders walling up a way (slice 4.11's rest, 2026-11-02: DENIZENS.md's tunnellers and builders, HOUSE.md's builders, for review): a hand-written theme's dwarf miners (seed 374), the hour 8 in the morning (their working hours).  The character, with a pick, digs out through the cell's wall, a way into a room of the miners' home's chunk (the wall's square a passage, its change record); it goes down the stairway and waits four hours (a lair that found no dig to do asks again as the clock's high word moves), the miners away from it: at their work a miner, its faction's builder, walls the way up (dq0d's way_in, part 15's: a square a dig made a passage, a room's floor beside it, not of their own mine; its work coarse, I_KIT's KT_BUILD who 0: the world's wdig.c), the square a wall again by the world's change record",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 8 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 374 -c /ram/c.hm -k pick -T -d /ram/g -S /ram/s.sc',
        'step west', 'dig north', 'wait', 'step east', 'd', 'wait 2 hours', 'wait 2 hours', 'u', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), at = T.of('wait').at, fin = T.of('quit').at, d = T.of('dig north');
        if (!dq0Ev(d, 'E_DIG', { how: 'D_THROUGH' }).length && !dq0Ev(T.of('wait'), 'E_DIG', { how: 'D_THROUGH' }).length) f.push('the character not through the wall: ' + dq0Evs(d));
        const dug = at ? at.changes.filter(c => c.what === 'SQUARE' && c.tile === 4 && c.sn === at.pc.sn) : [];
        if (!dug.length) f.push('no square dug a passage, before the miners');
        for (const c of dug) {
          const now = (fin ? fin.changes : []).find(e => e.what === 'SQUARE' && e.sn === c.sn && e.x === c.x && e.y === c.y);
          if (!now || now.tile !== 2) f.push('the way not walled up: ' + JSON.stringify([c, now]));
        }
        if (f.length) f.push(s.slice(s.indexOf('> dig north'), s.indexOf('> dig north') + 700));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denportc', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's portcullis worked by a denizen (2026-11-02: the minds' leftovers of Phase 3C's one path, DENIZENS.md's a portcullis and a lock, HOUSE.md's denizens and doors, for review): dq0tunnel's scene, the dwarf's tries at the portcullis not given up: kept from the character by the portcullis let down (from the character's side, its winch's), it heaves at it from the far side (E_DEN DE_HEAVES, the character's code: wwin.c's heave, P_LIFT by its sheet), and raises it (DE_RAISES) or, held twice, strains at its bars (DE_BENDS), each told seen or heard, and each heard as the character's own would be (the user's, 2026-11-17, GAME-CRITIQUE.md's 4.2: the sound task's), a portcullis raised (the world's *chains), a strain a work noise (*dig); its way there found (the minds' ways: a field made, dq0d's rb).  [dq0tunnel's what, for the scene:] Dingus Quest Zero's tunnellers (slice 4.11, 2026-11-01: DENIZENS.md's tunnellers and builders, HOUSE.md's tunnellers, for review): a hand-written theme's dwarf miners (seed 1, south of the start, wary; a miner digs with the tools it carries, kinds.txt's ways m: its pick, outfits.txt's [role khuzmin]), the character in their guardroom, the portcullis it came in by let down behind it ('close the door'); one of them put in the passage beyond it, hostile, drawn on to where it last sensed the character (the scene set as the first wait's typed: the hub's actor and its record, the minds' drawn and last-sensed square poked).  A portcullis it can't raise, no way nearer: it digs its way round it (dq0d's dig_way, a diagonal round the door: nearer as the crow flies; I_KIT's KT_DIG, the world's wdig.c a spell of ten minutes at a time by the character's rules: a pick's rate on the wall, a Strength test by its sheet), its first spell seen through the portcullis (E_DEN's DE_DIG: \"The dwarf miner digs into the wall.\", or heard, a scraping in rock), the hole kept as it goes (DUG), and through (DE_DUG: the square a passage, told seen, \"A fresh tunnel opens ...\", or heard, rock falling); the dwarf come through it after; both referees",
      init: 't_rc', cycles: 7000e6,   // (2026-11-08, one line of sight: the dwarf sees the character through the bars, heaves the portcullis up and comes at it, knocking it out: the game runs on till it comes round, 6.1G cycles; 3000e6 before)
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7 -S /ram/s.sc',
        'open the door', 'go through it', 'turn around', 'close the door ahead', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })), ymLog: true },
      start(m) {
        m.portc = true;
        dq0Played(m);
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.tun = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K) => {   // (The scene: one dwarf miner beyond the shut door ahead, hostile, drawn on)
                const u16 = o => h[o] | h[o + 1] << 8;
                const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
                const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], S = stratumRec(m, sn), ch = liveChunks(m, sn);
                const tile = (x, y) => {
                  const b = S && liveChunks(m, sn)[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)];
                  return b ? b[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1;
                };
                const at = k => L.actor + k * L.actorSize, named = k => {
                  let n = '';
                  for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
                  return n;
                };
                const f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
                const k = [...Array(L.nactors).keys()].find(j => named(j) === 'dwarf miner');
                if (k !== undefined && ch && tile(px + dx, py + dy) === 3 && [1, 4].includes(tile(px + 2 * dx, py + 2 * dy))) {
                  const x = px + 2 * dx, y = py + 2 * dy, o = at(k);
                  w16(o + L.aX, x);
                  w16(o + L.aY, y);
                  h[o + L.aSn] = sn;
                  h[o + L.aAttitude] = K.AT_HOSTILE;
                  h[o + L.aDoing] = 0;
                  const R = dq0Rec(m), q = R.at(h[o + L.aDen]);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  R.mem[q + R.SN] = sn;
                  R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  const ad = dq0Ad(lbl), ram = dq0LaterRam(m);
                  ram[ad('drawn') + k] = 8;
                  ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
                  ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.tun ={ k, door: [px + dx, py + dy], beyond: [x, y], pc: [px, py], tile };
                }
                done = true;
              }, null, 'the dwarf beyond the door', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), DE_DIG = 24, DE_DUG = 25;
        if (!m.tun) return ['the scene not set: no dwarf miner near, or no portcullis ahead with a floor beyond it'];
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.tun.k + 1), { K } = m.dq0.layout();
        if (!dens.some(e => e.a === K.DE_HEAVES)) f.push('the dwarf never heaved at the portcullis (E_DEN DE_HEAVES)');
        if (!dens.some(e => e.a === K.DE_RAISES || e.a === K.DE_BENDS)) f.push('the portcullis neither raised nor its bars strained at (DE_RAISES, DE_BENDS)');
        if (!/The dwarf miner sets to heaving at the portcullis\.|iron scraping/.test(s)) f.push('its heaving neither seen nor heard');
        for (const [de, snd, what] of [[K.DE_RAISES, WSND.chains, 'the portcullis it raised not heard as one raised (the world sound *chains)'],
          [K.DE_BENDS, WSND.dig, 'its strain at the bars not heard as a work noise (the world sound *dig)']]) {   // (As the character's own: 2026-11-17)
          if (dens.some(e => e.a === de) && !m.played.some(p => p.who === m.tun.k + 1 && p.snd === snd)) f.push(what + ': ' + JSON.stringify(m.played.filter(p => p.who)));
        }
        this.notes = ['the dwarf heard as: ' + [...new Set(m.played.filter(p => p.who === m.tun.k + 1).map(p => p.snd))].join(' ')];
        for (const e of dens.filter(e => e.a >= K.DE_OPENS)) if (e.w1 !== m.tun.door[0] || e.w2 !== m.tun.door[1]) f.push('a portal worked not the portcullis: ' + [e.a, e.w1, e.w2]);
        const h = m.dq0.bank(0), lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        if (!m.taskRam[h[12 + 2]][parseInt(/^al ([0-9A-F]+) \._rb$/m.exec(lbl)[1], 16)]) f.push('no way found (dq0d\'s rb: the ways\' bank never had)');
        if (f.length) f.push(JSON.stringify(dens.filter(e => e.a).map(e => [e.a, e.w1, e.w2, e.t])) + ' ' + s.slice(s.indexOf('> close the door'), s.indexOf('> close the door') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denportal', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's one path for every actor (Phase 3C, 2026-11-02: DUNGEON.md's one path, DENIZENS.md's doors, HOUSE.md's denizens and doors, for review): a denizen's mind posts the character's own intent (I_PORTAL) and the world carries it out by the character's parts (wdoor.c's, by the world's wport.c).  A hand-written theme's wary bandits (seed 1, dq0kit's); the character (a rope and a shortsword carried more, dq0 -k: twelve pounds, its burden as the scene was written before D13's free pounds) opens the door ahead and shuts it again, and a bandit is put beyond it, hostile, drawn on to where it last sensed the character (the scene set as the first wait's typed: the hub's actor and its record, the minds' drawn and last-sensed square poked): no step nearer but through the shut door, it opens it (E_DEN DE_OPENS at the door's square: 'The bandit opens the door.'), comes through and fights the character; and a bandit fleeing beyond the door (the scene set as the last wait's typed, 2026-11-02: a living one put there, fleeing, the minds' fled poked, whichever way the fight's dice went), the character beyond it, slams the door shut behind it (DE_SLAMS at the door: 'The bandit slams the door shut.', unseen if the character lies senseless as it's done, the user's, 2026-11-14; the door shut in its chunk at the end); each heard as the character's own would be (the user's, 2026-11-17, GAME-CRITIQUE.md's 4.2: the sound task's, a denizen's door work told but silent before): the opening played as a door opening (the world's *open), the slam as a door shutting (*shut), the bandit's own",
      init: 't_rc', cycles: 3000e6,
      machine: { ymLog: true, input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k rope,shortsword -S /ram/s.sc',
        'open the door', 'close the door', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        dq0DoorScene(m, 'wait', 'bandit');
        dq0Played(m);
        const a = m.acia, read = a.read;       // (The slam's scene, 2026-11-02, as the last wait's typed: whichever way the fight's dice went, a
        let typed = '';                        //   living bandit put beyond the open door, fleeing, the character on the other side: the minds' fled)
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 1 minute' && m.door && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K, at) => {
                const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
                const name = k => { let n = ''; for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]); return n; };
                const up = [...Array(L.nactors).keys()].filter(k => h[at(k) + L.aState] && name(k) === 'bandit' && h[at(k) + L.aDoing] !== K.DO_DOWN);
                const on = (k, q) => u16(at(k) + L.aX) === q[0] && u16(at(k) + L.aY) === q[1];
                const k = up.find(j => on(j, m.door.door)) ?? up.find(j => on(j, m.door.beyond)) ?? up[0];
                if (k !== undefined && (m.door.state(...m.door.door) & 1)) {
                  const [x, y] = m.door.beyond, o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]);
                  w16(o + L.aX, x);
                  w16(o + L.aY, y);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  h[o + L.aAttitude] = K.AT_UNFRIENDLY;
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  const ad = dq0Ad(lbl), ram = dq0LaterRam(m);
                  ram[ad('fled') + k] = 8;
                  ram[ad('sgen') + k] = 0;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.fleer = k;
                }
              }, null, 'the bandit fleeing beyond the door', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), DE_OPENS = 27, DE_SLAMS = 29;
        if (!m.door) return ['the scene not set: no bandit near, or no door ahead with a floor beyond it: ' + JSON.stringify(m.doorWhy)];
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.door.k + 1);
        const opens = dens.filter(e => e.a === DE_OPENS);
        if (!opens.length) f.push('the bandit never opened the door (E_DEN DE_OPENS)');
        const slammed = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.a === DE_SLAMS && e.w1 === m.door.door[0] && e.w2 === m.door.door[1]);
        if (!slammed.length) f.push('no bandit slammed the door shut as it fled (E_DEN DE_SLAMS)');
        for (const e of [...opens, ...slammed]) if (e.w1 !== m.door.door[0] || e.w2 !== m.door.door[1]) f.push('a door worked not the one between them: ' + [e.a, e.w1, e.w2]);
        if (opens.length && slammed.length && slammed[0].t < opens[0].t) f.push('slammed before it was opened');
        if (m.door.state(...m.door.door) & 1) f.push('the door open in its chunk at the end');
        if (!/The bandit opens the door\./.test(s)) f.push('its opening not seen: "The bandit opens the door."');
        const ko = T.flatMap(t => t.dens).find(e => e.kind === 'E_FIGHT' && e.def === 0 && (e.ff & 4)), up = ko && T.flatMap(t => t.events).find(e => e.kind === 'E_READY' && e.t >= ko.t);
        const lay = slammed.length && ko && slammed[0].t >= ko.t && (!up || slammed[0].t < up.t);   // (Senseless as it was slammed: nothing seen, the user's, 2026-11-14)
        if (!lay && !/The bandit slams the door shut\./.test(s)) f.push('its slam not seen: "The bandit slams the door shut."');
        if (lay && /The bandit slams the door shut\./.test(s)) f.push('its slam seen as it lay senseless');
        if (!T.flatMap(t => t.dens).some(e => e.kind === 'E_FIGHT' && e.who === m.door.k + 1)) f.push('the bandit never came through to fight');
        if (opens.length && !m.played.some(p => p.who === m.door.k + 1 && p.snd === WSND.open)) f.push('its opening not heard as a door opening (the world\'s *open): ' + JSON.stringify(m.played.filter(p => p.who)));
        if (slammed.length && !lay && !m.played.some(p => p.who === slammed[0].who && p.snd === WSND.shut)) f.push('its slam not heard as a door shutting (the world\'s *shut): ' + JSON.stringify(m.played.filter(p => p.who)));
        // (The act table's parity, 2026-11-19: the character's opening and shutting heard as the bandit's are, by the same
        // rows, at the door: its E_NOISE the row's loudness and NZ_DOOR, the bandit's E_DEN the row's loudness and DE_*)
        const A = dq0Acts(), [dx, dy] = m.door.door;
        let done = 0;
        for (const [line, r, po] of [['open the door', A.open, 'PO_OPENED'], ['close the door', A.shut, 'PO_CLOSED']]) {
          if (!dq0Ev(T.of(line), 'E_PORTAL', { po }).length) continue;   // (Open already as it begins: the scene's)
          const me = dq0Ev(T.of(line), 'E_NOISE', { a: r.loud, b: r.nz });
          done++;
          if (!me.length) f.push('"' + line + '" made no noise by its row (E_NOISE ' + r.loud + ', ' + r.nz + '): ' + dq0Evs(T.of(line)));
          else if (me[0].w1 !== dx || me[0].w2 !== dy) f.push('"' + line + '"\'s noise not at the door: ' + [me[0].w1, me[0].w2]);
        }
        if (!done) f.push('the character neither opened nor shut the door');
        for (const [es, r] of [[opens, A.open], [slammed, A.slam]]) for (const e of es) if (e.a !== r.de || e.b !== r.loud) f.push('the bandit\'s ' + r.name + ' not by its row: ' + [e.a, e.b] + ' not ' + [r.de, r.loud]);
        if (f.length) f.push(JSON.stringify(dens.filter(e => e.a).map(e => [e.a, e.w1, e.w2, e.t])) + ' ' + s.slice(s.indexOf('> close the door'), s.indexOf('> close the door') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denunbar', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's denizen unbarring a door to open it, heard as the character's own (the user's, 2026-11-17: \"A key, a bar and a pick make quiet sounds\", and a denizen's unbarring that opens a door plays the door opening, as the character's does; built/2026-11-17-bodies.md's noticed: its PO_UNBARRED a DE_UNBARS, silent): dq0allydoor's scene (seed 1's bandits, the door ahead opened and shut, a bandit put beyond it and made the character's ally), the door barred from the bandit's side (its portal record's state and its bar's side poked as the look's typed); \"ask the bandit to open the door\" (its errand's PA_OPEN, the character's own act; a mind's own door plan lifts a bar first, PA_LIFT, two doings): the bandit lifts the bar and opens the door at once, the character's own unbarring (E_DEN DE_UNBARRED at the door's square: 'The bandit lifts the bar and opens the door.', the character's own words, PORTAL.unbarred's piece; heard, a door opening), played as the door opening, as the character's is (the world's *open)",
      init: 't_rc', cycles: 2500e6,
      machine: { ymLog: true, input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'open the door', 'close the door', 'look', 'ask the bandit to open the door', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, name: 'bandit', set: { attitude: 5, doing: 0 } }, { g: 1, line: 'ask the bandit to open the door', n: 1, name: 'bandit', set: { attitude: 5 } }]);
        dq0DoorScene(m, 'look', 'bandit', (h, o, L, K) => {   // (The door barred, its bar on the bandit's side: the way from the door to it)
          const f = h[L.pcFace], u16 = q => h[q] | h[q + 1] << 8;
          const x = u16(L.pcX) + [0, 1, 1, 1, 0, -1, -1, -1][f], y = u16(L.pcY) + [-1, -1, 0, 1, 1, 1, 0, -1][f];
          const live = !(f & 1) && dq0PortalAt(m, h[L.pcSn], x, y);   // (The cache's own bytes: its record's state, 3; its bar's side, 7)
          if (live) { live.b[live.o + 3] = (live.b[live.o + 3] & ~1) | 4; live.b[live.o + 7] = f >> 1; m.barred = { side: f >> 1 }; }
        });
        dq0Played(m);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), { K } = m.dq0.layout();
        if (!m.door || !m.barred) return ['the scene not set: no bandit near, no door ahead with a floor beyond it, or not barred: ' + JSON.stringify(m.doorWhy)];
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.door.k + 1 && e.w1 === m.door.door[0] && e.w2 === m.door.door[1]);
        const unbarred = dens.filter(e => e.a === K.DE_UNBARRED);
        if (!unbarred.length) f.push('the bandit never unbarred and opened the door (E_DEN DE_UNBARRED at the door)');
        if (dens.some(e => e.a === K.DE_UNBARS || e.a === K.DE_OPENS)) f.push('its unbarring told in two (DE_UNBARS, DE_OPENS), not as the character\'s one');
        if (!/The bandit lifts the bar and opens the door\.|a door opening/.test(s)) f.push('its unbarring neither seen ("The bandit lifts the bar and opens the door.") nor heard');
        if (unbarred.length && !m.played.some(p => p.who === m.door.k + 1 && p.snd === WSND.open)) f.push('the door it unbarred not heard opening (the world\'s *open): ' + JSON.stringify(m.played.filter(p => p.who)));
        if (m.door.state(...m.door.door) & 4) f.push('the door still barred in its chunk');
        const a = T.of('ask the bandit to open the door');
        if (!a || !dq0Rep(a, 'sway', { kind: 'SW_ORDER', order: 'OR_OPEN', result: 'SR_OK' }).length) f.push('the ally not asked to open the door (' + JSON.stringify(a && a.replies.filter(r => r.req === 'sway')) + ')');
        if (f.length) f.push(JSON.stringify(dens.map(e => [e.a, e.w1, e.w2, e.t])) + ' ' + s.slice(s.indexOf('> close the door'), s.indexOf('> close the door') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denkey', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's doors' keys and a denizen forcing a locked door (Phase 3C's one path, 2026-11-02: HOUSE.md's keys that fit, every actor's; DENIZENS.md's doors): at seed 1 (dq0denportal's wary bandits, the character with an iron key, dq0 -k), the character opens the door ahead and shuts it again; the door given a lock and the key made its own (the scene set as 'lock the door' is typed: the chunk's portal record's lock, the key's size and thick DOOR_KEY's), 'lock the door' locks it ('You lock it with its key.'); a bandit put beyond it, hostile, drawn on (dq0denportal's scene); it has no key, so it strains at the door (E_DEN DE_STRAINS: 'The bandit strains at the door.', or a door creaking) by the character's forcing (the referee's Strength test, its own sheet's), and, held, batters it (DE_BATTERS), each told seen or heard, till it's through or it gives up; each heard as the character's own would be (the user's, 2026-11-17, GAME-CRITIQUE.md's 4.2: the sound task's): its strain as work's noise (the world's *dig, the character's forcing's), a blow at the door as a blow (*blow), a door burst open as a door opening (*open)",
      init: 't_rc', cycles: 4000e6,
      machine: { ymLog: true, input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'iron key' -S /ram/s.sc",
        'open the door', 'close the door', 'lock the door', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        dq0DoorScene(m, 'wait', 'bandit');
        dq0Played(m);
        const a = m.acia, read = a.read;
        let typed = '';
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'lock the door' && m.dq0 && m.dq0.bank(0) && !m.keyed) dq0SceneNow(m, (h, L, K) => {   // (The door ahead given a lock, the key the character's)
                const u16 = o => h[o] | h[o + 1] << 8;
                const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], f = h[L.pcFace];
                const x = px + [0, 1, 1, 1, 0, -1, -1, -1][f], y = py + [-1, -1, 0, 1, 1, 1, 0, -1][f], P = dq0PortalAt(m, sn, x, y);
                const id = 0x8000 | ((x * 37 + y * 101 + sn * 4099) & 0x7FFF), per = Math.floor(0x2000 / L.itemSize);
                let key = -1;
                for (let i = 1; P && i < L.nitems && key < 0; i++) {
                  const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
                  if (b && b[o + L.iKind] === K.KI_KEY && b[o + L.iWhere] !== K.IW_DEN && b[o + L.iWhere] !== K.IW_SQUARE) {
                    b[o + L.iSize] = id & 255;
                    b[o + L.iThick] = id >> 8;
                    key = i;
                  }
                }
                if (P && key > 0) {
                  if (!P.b[P.o + 4]) P.b[P.o + 4] = 3;   // (Its lock: three steps worse to pick)
                  m.keyed = { door: [x, y], sn, key };
                }
              }, null, 'the door given a lock, its key', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), { K } = m.dq0.layout();
        if (!m.keyed) return ['the key not made the door\'s: no door ahead, or no iron key carried'];
        if (!m.door) return ['the scene not set: no bandit near, or no door ahead with a floor beyond it: ' + JSON.stringify(m.doorWhy)];
        if (!/> lock the door[^>]*You lock the door with its key\./.test(s)) f.push('"lock the door" not "You lock the door with its key."');
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.door.k + 1);
        const strains = dens.filter(e => e.a === K.DE_STRAINS), opens = dens.filter(e => e.a === K.DE_OPENS);
        const picks = dens.filter(e => e.a === K.DE_PICKING), picked = dens.some(e => e.a === K.DE_PICKS);   // (A robber's kind picks it first, 2026-11-02)
        if (!picks.length) f.push('the bandit never worked at the lock (E_DEN DE_PICKING: a robber\'s kind picks first)');
        if (!picked && !strains.length) f.push('the bandit, the lock not picked, never strained at the locked door (E_DEN DE_STRAINS)');
        if (opens.length && !picked && strains.length && opens[0].t < strains[0].t) f.push('the locked door opened before it was picked or forced');
        if (!/The bandit sets to picking the lock\.|a faint scratching/.test(s)) f.push('its picking neither seen nor heard');
        if (!picked && !/The bandit sets to forcing the door\.|a door creaking/.test(s)) f.push('its forcing neither seen nor heard');
        if (!picked && !dens.some(e => e.a === K.DE_BURSTS || e.a === K.DE_BATTERS)) f.push('its forcing neither burst the door nor, held, went on to battering');
        const heard = (de, snd, what) => { if (dens.some(e => e.a === de) && !m.played.some(p => p.who === m.door.k + 1 && p.snd === snd)) f.push(what + ': ' + JSON.stringify(m.played.filter(p => p.who))); };
        heard(K.DE_STRAINS, WSND.dig, 'its strain at the door not heard as a work noise (the world sound *dig)');
        heard(K.DE_BATTERS, WSND.blow, 'its blows at the door not heard as a blow (the world sound *blow)');
        heard(K.DE_BURSTS, WSND.open, 'the door it burst not heard opening (the world sound *open)');
        heard(K.DE_PICKING, WSND.click, 'its picking at the lock not heard, quiet (the world sound *click: 2026-11-17)');
        heard(K.DE_PICKS, WSND.click, 'the lock it picked not heard, quiet (the world sound *click)');
        // (The character's own key turned in the lock: a quiet noise the minds hear, E_NOISE NZ_CLICK of 1, and played, its
        // *click: the user's, 2026-11-17, "A key, a bar and a pick make quiet sounds ... for every actor")
        const lk = T.of('lock the door');
        if (!dq0Ev(lk, 'E_NOISE', { a: 1, b: K.NZ_CLICK }).length) f.push('the key turned in the lock made no quiet noise (E_NOISE NZ_CLICK, 1)');
        if (!m.played.some(p => p.who === 0 && p.snd === WSND.click)) f.push('the key turned in the lock not played (the world sound *click): ' + JSON.stringify(m.played.filter(p => !p.who)));
        // (The act table's parity, 2026-11-19: the character's key heard at the door by its row; the bandit's picking, its
        // forcing and their ends by theirs; its forcing the referee's P_FORCE of its own sheet, the character's purpose)
        const A = dq0Acts(), mine = dq0Ev(lk, 'E_NOISE', { a: A.lock.loud, b: A.lock.nz });
        if (mine.length && (mine[0].w1 !== m.door.door[0] || mine[0].w2 !== m.door.door[1])) f.push('the key\'s noise not at the door: ' + [mine[0].w1, mine[0].w2]);
        for (const r of [A.picking, A.picked, A.forcing, A.burst, A.held]) for (const e of dens.filter(e => e.a === r.de)) if (e.b !== r.loud) f.push('the bandit\'s ' + r.name + ' not by its row: loud ' + e.b + ' not ' + r.loud);
        const forced = T.flatMap(t => t.replies).filter(q => q.kind === K.RS_TEST && q.a === A.forcing.p && q.actor === m.door.k + 1);
        if (strains.length && !forced.length) f.push('the bandit strained at the door with no test of its own sheet (RS_TEST P_FORCE, its actor)');
        this.notes = ['the bandit heard as: ' + [...new Set(m.played.filter(p => p.who === m.door.k + 1).map(p => p.snd))].join(' ')];
        if (process.env.DQ0SHOW) f.push(s.slice(s.indexOf('> lock the door'), s.indexOf('> lock the door') + 2500));
        for (const e of dens.filter(e => e.a >= K.DE_OPENS)) if (e.w1 !== m.door.door[0] || e.w2 !== m.door.door[1]) f.push('a door worked not the one between them: ' + [e.a, e.w1, e.w2]);
        if (f.length) f.push(JSON.stringify(dens.filter(e => e.a).map(e => [e.a, e.w1, e.w2, e.t])) + ' ' + s.slice(s.indexOf('> lock the door'), s.indexOf('> lock the door') + 1800));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dentake', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's denizens taking things by the character's take (Phase 3C's one path, 2026-11-02: DUNGEON.md's one path, DENIZENS.md's things taken, HOUSE.md's denizens taking things, for review): at seed 1 (dq0denportal's wary bandits), the character, given a sword (dq0 -k), drops it at its feet; a bandit is put beside it with no weapon to hand (the scene set as the first wait's typed: the hub's actor and its record, its agear's AG_ARMED off, as the world has it for one unarmed), so its mind wants a weapon and posts the character's own intent (I_TAKE, a its wants: TW_ARMS), and the world takes the sword up for it by the take rule (wtake.c: within its reach, not fixed, its hands), held, its own (the item store's IW_DEN, its number's), told ('The bandit snatches up its sword.': E_BEAT WB_PICKS, its kind)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k sword -S /ram/s.sc",
        'drop the sword', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.take = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K) => {   // (A bandit put beside the character, its agear unarmed)
                const u16 = o => h[o] | h[o + 1] << 8;
                const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
                const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], S = stratumRec(m, sn);
                const tile = (x, y) => { const b = S && liveChunks(m, sn)[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)]; return b ? b[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1; };
                const at = k => L.actor + k * L.actorSize, named = k => {
                  let n = '';
                  for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
                  return n;
                };
                const busy = (x, y) => [...Array(L.nactors).keys()].some(j => h[at(j) + L.aState] && u16(at(j) + L.aX) === x && u16(at(j) + L.aY) === y);
                const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
                const d = [[0, -1], [1, 0], [0, 1], [-1, 0]].find(([dx, dy]) => [1, 4].includes(tile(px + dx, py + dy)) && !busy(px + dx, py + dy));
                if (k !== undefined && d) {
                  const x = px + d[0], y = py + d[1], o = at(k);
                  w16(o + L.aX, x);
                  w16(o + L.aY, y);
                  h[o + L.aSn] = sn;
                  h[o + L.aDoing] = 0;
                  h[L.agear + k] &= ~1;           // (AG_ARMED: none to hand)
                  const R = dq0Rec(m), q = R.at(h[o + L.aDen]);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  R.mem[q + R.SN] = sn;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.take = { k, did: u16(o + L.aDid) };
                }
                done = true;
              }, null, 'the bandit beside, unarmed', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), { L, K } = m.dq0.layout();
        if (!m.take) return ['the scene not set: no bandit near, or no open square beside the character'];
        if (!/> drop the sword[^>]*(Put down\.|You put down the sword\.)/.test(s)) f.push('the sword not put down');
        const picks = T.flatMap(t => [...t.events, ...t.dens]).filter(e => e.kind === 'E_BEAT' && e.beat === 'WB_PICKS' && e.b === m.take.k + 1);
        if (!picks.length) f.push('the bandit never took up a thing (E_BEAT WB_PICKS)');
        if (!/The bandit snatches up their sword\./.test(s)) f.push('not told "The bandit snatches up their sword."');
        const per = Math.floor(0x2000 / L.itemSize);
        let sword = null;
        for (let i = 1; i < L.nitems; i++) {
          const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
          if (b && b[o + L.iWhere] === K.IW_DEN && b[o + L.iOwner] === (m.take.did & 255) && b[o + L.iKind] === ((picks[0] || {}).w1 >> 8)) sword = { i, flags: b[o + L.iFlags] };
        }
        if (picks.length && !sword) f.push('the thing taken not the bandit\'s own in the store (IW_DEN, its number)');
        if (sword && !(sword.flags & K.I_HELD)) f.push('the sword not held');
        if (f.length) f.push(JSON.stringify(picks) + ' ' + s.slice(s.indexOf('> drop the sword'), s.indexOf('> drop the sword') + 1200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denpit', outfit: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's things in a pit out of a denizen's reach from its edge (the user's, 2026-11-18: DECISIONS.md's \"The merged branches' questions\", H10, every actor's: wtake.c's take rule as the character's things.c): dq0dentake's scene (seed 1's bandits, the character given a sword), the sword dropped at its feet, a square beside the character made a pit by a poke and the sword put down in it (the item store's); a bandit with no weapon to hand put on a square beside the pit: it doesn't snatch the sword up from the pit's edge (no E_BEAT WB_PICKS of it while it stands outside the pit), the sword left in the pit unless it went down into it",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k sword -S /ram/s.sc",
        'drop the sword', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const sq = () => {                                   // (The character's chunk's bank, its tiles' offset, its square in it)
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), w = m.dq0.bank(WORLD_BANK_N), st = stratumRec(m, h[L.pcSn]);
          const px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8, ox = px - st.x0, oy = py - st.y0;
          const [TILE] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, tile)']);
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (!w[sl] || !b || w[sl + 1] !== h[L.pcSn] || (w[sl + 2] | w[sl + 3] << 8) !== ox >> 5 || (w[sl + 4] | w[sl + 5] << 8) !== oy >> 5) continue;
            return { b, t: o + TILE, x: ox & 31, y: oy & 31, px, py };
          }
          return null;
        };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: () => {   // (A floor square beside it a pit, another beside both for the bandit)
          const q = sq(); if (!q) return;
          const tile = (dx, dy) => { const x = q.x + dx, y = q.y + dy; return x < 0 || y < 0 || x > 31 || y > 31 ? -1 : q.b[q.t + y * 32 + x] & 15; };
          for (const [ax, ay, bx, by] of [[1, 0, 0, 1], [0, 1, -1, 0], [-1, 0, 0, -1], [0, -1, 1, 0]]) {
            if (tile(ax, ay) === 1 && tile(bx, by) === 1) {
              q.b[q.t + (q.y + ay) * 32 + q.x + ax] = q.b[q.t + (q.y + ay) * 32 + q.x + ax] & 0xF0 | 9;
              m.pit = [q.px + ax, q.py + ay];
              const { L, K } = m.dq0.layout(), per = Math.floor(0x2000 / L.itemSize);   // (The sword dropped at its feet put down in the pit)
              for (let n = 1; n < L.nitems; n++) {
                const b = m.dq0.bank(L.item0 + Math.floor(n / per)), o = (n % per) * L.itemSize;
                if (b && b[o + L.iWhere] === K.IW_SQUARE && (b[o + L.iX] | b[o + L.iX + 1] << 8) === q.px && (b[o + L.iY] | b[o + L.iY + 1] << 8) === q.py) {
                  b[o + L.iX] = m.pit[0] & 255; b[o + L.iX + 1] = m.pit[0] >> 8; b[o + L.iY] = m.pit[1] & 255; b[o + L.iY + 1] = m.pit[1] >> 8; m.moved = (m.moved || 0) + 1;
                }
              }
              m.side = [q.px + bx, q.py + by];
              return;
            }
          }
        } }, { g: 1, line: 'wait', n: 1, call: (h, L, K, at) => {
          if (!m.side) return;
          const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const named = k => { let n = ''; for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]); return n; };
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          const o = at(k), [x, y] = m.side;
          w16(o + L.aX, x); w16(o + L.aY, y); h[o + L.aSn] = h[L.pcSn]; h[o + L.aDoing] = 0;
          h[L.agear + k] &= ~1;                              // (AG_ARMED: none to hand, so it wants a weapon)
          const R = dq0Rec(m), r = R.at(h[o + L.aDen]);
          R.mem[r + R.X] = x & 255; R.mem[r + R.X + 1] = x >> 8; R.mem[r + R.Y] = y & 255; R.mem[r + R.Y + 1] = y >> 8; R.mem[r + R.SN] = h[L.pcSn];
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          m.take = { k, did: u16(o + L.aDid) };
        } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        if (!m.pit || !m.take) return ['the scene not set: no pit beside the character, or no bandit: ' + JSON.stringify([m.pit, m.take])];
        if (!m.moved) return ['the sword not put down in the pit'];
        for (const t of T) {
          const p = [...t.events, ...t.dens].filter(e => e.kind === 'E_BEAT' && e.beat === 'WB_PICKS' && e.b === m.take.k + 1);
          const a = t.after && t.after.actors.find(x => x.name === 'bandit' && x.x === m.side[0] && x.y === m.side[1]);
          if (p.length && (!t.after || t.after.actors.some(x => x.name === 'bandit' && x.x === m.pit[0] && x.y === m.pit[1])) === false) f.push('the bandit took a thing up, not in the pit: ' + t.line + (a ? ' (from the pit\'s edge)' : ''));
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0follow', near: ['denizens'], what: 'Dingus Quest Zero\'s following (the verbs plan\'s B6, 2026-11-10: DECISIONS.md\'s "Prayer, fasting, swimming and following"), by the words, the sway\'s replies and the character\'s squares: "follow me" an ally\'s order still (dq0fetch\'s seed 21 rat befriended twice, an ally: SW_ORDER OR_FOLLOW, SR_OK); at seed 5, "follow the sack" refused ("The sack isn\'t going anywhere."); down the ladder, "follow the rat" trails a giant rat that keeps its distance ("You follow the giant rat.", walked after it, the world\'s close-in re-aimed each time, looked round for as it goes behind), till it\'s lost ("You\'ve lost the giant rat.", "track" offered); out of reach as it begins, said so once then ("You follow the giant rat. You can\'t get near enough to the giant rat.", the user\'s, 2026-11-17: "A follow out of reach says so once, when it starts"), never again as it\'s re-aimed',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', lines: ['echo ml ani 90 >> /ram/a.hm',
        'dq0 -p t -s 21 -c /ram/a.hm -T -R 117', 'befriend the rat', 'befriend the rat', 'follow me', 'quit',
        'dq0 -p t -s 5 -c /ram/a.hm -T', 'follow the sack', 'd', 'wait', 'follow the rat', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), o = dq0Prose(out), f = [];
        const fm = T.of('follow me');
        if (!dq0Rep(fm, 'sway', { kind: 'SW_ORDER', order: 'OR_FOLLOW', result: 'SR_OK' }).length) f.push('"follow me" not the ally\'s order (' + JSON.stringify(fm.replies.filter(r => r.req === 'sway')) + ')');
        inTurn(o, ['> follow the sack The sack isn\'t going anywhere.', '> follow the rat You follow the giant rat.', 'You\'ve lost the giant rat. ("track" may find its trail.)'], f);
        const p = dq0Prose(out);              // (Its words in order, the status line's draws among them left out: its bar
                                                //   drawn as the follow's long task went on, 2026-11-18)
        const fo = p.slice(p.lastIndexOf('> follow the rat'), p.indexOf('You\'ve lost the giant rat.')).replace(/\s+/g, ' '), far = fo.split('You can\'t get near enough to the giant rat.').length - 1;
        if (far !== 1 || !/^> follow the rat You follow the giant rat\. You can't get near enough to the giant rat\./.test(fo)) f.push('out of reach as the follow began: not said once, then: ' + far + ' ' + fo.slice(0, 200));
        const fr = T.of('follow the rat');
        if (!fr.at || !fr.after || (fr.at.pc.x === fr.after.pc.x && fr.at.pc.y === fr.after.pc.y)) f.push('not walked after the rat');
        if (/in this game yet/.test(o)) f.push('a "not yet" said');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lurk', initiative: true, near: ['denizens'], what: 'Dingus Quest Zero\'s robbers lying in wait of themselves (A12: HOUSE.md\'s hiding oneself, the denizens hiding; initiative: true, as -I turns it off): a lair of hostile bandits about, the character standing with its torch out, waiting: a bandit that comes to sense it, unseen, in the dark or under cover, hides (its state past 1, watched line by line), its hiding the world\'s by the one code',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc", 'douse the torch', ...Array(12).fill('wait'), 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        m.lurked = [];
        dq0Scene(m, Array.from({ length: 12 }, (_, i) => ({ g: 1, line: 'wait', n: i + 1, call: (h, L, K, at) => { for (let k = 0; k < L.nactors; k++) m.lurked.push(h[at(k) + L.aState]); } })));
      },
      check(m, out) {
        const f = [];
        if (!m.lurked.some(v => v > 1)) f.push('no bandit hid: ' + [...new Set(m.lurked)].join(',') + ' ' + dq0Story(out).slice(-700));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lurkrest', initiative: true, near: ['denizens', 'time'], what: 'Dingus Quest Zero\'s robbers lying in wait for a character that lies down to rest (the user\'s, 2026-11-06: "robbers should come and rob a resting character"; HOUSE.md\'s those that want it, as it rests, for review): dq0lurk\'s bandits about in the dark, one hidden as the character waits; the character rests for an hour: the hidden one comes out at once and comes on (before, its two minutes up, it hid again at its next turn, a whole night by a resting character: dq0restbusy\'s scene), none hidden as the rest goes, the rest stopped as the character sees it coming in the cell\'s light',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc", 'douse the torch', ...Array(12).fill('wait'), 'rest for an hour', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        const R = this.R = { hid: [], during: [] }, a = m.acia, cpu = m.cpu;
        dq0Scene(m, [{ g: 1, line: 'wait', n: 12, call: (h, L, K, at) => { for (let k = 0; k < L.nactors; k++) R.hid.push(h[at(k) + L.aState]); } }]);
        const read = a.read;
        let cur = '', on = false, next = 0, n0 = -1;
        a.read = (r, c) => {
          const h = m.dq0 && m.dq0.bank && m.dq0.bank(0);
          if (r === 0 && a.rdrf) {
            const ch = a.rx;
            if (ch === 13) { on = cur === 'rest for an hour'; n0 = -1; cur = ''; }
            else if (ch >= 32 && ch < 127) cur += String.fromCharCode(ch);
          }
          if (on && h && h[0] === 0x44 && cpu.cyc >= next) {      // (Its actors' states as the rest goes, its game time with them)
            next = cpu.cyc + 2e6;
            const { L } = m.dq0.layout(), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16;
            if (n0 < 0) n0 = now;
            for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aState]) R.during.push({ k, t: now - n0, state: h[L.actor + k * L.actorSize + L.aState] });
          }
          return read(r, c);
        };
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], R = this.R, s = dq0Prose(out), rt = T.of('rest for an hour');
        if (!R.hid.some(v => v > 1)) return ['no bandit hidden before the rest: ' + [...new Set(R.hid)].join(',')];
        if (!R.during.length) f.push('the rest not watched');
        const late = R.during.filter(e => e.t > 300 && e.state > 1);     // (Thirty seconds on: its next turn's come)
        if (late.length) f.push('a bandit hidden as the character rests: ' + JSON.stringify(late.slice(0, 4)));
        // (The rest stopped by the robber: seen coming, or caught at the purse, gw3's, 2026-11-19: with facing a robber comes on
        // hidden in the dark and robs before it's seen, the theft its stop: the user's "come and rob a resting character".
        // Since the filch is the robber's Legerdemain (P_FILCH, a creature's Dexterity: the user's, 2026-11-23), its purse
        // cut unawares as it lies, the robber seen as it goes off with the coins, the rest stopped then: the dice's offsets
        // (-R 1 to 6) lose the scene's hidden bandit, so the scene is kept and that taken as its stop too)
        if (!/You settle down to rest for an hour\.[\s\S]*(bandit[^.]*coming towards|You catch something at your purse)[\s\S]*You stop waiting\./.test(s) &&
          !/You settle down to rest for an hour\.[\s\S]*A bandit is [^.]*\.[\s\S]*You stop waiting\.[\s\S]*You find your coins gone: someone robbed you as you lay\./.test(s)) f.push('not seen coming nor caught robbing, the rest not stopped: ' + JSON.stringify(s.slice(s.indexOf('> rest for an hour'), s.indexOf('> inventory'))));
        if (!dq0Ev(rt, 'E_TASK', {}).length) f.push('no E_TASK in the rest');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0robrest', initiative: true, near: ['denizens', 'time'], what: 'Dingus Quest Zero\'s robbers at a sleeper (the user\'s, 2026-11-06: "robbers should come and rob a resting character"; HOUSE.md\'s those that want it, as it rests, for review): a lair of hostile bandits about, the character asleep in the dark: at seed 1 a bandit comes to it and robs it as it lies, unnoticed (the book\'s Legerdemain, its Stealth, against the character\'s Awareness at half asleep: the referee\'s test of it in the minds\' slot, its steps the fewer by half its mastery), its coins alone taken (E_TASK TE_ROBBED, RB_PURSE), the sleep going on, told as it\'s up ("You find your coins gone: someone robbed you as you lay."); at seed 4 one caught at it, nothing taken (no robbery unawares), the sleep stopped ("You wake to find something at your purse!", RB_CAUGHT)',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc", 'douse the torch', 'sleep', 'inventory', 'quit', 'mkdir /ram/h', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/h/theme.txt",
        "dq0 -p t -s 4 -c /ram/c.hm -T -d /ram/h -S /ram/s.sc", 'douse the torch', 'sleep', 'quit', 'echo $status']) },   // (A folder of its own: the first's a life gone on
                                                // with, its coins gone; quit as it's asked, the fight with the robber not fought)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], RS_TEST = m.dq0.layout().K.RS_TEST, STEALTH = 4, FILCH = m.dq0.layout().K.P_FILCH, SOUND = 1, A = dq0Answers(out).filter(a => a.cmd === 'inventory');
        const gs = T.filter(t => /^dq0 -p t/.test(t.line)), sl = gs.map(g => T.of('sleep', g.k));
        if (sl.length !== 2 || sl.some(t => t.k < 0) || A.length !== 1) return ['the games not played'];
        const robs = sl.map(t => [...dq0Ev(t, 'E_TASK', { te: 'TE_ROBBED' }), ...dq0Ev(T[t.k + 1] || t, 'E_TASK', { te: 'TE_ROBBED' })]);
        const tests = t => ({ them: dq0Rep(t, 'dslot', { kind: RS_TEST, a: FILCH, actor: v => v > 0, other: v => v !== 0 }), it: dq0Rep(t, 'dslot', { kind: RS_TEST, a: SOUND, actor: 0 }),
          stalk: dq0Rep(t, 'dslot', { kind: RS_TEST, a: STEALTH, actor: v => v > 0, other: 0 }) });   // (Its sleight of hand, the asker's dice; its stalk, its own)
        const t0 = tests(sl[0]), t1 = tests(sl[1]), s = dq0Prose(out);
        if (robs[0].length !== 1 || !robs[0][0].purse || robs[0][0].caught || !robs[0][0].asleep) f.push('seed 1: not robbed once unawares as it slept: ' + JSON.stringify(robs[0]));
        if (!t0.it.length || !t0.them.length) f.push('seed 1: no Legerdemain against its Awareness: ' + t0.them.length + ' and ' + t0.it.length + ' tests');
        else if (!(t0.it[t0.it.length - 1].steps < 0)) f.push('seed 1: its Awareness asleep not at half: ' + JSON.stringify(t0.it));
        if (!t0.stalk.length) f.push('seed 1: the robber came at the sleeper not stalking it (no Stealth tested as it began)');
        if (!/You find your coins gone: someone robbed you as you lay\./.test(s)) f.push('seed 1: the robbery not told as it\'s up');
        if (/penn/.test(A[0].text)) f.push('seed 1: its pennies not taken: ' + A[0].text);
        if (!robs[1].length || !robs[1][0].caught || !robs[1][0].asleep || robs[1].some(e => e.purse && !e.caught)) f.push('seed 4: not caught at it as it slept: ' + JSON.stringify(robs[1]));   // (Then the fight; helpless in it, robbed as one is)
        if (!/You wake to find something at your purse!/.test(s)) f.push('seed 4: the robber caught not told');
        if (t1.it.length && t1.them.length && !(t1.it[t1.it.length - 1].grade > t1.them[t1.them.length - 1].grade)) f.push('seed 4: caught by the lesser level');
        if (!/at your purse![\s\S]*You stop sleeping\./.test(s)) f.push('seed 4: the sleep not stopped');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0killrest', initiative: true, near: ['denizens', 'time'], what: 'Dingus Quest Zero\'s killers at a sleeper (HOUSE.md\'s those that want it, as it rests, for review, 2026-11-06): a hand-written theme\'s hostile cultists (killers, kinds.txt\'s k), the character asleep in the dark at seed 1: one comes on and strikes the sleeper as its kind\'s rule says, not standing off, the blow with no defence (the book\'s Ignore, RS_ASLEEP); a person not seen told as a thing is, "Something strikes at you with its dagger" (the user\'s people "they", 2026-11-17: no word of what can\'t be seen)',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc', "echo 'faction | Foes | cultist | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc", 'douse the torch', 'sleep', 'echo $status']) },   // (Dead of it: the game over)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], RS_ATTACK = m.dq0.layout().K.RS_ATTACK, sl = T.of('sleep'), s = dq0Prose(out);
        if (sl.k < 0) return ['the game not played'];
        const all = [sl, ...T.filter(t => t.k > sl.k)];
        // (The blow as it was asked, its flags (msgs.def's RS_ATTACK w2): RS_ASLEEP its first alone, the sleeper woken by it (the
        // user's K11, 2026-11-21); the reply's w2 its time out, not the flags, read before)
        const asleep = all.flatMap(t => dq0Rep(t, 'slot0', { kind: RS_ATTACK, other: 0 })).filter(e => e.ask.asleep);
        if (asleep.length !== 1) f.push('not one blow at the sleeper with no defence (RS_ASLEEP), its first: ' + asleep.length);
        if (!/You settle down to sleep for eight hours\.[\s\S]*strikes at you/.test(s)) f.push('the sleeper not struck: ' + JSON.stringify(s.slice(s.indexOf('> sleep'), s.indexOf('> sleep') + 400)));
        // (A person not seen, "something": its words a thing's, "its", not a person's "their", the user's people "they",
        // 2026-11-17: blows.c's, not a word of what can't be seen)
        if (!/Something strikes at you with its /.test(s)) f.push('the cultist unseen not "Something strikes at you with its ...": ' + JSON.stringify(s.slice(s.indexOf('> sleep'), s.indexOf('> sleep') + 400)));
        const told = s.match(/[^.!?]*\b[Ss]omething\b[^.!?]*\b(?:they|their|them)\b[^.!?]*[.!?]/);
        if (told) f.push('one not seen given a person\'s words: ' + JSON.stringify(told[0]));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wakeblow', near: ['denizens', 'fight'], what: 'Dingus Quest Zero\'s sleeper struck wakes on the first blow (the user\'s K11, 2026-11-21: the first blow lands undefended, the book\'s Ignore, RS_ASLEEP, and wakes the sleeper; every actor alike, the world\'s one blow, wfight.c): a creature asleep (dq0noise\'s hand-written theme, its bandits at seed 1, one put beside the character asleep) attacked twice, the first blow at it with no defence and none after, the bandit awake from the first on (not DO_SLEEP); and the character asleep in the dark among hostile cultists (dq0killrest\'s theme, the character tough: Endurance 99), the first blow at it with no defence, none after, its sleep stopped by it (E_TASK TE_HIT)',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'echo hour 128 >> /ram/s.sc', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'attack the bandit', 'attack the bandit', 'quit',
        'hmchar -q -s 23 -u feudal -c hunter -o /ram/d.hm', 'echo att str 99 >> /ram/d.hm', 'echo att sta 99 >> /ram/d.hm', 'echo att wil 99 >> /ram/d.hm', 'mkdir /ram/h',
        "echo 'faction | Foes | cultist | 0-0 | 30 | - | 0 | h | -' > /ram/h/theme.txt", 'dq0 -p t -s 1 -c /ram/d.hm -T -d /ram/h -S /ram/s.sc', 'douse the torch', 'sleep', 'quit', 'echo $status']) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'attack the bandit', n: 1, name: 'bandit', set: { attitude: 3, doing: 10 }, beside: 4 }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], { K } = m.dq0.layout(), RS_ATTACK = K.RS_ATTACK, games = [], g = [];
        for (const t of T) { if (/^dq0 /.test(t.line)) games.push(g.splice(0)); g.push(t); }
        games.push(g.splice(0)); games.shift();
        if (games.length !== 2) return ['the two games not played: ' + games.length];
        // (The creature asleep: the character's blows at it, as asked; the first with no defence, the rest not)
        const at = games[0].flatMap(t => dq0Rep(t, 'slot0', { kind: RS_ATTACK, actor: 0 }).filter(e => e.other));
        if (at.length < 2) f.push('the bandit not struck at twice: ' + at.length);
        else {
          if (!(at[0].ask.asleep)) f.push('the sleeping bandit\'s first blow not undefended (RS_ASLEEP)');
          if (at.slice(1).some(e => e.ask.asleep)) f.push('a blow after the first at the bandit undefended still: ' + at.map(e => e.ask.asleep).join(' '));
        }
        const a1 = games[0].find(t => t.line === 'attack the bandit'), who = at.length ? at[0].other - 1 : -1;
        const doing = a1 && a1.after && who >= 0 ? (a1.after.actors[who] || {}).doing : undefined;
        if (doing === 'DO_SLEEP') f.push('the bandit asleep still after the first blow');
        // (The character asleep: the cultists' blows at it, the first with no defence, none after; its sleep stopped)
        const sl = games[1].find(t => t.line === 'sleep'), on = sl ? games[1].slice(games[1].indexOf(sl)) : [];
        const bl = on.flatMap(t => dq0Rep(t, 'slot0', { kind: RS_ATTACK, other: 0 }).filter(e => e.actor));
        if (bl.length < 2) f.push('the sleeper not struck at twice: ' + bl.length);
        else {
          if (!(bl[0].ask.asleep)) f.push('the sleeping character\'s first blow not undefended (RS_ASLEEP)');
          if (bl.slice(1).some(e => e.ask.asleep)) f.push('a blow after the first at the character undefended still: ' + bl.map(e => e.ask.asleep).join(' '));
        }
        if (!on.some(t => dq0Ev(t, 'E_TASK', { te: 'TE_HIT', task: 'LT_SLEEP' }).length)) f.push('the sleep not stopped by the blow');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denizens', meaning: true, what: 'Dingus Quest Zero\'s denizens (dq0d: the populations decided as a stratum is grown, from a stand-in theme\'s kinds, kinds.txt), by the actors\' records, their events and what the character perceives (their kinds\' records the referee\'s, HarnMaster\'s): the giant rats that lair by the top stratum\'s ways placed round the way down as the character starts there (dq0 -T), seen (the world\'s actors, in what it perceives), and moving about as time passes (their own task deciding): at the turns pace through all the time a minute\'s search takes (the clock on a slice at a time as they keep up), their stirring told (the world\'s E_VIEW) as what the character sees of them changes (gone out of sight), not again when nothing has; then (seed 5, down its ladder) the rats there seen, cornered, and in the character\'s way: the world stops it short of them, and won\'t let it walk or step onto a rat\'s square (dq0steps\' rat in the way, a step, in it)',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T', 'search', 'search', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'd', 'go right', 'go forward', 'step forward', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T'), s0 = g.after;
        const rats = s => s ? s.actors.filter(a => a.state && a.name === 'giant rat') : [];
        const near = (s, n) => rats(s).filter(a => a.sn === s.pc.sn && Math.max(Math.abs(a.x - s.pc.x), Math.abs(a.y - s.pc.y)) <= n);
        if (near(s0, 4).length < 2) f.push('the giant rats not placed round the way down: ' + JSON.stringify(rats(s0)));
        if (!s0 || !s0.sight || !s0.sight.dens.some(d => d.name === 'giant rat')) f.push('the rats not seen as it starts');
        const s1 = T.of('search'), s2 = T.after(s1, 'search'), at = r => r.x + ',' + r.y;
        if (!dq0Ev(s1, 'E_DEN', { de: 'DE_MOVED' }).length || rats(s1.after).map(at).join(' ') === rats(s0).map(at).join(' ')) f.push('the rats not moving about as the search went on');
        dq0Need(f, s1, 'E_VIEW', { why: 'V_STIR' }, 'the rats\' stirring not told as they went out of sight');
        if (!s1.after || !s1.after.sight || s1.after.sight.dens.length) f.push('the rats not gone out of sight');
        dq0Never(f, s2, 'E_VIEW', { why: 'V_STIR' }, 'the rats told stirring again, with nothing seen changed');
        const d = T.of('d');
        if (!d.after || !d.after.sight || d.after.sight.dens.filter(x => x.name === 'giant rat').length < 2) f.push('the rats by the ladder not seen as the character comes down');
        const r = T.of('go right'), gf = T.of('go forward'), sf = T.of('step forward');
        dq0Need(f, r, 'E_NOWAY', { why: 'N_DEN' }, 'not stopped short of the rats');
        for (const t of [gf, sf]) {
          dq0Need(f, t, 'E_NOWAY', { why: 'N_DEN' }, t.line + ': a rat not in the way');
          if (!t.at || !t.after || t.after.pc.x !== t.at.pc.x || t.after.pc.y !== t.at.pc.y) f.push(t.line + ': moved onto a rat\'s square');
        }
        // (A walk into a rat seen there not begun: its first words the world's, not "You set off west." the UI's at
        // once, 2026-11-17)
        if (/You set off [a-z]+\.\s+A giant rat is in the way/.test(dq0Story(out))) f.push('a walk into a rat told set off first');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0popgen', what: 'Dingus Quest Zero\'s population generator (Phase 3D: popgen, a bank program, bank/popgen; games/dq0d/pop.h) from the shell (bankrun): a theme file\'s inhabitants written by hand (the card\'s games/lib/dq0/inhabit.txt: four factions, one\'s line going on with the next), each faction read (its kinds, its strata, its number, its leader, its lair\'s stratum, its attitude, how it stands with the others, the worse of the two ways it\'s said), and each stratum\'s lairs: each faction\'s number spread over its strata, the most on its lair\'s, shared among its kinds (the first twice each other\'s share) in groups of its kind\'s size, its leader in its home; the same again (its own die, the stratum\'s seed)',
      init: 't_rc', cycles: 600e6,
      machine: { input: ['bankrun /sd/0/games/lib/dq0/popgen.bp /sd/0/games/lib/dq0 /sd/0/games/lib/dq0/inhabit.txt 3 >/ram/p1',
        'bankrun /sd/0/games/lib/dq0/popgen.bp /sd/0/games/lib/dq0 /sd/0/games/lib/dq0/inhabit.txt 3 >/ram/p2',
        'cat /ram/p1', 'cmp /ram/p1 /ram/p2 && echo same', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['popgen: 4 factions, 29 kinds\n',
        'Gnawers (rat bat): 40 on strata 0-3, their lair on 0; hostile to you; hostile to Diggers, wary of Brigands\n',
        'Diggers (kobold rat): 30 on strata 1-4, led by a koblord, their lair on 2; unfriendly to you; hostile to Gnawers, hostile to Brigands, hostile to Hollow\n',
        'Hollow (skel wight): 24 on strata 2-5, led by a shade, their lair on 4; hostile to you; hostile to Diggers\n',
        'stratum 0: rat 5 home, rat 4, rat 2, bat 5 (Gnawers); bandit 3 home, banlord 1 leader, bandit 3 (Brigands)\n',
        'stratum 4: kobold 4, rat 1 (Diggers); skel 4 home, shade 1 leader, skel 2, wight 2 (Hollow)\nstratum 5: skel 4, wight 2 (Hollow)\n',
        'echo same\nsame', '% echo done'],
      sdcard: true
    },
    {
      name: 'dq0inhab', what: 'Dingus Quest Zero\'s denizens from a theme\'s inhabitants (Phase 3D: the theme file in the save folder, theme.txt, its faction lines: games/dq0d/pop.h; written by hand here, the card\'s inhabit.txt, copied there before the game\'s begun): popgen (bank/popgen, a bank program of the denizens\' task\'s) places the theme\'s factions as each stratum is grown, in place of the stand-in\'s (kinds.txt\'s, factions.txt\'s): at seed 3 its Gnawers\' home, five giant rats (the stand-in\'s: two to five), by the top stratum\'s way down where the character starts (dq0 -T), and hostile to it (the theme\'s; the stand-in\'s vermin are wary), so they come at it while it searches; the game kept and carried on (dq0 -d), the rats hostile still (the theme read again, their records the save\'s: their attitudes read from their records as the kept game\'s first line is typed, the rats out of sight by then as the dice fall since the minds\' ways found, 2026-11-02).  And the world\'s deities\' names (deities.txt\'s, the card\'s lib/dq0/dq0g.txt) the command parser\'s words for them (CQ_WORLD\'s third list, after the things\' words and the names given: dq0c keeps them)',
      init: 't_rc', cycles: 1200e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'look', 'examine the rat', 'search', 'quit', 'dq0 -d /ram/g', 'examine the rat', 'quit',
        'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['Two giant rats are ahead to your left, 15 feet away.', 'The giant rat seems hostile.', 'The giant rat bites at you',
        '% dq0 -d /ram/g', '% echo $status'],
      // (The theme's five by the way down, its home: two of them in sight as the game begins, the rest round the
      // corner; the stand-in's at seed 3 are wary, and keep their distance)
      start(m) {   // (The rats' records as the kept game's 'examine the rat' is typed: their attitudes the save's; 2026-11-02, the
        const a = m.acia, read = a.read;   //   rats out of sight by then as the dice fall since the ways found)
        let typed = '', g = 0;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (/^dq0 /.test(typed)) ++g;
              if (g === 2 && typed === 'examine the rat' && m.dq0 && m.dq0.bank(0)) {
                const R = dq0Rec(m);
                m.ratAtt = [...Array(224).keys()].map(i => R.at(i)).filter(o => R.mem[o] === 1 && !(R.mem[o + R.FLAGS] & 4)).map(o => R.mem[o + R.ATTITUDE]);
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['> examine the rat', 'The giant rat seems hostile.', '> search', 'The giant rat bites at you', '> quit',
          '% dq0 -d /ram/g', '> examine the rat', '> quit'], f);   // (Hostile, or unfriendly after it, the character helpless: the fight's dice since slice 4.5's hearing)
        if (!m.ratAtt || !m.ratAtt.length || !m.ratAtt.some(t => t <= 1)) f.push('the kept game\'s rats not hostile still (their records\' attitudes: ' + JSON.stringify(m.ratAtt) + ')');   // (Hostile, or unfriendly after it, the character helpless: the fight's dice since slice 4.5's hearing)
        const want = Buffer.from(' \0\0' + fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0g.txt'), 'latin1') + '\0', 'latin1');
        if (!m.taskRam.some(r => Buffer.from(r.buffer, r.byteOffset, r.length).includes(want)))
          f.push('the deities\' names not the command parser\'s words (no task\'s RAM has them as CQ_WORLD\'s third list)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0sway', what: 'Dingus Quest Zero\'s attitudes and influence (DENIZENS.md; the user: "skills like animal husbandry could influence animals encountered"), by the denizens\' replies to the character\'s ways with them (the hub\'s sway: the result, the attitude before and after) and the actors\' records (the referee\'s Animalcraft tests in their order at this seed: HarnMaster\'s): a character good with animals (Animalcraft 90, hmchar\'s file) at seed 21\'s giant rats (wary, the vermin faction\'s), words refused (an animal: persuasion doesn\'t reach it), how one seems (examined: wary), befriended a test at a time (indifferent, then won over: an ally, following; then asked no more), named Grub, asked to wait and seen waiting as the character turns round, then to follow, following it across the room (seen beside it as it turns round again) and down the stairway (come along to the stratum below); the game kept (dq0 -d) and carried on, Grub its ally still; then a fresh game, a blow at a rat out of reach not struck, the rat as wary as it was',
      init: 't_rc', cycles: 1400e6,
      // (-R 117: the dice's offset that gives the game it was written for, since the dice by who asks,
      // 2026-10-30: the same dungeon, other rolls)
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', lines: ['echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -d /ram/kept -R 117',
        'persuade the rat', 'examine the rat', 'befriend the rat', 'befriend the rat', 'befriend the rat', 'name the rat grub', 'tell grub to wait', 'turn around',
        'follow me', 'go east', 'turn around', 'd', 'quit', 'dq0 -d /ram/kept -R 117', 'examine grub', 'quit', 'dq0 -p t -s 3 -c /ram/a.hm -T -R 117', 'attack the rat',
        'examine the rat', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const sw = (t, want, what) => { if (!dq0Rep(t, 'sway', want).length) f.push(what + ' (' + JSON.stringify(t.replies.filter(r => r.req === 'sway')) + ')'); };
        sw(T.of('persuade the rat'), { kind: 'SW_PERSUADE', result: 'SR_BEAST' }, 'words not refused for an animal');
        sw(T.of('examine the rat'), { kind: 'SW_LOOK', result: 'SR_OK', now: 'AT_WARY' }, 'the rat not seen wary');
        const b = T.filter(t => t.line === 'befriend the rat');
        sw(b[0], { kind: 'SW_BEFRIEND', result: 'SR_OK', was: 'AT_WARY', now: 'AT_INDIFFERENT' }, 'the first coaxing not swaying it to indifferent');
        sw(b[1], { kind: 'SW_BEFRIEND', result: 'SR_OK', was: 'AT_INDIFFERENT', now: 'AT_ALLIED' }, 'the second not winning it over');
        sw(b[2], { kind: 'SW_BEFRIEND', result: 'SR_AS_FAR' }, 'the third not refused (asked no more)');
        const grub = s => s ? s.actors.find(a => a.name === 'Grub') : null, near = s => { const g = grub(s); return g && g.sn === s.pc.sn && Math.max(Math.abs(g.x - s.pc.x), Math.abs(g.y - s.pc.y)) <= 2; };
        const seen = (t, doing) => t.after && t.after.sight && t.after.sight.dens.some(d => d.name === 'Grub' && d.doing === doing);
        const nm = T.of('name the rat grub');
        sw(nm, { kind: 'SW_NAME', result: 'SR_OK' }, 'the rat not named');
        if (!grub(nm.after) || grub(nm.after).attitude !== 'AT_ALLIED') f.push('no ally Grub');
        const w = T.of('tell grub to wait'), ta = T.after(w, 'turn around'), fm = T.of('follow me'), ge = T.of('go east'), ta2 = T.after(ge, 'turn around');
        sw(w, { kind: 'SW_ORDER', order: 'OR_WAIT', result: 'SR_OK' }, 'Grub not asked to wait');
        if (!seen(ta, 'DO_WAIT')) f.push('Grub not seen waiting as the character turned round');
        sw(fm, { kind: 'SW_ORDER', order: 'OR_FOLLOW', result: 'SR_OK' }, 'Grub not asked to follow');
        dq0Need(f, ge, 'E_ARRIVE', {}, 'the room not crossed');
        if (!seen(ta2, 'DO_FOLLOW') || !near(ta2.after)) f.push('Grub not seen following across the room, beside the character');
        const d = T.of('d');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        if (!d.after || d.after.pc.sn !== 1 || !near(d.after)) f.push('Grub not come along down the stairway');
        const c = T.of('dq0 -d /ram/kept');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        sw(T.of('examine grub'), { kind: 'SW_LOOK', result: 'SR_OK', now: 'AT_ALLIED' }, 'Grub not an ally still, carried on');
        const a = T.of('attack the rat');
        if (dq0Ev(a, 'E_FIGHT', { who: 0 }).length) f.push('a blow struck at a rat out of reach');
        sw(T.after(a, 'examine the rat'), { kind: 'SW_LOOK', now: 'AT_WARY' }, 'the rat not as wary as it was');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fetch', near: ['denizens'], what: 'Dingus Quest Zero\'s errands and gifts (slice 4.8, DENIZENS.md\'s fetching and influence; HOUSE.md\'s fetching and gifts, for review): at seed 21 (dq0sway\'s game) a giant rat befriended, won over and named Grub; two rations put down and the character two squares off, turned round to see them; Grub told to fetch the rations ("tell Grub to fetch the rations": the order\'s word not the thing\'s, the parser\'s), going to fetch them, picking them up in its mouth (two pounds: a beast\'s 3 lb at most) and handing them back to the character as it waits (E_TASK TE_FETCHED, FT_TOOK then FT_GAVE; the rations the character\'s again); asked to fetch a thing it can\'t see, refused; offered the tinderbox (no food: a beast has no use for it, SR_CANT); given a ration ("give 1 ration to Grub": its worth and food, the gift taken, a lot\'s one split off, FT_GOT, the character\'s rations one fewer); told to follow at 3 squares (SR_OK)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', lines: ['echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -R 117',
        'befriend the rat', 'befriend the rat', 'name the rat grub', 'drop 2 rations', 'move east 2 squares', 'turn around', 'tell grub to fetch the rations', 'wait', 'wait',
        'inventory', 'tell grub to fetch the anvil', 'give the tinderbox to grub', 'give 1 ration to grub', 'inventory', 'tell grub to follow me at 3 squares',
        'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const sw = (t, want, what) => { if (!dq0Rep(t, 'sway', want).length) f.push(what + ' (' + JSON.stringify(t.replies.filter(r => r.req === 'sway')) + ')'); };
        const b = T.filter(t => t.line === 'befriend the rat');
        if (!b.some(t => dq0Rep(t, 'sway', { now: 'AT_ALLIED' }).length)) f.push('the rat not won over');
        const rations = s => dq0Carried(s).filter(i => i.name === 'rations').reduce((n, i) => n + i.qty, 0);
        const d = T.of('drop 2 rations'), had = d.after ? rations(d.after) : 0;
        const fe = T.of('tell grub to fetch the rations');
        sw(fe, { kind: 'SW_ORDER', order: 'OR_FETCH', result: 'SR_OK' }, 'Grub not sent to fetch the rations');
        const ev = T.filter(t => t.k >= fe.k).flatMap(t => dq0Ev(t, 'E_TASK', { te: 'TE_FETCHED' }));
        if (!ev.some(e => e.ft === 'FT_TOOK')) f.push('Grub never picked the rations up (' + JSON.stringify(ev) + ')');
        if (!ev.some(e => e.ft === 'FT_GAVE')) f.push('Grub never handed the rations over (' + JSON.stringify(ev) + ')');
        const inv = T.of('inventory');
        if (!inv.after || rations(inv.after) !== had + 2) f.push('the rations not the character\'s again (' + (inv.after && rations(inv.after)) + ', ' + had + ' before)');
        if (dq0Rep(T.of('tell grub to fetch the anvil'), 'sway', {}).length) f.push('a thing not seen asked of the denizens');
        sw(T.of('give the tinderbox to grub'), { kind: 'SW_GIVE', result: 'SR_CANT' }, 'a beast taking a tinderbox');
        const g = T.of('give 1 ration to grub');
        sw(g, { kind: 'SW_GIVE', result: 'SR_OK' }, 'the ration not given');
        if (!dq0Ev(g, 'E_TASK', { te: 'TE_FETCHED', ft: 'FT_ATE' }).length) f.push('the ration not taken and eaten (' + dq0Evs(g) + ')');   // (Food given eaten as it's taken, T3: FT_GOT till 2026-11-24, held for ever counted eaten)
        const inv2 = T.after(g, 'inventory');
        if (!inv2.after || rations(inv2.after) !== had + 1) f.push('the character\'s rations not one fewer (' + (inv2.after && rations(inv2.after)) + ')');
        sw(T.of('tell grub to follow me at 3 squares'), { kind: 'SW_ORDER', order: 'OR_FOLLOW', result: 'SR_OK' }, 'Grub not told to follow at a distance');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fetchlid', near: ['denizens', 'gear'], what: 'Dingus Quest Zero\'s ally\'s errand to a chest\'s lid (the user\'s, 2026-11-16: every reader of a thing\'s place by its compartment, the errand\'s fetch among them; DECISIONS.md\'s "Compartments: the second stage"): dq0fetch\'s seed 21 rat befriended and named Grub, with a chest besides (dq0 -k): the chest set down, two rations put on its lid, the character two squares off and turned round; Grub told to fetch the rations: found where they lie (on the chest\'s lid, open to the hand, not in a thing closed), picked up and handed over (FT_TOOK, FT_GAVE), the rations the character\'s again',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', lines: ['echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -R 117 -k chest',
        'befriend the rat', 'befriend the rat', 'name the rat grub', 'drop the chest', 'close the chest', 'put 2 rations on the chest', 'move east 2 squares', 'turn around',
        'tell grub to fetch the rations', 'wait', 'wait', 'inventory', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const { IC_MARK, CP_ON } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        const p = T.of('put 2 rations on the chest').after, chest = p && p.items.find(i => i.name === 'chest');
        const on = p && chest ? p.items.filter(i => i.name === 'rations' && i.where === 'IW_ITEM' && i.x === chest.n) : [];
        if (on.length !== 1 || on[0].y !== (IC_MARK | CP_ON << 4 | 1)) f.push('the rations not on the chest\'s lid: ' + JSON.stringify(on));
        const fe = T.of('tell grub to fetch the rations');
        if (!dq0Rep(fe, 'sway', { kind: 'SW_ORDER', order: 'OR_FETCH', result: 'SR_OK' }).length) f.push('Grub not sent to fetch the rations (' + JSON.stringify(fe.replies.filter(r => r.req === 'sway')) + ')');
        const ev = T.filter(t => t.k >= fe.k).flatMap(t => dq0Ev(t, 'E_TASK', { te: 'TE_FETCHED' }));
        if (!ev.some(e => e.ft === 'FT_TOOK')) f.push('Grub never picked the rations up from the lid (' + JSON.stringify(ev) + ')');
        if (!ev.some(e => e.ft === 'FT_GAVE')) f.push('Grub never handed the rations over (' + JSON.stringify(ev) + ')');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denlid', near: ['denizens', 'gear'], what: 'Dingus Quest Zero\'s denizen\'s opening of a lid tips off what lies on it (the user\'s, 2026-11-17: phase A\'s bugs, "a denizen opening a lid slides things off": the one function, the character\'s, things.c\'s slide, called from wfight.c\'s den_pry, untold as a denizen\'s opening is; dq0lidslide the character\'s): dq0robcoins\' bandits (seed 1, a case more, dq0 -k), the case set down, the pennies put in it, closed, the dagger put on its lid, the case taken up (no hand free: into the pack); a bandit put beside the character, engaged, the character senseless (dq0PcShock\'s): robbed, the robber goes through its things for its coins (den_pry: the case opened, a closed one, to get at them), the dagger slid off the lid onto the floor where the character lies (a carried box\'s, the character\'s square), the pennies taken, the case left open in the pack',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k case -S /ram/s.sc',
        'drop the case', 'put the pennies in the case', 'close the case', 'put the dagger on the case', 'take the case', 'i', 'wait', 'wait', 'i', 'quit',
        'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face', engage: true }, { g: 1, line: 'wait', n: 1, call: () => dq0PcShock(m, 3000) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const { IC_MARK, CP_ON } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        const [i1, i2] = T.filter(t => t.line === 'i'), p = i1 && i1.after, box = p && p.items.find(i => i.name === 'case');
        const dg = p && box ? p.items.filter(i => i.name === 'dagger' && i.where === 'IW_ITEM' && i.x === box.n) : [];
        if (!box || !dq0Carried(p).includes(box) || !(box.shut & 1)) return ['the case not carried, closed: ' + JSON.stringify(box)];
        if (dg.length !== 1 || dg[0].y !== (IC_MARK | CP_ON << 4 | 1)) return ['the dagger not on the case\'s lid: ' + JSON.stringify(dg)];
        if (!p.items.some(i => i.name === 'pennies' && i.where === 'IW_ITEM' && i.x === box.n)) return ['the pennies not in the case'];
        if (!/goes through your things/.test(s)) f.push('not robbed');
        const q = i2 && i2.after, b2 = q && q.items.find(i => i.n === box.n), d2 = q && q.items.find(i => i.n === dg[0].n);
        if (!q || q.items.some(i => i.name === 'pennies' && dq0Carried(q).includes(i))) f.push('robbed, its pennies left on it');
        if (!b2 || (b2.shut & 1)) f.push('the case not opened for the coins in it: ' + JSON.stringify(b2));
        if (!d2 || d2.where !== 'IW_SQUARE' || d2.x !== q.pc.x || d2.y !== q.pc.y) f.push('the dagger not slid off the lid onto the floor where the character lies: ' + JSON.stringify(d2));
        if (/From the lid/.test(s.slice(s.indexOf('> wait')))) f.push('a denizen\'s opening told as the character\'s');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0actlid', meaning: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's act table (GAME-CRITIQUE.md's E6, ONE-PATH.md's 2.1: the one-path plan's stage 1, 2026-11-19, games/dq0w/acts.txt): a lid's lock forced by the character and by a denizen by the same row and the same senders: at seed 1 (dq0denlid's bandits and case, dq0 -k), the case set down, its lock (its own, kinds.c's) locked (its record's shut poked as 'force the case' is typed: no key to it), forced by the character (a long task, ten seconds: its noise the lidforce row's, loud, E_NOISE NZ_WORK where it stands; its test the referee's P_FORCE, its steps by the case's make, wood or not, how well made); the case made whole again, opened, the pennies put in it, closed, locked again (poked as 'take the case' is typed), taken up (into the pack); a bandit put beside the character, engaged, the character senseless (dq0PcShock's): its robber goes through its things and gets at the coins in the locked case (wfight.c's den_pry: no key of its own, forced; told robbed only if it took something) by the same row (its noise the lidforce row's, E_DEN DE_PRIES, as loud) and the same test (P_FORCE, its own sheet's, at the same steps for the same case)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k case -S /ram/s.sc',
        'drop the case', 'force the case', 'open the case', 'put the pennies in the case', 'close the case', 'take the case', 'i', 'wait', 'wait', 'i', 'quit',
        'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face', engage: true }, { g: 1, line: 'wait', n: 1, call: () => dq0PcShock(m, 3000) },
          { g: 1, line: 'wait', n: 1, call: (h, L, K) => dq0NoPicks(m) }]);   // (No robber's lockpicks: it forces the case, the test's act, not
                                                //   picks it, a bandit's purse's since 2026-11-21: dq0parlid's)
        const a = m.acia, read = a.read, IS_CLOSED = 1, IS_LOCKED = 2, IS_LOCK = 4;
        const shut = { 'force the case': IS_CLOSED | IS_LOCK | IS_LOCKED, 'open the case': IS_CLOSED, 'take the case': IS_CLOSED | IS_LOCK | IS_LOCKED };
        let typed = '';
        m.locked = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (shut[typed] !== undefined && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K) => {   // (The case's lock: its record's shut, the item store's)
                const per = Math.floor(0x2000 / L.itemSize);
                for (let i = 1; i < L.nitems; i++) {
                  const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
                  if (b && b[o + L.iKind] === K.KI_CASE) { b[o + L.iShut] = shut[typed]; m.locked.push(typed); break; }   // (The kit's: the first made)
                }
              }, null, 'the case\'s lock', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), A = dq0Acts(), { K } = m.dq0.layout();
        if (m.locked.length !== 3) return ['the case not given its lock as the lines were typed: ' + JSON.stringify(m.locked)];
        const fc = T.of('force the case'), all = T.flatMap(t => t.replies).filter(q => q.kind === K.RS_TEST && q.a === A.lidforce.p);
        const mine = all.filter(q => q.actor === 0), theirs = all.filter(q => q.actor > 0);
        if (!dq0Ev(T.and(fc), 'E_NOISE', { a: A.lidforce.loud, b: A.lidforce.nz }).length) f.push('the character\'s forcing made no noise by its row (E_NOISE ' + A.lidforce.loud + ', ' + A.lidforce.nz + '): ' + dq0Evs(fc));
        if (!mine.length) f.push('the character\'s forcing not tested (RS_TEST P_FORCE, actor 0)');
        const pries = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.a === A.lidforce.de);
        if (!pries.length) f.push('the robber never forced the case (E_DEN DE_PRIES)');
        for (const e of pries) if (e.b !== A.lidforce.loud) f.push('the robber\'s forcing not by its row: loud ' + e.b + ' not ' + A.lidforce.loud);
        if (!theirs.length) f.push('the robber\'s forcing not tested by its own sheet (RS_TEST P_FORCE, its actor)');
        if (mine.length && theirs.length && mine[0].steps !== theirs[0].steps) f.push('the same case forced at different steps: the character\'s ' + mine[0].steps + ', the robber\'s ' + theirs[0].steps);
        this.notes = ['steps ' + (mine[0] && mine[0].steps) + ' ' + (theirs[0] && theirs[0].steps) + '; the robber\'s forcing ' + (theirs[0] && theirs[0].grade > 0 ? 'burst it' : 'held')];
        if (f.length) f.push(s.slice(s.indexOf('> force the case'), s.indexOf('> force the case') + 2500));
        return f;
      },
      sdcard: true
    },
    {
      // (Search and hide, every actor's: the one-path plan's stage 2, 2026-11-21, ONE-PATH.md's 2.1 and 2.7)
      name: 'dq0srchact', outfit: true, meaning: true, near: ['denizens', 'senses'], what: "Dingus Quest Zero's search, every actor's by the same code (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, 2026-11-21; the user's, 2026-11-17: \"searching is anyone's\", \"a denizen's search seen or heard is told as the character's is\"): at seed 1 (dq0denfound's wary bandits, the start's cell lit by its sconce) the crowns hidden at the character's feet and made unknown, their hiding 2, and the cell's door made secret (its portal's state, PS_HIDDEN), both poked: the character's search about it (a minute, the act table's search row; no noise of its own but its busy noise, the row's loudness; the referee's P_SEARCH, actor 0, the door at 0 steps, the crowns at -2: wtrap.c's and things.c's tests); and the same scene, the character gone out through the door, shut, its torch out (dq0denfound's game 5), a bandit put on the crowns' square, hostile, drawn on to where it last sensed the character there: its own search, the character's (wsearch.c's den_search: its minute on its ready, told E_DEN DE_SEARCHES at the row's loudness; wtrap.c's search of the door by its own sight and its own sheet, the crowns by the same test, at the same steps); each find found as its test says (the door's PORTAL_STATE no longer hidden, handled (PP_KNOWN) by the character's find; the crowns known to the character, or uncovered by the bandit)",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k crowns -S /ram/s.sc', 'hide the crowns', 'look', 'search', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k crowns -R 17 -S /ram/s.sc',
        'hide the crowns', 'close the door', 'open the door', 'go through it', 'turn around', 'close the door ahead', 'put out the torch', 'turn around', 'go left', 'hide',
        'wait', 'wait 1 minute', 'wait 1 minute', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        const S = m.srch = { door: {}, hid: {}, put: null, ready: [] };
        const u16 = (h, o) => h[o] | h[o + 1] << 8;
        // (The crowns at the character's feet unknown, hidden at 2; the door it faces kept to be made secret: game g's)
        const scene = g => (h, L, K) => {
          const x = u16(h, L.pcX), y = u16(h, L.pcY), sn = h[L.pcSn], per = Math.floor(0x2000 / L.itemSize);
          S.hid[g] = [x, y, sn];
          for (let i = 1; i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (b && b[o + L.iKind] === K.KI_CROWNS && b[o + L.iWhere] === K.IW_SQUARE && b[o + L.iSn] === sn && u16(b, o + L.iX) === x && u16(b, o + L.iY) === y) {
              b[o + L.iFlags] = b[o + L.iFlags] & ~0x20 | DQ0_I.HIDDEN; b[o + L.iHide] = 2; S.hid[g].push(i);
            }
          }
          for (let j = -3; j <= 3; j++) for (let i = -3; i <= 3; i++) {   // (The cell's door within a search's reach: seed 1's, two
            const p = i * i + j * j < 19 && !S.door[g] && dq0Portal(m, sn, x + i, y + j);   //   squares south of the start)
            if (p && p.kind === 0) S.door[g] = { x: x + i, y: y + j, sn };
          }
        };
        const secret = g => { const d = S.door[g], p = d && dq0Portal(m, d.sn, d.x, d.y); if (p) { p.b[p.state] |= 0x20; d.made = 1; } };
        // (Game 2's: a bandit on the crowns' square, hostile, drawn on to it: where it last sensed the character, dq0denfound's)
        const searcher = (h, L, K, at, named) => {
          const H = S.hid[2];
          if (!H) return;
          secret(2);
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          dq0Alone(m, h, L, K, at, k);
          const sn = h[L.pcSn], o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; };
          w16(o + L.aX, H[0]); w16(o + L.aY, H[1]); h[o + L.aSn] = sn; h[o + L.aDoing] = 0; h[o + L.aState] = 1; h[o + L.aPos] = K.POS_STANDING;
          for (let i = 0; i < 4; i++) h[o + L.aReady + i] = h[L.hubNow + i];
          R.mem[q + R.X] = H[0] & 255; R.mem[q + R.X + 1] = H[0] >> 8; R.mem[q + R.Y] = H[1] & 255; R.mem[q + R.Y + 1] = H[1] >> 8; R.mem[q + R.SN] = sn;
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
          const ad = dq0Ad(lbl()), ram = dq0LaterRam(m);
          ram[ad('aloot') + k] = ram[ad('now') + 1];
          ram[ad('drawn') + k] = 8; ram[ad('sres') + k] = 0; ram[ad('asrch') + k] = 0;
          ram[ad('lsx') + 2 * k] = H[0] & 255; ram[ad('lsx') + 2 * k + 1] = H[0] >> 8;
          ram[ad('lsy') + 2 * k] = H[1] & 255; ram[ad('lsy') + 2 * k + 1] = H[1] >> 8;
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          S.put = { k, at: [H[0], H[1]] };
          S.world = h[12 + 1];                  // (The world's task and an actor's ready's offset: its search's start, below)
          S.rd = L.aReady;
        };
        // (The bandit's ready as each line's typed after it's put: its minute's search on it)
        const ready = (h, L) => { if (S.put) { const o = L.actor + S.put.k * L.actorSize + L.aReady; S.ready.push((h[o] | h[o + 1] << 8 | h[o + 2] << 16 | h[o + 3] << 24) >>> 0); } };
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, call: (h, L, K) => { scene(1)(h, L, K); secret(1); } },
          { g: 2, line: 'close the door', n: 1, call: scene(2) }, { g: 2, line: 'wait', n: 1, call: searcher },
          { g: 2, line: 'wait 1 minute', n: 1, call: ready }, { g: 2, line: 'wait 1 minute', n: 2, call: ready }, { g: 2, line: 'wait', n: 2, call: ready }]);
        // (Its search's real start and end: its ready as the world takes the search up and as it's done, the body's copy
        // (bd, bk) as its stub den_search is entered, the bandit's own, and as that call returns to its caller (its return
        // address, the stack as it was); the minds' catch-up can have it begin before the moment it's told, and what it
        // does after (its finds taken up) is its own time: the user's, 2026-11-24, build2's question 11)
        const lbw = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0w', 'dq0w.lbl'), 'latin1');
        const lw = n => parseInt(new RegExp('^al ([0-9A-F]+) \\._' + n + '$', 'm').exec(lbw)[1], 16), DS = lw('den_search'), BD = lw('bd'), BK = lw('bk');
        const cpu = m.cpu, step = cpu.step.bind(cpu), rdy = r => { const o = BD + S.rd; return (r[o] | r[o + 1] << 8 | r[o + 2] << 16 | r[o + 3] << 24) >>> 0; };
        let back = null;
        cpu.step = v => {
          if (S.put && S.ended === undefined && m.T === S.world) {
            const r = m.taskRam[S.world];
            if (!back && cpu.PC === DS && r[BK] === S.put.k) {
              S.began = rdy(r);
              back = { pc: (r[0x100 + ((cpu.S + 1) & 0xFF)] | r[0x100 + ((cpu.S + 2) & 0xFF)] << 8) + 1, s: (cpu.S + 2) & 0xFF };
            } else if (back && cpu.PC === back.pc && cpu.S === back.s) {
              S.ended = rdy(r);
            }
          }
          return step(v);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout(), S = m.srch, s = dq0Prose(out);
        if (!S.door[1] || !S.door[1].made || S.hid[1].length < 4 || !S.door[2] || !S.door[2].made || !S.hid[2] || S.hid[2].length < 4 || !S.put) return ['the scenes not set: ' + JSON.stringify(S)];
        const se = T.of('search'), q1 = T.of('quit'), q2 = T.of('quit', q1.k + 1), w0 = T.of('wait', q1.k + 1);
        const tests = list => list.filter(q => q.kind === K.RS_TEST && q.a === A.search.p);
        const mine = tests(se.replies).filter(q => q.actor === 0), theirs = tests(T.filter(t => t.k >= w0.k && t.k <= q2.k).flatMap(t => t.replies)).filter(q => q.actor === S.put.k + 1);
        const at = (list, st) => list.filter(q => q.steps === st);
        // (Its test: the same purpose, the same steps for the same door and the same crowns)
        if (!at(mine, 0).length) f.push('the character\'s search not testing the secret door at 0 steps: ' + JSON.stringify(mine.map(q => [q.steps, q.grade])));
        if (!at(mine, -2).length) f.push('the character\'s search not testing the crowns at -2 steps: ' + JSON.stringify(mine.map(q => [q.steps, q.grade])));
        if (!at(theirs, 0).length) f.push('the bandit\'s search not testing the secret door at 0 steps (its own sheet): ' + JSON.stringify(theirs.map(q => [q.steps, q.grade])));
        if (!at(theirs, -2).length) f.push('the bandit\'s search not testing the crowns at -2 steps (its own sheet): ' + JSON.stringify(theirs.map(q => [q.steps, q.grade])));
        // (Its time: the row's minute, the character's clock and the bandit's ready)
        const took = se.at && se.after ? se.after.now - se.at.now : -1;
        if (took !== A.search.t) f.push('the character\'s search not the row\'s minute: ' + took);
        const de = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.a === A.search.de && e.who === S.put.k + 1);
        if (de.length !== 1) f.push('the bandit\'s search not told once (E_DEN DE_SEARCHES): ' + de.length);
        else {
          if (de[0].b !== A.search.loud) f.push('the bandit\'s search not as loud as the row: ' + de[0].b);
          if (S.began === undefined || S.ended === undefined) f.push('the bandit\'s search never taken up and done by the world (den_search): ' + JSON.stringify([S.began, S.ended]));
          else if (S.ended !== S.began + A.search.t)        // (Its minute from its real start, exactly: the user's, 2026-11-24;
            f.push('the bandit not busy exactly the row\'s minute from its search\'s start (' + S.began + ' to ' + S.ended + ', told at ' + de[0].t + ')');   //   the told moment before)
          if (S.ended !== undefined && (!S.ready.length || S.ready[0] < S.ended)) f.push('the bandit\'s ready after its search less than as the search ended: ' + JSON.stringify([S.ended, S.ready]));
          S.told = de[0].t;
        }
        // (Its noise: the character's none of its own (its busy noise, the senses', the row's loudness: 2); the bandit's told)
        if (A.search.nz !== 0xFF || A.search.loud !== 2) f.push('the search row not the character\'s busy noise: ' + JSON.stringify(A.search));
        if (dq0Ev(se, 'E_NOISE', {}).length) f.push('the character\'s search made an event\'s noise: ' + dq0Evs(se));
        // (Its effect, as each test went: the door no longer secret, the crowns found)
        const rec = (t, d) => t.at && t.at.changes.find(c => c.what === 'PORTAL_STATE' && c.x === d.x && c.y === d.y);
        const ok = list => list.some(q => q.grade > 0 && !q.result);
        for (const [who, list, t, d] of [['the character', at(mine, 0), q1, S.door[1]], ['the bandit', at(theirs, 0), q2, S.door[2]]]) {
          const r = rec(t, d), open = r && !(r.tile & 0x20);
          if (ok(list) !== !!open) f.push(who + '\'s door test ' + JSON.stringify(list.map(q => [q.grade, q.result])) + ' but the door ' + (open ? 'found' : 'still secret') + ': ' + JSON.stringify(r));
          if (r && open && who === 'the character' && !(r.elev & 0x80)) f.push(who + '\'s find of the door not handled as its own (PP_KNOWN): ' + JSON.stringify(r));
        }
        if (ok(at(mine, 0)) !== !!dq0Ev(se, 'E_PORTAL', { po: 'PO_FOUND' }).length) f.push('the character\'s door find not told as its test went: ' + dq0Evs(se));
        const cr = (t, n) => t.at && t.at.items.find(i => i.n === n);
        const c1 = cr(q1, S.hid[1][3]), c2 = cr(q2, S.hid[2][3]);
        if (ok(at(mine, -2)) !== !!(c1 && (c1.flags & 0x20))) f.push('the character\'s crowns test ' + JSON.stringify(at(mine, -2).map(q => q.grade)) + ' but the crowns ' + JSON.stringify(c1));
        if (ok(at(theirs, -2)) !== !!(c2 && !(c2.where === 'IW_SQUARE' && (c2.flags & DQ0_I.HIDDEN)))) f.push('the bandit\'s crowns test ' + JSON.stringify(at(theirs, -2).map(q => q.grade)) + ' but the crowns ' + JSON.stringify(c2));
        this.notes = ['door ' + JSON.stringify([at(mine, 0).map(q => q.grade), at(theirs, 0).map(q => q.grade)]) + ', crowns ' + JSON.stringify([at(mine, -2).map(q => q.grade), at(theirs, -2).map(q => q.grade)]),
          'the bandit\'s crowns at the end ' + JSON.stringify(c2 && [c2.where, c2.flags]),
          'the bandit\'s search: begun ' + S.began + ', done ' + S.ended + ', told at ' + S.told + ', its ready at the next line ' + S.ready[0]];
        if (f.length) f.push(s.slice(s.indexOf('> search'), s.indexOf('> search') + 600));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hideact', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's hiding a thing, every actor's by the same code (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, 2026-11-21; the audit's C14): the act table's conceal row, its minute and its test (the referee's P_CONCEAL, its steps the row's), its grade the hiding's steps (12, 6, 2, at a critical failure 0: acts.h's HIDE_OF): at seed 1 the crowns hidden by the character (\"hide the crowns\": actor 0, its clock a minute on, the crowns hidden and known, their hiding by the grade); and a tribe of brown orcs (dq0hoard's second game: seed 1, south of the start, a brown orc keeps its things hidden), the character knocked senseless and robbed by one, the takings hidden at its home (RB_HIDE) by its own test (its actor's, its sheet's), the same row's steps, its minute on its ready, each thing's hiding by its grade (a fixed 1 before, with no test and no time)",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml dge 5 >> /ram/c.hm', 'echo ml una 5 >> /ram/c.hm',
        'echo ml dag 5 >> /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/h', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'echo hour 128 >> /ram/s.sc',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k crowns -S /ram/s.sc', 'hide the crowns', 'quit',
        "echo 'faction | Tribe | hyeka | 0-0 | 96 | - | 0 | h | -' > /ram/h/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k dagger -S /ram/s.sc',
        'defend none', 'hold the dagger', 'open the door', 's 3', ...Array(20).fill('wait'), 'wait 1 minute', 'wait 1 minute', 'quit', 'echo $status'] })) },   // (Two
                                                // minutes more: the robber carries its takings home, hidden there as it gets there, the user's C10, 2026-11-23)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout();
        const HIDE_OF = g => g === 2 ? 12 : g === 1 ? 6 : g === -1 ? 2 : 0;   // (acts.h's: G_WELL, G_PASS, G_FAIL, G_BADLY)
        const conceal = list => list.filter(q => q.kind === K.RS_TEST && q.a === A.conceal.p);
        const hc = T.of('hide the crowns'), q1 = T.of('quit');
        const mine = conceal(hc.replies);
        if (mine.length !== 1 || mine[0].actor !== 0 || mine[0].steps !== A.conceal.s) f.push('the character\'s hiding not the row\'s test (P_CONCEAL, actor 0, ' + A.conceal.s + ' steps): ' + JSON.stringify(mine.map(q => [q.actor, q.steps])));
        const took = hc.at && hc.after ? hc.after.now - hc.at.now : -1;
        if (took !== A.conceal.t) f.push('the character\'s hiding not the row\'s minute: ' + took);
        const c1 = q1.at && q1.at.items.find(i => i.name === 'crowns' && i.where === 'IW_SQUARE');
        if (!c1 || !(c1.flags & DQ0_I.HIDDEN) || !(c1.flags & 0x20) || (mine[0] && c1.hide !== HIDE_OF(mine[0].grade))) f.push('the crowns not hidden, known, their hiding by the grade: ' + JSON.stringify(c1) + ' ' + JSON.stringify(mine.map(q => q.grade)));
        const h2 = T.of('hold the dagger', q1.k + 1), q2 = T.of('quit', h2.k + 1);
        if (!h2.after || !q2.at) return [...f, 'the second game not played'];
        const robs = T.filter(t => t.k > h2.k).flatMap(t => dq0Ev(t, 'E_TASK', { te: 'TE_ROBBED' }));
        if (robs.length !== 1) return [...f, 'the second game: not robbed once: ' + robs.length];
        const by = robs[0].by + 1, theirs = conceal(T.filter(t => t.k > h2.k).flatMap(t => t.replies)).filter(q => q.actor === by);   // (E_TASK's by: its entry; an actor's, plus 1)
        if (theirs.length !== 1 || theirs[0].steps !== A.conceal.s) f.push('the robber\'s stash not hidden by the row\'s test (P_CONCEAL, its actor, ' + A.conceal.s + ' steps): ' + JSON.stringify(theirs.map(q => [q.actor, q.steps])));
        const b2 = dq0Carried(h2.after), taken = [dq0Named(b2, 'dagger')[0], dq0Named(b2, 'pennies')[0]].filter(Boolean), by2 = new Map(q2.at.items.map(i => [i.n, i]));
        for (const t of taken) {
          const i = by2.get(t.n);
          if (!i || i.where !== 'IW_SQUARE' || !(i.flags & DQ0_I.HIDDEN) || (theirs[0] && i.hide !== HIDE_OF(theirs[0].grade))) f.push('the ' + t.name + ' not hidden at the robber\'s home by its grade: ' + JSON.stringify(i) + ' ' + JSON.stringify(theirs.map(q => q.grade)));
        }
        // (Its minute on the robber's ready, as it hides them at its home, come there with them (the user's C10,
        //   2026-11-23: carried home): its next doing told no sooner)
        const sq = taken.map(t => by2.get(t.n)).find(i => i && i.where === 'IW_SQUARE'), mv = T.flatMap(t => t.dens).filter(e => e.who === by && e.t > robs[0].t);
        const near = sq ? mv.filter(e => (e.de === 'DE_MOVED' || e.de === 'DE_NOISE') && Math.abs(e.sx - sq.x) <= 1 && Math.abs(e.sy - sq.y) <= 1) : [];
        const next = e => mv.find(x => x.t > e.t && x.de !== 'DE_NOTICED' && x.de !== 'DE_LOST'), hid = near.find(e => !next(e) || next(e).t >= e.t + A.conceal.t);   // (Its step home, then its minute: its next doing, not what it notices)
        if (sq && !near.length) f.push('the robber never come home with its takings: ' + JSON.stringify(mv.slice(0, 6)));
        else if (sq && !hid) f.push('the robber about again within its minute\'s hiding: ' + JSON.stringify(near.map(e => [e.t, next(e) && next(e).t])));
        this.notes = ['grades ' + JSON.stringify([mine.map(q => q.grade), theirs.map(q => q.grade)])];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0sniffact', near: ['denizens', 'senses'], what: "Dingus Quest Zero's sniff, every actor's by the same rule (ONE-PATH.md's 2.1 and 2.7, the one-path plan's stage 2, 2026-11-21: \"sniff\" and \"track\" for anyone, the audit's M1 and M2): the act table's sniff row, its three seconds and its test (the referee's P_NOTICE_SMELL), its steps by the clearest scent's strength (trace.h's TRACE_STEPS: twice its points less 6, -8 to 6), the character's and a hunter's by smell alike: dq0scent's scene (seed 3, the theme's hostile giant rats, a keen nose, held beside the squares the character walks out through the doorway and round the corner, its torch put out, five minutes there): the rats let go on its trail sniff the traces' store as their journal's trail runs out (wtrscent.c's, a store sniff each ten seconds at most: their test by their own sheets, its time on their ready, a critical failure the trail lost), and the character's own sniff at the end (\"sniff\": its test, actor 0, the turn three seconds)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt', 'echo ml clu 60 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'step west', 's', 'go west', 'go west', 'douse the lit torch', ...Array(5).fill('wait 1 minute'), 'sniff', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {      // (dq0scent's scene: the rats put in the room beside its trail and held, till it's round the corner)
        const hold = (h, L) => { const now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, t = now + 900; for (let k = 0; k < L.nactors; k++) { const o = L.actor + k * L.actorSize + L.aReady; if (h[L.actor + k * L.actorSize + L.aName]) { h[o] = t & 255; h[o + 1] = t >> 8 & 255; h[o + 2] = t >> 16 & 255; h[o + 3] = 0; } } };
        const room = sq => (h, L) => { const R = dq0Rec(m); let n = 0; for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName]) { const a = L.actor + k * L.actorSize, o = R.at(h[a + L.aDen]), [x, y] = sq[n++ & 3]; h[a + L.aX] = R.mem[o + R.X] = x & 255; h[a + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8; h[a + L.aY] = R.mem[o + R.Y] = y & 255; h[a + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8; } h[L.denSeq] = h[L.denSeq] + 1 & 255; hold(h, L); };
        dq0Scene(m, [{ g: 1, line: 'step west', n: 1, call: room([[32722, 32721], [32722, 32722], [32721, 32721], [32721, 32722]]) },
          ...['douse the lit torch', 's', 'go west'].map(line => ({ g: 1, line, n: 1, call: hold })), { g: 1, line: 'go west', n: 2, call: hold }]);
      },
      check(m) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout();
        const sniffs = list => list.filter(q => q.kind === K.RS_TEST && q.a === A.sniff.p);
        const rule = q => q.steps >= -8 && q.steps <= 6 && !((q.steps - A.sniff.s) & 1);   // (Twice the points less 6, -8 to 6: even)
        const sn = T.of('sniff'), mine = sniffs(sn.replies).filter(q => q.actor === 0);
        if (mine.length !== 1) f.push('the character\'s sniff not tested once (P_NOTICE_SMELL, actor 0): ' + JSON.stringify(sniffs(sn.replies)));
        else if (!rule(mine[0])) f.push('the character\'s sniff not at the strength\'s steps: ' + mine[0].steps);
        const took = sn.at && sn.after ? sn.after.now - sn.at.now : -1;
        if (took !== A.sniff.t) f.push('the character\'s sniff not the row\'s three seconds: ' + took);
        const rats = sniffs(T.flatMap(t => t.replies)).filter(q => q.actor > 0);
        if (!rats.length) f.push('no rat\'s store sniff tested by its own sheet (P_NOTICE_SMELL, its actor)');
        const bad = rats.filter(q => !rule(q));
        if (bad.length) f.push('a rat\'s sniff not at the strength\'s steps: ' + JSON.stringify(bad.map(q => [q.actor, q.steps])));
        this.notes = ['the character ' + JSON.stringify(mine.map(q => [q.steps, q.grade])) + ', the rats ' + JSON.stringify(rats.map(q => [q.actor, q.steps, q.grade]))];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0parlid', meaning: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's one path for every actor, closures (ONE-PATH.md's stage 2, 2026-11-21: the act audit's G-8, a denizen picking a lid's lock): a lid's lock picked by the character and by a robber by the same row and the same test: at seed 1 (dq0actlid's bandits and case, dq0 -k with lockpicks), the case set down, its own lock locked (its record's shut poked as 'unlock the case' is typed: no key to it, so picked), picked by the character (a long task, a minute: its noise the lidpick row's, quiet, E_NOISE NZ_CLICK where it stands; its test the referee's P_PICKLOCK, its steps by the case's make); the case opened, the pennies put in it, closed, locked again (poked as 'take the case' is typed), taken up; a bandit put beside the character, engaged, the character senseless (dq0PcShock's): its robber goes through its things and gets at the coins in the locked case with its own lockpicks (a bandit's purse's, outfits.txt; wfight.c's den_pry: its key, else its lockpicks, else forced) by the same row (E_DEN DE_PICKING, as quiet) and the same test (P_PICKLOCK, its own sheet's, at the same steps for the same case)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k case,lockpicks -S /ram/s.sc',
        'drop the case', 'unlock the case', 'open the case', 'put the pennies in the case', 'close the case', 'take the case', 'i', 'wait', 'wait', 'i', 'quit',
        'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face', engage: true }, { g: 1, line: 'wait', n: 1, call: () => dq0PcShock(m, 3000) }, { g: 1, line: 'wait', n: 1, call: (h, L, K, at, named) => {   // (The character's lockpicks the robber's own,
            const per = Math.floor(0x2000 / L.itemSize), k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');   //   its things': its purse's
            const did = k === undefined ? 0 : h[at(k) + L.aDid] | h[at(k) + L.aDid + 1] << 8;   //   lockpicks, a bandit's, not always made by then)
            for (let i = 1; did && i < L.nitems; i++) {
              const b = m.dq0.bank(L.item0 + Math.floor(i / per)), q = (i % per) * L.itemSize;
              if (b && b[q + L.iKind] === K.KI_LOCKPICKS && b[q + L.iWhere] === K.IW_ACTOR) { b[q + L.iWhere] = K.IW_DEN; b[q + L.iOwner] = did & 255; b[q + L.iHide] = did >> 8; m.robPicks = i; }
            }
          } }]);
        const a = m.acia, read = a.read, IS_CLOSED = 1, IS_LOCKED = 2, IS_LOCK = 4;
        const shut = { 'unlock the case': IS_CLOSED | IS_LOCK | IS_LOCKED, 'open the case': IS_CLOSED, 'take the case': IS_CLOSED | IS_LOCK | IS_LOCKED };
        let typed = '';
        m.locked = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (shut[typed] !== undefined && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K) => {   // (The case's lock: its record's shut, the item store's)
                const per = Math.floor(0x2000 / L.itemSize);
                for (let i = 1; i < L.nitems; i++) {
                  const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
                  if (b && b[o + L.iKind] === K.KI_CASE) { b[o + L.iShut] = shut[typed]; m.locked.push(typed); break; }   // (The kit's: the first made)
                }
              }, null, 'the case\'s lock', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), A = dq0Acts(), { K } = m.dq0.layout();
        if (m.locked.length !== 3) return ['the case not given its lock as the lines were typed: ' + JSON.stringify(m.locked)];
        const u = T.of('unlock the case'), all = T.flatMap(t => t.replies).filter(q => q.kind === K.RS_TEST && q.a === A.lidpick.p);
        const mine = all.filter(q => q.actor === 0), theirs = all.filter(q => q.actor > 0);
        if (!dq0Ev(T.and(u), 'E_NOISE', { a: A.lidpick.loud, b: A.lidpick.nz }).length) f.push('the character\'s picking made no noise by its row (E_NOISE ' + A.lidpick.loud + ', ' + A.lidpick.nz + '): ' + dq0Evs(u));
        if (!mine.length) f.push('the character\'s picking not tested (RS_TEST P_PICKLOCK, actor 0)');
        const picks = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.a === A.lidpick.de);
        if (!picks.length) f.push('the robber never picked the case\'s lock (E_DEN DE_PICKING)');
        for (const e of picks) if (e.b !== A.lidpick.loud) f.push('the robber\'s picking not by its row: loud ' + e.b + ' not ' + A.lidpick.loud);
        if (!theirs.length) f.push('the robber\'s picking not tested by its own sheet (RS_TEST P_PICKLOCK, its actor)');
        if (mine.length && theirs.length && mine[0].steps !== theirs[0].steps) f.push('the same case picked at different steps: the character\'s ' + mine[0].steps + ', the robber\'s ' + theirs[0].steps);
        if (!m.robPicks) f.push('the robber not given its lockpicks as the first wait was typed');
        this.notes = ['steps ' + (mine[0] && mine[0].steps) + ' ' + (theirs[0] && theirs[0].steps) + '; the robber\'s picking ' + (theirs[0] && theirs[0].grade > 0 ? 'opened it' : 'failed')];
        if (f.length) f.push(s.slice(s.indexOf('> unlock the case'), s.indexOf('> unlock the case') + 2500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denpace', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's denizens' paces (the user's, 2026-11-03: \"walk unless hurried, and closing in to attack should be hurried unless they are cautious\": HOUSE.md's their paces), by the time between an actor's steps (its E_DEN DE_MOVEDs: the world's wbody.c, every actor's): a hand-written theme's hostile bandits (seed 1), a kind whose walk is 20 tenths a square: coming at the character, at its jog, 10; a theme's hostile giant rats (seed 3), a shy kind whose walk is 12, cautious: coming on at its walk, no quicker, for all it's hostile; both referees",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'wait 1 minute', 'wait 1 minute', 'quit',
        "echo 'faction | Gnawers | rat | 0-0 | 30 | - | 0 | h | -' > /ram/h/theme.txt", 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/h -S /ram/s.sc', 'wait 1 minute', 'wait 1 minute', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [];
        // (Its steps in a slice of the clock, ten seconds at the turns pace, an actor's most: the events' t the slice's)
        const most = g => {
          const by = {};
          for (const t of T) for (const e of t.dens) if (e.game === g && e.kind === 'E_DEN' && e.de === 'DE_MOVED') { const k = e.who + '/' + e.t; by[k] = (by[k] || 0) + 1; }
          return Math.max(0, ...Object.values(by));
        };
        const band = most(0), rats = most(1);
        this.notes = [band + ' ' + rats];
        if (band < 6 || band > 11) f.push('a hostile bandit not at its jog: ' + band + ' steps in ten seconds');
        if (!rats || rats > 9) f.push('a shy hostile rat not at its walk: ' + rats + ' steps in ten seconds');
        return f;
      },
      sdcard: true
    },
    {
      // (An entry's state its own actor's: GAME-CRITIQUE.md's 3.5, 2026-11-17)
      name: 'dq0freshslot', meaning: true, near: ['denizens'], what: "Dingus Quest Zero's actors' entries made fresh as they're reused (GAME-CRITIQUE.md's 3.5, 2026-11-17: the minds' per-actor arrays reset by hand, drawn and fooled never, \"a hostile denizen enlisted into a dead one's slot can inherit drawn on with its target zeroed\"), by the minds' arrays (dq0d's, by their labels) and the hub's own per-actor bytes as the actors are: at seed 5 (dq0denizens' second game) the character at the top's way down among its rats, every entry's state poked full of what no fresh one has (drawn on a long while, fooled, its door's tries past their dozen, its rounds' room far on, its errand's square, the world's dig_t its mind told it ate, its things all AG_*) as it climbs down; the rats below enlisted into the entries the top's left: none of it theirs (drawn on no more than its eight decisions, not fooled, no door tried, no rounds, no errand's square, nothing eaten, none of the things it doesn't have)",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 5 -c /ram/c.hm -T', 'look', 'd', 'look', 'quit', 'echo $status'] })) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '';
        m.fresh = { was: null, now: null };
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13 && m.dq0 && m.dq0.bank(0) && (typed === 'd' || (typed === 'quit' && m.fresh.was))) {
              const scene = typed === 'd', body = (h, L) => {   // (At 'd' a scene: the hub's copy, the minds' own later)
              const at = k => L.actor + k * L.actorSize, ram = scene ? dq0LaterRam(m) : m.taskRam[h[12 + 2]];
              const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
              const ad = dq0Ad(lbl);
              const read = k => ({ k, den: h[at(k) + L.aDen], name: (() => { let n = ''; for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]); return n; })(),
                state: h[at(k) + L.aState], drawn: ram[ad('drawn') + k], fooled: ram[ad('fooled') + k], atry: ram[ad('atry') + k], pway: ram[ad('pway') + k],
                afx: ram[ad('afx') + 2 * k] | ram[ad('afx') + 2 * k + 1] << 8, digt: h[L.digT + 2 * k] | h[L.digT + 2 * k + 1] << 8, agear: h[L.agear + k] });
              if (typed === 'd') {             // (Every entry full of what no fresh one has)
                m.fresh.was = [...Array(L.nactors).keys()].map(read);
                for (let k = 0; k < L.nactors; k++) {
                  ram[ad('drawn') + k] = 0x77; ram[ad('fooled') + k] = 0x5A; ram[ad('atry') + k] = 0xFF; ram[ad('pway') + k] = 0xEE;
                  ram[ad('afx') + 2 * k] = 0xEF; ram[ad('afx') + 2 * k + 1] = 0xBE;
                  h[L.digT + 2 * k] = 0x00; h[L.digT + 2 * k + 1] = 0xFD;   // (hub.h's TW_ATE)
                  h[L.agear + k] = 0xFF;
                }
              } else m.fresh.now = [...Array(L.nactors).keys()].map(read);
              };
              if (scene) dq0SceneNow(m, body, null, 'every mind\'s entry full', true); else body(m.dq0.bank(0), m.dq0.layout().L);
            }
            if (c === 13) typed = ''; else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m) {
        const f = [], F = m.fresh;
        if (!F.was || !F.now) return ['the entries never read: ' + JSON.stringify(F)];
        const anew = F.now.filter(e => e.state === 1 && e.name && !F.was.some(w => w.k === e.k && w.state === 1 && w.den === e.den && w.name === e.name));
        this.notes = ['entries reused: ' + anew.map(e => e.k + ' ' + e.name).join(', ')];
        if (!anew.length) f.push('no entry reused by another denizen as the character climbed down: ' + JSON.stringify(F.now.map(e => [e.k, e.name, e.state])));
        for (const e of anew) {
          const bad = [e.drawn > 8 && 'drawn ' + e.drawn, e.fooled > 1 && 'fooled ' + e.fooled, e.atry >> 4 > 12 && 'atry ' + e.atry,
            e.pway >= 0x80 && 'pway ' + e.pway, e.afx === 0xBEEF && 'afx ' + e.afx, e.digt === 0xFD00 && 'dig_t TW_ATE', e.agear === 0xFF && 'agear ' + e.agear].filter(Boolean);
          if (bad.length) f.push('entry ' + e.k + ' (' + e.name + ') holds its last actor\'s: ' + bad.join(', '));
        }
        return f;
      },
      sdcard: true
    },
    {
      // (Only what it's found: the user's, 2026-11-17; GAME-CRITIQUE.md's 3.1 and E4)
      name: 'dq0denfound', outfit: true, near: ['denizens'], what: "Dingus Quest Zero's denizens taking only what they've found (the user's, 2026-11-17: \"Denizens find hidden things only by a search of their own, by skill and time, where they can see or feel the place; what they haven't found they don't take\", and follow the character's compartment rules; GAME-CRITIQUE.md's 3.1: a robber took what the character hid, through walls, in the dark), by the item store's records, the hub's dig_t (what a denizen's take saw, told its mind) and the words: at seed 1 (dq0robcoins' wary bandits, robbers by their kind; the start's cell lit by its sconce) the crowns the character hid (a minute's work, its Conceal test) at its feet, a bandit put beside it: not taken, still hidden; the crowns let fall by the door, shut, a bandit put beyond it, two squares off: not seen through the shut door (no TW_SEEN told its mind), not taken; the same with the door open: seen (TW_SEEN told its mind as its look comes due; it shuts the door, the character beyond it); the crowns put in the pouch and the pouch tied shut, the character senseless and a bandit beside it engaged: robbed, the pouch untied for them (a robbery's reach by the compartments' rules); and the crowns hidden at the start (their hiding made the easiest), the character gone through the door, shut, its torch out, hiding itself: a bandit put on the crowns' square, hostile, drawn on to where it last sensed the character there, searches it (its own search, a minute's, by its sheet: the referee's P_SEARCH, a creature's by its Dexterity; the dice's offset 17, one it passes) and finds them, taken up",
      init: 't_rc', cycles: 6000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k crowns -S /ram/s.sc',
        'hide the crowns', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        'mkdir /ram/h', 'cp /ram/g/d1/theme.txt /ram/h/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k crowns -S /ram/s.sc', 'open the door', 'close the door', 'drop the crowns', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        'mkdir /ram/i', 'cp /ram/g/d1/theme.txt /ram/i/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/i -k crowns -S /ram/s.sc', 'close the door', 'open the door', 'drop the crowns', 'wait', 'wait', 'quit',
        'mkdir /ram/j', 'cp /ram/g/d1/theme.txt /ram/j/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/j -k crowns -S /ram/s.sc', 'put the crowns in the pouch', 'close the pouch', 'i', 'wait', 'wait', 'i', 'quit',
        'mkdir /ram/k', 'cp /ram/g/d1/theme.txt /ram/k/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/k -k crowns -R 17 -S /ram/s.sc',
        // (Game 5's walk on down the guardroom before the searcher's put, gw3's, 2026-11-19: with facing, a denizen turned senses
        // again, and the searcher on the crowns' square found the character in the guardroom beyond the portcullis, as it
        // would, and went for it: the character out of its senses, the search its own)
        'hide the crowns', 'close the door', 'open the door', 'go through it', 'turn around', 'close the door ahead', 'put out the torch', 'turn around', 'go left', 'hide',
        'wait', 'wait 1 minute', 'wait 1 minute', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        m.found = { put: {}, digt: {} };
        const lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        // (A bandit, the first actor of them, put on square x, y: its record too; the rest sent off; unhidden, standing)
        const put = (h, L, K, at, named, x, y, how) => {
          const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return null;
          dq0Alone(m, h, L, K, at, k);
          const sn = h[L.pcSn], o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]);
          w16(o + L.aX, x); w16(o + L.aY, y); h[o + L.aSn] = sn; h[o + L.aDoing] = 0; h[o + L.aState] = 1; h[o + L.aPos] = K.POS_STANDING;
          for (let i = 0; i < 4; i++) h[o + L.aReady + i] = h[L.hubNow + i];
          R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8; R.mem[q + R.SN] = sn;
          const ad = dq0Ad(lbl()), ram = dq0LaterRam(m);
          ram[ad('aloot') + k] = ram[ad('now') + 1];   // (Its look for things it wants due as it next decides: NOW_T)
          if (how) how(o, R, q, k);
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          return { k, did: u16(o + L.aDid), at: [x, y], ready: u16(o + L.aReady) };
        };
        // (Seed 1's: beyond the door the character faces, beside it, two squares off)
        const beyond = g => (h, L, K, at, named) => {
          const u16 = o => h[o] | h[o + 1] << 8, f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
          m.found.put[g] = put(h, L, K, at, named, u16(L.pcX) + 2 * dx, u16(L.pcY) + 2 * dy);
        };
        // (Game 5's: on the crowns' square, hostile, drawn on to it: where it last sensed the character)
        const searcher = (h, L, K, at, named) => {
          const H = m.found.hid;
          m.found.who = [...Array(L.nactors).keys()].map(j => [named(j), h[at(j) + L.aState], h[at(j) + L.aSn]]);
          if (!H) return;
          m.found.put[5] = put(h, L, K, at, named, H[0], H[1], (o, R, q, k) => {
            h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
            const ad = dq0Ad(lbl()), ram = dq0LaterRam(m);
            ram[ad('drawn') + k] = 8; ram[ad('sres') + k] = 0; ram[ad('asrch') + k] = 0;
            ram[ad('lsx') + 2 * k] = H[0] & 255; ram[ad('lsx') + 2 * k + 1] = H[0] >> 8;
            ram[ad('lsy') + 2 * k] = H[1] & 255; ram[ad('lsy') + 2 * k + 1] = H[1] >> 8;
          });
        };
        // (Game 5's: the crowns hidden at the character's feet made the easiest to find, hide 0, as it sets out)
        const easy = (h, L, K) => {
          const u16 = o => h[o] | h[o + 1] << 8, x = u16(L.pcX), y = u16(L.pcY), sn = h[L.pcSn], per = Math.floor(0x2000 / L.itemSize);
          m.found.hid = [x, y, sn];
          for (let i = 1; i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (b && b[o + L.iKind] === K.KI_CROWNS && b[o + L.iWhere] === K.IW_SQUARE && b[o + L.iSn] === sn && u16o(b, o + L.iX) === x && u16o(b, o + L.iY) === y &&
              (b[o + L.iFlags] & DQ0_I.HIDDEN)) { b[o + L.iHide] = 0; m.found.easy = i; }
          }
        };
        const u16o = (b, o) => b[o] | b[o + 1] << 8;
        // (The bandit's dig_t, what its take saw told its mind, as each line's typed: game 2's)
        const digt = g => (h, L) => { const P = m.found.put[g]; if (P) (m.found.digt[g] = m.found.digt[g] || []).push(h[L.digT + 2 * P.k] | h[L.digT + 2 * P.k + 1] << 8); };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face' },
          { g: 2, line: 'wait', n: 1, call: beyond(2) }, ...[2, 3, 4, 5, 6].map(n => ({ g: 2, line: 'wait', n, call: digt(2) })), { g: 2, line: 'quit', n: 1, call: digt(2) },
          { g: 3, line: 'wait', n: 1, call: beyond(3) }, { g: 3, line: 'wait', n: 2, call: digt(3) }, { g: 3, line: 'quit', n: 1, call: digt(3) },
          { g: 4, line: 'wait', n: 1, name: 'bandit', beside: 'face', engage: true }, { g: 4, line: 'wait', n: 1, call: () => dq0PcShock(m, 3000) },
          { g: 5, line: 'close the door', n: 1, call: easy }, { g: 5, line: 'wait', n: 1, call: searcher }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), s = dq0Story(out), G = s.split('% dq0 ').slice(1), f = [], F = m.found;
        if (G.length !== 5) return ['not five games: ' + G.length];
        const quit = g => T.filter(t => t.line === 'quit')[g], crowns = t => t && t.at ? t.at.items.filter(i => i.name === 'crowns') : [];
        const picks = g => T.flatMap(t => [...t.events, ...t.dens]).filter(e => e.game === g && e.kind === 'E_BEAT' && e.beat === 'WB_PICKS');
        // (Hidden at its feet, a bandit beside it: not taken, still hidden)
        const c1 = crowns(quit(0));
        if (!c1.length || c1.some(i => i.where !== 'IW_SQUARE' || !(i.flags & DQ0_I.HIDDEN))) f.push('the hidden crowns not lying hidden still: ' + JSON.stringify(c1.map(i => [i.where, i.flags, i.did])));
        if (picks(0).length) f.push('a bandit took up something beside the hidden crowns: ' + JSON.stringify(picks(0)));
        // (Beyond the shut door: unseen, nothing told its mind, not taken; the door open: seen, taken)
        if (!F.put[2] || !F.put[3]) f.push('a bandit not put beyond the door: ' + JSON.stringify(F.put));
        const c2 = crowns(quit(1)), c3 = crowns(quit(2));
        if ((F.digt[2] || []).some(w => w >> 8 === 0xFE)) f.push('the crowns beyond the shut door seen (its take\'s TW_SEEN): ' + JSON.stringify(F.digt[2]));
        if (!c2.length || c2.some(i => i.where !== 'IW_SQUARE')) f.push('the crowns beyond the shut door taken: ' + JSON.stringify(c2.map(i => [i.where, i.did])));
        if (!(F.digt[3] || []).some(w => w >> 8 === 0xFE)) f.push('the crowns through the open door not seen (no TW_SEEN told its mind): ' + JSON.stringify(F.digt[3]));
        // (The pouch tied shut: robbed, the pouch untied for its crowns)
        const i1 = T.filter(t => t.line === 'i')[0], i2 = T.filter(t => t.line === 'i')[1];
        const shut = (t, n) => { const it = t && t.after ? dq0Carried(t.after).find(i => i.name === n) : null; return it ? it.shut & 1 : -1; };
        const inPouch = i1 && i1.after && dq0Carried(i1.after).some(i => i.name === 'crowns' && i.where === 'IW_ITEM' && (i1.after.items.find(j => j.n === i.x) || {}).name === 'pouch');
        if (!inPouch || shut(i1, 'pouch') !== 1) f.push('the crowns not in the pouch tied shut as it began: ' + JSON.stringify(i1 && i1.after && dq0Carried(i1.after).map(i => [i.name, i.where, i.shut])));
        if (!/goes through your things/.test(G[3])) f.push('not robbed');
        if (i2 && i2.after && dq0Carried(i2.after).some(i => i.name === 'crowns')) f.push('robbed, its crowns left in the pouch');
        if (shut(i2, 'pouch') === 1) f.push('robbed of the crowns in the pouch, the pouch still tied shut');
        // (Game 5's search: the bandit drawn on to where it last sensed the character finds the hidden crowns, its own search)
        const P = F.put[5], c5 = crowns(quit(4));
        if (!F.easy) f.push('the hidden crowns not found in the store at the hiding square: ' + JSON.stringify(F.hid));
        if (!P) f.push('no bandit put on the crowns\' square: ' + JSON.stringify(F.who));
        else if (!c5.some(i => i.n === F.easy && (r => r.where === 'IW_DEN' && r.did === P.did)(dq0Root(quit(4).at, i)))) f.push('the hidden crowns not found by the bandit\'s search: ' + JSON.stringify(c5.map(i => [i.n, i.where, i.did, i.flags])));   // (Its own: held, or in its purse, coins' home: the one-path plan's stage 2, things, 2026-11-23)
        const tests5 = T.filter(t => t.game === 4).flatMap(t => t.replies).filter(r => r.req === 'slot0' && P && r.actor === P.k + 1).map(r => [r.rs, r.a, r.steps, r.grade]);
        const { K } = m.dq0.layout(), PS = K.P_SEARCH === undefined ? 3 : K.P_SEARCH;
        if (P && !tests5.some(r => r[0] === 'RS_TEST' && r[1] === PS)) f.push('no search of the bandit\'s own (the referee\'s P_SEARCH by its sheet): ' + JSON.stringify(tests5));
        this.notes = ['the bandit\'s tests in the search\'s game: ' + JSON.stringify(tests5), 'dig_t beyond the shut door: ' + JSON.stringify(F.digt[2]) + ', the open door: ' + JSON.stringify(F.digt[3])];
        return f;
      },
      sdcard: true
    },
    {
      // (DECISIONS.md's "Opening a portcullis", its open question 1: a denizen's opening a raise, timed, as the character's)
      name: 'dq0allyportc', meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's portcullis opened by a denizen as the character's is (2026-11-17: DECISIONS.md's \"Opening a portcullis\", its open question 1: a denizen's PA_OPEN at a portcullis raised it at once, untimed), by the denizens' events and the portcullis's state: at seed 1 (dq0denportc's wary dwarf miners in their guardroom) the portcullis the character came in by let down behind it by its winch, the character a step back from it; a dwarf put before it on the winch's side, an ally sent to open it (the minds' aorder OR_OPEN, its square, an errand's: \"ask it to open the door\"): it winds the winch, its long work begun (E_DEN DE_WINDS: the world's timed raise, the character's own, half a minute), and raises it as it's asked again, its time up (DE_RAISES), the portcullis up; then follows again",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7 -S /ram/s.sc',
        'open the door', 'go through it', 'turn around', 'close the door ahead', 'step back', 'wait', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        m.ally = null;
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: (h, L, K, at, named) => {
          const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && h[at(j) + L.aSn] === sn && named(j) === 'dwarf miner');
          if (k === undefined) { m.allyWhy = [...Array(L.nactors).keys()].map(j => [named(j), h[at(j) + L.aState], h[at(j) + L.aSn]]); return; }
          dq0Alone(m, h, L, K, at, k);
          const x = px + dx, y = py + dy, o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]);   // (Before the portcullis, its winch's side: where the character stood)
          w16(o + L.aX, x); w16(o + L.aY, y); h[o + L.aSn] = sn; h[o + L.aDoing] = 0; h[o + L.aState] = 1; h[o + L.aAttitude] = K.AT_ALLIED; h[o + L.aPos] = K.POS_STANDING;
          for (let i = 0; i < 4; i++) h[o + L.aReady + i] = h[L.hubNow + i];
          R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8; R.mem[q + R.SN] = sn;
          R.mem[q + R.ATTITUDE] = K.AT_ALLIED;
          const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
          const ad = dq0Ad(lbl), ram = dq0LaterRam(m);
          ram[ad('aorder') + k] = 5;            // (OR_OPEN, hub.h's)
          ram[ad('afetch') + 2 * k] = 0; ram[ad('afetch') + 2 * k + 1] = 0;
          const ux = px + 2 * dx, uy = py + 2 * dy;
          ram[ad('afx') + 2 * k] = ux & 255; ram[ad('afx') + 2 * k + 1] = ux >> 8;
          ram[ad('afy') + 2 * k] = uy & 255; ram[ad('afy') + 2 * k + 1] = uy >> 8;
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          m.ally = { k, door: [ux, uy], sn, name: named(k) };
        } }]);
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], { K } = m.dq0.layout(), A = m.ally;
        if (!A) return ['the scene not set: no dwarf miner near: ' + JSON.stringify(m.allyWhy)];
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === A.k + 1 && e.a >= K.DE_OPENS);
        const winds = dens.filter(e => e.a === K.DE_WINDS), raises = dens.filter(e => e.a === K.DE_RAISES);
        this.notes = [A.name + ': ' + JSON.stringify(dens.map(e => [e.a, e.w1, e.w2, e.t]))];
        if (!winds.length) f.push('the winch never wound, its work begun (E_DEN DE_WINDS): ' + JSON.stringify(dens.map(e => [e.a, e.t])));
        if (!raises.length) f.push('the portcullis never raised (E_DEN DE_RAISES)');
        if (winds.length && raises.length && raises[0].t - winds[0].t < 300) f.push('raised ' + (raises[0].t - winds[0].t) + ' tenths after its winding began, not half a minute');
        for (const e of dens) if (e.w1 !== A.door[0] || e.w2 !== A.door[1]) f.push('a portal worked not the portcullis: ' + [e.a, e.w1, e.w2]);
        const st = dq0Portal(m, A.sn, ...A.door);
        if (!st || !(st.b[st.state] & 1)) f.push('the portcullis not up at the end');
        return f;
      },
      sdcard: true
    },
    {
      // (The diagonal's own: a denizen's steps' time is the world's step_cost, the character's, 2026-11-17)
      name: 'dq0dendiag', meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's diagonal step for every actor (GAME-CRITIQUE.md's E4, the user's of 2026-11-17: \"the 7/5 diagonal for every actor\"; one step_cost, dq0w.c's, the character's and a denizen's, wbody.c's), by the time an actor's steps take (its ready, the hub's, as each line's typed) and the squares it steps to (its E_DEN DE_MOVEDs): at seed 3, every built passage on the diagonal (dq0 -A, dq0slant's), the character gone east from the kitchen along the passage that bends away northeast; a denizen put at the kitchen's door the character set out from, hostile, drawn on to where it stands (the minds' drawn and last-sensed square poked), the others sent off: its steps up the passage on the diagonal each seven-fifths of a step on the flat (its jog's 10 tenths, 14), where they were a straight step's",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 3 -c /ram/c.hm -A -d /ram/g -S /ram/s.sc', 'look',
        'go through the door to the east', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '';
        m.diag = { start: null, k: -1, at: [], why: '' };
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13 && m.dq0 && m.dq0.bank(0)) {
              const body = (h, L, K) => {     // (The scene's line's a scene: the hub's copy, the minds' own later; the others read as they are)
              const u16 = o => h[o] | h[o + 1] << 8, at = k => L.actor + k * L.actorSize;
              const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; }, u32 = o => (u16(o) + u16(o + 2) * 65536) >>> 0;
              const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn];
              if (typed === 'go through the door to the east') m.diag.start = [px, py, sn];
              if (typed === 'wait' && m.diag.k < 0 && m.diag.start) {   // (The scene: one put where the character set out from)
                const live = [...Array(L.nactors).keys()].filter(j => h[at(j) + L.aState] === 1 && h[at(j) + L.aSn] === sn);
                const k = live[0];
                m.diag.why = 'actors ' + live.length;
                if (k !== undefined) {
                  dq0Alone(m, h, L, K, at, k);
                  const [x, y] = m.diag.start, o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]);
                  w16(o + L.aX, x);
                  w16(o + L.aY, y);
                  h[o + L.aSn] = sn;
                  h[o + L.aAttitude] = K.AT_HOSTILE;
                  h[o + L.aDoing] = 0;
                  h[o + L.aPos] = K.POS_STANDING;
                  for (let i = 0; i < 4; i++) h[o + L.aReady + i] = h[L.hubNow + i];
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  R.mem[q + R.SN] = sn;
                  R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  const ad = dq0Ad(lbl), ram = dq0LaterRam(m);
                  ram[ad('drawn') + k] = 8;
                  ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
                  ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.diag.k = k;
                  m.diag.name = (() => { let n = ''; for (let i = 0; h[o + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[o + L.aName + i]); return n; })();
                }
              }
              if (m.diag.k >= 0 && (typed === 'wait' || typed === 'quit')) {   // (Where it is and when it's next free, as each line's typed)
                const o = at(m.diag.k);
                m.diag.at.push({ x: u16(o + L.aX), y: u16(o + L.aY), ready: u32(o + L.aReady), step: h[o + L.aReady + 4], doing: h[o + L.aDoing] });
              }
              };
              if (typed === 'wait' && m.diag.k < 0 && m.diag.start) dq0SceneNow(m, body, null, 'one put where the character set out from', true);
              else { const { L, K } = m.dq0.layout(); body(m.dq0.bank(0), L, K); }
            }
            if (c === 13) typed = ''; else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], D = m.diag;
        if (D.k < 0 || D.at.length < 2) return ['the scene not set: no actor on the stratum (' + D.why + ')'];
        // (Its first wait's steps, from the kitchen up the passage: each square it came to, the four ways' a straight
        // step, a corner's a diagonal; their time its ready's, the flat's, at its jog (hostile, coming at it: half its walk))
        const from = D.at[0], to = D.at[1], w = T.of('wait');
        let x = from.x, y = from.y, nd = 0, ns = 0;
        for (const e of w.dens) if (e.kind === 'E_DEN' && e.who === D.k + 1 && (e.sx !== x || e.sy !== y)) {
          if (Math.abs(e.sx - x) === 1 && Math.abs(e.sy - y) === 1) nd++; else ns++;
          x = e.sx; y = e.sy;
        }
        const s = from.step >> 1, d = s + Math.floor(2 * s / 5), want = ns * s + nd * d;
        this.notes = [D.name + ', its walk ' + from.step + ' tenths: ' + ns + ' straight steps, ' + nd + ' on the diagonal, ' + (to.ready - from.ready) + ' tenths'];
        if (nd < 3) f.push('fewer than three steps on the diagonal up the passage: ' + nd + ' (' + ns + ' straight)');
        if (to.ready - from.ready !== want) f.push('its steps took ' + (to.ready - from.ready) + ' tenths, not ' + want + ' (' + ns + ' straight at ' + s +
          ', ' + nd + ' on the diagonal at ' + d + '; ' + (ns + nd) * s + ' were every step a straight one\'s)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0noise', outfit: true, near: ['denizens', 'parser'], what: 'Dingus Quest Zero\'s words said aloud, a knock, and the minds swayed (the verbs plan\'s B2 and B3, 2026-11-10: DECISIONS.md\'s "Words said, a knock, the minds swayed"; minds.c, the world\'s I_NOISE, the denizens\' s_minded), by the words, the denizens\' sway and the senses\' answers (dq0trade\'s hand-written theme, its bandits at seed 1; Rhetoric, Oratory and Acting 95): "say" to a bandit beside and facing it, taken as talk (SW_TALK); a whisper heard at loudness 1, a yell at 6 by every bandit in hearing, a line in quotes said; a knock on the door (walked to and faced) heard at 3; a knock along the north wall, a short search by ear; "wake" awake ("You\'re awake."); a hostile bandit asleep beside it shaken awake, turning on it (SW_WAKE); an indifferent one taunted (SW_TAUNT, Rhetoric) made hostile, coming for it; a speech (SW_PERSUADE | SWO_ALL, one Oratory test) swaying those in earshot; ignorance feigned, a tale spun (deceive\'s); death feigned, lying down, the bandit beside taken in (Acting) and treating it as helpless, a robber robbing it; then seed 7\'s hall, its doors made secret (dq0Poke\'s traps), the door behind found by knocking along the north wall, and a speech with no one to hear it',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml rhe 95 >> /ram/c.hm', 'echo ml ora 95 >> /ram/c.hm', 'echo ml act 95 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'say "hello there"', 'whisper "psst"', 'yell "wake up!"', '"Hi"', 'knock on the door', 'knock on the north wall', 'wake', 'pretend ignorance', 'pretend dead',
        'wake the bandit', 'taunt the bandit', 'orate', 'quit',
        'echo ml awa 95 >> /ram/c.hm', 'dq0 -p t -s 7 -c /ram/c.hm -S /ram/s.sc', 'open the door behind me', 's', 'knock on the north wall', 'orate', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'say "hello there"', n: 1, name: 'bandit', set: { attitude: 3, doing: 1, face: 0 }, beside: 4 },
          { g: 1, line: 'yell "wake up!"', n: 1, name: 'bandit', set: { attitude: 3, doing: 10 } },
          { g: 1, line: 'wake the bandit', n: 1, name: 'bandit', set: { attitude: 0, doing: 10 }, beside: 4 },
          { g: 1, line: 'taunt the bandit', n: 1, name: 'bandit', set: { attitude: 3, doing: 1 }, beside: 4 },
          { g: 1, line: 'pretend dead', n: 1, name: 'bandit', set: { attitude: 0, doing: 1 }, beside: 4 },
          { g: 2, line: 'knock on the north wall', n: 1, traps: { hide: 0, plates: true } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), s = dq0Prose(out), f = [];
        inTurn(s, ['> say "hello there" You say, "hello there" You talk with the bandit.', '> whisper "psst" You whisper, "psst"',
          '> yell "wake up!" You yell, "wake up!"', '> "Hi" You say, "Hi"', '> knock on the door You knock on the door.',
          '> knock on the north wall You knock along the north wall, listening for a hollow...', '> wake You\'re awake.',
          '> pretend ignorance The bandit', '> pretend dead You lie still, feigning death. The bandit is taken in.',
          'goes through your things as you lie helpless', '> wake the bandit You shake the bandit awake. They come to with a snarl, and turn on you!',
          '> taunt the bandit You taunt the bandit. They\'re goaded: they come for you!', '> orate You hold', 'to those about you, in common.', '> knock on the north wall You knock along the north wall, listening for a hollow...',
          'You find a door hidden in the wall.', '> orate There\'s no one here to hear you.', '% echo $status'], f);
        if (/in this game yet/.test(s)) f.push('a "not yet" said: ' + (s.match(/> [^>]*in this game yet/) || [''])[0]);
        const sw = (line, want, what) => { const t = T.of(line); if (!t || !dq0Rep(t, 'sway', want).length) f.push(what + ' (' + JSON.stringify(t && t.replies.filter(r => r.req === 'sway')) + ')'); };
        sw('say "hello there"', { kind: 'SW_TALK', result: 'SR_OK' }, 'words to one facing it not taken as talk');
        sw('wake the bandit', { kind: 'SW_WAKE', result: 'SR_OK', now: 'AT_HOSTILE' }, 'the sleeper not woken');
        sw('taunt the bandit', { kind: 'SW_TAUNT', result: 'SR_OK', was: 'AT_INDIFFERENT', now: 'AT_HOSTILE' }, 'the bandit not goaded to hostile');
        sw('orate', { kind: 'SW_PERSUADE', result: 'SR_OK', grade: g => g > 0 }, 'no speech heard');
        sw('pretend dead', { kind: 'SW_DECEIVE', result: 'SR_OK' }, 'death not feigned to the foes');
        const heard = (line, loud) => { const t = T.of(line); return t ? dq0Rep(t, 'dsense', { noise: loud }).filter(r => r.res & 2).length : 0; };
        if (!heard('whisper "psst"', 1)) f.push('the whisper not heard (loudness 1)');
        if (heard('yell "wake up!"', 6) < 2) f.push('the yell not heard by the bandits about (loudness 6): ' + heard('yell "wake up!"', 6));
        if (!heard('knock on the door', 3)) f.push('the knock not heard (loudness 3)');
        sw('pretend ignorance', { kind: 'SW_DECEIVE', order: 'OR_NONE' }, 'ignorance feigned not a deception');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tongue2', outfit: true, near: ['denizens'], what: "Dingus Quest Zero's tongues (the user's, 2026-11-03: \"Book, plus leaders\": HOUSE.md's words reach a creature): a hunter with Rhetoric 95: a dwarf out of its reach (SR_TONGUE), its elder not, who knows common too; with a file's language khuzdul=10, the dwarf reached, the skill through it capped at its mastery (its test's steps down to 10); and hmchar's Scribe has a language line, a hunter none",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml rhe 95 >> /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc', "echo 'faction | Hold | khuzwar | 0-0 | 96 | khuzeld | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7 -S /ram/s.sc', 'open the door', 'south', 'wait', 'persuade the dwarf', 'quit', 'cp /ram/c.hm /ram/d.hm', 'echo language khuzdul=10 >> /ram/d.hm', 'dq0 -p t -s 1 -c /ram/d.hm -T -d /ram/h -R 7 -S /ram/s.sc', 'open the door', 'south', 'wait', 'persuade the dwarf', 'quit',
        'mkdir /ram/e', "echo 'faction | Hold | khuzeld | 0-0 | 96 | - | 0 | w | -' > /ram/e/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/e -R 7 -S /ram/s.sc', 'open the door', 'south', 'wait', 'persuade the dwarf elder', 'quit',
        'hmchar -q -s 5 -u feudal -c scribe -o /ram/s.hm', 'cat /ram/s.hm', 'grep language /ram/c.hm', 'echo $status'] })) },
      start(m) {
        const put = (g, line) => [{ g, line, n: 1, name: line.slice(13), set: { attitude: 3, doing: 0 }, beside: 4 }];
        dq0Scene(m, [...put(1, 'persuade the dwarf'), ...put(2, 'persuade the dwarf'), ...put(3, 'persuade the dwarf elder')]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], C = require('../../../sim/lib/dq0meaning.js').constants().names;
        const sw = (t, want, what) => { if (!t || !dq0Rep(t, 'sway', want).length) f.push(what + ' (' + JSON.stringify(t && t.replies.filter(r => r.req === 'sway')) + ')'); };
        const d = T.filter(t => t.line === 'persuade the dwarf');
        sw(d[0], { result: 'SR_TONGUE' }, 'the dwarf reached with no khuzdul');
        sw(T.of('persuade the dwarf elder'), { result: 'SR_OK' }, 'its elder not reached in common');
        sw(d[1], { result: 'SR_OK' }, 'the dwarf not reached in khuzdul');
        const test = d[1] && dq0Rep(d[1], 'dslot', { kind: C.RS_TEST })[0];
        this.notes = [JSON.stringify(test && { steps: test.steps })];
        const sc = out.slice(out.indexOf('cat /ram/s.hm'));
        if (!/\nlanguage (khuzdul|sindarin)=\d+/.test(sc)) f.push('a feudal Scribe with no language line: ' + sc.slice(0, 600));
        return f;
      },
      sdcard: true
    },
    // ---- (Its end)
    {
      name: 'dq0trade', outfit: true, near: ['denizens', 'money'], what: "Dingus Quest Zero's market and entertaining (2026-11-02: DENIZENS.md's trading, HOUSE.md's the market, for review), by the denizens' sway, the things changing hands and the words: a hand-written theme's bandits (seed 1, dq0kit's), one put beside the character, friendly, at each command (dq0Poke's); given 8 pennies without a \"to\" (\"give the bandit 8 pennies\": the one given it the line's first word, SW_GIVE, taken); what it would give for the flask of oil asked (SW_OFFER, half its worth friendly: 'would give you 1 penny'); the oil sold to it (SW_SELL: it takes the oil, FT_GOT, and hands over its pennies, FT_GAVE, by the character's own way with things, the world's wfetch.c; the character's flask gone, its pennies one more); its price asked back (SW_PRICE: 'asks'), named Bob and bought back from it (SW_BUY, by the name given: the character's pennies taken, the oil handed over, the flask the character's again); a rope sold it and haggled for (SW_HAGGLE, the book's Rhetoric in its tongue, Rhetoric 95: its price under the price asked, that price paid); sung to, indifferent (SW_ENTERTAIN: Singing, the referee's)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml rhe 95 >> /ram/c.hm', 'echo wealth 6000 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k rope -S /ram/s.sc',
        'inventory', 'give the bandit 8 pennies', 'what will you give for the oil', 'sell the oil to the bandit', 'inventory', 'how much for the oil',
        'name the bandit bob', 'buy the oil from bob', 'inventory', 'drop the rope', 'sell the rope to bob', 'how much for the rope', 'inventory', 'haggle for the rope', 'inventory', 'sing to bob', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, ['inventory', 'give the bandit 8 pennies', 'what will you give for the oil', 'sell the oil to the bandit', 'how much for the oil', 'name the bandit bob'].map(line => ({ g: 1, line, n: 1, name: 'bandit', set: { attitude: 4, doing: 0 }, beside: 4 }))
          .concat(['buy the oil from bob', 'sell the rope to bob', 'how much for the rope', 'haggle for the rope', 'sing to bob'].map(line => ({ g: 1, line, n: 1, name: 'Bob', set: { attitude: line[1] === 'i' ? 3 : 4, doing: 0 }, beside: 4 }))));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const sw = (line, want, what) => { const t = T.of(line); if (!t || !dq0Rep(t, 'sway', want).length) f.push(what + ' (' + JSON.stringify(t && t.replies.filter(r => r.req === 'sway')) + ')'); return t; };
        const oil = st => dq0Carried(st).filter(i => i.name === 'flask of oil').length;
        const pennies = st => dq0Carried(st).filter(i => i.name === 'pennies').reduce((n, i) => n + i.qty, 0);
        const i0 = T.of('inventory');
        if (!i0 || !i0.after || !oil(i0.after)) return ['no flask of oil to trade at the start'];
        const g = sw('give the bandit 8 pennies', { kind: 'SW_GIVE', result: 'SR_OK' }, 'the pennies not given without "to"');
        if (g && !dq0Ev(g, 'E_TASK', { te: 'TE_FETCHED', ft: 'FT_GOT' }).length) f.push('the gift not taken (' + dq0Evs(g) + ')');
        sw('what will you give for the oil', { kind: 'SW_OFFER', result: 'SR_OK' }, 'no offer for the oil');
        if (!/The bandit would give you \d+ penn(y|ies) for your oil\./.test(s)) f.push('the offer not told');
        const se = sw('sell the oil to the bandit', { kind: 'SW_SELL', result: 'SR_OK' }, 'the oil not sold');
        const evs = t => T.filter(u => u.k >= t.k && u.k <= t.k + 1).flatMap(u => dq0Ev(u, 'E_TASK', { te: 'TE_FETCHED' })).map(e => e.ft);
        if (se && (!evs(se).includes('FT_GOT') || !evs(se).includes('FT_GAVE'))) f.push('the sale\'s things not changing hands: ' + JSON.stringify(evs(se)));
        const i1 = T.after(se, 'inventory');
        if (!i1 || !i1.after || oil(i1.after) !== oil(i0.after) - 1) f.push('the oil still the character\'s after the sale');
        if (i1 && i1.after && pennies(i1.after) <= pennies(i0.after) - 8) f.push('no pennies for the oil: ' + pennies(i1.after));
        sw('how much for the oil', { kind: 'SW_PRICE', result: 'SR_OK' }, 'no price asked for the oil');
        if (!/The bandit asks \d+ penn(y|ies) for the oil\./.test(s)) f.push('the price not told');
        const b = sw('buy the oil from bob', { kind: 'SW_BUY', result: 'SR_OK' }, 'the oil not bought back (from one by the name it was given)');
        if (b && (!evs(b).includes('FT_GOT') || !evs(b).includes('FT_GAVE'))) f.push('the purchase\'s things not changing hands: ' + JSON.stringify(evs(b)));
        const i2 = T.after(b, 'inventory');
        if (!i2 || !i2.after || oil(i2.after) !== oil(i0.after)) f.push('the oil not the character\'s again');
        // (A haggle's price, Rhetoric 95: the rope sold it, its price asked, haggled down; the money paid the price agreed)
        sw('sell the rope to bob', { kind: 'SW_SELL', result: 'SR_OK' }, 'the rope not sold');
        const h = sw('haggle for the rope', { kind: 'SW_HAGGLE', result: 'SR_OK', grade: v => v > 0 }, 'not haggled down');
        const ask = /Bob asks (\d+) penn(?:y|ies) for the rope\./.exec(s), got = /Bob (?:gives way|gives a little), and agrees: (\d+) penn(?:y|ies) for the rope\./.exec(s);
        if (!ask || !got || +got[1] >= +ask[1]) f.push('the haggled price not under the price asked: ' + (ask && ask[1]) + ', ' + (got && got[1]));
        const before = T.after(T.of('how much for the rope'), 'inventory'), after = T.after(h, 'inventory');
        if (got && before && after && before.after && after.after && pennies(before.after) - pennies(after.after) !== +got[1])
          f.push('not the haggled price paid: ' + pennies(before.after) + ' then ' + pennies(after.after) + ', agreed ' + got[1]);
        if (after && after.after && !dq0Carried(after.after).some(i => i.name === 'rope')) f.push('the rope not bought back');
        sw('sing to bob', { kind: 'SW_ENTERTAIN', result: 'SR_OK' }, 'not sung to');   // (Indifferent: the referee's Singing tested)
        if (!/You sing for Bob\. (They're delighted|They're pleased|They're bored|They aren't moved)/.test(s)) f.push('the song not told');
        if (f.length) f.push(s.slice(s.indexOf('> buy the oil'), s.indexOf('> buy the oil') + 2500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0allydoor', outfit: true, near: ['denizens'], what: "Dingus Quest Zero's ally's errand at a door (2026-11-02: DENIZENS.md's allies, its errands by the one path; the parser's order words open, close, unlock): seed 1's bandits (dq0denportal's), the character opens the door ahead and shuts it again; a bandit put beyond it (dq0DoorScene's) and made the character's ally (dq0Poke's, the same line); \"ask the bandit to open the door\" (SW_ORDER, OR_OPEN, the door the character's own command finds: its square) and waited for: the ally opens it by the character's code for every actor (its mind's I_PORTAL, the world's wport.c: E_DEN DE_OPENS at the door's square, 'The bandit opens the door.')",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'open the door', 'close the door', 'look', 'ask the bandit to open the door', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, name: 'bandit', set: { attitude: 5, doing: 0 } }, { g: 1, line: 'ask the bandit to open the door', n: 1, name: 'bandit', set: { attitude: 5 } }]);
        dq0DoorScene(m, 'look', 'bandit');
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), DE_OPENS = 27;
        if (!m.door) return ['the scene not set: no bandit near, or no door ahead with a floor beyond it: ' + JSON.stringify(m.doorWhy)];
        const a = T.of('ask the bandit to open the door');
        if (!a || !dq0Rep(a, 'sway', { kind: 'SW_ORDER', order: 'OR_OPEN', result: 'SR_OK' }).length) f.push('the ally not asked to open the door (' + JSON.stringify(a && a.replies.filter(r => r.req === 'sway')) + ')');
        if (!/The bandit goes to the door\./.test(s)) f.push('its going not told: "The bandit goes to the door."');
        const opens = T.filter(t => a && t.k >= a.k).flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.door.k + 1 && e.a === DE_OPENS);
        if (!opens.length) f.push('the ally never opened the door (E_DEN DE_OPENS)');
        for (const e of opens) if (e.w1 !== m.door.door[0] || e.w2 !== m.door.door[1]) f.push('a door opened not the one asked: ' + [e.w1, e.w2]);
        if (!/The bandit opens the door\./.test(s)) f.push('its opening not seen: "The bandit opens the door."');
        if (f.length) f.push(s.slice(s.indexOf('> look'), s.indexOf('> look') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0parbar', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's one path for every actor, closures (ONE-PATH.md's stage 2, 2026-11-21: the act audit's G-5, a denizen's bar): a door barred and its bar lifted out by the character and by an ally, by the same code (wdoor.c's PA_BAR and PA_LIFT, the act table's bar and lift rows: their time, twice the actor's step, and their noise, quiet, at the door).  dq0allydoor's scene (seed 1's bandits, the door ahead opened and shut, a bandit put beyond it and made the character's ally, asked to wait), the door given brackets and barred from the character's side (its portal record's state, bar side and features poked as 'look' is typed), the character's bar (dq0 -k bar) made wide enough to span it: 'unbar the door' (PO_LIFTED: E_NOISE by the lift row at the door), 'bar the door' (PO_REBARRED: by the bar row); then the door unbarred again, its bar side the bandit's and the bar lying at the bandit's feet (poked as the first wait's typed): 'ask the bandit to bar the door' (OR_BAR, its errand's PA_BAR: the widest bar in its reach, wport.c's den_bar, the character's own rule) bars it (E_DEN DE_BARS at the door by the bar row: 'The bandit sets the bar in its brackets: the door is barred.', or a bar dropping; played as a bar, as the character's is); a denizen could set no bar before (wk.link was its picking)",
      init: 't_rc', cycles: 3000e6,
      machine: { ymLog: true, input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k bar -S /ram/s.sc',
        'open the door', 'close the door', 'look', 'ask the bandit to wait', 'unbar the door', 'bar the door', 'wait', 'ask the bandit to bar the door', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, item: { bar: { size: 15 } } }]);
        dq0ParScene(m, ['ask the bandit to wait', 'ask the bandit to bar the door'], (h, o, L, K) => {
          const f = h[L.pcFace], u16 = q => h[q] | h[q + 1] << 8;
          const x = u16(L.pcX) + [0, 1, 1, 1, 0, -1, -1, -1][f], y = u16(L.pcY) + [-1, -1, 0, 1, 1, 1, 0, -1][f];
          const live = !(f & 1) && dq0PortalAt(m, h[L.pcSn], x, y);   // (Its state, 3: barred; its bar's side, 7: the character's; its features, 12: brackets)
          if (live) { live.b[live.o + 3] = (live.b[live.o + 3] & ~1) | 4; live.b[live.o + 7] = ((f >> 1) + 2) & 3; live.b[live.o + 12] |= 4; m.par = { face: f, door: [x, y], sn: h[L.pcSn] }; }
        });
        dq0OnLine(m, 'wait', (h, L, K) => {   // (The door unbarred, its bar side the bandit's, the bar the character set in it at the bandit's feet)
          if (!m.par || !m.door || m.par.bar !== undefined) return;
          const live = dq0PortalAt(m, m.par.sn, ...m.par.door), per = Math.floor(0x2000 / L.itemSize);
          if (live) { live.b[live.o + 3] &= ~5; live.b[live.o + 7] = m.par.face >> 1; }
          for (let i = 1; i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), q = (i % per) * L.itemSize;
            if (b && b[q + L.iKind] === K.KI_BAR && b[q + L.iWhere] === K.IW_PORTAL) {
              b[q + L.iWhere] = K.IW_SQUARE; b[q + L.iSn] = m.par.sn;
              b[q + L.iX] = m.door.beyond[0] & 255; b[q + L.iX + 1] = m.door.beyond[0] >> 8; b[q + L.iY] = m.door.beyond[1] & 255; b[q + L.iY + 1] = m.door.beyond[1] >> 8;
              m.par.bar = i;
              break;
            }
          }
        });
        dq0Played(m);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), A = dq0Acts();
        if (!m.door || !m.par) return ['the scene not set: no bandit near, or no door ahead with a floor beyond it: ' + JSON.stringify(m.doorWhy)];
        const [dx, dy] = m.door.door;
        for (const [line, r, po] of [['unbar the door', A.lift, 'PO_LIFTED'], ['bar the door', A.bar, 'PO_REBARRED']]) {
          const t = T.of(line);
          if (!dq0Ev(t, 'E_PORTAL', { po }).length) { f.push('"' + line + '" not ' + po + ': ' + dq0Evs(t)); continue; }
          const me = dq0Ev(t, 'E_NOISE', { a: r.loud, b: r.nz });
          if (!me.length) f.push('"' + line + '" made no noise by its row (E_NOISE ' + r.loud + ', ' + r.nz + '): ' + dq0Evs(t));
          else if (me[0].w1 !== dx || me[0].w2 !== dy) f.push('"' + line + '"\'s noise not at the door: ' + [me[0].w1, me[0].w2]);
        }
        if (m.par.bar === undefined) f.push('the character\'s bar not found set in the door\'s brackets as the first wait was typed');
        const a = T.of('ask the bandit to bar the door');
        if (a.k < 0 || !dq0Rep(a, 'sway', { kind: 'SW_ORDER', order: 'OR_BAR', result: 'SR_OK' }).length) f.push('the ally not asked to bar the door (' + JSON.stringify(a.replies.filter(r => r.req === 'sway')) + ')');
        const dens = T.filter(t => a.k >= 0 && t.k >= a.k).flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.door.k + 1);
        const bars = dens.filter(e => e.a === A.bar.de);
        if (!bars.length) f.push('the ally never barred the door (E_DEN DE_BARS): ' + JSON.stringify(dens.map(e => [e.a, e.b, e.w1, e.w2])));
        for (const e of bars) {
          if (e.b !== A.bar.loud) f.push('the ally\'s bar not by its row: loud ' + e.b + ' not ' + A.bar.loud);
          if (e.w1 !== dx || e.w2 !== dy) f.push('the ally barred a door not the one asked: ' + [e.w1, e.w2]);
        }
        if (bars.length && !/The bandit sets the bar in its brackets: the door is barred\.|a bar dropping/.test(s)) f.push('its barring neither seen nor heard');
        if (bars.length && !m.played.some(p => p.who === m.door.k + 1 && p.snd === WSND.bar)) f.push('its barring not played as a bar (the world\'s *bar), as the character\'s is: ' + JSON.stringify(m.played.filter(p => p.who)));
        this.notes = ['the ally after its order: ' + dens.map(e => e.a).join(' ')];
        if (f.length) f.push(s.slice(s.indexOf('> unbar the door'), s.indexOf('> unbar the door') + 1800));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0parpick', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's one path for every actor, closures (ONE-PATH.md's stage 2, 2026-11-21: the act audit's B4, a lock picked only with lockpicks, every actor's): a door's lock picked by the character and by an ally by the same code (wdoor.c's PA_PICK: a minute's timed work, the act table's picking row its time and noise, quiet, at the door; the referee's P_PICKLOCK at the lock's steps): dq0allydoor's scene (seed 1's bandits, the character with lockpicks, dq0 -k), the door locked (its portal record's state and lock poked as 'look' is typed), the bandit asked to wait; 'pick the lock' (E_NOISE by the picking row at the door, RS_TEST P_PICKLOCK, actor 0); the door locked again (as the first wait's typed); 'ask the bandit to pick the door' (OR_PICK, its errand's PA_PICK, begun and done, with its own lockpicks, a bandit's purse's: outfits.txt) (E_DEN DE_PICKING by the row, RS_TEST P_PICKLOCK of its own sheet at the same steps); then every denizen's lockpicks taken from it and the door locked again (as the third wait's typed): asked again, it can't (PO_NO_PICKS, no test): a robber picked with none before",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k lockpicks -S /ram/s.sc',
        'open the door', 'close the door', 'look', 'ask the bandit to wait', 'pick the lock', 'wait', 'ask the bandit to pick the door', 'wait 2 minutes', 'wait',
        'ask the bandit to pick the door', 'wait 2 minutes', 'quit', 'echo $status'] })) },
      start(m) {
        dq0ParScene(m, ['ask the bandit to wait', 'ask the bandit to pick the door'], (h, o, L, K) => {
          const f = h[L.pcFace], u16 = q => h[q] | h[q + 1] << 8;
          const x = u16(L.pcX) + [0, 1, 1, 1, 0, -1, -1, -1][f], y = u16(L.pcY) + [-1, -1, 0, 1, 1, 1, 0, -1][f];
          const live = !(f & 1) && dq0PortalAt(m, h[L.pcSn], x, y);   // (Its state, 3: locked; its lock, 4: three steps worse)
          if (live) { live.b[live.o + 3] = (live.b[live.o + 3] & ~1) | 2; live.b[live.o + 4] = 3; m.par = { door: [x, y], sn: h[L.pcSn], waits: 0 }; }
        });
        dq0OnLine(m, 'wait', (h, L, K) => {   // (Each wait: the door locked again; the second, every denizen's lockpicks a rope)
          if (!m.par) return;
          const live = dq0PortalAt(m, m.par.sn, ...m.par.door);
          if (live) live.b[live.o + 3] = (live.b[live.o + 3] & ~1) | 2;
          if (++m.par.waits === 2) {
            const per = Math.floor(0x2000 / L.itemSize);
            m.par.taken = 0;
            for (let i = 1; i < L.nitems; i++) {
              const b = m.dq0.bank(L.item0 + Math.floor(i / per)), q = (i % per) * L.itemSize;
              if (b && b[q + L.iKind] === K.KI_LOCKPICKS && b[q + L.iWhere] !== K.IW_ACTOR && b[q + L.iWhere] !== K.IW_SQUARE) { b[q + L.iKind] = K.KI_ROPE; m.par.taken++; }
            }
          }
        });
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), A = dq0Acts(), { K } = m.dq0.layout();
        if (!m.door || !m.par) return ['the scene not set: no bandit near, or no door ahead with a floor beyond it: ' + JSON.stringify(m.doorWhy)];
        const [dx, dy] = m.door.door, k = m.door.k + 1;
        const p = T.of('pick the lock'), me = dq0Ev(p, 'E_NOISE', { a: A.picking.loud, b: A.picking.nz });
        if (!me.length) f.push('the character\'s picking made no noise by its row (E_NOISE ' + A.picking.loud + ', ' + A.picking.nz + '): ' + dq0Evs(p));
        else if (me[0].w1 !== dx || me[0].w2 !== dy) f.push('the character\'s picking not heard at the door: ' + [me[0].w1, me[0].w2]);
        const tests = T.flatMap(t => t.replies.map(q => Object.assign({ line: t.k }, q))).filter(q => q.kind === K.RS_TEST && q.a === A.picking.p);
        const a1 = T.of('ask the bandit to pick the door'), a2 = a1.k < 0 ? a1 : T.of('ask the bandit to pick the door', a1.k + 1);
        const mine = tests.filter(q => q.actor === 0), theirs = tests.filter(q => q.actor === k && q.line >= a1.k && (a2.k < 0 || q.line < a2.k));
        const after = tests.filter(q => q.actor === k && a2.k >= 0 && q.line >= a2.k);
        if (!mine.length) f.push('the character\'s picking not tested (RS_TEST P_PICKLOCK, actor 0)');
        if (a1.k < 0 || !dq0Rep(a1, 'sway', { kind: 'SW_ORDER', order: 'OR_PICK', result: 'SR_OK' }).length) f.push('the ally not asked to pick the lock (' + JSON.stringify(a1.replies.filter(r => r.req === 'sway')) + ')');
        const dens = T.filter(t => a1.k >= 0 && t.k >= a1.k && (a2.k < 0 || t.k < a2.k)).flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === k);
        const picking = dens.filter(e => e.a === A.picking.de);
        if (!picking.length) f.push('the ally never picked at the lock (E_DEN DE_PICKING): ' + JSON.stringify(dens.map(e => [e.a, e.b])));
        for (const e of picking) {
          if (e.b !== A.picking.loud) f.push('the ally\'s picking not by its row: loud ' + e.b + ' not ' + A.picking.loud);
          if (e.w1 !== dx || e.w2 !== dy) f.push('the ally picked at a door not the one asked: ' + [e.w1, e.w2]);
        }
        if (!theirs.length) f.push('the ally\'s picking not tested by its own sheet (RS_TEST P_PICKLOCK, its actor)');
        if (mine.length && theirs.length && mine[0].steps !== theirs[0].steps) f.push('the same lock picked at different steps: the character\'s ' + mine[0].steps + ', the ally\'s ' + theirs[0].steps);
        if (!m.par.taken) f.push('no denizen\'s lockpicks found to take as the second wait was typed');
        if (after.length) f.push('the ally picked the lock with no lockpicks (RS_TEST P_PICKLOCK after they were taken)');
        if (a2.k >= 0 && T.filter(t => t.k >= a2.k).flatMap(t => t.dens).some(e => e.kind === 'E_DEN' && e.who === k && e.a === A.picking.de)) f.push('the ally set to picking with no lockpicks (E_DEN DE_PICKING)');
        this.notes = ['steps ' + (mine[0] && mine[0].steps) + ' ' + (theirs[0] && theirs[0].steps) + '; picked: the character ' + (mine.length > 0 && mine[mine.length - 1].grade > 0) + ', the ally ' + (theirs.length > 0 && theirs[theirs.length - 1].grade > 0)];
        if (f.length) f.push(s.slice(s.indexOf('> pick the lock'), s.indexOf('> pick the lock') + 2000));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0parslam', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's one path for every actor, closures (ONE-PATH.md's stage 2, 2026-11-21: the act audit's G-20, the character's slam): 'slam the door' (a new verb: PA_SLAM, wdoor.c's shut, every actor's) slams the door ahead shut by the act table's slam row, as a fleeing denizen's is (dq0denportal's bandit: PA_SLAM its mind's now, its noise the same row's): PO_SLAMMED, 'You slam the door shut.' (the piece @slam, DEN.slams' too), heard by the minds (E_NOISE by the row, louder than a shut, at the door) and played as a door shutting (the world's *shut); 'slam the case' shuts the case as 'close' does",
      init: 't_rc', cycles: 1500e6,
      machine: { ymLog: true, input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k case -S /ram/s.sc',
        'open the door', 'slam the door', 'drop the case', 'open the case', 'slam the case', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Played(m);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), A = dq0Acts(), { K } = m.dq0.layout();
        const t = T.of('slam the door'), o = dq0Ev(T.of('open the door'), 'E_NOISE', { a: A.open.loud, b: A.open.nz });
        if (!dq0Ev(t, 'E_PORTAL', { po: 'PO_SLAMMED' }).length) f.push('"slam the door" not PO_SLAMMED: ' + dq0Evs(t));
        if (!/> slam the door[^>]*You slam the door shut\./.test(s)) f.push('"slam the door" not "You slam the door shut."');
        const me = dq0Ev(t, 'E_NOISE', { a: A.slam.loud, b: A.slam.nz });
        if (!me.length) f.push('the slam made no noise by its row (E_NOISE ' + A.slam.loud + ', ' + A.slam.nz + '): ' + dq0Evs(t));
        else if (o.length && (me[0].w1 !== o[0].w1 || me[0].w2 !== o[0].w2)) f.push('the slam not heard at the door: ' + [me[0].w1, me[0].w2]);
        if (A.slam.loud <= A.shut.loud) f.push('the slam row no louder than the shut row');
        if (A.slam.de !== K.DE_SLAMS) f.push('the slam row not the fleeing denizen\'s DE_SLAMS');
        if (!m.played.some(p => p.who === 0 && p.snd === WSND.shut)) f.push('the slam not played as a door shutting (the world\'s *shut)');
        if (!/> slam the case[^>]*You close the case\./.test(s)) f.push('"slam the case" not "You close the case."');
        if (f.length) f.push(s.slice(s.indexOf('> open the door'), s.indexOf('> open the door') + 1200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0parwin', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's one path for every actor, closures (ONE-PATH.md's stage 2, 2026-11-21: the act audit's G-6, a denizen at a window): a window's shutters opened and shut by the character and by an ally by the same code (wdoor.c's, a window's shutters a door's: the act table's wopen and wshut rows, a door's numbers, told as shutters), wport.c taking a window now (its shutters, its bars bent, its pane worked out; climbed through not yet, the climb's): dq0sky's window (seed 1, dq0sky=1: a window on the surface to the east, given shutters, shut, as 'open the window' is typed), a hand-written theme's bandits; 'open the window', 'close the window' (E_NOISE by the rows at the window), the character stepped back from it and a bandit put before the window (dq0Poke's beside, the way the character faces) and made its ally; 'ask the bandit to open the window' (OR_OPEN, the errand's window: E_DEN DE_WOPENS at the window by the wopen row, 'The bandit opens the shutters.' or shutters opening), 'ask the bandit to close the window' (DE_WSHUTS by the wshut row)",
      init: 't_rc', cycles: 3000e6,
      machine: { ymLog: true, input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo sky 1 >> /ram/s.sc', 'echo hour 12 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'look east', 'open the window', 'close the window', 'step back', 'look', 'ask the bandit to open the window', 'wait', 'ask the bandit to close the window', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        dq0OnLine(m, 'open the window', (h, L, K) => {   // (The nearest window given shutters, shut: its portal record's parts, 8, and state, 3)
          const u16 = q => h[q] | h[q + 1] << 8, px = u16(L.pcX), py = u16(L.pcY);
          for (let r = 1; r <= 3 && !m.win; r++) for (let dy = -r; dy <= r && !m.win; dy++) for (let dx = -r; dx <= r && !m.win; dx++) {
            const p = dq0PortalAt(m, h[L.pcSn], px + dx, py + dy);
            if (p && p.b[p.o + 2] === 1) { p.b[p.o + 8] |= 2; p.b[p.o + 3] &= ~1; m.win = [px + dx, py + dy]; }
          }
        });
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, name: 'bandit', beside: 'face', set: { attitude: 5, doing: 0 } },
          ...['ask the bandit to open the window', 'ask the bandit to close the window'].map(line => ({ g: 1, line, n: 1, name: 'bandit', set: { attitude: 5 } }))]);
        dq0Played(m);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), A = dq0Acts(), { K } = m.dq0.layout();
        let win = null;
        for (const [line, r] of [['open the window', A.wopen], ['close the window', A.wshut]]) {
          const t = T.of(line), me = dq0Ev(t, 'E_NOISE', { a: r.loud, b: r.nz });
          if (!dq0Ev(t, 'E_PORTAL', { po: line[0] === 'o' ? 'PO_OPENED' : 'PO_CLOSED' }).length) f.push('"' + line + '" not done: ' + dq0Evs(t));
          if (!me.length) f.push('"' + line + '" made no noise by its row (E_NOISE ' + r.loud + ', ' + r.nz + '): ' + dq0Evs(t));
          else win = win || [me[0].w1, me[0].w2];
        }
        if (!win) return f.concat(['no window worked by the character: ' + s.slice(s.indexOf('> look southwest'), s.indexOf('> look southwest') + 1200)]);
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.a >= K.DE_OPENS);
        for (const [line, r, words] of [['ask the bandit to open the window', A.wopen, /The bandit opens the shutters\.|shutters opening/], ['ask the bandit to close the window', A.wshut, /The bandit closes the shutters\.|shutters shutting/]]) {
          const a = T.of(line);
          if (a.k < 0 || !dq0Rep(a, 'sway', { kind: 'SW_ORDER', result: 'SR_OK' }).length) f.push('"' + line + '" not taken (' + JSON.stringify(a.replies.filter(q => q.req === 'sway')) + ')');
          const e = dens.filter(e => e.a === r.de);
          if (!e.length) f.push('the ally never ' + r.name + ' (E_DEN ' + r.de + '): ' + JSON.stringify(dens.map(e => [e.a, e.b, e.w1, e.w2])));
          for (const x of e) {
            if (x.b !== r.loud) f.push('the ally\'s ' + r.name + ' not by its row: loud ' + x.b + ' not ' + r.loud);
            if (x.w1 !== win[0] || x.w2 !== win[1]) f.push('the ally worked a window not the character\'s: ' + [x.w1, x.w2] + ' not ' + win);
          }
          if (e.length && r === A.wshut && !words.test(s)) f.push('the ally\'s ' + r.name + ' neither seen nor heard as shutters');   // (Its opening,
                                                //   quiet (2), told or not by the UI's hearing rule as a door's opening is: caps.c's den_heard)
        }
        if (/The bandit opens the door\.|The bandit closes the door\./.test(s)) f.push('the ally\'s shutters told as a door\'s');
        if (f.length) f.push(s.slice(s.indexOf('> look east'), s.indexOf('> step back') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0letdens', near: ['denizens', 'parser'], what: "Dingus Quest Zero's designators, step 4 (the user's design, 2026-11-06, DECISIONS.md's: \"denizens numbered first\"): at seed 3 (-T) three giant rats in sight, each a capital before the things' (the bones'), shown in the look's sentences ((A, C) Two giant rats ... (B) A giant rat ...) and in around, one a line with where it is; a capital where a denizen's name goes (look at B: how it seems, the one named, the denizens' task told which; talk to C); two rats told apart and each attacked by its letter (the scene set: both senseless beside the character, A to its left, C to its right: attack C finishes C and not A, then attack A finishes A); a capital kept while it's remembered (B still B); each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'echo b115200 >/dev/serctl', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', ...LETDENS, 'quit',
        ...dq0Paces('dq0 -p t -s 3 -c /ram/c.hm -k pick -T', LETDENS), 'echo $status'] })) },
      start(m) {
        this.timer = dq0Timer(m);
        dq0Asks(m, [3]); dq0PlayLetters(m, 1, 3); dq0WhenFree(m); dq0FightPause(m);   // (Each line typed as the character's free; the played game's
                                                //   scene set by the test's own lines though its letters are that game's: dq0Poke's, 2026-11-20)
        const self = this;
        self.dead = {};
        // (The capital's actor: its record the UI bank's cap's id, CAP_DEN's, the actor's record the byte after its name)
        const capk = (h, L, at, c) => { const u = m.dq0.bank(L.uiBank), o = L.uCap + c * L.ucapSize, id = u[o] | u[o + 1] << 8;
          return id >= 0xFF00 && u[o + 2] === 2 ? [...Array(L.nactors).keys()].find(j => h[at(j) + L.aName] && h[at(j) + L.aDen] === (id & 255)) : undefined; };
        const wary = (h, L) => { for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName]) h[L.actor + k * L.actorSize + L.aAttitude] = 2; };
        // (Each game's scene the same: the test's own, and its lines typed again at the menacing pace and as the game is
        // played, dq0Paces', games 2 and 3)
        const scene = g => {
          const pa = { g, line: 'attack C', n: 1, beside: 0, senseless: 6000 }, pc = { g, line: 'attack C', n: 1, beside: 0, senseless: 6000 };
          const pb = { g, line: 'attack C', n: 1, senseless: 60000 };   // (B out of it, senseless where it is: no fight for the test's words)
          const at0 = { g, line: 'attack C', n: 1, call: (h, L, K, at) => { wary(h, L); pa.k = capk(h, L, at, 0); pb.k = capk(h, L, at, 1); pc.k = capk(h, L, at, 2); pa.beside = (h[L.pcFace] + 6) & 7; pc.beside = (h[L.pcFace] + 2) & 7; if (g === 1) { self.ka = pa.k; self.kc = pc.k; } } };
          return ['look', 'around', 'look at B', 'talk to C'].map(line => ({ g, line, n: 1, call: wary })).concat([at0, pa, pb, pc]);
        };
        const note = (line, n = 1) => ({ g: 1, line, n, call: (h, L, K, at, named) => { wary(h, L); self.dead[line] = [self.ka, self.kc].map(k => k === undefined ? '?' : named(k) ? 'up' : 'gone'); } });
        dq0Scene(m, [...scene(1), ...scene(2), ...scene(3), note('attack A'), note('around', 2), note('quit')]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of LETDENS) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;
          const e = o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        this.notes = LETDENS.map((l, i) => l + ': ' + seg[i].slice(0, 400)).concat(['actors A, C after each: ' + JSON.stringify(this.dead)]);
        const said = (k, re, from = 0) => { const n = LETDENS.indexOf(k, from); if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        said('look', /\(A, C\) Two giant rats are ahead to your left, 15 feet away\. \(B\) A giant rat is ahead, 10 feet away\./);
        said('look', /\(D\) some bones/);
        said('around', /^\(A\) a giant rat, ahead to your left, 15 feet away \(B\) a giant rat, ahead, 10 feet away \(C\) a giant rat, ahead to your left, 15 feet away \(D\) some bones, ahead to your right/);
        said('look at B', /^You look at the giant rat\. The giant rat seems wary of you\./);   // (Its first words at once, 2026-11-17)
        said('talk to C', /words won't reach it/);
        said('attack C', /You finish the giant rat where it lies\./);
        said('attack A', /You finish the giant rat where it lies\./);
        said('around', /\(D\) some bones, ahead to your right/, 6);   // (A thing's capital kept; the rats finished gone from it)
        if (/\((A|C)\) a giant rat/.test(seg[6])) f.push('around: a rat finished still told: ' + seg[6]);
        const d = this.dead;
        if (!d['attack A'] || d['attack A'][0] !== 'up' || d['attack A'][1] !== 'gone') f.push('attack C: not C finished and A left: ' + JSON.stringify(d));
        if (!d['around'] || d['around'][0] !== 'gone') f.push('attack A: A not finished: ' + JSON.stringify(d));
        // (Every line held, a blow too, in each typing (the test's own, the menacing pace's, the game as played: dq0Paces'):
        // an attack's first words its own, "You close with the giant rat.", not a stir's denizens told before it, 2026-11-17)
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], LETDENS.map(l => ({ line: l, want: /^attack/.test(l) ? 'You' : 'You|The|(' })));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0regard', near: ['denizens'], what: 'Dingus Quest Zero\'s gifts, reputation and an ally moving aside (slice 4.8: DENIZENS.md\'s allies and influence, HOUSE.md\'s fetching, gifts and reputation, D10), scenes set by pokes: at seed 21 (dq0fetch\'s game) a giant rat won over and named Grub, the character two squares on and turned round; Grub set in its way as it steps back (the hub\'s actor\'s square, the poke\'s): the step stopped short of it (E_NOWAY, N_DEN), Grub moving aside at once, the step taken again past it; Grub set wary of the character as a ration\'s given it (the poke\'s attitude): its attitude moved by the gift (food, to a beast, the house\'s steps for its worth); set beside the character again (an ally no more, it doesn\'t follow) and given another, its faction\'s regard raised by the two (dq0d\'s head, fatt: a step for its deeds\' points, four)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', lines: ['echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -R 117',
        'befriend the rat', 'befriend the rat', 'name the rat grub', 'move east 2 squares', 'turn around', 'move west 1 square', 'move west 1 square',
        'give 1 ration to grub', 'give 1 ration to grub', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {
        // (Its scenes: Grub put ahead of the character as the first step back's typed; set wary as the first gift's; the
        // denizens' head's factions' attitudes read as the gifts' lines and the quit are typed)
        const a = m.acia, read = a.read, self = this;
        let typed = '', seen = {};
        self.fatt = {};
        a.read = (r, t) => {
          if (r === 0 && a.rdrf && a.rx === 13 && m.dq0) {
            seen[typed] = (seen[typed] || 0) + 1;
            const { L } = m.dq0.layout(), h = m.dq0.bank(0), at = k => L.actor + k * L.actorSize;
            const grub = h && [...Array(L.nactors).keys()].find(k => String.fromCharCode(...h.subarray(at(k) + L.aName, at(k) + L.aName + 4)) === 'Grub');
            if (h && grub !== undefined) dq0SceneNow(m, (h, L) => {   // (A scene: the hub's copy; a line writing none, none)
              const o = at(grub);
              if (typed === 'move west 1 square' && seen[typed] === 1) {
                const x = h[L.pcX] | h[L.pcX + 1] << 8, y = h[L.pcY] | h[L.pcY + 1] << 8;
                h[o + L.aX] = (x - 1) & 0xFF; h[o + L.aX + 1] = (x - 1) >> 8; h[o + L.aY] = y & 0xFF; h[o + L.aY + 1] = y >> 8;
                self.placed = { x: x - 1, y };
              }
              if (typed === 'give 1 ration to grub' && seen[typed] === 1) h[o + L.aAttitude] = 2;   // (AT_WARY)
              if (typed === 'give 1 ration to grub' && seen[typed] === 2) {   // (Beside it again, where the character was:
                const x = h[L.pcX] | h[L.pcX + 1] << 8;                         //   an ally no more, it doesn't follow)
                h[o + L.aX] = (x + 1) & 0xFF; h[o + L.aX + 1] = (x + 1) >> 8;
                h[o + L.aY] = h[L.pcY]; h[o + L.aY + 1] = h[L.pcY + 1];
              }
              self.fac = h[o + L.aFaction];
            }, null, 'Grub\'s "' + typed + '"', true);
            if (typed === 'give 1 ration to grub' || typed === 'quit') {
              const tk = h[12 + 2], lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
              const hd = parseInt(/^al ([0-9A-F]+) \._hd$/m.exec(lbl)[1], 16);
              self.fatt[typed + seen[typed]] = [...Array(16).keys()].map(f => m.taskRam[tk][hd + 4 + f]);
            }
          }
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) typed = ''; else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      check(m) {
        const T = dq0Turns(m), f = [];
        if (!T.filter(t => t.line === 'befriend the rat').some(t => dq0Rep(t, 'sway', { now: 'AT_ALLIED' }).length)) f.push('the rat not won over');
        const w = T.filter(t => t.line === 'move west 1 square');
        if (!this.placed) f.push('Grub not set in the way');
        dq0Need(f, w[0], 'E_NOWAY', {}, 'the step back not stopped short of Grub');
        const p0 = w[0] && w[0].after && w[0].after.pc, p1 = w[1] && w[1].after && w[1].after.pc;
        if (!p0 || !p1 || p1.x !== p0.x - 1) f.push('the step back not taken past Grub, moved aside (' + JSON.stringify([p0 && [p0.x, p0.y], p1 && [p1.x, p1.y]]) + ')');
        const gs = T.filter(t => t.line === 'give 1 ration to grub');
        const r0 = gs[0] ? dq0Rep(gs[0], 'sway', { kind: 'SW_GIVE' })[0] : null;
        if (!r0 || r0.was !== 'AT_WARY' || r0.now === 'AT_WARY' || r0.now === 'AT_UNFRIENDLY') f.push('the wary rat\'s attitude not moved by the gift (' + JSON.stringify(r0) + ')');
        const b = this.fatt['give 1 ration to grub1'], e = this.fatt['quit1'], k = (this.fac || 0) & 7;
        if (!b || !e || e[k] !== b[k] + 1) f.push('its faction\'s regard not raised a step by the two gifts (' + JSON.stringify([k, b, e, gs.map(t => t.replies.filter(r => r.req === 'sway'))]) + ')');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denhands', near: ['denizens'], what: "Dingus Quest Zero's denizens and things by the character's rules, the rest of Phase 3C's one path (2026-11-04: DUNGEON.md's one path, DENIZENS.md's things handled, HOUSE.md's denizens handling things, for review), by the world's beats: dq0denfood's garrison (seed 2), rations the character's let fall, a soldier beside them, friendly, made hungry (dq0hungry's way): it takes them up (WB_PICKS), then eats them at its next look (WB_EATS: five minutes, its mind fed: the hub's dig_t TW_ATE, it wants no more); the next game, one takes them up and is fed, a fellow of its faction put beside it, made hungry: handed over (WB_HANDS_TO, the one hand of a thing, the receiver's take rule, the give row: it eats them at its own next look, hungry still); the third, one that's taken them up made to flee (the minds' fled): it lets them fall (WB_DROP), no time, lying where it stood",
      init: 't_rc', cycles: 4000e6,
      machine: { input: (() => {
        const game = d => ['dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/' + d, 'drop the rations', ...Array(12).fill('wait 1 minute'), 'quit'];
        return dq0Typed(dq0Game({ lines: ['echo hour 128 >> /ram/s.sc', 'mkdir /ram/g', 'mkdir /ram/h', 'mkdir /ram/i', "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt', 'cp /ram/g/theme.txt /ram/i/theme.txt', ...game('g'), ...game('h'), ...game('i'), 'echo $status'] }));
      })() },
      start(m) {
        const lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        const ad = dq0Ad(lbl());
        const ramOf = () => dq0LaterRam(m);   // (The minds' own, as the scene's in place)
        const place = (h, L, K, at, R, k, x, y, att, fac) => {
          const o = R.at(h[at(k) + L.aDen]);
          h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8; h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
          h[at(k) + L.aSn] = h[L.pcSn];
          h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = att;
          h[at(k) + L.aDoing] = K.DO_ROAM;
          if (fac !== undefined) h[at(k) + L.aFaction] = fac;
        };
        const set = g => (h, L, K, at) => {
          const R = dq0Rec(m), ram = ramOf(h), px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;
          const list = [...Array(L.nactors).keys()].filter(k => h[at(k) + L.aName] && h[at(k) + L.aState]);
          const aate = ad('aate'), aloot = ad('aloot');
          const now = h[L.now + 2];
          list.forEach((k, n) => {
            if (!n) {
              place(h, L, K, at, R, k, px + 1, py, K.AT_FRIENDLY);
              ram[aate + k] = (now - 20) & 255;     // (Hungry)
              ram[aloot + k] = 0;
              m.taker = k;
            } else {
              h[at(k) + L.aAttitude] = K.AT_FRIENDLY;
              if (g === 2 && n === 1) m.fellow = k; else h[at(k) + L.aFaction] = 7;
            }
          });
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        const second = (h, L, K, at) => {             // (Game 2: it's taken them up: fed, a fellow put beside it, hungry)
          const R = dq0Rec(m), ram = ramOf(h), aate = ad('aate'), aloot = ad('aloot'), k = m.taker, j = m.fellow;
          if (j === undefined) return;
          const x = (h[at(k) + L.aX] | h[at(k) + L.aX + 1] << 8), y = (h[at(k) + L.aY] | h[at(k) + L.aY + 1] << 8) + 1;
          place(h, L, K, at, R, j, x, y, K.AT_FRIENDLY);
          const now = h[L.now + 2];
          ram[aate + k] = now;
          ram[aate + j] = (now - 20) & 255;
          ram[aloot + k] = 0;
        };
        const flee = (h, L, K, at) => {
          const ram = ramOf(h);
          ram[ad('fled') + m.taker] = 200;
          ram[ad('aloot') + m.taker] = 0;
        };
        dq0Scene(m, [
          { g: 1, line: 'wait 1 minute', n: 1, call: set(1) },
          { g: 2, line: 'wait 1 minute', n: 1, call: set(2) },
          { g: 2, line: 'wait 1 minute', n: 2, call: second },
          { g: 3, line: 'wait 1 minute', n: 1, call: set(3) },
          { g: 3, line: 'wait 1 minute', n: 2, call: flee }]);
      },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], beats = (g, w) => T.flatMap(t => [...t.events, ...t.dens]).filter(e => e.game === g && e.kind === 'E_BEAT' && e.beat === w);
        this.notes = [0, 1, 2].map(g => ['WB_PICKS', 'WB_EATS', 'WB_HANDS_TO', 'WB_DROP'].map(w => w + ' ' + beats(g, w).length).join(', '));
        if (!beats(0, 'WB_PICKS').length) f.push('the hungry soldier never took up the rations');
        if (!beats(0, 'WB_EATS').length) f.push('the hungry soldier, the rations in hand, never ate (WB_EATS)');
        if (!beats(1, 'WB_HANDS_TO').length) f.push('the fellow, hungry, was never handed the food (WB_HANDS_TO: the one hand of a thing, every actor\'s, wthings.c\'s FE_HAND, 2026-11-23; WB_HANDS before)');
        if (!beats(2, 'WB_DROP').length) f.push('the one fleeing never let what it held fall (WB_DROP)');
        return f;
      },
    },
    {
      name: 'dq0allythrow', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's ally's thing thrown by the character's own throw (2026-11-04: Phase 3C's one path, the things' rest; DENIZENS.md's things handled): dq0denportc's scene, the portcullis let down between them: a dwarf miner made the character's ally, beyond it in its sight, brought back a thing of its own (its errand's state poked: OR_FETCH, taken up); no way nearer, it throws it (I_ROB FE_THROW: things.c's s_throw_at, its name 0; E_BEAT WB_THREW or WB_FELL, seen), the thing no more its own, lying on the character's square or by a band's end on the line",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7 -S /ram/s.sc',
        'open the door', 'go through it', 'turn around', 'close the door ahead', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        const lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        const ad = dq0Ad(lbl());
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: (h, L, K, at, named) => {
          const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
          const k = [...Array(L.nactors).keys()].find(j => named(j) === 'dwarf miner');
          if (k === undefined) return;
          const x = px + 2 * dx, y = py + 2 * dy, o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), did = u16(o + L.aDid);
          const per = Math.floor(0x2000 / L.itemSize);
          let thing = 0;
          for (let i = 1; i < L.nitems && !thing; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), io = (i % per) * L.itemSize;
            if (b && b[io + L.iKind] && b[io + L.iWhere] === K.IW_DEN && (b[io + L.iOwner] | b[io + L.iHide] << 8) === did) thing = i;
          }
          if (!thing) return;
          w16(o + L.aX, x);
          w16(o + L.aY, y);
          h[o + L.aSn] = sn;
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_ALLIED;
          R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
          R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
          R.mem[q + R.SN] = sn;
          const t = h[12 + 2], ram = dq0LaterRam(m);
          ram[ad('aorder') + k] = K.OR_FETCH;
          ram[ad('afetch') + 2 * k] = thing & 255;
          ram[ad('afetch') + 2 * k + 1] = (thing >> 8) | 0x80;
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          m.thrower = { k, thing };
        } }]);
      },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out);
        if (!m.thrower) return ['the scene not set: no dwarf miner, or none of its own things found'];
        const ev = T.flatMap(t => [...t.events, ...t.dens]).filter(e => e.kind === 'E_BEAT' && (e.beat === 'WB_THREW' || e.beat === 'WB_FELL'));
        if (!ev.length) f.push('it never threw the thing over (WB_THREW, WB_FELL): ' + o.slice(o.indexOf('> wait'), o.indexOf('> wait') + 900));
        if (!/throws you their/.test(o)) f.push('no words of it: ' + o.slice(o.indexOf('> wait'), o.indexOf('> wait') + 900));
        return f;
      },
    },
    {
      name: 'dq0denfood', near: ['denizens'], what: "Dingus Quest Zero's hungry denizen taking food (HOUSE.md's denizens taking things: what it wants, a hungry one food; dq0hungry has hunger's hunting), by the world's beats: dq0daily's garrison (seed 2, soldiers: folk, no robbers), one set beside rations the character's let fall, friendly and idle (dq0Poke's): fed lately, it leaves them; made hungry (the minds' aate, dq0hungry's way), it takes them up (E_BEAT WB_PICKS, by the take rule)",
      init: 't_rc', cycles: 2000e6,
      machine: { input: (() => {
        const game = d => ['dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/' + d, 'drop the rations', ...Array(5).fill('wait 1 minute'), 'quit'];
        return dq0Typed(dq0Game({ lines: ['echo hour 128 >> /ram/s.sc', 'mkdir /ram/g', 'mkdir /ram/h', "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt', ...game('g'), ...game('h'), 'echo $status'] }));
      })() },
      start(m) {
        const set = hungry => (h, L, K, at, named) => {
          const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ad = dq0Ad(lbl), ram = dq0LaterRam(m), px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8, nowU = h[L.now + 2] | h[L.now + 3] << 8, R = dq0Rec(m);
          let n = 0;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            if (!n++) {
              const x = px + 1, y = py, o = R.at(h[at(k) + L.aDen]);
              h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8; h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
              h[at(k) + L.aSn] = h[L.pcSn];
              h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_FRIENDLY; h[at(k) + L.aDoing] = K.DO_ROAM;
              ram[ad('aate') + k] = hungry ? (nowU - 20) & 255 : nowU & 255;
              ram[ad('aloot') + k] = 0;
            } else {
              h[at(k) + L.aAttitude] = K.AT_FRIENDLY; h[at(k) + L.aFaction] = 7;
            }
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        dq0Scene(m, [1, 2].flatMap(g => [1, 2, 3, 4, 5].map(n => ({ g, line: 'wait 1 minute', n, call: n === 1 ? set(g === 2) : () => {} }))));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], picks = g => T.flatMap(t => [...t.events, ...t.dens]).filter(e => e.game === g && e.kind === 'E_BEAT' && e.beat === 'WB_PICKS');
        const rations = (m.dq0.games[0] && T.of('drop the rations').after) ? T.of('drop the rations').after.items.find(i => i.name === 'rations') : null;
        if (!rations) return ['no rations'];
        const p0 = picks(0).filter(e => e.thing === rations.kind), p1 = picks(1).filter(e => e.thing === rations.kind);
        this.notes = [p0.length + ' taken fed, ' + p1.length + ' hungry'];
        if (p0.length) f.push('taken up, fed lately: ' + JSON.stringify(p0));
        if (!p1.length) f.push('not taken up, hungry: ' + JSON.stringify(picks(1)) + dq0Story(out).split('% dq0 ')[2].slice(-900));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hoard', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's robbers' takings carried off (gear3's item 4, left; DENIZENS.md's a robber's takings, HOUSE.md's the denizens' belongings and the peoples): a hand-written theme's hostile giant rats with an orc king as their leader (seed 2: their home by the way down where the character starts, the king with them: popgen's, its own lair things made with the home since 2026-10-29, its hoard a chest beside it), the character poor and undefended with a dagger in hand: knocked senseless and robbed by the king (a robber since 2026-10-29: the tribe's spoils its hoard's), its dagger and its pennies put in the king's hoard (I_ROB's RB_HOARD: the chest marked OW_HOARD, its tribe's), found there (\"open the chest\", \"look in the chest\": unlocked, the house's), and taken back; and a tribe of brown orcs with no king (seed 1, south of the start): robbed by one, the takings hidden at its home (RB_HIDE: a brown orc keeps its things hidden, its mates not trusted), off the character's square, found by searching only (I_HIDDEN).  The scene set (since the denizens' day, 2026-10-31: brown orcs keep watches, a third asleep, a third on watch): the second game's with no day (the environment's dq0hour=128, rc's, before it: dq0d's), every orc about as before",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml dge 5 >> /ram/c.hm', 'echo ml una 5 >> /ram/c.hm',
        'echo ml dag 5 >> /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/h', "echo 'faction | Tribe | rat | 0-0 | 5 | garking | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k dagger',
        'defend none', 'hold the dagger', ...Array(16).fill('wait'), 'stand up', 'open the chest', 'open the chest', 'look in the chest', 'take the dagger from the chest', 'quit',
        "echo 'faction | Tribe | hyeka | 0-0 | 96 | - | 0 | h | -' > /ram/h/theme.txt", 'echo hour 128 >> /ram/s.sc', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k dagger -S /ram/s.sc',
        'defend none', 'hold the dagger', 'open the door', 's 3', ...Array(20).fill('wait'), 'wait 1 minute', 'wait 1 minute', 'quit', 'echo $status'] })) },   // (Two
                                                // minutes more: the robber carries its takings home, the user's C10, 2026-11-23)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], I = DQ0_I, s = dq0Story(out);
        const robs = t0 => T.filter(t => t.k > t0.k).flatMap(t => dq0Ev(t, 'E_TASK', { te: 'TE_ROBBED' }));
        const h1 = T.of('hold the dagger'), o1 = T.of('open the chest'), tk = T.of('take the dagger from the chest');
        if (!h1.after || !o1.at || !tk.after) return ['the first game not played'];
        const b1 = dq0Carried(h1.after), dag = dq0Named(b1, 'dagger')[0], pen = dq0Named(b1, 'pennies')[0];
        if (!dag || !pen) return ['no dagger or pennies to begin with'];
        const r1 = robs(h1).filter(e => T.indexOf(T.find(t => t.events.includes(e) || t.dens.includes(e))) < o1.k);
        if (r1.length !== 1) f.push('the first game: not robbed once: ' + r1.length);
        else if (!o1.at.actors.some(a => a.k === r1[0].by && a.name === 'orc king')) f.push('not robbed by the orc king: ' + JSON.stringify(o1.at.actors.map(a => [a.k, a.name])) + ' ' + r1[0].by);
        const by = new Map(o1.at.items.map(i => [i.n, i])), box = n => { const i = by.get(n); return i && i.where === 'IW_ITEM' ? by.get(i.x) : null; };
        for (const t of [dag, pen]) {
          const c = box(t.n);
          if (!c || c.name !== 'chest' || !(c.owner & 0x80)) f.push('the ' + t.name + ' not in the king\'s hoard: ' + JSON.stringify(by.get(t.n)) + ' ' + JSON.stringify(c));
        }
        const look = s.slice(s.indexOf('> look in the chest'), s.indexOf('> take the dagger from the chest'));
        const shock = /open the chest You're in shock: you can do no more than rest/.test(s);   // (Slice 4.3's: come round in shock, it handles
                                                // nothing, the book's; the hoard's then the hub's alone, above)
        if (shock && !/take the dagger from the chest You're in shock: you can do no more than rest/.test(s)) f.push('in shock, the dagger taken from the chest');
        else if (!shock && (!/dagger/.test(look) || !/penn/.test(look))) f.push('the takings not found in the chest: ' + JSON.stringify(look.slice(0, 600)));
        if (!shock && !dq0Named(dq0Carried(tk.after), 'dagger').length) f.push('the dagger not taken back from the hoard: ' + JSON.stringify(s.slice(s.indexOf('> take the dagger from the chest'), s.indexOf('> take the dagger from the chest') + 300)));
        const h2 = T.of('hold the dagger', h1.k + 1), q2 = T.of('quit', h2.k + 1);
        if (!h2.after || !q2.at) return [...f, 'the second game not played'];
        const b2 = dq0Carried(h2.after), dag2 = dq0Named(b2, 'dagger')[0], pen2 = dq0Named(b2, 'pennies')[0];
        const r2 = robs(h2);
        if (r2.length !== 1) f.push('the second game: not robbed once: ' + r2.length);
        else if (!q2.at.actors.some(a => a.k === r2[0].by && a.name === 'brown orc') && !/A brown orc|The brown orc/.test(s.slice(s.lastIndexOf('% echo \'faction | Tribe | hyeka')))) f.push('not robbed by a brown orc');
        const by2 = new Map(q2.at.items.map(i => [i.n, i]));
        for (const t of [dag2, pen2].filter(Boolean)) {
          const i = by2.get(t.n);
          if (!i || i.where !== 'IW_SQUARE' || !(i.flags & I.HIDDEN) || !i.hide || (i.x === q2.at.pc.x && i.y === q2.at.pc.y))
            f.push('the ' + t.name + ' not hidden at the robber\'s home: ' + JSON.stringify(i));
        }
        if (!dag2 || !pen2) f.push('no dagger or pennies in the second game');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0peoples', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's peoples (Phase 4: the user's answer 11, the HarnWorld Bestiary's Khuzdul, Sindarin, men and Gargun; DENIZENS.md's the peoples, HOUSE.md's for review), each in a hand-written theme's faction on the top stratum (seed 1, south of the start): dwarves wary, seen in mail and armed (\"A dwarf in mail, armed, ...\", \"Two dwarves\" (or three, as many as are told together: the dice by who asks, 2026-10-30): AG_MAIL, the plural), examined their arms and mail (outfits.txt's [role khuzwar]), deaf to the common tongue (\"doesn't speak any tongue you know\"), heard by a character who speaks khuzdul (its file's language line: the faction's tongue, factions.txt's); a tribe of brown orcs and black orcs under a king, hostile, seen armed, their own weapons made (a mankar or a mang: the catalogue's, Combat 3's (G)); elves keeping their distance (shy), a longknife at the belt and a hartbow worn; every one's sheet the referee's from its own key (RS_NEW_ACTOR answered 0: the bestiary's, beasts.c's).  The scene set (since the denizens' day, 2026-10-31: the dwarves, the brown orcs and the elves keep watches, the black orcs asleep by day): no day (the environment's dq0hour=128, rc's, before its games: dq0d's), every one about as before; and an elf put beside the character, keeping its distance, as it first waits (dq0Poke, 2026-11-01: the elves' wariness, not where a walk's beats happen to look about: the walk south, its jog told as one bend since a passage's heading, step 2 of the ways off the compass, met them at another moment)",
      init: 't_rc', cycles: 4000e6,
      // (-R 7: the dice's offset that gives the game it was written for, since the dice by who asks,
      // 2026-10-30: the same dungeon, other rolls)
      machine: { input: dq0Typed(dq0Game({ lines: ['cp /ram/c.hm /ram/k.hm', 'echo hour 128 >> /ram/s.sc', 'echo language khuzdul >> /ram/k.hm',
        'mkdir /ram/g', 'mkdir /ram/g2', 'mkdir /ram/h', 'mkdir /ram/e',
        "echo 'faction | Hold | khuzwar | 0-0 | 96 | khuzeld | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/g2/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7 -S /ram/s.sc',
        'open the door', 'south', 'wait', 'examine the dwarf', 'talk to the dwarf', 'quit',
        'dq0 -p t -s 1 -c /ram/k.hm -T -d /ram/g2 -R 7 -S /ram/s.sc', 'open the door', 'south', 'wait', 'talk to the dwarf', 'quit',
        "echo 'faction | Tribe | hyeka khanu | 0-0 | 96 | garking | 0 | h | -' > /ram/h/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -R 7 -S /ram/s.sc',
        'open the door', 'south', ...Array(8).fill('wait'), 'quit',
        "echo 'faction | Wardens | sindwar | 0-0 | 96 | - | 0 | w | -' > /ram/e/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/e -R 7 -S /ram/s.sc',
        'open the door', 'south', 'wait', 'wait', 'examine the elf', 'quit', 'echo $status'] })) },
      start(m) {                                  // (The elves' scene set, 2026-11-01: an elf put beside the character as it waits,
        dq0Scene(m, [{ g: 4, line: 'wait', n: 1, name: 'elf', beside: 2, set: { doing: m.dq0.layout().K.DO_SHY } }]);   //   keeping its distance: their
      },                                          //   wariness, not where a walk's beats happen to look)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), K = m.dq0.layout().K;
        const games = s.split('% dq0 -p t').slice(1);
        if (games.length !== 4) return ['not four games: ' + games.length];
        const [dw, dk, or, el] = games;
        if (!/A dwarf in mail, armed, is /.test(dw)) f.push('no dwarf in mail, armed, seen');
        if (!/(Two|Three|Four|Five|Six) dwarves in mail/.test(dw)) f.push('no "Two dwarves" (or more) told');
        const ex = dw.slice(dw.indexOf('> examine the dwarf'), dw.indexOf('> talk to the dwarf'));
        if (!/seems wary/.test(ex) || !/mail byrnie|scale byrnie/.test(ex) || !/axe|warhammer|battleaxe/.test(ex)) f.push('the dwarf not examined wary, in mail, armed: ' + JSON.stringify(ex.slice(0, 300)));
        if (!/The dwarf doesn't speak any tongue you know\./.test(dw)) f.push('the dwarf understood the common tongue');
        const tk = dk.slice(dk.indexOf('> talk to the dwarf'));
        if (/doesn't speak any tongue you know/.test(tk) || !/> talk to the dwarf/.test(dk)) f.push('the dwarf not heard in khuzdul: ' + JSON.stringify(tk.slice(0, 200)));
        if (!/A (brown|black) orc, armed/.test(or)) f.push('no orc seen armed');
        const items = T.filter(t => t.after).flatMap(t => t.after.items);
        if (!items.some(i => i.name === 'mankar' || i.name === 'mang')) f.push('no mankar or mang made (the orcs\x27 own): ' + [...new Set(items.map(i => i.name))].join(', '));
        if (!/An elf/.test(el) || !/keeping their distance/.test(el)) f.push('no elf keeping their distance');
        const ee = el.slice(el.indexOf('> examine the elf'));
        if (!/longknife/.test(ee) || !/hartbow/.test(ee)) f.push('the elf\'s longknife and hartbow not seen: ' + JSON.stringify(ee.slice(0, 300)));
        const na = T.flatMap(t => t.replies).filter(r => r.kind === K.RS_NEW_ACTOR);
        if (!na.length || na.some(r => r.result !== 0)) f.push('a sheet not the bestiary\'s own (RS_NEW_ACTOR): ' + na.filter(r => r.result !== 0).length + ' of ' + na.length);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hungry', near: ['denizens'], what: "Dingus Quest Zero's denizens' hunger (2026-10-31, the brief's every actor's body: HOUSE.md's the denizens' bodies, for review): seed 3's giant rats round the way down (dq0 -T, dq0denizens' scene: wary vermin, hunters by smell), fed lately as the game begins, none hostile through a minute's search; then each rat made very hungry as the second search is typed (its want, the minds' aate: the clock's high word less 20, some 36 hours), and a very hungry hunter that senses the character hunts it: hostile (its attitude in the hub), coming at it",
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T', 'search', 'search', 'quit', 'echo $status'] })) },
      start(m) {   // (The rats made very hungry as the second search is typed: dq0d's aate, each actor's, by its label)
        const a = m.acia, read = a.read;
        let typed = '', n = 0;
        m.hungered = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'search' && ++n === 2 && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L) => {   // (A scene: the minds' own, dq0LaterRam's)
                const ad = dq0Ad(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'))('aate');
                const nowU = h[L.now + 2];
                for (let k = 0; k < L.nactors; k++) {
                  const o = L.actor + k * L.actorSize;
                  let s = '';
                  for (let i = 0; h[o + L.aName + i] && i < 12; i++) s += String.fromCharCode(h[o + L.aName + i]);
                  if (s === 'giant rat') { dq0LaterRam(m)[ad + k] = nowU - 20 & 255; m.hungered.push(k); }
                }
              }, null, 'the rats made hungry', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], s1 = T.of('search'), s2 = T.of('search', s1.k + 1);
        const rats = s => s && s.actors ? s.actors.filter(x => x.name === 'giant rat') : [];
        if (!rats(s1.after).length) return ['no giant rats about the way down: ' + JSON.stringify(s1.after && s1.after.actors)];
        if (rats(s1.after).some(x => x.attitude === 'AT_HOSTILE')) f.push('a rat hostile before it hungered: ' + JSON.stringify(rats(s1.after)));
        if (!m.hungered.length) f.push('no rat made hungry');
        if (!rats(s2.after).some(x => x.attitude === 'AT_HOSTILE')) f.push('no very hungry rat hunting the character (hostile): ' + JSON.stringify(rats(s2.after)));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0robcoins', outfit: true, near: ['denizens', 'money'], what: "Dingus Quest Zero's robber takes coins of any kind (the check of HOUSE.md, 2026-11-03: its takes were pennies alone by the kind's number; crowns and farthings are the appraisal's metal; HOUSE.md's denizens taking things): a bandit, a robber by its kind, armed, put beside crowns the character's let fall: it takes them up by the take rule; farthings the same; and the character senseless (dq0PcShock's) with crowns and farthings carried and a bandit put beside it: it goes through its things and takes them as it takes its pennies (the same fault, 2026-11-03: a robbery's its pennies alone by the kind's number)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k crowns -S /ram/s.sc',
        'drop the crowns', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        'mkdir /ram/h', 'cp /ram/g/d1/theme.txt /ram/h/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k farthings -S /ram/s.sc', 'drop the farthings', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        'mkdir /ram/i', 'cp /ram/g/d1/theme.txt /ram/i/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/i -k crowns,farthings -S /ram/s.sc', 'i', 'wait', 'wait', 'i', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face' }, { g: 2, line: 'wait', n: 1, name: 'bandit', beside: 'face' },
          { g: 3, line: 'wait', n: 1, name: 'bandit', beside: 'face', engage: true }, { g: 3, line: 'wait', n: 1, call: () => dq0PcShock(m, 3000) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), s = dq0Prose(out), G = s.split('% dq0 ').slice(1), f = [];
        const picks = g => T.flatMap(t => [...t.events, ...t.dens]).filter(e => e.game === g && e.kind === 'E_BEAT' && e.beat === 'WB_PICKS');
        if (G.length !== 3) return ['not three games: ' + G.length];
        for (const [g, w] of [[0, 'crowns'], [1, 'farthings']]) {
          if (!picks(g).length) f.push('the bandit never took the ' + w + ' up');
          else if (!new RegExp(w).test(G[g].slice(G[g].indexOf('> wait')))) f.push('not told of the ' + w + ' taken');
        }
        const i1 = T.filter(t => t.line === 'i')[0], i2 = T.filter(t => t.line === 'i')[1], has = (u, n) => u && u.after ? dq0Carried(u.after).some(i => i.name === n) : null;
        if (!has(i1, 'crowns') || !has(i1, 'farthings') || !has(i1, 'pennies')) f.push('the coins not carried to begin with');
        if (!/goes through your things/.test(G[2])) f.push('not robbed');
        for (const n of ['crowns', 'farthings', 'pennies']) if (has(i2, n)) f.push('robbed, its ' + n + ' left on it');
        return f;
      },
      sdcard: true
    },
    {
      // (dq0sky=0, 2026-11-03, a scene's: with seed 2's sky the dice other, a record on its rounds finds the body, not an actor: the faction's a step worse, no alarm heard, as the rule is)
      name: 'dq0daily', near: ['denizens'], what: "Dingus Quest Zero's daily life (2026-10-31: DENIZENS.md's daily life, HOUSE.md's the denizens' day, for review; dq0d.c's part 9): the denizens' days by their kinds (kinds.txt's day and work) and the game's clock (the game begun at dusk, 18 o'clock, the house's), by the actors' records and their events: a hand-written theme's garrison of men-at-arms (seed 2, their lair south of the start; soldiers keep watches) watched for four hours from the start, half an hour at a time: a third of them on watch, walking their rounds (moving between one look and the next), a third asleep, as the clock goes on, the third that wakes at its meal (folk, at their larder: fed, the bodies' want); the watch relieved as it changes at 20 o'clock (those on watch before it asleep after, others on watch: none the same); those on watch talking as they go and the sleepers snoring (their events, DE_TALK and DE_SNORE); and the stand-in's giant rats by the way down (seed 3), nocturnal, the character gone down the ladder (out of their senses: those watching it don't sleep) and its torch put out (so its burning out breaks no long wait), the day's hour poked to 10 o'clock as it goes (dq0Hour's), their records (dq0d's, away from the character: decided lazily, by the clock) read as each look is typed: asleep at their lair's home (its square or one beside it) ten minutes on, not looked at again till they wake (due after 16 o'clock: more than five hours on); up as they wake (16 o'clock: a beast's meal its prey, the bodies' want, no meal of the day's), and about by 18 o'clock (due a minute or so on, as ever); and the garrison again, a body of theirs found on the rounds: the scene set by pokes as the first line's typed (dq0Rec's: a soldier of another lair, away, its record made dead in the near lair's home, as killed: its faction's body not found yet, dq0d's unf), the alarm raised as one on watch comes by it (DE_ALARM), once, its faction's attitude to the character a step worse (unfriendly to hostile: the head's fatt)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/s',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/s/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/s -S /ram/s.sc', ...Array(8).fill('wait 30 minutes'), 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -S /ram/s.sc', 'd', 'douse the torch', 'wait 10 minutes', 'look', 'wait 6 hours 30 minutes', 'look', 'wait 2 hours', 'look', 'quit',
        'mkdir /ram/t', 'cp /ram/s/d1/theme.txt /ram/t/theme.txt', 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/t -S /ram/s.sc', 'wait 1 minute', ...Array(4).fill('wait 30 minutes'), 'quit', 'echo $status']) },
      start(m) {
        dq0Hour(m, [{ g: 2, line: 'd', n: 1, hour: 10 }]);   // (The rats' game at 10 o'clock: asleep by day)
        const a = m.acia, read = a.read;                      // (The rats' records as each look's typed: dq0Rec's)
        let typed = '';
        m.rats = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 1 minute' && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, () => {   // (Game 3: a body of the garrison's in its near lair's home: a scene)
                const R = dq0Rec(m), u16 = o => R.mem[o] | R.mem[o + 1] << 8, w16 = (o, v) => { R.mem[o] = v & 255; R.mem[o + 1] = v >> 8; };
                const MANARMS = 28, at = [...Array(224).keys()].map(i => R.at(i)).filter(o => R.mem[o] === MANARMS);
                const near = at.find(o => R.mem[o + R.FLAGS] & 2), far = at.find(o => !(R.mem[o + R.FLAGS] & 6) && u16(o + 21) !== (near ? u16(near + 21) : -1));
                if (near && far) {
                  R.mem[far + R.FLAGS] = 4 | 8;               // (D_DEAD, D_MET: killed as an actor)
                  w16(far + 8, u16(near + 21));
                  w16(far + 10, u16(near + 23));
                  R.mem[far + R.SN] = R.mem[near + R.SN];
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  dq0LaterRam(m)[parseInt(/^al ([0-9A-F]+) \._unf$/m.exec(lbl)[1], 16) + (R.mem[far + 25] & 7)] = 1;
                  m.body = { fac: R.mem[far + 25] };
                }
              }, null, 'a body of the garrison\'s', true);
              if (typed === 'wait 30 minutes' && m.dq0 && m.body) {   // (The faction's attitude to the character, the head's fatt)
                const h = m.dq0.bank(0), lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                (m.body.fatt = m.body.fatt || []).push(m.taskRam[h[12 + 2]][parseInt(/^al ([0-9A-F]+) \._hd$/m.exec(lbl)[1], 16) + 4 + (m.body.fac & 7)]);
              }
              if (typed === 'look' && m.dq0) {
                const R = dq0Rec(m), h = m.dq0.bank(0), u16 = o => R.mem[o] | R.mem[o + 1] << 8, recs = [];
                for (let i = 0; i < 224; i++) {
                  const o = R.at(i);
                  if (R.mem[o] === 1 && (R.mem[o + R.FLAGS] & 3) === 1 && R.mem[o + R.SN] === 0)   // (A giant rat, out, not an actor)
                    recs.push({ id: u16(o + 4), x: u16(o + 8), y: u16(o + 10), hx: u16(o + 21), hy: u16(o + 23), ready: u16(o + 12) + u16(o + 14) * 65536 });
                }
                m.rats.push(recs);
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], waits = T.filter(t => t.line === 'wait 30 minutes').slice(0, 8);
        const men = s => s ? s.actors.filter(a => a.state && a.name === 'soldier') : [], on = (s, d) => men(s).filter(a => a.doing === d);
        const show = s => JSON.stringify(men(s).map(a => [a.did, a.doing, a.x, a.y]));
        if (waits.length !== 8 || !waits.every(t => t.after && men(t.after).length >= 3)) f.push('the garrison not about the character as it waits: ' + waits.map(t => show(t.after)).join(' '));
        else {
          const both = waits.filter(t => on(t.after, 'DO_GUARD').length && on(t.after, 'DO_SLEEP').length);   // (One on its way to bed, now and then)
          if (both.length < 6) f.push('not some on watch and some asleep, at most looks: ' + waits.map(t => show(t.after)).join(' '));
          const walked = waits.slice(1).some((t, i) => on(t.after, 'DO_GUARD').some(a => { const b = men(waits[i].after).find(c => c.did === a.did && c.doing === 'DO_GUARD'); return b && (b.x !== a.x || b.y !== a.y); }));
          if (!walked) f.push('those on watch not walking their rounds: ' + waits.map(t => show(t.after)).join(' '));
          const g1 = new Set([0, 1].flatMap(i => on(waits[i].after, 'DO_GUARD').map(a => a.did))), g2 = new Set([5, 6].flatMap(i => on(waits[i].after, 'DO_GUARD').map(a => a.did)));
          if (!g1.size || !g2.size || [...g2].some(d => g1.has(d))) f.push('the watch not relieved at 20 o\'clock: before ' + [...g1] + ', after ' + [...g2]);
          const slept = [5, 6].flatMap(i => men(waits[i].after).filter(a => g1.has(a.did)));   // (Asleep, or on the way to bed: never on watch)
          if (!slept.some(a => a.doing === 'DO_SLEEP') || slept.some(a => a.doing === 'DO_GUARD')) f.push('those on watch before it not asleep after: ' + show(waits[5].after) + ' ' + show(waits[6].after));
          const evs = waits.flatMap(t => t.dens.filter(e => e.kind === 'E_DEN'));
          if (!evs.some(e => e.de === 'DE_TALK')) f.push('those on watch never talking as they go');
          if (!evs.some(e => e.de === 'DE_SNORE')) f.push('the sleepers never snoring');
          if (!waits.some(t => on(t.after, 'DO_EAT').length)) f.push('none of its third at its meal as it wakes: ' + waits.map(t => show(t.after)).join(' '));
        }
        const looks = T.filter(t => t.line === 'look'), home = r => Math.max(Math.abs(r.x - r.hx), Math.abs(r.y - r.hy)) <= 1, H = 36000;
        const rs = i => (looks[i] && looks[i].at ? looks[i].at.now : '?') + ' ' + JSON.stringify((m.rats[i] || []).map(r => [r.id, r.x - r.hx, r.y - r.hy, r.ready, looks[i] && looks[i].at ? r.ready - looks[i].at.now : r.ready]));
        if (looks.length !== 3 || m.rats.length !== 3 || m.rats.some(l => l.length < 2) || looks.some(t => !t.at)) f.push('the rats\' records not read as the looks were typed: ' + m.rats.length);
        else {
          const due = i => m.rats[i].map(r => r.ready - looks[i].at.now);
          if (!m.rats[0].every(home) || due(0).some(d => d < 5 * H)) f.push('the rats not asleep at home at 10 o\'clock, due as they wake: ' + rs(0));
          if (due(1).some(d => d > 6000)) f.push('the rats not up as they woke, past sixteen hours: ' + rs(1));
          if (due(2).some(d => d > 6000)) f.push('the rats not about at 18 o\'clock: ' + rs(2));
        }
        const g3 = T.filter(t => t.line === 'wait 30 minutes').slice(8), alarms = g3.flatMap(t => t.dens.filter(e => e.kind === 'E_DEN' && e.de === 'DE_ALARM'));
        if (!m.body) f.push('the body\'s scene not set: no soldier of another lair away');
        else {
          if (alarms.length !== 1) f.push('the alarm not raised once as the body was found: ' + alarms.length);
          if (!m.body.fatt || m.body.fatt[0] !== 1 || m.body.fatt[m.body.fatt.length - 1] !== 0) f.push('the garrison\'s attitude not a step worse as the body was found: ' + JSON.stringify(m.body.fatt));
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0asleep', near: ['denizens'], meaning: true, what: "Dingus Quest Zero's sleepers lying down (2026-11-02: the minds' leftovers of slices 4.5-4.7; DENIZENS.md's ways, positions, bodies found, HOUSE.md's lying down to sleep, for review): dq0daily's garrison of men-at-arms (seed 2, their lair south of the start), watched for two hours from the start, ten minutes at a time: those asleep at their beds lying prone (the hub's actors' pos, POS_PRONE, I_RISE's: the world's), none of them standing asleep for long, and the snores still heard (an event told where a sleeper lies: not got up for)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/t',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/t/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/t', ...Array(12).fill('wait 10 minutes'), 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const sleepers = T.flatMap(t => t.after ? t.after.actors.filter(a => a.state && a.name === 'soldier' && a.doing === 'DO_SLEEP') : []);
        if (!sleepers.length) f.push('none of the garrison near the character asleep at a look');
        else if (sleepers.filter(a => a.pos === 1).length * 2 < sleepers.length) f.push('the sleepers not lying prone: ' + JSON.stringify(sleepers.map(a => [a.did, a.pos])));
        if (!T.flatMap(t => t.dens).some(e => e.kind === 'E_DEN' && e.de === 'DE_SNORE')) f.push('the sleepers never snoring');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rounds', near: ['denizens'], what: "Dingus Quest Zero's bodies found on the records' rounds (2026-11-02: the minds' leftovers; DENIZENS.md's ways, positions, bodies found, HOUSE.md's a body found on a record's rounds, for review; dq0d.c's part 17, rfound): dq0daily's garrison of men-at-arms and their captain (seed 2, their lair by the way down where the character starts), the character gone down the way (d: the garrison no actors after, their records going about their days, on watch walking their rounds); the scene set by pokes as the first wait's typed (dq0Rec's: a soldier of the placed lair, its rank poked a post's (D_RANK: a leader), made dead at its lair's home, as killed there: its faction's body not found yet, dq0d's unf; the lair's others put six squares from it, so a gathering's its own); waited for two hours, ten minutes at a time, the records read as each wait's typed: the body found by one of them on its rounds (its record's D_MET cleared), out of the character's hearing (no alarm told), its faction's attitude to the character a step worse (the head's fatt), every other of its lair gathered to it as it's found (each on a square beside it, none of them all there before), and, a leader's body, its loss known to them all (their records' D_LOST)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/t',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/t/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/t', 'd', 'wait 1 minute', ...Array(12).fill('wait 10 minutes'), 'quit', 'echo $status'] })) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '';
        m.samples = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13 && m.dq0 && m.dq0.bank(0) && /^wait/.test(typed)) {
              const scene = typed === 'wait 1 minute' && !m.body, body = () => {   // (The scene's line a scene: the records' copy, the minds' own later)
              const R = dq0Rec(m), h = m.dq0.bank(0), u16 = o => R.mem[o] | R.mem[o + 1] << 8, w16 = (o, v) => { R.mem[o] = v & 255; R.mem[o + 1] = v >> 8; };
              const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
              const ad = dq0Ad(lbl), ram = scene ? dq0LaterRam(m) : m.taskRam[h[12 + 2]];
              const MANARMS = 28, all = [...Array(224).keys()].map(i => R.at(i)), out = all.filter(o => R.mem[o] && (R.mem[o + R.FLAGS] & 7) === 1);
              if (typed === 'wait 1 minute' && !m.body) {   // (A soldier with a post, dead at its lair's home, the rest away from it)
                const g = out.filter(o => out.some(p => p !== o && R.mem[p + 25] === R.mem[o + 25] && u16(p + 21) === u16(o + 21)));
                const o = g[0];
                m.outs = out.length;
                if (o) {
                  R.mem[o + R.FLAGS] = 4 | 8 | 1 << 5;      // (D_DEAD, D_MET: killed as an actor; D_RANK 1, a post)
                  for (const p of g) if (p !== o) w16(p + 8, u16(p + 21) + 6);
                  w16(o + 8, u16(o + 21));
                  w16(o + 10, u16(o + 23));
                  ram[ad('unf') + (R.mem[o + 25] & 7)] = 1;
                  m.body = { o, x: u16(o + 21), y: u16(o + 23), fac: R.mem[o + 25] };
                }
              }
              if (m.body) {
                const b = m.body.o, mine = out.filter(o => o !== b && R.mem[o + 25] === m.body.fac);
                m.samples.push({ met: R.mem[b + R.FLAGS] & 8, fatt: ram[ad('hd') + 4 + (m.body.fac & 7)],
                  near: mine.filter(o => Math.max(Math.abs(u16(o + 8) - m.body.x), Math.abs(u16(o + 10) - m.body.y)) <= 1).length,
                  lost: mine.filter(o => R.mem[o + R.FLAGS] & 16).length, n: mine.length });
              }
              };
              if (scene) dq0SceneNow(m, body, null, 'a soldier dead at its post', true); else body();
            }
            if (c === 13) typed = '';
            else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], S = m.samples;
        if (!m.body) return ['the scene not set: no lair of the garrison placed with its records out and not actors (' + m.outs + ' out)'];
        const i = S.findIndex(s => !s.met);
        if (i < 0) f.push('the body never found on the records\' rounds: ' + JSON.stringify(S));
        else {
          if (!(S[S.length - 1].fatt < S[0].fatt)) f.push('the garrison\'s attitude not a step worse as the body was found: ' + JSON.stringify(S.map(s => s.fatt)));
          if (!i || S.slice(0, i).some(s => s.near === s.n)) f.push('its faction all about the body before it was found: ' + JSON.stringify(S));
          if (S[i].near !== S[i].n || !S[i].n) f.push('its faction not gathered to the body as it was found: ' + JSON.stringify(S));
          if (S[i].lost !== S[i].n) f.push('the leader\'s loss not known to its faction (D_LOST): ' + JSON.stringify(S));
          if (S[i - 1].lost) f.push('its loss known before it was found: ' + JSON.stringify(S));
        }
        if (T.flatMap(t => t.dens).some(e => e.kind === 'E_DEN' && e.de === 'DE_ALARM')) f.push('an alarm told: the records\' finding is out of the character\'s hearing');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0leisure', near: ['denizens'], what: "Dingus Quest Zero's leisure (the user's, 2026-10-31: \"For what folk are doing, there should be a \\\"leisure time\\\" state where denizens sit around reading, dicing, talking, drinking, playing other games, etc.\"; DENIZENS.md's daily life, HOUSE.md's leisure, for review; dq0d.c's parts 9 and 10), by the actors' records and their events: dq0daily's garrison of men-at-arms (seed 2, their lair south of the start; soldiers keep watches, a third about, its meal then two hours' leisure) watched from the start, half an hour at a time till half past nine: some at leisure (DO_LEISURE and its pastime, one of a soldier's: dice, a board game, talk, song, drink, mending), those of a lair at a social pastime at it together (the same pastime, a few squares apart in the room they gather in), heard at it (DE_PASTIME's events: dice rattling, laughter, singing); one at its cups in drink after (its mood's AM_DRUNK, the hub's, as the next line's typed); then one at leisure put beside the character (a test's scene: its square the hub's and its record's) and struck at, rising to fight and letting its pastime go, told (DE_UP and its pastime); and the garrison's game begun at 13 o'clock (the environment's dq0hour=13), the third with the lair's two (ids 4 and 7) about, at leisure together after its meal",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/s',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/s/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/s', ...Array(7).fill('wait 30 minutes'), 'wait 1 minute', 'attack the soldier', 'quit',
        'mkdir /ram/u', 'cp /ram/s/d1/theme.txt /ram/u/theme.txt', 'echo hour 13 >> /ram/s.sc', 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/u -S /ram/s.sc', ...Array(3).fill('wait 30 minutes'), 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, []);                                       // (The actors' moods and doings at each line's Enter: m.hearts)
        const a = m.acia, read = a.read;                      // (One at leisure put beside the character as the first wait
        let typed = '', seen = 0;                             //   of a minute's typed)
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 1 minute' && !seen++ && m.dq0 && m.dq0.bank(0)) dq0SceneNow(m, (h, L, K, at) => {   // (A scene: the hub's copy, its record's)
                const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
                const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && h[at(j) + L.aDoing] >= K.DO_LEISURE);
                if (k !== undefined) {
                  const d = h[L.pcFace] & 7, x = (h[L.pcX] | h[L.pcX + 1] << 8) + [0, 1, 1, 1, 0, -1, -1, -1][d], y = (h[L.pcY] | h[L.pcY + 1] << 8) + [-1, -1, 0, 1, 1, 1, 0, -1][d];
                  const R = dq0Rec(m), o = R.at(h[at(k) + L.aDen]);
                  w16(at(k) + L.aX, x);
                  w16(at(k) + L.aY, y);
                  h[at(k) + L.aSn] = h[L.pcSn];
                  h[at(k) + L.aPos] = K.POS_STANDING;
                  R.mem[o + R.X] = x & 255; R.mem[o + R.X + 1] = x >> 8;
                  R.mem[o + R.Y] = y & 255; R.mem[o + R.Y + 1] = y >> 8;
                  R.mem[o + R.SN] = h[L.pcSn];
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.risen = { k, past: h[at(k) + L.aDoing] - K.DO_LEISURE };
                }
              }, null, 'one at leisure beside the character', true);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], C = require('../../../sim/lib/dq0meaning.js').constants().names, LEISURE = C.DO_LEISURE, PT = ['read', 'dice', 'game', 'talk', 'sing', 'drink', 'mend', 'whittle'];
        const waits = T.filter(t => t.line === 'wait 30 minutes'), num = d => typeof d === 'number' ? d : C[d];
        const men = s => s ? s.actors.filter(a => a.state && a.name === 'soldier') : [], at = s => men(s).filter(a => num(a.doing) >= LEISURE);
        const show = s => JSON.stringify(men(s).map(a => [a.did, a.doing, a.x, a.y]));
        const lei = waits.flatMap(t => at(t.after));
        if (!lei.length) f.push('none at leisure: ' + waits.map(t => show(t.after)).join(' '));
        if (lei.some(a => !['dice', 'game', 'talk', 'sing', 'drink', 'mend'].includes(PT[num(a.doing) - LEISURE]))) f.push('a pastime not a soldier\'s: ' + JSON.stringify(lei.map(a => a.doing)));
        const together = waits.some(t => at(t.after).some(a => { const p = num(a.doing) - LEISURE; return p >= 1 && p <= 5 &&
          at(t.after).some(b => b.did !== a.did && num(b.doing) === num(a.doing) && Math.max(Math.abs(a.x - b.x), Math.abs(a.y - b.y)) <= 4); }));
        if (!together) f.push('no two of a lair at a social pastime together: ' + waits.map(t => show(t.after)).join(' '));
        const evs = waits.flatMap(t => t.dens.filter(e => e.kind === 'E_DEN'));
        if (!evs.some(e => e.a > C.DE_PASTIME && e.a < C.DE_UP)) f.push('no pastime heard (DE_PASTIME)');
        const drinkers = (m.hearts || []).filter(h => h.acts.some(x => x.doing === LEISURE + 5));
        if (drinkers.length && !(m.hearts || []).some(h => h.acts.some(x => x.mood & 0x10))) f.push('one at its cups never in drink (AM_DRUNK)');
        if (!m.risen) f.push('none at leisure to put beside the character');
        else {
          const ups = T.filter(t => t.line === 'wait 1 minute' || t.line === 'attack the soldier').flatMap(t => t.dens.filter(e => e.kind === 'E_DEN' && e.a === C.DE_UP + m.risen.past));
          if (!ups.length) f.push('the one put beside the character not told letting its pastime go (DE_UP + ' + PT[m.risen.past] + ')');
        }
        return f;
      },
      sdcard: true
    },
    {
      // (Digging, every actor's: the one-path plan's stage 2, 2026-11-23, ONE-PATH.md's 2.1 and 2.7, the act audit's E1, G-13)
      name: 'dq0digact', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's digging, every actor's by the same code (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, digging, 2026-11-23; the user's, 2026-11-17: \"the player and any agent's interaction with the world are handled the same, in all cases\"): the character with a pick digs the wall north of seed 374's start (\"dig north\": asked as it begins whether anything it has bites, the referee's RS_DIG_QUERY, actor 0; each spell the act table's dig row, ten minutes, its noise the row's, the referee's P_DIG test by its sheet, its rate the house's pick's by the grade, the pick worn), and dq0tunnel's dwarf miner digs round the portcullis let down (seed 1, -R 7: its own dig, the same spell, dq0w.c's dig_spell: asked as it begins, actor its own; each spell the row's ten minutes on its ready, its noise told at the row's loudness, the same test by its own sheet, the same rate by the grade for the same tool chosen by the referee among its things, its own pick worn); and a dwarf miner kept from the character by its cell's door made secret (its portal's state, PS_HIDDEN, poked; its search done already, the minds' asrch, so it's the dig that finds it) digs at the door as the wall it seems, and finds it as its facing comes off (E_DEN DE_DIGFOUND at the door, the act table's digfound row: the door no longer hidden, a find for every one, as the character's own dig finds one)",
      init: 't_rc', cycles: 4500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        'dq0 -p t -s 374 -c /ram/c.hm -k pick -T -S /ram/s.sc', 'dig north', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7 -S /ram/s.sc', 'open the door', 'go through it', 'turn around', 'close the door ahead', 'wait', 'wait 2 hours', 'wait', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -S /ram/s.sc', 'open the door', 'close the door', 'wait 1 minute', 'wait 30 minutes', 'wait 30 minutes', 'look', 'quit',
        'echo $status'] })) },
      start(m) {
        dq0OnWords(m, [{ g: 2, line: 'wait 2 hours', n: 1, words: 'A fresh tunnel opens', text: 'stop\r' }]);   // (dq0tunnel's)
        const others = (h, L) => { for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName] && (!m.door || k !== m.door.k)) h[L.actor + k * L.actorSize + L.aAttitude] = 3; };
        dq0DoorScene(m, 'wait 1 minute', 'dwarf miner', (h, o, L, K) => {   // (Game 3's: the cell's door, shut, made secret; its search done)
          m.g2 = m.door;
          const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ad = dq0Ad(lbl), ram = dq0LaterRam(m);
          const k = (o - L.actor) / L.actorSize, f = h[L.pcFace], u16 = a => h[a] | h[a + 1] << 8;
          const x = u16(L.pcX) + [0, 1, 1, 1, 0, -1, -1, -1][f], y = u16(L.pcY) + [-1, -1, 0, 1, 1, 1, 0, -1][f], p = dq0Portal(m, h[L.pcSn], x, y);
          if (p) { p.b[p.state] |= 0x20; m.secret = { x, y, sn: h[L.pcSn] }; }
          ram[ad('asrch') + k] = 1;
        });
        dq0Scene(m, [{ g: 2, line: 'wait 2 hours', n: 1, call: others }]);   // (The other miners let be: this one's the scene)
        dq0DoorScene(m, 'wait', 'dwarf miner', (h, o, L, K) => {
          const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ad = dq0Ad(lbl), ram = dq0LaterRam(m);
          const k = (o - L.actor) / L.actorSize, f = h[L.pcFace];
          ram[ad('atry') + k] = ((f >> 1) + 2 & 3) | 3 << 2;   // (Its tries at the portcullis given up already: dq0tunnel's)
        });
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout(), s = dq0Prose(out);
        const C = require('../../../sim/lib/dq0meaning.js').constants().names;
        const PICK = [2, 4, 2, 6, 4], rate = (g, k) => g === 2 ? 2 * k : g === 1 ? k : g === -1 ? k >> 1 : 0;
        const digs = list => list.filter(q => q.kind === K.RS_TEST && q.a === A.dig.p);
        // (The character's: dig north, its query and its spells)
        const dg = T.of('dig north'), mine = digs(dg.replies).filter(q => q.actor === 0);
        if (!mine.some(q => q.steps === 127)) f.push('the character\'s dig not asked as it began (RS_DIG_QUERY): ' + JSON.stringify(mine.map(q => q.steps)));
        const ms = mine.filter(q => q.steps === 0);
        if (!ms.length) f.push('the character dug no spell: ' + dq0Evs(dg));
        for (const q of ms) if (q.w1 !== rate(q.grade, PICK[q.b > 4 ? 1 : q.b])) f.push('the character\'s spell\'s rate not the pick\'s by its grade: ' + JSON.stringify([q.b, q.grade, q.w1]));
        const spells = dq0Ev(dg, 'E_DIG').filter(e => /^D_(GOOD|POOR|SLIP|ROOF|THROUGH|FOUND)$/.test(e.how));
        for (let i = 1; i < spells.length; i++) if (spells[i].t - spells[i - 1].t !== A.dig.t) f.push('the character\'s spell not the row\'s ten minutes: ' + JSON.stringify(spells.map(e => [e.how, e.t])));
        const nz = dq0Ev(dg, 'E_NOISE');
        if (nz.length !== ms.length || nz.some(e => e.loud !== A.dig.loud)) f.push('the character\'s spells\' noise not the row\'s, one a spell: ' + JSON.stringify(nz.map(e => [e.loud, e.what])) + ' ' + ms.length);
        const pk = s0 => s0 ? dq0Carried(s0).find(i => i.name === 'pick') : null, p0 = pk(dg.at), p1 = pk(dg.after);
        if (ms.some(q => q.w2) && !(p0 && p1 && p1.wear > p0.wear)) f.push('the character\'s pick not worn by its spells: ' + JSON.stringify([p0 && p0.wear, p1 && p1.wear]));
        // (The dwarf's: dq0tunnel's scene)
        const D2 = m.g2;
        if (!D2) return [...f, 'the dwarf\'s scenes not set: ' + JSON.stringify(m.doorWhy)];
        const by = D2.k + 1, g2 = T.filter(t => t.game === T.of('wait 2 hours').game);
        const theirs = digs(g2.flatMap(t => t.replies)).filter(q => q.actor === by), ts = theirs.filter(q => q.steps === 0);
        if (!theirs.some(q => q.steps === 127)) f.push('the dwarf\'s dig not asked as it began (RS_DIG_QUERY, its actor): ' + JSON.stringify(theirs.map(q => q.steps)));
        if (!ts.length) f.push('the dwarf dug no spell by the referee\'s test');
        for (const q of ts) if (q.w1 !== rate(q.grade, PICK[q.b > 4 ? 1 : q.b])) f.push('the dwarf\'s spell\'s rate not the pick\'s by its grade: ' + JSON.stringify([q.b, q.grade, q.w1]));
        const de = g2.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === by && [C.DE_DIG, C.DE_DIGS, C.DE_ROOF, C.DE_DUG].includes(e.a));
        if (!de.length || de[0].a !== C.DE_DIG) f.push('the dwarf\'s dig not begun (DE_DIG): ' + JSON.stringify(de.map(e => [e.a, e.t])));
        for (let i = 1; i < de.length; i++) if (de[i].t - de[i - 1].t < A.dig.t) f.push('the dwarf\'s spell sooner than the row\'s ten minutes: ' + JSON.stringify(de.map(e => [e.a, e.t])));
        if (de.filter(e => e.a !== C.DE_DUG).some(e => e.b !== A.dig.loud)) f.push('the dwarf\'s spells\' noise not the row\'s: ' + JSON.stringify(de.map(e => [e.a, e.b])));
        if (!de.some(e => e.a === C.DE_DUG)) f.push('the dwarf never through (DE_DUG)');
        // (The dwarf's pick as its dig ends, at the wait after its tunnel opens, not at the quit: by then the fight that
        // follows may have taken it out of the actors, the character knocked senseless (room7, 2026-11-23: climb1's minds
        // with dig1's scene))
        const fin = T.of('wait', T.of('wait 2 hours').k).at, all = fin ? fin.items.filter(i => i.name === 'pick' && i.did && fin.actors.some(a => a.k === D2.k && a.did === i.did)) : [];
        const st = T.of('wait 2 hours').at, was = st ? st.items.filter(i => all.some(j => j.n === i.n)) : [];
        if (ts.some(q => q.w2) && !(all.length && was.length && all[0].wear > was[0].wear)) f.push('the dwarf\'s own pick not worn by its spells: ' + JSON.stringify([was.map(i => i.wear), all.map(i => i.wear)]));
        // (Game 3's: a hidden door dug into, found)
        if (!m.door || m.door === D2 || !m.secret) f.push('the hidden door\'s scene not set: ' + JSON.stringify([m.doorWhy, m.secret]));
        else {
          const by3 = m.door.k + 1, w3 = T.of('wait 1 minute'), g3 = T.filter(t => t.k >= w3.k);
          const fd = g3.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === by3 && e.a === C.DE_DIGFOUND);
          const t3 = digs(g3.flatMap(t => t.replies)).filter(q => q.actor === by3 && q.steps === 0);
          if (fd.length !== 1 || fd[0].w1 !== m.secret.x || fd[0].w2 !== m.secret.y) f.push('the dwarf\'s dig never found the hidden door, once, at the door (DE_DIGFOUND): ' + JSON.stringify(fd.map(e => [e.w1, e.w2, e.t])) + ' ' + JSON.stringify(m.secret) + ' its tests ' + JSON.stringify(t3.map(q => [q.b, q.grade, q.w1])));
          else {
            if (fd[0].b !== A.digfound.loud) f.push('the find not as loud as the digfound row: ' + fd[0].b);
            const st = m.door.state(m.secret.x, m.secret.y);
            if (st < 0 || (st & 0x20)) f.push('the door still hidden after its find: ' + st);
            if (!/strikes a door behind the facing|a scraping in rock/.test(s.slice(s.lastIndexOf('> wait 1 minute')))) f.push('the find neither seen nor heard');
          }
        }
        this.notes = ['the character ' + JSON.stringify(ms.map(q => [q.b, q.grade, q.w1])), 'the dwarf ' + JSON.stringify(ts.map(q => [q.b, q.grade, q.w1]))];
        if (f.length) f.push(s.slice(s.indexOf('> dig north'), s.indexOf('> dig north') + 800));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0thingact', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's things handled, every actor's by the same rules and the same rows (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, things and robbery, 2026-11-23; the act audit's C2, C6, G-9, G-10, G-11): at seed 1 (dq0denportal's wary bandits, dq0 -k dagger, cloak, chest; the chest set a square off, room beyond it) the character drops its kit's sack, puts the dagger in it (the act table's put row and its reach, a level, part 56's R_IN), takes it from the sack (the takeout row and the level), wears the cloak (the don row), and pushes the chest a square (the drag row: its time, its noise, the referee's P_FORCE by its weight); the same scene again, each game in its own folder, the things laid down, the character a step aside, a bandit put where it stood, made its own (its dagger and cloak, IW_DEN, held), the others sent away and the minds quiet: its intents posted as its mind would (I_ROB FE_PUT, FE_TAKE, FE_DON, FE_DRAG: wthings.c's, wfetch.c's, wdrag.c's, the character's own code), each its time on its ready the character's, its noise the same row's (E_DEN DE_DRAGS, as loud as the character's E_NOISE), its test the same purpose at the same steps (its own sheet), its effect the character's (in the sack, out of it, worn, the chest a square on as its test went)",
      init: 't_rc', cycles: 3200e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k dagger,cloak,chest -S /ram/s.sc",
        'drop the chest', 'wait', 'drop the sack', 'hold the dagger', 'put the dagger in the sack', 'take the dagger from the sack', 'drop the dagger', 'hold the cloak', 'wear the cloak', 'push the chest', 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k dagger,cloak,chest -S /ram/s.sc",
        'drop the chest', 'wait', 'drop the sack', 'drop the dagger', 'drop the cloak', 'step south', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.tact = { k: null, ready: [], items: {} };
        const u16 = (b, o) => b[o] | b[o + 1] << 8, w16 = (b, o, v) => { b[o] = v & 255; b[o + 1] = v >> 8 & 255; };
        const store = (L, fn) => { const per = Math.floor(0x2000 / L.itemSize); for (let i = 1; i < L.nitems; i++) { const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize; if (b && b[o + L.iKind] && fn(i, b, o) === false) return; } };
        const rd = (h, L) => { const o = L.actor + S.k * L.actorSize + L.aReady; return (h[o] | h[o + 1] << 8 | h[o + 2] << 16 | h[o + 3] << 24) >>> 0; };
        // (The chest set a square from the character, room beyond it both ways it'll be pushed: the character's way and a
        //   bandit's across it; the same square both games, the start's)
        const spot = (h, L, K) => {
          const px = u16(h, L.pcX), py = u16(h, L.pcY), sn = h[L.pcSn];
          const st = stratumRec(m, sn), tile = (x, y) => { const c = st && liveChunks(m, sn)[((x - st.x0) >> 5) + ',' + ((y - st.y0) >> 5)]; return c ? c[443 + ((y - st.y0) & 31) * 32 + ((x - st.x0) & 31)] & 15 : -1; };
          const ok = (x, y) => [1, 4].includes(tile(x, y)) && !(x === px && y === py);
          for (const [dx, dy] of [[-1, 0], [1, 0], [0, -1]]) {   // (Not south: the character steps that way in the second game, the bandit then where it stood, pushing the chest the same way)
            const cx = px + dx, cy = py + dy, perp = [0, 0];
            if (ok(cx, cy) && ok(cx + dx, cy + dy) && ok(px, py + 1)) {
              store(L, (i, b, o) => { if (b[o + L.iKind] === K.KI_CHEST && b[o + L.iWhere] === K.IW_SQUARE) { w16(b, o + L.iX, cx); w16(b, o + L.iY, cy); return false; } });
              S.chest = [cx, cy];
              S.perp = perp;
              S.p0 = [px, py];
              return;
            }
          }
        };
        // (The bandit beside the things laid down: its own dagger and cloak, the sack and chest lying; the minds quiet, its ready held)
        const scene = (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined || !S.chest) return;
          dq0Alone(m, h, L, K, at, k);
          S.k = k;
          const sn = h[L.pcSn], x = S.p0[0], y = S.p0[1];   // (Where the character stood: it pushes the chest the same way, the character stepped aside)
          const o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), did = u16(h, o + L.aDid);
          const far = 3000 + (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16);
          for (let i = 0; i < 4; i++) h[o + L.aReady + i] = far >>> (8 * i) & 255;
          w16(h, o + L.aX, x); w16(h, o + L.aY, y); h[o + L.aSn] = sn; h[o + L.aState] = 1; h[o + L.aPos] = K.POS_STANDING; h[o + L.aDoing] = K.DO_WAIT;
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_INDIFFERENT;
          R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8; R.mem[q + R.SN] = sn;
          store(L, (i, b, io) => {   // (A cloak of its own worn taken off: one of a kind worn at once, its don's the scene's)
            if (b[io + L.iKind] === K.KI_CLOAK && b[io + L.iWhere] === K.IW_DEN && (b[io + L.iOwner] | b[io + L.iHide] << 8) === did) b[io + L.iFlags] &= ~4;
          });
          store(L, (i, b, io) => {
            if (b[io + L.iWhere] !== K.IW_SQUARE || b[io + L.iSn] !== sn) return;
            const kd = b[io + L.iKind], ix = u16(b, io + L.iX), iy = u16(b, io + L.iY);
            if (kd === K.KI_CHEST) { if (ix === S.chest[0] && iy === S.chest[1]) S.items.chest = i; return; }
            if (ix !== S.p0[0] || iy !== S.p0[1]) return;   // (The things laid down where the character stood)
            if (kd === K.KI_DAGGER || kd === K.KI_CLOAK) {   // (Its own, held: IW_DEN at its lair's home, its number's)
              b[io + L.iWhere] = K.IW_DEN; b[io + L.iOwner] = did & 255; b[io + L.iHide] = did >> 8; b[io + L.iFlags] |= 2;
              S.items[kd === K.KI_DAGGER ? 'dagger' : 'cloak'] = i;
            }
            if (kd === K.KI_SACK) { S.items.sack = i; S.at = [u16(b, io + L.iX), u16(b, io + L.iY)]; }   // (At the character's feet, within its reach)
            if (kd === K.KI_CHEST) S.items.chest = i;
          });
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        const post = (what, f) => (h, L, K) => { if (S.k === null || !S.items.dagger) return; S.ready.push([what, rd(h, L)]); dq0Intent(h, L, K.I_ROB, S.k, f(K)); };
        const mark = (h, L) => { if (S.k !== null) S.ready.push(['end', rd(h, L)]); };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: spot }, { g: 2, line: 'wait', n: 1, call: spot }, { g: 2, line: 'wait', n: 2, call: scene },
          { g: 2, line: 'wait', n: 2, call: post('put', K => ({ a: K.FE_PUT, t: S.items.dagger, w1: S.items.sack, w2: 0 })) },
          { g: 2, line: 'wait', n: 3, call: post('takeout', K => ({ a: K.FE_TAKE, t: S.items.dagger, w1: S.at[0], w2: S.at[1] })) },
          { g: 2, line: 'wait', n: 4, call: post('don', K => ({ a: K.FE_DON, t: S.items.cloak })) },
          { g: 2, line: 'wait', n: 5, call: post('drag', K => ({ a: K.FE_DRAG, t: S.items.chest, w2: 0 })) },
          { g: 2, line: 'wait', n: 6, call: mark }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout(), S = m.tact;
        if (S.k === null || !S.items.dagger || !S.items.sack || !S.items.cloak || !S.items.chest || S.ready.length < 5) return ['the scene not set: ' + JSON.stringify(S)];
        const took = line => { const t = T.of(line); return t.at && t.after ? t.after.now - t.at.now : -1; };
        const R_IN = 20, r = Object.fromEntries(S.ready.map(([w, v], i) => [w, i + 1 < S.ready.length ? S.ready[i + 1][1] - v : 0]));
        // (Its time: the character's clock, the bandit's ready, by the same rows and the same reach)
        const want = { put: A.put.t + R_IN, takeout: A.takeout.t + R_IN, don: A.don.t, drag: A.drag.t };
        const mine = { put: took('put the dagger in the sack'), takeout: took('take the dagger from the sack'), don: took('wear the cloak'), drag: took('push the chest') };
        for (const w of Object.keys(want)) {
          if (mine[w] !== want[w]) f.push('the character\'s ' + w + ' not its row\'s time and its reach (' + want[w] + '): ' + mine[w]);
          if (r[w] !== want[w]) f.push('the bandit\'s ' + w + ' not the character\'s time (' + want[w] + '): ' + r[w]);
        }
        // (Its noise and its test: the drag row's for both)
        const pu = T.of('push the chest'), nz = dq0Ev(pu, 'E_NOISE', {});
        if (!nz.length || nz[0].loud !== A.drag.loud) f.push('the character\'s push not heard as the drag row says: ' + dq0Evs(pu) + ' ' + JSON.stringify(nz));
        const g2 = T.filter(t => t.game === 1), de = g2.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.a === A.drag.de && e.who === S.k + 1);
        if (de.length !== 1 || (de[0].b & 15) !== A.drag.loud) f.push('the bandit\'s drag not told once as loud as the row (E_DEN DE_DRAGS): ' + JSON.stringify(de));
        const mt = pu.replies.filter(q => q.kind === K.RS_TEST && q.actor === 0 && q.a === A.drag.p), bt = g2.flatMap(t => t.replies).filter(q => q.kind === K.RS_TEST && q.actor === S.k + 1 && q.a === A.drag.p);
        if (mt.length !== 1 || bt.length !== 1 || mt[0].steps !== bt[0].steps) f.push('the drag\'s test not the same purpose at the same steps: ' + JSON.stringify([mt.map(q => q.steps), bt.map(q => q.steps)]));
        // (Its effect, the character's: in the sack, out of it, worn, the chest a square on as its test went)
        const last = T.filter(t => t.game === 1 && t.line === 'quit')[0], it = n => last && last.at && last.at.items.find(i => i.n === n);
        const d = it(S.items.dagger), c = it(S.items.cloak), ch = it(S.items.chest);
        if (!d || d.where !== 'IW_DEN') f.push('the dagger not the bandit\'s in hand again after its take: ' + JSON.stringify(d));
        if (!c || c.where !== 'IW_DEN' || !(c.flags & DQ0_I.WORN)) f.push('the cloak not worn by the bandit: ' + JSON.stringify(c));
        const moved = ch && S.chest && (ch.x !== S.chest[0] || ch.y !== S.chest[1]);
        if (bt.length && (bt[0].grade > 0) !== !!moved) f.push('the chest ' + (moved ? 'moved' : 'not moved') + ' though the bandit\'s test went ' + bt[0].grade);
        this.notes = ['times ' + JSON.stringify([mine, r]), 'drag steps ' + JSON.stringify([mt.map(q => [q.steps, q.grade]), bt.map(q => [q.steps, q.grade])])];
        return f;
      },
      sdcard: true
    },
    {
      // (The step and its hazards, every actor's: the one-path plan's stage 2, 2026-11-23, ONE-PATH.md's 2.1 and 2.7)
      name: 'dq0steptrap', outfit: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's trap underfoot, every actor's by the same code (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, the step and its hazards, 2026-11-23; the act audit's G1 and G3): at seed 1 (wary giant rats) a covered pit, hidden at 20 and 6 feet deep, poked into the square ahead of the character (its chunk's trap records); the character steps onto it: the referee's P_NOTICE_SIGHT (a passing glance's, b 1) at its hiding's steps, then, not noticed, the pit given way (TRAP_STATE sprung) and the character's fall into it (RS_HARM HC_FALL, the floor, a clear fall, its depth); and the same scene with giant rats (hostile, no bow), one put two squares ahead, the pit between: coming on, the same test asked of it (its actor's, by its sheet), and, not noticed, the same pit given way and the same harm asked of it (RS_HARM at its actor: its sheet's wound, a blow's), told if seen ('The giant rat falls into a pit!'); in the pit, it climbs out a round at a time when it would leave it (the referee's P_CLIMB, its actor's), unless it fights from it, or lies senseless or dead",
      init: 't_rc', cycles: 9000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc', 'echo sky 0 >> /ram/s.sc',
        "echo 'faction | Gnawers | rat | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'wait', 'step forward', 'wait', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -S /ram/s.sc', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.step = { trap: {}, foe: undefined };
        const HEAD_X = [0, 1, 1, 1, 0, -1, -1, -1], HEAD_Y = [-1, -1, 0, 1, 1, 1, 0, -1];
        const [TILE, NTRAPS, TRAP, TSIZE] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, tile)', 'offsetof (struct chunk, ntraps)',
          'offsetof (struct chunk, trap)', 'sizeof (struct trap)']);
        // (A covered pit, hidden at 20, 6 feet deep, on the square beside the character: its chunk's trap records, game g's;
        // the way it faces (game 1's, stepped onto), else the first of the four ways with floor two squares on (game 2's:
        // the rat's straight way to it through the pit)
        const pit = g => (h, L) => {
          const w = m.dq0.bank(WORLD_BANK_N), sn = h[L.pcSn], st = stratumRec(m, sn), px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;
          const cell = (x, y) => {
            const ox = x - st.x0, oy = y - st.y0;
            for (let c = 0; c < 24; c++) {
              const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
              if (w[sl] && b && w[sl + 1] === sn && (w[sl + 2] | w[sl + 3] << 8) === ox >> 5 && (w[sl + 4] | w[sl + 5] << 8) === oy >> 5) return { b, o, lx: ox & 31, ly: oy & 31 };
            }
            return null;
          };
          const floor = (x, y) => { const q = cell(x, y); return q && (q.b[q.o + TILE + q.ly * 32 + q.lx] & 15) === T_FLOOR; };
          const d = g === 1 ? h[L.pcFace] & 7 : [0, 2, 4, 6].find(e => floor(px + HEAD_X[e], py + HEAD_Y[e]) && floor(px + 2 * HEAD_X[e], py + 2 * HEAD_Y[e]));
          if (d === undefined) return;
          const x = px + HEAD_X[d], y = py + HEAD_Y[d], q = cell(x, y);
          if (!q || !floor(x, y) || q.b[q.o + NTRAPS] >= 6) return;
          const r = q.o + TRAP + q.b[q.o + NTRAPS]++ * TSIZE;
          [q.lx, q.ly, 1, 0, 0x20, 20, 6].forEach((v, i) => { q.b[r + i] = v; });   // (TK_COVERED, TT_PLATE, TS_HIDDEN, its hiding, its feet)
          S.trap[g] = { x, y, sn, face: d };
        };
        // (Game 2's: a hostile giant rat two squares ahead of the character, the pit between: dq0oilpool's scene, a beast with no bow)
        const ahead = (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j)), T2 = S.trap[2];
          if (k === undefined || !T2) return;
          dq0Alone(m, h, L, K, at, k);
          const x = T2.x + HEAD_X[T2.face], y = T2.y + HEAD_Y[T2.face];
          const R = dq0Rec(m), o = R.at(h[at(k) + L.aDen]), w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; }, r16 = (q, v) => { R.mem[o + q] = v & 255; R.mem[o + q + 1] = v >> 8 & 255; };
          w16(at(k) + L.aX, x); w16(at(k) + L.aY, y); h[at(k) + L.aSn] = h[L.pcSn]; h[at(k) + L.aPos] = K.POS_STANDING;
          r16(R.X, x); r16(R.Y, y); R.mem[o + R.SN] = h[L.pcSn];
          h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_HOSTILE;
          h[at(k) + L.aDoing] = 0;
          const px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;   // (Drawn on to the character's square,
          const ad = dq0Ad(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1')), ram = dq0LaterRam(m);   //   dq0DoorScene's: the pit on its way)
          ram[ad('drawn') + k] = 8;
          ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
          ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          S.foe = k;
        };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: pit(1) }, { g: 2, line: 'wait', n: 1, call: pit(2) }, { g: 2, line: 'wait', n: 2, call: ahead }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], S = m.step, s = dq0Prose(out);
        if (!S.trap[1] || !S.trap[2] || S.foe === undefined) return ['the scenes not set: ' + JSON.stringify(S)];
        const games = [], g = [];
        for (const t of T) { if (/^dq0 /.test(t.line)) games.push(g.splice(0)); g.push(t); }
        games.push(g.splice(0)); games.shift();
        if (games.length !== 2) return ['the two games not played: ' + games.length];
        const who = [0, S.foe + 1], name = ['the character', 'the rat'];
        const asks = (gm, w, rs, pred) => gm.flatMap(t => t.replies).filter(r => r.rs === rs && r.actor === w && pred(r.ask));
        const changes = gm => gm.flatMap(t => [t.at, t.after].filter(Boolean).flatMap(x => x.changes || []));
        const seen = [];
        for (let i = 0; i < 2; i++) {
          const gm = games[i], tr = S.trap[i + 1];
          const n = asks(gm, who[i], 'RS_TEST', q => q.purpose === 'P_NOTICE_SIGHT' && q.how === 1);
          const h = asks(gm, who[i], 'RS_HARM', q => q.cause === 'HC_FALL');
          seen.push({ notice: n.map(r => [r.ask.steps, r.grade]), harm: h.map(r => [r.ask.ground, r.ask.how, r.ask.feet, r.reply.band, r.reply.out]) });
          if (!n.length) { f.push(name[i] + ' asked no P_NOTICE_SIGHT (a passing glance\'s) stepping onto the hidden pit'); continue; }
          if (n[0].ask.steps !== -20) f.push(name[i] + '\'s notice test not at the trap\'s hiding (-20): ' + n[0].ask.steps);
          if (n[0].grade > 0) {
            if (h.length) f.push(name[i] + ' noticed the pit, but fell into it');
            continue;
          }
          if (h.length !== 1) { f.push(name[i] + ' not harmed once by the pit given way: ' + h.length); continue; }
          if (h[0].ask.ground !== 'GR_FLOOR' || h[0].ask.how !== 1 || h[0].ask.feet !== 6) f.push(name[i] + '\'s fall not the pit\'s (the floor, a clear fall, 6 feet): ' + JSON.stringify(h[0].ask));
          if (!changes(gm).some(c => c.what === 'TRAP_STATE' && c.x === tr.x && c.y === tr.y && (c.tile & 1))) f.push(name[i] + '\'s pit not kept sprung (TRAP_STATE)');
        }
        // (The two the same: the test's purpose, its how and its steps; the harm's cause, ground, how and feet)
        if (seen[0].notice.length && seen[1].notice.length && seen[0].notice[0][0] !== seen[1].notice[0][0]) f.push('the two notice tests not at the same steps: ' + JSON.stringify(seen));
        if (seen[0].harm.length && seen[1].harm.length && JSON.stringify(seen[0].harm[0].slice(0, 3)) !== JSON.stringify(seen[1].harm[0].slice(0, 3))) f.push('the two falls not the same harm: ' + JSON.stringify(seen));
        // (The rat in its pit: told if seen; climbing out a round at a time, unless it lies senseless or dead)
        this.notes = [JSON.stringify(seen)];
        const fell = seen[1].harm.length === 1, h1 = fell && seen[1].harm[0];
        const fought = games[1].flatMap(t => t.dens).some(e => e.kind === 'E_FIGHT' && (e.who === who[1] || e.def === who[1]) && !/HURT|252/.test(String(e.out)));
        const climbs = asks(games[1], who[1], 'RS_TEST', q => q.purpose === 'P_CLIMB');
        this.notes.push('the rat\'s climb rounds: ' + climbs.map(r => r.grade).join(' ') + (fought ? ' (it fought from the pit)' : ''));
        if (fell && !h1[4] && h1[3] < 4 && !fought && !climbs.length) f.push('the rat in the pit, not fighting, not climbing out (P_CLIMB, its actor\'s)');
        if (fell && !/The giant rat falls into a pit!|You hear something fall\./.test(s)) f.push('the rat\'s fall into the pit not told: ' + s.slice(-600));
        return f;
      },
      sdcard: true
    },
    {
      // (The step and its hazards' second round, the user's answers of 2026-11-23: each faction knows its own traps, a winged body over them)
      name: 'dq0trapknow', near: ['denizens', 'map'], what: "Dingus Quest Zero's traps known by faction, and a winged body over a floor's traps (the user's, 2026-11-23: \"each faction knows its own traps: its own walked round from the start; another's known only once found\"; \"a winged body passes over floor traps, pits and spilt oil\"): at seed 1 (a theme's Brigands, bandits; then its Bats) a covered pit, hidden at 20 and 6 feet deep, poked into the square beside the character with floor beyond it, as dq0steptrap's; a hostile bandit put two squares off, the pit between, drawn on to the character (dq0 -K: the denizens' kit none, so it comes on, no bow).  The pit its faction's own (its knowers' bit, struct trap's know: dq0d's wet_ok, ktrap.s): its ways go round it, no notice test asked of it at the pit, never on it, the pit left set.  Another's (know 0): the bandit coming on asked the passing glance's notice test at the pit's hiding (P_NOTICE_SIGHT, b 1, -20: its actor's), and, noticed, the pit its faction's to know from then on (TRAP_STATE, its bit in the knowers, still hidden from the character), else fallen into.  A bat (its body winged) drawn on across the same pit (know 0): no test, no fall, the pit left set.  And a lair of folk placed, the minds' I_KIT KT_TRAPS (its faction, its home square) sent the world (wstep.c's own_traps: the traps underfoot in its chunk its faction's)",
      init: 't_rc', cycles: 9000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'mkdir /ram/j', 'echo hour 128 >> /ram/s.sc', 'echo sky 0 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        "echo 'faction | Bats | bat | 0-0 | 30 | - | 0 | w | -' > /ram/j/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -S /ram/s.sc', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/j -S /ram/s.sc', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.tknow = { trap: {}, foe: {}, kb: {} };
        const HEAD_X = [0, 1, 1, 1, 0, -1, -1, -1], HEAD_Y = [-1, -1, 0, 1, 1, 1, 0, -1];
        const [TILE, NTRAPS, TRAP, TSIZE, KNOW] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, tile)', 'offsetof (struct chunk, ntraps)',
          'offsetof (struct chunk, trap)', 'sizeof (struct trap)', 'offsetof (struct trap, know)']);
        // (The pit beside the character, the first of the four ways with floor two squares on: dq0steptrap's game 2)
        const pit = g => (h, L) => {
          const w = m.dq0.bank(WORLD_BANK_N), sn = h[L.pcSn], st = stratumRec(m, sn), px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;
          const cell = (x, y) => {
            const ox = x - st.x0, oy = y - st.y0;
            for (let c = 0; c < 24; c++) {
              const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
              if (w[sl] && b && w[sl + 1] === sn && (w[sl + 2] | w[sl + 3] << 8) === ox >> 5 && (w[sl + 4] | w[sl + 5] << 8) === oy >> 5) return { b, bi: 5 + Math.floor(c / 3), o, lx: ox & 31, ly: oy & 31 };
            }
            return null;
          };
          const floor = (x, y) => { const q = cell(x, y); return q && (q.b[q.o + TILE + q.ly * 32 + q.lx] & 15) === T_FLOOR; };
          const d = [0, 2, 4, 6].find(e => floor(px + HEAD_X[e], py + HEAD_Y[e]) && floor(px + 2 * HEAD_X[e], py + 2 * HEAD_Y[e]));
          if (d === undefined) return;
          const x = px + HEAD_X[d], y = py + HEAD_Y[d], q = cell(x, y);
          if (!q || q.b[q.o + NTRAPS] >= 6) return;
          const r = q.o + TRAP + q.b[q.o + NTRAPS]++ * TSIZE;
          [q.lx, q.ly, 1, 0, 0x20, 20, 6].forEach((v, i) => { q.b[r + i] = v; });   // (TK_COVERED, TT_PLATE, TS_HIDDEN, its hiding, its feet)
          q.b[r + KNOW] = 0;
          S.trap[g] = { x, y, sn, face: d, bi: q.bi, r };   // (Its bank's number: the scene's copies are its own)
        };
        // (One of the theme's put two squares off, the pit between, hostile, drawn on to the character: dq0steptrap's;
        // game 1's pit its faction's own, its bit in the pit's knowers)
        const ahead = g => (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j)), T2 = S.trap[g];
          if (k === undefined || !T2) return;
          dq0Alone(m, h, L, K, at, k);
          S.kb[g] = 1 << (h[at(k) + L.aFaction] & 7);
          if (g === 1) m.dq0.bank(T2.bi)[T2.r + KNOW] = S.kb[g];   // (This scene's copy of the pit's bank)
          const x = T2.x + HEAD_X[T2.face], y = T2.y + HEAD_Y[T2.face];
          const R = dq0Rec(m), o = R.at(h[at(k) + L.aDen]), w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; }, r16 = (q, v) => { R.mem[o + q] = v & 255; R.mem[o + q + 1] = v >> 8 & 255; };
          w16(at(k) + L.aX, x); w16(at(k) + L.aY, y); h[at(k) + L.aSn] = h[L.pcSn]; h[at(k) + L.aPos] = K.POS_STANDING;
          r16(R.X, x); r16(R.Y, y); R.mem[o + R.SN] = h[L.pcSn];
          h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_HOSTILE;
          h[at(k) + L.aDoing] = 0;
          const px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;
          const ad = dq0Ad(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1')), ram = dq0LaterRam(m);
          ram[ad('drawn') + k] = 8;
          ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
          ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          S.foe[g] = k;
        };
        dq0Scene(m, [1, 2, 3].flatMap(g => [{ g, line: 'wait', n: 1, call: pit(g) }, { g, line: 'wait', n: 2, call: ahead(g) }]));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], S = m.tknow;
        if ([1, 2, 3].some(g => !S.trap[g] || S.foe[g] === undefined)) return ['the scenes not set: ' + JSON.stringify({ trap: Object.keys(S.trap), foe: S.foe })];
        const games = [], g = [];
        for (const t of T) { if (/^dq0 /.test(t.line)) games.push(g.splice(0)); g.push(t); }
        games.push(g.splice(0)); games.shift();
        if (games.length !== 3) return ['the three games not played: ' + games.length];
        const name = ['the bandit (its own pit)', 'the bandit (another\'s pit)', 'the bat'];
        const notes = [];
        games.forEach((gm, i) => {
          const tr = S.trap[i + 1], w = S.foe[i + 1] + 1;
          const asks = (rs, pred) => gm.flatMap(t => t.replies).filter(r => r.rs === rs && r.actor === w && pred(r.ask));
          const n = asks('RS_TEST', q => q.purpose === 'P_NOTICE_SIGHT' && q.how === 1 && q.steps === -20), h = asks('RS_HARM', q => q.cause === 'HC_FALL');
          const ch = gm.flatMap(t => [t.at, t.after].filter(Boolean).flatMap(x => x.changes || [])).filter(c => c.what === 'TRAP_STATE' && c.x === tr.x && c.y === tr.y);
          const moved = gm.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === w && e.de === 'DE_MOVED');
          const at0 = (gm[0].at || {}).changes || [], end = (gm[gm.length - 1].after || {}).changes || [];   // (The change records' cache: a game's own, past its first line's)
          const mine = l => l.filter(c => c.what === 'TRAP_STATE' && c.x === tr.x && c.y === tr.y && (c.tile & 1)).length;
          const on = moved.some(e => e.sx === tr.x && e.sy === tr.y), sprung = mine(end) > mine(at0);
          notes.push(name[i] + ': ' + JSON.stringify({ notice: n.map(r => r.grade), harm: h.length, steps: moved.length, on, changes: ch.map(c => [c.tile, c.elev]) }));
          if (!moved.length) f.push(name[i] + ' never stepped (the scene not played)');
          if (i !== 1) {
            if (n.length) f.push(name[i] + ' asked the notice test at the pit, as one that doesn\'t know of it (or isn\'t over it)');
            if (h.length || sprung) f.push(name[i] + ' fell into the pit');
            if (i === 0 && on) f.push(name[i] + ' stepped onto its own pit (not gone round)');
            return;
          }
          if (!n.length) { f.push(name[i] + ' asked no notice test coming onto another\'s pit'); return; }
          if (n[0].grade > 0) {
            if (h.length) f.push(name[i] + ' noticed the pit, but fell into it');
            if (!ch.some(c => (c.elev & S.kb[2]) && (c.tile & 0x20) && !(c.tile & 1))) f.push(name[i] + '\'s find not kept its faction\'s (TRAP_STATE: its bit in the knowers, still hidden from the character): ' + JSON.stringify(ch));
          } else if (h.length !== 1 || !sprung) f.push(name[i] + ' not fallen into the pit it didn\'t notice');
        });
        // (A lair of folk placed: its traps its faction's, the minds' I_KIT KT_TRAPS)
        const kt = games[0].flatMap(t => t.intents).filter(e => e.kind === 'I_KIT' && (e.draw === 'KT_TRAPS' || e.draw === 0x40));
        notes.push('KT_TRAPS: ' + JSON.stringify(kt.map(e => [e.faction, e.x, e.y])));
        if (!kt.length) f.push('no lair of the Brigands told the world its traps (I_KIT KT_TRAPS)');
        else if (kt.some(e => (1 << (e.faction & 7)) !== S.kb[1])) f.push('a KT_TRAPS not the bandits\' faction: ' + JSON.stringify(kt));
        this.notes = notes;
        return f;
      },
      sdcard: true
    },
    {
      // (The step and its hazards' second round, the user's answers of 2026-11-23: a trap's wound tests courage; a cry heard as any noise)
      name: 'dq0trapheart', outfit: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's creature hurt by a trap, its courage tested as a blow's wound tests it, and its cry told only where it's heard (the user's, 2026-11-23: \"a wound from a trap or a fall tests a creature's courage as a blow's does\"; \"'You hear a cry of pain' is told only where it can be heard, by the senses' hearing like any noise\"): dq0doortrap's scene (seed 1, a theme's Brigands; the character opens and shuts the door ahead), a scything blade, hidden at 10, its power 9, poked onto the door, a hostile bandit put beyond it, its heart poked low (-20 steps: the field and its bonds against it).  It opens the door, the blade goes off at it (P_DODGE_HAZARD), and struck: the referee's harm asked with its heart and its shyness (RS_HARM's x[0], x[1], a blow's RS_ATTACK's), the reply what its courage left it in (FF_FLEES: it flees, DO_FLEE, told 'They flee.' if seen; AF_CAUTIOUS: it holds back); its wound told E_FIGHT FO_HURT with it; its cry an E_DEN of its own (DE_CRY, DE_CRYCUT dead of it), the senses' hearing's to tell ('You hear a cry of pain ...'), not the wound's words unseen",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/h/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k rope,shortsword -S /ram/s.sc', 'open the door', 'close the door', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.theart = {};
        const HEAD_X = [0, 1, 1, 1, 0, -1, -1, -1], HEAD_Y = [-1, -1, 0, 1, 1, 1, 0, -1];
        const [NTRAPS, TRAP, TSIZE] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, ntraps)', 'offsetof (struct chunk, trap)', 'sizeof (struct trap)']);
        const blade = (h, L) => {
          const w = m.dq0.bank(WORLD_BANK_N), sn = h[L.pcSn], st = stratumRec(m, sn), d = h[L.pcFace] & 7;
          const x = (h[L.pcX] | h[L.pcX + 1] << 8) + HEAD_X[d], y = (h[L.pcY] | h[L.pcY + 1] << 8) + HEAD_Y[d], ox = x - st.x0, oy = y - st.y0;
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (!w[sl] || !b || w[sl + 1] !== sn || (w[sl + 2] | w[sl + 3] << 8) !== ox >> 5 || (w[sl + 4] | w[sl + 5] << 8) !== oy >> 5) continue;
            if (b[o + NTRAPS] >= 6) return;
            const q = o + TRAP + b[o + NTRAPS]++ * TSIZE;
            [ox & 31, oy & 31, 4, 2, 0x20, 10, 9, 0].forEach((v, i) => { b[q + i] = v; });   // (TK_BLADE, TT_DOOR, TS_HIDDEN, its hiding, its power, no knowers)
            S.trap = { x, y, sn };
            return;
          }
        };
        const heart = (h, L) => { if (m.door) { h[L.actor + m.door.k * L.actorSize + L.aHeart] = 256 - 20; S.heart = -20; } };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: blade }, { g: 1, line: 'wait', n: 1, call: heart }]);
        dq0DoorScene(m, 'wait', 'bandit');   // (After: its scene set first as the line's typed, then the blade and the heart)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], S = m.theart, s = dq0Prose(out), { K } = m.dq0.layout();
        if (!S.trap || !m.door || S.heart === undefined) return ['the scene not set: ' + JSON.stringify([S, m.doorWhy])];
        const w = m.door.k + 1, all = T.filter(t => t.k >= 0);
        const dodge = all.flatMap(t => t.replies).filter(r => r.rs === 'RS_TEST' && r.actor === w && r.ask.purpose === 'P_DODGE_HAZARD');
        const h = all.flatMap(t => t.replies).filter(r => r.rs === 'RS_HARM' && r.actor === w);
        const fe = all.flatMap(t => t.dens).filter(e => e.kind === 'E_FIGHT' && e.who === w && /HURT|252/.test(String(e.out)));
        const cry = all.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === w && /^DE_CRY/.test(String(e.de)));
        this.notes = [JSON.stringify({ dodge: dodge.map(r => r.grade), harm: h.map(r => [r.ask.heart, r.ask.shy, r.reply.band, r.reply.out, r.reply.left, r.reply.af]),
          fight: fe.map(e => e.ff), cry: cry.map(e => [e.de, e.seen, e.loud]) })];
        if (!dodge.length) return ['the bandit not asked to dodge the blade (the door not opened by it)'];
        if (dodge[0].grade > 0) { if (h.length) f.push('the blade dodged, but its harm asked'); return f; }
        if (h.length !== 1) return ['the bandit\'s strike not asked once: ' + h.length];
        const r = h[0];
        if ((r.ask.heart << 24 >> 24) !== S.heart) f.push('its harm not asked with its heart (' + S.heart + '): ' + r.ask.heart);
        if (!r.reply.band) return f;
        if (!fe.length) f.push('its wound not told (E_FIGHT FO_HURT)');
        const flees = (r.reply.left & 0x10) !== 0;
        if (fe.length && ((fe[0].ff & 0x10) !== 0) !== flees) f.push('its wound\'s telling not what its courage left it in: ' + JSON.stringify([fe[0].ff, r.reply.left]));
        if (flees) {
          const later = all.filter(t => t.at && t.at.dens).flatMap(t => t.at.dens).some(d => d.doing === 'DO_FLEE' || d.doing === K.DO_FLEE);
          this.notes.push('it fled: ' + later);
        }
        if (cry.length !== 1) f.push('its cry not an E_DEN of its own, once: ' + cry.length);
        else {
          const dead = fe.length && (fe[0].ff & 0x02);
          if ((cry[0].de === 'DE_CRYCUT') !== !!dead) f.push('its cry not cut short as it died, or cut short alive: ' + JSON.stringify([cry[0].de, fe[0] && fe[0].ff]));
          this.notes.push('its cry ' + (cry[0].seen ? 'seen' : /You hear (a |a muffled |muffled )?cry of pain/.test(s) ? 'heard' : 'not heard (by the senses\' rule: its loudness, the background, the hearing)'));
        }
        if (/You hear a cry of pain\.|that stops short\./.test(s)) f.push('the old words of a cry told, the wound\'s unseen: ' + s.slice(s.indexOf('cry of pain') - 100, s.indexOf('cry of pain') + 100));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pitstep', meaning: true, near: ['map', 'body'], what: "Dingus Quest Zero's climb out of a pit ending with a step onto its edge, its time, for every actor (the user's, 2026-11-23: \"climbing out of a pit ends with a step onto its edge for every actor: the character pays the step's time too\"; a denizen's, wstep.c's, its I_STEP's): dq0stairtrap's scene (seed 38: the walk to the stairs stopped on a covered pit, fallen in, a slip back; climbed out again): the last round's time, from its E_CLIMB before to the K_UP, a round's and a step's more, where a round of the climb before it was a round's alone",
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 38 -c /ram/c.hm', 'se', 'walk to the door east',
        'u', 'climb up', 'climb up', 'u', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const t = T.filter(u => u.k >= T.of('u').k).find(u => u.events.some(e => e.kind === 'E_CLIMB' && e.how === 'K_UP'));
        if (!t) return ['not climbed out of the pit (no K_UP)'];
        const up = t.events.find(e => e.kind === 'E_CLIMB' && e.how === 'K_UP'), a = t.after;
        const step = a && a.pc ? a.pc.ready - up.t : null;
        this.notes = ['out at ' + up.t + ', free at ' + (a && a.pc && a.pc.ready) + ': its step ' + step];
        if (step === null) return ['no state after the climb out'];
        if (step < 5 || step > 40) f.push('the climb out not ended by a step\'s time (5 to 40 tenths) after it: ' + step);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pcrob', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's character robbing and filching a living denizen (ONE-PATH.md's 2.7: the one-path plan's stage 2, things and robbery, 2026-11-23; the denizens' own robbery the other way about): at seed 1 (dq0denportal's wary bandits) a bandit put beside the character, senseless: 'rob the bandit' (SW_ROB, the minds' robbed: its I_ROB FE_ROBBED, wthings.c's) goes through its things, what it holds and its coins taken into the character's hands, pack or purse by the character's carry, each told ('You take the bandit's ...'), the bandit's record no longer holding them; and a bandit put beside it asleep: 'filch from the bandit' (SW_FILCH) tests the character's Legerdemain against its hearing (the referee's P_FILCH, actor 0, the user's, 2026-11-23; P_NOTICE_SOUND, its own sheet), its coins taken if the better is the character's, else caught ('wakes as your hand finds its purse'), as the tests went; a bandit awake refuses it",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k sack -S /ram/s.sc", 'wait', 'rob the bandit', 'i', 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k sack -S /ram/s.sc", 'wait', 'filch from the bandit', 'wait', 'filch from the bandit', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.prob = { k: {}, did: {} };
        const u16 = (b, o) => b[o] | b[o + 1] << 8;
        const put = g => (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          dq0Alone(m, h, L, K, at, k);
          S.k[g] = k;
          S.did[g] = u16(h, at(k) + L.aDid);
          const R = dq0Rec(m), q = R.at(h[at(k) + L.aDen]);
          h[at(k) + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_INDIFFERENT;
        };
        const far = (h, L, k) => { const v = 3000 + (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16), o = L.actor + k * L.actorSize + L.aReady; for (let i = 0; i < 4; i++) h[o + i] = v >>> (8 * i) & 255; };
        const asleep = (h, L, K, at) => { if (S.k[2] !== undefined) { h[at(S.k[2]) + L.aDoing] = K.DO_SLEEP; far(h, L, S.k[2]); h[L.denSeq] = h[L.denSeq] + 1 & 255; } };
        // (Its dagger in its hand: out of its sheath, held, its own: what a robbery takes besides its coins)
        const armed = (h, L, K) => {
          if (S.k[1] === undefined) return;
          const per = Math.floor(0x2000 / L.itemSize), did = S.did[1], rec = i => { const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize; return { b, o }; };
          let home = null;
          for (let i = 1; i < L.nitems && !home; i++) { const { b, o } = rec(i); if (b && b[o + L.iKind] && b[o + L.iWhere] === K.IW_DEN && (b[o + L.iOwner] | b[o + L.iHide] << 8) === did) home = [b[o + L.iSn], u16(b, o + L.iX), u16(b, o + L.iY)]; }
          for (let i = 1; i < L.nitems && home; i++) {
            const { b, o } = rec(i);
            if (!b || b[o + L.iKind] !== K.KI_DAGGER || b[o + L.iWhere] !== K.IW_ITEM) continue;
            let j = u16(b, o + L.iX), r = rec(j), n = 0;
            while (r.b && r.b[r.o + L.iWhere] === K.IW_ITEM && n++ < 6) { j = u16(r.b, r.o + L.iX); r = rec(j); }
            if (!r.b || r.b[r.o + L.iWhere] !== K.IW_DEN || (r.b[r.o + L.iOwner] | r.b[r.o + L.iHide] << 8) !== did) continue;
            b[o + L.iWhere] = K.IW_DEN; b[o + L.iOwner] = did & 255; b[o + L.iHide] = did >> 8; b[o + L.iFlags] |= 2;
            b[o + L.iSn] = home[0]; b[o + L.iX] = home[1] & 255; b[o + L.iX + 1] = home[1] >> 8; b[o + L.iY] = home[2] & 255; b[o + L.iY + 1] = home[2] >> 8;
            S.armed = i;
            break;
          }
        };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: put(1) }, { g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face', senseless: 3000 }, { g: 1, line: 'wait', n: 1, call: armed },
          { g: 2, line: 'wait', n: 1, call: put(2) }, { g: 2, line: 'wait', n: 1, name: 'bandit', beside: 'face' },
          { g: 2, line: 'wait', n: 1, call: asleep }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], { K } = m.dq0.layout(), S = m.prob, s = dq0Prose(out), G = s.split('% dq0 ').slice(1);
        if (S.k[1] === undefined || S.k[2] === undefined) return ['the scenes not set: ' + JSON.stringify(S)];
        const rob = T.of('rob the bandit'), r1 = T.after(rob, 'i');
        if (!S.armed) f.push('the bandit not armed for the scene');
        const theirs = (t, g) => t && t.at ? t.at.items.filter(i => dq0Root(t.at, i).where === 'IW_DEN' && dq0Root(t.at, i).did === S.did[g]) : [];
        const had = theirs(rob, 1).filter(i => ['pennies', 'farthings', 'crowns'].includes(i.name) || (i.where === 'IW_DEN' && (i.flags & DQ0_I.HELD) && !(i.flags & 1)));
        if (!/You go through the bandit's things\./.test(G[0])) f.push('the robbery not told: ' + G[0].slice(G[0].indexOf('> rob'), G[0].indexOf('> rob') + 300));
        const after = r1.after || r1.at, mine = dq0Carried(after).map(i => i.n);
        const left = had.filter(i => !mine.includes(i.n) && !(after && after.items.some(j => j.n === i.n && j.where === 'IW_SQUARE')));
        if (!had.length) f.push('the bandit had nothing to rob: ' + JSON.stringify(theirs(rob, 1).map(i => [i.name, i.where, i.flags])));
        if (left.length) f.push('robbed, its things left with it: ' + JSON.stringify(left.map(i => i.name)));
        for (const i of had) if (!new RegExp('You take the bandit\'s ' + i.name).test(G[0])) f.push('not told taking its ' + i.name);
        // (The filch: the character's Stealth against its hearing, as the tests went)
        const fi = T.of('filch from the bandit', T.of('quit').k + 1), reps = fi.replies.filter(q => q.kind === K.RS_TEST);
        const P_FILCH = K.P_FILCH, P_NOTICE_SOUND = 1;   // (Its Legerdemain, the user's, 2026-11-23: its Stealth before)
        const st = reps.find(q => q.actor === 0 && q.a === P_FILCH), no = reps.find(q => q.actor === S.k[2] + 1 && q.a === P_NOTICE_SOUND);
        if (!st || !no) f.push('the filch not the character\'s Legerdemain against its hearing: ' + JSON.stringify(reps.map(q => [q.actor, q.a, q.grade])));
        else {
          const caught = no.grade > st.grade, g2 = G[1].slice(G[1].indexOf('> filch'));
          if (caught !== /wakes as your hand finds its purse/.test(g2)) f.push('caught ' + caught + ' but told: ' + g2.slice(0, 200));
          if (!caught && !/You slip a hand into the bandit's purse\./.test(g2)) f.push('not caught, the filch not told: ' + g2.slice(0, 200));
          if (!caught && /You take the bandit's (?!pennies|farthings|crowns)/.test(g2.slice(0, g2.indexOf('> wait')))) f.push('a filch taking more than coins: ' + g2.slice(0, 300));
          const again = G[1].slice(G[1].lastIndexOf('> filch'));
          if (caught && !/is awake: it would feel your hand\./.test(again)) f.push('a bandit awake not refusing a filch: ' + again.slice(0, 200));
          this.notes = ['filch grades ' + JSON.stringify([st.grade, no.grade])];
        }
        return f;
      },
      sdcard: true
    },
    {
      // (Things and robbery, the user's decisions of 2026-11-23: robbing one held or asleep; a filch the character's
      //   Legerdemain; one that catches it, its attitude moved by its nature)
      name: 'dq0robheld', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's character robbing one held or asleep, as well as one senseless (the user's, 2026-11-23: DECISIONS.md's stage 2's questions, first round), a filch its Legerdemain against the victim's hearing, and one that catches it moved against it by its own nature: at seed 1 (dq0pcrob's wary bandits) a bandit put beside the character, awake and free: 'rob the bandit' refused ('isn't helpless'); the same bandit held (the world's grip and the hub's pc_grip, the character's grapple's, poked): 'rob the bandit' goes through its things (its I_ROB FE_ROBBED let through the hold, not made a try to break free), what it holds and its coins taken, each told; a bandit asleep beside it, robbed, each game its own (the character's Legerdemain 5, at two of the dice's offsets, -R 1 and 2, then 95), caught in one game at least and not in another: the character's Legerdemain (the referee's P_FILCH, actor 0) against its hearing (P_NOTICE_SOUND, its own sheet), the better caught: 'wakes as you go through their things!', its attitude two steps the worse (a bandit neither shy nor bold nor a killer: kinds.txt's ways fr), told ('They seem ... now'); else its things taken, its attitude as it was",
      init: 't_rc', cycles: 5000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['cp /ram/c.hm /ram/d.hm', 'echo ml leg 5 >> /ram/c.hm',
        'echo ml leg 95 >> /ram/d.hm', 'mkdir /ram/g', 'mkdir /ram/h1', 'mkdir /ram/h2', 'mkdir /ram/i', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", ...['h1', 'h2', 'i'].map(d => 'cp /ram/g/theme.txt /ram/' + d + '/theme.txt'),
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k sack -S /ram/s.sc", 'wait', 'rob the bandit', 'wait', 'rob the bandit', 'quit',
        ...[1, 2].flatMap(r => ["dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h" + r + " -k sack -S /ram/s.sc -R " + r, 'wait', 'rob the bandit', 'wait', 'quit']),
        "dq0 -p t -s 1 -c /ram/d.hm -T -d /ram/i -k sack -S /ram/s.sc", 'wait', 'rob the bandit', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.prob = { k: {}, did: {} };
        const u16 = (b, o) => b[o] | b[o + 1] << 8;
        const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0w', 'dq0w.lbl'), 'latin1');
        const grip = parseInt(/^al ([0-9A-F]+) \._grip$/m.exec(lbl)[1], 16);
        const far = (h, L, k) => { const v = 3000 + (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16), o = L.actor + k * L.actorSize + L.aReady; for (let i = 0; i < 4; i++) h[o + i] = v >>> (8 * i) & 255; };
        const put = g => (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          dq0Alone(m, h, L, K, at, k);
          S.k[g] = k;
          S.did[g] = u16(h, at(k) + L.aDid);
          const R = dq0Rec(m), q = R.at(h[at(k) + L.aDen]);
          h[at(k) + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_INDIFFERENT;
        };
        const held = (h, L) => {                    // (Held: the character's grapple's hold, the world's and the hub's)
          if (S.k[1] === undefined) return;
          h[L.pcGrip] = S.k[1] + 1;
          dq0LaterRam(m, 1)[grip] = S.k[1] + 1;
          far(h, L, S.k[1]);
        };
        const asleep = g => (h, L, K, at) => { if (S.k[g] !== undefined) { h[at(S.k[g]) + L.aDoing] = K.DO_SLEEP; far(h, L, S.k[g]); h[L.denSeq] = h[L.denSeq] + 1 & 255; } };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: put(1) }, { g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face' }, { g: 1, line: 'wait', n: 2, call: held },
          ...[2, 3, 4].flatMap(g => [{ g, line: 'wait', n: 1, call: put(g) }, { g, line: 'wait', n: 1, name: 'bandit', beside: 'face' }, { g, line: 'wait', n: 1, call: asleep(g) }])]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], { K } = m.dq0.layout(), S = m.prob, s = dq0Prose(out), G = s.split('% dq0 ').slice(1), notes = [];
        if ([1, 2, 3, 4].some(g => S.k[g] === undefined) || G.length < 4) return ['the scenes not set: ' + JSON.stringify(S)];
        // (Awake and free: refused; held: robbed, its robbery not made a try to break free)
        const g1 = G[0], r1 = g1.indexOf('> rob the bandit'), r2 = g1.lastIndexOf('> rob the bandit');
        if (!/isn't helpless/.test(g1.slice(r1, r2))) f.push('one awake and free not refused: ' + g1.slice(r1, r1 + 200));
        const held = g1.slice(r2);
        if (!/You go through the bandit's things\./.test(held) || !/You take the bandit's |You find nothing on the bandit worth the taking\./.test(held)) f.push('one held not robbed: ' + held.slice(0, 300));
        const rb = T.filter(t => t.game === 0 && t.line === 'rob the bandit')[1];
        const brk = rb ? T.filter(t => t.game === 0 && t.k >= rb.k).flatMap(t => t.dens || []).filter(e => e.kind === 'E_FIGHT' && e.who === S.k[1] + 1) : [];
        if (!rb) f.push('no second robbery in the first game');
        if (brk.length) f.push('the robbery of one held made its try to break free: ' + JSON.stringify(brk));
        // (Asleep: its Legerdemain against its hearing; caught, moved by its nature)
        const seen = [];
        for (const g of [2, 3, 4]) {
          const q = T.filter(t => t.game === g - 1 && t.line === 'rob the bandit')[0];
          if (!q) { f.push('game ' + g + ': no robbery'); continue; }
          const reps = q.replies.filter(r => r.kind === K.RS_TEST), lg = reps.find(r => r.actor === 0 && r.a === K.P_FILCH), hr = reps.find(r => r.actor === S.k[g] + 1 && r.a === K.P_NOTICE_SOUND);
          const w = G[g - 1].slice(G[g - 1].indexOf('> rob the bandit'));
          if (!lg || !hr) { f.push('game ' + g + ': not its Legerdemain against its hearing: ' + JSON.stringify(reps.map(r => [r.actor, r.a, r.grade]))); continue; }
          const caught = hr.grade > lg.grade, a = q.after && q.after.actors.find(x => x.k === S.k[g]);
          seen.push(caught);
          notes.push('game ' + g + ': Legerdemain ' + lg.grade + ', hearing ' + hr.grade + (caught ? ', caught' : ', not caught'));
          if (caught) {
            if (!/wakes as you go through their things! They seem unfriendly now\./.test(w)) f.push('game ' + g + ': caught, not told so: ' + w.slice(0, 250));
            if (!a || a.attitude !== 'AT_UNFRIENDLY') f.push('game ' + g + ': caught, its attitude not two steps the worse (indifferent to unfriendly): ' + JSON.stringify(a && a.attitude));
          } else {
            if (!/You go through the bandit's things\./.test(w)) f.push('game ' + g + ': not caught, the robbery not told: ' + w.slice(0, 250));
            if (!a || a.attitude !== 'AT_INDIFFERENT') f.push('game ' + g + ': not caught, its attitude moved: ' + JSON.stringify(a && a.attitude));
          }
        }
        if (!seen.includes(true) || !seen.includes(false)) f.push('the robbery of a sleeper not both caught and not, as the dice went: ' + JSON.stringify(seen));
        this.notes = notes;
        return f;
      },
      sdcard: true
    },
    {
      // (The minds use the new errands: the user's, 2026-11-23, things1's question 1)
      name: 'dq0denerrands', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's denizens using their things by the character's rules (the user's, 2026-11-23: DECISIONS.md's stage 2's questions, first round: a denizen wears armour it finds that's better, stows its loot, gives a fellow food, a weapon or a light it needs, and drags furniture to brace a door), at seed 1 (dq0thingact's wary bandits).  The world's choice (I_ROB's thing 0, wthings.c's choose), its intents posted as its mind would: a bandit made the owner of a mail hauberk (hung at its belt, not worn), the character's crowns and dagger in its hands and two of its torches: FE_DON 0 wears the hauberk (worth more than any armour it wears: the don row's time), FE_STOW 0 puts the crowns in its pouch (its loot held), FE_HAND 0 to a fellow beside it with PN_ARMS hands it a spare weapon (one not in hand, got out first: two of its own at least) and with PN_LIGHT a torch (one of two, not lit).  Its mind's own: a bandit with a spare dagger of its own, a mail hauberk lying at its feet and a fellow beside it with no weapon: its look posts FE_DON 0, FE_STOW 0 and FE_PASS PN_ARMS (handed over), and takes the hauberk up (I_TAKE's TW_ARMOUR: better than it wears), wearing it at its next look.  A brace: a bandit on the square inside a shut door, a cask beside it: FE_DRAG 0 drags the cask onto its square, as its test went (a creature's test by its own dice: in this quiet scene its strength failed it at each of -R 0 to 3) (the drag row's test, P_FORCE by its weight, told as heard, E_DEN DE_DRAGS): the door braced",
      init: 't_rc', cycles: 6000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'mkdir /ram/i', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt', 'cp /ram/g/theme.txt /ram/i/theme.txt',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'dagger,mail hauberk,crowns' -S /ram/s.sc",
        'wait', 'drop the dagger', 'drop the mail hauberk', 'drop the crowns', 'step south', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k 'dagger,mail hauberk' -S /ram/s.sc",
        'wait', 'drop the dagger', 'drop the mail hauberk', 'step south', ...Array(10).fill('wait 1 minute'), 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/i -k cask -S /ram/s.sc", 'close the door', 'go north', 'wait', 'drop the cask', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.errs = { p0: {}, a: {}, b: {}, did: {}, items: {}, ready: [], brace: null };
        const u16 = (b, o) => b[o] | b[o + 1] << 8, w16 = (b, o, v) => { b[o] = v & 255; b[o + 1] = v >> 8 & 255; };
        const store = (L, fn) => { const per = Math.floor(0x2000 / L.itemSize); for (let i = 1; i < L.nitems; i++) { const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize; if (b && b[o + L.iKind] && fn(i, b, o) === false) return; } };
        const rd = (h, L, k) => { const o = L.actor + k * L.actorSize + L.aReady; return (h[o] | h[o + 1] << 8 | h[o + 2] << 16 | h[o + 3] << 24) >>> 0; };
        const far = (h, L, k) => { const v = 3000 + (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16), o = L.actor + k * L.actorSize + L.aReady; for (let i = 0; i < 4; i++) h[o + i] = v >>> (8 * i) & 255; };
        const tileAt = (h, L, sn) => { const st = stratumRec(m, sn); return (x, y) => { const c = st && liveChunks(m, sn)[((x - st.x0) >> 5) + ',' + ((y - st.y0) >> 5)]; return c ? c[443 + ((y - st.y0) & 31) * 32 + ((x - st.x0) & 31)] & 15 : -1; }; };
        const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ad = dq0Ad(lbl);
        const ramOf = () => dq0LaterRam(m);   // (The minds' own, as the scene's in place)
        const here = g => (h, L) => { S.p0[g] = [u16(h, L.pcX), u16(h, L.pcY), h[L.pcSn]]; };
        // (An actor put on square x, y: the hub's and its record's)
        const place = (h, L, K, at, k, x, y, sn, att) => {
          const R = dq0Rec(m), o = at(k), q = R.at(h[o + L.aDen]);
          w16(h, o + L.aX, x); w16(h, o + L.aY, y); h[o + L.aSn] = sn; h[o + L.aState] = 1; h[o + L.aPos] = K.POS_STANDING; h[o + L.aDoing] = K.DO_WAIT;
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = att;
          R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8; R.mem[q + R.SN] = sn;
        };
        const own = (L, K, b, o, did, f) => { b[o + L.iWhere] = K.IW_DEN; b[o + L.iOwner] = did & 255; b[o + L.iHide] = did >> 8; b[o + L.iFlags] = (b[o + L.iFlags] & ~(2 | 4 | 16)) | f; };
        const hands = (L, K, did, f) => store(L, (i, b, o) => { if (b[o + L.iWhere] === K.IW_DEN && (b[o + L.iOwner] | b[o + L.iHide] << 8) === did) f(i, b, o); });
        // (Two bandits: A on the square the character left, B beside it, its hands empty; the rest away)
        const two = (g, att, quiet) => (h, L, K, at, named) => {
          const ks = [...Array(L.nactors).keys()].filter(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (ks.length < 2 || !S.p0[g]) return;
          const [a, b] = ks, [x, y, sn] = S.p0[g], T = tileAt(h, L, sn), px = u16(h, L.pcX), py = u16(h, L.pcY);
          const side = [[1, 0], [-1, 0], [0, -1], [1, -1], [-1, -1]].find(([dx, dy]) => [1, 4].includes(T(x + dx, y + dy)) && !(x + dx === px && y + dy === py));
          if (!side) return;
          dq0Alone(m, h, L, K, at, [a, b]);
          place(h, L, K, at, a, x, y, sn, K[att]);
          place(h, L, K, at, b, x + side[0], y + side[1], sn, K[att]);
          if (quiet) far(h, L, a);
          far(h, L, b);
          S.a[g] = a; S.b[g] = b; S.did[g] = [u16(h, at(a) + L.aDid), u16(h, at(b) + L.aDid)];
          hands(L, K, S.did[g][0], (i, b2, o) => { b2[o + L.iFlags] &= ~2; });   // (Their hands emptied: what they held hung at their belts)
          hands(L, K, S.did[g][1], (i, b2, o) => { b2[o + L.iFlags] &= ~2; });
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        // (Game 1: the things laid down made A's: the dagger and the crowns in hand, the hauberk hung, two of the character's torches)
        const kit1 = (h, L, K) => {
          if (S.a[1] === undefined) return;
          const did = S.did[1][0], [x, y, sn] = S.p0[1];
          let torches = 0;
          // (The two bandits' own torches, their outfits' since 2026-11-23 (build2: bandits carry a torch and a tinderbox,
          // a pouch of their own), made rations, as dq0lightden's are: A's torches the scene's two alone, B with no light
          // of its own; and the two the character's, none of a denizen's.  room8, 2026-11-24: with its kit's torch A
          // handed B that one and kept both of the scene's)
          const recs = new Map();
          store(L, (i, b, o) => { recs.set(i, [b, o]); });
          const rootDid = i => {
            let j = i;
            for (let n = 0; n < 8 && recs.has(j) && recs.get(j)[0][recs.get(j)[1] + L.iWhere] === K.IW_ITEM; n++) j = u16(recs.get(j)[0], recs.get(j)[1] + L.iX);
            const r = recs.get(j);
            return r && r[0][r[1] + L.iWhere] === K.IW_DEN ? r[0][r[1] + L.iOwner] | r[0][r[1] + L.iHide] << 8 : -1;
          };
          for (const [i, [b, o]] of recs) if (b[o + L.iKind] === K.KI_TORCH && S.did[1].includes(rootDid(i))) b[o + L.iKind] = K.KI_RATIONS;
          store(L, (i, b, o) => {
            const kd = b[o + L.iKind];
            if (b[o + L.iWhere] === K.IW_SQUARE && b[o + L.iSn] === sn && u16(b, o + L.iX) === x && u16(b, o + L.iY) === y) {
              if (kd === K.KI_DAGGER) { own(L, K, b, o, did, 2); S.items.dagger = i; }
              if (kd === K.KI_CROWNS) { own(L, K, b, o, did, 2); S.items.crowns = i; }
              if (kd === K.KI_MAIL_HAUBERK) { own(L, K, b, o, did, 0); S.items.hauberk = i; }
            } else if (kd === K.KI_TORCH && torches < 2 && !(b[o + L.iFlags] & 1) && b[o + L.iWhere] === K.IW_ITEM && rootDid(i) < 0) {
              own(L, K, b, o, did, 0); S.items['torch' + torches++] = i;
            }
          });
          // (Its own hauberk-worth armour none: worn armour of its outfit taken off, the hauberk the better)
          hands(L, K, did, (i, b, o) => { if (i !== S.items.hauberk && b[o + L.iKind] === K.KI_MAIL_HAUBERK) b[o + L.iFlags] &= ~4; });
        };
        const post = (what, f) => (h, L, K) => { if (S.a[1] === undefined || !S.items.hauberk) return; S.ready.push([what, rd(h, L, S.a[1])]); dq0Intent(h, L, K.I_ROB, S.a[1], f(K)); };
        const mark = (h, L) => { if (S.a[1] !== undefined) S.ready.push(['end', rd(h, L, S.a[1])]); };
        // (Game 2: A's mind about: a dagger of its own spare, the hauberk lying at its feet, B unarmed; its look now, and again later)
        const kit2 = (h, L, K) => {
          if (S.a[2] === undefined) return;
          const [did, bdid] = S.did[2], [x, y, sn] = S.p0[2];
          store(L, (i, b, o) => {
            if (b[o + L.iWhere] === K.IW_SQUARE && b[o + L.iSn] === sn && u16(b, o + L.iX) === x && u16(b, o + L.iY) === y && b[o + L.iKind] === K.KI_DAGGER) { own(L, K, b, o, did, 0); S.items.dagger2 = i; }
            if (b[o + L.iKind] === K.KI_MAIL_HAUBERK && b[o + L.iWhere] === K.IW_SQUARE) S.items.hauberk2 = i;
          });
          hands(L, K, bdid, (i, b, o) => { if ([K.KI_KNIFE, K.KI_DAGGER, K.KI_SHORTSWORD, K.KI_SPEAR, K.KI_CLUB, K.KI_HATCHET, K.KI_FALCHION, K.KI_SWORD].includes(b[o + L.iKind])) {
            b[o + L.iWhere] = K.IW_SQUARE; b[o + L.iSn] = sn; w16(b, o + L.iX, 32706); w16(b, o + L.iY, 32790); b[o + L.iOwner] = b[o + L.iHide] = 0; } });
          h[L.agear + S.b[2]] &= ~K.AG_ARMED;
          const ram = ramOf(h), now = h[L.now + 2];
          ram[ad('aloot') + S.a[2]] = 0;
          ram[ad('aate') + S.a[2]] = now;
          ram[ad('aate') + S.b[2]] = now;
        };
        const again = (h, L) => { if (S.a[2] !== undefined) ramOf(h)[ad('aloot') + S.a[2]] = 0; };
        // (Game 3: a door near (the start cell's has a wall beside the square inside it), the square inside it, a square
        //   beside that one, room behind: A there, the cask beside it; the door shut for the scene, its cached chunk's)
        const door = (h, L, K, at, named) => {
          const px = u16(h, L.pcX), py = u16(h, L.pcY), sn = h[L.pcSn], T = tileAt(h, L, sn), ok = (x, y) => [1, 4].includes(T(x, y)) && !(x === px && y === py);
          for (let dy = -16; dy <= 16 && !S.brace; dy++) for (let dx = -16; dx <= 16 && !S.brace; dx++) {
            const dX = px + dx, dY = py + dy, p = T(dX, dY) === 3 && dq0Portal(m, sn, dX, dY);
            if (!p || p.kind !== K.PK_DOOR) continue;
            for (const [ix, iy] of [[0, 1], [0, -1], [1, 0], [-1, 0]]) {
              const sx = dX + ix, sy = dY + iy;
              if (!ok(sx, sy) || !ok(dX - ix, dY - iy)) continue;
              for (const [cx, cy] of ix ? [[0, 1], [0, -1]] : [[1, 0], [-1, 0]]) if (ok(sx + cx, sy + cy) && ok(sx - cx, sy - cy)) { S.brace = { door: [dX, dY], s: [sx, sy], c: [sx + cx, sy + cy], sn }; p.b[p.state] &= ~1; break; }
              if (S.brace) break;
            }
          }
        };
        const scene3 = (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined || !S.brace) return;
          dq0Alone(m, h, L, K, at, k);
          place(h, L, K, at, k, S.brace.s[0], S.brace.s[1], S.brace.sn, K.AT_INDIFFERENT);
          far(h, L, k);
          S.a[3] = k;
          store(L, (i, b, o) => { if (b[o + L.iKind] === K.KI_CASK && b[o + L.iWhere] === K.IW_SQUARE) { w16(b, o + L.iX, S.brace.c[0]); w16(b, o + L.iY, S.brace.c[1]); S.items.chest = i; return false; } });
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          dq0Intent(h, L, K.I_ROB, k, { a: K.FE_DRAG, t: 0, w2: 1 });
        };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: here(1) }, { g: 1, line: 'wait', n: 2, call: two(1, 'AT_INDIFFERENT', true) }, { g: 1, line: 'wait', n: 2, call: kit1 },
          { g: 1, line: 'wait', n: 2, call: post('don', K => ({ a: K.FE_DON, t: 0 })) },
          { g: 1, line: 'wait', n: 3, call: post('stow', K => ({ a: K.FE_STOW, t: 0 })) },
          { g: 1, line: 'wait', n: 4, call: post('arms', K => ({ a: K.FE_HAND, t: 0, w1: S.b[1] + 1, w2: K.PN_ARMS })) },
          { g: 1, line: 'wait', n: 5, call: post('light', K => ({ a: K.FE_HAND, t: 0, w1: S.b[1] + 1, w2: K.PN_LIGHT })) },
          { g: 1, line: 'wait', n: 6, call: mark },
          { g: 2, line: 'wait', n: 1, call: here(2) }, { g: 2, line: 'wait 1 minute', n: 1, call: two(2, 'AT_FRIENDLY', false) }, { g: 2, line: 'wait 1 minute', n: 1, call: kit2 },
          { g: 2, line: 'wait 1 minute', n: 5, call: again },
          { g: 3, line: 'wait', n: 1, call: door }, { g: 3, line: 'wait', n: 2, call: scene3 }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout(), S = m.errs, notes = [];
        const last = g => T.filter(t => t.game === g - 1 && t.line === 'quit')[0], it = (g, n) => { const q = last(g); return q && q.at && q.at.items.find(i => i.n === n); };
        const root = (g, i) => dq0Root(last(g).at, i);
        // (Game 1: the world's choice, each its row's time)
        if (S.a[1] === undefined || !S.items.hauberk || !S.items.crowns || S.ready.length < 5) f.push('game 1: the scene not set: ' + JSON.stringify(S));
        else {
          const r = Object.fromEntries(S.ready.map(([w, v], i) => [w, i + 1 < S.ready.length ? S.ready[i + 1][1] - v : 0])), [ad, bd] = S.did[1];
          notes.push('game 1 times ' + JSON.stringify(r));
          const hb = it(1, S.items.hauberk), cr = it(1, S.items.crowns);
          if (!hb || hb.where !== 'IW_DEN' || hb.did !== ad || !(hb.flags & DQ0_I.WORN)) f.push('FE_DON 0: the hauberk not worn: ' + JSON.stringify(hb));
          if (r.don < A.don.t) f.push('FE_DON 0: not the don row\'s time (' + A.don.t + '): ' + r.don);
          const pouch = cr && cr.where === 'IW_ITEM' && last(1).at.items.find(i => i.n === cr.x);
          if (!pouch || pouch.name !== 'pouch' || root(1, cr).did !== ad) f.push('FE_STOW 0: the crowns not put in its pouch: ' + JSON.stringify([cr, pouch]));
          if (r.stow < A.put.t) f.push('FE_STOW 0: not the put row\'s time (' + A.put.t + '): ' + r.stow);
          const bh = last(1).at.items.filter(i => i.where === 'IW_DEN' && i.did === bd && (i.flags & DQ0_I.HELD));
          if (!bh.some(i => ['knife', 'dagger', 'shortsword', 'spear', 'club', 'hatchet', 'falchion', 'sword', 'shortbow'].includes(i.name))) f.push('FE_HAND 0 PN_ARMS: no weapon handed the fellow: ' + JSON.stringify(bh.map(i => i.name)));
          if (!bh.some(i => i.name === 'torch')) f.push('FE_HAND 0 PN_LIGHT: no torch handed the fellow: ' + JSON.stringify(bh.map(i => i.name)));
          const at = [S.items.torch0, S.items.torch1].map(n => it(1, n)).filter(Boolean);
          if (at.filter(i => i.did === ad).length !== 1) f.push('PN_LIGHT: one of its two torches not kept: ' + JSON.stringify(at.map(i => [i.where, i.did])));
          if (r.arms < A.give.t + A.takeout.t) f.push('FE_HAND 0: not got out and given (' + (A.give.t + A.takeout.t) + '): ' + r.arms);
        }
        // (Game 2: its mind's own)
        if (S.a[2] === undefined || !S.items.dagger2 || !S.items.hauberk2) f.push('game 2: the scene not set: ' + JSON.stringify([S.a[2], S.items]));
        else {
          const g2 = T.filter(t => t.game === 1), mi = g2.flatMap(t => t.intents).filter(e => e.ring === 'den' && e.kind === 'I_ROB' && e.who === S.a[2] + 1);
          const er = e => mi.filter(i => i.raw.a === e);
          if (!er(K.FE_DON).some(i => !i.raw.t) || !er(K.FE_STOW).some(i => !i.raw.t)) f.push('game 2: its look not its FE_DON 0 and FE_STOW 0: ' + JSON.stringify(mi.map(i => [i.raw.a, i.raw.t])));
          if (!er(K.FE_PASS).some(i => i.raw.t === K.PN_ARMS && i.raw.b === S.b[2] + 1)) f.push('game 2: its fellow\'s want of a weapon not seen: ' + JSON.stringify(mi.map(i => [i.raw.a, i.raw.b, i.raw.t])));
          const [ad, bd] = S.did[2], hb = it(2, S.items.hauberk2), bh = last(2).at.items.filter(i => i.where === 'IW_DEN' && i.did === bd && (i.flags & DQ0_I.HELD));
          if (!bh.length) f.push('game 2: no weapon handed its fellow');
          if (!hb || root(2, hb).did !== ad) f.push('game 2: the hauberk lying not taken up (better than it wears): ' + JSON.stringify(hb));
          else if (!(hb.flags & DQ0_I.WORN)) f.push('game 2: the hauberk taken up, not worn at its next look: ' + JSON.stringify(hb));
        }
        // (Game 3: the brace)
        if (S.a[3] === undefined || !S.brace || !S.items.chest) f.push('game 3: the scene not set: ' + JSON.stringify([S.a[3], S.brace]));
        else {
          const ch = it(3, S.items.chest), g3 = T.filter(t => t.game === 2);
          const de = g3.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.a === A.drag.de && e.who === S.a[3] + 1);
          const ts = g3.flatMap(t => t.replies).filter(q => q.kind === K.RS_TEST && q.actor === S.a[3] + 1 && q.a === A.drag.p);
          const ok = ts.some(q => q.grade > 0);
          if (ts.length !== 1 || de.length !== 1 || ts[0].steps !== A.drag.s - 1)   // (The drag row's steps, a cask's ten pounds a step the worse)
            f.push('game 3: the brace not the drag row\'s (its noise once, its test once at the row\'s steps for its weight): ' + JSON.stringify([de.length, ts.map(q => [q.steps, q.grade])]));
          const on = ch && ch.x === S.brace.s[0] && ch.y === S.brace.s[1];
          if (ok !== !!on) f.push('game 3: the cask ' + (on ? 'dragged' : 'not dragged') + ' onto the square inside the door though its tests went ' + JSON.stringify(ts.map(q => q.grade)) + ': ' + JSON.stringify([ch && [ch.x, ch.y], S.brace]));
          notes.push('brace ' + JSON.stringify([S.brace, ts.map(q => [q.steps, q.grade])]));
        }
        this.notes = notes;
        return f;
      },
      sdcard: true
    },
    {
      // (Climbing and links, every actor's: the one-path plan's stage 2, 2026-11-23, ONE-PATH.md's 2.1 and 2.7, the act audit's A11, G-1)
      name: 'dq0flightact', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's flights between strata crossed by every actor by the same rule (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, climbing and links, 2026-11-23; the act audit's A11 and G-1): a stairway's, a ladder's or a long slope's crossing its time the flight's, the drop between the floors at the actor's own step (acts.h's FLIGHT_T: the character's s_climb, an actor's wlink.c den_flight), told as a step (the act table's flight row).  At seed 1 (dq0denportal's wary bandits; the character begun at a way down, dq0 -T) a bandit put beside the character and made its ally (the scene set as 'look' is typed: the hub's actor and its record; the others sent far off), the character goes down ('d': its time the flight's at its step, and its walk to the flight's foot); the ally comes along down the same flight (its mind's I_CLIMB, as it did) by the character's rule, not put beside it in a second: its time on its ready the flight's at its own step, and its walk to the flight's foot a step a square (three at most), out at the flight's other end or beside it, on the stratum below, told there as its step is (E_DEN DE_MOVED by the flight row's loudness)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'look', 'd', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.flight = { ready: [], dbg: [] }, r32 = (h, o) => (h[o] | h[o + 1] << 8 | h[o + 2] << 16 | h[o + 3] << 24) >>> 0;
        const put = (h, L, K, at, named, sc) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          dq0Alone(m, h, L, K, at, k);
          const sq = dq0FloorBy(m, h, L, 1);
          if (!sq) return;
          const o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; };
          w16(o + L.aX, sq[0]); w16(o + L.aY, sq[1]); h[o + L.aSn] = h[L.pcSn]; h[o + L.aPos] = K.POS_STANDING; h[o + L.aDoing] = K.DO_FOLLOW;
          R.mem[q + R.X] = sq[0] & 255; R.mem[q + R.X + 1] = sq[0] >> 8; R.mem[q + R.Y] = sq[1] & 255; R.mem[q + R.Y + 1] = sq[1] >> 8; R.mem[q + R.SN] = h[L.pcSn];
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_ALLIED;
          for (let i = 0; i < 4; i++) h[o + L.aReady + i] = h[L.hubNow + i];
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          S.k = k; S.step = h[o + L.aStep]; S.sq = sq;
        };
        const ready = (h, L) => { if (S.k !== undefined) S.ready.push({ now: r32(h, L.hubNow), ready: r32(h, L.actor + S.k * L.actorSize + L.aReady) }); };
        const strata = h => { S.datum = [0, 1, 2, 3].map(sn => dq0Datum(m, sn)); };
        const dbg = (h, L) => { if (S.k !== undefined) { const o = L.actor + S.k * L.actorSize; S.dbg.push([h[o + L.aSn], h[o + L.aX] | h[o + L.aX + 1] << 8, h[o + L.aY] | h[o + L.aY + 1] << 8, r32(h, o + L.aReady), r32(h, L.hubNow)]); } };
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, call: put }, { g: 1, line: 'd', n: 1, call: (h, L) => { dbg(h, L); } },
          { g: 1, line: 'wait', n: 1, call: (h, L) => { ready(h, L); strata(h); dbg(h, L); } }, { g: 1, line: 'wait', n: 2, call: ready }, { g: 1, line: 'wait', n: 3, call: ready }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout(), S = m.flight, s = dq0Prose(out);
        if (S.k === undefined) return ['the scene not set: no bandit, or no floor beside the character'];
        const d = T.of('d'), q = T.of('quit');
        if (!d.at || !d.after || d.after.pc.sn === d.at.pc.sn) return ['the character didn\'t go down: ' + dq0Evs(d)];
        const st = stratumRec(m, d.at.pc.sn), lk = st && st.links.find(l => l.x === d.after.pc.x && l.y === d.after.pc.y && l.to === d.after.pc.sn);
        if (!lk) return ['no link where the character came down: ' + JSON.stringify(st && st.links) + ' ' + JSON.stringify(d.after.pc)];
        const feet = Math.abs(S.datum[d.at.pc.sn] - S.datum[d.after.pc.sn]), up = [K.O_STAIRS_UP, K.O_LADDER_UP, K.O_RAMP_UP].includes(lk.kind);
        // (The character's: its walk to the foot and the flight, at its step)
        const ct = d.after.now - d.at.now, cf = dq0Flight(lk.kind, d.at.sum.step, feet, up);
        if (ct < cf) f.push('the character\'s crossing ' + ct + ' tenths, less than the flight\'s ' + cf + ' at its step ' + d.at.sum.step);
        // (The ally's: told where it came out, on the stratum below, by the flight row; its time the flight's at its own step)
        const de = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === S.k + 1 && e.sn === d.after.pc.sn && e.t >= d.at.now);
        if (!de.length) f.push('the ally never told coming out below (E_DEN on stratum ' + d.after.pc.sn + ')');
        else {
          const e = de[0];
          if (e.a !== A.flight.de || e.b !== A.flight.loud) f.push('its coming out not by the flight row: ' + [e.a, e.b] + ' not ' + [A.flight.de, A.flight.loud]);
          if (Math.max(Math.abs(e.w1 - lk.x), Math.abs(e.w2 - lk.y)) > 1) f.push('it came out away from the flight\'s foot: ' + [e.w1, e.w2] + ' not by ' + [lk.x, lk.y]);
          const r = S.ready.find(x => x.ready > e.t), bt = r ? r.ready - e.t : -1, bf = dq0Flight(lk.kind, S.step, feet, up), walk = bt - bf;
          if (!r) f.push('its ready never past its coming out: ' + JSON.stringify(S.ready) + ' at ' + e.t);
          else if (walk < 0 || walk >= 4 * S.step) f.push('its crossing ' + bt + ' tenths not the flight\'s ' + bf + ' at its step ' + S.step + ' and a walk of a step a square (three at most)');
          this.notes = ['link ' + lk.kind + ' feet ' + feet + '; the character ' + ct + ' (flight ' + cf + ', step ' + d.at.sum.step + '); the ally ' + bt + ' (flight ' + bf + ', step ' + S.step + ')'];
        }
        const end = q.at && q.at.actors.find(a => a.k === S.k);
        if (!end || end.sn !== d.after.pc.sn) f.push('the ally not on the stratum below at the end: ' + JSON.stringify(end));
        if (f.length) f.push(s.slice(s.indexOf('> d'), s.indexOf('> d') + 800));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dentakein', outfit: true, near: ['denizens', 'gear'], what: "Dingus Quest Zero's denizen taking a thing from an open container lying by it (the user's, 2026-11-17: a denizen takes from containers as the character can; H17, 2026-11-21: a thing in a container within reach to hand at any depth, a level's more time each; the one-path plan's stage 2, things and robbery, 2026-11-23: the act audit's C2, C9): at seed 1 (dq0denportal's wary bandits, dq0 -k crowns) the crowns put in the kit's sack, the sack set down open; a bandit put beside it, its own take posted (I_TAKE, its wants TW_LOOT, as its mind would): it sees the crowns in the open sack and takes them (the takeout row's time and the sack's level, the character's own), into its purse (coins to the purse, the character's carry), told ('The bandit takes the crowns from the sack.'); the sack tied shut instead, it takes nothing (what's in a closed thing unseen)",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k crowns -S /ram/s.sc", 'drop the sack', 'put the crowns in the sack', 'wait', 'wait', 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k crowns -S /ram/s.sc", 'drop the sack', 'put the crowns in the sack', 'close the sack', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.tin = { k: {}, did: {}, ready: {} };
        const u16 = (b, o) => b[o] | b[o + 1] << 8;
        const put = g => (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          dq0Alone(m, h, L, K, at, k);
          S.k[g] = k;
          S.did[g] = u16(h, at(k) + L.aDid);
          const R = dq0Rec(m), q = R.at(h[at(k) + L.aDen]);
          h[at(k) + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_INDIFFERENT;
          h[at(k) + L.aDoing] = K.DO_WAIT;
          const far = 3000 + (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16);   // (Its mind quiet: free some five minutes on)
          for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = far >>> (8 * i) & 255;
        };
        const rd = (h, L, k) => { const o = L.actor + k * L.actorSize + L.aReady; return (h[o] | h[o + 1] << 8 | h[o + 2] << 16 | h[o + 3] << 24) >>> 0; };
        const take = g => (h, L, K) => { if (S.k[g] === undefined) return; S.ready[g] = [rd(h, L, S.k[g])]; dq0Intent(h, L, K.I_TAKE, S.k[g], { a: 4, b: 0 }); };   // (TW_LOOT)
        const end = g => (h, L) => { if (S.k[g] !== undefined && S.ready[g]) S.ready[g].push(rd(h, L, S.k[g])); };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face' }, { g: 1, line: 'wait', n: 1, call: put(1) }, { g: 1, line: 'wait', n: 1, call: take(1) }, { g: 1, line: 'wait', n: 2, call: end(1) },
          { g: 2, line: 'wait', n: 1, name: 'bandit', beside: 'face' }, { g: 2, line: 'wait', n: 1, call: put(2) }, { g: 2, line: 'wait', n: 1, call: take(2) }, { g: 2, line: 'wait', n: 2, call: end(2) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), S = m.tin, s = dq0Prose(out), G = s.split('% dq0 ').slice(1);
        if (S.k[1] === undefined || S.k[2] === undefined) return ['the scenes not set: ' + JSON.stringify(S)];
        const q1 = T.filter(t => t.line === 'quit')[0], q2 = T.filter(t => t.line === 'quit')[1];
        const cr = t => t && t.at ? t.at.items.filter(i => i.name === 'crowns') : [];
        const c1 = cr(q1), c2 = cr(q2);
        const its = (t, i, g) => dq0Root(t.at, i).where === 'IW_DEN' && dq0Root(t.at, i).did === S.did[g];
        if (!c1.some(i => its(q1, i, 1))) f.push('the crowns in the open sack not taken by the bandit: ' + JSON.stringify(c1.map(i => [i.where, i.x])));
        else if (!c1.some(i => its(q1, i, 1) && i.where === 'IW_ITEM' && (dq0Holder(q1.at, i) || {}).name === 'pouch')) f.push('the crowns not put in its purse (coins to the purse): ' + JSON.stringify(c1.map(i => [i.where, (dq0Holder(q1.at, i) || {}).name])));
        const told = dq0Turns(m).filter(t => t.game === 0).flatMap(t => t.dens).filter(e => e.kind === 'E_BEAT' && e.whose === S.k[1] + 1);   // (Told if its square's in sight: E_BEAT:things WB_TAKES, the crowns from the sack)
        if (told.length && !told.some(e => e.beat === 'WB_TAKES' && e.thing === m.dq0.layout().K.KI_CROWNS && e.other === m.dq0.layout().K.KI_SACK)) f.push('the take from the sack told otherwise: ' + JSON.stringify(told));
        const t1 = S.ready[1] && S.ready[1][1] - S.ready[1][0];
        if (t1 !== A.takeout.t + 20) f.push('its take not the takeout row\'s time and the sack\'s level (' + (A.takeout.t + 20) + '): ' + t1);
        if (c2.some(i => its(q2, i, 2))) f.push('the crowns in the sack tied shut taken: unseen, they should be left');
        this.notes = ['its time ' + t1];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0flightfoe', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's foe after the character up or down a flight, its mind's choice (ONE-PATH.md's 2.7: the one-path plan's stage 2, climbing and links, 2026-11-23; the act audit's G-1: a denizen went up or down a way only as an ally, before): at seed 1 (dq0flightact's scene: the character begun at a way down, a bandit beside it its ally, come along down the flight) the bandit put back at the flight's top, on the stratum above, hostile, coming at the character, near it as it went (the scene set as the first wait's typed: the hub's actor and its record, the minds' anear, drawn and last-sensed square poked); its mind goes after it (a foe that was coming at the character or fighting it, near it as it went up or down: its I_CLIMB, as an ally's), down the same flight by the character's rule (the world's den_flight: its walk to the flight's foot and the flight at its own step, on its ready; not roused from it), out at the other end or beside it on the stratum below, told there as its step (E_DEN by the flight row)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'look', 'd', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.flight = { ready: [] }, r32 = (h, o) => (h[o] | h[o + 1] << 8 | h[o + 2] << 16 | h[o + 3] << 24) >>> 0;
        const ally = (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          dq0Alone(m, h, L, K, at, k);
          const sq = dq0FloorBy(m, h, L, 1);
          if (!sq) return;
          const o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; };
          w16(o + L.aX, sq[0]); w16(o + L.aY, sq[1]); h[o + L.aSn] = h[L.pcSn]; h[o + L.aPos] = K.POS_STANDING; h[o + L.aDoing] = K.DO_FOLLOW;
          R.mem[q + R.X] = sq[0] & 255; R.mem[q + R.X + 1] = sq[0] >> 8; R.mem[q + R.Y] = sq[1] & 255; R.mem[q + R.Y + 1] = sq[1] >> 8; R.mem[q + R.SN] = h[L.pcSn];
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_ALLIED;
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          S.k = k; S.step = h[o + L.aStep]; S.top = [h[L.pcSn], h[L.pcX] | h[L.pcX + 1] << 8, h[L.pcY] | h[L.pcY + 1] << 8];
        };
        const foe = (h, L, K, at) => {   // (Back at the flight's top, beside its foot, hostile, coming: near the character as it went)
          if (S.k === undefined || h[at(S.k) + L.aSn] === S.top[0]) return;
          const o = at(S.k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; };
          const x = S.top[1], y = S.top[2] + 1, px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;
          w16(o + L.aX, x); w16(o + L.aY, y); h[o + L.aSn] = S.top[0]; h[o + L.aDoing] = K.DO_COMING;
          R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8; R.mem[q + R.SN] = S.top[0];
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
          for (let i = 0; i < 4; i++) h[o + L.aReady + i] = h[L.hubNow + i];
          const ad = dq0Ad(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1')), ram = dq0LaterRam(m);
          ram[ad('drawn') + S.k] = 8; ram[ad('anear') + S.k] = 1;
          ram[ad('lsx') + 2 * S.k] = px & 255; ram[ad('lsx') + 2 * S.k + 1] = px >> 8;
          ram[ad('lsy') + 2 * S.k] = py & 255; ram[ad('lsy') + 2 * S.k + 1] = py >> 8;
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          S.put = r32(h, L.hubNow);
        };
        const ready = (h, L) => { if (S.k !== undefined) S.ready.push(r32(h, L.actor + S.k * L.actorSize + L.aReady)); };
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, call: ally }, { g: 1, line: 'wait', n: 1, call: foe }, { g: 1, line: 'wait', n: 2, call: ready }, { g: 1, line: 'wait', n: 3, call: ready }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), S = m.flight, s = dq0Prose(out);
        if (S.k === undefined || S.put === undefined) return ['the scene not set: no bandit, no floor beside the character, or it never came along: ' + JSON.stringify(S)];
        const d = T.of('d'), w = T.of('wait'), q = T.of('quit');
        const de = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === S.k + 1 && e.sn === d.after.pc.sn && e.t >= S.put && e.a === A.flight.de && e.b === A.flight.loud);
        if (!de.length) f.push('the foe never came down after the character (E_DEN by the flight row on stratum ' + d.after.pc.sn + ')');
        else {
          if (Math.max(Math.abs(de[0].w1 - S.top[1]), Math.abs(de[0].w2 - S.top[2])) > 1) f.push('it came out away from the flight\'s foot: ' + [de[0].w1, de[0].w2]);
          const st = stratumRec(m, S.top[0]), lk = st && st.links.find(l => l.x === S.top[1] && l.y === S.top[2]), feet = Math.abs(dq0Datum(m, S.top[0]) - dq0Datum(m, d.after.pc.sn));
          const bf = lk ? dq0Flight(lk.kind, S.step, feet, false) : 0, r = S.ready.find(v => v > de[0].t), walk = r === undefined ? -1 : r - de[0].t - bf;
          if (walk < 0) f.push('its crossing ' + (r - de[0].t) + ' tenths not the flight\'s ' + bf + ' at its step ' + S.step + ' and a walk of a step a square: ' + JSON.stringify(S.ready));
          this.notes = ['the foe\'s crossing ' + (r - de[0].t) + ' (flight ' + bf + ', step ' + S.step + ')'];
        }
        const end = q.at && q.at.actors.find(a => a.k === S.k);
        if (!end || end.sn !== d.after.pc.sn) f.push('the foe not on the stratum below at the end: ' + JSON.stringify(end));
        if (f.length) f.push(s.slice(s.indexOf('> wait'), s.indexOf('> wait') + 800));
        return f;
      },
      sdcard: true
    },
    {
      // (Lights, every actor's: the one-path plan's stage 2, 2026-11-23, ONE-PATH.md's 2.2 and 2.7, the act audit's G-4, the sense audit's S2, S3)
      name: 'dq0lightden', outfit: true, meaning: true, near: ['denizens', 'senses'], what: "Dingus Quest Zero's denizens' lights, every actor's by the character's rules (ONE-PATH.md's 2.2 and 2.7: the one-path plan's stage 2, lights, 2026-11-23; the act audit's G-4, the sense audit's S2 and S3): at seed 1 (dq0flightact's scene: a bandit beside the character made its ally, two of its own things in its purse made a torch and a tinderbox, as 'look' is typed: the item store's records, the hub's alight its AL_KIT) the character goes down the flight, the ally with it, and puts out its torch: the passage below dark.  Its mind (a kind that wants light, kinds.txt's l) lights it where it's dark to it (I_LIGHT; the world's wlink.c den_light): its tinderbox got out (a second), its tries the act table's kindle row (five seconds each, the referee's P_GROPE of its own sheet, 4 harder in the dark: the character's own), the torch got out of its purse into its hand, lit (told if seen: E_BEAT WB_LIGHTS, 'The bandit lights its torch.'; in the dark, not till its light shows it), its Dexterity poked to 18 (a deft one: its tries by its own die, RS_ASKER), its carried light the hub's alight, a lamp at its square for every looker (the senses': the character sees it by its light); its torch burning down as any light does (its fuel poked low as the second long wait's typed) and going out, told as one seen ('The torch gutters and goes out.'), its alight none after; and the character's own lighting in the same dark (the same P_GROPE at the same steps)",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'look', 'd', 'put out the torch',
        'wait', 'wait', 'wait 2 minutes', 'wait 1 minute', 'wait 1 minute', 'wait 1 minute', 'light the torch', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.lit = { al: [] }, H = require(path.join(TESTS, '..', 'games', 'dq0', 'hubgen.js')).load(), AL = H.off('struct hub', 'alight');
        const own = (h, L, K, did) => {   // (The bandit's own things and what's in them: the store's records)
          const per = Math.floor(0x2000 / L.itemSize), rec = i => ({ b: m.dq0.bank(L.item0 + Math.floor(i / per)), o: (i % per) * L.itemSize });
          const top = [], inn = [];
          for (let i = 1; i < L.nitems; i++) { const { b, o } = rec(i); if (b && b[o + L.iKind] && b[o + L.iWhere] === K.IW_DEN && (b[o + L.iOwner] | b[o + L.iHide] << 8) === did) top.push(i); }
          for (let i = 1; i < L.nitems; i++) { const { b, o } = rec(i); if (b && b[o + L.iKind] && b[o + L.iWhere] === K.IW_ITEM && top.includes(b[o + L.iX] | b[o + L.iX + 1] << 8)) inn.push(i); }
          return { rec, top, inn };
        };
        const put = (h, L, K, at, named, sc) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          dq0Alone(m, h, L, K, at, k);
          const sq = dq0FloorBy(m, h, L, 1);
          if (!sq) return;
          const o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; };
          w16(o + L.aX, sq[0]); w16(o + L.aY, sq[1]); h[o + L.aSn] = h[L.pcSn]; h[o + L.aPos] = K.POS_STANDING; h[o + L.aDoing] = K.DO_FOLLOW;
          R.mem[q + R.X] = sq[0] & 255; R.mem[q + R.X + 1] = sq[0] >> 8; R.mem[q + R.Y] = sq[1] & 255; R.mem[q + R.Y + 1] = sq[1] >> 8; R.mem[q + R.SN] = h[L.pcSn];
          h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_ALLIED;
          for (let i = 0; i < 4; i++) h[o + L.aReady + i] = h[L.hubNow + i];
          const did = h[o + L.aDid] | h[o + L.aDid + 1] << 8, { rec, inn } = own(h, L, K, did);
          if (inn.length < 2) { S.why = 'its purse holds ' + inn.length + ' things'; return; }
          const make = (i, kind, fuel) => { const { b, o: p } = rec(i); b[p + L.iKind] = kind; b[p + L.iFlags] = 0; b[p + L.iQty] = 1; b[p + L.iId] = 0xF0 + kind; b[p + L.iId + 1] = 0xFF; b[p + L.iFuel] = fuel & 255; b[p + L.iFuel + 1] = fuel >> 8; b[p + L.iShut] = 0; };
          make(inn[0], K.KI_TORCH, 60);
          make(inn[1], K.KI_TINDERBOX, 20);
          // (Its outfit's own torch, a bandit's since 2026-11-23 (the user's: bandits carry a torch and a tinderbox, a
          // pouch of their own), made its rations: the scene's torch its only light, so its burning out leaves it none)
          for (let i = 1; i < L.nitems; i++) {
            if (i === inn[0]) continue;
            let j = i;
            const { b, o: p } = rec(i);
            if (!b || b[p + L.iKind] !== K.KI_TORCH) continue;
            for (let n = 0; n < 8 && rec(j).b[rec(j).o + L.iWhere] === K.IW_ITEM; n++) j = rec(j).b[rec(j).o + L.iX] | rec(j).b[rec(j).o + L.iX + 1] << 8;
            const r = rec(j);
            if (r.b[r.o + L.iWhere] === K.IW_DEN && (r.b[r.o + L.iOwner] | r.b[r.o + L.iHide] << 8) === did) b[p + L.iKind] = K.KI_RATIONS;
          }
          sc.att(k, 2, 18);                     // (A deft one, its Dexterity 18: its sheet's att[2], the grope's: the referee's)
          h[AL + k] = 0x80;                     // (AL_KIT: its things as the world would have them told)
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          S.k = k; S.torch = inn[0]; S.tinder = inn[1]; S.sq = sq;
        };
        const look = (h, L) => { if (S.k !== undefined) S.al.push(h[AL + S.k]); };
        const low = (h, L, K) => {   // (Its torch near its end: burnt out in the minute)
          look(h, L);
          if (S.k === undefined) return;
          const per = Math.floor(0x2000 / L.itemSize), b = m.dq0.bank(L.item0 + Math.floor(S.torch / per)), p = (S.torch % per) * L.itemSize;
          if (b[p + L.iFlags] & 1) { b[p + L.iFuel] = 1; b[p + L.iFuel + 1] = 0; S.low = 1; }
        };
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, call: put }, { g: 1, line: 'wait', n: 1, call: look }, { g: 1, line: 'wait', n: 2, call: look }, { g: 1, line: 'wait 1 minute', n: 1, call: look },
          { g: 1, line: 'wait 1 minute', n: 2, call: low }, { g: 1, line: 'wait 1 minute', n: 3, call: look },
          { g: 1, line: 'light the torch', n: 1, call: look }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), { K } = m.dq0.layout(), S = m.lit, s = dq0Prose(out);
        if (S.k === undefined) return ['the scene not set: ' + (S.why || 'no bandit, or no floor by the character')];
        const w0 = T.of('put out the torch'), lt = T.of('light the torch'), all = T.filter(t => t.k >= w0.k);
        const grope = list => list.filter(q => q.kind === K.RS_TEST && q.a === A.kindle.p);
        const theirs = grope(all.filter(t => t.k < lt.k).flatMap(t => t.replies)).filter(q => q.actor === S.k + 1);
        const mine = grope(lt.replies).filter(q => q.actor === 0);
        if (!theirs.length) f.push('the bandit never tried its tinderbox (RS_TEST P_GROPE, its own sheet)');
        if (theirs.some(q => q.steps !== -4)) f.push('its tries not 4 harder in the dark: ' + JSON.stringify(theirs.map(q => q.steps)));
        if (mine.length && theirs.length && mine[0].steps !== theirs[0].steps) f.push('the same dark, other steps: the character\'s ' + mine[0].steps + ', the bandit\'s ' + theirs[0].steps);
        const lit = T.flatMap(t => t.dens).filter(e => e.kind === 'E_BEAT' && e.w1 % 256 === K.WB_LIGHTS);
        if (!lit.length) f.push('its lighting never told (E_BEAT WB_LIGHTS)');
        else if (lit[0].w1 >> 8 !== K.KI_TORCH) f.push('its lighting told of another thing: ' + (lit[0].w1 >> 8));
        // (Its lighting told only if seen: in the dark, not till its own light shows it, as any act unseen)
        const torchAt = t => t.at && t.at.items.find(i => i.n === S.torch);
        const mid = T.of('wait 1 minute', w0.k), tm = torchAt(T.of('wait 1 minute', mid.k + 1));
        if (!tm || !(tm.flags & DQ0_I.LIT) || (tm.where === 'IW_DEN') !== !!(tm.flags & DQ0_I.HELD)) f.push('its torch not lit, in its hand or hung at its belt, as the minute went on: ' + JSON.stringify(tm));
        if (!S.al.some(v => (v & 0x1F) > 0)) f.push('its carried light never the hub\'s (alight): ' + JSON.stringify(S.al));
        const lt0 = lit.length ? T.find(t => t.dens.includes(lit[0])) : null, seen = lt0 && T.filter(t => t.k > lt0.k && t.k < lt.k && t.at && t.at.sight && t.at.sight.dens.length);
        if (lit.length && (!seen || !seen.length)) f.push('the bandit never seen by its light after it lit it');
        if (!S.low) f.push('its torch not lit as the second minute began: not burnt down');
        else {
          const out1 = T.flatMap(t => t.events).filter(e => e.kind === 'E_TASK' && e.te === 'TE_OUT');
          if (!/The torch gutters and goes out\./.test(s)) f.push('its torch\'s going out not told as one seen: "The torch gutters and goes out." ' + JSON.stringify(out1.map(e => e.w1)));
          if ((S.al[S.al.length - 1] & 0x1F) !== 0) f.push('its carried light the hub\'s still, its torch burnt out: ' + JSON.stringify(S.al));
        }
        this.notes = ['its tries ' + JSON.stringify(theirs.map(q => [q.steps, q.grade])) + ', the character\'s ' + JSON.stringify(mine.map(q => [q.steps, q.grade])), 'alight ' + JSON.stringify(S.al)];
        if (f.length) f.push(s.slice(s.indexOf('> wait'), s.indexOf('> wait') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      // (A door's trap, every actor's: the one-path plan's stage 2, the step and its hazards, 2026-11-23; the act audit's B9)
      name: 'dq0doortrap', outfit: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's door's trap set off by whoever opens the door, every actor's by the same code (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, 2026-11-23; the act audit's B9: workpart.h's front, the trap armed for every actor): at seed 1 (dq0denportal's wary bandits, the character with a rope and a shortsword) a scything blade, hidden at 10, its power 4, poked onto the door ahead (its chunk's trap records, TT_DOOR); the character opens the door: the trap set off (TRAP_STATE sprung), its dodge asked (the referee's P_DODGE_HAZARD, its power's half the steps worse), and, not dodged, its strike (RS_HARM HC_STRIKE: an edge's, at the middle, 2d6, its power more); and the same door, the character having opened and shut it, the blade poked on it, a hostile bandit put beyond it (dq0denportal's scene): it opens the door to come at the character, and the same trap goes off at it, the same dodge asked of it (its actor's, by its sheet), and, not dodged, the same strike (RS_HARM at its actor: its sheet's wound, a blow's)",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'mkdir /ram/h', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k rope,shortsword -S /ram/s.sc', 'open the door', 'close the door', 'look', 'open the door', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k rope,shortsword -S /ram/s.sc', 'open the door', 'close the door', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.dtrap = {};
        const HEAD_X = [0, 1, 1, 1, 0, -1, -1, -1], HEAD_Y = [-1, -1, 0, 1, 1, 1, 0, -1];
        const [NTRAPS, TRAP, TSIZE] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, ntraps)', 'offsetof (struct chunk, trap)', 'sizeof (struct trap)']);
        // (A scything blade on the door ahead of the character, hidden at 10, its power 4: its chunk's trap records, game g's)
        const blade = g => (h, L) => {
          const w = m.dq0.bank(WORLD_BANK_N), sn = h[L.pcSn], st = stratumRec(m, sn), d = h[L.pcFace] & 7;
          const x = (h[L.pcX] | h[L.pcX + 1] << 8) + HEAD_X[d], y = (h[L.pcY] | h[L.pcY + 1] << 8) + HEAD_Y[d], ox = x - st.x0, oy = y - st.y0;
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (!w[sl] || !b || w[sl + 1] !== sn || (w[sl + 2] | w[sl + 3] << 8) !== ox >> 5 || (w[sl + 4] | w[sl + 5] << 8) !== oy >> 5) continue;
            if (b[o + NTRAPS] >= 6) return;
            const q = o + TRAP + b[o + NTRAPS]++ * TSIZE;
            [ox & 31, oy & 31, 4, 2, 0x20, 10, 4].forEach((v, i) => { b[q + i] = v; });   // (TK_BLADE, TT_DOOR, TS_HIDDEN, its hiding, its power)
            S[g] = { x, y, sn };
            return;
          }
        };
        dq0DoorScene(m, 'wait', 'bandit');
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, call: blade(1) }, { g: 2, line: 'wait', n: 1, call: blade(2) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], S = m.dtrap;
        if (!S[1] || !S[2] || !m.door) return ['the scenes not set: ' + JSON.stringify([S, m.doorWhy])];
        if (S[2].x !== m.door.door[0] || S[2].y !== m.door.door[1]) return ['the blade not on the bandit\'s door: ' + JSON.stringify([S[2], m.door.door])];
        const games = [], g = [];
        for (const t of T) { if (/^dq0 /.test(t.line)) games.push(g.splice(0)); g.push(t); }
        games.push(g.splice(0)); games.shift();
        if (games.length !== 2) return ['the two games not played: ' + games.length];
        const who = [0, m.door.k + 1], name = ['the character', 'the bandit'];
        const asks = (gm, w, rs, pred) => gm.flatMap(t => t.replies).filter(r => r.rs === rs && r.actor === w && pred(r.ask));
        const changes = gm => gm.flatMap(t => [t.at, t.after].filter(Boolean).flatMap(x => x.changes || []));
        const seen = [];
        for (let i = 0; i < 2; i++) {
          const gm = games[i], tr = S[i + 1];
          const dodge = asks(gm, who[i], 'RS_TEST', q => q.purpose === 'P_DODGE_HAZARD');
          const h = asks(gm, who[i], 'RS_HARM', q => q.cause === 'HC_STRIKE');
          seen.push({ dodge: dodge.map(r => [r.ask.steps, r.grade]), harm: h.map(r => [r.ask.ground, r.ask.how, r.ask.feet, r.reply.band]) });   // (The request's b, c, w1 by the harm's default names: its aspect, its dice, its impact more)
          if (!changes(gm).some(c => c.what === 'TRAP_STATE' && c.x === tr.x && c.y === tr.y && (c.tile & 1))) { f.push(name[i] + ' opened the door, its blade not set off (TRAP_STATE sprung)'); continue; }
          if (dodge.length !== 1) { f.push(name[i] + ' not asked to dodge the blade once: ' + dodge.length); continue; }
          if (dodge[0].ask.steps !== -2) f.push(name[i] + '\'s dodge not at its power\'s half (-2): ' + dodge[0].ask.steps);
          if (dodge[0].grade > 0 ? h.length : h.length !== 1) f.push(name[i] + '\'s dodge ' + dodge[0].grade + ' but its strikes ' + h.length);
          if (h.length && (h[0].ask.how !== 2 || h[0].ask.feet !== 4)) f.push(name[i] + '\'s strike not the blade\'s (2d6, 4 more): ' + JSON.stringify(h[0].ask));
        }
        if (seen[0].dodge.length && seen[1].dodge.length && seen[0].dodge[0][0] !== seen[1].dodge[0][0]) f.push('the two dodges not at the same steps: ' + JSON.stringify(seen));
        if (seen[0].harm.length && seen[1].harm.length && JSON.stringify(seen[0].harm[0].slice(0, 3)) !== JSON.stringify(seen[1].harm[0].slice(0, 3))) f.push('the two strikes not the same: ' + JSON.stringify(seen));
        this.notes = [JSON.stringify(seen)];
        return f;
      },
      sdcard: true
    },
    {
      // (Building, every actor's: the user's, 2026-11-23, "The character builds anywhere, with materials"; build2)
      name: 'dq0buildact', outfit: true, meaning: true, near: ['denizens', 'map'], what: "Dingus Quest Zero's building, every actor's by the same code (the user's, 2026-11-23: \"The character builds anywhere, with materials\"; ONE-PATH.md's 2.7, the act audit's G-17): at seed 1 (-R 7, a theme of dwarf miners, dq0 -k hammer) the character opens its cell's door, the door broken down (its portal's state poked, PS_WRECKED); with its hammer dropped, \"hang the door\" refused (\"You have no hammer.\"); with it, the door hung again (the things' IO_BUILD, wbuild.c's: the referee's Timbercraft, P_TIMBER, actor 0, the act table's hang row's steps; its noise the row's; its time the row's ten minutes, the UI's wait; done, the door's PORTAL_STATE shut and whole, else nothing); \"wall up\" facing the door refused; turned to the cell's wall, the square ahead made a passage (poked), \"wall up\" refused for want of stones, then with rubble poked beside it walled up (P_MASONRY, the wallup row's hour, the passage a wall, the rubble's square cleared); then a dwarf miner put beyond the door broken down again, its builder's intent posted as its mind would (I_KIT KT_BUILD at the door, then at a passage beyond it with rubble beside): the same test by its own sheet at the same steps, the same time on its ready, its noise the same row's (E_DEN DE_HANGS, DE_WALLS, told seen or heard), the same effects",
      init: 't_rc', cycles: 4500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k hammer -R 7 -S /ram/s.sc', 'open the door', 'wait', 'drop the hammer', 'hang the door', 'take the hammer',
        'hang the door', 'wall up', 'turn left', 'wait', 'wall up', 'wait', 'wall up', 'turn right', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const S = m.build = { notes: [] };
        const HX = [0, 1, 1, 1, 0, -1, -1, -1], HY = [-1, -1, 0, 1, 1, 1, 0, -1];
        const [TILE] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, tile)']);
        const C = require('../../../sim/lib/dq0meaning.js').constants().names;
        const u16 = (h, o) => h[o] | h[o + 1] << 8, u32 = (h, o) => (h[o] | h[o + 1] << 8 | h[o + 2] << 16 | h[o + 3] << 24) >>> 0;
        const cell = (h, L, x, y) => {   // (Square x, y's tile byte in the chunks' cache: { b, i })
          const w = m.dq0.bank(WORLD_BANK_N), sn = h[L.pcSn], st = stratumRec(m, sn), ox = x - st.x0, oy = y - st.y0;
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (w[sl] && b && w[sl + 1] === sn && (w[sl + 2] | w[sl + 3] << 8) === ox >> 5 && (w[sl + 4] | w[sl + 5] << 8) === oy >> 5) return { b, i: o + TILE + (oy & 31) * 32 + (ox & 31) };
          }
          return null;
        };
        const tile = (h, L, x, y) => { const q = cell(h, L, x, y); return q ? q.b[q.i] & 15 : -1; };
        const set = (h, L, x, y, t) => { const q = cell(h, L, x, y); if (q) q.b[q.i] = q.b[q.i] & 0xF0 | t; return !!q; };
        const ahead = (h, L, n) => { const f = h[L.pcFace] & 7; return [u16(h, L.pcX) + n * HX[f], u16(h, L.pcY) + n * HY[f], f]; };
        const wreck = (h, L) => {   // (The door ahead broken down: PS_WRECKED | PS_OPEN)
          const [x, y, f] = ahead(h, L, 1), p = dq0Portal(m, h[L.pcSn], x, y);
          if (p && p.kind === 0) { p.b[p.state] = 0x81; S.door = { x, y, sn: h[L.pcSn], face: f }; }
          else S.notes.push('no door ahead: ' + JSON.stringify([x, y, tile(h, L, x, y)]));
        };
        const rubble = (h, L, x, y, not) => {   // (A square beside x, y made rubble: not the character's, not those given, no door)
          for (let d = 0; d < 8; d++) {
            const rx = x + HX[d], ry = y + HY[d];
            if ((rx === u16(h, L.pcX) && ry === u16(h, L.pcY)) || not.some(([a, b]) => a === rx && b === ry) || [3, 5, 6, 7, 8].includes(tile(h, L, rx, ry))) continue;
            set(h, L, rx, ry, 10);
            return [rx, ry];
          }
          return null;
        };
        const rd = (h, L) => u32(h, L.actor + S.k * L.actorSize + L.aReady);
        dq0Scene(m, [
          { g: 1, line: 'wait', n: 1, call: (h, L) => wreck(h, L) },
          { g: 1, line: 'wait', n: 2, call: (h, L) => {   // (Turned to the cell's wall: the square ahead a passage, none on it, no rubble by it)
            const [x, y] = ahead(h, L, 1);
            S.wall = { x, y, was: tile(h, L, x, y) };
            if (!set(h, L, x, y, T_CORR)) S.notes.push('no square ahead to make a passage');
          } },
          { g: 1, line: 'wait', n: 3, call: (h, L) => { if (S.wall) S.rubble = rubble(h, L, S.wall.x, S.wall.y, S.door ? [[S.door.x, S.door.y]] : []); } },
          { g: 1, line: 'wait', n: 4, call: (h, L, K, at, named) => {   // (The door broken down again; a dwarf miner beyond it, its builder's intent there)
            wreck(h, L);
            const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'dwarf miner');
            if (k === undefined || !S.door) { S.notes.push('no dwarf miner: ' + [...Array(L.nactors).keys()].map(named)); return; }
            dq0Alone(m, h, L, K, at, k);
            S.k = k;
            const [x, y, f] = ahead(h, L, 2), o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]);
            const w16 = (a, v) => { h[a] = v & 255; h[a + 1] = v >> 8 & 255; };
            w16(o + L.aX, x); w16(o + L.aY, y); h[o + L.aSn] = h[L.pcSn]; h[o + L.aState] = 1; h[o + L.aPos] = K.POS_STANDING; h[o + L.aDoing] = K.DO_WAIT;
            h[o + L.aFace] = (f + 4) & 7;
            h[o + L.aAttitude] = R.mem[q + R.ATTITUDE] = K.AT_INDIFFERENT;
            R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8; R.mem[q + R.SN] = h[L.pcSn];
            const far = 3000 + u32(h, L.now);
            for (let i = 0; i < 4; i++) h[o + L.aReady + i] = far >>> (8 * i) & 255;
            S.dwarf = { x, y, hang: { now: u32(h, L.now) } };
            h[L.denSeq] = h[L.denSeq] + 1 & 255;
            dq0Intent(h, L, C.I_KIT, k, { a: C.KT_BUILD, b: (f + 4) & 7, w1: S.door.x, w2: S.door.y });
          } },
          { g: 1, line: 'wait', n: 5, call: (h, L) => {   // (Its ready after the door; then a passage beyond it, rubble beside, walled up)
            if (S.k === undefined || !S.dwarf) return;
            S.dwarf.hang.ready = rd(h, L);
            const f = S.door.face, x = S.dwarf.x + HX[f], y = S.dwarf.y + HY[f];
            if (!set(h, L, x, y, T_CORR)) return;
            S.dwarf.wall = { x, y, now: u32(h, L.now), rubble: rubble(h, L, x, y, [[S.dwarf.x, S.dwarf.y], [S.door.x, S.door.y]]) };
            dq0Intent(h, L, C.I_KIT, S.k, { a: C.KT_BUILD, b: f, w1: x, w2: y });
          } },
          { g: 1, line: 'wait', n: 6, call: (h, L) => {   // (Its ready after the wall; the door as it stands)
            if (!S.dwarf || !S.dwarf.wall) return;
            S.dwarf.wall.ready = rd(h, L);
            const p = dq0Portal(m, S.door.sn, S.door.x, S.door.y);
            S.dwarf.door = p ? p.b[p.state] : -1;
            S.dwarf.tile = tile(h, L, S.dwarf.wall.x, S.dwarf.wall.y);
          } },
        ]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Acts(), s = dq0Prose(out), S = m.build;
        const C = require('../../../sim/lib/dq0meaning.js').constants().names;
        const tests = (t, who, p) => t.replies.filter(q => q.kind === C.RS_TEST && q.actor === who && q.a === p);
        const changes = T.of('quit').at ? T.of('quit').at.changes : [];
        const at = (c, x, y, what) => c.what === what && c.x === x && c.y === y;
        if (!S.door || !S.wall || !S.rubble) return ['the scenes not set: ' + JSON.stringify(S)];
        // (The character's: refused, no hammer; hung; refused, no passage; refused, no stones; walled up)
        const h1 = T.of('hang the door'), h2 = T.of('hang the door', h1.k + 1), w1 = T.of('wall up'), w2 = T.of('wall up', w1.k + 1), w3 = T.of('wall up', w2.k + 1);
        const words = t => dq0Prose(out).split('> ').filter(x => x.startsWith(t.line) && x.slice(t.line.length).trim())[T.filter(u => u.line === t.line && u.k <= t.k).length - 1] || '';   // (Its record's, not its echo's)
        if (!/You have no hammer\./.test(words(h1)) || tests(h1, 0, A.hang.p).length) f.push('"hang the door" not refused for want of a hammer: ' + words(h1));
        const ht = tests(h2, 0, A.hang.p);
        if (ht.length !== 1 || ht[0].steps !== A.hang.s) f.push('the character\'s hang not one Timbercraft test at the row\'s steps: ' + JSON.stringify(ht.map(q => [q.a, q.steps, q.grade])));
        const hn = dq0Ev(h2, 'E_NOISE');
        if (!hn.some(e => e.loud === A.hang.loud)) f.push('the character\'s hang\'s noise not the row\'s: ' + JSON.stringify(hn.map(e => e.loud)));
        if (!h2.intents.some(i => i.kind === 'I_WAIT' && i.tenths === A.hang.t)) f.push('the character\'s hang not the row\'s time: ' + JSON.stringify(h2.intents));
        const hung = ht.length && ht[0].grade > 0;
        if (hung && !(/You mend the broken leaf and hang the door again\./.test(words(h2)) && h2.after && h2.after.changes.some(c => at(c, S.door.x, S.door.y, 'PORTAL_STATE') && !(c.tile & 0x81))))
          f.push('the door not hung though its test went ' + ht[0].grade + ': ' + words(h2));
        if (ht.length && !hung && !/The work goes badly/.test(words(h2))) f.push('the hang gone badly not told: ' + words(h2));
        if (!/no clear passage ahead to wall up/.test(words(w1))) f.push('"wall up" facing the door not refused: ' + words(w1));
        if (!/You have no stones to wall it up with\./.test(words(w2))) f.push('"wall up" with no stones not refused: ' + words(w2));
        const wt = tests(w3, 0, A.wallup.p);
        if (wt.length !== 1 || wt[0].steps !== A.wallup.s) f.push('the character\'s wall not one Masonry test at the row\'s steps: ' + JSON.stringify(wt.map(q => [q.a, q.steps, q.grade])));
        const wn = dq0Ev(w3, 'E_NOISE');
        if (!wn.some(e => e.loud === A.wallup.loud)) f.push('the character\'s wall\'s noise not the row\'s: ' + JSON.stringify(wn.map(e => e.loud)));
        if (!w3.intents.some(i => i.kind === 'I_WAIT' && i.tenths === 6 * A.wallup.t)) f.push('the character\'s wall not six of the row\'s spells: ' + JSON.stringify(w3.intents));
        const walled = wt.length && wt[0].grade > 0, wa = w3.after ? w3.after.changes : [];
        if (walled && !(wa.some(c => at(c, S.wall.x, S.wall.y, 'SQUARE') && c.tile === T_WALL) && wa.some(c => at(c, S.rubble[0], S.rubble[1], 'SQUARE') && c.tile === T_CORR) &&
          /stones from the rubble/.test(words(w3)))) f.push('the way not walled up with the rubble\'s stones though its test went ' + wt[0].grade + ': ' + words(w3) + ' ' + JSON.stringify(wa.slice(-4)));
        if (wt.length && !walled && wa.some(c => at(c, S.wall.x, S.wall.y, 'SQUARE') && c.tile === T_WALL)) f.push('walled up though its test failed');
        // (The dwarf miner's: the same code)
        if (S.k === undefined || !S.dwarf || !S.dwarf.wall) return [...f, 'the dwarf\'s scenes not set: ' + JSON.stringify(S)];
        const by = S.k + 1, all = T.flatMap(t => t.replies), dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === by);
        const dh = all.filter(q => q.kind === C.RS_TEST && q.actor === by && q.a === A.hang.p), dw = all.filter(q => q.kind === C.RS_TEST && q.actor === by && q.a === A.wallup.p);
        if (dh.length !== 1 || dh[0].steps !== A.hang.s) f.push('the dwarf\'s hang not one Timbercraft test by its sheet at the row\'s steps: ' + JSON.stringify(dh.map(q => [q.steps, q.grade])));
        if (dw.length !== 1 || dw[0].steps !== A.wallup.s) f.push('the dwarf\'s wall not one Masonry test by its sheet at the row\'s steps: ' + JSON.stringify(dw.map(q => [q.steps, q.grade])));
        const dt = (r, n) => r.ready - r.now >= n && r.ready - r.now < n + 50;
        if (!dt(S.dwarf.hang, A.hang.t)) f.push('the dwarf\'s hang not the row\'s time on its ready: ' + JSON.stringify(S.dwarf.hang));
        if (!dt(S.dwarf.wall, 6 * A.wallup.t)) f.push('the dwarf\'s wall not six of the row\'s spells on its ready: ' + JSON.stringify(S.dwarf.wall));
        const told = (de, x, y) => dens.filter(e => e.a === de && e.w1 === x && e.w2 === y);
        if (dh.length && dh[0].grade > 0) {
          const e = told(C.DE_HANGS, S.door.x, S.door.y);
          if (e.length !== 1 || (e[0].b & 15) !== A.hang.loud) f.push('the dwarf\'s door hung not told once at the hang row\'s loudness: ' + JSON.stringify(e.map(x => x.b)));
          if (S.dwarf.door !== 0) f.push('the dwarf\'s door not hung: its state ' + S.dwarf.door);
        } else if (dh.length && !told(C.DE_NOISE, S.door.x, S.door.y).length) f.push('the dwarf\'s hang gone badly not heard');
        if (dw.length && dw[0].grade > 0) {
          const e = told(C.DE_WALLS, S.dwarf.wall.x, S.dwarf.wall.y);
          if (e.length !== 1 || (e[0].b & 15) !== A.wallup.loud) f.push('the dwarf\'s wall not told once at the wallup row\'s loudness: ' + JSON.stringify(e.map(x => x.b)));
          if (S.dwarf.tile !== T_WALL || !changes.some(c => at(c, S.dwarf.wall.x, S.dwarf.wall.y, 'SQUARE') && c.tile === T_WALL)) f.push('the dwarf\'s way not walled up: ' + S.dwarf.tile);
          const r = S.dwarf.wall.rubble;
          if (!r || !changes.some(c => at(c, r[0], r[1], 'SQUARE') && c.tile === T_CORR)) f.push('the dwarf\'s stones not the rubble\'s: ' + JSON.stringify(r));
        }
        this.notes = ['the character ' + JSON.stringify([ht.map(q => q.grade), wt.map(q => q.grade)]), 'the dwarf ' + JSON.stringify([dh.map(q => q.grade), dw.map(q => q.grade)]), ...S.notes];
        return f;
      },
      sdcard: true
    },
  ],
};
