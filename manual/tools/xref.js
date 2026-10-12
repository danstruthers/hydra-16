#!/usr/bin/env node
// xref.js - the books' cross-references, checked: every "Chapter 7", "section 7.4", "Figure 4-3" and "Appendix K"
// points at a chapter, a section, a figure or an appendix that the manual has.  (The companion and the course
// guide point into the manual too, so their references are checked against it.)
//
// Usage: node manual/tools/xref.js [--list] [FILE ...]
//   FILE     Markdown files to check (the default: every file of every book.txt found in manual/ and its folders)
//   --list   every reference and the title it points at
//
// What a reference is:
//   Chapter 7        chapter 7        Chapters 3, 4 and 5       Chapters 18 to 24
//   section 7.4      Section 7.4      sections 8.3 and 8.4      sections 26.2 to 26.5     section A.2
//   Figure 4-3       Figures 4-3 to 4-5                        Figure E-1
//   Appendix K       Appendices A, B and C
//   (7.4)            (7.4, 12.2)      in a glossary only: its entries end with the sections that explain them
// What defines one: a file's first heading ("# Chapter 7. Title", "# Appendix K. Title"), a "## 7.4 Title" heading,
// and a caption ("<figcaption>Figure 4-3. ...").
'use strict';
const fs = require('fs');
const path = require('path');
const MANUAL = path.join(__dirname, '..');

const SEP = '(?:,\\s+and\\s+|,\\s+|\\s+and\\s+|\\s+to\\s+|\\s+or\\s+)';
const list = item => '(' + item + '(?:' + SEP + item + ')*)';
// Each kind: the words before the numbers, one number's shape, and whether one word can lead a list
const KINDS = [
  { kind: 'chapter', re: new RegExp('\\b([Cc]hapters)\\s+' + list('\\d+(?![\\d.-]|\\s*(?:MHz|K\\b|MB\\b))'), 'g'), item: /\d+/g },
  { kind: 'chapter', re: /\b([Cc]hapter)\s+(\d+)(?![\d-]|\.\d)/g, item: /\d+/g },
  { kind: 'section', re: new RegExp('\\b([Ss]ections)\\s+' + list('(?:\\d+|[A-K])\\.\\d+'), 'g'), item: /(?:\d+|[A-K])\.\d+/g },
  { kind: 'section', re: /\b([Ss]ection)\s+((?:\d+|[A-K])\.\d+)/g, item: /(?:\d+|[A-K])\.\d+/g },
  { kind: 'figure', re: new RegExp('\\b(Figures)\\s+' + list('(?:\\d+|[A-K])-\\d+'), 'g'), item: /(?:\d+|[A-K])-\d+/g },
  { kind: 'figure', re: /\b(Figure)\s+((?:\d+|[A-K])-\d+)/g, item: /(?:\d+|[A-K])-\d+/g },
  { kind: 'appendix', re: new RegExp('\\b(Appendices)\\s+' + list('[A-K]\\b'), 'g'), item: /\b[A-K]\b/g },
  { kind: 'appendix', re: /\b(Appendix)\s+([A-K])\b/g, item: /[A-K]/g },
];
const GLOSSARY = /\((\d+\.\d+(?:,\s+\d+\.\d+)*)\)/g;

// The references in a text: { kind, num, at } in order of position (at: where the number starts)
function refs(text, glossary) {
  const out = [];
  for (const k of KINDS) {
    k.re.lastIndex = 0;
    let m;
    while ((m = k.re.exec(text))) {
      const start = m.index + m[0].length - m[2].length;
      k.item.lastIndex = 0;
      let im;
      while ((im = k.item.exec(m[2]))) out.push({ kind: k.kind, num: im[0], at: start + im.index });
    }
  }
  if (glossary) {
    GLOSSARY.lastIndex = 0;
    let m;
    while ((m = GLOSSARY.exec(text))) {
      const re = /\d+\.\d+/g;
      let im;
      while ((im = re.exec(m[1]))) out.push({ kind: 'section', num: im[0], at: m.index + 1 + im.index });
    }
  }
  out.sort((a, b) => a.at - b.at);
  return out.filter((r, i) => i === 0 || r.at !== out[i - 1].at);
}

