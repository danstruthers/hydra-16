#!/usr/bin/env node
// ****************************************************************************
// build.js - builds HydraOS (docs/design/reimplementation-from-scratch.md, phase 0) on the base (../base: the kernel,
// the system calls, the parts every system on it shares; docs/design/plans/BASE.md):
//   1. the base            ../base/build.js: the jump table, the error codes, hydra.inc (../base/obj), and the kernel,
//                          ../base/bin/bios.bin, copied to bin/bios.bin (the same BIOS ROM, byte for byte)
//   2. tools/apigen.js     the languages' bindings to the calls, the reference, the libraries' (spec/numbers.def,
//                          asm.def)
//   3. the modules         modules/NAME/*.s -> obj/modules/NAME.bin; tests/mod/NAME/*.s -> obj/tests/NAME.bin;
//                          the test RAM programs, tests/ram/NAME/*.s -> obj/tests/NAME.hyx (sdk/asm/hyx2.cfg); the
//                          ROM disk's programs (its bin), programs/NAME/*.s -> obj/programs/NAME.hyx; the SDK's
//                          samples (its sample), sdk/asm/samples/NAME/*.s -> obj/samples/NAME.hyx (a driver's,
//                          NAME.bin: a module, for a test's ROM); each checked:
//                          only the kernel writes T, V and W (../base/tools/check.js); and HyForth's libraries,
//                          forthlib/NAME.s -> obj/forthlib/NAME.fl (tools/forthlib.js: the core's id patched into
//                          obj/modules/forth.bin first), for the ROM disk's /lib/forth; and hylang's snapshot,
//                          obj/modules/hysnap.bin (tools/hysnap.js: hylang's id patched into obj/modules/hylang.bin,
//                          then its heap with its library loaded, taken in the emulator), if rom.txt has it
//   4. the paged ROM       modules/rom.txt -> bin/prom0.bin, prom1.bin ... (../base/tools/romimg.js): a 512K image for
//                          each socket it fills, in order, as many as it needs; with the hardware test in bank 1
//                          (from ../os_rom/bin/paged_rom_C02.bin) and the ROMs' checksums for it, and the ROM
//                          disk's volume after the modules (romfs/romfs.txt: tools/romfs.js), each file read back
//   5. the budgets         sizes, and room left (../base/tools/budget.js)
//   6. the SDK             bin/sdk/asm: the assembly SDK whole, to take away (sdk/asm, the generated hydra.inc and
//                          numbers.inc, asmlib.inc, the samples' sources); the C library, obj/sdk/c/hydra.lib (cc65's none.lib
//                          with sdk/c/lib's modules), and bin/sdk/c: the C SDK whole (sdk/c, the generated
//                          hydracalls.h, numdefs.h and asmdefs.h, the library, the samples' sources)
// A program's folder (programs/, tests/ram/, a sample's) with a .c in it is a C program: its .c and .s files compiled
// (cc65 -t none) and linked with the C library and sdk/c/hydra.cfg (sdk/c/README.md); the C samples are
// sdk/c/samples/NAME -> obj/samples/c/NAME.hyx.
// and obj/build.json: the options it was built with ({ clock, acia }: sim/test.js's budgets can depend on them).
//
// Usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--quiet]
//   --clock 2: for a 7.16 MHz board (jumper J8); --acia wdc: a WDC W65C51N in the serial port (its TDRE bug)
//        node build.js prog DIR
//   a RAM program from DIR/*.s (with the SDK: sdk/asm/README.md), or DIR/*.c and *.s (a C program: sdk/c/README.md),
//   into DIR/NAME.hyx, NAME the folder's
// The cc65 tools: $CC65_BIN, else $CC65_HOME/bin (as the repository's CI sets it), else
// C:/source/cc65/win64_snapshot/bin, else the PATH; cc65's own folders (include, asminc, lib): $CC65_HOME, else the
// folder above its tools'.
// From Node: require('./build.js').build(opts) does the same, and gives what it made.
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const base = require('../base/build.js');
const apigen = require('./tools/apigen.js');
const romimg = require('./tools/romimg.js');
const romfs = require('./tools/romfs.js');
const sdcard = require('./tools/sdcard.js');
const forthlib = require('./tools/forthlib.js');
const hysnap = require('./tools/hysnap.js');
const budget = require('../base/tools/budget.js');
const check = require('../base/tools/check.js');

const ROOT = __dirname;
const at = (...p) => path.join(ROOT, ...p);
const atBase = (...p) => path.join(base.ROOT, ...p);

