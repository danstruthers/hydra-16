// ****************************************************************************
// schedcheck.js - a check of the scheduler (kernel/sched.s), as a preload: NODE_OPTIONS="--require THIS" on any run
// that makes machines (sim/test.js, a tool of the emulator's).  At every pick (K_SCHED_PICK, found by the switch's
// call of it in BIOS ROM page 0) it notes the tick, the task switched out and the task picked; runs a model of the
// pick as it was before the candidates (7dfff75a: every task looked at, pick_ok's look) on the machine's memory as
// the pick began, and compares: the task picked, the scheduler's bytes (K0_FORES, FTICK, FMODE, FPICK, RR) and
// every task's state after it (a pick with an interrupt in one of its moments for them may differ: counted apart);
// and (SCHEDINV=1) that every task in a state the pick runs or ends, but the one switched out, is a candidate
// (K0_CANDLO/HI).  2026-11-21, the candidates' proof (docs/03-base/kernel.md, "The scheduler").
//   SCHEDTRACE=DIR    where the traces go: DIR/NAME.trace (a line a pick: the tick, the task out, the task picked,
//                     hex) and DIR/NAME.sum (the counts, the first mismatches); NAME: SCHEDNAME, else a test's name
//   SCHEDBIOS=FILE    the machines boot this BIOS ROM image instead (another build's kernel, the rest as built)
//   SCHEDNOMODEL=1    the trace alone
// sim/test.js runs each test in a process of its own: NODE_OPTIONS reaches each.  Breakpoints at the pick make the
// emulator step in JavaScript: some ten times slower.  E.g. (from reborn/): SCHEDINV=1 SCHEDTRACE=DIR
// NODE_OPTIONS="--require ../base/tools/schedcheck.js" node sim/test.js dq0 rc
'use strict';
const fs = require('fs'), path = require('path');
const mach = require(path.join(__dirname, '..', 'sim', 'lib', 'machine.js'));
const DIR = process.env.SCHEDTRACE || '.';
const NOMODEL = !!process.env.SCHEDNOMODEL;
fs.mkdirSync(DIR, { recursive: true });

const ST = { FREE: 0, READY: 1, WAIT: 2, CALL: 3, IDLE: 4, NEW: 5, SLEEP: 6, BLOCKED: 7, EVENT: 8 };
const TK_STATE = 0x81, TK_FLAGS = 0x82, TK_BUSY = 0x85, TK_BLOCKEDON = 0x86, TK_INCALLER = 0x88;
const TK_EVENT = 0xBD, TK_EVTASK = 0xBE, TK_EVV = 0xBF;
const TF_FORE = 0x02, TF_STOPPED = 0x80;
const K0 = { TICKS: 0x22, NQ_ANY: 0x41, FORES: 0x78, FTICK: 0x79, FMODE: 0x7A, FPICK: 0x7B, RR: 0x7C, FSEEN: 0x7D, FSRV: 0x7E };
const K_WAKE_LO = 0x400, K_WAKE_HI = 0x480, FORE_RUN = 8;

