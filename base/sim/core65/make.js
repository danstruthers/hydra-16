#!/usr/bin/env node
// ****************************************************************************
// make.js - the WebAssembly core (src/lib.rs: the CPU and the memory map, machine.js runs it) built into
// ../lib/core65.js, its bytes as base64 and the source's hash, which is in Git: the emulator (and the web page, one
// file) needs no Rust.  It's built again only when the source has changed since (Cargo.toml and src/lib.rs, their
// hash against the one core65.js holds) and Rust is here (cargo, with the wasm32-unknown-unknown target: rustup
// target add wasm32-unknown-unknown); with no Rust a changed source is said, and the committed core is used.
// base/build.js runs it first (make()); node make.js [--force] builds it now (--force: even if the source hasn't
// changed).  The build's own files: ../../obj/core65 (cargo's target folder).
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { execFileSync } = require('child_process');

const DIR = __dirname, OUT = path.join(DIR, '..', 'lib', 'core65.js'), TARGET = path.join(DIR, '..', '..', 'obj', 'core65');
const SOURCES = ['Cargo.toml', path.join('src', 'lib.rs')];

// The source's hash (its line ends as LF: a checkout's CRLF changes nothing)
const sourceHash = () => {
  const h = crypto.createHash('sha1');
  for (const f of SOURCES) h.update(f.replace(/\\/g, '/') + '\n' + fs.readFileSync(path.join(DIR, f), 'utf8').replace(/\r\n/g, '\n'));
  return h.digest('hex');
};
const recorded = () => { try { return (fs.readFileSync(OUT, 'utf8').match(/const SOURCE = '([0-9a-f]+)'/) || [])[1] || null; } catch (e) { return null; } };
// Rust, if it's here: cargo, and rustc's wasm32-unknown-unknown library
function rust() {
  try {
    execFileSync('cargo', ['--version'], { stdio: 'pipe' });
    const sysroot = execFileSync('rustc', ['--print', 'sysroot'], { encoding: 'utf8', stdio: 'pipe' }).trim();
    return fs.existsSync(path.join(sysroot, 'lib', 'rustlib', 'wasm32-unknown-unknown'));
  } catch (e) { return false; }
}

// OUT: what it did, a line (or null: the core was up to date)
function make(opt = {}) {
  const hash = sourceHash();
  if (!opt.force && hash === recorded()) return null;
  if (!rust()) return 'core65: its source has changed, but Rust isn\'t here (cargo, with the wasm32-unknown-unknown target): the committed base/sim/lib/core65.js is used';
  execFileSync('cargo', ['build', '--release', '--target', 'wasm32-unknown-unknown', '--target-dir', TARGET], { cwd: DIR, stdio: 'pipe' });
  const wasm = fs.readFileSync(path.join(TARGET, 'wasm32-unknown-unknown', 'release', 'core65.wasm'));
  new WebAssembly.Module(wasm);                               // (It compiles)
  fs.writeFileSync(OUT, [
    '// core65.js - the WebAssembly core\'s bytes (base/sim/core65: its make.js wrote this, from the source whose hash',
    '// is SOURCE; core.js loads it).  In Git: the emulator needs no Rust.',
    '\'use strict\';',
    'const SOURCE = \'' + hash + '\';',
    'const WASM = \'' + wasm.toString('base64') + '\';',
    'module.exports = { SOURCE, WASM };', ''].join('\r\n'));
  return 'core65: built (' + wasm.length + ' bytes): base/sim/lib/core65.js';
}

if (require.main === module) {
  const r = make({ force: process.argv.includes('--force') });
  console.log(r || 'core65: up to date');
}
module.exports = { make, sourceHash };
