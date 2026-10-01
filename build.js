#!/usr/bin/env node
// ****************************************************************************
// build.js - builds the Hydra-16's software on any OS (Node.js and cc65 needed).
//
// Usage: node build.js [what ...]
//   (nothing)       everything: the C library and its samples, the assembly sample, the ROM images (which have
//                   some of the samples in /rom)
//   rom             the ROM images: os_rom/bin/os_rom_C02.bin and paged_rom_C02.bin (with /rom's files and the
//                   checksums), checked for calls across ROM pages without a gate, and the space left on each page
//   c               the C library (programs/c/lib/hydra.lib) and the C samples (programs/c/bin/*.hyx)
//   asm             the assembly samples (programs/asm/samples/*.s -> programs/asm/bin/*.hyx)
//   prog FILE ...   a C program of your own (FILE.c, and more .c or .s files): programs/c/bin/FILE.hyx, as hyc.bat
//                   builds it (HYC_CFLAGS and HYC_LDFLAGS, if set, go to cc65 and ld65)
//   variants        the ROM's build options (os_rom/include/hw.inc) that the images in bin/ don't have: built into
//                   os_rom/obj/variants/NAME/, each with a few of the regression tests (CPU_CLOCK_MULT 2: the CPU at
//                   7.16 MHz; SER_ACIA_WDC: a WDC W65C51N ACIA)
//   test            then the regression tests (sim/regress.js), and the variants'
//
// cc65 is found in CC65_HOME (its bin, include, asminc and lib folders), or on the PATH (its home is then the
// folder above the one ca65 is in), or in C:\source\cc65\win64_snapshot.  The .bat files (os_rom/makeC02.bat,
// programs/c/make.bat and hyc.bat, programs/asm/make.bat) run this.
// ****************************************************************************
'use strict';
const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const ROOT = __dirname;
const OS_ROM = path.join(ROOT, 'os_rom');
const CPROG = path.join(ROOT, 'programs', 'c');
const EXE = process.platform === 'win32' ? '.exe' : '';

// ---- cc65
function findCc65() {
  const has = dir => dir && fs.existsSync(path.join(dir, 'bin', 'ca65' + EXE));
  if (process.env.CC65_HOME) {
    if (!has(process.env.CC65_HOME)) fail('CC65_HOME (' + process.env.CC65_HOME + ') has no bin/ca65' + EXE);
    return process.env.CC65_HOME;
  }
  for (const dir of (process.env.PATH || '').split(path.delimiter)) {
    if (dir && fs.existsSync(path.join(dir, 'ca65' + EXE))) return path.dirname(dir);
  }
  const snapshot = 'C:\\source\\cc65\\win64_snapshot';
  if (process.platform === 'win32' && has(snapshot)) return snapshot;
  fail('cc65 not found: set CC65_HOME to its folder, or put its bin folder on the PATH');
}
let cc65Home = null;
const cc65 = () => cc65Home || (cc65Home = findCc65());
const tool = name => path.join(cc65(), 'bin', name + EXE);

// ---- running things
function fail(msg) { console.error('build: ' + msg); process.exit(1); }
function run(cmd, args, cwd) {
  const r = spawnSync(cmd, args, { cwd, stdio: 'inherit' });
  if (r.error) fail(cmd + ': ' + r.error.message);
  if (r.status !== 0) fail(path.basename(cmd) + ' failed (exit ' + r.status + ')');
}
const node = (script, args, cwd) => run(process.execPath, [script, ...args], cwd);
const mkdir = dir => fs.mkdirSync(dir, { recursive: true });

