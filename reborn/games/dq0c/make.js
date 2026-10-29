// make.js - the command parser's data file, after its build (build.js calls build): obj/games/dq0c.dat, the grammar,
// its index and the default aliases as sim/tools/commandtables.js makes them and the game's words as commandwords.js
// does, behind the header dq0c.s reads ("DQ0C", its version, the five parts' lengths: two bytes each): the grammar's
// counts and its words' length (the words themselves not: the index has them, and the RAM's room is short), the index, the aliases, the words' table (dq0c's RAM's, no more than dq0c.s's TABLES_MAX with
// the header), then the grammar's kinds and rules (its bank's, RULES_MAX at most); and hub.inc held to hub.h (the
// hub's places dq0c reads, and their constants: the build stops if they differ).
'use strict';
const fs = require('fs');
const path = require('path');
const tables = require('../../sim/tools/commandtables.js');
const { loadRules } = require('../../sim/tools/commandcompile.js');
const words = require('../../sim/tools/commandwords.js');

const DAT_VERSION = 4;                                            // (dq0c.s's DAT_VERSION)
const SAME = ['HUB_MAGIC', 'HUB_VERSION', 'HUBO_SEM', 'HUBO_TASK', 'HUBO_BEAT', 'HUBO_CQ', 'HUBO_QUIT', 'CQO_BANK',
  'CQO_LEN', 'CQO_RES', 'CQO_FACE', 'CQ_WORDS', 'CQ_CMD', 'CMD_SIZE', 'REQ_FREE', 'REQ_POSTED', 'REQ_DONE', 'CQ_NONE', 'CQ_MATCH', 'CQ_WORLD'];
const MEAN = ['CM_SIZE', 'CF_NOVERB', 'CF_HALF', 'CF_UNSAID'];                 // (cmdmean.s's, hub.h's: CMD_SIZE, CF_*)

// The grammar table's counts and words (its first part): their bytes (commandtables.js's layout: 11 bytes, then each
// word its length and its letters, 5 bits each)
function grammarHead(g) {
  let p = 11;
  for (let n = g[9] | g[10] << 8; n--;) p += 1 + Math.ceil(g[p] * 5 / 8);
  return p;
}

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
  const w = words.encode(words.load(), tables.indexWords(i, g));
  const u16 = n => [n & 255, n >> 8];
  const h = grammarHead(g), head = Buffer.from([...g.subarray(0, 11), ...u16(h - 11)]), body = g.subarray(h);
  const hdr = Buffer.from([0x44, 0x51, 0x30, 0x43, DAT_VERSION, ...[head, i, a, body, w].flatMap(x => u16(x.length))]);
  const dat = Buffer.concat([hdr, head, i, a, body, w]);
  const lim = incNames(fs.readFileSync(path.join(dir, 'dq0c.s'), 'latin1'));
  const mean = incNames(fs.readFileSync(path.join(dir, 'cmdmean.s'), 'latin1'));
  const bad = MEAN.filter(n => mean[n] === undefined || mean[n] !== (n === 'CM_SIZE' ? hub.CMD_SIZE : hub[n]));
  if (bad.length) throw new Error('games/dq0c/cmdmean.s and games/dq0/hub.h differ: ' + bad.join(', '));
  actionsSame(path.join(dir, '..', 'dq0'));
  const ram = hdr.length + head.length + i.length + a.length;
  if (hdr.length !== lim.DAT_HEAD) throw new Error('dq0c.dat: its header ' + hdr.length + " bytes, dq0c.s's DAT_HEAD " + lim.DAT_HEAD);
  if (!(ram <= lim.TABLES_MAX)) throw new Error('obj/games/dq0c.dat: ' + ram + " bytes for dq0c's RAM, more than dq0c.s's TABLES_MAX (" + lim.TABLES_MAX + ')');
  if (!(body.length <= lim.RULES_MAX)) throw new Error('obj/games/dq0c.dat: the rules ' + body.length + " bytes, more than dq0c.s's RULES_MAX (" + lim.RULES_MAX + ')');
  if (!(w.length <= lim.WORDS_MAX)) throw new Error('obj/games/dq0c.dat: the words ' + w.length + " bytes, more than dq0c.s's WORDS_MAX (" + lim.WORDS_MAX + ')');
  const match = incNames(fs.readFileSync(path.join(dir, 'cmdmatch.s'), 'latin1'));   // (Its rules counted, and its open
  if (!((g[5] | g[6] << 8) <= match.MAXRULES && (g[7] | g[8] << 8) - (g[5] | g[6] << 8) <= 255))   //   names: cmdmatch.s's)
    throw new Error('games/dq0/commands.ebnf: ' + (g[5] | g[6] << 8) + " rules, more than cmdmatch.s's MAXRULES (" + match.MAXRULES + ')');
  fs.mkdirSync(objdir, { recursive: true });
  fs.writeFileSync(path.join(objdir, '..', 'dq0c.dat'), dat);
  say('dq0c.dat: ' + dat.length + ' bytes: in the RAM ' + ram + ' (the grammar\'s words ' + head.length + ', its index ' + i.length +
    ', the aliases ' + a.length + '; room for ' + lim.TABLES_MAX + '), in banks the rules ' + body.length + ' (room for ' +
    lim.RULES_MAX + ') and the game\'s words and verbs ' + w.length + ' (room for ' + lim.WORDS_MAX + ')');
}

// actions.h's enum held to the catalogue (commands.actions' [actions], in order, VB_NONE first, NACTIONS last)
function actionsSame(game) {
  const cat = fs.readFileSync(path.join(game, 'commands.actions'), 'latin1').split(/\r?\n/);
  const want = ['VB_NONE'];
  let on = false;
  for (const l of cat) {
    if (l.startsWith('[')) { on = l.trim() === '[actions]'; continue; }
    if (on && /^VB_/.test(l)) want.push(l.split(/\s+/)[0]);
  }
  want.push('NACTIONS');
  const h = fs.readFileSync(path.join(game, 'actions.h'), 'latin1').replace(/\/\*[\s\S]*?\*\//g, ' ');
  const e = h.match(/enum\s*\{([^}]*)\}/);
  const have = e ? e[1].split(',').map(s => s.trim()).filter(Boolean) : [];
  const at = want.findIndex((n, k) => have[k] !== n);
  if (at >= 0 || have.length !== want.length) throw new Error('games/dq0/actions.h and commands.actions differ' +
    (at >= 0 ? ': ' + want[at] + ' (actions.h has ' + have[at] + ')' : ''));
}

module.exports = { build };
