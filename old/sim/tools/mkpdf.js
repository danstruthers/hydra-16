#!/usr/bin/env node
// ****************************************************************************
// mkpdf.js - all of the documentation as one PDF: docs/hydra-16.pdf.  Every Markdown file in docs/, and the
// top README, are turned into one HTML book (a cover with the contents, then each document on pages of its
// own), and a headless Microsoft Edge or Google Chrome prints it.  Links between the documents become links
// inside the PDF, to the document or its section; links to other files in the repository go to GitHub.  The
// PDF's bookmarks list the documents and their sections.
//
// Usage: node sim/tools/mkpdf.js [OUT.pdf] [--html OUT.html] [--browser PATH]
//          OUT.pdf defaults to docs/hydra-16.pdf; --html also keeps the HTML book; --browser names the
//          browser when it isn't where Edge or Chrome usually is (or set HYDRA_BROWSER).
//
// No packages needed: the Markdown is the subset the docs use (GitHub's: headings, paragraphs, lists,
// tables, code blocks, quotes, inline code, bold, italic, links), converted here.
// ****************************************************************************
'use strict';
const fs = require('fs');
const path = require('path');
const os = require('os');
const { execFileSync } = require('child_process');

const ROOT = path.resolve(__dirname, '../..');
const REPO_URL = 'https://github.com/danstruthers/hydra-16/blob/main/';

// The documents, in the book's order: the master document, the way in, the hardware, using it,
// programming it, the tools, the plans, the indexes.  Any other .md in docs/ goes at the end.
const ORDER = [
  'docs/hydra-16.md', 'docs/tutorial.md', 'docs/getting-started.md', 'docs/hardware.md',
  'docs/using/hyforth.md', 'docs/using/wozmon.md',
  'docs/programming/README.md', 'docs/programming/rom-layout.md', 'docs/programming/tasks.md',
  'docs/programming/interrupts.md', 'docs/programming/memory.md', 'docs/programming/io.md',
  'docs/programming/servers.md', 'docs/programming/programs.md', 'docs/programming/c.md',
  'docs/tools/emulator.md',
  'docs/plans/NEXT_STEPS.md', 'docs/plans/IDEAS.md', 'docs/plans/NAMESPACES.md', 'docs/plans/PROC.md', 'docs/plans/PC.md',
  'docs/plans/DISKS.md', 'docs/plans/VIDEO.md', 'docs/plans/SOUND.md', 'docs/plans/HYDRAFS.md',
  'docs/plans/IO_PLAN.md', 'docs/plans/MMU_PLAN.md', 'docs/plans/SHELL.md', 'docs/plans/REORG_PLAN.md',
  'docs/plans/CODE_REVIEW.md',
  'docs/README.md', 'README.md',
];

const BROWSERS = [
  'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
  'C:/Program Files/Google/Chrome/Application/chrome.exe',
  'C:/Program Files (x86)/Google/Chrome/Application/chrome.exe',
  '/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge',
  '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome',
  '/usr/bin/microsoft-edge', '/usr/bin/google-chrome', '/usr/bin/chromium', '/usr/bin/chromium-browser',
];

function allDocs() {
  const found = [];
  (function walk(dir) {
    for (const f of fs.readdirSync(path.join(ROOT, dir)).sort()) {
      const p = dir + '/' + f;
      if (fs.statSync(path.join(ROOT, p)).isDirectory()) walk(p);
      else if (f.endsWith('.md')) found.push(p);
    }
  })('docs');
  return ORDER.filter(f => fs.existsSync(path.join(ROOT, f))).concat(found.filter(f => !ORDER.includes(f)));
}

// ---------------------------------------------------------------------------
// Names: a document's id, and a heading's anchor (GitHub's: lower case, punctuation dropped, spaces to -)

