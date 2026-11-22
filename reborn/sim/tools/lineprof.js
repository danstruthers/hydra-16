#!/usr/bin/env node
// ****************************************************************************
// lineprof.js - a Dingus Quest Zero line's D17 window profiled (room6's scratch profiler, made a tool by speed9,
// 2026-11-22): a test run once to find its lines typed (each Enter's cycle and its answer's first letter: the test's
// dq0Timer, tests/lib/dq0.js), then again with the machine's profile on from the chosen line's Enter to its first
// letter (the emulator's the same, cycle for cycle, each run at one build: the window's exact).  Every instruction in
// the window is counted by task (the hub's task table: the UI, the world, the denizens, the referee ...) and by C
// function, the runtime's helpers, the kernel's calls and the libraries' routines charged to the C function that
// called them (by the return addresses on the 6502's stack, up to three deep); and each part a task copied into its
// swap area (swload, ../games/dq0/swap.inc) with its cycle and the cycles the copy took.
//
// Usage: node sim/tools/lineprof.js TEST [LINE] [--nth N] [--list] [--top N] [--seed N] [--out FILE] [--lead N]
//   TEST       a test that times its lines (a D17 test: this.timer = dq0Timer(m), tests/suites/dq0/*.js)
//   LINE       the line typed, as the test types it (its first letters enough: "go left" for MID_MARK's "go left";
//              "d$" that line alone);
//              without it, the slowest first letter in the run
//   --nth N    its Nth typing in the run (1 on: the test's own game, then dq0Paces' menacing and played); default: the
//              slowest of them
//   --list     each line typed and its first letter, no profile
//   --top N    the N functions and routines that took the most (default 60)
//   --seed N   the runner's --seed (default 1)
//   --out FILE the profile written there too
//   --timeline N  each task's run in the window of N cycles or more, in order (who ran when)
//   --lead N   the profile begun N cycles before the Enter (what came just before it: a part copied in, a round's
//              work begun); its cycles in the counts, its part loads at their (negative) cycles
// The run's other checks are the test's as ever (its failures said, not the tool's): GUARDS, HUBOWN and TRANSCRIPTS
// as sim/test.js has them.
'use strict';
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..', '..'), BASE = path.join(ROOT, '..', 'base');
const PROGS = ['dq0', 'dq0w', 'dq0d', 'dq0harn', 'dq0s', 'dq0m', 'dq0p', 'dq0c'];   // (hub.h's T_UI ... T_COMMANDS)
const CONS_T = 15;                                     // (The console's task: its module's code in the window)

function labels(file) {                               // (A program's labels: [addr, name], by address)
  const out = [];
  if (!fs.existsSync(file)) return out;
  for (const l of fs.readFileSync(file, 'latin1').split(/\r?\n/)) {
    const m = /^al ([0-9A-F]+) \.(\S+)/.exec(l);
    if (m && !/^(__|@|L[0-9A-F]+$)/.test(m[2])) out.push([parseInt(m[1], 16), m[2]]);
  }
  return out.sort((x, y) => x[0] - y[0]);
}
function near(L, a) {                                 // (The label at or below a)
  let lo = 0, hi = L.length - 1, best = null;
  while (lo <= hi) { const mid = (lo + hi) >> 1; if (L[mid][0] <= a) { best = L[mid]; lo = mid + 1; } else hi = mid - 1; }
  return best ? best[1] : '?' + a.toString(16);
}
function dbgBanks(file, by) {                         // (ld65's debug file's labels by bank: by(seg line) its bank)
  const banks = [], bank = {};
  if (!fs.existsSync(file)) return banks;
  for (const l of fs.readFileSync(file, 'latin1').split(/\r?\n/)) {
    let m = /^seg\tid=(\d+),(.*)/.exec(l);
    if (m) { const b = by(m[2]); if (b !== null) bank[m[1]] = b; continue; }
    m = /^sym\tid=\d+,name="([^"]+)",(?!.*parent=).*val=0x([0-9A-F]+),seg=(\d+),type=lab/.exec(l);
    if (m && bank[m[3]] !== undefined) (banks[bank[m[3]]] = banks[bank[m[3]]] || []).push([parseInt(m[2], 16), m[1]]);
  }
  for (const p of banks) if (p) p.sort((x, y) => x[0] - y[0]);
  return banks;
}

