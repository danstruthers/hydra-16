#!/usr/bin/env node
// ****************************************************************************
// sdcard.js - HydraOS's SD card image (bin/sdcard.img): the files sdcard/sdcard.txt lists (the samples, the sample
// songs: romfs.js's manifest format), on a HydraFS of SIZE_MB megabytes in a partition (a partition table in block 0,
// the volume from block 2048, as sim/tools/hydrafs.js's mkfs -p 0 makes it), so a PC sees a card it knows and leaves
// alone.  Stamped at STAMP, so each build makes the same image, byte for byte; every file read back as its source.
// The build (build.js) makes it after the samples; the tests that run a sample have it as their card (SD device 0:
// /sd/0); sim/tools/sdwrite.js writes it to a card on the PC.
//
// Usage: node tools/sdcard.js [OUT]        (after a build: OUT, default bin/sdcard.img)
// From Node: build(root, out) gives { files, bytes, written } (written: false if the image was already that)
'use strict';
const fs = require('fs');
const path = require('path');
const os = require('os');
const hydrafs = require('../sim/tools/hydrafs.js');
const romfs = require('./romfs.js');

const SIZE_MB = 16;                                          // The volume (the image is 1M more: the partition's start)
const LABEL = 'HYDRA-16';
const STAMP = (Date.UTC(2026, 0, 1) - Date.UTC(2000, 0, 1)) / 1000;   // 2026-01-01 00:00:00, in seconds since 2000 (the Hydra's)

function build(root, out = path.join(root, 'bin', 'sdcard.img')) {
  const files = romfs.manifest(path.join(root, 'sdcard', 'sdcard.txt'));
  const tmp = path.join(os.tmpdir(), 'reborn-sdcard-' + process.pid + '.img');
  try {
    hydrafs.setNow(STAMP);
    hydrafs.mkfs(tmp, SIZE_MB, LABEL, undefined, false, 0);
    const v = new hydrafs.Volume(tmp);
    for (const f of [...files].sort((a, b) => (a.path < b.path ? -1 : 1))) {
      const dir = f.path.split('/').slice(0, -1).join('/');
      if (dir) v.mkdir(dir);
      v.put(f.path, f.data);
    }
    v.close();
    const check = new hydrafs.Volume(tmp);                    // Each file read back
    for (const f of files) {
      const e = check.tryWalk(f.path), back = e && !e.isDir ? check.read(e) : null;
      if (!back || !Buffer.from(back).equals(f.data)) throw new Error('sdcard: ' + f.path + ' read back differs');
    }
    check.close();
    const img = fs.readFileSync(tmp);
    const same = fs.existsSync(out) && fs.readFileSync(out).equals(img);
    if (!same) { fs.mkdirSync(path.dirname(out), { recursive: true }); fs.writeFileSync(out, img); }
    return { files, bytes: files.reduce((n, f) => n + f.data.length, 0), size: img.length, written: !same };
  } finally {
    hydrafs.setNow(null);
    fs.rmSync(tmp, { force: true });
  }
}

if (require.main === module) {
  try {
    const root = path.join(__dirname, '..'), out = process.argv[2] ? path.resolve(process.argv[2]) : undefined;
    const r = build(root, out);
    console.log('sdcard: ' + r.files.length + ' files, ' + r.bytes + ' bytes; ' + (r.size / 1048576) + ' MB image' + (r.written ? '' : ' (unchanged)'));
  } catch (e) { console.error(e.message); process.exit(1); }
}
module.exports = { build, SIZE_MB, LABEL };
