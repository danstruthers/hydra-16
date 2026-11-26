// ****************************************************************************
// time.js - Dingus Quest Zero's tests: time: the pace, long tasks, waiting.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { card, typed } = require('../../lib/common.js');
const {
  CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N, CSLOT_AT, liveChunks, stratumRec, chunkDiff, dq0Story, dq0Prose, inTurn, dq0Turns,
  dq0Ev, dq0Rep, dq0Evs, dq0Need, dq0Never, dq0Thing, DQ0_I, dq0Typed, dq0Game, dq0Rec, dq0Alone, dq0Scene, dq0SceneNow, dq0Timer, DQ0_PLAYED
} = require('../../lib/dq0.js');
const { trend, target, D17 } = require('../../lib/trends.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')
// (dq0lockstep's lines: the rats about the way down at seed 3 moving as the character waits, turns and looks)
// (dq0lockwake's: dq0wakeblow's sleeper among the cultists, put to sleep in the dark; each game's lines' lag)
const LOCK_WAKE = ['douse the torch', 'sleep'], LOCK_WAKE_LAGS = [0, 9000, 37000, 211000];
const LOCK_LINES = ['look', 'wait 1 minute', 'turn around', 'wait 30 seconds', 'look', 'inventory', 'wait 1 minute', 'turn left', 'wait 2 minutes', 'look'];

// Dingus Quest Zero's tasks' overlays as they are in their banks (at the first prompt: dq0starttime's), each the same as
// ld65 made it (obj/games/NAME.hyx.ovl, each bank filled out to 8K with $FF): read from the card packed and unpacked
// in place by the unpack library (modules/unpack, games/dq0/ovlpack.js, 2026-10-31).  Each task by its name (its task
// RAM's TA_NAME), its banks by its _ovbank (its labels).  OUT: a line a task, "ok ..." or what's wrong
function dq0OvlBanks(m) {
  const out = [], root = path.join(TESTS, '..');
  for (const name of ['dq0', 'dq0w', 'dq0m', 'dq0harn', 'dq0p', 'dq0d']) {
    let t = -1;
    for (let k = 1; k < 16 && t < 0; k++) {
      let n = '';
      for (let i = 0; i < 12 && m.taskRam[k][0x230 + i]; i++) n += String.fromCharCode(m.taskRam[k][0x230 + i]);
      if (n === name) t = k;
    }
    if (t < 0) { out.push(name + ': no task'); continue; }
    const lbl = /^al ([0-9A-F]+) \._ovbank$/m.exec(fs.readFileSync(path.join(root, 'obj', 'games', name, name + '.lbl'), 'latin1'));
    const ovl = fs.readFileSync(path.join(root, 'obj', 'games', name + '.hyx.ovl'));
    let n = Math.ceil(ovl.length / 0x2000), bad = 0;
    const want = k => { const b = Buffer.alloc(0x2000, 0xFF); ovl.copy(b, 0, k * 0x2000, Math.min(ovl.length, (k + 1) * 0x2000)); return b; };
    while (n > 1 && want(n - 1).every(x => x === 0xFF)) n--;
    for (let k = 0; k < n; k++) {
      const b = m.taskBankMem(t, m.taskRam[t][parseInt(lbl[1], 16) + k]), w = want(k);
      let d = -1;
      const data = name === 'dq0' && new RegExp('^OV' + (k + 1) + 'BSS\\s+00([89A-F][0-9A-F]{3})', 'm').exec(fs.readFileSync(path.join(root, 'obj', 'games', name, name + '.map'), 'latin1'));
      const end = name === 'dq0' && k === 4 ? 0x1000 : data ? parseInt(data[1], 16) - 0x8000 : 0x2000;  // (dq0's fifth: its second half the theme's, read in
                                                // as it starts: theme.h's THEME_PLACE; its 22nd's and 23rd's data, OV22BSS, OV23BSS, last: the designators', written as it goes)
      for (let i = 0; i < end && d < 0; i++) if (!b || b[i] !== w[i]) d = i;
      if (d >= 0) { bad++; out.push(name + '\'s overlay ' + (k + 1) + ' not as ld65 made it, from $' + (0x8000 + d).toString(16).toUpperCase()); }
    }
    if (!bad) out.push('ok: ' + name + '\'s ' + n + ' overlays as ld65 made them');
  }
  return out;
}

// dq0jobs: the world's periodic work made to fall on an answer, whatever the phase (the user's D17: a build's
// inventory had its first letter at 889,825 cycles, the traces' journal taken in just as it was typed; which answer it
// falls on moves with any change to the code).  At the normal pace, seed 3: what's carried asked again and again, each
// time the job poked due a set time before its Enter (0, 100,000, 250,000 and 450,000 cycles): the lights' minute (the
// world's burn_due, 0), the traces' journal (tr_n full, held empty till then so there's a journal to take in), and both;
// then twenty more at waits that move the phase.  Each within its budget, and each job shown to have run (its stub called:
// given way to, not lost).  And (the user's, 2026-11-17) the same typed again at the menacing pace, and as the game is
// played (the normal pace without the runner's switches: DQ0_PLAYED), each held to the same target, its measures its
// own (", menacing", ", played")
function dq0JobsTest() {
  const BEFORE = [0, 100000, 250000, 450000], LINE = 'inventory', WANT = 'You carry', MAX = D17.first;
  const each = ['burn', 'trace', 'both'].flatMap(j => BEFORE.map(d => ({ j, d })));
  const pre = ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl'], game = 'dq0 -p n -s 3 -c /ram/c.hm -T';
  const games = [game, game.replace(' -p n', ' -p m'), DQ0_PLAYED + game], sfx = ['', ', menacing', ', played'];
  const free = 20, gaps = [1, 2, 3, 1, 4, 2];
  const play = [...each.map(() => LINE), ...Array(free).fill(LINE)];
  const input = [...pre.map(l => 'ā' + l + '\r'), ...games.flatMap(g => ['ā' + g + '\r', 'ālook\r',
    ...play.map((l, k) => 'ā' + 'Ā'.repeat(k < each.length ? 3 : gaps[k % gaps.length]) + l + '\r'), 'āquit\r']), 'āecho $status\r'].join('');
  return {
    name: 'dq0jobs', what: 'Dingus Quest Zero\'s periodic work giving way to a command (the user\'s D17, a command\'s first letter within 720,000 cycles): the lights\' minute and the traces\' journal made due a set time before an Enter, each answer within its budget and each job run after it; and what\'s carried asked at waits that move the phase',
    meaning: true, outfit: true,
    init: 't_rc', cycles: 2500e6,
    machine: { input },
    expect: ['% echo $status'],
    start(m) {
      this.timer = dq0Timer(m);
      const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0w', 'dq0w.lbl'), 'latin1');
      const at = n => parseInt(new RegExp('^al ([0-9A-F]+) \\.' + n + '$', 'm').exec(lbl)[1], 16);
      const burn = at('_burn_job'), flush = at('_tr_flush');
      const a = m.acia, cpu = m.cpu, step = cpu.step.bind(cpu), ran = this.ran = { burn: 0, trace: 0, burnWant: 0, traceWant: 0 }, read = a.read;
      let qlen = -1, k = -1, fire = 0, job = null, world = -1, held = 0, typed = '';
      a.read = (r, t) => {                      // (Each game its jobs again, its world's: as its dq0 line's entered)
        if (r === 0 && a.rdrf) {
          const c = a.rx;
          if (c === 13) { if (/^dq0 /.test(typed)) { k = world = -1; fire = 0; } typed = ''; }
          else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
        }
        return read(r, t);
      };
      cpu.step = v => {
        if (a.rxQueue.length !== qlen) {
          qlen = a.rxQueue.length;
          if (a.rxQueue.length && a.rxQueue.slice(0, LINE.length).join('') === LINE && k + 1 < each.length && a.rxDelay >= 1000000) {
            job = each[++k];                    // (The wait before the line begun: its Enter a character's time a letter after it)
            fire = cpu.cyc + a.rxDelay + (LINE.length + 1) * 20000 - job.d;
            if (world < 0) world = m.dq0.bank(0)[12 + 1];
          }
        }
        if (world >= 0) {
          // (Each made due by a scene, the world's own between its intents: SO_DUE, OPEN.md's X1, burn_due's poke lost when
          // a minute was under way; the journal held empty the same, not while a scene's on its way: the next one's)
          if (fire && cpu.cyc >= fire) {
            const j = job.j;
            dq0SceneNow(m, (h, L, K, at, named, sc) => { if (j !== 'trace') sc.due('burn'); if (j !== 'burn') sc.due('trace'); }, null, 'dq0jobs\'s ' + j + ' ' + job.d + ' before');
            if (j !== 'trace') ran.burnWant++;
            if (j !== 'burn') ran.traceWant++;
            fire = 0;
          } else if (fire && job.j !== 'burn' && cpu.cyc - held > 400000 && !(m.dq0Scenes && (m.dq0Scenes.cur || m.dq0Scenes.queue.length))) {
            held = cpu.cyc;
            dq0SceneNow(m, (h, L, K, at, named, sc) => sc.due('hold'), null, 'dq0jobs\'s journal held');   // (Its journal filling till then)
          }
          if (m.T === world) {
            if (cpu.PC === burn) ran.burn++;
            else if (cpu.PC === flush) ran.trace++;
          }
        }
        return step(v);
      };
    },
    check() {
      const f = [], notes = this.notes = [], R = this.ran;
      games.forEach((x, g) => this.timer.game(g, play.map(l => ({ line: l, want: WANT }))).forEach((r, i) => {
        const j = each[i], name = (j ? j.j + ' ' + j.d + ' before' : 'free ' + (i - each.length)) + sfx[g];
        notes.push(name + ': first letter ' + r.first);
        if (target(this, name + ': first letter', r.first, MAX)) f.push(name + ': its first letter ' + r.first + ' cycles from Enter (the user\'s ' + MAX + ')');
      }));
      notes.push('the jobs made due: lights ' + R.burnWant + ', traces ' + R.traceWant + '; run: lights ' + R.burn + ', traces ' + R.trace);
      if (R.burn < R.burnWant) f.push('the lights\' minute made due ' + R.burnWant + ' times, run ' + R.burn);
      if (R.trace < R.traceWant) f.push('the traces\' journal made due ' + R.traceWant + ' times, taken in ' + R.trace);
      return f;
    },
    sdcard: true
  };
}

