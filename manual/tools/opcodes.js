#!/usr/bin/env node
// opcodes.js - Appendix D's tables, from the W65C02S's instruction table (base/lib/w65c02.inc: each opcode's name and
// addressing mode), written as Markdown on stdout: each instruction (what it does, the flags it changes, its opcode
// in each addressing mode, and its cycles: "+" for one more when its indexing crosses a page), then the 16 x 16
// matrix of the opcodes.
// Usage: node manual/tools/opcodes.js [--update manual/app-d-65c02.md]   (--update: between the file's markers)
'use strict';
const fs = require('fs');
const path = require('path');
const src = fs.readFileSync(path.join(__dirname, '..', '..', 'base', 'lib', 'w65c02.inc'), 'utf8').split(String.fromCharCode(13)).join('');

// The .byte rows after a label, as numbers (or the names' string)
function bytesAfter(label) {
  const i = src.indexOf('\n' + label + ':');
  const out = [];
  for (const line of src.slice(i + label.length + 2).split('\n')) {
    const m = /^\s*(?:[a-z_]+:)?\s*\.byte\s+(.*)$/.exec(line);
    if (!m) { if (out.length) break; else continue; }
    for (const t of m[1].split(',')) { const s = t.trim(); if (s) out.push(s); }
  }
  return out;
}
// Each opcode's cycles (the emulator's table, from WDC's data sheet: base/sim/lib/cpu65c02.js's CYC), and those
// that take one more when their indexing crosses a page (PAGE_X)
const cpu = fs.readFileSync(path.join(__dirname, '..', '..', 'base', 'sim', 'lib', 'cpu65c02.js'), 'utf8');
const cyc = cpu.slice(cpu.indexOf('const CYC'), cpu.indexOf('.split', cpu.indexOf('const CYC'))).match(/'[0-9]{16}'/g)
  .map(s => s.slice(1, -1)).join('').split('').map(Number);
const pageX = new Set(cpu.slice(cpu.indexOf('const PAGE_X'), cpu.indexOf('PAGE_X[o] = 1')).match(/0x[0-9A-F]{2}/g).map(Number));
if (cyc.length !== 256) throw new Error('opcodes.js: no cycle table in cpu65c02.js');
const cycles = op => cyc[op] + (pageX.has(op) ? '+' : '');
const names = bytesAfter('names').map(s => s.split('"').join('')).join('').match(/.../g);
const opName = bytesAfter('op_name').map(Number);
const opMode = bytesAfter('op_mode').map(Number);
// The modes as the tables write them (Appendix D's table D-2 lists them)
const SHORT = ['', 'a', '#', 'zp', 'zp,x', 'zp,y', '(zp,x)', '(zp),y', '(zp)', 'abs', 'abs,x', 'abs,y', '(abs)', '(abs,x)', 'rel', 'zp,rel'];
const hx = n => n.toString(16).toUpperCase().padStart(2, '0');
const BITS = ['rmb', 'smb', 'bbr', 'bbs'];
const mnemonic = op => { const n = names[opName[op]]; return BITS.includes(n) ? n + ((op >> 4) & 7) : n; };
const defined = op => opMode[op] < 16;

// What each does, and the flags it changes (of N V B D I Z C)
const WHAT = {
  adc: ['Add with carry: A = A + M + C', 'NVZC'], and: ['A = A AND M', 'NZ'], asl: ['Shift left: C gets bit 7, bit 0 gets 0', 'NZC'],
  bbr: ['Branch if bit *n* of a zero page byte is 0', ''], bbs: ['Branch if bit *n* of a zero page byte is 1', ''],
  bcc: ['Branch if C = 0', ''], bcs: ['Branch if C = 1', ''], beq: ['Branch if Z = 1 (equal)', ''],
  bit: ['Test bits: Z = 1 if (A AND M) is 0; N and V get bits 7 and 6 of M (but not with #)', 'NVZ'],
  bmi: ['Branch if N = 1 (minus)', ''], bne: ['Branch if Z = 0 (not equal)', ''], bpl: ['Branch if N = 0 (plus)', ''],
  bra: ['Branch always', ''], brk: ['Break: a software interrupt', 'BDI'], bvc: ['Branch if V = 0', ''], bvs: ['Branch if V = 1', ''],
  clc: ['C = 0', 'C'], cld: ['D = 0: binary arithmetic', 'D'], cli: ['I = 0: interrupts allowed', 'I'], clv: ['V = 0', 'V'],
  cmp: ['Compare A with M (A - M, kept only in the flags)', 'NZC'], cpx: ['Compare X with M', 'NZC'], cpy: ['Compare Y with M', 'NZC'],
  dec: ['M = M - 1 (or A = A - 1)', 'NZ'], dex: ['X = X - 1', 'NZ'], dey: ['Y = Y - 1', 'NZ'], eor: ['A = A XOR M', 'NZ'],
  inc: ['M = M + 1 (or A = A + 1)', 'NZ'], inx: ['X = X + 1', 'NZ'], iny: ['Y = Y + 1', 'NZ'], jmp: ['Jump', ''],
  jsr: ['Jump to a subroutine (pushing the return address)', ''], lda: ['A = M', 'NZ'], ldx: ['X = M', 'NZ'], ldy: ['Y = M', 'NZ'],
  lsr: ['Shift right: C gets bit 0, bit 7 gets 0', 'NZC'], nop: ['Nothing', ''], ora: ['A = A OR M', 'NZ'],
  pha: ['Push A', ''], php: ['Push P', ''], phx: ['Push X', ''], phy: ['Push Y', ''], pla: ['Pull A', 'NZ'],
  plp: ['Pull P', 'NVDIZC'], plx: ['Pull X', 'NZ'], ply: ['Pull Y', 'NZ'], rmb: ['Clear bit *n* of a zero page byte', ''],
  rol: ['Rotate left: C gets bit 7, bit 0 gets C', 'NZC'], ror: ['Rotate right: C gets bit 0, bit 7 gets C', 'NZC'],
  rti: ['Return from an interrupt (pulling P, then the address)', 'NVDIZC'], rts: ['Return from a subroutine', ''],
  sbc: ['Subtract with borrow: A = A - M - (1 - C)', 'NVZC'], sec: ['C = 1', 'C'], sed: ['D = 1: decimal (BCD) arithmetic', 'D'],
  sei: ['I = 1: interrupts held off', 'I'], smb: ['Set bit *n* of a zero page byte', ''], sta: ['M = A', ''],
  stp: ['Stop the processor, until a reset', ''], stx: ['M = X', ''], sty: ['M = Y', ''], stz: ['M = 0', ''],
  tax: ['X = A', 'NZ'], tay: ['Y = A', 'NZ'], trb: ['Test and reset bits: Z as BIT\'s; then M = M AND NOT A', 'Z'],
  tsb: ['Test and set bits: Z as BIT\'s; then M = M OR A', 'Z'], tsx: ['X = S', 'NZ'], txa: ['A = X', 'NZ'],
  txs: ['S = X', ''], tya: ['A = Y', 'NZ'], wai: ['Wait for an interrupt', ''],
};

