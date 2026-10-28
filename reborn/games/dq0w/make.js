// make.js - (dq0w) the world's data, after its build (build.js calls build): the catalogue's file (obj/games/
// dq0w.hyx.cat, ld65's: the kinds' 8K, the hub's KIND_BANK, then the catalogue's bank, CAT_BANK) given the words of the
// kinds' names, each once, after the kinds (items.h's KIND_WORDS: the UI's world words for the command parser, the
// parser's lists being words); and the outfits (games/dq0/outfits.txt: the character's starting outfits and the
// denizens' gear, the outfits' plan's) compiled into the world's data file, obj/games/dq0o.dat (on the card as
// lib/dq0/dq0o.dat), the kinds by their names in the catalogue as it's linked.
'use strict';
const fs = require('fs');
const path = require('path');
const outfits = require('./outfits.js');
const tables = require('../../sim/tools/commandtables.js');
const { loadRules } = require('../../sim/tools/commandcompile.js');

const KIND = 37;                                        // (items.h's struct kind: its size)

// A #define's number from a header
function define(file, name) {
  const m = new RegExp('#define\\s+' + name + '\\s+(\\d+)').exec(fs.readFileSync(file, 'latin1'));
  if (!m) throw new Error(file + ': no ' + name);
  return +m[1];
}

function build({ dir, objdir, say = () => {} }) {
  const games = path.join(dir, '..');
  const nkinds = define(path.join(games, 'dq0', 'items.h'), 'NKINDS'), ntk = define(path.join(games, 'dq0', 'theme.h'), 'NTKINDS');
  const f = path.join(objdir, '..', 'dq0w.hyx.cat');
  const cat = fs.readFileSync(f);
  const names = [];
  for (let k = 0; k < nkinds; k++) {                  // (The theme's eight, theme.h's KI_THEME on, empty here)
    const r = cat.subarray(k * KIND, k * KIND + 14);
    names.push(r.subarray(0, r.indexOf(0) < 0 ? 14 : r.indexOf(0)).toString('latin1'));
  }
  // The words, each once, those the parser's dictionary has (a word it hasn't is a thing's name to it anyway: the
  // world's words only mark its own, dq0c's marklist)
  const grammar = path.join(games, 'dq0', 'commands.ebnf'), rules = loadRules(grammar);
  const g = tables.encodeGrammar(rules, tables.openNames(grammar)), dict = new Set(tables.indexWords(tables.encodeIndex(rules, g), g));
  const seen = new Set(), words = [];
  for (const n of names) for (const w of n.split(' ')) if (w && dict.has(w) && !seen.has(w)) { seen.add(w); words.push(w); }
  const text = Buffer.from(words.map(w => w + ' ').join('') + '\0', 'latin1');
  const at = nkinds * KIND, kin = (at + text.length + 63) & ~63;   // (The kinds' part of the file, whole pieces: wfile.c's)
  if (kin > 0x1580) throw new Error('dq0w.cat: the kinds\' words past the kinds\' bank (' + (at + text.length) + ' bytes)');
  if (text.length > 1024 - 200) throw new Error('dq0w.cat: the kinds\' words ' + text.length + ' bytes, past the parser\'s room less the theme\'s (hub.h\'s CQ_WORDS)');
  if (cat.readUInt16LE(18)) throw new Error('dq0w.cat: the first kind\'s cap_weight not 0 (the kinds\' length goes there)');
  const file = Buffer.concat([cat.subarray(0, at), text, Buffer.alloc(kin - at - text.length), cat.subarray(at)]);
  file.writeUInt16LE(kin, 18);                          // (The kinds' length, in the first's (none's) cap_weight: wfile.c's)
  fs.writeFileSync(f, file);
  // The outfits
  const o = outfits.compile({ src: path.join(games, 'dq0', 'outfits.txt'), names, out: path.join(objdir, '..', 'dq0o.dat'),
    dens: path.join(games, 'dq0d', 'kinds.txt') });
  say('dq0w.cat: ' + names.filter(Boolean).length + ' kinds, their words ' + text.length + ' bytes; dq0o.dat: ' + o.sections + ' outfits (and ' + o.kinds + ' of the denizens\' kinds), ' + o.bytes + ' bytes');
  return { words: text.length, outfits: o };
}

module.exports = { build };
