// ****************************************************************************
// forthlib.js - HyForth's libraries (docs/design/reimplementation-from-scratch.md, §16): forthlib/NAME.s, a word set each,
// assembled (ca65) and linked (ld65: forthlib/forthlib.cfg, o65, relocatable from 0) against the core's labels, then
// made a library file, obj/forthlib/NAME.fl, which forth loads into its dictionary, relocated to HERE (INCLUDED of a
// file that starts HYFL: modules/forth/ffile.inc's lib_load).  build.js runs it after the modules.
//   The core: obj/modules/forth.bin and its labels (forth.lbl).  Its id is a CRC-16 of its image with the id's own
// two bytes (core_id) 0, patched into it; a library carries the id of the core it was built for, and the loader
// refuses another's.  obj/gen/forthcore.inc: the core's labels as equates (but its locals, its headers' and the
// linker's), forth_last (its last header) and CORE_ID, for the libraries.
//   A library file (numbers low byte first):
//     0   "HYFL"
//     4   1, the format's version
//     5   the core's id (2)
//     7   the image's length (2): its code and data, loaded at HERE
//     9   its BSS's length (2): zeros after the image
//     11  its first header's link (2): an offset in the image, set as it loads to the compilation word list's last
//         header ($FFFF: no headers)
//     13  its last header (2): an offset, the word list's last then
//     15  its init routine (2): an offset, called once it's loaded ($FFFF: none)
//     17  its relocations' count (2)
//     19  the image (linked at 0), then the relocations: each an offset (2) with its kind in the top 2 bits (0: a
//         word, 1: a low byte, 2: a high byte, then its address's low byte, 1 byte more), to which HERE is added
'use strict';
const fs = require('fs');
const path = require('path');

const MAGIC = 'HYFL', VERSION = 1, NONE = 0xFFFF;
const KIND_WORD = 0, KIND_LOW = 1, KIND_HIGH = 2;

// CRC-16/CCITT (polynomial $1021, from $FFFF)
function crc16(buf) {
  let crc = 0xFFFF;
  for (const b of buf) {
    crc ^= b << 8;
    for (let i = 0; i < 8; i++) crc = crc & 0x8000 ? ((crc << 1) ^ 0x1021) & 0xFFFF : (crc << 1) & 0xFFFF;
  }
  return crc;
}

// A label file (ld65 -Ln): { name: address }
function readLabels(file) {
  const labels = {};
  for (const line of fs.readFileSync(file, 'latin1').split(/\r?\n/)) {
    const m = line.match(/^al ([0-9A-F]+) \.(\S+)$/);
    if (m) labels[m[2]] = parseInt(m[1], 16);
  }
  return labels;
}

// The core: its id (core_id's 2 bytes 0 for the CRC) patched into its image (bin, a Buffer: changed in place), and
// forthcore.inc's text.  OUT: { id, inc }
function core(bin, labels) {
  const at = labels.core_id;
  if (at === undefined) throw new Error('forth: no core_id');
  const off = at - 0xA000;
  bin[off] = bin[off + 1] = 0;
  const id = crc16(bin.subarray(0, 0x4000)) || 1;
  bin[off] = id & 0xFF;
  bin[off + 1] = id >> 8;
  let last = 0, lastN = 0;
  let inc = '; forthcore.inc - the HyForth core\'s labels, for its libraries.  Made by tools/forthlib.js from\r\n' +
    '; obj/modules/forth/forth.lbl: don\'t edit\r\n\r\n';
  for (const [name, v] of Object.entries(labels)) {
    const h = name.match(/^hdr_(\d+)$/);
    if (h) { if (+h[1] > lastN) { lastN = +h[1]; last = v; } continue; }
    if (name.startsWith('@') || name.startsWith('__')) continue;
    inc += name.padEnd(20) + '= $' + v.toString(16).toUpperCase().padStart(4, '0') + '\r\n';
  }
  inc += 'forth_last'.padEnd(20) + '= $' + last.toString(16).toUpperCase().padStart(4, '0') + '\r\n';
  inc += 'CORE_ID'.padEnd(20) + '= $' + id.toString(16).toUpperCase().padStart(4, '0') + '\r\n';
  return { id, inc };
}

