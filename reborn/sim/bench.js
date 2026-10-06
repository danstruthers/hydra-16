#!/usr/bin/env node
// ****************************************************************************
// bench.js - hylang against HyForth: the same benchmarks in each (romfs/bench: bench.hl, bench.fs; on the ROM disk at
// /rom/bench), run in the emulator, their times compared.  Each program runs each benchmark reps times and prints
//   bench LANGUAGE NAME RESULT TICKS REPS
// (the ticks the reps took, 200 a second, as the machine counts them); this prints a table of the results (which
// must be the same in both) and the times of one run of each, and hylang's against HyForth's.
//
// Usage: node sim/bench.js [--quick] [--hylang-reps N] [--forth-reps N] [-v]
//   --quick           the small sizes (the bench test's)
//   --hylang-reps N   each of hylang's benchmarks run N times (default 1)
//   --forth-reps N    HyForth's (default 20: a run of its is a few ticks)
//   -v                the console's output too
// On the board: hylang /rom/bench/bench.hl [reps [quick]], forth /rom/bench/bench.fs [reps [quick]].
// Build first (node build.js).  Its status: 1 if a result isn't the same in both, or a benchmark didn't finish.
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
  const opt = { quick: false, hy: 1, fo: 20, verbose: false };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--quick') opt.quick = true;
    else if (argv[i] === '--hylang-reps') opt.hy = Math.max(1, +argv[++i] | 0);
    else if (argv[i] === '--forth-reps') opt.fo = Math.max(1, +argv[++i] | 0);
    else if (argv[i] === '-v') opt.verbose = true;
    else { console.error('usage: node sim/bench.js [--quick] [--hylang-reps N] [--forth-reps N] [-v]'); process.exit(2); }
  }
  const q = opt.quick ? ' q' : '';
  const lines = ['hylang /rom/bench/bench.hl ' + opt.hy + q, 'forth /rom/bench/bench.fs ' + opt.fo + q, 'echo %%END%%'];
  const m = boot({ seed: 1, prom: rom(), input: lines.map(l => '\u0101' + l + '\r').join('') });
  const limit = 20e9;
  let shown = 0;
  while (m.cpu.cyc < limit) {
    m.run(m.cpu.cyc + 50e6);
    const out = m.out.replace(/\r/g, '');
    if (opt.verbose && out.length > shown) { process.stdout.write(out.slice(shown)); shown = out.length; }
    if (/^%%END%%$/m.test(out)) break;
  }
  const out = m.out.replace(/\r/g, '');
  const got = { hylang: {}, forth: {} };
  for (const [, lang, name, result, ticks, reps] of out.matchAll(/^bench (hylang|forth) (\S+) (\S+) (\d+) (\d+)/gm))
    got[lang][name] = { result, ticks: +ticks, reps: +reps };

  const mult = (() => { try { return JSON.parse(fs.readFileSync(path.join(ROOT, 'obj', 'build.json'), 'utf8')).clock || 1; } catch (e) { return 1; } })();
  const ms = r => r.ticks * 5 / r.reps;
  const fmt = (v, w) => String(v).padStart(w);
  console.log('hylang against HyForth' + (opt.quick ? ' (quick sizes)' : '') + ', at ' + (3.58 * mult).toFixed(2) + ' MHz: ' +
    'one run of each, in ms (hylang ' + opt.hy + ' rep' + (opt.hy > 1 ? 's' : '') + ', HyForth ' + opt.fo + ')');
  console.log('');
  console.log('benchmark  result   hylang ms  HyForth ms  hylang/HyForth  what');
  let bad = 0, sumH = 0, sumF = 0, logs = 0, n = 0;
  for (const name of NAMES) {
    const h = got.hylang[name], f = got.forth[name];
    if (!h || !f) { console.log(name.padEnd(9) + '  (no result: ' + (!h ? 'hylang' : 'HyForth') + ' didn\'t finish it)'); bad++; continue; }
    const same = h.result === f.result;
    if (!same) bad++;
    const hm = ms(h), fm = ms(f), ratio = fm > 0 ? hm / fm : Infinity;
    sumH += hm; sumF += fm;
    if (isFinite(ratio)) { logs += Math.log(ratio); n++; }
    console.log(name.padEnd(9) + '  ' + (same ? h.result : h.result + '/' + f.result + '!').padEnd(7) + fmt(hm.toFixed(1), 10) +
      fmt(fm.toFixed(2), 12) + fmt(isFinite(ratio) ? ratio.toFixed(1) + 'x' : '-', 16) + '  ' + WHAT[name]);
  }
  if (n) console.log('\nall        ' + ''.padEnd(5) + fmt(sumH.toFixed(1), 10) + fmt(sumF.toFixed(2), 12) +
    fmt((sumH / sumF).toFixed(1) + 'x', 16) + '  (the geometric mean of the ratios: ' + Math.exp(logs / n).toFixed(1) + 'x)');
  if (bad) console.log('\n' + bad + ' benchmark(s) not the same in both, or not finished');
  process.exit(bad ? 1 : 0);
}

if (require.main === module) main(process.argv.slice(2));