// The old pick, on a view of memory (R[t][addr]); writes go to the view.  OUT: the task picked
function oldPick(R, Y) {
  const k0 = R[0];
  const ticks16 = () => k0[K0.TICKS] | k0[K0.TICKS + 1] << 8;
  const ok = (x, mask) => {
    const r = R[x];
    if (r[TK_FLAGS] & mask) return false;
    const s = r[TK_STATE];
    if (s === ST.READY) return true;
    let go = false;
    if (s === ST.SLEEP) go = ((ticks16() - (k0[K_WAKE_LO + x] | k0[K_WAKE_HI + x] << 8)) & 0x8000) === 0;
    else if (s === ST.BLOCKED) go = R[r[TK_BLOCKEDON] & 15][TK_BUSY] === 0;
    else if (s === ST.EVENT) { const sv = R[r[TK_EVTASK] & 15]; go = sv[TK_STATE] === 0 || sv[TK_EVENT] !== r[TK_EVV]; }
    if (go) r[TK_STATE] = ST.READY;
    return go;
  };
  if (Y !== k0[K0.FPICK]) k0[K0.RR] = Y;
  let robin = true;
  if (k0[K0.FORES]) {
    const a = k0[K0.TICKS];
    let fpass = false;
    if (k0[K0.FMODE]) {
      if (a !== k0[K0.FTICK]) { k0[K0.FMODE] = 0; k0[K0.FTICK] = a; fpass = true; }
    } else if (((a - k0[K0.FTICK]) & 255) < FORE_RUN) fpass = true;
    else { k0[K0.FMODE] = (k0[K0.FMODE] + 1) & 255; k0[K0.FTICK] = k0[K0.TICKS]; }
    if (fpass) {
      k0[K0.FSEEN] = 0;
      let x = Y;
      for (let n = 0; n < 16; n++) {
        x = (x + 1) & 15;
        if (!x) continue;
        const r = R[x];
        if (r[TK_FLAGS] & TF_FORE) {
          if (ok(x, TF_STOPPED)) { k0[K0.FPICK] = x; return x; }
          k0[K0.FSEEN] = TF_FORE; continue;
        }
        const b = r[TK_BUSY];
        if (b && b < 0x80) {
          const c = R[r[TK_INCALLER] & 15];
          if (c[TK_STATE] === ST.CALL && (c[TK_FLAGS] & TF_FORE)) {
            if (ok(x, TF_STOPPED)) { k0[K0.FPICK] = x; return x; }
            continue;
          }
        }
        if (x === k0[K0.FSRV]) { if (ok(x, TF_STOPPED)) { k0[K0.FPICK] = x; return x; } }
      }
      if (!k0[K0.FSEEN]) k0[K0.FORES] = 0;
      k0[K0.FTICK] = k0[K0.TICKS];
    }
  }
  if (robin) {
    k0[K0.FPICK] = 0xFF;
    let x = k0[K0.RR];
    if (k0[K0.FORES]) {
      for (let n = 0; n < 16; n++) {
        x = (x + 1) & 15;
        if (!x) { if (k0[K0.NQ_ANY]) return 0; continue; }
        if (ok(x, TF_STOPPED | TF_FORE)) return x;
      }
      x = k0[K0.RR];
    }
    for (let n = 0; n < 16; n++) {
      x = (x + 1) & 15;
      if (!x) { if (k0[K0.NQ_ANY]) return 0; continue; }
      if (ok(x, TF_STOPPED)) return x;
    }
    return 0;
  }
}

// The switch's call of the pick in page 0 of the BIOS image: inc TK_PREEMPT; cli; nop; sei; jsr K_SCHED_PICK; dec
function findPick(osrom) {
  const pat = [0xE6, 0x83, 0x58, 0xEA, 0x78, 0x20];
  let at = -1;
  for (let i = 0; i < 0x2000 - 10; i++) if (pat.every((b, k) => osrom[i + k] === b) && (osrom[i + 8] === 0xC6 || osrom[i + 8] === 0x58)) { if (at >= 0) throw new Error('two'); at = i; }
  if (at < 0) throw new Error('tracer: no K_SCHED_SWITCH');
  return { pick: osrom[at + 6] | osrom[at + 7] << 8, after: 0xE000 + at + 8 };
}

