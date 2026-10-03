// pcfs.js - /pc's file server: a folder on the PC served to the Hydra, a request at a time, as its frames carry
// them (docs/plans/PC.md; the frames: lib/pcproto.js).  The emulators (hydrasim.js --pc-dir, reborn/sim/run.js
// --pc-dir) and the PC tool (hydrapc.js) all use it.  It speaks both versions of what the frames carry, the attach
// naming the Hydra's:
//   1  the old system's H9 requests, answered the way its HydraFS does (docs/programming/io.md, "The files on a
//      card"): a directory reads as "name size" lines (or "name/"), or as 48-byte stat records when opened with
//      IO_MODE_STAT; IO_CREATE of a file that's there empties it; IO_REMOVE takes a file or an empty directory;
//      IO_WSTAT renames in the directory and sets the read-only bit.
//   2  reborn's requests (its request block, appendix B of docs/reimplementation-from-scratch.md), answered the way
//      reborn's HydraFS (#f) does: a directory reads as 64-byte stat records (SR_*), whole ones from a record's
//      start; R_CREATE of a file that's there empties it, and with DM_DIR makes a directory; R_REMOVE takes a file
//      or an empty directory; R_WSTAT renames in the directory (a name whose first byte isn't 0) and sets the mode
//      (no w bits: read-only; $FFFF keeps it); R_DUP is the same fid again; the errors are reborn's
//      (reborn/spec/errors.def).
// Nothing outside the folder is reached: a name's elements may not be "." or "..", or hold a '\' or a ':'.
//
// createPcFs({ root, readOnly, log }) gives { request(tag, payload) -> reply payload, attach(payload) -> reply
// payload, close(), stats: { repeats } }.  A request repeated with the tag just answered (its reply was lost: the
// Hydra asks again) gets the same reply, not done twice.
'use strict';
const fs = require('fs');
const path = require('path');
const { REQ_HDR, REQ_HDR2 } = require('../lib/pcproto.js');

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
// Version 2's requests and their fields (reborn/spec/api.def)
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

