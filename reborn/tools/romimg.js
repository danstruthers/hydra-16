#!/usr/bin/env node
// ****************************************************************************
// romimg.js - the paged ROM image: the module directory in bank 0, and each module in banks of its own, run in
// place (docs/reimplementation-from-scratch.md, §11).
//
//   bank 0, $A000-$A1FF   block 0: a signature (the partition table, when the ROM disk comes: phase 4)
//   bank 0, $A200-$A3FF   the module directory (include/layout.inc: MD_*, ME_*): "HYMD", its version, the count,
//                         init's entry, then 16 bytes a module: its bank, banks, type, flags and name
//   banks 1 ...           the modules, each at $A000 of its first bank (its HYX2 header first)
//
// The image is the chips' view, not the CPU's: the board swaps A13 (each bank's $C000 half comes first), and on
// the V1 board a bank number's bits 2 and 3, and 6 and 7, trade places before they reach the chips, so bank b
// sits at bank swap(b)'s place (sim/lib/machine.js: romBank).
//
// Usage: node tools/romimg.js OUT.bin --init NAME MODULE.bin ...      (--list: what's where)
// From Node: build({ modules: [Buffer, ...], init: 'name' }) gives { image, entries }.
'use strict';
const fs = require('fs');
const path = require('path');

const BANK = 0x4000, WINDOW = 0xA000;
const MD_BASE = 0xA200, MD_VERSION = 1, MD_MAX = 31, ME_SIZE = 16, NAME_LEN = 12;
const HX = { MAGIC: 0, HSIZE: 4, TYPE: 5, FLAGS: 6, ABI: 7, LOAD: 8, LENGTH: 10, BANKS: 33, NAME: 36, SIZE: 48 };
const TYPES = { 1: 'program', 2: 'driver', 3: 'library' };
const SIGNATURE = 'Hydra-16 reborn paged ROM: block 0 this, then the module directory; the modules from bank 1\r\n';
const romBank = b => (b & 0x33) | ((b & 0x04) << 1) | ((b & 0x08) >> 1) | ((b & 0x40) << 1) | ((b & 0x80) >> 1);

// A module's header, checked.  OUT: { name, type, flags, banks, data }
function readHeader(data, what) {
  const fail = msg => { throw new Error(what + ': ' + msg); };
  if (data.length < HX.SIZE || data.toString('latin1', 0, 4) !== 'HYX2') fail('not a HYX2 module');
  if (data[HX.HSIZE] !== HX.SIZE) fail('a header of ' + data[HX.HSIZE] + ' bytes (48 expected)');
  if (!TYPES[data[HX.TYPE]]) fail('type ' + data[HX.TYPE] + '?');
  if (data.readUInt16LE(HX.LOAD) !== WINDOW) fail('not built to run in place (load address $' + data.readUInt16LE(HX.LOAD).toString(16) + ')');
  const length = data.readUInt16LE(HX.LENGTH);
  if (length !== data.length) fail('its header says ' + length + ' bytes, the file has ' + data.length);
  const banks = Math.ceil(data.length / BANK);
  if (banks !== 1 || data[HX.BANKS] !== 1) fail('more than one bank (16K): not yet');
  const raw = data.subarray(HX.NAME, HX.NAME + NAME_LEN), end = raw.indexOf(0);
  if (end < 1) fail('no name');
  const name = raw.toString('latin1', 0, end);
  return { name, type: data[HX.TYPE], flags: data[HX.FLAGS], banks, data };
}

// Write bytes at a CPU address in a bank, the chips' way
function put(image, bank, addr, bytes) {
  for (let i = 0; i < bytes.length; i++) {
    const a = addr + i - WINDOW, b = bank + Math.floor(a / BANK);
    image[romBank(b) * BANK + ((a % BANK) ^ 0x2000)] = bytes[i];
  }
}

function build({ modules, init }) {
  const entries = modules.map((m, i) => readHeader(m.data || m, m.file || 'module ' + i));
  if (entries.length > MD_MAX) throw new Error(entries.length + ' modules: ' + MD_MAX + ' at most');
  const names = new Set();
  for (const e of entries) { if (names.has(e.name)) throw new Error('two modules named ' + e.name); names.add(e.name); }
  let initIndex = 0xFF;
  if (init) { initIndex = entries.findIndex(e => e.name === init); if (initIndex < 0) throw new Error('no module ' + init + ' for init'); }
  if (initIndex !== 0xFF && entries[initIndex].type !== 1) throw new Error(init + ' is not a program');
  let bank = 1;
  for (const e of entries) { e.bank = bank; bank += e.banks; }
  if (bank > 256) throw new Error('the modules need ' + bank + ' banks: 256 at most');
  let top = 0;
  for (let b = 0; b < bank; b++) top = Math.max(top, romBank(b));
  const image = Buffer.alloc((top + 1) * BANK, 0xFF);
  const block0 = Buffer.alloc(512, 0);
  block0.write(SIGNATURE, 'latin1');
  put(image, 0, WINDOW, block0);
  const md = Buffer.alloc(512, 0);
  md.write('HYMD', 0, 'latin1');
  md[4] = MD_VERSION; md[5] = entries.length; md[6] = initIndex;
  entries.forEach((e, i) => {
    const o = 8 + i * ME_SIZE;
    md[o] = e.bank; md[o + 1] = e.banks; md[o + 2] = e.type; md[o + 3] = e.flags;
    md.write(e.name, o + 4, 'latin1');
  });
  put(image, 0, MD_BASE, md);
  for (const e of entries) put(image, e.bank, WINDOW, e.data);
  return { image, entries };
}

// The image as the CPU reads it: bank b, address a (for checks)
function read(image, bank, addr) {
  const off = romBank(bank) * BANK + ((addr - WINDOW) ^ 0x2000);
  return off < image.length ? image[off] : 0xFF;
}

function main(argv) {
  const out = argv[0], files = [];
  let init = null, list = false;
  for (let i = 1; i < argv.length; i++) {
    if (argv[i] === '--init') init = argv[++i];
    else if (argv[i] === '--list') list = true;
    else files.push(argv[i]);
  }
  if (!out || !files.length) { console.error('usage: romimg.js OUT.bin [--init NAME] [--list] MODULE.bin ...'); process.exit(2); }
  const { image, entries } = build({ modules: files.map(f => ({ file: f, data: fs.readFileSync(f) })), init });
  fs.writeFileSync(out, image);
  if (list) for (const e of entries)
    console.log('bank ' + e.bank.toString(16).padStart(2, '0') + '  ' + TYPES[e.type].padEnd(8) + e.name.padEnd(12) + e.data.length + ' bytes' + (e.name === init ? '  (init)' : ''));
}

if (require.main === module) {
  try { main(process.argv.slice(2)); } catch (e) { console.error('romimg: ' + e.message); process.exit(1); }
}
module.exports = { build, read, readHeader, romBank };
