#!/usr/bin/env node
// ****************************************************************************
// nprof.js - a profiler by stretch (docs/design/plans/NUMSPEED.md, step 0): a program run at rc, its files on a card
// (cd /sd/0), marks the stretches it wants measured by writing k to a byte (BASIC: POKE &H6F, k; hylang: (poke 32767
// k), --mark 7FFF), the kth of the names given (0: none, not counted); each stretch's cycles are counted by where
// they're spent: every module's labels by bank (from ld65's debug file of the module linked again), the kernel's, the
// RAM's.  Each library call (an XCALL) is timed from its call to its return, and inside it by label; and every
// routine the program's task calls is timed inclusive of what it calls (a shadow stack of its JSRs).
//
// Usage: node sim/tools/nprof.js DIR CMD --segs A,B,... [--mark HEX] [--top N] [--entries] [--inside N] [--tree N]
//                                [--local] [--only SEG,...] [--build TREE]
//   DIR          the folder whose files go on the card
//   CMD          the line typed at rc (its first word is the program's module: its marks count, its RAM code's labels)
//   --segs       the stretches' names, in the order of their marks (1, 2 ...)
//   --mark HEX   the byte the program writes its marks to (default 6F: free in BASIC's zero page)
//   --top N      the N labels that took the most (default 25; 0: none)
//   --entries    each library entry: its calls, its cycles a call (inclusive); --inside N: the N labels inside it
//   --tree N     the N routines that took the most, inclusive of what they call
//   --local      cheap local labels (@name) as their own, under their routine's name
//   --only       those stretches alone in the report
//   --build TREE another tree's build (its reborn folder): the default is this one's
// As a module: profile({ tree, files, cmd, segs, mark, labels, entries, inside, tree: n, local }) gives back the
// output and the stretches (a Map: name -> { total, other, prof, ent, inside, tree }).
'use strict';
const fs = require('fs');
const path = require('path');
const os = require('os');
const { execFileSync } = require('child_process');

function ld65() {
  for (const dir of [process.env.CC65_BIN, process.env.CC65_HOME && path.join(process.env.CC65_HOME, 'bin'), 'C:/source/cc65/win64_snapshot/bin']) {
    if (!dir) continue;
    for (const f of [path.join(dir, 'ld65.exe'), path.join(dir, 'ld65')]) if (fs.existsSync(f)) return f;
  }
  return 'ld65';
}

