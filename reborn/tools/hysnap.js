// ****************************************************************************
// hysnap.js - hylang's start-up snapshot: the module hysnap, a library of data in the paged ROM that holds hylang's
// heap as it is with its library (/lib/hylang/globals.hl, the ROM disk's) loaded, so it starts without reading and
// evaluating that text (modules/hylang/hylang.s: snap_find, snap_restore).  build.js runs it after the modules.
//   hylang's id first: a CRC-16 of its image (obj/modules/hylang.bin) with the id's own two bytes (snap_id) 0, patched
// into it (1 for a CRC of 0); a snapshot carries the id of the hylang it's of, and hylang refuses another's (it loads
// its library as text then).  Then the snapshot: a paged ROM of the modules (rom.txt's but hysnap) with t_rc for init,
// booted in the emulator, hylang typed; when it's loaded its library (at lib_done, in its first bank) the machine
// stops, and what hylang keeps (its PSTATE segment: the heap's tables, the symbols, the evaluator's own) and its
// heap's banks are read from its task's RAM.
//   The module (a HYX2 header: a library, in place, named hysnap), from $A000 of its first bank:
//     $A000   its header (48 bytes)
//     $A030   its index: "HYSN"; the id (2); the cell banks, the blob banks (1 each); PSTATE's size (2); each cell
//             bank's pages kept, a mask (2 each, 16: bit n page n, those of a kind of cell); each blob bank's length
//             (2 each, 16: its bytes from its start)
//     $A100   PSTATE's bytes, then each page kept (512 bytes; the cell banks in turn, each's pages in order), then
//             each blob bank's bytes: each from a 256-byte boundary (none crosses a bank's end), $FF between
//   In its banks in turn: past $DFFF, the next's $A000 (romimg.js puts a module's banks in a row).
'use strict';
const fs = require('fs');
const path = require('path');
const romimg = require('./romimg.js');
const romfs = require('./romfs.js');

const BANK = 0x4000, WINDOW = 0xA000, MAX_BANKS = 8;
const HX = { TYPE: 5, FLAGS: 6, ABI: 7, LOAD: 8, LENGTH: 10, BANKS: 33, VERSION: 34, NAME: 36, SIZE: 48 };
const HT_LIBRARY = 3, HF_INPLACE = 0x01, ABI_VERSION = 1;
const INDEX = 48, DATA = 256, PAGE = 512, PK_SCONS = 3, PAGES = 16;
const CYCLES = 400e6;                                         // (hylang's start with its library as text: ~10M)

// CRC-16/CCITT (polynomial $1021, from $FFFF): forthlib.js's
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

// hylang's id patched into its image (bin, a Buffer: changed in place).  OUT: the id
function patchId(bin, labels) {
  const at = labels.snap_id - WINDOW;                         // (In its first bank: its RODATA)
  if (!(at >= 0 && at + 2 <= BANK)) throw new Error('hylang: snap_id isn\'t in its first bank');
  bin.writeUInt16LE(0, at);
  const id = crc16(bin) || 1;
  bin.writeUInt16LE(id, at);
  return id;
}

