#!/usr/bin/env node
// ****************************************************************************
// test.js - the regression tests (tests/tests.js), in the emulator: each test boots its own paged ROM image (the
// system's modules and its own, its init), runs, and is judged on its output ("ok"/"not ok" lines and "PASS"),
// its time budgets (cycles between marks), the longest IRQs-off stretch after the boot, and its own checks.
//
// Usage: node sim/test.js [NAME ...] [--build] [-v] [--seed N]
//   NAME      only these tests (default: all)
//   --build   build first (node build.js)
//   -v        each test's output, and its "ok" lines
//   --seed N  the power-up's random RAM (default 1: the same each run)
'use strict';
const fs = require('fs');
const path = require('path');
const { boot, labels } = require('./run.js');
const { createPcHost } = require('./lib/pchost.js');
const romimg = require('../tools/romimg.js');
const romfs = require('../tools/romfs.js');
const { readManifest, hwtest } = require('../build.js');
const { tests, IRQ_OFF_MAX } = require('../tests/tests.js');

// The options the image was built with (build.js: obj/build.json)
function built() {
  try { return JSON.parse(fs.readFileSync(path.join(__dirname, '..', 'obj', 'build.json'), 'utf8')); }
  catch (e) { return { clock: 1, acia: 'rockwell' }; }
}

const ROOT = path.join(__dirname, '..');
const bin = (dir, n) => fs.readFileSync(path.join(ROOT, 'obj', dir, n + '.bin'));

function image(t) {
  const sys = readManifest(path.join(ROOT, 'modules', 'rom.txt')).modules.filter(n => !(t.without || []).includes(n));
  const own = [...(t.modules || [])];
  if (!sys.includes(t.init) && !own.includes(t.init)) own.unshift(t.init);
  const own_ = n => bin(['tests', 'samples'].find(d => fs.existsSync(path.join(ROOT, 'obj', d, n + '.bin'))) || 'modules', n);   // (Or the SDK's, or a module the system's ROM hasn't: hylang)
  const mods = [...sys.map(n => ({ file: n, data: bin('modules', n) })), ...own.map(n => ({ file: n, data: own_(n) }))];
  return romimg.build({ modules: mods, init: t.init, hwtest: hwtest(), bios: fs.readFileSync(path.join(ROOT, 'bin', 'bios.bin')),
    romfs: romfs.manifest(path.join(ROOT, 'romfs', 'romfs.txt')) }).image;
}

// A test's PC folder (t.pc: { files: { name: text or bytes (or a function giving them), 'dir/': '' } (or a function
// giving that), readOnly, damage }), made afresh in
// obj/pc/NAME, its files stamped 2026-10-03 15:04:05; and the PC tool for it (sim/lib/pchost.js).  OUT: { dir, host }
function pcFolder(t, opt) {
  const dir = path.join(ROOT, 'obj', 'pc', t.name), when = new Date(2026, 9, 3, 15, 4, 5);
  fs.rmSync(dir, { recursive: true, force: true });
  fs.mkdirSync(dir, { recursive: true });
  const files = typeof t.pc.files === 'function' ? t.pc.files() : t.pc.files || {};
  for (const [n, data] of Object.entries(files)) {
    const f = path.join(dir, n);
    if (n.endsWith('/')) fs.mkdirSync(f, { recursive: true });
    else { fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, typeof data === 'function' ? data() : data); }
  }
  const stamp = d => { for (const n of fs.readdirSync(d)) { const f = path.join(d, n); if (fs.statSync(f).isDirectory()) stamp(f); fs.utimesSync(f, when, when); } };
  stamp(dir);
  const host = createPcHost({ dir, readOnly: !!t.pc.readOnly, damage: t.pc.damage || [],
    log: opt.verbose ? s => console.log('  [pc] ' + s) : undefined });
  return { dir, host };
}

