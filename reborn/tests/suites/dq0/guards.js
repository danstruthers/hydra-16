// ****************************************************************************
// guards.js - the emulator's guards' own test (sim/lib/guards.js: GAME-CRITIQUE.md's 6.1, E1): each guard made to fire
// in a game, by a poke that does no harm, and heard (the test's guards: the kinds it makes fire, noted not failed, each
// wanted).  (tests.js, the index: the suites in their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { dq0Typed, dq0Story } = require('../../lib/dq0.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder)

// dq0guards: at seed 3, the turns pace, a few commands; as the world runs (its task by its name, its facts the
// guards' own: guards.facts) each guard made to fire once, the game going on unharmed:
//   fetch  an RTS run from the hub's header's bank (a $60 put in its last byte, past struct hub: the bank's room no
//          one uses; its byte put back after), its return the instruction it was put before: as a return into the
//          window with the hub in it would (79083a98's crash)
//   store  one of the world's stores that name the hub's header run with a bank of the world's own that it never
//          used in the window; the byte it wrote put where it belonged after (79083a98's HUB->ireq.to)
//   swap   the swap stack's depth put at its SWDEPTH before sw_call's inc swsp, put right after it
//   stack  a write of the C library's to cc65's sp's high byte (sta, stx, sty, inc, dec sp+1: its first, crt0's as
//          the world starts) made to write the page under its floor (the register or the byte put so before it), what
//          it should have written put right after
// Each in the JS CPU (cpu.step's), the guards' JS ways; the WebAssembly core's are every other test's
function dq0GuardsTest() {
  const lines = ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T',
    'look', 'turn left', 'inventory', 'step forward', 'look', 'quit', 'echo $status'];
  return {
    name: 'dq0guards', near: ['dq0'], guards: ['fetch', 'store', 'swap', 'stack'],
    what: 'The emulator\'s guards (sim/lib/guards.js, GAME-CRITIQUE.md\'s 6.1): in a game at seed 3, the world\'s task made, by pokes that do no harm, to fetch an RTS from the hub\'s header\'s bank, to run a store that names the hub\'s header with a bank of its own in the window, to take its swap stack past its depth and its C stack below its floor: each guard\'s hit heard (noted, not failed), and the game going on to its quit',
    init: 't_rc', cycles: 1500e6,
    machine: { input: dq0Typed(lines) },
    expect: ['% echo $status'],
    start(m) {
      const guards = require('../../../sim/lib/guards.js'), f = guards.facts('dq0w'), R = this.R = {};
      const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0w', 'dq0w.lbl'), 'latin1');
      const swCall = parseInt(/^al ([0-9A-F]+) \.sw_call$/m.exec(lbl)[1], 16);
      const stores = new Set(f.stores.main), PLAIN = { 0x8D: 0, 0x8E: 0, 0x8C: 0, 0x9C: 0, 0x9D: 'X', 0x9E: 'X', 0x99: 'Y' };
      const cpu = m.cpu, step = cpu.step.bind(cpu), H = require('../../../games/dq0/hubgen.js').load().C;
      const nameOf = T => { let s = ''; for (let i = 0; i < 16 && m.taskRam[T][0x230 + i]; i++) s += String.fromCharCode(m.taskRam[T][0x230 + i]); return s; };
      let world = -1, incAt = -1;
      const SP_OPS = { 0x85: 'A', 0x86: 'X', 0x84: 'Y', 0xC6: -1, 0xE6: 1 };   // (Writes to sp + 1: the register, or a step)
      const header = () => {                                  // (The window's bank the hub's header: its shared bank's)
        const b = m.taskRam[world][0];
        if (b < 0xF0) return null;
        const k = m.U * 16 + (b & 15), bank = m.sharedBanks()[k];
        return bank && bank[0] === (H.HUB_MAGIC & 255) && bank[1] === H.HUB_MAGIC >> 8 ? { k, bank } : null;
      };
      let rts = null;
      cpu.step = v => {
        if (world < 0) {                                      // (The world's task, once the game's begun)
          if (!(m.dq0 && m.dq0.games.length)) return step(v);
          for (let T = 0; T < 16; T++) if (nameOf(T) === 'dq0w') world = T;
          if (world < 0) return step(v);
        }
        if (m.T !== world) return step(v);
        const r = m.taskRam[world], pc = cpu.PC;
        if (pc === swCall && incAt < 0) {                     // (Its code's places, its code there: its first part call)
          for (let a = swCall; a < swCall + 64 && incAt < 0; a++) if (r[a] === 0xE6 && r[a + 1] === f.swsp) incAt = a;   // (inc swsp)
        }
        if (rts && pc === rts.from) { rts.bank[0x1FFF] = rts.was; rts = null; }   // (Back from the RTS: its byte put back)
        if (!R.fetch && incAt >= 0 && pc < f.swap[0] && header()) {   // (fetch: an RTS put in the hub's header's bank)
          const { bank } = header();
          rts = { from: pc, bank, was: bank[0x1FFF] };
          bank[0x1FFF] = 0x60;
          const ret = (pc - 1) & 0xFFFF;
          r[0x100 + cpu.S] = ret >> 8; cpu.S = (cpu.S - 1) & 0xFF; r[0x100 + cpu.S] = ret & 0xFF; cpu.S = (cpu.S - 1) & 0xFF;
          cpu.PC = 0x9FFF;
          R.fetch = { from: pc, to: cpu.PC };
          return;                                             // (Not stepped: the next is the fetch, the guards' look first)
        }
        if (!R.store && stores.has(pc) && PLAIN[r[pc]] !== undefined && header()) {   // (store: a bank of its own)
          let free = -1;
          for (let b = 0x2F; b > 0 && free < 0; b--) if (!m.taskBankMem(world, b)) free = b;
          if (free > 0) {
            const { bank } = header(), was = r[0], op = r[pc], a = (r[pc + 1] | r[pc + 2] << 8) + (PLAIN[op] === 'X' ? cpu.X : PLAIN[op] === 'Y' ? cpu.Y : 0);
            r[0] = free;
            const out = step(v);
            r[0] = was;
            const v1 = m.taskBankMem(world, free)[a - 0x8000];
            bank[a - 0x8000] = v1;                            // (The byte where it belonged)
            R.store = { pc, a, v: v1, bank: free };
            return out;
          }
        }
        if (!R.swap && pc === incAt) {                        // (swap: its depth at SWDEPTH as inc swsp runs)
          const was = r[f.swsp];
          r[f.swsp] = f.swdepth;
          const out = step(v);
          r[f.swsp] = (was + 1) & 0xFF;
          R.swap = { was };
          return out;
        }
        if (!R.stack && pc < f.swap[0] && SP_OPS[r[pc]] !== undefined && r[pc + 1] === f.sp + 1) {   // (stack: sp's high byte
          const how = SP_OPS[r[pc]], lo = (f.floor >> 8) - 1, was = r[f.sp + 1];   //   written under its floor's page: its first, crt0's)
          let want, out;
          if (typeof how === 'number') {                      // (inc, dec: the byte put so the step lands under the floor)
            want = (was + how) & 0xFF;
            r[f.sp + 1] = (lo - how) & 0xFF;
            out = step(v);
            cpu.P = (cpu.P & ~0x82) | (want & 0x80) | (want ? 0 : 2);   // (Its flags the true result's)
          } else {                                            // (sta, stx, sty: the register so, put back after)
            const reg = cpu[how];
            want = reg;
            cpu[how] = lo;
            out = step(v);
            cpu[how] = reg;
          }
          r[f.sp + 1] = want;
          R.stack = { pc, how, was, want };
          return out;
        }
        return step(v);
      };
    },
    check(m, out) {
      const f = [], R = this.R, s = dq0Story(out);
      for (const k of ['fetch', 'store', 'swap', 'stack']) if (!R[k]) f.push('the ' + k + ' poke never made');
      if (!/> quit[^%]*% echo \$status/.test(s) || /rc: (look|quit|turn|step): not found/.test(s)) f.push('the game ended before its quit');
      if (!/You carry/.test(s)) f.push('no inventory: the game not unharmed');
      this.notes = ['the pokes: ' + JSON.stringify(R)];
      return f;
    },
    sdcard: true
  };
}

module.exports = {
  tests: [
    { name: 'dq0guards', ...dq0GuardsTest() },
  ]
};