// The profiler's part of a run (sim/test.js's addInstrument): the machine's profile from..to, each instruction's
// task, function and its callers counted, and the swap areas' loads.  OUT: { part(t), result() }
function profiler(name, from, to) {
  let m = null;
  const self = new Map(), calls = new Map(), loads = [], byTask = new Array(16).fill(0);
  let total = 0, run = null;
  const last = [], runs = [];
  const lab = {}, swl = {};
  for (const p of PROGS) { lab[p] = labels(path.join(ROOT, 'obj', 'games', p, p + '.lbl')); const s = lab[p].find(x => x[1] === 'swload'); if (s) swl[p] = s[0]; }
  const bios = dbgBanks(path.join(BASE, 'obj', 'kernel', 'bios.dbg'), s => { const b = /bank=(\d+)/.exec(s); return b ? +b[1] : null; });
  const mod = n => dbgBanks(path.join(ROOT, 'obj', 'modules', n, n + '.dbg'), s => { const o = /ooffs=(\d+)/.exec(s); return o ? Math.floor(+o[1] / 16384) : null; });
  const cons = mod('cons'), lib = mod('dq0lib');
  let task = null, b0 = null;
  const taskOf = () => {                              // (The hub's task table: each task's number, its program)
    if (task) return task;
    const h = m.dq0 && m.dq0.bank(0);
    if (!h || !h[12]) return {};
    task = {};
    PROGS.forEach((p, k) => { if (h[12 + k]) task[h[12 + k]] = p; });
    return task;
  };
  const fnOf = (T, W, pc, bk) => {
    if (pc >= 0xE000) return 'bios' + W + ':' + near(bios[W] || [], pc);
    if (pc >= 0xA000) return T === CONS_T ? 'cons:' + near(cons[bk - b0] || [], pc) : 'lib:' + near(lib[0] || [], pc);
    const p = taskOf()[T];
    return p ? near(lab[p], pc) + (pc >= 0x8000 ? '(win)' : '') : '?' + pc.toString(16);
  };
  const step = (T, W, pc, dc, c0) => {
    total += dc;
    byTask[T] += dc;
    const st = m.taskRam[T], sp = m.cpu.S, ret = k => st[0x100 + ((sp + k) & 0xFF)] | st[0x100 + ((sp + k + 1) & 0xFF)] << 8;
    const bk = st[1];
    if (T === CONS_T && pc >= 0xA000 && pc < 0xE000 && (b0 === null || bk < b0)) b0 = bk;   // (The console's first bank)
    const own = fnOf(T, W, pc, bk);
    let fn = own;
    for (const k of [1, 3, 5]) { if (/^(_|bios|lib|cons|swload$|sw_call$)/.test(fn)) break; fn = fnOf(T, W, ret(k), bk); }   // (A helper's: its C caller's; a part's copy its own)
    const p = taskOf()[T], key = 'T' + T + (p ? '/' + p : '') + ' ' + fn;
    if (!run || run.T !== T) { run = { T, at: c0, cycles: 0, fns: new Map() }; runs.push(run); }   // (The timeline: a task's run)
    run.cycles += dc;
    run.fns.set(fn, (run.fns.get(fn) || 0) + dc);
    self.set(key, (self.get(key) || 0) + dc);
    const ck = 'T' + T + ' ' + own + ' <- ' + fnOf(T, W, ret(1), bk) + ' <- ' + fnOf(T, W, ret(3), bk);
    calls.set(ck, (calls.get(ck) || 0) + dc);
    if (p && swl[p] === pc) {                         // (A load: sw_call's, a stub's call, or sw_back's, a return; and
      last[T] = { at: c0, p, part: m.cpu.Y, cycles: 0, via: fnOf(T, W, ret(1), bk), by: fnOf(T, W, ret(3), bk) };   //   for whom)
      loads.push(last[T]);
    }
    if (own === 'swload' && last[T]) last[T].cycles += dc;   // (A load's cycles: swload's own, its task's last load's)
  };
  const part = t => t.name === name ? {
    boot: { profile: from, profileTo: to, profileStep: step }, attach: mm => { m = mm; }, judge: () => ({ failures: [], notes: [] }),
    state: () => null, restore: () => {},
  } : null;
  const result = () => ({ total, byTask, task: taskOf(), self, calls, loads, runs });
  return { part, result };
}