function runTest(t, opt) {
  const bootDone = labels().byName.get('BOOT_DONE');          // (The boot's cli: IRQs-off stretches count from it)
  const marks = {}, failures = [], lines = [];
  const markNames = [...new Set([...(t.budgets || []).flatMap(b => [b.from, b.to, ...(b.minus || [])]), ...(t.send ? [t.send.after] : [])])];
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
  const pc = t.pc ? pcFolder(t, opt) : null;
  m = boot(Object.assign({ prom: image(t), seed: opt.seed, marks: markNames, log, trace: 40,
    pcWatches: bootDone === undefined ? [] : [{ pc: bootDone, page: 0 }] }, t.machine || {}, pc ? { pcHost: pc.host } : {}));
  m.pc = pc;
  const done = new RegExp('^' + t.init + ': (PASS|FAIL)', 'm');
  const target = t.expect ? null : done;
  let status = '';
  while (m.cpu.cyc < t.cycles && !m.cpu.halted) {
    m.run(Math.min(t.cycles, m.cpu.cyc + 200000));
    const out = m.out.replace(/\r/g, '');
    if (target) { const r = out.match(target); if (r) { status = r[1]; break; } }
    else if (t.expect.every(e => out.includes(e))) { status = 'PASS'; break; }
  }
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
  if (ioff) budgets.push({ what: 'longest IRQs-off stretch (' + ioff[1] + ' - ' + ioff[2] + ')', value: ioff[0], max: IRQ_OFF_MAX, total: true });
  if (ioff && ioff[0] > IRQ_OFF_MAX) failures.push('IRQs off for ' + ioff[0] + ' cycles at ' + ioff[1] + ' - ' + ioff[2] + ' (budget ' + IRQ_OFF_MAX + ')');
  if (t.check) failures.push(...t.check(m, out));
  const notes = [...(t.notes || []), ...(pc ? [pc.host.report() + '; ' + m.acia.pcLost + ' reply byte(s) lost'] : [])];
  return { m, out, lines, failures, budgets, notes };
}

function main(argv) {
  const opt = { seed: 1, verbose: false, names: [] };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '-v') opt.verbose = true;
    else if (argv[i] === '--seed') opt.seed = +argv[++i];
    else if (argv[i] === '--build') opt.build = true;
    else if (argv[i].startsWith('-')) { console.error('usage: node sim/test.js [NAME ...] [--build] [-v] [--seed N]'); process.exit(2); }
    else opt.names.push(argv[i]);
  }
  if (opt.build) require('../build.js').build({ quiet: true });
  const list = opt.names.length ? tests.filter(t => opt.names.includes(t.name)) : tests;
  if (!list.length) { console.error('no such test: ' + opt.names.join(' ')); process.exit(2); }
  const lbl = labels();
  let failed = 0;
  for (const t of list) {
    const r = runTest(t, opt);
    const ok = !r.failures.length;
    if (!ok) failed++;
    console.log((ok ? 'PASS ' : 'FAIL ') + t.name.padEnd(8) + t.what + '  (' + r.lines.length + ' checks, ' + (r.m.cpu.cyc / 1e6).toFixed(1) + 'M cycles)');
    if (opt.verbose) console.log(r.out.split('\n').map(l => '    | ' + l).join('\n'));
    for (const b of r.budgets) {
      const at = b.what.replace(/(\w):([0-9A-F]{4})/g, (_, w, pc) => w + ':' + pc + ' ' + lbl.at(parseInt(pc, 16), parseInt(w, 16)));
      console.log('       ' + (b.value > b.max ? 'OVER ' : '     ') + at + ': ' + (Number.isInteger(b.value) ? b.value : b.value.toFixed(1)) + ' cycles (budget ' + b.max + ')');
    }
    for (const n of r.notes) console.log('       ' + n);
    for (const f of r.failures) console.log('       ! ' + f);
    if (!ok && !opt.verbose) {
      console.log('       the output\'s end:');
      console.log(r.out.split('\n').slice(-12).map(l => '    | ' + l).join('\n'));
    }
  }
  console.log(failed ? failed + ' of ' + list.length + ' tests failed' : 'all ' + list.length + ' tests passed');
  process.exit(failed ? 1 : 0);
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { runTest, image };
