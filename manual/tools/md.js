// md.js - the manual's Markdown, to HTML.  GitHub's kind, as much of it as the book uses: headings (with {#id}),
// paragraphs, emphasis, code spans and fenced blocks, links and images, block quotes, lists (nested; numbered from
// any start), tables (with alignment), rules, line breaks, HTML blocks and a few inline tags; and two of the book's
// own: definition lists (a line, then ": its definition") and session blocks (```session: a line starting with a
// prompt shows what's typed after it in bold).
'use strict';

const esc = s => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
const escAttr = s => esc(s).replace(/"/g, '&quot;');

// A heading's id: its text, lower case, letters, digits and dashes
function slug(text) {
  return text.toLowerCase().replace(/<[^>]*>/g, '').replace(/&[a-z]+;/g, '').replace(/[^a-z0-9\s.-]/g, '')
    .trim().replace(/[\s.]+/g, '-').replace(/-+/g, '-').replace(/^-|-$/g, '');
}

// ---- Inline text
const INLINE_TAGS = /^<\/?(kbd|br|sup|sub|span|small|u|b|i|em|strong|code|a|img|abbr|mark|s)\b[^>]*>/i;

function inline(s, ctx) {
  let out = '';
  let i = 0;
  while (i < s.length) {
    const c = s[i];
    // escapes
    if (c === '\\' && i + 1 < s.length && /[\\`*_{}\[\]()#+\-.!|<>~"']/.test(s[i + 1])) { out += esc(s[i + 1]); i += 2; continue; }
    // hard break: backslash at the end of a line
    if (c === '\\' && s[i + 1] === '\n') { out += '<br>\n'; i += 2; continue; }
    // code span
    if (c === '`') {
      let n = 0; while (s[i + n] === '`') n++;
      const fence = '`'.repeat(n);
      const end = s.indexOf(fence, i + n);
      if (end > 0) {
        let code = s.slice(i + n, end).replace(/\n/g, ' ');
        if (/^ .* $/.test(code)) code = code.slice(1, -1);
        out += '<code>' + esc(code) + '</code>';
        i = end + n; continue;
      }
      out += esc(fence); i += n; continue;
    }
    // images and links
    if (c === '!' && s[i + 1] === '[' || c === '[') {
      const img = c === '!';
      const open = img ? i + 1 : i;
      const close = matchBracket(s, open);
      if (close > 0 && s[close + 1] === '(') {
        const pend = matchParen(s, close + 1);
        if (pend > 0) {
          const text = s.slice(open + 1, close);
          let target = s.slice(close + 2, pend).trim(), title = '';
          const tm = /^(\S+)\s+"([^"]*)"$/.exec(target);
          if (tm) { target = tm[1]; title = tm[2]; }
          if (ctx && ctx.link) target = ctx.link(target);
          if (img) out += '<img src="' + escAttr(target) + '" alt="' + escAttr(text) + '"' + (title ? ' title="' + escAttr(title) + '"' : '') + '>';
          else out += '<a href="' + escAttr(target) + '"' + (title ? ' title="' + escAttr(title) + '"' : '') + '>' + inline(text, ctx) + '</a>';
          i = pend + 1; continue;
        }
      }
    }
    // autolinks and inline HTML
    if (c === '<') {
      const am = /^<(https?:\/\/[^>\s]+)>/.exec(s.slice(i));
      if (am) { out += '<a href="' + escAttr(am[1]) + '">' + esc(am[1]) + '</a>'; i += am[0].length; continue; }
      const hm = INLINE_TAGS.exec(s.slice(i));
      if (hm) { out += hm[0]; i += hm[0].length; continue; }
      out += '&lt;'; i++; continue;
    }
    // emphasis: ** or __ strong, * or _ em (an _ inside a word is just an underscore)
    if (c === '*' || c === '_') {
      const two = s[i + 1] === c;
      const mark = two ? c + c : c;
      const prev = s[i - 1] || ' ', next = s[i + mark.length] || ' ';
      const canOpen = !/\s/.test(next) && (c === '*' || !/[A-Za-z0-9]/.test(prev));
      if (canOpen) {
        let j = i + mark.length;
        let found = -1;
        while (j < s.length) {
          if (s[j] === '\\') { j += 2; continue; }
          if (s[j] === '`') { const k = s.indexOf('`', j + 1); if (k < 0) break; j = k + 1; continue; }
          if (s.startsWith(mark, j) && !/\s/.test(s[j - 1]) && (c === '*' || !/[A-Za-z0-9]/.test(s[j + mark.length] || ' '))
            && !(mark.length === 1 && s[j + 1] === c)) { found = j; break; }
          if (mark.length === 1 && s.startsWith(c + c, j)) { j += 2; continue; }
          j++;
        }
        if (found > 0) {
          const tag = two ? 'strong' : 'em';
          out += '<' + tag + '>' + inline(s.slice(i + mark.length, found), ctx) + '</' + tag + '>';
          i = found + mark.length; continue;
        }
      }
      out += esc(mark); i += mark.length; continue;
    }
    // hard break: two spaces at the end of a line
    if (c === ' ' && s.startsWith('  \n', i)) { out += '<br>\n'; i += 3; continue; }
    if (c === '&') { const em = /^&(#\d+|#x[0-9a-f]+|[a-z]+);/i.exec(s.slice(i)); if (em) { out += em[0]; i += em[0].length; continue; } }
    out += esc(c); i++;
  }
  return out;
}
function matchBracket(s, i) {
  let d = 0;
  for (let j = i; j < s.length; j++) {
    if (s[j] === '\\') { j++; continue; }
    if (s[j] === '`') { const k = s.indexOf('`', j + 1); if (k > 0) { j = k; continue; } }
    if (s[j] === '[') d++;
    else if (s[j] === ']') { d--; if (d === 0) return j; }
  }
  return -1;
}
function matchParen(s, i) {
  let d = 0;
  for (let j = i; j < s.length; j++) {
    if (s[j] === '(') d++;
    else if (s[j] === ')') { d--; if (d === 0) return j; }
  }
  return -1;
}

// ---- Blocks
const indentOf = l => { let n = 0; for (const ch of l) { if (ch === ' ') n++; else if (ch === '\t') n += 4; else break; } return n; };
const dedent = (l, n) => { let k = 0, w = 0; while (k < l.length && w < n && (l[k] === ' ' || l[k] === '\t')) { w += l[k] === '\t' ? 4 : 1; k++; } return l.slice(k); };
const LIST_RE = /^( *)([-*+]|\d{1,9}[.)])( +|$)/;
const HTML_BLOCK = /^<(\/?)(div|p|table|thead|tbody|tr|td|th|pre|section|aside|figure|figcaption|img|hr|h[1-6]|ul|ol|li|dl|dt|dd|blockquote|svg|center|style|br|!--)/i;

function blocks(lines, ctx) {
  const out = [];
  let i = 0;
  const para = [];
  const flush = () => { if (para.length) { out.push('<p>' + inline(para.join('\n'), ctx) + '</p>'); para.length = 0; } };
  while (i < lines.length) {
    const line = lines[i];
    const t = line.trim();
    if (!t) { flush(); i++; continue; }
    // fenced code
    const fm = /^(\s*)(```+|~~~+)\s*([\w-]*)(.*)$/.exec(line);
    if (fm) {
      flush();
      const fence = fm[2], info = fm[3], extra = fm[4].trim(), ind = fm[1].length;
      const body = [];
      i++;
      while (i < lines.length && !lines[i].trim().startsWith(fence)) { body.push(dedent(lines[i], ind)); i++; }
      i++;
      out.push(codeBlock(body.join('\n'), info, extra, ctx));
      continue;
    }
    // heading
    const hm = /^(#{1,6})\s+(.*?)\s*#*\s*$/.exec(line);
    if (hm) {
      flush();
      let text = hm[2], id = null;
      const im = /\s*\{#([\w.-]+)\}\s*$/.exec(text);
      if (im) { id = im[1]; text = text.slice(0, im.index); }
      const html = inline(text, ctx);
      id = id || slug(text);
      if (ctx && ctx.ids) { let base = id, k = 2; while (ctx.ids.has(id)) id = base + '-' + k++; ctx.ids.add(id); }
      if (ctx && ctx.headings) ctx.headings.push({ level: hm[1].length, text, html, id });
      out.push('<h' + hm[1].length + ' id="' + id + '">' + html + '</h' + hm[1].length + '>');
      i++; continue;
    }
    // rule
    if (/^(\*\s*){3,}$|^(-\s*){3,}$|^(_\s*){3,}$/.test(t) && !para.length) { out.push('<hr>'); i++; continue; }
    // HTML block
    if (!para.length && HTML_BLOCK.test(t)) {
      flush();
      const body = [];
      while (i < lines.length && lines[i].trim()) { body.push(lines[i]); i++; }
      out.push(body.join('\n'));
      continue;
    }
    // table
    if (t.startsWith('|') && i + 1 < lines.length && /^\s*\|?\s*:?-{2,}:?\s*(\|\s*:?-{2,}:?\s*)*\|?\s*$/.test(lines[i + 1])) {
      flush();
      const row = l => { let s = l.trim(); if (s.startsWith('|')) s = s.slice(1); if (s.endsWith('|') && !s.endsWith('\\|')) s = s.slice(0, -1);
        const cells = []; let cur = '';
        for (let k = 0; k < s.length; k++) {
          if (s[k] === '\\' && s[k + 1] === '|') { cur += '|'; k++; continue; }
          if (s[k] === '`') {                                   // (A code span, its | not a cell's end: `` ` `` too)
            let n = 0; while (s[k + n] === '`') n++;
            const end = s.indexOf('`'.repeat(n), k + n);
            const to = end > 0 ? end + n : k + n;
            cur += s.slice(k, to); k = to - 1; continue;
          }
          if (s[k] === '|') { cells.push(cur.trim()); cur = ''; } else cur += s[k];
        }
        cells.push(cur.trim()); return cells; };
      const head = row(line), aligns = row(lines[i + 1]).map(c => /^:-+:$/.test(c) ? 'center' : /-:$/.test(c) ? 'right' : /^:-/.test(c) ? 'left' : '');
      i += 2;
      const rows = [];
      while (i < lines.length && lines[i].trim().startsWith('|')) { rows.push(row(lines[i])); i++; }
      const cell = (tag, c, k) => '<' + tag + (aligns[k] ? ' style="text-align:' + aligns[k] + '"' : '') + '>' + inline(c, ctx) + '</' + tag + '>';
      out.push('<table><thead><tr>' + head.map((c, k) => cell('th', c, k)).join('') + '</tr></thead><tbody>' +
        rows.map(r => '<tr>' + r.map((c, k) => cell('td', c, k)).join('') + '</tr>').join('\n') + '</tbody></table>');
      continue;
    }
    // block quote (a "> **NOTE:**" one gets the note's class)
    if (/^\s*>/.test(line)) {
      flush();
      const body = [];
      while (i < lines.length && /^\s*>/.test(lines[i])) { body.push(lines[i].replace(/^\s*> ?/, '')); i++; }
      const inner = blocks(body, ctx).join('\n');
      const km = /^<p><strong>([A-Z][A-Z ]+?)[:!]?<\/strong>/.exec(inner);
      const kind = km ? km[1].toLowerCase().replace(/\s+/g, '-') : '';
      out.push('<blockquote' + (kind ? ' class="box ' + kind + '"' : '') + '>' + inner + '</blockquote>');
      continue;
    }
    // list
    const lm = LIST_RE.exec(line);
    if (lm && !(para.length && !/^(\s*)[-*+]\s/.test(line) && !/^\s*1[.)]/.test(line))) {
      flush();
      const r = list(lines, i, ctx);
      out.push(r.html); i = r.next; continue;
    }
    // definition list: a term, then ": definition"
    if (!para.length && i + 1 < lines.length && /^:\s+/.test(lines[i + 1])) {
      const items = [];
      while (i < lines.length && lines[i].trim() && i + 1 < lines.length && /^:\s+/.test(lines[i + 1])) {
        const term = lines[i]; i++;
        const defs = [];
        while (i < lines.length && /^:\s+/.test(lines[i])) {
          const d = [lines[i].replace(/^:\s+/, '')]; i++;
          while (i < lines.length && lines[i].trim() && /^\s{2,}/.test(lines[i]) && !/^:\s+/.test(lines[i])) { d.push(lines[i].trim()); i++; }
          defs.push(d.join('\n'));
        }
        items.push('<dt>' + inline(term, ctx) + '</dt>' + defs.map(d => '<dd>' + inline(d, ctx) + '</dd>').join(''));
        while (i < lines.length && !lines[i].trim()) {
          if (i + 2 < lines.length && lines[i + 1].trim() && /^:\s+/.test(lines[i + 2])) { i++; break; }
          break;
        }
      }
      out.push('<dl>' + items.join('\n') + '</dl>');
      continue;
    }
    para.push(line.replace(/^\s+/, ''));
    i++;
  }
  flush();
  return out;
}

