#!/usr/bin/env node
// ****************************************************************************
// regress.js - the OS ROM's regression tests: boots the ROM images in hydrasim.js with typed input, and
// checks the serial output (and the emulator's report) for what each test expects.
//
// Usage: node regress.js [options] [NAME ...]
//   NAME                Run only the tests whose names contain NAME (default: all of them)
//   --rom DIR           ROM images directory (default: hydrasim.js's, ../os_rom/bin)
//   --seed N            Power-up random number seed for every run (default 1: runs repeat exactly)
//   --random            A new random power-up each run (finds code that depends on uninitialised RAM)
//   --jobs N            Emulators run at once (default: the number of CPUs)
//   --verbose           Show each test's serial output, not just the failing ones'
//   --sim "ARGS"        More options for every emulator run (e.g. --sim "--acia wdc" for a ROM built for it)
//   --cycles-scale N    Each test's --cycles times N (a ROM built for a faster CPU clock needs more cycles for the same time)
//   --list              List the tests
//
// Exit code 0 when every test passes, 1 otherwise.  Each failure shows what was missing (or found when it
// shouldn't be), the command that reproduces it, and the serial output.
//
// A test: name, what it checks (about), the emulator options (args: --cycles, --input, ...), and:
//   expect  text or regular expressions that must appear in the serial output, in this order (each is
//           looked for after the previous one's match).  Line ends are "\n" (the ROM's CR LF, and ESC
//           shows as <ESC>)
//   forbid  text or regular expressions that must not appear anywhere in the serial output (every test
//           also forbids a driver's boot FAIL, and the emulator halting: a BRK to nowhere, an STP, ...)
//   check   function (serial output, the emulator's whole report, the test's files) returning an error
//           message, or nothing when it's good
//   fullRun true: run to --cycles (otherwise the run ends STOP_AFTER_INPUT cycles after the prompt that follows the
//           input's last key: a \p is added to the input, and --stop-after-input)
//   pagedRom  function (the paged ROM image in use) returning another, to run with (and the BIOS ROM in use)
// The input's waits (tests/common.js): P for the prompt (the last command done), W(n) for n * 2M cycles (where time
// has to pass).  The tests are in tests/*.js, by area.
//   sd      true: a blank 1 MB SD card image on device 0 (files.sd = its path); or a list of cards: { dev (0-7),
//           mb (default 1), sdsc (true: standard capacity), fill (a function given the image to fill in),
//           label, blocks (a HydraFS of that many blocks: fewer, and the image is cut to it; more, and the
//           image stays mb, for a card that claims more than the test needs) and hfs (a function given a
//           HydraFS volume made on the card, and the hydrafs module, to put files in it), quick (that HydraFS
//           as the Hydra's quick format makes one: version 2, no free map written), claim (the card says it
//           has that many blocks, more than its image: they read as zeros), image (a card image from
//           sim/cards to start from, instead of a blank one: a copy, so the fixture never changes), part
//           (the HydraFS in a partition, after a FAT one of that many MB: 0 for none) }
//           (files.sds[dev] = each one's path)
//   pc      a folder on the PC for /pc (hydrasim.js --pc-dir): { files: { name: contents (a string or a Buffer; a
//           name ending in / a folder) }, readOnly (--pc-read-only) } (files.pc = its path, to look at afterwards)
//
// Every test also fails if a task's stack came within STACK_MARGIN bytes of its bottom (the emulator reports
// each task's lowest stack pointer); the summary shows the deepest stack of the whole run.
// ****************************************************************************
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFile } = require('child_process');
const hydrafs = require('./tools/hydrafs.js');            // (The runner makes the tests' SD card images)

const SIM = path.join(__dirname, 'hydrasim.js');
const CARDS = path.join(__dirname, 'cards');                    // Fixture card images (cards/README.md)
const STACK_MARGIN = 32;                                        // Free stack bytes a task must keep
const STOP_AFTER_INPUT = 3000000;                               // A test's run ends this long after its last key (fullRun: at --cycles)

// The tests, by area (sim/tests/*.js; their helpers: tests/common.js)
const TESTS = [
  ...require('./tests/system.js'),
  ...require('./tests/forth.js'),
  ...require('./tests/devices.js'),
  ...require('./tests/storage.js'),
  ...require('./tests/shell.js'),
  ...require('./tests/pc.js'),
];