let serial = 0;
const orig = mach.createMachine;
mach.createMachine = function (opt) {
  if (process.env.SCHEDBIOS) opt = Object.assign({}, opt, { osrom: fs.readFileSync(process.env.SCHEDBIOS) });
  const where = findPick(opt.osrom);
  opt = Object.assign({}, opt, { breaks: [...(opt.breaks || []), { pc: where.pick, page: 0, bank: -1, spec: 'pick' }, { pc: where.after, page: 0, bank: -1, spec: 'picked' }] });
  const m = orig(opt);
  const name = (process.env.SCHEDNAME || (process.argv.find((a, i) => i > 1 && /^[a-z]\w+$/.test(a) && a !== 'sim') || 'pid' + process.pid)) + (serial++ ? '-' + serial : '');
  const out = fs.openSync(path.join(DIR, name + '.trace'), 'w');
  let inv = 0, invs = [], pickCyc = 0, buf = [], picks = 0, bad = 0, badTick = 0, badIrq = 0, pend = null, firstBad = [];
  const flush = () => { if (buf.length) { fs.writeSync(out, buf.join('')); buf = []; } };
  const snap = () => m.taskRam.map(r => r.slice(0, 0x500));
  const run = m.run;
  let irqIn = false;
  const intr = m.cpu.interrupt.bind(m.cpu);
  m.cpu.interrupt = v => { irqIn = true; return intr(v); };
  m.run = function (limit) {
    for (;;) {
      run.call(m, limit);
      const b = m.breakHit;
      if (!b || (b.spec !== 'pick' && b.spec !== 'picked')) break;
      m.breakHit = null; m.skipBreak = true;
      const R = m.taskRam, k0 = R[0];
      if (b.spec === 'pick') {
        const Y = m.T;
        let want = null;
        if (!NOMODEL) { const V = snap(); want = { task: oldPick(V, Y), V, tick: k0[K0.TICKS] }; }
        pend = { Y, cyc: m.cpu.cyc, want }; irqIn = false;
        if (process.env.SCHEDINV) { const cand = k0[0x7D] | k0[0x7F] << 8; for (let x = 1; x < 16; x++) { const st = R[x][TK_STATE]; if (x !== Y && (st === 1 || st >= 6) && !(cand >> x & 1)) { inv++; if (invs.length < 30) invs.push('cyc ' + m.cpu.cyc + ' Y ' + Y + ': task ' + x + ' state ' + st + ' not a candidate (' + cand.toString(16) + ')'); } } }
      } else if (pend) {
        const got = m.cpu.A & 15, tick = k0[K0.TICKS] | k0[K0.TICKS + 1] << 8;
        buf.push(tick + ' ' + pend.Y.toString(16) + ' ' + got.toString(16) + '\n');
        picks++; pickCyc += m.cpu.cyc - pend.cyc;
        if (pend.want) {
          const V = pend.want.V, diffs = [];
          if (pend.want.task !== got) diffs.push('task ' + pend.want.task + ' / ' + got);
          for (const k of ['FORES', 'FTICK', 'FMODE', 'FPICK', 'RR']) if (V[0][K0[k]] !== k0[K0[k]]) diffs.push(k + ' ' + V[0][K0[k]] + ' / ' + k0[K0[k]]);
          for (let t = 1; t < 16; t++) if (V[t][TK_STATE] !== R[t][TK_STATE]) diffs.push('state' + t + ' ' + V[t][TK_STATE] + ' / ' + R[t][TK_STATE]);
          if (diffs.length) {
            if (pend.want.tick !== k0[K0.TICKS]) badTick++; else if (irqIn) badIrq++; else bad++;
            if (firstBad.length < 20 && !irqIn) firstBad.push('cyc ' + pend.cyc + ' Y ' + pend.Y + (pend.want.tick !== k0[K0.TICKS] ? ' (a tick in it)' : irqIn ? ' (an interrupt in it)' : '') + ': ' + diffs.join(', '));
          }
        }
        pend = null;
        if (buf.length > 4096) flush();
      }
    }
    flush();
    fs.writeFileSync(path.join(DIR, name + '.sum'), 'picks ' + picks + ' (' + (picks ? (pickCyc / picks).toFixed(0) : 0) + ' cycles a pick, from its call to its return)' + ', model mismatches ' + bad + ' (+' + badTick + ' with a tick inside the pick, +' + badIrq + ' with another interrupt inside)\n' + firstBad.join('\n') + '\ninvariant breaks ' + inv + '\n' + invs.join('\n') + '\n');
  };
  return m;
};
module.exports = { oldPick };
