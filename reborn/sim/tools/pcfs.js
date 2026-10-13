// pcfs.js - /pc's file server: a folder on the PC, or a HydraFS image file (a card's image: hydrafs.js), served to
// the Hydra, a request at a time, as its frames carry them (docs/design/plans/PC.md; the frames: lib/pcproto.js).
// The emulators (hydrasim.js --pc-dir, reborn/sim/run.js --pc-dir) and the PC tool (hydrapc.js) all use it.  It
// speaks both versions of what the frames carry, the attach naming the Hydra's:
//   1  the old system's H9 requests, answered the way its HydraFS does (old/docs/programming/io.md, "The files on a
//      card"): a directory reads as "name size" lines (or "name/"), or as 48-byte stat records when opened with
//      IO_MODE_STAT; IO_CREATE of a file that's there empties it; IO_REMOVE takes a file or an empty directory;
//      IO_WSTAT renames in the directory and sets the read-only bit.
//   2  reborn's requests (its request block, appendix B of docs/design/reimplementation-from-scratch.md), answered the way
//      reborn's HydraFS (#f) does: a directory reads as 64-byte stat records (SR_*), whole ones from a record's
//      start; R_CREATE of a file that's there empties it, and with DM_DIR makes a directory; R_REMOVE takes a file
//      or an empty directory; R_WSTAT renames in the directory (a name whose first byte isn't 0), sets a file's
//      length ($FFFFFFFF keeps it) and the mode (no w bits: read-only; $FFFF keeps it); R_DUP is the same fid again; the errors are reborn's
//      (base/spec/errors.def).
// What's served is a store (below): the requests are done the same way on either kind.
//   A folder: nothing outside it is reached (a name's elements may not be "." or "..", or hold a '\' or a ':').  A
//     file's id is its inode number, its stamp the PC's time for it, read-only its having no write permission.
//   An image: the HydraFS in the file (or in its partition, as a card's has it), its root /pc.  The names, ids,
//     stamps and read-only bits are the card's own, and a directory lists in the card's order, as the Hydra would
//     show the card itself.  The image is whole on the PC after every request that changed it (its free map and
//     superblock written then), so it can be copied, or put on a card, whenever the Hydra isn't writing.  Don't give
//     the same image to an emulator as a card (--sd) while it's served: each would write over the other's work.
//
// createPcFs({ root, readOnly, log, now }) gives { request(tag, payload) -> reply payload, attach(payload) -> reply
// payload, close(), stats: { requests, repeats, attaches }, kind: 'folder' or 'image', what: a line about it, readOnly }.  root: the
// folder, or the image file.  now: an image's stamps for what's written (seconds since 2000: a test's; else the
// PC's time).  A request repeated with the tag just answered (its reply was lost: the Hydra asks again) gets the
// same reply, not done twice.
'use strict';
const fs = require('fs');
const path = require('path');
const { REQ_HDR, REQ_HDR2 } = require('../lib/pcproto.js');
const { Volume, MODE_DIR, MODE_RO } = require('./hydrafs.js');

const H9_OPEN = 1, H9_READ = 2, H9_WRITE = 3, H9_CLUNK = 4, H9_STAT = 5, H9_CTL = 6, H9_DUP = 7;
const H9_CREATE = 8, H9_REMOVE = 9, H9_WSTAT = 10;
const MODE_WRITE = 0x02, MODE_STAT = 0x04, MODE_TRUNC = 0x08;
const M_DIR = 0x80, M_RO = 0x01;
// The errors, by name: version 1's codes (the old system's), and version 2's (reborn's)
const ERR = { NOT_FOUND: 0x70, BAD_FD: 0x71, MODE: 0x72, NO_FDS: 0x75, NAME: 0x77, BAD_REQ: 0x78, DEVICE: 0x79,
  FULL: 0x81, EXISTS: 0x82, NOT_EMPTY: 0x83, BUSY: 0x84, NOT_DIR: 0x85, IS_DIR: 0x86, PERM: 0x88, READ_ONLY: 0x88 };
