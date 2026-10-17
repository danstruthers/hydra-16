#!/usr/bin/env node
// ****************************************************************************
// build.js - builds the base (docs/design/plans/BASE.md, in reborn/docs): the Hydra-16's kernel, the system calls
// and the parts every system on it shares:
//   1. tools/apigen.js     spec/api.def, errors.def -> the jump table, the error codes and texts, api.json (obj/gen),
//                          and the assembly SDK's hydra.inc (sdk/asm: in Git)
//   2. the kernel          kernel/*.s and the generated sources -> bin/bios.bin (the 128K BIOS ROM), with its map,
//                          labels and debug information in obj/kernel/
//   3. the modules         modules/NAME/*.s -> obj/modules/NAME.bin: ser (the console's driver, task F), kdev (the
//                          kernel's devices, task E), wozmon (the monitor, init: task 1); and the test modules,
//                          tests/mod/NAME/*.s -> obj/tests/NAME.bin
//   4. the paged ROM       modules/rom.txt -> bin/prom0.bin (tools/romimg.js): the module directory, the hardware
//                          test in bank 1, the modules (one 512K chip, as few banks as it needs; the rest $FF)
//   5. the budgets         the BIOS ROM's pages, the modules: used, and room left (tools/budget.js)
// All of it twice, once for each serial chip U3 can be (ACIA_CHIP, the kernel's alone: Rockwell's R65C51, WDC's
// W65C51N), and the images named for the chip where the two builds' differ (tools/images.js): the BIOS ROM's,
// bin/bios_rockwell.bin and bios_wdc.bin.  The paged ROM comes out the same for both (its drivers ask the BIOS, and
// the BIOS's checksums are fixed at 0: fixSums), one file, bin/prom0.bin.  obj/ is the last build's: the Rockwell
// one's, unless --acia wdc.
// HydraOS (../reborn/build.js) runs this first, and builds its modules, its paged ROM and its SDKs with what this
// gives: buildModule (a module, or a RAM program, with the base's includes: sdk/asm, include, obj/gen, lib),
// assemble, the cc65 tools, the hardware test, readManifest (a rom.txt).
// The cc65 tools: $CC65_BIN, else $CC65_HOME/bin (as the repository's CI sets it), else CC65_DIR/bin, else the PATH.
// CC65_DIR (below) is the one place the toolchain's folder is named: C:/source/cc65/win64_hydra, cc65 as the Hydra
// builds with it, upstream's 6bd01e8a5 (October 2026) with four fixes of its code generator and optimizer
// (tools/cc65/*.patch: the cc65 clone's branch hydra-optfix; the folder's VERSION.txt says so).  Its ca65 knows WAI
// and STP as the W65C02's (--cpu W65C02), not the 65C02's (Rockwell's) as 2.19's did.  reborn/build.js and its tools
// (sim/lib/hubown.js, sim/tools/nprof.js) find the tools by tool () here.
//
// Usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--quiet]
//   --clock 2: for a 7.16 MHz board (jumper J8); --acia: the chip whose build is the last, its objects left in obj/
//   for the tests (rockwell unless it's wdc: a WDC W65C51N, with its TDRE bug's workaround)
// From Node: require('./build.js').buildBoth(opts) does the same, and gives what the last build made; build(opts)
// builds for opts.acia alone, its images bin/bios.bin, prom0.bin ... (buildBoth names them).
'use strict';
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');
const apigen = require('./tools/apigen.js');
const budget = require('./tools/budget.js');
const check = require('./tools/check.js');
const bankcheck = require('./tools/bankcheck.js');
const romimg = require('./tools/romimg.js');
const images = require('./tools/images.js');

const ROOT = __dirname;
const at = (...p) => path.join(ROOT, ...p);

const CC65_DIR = 'C:/source/cc65/win64_hydra';

function tool(name) {
  for (const dir of [process.env.CC65_BIN, process.env.CC65_HOME && path.join(process.env.CC65_HOME, 'bin'), path.join(CC65_DIR, 'bin')]) {
    if (!dir) continue;
    for (const f of [path.join(dir, name + '.exe'), path.join(dir, name)]) if (fs.existsSync(f)) return f;
  }
  return name;
}
const CA65 = tool('ca65'), LD65 = tool('ld65');

function run(exe, args, cwd = ROOT) {
  try { execFileSync(exe, args, { cwd, stdio: ['ignore', 'pipe', 'pipe'] }); }
  catch (e) { throw new Error(path.basename(exe) + ' ' + args.filter(a => /\.(s|c|cfg)$/.test(a)).join(' ') + ':\n' + ((e.stderr || '') + (e.stdout || '')).toString().trim()); }
}
const mkdir = d => fs.mkdirSync(d, { recursive: true });
const sources = dir => fs.readdirSync(dir).filter(f => f.endsWith('.s')).sort().map(f => path.join(dir, f));

