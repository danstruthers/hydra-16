// make.js - the command parser's data file, after its build (build.js calls build): obj/games/dq0c.dat, the grammar,
// its index and the default aliases as sim/tools/commandtables.js makes them and the game's words as commandwords.js
// does, behind the header dq0c.s reads ("DQ0C", its version, the five parts' lengths: two bytes each): the grammar's
// counts and its words' length (the words themselves not: the index has them, and the RAM's room is short), the index, the aliases, the words' table (dq0c's RAM's, no more than dq0c.s's TABLES_MAX with
// the header), then the grammar's kinds and rules (its bank's, RULES_MAX at most).  (The hub's places dq0c reads, and
// their constants, and the actions' enum are the build's, from hub.h and commands.actions: games/dq0/hubgen.js.)
'use strict';
const fs = require('fs');
const path = require('path');
const tables = require('../../sim/tools/commandtables.js');
const { loadRules } = require('../../sim/tools/commandcompile.js');
const words = require('../../sim/tools/commandwords.js');

const DAT_VERSION = 5;                                            // (dq0c.s's DAT_VERSION)

// The grammar table's counts and words (its first part): their bytes (commandtables.js's layout: 11 bytes, then each
// word its length and its letters, 5 bits each)
function grammarHead(g) {
  let p = 11;
  for (let n = g[9] | g[10] << 8; n--;) p += 1 + Math.ceil(g[p] * 5 / 8);
  return p;
}

// A number as the assembly writes it: $5144, 51
const value = t => t.startsWith('$') ? parseInt(t.slice(1), 16) : parseInt(t, 10);

// An assembly file's NAME = number lines
function incNames(text) {
  const out = {};
  for (const l of text.split(/\r?\n/)) {
    const k = l.split(';')[0].split('=');
    if (k.length === 2 && k[0].trim() && /^\$?[0-9A-Fa-f]+$/.test(k[1].trim())) out[k[0].trim()] = value(k[1].trim());
  }
  return out;
}

function build({ dir, objdir, say = () => {} }) {
  const grammar = path.join(dir, '..', 'dq0', 'commands.ebnf');
  const rules = loadRules(grammar);
  const g = tables.encodeGrammar(rules, tables.openNames(grammar)), i = tables.encodeIndex(rules, g), a = tables.encodeAliases();
  const w = words.encode(words.load(), tables.indexWords(i, g));
  const u16 = n => [n & 255, n >> 8];
  const h = grammarHead(g), head = Buffer.from([...g.subarray(0, 11), ...u16(h - 11)]), body = g.subarray(h);
  const hdr = Buffer.from([0x44, 0x51, 0x30, 0x43, DAT_VERSION, ...[head, i, a, body, w].flatMap(x => u16(x.length))]);
  const dat = Buffer.concat([hdr, head, i, body, w, a]);  // (The aliases after the words, in their bank: 2026-11-17)
  const lim = incNames(fs.readFileSync(path.join(dir, 'dq0c.s'), 'latin1'));
  const ram = hdr.length + head.length + i.length;
  if (hdr.length !== lim.DAT_HEAD) throw new Error('dq0c.dat: its header ' + hdr.length + " bytes, dq0c.s's DAT_HEAD " + lim.DAT_HEAD);
  if (!(ram <= lim.TABLES_MAX)) throw new Error('obj/games/dq0c.dat: ' + ram + " bytes for dq0c's RAM, more than dq0c.s's TABLES_MAX (" + lim.TABLES_MAX + ')');
  if (!(body.length <= lim.RULES_MAX)) throw new Error('obj/games/dq0c.dat: the rules ' + body.length + " bytes, more than dq0c.s's RULES_MAX (" + lim.RULES_MAX + ')');
  if (!(w.length + a.length < lim.WORDS_MAX)) throw new Error('obj/games/dq0c.dat: the words and the aliases ' + (w.length + a.length) + " bytes, more than dq0c.s's WORDS_MAX (" + lim.WORDS_MAX + ')');
  const match = incNames(fs.readFileSync(path.join(dir, 'cmdmatch.s'), 'latin1'));   // (Its rules counted, and its open
  const ient = i[9] | i[10] << 8;                // (The index's entries, past which cm_init refuses it, the task gone silently)
  if (!(ient <= match.MAXENT - match.MAXDYN)) throw new Error('obj/games/dq0c.dat: ' + ient + " dictionary entries, more than cmdmatch.s's MAXENT - MAXDYN (" + (match.MAXENT - match.MAXDYN) + ')');
  if (!((g[5] | g[6] << 8) <= match.MAXRULES && (g[7] | g[8] << 8) - (g[5] | g[6] << 8) <= 255))   //   names: cmdmatch.s's)
    throw new Error('games/dq0/commands.ebnf: ' + (g[5] | g[6] << 8) + " rules, more than cmdmatch.s's MAXRULES (" + match.MAXRULES + ')');
  fs.mkdirSync(objdir, { recursive: true });
  fs.writeFileSync(path.join(objdir, '..', 'dq0c.dat'), dat);
  say('dq0c.dat: ' + dat.length + ' bytes: in the RAM ' + ram + ' (the grammar\'s words ' + head.length + ', its index ' + i.length +
    '; room for ' + lim.TABLES_MAX + '), in banks the rules ' + body.length + ' (room for ' + lim.RULES_MAX + ') and the game\'s words and verbs ' +
    w.length + ', the aliases ' + a.length + ' (room for ' + lim.WORDS_MAX + ')');
}

module.exports = { build };