const ERR2 = { NOT_FOUND: 0x20, BAD_FD: 0x25, MODE: 0x23, NO_FDS: 0x27, NAME: 0x02, BAD_REQ: 0x03, DEVICE: 0x2A,
  FULL: 0x28, EXISTS: 0x21, NOT_EMPTY: 0x24, BUSY: 0x07, NOT_DIR: 0x22, IS_DIR: 0x23, PERM: 0x01, READ_ONLY: 0x29,
  TOO_LONG: 0x0A };
const STAT_SIZE = 48, NAME_MAX = 31, DISK_PC = 11;           // (The stat record's disk byte: /pc's)
const EPOCH_2000 = 946684800;
// Version 2's requests and their fields (base/spec/api.def)
const R_OPEN = 1, R_CREATE = 2, R_READ = 3, R_WRITE = 4, R_CLUNK = 5, R_STAT = 6, R_WSTAT = 7, R_REMOVE = 8, R_DUP = 10;
const O_RW_MASK = 3, O_READ = 0, O_TRUNC = 0x10;
const SR_SIZE = 64, QT_DIR = 0x80, DM_DIR = 0x80, DEV_PC = 0x50;  // (The stat record's device letter: P)

// A Node.js error as the Hydra's (by name); version 1 has no "not a directory" for a name that walks through a file
function errName(e, version) {
  if (typeof e === 'string') return e;
  switch (e && e.code) {
    case 'ENOENT': return 'NOT_FOUND';
    case 'ENOTDIR': return version === 2 ? 'NOT_DIR' : 'NOT_FOUND';
    case 'EEXIST': return 'EXISTS';
    case 'ENOTEMPTY': return 'NOT_EMPTY';
    case 'EBUSY': return 'BUSY';
    case 'EISDIR': return 'IS_DIR';
    case 'EACCES': case 'EPERM': return 'PERM';
    case 'ENOSPC': return 'FULL';
    case 'ENAMETOOLONG': return version === 2 ? 'TOO_LONG' : 'NAME';
    case 'EINVAL': return 'NAME';
  }
  return 'DEVICE';
}

// ---- The stores.  A store names each file by a key of its own (the root's is store.root), and gives:
//   where(parts) -> key    the Hydra's name's elements, from the root (it throws 'NAME' for one it won't have)
//   rel(key), base(key), sibling(key, name) -> its name from the root, its last element, a key beside it
//   stat(key) -> { dir, size, ro, id, ver, stamp }   (or it throws: a Node.js error, or one with its code)
//   list(key) -> [{ name, file: key, st }]   a directory's files, in the order they're shown
//   exists(key); mkdir(key); mkfile(key) (a file made, or emptied); remove(key, dir); rename(key, toKey);
//   truncate(key, length); setReadOnly(key, ro)
//   open(key, write, trunc) -> handle; read(key, handle, count, offset) -> Buffer; write(key, handle, data, offset)
//   -> the bytes written; close(handle)
//   sync()   after each request: what it changed is on the PC's disk as a whole;  done()   the store closed
//   enc      its names' bytes, as the Hydra sends and shows them: 'utf8' (the PC's names), 'latin1' (a card's)

