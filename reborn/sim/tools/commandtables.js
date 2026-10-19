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
//   node sim/tools/commandtables.js [grammar.bin] [aliases.bin] [index.bin]   write the tables and print their sizes
//   node sim/tools/commandtables.js --check                       encode, decode, and run the corpus through the tables
'use strict';
const fs = require('fs');
const path = require('path');
const { loadRules, Compiler, buildContext, DEFAULT_WORLD, checkCorpus, DEFAULT_ALIASES, DEFAULT_ANYWHERE, LEXICON } = require('./commandcompile.js');

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

// The kind of each open name, one byte: the matcher's K_ codes in cmdmatch.s (quoted-text is a rule, so not listed).
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

// ---- The index table: what the 65C02 matcher uses to find words, and to skip what can't match -------------------------
// Index layout (version 2): magic "DQ0I", the version; the rule count, the word count (the grammar table's) and the
// dictionary's entry count (two bytes each); the rules "line", "sequence" and "quoted-text" (a byte each: what the
// matcher treats by name); the hash's permutation (256 bytes: Pearson's, the matcher's and wordHash's); the dictionary:
// for each of the 256 hashes, where its words start (257 offsets from the first word, two bytes each), then the words in
// hash order, each its length, its letters and its entry (two bytes: entries 0 to n - 1 are the grammar's words, by
// number; then the lexicon's other words, and the default aliases' first words); each entry's flags (a byte: LX_*);
// each rule's first words as a 64-bit Bloom filter (eight rows of a byte a rule: word w sets bit w & 63) and its first
// open-name kinds (two rows of a byte a rule: the K_ bits; in the second row's top bit, whether the rule can match
// nothing); then the large alternations' indexes: the count, and for each its offset in the grammar table, its options'
// offsets, its first words (the count; the words, sorted, two bytes each; then each one's option, a byte) and its options
// that can begin with an open name or match nothing (the count, then option and kinds, bit 15 for nothing).
const INDEX_MAGIC = [0x44, 0x51, 0x30, 0x49]; // "DQ0I"
const INDEX_VERSION = 2;
const ALT_INDEX_MIN = 8;          // an alternation of this many options or more is indexed by its first words
const K_QUOTED = 10;              // the matcher's kind for quoted text (cmdmatch.s's K_QUOTED)
const LX = { adj: 1, task: 2, ordinal: 4, number: 8, notspell: 16, notobject: 32 };   // cmdmatch.s's LX_*

// The hash: Pearson's, a byte from a byte at a time through a permutation of 0..255 (fixed: from a seeded shuffle).
const PEARSON = (() => {
  const t = [...Array(256).keys()];
  let x = 20261019;
  for (let i = 255; i > 0; i--) {
    x = (Math.imul(x, 1103515245) + 12345) >>> 0;
    const j = x % (i + 1);
    [t[i], t[j]] = [t[j], t[i]];
  }
  return t;
})();
function wordHash(w) {
  let h = 0;
  for (const c of w) h = PEARSON[h ^ c.charCodeAt(0)];
  return PEARSON[h ^ w.length];
}

// The grammar table's words, in their order (their numbers)
function tableWords(buf) {
  let p = 5;
  const u16 = () => { const v = buf[p] | (buf[p + 1] << 8); p += 2; return v; };
  u16(); u16();
  const n = u16(), words = [];
  for (let i = 0; i < n; i++) {
    const len = buf[p++];
    let w = '';
    for (let c = 0; c < len; c++) {
      const bit = 5 * c, b = bit >> 3, sh = bit & 7;
      w += PACK[((buf[p + b] | (buf[p + b + 1] << 8)) >> sh) & 31];
    }
    p += Math.ceil(5 * len / 8);
    words.push(w);
  }
  return words;
}

