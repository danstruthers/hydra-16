#!/usr/bin/env node
// ****************************************************************************
// hydrafs.js - HydraFS card images on the PC (the format: docs/plans/HYDRAFS.md).  Makes test cards for the
// emulator (--sd), and moves files to and from a real card's image (written or read with a disk imager).
//
// Usage: node hydrafs.js COMMAND IMAGE [ARGS]
//   mkfs IMAGE MB [LABEL]       a new, empty HydraFS image of MB megabytes (an existing file is replaced)
//   info IMAGE                  the label, size, and free space
//   ls IMAGE [PATH]             a directory's entries ("name size", "name/" for a directory), as the
//                               Hydra's text directory read gives them; or one file's
//   put IMAGE FILE PATH         copy a PC file in (PATH: the new name, or a directory to put it in)
//   get IMAGE PATH FILE         copy a file out to the PC
//   mkdir IMAGE PATH            make a directory (and any missing ones above it)
//   rm IMAGE PATH               remove a file, or an empty directory
//   import IMAGE FOLDER [PATH]  copy a whole PC folder tree in (to PATH, default the root)
//   check IMAGE                 check the free map against the files (lost or doubly used clusters)
// Paths on the card start at its root: /games/star.frt (the Hydra sees it as /sd/N/games/star.frt); the
// first / can be left out (in Git Bash, leave it out: it turns /games into a Windows path).
//
// As a module: require('./hydrafs.js') gives { mkfs, Volume }, e.g. for regress.js's test cards.
// ****************************************************************************
'use strict';
const fs = require('fs');
const path = require('path');

const BLOCK = 512, CLUSTER_SHIFT = 3, CLUSTER_BLOCKS = 1 << CLUSTER_SHIFT, CLUSTER = BLOCK * CLUSTER_BLOCKS;
const ENTRY = 64, NAME_MAX = 31, MAP_BITS = BLOCK * 8, EXT_PER_BLOCK = 84, EXT_MAX = 0xFFFF;
const MAGIC = 'HYDRAFS1', VERSION = 1, ROOT_LOC = { block: 0, off: 64 };
const MODE_DIR = 0x80, MODE_APPEND = 0x40, MODE_RO = 0x01;

// A directory entry (64 bytes, at loc: its block and offset)
class Entry {
  constructor(buf, loc) { this.buf = buf; this.loc = loc; }
  get name() { const n = this.buf.indexOf(0); return this.buf.toString('latin1', 0, n < 0 || n > 32 ? 32 : n); }
  set name(s) { this.buf.fill(0, 0, 32); this.buf.write(s, 0, 'latin1'); }
  get free() { return this.buf[0] === 0; }
  get mode() { return this.buf[32]; } set mode(v) { this.buf[32] = v; }
  get isDir() { return (this.mode & MODE_DIR) !== 0; }
  get qver() { return this.buf.readUInt16LE(34); } set qver(v) { this.buf.writeUInt16LE(v & 0xFFFF, 34); }
  get qid() { return this.buf.readUInt32LE(36); } set qid(v) { this.buf.writeUInt32LE(v >>> 0, 36); }
  get size() { return this.buf.readUInt32LE(40); } set size(v) { this.buf.writeUInt32LE(v >>> 0, 40); }
  get stamp() { return this.buf.readUInt32LE(44); } set stamp(v) { this.buf.writeUInt32LE(v >>> 0, 44); }
  ext(i) { return { start: this.buf.readUInt32LE(48 + i * 6), len: this.buf.readUInt16LE(52 + i * 6) }; }
  setExt(i, e) { this.buf.writeUInt32LE(e ? e.start : 0, 48 + i * 6); this.buf.writeUInt16LE(e ? e.len : 0, 52 + i * 6); }
  get extBlock() { return this.buf.readUInt32LE(60); } set extBlock(v) { this.buf.writeUInt32LE(v >>> 0, 60); }
}

