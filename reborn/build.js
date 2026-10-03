#!/usr/bin/env node
// ****************************************************************************
// build.js - builds everything (docs/reimplementation-from-scratch.md, phase 0):
//   1. tools/apigen.js     spec/ -> the jump table, the error codes and texts, the SDK's hydra.inc, the reference
//   2. the kernel          kernel/*.s and the generated sources -> bin/bios.bin (the 128K BIOS ROM), with its map
//                          and labels in obj/kernel/
//   3. the modules         modules/NAME/*.s -> obj/modules/NAME.bin; tests/mod/NAME/*.s -> obj/tests/NAME.bin;
//                          the test RAM programs, tests/ram/NAME/*.s -> obj/tests/NAME.hyx (sdk/asm/hyx2.cfg); the
//                          ROM disk's programs (its bin), programs/NAME/*.s -> obj/programs/NAME.hyx; the SDK's
//                          samples (its sample), sdk/asm/samples/NAME/*.s -> obj/samples/NAME.hyx; each checked:
//                          only the kernel writes T, V and W (tools/check.js)
//   4. the paged ROM       modules/rom.txt -> bin/prom.bin (tools/romimg.js), with the hardware test in bank 1
//                          (from ../os_rom/bin/paged_rom_C02.bin) and the ROMs' checksums for it, and the ROM
//                          disk's volume after the modules (romfs/romfs.txt: tools/romfs.js), each file read back
//   5. the budgets         sizes, and room left (tools/budget.js)
//   6. the SDK             bin/sdk/asm: the assembly SDK whole, to take away (sdk/asm, the generated hydra.inc, the
//                          samples' sources)
// and obj/build.json: the options it was built with ({ clock, acia }: sim/test.js's budgets can depend on them).
//
// Usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--quiet]
//   --clock 2: for a 7.16 MHz board (jumper J7); --acia wdc: a WDC W65C51N in the serial port (its TDRE bug)
//        node build.js prog DIR
//   a RAM program from DIR/*.s (with the SDK: sdk/asm/README.md) into DIR/NAME.hyx, NAME the folder's
// The cc65 tools: $CC65_BIN, else $CC65_HOME/bin (as the repository's CI sets it), else
// C:/source/cc65/win64_snapshot/bin, else the PATH.
// From Node: require('./build.js').build(opts) does the same, and gives what it made.
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const apigen = require('./tools/apigen.js');
const romimg = require('./tools/romimg.js');
const romfs = require('./tools/romfs.js');
const budget = require('./tools/budget.js');
const check = require('./tools/check.js');

const ROOT = __dirname;
const at = (...p) => path.join(ROOT, ...p);

function tool(name) {
  for (const dir of [process.env.CC65_BIN, process.env.CC65_HOME && path.join(process.env.CC65_HOME, 'bin'), 'C:/source/cc65/win64_snapshot/bin']) {
    if (!dir) continue;
    for (const f of [path.join(dir, name + '.exe'), path.join(dir, name)]) if (fs.existsSync(f)) return f;
  }
  return name;
}
const CA65 = tool('ca65'), LD65 = tool('ld65');

function run(exe, args) {
  try { execFileSync(exe, args, { cwd: ROOT, stdio: ['ignore', 'pipe', 'pipe'] }); }
  catch (e) { throw new Error(path.basename(exe) + ' ' + args.filter(a => a.endsWith('.s') || a.endsWith('.cfg')).join(' ') + ':\n' + ((e.stderr || '') + (e.stdout || '')).toString().trim()); }
}
const mkdir = d => fs.mkdirSync(d, { recursive: true });
const sources = dir => fs.readdirSync(dir).filter(f => f.endsWith('.s')).sort().map(f => path.join(dir, f));

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