const docId = f => f.replace(/^docs\//, '').replace(/\.md$/, '').replace(/[^A-Za-z0-9]+/g, '-').toLowerCase();

function slug(text) {
  return text.replace(/\[([^\]]*)\]\([^)]*\)/g, '$1').replace(/[*`]/g, '').trim().toLowerCase()
    .replace(/[^\p{L}\p{N} _-]/gu, '').replace(/ /g, '-');
}

const esc = s => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// A heading as plain text (for the contents and the PDF's outline)
const plain = s => s.replace(/\[([^\]]*)\]\([^)]*\)/g, '$1').replace(/\\(.)/g, '$1').replace(/[*`]/g, '').trim();

// ---------------------------------------------------------------------------
// Inline Markdown

function inline(s, ctx, held = []) {                                   // (held: shared with a link's text)
  const hold = html => '\u0000' + (held.push(html) - 1) + '\u0001';
  s = s.replace(/(`+)([\s\S]*?[^`])\1(?!`)/g, (m, t, code) => hold('<code>' + esc(code.trim() || code).replace(/\/(?=.)/g, '/<wbr>') + '</code>'));
  s = s.replace(/\\([\\`*_{}\[\]()#+\-.!|<>~])/g, (m, c) => hold(esc(c)));
  s = s.replace(/!?\[((?:[^\[\]]|\[[^\]]*\])*)\]\(([^)\s]+)\)/g,
    (m, text, url) => hold('<a href="' + esc(ctx.link(url)) + '">' + inline(text, ctx, held) + '</a>'));
  s = s.replace(/\bhttps?:\/\/[^\s<>()]*[^\s<>().,;:]/g, url => hold('<a href="' + esc(url) + '">' + esc(url) + '</a>'));
  s = esc(s);
  s = s.replace(/\*\*(?=\S)([\s\S]*?\S)\*\*/g, '<strong>$1</strong>');
  s = s.replace(/(^|[^\w*])\*(?=[^\s*])([\s\S]*?[^\s*])\*(?![\w*])/g, '$1<em>$2</em>');
  s = s.replace(/(^|[^\w])_(?=\S)([\s\S]*?\S)_(?!\w)/g, '$1<em>$2</em>');
  for (let i = 0; i < 4 && s.includes('\u0000'); i++)
    s = s.replace(/\u0000(\d+)\u0001/g, (m, n) => held[n]);
  return s;
}

// ---------------------------------------------------------------------------
// Blocks

const RE_FENCE = /^(\s*)(```+|~~~+)(.*)$/;
const RE_HEADING = /^ {0,3}(#{1,6})\s+(.*?)\s*#*\s*$/;
const RE_HR = /^ {0,3}([-*_])(\s*\1){2,}\s*$/;
const RE_ITEM = /^( *)([*+-]|\d+[.)])( +|$)(.*)$/;
const RE_QUOTE = /^ {0,3}> ?/;
const RE_TABLE_SEP = /^\s*\|?\s*:?-+:?\s*(\|\s*:?-*:?\s*)*\|?\s*$/;

const indentOf = line => line.match(/^ */)[0].length;
const blank = line => /^\s*$/.test(line);

function startsBlock(lines, i) {
  const l = lines[i];
  return RE_FENCE.test(l) || RE_HEADING.test(l) || RE_HR.test(l) || RE_ITEM.test(l) || RE_QUOTE.test(l) ||
    (l.includes('|') && i + 1 < lines.length && RE_TABLE_SEP.test(lines[i + 1]) && lines[i + 1].includes('-'));
}

function cells(row) {
  row = row.trim().replace(/^\|/, '').replace(/(^|[^\\])\|$/, '$1');
  const out = []; let cur = '', code = false;
  for (let i = 0; i < row.length; i++) {
    const c = row[i];
    if (c === '\\' && row[i + 1] === '|') { cur += code ? '|' : '\\|'; i++; continue; }  // (GitHub's: | in a cell)
    if (c === '`') code = !code;
    if (c === '|' && !code) { out.push(cur.trim()); cur = ''; continue; }
    cur += c;
  }
  out.push(cur.trim());
  return out;
}

