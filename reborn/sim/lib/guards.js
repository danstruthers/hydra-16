// ****************************************************************************
// guards.js - the emulator's guards (games/dq0/GAME-CRITIQUE.md's 6.1, E1): the bank window and the stacks checked as
// a test runs, at no cost in a program's bytes (base/sim/lib/machine.js's opt.guard: the WebAssembly core's checks,
// core65's lib.rs, and the JS CPU's, the same).  Every test has them (sim/test.js: GUARDS=0 none; GUARDS=note each hit
// noted, none stopping the run; else the first new one stops the machine there, and fails the test).  Five guards:
//   fetch  an instruction fetched from the window ($8000-$9FFF) with no code there: the task's program's code linked
//          at $8000 (its link config's areas there, an overlay's or the world's catalogue's) as the build made it (the
//          bank's bytes those of the area's file: obj/games/NAME.hyx.ovl ...), or a module of the kernel's code cache
//          (a bank program run in place, a child: the task's TK_RCACHE banks, their directories).  79083a98's crash
//          ("look in the side pocket" returned into the hub's bytes as overlay 1's code) stops at its first byte.
//   store  a write into the window by an instruction that names the hub's header (an absolute store, plain or
//          indexed, its address in struct hub, from a C line with HUB-> or a macro that means it, or an assembly line
//          with a hub.* offset's name: obj/gen/dq0hub_*.inc), the window's bank not the hub's header's (79083a98's
//          HUB->ireq.to written into overlay 1's bank).  Instructions at the same PC in another swapped part aren't.
//   load   a read from the window by an instruction that names the hub's header (an absolute load, plain or indexed, a
//          compare, a sum, a BIT, or a read-modify-write: a load and a store both), from such a line, the window's bank
//          not the hub's header's (7a65b8eb's HUB->seed read with a chunk's bank there), the byte read in a field of the
//          header the line names (HUB->pc_x: pc_x's two bytes; a macro, the fields its body names; an assembly line,
//          its offsets' fields): a line that names the hub and reads another bank's bytes on purpose (a chunk's, an
//          item's: ITEM_AT (i)->...) isn't reading the hub there.
//   swap   a task's swapped parts' calls past its swap stack (swap.inc's swsp past SWDEPTH: swstk overrun), and its
//          overlays' calls past theirs (bank.s's ovsp past DEPTH).
//   stack  cc65's C stack pointer below its floor: __RAMTOP__ - __STACKMIN__ in a task with a swap area (the part in
//          use under it), else its BSS's end (its heap's room's top, __RAMTOP__ - __STACKSIZE__, with one); a C stack
//          of the C library's own put in it not (signal.s's handler's, in its BSS).  The canary is the stack pointer
//          itself, watched: the bytes under the floor are a part's, rewritten by every swap, so no byte planted there
//          could tell.  Its high byte is what's watched (the floors are page-aligned).
// A program's facts are the build's (each the first time it's met, cached in NAME.guards.json beside its map): its
// map, labels, link config (NAME.cfg: its areas at $8000, their files, their segments' places), listings (bank.lst's
// SWDEPTH, DEPTH; every .lst's stores and loads, their C lines), overlay files; the kernel's (base/include/layout.inc: a
// task's name, its code cache); the hub's (obj/gen/dq0hub.json: struct hub's fields).  A task's program is known by its
// name (TA_NAME).  The stores' and loads' tables a task's swap area's PCs are its part's in use (swap.inc's swapped,
// watched: each part copied in, its own put there).
// KNOWN: what the guards take as it is, each with its reason: a program whose own banks hold code it makes as it runs
// (banks: 'own': their pages taken as code, nothing said), or a hit known and not yet mended (at: its function: noted,
// not failed).
'use strict';
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..', '..');                // (reborn/)
const BASE = path.join(ROOT, '..', 'base');
const VERSION = 4;                                            // (The facts' form: a change, all made again)
const KNOWN = [
  { kind: 'fetch', prog: 'hylang', banks: 'own', why: 'hylang compiles a function\'s bytecode to the 65C02\'s own code in its arena, banks of its own (modules/hylang/vmx.inc), and runs it there: code made as it runs, no build output says where' },
  { kind: 'fetch', prog: 'forth', banks: 'own', why: 'HyForth is subroutine threaded: a colon definition is 65C02 code, compiled into its code banks (modules/forth/fcomp.inc) and run there' },
  { kind: 'fetch', prog: 'basic', banks: 'own', why: 'BASIC compiles a program to 65C02 code in its regions\' banks (modules/basic/comp.inc) and runs it there' },
];

const hx = (v, n = 4) => '$' + v.toString(16).toUpperCase().padStart(n, '0');
const num = s => { s = String(s || '').trim().replace(/^"|"$/g, ''); return /^\$[0-9A-Fa-f]+$/.test(s) ? parseInt(s.slice(1), 16) : /^\d+$/.test(s) ? +s : NaN; };

// ---- The kernel's layout (base/include/layout.inc): a task's name, its code cache
let K = null;
function kernel() {
  if (K) return K;
  const t = fs.readFileSync(path.join(BASE, 'include', 'layout.inc'), 'latin1'), v = {};
  for (const [, n, x] of t.matchAll(/^(\w+)\s*=\s*(\$[0-9A-Fa-f]+|\d+)\b/gm)) if (v[n] === undefined) v[n] = num(x);
  for (const n of ['TA_NAME', 'TK_RCACHE', 'RB_NEXT', 'RB_DIR', 'RB_ENTS', 'RE_START', 'RE_END', 'RE_GEN'])
    if (v[n] === undefined) throw new Error('guards: base/include/layout.inc has no ' + n);
  return (K = v);
}

