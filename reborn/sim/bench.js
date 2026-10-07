#!/usr/bin/env node
// ****************************************************************************
// bench.js - hylang, HyForth and BASIC: the same benchmarks in each (romfs/bench: bench.hl, bench.fs, bench.bas; on
// the ROM disk at /rom/bench), run in the emulator, their times compared.  Each program runs each benchmark reps times
// and prints
//   bench LANGUAGE NAME RESULT TICKS REPS
// (the ticks the reps took, 200 a second, as the machine counts them); this prints a table of the results (which
// must be the same in all three) and the times of one run of each, and each language's against the others'.
//
// Usage: node sim/bench.js [--quick] [--hylang-reps N] [--forth-reps N] [--basic-reps N] [-v]
//   --quick           the small sizes (the bench test's)
//   --hylang-reps N   each of hylang's benchmarks run N times (default 1)
//   --forth-reps N    HyForth's (default 20: a run of its is a few ticks)
//   --basic-reps N    BASIC's (default 1)
//   -v                the console's output too
// On the board: hylang /rom/bench/bench.hl [reps [quick]], forth /rom/bench/bench.fs [reps [quick]], basic
// /rom/bench/bench.bas [reps [quick]].  Build first (node build.js).  Its status: 1 if a result isn't the same in all
// three, or a benchmark didn't finish.
'use strict';
const fs = require('fs');
const path = require('path');
const { boot } = require('./run.js');

const ROOT = path.join(__dirname, '..');
const NAMES = ['loop', 'calls', 'fib', 'sieve', 'sort', 'gcd'];
const WHAT = {
  loop: 'a counting loop, a step each', calls: 'calls of a function of two arguments', fib: 'Fibonacci, recursively',
  sieve: 'the primes below n, in bytes', sort: 'n bytes sorted by insertion', gcd: 'gcd(i, j) by subtraction, summed',
};
// The languages: the name their lines give, the name shown, the command, the option for their reps and its default
const LANGS = [
  { key: 'hylang', name: 'hylang', cmd: 'hylang /rom/bench/bench.hl', opt: '--hylang-reps', reps: 1 },
  { key: 'forth', name: 'HyForth', cmd: 'forth /rom/bench/bench.fs', opt: '--forth-reps', reps: 20 },
  { key: 'basic', name: 'BASIC', cmd: 'basic /rom/bench/bench.bas', opt: '--basic-reps', reps: 1 },
];
// The ratios shown: each the first's time over the second's
const RATIOS = [['hylang', 'forth'], ['basic', 'forth'], ['basic', 'hylang']];

function rom() {
  const romimg = require(path.join(ROOT, 'tools', 'romimg.js')), romfs = require(path.join(ROOT, 'tools', 'romfs.js'));
  const { readManifest, hwtest } = require(path.join(ROOT, 'build.js'));
  const bin = (d, n) => fs.readFileSync(path.join(ROOT, 'obj', d, n + '.bin'));
  const sys = readManifest(path.join(ROOT, 'modules', 'rom.txt')).modules;
  const mods = [...sys.map(n => ({ file: n, data: bin('modules', n) })), { file: 't_rc', data: bin('tests', 't_rc') }];
  return romimg.build({ modules: mods, init: 't_rc', hwtest: hwtest(), bios: fs.readFileSync(path.join(ROOT, 'bin', 'bios.bin')),
    romfs: romfs.manifest(path.join(ROOT, 'romfs', 'romfs.txt')) }).image;
}

