// make.js - the command parser's data file, after its build (build.js calls build): obj/games/dq0c.dat, the grammar,
// its index and the default aliases as sim/tools/commandtables.js makes them, behind the header dq0c.s reads
// ("DQ0C", its version, the three tables' lengths: two bytes each), no larger than dq0c.s's TABLES_MAX; and hub.inc
// held to hub.h (the hub's places dq0c reads, and their constants: the build stops if they differ).
'use strict';
const fs = require('fs');
const path = require('path');
const tables = require('../../sim/tools/commandtables.js');
const { loadRules } = require('../../sim/tools/commandcompile.js');

const DAT_VERSION = 1;                                            // (dq0c.s's DAT_VERSION)
const SAME = ['HUB_MAGIC', 'HUB_VERSION', 'HUBO_SEM', 'HUBO_TASK', 'HUBO_BEAT', 'HUBO_CQ', 'HUBO_QUIT', 'CQO_BANK',
  'CQO_LEN', 'CQO_RES', 'REQ_FREE', 'REQ_POSTED', 'REQ_DONE', 'CQ_NONE', 'CQ_MATCH', 'CQ_WORLD'];

// A number as either file writes it: $5144, 0x5144, 51
const value = t => t.startsWith('$') ? parseInt(t.slice(1), 16) : t.startsWith('0x') ? parseInt(t.slice(2), 16) : parseInt(t, 10);

// hub.inc's NAME = value lines; hub.h's #define NAME value lines and its enum of CQ_* (their order their values)
function incNames(text) {
  const out = {};
  for (const l of text.split(/\r?\n/)) {
    const k = l.split(';')[0].split('=');
    if (k.length === 2 && k[0].trim()) out[k[0].trim()] = value(k[1].trim());
  }
  return out;
}
function hubNames(text) {
  const out = {};
  for (const l of text.split(/\r?\n/)) {
    const w = l.trim().split(/ +/);
    if (w[0] === '#define' && w.length >= 3) out[w[1]] = value(w[2]);
    const e = l.match(/^enum { (CQ_[A-Z_, ]+) };/);
    if (e) e[1].split(',').map(s => s.trim()).forEach((n, i) => { out[n] = i; });
  }
  return out;
}

function build({ dir, objdir, say = () => {} }) {
  const inc = incNames(fs.readFileSync(path.join(dir, 'hub.inc'), 'latin1'));
  const hub = hubNames(fs.readFileSync(path.join(dir, '..', 'dq0', 'hub.h'), 'latin1'));
  const differ = SAME.filter(n => inc[n] === undefined || hub[n] === undefined || inc[n] !== hub[n]);
  if (differ.length) throw new Error('games/dq0c/hub.inc and games/dq0/hub.h differ: ' + differ.map(n => n + ' ' + inc[n] + ' ' + hub[n]).join(', '));
  const grammar = path.join(dir, '..', 'dq0', 'commands.ebnf');
  const rules = loadRules(grammar);
  const g = tables.encodeGrammar(rules, tables.openNames(grammar)), i = tables.encodeIndex(rules, g), a = tables.encodeAliases();
  const u16 = n => [n & 255, n >> 8];
  const dat = Buffer.concat([Buffer.from([0x44, 0x51, 0x30, 0x43, DAT_VERSION, ...u16(g.length), ...u16(i.length), ...u16(a.length)]), g, i, a]);
  const max = incNames(fs.readFileSync(path.join(dir, 'dq0c.s'), 'latin1')).TABLES_MAX;
  if (!(dat.length <= max)) throw new Error('obj/games/dq0c.dat: ' + dat.length + ' bytes, more than dq0c.s\'s TABLES_MAX (' + max + ')');
  fs.mkdirSync(objdir, { recursive: true });
  fs.writeFileSync(path.join(objdir, '..', 'dq0c.dat'), dat);
  say('dq0c.dat: ' + dat.length + ' bytes (the grammar ' + g.length + ', its index ' + i.length + ', the aliases ' + a.length +
    '; room for ' + max + ')');
}

module.exports = { build };