function tool(name) {
  for (const dir of [process.env.CC65_BIN, process.env.CC65_HOME && path.join(process.env.CC65_HOME, 'bin'), 'C:/source/cc65/win64_snapshot/bin']) {
    if (!dir) continue;
    for (const f of [path.join(dir, name + '.exe'), path.join(dir, name)]) if (fs.existsSync(f)) return f;
  }
  return name;
}
const CA65 = tool('ca65'), LD65 = tool('ld65'), CC65 = tool('cc65'), AR65 = tool('ar65');

// cc65's home: $CC65_HOME, else the folder above the one its tools are in
function cc65Home() {
  if (process.env.CC65_HOME) return process.env.CC65_HOME;
  let bin = path.dirname(CA65);
  if (!path.isAbsolute(CA65))       // (On the PATH)
    bin = (process.env.PATH || '').split(path.delimiter).find(d => d && (fs.existsSync(path.join(d, 'ca65.exe')) || fs.existsSync(path.join(d, 'ca65')))) || '.';
  return path.dirname(bin);
}

function run(exe, args) {
  try { execFileSync(exe, args, { cwd: ROOT, stdio: ['ignore', 'pipe', 'pipe'] }); }
  catch (e) { throw new Error(path.basename(exe) + ' ' + args.filter(a => /\.(s|c|cfg)$/.test(a)).join(' ') + ':\n' + ((e.stderr || '') + (e.stdout || '')).toString().trim()); }
}
const mkdir = d => fs.mkdirSync(d, { recursive: true });
const sources = dir => fs.readdirSync(dir).filter(f => f.endsWith('.s')).sort().map(f => path.join(dir, f));
const csources = dir => fs.readdirSync(dir).filter(f => /\.[cs]$/.test(f)).sort().map(f => path.join(dir, f));
const isC = dir => fs.readdirSync(dir).some(f => f.endsWith('.c'));

// Assemble files into objdir.  OUT: the objects
function assemble(files, objdir, includes, defines) {
  mkdir(objdir);
  return files.map(f => {
    const o = path.join(objdir, path.basename(f, '.s') + '.o');
    run(CA65, ['--cpu', '65C02', '-g', ...includes.flatMap(i => ['-I', i]), ...defines.flatMap(d => ['-D', d]),
      '-l', o.replace(/\.o$/, '.lst'), '-o', o, f]);
    return o;
  });
}

// HydraOS's includes, then the base's (its hydra.inc, the SDK's core, the hardware's, errors.inc): a module's, a
// program's; a test module's, the base's testlib.inc too
const INCLUDES = [at('obj', 'sdk'), at('sdk', 'asm'), ...base.INCLUDES, at('obj', 'gen'), atBase('tests', 'mod')];

// A module or a RAM program (the base's buildModule: ../base/modules/moduleN.cfg, ../base/sdk/asm/hyx2.cfg), with
// HydraOS's includes and the base's, and what apigen makes (forth's sys- words: obj/gen/forthsys.inc); a test RAM
// program's, tests/mod's too (testlib.inc).  OUT: its image (a Buffer)
function buildModule(dir, objdir, defines, ram = false, libs = {}) {
  return base.buildModule(dir, objdir, defines, ram, libs, ram ? [...INCLUDES, at('tests', 'mod')] : INCLUDES);
}

// ---- The C target (sdk/c): cc65 -t none, and the library obj/sdk/c/hydra.lib
function cflags() {
  const home = cc65Home(), own = d => fs.existsSync(path.join(home, d)) ? ['-I', path.join(home, d)] : [];
  return {
    cc: ['-g', '-t', 'none', '--cpu', '65C02', '-O', '-I', at('sdk', 'c', 'include'), '-I', at('obj', 'sdk', 'c'), ...own('include')],
    as: ['-g', '--cpu', '65C02', '-I', at('obj', 'sdk'), '-I', atBase('obj', 'sdk'), '-I', at('obj', 'sdk', 'c'), ...own('asminc')],
  };
}

// C and assembly sources into objdir (a .c: cc65 makes its .s there, then ca65).  OUT: the objects
function compileC(files, objdir, extra = []) {
  mkdir(objdir);
  const fl = cflags();
  return files.map(f => {
    const name = path.basename(f).replace(/\.[cs]$/, ''), o = path.join(objdir, name + '.o');
    let s = f;
    if (f.endsWith('.c')) {
      s = path.join(objdir, name + '.s');
      run(CC65, [...fl.cc, ...extra, '-o', s, f]);
    }
    run(CA65, [...fl.as, '-l', path.join(objdir, name + '.lst'), '-o', o, s]);
    return o;
  });
}

