#!/usr/bin/env node
// build.js - the Hydra-16 User's and Builder's Manual: its chapters (book.txt lists them, in order) into one book,
// manual/hydra-16-manual.html and manual/hydra-16-manual.pdf.  The PDF is printed by Edge or Chrome (headless): set
// $BROWSER to its path if it isn't found.  Its contents pages get their page numbers from a first printing (read
// back with pdftotext, from poppler; without it, the contents have no page numbers).
//
// Usage: node manual/build.js [--html]       (--html: only the HTML, no PDF)
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFileSync } = require('child_process');
const md = require('./tools/md.js');

const DIR = __dirname;
const OUT_HTML = path.join(DIR, 'hydra-16-manual.html');
const OUT_PDF = path.join(DIR, 'hydra-16-manual.pdf');

function browser() {
  const tries = [process.env.BROWSER,
    'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe', 'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
    'C:/Program Files/Google/Chrome/Application/chrome.exe', '/usr/bin/chromium', '/usr/bin/chromium-browser',
    '/usr/bin/google-chrome', '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'];
  for (const b of tries) if (b && fs.existsSync(b)) return b;
  throw new Error('no browser to print with: set $BROWSER to Edge or Chrome');
}

// ---- The book's parts, from book.txt: a file a line (# comments)
function files() {
  return fs.readFileSync(path.join(DIR, 'book.txt'), 'utf8').split(/\r?\n/).map(l => l.replace(/#.*/, '').trim()).filter(Boolean);
}

// A file's kind, from its first heading: "Part One. Title", "Chapter 3. Title", "Appendix A. Title", "Appendices" (a
// part page with no number), or front matter
function kindOf(h) {
  if (/^Appendices$/.test(h)) return { kind: 'part', num: '', title: h };
  let m = /^Part\s+(\w+)[.:]\s*(.*)$/.exec(h);
  if (m) return { kind: 'part', num: m[1], title: m[2] };
  m = /^Chapter\s+(\d+)[.:]\s*(.*)$/.exec(h);
  if (m) return { kind: 'chapter', num: m[1], title: m[2] };
  m = /^Appendix\s+([A-Z])[.:]\s*(.*)$/.exec(h);
  if (m) return { kind: 'appendix', num: m[1], title: m[2] };
  return { kind: 'front', num: '', title: h };
}

const MARK = n => '<span class="mk">QQM' + n + 'QQ</span>';

/// ---- The index: index.txt's everyday words, by section; each term a chapter or appendix defines in bold (**term**, in its text: not in a table, a heading,
// a code block or a caption; not a run-in heading, which ends with a stop), and each command of Chapter 11, with the
// pages it's on (each place a mark, as the contents' entries have)
const plain = s => s.replace(/<[^>]*>/g, '').replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&quot;/g, '"').replace(/&#39;/g, "'").replace(/&amp;/g, '&');
// (Words in bold that aren't terms: buttons, steps' names, run-in phrases, rc's keywords)
const NOT_TERMS = new Set(['*', 'about fifty commands', 'and', 'both', 'chapter 2', 'copy text', 'definitions and control', 'first',
  'follows instructions', 'four times', 'from assembly language', 'from the languages', 'inspect', 'logic and comparisons',
  'looking inside', 'new', 'one', 'one byte', 'part two', 'paste', 'pause', 'power cycle', 'power cycle with these', 'read it',
  'write a command to it', 'real time', 'remove', 'remove the solder', 'renamed', 'samples', 'save card', 'sound',
  'tack the opposite corner', 'text and numbers', 'three programming languages', 'wash your hands', 'shown', 'size', 'long',
  'holds', 'zooms', 'wide', 'order', 'threw', "kernel's", 'or', 'not', 'for', 'if', 'if not', 'while', 'switch', 'no', 'normal',
  'clip the leads', 'good joint', 'too little solder', 'too much solder', 'you type', 'bold', 'input', 'output', 'memory',
  'processing', 'reset', 'setup…', 'load…', 'code', 'download zip', 'end', 'step', 'next', 'continue', 'until', 'disassemble',
  'write', 'quit', 'name', 'groups', 'number', 'program', 'actions', 'message', 'entries', 'requests', 'storage', 'files',
  'windows', 'shells', 'programs', 'the appendices', 'appendices', 'the tasks', 'the pc', 'the disks', 'the clock', 'the pins',
  'the windows', 'the terminal', 'the i²c bus', 'the pin headers', 'the pin sockets', 'the ram disks', 'the rom disk', 'odd',
  'cold joint', 'bridge', 'lifted pad', 'functions', 'variables', 'teachers', 'students aged 12 and up', 'part one, meet the hydra',
  'chapter 7', 'chapter 8', 'part four', 'fifteen to twenty hours', 'if you burn yourself', 'ram disk forgets', 'the ram disk forgets']);
function indexTerm(t) {
  t = t.replace(/\s+/g, ' ');
  const p = plain(t).trim();
  if (p.length < 2 || p.length > 40 || /[.:!?,;…]$/.test(p) || /,/.test(p) || /^[$0-9(]/.test(p) || p.split(/\s+/).length > 4) return null;
  if (NOT_TERMS.has(p.toLowerCase())) return null;
  t = t.replace(/^(The|the|A|a|An|an) /, '');
  return NOT_TERMS.has(plain(t).toLowerCase()) ? null : t;
}
// The word a reference table's entry is for: "(len l)" is len, "LEFT$(s$, n)" is LEFT$, "dup" is dup; nothing for
// a piece of syntax ("{a b ...}", a closing quote, an example in a later column's style)
function languageWord(code) {
  let w = code.trim();
  if (w.startsWith('(') && w.length > 1) w = w.slice(1);       // (A call: its function)
  w = w.split(/[\s()]/)[0];
  if (!w || w.length > 24 || /^["')}\]]+$/.test(w) || /^[{[]/.test(w) || /\.\.\.$/.test(w) || w === '|') return null;
  return w;
}
// The places in html not to look in: its tables, code blocks, headings, captions and openers
function skipRanges(html) {
  const out = [];
  for (const re of [/<table[\s\S]*?<\/table>/g, /<pre[\s\S]*?<\/pre>/g, /<h[1-6][\s\S]*?<\/h[1-6]>/g, /<figcaption[\s\S]*?<\/figcaption>/g, /<header[\s\S]*?<\/header>/g]) {
    for (const m of html.matchAll(re)) out.push([m.index, m.index + m[0].length]);
  }
  return out;
}
function indexHtml(index, pages) {
  // Plurals into their singular, where both are there ("tasks" into "task")
  for (const [key, e] of [...index]) {
    const one = key.endsWith('s') && index.get(key.slice(0, -1));
    if (one) { one.marks.push(...e.marks); for (const [f, n] of e.forms) one.forms.set(f.replace(/s$/, ''), (one.forms.get(f.replace(/s$/, '')) || 0) + n); index.delete(key); }
  }
  const rows = [...index].map(([key, e]) => {
    let form = [...e.forms].sort((a, b) => b[1] - a[1])[0][0];
    // (A term set in capitals only for starting a run-in label: "Ceramic capacitors" as "ceramic capacitors")
    if (/^(Arithmetic|Card-edge|Ceramic|Complex|Conditional|Desoldering|Electrolytic|Fixed|Indexing|Indirect|Integers|Lead-free|Pin|Rationals|Page|Slot|Stack|Task)\b/.test(form)) form = form[0].toLowerCase() + form.slice(1);
    const sortKey = plain(form).toLowerCase().replace(/^[^a-z0-9]+/, '');
    // (Pass 1: room for the numbers; no numbers known (--html): the terms alone)
    const pgs = !pages ? ['000'] : Object.keys(pages).length ? [...new Set(e.marks.map(m => pages[m]).filter(Boolean))].sort((a, b) => a - b) : [''];
    return { sortKey, form, pgs };
  }).filter(r => r.pgs.length).sort((a, b) => a.sortKey < b.sortKey ? -1 : a.sortKey > b.sortKey ? 1 : 0);
  let html = '', letter = '';
  for (const r of rows) {
    const l = /^[a-z]/.test(r.sortKey) ? r.sortKey[0].toUpperCase() : '#';
    if (l !== letter) { letter = l; html += '<h3 class="ix-letter">' + l + '</h3>\n'; }
    html += '<p class="ix">' + r.form + '<span class="ix-pg">' + r.pgs.join(', ') + '</span></p>\n';
  }
  return html;
}

// The whole book's HTML.  pages: page numbers of the contents' entries (by mark number), or null (pass 1)
function book(pages) {
  const ids = new Set();
  const sections = [], toc = [], runCss = [];
  const index = new Map();                                    // A term's key: { forms (as written: how often), marks }
  const addIndex = (form, n) => {
    const key = plain(form).toLowerCase();
    if (!index.has(key)) index.set(key, { forms: new Map(), marks: [] });
    const e = index.get(key);
    e.forms.set(form, (e.forms.get(form) || 0) + 1);
    e.marks.push(n);
  };
  let mark = 0;
  for (const f of files()) {
    const src = fs.readFileSync(path.join(DIR, f), 'utf8');
    const first = /^#\s+(.*)$/m.exec(src);
    const k = kindOf(first ? first[1].replace(/\s*\{#[^}]*\}\s*$/, '') : f);
    const r = md.render(src, { ids, link: l => /^[a-z0-9-]+\.md(#.*)?$/.test(l) ? l.replace(/^[^#]*\.md/, '') || '#' : l });
    let html = r.html;
    const name = 'p' + sections.length;
    // The contents: a part, a chapter or appendix and its sections (## headings), each with a mark
    const entry = h => { const n = mark++; html = html.replace('<h' + h.level + ' id="' + h.id + '">', '<h' + h.level + ' id="' + h.id + '">' + (pages ? '' : MARK(n))); return n; };
    if (k.kind === 'part') {
      const h = r.headings[0];
      toc.push({ kind: 'part', text: k.num ? 'Part ' + k.num + ': ' + k.title : k.title, id: h.id, mark: entry(h) });
      html = '<div class="part-page"><div class="stripes"><i></i><i></i><i></i><i></i><i></i></div><p class="part-num">' + (k.num ? 'Part ' + md.esc(k.num) : 'Reference') + '</p>' +
        html.replace(/^<h1 id="([^"]+)">(?:<span class="mk">[^<]*<\/span>)?[^<]*<\/h1>/, (m0, id) => '<h1 id="' + id + '">' + (pages ? '' : m0.match(/<span class="mk">[^<]*<\/span>/)?.[0] || '') + md.esc(k.title) + '</h1>') + '</div>';
    } else if (k.kind === 'chapter' || k.kind === 'appendix') {
      const label = k.kind === 'chapter' ? 'Chapter ' + k.num : 'Appendix ' + k.num;
      for (const h of r.headings) {
        if (h.level === 1) toc.push({ kind: k.kind, text: label + '. ' + k.title, num: k.num, title: k.title, id: h.id, mark: entry(h) });
        else if (h.level === 2) {
          const n = entry(h);
          toc.push({ kind: 'section', text: h.text, id: h.id, mark: n });
          const cm = /^([a-z][a-z0-9-]*): /.exec(h.text);       // (the commands' chapter: "ls: list files")
          if (k.kind === 'chapter' && /^Commands\b/.test(k.title) && cm) addIndex('<code>' + cm[1] + '</code> command', n);
        }
      }
      // A language's reference ("HyForth Reference"): each word its tables list, in the index, with its
      // language after it ("dup HyForth"), at the page its row is on
      const lang = k.kind === 'appendix' && /^(\S+) Reference$/.exec(k.title);
      if (lang) {
        html = html.replace(/<tr><td>([\s\S]*?)<\/td>/g, (m0, cell) => {
          const words = [];
          for (const c of cell.matchAll(/<code>([^<]+)<\/code>/g)) {
            const w = languageWord(plain(c[1]));
            if (w && !words.includes(w)) words.push(w);
          }
          if (!words.length) return m0;
          const n = mark++;
          for (const w of words) addIndex('<code>' + md.esc(w) + '</code> <i class="ix-lang">' + lang[1] + '</i>', n);
          return '<tr><td>' + (pages ? '' : MARK(n)) + cell + '</td>';
        });
      }
      // The index's terms: a mark before each
      const skip = skipRanges(html);
      html = html.replace(/<strong>([^<]{2,60}|<code>[^<]{1,40}<\/code>)<\/strong>/g, (m0, t, at) => {
        if (skip.some(([a, b]) => at >= a && at < b)) return m0;
        const term = indexTerm(t);
        if (!term) return m0;
        const n = mark++;
        addIndex(term, n);
        return (pages ? '' : MARK(n)) + m0;
      });
      // The opener: "CHAPTER 3" over the title
      html = html.replace(/^<h1 id="([^"]+)">((?:<span class="mk">[^<]*<\/span>)?)[^<]*<\/h1>/, (m0, id, mk) =>
        '<header class="opener"><div class="stripes"><i></i><i></i><i></i><i></i><i></i></div><p class="label">' + label.toUpperCase().replace(/ (\S+)$/, ' <span class="num">$1</span>') +
        '</p><h1 id="' + id + '">' + mk + md.esc(k.title) + '</h1></header>');
      runCss.push('@page ' + name + ' { @top-left { content: "' + label + ' \\00B7  ' + k.title.replace(/["\\]/g, '') + '"; } }');
    } else {
      for (const h of r.headings) if (h.level === 1) toc.push({ kind: 'front', text: h.text, id: h.id, mark: entry(h) });
    }
    sections.push('<section class="' + k.kind + '" style="page: ' + (k.kind === 'chapter' || k.kind === 'appendix' ? name : k.kind) + '">' + html + '</section>');
  }
  // The index's everyday words (index.txt: "term: 6.6, 9.2"): each at the page its section starts on
  const extra = path.join(DIR, 'index.txt');
  if (fs.existsSync(extra)) {
    for (const line of fs.readFileSync(extra, 'utf8').split(/\r?\n/)) {
      const m = /^([^#:][^:]*):\s*(.+)$/.exec(line.trim());
      if (!m) continue;
      for (const sec of m[2].split(/,\s*/)) {
        const s = sec.trim();                                 // ("3": the chapter's first page)
        const e = toc.find(t => (t.kind === 'section' && t.text.startsWith(s + ' ')) || (t.kind === 'chapter' && t.num === s));
        if (!e) throw new Error('index.txt: no section ' + s + ' (for "' + m[1] + '")');
        addIndex(md.inline(m[1].trim()), e.mark);
      }
    }
  }
  // The index, last
  {
    const n = mark++;
    toc.push({ kind: 'front', text: 'Index', id: 'index', mark: n });
    sections.push('<section class="front index-section" style="page: index"><h1 class="index-title" id="index">' + (pages ? '' : MARK(n)) + 'Index</h1>' +
      '<div class="index">' + indexHtml(index, pages) + '</div></section>');
    runCss.push('@page index { @top-left { content: "Index"; } }');
  }
  // The contents
  const pg = e => pages && pages[e.mark] ? String(pages[e.mark]) : (pages ? '' : '000');
  const contents = '<section class="front contents" style="page: front"><h1 class="contents-title">Contents</h1><div class="toc">' + toc.filter(e => e.kind !== 'front' || e.id !== 'contents').map(e => {
    if (e.kind === 'part') return '<p class="toc-part"><a href="#' + e.id + '">' + md.esc(e.text) + '</a><span class="pg">' + pg(e) + '</span></p>';
    if (e.kind === 'chapter' || e.kind === 'appendix') return '<p class="toc-ch"><a href="#' + e.id + '"><span class="tnum">' + md.esc(e.num) + '</span>' + md.esc(e.title) + '</a><span class="dots"></span><span class="pg">' + pg(e) + '</span></p>';
    if (e.kind === 'section') return '<p class="toc-sec"><a href="#' + e.id + '">' + md.inline(e.text) + '</a><span class="dots"></span><span class="pg">' + pg(e) + '</span></p>';
    return '<p class="toc-front"><a href="#' + e.id + '">' + md.esc(e.text) + '</a><span class="dots"></span><span class="pg">' + pg(e) + '</span></p>';
  }).join('\n') + '</div></section>';
  const title = fs.readFileSync(path.join(DIR, 'title.html'), 'utf8');
  const css = fs.readFileSync(path.join(DIR, 'book.css'), 'utf8') + '\n' + runCss.join('\n');
  // (The contents go after the front matter's first file, the preface)
  const firstChapter = sections.findIndex(s => /^<section class="(part|chapter)"/.test(s));
  const body = [title, ...sections.slice(0, firstChapter), contents, ...sections.slice(firstChapter)].join('\n');
  return '<!doctype html>\n<html lang="en"><head><meta charset="utf-8"><title>The Hydra-16 User\'s and Builder\'s Manual</title>\n<base href="' +
    'file:///' + DIR.replace(/\\/g, '/') + '/">\n<style>\n' + css + '\n</style></head><body>\n' + body + '\n</body></html>\n';
}

function printPdf(htmlFile, pdfFile) {
  const prof = fs.mkdtempSync(path.join(os.tmpdir(), 'hydra-manual-'));
  fs.rmSync(pdfFile, { force: true });                        // (So an old PDF can't pass for a new one)
  try {
    execFileSync(browser(), ['--headless=new', '--disable-gpu', '--no-first-run', '--no-default-browser-check', '--user-data-dir=' + prof,
      '--no-pdf-header-footer', '--run-all-compositor-stages-before-draw', '--virtual-time-budget=20000',
      '--print-to-pdf=' + pdfFile, 'file:///' + htmlFile.replace(/\\/g, '/')], { stdio: 'ignore', timeout: 600000 });
    // The browser's launcher can return before its PDF is written: wait till the file is there and has stopped
    // growing (ten minutes at most)
    const nap = ms => Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, ms);
    let size = -1, same = 0;
    for (let waited = 0; waited < 600000 && same < 3; waited += 1000) {
      nap(1000);
      const now = fs.existsSync(pdfFile) ? fs.statSync(pdfFile).size : -1;
      same = now > 0 && now === size ? same + 1 : 0;
      size = now;
    }
  } finally { try { fs.rmSync(prof, { recursive: true, force: true }); } catch (e) {} }
  if (!fs.existsSync(pdfFile)) throw new Error('the browser made no PDF');
}

// The page each mark is on (pdftotext: pages separated by form feeds)
function markPages(pdfFile) {
  let text;
  try { text = execFileSync('pdftotext', ['-enc', 'UTF-8', pdfFile, '-'], { maxBuffer: 1 << 30 }).toString('utf8'); }
  catch (e) { return null; }
  const pages = {};
  text.split('\f').forEach((p, i) => { for (const m of p.matchAll(/QQM(\d+)QQ/g)) if (!(m[1] in pages)) pages[m[1]] = i + 1; });
  return pages;
}

function main() {
  const htmlOnly = process.argv.includes('--html');
  if (htmlOnly) { fs.writeFileSync(OUT_HTML, book({})); console.log('wrote', path.relative(process.cwd(), OUT_HTML)); return; }
  // Pass 1: marks in, the contents' numbers "000"; read back where each mark landed
  const tmp = path.join(os.tmpdir(), 'hydra-manual-pass1.pdf');
  fs.writeFileSync(OUT_HTML, book(null));
  printPdf(OUT_HTML, tmp);
  const pages = markPages(tmp);
  // Pass 2: the numbers in, the marks out
  fs.writeFileSync(OUT_HTML, book(pages || {}));
  printPdf(OUT_HTML, OUT_PDF);
  const n = (fs.readFileSync(OUT_PDF, 'latin1').match(/\/Type\s*\/Page[^s]/g) || []).length;
  console.log('wrote', path.relative(process.cwd(), OUT_PDF) + ',', n, 'pages' + (pages ? '' : ' (no pdftotext: the contents have no page numbers)'));
}

if (require.main === module) main();
module.exports = { book };
