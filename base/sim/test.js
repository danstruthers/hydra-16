#!/usr/bin/env node
// ****************************************************************************
// test.js - the regression tests, in the emulator: the base's (tests/tests.js), or a system's on the base (HydraOS's:
// reborn/sim/test.js runs this with its own, the base's first).  Each test boots its own paged ROM image (the
// system's modules and its own, its init), runs, and is judged on its output ("ok"/"not ok" lines and "PASS"), its
// time budgets (cycles between marks), the longest IRQs-off stretch after the boot, and its own checks.  A test of the
// base's (system: 'base') boots the base's image: modules/rom.txt's modules (ser, wozmon), less its without, and its
// own (tests/mod's: obj/tests), its init.
//
// Usage: node sim/test.js [NAME ...] [--build] [-v] [--seed N] [-j N] [--dl]
//   NAME      only these tests (default: all)
//   --build   build first (node build.js)
//   -v        each test's output, and its "ok" lines
//   --seed N  the power-up's random RAM (default 1: the same each run)
//   -j N      N tests at a time, each in a process of its own (default: the CPU's cores; -j 1, one after another
//             here).  The reports come in the list's order either way
//   --dl      in the danlang emulator (HydraOS's: reborn/sim/dl), not sim/lib's; judged the same way.  A test marked
//             jsOnly (its reason: the sound, which sim/dl doesn't make) is skipped there, and said so (SKIP)
// From Node: setup(env) gives the runner a system: { root (its folder: obj/build.json's options), tests, IRQ_OFF_MAX,
// image(t) (its own; the base's tests still the base's), build(), pcFolder(t, opt), runDl(t, machine, opt, extra),
// self (the file a test's process runs: its --one) }; then main(argv).
// The emulator runs as fast as the host can, never paced to the Hydra's clock (run.js -i is): the cycles a test
// reports, and its budgets, are the emulated machine's.
'use strict';
const fs = require('fs');
const path = require('path');
const { boot, labels } = require('./run.js');
const romimg = require('../tools/romimg.js');
const { readManifest, hwtest } = require('../build.js');

const BASE = path.join(__dirname, '..');
// The system the tests are run for (setup): the base's own, unless a system on it says otherwise
const env = { root: BASE, tests: null, IRQ_OFF_MAX: 200, image: null, build: null, pcFolder: null, runDl: null, self: __filename };
function setup(e) { Object.assign(env, e); }
const baseTests = () => require('../tests/tests.js').tests.map(t => Object.assign(t, { system: 'base' }));

// The options the image was built with (build.js: obj/build.json)
function built() {
  try { return JSON.parse(fs.readFileSync(path.join(env.root, 'obj', 'build.json'), 'utf8')); }
  catch (e) { return { clock: 1, acia: 'rockwell' }; }
}

const bin = (dir, n) => fs.readFileSync(path.join(BASE, 'obj', dir, n + '.bin'));

// A base test's image: the base's modules (modules/rom.txt's, less its without), its own (obj/tests, or the base's
// obj/modules), its init.  A system's test: the system's image(t)
function baseImage(t) {
  const sys = readManifest(path.join(BASE, 'modules', 'rom.txt')).modules.filter(n => !(t.without || []).includes(n));
  const own = [...(t.modules || [])];
  if (!sys.includes(t.init) && !own.includes(t.init)) own.unshift(t.init);
  const own_ = n => bin(fs.existsSync(path.join(BASE, 'obj', 'tests', n + '.bin')) ? 'tests' : 'modules', n);
  const mods = [...sys.map(n => ({ file: n, data: bin('modules', n) })), ...own.map(n => ({ file: n, data: own_(n) }))];
  return romimg.build({ modules: mods, init: t.init, hwtest: hwtest(), bios: fs.readFileSync(path.join(BASE, 'bin', 'bios.bin')) }).image;
}
const image = t => t.system === 'base' || !env.image ? baseImage(t) : env.image(t);

// A test's marks: its budgets' and its send's
const markNamesOf = t => [...new Set([...(t.budgets || []).flatMap(b => [b.from, b.to, ...(b.minus || [])]), ...(t.send ? [t.send.after] : [])])];

