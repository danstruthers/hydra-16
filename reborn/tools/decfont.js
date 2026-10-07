#!/usr/bin/env node
// ****************************************************************************
// decfont.js - the DEC Special Graphics (the VT100's line drawing: ESC ( 0, its $5F-$7E) as the first 32 glyphs
// of the console's fonts (docs/plans/WINDOWS.md, W2): the console keeps them in a window's cells as $00-$1F, and
// vid's terminal shows them there.  No byte the terminal prints reaches those glyphs otherwise ($00-$1F are its
// controls), so a font loses nothing: the ISO-8859-15 font's reversed letters, cp437's smileys and the rest were
// never shown.  The £ and the middle dot are each font's own (its $A3 and $B7 in ISO-8859-15, $9C and $FA in cp437).
//
// Usage: node tools/decfont.js [FONT[:iso|:cp437] ...]
//   (none: modules/vid/iso8859-15.fnt and romfs/lib/font/cp437, each written in place)
'use strict';
const fs = require('fs');
const path = require('path');

// Each glyph 8 rows of 8 pixels, # set; in the DEC set's order ($5F-$7E)
const ART = {
  blank: ['........', '........', '........', '........', '........', '........', '........', '........'],
  diamond: ['........', '...#....', '..###...', '.#####..', '..###...', '...#....', '........', '........'],
  checker: ['#.#.#.#.', '.#.#.#.#', '#.#.#.#.', '.#.#.#.#', '#.#.#.#.', '.#.#.#.#', '#.#.#.#.', '.#.#.#.#'],
  degree: ['..##....', '.#..#...', '..##....', '........', '........', '........', '........', '........'],
  plusminus: ['...#....', '...#....', '.#####..', '...#....', '...#....', '........', '.#####..', '........'],
  lr: ['...#....', '...#....', '...#....', '####....', '........', '........', '........', '........'],  // ┘
  ur: ['........', '........', '........', '####....', '...#....', '...#....', '...#....', '...#....'],  // ┐
  ul: ['........', '........', '........', '...#####', '...#....', '...#....', '...#....', '...#....'],  // ┌
  ll: ['...#....', '...#....', '...#....', '...#####', '........', '........', '........', '........'],  // └
  cross: ['...#....', '...#....', '...#....', '########', '...#....', '...#....', '...#....', '...#....'],
  scan1: ['########', '........', '........', '........', '........', '........', '........', '........'],
  scan3: ['........', '########', '........', '........', '........', '........', '........', '........'],
  scan5: ['........', '........', '........', '########', '........', '........', '........', '........'],  // ─
  scan7: ['........', '........', '........', '........', '........', '########', '........', '........'],
  scan9: ['........', '........', '........', '........', '........', '........', '........', '########'],
  lt: ['...#....', '...#....', '...#....', '...#####', '...#....', '...#....', '...#....', '...#....'],  // ├
  rt: ['...#....', '...#....', '...#....', '####....', '...#....', '...#....', '...#....', '...#....'],  // ┤
  bt: ['...#....', '...#....', '...#....', '########', '........', '........', '........', '........'],  // ┴
  tt: ['........', '........', '........', '########', '...#....', '...#....', '...#....', '...#....'],  // ┬
  vbar: ['...#....', '...#....', '...#....', '...#....', '...#....', '...#....', '...#....', '...#....'],
  le: ['....##..', '..##....', '##......', '..##....', '....##..', '........', '######..', '........'],
  ge: ['##......', '..##....', '....##..', '..##....', '##......', '........', '######..', '........'],
  pi: ['........', '.######.', '..#..#..', '..#..#..', '..#..#..', '..#..#..', '........', '........'],
  ne: ['........', '.....#..', '.######.', '....#...', '.######.', '..#.....', '........', '........'],
};
// Two letters a glyph (the control pictures: HT FF CR LF NL VT), small: the first at the top left, the second at
// the bottom right
const LETTER = { H: ['#.#', '###', '#.#'], T: ['###', '.#.', '.#.'], F: ['###', '##.', '#..'], C: ['###', '#..', '###'],
  R: ['##.', '###', '#.#'], L: ['#..', '#..', '###'], N: ['###', '#.#', '#.#'], V: ['#.#', '#.#', '.#.'] };
const pair = (a, b) => {
  const rows = Array.from({ length: 8 }, () => '........'.split(''));
  LETTER[a].forEach((r, y) => [...r].forEach((c, x) => { if (c === '#') rows[y][x] = '#'; }));
  LETTER[b].forEach((r, y) => [...r].forEach((c, x) => { if (c === '#') rows[y + 4][x + 4] = '#'; }));
  return rows.map(r => r.join(''));
};
const bits = art => Buffer.from(art.map(r => parseInt(r.replace(/#/g, '1').replace(/\./g, '0'), 2)));

// The set's 32, $5F-$7E: a font's own glyph (by code) for the £ and the middle dot
function glyphs(own) {
  return [ART.blank, ART.diamond, ART.checker, pair('H', 'T'), pair('F', 'F'), pair('C', 'R'), pair('L', 'F'),
    ART.degree, ART.plusminus, pair('N', 'L'), pair('V', 'T'), ART.lr, ART.ur, ART.ul, ART.ll, ART.cross,
    ART.scan1, ART.scan3, ART.scan5, ART.scan7, ART.scan9, ART.lt, ART.rt, ART.bt, ART.tt, ART.vbar,
    ART.le, ART.ge, ART.pi, ART.ne, own.pound, own.dot].map(g => Buffer.isBuffer(g) ? g : bits(g));
}

function patch(file, kind) {
  const font = fs.readFileSync(file);
  if (font.length !== 2048) throw new Error(file + ': not a font of 256 8 x 8 glyphs');
  const at = c => font.subarray(c * 8, c * 8 + 8);
  const own = kind === 'cp437' ? { pound: Buffer.from(at(0x9C)), dot: Buffer.from(at(0xFA)) }
    : { pound: Buffer.from(at(0xA3)), dot: Buffer.from(at(0xB7)) };
  glyphs(own).forEach((g, i) => g.copy(font, i * 8));
  fs.writeFileSync(file, font);
  console.log(file + ': the DEC Special Graphics in glyphs $00-$1F (' + kind + ')');
}

const ROOT = path.join(__dirname, '..');
const args = process.argv.slice(2);
const list = args.length ? args.map(a => { const [f, k] = a.split(':'); return [f, k || (/cp437/i.test(f) ? 'cp437' : 'iso')]; })
  : [[path.join(ROOT, 'modules', 'vid', 'iso8859-15.fnt'), 'iso'], [path.join(ROOT, 'romfs', 'lib', 'font', 'cp437'), 'cp437']];
for (const [f, k] of list) patch(f, k);
