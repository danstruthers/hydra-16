// ****************************************************************************
// denizens.js - Dingus Quest Zero's tests: the denizens, their minds and their places.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { card, typed } = require('../../lib/common.js');
const {
  T_ROCK, T_FLOOR, T_WALL, T_CORR, T_WINDOW, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N, CSLOT_AT, liveChunks, stratumRec,
  dq0Story, inTurn, dq0Answers, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need, dq0Never, dq0Carried, dq0Named, DQ0_I,
  dq0Typed, dq0Portal, dq0Rec, dq0Poke, dq0PcShock, dq0Aware, dq0PortalAt, dq0OnWords, dq0Timer
} = require('../../lib/dq0.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')

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
        if (h && p) {
          const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
          m.taskRam[h[12 + 2]][parseInt(/^al ([0-9A-F]+) \._hour0$/m.exec(lbl)[1], 16)] = p.hour;
        }
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
        if (typed === on && !done && m.dq0) {
          const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
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
            const R = dq0Rec(m), q = R.at(h[o + L.aName + 12]);
            R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
            R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
            R.mem[q + R.SN] = sn;
            R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
            const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
            const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16), ram = m.taskRam[h[12 + 2]];
            ram[ad('drawn') + k] = 8;
            ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
            ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
            if (how) how(h, o, L, K);
            h[L.denSeq] = h[L.denSeq] + 1 & 255;
            m.door = { k, door: [px + dx, py + dy], beyond: [x, y], pc: [px, py], tile, state };
          }
          done = true;
        }
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7',
        'open the door', 'go through it', 'turn around', 'close the door ahead', 'wait', 'wait 2 hours', 'wait', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'wait 2 hours', n: 1, words: 'A fresh tunnel opens', text: 'stop\r' }]);   // (Stopped as it's told: 2026-11-03)
        const others = (h, L) => { for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName] && (!m.tun || k !== m.tun.k)) h[L.actor + k * L.actorSize + L.aAttitude] = 3; };
        dq0Poke(m, ['wait 2 hours', 'stop'].map(line => ({ g: 1, line, n: 1, call: others })));   // (The other miners let be: this one's the scene)
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.tun = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0) {   // (The scene: one dwarf miner beyond the shut door ahead, hostile, drawn on)
                const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
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
                  const R = dq0Rec(m), q = R.at(h[o + L.aName + 12]);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  R.mem[q + R.SN] = sn;
                  R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16), ram = m.taskRam[h[12 + 2]];
                  ram[ad('drawn') + k] = 8;
                  if (!m.portc) ram[ad('atry') + k] = ((f >> 1) + 2 & 3) | 3 << 2;   // (Its tries at the portcullis given up already (the minds'
                  // atry: its way, stage 3), 2026-11-02: so it digs round; dq0portc's the portcullis worked)
                  ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
                  ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.tun = { k, door: [px + dx, py + dy], beyond: [x, y], pc: [px, py], tile };
                }
                done = true;
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), DE_DIG = 24, DE_DUG = 25;
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=8',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 374 -c /ram/c.hm -T -d /ram/g',
        'wait 2 hours', 'wait 2 hours', 'wait 2 hours', 'quit', 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '';
        m.mine = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 2 hours' && !m.opened && m.dq0) {   // (The scene: the portcullis on the mine's way across raised)
                const { L } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
                const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ram = m.taskRam[h[12 + 2]];
                const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16), sn = h[L.pcSn];
                const hx = ram[ad('ahx')] | ram[ad('ahx') + 1] << 8, hy = ram[ad('ahy')] | ram[ad('ahy') + 1] << 8, v = dq0Vein(u16(L.seed), hx, hy);
                m.opened = [];
                for (let x = hx; v && x !== v.vx; x += Math.sign(v.vx - hx)) {
                  const p = dq0Portal(m, sn, x, hy);
                  if (p) {
                    m.opened.push([x, p.kind]);
                    p.b[p.state] = (p.b[p.state] & ~0x06) | 1;   // (Open (a portcullis raised), unlocked, unbarred)
                  }
                }
              }
              if (typed === 'quit' && !m.mine && m.dq0) {   // (The miners' homes, the actors': the minds' ahx, ahy)
                const { L } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
                const at = k => L.actor + k * L.actorSize, named = k => {
                  let n = '';
                  for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]);
                  return n;
                };
                const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16), ram = m.taskRam[h[12 + 2]];
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
        const T = dq0Turns(m), f = [], s = dq0Story(out), DE_DIG = 24, DE_DUG = 25, DE_DIGS = 26;
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g',
        "echo 'faction | Vermin | rat | 1-1 | 30 | - | 1 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s ' + 3 + ' -c /ram/c.hm -T -d /ram/g',
        'd', 'u', 'wait 3 hours', 'wait 3 hours', 'quit', 'echo $status']) },
      start(m) {
        m.burrow = { homes: [], rubble: [] };
        const HX = [0, 1, 0, -1], HY = [-1, 0, 1, 0];
        dq0Poke(m, [{ g: 1, line: 'wait 3 hours', n: 1, call: (h, L, K) => {   // (Their lairs placed by now: their records' numbers and homes)
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=8',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 374 -c /ram/c.hm -T -d /ram/g',
        'wait', 'look', 'd', 'wait 2 hours', 'wait 2 hours', 'u', 'look', 'quit', 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.door = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0) {   // (The scene: the closed door beside the character broken down, in the cache)
                const { L } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
                const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn];
                for (let d = 0; d < 8 && !m.door; d += 2) {
                  const x = px + [0, 1, 1, 1, 0, -1, -1, -1][d], y = py + [-1, -1, 0, 1, 1, 1, 0, -1][d], p = dq0Portal(m, sn, x, y);
                  if (p && p.kind === 0 && !(p.b[p.state] & 1)) {
                    p.b[p.state] = 0x81;   // (PS_WRECKED | PS_OPEN: broken down)
                    m.door = { x, y, sn, seed: u16(L.seed) };
                  }
                }
                const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ram = m.taskRam[h[12 + 2]];
                const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16);
                if (m.door) m.door.home = [ram[ad('ahx')] | ram[ad('ahx') + 1] << 8, ram[ad('ahy')] | ram[ad('ahy') + 1] << 8];
                const v = m.door && dq0Vein(m.door.seed, m.door.home[0], m.door.home[1]);   // (The portcullis on the mine's way across raised, as dq0mine's)
                for (let x = m.door ? m.door.home[0] : 0; v && x !== v.vx; x += Math.sign(v.vx - m.door.home[0])) {
                  const p = dq0Portal(m, sn, x, m.door.home[1]);
                  if (p && p.kind) p.b[p.state] = (p.b[p.state] & ~0x06) | 1;
                }
                done = true;
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out);
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=8',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 374 -c /ram/c.hm -k pick -T -d /ram/g',
        'step west', 'dig north', 'wait', 'step east', 'd', 'wait 2 hours', 'wait 2 hours', 'u', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), at = T.of('wait').at, fin = T.of('quit').at, d = T.of('dig north');
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
      name: 'dq0denportc', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's portcullis worked by a denizen (2026-11-02: the minds' leftovers of Phase 3C's one path, DENIZENS.md's a portcullis and a lock, HOUSE.md's denizens and doors, for review): dq0tunnel's scene, the dwarf's tries at the portcullis not given up: kept from the character by the portcullis let down (from the character's side, its winch's), it heaves at it from the far side (E_DEN DE_HEAVES, the character's code: wwin.c's heave, P_LIFT by its sheet), and raises it (DE_RAISES) or, held twice, strains at its bars (DE_BENDS), each told seen or heard; its way there found (the minds' ways: a field made, dq0d's rb).  [dq0tunnel's what, for the scene:] Dingus Quest Zero's tunnellers (slice 4.11, 2026-11-01: DENIZENS.md's tunnellers and builders, HOUSE.md's tunnellers, for review): a hand-written theme's dwarf miners (seed 1, south of the start, wary; a miner digs with the tools it carries, kinds.txt's ways m: its pick, outfits.txt's [role khuzmin]), the character in their guardroom, the portcullis it came in by let down behind it ('close the door'); one of them put in the passage beyond it, hostile, drawn on to where it last sensed the character (the scene set as the first wait's typed: the hub's actor and its record, the minds' drawn and last-sensed square poked).  A portcullis it can't raise, no way nearer: it digs its way round it (dq0d's dig_way, a diagonal round the door: nearer as the crow flies; I_KIT's KT_DIG, the world's wdig.c a spell of ten minutes at a time by the character's rules: a pick's rate on the wall, a Strength test by its sheet), its first spell seen through the portcullis (E_DEN's DE_DIG: \"The dwarf miner digs into the wall.\", or heard, a scraping in rock), the hole kept as it goes (DUG), and through (DE_DUG: the square a passage, told seen, \"A fresh tunnel opens ...\", or heard, rock falling); the dwarf come through it after; both referees",
      init: 't_rc', cycles: 7000e6,   // (2026-11-08, one line of sight: the dwarf sees the character through the bars, heaves the portcullis up and comes at it, knocking it out: the game runs on till it comes round, 6.1G cycles; 3000e6 before)
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7',
        'open the door', 'go through it', 'turn around', 'close the door ahead', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status']) },
      start(m) {
        m.portc = true;
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.tun = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0) {   // (The scene: one dwarf miner beyond the shut door ahead, hostile, drawn on)
                const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
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
                  const R = dq0Rec(m), q = R.at(h[o + L.aName + 12]);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  R.mem[q + R.SN] = sn;
                  R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16), ram = m.taskRam[h[12 + 2]];
                  ram[ad('drawn') + k] = 8;
                  ram[ad('lsx') + 2 * k] = px & 255; ram[ad('lsx') + 2 * k + 1] = px >> 8;
                  ram[ad('lsy') + 2 * k] = py & 255; ram[ad('lsy') + 2 * k + 1] = py >> 8;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.tun = { k, door: [px + dx, py + dy], beyond: [x, y], pc: [px, py], tile };
                }
                done = true;
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), DE_DIG = 24, DE_DUG = 25;
        if (!m.tun) return ['the scene not set: no dwarf miner near, or no portcullis ahead with a floor beyond it'];
        const dens = T.flatMap(t => t.dens).filter(e => e.kind === 'E_DEN' && e.who === m.tun.k + 1), { K } = m.dq0.layout();
        if (!dens.some(e => e.a === K.DE_HEAVES)) f.push('the dwarf never heaved at the portcullis (E_DEN DE_HEAVES)');
        if (!dens.some(e => e.a === K.DE_RAISES || e.a === K.DE_BENDS)) f.push('the portcullis neither raised nor its bars strained at (DE_RAISES, DE_BENDS)');
        if (!/The dwarf miner sets to heaving at the portcullis\.|iron scraping/.test(s)) f.push('its heaving neither seen nor heard');
        for (const e of dens.filter(e => e.a >= K.DE_OPENS)) if (e.w1 !== m.tun.door[0] || e.w2 !== m.tun.door[1]) f.push('a portal worked not the portcullis: ' + [e.a, e.w1, e.w2]);
        const h = m.dq0.bank(0), lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        if (!m.taskRam[h[12 + 2]][parseInt(/^al ([0-9A-F]+) \._rb$/m.exec(lbl)[1], 16)]) f.push('no way found (dq0d\'s rb: the ways\' bank never had)');
        if (f.length) f.push(JSON.stringify(dens.filter(e => e.a).map(e => [e.a, e.w1, e.w2, e.t])) + ' ' + s.slice(s.indexOf('> close the door'), s.indexOf('> close the door') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denportal', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's one path for every actor (Phase 3C, 2026-11-02: DUNGEON.md's one path, DENIZENS.md's doors, HOUSE.md's denizens and doors, for review): a denizen's mind posts the character's own intent (I_PORTAL) and the world carries it out by the character's parts (wdoor.c's, by the world's wport.c).  A hand-written theme's wary bandits (seed 1, dq0kit's); the character (a rope and a shortsword carried more, dq0 -k: twelve pounds, its burden as the scene was written before D13's free pounds) opens the door ahead and shuts it again, and a bandit is put beyond it, hostile, drawn on to where it last sensed the character (the scene set as the first wait's typed: the hub's actor and its record, the minds' drawn and last-sensed square poked): no step nearer but through the shut door, it opens it (E_DEN DE_OPENS at the door's square: 'The bandit opens the door.'), comes through and fights the character; and a bandit fleeing beyond the door (the scene set as the last wait's typed, 2026-11-02: a living one put there, fleeing, the minds' fled poked, whichever way the fight's dice went), the character beyond it, slams the door shut behind it (DE_SLAMS at the door: 'The bandit slams the door shut.', unseen if the character lies senseless as it's done, the user's, 2026-11-14; the door shut in its chunk at the end)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k rope,shortsword',
        'open the door', 'close the door', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status']) },
      start(m) {
        dq0DoorScene(m, 'wait', 'bandit');
        const a = m.acia, read = a.read;       // (The slam's scene, 2026-11-02, as the last wait's typed: whichever way the fight's dice went, a
        let typed = '';                        //   living bandit put beyond the open door, fleeing, the character on the other side: the minds' fled)
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 1 minute' && m.door && m.dq0) {
                const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), at = k => L.actor + k * L.actorSize, u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
                const name = k => { let n = ''; for (let i = 0; h[at(k) + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[at(k) + L.aName + i]); return n; };
                const up = [...Array(L.nactors).keys()].filter(k => h[at(k) + L.aState] && name(k) === 'bandit' && h[at(k) + L.aDoing] !== K.DO_DOWN);
                const on = (k, q) => u16(at(k) + L.aX) === q[0] && u16(at(k) + L.aY) === q[1];
                const k = up.find(j => on(j, m.door.door)) ?? up.find(j => on(j, m.door.beyond)) ?? up[0];
                if (k !== undefined && (m.door.state(...m.door.door) & 1)) {
                  const [x, y] = m.door.beyond, o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aName + 12]);
                  w16(o + L.aX, x);
                  w16(o + L.aY, y);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  h[o + L.aAttitude] = K.AT_UNFRIENDLY;
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
                  const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16), ram = m.taskRam[h[12 + 2]];
                  ram[ad('fled') + k] = 8;
                  ram[ad('sgen') + k] = 0;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.fleer = k;
                }
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), DE_OPENS = 27, DE_SLAMS = 29;
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
        if (f.length) f.push(JSON.stringify(dens.filter(e => e.a).map(e => [e.a, e.w1, e.w2, e.t])) + ' ' + s.slice(s.indexOf('> close the door'), s.indexOf('> close the door') + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denkey', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's doors' keys and a denizen forcing a locked door (Phase 3C's one path, 2026-11-02: HOUSE.md's keys that fit, every actor's; DENIZENS.md's doors): at seed 1 (dq0denportal's wary bandits, the character with an iron key, dq0 -k), the character opens the door ahead and shuts it again; the door given a lock and the key made its own (the scene set as 'lock the door' is typed: the chunk's portal record's lock, the key's size and thick DOOR_KEY's), 'lock the door' locks it ('You lock it with its key.'); a bandit put beyond it, hostile, drawn on (dq0denportal's scene); it has no key, so it strains at the door (E_DEN DE_STRAINS: 'The bandit strains at the door.', or a door creaking) by the character's forcing (the referee's Strength test, its own sheet's), and, held, batters it (DE_BATTERS), each told seen or heard, till it's through or it gives up",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'iron key'",
        'open the door', 'close the door', 'lock the door', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status']) },
      start(m) {
        dq0DoorScene(m, 'wait', 'bandit');
        const a = m.acia, read = a.read;
        let typed = '';
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'lock the door' && m.dq0 && !m.keyed) {   // (The door ahead given a lock, the key the character's)
                const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
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
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), { K } = m.dq0.layout();
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k sword",
        'drop the sword', 'wait', 'wait', 'wait', 'quit', 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '', done = false;
        m.take = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait' && !done && m.dq0) {   // (A bandit put beside the character, its agear unarmed)
                const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
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
                  const R = dq0Rec(m), q = R.at(h[o + L.aName + 12]);
                  R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
                  R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
                  R.mem[q + R.SN] = sn;
                  h[L.denSeq] = h[L.denSeq] + 1 & 255;
                  m.take = { k, did: u16(o + L.aDid) };
                }
                done = true;
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), { L, K } = m.dq0.layout();
        if (!m.take) return ['the scene not set: no bandit near, or no open square beside the character'];
        if (!/> drop the sword[^>]*(Put down\.|You put down the sword\.)/.test(s)) f.push('the sword not put down');
        const picks = T.flatMap(t => [...t.events, ...t.dens]).filter(e => e.kind === 'E_BEAT' && e.beat === 'WB_PICKS' && e.b === m.take.k + 1);
        if (!picks.length) f.push('the bandit never took up a thing (E_BEAT WB_PICKS)');
        if (!/The bandit snatches up its sword\./.test(s)) f.push('not told "The bandit snatches up its sword."');
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
      name: 'dq0follow', near: ['denizens'], what: 'Dingus Quest Zero\'s following (the verbs plan\'s B6, 2026-11-10: DECISIONS.md\'s "Prayer, fasting, swimming and following"), by the words, the sway\'s replies and the character\'s squares: "follow me" an ally\'s order still (dq0fetch\'s seed 21 rat befriended twice, an ally: SW_ORDER OR_FOLLOW, SR_OK); at seed 5, "follow the sack" refused ("The sack isn\'t going anywhere."); down the ladder, "follow the rat" trails a giant rat that keeps its distance ("You follow the giant rat.", walked after it, the world\'s close-in re-aimed each time, looked round for as it goes behind), till it\'s lost ("You\'ve lost the giant rat.", "track" offered)',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', 'echo ml ani 90 >> /ram/a.hm',
        'dq0 -p t -s 21 -c /ram/a.hm -T -R 117', 'befriend the rat', 'befriend the rat', 'follow me', 'quit',
        'dq0 -p t -s 5 -c /ram/a.hm -T', 'follow the sack', 'd', 'wait', 'follow the rat', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), o = dq0Story(out), f = [];
        const fm = T.of('follow me');
        if (!dq0Rep(fm, 'sway', { kind: 'SW_ORDER', order: 'OR_FOLLOW', result: 'SR_OK' }).length) f.push('"follow me" not the ally\'s order (' + JSON.stringify(fm.replies.filter(r => r.req === 'sway')) + ')');
        inTurn(o, ['> follow the sack The sack isn\'t going anywhere.', '> follow the rat You follow the giant rat.', 'You\'ve lost the giant rat. ("track" may find its trail.)'], f);
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g", 'douse the torch', ...Array(12).fill('wait'), 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        m.lurked = [];
        dq0Poke(m, Array.from({ length: 12 }, (_, i) => ({ g: 1, line: 'wait', n: i + 1, call: (h, L, K, at) => { for (let k = 0; k < L.nactors; k++) m.lurked.push(h[at(k) + L.aState]); } })));
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g", 'douse the torch', ...Array(12).fill('wait'), 'rest for an hour', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        const R = this.R = { hid: [], during: [] }, a = m.acia, cpu = m.cpu;
        dq0Poke(m, [{ g: 1, line: 'wait', n: 12, call: (h, L, K, at) => { for (let k = 0; k < L.nactors; k++) R.hid.push(h[at(k) + L.aState]); } }]);
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
        const T = dq0Turns(m), f = [], R = this.R, s = dq0Story(out), rt = T.of('rest for an hour');
        if (!R.hid.some(v => v > 1)) return ['no bandit hidden before the rest: ' + [...new Set(R.hid)].join(',')];
        if (!R.during.length) f.push('the rest not watched');
        const late = R.during.filter(e => e.t > 300 && e.state > 1);     // (Thirty seconds on: its next turn's come)
        if (late.length) f.push('a bandit hidden as the character rests: ' + JSON.stringify(late.slice(0, 4)));
        if (!/You settle down to rest for an hour\.[\s\S]*bandit[^.]*coming towards[\s\S]*You stop waiting\./.test(s)) f.push('not seen coming, the rest not stopped: ' + JSON.stringify(s.slice(s.indexOf('> rest for an hour'), s.indexOf('> inventory'))));
        if (!dq0Ev(rt, 'E_TASK', {}).length) f.push('no E_TASK in the rest');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0robrest', initiative: true, near: ['denizens', 'time'], what: 'Dingus Quest Zero\'s robbers at a sleeper (the user\'s, 2026-11-06: "robbers should come and rob a resting character"; HOUSE.md\'s those that want it, as it rests, for review): a lair of hostile bandits about, the character asleep in the dark: at seed 1 a bandit comes to it and robs it as it lies, unnoticed (the book\'s Legerdemain, its Stealth, against the character\'s Awareness at half asleep: the referee\'s test of it in the minds\' slot, its steps the fewer by half its mastery), its coins alone taken (E_TASK TE_ROBBED, RB_PURSE), the sleep going on, told as it\'s up ("You find your coins gone: someone robbed you as you lay."); at seed 4 one caught at it, nothing taken (no robbery unawares), the sleep stopped ("You wake to find something at your purse!", RB_CAUGHT)',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g", 'douse the torch', 'sleep', 'inventory', 'quit', 'mkdir /ram/h', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/h/theme.txt",
        "dq0 -p t -s 4 -c /ram/c.hm -T -d /ram/h", 'douse the torch', 'sleep', 'quit', 'echo $status']) },   // (A folder of its own: the first's a life gone on
                                                // with, its coins gone; quit as it's asked, the fight with the robber not fought)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], RS_TEST = m.dq0.layout().K.RS_TEST, STEALTH = 4, SOUND = 1, A = dq0Answers(out).filter(a => a.cmd === 'inventory');
        const gs = T.filter(t => /^dq0 -p t/.test(t.line)), sl = gs.map(g => T.of('sleep', g.k));
        if (sl.length !== 2 || sl.some(t => t.k < 0) || A.length !== 1) return ['the games not played'];
        const robs = sl.map(t => [...dq0Ev(t, 'E_TASK', { te: 'TE_ROBBED' }), ...dq0Ev(T[t.k + 1] || t, 'E_TASK', { te: 'TE_ROBBED' })]);
        const tests = t => ({ them: dq0Rep(t, 'dslot', { kind: RS_TEST, a: STEALTH, actor: v => v > 0, other: v => v !== 0 }), it: dq0Rep(t, 'dslot', { kind: RS_TEST, a: SOUND, actor: 0 }),
          stalk: dq0Rep(t, 'dslot', { kind: RS_TEST, a: STEALTH, actor: v => v > 0, other: 0 }) });   // (Its sleight of hand, the asker's dice; its stalk, its own)
        const t0 = tests(sl[0]), t1 = tests(sl[1]), s = dq0Story(out);
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
      name: 'dq0killrest', initiative: true, near: ['denizens', 'time'], what: 'Dingus Quest Zero\'s killers at a sleeper (HOUSE.md\'s those that want it, as it rests, for review, 2026-11-06): a hand-written theme\'s hostile cultists (killers, kinds.txt\'s k), the character asleep in the dark at seed 1: one comes on and strikes the sleeper as its kind\'s rule says, not standing off, the blow with no defence (the book\'s Ignore, RS_ASLEEP)',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Foes | cultist | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g", 'douse the torch', 'sleep', 'echo $status']) },   // (Dead of it: the game over)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], RS_ATTACK = m.dq0.layout().K.RS_ATTACK, sl = T.of('sleep'), s = dq0Story(out);
        if (sl.k < 0) return ['the game not played'];
        const all = [sl, ...T.filter(t => t.k > sl.k)];
        if (!all.some(t => dq0Rep(t, 'slot0', { kind: RS_ATTACK, w2: v => v & 0x80 }).length)) f.push('no blow at the sleeper with no defence (RS_ASLEEP)');
        if (!/You settle down to sleep for eight hours\.[\s\S]*strikes at you/.test(s)) f.push('the sleeper not struck: ' + JSON.stringify(s.slice(s.indexOf('> sleep'), s.indexOf('> sleep') + 400)));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denizens', meaning: true, what: 'Dingus Quest Zero\'s denizens (dq0d: the populations decided as a stratum is grown, from a stand-in theme\'s kinds, kinds.txt), by the actors\' records, their events and what the character perceives (their kinds\' records the referee\'s, HarnMaster\'s): the giant rats that lair by the top stratum\'s ways placed round the way down as the character starts there (dq0 -T), seen (the world\'s actors, in what it perceives), and moving about as time passes (their own task deciding): at the turns pace through all the time a minute\'s search takes (the clock on a slice at a time as they keep up), their stirring told (the world\'s E_VIEW) as what the character sees of them changes (gone out of sight), not again when nothing has; then (seed 5, down its ladder) the rats there seen, cornered, and in the character\'s way: the world stops it short of them, and won\'t let it walk or step onto a rat\'s square (dq0steps\' rat in the way, a step, in it)',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'search', 'search', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'd', 'go right', 'go forward', 'step forward', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', 'echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -d /ram/kept -R 117',
        'persuade the rat', 'examine the rat', 'befriend the rat', 'befriend the rat', 'befriend the rat', 'name the rat grub', 'tell grub to wait', 'turn around',
        'follow me', 'go east', 'turn around', 'd', 'quit', 'dq0 -d /ram/kept -R 117', 'examine grub', 'quit', 'dq0 -p t -s 3 -c /ram/a.hm -T -R 117', 'attack the rat',
        'examine the rat', 'quit', 'echo $status']) },
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', 'echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -R 117',
        'befriend the rat', 'befriend the rat', 'name the rat grub', 'drop 2 rations', 'move east 2 squares', 'turn around', 'tell grub to fetch the rations', 'wait', 'wait',
        'inventory', 'tell grub to fetch the anvil', 'give the tinderbox to grub', 'give 1 ration to grub', 'inventory', 'tell grub to follow me at 3 squares',
        'quit', 'echo $status']) },
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
        if (!dq0Ev(g, 'E_TASK', { te: 'TE_FETCHED', ft: 'FT_GOT' }).length) f.push('the ration not taken (' + dq0Evs(g) + ')');
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', 'echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -R 117 -k chest',
        'befriend the rat', 'befriend the rat', 'name the rat grub', 'drop the chest', 'close the chest', 'put 2 rations on the chest', 'move east 2 squares', 'turn around',
        'tell grub to fetch the rations', 'wait', 'wait', 'inventory', 'quit', 'echo $status']) },
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
      name: 'dq0denpace', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's denizens' paces (the user's, 2026-11-03: \"walk unless hurried, and closing in to attack should be hurried unless they are cautious\": HOUSE.md's their paces), by the time between an actor's steps (its E_DEN DE_MOVEDs: the world's wbody.c, every actor's): a hand-written theme's hostile bandits (seed 1), a kind whose walk is 20 tenths a square: coming at the character, at its jog, 10; a theme's hostile giant rats (seed 3), a shy kind whose walk is 12, cautious: coming on at its walk, no quicker, for all it's hostile; both referees",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/h', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g', 'wait 1 minute', 'wait 1 minute', 'quit',
        "echo 'faction | Gnawers | rat | 0-0 | 30 | - | 0 | h | -' > /ram/h/theme.txt", 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/h', 'wait 1 minute', 'wait 1 minute', 'quit', 'echo $status']) },
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
      name: 'dq0noise', outfit: true, near: ['denizens', 'parser'], what: 'Dingus Quest Zero\'s words said aloud, a knock, and the minds swayed (the verbs plan\'s B2 and B3, 2026-11-10: DECISIONS.md\'s "Words said, a knock, the minds swayed"; minds.c, the world\'s I_NOISE, the denizens\' s_minded), by the words, the denizens\' sway and the senses\' answers (dq0trade\'s hand-written theme, its bandits at seed 1; Rhetoric, Oratory and Acting 95): "say" to a bandit beside and facing it, taken as talk (SW_TALK); a whisper heard at loudness 1, a yell at 6 by every bandit in hearing, a line in quotes said; a knock on the door (walked to and faced) heard at 3; a knock along the north wall, a short search by ear; "wake" awake ("You\'re awake."); a hostile bandit asleep beside it shaken awake, turning on it (SW_WAKE); an indifferent one taunted (SW_TAUNT, Rhetoric) made hostile, coming for it; a speech (SW_PERSUADE | SWO_ALL, one Oratory test) swaying those in earshot; ignorance feigned, a tale spun (deceive\'s); death feigned, lying down, the bandit beside taken in (Acting) and treating it as helpless, a robber robbing it; then seed 7\'s hall, its doors made secret (dq0Poke\'s traps), the door behind found by knocking along the north wall, and a speech with no one to hear it',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml rhe 95 >> /ram/c.hm', 'echo ml ora 95 >> /ram/c.hm', 'echo ml act 95 >> /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g',
        'say "hello there"', 'whisper "psst"', 'yell "wake up!"', '"Hi"', 'knock on the door', 'knock on the north wall', 'wake', 'pretend ignorance', 'pretend dead',
        'wake the bandit', 'taunt the bandit', 'orate', 'quit',
        'echo ml awa 95 >> /ram/c.hm', 'dq0 -p t -s 7 -c /ram/c.hm', 'open the door behind me', 's', 'knock on the north wall', 'orate', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 1, line: 'say "hello there"', n: 1, name: 'bandit', set: { attitude: 3, doing: 1, face: 0 }, beside: 4 },
          { g: 1, line: 'yell "wake up!"', n: 1, name: 'bandit', set: { attitude: 3, doing: 10 } },
          { g: 1, line: 'wake the bandit', n: 1, name: 'bandit', set: { attitude: 0, doing: 10 }, beside: 4 },
          { g: 1, line: 'taunt the bandit', n: 1, name: 'bandit', set: { attitude: 3, doing: 1 }, beside: 4 },
          { g: 1, line: 'pretend dead', n: 1, name: 'bandit', set: { attitude: 0, doing: 1 }, beside: 4 },
          { g: 2, line: 'knock on the north wall', n: 1, traps: { hide: 0, plates: true } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), s = dq0Story(out), f = [];
        inTurn(s, ['> say "hello there" You say, "hello there" You talk with the bandit.', '> whisper "psst" You whisper, "psst"',
          '> yell "wake up!" You yell, "wake up!"', '> "Hi" You say, "Hi"', '> knock on the door You knock on the door.',
          '> knock on the north wall You knock along the north wall, listening for a hollow...', '> wake You\'re awake.',
          '> pretend ignorance The bandit', '> pretend dead You lie still, feigning death. The bandit is taken in.',
          'goes through your things as you lie helpless', '> wake the bandit You shake the bandit awake. It comes to with a snarl, and turns on you!',
          '> taunt the bandit You taunt the bandit. It\'s goaded: it comes for you!', '> orate You hold', 'to those about you, in common.', '> knock on the north wall You knock along the north wall, listening for a hollow...',
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml rhe 95 >> /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/h', 'dq0hour=128', "echo 'faction | Hold | khuzwar | 0-0 | 96 | khuzeld | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt',
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7', 'open the door', 'south', 'wait', 'persuade the dwarf', 'quit', 'cp /ram/c.hm /ram/d.hm', 'echo language khuzdul=10 >> /ram/d.hm', 'dq0 -p t -s 1 -c /ram/d.hm -T -d /ram/h -R 7', 'open the door', 'south', 'wait', 'persuade the dwarf', 'quit',
        'mkdir /ram/e', "echo 'faction | Hold | khuzeld | 0-0 | 96 | - | 0 | w | -' > /ram/e/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/e -R 7', 'open the door', 'south', 'wait', 'persuade the dwarf elder', 'quit',
        'hmchar -q -s 5 -u feudal -c scribe -o /ram/s.hm', 'cat /ram/s.hm', 'grep language /ram/c.hm', 'echo $status']) },
      start(m) {
        const put = (g, line) => [{ g, line, n: 1, name: line.slice(13), set: { attitude: 3, doing: 0 }, beside: 4 }];
        dq0Poke(m, [...put(1, 'persuade the dwarf'), ...put(2, 'persuade the dwarf'), ...put(3, 'persuade the dwarf elder')]);
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml rhe 95 >> /ram/c.hm', 'echo wealth 6000 >> /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k rope',
        'inventory', 'give the bandit 8 pennies', 'what will you give for the oil', 'sell the oil to the bandit', 'inventory', 'how much for the oil',
        'name the bandit bob', 'buy the oil from bob', 'inventory', 'drop the rope', 'sell the rope to bob', 'how much for the rope', 'inventory', 'haggle for the rope', 'inventory', 'sing to bob', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, ['inventory', 'give the bandit 8 pennies', 'what will you give for the oil', 'sell the oil to the bandit', 'how much for the oil', 'name the bandit bob'].map(line => ({ g: 1, line, n: 1, name: 'bandit', set: { attitude: 4, doing: 0 }, beside: 4 }))
          .concat(['buy the oil from bob', 'sell the rope to bob', 'how much for the rope', 'haggle for the rope', 'sing to bob'].map(line => ({ g: 1, line, n: 1, name: 'Bob', set: { attitude: line[1] === 'i' ? 3 : 4, doing: 0 }, beside: 4 }))));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out);
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
        if (!/You sing for Bob\. (It's delighted|It's pleased|It's bored|It isn't moved)/.test(s)) f.push('the song not told');
        if (f.length) f.push(s.slice(s.indexOf('> buy the oil'), s.indexOf('> buy the oil') + 2500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0allydoor', outfit: true, near: ['denizens'], what: "Dingus Quest Zero's ally's errand at a door (2026-11-02: DENIZENS.md's allies, its errands by the one path; the parser's order words open, close, unlock): seed 1's bandits (dq0denportal's), the character opens the door ahead and shuts it again; a bandit put beyond it (dq0DoorScene's) and made the character's ally (dq0Poke's, the same line); \"ask the bandit to open the door\" (SW_ORDER, OR_OPEN, the door the character's own command finds: its square) and waited for: the ally opens it by the character's code for every actor (its mind's I_PORTAL, the world's wport.c: E_DEN DE_OPENS at the door's square, 'The bandit opens the door.')",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g',
        'open the door', 'close the door', 'look', 'ask the bandit to open the door', 'wait', 'wait', 'wait', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 1, line: 'look', n: 1, name: 'bandit', set: { attitude: 5, doing: 0 } }, { g: 1, line: 'ask the bandit to open the door', n: 1, name: 'bandit', set: { attitude: 5 } }]);
        dq0DoorScene(m, 'look', 'bandit');
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), DE_OPENS = 27;
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
      name: 'dq0letdens', near: ['denizens', 'parser'], what: "Dingus Quest Zero's designators, step 4 (the user's design, 2026-11-06, DECISIONS.md's: \"denizens numbered first\"): at seed 3 (-T) three giant rats in sight, each a capital before the things' (the bones'), shown in the look's sentences ((A, C) Two giant rats ... (B) A giant rat ...) and in around, one a line with where it is; a capital where a denizen's name goes (look at B: how it seems, the one named, the denizens' task told which; talk to C); two rats told apart and each attacked by its letter (the scene set: both senseless beside the character, A to its left, C to its right: attack C finishes C and not A, then attack A finishes A); a capital kept while it's remembered (B still B); each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', ...LETDENS, 'quit', 'echo $status']) },
      start(m) {
        this.timer = dq0Timer(m);
        const self = this;
        self.dead = {};
        // (The capital's actor: its record the UI bank's cap's id, CAP_DEN's, the actor's record the byte after its name)
        const capk = (h, L, at, c) => { const u = m.dq0.bank(L.uiBank), o = L.uCap + c * L.ucapSize, id = u[o] | u[o + 1] << 8;
          return id >= 0xFF00 && u[o + 2] === 2 ? [...Array(L.nactors).keys()].find(j => h[at(j) + L.aName] && h[at(j) + L.aName + 12] === (id & 255)) : undefined; };
        const wary = (h, L) => { for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName]) h[L.actor + k * L.actorSize + L.aAttitude] = 2; };
        const pa = { g: 1, line: 'attack C', n: 1, beside: 0, senseless: 6000 }, pc = { g: 1, line: 'attack C', n: 1, beside: 0, senseless: 6000 };
        const pb = { g: 1, line: 'attack C', n: 1, senseless: 60000 };   // (B out of it, senseless where it is: no fight for the test's words)
        const at0 = { g: 1, line: 'attack C', n: 1, call: (h, L, K, at) => { wary(h, L); pa.k = capk(h, L, at, 0); pb.k = capk(h, L, at, 1); pc.k = capk(h, L, at, 2); pa.beside = (h[L.pcFace] + 6) & 7; pc.beside = (h[L.pcFace] + 2) & 7; self.ka = pa.k; self.kc = pc.k; } };
        const note = (line, n = 1) => ({ g: 1, line, n, call: (h, L, K, at, named) => { wary(h, L); self.dead[line] = [self.ka, self.kc].map(k => k === undefined ? '?' : named(k) ? 'up' : 'gone'); } });
        dq0Poke(m, ['look', 'around', 'look at B', 'talk to C'].map(line => ({ g: 1, line, n: 1, call: wary })).concat([at0, pa, pb, pc, note('attack A'), note('around', 2), note('quit')]));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(out).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
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
        said('look at B', /^The giant rat seems wary of you\./);
        said('talk to C', /words won't reach it/);
        said('attack C', /You finish the giant rat where it lies\./);
        said('attack A', /You finish the giant rat where it lies\./);
        said('around', /\(D\) some bones, ahead to your right/, 6);   // (A thing's capital kept; the rats finished gone from it)
        if (/\((A|C)\) a giant rat/.test(seg[6])) f.push('around: a rat finished still told: ' + seg[6]);
        const d = this.dead;
        if (!d['attack A'] || d['attack A'][0] !== 'up' || d['attack A'][1] !== 'gone') f.push('attack C: not C finished and A left: ' + JSON.stringify(d));
        if (!d['around'] || d['around'][0] !== 'gone') f.push('attack A: A not finished: ' + JSON.stringify(d));
        const tm = this.timer(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', ...LETDENS],
          LETDENS.map(l => ({ line: l, want: 'You|The|(' })));
        for (const r of tm) {
          this.notes.push(r.line + ': first letter ' + r.first);
          // (A blow's words the fight's, its first a status line's: as "attack the rat"'s)
          if (!/^(look|around|attack .)$/.test(r.line) && (r.first === null || r.first > 720000)) f.push(r.line + ': its first letter ' + r.first + ' cycles from Enter (the user\'s 720,000)');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0regard', near: ['denizens'], what: 'Dingus Quest Zero\'s gifts, reputation and an ally moving aside (slice 4.8: DENIZENS.md\'s allies and influence, HOUSE.md\'s fetching, gifts and reputation, D10), scenes set by pokes: at seed 21 (dq0fetch\'s game) a giant rat won over and named Grub, the character two squares on and turned round; Grub set in its way as it steps back (the hub\'s actor\'s square, the poke\'s): the step stopped short of it (E_NOWAY, N_DEN), Grub moving aside at once, the step taken again past it; Grub set wary of the character as a ration\'s given it (the poke\'s attitude): its attitude moved by the gift (food, to a beast, the house\'s steps for its worth); set beside the character again (an ally no more, it doesn\'t follow) and given another, its faction\'s regard raised by the two (dq0d\'s head, fatt: a step for its deeds\' points, four)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', 'echo ml ani 90 >> /ram/a.hm', 'dq0 -p t -s 21 -c /ram/a.hm -T -R 117',
        'befriend the rat', 'befriend the rat', 'name the rat grub', 'move east 2 squares', 'turn around', 'move west 1 square', 'move west 1 square',
        'give 1 ration to grub', 'give 1 ration to grub', 'quit', 'echo $status']) },
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
            if (h && grub !== undefined) {
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
            }
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
      name: 'dq0denhands', near: ['denizens'], what: "Dingus Quest Zero's denizens and things by the character's rules, the rest of Phase 3C's one path (2026-11-04: DUNGEON.md's one path, DENIZENS.md's things handled, HOUSE.md's denizens handling things, for review), by the world's beats: dq0denfood's garrison (seed 2), rations the character's let fall, a soldier beside them, friendly, made hungry (dq0hungry's way): it takes them up (WB_PICKS), then eats them at its next look (WB_EATS: five minutes, its mind fed: the hub's dig_t TW_ATE, it wants no more); the next game, one takes them up and is fed, a fellow of its faction put beside it, made hungry: handed over (WB_HANDS: it eats them at its own next look, hungry still); the third, one that's taken them up made to flee (the minds' fled): it lets them fall (WB_DROP), no time, lying where it stood",
      init: 't_rc', cycles: 4000e6,
      machine: { input: (() => {
        const game = d => ['dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/' + d, 'drop the rations', ...Array(12).fill('wait 1 minute'), 'quit'];
        return dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0hour=128', 'mkdir /ram/g', 'mkdir /ram/h', 'mkdir /ram/i', "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt', 'cp /ram/g/theme.txt /ram/i/theme.txt', ...game('g'), ...game('h'), ...game('i'), 'echo $status']);
      })() },
      start(m) {
        const lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) \\._' + n + '$', 'm').exec(lbl())[1], 16);
        const ramOf = h => { const t = h[12 + 2]; return m.taskRam[t]; };
        const place = (h, L, K, at, R, k, x, y, att, fac) => {
          const o = R.at(h[at(k) + L.aName + 12]);
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
        dq0Poke(m, [
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
        this.notes = [0, 1, 2].map(g => ['WB_PICKS', 'WB_EATS', 'WB_HANDS', 'WB_DROP'].map(w => w + ' ' + beats(g, w).length).join(', '));
        if (!beats(0, 'WB_PICKS').length) f.push('the hungry soldier never took up the rations');
        if (!beats(0, 'WB_EATS').length) f.push('the hungry soldier, the rations in hand, never ate (WB_EATS)');
        if (!beats(1, 'WB_HANDS').length) f.push('the fellow, hungry, was never handed the food (WB_HANDS)');
        if (!beats(2, 'WB_DROP').length) f.push('the one fleeing never let what it held fall (WB_DROP)');
        return f;
      },
    },
    {
      name: 'dq0allythrow', outfit: true, meaning: true, near: ['denizens'], what: "Dingus Quest Zero's ally's thing thrown by the character's own throw (2026-11-04: Phase 3C's one path, the things' rest; DENIZENS.md's things handled): dq0denportc's scene, the portcullis let down between them: a dwarf miner made the character's ally, beyond it in its sight, brought back a thing of its own (its errand's state poked: OR_FETCH, taken up); no way nearer, it throws it (I_ROB FE_THROW: things.c's s_throw_at, its name 0; E_BEAT WB_THREW or WB_FELL, seen), the thing no more its own, lying on the character's square or by a band's end on the line",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7',
        'open the door', 'go through it', 'turn around', 'close the door ahead', 'wait', 'wait', 'wait 1 minute', 'quit', 'echo $status']) },
      start(m) {
        const lbl = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
        const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) \\._' + n + '$', 'm').exec(lbl())[1], 16);
        dq0Poke(m, [{ g: 1, line: 'wait', n: 1, call: (h, L, K, at, named) => {
          const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
          const k = [...Array(L.nactors).keys()].find(j => named(j) === 'dwarf miner');
          if (k === undefined) return;
          const x = px + 2 * dx, y = py + 2 * dy, o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aName + 12]), did = u16(o + L.aDid);
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
          const t = h[12 + 2], ram = m.taskRam[t];
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
        if (!/throws you its/.test(o)) f.push('no words of it: ' + o.slice(o.indexOf('> wait'), o.indexOf('> wait') + 900));
        return f;
      },
    },
    {
      name: 'dq0denfood', near: ['denizens'], what: "Dingus Quest Zero's hungry denizen taking food (HOUSE.md's denizens taking things: what it wants, a hungry one food; dq0hungry has hunger's hunting), by the world's beats: dq0daily's garrison (seed 2, soldiers: folk, no robbers), one set beside rations the character's let fall, friendly and idle (dq0Poke's): fed lately, it leaves them; made hungry (the minds' aate, dq0hungry's way), it takes them up (E_BEAT WB_PICKS, by the take rule)",
      init: 't_rc', cycles: 2000e6,
      machine: { input: (() => {
        const game = d => ['dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/' + d, 'drop the rations', ...Array(5).fill('wait 1 minute'), 'quit'];
        return dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0hour=128', 'mkdir /ram/g', 'mkdir /ram/h', "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt', ...game('g'), ...game('h'), 'echo $status']);
      })() },
      start(m) {
        const set = hungry => (h, L, K, at, named) => {
          const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), ad = n => parseInt(new RegExp('^al ([0-9A-F]+) \\._' + n + '$', 'm').exec(lbl)[1], 16), ram = m.taskRam[h[12 + 2]], px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8, nowU = h[L.now + 2] | h[L.now + 3] << 8, R = dq0Rec(m);
          let n = 0;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            if (!n++) {
              const x = px + 1, y = py, o = R.at(h[at(k) + L.aName + 12]);
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
        dq0Poke(m, [1, 2].flatMap(g => [1, 2, 3, 4, 5].map(n => ({ g, line: 'wait 1 minute', n, call: n === 1 ? set(g === 2) : () => {} }))));
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml dge 5 >> /ram/c.hm', 'echo ml una 5 >> /ram/c.hm',
        'echo ml dag 5 >> /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/h', "echo 'faction | Tribe | rat | 0-0 | 5 | garking | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k dagger',
        'defend none', 'hold the dagger', ...Array(16).fill('wait'), 'stand up', 'open the chest', 'open the chest', 'look in the chest', 'take the dagger from the chest', 'quit',
        "echo 'faction | Tribe | hyeka | 0-0 | 96 | - | 0 | h | -' > /ram/h/theme.txt", 'dq0hour=128', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k dagger',
        'defend none', 'hold the dagger', 'open the door', 's 3', ...Array(20).fill('wait'), 'quit', 'echo $status']) },
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/k.hm', 'dq0hour=128', 'echo language khuzdul >> /ram/k.hm',
        'mkdir /ram/g', 'mkdir /ram/g2', 'mkdir /ram/h', 'mkdir /ram/e',
        "echo 'faction | Hold | khuzwar | 0-0 | 96 | khuzeld | 0 | w | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/g2/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -R 7',
        'open the door', 'south', 'wait', 'examine the dwarf', 'talk to the dwarf', 'quit',
        'dq0 -p t -s 1 -c /ram/k.hm -T -d /ram/g2 -R 7', 'open the door', 'south', 'wait', 'talk to the dwarf', 'quit',
        "echo 'faction | Tribe | hyeka khanu | 0-0 | 96 | garking | 0 | h | -' > /ram/h/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -R 7',
        'open the door', 'south', ...Array(8).fill('wait'), 'quit',
        "echo 'faction | Wardens | sindwar | 0-0 | 96 | - | 0 | w | -' > /ram/e/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/e -R 7',
        'open the door', 'south', 'wait', 'wait', 'examine the elf', 'quit', 'echo $status']) },
      start(m) {                                  // (The elves' scene set, 2026-11-01: an elf put beside the character as it waits,
        dq0Poke(m, [{ g: 4, line: 'wait', n: 1, name: 'elf', beside: 2, set: { doing: m.dq0.layout().K.DO_SHY } }]);   //   keeping its distance: their
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
        if (!/An elf/.test(el) || !/keeping its distance/.test(el)) f.push('no elf keeping its distance');
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'search', 'search', 'quit', 'echo $status']) },
      start(m) {   // (The rats made very hungry as the second search is typed: dq0d's aate, each actor's, by its label)
        const a = m.acia, read = a.read;
        let typed = '', n = 0;
        m.hungered = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'search' && ++n === 2 && m.dq0) {
                const h = m.dq0.bank(0), { L } = m.dq0.layout(), tk = h[12 + 2];
                const ad = parseInt(/^al ([0-9A-F]+) \._aate$/m.exec(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'))[1], 16);
                const nowU = h[L.now + 2];
                for (let k = 0; k < L.nactors; k++) {
                  const o = L.actor + k * L.actorSize;
                  let s = '';
                  for (let i = 0; h[o + L.aName + i] && i < 12; i++) s += String.fromCharCode(h[o + L.aName + i]);
                  if (s === 'giant rat') { m.taskRam[tk][ad + k] = nowU - 20 & 255; m.hungered.push(k); }
                }
              }
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k crowns',
        'drop the crowns', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        'mkdir /ram/h', 'cp /ram/g/d1/theme.txt /ram/h/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k farthings', 'drop the farthings', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        'mkdir /ram/i', 'cp /ram/g/d1/theme.txt /ram/i/theme.txt', 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/i -k crowns,farthings', 'i', 'wait', 'wait', 'i', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face' }, { g: 2, line: 'wait', n: 1, name: 'bandit', beside: 'face' },
          { g: 3, line: 'wait', n: 1, name: 'bandit', beside: 'face', engage: true }, { g: 3, line: 'wait', n: 1, call: () => dq0PcShock(m, 3000) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), s = dq0Story(out), G = s.split('% dq0 ').slice(1), f = [];
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/s',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/s/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/s', ...Array(8).fill('wait 30 minutes'), 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'd', 'douse the torch', 'wait 10 minutes', 'look', 'wait 6 hours 30 minutes', 'look', 'wait 2 hours', 'look', 'quit',
        'mkdir /ram/t', 'cp /ram/s/d1/theme.txt /ram/t/theme.txt', 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/t', 'wait 1 minute', ...Array(4).fill('wait 30 minutes'), 'quit', 'echo $status']) },
      start(m) {
        dq0Hour(m, [{ g: 2, line: 'd', n: 1, hour: 10 }]);   // (The rats' game at 10 o'clock: asleep by day)
        const a = m.acia, read = a.read;                      // (The rats' records as each look's typed: dq0Rec's)
        let typed = '';
        m.rats = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 1 minute' && m.dq0) {   // (Game 3: a body of the garrison's in its near lair's home)
                const R = dq0Rec(m), h = m.dq0.bank(0), u16 = o => R.mem[o] | R.mem[o + 1] << 8, w16 = (o, v) => { R.mem[o] = v & 255; R.mem[o + 1] = v >> 8; };
                const MANARMS = 28, at = [...Array(224).keys()].map(i => R.at(i)).filter(o => R.mem[o] === MANARMS);
                const near = at.find(o => R.mem[o + R.FLAGS] & 2), far = at.find(o => !(R.mem[o + R.FLAGS] & 6) && u16(o + 21) !== (near ? u16(near + 21) : -1));
                if (near && far) {
                  R.mem[far + R.FLAGS] = 4 | 8;               // (D_DEAD, D_MET: killed as an actor)
                  w16(far + 8, u16(near + 21));
                  w16(far + 10, u16(near + 23));
                  R.mem[far + R.SN] = R.mem[near + R.SN];
                  const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'), tk = h[12 + 2];
                  m.taskRam[tk][parseInt(/^al ([0-9A-F]+) \._unf$/m.exec(lbl)[1], 16) + (R.mem[far + 25] & 7)] = 1;
                  m.body = { fac: R.mem[far + 25] };
                }
              }
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/t',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/t/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/t', ...Array(12).fill('wait 10 minutes'), 'quit', 'echo $status']) },
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/t',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/t/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/t', 'd', 'wait 1 minute', ...Array(12).fill('wait 10 minutes'), 'quit', 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '';
        m.samples = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13 && m.dq0 && m.dq0.bank(0) && /^wait/.test(typed)) {
              const R = dq0Rec(m), h = m.dq0.bank(0), u16 = o => R.mem[o] | R.mem[o + 1] << 8, w16 = (o, v) => { R.mem[o] = v & 255; R.mem[o + 1] = v >> 8; };
              const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1');
              const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) ._' + n + '$', 'm').exec(lbl)[1], 16), ram = m.taskRam[h[12 + 2]];
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
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/s',
        "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/s/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/s', ...Array(7).fill('wait 30 minutes'), 'wait 1 minute', 'attack the soldier', 'quit',
        'mkdir /ram/u', 'cp /ram/s/d1/theme.txt /ram/u/theme.txt', 'dq0hour=13', 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/u', ...Array(3).fill('wait 30 minutes'), 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, []);                                       // (The actors' moods and doings at each line's Enter: m.hearts)
        const a = m.acia, read = a.read;                      // (One at leisure put beside the character as the first wait
        let typed = '', seen = 0;                             //   of a minute's typed)
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'wait 1 minute' && !seen++ && m.dq0) {
                const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), at = k => L.actor + k * L.actorSize, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
                const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && h[at(j) + L.aDoing] >= K.DO_LEISURE);
                if (k !== undefined) {
                  const d = h[L.pcFace] & 7, x = (h[L.pcX] | h[L.pcX + 1] << 8) + [0, 1, 1, 1, 0, -1, -1, -1][d], y = (h[L.pcY] | h[L.pcY + 1] << 8) + [-1, -1, 0, 1, 1, 1, 0, -1][d];
                  const R = dq0Rec(m), o = R.at(h[at(k) + L.aName + 12]);
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
              }
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
  ],
};