function blocks(lines, ctx) {
  let html = '', i = 0;
  while (i < lines.length) {
    const line = lines[i];
    if (blank(line)) { i++; continue; }
    let m;
    if ((m = RE_FENCE.exec(line))) {                                     // A code block
      const pad = m[1].length, fence = m[2], body = [];
      for (i++; i < lines.length && !new RegExp('^\\s*' + fence[0] + '{' + fence.length + ',}\\s*$').test(lines[i]); i++)
        body.push(lines[i].slice(Math.min(pad, indentOf(lines[i]))));
      i++;
      html += '<pre><code>' + esc(body.join('\n')) + '</code></pre>\n';
      continue;
    }
    if ((m = RE_HEADING.exec(line))) {                                   // A heading
      const level = m[1].length, id = ctx.anchor(m[2]);
      html += `<h${level} id="${id}">${inline(m[2], ctx)}</h${level}>\n`;
      ctx.headings.push({ level, id, text: m[2] });
      i++;
      continue;
    }
    if (RE_HR.test(line)) { html += '<hr>\n'; i++; continue; }
    if (line.includes('|') && i + 1 < lines.length && RE_TABLE_SEP.test(lines[i + 1]) && lines[i + 1].includes('-')) {
      const head = cells(line), align = cells(lines[i + 1]).map(c =>
        /^:-*:$/.test(c) ? 'center' : /-:$/.test(c) ? 'right' : /^:/.test(c) ? 'left' : '');
      const td = (tag, c, k) => `<${tag}${align[k] ? ` style="text-align:${align[k]}"` : ''}>${inline(c, ctx)}</${tag}>`;
      const empty = head.every(c => c === '');
      html += '<table>' + (empty ? '' : '<thead><tr>' + head.map((c, k) => td('th', c, k)).join('') + '</tr></thead>') + '<tbody>';
      for (i += 2; i < lines.length && !blank(lines[i]) && lines[i].includes('|'); i++) {
        const row = cells(lines[i]);
        while (row.length < head.length) row.push('');
        html += '<tr>' + row.slice(0, head.length).map((c, k) => td('td', c, k)).join('') + '</tr>';
      }
      html += '</tbody></table>\n';
      continue;
    }
    if (RE_QUOTE.test(line)) {                                           // A quote
      const body = [];
      for (; i < lines.length && !blank(lines[i]); i++) body.push(lines[i].replace(RE_QUOTE, ''));
      html += '<blockquote>' + blocks(body, ctx) + '</blockquote>\n';
      continue;
    }
    if ((m = RE_ITEM.exec(line))) { const r = list(lines, i, ctx); html += r.html; i = r.i; continue; }
    const para = [line.trim()];                                          // A paragraph
    for (i++; i < lines.length && !blank(lines[i]) && !startsBlock(lines, i); i++) para.push(lines[i].trim());
    html += '<p>' + inline(para.join('\n'), ctx) + '</p>\n';
  }
  return html;
}

function list(lines, i, ctx) {
  const first = RE_ITEM.exec(lines[i]), base = first[1].length, ordered = /\d/.test(first[2]);
  let html = ordered ? `<ol${parseInt(first[2]) !== 1 ? ` start="${parseInt(first[2])}"` : ''}>` : '<ul>';
  while (i < lines.length) {
    const m = RE_ITEM.exec(lines[i]);
    if (!m || m[1].length < base || m[1].length > base + 1 || /\d/.test(m[2]) !== ordered) break;
    const inner = m[1].length + m[2].length + Math.max(1, Math.min(m[3].length, 4));
    const body = [m[4]];
    let loose = false;
    for (i++; i < lines.length; i++) {
      const l = lines[i];
      if (blank(l)) {
        let k = i; while (k < lines.length && blank(lines[k])) k++;
        if (k < lines.length && indentOf(lines[k]) >= inner) { body.push(''); loose = true; continue; }
        break;
      }
      if (indentOf(l) > base && (indentOf(l) >= inner || RE_ITEM.test(l) || RE_FENCE.test(l))) {
        body.push(l.slice(Math.min(indentOf(l), inner)));
        continue;
      }
      if (!startsBlock(lines, i) && !blank(body[body.length - 1])) { body.push(l.trim()); continue; }  // Lazy
      break;
    }
    let item = blocks(body, ctx);
    if (!loose) item = item.replace(/^<p>([\s\S]*?)<\/p>\n/, '$1\n');
    html += '<li>' + item.trim() + '</li>\n';
    let k = i; while (k < lines.length && blank(lines[k])) k++;     // (A blank line between items)
    const next = k < lines.length && RE_ITEM.exec(lines[k]);
    if (next && next[1].length >= base && next[1].length <= base + 1 && /\d/.test(next[2]) === ordered) i = k;
    else break;
  }
  return { html: html + (ordered ? '</ol>\n' : '</ul>\n'), i };
}

