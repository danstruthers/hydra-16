// ****************************************************************************
// parser.js - Dingus Quest Zero's tests: the parser and the commands' words.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const path = require('path');
const { typed } = require('../../lib/common.js');
const {
  T_ROCK, T_FLOOR, T_WALL, T_CORR, T_WINDOW, PK_PORTCULLIS, T_DOOR, T_STEPS, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N,
  CSLOT_AT, liveChunks, stratumRec, dq0Story, dq0Prose, inTurn, dq0Answers, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need, dq0Never,
  dq0Carried, dq0Here, dq0Thing, DQ0_I, dq0Typed, dq0Rec, dq0Alone, dq0Scene, dq0SceneNow, dq0OnWords, dq0AnswerTest, dq0Game
} = require('../../lib/dq0.js');

// The action x kind sweep (COMMANDS-CRITIQUE.md, 4.6): every verb typed at every kind of thing in one scene, the
// kitchen at seed 3 (the pot and the well on the floor, the sconce on the wall, the stairs up, the wall; the backpack
// worn, the torch in hand, the sack carried: -k sack), and one rule held: the answer to a line that names a thing
// names it too (done to it, or refused for it: never the verb done to nothing, or to something else, unsaid).  The
// cells that break the rule now are listed (SWEEP_GAPS): a new one fails the test, and so does a listed one that
// keeps the rule now (take it off the list).  A look or an examine answered with the place's own words ("You are in
// ...") breaks it too, whatever they name (the stairs named only as the room's).  The verbs that would end the scene
// (climb the well: down its shaft) come last.  DQ0_SWEEP=1: the gaps, with their answers; DQ0_SWEEP=all: every answer.
const SWEEP_THINGS = [['pot', /pot/], ['backpack', /pack/], ['torch', /torch/], ['sack', /sack/], ['sconce', /sconce/],
  ['stairs', /stair/], ['wall', /wall/], ['well', /well/]];
const SWEEP_VERBS = ['look at', 'examine', 'touch', 'push', 'pull', 'knock on', 'kick', 'smell', 'taste', 'read', 'sit on',
  'search', 'light', 'open', 'close', 'take', 'climb'];
const SWEEP_GAPS = new Set([]);                 // (None left, 2026-11-07: the stairs looked at, examined, sniffed toward)
const SWEEP_SAID = [                             // (Taken, of a thing worn or in hand; lit, of the torch lit in hand: the
  ['take the backpack', 'You wear the backpack already.'],    //   hold's and the wear's words, not "You see none of those
  ['take the sack', 'You hold the sack in your right hand already.'],   //   here."; HOUSE.md's which is lit, not a
  ['light the torch', 'The torch is lit already.'],           //   spare got out of the pack: 2026-11-07)
  ['look at the stairs', 'You stand by the stairs up to the way out. The way out is shut fast.'],   // (An opening
  ['examine the stairs', 'You stand by the stairs up to the way out.'],    //   looked at: its own words, way.c's opened)
  ['smell the stairs', 'You sniff toward the stairway.'],
];
// (Left out: the wall the character faces at the time is the kitchen's trapped door, so "open the wall" opens it and
// its blade ends the game: the wall made a door, a gap of its own, swept once a thing named is never made another)
const SWEEP_SKIP = new Set(['open the wall', 'close the wall']);
// The second sweep (COMMANDS-CRITIQUE.md's §5.2, 2026-11-07): the other verbs that take a thing, those that leave the scene as
// it was (none taken up, put down, hidden, put out), each at every kind of thing, the same rule
const SWEEP2_VERBS = ['eat', 'drink', 'fill', 'look in', 'tie', 'untie', 'wear', 'push', 'pull', 'unlock', 'lock', 'raise',
  'lower', 'disarm', 'force', 'bend', 'bar'];
