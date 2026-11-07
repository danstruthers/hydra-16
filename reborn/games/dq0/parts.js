// parts.js - a Dingus Quest Zero task's swapped parts (swap.inc's) made by the build, not typed twice
// (ENGINE-CRITIQUE.md's step 2): a task's parts.txt says each part's overlay bank (where its image is kept) and what
// it's for; the C says which part a function is in (#pragma code-name "SWnCODE"), and the build (build.js's cprog,
// for a folder with a parts.txt) makes the rest from what the compiler made of it:
//   - the stubs (swap.inc's SWFN): a function s_NAME defined in a part's code and NAME used anywhere (the C's own
//     output, the folder's assembly): that one's stub, NAME; a stub of a part's assembly or a library's (not s_ C) is
//     a stub line of parts.txt.  A function moved from part to part is its pragma alone; a stub no one calls isn't
//     made (each costs the main RAM);
//   - the parts' records (swap.inc's SWPART: a part, its bank) and the stubs, OBJDIR/parts.inc, which the task's
//     bank.s .includes where its SWPART and SWFN lines were;
//   - the link's config, OBJDIR/NAME.cfg: the task's own NAME.cfg with its #@SWMEMORY and #@SWSEGMENTS lines made the
//     parts' (each part's run area SWn, its code and read-only data loaded into its overlay bank, run there); a line
//     #@SW n puts part n's segments there instead, its image's place among its bank's others (the UI's dq0.cfg).
// parts.txt: a line a part, "N OV what it's for", in order from 1; "stub NAME N" a stub the C doesn't show; # begins a
// comment.
'use strict';
const fs = require('fs');
const path = require('path');