// A module: the .s files in dir, linked with modules/module.cfg; or a RAM program (ram: assembled with HYX2_RAM,
// linked with sdk/asm/hyx2.cfg, NAME.hyx).  OUT: its image (a Buffer)
function buildModule(dir, objdir, defines, ram = false) {
  const name = path.basename(dir), od = path.join(objdir, name);
  const objs = assemble(sources(dir), od, [at('obj', 'sdk'), at('sdk', 'asm'), at('include'), dir, path.dirname(dir),
    ...(ram ? [at('tests', 'mod')] : [])], ram ? [...defines, 'HYX2_RAM'] : defines);
  const bin = path.join(objdir, name + (ram ? '.hyx' : '.bin'));
  const two = sources(dir).some(f => /\.segment\s+"CODE2"/i.test(fs.readFileSync(f, 'latin1')));   // (Two banks: module2.cfg)
  const cfg = ram ? at('sdk', 'asm', 'hyx2.cfg') : at('modules', two ? 'module2.cfg' : 'module.cfg');
  run(LD65, ['-C', cfg, '-o', bin, '-m', path.join(od, name + '.map'), '-Ln', path.join(od, name + '.lbl'), ...objs]);
  const data = fs.readFileSync(bin);
  check.checkModule(name, data, ram ? 0x0800 : 0xA000);       // (Only the kernel writes T, V and W)
  return data;
}

// bin/sdk/asm: the assembly SDK, to take away: sdk/asm's files, the generated hydra.inc, the samples' sources
function sdk() {
  const out = at('bin', 'sdk', 'asm');
  fs.rmSync(out, { recursive: true, force: true });
  mkdir(path.join(out, 'samples'));
  for (const f of fs.readdirSync(at('sdk', 'asm')).filter(f => /\.(inc|s|cfg|md)$/.test(f)))
    fs.copyFileSync(at('sdk', 'asm', f), path.join(out, f));
  fs.copyFileSync(at('obj', 'sdk', 'hydra.inc'), path.join(out, 'hydra.inc'));
  for (const d of fs.readdirSync(at('sdk', 'asm', 'samples'), { withFileTypes: true }).filter(d => d.isDirectory())) {
    mkdir(path.join(out, 'samples', d.name));
    for (const f of sources(at('sdk', 'asm', 'samples', d.name))) fs.copyFileSync(f, path.join(out, 'samples', d.name, path.basename(f)));
  }
}

// A RAM program from dir/*.s, anywhere (node build.js prog DIR): dir/NAME.hyx.  OUT: its file
function prog(dir) {
  dir = path.resolve(dir);
  apigen.generate(ROOT);
  const name = path.basename(dir), od = at('obj', 'prog', name), bin = path.join(dir, name + '.hyx');
  const objs = assemble(sources(dir), od, [at('obj', 'sdk'), at('sdk', 'asm'), dir], ['HYX2_RAM']);
  run(LD65, ['-C', at('sdk', 'asm', 'hyx2.cfg'), '-o', bin, '-m', path.join(od, name + '.map'), ...objs]);
  check.checkModule(name, fs.readFileSync(bin), 0x0800);
  return bin;
}

// The hardware test: bank 1 of the old system's paged ROM image (os_rom/bin, in Git), or null without it
const HWTEST_IMAGE = path.join(ROOT, '..', 'os_rom', 'bin', 'paged_rom_C02.bin');
function hwtest() {
  return fs.existsSync(HWTEST_IMAGE) ? fs.readFileSync(HWTEST_IMAGE) : null;
}

// modules/rom.txt: { init, modules: [names] }
function readManifest(file) {
  const m = { init: null, modules: [] };
  fs.readFileSync(file, 'latin1').split(/\r?\n/).forEach((raw, i) => {
    const line = raw.replace(/;.*/, '').trim();
    if (!line) return;
    const [what, name] = line.split(/\s+/);
    if (what === 'init') m.init = name;
    else if (what === 'module') m.modules.push(name);
    else throw new Error(file + ':' + (i + 1) + ': "init NAME" or "module NAME"');
  });
  return m;
}