// Assemble files into objdir.  OUT: the objects
function assemble(files, objdir, includes, defines) {
  mkdir(objdir);
  return files.map(f => {
    const o = path.join(objdir, path.basename(f, '.s') + '.o');
    run(CA65, ['--cpu', 'W65C02', '-g', ...includes.flatMap(i => ['-I', i]), ...defines.flatMap(d => ['-D', d]),
      '-l', o.replace(/\.o$/, '.lst'), '-o', o, f]);
    return o;
  });
}

// The base's includes, for a module or a program: the SDK's (hydra.inc, made from spec/, and its core), the
// hardware's, the generated errors.inc, and the sources the base's modules share with HydraOS's (lib: serial.inc ...)
const INCLUDES = [at('sdk', 'asm'), at('include'), at('obj', 'gen'), at('lib')];

// A module's link: modules/module.cfg, or moduleN.cfg for N banks
const moduleCfg = banks => at('modules', banks > 1 ? 'module' + banks + '.cfg' : 'module.cfg');

// A module: the .s files in dir, linked with modules/module.cfg (moduleN.cfg, for N banks), or with dir's own
// NAME.cfg if it has one, whose files %O.LIBRARY are library modules of its own (hylang's: put in libs, by name); or
// a RAM program (ram: assembled with HYX2_RAM, linked with sdk/asm/hyx2.cfg, NAME.hyx).  Its includes: includes
// (the base's, INCLUDES, by default), its own folder and the one above it.  OUT: its image (a Buffer)
function buildModule(dir, objdir, defines, ram = false, libs = {}, includes = INCLUDES) {
  const name = path.basename(dir), od = path.join(objdir, name);
  const objs = assemble(sources(dir), od, [...includes, dir, path.dirname(dir)], ram ? [...defines, 'HYX2_RAM'] : defines);
  const bin = path.join(objdir, name + (ram ? '.hyx' : '.bin'));
  const banks = Math.max(1, ...sources(dir).map(f => Math.max(0, ...[...fs.readFileSync(f, 'latin1').matchAll(/\.segment\s+"CODE([2-8])"/gi)]
    .map(m => +m[1]))));                                      // (Its last bank's CODEn: moduleN.cfg)
  const own = path.join(dir, name + '.cfg');
  const cfg = ram ? at('sdk', 'asm', 'hyx2.cfg') : fs.existsSync(own) ? own : moduleCfg(banks);
  const dbg = path.join(od, name + '.dbg');
  run(LD65, ['-C', cfg, '-o', bin, '-m', path.join(od, name + '.map'), '-Ln', path.join(od, name + '.lbl'), ...(ram ? [] : ['--dbgfile', dbg]), ...objs]);
  const data = fs.readFileSync(bin);
  check.checkModule(name, data, ram ? 0x0800 : 0xA000);       // (Only the kernel writes T, V and W)
  if (!ram) {                                                   // (A module of banks: none uses another's directly)
    const bad = bankcheck.check(fs.readFileSync(dbg, 'latin1'));
    if (bad.length) throw new Error(name + ': an instruction in one bank uses a label in another (tools/bankcheck.js):\n' +
      bad.map(b => '  ' + path.relative(ROOT, b.file) + ':' + b.line + ': bank ' + b.from + ', ' + b.label + ' bank ' + b.to + ': ' + b.text).join('\n'));
  }
  if (cfg === own) for (const [, lib] of fs.readFileSync(own, 'latin1').matchAll(/"%O\.(\w+)"/g)) {
    const file = path.join(objdir, lib + '.bin');               // (A library module of its own: NAME.bin, as any module)
    fs.renameSync(bin + '.' + lib, file);
    libs[lib] = fs.readFileSync(file);
    check.checkModule(lib, libs[lib], 0xA000);
  }
  return data;
}

// May the module in dir move (be packed in a bank with others: romimg.js's plan)?  A program or a driver of a bank,
// linked by modules/module.cfg (not a library, nor one with a link of its own)
function packable(dir, data) {
  return data[33] === 1 && (data[5] === 1 || data[5] === 2) && !fs.existsSync(path.join(dir, path.basename(dir) + '.cfg'));
}