// A folder on the PC
function folderStore(root) {
  const rel = file => path.relative(root, file).split(path.sep).join('/');
  // A file's id (its inode number, or a hash of its name), and its stamp (seconds since 2000, local time: the
  // Hydra's clock keeps local time)
  function fileId(file, st) {
    let id = Number(BigInt.asUintN(32, BigInt(st.ino || 0)));
    if (!id) for (const c of Buffer.from(rel(file))) id = (Math.imul(id, 31) + c) >>> 0;   // (No inode numbers here)
    return id;
  }
  function stamp(st) {
    const d = new Date(st.mtimeMs);
    const s = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate(), d.getHours(), d.getMinutes(), d.getSeconds()) / 1000 - EPOCH_2000;
    return Math.max(0, Math.min(s, 0xFFFFFFFF));
  }
  const info = (file, st) => { const s = stamp(st); return { dir: st.isDirectory(), size: st.size, ro: !(st.mode & 0o200), id: fileId(file, st), ver: s & 0xFF, stamp: s }; };
  return {
    kind: 'folder', root, what: root, enc: 'utf8',
    where(parts) {
      for (const p of parts) if (p === '.' || p === '..' || /[\\:]/.test(p)) throw 'NAME';
      return path.join(root, ...parts);
    },
    rel,
    base: file => path.basename(file),
    sibling: (file, name) => path.join(path.dirname(file), name),
    stat(file) {
      const st = fs.statSync(file);
      if (!st.isDirectory() && !st.isFile()) throw 'NOT_FOUND';
      return info(file, st);
    },
    list(dir) {
      const out = [];
      for (const name of fs.readdirSync(dir).sort()) {
        const file = path.join(dir, name);
        let st;
        try { st = fs.statSync(file); } catch (e) { continue; }   // (A link to nowhere, ...)
        if (st.isDirectory() || st.isFile()) out.push({ name, file, st: info(file, st) });
      }
      return out;
    },
    exists: file => fs.existsSync(file),
    mkdir(file) { fs.mkdirSync(file); },
    mkfile(file) { fs.closeSync(fs.openSync(file, 'w')); },
    remove(file, dir) { if (dir) fs.rmdirSync(file); else fs.unlinkSync(file); },
    rename(file, to) { fs.renameSync(file, to); },
    truncate(file, len) { fs.truncateSync(file, len); },
    setReadOnly(file, ro) { const st = fs.statSync(file); fs.chmodSync(file, ro ? st.mode & ~0o222 : st.mode | 0o200); },
    open(file, write, trunc) {
      const fd = fs.openSync(file, write ? 'r+' : 'r');
      if (write && trunc) fs.ftruncateSync(fd, 0);
      return fd;
    },
    read(file, fd, count, ofs) { const out = Buffer.alloc(count); return out.subarray(0, fs.readSync(fd, out, 0, count, ofs)); },
    write(file, fd, data, ofs) { return fs.writeSync(fd, data, 0, data.length, ofs); },
    close(fd) { try { fs.closeSync(fd); } catch (e) {} },
    sync() {},
    done() {},
  };
}

