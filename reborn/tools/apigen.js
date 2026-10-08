#!/usr/bin/env node
// ****************************************************************************
// apigen.js - makes everything the system calls' specification implies (spec/api.def, spec/errors.def) and the
// number libraries' (spec/numbers.def), so the calls are written down once (docs/design/reimplementation-from-scratch.md,
// principle P9):
//   obj/gen/jumptable.s   the jump table on BIOS ROM page 0 ($F800 up), a jmp per slot (spare slots: K_NOSYS)
//   obj/gen/errors.inc    the error codes, for the kernel
//   obj/gen/errtext.s     their texts, for ERRSTR
//   obj/gen/errnames.inc  their names, lower case without E_ (hylang's error codes: :noent ...), a table's macro
//   obj/sdk/hydra.inc     the calls' addresses, the error codes and the constants, for programs in assembly
//   obj/sdk/c/hydracalls.h   the same for C (HY_ before each name: cc65's headers have some of them)
//   obj/sdk/c/oserrmap.inc   the C library's map from the error codes to errno (errors.def's last column)
//   obj/gen/api.md        the reference: every call, its registers, its errors (and its HyForth word)
//   obj/gen/api.json      the same as data (the emulator names calls with it: sim/run.js --trace-calls)
//   obj/gen/forthsys.inc  HyForth's sys- words, for its Hydra library (forthlib/hydra.s)
//   obj/gen/hylsys.inc    hylang's sys- functions, the calls' records (modules/hylang/hysys.inc), from the hl: lines
//   obj/gen/hydra.fs      the constants and error codes for HyForth, a library on the ROM disk (/lib/forth)
//   obj/gen/basicsys.inc  BASIC's SYS "NAME": the calls a program makes, by name (modules/basic/machine.inc)
//   obj/sdk/numbers.inc   the number libraries' entries (NUM_ADD ...), constants and call macros, for programs in
//                         assembly
//   obj/sdk/c/numdefs.h   their constants, for C (num.h includes it)
//   obj/gen/numbers_jt.inc, math_jt.inc   each number library's jump table (its module includes it)
//
// Usage: node tools/apigen.js [ROOT]       (ROOT: the reborn folder; default: this file's parent)
// From Node: require('./apigen.js').generate(root) gives { calls, errors, consts, groups, numbers }; readNumbers(file)
// reads spec/numbers.def alone.
'use strict';
const fs = require('fs');
const path = require('path');

const CRLF = '\r\n';
const hx = (v, n) => '$' + v.toString(16).toUpperCase().padStart(n, '0');
const num = s => (s.startsWith('$') ? parseInt(s.slice(1), 16) : parseInt(s, 10));

function fail(file, line, msg) { throw new Error(file + (line ? ':' + line : '') + ': ' + msg); }

// ---- reading the specification
function readApi(file) {
  const groups = [], calls = [], consts = [];
  let call = null;
  fs.readFileSync(file, 'utf8').split(/\r?\n/).forEach((raw, i) => {
    const line = raw.replace(/\s+$/, ''), n = i + 1;
    if (!line.trim() || line.trim().startsWith('#')) return;
    let m;
    if ((m = line.match(/^group\s+(\w+)\s+(\$[0-9A-Fa-f]+)\s+(\d+)\s+"([^"]*)"$/))) {
      groups.push({ name: m[1], base: num(m[2]), slots: +m[3], doc: m[4], calls: [] }); call = null;
    } else if ((m = line.match(/^call\s+(\w+)\s+(\w+)\s+impl=(\w+)(\s+far)?$/))) {
      const g = groups.find(x => x.name === m[2]);
      if (!g) fail(file, n, 'no group ' + m[2]);
      if (calls.find(c => c.name === m[1])) fail(file, n, 'call ' + m[1] + ' twice');
      call = { name: m[1], group: g.name, impl: m[3], far: !!m[4], in: [], out: [], errors: [], blocks: '', doc: [], line: n };
      call.slot = g.calls.length;
      if (call.slot >= g.slots) fail(file, n, 'group ' + g.name + ' is full (' + g.slots + ' slots)');
      call.addr = g.base + 3 * call.slot;
      g.calls.push(call); calls.push(call);
    } else if ((m = line.match(/^const\s+(\w+)\s+(\$?[0-9A-Fa-f]+)(?:\s+"([^"]*)")?$/))) {
      consts.push({ name: m[1], value: num(m[2]), text: m[2], doc: m[3] || '' }); call = null;
    } else if ((m = line.match(/^\s+(in|out|errors|blocks|doc|hl):\s*(.*)$/))) {
      if (!call) fail(file, n, m[1] + ': outside a call');
      const v = m[2].trim();
      if (m[1] === 'errors') { if (v !== '-') call.errors.push(...v.replace(/\([^)]*\)/g, '').split(/[\s,]+/).filter(Boolean)); call.errorsText = (call.errorsText ? call.errorsText + ' ' : '') + v; }
      else if (m[1] === 'blocks') call.blocks = v;
      else if (m[1] === 'hl') { if (call.hl !== undefined) fail(file, n, call.name + ': two hl: lines'); call.hl = v; call.hlLine = n; }
      else if (v !== '-') call[m[1]].push(v);
    } else fail(file, n, 'what is "' + line.trim() + '"?');
  });
  // The groups: in address order, each after the last, inside the jump table's room ($F800-$FCFF)
  groups.sort((a, b) => a.base - b.base);
  for (let i = 0; i < groups.length; i++) {
    const g = groups[i], end = g.base + 3 * g.slots;
    if (i === 0 && g.base !== 0xF800) fail(file, 0, 'the first group must start at $F800');
    if (i + 1 < groups.length && end > groups[i + 1].base) fail(file, 0, 'group ' + g.name + ' runs into ' + groups[i + 1].name);
    if (end > 0xFD00) fail(file, 0, 'group ' + g.name + ' runs into the COMMON block ($FD00)');
  }
  return { groups, calls, consts };
}

