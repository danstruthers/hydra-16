#!/usr/bin/env node
// ****************************************************************************
// bench.js - hylang against HyForth: the same benchmarks in each (sdcard/bench: bench.hl and hl/NAME.hl, bench.fs; on
// HydraOS's SD card, bin/sdcard.img, at /sd/0/bench), run in the emulator, their times compared, by kind; and BASIC's (bench.bas, all
// twenty too), and BASIC's inline assembly's (benchasm.bas: each one's work in an ASM block).  Each program runs each
// benchmark reps times and prints
//   bench LANGUAGE NAME RESULT TICKS REPS
// (the ticks the reps took, 200 a second, as the machine counts them); this prints a table of the results (which
// must be the same in both), the time of one run of each, and hylang's against HyForth's, with each kind's geometric
// mean and all of them's.  Each of hylang's benchmarks runs in a hylang of its own (--together: all in one, where
// the functions of all of them share the machine's room for code).
//
// Usage: node sim/bench.js [--quick] [--only NAME,...] [--kind KIND,...] [--hylang-reps N] [--forth-reps N]
//                          [--basic-reps N] [--asm-reps N] [--no-asm] [--together] [--vs TREE] [--json FILE] [-v]
//   --quick           the small sizes (the bench test's)
//   --only NAME,...   those benchmarks alone; --kind KIND,...: those kinds' (calls, loops, arith, bytes, lists, text)
//   --hylang-reps N   each of hylang's benchmarks run N times (default 1: each takes a second or so)
//   --forth-reps N    HyForth's (default 5)
//   --basic-reps N    BASIC's (default 1; its benchmarks in one run of bench.bas)
//   --asm-reps N      BASIC's inline assembly's (default 50: each takes a few ticks; in one run of benchasm.bas)
//   --no-asm          benchasm.bas not run
//   --together        hylang's in one hylang, one after another
//   --vs TREE         hylang's again in another tree's build (another branch's worktree, built: its modules, these
//                     benchmarks), a column of its, and this tree's hylang against it
//   --json FILE       the results, as JSON, to FILE too
//   -v                the console's output too
// On the board: hylang /sd/0/bench/bench.hl [reps [q|f [name...]]], forth /sd/0/bench/bench.fs [reps [q|f [name...]]],
// basic /sd/0/bench/bench.bas [reps [q|f [name...]]], basic /sd/0/bench/benchasm.bas [reps [q|f [name...]]].
// Build first (node build.js).  Its status: 1 if a result isn't the same in both, or a benchmark didn't finish.
'use strict';
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..');
// The benchmarks, in the programs' order: their kinds, what each measures
const BENCH = [
  ['calls', 'calls', 'calls of a function of two arguments, in a tail call\'s loop'],
  ['fib', 'calls', 'Fibonacci, recursively: calls and returns, + and <'],
  ['tak', 'calls', 'Takeuchi\'s function: calls of three arguments, deep'],
  ['ack', 'calls', 'Ackermann\'s function: calls nested deep, a tail call in each'],
  ['loop', 'loops', 'a counting loop: a tail call a step (HyForth: DO LOOP)'],
  ['while', 'loops', 'a sum by while over two locals (BEGIN WHILE REPEAT)'],
  ['dotimes', 'loops', 'the same sum by dotimes (DO LOOP)'],
  ['nested', 'loops', 'a dotimes in a dotimes, xor and a test in it'],
  ['gcd', 'arith', 'gcd(i, j) by subtraction, summed: tests and tail calls'],
  ['collatz', 'arith', 'the Collatz steps of 1 to n: halving, 3n + 1'],
  ['hash', 'arith', 'h = ((h & 255) * 31 + i) & 4095: a multiplication a step'],
  ['sieve', 'bytes', 'the primes below n, in a buffer of bytes (HyForth: memory)'],
  ['sort', 'bytes', 'n bytes sorted by insertion, in a buffer'],
  ['matrix', 'bytes', 'two matrices of bytes multiplied: indexes and *'],
  ['queens', 'bytes', 'n queens placed by backtracking, a byte each column, diagonal'],
  ['mapf', 'lists', 'sum, map and filter over range (HyForth: a loop, EXECUTE)'],
  ['fold', 'lists', 'foldl and a function made with fn (HyForth: EXECUTE)'],
  ['each', 'lists', 'each over a list (HyForth: DO LOOP over an array)'],
  ['chars', 'text', 'the a\'s in a string: char-at, char-code (HyForth: C@)'],
  ['digits', 'text', 'numbers written out: to-str (HyForth: <# #S #>)'],
];
const KINDS = ['calls', 'loops', 'arith', 'bytes', 'lists', 'text'];
const BASIC = BENCH.map(b => b[0]);                                // (bench.bas's and benchasm.bas's: all of them)

