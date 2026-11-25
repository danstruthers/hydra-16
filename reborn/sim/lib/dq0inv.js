// dq0inv.js - the invariants every Dingus Quest Zero test is held to, after every line it types (phase C's step 3,
// N:/.claude/critique/DQ0-CRITIQUE.md's section 6: "invariants checked after every line, in every test, free"): the
// world's state as the recorder kept it a line at a time (sim/lib/dq0meaning.js: m.dq0's snapshots, events and intents)
// and the words of every reply (the screen, m.out), each against the rules of games/dq0/WORLD-MODEL.md.  sim/test.js's
// dq0 instrument calls check(name, m) in every dq0 test; a broken invariant fails the test, but one on the known list
// (tests/dq0/invknown.txt: a bug the one-path rewrite is to end, two paths that disagree, each with its invariant and
// why) is a note instead, never passed over silently.  INV=0: none checked.
//
// The state's (each line's snapshot, and the test's end):
//   place      each thing in one place: its holder a thing that is, no thing in itself, no two records of one thing
//              (one id), held or worn not both (T1)
//   hands      a body's hands hold at most one thing each (B5), the character's by its hands' marks; a denizen's two at
//              most if it has two hands, a beast's mouth one; no thing a denizen's that's no actor's (did 0: the code
//              review's finding 1, a beast's own things every beast's)
//   hoard      a faction's hoard holds no more weight than it bears (T2: a thing is put only where it can rest)
//   handed     nobody hands a thing to itself (E_BEAT WB_HANDS_TO: whose, the receiver)
//   posts      a senseless actor posts no intent (B3: the senseless do nothing), its intent's moment's state (on
//              the known list: the minds a step behind the world, which makes it a wait)
//   rock       the character stands in no rock nor wall, nor does a denizen step into one (B2: the world's steps; a
//              test's scene may put one anywhere)
//   acts       a senseless body does nothing (B3): no blow struck, no step, no door worked, as it lies
// The words' (each reply: the story between one prompt and the next, the game's "> " alone):
//   repeat     no sentence more than three times in one reply but those told as they go by the user's decisions (a
//              fight's blows, a fire-maker's tries, a dig's or a climb's spells: GOES below)
//   feet       no "1 feet" (a foot)
//   twin       no "a X and a X" (two of them)
//   stop       no sentence run into the next with no space after its full stop
//   unseen     no denizen named the senses' record doesn't show seen, in that reply (S1, section 7: only seen names)
// And "nothing told twice in one answer" (section 7) is tests/lib/dq0.js's dq0Check's, as before.
'use strict';
const fs = require('fs');
const path = require('path');

// ---- The known list (tests/dq0/invknown.txt): a line each, "INVARIANT TEST-REGEX /WHAT-REGEX/ why"; a broken
// invariant of that name, in a test the regex matches, its words matching WHAT, a note, not a failure
const KNOWN_FILE = path.join(__dirname, '..', '..', 'tests', 'dq0', 'invknown.txt');
let KNOWN = null;
function known() {
  if (KNOWN) return KNOWN;
  KNOWN = [];
  let text = '';
  try { text = fs.readFileSync(KNOWN_FILE, 'utf8'); } catch (e) { return KNOWN; }
  for (const raw of text.split(/\r?\n/)) {
    const l = raw.replace(/^\s+|\s+$/g, '');
    if (!l || l[0] === '#') continue;
    const m = l.match(/^(\S+)\s+(\S+)\s+\/((?:[^/\\]|\\.)*)\/\s+(.*)$/);
    if (!m) throw new Error('invknown.txt: ' + l);
    KNOWN.push({ inv: m[1], test: new RegExp('^(?:' + m[2] + ')$'), what: new RegExp(m[3]), why: m[4] });
  }
  return KNOWN;
}

// ---- The words

