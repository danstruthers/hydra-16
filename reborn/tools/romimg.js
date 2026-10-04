#!/usr/bin/env node
// ****************************************************************************
// romimg.js - the paged ROM image: the module directory in bank 0, each module in banks of its own, run in place
// (docs/reimplementation-from-scratch.md, §11), and the ROM disk's volume after them (tools/romfs.js).
//
//   bank 0, $A000-$A1FF   block 0: a signature line, and the ROM disk's partition table (romfs.js)
//   bank 0, $A200-$A9FF   the module directory (include/layout.inc: MD_*, ME_*): "HYMD", its version, the count,
//                         init's entry, whether bank 1 has the hardware test, then 16 bytes a module: its bank,
//                         banks, type, flags and name
//   bank 1                the hardware test (os_rom/hwtest, unchanged: its $A000-$DEFF copied from the old
//                         system's paged ROM image), with the ROMs' checksums at $DF00 (as os_rom/tools/romsum.js
//                         makes them: a CRC-16 of each BIOS ROM page and paged ROM bank, here of reborn's images)
//   banks 2 ...           the modules, each at $A000 of its first bank (its HYX2 header first)
//   the banks after them  the ROM disk's HydraFS volume (/rom), as big as its files need (whole banks)
// Banks in socket order: the Nth bank used is the CPU's bank N with bits 6 and 7 swapped (socketBank), so what's
// there fills the sockets (512K chips, 32 banks each) in turn, and the storage driver reads the ROM disk the same way
// (rom_read).  A module of two banks doesn't straddle the 64th: its banks are N and N + 1 to the CPU too.
//
// The image is the chips' view, not the CPU's: the board swaps A13 (each bank's $C000 half comes first), and on
// the V1 board a bank number's bits 2 and 3, and 6 and 7, trade places before they reach the chips, so bank b
// sits at bank swap(b)'s place (sim/lib/machine.js: romBank).  It's whole chips, as many as what's there needs: chip k
// is its 512K from k * 512K (build.js writes each to bin/promK.bin).
//
// Usage: node tools/romimg.js OUT.bin --init NAME [--hwtest OLD_PAGED_ROM.bin --bios BIOS.bin] [--romfs MANIFEST]
//        MODULE.bin ...  (--list: what's where)
// From Node: build({ modules: [Buffer, ...], init: 'name', hwtest, bios, romfs: romfs.js's files }) gives { image,
// entries, disk, banks, chips } (disk: the ROM disk, its files read back from the image: romfs.js's readBack, or
// null; banks: the banks used, in socket order; chips: the image's 512K chips).
'use strict';
const fs = require('fs');
const path = require('path');
const romfs = require('./romfs.js');

const BANK = 0x4000, WINDOW = 0xA000, PAGE = 0x2000;
const MD_BASE = 0xA200, MD_VERSION = 1, MD_MAX = 127, ME_SIZE = 16, NAME_LEN = 12;
const HX = { MAGIC: 0, HSIZE: 4, TYPE: 5, FLAGS: 6, ABI: 7, LOAD: 8, LENGTH: 10, BANKS: 33, NAME: 36, SIZE: 48 };
const TYPES = { 1: 'program', 2: 'driver', 3: 'library' };
const SIGNATURE = 'Hydra-16 reborn paged ROM: block 0 this and the ROM disk\'s partition table, then the module directory; ' +
  'bank 1 the hardware test; the modules from bank 2, then the ROM disk\'s volume\r\n';
const BLOCKS_PER_BANK = BANK / 512, ROM_BANKS = 256, CHIP = 32 * BANK, CPU_SUMMED = 64;
const HWT_BANK = 1, HWT_SUMS = 0xDF00, HWT_SUMS_SIZE = 256, FIRST_MODULE_BANK = 2;
const romBank = b => (b & 0x33) | ((b & 0x04) << 1) | ((b & 0x08) >> 1) | ((b & 0x40) << 1) | ((b & 0x80) >> 1);
const socketBank = n => (n & 0x3F) | ((n & 0x40) << 1) | ((n & 0x80) >> 1);   // (The Nth bank in socket order)

// CRC-16/CCITT-FALSE (polynomial $1021, from $FFFF), of n bytes from get(i): the hardware test's (hwt_rom.s)
function crc16(get, n) {
  let c = 0xFFFF;
  for (let i = 0; i < n; i++) {
    c ^= get(i) << 8;
    for (let k = 0; k < 8; k++) c = c & 0x8000 ? ((c << 1) ^ 0x1021) & 0xFFFF : (c << 1) & 0xFFFF;
  }
  return c;
}

