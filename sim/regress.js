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
//   sd      true: a blank 1 MB SD card image on device 0 (files.sd = its path)
// ****************************************************************************
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFile } = require('child_process');

const SIM = path.join(__dirname, 'hydrasim.js');
const W = n => '\\w'.repeat(n);                                 // Wait n * ~2M cycles before the next key
const BOOT = W(1);                                              // Before the first key: to the HyForth prompt
const TO_MON = BOOT + 'bye\\r' + W(1);                          // To WOZMON

// A Forth number as "." prints it: " 0003"
const num = n => ' ' + n.toString(16).toUpperCase().padStart(4, '0');

const TESTS = [
  {
    name: 'boot', about: 'POST (task mapping, shared RAM, ROM page 1, RAM lines), drivers, HyForth banner',
    args: ['--cycles', '30000000'],
    expect: ['POST ZP:T ST:T LO:T 7D:T SH:S P1:4C\n',
      /RAM U:0( [0-9A-F]{2}:0\/00\/0000)+\n/,
      'Welcome to the HYDRA-16!', /HyForth \d/, 'HF>'],
  },
  {
    name: 'selftest', about: 'the ROM self tests from WOZMON: MMU (F833), scheduler (F869), IO (F88A)',
    args: ['--cycles', '60000000', '--input', TO_MON + 'F833R\\r' + W(2) + 'F869R\\r' + W(4) + 'F88AR\\r' + W(1)],
    expect: ['T1 00:00>F833R', 'MMU test: ok',
      'Sched test:\n', /^(?=[abm]*a)(?=[abm]*b)(?=[abm]*m)[abm]+\n/m,  // 1. a, b and m interleave
      /^\[c{10}\]m+\n/m,                                        // 2. NO_PREEMPT: the c's all together
      'wW\n', 'done',                                           // 3. TASK_WAIT, IO_WAKE
      'IO test: cons ok'],
    forbid: ['FAIL'],
  },
  {
    name: 'selftest-small', about: 'the self tests with 1 RAM module and 1 shared RAM macro-page',
    args: ['--cycles', '60000000', '--modules', '1', '--shared-u', '1',
      '--input', BOOT + 'mmtest\\r' + W(1) + 'bye\\r' + W(1) + 'F869R\\r' + W(4) + 'F88AR\\r' + W(1)],
    expect: [/RAM U:F F0:0\/00\/0000 F4:0\/00\/0000 F8:0\/00\/0000 FC:0\/00\/0000 00:0\/00\/0000\n/,
      'MMU test: ok', 'Sched test:', 'done', 'IO test: cons ok'],
    forbid: ['FAIL'],
  },
  {
    name: 'post-ram-fault', about: 'POST reports a stuck address line (A0 high on the chip holding shared bank $F0)',
    args: ['--cycles', '8000000', '--ram-fault', 'F0:A0:high'],
    expect: [/RAM U:0 F0:0\/00\/0001 F4:0\/00\/0000 /],
  },
  {
    name: 'post-no-shared', about: 'POST with no shared RAM: SH:X, and the drivers that need it fail',
    args: ['--cycles', '30000000', '--model', 'noshared'],
    expect: ['SH:X', 'SOUND FAIL', 'HF>'],
    bootFailOk: true,
  },
  {
    name: 'forth', about: 'HyForth: arithmetic (decimal in, hex out), typed wc (Ctrl-D ends it)',
    args: ['--cycles', '60000000', '--input', BOOT + '1 2 + .\\r1000 24 - .\\rwc\\rab c\\r\\x04. . .\\r'],
    expect: ['HF>1 2 + .\n' + num(3) + '\n', num(1000 - 24),
      'HF>. . .\n' + num(5) + num(2) + num(1) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'pipes', about: 'pipelines: words | wc, and through cat (a copy of the shell in the middle) gives the same',
    args: ['--cycles', '150000000', '--input', BOOT + 'words | wc . . .\\rwords | cat | wc . . .\\rwords | cat | cat | wc . . .\\r1 2 + .\\r'],
    expect: [/HF>words \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/, /HF>words \| cat \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/,
      /HF>words \| cat \| cat \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/, 'HF>1 2 + .\n' + num(3)],
    forbid: ['!DS PTR ERROR!', '!IO ERR!'],
    check: out => {
      const counts = [...out.matchAll(/wc \. \. \.\n((?: [0-9A-F]{4}){3})\n/g)].map(m => m[1]);
      if (counts.length !== 3 || counts.some(c => c !== counts[0])) return 'the pipelines counted differently: ' + counts.join(' /');
      if (/ 0000 0000 0000/.test(counts[0])) return 'wc counted nothing';
    },
  },
  {
    name: 'io', about: 'open/read /dev/zero, a missing file (ERR 70), mount and ns, /dev/proc',
    args: ['--cycles', '90000000', '--input', BOOT +
      'q^/dev/zero^ 1 open here @ 5 read . ioerr .\\r' +
      'q^/dev/nothere^ 1 open\\rioerr .\\r' +
      'q^/z^ q^zero^ mount ns\\rq^/z^ 1 open here @ 3 read .\\r' +
      'q^/dev/proc^ 1 open here @ 100 read .\\r'],
    expect: ['read . ioerr .\n' + num(5) + num(0) + '\n',
      '!IO ERR!', 'HF>ioerr .\n' + num(0x70) + '\n',
      '/z -> zero\n', 'read .\n' + num(3) + '\n',
      /\/dev\/proc\^ 1 open here @ 100 read \.\n 00[1-9A-F][0-9A-F]\n/],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'tasks', about: 'another shell: ps, Ctrl-] to switch the console, kill; Ctrl-C breaks a read',
    args: ['--cycles', '150000000', '--input', BOOT + 'shell\\r' + W(1) + 'ps\\r' + W(1) + '\\x1dB' + W(1) + '\\r1 2 + .\\r\\x1d1' + W(1) +
      '\\r11 kill\\rps\\rcat\\r' + W(1) + '\\x03' + W(1) + '3 4 + .\\r'],
    expect: ['HF>ps\n0 R -\n1 R 0 *\nB W 1\n', '[B]', 'HF>1 2 + .\n' + num(3), '[1]',
      'HF>ps\n0 R -\n1 R 0 *\nC D -\n', 'HF>cat\n', '!BREAK!', 'HF>3 4 + .\n' + num(7)],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'sound', about: 'sndtest plays in a task of its own while the shell runs; sndstop ends it; the bell (Ctrl-G) first',
    args: ['--cycles', '90000000', '--input', BOOT + '\\x07\\r' + W(1) + 'sndtest\\r' + W(1) + 'ps\\r' + W(2) + 'sndstop\\rps\\r'],
    expect: ['HF>ps\n0 R -\n1 R 0 *\nB R E\n', 'HF>sndstop\n', 'HF>ps\n0 R -\n1 R 0 *\nC D -\n'],
    check: (out, report) => {
      const m = /--- YM2151 key-ons: (\d+) \((.*)\)/.exec(report);
      if (!m || +m[1] < 5) return 'the tune played ' + (m ? m[1] : 'no') + ' notes';
      if (!/^ch 7 at/.test(m[2])) return 'no bell (the first key-on, on channel 7)';
    },
  },
  {
    name: 'sd', about: '/dev/sd: write a byte at offset 512, read it back, and it\'s in the card image',
    sd: true,
    args: ['--cycles', '90000000', '--input', BOOT + 'q^/dev/sd^ 3 open .\\r90 here @ c!\\r' +
      '3 512 0 seek 3 here @ 1 write . ioerr .\\r91 here @ c!\\r3 512 0 seek 3 here @ 1 read . here @ c@ .\\r3 close\\r'],
    expect: ['open .\n' + num(3) + '\n', 'write . ioerr .\n' + num(1) + num(0) + '\n',
      'read . here @ c@ .\n' + num(1) + num(90) + '\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const img = fs.readFileSync(files.sd);
      if (img[512] !== 90) return 'the card image has $' + img[512].toString(16) + ' at 512, not $5A';
      if (img.some((b, i) => b && i !== 512)) return 'the card image changed somewhere else too';
    },
  },
];

// ---- options
const opt = { rom: null, seed: 1, jobs: os.cpus().length, verbose: false, list: false, names: [] };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const a = argv[i], next = () => argv[++i];
  switch (a) {
    case '--rom': opt.rom = next(); break;
    case '--seed': opt.seed = +next(); break;
    case '--random': opt.seed = -1; break;
    case '--jobs': opt.jobs = Math.max(1, +next()); break;
    case '--verbose': opt.verbose = true; break;
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
  const args = [SIM, ...t.args];
  if (opt.rom) args.push('--rom', opt.rom);
  if (opt.seed >= 0) args.push('--seed', String(opt.seed));
  if (t.sd) {
    files.sd = path.join(tmpDir, t.name + '.img');
    fs.writeFileSync(files.sd, Buffer.alloc(1 << 20));
    args.push('--sd', files.sd);
  }
  const cmd = 'node hydrasim.js ' + args.slice(1).map(quote).join(' ').replace(files.sd || '\0', t.name + '.img');
  return new Promise(resolve => execFile(process.execPath, args, { maxBuffer: 64 << 20 }, (err, stdout, stderr) => {
    const report = stdout.replace(/\r/g, '');
    const m = /--- serial output ---\n([\s\S]*?)\n--- last instructions/.exec(report);
    const out = m ? m[1] : '';
    const errors = [];
    if (err) errors.push('the emulator failed: ' + (stderr.trim().split('\n').pop() || err.message));
    else if (!m) errors.push('no serial output in the emulator\'s report');
    const halted = /--- halted: (.*)/.exec(report);
    if (halted) errors.push('the emulator halted: ' + halted[1]);
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
    if (t.check && !errors.length) { const e = t.check(out, report, files); if (e) errors.push(e); }
    resolve({ t, errors, out, cmd });
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
  fs.rmSync(tmpDir, { recursive: true, force: true });
  console.log('\n' + (tests.length - failed.length) + ' of ' + tests.length + ' tests passed (' +
    ((Date.now() - start) / 1000).toFixed(1) + ' s' + (opt.seed >= 0 ? ', seed ' + opt.seed : ', random power-up') + ')');
  process.exit(failed.length ? 1 : 0);
})();