// The paged ROM of a tree's build (its modules, its ROM disk); the benchmarks are this tree's SD card's (card())
function rom(tree) {
  const romimg = require(path.join(tree, 'tools', 'romimg.js')), romfs = require(path.join(tree, 'tools', 'romfs.js'));
  const { readManifest, hwtest } = require(path.join(tree, 'build.js'));
  const bin = (d, n) => fs.readFileSync(path.join(tree, 'obj', d, n + '.bin'));
  const sys = readManifest(path.join(tree, 'modules', 'rom.txt')).modules;
  const mods = [...sys.map(n => ({ file: n, data: bin('modules', n) })), { file: 't_rc', data: bin('tests', 't_rc') }];
  const files = romfs.manifest(path.join(tree, 'romfs', 'romfs.txt'));
  return romimg.build({ modules: mods, init: 't_rc', hwtest: hwtest(), bios: fs.readFileSync(path.join(tree, 'bin', 'bios.bin')),
    romfs: files }).image;
}

// The lines a run types (rc's), and its output's results: { LANGUAGE: { NAME: { result, ticks, reps } } }
// This tree's SD card (bin/sdcard.img: the benchmarks, in SD device 0), its writes kept in memory, for every tree run
function card() {
  const img = fs.readFileSync(path.join(ROOT, 'bin', 'sdcard.img')), written = new Map();
  return { dev: 0, blocks: img.length / 512, read: n => written.get(n) || Buffer.from(img.subarray(n * 512, n * 512 + 512)),
    write: (n, b) => written.set(n, Buffer.from(b)) };
}

function run(tree, lines, verbose) {
  const { boot } = require(path.join(tree, 'sim', 'run.js'));
  const m = boot({ seed: 1, prom: rom(tree), sd: [card()], input: [...lines, 'echo %%END%%'].map(l => 'ā' + l + '\r').join('') });
  let shown = 0;
  while (m.cpu.cyc < 2e11) {
    m.run(m.cpu.cyc + 50e6);
    const out = m.out.replace(/\r/g, '');
    if (verbose && out.length > shown) { process.stdout.write(out.slice(shown)); shown = out.length; }
    if (/^%%END%%$/m.test(out)) break;
  }
  const got = {};
  for (const [, lang, name, result, ticks, reps] of m.out.replace(/\r/g, '').matchAll(/^bench (hylang|forth|basic|basm) (\S+) (\S+) (\d+) (\d+)/gm))
    (got[lang] = got[lang] || {})[name] = { result, ticks: +ticks, reps: +reps };
  return got;
}