function build(opt = {}) {
  const say = opt.quiet ? () => {} : s => console.log(s);
  const defines = [];
  if (opt.clock) defines.push('CPU_CLOCK_MULT=' + opt.clock);
  if (opt.acia) defines.push('ACIA_CHIP=' + (opt.acia === 'wdc' ? 1 : 0));

  apigen.generate(ROOT);
  mkdir(at('bin'));

  // The kernel
  const kobj = at('obj', 'kernel');
  const kfiles = [...sources(at('kernel')), at('obj', 'gen', 'jumptable.s'), at('obj', 'gen', 'errtext.s')];
  const objs = assemble(kfiles, kobj, [at('include'), at('obj', 'gen'), at('kernel')], defines);
  run(LD65, ['-C', at('kernel', 'bios.cfg'), '-o', at('bin', 'bios.bin'), '-m', path.join(kobj, 'bios.map'), '-Ln', path.join(kobj, 'bios.lbl'),
    '--dbgfile', path.join(kobj, 'bios.dbg'), ...objs]);

  // The modules, the test modules and the test RAM programs
  const modules = {}, tests = {}, progs = {}, programs = {}, samples = {};
  for (const d of fs.readdirSync(at('modules'), { withFileTypes: true }).filter(d => d.isDirectory()))
    modules[d.name] = buildModule(at('modules', d.name), at('obj', 'modules'), defines);
  if (fs.existsSync(at('tests', 'mod')))
    for (const d of fs.readdirSync(at('tests', 'mod'), { withFileTypes: true }).filter(d => d.isDirectory()))
      tests[d.name] = buildModule(at('tests', 'mod', d.name), at('obj', 'tests'), defines);
  if (fs.existsSync(at('tests', 'ram')))
    for (const d of fs.readdirSync(at('tests', 'ram'), { withFileTypes: true }).filter(d => d.isDirectory()))
      progs[d.name] = buildModule(at('tests', 'ram', d.name), at('obj', 'tests'), defines, true);
  if (fs.existsSync(at('programs')))
    for (const d of fs.readdirSync(at('programs'), { withFileTypes: true }).filter(d => d.isDirectory()))
      programs[d.name] = buildModule(at('programs', d.name), at('obj', 'programs'), defines, true);
  for (const d of fs.readdirSync(at('sdk', 'asm', 'samples'), { withFileTypes: true }).filter(d => d.isDirectory()))
    samples[d.name] = buildModule(at('sdk', 'asm', 'samples', d.name), at('obj', 'samples'), defines, true);
  sdk();

  // The paged ROM
  const manifest = readManifest(at('modules', 'rom.txt'));
  for (const n of manifest.modules) if (!modules[n]) throw new Error('modules/rom.txt: no module ' + n);
  const hwt = hwtest();
  if (!hwt) say('(no ' + path.relative(ROOT, HWTEST_IMAGE) + ': the paged ROM has no hardware test)');
  const { image, entries, disk } = romimg.build({ modules: manifest.modules.map(n => ({ file: n, data: modules[n] })), init: manifest.init,
    hwtest: hwt, bios: fs.readFileSync(at('bin', 'bios.bin')), romfs: romfs.manifest(at('romfs', 'romfs.txt')) });
  fs.writeFileSync(at('bin', 'prom.bin'), image);
  fs.writeFileSync(at('obj', 'build.json'), JSON.stringify({ clock: opt.clock || 1, acia: opt.acia || 'rockwell' }) + '\n');

  const report = budget.report(ROOT, { modules, tests, progs, programs, samples, entries });
  say(report.text);
  if (disk) {
    const bytes = disk.files.reduce((n, f) => n + f.size, 0), used = disk.volume.length / 512, first = disk.start / 32;
    say('ROM disk: ' + disk.files.length + ' files, ' + bytes + ' bytes; its volume uses ' + used + ' of ' + disk.blocks + ' blocks (paged ROM banks ' +
      first + '-' + (first + Math.ceil(used / 32) - 1) + '); every file read back as its source');
  }
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
module.exports = { build, buildModule, readManifest, hwtest, prog };
