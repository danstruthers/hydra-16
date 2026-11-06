// soak.js - Dingus Quest Zero soaked (Phase 3A's soak runner): N seeds played at the turns pace by a random player in
// the emulator, each game its own (the tests' machine: sim/test.js's runTest, the SD card's games, the hub writers'
// check and the dq0 tests' own checks with it), and what went wrong told:
//   crashes (the game gone back to the shell by itself, the CPU halted), hangs (no prompt again in the cycles a turn may
//   take), the clock standing still (the status line's minutes the same over many turns), a hub write by the wrong task,
//   a sentence told twice in one answer, a chunk's fixtures doubled; and, to read, how each game ended (its death's
//   cause, the turns lived), a start never left (no other room come into), the lines the grammar refused.
// The player: commands drawn from the game's verbs, weighted toward moving and looking, its things and denizens the
// ones the game's words have named (a thing seen lying, a denizen in sight), by a random stream of the seed's own.
//   [RULES=NAME] node sim/tools/soak.js [-n SEEDS] [-from FIRST] [-seeds A,B,...] [-turns N] [-fight P] [-life D] [-power N] [-same] [-j N] [-v]
//     -n 20, -from 1: seeds FIRST to FIRST + SEEDS - 1; -turns 60: the commands a game is given before it's quit;
//     -fight P: the fights soaked.  A plain game hardly ever meets a denizen in its 60 turns (2026-11-04: 1 game in 40 told of
//     one), so each seed's game is a scene: hostile folk or beasts of the seed's choosing at the way down where it begins (a
//     theme written by hand, dq0 -T -d), a character sometimes made able, a weapon and armour sometimes given it, and the player
//     random still, a foe named or striking attacked with chance P (0-1) each turn.  Told: the blows, the foes felled, the deaths;
//     a run with too few of them to mean anything fails (FIGHT_MIN);
//     -life D: a character's life soaked, D dungeons ended in one save's folder (dq0 -d) played by the random player, -turns N
//     (20 here) commands each: the game's death or leaving (the way out opened and the character put by it: a scene's pokes, as
//     nothing random finds a puzzle's end), or quit and gone on with, the time between answered at random (dq0acct: no dq0auto;
//     its town, dq0town's menu, each place's words at random, the scholar's house's too (dq0schol), Enter oftenest),
//     hard mode for some seeds, a hostile theme for some; the record (life.dat) read after each game and judged by what it ended in;
//     -power N (1): the machine's power-up RAM, so a seed's game is the same each run of one tree (a failure can be chased: until 2026-11-04 its RAM was
//     random, each run other timings between the tasks, so other words at the player's prompts, so other commands, and it drifted);
//     -same: each game played again with its commands and scenes as they were, by a machine powered up otherwise: the same events, the same replies,
//     or it fails (the turns pace's rule: seed and commands alone, whatever the tasks' timing);
//     -j the cores; -v every game's transcript in obj/soak/SEED.txt (else only the ones that went wrong); RULES, the
//     games with that ruleset's referee (dq0 -r NAME: games/dq0/RULESETS.md)
// Exit status 0 if every game soaked clean.  Each game is a process of its own (--one SEED), the reports in order.
'use strict';
const fs = require('fs');
const path = require('path');
const cp = require('child_process');
const os = require('os');

const ROOT = path.join(__dirname, '..', '..');
const OUT = path.join(ROOT, 'obj', 'soak');
const TURN_CYCLES = 60e6;                       // (A turn's most: a walk across a large room, a fight's round, a look)
const STILL = 40;                               // (Turns with the clock's minutes the same: it's standing still)

// A small random stream (a seed's own: the same game each run)
function stream(seed) {
  let s = (seed * 2654435761) >>> 0 || 1;
  const next = () => { s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0; return s; };
  return { int: n => next() % n, pick: a => a[next() % a.length], chance: p => next() % 1000 < p * 1000 };
}

const strip = s => s.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/[\x00-\x09\x0b-\x1f]/g, '');
const DIRS = ['n', 's', 'e', 'w', 'ne', 'nw', 'se', 'sw', 'north', 'south', 'east', 'west', 'f', 'b', 'l', 'r', 'u', 'd'];
const KIT = ['dagger', 'torch', 'rations', 'waterskin', 'pouch', 'backpack', 'flint', 'flask'];