function main(argv) {
  const opt = { quick: false, hy: 1, fo: 5, ba: 1, as: 50, asm: true, verbose: false, together: false, only: null, kind: null, vs: null, json: null };
  const list = s => s.split(',').map(x => x.trim()).filter(Boolean);
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--quick') opt.quick = true;
    else if (a === '--hylang-reps') opt.hy = Math.max(1, +argv[++i] | 0);
    else if (a === '--forth-reps') opt.fo = Math.max(1, +argv[++i] | 0);
    else if (a === '--basic-reps') opt.ba = Math.max(1, +argv[++i] | 0);
    else if (a === '--asm-reps') opt.as = Math.max(1, +argv[++i] | 0);
    else if (a === '--no-asm') opt.asm = false;
    else if (a === '--only') opt.only = list(argv[++i] || '');
    else if (a === '--kind') opt.kind = list(argv[++i] || '');
    else if (a === '--together') opt.together = true;
    else if (a === '--vs') opt.vs = path.resolve(argv[++i] || '');
    else if (a === '--json') opt.json = argv[++i];
    else if (a === '-v') opt.verbose = true;
    else {
      console.error('usage: node sim/bench.js [--quick] [--only NAME,...] [--kind KIND,...] [--hylang-reps N] [--forth-reps N] ' +
        '[--basic-reps N] [--asm-reps N] [--no-asm] [--together] [--vs TREE] [--json FILE] [-v]');
      process.exit(2);
    }
  }
  const bad = [...(opt.only || []).filter(n => !BENCH.some(b => b[0] === n)), ...(opt.kind || []).filter(k => !KINDS.includes(k))];
  if (bad.length) { console.error('bench.js: no such benchmark or kind: ' + bad.join(', ')); process.exit(2); }
  const chosen = BENCH.filter(([n, k]) => (!opt.only || opt.only.includes(n)) && (!opt.kind || opt.kind.includes(k)));
  if (!chosen.length) { console.error('bench.js: no benchmark chosen'); process.exit(2); }
  const names = chosen.length === BENCH.length ? '' : ' ' + chosen.map(b => b[0]).join(' ');
  const sz = opt.quick ? 'q' : 'f';
  const hyLines = opt.together ? ['hylang /sd/0/bench/bench.hl ' + opt.hy + ' ' + sz + names]
    : chosen.map(([n]) => 'hylang /sd/0/bench/bench.hl ' + opt.hy + ' ' + sz + ' ' + n);
  const baLines = chosen.some(([n]) => BASIC.includes(n)) ? ['basic /sd/0/bench/bench.bas ' + opt.ba + ' ' + sz + names] : [];
  const asLines = baLines.length && opt.asm ? ['basic /sd/0/bench/benchasm.bas ' + opt.as + ' ' + sz + names] : [];
  const got = run(ROOT, [...hyLines, 'forth /sd/0/bench/bench.fs ' + opt.fo + ' ' + sz + names, ...baLines, ...asLines], opt.verbose);
  const vs = opt.vs ? run(opt.vs, hyLines, opt.verbose) : null;

  const mult = (() => { try { return JSON.parse(fs.readFileSync(path.join(ROOT, 'obj', 'build.json'), 'utf8')).clock || 1; } catch (e) { return 1; } })();
  const ms = r => r ? r.ticks * 5 / r.reps : NaN;
  const fmt = (v, w) => String(v).padStart(w);
  const x = r => isFinite(r) ? r.toFixed(1) + 'x' : '-';
  const x2 = r => isFinite(r) ? r.toFixed(r < 1 ? 3 : 1) + 'x' : '-';            // (inline assembly's: well under 1)
  console.log('hylang against HyForth' + (opt.quick ? ' (quick sizes)' : '') + ', at ' + (3.58 * mult).toFixed(2) + ' MHz: ' +
    'one run of each, in ms (hylang ' + opt.hy + ' rep' + (opt.hy > 1 ? 's' : '') + (opt.together ? ', all in one hylang' : ', each in a hylang of its own') +
    ', HyForth ' + opt.fo + (baLines.length ? ', BASIC ' + opt.ba : '') + (asLines.length ? ', BASIC\'s ASM ' + opt.as : '') + ')' +
    (vs ? '; vs: ' + opt.vs : ''));
  console.log('');
  console.log('kind   benchmark  result  hylang ms' + (vs ? '      vs ms  vs/this' : '') + '  HyForth ms  hylang/HyForth' +
    (baLines.length ? '    BASIC ms  BASIC/HyForth  BASIC/hylang' : '') + (asLines.length ? '    ASM ms  ASM/HyForth  BASIC/ASM' : '') + '  what');
  let fails = 0;
  const rows = [], geo = (a) => a.length ? Math.exp(a.reduce((s, v) => s + Math.log(v), 0) / a.length) : NaN;
  for (const kind of KINDS) {
    const ks = chosen.filter(b => b[1] === kind);
    if (!ks.length) continue;
    const ratios = [], vsr = [], bfr = [], bhr = [], afr = [], bar = [];
    for (const [name, , what] of ks) {
      const h = (got.hylang || {})[name], f = (got.forth || {})[name], v = vs && (vs.hylang || {})[name];
      const b = baLines.length && BASIC.includes(name) ? (got.basic || {})[name] || null : undefined;
      const as = asLines.length && BASIC.includes(name) ? (got.basm || {})[name] || null : undefined;
      const same = h && f && h.result === f.result && (!v || v.result === h.result) && (b === undefined || (b && b.result === h.result)) &&
        (as === undefined || (as && as.result === h.result));
      if (!same) fails++;
      const hm = ms(h), fm = ms(f), vm = ms(v), ratio = hm / fm;
      if (isFinite(ratio) && ratio > 0) ratios.push(ratio);
      if (v && isFinite(vm / hm) && vm > 0 && hm > 0) vsr.push(vm / hm);
      if (b && isFinite(ms(b)) && ms(b) > 0 && fm > 0 && hm > 0) { bfr.push(ms(b) / fm); bhr.push(ms(b) / hm); }
      if (as && ms(as) > 0 && fm > 0) afr.push(ms(as) / fm);
      if (as && b && ms(as) > 0 && ms(b) > 0) bar.push(ms(b) / ms(as));
      const res = !h || !f ? '(none)' : same ? h.result : h.result + '/' + f.result + (v ? '/' + v.result : '') +
        (b ? '/' + b.result : '') + (as ? '/' + as.result : '') + '!';
      console.log(kind.padEnd(7) + name.padEnd(10) + ' ' + String(res).padEnd(7) + fmt(isFinite(hm) ? hm.toFixed(1) : '-', 10) +
        (vs ? fmt(isFinite(vm) ? vm.toFixed(1) : '-', 11) + fmt(x(vm / hm), 9) : '') +
        fmt(isFinite(fm) ? fm.toFixed(2) : '-', 12) + fmt(x(ratio), 16) +
        (baLines.length ? (b === undefined ? ''.padEnd(41) : fmt(isFinite(ms(b)) ? ms(b).toFixed(1) : '-', 12) + fmt(x(ms(b) / fm), 15) + fmt(x(ms(b) / hm), 14)) : '') +
        (asLines.length ? (as === undefined ? ''.padEnd(34) : fmt(isFinite(ms(as)) ? ms(as).toFixed(2) : '-', 10) + fmt(x2(ms(as) / fm), 13) +
          fmt(x(b ? ms(b) / ms(as) : NaN), 11)) : '') +
        '  ' + what);
      rows.push({ name, kind, result: h && h.result, hylang: hm, forth: fm, vs: vs ? vm : undefined, ratio, basic: b ? ms(b) : undefined,
        asm: as ? ms(as) : undefined });
    }
    console.log(''.padEnd(7) + '(' + kind + ': hylang/HyForth ' + x(geo(ratios)) + (vs ? ', vs/this ' + x(geo(vsr)) : '') +
      (bfr.length ? ', BASIC/HyForth ' + x(geo(bfr)) + ', BASIC/hylang ' + x(geo(bhr)) : '') +
      (afr.length ? ', ASM/HyForth ' + x2(geo(afr)) + ', BASIC/ASM ' + x(geo(bar)) : '') + ', geometric means)');
  }
  const all = rows.filter(r => isFinite(r.ratio) && r.ratio > 0);
  const sumH = all.reduce((s, r) => s + r.hylang, 0), sumF = all.reduce((s, r) => s + r.forth, 0);
  const vsAll = rows.filter(r => isFinite(r.vs) && isFinite(r.hylang) && r.vs > 0 && r.hylang > 0);
  console.log('\nall ' + all.length + ': hylang ' + sumH.toFixed(1) + ' ms, HyForth ' + sumF.toFixed(2) + ' ms, ' + x(sumH / sumF) +
    ' in all; the geometric mean of the ratios ' + x(geo(all.map(r => r.ratio))) +
    (vs ? '; vs ' + vsAll.reduce((s, r) => s + r.vs, 0).toFixed(1) + ' ms, vs/this ' + x(geo(vsAll.map(r => r.vs / r.hylang))) + ' (geometric mean)' : ''));
  const bas = rows.filter(r => isFinite(r.basic) && r.basic > 0 && r.forth > 0 && r.hylang > 0);
  if (bas.length) console.log('BASIC\'s ' + bas.length + ': BASIC ' + bas.reduce((t, r) => t + r.basic, 0).toFixed(1) + ' ms; the geometric means of the ratios: BASIC/HyForth ' +
    x(geo(bas.map(r => r.basic / r.forth))) + ', BASIC/hylang ' + x(geo(bas.map(r => r.basic / r.hylang))));
  const asm = rows.filter(r => isFinite(r.asm) && r.asm > 0 && r.forth > 0);
  if (asm.length) console.log('BASIC\'s inline assembly\'s ' + asm.length + ': ' + asm.reduce((t, r) => t + r.asm, 0).toFixed(1) +
    ' ms; the geometric means of the ratios: ASM/HyForth ' + x2(geo(asm.map(r => r.asm / r.forth))) + ', ASM/hylang ' +
    x2(geo(asm.filter(r => r.hylang > 0).map(r => r.asm / r.hylang))) + ', BASIC/ASM ' + x(geo(asm.filter(r => r.basic > 0).map(r => r.basic / r.asm))));
  if (opt.json) fs.writeFileSync(opt.json, JSON.stringify({ quick: opt.quick, together: opt.together, hylangReps: opt.hy, forthReps: opt.fo, basicReps: opt.ba,
    asmReps: asLines.length ? opt.as : undefined,
    vs: opt.vs, rows }, null, 2) + '\n');
  if (fails) console.log('\n' + fails + ' benchmark(s) not the same in each, or not finished');
  process.exit(fails ? 1 : 0);
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { BENCH, KINDS };
