#!/usr/bin/env node
// board.js - the book's drawings of the Hydra-16's circuit board, from its layout (board/hydra-16.kicad_pcb): the
// board's outline, each part's outline and pads, the printed labels, and the parts' names, in black ink; with some
// parts coloured (a group each, and a key), for the tour and the building steps.
//
// Usage: node manual/tools/board.js PCB OUT.svg [--color GROUP:COLOR:REF,REF-REF,...]... [--title TEXT]
//        [--key] [--names all|colored|none] [--fade]
//   --color   the parts to colour: REFs (U1), ranges (C80-C85) or patterns (C*); GROUP the key's name for them
//   --fade    the rest drawn pale (a building step's figure: what to fit now stands out)
'use strict';
const fs = require('fs');
const path = require('path');

// ---- S-expressions (KiCad's): lists of atoms and strings
function parse(s) {
  let i = 0;
  function item() {
    while (/\s/.test(s[i])) i++;
    if (s[i] === '(') { i++; const l = []; for (;;) { while (/\s/.test(s[i])) i++; if (s[i] === ')') { i++; return l; } l.push(item()); } }
    if (s[i] === '"') { i++; let t = ''; while (s[i] !== '"') { if (s[i] === '\\') { t += s[i + 1]; i += 2; } else t += s[i++]; } i++; return { s: t }; }
    let t = ''; while (i < s.length && !/[\s()]/.test(s[i])) t += s[i++]; return t;
  }
  return item();
}
const head = l => Array.isArray(l) ? l[0] : null;
const find = (l, h) => l.find(e => head(e) === h);
const all = (l, h) => l.filter(e => head(e) === h);
const str = e => e && (e.s !== undefined ? e.s : e);
const num = e => +str(e);

// A local point in a footprint at (X, Y, rot): KiCad's angles turn counter-clockwise on a y-down board
function xf(fp, x, y) {
  const a = fp.rot * Math.PI / 180, c = Math.cos(a), s = Math.sin(a);
  return [fp.x + x * c + y * s, fp.y - x * s + y * c];
}

function load(file) {
  const t = parse(fs.readFileSync(file, 'utf8'));
  const board = { edges: [], texts: [], fps: [] };
  for (const g of t.slice(1)) {
    const h = head(g);
    const layer = g && find(g, 'layer') ? str(find(g, 'layer')[1]) : '';
    if ((h === 'gr_line' || h === 'gr_rect' || h === 'gr_arc' || h === 'gr_circle') && layer === 'Edge.Cuts') board.edges.push(g);
    if (h === 'gr_text' && layer === 'F.SilkS') {
      const at = find(g, 'at'), eff = find(g, 'effects'), font = eff && find(eff, 'font'), size = font && find(font, 'size');
      board.texts.push({ text: str(g[1]), x: num(at[1]), y: num(at[2]), rot: num(at[3] || 0), size: size ? num(size[1]) : 1.5 });
    }
    if (h === 'footprint') {
      const at = find(g, 'at');
      const fp = { lib: str(g[1]), x: num(at[1]), y: num(at[2]), rot: num(at[3] || 0), ref: '', value: '', lines: [], pads: [], refAt: null };
      for (const p of all(g, 'property')) {
        if (str(p[1]) === 'Reference') { fp.ref = str(p[2]); const pa = find(p, 'at'); fp.refAt = [num(pa[1]), num(pa[2])]; }
        if (str(p[1]) === 'Value') fp.value = str(p[2]);
      }
      for (const e of g) {
        const eh = head(e);
        if (!['fp_line', 'fp_rect', 'fp_circle', 'fp_arc', 'fp_poly'].includes(eh)) continue;
        const ly = str(find(e, 'layer')[1]);
        if (ly !== 'F.SilkS' && ly !== 'F.Fab') continue;
        fp.lines.push({ kind: eh, layer: ly, e });
      }
      for (const p of all(g, 'pad')) {
        const pa = find(p, 'at'), sz = find(p, 'size'), dr = find(p, 'drill');
        fp.pads.push({ shape: p[3], x: num(pa[1]), y: num(pa[2]), rot: num(pa[3] || 0), w: num(sz[1]), h: num(sz[2]), drill: dr ? num(dr[1]) : 0, num: str(p[1]) });
      }
      board.fps.push(fp);
    }
  }
  return board;
}

