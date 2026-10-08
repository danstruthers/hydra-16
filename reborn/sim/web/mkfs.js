// mkfs.js - a new HydraFS card's image in memory, for the browser emulator's page (web/page.js): as sim/tools/hydrafs.js's
// mkfs makes one, without a partition table (web.js --check holds the two against each other).  No Node.js.
'use strict';

// A new HydraFS card of mb megabytes, as sim/tools/hydrafs.js's mkfs makes one (no partition table): the
// superblock in block 0, the free map after it (all free), its root directory empty
function mkfs(mb, label = '', stamp = null) {
  const BLOCK = 512, SHIFT = 3, CB = 1 << SHIFT, blocks = Math.floor(mb * 1024 * 1024 / BLOCK);
  const mapBlocks = Math.ceil(Math.floor((blocks - 1) / CB) / (BLOCK * 8)), dataStart = 1 + mapBlocks;
  const clusters = Math.floor((blocks - dataStart) / CB), img = new ArrayBuffer(blocks * BLOCK), b = new Uint8Array(img, 0, BLOCK);
  const dv = new DataView(img), d = new Date();
  stamp = stamp !== null ? stamp : Math.max(0, Math.floor(d.getTime() / 1000 - d.getTimezoneOffset() * 60 - Date.UTC(2000, 0, 1) / 1000));
  [...'HYDRAFS1'].forEach((c, i) => { b[i] = c.charCodeAt(0); });
  b[8] = 1; b[9] = SHIFT;                                       // (Version 1: the free map written)
  for (const [o, v] of [[12, clusters], [16, 1], [20, mapBlocks], [24, dataStart], [28, clusters], [32, 0], [36, 2], [40, stamp]]) dv.setUint32(o, v, true);
  b[64] = 0x2F; b[96] = 0x80; dv.setUint32(100, 1, true);       // The root's entry: "/", a directory, qid 1
  [...label].forEach((c, i) => { b[128 + i] = c.charCodeAt(0) & 0xFF; });
  return img;
}

module.exports = { mkfs };
