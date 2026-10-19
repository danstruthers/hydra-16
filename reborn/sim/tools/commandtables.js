// commandtables.js - the grammar and the default aliases as tables, the forms the 65C02 compiler reads (host side).
// encodeGrammar turns the rules of commands.ebnf into bytes; encodeAliases turns the default aliases (commands.aliases)
// into their own bytes, so the grammar and the aliases can sit in different banks. decodeGrammar and decodeAliases read
// them back to what commandcompile.js uses. The corpus must give the same results from the bytes as from the text.
// Grammar layout (version 7): magic "DQ0G"; the rule count; the name count; the word count and the words (5 bits a
// letter, a length byte); one kind byte per open name (the matcher's K_ codes, OPEN_KIND); then the rules, one after
// another. The names are not in the table: a rule or open name is its number in grammarNames' order (rules, then open
// names). A composite node (sequence, alternative, option, repeat) is [tag, size, count or child, ...]: the size is one
// byte under 128, else two bytes with the top bit of the first set, counted from the tag, so a reader steps over the
// node without walking it. A leaf (name, word list, quote) has no size: its length is in its tag or count.
// Alias layout: magic "DQ0A", version 1; the count; per alias: the word, the verb, and a flag (1: stands anywhere).
//   node sim/tools/commandtables.js [grammar.bin] [aliases.bin]   write the tables and print their sizes
//   node sim/tools/commandtables.js --check                       encode, decode, and run the corpus through the tables
'use strict';
const fs = require('fs');
const path = require('path');
const { loadRules, Compiler, buildContext, DEFAULT_WORLD, checkCorpus, DEFAULT_ALIASES, DEFAULT_ANYWHERE } = require('./commandcompile.js');

const GAME = path.join(__dirname, '..', '..', 'games', 'dq0');
const MAGIC = [0x44, 0x51, 0x30, 0x47]; // "DQ0G"
const MAGIC_ALIAS = [0x44, 0x51, 0x30, 0x41]; // "DQ0A"
const VERSION = 7;
const VERSION_ALIAS = 1;
const TAG = { seq: 1, alt: 2, opt: 3, rep: 4, nt: 5, term: 6, quote: 7 };

// The open names are declared on the header line of the grammar: "open: ...".
function openNames(file) {
  const m = fs.readFileSync(file, 'utf8').match(/open:\s*([^\n*]*)/);
  return m ? m[1].trim().split(/\s+/).filter(Boolean) : [];
}

// The words are packed: 5 bits a letter (a to z, then ! = - ') and a length byte.
const PACK = "abcdefghijklmnopqrstuvwxyz!=-'";
function packWord(w) {
  const bits = [];
  for (const c of w) {
    const v = PACK.indexOf(c);
    if (v < 0) throw new Error('a letter the word alphabet lacks: ' + c);
    for (let i = 0; i < 5; i++) bits.push((v >> i) & 1);
  }
  const out = [w.length];
  for (let k = 0; k < bits.length; k += 8) {
    let b = 0;
    for (let i = 0; i < 8 && k + i < bits.length; i++) b |= bits[k + i] << i;
    out.push(b);
  }
  return out;
}

// The names of the grammar in the table's order: the rules, then the open names that are not rules.
function grammarNames(rules, open) {
  return [...rules.keys(), ...open.filter(n => !rules.has(n))];
}

// The kind of each open name, one byte: the matcher's K_ codes in cmdmatch.c (quoted-text is a rule, so not listed).
const OPEN_KIND = { name: 0, alias: 0, adj: 1, number: 2, ordinal: 3, 'task-name': 4, 'spell-word': 5, text: 6,
  'meta-topic': 6, 'name-word': 7, 'deity-name': 8, 'pretense-word': 9 };

function encodeGrammar(rules, open) {
  const names = grammarNames(rules, open);
  const nameId = new Map(names.map((n, i) => [n, i]));
  const words = [];
  const wordId = new Map();
  const wid = w => {
    if (!wordId.has(w)) { wordId.set(w, words.length); words.push(w); }
    return wordId.get(w);
  };
  const u16 = v => [v & 255, v >> 8];
  // a composite's size: one byte under 128, else two (the first with its top bit set), counted from the tag
  const composite = (tag, head, kids) => {
    const body = [...head, ...kids];
    const small = 2 + body.length;
    if (small < 128) return [tag, small, ...body];
    const size = 3 + body.length;
    if (size > 32767) throw new Error('node too large');
    return [tag, 0x80 | (size >> 8), size & 255, ...body];
  };
  const node = n => {
    switch (n.t) {
      case 'seq': return composite(TAG.seq, [n.items.length], n.items.flatMap(node));
      case 'alt': return composite(TAG.alt, [n.opts.length], n.opts.flatMap(node));
      case 'opt': return composite(TAG.opt, [], node(n.x));
      case 'rep': return composite(TAG.rep, [], node(n.x));
      case 'nt':
        if (!nameId.has(n.name)) throw new Error('undefined name <' + n.name + '>');
        return [TAG.nt, ...u16(nameId.get(n.name))];
      case 'term':
        if (n.words.length > 255) throw new Error('term too long');
        return [TAG.term, n.words.length, ...n.words.flatMap(w => u16(wid(w)))];
      case 'quote': return [TAG.quote];
      default: throw new Error('bad node ' + n.t);
    }
  };
  const ruleBytes = [];
  for (const tree of rules.values()) ruleBytes.push(...node(tree));
  const kinds = names.slice(rules.size).map(n => {
    if (!(n in OPEN_KIND)) throw new Error('no matcher kind for the open name ' + n);
    return OPEN_KIND[n];
  });
  const head = [...MAGIC, VERSION, ...u16(rules.size), ...u16(names.length), ...u16(words.length)];
  for (const w of words) head.push(...packWord(w));
  return Buffer.from([...head, ...kinds, ...ruleBytes]);
}

