// ****************************************************************************
// select.js - the tests a change can affect: sim/test.js's --changed, --near and --list (the tiers of checks:
// reborn/tests/TESTING.md).  The files changed since a git REF (its merge base with HEAD: what's committed since, what's
// staged and not, and the new files git doesn't ignore) each mapped to areas by a system's table of paths (its
// tests/areas.js: the first of its rules that matches), and the tests that depend on those areas the ones run.  Erring
// on the side of running more: a path no rule names runs every test.
//
// A rule: [pattern, areas]: pattern a path's start (from the repository's top, '/' between folders: a folder's ends in
// '/') or a RegExp; areas, what a change there affects, each of them
//   '*'          every test
//   'none'       no test (the design documents, the books)
//   'entries'    a tests file (tests.js): the tests whose entries the diff touches, and those whose entries use a
//                helper of the file's it touches (a top-level declaration: theirs by the names they use, at any depth);
//                a line it touches outside both (the file's require lines, its exports) runs every test
//   'word:NAME'  the tests whose entries (with their helpers) name NAME (a whole word): a test module, a tests/ram
//                program, a helper module's own function.  None of them: every test (a name no test has, unknown)
//   'dl'         the danlang emulator's: every test with --dl, else none
//   AREA         the tests whose areas (the system's areasOf(t, text)) have AREA
// A RegExp rule's areas can have $1, $2 ...: its groups (word:$1, mod:$1).
//
// From Node:
//   changedFiles(top, ref)              { ref, base, files }: base the merge base's commit, files the paths changed
//   parseTests(file, tests)             a tests file's entries: { items, texts (name -> its entry's text and its
//                                       helpers'), lineOf(n) -> { item | decls | global | none } }, or null
//   select(tests, ctx)                  { run: Map(name -> [why]), changed: [{ file, areas }] }; ctx: { files, rules,
//                                       areasOf, texts, entries(file) -> { names, all } }
//   entriesChanged(top, base, file, parsed)   a tests file's diff by entry: { names: Set, all: why or null }
'use strict';
const fs = require('fs');
const path = require('path');
const cp = require('child_process');