function main(argv) {
  const opt = { quick: false, verbose: false, reps: Object.fromEntries(LANGS.map(l => [l.key, l.reps])) };
  for (let i = 0; i < argv.length; i++) {
    const lang = LANGS.find(l => l.opt === argv[i]);
    if (argv[i] === '--quick') opt.quick = true;
    else if (lang) opt.reps[lang.key] = Math.max(1, +argv[++i] | 0);
    else if (argv[i] === '-v') opt.verbose = true;
    else { console.error('usage: node sim/bench.js [--quick] [--hylang-reps N] [--forth-reps N] [--basic-reps N] [-v]'); process.exit(2); }
  }
  const q = opt.quick ? ' q' : '';
  const lines = [...LANGS.map(l => l.cmd + ' ' + opt.reps[l.key] + q), 'echo %%END%%'];
  const m = boot({ seed: 1, prom: rom(), input: lines.map(l => 'ā' + l + '\r').join('') });
  const limit = 20e9;
  let shown = 0;
  while (m.cpu.cyc < limit) {
    m.run(m.cpu.cyc + 50e6);
    const out = m.out.replace(/\r/g, '');
    if (opt.verbose && out.length > shown) { process.stdout.write(out.slice(shown)); shown = out.length; }
    if (/^%%END%%$/m.test(out)) break;
  }
  const out = m.out.replace(/\r/g, '');
  const got = Object.fromEntries(LANGS.map(l => [l.key, {}]));
  for (const [, lang, name, result, ticks, reps] of out.matchAll(/^bench (hylang|forth|basic) (\S+) (\S+) (\d+) (\d+)/gm))
    got[lang][name] = { result, ticks: +ticks, reps: +reps };

  const mult = (() => { try { return JSON.parse(fs.readFileSync(path.join(ROOT, 'obj', 'build.json'), 'utf8')).clock || 1; } catch (e) { return 1; } })();
  const ms = r => r.ticks * 5 / r.reps;
  const fmt = (v, w) => String(v).padStart(w);
  const nameOf = k => LANGS.find(l => l.key === k).name;
  console.log('hylang, HyForth and BASIC' + (opt.quick ? ' (quick sizes)' : '') + ', at ' + (3.58 * mult).toFixed(2) + ' MHz: ' +
    'one run of each, in ms (' + LANGS.map(l => l.name + ' ' + opt.reps[l.key] + ' rep' + (opt.reps[l.key] > 1 ? 's' : '')).join(', ') + ')');
  console.log('');
  const ratioHead = RATIOS.map(([a, b]) => nameOf(a) + '/' + nameOf(b));
  console.log('benchmark  result' + LANGS.map(l => fmt(l.name + ' ms', 12)).join('') + ratioHead.map(h => fmt(h, 16)).join('') + '  what');
  let bad = 0;
  const sum = Object.fromEntries(LANGS.map(l => [l.key, 0])), logs = RATIOS.map(() => 0);
  let n = 0;
  for (const name of NAMES) {
    const r = Object.fromEntries(LANGS.map(l => [l.key, got[l.key][name]]));
    const missing = LANGS.filter(l => !r[l.key]);
    if (missing.length) { console.log(name.padEnd(9) + '  (no result: ' + missing.map(l => l.name).join(', ') + ' didn\'t finish it)'); bad++; continue; }
    const results = [...new Set(LANGS.map(l => r[l.key].result))];
    if (results.length > 1) bad++;
    const t = Object.fromEntries(LANGS.map(l => [l.key, ms(r[l.key])]));
    for (const l of LANGS) sum[l.key] += t[l.key];
    const ratios = RATIOS.map(([a, b]) => t[b] > 0 ? t[a] / t[b] : Infinity);
    if (ratios.every(isFinite)) { ratios.forEach((v, i) => { logs[i] += Math.log(v); }); n++; }
    console.log(name.padEnd(9) + '  ' + (results.length === 1 ? results[0] : results.join('/') + '!').padEnd(6) +
      LANGS.map(l => fmt(t[l.key].toFixed(l.key === 'forth' ? 2 : 1), 12)).join('') +
      ratios.map(v => fmt(isFinite(v) ? v.toFixed(1) + 'x' : '-', 16)).join('') + '  ' + WHAT[name]);
  }
  if (n) console.log('\nall        ' + ''.padEnd(6) + LANGS.map(l => fmt(sum[l.key].toFixed(l.key === 'forth' ? 2 : 1), 12)).join('') +
    RATIOS.map(([a, b]) => fmt((sum[a] / sum[b]).toFixed(1) + 'x', 16)).join('') + '\n(the geometric means of the ratios: ' +
    RATIOS.map(([a, b], i) => nameOf(a) + '/' + nameOf(b) + ' ' + Math.exp(logs[i] / n).toFixed(1) + 'x').join(', ') + ')');
  if (bad) console.log('\n' + bad + ' benchmark(s) not the same in all three, or not finished');
  process.exit(bad ? 1 : 0);
}

if (require.main === module) main(process.argv.slice(2));
