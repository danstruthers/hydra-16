#!/usr/bin/env node
// ****************************************************************************
// apigen.js - makes everything the system calls' specification implies (spec/api.def, spec/errors.def), so the
// calls are written down once (docs/reimplementation-from-scratch.md, principle P9):
//   obj/gen/jumptable.s   the jump table on BIOS ROM page 0 ($F800 up), a jmp per slot (spare slots: K_NOSYS)
//   obj/gen/errors.inc    the error codes, for the kernel
//   obj/gen/errtext.s     their texts, for ERRSTR
//   obj/sdk/hydra.inc     the calls' addresses, the error codes and the constants, for programs in assembly
//   obj/gen/api.md        the reference: every call, its registers, its errors
//   obj/gen/api.json      the same as data (the emulator names calls with it: sim/run.js --trace-calls)
//
// Usage: node tools/apigen.js [ROOT]       (ROOT: the reborn folder; default: this file's parent)
// From Node: require('./apigen.js').generate(root) gives { calls, errors, consts, groups }.
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
    } else if ((m = line.match(/^\s+(in|out|errors|blocks|doc):\s*(.*)$/))) {
      if (!call) fail(file, n, m[1] + ': outside a call');
      const v = m[2].trim();
      if (m[1] === 'errors') { if (v !== '-') call.errors.push(...v.replace(/\([^)]*\)/g, '').split(/[\s,]+/).filter(Boolean)); call.errorsText = (call.errorsText ? call.errorsText + ' ' : '') + v; }
      else if (m[1] === 'blocks') call.blocks = v;
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
    const m = line.match(/^error\s+(E_\w+)\s+(\$[0-9A-Fa-f]{2})\s+"([^"]+)"$/);
    if (!m) fail(file, i + 1, 'what is "' + line + '"?');
    const code = num(m[2]);
    if (errors.find(e => e.code === code || e.name === m[1])) fail(file, i + 1, m[1] + ': its name or code is used twice');
    if (m[3].length > 31) fail(file, i + 1, m[1] + ': its text is over 31 characters (ERRSTR\'s buffer is 32 bytes)');
    errors.push({ name: m[1], code, text: m[3] });
  });
  return errors;
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

function apiMd(api, errors) {
  let s = '## **The system calls**' + CRLF + CRLF;
  s += 'Made by `tools/apigen.js` from `spec/api.def` and `spec/errors.def`: don\'t edit.  The rules for every call are in [conventions.md](../../docs/conventions.md#the-abi): `.A`, `.X`, `.Y` and `r0-r15` in and out, 16-bit results in `.A`/`.X`, and C = 1 with the error code in `.A` on failure.' + CRLF;
  for (const g of api.groups) {
    s += CRLF + '### **' + g.name + '** (`' + hx(g.base, 4) + '`, ' + g.slots + ' slots)' + CRLF + CRLF + g.doc + '.' + CRLF;
    if (!g.calls.length) { s += CRLF + '(No calls yet.)' + CRLF; continue; }
    s += CRLF + '| Call | Address | In | Out | Errors | Waits |' + CRLF + '| :--- | :------ | :- | :-- | :----- | :---- |' + CRLF;
    const esc = t => t.replace(/\|/g, '\\|');
    for (const c of g.calls) s += '| `' + c.name + '` | `' + hx(c.addr, 4) + '` | ' + esc(c.in.join(' ') || '-') + ' | ' + esc(c.out.join(' ') || '-') + ' | ' + esc(c.errorsText || '-') + ' | ' + (c.blocks || '-') + ' |' + CRLF;
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
  const gen = path.join(root, 'obj', 'gen');
  write(path.join(gen, 'jumptable.s'), jumptable(api));
  write(path.join(gen, 'errors.inc'), errorsInc(errors));
  write(path.join(gen, 'errtext.s'), errText(errors));
  write(path.join(root, 'obj', 'sdk', 'hydra.inc'), sdkInc(api, errors));
  write(path.join(gen, 'api.md'), apiMd(api, errors));
  write(path.join(gen, 'api.json'), JSON.stringify({
    calls: api.calls.map(c => ({ name: c.name, addr: c.addr, group: c.group, in: c.in.join(' '), out: c.out.join(' '), errors: c.errors, blocks: c.blocks })),
    errors, consts: api.consts.map(k => ({ name: k.name, value: k.value })),
  }, null, 1) + '\n');
  return { groups: api.groups, calls: api.calls, consts: api.consts, errors };
}

module.exports = { generate };

if (require.main === module) {
  try {
    const r = generate(path.resolve(process.argv[2] || path.join(__dirname, '..')));
    console.log('apigen: ' + r.calls.length + ' calls in ' + r.groups.length + ' groups, ' + r.errors.length + ' error codes');
  } catch (e) { console.error('apigen: ' + e.message); process.exit(1); }
}
