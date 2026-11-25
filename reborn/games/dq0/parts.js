// parts.js - a Dingus Quest Zero task's swapped parts (swap.inc's) made by the build, not typed twice
// (ENGINE-CRITIQUE.md's step 2; names, not numbers, names1, 2026-11-24): a task's parts.txt names each part, says
// where it wishes its image kept (a bank, a group of banks, or the packer's choice) and what it's for; the C says
// which part a function is in by the part's name (#pragma code-name "SW_name_CODE", rodata-name "SW_name_RODATA"),
// and the build (build.js's cprog, for a folder with a parts.txt) makes the rest from what the compiler made of it:
//   - the parts' numbers: their order in parts.txt (swap.inc's records' index, a stub's part), the build's alone:
//     nothing typed names one, so a part added on two branches merges with none renumbered (.gitattributes's union
//     merge keeps both lines);
//   - the stubs (swap.inc's SWFN): a function s_NAME defined in a part's code and NAME used anywhere (the C's own
//     output, the folder's assembly): that one's stub, NAME; a stub of a part's assembly or a library's (not s_ C) is
//     a stub line of parts.txt.  A function moved from part to part is its pragma alone; a stub no one calls isn't
//     made (each costs the main RAM);
//   - the parts' records (swap.inc's SWPART: a part, its bank) and the stubs, OBJDIR/parts.inc, which the task's
//     bank.s .includes; the parts' names in their order, OBJDIR/parts.json (the emulator's tools': a part's number,
//     swap.inc's swapped, to its name);
//   - the link's config, OBJDIR/NAME.cfg: the task's own NAME.cfg with its #@SWMEMORY and #@SWSEGMENTS lines made the
//     parts' (each part's run area SW_name, its code and read-only data loaded into its bank, run there); a line
//     "#@SW name" puts that part's segments there instead, its image's place among its bank's others (the UI's
//     dq0.cfg).
// parts.txt: a line a part, "NAME WISH what it's for" (NAME: a-z, 0-9 and _, begun with a letter, the task's own);
// WISH the bank its image is wished in: "-" none (the packer's choice among the first file's banks), N the first
// file's Nth (OVN, as that file comes first), GROUP any of a group's (a file's banks: its name's extension, "rop"
// for dq0w.rop), GROUP:N that group's Nth (a group's banks numbered from 1 in it, so a bank added to another file
// renumbers none); a bank too full, the packer puts the part in another of its file's.  "stub NAME PART" a part's
// stub the C doesn't show; "spare N" each bank's room kept free, if its parts allow (without, SPARE: 64, the
// headroom rule's); # begins a comment.
'use strict';
const fs = require('fs');
const path = require('path');

const NAME = /^[a-z][a-z0-9_]*$/;
const seg = (name, k) => 'SW_' + name + '_' + k;      // (A part's segments, k CODE or RODATA; its run area SW_name)
const SEG = /^SW_(\w+)_(CODE|RODATA)$/;               // (A part's segment: its name, its kind)