// ---- The programs the build linked (a map and labels): name -> { name, dir, out: its image }; a name two have, null
let index = null;
function programs() {
  if (index) return index;
  index = new Map();
  for (const r of ['games', 'programs', 'samples', path.join('samples', 'c'), 'tests']) {
    const d = path.join(ROOT, 'obj', r);
    if (!fs.existsSync(d)) continue;
    for (const n of fs.readdirSync(d)) {
      const dir = path.join(d, n);
      if (!fs.existsSync(path.join(dir, n + '.map')) || !fs.existsSync(path.join(dir, n + '.lbl'))) continue;
      index.set(n, index.has(n) ? null : { name: n, dir, out: path.join(d, n + '.hyx') });
    }
  }
  return index;
}
// Its link config: the build's (a task with parts: parts.js's), else its own, else none (sdk/c/hydra.cfg's: nothing
// at $8000)
function cfgOf(p) {
  const made = path.join(p.dir, p.name + '.cfg');
  if (fs.existsSync(made)) return fs.readFileSync(made, 'latin1');
  for (const d of ['games', 'programs', path.join('tests', 'ram'), path.join('sdk', 'c', 'samples')]) {
    const f = path.join(ROOT, d, p.name, p.name + '.cfg');
    if (fs.existsSync(f)) return fs.readFileSync(f, 'latin1');
  }
  return null;
}

// ---- A map's segment list (name -> { start: its run address, size, align }) and modules list (an object -> { its
// segments' offsets })
function segments(map) {
  const out = {}, list = (map.split('Segment list:')[1] || '').split(/Exports list/)[0];
  for (const [, n, s, , z, a] of list.matchAll(/^(\w+)\s+([0-9A-F]{6})\s+([0-9A-F]{6})\s+([0-9A-F]{6})\s+([0-9A-F]{5})/gm))
    out[n] = { start: parseInt(s, 16), size: parseInt(z, 16), align: parseInt(a, 16) || 1 };
  return out;
}
function modules(map) {
  const out = {}, list = (map.split('Modules list:')[1] || '').split('Segment list:')[0];
  let cur = null;
  for (const l of list.split(/\r?\n/)) {
    const o = l.match(/^(\S.*?\.o\)?):\s*$/);                // (A library's member: hydra.lib(signal.o))
    if (o) { cur = out[o[1]] = { sizes: {} }; continue; }
    const s = l.match(/^\s+(\w+)\s+Offs=([0-9A-F]+)\s+Size=([0-9A-F]+)/);
    if (s && cur) { cur[s[1]] = parseInt(s[2], 16); cur.sizes[s[1]] = parseInt(s[3], 16); }
  }
  return out;
}
// A label file's names (the first address each)
function labelsOf(file) {
  const out = {}, list = [];
  for (const [, a, n] of fs.readFileSync(file, 'latin1').matchAll(/^al ([0-9A-F]+) \.(\S+)/gm)) {
    if (out[n] === undefined) out[n] = parseInt(a, 16);
    list.push([parseInt(a, 16), n]);
  }
  return { by: out, list };
}

