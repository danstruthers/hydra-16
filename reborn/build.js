#!/usr/bin/env node
// ****************************************************************************
// build.js - builds everything (docs/reimplementation-from-scratch.md, phase 0):
//   1. tools/apigen.js     spec/ -> the jump table, the error codes and texts, the SDK's hydra.inc, the reference
//   2. the kernel          kernel/*.s and the generated sources -> bin/bios.bin (the 128K BIOS ROM), with its map
//                          and labels in obj/kernel/
//   3. the modules         modules/NAME/*.s -> obj/modules/NAME.bin; tests/mod/NAME/*.s -> obj/tests/NAME.bin;
//                          each checked: only the kernel writes T, V and W (tools/check.js)
//   4. the paged ROM       modules/rom.txt -> bin/prom.bin (tools/romimg.js), with the hardware test in bank 1
//                          (from ../os_rom/bin/paged_rom_C02.bin) and the ROMs' checksums for it
//   5. the budgets         sizes, and room left (tools/budget.js)
// and obj/build.json: the options it was built with ({ clock, acia }: sim/test.js's budgets can depend on them).
//
// Usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--quiet]
//   --clock 2: for a 7.16 MHz board (jumper J7); --acia wdc: a WDC W65C51N in the serial port (its TDRE bug)
// The cc65 tools: $CC65_BIN, else $CC65_HOME/bin (as the repository's CI sets it), else
// C:/source/cc65/win64_snapshot/bin, else the PATH.
// From Node: require('./build.js').build(opts) does the same, and gives what it made.
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const apigen = require('./tools/apigen.js');
const romimg = require('./tools/romimg.js');
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

// A module: the .s files in dir, linked with modules/module.cfg.  OUT: its image (a Buffer)
function buildModule(dir, objdir, defines) {
  const name = path.basename(dir), od = path.join(objdir, name);
  const objs = assemble(sources(dir), od, [at('obj', 'sdk'), at('sdk', 'asm'), at('include'), dir, path.dirname(dir)], defines);
  const bin = path.join(objdir, name + '.bin');
  run(LD65, ['-C', at('modules', 'module.cfg'), '-o', bin, '-m', path.join(od, name + '.map'), '-Ln', path.join(od, name + '.lbl'), ...objs]);
  const data = fs.readFileSync(bin);
  check.checkModule(name, data);                              // (Only the kernel writes T, V and W)
  return data;
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

  // The modules and the test modules
  const modules = {}, tests = {};
  for (const d of fs.readdirSync(at('modules'), { withFileTypes: true }).filter(d => d.isDirectory()))
    modules[d.name] = buildModule(at('modules', d.name), at('obj', 'modules'), defines);
  if (fs.existsSync(at('tests', 'mod')))
    for (const d of fs.readdirSync(at('tests', 'mod'), { withFileTypes: true }).filter(d => d.isDirectory()))
      tests[d.name] = buildModule(at('tests', 'mod', d.name), at('obj', 'tests'), defines);

  // The paged ROM
  const manifest = readManifest(at('modules', 'rom.txt'));
  for (const n of manifest.modules) if (!modules[n]) throw new Error('modules/rom.txt: no module ' + n);
  const hwt = hwtest();
  if (!hwt) say('(no ' + path.relative(ROOT, HWTEST_IMAGE) + ': the paged ROM has no hardware test)');
  const { image, entries } = romimg.build({ modules: manifest.modules.map(n => ({ file: n, data: modules[n] })), init: manifest.init,
    hwtest: hwt, bios: fs.readFileSync(at('bin', 'bios.bin')) });
  fs.writeFileSync(at('bin', 'prom.bin'), image);
  fs.writeFileSync(at('obj', 'build.json'), JSON.stringify({ clock: opt.clock || 1, acia: opt.acia || 'rockwell' }) + '\n');

  const report = budget.report(ROOT, { modules, tests, entries });
  say(report.text);
  return { modules, tests, manifest, report };
}

if (require.main === module) {
  const a = process.argv.slice(2), opt = {};
  for (let i = 0; i < a.length; i++) {
    if (a[i] === '--clock') opt.clock = +a[++i];
    else if (a[i] === '--acia') opt.acia = a[++i];
    else if (a[i] === '--quiet') opt.quiet = true;
    else { console.error('usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--quiet]'); process.exit(2); }
  }
  try { build(opt); } catch (e) { console.error('build: ' + e.message); process.exit(1); }
}
module.exports = { build, buildModule, readManifest, hwtest };