// Where each node of the rules is in the grammar table: walked as the matcher walks it
function nodeOffsets(buf, rules) {
  let p = 5;
  const rd = q => buf[q] | (buf[q + 1] << 8);
  const nrules = rd(p), nnames = rd(p + 2), nwords = rd(p + 4);
  p += 6;
  for (let i = 0; i < nwords; i++) p += 1 + Math.ceil(5 * buf[p] / 8);
  p += nnames - nrules;
  const head = q => (buf[q + 1] & 0x80) ? q + 3 : q + 2;
  const next = q => {
    switch (buf[q]) {
      case TAG.seq: case TAG.alt: case TAG.opt: case TAG.rep:
        return q + ((buf[q + 1] & 0x80) ? (((buf[q + 1] & 0x7F) << 8) | buf[q + 2]) : buf[q + 1]);
      case TAG.nt: return q + 3;
      case TAG.term: return q + 2 + 2 * buf[q + 1];
      default: return q + 1;
    }
  };
  const at = new Map();
  const visit = (n, q) => {
    if (buf[q] !== TAG[n.t]) throw new Error('the grammar table and the rules differ at ' + q);
    at.set(n, q);
    if (n.t === 'seq' || n.t === 'alt') {
      let c = head(q) + 1;
      for (const k of n.t === 'seq' ? n.items : n.opts) { visit(k, c); c = next(c); }
    } else if (n.t === 'opt' || n.t === 'rep') visit(n.x, head(q));
  };
  for (const tree of rules.values()) { visit(tree, p); p = next(p); }
  if (p !== buf.length) throw new Error('the grammar table has bytes after its rules');
  return at;
}

// The first words of the rules (and of any node): the words a match can begin with, the open-name kinds it can begin
// with (a bit each, K_QUOTED for quoted text), and whether it can match nothing. The rules' are worked out to a fixed
// point, as a rule may begin with one defined after it. As the matcher has them: quoted-text is a kind, not a rule.
function firstSets(rules) {
  const R = new Map([...rules.keys()].map(k => [k, { words: new Set(), kinds: 0, nullable: false }]));
  const of = n => {
    switch (n.t) {
      case 'term': return { words: new Set([n.words[0]]), kinds: 0, nullable: false };
      case 'quote': return { words: new Set(), kinds: 1 << K_QUOTED, nullable: false };
      case 'nt':
        if (n.name === 'quoted-text') return { words: new Set(), kinds: 1 << K_QUOTED, nullable: false };
        if (!rules.has(n.name)) {
          if (!(n.name in OPEN_KIND)) throw new Error('no matcher kind for the open name ' + n.name);
          return { words: new Set(), kinds: 1 << OPEN_KIND[n.name], nullable: false };
        }
        return R.get(n.name);
      case 'seq': {
        const f = { words: new Set(), kinds: 0, nullable: true };
        for (const it of n.items) {
          const g = of(it);
          for (const w of g.words) f.words.add(w);
          f.kinds |= g.kinds;
          if (!g.nullable) { f.nullable = false; break; }
        }
        return f;
      }
      case 'alt': {
        const f = { words: new Set(), kinds: 0, nullable: false };
        for (const o of n.opts) {
          const g = of(o);
          for (const w of g.words) f.words.add(w);
          f.kinds |= g.kinds;
          f.nullable = f.nullable || g.nullable;
        }
        return f;
      }
      case 'opt': case 'rep': { const g = of(n.x); return { words: new Set(g.words), kinds: g.kinds, nullable: true }; }
      default: throw new Error('bad node ' + n.t);
    }
  };
  for (let changed = true; changed;) {
    changed = false;
    for (const [k, tree] of rules) {
      const f = of(tree), cur = R.get(k);
      if (f.words.size !== cur.words.size || f.kinds !== cur.kinds || f.nullable !== cur.nullable) { R.set(k, f); changed = true; }
    }
  }
  return { R, of };
}

// The lexicon: the words the open names look for (commandcompile.js's lists), each with its LX_ flags
function lexiconWords() {
  const m = new Map();
  const add = (words, flag) => { for (const w of words) m.set(w, (m.get(w) || 0) | flag); };
  add(LEXICON.adj, LX.adj);
  add(LEXICON.tasks, LX.task);
  add(LEXICON.ordinals, LX.ordinal);
  add(LEXICON.numbers, LX.number);
  add(LEXICON.spellStop, LX.notspell);
  add(LEXICON.invocations, LX.notspell);
  add(LEXICON.notObject, LX.notobject);
  return m;
}