// dq0hold: the world's long jobs holding back from a line's first key to its answer's first word (the user's "hold back",
// 2026-11-04), by scenes, the jobs poked due as dq0jobs does.  At the normal pace, seed 3: a long line typed slowly, a job
// made due just after its first key, another just after the first's run: each held about KEY_HOLD ticks (keys_hold's: 600,
// 10.7M cycles), each run though the keys go on, none starved; the line let go by Ctrl-U, a job at once; "inventory" typed
// fast with both due from its first key: neither before its first letter, both after it.  At the turns pace nothing held,
// the same typing and a job run with the keys on the line: a turns-pace game turns on no one's typing.  The denizens' own
// work held the same way (2026-11-06): no mind deciding (dq0d), no senses' answer to its ask (dq0p) and no intents of
// theirs carried out (the world's) in the long line's first HELD cycles of keys nor from inventory's first key to its first
// letter; some of it run as the long line's keys go on past LIMIT
function dq0HoldTest() {
  const LONG = 'look about the room for a while', LINE = 'inventory', WANT = 'You carry', MAX = D17.first, HELD = 8000000, LIMIT = 16000000, FREE = 1200000;
  const pre = ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl'];
  const slow = [...LONG].join('Ā');
  // (At the normal pace the long line let go part way, as the denizens are about (seed 3's: a mind deciding, the senses and
  // their intents some 30M cycles on), and typed again from its "room": a line's first key as they're at it, its hold theirs)
  const slow1 = [...LONG.slice(0, 14)].join('Ā') + 'Ā\x15Ā' + [...LONG.slice(15)].join('Ā');
  const lines = [...pre, 'dq0 -p n -s 3 -c /ram/c.hm -T', LINE, 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'quit', 'echo $status'];
  // (And the normal pace's typed again at the menacing pace, and as the game is played (the user's, 2026-11-17: without the
  // runner's switches, DQ0_PLAYED), inventory's first letter held as the normal pace's)
  const more = ['dq0 -p m -s 3 -c /ram/c.hm -T', DQ0_PLAYED + 'dq0 -p n -s 3 -c /ram/c.hm -T'].map(g => 'ā' + g + '\r' + 'ā' + slow1 + 'Ā\x15ĀĀ' + LINE + '\rāquit\r').join('');
  const input = pre.map(l => 'ā' + l + '\r').join('') + 'ā' + lines[3] + '\r' + 'ā' + slow1 + 'Ā\x15ĀĀ' + LINE + '\rā' + 'quit\r' + 'ā' + lines[6] + '\r' + 'ā' + slow + '\x15Ā' + 'quit\r' + more + 'āecho $status\r';
  return {
    name: 'dq0hold', what: 'Dingus Quest Zero\'s world\'s long jobs waiting from a line\'s first key to its answer\'s first word at the real paces, three seconds at most while keys go on (the user\'s "hold back"): the lights\' minute and the traces\' journal made due as a long line is typed slowly, as it\'s let go, and as a line\'s typed fast; at the turns pace not held',
    meaning: true, outfit: true,
    init: 't_rc', cycles: 2600e6,
    machine: { input },
    expect: ['% echo $status'],
    start(m) {
      this.timer = dq0Timer(m);
      const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0w', 'dq0w.lbl'), 'latin1');
      const at = n => parseInt(new RegExp('^al ([0-9A-F]+) \\.' + n + '$', 'm').exec(lbl)[1], 16);
      const burn = at('_burn_job'), flush = at('_tr_flush');
      const a = m.acia, cpu = m.cpu, step = cpu.step.bind(cpu), read = a.read, R = this.R = {};
      // (The background's own work, each run's cycle at the normal pace: a mind deciding, the senses answering a mind's ask,
      // the denizens' intents carried out, by their first addresses in their tasks: dq0d, dq0p, the world)
      const atIn = (p, n) => parseInt(new RegExp('^al ([0-9A-F]+) \\.' + n + '$', 'm').exec(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', p, p + '.lbl'), 'latin1'))[1], 16);
      const BG = [['decide', 'dq0d', 2, '_decide'], ['sense', 'dq0p', 6, '_sense_actor'], ['intents', 'dq0w', 1, '_den_intents']].map(([k, p, t, n]) => ({ k, t, pc: atIn(p, n) }));
      const runs = this.runs = { decide: [], sense: [], intents: [] };
      let world = -1, game = 0, typed = '';
      const plan = [], wants = { burn: null, trace: null };
      // (A job made due so many cycles from now, and looked for by its first address, every run of it its name's)
      const go = (name, what, delay) => { world = m.dq0.bank(0)[12 + 1]; BG.forEach(b => { b.task = game === 1 ? m.dq0.bank(0)[12 + b.t] : -1; }); plan.push({ name, what, at: cpu.cyc + delay }); };
      a.read = (r, t) => {
        if (r === 0 && a.rdrf) {
          const c = a.rx;
          if (c === 13) {
            if (/^dq0 -p/.test(typed)) game++;
            if (typed === LINE && game === 1) R.enter = a.rxAt;
            typed = '';
          } else if (c === 0x15) {
            R['letgo' + game] = a.rxAt;
            if (game === 1) go('letgo', 'burn', 300000);
            typed = '';
          } else {
            typed += String.fromCharCode(c);
            if (typed.length === 1 && game === 1) R['key' + typed] = a.rxAt;   // (A line's first key: the long line's, inventory's)
            if (typed === 'l' && game <= 2) go(game === 1 ? 'held1' : 'turns', 'burn', 100000);
            if (typed === 'i' && game === 1) { go('fastburn', 'burn', 100000); go('fasttrace', 'trace', 100000); }
          }
        }
        return read(r, t);
      };
      cpu.step = v => {
        const c = cpu.cyc;
        if (plan.length && c >= plan[0].at && (plan[0].name === 'letgo' || m.dq0.bank(0)[m.dq0.layout().L.uiKeys])) {   // (Its keys seen by the UI, as they may not be yet: a status line being drawn)
          const p = plan.shift();               // (Made due by a scene, the world's own: SO_DUE, OPEN.md's X1)
          dq0SceneNow(m, (h, L, K, at, named, sc) => sc.due(p.what), null, 'dq0hold\'s ' + p.name);
          R[p.name] = { made: c, ran: 0, keys: m.dq0.bank(0)[m.dq0.layout().L.uiKeys] };
          wants[p.what] = p.name;
        }
        if (m.T === world && (cpu.PC === burn || cpu.PC === flush)) {
          const w = cpu.PC === burn ? 'burn' : 'trace', n = wants[w];
          if (n) {
            R[n].ran = c;
            wants[w] = null;
            if (n === 'held1') go('held2', 'trace', 400000);
          }
        }
        for (const b of BG) if (m.T === b.task && cpu.PC === b.pc) runs[b.k].push(c);
        for (const k of ['l', 'r', 'i']) if (R['key' + k] && !R['seen' + k] && m.dq0.bank(0)[m.dq0.layout().L.uiKeys]) R['seen' + k] = c;   // (The first key as the UI has it)
        return step(v);
      };
    },
    check() {
      const f = [], notes = this.notes = [], R = this.R, late = n => R[n] && R[n].ran ? R[n].ran - R[n].made : null;
      for (const n of ['held1', 'held2']) {
        const l = late(n);
        notes.push(n + ': made due with ' + (R[n] && R[n].keys) + ' keys on the line, run ' + l + ' cycles later');
        if (!(l !== null && l >= HELD)) f.push(n + ': run ' + l + ' cycles after it was due, a line being typed: not held back (at least ' + HELD + ')');
        if (!(l !== null && l <= LIMIT)) f.push(n + ': run ' + l + ' cycles after it was due: held too long (at most ' + LIMIT + ')');
        if (R[n] && R[n].ran > R.letgo1) f.push(n + ': run only after the line was let go, not as the keys went on');
      }
      const g = late('letgo');
      notes.push('the line let go: run ' + g + ' cycles after it was due');
      if (!(g !== null && g <= FREE)) f.push('no keys on the line: a job ' + g + ' cycles late (at most ' + FREE + ')');
      const t = late('turns');
      notes.push('the turns pace: run ' + t + ' cycles after it was due, keys on the line');
      if (!(t !== null && t <= FREE)) f.push('the turns pace: a job ' + t + ' cycles late, held for the keys (at most ' + FREE + ')');
      const r = this.timer(lines, [{ line: LINE, want: WANT }])[0];
      notes.push(LINE + ' typed fast: first letter ' + r.first + '; the lights\' minute run ' + (R.fastburn ? R.fastburn.ran - R.enter : null) + ', the journal ' + (R.fasttrace ? R.fasttrace.ran - R.enter : null) + ' after Enter');
      if (target(this, LINE + ': first letter', r.first, MAX)) f.push(LINE + ': its first letter ' + r.first + ' cycles from Enter (the user\'s ' + MAX + ')');
      ['menacing', 'played'].forEach((p, i) => {   // (Typed again at the menacing pace, and as the game is played: their third and fourth games)
        const q = this.timer.game(2 + i, [{ line: LINE, want: WANT }])[0];
        notes.push(LINE + ', ' + p + ': first letter ' + q.first);
        if (target(this, LINE + ', ' + p + ': first letter', q.first, MAX)) f.push(LINE + ', ' + p + ': its first letter ' + q.first + ' cycles from Enter (the user\'s ' + MAX + ')');
      });
      for (const n of ['fastburn', 'fasttrace']) {
        if (!(R[n] && R[n].ran)) f.push(n + ': made due as the line was typed, never run');
        else if (r.first !== null && R[n].ran - R.enter < r.first) f.push(n + ': run ' + (R[n].ran - R.enter) + ' cycles after Enter, before its first letter at ' + r.first);
      }
      // (The denizens' own work held as well, 2026-11-06: a mind deciding, the senses answering its ask, their intents carried
      // out, none from the first key the UI has till HELD cycles on, the long line typed slowly, nor from inventory's first key
      // to its first letter; and some of it run as the long line's keys went on past LIMIT, three seconds the most it's held)
      const runs = this.runs, inn = (k, a, b) => runs[k].filter(c => c >= a && c < b).length, end = R.enter + r.first;
      let ran = 0;
      for (const k of Object.keys(runs)) {
        const slow = inn(k, R.seenl, R.seenl + HELD), again = inn(k, R.seenr, R.seenr + HELD), fast = inn(k, R.seeni, end);
        ran += inn(k, R.seenl + LIMIT, R.letgo1);
        notes.push(k + ': ' + runs[k].length + ' runs; the long line\'s keys, its first ' + HELD + ' cycles ' + slow + ', after ' + LIMIT + ' ' + inn(k, R.seenl + LIMIT, R.letgo1) + '; typed again from its "room" as the denizens are about, its first ' + HELD + ' cycles ' + again + '; inventory\'s first key to its first letter ' + fast + ', after it ' + inn(k, end, Infinity));
        if (!R.seenl || slow) f.push(k + ': ' + slow + ' run(s) in the long line\'s first ' + HELD + ' cycles of keys: not held back');
        if (!R.seenr || again) f.push(k + ': ' + again + ' run(s) in the first ' + HELD + ' cycles of the line typed again: not held back');
        if (!R.seeni || r.first === null || fast) f.push(k + ': ' + fast + ' run(s) from inventory\'s first key to its first letter: not held back');
      }
      if (!ran) f.push('the denizens\' work: none run as the long line\'s keys went on past ' + LIMIT + ' cycles: starved');
      return f;
    },
    sdcard: true
  };
}

module.exports = {
  tests: [
    {
      name: 'dq0hunger', asks: true, near: ['time'], what: 'Dingus Quest Zero\'s hunger and thirst (slice 4.9: HOUSE.md\'s water, hunger and thirst, the user\'s of 2026-10-21, DECISIONS.md\'s; the referee\'s, rswant.h), by the words told: at seed 3 in the kitchen, nothing near, its torch put out: "eat the rations" at once refused, not hungry; "wait 9 hours", the want\'s first levels told as they come ("You\'re getting thirsty.", "You\'re getting hungry.") and the status line\'s "thirsty"; over to the kitchen\'s well first ("look into the well"), its torch lit again after the wait, "drink" its fill there; "eat the rations" a meal (a day\'s ration begun, five minutes), again refused; "fill the waterskin" at the well (the old kit\'s, empty); its torch put out, "wait 17 hours", "You\'re very thirsty."; "drink from the waterskin" its pints',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm', 'eat the rations', 'look into the well', 'douse the torch', 'wait 9 hours', 'light the torch', 'drink', 'eat the rations',
        'eat the rations', 'fill the waterskin', 'douse the torch', 'wait 17 hours', 'drink from the waterskin', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        const at = (line, n = 0) => { let i = -1; for (let k = 0; k <= n; k++) i = o.indexOf('> ' + line + ' > ' + line + ' ', i + 1); return i; };   // (A line typed, echoed)
        const after = (line, words, n = 0) => { const i = at(line, n); return i >= 0 && o.slice(i, i + 2 * line.length + 300).includes(words); };
        if (!after('eat the rations', 'You\'re not hungry.')) f.push('the first "eat the rations" not refused, not hungry');
        const w8 = at('wait 9 hours'), ap = at('light the torch'), w8s = w8 >= 0 && ap > w8 ? o.slice(w8, ap) : '';
        if (!w8s.includes('You\'re getting thirsty.')) f.push('eight hours waited, not told "You\'re getting thirsty."');
        if (!w8s.includes('You\'re getting hungry.')) f.push('eight hours waited, not told "You\'re getting hungry."');
        if (o.slice(0, w8).includes('You\'re getting')) f.push('hunger or thirst told before eight hours');
        if (!/thirsty/.test(w8s.replace(/You're getting thirsty\./g, ''))) f.push('no "thirsty" on the status line');
        if (!after('drink', 'You drink your fill.')) f.push('"drink" by the well not its fill');
        if (!after('eat the rations', 'You eat a meal.', 1)) f.push('"eat the rations" after eight hours not a meal');
        if (!after('eat the rations', 'You\'re not hungry.', 2)) f.push('a meal eaten, "eat the rations" again not refused');
        if (!after('fill the waterskin', 'You fill your waterskin.')) f.push('"fill the waterskin" at the well not filled');
        const w16 = at('wait 17 hours'), ds = at('drink from the waterskin');
        if (!(w16 >= 0 && ds > w16 && o.slice(w16, ds).includes('You\'re very thirsty.'))) f.push('sixteen hours more, not told "You\'re very thirsty."');
        if (!after('drink from the waterskin', 'You drink from your waterskin.')) f.push('"drink from the waterskin" not drunk from it');
        if (f.length) this.notes = [o.slice(Math.max(0, w8 - 200), w8 + 2500)];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0longwait', asks: true, what: 'Dingus Quest Zero\'s long waits (the user\'s, 2026-10-28: "waits should be able to last a full night\'s sleep, so 8 hours at least.  Some spells may take longer than that to cast, so 16-bit minutes would be better."; DECISIONS.md\'s, HOUSE.md\'s long waits), by the world\'s events and the clock: at seed 3 in the kitchen (nothing near), its torch put out, "wait 8 hours" waited whole (its intent in minutes, I_WAIT\'s WA_MIN: the clock 288,000 tenths on, the long task 480 minutes, mins), told "You wait eight hours.", the status after; "wait 3 hours 20 minutes" (200 minutes, its spans added up) and "wait 3 hours and 20 minutes" (one duration, not two commands) told so; "rest" a night\'s sleep (LT_SLEEP, "You settle down to rest for eight hours."), "sleep 8 hours" the same; each in the cycles a bearable wait takes (the coarser ticks while nothing\'s near: ten-minute slices); at the normal pace the same wait as quick (run as the turns pace runs it); at seed 21 below, by the giant rats, a long wait asked about at a sound ("Keep waiting?") and answered no: broken off, hours short of its eight; and with the hand-written theme\'s hostile rats a sleep broken off hard by a bite (dq0tasks\' long tasks)',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm', 'douse the torch', 'wait 8 hours', 'status', 'wait 3 hours 20 minutes', 'wait 3 hours and 20 minutes', 'rest', 'sleep 8 hours', 'quit',
        'dq0 -p n -s 3 -c /ram/c.hm', 'douse the torch', 'wait 8 hours', 'quit',
        'dq0 -p t -s 21 -c /ram/c.hm -T', 'd', 'wait 8 hours', 'no', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'sleep 8 hours', 'quit', 'echo $status'] })) },
      // (The bitten sleep, felled1's, 2026-11-21: a sleeping character struck as a sleeper is, no defence (RS_ASLEEP, one
      // path), and a felled one's rolls a round each; its pin, -R 1, dropped with dice2's keyed dice, 2026-11-24)
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out);
        const games = [], g = [];
        for (const t of T) { if (/^dq0 /.test(t.line)) games.push(g.splice(0)); g.push(t); }
        games.push(g.splice(0)); games.shift();
        if (games.length !== 4) return ['the four games not played: ' + games.length];
        const of = (gm, line, n = 0) => gm.filter(t => t.line === line)[n] || { k: -1, line, events: [], dens: [], at: null, after: null };
        const took = t => t.at && t.after ? t.after.now - t.at.now : -1, cyc = t => t.at && t.after ? t.after.cyc - t.at.cyc : Infinity;
        const time = t => dq0Ev(t, 'E_TIME', {})[0];
        // (Waited whole, nothing near: its time in minutes, told, the clock on; bearable)
        const [a, b, c, d] = games;
        const w8 = of(a, 'wait 8 hours'), e8 = time(w8);
        if (took(w8) !== 288000) f.push('"wait 8 hours" not eight hours waited: ' + took(w8) + ' tenths');
        if (!e8 || e8.w1 !== 480 || !e8.mins) f.push('"wait 8 hours" not its 480 minutes, in minutes (E_TIME\'s WA_MIN): ' + JSON.stringify(e8));
        const i8 = (w8.intents || []).find(e => e.kind === 'I_WAIT');   // (The UI's intent, by its payload's names: msgs.def's)
        if (!i8 || i8.ring !== 'ui' || i8.tenths !== 480 || i8.how !== 1 || i8.task !== 'LT_NONE' || i8.quiet) f.push('"wait 8 hours" not an I_WAIT of 480 minutes (WA_MIN): ' + JSON.stringify(i8));
        if (!w8.after || w8.after.pc.task.len !== 480 || !w8.after.pc.task.mins) f.push('the long task not 480 minutes, in minutes: ' + JSON.stringify(w8.after && w8.after.pc.task));
        if (!/> wait 8 hours You wait eight hours\./.test(o)) f.push('"wait 8 hours" not told "You wait eight hours."');
        // (Each wait's cycles a trend, 2026-11-17: tests/lib/trends.js, a rise of more than 5% said, not failed; 40M at
        // most before, 60M at the normal pace)
        const timed = (name, t) => { if (!isFinite(cyc(t))) f.push(name + ': not timed'); const n = trend(this, name, cyc(t)); if (n) (this.notes = this.notes || []).push(n); };
        timed('wait 8 hours', w8);
        const st = of(a, 'status');
        if (!st.at || st.at.now < 288000) f.push('the clock not on eight hours at the status after: ' + (st.at && st.at.now));
        if (!/> status Pace: [^>]*Facing: south\./.test(o)) f.push('no status after the wait');
        for (const line of ['wait 3 hours 20 minutes', 'wait 3 hours and 20 minutes']) {
          const t = of(a, line), words = 'You wait three hours and twenty minutes.';
          if (took(t) !== 120000) f.push('"' + line + '" not 200 minutes: ' + took(t) + ' tenths');
          // (The task's bar, the status line's, may come before the words: typed as the status answer ended, the
          // line's drawn again as it's taken up, 2026-10-29)
          if (!o.replace(/\[[.#]{10}\] /g, '').includes('> ' + line + ' ' + words)) f.push('"' + line + '" not told "' + words + '"');
        }
        if (/I don't know how to minutes/.test(o)) f.push('"and 20 minutes" taken for a command of its own');
        for (const [line, words] of [['rest', 'You settle down to rest for eight hours.'], ['sleep 8 hours', 'You settle down to sleep for eight hours.']]) {
          const t = of(a, line), e = time(t);
          if (took(t) !== 288000) f.push('"' + line + '" not a night\'s sleep: ' + took(t) + ' tenths');
          if (!e || !e.asleep) f.push('"' + line + '" not asleep (E_TIME\'s WA_SLEEP): ' + JSON.stringify(e));
          if (!o.includes('> ' + line + ' ' + words)) f.push('"' + line + '" not told "' + words + '"');
          timed(line, t);
        }
        if (!m.dq0.games[0].tasks.some(k => k.kind === 'LT_SLEEP' && k.len === 480)) f.push('no sleep\'s long task, LT_SLEEP, 480 minutes');
        // (At the normal pace: the long wait as quick, the turns pace's way)
        const n8 = of(b, 'wait 8 hours');
        if (took(n8) < 288000 || took(n8) > 288600) f.push('the normal pace\'s "wait 8 hours" not eight hours: ' + took(n8) + ' tenths (its real clock a few more at most)');
        timed('the normal pace: wait 8 hours', n8);
        // (Broken off: asked at a sound, "no"; and stopped hard by a bite)
        const all = gm => gm.flatMap(t => t.events);
        if (!all(c).some(e => e.kind === 'E_TASK' && e.te === 'TE_ASK_HEARD' && e.task === 'LT_WAIT')) f.push('the long wait not asked about at a sound');
        if (!all(c).some(e => e.kind === 'E_TASK' && e.te === 'TE_STOPPED' && e.task === 'LT_WAIT')) f.push('the long wait not stopped at no');
        const cw = of(c, 'wait 8 hours'), cn = of(c, 'no'), cend = cn.after ? cn.after.now : -1;
        if (!cw.at || cend < 0 || cend - cw.at.now >= 288000) f.push('the broken-off wait not short of its eight hours: ' + (cend - (cw.at ? cw.at.now : 0)));
        if (!all(d).some(e => e.kind === 'E_TASK' && e.te === 'TE_HIT' && e.task === 'LT_SLEEP')) f.push('the sleep not stopped hard by a bite: ' + all(d).filter(e => e.kind === 'E_TASK').map(e => e.te + ':' + e.task).join(', '));
        const ds = of(d, 'sleep 8 hours');
        if (!ds.at || !ds.after || ds.after.now - ds.at.now >= 288000) f.push('the bitten sleep not short of its eight hours');
        this.notes = ['the cycles of the long waits: eight hours nothing near ' + (cyc(w8) / 1e6).toFixed(1) + 'M, rest ' + (cyc(of(a, 'rest')) / 1e6).toFixed(1) + 'M, at the normal pace ' + (cyc(n8) / 1e6).toFixed(1) + 'M', ...(this.notes || [])];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tasks', asks: true, what: 'Dingus Quest Zero\'s long tasks (slice 4.0, HOUSE.md\'s: the world\'s, the hub\'s pc_task; the user\'s choices: a bar and words, hard stops, soft asks, all wounds hard), by the world\'s events: at seed 3 "stop" with nothing to stop, then a walk east down a passage a square at a time, its beats as it goes (it turns, a quiet stretch) and its bar on the status line; at seed 3 by the way down a dig begun, "stop walking" refused (it\'s a dig), "stop digging" stops it; at seed 21 below, a long wait asked about at a sound heard ("Keep waiting?": the UI\'s, I_ASK), answered yes (it goes on), no (it stops), and with a command (it stops, and the command\'s carried out); and with the hand-written theme\'s hostile rats (inhabit.txt) a long wait stopped hard by a bite',
      init: 't_rc', cycles: 3500e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm', 'stop', 'go through the door to the east', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', 'dig down', '!stop walking', '!stop digging', 'quit',
        ...['yes', 'no', 'look'].flatMap(ans => ['dq0 -p t -s 21 -c /ram/c.hm -T', 'd', 'wait 30 minutes', ans, 'quit']),
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'wait 10 minutes', 'quit', 'echo done']
        .map(l => (l[0] === '!' ? l.slice(1) : 'ā' + l) + '\r').join('') },
      expect: ['% echo done'],
      check(m, out) {
        const T = dq0Turns(m), f = [], ev = (t, k, want) => dq0Ev(t, k, want);
        const games = [], g = [];
        for (const t of T) { if (/^dq0 /.test(t.line)) games.push(g.length ? g.splice(0) : []); g.push(t); }
        games.push(g.splice(0)); games.shift();
        if (games.length !== 6) return ['the six games not played: ' + games.length];
        const all = gm => gm.flatMap(t => t.events), has = (gm, want, what) => { if (!all(gm).some(e => e.kind === 'E_TASK' && Object.entries(want).every(([k, v]) => e[k] === v))) f.push(what + ' (' + all(gm).filter(e => e.kind === 'E_TASK').map(e => e.te + ':' + e.task).join(', ') + ')'); };
        // (A walk: stop with none, then east down the passage, its beats and its bar)
        const [w] = [games[0]], stop = w.find(t => t.line === 'stop'), east = w.find(t => t.line === 'go through the door to the east');
        if (!ev(stop, 'E_TASK', { te: 'TE_IDLE' }).length) f.push('"stop" with nothing to stop not told so');
        const beats = ev(east, 'E_BEAT', {});
        if (!beats.some(e => e.beat === 'WB_TURN') || !beats.some(e => e.beat === 'WB_QUIET')) f.push('the walk east not told as it went: ' + beats.map(e => e.beat + ':' + e.side).join(', '));
        if (!ev(east, 'E_ARRIVE', {}).length) f.push('the walk east not ended');
        if (!/\[[#.]+\]/.test(out) || !out.includes('#')) f.push('no long task\'s bar on the status line');   // (Empty as it begins, now its first words are told at once; its #s drawn after it, each as it's done)
        // (A dig: a stop that names another task refused, then the dig stopped)
        has(games[1], { te: 'TE_NOT', task: 'LT_DIG', named: 'LT_WALK' }, '"stop walking" not refused as the dig went on');
        has(games[1], { te: 'TE_STOPPED', task: 'LT_DIG' }, '"stop digging" not the dig stopped');
        // (A long wait at a sound heard: asked, answered yes, no, and with a command)
        for (const [k, ans] of [[2, 'yes'], [3, 'no'], [4, 'look']]) {
          has(games[k], { te: 'TE_ASK_HEARD', task: 'LT_WAIT' }, 'the wait (' + ans + ') not asked about at a sound');
          if (ans === 'yes') has(games[k], { te: 'TE_ON', task: 'LT_WAIT' }, 'the wait not gone on at yes');
          else has(games[k], { te: 'TE_STOPPED', task: 'LT_WAIT' }, 'the wait not stopped at ' + ans);
        }
        if (!/> look[\s\S]{0,40}\n?[\s\S]*?You stop waiting\.[\s\S]*?You can see part of a cave/.test(dq0Story(out).replace(/> /g, '\n> '))) f.push('the look typed for an answer not carried out after the wait stopped');
        // (Hostile rats: the wait stopped hard by a bite)
        if (!all(games[5]).length || !games[5].flatMap(t => t.dens).some(e => e.kind === 'E_FIGHT' && e.def === 0)) f.push('no rat bit at the character as it waited');
        has(games[5], { te: 'TE_HIT', task: 'LT_WAIT' }, 'the wait not stopped hard by the bite');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0skykin', near: ['time', 'senses'], what: "Dingus Quest Zero's sky's kin (2026-11-03: HOUSE.md's what shows the sky, daylight falling in, kin; dq0sky's the rest), by the senses' replies, the words and the chunks, every room's skylight and window and a shaft the environment's dq0sky=1: a denizen's sight in daylight: a cave adder (sees 30 feet by light, 10 with none, hears 15; held where it is, dq0Poke's) 30 feet from the character, its torch out, by a skylight's daylight at noon: seen (dsense's SN_SEEN); the same at ten at night: not; the drip under a shaft: seed 2's shaft's square, nothing but water trickling heard, and the same square with no sky (dq0sky=0), no water; a save and a load: quit and carried on (dq0 -d) under the shaft, the same words, daylight falling, its chunk the same as it was kept; and a chunk read out of the cache and read in again (the character down the ladder, the chunk's slot let go, as a full cache's evicting it; dq0pure's kind): the same bytes, the shaft and its daylight as before",
      init: 't_rc', cycles: 3100e6,
      machine: { input: (() => {
        const adder = d => ['dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/' + d + ' -S /ram/s.sc', 'douse the torch', ...Array(6).fill('wait 1 minute'), 'look', 'quit'];
        return dq0Typed(dq0Game({ lines: ['mkdir /ram/g', "echo 'faction | Serpents | adder | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'mkdir /ram/n', 'cp /ram/g/theme.txt /ram/n/theme.txt', 'echo sky 1 >> /ram/s.sc', 'echo hour 12 >> /ram/s.sc',
          ...adder('g'), 'echo hour 22 >> /ram/s.sc', ...adder('n'), 'echo hour 12 >> /ram/s.sc', 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/s -S /ram/s.sc', 'go to the shaft', 'listen', 'look', 'quit', 'dq0 -d /ram/s -S /ram/s.sc', 'look', 'quit',
          'dq0 -p t -s 2 -c /ram/c.hm -T -S /ram/s.sc', 'd', 'u', 'look', 'go to the shaft', 'look', 'quit', 'echo sky 0 >> /ram/s.sc', 'dq0 -p t -s 2 -c /ram/c.hm -T -S /ram/s.sc', 'look', 'listen', 'quit', 'echo $status'] }));
      })() },
      start(m) {
        const scene = (h, L, K) => {
          const at = k => L.actor + k * L.actorSize, R = dq0Rec(m), w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8; };
          w16(L.pcX, 32706);                    // (In the room up the passage, by its skylight at 32709, 32762)
          w16(L.pcY, 32762);
          let n = 0;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            const o = R.at(h[at(k) + L.aDen]), [x, y] = n ? [32708 + n, 32768] : [32712, 32762];
            m.watched = n ? m.watched : k;
            ++n;
            w16(at(k) + L.aX, x); w16(at(k) + L.aY, y);
            R.mem[o + R.X] = x & 255; R.mem[o + R.X + 1] = x >> 8; R.mem[o + R.Y] = y & 255; R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_WARY;
            h[at(k) + L.aDoing] = K.DO_WATCH;
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        const p = [], shaft = () => { const S = stratumRec(m, 0), l = S.links.find(i => i.kind === 5 && i.dug === 2); return { x: l.x, y: l.y, key: ((l.x - S.x0) >> 5) + ',' + ((l.y - S.y0) >> 5) }; };
        for (const g of [1, 2]) { p.push({ g, line: 'douse the torch', n: 1, call: scene }); for (let n = 1; n <= 6; n++) p.push({ g, line: 'wait 1 minute', n, call: scene }); }
        p.push({ g: 3, line: 'quit', n: 1, call: () => { m.kept = liveChunks(m, 0)[shaft().key]; } }, { g: 4, line: 'quit', n: 1, call: () => { m.back = liveChunks(m, 0)[shaft().key]; } });
        p.push({ g: 5, line: 'u', n: 1, call: () => {       // (On the stratum below: its chunk's slot let go)
          const w = m.dq0.bank(WORLD_BANK_N), key = shaft().key;
          m.was = liveChunks(m, 0)[key];
          for (let c = 0; c < 24; c++) { const sl = CSLOT_AT + c * 8; if (w[sl] && w[sl + 1] === 0 && (w[sl + 2] | w[sl + 3] << 8) + ',' + (w[sl + 4] | w[sl + 5] << 8) === key) { w[sl] = 0; m.evicted = true; } }
        } }, { g: 5, line: 'quit', n: 1, call: () => { m.now = liveChunks(m, 0)[shaft().key]; } });
        p.push({ g: 5, line: 'look', n: 1, call: () => { m.after = liveChunks(m, 0)[shaft().key]; } });
        p.push({ g: 6, line: 'look', n: 1, call: (h, L) => { const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8; }; w16(L.pcX, m.sq.x); w16(L.pcY, m.sq.y); } });
        p.push({ g: 3, line: 'listen', n: 1, call: (h, L) => { m.sq = { x: h[L.pcX] | h[L.pcX + 1] << 8, y: h[L.pcY] | h[L.pcY + 1] << 8 }; } });
        dq0Scene(m, p);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], gs = m.dq0.games, o = dq0Prose(out), G = o.split('% dq0 ').slice(1), SN_SEEN = 1;
        if (gs.length !== 6 || G.length !== 6) return ['not six games: ' + gs.length];
        const seen = g => gs[g].replies.filter(r => r.req === 'dsense' && r.who === m.watched && !r.noise && r.x === 32712 && r.y === 32762 && r.pc.x === 32706 && r.pc.y === 32762);
        const d = seen(0), n = seen(1);
        this.notes = ['day ' + JSON.stringify(d.map(r => r.res)) + ', night ' + JSON.stringify(n.map(r => r.res))];
        if (!d.length || !d.every(r => r.res & SN_SEEN)) f.push("the adder at 30 feet not seeing it by the skylight's daylight: " + JSON.stringify(d.map(r => r.res)));
        if (!n.length || n.some(r => r.res & SN_SEEN)) f.push('the adder at 30 feet seeing it by night, unlit: ' + JSON.stringify(n.map(r => r.res)));
        if (!/You hear nothing but water trickling somewhere near\./.test(G[2])) f.push('no drip under the shaft');
        if (/trickling/.test(G[5])) f.push('water heard there with no sky');
        const say = /You stand under a shaft open to the sky, daylight falling from it\./;
        if (!say.test(G[2]) || !say.test(G[3])) f.push('carried on, the shaft and its daylight not told');
        if (G.some(g => g && say.test(g) && /Above, the dark hides the ceiling/.test(g.slice(Math.max(0, g.search(say) - 300), g.search(say) + 300)))) f.push('the dark hiding the ceiling, by a shaft\'s daylight (2026-11-04)');
        const zero = (a, b, w) => { if (!a || !b) return f.push(w + ': a chunk not read'); const c = chunkDiff(a, b); if (c.portal + c.trap + c.square + c.opening + c.other.length) f.push(w + ': ' + JSON.stringify(c)); };
        zero(m.kept, m.back, 'quit and carried on, the chunk not as it was kept');
        if (!m.evicted) f.push('its chunk not in the cache to let go');
        zero(m.was, m.after, 'let go and read in again, the chunk not the same');
        if (!say.test(G[4])) f.push('read in again, the shaft and its daylight not told');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0gamehour', hours: true, near: ['time'], what: "Dingus Quest Zero's hour by the game (the user's, 2026-11-03: \"vary by game\": HOUSE.md's the hour): seeds 5 and 14 walked into, the world's and the denizens' hour each the seed modulo 24, not dusk; dq0 -E dusk, 18; the environment's dq0hour above both",
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 5 -c /ram/c.hm -T', 'inventory', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -T', 'inventory', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -T -E', 'inventory', 'quit', 'echo hour 3 >> /ram/s.sc', 'dq0 -p t -s 14 -c /ram/c.hm -T -S /ram/s.sc', 'inventory', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], w = T.filter(t => t.line === 'inventory').map(t => t.after && t.after.pc && t.after.pc.tb && t.after.pc.tb.hour0);
        this.notes = ['hours ' + w.join(', ')];
        if (w.join() !== '5,14,18,3') f.push('not the seeds\' hours, dusk, the environment\'s: ' + w.join());
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0turnsame', near: ['time', 'fight'], meaning: true, what: 'Dingus Quest Zero at the turns pace the same game whatever the tasks\' timing (the turns pace: a game turns on its seed and its commands alone): dq0cmd\'s fight at seed 3 (-T: three giant rats, a dagger thrown at one, then blows) played twice, the second time its commands typed late (its first some 6M cycles after the prompt, the rest 2M), every event of the world\'s and the denizens\' the same, in the same order, at the same game times; 2026-10-31: a throw\'s two intents (taken in hand, then thrown) let the world\'s clock and the denizens\' turn in between them as the tasks\' timing fell (ui_giving)',
      init: 't_rc', cycles: 1500e6,
      get machine() {
        const g = 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', cmds = ['target the rat', '1', 'throw the dagger hard at the closest rat', 'attack', 'attack', 'wait', 'quit'];
        return { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', g, ...cmds].map(l => '\u0101' + l + '\r').join('') +
          '\u0101' + g + '\r' + cmds.map((l, i) => '\u0101' + (i ? '\u0100' : '\u0100\u0100\u0100') + l + '\r').join('') + '\u0101echo $status\r' };
      },
      expect: ['% echo $status'],
      check(m) {
        const g = m.dq0.games, f = [];
        if (g.length !== 2) return ['games played: ' + g.length + ', not 2'];
        const ev = k => g[k].events.concat(g[k].dens, g[k].replies).map(e => JSON.stringify(Object.assign({}, e, { line: undefined })));
        const a = ev(0), b = ev(1);
        let d = 0;
        while (d < a.length && d < b.length && a[d] === b[d]) d++;
        if (d < a.length || d < b.length) f.push('the late game not the same, from its event ' + d + ' (of ' + a.length + ' and ' + b.length + '): ' + (a[d] || 'none') + ' / ' + (b[d] || 'none'));
        if (a.length < 20) f.push('too little happened: ' + a.length + ' records');
        this.notes = [a.length + ' and ' + b.length + ' records (events, the denizens\' events, the replies)' + (f.length ? '' : ', the same in both')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0starttime', near: ['time'], outfit: true, what: 'Dingus Quest Zero\'s start timed at the normal pace (the user\'s: a start of some seconds is fine with its progress shown; the start agent\'s figures, 2026-10-30: on the board, at 3.58 MHz, a minute and more, the SD card\'s reading two thirds of it), from Enter on the dq0 line to the first prompt: seed 1 (a stand-in\'s theme) and seed 123456789 (its own history made by themegen, as a game whose seed is the clock\'s), each within its budget; the progress shown all the way (no silence on the screen longer than about 10 s at 3.58 MHz); and the help\'s line said with the first prompt, after the first look; and each task\'s overlays in its banks as ld65 made them (read packed from the card and unpacked in place: the unpack library)',
      init: 't_rc', cycles: 1500e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p n -s 1 -c /ram/c.hm', 'quit',
        'dq0 -p n -s 123456789 -c /ram/c.hm', 'quit', 'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['Dingus Quest Zero is starting', 'The place\'s history is being made', 'The dungeon is being made ready', 'Nearly ready', '% echo $status'],
      // (The budgets (the lead's, 2026-10-30): the start's cycles as measured then, 282M and 372M, with a tenth more;
      // 252M and 338M since the overlays' files are packed, each bank read straight into its bank, dq0.ovl opened once;
      // 229M and 311M with the storage driver's quicker block reads; 208M and 289M with the overlays packed (LZ4's
      // block, the unpack library's: modules/unpack, 2026-10-31), so 229M and 318M, a tenth more; 232M for seed 1 since the tunnellers' work and builders, 2026-11-02: some 3K of packed code more, 270 cycles a byte off the card and 40 unpacking; 234M since the character's (slice 4.4): some 3.5K more, the catalogue, dq0d.dat and a theme read straight into their banks giving back 2.5M.  The silence's:
      // 36M cycles, 10 s at 3.58 MHz; 25M now, the longest (the UI's overlays read, before the screen's made).  The
      // help's line: after the first look's words, only the status line's and the prompt after it.  And every task's
      // overlays in their banks at the first prompt as ld65 made them (dq0OvlBanks))
      starts: [{ seed: 1 }, { seed: 123456789 }], quiet: 36e6,   // (2026-11-17, the user's: the starts' budgets, 245M and 325M, trends: tests/lib/trends.js, a rise of more than 5% said, not failed; the silence's 36M still a failure)  // (Before: 2026-11-11, the user's: raised from 235M and 315M, the start at 234.1M and 311.0M after the UI's room pass and narration's stage 1, a bank more to come; 2026-11-03: dq0w's new overlay, and two kinds more, to load: 241.3M, 321.0M, -K's; with the outfits, as a game's played, 249.7M and 332.7M; then the margins' pass, DUNGEON.md's: 219.7M, 298.8M; 2026-11-07, the designators merged: 224.7M, 301.9M; the stairs looked at, way.c's opened: 225.3M, 302.5M, seed 1's budget 226M for review; then the user, 2026-11-07: "Don't worry too much about the startup time": 235M and 315M, room for the work to come, 225.8M and some 302M then)    // (2026-11-11, the user's: raised from 235M and 315M, the start at 234.1M and 311.0M after the UI's room pass and narration's stage 1, a bank more to come; 2026-11-03: dq0w's new overlay, and two kinds more, to load: 241.3M, 321.0M, -K's; with the outfits, as a game's played, 249.7M and 332.7M; then the margins' pass, DUNGEON.md's: 219.7M, 298.8M; 2026-11-07, the designators merged: 224.7M, 301.9M; the stairs looked at, way.c's opened: 225.3M, 302.5M, seed 1's budget 226M for review; then the user, 2026-11-07: "Don't worry too much about the startup time": 235M and 315M, room for the work to come, 225.8M and some 302M then)
      start(m) {
        const a = m.acia, read = a.read, shown = a.shown, self = this;
        let typed = '', g = null, tail = '', esc = 0, text = '';
        this.games = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (/^dq0 /.test(typed)) { g = { line: typed, t0: m.cpu.cyc, last: m.cpu.cyc, quiet: 0, quietAt: 0, total: 0, text: '' }; self.games.push(g); tail = ''; text = ''; }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
        a.shown = (v, t) => {
          shown(v, t);
          if (!g || g.total) return;
          const now = m.cpu.cyc;
          if (now - g.last > g.quiet) { g.quiet = now - g.last; g.quietAt = g.last - g.t0; }
          g.last = now;
          if (esc === 1) { esc = v === 0x5B ? 2 : 0; return; }
          if (esc === 2) { if (v >= 0x40 && v <= 0x7E) esc = 0; return; }
          if (v === 0x1B) { esc = 1; return; }
          if (v >= 32 || v === 10) text += String.fromCharCode(v);
          tail = (tail + String.fromCharCode(v)).slice(-2);
          if (tail === '> ') { g.total = now - g.t0; g.text = text; if (self.games.length === 1) self.banks = dq0OvlBanks(m); }
        };
      },
      check(m) {
        const f = [], M = n => (n / 1e6).toFixed(1) + 'M', rose = [];
        this.starts.forEach((s, i) => {
          const g = this.games[i];
          if (!g || !g.total) return f.push('seed ' + s.seed + ': no first prompt');
          const tn = trend(this, 'seed ' + s.seed + ': the start', g.total);
          if (tn) rose.push(tn);
          if (g.quiet > this.quiet) f.push('seed ' + s.seed + ': ' + M(g.quiet) + ' cycles with nothing on the screen, ' + M(g.quietAt) + ' after Enter (' + M(this.quiet) + ' at most)');
          const w = g.text.replace(/\s+/g, ' '), h = w.lastIndexOf('Type help for the commands.'), look = w.lastIndexOf('You ');
          if (h < 0 || look < 0 || h < look || w.length - h > 80) f.push('seed ' + s.seed + ': the help\'s line not with the first prompt, after the first look: ' + JSON.stringify(w.slice(-300)));
        });
        f.push(...(this.banks || ['the overlays\' banks not looked at']).filter(x => !/^ok/.test(x)));
        this.notes = (this.banks || []).filter(x => /^ok/.test(x)).concat(this.games.map((g, i) => 'seed ' + (this.starts[i] || {}).seed + ': ' + M(g.total) + ' cycles to the first prompt (' + (g.total / 3579545).toFixed(1) + ' s at 3.58 MHz); the longest silence ' + M(g.quiet) + ', ' + M(g.quietAt) + ' after Enter')).concat(rose);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0startwork', near: ['time'], what: 'Dingus Quest Zero\'s start\'s own work done before its first prompt (the user\'s, 2026-11-20: "The start\'s work finishes before the first prompt, so the first command is answered on time; the start takes a little longer"; built/2026-11-20-start1.md), at seed 5 by its way down (-T: dq0fightcmd\'s, whose "d" waited on it) at the turns pace and as played, and at seed 3 at the normal pace: as the first command\'s typed at the first prompt, no chunk the maker stocked is left with its things not put down (the world bank\'s cslot table: none CS_FRESH), and at the real paces the lights\' first minute has been reckoned (the hub\'s pc_left, the light\'s words on the status line, said; at the turns pace it\'s the first command\'s pass\'s, its test\'s dice where they always were)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 5 -c /ram/c.hm -k sword -T', 'quit',
        DQ0_PLAYED + 'dq0 -p n -s 5 -c /ram/c.hm -k sword -T', 'quit', 'dq0 -p n -s 3 -c /ram/c.hm -T', 'quit', 'echo $status'] })) },
      start(m) {
        const H = require(path.join(TESTS, '..', 'games', 'dq0', 'hubgen.js')).load(), CSLOT = H.off('wrec', 'cslot'), CSZ = H.size('cslot'),
          LEFT = H.off('hub', 'pc_left'), a = m.acia, read = a.read, self = this;
        let typed = '', game = '';
        this.seen = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (/^dq0 /.test(typed)) game = typed;
              else if (typed === 'quit' && game) {
                const h = m.dq0 && m.dq0.bank(0), w = m.dq0 && m.dq0.bank(WORLD_BANK_N), fresh = [];
                let left = '';
                for (let i = 0; h && h[LEFT + i] && i < 16; i++) left += String.fromCharCode(h[LEFT + i]);
                for (let k = 0; w && k < H.C.NCSLOTS; k++) if (w[CSLOT + k * CSZ] === H.C.CS_FRESH) fresh.push(k);
                self.seen.push({ game, fresh, left, turns: / -p t/.test(game), ok: !!(h && w) });
                game = '';
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m) {
        const f = [];
        if (this.seen.length !== 3) f.push('the first prompt met ' + this.seen.length + ' times, not 3');
        for (const s of this.seen) {
          if (!s.ok) f.push(s.game + ': the hub not there at the first command');
          if (s.fresh.length) f.push(s.game + ': at the first command, chunk slot(s) ' + s.fresh.join(', ') + ' stocked, their things not put down');
          if (!s.turns && !s.left) f.push(s.game + ': at the first command, the lights\' first minute not reckoned (pc_left empty)');
        }
        this.notes = this.seen.map(s => s.game + ': the light "' + s.left + '", fresh slots ' + (s.fresh.length ? s.fresh.join(', ') : 'none'));
        return f;
      },
      sdcard: true
    },
    { name: 'dq0jobs', ...dq0JobsTest() },
    { name: 'dq0hold', ...dq0HoldTest() },
    {
      // (Its third game's kit a chalk and a candle more (-k lockpicks,chalk,candle), ans1's, 2026-11-22: a found chest locked one
      // time in three by its number, every thing's one more since a flask of oil's oil is a record of its own (H16): two more
      // things, the chest's number one that locks again, the picking's scene as it was)
      name: 'dq0timed', meaning: true, what: 'Dingus Quest Zero\'s timed work (the user\'s playtest, 2026-10-22: "It would be better if the first half (for example "You study the mechanism a while") comes out first, followed by, as the task timer expires, the second part"; HOUSE.md\'s long tasks): whatever spends time and whose outcome is a test a long task of its own, its first words at once (E_TASK TE_BEGUN, the clock where the command found it), its test made and its outcome told as its time\'s done (the clock its time on), stopped first, nothing: at seed 7 a thief (Lockcraft 95) finds a dart trap on a door (three searches), sets to making out its mechanism and is stopped at once (TE_STOPPED, no outcome: nothing made out), makes it out (LT_STUDY, TO_ASSESSED its minute on), disarms it (LT_DISARM, TO_DISARMED its minute on), the door opened and nothing set off; at seed 16 a door forced (LT_FORCE, its outcome its ten seconds on, the walk up to it with them); at seed 3 a track read (LT_TRACK, the reading, V_TRACED, its minute on), and another stopped at once (none read); at seed 17 a found chest walked over to and its lock picked (the things\' own: a long task of the UI\'s, LT_PICK, a minute, the things asked as it\'s begun and as it\'s done), a dagger hidden (LT_HIDE, a minute, hidden at its end); the words in order, each first half before its second',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['hmchar -q -s 1 -u feudal -c thief -o /ram/t.hm',
        'echo ml lck 95 >> /ram/t.hm', 'dq0 -p t -s 7 -c /ram/t.hm', 'search', 'search', 'search', 'face south', 'disarm'] })) + 'stop\r' +
        dq0Typed(['disarm', 'disarm', 'open', 'quit', 'dq0 -p t -s 16 -c /ram/c.hm -T', 'face east', 'force', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'track',
        'track']) + 'stop tracking\r' + dq0Typed(['quit', 'dq0 -p t -s 17 -c /ram/c.hm -T -k lockpicks,chalk,candle', 'wait', 'wait', 'wait', 'open the chest',
        'pick the lock of the chest', 'hide the dagger', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {   // (The third search finding the dart trap rigged, dice2: dq0traps' own)
        dq0Scene(m, [{ g: 1, line: 'search', n: 3, rig: [{ kind: 'TEST', a: 'P_SEARCH', actor: -1, grade: 'pass' }] }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out), tasks = m.dq0.games.flatMap(g => g.tasks);
        const begun = (t, task) => dq0Ev(t, 'E_TASK', { te: 'TE_BEGUN', task })[0];
        // (A timed work's turn: begun at once, the clock where the command found it; its outcome its time on, then free)
        const timed = (t, task, kind, want, len, what) => {
          const b = begun(t, task), e = dq0Ev(t, kind, want)[0];
          if (!b || !e) return f.push(what + ': ' + (b ? 'no outcome' : 'not begun') + ' (' + dq0Evs(t) + ')');
          if (t.at && b.t !== t.at.now) f.push(what + ': begun at ' + b.t + ', not at once (' + t.at.now + ')');
          if (e.t - b.t < len) f.push(what + ': its outcome ' + (e.t - b.t) + ' tenths after its first words, not ' + len);
          if (t.events.indexOf(b) > t.events.indexOf(e)) f.push(what + ': its outcome before its first words');
          if (!dq0Ev(t, 'E_READY', {}).some(r => r.t >= e.t)) f.push(what + ': not free again after its outcome');
        };
        const g1 = T.of('dq0 -p t -s 7 -c /ram/t.hm'), d0 = T.after(g1, 'disarm'), st = T.after(d0, 'stop'), d1 = T.after(st, 'disarm');
        const d2 = T.after(d1, 'disarm'), op = T.after(d2, 'open'), both = T.and(d0);
        if (!begun(both, 'LT_STUDY')) f.push('the stopped study not begun (' + dq0Evs(both) + ')');
        dq0Need(f, both, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_STUDY' }, 'the study not stopped');
        dq0Never(f, both, 'E_TRAP', {}, 'an outcome of the study stopped first');
        // (The referee's dice its own: HarnMaster's at this seed makes it out and disarms it, the null referee's may not)
        timed(d1, 'LT_STUDY', 'E_TRAP', { to: x => x === 'TO_ASSESSED' || x === 'TO_PUZZLED' }, 600, 'the mechanism studied (nothing made out by the one stopped)');
        const made = dq0Ev(d1, 'E_TRAP', { to: 'TO_ASSESSED' }).length > 0;
        timed(d2, made ? 'LT_DISARM' : 'LT_STUDY', 'E_TRAP', {}, 600, made ? 'the trap disarmed' : 'the mechanism studied again');
        if (dq0Ev(d2, 'E_TRAP', { to: 'TO_DISARMED' }).length) {
          dq0Need(f, op, 'E_PORTAL', { po: 'PO_OPENED' }, 'the door not opened, disarmed');
          dq0Never(f, op, 'E_TRAP', {}, 'the trap went off, disarmed');
        }
        timed(T.after(T.of('dq0 -p t -s 16 -c /ram/c.hm -T'), 'force'), 'LT_FORCE', 'E_PORTAL', { po: p => p === 'PO_FORCED' || p === 'PO_HELD' }, 100, 'the door forced');
        const g3 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T'), t1 = T.after(g3, 'track'), t2 = T.after(t1, 'track'), both2 = T.and(t2);
        timed(t1, 'LT_TRACK', 'E_VIEW', { why: 'V_TRACED' }, 600, 'the track read');
        dq0Need(f, both2, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_TRACK' }, 'the second track not stopped');
        dq0Never(f, both2, 'E_VIEW', { why: 'V_TRACED' }, 'a track read, stopped first');
        const pk = T.of('pick the lock of the chest'), hd = T.of('hide the dagger');
        for (const [t, kind, io, what] of [[pk, 'LT_PICK', 'IO_SHUT', 'the chest\'s lock picked'], [hd, 'LT_HIDE', 'IO_HIDE', 'the dagger hidden']]) {
          if (!tasks.some(e => e.kind === kind && e.len === 600 && e.line === t.k + 1)) f.push(what + ': not a minute\'s long task, ' + kind);
          if (dq0Rep(t, 'ireq', { kind: io }).length < 2) f.push(what + ': the things not asked as it was begun and as it was done');
        }
        const dg = dq0Thing(hd.after, 'dagger');
        if (!dg || !(dg.flags & DQ0_I.HIDDEN)) f.push('the dagger not hidden at its minute\'s end');
        inTurn(o, ['> disarm', 'You study the mechanism.', 'You stop studying the mechanism.', '> disarm', 'You study the mechanism.', '> disarm', '> open',
          '> force', 'You set to forcing the door.', '> track', 'You cast about for tracks.', '> track', 'You cast about for tracks.', 'You stop tracking.',
          '> pick the lock of the chest', 'You set to picking the lock.', 'the lock of the chest.', '> hide the dagger', 'You set to hiding the dagger.',
          'You hide it'], f);
        if (made) inTurn(o, ['You study the mechanism.', 'You stop studying the mechanism.', 'You study the mechanism.',
          'You make out how it works: now to disarm it.', 'You set to disarming the trap.'], f, 'its halves in order: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lowasleep', near: ['time'], what: "Dingus Quest Zero's light burning low not told in its sleep (2026-11-04: a review's fault, HOUSE.md's): a torch with twelve minutes left: awake, 'wait 5 minutes' tells it burning low; the next game, the same torch, 'sleep 5 minutes' doesn't",
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T', 'look', 'wait 5 minutes', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'look', 'sleep 5 minutes', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, fuel: { torch: 12 } }, { g: 2, line: 'look', n: 1, fuel: { torch: 12 } }]);
      },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [], i = o.indexOf('> wait 5 minutes'), j = o.indexOf('> sleep 5 minutes');
        if (i < 0 || j < 0) return ['the lines not found'];
        if (!/burning low/.test(o.slice(i, j))) f.push('awake, the torch not told burning low: ' + o.slice(i, j).slice(0, 400));
        if (/burning low/.test(o.slice(j))) f.push('told burning low asleep: ' + o.slice(j).slice(0, 400));
        return f;
      },
    },
    {
      name: 'dq0restdark', near: ['time', 'gear'], what: "Dingus Quest Zero's rest puts a burning light out first (the user's, 2026-11-17: DECISIONS.md's \"The one-path plan and phase A's questions: the user's answers\", \"A rest puts a burning light out first (for every actor)\"; dq0w.c's rest_dark, the world's as the wait begins asleep, WA_SLEEP; its words E_BEAT's WB_DOUSE), by the words and the item store: at seed 3 (dq0 -T) \"rest\": \"You put out your torch.\" before \"You settle down to rest for eight hours.\", the torch not lit after, the hub's light none; a second game, a lantern lit as well (dq0 -k lantern), \"sleep 1 hour\": both put out first; a third, \"wait 10 minutes\" awake: the torch still lit",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T', 'rest', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -k lantern', 'light the lantern', 'sleep 1 hour', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'wait 10 minutes', 'quit', 'echo $status'] })) },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        const lit = (t, n) => t && t.after ? t.after.items.filter(i => i.name === n && i.where === 'IW_ACTOR' && (i.flags & DQ0_I.LIT)) : [];
        inTurn(o, ['> rest', 'You put out your torch.', 'You settle down to rest for eight hours.'], f, 'the rest: ');   // (The dark told between: L6's look, 2026-11-18)
        const r = T.of('rest');
        if (lit(r, 'torch').length) f.push('the torch still lit after the rest');
        if (!r || !r.after || r.after.pc.light) f.push('the hub\'s light not none after the rest: ' + JSON.stringify(r && r.after && r.after.pc.light));
        const sl = o.slice(o.indexOf('> sleep 1 hour'), o.indexOf('You settle down to sleep', o.indexOf('> sleep 1 hour')));
        if (!/You put out your torch\./.test(sl) || !/You put out your lantern\./.test(sl)) f.push('the sleep: not both lights put out first: ' + sl.slice(0, 300));
        const s = T.of('sleep 1 hour');
        if (lit(s, 'torch').length || lit(s, 'lantern').length) f.push('a light still lit after the sleep');
        const w = T.of('wait 10 minutes');
        if (!lit(w, 'torch').length || /You put out your torch\./.test(o.slice(o.indexOf('> wait 10 minutes')))) f.push('a wait awake: the torch put out');
        return f;
      },
    },
    {
      name: 'dq0restnear', near: ['time', 'gear'], what: "Dingus Quest Zero's rest puts out every burning light within reach (the user's, 2026-11-18: DECISIONS.md's \"The merged branches' questions\", L6: \"A rest puts out every burning light within reach, not only those carried\"; dq0w.c's rest_dark: on its square or beside it, on a thing or in one there, E_BEAT's WB_DOUSE_NEAR), by the words and the item store: at seed 3 (dq0 -T, a lantern more) the lantern lit and set down at its feet: \"sleep 5 minutes\": \"You put out your torch.\" and \"You put out the lantern.\" before \"You settle down to sleep\", neither lit after; a second game the torch dropped lit and the character gone to the room's far side: the torch out of reach, still lit after the sleep, nothing put out",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T -k lantern', 'light the lantern', 'drop the lantern', 'sleep 5 minutes', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'drop the torch', 'go to the far side', 'sleep 5 minutes', 'quit', 'echo $status'] })) },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        const g = o.split('% dq0 -p t -s 3').slice(1);
        if (g.length !== 2) return ['not two games: ' + g.length];
        const sl = g[0].slice(g[0].indexOf('> sleep 5 minutes'), g[0].indexOf('You settle down to sleep'));
        if (!/You put out your torch\./.test(sl) || !/You put out the lantern\./.test(sl)) f.push('the lights carried and at its feet not both put out first: ' + sl.slice(0, 300));
        const [s1, s2] = T.filter(t => t.line === 'sleep 5 minutes'), lit = (t, n) => t && t.after ? t.after.items.filter(i => i.name === n && (i.flags & DQ0_I.LIT)) : [];
        if (lit(s1, 'torch').length || lit(s1, 'lantern').length) f.push('a light within reach still lit after the sleep');
        if (/You put out/.test(g[1])) f.push('a light out of reach put out: ' + g[1].slice(g[1].indexOf('> sleep'), g[1].indexOf('> sleep') + 300));
        if (!lit(s2, 'torch').filter(i => i.where === 'IW_SQUARE').length) f.push('the torch out of reach not still lit');
        return f;
      },
    },
    {
      name: 'dq0passes', near: ['time', 'gear'], what: "Dingus Quest Zero's minute's lighter passes at a real pace (2026-11-04: seen to in the world's main loop, part 27 not copied for them, the swap study's; the same work as when they were part 27's): at the normal pace, idle, not in a long wait: a torch burning in a sconce, its last minute poked, goes out at the minute's look and the sconce's own pass after it takes its room's light away (BA_SCONCES); the character's own torch burns a minute down for each minute gone; a wait's end asks the time it believes (TS_ASK): it's reckoned by then (its next due ahead of the clock, its minute this one)",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p n -s 535 -c /ram/c.hm -T -T -T', 'light the torch in the sconce', 'wait 5 seconds',
        '\u0100'.repeat(160) + 'look', 'wait 1 minute', String.fromCharCode(0x100).repeat(8) + 'inventory', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {                                  // (Its last minute, as the wait's typed: the sconce's own torch, the one lit in it)
        dq0Scene(m, [{ g: 1, line: 'wait 5 seconds', n: 1, call: () => {
          const { L } = m.dq0.layout(), per = Math.floor(0x2000 / L.itemSize), cat = m.dq0.bank(L.catBank), name = k => { let s = ''; for (let c = k * L.kindSize; cat[c]; c++) s += String.fromCharCode(cat[c]); return s; };
          const rec = i => { const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize; return { b, o }; };
          for (let i = 0; i < L.nitems; i++) {
            const { b, o } = rec(i);
            if (!b[o + L.iKind] || name(b[o + L.iKind]) !== 'torch' || !(b[o + L.iFlags] & 1) || !b[o + L.iWhere]) continue;
            const c = b[o + L.iX] | b[o + L.iY] << 8;
            const r = c < L.nitems && rec(c);
            if (b[o + L.iWhere] === m.dq0.layout().K.IW_ITEM && r && name(r.b[r.o + L.iKind]) === 'sconce') { b[o + L.iFuel] = 1; b[o + L.iFuel + 1] = 0; }
          }
        } }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], LIT = 1;
        const lit = T.of('light the torch in the sconce').after, idle = T.of('look'), after = idle.at, end = T.of('inventory').at;
        if (!lit || !after || !end) return ['no state'];
        const sconces = s => s.items.filter(i => i.name === 'sconce');
        const torchIn = (s, c) => s.items.find(i => i.where === 'IW_ITEM' && i.x === c.n) || null;
        const near = sconces(lit).sort((a, b) => Math.max(Math.abs(a.x - lit.pc.x), Math.abs(a.y - lit.pc.y)) - Math.max(Math.abs(b.x - lit.pc.x), Math.abs(b.y - lit.pc.y)))[0];
        const roomLit = (s, c) => { const r = s.changes.filter(x => x.what === 'ROOM_LIT' && x.sn === c.sn && x.x === c.x && x.y === c.y); return r.length ? r[r.length - 1].tile : -1; };
        if (!near || !torchIn(lit, near) || !(torchIn(lit, near).flags & LIT) || roomLit(lit, near) !== 2) f.push('the sconce not lit, its room not lit by it: the scene is gone');
        const t = near && torchIn(after, near);
        if (t && (t.flags & LIT)) f.push('the sconce\'s torch still lit after its last minute: ' + JSON.stringify(t));
        if (near && roomLit(after, near) !== 0) f.push('the sconce out, its room\'s light kept (the sconces\' pass, BA_SCONCES, not run): ' + roomLit(after, near));
        const mine = s => s.items.find(i => i.name === 'torch' && i.where === 'IW_ACTOR' && !i.x && (i.flags & LIT));
        const m0 = mine(lit), m1 = mine(after);
        if (!m0 || !m1 || !(m1.fuel < m0.fuel)) f.push('its own torch not burnt down in the minute gone: ' + JSON.stringify([m0 && m0.fuel, m1 && m1.fuel]));
        if (!(end.pc.tb.due > end.now)) f.push('the time it believes not reckoned after the wait (its next due not ahead of the clock): ' + end.pc.tb.due + ' at ' + end.now);
        const m2 = mine(end);
        if (m1 && m2 && !(m2.fuel < m1.fuel)) f.push('its own torch not burnt down through the wait: ' + m1.fuel + ' to ' + m2.fuel);
        if (!(end.pc.tb.at * 600 > end.now - 600)) f.push('the time it believes not brought up to the wait: ' + end.pc.tb.at + ' at ' + end.now);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0time', near: ['time', 'senses'], what: "Dingus Quest Zero's time of day believed (2026-10-31: DUNGEON.md's the time of day believed, HOUSE.md's, for review; the user's: fuzzy unless the sky's seen, arbitrary after a sleep, set by the sun when north is known or by watching it): the character's belief (the hub's tb: its error from the true clock and its doubt, the world's part 76) and its words: a hand-written theme's garrison of men-at-arms (seed 2, their lair south of the start: dq0daily's, out of earshot), the game begun at 8 o'clock (the environment's dq0hour), \"time\" told plainly (\"It's eight in the morning.\"); its torch put out (so its burning out breaks no long wait), two hours waited (the hours' tests: a doubt, \"about\" or \"some time around\" the hour, not plainly); three hours slept (TG_SLEEP, its doubt an eighth of the sleep more at the least); an hour more waited, then a sky in sight poked (tb.sky's SKY_OPEN, as a far exit's open sky: the afternoon's sun) as the sun's watched (\"watch the sun\", a quarter hour's wait): first read by the north it believes in (\"You see the sun in the sky\"), then watched ten minutes (\"You watch the sun a while\": the time to a quarter of an hour, its error none); the sky gone, then poked in sight again with its north believed a quarter of a turn off (pc_skew 2, unknown to it): the sun read six hours wrong (its error 360 minutes, its doubt half an hour); \"wait until night\" lasting till night as it believes the time (21 o'clock by its belief: the wait's minutes and the believed minute meet there); quit and carried on (dq0 -d), the belief as it was (world.dat's); and a character with absolute timesense (its file's \"timesense\" line, hmchar's rare trait): two hours slept, its time known still, told plainly",
      init: 't_rc', cycles: 5600e6,   // (4454M, 2026-10-31: the garrison's day as it waits; 4903M, 2026-11-02: a room search's bytes and moving's at each start)
      machine: { input: dq0Typed(dq0Game({ lines: ['cp /ram/c.hm /ram/k.hm', 'echo timesense >> /ram/k.hm', 'echo hour 8 >> /ram/s.sc', 'mkdir /ram/s', "echo 'faction | Guard | manarms | 0-0 | 12 | captain | 0 | u | -' > /ram/s/theme.txt", 'mkdir /ram/g', 'cp /ram/s/theme.txt /ram/g/theme.txt',
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'time', 'douse the torch', 'wait 2 hours', 'time', 'sleep 3 hours', 'time', 'wait 1 hour',
        'watch the sun', 'time', 'wait 10 minutes', 'wait 1 minute', 'time', 'wait until night', 'time', 'quit',
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'look', 'quit',
        'dq0 -p t -s 1 -c /ram/k.hm -T -d /ram/h -S /ram/s.sc', 'douse the torch', 'sleep 2 hours', 'time', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {
        const due = { tbDue: 0, tbDue1: 0, tbDue2: 0, tbDue3: 0 };
        dq0Scene(m, [{ g: 1, line: 'watch the sun', n: 1, hub: Object.assign({ tbSky: 2 }, due) },   // (The open sky in sight: SKY_OPEN)
          { g: 1, line: 'wait 10 minutes', n: 1, hub: Object.assign({ tbSky: 0 }, due) },          // (Gone from sight)
          { g: 1, line: 'wait 1 minute', n: 1, hub: Object.assign({ tbSky: 2, pcSkew: 2, pcLost: 0 }, due) },   // (Its north a quarter off)
          { g: 1, line: 'wait until night', n: 1, hub: Object.assign({ tbSky: 0 }, due) }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), P = x => (x && x.pc && x.pc.tb) || null, at = l => T.of(l).at;
        const after = l => P(T.of(l).after), times = [...s.matchAll(/time ([^>]*?)(?= > |$)/g)].map(x => x[1]);
        if (!at('quit')) return ['the game not played'];
        if (!/time It's eight in the morning\./.test(s)) f.push('the start not told plainly: ' + times[0]);
        const w = after('wait 2 hours'), sl = after('sleep 3 hours');
        if (!w || !(w.unc > 0) || w.how !== 0) f.push('no doubt after two hours awake: ' + JSON.stringify(w));
        if (!/time It's (about|some time around) [a-z ]+, you (think|'d guess)\./.test(s)) f.push('the doubt not told: ' + times.join(' | '));
        if (!sl || !w || sl.how !== 1 || !(sl.unc >= w.unc + 20)) f.push('the sleep not doubted (TG_SLEEP, an eighth of it more): ' + JSON.stringify([w, sl]));
        if (!/You see the sun in the sky: it's [a-z ]+, you judge\./.test(s)) f.push('the sun not read as seen');
        if (!/You watch the sun a while, its slow way across the sky: it's [a-z ]+, and you know which way north lies now\./.test(s)) f.push('the sun not watched');
        const ws = after('watch the sun');
        if (!ws || ws.skew !== 0 || ws.unc !== 15) f.push('the watched sun not the time to a quarter hour: ' + JSON.stringify(ws));
        const w1 = after('wait 1 minute');
        if (!w1 || w1.skew !== 360 || w1.unc !== 30) f.push('the sun not read six hours off by a north a quarter off: ' + JSON.stringify(w1));
        const ud = T.of('wait until night'), ev = ud.events.filter(e => e.kind === 'E_TIME'), ub = P(ud.at);
        if (ub && ev.length) {
          const b = (Math.floor(ud.at.now / 600) + 8 * 60 + 1440 + ub.skew) % 1440;
          const mins = ev[0].mins ? ev[0].w1 : ev[0].w1 / 600;
          if ((b + mins) % 1440 !== 1260) f.push('the wait not till night as believed: ' + b + ' + ' + mins);
        } else f.push('the wait until night not begun');
        const back = P(at('look')), left = P(T.of('quit').at);
        if (!back || !left || back.skew !== left.skew || back.unc !== left.unc) f.push('the belief not carried on: ' + JSON.stringify([left, back]));
        const kt = after('sleep 2 hours');
        if (!kt || kt.tsense !== 1 || kt.unc !== 0 || kt.skew !== 0) f.push('absolute timesense not kept through a sleep: ' + JSON.stringify(kt));
        if (!/time It's ten in the morning\.[^]*time It's ten in the morning\./.test(s)) f.push('the timesense character\'s time not told plainly: ' + times.join(' | '));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0sky', near: ['time', 'senses'], what: "Dingus Quest Zero's sky (2026-11-03, the user's \"All three together\": HOUSE.md's what shows the sky, for review): by the seed a shaft open to the sky in a stratum's links (seed 7's, choked below; seed 1 none), the rest by the environment's dq0sky (every top-stratum room a skylight and a window on the surface, a shaft beside the first way down, going on down): a skylight and a window told and on the map, the sun read from beside them, watched, the stars; daylight lighting the squares by day, none by night; a window's sky seen through it, its shutters shut not, never climbed through; a shaft too high to reach, barred at the top, climbed down by the ledge's rules to its foot on the stratum below, its light alone there; a thing dropped down it found at its foot; a shaft a way out only with the way out open",
      init: 't_rc', cycles: 5000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 1 -c /ram/c.hm', 'quit', 'dq0 -p t -s 7 -c /ram/c.hm', 'look', 'd', 'quit',
        'echo sky 1 >> /ram/s.sc', 'echo hour 12 >> /ram/s.sc',
        'dq0 -p t -s 1 -c /ram/c.hm -S /ram/s.sc', 'look east', 'examine the window', 'climb through the window', 'douse the torch', 'look', 'map', 'wait 5 hours', 'wait 4 hours', 'look', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -S /ram/s.sc', 'look southwest', 'open the window', 'look southwest', 'quit',
        'dq0 -p t -s 2 -c /ram/c.hm -T -S /ram/s.sc', 'u', 'go to the shaft', 'drop the flask', 'd', 'look', 'u', 'quit',
        'dq0 -p t -s 2 -c /ram/c.hm -T -S /ram/s.sc', 'go to the shaft', 'u',
        'echo hour 22 >> /ram/s.sc', 'dq0 -p t -s 2 -c /ram/c.hm -T -S /ram/s.sc', 'go to the shaft', 'douse the torch', 'look', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {
        const roof = ft => () => {                       // (The roof of the character's chunk's rooms: a shaft's mouth in reach)
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), w = m.dq0.bank(WORLD_BANK_N), st = stratumRec(m, h[L.pcSn]);
          const ox = (h[L.pcX] | h[L.pcX + 1] << 8) - st.x0, oy = (h[L.pcY] | h[L.pcY + 1] << 8) - st.y0;
          const [ZONE, ZSIZE, CMID, NZ] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, zone)', 'sizeof (struct zone)', 'offsetof (struct zone, cmid)', 'offsetof (struct chunk, nzones)']);
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (w[sl] && b && w[sl + 1] === h[L.pcSn] && (w[sl + 2] | w[sl + 3] << 8) === ox >> 5 && (w[sl + 4] | w[sl + 5] << 8) === oy >> 5) {
              for (let z = 0; z < b[o + NZ]; z++) b[o + ZONE + z * ZSIZE + CMID] = ft;
            }
          }
        };
        const rec = g => ({ g, line: 'quit', n: 1, call: () => { (m.sky = m.sky || {})[g] = [stratumRec(m, 0), stratumRec(m, 1)]; } });
        const goto_head = () => {                        // (Seed 7: put on the head of the shaft, to go down it)
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), k = stratumRec(m, 0).links.find(l => l.kind === 6 && l.dug === 2);
          if (k) { h[L.pcX] = k.x & 255; h[L.pcX + 1] = k.x >> 8; h[L.pcY] = k.y & 255; h[L.pcY + 1] = k.y >> 8; }
        };
        dq0Scene(m, [rec(1), rec(2), { g: 2, line: 'd', n: 1, call: goto_head }, { g: 3, line: 'look', n: 2, call: () => { const h = m.dq0.bank(0), [A,B] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct hub, tb.nsky)', 'offsetof (struct hub, tb.sun)']); m.dbg = [h[A], h[B]]; } }, rec(5), { g: 6, line: 'u', n: 1, call: roof(8) }]);
      },
      check(m, out) {
        const f = [], G = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '').split('enters the dungeon'), ok = (g, re, what) => { if (!re.test(G[g] || '')) f.push(what); };
        const sk = m.sky || {}, sky = (r, kind) => r && r.links.find(l => l.kind === kind && l.dug === 2);
        if (!sk[1] || sky(sk[1][0], 5)) f.push('seed 1 with a shaft: ' + JSON.stringify(sk[1]));
        const up = sk[2] && sky(sk[2][0], 5), dn = sk[2] && sky(sk[2][0], 6);
        if (!up || up.to !== 255 || !dn || dn.to !== 1 || dn.x !== up.x || dn.y !== up.y) f.push('seed 7 with no shaft going on down: ' + JSON.stringify(sk[2]));
        ok(2, /> d > d The shaft is choked with fallen rock a little way down\./, "seed 7's shaft not choked below");
        ok(3, /There is an empty window frame ahead, looking on the sky\./, 'the window on the surface not told');
        ok(3, /You see a skylight in the roof ahead\./, 'the skylight not told');
        ok(3, /The sun shows in the skylight above: it's noon, you judge\./, 'the sun not read at the skylight');
        ok(3, /Through the window ahead you see the sky, up a light well\./, 'the sky not seen through the window');
        ok(3, /It looks on the sky\./, 'the window not told on examining it');
        ok(3, /You can't get up onto the sill\./, 'the window climbed through');
        ok(3, /> look > look You are at the far end of a bedchamber\. /, 'daylight not lighting the room with the torch out');
        ok(3, /\|\.\^\|/, 'the skylight not on the map');
        ok(3, /You watch the sun a while at the window, its slow way across the sky: it's [a-z ]+, and you know which way north lies now\./, 'the sun not watched');
        ok(3, /You watch the stars a while at the window as they wheel: it's [a-z ]+, and you know which way north lies now\./, 'the stars not watched');
        if (!m.dbg || m.dbg[1] !== 0) f.push('the daylight not gone by night: ' + JSON.stringify(m.dbg));
        ok(4, /a glazed window ahead to your right \(its shutters closed\), looking on the sky\./, 'the shuttered window not told');
        ok(4, /You can't see past the window ahead: its shutters are closed\./, 'seen through its shutters');
        ok(4, /You open the shutters\. /, 'the shutters not opened, plainly');
        ok(4, /Through the window ahead to your left you see the sky, up a light well\./, 'the sky not seen with its shutters open');
        ok(5, /The sun shows at the top of the shaft above: it's noon, you judge\./, 'the sun not read at the shaft');
        ok(5, /> u > u The shaft's mouth is too high to reach\./, 'the high roof reached');
        ok(5, /You stand under a shaft open to the sky, daylight falling from it\. You stand by a shaft in the floor\./, 'the shaft not told');
        ok(5, /Put down\. It falls away down the shaft, and you hear it land far below\./, 'the flask not fallen');
        ok(5, /You set to climbing: \d+ feet of it\./, 'the shaft not climbed by rounds');
        ok(5, /You climb down the shaft\. You come into /, 'not come down the shaft');
        ok(5, /You climb down the shaft\.[^]*At your feet lies a flask of oil\./, 'the flask not at the foot');
        ok(5, /Pale daylight falls from far up the shaft above: it's day\./, 'the light alone not told from below');
        if (/skylight/.test((G[5] || '').split('You come into ')[1] || '')) f.push('a skylight below the top stratum');
        if (!sk[5] || !sky(sk[5][1], 5) || sky(sk[5][1], 5).to !== 0) f.push('the foot not a link on the stratum below: ' + JSON.stringify(sk[5]));
        ok(6, /You set to climbing: 20 feet of it\.[^]*You climb out into the light of day with Aldric's door still shut/, 'not out by the shaft, unsolved, as it goes up');
        ok(7, /Stars show at the top of the shaft above: it's [a-z ]+, you judge\./, 'the stars not read at the shaft');
        if (/daylight falling/.test(G[7] || '')) f.push('daylight by night, told');
        ok(7, /> look > look It is dark\./, 'lit by night with the torch out');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pace', meaning: true, what: 'Dingus Quest Zero\'s pace and watchfulness (the user\'s, 2026-10-16: DUNGEON.md, moving), by the world\'s state of them (the hub\'s gait and watch: the UI\'s requests, WQ_SET): a walk to begin with; a jog set, then searching for traps as it goes, listening too, each stopped, mapping as it goes and stopped; a sprint set, a slow walk; "run north" setting the run as it goes (the move the command\'s), "run south" going on at it; a creep, listening as it goes; kept (dq0 -d: the world\'s world.dat) and the game carried on at them (its words: dq0wordsthings; the sheet\'s pace line, the referee\'s: dq0harm)',
      init: 't_rc', cycles: 1100e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -d /ram/p', 'status', 'jog',
        'search for traps as I go', 'listen as I go', 'stop searching', 'stop listening', 'map as I go', 'stop mapping', 'set pace to sprint',
        'walk slowly', 'run north', 'run south', 'creep', 'listen as I go', 'quit', 'dq0 -p t -d /ram/p', 'status', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        // (rs.h's GA_CREEP 0 ... GA_SPRINT 5; hub.h's WA_TRAPS 1, WA_LISTEN 2, WA_MAP 4)
        const T = dq0Turns(m), f = [];
        const want = [['status', 2, 0], ['jog', 3, 0], ['search for traps as I go', 3, 1], ['listen as I go', 3, 3], ['stop searching', 3, 2],
          ['stop listening', 3, 0], ['map as I go', 3, 4], ['stop mapping', 3, 0], ['set pace to sprint', 5, 0], ['walk slowly', 1, 0],
          ['run north', 4, 0], ['run south', 4, 0], ['creep', 0, 0], ['listen as I go', 0, 2], ['dq0 -p t -d /ram/p', 0, 2], ['status', 0, 2]];
        let k = 0;
        for (const [l, gait, watch] of want) {
          const t = T.of(l, k), s = t.after;
          k = t.k + 1;
          if (t.k < 0 || !s || s.pc.gait !== gait || s.pc.watch !== watch) { f.push(l + ': pace ' + (s && s.pc.gait) + ', watching ' + (s && s.pc.watch) + ', not ' + gait + ', ' + watch); if (t.k < 0) break; }
        }
        dq0Need(f, T.of('dq0 -p t -d /ram/p'), 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tiring', asks: true, what: 'Dingus Quest Zero\'s strenuous work tiring the character (the user\'s, 2026-10-20: "Tiring work includes swimming, climbing, running, battering down a door"; HOUSE.md\'s moving and tiring: the world\'s RS_EXERT at GA_WORK, the referee\'s rate the book\'s for strenuous activity, a level each five minutes) and a search cut short (the user\'s: the part done, the nearest squares first, each at the full chance), by the world\'s events and the referee\'s replies and summary (HarnMaster\'s, at these seeds): at seed 3 a pick\'s shaft (dq0 -T), each ten-minute spell two fatigue levels, a level more asked of ("You\'re tiring.  Keep digging?"), yes going on, no stopping it (nothing more charged), and a spell stopped part way tiring for the part of it done; one nearly a level tired (pc.hm\'s pace line) climbing seed 517\'s ledge (dq0harm\'s: a round without a hold first), its first round tiring it a level more, asked of, no refused part way up and the climb on up; the same battering seed 14\'s door (dq0battering\'s, the torch put down first, the pick taken), its first blow tiring it a level more, asked of, and each blow heard (a noise a blow, E_NOISE\'s NZ_BLOW); then (seed 7\'s thief by the door with a hidden dart trap, dq0traps\', at the menacing pace) two whole searches reaching every square (E_PORTAL\'s reach, 19), a third stopped at once reaching only the character\'s own square (the trap beside it not found), and in the same game again a third stopped part way reaching the door, the trap found (at the full chance: the third search\'s, as dq0traps\' whole one finds it), each search\'s reach its part done\'s',
      init: 't_rc', cycles: 2700e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/t.hm', 'echo pace 2 0 5990 >> /ram/t.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', 'dig down', 'yes', 'no', 'dig down', '!' + 'Ā'.repeat(3) + 'stop', 'quit',
        'dq0 -p t -s 517 -c /ram/t.hm', 'ne', 'e', 'u', 'no', 'quit', 'dq0 -p t -s 14 -c /ram/t.hm -k pick -a', 'drop the torch', 'break the door', 'quit',
        'hmchar -q -s 1 -u feudal -c thief -o /ram/th.hm', 'echo ml lck 95 >> /ram/th.hm',
        ...['!stop', '!' + 'Ā'.repeat(6) + 'stop'].flatMap(st => ['dq0 -p m -s 7 -c /ram/th.hm -a', 'search', 'search', 'search', st, 'quit']), 'echo $status']
        .map(l => (l[0] === '!' ? l.slice(1) : 'ā' + l) + '\r').join('') },
      start(m) {   // (2026-11-08, one line of sight: seed 3's rats sent off as the digs begin, dq0Alone's: one in the passage
        // below, out of the character's sight now as the character's out of its, wanders in and out of view, its stirring
        // stopping the spell short)
        dq0Scene(m, [1, 2].map(n => ({ g: 1, line: 'dig down', n, call: (h, L, K, at) => dq0Alone(m, h, L, K, at, -1) })).concat([
          // (Rigged, dice2: the ledge's climb a round without a hold first (a marginal failure), then made; the third search
          // stopped part way finding the trap it reaches, at the full chance dq0traps' whole one has)
          { g: 2, line: 'u', n: 1, rig: [{ kind: 'TEST', a: 'P_CLIMB', actor: -1, uses: 1, grade: 'fail' }, { kind: 'TEST', a: 'P_CLIMB', actor: -1, grade: 'pass' }] },
          // (And the first two searches of each failing, fix2's: whole, nothing found, as the test has them; with the keyed
          // dice at fix2's timing the second of game 4 found the trap)
          ...[4, 5].map(g => ({ g, line: 'search', n: 1, rig: [{ kind: 'TEST', a: 'P_SEARCH', actor: -1, grade: 'fail' }] })),
          { g: 5, line: 'search', n: 3, rig: [{ kind: 'TEST', a: 'P_SEARCH', actor: -1, grade: 'pass' }] }]));
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], K = m.dq0.layout().K;
        const up = x => x ? +((x.status.match(/UP (\d+)/) || [])[1]) : NaN, ups = (t, u = t) => 'UP ' + up(t.at) + ', then ' + up(u.after);
        const work = t => t.replies.filter(r => /^slot/.test(r.req) && r.kind === K.RS_EXERT && r.a === K.GA_WORK).map(r => r.w1);
        // (A dig: each spell its ten minutes' strenuous work, two levels; a level more asked of, as it goes on)
        const d = T.after(T.of('dq0 -p t -s 3 -c /ram/c.hm -k pick -T'), 'dig down'), y = T.after(d, 'yes'), n = T.after(y, 'no');
        dq0Need(f, d, 'E_TASK', { te: 'TE_ASK_TIRED', task: 'LT_DIG' }, 'the first spell\'s fatigue not asked of');
        if (!(up(d.at) === 0 && up(d.after) === 2) || work(d).join() !== '6000') f.push('a spell of digging not ten minutes\' work, two levels: ' + ups(d) + ' (' + work(d) + ')');
        dq0Need(f, y, 'E_TASK', { te: 'TE_ON', task: 'LT_DIG' }, 'the dig not gone on at yes');
        dq0Need(f, y, 'E_TASK', { te: 'TE_ASK_TIRED', task: 'LT_DIG' }, 'the second spell\'s fatigue not asked of');
        if (up(y.after) !== 4) f.push('the second spell not two levels more: ' + ups(y));
        dq0Need(f, n, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_DIG' }, 'the dig not stopped at no');
        if (up(n.after) !== 4 || work(n).some(w => w)) f.push('tired more by the spell stopped as it began: ' + ups(n) + ' (' + work(n) + ')');
        const s = T.after(T.after(n, 'dig down'), 'stop'), w = work(s);
        dq0Need(f, s, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_DIG' }, 'the dig again not stopped');
        if (w.length !== 1 || !(w[0] > 0 && w[0] < 6000)) f.push('the spell stopped part way not tiring for the part of it done (' + w + ')');
        // (A climb's round and a battering's blow tiring one nearly a level tired a level more, asked of, going on)
        const c = T.after(T.of('dq0 -p t -s 517 -c /ram/t.hm'), 'u'), cn = T.after(c, 'no'), cw = [...work(c), ...work(cn)];
        dq0Need(f, c, 'E_TASK', { te: 'TE_ASK_TIRED', task: 'LT_CLIMB' }, 'the climb\'s first round tiring a level more not asked of');
        dq0Need(f, cn, 'E_TASK', { te: 'TE_CANT', task: 'LT_CLIMB' }, 'no not refused part way up the climb');
        dq0Need(f, cn, 'E_CLIMB', { how: 'K_UP' }, 'the climb not gone on up, no for an answer');
        if (!(up(c.at) === 0 && up(cn.after) === 1) || !cw.length || cw.some(w => w !== 100)) f.push('the climb\'s rounds not strenuous work, a level more: ' + ups(c, cn) + ' (' + cw + ')');
        const b = T.after(T.of('dq0 -p t -s 14 -c /ram/t.hm -k pick'), 'break the door'), blows = work(b);
        dq0Need(f, b, 'E_TASK', { te: 'TE_ASK_TIRED', task: 'LT_BATTER' }, 'the first blow tiring a level more not asked of');
        if (!(up(b.at) === 0 && up(b.after) === 1) || blows.length !== 12 || blows.some(w => w !== 30)) f.push('the dozen blows not strenuous work, three seconds each, a level more: ' + ups(b) + ' (' + blows + ')');
        const heard = dq0Ev(b, 'E_NOISE', {});
        if (heard.length !== blows.length || heard.some(e => e.b !== K.NZ_BLOW)) f.push('each blow not heard, a blow\'s noise: ' + heard.map(e => e.b + ':' + e.a).join(' ') + ' for ' + blows.length + ' blows');
        // (Searches: whole, every square; cut short, the nearest as far as its part done reached, its reach told)
        const reach = t => t.after ? Math.min(19, (t.after.pc.task.done + 31) >> 5) : -1, cut = g => T.after(T.after(T.after(T.after(g, 'search'), 'search'), 'search'), 'stop');
        const g1 = T.of('dq0 -p m -s 7 -c /ram/th.hm'), g2 = T.of('dq0 -p m -s 7 -c /ram/th.hm', g1.k + 1), x1 = cut(g1), x2 = cut(g2);
        for (const [g, x, how] of [[g1, x1, 'at once'], [g2, x2, 'part way']]) {
          const s1 = T.after(g, 'search'), s2 = T.after(s1, 'search');
          for (const t of [s1, s2]) dq0Need(f, t, 'E_PORTAL', { po: 'PO_NOTHING', a: 19 }, 'a whole search (' + how + ') not reaching every square, nothing found');
          dq0Need(f, x, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_SEARCH' }, 'the third search not stopped ' + how);
          dq0Need(f, x, 'E_PORTAL', { a: reach(x) }, 'the search stopped ' + how + ' not reaching as far as its part done (' + reach(x) + ')');
        }
        dq0Need(f, x1, 'E_PORTAL', { po: 'PO_NOTHING', a: a => a < 2 }, 'the search stopped at once not its own square only');
        dq0Never(f, x1, 'E_TRAP', { to: 'TO_FOUND' }, 'the trap beside it found by a search that didn\'t reach it');
        dq0Need(f, x2, 'E_PORTAL', { po: 'PO_FOUND_TRAP', a: a => a >= 2 && a < 19 }, 'the search stopped part way, reaching the door, not finding its trap');
        dq0Need(f, x2, 'E_TRAP', { tk: 'TK_DARTS', tt: 'TT_DOOR', to: 'TO_FOUND' }, 'the dart trap not found by the search stopped part way');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lockstep', meaning: true, near: ['time'], what: "Dingus Quest Zero's lockstep turns pace (GAME-CRITIQUE.md's 6.4, the user's of 2026-11-17, lock1's): the UI takes a line only as the world, the minds and the senses are idle (hub.h's ui_want, w_idle), so a game at the turns pace is the same whatever the tasks' timing: seed 3 (dq0 -T, the giant rats about the way down) played three times with the same lines, each line's first key 0, 37,000 and 211,000 cycles after its prompt (acia.js's linePause: the lines landing at other phases of the denizens' turns, the minds' deciding and the senses' answers), the three games' stories the same, sentence for sentence, and the denizens' doings in them",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm',
        ...[0, 1, 2].flatMap(() => ['dq0 -p t -s 3 -c /ram/c.hm -T', ...LOCK_LINES, 'quit']), 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read, lags = [0, 37000, 211000];
        let typed = '', g = -1;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) { if (/^dq0 -p t/.test(typed)) ++g; typed = ''; } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
        a.linePause = () => g >= 0 && g < 3 ? lags[g] : 0;
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), gs = [];
        for (let i = s.indexOf('D I N G U S'); i >= 0; i = s.indexOf('D I N G U S', i + 1)) gs.push(i);
        if (gs.length !== 3) return ['not three games: ' + gs.length];
        const game = k => { const t = s.slice(gs[k], k + 1 < gs.length ? gs[k + 1] : s.length); return t.slice(0, t.indexOf('> quit')); };
        const sents = t => t.replace(/ ?\.{3,}/g, ' ...').split(/(?<=[.!?:])\s+/).map(x => x.trim()).filter(Boolean);   // (The start's dots its real time's)
        const g0 = sents(game(0));
        if (!g0.some(x => /giant rat/.test(x))) f.push('no rat met: the scene not set (' + game(0).slice(0, 400) + ')');
        for (const k of [1, 2]) {
          const gk = sents(game(k));
          let i = 0;
          while (i < g0.length && i < gk.length && g0[i] === gk[i]) i++;
          if (i < g0.length || i < gk.length) f.push('game ' + (k + 1) + ' (its lines ' + [0, 37000, 211000][k] + ' cycles after their prompts) not the first game\'s, from sentence ' + i + ': ' + JSON.stringify(g0.slice(i, i + 3)) + ' against ' + JSON.stringify(gk.slice(i, i + 3)));
        }
        const dens = k => (m.dq0.games[k] || { dens: [] }).dens.map(e => [e.de, e.who, e.sx, e.sy, e.t].join(':')).join(' ');
        if (m.dq0.games.length >= 3) for (const k of [1, 2]) if (dens(k) !== dens(0)) f.push('game ' + (k + 1) + ': its denizens\' doings not the first game\'s');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lockwake', meaning: true, near: ['time', 'fight'], what: "Dingus Quest Zero's lockstep turns pace as a long task is asked about and broken off (even1's, 2026-11-25: the world ran its passes, its jobs among them, as it waited for the UI to see to the events it told, and let it by at once on an even tick: a sleep's question answered (\"go on\", dq0 -a's) before the time believed was reckoned or after it, as the tasks' timing had it, so dq0wakeblow's and dq0digact's turns-pace games parted between two layouts): dq0wakeblow's second game (seed 1, the character tough among hostile cultists in the dark) put to sleep and woken by their blows, played four times with the same lines, each line's first key 0, 9,000, 37,000 and 211,000 cycles after its prompt (acia.js's linePause: the UI's answers to the world's events at other phases of its ticks), the four games' stories the same, sentence for sentence, and the world's events, the denizens' doings and the referee's answers in them",
      init: 't_rc', cycles: 3200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'echo hour 128 >> /ram/s.sc', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/d.hm',
        'echo att str 99 >> /ram/d.hm', 'echo att sta 99 >> /ram/d.hm', 'echo att wil 99 >> /ram/d.hm', 'mkdir /ram/h', "echo 'faction | Foes | cultist | 0-0 | 30 | - | 0 | h | -' > /ram/h/theme.txt",
        ...LOCK_WAKE_LAGS.flatMap(() => ['dq0 -p t -s 1 -c /ram/d.hm -T -d /ram/h -S /ram/s.sc', ...LOCK_WAKE, 'quit']), 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '', g = -1;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) { if (/^dq0 -p t/.test(typed)) ++g; typed = ''; } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
        a.linePause = () => g >= 0 && g < LOCK_WAKE_LAGS.length ? LOCK_WAKE_LAGS[g] : 0;
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), gs = [], n = LOCK_WAKE_LAGS.length, ks = LOCK_WAKE_LAGS.map((x, k) => k).slice(1);
        for (let i = s.indexOf('D I N G U S'); i >= 0; i = s.indexOf('D I N G U S', i + 1)) gs.push(i);
        if (gs.length !== n || m.dq0.games.length !== n) return ['not ' + n + ' games: ' + gs.length + ', ' + m.dq0.games.length];
        const game = k => { const t = s.slice(gs[k], k + 1 < gs.length ? gs[k + 1] : s.length), q = [t.indexOf('> quit'), t.indexOf('% ')].filter(x => x >= 0); return t.slice(0, Math.min(...q, t.length)); };   // (Its end: its quit, or the shell's prompt after the character's death)
        const sents = t => t.replace(/ ?\.{3,}/g, ' ...').split(/(?<=[.!?:])\s+/).map(x => x.trim()).filter(Boolean);   // (The start's dots its real time's)
        const g0 = sents(game(0)), G = m.dq0.games;
        if (!g0.some(x => /cultist/.test(x)) || !G[0].events.some(e => e.te === 'TE_HIT')) f.push('no cultist met, its blow not waking the sleeper (E_TASK TE_HIT): the case not met');
        if (!G[0].events.some(e => e.te === 'TE_ASK_HEARD')) f.push('the sleep not asked about as it went (E_TASK TE_ASK_HEARD): the case not met');
        for (const k of ks) {
          const gk = sents(game(k));
          let i = 0;
          while (i < g0.length && i < gk.length && g0[i] === gk[i]) i++;
          if (i < g0.length || i < gk.length) f.push('game ' + (k + 1) + ' (its lines ' + LOCK_WAKE_LAGS[k] + ' cycles after their prompts) not the first game\'s, from sentence ' + i + ': ' + JSON.stringify(g0.slice(i, i + 3)) + ' against ' + JSON.stringify(gk.slice(i, i + 3)));
        }
        // (What happened, in order: the world's events, the denizens', the referee's answers to the slots)
        const evs = k => G[k].events.map(e => [e.kind, e.t, e.a, e.b, e.w1, e.w2].join(':')), dens = k => G[k].dens.map(e => [e.de, e.who, e.sx, e.sy, e.t].join(':'));
        const refs = k => G[k].replies.filter(r => /^d?slot/.test(r.req)).map(r => [r.rs, r.actor, r.a, r.b, r.w1, r.result, r.grade].join(':'));
        for (const k of ks) for (const [what, w] of [['the world\'s events', evs], ['the denizens\' doings', dens], ['the referee\'s answers', refs]]) {
          const x = w(0), y = w(k);
          let i = 0;
          while (i < x.length && i < y.length && x[i] === y[i]) i++;
          if (i < x.length || i < y.length) f.push('game ' + (k + 1) + ': ' + what + ' not the first game\'s, from ' + i + ': ' + x.slice(i, i + 2).join(' ') + ' against ' + y.slice(i, i + 2).join(' '));
        }
        return f;
      },
      sdcard: true
    },
  ],
};