function readErrors(file) {
  const errors = [];
  fs.readFileSync(file, 'utf8').split(/\r?\n/).forEach((raw, i) => {
    const line = raw.trim();
    if (!line || line.startsWith('#')) return;
    const m = line.match(/^error\s+(E_\w+)\s+(\$[0-9A-Fa-f]{2})\s+"([^"]+)"\s+(E[A-Z]+)$/);
    if (!m) fail(file, i + 1, 'what is "' + line + '"?');
    const code = num(m[2]);
    if (errors.find(e => e.code === code || e.name === m[1])) fail(file, i + 1, m[1] + ': its name or code is used twice');
    if (m[3].length > 31) fail(file, i + 1, m[1] + ': its text is over 31 characters (ERRSTR\'s buffer is 32 bytes)');
    errors.push({ name: m[1], code, text: m[3], errno: m[4] });
  });
  return errors;
}

// The number libraries (spec/numbers.def): each library's entries in its slots' order (its jump table at $A030, after
// its header), with the prefix of their names in assembly (NUM_ADD, MATH_SQRT); and their constants
const NUM_PREFIX = { numbers: 'NUM_', math: 'MATH_' }, NUM_TABLE = 0xA030;
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

// ---- writing
const header = (c, what) => c + ' ' + what + '.  Made by tools/apigen.js from spec/: don\'t edit' + CRLF;
function write(file, text) {
  fs.mkdirSync(path.dirname(file), { recursive: true });
  if (!fs.existsSync(file) || fs.readFileSync(file, 'utf8') !== text) fs.writeFileSync(file, text);
}
const pad = (s, n) => (s.length >= n ? s + ' ' : s + ' '.repeat(n - s.length));

function jumptable(api) {
  const impls = [...new Set(api.calls.map(c => c.impl))];
  let s = header(';', 'jumptable.s - the system calls\' jump table, on BIOS ROM page 0');
  s += '; Each slot is a jmp to the kernel\'s routine; a spare slot is a jmp to K_NOSYS (E_NOSYS).  A call whose' + CRLF;
  s += '; routine is on another BIOS ROM page ("far" in the specification) jumps to a stub here, on page 0, which' + CRLF;
  s += '; gives K_FARJMP the routine\'s page and address (kernel/far.s): .A, .X, .Y and C pass both ways.' + CRLF + CRLF;
  s += '.import     K_NOSYS' + (api.calls.some(c => c.far) ? ', K_FARJMP' : '') + CRLF;
  for (let i = 0; i < impls.length; i += 6) s += '.import     ' + impls.slice(i, i + 6).join(', ') + CRLF;
  s += CRLF + '.segment "JUMPTABLE"' + CRLF;
  let at = 0xF800;
  for (const g of api.groups) {
    if (g.base > at) s += CRLF + '            .repeat     ' + (g.base - at) / 3 + CRLF + '            jmp         K_NOSYS' + CRLF + '            .endrepeat' + CRLF;
    s += CRLF + '; ---- ' + g.name + ': ' + g.doc + CRLF;
    s += 'API_' + g.name.toUpperCase() + ':' + CRLF;
    for (const c of g.calls)
      s += '            jmp         ' + pad(c.far ? 'FAR_' + c.name : c.impl, 24) + '; ' + hx(c.addr, 4) + ' ' + c.name + (c.far ? ' (far)' : '') + CRLF;
    if (g.slots > g.calls.length) s += '            .repeat     ' + (g.slots - g.calls.length) + CRLF + '            jmp         K_NOSYS' + CRLF + '            .endrepeat' + CRLF;
    s += '.assert     API_' + g.name.toUpperCase() + ' = ' + hx(g.base, 4) + ', lderror, "The jump table\'s group ' + g.name + ' has moved"' + CRLF;
    at = g.base + 3 * g.slots;
  }
  s += CRLF + '.export     API_END' + CRLF + 'API_END:' + CRLF;
  const far = api.calls.filter(c => c.far);
  if (far.length) {
    s += CRLF + '; ---- The far calls\' stubs (6 bytes each, on page 0)' + CRLF + '.segment "KCODE"' + CRLF;
    for (const c of far)
      s += 'FAR_' + c.name + ':' + CRLF + '            jsr         K_FARJMP' + CRLF +
        '            .byte       <.bank(' + c.impl + ')' + CRLF + '            .word       ' + c.impl + CRLF;
  }
  return s;
}

