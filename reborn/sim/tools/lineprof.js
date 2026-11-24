#!/usr/bin/env node
// ****************************************************************************
// lineprof.js - a Dingus Quest Zero line's D17 window profiled (room6's scratch profiler, made a tool by speed9,
// 2026-11-22): a test run once to find its lines typed (each Enter's cycle and its answer's first letter: the test's
// dq0Timer, tests/lib/dq0.js), then again with the machine's profile on from the chosen line's Enter to its first
// letter (the emulator's the same, cycle for cycle, each run at one build: the window's exact).  Every instruction in
// the window is counted by task (the hub's task table: the UI, the world, the denizens, the referee ...) and by C
// function, the runtime's helpers, the kernel's calls and the libraries' routines charged to the C function that
// called them (by the return addresses on the 6502's stack, up to three deep), a function in an overlay or a swapped
// part named with it ("_o_pc_side(ov3)": the bank in the task's window, its ovbank's; "_has_role(p51)": its swapped;
// which overlay or part each C function is in, from the cc65 output's segments, obj/games/TASK/*.s: speed11's); and
// each part a task copied into its swap area (swload, ../games/dq0/swap.inc) with its cycle and the cycles the copy took.
//
// Usage: node sim/tools/lineprof.js TEST [LINE] [--nth N] [--list] [--top N] [--seed N] [--out FILE] [--lead N]
//        node sim/tools/lineprof.js --sweep TEST ... [--lags L,L,...] [--jobs N] [--top N] [--seed N] [--all]
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
//   --sweep    (speed10's lag sweeps, a tool since speed11, 2026-11-22) each TEST run once at each lag (SIM_PROMPT_LAG,
//              ../base/sim/lib/acia.js: a line's first key that many cycles later after its prompt, so its lines land at
//              other phases of the background's work), the runs side by side (--list's, each its own process): every
//              typing of every line in a game (not its dq0 line, nor quit; nor a soft question's yes or no, dq0Asks',
//              unless --all) with its first letter at each lag, the --top N (default 15) by their worst; and each
//              test's sum: the typings, their mean, those past 85%, 90% and 100% of D17's 720,000, the worst
//   --lags     the lags (default 0,7000,13000,23000,31000,41000,53000,67000: speed10's eight, primes apart)
//   --jobs N   runs at once (default: the processors less one, at most the lags)
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
function segsOf(dir) {                               // (A program's C names' segments: its cc65 .s files', name -> segment)
  const seg = {};
  if (!fs.existsSync(dir)) return seg;
  for (const f of fs.readdirSync(dir).filter(x => x.endsWith('.s'))) {
    let cur = 'CODE';
    for (const l of fs.readFileSync(path.join(dir, f), 'latin1').split(/\r?\n/)) {
      let m = /^\.segment\s+"([^"]+)"/.exec(l);
      if (m) { cur = m[1]; continue; }
      m = /^\.proc\s+(_\w+)/.exec(l) || /^(_\w+):/.exec(l);
      if (m && !(m[1].slice(1) in seg)) seg[m[1].slice(1)] = cur;
    }
  }
  return seg;
}
function byPart(L, seg, swap, parts) {                // (Its labels split by where they are and their segment: the main
  const r = { main: [], ov: {}, sw: {}, names: parts.list };   //   RAM's, each overlay's, each part's, by its number:
  for (const x of L) {                                 //   parts.js's names)
    const g = seg[x[1].replace(/^_/, '')] || '', o = /^OV(\d+)/.exec(g), w = parts.of(g);
    if (x[0] >= 0x8000) { if (o) (r.ov[o[1]] = r.ov[o[1]] || []).push(x); }
    else if (x[0] >= swap) { if (w) (r.sw[w] = r.sw[w] || []).push(x); }
    else r.main.push(x);
  }
  return r;
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
  const lab = {}, swl = {}, parts = {};
  for (const p of PROGS) {
    lab[p] = labels(path.join(ROOT, 'obj', 'games', p, p + '.lbl'));
    const s = lab[p].find(x => x[1] === 'swload'); if (s) swl[p] = s[0];
    const at = n => { const x = lab[p].find(y => y[1] === n); return x ? x[0] : -1; };
    const sa = (/^al ([0-9A-F]+) \.__SWAP_START__$/m.exec(fs.existsSync(path.join(ROOT, 'obj', 'games', p, p + '.lbl')) ? fs.readFileSync(path.join(ROOT, 'obj', 'games', p, p + '.lbl'), 'latin1') : '') || [])[1];
    parts[p] = Object.assign(byPart(lab[p], segsOf(path.join(ROOT, 'obj', 'games', p)), sa ? parseInt(sa, 16) : 0x8000,
      require('../../games/dq0/parts.js').names(path.join(ROOT, 'obj', 'games', p))), { ovbank: at('_ovbank'), swapped: at('swapped'), swap: sa ? parseInt(sa, 16) : 0x8000 });
  }
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
    if (!p) return '?' + pc.toString(16);
    const q = parts[p], r = m.taskRam[T];           // (An overlay's by the bank in its window, ovbank's; a part's by swapped)
    if (pc >= 0x8000 && q.ovbank >= 0) {
      for (let n = 1; n < 64; n++) if (r[q.ovbank + n - 1] === r[0] && q.ov[n]) return near(q.ov[n], pc) + '(ov' + n + ')';
      return near(lab[p], pc) + '(win)';
    }
    if (pc >= q.swap && pc < 0x8000 && q.swapped >= 0 && q.sw[r[q.swapped]]) return near(q.sw[r[q.swapped]], pc) + '(' + (q.names[r[q.swapped] - 1] || 'p' + r[q.swapped]) + ')';
    return near(q.main.length ? q.main : lab[p], pc);
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

// ---- The lag sweep (--sweep): each test at each lag, side by side
const D17 = 720000, LAGS = [0, 7000, 13000, 23000, 31000, 41000, 53000, 67000];
function listRun(name, lag, seed) {                 // (One run's --list at one lag: [{ i, first, line }], and its failures)
  const { execFile } = require('child_process');
  return new Promise(res => execFile(process.execPath, [__filename, name, '--list', '--seed', String(seed)],
    { cwd: ROOT, maxBuffer: 1e8, env: Object.assign({}, process.env, { SIM_PROMPT_LAG: String(lag) }) }, (e, out, err) => {
      const rows = [];
      for (const l of out.split(/\r?\n/)) {
        const m = /^\s*(\d+)\s+([\d,]+|-)  (.*)$/.exec(l);
        if (m) rows.push({ i: +m[1], first: m[2] === '-' ? null : +m[2].replace(/,/g, ''), line: m[3] });
      }
      res({ lag, rows, fails: (err || '').split(/\r?\n/).filter(x => /^the test: |^lineprof: /.test(x)) });
    }));
}
async function sweep(names, opt) {
  const lags = opt.lags, jobs = Math.max(1, Math.min(opt.jobs || require('os').cpus().length - 1, lags.length * names.length));
  const todo = [];
  for (const n of names) for (const g of lags) todo.push({ n, g });
  const done = new Map();
  let next = 0;
  const worker = async () => { while (next < todo.length) { const w = todo[next++]; done.set(w.n + ' ' + w.g, await listRun(w.n, w.g, opt.seed)); } };
  await Promise.all(Array.from({ length: jobs }, worker));
  const game = l => /^(?:\S*\/)?dq0( |$)/.test(l);
  const fmt = v => v === undefined || v === null ? '-' : v.toLocaleString('en-US');
  const pct = v => Math.round(100 * v / D17) + '%';
  let failed = 0;
  for (const n of names) {
    const by = new Map();
    for (const g of lags) {
      const r = done.get(n + ' ' + g);
      for (const f of r.fails) { console.log(n + ' at lag ' + g + ': ' + f); failed = 1; }
      let inGame = false;
      for (const x of r.rows) {                     // (A line in a game: after its dq0 line, before its quit)
        if (game(x.line)) { inGame = true; continue; }
        if (!inGame) continue;
        if (x.line === 'quit') { inGame = false; continue; }
        if (!opt.all && /^(yes|no)$/.test(x.line)) continue;
        const k = x.i + ' ' + x.line;
        if (!by.has(k)) by.set(k, { i: x.i, line: x.line, v: {} });
        by.get(k).v[g] = x.first;
      }
    }
    const rows = [...by.values()].map(r => Object.assign(r, { worst: Math.max(...lags.map(g => r.v[g] || 0)) }));
    const all = [];
    for (const r of rows) for (const g of lags) if (r.v[g] !== null && r.v[g] !== undefined) all.push(r.v[g]);
    const mean = all.length ? Math.round(all.reduce((a, b) => a + b, 0) / all.length) : 0, worst = all.length ? Math.max(...all) : 0;
    console.log(n + ': ' + rows.length + ' lines, ' + all.length + ' typings at ' + lags.length + ' lags; mean ' + fmt(mean) + ', past 85% ' +
      all.filter(v => v > 0.85 * D17).length + ', past 90% ' + all.filter(v => v > 0.9 * D17).length + ', past D17 ' +
      all.filter(v => v > D17).length + ', worst ' + fmt(worst) + ' (' + pct(worst) + ')');
    console.log('    worst   line (its index, as --list numbers it)'.padEnd(48) + lags.map(g => ('lag ' + g).padStart(10)).join(''));
    for (const r of rows.sort((a, b) => b.worst - a.worst).slice(0, opt.top))
      console.log(fmt(r.worst).padStart(9) + ' ' + (String(r.i).padStart(4) + ' ' + r.line).slice(0, 38).padEnd(38) + lags.map(g => fmt(r.v[g]).padStart(10)).join(''));
  }
  return failed;
}

async function main(argv) {
  const opt = { seed: 1, top: 0, nth: 0, list: false, out: '', lead: 0, timeline: 0, names: [], sweep: false, lags: LAGS, jobs: 0, all: false };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--sweep') { opt.sweep = true; continue; }
    if (argv[i] === '--lags') { opt.lags = argv[++i].split(',').map(Number); continue; }
    if (argv[i] === '--jobs') { opt.jobs = +argv[++i]; continue; }
    if (argv[i] === '--all') { opt.all = true; continue; }
    if (argv[i] === '--nth') opt.nth = +argv[++i];
    else if (argv[i] === '--list') opt.list = true;
    else if (argv[i] === '--top') opt.top = +argv[++i];
    else if (argv[i] === '--seed') opt.seed = +argv[++i];
    else if (argv[i] === '--out') opt.out = argv[++i];
    else if (argv[i] === '--lead') opt.lead = +argv[++i];
    else if (argv[i] === '--timeline') opt.timeline = +argv[++i];
    else if (argv[i].startsWith('-')) { console.error('usage: node sim/tools/lineprof.js TEST [LINE] [--nth N] [--list] [--top N] [--seed N] [--out FILE] [--lead N] [--timeline N]\n' +
      '       node sim/tools/lineprof.js --sweep TEST ... [--lags L,L,...] [--jobs N] [--top N] [--seed N] [--all]'); process.exit(2); }
    else opt.names.push(argv[i]);
  }
  if (opt.sweep) {
    const T = require('../test.js');
    for (const n of opt.names) if (!T.tests.find(x => x.name === n)) { console.error('lineprof: no test ' + n); process.exit(2); }
    opt.top = opt.top || 15;
    process.exit(await sweep(opt.names, opt));
  }
  opt.top = opt.top || 60;
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
