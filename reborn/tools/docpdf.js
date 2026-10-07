#!/usr/bin/env node
// ****************************************************************************
// docpdf.js - the documents as one book to print, docs/hydra-16.pdf: the guide, the tutorial, the guides for using
// the Hydra, the programmer's guide and the SDKs', and the hardware reference, a chapter each.  Their links are links
// within the book, or to the repository on GitHub for a document that isn't in it.  It makes an HTML page of them
// (with a Markdown converter of its own: what the documents use, read GitHub's way) and has Edge or Chrome print it,
// headless, over the DevTools protocol: page numbers, and the PDF's outline from the headings.  It isn't part of
// node build.js (it needs a browser): run it after changing a document in the book, and commit the PDF with it.
// (The old system's documents had their own, old/sim/tools/mkpdf.js, made the same way: old/docs/hydra-16.pdf.)
//
// Usage: node tools/docpdf.js [--out FILE] [--html FILE] [--browser PATH] [--a4]
//   --out FILE      the PDF (default docs/hydra-16.pdf)
//   --html FILE     the HTML page it prints (default obj/docs/hydra-16.html)
//   --browser PATH  Edge's or Chrome's program (default: $HYDRA_BROWSER, then the usual places)
//   --a4            A4 paper (default US Letter)
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn } = require('child_process');

const ROOT = path.join(__dirname, '..');                     // reborn/
const REPO = path.join(ROOT, '..');
const GITHUB = 'https://github.com/danstruthers/hydra-16/blob/main/';

// The book: its parts, and each part's documents (paths from the repository's top), in order
const BOOK = [
  ['The guide', ['reborn/docs/hydra-16.md']],
  ['First steps', ['reborn/docs/tutorial.md']],
  ['Using the Hydra', ['reborn/docs/using/README.md', 'reborn/docs/using/rc.md', 'reborn/docs/using/tools.md',
    'reborn/docs/using/hyforth.md', 'reborn/docs/using/hylang.md', 'reborn/docs/using/basic.md']],
  ['Programming', ['reborn/docs/programming/README.md', 'reborn/docs/programming/calls.md',
    'reborn/docs/programming/memory.md', 'reborn/docs/programming/tasks.md', 'reborn/docs/programming/files.md',
    'reborn/docs/programming/servers.md', 'reborn/docs/programming/modules.md', 'reborn/docs/programming/video.md',
    'reborn/sdk/asm/README.md', 'reborn/sdk/c/README.md']],
  ['The hardware', ['reborn/docs/hardware.md']],
];

// ---------------------------------------------------------------------------------------------------------------
// Markdown, GitHub's way (what these documents use: headings, paragraphs, lists, tables, code blocks, code spans,
// links, emphasis)

const esc = s => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

// A heading's anchor, as GitHub makes it: its text in lower case, punctuation gone, each space a hyphen
const slug = text => text.toLowerCase().replace(/[^\p{L}\p{M}\p{N}\p{Pc} -]/gu, '').replace(/ /g, '-');

