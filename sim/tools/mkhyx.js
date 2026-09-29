#!/usr/bin/env node
// ****************************************************************************
// mkhyx.js - Hydra executables (.hyx) on the PC: puts the 16-byte header on a raw binary (the layout:
// os_rom/include/shell.inc), or shows a .hyx file's header.  (A program built with ca65 and
// programs/hyx.cfg has its header already.)
//
// Usage: node mkhyx.js BINARY OUT.hyx LOAD [ENTRY]
//          LOAD, ENTRY: addresses ($0800, 0x0800 or 2048); ENTRY defaults to LOAD.  The program must fit in
//          task RAM, $0800-$7BFF.
//        node mkhyx.js --info FILE.hyx
//
// As a module: require('./mkhyx.js') gives { hyx, info } (e.g. for regress.js's test programs).
// ****************************************************************************
'use strict';
const fs = require('fs');

const MAGIC = 'HYX1', HEADER = 16, RAM_LOW = 0x0800, RAM_END = 0x7C00;

// A .hyx file: the header, then the code (a Buffer or byte array) to load at load, entered at entry
function hyx(load, code, entry = load) {
  code = Buffer.from(code);
  if (load < RAM_LOW || load + code.length > RAM_END) throw new Error('the program must fit in $0800-$7BFF');
  if (entry < load || entry >= load + code.length) throw new Error('the entry point is outside the program');
  const b = Buffer.alloc(HEADER + code.length);
  b.write(MAGIC, 0, 'latin1');
  b.writeUInt16LE(load, 4);
  b.writeUInt16LE(code.length, 6);
  b.writeUInt16LE(entry, 8);
  code.copy(b, HEADER);
  return b;
}

// A .hyx file's header: { load, length, entry, flags }, or an error
function info(b) {
  if (b.length < HEADER || b.toString('latin1', 0, 4) !== MAGIC) throw new Error('not a Hydra executable (no HYX1 header)');
  const h = { load: b.readUInt16LE(4), length: b.readUInt16LE(6), entry: b.readUInt16LE(8), flags: b.readUInt16LE(10) };
  if (b.length - HEADER < h.length) throw new Error('the file is shorter than its header says');
  return h;
}

const addr = s => {
  const n = /^\$/.test(s) ? parseInt(s.slice(1), 16) : Number(s);
  if (!Number.isInteger(n) || n < 0 || n > 0xFFFF) throw new Error('not an address: ' + s);
  return n;
};
const hex = n => '$' + n.toString(16).toUpperCase().padStart(4, '0');

if (require.main === module) {
  const args = process.argv.slice(2);
  try {
    if (args[0] === '--info' && args.length === 2) {
      const h = info(fs.readFileSync(args[1]));
      console.log(`load ${hex(h.load)}, length ${h.length} (to ${hex(h.load + h.length - 1)}), entry ${hex(h.entry)}, flags ${h.flags}`);
    } else if (args.length === 3 || args.length === 4) {
      const load = addr(args[2]);
      fs.writeFileSync(args[1], hyx(load, fs.readFileSync(args[0]), args[3] ? addr(args[3]) : load));
    } else {
      console.error('Usage: node mkhyx.js BINARY OUT.hyx LOAD [ENTRY]\n       node mkhyx.js --info FILE.hyx');
      process.exit(2);
    }
  } catch (e) {
    console.error('mkhyx: ' + e.message);
    process.exit(1);
  }
}

module.exports = { hyx, info };