// The next command: from the game's words since the last (seen: the things lying, the denizens in sight)
function choose(r, seen, fight) {
  const thing = seen.things.length && r.chance(0.7) ? r.pick(seen.things) : r.pick(KIT);
  const den = seen.dens.length ? r.pick(seen.dens) : null;
  const fixture = r.pick(['door', 'window', 'portcullis', 'stairs', 'ladder', 'passage', 'wall', 'trap']);
  if (den && fight && r.chance(fight)) return r.pick(['attack the ' + den, 'attack the ' + den, 'kick the ' + den, 'attack']);
  const k = r.int(100);
  if (seen.ways.length && k < 30) {             // (A way the words named: through it, to it, up or down it)
    const w = r.pick(seen.ways);
    if (w === 'up' || w === 'down') return r.pick([w === 'up' ? 'u' : 'd', 'go ' + w, 'climb ' + w]);
    return r.pick(['go through the ' + w, 'enter the ' + w, 'go to the ' + w, 'go through the ' + w, 'approach the ' + w]);
  }
  if (den && k < 42) return r.pick(['attack the ' + den, 'kick the ' + den, 'talk to the ' + den, 'calm the ' + den, 'threaten the ' + den,
    'look at the ' + den, 'flee', 'defend', 'back away ' + r.pick(['n', 's', 'e', 'w']),
    'target the ' + den, 'throw the dagger at the ' + den, 'throw the ' + thing + ' hard at the closest ' + den, 'attack', 'target']);
  if (k < 45) return r.pick([...DIRS, ...DIRS, 'go ' + r.pick(DIRS), 'run ' + r.pick(DIRS), 'walk ' + r.pick(DIRS), 'go through the door',
    'go to the door', 'enter the door', 'climb the stairs', 'go down the stairs', 'go up', 'turn ' + r.pick(['left', 'right', 'around']),
    'face ' + r.pick(['north', 'south', 'east', 'west']), 'approach the ' + fixture, 'step ' + r.pick(['left', 'right', 'back']),
    'put out the torch', 'walk quickly ' + r.pick(DIRS), 'sprint ' + r.pick(DIRS), 'jog ' + r.pick(DIRS), 'walk ' + r.pick(DIRS)]);   // (The dark's
                                                //   walks, 2026-10-22: feeling the way, or said outright)
  if (k < 65) return r.pick(['look', 'look', 'l', 'look ' + r.pick(DIRS), 'x room', 'examine the ' + fixture, 'search', 'listen',
    'look around', 'look at the ' + thing, 'examine the ' + thing, 'assess the ' + thing, 'judge the ' + thing,   // (A thing's
    'appraise the ' + thing, 'heft the ' + thing, 'what is the ' + thing + ' worth', 'check its condition',      //   aspects:
    'examine the lock of the ' + thing, 'examine the lock', 'check the ' + thing + ' for traps',                 //   2026-10-22)
    'check the door for traps', 'judge the door', 'condition of the ' + thing,
    'hit the door with the ' + thing, 'attack the rat using the dagger', 'dig down with the ' + thing,
    'search nearby', 'search around', 'search the ' + r.pick(['north', 'south', 'east', 'west']) + ' wall', 'search the wall ahead',   // (The
    'search the wall to my left', 'search around for the ' + thing, 'search for the ' + thing, 'search the floor',   //   search's
    'search around on the floor', 'search on the ' + thing, 'search the ' + thing, 'feel around', 'feel about for the ' + thing,   //   forms,
    'grope about', 'search the ' + thing + ' for traps', 'search for traps', 'search the door']);   //   2026-10-22)
  if (k < 80) return r.pick(['inventory', 'i', 'get the ' + thing, 'take the ' + thing, 'drop the ' + thing, 'pick up the ' + thing,
    'light the torch', 'put out the torch', 'hold the dagger', 'put the ' + thing + ' in the backpack', 'eat the rations', 'drink the waterskin',
    'take', 'take all', 'take it', 'drop', 'drop it', 'take the closest', 'clear target', 'dip the torch in the oil', 'fill the lantern',
    'light the torch', 'light the candle']);
  if (k < 90) return r.pick(['open the door', 'close the door', 'unlock the door', 'force the door', 'kick the door', 'break the window',
    'raise the portcullis', 'lower the portcullis', 'disarm the trap', 'bar the door', 'climb through the window']);
  return r.pick(['wait', 'wait', 'wait 5 minutes', 'wait 30 minutes', 'map', 'status', 'sheet', 'rest', 'hide', 'sneak ' + r.pick(['n', 's', 'e', 'w']),
    'dig down', 'dig ' + r.pick(['north', 'east', 'south', 'west']), 'stop', 'stop walking', 'sing a song', 'xyzzy']);
}

// What the game's words since pos have named: things lying, denizens in sight
function noticed(text) {
  const things = [], dens = [];
  for (const m of text.matchAll(/\b(?:lies|lie) (?:a|an|some|\d+) ([a-z]+(?: [a-z]+)?)[.,]/g)) things.push(m[1].split(' ').pop());
  for (const m of text.matchAll(/\b(?:A|An) ([a-z]+(?: [a-z]+)?) (?:is|are) (?:ahead|behind|to your|\d+ feet|beside)/g)) dens.push(m[1].split(' ').pop());
  const ways = [];
  for (const m of text.matchAll(/\b(?:is|are) (?:a|an|two|three|four) (door|doorway|passage|opening|archway|window frame|window|tunnel|portcullis)s?\b/g)) ways.push(m[1].split(' ')[0]);
  for (const m of text.matchAll(/\b(?:stairway|stairs|ladder|shaft)[a-z ]*? (up|down)\b/g)) ways.push(m[1]);
  return { things: [...new Set(things)].slice(-6), dens: [...new Set(dens)].slice(-3), ways: [...new Set(ways)] };
}

// ---- The fights and the lives: a player that reads the whole of each answer as its prompt comes (the plain soak's chooses as
// the answer before's echo begins, its seeds' games kept as they are)

const FIGHT_MIN = { blows: 3, fought: 0.5, felled: 0.2 };   // (A run's least, of at least 20 games: blows a game, of the games a share with a
                                                //   blow by the character, foes felled a game; 2026-11-04's first 50 seeds: 6.2, 78%, 0.42)
// Who's there: the bestiary's kinds (dq0d/kinds.txt's keys), a faction a line (pop.h's), every sort of fighter
const FOES = [
  'Vermin | rat', 'Serpents | adder', 'Crawlers | spider', 'Dead | ghoul', 'Bones | skel', 'Beasts | bear', 'Kobolds | kobold', 'Brigands | bandit',
  'Cult | cultist', 'Tribe | arak', 'Horde | hyeka khanu', 'Hold | khuzwar', 'Diggers | khuzmin', 'Wardens | sindwar', 'Guard | manarms', 'Wights | wight'];
// What it's given, and the skill it's made able in (hmchar's keys): -k's names, the words that use them
const ARMS = [['sword', 'swd'], ['mace', 'clu'], ['spear', 'wpn'], ['axe', 'wpn'], ['staff', 'wpn'], ['falchion', 'swd'], ['club', 'clu'], ['dagger', 'dag']];
const ARMOUR = ['gambeson,mail hauberk', 'mail hauberk,great helm', 'leather tunic,iron helm', 'scale byrnie,round shield', 'kite shield'];

// The scene of seed s's fights: the lines typed before the game, its dq0 line's words, what the player may reach for
function fightScene(s) {
  const r = stream(s + 7777), facs = [r.pick(FOES)], n = () => 12 + r.int(30);
  if (r.chance(0.4)) facs.push(r.pick(FOES));
  if (facs[1] === facs[0]) facs.pop();
  const head = l => l.split(' | ')[0], att = r.chance(0.8) ? 'h' : r.pick(['u', 'w']);
  const lines = facs.map((l, i) => "echo 'faction | " + l + ' | 0-1 | ' + n() + ' | - | 0 | ' + (i ? 'h' : att) + ' | ' + (facs[1 - i] && r.chance(0.7) ? head(facs[1 - i]) + ' h' : '-') + "' " + (i ? '>>' : '>') + ' /ram/g/theme.txt');
  const able = r.chance(0.6), arm = r.chance(0.6) ? r.pick(ARMS) : null, wear = r.chance(0.5) ? r.pick(ARMOUR) : null;
  const sheet = able ? [...(arm ? ['ml ' + arm[1] + ' 95'] : []), 'ml una 70', 'ml dge 95', 'att sta 99', 'att wil 99'].map(l => 'echo ' + l + ' >> /ram/c.hm') : [];
  const kit = [...(arm ? [arm[0]] : []), ...(wear ? wear.split(',') : [])];
  return { lines: [...(r.chance(0.8) ? ['dq0hour=128'] : []), ...sheet, 'mkdir /ram/g', ...lines], kit: kit.length ? " -k '" + kit.join(',') + "'" : '', things: kit };
}