// A HydraFS image: a key is the file's name from the root ("" the root, "a/b"), walked anew for each thing done
// (so nothing kept here goes stale when another fid changes the file, or its directory grows)
function imageStore(file, readOnly, now) {
  const vol = new Volume(file, { readOnly, now });
  const fail = code => Object.assign(new Error(code), { code });
  const parts = key => key ? key.split('/') : [];
  let dirty = false;
  function walk(key) {
    let e = vol.root;
    for (const name of parts(key)) {
      if (!e.isDir) throw fail('ENOTDIR');
      e = vol.lookup(e, name);
      if (!e) throw fail('ENOENT');
    }
    return e;
  }
  const dirOf = key => { const d = walk(parts(key).slice(0, -1).join('/')); if (!d.isDir) throw fail('ENOTDIR'); return d; };
  const base = key => parts(key).pop() || '';
  const info = e => ({ dir: e.isDir, size: e.isDir ? 0 : e.size, ro: !!(e.mode & MODE_RO), id: e.qid, ver: e.qver & 0xFF, stamp: e.stamp });
  const changed = () => { dirty = true; };
  return {
    kind: 'image', root: '', enc: 'latin1',
    what: file + ' (a HydraFS image' + (vol.label ? ', "' + vol.label + '"' : '') + ': ' + (vol.clusters * vol.cluster / 1048576).toFixed(1) + ' MB, ' +
      (vol.freeCount * vol.cluster / 1048576).toFixed(1) + ' free)',
    where(ps) {
      for (const p of ps) if (p === '.' || p === '..') throw 'NAME';
      return ps.join('/');
    },
    rel: key => key,
    base,
    sibling: (key, name) => [...parts(key).slice(0, -1), name].join('/'),
    stat: key => info(walk(key)),
    list(key) {
      const d = walk(key);
      if (!d.isDir) throw fail('ENOTDIR');
      return vol.entries(d).filter(x => !x.free).map(x => ({ name: x.name, file: key ? key + '/' + x.name : x.name, st: info(x) }));
    },
    exists(key) { try { walk(key); return true; } catch (e) { return false; } },
    mkdir(key) { vol.create(dirOf(key), base(key), MODE_DIR); changed(); },
    mkfile(key) {
      const d = dirOf(key), e = vol.lookup(d, base(key));
      if (e) vol.truncate(e); else vol.create(d, base(key), 0);
      changed();
    },
    remove(key) { vol.remove(key); changed(); },
    rename(key, to) { vol.rename(dirOf(key), walk(key), base(to)); changed(); },
    truncate(key, len) { vol.resize(walk(key), len); changed(); },
    setReadOnly(key, ro) { vol.setReadOnly(walk(key), ro); changed(); },
    open(key, write, trunc) {
      const e = walk(key);
      if (write && (e.mode & MODE_RO)) throw fail('EACCES');
      if (write && trunc) { vol.truncate(e); changed(); }
      return null;
    },
    read: (key, h, count, ofs) => vol.read(walk(key), ofs, count),
    write(key, h, data, ofs) { vol.write(walk(key), ofs, data); changed(); return data.length; },
    close() {},
    sync() { if (dirty) { vol.flush(); dirty = false; } },
    done() { this.sync(); vol.close(); },
  };
}