function runTest(t, opt) {
  const bootDone = labels().byName.get('BOOT_DONE');          // (The boot's cli: IRQs-off stretches count from it)
  const marks = {};
  const markNames = markNamesOf(t);
  let m = null;
  const log = s => {
    const p = s.match(/^pc: .* at cycle (\d+)$/);
    if (p) { if (m.acia.typedAt < 0) m.acia.typedAt = +p[1]; return; }
    const k = s.match(/^mark: (".*") at cycle (\d+)$/);
    if (!k) { if (opt.verbose) console.log('  [sim] ' + s); return; }
    const name = JSON.parse(k[1]), at = +k[2];
    if (!(name in marks)) marks[name] = at;
    if (t.send && name === t.send.after) m.acia.send(t.send.bytes);
  };
  const pc = t.pc ? env.pcFolder(t, opt) : null;
  m = boot(Object.assign({ prom: image(t), seed: opt.seed, marks: markNames, log, trace: 0,
    pcWatches: bootDone === undefined ? [] : [{ pc: bootDone, page: 0 }] }, t.machine || {}, pc ? { pcHost: pc.host } : {}));
  m.pc = pc;
  if (t.start) t.start(m);
  const done = new RegExp('^' + t.init + ': (PASS|FAIL)', 'm');
  const target = t.expect ? null : done;
  let status = '';
  while (m.cpu.cyc < t.cycles && !m.cpu.halted) {
    m.run(Math.min(t.cycles, m.cpu.cyc + 200000));
    const out = m.out.replace(/\r/g, '');
    if (target) { const r = out.match(target); if (r) { status = r[1]; break; } }
    else if (t.expect.every(e => out.includes(e))) { status = 'PASS'; break; }
  }
  return judge(t, m, marks, pc, status);
}

// Test t run in the danlang emulator (sim/dl: bridge.js runs it), judged as runTest's is.  OUT: (a promise) runTest's
async function runTestDl(t, opt) {
  const bootDone = labels().byName.get('BOOT_DONE');
  const pc = t.pc ? env.pcFolder(t, opt) : null;
  const machine = t.machine || {};                            // (A getter's, once: its cards, its peer)
  if (!env.runDl) throw new Error('--dl: the danlang emulator is HydraOS\'s (reborn/sim/test.js)');
  const { m, marks } = await env.runDl(t, machine, opt,
    { image: image(t), marks: markNamesOf(t), bootDone, peer: pc ? pc.host : machine.pcHost || null });
  m.pc = pc;
  const out = m.out.replace(/\r/g, ''), r = out.match(new RegExp('^' + t.init + ': (PASS|FAIL)', 'm'));
  const status = t.expect ? (t.expect.every(e => out.includes(e)) ? 'PASS' : '') : r ? r[1] : '';
  return judge(t, m, marks, pc, status);
}

// A test's run judged (m the machine afterwards, marks the cycles of its marks, status PASS, FAIL or '' as its result
// came out): its "ok" lines, its result, a halt, its budgets, the longest IRQs-off stretch, its own checks.
// OUT: { m, out, lines, failures, budgets, notes }
function judge(t, m, marks, pc, status) {
  const failures = [], lines = [];
  const out = m.out.replace(/\r/g, '');
  for (const line of out.split('\n')) if (/^(not )?ok - /.test(line)) lines.push(line);
  for (const l of lines) if (l.startsWith('not ok')) failures.push(l);
  if (m.cpu.halted) failures.push('the CPU halted: ' + m.cpu.halted);
  if (!status) failures.push(t.expect ? 'missing: ' + t.expect.filter(e => !out.includes(e)).map(e => JSON.stringify(e)).join(', ')
    : 'no result in ' + t.cycles + ' cycles');
  else if (status === 'FAIL' && !failures.length) failures.push(t.init + ': FAIL');
  const budgets = [], options = built();
  for (const b of t.budgets || []) {
    if (![b.from, b.to, ...(b.minus || [])].every(k => k in marks)) { failures.push('budget ' + b.what + ': no marks'); continue; }
    const v = (marks[b.to] - marks[b.from] - (b.minus ? marks[b.minus[1]] - marks[b.minus[0]] : 0)) / b.per;
    const max = typeof b.max === 'function' ? b.max(options) : b.max;
    budgets.push({ what: b.what, value: v, max });
    if (v > max) failures.push(b.what + ': ' + v.toFixed(1) + ' cycles (budget ' + max + ')');
  }
  const ioff = m.iOffTop[0];
  if (ioff) budgets.push({ what: 'longest IRQs-off stretch (' + ioff[1] + ' - ' + ioff[2] + ')', value: ioff[0], max: env.IRQ_OFF_MAX, total: true });
  if (ioff && ioff[0] > env.IRQ_OFF_MAX) failures.push('IRQs off for ' + ioff[0] + ' cycles at ' + ioff[1] + ' - ' + ioff[2] + ' (budget ' + env.IRQ_OFF_MAX + ')');
  if (t.check) failures.push(...t.check(m, out));
  const notes = [...(t.notes || []), ...(pc ? [pc.host.report() + '; ' + m.acia.pcLost + ' reply byte(s) lost'] : [])];
  return { m, out, lines, failures, budgets, notes };
}