// A rule that can call itself at its own start, through what can match nothing, would loop the matcher (it keeps no
// "being matched" mark: its memo is a cache). The grammar has none; this says so, or throws.
function checkLeftRecursion(rules, of) {
  const starts = n => {
    switch (n.t) {
      case 'nt': return rules.has(n.name) && n.name !== 'quoted-text' ? [n.name] : [];
      case 'seq': { const out = []; for (const it of n.items) { out.push(...starts(it)); if (!of(it).nullable) break; } return out; }
      case 'alt': return n.opts.flatMap(starts);
      case 'opt': case 'rep': return starts(n.x);
      default: return [];
    }
  };
  const edges = new Map([...rules].map(([k, t]) => [k, starts(t)]));
  for (const k of rules.keys()) {
    const seen = new Set(), stack = [...edges.get(k)];
    while (stack.length) {
      const x = stack.pop();
      if (x === k) throw new Error('<' + k + '> is left-recursive: the 65C02 matcher would not finish');
      if (!seen.has(x)) { seen.add(x); stack.push(...edges.get(x)); }
    }
  }
}

function encodeIndex(rules, gbuf, aliases = DEFAULT_ALIASES) {
  const u16 = v => [v & 255, v >> 8];
  const words = tableWords(gbuf), wid = new Map(words.map((w, i) => [w, i]));
  const at = nodeOffsets(gbuf, rules);
  const { R, of } = firstSets(rules);
  checkLeftRecursion(rules, of);
  const names = [...rules.keys()], n = names.length;
  const id = k => { const i = names.indexOf(k); if (i < 0 || i > 255) throw new Error('no rule <' + k + '> for the matcher'); return i; };
  // The dictionary: the grammar's words (their numbers), then the lexicon's others, then the aliases' first words
  const entries = [...words], flags = words.map(() => 0), entryOf = new Map(words.map((w, i) => [w, i]));
  const enter = w => { if (!entryOf.has(w)) { entryOf.set(w, entries.length); entries.push(w); flags.push(0); } return entryOf.get(w); };
  for (const [w, f] of lexiconWords()) flags[enter(w)] |= f;
  for (const key of aliases.keys()) enter(key.split(' ')[0]);
  for (const w of entries) if (!w.length || w.length > 255 || /[^a-z0-9!='-]/.test(w)) throw new Error('a word the matcher cannot hold: ' + w);
  const buckets = Array.from({ length: 256 }, () => []);
  entries.forEach((w, e) => buckets[wordHash(w)].push(e));
  const recs = [], starts = [];
  for (const b of buckets) {
    starts.push(recs.length);
    for (const e of b) { const w = entries[e]; recs.push(w.length, ...[...w].map(c => c.charCodeAt(0)), ...u16(e)); }
  }
  starts.push(recs.length);
  if (recs.length > 65535) throw new Error('the dictionary is too large');
  // The rules' first words and kinds
  const bloom = Array.from({ length: 8 }, () => new Array(n).fill(0));
  const klo = [], khi = [];
  names.forEach((k, r) => {
    const f = R.get(k);
    for (const w of f.words) { const b = wid.get(w) & 63; bloom[b >> 3][r] |= 1 << (b & 7); }
    klo.push(f.kinds & 255);
    khi.push(((f.kinds >> 8) & 0x7F) | (f.nullable ? 0x80 : 0));
  });
  const out = [...INDEX_MAGIC, INDEX_VERSION, ...u16(n), ...u16(words.length), ...u16(entries.length),
    id('line'), id('sequence'), id('quoted-text'), ...PEARSON, ...starts.flatMap(u16), ...recs, ...flags, ...bloom.flat(), ...klo, ...khi];
  // The large alternations
  const alts = [];
  const walk = t => {
    if (t.t === 'alt') { if (t.opts.length >= ALT_INDEX_MIN) alts.push(t); t.opts.forEach(walk); }
    else if (t.t === 'seq') t.items.forEach(walk);
    else if (t.t === 'opt' || t.t === 'rep') walk(t.x);
  };
  for (const t of rules.values()) walk(t);
  if (alts.length > 255) throw new Error('too many indexed alternations');
  out.push(alts.length);
  for (const a of alts) {
    if (a.opts.length > 255) throw new Error('an alternation of more than 255 options');
    const ents = [], wild = [];
    a.opts.forEach((o, i) => {
      const f = of(o);
      if (f.nullable || f.kinds) wild.push([i, f.kinds | (f.nullable ? 0x8000 : 0)]);
      if (!f.nullable) for (const w of f.words) ents.push([wid.get(w), i]);
    });
    ents.sort((x, y) => x[0] - y[0] || x[1] - y[1]);
    if (wild.length > 255 || ents.length > 32767) throw new Error('an alternation too large to index');
    out.push(...u16(at.get(a)), a.opts.length, ...a.opts.flatMap(o => u16(at.get(o))), ...u16(ents.length),
      ...ents.flatMap(([w]) => u16(w)), ...ents.map(([, i]) => i), wild.length, ...wild.flatMap(([i, k]) => [i, ...u16(k)]));
  }
  return Buffer.from(out);
}

// The index's dictionary read back: each word in the bucket of its own hash, the grammar's words first with their
// own numbers, every entry once.  OUT: the entries, or it throws.
function checkIndex(ibuf, gbuf) {
  const rd = q => ibuf[q] | (ibuf[q + 1] << 8);
  const words = tableWords(gbuf), n = rd(9), seen = new Array(n).fill(null);
  for (let b = 0; b < 256; b++) {
    for (let p = 784 + rd(270 + 2 * b), end = 784 + rd(272 + 2 * b); p < end;) {
      const len = ibuf[p];
      const w = String.fromCharCode(...ibuf.subarray(p + 1, p + 1 + len)), e = rd(p + 1 + len);
      if (wordHash(w) !== b) throw new Error('the index: ' + w + ' in the wrong bucket');
      if (e >= n || seen[e] !== null) throw new Error('the index: entry ' + e + ' twice or out of range');
      if (e < words.length && words[e] !== w) throw new Error('the index: entry ' + e + ' is ' + w + ', the grammar has ' + words[e]);
      seen[e] = w;
      p += len + 3;
    }
  }
  if (seen.includes(null)) throw new Error('the index: an entry in no bucket');
  return n;
}

module.exports = { encodeGrammar, decodeGrammar, encodeAliases, decodeAliases, openNames, grammarNames, OPEN_KIND, encodeIndex, checkIndex, firstSets, wordHash, PEARSON };

if (require.main === module) {
  const grammar = path.join(GAME, 'commands.ebnf');
  const rules = loadRules(grammar);
  const gbytes = encodeGrammar(rules, openNames(grammar));
  const abytes = encodeAliases();
  const ibytes = encodeIndex(rules, gbytes);
  const args = process.argv.slice(2);
  console.log('grammar table: ' + gbytes.length + ' bytes for ' + rules.size + ' rules');
  console.log('alias table: ' + abytes.length + ' bytes for ' + DEFAULT_ALIASES.size + ' default aliases');
  console.log('index table: ' + ibytes.length + ' bytes');
  if (args[0] && args[0] !== '--check') fs.writeFileSync(args[0], gbytes);
  if (args[1] && args[1] !== '--check') fs.writeFileSync(args[1], abytes);
  if (args[2] && args[2] !== '--check') fs.writeFileSync(args[2], ibytes);
  if (args.includes('--check')) {
    const decoded = decodeGrammar(gbytes, grammarNames(rules, openNames(grammar)));
    const same = [...rules.keys()].every(k => JSON.stringify(rules.get(k)) === JSON.stringify(decoded.get(k)));
    const al = decodeAliases(abytes);
    const aliasSame = [...DEFAULT_ALIASES].every(([k, v]) => al.aliases.get(k) === v) && al.aliases.size === DEFAULT_ALIASES.size &&
      al.anywhere.size === DEFAULT_ANYWHERE.size && [...DEFAULT_ANYWHERE].every(k => al.anywhere.has(k));
    console.log('decoded rules identical to the text: ' + same + '; aliases identical: ' + aliasSame);
    console.log('index: ' + checkIndex(ibytes, gbytes) + ' dictionary entries, each found by its hash');
    const { pass, total } = checkCorpus(new Compiler(decoded), path.join(GAME, 'commands.corpus'), console.log);
    console.log('corpus through the grammar table: ' + pass + '/' + total + ' cases pass');
    process.exit(same && aliasSame && pass === total ? 0 : 1);
  }
}