// A task's parts.txt: { parts: [{ n, ov, what }], extra: [{ name, part }] }
function read(file) {
  const parts = [], extra = [];
  fs.readFileSync(file, 'latin1').split(/\r?\n/).forEach((l, i) => {
    const t = l.replace(/#.*/, '').trim();
    let m;
    if (!t) return;
    if ((m = /^stub\s+(\w+)\s+(\d+)$/.exec(t))) extra.push({ name: m[1], part: +m[2] });
    else if ((m = /^(\d+)\s+(\d+)(?:\s+(.*))?$/.exec(t))) parts.push({ n: +m[1], ov: +m[2], what: m[3] || '' });
    else throw new Error(file + ' line ' + (i + 1) + ': not a part ("N OV what") nor a stub ("stub NAME N"): ' + l);
  });
  const seen = new Set();
  parts.forEach((p, i) => {                             // (swap.inc's records: in order from 1, an index each)
    if (p.n !== i + 1) throw new Error(file + ': part ' + p.n + ' where part ' + (i + 1) + ' should be (in order from 1, none left out)');
    seen.add(p.n);
  });
  for (const s of extra) if (!seen.has(s.part)) throw new Error(file + ': stub ' + s.name + '\'s part ' + s.part + ' not a part of its');
  return { parts, extra };
}

// The stubs wanted: compiled are the C's assembly (OBJDIR's .s of each .c), own the folder's own assembly (less
// bank.s); each s_NAME a part's code defines, if NAME's used anywhere; then parts.txt's own.  [{ name, part }]
function wanted(compiled, own, extra) {
  const defined = new Map(), used = new Set();
  for (const f of compiled) {
    let seg = 'CODE';
    for (const l of fs.readFileSync(f, 'latin1').split(/\r?\n/)) {
      let m;
      if ((m = /^\s*\.segment\s+"(\w+)"/.exec(l))) {
        seg = m[1];
      } else if ((m = /^\.proc\s+_s_(\w+)/.exec(l))) {
        const p = /^SW(\d+)CODE$/.exec(seg);
        if (p && !defined.has(m[1])) defined.set(m[1], +p[1]);
      }
      for (const [, n] of l.replace(/;.*/, '').matchAll(/\b_(\w+)\b/g)) used.add(n);
    }
  }
  for (const f of own) for (const l of fs.readFileSync(f, 'latin1').split(/\r?\n/))
    for (const [, n] of l.replace(/;.*/, '').matchAll(/\b_(\w+)\b/g)) used.add(n);
  const out = [];
  for (const [name, part] of defined) if (used.has(name)) out.push({ name, part });
  for (const s of extra) if (!out.some(o => o.name === s.name)) out.push(s);
  return out.sort((a, b) => a.part - b.part || a.name.localeCompare(b.name));
}

// parts.inc's text: the parts' records, then the stubs
function inc(task, list, stubs) {
  const w = Math.max(...stubs.map(s => s.name.length), 8);
  return '; parts.inc - ' + task + '\'s swapped parts and their stubs, made by the build from its parts.txt and its C\'s\n' +
    '; #pragma code-name "SWnCODE" (games/dq0/parts.js): not to be edited\n\n' +
    list.parts.map(p => '            SWPART      ' + (p.n + ', ' + p.ov).padEnd(9) + (p.what ? '; ' + p.what : '')).join('\n') + '\n\n' +
    stubs.map(s => '            SWFN        ' + s.name + ', ' + s.part).join('\n') + '\n';
}

// The link's config: the task's own, its #@SWMEMORY and #@SWSEGMENTS lines the parts'
function cfg(text, list, file) {
  const mem = list.parts.map(p => '    SW' + p.n + ':' + ' '.repeat(Math.max(1, 4 - String(p.n).length)) +
    'file = "", start = __SWAP_START__, size = __SWAPSIZE__, type = rw, define = yes;').join('\n');
  const segs = p => {
    const a = ('SW' + p.n + 'CODE:').padEnd(12), b = ('SW' + p.n + 'RODATA:').padEnd(12), ov = ('OV' + p.ov + ',').padEnd(6);
    return '    ' + a + 'load = ' + ov + 'run = SW' + p.n + ',  type = ro,  define = yes;' + (p.what ? '    # (' + p.what + ')' : '') + '\n' +
      '    ' + b + 'load = ' + ov + 'run = SW' + p.n + ',  type = ro,  optional = yes;';
  };
  const at = k => new RegExp('^[ \\t]*#@' + k + '[ \\t]*$', 'm');   // (On its own line: \s would take the line's
  const eol = /\r\n/.test(text) ? '\r\n' : '\n';                        //   end, ^ matching after a CR in a CRLF file)
  const placed = new Set();                             // (#@SW n: part n's segments there, its image's place among
  text = text.replace(/^[ \t]*#@SW(\d+)[ \t]*$/gm, (m, n) => {   //   its bank's others; the rest at #@SWSEGMENTS)
    const p = list.parts.find(q => q.n === +n);
    if (!p) throw new Error(file + ': #@SW' + n + ', not a part of parts.txt\'s');
    placed.add(p.n);
    return segs(p).replace(/\n/g, eol);
  });
  const rest = list.parts.filter(p => !placed.has(p.n));
  if (!at('SWMEMORY').test(text) || (rest.length && !at('SWSEGMENTS').test(text)))
    throw new Error(file + ': no #@SWMEMORY or #@SWSEGMENTS line (where the parts\' areas and segments go: games/dq0/parts.js)');
  return text.replace(at('SWMEMORY'), mem.replace(/\n/g, eol)).replace(at('SWSEGMENTS'), rest.map(segs).join('\n').replace(/\n/g, eol));
}

// The build's: a task with a parts.txt, its C compiled (compiled: OBJDIR's .s of each .c), its parts.inc and its
// config made in objdir.  OUT: the config's path
function make({ dir, objdir, compiled, own }) {
  const task = path.basename(dir), list = read(path.join(dir, 'parts.txt'));
  const stubs = wanted(compiled, own.filter(f => path.basename(f) !== 'bank.s'), list.extra);
  for (const s of stubs) if (!list.parts.some(p => p.n === s.part))
    throw new Error(task + ': ' + s.name + ' is in part ' + s.part + ', which parts.txt doesn\'t have');
  fs.writeFileSync(path.join(objdir, 'parts.inc'), inc(task, list, stubs));
  const src = path.join(dir, task + '.cfg'), out = path.join(objdir, task + '.cfg');
  fs.writeFileSync(out, cfg(fs.readFileSync(src, 'latin1'), list, src));
  return out;
}

module.exports = { read, wanted, inc, cfg, make };