// The module in dir (buildModule's objects in objdir) linked again at page `page` of its bank ($A000 + 256 * page:
// romimg.js's plan), its image, map and labels into outdir.  OUT: its image (as long as it was: the same code, at
// another address)
function relinkModule(dir, objdir, outdir, page, length) {
  const name = path.basename(dir), od = path.join(objdir, name), out = path.join(outdir, name);
  const objs = sources(dir).map(f => path.join(od, path.basename(f).replace(/\.s$/, '.o')));
  mkdir(out);
  const base = fs.readFileSync(moduleCfg(1), 'latin1'), start = 0xA000 + page * 256;
  const text = base.replace(/(ROM:\s+start = )\$A000, size = \$4000/, '$1$' + start.toString(16).toUpperCase() + ', size = $' +
    (0x4000 - page * 256).toString(16).toUpperCase());
  if (text === base) throw new Error('modules/module.cfg: no ROM at $A000 to move');
  const cfg = path.join(out, name + '.cfg'), bin = path.join(outdir, name + '.bin'), dbg = path.join(out, name + '.dbg');
  fs.writeFileSync(cfg, text);
  run(LD65, ['-C', cfg, '-o', bin, '-m', path.join(out, name + '.map'), '-Ln', path.join(out, name + '.lbl'), '--dbgfile', dbg, ...objs]);
  const data = fs.readFileSync(bin);
  if (data.length !== length) throw new Error(name + ': ' + data.length + ' bytes linked at $' + start.toString(16) + ', ' + length + ' at $A000');
  check.checkModule(name, data, start);
  return data;
}

// The hardware test: bank 1 of the old system's paged ROM image (old/os_rom/bin, in Git), or null without it
const HWTEST_IMAGE = path.join(ROOT, '..', 'old', 'os_rom', 'bin', 'paged_rom_C02.bin');
function hwtest() {
  return fs.existsSync(HWTEST_IMAGE) ? fs.readFileSync(HWTEST_IMAGE) : null;
}

// A rom.txt: { init, modules: [names] }
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

// The build's defines, from its options (the modules' and programs': the serial chip is the kernel's alone,
// kernelDefines, so the paged ROM is the same for both)
function definesOf(opt) {
  const defines = [];
  if (opt.clock) defines.push('CPU_CLOCK_MULT=' + opt.clock);
  return defines;
}
function kernelDefines(opt) {
  return [...definesOf(opt), 'ACIA_CHIP=' + (opt.acia === 'wdc' ? 1 : 0)];
}

// Each BIOS page's checksum (romimg's CRC of $E000-$FEFF, the hardware test's) made 0: its last two bytes
// (BIOS_SUMFIX, $FEFE) the CRC of the rest, high first.  So the paged ROM's table of them is the same for every BIOS
const SUMFIX = 0x1EFE;
function fixSums(bios) {
  for (let p = 0; p * romimg.PAGE < bios.length; p++) {
    const at = p * romimg.PAGE;
    if (bios.readUInt16LE(at + SUMFIX) !== 0) throw new Error('BIOS page ' + p + ': $FEFE is not the checksum fix (bios.cfg)');
    bios.writeUInt16BE(romimg.crc16(i => bios[at + i], SUMFIX), at + SUMFIX);
    if (romimg.crc16(i => bios[at + i], SUMFIX + 2) !== 0) throw new Error('BIOS page ' + p + ': its checksum is not 0');
  }
  return bios;
}

