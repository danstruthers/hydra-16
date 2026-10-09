#!/usr/bin/env node
// ****************************************************************************
// apigen.js - HydraOS's half of what the system calls' specification implies (../base/spec/api.def, errors.def: the
// base's tools/apigen.js reads them, and makes the kernel's jump table, the error codes and texts, the assembly SDK's
// hydra.inc and api.json in base/obj, which this runs first), and everything the number libraries' (spec/numbers.def)
// and the asm library's (spec/asm.def) imply, so the calls are written down once
// (docs/design/reimplementation-from-scratch.md, principle P9):
//   obj/gen/errnames.inc  the error codes' names, lower case without E_ (hylang's error codes: :noent ...), a table's macro
//   obj/sdk/c/hydracalls.h   the calls' addresses, the error codes and the constants, for C (HY_ before each name:
//                         cc65's headers have some of them)
//   obj/sdk/c/oserrmap.inc   the C library's map from the error codes to errno (errors.def's last column)
//   obj/gen/api.md        the reference: every call, its registers, its errors (and its HyForth word)
//   obj/gen/forthsys.inc  HyForth's sys- words, for its Hydra library (forthlib/hydra.s)
//   obj/gen/hylsys.inc    hylang's sys- functions, the calls' records (modules/hylang/hysys.inc), from the hl: lines
//   obj/gen/hydra.fs      the constants and error codes for HyForth, a library on the ROM disk (/lib/forth)
//   obj/gen/basicsys.inc  BASIC's SYS "NAME": the calls a program makes, by name (modules/basic/machine.inc)
//   obj/sdk/numbers.inc   the number libraries' entries (NUM_ADD ...), constants and call macros, for programs in
//                         assembly
//   obj/sdk/c/numdefs.h   their constants, for C (num.h includes it)
//   obj/sdk/asmlib.inc, obj/sdk/c/asmdefs.h, obj/gen/asm_jt.inc   the same for the asm library (spec/asm.def: the
//                         W65C02S's instructions as as writes them; asm.h includes asmdefs.h)
//   obj/gen/numbers_jt.inc, math_jt.inc   each number library's jump table (its module includes it)
//   obj/gen/pow10.inc     the powers of 10 to 10^24 the registers' r_pow10 copies (nmreg.inc)
//   obj/gen/atantab.inc   atan(1/m) for m 2 to 15 at 192 bits, the math library's ATAN's (mttrig.inc)
//   obj/gen/numconst.inc  the math library's constants (pi, log 2, log 10) at NC_BITS bits, from sim/tools/numref.js:
//                         the numbers library's INIT puts them in the state's cache (docs/design/plans/NUMSPEED.md)
//
// Usage: node tools/apigen.js [ROOT]       (ROOT: the reborn folder; default: this file's parent; the base's is ../base)
// From Node: require('./apigen.js').generate(root) gives { calls, errors, consts, groups, numbers, asm }; readNumbers(file)
// reads spec/numbers.def (or asm.def) alone.
'use strict';
const fs = require('fs');
const path = require('path');
const base = require('../../base/tools/apigen.js');   // (The spec's reader, the kernel's outputs, the writers' helpers)
const { readApi, readErrors, write, header, pad, hx, num, fail, CRLF } = base;
const BASE = path.join(__dirname, '..', '..', 'base');