function createPcFs({ root, readOnly = false, log = () => {}, now }) {
  root = path.resolve(root);
  const rst = fs.statSync(root);
  let store;
  if (rst.isDirectory()) store = folderStore(root);
  else if (rst.isFile()) {
    if (!readOnly) try { fs.closeSync(fs.openSync(root, 'r+')); } catch (e) { readOnly = true; }   // (A file that can't be written: served read-only)
    store = imageStore(root, readOnly, now);
  } else throw new Error(root + ' is neither a folder nor an image file');
  const fids = new Map();                                      // fid -> { file, fd, dir, stat, refs }
  let lastTag = -1, lastReply = null, version = 1;
  const stats = { requests: 0, repeats: 0, attaches: 0 };     // (Requests served; asked again, answered from lastReply; attaches)

  // The Hydra's name (the rest after /pc: "" or "/a/b") as the store's key
  const where = name => store.where(name.split('/').filter(p => p.length));
  const rel = file => store.rel(file);

  // A file's 48-byte stat record (version 1: io.md, "Stat")
  function statRecord(file, st) {
    const b = Buffer.alloc(STAT_SIZE);
    const name = file === store.root ? '' : store.base(file);
    Buffer.from(name, store.enc).copy(b, 0, 0, NAME_MAX);
    b[32] = (st.dir ? M_DIR : 0) | (st.ro ? M_RO : 0);
    b[33] = DISK_PC;
    b.writeUInt32LE(st.id, 36);
    b.writeUInt32LE(Math.min(st.dir ? 0 : st.size, 0xFFFFFFFF), 40);
    b.writeUInt32LE(st.stamp, 44);
    return b;
  }

  // A file's 64-byte stat record (version 2: SR_*): its name ("/" for the root), its qid (its type, its version:
  // the stamp's low byte, or the card's own; its id), its mode (rw-rw-rw-, or r--r--r-- read-only or served so;
  // DM_DIR), its length, its stamp, the device (P)
  function statRecord2(file, st) {
    const b = Buffer.alloc(SR_SIZE);
    Buffer.from(file === store.root ? '/' : store.base(file), store.enc).copy(b, 0, 0, NAME_MAX);
    b[32] = st.dir ? QT_DIR : 0;
    b[33] = st.ver;
    b.writeUInt32LE(st.id, 34);
    b.writeUInt16LE((!st.ro && !readOnly ? 0o666 : 0o444) | (st.dir ? DM_DIR << 8 : 0), 38);
    b.writeUInt32LE(st.dir ? 0 : Math.min(st.size, 0xFFFFFFFF), 40);
    b.writeUInt32LE(st.stamp, 44);
    b[48] = DEV_PC;
    return b;
  }

  // A directory as a read sees it: its lines, or its stat records (version 1's, or version 2's)
  function listing(f) {
    const out = [];
    for (const { name, file, st } of store.list(f.file))
      out.push(version === 2 ? statRecord2(file, st) : f.stat ? statRecord(file, st) : Buffer.from(name + (st.dir ? '/' : ' ' + st.size) + '\r\n', store.enc));
    return Buffer.concat(out);
  }

  function newFid(f) {
    for (let i = 0; i < 256; i++) if (!fids.has(i)) { fids.set(i, Object.assign({ refs: 1 }, f)); return i; }
    throw 'NO_FDS';
  }
  // A file opened: for writing (or not), emptied (or not)
  function openAs(file, write, trunc, created, statMode) {
    const st = store.stat(file);
    if (st.dir) {
      if (write && !created) throw version === 2 ? 'IS_DIR' : 'MODE';
      return newFid({ file, fd: null, dir: true, stat: statMode });
    }
    if (write && readOnly) throw version === 2 ? 'READ_ONLY' : 'PERM';
    const fd = store.open(file, write, trunc);
    return newFid({ file, fd, dir: false, write });
  }
  const openFile = (file, mode, created) => openAs(file, !!(mode & MODE_WRITE), !!(mode & MODE_TRUNC), created, !!(mode & MODE_STAT));
  const openFile2 = (file, mode, created) => openAs(file, (mode & O_RW_MASK) !== O_READ || (created && !store.stat(file).dir),
    !!(mode & O_TRUNC), created, true);
  function fidOf(n) { const f = fids.get(n); if (!f) throw 'BAD_FD'; return f; }
  function closeFid(n, f) { if (!f.dir) store.close(f.fd); fids.delete(n); }
  const nameIn = data => { const z = data.indexOf(0); return Buffer.from(z < 0 ? data : data.subarray(0, z)).toString(store.enc); };
  // (A write on a fid opened for reading: the PC's own refusal for a folder's file, a device error; the same for an image's)
  const writable = f => { if (!f.write) throw 'DEVICE'; };

  // A file or directory made: a directory (dir), or a file (emptied if it's there)
  function make(file, dir) {
    if (readOnly) throw version === 2 ? 'READ_ONLY' : 'PERM';
    if (file === store.root) throw 'EXISTS';
    if (dir) store.mkdir(file);
    else {
      if (store.exists(file) && store.stat(file).dir) throw 'EXISTS';
      store.mkfile(file);
    }
  }
  // A file or an empty directory removed (not one that's open)
  function remove(name) {
    if (readOnly) throw version === 2 ? 'READ_ONLY' : 'PERM';
    const file = where(name);
    if (file === store.root) throw 'PERM';
    for (const f of fids.values()) if (f.file === file) throw 'BUSY';
    store.remove(file, store.stat(file).dir);
    log('remove ' + name);
  }
  // Fid f's file renamed, in its directory
  function rename(f, newName) {
    if (/[\/\\:]/.test(newName) || newName === '.' || newName === '..') throw 'NAME';
    const to = store.sibling(f.file, newName);
    if (to === f.file) return;
    if (store.exists(to)) throw 'EXISTS';
    store.rename(f.file, to);
    log('rename ' + rel(f.file) + ' -> ' + newName);
    const from = f.file;
    for (const g of fids.values()) if (g.file === from) g.file = to;
  }

  // Version 1: a request's block's first 16 bytes, then its data.  The reply: status (0, or the error), a value (the
  // fid for an open), the count, then data
  function serve(p) {
    const type = p[0], fidN = p[1], mode = p[2], ofs = (p[4] | p[5] << 8 | p[6] << 16 | p[7] << 24) >>> 0;
    const count = (p[8] | p[9] << 8) || 256, perm = p[12], data = Buffer.from(p.subarray(REQ_HDR));
    let value = 0, n = 0, out = Buffer.alloc(0);
    switch (type) {
      case H9_OPEN: {
        const name = nameIn(data);
        value = openFile(where(name), mode, false);
        log('open ' + (name || '/') + ' -> ' + value);
        break;
      }
      case H9_CREATE: {
        const name = nameIn(data), file = where(name);
        make(file, perm & M_DIR);
        value = openFile(file, mode | (perm & M_DIR ? 0 : MODE_WRITE), true);
        log('create ' + name + ' -> ' + value);
        break;
      }
      case H9_REMOVE: remove(nameIn(data)); break;
      case H9_READ: {
        const f = fidOf(fidN);
        if (f.dir) { const l = listing(f); out = l.subarray(Math.min(ofs, l.length), Math.min(ofs + count, l.length)); }
        else out = store.read(f.file, f.fd, count, ofs);
        n = out.length;
        break;
      }
      case H9_WRITE: {
        const f = fidOf(fidN);
        if (f.dir) throw 'MODE';
        writable(f);
        n = store.write(f.file, f.fd, data.subarray(0, Math.min(count, data.length)), ofs);
        break;
      }
      case H9_STAT: { const f = fidOf(fidN); out = statRecord(f.file, store.stat(f.file)); break; }
      case H9_WSTAT: {
        if (readOnly) throw 'PERM';
        const f = fidOf(fidN);
        if (f.file === store.root) throw 'PERM';
        const newName = nameIn(data.subarray(0, 32)), m = data[32];
        if (newName) rename(f, newName);
        if (m !== 0xFF && m !== undefined && !f.dir) store.setReadOnly(f.file, !!(m & M_RO));
        break;
      }
      case H9_CLUNK: { const f = fids.get(fidN); if (f && --f.refs <= 0) closeFid(fidN, f); break; }
      case H9_DUP: fidOf(fidN).refs++; break;
      case H9_CTL: throw 'BAD_REQ';
      default: throw 'BAD_REQ';
    }
    return [0, value, n & 0xFF, n >> 8, ...out];
  }

  // Version 2: a request's whole block (REQ_HDR2 bytes), then its data: a name (R_OPEN, R_CREATE, R_REMOVE), a
  // write's bytes, a stat record (R_WSTAT).  The reply: status (0, or the error), the fid (R_OPEN's, R_CREATE's,
  // R_DUP's; the request's for the rest), the count done, the qid type, then data (a read's, a stat record)
  function serve2(p) {
    const type = p[0], fidN = p[1], mode = p[2], ofs = (p[6] | p[7] << 8 | p[8] << 16 | p[9] << 24) >>> 0;
    const count = p[10] | p[11] << 8, perm = p[16], data = Buffer.from(p.subarray(REQ_HDR2));
    let fid = fidN, n = 0, out = Buffer.alloc(0);
    switch (type) {
      case R_OPEN: {
        const name = nameIn(data);
        fid = openFile2(where(name), mode, false);
        log('open ' + (name || '/') + ' -> ' + fid);
        break;
      }
      case R_CREATE: {
        const name = nameIn(data), file = where(name);
        make(file, perm & DM_DIR);
        fid = openFile2(file, mode, true);
        log('create ' + name + ' -> ' + fid);
        break;
      }
      case R_REMOVE: remove(nameIn(data)); break;
      case R_READ: {
        const f = fidOf(fidN);
        if (f.dir) {                                           // (Whole records, from a record's start)
          const l = listing(f), at = Math.floor(ofs / SR_SIZE) * SR_SIZE;
          out = l.subarray(Math.min(at, l.length), Math.min(at + Math.floor(count / SR_SIZE) * SR_SIZE, l.length));
        } else out = store.read(f.file, f.fd, count, ofs);
        n = out.length;
        break;
      }
      case R_WRITE: {
        const f = fidOf(fidN);
        if (f.dir) throw 'IS_DIR';
        writable(f);
        n = store.write(f.file, f.fd, data.subarray(0, Math.min(count, data.length)), ofs);
        break;
      }
      case R_STAT: { const f = fidOf(fidN); out = statRecord2(f.file, store.stat(f.file)); n = out.length; break; }
      case R_WSTAT: {
        if (readOnly) throw 'READ_ONLY';
        const f = fidOf(fidN);
        if (f.file === store.root) throw 'PERM';
        const newName = nameIn(data.subarray(0, 32)), m = data.length >= 40 ? data[38] | data[39] << 8 : 0xFFFF;
        if (newName) rename(f, newName);
        const len = data.length >= 44 ? data.readUInt32LE(40) : 0xFFFFFFFF;
        if (len !== 0xFFFFFFFF) {                                               // (A file's length: longer, with zeros, or cut)
          if (f.dir) throw 'IS_DIR';
          store.truncate(f.file, len);
          log('length ' + rel(f.file) + ' ' + len);
        }
        if (m !== 0xFFFF && !f.dir) store.setReadOnly(f.file, !(m & 0o222));
        break;
      }
      case R_CLUNK: { const f = fids.get(fidN); if (f && --f.refs <= 0) closeFid(fidN, f); break; }
      case R_DUP: fidOf(fidN).refs++; fid = fidN; break;
      default: throw 'BAD_REQ';
    }
    const f = fids.get(fid);
    return [0, fid, n & 0xFF, n >> 8, (type === R_OPEN || type === R_CREATE || type === R_DUP) && f && f.dir ? QT_DIR : 0, ...out];
  }

  return {
    request(tag, payload) {
      if (tag === lastTag && lastReply) { stats.repeats++; log('asked again (tag ' + tag + '): my reply was lost on the way'); return lastReply; }
      stats.requests++;
      let reply;
      try { reply = version === 2 ? serve2(payload) : serve(payload); }
      catch (e) {
        const name = errName(e, version);
        reply = version === 2 ? [ERR2[name], 0, 0, 0, 0] : [ERR[name] || ERR.DEVICE, 0, 0, 0];
        if (typeof e !== 'string') log('error: ' + e.message);
      }
      try { store.sync(); } catch (e) { log('error: ' + e.message); }
      lastTag = tag; lastReply = reply;
      return reply;
    },
    // A new session: the Hydra has no fids of ours.  Its payload: the Hydra's version (none: 1).  The reply: 0, 1 if
    // the folder is served read-only, and the count 0; version 2's has the version, 2, where the qid type goes
    attach(payload) {
      for (const [n, f] of fids) closeFid(n, f);
      lastTag = -1; lastReply = null; stats.attaches++;
      version = payload && payload.length && payload[0] === 2 ? 2 : 1;
      log('attach (protocol ' + version + ')');
      return version === 2 ? [0, readOnly ? 1 : 0, 0, 0, 2] : [0, readOnly ? 1 : 0, 0, 0];
    },
    close() { for (const [n, f] of fids) closeFid(n, f); store.done(); },
    stats, kind: store.kind, what: store.what, readOnly,
  };
}

module.exports = { createPcFs, ERR, ERR2 };