const SWEEP2_GAPS = new Set([]);
function dq0SweepLines(verbs = SWEEP_VERBS) {
  const out = [];
  for (const [t] of SWEEP_THINGS) for (const v of verbs) if (!SWEEP_SKIP.has(v + ' the ' + t)) out.push(v + ' the ' + t);
  return out;
}
// A thing the resolver chose of several lying about made the one the world's asked of (its id, letters.c's des_seen:
// COMMANDS-CRITIQUE.md's handles travel, 2026-11-07): at seed 3 a torch put down lit where the character stands and
// one unlit a step behind; "take the closest torch" takes the lit one at its feet, not asked which (the world, given
// the name alone, saw two that differ and asked again); DQ0_CLOSEST=1 prints the answers
function dq0ClosestTest() {
  const lines = ['drop the unlit torch', 'step forward', 'drop the lit torch', 'take the closest torch', 'i'];
  return {
    name: 'dq0closest', near: ['parser'], what: 'Dingus Quest Zero\'s resolver\'s choice of several kept (handles travel): "take the ' +
      'closest torch" of a lit one at the character\'s feet and an unlit one a step off takes the lit one, no "Which one?"',
    init: 't_rc', cycles: 300e6 + lines.length * 40e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -k torch -T', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_CLOSEST) for (const l of lines) console.log(l + ' => ' + a(l));
      if (!/Taken\./.test(a('take the closest torch')) || /Which one/.test(a('take the closest torch')))
        f.push('"take the closest torch": not taken at once (its id the world\'s, des_seen\'s): ' + a('take the closest torch').slice(0, 120));
      if (!/a torch \(lit\), in your (left|right) hand/.test(a('i'))) f.push('not the lit one taken: ' + a('i').slice(0, 160));
      return f;
    },
    sdcard: true
  };
}
// One name match (COMMANDS-CRITIQUE.md's §5.1 step F, 2026-11-07): the UI's choosers ask the world's rule of a name
// (a word of it or its start, a plural, the end of a name: "pack" a backpack; the word before it, "black", "giant", in
// the name too: nameis.s's name_is, the world's things.c's named).  At seed 3, the backpack put down at the character's
// feet: "target the pack" and "target the bon" find the backpack and the bones, as the world's "examine the pack" does
// (the resolver's own rule, whole words, saw no pack, no bon: "You see no pack here."); "target the black rat" none (a
// qualifier not in the rats' name), "target the giant rats" a rat (the one still in sight).  DQ0_NAMEMATCH=1 prints the answers
function dq0NameMatchTest() {
  const lines = ['drop the backpack', 'examine the pack', 'target the pack', 'target the bon', 'target the black rat',
    'target the giant rats'];
  return {
    name: 'dq0namematch', near: ['parser'], what: 'Dingus Quest Zero\'s one name match: "pack" names the backpack to ' +
      'the UI\'s resolver as to the world ("target the pack", "examine the pack"), "bon" the bones; "the black rat" none ' +
      'of the giant rats',
    init: 't_rc', cycles: 300e6 + lines.length * 40e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -T', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_NAMEMATCH) for (const l of lines) console.log(l + ' => ' + a(l));
      if (!/backpack/.test(a('examine the pack'))) f.push('"examine the pack": the world found no backpack: ' + a('examine the pack').slice(0, 120));
      if (!/^Your target: the backpack\./.test(a('target the pack')))
        f.push('"target the pack": the resolver found no backpack (its own rule): ' + a('target the pack').slice(0, 120));
      if (!/^Your target: the bones\./.test(a('target the bon')))
        f.push('"target the bon": the resolver found no bones (a word\'s start): ' + a('target the bon').slice(0, 120));
      if (!/^You see no black rat here\./.test(a('target the black rat')))   // (The name said whole, "black rat": the
                                                // parser's, 2026-11-17)
        f.push('"target the black rat": not none (the qualifier): ' + a('target the black rat').slice(0, 120));
      if (!/^(?:Your target: the giant rat\.|Which one\? \(.\) the giant rat)/.test(a('target the giant rats')))
        f.push('"target the giant rats": not the rats asked about: ' + a('target the giant rats').slice(0, 120));
      return f;
    },
    sdcard: true
  };
}
// "take all" and "it" carry the thing, not its word (COMMANDS-CRITIQUE.md's §5.1 step B, 2026-11-07): each thing "take
// all" lists taken by its id (resolve.c's taken_next, letters.c's des_add), and the thing last named kept with its id
// where that's known (a letter's, the resolver's choice, the first words' bound: resolve.c's it_is), so the world
// doesn't choose again by the name.  At seed 3: a torch hidden where the character stands, the lit one put down a step
// ahead; "take all" takes the lit one it sees (and the bones), not the hidden one at its feet (the world, given the
// name, took the nearest it knew).  Then the spare put down there, the lit one a step on: "look at the closest torch"
// and "take it" take the lit one at once (the world, given the name, saw several that differ and asked which).
// DQ0_IT=1 prints the answers
function dq0ItTest() {
  const lines = ['hide the unlit torch', 'step forward', 'drop the lit torch', 'step back', 'take all', 'i',
    'drop the unlit torch', 'step forward', 'drop the lit torch', 'look at the closest torch', 'take it', 'inventory'];
  return {
    name: 'dq0it', near: ['parser'], what: 'Dingus Quest Zero\'s "take all" and "it" by the thing, not its word ' +
      '(handles travel): "take all" takes the lit torch it sees, not one hidden at the character\'s feet; "look at the ' +
      'closest torch" of a lit one at its feet and an unlit one a step off, then "take it", takes the lit one, no "Which one?"',
    init: 't_rc', cycles: 300e6 + lines.length * 40e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -k torch -T', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_IT) for (const l of lines) console.log(l + ' => ' + a(l));
      if (/Which one/.test(a('take all')) || !/a torch \(lit\)/.test(a('i')) || !/bones/.test(a('i')))
        f.push('"take all": not the lit torch it sees taken (each by its id, taken_next\'s): ' + a('take all').slice(0, 80) +
          ' / ' + a('i').slice(0, 160));
      if (!/Taken\./.test(a('take it')) || /Which one/.test(a('take it')))
        f.push('"take it": not taken at once (its id the world\'s, it_is\'s): ' + a('take it').slice(0, 120));
      if (!/a torch \(lit\), in your (left|right) hand/.test(a('inventory')))
        f.push('"take it": not the lit one taken: ' + a('inventory').slice(0, 160));
      return f;
    },
    sdcard: true
  };
}
// A thing carried named by its state, chosen by the UI (sway.c's ix_pick, for the first words) and made the one the
// world's asked of (its id: sway.c's bind), as the world's own chooser would choose it (pick_ and unlike: the UI's
// flags s_ix_make's, the same tests; COMMANDS-CRITIQUE.md's step C, 2026-11-07).  At seed 3 the pack holds potsherds,
// a pot of pitch and a basket, the sherds first: "the full pot" is the pitch (the sherds are neither a container nor
// a fuel: the old flags took them, and the world, given their id, found no full pot); "the open basket" is none (a
// basket doesn't close); "the spent torch" is the one burnt out in hand, not the spare in the pack.  DQ0_STATESEL=1
// prints the answers
function dq0StateSelTest() {
  const lines = ['drop the full pot', 'drop the open basket', 'wait 2 hours', 'drop the spent torch', 'i'];
  return {
    name: 'dq0statesel', near: ['parser', 'gear'], what: 'Dingus Quest Zero\'s state said of a thing carried, chosen ' +
      'once (the UI\'s choice the world\'s, as the world would choose): "drop the full pot" of potsherds and a pot of ' +
      'pitch in the pack puts down the pitch; "drop the open basket" finds none; "drop the spent torch" the one burnt ' +
      'out in hand, not the spare',
    init: 't_rc', cycles: 300e6 + lines.length * 60e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -k potsherds,pitch,basket -T', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_STATESEL) for (const l of lines) console.log(l + ' => ' + a(l));
      if (!/^You (reach for your backpack\.|put down the pot of pitch\.)[\s\S]*Put down\./.test(a('drop the full pot')) || /sherds/.test(a('drop the full pot')))
        f.push('"drop the full pot": not the pot of pitch put down: ' + a('drop the full pot').slice(0, 160));
      if (/Put down|You put down/.test(a('drop the open basket')))
        f.push('"drop the open basket": a basket put down (it doesn\'t close): ' + a('drop the open basket').slice(0, 160));
      if (!/^You put down the spent torch\./.test(a('drop the spent torch')))
        f.push('"drop the spent torch": not the one in hand put down at once: ' + a('drop the spent torch').slice(0, 160));
      if (!/potsherds/.test(a('i')) || /pot of pitch/.test(a('i')) || !/a torch \(lit\)|a torch,|a torch \(/.test(a('i')))
        f.push('what\'s carried after: ' + a('i').slice(0, 300));
      return f;
    },
    sdcard: true
  };
}
// One line of sight (ENGINE-CRITIQUE.md's §4, step 2, 2026-11-08): the senses' lines and the world's agree.  At seed
// 1, dq0tunnel's scene (a hand-written theme's dwarf miners; the character in their guardroom, the portcullis it came
// in by let down behind it), the character's lantern lit, a step back from the bars; a dwarf miner held (wary,
// watching) four squares ahead, at the far end of the passage beyond them.  The character sees it through the bars
// (the look's line, cast.s: a portcullis is seen through, up or down), and a missile would pass them (the world's line
// of fire, wshoot.c's), so the dwarf would see the character too: the character held hidden as it waits (the hub's
// pc_hid poked, so silent: the dwarf's hearing, which answers before its eyes are asked, is out of it), dsense's
// SN_LOOK (it would see it but for that) every time it's asked.  Till 2026-11-08 the denizens' line (eyes.c's
// clear_line) took a lowered portcullis for a shut door: nothing.  DQ0_LINE=1 prints the replies
function dq0LineTest() {
  const lines = ['open the door', 'go through it', 'turn around', 'close the door ahead', 'light the lantern', 'step back',
    'wait 1 minute', 'wait 1 minute', 'wait 1 minute', 'look'];
  return {
    name: 'dq0line', near: ['senses', 'denizens'], what: 'Dingus Quest Zero\'s one line of sight (ENGINE-CRITIQUE.md\'s ' +
      '§4): a dwarf miner beyond a lowered portcullis, seen by the character through its bars (and in a missile\'s line), ' +
      'would see the character too',
    init: 't_rc', cycles: 600e6 + lines.length * 60e6,
    machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
      "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -k lantern -T -d /ram/g -S /ram/s.sc',
      ...lines, 'quit', 'echo $status'] })) },
    start(m) {
      const scene = (h, L, K, at, named) => {
        const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
        const px = u16(L.pcX), py = u16(L.pcY), f = h[L.pcFace], dx = [0, 1, 1, 1, 0, -1, -1, -1][f], dy = [-1, -1, 0, 1, 1, 1, 0, -1][f];
        let k = m.lineK;
        if (k === undefined) k = m.lineK = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'dwarf miner');
        if (k === undefined) return;
        const x = px + 4 * dx, y = py + 4 * dy, o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]);
        dq0Alone(m, h, L, K, at, k);
        w16(o + L.aX, x); w16(o + L.aY, y);
        h[o + L.aSn] = h[L.pcSn];
        h[o + L.aAttitude] = K.AT_WARY;
        h[o + L.aDoing] = K.DO_WATCH;
        R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
        R.mem[q + R.SN] = h[L.pcSn];
        R.mem[q + R.ATTITUDE] = K.AT_WARY;
        h[L.denSeq] = h[L.denSeq] + 1 & 255;
        const sn = h[L.pcSn], S = stratumRec(m, sn), tile = (x, y) => {
          const b = S && liveChunks(m, sn)[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)];
          return b ? b[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1;
        };
        m.line = { k, x, y, px, py, tiles: [1, 2, 3, 4].map(i => tile(px + i * dx, py + i * dy)) };
      };
      const p = [];
      for (let n = 1; n <= 3; n++) p.push({ g: 1, line: 'wait 1 minute', n, call: (h, L, K, at, named) => { scene(h, L, K, at, named); h[L.pcHid] = 1; } });
      p.push({ g: 1, line: 'look', n: 1, call: (h, L, K, at, named) => { scene(h, L, K, at, named); h[L.pcHid] = 0; } });
      dq0Scene(m, p);
    },
    expect: ['% echo $status'],
    check(m) {
      const T = dq0Turns(m), f = [], SN_SEEN = 1, SN_LOOK = 8, T_FLOOR = 1, T_DOOR = 3, T_CORR = 4, l = m.line;
      if (!l) return ['the scene not set: no dwarf miner to hold'];
      dq0Need(f, T.of('close the door ahead'), 'E_PORTAL', { po: 'PO_LOWERED', pk: 'PK_PORTCULLIS' }, 'the portcullis not lowered');
      if (l.tiles.join() !== [T_FLOOR, T_DOOR, T_CORR, T_CORR].join())
        f.push('not the scene: the squares ahead ' + JSON.stringify(l.tiles) + ' (a floor, the portcullis, the passage)');
      const rs = T.filter(t => t.line === 'wait 1 minute').flatMap(t => t.replies).filter(r => r.req === 'dsense' && r.who === l.k &&
        !r.noise && r.x === l.x && r.y === l.y && r.pc.x === l.px && r.pc.y === l.py);
      const look = T.of('look'), sees = look.after && look.after.sight && look.after.sight.dens.some(d => d.x === l.x && d.y === l.y);
      this.notes = ['the dwarf at ' + l.x + ',' + l.y + ', the character at ' + l.px + ',' + l.py + ': its senses ' +
        JSON.stringify(rs.map(r => r.res)) + '; the character sees it: ' + !!sees];
      if (process.env.DQ0_LINE) console.log(this.notes[0]);
      if (!sees) f.push('the character doesn\'t see the dwarf through the lowered portcullis (the scene\'s premise)');
      if (!rs.length) f.push('the dwarf\'s senses never asked as it watched');
      else if (!rs.every(r => r.res & (SN_SEEN | SN_LOOK)))
        f.push('the dwarf the character sees through the bars wouldn\'t see it (no SN_LOOK): ' + JSON.stringify(rs.map(r => r.res)));
      return f;
    },
    sdcard: true
  };
}
// One router (ENGINE-CRITIQUE.md's §4, step 2's second part, 2026-11-09): a denizen's way found (dq0w/walk.inc's
// search, dq0d's route) by the same rules its steps go by.  At seed 1, dq0line's scene: the character come into the
// dwarf miners' guardroom, its lantern lit; a dwarf miner made hostile four squares ahead across the room's floor, the
// others sent off, and a pocket of bad air poked into the chunk on the square halfway between them (its middle there,
// its reach none: that square alone).  A denizen keeps out of bad air, as out of water deeper than it wades (dq0d's
// wades): its greedy step stops short of the pocket, and its way round is found.  Till 2026-11-09 the way (route) took
// no notice of the air nor the water: it led through the pocket, that step was refused, and the dwarf stood at the
// pocket's edge, the character two squares off, for good.  DQ0_ROUTE=1 prints where it was
function dq0RouteTest() {
  const lines = ['open the door', 'go through it', 'light the lantern', 'wait 1 minute', 'wait 1 minute', 'wait 1 minute', 'look'];
  return {
    name: 'dq0route', near: ['denizens'], what: 'Dingus Quest Zero\'s one router (ENGINE-CRITIQUE.md\'s §4): a hostile ' +
      'dwarf miner four squares off across a room, a pocket of bad air on the square between, goes round it to the ' +
      'character (its way found by the rule its steps go by: no deeper water than it wades, no bad air), not stopped at its edge',
    init: 't_rc', cycles: 600e6 + lines.length * 80e6,
    machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
      "echo 'faction | Diggers | khuzmin | 0-0 | 96 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -k lantern -T -d /ram/g -S /ram/s.sc',
      ...lines, 'quit', 'echo $status'] })) },
    start(m) {
      const [AIR, AX, AY, AR, TILE] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, air)',
        'offsetof (struct chunk, ax)', 'offsetof (struct chunk, ay)', 'offsetof (struct chunk, ar)', 'offsetof (struct chunk, tile)']);
      const u16 = (h, o) => h[o] | h[o + 1] << 8;
      // (The chunk of square x, y of stratum sn in the hub's cache: its bank's bytes, its offset there, and the square's)
      const chunk = (sn, x, y) => {
        const S = stratumRec(m, sn), w = m.dq0.bank(WORLD_BANK_N);
        if (!S || !w) return null;
        const cx = (x - S.x0) >> 5, cy = (y - S.y0) >> 5;
        for (let c = 0; c < 24; c++) {
          const sl = CSLOT_AT + c * 8;
          if (!w[sl] || w[sl + 1] !== sn || (w[sl + 2] | w[sl + 3] << 8) !== cx || (w[sl + 4] | w[sl + 5] << 8) !== cy) continue;
          return { b: m.dq0.bank(5 + Math.floor(c / 3)), o: (c % 3) * 0xAA0, lx: (x - S.x0) & 31, ly: (y - S.y0) & 31 };
        }
        return null;
      };
      const tile = (sn, x, y) => { const c = chunk(sn, x, y); return c ? c.b[c.o + TILE + c.ly * 32 + c.lx] & 15 : -1; };
      const where = (h, L, at) => {
        const r = m.route;
        if (!r || r.none) return;
        r.seen.push({ x: u16(h, at(r.k) + L.aX), y: u16(h, at(r.k) + L.aY), px: u16(h, L.pcX), py: u16(h, L.pcY), on: h[at(r.k) + L.aName] !== 0 });
      };
      const scene = (h, L, K, at, named) => {
        const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'dwarf miner');
        if (k === undefined) return;
        const sn = h[L.pcSn], px = u16(h, L.pcX), py = u16(h, L.pcY), DX = [0, 1, 0, -1], DY = [-1, 0, 1, 0];
        // (The way of the four with four squares of floor straight on and a row of floor beside them, its face's first)
        const floor = (j, s, n) => [...Array(n).keys()].every(i => tile(sn, px + (i + 1) * DX[j] + s * DY[j], py + (i + 1) * DY[j] - s * DX[j]) === 1);
        const f = [0, 1, 2, 3].map(j => (j + (h[L.pcFace] >> 1)) & 3).find(j => floor(j, 0, 4) && (floor(j, 1, 3) || floor(j, -1, 3)));
        if (f === undefined) { m.route = { none: true }; return; }
        const dx = DX[f], dy = DY[f], x = px + 4 * dx, y = py + 4 * dy, mx = px + 2 * dx, my = py + 2 * dy;
        const o = at(k), R = dq0Rec(m), q = R.at(h[o + L.aDen]), w16 = (p, v) => { h[p] = v & 255; h[p + 1] = v >> 8 & 255; };
        dq0Alone(m, h, L, K, at, k);
        w16(o + L.aX, x); w16(o + L.aY, y);
        h[o + L.aSn] = sn;
        h[o + L.aAttitude] = K.AT_HOSTILE;
        h[o + L.aDoing] = K.DO_WATCH;
        R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
        R.mem[q + R.SN] = sn;
        R.mem[q + R.ATTITUDE] = K.AT_HOSTILE;
        h[L.denSeq] = h[L.denSeq] + 1 & 255;
        const c = chunk(sn, mx, my);
        if (c) { c.b[c.o + AIR] = 0x13; c.b[c.o + AX] = c.lx; c.b[c.o + AY] = c.ly; c.b[c.o + AR] = 0; }
        // (The squares ahead, and each side's, for the scene's premise)
        const side = s => [1, 2, 3].map(i => tile(sn, px + i * dx + s * dy, py + i * dy - s * dx));
        m.route = { k, x, y, px, py, mx, my, chunk: !!c, ahead: [1, 2, 3, 4].map(i => tile(sn, px + i * dx, py + i * dy)), left: side(1), right: side(-1), seen: [] };
      };
      dq0Scene(m, [{ g: 1, line: 'wait 1 minute', n: 1, call: scene },
        { g: 1, line: 'wait 1 minute', n: 2, call: (h, L, K, at) => where(h, L, at) },
        { g: 1, line: 'wait 1 minute', n: 3, call: (h, L, K, at) => where(h, L, at) },
        { g: 1, line: 'look', n: 1, call: (h, L, K, at) => where(h, L, at) },
        { g: 1, line: 'quit', n: 1, call: (h, L, K, at) => where(h, L, at) }]);
    },
    expect: ['% echo $status'],
    check(m) {
      const f = [], r = m.route, T_FLOOR = 1;
      if (!r) return ['the scene not set: no dwarf miner to make hostile'];
      if (r.none) return ['not the scene: no four squares of floor straight on from the character, a row of floor beside them'];
      const near = r.seen.map(s => Math.max(Math.abs(s.x - s.px), Math.abs(s.y - s.py)));
      this.notes = ['the dwarf put at ' + r.x + ',' + r.y + ', the character at ' + r.px + ',' + r.py + ', the pocket at ' + r.mx + ',' + r.my +
        '; ahead ' + JSON.stringify(r.ahead) + ', left ' + JSON.stringify(r.left) + ', right ' + JSON.stringify(r.right) + '; then ' +
        r.seen.map(s => s.x + ',' + s.y + (s.on ? '' : ' (gone)')).join(' / ')];
      if (process.env.DQ0_ROUTE) console.log(this.notes[0]);
      if (!r.chunk || r.ahead.join() !== [T_FLOOR, T_FLOOR, T_FLOOR, T_FLOOR].join() ||
        !(r.left.every(t => t === T_FLOOR) || r.right.every(t => t === T_FLOOR)))
        f.push('not the scene (four squares of floor ahead, a row of floor beside them): ' + this.notes[0]);
      if (r.seen.some(s => s.x === r.mx && s.y === r.my)) f.push('the dwarf walked into the bad air: ' + this.notes[0]);
      if (!near.some(n => n <= 1)) f.push('the dwarf never came round the pocket of bad air to the character (its way led through it): ' + this.notes[0]);
      return f;
    },
    sdcard: true
  };
}
// §5.1's last disagreements (COMMANDS-CRITIQUE.md, 2026-11-07): a capital for a thing lying about worded as that thing,
// not a carried one of its name (7: sway.c's first words, its handle's); several alike lying there, "take the sack",
// no "You look them over." (5: the world asks which only of two that differ); DQ0_LEFTOVERS=1 prints the answers
function dq0LeftoversTest() {
  const lines = ['drop the unlit torch', 'around', 'take E', 'drop the sack', 'drop the sack', 'take the sack'];
  return {
    name: 'dq0leftovers', near: ['parser'], what: 'Dingus Quest Zero\'s first words of the thing meant: a capital for a ' +
      'torch lying at the character\'s feet worded as that one, not the lit one in hand; two sacks alike, "take the sack" ' +
      'reached for, no "You look them over." (the world asks which only of two that differ)',
    init: 't_rc', cycles: 300e6 + lines.length * 40e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -T -k \'torch,sack,sack\'', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.filter(x => x.cmd === l).pop() || { text: '(no answer)' }).text;
      if (process.env.DQ0_LEFTOVERS) for (const l of lines) console.log(l + ' => ' + a(l));
      if (!/\(E\) a torch, at your feet/.test(a('around'))) f.push('the torch put down not E: ' + a('around').slice(0, 160));
      else if (!/^You reach for the torch\. Taken\./.test(a('take E'))) f.push('"take E": not worded as the torch at its feet: ' + a('take E').slice(0, 120));
      if (/look them over/.test(a('take the sack')) || !/^You reach for the sack\./.test(a('take the sack')))
        f.push('"take the sack", two alike: ' + a('take the sack').slice(0, 120));
      return f;
    },
    sdcard: true
  };
}
// One path for things (COMMANDS-CRITIQUE.md's §5.2 step H, 2026-11-07): every verb that acts on a thing asks the
// world the same way (main.c's things with IO_GO: out of reach, walked over to first and the command again; several
// that differ, which asked).  At seed 3 the bones lie beside the character: "hide the bones" said "You carry none of
// those." (the hide looked only at what's carried); now they're taken up first, then hidden, as HOUSE.md's hiding a
// thing (where it stands, the thing put down at its end).  Then the dagger put down and the character gone over to the
// window, "hide the dagger" walks back to it first (said "You carry none of those." too), takes it and hides it.
// DQ0_ONEPATH=1 prints the answers
function dq0OnePathTest() {
  const lines = ['around', 'hide the bones', 'i', 'drop the dagger', 'go to the window', 'look', 'hide the dagger', 'inventory'];
  return {
    name: 'dq0onepath', near: ['parser'], what: 'Dingus Quest Zero\'s one path for things: "hide the bones" of bones ' +
      'lying beside the character takes them up first and hides them (not "You carry none of those."); "hide the ' +
      'dagger" of one out of reach walks over to it first, as any verb at a thing out of reach',
    init: 't_rc', cycles: 300e6 + lines.length * 60e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -T', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_ONEPATH) for (const l of lines) console.log(l + ' => ' + a(l));
      if (!/bones/.test(a('around'))) f.push('no bones in sight: ' + a('around').slice(0, 160));
      if (/carry none/.test(a('hide the bones')) || !/hid/i.test(a('hide the bones')))
        f.push('"hide the bones", lying beside it: not taken up and hidden: ' + a('hide the bones').slice(0, 160));
      if (/bones/.test(a('i'))) f.push('the bones carried still after the hide: ' + a('i').slice(0, 200));
      if (!/go over to the/.test(a('hide the dagger')) || !/hid/i.test(a('hide the dagger')))
        f.push('"hide the dagger", out of reach: not walked over to and hidden: ' + a('hide the dagger').slice(0, 200));
      return f;
    },
    sdcard: true
  };
}
function dq0FeaturesTest() {
  const near = ['examine the door to my right', 'look at the door to my right', 'check the door to my right for traps'];
  const a = ['n', 's', 's', 'e', ...near.flatMap(l => ['face north', l])];
  const b = ['step west', 's', 'exit the door', '1'];
  return {
    name: 'dq0features', near: ['parser', 'map'], what: 'Dingus Quest Zero\'s one path for features (COMMANDS-CRITIQUE.md\'s ' +
      '§5.2 step J: found, walked up to, faced, its first words, then the command\'s own act): at seed 8, beside a door to its ' +
      'right, "examine the door to my right" turns to face it as "look at" and "check ... for traps" do (it turned only to ' +
      'one behind it); at seed 3 in the passage off the cell, "exit the door" by the door beyond the passage walks up to it ' +
      'first, as "go through the door" does ("You head for the door.", not "You can\'t go that way.")',
    init: 't_rc', cycles: 1400e6,
    // (The second game -R 2, felled1's, 2026-11-21: a felled one's shock rolls a round each as the time comes, the dice in
    // another order, a rat engaged it on its way out; a search of -R 1-14 found 2, 7, 9, 12 and 14)
    machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g',
      'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
      'dq0 -p t -s 8 -c /ram/c.hm -S /ram/s.sc', ...a, 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -R 2 -S /ram/s.sc', ...b, 'quit', 'echo $status']) },
    expect: ['% echo $status'],
    check(m, out) {
      const T = dq0Turns(m), A = dq0Answers(out), f = [], said = l => (A.filter(y => y.cmd === l).pop() || { text: '' }).text;
      if (process.env.DQ0_FEATURES) for (const y of A) console.log('  > ' + y.cmd + ' => ' + y.text.slice(0, 300));
      for (const l of near) {
        const t = T.of(l);
        if (!t.at || t.at.pc.face !== 0) f.push(l + ': not begun facing north, the door to its right');
        else if (!t.after || t.after.pc.face !== 2) f.push(l + ': not turned to face the door (east): facing ' + (t.after && t.after.pc.face));
      }
      if (!/A door of wood/.test(said(near[0]))) f.push(near[0] + ': not examined: ' + said(near[0]).slice(0, 140));
      const x = T.of('1'), w = said('1');
      if (!/Which door\?/.test(said('exit the door'))) f.push('exit the door: not asked which: ' + said('exit the door').slice(0, 140));
      if (!/You head for the door\./.test(w) || /can't go that way/.test(w)) f.push('exit the door (1): not walked up to it: ' + w.slice(0, 140));
      if (!x.at || !x.after || x.after.pc.x === x.at.pc.x && x.after.pc.y === x.at.pc.y) f.push('exit the door (1): never moved');
      return f;
    },
    sdcard: true
  };
}
// The parser's quick fixes (GAME-CRITIQUE.md's section 10, phase A, 2026-11-17: its 2.2, 2.6 and the play test's
// section 1), at seed 3 in the kitchen (dq0 -T, a kit of things more: a chest and its key, a rope, a case, a chisel, a
// table, a dagger): the other thing's phrase its head, the last word ("with the iron key": the key, not "iron"); a part
// of the place after the thing and its preposition the other's, refused as where a thing's put or tied ("You can't put
// the torch on the door.", not the belt's "won't hang"; a pillar a tie point, the user's, 2026-11-17: "You see no pillar
// here.", none beside it, not the nearest holder); a part of the place its verb can't use said so, not "Push the wall
// what?"; a word that names nothing to open
// said so ("You see no drawer here."), not the door ahead worked; words said with no quotes, the form shown; "eat"
// alone a meal's, as "drink" alone; "pick the lock" the one in front picked (the window: no lock to pick, as "unlock"
// says); "take all from the case" each thing in it named, one list (the user's, 2026-11-17), not a handle's "#17"; a
// surface's "on", not "in"; "drop all" what's in its hands, each put down, nothing worn;
// "name" alone asks of its own line.  DQ0_PARSEFIX=1 prints the answers
function dq0ParseFixTest() {
  const lines = ['drop the chest', 'unlock the chest with the iron key', 'put the torch on the door', 'tie the rope to the pillar',
    'push the wall', 'taste the wall', 'open the drawer', 'say hello', 'eat', 'drink', 'unlock the window', 'pick the lock', 'pick lock',
    'drop the case',
    'put the chisel in the case', 'put the key in the case', 'take all from the case', 'take all from the case', 'drop the table',
    'take the dagger from the table', 'take all from the table', 'i', 'drop all', 'i', 'name'];
  return {
    name: 'dq0parsefix', near: ['parser'], what: 'Dingus Quest Zero\'s parser\'s quick fixes (GAME-CRITIQUE.md\'s phase A, ' +
      '2026-11-17): "unlock the chest with the iron key" the key\'s (the other thing\'s head, its last word); "put the ' +
      'torch on the door" refused plainly (a part of the place where it\'s put); "tie the rope to the pillar", none ' +
      'beside it, "You see no pillar here." (a pillar a tie point, the user\'s, 2026-11-17), not the nearest holder; "push the ' +
      'wall" "You can\'t push the wall."; "open the drawer", none about, "You see no drawer here."; "say hello" its form ' +
      'shown; "eat" alone as "drink" alone; "pick the lock" as "unlock" at the window; "take all from the case" each thing ' +
      'in it named, one list; "on ' +
      'the table" for a surface; "drop all" what\'s in its hands; "name" alone',
    init: 't_rc', cycles: 400e6 + lines.length * 50e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,key,rope,case,chisel,table,dagger\'', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], all = l => A.filter(x => x.cmd === l).map(x => x.text),
        a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_PARSEFIX) for (const l of lines) console.log(l + ' => ' + a(l));
      const one = t => t.split(/(?<=[.!?]) /)[0];   // (Its first sentence: the rats' stirs after it, the scene's)
      const want = (l, re, why) => { if (!re.test(a(l))) f.push('"' + l + '" ' + why + ': ' + a(l).slice(0, 160)); };
      want('unlock the chest with the iron key', /You unlock the chest/, 'not unlocked with the key (the other thing\'s head)');
      if (/won't do for a lock/.test(a('unlock the chest with the iron key'))) f.push('the key named by "iron"');
      want('put the torch on the door', /You can't put the torch on the door\./, 'not refused plainly');
      want('tie the rope to the pillar', /You see no pillar here\./, 'not refused plainly, none beside it');
      want('push the wall', /You can't push the wall\./, 'not said so');
      want('taste the wall', /You can't taste the wall\./, 'not said so');
      want('open the drawer', /You see no drawer here\./, 'not said so');
      want('say hello', /Put the words in quotes: say "hello"\./, 'its form not shown');
      want('eat', /^You're not hungry\./, 'not a meal\'s');
      want('drink', /^You're not thirsty\./, 'not a drink\'s');
      for (const l of ['pick the lock', 'pick lock']) {   // (The one in front, the window frame ahead: its words "unlock the
        if (!a('unlock the window').includes(one(a(l))) || /understand/.test(a(l))) {   //   window"'s, after its walk up to it)
          f.push('"' + l + '" not "unlock the window": ' + a(l).slice(0, 120) + ' / ' + a('unlock the window').slice(0, 120));
        }
      }
      const [t1, t2] = all('take all from the case');
      if (!/You take (?:a chisel and an iron key|an iron key and a chisel) from the case\./.test(t1 || '')) {
        f.push('"take all from the case": not each named, one list: ' + (t1 || '').slice(0, 160));
      }
      if (!/You find nothing in the case\./.test(t2 || '')) f.push('"take all from the case" again: not said empty: ' + (t2 || '').slice(0, 160));
      want('take the dagger from the table', /You find no dagger on the table\./, 'not "on" a surface');
      want('take all from the table', /You find nothing on the table\./, 'not "on" a surface');
      if (/#\d/.test(A.map(x => x.text).join(' '))) f.push('a handle told: ' + (/[^.]*#\d[^.]*/.exec(A.map(x => x.text).join(' ')) || [''])[0]);
      const [i1, i2] = all('i'), held = /in your (?:right|left) hand|in both hands/;
      if (!held.test(i1 || '')) f.push('nothing in hand before "drop all": ' + (i1 || '').slice(0, 200));
      if (held.test(i2 || '')) f.push('"drop all": something still in hand: ' + (i2 || '').slice(0, 200));
      if (!/You put down/.test(A.filter(x => /^drop [a-z]{1,2}$/.test(x.cmd)).map(x => x.text).join(' ') + a('drop all'))) {
        f.push('"drop all": nothing put down: ' + a('drop all').slice(0, 160));
      }
      if (!/backpack, worn/.test(i2 || '')) f.push('"drop all": the worn backpack not kept: ' + (i2 || '').slice(0, 200));
      want('name', /^Name what\?/, 'not asked of its own line');
      return f;
    },
    sdcard: true
  };
}
// "And" for "with", and the "Which one?" choices each their own capital (GAME-CRITIQUE.md's section 1, the play test's
// bugs, 2026-11-17): at seed 3 (the crypt, the sunstone and the moonstone given) "combine the sunstone and the moonstone"
// one line, the stones made one (it was two lines, "Combine what with what?" twice); then two daggers put down where
// the character stands, "go to the dagger" asks which, each dagger its own capital (the resolver's choices by their
// place in the sight record, not their square: the play test's three cave adders on one square all (G)), no side said
// of two on one square.  DQ0_PAIRS=1 prints the answers
function dq0PairsTest() {
  const lines = ['combine the sunstone and the moonstone', 'i', 'drop the dagger', 'drop the dagger', 'go to the dagger'];
  return {
    name: 'dq0pairs', near: ['parser'], what: 'Dingus Quest Zero\'s "combine X and Y" one line, the one with the other ' +
      '(the play test\'s "Combine what with what?" twice), and the resolver\'s "Which one?" choices on one square each ' +
      'their own capital (the play test\'s three adders all (G)), no side said of them',
    init: 't_rc', cycles: 300e6 + lines.length * 60e6,
    machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 909 -u feudal -c sage -o /ram/c.hm', lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -k \'sunstone,moonstone,dagger,dagger\'', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_PAIRS) for (const l of lines) console.log(l + ' => ' + a(l));
      if (/Combine what with what/.test(a('combine the sunstone and the moonstone')) || !/seal/.test(a('combine the sunstone and the moonstone')))
        f.push('"combine the sunstone and the moonstone": not made one: ' + a('combine the sunstone and the moonstone').slice(0, 160));
      if (!/seal/.test(a('i'))) f.push('no seal carried after the combine: ' + a('i').slice(0, 200));
      const q = a('go to the dagger'), caps = [...q.matchAll(/\(([A-Z]{1,2})\) the dagger/g)].map(x => x[1]);
      if (!/^Which one\?/.test(q) || caps.length !== 2) f.push('"go to the dagger": not asked which of two: ' + q.slice(0, 160));
      else if (caps[0] === caps[1]) f.push('"go to the dagger": both daggers the same capital: ' + q.slice(0, 160));
      if (/on the (left|right)/.test(q)) f.push('"go to the dagger": a side said of two on one square: ' + q.slice(0, 160));
      return f;
    },
    sdcard: true
  };
}
// The grammar's roles and its forms (GAME-CRITIQUE.md's 2.8 steps 1 and 3, phase B, 2026-11-17): at seed 3 (-T, a kit
// of a chest, a key, a table, a chair and a stone chisel, the table and the chair set down) a thing named whole ("the
// stone chisel": its qualifier and its head, not the plain chisel); the other thing's name its own ("remove the key
// from the chest": taken from it); "sit in the chair" as "sit on the chair"; "climb on the table" refused plainly, "get
// off" with nothing it's on (not a stairway down taken for it); "look under the table" and "look behind the chest" a
// search of it; "the north wall" anywhere (examined, knocked on); the floor's and the tracks' looks by their thing's name
// (commands.verbs' [turns]: their phrase verbs gone); "get in", "in", "out", "get out" understood (the enter's, the
// leave's).  DQ0_FORMS=1 prints the answers
function dq0FormsTest() {
  const lines = ['drop the table', 'drop the chair', 'examine the stone chisel', 'drop the chest', 'unlock the chest', 'open the chest', 'put the key in the chest',
    'remove the key from the chest', 'sit on the chair', 'stand', 'sit in the chair', 'stand', 'climb on the table', 'get off', 'look under the table',
    'look behind the chest', 'examine the north wall', 'knock on the north wall', 'look at the floor', 'look for tracks', 'get in', 'in', 'out', 'get out'];
  return {
    name: 'dq0forms', near: ['parser'], what: 'Dingus Quest Zero\'s grammar\'s roles and forms (GAME-CRITIQUE.md\'s 2.8 steps 1 ' +
      'and 3, phase B): a thing named whole ("the stone chisel"); "remove the key from the chest" taken from it; "sit in the ' +
      'chair" as "sit on the chair"; "climb on the table" refused plainly, "get off" with nothing it\'s on; "look under the ' +
      'table" and "look behind the chest" a search of it; "the north wall" examined, knocked on; "look at the floor" the ' +
      'floor\'s look, "look for tracks" the tracks\' (their thing\'s name, commands.verbs\' [turns]); "get in", "in", "out", ' +
      '"get out" understood',
    init: 't_rc', cycles: 600e6 + lines.length * 80e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,key,table,chair,stone chisel\'', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_FORMS) for (const l of lines) console.log(l + ' => ' + a(l));
      const want = (l, re, why) => { if (!re.test(a(l))) f.push('"' + l + '" ' + why + ': ' + a(l).slice(0, 160)); };
      for (const l of lines) if (/understand/.test(a(l))) f.push('"' + l + '" not understood: ' + a(l).slice(0, 120));
      want('examine the stone chisel', /You look at the stone chisel\./, 'not named whole');
      want('remove the key from the chest', /You take the (?:iron )?key from the chest\./, 'not taken from the chest');
      const sit = t => t.replace(/ (?:Nothing|Something) stirs[^.]*\./g, '').replace(/ (?:on|in) the chair/, '');
      if (sit(a('sit in the chair')) !== sit(a('sit on the chair'))) f.push('"sit in the chair" not as "sit on the chair": ' +
        a('sit in the chair').slice(0, 120) + ' / ' + a('sit on the chair').slice(0, 120));
      want('climb on the table', /You can't climb on the table\./, 'not refused plainly');
      want('get off', /^You're not on anything\./, 'not said so');
      want('look under the table', /You search the table/, 'not a search of the table');
      want('look behind the chest', /You search the chest/, 'not a search of the chest');
      want('examine the north wall', /You look at the wall\./, 'not the wall looked at');
      want('knock on the north wall', /You knock along the north wall/, 'not knocked along');
      want('look at the floor', /You look at the floor\./, 'not the floor\'s look');
      want('look for tracks', /You cast about for tracks\./, 'not the tracks\' look');
      return f;
    },
    sdcard: true
  };
}
// The user's parser answers (DECISIONS.md's "The parser and the words", 2026-11-17), at seed 3 (-T, a kit of a chest,
// its key and lockpicks): "say" alone shows the quoted form (and "yell"); "unlock the chest" its key, as it fits, and
// "lock the chest" the same; "pick the lock of the chest" picks it, the key carried or not (a minute's work begun, no
// key tried); a thing dropped where nothing can rest refused as a put is ("drop the dagger on the wall"), on the floor
// put down, on the stairs placed there (its stairway the character's own square, as the game begins at seed 3; else
// out of reach).  DQ0_PARSEANS=1 prints the answers
function dq0ParseAnswersTest() {
  const lines = ['say', 'yell', 'drop the chest', 'unlock the chest', 'lock the chest', 'pick the lock of the chest', 'drop the dagger on the wall',
    'drop the dagger on the floor', 'take the dagger', 'put the dagger on the stairs', 'look'];
  return {
    name: 'dq0parseanswers', near: ['parser'], what: 'Dingus Quest Zero\'s parser answers (the user\'s, 2026-11-17): "say" alone ' +
      'shows the quoted form; "unlock" its key, if one fits; "pick the lock" picks, the key carried; a thing dropped where ' +
      'nothing can rest refused, on the floor put down, on the stairs placed there (its stairway underfoot; else out of reach)',
    init: 't_rc', cycles: 600e6 + lines.length * 120e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,key,lockpicks\'', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const A = dq0Answers(m.out), f = [], all = l => A.filter(x => x.cmd === l).map(x => x.text),
        a = l => (A.find(x => x.cmd === l) || { text: '(no answer)' }).text;
      if (process.env.DQ0_PARSEANS) for (const x of A) console.error(x.cmd + ' => ' + x.text);
      const want = (l, re, why) => { if (!re.test(a(l))) f.push('"' + l + '" ' + why + ': ' + a(l).slice(0, 160)); };
      want('say', /^Put the words in quotes: say "hello"\./, 'its form not shown');
      want('yell', /^Put the words in quotes: yell "hello"\./, 'its form not shown');
      want('unlock the chest', /You unlock the chest with its key\./, 'not its key');
      want('lock the chest', /You lock the chest with its key\./, 'not its key');
      want('pick the lock of the chest', /You set to picking the lock\./, 'not picked');
      if (/with its key/.test(a('pick the lock of the chest'))) f.push('"pick the lock of the chest": its key used');
      want('drop the dagger on the wall', /You can't drop the dagger on the wall\./, 'not refused');
      want('drop the dagger on the floor', /You put down the dagger\./, 'not put down');
      want('put the dagger on the stairs', /You put down the dagger\./, 'not placed on the stairs, its stairway underfoot');
      return f;
    },
    sdcard: true
  };
}
// Every command of the grammar typed once mid-game in some test (savecrash's, 2026-11-19: "save" went untested four
// weeks, the suite's only "save" the quit's).  sim/tools/cmdcover.js lists the actions (actions.h's VB_*, as the parser
// makes a line's: commands.verbs) and the verbs no other test types; this one types each, at seed 3 (-T), mid-game:
// each answered (some words, not "I don't understand"), the game on to its quit (the guards on, as every test's).  The
// actions no line makes (VB_THROW_DIRECTION, VB_ATTACK_SPLIT, VB_SEQUENCE, VB_ACTIVATE, VB_NEGOTIATE,
// VB_JUDGE_SINCERITY: commands.actions' rows with no verb of commands.verbs) aren't the grammar's
const COVER_LINES = ['double time', 'pass', 'call on', 'unmemorize the rat', 'research the rat', 'dispel the rat', 'release the rat',
  'possess the rat', 'link the rat', 'intimidate the rat', 'command the rat', 'bluff the rat', 'deceive the rat', 'threaten the rat',
  'calm the rat', 'come', 'stay', 'dismiss the rat', 'grope for the key', 'find the key', 'dodge', 'block', 'counter the rat',
  'bid 5 pennies', 'watch the stars', 'short rest', 'short sleep', 'pause'];
const COVER_SAID = [['double time', /^Pace: a jog\b/, 'no jog set (the state shown: bugs1\'s fault)'],   // (Answers held, room6's: the pace set, its
  ['call on', /^You know no invocations\b/, 'not the powers\' refusal (read as a name: bugs1\'s fault)']];   //   words the jog's; the call the powers' refusal)
function dq0CoverTest() {
  return {
    name: 'dq0cover', near: ['parser'], what: 'Dingus Quest Zero\'s commands no other test types (sim/tools/cmdcover.js), each typed ' +
      'once mid-game at seed 3 (-T): ' + COVER_LINES.join(', ') + ' (then Esc, the pause\'s end): each answered, not "I don\'t ' +
      'understand", and the game on to its quit; "double time" a jog set, "call on" the powers\' refusal (not a name\'s); "pause" pauses ("PAUSED" on the status line)',
    init: 't_rc', cycles: 600e6 + COVER_LINES.length * 120e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -T', ...COVER_LINES] })) + '\u001b' + dq0Typed(['look', 'quit', 'echo $status']) },
    expect: ['% echo $status'],
    check(m, out) {
      const A = dq0Answers(out), f = [];
      if (process.env.DQ0_COVER) for (const l of COVER_LINES) console.log(l + ' => ' + ((A.find(x => x.cmd === l) || {}).text || '(none)'));
      for (const l of [...COVER_LINES, 'look']) {
        const a = A.find(x => x.cmd === l);
        if (!a) { f.push('"' + l + '" not answered (the game ended before it?)'); continue; }
        if (l !== 'pause' && !a.text) f.push('"' + l + '": nothing said');
        if (/I don't understand/.test(a.text)) f.push('"' + l + '" not understood: ' + a.text.slice(0, 140));
        for (const [k, re, why] of COVER_SAID) if (k === l && !re.test(a.text)) f.push('"' + l + '": ' + why + ': ' + a.text.slice(0, 140));
      }
      if (!/PAUSED/.test(out.slice(out.lastIndexOf('pause')))) f.push('"pause": not paused (no PAUSED on the status line)');
      return f;
    },
    sdcard: true
  };
}
function dq0SweepTest(opt = {}) {
  const name = opt.name || 'dq0sweep', gapsOf = opt.gaps || SWEEP_GAPS, saidOf = opt.said || SWEEP_SAID, lines = dq0SweepLines(opt.verbs);
  return {
    name, what: (opt.what || '') + 'Dingus Quest Zero\'s commands swept, every verb at every kind of thing ' +
      '(COMMANDS-CRITIQUE.md\'s action x kind sweep): at seed 3, the kitchen\'s pot, well, sconce, stairs and wall, the ' +
      'backpack worn, the torch in hand and a sack carried, ' + lines.length + ' lines; the answer to each names its thing ' +
      '(done to it, or refused for it), the known gaps listed (SWEEP_GAPS: none new, none mended and still listed)',
    init: 't_rc', cycles: 400e6 + lines.length * 30e6,
    machine: { input: dq0Typed(dq0Game({ lines: [
      'dq0 -p t -s 3 -c /ram/c.hm -k sack', ...lines, 'quit', 'echo $status'] })) },
    expect: ['% echo $status'],
    check(m) {
      const f = [], A = dq0Answers(m.out), re = new Map(SWEEP_THINGS), gaps = [];
      let from = 0;
      for (const l of lines) {
        const i = A.findIndex((a, k) => k >= from && a.cmd === l);
        if (i < 0) { f.push('no answer to "' + l + '" (the game ended before it?)'); break; }
        from = i + 1;
        const thing = l.slice(l.lastIndexOf(' ') + 1), place = /^(look at|examine) /.test(l) && /You are in /.test(A[i].text),
          ok = re.get(thing).test(A[i].text) && !place;
        if (!ok || process.env.DQ0_SWEEP === 'all') gaps.push(l + ' => ' + A[i].text.slice(0, 110));
        if (!ok && !gapsOf.has(l)) f.push('"' + l + '": its thing not named: ' + (A[i].text.slice(0, 140) || '(nothing)'));
        if (/ what\?/.test(A[i].text)) f.push('"' + l + '": a thing named, asked "... what?" (GAME-CRITIQUE.md\'s 2.8 step 7): ' +
          A[i].text.slice(0, 140));
        if (ok && gapsOf.has(l)) f.push('"' + l + '" names its thing now: take it off SWEEP_GAPS (' + A[i].text.slice(0, 100) + ')');
      }
      for (const [l, w] of saidOf) {                // (Answers held word for word: a thing carried taken, lit)
        const a = A.find(x => x.cmd === l);
        if (a && !a.text.includes(w)) f.push('"' + l + '": not "' + w + '" but: ' + a.text.slice(0, 140));
      }
      if (process.env.DQ0_SWEEP) console.log(name + ' gaps (' + gaps.length + ' of ' + lines.length + '):\n' + JSON.stringify(gaps));
      return f;
    },
    sdcard: true
  };
}

module.exports = {
  tests: [
    {
      name: 'dq0cmd', what: 'Dingus Quest Zero\'s command interpreter (Phase 3C, the user\'s DesignIdeas.md): at seed 3 (-T), three giant rats near, the target set by a choice ("target the rat": which, in their order from the left round to the right, two the same way and as far told apart by their side, answered by number) and shown; a dagger thrown hard at the closest of them (a missile\'s attack since slice 4.7: taken in hand, thrown, the referee\'s Throwing; the rats set senseless as it\'s thrown, no fight for this test\'s words; a bare attack at one put before it, finished, a body to be named: the scene\'s, 2026-11-04); a bare take, the last thing named carried, asking which of the things near (the dead rat\'s body among them), answered by a name, then by a selector; take all, hands and pack full; a bare drop, nothing carried of those; it; a throw with no mark, the target lost from sight; a bare attack, the senseless rat finished; the target cleared; a look one way (turned to), a look round the corner, N alone going north',
      init: 't_rc', cycles: 1500e6,
      machine: { input: '\u0101bind -a /sd/0/games /bin\r\u0101hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\r\u0101echo att sta 99 >> /ram/c.hm\r\u0101echo att wil 99 >> /ram/c.hm\r\u0101dq0 -p t -s 3 -c /ram/c.hm -k pick -T\r' +
        ['target the rat', '1', 'target', 'throw the dagger hard at the closest rat', 'clear target', 'attack', 'drop pick', 'examine the torch', 'take', 'the bones', 'take',
          'the closest', 'take all', 'i', 'drop', 'take it', 'throw the dagger', 'attack', 'clear target', 'target', 'look north',
          'look around the corner', 'n', 'quit', 'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      start(m) {        // (The scene set, 2026-11-03: the rats wary from here on, the things' words the test's, not a fight)
        const wary = (h, L) => { for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName]) h[L.actor + k * L.actorSize + L.aAttitude] = 2; };
        dq0Scene(m, ['drop pick', 'examine the torch', 'take', 'the bones', 'the closest', 'take all', 'i', 'drop', 'take it', 'throw the dagger', 'look north', 'look around the corner', 'n'].map(line => ({ g: 1, line, n: 1, call: wary })).concat([{ g: 1, line: 'take', n: 2, call: wary }, { g: 1, line: 'clear target', n: 2, call: wary }, { g: 1, line: 'target', n: 2, call: wary }, ...[0, 1, 2, 3].map(k => ({ g: 1, line: 'throw the dagger hard at the closest rat', n: 1, k, senseless: 600 })),
          // (Its bare attack: a rat senseless beside it, as the fight would leave one: the dice no longer do)
          { g: 1, line: 'attack', n: 1, call: wary }, { g: 1, line: 'attack', n: 1, name: 'giant rat', beside: 'face', senseless: 600 }, { g: 1, line: 'attack', n: 2, call: wary }, { g: 1, line: 'attack', n: 2, name: 'giant rat', beside: 'face', senseless: 600 }]));
      },
      expect: ['% echo $status'],
      // (Its words checked with the screen's codes and the lines' wrapping taken out, each in its turn)
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        const want = ['Which one? (A) the giant rat ahead to your left, on the right, (C) the giant rat ahead to your left, on the ' +
          'left, (B) the giant rat ahead', 'Your target: the giant rat.', 'Your target: the giant rat ahead to your left.',
          'You take the dagger in your right hand.', 'You throw your dagger hard at the giant rat', 'You finish the giant rat where it lies.', 'Put down.',
          'Which one? (E) the body ahead', 'the bones ahead to your right, (F) the pick where you stand', 'Taken.',
          'Which one? (E) the body ahead', '(F) the pick where you stand', 'You\'ve no hand free for the pick: you put it in your backpack.', 'Your hands are full, and there\'s nowhere to put it.',
          '(j) a pick (k) a torch (l) a flask of oil (full)', '(n) bones, in your right hand',   // (The flask's oil a record of its own in it, a letter: H16, 2026-11-21)
          'Drop what? [', 'Your hands are full, and there\'s nowhere to put it.', 'Throw it at what?',
          'You finish the giant rat where it lies.', 'You have no target now.',
          'You have no target.', 'You turn to face north.', 'You come to the wall.', '% echo $status'];
        let at = 0;
        for (const w of want) {
          const i = o.indexOf(w, at);
          if (i < 0) { f.push('not found (in its turn): ' + w); break; }
          at = i + w.length;
        }
        return f;
      },
      sdcard: true
    },
    dq0SweepTest(),
    dq0CoverTest(),
    dq0SweepTest({ name: 'dq0sweep2', verbs: SWEEP2_VERBS, gaps: SWEEP2_GAPS, said: [], what: 'The second sweep, the other verbs: ' }),
    dq0ClosestTest(),
    dq0NameMatchTest(),
    dq0ItTest(),
    dq0StateSelTest(),
    dq0LeftoversTest(),
    dq0RouteTest(),
    dq0LineTest(),
    dq0OnePathTest(),
    dq0FeaturesTest(),
    dq0ParseFixTest(),
    dq0PairsTest(),
    dq0FormsTest(),
    dq0ParseAnswersTest(),
    {
      name: 'dq0parse', what: 'Dingus Quest Zero\'s parser honest (a review of the commands, 2026-10-22): a verb it doesn\'t know named ("bribe": I don\'t know how to bribe), a grapple at a rat out of reach (since 2026-11-09, HOUSE.md\'s grappling: "go up to it first"; till then the grammar\'s and not yet the game\'s); a gift the game takes since slice 4.8 ("give the torch to the rat": the rat out of reach, or out of sight); "?" the help; several commands in a line, each in turn ("then", ";", "and": "take the bones and the pick", the verb carried); "north and east" one way; a manner word not taking the command ("examine the torch carefully" an examining, not a watch for traps); "pick the bones" taken, not a lock picked; "defend counter" and "defend best"; "run away" a flight, not a run',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -k pick -T',
        'bribe the rat', 'grapple the rat', 'give the torch to the rat', '?', 'examine the torch carefully', 'status', 'drop pick then take it',
        'drop pick; take it', 'drop the pick', 'take the bones and the pick', 'i', 'drop the bones', 'pick the bones', 'drop the pick carefully',
        'status', 'defend counter', 'defend best', 'run away', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      // (Its words checked with the screen's codes and the lines' wrapping taken out, each in its turn)
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        const want = ['> bribe the rat I don\'t know how to bribe.', '> grapple the rat',
          '> give the torch to the rat', '> ? Moving: north,', 'Several at once: north then east;',
          '> examine the torch carefully', '> status Pace: a walk', 'Watching: nothing in particular.',
          '> drop pick then take it You reach for your backpack. Put down.', 'You reach for the pick. You\'ve no hand free for the pick: you put it in your backpack.', '> drop pick; take it You reach for your backpack. Put down. You reach for the pick. You\'ve no hand free for the pick: you put it in your backpack.',
          '> take the bones and the pick You reach for the bones. Taken. You reach for the pick. You\'ve no hand free for the pick: you put it in your backpack.', 'a pick', 'bones', '> pick the bones You reach for the bones. Taken.',
          '> drop the pick carefully You reach for your backpack. Put down.', '> status Pace: a walk', 'Watching: nothing in particular.',
          '> defend counter You will strike back first.', '> defend best You will defend yourself as best you can.',
          '> run away You see nothing to flee from.', '% echo $status'];
        let at = 0;
        for (const w of want) {
          const i = o.indexOf(w, at);
          if (i < 0) { f.push('not found (in its turn): ' + w); break; }
          at = i + w.length;
        }
        if (/examine the torch carefully[^>]*I don't understand that/.test(o)) f.push('"examine the torch carefully" not understood');
        if (!/> grapple the rat (?:The giant rat is out of reach: go up to it first\.|There's nothing like that here to fight\.)/.test(o)) {
          f.push('the grapple not answered as one out of reach (' + (o.match(/> grapple the rat[^>]*/) || [''])[0] + ')');   // (A grapple's since 2026-11-09, HOUSE.md's grappling: at one beside)
        }
        if (!/> give the torch to the rat (?:You look for the torch\. )?(The giant rat is out of reach: go up to it first\.|You see no rat here\.)/.test(o)) {
          f.push('the gift not answered as one out of reach (' + (o.match(/> give the torch to the rat[^>]*/) || [''])[0] + ')');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0notyet', what: 'Dingus Quest Zero\'s verbs not done yet, the shared groundwork (2026-11-09, the plan for them, its batch B0: DECISIONS.md\'s "The verbs not done yet: the rows, the aliases and the powers\' refusals"): the powers refused, true to the character, from the rows (a spell: "You\'ve no magic: you never studied the convocations."; a talent: "You reach for it; nothing stirs."; an invocation, one learnt: "You know no invocations of Peoni\'s.", the character\'s deity the referee\'s, RS_DESCRIBE\'s RD_DEITY, its file\'s att dty 6); the aliases (activate, use\'s: a light used is lit, the things\' batch B4\'s, 2026-11-10; negotiate, persuade\'s; sense a lie, examine\'s); "status" (pace.c\'s, before the table) and "pace" alone (the table\'s H_STATUS, the same words); "sneak north" a creep, then the move (not the pace alone, as it was)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att dty 6 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'cast light', 'memorize the light spell', 'telepathy', 'invoke divination', 'learn divination',
        'activate the torch', 'negotiate with the rat', 'sense a lie from the rat', 'status', 'pace', 'sneak north', 'status', 'quit',
        'hmchar -q -s 5 -u feudal -c hunter -o /ram/d.hm', 'echo att dty 10 >> /ram/d.hm', 'dq0 -p t -s 3 -c /ram/d.hm -T', 'invoke divination', 'quit',
        'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [];
        inTurn(o, ['> cast light You\'ve no magic: you never studied the convocations.',
          '> memorize the light spell You\'ve no magic: you never studied the convocations.', '> telepathy You reach for it; nothing stirs.',
          '> invoke divination You know no invocations of Peoni\'s.', '> learn divination You know no invocations of Peoni\'s.',
          '> activate the torch', 'The torch is lit already.', '> negotiate with the rat', '> sense a lie from the rat You look at the giant rat. The giant rat seems', '> status Pace: a walk',
          'Watching: nothing in particular.', '> pace Pace: a walk', 'Watching: nothing in particular.', '> sneak north Pace: a creep',
          'You set off north.', '> status Pace: a creep', '> invoke divination You know no invocations.', '% echo $status'], f);
        if (/in this game yet/.test(o)) f.push('a "not yet" said: ' + (o.match(/> [^>]*in this game yet/) || [''])[0]);
        const neg = o.slice(o.indexOf('> negotiate with the rat'), o.indexOf('> sense a lie'));
        if (!/words won't reach it/.test(neg)) f.push('negotiate not a persuasion (the sway\'s words for an animal): ' + neg);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pray', what: 'Dingus Quest Zero\'s prayer and fasting (the verbs plan\'s B5, 2026-11-10: the user\'s; DECISIONS.md\'s "Prayer, fasting, swimming and following"), by the words told and the clock: a character of Peoni (its file\'s att dty 6), its torch put out first (an hour\'s burning would stop the prayer): "pray" kneels and prays to Peoni ("You kneel and pray to Peoni."), an hour (the clock 36,000 tenths on), then rises, the deity named in a calming line (its Ritual test\'s, P_RITUAL: words alone); "pray for strength" the same; "fast" a vow ("You vow to fast for a day"), "eat food" refused ("You\'re fasting, with 24 hours of your vow to go: you leave the food be."), "fast" again told so, "stop" (no long task) ends it ("You break your fast"), "eat food" not refused then; "break my fast" ends a second vow; "stop fasting" with none, "You aren\'t fasting."; a second game\'s character of no deity (att dty 10): the hour\'s prayer, "though you know no god to pray to", and its end\'s line of its own',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att dty 6 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'put out the torch', 'pray', 'pray for strength', 'fast', 'eat food', 'fast', 'stop', 'eat food', 'fast', 'break my fast',
        'stop fasting', 'quit', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/d.hm', 'echo att dty 10 >> /ram/d.hm', 'dq0 -p t -s 3 -c /ram/d.hm -T',
        'put out the torch', 'pray', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), o = dq0Prose(out), f = [];
        inTurn(o, ['> pray You kneel and pray to Peoni.', 'You rise from your prayers', 'Peoni', '> pray for strength You kneel and pray to Peoni.',
          'You rise from your prayers', 'Peoni', '> fast You vow to fast for a day', '> eat food You\'re fasting, with 24 hours of your vow to go: you leave the food be.',
          '> fast You\'re fasting already, with 24 hours of your vow to go.', '> stop You break your fast: you may eat again.', '> eat food',
          '> fast You vow to fast for a day', '> break my fast You break your fast: you may eat again.', '> stop fasting You aren\'t fasting.',
          '> pray You kneel and pray, though you know no god to pray to.', 'You rise from your prayers. If anyone heard, no sign of it came'], f);
        for (const l of ['pray', 'pray for strength']) {
          const t = T.of(l);
          if (!t.at || !t.after || t.after.now - t.at.now < 36000) f.push(l + ': not an hour (' + (t.at && t.at.now) + ' to ' + (t.after && t.after.now) + ')');
        }
        const e = o.slice(o.indexOf('> stop You break'), o.indexOf('> fast', o.indexOf('> stop You break')));
        if (/fasting/.test(e.slice(e.indexOf('> eat food')))) f.push('eating refused after the fast was ended: ' + e);
        if (/in this game yet/.test(o)) f.push('a "not yet" said');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0alias', what:'Dingus Quest Zero\'s shortcuts (the user, 2026-10-22: "alias should be for shortcuts, and name for naming things"; alias.c): none made, and the game\'s own words listed; one made ("alias st is status"), used, shown; its replacement asked about (no: kept; yes: replaced); a command\'s word refused ("alias look is ..."), "alias the sword as Linda" sent to name; a game\'s own word made the player\'s ("peer"), then removed, the game\'s own left (can\'t be removed); a shortcut of a shortcut given its line; one of several commands ("status; i"), the words typed after one put after its line; them all listed; the game kept and carried on, the shortcuts with it; one forgotten ("forget alias st")',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/kept',
        'alias', 'alias st is status', 'st', 'alias st', 'alias st is i', 'no', 'st', 'alias look is status', 'alias the sword as Linda',
        'alias peer = status', 'peer', 'alias s2 is st', 'alias both2 is status; i', 'both2', 'alias ex is examine', 'ex the torch', 'alias',
        'unalias peer', 'unalias peer', 'alias st is i', 'yes', 'quit', 'dq0 -d /ram/kept', 'st', 'alias st', 'forget alias st', 'alias st',
        'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      // (Its words checked with the screen's codes and the lines' wrapping taken out, each in its turn)
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        const want = ['> alias You\'ve made no shortcuts (alias dd is dig down makes one).', 'The game\'s own words: peer (look), gaze (look),',
          '> alias st is status st now stands for: status', '> st Pace: a walk', '> alias st st stands for: status',
          '> alias st is i st stands for: status. Replace it? (yes or no)', '> no It stands as it was.', '> st Pace: a walk',
          '> alias look is status look is one of the game\'s own command words: choose another.',
          '> alias the sword as Linda To name a thing, use name: name the sword Linda.', '> alias peer = status peer now stands for: status',
          '> peer Pace: a walk', '> alias s2 is st s2 now stands for: status', '> alias both2 is status; i both2 now stands for: status; i',
          '> both2 Pace: a walk', 'You carry', '> alias ex is examine ex now stands for: examine', '> ex the torch',
          '> alias Your shortcuts: st: status; peer: status; s2: status; both2: status; i; ex: examine.', 'The game\'s own words: peer (look)',
          '> unalias peer peer isn\'t a shortcut now.', '> unalias peer peer is the game\'s own word (for look): it can\'t be removed',
          '> alias st is i st stands for: status. Replace it? (yes or no)', '> yes st now stands for: i', '> st You carry',
          '> alias st st stands for: i', '> forget alias st st isn\'t a shortcut now.', '> alias st st isn\'t a shortcut.', '% echo $status'];
        let at = 0;
        for (const w of want) {
          const i = o.indexOf(w, at);
          if (i < 0) { f.push('not found (in its turn): ' + w); break; }
          at = i + w.length;
        }
        if (/> ex the torch[^>]*I don't/.test(o)) f.push('"ex the torch" not made "examine the torch"');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0talk', meaning: true, what: 'Dingus Quest Zero understanding the player (the command parser, dq0c: the grammar and the game\'s words, after the user\'s Language.txt), by what each command did: synonyms (take a gander: a look, no time), several words to a verb, turns as the character faces (half left) and two directions together (north and east), a door behind walked up to (asked which: the second), the little words passed over (off the verb\'s in "take off my coat": nothing worn refused; a filler in "saunter off to the left": a move to its left), a thing not there refused, a word it doesn\'t know doing nothing (their words: dq0wordsthings)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T', 'take a gander', 'turn half left',
        'face north and east', 'walk to the door behind me', 'pick up the sword', 'take off my coat', 'saunter off to the left', 'foo', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], still = t => t.k >= 0 && !t.events.length && t.at && t.after && t.after.now === t.at.now && t.after.pc.x === t.at.pc.x && t.after.pc.y === t.at.pc.y;
        const face = (l, h) => { const t = T.of(l); if (!t.after || t.after.pc.face !== h) f.push(l + ': facing ' + (t.after && t.after.pc.face) + ', not ' + h); return t; };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T').after;
        if (!s0 || s0.pc.face !== 6) return ['not begun facing west'];
        if (!still(T.of('take a gander'))) f.push('take a gander not a look (' + dq0Evs(T.of('take a gander')) + ')');
        face('turn half left', 5);
        face('face north and east', 1);
        const c = T.of('walk to the door behind me');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'the door behind not walked up to');
        if (!c.after || c.after.pc.face !== 4) f.push('not facing the door behind it (south)');
        if (!dq0Rep(T.of('pick up the sword'), 'ireq', { kind: 'IO_GET', result: 'IR_NONE' }).length) f.push('a sword not there not refused');
        if (!dq0Rep(T.of('take off my coat'), 'ireq', { kind: 'IO_DOFF', result: 'IR_NO' }).length) f.push('take off my coat not a doff refused');
        const s = T.of('saunter off to the left');
        dq0Need(f, s, 'E_ARRIVE', {}, 'saunter off to the left not a move');
        if (!s.at || !s.after || !(s.after.pc.x > s.at.pc.x) || s.after.pc.face !== 2) f.push('not moved to its left (east, facing south)');
        if (!still(T.of('foo'))) f.push('a word it doesn\'t know did something (' + dq0Evs(T.of('foo')) + ')');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0names', meaning: true, what: 'Dingus Quest Zero\'s names (the user, 2026-10-16: "I would like to be able to reference items by aliases, as long as the alias doesn\'t clash with something else"; names.c), by the names table (hub.h\'s struct tname, NAMES_AT) and the things\' records: a sword named Linda (dq0 -k sword: its record marked named), names that clash refused (another name, a kind of thing\'s, a kind of creature\'s, a word the parser needs: no name made), Linda used in commands (dropped, seen lying there, taken up, examined, taken in hand to strike a rat with), the room named the Mausoleum; the game kept and carried on, the names with it; then a name forgotten (its record no longer named) (the names\' words: dq0wordsthings)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T -k sword -d /ram/kept',
        'name my sword Linda', 'name the dagger as linda', 'name the dagger Torch', 'name the dagger kobold', 'name the dagger north', 'drop Linda',
        'look', 'take Linda', 'examine Linda', 'attack the rat with Linda', 'call this room the mausoleum', 'quit', 'dq0 -d /ram/kept',
        'forget the name Linda', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], NAMED = 0x40;
        const names = t => (t.after ? t.after.names : []).map(n => n.what + ' ' + n.name).join(', ');
        const sword = t => dq0Thing(t.after, 'sword');
        const n1 = T.of('name my sword Linda');
        if (names(n1) !== 'NM_ITEM Linda') f.push('the sword not named Linda: ' + names(n1));
        if (!sword(n1) || !(sword(n1).flags & NAMED)) f.push('the sword\'s record not marked named');
        for (const l of ['name the dagger as linda', 'name the dagger Torch', 'name the dagger kobold', 'name the dagger north'])
          if (names(T.of(l)) !== 'NM_ITEM Linda') f.push(l + ': a name that clashes made: ' + names(T.of(l)));
        const d = T.of('drop Linda');
        if (!dq0Here(d.after).some(i => i.name === 'sword')) f.push('Linda not put down');
        const lk = T.after(d, 'look');
        if (!lk.after || !lk.after.sight || !lk.after.sight.items.some(i => i.name === 'sword' && i.rel === 255)) f.push('Linda not seen lying there');
        if (!dq0Carried(T.of('take Linda').after).some(i => i.name === 'sword')) f.push('Linda not taken up');
        if (!dq0Rep(T.of('examine Linda'), 'ireq', { kind: 'IO_LOOK', result: 'IR_OK' }).length) f.push('Linda not examined');
        const a = T.of('attack the rat with Linda');
        if (!sword(a) || !(sword(a).flags & DQ0_I.HELD)) f.push('Linda not taken in hand to strike with');
        const r = T.of('call this room the mausoleum');
        if (names(r) !== 'NM_ITEM Linda, NM_ROOM the Mausoleum') f.push('the room not named the Mausoleum: ' + names(r));
        const c = T.of('dq0 -d /ram/kept');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (names(c) !== 'NM_ITEM Linda, NM_ROOM the Mausoleum') f.push('the names not kept with the game: ' + names(c));
        const g = T.of('forget the name Linda');
        if (names(g) !== 'NM_ROOM the Mausoleum') f.push('Linda not forgotten: ' + names(g));
        if (!sword(g) || (sword(g).flags & NAMED)) f.push('the sword\'s record still marked named');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0prompt', what: 'Dingus Quest Zero answering at once at the normal pace while the denizens act (DENIZENS.md: events; the user: "the UI is slow to respond"): the giant rats by the way down moving about on their own (their task\'s, in the background), a while waited, then a turn and what the character carries asked for at once, while the senses look about after the turn (the character still busy): told within a fifth of a second of Enter, before the look\'s done (the senses answer it between the look\'s rows, the UI a command that wants nothing of the character at once), and a step that can\'t be taken (up, with no way up) told as quickly (the world\'s answer); the status line drawn only where it changed, a second at most (the user: the cursor flickered), the cursor hidden while it\'s there',
      meaning: true,
      init: 't_rc', cycles: 900e6,
      machine: { input: 'ābind -a /sd/0/games /bin\rāhmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\rādq0 -p n -s 3 -c /ram/c.hm -T\r' +
        'ĀĀāturn left\rinventory\rāgo up\rĀĀĀĀāquit\rāecho $status\r' },
      expect: ['You carry', 'There\'s no way up or down here.', '% echo $status'],
      // (The inventory's budget a guard against a large slowdown at 9,600 baud, not a target: 1,410,000 to 1,530,000
      // cycles measured, as the rats' stirring falls on the line before it; the user's targets are 115,200's, D17:
      // dq0answer's)
      // (Trends since 2026-11-17, the user's, their budgets 2,000,000 and 700,000 before: tests/lib/trends.js, a rise
      // of more than 5% said, not failed)
      budgets: [{ what: 'inventory answered (Enter to the answer)', from: '> inv', to: 'You carry', per: 1, trend: true },
        { what: 'a step that can\'t be taken answered (the world\'s: from the line told into the story)', from: '\n> go up',
          to: 'There\'s no way up or down here.', per: 1, trend: true }],
      // (Each a mark in what the serial line sends: the inventory's from the line typed at the bottom, a fifth of a
      // second, the user's; the step's from the line entered, as it's told into the story (on a line of its own: the
      // line typed's row, 24, after it, or the status line's clock first, as the second falls): about 285,000 cycles to "go up"'s answer since the world sleeps on its
      // doorbell (the hub's protocol, SEM_WAIT: the UI's intent wakes it at once; 354,000 when it napped up to 4 ticks
      // between looks): some 150,000 the world's (its climbing part swapped in for it), 130,000 the UI's words, a rat's
      // step about then or not.  From the line typed it was
      // 380,000 or 600,000 cycles, as that echo went out at once or waited behind the words still going out on the
      // line: when Enter fell against what the line was sending, moved by any task's code, not the answer's speed)
      // (While nothing happens, between the step and quitting, 8M cycles, 2.2 s: the status line's clock, a move up to
      // it a second at most, the cursor hidden for it)
      // (Move 2.5: the inventory's mark its typed line's first letters, '> inv': '> inventory' met the line told into
      // the story, just before the answer, only when the status line's clock split the typed echo (87,000 cycles then);
      // from the line typed it was some 3.3M cycles since move 2.3 (the world's answer, its swapped part's words,
      // about 1.1M of it, the UI's parsing and words the rest), 2.6M now, the senses' look giving way to the command
      // in hand at the real paces: a fifth of a second (1.6M) not met yet, the world's words for it to quicken)
      // (The speed pass: 690,000 to 900,000 now, as the code lies (a fifth of a second met in some builds, not all): the
      // UI's loop looking at the keys every four ticks, not two; the world's list in one pass of the store, part 11
      // alone; the line a command in hand (the look giving way) from its first parse, at_once's, which command takes as
      // it is; the verbs' and the paces' words looked for by their first letters; the look giving way at more of its
      // stages.  What's left: the line typed while the UI still says the turn (its words wait for the serial line,
      // full of the rats' stirring at 9,600 baud), the typed echo's last letters with the answer, 600,000 cycles after
      // its first; and the world's list, 200,000 of its own)
      // (reborn-game-stream: these at 9,600 baud, the user's D17 (the targets are 115,200's: dq0answer).  The turn's first
      // words told as it begins (V_FACING), ahead of what it carries now, and the cursor's moves to the line typed and
      // back round them, some 60 characters at 3,730 cycles each: the inventory 1,157,000 from its typed letters (887,000
      // before), a budget of 1,450,000; and the line entered told into the story as it's entered, not with the answer,
      // so from it the step's answer has the UI's parse and the world's climb in it: 456,000, a budget of 600,000 (from
      // Enter it's quicker: dq0answer96's go up, some 560,000, 575,000 before).  What it carries is still told before
      // the turn's look, its words after its first.  Since slice 4.1 (2026-10-22) 531,000 to 635,000 as the boot's
      // timing moves where the rats' words fall at 9,600 baud (the kernel's polled console timed, the step's own path the
      // same): a budget of 700,000, a guard against a large slowdown, as the inventory's; the user's targets dq0answer's)
      check(m, out) {
        const a = out.indexOf('There\'s no way up or down here.'), b = out.indexOf('> quit', a), seg = a < 0 || b < 0 ? '' : out.slice(a, b);
        const top = (seg.match(/\x1b\[1;\d+H/g) || []).length, f = [];
        if (top > 4) f.push(top + ' moves up to the status line while nothing happened (a second at most each)');
        const turned = out.indexOf('You turn '), looked = turned < 0 ? -1 : turned + out.slice(turned).search(/You (can see|are in)/);
        if (looked < turned || out.indexOf('You carry') > looked) f.push('what it carries told only after the turn\'s look');
        for (const s of seg.matchAll(/\x1b\[1;\d+H/g)) {      // (Hidden as it moved there: hidden last, here or before)
          const at = a + s.index;
          if (out.lastIndexOf('\x1b[?25l', at) <= out.lastIndexOf('\x1b[?25h', at)) { f.push('the cursor not hidden while the status line was drawn'); break; }
        }
        return f;
      },
      sdcard: true
    },
    dq0AnswerTest(115200),
    {
      name: 'dq0words', what: 'Dingus Quest Zero\'s words, a test about wording alone (the others check meaning: dq0words, dq0wordsfix and dq0wordsthings are where the game\'s sentences are held to): rooms and what\'s seen as the user asked (2026-10-15 and 16): at seed 3 a room told in general words at a glance (a small kitchen, a barrel vault high overhead), the door behind not told ("descriptions often tell you what lies behind you, which you can\'t really see"), its figures (its size in feet, its ceiling\'s height) only on a close look (look closely, x room); the door behind told as it turns round, examined in full (its make, size and thickness); at seed 5 a close look at a hall whose far side is dark, no figures, its far side lost in the dark; at seed 11 a door across the pantry examined, only its look from afar (tall, of wood, how far, go closer), and by a weak-eyed character (Eyesight 4: "Eyesight score should affect the amount of detail") not even its wood (dq0doors\' and dq0sense\'s words)',
      init: 't_rc', cycles: 1350e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['cp /ram/c.hm /ram/w.hm', 'echo att eye 4 >> /ram/w.hm',
        'dq0 -p t -s 3 -c /ram/c.hm', 'look', 'look closely', 'x room', 'turn around', 'examine the door ahead to my left', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm', 'x room', 'quit', 'dq0 -p t -s 11 -c /ram/c.hm', 'examine the door ahead to my left', 'quit',
        'dq0 -p t -s 11 -c /ram/w.hm', 'examine the door ahead to my left', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['% dq0 -p t -s 3', '> look You are in the middle of a small kitchen', 'A barrel vault arches high overhead. There is a door ahead to your left (open).',
          '> look closely', 'It is 10 feet by 10.', 'A barrel vault rises to 13 feet, 9 at the walls.', '> x room', 'It is 10 feet by 10.', '> turn around',
          'There is a door ahead to your left (closed).', '> examine the door ahead to my left', 'A door of iron-bound wood, 3 feet wide and 7 high, 3 inches thick.',
          '% dq0 -p t -s 5', '> x room You can see part of a hall, by the light of your torch. Its far side is lost in the dark.',
          '% dq0 -p t -s 11 -c /ram/c.hm', 'You head for the door.', 'A door of wood, 3 feet wide and 8 high',
          '% dq0 -p t -s 11 -c /ram/w.hm', 'You head for the door.', 'A door of wood, 3 feet wide and 8 high', '% echo $status'], f);
        const g = o.slice(o.indexOf('> look You are'), o.indexOf('> look closely'));
        if (/feet up|feet by/.test(g)) f.push('the glance gives figures: ' + g);
        if (/behind you/.test(g)) f.push('the glance tells what\'s behind: ' + g);
        const r = o.slice(o.indexOf('> x room You can see part'), o.indexOf('lost in the dark'));
        if (/feet by/.test(r)) f.push('the dark room\'s figures given: ' + r);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wordsfix', what: 'Dingus Quest Zero\'s words, a test about wording alone: the fixtures\' (the doors, their bars, the windows) as the user asked (2026-10-15 and 16): at seed 4, three doors none of them ahead, open door listing them, numbered, from the character\'s left round (each where it lies, its state), 3 turning to and opening the one behind it to its right, the room\'s doors told again; its door\'s bar lifted out of its brackets and taken, carried; at seed 12 a window examined (its make, its pane judged), its pane looked at closely, not climbed through glazed, broken (shards at its feet), climbed through into a passage, the window seen from there; at seed 15 a barred door examined (its bar in its brackets), the bar looked at closely, the door battered with a pick till it gives way (the torch put down, the pick taken in hand: "You set to battering the door with the pick."), broken down for good: it won\'t shut (dq0doors\', dq0choose\'s and dq0window\'s words)',
      init: 't_rc', cycles: 1150e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 4 -c /ram/c.hm', 'open door', '3', 'look', 'take the bar',
        'i', 'quit', 'dq0 -p t -s 12 -c /ram/c.hm', 'go through the door to the north', 'examine the window', 'x glass', 'climb through window', 'break window', 'look', 'climb through window',
        'turn around', 'quit', 'dq0 -p t -s 15 -c /ram/c.hm -k pick', 'examine the door ahead to my left', 'examine bar', 'drop the torch',
        'break the door ahead to my left', 'look',
        'close', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\([A-Z]{1,2}\) /g, '').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['% dq0 -p t -s 4', '> open door Which door? (1) the door ahead to your right (open), (2) the one to your right (closed, barred), (3) the one behind you to your right (closed)',
          '> 3', 'You turn to face north.', 'You open the door, revealing a passage, 15 feet of it.', '> look You are on the right side of a long, narrow bedchamber',
          'There is a door ahead (open), one to your left (open) and one ahead to your left (closed, barred).', '> take the bar',
          'You lift the bar out of its brackets, and set it down.', 'Taken.', '> i', 'a bar, in your right hand', '% dq0 -p t -s 12', '> examine the window',
          'A window, 2 feet wide and 3 high; its shutters of wood, 2 inches thick. It\'s glazed', 'There is a glazed window ahead (its shutters open).', '> x glass',
          'It\'s made of glass, 2 by 3 feet, a quarter inch thick', '> climb through window', 'You can\'t climb through: it\'s shut, glazed or barred.', '> break window',
          'The glass shatters with a crash.', '> look', 'There is a shuttered window ahead (its shutters open). At your feet lie some glass shards.',
          '> climb through window', 'You set to climbing through the window.', 'You get up onto the sill, and through. You are in a passage.', '> turn around',
          'There is a shuttered window ahead (its shutters open).',
          '% dq0 -p t -s 15', '> examine the door ahead to my left', 'Brackets for a bar are fixed on this side, the bar in them.', '> examine bar',
          '4 feet long, 3 inches thick', '> break the door ahead to my left', 'You take the pick in both hands.', 'You set to battering the door with the pick.',
          'You break the door down: it crashes open', '> look',
          'There is a door ahead (broken down).', '> close', 'It\'s broken: it won\'t shut.', '% echo $status'], f);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wordsthings', what: 'Dingus Quest Zero\'s words, a test about wording alone: the things\', the names\', the pace\'s and the help\'s, in one game (seed 3, dq0 -T, a kit of things more: dq0 -k): a portcullis and a door behind, told apart by how far off each is; what\'s carried listed (worn, held in both hands, in and on what, the load: the kit\'s rations a pound each, slice 4.1\'s counted lots; the arrows one arrow); a table put down and a candle put on it, taken from it; a cloak worn and taken off, a basket held, the sand it won\'t keep, the waterskin that won\'t take a dagger, a quiver hung and arrows put in it, a candle that won\'t hang, hands full, the pack not into the basket in it; a sword named Linda (shown so in what\'s carried, no mark of its number), names that clash refused with why; a thing that isn\'t there, a coat not worn, a word the game doesn\'t know; the status (pace, watching, condition, light); a jog, held to a slow walk while it searches; a run set as it goes, its look as it ends a brief one (no ceiling), going on at it unsaid; the help (dq0equip\'s, dq0items\', dq0names\', dq0talk\'s, dq0choose\'s, dq0pace\'s and dq0steps\' words)',
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T -k table,basket,sand,cloak,candle,quiver,arrows,sword',
        'face north and east', 'walk to the door behind me', 'i', 'drop table', 'put the candle on the table', 'look', 'take the candle from the table', 'stow the candle', 'b',
        'wear cloak', 'hold basket', 'put sand in basket', 'put the dagger in the waterskin', 'hang the quiver on my belt', 'put arrows in quiver', 'hang the candle',
        'wield the dagger', 'stow basket', 'put the backpack in the basket', 'take the cloak off', 'stow cloak', 'name my sword Linda', 'name the dagger as linda',
        'name the dagger Torch', 'name the dagger kobold', 'name the dagger north', 'i', 'examine the moon', 'take off my coat', 'foo', 'status', 'jog',
        'search for traps as I go', 'stop searching', 'run north', 'run south', 'help', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\([A-Z]{1,2}\) /g, '').replace(/\s+/g, ' '), f = [];
        inTurn(o, ['> walk to the door behind me', 'You head for the door.', '> i', 'You carry: (a) a torch (lit), in your left hand (b) a backpack, worn (c) 3 rations (d) a tinderbox (k) a basket (l) a heap of sand',
          '(q) a sword (r) a torch (s) a flask of oil (full) (e) a belt, worn (f) a pouch (g) 12 pennies (h) a dagger (i) a waterskin (empty) (j) a table, in both hands In all, 60 lb.', '> drop table', 'You put down the table.',
          '> put the candle on the table', 'You put the candle on the table.', '> look', 'At your feet lies a table. On the table lies a candle.',
          '> take the candle from the table', 'You take the candle from the table. It\'s in your hand.', '> stow the candle', 'Stow it where? (b) the backpack, the side pocket, (f) the pouch', '> b', 'You stow the candle in your backpack.', '> wear cloak', 'You put on the cloak.',
          '> hold basket', 'You take the basket in your right hand.', '> put sand in basket', 'The basket won\'t keep the heap of sand: it would run out.',
          '> put the dagger in the waterskin', 'The dagger won\'t go in the waterskin.', '> hang the quiver on my belt', 'You hang the quiver from your belt.',
          '> put arrows in quiver', 'You put the arrows in the quiver.', '> hang the candle', 'The candle won\'t hang from a belt.', '> wield the dagger',
          'Your hands are full: stow something, or put it down.', '> stow basket', 'You stow the basket in your backpack.', '> put the backpack in the basket',
          'The backpack won\'t go inside itself.', '> take the cloak off', 'You take off the cloak. It\'s in your hand.', '> stow cloak',
          'You stow the cloak in your backpack.', '> name my sword Linda', 'You name your sword Linda.', '> name the dagger as linda', 'That name is taken by your sword.',
          '> name the dagger Torch', 'That name is taken by a kind of thing (torch).', '> name the dagger kobold', 'That name is taken by a kind of creature (kobold).',
          '> name the dagger north', 'That name is taken: it\'s a word your commands need.', '> i', '(n) a candle (q) Linda, your sword (r) a torch (s) a flask of oil',
          '(i) a waterskin (empty) (o) a quiver (p) an arrow In all, 20 lb.', '> examine the moon', 'You see no moon here.', '> take off my coat', 'You aren\'t wearing any coat.',
          '> foo', 'I don\'t know how to foo.', '> status', 'Pace: a walk (40 feet a round). Watching: nothing in particular. Condition: unhurt; a little burdened (encumbrance 1); penalties: universal 0, physical 1. Light: your torch (20 feet).',
          '> jog', 'Pace: a jog (80 feet a round). Watching: nothing in particular.', '> search for traps as I go',
          'Pace: a jog, held to a slow walk while you search as you go (20 feet a round). Watching: for traps and hidden doors as you go.', '> stop searching',
          'Pace: a jog (80 feet a round).', '> run north', 'Pace: a run (160 feet a round).', 'You are in the middle of a long, narrow cell, by the light of your torch.',
          '> run south', '> help', 'Moving: north, northeast, east', 'Pace: creep (sneak), walk slowly, walk, jog, run, sprint', '19200 or 115200 baud', '% echo $status'], f);
        if (/#\d/.test(o)) f.push('a thing\'s mark ("#12") said');
        const rn = o.slice(o.indexOf('> run north'), o.indexOf('> run south')), rs = o.slice(o.indexOf('> run south'), o.indexOf('> help'));
        if (/ceiling|vault/.test(rn.slice(rn.indexOf('You are in')))) f.push('the look as the run ended not brief: ' + rn);
        if (/Pace:/.test(rs)) f.push('the pace said again going on as it was: ' + rs);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0choose', meaning: true, what: 'Dingus Quest Zero asking which, of several (the user\'s, 2026-10-16: "If I say open door and there are three closed doors in the room, it should give me a numbered list of choices if one is not directly in front of me"), by what each answer did: at seed 4, three doors on the bedchamber\'s north and west walls, none ahead: open door asks (nothing done), and 3 opens the one behind it to its right (turned to: facing north); then a fresh game, examine door asked again, answered with a way (right: the one to its right, examined); go through the door asked, a command not a choice (look) letting it go (1 then a word it doesn\'t know: nothing done), asked again and 1 going through the one ahead to its right; at seed 3 (dq0 -T) a portcullis and a door behind it, the first, the portcullis, walked up to (the lists\' words: dq0wordsfix, dq0wordsthings)',
      init: 't_rc', cycles: 1100e6,   // (1000e6 till 2026-10-30: 994M then, over it with morale's bigger actors and parts)
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 4 -c /ram/c.hm', 'open door', '3', 'quit',
        'dq0 -p t -s 4 -c /ram/c.hm', 'examine door', 'right', 'face south', 'go through the door', 'look', '1', 'go through the door', '1', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'face north and east', 'walk to the portcullis behind me', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], asked = t => t.k >= 0 && !t.events.length && !t.replies.some(r => r.req === 'wreq' && r.kind === 'WQ_EXAMINE');
        const o = T.of('open door'), c3 = T.of('3');
        if (!asked(o)) f.push('open door not asked which (' + dq0Evs(o) + ')');
        dq0Need(f, c3, 'E_PORTAL', { po: 'PO_OPENED' }, '3 didn\'t open the door');
        if (!c3.after || c3.after.pc.face !== 0) f.push('not turned to the door behind to its right (north)');
        const x = T.of('examine door'), r = T.of('right');
        if (!asked(x)) f.push('examine door not asked which');
        if (!dq0Rep(r, 'wreq', { kind: 'WQ_EXAMINE', res: 0 }).length) f.push('right not answered with the door to its right examined');
        const g1 = T.of('go through the door'), lk = T.after(g1, 'look'), n1 = T.after(lk, '1'), g2 = T.after(n1, 'go through the door'), n2 = T.after(g2, '1');
        if (!asked(g1) || !asked(g2)) f.push('go through the door not asked which');
        if (n1.k < 0 || n1.events.length) f.push('1 after a look (the question let go) did something (' + dq0Evs(n1) + ')');
        dq0Need(f, n2, 'E_ARRIVE', {}, '1 not gone through the door');
        const p = T.of('walk to the portcullis behind me');   // (A door is a door before a portcullis, the look's words: 2026-11-02)
        dq0Need(f, p, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'the portcullis not walked up to');
        if (!p.at || !p.after || p.at.pc.x - p.after.pc.x !== 2) f.push('not gone the portcullis\'s 10 feet');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stopwords', near: ['parser'], what: "Dingus Quest Zero's stop words other than \"stop working\" (HOUSE.md's stopping: the work named; 2026-11-03: the grammar had none of dipping, sharpening, freeing, and sharpening's was filling's kind), typed as each task's told (dq0OnWords): at the normal pace (a task's time for the words to be typed in), at seed 12 the window's pane being worked free: \"stop digging\" refused, naming what it's at, \"stop freeing\" stops it, the pane still in its frame; a blunted sword being sharpened (dq0Poke's wear): \"stop dipping\" refused, \"stop sharpening\" stops it, no fresh edge told; a torch being dipped: \"stop dipping\" stops it, no soaking told; dipped again, \"stop soaking\" stops it too (filling, soaking and dipping: whichever of them it's at, 2026-11-06; the UI's overlay 7 read the task's kind in its own bank till the load guard found it)",
      init: 't_rc', cycles: 1300e6,
      machine: { input: dq0Typed(dq0Game({ lines: [
        'dq0 -p n -s 12 -c /ram/c.hm', 'go through the door to the north', 'lift the pane', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword,whetstone', 'sharpen the sword', 'quit',
        'dq0 -p n -s 3 -c /ram/c.hm -T', 'drop a fresh torch', 'wait 20 minutes', 'dip the torch', 'dip the torch', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 2, line: 'sharpen the sword', n: 1, wear: { sword: 200 } }]);
        dq0OnWords(m, [{ g: 1, line: 'lift the pane', n: 1, words: 'You set to working the pane free.', text: 'stop digging\rstop freeing\r' },
          { g: 2, line: 'sharpen the sword', n: 1, words: 'You set to sharpening', text: 'stop dipping\rstop sharpening\r' },
          { g: 3, line: 'dip the torch', n: 1, words: 'You set to dipping', text: 'stop dipping\r' },
          { g: 3, line: 'dip the torch', n: 2, words: 'You set to dipping', text: 'stop soaking\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), G = o.split('% dq0 ').slice(1), f = [];
        if (G.length !== 3) return ['not three games: ' + G.length];
        if (!/You aren't digging: you're working the pane free\./.test(G[0])) f.push('the pane: a stop naming another not refused');
        if (/I don't understand/.test(G[0] + G[1] + G[2])) f.push('a stop word not understood');
        if (/The pane comes free/.test(G[0])) f.push('the pane: not stopped');
        if (!/You aren't dipping: you're sharpening the sword\./.test(G[1])) f.push('sharpening: a stop naming another not refused');
        if (/You put a fresh edge/.test(G[1])) f.push('sharpening: not stopped');
        if (/You aren't (filling|dipping|soaking)|You aren't doing anything/.test(G[2])) f.push('dipping: refused');
        if (/You soak the torch/.test(G[2])) f.push('dipping: not stopped');
        if ((G[2].match(/You set to dipping/g) || []).length !== 2) f.push('the torch not dipped twice');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lists', outfit: true, near: ['parser'], what: "Dingus Quest Zero's lists (2026-11-04: games/dq0/parts.c, HOUSE.md's lists; the user's \"and\" lists, commands in turn built on): 'drop the rope and the pick', 'take pick and rope' (no word to say they're things: the verb put to the later part, the parser's no verb), 'drop rope, pick' (a comma), 'take the pick and look' (a later part a command of its own stays one), 'put the pick and the rope in the sack' (what's said after the last for all), 'take and drop the pick' (a verb alone, the next one's thing), 'pick up the rope and the pick' (a verb of two words), nothing within quotes; 'shrug off the backpack' (the user's word, C20) lets it fall; any word after stop ('stop honing', 'stop eating'); a kind's word said before a name, 'attack the black orc' with none but rats: nothing like that here, and 'attack the sleeping rat', a word that makes no kind, passed over",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -k rope -T',
        'take the sack then hold the sack', 'drop the rope and the sack', 'take sack and rope', 'drop rope, sack', 'take the sack and look', 'take the rope', 'hold the sack', 'open the sack',
        'put the rope and the torch in the sack', 'i', 'take the rope from the sack', 'drop the rope', 'take and drop the rope', 'pick up the rope and the torch', 'say "north, and then east"',
        'stop honing', 'stop eating', 'attack the black orc', 'attack the sleeping rat', 'shrug off the backpack', 'i', 'quit', 'echo $status'] })) },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [], part = (a, b) => { const i = o.indexOf('> ' + a), j = b ? o.indexOf('> ' + b, i + 1) : -1; return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const n = (t, w) => (t.match(new RegExp(w, 'g')) || []).length;
        if (n(part('drop the rope and the sack', 'take sack and rope'), 'Put down|You put down') !== 2) f.push('"drop the rope and the sack" not each put down: ' + part('drop the rope and the sack', 'take sack and rope'));
        const t2 = part('take sack and rope', 'drop rope, sack');
        if (n(t2, 'Taken|you put it in your') !== 2 || /know how/.test(t2)) f.push('"take sack and rope" not both taken: ' + t2);
        if (n(part('drop rope, sack', 'take the sack and look'), 'Put down|You put down') !== 2) f.push('"drop rope, sack" not both put down');
        const t3 = part('take the sack and look', 'take the rope');
        if (!/Taken/.test(t3) || !/You are |You can see /.test(t3)) f.push('"take the sack and look" not a take and a look: ' + t3);
        const p = part('put the rope and the torch in the sack', 'i');
        if (/understand/.test(p)) f.push('"put the rope and the torch in the sack": the rope has no sack: ' + p);
        const inv = part('i', 'take the rope from the sack');
        if (!/sack[^;]*rope|rope[^;]*sack/.test(part('put the rope and the torch in the sack', 'take the rope from the sack'))) f.push('the rope not in the sack: ' + inv);
        const t4 = part('take and drop the rope', 'pick up the rope and the torch');
        if (/understand|nothing here to take/.test(t4) || !/Taken|You take|you put it in your/.test(t4) || !/Put down|You put down/.test(t4)) f.push('"take and drop the rope" not a take and a drop: ' + t4);
        const t5 = part('pick up the rope and the torch', 'say');
        if (/know how|understand/.test(t5)) f.push('"pick up the rope and the torch": ' + t5);
        if (!/You can't say|You say/.test(part('say "north, and then east"', 'stop honing'))) f.push('the quote split: ' + part('say "north, and then east"', 'stop honing'));
        if (/set off/.test(part('say "north, and then east"', 'stop honing'))) f.push('a way within quotes walked');
        for (const w of ['stop honing', 'stop eating']) if (!/doing anything to stop/.test(part(w, w === 'stop honing' ? 'stop eating' : 'attack the black'))) f.push(w + ': not a stop');
        if (!/nothing like that here/.test(part('attack the black orc', 'attack the sleeping'))) f.push('"attack the black orc" with none there: ' + part('attack the black orc', 'attack the sleeping'));
        if (/nothing like that here/.test(part('attack the sleeping rat', 'shrug'))) f.push('"attack the sleeping rat" refused for its word');
        const sh = part('shrug off the backpack', 'i');
        if (!/Put down|You put down the backpack/.test(sh)) f.push('"shrug off the backpack": ' + sh);
        if (/a backpack, worn/.test(o.slice(o.indexOf('> shrug off the backpack')))) f.push('the pack still worn after it\'s shrugged off');
        return f;
      },
    },
    {
      name: 'dq0liststop', reach: true, near: ['parser', 'gear'], what: "Dingus Quest Zero's list stopped (2026-11-04: parts.c, HOUSE.md's lists): by the reach's steps, 'take the rope and the torch' from the worn pack ('drop' first), each thing by its steps in turn ('You take off the backpack' once); 'wait 30 minutes and drop the torch' stopped by 'stop', typed as the wait begins: it's taken at once, not after the look, and the look goes with it (the rest of its line let go); and a line typed after a list stays",
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T',
        'wait 30 minutes and drop the torch', 'quit', 'echo $status'] })) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'wait 30 minutes and drop the torch', n: 1, words: 'You wait', text: 'stop\r' }]);
      },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [], i = o.indexOf('> wait 30 minutes and drop the torch');
        const t = o.slice(i, o.indexOf('> quit', i));
        if (!/You stop waiting/.test(t)) f.push('the wait not stopped: ' + t);
        if (/Put down|You let the/.test(t)) f.push('the rest of the line not let go: ' + t);
        return f;
      },
    },
    {
      name: 'dq0several', what: 'Dingus Quest Zero\'s things of several alike, and how many (the user\'s playtest, 2026-10-21: "if I have 3 sacks, and I say put the sack in the sack it says you can\'t put the sack inside itself"; "drop two sacks", "take 4 rations"): at seed 3 (dq0 -T, a kit of three sacks more and a torch) what\'s carried with things alike told together ("two sacks"); a sack put in another (never itself, nor one in it: the world\'s chooser, the one most to hand first), the first in the second (the ordinals for both things), one in the other ("the other", SL_OTHER), one taken from another; two of the sacks put down (each its own record), both taken up; 2 of the 3 rations put down (the lot split: a lot of 2 at its feet, 1 carried), 4 taken (only 2 there: told, and joined again to the one carried, one lot of 3), half put down (one), 2 put in a sack (the lot split, the part in it); all the sacks put down and taken up; the torch not lit put down (by its state, SL_UNLIT: the lit one still in hand), its words by the count throughout',
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(dq0Game({ lines: [
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'sack,sack,sack,torch\'', 'i', 'put the sack in the sack', 'take the sack from the sack',
        'put the first sack in the second sack', 'put the sack in the other sack', 'take the sack from the sack', 'take the sack from the sack',
        'drop two of the sacks', 'take both sacks', 'drop 2 rations', 'take 4 rations', 'drop half the rations', 'put 2 rations in the sack',
        'drop all the sacks', 'look', 'take all the sacks', 'drop the unlit torch', 'i', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), I = DQ0_I;
        const sacks = st => st ? st.items.filter(i => i.name === 'sack') : [];
        const lots = (st, where) => st ? st.items.filter(i => i.name === 'rations' && (where === 'carried' ? dq0Carried(st).includes(i) : where === 'here' ?
          i.where === 'IW_SQUARE' && i.sn === st.pc.sn && i.x === st.pc.x && i.y === st.pc.y : true)) : [];
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k \'sack,sack,sack,torch\'');
        if (!g.after) return ['the game not begun'];
        const inSack = st => sacks(st).filter(i => i.where === 'IW_ITEM' && sacks(st).some(j => j.n === i.x)).length;
        for (const [l, k, n] of [['put the sack in the sack', 0, 1], ['put the first sack in the second sack', 0, 1], ['put the sack in the other sack', 0, 2]]) {
          const t = T.of(l, k);
          if (!dq0Rep(t, 'ireq', { kind: 'IO_PUT', result: 'IR_OK' }).length || inSack(t.after) !== n) f.push(l + ': not one sack put in another (' + inSack(t.after) + ' in sacks)');
        }
        const ds = T.of('drop two of the sacks'), tb = T.of('take both sacks');
        if (dq0Here(ds.after).filter(i => i.name === 'sack').length !== 2) f.push('not two sacks put down: ' + dq0Here(ds.after).map(i => i.name));
        if (dq0Here(tb.after).filter(i => i.name === 'sack').length) f.push('not both taken up');
        const d2 = T.of('drop 2 rations'), t4 = T.of('take 4 rations'), dh = T.of('drop half the rations'), p2 = T.of('put 2 rations in the sack');
        const q = list => list.map(i => i.qty).sort().join(',');
        if (q(lots(d2.after, 'here')) !== '2' || q(lots(d2.after, 'carried')) !== '1') f.push('the rations not split, 2 put down, 1 kept: ' + q(lots(d2.after, 'here')) + ' / ' + q(lots(d2.after, 'carried')));
        if (lots(t4.after, 'here').length || q(lots(t4.after, 'carried')) !== '3') f.push('the 2 taken up not joined to the one carried, a lot of 3: ' + q(lots(t4.after, 'carried')));
        if (q(lots(dh.after, 'here')) !== '1' || q(lots(dh.after, 'carried')) !== '2') f.push('not half the rations put down: ' + q(lots(dh.after, 'here')) + ' / ' + q(lots(dh.after, 'carried')));
        const inS = p2.after ? lots(p2.after, 'carried').filter(i => i.where === 'IW_ITEM' && sacks(p2.after).some(j => j.n === i.x)) : [];
        if (q(inS) !== '2') f.push('not 2 rations put in a sack: ' + q(inS));
        const da = T.of('drop all the sacks'), ta = T.of('take all the sacks'), du = T.of('drop the unlit torch');
        if (sacks(da.after).some(i => dq0Carried(da.after).includes(i) && i.where !== 'IW_ITEM')) f.push('not all the sacks put down');
        if (!ta.after || sacks(ta.after).some(i => !dq0Carried(ta.after).includes(i))) f.push('not all the sacks taken up');
        const torches = du.after ? du.after.items.filter(i => i.name === 'torch') : [];
        if (!dq0Here(du.after).some(i => i.name === 'torch' && !(i.flags & I.LIT)) || !dq0Carried(du.after).some(i => i.name === 'torch' && (i.flags & I.LIT) && (i.flags & I.HELD)))
          f.push('not the unlit torch put down, the lit one kept in hand: ' + JSON.stringify(torches.map(i => [i.where, i.flags])));
        inTurn(s, ['> i', 'two sacks', '> put the sack in the sack', 'You put the sack in the sack.', '> put the first sack in the second sack',
          'You put the sack in the sack.', '> put the sack in the other sack', 'You put the sack in the sack.', '> drop two of the sacks',
          'You put down two sacks.', '> take both sacks', 'You take two sacks.', '> drop 2 rations', 'You put down 2 rations.', '> take 4 rations',
          'There are only 2 there; you take both.', '> drop half the rations', 'You put down a ration.', '> put 2 rations in the sack',
          'You put the 2 rations in the sack.', '> drop all the sacks', 'You put down', 'sacks.', '> take all the sacks', 'You take', 'sacks.',
          '> drop the unlit torch', 'You reach for your backpack.', 'Put down.'], f, 'the words: ');
        if (/won't go inside itself/.test(s)) f.push('a sack put in itself');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0walkwords', meaning: true, what: 'Dingus Quest Zero\'s walks told as they go (the user\'s playtest, 2026-10-21: "Crossing a room to a door says nothing till the door"; "in a passage two squares wide, every step says the passageway forks here"): at seed 3 the first room\'s doors opened and walked through to the hall, crossed, and back south across it to its door: the room\'s crossing told as the walk leaves it (the world\'s E_BEAT WB_CROSS, "You cross the room.") before the doorway\'s beat (WB_DOOR, "You go through a doorway."); then at seed 3\'s stratum below (caves, dq0 -T) east from the cave by the stairs into the tunnels: a walk set out from a wide fork crosses it (three squares the way said, not a square and a fork again: it was one square a walk) (its caves dry, dq0 -W: a stream along that passage would have slowed and turned the walk, slice 4.9\'s)',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm',
        'open the door behind me on my right', 'go through it', 'open', 'go through it', 'e', 'go through the door to the south', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -W', 'd', 'n', 'e', 'e', 'n', 'quit',
        'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm'), e = T.after(g, 'e'), so = T.after(e, 'go through the door to the south');
        const beats = so.events.filter(x => x.kind === 'E_BEAT').map(x => x.beat);
        const c = beats.indexOf('WB_CROSS'), d = beats.indexOf('WB_DOOR');
        if (c < 0 || d < 0 || c > d) f.push('the hall\'s crossing not told before its doorway: ' + beats.join(','));
        if (dq0Ev(so, 'E_ARRIVE', {}).some(x => x.how === 'A_CROSSED')) f.push('the walk south told as a crossing alone');
        const ans = s.slice(s.indexOf('> go through the door to the south', s.indexOf('> e')), s.indexOf('> quit'));
        inTurn(ans, ['You cross the room.', 'You go through a doorway.'], f, 'the walk south\'s words: ');
        const g2 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -W'), e1 = T.after(g2, 'e'), e2 = T.after(e1, 'e'), e3 = T.after(e2, 'n');   // (North from the wide fork: straight walks, 2026-11-02)
        if (!e2.after || !e3.after) return f.concat('the cave\'s walks not made');
        const dx = e3.after.pc.x - e2.after.pc.x, dy = e3.after.pc.y - e2.after.pc.y, n = Math.max(Math.abs(dx), Math.abs(dy));
        if (n < 2) f.push('the walk from the wide fork not across it: ' + dx + ',' + dy + ' (it was a square)');
        dq0Never(f, e3, 'E_NOWAY', {}, 'the walk from the fork refused');
        return f;
      },
      sdcard: true
    },
  ],
};