// Each instruction's opcodes, by mode (RMB, SMB, BBR and BBS: one for each bit, 0 to 7)
const byName = new Map();
for (let op = 0; op < 256; op++) {
  if (!defined(op)) continue;
  const base = names[opName[op]];
  if (!byName.has(base)) byName.set(base, []);
  byName.get(base).push(op);
}
let out = '| Instruction | What it does | Flags | Opcodes |\n|---|---|---|---|\n';
for (const base of [...byName.keys()].sort()) {
  const ops = byName.get(base);
  const [what, flags] = WHAT[base] || ['?', ''];
  const codes = BITS.includes(base) ?
    ops.map(op => mnemonic(op).toUpperCase() + ' $' + hx(op) + ' (' + cycles(op) + ')').join(', ') :
    ops.map(op => (SHORT[opMode[op]] ? SHORT[opMode[op]] + ' ' : '') + '$' + hx(op) + ' (' + cycles(op) + ')').join(', ');
  out += '| `' + base.toUpperCase() + (BITS.includes(base) ? '`*n* | ' : '` | ') + what + ' | ' +
    (flags ? flags.split('').join(' ') : '-') + ' | ' + codes + ' |\n';
}
out += '\n## D.4 The opcodes in order\n\nEach square is an opcode: its row is the opcode\'s first hex digit, and its column the second. So `$A9`, row\n' +
  '`Ax` and column `x9`, is `LDA #`, taking 2 cycles. An empty square is an opcode the W65C02S doesn\'t use: it does\n' +
  'nothing, as a `NOP` does.\n\n<div class="opmatrix">\n\n';
out += '| | ' + [...Array(16).keys()].map(c => 'x' + c.toString(16).toUpperCase()).join(' | ') + ' |\n';
out += '|---' + '|---'.repeat(16) + '|\n';
for (let r = 0; r < 16; r++) {
  const cells = [];
  for (let c = 0; c < 16; c++) {
    const op = r * 16 + c;
    if (!defined(op)) { cells.push(''); continue; }
    const m = SHORT[opMode[op]];
    cells.push(mnemonic(op).toUpperCase() + '<br>' + (m || '&nbsp;') + '<br>' + cycles(op));
  }
  out += '| **' + r.toString(16).toUpperCase() + 'x** | ' + cells.join(' | ') + ' |\n';
}
out += '\n</div>\n';
emit(out, 'tools/opcodes.js');

// --update FILE: the text between FILE's markers (<!-- generated: TAG --> and <!-- end generated: TAG -->) made this
// output; else it's written on stdout
function emit(text, tag) {
  const i = process.argv.indexOf('--update');
  if (i < 0) { process.stdout.write(text); return; }
  const f = process.argv[i + 1], a = '<!-- generated: ' + tag + ' -->\n\n', b = '\n<!-- end generated: ' + tag + ' -->';
  const s = fs.readFileSync(f, 'utf8').split(String.fromCharCode(13)).join(''), p = s.indexOf(a), q = s.indexOf(b, p);
  if (p < 0 || q < 0) throw new Error(f + ': no markers for ' + tag);
  fs.writeFileSync(f, s.slice(0, p + a.length) + text.replace(/\n+$/, '\n') + s.slice(q));
}