// ---- options
const opt = { rom: null, seed: 1, jobs: os.cpus().length, verbose: false, list: false, names: [], sim: [], cyclesScale: 1 };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const a = argv[i], next = () => argv[++i];
  switch (a) {
    case '--rom': opt.rom = next(); break;
    case '--seed': opt.seed = +next(); break;
    case '--random': opt.seed = -1; break;
    case '--jobs': opt.jobs = Math.max(1, +next()); break;
    case '--verbose': opt.verbose = true; break;
    case '--sim': opt.sim.push(...next().split(/\s+/).filter(Boolean)); break;
    case '--cycles-scale': opt.cyclesScale = +next(); break;
    case '--list': opt.list = true; break;
    default:
      if (a.startsWith('--')) { console.error('unknown option ' + a); process.exit(2); }
      opt.names.push(a);
  }
}
const tests = TESTS.filter(t => !opt.names.length || opt.names.some(n => t.name.includes(n)));
if (opt.list) { for (const t of TESTS) console.log(t.name.padEnd(16) + t.about); process.exit(0); }
if (!tests.length) { console.error('no tests match ' + opt.names.join(' ')); process.exit(2); }

// ---- run
const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'hydra-regress-'));
const quote = s => /^[\w\/.:=-]+$/.test(s) ? s : "'" + s + "'";

function runTest(t) {
  const files = {};
  const args = [SIM, ...(typeof t.args === 'function' ? t.args() : t.args)];     // (A function: made when it runs)
  if (!t.fullRun && !t.romImage) {                              // Done soon after its input: its last command's prompt
    const i = args.indexOf('--input');
    if (i > 0) args[i + 1] += '\\p';
    args.push('--stop-after-input', String(STOP_AFTER_INPUT));
  }
  if (t.romImage) {                                             // A ROM image of its own
    const { bios, cycles } = t.romImage(), dir = path.join(tmpDir, t.name);
    fs.mkdirSync(dir);
    fs.writeFileSync(path.join(dir, 'os_rom_C02.bin'), bios);
    fs.writeFileSync(path.join(dir, 'paged_rom_C02.bin'), Buffer.alloc(0x10000, 0xFF));
    files.expectCycles = cycles;
    args.push('--rom', dir);
  } else if (t.pagedRom) {                                      // The ROMs in use, with a paged ROM image of its own
    const rom = opt.rom || path.join(__dirname, '..', 'os_rom', 'bin'), dir = path.join(tmpDir, t.name);
    fs.mkdirSync(dir);
    fs.copyFileSync(path.join(rom, 'os_rom_C02.bin'), path.join(dir, 'os_rom_C02.bin'));
    fs.writeFileSync(path.join(dir, 'paged_rom_C02.bin'), t.pagedRom(fs.readFileSync(path.join(rom, 'paged_rom_C02.bin'))));
    args.push('--rom', dir);
  } else if (opt.rom) args.push('--rom', opt.rom);
  if (opt.seed >= 0) args.push('--seed', String(opt.seed));
  args.push(...opt.sim);
  const ci = args.indexOf('--cycles');
  if (ci > 0 && opt.cyclesScale !== 1) args[ci + 1] = String(Math.round(+args[ci + 1] * opt.cyclesScale));
  files.sds = [];
  for (const c of t.sd === true ? [{ dev: 0 }] : t.sd || []) {    // The SD cards
    const f = path.join(tmpDir, t.name + '-' + c.dev + '.img');
    if (c.image) fs.copyFileSync(path.join(CARDS, c.image), f);  // A fixture card (a copy: it stays as it is)
    else {
      const img = Buffer.alloc((c.mb || 1) << 20);
      if (c.fill) c.fill(img);
      fs.writeFileSync(f, img);
    }
    if (c.hfs) {                                                // A HydraFS on it, made with the host tool
      hydrafs.mkfs(f, c.mb || 1, c.label || '', c.blocks, c.quick, c.part === undefined ? -1 : c.part);   // (blocks: a smaller filesystem)
      const v = new hydrafs.Volume(f);
      try { c.hfs(v, hydrafs); } finally { v.close(); }
      if (fs.statSync(f).size > (c.mb || 1) << 20) fs.truncateSync(f, (c.mb || 1) << 20);   // (A HydraFS bigger than
    }                                                           //   the card: only its first blocks are used)
    files.sds[c.dev] = f;
    if (!files.sd) files.sd = f;
    args.push('--sd', c.dev + ':' + f + (c.claim ? '@' + c.claim : ''));
    if (c.sdsc) args.push('--sdsc', String(c.dev));
  }
  if (t.pc) {                                                    // A folder on the PC (/pc)
    const d = path.join(tmpDir, t.name + '-pc');
    fs.mkdirSync(d);
    for (const [name, data] of Object.entries(t.pc.files || {})) {
      const f = path.join(d, ...name.split('/'));
      if (name.endsWith('/')) fs.mkdirSync(f, { recursive: true });
      else { fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, data); }
    }
    files.pc = d;
    args.push('--pc-dir', d);
    if (t.pc.readOnly) args.push('--pc-read-only');
  }
  let cmd = 'node hydrasim.js ' + args.slice(1).map(quote).join(' ');
  for (const f of files.sds) if (f) cmd = cmd.split(f).join(path.basename(f));
  if (files.pc) cmd = cmd.split(files.pc).join(path.basename(files.pc));
  return new Promise(resolve => execFile(process.execPath, args, { maxBuffer: 64 << 20 }, (err, stdout, stderr) => {
    const report = stdout.replace(/\r/g, '');
    const m = /--- serial output ---\n([\s\S]*?)\n--- last instructions/.exec(report);
    const out = m ? m[1] : '';
    const errors = [];
    if (err) errors.push('the emulator failed: ' + (stderr.trim().split('\n').pop() || err.message));
    else if (!m) errors.push('no serial output in the emulator\'s report');
    const halted = /--- halted: (.*)/.exec(report);
    if (t.halts) { if (!halted || !t.halts.test(halted[1])) errors.push('the emulator didn\'t halt as expected (' + t.halts + '): ' + (halted ? halted[1] : 'it ran on')); }
    else if (halted) errors.push('the emulator halted: ' + halted[1]);
    if (!t.bootFailOk && / FAIL [0-9A-F]{2}\n/.test(out)) errors.push('a driver failed to start: ' + / (\S+ FAIL [0-9A-F]{2})\n/.exec(out)[1]);
    let at = 0;
    for (const e of t.expect || []) {
      let found = -1, len = 0;
      if (e instanceof RegExp) {
        const re = new RegExp(e.source, e.flags.replace('g', '') + 'g'); re.lastIndex = at;
        const r = re.exec(out); if (r) { found = r.index; len = r[0].length; }
      } else { found = out.indexOf(e, at); len = e.length; }
      if (found < 0) { errors.push('missing (in order): ' + show(e)); break; }
      at = found + len;
    }
    for (const f of t.forbid || []) if (f instanceof RegExp ? f.test(out) : out.includes(f)) errors.push('found: ' + show(f));
    const stacks = [];                                          // [task, free bytes, W:PC]
    const sl = /--- lowest stack pointer by task .*?\): (.*)/.exec(report);
    if (sl) for (const e of sl[1].matchAll(/([0-9A-F]):[0-9A-F]{2} \((\d+); ([0-9A-F]:[0-9A-F]{4})\)/g)) stacks.push([e[1], +e[2], e[3]]);
    for (const [task, free, at] of stacks) if (free < STACK_MARGIN) errors.push('task ' + task + '\'s stack got down to ' + free + ' free bytes (at ' + at + ')');
    if (t.check && !errors.length) { const e = t.check(out, report, files); if (e) errors.push(e); }
    resolve({ t, errors, out, cmd, stacks });
  }));
}
const show = e => e instanceof RegExp ? String(e) : JSON.stringify(e);

