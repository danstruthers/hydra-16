#!/usr/bin/env node
// ****************************************************************************
// layout.js - the boards' layouts, from their KiCad PCBs (board/): a drawing of each board (an SVG: its outline, each
// part's outline from its courtyard, its reference, the silkscreen's labels for the board's areas), and a table of
// where each part is (millimetres from the board's top left corner, its side and its rotation), for finding a part
// on the board.  The drawing goes beside the chapter (docs/02-hardware/layout-BOARD.svg), the table into the chapter
// between its markers, <!-- layout:BOARD:begin --> and <!-- layout:BOARD:end -->.
//
// Usage: node docs/build/layout.js [--board DIR] [--check] [CHAPTER.md]
//   --board DIR   the KiCad files (default: the repository's board/)
//   --check       don't write: say if the drawings or the tables are out of date (exit 1)
//   CHAPTER.md    the chapter (default docs/02-hardware/layout.md)
'use strict';
const fs = require('fs');
const path = require('path');
const { parse } = require('./sexpr.js');
const { plain } = require('./bom.js');

const REPO = path.join(__dirname, '..', '..');
const BOARDS = [
  { key: 'main', name: 'the main board', file: 'hydra-16.kicad_pcb' },
  { key: 'memory', name: 'the memory daughter card (through-hole)', file: 'MemoryDaughterCard/MemDaughterCard.kicad_pcb' },
  { key: 'memory-smd', name: 'the memory daughter card (surface mount)', file: 'MemoryDaughterCard/MemDaughterCard_SurfaceMount.kicad_pcb' },
  { key: 'breakout', name: 'the bus breakout card', file: 'HydraBusBreakoutCard/HydraBusBreakoutCard.kicad_pcb' },
];

const head = l => Array.isArray(l) ? l[0] : null;
const field = (l, name) => Array.isArray(l) ? l.find(x => head(x) === name) : undefined;
const fields = (l, name) => Array.isArray(l) ? l.filter(x => head(x) === name) : [];
const num = x => parseFloat(x);
const prop = (l, name) => { const p = !Array.isArray(l) ? null : l.find(x => head(x) === 'property' && x[1] === name); return p ? p[2] : ''; };

// A board: its outline's box, and its parts with their boxes (board coordinates) and labels
function read(file) {
  const doc = parse(fs.readFileSync(file, 'utf8'));
  const pts = [];
  for (const x of doc) {                                      // The outline: Edge.Cuts' lines and rectangles
    const layer = (field(x, 'layer') || [])[1];
    if (layer !== 'Edge.Cuts') continue;
    for (const k of ['start', 'end', 'center', 'mid']) { const p = field(x, k); if (p) pts.push([num(p[1]), num(p[2])]); }
  }
  const labels = [];
  // Not an area's label: a header's pin legend (blank lines between its pins, or small), a table, a rule, the credits, a +
  const legend = (s, size) => /\n\s*\n/.test(s) || /[|\t]|^[\s_-]*$|\n[\s_-]*\n?$|^[\s_-]+\n/.test(s) || size < 1.2 ||
    /^(Designed|Revision)/.test(s.trim()) || s.trim().length <= 2;
  for (const x of doc) if (head(x) === 'gr_text' && /SilkS/.test((field(x, 'layer') || [])[1] || '')) {
    const at = field(x, 'at'), effects = field(x, 'effects') || [], font = field(effects, 'font') || [];
    const size = num((field(font, 'size') || [])[1] || 1.5), justify = (field(effects, 'justify') || []).slice(1);
    if (legend(x[1], size)) continue;
    labels.push({ text: x[1], x: num(at[1]), y: num(at[2]), anchor: justify.includes('left') ? 'start' : justify.includes('right') ? 'end' : 'middle' });
  }
  const parts = [];
  for (const fp of fields(doc, 'footprint')) {
    const at = field(fp, 'at'), x0 = num(at[1]), y0 = num(at[2]), rot = num(at[3] || 0);
    const side = (field(fp, 'layer') || [])[1] === 'B.Cu' ? 'back' : 'front';
    const ref = prop(fp, 'Reference');
    if (!ref || ref.startsWith('#') || ref === 'REF**') continue;
    // Its outline: the courtyard's lines and rectangles (else the fab layer's, else the pads'), rotated and moved
    const local = [];
    for (const layerWanted of [/CrtYd/, /Fab/, null]) {
      for (const g of fp) {
        if (layerWanted === null) {
          if (head(g) === 'pad') { const p = field(g, 'at'), s = field(g, 'size'); if (p && s) { const px = num(p[1]), py = num(p[2]), w = num(s[1]) / 2, h = num(s[2]) / 2; local.push([px - w, py - h], [px + w, py + h]); } }
          continue;
        }
        if (!/^fp_(line|rect|circle|arc|poly)$/.test(head(g) || '')) continue;
        if (!layerWanted.test((field(g, 'layer') || [])[1] || '')) continue;
        for (const k of ['start', 'end', 'center', 'mid']) { const p = field(g, k); if (p) local.push([num(p[1]), num(p[2])]); }
        const poly = field(g, 'pts');
        if (poly) for (const xy of fields(poly, 'xy')) local.push([num(xy[1]), num(xy[2])]);
      }
      if (local.length) break;
    }
    const r = -rot * Math.PI / 180, c = Math.cos(r), s = Math.sin(r);
    const world = local.map(([x, y]) => [x0 + x * c - y * s, y0 + x * s + y * c]);
    if (!world.length) world.push([x0 - 1, y0 - 1], [x0 + 1, y0 + 1]);
    const xs = world.map(p => p[0]), ys = world.map(p => p[1]);
    parts.push({ ref, value: prop(fp, 'Value'), side, rot: ((rot % 360) + 360) % 360, x: x0, y: y0,
      box: [Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys)] });
  }
  if (!pts.length) for (const p of parts) pts.push([p.box[0], p.box[1]], [p.box[2], p.box[3]]);
  const xs = pts.map(p => p[0]), ys = pts.map(p => p[1]);
  return { outline: [Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys)], parts, labels };
}