const git = (top, args) => cp.execFileSync('git', args, { cwd: top, encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'], maxBuffer: 64 << 20 });
const topOf = dir => git(dir, ['rev-parse', '--show-toplevel']).trim();

// The files changed since ref's merge base with HEAD: committed since, staged, unstaged, new (untracked, not ignored).
// OUT: { ref, base, files } (files: paths from the top, sorted)
function changedFiles(top, ref) {
  let base;
  try { base = git(top, ['merge-base', ref, 'HEAD']).trim(); }
  catch (e) { throw new Error('--changed: no merge base of ' + ref + ' and HEAD (git merge-base ' + ref + ' HEAD)'); }
  const files = new Set(git(top, ['diff', '--name-only', '--no-renames', base]).split('\n').map(s => s.trim()).filter(Boolean));
  for (const f of git(top, ['ls-files', '--others', '--exclude-standard', '--full-name']).split('\n').map(s => s.trim()).filter(Boolean)) files.add(f);
  return { ref, base, files: [...files].sort() };
}

// The areas a path's change affects: the first rule that matches.  OUT: { areas, rule } (no rule: ['*'], null)
function areasOfPath(rules, file) {
  for (const r of rules) {
    const [p, areas] = r;
    if (typeof p === 'string') { if (file === p || file.startsWith(p)) return { areas, rule: r }; continue; }
    const m = file.match(p);
    if (m) return { areas: areas.map(a => a.replace(/\$(\d)/g, (_, k) => m[+k] || '')), rule: r };
  }
  return { areas: ['*'], rule: null };
}

const IDENT = /[A-Za-z_$][\w$]*/g;
const idents = s => new Set(s.match(IDENT) || []);
const isComment = l => /^\s*(\/\/.*)?$/.test(l);

// A tests file's entries (tests.js: module.exports = { ..., tests: [ an entry an item, 4 spaces in ], ... }; a comment
// just before an item its own), its helpers (its top-level declarations before the exports: const, let, function,
// class; a comment just before one its own) and the rest (the exports' other lines: what every test has).  tests: the
// list the file makes, in its order (an item's names: its name: '...', or, made by code, those between the named items
// around it).  OUT: null (not that shape), or { lines, items: [{ from, to, names }], decls: [{ from, to, names, uses }],
// texts: Map(name -> its entry's text and its helpers', at any depth), lineOf(n) (1-based) -> { item } | { decl } |
// { global: true } | { none: true } (a comment, a blank), global: the helpers every test has }
function parseTests(file, tests) {
  let src;
  try { src = fs.readFileSync(file, 'utf8'); } catch (e) { return null; }
  const lines = src.split(/\r?\n/);
  const ex = lines.findIndex(l => /^module\.exports = \{/.test(l));
  const s = lines.findIndex((l, i) => i > ex && /^  tests: \[/.test(l));
  if (ex < 0 || s < 0) return null;
  let e = s + 1;
  while (e < lines.length && !/^  \],?\s*$/.test(lines[e])) e++;
  if (e >= lines.length) return null;
  // The items: each from its comment's first line to the line before the next's
  const starts = [];
  for (let i = s + 1; i < e; i++) if (/^    \S/.test(lines[i]) && !/^    \/\//.test(lines[i]) && !/^    [\])}][\])},;\s]*$/.test(lines[i])) {
    let f = i;
    while (f - 1 > s && /^\s*\/\//.test(lines[f - 1]) && !(starts.length && f - 1 <= starts[starts.length - 1].at)) f--;
    starts.push({ at: i, from: f });
  }
  const items = starts.map((st, k) => {
    const to = k + 1 < starts.length ? starts[k + 1].from - 1 : e - 1;
    const text = lines.slice(st.from, to + 1).join('\n'), m = text.match(/^ {4}(?:\},\s*)?\{ ?name: '([^']+)'(?=\s*,)|^ {6}name: '([^']+)'(?=\s*,)/m);
    return { from: st.from, to, text, literal: m ? m[1] || m[2] : null, names: [] };
  });
  // Their names: a named item its own; those the code makes, the tests between the named ones around them
  const order = tests.map(t => t.name);
  let at = 0, pending = [];
  for (const it of items) {
    if (!it.literal) { pending.push(it); continue; }
    const k = order.indexOf(it.literal, at);
    if (k < 0) { it.names = [it.literal]; continue; }
    const made = order.slice(at, k);
    for (const p of pending) p.names = made;
    pending = [];
    it.names = [it.literal];
    at = k + 1;
  }
  for (const p of pending) p.names = order.slice(at);
  // The helpers: the top-level declarations before the exports
  const decls = [];
  for (let i = 0; i < ex; i++) {
    const d = lines[i].match(/^(?:const|let|var|async function|function|class)\s+(?:\{([^}]*)\}|([A-Za-z_$][\w$]*))/);
    if (!d) continue;
    let f = i;
    while (f - 1 >= 0 && /^\/\//.test(lines[f - 1])) f--;
    const names = d[1] ? d[1].split(',').map(x => x.split(':').pop().trim()).filter(Boolean) : [d[2]];
    if (!d[1] && /^(const|let|var) /.test(lines[i])) {          // (const A = 1, B = 2: the commas outside brackets)
      let depth = 0, q = null;
      for (let k = 0; k < lines[i].length; k++) {
        const ch = lines[i][k];
        if (q) { if (ch === '\\') k++; else if (ch === q) q = null; continue; }
        if (ch === "'" || ch === '"' || ch === '`') q = ch;
        else if ('([{'.includes(ch)) depth++;
        else if (')]}'.includes(ch)) depth--;
        else if (ch === ',' && depth === 0) { const m = lines[i].slice(k + 1).match(/^\s*([A-Za-z_$][\w$]*)\s*=(?![=>])/); if (m) names.push(m[1]); }
      }
    }
    if (decls.length) decls[decls.length - 1].to = f - 1;
    decls.push({ from: f, to: ex - 1, start: i, names });
  }
  const byName = new Map();
  for (const d of decls) { d.text = lines.slice(d.from, d.to + 1).join('\n'); for (const n of d.names) byName.set(n, d); }
  for (const d of decls) d.uses = [...idents(d.text)].map(n => byName.get(n)).filter(x => x && x !== d);
  const closure = text => {
    const seen = new Set(), todo = [...idents(text)].map(n => byName.get(n)).filter(Boolean);
    while (todo.length) { const d = todo.pop(); if (seen.has(d)) continue; seen.add(d); todo.push(...d.uses); }
    return seen;
  };
  // What every test has: the exports' lines but the items (IRQ_OFF_MAX, dq0Check, the sdcard's)
  const inItem = new Array(lines.length).fill(null);
  for (const it of items) for (let i = it.from; i <= it.to; i++) inItem[i] = it;
  const rest = lines.filter((l, i) => i >= ex && !inItem[i] && !isComment(l)).join('\n');
  const global = closure(rest);
  const texts = new Map();
  for (const it of items) {
    it.helpers = closure(it.text);
    const t = it.text + '\n' + [...it.helpers].map(d => d.text).join('\n');
    for (const n of it.names) texts.set(n, (texts.get(n) || '') + t);
  }
  const inDecl = new Array(lines.length).fill(null);
  for (const d of decls) for (let i = d.from; i <= d.to; i++) inDecl[i] = d;
  const lineOf = n => {
    const i = n - 1;
    if (i < 0 || i >= lines.length) return { global: true };
    if (inItem[i]) return { item: inItem[i] };
    if (isComment(lines[i])) return { none: true };            // (A comment, a blank: no test's; in an item, the item's)
    const stray = /^[^\s}\])]/.test(lines[i]) && !(inDecl[i] && inDecl[i].start === i);   // (A top-level statement)
    if (inDecl[i] && !stray) return global.has(inDecl[i]) ? { global: true } : { decl: inDecl[i] };
    return { global: true };
  };
  return { lines, items, decls, texts, global, lineOf };
}

// A tests file's diff since base (git diff -U0) by its entries (parsed: parseTests's, the file as it is now).
// OUT: { names: Set (the tests whose entries, or whose helpers, it touches), all: why every test, or null }
function entriesChanged(top, base, rel, parsed) {
  if (!parsed) return { names: new Set(), all: 'not read as entries' };
  let diff;
  try { diff = git(top, ['diff', '-U0', '--no-renames', base, '--', rel]); } catch (e) { return { names: new Set(), all: 'no diff' }; }
  if (!diff.trim()) {                                           // (New: every test)
    const tracked = git(top, ['ls-files', '--', rel]).trim();
    return tracked ? { names: new Set(), all: null } : { names: new Set(), all: 'a new file' };
  }
  const touched = [];
  for (const h of diff.matchAll(/^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@/gm)) {
    const c = +h[1], d = h[2] === undefined ? 1 : +h[2];
    touched.push(...(d ? Array.from({ length: d }, (_, k) => c + k) : [c, c + 1]));   // (Lines taken out: those around)
  }
  return namesForLines(parsed, touched);
}

// The tests a tests file's lines (1-based, the file as it is now) are part of: their entries', and those whose
// entries use the helpers they're in.  OUT: { names: Set, all: why every test, or null }
function namesForLines(parsed, touched) {
  const names = new Set(), decls = new Set();
  let all = null;
  for (const n of touched) {
    const w = parsed.lineOf(n);
    if (w.item) w.item.names.forEach(x => names.add(x));
    else if (w.decl) decls.add(w.decl);
    else if (w.global && !all) all = 'line ' + n + ', every test\'s';
  }
  for (const it of parsed.items) if ([...decls].some(d => it.helpers.has(d))) it.names.forEach(x => names.add(x));
  return { names, all };
}

const wordRe = w => new RegExp('(^|[^\\w$])' + w.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + '(?![\\w$])');

// The tests to run for the changed files.  ctx: { files, rules, areasOf(t) -> Set, texts: Map (name -> text; null:
// unknown), entries(file) -> entriesChanged's }.  OUT: { run: Map(name -> [why ...]), changed: [{ file, areas, why }] }
function select(tests, ctx) {
  const run = new Map(), changed = [];
  const add = (t, why) => { const w = run.get(t.name) || []; if (!w.includes(why)) w.push(why); run.set(t.name, w); };
  const areas = new Map(tests.map(t => [t.name, ctx.areasOf(t)]));
  for (const file of ctx.files) {
    const { areas: as } = areasOfPath(ctx.rules, file), said = [];
    for (const a of as) {
      if (a === 'none' || (a === 'dl' && !ctx.dl)) { said.push(a === 'dl' ? 'none (but with --dl)' : 'none'); continue; }
      if (a === '*' || a === 'dl') { tests.forEach(t => add(t, file)); said.push('every test'); continue; }
      if (a === 'entries') {
        const e = ctx.entries(file);
        if (e.all) { tests.forEach(t => add(t, file + ' (' + e.all + ')')); said.push('every test (' + e.all + ')'); continue; }
        const hit = tests.filter(t => e.names.has(t.name));
        hit.forEach(t => add(t, file + ': its entry, or a helper of it'));
        said.push('entries: ' + (hit.length ? hit.map(t => t.name).join(' ') : 'none'));
        continue;
      }
      if (a.startsWith('word:')) {
        const w = a.slice(5), re = wordRe(w);
        const hit = ctx.texts ? tests.filter(t => ctx.texts.has(t.name) && re.test(ctx.texts.get(t.name))) : null;
        if (!hit || !hit.length) { tests.forEach(t => add(t, file + ' (' + w + ': no test names it)')); said.push(w + ': no test names it, every test'); continue; }
        hit.forEach(t => add(t, file + ' (' + w + ')'));
        said.push(w + ': ' + hit.length + ' test(s)');
        continue;
      }
      const hit = tests.filter(t => areas.get(t.name).has(a));
      hit.forEach(t => add(t, file + ' (' + a + ')'));
      said.push(a + ': ' + hit.length + ' test(s)');
    }
    changed.push({ file, areas: as, said });
  }
  return { run, changed };
}

// The tests files' entries read (files: { path from the top: the tests it makes, in order }).  OUT: { parsed: { path:
// parseTests's }, texts: Map (name -> text), ok: every file read }
function testTexts(top, files) {
  const parsed = {}, texts = new Map();
  for (const [rel, list] of Object.entries(files)) {
    const p = parseTests(path.join(top, rel), list);
    parsed[rel] = p;
    if (p) for (const [k, v] of p.texts) texts.set(k, v);
  }
  return { parsed, texts, ok: Object.values(parsed).every(Boolean) };
}

// A test's areas, the system's (areas: { rules, areasOf(t, text) }), its text the entry's (null: not read)
const areasFn = (areas, tt) => t => areas.areasOf(t, tt.ok && tt.texts.has(t.name) ? tt.texts.get(t.name) : null);

// What changed since ref, and the tests it affects.  o: { top, ref, tests, files (testTexts's), areas, dl }.  OUT:
// { ref, base, files, run, changed } (changedFiles's and select's)
function forChanges(o) {
  const ch = changedFiles(o.top, o.ref), tt = testTexts(o.top, o.files);
  const sel = select(o.tests, { files: ch.files, rules: o.areas.rules, areasOf: areasFn(o.areas, tt), texts: tt.ok ? tt.texts : null,
    dl: o.dl, entries: f => entriesChanged(o.top, ch.base, f, tt.parsed[f]) });
  return Object.assign(ch, sel);
}

// The tests of the areas named (or the tests named), for --near.  OUT: { list, unknown: the words that named none }
function near(o, words) {
  const tt = testTexts(o.top, o.files), of = areasFn(o.areas, tt), unknown = [];
  const sets = new Map(o.tests.map(t => [t.name, of(t)]));
  const list = o.tests.filter(t => words.some(w => t.name === w || sets.get(t.name).has(w)));
  for (const w of words) if (!o.tests.some(t => t.name === w || sets.get(t.name).has(w))) unknown.push(w);
  return { list, unknown };
}

module.exports = { topOf, changedFiles, areasOfPath, parseTests, entriesChanged, namesForLines, select, wordRe, testTexts, areasFn, forChanges, near };