function build(opt = {}) {
  const say = opt.quiet ? () => {} : s => console.log(s);
  const defines = definesOf(opt);
  apigen.generate(ROOT);
  mkdir(at('bin'));

  // The kernel
  const kobj = at('obj', 'kernel');
  const kfiles = [...sources(at('kernel')), at('obj', 'gen', 'jumptable.s'), at('obj', 'gen', 'errtext.s')];
  const objs = assemble(kfiles, kobj, [at('include'), at('obj', 'gen'), at('kernel')], kernelDefines(opt));
  run(LD65, ['-C', at('kernel', 'bios.cfg'), '-o', at('bin', 'bios.bin'), '-m', path.join(kobj, 'bios.map'), '-Ln', path.join(kobj, 'bios.lbl'),
    '--dbgfile', path.join(kobj, 'bios.dbg'), ...objs]);
  fs.writeFileSync(at('bin', 'bios.bin'), fixSums(fs.readFileSync(at('bin', 'bios.bin'))));
  fs.writeFileSync(at('obj', 'build.json'), JSON.stringify({ clock: opt.clock || 1, acia: opt.acia || 'rockwell' }) + '\n');

  // The base's modules (modules/NAME/*.s: obj/modules/NAME.bin)
  const modules = {};
  for (const d of fs.readdirSync(at('modules'), { withFileTypes: true }).filter(d => d.isDirectory()))
    modules[d.name] = buildModule(at('modules', d.name), at('obj', 'modules'), defines, false, modules);
  // Its test modules (tests/mod/NAME: obj/tests/NAME.bin; testlib.inc theirs, and HydraOS's test modules')
  const tests = {};
  for (const d of fs.readdirSync(at('tests', 'mod'), { withFileTypes: true }).filter(d => d.isDirectory()))
    tests[d.name] = buildModule(at('tests', 'mod', d.name), at('obj', 'tests'), defines);
  if (opt.modulesOnly) return { bios: fs.readFileSync(at('bin', 'bios.bin')), defines, modules, tests };

  // The base's paged ROM: its rom.txt's modules, its init
  const manifest = readManifest(at('modules', 'rom.txt'));
  for (const n of manifest.modules) if (!modules[n]) throw new Error('modules/rom.txt: no module ' + n);
  const bios = fs.readFileSync(at('bin', 'bios.bin'));
  const { image, chips } = romimg.build({ modules: manifest.modules.map(n => ({ file: n, data: modules[n] })), init: manifest.init,
    hwtest: hwtest(), bios });
  for (const f of fs.readdirSync(at('bin')).filter(f => /^prom\d*\.bin$/.test(f))) fs.rmSync(at('bin', f));   // (The last build's)
  for (let k = 0; k < chips; k++) fs.writeFileSync(at('bin', 'prom' + k + '.bin'), image.subarray(k * romimg.CHIP, (k + 1) * romimg.CHIP));
  if (!opt.noReport) {
    say(budget.report(ROOT, { modules, tests }).text);
    say('Paged ROM: ' + manifest.modules.join(', ') + ' (init ' + manifest.init + '): bin/prom0.bin');
  }
  return { bios, defines, modules, tests, manifest };
}

// A build's images in dir (names: a RegExp), its plain ones (NAME.bin, not NAME_rockwell.bin ...), read and removed
const IMAGE_NAMES = /^(bios|prom\d+)(_(rockwell|wdc))?\.bin$/;
function takeImages(dir, names) {
  const out = {};
  for (const f of fs.readdirSync(dir).filter(f => names.test(f) && !/_(rockwell|wdc)\./.test(f))) {
    out[f] = fs.readFileSync(path.join(dir, f));
    fs.rmSync(path.join(dir, f));
  }
  return out;
}

// The two builds, one for each serial chip, the other's first and opts.acia's (rockwell's) last, and the images
// written for both (images.js).  dirs: each folder of images (the base's bin; HydraOS's adds its own) and its names.
// OUT: the last build's result
function buildBoth(opt = {}, buildOne = build, dirs = [[at('bin'), IMAGE_NAMES]]) {
  const last = opt.acia || 'rockwell';
  if (!images.ACIAS.includes(last)) throw new Error('--acia rockwell or wdc');
  const order = [images.ACIAS.find(a => a !== last), last], made = dirs.map(() => ({}));
  let result;
  for (const acia of order) {
    if (!opt.quiet && acia !== last) console.log('(Built for the ' + (acia === 'wdc' ? 'WDC W65C51N' : 'Rockwell R65C51') + ' first: its images only)');
    result = buildOne(Object.assign({}, opt, { acia, quiet: opt.quiet || acia !== last }));
    dirs.forEach(([dir, names], i) => { made[i][acia] = takeImages(dir, names); });
  }
  const files = dirs.map(([dir, names], i) => images.write(dir, made[i], names).map(f => (path.relative(process.cwd(), dir) || '.').split(path.sep).join('/') + '/' + f));
  if (!opt.quiet) console.log('Images (R65C51 and W65C51N): ' + files.flat().join(', '));
  return result;
}

if (require.main === module) {
  const a = process.argv.slice(2), opt = {};
  for (let i = 0; i < a.length; i++) {
    if (a[i] === '--clock') opt.clock = +a[++i];
    else if (a[i] === '--acia') opt.acia = a[++i];
    else if (a[i] === '--quiet') opt.quiet = true;
    else { console.error('usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--quiet]'); process.exit(2); }
  }
  try { buildBoth(opt); } catch (e) { console.error('build: ' + e.message); process.exit(1); }
}
module.exports = { build, buildBoth, fixSums, kernelDefines, takeImages, IMAGE_NAMES, buildModule, packable, relinkModule, assemble, run, tool, sources, readManifest, hwtest, definesOf, moduleCfg, INCLUDES, CA65, LD65, ROOT, HWTEST_IMAGE, CC65_DIR };
