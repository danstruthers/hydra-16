// ****************************************************************************
// romfs.js - the ROM disk (docs/reimplementation-from-scratch.md, §8.3; phase 3.5): the paged ROM as one disk (x, the
// storage driver's), with a partition table in its block 0, the system's banks (the module directory, the hardware
// test, the modules) a partition of their own, and a read-only HydraFS volume in the banks after the modules, which
// init mounts at /rom (mount '#f' /rom x).  Its files come from a manifest (romfs/romfs.txt); the volume is made with
// the HydraFS PC tool (sim/tools/hydrafs.js), stamped 2000-01-01, so each build makes the same; tools/romimg.js puts
// it in the paged ROM image, then reads every file back as the CPU sees it (readBack).
//
//   block 0               bank 0's first block: romimg.js's signature line, then the partition table (an MBR)
//   blocks 1 ... S-1      partition 1, type $DA (not a file system): the module directory, the hardware test, the
//                         modules
//   blocks S ...          partition 2, type $7F: the HydraFS volume, from the bank after the modules, as many whole
//                         banks as its files need
// The disk is the paged ROM in socket order (romimg.js: socketBank).  Only the blocks the volume uses are in the
// image: the rest of it reads as erased ($FF), and nothing reads it.
//
// From Node: manifest(file) gives the files ([{ path, data, src }]); volume(files, blocks) the volume's blocks up to
// its last used one (a Buffer); table(start, blocks) block 0's partition table; readBack(read, blocks, files) checks
// each file.
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const hydrafs = require('../sim/tools/hydrafs.js');

const BLOCK = 512, LABEL = 'ROM', PART_SYSTEM = 0xDA;

// The manifest: [{ path, data, src }], from its "path source" lines (the source relative to the manifest's folder)
function manifest(file) {
  const base = path.dirname(file), files = [], seen = new Set();
  fs.readFileSync(file, 'latin1').split(/\r?\n/).forEach((raw, n) => {
    const line = raw.replace(/;.*/, '').trim();
    if (!line) return;
    const [where, src] = line.split(/\s+/);
    if (!src) throw new Error(file + ':' + (n + 1) + ': a path on the ROM disk, then its source');
    const names = where.split('/').filter(x => x), p = '/' + names.join('/');
    for (const name of names) if (name.length > 31) throw new Error(where + ': "' + name + '" is longer than 31 characters');
    if (seen.has(p)) throw new Error(where + ': twice');
    seen.add(p);
    files.push({ path: p, src, data: fs.readFileSync(path.join(base, src)) });
  });
  return files;
}

// A HydraFS volume of `blocks` blocks with the files in it (their directories made for them; each directory's
// entries by name), stamped 2000-01-01.  OUT: its blocks up to its last used one (those after are all zeros, and
// needn't be in the image)
function volume(files, blocks) {
  const tmp = path.join(os.tmpdir(), 'reborn-romfs-' + process.pid + '.img');
  try {
    hydrafs.setNow(0);
    hydrafs.mkfs(tmp, 0, LABEL, blocks);
    const v = new hydrafs.Volume(tmp);
    for (const f of [...files].sort((a, b) => (a.path < b.path ? -1 : 1))) {
      const dir = f.path.split('/').slice(0, -1).join('/');
      if (dir) v.mkdir(dir);
      v.put(f.path, f.data);
    }
    v.close();
    const img = fs.readFileSync(tmp);
    let used = img.length / BLOCK;
    while (used > 0 && !img.subarray((used - 1) * BLOCK, used * BLOCK).some(x => x)) used--;
    return Buffer.from(img.subarray(0, used * BLOCK));
  } finally { fs.rmSync(tmp, { force: true }); }
}

// Block 0's partition table: partition 1 the system's banks (blocks 1 to start - 1), partition 2 the volume (`blocks`
// from `start`).  OUT: 512 bytes (the table at $1BE, $55 $AA at the end, zeros before: room for a signature line)
function table(start, blocks) {
  return hydrafs.mbrMake([{ type: PART_SYSTEM, start: 1, blocks: start - 1 }, { type: hydrafs.PART_TYPE, start, blocks }]);
}

// Every file read back from the disk's first `blocks` blocks (read(n): block n, as the CPU sees it), as a HydraFS
// found through block 0's partition table, as the Hydra finds it: each must be its source, byte for byte, wherever
// its blocks are.  OUT: [{ path, size, src, banks }]; throws if one differs
function readBack(read, blocks, files) {
  const tmp = path.join(os.tmpdir(), 'reborn-romfs-check-' + process.pid + '.img'), disk = Buffer.alloc(blocks * BLOCK);
  for (let n = 0; n < blocks; n++) read(n).copy(disk, n * BLOCK);
  fs.writeFileSync(tmp, disk);
  try {
    const v = new hydrafs.Volume(tmp), out = [];
    for (const f of files) {
      const e = v.walk(f.path), got = v.read(e);
      if (!got.equals(f.data)) throw new Error('the ROM disk\'s ' + f.path + ' doesn\'t read back as ' + f.src);
      const banks = new Set();
      for (let p = 0; p < f.data.length; p += BLOCK) banks.add(Math.floor((v.base + v.fileBlock(e, p)) / 32));
      out.push({ path: f.path, size: f.data.length, src: f.src, banks: [...banks].sort((a, b) => a - b) });
    }
    v.close();
    return out;
  } finally { fs.rmSync(tmp, { force: true }); }
}

module.exports = { manifest, volume, table, readBack };
