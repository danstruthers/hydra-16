// ****************************************************************************
// images.js - a system's ROM images in its bin folder, for the two serial chips the Hydra-16 can have in U3:
// Rockwell's R65C51 and WDC's W65C51N (whose transmitter needs the kernel's workaround: ACIA_CHIP).  build.js builds
// the system once for each, and an image that comes out the same both times has one file, NAME.bin (prom0.bin); one
// that doesn't, a file for each, NAME_rockwell.bin and NAME_wdc.bin (the BIOS ROM's, the one that does:
// bios_rockwell.bin, bios_wdc.bin).  The emulators and the tests read them through here, for the chip they model; an
// older tree's bin/bios.bin too.
//   file(root, name, acia)      the path of image NAME (bios, prom0 ...) for acia ('rockwell' or 'wdc'), or null
//   bios(root, acia)            the BIOS ROM's bytes
//   chipList(root, acia)        the paged ROM's chips, socket 0 on: [Buffer]
//   built(root)                 the chip the last build left its objects for (obj/build.json's acia: the tests')
//   write(dir, made, names)     made: { rockwell: { file: Buffer }, wdc: { ... } } (each build's images, by their
//                               plain file names): each written as one file, or a file for each chip if they
//                               differ; the files of names (a RegExp) that aren't the build's any more removed
'use strict';
const fs = require('fs');
const path = require('path');

const ACIAS = ['rockwell', 'wdc'];

function file(root, name, acia = 'rockwell') {
  for (const f of [name + '_' + acia + '.bin', name + '.bin']) {
    const p = path.join(root, 'bin', f);
    if (fs.existsSync(p)) return p;
  }
  return null;
}

function bios(root, acia) {
  const p = file(root, 'bios', acia);
  if (!p) throw new Error('no ' + path.join('bin', 'bios_' + (acia || 'rockwell') + '.bin') + ': node build.js');
  return fs.readFileSync(p);
}

function chipList(root, acia) {
  const out = [];
  for (let k = 0, p; (p = file(root, 'prom' + k, acia)); k++) out.push(fs.readFileSync(p));
  if (!out.length) throw new Error('no ' + path.join('bin', 'prom0.bin') + ': node build.js');
  return out;
}

function built(root) {
  try { return JSON.parse(fs.readFileSync(path.join(root, 'obj', 'build.json'), 'utf8')).acia || 'rockwell'; }
  catch (e) { return 'rockwell'; }
}

function write(dir, made, names) {
  const keep = new Set();
  const all = [...new Set(ACIAS.flatMap(a => Object.keys(made[a] || {})))].sort();
  for (const f of all) {
    const [r, w] = ACIAS.map(a => made[a] && made[a][f]);
    const ext = path.extname(f), stem = f.slice(0, -ext.length);
    if (r && w && r.equals(w)) { fs.writeFileSync(path.join(dir, f), r); keep.add(f); continue; }
    ACIAS.forEach((a, i) => {
      const data = i ? w : r;
      if (!data) return;
      const g = stem + '_' + a + ext;
      fs.writeFileSync(path.join(dir, g), data);
      keep.add(g);
    });
  }
  for (const f of fs.readdirSync(dir)) if (names.test(f) && !keep.has(f)) fs.rmSync(path.join(dir, f));
  return [...keep].sort();
}

module.exports = { ACIAS, file, bios, chipList, built, write };
