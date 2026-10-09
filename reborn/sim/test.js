#!/usr/bin/env node
// ****************************************************************************
// test.js - HydraOS's regression tests (tests/tests.js), and the base's before them: the base's runner
// (../../base/sim/test.js: its options, -v, -j N, --seed, --build, --dl, are there) with HydraOS's images (its
// modules/rom.txt's modules, less a test's without, and the test's own; the ROM disk), its PC folders (/pc: a test's
// pc), the danlang emulator (sim/dl) and its build.
//
// Usage: node sim/test.js [NAME ...] [--build] [-v] [--seed N] [-j N] [--dl]
'use strict';
const fs = require('fs');
const path = require('path');
const runner = require('../../base/sim/test.js');
const { createPcHost } = require('./lib/pchost.js');
const romimg = require('../tools/romimg.js');
const romfs = require('../tools/romfs.js');
const { readManifest, hwtest } = require('../build.js');
const { tests, IRQ_OFF_MAX } = require('../tests/tests.js');

const ROOT = path.join(__dirname, '..');
const bin = (dir, n) => fs.readFileSync(path.join(ROOT, 'obj', dir, n + '.bin'));

function image(t) {
  const sys = readManifest(path.join(ROOT, 'modules', 'rom.txt')).modules.filter(n => !(t.without || []).includes(n));
  const own = [...(t.modules || [])];
  if (!sys.includes(t.init) && !own.includes(t.init)) own.unshift(t.init);
  const own_ = n => {                                     // (A test's, the SDK's, HydraOS's; or the base's: a test
    const d = ['tests', 'samples', 'modules'].find(d => fs.existsSync(path.join(ROOT, 'obj', d, n + '.bin')));   //   module, a module
    if (d) return bin(d, n);
    const b = ['tests', 'modules'].find(d => fs.existsSync(path.join(ROOT, '..', 'base', 'obj', d, n + '.bin')));
    return fs.readFileSync(path.join(ROOT, '..', 'base', 'obj', b || 'modules', n + '.bin'));
  };
  const mods = [...sys.map(n => ({ file: n, data: bin('modules', n) })), ...own.map(n => ({ file: n, data: own_(n) }))];
  return romimg.build({ modules: mods, init: t.init, hwtest: hwtest(), bios: fs.readFileSync(path.join(ROOT, 'bin', 'bios.bin')),
    romfs: romfs.manifest(path.join(ROOT, 'romfs', 'romfs.txt')) }).image;
}

// A test's PC folder (t.pc: { files: { name: text or bytes (or a function giving them), 'dir/': '' } (or a function
// giving that), readOnly, damage }), made afresh in
// obj/pc/NAME, its files stamped 2026-10-03 15:04:05; and the PC tool for it (sim/lib/pchost.js).  OUT: { dir, host }
function pcFolder(t, opt) {
  const dir = path.join(ROOT, 'obj', 'pc', t.name), when = new Date(2026, 9, 3, 15, 4, 5);
  fs.rmSync(dir, { recursive: true, force: true });
  fs.mkdirSync(dir, { recursive: true });
  const files = typeof t.pc.files === 'function' ? t.pc.files() : t.pc.files || {};
  for (const [n, data] of Object.entries(files)) {
    const f = path.join(dir, n);
    if (n.endsWith('/')) fs.mkdirSync(f, { recursive: true });
    else { fs.mkdirSync(path.dirname(f), { recursive: true }); fs.writeFileSync(f, typeof data === 'function' ? data() : data); }
  }
  const stamp = d => { for (const n of fs.readdirSync(d)) { const f = path.join(d, n); if (fs.statSync(f).isDirectory()) stamp(f); fs.utimesSync(f, when, when); } };
  stamp(dir);
  const host = createPcHost({ dir, readOnly: !!t.pc.readOnly, damage: t.pc.damage || [],
    log: opt.verbose ? s => console.log('  [pc] ' + s) : undefined });
  return { dir, host };
}

runner.setup({ root: ROOT, tests, IRQ_OFF_MAX, image, pcFolder, self: __filename,
  build: () => require('../build.js').build({ quiet: true }),
  runDl: (...a) => require('./dl/bridge.js').runDl(...a) });

if (require.main === module) runner.main(process.argv.slice(2));
module.exports = { runTest: runner.runTest, runTestDl: runner.runTestDl, image };