// Make an empty HydraFS image: MB megabytes (or blocks, if given), with a label
function mkfs(file, mb, label = '', blocks = Math.floor(mb * 1024 * 1024 / BLOCK)) {
  if (label.length > NAME_MAX) throw new Error('the label is longer than ' + NAME_MAX + ' characters');
  const mapBlocks = Math.ceil(Math.floor((blocks - 1) / CLUSTER_BLOCKS) / MAP_BITS);
  const dataStart = 1 + mapBlocks, clusters = Math.floor((blocks - dataStart) / CLUSTER_BLOCKS);
  if (clusters < 2) throw new Error('too small for HydraFS');
  const fd = fs.openSync(file, 'w+');
  fs.ftruncateSync(fd, blocks * BLOCK);
  const sb = Buffer.alloc(BLOCK);
  sb.write(MAGIC, 0, 'latin1');
  sb[8] = VERSION; sb[9] = CLUSTER_SHIFT;
  sb.writeUInt32LE(clusters, 12); sb.writeUInt32LE(1, 16); sb.writeUInt32LE(mapBlocks, 20);
  sb.writeUInt32LE(dataStart, 24); sb.writeUInt32LE(clusters, 28); sb.writeUInt32LE(0, 32);
  sb.writeUInt32LE(2, 36); sb.writeUInt32LE(1, 40);                 // Next qid (the root has 1), next stamp
  const root = new Entry(sb.subarray(64, 128), ROOT_LOC);
  root.name = '/'; root.mode = MODE_DIR; root.qid = 1;
  sb.write(label, 128, 'latin1');
  fs.writeSync(fd, sb, 0, BLOCK, 0);
  const zero = Buffer.alloc(BLOCK);                                  // The free map: all free
  for (let b = 0; b < mapBlocks; b++) fs.writeSync(fd, zero, 0, BLOCK, (1 + b) * BLOCK);
  fs.closeSync(fd);
}

class Volume {
  constructor(file) {
    this.fd = fs.openSync(file, 'r+');
    this.sb = this.readBlock(0);
    if (this.sb.toString('latin1', 0, 8) !== MAGIC) throw new Error(file + ' isn\'t a HydraFS image');
    if (this.sb[8] !== VERSION || this.sb[9] !== CLUSTER_SHIFT) throw new Error('HydraFS version ' + this.sb[8] + ' isn\'t supported');
    this.map = new Map();                                            // Free map blocks read so far
  }
  close() { this.flush(); fs.closeSync(this.fd); }

  readBlock(b) { const buf = Buffer.alloc(BLOCK); fs.readSync(this.fd, buf, 0, BLOCK, b * BLOCK); return buf; }
  writeBlock(b, buf) { fs.writeSync(this.fd, buf, 0, BLOCK, b * BLOCK); }

  // The superblock's numbers
  get clusters() { return this.sb.readUInt32LE(12); }
  get mapStart() { return this.sb.readUInt32LE(16); }
  get dataStart() { return this.sb.readUInt32LE(24); }
  get freeCount() { return this.sb.readUInt32LE(28); } set freeCount(v) { this.sb.writeUInt32LE(v, 28); }
  get hint() { return this.sb.readUInt32LE(32); } set hint(v) { this.sb.writeUInt32LE(v >>> 0, 32); }
  get label() { const n = this.sb.indexOf(0, 128); return this.sb.toString('latin1', 128, Math.min(n, 160)); }
  nextQid() { const q = this.sb.readUInt32LE(36); this.sb.writeUInt32LE(q + 1, 36); return q; }
  nextStamp() { const s = this.sb.readUInt32LE(40); this.sb.writeUInt32LE(s + 1, 40); return s; }
  flush() {
    for (const [b, buf] of this.map) if (buf.dirty) { this.writeBlock(b, buf); buf.dirty = false; }
    this.sb.set(this.root.buf, 64);
    this.writeBlock(0, this.sb);
  }

  // The free map: 1 bit per cluster (1 = in use)
  mapBlock(c) {
    const b = this.mapStart + Math.floor(c / MAP_BITS);
    if (!this.map.has(b)) this.map.set(b, this.readBlock(b));
    return this.map.get(b);
  }
  used(c) { return (this.mapBlock(c)[(c % MAP_BITS) >> 3] >> (c & 7)) & 1; }
  setUsed(c, on) {
    const m = this.mapBlock(c), i = (c % MAP_BITS) >> 3;
    m[i] = on ? m[i] | (1 << (c & 7)) : m[i] & ~(1 << (c & 7)); m.dirty = true;
  }
  // A run of up to n free clusters: at `near` if it's free, else the first one from the hint
  alloc(n, near = -1) {
    let start = -1;
    if (near >= 0 && near < this.clusters && !this.used(near)) start = near;
    for (let i = 0; start < 0 && i < this.clusters; i++) {
      const c = (this.hint + i) % this.clusters;
      if (!this.used(c)) start = c;
    }
    if (start < 0) throw new Error('the card is full');
    let len = 0;
    while (len < n && len < EXT_MAX && start + len < this.clusters && !this.used(start + len)) this.setUsed(start + len++, true);
    this.freeCount -= len; this.hint = (start + len) % this.clusters;
    return { start, len };
  }
  release(start, len) {
    for (let c = start; c < start + len; c++) { if (!this.used(c)) throw new Error('cluster ' + c + ' was free already'); this.setUsed(c, false); }
    this.freeCount += len;
  }
  clusterBlock(c) { return this.dataStart + c * CLUSTER_BLOCKS; }