function createPcFs({ root, readOnly = false, log = () => {} }) {
  root = path.resolve(root);
  if (!fs.statSync(root).isDirectory()) throw new Error(root + ' is not a folder');
  const fids = new Map();                                      // fid -> { file, fd, dir, stat, refs }
  let lastTag = -1, lastReply = null, version = 1;
  const stats = { repeats: 0 };                                // (Requests asked again, answered from lastReply)

  // The Hydra's name (the rest after /pc: "" or "/a/b") as a path in the folder
  function where(name) {
    const parts = name.split('/').filter(p => p.length);
    for (const p of parts) if (p === '.' || p === '..' || /[\\:]/.test(p)) throw 'NAME';
    return path.join(root, ...parts);
  }
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

  // A file's 48-byte stat record (version 1: io.md, "Stat")
  function statRecord(file, st) {
    const b = Buffer.alloc(STAT_SIZE);
    const name = file === root ? '' : path.basename(file);
    Buffer.from(name, 'utf8').copy(b, 0, 0, NAME_MAX);
    b[32] = (st.isDirectory() ? M_DIR : 0) | (st.mode & 0o200 ? 0 : M_RO);
    b[33] = DISK_PC;
    b.writeUInt32LE(fileId(file, st), 36);
    b.writeUInt32LE(Math.min(st.isDirectory() ? 0 : st.size, 0xFFFFFFFF), 40);
    b.writeUInt32LE(stamp(st), 44);
    return b;
  }

  // A file's 64-byte stat record (version 2: SR_*): its name ("/" for the folder), its qid (its type, its version:
  // the stamp's low byte, its id), its mode (rw-rw-rw-, or r--r--r-- read-only or served so; DM_DIR), its length,
  // its stamp, the device (P)
  function statRecord2(file, st) {
    const b = Buffer.alloc(SR_SIZE), dir = st.isDirectory(), s = stamp(st);
    Buffer.from(file === root ? '/' : path.basename(file), 'utf8').copy(b, 0, 0, NAME_MAX);
    b[32] = dir ? QT_DIR : 0;
    b[33] = s & 0xFF;
    b.writeUInt32LE(fileId(file, st), 34);
    b.writeUInt16LE((st.mode & 0o200 && !readOnly ? 0o666 : 0o444) | (dir ? DM_DIR << 8 : 0), 38);
    b.writeUInt32LE(dir ? 0 : Math.min(st.size, 0xFFFFFFFF), 40);
    b.writeUInt32LE(s, 44);
    b[48] = DEV_PC;
    return b;
  }

  // A directory as a read sees it: its lines, or its stat records (version 1's, or version 2's)
  function listing(f) {
    const names = fs.readdirSync(f.file).sort();
    const out = [];
    for (const n of names) {
      let st;
      try { st = fs.statSync(path.join(f.file, n)); } catch (e) { continue; }   // (A link to nowhere, ...)
      if (!st.isDirectory() && !st.isFile()) continue;
      const file = path.join(f.file, n);
      out.push(version === 2 ? statRecord2(file, st) : f.stat ? statRecord(file, st) : Buffer.from(n + (st.isDirectory() ? '/' : ' ' + st.size) + '\r\n', 'utf8'));
    }
    return Buffer.concat(out);
  }

  function newFid(f) {
    for (let i = 0; i < 256; i++) if (!fids.has(i)) { fids.set(i, Object.assign({ refs: 1 }, f)); return i; }
    throw 'NO_FDS';
  }
  // A file opened: for writing (or not), emptied (or not)
  function openAs(file, write, trunc, created, statMode) {
    const st = fs.statSync(file);
    if (st.isDirectory()) {
      if (write && !created) throw version === 2 ? 'IS_DIR' : 'MODE';
      return newFid({ file, fd: null, dir: true, stat: statMode });
    }
    if (!st.isFile()) throw 'NOT_FOUND';
    if (write && readOnly) throw version === 2 ? 'READ_ONLY' : 'PERM';
    const fd = fs.openSync(file, write ? 'r+' : 'r');
    if (write && trunc) fs.ftruncateSync(fd, 0);
    return newFid({ file, fd, dir: false });
  }
  const openFile = (file, mode, created) => openAs(file, !!(mode & MODE_WRITE), !!(mode & MODE_TRUNC), created, !!(mode & MODE_STAT));
  const openFile2 = (file, mode, created) => openAs(file, (mode & O_RW_MASK) !== O_READ || (created && !fs.statSync(file).isDirectory()),
    !!(mode & O_TRUNC), created, true);
  function fidOf(n) { const f = fids.get(n); if (!f) throw 'BAD_FD'; return f; }
  function closeFid(n, f) { if (f.fd !== null) try { fs.closeSync(f.fd); } catch (e) {} fids.delete(n); }
  const nameIn = data => { const z = data.indexOf(0); return Buffer.from(z < 0 ? data : data.subarray(0, z)).toString('utf8'); };

  // A file or directory made: a directory (dir), or a file (emptied if it's there)
  function make(file, dir) {
    if (readOnly) throw version === 2 ? 'READ_ONLY' : 'PERM';
    if (file === root) throw 'EXISTS';
    if (dir) fs.mkdirSync(file);
    else {
      if (fs.existsSync(file) && fs.statSync(file).isDirectory()) throw 'EXISTS';
      fs.closeSync(fs.openSync(file, 'w'));
    }
  }
  // A file or an empty directory removed (not one that's open)
  function remove(name) {
    if (readOnly) throw version === 2 ? 'READ_ONLY' : 'PERM';
    const file = where(name);
    if (file === root) throw 'PERM';
    for (const f of fids.values()) if (f.file === file) throw 'BUSY';
    if (fs.statSync(file).isDirectory()) fs.rmdirSync(file); else fs.unlinkSync(file);
    log('remove ' + name);
  }
  // Fid f's file renamed, in its directory
  function rename(f, newName) {
    if (/[\/\\:]/.test(newName) || newName === '.' || newName === '..') throw 'NAME';
    const to = path.join(path.dirname(f.file), newName);
    if (to === f.file) return;
    if (fs.existsSync(to)) throw 'EXISTS';
    fs.renameSync(f.file, to);
    log('rename ' + rel(f.file) + ' -> ' + newName);
    const from = f.file;
    for (const g of fids.values()) if (g.file === from) g.file = to;
  }
  // Fid f's file read-only, or not
  function setReadOnly(f, ro) {
    const st = fs.statSync(f.file);
    fs.chmodSync(f.file, ro ? st.mode & ~0o222 : st.mode | 0o200);
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
        else { out = Buffer.alloc(count); out = out.subarray(0, fs.readSync(f.fd, out, 0, count, ofs)); }
        n = out.length;
        break;
      }
      case H9_WRITE: {
        const f = fidOf(fidN);
        if (f.dir || f.fd === null) throw 'MODE';
        n = fs.writeSync(f.fd, data, 0, Math.min(count, data.length), ofs);
        break;
      }
      case H9_STAT: { const f = fidOf(fidN); out = statRecord(f.file, fs.statSync(f.file)); break; }
      case H9_WSTAT: {
        if (readOnly) throw 'PERM';
        const f = fidOf(fidN);
        if (f.file === root) throw 'PERM';
        const newName = nameIn(data.subarray(0, 32)), m = data[32];
        if (newName) rename(f, newName);
        if (m !== 0xFF && m !== undefined && !f.dir) setReadOnly(f, m & M_RO);
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
        } else { out = Buffer.alloc(count); out = out.subarray(0, fs.readSync(f.fd, out, 0, count, ofs)); }
        n = out.length;
        break;
      }
      case R_WRITE: {
        const f = fidOf(fidN);
        if (f.dir || f.fd === null) throw 'IS_DIR';
        n = fs.writeSync(f.fd, data, 0, Math.min(count, data.length), ofs);
        break;
      }
      case R_STAT: { const f = fidOf(fidN); out = statRecord2(f.file, fs.statSync(f.file)); n = out.length; break; }
      case R_WSTAT: {
        if (readOnly) throw 'READ_ONLY';
        const f = fidOf(fidN);
        if (f.file === root) throw 'PERM';
        const newName = nameIn(data.subarray(0, 32)), m = data.length >= 40 ? data[38] | data[39] << 8 : 0xFFFF;
        if (newName) rename(f, newName);
        if (m !== 0xFFFF && !f.dir) setReadOnly(f, !(m & 0o222));
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
      if (tag === lastTag && lastReply) { stats.repeats++; return lastReply; }   // (Asked again: its reply was lost)
      let reply;
      try { reply = version === 2 ? serve2(payload) : serve(payload); }
      catch (e) {
        const name = errName(e, version);
        reply = version === 2 ? [ERR2[name], 0, 0, 0, 0] : [ERR[name] || ERR.DEVICE, 0, 0, 0];
        if (typeof e !== 'string') log('error: ' + e.message);
      }
      lastTag = tag; lastReply = reply;
      return reply;
    },
    // A new session: the Hydra has no fids of ours.  Its payload: the Hydra's version (none: 1).  The reply: 0, 1 if
    // the folder is served read-only, and the count 0; version 2's has the version, 2, where the qid type goes
    attach(payload) {
      for (const [n, f] of fids) closeFid(n, f);
      lastTag = -1; lastReply = null;
      version = payload && payload.length && payload[0] === 2 ? 2 : 1;
      log('attach (protocol ' + version + ')');
      return version === 2 ? [0, readOnly ? 1 : 0, 0, 0, 2] : [0, readOnly ? 1 : 0, 0, 0];
    },
    close() { for (const [n, f] of fids) closeFid(n, f); },
    stats,
  };
}

module.exports = { createPcFs, ERR, ERR2 };