function profile(o) {
  const tree = path.resolve(o.tree || path.join(__dirname, '..', '..'));
  const SEGS = o.segs || [], MARK = o.mark === undefined ? 0x6F : o.mark, LABELS = o.labels !== false;
  const ENTRIES = !!o.entries || !!o.inside, INSIDE = o.inside || 0, TREE = o.routines || 0, LOCAL = !!o.local;
  const MAX = o.max || 60e9;
  const hydrafs = require(path.join(tree, 'sim', 'tools', 'hydrafs.js'));
  const { boot, labels } = require(path.join(tree, 'sim', 'run.js'));
  const romimg = require(path.join(tree, 'tools', 'romimg.js'));
  const romfs = require(path.join(tree, 'tools', 'romfs.js'));
  const { readManifest, hwtest } = require(path.join(tree, 'build.js'));
  const { romBank } = require(path.join(tree, 'sim', 'lib', 'machine.js'));

  // The image: the system's modules and t_rc as init (the tests' image), each module's banks noted
  const bin = (d, n) => fs.readFileSync(path.join(tree, 'obj', d, n + '.bin'));
  const sys = readManifest(path.join(tree, 'modules', 'rom.txt')).modules;
  const built = romimg.build({ modules: [...sys.map(n => ({ file: n, data: bin('modules', n) })), { file: 't_rc', data: bin('tests', 't_rc') }],
    init: 't_rc', hwtest: hwtest(), bios: fs.readFileSync(path.join(tree, 'bin', 'bios.bin')), romfs: romfs.manifest(path.join(tree, 'romfs', 'romfs.txt')) });
  const modOfBank = new Map();
  for (const e of built.entries) for (let k = 0; k < e.banks; k++) modOfBank.set(e.bank + k, [e.name, k]);
  const modAt = b => modOfBank.get(b) || modOfBank.get(romBank(b));

  // Each module's labels by bank, and its RAM code's (its DATA segment's): ld65's debug file of the module linked again
  const LBL = new Map();
  const modLabels = name => {
    if (LBL.has(name)) return LBL.get(name);
    const r = { banks: [], ram: [] };
    LBL.set(name, r);
    const od = path.join(tree, 'obj', 'modules', name), md = path.join(tree, 'modules', name);
    if (!LABELS || !fs.existsSync(od) || !fs.existsSync(md)) return r;
    const objs = fs.readdirSync(od).filter(f => f.endsWith('.o')).map(f => path.join(od, f));
    const srcs = fs.readdirSync(md).filter(f => /\.(s|inc)$/.test(f)).map(f => fs.readFileSync(path.join(md, f), 'latin1'));
    const banks = Math.max(1, ...srcs.map(s => Math.max(0, ...[...s.matchAll(/\.segment\s+"CODE([2-8])"/gi)].map(m => +m[1]))));
    const own = path.join(md, name + '.cfg');
    const cfg = fs.existsSync(own) ? own : path.join(tree, 'modules', banks > 1 ? 'module' + banks + '.cfg' : 'module.cfg');
    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'nprof'));
    let t;
    try {
      execFileSync(ld65(), ['-C', cfg, '-o', path.join(tmp, 'm.bin'), '--dbgfile', path.join(tmp, 'm.dbg'), ...objs], { stdio: 'pipe' });
      t = fs.readFileSync(path.join(tmp, 'm.dbg'), 'latin1').split(/\r?\n/);
    } catch (e) { t = []; }
    fs.rmSync(tmp, { recursive: true, force: true });
    const segs = new Map(), syms = new Map();
    for (const l of t) {
      const m = /^seg\tid=(\d+),name="(\w+)",start=0x([0-9A-F]+)/.exec(l);
      if (m) { const off = /,ooffs=(\d+)/.exec(l); segs.set(+m[1], { name: m[2], bank: off ? Math.floor(+off[1] / 16384) : -1 }); }
    }
    for (const l of t) {
      const m = /^sym\tid=(\d+),name="([^"]+)",.*val=0x([0-9A-F]+),seg=(\d+),type=lab/.exec(l);
      if (m) { const p = /,parent=(\d+),/.exec(l); syms.set(+m[1], { name: m[2], parent: p ? +p[1] : -1, val: parseInt(m[3], 16), seg: +m[4] }); }
    }
    for (const s of syms.values()) {
      const sg = segs.get(s.seg);
      if (!sg) continue;
      let n = s.name;
      if (n.startsWith('@')) { if (!LOCAL) continue; const p = syms.get(s.parent); n = (p ? p.name : '?') + n; }
      if (n.startsWith('__')) continue;
      if (sg.name === 'DATA') r.ram.push([s.val, n]);
      else if (s.val >= 0xA000 && s.val < 0xE000 && sg.bank >= 0) (r.banks[sg.bank] = r.banks[sg.bank] || []).push([s.val, n]);
    }
    for (const L of [r.ram, ...r.banks]) if (L) L.sort((a, b) => a[0] - b[0]);
    return r;
  };
  const near = (L, a) => {
    if (!L) return '?';
    let lo = 0, hi = L.length - 1, r = null;
    while (lo <= hi) { const mid = (lo + hi) >> 1; if (L[mid][0] <= a) { r = L[mid]; lo = mid + 1; } else hi = mid - 1; }
    return r ? r[1] : '?';
  };

  // The card
  const img = path.join(os.tmpdir(), 'nprof-' + process.pid + '-' + Date.now() + '.img');
  hydrafs.setNow(0x1000);
  hydrafs.mkfs(img, 8, 'NPR', undefined, true);
  const v = new hydrafs.Volume(img);
  for (const [n, d] of Object.entries(o.files)) v.put(n, Buffer.isBuffer(d) ? d : Buffer.from(d, 'latin1'));
  v.close();
  const base = fs.readFileSync(img), written = new Map();
  fs.rmSync(img, { force: true });
  const card = { dev: 0, blocks: 16384, file: img,
    read: n => written.get(n) || Buffer.concat([n * 512 < base.length ? base.subarray(n * 512, n * 512 + 512) : Buffer.alloc(0)], 512),
    write: (n, b) => written.set(n, Buffer.from(b)) };

  // The marks: the program's own writes to the byte (its module's code), each a stretch
  const prog = o.cmd.split(' ')[0];
  const segs = new Map(), order = [];
  let seg = null, progTask = -1, m;
  const segOf = n => { if (!segs.has(n)) { segs.set(n, { prof: new Map(), ent: new Map(), inside: new Map(), tree: new Map(), total: 0, other: 0 }); order.push(n); } return segs.get(n); };
  const log = t => {
    const mm = /^watch: \$[0-9A-F]{4} \(task ([0-9A-F])\) [0-9A-F]{2} -> ([0-9A-F]{2})/.exec(t);
    if (!mm) return;
    const mb = modAt(m.taskRam[m.T][1]);
    if (!mb || mb[0] !== prog) return;
    progTask = parseInt(mm[1], 16);
    const k = parseInt(mm[2], 16);
    seg = k ? (SEGS[k - 1] || 'seg' + k) : null;
    if (seg) segOf(seg);
  };
  m = boot({ prom: built.image, seed: 1, marks: [], log, watches: [{ addr: MARK, task: -1 }], trace: 0, sd: [card],
    input: '\u0101cd /sd/0; echo nprof-start\r\u0101' + o.cmd + '\r\u0101echo nprof-end\r' });
  const kl = LABELS ? labels() : { at: () => '' };
  const XCALL = 0xF80C;
  const ENT = new Map();                                       // (The libraries' entries by address: the SDK's numbers.inc)
  const inc = path.join(tree, 'bin', 'sdk', 'asm', 'numbers.inc');
  if (fs.existsSync(inc)) for (const l of fs.readFileSync(inc, 'latin1').split(/\r?\n/)) {
    const x = /^((NUM|MATH)_\w+)\s*=\s*\$([0-9A-F]{4})/.exec(l);
    if (x) ENT.set((x[2] === 'NUM' ? 'numbers' : 'math') + ':' + parseInt(x[3], 16), x[1]);
  }
  const where = (T, pc) => {
    if (pc >= 0xE000) return 'BIOS ' + kl.at(pc, m.W).split('+')[0];
    if (pc >= 0xA000) {
      const mb = modAt(m.taskRam[T][1]);
      if (!mb) return 'PROM?';
      const [n, kk] = mb, L = modLabels(n);
      return n + (L.banks.length > 1 ? '#' + (kk + 1) : '') + (LABELS ? ' ' + near(L.banks[kk], pc) : '');
    }
    if (pc >= 0x0400 && pc < 0x0800 && T === progTask) return prog + '#ram' + (LABELS ? ' ' + near(modLabels(prog).ram, pc) : '');
    return 'RAM ' + (pc < 0x8000 ? 'task' : 'bank');
  };
  const xstack = [], jstack = [], jcount = new Map();
  const step0 = m.cpu.step.bind(m.cpu);
  m.cpu.step = function (iv) {
    const pc = this.PC, c0 = this.cyc, T = m.T, op0 = TREE ? m.rd(pc) : 0;
    if (ENTRIES && pc === XCALL && seg) {                      // (A library call: its entry, its return watched for)
      const sp = this.S, ret = (m.rd(0x0101 + sp) | m.rd(0x0102 + sp) << 8) + 1;
      const tgt = m.rd(0x20) | m.rd(0x21) << 8, mb = modAt(m.rd(0x1E)), n = mb ? mb[0] : '?';
      xstack.push({ ret, sp: (sp + 2) & 0xFF, c0, name: ENT.get(n + ':' + tgt) || n + ':$' + tgt.toString(16), T });
    }
    if (TREE && seg && T === progTask && op0 === 0x20) {        // (A JSR: the routine, inclusive)
      const a = m.rd(pc + 1) | m.rd(pc + 2) << 8;
      if (a >= 0xA000) { const name = where(T, a); jstack.push({ name, sp: this.S, c0, T }); jcount.set(name, (jcount.get(name) || 0) + 1); }
    }
    const r = step0(iv);
    if (xstack.length) {
      const top = xstack[xstack.length - 1];
      if (this.PC === top.ret && this.S === top.sp && m.T === top.T) {
        xstack.pop();
        const s = segs.get(seg);
        if (s) { const e = s.ent.get(top.name) || [0, 0]; e[0]++; e[1] += this.cyc - top.c0; s.ent.set(top.name, e); }
      }
    }
    while (jstack.length && (op0 === 0x60 || op0 === 0x9A) && (pc < 0xE000 || jstack[jstack.length - 1].name.startsWith('BIOS'))
      && m.T === jstack[jstack.length - 1].T && this.S >= jstack[jstack.length - 1].sp) {
      const fr = jstack.pop(), n = jcount.get(fr.name) - 1;
      jcount.set(fr.name, n);
      const sg = segs.get(seg);
      if (sg && n === 0) { const e = sg.tree.get(fr.name) || [0, 0]; e[0]++; e[1] += this.cyc - fr.c0; sg.tree.set(fr.name, e); }
    }
    if (seg) {
      const dc = this.cyc - c0, s = segs.get(seg);
      s.total += dc;
      if (T !== progTask) s.other += dc;
      if (LABELS || INSIDE) {
        const k = where(T, pc);
        s.prof.set(k, (s.prof.get(k) || 0) + dc);
        if (INSIDE && xstack.length) { const e = xstack[xstack.length - 1].name; if (!s.inside.has(e)) s.inside.set(e, new Map()); const im = s.inside.get(e); im.set(k, (im.get(k) || 0) + dc); }
      }
    }
    return r;
  };
  while (m.cpu.cyc < MAX && !m.cpu.halted) {
    m.run(m.cpu.cyc + 2e6);
    if (/\nnprof-end\n/.test(m.out.slice(-100).replace(/\r/g, ''))) break;
  }
  let out = m.out.replace(/\r/g, '');
  const at = out.indexOf('\nnprof-start\n');
  out = at < 0 ? out.slice(-1500) : out.slice(at + 13);
  const end = out.indexOf('echo nprof-end');
  out = end < 0 ? out + '\n[no end]\n' : out.slice(0, end);
  return { out, segs, order, cycles: m.cpu.cyc };
}