// The replies: the story after each of the game's prompts ("> ", not rc's "% "), as tests/lib/dq0.js's dq0Answers
// (the status line, the escapes and the line typed out, its wrapping undone), each { cmd, text, game: its prompt's }
function replies(out) {
  const s = out.replace(/\r/g, '').replace(/\x1b\[7m[\s\S]*?\x1b\[0m/g, '')
    .replace(/\x1b\[\d+;1H\x1b\[K(?:\x1b\[\?25h)?[^\x1b\n]*/g, '').replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '');
  const res = [];
  let cmd = '', ans = [], game = false;
  for (const l of s.split('\n')) {
    const p = l.match(/^([>%]) (.*)$/);
    if (p) { res.push({ cmd, game, text: ans.join(' ').replace(/\s+/g, ' ').trim() }); cmd = p[2].trim(); game = p[1] === '>'; ans = []; continue; }
    ans.push(l);
  }
  res.push({ cmd, game, text: ans.join(' ').replace(/\s+/g, ' ').trim() });
  return res;
}
const sentences = t => t.split(/(?<=[.!?])\s+/).filter(x => x.length > 8 && /[a-z]{2}/i.test(x));

// Told as it goes, each time (the user's decisions): such a sentence may come more than three times in one reply
const GOES = [
  // (A fight's rounds, each blow told as it falls (the user's, 2026-10-20: fighting on), its falls, risings, turns and tries
  // to rise; a grapple's rounds, HOUSE.md's grappling, 2026-11-09; its fumbles; commands typed ahead taking it over)
  /^(?:(?:The|A) .* |Something |[A-Z][a-z]+ )(?:bites|claws|strikes|swings|lunges|thrusts|shoots|throws|stings|gores|kicks|butts)\b.*\bat you\b/,
  /^You (?:strike|swing|thrust|punch|kick|lunge|shoot|throw|grapple)\b/,
  /^(?:Your|Its|Their) [a-z]+ strikes .* instead\b/,
  /^(?:You get to your feet|You fall|You struggle, but can't get up|You stop fighting|You turn to .+|You fight back against .+|The .+ (?:falls|gets to (?:its|their) feet|closes with you|takes aim at you|goes down|rises again)|Something (?:gets to its feet|falls|coils itself up again))\.$/,
  /^(?:You (?:fumble for|reach for) the .+|You drop your .+|The .+ drops (?:its|their) .+)\.$/,
  /^(?:You choke .+|(?:The [a-z ]+|Something) (?:struggles in your grip|resists your hold))\.$/,
  /^(?:The [a-z ]+|Something) (?:struggles|scrabbles|writhes|flails its legs|flaps|wavers), but can't (?:get up|right itself|rise)\.$/,
  // (A fire-maker's tries: the user's, 2026-11-02, "continue until successful, reporting as it goes along")
  /^(?:You strike a spark|It doesn't catch)\.$/,
  // (A dig's spells, a climb's rounds, a battering's beats: each told as it ends, the user's, 2026-10-22)
  /^(?:\(\d+ feet to go\.\) |\(A foot to go\.\) )?(?:The work goes (?:well|slowly)|Your tool slips and turns; little comes of that spell of work)\.(?: \(.*\))?$/,
  /^\((?:\d+ feet|A foot) to go\.\)/,
  /^You (?:climb on|slip back a little|find no hold yet) \((?:\d+ feet|a foot) to go\)\.$/,
  /^You keep at it\.$/,
  // (A walk's beats as it goes, slice 4.0's and 4.10's: an incline told four feet of rise at a time, dq0incline's two or more)
  /^The way (?:rises|falls away)\.$/,
  // (Each stir its own telling, what it sees of the denizens changed: tests/lib/dq0.js's dq0Repeats tells each apart)
  /^Something stirs\.$/,
];

function words(name, out) {
  const f = [];
  for (const a of replies(out || '')) {
    if (!a.game || !a.text) continue;
    const at = ' (> ' + a.cmd + ')';
    const n = new Map();
    for (const t of sentences(a.text)) n.set(t, (n.get(t) || 0) + 1);
    for (const [t, c] of n) if (c > 3 && !GOES.some(r => r.test(t))) f.push(['repeat', 'told ' + c + ' times in one reply' + at + ': ' + JSON.stringify(t)]);
    let m;
    const feet = /\b1 feet\b/g;
    while ((m = feet.exec(a.text))) f.push(['feet', '"1 feet"' + at + ': ' + JSON.stringify(a.text.slice(Math.max(0, m.index - 40), m.index + 20))]);
    const twin = /\b((?:an?|the) [a-z][a-z' -]*?) and \1(?=[.,;:!?) ]|$)/g;
    while ((m = twin.exec(a.text))) f.push(['twin', '"' + m[0] + '"' + at]);
    const stop = /[a-z][.!?][A-Z]/g;
    while ((m = stop.exec(a.text))) f.push(['stop', 'no space after a full stop' + at + ': ' + JSON.stringify(a.text.slice(Math.max(0, m.index - 30), m.index + 30))]);
  }
  return f;
}

// ---- The state

const IW = { SQUARE: 'IW_SQUARE', ACTOR: 'IW_ACTOR', ITEM: 'IW_ITEM', PORTAL: 'IW_PORTAL', DEN: 'IW_DEN' };
const I_HELD = 0x02, I_WORN = 0x04, IH_LEFT = 1, IH_RIGHT = 2;
const K_CONTAINER = 0x01, K_BELT = 0x02, K_PLURAL = 0x40, KF_COUNT = 0x40, KF_TENS = 0x10, KF_SMALL = 0x02, KF_ON = 0x10;
const OW_HOARD = 0x80, IC_MARK = 0xFF00;

// Each snapshot's things (a line's, the end's), its faults: [invariant, words]
function things(s, L, kit = new Set()) {
  const f = [], by = new Map(s.items.map(i => [i.n, i])), ids = new Map();
  const what = i => (i.name || 'kind ' + i.kind) + ' #' + i.n;
  for (const i of s.items) {
    if (i.id) {
      if (ids.has(i.id)) f.push(['place', 'one thing in two records (id ' + i.id + '): ' + what(ids.get(i.id)) + ', ' + what(i)]);
      else ids.set(i.id, i);
    }
    if ((i.flags & (I_HELD | I_WORN)) === (I_HELD | I_WORN)) f.push(['place', what(i) + ' held and worn at once']);
    if (i.where === IW.ITEM) {
      let j = i, d = 0;
      for (; j && j.where === IW.ITEM && d < 12; d++) {
        const h = by.get(j.x);
        if (!h) { f.push(['place', what(i) + ' in thing #' + j.x + ', which isn\'t there']); j = null; break; }
        if (h === i) { f.push(['place', what(i) + ' in itself']); j = null; break; }
        j = h;
      }
      if (j && d >= 12) f.push(['place', what(i) + ': its holders go round']);
    } else if (i.where === IW.ACTOR && i.x !== 0) f.push(['place', what(i) + ' an actor\'s (IW_ACTOR) not the character\'s: ' + i.x]);
  }
  // (Hands: the character's, its things held by their hands' marks; each denizen's, its held things counted)
  const hand = [0, 0];
  for (const i of s.items) if (i.where === IW.ACTOR && i.x === 0 && (i.flags & I_HELD)) {
    if (i.sn & IH_LEFT) hand[0]++;
    if (i.sn & IH_RIGHT) hand[1]++;
    if (!(i.sn & (IH_LEFT | IH_RIGHT))) f.push(['hands', what(i) + ' held by the character in no hand (' + i.sn + ')']);
  }
  if (hand[0] > 1 || hand[1] > 1) f.push(['hands', 'the character\'s hands hold ' + hand[0] + ' (left) and ' + hand[1] + ' (right): ' +
    s.items.filter(i => i.where === IW.ACTOR && i.x === 0 && (i.flags & I_HELD)).map(i => what(i) + ' (hand ' + i.sn + ')').join(', ') +
    (s.items.some(i => kit.has(i.n) && i.where === IW.ACTOR && (i.flags & I_HELD)) ? " (the tests' kit held all the same: dq0 -k)" : '')]);
  const held = new Map();
  for (const i of s.items) if (i.where === IW.DEN && (i.flags & I_HELD)) held.set(i.did, [...(held.get(i.did) || []), i]);
  for (const [did, list] of held) {
    const a = s.actors.find(x => x.did === did && (x.state & 1));
    if (!did) { f.push(['hands', 'held by no actor (did 0, a beast\'s: all beasts\' at once): ' + list.map(what).join(', ')]); continue; }
    if (!a) continue;                                        // (Its denizen no actor now: its record's, not the world's)
    const cap = a.body === L.BD_TWO ? 2 : 1;
    if (list.length > cap) f.push(['hands', (a.name || 'actor ' + a.k) + ' holds ' + list.length + ' things (' + (cap === 2 ? 'two hands' : 'a mouth') + '): ' + list.map(what).join(', ')]);
  }
  // (A faction's hoard: what's in it, its first compartment's, by weight, against what that bears)
  const wt = i => {                                          // (things.c's s_weight_of)
    let w = i.weight * (i.kflags & K_PLURAL && !(i.kform & KF_COUNT) ? 1 : i.qty);
    if (i.kform & KF_TENS) w = i.kform & KF_SMALL ? (w + 32) >> 6 : Math.floor((w + 9) / 10);
    if (i.kflags & (K_CONTAINER | K_BELT) || i.kkeeps & KF_ON) for (const j of s.items) if (j.where === IW.ITEM && j.x === i.n) w += wt(j);
    return w;
  };
  for (const h of s.items) if (h.owner & OW_HOARD && h.where !== IW.DEN && h.capWeight) {
    const w = s.items.filter(j => j.where === IW.ITEM && j.x === h.n && (j.y < IC_MARK || !(j.y & 15))).reduce((a, j) => a + wt(j), 0);
    if (w > h.capWeight) f.push(['hoard', 'the hoard ' + what(h) + ' holds ' + w + ' (quarter pounds), it bears ' + h.capWeight]);
  }
  // (The character on rock: the square the recorder read under it; a denizen's, as it steps there, acts' below: a
  // test's scene puts them where it will, out of the way, its pokes no step of the world's)
  if (s.pc.tile === 0 || s.pc.tile === 2) f.push(['rock', 'the character stands in ' + (s.pc.tile ? 'a wall' : 'the rock') + ' at ' + s.pc.x + ',' + s.pc.y]);
  return f;
}

// The denizens' intents and events: a dead or senseless one's intent; a thing handed to oneself
const DOES = /^DE_(?:MOVED|DIG|DUG|DIGS|OPENS|SHUTS|SLAMS|UNLOCKS|LOCKS|BARS|UNBARS|STRAINS|BURSTS|BATTERS|BREAKS|DRAGS)$/;   // (A body's own acts)
const BOOK = new Set(['I_ENTER', 'I_LEAVE', 'I_KIT', 'I_NONE', 'I_DONE', 'I_SWEPT', 'I_TROD', 'I_ROUSE']);   // (Its record's, not its body's acts)
function acts(g) {
  const f = [];
  for (const q of g.intents || []) {
    if (q.ring !== 'den' || BOOK.has(q.kind) || !q.as) continue;
    if ((q.as.state & 1) && q.as.doing === 'DO_DOWN') f.push(['posts', q.as.name + ', senseless, posts ' + q.kind + ' (line ' + q.line + ')']);
  }
  for (const e of [...(g.events || []), ...(g.dens || [])]) {
    if (e.kind === 'E_BEAT' && e.beat === 'WB_HANDS_TO' && e.whose && e.whose === e.other) f.push(['handed', 'actor ' + (e.whose - 1) + ' hands a thing to itself (line ' + e.line + ')']);
    // (A body's step onto the rock or into a wall, B2: the world's own, told as it's taken)
    if (e.kind === 'E_DEN' && e.de === 'DE_MOVED' && (e.tile === 0 || e.tile === 2)) f.push(['rock', (e.as ? e.as.name : 'actor ' + (e.who - 1)) + ' steps into ' + (e.tile ? 'a wall' : 'the rock') + ' at ' + e.sx + ',' + e.sy + ' (line ' + e.line + ')']);
    // (What a senseless body does: nothing, B3; its blow struck, its step taken, a door worked, a spell dug, as it lay;
    // its turn spent getting up, or taking its weapon up, not a blow)
    if (!e.as || e.as.doing !== 'DO_DOWN') continue;
    if (e.kind === 'E_FIGHT' && !(e.ff & 1) && !/RISE|REARM|^FO_/.test(e.out)) f.push(['acts', e.as.name + ', senseless, strikes (' + e.out + ', line ' + e.line + ')']);
    if (e.kind === 'E_DEN' && DOES.test(e.de)) f.push(['acts', e.as.name + ', senseless, ' + e.de + ' (line ' + e.line + ')']);
  }
  return f;
}

// Who is named in a reply that wasn't seen: each actor's name (its kind's, "the bandit", or its own, "Grub") said as a
// denizen's ("The bandit ...", "Grub ...") in a reply of the line whose sights (the senses' record's denizens, as the
// recorder read them through that line, and those the world told seen) have none of that name
function unseen(r, rs) {
  const f = [], names = new Set(), seenAt = {};
  for (const g of r.games) {
    for (const s of g.snaps || []) for (const a of s.actors || []) if (a.name) names.add(a.name);
    for (const [k, v] of Object.entries(g.seen || {})) seenAt[k] = new Set([...(seenAt[k] || []), ...v]);
  }
  if (!names.size) return f;
  const esc = s => s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  let at = 0;
  const lines = (r.lines || []).map(l => l.trim());
  rs.forEach(a => {                                          // (Each reply the answer to its line: the next typed so)
    const j = a.cmd ? lines.indexOf(a.cmd, at) : -1;
    if (j < 0) return;
    at = j + 1;
    const line = j + 1;                                      // (Its CR the line-th: the recorder's enters then)
    if (!a.game || !a.text) return;
    const seen = new Set([...(seenAt[line - 1] || []), ...(seenAt[line] || [])]);
    for (const n of names) {
      if (seen.has(n)) continue;
      const re = new RegExp('(?:^|[.!?:] )(?:The ' + esc(n) + (n[0] < 'a' ? '|' + esc(n) : '') + ')\\b');
      if (re.test(a.text)) f.push(['unseen', '"' + n + '" named, not seen (> ' + a.cmd + ', line ' + line + ')']);
    }
  });
  return f;
}

function check(name, m) {
  if (process.env.INV === '0') return { failures: [], notes: [] };
  const all = [];
  for (const [inv, w] of words(name, m.out)) all.push({ inv, w });
  const r = m.dq0;
  if (r && r.games) {
    const { L } = r.layout ? { L: Object.assign({}, r.layout().L, { BD_TWO: r.layout().K.BD_TWO }) } : { L: {} };
    r.games.forEach((g, gi) => {
      const pre = r.games.length > 1 ? 'game ' + (gi + 1) + ': ' : '';
      const seen = new Set(), s0 = (g.snaps || []).find(s => s.game === undefined || s.game === gi);
      const kit = new Set(s0 && things(s0, L).some(([inv]) => inv === 'hands') ? s0.items.filter(i => i.where === IW.ACTOR && i.x === 0 && (i.flags & I_HELD)).map(i => i.n) : []);   // (Held as the game began, more than the hands hold: the tests' kit's, dq0 -k's "held all the same", things.c's s_kit)
      for (const s of [...(g.snaps || []), ...(gi === r.games.length - 1 && r.final ? [r.final()].filter(Boolean) : [])]) {
        if (s.game !== undefined && s.game !== gi) continue;
        for (const [inv, w] of things(s, L, kit)) {
          const k = inv + w;
          if (!seen.has(k)) { seen.add(k); all.push({ inv, w: pre + w + ' (at line ' + s.line + ')' }); }
        }
      }
      for (const [inv, w] of acts(g)) all.push({ inv, w: pre + w });
    });
    if (r.games.length) for (const [inv, w] of unseen(r, replies(m.out || ''))) all.push({ inv, w });
  }
  const failures = [], notes = [], K = known();
  for (const { inv, w } of all) {
    const k = K.find(e => e.inv === inv && e.test.test(name.replace(/@\d+$/, '')) && e.what.test(w));
    if (k) notes.push('invariant ' + inv + ' (known: ' + k.why + '): ' + w);
    else failures.push('invariant ' + inv + ': ' + w);
  }
  return { failures, notes };
}

module.exports = { check, replies, words, things, acts, GOES };