// Foes brought to the character, as none has come for a while (a plain game's denizens hardly ever do): up to n of the denizens'
// records still in their lairs put on squares it's walked in the last turns, or of the room it's in, its own stratum's; a scene's
// poke (tests.js's dq0Rec's, between two commands, the turns pace waiting), the minds going on from there.  D_* as dq0d.c's
function summon(m, r, n, wr) {
  const D_OUT = 1, D_ACTOR = 2, D_DEAD = 4, g = m.dq0.games[m.dq0.games.length - 1], h = m.dq0.bank(0);
  if (!g || !h) return 0;
  const { L } = m.dq0.layout(), sn = h[L.pcSn], x = h[L.pcX] | h[L.pcX + 1] << 8, y = h[L.pcY] | h[L.pcY + 1] << 8, near = (a, b) => Math.max(Math.abs(a - x), Math.abs(b - y));
  const trail = g.snaps.slice(-12).filter(q => q.pc.sn === sn && near(q.pc.x, q.pc.y) >= 2 && near(q.pc.x, q.pc.y) <= 12).map(q => [q.pc.x, q.pc.y]);
  const q = g.snaps[g.snaps.length - 1], w = q && q.sight;
  if (w && w.where === 0 && w.rw && w.rh) for (let i = 0; i < 6; i++) { const a = w.rx + r.int(w.rw), b = w.ry + r.int(w.rh); if (near(a, b) >= 2) trail.push([a, b]); }
  if (!trail.length) return 0;
  const R = dq0Rec(m), nd = 224, free = [];
  for (let i = 0; i < nd; i++) { const o = R.at(i); if (R.mem[o] && !(R.mem[o + R.FLAGS] & (D_ACTOR | D_DEAD))) free.push(o); }
  let brought = 0;
  for (; brought < n && free.length; brought++) {
    const o = free.splice(r.int(free.length), 1)[0], [a, b] = r.pick(trail);
    wr('r', o + R.FLAGS, R.mem[o + R.FLAGS] | D_OUT); wr('r', o + R.SN, sn);
    wr('r', o + R.X, a & 255); wr('r', o + R.X + 1, a >> 8); wr('r', o + R.Y, b & 255); wr('r', o + R.Y + 1, b >> 8);
  }
  return brought;
}

// A foe's name for the words to come: the last of its words, one of it
const oneOf = w => { w = w.split(' ').pop(); return w === 'dwarves' ? 'dwarf' : w === 'elves' ? 'elf' : w.replace(/s$/, ''); };
// The denizens the words name, as they're told now (dens.c's and the blows'): wrapped however, in numbers, in their gear
function foesNamed(text) {
  const t = text.replace(/\s+/g, ' '), dens = [];
  for (const m of t.matchAll(/\b(?:A|An|One|Two|Three|Four) ([a-z]+(?: [a-z]+)??)(?: in mail)?(?:, (?:armed|armoured|armed and armoured),)? (?:is|are) (?:ahead|behind|to your|beside)/g)) dens.push(oneOf(m[1]));
  for (const m of t.matchAll(/\bThe ([a-z]+(?: [a-z]+)?) [a-z]+s at you\b/g)) dens.push(oneOf(m[1]));
  return [...new Set(dens)].slice(-3);
}
const struck = t => /\b(?:strikes|bites|claws|stabs|slashes|kicks|punches|butts|lashes) at you\b|\bat you with its\b/.test(t.replace(/\s+/g, ' '));

// The next command in a fight: as a player might, blows the most, with other ways of fighting among them
function fightLine(r, seen, things, foe, since) {
  const w = things.length ? r.pick(things) : 'dagger';
  if (/out of reach/.test(since) && r.chance(0.8)) return r.pick(['approach the ' + (foe || 'door'), 'go to the ' + (foe || 'door'), 'wait', 'wait', 'wait 1 minute']);
  if (/You see no /.test(since) && r.chance(0.8)) return r.pick(['wait', 'wait', 'look', 'listen', 'attack']);
  if (!foe) return r.pick(['attack', 'attack', 'attack', 'kick', 'target', 'dodge', 'block', 'hold the ' + w, 'hold the dagger']);
  return r.pick(['attack the ' + foe, 'attack the ' + foe, 'attack the ' + foe, 'attack', 'attack', 'fight the ' + foe, 'kick the ' + foe, 'punch the ' + foe,
    'attack the ' + foe + ' with the ' + w, 'hold the ' + w, 'wear the ' + w, 'target the ' + foe, 'throw the dagger at the ' + foe, 'dodge', 'block', 'defend',
    'back away ' + r.pick(['n', 's', 'e', 'w']), 'flee', 'look at the ' + foe, 'look at the body', 'take the closest']);
}

// The record's head and its dungeons, from an xd's lines since a prompt (life.h's: struct life_head, struct life_dungeon)
function lifeRecord(text) {
  const b = [];
  for (const m of text.matchAll(/^\s*[0-9A-Fa-f]{4,8}:?((?: +[0-9A-Fa-f]{2}){1,16})/gm)) for (const x of m[1].trim().split(/ +/)) b.push(parseInt(x, 16));
  if (b.length < 32) return null;
  const u16 = o => b[o] | b[o + 1] << 8, u32 = o => (b[o] | b[o + 1] << 8 | b[o + 2] << 16 | b[o + 3] << 24) >>> 0;
  return { magic: u16(0), flags: b[3], seed: u32(4), dseed: u32(8), dungeon: u16(12), date: u32(14), out: n => b[32 + (n - 1) * 8 + 6] };
}