function report(r, line, top, tl) {
  const out = [];
  if (tl) {                                           // (--timeline N: each task's run of N cycles or more, in order,
    out.push('-- timeline (cycle in the window, task, its run cycles, its top functions)');   //   its top functions)
    let rest = 0;
    for (const u of r.runs) {
      if (u.cycles < tl) { rest += u.cycles; continue; }
      const fs_ = [...u.fns].sort((a, b) => b[1] - a[1]).slice(0, 3).map(([f, c]) => f + ' ' + c).join(', ');
      out.push(String(u.at - line.t).padStart(9) + ' T' + u.T + ' ' + (r.task[u.T] || (u.T === CONS_T ? 'cons' : '')).padEnd(7) + String(u.cycles).padStart(7) + '  ' + fs_ + (rest ? '  (+' + rest + ' in short runs before)' : ''));
      rest = 0;
    }
  }
  out.push('window ' + line.first.toLocaleString('en-US') + ' cycles, "' + line.line + '" (the Enter at cycle ' + line.t + ')');
  out.push('-- by task');
  r.byTask.forEach((c, T) => { if (c) out.push(String(c).padStart(9) + ' T' + T + ' ' + (r.task[T] || (T === CONS_T ? 'cons' : ''))); });
  out.push('-- parts copied in (cycle in the window, task, part, the copy\'s cycles; sw_call\'s or sw_back\'s, for whom)');
  for (const l of r.loads) out.push(String(l.at - line.t).padStart(9) + ' ' + l.p + ' part ' + l.part + ' ' + l.cycles + '  (' + l.via + ', ' + l.by + ')');
  out.push('-- by C function (its helpers charged to it)');
  for (const [k, c] of [...r.self].sort((a, b) => b[1] - a[1]).slice(0, top)) out.push(String(c).padStart(9) + ' ' + k);
  out.push('-- by routine <- its caller <- its caller');
  for (const [k, c] of [...r.calls].sort((a, b) => b[1] - a[1]).slice(0, top)) out.push(String(c).padStart(9) + ' ' + k);
  return out.join('\n');
}

async function main(argv) {
  const opt = { seed: 1, top: 60, nth: 0, list: false, out: '', lead: 0, timeline: 0, names: [] };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--nth') opt.nth = +argv[++i];
    else if (argv[i] === '--list') opt.list = true;
    else if (argv[i] === '--top') opt.top = +argv[++i];
    else if (argv[i] === '--seed') opt.seed = +argv[++i];
    else if (argv[i] === '--out') opt.out = argv[++i];
    else if (argv[i] === '--lead') opt.lead = +argv[++i];
    else if (argv[i] === '--timeline') opt.timeline = +argv[++i];
    else if (argv[i].startsWith('-')) { console.error('usage: node sim/tools/lineprof.js TEST [LINE] [--nth N] [--list] [--top N] [--seed N] [--out FILE] [--lead N]'); process.exit(2); }
    else opt.names.push(argv[i]);
  }
  const [name, want] = opt.names;
  const T = require('../test.js');
  const t = T.tests.find(x => x.name === name);
  if (!t) { console.error('lineprof: no test ' + name); process.exit(2); }
  const ro = { seed: opt.seed, verbose: false };
  const r1 = await T.runTest(t, ro);                  // (The lines: each Enter, each first letter)
  if (!t.timer) { console.error('lineprof: ' + name + ' times no lines (no dq0Timer)'); process.exit(2); }
  const strip = s => s.replace(/^[^\x20-\x7e]+/, '');
  const lines = t.timer.enters.map((e, i) => Object.assign({ i, line: strip(e.line), t: e.t }, { first: t.timer.at(i).first }));
  if (opt.list) {
    for (const l of lines) console.log(String(l.i).padStart(4) + ' ' + (l.first === null ? '-' : l.first.toLocaleString('en-US')).padStart(9) + '  ' + l.line);
    process.exit(r1.failures.length ? 1 : 0);
  }
  let pick = lines.filter(l => l.first !== null && (!want || (want.endsWith('$') ? l.line === want.slice(0, -1) : l.line.startsWith(want))));
  if (opt.nth) pick = pick.slice(opt.nth - 1, opt.nth);
  if (!pick.length) { console.error('lineprof: no line ' + JSON.stringify(want || '') + ' timed in ' + name); process.exit(2); }
  const line = pick.reduce((a, b) => b.first > a.first ? b : a);
  const P = profiler(name, line.t - opt.lead, line.t + line.first);
  T.addInstrument(P.part);
  const r2 = await T.runTest(t, ro);
  const again = t.timer.at(line.i).first;
  if (again !== line.first) console.error('lineprof: the second run\'s first letter ' + again + ', not ' + line.first + ' (the window not the same)');
  const text = report(P.result(), line, opt.top, opt.timeline);
  console.log(text);
  if (opt.out) fs.writeFileSync(opt.out, text + '\n');
  for (const f of [...r1.failures, ...r2.failures].slice(0, 5)) console.error('the test: ' + f);
}

if (require.main === module) main(process.argv.slice(2)).catch(e => { console.error(e.stack || e.message); process.exit(1); });
module.exports = { profiler, report };