// ---- the ROM
function rom() {
  mkdir(path.join(OS_ROM, 'bin'));
  mkdir(path.join(OS_ROM, 'obj'));
  // The version (os_rom/VERSION: the boot's welcome shows it), for the assembly
  const version = fs.readFileSync(path.join(OS_ROM, 'VERSION'), 'utf8').trim();
  if (!/^[\x20-\x7E]{1,16}$/.test(version)) fail('os_rom/VERSION: 1-16 printable characters, please');
  writeIfChanged(path.join(OS_ROM, 'obj', 'version.inc'),
    '; Made by build.js from os_rom/VERSION: don\'t edit\r\n.define HY_VERSION "' + version + '"\r\n');
  // The test song (sndtest's: /rom/songs/test.zsm, on the ROM disk), from its score
  node(path.join(ROOT, 'sim', 'tools', 'hysong.js'), ['songs/test.mml', 'songs/test.zsm', '--quiet'], OS_ROM);
  run(tool('ca65'), ['-g', '-o', 'obj/all_C02.o', '-l', 'obj/all_C02.txt', '--cpu', '65C02', 'all.s'], OS_ROM);
  run(tool('ld65'), ['-C', 'os_rom_C02.cfg', 'obj/all_C02.o', '-Ln', 'obj/os_rom_C02.lbl', '-m', 'obj/os_rom_C02.map',
    '--dbgfile', 'obj/os_rom_C02.dbg'], OS_ROM);
  // The ROM disk (/rom: a HydraFS volume, from romfs.txt) into the paged ROM image from bank 2; then the checksums for the
  // hardware test; then calls to another ROM page that don't go through a gate (a crash on the board)
  node(path.join(ROOT, 'sim', 'tools', 'mkromdisk.js'), ['romfs.txt', 'bin/paged_rom_C02.bin'], OS_ROM);
  node(path.join(OS_ROM, 'tools', 'romsum.js'), ['bin/os_rom_C02.bin', 'bin/paged_rom_C02.bin'], OS_ROM);
  node(path.join(OS_ROM, 'tools', 'check_pages.js'), ['obj/os_rom_C02.dbg'], OS_ROM);
  node(path.join(OS_ROM, 'tools', 'rom_space.js'), ['obj/os_rom_C02.map', 'os_rom_C02.cfg'], OS_ROM);
}
function writeIfChanged(file, text) {
  if (!fs.existsSync(file) || fs.readFileSync(file, 'utf8') !== text) fs.writeFileSync(file, text);
}