function report(r, o) {
  const lines = [];
  const ms = c => (c / 3579.545).toFixed(0) + ' ms';
  for (const n of r.order) {
    if (o.only && o.only.length && !o.only.includes(n)) continue;
    const s = r.segs.get(n);
    lines.push('', '==== ' + n + ': ' + (s.total / 1e6).toFixed(2) + 'M cycles (' + ms(s.total) + '), other tasks ' + (100 * s.other / (s.total || 1)).toFixed(1) + '%');
    const g = new Map();
    for (const [k, c] of s.prof) { const x = k.split(' ')[0]; g.set(x, (g.get(x) || 0) + c); }
    if (g.size) lines.push('  by module: ' + [...g].sort((a, b) => b[1] - a[1]).map(([x, c]) => x + ' ' + (100 * c / s.total).toFixed(1) + '%').join(', '));
    for (const [k, c] of [...s.prof].sort((a, b) => b[1] - a[1]).slice(0, o.top))
      lines.push('  ' + (100 * c / s.total).toFixed(1).padStart(5) + '% ' + (c / 1e6).toFixed(2).padStart(8) + 'M  ' + k);
    if (s.tree.size) {
      lines.push('  routines (inclusive):');
      for (const [k, [nc, c]] of [...s.tree].sort((a, b) => b[1][1] - a[1][1]).slice(0, o.routines))
        lines.push('  ' + (100 * c / s.total).toFixed(1).padStart(5) + '% ' + String(nc).padStart(7) + ' calls ' + String(Math.round(c / nc)).padStart(8) + ' a call  ' + k);
    }
    if (s.ent.size) {
      lines.push('  library entries (inclusive):');
      for (const [k, [nc, c]] of [...s.ent].sort((a, b) => b[1][1] - a[1][1])) {
        lines.push('  ' + (100 * c / s.total).toFixed(1).padStart(5) + '% ' + String(nc).padStart(6) + ' calls ' + String(Math.round(c / nc)).padStart(8) + ' a call  ' + k);
        const im = s.inside.get(k);
        if (im) for (const [kk, cc] of [...im].sort((a, b) => b[1] - a[1]).slice(0, o.inside))
          lines.push('          ' + String(Math.round(cc / nc)).padStart(8) + ' a call ' + (100 * cc / c).toFixed(1).padStart(5) + '%  ' + kk);
      }
    }
  }
  return lines.join('\n');
}

module.exports = { profile, report };

if (require.main === module) {
  const args = process.argv.slice(2);
  const opt = (n, d) => { const i = args.indexOf(n); if (i < 0) return d; const v = args[i + 1]; args.splice(i, 2); return v; };
  const flag = n => { const i = args.indexOf(n); if (i < 0) return false; args.splice(i, 1); return true; };
  const o = { segs: opt('--segs', '').split(',').filter(Boolean), mark: parseInt(opt('--mark', '6F'), 16), top: +opt('--top', 25),
    inside: +opt('--inside', 0), routines: +opt('--tree', 0), only: opt('--only', '').split(',').filter(Boolean),
    local: flag('--local'), entries: flag('--entries'), tree: opt('--build', '') };
  const [dir, cmd] = args;
  if (!dir || !cmd) { console.error('usage: node sim/tools/nprof.js DIR CMD --segs A,B,... (see its head)'); process.exit(2); }
  o.files = {};
  for (const n of fs.readdirSync(dir)) if (fs.statSync(path.join(dir, n)).isFile()) o.files[n] = fs.readFileSync(path.join(dir, n));
  o.cmd = cmd;
  const r = profile(o);
  console.log(r.out);
  console.log(report(r, o));
}