// The number libraries (spec/numbers.def): each library's entries in its slots' order (its jump table at $A030, after
// its header), with the prefix of their names in assembly (NUM_ADD, MATH_SQRT); and their constants
const NUM_PREFIX = { numbers: 'NUM_', math: 'MATH_', asm: 'ASM_' }, NUM_TABLE = 0xA030;
function readNumbers(file) {
  const libs = [], entries = [], consts = [];
  let entry = null;
  fs.readFileSync(file, 'utf8').split(/\r?\n/).forEach((raw, i) => {
    const line = raw.replace(/\s+$/, ''), n = i + 1;
    if (!line.trim() || line.trim().startsWith('#')) return;
    let m;
    if ((m = line.match(/^library\s+(\w+)\s+"([^"]*)"$/))) {
      if (!NUM_PREFIX[m[1]]) fail(file, n, 'library ' + m[1] + ': no prefix for its entries\' names (apigen.js, NUM_PREFIX)');
      libs.push({ name: m[1], doc: m[2], entries: [] }); entry = null;
    } else if ((m = line.match(/^entry\s+(\w+)\s+(\w+)\s+impl=(\w+)$/))) {
      const lib = libs.find(l => l.name === m[2]);
      if (!lib) fail(file, n, 'no library ' + m[2]);
      if (lib.entries.find(e => e.name === m[1])) fail(file, n, 'entry ' + m[1] + ' twice');
      entry = { name: m[1], lib: lib.name, impl: m[3], symbol: NUM_PREFIX[lib.name] + m[1], addr: NUM_TABLE + 3 * lib.entries.length,
        in: [], out: [], errors: [], names: '', doc: [], line: n };
      lib.entries.push(entry); entries.push(entry);
    } else if ((m = line.match(/^const\s+(\w+)\s+(\$?[0-9A-Fa-f]+)(?:\s+"([^"]*)")?$/))) {
      consts.push({ name: m[1], value: num(m[2]), text: m[2], doc: m[3] || '' }); entry = null;
    } else if ((m = line.match(/^\s+(in|out|errors|names|doc):\s*(.*)$/))) {
      if (!entry) fail(file, n, m[1] + ': outside an entry');
      const v = m[2].trim();
      if (m[1] === 'errors') { if (v !== '-') entry.errors.push(...v.split(/[\s,]+/).filter(Boolean)); }
      else if (m[1] === 'names') entry.names = v;
      else if (v !== '-') entry[m[1]].push(v);
    } else fail(file, n, 'what is "' + line.trim() + '"?');
  });
  for (const e of entries) for (const x of e.errors) if (!consts.find(k => k.name === x)) fail(file, e.line, e.name + ': no error ' + x);
  return { libs, entries, consts };
}

function errNames(errors) {
  let s = header(';', 'errnames.inc - the error codes\' names, lower case without E_ (hylang\'s error codes, :noent ...)');
  s += '; ERR_NAMES: the table, where it\'s used.  Entries: the code, then the name, zero-terminated; a code of 0 ends it.' + CRLF + CRLF;
  s += '.macro ERR_NAMES' + CRLF;
  for (const e of errors) s += '            .byte       ' + hx(e.code, 2) + ', "' + e.name.replace(/^E_/, '').toLowerCase() + '", 0' + CRLF;
  s += '            .byte       0' + CRLF + '.endmacro' + CRLF;
  return s;
}

// The number libraries, for programs in assembly: each entry's address (XCALL's r15), and the constants
function numbersInc(nums) {
  let s = header(';', 'numbers.inc - the number libraries\' entries, constants and call macros, for programs in assembly');
  s += '; A call (spec/numbers.def): XCALL, r15 the entry (its address here), r14 its library\'s bank (MODINFO finds it,' + CRLF;
  s += '; by the library\'s name), r13 the libraries\' bank (a bank of the program\'s, which NUM_INIT fills); r0-r3 the' + CRLF;
  s += '; operands, the result\'s place and its room; .A/.X the result\'s length, C = 0; or C = 1 and .A an error (NE_).' + CRLF;
  s += '; NUMCALL NUM_ADD (an entry of the numbers library\'s) and MATHCALL MATH_SQRT (the math library\'s) make one, .A,' + CRLF;
  s += '; .X and .Y the entry\'s, from three bytes of the program\'s: num_bank (r13), num_mod and math_mod (r14), which' + CRLF;
  s += '; numlib.s\'s num_open sets (or the program\'s own code).' + CRLF;
  for (const [m, mod] of [['NUMCALL', 'num_mod'], ['MATHCALL', 'math_mod']]) {
    s += CRLF + '.macro ' + pad(m, 12) + 'entry' + CRLF;
    for (const l of ['pha', 'lda #<(entry)', 'sta r15', 'lda #>(entry)', 'sta r15 + 1', 'lda ' + mod, 'sta r14', 'lda num_bank', 'sta r13', 'pla',
      'jsr XCALL'])
      s += ('            ' + pad(l.split(' ')[0], 12) + l.split(' ').slice(1).join(' ')).trimEnd() + CRLF;
    s += '.endmacro' + CRLF;
  }
  for (const l of nums.libs) {
    s += CRLF + '; ---- ' + l.name + ': ' + l.doc + CRLF;
    for (const e of l.entries) {
      s += pad(e.symbol, 16) + '= ' + hx(e.addr, 4) + '       ; ' + (e.in.join(' ') || '-') + CRLF;
      if (e.out.length) s += pad('', 30) + '; -> ' + e.out.join(' ') + CRLF;
    }
  }
  s += CRLF + '; ---- constants' + CRLF;
  for (const k of nums.consts) s += pad(k.name, 16) + '= ' + pad(k.text, 12) + (k.doc ? '; ' + k.doc : '') + CRLF;
  return s;
}

// A number library's jump table (its module includes it right after its header): a jmp to each entry's code
// The constants the math library caches (st_const: pi, log 2, log 10, numbers/nmbank.inc's order), each at NC_BITS
// bits (cut toward 0, within an ulp): CONST_BYTES bytes each, least first
const NC_BITS = 824, CONST_BYTES = 104;
function numConst() {
  const R = require('../sim/tools/numref.js');
  let s = header(';', 'numconst.inc - the math library\'s constants, pi, log 2 and log 10, at NC_BITS bits (sim/tools/numref.js\'s)');
  s += 'NC_BITS         = ' + NC_BITS + CRLF + '.rodata' + CRLF + 'nm_consts:' + CRLF;
  for (const [name, v] of [['pi', R.piFixed(NC_BITS)], ['log 2', R.ln2Fixed(NC_BITS)], ['log 10', R.ln10Fixed(NC_BITS)]]) {
    const b = [];
    for (let x = v, k = 0; k < CONST_BYTES; k++, x >>= 8n) b.push(Number(x & 255n));
    if (v >> BigInt(8 * CONST_BYTES)) throw new Error('numconst: ' + name + ' past ' + CONST_BYTES + ' bytes');
    s += '; ' + name + CRLF;
    for (let k = 0; k < CONST_BYTES; k += 16) s += '            .byte       ' + b.slice(k, k + 16).map(x => hx(x, 2)).join(', ') + CRLF;
  }
  s += '.code' + CRLF;
  return s;
}

// The powers of 10 below P10_TAB, for r_pow10 (nmreg.inc, both libraries' banks: NM_RODATA): each's length, its
// bytes' offset in p10_dat, its bytes least first
const P10_TAB = 25;
function pow10Inc() {
  let s = header(';', 'pow10.inc - the powers of 10 below P10_TAB for r_pow10 (modules/numbers/nmreg.inc): lengths, offsets, bytes');
  const dat = [], off = [], len = [];
  for (let k = 0; k < P10_TAB; k++) {
    let v = 10n ** BigInt(k);
    off.push(dat.length);
    let n = 0;
    while (v) { dat.push(Number(v & 255n)); v >>= 8n; n++; }
    len.push(n);
  }
  if (dat.length > 256) throw new Error('pow10: past 256 bytes');
  s += 'P10_TAB         = ' + P10_TAB + CRLF + '            NM_RODATA' + CRLF;
  s += 'p10_len:    .byte       ' + len.join(', ') + CRLF + 'p10_off:    .byte       ' + off.join(', ') + CRLF + 'p10_dat:' + CRLF;
  for (let k = 0; k < dat.length; k += 16) s += '            .byte       ' + dat.slice(k, k + 16).map(x => hx(x, 2)).join(', ') + CRLF;
  s += '            NM_CODE' + CRLF;
  return s;
}

// atan(1/m), m 2 to 15, at AT_BITS bits (cut toward 0: made at 64 bits more by numref.js's series, then shifted):
// AT_BITS / 8 bytes each, least first
const AT_BITS = 192;
function atanTab() {
  const R = require('../sim/tools/numref.js');
  let s = header(';', 'atantab.inc - atan(1/m) for m 2 to 15 at AT_BITS bits, for ATAN (modules/math/mttrig.inc)');
  s += 'AT_BITS         = ' + AT_BITS + CRLF + '            NM_RODATA' + CRLF + 'at_tab:' + CRLF;
  for (let m = 2; m <= 15; m++) {
    const v = R.atanInvExact(m, AT_BITS);
    const b = [];
    for (let x = v, k = 0; k < AT_BITS / 8; k++, x >>= 8n) b.push(Number(x & 255n));
    s += '; m = ' + m + CRLF;
    for (let k = 0; k < b.length; k += 12) s += '            .byte       ' + b.slice(k, k + 12).map(x => hx(x, 2)).join(', ') + CRLF;
  }
  s += '            NM_CODE' + CRLF;
  return s;
}

function numbersJt(lib) {
  let s = header(';', lib.name + '_jt.inc - the ' + lib.name + ' library\'s jump table, after its header');
  s += CRLF + '.code' + CRLF + '.assert     * = ' + hx(NUM_TABLE, 4) + ', lderror, "The ' + lib.name + ' library\'s jump table isn\'t right after its header"' + CRLF;
  for (const e of lib.entries) s += '            jmp         ' + pad(e.impl, 24) + '; ' + hx(e.addr, 4) + ' ' + e.name + CRLF;
  return s;
}

// C: the calls (their slots: hy_call), the error codes and the constants, each name with HY_ before it
function cHeader(api, errors) {
  const cx = v => '0x' + v.toString(16).toUpperCase();
  const def = (name, value, doc) => ('#define ' + pad('HY_' + name, 24) + pad(value, 10) + (doc ? '/* ' + doc.replace(/\*\//g, '* /') + ' */' : '')).trimEnd() + CRLF;
  let s = '/*' + CRLF + '** hydracalls.h - the Hydra-16\'s system calls, error codes and constants, for C (cc65).  Made by tools/apigen.js' + CRLF;
  s += '** from spec/: don\'t edit.  hydra.h includes it.  A call\'s name is its slot in the jump table, for hy_call; the' + CRLF;
  s += '** registers are in spec/api.def and the reference (/rom/doc/api.md).  Each name has HY_ before it (cc65\'s' + CRLF;
  s += '** headers have some of them: O_RDWR, say).' + CRLF + '*/' + CRLF + CRLF + '#ifndef _HYDRACALLS_H' + CRLF + '#define _HYDRACALLS_H' + CRLF;
  for (const g of api.groups) {
    if (!g.calls.length) continue;
    s += CRLF + '/* ---- ' + g.name + ': ' + g.doc + ' */' + CRLF;
    for (const c of g.calls) s += def(c.name, cx(c.addr), '');
  }
  s += CRLF + '/* ---- error codes (_oserror) */' + CRLF;
  for (const e of errors) s += def(e.name, cx(e.code), e.text);
  s += CRLF + '/* ---- constants */' + CRLF;
  for (const k of api.consts) s += def(k.name, k.text.startsWith('$') ? '0x' + k.text.slice(1) : k.text, k.doc);
  s += CRLF + '#endif' + CRLF;
  return s;
}

// The asm library, for programs in assembly: its entries' addresses, its constants and its call macro
function asmInc(lib) {
  let s = header(';', 'asmlib.inc - the asm library\'s entries, constants and call macro, for programs in assembly');
  s += '; A call (spec/asm.def): XCALL, r15 the entry (its address here), r14 the library\'s bank (MODINFO finds it, by' + CRLF;
  s += '; the name "asm").  ASMCALL ASM_DIS makes one, .A, .X and .Y the entry\'s, r14 from a byte of the program\'s,' + CRLF;
  s += '; asm_mod.' + CRLF;
  s += CRLF + '.macro ' + pad('ASMCALL', 12) + 'entry' + CRLF;
  for (const l of ['pha', 'lda #<(entry)', 'sta r15', 'lda #>(entry)', 'sta r15 + 1', 'lda asm_mod', 'sta r14', 'pla', 'jsr XCALL'])
    s += ('            ' + pad(l.split(' ')[0], 12) + l.split(' ').slice(1).join(' ')).trimEnd() + CRLF;
  s += '.endmacro' + CRLF;
  for (const l of lib.libs) {
    s += CRLF + '; ---- ' + l.name + ': ' + l.doc + CRLF;
    for (const e of l.entries) {
      s += pad(e.symbol, 16) + '= ' + hx(e.addr, 4) + '       ; ' + (e.in.join(' ') || '-') + CRLF;
      if (e.out.length) s += pad('', 30) + '; -> ' + e.out.join(' ') + CRLF;
    }
  }
  s += CRLF + '; ---- constants' + CRLF;
  for (const k of lib.consts) s += pad(k.name, 16) + '= ' + pad(k.text, 12) + (k.doc ? '; ' + k.doc : '') + CRLF;
  return s;
}

// C: a library's constants (num.h, asm.h include them), named as in assembly
function cLibHeader(nums, file, what, from, by) {
  const def = (name, value, doc) => ('#define ' + pad(name, 24) + pad(value, 10) + (doc ? '/* ' + doc.replace(/\*\//g, '* /') + ' */' : '')).trimEnd() + CRLF;
  const guard = '_' + file.toUpperCase().replace('.', '_');
  let s = '/*' + CRLF + '** ' + file + ' - ' + what + ', for C (cc65).  Made by tools/apigen.js from spec/' + from + ':' + CRLF;
  s += '** don\'t edit.  ' + by + ' includes it.' + CRLF + '*/' + CRLF + CRLF + '#ifndef ' + guard + CRLF + '#define ' + guard + CRLF + CRLF;
  for (const k of nums.consts) s += def(k.name, k.text.startsWith('$') ? '0x' + k.text.slice(1) : k.text, k.doc);
  s += CRLF + '#endif' + CRLF;
  return s;
}

// C: the number libraries' constants (num.h includes them), named as in assembly
function cNumHeader(nums) {
  const def = (name, value, doc) => ('#define ' + pad(name, 24) + pad(value, 10) + (doc ? '/* ' + doc.replace(/\*\//g, '* /') + ' */' : '')).trimEnd() + CRLF;
  let s = '/*' + CRLF + '** numdefs.h - the number libraries\' constants, for C (cc65).  Made by tools/apigen.js from spec/numbers.def:' + CRLF;
  s += '** don\'t edit.  num.h includes it.' + CRLF + '*/' + CRLF + CRLF + '#ifndef _NUMDEFS_H' + CRLF + '#define _NUMDEFS_H' + CRLF + CRLF;
  for (const k of nums.consts) s += def(k.name, k.text.startsWith('$') ? '0x' + k.text.slice(1) : k.text, k.doc);
  s += CRLF + '#endif' + CRLF;
  return s;
}

// The C library's errno for each error code (oserror.s includes it: .byte code, errno)
function oserrMap(errors) {
  let s = header(';', 'oserrmap.inc - the error codes\' errno, for the C library\'s __osmaperrno');
  for (const e of errors) s += '            .byte       ' + pad(e.name + ',', 16) + e.errno + CRLF;
  return s;
}

// ---- HyForth (modules/forth): a sys- word for each call a program makes (not a server's, nor a debugging one; nor
// NOTIFY, as forth has its own note handler; nor XCALL, which runs code at an address), the call's registers as stack
// items in the specification's order, the first deepest: in, then out, then an ior (0, or -512 less the error code)
// if the call can fail
const FORTH_GROUPS_OUT = ['server', 'dbg'], FORTH_CALLS_OUT = ['NOTIFY', 'XCALL'];
const forthCalls = api => api.calls.filter(c => !FORTH_GROUPS_OUT.includes(c.group) && !FORTH_CALLS_OUT.includes(c.name));
const forthName = c => 'sys-' + c.name.toLowerCase().replace(/_/g, '-');

// The registers a call's in: or out: lines name, in order, each { code, text }: rN ($0N: 16 bits), rN and the next
// ($1N: a double, its low cell rN; "r0, r1" or "r0/r1"), .A, .X, .Y ($20-$22: a byte), .A/.X ($23: 16 bits)
function regsOf(c, lines) {
  let t = lines.join(' ');
  for (let u; (u = t.replace(/\([^()]*\)/g, '')) !== t; ) t = u;
  const regs = [];
  for (const part of t.split(';')) {
    const p = part.trim();
    let m;
    if ((m = p.match(/^r(\d+)\s*[,\/]\s*r(\d+)\s*=/))) {
      if (+m[2] !== +m[1] + 1 || +m[2] > 15) fail('spec/api.def', c.line, c.name + ': "' + m[0] + '": a register and the next?');
      regs.push({ code: 0x10 + +m[1], text: 'r' + m[1] + '/r' + m[2] });
    } else if ((m = p.match(/^r(\d+)\s*=/))) {
      if (+m[1] > 15) fail('spec/api.def', c.line, c.name + ': no register r' + m[1]);
      regs.push({ code: +m[1], text: 'r' + m[1] });
    } else if (/^\.A\/\.X\s*=/.test(p)) regs.push({ code: 0x23, text: '.A/.X' });
    else if ((m = p.match(/^\.([AXY])\s*=/))) regs.push({ code: 0x20 + 'AXY'.indexOf(m[1]), text: '.' + m[1] });
  }
  return regs;
}

// A call's sys- word: its stack effect, as text
function forthEffect(c) {
  const ins = regsOf(c, c.in), outs = regsOf(c, c.out);
  return '( ' + [...ins.map(r => r.text), '--', ...outs.map(r => r.text), ...(c.errors.length ? ['ior'] : [])].join(' ') + ' )';
}

function forthSys(api) {
  let s = header(';', 'forthsys.inc - HyForth\'s sys- words, for its Hydra library (forthlib/hydra.s)');
  s += '; A word each, its header (HEADER: fdefs.inc), then its code: jsr sys_call and its descriptor, the call\'s address;' + CRLF;
  s += '; flags ($80: it gives an ior); its inputs, a count and their registers, the top\'s first; its outputs, a count' + CRLF;
  s += '; and their registers, the first pushed first.  A register: $0N rN, $1N rN and the next (a double), $20 .A, $21' + CRLF;
  s += '; .X, $22 .Y, $23 .A/.X.' + CRLF;
  for (const c of forthCalls(api)) {
    const n = forthName(c), ins = regsOf(c, c.in), outs = regsOf(c, c.out);
    if (n.length > 31) fail('spec/api.def', c.line, n + ': a Forth name is 31 characters at most');
    const codes = [c.errors.length ? 0x80 : 0, ins.length, ...ins.reverse().map(r => r.code), outs.length, ...outs.map(r => r.code)];
    s += CRLF + '            HEADER      "' + n + '", 0' + CRLF;
    s += '            jsr         sys_call' + CRLF;
    s += '            .word       ' + pad(hx(c.addr, 4), 36) + '; ' + forthEffect(c) + CRLF;
    s += '            .byte       ' + codes.map(v => hx(v, 2)).join(', ') + CRLF;
  }
  return s;
}

// ---- BASIC (modules/basic): SYS "NAME" calls one by its name.  basicsys.inc: each call a program makes (forth's),
// its name (upper case, its last character's bit 7 set: machine.inc's htasc) and its address; a 0 after the last
function basicSys(api) {
  let s = header(';', 'basicsys.inc - BASIC\'s SYS "NAME": the calls a program makes, by name (modules/basic/machine.inc)');
  s += 'SYS_NAMES:' + CRLF;
  for (const c of forthCalls(api)) s += '            htasc       "' + c.name + '"' + CRLF + '            .word       ' + hx(c.addr, 4) + CRLF;
  return s + '            .byte       0' + CRLF;
}

// ---- hylang (modules/hylang): a sys- function for each call a program makes (forth's), from its hl: line (the
// format: spec/api.def's header).  hylsys.inc: each call's record, read by hysys.inc's sys: its name (lower case, no
// sys-, a 0), its address, the arguments it needs and the most it takes, its inputs (a count, then each one's kind,
// register and parameter: bytes' count register, a byte; buf's and io's size, a word) and its outputs (a count,
// then each one's kind and register).  A register as forth's: $0N rN, $1N rN and the next, $20 .A, $21 .X, $22 .Y,
// $23 .A/.X
const HL_IN = { n: 1, 'n?': 2, i: 3, s: 4, 's?': 5, 'args?': 6, 'map?': 7, bytes: 8, bufn: 9, buf: 10, count: 11, size: 12, io: 13, stat: 14 };
const HL_OPT = ['n?', 's?', 'args?', 'map?', 'io'], HL_NOARG = ['bufn', 'buf', 'size'], HL_ROOM = ['s', 's?', 'args?', 'map?', 'bytes', 'bufn', 'io', 'stat'];
const HL_OUT = { reg: 1, buf: 2, z: 3, bufreg: 4, stat: 5 };
const HL_ARGS_MAX = 8;          // (hysys.inc's: sys's most arguments, less its name)
const hlName = c => 'sys-' + c.name.toLowerCase().replace(/_/g, '-');

// A register's code, from its name in an hl: line
function hlReg(c, t) {
  let m;
  if (t === '.A/.X') return 0x23;
  if ((m = t.match(/^\.([AXY])$/))) return 0x20 + 'AXY'.indexOf(m[1]);
  if ((m = t.match(/^r(\d+)\/r(\d+)$/)) && +m[2] === +m[1] + 1 && +m[2] <= 15) return 0x10 + +m[1];
  if ((m = t.match(/^r(\d+)$/)) && +m[1] <= 15) return +m[1];
  fail('spec/api.def', c.hlLine, c.name + ': no register "' + t + '"');
}

// A call's hl: line read and checked: { ins: [{ kind, reg, param }], outs: [{ kind, reg }], min, max }
function hlOf(c, consts) {
  const where = msg => fail('spec/api.def', c.hlLine, c.name + ': ' + msg);
  const [inText, outText, more] = c.hl.split('->');
  if (outText === undefined || more !== undefined) where('an hl: line has one ->');
  const inRegs = regsOf(c, c.in).map(r => r.code), outRegs = regsOf(c, c.out).map(r => r.code);
  const size = t => t.split('+').reduce((v, p) => {
    p = p.trim();
    if (/^\d+$/.test(p)) return v + +p;
    const k = consts.find(x => x.name === p);
    if (!k) where('no const ' + p);
    return v + k.value;
  }, 0);
  const ins = [], outs = [];
  let buffer = null, args = 0, min = 0;
  for (const tok of inText.trim().split(/\s+/).filter(Boolean)) {
    const m = tok.match(/^([^=]+)=(\w+\??)(?:\(([^)]*)\))?$/);
    if (!m) where('what is "' + tok + '"?');
    const reg = hlReg(c, m[1]);
    if (!inRegs.includes(reg)) where(m[1] + ' isn\'t among in:\'s registers');
    let kind = m[2], param = null;
    if (kind === 'buf' && m[3] !== undefined) kind = 'bufn';
    if (!(kind in HL_IN)) where('no kind ' + kind);
    if (['bytes', 'bufn', 'io'].includes(kind) !== (m[3] !== undefined)) where(kind + ': its parameter?');
    if (kind === 'bytes') { param = hlReg(c, m[3]); if (!inRegs.includes(param) || param > 15) where(m[3] + ': a count register, rN'); }
    if (kind === 'bufn' || kind === 'io') param = size(m[3]);
    if (kind === 'i' && (reg & 0xF0) !== 0x10) where('i: a 32-bit register\'s');
    if (['s', 's?', 'args?', 'map?', 'bytes', 'bufn', 'buf', 'io', 'stat'].includes(kind) && reg > 15) where(kind + ': a pointer, rN');
    if (HL_ROOM.includes(kind) && buffer && buffer.kind === 'buf') where(kind + ' after buf: buf is the room that\'s left');
    if (['bufn', 'buf', 'io'].includes(kind)) { if (buffer) where('one buffer at most'); buffer = { kind }; }
    if ((kind === 'count' || kind === 'size') && (!buffer || buffer.kind !== 'buf' || buffer.sized)) where(kind + ': after buf, once');
    if (kind === 'count' || kind === 'size') buffer.sized = true;
    if (!HL_NOARG.includes(kind)) { args++; if (!HL_OPT.includes(kind)) min = args; }
    ins.push({ kind, reg, param });
  }
  if (buffer && buffer.kind === 'buf' && !buffer.sized) where('buf: no count or size');
  if (args > HL_ARGS_MAX) where('more than ' + HL_ARGS_MAX + ' arguments');
  for (const tok of outText.trim().split(/\s+/).filter(Boolean)) {
    const m = tok.match(/^buf(?:=(.+))?$/);
    if (m) {
      if (!buffer) where(tok + ': no buffer');
      if (m[1] === undefined) outs.push({ kind: 'buf', reg: 0 });
      else if (m[1] === 'z' || m[1] === 'stat') outs.push({ kind: m[1], reg: 0 });
      else {
        const reg = hlReg(c, m[1]);
        if (!outRegs.includes(reg)) where(m[1] + ' isn\'t among out:\'s registers');
        outs.push({ kind: 'bufreg', reg });
      }
      if (m[1] === 'stat' && (buffer.kind !== 'bufn' || ins.find(x => x.kind === 'bufn').param !== consts.find(x => x.name === 'SR_SIZE').value)) where('buf=stat: buf(SR_SIZE)\'s');
    } else {
      const reg = hlReg(c, tok);
      if (!outRegs.includes(reg)) where(tok + ' isn\'t among out:\'s registers');
      outs.push({ kind: 'reg', reg });
    }
  }
  return { ins, outs, min, max: args };
}

function hylSys(api) {
  let s = header(';', 'hylsys.inc - hylang\'s sys- functions, the calls\' records (hysys.inc\'s sys reads them)');
  s += '; A record each: its name (lower case, no sys-, a 0), its address, the arguments it needs and the most it takes;' + CRLF;
  s += '; its inputs, a count, then each one\'s kind (SK_*), register and parameter (SK_BYTES\'s: its count\'s register;' + CRLF;
  s += '; SK_BUFN\'s and SK_IO\'s: a size, a word); its outputs, a count, then each one\'s kind (SO_*) and register.  A' + CRLF;
  s += '; register: $0N rN, $1N rN and the next (32 bits), $20 .A, $21 .X, $22 .Y, $23 .A/.X.' + CRLF + CRLF;
  for (const [k, v] of Object.entries(HL_IN)) s += pad('SK_' + k.replace('?', 'Q').toUpperCase(), 16) + '= ' + v + CRLF;
  for (const [k, v] of Object.entries(HL_OUT)) s += pad('SO_' + k.toUpperCase(), 16) + '= ' + v + CRLF;
  const calls = forthCalls(api);
  s += pad('SYS_CALLS', 16) + '= ' + calls.length + CRLF;
  s += pad('SYS_ARGS', 16) + '= ' + HL_ARGS_MAX + CRLF + CRLF;
  s += 'sys_calls:' + CRLF;
  calls.forEach((c, i) => { s += '            .word       hs_' + i + CRLF; });
  calls.forEach((c, i) => {
    const h = hlOf(c, api.consts), n = c.name.toLowerCase().replace(/_/g, '-');
    if (n.length > 27) fail('spec/api.def', c.hlLine, n + ': a sys- name is 31 characters at most');
    const ins = [h.ins.length], outs = [h.outs.length];
    for (const x of h.ins) {
      ins.push(HL_IN[x.kind], x.reg);
      if (x.kind === 'bytes') ins.push(x.param);
      if (x.kind === 'bufn' || x.kind === 'io') ins.push(x.param & 0xFF, x.param >> 8);
    }
    for (const x of h.outs) outs.push(HL_OUT[x.kind], x.reg);
    s += CRLF + pad('hs_' + i + ':', 12) + '.byte       "' + n + '", 0' + CRLF;
    s += '            .word       ' + pad(hx(c.addr, 4), 36) + '; ' + hlName(c) + ': ' + c.hl + CRLF;
    s += '            .byte       ' + [h.min, h.max].join(', ') + CRLF;
    s += '            .byte       ' + ins.map(v => hx(v, 2)).join(', ') + CRLF;
    s += '            .byte       ' + outs.map(v => hx(v, 2)).join(', ') + CRLF;
  });
  return s;
}

// hydra.fs: the constants a program uses (not the servers', nor the kernel's own addresses), and the error codes,
// as constants, a library (its own word list, hydra); every name in lower case, as forth's are.  LF line ends, as
// the ROM disk's files have, and short lines (a file's line is 128 at most)
const FORTH_CONSTS_OUT = /^(r\d+$|RQ_|R_|RF_|TASK_INBOX$|TASK_PATH$|TASK_EVENT$|PROG_ZP|IRQ_RESCHED$|LINE_|HX_|TM_|INIT_TASK$)/;
function forthLib(api, errors) {
  const LF = '\n', line = (v, name, doc) => ('$' + v.toString(16).toUpperCase().padStart(2, '0') + ' constant ' + pad(name.toLowerCase(), 16) + '\\ ' + doc).slice(0, 110).trimEnd() + LF;
  let s = '\\ hydra.fs - the Hydra-16\'s constants and error codes, for HyForth: require hydra.fs (/lib/forth\'s).  Made by' + LF;
  s += '\\ tools/apigen.js from spec/: don\'t edit.  A library: its words in a word list of its own, hydra, which goes first' + LF;
  s += '\\ in the search order.  An error code\'s ior (a file word\'s, a sys- word\'s) is -512 less it.' + LF;
  s += LF + 'require search.fl            \\ (library)' + LF + 'library hydra' + LF;
  s += LF + '\\ ---- constants' + LF;
  for (const k of api.consts) if (!FORTH_CONSTS_OUT.test(k.name)) s += line(k.value, k.name, k.doc);
  s += LF + '\\ ---- error codes' + LF;
  for (const e of errors) s += line(e.code, e.name, e.text);
  s += LF + 'end-library' + LF;
  return s;
}

function apiMd(api, errors) {
  let s = '## **The system calls**' + CRLF + CRLF;
  s += 'Made by `tools/apigen.js` from `spec/api.def` and `spec/errors.def`: don\'t edit.  The rules for every call are in [conventions.md](../../docs/conventions.md#the-abi): `.A`, `.X`, `.Y` and `r0-r15` in and out, 16-bit results in `.A`/`.X`, and C = 1 with the error code in `.A` on failure.' + CRLF;
  for (const g of api.groups) {
    s += CRLF + '### **' + g.name + '** (`' + hx(g.base, 4) + '`, ' + g.slots + ' slots)' + CRLF + CRLF + g.doc + '.' + CRLF;
    if (!g.calls.length) { s += CRLF + '(No calls yet.)' + CRLF; continue; }
    s += CRLF + '| Call | Address | In | Out | Errors | Waits | HyForth | hylang |' + CRLF + '| :--- | :------ | :- | :-- | :----- | :---- | :------ | :----- |' + CRLF;
    const esc = t => t.replace(/\|/g, '\\|'), fc = forthCalls(api);
    for (const c of g.calls) s += '| `' + c.name + '` | `' + hx(c.addr, 4) + '` | ' + esc(c.in.join(' ') || '-') + ' | ' + esc(c.out.join(' ') || '-') + ' | ' + esc(c.errorsText || '-') + ' | ' + (c.blocks || '-') + ' | ' + (fc.includes(c) ? '`' + forthName(c) + ' ' + forthEffect(c) + '`' : '-') + ' | ' + (fc.includes(c) ? '`' + esc(hlName(c) + ' ' + c.hl) + '`' : '-') + ' |' + CRLF;
    s += CRLF;
    for (const c of g.calls) s += '* **`' + c.name + '`**: ' + c.doc.join(' ') + CRLF;
  }
  s += CRLF + '### **Error codes**' + CRLF + CRLF + '| Code | Name | Text |' + CRLF + '| :--- | :--- | :--- |' + CRLF;
  for (const e of errors) s += '| `' + hx(e.code, 2) + '` | `' + e.name + '` | ' + e.text + ' |' + CRLF;
  return s;
}

function generate(root) {
  base.generate(BASE);                                      // (The kernel's and the assembly SDK's: base/obj)
  const api = readApi(path.join(BASE, 'spec', 'api.def'));
  const errors = readErrors(path.join(BASE, 'spec', 'errors.def'));
  const fc = forthCalls(api);
  for (const c of api.calls) {
    if (fc.includes(c) && c.hl === undefined) fail('spec/api.def', c.line, c.name + ': no hl: line (hylang\'s ' + hlName(c) + ')');
    if (!fc.includes(c) && c.hl !== undefined) fail('spec/api.def', c.hlLine, c.name + ': an hl: line, and not a call a program makes');
  }
  const gen = path.join(root, 'obj', 'gen');
  write(path.join(gen, 'errnames.inc'), errNames(errors));
  write(path.join(root, 'obj', 'sdk', 'c', 'hydracalls.h'), cHeader(api, errors));
  write(path.join(root, 'obj', 'sdk', 'c', 'oserrmap.inc'), oserrMap(errors));
  write(path.join(gen, 'api.md'), apiMd(api, errors));
  write(path.join(gen, 'forthsys.inc'), forthSys(api));
  write(path.join(gen, 'hylsys.inc'), hylSys(api));
  write(path.join(gen, 'hydra.fs'), forthLib(api, errors));
  write(path.join(gen, 'basicsys.inc'), basicSys(api));
  const nums = readNumbers(path.join(root, 'spec', 'numbers.def'));
  write(path.join(root, 'obj', 'sdk', 'numbers.inc'), numbersInc(nums));
  write(path.join(root, 'obj', 'sdk', 'c', 'numdefs.h'), cNumHeader(nums));
  for (const l of nums.libs) write(path.join(gen, l.name + '_jt.inc'), numbersJt(l));
  const asm = readNumbers(path.join(root, 'spec', 'asm.def'));
  write(path.join(root, 'obj', 'sdk', 'asmlib.inc'), asmInc(asm));
  write(path.join(root, 'obj', 'sdk', 'c', 'asmdefs.h'), cLibHeader(asm, 'asmdefs.h', 'the asm library\'s constants', 'asm.def', 'asm.h'));
  for (const l of asm.libs) write(path.join(gen, l.name + '_jt.inc'), numbersJt(l));
  write(path.join(gen, 'numconst.inc'), numConst());
  write(path.join(gen, 'pow10.inc'), pow10Inc());
  write(path.join(gen, 'atantab.inc'), atanTab());
  return { groups: api.groups, calls: api.calls, consts: api.consts, errors, numbers: nums, asm };
}

module.exports = { generate, readNumbers };

if (require.main === module) {
  try {
    const r = generate(path.resolve(process.argv[2] || path.join(__dirname, '..')));
    console.log('apigen: ' + r.calls.length + ' calls in ' + r.groups.length + ' groups, ' + r.errors.length + ' error codes');
  } catch (e) { console.error('apigen: ' + e.message); process.exit(1); }
}