// ---- the ROM's build options, each built on its own and tested with the emulator set to match
const VARIANTS = [
  { name: 'cpu7mhz', defines: ['CPU_CLOCK_MULT=2'], sim: ['--clock', '7.16'], scale: 2, tests: ['boot', 'selftest', 'forth', 'io', 'pipes', 'serial', 'hydrafs'] },
  // (Not serial: it checks the Rockwell ACIA's command register, which turns on the TX interrupt)
  { name: 'wdc-acia', defines: ['SER_ACIA=1'], sim: ['--acia', 'wdc'], scale: 1, tests: ['boot', 'selftest', 'forth', 'io', 'pipes', 'serial-unpaced', 'paste', 'fast-output', 'sound'] },
];
function variants(test) {
  for (const v of VARIANTS) {
    const out = 'obj/variants/' + v.name;
    mkdir(path.join(OS_ROM, out));
    console.log('-- ROM variant ' + v.name + ': ' + v.defines.join(' '));
    run(tool('ca65'), ['-g', '-o', out + '/all_C02.o', '--cpu', '65C02', ...v.defines.flatMap(d => ['-D', d]), 'all.s'], OS_ROM);
    // The link config, with its images in the variant's folder
    const cfg = fs.readFileSync(path.join(OS_ROM, 'os_rom_C02.cfg'), 'utf8').replace(/file = "bin\//g, 'file = "' + out + '/');
    fs.writeFileSync(path.join(OS_ROM, out, 'os_rom_C02.cfg'), cfg);
    run(tool('ld65'), ['-C', out + '/os_rom_C02.cfg', out + '/all_C02.o', '-m', out + '/os_rom_C02.map'], OS_ROM);
    node(path.join(ROOT, 'sim', 'tools', 'mkromdisk.js'), ['romfs.txt', out + '/paged_rom_C02.bin'], OS_ROM);
    node(path.join(OS_ROM, 'tools', 'romsum.js'), [out + '/os_rom_C02.bin', out + '/paged_rom_C02.bin'], OS_ROM);
    if (test) node(path.join(ROOT, 'sim', 'regress.js'), ['--rom', path.join(OS_ROM, out), '--sim', v.sim.join(' '), '--cycles-scale', String(v.scale), ...v.tests], path.join(ROOT, 'sim'));
  }
}

// ---- the C library and programs
function cOpts() {
  return {
    cc: ['-g', '-t', 'none', '--cpu', '65C02', '-O', '-I', path.join(CPROG, 'include'), '-I', path.join(cc65(), 'include')],
    as: ['-g', '--cpu', '65C02', '-I', path.join(CPROG, 'lib'), '-I', path.join(cc65(), 'asminc')],
  };
}
// One .c or .s: obj/NAME.o.  (A module named as one of cc65's, e.g. getenv, takes its place in the library)
function compile(file, extra) {
  const o = cOpts(), obj = path.join(CPROG, 'obj'), name = path.basename(file).replace(/\.[cs]$/i, '');
  const out = path.join(obj, name + '.o');
  if (/\.s$/i.test(file)) run(tool('ca65'), [...o.as, '-o', out, file], CPROG);
  else {
    run(tool('cc65'), [...o.cc, ...(extra || []), '-o', path.join(obj, name + '.s'), file], CPROG);
    run(tool('ca65'), [...o.as, '-o', out, path.join(obj, name + '.s')], CPROG);
  }
  return out;
}
function clib() {
  mkdir(path.join(CPROG, 'obj'));
  mkdir(path.join(CPROG, 'bin'));
  const objs = [];
  for (const ext of ['.s', '.c']) {
    for (const d of ['crt', 'io', 'env', 'conio', 'snd', 'sys']) {
      const dir = path.join(CPROG, 'lib', d);
      for (const f of sorted(fs.readdirSync(dir).filter(f => f.toLowerCase().endsWith(ext)))) objs.push(compile(path.join(dir, f)));
    }
  }
  const lib = path.join(CPROG, 'lib', 'hydra.lib');
  fs.copyFileSync(path.join(cc65(), 'lib', 'none.lib'), lib);
  run(tool('ar65'), ['a', lib, ...objs], CPROG);
  for (const f of sorted(fs.readdirSync(path.join(CPROG, 'samples')).filter(f => f.endsWith('.c')))) prog([path.join(CPROG, 'samples', f)]);
}
// Names in the order Windows lists them (by their upper case: _cwd.s after fileio.s), so the library's modules,
// and so the programs linked with it, come out the same on any OS
const sorted = names => names.sort((a, b) => (a.toUpperCase() < b.toUpperCase() ? -1 : a.toUpperCase() > b.toUpperCase() ? 1 : 0));
function prog(files) {
  if (!files.length) fail('prog FILE.c [MORE.c | MORE.s ...]');
  mkdir(path.join(CPROG, 'obj'));
  mkdir(path.join(CPROG, 'bin'));
  const split = s => (s || '').split(/\s+/).filter(Boolean);
  const objs = files.map(f => compile(path.resolve(f), split(process.env.HYC_CFLAGS)));
  const name = path.basename(files[0]).replace(/\.[cs]$/i, '');
  run(tool('ld65'), ['-C', path.join(CPROG, 'hydra.cfg'), ...split(process.env.HYC_LDFLAGS), '-m', path.join(CPROG, 'bin', name + '.map'),
    '-o', path.join(CPROG, 'bin', name + '.hyx'), ...objs, path.join(CPROG, 'lib', 'hydra.lib')], CPROG);
}

// ---- the assembly samples: each .s in programs/asm/samples is a program (hyx.inc's header; hyx.cfg)
function asm() {
  const dir = path.join(ROOT, 'programs', 'asm');
  mkdir(path.join(dir, 'obj'));
  mkdir(path.join(dir, 'bin'));
  for (const f of sorted(fs.readdirSync(path.join(dir, 'samples')).filter(f => f.endsWith('.s')))) {
    const name = f.slice(0, -2);
    run(tool('ca65'), ['-g', '--cpu', '65C02', '-I', '.', '-o', 'obj/' + name + '.o', 'samples/' + f], dir);
    run(tool('ld65'), ['-C', 'hyx.cfg', '-o', 'bin/' + name + '.hyx', 'obj/' + name + '.o'], dir);
  }
}

// ---- what to do
const args = process.argv.slice(2);
if (args[0] === 'prog') { prog(args.slice(1)); process.exit(0); }
const what = new Set(args.length ? args : ['rom', 'c', 'asm']);
for (const w of what) if (!['rom', 'c', 'asm', 'variants', 'test'].includes(w)) fail('what\'s ' + w + '?  (rom, c, asm, variants, prog FILE ..., test)');
if (what.size === 1 && what.has('test')) ['rom', 'c', 'asm'].forEach(w => what.add(w));
if (what.has('c')) clib();                     // (First: /rom has the C samples in it)
if (what.has('asm')) asm();
if (what.has('rom')) rom();
if (what.has('test')) node(path.join(ROOT, 'sim', 'regress.js'), [], path.join(ROOT, 'sim'));
if (what.has('variants') || (what.has('test') && what.has('rom'))) variants(what.has('test'));
