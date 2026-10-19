// commandtables.js - the grammar as a table, the form the 65C02 compiler reads (host side, in node).
// encodeGrammar turns the rules of commands.ebnf into bytes; decodeGrammar reads them back to the same trees that
// commandcompile.js uses. The corpus must give the same results from the bytes as from the text: that is the check.
//   node sim/tools/commandtables.js [out.bin]    write the table and print its size
//   node sim/tools/commandtables.js --check      encode, decode, and run the corpus through the decoded table
'use strict';
const fs = require('fs');
const path = require('path');
const { loadRules, Compiler, buildContext, DEFAULT_WORLD, checkCorpus } = require('./commandcompile.js');

const GAME = path.join(__dirname, '..', '..', 'games', 'dq0');
const MAGIC = [0x44, 0x51, 0x30, 0x47]; // "DQ0G"
const VERSION = 1;
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
  const out = [];
  const u8 = v => { if (v < 0 || v > 255) throw new Error('byte out of range: ' + v); out.push(v); };
  const u16 = v => { u8(v & 255); u8(v >> 8); };
  const node = n => {
    switch (n.t) {
      case 'seq': u8(TAG.seq); u8(n.items.length); n.items.forEach(node); break;
      case 'alt': u8(TAG.alt); u8(n.opts.length); n.opts.forEach(node); break;
      case 'opt': u8(TAG.opt); node(n.x); break;
      case 'rep': u8(TAG.rep); node(n.x); break;
      case 'nt': {
        if (!nameId.has(n.name)) throw new Error('undefined name <' + n.name + '>');
        u8(TAG.nt); u16(nameId.get(n.name)); break;
      }
      case 'term': u8(TAG.term); u8(n.words.length); n.words.forEach(w => u16(wid(w))); break;
      case 'quote': u8(TAG.quote); break;
      default: throw new Error('bad node ' + n.t);
    }
  };
  // Rules are written in order, each with its offset, so a C reader can jump to any rule.
  const offsets = [];
  for (const [name, tree] of rules) {
    offsets.push(out.length);
    node(tree);
  }
  const rulesBytes = out.splice(0);
  // The name table: the rules' names, then the open names. The word table: every literal word.
  const head = [];
  const h8 = v => head.push(v & 255);
  const h16 = v => { h8(v); h8(v >> 8); };
  MAGIC.forEach(h8); h8(VERSION);
  h16(rules.size); h16(names.length);
  for (const n of names) { h8(n.length); for (const c of n) h8(c.charCodeAt(0)); }
  h16(words.length);
  for (const w of words) { h8(w.length); for (const c of w) h8(c.charCodeAt(0)); }
  offsets.forEach(o => h16(o));
  return Buffer.from(head.concat(rulesBytes));
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
  for (let i = 0; i < wordCount; i++) words.push(str());
  const offsets = [];
  for (let i = 0; i < ruleCount; i++) offsets.push(u16());
  const rulesStart = p;
  const node = () => {
    const tag = u8();
    switch (tag) {
      case TAG.seq: { const n = u8(); const items = []; for (let i = 0; i < n; i++) items.push(node()); return { t: 'seq', items }; }
      case TAG.alt: { const n = u8(); const opts = []; for (let i = 0; i < n; i++) opts.push(node()); return { t: 'alt', opts }; }
      case TAG.opt: return { t: 'opt', x: node() };
      case TAG.rep: return { t: 'rep', x: node() };
      case TAG.nt: return { t: 'nt', name: names[u16()] };
      case TAG.term: { const n = u8(); const ws = []; for (let i = 0; i < n; i++) ws.push(words[u16()]); return { t: 'term', words: ws }; }
      case TAG.quote: return { t: 'quote' };
      default: throw new Error('bad tag ' + tag + ' at ' + (p - 1));
    }
  };
  const rules = new Map();
  for (let i = 0; i < ruleCount; i++) {
    p = rulesStart + offsets[i];
    rules.set(names[i], node());
  }
  return rules;
}

module.exports = { encodeGrammar, decodeGrammar, openNames };

if (require.main === module) {
  const grammar = path.join(GAME, 'commands.ebnf');
  const rules = loadRules(grammar);
  const bytes = encodeGrammar(rules, openNames(grammar));
  const args = process.argv.slice(2);
  console.log(`grammar table: ${bytes.length} bytes for ${rules.size} rules`);
  if (args[0] && args[0] !== '--check') fs.writeFileSync(args[0], bytes);
  if (args.includes('--check')) {
    const decoded = decodeGrammar(bytes);
    const same = [...rules.keys()].every(k => JSON.stringify(rules.get(k)) === JSON.stringify(decoded.get(k)));
    console.log(`decoded rules identical to the text: ${same}`);
    const { pass, total } = checkCorpus(new Compiler(decoded), path.join(GAME, 'commands.corpus'), console.log);
    console.log(`corpus through the table: ${pass}/${total} cases pass`);
    process.exit(same && pass === total ? 0 : 1);
  }
}