// A task's parts.txt: { parts: [{ n, name, wish, what }], extra: [{ name, part }], spare: bytes, or undefined, num:
// Map name → n }; n each part's number, its place among the file's parts, from 1
function read(file) {
  const parts = [], extra = [], num = new Map();
  let spare;
  fs.readFileSync(file, 'latin1').split(/\r?\n/).forEach((l, i) => {
    const t = l.replace(/#.*/, '').trim(), at = file + ' line ' + (i + 1) + ': ';
    let m;
    if (!t) return;
    if ((m = /^stub\s+(\w+)\s+(\w+)$/.exec(t))) extra.push({ name: m[1], part: m[2], at });
    else if ((m = /^spare\s+(\d+)$/.exec(t))) spare = +m[1];
    else if ((m = /^(\w+)\s+(-|\d+|[a-z]+(?::\d+)?)(?:\s+(.*))?$/.exec(t))) {
      if (!NAME.test(m[1])) throw new Error(at + 'a part\'s name, ' + m[1] + ': a-z, 0-9 and _, begun with a letter');
      if (num.has(m[1])) throw new Error(at + 'part ' + m[1] + ' twice (a merge\'s both sides changed its line? keep one)');
      num.set(m[1], parts.length + 1);
      parts.push({ n: parts.length + 1, name: m[1], wish: m[2], what: m[3] || '' });
    } else throw new Error(at + 'not a part ("NAME WISH what"), a stub ("stub NAME PART") nor a spare ("spare N"): ' + l);
  });
  for (const s of extra) {
    if (!num.has(s.part)) throw new Error(s.at + 'stub ' + s.name + '\'s part, ' + s.part + ', not a part of parts.txt\'s');
    s.part = num.get(s.part);
    delete s.at;
  }
  return { parts, extra, spare, num };
}

// A config's banks (its MEMORY's OVn): Map n → { file, group (its file's name's extension), k (its place in its
// group, from 1) }, and the first file's group (a wish of "-" or a number alone)
function groups(text) {
  const mem = (text.split(/^MEMORY\s*\{/m)[1] || '').split('}')[0], banks = new Map(), count = new Map();
  let first;
  for (const [, n, f] of mem.replace(/#.*/g, '').matchAll(/^\s*OV(\d+):\s*file\s*=\s*"([^"]*)"/gm)) {
    const g = (/\.(\w+)$/.exec(f) || [, f])[1];
    count.set(g, (count.get(g) || 0) + 1);
    banks.set(+n, { file: f, group: g, k: count.get(g) });
    if (first === undefined) first = g;
  }
  return { banks, first };
}

// The parts' wishes made banks by the config (text): each part's ov (its wished bank's number, OVn; null, none: the
// packer's choice) and file (its bank's file, the one it must be in).  A part with its ov already (a test's) as it is.
// Throws at a wish no bank of the config's answers
function wished(text, list, file) {
  const { banks, first } = groups(text), names = () => [...new Set([...banks.values()].map(x => x.group))].join(', ');
  return { ...list, parts: list.parts.map(p => {
    if (p.ov !== undefined) return p;
    const w = p.wish, m = /^(?:([a-z]+):)?(\d+)$/.exec(w), bad = why => {
      throw new Error(file + ': part ' + p.name + '\'s wish, ' + w + ': ' + why + ' (games/dq0/parts.js: "-", N, GROUP or GROUP:N)'); };
    if (w === '-') return { ...p, ov: null, file: [...banks.values()].find(b => b.group === first).file };
    if (m) {
      const b = [...banks].find(([, x]) => x.group === (m[1] || first) && x.k === +m[2]);
      if (!b) bad('no bank ' + m[2] + ' in the group "' + (m[1] || first) + '" (the config\'s groups: ' + names() + ')');
      return { ...p, ov: b[0], file: b[1].file };
    }
    const b = [...banks.values()].find(x => x.group === w);
    if (!b) bad('no group "' + w + '" (the config\'s groups: ' + names() + ')');
    return { ...p, ov: null, file: b.file };
  }) };
}

// The stubs wanted: compiled are the C's assembly (OBJDIR's .s of each .c), own the folder's own assembly (less
// bank.s); each s_NAME a part's code defines, if NAME's used anywhere; then parts.txt's own.  num: read()'s (a part's
// name → its number).  [{ name, part }].  Throws at a part's segment (SW_name_CODE, SW_name_RODATA) whose name isn't
// a part of parts.txt's: a pragma's name mistyped, or its part's line gone (the new scheme's check: ld65 would say
// only that the segment has no place)
function wanted(compiled, own, extra, num) {
  const defined = new Map(), used = new Set(), unknown = new Set();
  const partOf = (s, f) => {
    const m = SEG.exec(s);
    if (m && !num.has(m[1])) unknown.add(s + ' (' + path.basename(f) + ')');
    return m && m[2] === 'CODE' ? num.get(m[1]) : 0;
  };
  for (const f of compiled) {
    let seg = 'CODE';
    for (const l of fs.readFileSync(f, 'latin1').split(/\r?\n/)) {
      let m;
      if ((m = /^\s*\.segment\s+"(\w+)"/.exec(l))) {
        seg = m[1];
        partOf(seg, f);
      } else if ((m = /^\.proc\s+_s_(\w+)/.exec(l))) {
        const p = partOf(seg, f);
        if (p && !defined.has(m[1])) defined.set(m[1], p);
      }
      if (/^\s*\.(import|dbg)\b/.test(l)) continue;     // (The compiler's imports of all it names, not its uses)
      for (const [, n] of l.replace(/;.*/, '').matchAll(/\b_(\w+)\b/g)) used.add(n);
    }
  }
  for (const f of own) for (const l of fs.readFileSync(f, 'latin1').split(/\r?\n/)) {
    const s = l.replace(/;.*/, '');
    for (const [, n] of s.matchAll(/\b_(\w+)\b/g)) used.add(n);
    for (const [, n] of s.matchAll(/"(SW_\w+)"/g)) partOf(n, f);
  }
  if (unknown.size) throw new Error('a part\'s segment that names no part of parts.txt\'s (its pragma\'s name mistyped, or its line gone): ' +
    [...unknown].join(', '));
  const out = [];
  for (const [name, part] of defined) if (used.has(name)) out.push({ name, part });
  for (const s of extra) if (!out.some(o => o.name === s.name)) out.push(s);
  return out.sort((a, b) => a.part - b.part || a.name.localeCompare(b.name));
}

// The overlays' stubs wanted (bank.s's OVFN, a task whose bank.s has the macro): each o_NAME defined in an overlay's
// code (a segment the config loads into overlay j's bank, OVj or the rest of it, OVjT: overlay j's stub), if NAME's
// used anywhere (the C's output, the folder's assembly, bank.s's but its stubs); in the order of the overlays, then of
// the names.  text: the task's config; compiled, own: wanted()'s (own with bank.s).  [{ name, ov }].  Throws at an
// o_NAME defined twice, or one whose segment the config doesn't load into an overlay
function ovstubs(text, compiled, own) {
  const block = k => (text.split(new RegExp('^' + k + '\\s*\\{', 'm'))[1] || '').split('}')[0];
  const loads = new Map();                               // (A segment's overlay: its load area's number)
  for (const [, s, j] of block('SEGMENTS').replace(/#.*/g, '').matchAll(/^\s*(\w+):\s*load\s*=\s*OV(\d+)[A-Z]*\s*[,;]/gm)) loads.set(s, +j);
  const defined = new Map(), used = new Set(), faults = [];
  const def = (name, s, f) => {
    if (!/^OV\d+/.test(s)) return;
    if (!loads.has(s)) faults.push('o_' + name + ' (' + path.basename(f) + ') in ' + s + ', which the config loads into no overlay');
    else if (defined.has(name) && defined.get(name) !== loads.get(s)) faults.push('o_' + name + ' twice');
    else defined.set(name, loads.get(s));
  };
  for (const f of compiled) {
    let s = 'CODE';
    for (const l of fs.readFileSync(f, 'latin1').split(/\r?\n/)) {
      let m;
      if ((m = /^\s*\.segment\s+"(\w+)"/.exec(l))) s = m[1];
      else if ((m = /^\.proc\s+_o_(\w+)/.exec(l))) def(m[1], s, f);
      if (/^\s*\.(import|dbg)\b/.test(l)) continue;
      for (const [, n] of l.replace(/;.*/, '').matchAll(/\b_(\w+)\b/g)) used.add(n);
    }
  }
  for (const f of own) {
    let s = 'CODE';
    for (const l of fs.readFileSync(f, 'latin1').split(/\r?\n/)) {
      const c = l.replace(/;.*/, '');
      let m;
      if (/^\s*(OVFN|SWFN)\b/.test(c)) continue;         // (Stubs, not uses)
      if ((m = /^\s*\.segment\s+"(\w+)"/.exec(c))) s = m[1];
      else if (/^\s*\.code\b/.test(c)) s = 'CODE';
      else if ((m = /^_o_(\w+)\s*:/.exec(c))) def(m[1], s, f);
      for (const [, n] of c.matchAll(/\b_(\w+)\b/g)) used.add(n);
    }
  }
  if (faults.length) throw new Error('the overlays\' stubs (games/dq0/parts.js\'s ovstubs): ' + faults.join('; '));
  return [...defined].filter(([n]) => used.has(n)).map(([name, ov]) => ({ name, ov })).sort((a, b) => a.ov - b.ov || (a.name < b.name ? -1 : 1));
}

// ovfn.inc's text: the overlays' stubs, a line each
function ovinc(task, stubs) {
  return '; ovfn.inc - ' + task + '\'s overlays\' stubs, made by the build from its C\'s o_NAME functions, each in its overlay\n' +
    '; (#pragma code-name "OVnCODE", the config\'s bank: games/dq0/parts.js\'s ovstubs): not to be edited\n\n' +
    stubs.map(s => '            OVFN        ' + s.name + ', ' + s.ov).join('\n') + '\n';
}

// parts.inc's text: the parts' records (each by its name; its number its place), then the stubs (each its part's
// number, by its name in a comment)
function inc(task, list, stubs) {
  const w = Math.max(...list.parts.map(p => p.name.length + String(p.ov).length + 2), 9);
  return '; parts.inc - ' + task + '\'s swapped parts and their stubs, made by the build from its parts.txt and its C\'s\n' +
    '; #pragma code-name "SW_name_CODE" (games/dq0/parts.js): not to be edited.  A part\'s number is its place here\n\n' +
    list.parts.map(p => '            SWPART      ' + (p.name + ', ' + p.ov).padEnd(w) + ' ; ' + p.n + (p.what ? ': ' + p.what : '')).join('\n') + '\n\n' +
    stubs.map(s => '            SWFN        ' + (s.name + ', ' + s.part).padEnd(28) + ' ; ' + list.parts[s.part - 1].name).join('\n') + '\n';
}

// The link's config: the task's own, its #@SWMEMORY and #@SWSEGMENTS lines the parts'
function cfg(text, list, file) {
  const w = Math.max(...list.parts.map(p => p.name.length)) + 3;
  const mem = list.parts.map(p => '    ' + ('SW_' + p.name + ':').padEnd(w + 2) +
    'file = "", start = __SWAP_START__, size = __SWAPSIZE__, type = rw, define = yes;').join('\n');
  const segs = p => {
    const a = (seg(p.name, 'CODE') + ':').padEnd(w + 9), b = (seg(p.name, 'RODATA') + ':').padEnd(w + 9), ov = ('OV' + p.ov + ',').padEnd(6);
    return '    ' + a + 'load = ' + ov + 'run = SW_' + p.name + ',  type = ro,  define = yes;' + (p.what ? '    # (' + p.n + ': ' + p.what + ')' : '') + '\n' +
      '    ' + b + 'load = ' + ov + 'run = SW_' + p.name + ',  type = ro,  optional = yes;';
  };
  const at = k => new RegExp('^[ \\t]*#@' + k + '[ \\t]*$', 'm');   // (On its own line: \s would take the line's
  const eol = /\r\n/.test(text) ? '\r\n' : '\n';                        //   end, ^ matching after a CR in a CRLF file)
  if (/^[ \t]*#@SW\d/m.test(text)) throw new Error(file + ': a #@SW n line, a part by its number: #@SW NAME now (games/dq0/parts.js)');
  const placed = new Set();                             // ("#@SW name": that part's segments there, its image's place
  text = text.replace(/^[ \t]*#@SW[ \t]+(\w+)[ \t]*$/gm, (m, n) => {   //   among its bank's others; the rest at
    const p = list.parts.find(q => q.name === n);                       //   #@SWSEGMENTS)
    if (!p) throw new Error(file + ': #@SW ' + n + ', not a part of parts.txt\'s');
    if (placed.has(p.n)) throw new Error(file + ': #@SW ' + n + ' twice');
    placed.add(p.n);
    return segs(p).replace(/\n/g, eol);
  });
  const rest = list.parts.filter(p => !placed.has(p.n));
  if (!at('SWMEMORY').test(text) || (rest.length && !at('SWSEGMENTS').test(text)))
    throw new Error(file + ': no #@SWMEMORY or #@SWSEGMENTS line (where the parts\' areas and segments go: games/dq0/parts.js)');
  return text.replace(at('SWMEMORY'), mem.replace(/\n/g, eol)).replace(at('SWSEGMENTS'), rest.map(segs).join('\n').replace(/\n/g, eol));
}
// The parts' banks, the build's (ENGINE-CRITIQUE.md's packer, its first half: a bank's room no one's to shuffle by
// hand): parts.txt's bank a part's wish; each bank's room what its own (an overlay's code and data: the config's
// non-SW segments loaded into it) leaves; a bank whose wishers don't fit it, the fewest bytes moved out (the smallest
// part enough to make room; or the biggest, and again); those, the biggest first, each put in the bank of its wish's
// file with the least room that holds it (the same file: one read later, dq0w.rop's, keeps its own); with none, room
// made in one (2026-11-07): a smaller part moved out of it to another bank with room for it, the smallest that's
// enough.  text: the task's config; sizes: Map segment → its bytes (all the objects').
// OUT: { at: Map part → its bank, moved: [{ n, from, to, size }], room: Map bank → its bytes left, file: Map bank → its
// file }; throws, a part no bank has room for.  A bank whose MEMORY line's comment says "no parts" is no part's (dq0's OV28,
// the phrase table's rows: their room kept for them, 2026-11-11)
function banks(text, list, sizes, file, spare = 0) {
  const block = k => (text.split(new RegExp('^' + k + '\\s*\\{', 'm'))[1] || '').split('}')[0];
  const cap = new Map(), fileOf = new Map(), room = new Map(), closed = new Set();
  for (const [l, n, f, z] of block('MEMORY').matchAll(/^\s*OV(\d+):\s*file\s*=\s*"([^"]*)"[^;\n]*?\bsize\s*=\s*\$([0-9A-Fa-f]+)[^\n]*/gm)) {
    cap.set(+n, parseInt(z, 16));
    fileOf.set(+n, f);
    if (/#.*\bno parts\b/.test(l)) closed.add(+n);
  }
  for (const [k, c] of cap) room.set(k, c);
  for (const [, s, k] of block('SEGMENTS').matchAll(/^\s*(\w+):\s*load\s*=\s*OV(\d+)\b/gm))
    if (!SEG.test(s) && room.has(+k)) room.set(+k, room.get(+k) - (sizes.get(s) || 0));
  for (const [, k] of block('SEGMENTS').matchAll(/^\s*SWTAB:\s*load\s*=\s*OV(\d+)\b/gm))   // (The parts' records, bank.s's: not in
    if (!sizes.has('SWTAB') && room.has(+k)) room.set(+k, room.get(+k) - 5 * list.parts.length);   //   the objects' sizes; 5 bytes a part, swap.inc's SWPART)
  const keep = new Map([...room].map(([k, x]) => [k, closed.has(k) ? 0 : Math.min(typeof spare === 'number' ? spare : spare.has(k) ? spare.get(k) : spareOf(list), Math.max(0, x))]));   // (spare:
  for (const [k, x] of keep) room.set(k, room.get(k) - x);                                        //   each bank's kept free)
  const base = new Map(room);                           // (Each bank's room before any part's in it)
  const size = p => (sizes.get(seg(p.name, 'CODE')) || 0) + (sizes.get(seg(p.name, 'RODATA')) || 0);
  const first = fileOf.get(Math.min(...cap.keys())), fileFor = p => p.ov !== null ? fileOf.get(p.ov) : p.file || first;   // (A part's file:
  const at = new Map(), left = list.parts.filter(p => p.ov === null), moved = [];                                        //   its bank's)
  for (const p of list.parts)                          // (A part with no bank wished: the packer's, as one moved out)
    if (p.ov !== null && !cap.has(p.ov)) throw new Error(file + ': part ' + p.name + '\'s bank, OV' + p.ov + ', not a bank of the config\'s');
  for (const k of cap.keys()) {                         // (A bank too full for its wishers: the least moved out that
    const in_ = list.parts.filter(p => p.ov === k);     //   makes room, the smallest that's enough, or the biggest and
    let over = in_.reduce((t, p) => t + size(p), 0) - room.get(k);   //   again)
    while (over > 0) {
      if (!in_.length) throw new Error(file + ': overlay ' + k + '\'s own code and data are ' + (-room.get(k)) +
        ' bytes past its bank (' + cap.get(k) + '): no part of its to move out (its code smaller, or some of it to another overlay)');
      const by = [...in_].sort((a, b) => size(a) - size(b) || b.n - a.n), p = by.find(q => size(q) >= over) || by[by.length - 1];
      in_.splice(in_.indexOf(p), 1);
      left.push(p);
      over -= size(p);
    }
    for (const p of in_) at.set(p.n, k);
    room.set(k, room.get(k) - in_.reduce((t, p) => t + size(p), 0));
  }
  left.sort((a, b) => size(b) - size(a) || a.n - b.n);
  const same = p => [...room].filter(([k]) => fileOf.get(k) === fileFor(p) && !closed.has(k));
  const put = (p, k) => {
    const was = moved.findIndex(m => m.n === p.n);
    if (was >= 0) moved.splice(was, 1);
    at.set(p.n, k);
    room.set(k, room.get(k) - size(p));
    if (p.ov !== null && k !== p.ov) moved.push({ n: p.n, from: p.ov, to: k, size: size(p) });
  };
  const redone = new Set();                            // (The files whose parts were all placed again)
  for (const p of left) {
    if (redone.has(fileFor(p))) continue;
    const fits = same(p).filter(([, r]) => r >= size(p)).sort((a, b) => a[1] - b[1] || a[0] - b[0]);
    if (fits.length) {
      put(p, fits[0][0]);
      continue;
    }
    let best = null;                                    // (No bank with room: one made, a smaller part moved out of it
    for (const [b, r] of same(p)) {                     //   to another with room for it, the smallest that's enough)
      for (const q of list.parts) {
        if (at.get(q.n) !== b || r + size(q) < size(p)) continue;
        const c = same(p).filter(([k, x]) => k !== b && x >= size(q)).sort((x, y) => x[1] - y[1] || x[0] - y[0])[0];
        if (c && (!best || size(q) < size(best.q) || (size(q) === size(best.q) && b < best.b))) best = { q, b, c: c[0] };
      }
    }
    if (!best) {                                        // (None: every part of the file placed again, the biggest first,
      const f = fileFor(p), mine = list.parts.filter(q => fileFor(q) === f);   //   each in the bank with the
      const free = new Map([...base].filter(([k]) => fileOf.get(k) === f && !closed.has(k)));   //   least room that holds it)
      const place = new Map();
      for (const q of [...mine].sort((a, b) => size(b) - size(a) || a.n - b.n)) {
        const c = [...free].filter(([, x]) => x >= size(q)).sort((x, y) => x[1] - y[1] || x[0] - y[0])[0];
        if (!c) throw new Error(file + ': part ' + p.name + ' (' + size(p) + ' bytes) fits no bank of "' + f + '", not even with its parts all ' +
          'placed again: ' + [...base].filter(([k]) => fileOf.get(k) === f).reduce((t, [, x]) => t + x, 0) + ' bytes for them, ' +
          mine.reduce((t, x) => t + size(x), 0) + ' wanted (a bank more, or the parts smaller)');
        place.set(q.n, c[0]);
        free.set(c[0], c[1] - size(q));
      }
      for (const q of mine) {
        const was = moved.findIndex(m => m.n === q.n);
        if (was >= 0) moved.splice(was, 1);
        at.set(q.n, place.get(q.n));
        if (q.ov !== null && place.get(q.n) !== q.ov) moved.push({ n: q.n, from: q.ov, to: place.get(q.n), size: size(q) });
      }
      for (const [k, x] of free) room.set(k, x);
      redone.add(f);
      continue;
    }
    room.set(best.b, room.get(best.b) + size(best.q));
    put(best.q, best.c);
    put(p, best.b);
  }
  for (const [k, x] of keep) room.set(k, room.get(k) + x);
  return { at, moved, room, file: fileOf, spare };
}

// banks() with each bank kept SPARE bytes free (the headroom rule's: a bank under 64 free has its room won back),
// spread over every bank, not piled in a few: 64 in each if the task's parts fit so; else the bank whose keeping it
// stops them fitting keeps the most that lets them, each other 64 (a bank whose own code leaves less keeps what it
// has, no part put in it); else (two such) the most each bank alike.  spare: banks()'s, Map bank -> bytes or a
// number.  OUT: banks()'s, its spare that.  (A task's parts.txt may keep more in each, its "spare N": the world's 128,
// room8, 2026-11-24, the next round's room spread over its banks)
const SPARE = 64, spareOf = list => list.spare !== undefined ? list.spare : SPARE;
function spread(text, list, sizes, file) {
  const S = spareOf(list);
  const tryIt = s => { try { return banks(text, list, sizes, file, s); } catch (e) { return null; } };
  const most = (lo, hi, at) => { let r = null; while (lo < hi) { const mid = (lo + hi + 1) >> 1, m = tryIt(at(mid)); if (m) { lo = mid; r = m; } else hi = mid - 1; } return r || tryIt(at(lo)); };
  let r = tryIt(S);
  if (r) return r;
  const fail = banks(text, list, sizes, file, 0);       // (Its throw the task's own, if none kept won't do)
  r = mended(fail, list, sizes, S);                     // (The best fit's packing mended, its parts moved or swapped
  if (r) return r;                                      //   till each bank keeps S: speed9, 2026-11-22)
  const ks =[...new Set([...text.matchAll(/^\s*OV(\d+):/gm)].map(m => +m[1]))];
  for (const k of ks) {                                // (One bank the reason: the most it can keep, the rest S)
    const at = s => new Map([[k, s]]);
    if (tryIt(at(0))) return most(0, S - 1, at);
  }
  return most(0, S - 1, s => s) || fail;
}

// banks()'s packing (none kept) mended so each bank keeps SPARE free (one whose room without parts is less, what it
// has): a bank short, one of its parts moved to a bank of its file that keeps its SPARE after, or else swapped with
// a part of another such bank, the swap that leaves the two the most room; again till none is short (speed9,
// 2026-11-22: the best fit, the biggest first, left 13 of the world's banks under 64 with 4,651 bytes to spare among
// them).  The same each build (no chance in it).  r: banks()'s with spare 0.  OUT: banks()'s with spare, or null
function mended(r, list, sizes, spare) {
  const nm = new Map(list.parts.map(p => [p.n, p.name])), size = n => (sizes.get(seg(nm.get(n), 'CODE')) || 0) + (sizes.get(seg(nm.get(n), 'RODATA')) || 0);
  const at = new Map(r.at), room = new Map(r.room), wish = new Map(list.parts.map(p => [p.n, p.ov]));
  const own = new Map([...room].map(([k, x]) => [k, x + [...at].filter(([, b]) => b === k).reduce((t, [n]) => t + size(n), 0)]));
  const want = k => Math.min(spare, Math.max(0, own.get(k)));
  const ks = [...room.keys()].filter(k => [...at.values()].includes(k)).sort((a, b) => a - b);   // (Banks with parts)
  const peers = k => ks.filter(b => b !== k && r.file.get(b) === r.file.get(k));
  const inb = k => [...at].filter(([, b]) => b === k).map(([n]) => n).sort((a, b) => a - b);
  const move = (n, from, to) => { at.set(n, to); room.set(from, room.get(from) + size(n)); room.set(to, room.get(to) - size(n)); };
  for (let i = 0; i < 1000; i++) {
    const k = ks.find(b => room.get(b) < want(b));
    if (k === undefined) {
      const moved = list.parts.filter(p => wish.get(p.n) !== null && at.get(p.n) !== wish.get(p.n)).map(p => ({ n: p.n, from: p.ov, to: at.get(p.n), size: size(p.n) }));
      return { at, moved, room, file: r.file, spare };
    }
    let best = null;                                    // (A move first; else the swap that leaves the most)
    for (const n of inb(k)) {
      for (const b of peers(k)) {
        if (room.get(k) + size(n) >= want(k) && room.get(b) - size(n) >= want(b)) { best = { n, b }; break; }
        for (const m of inb(b)) {
          const d = size(n) - size(m), sc = Math.min(room.get(k) + d - want(k), room.get(b) - d - want(b));
          if (sc >= 0 && (!best || sc > best.sc)) best = { n, b, m, sc };
        }
      }
      if (best && best.m === undefined) break;
    }
    if (!best) return null;
    move(best.n, k, best.b);
    if (best.m !== undefined) move(best.m, best.b, k);
  }
  return null;
}


// banks()'s rules held, each build (make's): a bank too full moves out its smallest part that makes room, not the
// wish that filled it; one with a wish no bank of its file can hold fails; one read later keeps to its own file; a
// part with no bank wished goes in the first file's bank with the least room that holds it, one wishing a group in
// that group's (and wished() reads "-", N, GROUP and GROUP:N so)
function banksChecked() {
  const text = 'MEMORY {\n    OV1: file = "%O.ovl", start = $8000, size = $1000;\n    OV2: file = "%O.ovl", start = $8000, size = $1000;\n' +
    '    OV3: file = "%O.rop", start = $8000, size = $1000;\n}\nSEGMENTS {\n    OV1CODE: load = OV1, type = ro;\n}\n';
  const list = n => ({ parts: n.map((ov, i) => ({ n: i + 1, name: 'p' + (i + 1), ov, what: '' })), extra: [] });
  const sz = o => new Map(Object.entries(o).map(([k, v]) => [k.replace(/^SW(\d+)(CODE|RODATA)$/, (x, n, s) => seg('p' + n, s)), v]));
  const r = banks(text, list([1, 1, 1, 3]), sz({ OV1CODE: 2000, SW1CODE: 1000, SW2CODE: 900, SW3CODE: 300, SW4CODE: 4000 }), 't');
  const ok = r.moved.length === 1 && r.moved[0].n === 3 && r.moved[0].to === 2 && r.at.get(1) === 1 && r.at.get(2) === 1 && r.at.get(4) === 3;
  let failed = false;
  try { banks(text, list([3, 3]), sz({ SW1CODE: 3000, SW2CODE: 2000 }), 't'); } catch (e) { failed = /fits no bank of "%O.rop"/.test(e.message); }
  const t3 = 'MEMORY {\n    OV1: file = "o", size = $3E8;\n    OV2: file = "o", size = $3E8;\n    OV3: file = "o", size = $3E8;\n}\nSEGMENTS {\n}\n';
  const r3 = banks(t3, list([1, 1, 2, 2, 3]), sz({ SW1CODE: 500, SW2CODE: 200, SW3CODE: 950, SW4CODE: 400, SW5CODE: 750 }), 't');
  const made = r3.at.get(4) === 1 && r3.at.get(2) === 3 && r3.at.get(1) === 1 && r3.moved.length === 2;   // (Room made:
  const t4 = 'MEMORY {\n    OV1: file = "o", size = $3E8;\n    OV2: file = "o", size = $3E8;\n}\nSEGMENTS {\n}\n';   // (No swap
  const r4 = banks(t4, list([1, 1, 2, 2, 1]), sz({ SW1CODE: 600, SW2CODE: 350, SW3CODE: 550, SW4CODE: 400, SW5CODE: 100 }), 't');   //   makes
  const all = [1, 2, 1, 2, 2].every((k, i) => r4.at.get([1, 3, 4, 2, 5][i]) === [1, 2, 1, 2, 2][i]);   //   room: all again)
  const t5 = 'MEMORY {\n    OV1: file = "%O.ovl", size = $3E8;\n    OV2: file = "%O.ovl", size = $258;\n    OV3: file = "%O.rop", size = $3E8;\n' +
    '    OV4: file = "%O.rop", size = $1F4;\n}\nSEGMENTS {\n}\n';
  const w5 = wished(t5, { parts: ['-', 'rop', '1', 'rop:2'].map((wish, i) => ({ n: i + 1, name: 'p' + (i + 1), wish, what: '' })) }, 't');
  const r5 = banks(t5, w5, sz({ SW1CODE: 500, SW2CODE: 500, SW3CODE: 100, SW4CODE: 100 }), 't');
  const open = r5.at.get(1) === 2 && r5.at.get(2) === 3 && r5.at.get(3) === 1 && r5.at.get(4) === 4 && !r5.moved.length;   // (The open two
  if (!ok || !failed || !made || !all || !open) throw new Error('games/dq0/parts.js: banks() not as its rules say (banksChecked)');   //   4 in 1, 2 out to 3; best fits)
}

// Segment sizes from od65's dump of objects' (--dump-segsize): Map segment → its bytes, all of them together
function segsizes(dump) {
  const out = new Map();
  for (const [, s, z] of dump.matchAll(/^\s+(\w+):\s+(\d+)\s*$/gm)) out.set(s, (out.get(s) || 0) + +z);
  return out;
}

// The build's: a task with a parts.txt, its C compiled (compiled: OBJDIR's .s of each .c), its parts.inc and its
// config made in objdir, each part in its bank (banks(): sizes, its objects' segments but bank.s's; parts.txt's
// wishes as they are without).  The parts put in banks other than their wishes are written to OBJDIR/placed.txt (a
// line each, "name from to size", for the room table: build.js's headroom); the parts' names in their order to
// OBJDIR/parts.json (names()).  OUT: the config's path
function make({ dir, objdir, compiled, own, sizes }) {
  const task = path.basename(dir), src = path.join(dir, task + '.cfg'), out = path.join(objdir, task + '.cfg'), text = fs.readFileSync(src, 'latin1');
  const list = wished(text, read(path.join(dir, 'parts.txt')), path.join(dir, 'parts.txt'));
  const stubs = wanted(compiled, own.filter(f => path.basename(f) !== 'bank.s'), list.extra, list.num);
  if (sizes) banksChecked();
  const empty = sizes ? list.parts.filter(p => !sizes.get(seg(p.name, 'CODE'))) : [];   // (A part no code names: a line a
  if (empty.length) throw new Error(task + '/parts.txt: ' + empty.map(p => p.name).join(', ') + ': no code in it (no #pragma ' +   //   merge
    'code-name "SW_name_CODE" names it: its line left from a merge, or a part renamed on one side?)');                         //   left?)
  if (sizes) for (const s of ['SWSTP', 'SWSTL', 'SWSTH'])   // (The stubs' tables, swap.inc's SWSTUBOV: a byte a stub each, bank.s's,
    if (new RegExp('^\\s*' + s + ':', 'm').test(text)) sizes.set(s, stubs.length);   //   its bank's own: room7, 2026-11-23)
  const bank = own.find(f => path.basename(f) === 'bank.s');   // (Its overlays' stubs, if its bank.s has OVFN: ovfn.inc)
  const ovs = bank && /^\.macro\s+OVFN\b/m.test(fs.readFileSync(bank, 'latin1')) ? ovstubs(text, compiled, own) : null;
  const dice = ovs && /^DICE_OVL\s*=/m.test(fs.readFileSync(bank, 'latin1')) ? 1 : 0;   // (dice.inc's stub, by hand, in them too)
  if (sizes && ovs) for (const s of ['OVSTO', 'OVSTL', 'OVSTH'])   // (The overlays' stubs' tables, a byte a stub each,
    if (new RegExp('^\\s*' + s + ':', 'm').test(text)) sizes.set(s, ovs.length + dice);   //   bank.s's ovcall's: dq0harn's, fix2)
  const placed = sizes ? spread(text, list, sizes, src) : banks(text, list, new Map(), src);
  const put = { ...list, parts: list.parts.map(p => ({ ...p, ov: placed.at.get(p.n) })) };
  fs.writeFileSync(path.join(objdir, 'parts.inc'), inc(task, put, stubs));
  fs.writeFileSync(out, cfg(text, put, src));
  if (ovs) fs.writeFileSync(path.join(objdir, 'ovfn.inc'), ovinc(task, ovs));
  fs.writeFileSync(path.join(objdir, 'parts.json'), JSON.stringify({ parts: list.parts.map(p => p.name) }) + '\n');
  fs.writeFileSync(path.join(objdir, 'placed.txt'), placed.moved.map(m => list.parts[m.n - 1].name + ' ' + m.from + ' ' + m.to + ' ' + m.size + '\n').join(''));
  fs.writeFileSync(path.join(objdir, 'spare.txt'), (typeof placed.spare === 'number' ? placed.spare : [...placed.spare].map(([k, s]) => 'OV' + k + ' ' + s + ', the rest ' + spareOf(list)).join('')) + '\n');   // (Each bank's room kept free: the room table's)
  return out;
}

// A task's parts' numbers by name, as its build made them (OBJDIR/parts.json: make's): Map name → n, and a function of
// a segment's name giving its part's number (0: no part's).  The emulator's tools' (sim/lib/guards.js, lineprof.js:
// swap.inc's swapped, a part's number)
function names(objdir) {
  let list = [];
  try { list = JSON.parse(fs.readFileSync(path.join(objdir, 'parts.json'), 'latin1')).parts; } catch (e) { /* (No parts) */ }
  const num = new Map(list.map((n, i) => [n, i + 1]));
  return { num, list, of: s => { const m = SEG.exec(s || ''); return m && num.get(m[1]) || 0; } };
}

module.exports = { read, wished, groups, wanted, ovstubs, inc, cfg, banks, spread, segsizes, make, names, seg, SEG };