// A game played again by another machine (-same: another power-up's RAM, so other timings between the tasks) with the same commands and
// scenes: at the turns pace it's the same game, its world's events, the denizens', the replies to each request, record by record
// (tests.js's dq0turnsame's rule, for every game the soak plays); what differs is the game's fault, the first of it told
function sameGame(a, b, s) {
  // (Each ring and each slot a stream of its own: their order between them is the tasks' timing, nothing any of them reads.  An E_LIGHT's only its
  // square and its time mean anything, an E_TASK's x nothing, an RS_TIME's steps nothing: the rest what the RAM or the slot held.  Rest paid (RS_EXERT's
  // GA_REST, GA_STOOD) as the world's looked: in other slices the same in all, told as its sum.  What the hands hold asked by the UI (IO_HANDS) changes
  // nothing, whenever it looks)
  const clean = e => {
    const c = Object.assign({}, e, { line: undefined });
    if (e.kind === 'E_LIGHT') { c.who = c.a = c.b = c.x = undefined; }
    if (e.kind === 'E_BYE') c.t = undefined;
    if (e.kind === 'E_TASK') c.x = undefined;
    if (e.req && e.kind === 9) c.steps = undefined;
    return JSON.stringify(c);
  };
  const told = l => { const b = l.findIndex(e => e.kind === 'E_BYE'); return b < 0 ? l : l.filter((e, i) => !(e.kind === 'E_READY' && i < b && l.slice(i, b).every(q => q.kind === 'E_READY'))); };   // (The end: the quit's moment, a last E_READY or none, its clock a slice on or not)
  const GA_STOOD = 0xFD, GA_REST = 0xFF, restful = e => e.req && e.kind === 10 && (e.a === GA_REST || e.a === GA_STOOD);
  const recs = r => (r.m.dq0 ? r.m.dq0.games : []).map(g => {
    const by = new Map(), rest = g.replies.filter(restful).reduce((t, e) => t + e.w1, 0);
    for (const e of g.replies) if (!restful(e) && e.kind !== 'IO_HANDS') { if (!by.has(e.req)) by.set(e.req, []); by.get(e.req).push(clean(e)); }
    return [...told(g.events).map(clean), ...g.dens.map(clean), ...[...by.keys()].sort().flatMap(k => by.get(k)), 'rest ' + rest];
  });
  const x = recs(a), y = recs(b), f = [];
  if (x.length !== y.length) f.push('not the same game again: ' + x.length + ' games, then ' + y.length);
  for (let g = 0; g < x.length && g < y.length && !f.length; g++) {
    let d = 0;
    const un = l => { const i = l.map(q => q.includes('"req":"slot0"')).lastIndexOf(true); return l.filter((q, k) => k !== i); };   // (One more of a blow's requests as the character dies of it, or none, the world's end its own moment)
    if (x[g].length === y[g].length + 1) x[g] = un(x[g]);
    else if (y[g].length === x[g].length + 1) y[g] = un(y[g]);
    while (d < x[g].length && d < y[g].length && x[g][d] === y[g][d]) d++;
    if (d < x[g].length || d < y[g].length) f.push('not the same game again: its game ' + (g + 1) + ' from record ' + d + ' of ' + x[g].length + ' and ' + y[g].length + ': ' + (x[g][d] || 'none') + ' / ' + (y[g][d] || 'none'));
  }
  if (f.length) { fs.mkdirSync(OUT, { recursive: true }); fs.writeFileSync(path.join(OUT, s + '.same.json'), JSON.stringify({ x, y })); fs.writeFileSync(path.join(OUT, s + '.again.txt'), b.out); }
  return f;
}