// ---- The window's code: the config's areas at $8000-$9FFF in a file (an overlay's, the catalogue's ...), each with
// its place in its file (the areas before it there: a filled one's size, else its segments' bytes but a BSS's) and
// its code (its segments named ...CODE that run where they're loaded).  Each file's length is checked against the
// areas': one the build made longer after the link (the world's catalogue, its kinds' words put after the kinds:
// dq0w/make.js) has its areas' places as linked and as moved by the bytes added (offs: both tried); a shorter one, or
// a config this can't read, is said, not guessed.  OUT: [{ area, file, offs: [place in its file], start, code:
// [[from, to)], segs: [its segments' names], hub: [what in its code names the hub's header: make's] }]
function areas(p, cfg, segs) {
  const block = k => (cfg.split(new RegExp('^' + k + '\\s*\\{', 'm'))[1] || '').split('}')[0];
  const kv = s => { const o = {}; for (const x of s.split(',')) { const [k, v] = x.split('='); if (v !== undefined) o[k.trim()] = v.trim(); } return o; };
  const mem = [], order = [];
  for (const l of block('MEMORY').split(/\r?\n/)) {
    const m = l.replace(/#.*/, '').match(/^\s*(\w+)\s*:\s*(.*?);/);
    if (m) { const a = kv(m[2]); mem.push({ name: m[1], file: (a.file || '').replace(/^"|"$/g, ''), start: num(a.start), size: num(a.size), fill: a.fill === 'yes' }); }
  }
  for (const l of block('SEGMENTS').split(/\r?\n/)) {
    const m = l.replace(/#.*/, '').match(/^\s*(\w+)\s*:\s*(.*?);/);
    if (m) { const a = kv(m[2]); order.push({ name: m[1], load: a.load, run: a.run || a.load, bss: a.type === 'bss' }); }
  }
  const files = new Map(), out = [];
  for (const a of mem) {
    if (!/^%O\./.test(a.file)) continue;                    // (The program's own file, %O, or none: not an area's)
    if (isNaN(a.start) || (a.fill && isNaN(a.size))) throw new Error('guards: ' + p.name + '\'s ' + a.name + ': its start or size not a number');
    let cur = a.start, end = a.start, code = [], names = [];
    for (const s of order.filter(s => s.load === a.name && segs[s.name])) {
      const g = segs[s.name];
      cur = Math.ceil(cur / g.align) * g.align;
      if (s.run === a.name) {
        if (g.start !== cur) throw new Error('guards: ' + p.name + '\'s ' + s.name + ' at ' + hx(g.start) + ' in its map, ' + hx(cur) + ' by its config');
        if (/CODE$/.test(s.name) && g.size) code.push([cur, cur + g.size]);
        names.push(s.name);
      }
      cur += g.size;
      if (!s.bss) end = cur;                                // (A BSS isn't in the file)
    }
    const f = files.get(a.file) || 0;
    if (a.start >= 0x8000 && a.start < 0xA000 && code.length) out.push({ area: a.name, file: a.file.slice(2), offs: [f], start: a.start, code, segs: names, hub: [] });
    files.set(a.file, f + (a.fill ? a.size : end - a.start));
  }
  for (const [file, n] of files) {
    const f = p.out + file.slice(2), size = fs.existsSync(f) ? fs.statSync(f).size : n;
    if (size < n) throw new Error('guards: ' + p.name + '\'s ' + path.basename(f) + ' is ' + size + ' bytes, its config\'s areas ' + n);
    if (size > n) for (const A of out) if (A.file === file.slice(2)) A.offs.push(A.offs[0] + size - n);
  }
  return out;
}

// ---- The instructions that name the hub's header, from the listings: an absolute store (plain or indexed, or a read-
// modify-write) or load (a load, a compare, a sum, a BIT, a read-modify-write; plain or indexed) at an address in
// struct hub, from a C line that names it (HUB->, or a macro whose body does: the headers' and the file's own), or an
// assembly line naming one of the hub's header's offsets (obj/gen/dq0hub_*.inc: "(hub.").  A load's the fields of the
// header its line names too (their spans: the read is the hub's if the byte's in one).  The functions too (.proc), for
// the hits' words.  OUT: { main: [pc], parts: { n: [pc] }, window: [{ pc, seg, line, fn }], loads: { main: [[pc, s, w,
// a]], parts: { n: [...] }, window: [{ pc, seg, line, fn }] }, spans: [[from, to, ...]] (s: its index), lines:
// ['file:line'] (w), procs: [[pc, name, part]] }.  One in code that runs in the window (window: an overlay's) can't be
// right: the window holds its own bank as it runs, never the header
const STORES = new Set([0x8D, 0x9D, 0x99, 0x8E, 0x8C, 0x9C, 0x9E, 0xEE, 0xFE, 0xCE, 0xDE, 0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0x0C, 0x1C]);
// (The loads: LDA, LDX, LDY; ORA, AND, EOR, ADC, CMP, SBC; CPX, CPY; BIT; JMP's vector; and the read-modify-writes)
const LOADS = new Set([0xAD, 0xBD, 0xB9, 0xAE, 0xBE, 0xAC, 0xBC,
  0x0D, 0x1D, 0x19, 0x2D, 0x3D, 0x39, 0x4D, 0x5D, 0x59, 0x6D, 0x7D, 0x79, 0xCD, 0xDD, 0xD9, 0xED, 0xFD, 0xF9,
  0xEC, 0xCC, 0x2C, 0x3C, 0x6C, 0x7C,
  0xEE, 0xFE, 0xCE, 0xDE, 0x0E, 0x1E, 0x2E, 0x3E, 0x4E, 0x5E, 0x6E, 0x7E, 0x0C, 0x1C]);
let hubFacts = null;
function hub() {
  if (hubFacts) return hubFacts;
  const H = require('../../games/dq0/hubgen.js').load();
  const asm = new Map();                                      // (Assembly's names of the header's offsets: name -> field)
  const gen = path.join(ROOT, 'obj', 'gen');
  for (const f of fs.existsSync(gen) ? fs.readdirSync(gen).filter(n => /^dq0hub_.*\.inc$/.test(n)) : [])
    for (const [, n, p] of fs.readFileSync(path.join(gen, f), 'latin1').matchAll(/^(\w+)\s*=.*;\s*\(hub\.(\w+)/gm)) asm.set(n, p);
  // The header's fields (struct hub's own, its anonymous members' too: pc_x ...): name -> [from, to)
  const fields = new Map();
  for (const [n, f] of Object.entries(H.S.hub.f)) if (!n.includes('.')) fields.set(n, [f[0], f[0] + f[1]]);
  // The headers' macros that name the header (their bodies HUB->, or another such), every game's .h; and the fields
  // each names
  const defs = new Map();
  for (const d of fs.readdirSync(path.join(ROOT, 'games'))) {
    const dd = path.join(ROOT, 'games', d);
    if (fs.statSync(dd).isDirectory()) for (const f of fs.readdirSync(dd).filter(n => n.endsWith('.h'))) macros(fs.readFileSync(path.join(dd, f), 'latin1'), defs);
  }
  const fieldAt = off => [...fields].filter(([n, [s, e]]) => off >= s && off < e && !/^_/.test(n)).map(([n]) => n).join(', ') || 'no field';
  return (hubFacts = { size: H.size('hub'), magic: H.C.HUB_MAGIC, banks: H.C.HUB_BANKS, asm, fields, fieldAt, header: closure(defs, new Set()), named: named(defs, new Map()) });
}
// The fields of the header each macro names (its body's HUB->x, and the fields of the macros it names, theirs ...):
// the headers' (known) and a file's own (defs) -> { name: Set of fields }
function named(defs, known) {
  const F = new Map(known), body = [];
  for (const [n, b] of defs) body.push([n, [...b.matchAll(/\bHUB\s*->\s*(\w+)/g)].map(x => x[1]), new Set(b.match(/\b[A-Za-z_]\w*\b/g) || [])]);
  for (let more = true; more;) {
    more = false;
    for (const [n, direct, words] of body) {
      const s = new Set([...(F.get(n) || []), ...direct]);
      for (const w of words) if (w !== n && F.has(w)) for (const x of F.get(w)) s.add(x);
      if (s.size > (F.has(n) ? F.get(n).size : 0)) { F.set(n, s); more = true; }
    }
  }
  return F;
}
function macros(text, defs) {
  const t = text.replace(/\\\r?\n/g, ' ');
  for (const [, n, body] of t.matchAll(/^[ \t]*#[ \t]*define[ \t]+(\w+)(?:\([^)]*\))?(.*)$/gm)) defs.set(n, body.replace(/\/\*.*?\*\//g, '').replace(/\/\*.*$/, ''));
}
function closure(defs, names) {
  for (let more = true; more;) {
    more = false;
    const re = names.size ? new RegExp('\\b(' + [...names].join('|') + ')\\b') : null;
    for (const [n, b] of defs) if (!names.has(n) && (/\bHUB\s*->/.test(b) || (re && re.test(b)))) { names.add(n); more = true; }
  }
  return names;
}
// A C file's text with its comments blanked (its lines kept: a comment's newlines stay)
function uncomment(t) {
  let o = '', i = 0;
  while (i < t.length) {
    const c = t[i], d = t[i + 1];
    if (c === '/' && d === '*') { const e = t.indexOf('*/', i + 2), end = e < 0 ? t.length : e + 2; o += t.slice(i, end).replace(/[^\r\n]/g, ' '); i = end; }
    else if (c === '/' && d === '/') { const e = t.indexOf('\n', i), end = e < 0 ? t.length : e; o += ' '.repeat(end - i); i = end; }
    else if (c === '"' || c === '\'') { let j = i + 1; while (j < t.length && t[j] !== c && t[j] !== '\n') j += t[j] === '\\' ? 2 : 1; o += t.slice(i, j + 1); i = j + 1; }
    else { o += c; i++; }
  }
  return o;
}
function listings(p, segs, mods, f) {
  const H = hub(), lo = 0x8000, hi = 0x8000 + H.size, cfiles = new Map();
  const names = file => {                                     // (A C file's: its code's lines, what names the header, and
    let c = cfiles.get(file);                                 //   the fields each of its macros names)
    if (c) return c;
    let text = '';
    const rel = /[\\/](reborn|base)[\\/](.*)$/.exec(file);   // (Its tree moved since the build: the file here.  A name not
    for (const f of [path.resolve(ROOT, file), ...(rel ? [path.join(ROOT, '..', rel[1], rel[2])] : [])]) {   //   absolute is
      try { text = fs.readFileSync(f, 'latin1'); break; } catch (e) { /* (Gone: none of its lines named) */ }   //   reborn/'s,
    }                                                         //   where the build runs cc65, since 2026-11-17)
    const defs = new Map();
    macros(text, defs);
    const all = closure(defs, new Set(H.header));
    c = { code: uncomment(text).split(/\r?\n/), re: new RegExp('\\bHUB\\s*->' + (all.size ? '|\\b(' + [...all].join('|') + ')\\b' : '')), F: named(defs, H.named) };
    cfiles.set(file, c);
    return c;
  };
  // A load's spans: the header's fields its statement names (HUB->x, its macros' fields), or its assembly line's
  // offsets' (OUT: an index in out.spans, the spans' list kept once)
  const spanKeys = new Map();
  const spansOf = list => {
    const s = [...new Set(list)].filter(n => H.fields.has(n)).map(n => H.fields.get(n)).sort((x, y) => x[0] - y[0] || x[1] - y[1]);
    const k = s.map(x => x.join('-')).join(',');
    if (!spanKeys.has(k)) { spanKeys.set(k, out.spans.length); out.spans.push(s.flat()); }
    return spanKeys.get(k);
  };
  const cFields = (c, s) => {
    const list = [...s.matchAll(/\bHUB\s*->\s*(\w+)/g)].map(x => x[1]);
    for (const w of s.match(/\b[A-Za-z_]\w*\b/g) || []) if (c.F.has(w)) list.push(...c.F.get(w));
    return list;
  };
  const lineKeys = new Map();
  const lineOf = w => { if (!lineKeys.has(w)) { lineKeys.set(w, out.lines.length); out.lines.push(w); } return lineKeys.get(w); };
  // The statement a C line's code is said to be (cc65's .dbg line: a statement with a comment after it that runs on
  // is said at the comment's last line): back past the lines with no code, then to the statement's first line
  const statement = (c, n) => {
    let k = n - 1;
    while (k > 0 && !(c.code[k] || '').trim()) k--;
    let j = k;
    for (; j > 0; j--) { const q = (c.code[j - 1] || '').trim(); if (!q || /[;{}]$/.test(q) || /^#/.test(q)) break; }
    return c.code.slice(j, k + 1).join(' ');
  };
  const asmRe = H.asm.size ? new RegExp('\\b(' + [...H.asm.keys()].join('|') + ')\\b') : null;
  const out = { main: [], parts: {}, window: [], loads: { main: [], parts: {}, window: [] }, spans: [], lines: [], procs: [] };
  const partOf = seg => { const m = /^SW(\d+)/.exec(seg); return m ? +m[1] : 0; };
  for (const lst of fs.readdirSync(p.dir).filter(n => n.endsWith('.lst')).sort()) {
    const offs = mods[lst.replace(/\.lst$/, '.o')];
    if (!offs) continue;
    let seg = 'CODE', stack = [], cline = null, fn = '';
    const at = o => segs[seg] && offs[seg] !== undefined ? segs[seg].start + offs[seg] + o : null;
    for (const l of fs.readFileSync(path.join(p.dir, lst), 'latin1').split(/\r?\n/)) {
      const m = l.match(/^([0-9A-F]{6})[r ]\s*\d+\s\s([0-9A-Fr ]{0,12})(.*)$/);
      if (!m) continue;
      const src = m[3].trim(), bytes = m[2].trim().split(/\s+/).filter(Boolean);
      let d;
      if (!bytes.length) {
        if ((d = src.match(/^\.segment\s+"(\w+)"/i))) seg = d[1];
        else if ((d = src.match(/^\.(code|rodata|data|bss|zeropage)\b/i))) seg = d[1].toUpperCase();
        else if (/^\.pushseg\b/i.test(src)) stack.push(seg);
        else if (/^\.popseg\b/i.test(src)) seg = stack.pop() || 'CODE';
        else if ((d = src.match(/^\.dbg\s+line(?:\s*,\s*"([^"]+)"\s*,\s*(\d+))?/i))) cline = d[1] ? [d[1], +d[2]] : null;
        else if ((d = src.match(/^\.proc\s+(\w+)/i))) { fn = d[1].replace(/^_/, ''); const pc = at(parseInt(m[1], 16)); if (pc !== null) out.procs.push([pc, fn, partOf(seg)]); }
        continue;
      }
      const op = parseInt(bytes[0], 16), st = STORES.has(op), ld = LOADS.has(op);
      if (!(st || ld) || bytes.length !== 3 || bytes[1] === 'rr' || bytes[2] === 'rr') continue;
      const a = parseInt(bytes[1], 16) | parseInt(bytes[2], 16) << 8;
      if (a < lo || a >= hi) continue;
      let named, fields = [];                                 // (A C line's, cc65's listing: its statement, and a load's
      if (cline) {                                            //   fields its statement names; else the assembly's own
        const c = names(cline[0]), s = statement(c, cline[1]);   //   line, its offsets' fields)
        named = c.re.test(s);
        if (named && ld) fields = cFields(c, s);
      } else {
        const s = src.replace(/;.*/, '');
        named = !!asmRe && asmRe.test(s);
        if (named && ld) fields = [...s.matchAll(/\b\w+\b/g)].map(x => H.asm.get(x[0])).filter(Boolean);
      }
      if (!named) continue;
      const pc = at(parseInt(m[1], 16));
      if (pc === null) continue;
      const where = cline ? path.basename(cline[0]) + ':' + cline[1] : lst + ': ' + src, n = partOf(seg);
      if (st) {
        if (n) (out.parts[n] = out.parts[n] || []).push(pc);
        else if (pc >= 0x8000 && pc < 0xA000) out.window.push({ pc, seg, line: where, fn });
        else out.main.push(pc);
      }
      if (ld) {
        const e = [pc, spansOf(fields), lineOf(where), a];
        if (n) (out.loads.parts[n] = out.loads.parts[n] || []).push(e);
        else if (pc >= 0x8000 && pc < 0xA000) out.loads.window.push({ pc, seg, line: where, fn });
        else out.loads.main.push(e);
      }
    }
  }
  out.procs.sort((x, y) => x[0] - y[0]);
  return out;
}

// ---- A program's facts, made (OUT: what the guards use) or the cache's
const facts_ = new Map();
function facts(name) {
  if (facts_.has(name)) return facts_.get(name);
  const p = programs().get(name);
  if (!p) { facts_.set(name, null); return null; }
  const map = path.join(p.dir, name + '.map'), lbl = path.join(p.dir, name + '.lbl'), file = path.join(p.dir, name + '.guards.json');
  const key = [VERSION, ...[map, lbl].map(x => { const s = fs.statSync(x); return s.size + ':' + Math.round(s.mtimeMs); })].join(' ');
  let f = null;
  try { const j = JSON.parse(fs.readFileSync(file, 'utf8')); if (j.key === key) f = j; } catch (e) { /* (None yet) */ }
  if (!f) {
    f = make(p);
    f.key = key;
    try { fs.writeFileSync(file + '.' + process.pid, JSON.stringify(f)); fs.renameSync(file + '.' + process.pid, file); } catch (e) { /* (Made again next time) */ }
  }
  f.out = p.out;
  facts_.set(name, f);
  return f;
}
function make(p) {
  const text = fs.readFileSync(path.join(p.dir, p.name + '.map'), 'latin1'), L = labelsOf(path.join(p.dir, p.name + '.lbl')).by;
  const segs = segments(text), mods = modules(text);
  const f = { name: p.name, sp: null, floor: null, alt: [], swsp: null, swdepth: null, ovsp: null, depth: null, swapped: null, swap: null, ovbank: null,
    areas: [], stores: { main: [], parts: {}, window: [] }, loads: { main: [], parts: {}, window: [] }, spans: [], lines: [], procs: [] };
  for (const [o, s] of Object.entries(mods))                // (A C stack of the C library's own: signal.s's handler's, in its
    if (/\(signal\.o\)$/.test(o) && s.BSS !== undefined && segs.BSS) f.alt.push([segs.BSS.start + s.BSS, segs.BSS.start + s.BSS + s.sizes.BSS]);   //   BSS: the stack it runs on, not under the floor)
  const bank = fs.existsSync(path.join(p.dir, 'bank.lst')) ? fs.readFileSync(path.join(p.dir, 'bank.lst'), 'latin1') : '';
  const eq = n => { const m = bank.match(new RegExp('^[0-9A-F]{6}[r ]\\s*\\d+\\s+' + n + '\\s*=\\s*(\\$?[0-9A-Fa-f]+)\\b', 'm')); return m ? num(m[1]) : null; };
  if (L.__SWAP_START__ !== undefined) f.swap = [L.__SWAP_START__, L.__SWAP_START__ + (L.__SWAPSIZE__ || 0)];
  if (L.sp !== undefined && L.__RAMTOP__ !== undefined && L.__STACKMIN__ !== undefined && segs.BSS) {   // (The C stack's floor)
    const heap = /\((_heap|malloc|calloc|realloc|_heapadd)\.o\)/.test(text);
    f.sp = L.sp;
    f.floor = f.swap ? L.__RAMTOP__ - L.__STACKMIN__ : heap && L.__STACKSIZE__ !== undefined ? L.__RAMTOP__ - L.__STACKSIZE__ : segs.BSS.start + segs.BSS.size;
  }
  if (L.swsp !== undefined && eq('SWDEPTH') !== null) { f.swsp = L.swsp; f.swdepth = eq('SWDEPTH'); }
  if (L.ovsp !== undefined && eq('DEPTH') !== null) { f.ovsp = L.ovsp; f.depth = eq('DEPTH'); }
  if (L.swapped !== undefined) f.swapped = L.swapped;
  if (L._ovbank !== undefined) f.ovbank = L._ovbank;
  const cfg = cfgOf(p);
  if (cfg) f.areas = areas(p, cfg, segs);
  if (/^dq0/.test(p.name) && fs.existsSync(path.join(ROOT, 'obj', 'gen', 'dq0hub.json'))) {   // (The hub's: Dingus Quest Zero's)
    const s = listings(p, segs, mods, f);
    f.stores = { main: s.main, parts: s.parts, window: s.window };
    f.loads = s.loads;
    f.spans = s.spans;
    f.lines = s.lines;
    f.procs = s.procs;
    for (const [kind, list] of [['store', s.window], ['load', s.loads.window]]) for (const e of list) {   // (In the window:
      const A = f.areas.find(A => A.segs.includes(e.seg));                                                 //   its area's)
      if (A) A.hub.push(Object.assign({ kind }, e));
    }
  }
  return f;
}

// Where pc is in a program (its function, from the listings; else its labels): "name+$n", or the address
function where(f, pc, part) {
  if (!f) return hx(pc);
  let best = null;
  if (f.procs.length) {                                       // (A swapped part's, its own; the window's, its own)
    for (const e of f.procs) { if (e[0] > pc) break; if (e[2] === part && (e[0] >= 0x8000) === (pc >= 0x8000)) best = e; }
  } else {
    if (!f.labels) f.labels = labelsOf(path.join(programs().get(f.name).dir, f.name + '.lbl')).list.filter(e => !/^__/.test(e[1])).sort((x, y) => x[0] - y[0]);
    for (const e of f.labels) { if (e[0] > pc) break; best = e; }
  }
  return best ? best[1] + (pc > best[0] ? '+' + (pc - best[0]) : '') + ' (' + hx(pc) + (part ? ', part ' + part : '') + ')' : hx(pc) + (part ? ' (part ' + part + ')' : '');
}

// ---- The guard: createGuard({ mode: 'stop' (the first new hit halts the machine) or 'note', expect: [kinds] (a test
// of the guards: the kinds it makes fire on purpose, noted, each wanted) }).  OUT: machine.js's opt.guard ({ flags:
// GUARDS_FLAGS's (1 fetch, 2 store, 4 the stacks, 8 load), all four if none set, sync, hit }), and attach(m), judge(m)
// -> { failures, notes }, hits(), state(), restore(s)
function createGuard(opt = {}) {
  const k = kernel(), expect = opt.expect || [], mode = expect.length ? 'note' : opt.mode || 'stop', flags = +(process.env.GUARDS_FLAGS || 15);
  const names = new Array(16).fill(null), progs = new Array(16).fill(null), parts = new Array(16).fill(null), hits = [];
  let m = null, cleared = -Infinity;
  const stats = { pages: 0 };                                 // (The code pages a fetch found: their bytes checked)
  const images = new Map();
  const image = (f, file) => { const n = f.out + file; if (!images.has(n)) images.set(n, fs.readFileSync(n)); return images.get(n); };
  const nameOf = T => { const r = m.taskRam[T]; let s = ''; for (let i = 0; i < 16 && r[k.TA_NAME + i]; i++) s += String.fromCharCode(r[k.TA_NAME + i]); return s; };
  // A task's program's facts into the tables: its stores' and loads' PCs (the main code's, and its swap area's part's
  // in use), its watched bytes (the stacks', and the part's in use: swapped); its code slots forgotten
  function load(T) {
    const G = m.guards, f = facts(names[T] = nameOf(T));
    progs[T] = f;
    parts[T] = null;
    G.store.fill(0, T * 4096, T * 4096 + 4096);
    G.load.fill(0, T * 4096, T * 4096 + 4096);
    G.watch.fill(0, T * 16, T * 16 + 16);
    G.code.fill(0, T * G.slots, (T + 1) * G.slots);
    if (!f) return;
    for (const pc of f.stores.main) G.store[T * 4096 + (pc >> 3)] |= 1 << (pc & 7);
    for (const [pc] of f.loads.main) G.load[T * 4096 + (pc >> 3)] |= 1 << (pc & 7);
    let i = 0;
    const watch = (a, kind, lim) => { const o = T * 16 + 4 * i++; G.watch[o] = a & 255; G.watch[o + 1] = a >> 8; G.watch[o + 2] = kind; G.watch[o + 3] = lim; };
    if (flags & 4) {
      if (f.sp !== null) watch(f.sp + 1, 2, f.floor >> 8);     // (The C stack: its high byte under the floor's page)
      if (f.swsp !== null) watch(f.swsp, 1, f.swdepth);
      if (f.ovsp !== null) watch(f.ovsp, 1, f.depth);
    }
    if ((flags & 10) && f.swap && f.swapped !== null) {       // (The part in the swap area: each written, its PCs in)
      watch(f.swapped, 3, 0);
      partIn(T, f, m.taskRam[T][f.swapped]);
    }
  }
  // Part n in task T's swap area (swap.inc's swapped written: its code copied in next): its stores' and loads' PCs in
  // the tables, the part's before it out (the parts share the area's PCs: a part's instruction at another's place)
  function partIn(T, f, n) {
    const G = m.guards, o = T * 4096, was = parts[T];
    if (was) {
      for (const pc of was[0]) G.store[o + (pc >> 3)] &= ~(1 << (pc & 7));
      for (const e of was[1]) G.load[o + (e[0] >> 3)] &= ~(1 << (e[0] & 7));
    }
    const st = f.stores.parts[n] || [], ld = f.loads.parts[n] || [];
    for (const pc of st) G.store[o + (pc >> 3)] |= 1 << (pc & 7);
    for (const e of ld) G.load[o + (e[0] >> 3)] |= 1 << (e[0] & 7);
    parts[T] = [st, ld];
  }
  // As each run starts: the tasks' programs (a name changed, its tables made again), and the code slots forgotten
  // every 200,000 cycles (a bank given up and used again)
  function sync(mm) {
    m = mm;
    for (let T = 0; T < 16; T++) if (nameOf(T) !== names[T]) load(T);
    if (m.cpu.cyc - cleared >= 200000) { m.guards.code.fill(0); cleared = m.cpu.cyc; }
  }
  const bankOf = (T, slot) => slot < 256 ? m.taskBankMem(T, slot) : slot < 512 ? m.sharedBanks()[slot - 256] : null;
  // The program's code at pc, as the build linked it, in this bank's bytes: an area's code there, the page's part of
  // it the same as the area's file's (OUT: the area, or null)
  function linked(f, mem, pc) {
    const p0 = pc & 0xFF00;
    for (const A of f.areas) {
      if (!A.code.some(([s, e]) => pc >= s && pc < e)) continue;
      const img = image(f, A.file);
      for (const off of A.offs) {
        let same = true;
        for (const [s, e] of A.code) for (let x = Math.max(s, p0); x < Math.min(e, p0 + 256) && same; x++) same = mem[x - 0x8000] === img[off + x - A.start];
        if (same) return A;
      }
    }
    return null;
  }
  // The kernel's code cache's module at pc, in this bank of the task's (its directory's entries in use)
  function cached(T, slot, pc) {
    const seen = new Set();
    for (let b = m.taskRam[T][k.TK_RCACHE]; b < 0xF0 && !seen.has(b); ) {
      seen.add(b);
      const mem = m.taskBankMem(T, b);
      if (!mem) return false;
      if (b === slot) {
        for (let n = 1; n <= k.RB_ENTS; n++) {
          const e = k.RB_DIR - 0x8000 + 16 * n, w = o => mem[e + o] | mem[e + o + 1] << 8;
          if (mem[e + k.RE_GEN] && pc >= w(k.RE_START) && pc < w(k.RE_END)) return true;
        }
        return false;
      }
      b = mem[k.RB_NEXT - 0x8000];
    }
    return false;
  }
  // What a bank is, for the words: the task's own (an overlay's, by _ovbank), or a shared one (the hub's?)
  function bankWords(T, f, slot) {
    if (slot === 512) return 'no bank there (none installed, or the shared RAM off)';
    if (slot >= 256) {                                        // (The hub's: by its header's place, or its magic)
      const key = slot - 256, b = m.sharedBanks()[key];
      let h = m.guards.hubKey, H = null;
      try { H = hub(); } catch (e) { /* (No hub: not Dingus Quest Zero's) */ }
      if (H && h < 0 && b && b[0] === (H.magic & 255) && b[1] === H.magic >> 8) h = key;
      return 'shared bank ' + key + (H && h >= 0 && key >= h && key < h + H.banks ? ' (the hub\'s bank ' + (key - h) + (key === h ? ', its header' : '') + ')' : '');
    }
    let what = 'its bank ' + hx(slot, 2);
    if (f && f.ovbank !== null) for (let i = 0; i < 48; i++) if (m.taskRam[T][f.ovbank + i] === slot) { what += ' (_ovbank[' + i + ']: overlay ' + (i + 1) + '\'s)'; break; }
    return what;
  }
  function hit(mm, kind, a, v) {
    m = mm;
    const T = m.T;
    if (nameOf(T) !== names[T]) load(T);
    const f = progs[T], prog = names[T] || 'task ' + T, partAt = pc => f && f.swap && f.swapped !== null && pc >= f.swap[0] && pc < f.swap[1] ? m.taskRam[T][f.swapped] : 0;
    if (kind === 0) {                                         // (A fetch: the window's page code?)
      const mem = bankOf(T, v), own = v < 256 && KNOWN.some(e => e.kind === 'fetch' && e.prog === prog && e.banks === 'own');
      const A = mem && f ? linked(f, mem, a) : null, bad = A ? A.hub.filter(e => (e.pc & 0xFF00) === (a & 0xFF00)) : [];
      if (bad.length) {                                       // (Its code on this page names the hub's header, which can't
        const e = bad.find(e => e.pc === a);                  //   be in the window as it runs: the page not marked, each
        if (e) {                                              //   fetch from it seen)
          const at = e.fn + ' (' + hx(a) + ', ' + A.area + ')';
          report(e.kind, prog, at, 'task ' + T + ' (' + prog + '): ' + at + ', ' + e.line + ', ' + (e.kind === 'load' ? 'reads' : 'writes') +
            ' the hub\'s header in code run in the window: its own bank\'s there as it runs, never the header');
        }
        return;
      }
      if (mem && (own || A || (v < 256 && cached(T, v, a)))) { m.guards.code[T * m.guards.slots + v] |= 1 << ((a >> 8) & 31); stats.pages++; return; }
      const last = m.cpu.lastPC;
      return report('fetch', prog, where(f, last, partAt(last)), 'task ' + T + ' (' + prog + '): an instruction fetched at ' + hx(a) + ' in the bank window, ' +
        bankWords(T, f, v) + ', no code there; after the instruction at ' + where(f, last, partAt(last)), 'fetch ' + prog + ' ' + bankWords(T, f, v));
    }
    if (kind === 2 && f && a === f.swapped) { partIn(T, f, v); return; }   // (A part copied in: its PCs in the tables)
    const pc = m.cpu.lastPC, part = partAt(pc);
    if (kind === 3) {                                         // (A load that names the hub's header)
      if (!f) return;
      if (!f.lsets) f.lsets = { main: new Map(f.loads.main.map(e => [e[0], e])), parts: {} };
      if (part && !f.lsets.parts[part]) f.lsets.parts[part] = new Map((f.loads.parts[part] || []).map(e => [e[0], e]));
      const e = (part ? f.lsets.parts[part] : f.lsets.main).get(pc);
      if (!e) return;                                         // (Another part's, at the same PC)
      const b = m.taskRam[T][0], H = hub(), off = a - 0x8000, s = f.spans[e[1]];
      if (m.guards.hubKey < 0) {                              // (The hub's header not known yet: this bank, if it's the
        const key = m.U * 16 + (b & 15), bank = b >= 0xF0 ? m.sharedBanks()[key] : null;   //   header (its magic);
        if (bank && bank[0] === (H.magic & 255) && bank[1] === H.magic >> 8) m.guards.hubKey = key;   //   else no hub
        return;                                               //   made yet, none to read)
      }
      let named = false;                                      // (A field of the header its line names, the byte read;
      for (let i = 0; i < s.length; i += 2) if (off >= s[i] && off < s[i + 1]) named = true;
      if (!named) return;                                     //   else another bank's, read on purpose)
      return report('load', prog, where(f, pc, part), 'task ' + T + ' (' + prog + '): ' + where(f, pc, part) + ', ' + f.lines[e[2]] + ', read ' + hx(v, 2) + ' from ' +
        hx(a) + ' (the hub\'s header\'s offset ' + off + ', ' + H.fieldAt(off) + ') with ' + bankWords(T, f, b >= 0xF0 ? 256 + m.U * 16 + (b & 15) : b) +
        ' in the window, not the hub\'s header' + callers(T, f, partAt));
    }
    if (kind === 1) {                                         // (A store that names the hub's header)
      if (!f) return;
      if (!f.sets) f.sets = { main: new Set(f.stores.main), parts: {} };
      if (part && !f.sets.parts[part]) f.sets.parts[part] = new Set(f.stores.parts[part] || []);
      if (!(part ? f.sets.parts[part] : f.sets.main).has(pc)) return;   // (Another part's, at the same PC)
      const b = m.taskRam[T][0], H = hub();
      if (b >= 0xF0) {
        const key = m.U * 16 + (b & 15), bank = m.sharedBanks()[key], off = a - 0x8000;
        if (off < 2 && v === (off ? H.magic >> 8 : H.magic & 255)) { m.guards.hubKey = key; return; }   // (The hub made: its magic)
        if (m.guards.hubKey < 0 && bank && bank[0] === (H.magic & 255) && bank[1] === H.magic >> 8) { m.guards.hubKey = key; return; }
      }
      return report('store', prog, where(f, pc, part), 'task ' + T + ' (' + prog + '): ' + where(f, pc, part) + ' wrote ' + hx(v, 2) + ' to ' + hx(a) +
        ' (the hub\'s header\'s offset ' + (a - 0x8000) + ') with ' + bankWords(T, f, b >= 0xF0 ? 256 + m.U * 16 + (b & 15) : b) + ' in the window, not the hub\'s header');
    }
    if (f && a === f.sp + 1) {                                // (A C stack of the library's own put in sp: not under the floor)
      const sp = v << 8 | m.taskRam[T][f.sp];
      if (f.alt.some(([s, e]) => sp >= s && sp <= e)) return;
    }
    const what = f && a === f.sp + 1 ? 'the C stack below its floor ' + hx(f.floor) + ' (sp\'s high byte ' + hx(v, 2) + ')'
      : f && a === f.swsp ? 'the swap stack ' + v + ' deep, past its ' + f.swdepth + ' entries (swsp)'
        : f && a === f.ovsp ? 'the overlay calls\' stack ' + v + ' deep, past its ' + f.depth + ' entries (ovsp)' : hx(a) + ' written ' + hx(v, 2);
    return report(f && a === f.sp + 1 ? 'stack' : 'swap', prog, where(f, pc, part), 'task ' + T + ' (' + prog + '): ' + what + ', by ' + where(f, pc, part),
      'watch ' + prog + ' ' + hx(a) + ' ' + where(f, pc, part));
  }
  // The calls that reached the instruction, for the words: the 6502 stack's return addresses, each after a JSR in the
  // task's main RAM (the first few found; a swapped part's as its part's in the area now, so a guess)
  function callers(T, f, partAt) {
    const r = m.taskRam[T], out = [];
    for (let i = m.cpu.S + 1; i < 0xFF && out.length < 3; i++) {
      const at = ((r[0x100 + i] | r[0x101 + i] << 8) - 2) & 0xFFFF;
      if (at >= 0x200 && at < 0x8000 && r[at] === 0x20) { out.push(where(f, at, partAt(at))); i++; }
    }
    return out.length ? '; called from ' + out.join(', from ') : '';
  }
  // A hit: noted once by its key (a fetch's by its bank, a store's and a watch's by the instruction), counted after
  function report(kind, prog, at, text, key) {
    const fn = at.replace(/[+ ].*$/, ''), known = expect.includes(kind) ? { why: 'the test\'s own, made on purpose' }
      : KNOWN.find(e => e.kind === kind && e.prog === prog && e.at === fn);
    if (!key) key = kind + ' ' + prog + ' ' + at;
    const e = hits.find(x => x.key === key);
    if (e) { e.count++; return; }
    hits.push({ key, kind, prog, text: text + ' at cycle ' + m.cpu.cyc, count: 1, known: known ? known.why : null });
    if (!known && mode !== 'note' && !m.cpu.halted) m.cpu.halted = 'guard: ' + text + ' at cycle ' + m.cpu.cyc;
  }
  function judge() {
    const failures = [], notes = [];
    for (const e of hits) {
      if (e.known) notes.push('guard (known: ' + e.known + '): ' + e.text + (e.count > 1 ? ' (' + e.count + ' times)' : ''));
      else if (!(m && m.cpu.halted === 'guard: ' + e.text)) failures.push('guard: ' + e.text + (e.count > 1 ? ' (' + e.count + ' times)' : ''));
    }
    for (const kind of expect) if (!hits.some(e => e.kind === kind)) failures.push('guard: no ' + kind + ' hit, though the test made one');
    return { failures, notes };
  }
  const state = () => ({ hubKey: m ? m.guards.hubKey : -1, hits: hits.map(e => Object.assign({}, e)) });
  const restore = s => { names.fill(null); parts.fill(null); cleared = -Infinity; hits.splice(0, hits.length, ...s.hits); if (m) m.guards.hubKey = s.hubKey; };
  return { flags, sync, hit, attach: mm => { m = mm; }, judge, hits: () => hits, stats: () => stats, state, restore };
}

module.exports = { createGuard, facts, programs, kernel, where, KNOWN };