const kind = ref => /^(U|IC)\d/.test(ref) ? 'ic' : /^(J|P|JP)\d/.test(ref) ? 'conn' : /^(C|R|RN|D|LED|Y|X|L)\d/.test(ref) ? 'passive' : 'other';
const esc = s => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

// The drawing: millimetres, the board's top left at 0, 0
function svg(b) {
  const [ox, oy, ex, ey] = b.outline, w = ex - ox, h = ey - oy, m = 2;
  const out = [`<svg xmlns="http://www.w3.org/2000/svg" viewBox="${-m} ${-m} ${(w + 2 * m).toFixed(2)} ${(h + 2 * m).toFixed(2)}" width="${Math.round((w + 2 * m) * 4)}" height="${Math.round((h + 2 * m) * 4)}" font-family="Helvetica, Arial, sans-serif">`,
    '<style>.board{fill:#1f5f3a;stroke:#0b2b18;stroke-width:0.6}.ic{fill:#2b2b2b;stroke:#0d0d0d;stroke-width:0.2}.conn{fill:#c9c9c9;stroke:#555;stroke-width:0.2}' +
    '.passive{fill:#d9c07a;stroke:#6b5a2a;stroke-width:0.15}.other{fill:#8aa7c9;stroke:#3d5470;stroke-width:0.2}.back{fill-opacity:0.35;stroke-dasharray:0.8 0.6}' +
    '.ref{fill:#fff;text-anchor:middle;dominant-baseline:central}.refd{fill:#111;text-anchor:middle;dominant-baseline:central}.label{fill:#fffbe0;font-weight:bold;text-anchor:middle;stroke:#0b2b18;stroke-width:0.7;paint-order:stroke}</style>',
    `<rect class="board" x="0" y="0" width="${w.toFixed(2)}" height="${h.toFixed(2)}" rx="2"/>`];
  const sorted = [...b.parts].sort((p, q) => (p.side === 'back') - (q.side === 'back'));
  for (const p of sorted) {
    const [x1, y1, x2, y2] = p.box, k = kind(p.ref);
    out.push(`<rect class="${k}${p.side === 'back' ? ' back' : ''}" x="${(x1 - ox).toFixed(2)}" y="${(y1 - oy).toFixed(2)}" width="${(x2 - x1).toFixed(2)}" height="${(y2 - y1).toFixed(2)}"><title>${esc(p.ref + ' ' + p.value)}</title></rect>`);
  }
  for (const l of b.labels) {                                 // (The area labels under the references, so no chip's name is hidden)
    const lines = l.text.split('\n');
    lines.forEach((t, i) => out.push(`<text class="label"${l.anchor === 'middle' ? '' : ` text-anchor="${l.anchor}"`} x="${(l.x - ox).toFixed(2)}" y="${(l.y - oy + (i - (lines.length - 1) / 2) * 4).toFixed(2)}" font-size="3.2">${esc(t)}</text>`));
  }
  // Each label's box (3.2 mm text, 4 mm lines), so a reference it would cover moves to the top of its part
  const covers = b.labels.map(l => {
    const lines = l.text.split('\n'), w = Math.max(...lines.map(t => t.length)) * 3.2 * 0.6, h = lines.length * 4;
    const x0 = l.anchor === 'start' ? l.x : l.anchor === 'end' ? l.x - w : l.x - w / 2;
    return [x0, l.y - h / 2 - 1, x0 + w, l.y + h / 2];
  });
  const hit = (a, c) => a[0] < c[2] && c[0] < a[2] && a[1] < c[3] && c[1] < a[3];
  for (const p of sorted) {
    const [x1, y1, x2, y2] = p.box, bw = x2 - x1, bh = y2 - y1, k = kind(p.ref);
    const size = Math.max(1, Math.min(3.2, Math.min(bw, bh) * 0.55, bw / (p.ref.length * 0.62)));
    const cx = (x1 + x2) / 2, tw = p.ref.length * size * 0.62;
    let cy = (y1 + y2) / 2;
    if (covers.some(c => hit([cx - tw / 2, cy - size / 2, cx + tw / 2, cy + size / 2], c)) && bh > 3 * size) cy = y1 + size;
    out.push(`<text class="${k === 'ic' ? 'ref' : 'refd'}" x="${(cx - ox).toFixed(2)}" y="${(cy - oy).toFixed(2)}" font-size="${size.toFixed(2)}">${esc(p.ref)}</text>`);
  }
  out.push('</svg>');
  return out.join('\n') + '\n';
}

