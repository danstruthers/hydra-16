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
      if (/^\s*\.(import|dbg)\b/.test(l)) continue;     // (The compiler's imports of all it names, not its uses)
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
    if (!/^SW\d+(CODE|RODATA)$/.test(s) && room.has(+k)) room.set(+k, room.get(+k) - (sizes.get(s) || 0));
  const keep = new Map([...room].map(([k, x]) => [k, closed.has(k) ? 0 : Math.min(typeof spare === 'number' ? spare : spare.has(k) ? spare.get(k) : SPARE, Math.max(0, x))]));   // (spare:
  for (const [k, x] of keep) room.set(k, room.get(k) - x);                                        //   each bank's kept free)
  const base = new Map(room);                           // (Each bank's room before any part's in it)
  const size = p => (sizes.get('SW' + p.n + 'CODE') || 0) + (sizes.get('SW' + p.n + 'RODATA') || 0);
  const at = new Map(), left = [], moved = [];
  for (const p of list.parts)
    if (!cap.has(p.ov)) throw new Error(file + ': part ' + p.n + '\'s bank, OV' + p.ov + ', not a bank of the config\'s');
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
  const same = p => [...room].filter(([k]) => fileOf.get(k) === fileOf.get(p.ov) && !closed.has(k));
  const put = (p, k) => {
    const was = moved.findIndex(m => m.n === p.n);
    if (was >= 0) moved.splice(was, 1);
    at.set(p.n, k);
    room.set(k, room.get(k) - size(p));
    if (k !== p.ov) moved.push({ n: p.n, from: p.ov, to: k, size: size(p) });
  };
  const redone = new Set();                            // (The files whose parts were all placed again)
  for (const p of left) {
    if (redone.has(fileOf.get(p.ov))) continue;
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
      const f = fileOf.get(p.ov), mine = list.parts.filter(q => fileOf.get(q.ov) === f);   //   each in the bank with the
      const free = new Map([...base].filter(([k]) => fileOf.get(k) === f && !closed.has(k)));   //   least room that holds it)
      const place = new Map();
      for (const q of [...mine].sort((a, b) => size(b) - size(a) || a.n - b.n)) {
        const c = [...free].filter(([, x]) => x >= size(q)).sort((x, y) => x[1] - y[1] || x[0] - y[0])[0];
        if (!c) throw new Error(file + ': part ' + p.n + ' (' + size(p) + ' bytes) fits no bank of "' + f + '", not even with its parts all ' +
          'placed again: ' + [...base].filter(([k]) => fileOf.get(k) === f).reduce((t, [, x]) => t + x, 0) + ' bytes for them, ' +
          mine.reduce((t, x) => t + size(x), 0) + ' wanted (a bank more, or the parts smaller)');
        place.set(q.n, c[0]);
        free.set(c[0], c[1] - size(q));
      }
      for (const q of mine) {
        const was = moved.findIndex(m => m.n === q.n);
        if (was >= 0) moved.splice(was, 1);
        at.set(q.n, place.get(q.n));
        if (place.get(q.n) !== q.ov) moved.push({ n: q.n, from: q.ov, to: place.get(q.n), size: size(q) });
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
// number.  OUT: banks()'s, its spare that
const SPARE = 64;
function spread(text, list, sizes, file) {
  const tryIt = s => { try { return banks(text, list, sizes, file, s); } catch (e) { return null; } };
  const most = (lo, hi, at) => { let r = null; while (lo < hi) { const mid = (lo + hi + 1) >> 1, m = tryIt(at(mid)); if (m) { lo = mid; r = m; } else hi = mid - 1; } return r || tryIt(at(lo)); };
  let r = tryIt(SPARE);
  if (r) return r;
  const fail = banks(text, list, sizes, file, 0);       // (Its throw the task's own, if none kept won't do)
  r = mended(fail, list, sizes, SPARE);                 // (The best fit's packing mended, its parts moved or swapped
  if (r) return r;                                      //   till each bank keeps SPARE: speed9, 2026-11-22)
  const ks =[...new Set([...text.matchAll(/^\s*OV(\d+):/gm)].map(m => +m[1]))];
  for (const k of ks) {                                // (One bank the reason: the most it can keep, the rest 64)
    const at = s => new Map([[k, s]]);
    if (tryIt(at(0))) return most(0, SPARE - 1, at);
  }
  return most(0, SPARE - 1, s => s) || fail;
}

// banks()'s packing (none kept) mended so each bank keeps SPARE free (one whose room without parts is less, what it
// has): a bank short, one of its parts moved to a bank of its file that keeps its SPARE after, or else swapped with
// a part of another such bank, the swap that leaves the two the most room; again till none is short (speed9,
// 2026-11-22: the best fit, the biggest first, left 13 of the world's banks under 64 with 4,651 bytes to spare among
// them).  The same each build (no chance in it).  r: banks()'s with spare 0.  OUT: banks()'s with spare, or null
function mended(r, list, sizes, spare) {
  const size = n => (sizes.get('SW' + n + 'CODE') || 0) + (sizes.get('SW' + n + 'RODATA') || 0);
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
      const moved = list.parts.filter(p => at.get(p.n) !== wish.get(p.n)).map(p => ({ n: p.n, from: p.ov, to: at.get(p.n), size: size(p.n) }));
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
// wish that filled it; one with a wish no bank of its file can hold fails; one read later keeps to its own file
function banksChecked() {
  const text = 'MEMORY {\n    OV1: file = "%O.ovl", start = $8000, size = $1000;\n    OV2: file = "%O.ovl", start = $8000, size = $1000;\n' +
    '    OV3: file = "%O.rop", start = $8000, size = $1000;\n}\nSEGMENTS {\n    OV1CODE: load = OV1, type = ro;\n}\n';
  const list = n => ({ parts: n.map((ov, i) => ({ n: i + 1, ov, what: '' })), extra: [] });
  const sz = o => new Map(Object.entries(o));
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
  if (!ok || !failed || !made || !all) throw new Error('games/dq0/parts.js: banks() not as its rules say (banksChecked)');   //   4 in 1, 2 out to 3)
}

// Segment sizes from od65's dump of objects' (--dump-segsize): Map segment → its bytes, all of them together
function segsizes(dump) {
  const out = new Map();
  for (const [, s, z] of dump.matchAll(/^\s+(\w+):\s+(\d+)\s*$/gm)) out.set(s, (out.get(s) || 0) + +z);
  return out;
}

// The build's: a task with a parts.txt, its C compiled (compiled: OBJDIR's .s of each .c), its parts.inc and its
// config made in objdir, each part in its bank (banks(): sizes, its objects' segments but bank.s's; parts.txt's
// banks as they are without).  The parts put in banks other than parts.txt's are written to OBJDIR/placed.txt (a
// line each, "n from to size", for the room table: build.js's headroom).  OUT: the config's path
function make({ dir, objdir, compiled, own, sizes }) {
  const task = path.basename(dir), list = read(path.join(dir, 'parts.txt'));
  const stubs = wanted(compiled, own.filter(f => path.basename(f) !== 'bank.s'), list.extra);
  for (const s of stubs) if (!list.parts.some(p => p.n === s.part))
    throw new Error(task + ': ' + s.name + ' is in part ' + s.part + ', which parts.txt doesn\'t have');
  const src = path.join(dir, task + '.cfg'), out = path.join(objdir, task + '.cfg'), text = fs.readFileSync(src, 'latin1');
  if (sizes) banksChecked();
  const placed = sizes ? spread(text, list, sizes, src) : { at: new Map(list.parts.map(p => [p.n, p.ov])), moved: [], spare: 0 };
  const put = { ...list, parts: list.parts.map(p => ({ ...p, ov: placed.at.get(p.n) })) };
  fs.writeFileSync(path.join(objdir, 'parts.inc'), inc(task, put, stubs));
  fs.writeFileSync(out, cfg(text, put, src));
  fs.writeFileSync(path.join(objdir, 'placed.txt'), placed.moved.map(m => m.n + ' ' + m.from + ' ' + m.to + ' ' + m.size + '\n').join(''));
  fs.writeFileSync(path.join(objdir, 'spare.txt'), (typeof placed.spare === 'number' ? placed.spare : [...placed.spare].map(([k, s]) => 'OV' + k + ' ' + s + ', the rest ' + SPARE).join('')) + '\n');   // (Each bank's room kept free: the room table's)
  return out;
}

module.exports = { read, wanted, inc, cfg, banks, spread, segsizes, make };
