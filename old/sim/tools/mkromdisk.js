#!/usr/bin/env node
// mkromdisk.js: the paged ROM as one disk, the ROM disk: a partition table in its block 0, the system's banks
// (HyForth's variables, the hardware test) in a partition of their own, and a read-only HydraFS
// volume in the rest, which the Hydra mounts at /rom (mount hfs /rom x) (docs/plans/DISKS.md).  The files come from
// a manifest; the HydraFS is made with hydrafs.js (as a card's), stamped 2000-01-01 so the image is the same each
// build, and written into the paged ROM image after ld65 has written the system's banks (build.js).
//
// Usage: node mkromdisk.js MANIFEST PAGED_ROM.bin [--list]
//   MANIFEST: a line per file, "path/in/rom  source" (the source relative to the manifest's folder); ';' starts a
//   comment.  Directories are made for the paths; each directory's entries go in by name.  --list: what's where.
//
// The disk is the paged ROM as the CPU sees it, bank after bank: block n is bank n / 32, at $A000 + (n % 32) * 512.
//   block 0               the partition table (an MBR, as a card's)
//   blocks 1-63           partition 1, type $DA (not a filesystem): the system's banks 0-1 (but bank 0's block 0)
//   blocks 64-8191        partition 2, type $7F: the HydraFS volume, to the end of the 4 MB paged ROM
// The image holds each bank's $C000 half first (the board swaps A13), and on the V1 board a bank number's bits 2
// and 3 (and 6 and 7) trade places before they reach the chips, so bank b sits at bank swap(b)'s place in the image.
// Only the blocks the volume uses are written: the rest of the ROM reads as erased ($FF), and nothing reads it.
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const hydrafs = require('./hydrafs.js');

const BLOCK = 512, BANK = 0x4000, BLOCKS_PER_BANK = BANK / BLOCK;
const SYSTEM_BANKS = 2, DISK_BANKS = 256;                         // Banks 0-1 the system's; 4 MB in all
const PART_START = SYSTEM_BANKS * BLOCKS_PER_BANK, PART_BLOCKS = DISK_BANKS * BLOCKS_PER_BANK - PART_START;
const PART_SYSTEM = 0xDA, LABEL = 'ROM';
// Block 0's first bytes: what the disk is.  Bank 0's first page must read as no other bank's (the hardware test's
// bank lines: hwtest/hwt_rom.s), and the table alone leaves it all zeros, as an empty block in a later bank can be
const SIGNATURE = 'Hydra-16 ROM disk: the paged ROM as one disk (block 0: this table; 1-63: the system; 64 on: /rom)\r\n';
const MAX_IMAGE_BANKS = 111;                                      // (romsum.js's table in bank 1: 111 paged ROM banks)
const swap = b => (b & 0x33) | ((b & 0x04) << 1) | ((b & 0x08) >> 1) | ((b & 0x40) << 1) | ((b & 0x80) >> 1);

// The manifest: { name: { dir, kids } | { data } }, from "path source" lines
function manifest(file) {
  const base = path.dirname(file), root = new Map();
  for (const [n, raw] of fs.readFileSync(file, 'latin1').split(/\r?\n/).entries()) {
    const line = raw.replace(/;.*/, '').trim();
    if (!line) continue;
    const [where, src] = line.split(/\s+/);
    if (!src) throw new Error(file + ':' + (n + 1) + ': a path in /rom, then its source');
    const parts = where.split('/').filter(x => x);
    let d = root;
    parts.forEach((p, k) => {
      if (p.length > 31) throw new Error(where + ': "' + p + '" is longer than 31 characters');
      if (k < parts.length - 1) {
        if (!d.has(p)) d.set(p, new Map());
        if (!(d.get(p) instanceof Map)) throw new Error(where + ': ' + p + ' is a file');
        d = d.get(p);
      } else {
        if (d.has(p)) throw new Error(where + ': twice');
        d.set(p, { data: fs.readFileSync(path.join(base, src)), src });
      }
    });
  }
  return root;
}

// The volume: a HydraFS image file, its entries made directory by directory, by name.  OUT: the files, for --list
function makeVolume(file, root) {
  hydrafs.setNow(0);                                              // (2000-01-01 00:00:00: the same each build)
  hydrafs.mkfs(file, 0, LABEL, PART_BLOCKS);
  const v = new hydrafs.Volume(file), files = [];
  const fill = (dir, at) => {
    for (const name of [...dir.keys()].sort()) {
      const e = dir.get(name), p = at + '/' + name;
      if (e instanceof Map) { v.mkdir(p); fill(e, p); }
      else { v.put(p, e.data); files.push({ path: p, size: e.data.length, src: e.src, data: e.data }); }
    }
  };
  fill(root, '');
  v.close();
  return files;
}