// The table: each part, in reference order, where it is
function table(b) {
  const [ox, oy, ex, ey] = b.outline;
  const key = r => { const m = r.match(/^([A-Z]+)(\d+)$/i); return m ? [m[1], +m[2]] : [r, 0]; };
  const sorted = [...b.parts].sort((p, q) => { const x = key(p.ref), y = key(q.ref); return x[0] < y[0] ? -1 : x[0] > y[0] ? 1 : x[1] - y[1]; });
  const lines = ['', `The board is ${(ex - ox).toFixed(1)} mm wide and ${(ey - oy).toFixed(1)} mm high; a part's place is its centre, in millimetres from the board's top left corner (x across, y down), seen from the front.`, '',
    '| Part | Value | x | y | Side | Turned |', '| :--- | :---- | --: | --: | :--- | --: |'];
  for (const p of sorted)
    lines.push(`| ${p.ref} | ${String(plain(p)).replace(/\|/g, '\\|')} | ${(p.x - ox).toFixed(1)} | ${(p.y - oy).toFixed(1)} | ${p.side} | ${p.rot ? p.rot + '°' : ''} |`);
  return lines.join('\n');
}

function run(dir, chapter, check) {
  let text = fs.readFileSync(chapter, 'utf8');
  const nl = text.includes('\r\n') ? '\r\n' : '\n';
  text = text.replace(/\r\n/g, '\n');
  let stale = [];
  for (const b of BOARDS) {
    const f = path.join(dir, b.file);
    if (!fs.existsSync(f)) continue;
    const board = read(f), drawing = path.join(path.dirname(chapter), 'layout-' + b.key + '.svg'), pic = svg(board);
    if (!fs.existsSync(drawing) || fs.readFileSync(drawing, 'utf8') !== pic) { stale.push(path.relative(REPO, drawing)); if (!check) fs.writeFileSync(drawing, pic); }
    const begin = '<!-- layout:' + b.key + ':begin -->', end = '<!-- layout:' + b.key + ':end -->';
    const i = text.indexOf(begin), j = text.indexOf(end);
    if (i >= 0 && j > i) text = text.slice(0, i + begin.length) + '\n' + table(board) + '\n\n' + text.slice(j);
  }
  text = text.replace(/\n/g, nl);
  if (text !== fs.readFileSync(chapter, 'utf8')) { stale.push(path.relative(REPO, chapter)); if (!check) fs.writeFileSync(chapter, text); }
  return stale;
}

if (require.main === module) {
  const a = process.argv.slice(2);
  let dir = path.join(REPO, 'board'), check = false, chapter = path.join(REPO, 'docs', '02-hardware', 'layout.md');
  for (let i = 0; i < a.length; i++) {
    if (a[i] === '--board') dir = path.resolve(a[++i]);
    else if (a[i] === '--check') check = true;
    else chapter = path.resolve(a[i]);
  }
  try {
    const stale = run(dir, chapter, check);
    if (check && stale.length) { console.error('layout: out of date: ' + stale.join(', ') + ' (node docs/build/layout.js)'); process.exit(1); }
    console.log('layout: ' + (stale.length ? (check ? 'out of date: ' : 'written: ') + stale.join(', ') : 'as it was'));
  } catch (e) { console.error('layout: ' + e.message); process.exit(1); }
}
module.exports = { read, svg, table };