// A test's report: its result, its budgets, its notes and failures, and its output (with -v, or when it failed).
// OUT: { ok, text }
function report(t, r, opt, lbl) {
  const ok = !r.failures.length, out = [];
  out.push((ok ? 'PASS ' : 'FAIL ') + (t.name + ' ').padEnd(8) + t.what + '  (' + r.lines.length + ' checks, ' + (r.m.cpu.cyc / 1e6).toFixed(1) + 'M cycles)');
  if (opt.verbose) out.push(r.out.split('\n').map(l => '    | ' + l).join('\n'));
  for (const b of r.budgets) {
    const at = b.what.replace(/(\w):([0-9A-F]{4})/g, (_, w, pc) => w + ':' + pc + ' ' + lbl.at(parseInt(pc, 16), parseInt(w, 16)));
    out.push('       ' + (b.value > b.max ? 'OVER ' : '     ') + at + ': ' + (Number.isInteger(b.value) ? b.value : b.value.toFixed(1)) + ' cycles (budget ' + b.max + ')');
  }
  for (const n of r.notes) out.push('       ' + n);
  for (const f of r.failures) out.push('       ! ' + f);
  if (!ok && !opt.verbose) {
    out.push('       the output\'s end:');
    out.push(r.out.split('\n').slice(-12).map(l => '    | ' + l).join('\n'));
  }
  return { ok, text: out.join('\n') };
}

// The tests, opt.jobs at a time, each in a process of its own (this file, with --one NAME), the longest (by its
// cycles) started first; each report shown in the list's order once it and those before it are in.  OUT: (a promise)
// how many failed
function parallel(list, opt) {
  const { spawn } = require('child_process');
  const order = list.map((t, i) => i).sort((x, y) => list[y].cycles - list[x].cycles);
  const done = new Array(list.length).fill(null);
  let next = 0, shown = 0, running = 0, failed = 0;
  return new Promise(resolve => {
    const show = () => {
      for (; shown < list.length && done[shown]; shown++) { console.log(done[shown].text); if (!done[shown].ok) failed++; }
      if (shown === list.length) resolve(failed);
    };
    const start = () => {
      for (; running < opt.jobs && next < order.length; next++) {
        const i = order[next], t = list[i], args = [env.self, '--one', t.name, '--seed', String(opt.seed)];
        if (opt.verbose) args.push('-v');
        if (opt.dl) args.push('--dl');
        const p = spawn(process.execPath, args, { stdio: ['ignore', 'pipe', 'inherit'] });
        let text = '';
        running++;
        p.stdout.setEncoding('utf8');
        p.stdout.on('data', d => { text += d; });
        p.on('close', code => {                                 // (A process that ended without its report: a FAIL)
          running--;
          text = text.replace(/\r?\n$/, '');
          if (!/^(PASS|FAIL) /.test(text)) text = 'FAIL ' + (t.name + ' ').padEnd(8) + t.what + '\n       ! its process ended (code ' + code + ')' + (text ? '\n' + text : '');
          done[i] = { ok: code === 0 && text.startsWith('PASS '), text };
          show();
          start();
        });
      }
    };
    start();
  });
}

async function main(argv) {
  const tests = [...baseTests(), ...(env.tests || [])];
  const opt = { seed: 1, verbose: false, names: [], jobs: require('os').cpus().length, one: null };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '-v') opt.verbose = true;
    else if (argv[i] === '--seed') opt.seed = +argv[++i];
    else if (argv[i] === '--build') opt.build = true;
    else if (argv[i] === '-j') opt.jobs = Math.max(1, Math.floor(+argv[++i]) || 1);
    else if (argv[i] === '--one') opt.one = argv[++i];        // (parallel's: one test, its report, its status)
    else if (argv[i] === '--dl') opt.dl = true;
    else if (argv[i].startsWith('-')) { console.error('usage: node sim/test.js [NAME ...] [--build] [-v] [--seed N] [-j N] [--dl]'); process.exit(2); }
    else opt.names.push(argv[i]);
  }
  if (opt.one) {
    const t = tests.find(x => x.name === opt.one), r = report(t, await (opt.dl ? runTestDl : runTest)(t, opt), opt, labels());
    console.log(r.text);
    process.exit(r.ok ? 0 : 1);
  }
  if (opt.build) (env.build || (() => require('../build.js').build({ quiet: true })))();
  const named = opt.names.length ? tests.filter(t => opt.names.includes(t.name)) : tests;
  if (!named.length) { console.error('no such test: ' + opt.names.join(' ')); process.exit(2); }
  const list = named.filter(t => !(opt.dl && t.jsOnly));
  for (const t of named) if (!list.includes(t)) console.log('SKIP ' + (t.name + ' ').padEnd(8) + t.what + '  (' + t.jsOnly + ')');
  let failed = 0;
  if (opt.jobs > 1 && list.length > 1) failed = await parallel(list, opt);
  else {
    const lbl = labels();
    for (const t of list) {
      const r = report(t, await (opt.dl ? runTestDl : runTest)(t, opt), opt, lbl);
      console.log(r.text);
      if (!r.ok) failed++;
    }
  }
  console.log(failed ? failed + ' of ' + list.length + ' tests failed' : 'all ' + list.length + ' tests passed');
  process.exit(failed ? 1 : 0);
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { setup, main, runTest, runTestDl, image, baseImage };
