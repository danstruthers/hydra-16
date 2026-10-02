// pcfs.js - /pc's file server: a folder on the PC served to the Hydra, a request at a time, as its frames carry
// them (docs/plans/PC.md; the frames: lib/pcproto.js).  The emulator (hydrasim.js --pc-dir) and the PC tool
// (hydrapc.js) both use it.  It answers the H9 requests the way the Hydra's own HydraFS does (docs/programming/
// io.md, "The files on a card"): a directory reads as "name size" lines (or "name/"), or as 48-byte stat records
// when opened with IO_MODE_STAT; IO_CREATE of a file that's there empties it; IO_REMOVE takes a file or an empty
// directory; IO_WSTAT renames in the directory and sets the read-only bit.  Nothing outside the folder is reached:
// a name's elements may not be "." or "..", or hold a '\' or a ':'.
//
// createPcFs({ root, readOnly, log }) gives { request(tag, payload) -> reply payload, attach() -> reply payload,
// close(), stats: { repeats } }.  A request repeated with the tag just answered (its reply was lost: the Hydra asks
// again) gets the same reply, not done twice.
'use strict';
const fs = require('fs');
const path = require('path');
const { REQ_HDR } = require('../lib/pcproto.js');

const H9_OPEN = 1, H9_READ = 2, H9_WRITE = 3, H9_CLUNK = 4, H9_STAT = 5, H9_CTL = 6, H9_DUP = 7;
const H9_CREATE = 8, H9_REMOVE = 9, H9_WSTAT = 10;
const MODE_WRITE = 0x02, MODE_STAT = 0x04, MODE_TRUNC = 0x08;
const M_DIR = 0x80, M_RO = 0x01;
const ERR = { NOT_FOUND: 0x70, BAD_FD: 0x71, MODE: 0x72, NO_FDS: 0x75, NAME: 0x77, BAD_REQ: 0x78, DEVICE: 0x79,
  FULL: 0x81, EXISTS: 0x82, NOT_EMPTY: 0x83, BUSY: 0x84, NOT_DIR: 0x85, IS_DIR: 0x86, PERM: 0x88 };
const STAT_SIZE = 48, NAME_MAX = 31, DISK_PC = 11;           // (The stat record's disk byte: /pc's)
const EPOCH_2000 = 946684800;

// A Node.js error as the Hydra's
function errCode(e) {
  if (typeof e === 'number') return e;
  switch (e && e.code) {
    case 'ENOENT': case 'ENOTDIR': return ERR.NOT_FOUND;
    case 'EEXIST': return ERR.EXISTS;
    case 'ENOTEMPTY': return ERR.NOT_EMPTY;
    case 'EBUSY': return ERR.BUSY;
    case 'EISDIR': return ERR.IS_DIR;
    case 'EACCES': case 'EPERM': return ERR.PERM;
    case 'ENOSPC': return ERR.FULL;
    case 'ENAMETOOLONG': case 'EINVAL': return ERR.NAME;
  }
  return ERR.DEVICE;
}

