#!/usr/bin/env node
// ****************************************************************************
// bom.js - the parts lists of the Hydra-16's boards, from their KiCad schematics (board/): every placed symbol on
// every sheet (the root sheet's sheets, followed), but the power symbols, the ones marked not in the BOM, and the
// ones marked do-not-place; a part of several units (a 74F08's four gates) once.  Each board's parts grouped by
// value and package, with their references, how many, the package (from the footprint), and whether they sit in a
// socket (a footprint named ..._Socket...); then the sockets to buy, by kind.  The tables go into a chapter of the book
// between its markers, <!-- parts:BOARD:begin --> and <!-- parts:BOARD:end -->, so what's written around them stays.
//
// Usage: node docs/build/bom.js [--board DIR] [--check] [CHAPTER.md]
//   --board DIR   the KiCad files (default: the repository's board/)
//   --check       don't write: say if the chapter's tables are out of date (exit 1)
//   CHAPTER.md    the chapter (default docs/02-hardware/parts.md)
// From Node: boards(dir) gives [{ name, file, parts: [{ ref, value, footprint, lib, description }] }].
'use strict';
const fs = require('fs');
const path = require('path');
const { parse } = require('./sexpr.js');

const REPO = path.join(__dirname, '..', '..');

// The boards: each a KiCad project's root sheet, under board/
const BOARDS = [
  { key: 'main', name: 'The main board', file: 'hydra-16.kicad_sch' },
  { key: 'memory', name: 'The memory daughter card (through-hole)', file: 'MemoryDaughterCard/MemDaughterCard.kicad_sch' },
  { key: 'memory-smd', name: 'The memory daughter card (surface mount)', file: 'MemoryDaughterCard/MemDaughterCard_SurfaceMount.kicad_sch' },
  { key: 'breakout', name: 'The bus breakout card', file: 'HydraBusBreakoutCard/HydraBusBreakoutCard.kicad_sch' },
  { key: 'vera-x', name: 'The Vera X glue board', file: 'vera-x/vera-x.kicad_sch' },
];

const head = l => Array.isArray(l) ? l[0] : null;
const field = (l, name) => l.find(x => head(x) === name);
const prop = (l, name) => { const p = l.find(x => head(x) === 'property' && x[1] === name); return p ? p[2] : ''; };

// A sheet's symbols, and its sheets' (followed: their files beside it)
function sheet(file, out, seen) {
  if (seen.has(file)) return;
  seen.add(file);
  const doc = parse(fs.readFileSync(file, 'utf8'));
  for (const x of doc) {
    if (head(x) === 'symbol') {
      const lib = (field(x, 'lib_id') || [])[1] || '';
      const flag = n => (field(x, n) || [])[1];
      if (lib.startsWith('power:') || flag('in_bom') === 'no' || flag('dnp') === 'yes' || flag('on_board') === 'no') continue;
      const ref = prop(x, 'Reference');
      if (!ref || ref.startsWith('#')) continue;
      out.push({ ref, value: prop(x, 'Value'), footprint: prop(x, 'Footprint'), lib, description: prop(x, 'Description') });
    } else if (head(x) === 'sheet') {
      const f = prop(x, 'Sheetfile');
      if (f) sheet(path.join(path.dirname(file), f), out, seen);
    }
  }
}

function boards(dir) {
  return BOARDS.filter(b => fs.existsSync(path.join(dir, b.file))).map(b => {
    const all = [];
    sheet(path.join(dir, b.file), all, new Set());
    const byRef = new Map();
    for (const p of all) if (!byRef.has(p.ref)) byRef.set(p.ref, p);   // (A part of several units: once)
    return Object.assign({}, b, { parts: [...byRef.values()] });
  });
}

// ---- The tables