function errorsInc(errors) {
  let s = header(';', 'errors.inc - the error codes (C = 1, the code in .A)');
  for (const e of errors) s += pad(e.name, 16) + '= ' + hx(e.code, 2) + '         ; ' + e.text + CRLF;
  return s;
}

function errText(errors) {
  let s = header(';', 'errtext.s - the error codes\' texts, for ERRSTR');
  s += '; Entries: the code, then the text, zero-terminated; a code of 0 ends the table.' + CRLF + CRLF;
  s += '.export     ERR_TEXTS' + CRLF + '.segment "KRODATA"' + CRLF + 'ERR_TEXTS:' + CRLF;
  for (const e of errors) s += '            .byte       ' + hx(e.code, 2) + ', "' + e.text + '", 0' + CRLF;
  s += '            .byte       0' + CRLF;
  return s;
}

function errNames(errors) {
  let s = header(';', 'errnames.inc - the error codes\' names, lower case without E_ (hylang\'s error codes, :noent ...)');
  s += '; ERR_NAMES: the table, where it\'s used.  Entries: the code, then the name, zero-terminated; a code of 0 ends it.' + CRLF + CRLF;
  s += '.macro ERR_NAMES' + CRLF;
  for (const e of errors) s += '            .byte       ' + hx(e.code, 2) + ', "' + e.name.replace(/^E_/, '').toLowerCase() + '", 0' + CRLF;
  s += '            .byte       0' + CRLF + '.endmacro' + CRLF;
  return s;
}

function sdkInc(api, errors) {
  let s = header(';', 'hydra.inc - the Hydra-16\'s system calls, error codes and constants, for programs in assembly');
  s += '; The rules (reborn/docs/conventions.md): call with jsr; .A, .X, .Y and r0-r15 in and out; 16-bit results' + CRLF;
  s += '; in .A (low) and .X (high); C = 0 success, C = 1 failure with the error code in .A.  Calls change .A, .X, .Y,' + CRLF;
  s += '; r0-r15 and the flags; never $22-$7F (the program\'s zero page).' + CRLF;
  for (const g of api.groups) {
    if (!g.calls.length) continue;
    s += CRLF + '; ---- ' + g.name + ': ' + g.doc + CRLF;
    for (const c of g.calls) {
      s += pad(c.name, 16) + '= ' + hx(c.addr, 4) + '       ; ' + (c.in.join(' ') || '-') + CRLF;
      if (c.out.length) s += pad('', 30) + '; -> ' + c.out.join(' ') + CRLF;
    }
  }
  s += CRLF + '; ---- error codes' + CRLF;
  for (const e of errors) s += pad(e.name, 16) + '= ' + hx(e.code, 2) + '         ; ' + e.text + CRLF;
  s += CRLF + '; ---- constants' + CRLF;
  for (const k of api.consts) s += pad(k.name, 16) + '= ' + pad(k.text, 12) + (k.doc ? '; ' + k.doc : '') + CRLF;
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
  const api = readApi(path.join(root, 'spec', 'api.def'));
  const errors = readErrors(path.join(root, 'spec', 'errors.def'));
  for (const c of api.calls) for (const e of c.errors) if (!errors.find(x => x.name === e)) fail('spec/api.def', c.line, c.name + ': no error ' + e);
  const fc = forthCalls(api);
  for (const c of api.calls) {
    if (fc.includes(c) && c.hl === undefined) fail('spec/api.def', c.line, c.name + ': no hl: line (hylang\'s ' + hlName(c) + ')');
    if (!fc.includes(c) && c.hl !== undefined) fail('spec/api.def', c.hlLine, c.name + ': an hl: line, and not a call a program makes');
  }
  const gen = path.join(root, 'obj', 'gen');
  write(path.join(gen, 'jumptable.s'), jumptable(api));
  write(path.join(gen, 'errors.inc'), errorsInc(errors));
  write(path.join(gen, 'errtext.s'), errText(errors));
  write(path.join(gen, 'errnames.inc'), errNames(errors));
  write(path.join(root, 'obj', 'sdk', 'hydra.inc'), sdkInc(api, errors));
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
  write(path.join(gen, 'api.json'), JSON.stringify({
    calls: api.calls.map(c => ({ name: c.name, addr: c.addr, group: c.group, in: c.in.join(' '), out: c.out.join(' '), errors: c.errors, blocks: c.blocks })),
    errors, consts: api.consts.map(k => ({ name: k.name, value: k.value })),
  }, null, 1) + '\n');
  return { groups: api.groups, calls: api.calls, consts: api.consts, errors, numbers: nums };
}

module.exports = { generate, readNumbers };

if (require.main === module) {
  try {
    const r = generate(path.resolve(process.argv[2] || path.join(__dirname, '..')));
    console.log('apigen: ' + r.calls.length + ' calls in ' + r.groups.length + ' groups, ' + r.errors.length + ' error codes');
  } catch (e) { console.error('apigen: ' + e.message); process.exit(1); }
}
