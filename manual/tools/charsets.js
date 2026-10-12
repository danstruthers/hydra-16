#!/usr/bin/env node
// charsets.js - Appendix E's pictures and tables: the Vera X's two fonts (the console's ISO-8859-15, vid's own, and
// the PC's CP437 on the ROM disk), drawn from their files as 16 x 16 grids (figures/font-iso8859-15.svg,
// figures/font-cp437.svg); and on stdout, Markdown tables of the printable ASCII and ISO-8859-15 characters.
// Usage: node manual/tools/charsets.js [--update manual/app-e-characters.md]   (--update: between the file's markers)
'use strict';
const fs = require('fs');
const path = require('path');
const ROOT = path.join(__dirname, '..', '..');
const FIGS = path.join(__dirname, '..', 'figures');

// A font (256 glyphs of 8 bytes, the top row first, bit 7 the left) as a grid, its rows and columns labelled in hex
function fontSvg(file, title) {
  const f = fs.readFileSync(file);
  if (f.length !== 2048) throw new Error(file + ': not a 2K font');
  const S = 3, CELL = 8 * S + 8, LAB = 22, W = LAB + 16 * CELL + 4, H = LAB + 16 * CELL + 4;
  let p = '';
  for (let g = 0; g < 256; g++) {
    const x0 = LAB + (g & 15) * CELL + 4, y0 = LAB + (g >> 4) * CELL + 4;
    for (let r = 0; r < 8; r++) {
      const b = f[g * 8 + r];
      for (let c = 0; c < 8; c++) {
        if (!(b & (0x80 >> c))) continue;
        let n = 1;
        while (c + n < 8 && (b & (0x80 >> (c + n)))) n++;
        p += 'M' + (x0 + c * S) + ',' + (y0 + r * S) + 'h' + n * S + 'v' + S + 'h-' + n * S + 'z';
        c += n - 1;
      }
    }
  }
  let lab = '';
  for (let i = 0; i < 16; i++) {
    const h = i.toString(16).toUpperCase();
    lab += '<text x="' + (LAB + i * CELL + CELL / 2) + '" y="15" text-anchor="middle">x' + h + '</text>';
    lab += '<text x="11" y="' + (LAB + i * CELL + CELL / 2 + 4) + '" text-anchor="middle">' + h + 'x</text>';
  }
  let grid = '';
  for (let i = 0; i <= 16; i++) {
    grid += '<line x1="' + (LAB + i * CELL) + '" y1="' + LAB + '" x2="' + (LAB + i * CELL) + '" y2="' + (LAB + 16 * CELL) + '"/>';
    grid += '<line x1="' + LAB + '" y1="' + (LAB + i * CELL) + '" x2="' + (LAB + 16 * CELL) + '" y2="' + (LAB + i * CELL) + '"/>';
  }
  return '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ' + W + ' ' + H + '" width="' + W + '" height="' + H + '">' +
    '<title>' + title + '</title><rect width="' + W + '" height="' + H + '" fill="#fff"/>' +
    '<g stroke="#9ab" stroke-width="0.6">' + grid + '</g>' +
    '<g font-family="Courier New, monospace" font-size="10" fill="#357">' + lab + '</g>' +
    '<path fill="#123" d="' + p + '"/></svg>\n';
}

fs.writeFileSync(path.join(FIGS, 'font-iso8859-15.svg'), fontSvg(path.join(ROOT, 'reborn', 'modules', 'vid', 'iso8859-15.fnt'), 'The console font: ISO-8859-15'));
fs.writeFileSync(path.join(FIGS, 'font-cp437.svg'), fontSvg(path.join(ROOT, 'reborn', 'romfs', 'lib', 'font', 'cp437'), 'The PC font: CP437'));
console.error('wrote figures/font-iso8859-15.svg, figures/font-cp437.svg');

// ISO-8859-15: Latin-1, but for eight
const ISO15 = { 0xA4: 0x20AC, 0xA6: 0x160, 0xA8: 0x161, 0xB4: 0x17D, 0xB8: 0x17E, 0xBC: 0x152, 0xBD: 0x153, 0xBE: 0x178 };
const show = c => {
  if (c === 0x20) return '(space)';
  if (c === 0xA0) return '(no-break space)';
  if (c === 0xAD) return '(soft hyphen)';
  if (c === 0x7F) return '(DEL)';
  const u = ISO15[c] || c;
  const ch = String.fromCodePoint(u);
  if ('|`*_\\<>&[]'.includes(ch)) return '&#' + u + ';';
  return ch;
};
// A table of codes from a to b, in four groups of columns (decimal, hex, character)
function table(a, b) {
  const codes = [];
  for (let c = a; c <= b; c++) codes.push(c);
  const rows = Math.ceil(codes.length / 4);
  let out = '| Dec | Hex | Char | Dec | Hex | Char | Dec | Hex | Char | Dec | Hex | Char |\n' + '|---'.repeat(12) + '|\n';
  for (let r = 0; r < rows; r++) {
    const cells = [];
    for (let k = 0; k < 4; k++) {
      const c = codes[k * rows + r];
      if (c === undefined) cells.push('', '', '');
      else cells.push(String(c), '$' + c.toString(16).toUpperCase(), show(c));
    }
    out += '| ' + cells.join(' | ') + ' |\n';
  }
  return out;
}
if (process.argv.includes('--update')) { emit(table(32, 127), 'tools/charsets.js ascii'); emit(table(160, 255), 'tools/charsets.js iso'); }
else process.stdout.write(table(32, 127) + '\n' + table(160, 255));

// --update FILE: the text between FILE's markers (<!-- generated: TAG --> and <!-- end generated: TAG -->) made this
// output
function emit(text, tag) {
  const f = process.argv[process.argv.indexOf('--update') + 1], a = '<!-- generated: ' + tag + ' -->\n\n', b = '\n<!-- end generated: ' + tag + ' -->';
  const s = fs.readFileSync(f, 'utf8').split(String.fromCharCode(13)).join(''), p = s.indexOf(a), q = s.indexOf(b, p);
  if (p < 0 || q < 0) throw new Error(f + ': no markers for ' + tag);
  fs.writeFileSync(f, s.slice(0, p + a.length) + text.replace(/\n+$/, '\n') + s.slice(q));
}