function list(lines, start, ctx) {
  const first = LIST_RE.exec(lines[start]);
  const baseIndent = first[1].length;
  const ordered = /\d/.test(first[2]);
  const startNum = ordered ? parseInt(first[2], 10) : 1;
  const items = [];
  let i = start, loose = false, cur = null, blankBefore = false;
  while (i < lines.length) {
    const l = lines[i];
    const m = LIST_RE.exec(l);
    if (m && m[1].length === baseIndent && (/\d/.test(m[2]) === ordered)) {
      if (cur) items.push(cur);
      const contentIndent = m[1].length + m[2].length + Math.max(1, Math.min(m[3].length, 4));
      cur = { lines: [l.slice(Math.min(l.length, m[0].length))], indent: contentIndent };
      if (blankBefore && items.length) loose = true;
      blankBefore = false;
      i++; continue;
    }
    if (!l.trim()) {
      // a blank line: the list goes on if the next non-blank line is indented under it, or is another item
      let k = i + 1; while (k < lines.length && !lines[k].trim()) k++;
      if (k >= lines.length) break;
      const n = lines[k];
      const nm = LIST_RE.exec(n);
      if (indentOf(n) >= cur.indent || (nm && nm[1].length === baseIndent && /\d/.test(nm[2]) === ordered)) {
        if (indentOf(n) >= cur.indent) cur.lines.push('');
        blankBefore = true; i++; continue;
      }
      break;
    }
    if (indentOf(l) >= cur.indent || (indentOf(l) > baseIndent && cur)) { cur.lines.push(dedent(l, cur.indent)); if (blankBefore) loose = true; blankBefore = false; i++; continue; }
    // a lazy continuation of the item's paragraph
    if (cur && !m && !blankBefore && !/^\s*(#|```|>|\|)/.test(l)) { cur.lines.push(l.trim()); i++; continue; }
    break;
  }
  if (cur) items.push(cur);
  const html = items.map(it => {
    const inner = blocks(it.lines, ctx);
    if (!loose && inner.length && inner[0].startsWith('<p>')) inner[0] = inner[0].slice(3, -4);
    return '<li>' + inner.join('\n') + '</li>';
  }).join('\n');
  const tag = ordered ? 'ol' : 'ul';
  return { html: '<' + tag + (ordered && startNum !== 1 ? ' start="' + startNum + '"' : '') + '>' + html + '</' + tag + '>', next: i };
}

// Code blocks: ```session shows what's typed (after a prompt) in bold; ```text, ```forth ... plain.  A session's fence
// can name more prompts; "indented" says its lines that start with 8 spaces are output, not the second prompt's
const PROMPTS = [/^((?:[0-9a-z]:)?\/[^ ]*> )/, /^(% )/, /^(> )/, /^(hylang> )/, /^(\]\s?)/, /^(\* )/, /^(db> )/, /^(OK\s*$)/, /^( {8})(?=\S)/];
function codeBlock(code, info, extra, ctx) {
  const cls = info ? ' class="' + escAttr(info) + '"' : '';
  if (info === 'session') {
    const words = extra ? extra.split(/\s+/).filter(Boolean) : [];
    const prompts = words.filter(p => p !== 'indented').map(p => new RegExp('^(' + p.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace(/_/g, ' ') + ')'))
      .concat(words.includes('indented') ? PROMPTS.slice(0, -1) : PROMPTS);
    const html = code.split('\n').map(l => {
      if (l.startsWith('\u00ab') && l.endsWith('\u00bb')) return '<span class="note">' + esc(l.slice(1, -1)) + '</span>';
      if (l.startsWith('\u00bb ')) return '<b>' + esc(l.slice(2)) + '</b>';       // (A line typed with no prompt before it)
      for (const p of prompts) { const pm = p.exec(l); if (pm) return esc(pm[1]) + '<b>' + esc(l.slice(pm[1].length)) + '</b>'; }
      return esc(l);
    }).join('\n');
    return '<pre class="session"><code>' + html + '</code></pre>';
  }
  return '<pre' + cls + '><code>' + esc(code) + '</code></pre>';
}

// A document: { html, headings }
function render(text, opts = {}) {
  const ctx = { headings: [], ids: opts.ids || new Set(), link: opts.link };
  const lines = text.replace(/\r\n?/g, '\n').split('\n');
  return { html: blocks(lines, ctx).join('\n'), headings: ctx.headings };
}

module.exports = { render, slug, inline: (s, ctx) => inline(s, ctx || {}), esc };