// obj/sdk/c/hydra.lib: cc65's none.lib, with sdk/c/lib's modules added (one named as one of cc65's, getenv say,
// takes its place), less cc65's modules whose functions are in the library's under other names (stdio's:
// sdk/c/lib/hyfile.h).  OUT: its file
const CC65_DROPPED = ['fputc', 'fputs', 'puts', 'ftell', 'freopen'];
function clib() {
  const objs = compileC(csources(at('sdk', 'c', 'lib')), at('obj', 'sdk', 'c', 'lib'));
  const none = [path.join(cc65Home(), 'lib'), '/usr/share/cc65/lib', '/usr/local/share/cc65/lib'].map(d => path.join(d, 'none.lib')).find(fs.existsSync);
  if (!none) throw new Error('cc65\'s none.lib not found: set CC65_HOME to cc65\'s folder');
  const lib = at('obj', 'sdk', 'c', 'hydra.lib');
  fs.copyFileSync(none, lib);
  run(AR65, ['d', lib, ...CC65_DROPPED.map(m => m + '.o')]);
  run(AR65, ['r', lib, ...objs]);
  return lib;
}

// A C program: dir's .c and .s files, linked with the C library and sdk/c/hydra.cfg into out (a RAM program; its
// map and labels in objdir).  OUT: its image
function cprog(dir, objdir, out, extra) {
  const name = path.basename(out).replace(/\.hyx$/, '');
  const objs = compileC(csources(dir), objdir, extra);
  run(LD65, ['-C', at('sdk', 'c', 'hydra.cfg'), '-o', out, '-m', path.join(objdir, name + '.map'), '-Ln', path.join(objdir, name + '.lbl'),
    ...objs, at('obj', 'sdk', 'c', 'hydra.lib')]);
  const data = fs.readFileSync(out);
  check.checkModule(name, data, 0x0800);
  return data;
}

// bin/sdk/asm: the assembly SDK, to take away: sdk/asm's files, the generated hydra.inc, the samples' sources
function sdk() {
  const out = at('bin', 'sdk', 'asm');
  fs.rmSync(out, { recursive: true, force: true });
  mkdir(path.join(out, 'samples'));
  for (const d of [atBase('sdk', 'asm'), at('sdk', 'asm')])     // (The base's core, then HydraOS's)
    for (const f of fs.readdirSync(d).filter(f => /\.(inc|s|cfg|md)$/.test(f))) fs.copyFileSync(path.join(d, f), path.join(out, f));
  fs.copyFileSync(atBase('obj', 'sdk', 'hydra.inc'), path.join(out, 'hydra.inc'));
  fs.copyFileSync(at('obj', 'sdk', 'numbers.inc'), path.join(out, 'numbers.inc'));
  fs.copyFileSync(at('obj', 'sdk', 'asmlib.inc'), path.join(out, 'asmlib.inc'));
  for (const f of ['module.cfg', 'module2.cfg', 'module3.cfg', 'module4.cfg']) fs.copyFileSync(atBase('modules', f), path.join(out, f));   // (A module's links)
  for (const d of fs.readdirSync(at('sdk', 'asm', 'samples'), { withFileTypes: true }).filter(d => d.isDirectory())) {
    mkdir(path.join(out, 'samples', d.name));
    for (const f of sources(at('sdk', 'asm', 'samples', d.name))) fs.copyFileSync(f, path.join(out, 'samples', d.name, path.basename(f)));
  }
  // bin/sdk/c: the C SDK (its README, hydra.cfg, include with the generated hydracalls.h, numdefs.h and asmdefs.h,
  // lib/hydra.lib, samples)
  const c = at('bin', 'sdk', 'c');
  fs.rmSync(c, { recursive: true, force: true });
  for (const d of ['include', 'lib', 'samples']) mkdir(path.join(c, d));
  for (const f of ['README.md', 'hydra.cfg']) fs.copyFileSync(at('sdk', 'c', f), path.join(c, f));
  for (const f of fs.readdirSync(at('sdk', 'c', 'include'))) fs.copyFileSync(at('sdk', 'c', 'include', f), path.join(c, 'include', f));
  for (const f of ['hydracalls.h', 'numdefs.h', 'asmdefs.h']) fs.copyFileSync(at('obj', 'sdk', 'c', f), path.join(c, 'include', f));
  fs.copyFileSync(at('obj', 'sdk', 'c', 'hydra.lib'), path.join(c, 'lib', 'hydra.lib'));
  for (const d of fs.readdirSync(at('sdk', 'c', 'samples'), { withFileTypes: true }).filter(d => d.isDirectory())) {
    mkdir(path.join(c, 'samples', d.name));
    for (const f of csources(at('sdk', 'c', 'samples', d.name))) fs.copyFileSync(f, path.join(c, 'samples', d.name, path.basename(f)));
  }
}