// What a text defines: { kind, num, title, at } (at: where the number starts)
function defs(text) {
  const out = [];
  let m;
  const head = /^# (Chapter|Appendix) (\d+|[A-K])\. (.*)$/m.exec(text);
  if (head) out.push({ kind: head[1] === 'Chapter' ? 'chapter' : 'appendix', num: head[2], title: head[3].trim(), at: head.index + 2 + head[1].length + 1 });
  const sec = /^## ((?:\d+|[A-K])\.\d+) (.*)$/gm;
  while ((m = sec.exec(text))) out.push({ kind: 'section', num: m[1], title: m[2].trim(), at: m.index + 3 });
  const fig = /<figcaption>Figure ((?:\d+|[A-K])-\d+)\. ([^<]*)/g;
  while ((m = fig.exec(text))) out.push({ kind: 'figure', num: m[1], title: m[2].trim().slice(0, 60), at: m.index + '<figcaption>Figure '.length });
  return out;
}

// A book's files, from its book.txt
function bookFiles(dir) {
  const list = path.join(dir, 'book.txt');
  if (!fs.existsSync(list)) return [];
  return fs.readFileSync(list, 'utf8').split(/\r?\n/).map(s => s.trim()).filter(s => s && !s.startsWith('#')).map(s => path.join(dir, s));
}
function allBooks() {
  const dirs = [MANUAL].concat(fs.readdirSync(MANUAL, { withFileTypes: true }).filter(d => d.isDirectory()).map(d => path.join(MANUAL, d.name)));
  return dirs.filter(d => fs.existsSync(path.join(d, 'book.txt')));
}

// The manual's definitions: a map "kind num" -> { title, file }
function definitions(files) {
  const map = new Map(), dups = [];
  for (const f of files) {
    for (const d of defs(fs.readFileSync(f, 'utf8'))) {
      const key = d.kind + ' ' + d.num;
      if (map.has(key)) dups.push(key + ' (' + path.basename(f) + ' and ' + path.basename(map.get(key).file) + ')');
      map.set(key, { title: d.title, file: f });
    }
  }
  return { map, dups };
}

const lineOf = (text, at) => text.slice(0, at).split('\n').length;
const isGlossary = f => /glossary/.test(path.basename(f));

function main() {
  const args = process.argv.slice(2);
  const listAll = args.includes('--list');
  const named = args.filter(a => !a.startsWith('--')).map(a => path.resolve(a));
  const { map, dups } = definitions(bookFiles(MANUAL));
  const files = named.length ? named : [].concat(...allBooks().map(bookFiles));
  let bad = 0, count = 0;
  for (const d of dups) { console.log('defined twice: ' + d); bad++; }
  // A chapter's sections and figures in order: 7.1, 7.2 ... with none missing
  for (const kind of ['section', 'figure']) {
    const by = new Map();
    for (const key of map.keys()) {
      if (!key.startsWith(kind + ' ')) continue;
      const [c, n] = key.slice(kind.length + 1).split(/[.-]/);
      if (!by.has(c)) by.set(c, []);
      by.get(c).push(+n);
    }
    for (const [c, ns] of by) ns.forEach((n, i) => { if (n !== i + 1) { console.log(kind + ' numbers of ' + c + ' out of order: ' + ns.join(' ')); bad++; ns.length = 0; } });
  }
  for (const f of files) {
    const text = fs.readFileSync(f, 'utf8');
    const own = defs(text).find(d => d.kind === 'chapter' || d.kind === 'appendix');
    for (const r of refs(text, isGlossary(f))) {
      count++;
      const hit = map.get(r.kind + ' ' + r.num);
      const where = path.relative(MANUAL, f).replace(/\\/g, '/') + ':' + lineOf(text, r.at);
      if (!hit) { console.log(where + ': no ' + r.kind + ' ' + r.num); bad++; }
      else if (listAll) console.log(where + ': ' + r.kind + ' ' + r.num + ' = ' + hit.title);
    }
    // A chapter's own sections and figures carry its number
    if (own) for (const d of defs(text)) {
      if (d.kind !== 'chapter' && d.kind !== 'appendix' && d.num.split(/[.-]/)[0] !== own.num) { console.log(path.basename(f) + ': ' + d.kind + ' ' + d.num + ' in ' + own.kind + ' ' + own.num); bad++; }
    }
  }
  console.log(count + ' references in ' + files.length + ' files, ' + (bad ? bad + ' wrong' : 'all good'));
  process.exit(bad ? 1 : 0);
}

module.exports = { refs, defs, bookFiles, allBooks, definitions, isGlossary, MANUAL };
if (require.main === module) main();