  // Entries
  get root() { if (!this._root) this._root = new Entry(Buffer.from(this.sb.subarray(64, 128)), ROOT_LOC); return this._root; }
  readEntry(loc) {
    if (loc.block === 0) return this.root;
    return new Entry(this.readBlock(loc.block).subarray(loc.off, loc.off + ENTRY), loc);
  }
  writeEntry(e) {
    if (e.loc.block === 0) return;                                   // The root: in the superblock (flush)
    const b = this.readBlock(e.loc.block);
    b.set(e.buf, e.loc.off);
    this.writeBlock(e.loc.block, b);
  }
  touch(e) { e.qver = e.qver + 1; e.stamp = this.nextStamp(); }

  // A file's extents: the entry's two, then its extent blocks'
  extents(e) {
    const list = [];
    for (let i = 0; i < 2; i++) { const x = e.ext(i); if (x.len) list.push(x); }
    for (let b = e.extBlock; b; ) {
      const buf = this.readBlock(b), n = buf.readUInt16LE(4);
      for (let i = 0; i < n; i++) list.push({ start: buf.readUInt32LE(8 + i * 6), len: buf.readUInt16LE(12 + i * 6) });
      b = buf.readUInt32LE(0);
    }
    return list;
  }
  extentBlocks(e) {
    const list = [];
    for (let b = e.extBlock; b; b = this.readBlock(b).readUInt32LE(0)) list.push(b);
    return list;
  }
  // Store a file's extents: the first two in the entry, the rest in a chain of extent blocks (a cluster
  // each, its first block)
  setExtents(e, list) {
    for (const b of this.extentBlocks(e)) this.release((b - this.dataStart) / CLUSTER_BLOCKS, 1);
    e.setExt(0, list[0]); e.setExt(1, list[1]); e.extBlock = 0;
    const rest = list.slice(2), blocks = [];
    for (let i = 0; i < rest.length; i += EXT_PER_BLOCK) blocks.push(this.clusterBlock(this.alloc(1).start));
    blocks.forEach((b, j) => {
      const buf = Buffer.alloc(BLOCK), part = rest.slice(j * EXT_PER_BLOCK, (j + 1) * EXT_PER_BLOCK);
      buf.writeUInt32LE(blocks[j + 1] || 0, 0); buf.writeUInt16LE(part.length, 4);
      part.forEach((x, i) => { buf.writeUInt32LE(x.start, 8 + i * 6); buf.writeUInt16LE(x.len, 12 + i * 6); });
      this.writeBlock(b, buf);
    });
    if (blocks.length) e.extBlock = blocks[0];
  }
  // The block holding byte `pos` of a file
  fileBlock(e, pos, list = this.extents(e)) {
    let c = Math.floor(pos / CLUSTER);
    for (const x of list) {
      if (c < x.len) return this.clusterBlock(x.start + c) + Math.floor((pos % CLUSTER) / BLOCK);
      c -= x.len;
    }
    throw new Error('offset ' + pos + ' is past the file\'s clusters');
  }

  read(e, pos = 0, len = e.size - pos) {
    const out = Buffer.alloc(Math.max(0, Math.min(len, e.size - pos))), list = this.extents(e);
    for (let done = 0; done < out.length; ) {
      const p = pos + done, off = p % BLOCK, n = Math.min(BLOCK - off, out.length - done);
      this.readBlock(this.fileBlock(e, p, list)).copy(out, done, off, off + n);
      done += n;
    }
    return out;
  }
  // Write data at pos (growing the file: its last extent if the next cluster is free, else new extents)
  write(e, pos, data) {
    const end = pos + data.length, list = this.extents(e);
    let have = list.reduce((s, x) => s + x.len, 0), need = Math.ceil(end / CLUSTER);
    while (have < need) {
      const last = list[list.length - 1];
      const got = this.alloc(need - have, last ? last.start + last.len : -1);
      if (last && got.start === last.start + last.len && last.len + got.len <= EXT_MAX) last.len += got.len;
      else list.push(got);
      have += got.len;
    }
    this.setExtents(e, list);
    for (let done = 0; done < data.length; ) {
      const p = pos + done, off = p % BLOCK, n = Math.min(BLOCK - off, data.length - done);
      const b = this.fileBlock(e, p, list), buf = n === BLOCK ? Buffer.alloc(BLOCK) : this.readBlock(b);
      data.copy(buf, off, done, done + n);
      this.writeBlock(b, buf);
      done += n;
    }
    if (end > e.size) e.size = end;
    this.touch(e);
    this.writeEntry(e);
  }
  truncate(e) {
    for (const x of this.extents(e)) this.release(x.start, x.len);
    this.setExtents(e, []);
    e.size = 0;
    this.touch(e);
    this.writeEntry(e);
  }