// The board's outline's bounds
function bounds(board) {
  let x0 = Infinity, y0 = Infinity, x1 = -Infinity, y1 = -Infinity;
  const take = (x, y) => { x0 = Math.min(x0, x); y0 = Math.min(y0, y); x1 = Math.max(x1, x); y1 = Math.max(y1, y); };
  for (const g of board.edges) for (const k of ['start', 'end', 'mid', 'center']) { const p = find(g, k); if (p) take(num(p[1]), num(p[2])); }
  return { x0, y0, x1, y1 };
}

// Does REF match a list of refs, ranges and patterns?
function matcher(spec) {
  const parts = spec.split(',').map(s => s.trim()).filter(Boolean);
  return ref => parts.some(p => {
    if (p.endsWith('*')) return ref.startsWith(p.slice(0, -1)) && /^\d+$/.test(ref.slice(p.length - 1));
    const m = /^([A-Z]+)(\d+)-(?:[A-Z]+)?(\d+)$/.exec(p);
    if (m) { const r = /^([A-Z]+)(\d+)$/.exec(ref); return r && r[1] === m[1] && +r[2] >= +m[2] && +r[2] <= +m[3]; }
    return p === ref;
  });
}

const esc = s => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
const f = n => +n.toFixed(2);

function render(board, opt) {
  const b = bounds(board), pad = 3;
  const W = b.x1 - b.x0 + 2 * pad;
  const keyRows = []; { let row = [], x = 0; for (const c of opt.colors) { const w = 12 + c.name.length * 2.1; if (x + w > W - 2 * pad && row.length) { keyRows.push(row); row = []; x = 0; } row.push(c); x += w; } if (row.length) keyRows.push(row); }
  const Hh = b.y1 - b.y0 + 2 * pad + (opt.key ? keyRows.length * 8 + 4 : 0) + (opt.title ? 8 : 0);
  const ox = -b.x0 + pad, oy = -b.y0 + pad + (opt.title ? 8 : 0);
  const out = [];
  out.push(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${f(W)} ${f(Hh)}" width="${f(W)}mm" height="${f(Hh)}mm" font-family="Arial, Helvetica, sans-serif">`);
  out.push(`<rect x="0" y="0" width="${f(W)}" height="${f(Hh)}" fill="#fff"/>`);
  if (opt.title) out.push(`<text x="${f(W / 2)}" y="6" font-size="5" font-weight="bold" text-anchor="middle">${esc(opt.title)}</text>`);
  out.push(`<g transform="translate(${f(ox)} ${f(oy)})">`);
  // The board: a pale fill, then the outline
  out.push(`<rect x="${f(b.x0)}" y="${f(b.y0)}" width="${f(b.x1 - b.x0)}" height="${f(b.y1 - b.y0)}" rx="2" fill="#f7f6f0"/>`);
  for (const g of board.edges) {
    const h = head(g);
    if (h === 'gr_line') { const s = find(g, 'start'), e = find(g, 'end'); out.push(`<line x1="${f(num(s[1]))}" y1="${f(num(s[2]))}" x2="${f(num(e[1]))}" y2="${f(num(e[2]))}" stroke="#111" stroke-width="0.5"/>`); }
    if (h === 'gr_rect') { const s = find(g, 'start'), e = find(g, 'end'); out.push(`<rect x="${f(Math.min(num(s[1]), num(e[1])))}" y="${f(Math.min(num(s[2]), num(e[2])))}" width="${f(Math.abs(num(e[1]) - num(s[1])))}" height="${f(Math.abs(num(e[2]) - num(s[2])))}" fill="none" stroke="#111" stroke-width="0.5"/>`); }
    if (h === 'gr_arc') { const s = find(g, 'start'), m = find(g, 'mid'), e = find(g, 'end'); out.push(arcPath([num(s[1]), num(s[2])], [num(m[1]), num(m[2])], [num(e[1]), num(e[2])], '#111', 0.5)); }
  }
  // The printed labels
  for (const t of board.texts) {
    if (opt.noLabels) break;
    const rot = ((t.rot % 360) + 360) % 360;
    const txt = t.text.replace(/\\n/g, ' ');
    out.push(`<text x="${f(t.x)}" y="${f(t.y)}" font-size="${f(t.size * 0.95)}" fill="#8a7f60" text-anchor="middle" dominant-baseline="middle" font-style="italic"${rot ? ` transform="rotate(${-rot} ${f(t.x)} ${f(t.y)})"` : ''}>${esc(txt)}</text>`);
  }
  // The parts
  const colorOf = ref => { for (const c of opt.colors) if (c.match(ref)) return c; return null; };
  for (const fp of board.fps) {
    const c = colorOf(fp.ref);
    const faded = opt.fade && !c;
    const ink = faded ? '#c8c6bd' : '#1a1a1a', padInk = faded ? '#d8d6ce' : '#7a7a7a';
    // A coloured part: its fab outline's box filled
    const pts = [];
    for (const l of fp.lines) for (const k of ['start', 'end', 'center', 'mid']) { const p = find(l.e, k); if (p) pts.push(xf(fp, num(p[1]), num(p[2]))); }
    for (const p of fp.pads) pts.push(xf(fp, p.x, p.y));
    if (c && pts.length) {
      const xs = pts.map(p => p[0]), ys = pts.map(p => p[1]);
      const bx0 = Math.min(...xs) - 0.6, by0 = Math.min(...ys) - 0.6, bx1 = Math.max(...xs) + 0.6, by1 = Math.max(...ys) + 0.6;
      out.push(`<rect x="${f(bx0)}" y="${f(by0)}" width="${f(bx1 - bx0)}" height="${f(by1 - by0)}" rx="0.8" fill="${c.color}" fill-opacity="0.55" stroke="${c.stroke || '#000'}" stroke-width="0.45"/>`);
    }
    for (const l of fp.lines) {
      if (l.layer === 'F.Fab' && fp.lines.some(q => q.layer === 'F.SilkS')) continue;   // (the silkscreen's outline if it has one)
      const e = l.e, sw = 0.22;
      if (l.kind === 'fp_line') { const s = find(e, 'start'), en = find(e, 'end'); const [x1, y1] = xf(fp, num(s[1]), num(s[2])), [x2, y2] = xf(fp, num(en[1]), num(en[2])); out.push(`<line x1="${f(x1)}" y1="${f(y1)}" x2="${f(x2)}" y2="${f(y2)}" stroke="${ink}" stroke-width="${sw}"/>`); }
      if (l.kind === 'fp_rect') { const s = find(e, 'start'), en = find(e, 'end'); const c4 = [[num(s[1]), num(s[2])], [num(en[1]), num(s[2])], [num(en[1]), num(en[2])], [num(s[1]), num(en[2])]].map(p => xf(fp, p[0], p[1])); out.push(`<polygon points="${c4.map(p => f(p[0]) + ',' + f(p[1])).join(' ')}" fill="none" stroke="${ink}" stroke-width="${sw}"/>`); }
      if (l.kind === 'fp_circle') { const ce = find(e, 'center'), en = find(e, 'end'); const [cx, cy] = xf(fp, num(ce[1]), num(ce[2])); const r = Math.hypot(num(en[1]) - num(ce[1]), num(en[2]) - num(ce[2])); out.push(`<circle cx="${f(cx)}" cy="${f(cy)}" r="${f(r)}" fill="none" stroke="${ink}" stroke-width="${sw}"/>`); }
      if (l.kind === 'fp_arc') { const s = find(e, 'start'), m = find(e, 'mid'), en = find(e, 'end'); if (m) out.push(arcPath(xf(fp, num(s[1]), num(s[2])), xf(fp, num(m[1]), num(m[2])), xf(fp, num(en[1]), num(en[2])), ink, sw)); }
      if (l.kind === 'fp_poly') { const p = find(e, 'pts'); const ps = all(p, 'xy').map(q => xf(fp, num(q[1]), num(q[2]))); out.push(`<polygon points="${ps.map(q => f(q[0]) + ',' + f(q[1])).join(' ')}" fill="none" stroke="${ink}" stroke-width="${sw}"/>`); }
    }
    if (/^H[0-9]/.test(fp.ref)) { const [x, y] = xf(fp, 0, 0); const r = fp.pads.length ? Math.max(...fp.pads.map(p => Math.max(p.w, p.h, p.drill))) / 2 : 1.6; out.push(`<circle cx="${f(x)}" cy="${f(y)}" r="${f(r)}" fill="none" stroke="${ink}" stroke-width="0.3"/><circle cx="${f(x)}" cy="${f(y)}" r="${f(r * 0.55)}" fill="#fff" stroke="${ink}" stroke-width="0.3"/>`); continue; }
    for (const p of fp.pads) {
      const [x, y] = xf(fp, p.x, p.y);
      if (p.drill) out.push(`<circle cx="${f(x)}" cy="${f(y)}" r="${f(Math.min(p.w, p.h) / 2 * 0.75)}" fill="${padInk}"/>`);
      else out.push(`<rect x="${f(x - p.w / 2)}" y="${f(y - p.h / 2)}" width="${f(p.w)}" height="${f(p.h)}" fill="${padInk}" transform="rotate(${-p.rot} ${f(x)} ${f(y)})"/>`);
    }
    // Its name: where its reference is printed, or its middle
    const show = opt.names === 'all' || (opt.names === 'colored' && c);
    if (show && fp.ref && !/^H\d/.test(fp.ref)) {
      const xs = pts.map(p => p[0]), ys = pts.map(p => p[1]);
      const cx = (Math.min(...xs) + Math.max(...xs)) / 2, cy = (Math.min(...ys) + Math.max(...ys)) / 2;
      const wd = Math.max(...xs) - Math.min(...xs), ht = Math.max(...ys) - Math.min(...ys);
      const size = Math.max(1.4, Math.min(3.2, Math.min(wd, ht) * 0.45, wd / Math.max(2, fp.ref.length) * 1.4));
      const vertical = ht > wd * 1.6 && wd < 9;
      out.push(`<text x="${f(cx)}" y="${f(cy)}" font-size="${f(size)}" font-weight="bold" fill="${faded ? '#bdbab0' : '#000'}" text-anchor="middle" dominant-baseline="middle"${vertical ? ` transform="rotate(-90 ${f(cx)} ${f(cy)})"` : ''}>${esc(fp.ref)}</text>`);
    }
  }
  out.push('</g>');
  // The key
  if (opt.key && opt.colors.length) {
    let y = Hh - keyRows.length * 8 + 2;
    for (const row of keyRows) {
      let x = pad;
      for (const c of row) {
        out.push(`<rect x="${f(x)}" y="${f(y - 2.5)}" width="5" height="5" fill="${c.color}" fill-opacity="0.55" stroke="#000" stroke-width="0.3"/>`);
        out.push(`<text x="${f(x + 6.5)}" y="${f(y + 1.2)}" font-size="3.6">${esc(c.name)}</text>`);
        x += 12 + c.name.length * 2.1;
      }
      y += 8;
    }
  }
  out.push('</svg>');
  return out.join('\n');
}

// An arc through three points, as an SVG path
function arcPath(a, m, b, ink, sw) {
  const [x1, y1] = a, [x2, y2] = m, [x3, y3] = b;
  const d = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
  if (Math.abs(d) < 1e-9) return `<line x1="${f(x1)}" y1="${f(y1)}" x2="${f(x3)}" y2="${f(y3)}" stroke="${ink}" stroke-width="${sw}"/>`;
  const ux = ((x1 * x1 + y1 * y1) * (y2 - y3) + (x2 * x2 + y2 * y2) * (y3 - y1) + (x3 * x3 + y3 * y3) * (y1 - y2)) / d;
  const uy = ((x1 * x1 + y1 * y1) * (x3 - x2) + (x2 * x2 + y2 * y2) * (x1 - x3) + (x3 * x3 + y3 * y3) * (x2 - x1)) / d;
  const r = Math.hypot(x1 - ux, y1 - uy);
  const cross = (x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1);
  const large = 0, sweep = cross > 0 ? 1 : 0;
  const a1 = Math.atan2(y1 - uy, x1 - ux), a3 = Math.atan2(y3 - uy, x3 - ux);
  let span = a3 - a1; if (sweep) { while (span < 0) span += 2 * Math.PI; } else { while (span > 0) span -= 2 * Math.PI; }
  const big = Math.abs(span) > Math.PI ? 1 : large;
  return `<path d="M ${f(x1)} ${f(y1)} A ${f(r)} ${f(r)} 0 ${big} ${sweep} ${f(x3)} ${f(y3)}" fill="none" stroke="${ink}" stroke-width="${sw}"/>`;
}

if (require.main === module) {
  const argv = process.argv.slice(2);
  const opt = { colors: [], key: false, names: 'all', fade: false, title: '', noLabels: false };
  const files = [];
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--color') { const [name, color, refs] = argv[++i].split(':'); opt.colors.push({ name, color, match: matcher(refs) }); }
    else if (a === '--key') opt.key = true;
    else if (a === '--fade') opt.fade = true;
    else if (a === '--no-labels') opt.noLabels = true;
    else if (a === '--names') opt.names = argv[++i];
    else if (a === '--title') opt.title = argv[++i];
    else files.push(a);
  }
  if (files.length !== 2) { console.error('usage: node board.js PCB OUT.svg [--color NAME:COLOR:REFS]... [--key] [--fade] [--names all|colored|none] [--title T]'); process.exit(2); }
  fs.writeFileSync(files[1], render(load(files[0]), opt));
  console.log('wrote', files[1]);
}
module.exports = { load, render, matcher };