// The snapshot taken: hylang started in the emulator, stopped at lib_done.  opt: { modules (build.js's, by name),
// names (rom.txt's modules), rc (t_rc's image), hwtest, bios, files (romfs.js's manifest), labels, id }
function take({ modules, names, rc, hwtest, bios, files, labels, id }) {
  const { boot } = require('../sim/run.js');
  const mods = names.filter(n => n !== 'hysnap').map(n => ({ file: n, data: modules[n] }));
  mods.push({ file: 't_rc', data: rc });
  const { image, entries } = romimg.build({ modules: mods, init: 't_rc', hwtest, bios, romfs: files });
  const bank1 = entries.find(e => e.name === 'hylang').bank;
  const L = name => { const v = labels[name]; if (v === undefined) throw new Error('hylang: no label ' + name); return v; };
  let task = -1, m = null;
  m = boot({ prom: image, bios, seed: 1, marks: [], input: '\u0101hylang\r', pcWatches: [{ pc: L('lib_done'), page: -1 }],
    log: s => { if (s.startsWith('pc: ') && m.rd(1) === bank1) { task = m.T; m.cpu.halted = 'snapshot'; } } });
  while (task < 0 && !m.cpu.halted && m.cpu.cyc < CYCLES) m.run(m.cpu.cyc + 1e6);
  if (task < 0) throw new Error('hylang didn\'t load its library in the emulator' + (m.cpu.halted ? ' (' + m.cpu.halted + ')' : '') +
    '; its output: ' + JSON.stringify(m.out.slice(-300)));
  const said = m.out.replace(/\r/g, '').split('hylang\n').slice(1).join('hylang\n');
  if (said) throw new Error('hylang\'s library: ' + JSON.stringify(said.slice(0, 300)));   // (An error, on stderr)
  const ram = m.taskRam[task], byte = name => ram[L(name)];
  const pstate = Buffer.from(ram.subarray(L('__PSTATE_RUN__'), L('__PSTATE_RUN__') + L('__PSTATE_SIZE__')));
  const bankBytes = b => {
    if (b >= 0xF0) throw new Error('hylang\'s heap in a shared bank ($' + b.toString(16) + ')');
    return Buffer.from(m.taskBankMem(task, b) || new Uint8Array(0x2000));
  };
  const cells = [], blobs = [];
  for (let k = 0; k < byte('cell_banks'); k++) {
    const mem = bankBytes(ram[L('cell_bank') + k]), pages = [];
    for (let j = 0; j < PAGES; j++)
      if (ram[L('pk') + PAGES * k + j] >= PK_SCONS) pages.push([j, mem.subarray(PAGE * j, PAGE * (j + 1))]);
    cells.push(pages);
  }
  for (let k = 0; k < byte('blob_banks'); k++) {
    const length = ram[L('blob_tlo') + k] | ram[L('blob_thi') + k] << 8;
    blobs.push(bankBytes(ram[L('blob_bank') + k]).subarray(0, length));
  }
  return { pstate, cells, blobs, cycles: m.cpu.cyc };
}

// The module's image from a snapshot
function makeModule({ id, pstate, cells, blobs }) {
  const pad = b => Buffer.concat([b, Buffer.alloc(-b.length & 0xFF, 0xFF)]);
  const index = Buffer.alloc(10 + 32 + 32, 0);
  index.write('HYSN', 0, 'latin1');
  index.writeUInt16LE(id, 4);
  index[6] = cells.length; index[7] = blobs.length;
  index.writeUInt16LE(pstate.length, 8);
  cells.forEach((pages, k) => index.writeUInt16LE(pages.reduce((mask, [j]) => mask | 1 << j, 0), 10 + 2 * k));
  blobs.forEach((b, k) => index.writeUInt16LE(b.length, 42 + 2 * k));
  const head = Buffer.alloc(DATA, 0xFF);
  head.fill(0, 0, HX.SIZE);
  head.write('HYX2', 0, 'latin1');
  head[4] = HX.SIZE; head[HX.TYPE] = HT_LIBRARY; head[HX.FLAGS] = HF_INPLACE; head[HX.ABI] = ABI_VERSION;
  head.writeUInt16LE(WINDOW, HX.LOAD);
  head.writeUInt16LE(1, HX.VERSION);
  head.write('hysnap', HX.NAME, 'latin1');
  index.copy(head, INDEX);
  const data = Buffer.concat([head, pad(pstate), ...cells.flat().map(([, p]) => p), ...blobs.map(pad)]);
  const banks = Math.ceil(data.length / BANK);
  if (banks > MAX_BANKS) throw new Error('hysnap: ' + data.length + ' bytes, more than ' + MAX_BANKS + ' banks');
  data[HX.BANKS] = banks;
  data.writeUInt16LE(data.length - (banks - 1) * BANK, HX.LENGTH);
  return data;
}

// hylang's id patched in (modules.hylang and obj/modules/hylang.bin), the snapshot taken, the module made
// (modules.hysnap and obj/modules/hysnap.bin).  opt: { root, modules, names (rom.txt's), rc, hwtest, bios }.
// OUT: { id, bytes, pages, blobBytes, cycles }
function build({ root, modules, names, rc, hwtest, bios }) {
  const at = (...p) => path.join(root, ...p);
  const labels = readLabels(at('obj', 'modules', 'hylang', 'hylang.lbl'));
  const id = patchId(modules.hylang, labels);
  fs.writeFileSync(at('obj', 'modules', 'hylang.bin'), modules.hylang);
  const snap = take({ modules, names, rc, hwtest, bios, files: romfs.manifest(at('romfs', 'romfs.txt')), labels, id });
  const data = makeModule({ id, ...snap });
  modules.hysnap = data;
  fs.writeFileSync(at('obj', 'modules', 'hysnap.bin'), data);
  return { id, bytes: data.length, pages: snap.cells.reduce((n, p) => n + p.length, 0),
    blobBytes: snap.blobs.reduce((n, b) => n + b.length, 0), cycles: snap.cycles };
}

module.exports = { build, crc16 };