// The read-back: the disk as the CPU sees it, through the emulator's own bank mapping (lib/machine.js: the V1
// board's bank-bit swaps, and the A13 half-swap), read as a HydraFS: every file must be its source, byte for byte,
// whatever banks it's in.  OUT: each file's banks (for --list); throws if one differs
function readBack(image, files, used) {
  const { romBank } = require('../lib/machine.js');
  const blocks = PART_START + used, disk = Buffer.alloc(blocks * BLOCK);
  for (let d = 0; d < blocks; d++) {
    const bank = Math.floor(d / BLOCKS_PER_BANK), at = 0xA000 + (d % BLOCKS_PER_BANK) * BLOCK;
    for (let i = 0; i < BLOCK; i++) {
      const off = romBank(bank) * BANK + ((at + i - 0xA000) ^ 0x2000);
      disk[d * BLOCK + i] = off < image.length ? image[off] : 0xFF;
    }
  }
  const tmp = path.join(os.tmpdir(), 'hydra-romdisk-check-' + process.pid + '.img');
  fs.writeFileSync(tmp, disk);
  try {
    const v = new hydrafs.Volume(tmp);
    for (const f of files) {
      const e = v.walk(f.path), got = v.read(e);
      if (!got.equals(f.data)) throw new Error('the ROM disk\'s ' + f.path + ' doesn\'t read back as ' + f.src);
      const banks = new Set();
      for (let p = 0; p < f.size; p += BLOCK) banks.add(Math.floor((v.base + v.fileBlock(e, p)) / BLOCKS_PER_BANK));
      f.banks = [...banks].sort((a, b) => a - b);
    }
    v.close();
  } finally { fs.rmSync(tmp, { force: true }); }
}

function main(argv) {
  const [manifestFile, imageFile] = argv.filter(a => !a.startsWith('--'));
  if (!manifestFile || !imageFile) { console.error('Usage: node mkromdisk.js MANIFEST PAGED_ROM.bin [--list]'); process.exit(1); }
  const tmp = path.join(os.tmpdir(), 'hydra-romdisk-' + process.pid + '.img');
  let files, vol;
  try { files = makeVolume(tmp, manifest(manifestFile)); vol = fs.readFileSync(tmp); }
  finally { fs.rmSync(tmp, { force: true }); }
  let used = vol.length / BLOCK;                                  // Its last block that isn't all zeros (unused: unwritten)
  while (used > 0 && !vol.subarray((used - 1) * BLOCK, used * BLOCK).some(x => x)) used--;

  // The disk's blocks, into the image: block 0 (the table), then the volume's from PART_START
  const image = fs.readFileSync(imageFile);
  const mbr = hydrafs.mbrMake([{ type: PART_SYSTEM, start: 1, blocks: PART_START - 1 }, { type: hydrafs.PART_TYPE, start: PART_START, blocks: PART_BLOCKS }]);
  mbr.write(SIGNATURE, 0, 'latin1');                              // (In its boot code's room, which nothing runs)
  const blocks = [[0, mbr]];
  for (let b = 0; b < used; b++) blocks.push([PART_START + b, vol.subarray(b * BLOCK, (b + 1) * BLOCK)]);
  const at = d => swap(Math.floor(d / BLOCKS_PER_BANK)) * BANK + (((d % BLOCKS_PER_BANK) * BLOCK) ^ 0x2000);
  const banks = Math.max(image.length / BANK, ...blocks.map(([d]) => swap(Math.floor(d / BLOCKS_PER_BANK)) + 1));
  if (banks > MAX_IMAGE_BANKS) throw new Error('the ROM disk needs ' + banks + ' banks of image; romsum.js\'s table has room for ' + MAX_IMAGE_BANKS);
  const out = Buffer.alloc(banks * BANK, 0xFF);
  image.copy(out);
  for (const [d, data] of blocks) data.copy(out, at(d));
  fs.writeFileSync(imageFile, out);
  readBack(out, files, used);

  const bytes = files.reduce((n, f) => n + f.size, 0), lastBank = Math.floor((PART_START + used - 1) / BLOCKS_PER_BANK);
  console.log('ROM disk: ' + files.length + ' files, ' + bytes + ' bytes; its volume uses ' + used + ' of ' + PART_BLOCKS +
    ' blocks (paged ROM banks ' + SYSTEM_BANKS + '-' + lastBank + '; the image: ' + banks + ' banks)');
  const crossing = files.filter(f => f.banks.length > 1).length;
  console.log('ROM disk read back: every file as its source (' + crossing + ' of them across banks)');
  if (argv.includes('--list')) for (const f of files) console.log('  /rom' + f.path.padEnd(28) + String(f.size).padStart(7) + '  banks ' + f.banks.join(',').padEnd(8) + f.src);
}

main(process.argv.slice(2));
