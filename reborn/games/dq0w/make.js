// make.js - (dq0w) the world's data, after its build (build.js calls build): its overlays' file packed
// (obj/games/dq0w.ovp, ../dq0/ovlpack.js's: ld65's cut after the last overlay that has code, each bank's fill left
// out, so the world reads no more than it needs as it starts); the catalogue's file (obj/games/
// dq0w.hyx.cat, ld65's: the kinds' 8K, the hub's KIND_BANK, then the catalogue's bank, CAT_BANK) given the words of the
// kinds' names, each once, after the kinds (items.h's KIND_WORDS: the UI's world words for the command parser, the
// parser's lists being words); and the outfits (each ruleset's games/REFEREE/outfits.txt, the referees in
// games/dq0/rsmeta.h's RULESETS order: the character's starting outfits, the ruleset's; then games/dq0/outfits.txt: the
// denizens' gear, the outfits' plan's) compiled into the world's data file, obj/games/dq0o.dat (on the card as
// lib/dq0/dq0o.dat), the kinds by their names in the catalogue as it's linked.
'use strict';
const fs = require('fs');
const path = require('path');
const outfits = require('./outfits.js');
const tables = require('../../sim/tools/commandtables.js');
const { loadRules } = require('../../sim/tools/commandcompile.js');

const B = require('../../tools/dq0build.js'), H = B.hub(), C = H.C;  // (The headers' numbers, the build's: ../dq0/hubgen.js)
const KIND = H.size('kind'), NAME = H.size('kind', 'name'), CAP_WEIGHT = H.off('kind', 'cap_weight');   // (items.h's struct kind)

// The overlays' count one, the build's (OVLS, dq0w.cfg's OV1 ...: build.js's overlaysCounted): things.h's OVBANKS and
// bank.s's it, and the world's six banks after them in _ovbank's room and ovbank[] (a 22nd overlay with bank.s's
// table left at 25 overran it, 2026-10-31: "Nearly ready" hung)
function checkOverlays(dir) {
  const hfile = fs.readFileSync(path.join(dir, 'things.h'), 'latin1'), s = fs.readFileSync(path.join(dir, 'bank.s'), 'latin1');
  if (!/#define\s+OVBANKS\s+OVLS\b/.test(hfile) || !/^OVBANKS\s*=\s*OVLS\b/m.test(s) || !/^_ovbank:\s+\.res\s+OVBANKS \+ 6\b/m.test(s) ||
    !/ovbank\[OVBANKS \+ 6\]/.test(hfile))
    throw new Error('dq0w: the overlays\' count not the build\'s (OVLS) in things.h\'s OVBANKS and bank.s\'s, or _ovbank not OVBANKS + 6');
  // Each overlay but its file's last filled out to its 8K (fill = yes): ld65 writes an unfilled one's bytes only, so
  // every bank after it in its file is read in shifted (2026-11-10: OV28 left unfilled, a 29th .ovl bank after it, and
  // the parts the build moved there ran garbage, the game stalled at its first step)
  const banks = [...fs.readFileSync(path.join(dir, 'dq0w.cfg'), 'latin1').matchAll(/^\s*OV(\d+):\s*file\s*=\s*"([^"]*)"([^;\n]*)/gm)];
  banks.forEach(([, n, f, rest], i) => {
    if (banks.slice(i + 1).some(b => b[2] === f) && !/\bfill\s*=\s*yes\b/.test(rest))
      throw new Error('dq0w.cfg: OV' + n + ' not filled (fill = yes), with more of "' + f + '"\'s overlays after it');
  });
}