  // Directories: arrays of 64-byte entries
  entries(dir) {
    const list = [], ext = this.extents(dir);
    for (let pos = 0; pos < dir.size; pos += ENTRY) {
      const b = this.fileBlock(dir, pos, ext), off = pos % BLOCK;
      list.push(new Entry(this.readBlock(b).subarray(off, off + ENTRY), { block: b, off }));
    }
    return list;
  }
  lookup(dir, name) { return this.entries(dir).find(e => !e.free && e.name === name); }
  walk(p) {
    let e = this.root;
    for (const name of parts(p)) {
      if (!e.isDir) throw new Error(e.name + ' isn\'t a directory');
      e = this.lookup(e, name);
      if (!e) throw new Error('not found: ' + p);
    }
    return e;
  }
  // A new entry in a directory (a free one, or at its end)
  create(dir, name, mode) {
    if (!name.length || name.length > NAME_MAX || name.includes('/')) throw new Error('bad name: "' + name + '"');
    if (this.lookup(dir, name)) throw new Error('there\'s a ' + name + ' already');
    let e = this.entries(dir).find(x => x.free);
    if (!e) {
      const pos = dir.size;
      this.write(dir, pos, Buffer.alloc(ENTRY));
      const b = this.fileBlock(dir, pos);
      e = new Entry(Buffer.alloc(ENTRY), { block: b, off: pos % BLOCK });
    } else this.touch(dir), this.writeEntry(dir);
    e.buf.fill(0);
    e.name = name; e.mode = mode; e.qid = this.nextQid(); e.stamp = this.nextStamp();
    this.writeEntry(e);
    return e;
  }
  remove(p) {
    const ps = parts(p);
    if (!ps.length) throw new Error('the root can\'t be removed');
    const dir = this.walk(ps.slice(0, -1).join('/')), e = this.lookup(dir, ps[ps.length - 1]);
    if (!e) throw new Error('not found: ' + p);
    if (e.isDir && this.entries(e).some(x => !x.free)) throw new Error(p + ' isn\'t empty');
    this.truncate(e);
    e.buf.fill(0);
    this.writeEntry(e);
    this.touch(dir); this.writeEntry(dir);
  }
  mkdir(p) {
    let e = this.root;
    for (const name of parts(p)) {
      const next = this.lookup(e, name);
      if (next && !next.isDir) throw new Error(name + ' isn\'t a directory');
      e = next || this.create(e, name, MODE_DIR);
    }
    return e;
  }
  // Put data in a file (made, or emptied if it's there): p is its path, or a directory to put `name` in
  put(p, data, name) {
    let dir, file;
    const target = this.tryWalk(p);
    if (target && target.isDir) { dir = target; file = name; }
    else { const ps = parts(p); dir = this.walk(ps.slice(0, -1).join('/')); file = ps[ps.length - 1]; }
    let e = this.lookup(dir, file);
    if (e && e.isDir) throw new Error(file + ' is a directory');
    if (e) this.truncate(e); else e = this.create(dir, file, 0);
    if (data.length) this.write(e, 0, data);
    return e;
  }
  tryWalk(p) { try { return this.walk(p); } catch { return null; } }
  // A directory's text listing, as the Hydra's text directory read gives it
  list(e) {
    const line = x => x.isDir ? x.name + '/' : x.name + ' ' + x.size;
    return (e.isDir ? this.entries(e).filter(x => !x.free).map(line) : [line(e)]).join('\r\n') + '\r\n';
  }
  // Check the free map against the files: every cluster in use is in the map once, and nothing else is
  check() {
    const owner = new Map(), problems = [];
    const claim = (c, who) => {
      if (c >= this.clusters) problems.push(who + ': cluster ' + c + ' is past the end');
      else if (owner.has(c)) problems.push(who + ': cluster ' + c + ' is used by ' + owner.get(c) + ' too');
      else owner.set(c, who);
    };
    const visit = (e, p) => {
      for (const x of this.extents(e)) for (let c = x.start; c < x.start + x.len; c++) claim(c, p);
      for (const b of this.extentBlocks(e)) claim((b - this.dataStart) / CLUSTER_BLOCKS, p + ' (extents)');
      const clusters = this.extents(e).reduce((s, x) => s + x.len, 0);
      if (clusters * CLUSTER < e.size) problems.push(p + ': ' + e.size + ' bytes in ' + clusters + ' clusters');
      if (e.isDir) for (const x of this.entries(e)) if (!x.free) visit(x, (p === '/' ? '' : p) + '/' + x.name);
    };
    visit(this.root, '/');
    let free = 0;
    for (let c = 0; c < this.clusters; c++) {
      const u = this.used(c);
      if (!u) free++;
      if (u && !owner.has(c)) problems.push('cluster ' + c + ' is marked in use, but nothing uses it (lost)');
      if (!u && owner.has(c)) problems.push('cluster ' + c + ' is used by ' + owner.get(c) + ', but marked free');
    }
    if (free !== this.freeCount) problems.push('the superblock says ' + this.freeCount + ' clusters are free; ' + free + ' are');
    return problems;
  }
}

