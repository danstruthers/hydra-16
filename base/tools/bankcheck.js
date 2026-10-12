#!/usr/bin/env node
// ****************************************************************************
// bankcheck.js - a module of two banks or more: no instruction in one bank uses a label in another directly.  All of
// a module's banks are at $A000-$DFFF, so the linker can't tell a jsr to another bank's routine from one to this
// bank's: it would run whatever this bank has there.  From the link's debug information (ld65 --dbgfile): each
// label's segment (CODE, RODATA, HEADER: bank 1; CODEn, RODATAn: bank n), and each line that refers to it, its own
// segment and its text.  A line in one bank that refers to a label in another is fine if it only takes the label's
// address (a directive, .word or .byte; an immediate operand, #<label or #>label) or calls it the far way (FAR1,
// FAR2, FARN, FARX, or a module's own: C2 ...); an instruction with any other operand there is reported.  Code in RAM
// (DATA, run there) is every bank's, so it isn't checked.
//
// A table (.word label, .byte <label) in one bank of a label in another is listed with --tables: right for a table of
// far calls (hylang's builtins), wrong for a jump table (jmp (table,x) jumps in its own bank).
//
// Usage: node tools/bankcheck.js FILE.dbg [--far MACRO,...] [--tables]   (exit status 1 if there's a direct use)
// From Node: check(dbgText, { far: [...] }) gives [{ file, line, text, label, from, to }] (an empty list: all well)
'use strict';
const fs = require('fs');

// Macros that only take a label's address: far calls; immediates (LDR, LDAX, LDW); hylang's table of builtins (BI).  A
// module adds its own with a comment, "bankcheck: far NAME ...", in any of its files; and "bankcheck: copied" marks a
// file whose code is copied to RAM and run with another bank in place (hylang's native code's templates): not checked
const FAR = ['FAR1', 'FAR2', 'FARN', 'FARX', 'C2', 'NMFAR2', 'LDR', 'LDAX', 'LDW', 'BI'];
const OPS = new Set(('adc and asl bbr0 bbr1 bbr2 bbr3 bbr4 bbr5 bbr6 bbr7 bbs0 bbs1 bbs2 bbs3 bbs4 bbs5 bbs6 bbs7 bcc bcs ' +
  'beq bit bmi bne bpl bra brk bvc bvs clc cld cli clv cmp cpx cpy dec dex dey eor inc inx iny jmp jsr lda ldx ldy lsr ' +
  'nop ora pha php phx phy pla plp plx ply rmb0 rmb1 rmb2 rmb3 rmb4 rmb5 rmb6 rmb7 rol ror rti rts sbc sec sed sei ' +
  'smb0 smb1 smb2 smb3 smb4 smb5 smb6 smb7 sta stp stx sty stz tax tay trb tsb tsx txa txs tya wai').split(' '));

// A segment's bank: 1 for CODE, RODATA and HEADER, n for CODEn and RODATAn; 0 for RAM's (DATA, BSS, ZEROPAGE ...)
function bankOf(seg) {
  if (/^(CODE|RODATA|HEADER)$/.test(seg)) return 1;
  const m = /^(CODE|RODATA)([2-8])$/.exec(seg);
  return m ? +m[2] : 0;
}

function parse(text) {
  const recs = { file: {}, line: {}, span: {}, seg: {}, sym: {} };
  for (const raw of text.split(/\r?\n/)) {
    const tab = raw.indexOf('\t');
    if (tab < 0) continue;
    const kind = raw.slice(0, tab);
    if (!recs[kind]) continue;
    const r = {};
    for (const m of raw.slice(tab + 1).matchAll(/(\w+)=("(?:[^"\\]|\\.)*"|[^,]*)/g)) r[m[1]] = m[2][0] === '"' ? m[2].slice(1, -1) : m[2];
    recs[kind][r.id] = r;
  }
  return recs;
}

// What a source line does with a label: 'address' (a directive, an immediate operand, a far call), else 'direct'
function use(text, far) {
  let s = text.replace(/;.*/, '').trim();
  s = s.replace(/^[@A-Za-z_][\w@]*:\s*/, '').replace(/^:\s*/, '');           // (A label first)
  if (/^\.(word|addr|dword|faraddr|lobytes|hibytes)\b/i.test(s) || /^\.byte\b.*[<>]/i.test(s)) return 'table';
  if (!s || s[0] === '.') return 'address';
  const [op, ...rest] = s.split(/\s+/), arg = rest.join(' ');
  if (far.includes(op)) return 'address';
  if (OPS.has(op.toLowerCase())) return arg.startsWith('#') ? 'address' : 'direct';
  return 'macro ' + op;                                                      // (A macro of the module's own)
}

function check(text, opt = {}) {
  const far = [...FAR, ...(opt.far || [])], d = parse(text), lines = {}, copied = new Set(), out = [];
  const src = id => {
    const f = d.file[id];
    if (!lines[id]) lines[id] = fs.existsSync(f.name) ? fs.readFileSync(f.name, 'latin1').split(/\r?\n/) : [];
    return lines[id];
  };
  for (const id of Object.keys(d.file)) for (const l of src(id)) {          // (The files' own: bankcheck: ...)
    const m = /bankcheck:\s*(far|copied)\b([\w ]*)/.exec(l);
    if (m && m[1] === 'far') far.push(...m[2].trim().split(/\s+/).filter(Boolean));
    else if (m) copied.add(id);
  }
  const segOfLine = l => { const sp = l.span && d.span[l.span.split('+')[0]]; return sp ? d.seg[sp.seg].name : null; };
  for (const sym of Object.values(d.sym)) {
    const target = sym.type === 'imp' ? d.sym[sym.exp] : sym;
    if (!target || target.seg === undefined || target.type !== 'lab' || !sym.ref) continue;
    const to = bankOf(d.seg[target.seg].name);
    if (!to) continue;                                                       // (A label in RAM: every bank's)
    for (const id of new Set(sym.ref.split('+'))) {
      const l = d.line[id], seg = l && segOfLine(l);
      if (!seg || copied.has(l.file)) continue;
      const from = bankOf(seg);
      if (!from || from === to) continue;
      const text = src(l.file)[+l.line - 1] || '';
      const u = use(text, far);
      if (u !== 'address' && (u !== 'table' || opt.tables)) out.push({ file: d.file[l.file].name, line: +l.line, text: text.trim(), label: target.name, from, to, use: u });
    }
  }
  return out.sort((a, b) => (a.file + a.line).localeCompare(b.file + b.line));
}

if (require.main === module) {
  const args = process.argv.slice(2), file = args[0];
  const fi = args.indexOf('--far'), far = fi >= 0 ? args[fi + 1].split(',') : [], tables = args.includes('--tables');
  if (!file) { console.error('usage: bankcheck.js FILE.dbg [--far MACRO,...] [--tables]'); process.exit(2); }
  const all = check(fs.readFileSync(file, 'latin1'), { far, tables }), bad = all.filter(b => b.use !== 'table');
  for (const b of all) console.log(b.file + ':' + b.line + ': bank ' + b.from + ' uses ' + b.label + ' (bank ' + b.to + ') ' + (b.use === 'direct' ? 'directly' : b.use === 'table' ? 'in a table (a jump table\'s entries must be its bank\'s)' : 'in ' + b.use) + ': ' + b.text);
  process.exit(bad.length ? 1 : 0);
}
module.exports = { check, bankOf };