// ---------------------------------------------------------------------------
// The book

function book(docs) {
  const included = new Set(docs);
  const version = fs.existsSync(path.join(ROOT, 'os_rom/VERSION')) ?
    fs.readFileSync(path.join(ROOT, 'os_rom/VERSION'), 'utf8').trim() : '';
  const parts = docs.map(f => {
    const seen = {};
    const ctx = {
      headings: [],
      anchor(text) {
        let s = slug(text);
        if (seen[s] !== undefined) s += '-' + ++seen[s]; else seen[s] = 0;
        return docId(f) + '--' + s;
      },
      link(url) {
        if (/^[a-z]+:/i.test(url)) return url;
        const [p, a] = url.split('#');
        const target = p ? path.posix.normalize(path.posix.join(path.posix.dirname(f), p)).replace(/\/$/, '') : f;
        if (included.has(target)) return '#' + docId(target) + (a ? '--' + a : '');
        return REPO_URL + target + (a ? '#' + a : '');
      },
    };
    const text = fs.readFileSync(path.join(ROOT, f), 'utf8').replace(/^\uFEFF/, '').split(/\r?\n/);
    const body = blocks(text, ctx);
    const title = (ctx.headings[0] && plain(ctx.headings[0].text)) || f;
    const sections = ctx.headings.filter(h => h.level === 3).map(h => ({ id: h.id, title: plain(h.text) }));
    return { f, id: docId(f), title, body, sections };
  });
  // The contents: each document, and its sections (which also makes each a destination in the PDF, for the
  // outline: the browser keeps only the destinations something links to)
  const contents = parts.map(p =>
    `<li><a href="#${p.id}"><b>${esc(p.title)}</b></a> <span class="path">${esc(p.f)}</span>` +
    (p.sections.length ? '<div class="secs">' + p.sections.map(s => `<a href="#${s.id}">${esc(s.title)}</a>`).join(' &middot; ') + '</div>' : '') +
    '</li>').join('\n');
  const html = `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>The Hydra-16 documentation</title>
<style>
@page { size: Letter; margin: 16mm 15mm 18mm 15mm;
  @bottom-center { content: counter(page); font: 9pt Calibri, "Segoe UI", Arial, sans-serif; color: #666; } }
body { font: 10pt/1.42 Calibri, "Segoe UI", Arial, sans-serif; color: #1a1a1a; }
.doc { break-before: page; }
.path { color: #777; font-size: 8.5pt; }
.doc > .path:first-child { display: block; text-align: right; margin-bottom: 2mm; }
h1, h2, h3, h4, h5, h6 { break-after: avoid; line-height: 1.2; margin: 1.1em 0 .45em; }
h2 { font-size: 19pt; border-bottom: 2px solid #345; padding-bottom: 3px; margin-top: 0; }
h3 { font-size: 13.5pt; color: #234; border-bottom: 1px solid #ccd; padding-bottom: 2px; }
h4 { font-size: 11.5pt; color: #234; }
h5, h6 { font-size: 10.5pt; }
p { margin: .45em 0; }
ul, ol { margin: .35em 0; padding-left: 1.5em; }
li { margin: .12em 0; }
li > p { margin: .2em 0; }
a { color: #1a4f8a; text-decoration: none; }
code { font: 8.6pt Consolas, "Cascadia Mono", Menlo, monospace; background: #f1f3f5; padding: 0 2px; border-radius: 2px; }
pre { background: #f6f8fa; border: 1px solid #dde; border-radius: 3px; padding: 6px 8px; white-space: pre-wrap;
  overflow-wrap: anywhere; break-inside: avoid-page; }
pre code { background: none; padding: 0; font-size: 8.2pt; line-height: 1.3; }
table { border-collapse: collapse; margin: .55em 0; font-size: 9pt; width: auto; max-width: 100%; }
th, td { border: 1px solid #ccd; padding: 2px 5px; vertical-align: top; text-align: left; overflow-wrap: break-word; }
th { background: #eef1f5; }
tr { break-inside: avoid; }
blockquote { margin: .5em 0; padding: 0 .8em; border-left: 3px solid #ccd; color: #444; }
hr { border: 0; border-top: 1px solid #dde; margin: 1em 0; }
.cover h1 { font-size: 30pt; margin: 30mm 0 2mm; border: 0; }
.cover .sub { font-size: 13pt; color: #456; margin-bottom: 12mm; }
.cover ol { font-size: 10.5pt; line-height: 1.45; }
.cover li { margin: .3em 0; break-inside: avoid; }
.cover .secs { font-size: 8.5pt; line-height: 1.35; color: #444; }
</style></head><body>
<section class="cover">
<h1>The Hydra-16</h1>
<div class="sub">A multitasking 65C02 computer and its operating system: all of the documentation${version ? ' (OS ' + esc(version) + ')' : ''}</div>
<h3>The documents</h3>
<ol>
${contents}
</ol>
<p class="path">Made from the Markdown in the repository by sim/tools/mkpdf.js.  The documents are also on GitHub: ${esc(REPO_URL.replace(/blob\/main\/$/, ''))}</p>
</section>
${parts.map(p => `<section class="doc" id="${p.id}"><span class="path">${esc(p.f)}</span>\n${p.body}</section>`).join('\n')}
</body></html>
`;
  return { html, parts };
}