// A RAM program from dir/*.s, or a C program (dir/*.c and *.s: HYC_CFLAGS, if set, go to cc65), anywhere (node
// build.js prog DIR): dir/NAME.hyx.  OUT: its file
function prog(dir) {
  dir = path.resolve(dir);
  apigen.generate(ROOT);
  const name = path.basename(dir), od = at('obj', 'prog', name), bin = path.join(dir, name + '.hyx');
  if (isC(dir)) {
    if (!fs.existsSync(at('obj', 'sdk', 'c', 'hydra.lib'))) clib();
    cprog(dir, od, bin, (process.env.HYC_CFLAGS || '').split(/\s+/).filter(Boolean));
    return bin;
  }
  const objs = assemble(sources(dir), od, [at('obj', 'sdk'), at('sdk', 'asm'), base.INCLUDES[0], base.INCLUDES[1], dir], ['HYX2_RAM']);
  run(LD65, ['-C', atBase('sdk', 'asm', 'hyx2.cfg'), '-o', bin, '-m', path.join(od, name + '.map'), ...objs]);
  check.checkModule(name, fs.readFileSync(bin), 0x0800);
  return bin;
}

// The hardware test (bank 1 of the old system's paged ROM image) and a rom.txt's list: the base's
const { hwtest, readManifest, HWTEST_IMAGE } = base;