function encodeAliases(aliases = DEFAULT_ALIASES, anywhere = DEFAULT_ANYWHERE) {
  const text = s => { if (s.length > 255) throw new Error('word too long: ' + s); return [s.length, ...[...s].map(c => c.charCodeAt(0))]; };
  const al = [...aliases.entries()];
  if (al.length > 255) throw new Error('too many aliases');
  const out = [...MAGIC_ALIAS, VERSION_ALIAS, al.length];
  for (const [w, verb] of al) out.push(...text(w), ...text(verb), anywhere.has(w) ? 1 : 0);
  return Buffer.from(out);
}

function decodeGrammar(buf, names) {
  let p = 0;
  const u8 = () => buf[p++];
  const u16 = () => { const v = buf[p] | (buf[p + 1] << 8); p += 2; return v; };
  for (const m of MAGIC) if (u8() !== m) throw new Error('not a dq0 grammar table');
  if (u8() !== VERSION) throw new Error('grammar table version');
  const ruleCount = u16(), nameCount = u16();
  if (names.length !== nameCount) throw new Error('the names given do not match the table');
  const wordCount = u16();
  const words = [];
  for (let i = 0; i < wordCount; i++) {
    const n = u8();
    const bits = [];
    for (let k = 0; k < Math.ceil(5 * n / 8); k++) { const b = u8(); for (let j = 0; j < 8; j++) bits.push((b >> j) & 1); }
    let w = '';
    for (let c = 0; c < n; c++) { let v = 0; for (let j = 0; j < 5; j++) v |= bits[5 * c + j] << j; w += PACK[v]; }
    words.push(w);
  }
  p += nameCount - ruleCount;   // the kind of each open name: the matcher's, not needed for the rules
  const node = () => {
    const start = p;
    const tag = u8();
    switch (tag) {
      case TAG.seq: case TAG.alt: case TAG.opt: case TAG.rep: {
        const b = u8();
        const end = start + ((b & 0x80) ? (((b & 0x7F) << 8) | u8()) : b);
        let result;
        if (tag === TAG.seq || tag === TAG.alt) {
          const n = u8();
          const kids = [];
          for (let i = 0; i < n; i++) kids.push(node());
          result = tag === TAG.seq ? { t: 'seq', items: kids } : { t: 'alt', opts: kids };
        } else {
          result = tag === TAG.opt ? { t: 'opt', x: node() } : { t: 'rep', x: node() };
        }
        p = end;
        return result;
      }
      case TAG.nt: return { t: 'nt', name: names[u16()] };
      case TAG.term: { const n = u8(); const ws = []; for (let i = 0; i < n; i++) ws.push(words[u16()]); return { t: 'term', words: ws }; }
      case TAG.quote: return { t: 'quote' };
      default: throw new Error('bad tag ' + tag + ' at ' + start);
    }
  };
  const rules = new Map();
  for (let i = 0; i < ruleCount; i++) rules.set(names[i], node());
  return rules;
}

function decodeAliases(buf) {
  let p = 0;
  const u8 = () => buf[p++];
  const str = () => { const n = u8(); let s = ''; for (let i = 0; i < n; i++) s += String.fromCharCode(u8()); return s; };
  for (const m of MAGIC_ALIAS) if (u8() !== m) throw new Error('not a dq0 alias table');
  if (u8() !== VERSION_ALIAS) throw new Error('alias table version');
  const count = u8();
  const aliases = new Map();
  const anywhere = new Set();
  for (let i = 0; i < count; i++) { const w = str(); aliases.set(w, str()); if (u8()) anywhere.add(w); }
  return { aliases, anywhere };
}

module.exports = { encodeGrammar, decodeGrammar, encodeAliases, decodeAliases, openNames, grammarNames, OPEN_KIND };

if (require.main === module) {
  const grammar = path.join(GAME, 'commands.ebnf');
  const rules = loadRules(grammar);
  const gbytes = encodeGrammar(rules, openNames(grammar));
  const abytes = encodeAliases();
  const args = process.argv.slice(2);
  console.log('grammar table: ' + gbytes.length + ' bytes for ' + rules.size + ' rules');
  console.log('alias table: ' + abytes.length + ' bytes for ' + DEFAULT_ALIASES.size + ' default aliases');
  if (args[0] && args[0] !== '--check') fs.writeFileSync(args[0], gbytes);
  if (args[1] && args[1] !== '--check') fs.writeFileSync(args[1], abytes);
  if (args.includes('--check')) {
    const decoded = decodeGrammar(gbytes, grammarNames(rules, openNames(grammar)));
    const same = [...rules.keys()].every(k => JSON.stringify(rules.get(k)) === JSON.stringify(decoded.get(k)));
    const al = decodeAliases(abytes);
    const aliasSame = [...DEFAULT_ALIASES].every(([k, v]) => al.aliases.get(k) === v) && al.aliases.size === DEFAULT_ALIASES.size &&
      al.anywhere.size === DEFAULT_ANYWHERE.size && [...DEFAULT_ANYWHERE].every(k => al.anywhere.has(k));
    console.log('decoded rules identical to the text: ' + same + '; aliases identical: ' + aliasSame);
    const { pass, total } = checkCorpus(new Compiler(decoded), path.join(GAME, 'commands.corpus'), console.log);
    console.log('corpus through the grammar table: ' + pass + '/' + total + ' cases pass');
    process.exit(same && aliasSame && pass === total ? 0 : 1);
  }
}
