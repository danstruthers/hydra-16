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
//   5. the budgets         sizes, and room left (../base/tools/budget.js); and the games' room (headroom: each C
//                          game's main RAM free, its overlays' and swapped parts'): the build failed if one of
//                          Dingus Quest Zero's tasks, or the theme compiler or generator, has less main RAM free
//                          than its floor (FLOORS)
//   6. the SDK             bin/sdk/asm: the assembly SDK whole, to take away (sdk/asm, the generated hydra.inc and
//                          numbers.inc, asmlib.inc, the samples' sources); the C library, obj/sdk/c/hydra.lib (cc65's none.lib
//                          with sdk/c/lib's modules), and bin/sdk/c: the C SDK whole (sdk/c, the generated
//                          hydracalls.h, numdefs.h and asmdefs.h, the library, the samples' sources)
// A program's folder (programs/, tests/ram/, a sample's) with a .c in it is a C program: its .c and .s files compiled
// (cc65 -t none) and linked with the C library and sdk/c/hydra.cfg (sdk/c/README.md); the C samples are
// sdk/c/samples/NAME -> obj/samples/c/NAME.hyx.
// and obj/build.json: the options it was built with ({ clock, acia }: sim/test.js's budgets can depend on them).
// The kernel for each serial chip U3 can be (Rockwell's R65C51, WDC's W65C51N: the kernel's ACIA_CHIP, its alone), the
// rest once (GAME-CRITIQUE.md's 7.2, 2026-11-17: before, all of it twice, the rest coming out the same), and the images
// named for the chip where they differ (../base/tools/images.js): the BIOS ROM's, bin/bios_rockwell.bin and
// bios_wdc.bin, and the base's the same way in ../base/bin; the rest one file each (prom0.bin, prom1.bin, prom2.bin,
// sdcard.img).  obj/ is the last build's: the Rockwell one's, unless --acia wdc.
// cc65's, ca65's and od65's runs are kept in obj/cache (../base/tools/objcache.js: each by a hash of everything it
// read, the tool's own file, its flags and the environment it reads), and a run whose inputs are all as they were
// isn't run again; cc65 and ca65 are given the repository's files by their names from here (../base's, from there),
// so what they make, and the cache, are the same wherever the tree is.
//
// Usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--both] [--no-cache | --clean] [--quiet]
//   --clock 2: for a 7.16 MHz board (jumper J8); --acia: the chip whose build is the last, its objects left in obj/
//   for the tests (rockwell unless it's wdc: a WDC W65C51N, with its TDRE bug's workaround); --both: the whole build
//   for each chip, as before (the check that nothing but the BIOS ROM depends on the chip); --no-cache: every tool
//   run, nothing kept; --clean: the cache emptied first (every tool run, and kept)
//        node build.js games [--no-cache | --clean] [--quiet]
//   only what the games are made of, on the last whole build (buildGames): for a change in games/ (or bank/'s
//   programs); the whole build instead if there's none, or a module of the paged ROM changed (dq0lib is partly
//   games/'s)
//        node build.js prog DIR
//   a RAM program from DIR/*.s (with the SDK: sdk/asm/README.md), or DIR/*.c and *.s (a C program: sdk/c/README.md),
//   into DIR/NAME.hyx, NAME the folder's
// The cc65 tools: the base's (../base/build.js's tool (): $CC65_BIN, else $CC65_HOME/bin, else its CC65_DIR's bin on
// Windows, ../base/tools/cc65/win64_hydra/bin in the repository, else the PATH); cc65's own folders (include, asminc,
// lib): $CC65_HOME, else the folder above its tools'.  The assembler's CPU is the W65C02 (WAI, STP), the compiler's the 65C02.
// From Node: require('./build.js').buildBoth(opts) does the same, and gives what the last build made; build(opts)
// builds for opts.acia alone, its images bin/bios.bin, prom0.bin ... (buildBoth names them); buildGames(opts) the
// games' build.
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
const objcache = require('../base/tools/objcache.js');

const ROOT = __dirname;
const at = (...p) => path.join(ROOT, ...p);
const atBase = (...p) => path.join(base.ROOT, ...p);

const tool = base.tool;
const CA65 = tool('ca65'), LD65 = tool('ld65'), CC65 = tool('cc65'), AR65 = tool('ar65'), OD65 = tool('od65');

// cc65's home: $CC65_HOME, else the folder above the one its tools are in
function cc65Home() {
  if (process.env.CC65_HOME) return process.env.CC65_HOME;
  let bin = path.dirname(CA65);
  if (!path.isAbsolute(CA65))       // (On the PATH)
    bin = (process.env.PATH || '').split(path.delimiter).find(d => d && (fs.existsSync(path.join(d, 'ca65.exe')) || fs.existsSync(path.join(d, 'ca65')))) || '.';
  return path.dirname(bin);
}

// A tool run; its errors thrown.  cc65 and ca65 are given the repository's files by their names from here (so what
// they make names them so: the same wherever the tree is), and run through the cache in use, if there's one
// (../base/tools/objcache.js).  od65's output (its stdout), the cache's too
function run(exe, args) {
  const cache = objcache.current(), own = objcache.knows(exe), a = own ? objcache.relative(args, ROOT) : args;
  try {
    if (cache && own) cache.run(exe, a, ROOT);
    else execFileSync(exe, a, { cwd: ROOT, stdio: ['ignore', 'pipe', 'pipe'] });
  } catch (e) { throw new Error(path.basename(exe) + ' ' + args.filter(a => /\.(s|c|cfg)$/.test(a)).join(' ') + ':\n' + ((e.stderr || '') + (e.stdout || '') || e.message).toString().trim()); }
}
const od65 = args => objcache.current() ? objcache.current().output(OD65, args) : execFileSync(OD65, args, { encoding: 'latin1' });
// A tool's runs (each its arguments), in turn; those the cache in use hasn't run first side by side (objcache.js's
// ahead: the runs then taken from it)
function runAll(exe, list) {
  const cache = objcache.current();
  if (cache && objcache.knows(exe) && list.length > 1)
    try { cache.ahead(list.map(a => ({ exe, args: objcache.relative(a, ROOT), cwd: ROOT }))); } catch (e) { /* (Each run in turn, as ever) */ }
  for (const a of list) run(exe, a);
}
const mkdir = d => fs.mkdirSync(d, { recursive: true });
const sources = dir => fs.readdirSync(dir).filter(f => f.endsWith('.s')).sort().map(f => path.join(dir, f));
const csources = dir => fs.readdirSync(dir).filter(f => /\.[cs]$/.test(f)).sort().map(f => path.join(dir, f));
const isC = dir => fs.readdirSync(dir).some(f => f.endsWith('.c'));

// Assemble files into objdir.  OUT: the objects
function assemble(files, objdir, includes, defines) {
  mkdir(objdir);
  const objs = files.map(f => path.join(objdir, path.basename(f, '.s') + '.o'));
  runAll(CA65, files.map((f, i) => ['--cpu', 'W65C02', '-g', ...includes.flatMap(i => ['-I', i]), ...defines.flatMap(d => ['-D', d]),
    '-l', objs[i].replace(/\.o$/, '.lst'), '-o', objs[i], f]));
  return objs;
}

// HydraOS's includes, then the base's (its hydra.inc, the SDK's core, the hardware's, errors.inc): a module's, a
// program's; a test module's, the base's testlib.inc too
const INCLUDES = [at('sdk', 'asm'), ...base.INCLUDES, at('obj', 'gen'), atBase('tests', 'mod')];

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
    cc: ['-g', '-t', 'none', '--cpu', '65C02', '-O', '-I', at('sdk', 'c', 'include'), ...own('include'), '-I', at('obj', 'gen')],
    as: ['-g', '--cpu', 'W65C02', '-I', at('sdk', 'asm'), '-I', atBase('sdk', 'asm'), '-I', at('sdk', 'c', 'lib'), '-I', at('sdk', 'c', 'include'), ...own('asminc'),
      '-I', at('obj', 'gen')],
  };
}

