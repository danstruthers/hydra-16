// soak.js - Dingus Quest Zero soaked (Phase 3A's soak runner): N seeds played at the turns pace by a random player in
// the emulator, each game its own (the tests' machine: sim/test.js's runTest, the SD card's games, the hub writers'
// check and the dq0 tests' own checks with it), and what went wrong told:
//   crashes (the game gone back to the shell by itself, the CPU halted), hangs (no prompt again in the cycles a turn may
//   take), the clock standing still (the status line's minutes the same over many turns), a hub write by the wrong task,
//   a sentence told twice in one answer, a chunk's fixtures doubled; and, to read, how each game ended (its death's
//   cause, the turns lived), a start never left (no other room come into), the lines the grammar refused.
// The player: commands drawn from the game's verbs, weighted toward moving and looking, its things and denizens the
// ones the game's words have named (a thing seen lying, a denizen in sight), by a random stream of the seed's own.
//   [RULES=NAME] node sim/tools/soak.js [-n SEEDS] [-from FIRST] [-seeds A,B,...] [-turns N] [-fight P] [-j N] [-v]
//     -n 20, -from 1: seeds FIRST to FIRST + SEEDS - 1; -turns 60: the commands a game is given before it's quit;
//     -fight P: a denizen in sight attacked with chance P (0-1) each turn, the fights soaked (fighting on, a round's
//     defences: the random player otherwise seldom fights), the cycles a turn may take twice as many;
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

// One game (seed s), played and judged.  OUT: its report { seed, ok, problems: [...], end, turns, refused: [...], left }
function one(s, turns, verbose, fight) {
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
  const res = runTest(t, { verbose: false });
  const o = strip(res.out), problems = [...res.failures.filter(f => !/^missing:/.test(f))];
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
  const turns = opt('-turns', 60), verbose = argv.includes('-v'), fight = opt('-fight', 0);
  if (argv.includes('--one')) { process.stdout.write(JSON.stringify(one(opt('--one', 1), turns, verbose, fight)) + '\n'); return; }
  const list = argv.includes('-seeds') ? argv[argv.indexOf('-seeds') + 1].split(',').map(Number) : null;
  const n = list ? list.length : opt('-n', 20), first = opt('-from', 1), j = opt('-j', os.cpus().length);
  const seeds = list || Array.from({ length: n }, (_, i) => first + i), results = new Array(n);
  let next = 0, running = 0, shown = 0;
  const show = () => {
    while (shown < n && results[shown]) {
      const x = results[shown++];
      console.log((x.ok ? 'ok   ' : 'FAIL ') + String(x.seed).padStart(4) + '  ' + String(x.turns).padStart(3) + ' turns, ' + x.end +
        (x.left ? '' : '; never left its first room') + '; store ' + x.store + ' (' + x.dens + ' the denizens)' + (x.refused.length ? '; refused: ' + x.refused.slice(0, 4).map(l => JSON.stringify(l)).join(', ') : ''));
      for (const p of x.problems) console.log('        ! ' + p);
    }
  };
  return new Promise(done => {
    const start = () => {
      while (running < j && next < n) {
        const i = next++, s = seeds[i];
        running++;
        const args = [__filename, '--one', String(s), '-turns', String(turns), ...(fight ? ['-fight', String(fight)] : []), ...(verbose ? ['-v'] : [])];
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
    const bad = results.filter(x => !x.ok), died = results.filter(x => /^died/.test(x.end)), stuck = results.filter(x => !x.left);
    const refused = new Map();
    for (const x of results) for (const l of x.refused) refused.set(l, (refused.get(l) || 0) + 1);
    console.log(n + ' games: ' + (n - bad.length) + ' clean, ' + bad.length + ' not; ' + died.length + ' died (turns lived ' +
      (died.length ? died.map(x => x.turns).sort((a, b) => a - b).join(', ') : 'none') + '); ' + stuck.length + ' never left the first room');
    if (refused.size) console.log('refused (' + refused.size + '): ' + [...refused].sort((a, b) => b[1] - a[1]).slice(0, 20).map(([l, c]) => JSON.stringify(l) + (c > 1 ? ' x' + c : '')).join(', '));
    process.exit(bad.length ? 1 : 0);
  });
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { choose, noticed, stream };