(async () => {
  const start = Date.now(), results = [], queue = [...tests];
  await Promise.all(Array.from({ length: Math.min(opt.jobs, tests.length) }, async () => {
    for (let t; (t = queue.shift());) {
      const r = await runTest(t);
      results.push(r);
      console.log((r.errors.length ? 'FAIL ' : 'ok   ') + t.name.padEnd(16) + t.about);
    }
  }));
  const failed = tests.map(t => results.find(r => r.t === t)).filter(r => r.errors.length);
  for (const r of results) {
    if (!r.errors.length && !opt.verbose) continue;
    console.log('\n=== ' + r.t.name + (r.errors.length ? ': FAILED' : ''));
    for (const e of r.errors) console.log('  ' + e);
    console.log('  (cd sim; ' + r.cmd + ')\n--- serial output ---\n' + r.out.replace(/\.{8,}/g, '...'));
  }
  const deep = results.flatMap(r => r.stacks.map(s => [...s, r.t.name])).sort((a, b) => a[1] - b[1])[0];
  if (deep) console.log('\nDeepest stack: task ' + deep[0] + ', ' + deep[1] + ' bytes free (at ' + deep[2] + ', test ' + deep[3] + ')');
  fs.rmSync(tmpDir, { recursive: true, force: true });
  console.log('\n' + (tests.length - failed.length) + ' of ' + tests.length + ' tests passed (' +
    ((Date.now() - start) / 1000).toFixed(1) + ' s' + (opt.seed >= 0 ? ', seed ' + opt.seed : ', random power-up') + ')');
  process.exit(failed.length ? 1 : 0);
})();
