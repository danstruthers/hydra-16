// commandtables.js - the grammar as a table, the form the 65C02 compiler reads (host side, in node).
// encodeGrammar turns the rules of commands.ebnf, and the default aliases (commands.aliases), into bytes;
// decodeGrammar reads them back to the same trees that commandcompile.js uses. The corpus must give the same results
// from the bytes as from the text: that is the check.
// Layout: magic "DQ0G", version 2; the names (rules, then open names); the words (every literal word of the grammar);
// the default aliases (word and verb); the rule offsets; the rules. A child of a sequence, alternative, option or repeat
// is preceded by its size in bytes (one byte under 128, two above), so a reader can step over it without walking it.
//   node sim/tools/commandtables.js [out.bin]    write the table and print its size
//   node sim/tools/commandtables.js --check      encode, decode, and run the corpus through the decoded table
'use strict';
const fs = require('fs');
const path = require('path');
const { loadRules, Compiler, buildContext, DEFAULT_WORLD, checkCorpus, DEFAULT_ALIASES, DEFAULT_ANYWHERE } = require('./commandcompile.js');

const GAME = path.join(__dirname, '..', '..', 'games', 'dq0');
const MAGIC = [0x44, 0x51, 0x30, 0x47]; // "DQ0G"
const VERSION = 6;
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

const MAGIC_ALIAS = [0x44, 0x51, 0x30, 0x41]; // "DQ0A"
const VERSION_ALIAS = 1;
const TAG = { seq: 1, alt: 2, opt: 3, rep: 4, nt: 5, term: 6, quote: 7 };

// The open names are declared on the header line of the grammar: "open: ...".
function openNames(file) {
  const m = fs.readFileSync(file, 'utf8').match(/open:\s*([^\n*]*)/);
  return m ? m[1].trim().split(/\s+/).filter(Boolean) : [];
}

function encodeGrammar(rules, open) {
  const names = [...rules.keys(), ...open.filter(n => !rules.has(n))];
  const nameId = new Map(names.map((n, i) => [n, i]));
  const words = [];
  const wordId = new Map();
  const wid = w => {
    if (!wordId.has(w)) { wordId.set(w, words.length); words.push(w); }
    return wordId.get(w);
  };
  const u16 = v => [v & 255, v >> 8];
  // A node's bytes; its children are sized slots.
  const node = n => {
    switch (n.t) {
      case 'seq': return [TAG.seq, n.items.length, ...n.items.flatMap(slot)];
      case 'alt': return [TAG.alt, n.opts.length, ...n.opts.flatMap(slot)];
      case 'opt': return [TAG.opt, ...slot(n.x)];
      case 'rep': return [TAG.rep, ...slot(n.x)];
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
  // A slot: the child's size (one byte under 128; two bytes, the first with its top bit set, above), then the child.
  const slot = child => {
    const body = node(child);
    if (body.length > 32767) throw new Error('node too large');
    const size = body.length < 128 ? [body.length] : [0x80 | (body.length >> 8), body.length & 255];
    return [...size, ...body];
  };
  const ruleBytes = [];
  const offsets = [];
  for (const tree of rules.values()) {
    offsets.push(ruleBytes.length);
    ruleBytes.push(...node(tree));
  }
  const text = s => { if (s.length > 255) throw new Error('word too long: ' + s); return [s.length, ...[...s].map(c => c.charCodeAt(0))]; };
  const head = [...MAGIC, VERSION, ...u16(rules.size), ...u16(names.length)];
  for (const n of names) head.push(...text(n));
  head.push(...u16(words.length));
  for (const w of words) head.push(...packWord(w));
  for (const o of offsets) head.push(...u16(o));
  return Buffer.from([...head, ...ruleBytes]);
}

function encodeAliases(aliases = DEFAULT_ALIASES, anywhere = DEFAULT_ANYWHERE) {
  const text = s => { if (s.length > 255) throw new Error('word too long: ' + s); return [s.length, ...[...s].map(c => c.charCodeAt(0))]; };
  const al = [...aliases.entries()];
  if (al.length > 255) throw new Error('too many aliases');
  const out = [...MAGIC_ALIAS, VERSION_ALIAS, al.length];
  for (const [w, verb] of al) out.push(...text(w), ...text(verb), anywhere.has(w) ? 1 : 0);
  return Buffer.from(out);
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

function decodeGrammar(buf) {
  let p = 0;
  const u8 = () => buf[p++];
  const u16 = () => { const v = buf[p] | (buf[p + 1] << 8); p += 2; return v; };
  const str = () => { const n = u8(); let s = ''; for (let i = 0; i < n; i++) s += String.fromCharCode(u8()); return s; };
  for (const m of MAGIC) if (u8() !== m) throw new Error('not a dq0 grammar table');
  if (u8() !== VERSION) throw new Error('grammar table version');
  const ruleCount = u16(), nameCount = u16();
  const names = [];
  for (let i = 0; i < nameCount; i++) names.push(str());
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
  const offsets = [];
  for (let i = 0; i < ruleCount; i++) offsets.push(u16());
  const rulesStart = p;
  const node = () => {
    const tag = u8();
    switch (tag) {
      case TAG.seq: { const n = u8(); const items = []; for (let i = 0; i < n; i++) items.push(slot()); return { t: 'seq', items }; }
      case TAG.alt: { const n = u8(); const opts = []; for (let i = 0; i < n; i++) opts.push(slot()); return { t: 'alt', opts }; }
      case TAG.opt: return { t: 'opt', x: slot() };
      case TAG.rep: return { t: 'rep', x: slot() };
      case TAG.nt: return { t: 'nt', name: names[u16()] };
      case TAG.term: { const n = u8(); const ws = []; for (let i = 0; i < n; i++) ws.push(words[u16()]); return { t: 'term', words: ws }; }
      case TAG.quote: return { t: 'quote' };
      default: throw new Error('bad tag ' + tag + ' at ' + (p - 1));
    }
  };
  const slot = () => {
    const b = u8();
    const size = (b & 0x80) ? (((b & 0x7F) << 8) | u8()) : b;
    const end = p + size;
    const n = node();
    p = end;
    return n;
  };
  const rules = new Map();
  for (let i = 0; i < ruleCount; i++) {
    p = rulesStart + offsets[i];
    rules.set(names[i], node());
  }
  return rules;
}

module.exports = { encodeGrammar, decodeGrammar, encodeAliases, decodeAliases, openNames };

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
    const decoded = decodeGrammar(gbytes);
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