// Inline text without its markup (a heading's, for its anchor and the outline)
const plain = s => s.replace(/`+([^`]*?)`+/g, '$1').replace(/\[([^\]]*)\]\([^)]*\)/g, '$1').replace(/\*\*|__/g, '')
  .replace(/(^|[^\w])[*_](?=\S)(.+?)(?<=\S)[*_](?!\w)/g, '$1$2').replace(/\\(.)/g, '$1');

// Inline markup: code spans, escapes, links (ctx.href rewrites a link's target), bare and <> URLs, emphasis
function inline(s, ctx) {
  const held = [];
  const hold = html => '\u0000' + (held.push(html) - 1) + '\u0001';
  s = s.replace(/(`+)([^]*?[^`])\1(?!`)/g, (m, ticks, code) => {
    if (/^ .* $/.test(code)) code = code.slice(1, -1);
    return hold('<code>' + esc(code) + '</code>');
  });
  s = s.replace(/\\([\\`*_{}\[\]()#+\-.!|<>~])/g, (m, c) => hold(esc(c)));
  s = s.replace(/<(https?:\/\/[^>\s]+)>/g, (m, url) => hold('<a href="' + esc(url) + '">' + esc(url) + '</a>'));
  const emph = t => t.replace(/\*\*(?=\S)(.+?)(?<=\S)\*\*/g, '<strong>$1</strong>')
    .replace(/(^|[^\w*])\*(?=\S)(.+?)(?<=\S)\*(?![\w*])/g, '$1<em>$2</em>')
    .replace(/(^|[^\w])_(?=\S)(.+?)(?<=\S)_(?!\w)/g, '$1<em>$2</em>');
  s = s.replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, (m, text, url) =>
    hold('<a href="' + esc(ctx.href(url)) + '">' + emph(esc(text)) + '</a>'));
  s = s.replace(/https?:\/\/[^\s<>()\u0000\u0001]*[^\s<>().,;:\u0000\u0001]/g, url => hold('<a href="' + esc(url) + '">' + esc(url) + '</a>'));
  s = emph(esc(s));
  while (/\u0000/.test(s)) s = s.replace(/\u0000(\d+)\u0001/g, (m, n) => held[n]);
  return s;
}

const FENCE = /^(\s*)(```+|~~~+)/;
const HEADING = /^(#{1,6})\s+(.*?)\s*#*\s*$/;
const RULE = /^ {0,3}([-*_])( *\1){2,} *$/;
const ITEM = /^(\s*)([*+-]|\d+[.)])\s+/;
const TABLE_SEP = /^\s*\|?\s*:?-+:?\s*(\|\s*:?-+:?\s*)*\|?\s*$/;
const isTable = (lines, i) => /^\s*\|/.test(lines[i]) && i + 1 < lines.length && TABLE_SEP.test(lines[i + 1]);
const blank = l => !l.trim();
const indentOf = l => l.match(/^\s*/)[0].length;

// A table row's cells: split on each | not escaped (in code spans too, as GitHub does), then \| made |
function cells(line) {
  let s = line.trim();
  if (s.startsWith('|')) s = s.slice(1);
  if (s.endsWith('|') && !s.endsWith('\\|')) s = s.slice(0, -1);
  const out = [];
  let cur = '';
  for (let i = 0; i < s.length; i++) {
    if (s[i] === '\\' && s[i + 1] === '|') { cur += '\\|'; i++; }
    else if (s[i] === '|') { out.push(cur); cur = ''; }
    else cur += s[i];
  }
  out.push(cur);
  return out.map(c => c.trim().replace(/\\\|/g, '|'));
}

// Blocks: lines to HTML.  tight: a list item's, its paragraphs without <p>
function blocks(lines, ctx, tight = false) {
  const out = [];
  let i = 0;
  const startsBlock = j => FENCE.test(lines[j]) || HEADING.test(lines[j]) || RULE.test(lines[j]) || isTable(lines, j) ||
    /^\s*([*+-]|1[.)])\s+/.test(lines[j]);
  while (i < lines.length) {
    const l = lines[i];
    let m;
    if (blank(l)) { i++; continue; }
    if ((m = FENCE.exec(l))) {                                  // A code block, its indent taken off
      const ind = m[1].length, code = [];
      for (i++; i < lines.length && !new RegExp('^\\s*' + m[2][0] + '{' + m[2].length + ',}\\s*$').test(lines[i]); i++)
        code.push(lines[i].slice(Math.min(ind, indentOf(lines[i]))));
      i++;
      out.push('<pre><code>' + esc(code.join('\n')) + '</code></pre>');
    } else if ((m = HEADING.exec(l))) {
      const level = m[1].length, text = m[2];
      out.push(ctx.heading(level, text));
      i++;
    } else if (RULE.test(l)) {
      out.push('<hr>');
      i++;
    } else if (isTable(lines, i)) {
      const head = cells(lines[i]), align = cells(lines[i + 1]).map(c =>
        /^:-+:$/.test(c) ? 'center' : /-:$/.test(c) ? 'right' : '');
      const td = (tag, c, k) => '<' + tag + (align[k] ? ' style="text-align:' + align[k] + '"' : '') + '>' + inline(c, ctx) + '</' + tag + '>';
      const rows = [];
      for (i += 2; i < lines.length && /^\s*\|/.test(lines[i]); i++) rows.push(cells(lines[i]));
      const empty = head.every(c => !c);                       // (A table of two columns with no headings)
      out.push('<table>' + (empty ? '' : '<thead><tr>' + head.map((c, k) => td('th', c, k)).join('') + '</tr></thead>') +
        '<tbody>' + rows.map(r => '<tr>' + r.map((c, k) => td('td', c, k)).join('') + '</tr>').join('') + '</tbody></table>');
    } else if ((m = ITEM.exec(l))) {                            // A list: each item's lines, its indent taken off,
      const base = m[1].length, ordered = /\d/.test(m[2]);      //   are blocks of their own
      const items = [];
      let loose = false;
      while (i < lines.length) {
        const k = lines[i], im = ITEM.exec(k);
        if (blank(k)) {
          let j = i + 1;
          while (j < lines.length && blank(lines[j])) j++;
          const next = j < lines.length ? ITEM.exec(lines[j]) : null;
          const cur = items[items.length - 1];
          if (j < lines.length && ((next && indentOf(lines[j]) <= base + 1 && /\d/.test(next[2]) === ordered) ||
            indentOf(lines[j]) >= cur.indent)) {
            loose = true;
            cur.lines.push('');
            i = j;
            continue;
          }
          break;
        }
        if (im && im[1].length <= base + 1) {
          if (/\d/.test(im[2]) !== ordered) break;
          items.push({ start: parseInt(im[2], 10), indent: im[0].length, lines: [k.slice(im[0].length)] });
          i++;
          continue;
        }
        if (!im && !items[items.length - 1].lines.some(blank) && indentOf(k) <= base && startsBlock(i)) break;
        const cur = items[items.length - 1];
        cur.lines.push(k.slice(Math.min(cur.indent, indentOf(k))));
        i++;
      }
      const tag = ordered ? 'ol' : 'ul';
      out.push('<' + tag + (ordered && items[0].start !== 1 ? ' start="' + items[0].start + '"' : '') + '>' +
        items.map(it => '<li>' + blocks(it.lines, ctx, !loose) + '</li>').join('') + '</' + tag + '>');
    } else {                                                   // A paragraph
      const para = [l.trim()];
      for (i++; i < lines.length && !blank(lines[i]) && !startsBlock(i); i++) para.push(lines[i].trim());
      const html = inline(para.join('\n'), ctx);
      out.push(tight ? html : '<p>' + html + '</p>');
    }
  }
  return out.join('\n');
}

// ---------------------------------------------------------------------------------------------------------------
// The book

const docId = file => 'd-' + file.replace(/^reborn\//, '').replace(/\.md$/, '').replace(/[^A-Za-z0-9]+/g, '-');

function book() {
  const files = BOOK.flatMap(([, f]) => f), inBook = new Set(files);
  const anchors = new Map(files.map(f => [f, new Set()])), links = [];
  const chapters = [], titles = new Map();
  for (const file of files) {
    const lines = fs.readFileSync(path.join(REPO, file), 'utf8').split(/\r?\n/);
    const seen = new Map(), id = docId(file);
    const ctx = {
      heading(level, text) {                                    // Its anchor GitHub's, with the document's id before it
        let a = slug(plain(text));
        const n = seen.get(a) || 0;
        seen.set(a, n + 1);
        if (n) a += '-' + n;
        anchors.get(file).add(a);
        if (level === 1 && !titles.has(file)) titles.set(file, plain(text));
        return '<h' + level + ' id="' + id + '--' + a + '">' + inline(text, ctx) + '</h' + level + '>';
      },
      href(url) {                                               // A link: within the book, or to GitHub
        if (/^[a-z]+:/i.test(url)) return url;
        const [p, frag] = url.split('#');
        const target = p ? path.posix.normalize(path.posix.join(path.posix.dirname(file), p)) : file;
        if (inBook.has(target)) {
          links.push([file, target, frag]);
          return '#' + docId(target) + (frag ? '--' + frag : '');
        }
        const abs = path.join(REPO, target);
        if (!fs.existsSync(abs)) console.warn('docpdf: ' + file + ': no ' + target);
        const dir = fs.existsSync(abs) && fs.statSync(abs).isDirectory();
        return GITHUB.replace('/blob/', dir ? '/tree/' : '/blob/') + target.replace(/\/$/, '') + (frag ? '#' + frag : '');
      },
    };
    chapters.push('<section class="chapter" id="' + id + '">\n' + blocks(lines, ctx) + '\n</section>');
  }
  for (const [from, to, frag] of links)
    if (frag && !anchors.get(to).has(frag)) console.warn('docpdf: ' + from + ': no anchor ' + to + '#' + frag);
  return { chapters, titles };
}

const CSS = `
:root { color-scheme: light; }
body { font: 10pt/1.38 Cambria, Georgia, "Times New Roman", serif; color: #111; background: #fff; margin: 0; }
h1, h2, h3, h4, .title, .contents, .part { font-family: "Segoe UI", Calibri, Arial, sans-serif; }
h1 { font-size: 20pt; margin: 0 0 10pt; padding-bottom: 4pt; border-bottom: 1.5pt solid #333; }
h2 { font-size: 14pt; margin: 16pt 0 6pt; break-after: avoid; }
h3 { font-size: 11.5pt; margin: 12pt 0 4pt; break-after: avoid; }
h4, h5, h6 { font-size: 10.5pt; margin: 10pt 0 4pt; break-after: avoid; }
p { margin: 0 0 6pt; orphans: 3; widows: 3; }
a { color: #0b4f9c; text-decoration: none; }
code { font: 8.6pt Consolas, "Courier New", monospace; background: #f3f3f3; border-radius: 2pt; }
pre { background: #f6f6f6; border: 0.5pt solid #ddd; padding: 4pt 5pt; margin: 0 0 7pt; break-inside: avoid; }
pre code { font-size: 7.8pt; line-height: 1.3; background: none; padding: 0; white-space: pre; }
table { border-collapse: collapse; margin: 0 0 8pt; font-size: 8.6pt; line-height: 1.3; width: 100%; }
th, td { border: 0.5pt solid #bbb; padding: 2pt 4pt; vertical-align: top; text-align: left; overflow-wrap: break-word; }
th { background: #eee; }
td code, th code { font-size: 7.9pt; }
tr { break-inside: avoid; }
ul, ol { margin: 0 0 6pt; padding-left: 18pt; }
li { margin: 1pt 0; }
li > ul, li > ol { margin: 1pt 0 2pt; }
hr { border: none; border-top: 0.5pt solid #bbb; margin: 10pt 0; }
.chapter { break-before: page; }
.title { break-after: page; padding-top: 2.6in; text-align: center; }
.title .name { font-size: 30pt; font-weight: 600; }
.title .sub { font-size: 14pt; margin-top: 10pt; color: #333; }
.title .what { font-size: 10.5pt; margin-top: 30pt; color: #444; line-height: 1.6; }
.title .when { font-size: 10pt; margin-top: 60pt; color: #555; }
.contents h1 { border: none; }
.contents .part { font-size: 12pt; font-weight: 600; margin: 12pt 0 4pt; }
.contents ul { list-style: none; padding-left: 12pt; font-size: 10.5pt; }
`;

function html() {
  const { chapters, titles } = book();
  const date = new Date().toISOString().slice(0, 10);
  const contents = BOOK.map(([part, files]) => '<div class="part">' + esc(part) + '</div><ul>' +
    files.map(f => '<li><a href="#' + docId(f) + '">' + esc(titles.get(f) || f) + '</a></li>').join('') + '</ul>').join('\n');
  return '<!doctype html>\n<html lang="en"><head><meta charset="utf-8"><title>The Hydra-16 and HydraOS</title>\n' +
    '<style>' + CSS + '</style></head><body>\n' +
    '<div class="title"><div class="name">The Hydra-16 and HydraOS</div>' +
    '<div class="sub">The whole system in one place</div>' +
    '<div class="what">The guide, the tutorial, the guides for using it,<br>the programmer\'s guide and the hardware reference</div>' +
    '<div class="when">' + date + '<br>From the repository\'s documents (reborn/docs): node reborn/tools/docpdf.js</div></div>\n' +
    '<section class="contents"><h1>Contents</h1>\n' + contents + '</section>\n' + chapters.join('\n') + '\n</body></html>\n';
}

// ---------------------------------------------------------------------------------------------------------------
// Printing: the browser headless, a page opened on the HTML, Page.printToPDF

function findBrowser(given) {
  const list = [given, process.env.HYDRA_BROWSER,
    'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe', 'C:/Program Files/Microsoft/Edge/Application/msedge.exe',
    'C:/Program Files/Google/Chrome/Application/chrome.exe', 'C:/Program Files (x86)/Google/Chrome/Application/chrome.exe',
    '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome', '/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge',
    '/usr/bin/google-chrome', '/usr/bin/chromium', '/usr/bin/chromium-browser', '/usr/bin/microsoft-edge'];
  for (const b of list) if (b && fs.existsSync(b)) return b;
  throw new Error('docpdf: no Edge or Chrome found (--browser PATH)');
}

async function print(htmlFile, pdfFile, browser, a4) {
  const profile = fs.mkdtempSync(path.join(os.tmpdir(), 'docpdf-'));
  const proc = spawn(browser, ['--headless=new', '--disable-gpu', '--no-first-run', '--no-default-browser-check',
    '--remote-debugging-port=0', '--user-data-dir=' + profile, 'about:blank'], { stdio: ['ignore', 'ignore', 'pipe'] });
  try {
    const wsUrl = await new Promise((resolve, reject) => {     // "DevTools listening on ws://..."
      let err = '';
      const timer = setTimeout(() => reject(new Error('docpdf: the browser didn\'t start: ' + err)), 30000);
      proc.stderr.on('data', d => {
        err += d;
        const m = /DevTools listening on (ws:\/\/\S+)/.exec(err);
        if (m) { clearTimeout(timer); resolve(m[1]); }
      });
      proc.on('exit', code => reject(new Error('docpdf: the browser ended (' + code + '): ' + err)));
    });
    const ws = new WebSocket(wsUrl);
    await new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = reject; });
    let next = 1;
    const waiting = new Map(), events = [];
    ws.onmessage = e => {
      const msg = JSON.parse(e.data);
      if (msg.id && waiting.has(msg.id)) {
        const { resolve, reject } = waiting.get(msg.id);
        waiting.delete(msg.id);
        if (msg.error) reject(new Error('docpdf: ' + msg.error.message)); else resolve(msg.result);
      } else if (msg.method) events.forEach(f => f(msg));
    };
    const send = (method, params = {}, sessionId) => new Promise((resolve, reject) => {
      const id = next++;
      waiting.set(id, { resolve, reject });
      ws.send(JSON.stringify(sessionId ? { id, method, params, sessionId } : { id, method, params }));
    });
    const { targetId } = await send('Target.createTarget', { url: 'about:blank' });
    const { sessionId } = await send('Target.attachToTarget', { targetId, flatten: true });
    await send('Page.enable', {}, sessionId);
    const loaded = new Promise(resolve => events.push(m => m.method === 'Page.loadEventFired' && m.sessionId === sessionId && resolve()));
    await send('Page.navigate', { url: 'file:///' + path.resolve(htmlFile).replace(/\\/g, '/') }, sessionId);
    await loaded;
    await send('Runtime.evaluate', { expression: 'document.fonts.ready.then(() => true)', awaitPromise: true }, sessionId);
    const foot = '<div style="width:100%;font:8px Segoe UI,Arial,sans-serif;color:#666;padding:0 0.6in;display:flex;' +
      'justify-content:space-between"><span>The Hydra-16 and HydraOS</span><span class="pageNumber"></span></div>';
    const { data } = await send('Page.printToPDF', {
      paperWidth: a4 ? 8.27 : 8.5, paperHeight: a4 ? 11.69 : 11, marginTop: 0.55, marginBottom: 0.6, marginLeft: 0.6,
      marginRight: 0.6, printBackground: true, displayHeaderFooter: true, headerTemplate: '<div></div>',
      footerTemplate: foot, generateTaggedPDF: true, generateDocumentOutline: true,
    }, sessionId);
    fs.writeFileSync(pdfFile, Buffer.from(data, 'base64'));
    await send('Browser.close').catch(() => {});
    ws.close();
  } finally {
    proc.kill();
    await new Promise(r => setTimeout(r, 300));
    try { fs.rmSync(profile, { recursive: true, force: true }); } catch (e) { }
  }
}

async function main() {
  const args = process.argv.slice(2), opt = (name, def) => {
    const k = args.indexOf(name);
    return k >= 0 ? args[k + 1] : def;
  };
  const pdfFile = opt('--out', path.join(ROOT, 'docs', 'hydra-16.pdf'));
  const htmlFile = opt('--html', path.join(ROOT, 'obj', 'docs', 'hydra-16.html'));
  fs.mkdirSync(path.dirname(htmlFile), { recursive: true });
  fs.writeFileSync(htmlFile, html());
  const browser = findBrowser(opt('--browser'));
  await print(htmlFile, pdfFile, browser, args.includes('--a4'));
  console.log('docpdf: ' + path.relative(process.cwd(), pdfFile) + ', ' + Math.round(fs.statSync(pdfFile).size / 1024) + 'K (' +
    path.basename(browser) + ')');
}

if (require.main === module) main().catch(e => { console.error(e.message); process.exit(1); });
module.exports = { blocks, inline, slug };