function build(opt = {}) {
  const say = opt.quiet ? () => {} : s => console.log(s);
  const defines = base.definesOf(opt);

  // The base: the kernel (../base/bin/bios.bin), its BIOS ROM ours too
  const baseBuilt = base.build(Object.assign({}, opt, { noReport: true }));
  apigen.generate(ROOT);
  mkdir(at('bin'));
  fs.copyFileSync(atBase('bin', 'bios.bin'), at('bin', 'bios.bin'));

  // The C library (the C programs need it), the modules, the test modules and the test RAM programs
  clib();
  const ram = (dir, objdir) => isC(dir) ? cprog(dir, path.join(objdir, path.basename(dir)), path.join(objdir, path.basename(dir) + '.hyx'))
    : buildModule(dir, objdir, defines, true);
  const modules = {}, tests = {}, progs = {}, programs = {}, samples = {};
  mkdir(at('obj', 'modules'));                          // The base's modules (kdev, ser, wozmon) as HydraOS's too: in
  for (const [n, data] of Object.entries(baseBuilt.modules)) {  //   obj/modules with its own, for the tools that read them there
    modules[n] = data;
    fs.writeFileSync(at('obj', 'modules', n + '.bin'), data);
  }
  for (const d of fs.readdirSync(at('modules'), { withFileTypes: true }).filter(d => d.isDirectory()))
    modules[d.name] = buildModule(at('modules', d.name), at('obj', 'modules'), defines, false, modules);
  if (fs.existsSync(at('tests', 'mod')))
    for (const d of fs.readdirSync(at('tests', 'mod'), { withFileTypes: true }).filter(d => d.isDirectory()))
      tests[d.name] = buildModule(at('tests', 'mod', d.name), at('obj', 'tests'), defines);
  if (fs.existsSync(at('tests', 'ram')))
    for (const d of fs.readdirSync(at('tests', 'ram'), { withFileTypes: true }).filter(d => d.isDirectory()))
      progs[d.name] = ram(at('tests', 'ram', d.name), at('obj', 'tests'));
  if (fs.existsSync(at('programs')))
    for (const d of fs.readdirSync(at('programs'), { withFileTypes: true }).filter(d => d.isDirectory()))
      programs[d.name] = ram(at('programs', d.name), at('obj', 'programs'));
  for (const d of fs.readdirSync(at('sdk', 'asm', 'samples'), { withFileTypes: true }).filter(d => d.isDirectory())) {
    const dir = at('sdk', 'asm', 'samples', d.name);
    const driver = sources(dir).some(f => /^\s+HYX2_DRIVER\b/m.test(fs.readFileSync(f, 'latin1')));   // (A module)
    const data = buildModule(dir, at('obj', 'samples'), defines, !driver);
    if (driver) modules['sample ' + d.name] = data;
    else samples[d.name] = data;
  }
  for (const d of fs.readdirSync(at('sdk', 'c', 'samples'), { withFileTypes: true }).filter(d => d.isDirectory()))
    samples['c/' + d.name] = ram(at('sdk', 'c', 'samples', d.name), at('obj', 'samples', 'c'));
  sdk();
  const card = sdcard.build(ROOT);                          // The SD card's image (bin/sdcard.img): the samples, the songs
  forthlib.build({ root: ROOT, modules, assemble: (files, od, inc) => assemble(files, od, inc, defines), ld65: args => run(LD65, args) });

  // hylang's snapshot (the module hysnap: its heap with its library loaded, taken in the emulator), then the paged ROM
  const manifest = readManifest(at('modules', 'rom.txt'));
  const hwt = hwtest(), bios = fs.readFileSync(at('bin', 'bios.bin'));
  if (manifest.modules.includes('hysnap')) {
    const s = hysnap.build({ root: ROOT, modules, names: manifest.modules, rc: tests.t_rc, hwtest: hwt, bios });
    say('hylang\'s snapshot (hysnap): ' + s.bytes + ' bytes, ' + s.pages + ' pages of cells and ' + s.blobBytes + ' bytes of blobs; hylang\'s id $' +
      s.id.toString(16).toUpperCase().padStart(4, '0'));
  }
  for (const n of manifest.modules) if (!modules[n]) throw new Error('modules/rom.txt: no module ' + n);
  if (!hwt) say('(no ' + path.relative(ROOT, HWTEST_IMAGE) + ': the paged ROM has no hardware test)');
  const { image, entries, disk, banks, chips } = romimg.build({ modules: manifest.modules.map(n => ({ file: n, data: modules[n] })), init: manifest.init,
    hwtest: hwt, bios, romfs: romfs.manifest(at('romfs', 'romfs.txt')) });
  for (const f of fs.readdirSync(at('bin')).filter(f => /^prom\d*\.bin$/.test(f))) fs.rmSync(at('bin', f));   // (The last build's)
  for (let k = 0; k < chips; k++) fs.writeFileSync(at('bin', 'prom' + k + '.bin'), image.subarray(k * romimg.CHIP, (k + 1) * romimg.CHIP));
  fs.writeFileSync(at('obj', 'build.json'), JSON.stringify({ clock: opt.clock || 1, acia: opt.acia || 'rockwell' }) + '\n');

  const report = budget.report(base.ROOT, { modules, tests, progs, programs, samples, entries });
  say(report.text);
  if (disk) {
    const bytes = disk.files.reduce((n, f) => n + f.size, 0), used = disk.volume.length / 512, first = disk.start / 32;
    say('ROM disk: ' + disk.files.length + ' files, ' + bytes + ' bytes; its volume uses ' + used + ' of ' + disk.blocks + ' blocks (paged ROM banks ' +
      first + '-' + (first + disk.blocks / 32 - 1) + ', in socket order); every file read back as its source');
  }
  say('SD card: ' + card.files.length + ' files, ' + card.bytes + ' bytes (the samples, the songs): bin/sdcard.img, ' + card.size / 1048576 + ' MB');
  say('Paged ROM: ' + banks + ' banks of 256, ' + chips + ' chip' + (chips === 1 ? '' : 's') + ' of 512K: ' +
    [...Array(chips)].map((_, k) => 'bin/prom' + k + '.bin').join(', '));
  return { modules, tests, progs, programs, samples, manifest, report };
}

if (require.main === module) {
  const a = process.argv.slice(2), opt = {};
  if (a[0] === 'prog') {
    if (a.length !== 2) { console.error('usage: node build.js prog DIR'); process.exit(2); }
    try { console.log(path.relative(process.cwd(), prog(a[1]))); } catch (e) { console.error('build: ' + e.message); process.exit(1); }
    process.exit(0);
  }
  for (let i = 0; i < a.length; i++) {
    if (a[i] === '--clock') opt.clock = +a[++i];
    else if (a[i] === '--acia') opt.acia = a[++i];
    else if (a[i] === '--quiet') opt.quiet = true;
    else { console.error('usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--quiet]'); process.exit(2); }
  }
  try { build(opt); } catch (e) { console.error('build: ' + e.message); process.exit(1); }
}
module.exports = { build, buildModule, readManifest, hwtest, prog, clib, cprog };
