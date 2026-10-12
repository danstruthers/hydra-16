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

// A file's kind, from its first heading: "Part One. Title", "Chapter 3. Title", "Appendix A. Title", or front matter
function kindOf(h) {
  let m = /^Part\s+(\w+)[.:]\s*(.*)$/.exec(h);
  if (m) return { kind: 'part', num: m[1], title: m[2] };
  m = /^Chapter\s+(\d+)[.:]\s*(.*)$/.exec(h);
  if (m) return { kind: 'chapter', num: m[1], title: m[2] };
  m = /^Appendix\s+([A-Z])[.:]\s*(.*)$/.exec(h);
  if (m) return { kind: 'appendix', num: m[1], title: m[2] };
  return { kind: 'front', num: '', title: h };
}

const MARK = n => '<span class="mk">QQM' + n + 'QQ</span>';

// The whole book's HTML.  pages: page numbers of the contents' entries (by mark number), or null (pass 1)
function book(pages) {
  const ids = new Set();
  const sections = [], toc = [], runCss = [];
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
      toc.push({ kind: 'part', text: 'Part ' + k.num + ': ' + k.title, id: h.id, mark: entry(h) });
      html = '<div class="part-page"><div class="stripes"><i></i><i></i><i></i><i></i><i></i></div><p class="part-num">Part ' + md.esc(k.num) + '</p>' +
        html.replace(/^<h1 id="([^"]+)">(?:<span class="mk">[^<]*<\/span>)?[^<]*<\/h1>/, (m0, id) => '<h1 id="' + id + '">' + (pages ? '' : m0.match(/<span class="mk">[^<]*<\/span>/)?.[0] || '') + md.esc(k.title) + '</h1>') + '</div>';
    } else if (k.kind === 'chapter' || k.kind === 'appendix') {
      const label = k.kind === 'chapter' ? 'Chapter ' + k.num : 'Appendix ' + k.num;
      for (const h of r.headings) {
        if (h.level === 1) toc.push({ kind: k.kind, text: label + '. ' + k.title, num: k.num, title: k.title, id: h.id, mark: entry(h) });
        else if (h.level === 2) toc.push({ kind: 'section', text: h.text, id: h.id, mark: entry(h) });
      }
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
  try {
    execFileSync(browser(), ['--headless=new', '--disable-gpu', '--no-first-run', '--no-default-browser-check', '--user-data-dir=' + prof,
      '--no-pdf-header-footer', '--run-all-compositor-stages-before-draw', '--virtual-time-budget=20000',
      '--print-to-pdf=' + pdfFile, 'file:///' + htmlFile.replace(/\\/g, '/')], { stdio: 'ignore', timeout: 600000 });
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