// The holders' compartments (kinds.c's comps, items.h's struct comp: CATCOMP, the catalogue's bank's start, so at the
// kinds' end in the file as ld65 writes it) held to the kinds (2026-11-15, COMMANDS-CRITIQUE.md's step 6b): a kind that
// holds things (a container, a belt, a surface) has its rows together, in the kinds' order, NCOMP at most, and its first
// is its record's own (prep by its flags, its keeps but KF_ON and KF_FIXED, cap_bulk, cap_weight, slots); a kind that
// holds nothing has none.  Their count
function compsSame(cat, at, nkinds, names) {
  const K_CONTAINER = 1, K_BELT = 2, KF_ON = 0x10, KF_FIXED = 0x80, CP = H.enums.find(e => e[0] === 'CP_IN');
  const ROW = H.size('comp'), o = n => H.off('comp', n), f = n => H.off('kind', n), seen = new Map();
  let r = at, last = 0, rows = 0;
  for (; cat[r + o('kind')]; r += ROW, ++rows) {
    const k = cat[r + o('kind')];
    if (k < last) throw new Error('dq0w.cat: the compartments not in the kinds\' order (' + names[k] + ')');
    seen.set(k, (seen.get(k) || 0) + 1);
    if (seen.get(k) > C.NCOMP) throw new Error('dq0w.cat: ' + names[k] + ' has more than NCOMP compartments');
    last = k;
    if (seen.get(k) > 1) {                                // (A lid's top: its room and the weight it bears by its holder's size, items.h's LID_BULK)
      const CN = H.enums.find(e => e[0] === 'CN_NONE'), b = cat[k * KIND + f('bulk')];
      if ((cat[r + o('noun')] & 15) === CN.indexOf('CN_LID') && (cat[r + o('bulk')] !== (b >> 2) || cat.readUInt16LE(r + o('weight')) !== (b >> 2) * 10))
        throw new Error('dq0w.cat: ' + names[k] + '\'s lid\'s top not by its size (' + b + ' pints: LID_BULK, LID_WEIGHT)');
      continue;
    }
    const q = k * KIND, fl = cat[q + f('flags')], kp = cat[q + f('keeps')];
    const want = [fl & K_BELT ? CP.indexOf('CP_HUNG') : kp & KF_ON ? CP.indexOf('CP_ON') : CP.indexOf('CP_IN'), kp & ~(KF_ON | KF_FIXED),
      cat[q + f('cap_bulk')], cat.readUInt16LE(q + f('cap_weight')), cat[q + f('slots')]];
    const have = [cat[r + o('prep')], cat[r + o('keeps')], cat[r + o('bulk')], cat.readUInt16LE(r + o('weight')), cat[r + o('fam')]];
    if (want.some((v, i) => v !== have[i])) throw new Error('dq0w.cat: ' + names[k] + '\'s first compartment not its record\'s (prep, keeps, ' +
      'bulk, weight, family: ' + have.join(' ') + ', its record ' + want.join(' ') + ')');
  }
  for (let k = 1; k < nkinds; ++k) {
    const q = k * KIND, holds = cat[q + f('flags')] & (K_CONTAINER | K_BELT) || cat[q + f('keeps')] & KF_ON;
    if (names[k] && !!holds !== seen.has(k)) throw new Error('dq0w.cat: ' + names[k] + (holds ? ' holds things but has no compartment' : ' has compartments but holds nothing'));
  }
  return rows;
}