// The package a footprint names: DIP-28, SOIC-16, 0805, 3.5 mm jack ...
function packageOf(fp) {
  const name = fp.split(':').pop();
  let m;
  if ((m = name.match(/^(DIP|SOIC|TSSOP|SSOP|PLCC|QFN|SOP)-(\d+)/i))) return m[1].toUpperCase() + '-' + m[2];
  if ((m = name.match(/^(C|R|L|D|LED)_(\d{4})_/))) return m[2] + ' SMD';
  if ((m = name.match(/^CP_Radial_D([\d.]+)mm/))) return 'radial ' + m[1] + ' mm';
  if ((m = name.match(/^C_(Disc|Rect)_/))) return 'ceramic, through-hole';
  if ((m = name.match(/^R_Axial/))) return 'axial resistor';
  if ((m = name.match(/^R_Array_SIP(\d+)/))) return 'SIP-' + m[1] + ' network';
  if ((m = name.match(/^PinHeader_(\d+x\d+)_P([\d.]+)mm/))) return 'header ' + m[1] + ', ' + m[2] + ' mm';
  if ((m = name.match(/^PinSocket_(\d+x\d+)_P([\d.]+)mm/))) return 'socket ' + m[1] + ', ' + m[2] + ' mm';
  if ((m = name.match(/^Crystal_(HC\w+)/))) return 'crystal ' + m[1];
  if ((m = name.match(/^Oscillator_(DIP-\d+)/))) return 'oscillator ' + m[1];
  if ((m = name.match(/^LED_D([\d.]+)mm/))) return 'LED ' + m[1] + ' mm';
  return name.replace(/_/g, ' ');
}
const socketed = fp => /Socket/i.test(fp.split(':').pop()) && /^(DIP|PLCC)/i.test(fp.split(':').pop());   // (A DIP or a PLCC in its socket footprint)
const socketKind = fp => { const m = fp.split(':').pop().match(/^(DIP|PLCC)-(\d+)(?:_W([\d.]+)mm)?/i); return m ? m[1].toUpperCase() + '-' + m[2] + (m[3] ? ' (' + (+m[3] >= 15 ? '0.6"' : '0.3"') + ')' : '') : '?'; };

// References in order: U2 before U10, ranges where they run on (R4-R7)
function refList(refs) {
  const key = r => { const m = r.match(/^([A-Z]+)(\d+)$/i); return m ? [m[1], +m[2]] : [r, 0]; };
  const sorted = [...refs].sort((a, b) => { const x = key(a), y = key(b); return x[0] < y[0] ? -1 : x[0] > y[0] ? 1 : x[1] - y[1]; });
  const out = [];
  for (let i = 0; i < sorted.length; ) {
    let j = i;
    const [p, n] = key(sorted[i]);
    while (j + 1 < sorted.length && key(sorted[j + 1])[0] === p && key(sorted[j + 1])[1] === key(sorted[j])[1] + 1) j++;
    out.push(j - i >= 2 ? sorted[i] + '-' + n.toString().replace(/.*/, '') + key(sorted[j])[1] : sorted.slice(i, j + 1).join(', '));
    i = j + 1;
  }
  return out.join(', ').replace(/([A-Z]+)(\d+)-(\d+)/gi, '$1$2-$1$3');
}

// The kinds the parts are listed under, in order
const KINDS = [
  ['ICs', p => /^(U|IC)\d/.test(p.ref)],
  ['Crystals and oscillators', p => /^(X|Y|OSC)\d/.test(p.ref)],
  ['Capacitors', p => /^C\d/.test(p.ref)],
  ['Resistors and networks', p => /^(R|RN)\d/.test(p.ref)],
  ['LEDs and diodes', p => /^(D|LED)\d/.test(p.ref)],
  ['Connectors, headers and jumpers', p => /^(J|P|JP|CONN)\d/.test(p.ref)],
  ['Switches', p => /^(SW|S)\d/.test(p.ref)],
  ['Other', () => true],
];

// A capacitor's or resistor's three-digit EIA code with its plain value beside it: 104 (100 nF), 103 (10K), 220 (22 Ω)
function plain(p) {
  const m = /^(\d)(\d)(\d)(\s.*)?$/.exec(p.value), r = /^(R|RN)\d/.test(p.ref), c = /^C\d/.test(p.ref);
  if (p.value === '0' && r) return '0 (a link)';
  if (!m || !(r || c)) return p.value;
  const n = (+m[1] * 10 + +m[2]) * Math.pow(10, +m[3]);
  const units = r ? [[1e6, 'M'], [1e3, 'K'], [1, ' Ω']] : [[1e6, ' µF'], [1e3, ' nF'], [1, ' pF']];
  const [div, u] = units.find(([d]) => n >= d) || units[units.length - 1];
  const v = String(+(n / div).toPrecision(3)) + u;
  return m[1] + m[2] + m[3] + (m[4] || '') + ' (' + v + ')';
}