// ---------------------------------------------------------------------------
// The PDF's outline (its bookmarks): each document, with its sections under it, added to the browser's PDF as
// an incremental update.  (The browser makes an outline only with a tagged PDF, which is more than twice the
// size: a structure element for every table cell.)  Each entry goes to a named destination the browser made.

function addOutline(file, parts) {
  let pdf = fs.readFileSync(file, 'latin1');
  const startxref = +/startxref\s+(\d+)\s+%%EOF\s*$/.exec(pdf)[1];
  const trailer = pdf.slice(pdf.lastIndexOf('trailer'));
  const size = +/\/Size (\d+)/.exec(trailer)[1], root = +/\/Root (\d+) 0 R/.exec(trailer)[1];
  const info = /\/Info (\d+ 0 R)/.exec(trailer);
  const catStart = pdf.search(new RegExp('(^|\\n)' + root + ' 0 obj'));
  const catalog = pdf.slice(pdf.indexOf('<<', catStart), pdf.indexOf('endobj', catStart)).trim();
  const destsObj = /\/Dests (\d+) 0 R/.exec(catalog);
  let dests = '';
  if (destsObj) { const at = pdf.search(new RegExp('(^|\\n)' + destsObj[1] + ' 0 obj')); dests = pdf.slice(at, pdf.indexOf('endobj', at)); }
  const has = new Set([...dests.matchAll(/\/([^\s\/\[\]<>()]+)\s*\[/g)].map(m => m[1]));

  const text = s => '<FEFF' + Buffer.from(s, 'utf16le').swap16().toString('hex').toUpperCase() + '>';
  const items = parts.filter(p => has.has(p.id)).map(p => ({ title: p.title, dest: p.id,
    kids: p.sections.filter(s => has.has(s.id)).map(s => ({ title: s.title, dest: s.id, kids: [] })) }));
  let next = size;
  const objs = [];
  const number = list => list.forEach(it => { it.n = next++; number(it.kids); });
  const outlines = next++;
  number(items);
  const emit = (list, parent) => list.forEach((it, k) => {
    let d = `<</Title ${text(it.title)} /Parent ${parent} 0 R /Dest /${it.dest}`;
    if (k > 0) d += ` /Prev ${list[k - 1].n} 0 R`;
    if (k < list.length - 1) d += ` /Next ${list[k + 1].n} 0 R`;
    if (it.kids.length) d += ` /First ${it.kids[0].n} 0 R /Last ${it.kids[it.kids.length - 1].n} 0 R /Count -${it.kids.length}`;
    objs.push([it.n, d + '>>']);
    emit(it.kids, it.n);
  });
  objs.push([outlines, `<</Type /Outlines /First ${items[0].n} 0 R /Last ${items[items.length - 1].n} 0 R /Count ${items.length}>>`]);
  emit(items, outlines);
  objs.push([root, catalog.replace(/>>$/, '/Outlines ' + outlines + ' 0 R\n/PageMode /UseOutlines>>')]);

  let add = pdf.endsWith('\n') ? '' : '\n';
  const at = {};
  for (const [n, body] of objs) { at[n] = pdf.length + Buffer.byteLength(add, 'latin1'); add += `${n} 0 obj\n${body}\nendobj\n`; }
  const xref = pdf.length + Buffer.byteLength(add, 'latin1');
  const entry = n => String(at[n]).padStart(10, '0') + ' 00000 n \n';
  add += `xref\n0 1\n0000000000 65535 f \n${root} 1\n${entry(root)}${outlines} ${next - outlines}\n`;
  for (let n = outlines; n < next; n++) add += entry(n);
  add += `trailer\n<</Size ${next}\n/Root ${root} 0 R${info ? '\n/Info ' + info[1] : ''}\n/Prev ${startxref}>>\nstartxref\n${xref}\n%%EOF\n`;
  fs.appendFileSync(file, add, 'latin1');
  return items.reduce((n, it) => n + 1 + it.kids.length, 0);
}

// ---------------------------------------------------------------------------

function main(argv) {
  let out = path.join(ROOT, 'docs/hydra-16.pdf'), htmlOut = null, browser = process.env.HYDRA_BROWSER || null;
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--html') htmlOut = path.resolve(argv[++i]);
    else if (argv[i] === '--browser') browser = argv[++i];
    else out = path.resolve(argv[i]);
  }
  browser = browser || BROWSERS.find(b => fs.existsSync(b));
  if (!browser) throw new Error('no Edge or Chrome found: name one with --browser PATH');
  const docs = allDocs();
  const { html, parts } = book(docs);
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'mkpdf-'));
  const page = path.join(tmp, 'hydra-16.html');
  fs.writeFileSync(page, html);
  if (htmlOut) fs.writeFileSync(htmlOut, html);
  if (fs.existsSync(out)) fs.unlinkSync(out);
  execFileSync(browser, ['--headless', '--disable-gpu', '--no-first-run', '--disable-extensions',
    '--user-data-dir=' + path.join(tmp, 'profile'), '--no-pdf-header-footer', '--disable-pdf-tagging',
    '--print-to-pdf=' + out, 'file:///' + page.replace(/\\/g, '/')], { stdio: 'ignore', timeout: 180000 });
  try { fs.rmSync(tmp, { recursive: true, force: true }); } catch (e) { }   // (The browser may still hold its profile)
  if (!fs.existsSync(out)) throw new Error('the browser made no PDF');
  const marks = addOutline(out, parts);
  console.log(`${path.relative(ROOT, out)}: ${docs.length} documents, ${marks} bookmarks, ` +
    `${(fs.statSync(out).size / 1024).toFixed(0)}K`);
}

if (require.main === module) {
  try { main(process.argv.slice(2)); } catch (e) { console.error('mkpdf: ' + e.message); process.exit(1); }
}

module.exports = { slug, inline, blocks, book, addOutline };