// Dingus Quest Zero's generated files, before its tasks and their make.js files (tools/dq0build.js's generate: the hub's
// numbers for the other languages, from the C headers as cc65 sees them, and the records' header, games/dq0/hubgen.js;
// its phrase table, games/dq0/saidgen.js), cc65 as the C target has it
function hubNumbers(say = () => {}) {
  const home = cc65Home(), own = d => fs.existsSync(path.join(home, d)) ? ['-I', path.join(home, d)] : [];
  return require('./tools/dq0build.js').generate({ say, cc65: { bin: CC65, flags: ['-t', 'none', '--cpu', '65C02',
    '-I', at('sdk', 'c', 'include'), ...own('include'), '-I', at('games', 'dq0'), '-I', at('obj', 'gen')] } });
}

// C and assembly sources into objdir (a .c: cc65 makes its .s there, then ca65).  OUT: the objects
function compileC(files, objdir, extra = [], asExtra = []) {
  mkdir(objdir);
  const fl = cflags(), name = f => path.basename(f).replace(/\.[cs]$/, ''), s = f => f.endsWith('.c') ? path.join(objdir, name(f) + '.s') : f;
  runAll(CC65, files.filter(f => f.endsWith('.c')).map(f => [...fl.cc, ...extra, '-o', s(f), f]));   // (The C, then all of it
  runAll(CA65, files.map(f => [...fl.as, ...asExtra, '-l', path.join(objdir, name(f) + '.lst'), '-o', path.join(objdir, name(f) + '.o'), s(f)]));   //   assembled)
  return files.map(f => path.join(objdir, name(f) + '.o'));
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

// A bank program (docs/design/plans/RUN.md: dir/bank says so, "run", or "task" for one that runs only in a task of its
// own): dir/*.s assembled with HYX2_BANK and linked by sdk/asm/bank.cfg into an o65 module at $8000, its file
// (objdir/NAME.hyx) a copy of its own HYX2 header (HF_BANK set, and HF_TASK) then the module.  OUT: its file's bytes
const HF_BANK = 0x04, HF_TASK = 0x08;
function bankProgram(dir, objdir, defines) {
  const name = path.basename(dir), od = path.join(objdir, name), kind = fs.readFileSync(path.join(dir, 'bank'), 'latin1').trim().split(/\s+/)[0];
  if (kind !== 'run' && kind !== 'task') throw new Error(name + '/bank: "run" or "task", not "' + kind + '"');
  const objs = base.assemble(sources(dir), od, [...INCLUDES, dir, path.dirname(dir)], [...defines, 'HYX2_BANK']);
  const o65 = path.join(od, name + '.o65');
  run(LD65, ['-C', atBase('sdk', 'asm', 'bank.cfg'), '-o', o65, '-m', path.join(od, name + '.map'), '-Ln', path.join(od, name + '.lbl'), ...objs]);
  const m = fs.readFileSync(o65);
  let p = 26;                                                   // (Its text: past the o65 header and its options)
  while (m[p]) p += m[p];
  const hdr = Buffer.from(m.subarray(p + 1, p + 49));
  if (hdr.toString('latin1', 0, 4) !== 'HYX2') throw new Error(name + ': a bank program\'s text starts with its HYX2 header');
  hdr[6] |= HF_BANK | (kind === 'task' ? HF_TASK : 0);
  const w = i => m[i] | m[i + 1] << 8, room = w(10) + w(14) + w(18) + 41 + w(22) + w(14);   // (In its bank: its image, BSS,
  hdr.writeUInt16LE(48 + 27 + w(10) + w(14), 10);               // (HX_LENGTH: its file's bytes to its data's end)
  if (room > 0x1E00) throw new Error(name + ": a bank program, its frame and its data's copy are " + room + " bytes, past a bank's 7,680");   //   frame and data's copy)
  const out = Buffer.concat([hdr, o65Bare(m)]);
  fs.writeFileSync(path.join(objdir, name + '.hyx'), out);
  return out;
}

// A game's swapped parts and overlays checked in its sources: the compiler's output (objdir's .s of each .c) and its
// own assembly (the folder's .s, and what they .include of its own: ../dq0w/at.s ...); a label's place the segment
// it's defined in, the C's and the assembly's together (a C function in the main RAM calling a part's assembly,
// cast.s's, or the assembly a part's C).  The stubs (swap.inc's SWFN: the macro builds the part's name) and what's in
// a macro's body aren't references.  Throws, with each:
// - a part (its config's SW_name segments: code in the main RAM's area only while it's called, through its stub;
//   games/dq0/swap.inc): no reference to a function or data of one from outside it (the main code's, an overlay's,
//   another part's), which would find another part's there;
// - an overlay (the config's OVn in the bank window, OVnT the rest of OVn's bank; not the world's KIN and CAT, the
//   hub's banks, which their callers put in first: cat_map): no reference to its function, literal, table or data
//   from a part's or another overlay's code or data (2026-11-19: fight.c's literals, cc65's all in the read-only data named at the file's
//   end, overlay 2's, copied by overlay 10's code with its own bank in the window).  The main RAM's code may name
//   them (bread, the stubs' ...).  The sanctioned way, a read through a call that maps the bank (said.c's RB, bank.s's
//   said_b; hub_mem), is a source's "bankcheck: banked NAME ..." (the label's name, C's without its _)
function partsChecked(sources, objdir, cfg) {
  const part = seg => (/^SW_(\w+)_(CODE|RODATA)$/.exec(seg) || [])[1] || '';   // (A part's segment: its name)
  const areas = new Map(), runs = new Map(), block = k => (cfg.split(new RegExp('^' + k + '\\s*\\{', 'm'))[1] || '').split('}')[0];
  for (const l of block('MEMORY').split(/\r?\n/)) {          // (Each area of the window: its bank's name)
    const m = /^\s*(\w+)\s*:[^;]*\bstart\s*=\s*\$([0-9A-Fa-f]+)/.exec(l.replace(/#.*/, ''));
    if (m && /^OV\d+/.test(m[1]) && parseInt(m[2], 16) >= 0x8000 && parseInt(m[2], 16) < 0xA000) areas.set(m[1], m[1].replace(/^(OV\d+)\D+$/, '$1'));
  }
  for (const l of block('SEGMENTS').split(/\r?\n/)) {
    const m = /^\s*(\w+)\s*:([^;]*);/.exec(l.replace(/#.*/, ''));
    if (m) runs.set(m[1], (/\brun\s*=\s*(\w+)/.exec(m[2]) || /\bload\s*=\s*(\w+)/.exec(m[2]) || [])[1]);
  }
  const bank = seg => part(seg) || areas.get(runs.get(seg)) || '';   // (A part's, a window area's, or '': the main RAM's)
  const banked = new Set();
  for (const f of sources) for (const [, l] of fs.readFileSync(f, 'latin1').matchAll(/bankcheck:\s*banked\b([\w ]*)/g))
    l.trim().split(/\s+/).filter(Boolean).forEach(n => banked.add(n));
  const read = (file, seen) => {                        // (An assembly source's lines, its own .includes in place)
    if (seen.has(file)) return [];
    seen.add(file);
    const out = [];
    fs.readFileSync(file, 'latin1').split(/\r?\n/).forEach((l, i) => {
      const m = /^\s*\.include\s+"([^"]+)"/.exec(l), inc = m && path.resolve(path.dirname(file), m[1]);
      if (inc && fs.existsSync(inc)) out.push(...read(inc, seen));
      else out.push({ l, at: path.basename(file) + ' line ' + (i + 1) });
    });
    return out;
  };
  const files = sources.map(f => {                      // (Each label's segment: a C function's own, in its .proc;
    const asm = f.endsWith('.s');                       //   the file's; the program's, exported)
    const lines = asm ? read(f, new Set()) : fs.readFileSync(path.join(objdir, path.basename(f, '.c') + '.s'), 'latin1').split(/\r?\n/)
      .map((l, i) => ({ l, at: path.basename(f, '.c') + '.s line ' + (i + 1) }));
    const defs = new Map(), exported = new Set(), procOf = [], segOf = [], stack = [];
    let seg = 'CODE', proc = null, inMacro = false;
    lines.forEach(({ l }, i) => {
      let m;
      if (/^\s*\.macro\b/i.test(l)) inMacro = true;
      else if (/^\s*\.endmacro\b/i.test(l)) { inMacro = false; segOf[i] = null; return; }
      if (inMacro) { segOf[i] = null; return; }
      if ((m = /^\s*\.segment\s+"(\w+)"/.exec(l))) seg = m[1];
      else if ((m = /^\s*\.(code|rodata|data|bss|zeropage)\b/i.exec(l))) seg = m[1].toUpperCase();
      else if (/^\s*\.pushseg\b/i.test(l)) stack.push(seg);
      else if (/^\s*\.popseg\b/i.test(l)) seg = stack.pop() || 'CODE';
      else if ((m = /^\s*\.export\s+(.*)/.exec(l))) m[1].replace(/;.*/, '').split(',').forEach(n => exported.add(n.trim()));
      else if ((m = /^\.proc\s+(\w+)/.exec(l))) { defs.set(m[1], seg); proc = new Map(); }
      else if (/^\.endproc/.test(l)) proc = null;
      else if ((m = /^(\w+)\s*:/.exec(l))) (proc || defs).set(m[1], seg);
      procOf[i] = proc;
      segOf[i] = seg;
    });
    return { f, asm, lines, defs, exported, procOf, segOf };
  });
  const global = new Map();
  for (const { defs, exported } of files) for (const [n, s] of defs) if (exported.has(n)) global.set(n, s);
  const faults = [];
  for (const { f, asm, lines, defs, procOf, segOf } of files) {
    lines.forEach(({ l, at: where }, i) => {
      const seg = segOf[i];
      if (seg === null || /^\s*\.(dbg|proc|endproc|export|import|importzp|exportzp|fopt|setcpu|smart|autoimport|case|debuginfo|macpack|forceimport|segment|include|global|globalzp)\b/i.test(l)) return;
      const body = l.replace(/^\w+\s*:=?/, '').replace(/^\s*@\w+\s*:/, '').replace(/;.*/, '').replace(/"[^"]*"/g, '');
      if (asm && /^\s*(SWFN|SWPART)\b/.test(body)) return;      // (The stubs, the parts' records: swap.inc's)
      for (const [, n] of body.matchAll(asm ? /\b([A-Za-z_]\w*)\b/g : /\b(_\w+|[LMS][0-9A-F]{4})\b/g)) {
        const own = procOf[i], s = own && own.has(n) ? own.get(n) : defs.has(n) ? defs.get(n) : global.get(n), p = part(s || '');
        if (p && p !== part(seg)) faults.push(path.basename(f) + ': ' + n + ' (' + s + '\'s) used from ' + seg + ' (' + where + ')');
        else if (!p && s && bank(s) && bank(seg) && bank(s) !== bank(seg) && !banked.has(n.replace(/^_/, '')))
          faults.push(path.basename(f) + ': ' + n + ' (' + s + '\'s, ' + bank(s) + ') used from ' + seg + ' (' + bank(seg) + ', ' + where + ')');
      }
    });
  }
  if (faults.length) throw new Error('a swapped part\'s code or data reached from outside it, not through its stub, or an overlay\'s from ' +
    'another bank\'s code or data (build.js\'s partsChecked):\n  ' + faults.join('\n  '));
}

// A game's overlays' calls into its swapped parts checked for the window (savecrash's, 2026-11-19: "save", overlay 7's,
// called part 14's s_save_game, which left the hub's bank in the window, and the part's return went on into the hub's
// header as code).  A part's stub returns into the overlay's code at the window: the part, and what it calls, must
// leave the window as it found it.  From the C alone (its functions by their #pragma code-name: OVn's an overlay's,
// SW_name's a part's, s_NAME the part's function its stub NAME calls; a file's static functions its own): each call from
// an overlay's function to a part's stub, its part followed through what it calls (C functions; another part's
// through its stub) for a hub_map with no hub_back in the same function (hub_here ... hub_back puts the window back;
// hub_mem and the assembly's own aren't followed).  A call known to be safe (a path that maps nothing) is a source's
// "windowcheck: NAME" on its line or the two after.  Throws, with each
function windowChecked(sources) {
  const fns = new Map(), local = new Map();
  for (const f of sources.filter(f => f.endsWith('.c'))) {
    const lines = fs.readFileSync(f, 'latin1').split(/\r?\n/), stack = [], mine = new Map();
    let seg = 'CODE', m;
    local.set(f, mine);
    for (let i = 0; i < lines.length; i++) {
      const l = lines[i];
      if ((m = /^#pragma\s+code-name\s*\(\s*push\s*,\s*"(\w+)"\s*\)/.exec(l))) { stack.push(seg); seg = m[1]; continue; }
      if (/^#pragma\s+code-name\s*\(\s*pop\s*\)/.test(l)) { seg = stack.pop() || 'CODE'; continue; }
      if ((m = /^#pragma\s+code-name\s*\(\s*"(\w+)"\s*\)/.exec(l))) { seg = m[1]; continue; }
      if ((m = /^(static\s+)?(?!const\b|extern\b|typedef\b|#)[A-Za-z_][\w\s*]*?\b(\w+)\s*\(([^;{}]*)\)\s*$/.exec(l)) && lines[i + 1] === '{') {
        let j = i + 2;
        const body = [];
        while (j < lines.length && lines[j] !== '}') body.push(lines[j++]);
        const text = body.join('\n').replace(/\/\*[\s\S]*?\*\//g, ' ').replace(/"(?:[^"\\]|\\.)*"/g, '""');
        const fn = { name: m[2], seg, text, file: f, raw: body, at: i + 3 };
        (m[1] ? mine : fns).set(m[2], fn);
        i = j;
      }
    }
  }
  const find = (f, n) => local.get(f.file).get(n) || fns.get(n);
  const part = n => { const p = fns.get('s_' + n); return p && /^SW_/.test(p.seg) ? p : null; };
  const calls = t => [...new Set([...t.matchAll(/\b([A-Za-z_]\w*)\s*\(/g)].map(m => m[1]))];
  const memo = new Map();
  const leaves = (f, seen) => {                  // (Why f may return with another bank in the window; or null)
    if (memo.has(f)) return memo.get(f);
    if (seen.has(f)) return null;
    seen.add(f);
    let why = null;
    if (!/\bhub_back\s*\(/.test(f.text)) {
      if (/\bhub_map\s*\(/.test(f.text)) why = f.name + ' (hub_map)';
      else for (const c of calls(f.text)) {
        const g = part(c) || find(f, c), w = g && g !== f ? leaves(g, seen) : null;
        if (w) { why = f.name + ' -> ' + w; break; }
      }
    }
    memo.set(f, why);
    return why;
  };
  const faults = [];
  let n = 0;
  for (const f of [...fns.values(), ...[...local.values()].flatMap(m => [...m.values()])]) {
    if (!/^OV\d+/.test(f.seg)) continue;
    for (const c of calls(f.text)) {
      const p = local.get(f.file).has(c) ? null : part(c), w = p ? leaves(p, new Set()) : null;
      n += p ? 1 : 0;
      if (!w) continue;
      const k = f.raw.findIndex(l => new RegExp('\\b' + c + '\\s*\\(').test(l.replace(/\/\*.*?\*\//g, '')));
      if (new RegExp('windowcheck:\\s*' + c + '\\b').test(f.raw.slice(k, k + 3).join('\n'))) continue;
      faults.push(path.basename(f.file) + ':' + (f.at + k) + ' ' + f.name + ' (' + f.seg + ') calls ' + c + ' (' + p.seg + '\'s s_' + c + '): ' + w);
    }
  }
  if (faults.length) throw new Error('an overlay\'s call into a swapped part that may leave another bank in the window (build.js\'s windowChecked; ' +
    'put it back, hub_here ... hub_back, or mark a call known safe "windowcheck: NAME"):\n  ' + faults.join('\n  '));
  return n;
}

// The games' room: each C program of games/ (its map, obj/games/NAME/NAME.map, and its link, its own NAME.cfg or
// sdk/c/hydra.cfg): its main RAM free, from its BSS's end to its swapped parts' area (__SWAP_START__: games/dq0/
// swap.inc), or, with none, to the C stack's least room (__RAMTOP__ - __STACKMIN__); each overlay's bytes free (OVn,
// of its own size: 8K as a rule) and each swapped part's (SW_name, of the area's: __SWAPSIZE__, by name in their order:
// parts.txt's).  One table of them all;
// a program with less main RAM free than its floor fails the build (floorOf, the user's floors: GAME-CRITIQUE.md's
// 6.2 and 7.4; the UI's, dq0's, 3,000; every other task of Dingus Quest Zero's, dq0*, 2,048, as they run together and
// grow; the theme compiler and the theme generator 512, tools run alone between dungeons and by the build, not in
// play beside the tasks, the user's of 2026-11-13); another game's program short of TASK_FLOOR (hmchar: run alone,
// most of it the harn library's), only a warning.  And the headroom (the user's standing rule, 2026-11-17: a part or
// bank under 64 bytes free has its room won back before the next feature): each main RAM within HEADROOM of its
// floor, each overlay's and swapped part's bank with under HEADROOM free (marked !), listed.  Nothing else of a
// task's is past its BSS's end: no heap (a dq0 task that takes in malloc fails: cc65's heap runs from the BSS's end
// to the stack's, over the swap area, and isn't counted), the C stack's least room (__STACKMIN__) above the measure's
// top, its arguments at TASK_ARGS ($0350).  OUT: { text, faults }
const TASK_FLOOR = 2048, FLOORS = { dq0: 3000, themec: 512, themegen: 512 }, HEADROOM = 64;
const floorOf = name => FLOORS[name] !== undefined ? FLOORS[name] : TASK_FLOOR;
const floored = name => /^dq0/.test(name) || FLOORS[name] !== undefined;     // (Below its floor, the build fails)
function headroom(names) {
  const rows = [], faults = [], warns = [], lows = [], hex = n => '$' + n.toString(16).toUpperCase();
  const byNumber = (a, b) => parseInt(a[0].slice(2)) - parseInt(b[0].slice(2));
  for (const name of names) {
    const map = at('obj', 'games', name, name + '.map'), made = at('obj', 'games', name, name + '.cfg'),   // (A task's made
      own = fs.existsSync(at('games', name, 'parts.txt')) && fs.existsSync(made) ? made : at('games', name, name + '.cfg');   //   config, parts.js's)
    if (!fs.existsSync(map) || !isC(at('games', name))) continue;   // (An assembly program's room: its own, hyx2.cfg's)
    const m = fs.readFileSync(map, 'latin1'), cfg = fs.readFileSync(fs.existsSync(own) ? own : at('sdk', 'c', 'hydra.cfg'), 'latin1');
    const seg = {}, sym = {}, size = {}, load = {}, run = {};
    const list = (m.split('Segment list:')[1] || '').split('Exports list')[0];
    for (const [, n, e, z] of list.matchAll(/^(\w+)\s+[0-9A-F]{6}\s+([0-9A-F]{6})\s+([0-9A-F]{6})/gm)) seg[n] = { end: parseInt(e, 16), size: parseInt(z, 16) };
    for (const [, n, v] of m.matchAll(/\b(__\w+__)\s+([0-9A-F]{6})\s+R/g)) sym[n] = parseInt(v, 16);
    const block = k => (cfg.split(new RegExp('^' + k + '\\s*\\{', 'm'))[1] || '').split('}')[0];
    for (const [, n, z] of block('MEMORY').matchAll(/^\s*(\w+):[^;]*\bsize\s*=\s*\$([0-9A-Fa-f]+)/gm)) size[n] = parseInt(z, 16);
    for (const [, n, l, r] of block('SEGMENTS').matchAll(/^\s*(\w+):\s*load\s*=\s*(\w+)(?:,\s*run\s*=\s*(\w+))?/gm)) { load[n] = l; if (r) run[n] = r; }
    if (!seg.BSS || sym.__RAMTOP__ === undefined) throw new Error('games/' + name + ': no BSS, or no __RAMTOP__, in its map');
    const top = sym.__SWAP_START__ !== undefined ? sym.__SWAP_START__ : sym.__RAMTOP__ - sym.__STACKMIN__;
    const free = top - (seg.BSS.end + 1), task = /^dq0/.test(name), floor = floorOf(name);
    if (/\((_heap|malloc|calloc|realloc|free|_heapadd)\.o\)/.test(m) && task)   // (cc65's heap: from the BSS's end
      faults.push(name + ' takes in the heap (malloc: from its BSS\'s end up, into the swap area, uncounted)');   //   to the stack's)
    const used = by => { const u = {}; for (const [n, s] of Object.entries(seg)) if (by[n]) u[by[n]] = (u[by[n]] || 0) + s.size; return u; };
    const ov = Object.entries(used(load)).filter(([a]) => /^OV\d+$/.test(a) && size[a]).sort(byNumber);
    const order = [...block('MEMORY').matchAll(/^\s*(\w+):/gm)].map(m => m[1]), sw =Object.entries(used(run)).filter(([a]) => /^SW_\w+$/.test(a))   // (The parts in their
      .sort((a, b) => order.indexOf(a[0]) - order.indexOf(b[0]));                                         //   order, the config's)
    const short = [];                                   // (Its main RAM near its floor, its overlays and parts under HEADROOM)
    if (floored(name) && free >= floor && free - floor < HEADROOM) short.push('main RAM ' + free + ' (' + (free - floor) + ' over its ' + floor + ')');
    const room = (what, list, of) => {
      const s = [];
      const text = list.map(([a, u]) => {
        const left = of(a) - u;
        const n = what === 'part' ? a.slice(3) : a.slice(2);   // (A part by its name, SW_name's; an overlay by its number)
        if (left < HEADROOM) s.push(n + ' (' + left + ')');
        return n + ' ' + left + (of(a) !== 0x2000 && what === 'overlay' ? ' of ' + of(a) : '') + (left < HEADROOM ? '!' : '');
      }).join(', ');
      if (s.length) short.push(what + (s.length > 1 ? 's ' : ' ') + s.join(', '));
      return text;
    };
    if (free < floor) (floored(name) ? faults : warns).push(name + "'s main RAM: " + free + ' bytes free, under ' + floor);
    rows.push('  ' + name.padEnd(9) + String(free).padStart(6) + (free < floor ? '!' : ' ') + ' (' + hex(seg.BSS.end + 1) + '-' +
      hex(top - 1) + (sw.length ? '; the swap area ' + hex(top) + ', ' + sym.__SWAPSIZE__ + ' bytes)' : ')'));
    if (ov.length) rows.push('             overlays (of 8192):  ' + room('overlay', ov, a => size[a]));
    if (sw.length) rows.push('             parts (of ' + sym.__SWAPSIZE__ + '):' + ' '.repeat(Math.max(1, 6 - String(sym.__SWAPSIZE__).length)) + room('part', sw, () => sym.__SWAPSIZE__));
    const placed = at('obj', 'games', name, 'placed.txt'), moved = fs.existsSync(placed) && own === made ?   // (Parts the build
      fs.readFileSync(placed, 'latin1').split('\n').filter(Boolean).map(l => l.split(' ')) : [];               //   put in banks
    if (moved.length) rows.push('             parts not in parts.txt\'s banks (theirs full): ' +                //   not parts.txt's:
      moved.map(([n, f, t]) => n + ' in ' + t + ', not ' + f).join(', '));                                      //   games/dq0/parts.js)
    const fns = at('obj', 'games', name, 'moved.txt'), fmoved = fs.existsSync(fns) && own === made ?   // (Functions the build
      fs.readFileSync(fns, 'latin1').split('\n').filter(Boolean).map(l => l.split(' ')) : [];          //   moved from a part
    if (fmoved.length) rows.push('             moved from parts past the swap area: ' +               //   past the area: pack.js)
      fmoved.map(([n, f, t, b]) => n + ' (' + b + ') from ' + f + ' to ' + t).join(', '));
    if (short.length) lows.push(name + "'s " + short.join(' and '));
  }
  const text = ["The games' room (bytes free: the main RAM's, from the BSS's end; at least " + FLOORS.dq0 + " for the UI, " + TASK_FLOOR +
    " for dq0's other tasks, " + FLOORS.themec + ' for themec and themegen; ! short, or an ' +
    'overlay or a part under ' + HEADROOM + '):', ...rows, ...warns.map(w => '  warning: ' + w),
    ...(lows.length ? ['  headroom (the standing rule: room won back before the next feature), under ' + HEADROOM + ' bytes free or within ' +
      HEADROOM + ' of a floor:', ...lows.map(l => '    ' + l)] : [])].join('\n');
  return { text, faults };
}

// A game's task's overlays' count, one source: its config's (cfg) OVn areas (OV5T, the rest of OV5's bank, not one),
// given its C and its assembly as OVLS (critique 6.2: the count once kept by hand in three files, 2026-11-19; a 22nd
// overlay with bank.s's table left short overran it, 2026-10-31).  Throws at a count its sources keep by hand: an
// assembly's OVMAX, OVL_BANKS or OVBANKS a number, or its _ovbank's room; the C's ovbank[] a number, or OVLS,
// OVBANKS or ACCT_NOV #defined as one.  OUT: the compilers' arguments (-D OVLS=n)
function overlaysCounted(dir, cfg) {
  const mem = (fs.readFileSync(cfg, 'latin1').split(/^MEMORY\s*\{/m)[1] || '').split('}')[0];
  const n = (mem.replace(/#.*/g, '').match(/^\s*OV\d+\s*:/gm) || []).length, faults = [];
  for (const f of fs.readdirSync(dir).filter(f => /\.(s|inc|c|h)$/.test(f))) {
    fs.readFileSync(path.join(dir, f), 'latin1').split(/\r?\n/).forEach((l, i) => {
      const s = /\.(s|inc)$/.test(f) ? l.replace(/;.*/, '') : l.replace(/\/\*.*?(\*\/|$)/g, '').replace(/\/\/.*/, '');
      if (/\.(s|inc)$/.test(f) ? /^\s*(OVMAX|OVL_BANKS|OVBANKS)\s*=\s*\d|^_ovbank:\s*\.res\s+\d/.test(s) :
        /\bovbank\s*\[\s*\d+\s*\]\s*;/.test(s) && /\bextern\b/.test(s) || /^\s*#\s*define\s+(OVLS|OVBANKS|ACCT_NOV)\s+\d/.test(s))
        faults.push(f + ' line ' + (i + 1) + ': ' + l.trim());
    });
  }
  if (faults.length) throw new Error(path.basename(dir) + ': its overlays\' count kept by hand (its config\'s ' + n + ' OVn, OVLS: build.js\'s ' +
    'overlaysCounted):\n  ' + faults.join('\n  '));
  return ['-D', 'OVLS=' + n];
}

// A C program: dir's .c and .s files, linked with the C library and sdk/c/hydra.cfg into out (a RAM program; its
// map and labels in objdir); or with the folder's own NAME.cfg, if it has one (a game's, with its overlays: files
// beside out that the cfg names; its overlays' count, OVLS, given its C and its assembly: overlaysCounted).  OUT: its
// image
function cprog(dir, objdir, out, extra = []) {
  const name = path.basename(out).replace(/\.hyx$/, ''), parts = path.join(dir, 'parts.txt');
  let own = path.join(dir, path.basename(dir) + '.cfg'), objs;
  const ovls = fs.existsSync(own) ? overlaysCounted(dir, own) : [], cc = [...extra, ...ovls];
  if (fs.existsSync(parts)) {                           // (A game's task whose swapped parts the build makes:
    objs = require('./tools/dq0build.js').swappedParts(dir, objdir, cc, { cc65: args => run(CC65, args),   //   tools/
      cc65all: list => runAll(CC65, list), od65, csources, flags: cflags(),                               //   dq0build.js's)
      compileC: (files, od, ex, asEx = []) => compileC(files, od, ex, [...asEx, ...ovls]) });
    own = path.join(objdir, path.basename(dir) + '.cfg');
  } else {
    objs = compileC(csources(dir), objdir, cc, ovls);
  }
  if (fs.existsSync(own)) partsChecked(csources(dir), objdir, fs.readFileSync(own, 'latin1'));
  if (fs.existsSync(own)) windowChecked(csources(dir));
  run(LD65, ['-C', fs.existsSync(own) ? own : at('sdk', 'c', 'hydra.cfg'), '-o', out, '-m', path.join(objdir, name + '.map'), '-Ln', path.join(objdir, name + '.lbl'),
    ...objs, at('obj', 'sdk', 'c', 'hydra.lib')]);
  exportsOf.set(name, writeExports(path.join(objdir, name + '.map'), out.replace(/\.hyx$/, '') + '.exp'));
  const data = fs.readFileSync(out);
  check.checkModule(name, data, 0x0800);
  return data;
}

// A C program's exports by name, for the bank programs it calls (bankprog.h: their imports found in them as they're
// loaded): NAME.exp beside NAME.hyx, from its link's map: "BX", 1, then each its name's length, its name and its
// address (two bytes), a 0 after the last.  OUT: the names (exportsOf keeps each program's, for buildChild's check)
const exportsOf = new Map();
function writeExports(map, out) {
  const t = fs.readFileSync(map, 'latin1'), a = t.indexOf('Exports list by name:'), b = t.indexOf('Exports list by value:');
  const parts = [Buffer.from([0x42, 0x58, 1])], names = new Set();
  for (const [, n, v] of t.slice(a, b).matchAll(/([A-Za-z_.][\w.]*)\s+([0-9A-F]{6})\s+R\w*/g)) {
    const x = parseInt(v, 16);
    if (names.has(n) || n.length > 31 || x > 0xFFFF) continue;
    names.add(n);
    parts.push(Buffer.from([n.length]), Buffer.from(n, 'latin1'), Buffer.from([x & 255, x >> 8]));
  }
  parts.push(Buffer.from([0]));
  fs.writeFileSync(out, Buffer.concat(parts));
  return names;
}

// A child's o65 file as the o65 spec has it: an import's HIGH relocation entry given the low byte ld65 leaves out
// (the spec has one after every HIGH entry, whatever its segment, and the loader reads one: without it the table's
// misread from there on, and the child refused, BE_FORMAT); every other entry kept as it is, so a loader may put the
// child anywhere.  OUT: the file's bytes
function childRelocs(bp) {
  const w = o => bp[o] | bp[o + 1] << 8;
  let p = 26;
  while (bp[p]) p += bp[p];
  p += 1 + w(10) + w(14);                               // (Past its options, its text and data)
  const nimp = w(p);
  p += 2;
  for (let i = 0; i < nimp; i++) { while (bp[p]) p++; p++; }   // (Past its imports' names)
  const head = bp.subarray(0, p), tables = [];
  for (let k = 0; k < 2; k++) {                         // (Its text's table, then its data's)
    const out = [];
    for (;;) {
      let c = bp[p++];
      out.push(c);
      if (!c) break;
      while (c === 255) { c = bp[p++]; out.push(c); }
      const t = bp[p++], seg = t & 15, extra = (seg === 0 ? 2 : 0) + ((t & 0xE0) === 0x40 && seg !== 0 ? 1 : 0);
      out.push(t, ...bp.subarray(p, p + extra));
      p += extra;
      if (seg === 0 && (t & 0xE0) === 0x40) out.push(0);     // (An import's HIGH: its low byte, 0, the guard's: an
    }                                                   //   offset's low byte is 0, buildChild's check)
    tables.push(Buffer.from(out));
  }
  return Buffer.concat([head, ...tables, bp.subarray(p)]);
}

// An o65 file's bytes without ld65's options (its name and version, and the time: an image the same each build; a
// bank program's loader, in its own task, reads its header and text at once)
function o65Bare(m) {
  let p = 26;
  while (m[p]) p += m[p];
  return Buffer.concat([m.subarray(0, 26), Buffer.from([0]), m.subarray(p + 1)]);
}

// A bank program (a child: sdk/c/include/bankprog.h, sdk/c/child.cfg), dir's .c and .s files, into objdir/NAME.bp:
// an o65 module at $8000, its imports what its objects use and don't have (od65's --dump-imports), its exports
// their globals (--dump-exports); checked against each caller its parents file names (its exports: exportsOf), the
// build failing if one hasn't something the child uses.  OUT: its file's bytes
const CHILD_ROOM = 7680, childRooms = new Map();   // (buildChild's: each bank program's room left, roomChecked says it)
function buildChild(dir, objdir) {
  const name = path.basename(dir), od = path.join(objdir, name);
  const objs = compileC(csources(dir), od);
  const names = (opt, re) => [...new Set(objs.flatMap(o => [...od65([opt, o]).matchAll(re)].map(m => m[1])))];
  const exps = names('--dump-exports', /Name:\s+"([^"]+)"/g), own = new Set(exps);
  const imps = names('--dump-imports', /Name:\s+"([^"]+)"/g).filter(n => !own.has(n));
  const hi = [];                                        // (The high byte of an import's address and an offset whose
  for (const f of csources(dir)) {                      //   low byte isn't 0: ld65's o65 has no room for that low
    const s = f.endsWith('.c') ? path.join(od, path.basename(f).replace(/\.c$/, '.s')) : f;   //   byte, so the loader
    for (const [x, n, sg, v] of fs.readFileSync(s, 'latin1').matchAll(/>\(\s*([A-Za-z_.@][\w.@]*)\s*([+-])\s*(\$?[0-9A-Fa-f]+)\s*\)/g))
      if (imps.includes(n) && (sg === '-' || parseInt(v.replace('$', ''), v.startsWith('$') ? 16 : 10) % 256)) hi.push(path.basename(s) + ': ' + x);
  }                                                     //   can't carry from it: refused)
  if (hi.length) throw new Error('bank program ' + name + ': the high byte of an import\'s address and an offset, which o65 can\'t carry: ' + [...new Set(hi)].join(', '));
  const list = [...exps.map(n => 'export = ' + n), ...imps.map(n => 'import = ' + n)];
  const cfg = fs.readFileSync(at('sdk', 'c', 'child.cfg'), 'latin1').replace(', IMPORTS_AND_EXPORTS;', list.length ? ', ' + list.join(', ') + ';' : ';');
  fs.writeFileSync(path.join(od, name + '.cfg'), cfg);
  const out = path.join(objdir, name + '.bp');
  run(LD65, ['-C', path.join(od, name + '.cfg'), '-o', out, ...objs]);   // (No map: ld65's fails on an o65's imports)
  const bp = o65Bare (fs.readFileSync(out));           // (ld65's options gone: its date of making and its own version,
                                                        //   "V2.19" or "V2.19 - Git ..." as it was built, the images
                                                        //   the same each build, on every machine)
  // Its room: the kernel's for a child (base/include/layout.inc's RB_TOP - RB_MODS, 7,680 bytes, not child.cfg's bank)
  // less its text, data and BSS (the o65 header's) and its exports' table (each its name, a 0, its address; a 0 after
  // the last: run.s's RE_TABLE).  Past it BANK_LOAD refuses it as the game runs (BL_BANK, 7): histgen's at 7,774 bytes,
  // 2026-11-21, the build none the wiser
  const room = CHILD_ROOM - bp.readUInt16LE(10) - bp.readUInt16LE(14) - bp.readUInt16LE(18) - exps.reduce((n, e) => n + e.length + 3, 1);
  if (room < 0) throw new Error('bank program ' + name + ': ' + -room + ' bytes past a child\'s room, ' + CHILD_ROOM + ' (its text, data, BSS and exports\' table)');
  childRooms.set(name, room);
  fs.writeFileSync(out, childRelocs(bp));
  const parents = path.join(dir, 'parents');
  if (fs.existsSync(parents)) {
    for (const p of fs.readFileSync(parents, 'latin1').split(/\s+/).filter(Boolean)) {
      const has = exportsOf.get(p);
      if (!has) throw new Error('bank program ' + name + ': its caller ' + p + ' isn\'t built (or isn\'t a C program)');
      const lack = imps.filter(n => !has.has(n));
      if (lack.length) throw new Error('bank program ' + name + ': its caller ' + p + ' hasn\'t ' + lack.join(', '));
    }
  }
  return fs.readFileSync(out);
}

// bin/sdk/asm: the assembly SDK, to take away: sdk/asm's files, the generated hydra.inc, the samples' sources
function sdk() {
  const out = at('bin', 'sdk', 'asm');
  fs.rmSync(out, { recursive: true, force: true });
  mkdir(path.join(out, 'samples'));
  for (const d of [atBase('sdk', 'asm'), at('sdk', 'asm')])     // (The base's, hydra.inc with it, then HydraOS's)
    for (const f of fs.readdirSync(d).filter(f => /\.(inc|s|cfg|md)$/.test(f))) fs.copyFileSync(path.join(d, f), path.join(out, f));
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
  for (const f of ['README.md', 'hydra.cfg', 'child.cfg']) fs.copyFileSync(at('sdk', 'c', f), path.join(c, f));
  for (const f of fs.readdirSync(at('sdk', 'c', 'include'))) fs.copyFileSync(at('sdk', 'c', 'include', f), path.join(c, 'include', f));
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
  hubNumbers();
  const name = path.basename(dir), od = at('obj', 'prog', name), bin = path.join(dir, name + '.hyx');
  if (isC(dir)) {
    if (!fs.existsSync(at('obj', 'sdk', 'c', 'hydra.lib'))) clib();
    cprog(dir, od, bin, (process.env.HYC_CFLAGS || '').split(/\s+/).filter(Boolean));
    return bin;
  }
  if (fs.existsSync(path.join(dir, 'bank'))) {                // (A bank program: dir/bank says so)
    fs.writeFileSync(bin, bankProgram(dir, path.dirname(od), []));
    return bin;
  }
  const objs = assemble(sources(dir), od, [at('sdk', 'asm'), ...base.INCLUDES, dir, at('obj', 'gen')], ['HYX2_RAM']);
  run(LD65, ['-C', atBase('sdk', 'asm', 'hyx2.cfg'), '-o', bin, '-m', path.join(od, name + '.map'), ...objs]);
  check.checkModule(name, fs.readFileSync(bin), 0x0800);
  return bin;
}

// The hardware test (bank 1 of the old system's paged ROM image) and a rom.txt's list: the base's
const { hwtest, readManifest, HWTEST_IMAGE } = base;

// A RAM program's folder built into objdir: a C program, a bank program (its folder's file bank says so), or one in
// assembly.  OUT: its image
function ramProgram(dir, objdir, defines) {
  return isC(dir) ? cprog(dir, path.join(objdir, path.basename(dir)), path.join(objdir, path.basename(dir) + '.hyx'))
    : fs.existsSync(path.join(dir, 'bank')) ? bankProgram(dir, objdir, defines) : buildModule(dir, objdir, defines, true);
}
// The test RAM programs (tests/NAME: obj/tests/NAME.hyx; t_rules reads the game's hub.h).  OUT: { NAME: its image }
function testPrograms(defines) {
  const progs = {};
  if (fs.existsSync(at('tests', 'ram')))
    for (const d of fs.readdirSync(at('tests', 'ram'), { withFileTypes: true }).filter(d => d.isDirectory()))
      progs[d.name] = ramProgram(at('tests', 'ram', d.name), at('obj', 'tests'), defines);
  return progs;
}
// The games (games/NAME: on the SD card), each a C program, and its make.js's data files, if it has one; then each
// make.js's after, if it has one: what wants another game built (dq0's themes, themec's).  bios: the BIOS ROM the
// make.js files run themec on.  OUT: { NAME: its image }
function gamesMade(say, bios, defines) {
  const games = {}, dirs = fs.existsSync(at('games')) ? fs.readdirSync(at('games'), { withFileTypes: true }).filter(d => d.isDirectory()).map(d => d.name) : [];
  for (const n of dirs) {
    games[n] = ramProgram(at('games', n), at('obj', 'games'), defines);
    const make = at('games', n, 'make.js');
    if (fs.existsSync(make)) require(make).build({ dir: at('games', n), objdir: at('obj', 'games', n), say, bios });
  }
  for (const n of dirs) {
    const make = at('games', n, 'make.js');
    if (fs.existsSync(make) && require(make).after) require(make).after({ dir: at('games', n), objdir: at('obj', 'games', n), say, bios });
  }
  return games;
}
// The bank programs (children: bankprog.h), their callers built: the tests', then the system's; with ready, only those
// whose callers (its parents file) are built already, the rest left for a call after (a clean tree's build: the ROM
// disk's bankrun's hello.bp before the games, whose themes themec compiles in the emulator, its ROM disk read whole:
// romfs/romfs.txt).  OUT: { NAME: its file }
function childrenMade(ready = false) {
  const children = {};
  for (const [src, od] of [[at('tests', 'bank'), at('obj', 'tests')], [at('bank'), at('obj', 'bank')]])
    if (fs.existsSync(src))
      for (const d of fs.readdirSync(src, { withFileTypes: true }).filter(d => d.isDirectory())) {
        const parents = path.join(src, d.name, 'parents');
        if (ready && fs.existsSync(parents) && fs.readFileSync(parents, 'latin1').split(/\s+/).some(p => p && !exportsOf.has(p))) continue;
        children[d.name] = buildChild(path.join(src, d.name), od);
      }
  return children;
}
// The games' room said; a task below its floor fails the build
function roomChecked(games, say) {
  const room = headroom(Object.keys(games));
  say(room.text);
  const kids = [...childRooms].sort((a, b) => a[0] < b[0] ? -1 : 1), low = kids.filter(([, r]) => r < HEADROOM);
  say('The bank programs\' room (of a child\'s ' + CHILD_ROOM + ' bytes; ! under ' + HEADROOM + '): ' + kids.map(([n, r]) => n + ' ' + r + (r < HEADROOM ? '!' : '')).join(', ') +
    (low.length ? '\n  headroom: ' + low.map(([n]) => n).join(', ') + ' under ' + HEADROOM + ' bytes free' : ''));
  if (room.faults.length) throw new Error('a game\'s task short of main RAM (below its floor): ' + room.faults.join('; '));
}

// A C program's exports as its NAME.exp has them (writeExports's file): its names
function readExports(file) {
  const b = fs.readFileSync(file), names = new Set();
  if (b.toString('latin1', 0, 2) !== 'BX' || b[2] !== 1) throw new Error(file + ': not an exports file');
  for (let p = 3; b[p]; p += 1 + b[p] + 2) names.add(b.toString('latin1', p + 1, p + 1 + b[p]));
  return names;
}

// The paged ROM's modules (modules/NAME) made again in place, as the whole build makes them (base.buildModule's
// assemble: their objects from the cache, or made anew), and held to what that build left: the first whose objects
// aren't the same byte for byte (an object made anew carries the time of its making), or null.  dq0lib's sources
// .include games/dq0's lnzp.inc and the hub's numbers (obj/gen/dq0hub_*.inc): a change in games/ can change it
function moduleChanged(defines) {
  for (const d of fs.readdirSync(at('modules'), { withFileTypes: true }).filter(d => d.isDirectory())) {
    const dir = at('modules', d.name), od = at('obj', 'modules', d.name), files = sources(dir);
    const objs = files.map(f => path.join(od, path.basename(f, '.s') + '.o')), was = objs.map(o => fs.existsSync(o) ? fs.readFileSync(o) : null);
    base.assemble(files, od, [...INCLUDES, dir, path.dirname(dir)], defines);
    if (objs.some((o, i) => !was[i] || !was[i].equals(fs.readFileSync(o)))) return d.name;
  }
  return null;
}

// Only what the games are made of, on the last whole build (for a change in games/, or bank/'s programs): the
// generated headers, the C library, the test RAM programs (t_rules reads hub.h), the games and their make.js files,
// the bank programs, the games' room, the SD card's image and the browser's page; the BIOS ROM and the paged ROM as
// that build left them, and its chip and clock (obj/build.json).  It's the whole build instead if there's none to
// build on (or it failed), or if a module of the paged ROM is no longer what it was made of (moduleChanged: dq0lib's
// sources are partly games/'s).  OUT: { games } (or buildBoth's)
function buildGames(opt = {}) {
  const say = opt.quiet ? () => {} : s => console.log(s);
  let was = null;
  try { was = JSON.parse(fs.readFileSync(at('obj', 'build.json'), 'utf8')); } catch (e) { /* (None) */ }
  const clock = was && was.clock > 1 ? { clock: was.clock } : {};   // (build.json's clock 1: none given, the default)
  const whole = why => {
    say('(The whole build: ' + why + ')');
    try { return buildBoth(Object.assign({}, opt, clock, was ? { acia: was.acia } : {})); } catch (e) { e.whole = true; throw e; }
  };
  const images = require('../base/tools/images.js'), bad = lastBuildFailed();
  if (!was || (bad && !bad.startsWith(GAMES_FAILED)) || !images.file(ROOT, 'bios', was.acia) || !images.file(ROOT, 'prom0', was.acia) ||
    !fs.existsSync(at('obj', 'tests', 't_rc.bin')))
    return whole('no whole build to build the games on' + (bad && !bad.startsWith(GAMES_FAILED) ? ': the last one failed' : ''));
  const c = base.useCache(at('obj', 'cache'), opt), defines = base.definesOf(clock);
  apigen.generate(ROOT);
  hubNumbers(say);
  const moved = moduleChanged(defines);
  if (moved) return whole('the paged ROM\'s ' + moved + ' isn\'t what the last whole build made it of');
  clib();
  testPrograms(defines);
  for (const dir of ['programs', 'games', 'tests'].map(d => at('obj', d)))   // (The callers the bank programs name:
    for (const f of fs.existsSync(dir) ? fs.readdirSync(dir).filter(f => f.endsWith('.exp')) : [])   //   the whole
      if (!exportsOf.has(f.slice(0, -4))) exportsOf.set(f.slice(0, -4), readExports(path.join(dir, f)));   //   build's)
  const games = gamesMade(say, images.bios(ROOT, was.acia), defines);
  childrenMade();
  roomChecked(games, say);
  const card = sdcard.build(ROOT);
  say('SD card: ' + card.files.length + ' files, ' + card.bytes + ' bytes: bin/sdcard.img, ' + card.size / 1048576 + ' MB');
  const page = require('./sim/web.js').build(at('bin', 'hydra-16.html'));
  say('The emulator in a browser: bin/hydra-16.html, ' + (page.size / 1048576).toFixed(1) + ' MB (sim/web.js); the BIOS ROM and the paged ROM the last whole build\'s');
  const said = base.cacheDone(c);
  if (said) say(said);
  return { games };
}

function build(opt = {}) {
  const say = opt.quiet ? () => {} : s => console.log(s);
  const defines = base.definesOf(opt);

  // The base: the kernel (../base/bin/bios.bin), its BIOS ROM ours too
  const baseBuilt = base.build(Object.assign({}, opt, { noReport: true }));
  apigen.generate(ROOT);
  if (fs.existsSync(at('games', 'dq0', 'hubgen.js'))) hubNumbers(say);   // (The hub's numbers, before the test RAM
  mkdir(at('bin'));                                                       //   programs (t_rules reads hub.h) and the games)
  fs.copyFileSync(atBase('bin', 'bios.bin'), at('bin', 'bios.bin'));

  // The C library (the C programs need it), the modules, the test modules and the test RAM programs
  clib();
  const ram = (dir, objdir) => ramProgram(dir, objdir, defines);
  const modules = {}, tests = {}, programs = {}, samples = {};
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
  const progs = testPrograms(defines);
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
  // What the ROM disk has of the build's (romfs/romfs.txt: HyForth's libraries, obj/forthlib; the bank programs whose
  // callers are built, obj/bank's hello.bp) before the games: themec boots a ROM with it to compile their themes (a
  // clean tree's build failed there, 2026-11-19)
  forthlib.build({ root: ROOT, modules, assemble: (files, od, inc) => assemble(files, od, inc, defines), ld65: args => run(LD65, args) });
  childrenMade(true);
  const games = gamesMade(say, fs.readFileSync(at('bin', 'bios.bin')), defines);
  childrenMade();
  roomChecked(games, say);
  sdk();
  const card = sdcard.build(ROOT);                          // The SD card's image (bin/sdcard.img): the samples, the songs, the benchmarks

  // hylang's snapshot (the module hysnap: its heap with its library loaded, taken in the emulator), then the paged ROM
  const manifest = readManifest(at('modules', 'rom.txt'));
  const hwt = hwtest(), bios = fs.readFileSync(at('bin', 'bios.bin'));
  if (manifest.modules.includes('hysnap')) {
    const s = hysnap.build({ root: ROOT, modules, names: manifest.modules, rc: tests.t_rc, hwtest: hwt, bios });
    say('hylang\'s snapshot (hysnap): ' + s.bytes + ' bytes, ' + s.pages + ' pages of cells and ' + s.blobBytes + ' bytes of blobs; hylang\'s id $' +
      s.id.toString(16).toUpperCase().padStart(4, '0'));
  }
  for (const n of manifest.modules) if (!modules[n]) throw new Error('modules/rom.txt: no module ' + n);
  // The small modules packed (after the snapshot, so its banks are known too): each program or driver of a bank
  // linked again at the page romimg.js's plan finds it room at (bank 0's, the top of another module's bank, or a
  // bank of its own), into obj/modules (a base module's too: kdev's)
  const dirOf = n => fs.existsSync(at('modules', n)) ? { dir: at('modules', n), objdir: at('obj', 'modules') }
    : { dir: atBase('modules', n), objdir: atBase('obj', 'modules') };
  const pages = romimg.plan(manifest.modules.map(n => ({ file: n, data: modules[n], pack: base.packable(dirOf(n).dir, modules[n]) })));
  for (const [n, page] of Object.entries(pages)) if (page) {
    const { dir, objdir } = dirOf(n);
    modules[n] = base.relinkModule(dir, objdir, at('obj', 'modules'), page, modules[n].length);
  }
  if (!hwt) say('(no ' + path.relative(ROOT, HWTEST_IMAGE) + ': the paged ROM has no hardware test)');
  const { image, entries, disk, banks, chips } = romimg.build({ modules: manifest.modules.map(n => ({ file: n, data: modules[n] })), init: manifest.init,
    hwtest: hwt, bios, romfs: romfs.manifest(at('romfs', 'romfs.txt')) });
  const { retry } = require('../base/tools/images.js');   // (An image held a moment, by a reader or a scanner: again)
  for (const f of fs.readdirSync(at('bin')).filter(f => /^prom\d*\.bin$/.test(f))) retry(() => fs.rmSync(at('bin', f)));   // (The last build's)
  for (let k = 0; k < chips; k++) retry(() => fs.writeFileSync(at('bin', 'prom' + k + '.bin'), image.subarray(k * romimg.CHIP, (k + 1) * romimg.CHIP)));
  fs.writeFileSync(at('obj', 'build.json'), JSON.stringify({ clock: opt.clock || 1, acia: opt.acia || 'rockwell' }) + '\n');

  const report = budget.report(base.ROOT, { modules, tests, progs, programs, samples, games, entries });
  say(report.text);
  if (disk) {
    const bytes = disk.files.reduce((n, f) => n + f.size, 0), used = disk.volume.length / 512, first = disk.start / 32;
    say('ROM disk: ' + disk.files.length + ' files, ' + bytes + ' bytes; its volume uses ' + used + ' of ' + disk.blocks + ' blocks (paged ROM banks ' +
      first + '-' + (first + disk.blocks / 32 - 1) + ', in socket order); every file read back as its source');
  }
  say('SD card: ' + card.files.length + ' files, ' + card.bytes + ' bytes (the samples, the songs, the benchmarks): bin/sdcard.img, ' + card.size / 1048576 + ' MB');
  say('Paged ROM: ' + banks + ' banks of 256, ' + chips + ' chip' + (chips === 1 ? '' : 's') + ' of 512K: ' +
    [...Array(chips)].map((_, k) => 'bin/prom' + k + '.bin').join(', '));
  return { modules, tests, progs, programs, samples, games, manifest, report };
}

// Both chips' builds, and their images (the base's buildBoth): HydraOS's bin (its card's image too) and the base's
const IMAGE_NAMES = /^(bios|prom\d+)(_(rockwell|wdc))?\.bin$|^sdcard(_(rockwell|wdc))?\.img$/;
function buildBoth(opt = {}) {
  const c = base.useCache(at('obj', 'cache'), opt);
  const r = base.buildBoth(opt, build, [[at('bin'), IMAGE_NAMES], [atBase('bin'), base.IMAGE_NAMES]]);
  const page = require('./sim/web.js').build(at('bin', 'hydra-16.html'));   // (The emulator in one page, from them)
  if (!opt.quiet) console.log('The emulator in a browser: bin/hydra-16.html, ' + (page.size / 1048576).toFixed(1) + ' MB (sim/web.js)');
  const said = base.cacheDone(c);
  if (said && !opt.quiet) console.log(said);
  return r;
}

// The last build's failure kept (obj/build.failed, its error), so no test, tier or soak runs on the images it left
// behind, the last good build's (sim/test.js's and soak.js's built check, 2026-11-06: the lead's, 47 tests run on a
// stale dq0); a good build takes it away.  A C, assembler or linker error fails the build (run's: their tools' status)
function failed(why) {
  const f = at('obj', 'build.failed');
  if (why === null) { if (fs.existsSync(f)) fs.unlinkSync(f); return; }
  mkdir(at('obj'));
  fs.writeFileSync(f, why + '\n');
}
const GAMES_FAILED = '(the games\' build) ';             // (Its failure's mark: a games' build may go on from it)
function lastBuildFailed() {
  const f = at('obj', 'build.failed');
  return fs.existsSync(f) ? fs.readFileSync(f, 'utf8').split('\n')[0] : null;
}

if (require.main === module) {
  const a = process.argv.slice(2), opt = {};
  if (a[0] === 'prog') {
    if (a.length !== 2) { console.error('usage: node build.js prog DIR'); process.exit(2); }
    try { base.useCache(at('obj', 'cache')); console.log(path.relative(process.cwd(), prog(a[1]))); base.cacheDone(objcache.current()); }
    catch (e) { console.error('build: ' + e.message); process.exit(1); }
    process.exit(0);
  }
  const games = a[0] === 'games';
  const usage = 'usage: node build.js [--clock 1|2] [--acia rockwell|wdc] [--both] [--no-cache | --clean] [--quiet]\n' +
    '       node build.js games [--no-cache | --clean] [--quiet]\n       node build.js prog DIR';
  for (let i = games ? 1 : 0; i < a.length; i++) {
    if (a[i] === '--clock' && !games) opt.clock = +a[++i];
    else if (a[i] === '--acia' && !games) opt.acia = a[++i];
    else if (a[i] === '--both' && !games) opt.both = true;
    else if (a[i] === '--quiet') opt.quiet = true;
    else if (a[i] === '--no-cache') opt.cache = false;
    else if (a[i] === '--clean') opt.clean = true;
    else { console.error(usage); process.exit(2); }
  }
  try { if (games) buildGames(opt); else buildBoth(opt); }
  catch (e) { failed((games && !e.whole ? GAMES_FAILED : '') + e.message); console.error('build: ' + e.message); process.exit(1); }
  failed(null);
}

module.exports = { build, buildBoth, buildGames, buildModule, readManifest, hwtest, prog, clib, cprog, headroom, lastBuildFailed, windowChecked };