// Numbered connectors (SLOT 0 ... SLOT 5, SPI Device 0 ... 7) grouped as one: their value without its number
const series = p => /^(J|P)\d/.test(p.ref) && /\D\s+\d+$/.test(p.value) ? p.value.replace(/\s+\d+$/, '') : p.value;
const numbers = g => { const n = g.map(p => +p.value.match(/(\d+)$/)[1]).sort((a, b) => a - b); return n[0] + '-' + n[n.length - 1]; };

function table(board) {
  const lines = [];
  const left = new Set(board.parts);
  let total = 0;
  for (const [kind, test] of KINDS) {
    const these = [...left].filter(test);
    if (!these.length) continue;
    these.forEach(p => left.delete(p));
    const groups = new Map();
    for (const p of these) {
      const k = series(p) + '\u0000' + packageOf(p.footprint) + '\u0000' + socketed(p.footprint);
      if (!groups.has(k)) groups.set(k, []);
      groups.get(k).push(p);
    }
    lines.push('', '**' + kind + '**', '', '| Qty | Part | Package | Socket | References |', '| --: | :--- | :------ | :----- | :--------- |');
    const rows = [...groups.values()].sort((a, b) => (a[0].value < b[0].value ? -1 : a[0].value > b[0].value ? 1 : 0));
    for (const g of rows) {
      const p = g[0];
      lines.push('| ' + g.length + ' | ' + (g.length > 1 && series(p) !== p.value ? series(p) + ' ' + numbers(g) : plain(p) || '?').replace(/\|/g, '\\|') + ' | ' + packageOf(p.footprint) + ' | ' + (socketed(p.footprint) ? socketKind(p.footprint) : '') +
        ' | ' + refList(g.map(x => x.ref)) + ' |');
      total += g.length;
    }
  }
  const sockets = new Map();
  for (const p of board.parts) if (socketed(p.footprint)) sockets.set(socketKind(p.footprint), (sockets.get(socketKind(p.footprint)) || 0) + 1);
  if (sockets.size) {
    lines.push('', '**Sockets to buy**', '', '| Qty | Socket |', '| --: | :----- |');
    for (const [k, n] of [...sockets].sort()) lines.push('| ' + n + ' | ' + k + ' |');
  }
  lines.push('', total + ' parts on ' + board.name.replace(/^The /, 'the ') + ', from `board/' + board.file + '`.');
  return lines.join('\n');
}

function update(chapter, dir) {
  let text = fs.readFileSync(chapter, 'utf8');
  const nl = text.includes('\r\n') ? '\r\n' : '\n';
  text = text.replace(/\r\n/g, '\n');
  for (const b of boards(dir)) {
    const begin = '<!-- parts:' + b.key + ':begin -->', end = '<!-- parts:' + b.key + ':end -->';
    const i = text.indexOf(begin), j = text.indexOf(end);
    if (i < 0 || j < i) continue;
    text = text.slice(0, i + begin.length) + '\n' + table(b) + '\n\n' + text.slice(j);
  }
  return text.replace(/\n/g, nl);
}

if (require.main === module) {
  const a = process.argv.slice(2);
  let dir = path.join(REPO, 'board'), check = false, chapter = path.join(REPO, 'docs', '02-hardware', 'parts.md');
  for (let i = 0; i < a.length; i++) {
    if (a[i] === '--board') dir = path.resolve(a[++i]);
    else if (a[i] === '--check') check = true;
    else chapter = path.resolve(a[i]);
  }
  try {
    const text = update(chapter, dir), old = fs.readFileSync(chapter, 'utf8');
    if (check) { if (text !== old) { console.error('bom: ' + path.relative(REPO, chapter) + ' is out of date (node docs/build/bom.js)'); process.exit(1); } }
    else if (text !== old) { fs.writeFileSync(chapter, text); console.log('bom: ' + path.relative(REPO, chapter) + ' written'); }
    else console.log('bom: ' + path.relative(REPO, chapter) + ' as it was');
  } catch (e) { console.error('bom: ' + e.message); process.exit(1); }
}
module.exports = { boards, table, packageOf, plain };