// A module's header, checked.  OUT: { name, type, flags, banks, data }
function readHeader(data, what) {
  const fail = msg => { throw new Error(what + ': ' + msg); };
  if (data.length < HX.SIZE || data.toString('latin1', 0, 4) !== 'HYX2') fail('not a HYX2 module');
  if (data[HX.HSIZE] !== HX.SIZE) fail('a header of ' + data[HX.HSIZE] + ' bytes (48 expected)');
  if (!TYPES[data[HX.TYPE]]) fail('type ' + data[HX.TYPE] + '?');
  if (data.readUInt16LE(HX.LOAD) !== WINDOW) fail('not built to run in place (load address $' + data.readUInt16LE(HX.LOAD).toString(16) + ')');
  const banks = data[HX.BANKS], length = data.readUInt16LE(HX.LENGTH);   // (Its length in its last bank)
  if (banks < 1 || banks > 2) fail(banks + ' banks: one or two');
  if (length < 1 || length > BANK || (banks - 1) * BANK + length !== data.length)
    fail('its header says ' + banks + ' banks, the last ' + length + ' bytes long; the file has ' + data.length);
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

// hwtest: the old system's paged ROM image (its bank 1 is the hardware test), or none; bios: the BIOS ROM image,
// for the checksums (with hwtest)
function build({ modules, init, hwtest, bios, romfs: files }) {
  const entries = modules.map((m, i) => readHeader(m.data || m, m.file || 'module ' + i));
  if (entries.length > MD_MAX) throw new Error(entries.length + ' modules: ' + MD_MAX + ' at most');
  const names = new Set();
  for (const e of entries) { if (names.has(e.name)) throw new Error('two modules named ' + e.name); names.add(e.name); }
  let initIndex = 0xFF;
  if (init) { initIndex = entries.findIndex(e => e.name === init); if (initIndex < 0) throw new Error('no module ' + init + ' for init'); }
  if (initIndex !== 0xFF && entries[initIndex].type !== 1) throw new Error(init + ' is not a program');
  let bank = FIRST_MODULE_BANK;                               // (In socket order: the CPU's socketBank(bank))
  for (const e of entries) {
    if (e.banks === 2 && bank % 64 === 63) bank++;            // (Two banks: N and N + 1 to the CPU as well)
    e.bank = socketBank(bank);
    bank += e.banks;
  }
  let disk = null;                                            // The ROM disk: its volume in the banks after them,
  if (files && bank < ROM_BANKS) {                            //   whole banks, as many as its files need
    const start = bank * BLOCKS_PER_BANK, most = ROM_BANKS * BLOCKS_PER_BANK - start;   // (From the blocks written: a
    let blocks = Math.ceil(romfs.volume(files, most).length / BANK) * BLOCKS_PER_BANK, volume = null;  //   file's last
    while (!volume) {                                         //   zeros aren't, but they're its)
      try { volume = romfs.volume(files, blocks); }
      catch (e) { if (!/full/.test(e.message) || blocks >= most) throw e; blocks = Math.min(blocks + BLOCKS_PER_BANK, most); }
    }
    disk = { start, blocks, volume };
    bank += blocks / BLOCKS_PER_BANK;
  }
  if (bank > ROM_BANKS) throw new Error('the modules and the ROM disk need ' + bank + ' banks: ' + ROM_BANKS + ' at most');
  const chips = Math.ceil(bank * BANK / CHIP);
  const image = Buffer.alloc(chips * CHIP, 0xFF);
  const block0 = disk ? romfs.table(disk.start, disk.blocks) : Buffer.alloc(512, 0);
  if (SIGNATURE.length > 0x1BE) throw new Error('the signature reaches the partition table');
  block0.write(SIGNATURE, 'latin1');
  put(image, 0, WINDOW, block0);
  const md = Buffer.alloc(8 + MD_MAX * ME_SIZE, 0);
  md.write('HYMD', 0, 'latin1');
  md[4] = MD_VERSION; md[5] = entries.length; md[6] = initIndex; md[7] = hwtest ? 1 : 0;
  entries.forEach((e, i) => {
    const o = 8 + i * ME_SIZE;
    md[o] = e.bank; md[o + 1] = e.banks; md[o + 2] = e.type; md[o + 3] = e.flags;
    md.write(e.name, o + 4, 'latin1');
  });
  put(image, 0, MD_BASE, md);
  for (const e of entries) put(image, e.bank, WINDOW, e.data);
  if (disk)                                                   // (A bank at a time: socket order)
    for (let i = 0; i * BANK < disk.volume.length; i++)
      put(image, socketBank(disk.start / BLOCKS_PER_BANK + i), WINDOW, disk.volume.subarray(i * BANK, (i + 1) * BANK));
  if (hwtest) {
    const code = Buffer.alloc(HWT_SUMS - WINDOW);
    for (let a = WINDOW; a < HWT_SUMS; a++) code[a - WINDOW] = read(hwtest, HWT_BANK, a);
    if (code[0] !== 0x78) throw new Error('the old paged ROM\'s bank 1 isn\'t the hardware test (no sei at $A000)');
    put(image, HWT_BANK, WINDOW, code);
    if (!bios) throw new Error('the hardware test\'s checksums need the BIOS ROM image');
    put(image, HWT_BANK, HWT_SUMS, sums(bios, image, Math.min(bank, CPU_SUMMED)));
  }
  if (disk) {                                                 // Every file read back, as the CPU sees the disk
    const block = n => { const b = Buffer.alloc(512); for (let i = 0; i < 512; i++) b[i] = read(image, socketBank(Math.floor(n / BLOCKS_PER_BANK)), WINDOW + (n % BLOCKS_PER_BANK) * 512 + i); return b; };
    disk.files = romfs.readBack(block, disk.start + disk.volume.length / 512, files);
  }
  return { image, entries, disk, banks: bank, chips };
}

// The hardware test's table of the ROMs' checksums (os_rom/tools/romsum.js): the BIOS pages (1 byte), the paged
// banks (1), then a CRC of each page ($E000-$FEFF) and of each bank as the CPU sees it ($A000-$DEFF), low first.
// The banks are 0 to banks - 1, as the CPU selects them: the first two sockets' at most (CPU_SUMMED), as the
// socket order goes on at bank 128 (the old test counts banks in a row)
function sums(bios, image, banks) {
  const pages = bios.length / PAGE;
  if (2 + 2 * (pages + banks) > HWT_SUMS_SIZE) throw new Error('too many pages and banks for the hardware test\'s table');
  const table = Buffer.alloc(HWT_SUMS_SIZE);
  table[0] = pages;
  table[1] = banks;
  let at = 2;
  for (let p = 0; p < pages; p++, at += 2) table.writeUInt16LE(crc16(i => bios[p * PAGE + i], 0x1F00), at);
  for (let b = 0; b < banks; b++, at += 2) table.writeUInt16LE(crc16(i => read(image, b, WINDOW + i), HWT_SUMS - WINDOW), at);
  return table;
}

// The image as the CPU reads it: bank b, address a (for checks)
function read(image, bank, addr) {
  const off = romBank(bank) * BANK + ((addr - WINDOW) ^ 0x2000);
  return off < image.length ? image[off] : 0xFF;
}

function main(argv) {
  const out = argv[0], mods = [];
  let init = null, list = false, hwtest = null, bios = null, files = null;
  for (let i = 1; i < argv.length; i++) {
    if (argv[i] === '--init') init = argv[++i];
    else if (argv[i] === '--list') list = true;
    else if (argv[i] === '--hwtest') hwtest = fs.readFileSync(argv[++i]);
    else if (argv[i] === '--bios') bios = fs.readFileSync(argv[++i]);
    else if (argv[i] === '--romfs') files = romfs.manifest(argv[++i]);
    else mods.push(argv[i]);
  }
  if (!out || !mods.length) { console.error('usage: romimg.js OUT.bin [--init NAME] [--hwtest OLD.bin --bios BIOS.bin] [--romfs MANIFEST] [--list] MODULE.bin ...'); process.exit(2); }
  const { image, entries, disk } = build({ modules: mods.map(f => ({ file: f, data: fs.readFileSync(f) })), init, hwtest, bios, romfs: files });
  fs.writeFileSync(out, image);
  if (list) for (const e of entries)
    console.log('bank ' + e.bank.toString(16).padStart(2, '0') + '  ' + TYPES[e.type].padEnd(8) + e.name.padEnd(12) + e.data.length + ' bytes' + (e.name === init ? '  (init)' : ''));
  if (list && disk) for (const f of disk.files)
    console.log('/rom' + f.path.padEnd(24) + String(f.size).padStart(7) + ' bytes, banks ' + f.banks.map(b => b.toString(16).padStart(2, '0')).join(','));
}

if (require.main === module) {
  try { main(process.argv.slice(2)); } catch (e) { console.error('romimg: ' + e.message); process.exit(1); }
}
module.exports = { build, read, readHeader, romBank, socketBank, sums, crc16, CHIP };