// An o65 file (ld65's: simple mode, small): { image (text and data), bss, relocs: [{ off, kind, low }] }
function readO65(buf, name) {
  const fail = msg => { throw new Error(name + ': ' + msg); };
  if (buf.readUInt16LE(0) !== 0x0001 || buf.toString('latin1', 2, 5) !== 'o65') fail('not an o65 file');
  const mode = buf.readUInt16LE(6);
  if (mode & 0x2000) fail('a 32-bit o65 file');
  if (mode & 0x4000) fail('page-wise relocation');
  const w = i => buf.readUInt16LE(8 + 2 * i);
  const tbase = w(0), tlen = w(1), dbase = w(2), dlen = w(3), bbase = w(4), blen = w(5), zlen = w(7);
  if (tbase !== 0 || dbase !== tlen || bbase !== tlen + dlen) fail('its segments aren\'t one after another from 0');
  if (zlen) fail('zero page of its own (use the core\'s)');
  let p = 26;
  while (buf[p]) p += buf[p];                                     // (The header's options)
  p++;
  const image = Buffer.from(buf.subarray(p, p + tlen + dlen));
  p += tlen + dlen;
  const undef = buf.readUInt16LE(p);
  if (undef) {
    p += 2;
    const names = [];
    for (let i = 0; i < undef; i++) { const z = buf.indexOf(0, p); names.push(buf.toString('latin1', p, z)); p = z + 1; }
    fail('undefined: ' + names.join(', '));
  }
  p += 2;
  const relocs = [];
  for (const segStart of [0, tlen]) {                             // The text's relocations, then the data's
    let pos = -1;
    for (;;) {
      let b = buf[p++];
      if (b === 0) break;
      while (b === 255) { pos += 254; b = buf[p++]; }
      pos += b;
      const type = buf[p++], seg = type & 0x0F, kind = type & 0xE0;
      if (seg < 2 || seg > 4) fail('a relocation to segment ' + seg);
      const off = segStart + pos;
      if (kind === 0x80) relocs.push({ off, kind: KIND_WORD });
      else if (kind === 0x20) relocs.push({ off, kind: KIND_LOW });
      else if (kind === 0x40) relocs.push({ off, kind: KIND_HIGH, low: buf[p++] });
      else fail('a relocation of kind $' + kind.toString(16));
      if (off > 0x3FFF) fail('over 16K');
    }
  }
  return { image, bss: blen, relocs };
}

// The library file for o65 file o65 (and its label file lbl), for core id
function libraryFile(o65, lbl, id, name) {
  const { image, bss, relocs } = readO65(o65, name), labels = readLabels(lbl);
  const off = l => (labels[l] === undefined ? NONE : labels[l]);
  let lastN = 0;                                                  // (Its headers: hdr_1 to hdr_N, as HEADER makes them)
  for (const l of Object.keys(labels)) { const h = l.match(/^hdr_(\d+)$/); if (h && +h[1] > lastN) lastN = +h[1]; }
  labels.lib_first = labels.hdr_1;
  labels.lib_last = labels['hdr_' + lastN];
  const head = Buffer.alloc(19);
  head.write(MAGIC, 0, 'latin1');
  head[4] = VERSION;
  head.writeUInt16LE(id, 5);
  head.writeUInt16LE(image.length, 7);
  head.writeUInt16LE(bss, 9);
  head.writeUInt16LE(off('lib_first'), 11);
  head.writeUInt16LE(off('lib_last'), 13);
  head.writeUInt16LE(off('lib_init'), 15);
  head.writeUInt16LE(relocs.length, 17);
  const rel = [];
  for (const r of relocs) {
    const v = r.off | (r.kind << 14);
    rel.push(v & 0xFF, v >> 8);
    if (r.kind === KIND_HIGH) rel.push(r.low);
  }
  return Buffer.concat([head, image, Buffer.from(rel)]);
}

// The libraries: the core patched (modules.forth, and its file) and forthcore.inc written, then each forthlib/NAME.s
// assembled, linked and made obj/forthlib/NAME.fl.  IN: root (the reborn folder), modules (build.js's: the core's
// image), assemble(files, objdir, includes) (build.js's: the objects), ld65(args).  OUT: { NAME: the file's bytes }
function build({ root, modules, assemble, ld65 }) {
  const at = (...p) => path.join(root, ...p);
  const src = at('forthlib');
  if (!modules.forth || !fs.existsSync(src)) return {};
  const { id, inc } = core(modules.forth, readLabels(at('obj', 'modules', 'forth', 'forth.lbl')));
  fs.writeFileSync(at('obj', 'modules', 'forth.bin'), modules.forth);
  fs.writeFileSync(at('obj', 'gen', 'forthcore.inc'), inc);
  const out = at('obj', 'forthlib'), libs = {};
  fs.mkdirSync(out, { recursive: true });
  const B = (...p) => path.join(root, '..', 'base', ...p);   // (The base's: hydra.inc, the SDK's core, the hardware's)
  const includes = [at('obj', 'sdk'), at('sdk', 'asm'), B('obj', 'sdk'), B('sdk', 'asm'), B('include'), at('obj', 'gen'), at('modules', 'forth'), src];
  for (const f of fs.readdirSync(src).filter(f => f.endsWith('.s')).sort()) {
    const name = path.basename(f, '.s'), od = path.join(out, name);
    const objs = assemble([path.join(src, f)], od, includes);
    const o65 = path.join(od, name + '.o65'), lbl = path.join(od, name + '.lbl');
    ld65(['-C', path.join(src, 'forthlib.cfg'), '-o', o65, '-m', path.join(od, name + '.map'), '-Ln', lbl, ...objs]);
    libs[name] = libraryFile(fs.readFileSync(o65), lbl, id, name);
    fs.writeFileSync(path.join(out, name + '.fl'), libs[name]);
  }
  return libs;
}

module.exports = { build, crc16, readO65 };
