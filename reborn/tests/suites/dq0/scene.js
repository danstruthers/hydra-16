// ****************************************************************************
// scene.js - Dingus Quest Zero's tests: the scene builder itself (phase C's step 3, scene1, 2026-11-24: games/dq0/hub.h's
// scene, tests/lib/dq0.js's dq0Scene).  (tests.js, the index: the suites in their order; ../../lib: the helpers they
// share)
'use strict';
const fs = require('fs');
const path = require('path');
const { dq0Typed, dq0Game, dq0Scene, dq0SceneNow, dq0Prose } = require('../../lib/dq0.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder)

// dq0scene: a scene set while another task's copy of what it sets is in hand, put in place, where a poke is lost (OPEN.md's
// G18 and X1).  Each scene in two games: one poked as the tests did till 2026-11-24, one a scene (dq0SceneNow), at the same
// moment, by the JS CPU's steps:
//   the world's body (G18): at seed 5 (dq0fightmid's rats, the character tough: Stamina and Will 99) a rat set beside it
//     and fighting it as inventory's typed (dq0Scene's engage), then waits; as the world asks the referee the rat's first
//     blow (its slot's RS_ATTACK, the rat its actor: its body bd copied out of the hub, the world waiting), the rat's gait
//     set another; the poke's lost to the body written back (bd_put) as the blow's done, the scene's in place after it;
//   the lights' minute (X1): at seed 3 the character idle at the normal pace; as the lights' minute begins (the world's
//     burn_job, its first after a look), made due again: the poke (burn_due 0) lost to the minute's end setting it again,
//     the job's passes in that minute after it only its own second's (the sconces'); the scene's (SO_DUE: burn_ask) a
//     minute more run in it
function dq0SceneTest() {
  const pre = ['echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm'];
  const fight = ['d', 'wait', 'go right', 'inventory', ...Array(6).fill('wait')], rest = ['look', '\u0100'.repeat(150) + 'look'];   // (Idle at the normal pace five minutes and more: a minute's lights met)
  const lines = dq0Game({ pre, lines: [...['poke', 'scene'].flatMap(() => ['dq0 -p t -s 5 -c /ram/c.hm -k sword -T', ...fight, 'quit']),
    ...['poke', 'scene'].flatMap(() => ['dq0 -p n -s 3 -c /ram/c.hm -T', ...rest, 'quit']), 'echo $status'] });
  return {
    what: "Dingus Quest Zero's scenes set while another task's copy of what they set is in hand (OPEN.md's G18, X1): a rat's gait set as the world asks the referee its blow (its body copied out, bd), poked and lost to the body written back, a scene's in place; the lights' minute made due as it's under way, poked (burn_due) and lost to the minute's end, a scene's (SO_DUE, burn_ask) run in the same minute",
    init: 't_rc', cycles: 3100e6,
    machine: { input: dq0Typed(lines) },
    expect: ['% echo $status'],
    start(m) {
      const r = this.r = { body: [], burn: [], minutes: 0 };
      dq0Scene(m, [1, 2].map(g => ({ g, line: 'inventory', n: 1, k: 0, beside: 'face', engage: true })));   // (The rat before it, fighting it)
      const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0w', 'dq0w.lbl'), 'latin1');
      const lab = n => parseInt(new RegExp('^al ([0-9A-F]+) \\.' + n + '$', 'm').exec(lbl)[1], 16), BURN = lab('_burn_job'), DUE = lab('_burn_due');
      const a = m.acia, read = a.read, cpu = m.cpu, step = cpu.step.bind(cpu);
      let g = 0, typed = '', armed = null, st = null, n = 0;
      a.read = (rr, t) => {
        if (rr === 0 && a.rdrf) {
          const c = a.rx;
          if (c === 13) {
            if (/^dq0 /.test(typed)) { g++; armed = null; st = null; }
            if (typed === 'inventory' && g <= 2) armed = 'body';
            if (typed === 'look' && g >= 3 && !st) armed = 'burn';
            typed = '';
          } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
        }
        return read(rr, t);
      };
      cpu.step = v => {
        if (armed && !(++n & 63) && m.dq0 && m.dq0.bank(0)) {
          const h = m.dq0.bank(0), { L, K } = m.dq0.layout(), world = h[L.task + K.T_WORLD], at = k => L.actor + k * L.actorSize;
          const poke = g === 1 || g === 3, slot = L.slot + K.SLOT_WORLD * L.slotSize;
          if (armed === 'body' && !st && h[slot] === K.REQ_POSTED && h[slot + L.rqKind] === K.RS_ATTACK && h[slot + L.rqActor]) {
            const k = h[slot + L.rqActor] - 1, val = (h[at(k) + L.aGait] + 1) % K.NGAITS;
            st = { g, k, val };
            if (poke) h[at(k) + L.aGait] = val;
            else dq0SceneNow(m, (hh, LL) => { hh[at(k) + LL.aGait] = val; }, () => { r.body.push({ g, how: 'scene', kept: m.dq0.bank(0)[at(k) + L.aGait] === val }); }, 'dq0scene\'s gait');
          } else if (armed === 'body' && st && poke && h[slot] !== K.REQ_POSTED && !st.freed) {
            st.freed = true;                      // (The blow answered: the world on, its body written back as it's done)
            st.until = cpu.cyc + 2e6;
          } else if (armed === 'body' && st && poke && st.freed && cpu.cyc >= st.until) {
            r.body.push({ g, how: 'poke', kept: h[at(st.k) + L.aGait] === st.val });
            armed = null;
          }
        }
        if (armed === 'burn' && cpu.PC === BURN && m.T === m.dq0.bank(0)[m.dq0.layout().L.task + m.dq0.layout().K.T_WORLD]) {
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16 | h[L.now + 3] << 24;
          if (!st) {
            st = { g, now };                      // (A minute under way: made due again)
            r.minutes++;
            if (g === 3) { const ram = m.taskRam[m.T]; ram[DUE] = ram[DUE + 1] = ram[DUE + 2] = ram[DUE + 3] = 0; }
            else dq0SceneNow(m, (hh, LL, KK, at, named, sc) => sc.due('burn'), null, 'dq0scene\'s minute');
          } else if (now - st.now < 600) {
            const e = r.burn.find(x => x.g === g) || (r.burn.push({ g, how: g === 3 ? 'poke' : 'scene', runs: 0 }), r.burn[r.burn.length - 1]);
            e.runs++;                             // (The job's passes in the same minute after it: a minute's own second
                                                  //   pass, its sconces', in both; a minute more in the scene's)
          }
        }
        return step(v);
      };
    },
    check(m, out) {
      const f = [], R = this.r;
      this.notes = ['bodies ' + JSON.stringify(R.body), 'minutes ' + JSON.stringify(R.burn)];
      const b = h => R.body.find(x => x.how === h), u = h => R.burn.find(x => x.how === h);
      if (!b('poke')) f.push('the rat\'s blow not met with the poke');
      else if (b('poke').kept) f.push('the poke kept: the race (G18) not met, the scene\'s test moot');
      if (!b('scene') || !b('scene').kept) f.push('the scene set as the world held the rat\'s body not in place after it: ' + JSON.stringify(b('scene')));
      const runs = h => (u(h) || { runs: 0 }).runs;
      if (runs('scene') <= runs('poke')) f.push('the scene\'s minute not run again in the same minute: its passes ' + runs('scene') + ', the poke\'s ' + runs('poke'));
      if (R.minutes < 2) f.push('the lights\' minute met under way in ' + R.minutes + ' game(s), not 2');
      if (!/You carry/.test(dq0Prose(out))) f.push('no inventory told');
      return f;
    },
    sdcard: true
  };
}

module.exports = {
  tests: [
    { name: 'dq0scene', ...dq0SceneTest() },
  ]
};