function createPcFs({ root, readOnly = false, log = () => {} }) {
  root = path.resolve(root);
  if (!fs.statSync(root).isDirectory()) throw new Error(root + ' is not a folder');
  const fids = new Map();                                      // fid -> { file, fd, dir, stat, refs }
  let lastTag = -1, lastReply = null;
  const stats = { repeats: 0 };                                // (Requests asked again, answered from lastReply)

  // The Hydra's name (the rest after /pc: "" or "/a/b") as a path in the folder
  function where(name) {
    const parts = name.split('/').filter(p => p.length);
    for (const p of parts) if (p === '.' || p === '..' || /[\\:]/.test(p)) throw ERR.NAME;
    return path.join(root, ...parts);
  }
  const rel = file => path.relative(root, file).split(path.sep).join('/');

  // A file's 48-byte stat record (io.md, "Stat")
  function statRecord(file, st) {
    const b = Buffer.alloc(STAT_SIZE);
    const name = file === root ? '' : path.basename(file);
    Buffer.from(name, 'utf8').copy(b, 0, 0, NAME_MAX);
    b[32] = (st.isDirectory() ? M_DIR : 0) | (st.mode & 0o200 ? 0 : M_RO);
    b[33] = DISK_PC;
    let id = Number(BigInt.asUintN(32, BigInt(st.ino || 0)));
    if (!id) for (const c of Buffer.from(rel(file))) id = (Math.imul(id, 31) + c) >>> 0;   // (No inode numbers here)
    b.writeUInt32LE(id, 36);
    b.writeUInt32LE(Math.min(st.isDirectory() ? 0 : st.size, 0xFFFFFFFF), 40);
    const d = new Date(st.mtimeMs);                              // (The Hydra's clock keeps local time)
    const s = Date.UTC(d.getFullYear(), d.getMonth(), d.getDate(), d.getHours(), d.getMinutes(), d.getSeconds()) / 1000 - EPOCH_2000;
    b.writeUInt32LE(Math.max(0, Math.min(s, 0xFFFFFFFF)), 44);
    return b;
  }

  // A directory as a read sees it: its lines, or its stat records
  function listing(f) {
    const names = fs.readdirSync(f.file).sort();
    const out = [];
    for (const n of names) {
      let st;
      try { st = fs.statSync(path.join(f.file, n)); } catch (e) { continue; }   // (A link to nowhere, ...)
      if (!st.isDirectory() && !st.isFile()) continue;
      out.push(f.stat ? statRecord(path.join(f.file, n), st) : Buffer.from(n + (st.isDirectory() ? '/' : ' ' + st.size) + '\r\n', 'utf8'));
    }
    return Buffer.concat(out);
  }

  function newFid(f) {
    for (let i = 0; i < 256; i++) if (!fids.has(i)) { fids.set(i, Object.assign({ refs: 1 }, f)); return i; }
    throw ERR.NO_FDS;
  }
  function openFile(file, mode, created) {
    const st = fs.statSync(file);
    if (st.isDirectory()) {
      if ((mode & MODE_WRITE) && !created) throw ERR.MODE;
      return newFid({ file, fd: null, dir: true, stat: !!(mode & MODE_STAT) });
    }
    if (!st.isFile()) throw ERR.NOT_FOUND;
    if ((mode & MODE_WRITE) && readOnly) throw ERR.PERM;
    const fd = fs.openSync(file, mode & MODE_WRITE ? 'r+' : 'r');
    if ((mode & MODE_WRITE) && (mode & MODE_TRUNC)) fs.ftruncateSync(fd, 0);
    return newFid({ file, fd, dir: false });
  }
  function fidOf(n) { const f = fids.get(n); if (!f) throw ERR.BAD_FD; return f; }
  function closeFid(n, f) { if (f.fd !== null) try { fs.closeSync(f.fd); } catch (e) {} fids.delete(n); }
  const nameIn = data => { const z = data.indexOf(0); return Buffer.from(z < 0 ? data : data.subarray(0, z)).toString('utf8'); };

  // A request: its block's first 16 bytes, then its data.  The reply: status (0, or the error), a value (the fid
  // for an open), the count, then data
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
        if (readOnly) throw ERR.PERM;
        const name = nameIn(data), file = where(name);
        if (file === root) throw ERR.EXISTS;
        if (perm & M_DIR) fs.mkdirSync(file);
        else {
          if (fs.existsSync(file) && fs.statSync(file).isDirectory()) throw ERR.EXISTS;
          fs.closeSync(fs.openSync(file, 'w'));
        }
        value = openFile(file, mode | (perm & M_DIR ? 0 : MODE_WRITE), true);
        log('create ' + name + ' -> ' + value);
        break;
      }
      case H9_REMOVE: {
        if (readOnly) throw ERR.PERM;
        const name = nameIn(data), file = where(name);
        if (file === root) throw ERR.PERM;
        for (const f of fids.values()) if (f.file === file) throw ERR.BUSY;
        if (fs.statSync(file).isDirectory()) fs.rmdirSync(file); else fs.unlinkSync(file);
        log('remove ' + name);
        break;
      }
      case H9_READ: {
        const f = fidOf(fidN);
        if (f.dir) { const l = listing(f); out = l.subarray(Math.min(ofs, l.length), Math.min(ofs + count, l.length)); }
        else { out = Buffer.alloc(count); out = out.subarray(0, fs.readSync(f.fd, out, 0, count, ofs)); }
        n = out.length;
        break;
      }
      case H9_WRITE: {
        const f = fidOf(fidN);
        if (f.dir || f.fd === null) throw ERR.MODE;
        n = fs.writeSync(f.fd, data, 0, Math.min(count, data.length), ofs);
        break;
      }
      case H9_STAT: { const f = fidOf(fidN); out = statRecord(f.file, fs.statSync(f.file)); break; }
      case H9_WSTAT: {
        if (readOnly) throw ERR.PERM;
        const f = fidOf(fidN);
        if (f.file === root) throw ERR.PERM;
        const newName = nameIn(data.subarray(0, 32)), m = data[32];
        if (newName) {
          if (/[\/\\:]/.test(newName) || newName === '.' || newName === '..') throw ERR.NAME;
          const to = path.join(path.dirname(f.file), newName);
          if (to !== f.file) {
            if (fs.existsSync(to)) throw ERR.EXISTS;
            fs.renameSync(f.file, to);
            for (const g of fids.values()) if (g.file === f.file) g.file = to;
            log('rename ' + rel(f.file) + ' -> ' + newName);
          }
        }
        if (m !== 0xFF && m !== undefined && !f.dir) {
          const st = fs.statSync(f.file);
          fs.chmodSync(f.file, m & M_RO ? st.mode & ~0o222 : st.mode | 0o200);
        }
        break;
      }
      case H9_CLUNK: { const f = fids.get(fidN); if (f && --f.refs <= 0) closeFid(fidN, f); break; }
      case H9_DUP: fidOf(fidN).refs++; break;
      case H9_CTL: throw ERR.BAD_REQ;
      default: throw ERR.BAD_REQ;
    }
    return [0, value, n & 0xFF, n >> 8, ...out];
  }

  return {
    request(tag, payload) {
      if (tag === lastTag && lastReply) { stats.repeats++; return lastReply; }   // (Asked again: its reply was lost)
      let reply;
      try { reply = serve(payload); } catch (e) { reply = [errCode(e), 0, 0, 0]; if (typeof e !== 'number') log('error: ' + e.message); }
      lastTag = tag; lastReply = reply;
      return reply;
    },
    attach() {                                                     // A new session: the Hydra has no fids of ours
      for (const [n, f] of fids) closeFid(n, f);
      lastTag = -1; lastReply = null;
      log('attach');
      return [0, readOnly ? 1 : 0, 0, 0];
    },
    close() { for (const [n, f] of fids) closeFid(n, f); },
    stats,
  };
}

module.exports = { createPcFs, ERR };