// A card path's names.  (Git Bash turns an argument like /games into C:/Program Files/Git/games: refuse
// those, rather than making directories called C: and Program Files.)
function parts(p) {
  if (/^[A-Za-z]:[\\/]/.test(p)) throw new Error('"' + p + '" looks like a PC path: in Git Bash, leave out the card path\'s first / (games/star.frt), or set MSYS_NO_PATHCONV=1');
  return String(p).split('/').filter(s => s && s !== '.');
}

// ---- the command line
function main(argv) {
  const [cmd, image, ...args] = argv;
  const usage = () => { console.error(fs.readFileSync(__filename, 'utf8').split('\n').slice(4, 18).map(l => l.slice(3)).join('\n')); process.exit(1); };
  if (!cmd || !image) usage();
  if (cmd === 'mkfs') { if (!(+args[0] > 0)) usage(); mkfs(image, +args[0], args[1] || ''); return; }
  const v = new Volume(image);
  try {
    switch (cmd) {
      case 'info':
        console.log('label "' + v.label + '", ' + v.clusters + ' clusters of ' + CLUSTER + ' bytes (' +
          (v.clusters * CLUSTER / 1048576).toFixed(1) + ' MB), ' + v.freeCount + ' free (' + (v.freeCount * CLUSTER / 1048576).toFixed(1) + ' MB)');
        break;
      case 'ls': process.stdout.write(v.list(v.walk(args[0] || '/'))); break;
      case 'put': if (args.length < 2) usage(); v.put(args[1], fs.readFileSync(args[0]), path.basename(args[0])); break;
      case 'get': { if (args.length < 2) usage(); const e = v.walk(args[0]); if (e.isDir) throw new Error(args[0] + ' is a directory');
        fs.writeFileSync(args[1], v.read(e)); break; }
      case 'mkdir': if (!args[0]) usage(); v.mkdir(args[0]); break;
      case 'rm': if (!args[0]) usage(); v.remove(args[0]); break;
      case 'import': {
        if (!args[0]) usage();
        const copy = (from, to) => {
          v.mkdir(to);
          for (const d of fs.readdirSync(from, { withFileTypes: true })) {
            const src = path.join(from, d.name), dst = to.replace(/\/$/, '') + '/' + d.name;
            if (d.isDirectory()) copy(src, dst); else if (d.isFile()) v.put(dst, fs.readFileSync(src));
          }
        };
        copy(args[0], args[1] || '/');
        break;
      }
      case 'check': { const p = v.check(); console.log(p.length ? p.join('\n') : 'ok'); if (p.length) process.exitCode = 1; break; }
      default: usage();
    }
  } finally { v.close(); }
}

if (require.main === module) {
  try { main(process.argv.slice(2)); } catch (e) { console.error('hydrafs: ' + e.message); process.exit(1); }
}
module.exports = { mkfs, Volume, MODE_DIR, MODE_APPEND, MODE_RO };