let dq0Rec = null;
// One game (or life) played by that player and judged.  OUT: its report, as one()'s, with the fights' and the life's counts
function driven(s, turns, verbose, o) {
  const { runTest } = require('../test.js');
  const { sdcardOf, dq0Rec: rec } = require('../../tests/tests.js');
  dq0Rec = rec;
  const r = stream(s), typed = [], folder = '/ram/g', problems = [];
  const scene = o.fight || r.chance(0.5) ? fightScene(s) : { lines: ['mkdir /ram/g'], kit: '', things: [] };
  const hard = !!o.life && r.chance(0.4) ? ' -H' : '', fight = o.fight || (scene.kit || scene.lines.length > 1 ? 0.7 : 0);
  const rules = process.env.RULES ? ' -r ' + process.env.RULES : '';
  let solving = !!o.life && s % 5 === 0;        // (Some lives' first: the crypt, its puzzle's pieces given, solved as it's left: dq0life's test's words)
  const begin = () => 'dq0 -p t -s ' + (solving ? 3 : s) + ' -c /ram/c.hm' + (o.fight ? ' -T' : '') + ' -d ' + folder + (solving ? " -k 'sunstone,moonstone'" : scene.kit) + hard + rules;
  const ends = [], tally = { left: 0, solved: 0, quit: 0, died: 0, abroad: 0, lives: 1, between: 0, most: 1, brought: 0 };
  const NDUNGEONS = o.life || 1, pending = ['bind -a /sd/0/games /bin', 'hmchar -q -s ' + s + ' -o /ram/c.hm', ...(o.life && r.chance(0.3) ? ["echo 'wealth 3000' >> /ram/c.hm"] : []), ...scene.lines, begin()];
  let gameFrom = 0, who = '', calm = 3, still = 0, lastClock = -1, from = 0, inGame = false, turn = 0, tries = 0, done = false, seen = { things: [], dens: [], ways: [] }, was = null, home = null, leaving = false, expect = null;
  // (What was typed and poked, each time the player was asked: played again as it was by -same, whatever its words then)
  const script = o.script || [];
  let pokes = null, M = null;
  const memOf = w => w === 'h' ? M.dq0.bank(0) : dq0Rec(M).mem;
  const wr = (w, a, v) => { memOf(w)[a] = v; pokes.push([w, a, v]); };
  function asked(m) {
    M = m;
    if (o.script) { const e = script.shift() || { l: null, p: [] }; for (const [w, a, v] of e.p) memOf(w)[a] = v; return e.l; }
    pokes = [];
    const l = decide(m);
    script.push({ l, p: pokes });
    return l;
  }
  function hubPoke(m) {                         // (The way out opened, the character by it: on the top stratum alone)
    const h = m.dq0.bank(0), { L } = m.dq0.layout();
    if (!h || !home || h[L.pcSn] !== home.sn) return false;
    wr('h', L.wayOpen, 1);
    wr('h', L.pcX, home.x & 255); wr('h', L.pcX + 1, home.x >> 8); wr('h', L.pcY, home.y & 255); wr('h', L.pcY + 1, home.y >> 8);
    return true;
  }
  function atShell(m, since) {                  // (A game over: how, told by its words; the record asked after)
    const t = strip(m.out.slice(gameFrom)).replace(/\s+/g, ' ');
    let end = /You are dead\./.test(t) ? 'died' : /\bThe life of /.test(t) ? 'abroad' : /The game is over\./.test(t) ? 'left' : typed[typed.length - 1] === 'quit' ? 'quit' : '';
    const fault = /dq0(?:life|acct|end|town|schol)?: [a-z]/.exec(t);
    if (fault) problems.push('it said: ' + t.slice(fault.index, fault.index + 80));
    if (!end) { problems.push('the game ended by itself after ' + JSON.stringify(typed[typed.length - 1] || '(the start)')); end = 'quit'; }
    ends.push(end);
    tally[end] = (tally[end] || 0) + 1;
    return end;
  }
  function decide(m) {
    const raw = m.out.slice(from), since = strip(raw), rc = /(?:^|[\n ])% $/.test(strip(m.out.slice(-40)));
    const inPrompt = /\x1b\[K(?:\x1b\[\?25h)?> $/.test(m.out.slice(-12));
    from = m.out.length;
    if (pending.length) {
      const l = pending.shift();
      if (/^dq0 /.test(l)) { inGame = true; gameFrom = m.out.length; turn = 0; tries = 0; leaving = false; home = null; seen = { things: [], dens: [], ways: [] }; }
      if (/^xd /.test(l)) m.lifeFrom = m.out.length;
      typed.push(l);
      return l;
    }
    if (done) return null;
    if (rc && !inGame) {                        // (The record read, after a game: judged by how the game ended)
      if (o.life) {
        const rec = lifeRecord(strip(m.out.slice(m.lifeFrom || 0)));
        if (!rec || rec.magic !== 0x464C) problems.push('no record to read after dungeon ' + ends.length + ': ' + since.slice(-120));
        else {
          tally.most = Math.max(tally.most, rec.dungeon);
          const end = ends[ends.length - 1];
          if (expect && expect(rec)) problems.push('the record: ' + expect(rec) + ', after ' + ends.slice(-2).join(', then ') + ' (dungeon ' + rec.dungeon + ', flags ' + rec.flags + ', date ' + rec.date + ')');
          const prev = rec;
          expect = end === 'left' ? (n => { if (n.out(prev.dungeon) === 1 && !prev.counted) { prev.counted = true; tally.solved++; tally.left--; } return n.flags & 2 ? '' : n.dungeon !== prev.dungeon + 1 ? 'left dungeon ' + prev.dungeon + ', now in ' + n.dungeon : n.date <= prev.date ? 'its date not on' : n.out(prev.dungeon) === 0 ? 'its dungeon\'s end not recorded' : ''; })
            : end === 'quit' ? (n => n.dungeon !== prev.dungeon || n.date !== prev.date ? 'quit, and gone on elsewhere: dungeon ' + prev.dungeon + ' to ' + n.dungeon : '')
              : null;
          if (ends.length >= NDUNGEONS) { done = true; typed.push('echo soaked'); return 'echo soaked'; }
          if (end === 'died' && !!hard !== !!(rec.flags & 2)) problems.push('the record after a death' + (hard ? ' in hard mode: its life not over' : ': its life over, with a save to go back to'));
          if (end === 'abroad' && !(rec.flags & 2)) problems.push('the record after a death between dungeons: its life not over');
          if (rec.flags & 2) { tally.lives++; who = ''; expect = n => n.dungeon !== 1 ? 'a life over, and the next not begun at dungeon 1' : ''; pending.push(begin()); }
          else if (end === 'died' && rec.dungeon === 1) { who = ''; pending.push(begin()); }   // (Its first dungeon, maybe nothing kept yet: a new life then, its character to say)
          else {
            if (end === 'left' && r.chance(0.3))   // (Wounds carried out, a scene's: the time between's healing, a death of them among its rolls: dq0account's lines)
              pending.push(...[r.pick(['inj 6 1 3 5 1 2', 'inj 6 1 3 5 0 0', 'inj 13 2 2 3 0 0', 'inj 13 2 3 4 1 1']), 'dealt 0'].map(x => "echo '" + x + "' >> " + folder + '/pc.hm'));
            pending.push('dq0 -p t -d ' + folder + rules);
          }
          return decide(m);
        }
      }
      done = true;
      typed.push('echo soaked');
      return 'echo soaked';
    }
    if (rc) {                                   // (Its end: the game gone back to the shell)
      const end = atShell(m, since);
      inGame = false;
      if (o.life) { pending.push('xd ' + folder + '/life.dat'); const l = pending.shift(); m.lifeFrom = m.out.length; typed.push(l); void end; return l; }
      done = true;
      typed.push('echo soaked');
      return 'echo soaked';
    }
    if (inPrompt && (typed[typed.length - 1] === 'quit' || /You\s+are\s+dead\.|The\s+game\s+is\s+over/.test(since))) return undefined;   // (Its last prompt: it's on its way out)
    if (!inPrompt) {                            // (The time between's own question, dq0acct's: answered at random)
      tally.between++;
      const town = [[/Where to\?[^>]*> $/, ['', '', '', '1', '2', '3', '4', '5', '6', 'inn', 'market', 'buy 2 torches', 'sell the sack', 'mend all', 'rest 3', 'temple', 'scholar', 'read 1', 'xyzzy']],   // (dq0town's: the street's,
        [/At the inn[^>]*> $/, ['', '', '2', 'a week', 'listen', 'buy a rope']], [/At the market[^>]*> $/, ['', '', 'buy a torch', 'buy 3 rations', 'sell the sack',   //   each place's at random;
          'sell my dagger', 'haggle for the lantern', 'how much for the rope', 'xyzzy']], [/At the smith[^>]*> $/, ['', '', 'mend all', 'whet my sword', 'buy a knife',   //   Enter oftenest, so it
          'haggle for a dagger']], [/Whose offer[^>]*> $/, ['', '1', '2', '3', 'take 1']],   //   (dq0schol's, the scholar's house, the plan's step 12:)
        [/At the scholar's house[^>]*> $/, ['', '', '', 'read 1', 'read the journal', 'study 1', 'study 1 for 2 weeks', 'learn ardic', 'learn delvic', 'learn kobold', 'folklore 1', 'heraldry 1', 'reckon 1', 'astrology 1', 'list', 'xyzzy']]].find(([re]) => re.test(since));   //   leaves)
      if (town) { const l = r.pick(town[1]); typed.push(l); return l; }
      const l = /Enter to go on\s*> $/.test(since) ? r.pick(['', '', '', 'x']) : /follow[^>]*> $/.test(since) ? r.pick(['', '1', '2', '3', '4', 'x', '9', 'viash']) :   // (The rumours': which to follow)
        r.pick(['', '', 'swimming', 'sword', 'sword, dodge', 'dodge and climbing and swimming', 'xyzzy', 'the quick brown fox jumps over the lazy dog, twice over, and more']);
      typed.push(l);
      return l;
    }
    // A command of the game's
    const s0 = m.dq0.games.length ? m.dq0.bank(0) : null;
    if (!home && s0) { const { L } = m.dq0.layout(); home = { sn: s0[L.pcSn], x: s0[L.pcX] | s0[L.pcX + 1] << 8, y: s0[L.pcY] | s0[L.pcY + 1] << 8 }; }
    turn++;
    const named = /([A-Z][a-z]+), [^.\n]{1,60}, enters the dungeon\./.exec(since);
    if (named) { if (who && who !== named[1]) problems.push('the character gone on as another: ' + who + ', then ' + named[1]); who = named[1]; }
    const now = noticed(since), foes = fight ? foesNamed(since) : [];
    if (foes.length) now.dens = foes;
    for (const k of ['things', 'dens', 'ways']) if (now[k].length || /You come into|You go /.test(since)) seen[k] = now[k];
    if (/Your? foe|is dead\.|dies\./.test(since) && !foes.length) seen.dens = [];
    calm = foes.length || struck(since) ? 0 : calm + 1;
    if (fight && calm >= 6) { calm = 0; tally.brought += summon(m, r, 1 + r.int(2), wr); }
    const { L: HL } = m.dq0.layout(), clock = s0 ? s0[HL.now] | s0[HL.now + 1] << 8 | s0[HL.now + 2] << 16 : 0;
    still = clock === lastClock ? still + 1 : 0;
    lastClock = clock;
    let l;
    if (turn > turns) {
      if (o.life && !leaving && r.chance(0.8)) leaving = true;
      if (leaving && solving && tries === 0) { tries++; hubPoke(m); l = 'combine the sunstone with the moonstone'; }
      else if (leaving && solving && tries === 1) { tries++; l = 'use the seal on the way out'; solving = false; }
      else if (leaving && tries++ < 5) { if (!hubPoke(m)) tries = 9; l = tries < 9 ? r.pick(['u', 'u', 'climb up', 'go up']) : 'quit'; }
      else l = 'quit';
    } else if (/Keep [a-z]+\?/.test(since) && r.chance(0.7)) l = r.pick(['yes', 'yes', 'no', 'go on']);
    else if (/Which [^?]*\b1\. [^?]*\?\s*>\s*$/.test(since) && r.chance(0.7)) l = r.pick(['1', '2', '1', 'the first', 'the closest']);
    else if (fight && r.chance(fight) && (seen.dens.length || struck(since))) l = fightLine(r, seen, scene.things, seen.dens.length ? r.pick(seen.dens) : null, since);
    else if (fight && still >= 3) l = r.pick(['wait', 'wait', 'wait 1 minute', 'wait 5 minutes', 'listen', 'search']);   // (Its time's standing: the foes with it)
    else l = choose(r, seen, fight);
    typed.push(l);
    return l;
  }
  const t = {
    name: 'dq0soak-' + s, what: 'a soak', init: 't_rc', cycles: (400e6 + turns * TURN_CYCLES) * (NDUNGEONS + (o.life ? 2 : 0)),
    machine: { input: 'ā' },
    expect: ['% echo soaked'],
    start(m) {
      const q = m.acia.rxQueue, shift = Array.prototype.shift;
      q.shift = function () {                   // (The wait for a prompt over: the next line, on the whole of the answer)
        const c = shift.call(this);
        if (c === 'ā' && !this.length) {
          const l = asked(m);
          if (l === undefined) this.push('ā');   // (Waited for again)
          else if (l !== null) { for (const ch of l + '\r') this.push(ch); this.push('ā'); }
        }
        return c;
      };
    }
  };
  sdcardOf(t);
  const res = runTest(t, { verbose: false, seed: o.power });
  if (o.script) return res;
  const out = strip(res.out);
  problems.push(...res.failures.filter(f => !/^missing:/.test(f)));
  if (res.failures.some(f => /^missing:/.test(f))) problems.push('hung: no prompt after ' + JSON.stringify(typed[typed.length - 1] || '(the start)') + ' (' + typed.length + ' lines)');
  const refused = [...new Set(out.split(/\n\s*> /).slice(1).filter(a => /I don't (?:understand that|know how to)/.test(a)).map(a => a.split('\n')[0].trim()))];
  // The fights, by the world's events (E_FIGHT: who struck whom, what it left them in, hub.h's FF_D_*): the actors' numbers
  // 1 on, 0 the character's
  const FF_D_DOWN = 4, FF_D_DEAD = 8, f = { blows: 0, atPc: 0, others: 0, felled: 0, downed: 0, hits: 0 };
  let store = 0, dens = 0;
  for (const g of (res.m.dq0 ? res.m.dq0.games : [])) {
    for (const e of [...g.events, ...g.dens]) {
      if (e.kind !== 'E_FIGHT' || e.out === 'FO_GONE' || e.out === 'FO_BLED' || e.ff === undefined) continue;
      if (e.who === 0) { f.blows++; if (e.ff & FF_D_DEAD) f.felled++; else if (e.ff & FF_D_DOWN) f.downed++; if (/HIT|CRIT|DISPATCH/.test(e.out)) f.hits++; }
      else if (e.def === 0) f.atPc++; else f.others++;
    }
    for (const sn of g.snaps) {
      const by = new Map(sn.items.map(i => [i.n, i])), top = i => { for (let d = 0; i && d < 8 && i.where === 'IW_ITEM'; d++) i = by.get(i.x); return i; };
      if (sn.items.length > store) { store = sn.items.length; dens = sn.items.filter(i => { const p = top(i); return p && p.where === 'IW_DEN'; }).length; }
    }
  }
  if (o.same) problems.push(...sameGame(res, driven(s, turns, false, { fight: o.fight, life: o.life, script: script.map(e => e), power: o.power + 1 }), s));
  const dead = ends.includes('died') || ends.includes('abroad');
  const end = o.life ? ends.join(' ') || 'did not end' : dead ? 'died' : ends[0] || 'did not end';
  const ok = !problems.length;
  if (!ok || verbose) { fs.mkdirSync(OUT, { recursive: true }); fs.writeFileSync(path.join(OUT, s + '.txt'), res.out); }
  return { seed: s, ok, problems, end, turns: typed.length, refused, left: true, cycles: res.m.cpu.cyc, store, dens, fights: f, tally, games: res.m.dq0 ? res.m.dq0.games.length : 0 };
}

// One game (seed s), played and judged.  OUT: its report { seed, ok, problems: [...], end, turns, refused: [...], left }
function one(s, turns, verbose, fight, power, same) {
  const { runTest } = require('../test.js');
  const { sdcardOf } = require('../../tests/tests.js');
  const r = stream(s);
  const typed = [];
  let quitting = false, pushed = 0;
  const t = {
    name: 'dq0soak-' + s, what: 'a soak', init: 't_rc', cycles: 400e6 + turns * TURN_CYCLES * (fight ? 2 : 1),
    machine: { input: 'ābind -a /sd/0/games /bin\rāhmchar -q -s ' + s + ' -o /ram/c.hm\rādq0 -p t -s ' + s + ' -c /ram/c.hm' +
      (process.env.RULES ? ' -r ' + process.env.RULES : '') + '\r' },
    expect: ['% echo soaked'],
    start(m) {
      let out = m.out, from = 0;
      const seen = { things: [], dens: [], ways: [] };
      const a = m.acia, push = l => { typed.push(l); for (const c of 'ā' + l + '\r') a.rxQueue.push(c); };
      Object.defineProperty(m, 'out', { configurable: true, get: () => out, set: v => {
        out = v;
        if (a.rxQueue.length || quitting) return;
        const since = strip(out.slice(from));
        from = out.length;
        const over = /You\s+are\s+dead\.|The\s+game\s+is\s+over/.test(since) || (pushed && /\n% $/.test(strip(out.slice(-40))));
        if (over || pushed >= turns) { quitting = true; if (!over) push('quit'); push('echo soaked'); return; }
        pushed++;
        const now = noticed(since);           // (What the last words named, else what was named before: a new
        for (const k of ['things', 'dens', 'ways']) if (now[k].length || /You come into|You go /.test(since)) seen[k] = now[k];   //   place forgets it)
        push(/Keep [a-z]+\?/.test(since) && r.chance(0.7) ? r.pick(['yes', 'yes', 'no', 'go on']) : choose(r, seen, fight));   // (A long task's
                                              //   question answered, mostly: else a command, which stops it)
      } });
    }
  };
  sdcardOf(t);
  const res = runTest(t, { verbose: false, seed: power });
  const o = strip(res.out), problems = [...res.failures.filter(f => !/^missing:/.test(f))];
  if (same) {                                   // (Its commands again, each at a prompt: -same's, as driven's)
    const again = { name: t.name, what: t.what, init: t.init, cycles: t.cycles, expect: t.expect,
      machine: { input: t.machine.input + typed.map(l => 'ā' + l + '\r').join('') } };
    sdcardOf(again);
    problems.push(...sameGame(res, runTest(again, { verbose: false, seed: power + 1 }), s));
  }
  const game = o.slice(o.indexOf('% dq0 -p t'));
  const dm = /You\s+are\s+dead\./.exec(game), dead = !!dm;           // (Its words wrapped as they may be)
  const quitAt = game.indexOf('> quit'), shellAt = game.indexOf('\n% ', 10);
  if (res.failures.some(f => /^missing:/.test(f)))
    problems.push('hung: no prompt after ' + JSON.stringify(typed[typed.length - 1] || '(the start)') + ' (turn ' + typed.length + ')');
  if (shellAt >= 0 && (quitAt < 0 || shellAt < quitAt) && !dead) problems.push('the game ended by itself after ' + JSON.stringify(typed[typed.length - 1] || '(the start)'));
  // (The start, as the first command finds it: in a room by the stairs up to the way out, what the character perceives
  // says, sim/lib/dq0meaning.js's; a passage's dead end is a dead-end start: tests.js's dq0start0 and dq0start1 hold
  // twelve of the seeds that were, the soak the rest)
  const s0 = res.m.dq0 && res.m.dq0.games.length ? res.m.dq0.games[0].snaps[0] : null;
  if (s0 && s0.sight && (s0.sight.where !== 0 || !s0.sight.open.some(w => w.kind === 'O_WAY_OUT' && w.rel === 255)))
    problems.push('a dead-end start: not in a room by the way out (' + (s0.sight.where ? 'a passage' : 'no way out at its feet') + ')');
  const mins = [...game.matchAll(/(\d+) min\b/g)].map(m => +m[1]);
  let still = 0, most = 0;
  for (let i = 1; i < mins.length; i++) { still = mins[i] === mins[i - 1] ? still + 1 : 0; most = Math.max(most, still); }
  const answers = game.split(/\n> /).length - 1;
  if (mins.length > STILL && most >= STILL * 2 && answers > STILL) problems.push('the clock stood still: its minutes the same for ' + most + ' status lines');
  const refused = [...new Set(game.split(/\n\s*> /).slice(1).filter(a => /I don't (?:understand that|know how to)/.test(a))   // (A prompt
    .map(a => a.split('\n')[0].trim()))];                                                                                 //   indented too)
  const left = /You come into|You go (?:through|down|up|over)|You climb/.test(game);
  const end = dead ? 'died: ' + (game.slice(Math.max(0, dm.index - 240), dm.index).replace(/\s+/g, ' ').split(/(?<=[.!]) /).slice(-3).join(' ')) : quitAt >= 0 ? 'quit' : 'did not end';
  const ok = !problems.length;
  if (!ok || verbose) { fs.mkdirSync(OUT, { recursive: true }); fs.writeFileSync(path.join(OUT, s + '.txt'), res.out); }
  // (The item store's use, the most of its records in use at a snapshot, and of them the denizens' things: IW_DEN and
  // what's in them, the outfits' gear, 2026-10-27)
  let store = 0, dens = 0;
  for (const g of (res.m.dq0 ? res.m.dq0.games : [])) for (const sn of g.snaps) {
    const by = new Map(sn.items.map(i => [i.n, i])), top = i => { for (let d = 0; i && d < 8 && i.where === 'IW_ITEM'; d++) i = by.get(i.x); return i; };
    if (sn.items.length > store) { store = sn.items.length; dens = sn.items.filter(i => { const p = top(i); return p && p.where === 'IW_DEN'; }).length; }
  }
  return { seed: s, ok, problems, end, turns: Math.min(typed.length, turns), refused, left, cycles: res.m.cpu.cyc, store, dens };
}

function main(argv) {
  const opt = (k, d) => { const i = argv.indexOf(k); return i >= 0 ? +argv[i + 1] : d; };
  const life = opt('-life', 0), turns = opt('-turns', life ? 20 : 60), verbose = argv.includes('-v'), fight = opt('-fight', 0), power = opt('-power', 1), same = argv.includes('-same');
  if (argv.includes('--one')) { process.stdout.write(JSON.stringify(life || fight ? driven(opt('--one', 1), turns, verbose, { fight, life, power, same }) : one(opt('--one', 1), turns, verbose, fight, power, same)) + '\n'); return; }
  const list = argv.includes('-seeds') ? argv[argv.indexOf('-seeds') + 1].split(',').map(Number) : null;
  const n = list ? list.length : opt('-n', 20), first = opt('-from', 1), j = opt('-j', os.cpus().length);
  const seeds = list || Array.from({ length: n }, (_, i) => first + i), results = new Array(n);
  let next = 0, running = 0, shown = 0;
  const show = () => {
    while (shown < n && results[shown]) {
      const x = results[shown++];
      console.log((x.ok ? 'ok   ' : 'FAIL ') + String(x.seed).padStart(4) + '  ' + String(x.turns).padStart(3) + ' turns, ' + x.end +
        (x.left ? '' : '; never left its first room') + '; store ' + x.store + ' (' + x.dens + ' the denizens)' +
        (x.fights ? '; brought ' + x.tally.brought + ', blows ' + x.fights.blows + ' (' + x.fights.hits + ' hit, ' + x.fights.felled + ' felled, ' + x.fights.downed + ' down), at it ' + x.fights.atPc + ', foes at each other ' + x.fights.others : '') +
        (x.tally && life ? '; dungeon ' + x.tally.most + ', lives ' + x.tally.lives + ', games ' + x.games + ', answers ' + x.tally.between : '') + (x.refused.length ? '; refused: ' + x.refused.slice(0, 4).map(l => JSON.stringify(l)).join(', ') : ''));
      for (const p of x.problems) console.log('        ! ' + p);
    }
  };
  return new Promise(done => {
    const start = () => {
      while (running < j && next < n) {
        const i = next++, s = seeds[i];
        running++;
        const args = [__filename, '--one', String(s), '-turns', String(turns), ...(fight ? ['-fight', String(fight)] : []), ...(life ? ['-life', String(life)] : []), ...(verbose ? ['-v'] : []), '-power', String(power), ...(same ? ['-same'] : [])];
        cp.execFile(process.execPath, args, { cwd: ROOT, maxBuffer: 1 << 26 }, (err, stdout, stderr) => {
          running--;
          let x;
          try { x = JSON.parse(stdout.trim().split('\n').pop()); }
          catch (e) { x = { seed: s, ok: false, problems: ['its process failed: ' + String(stderr || err).split('\n')[0]], end: '?', turns: 0, refused: [], left: true }; }
          results[i] = x;
          show();
          if (next < n) start(); else if (!running) done();
        });
      }
    };
    start();
  }).then(() => {
    const bad = results.filter(x => !x.ok), died = results.filter(x => /\bdied|abroad/.test(x.end)), stuck = results.filter(x => !x.left);
    const refused = new Map();
    for (const x of results) for (const l of x.refused) refused.set(l, (refused.get(l) || 0) + 1);
    console.log(n + ' games: ' + (n - bad.length) + ' clean, ' + bad.length + ' not; ' + died.length + ' died (turns lived ' +
      (died.length ? died.map(x => x.turns).sort((a, b) => a - b).join(', ') : 'none') + '); ' + stuck.length + ' never left the first room');
    if (refused.size) console.log('refused (' + refused.size + '): ' + [...refused].sort((a, b) => b[1] - a[1]).slice(0, 20).map(([l, c]) => JSON.stringify(l) + (c > 1 ? ' x' + c : '')).join(', '));
    let thin = '';
    const F = results.filter(x => x.fights);
    if (F.length) {
      const sum = k => F.reduce((a, x) => a + x.fights[k], 0), fought = F.filter(x => x.fights.blows).length;
      console.log('fights: ' + sum('blows') + ' blows by the character (' + (sum('blows') / F.length).toFixed(1) + ' a game, ' + sum('hits') + ' hit, ' + sum('felled') + ' foes felled, ' + sum('downed') + ' put down), ' +
        sum('atPc') + ' at it, ' + sum('others') + ' between foes; ' + fought + ' of ' + F.length + ' games with a blow; ' + died.length + ' deaths');
      if (fight && F.length >= 20 && (sum('blows') < FIGHT_MIN.blows * F.length || fought < FIGHT_MIN.fought * F.length || sum('felled') < FIGHT_MIN.felled * F.length)) thin = 'too few fights to mean anything (a game\'s least: ' + FIGHT_MIN.blows + ' blows, ' + FIGHT_MIN.felled + ' felled; ' + FIGHT_MIN.fought * 100 + '% of the games with one)';
    }
    if (life) {
      const T = results.filter(x => x.tally), sum = k => T.reduce((a, x) => a + x.tally[k], 0);
      console.log('lives: ' + sum('left') + ' left, ' + sum('solved') + ' solved, ' + sum('quit') + ' quit and gone on, ' + sum('died') + ' died in a dungeon, ' + sum('abroad') + ' between; ' + sum('lives') + ' lives, ' +
        T.reduce((a, x) => a + x.games, 0) + ' games, the furthest dungeon ' + T.reduce((a, x) => Math.max(a, x.tally.most), 0) + ', ' + sum('between') + ' questions answered');
      if (T.length >= 10 && sum('left') + sum('solved') < T.length) thin = 'too few dungeons left to mean anything: ' + sum('left') + ' in ' + T.length + ' lives';
    }
    if (thin) console.log('FAIL ' + thin);
    process.exit(bad.length || thin ? 1 : 0);
  });
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { choose, noticed, stream };