function build({ dir, objdir, say = () => {} }) {
  checkOverlays(dir);
  B.pack(objdir, 'dq0w');                                 // (Its overlays' file packed: obj/games/dq0w.ovp)
  const games = path.join(dir, '..');
  const nkinds = C.NKINDS;
  const f = path.join(objdir, '..', 'dq0w.hyx.cat');
  const cat = fs.readFileSync(f);
  const names = [];
  for (let k = 0; k < nkinds; k++) {                  // (The theme's eight, theme.h's KI_THEME on, empty here)
    const r = cat.subarray(k * KIND, k * KIND + NAME);
    names.push(r.subarray(0, r.indexOf(0) < 0 ? NAME : r.indexOf(0)).toString('latin1'));
  }
  // The words, each once, those the parser's dictionary has (a word it hasn't is a thing's name to it anyway: the
  // world's words only mark its own, dq0c's marklist)
  const grammar = path.join(games, 'dq0', 'commands.ebnf'), rules = loadRules(grammar);
  const g = tables.encodeGrammar(rules, tables.openNames(grammar)), dict = new Set(tables.indexWords(tables.encodeIndex(rules, g), g));
  const seen = new Set(), words = [];
  for (const n of names) for (const w of n.split(' ')) if (w && dict.has(w) && !seen.has(w)) { seen.add(w); words.push(w); }
  // (The compartments' own names' words not here: the kinds' bank has no room for them, the parser takes a word of its own
  // grammar's before a name's last as part of it: "the side pocket", "the false bottom")
  const text = Buffer.from(words.map(w => w + ' ').join('') + '\0', 'latin1');
  const at = nkinds * KIND, kin = (at + text.length + 63) & ~63;   // (The kinds' part of the file, whole pieces: wfile.c's)
  if (at + text.length > 0x1E3F || kin > 0x1E40) throw new Error('dq0w.cat: the kinds and their words into the outfits\' list (outfit.h\'s OF_N, ' + (at + text.length) + ' bytes)');
  if (text.length > C.CQ_WORDS - 200) throw new Error('dq0w.cat: the kinds\' words ' + text.length + ' bytes, past the parser\'s room less the theme\'s (hub.h\'s CQ_WORDS)');
  if (cat.length - at > 0x1900) throw new Error('dq0w.cat: the catalogue\'s bank\'s code ' + (cat.length - at) + ' bytes, into the outfits\' index (outfit.h\'s OF_INDEX)');
  // (The world's deities' names in the last two kinds' records, none yet (items.h's GODS_AT), a 0 first, so a name that's empty: dq0d's
  // make's, made first, spaces between, 0-ended; the UI's for the parser, so no file of their own to open as it starts, an
  // open on the card some 700,000 cycles, 2026-11-04)
  const gods = Buffer.from('\0' + fs.readFileSync(path.join(objdir, '..', 'dq0g.txt'), 'latin1') + '\0', 'latin1');
  if (gods.length > 2 * KIND || names[nkinds - 1] || names[nkinds - 2]) throw new Error('dq0w.cat: the deities\' ' + gods.length + ' bytes, or the last two kinds taken (items.h\'s GODS_AT)');
  if (cat.readUInt16LE(CAP_WEIGHT)) throw new Error('dq0w.cat: the first kind\'s cap_weight not 0 (the kinds\' length goes there)');
  const comps = compsSame(cat, at, nkinds, names);

  const file = Buffer.concat([cat.subarray(0, at), text, Buffer.alloc(kin - at - text.length), cat.subarray(at)]);
  file.writeUInt16LE(kin, CAP_WEIGHT);                          // (The kinds' length, in the first's (none's) cap_weight: wfile.c's)
  gods.copy(file, (nkinds - 2) * KIND);
  fs.writeFileSync(f, file);
  // The outfits
  const referees = [...(/#define\s+RULESETS\s+(.*)/.exec(fs.readFileSync(path.join(games, 'dq0', 'rsmeta.h'), 'latin1')) || ['', ''])[1]
    .matchAll(/\{\s*"([^"]+)"/g)].map(m => m[1]);   // (The rulesets' referees: their outfits first, those that have one)
  const o = outfits.compile({ src: [...referees.map(r => path.join(games, r, 'outfits.txt')), path.join(games, 'dq0', 'outfits.txt')],
    names, out: path.join(objdir, '..', 'dq0o.dat'),
    dens: path.join(games, 'dq0d', 'kinds.txt') });
  say('dq0w.cat: ' + names.filter(Boolean).length + ' kinds, ' + comps + ' compartments, their words ' + text.length + ' bytes; dq0o.dat: ' + o.sections + ' outfits (and ' + o.kinds + ' of the denizens\' kinds), ' + o.bytes + ' bytes');
  return { words: text.length, outfits: o };
}

module.exports = { build };
