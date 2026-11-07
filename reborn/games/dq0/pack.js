// pack.js - a Dingus Quest Zero task's swapped parts' functions, read from its build (ENGINE-CRITIQUE.md's packer, its
// second half): what's in each part, in pieces that can move from part to part, and what keeps a piece where it is.
//   - a function or a table: its bytes (the assembler's listing: each line's place in its segment; a segment's last
//     line's, to the object's end of it, od65's), what it refers to, the addresses it takes, the globals it stores
//     them in;
//   - a piece: a function its stub calls (s_NAME, exported) and what only it can reach (its static helpers, its
//     tables, a global function of its part's that isn't s_: called straight, it must stay with its callers), all
//     in one part; it can move whole, its calls from its old part through its stub;
//   - pinned (not to move): a piece with assembly in it or that assembly refers to (the assembly's not read); one
//     whose stubbed function's prototype has a pointer (an argument or its value: into a part's bytes, maybe, gone
//     when the other's swapped in); one that takes the address of a table not its own, or has bytes of no function's.
// The build's (build.js's swappedParts): route(), each build; plan() and apply(), when a part's past the swap area.
// node games/dq0/pack.js TASK [-v]: each part's pieces, what's pinned and why
'use strict';
const fs = require('fs');
const path = require('path');

const partOf = seg => { const m = /^SW(\d+)(CODE|RODATA)$/.exec(seg || ''); return m ? +m[1] : 0; };
const TOKEN = /\b(_\w+|[LMS][0-9A-F]{4})\b/g;
const stubbed = n => n.global && /^_s_/.test(n.name);

// od65's --dump-segsize of objects, each's: Map object's base name → Map segment → bytes
function objectSizes(dump) {
  const out = new Map();
  let cur = null;
  for (const l of dump.split(/\r?\n/)) {
    let m;
    if ((m = /^(\S.*\.o):\s*$/.exec(l))) out.set(path.basename(m[1], '.o'), (cur = new Map()));
    else if (cur && (m = /^\s+(\w+):\s+(\d+)\s*$/.exec(l))) cur.set(m[1], +m[2]);
  }
  return out;
}

// A compiled C file's listing read: its procs and its data labels (file-level, in a part's segment), each a node
// { name, file, global, part, bytes, refs: Map name → count, calls: Set, addrs: Set (#< #> of), leaks: Set (globals
// an address is stored in), proto }.  totals: the object's segments' sizes (od65's)
function readListing(file, totals) {
  const base = path.basename(file, '.lst'), nodes = new Map(), exported = new Set();
  const lines = fs.readFileSync(file, 'latin1').split(/\r?\n/);
  const recs = new Map();                               // (segment → [{ addr, node }]: its lines, in order)
  let seg = 'CODE', proc = null, proto = '', since = 99;
  const data = new Map();
  const node = (name, part) => {
    if (!nodes.has(name)) nodes.set(name, { name, file: base, global: false, part, bytes: 0, refs: new Map(), calls: new Set(),
      addrs: new Set(), leaks: new Set(), proto: '', locals: new Set() });
    return nodes.get(name);
  };
  for (const l of lines) {
    const m = /^([0-9A-F]{6})r\s(\d)\s\s/.exec(l);
    if (!m) continue;
    const at = parseInt(m[1], 16), src = l.slice(24), code = src.replace(/;.*/, '').replace(/"[^"]*"/g, '');
    let k;
    if ((k = /^\s*\.segment\s+"(\w+)"/.exec(src))) { seg = k[1]; continue; }
    if ((k = /^; (.*\w\s*\(.*\))\s*$/.exec(src))) { proto = k[1]; continue; }
    if ((k = /^\s*\.export\s+(.*)/.exec(code))) { k[1].split(',').forEach(n => exported.add(n.trim())); continue; }
    if ((k = /^\.proc\s+(\w+)/.exec(src))) {
      proc = node(k[1], partOf(seg));
      proc.proto = proto;
      proto = '';
      continue;
    }
    if (/^\.endproc/.test(src)) { proc = null; continue; }
    if (/^\s*\.(dbg|import|importzp|fopt|setcpu|smart|autoimport|case|debuginfo|macpack)\b/.test(src)) continue;
    let owner = proc;
    if ((k = /^(\w+)\s*:/.exec(src))) {
      if (proc) proc.locals.add(k[1]);
      else if (partOf(seg)) data.set(seg, node(k[1], partOf(seg)));
    }
    if (!owner && partOf(seg)) owner = data.get(seg) || node('?' + base + ':' + seg, partOf(seg));
    if (!recs.has(seg)) recs.set(seg, []);
    recs.get(seg).push({ addr: at, node: owner });
    if (!owner) continue;
    for (const [, n] of code.matchAll(TOKEN)) {
      if (owner.locals.has(n)) continue;
      owner.refs.set(n, (owner.refs.get(n) || 0) + 1);
      if (/^\s*(\w+:)?\s*(jsr|jmp)\b/.test(code)) owner.calls.add(n);
    }
    if ((k = /#[<>]\(\s*(\w+)/.exec(code))) { owner.addrs.add(k[1]); since = 0; }
    else since++;
    if ((k = /^\s*(\w+:)?\s*st[axyz]\s+(_\w+)/.exec(code)) && since <= 4) owner.leaks.add(k[2]);
  }
  for (const [s, list] of recs) {                       // (A line's bytes: to the next's place in its segment, the
    const end = totals.get(s) || 0;                     //   last's to the segment's end)
    for (let i = 0; i < list.length; i++) {
      const next = i + 1 < list.length ? list[i + 1].addr : end;
      if (list[i].node && next > list[i].addr && partOf(s)) list[i].node.bytes += next - list[i].addr;
    }
  }
  for (const n of nodes.values()) n.global = exported.has(n.name);
  return [...nodes.values()];
}

// An assembly source's part symbols and its references (its own .includes in place): { defs: Set, refs: Set }
function readAsm(file, seen = new Set()) {
  const defs = new Set(), refs = new Set();
  if (seen.has(file)) return { defs, refs };
  seen.add(file);
  let seg = 'CODE';
  for (const l of fs.readFileSync(file, 'latin1').split(/\r?\n/)) {
    let m;
    const inc = /^\s*\.include\s+"([^"]+)"/.exec(l), f = inc && path.resolve(path.dirname(file), inc[1]);
    if (f && fs.existsSync(f)) { const r = readAsm(f, seen); r.defs.forEach(d => defs.add(d)); r.refs.forEach(d => refs.add(d)); continue; }
    if ((m = /^\s*\.segment\s+"?(\w+)"?/.exec(l))) seg = m[1];
    if ((m = /^(\w+)\s*:/.exec(l)) && /^SW\d+/.test(seg)) defs.add(m[1]);
    for (const [, n] of l.replace(/;.*/, '').matchAll(/\b(_\w+)\b/g)) refs.add(n);
  }
  return { defs, refs };
}

// A task's pieces: { pieces: [{ id, part, bytes, nodes: [...], root, pinned: why or '' }], all: its nodes, find:
// (node, name) → the node it means by name (its file's, or a global) }.  lsts: the compiled files' listings; asm: the
// task's own assembly (less bank.s); sizes: objectSizes()'s
function pieces({ lsts, asm, sizes }) {
  const all = [], byName = new Map(), byFile = new Map();
  for (const f of lsts) {
    const list = readListing(f, sizes.get(path.basename(f, '.lst')) || new Map());
    byFile.set(path.basename(f, '.lst'), new Map(list.map(n => [n.name, n])));
    for (const n of list) {
      all.push(n);
      if (n.global) byName.set(n.name, n);
    }
  }
  const asmDefs = new Set(), asmRefs = new Set();
  for (const f of asm) { const r = readAsm(f); r.defs.forEach(d => asmDefs.add(d)); r.refs.forEach(d => asmRefs.add(d)); }
  const find = (n, name) => byFile.get(n.file).get(name) || byName.get(name);
  const up = new Map(all.map(n => [n, n]));             // (Union-find: a node's piece)
  const root = n => { while (up.get(n) !== n) { up.set(n, up.get(up.get(n))); n = up.get(n); } return n; };
  for (const n of all) {
    if (!n.part) continue;
    for (const name of n.refs.keys()) {
      const t = find(n, name);
      if (t && t !== n && t.part && t.part === n.part && !stubbed(t)) up.set(root(t), root(n));
    }
  }
  const by = new Map();
  for (const n of all) if (n.part) {
    const r = root(n);
    if (!by.has(r)) by.set(r, []);
    by.get(r).push(n);
  }
  const out = [];
  for (const list of by.values()) {
    if (list.every(n => !n.bytes && !n.refs.size)) continue;   // (A segment named, nothing in it)
    const top = list.find(stubbed), why = [];
    if (!top) why.push('no stub\'s');
    for (const n of list) if (stubbed(n) && /\*/.test(n.proto)) why.push('a pointer in ' + n.proto.replace(/\s+/g, ' '));
    if (list.some(n => /^\?/.test(n.name))) why.push('bytes of no function\'s');
    if (list.some(n => asmDefs.has(n.name) || asmRefs.has(n.name))) why.push('assembly\'s');
    const mine = new Set(list.map(n => n.name));
    for (const n of list) for (const a of n.addrs) {
      const t = find(n, a);
      if (t && t.part && !mine.has(t.name) && !stubbed(t)) why.push('takes ' + a + '\'s address');
    }
    out.push({ id: top ? top.name : list[0].name, part: list[0].part, bytes: list.reduce((t, n) => t + n.bytes, 0), nodes: list, root: top,
      pinned: [...new Set(why)].join('; ') });
  }
  return { pieces: out.sort((a, b) => a.part - b.part || b.bytes - a.bytes), all, byName, find };
}

// The compiled files' calls routed by part (each .s rewritten in place): a reference to s_NAME from outside its part
// (the main code's, an overlay's, another part's) the stub's, NAME; a call (jsr, jmp) of NAME from inside s_NAME's part
// s_NAME's own, no stub's work.  So where a function's called from needn't say where it is: its pragma alone.  OUT:
// Map s_ function's NAME → its part (the C's)
function route(files) {
  const where = new Map(), text = new Map();
  for (const f of files) {
    const t = fs.readFileSync(f, 'latin1');
    text.set(f, t);
    let seg = 'CODE';
    for (const l of t.split(/\r?\n/)) {
      let m;
      if ((m = /^\s*\.segment\s+"(\w+)"/.exec(l))) seg = m[1];
      else if ((m = /^\.proc\s+_s_(\w+)/.exec(l)) && partOf(seg)) where.set(m[1], partOf(seg));
    }
  }
  for (const f of files) {
    const t = text.get(f), eol = /\r\n/.test(t) ? '\r\n' : '\n';
    let seg = 'CODE', changed = false;
    const out = t.split(/\r?\n/).map(l => {
      const m = /^\s*\.segment\s+"(\w+)"/.exec(l);
      if (m) { seg = m[1]; return l; }
      if (/^\s*\./.test(l) || /^\s*;/.test(l)) return l;
      const p = partOf(seg), call = /^\s*(\w+:)?\s*(jsr|jmp)\b/.test(l);
      const r = l.replace(/\b_s_(\w+)\b/g, (x, n) => where.has(n) && where.get(n) !== p ? '_' + n : x)
        .replace(/\b_(\w+)\b/g, (x, n) => call && p && where.get(n) === p && !/^s_/.test(n) ? '_s_' + n : x);
      if (r !== l) changed = true;
      return r;
    });
    if (changed) fs.writeFileSync(f, out.join(eol), 'latin1');
  }
  return where;
}

// The stub calls in calls below each function (swap.inc's SWDEPTH: its stack's), along the C's calls (none through
// a pointer: as far as can be seen), each call of a stubbed function from outside its part one; a cycle once round.
// An estimate, and a high one: every call's taken as made from within every other (the world's main loop serves
// requests as it waits, so its deepest is 29, its SWDEPTH 12).  all, find: pieces()'s; at: node → its part (a
// move's, hypothetically).  OUT: Map node → its stub calls in calls
function depths(all, find, at) {
  const memo = new Map(), busy = new Set();
  const go = n => {
    if (memo.has(n)) return memo.get(n);
    if (busy.has(n)) return 0;                          // (A cycle: once round it)
    busy.add(n);
    let d = 0;
    for (const c of n.calls) {
      const t = find(n, c) || find(n, c.replace(/^_/, '_s_'));
      if (!t) continue;
      d = Math.max(d, go(t) + (stubbed(t) && at(t) !== at(n) ? 1 : 0));
    }
    busy.delete(n);
    memo.set(n, d);
    return d;
  };
  all.forEach(go);
  return memo;
}

// The pieces moved so each part fits its area (cap: __SWAPSIZE__, or a function of a part giving its room): a part over it moves out a piece free to move,
// the one with the fewest calls to and from the rest of its part that's big enough to make room (or the biggest, and
// again); to the part with the most calls to and from it that has room for it (then the most room, the lowest
// number).  A piece isn't moved from its part if a call between them would carry a pointer (its call of one of the
// part's stubbed functions with a pointer in its prototype), or if one stores an address in a global the other reads
// (a pointer to bytes gone when the other's swapped in), or if it would take a function's stub calls in calls
// (depths()'s) past maxDepth (one already past it, by that estimate, can't be judged so: there it's the tests').
// partBytes: Map part → its bytes (all its objects', its assembly's too); parts: the parts it may move pieces to;
// banks (parts.js's banks()'s: each part's bank, each bank's room and file): a piece goes only to a part whose bank
// has room for it (the world's banks are fuller than its parts) and is of its own part's bank's file (one read
// later, dq0w.rop's, read as ropes are first wanted, has only what's called after).
// OUT: [{ piece, from, to }]; throws, a part that can't be made to fit
function plan({ pieces: ps, all, find }, partBytes, cap, maxDepth, parts, banks) {
  const capOf = typeof cap === 'function' ? cap : () => cap;
  const bankOf = q => banks.at.get(q), left = new Map(banks.room);
  const at = new Map();
  for (const p of ps) for (const n of p.nodes) at.set(n, p.part);
  const partNow = n => at.has(n) ? at.get(n) : n.part;
  const size = new Map(partBytes);
  const target = (n, r) => find(n, r) || find(n, r.replace(/^_/, '_s_'));
  const links = (p, part) => {                          // (Calls and references between piece p and a part's others)
    let k = 0;
    const mine = new Set(p.nodes);
    for (const n of all) {
      const inP = mine.has(n);
      if (!inP && partNow(n) !== part) continue;
      for (const [r, c] of n.refs) {
        const t = target(n, r);
        if (t && (inP ? !mine.has(t) && partNow(t) === part : mine.has(t))) k += c;
      }
    }
    return k;
  };
  const safe = (p, part) => {                           // (Piece p can leave part: see above)
    const mine = new Set(p.nodes), rest = all.filter(n => !mine.has(n) && partNow(n) === part);
    for (const n of p.nodes) for (const c of n.calls) {
      const t = target(n, c);
      if (t && !mine.has(t) && partNow(t) === part && /\*/.test(t.proto)) return 'calls ' + t.name + ' (' + t.proto.replace(/\s+/g, ' ') + ')';
    }
    const reads = list => new Set(list.flatMap(n => [...n.refs.keys()]));
    const leakP = new Set(p.nodes.flatMap(n => [...n.leaks])), leakR = new Set(rest.flatMap(n => [...n.leaks]));
    const rR = reads(rest), rP = reads(p.nodes);
    for (const g of leakP) if (rR.has(g)) return 'an address in ' + g + ', read by the part\'s others';
    for (const g of leakR) if (rP.has(g)) return 'an address in ' + g + ', from the part\'s others';
    return '';
  };
  const moves = [];
  for (const part of [...size.keys()].sort((a, b) => a - b)) {
    while (size.get(part) > capOf(part)) {
      const over = size.get(part) - capOf(part), why = [];
      const cand = ps.filter(p => p.part === part && !p.pinned).map(p => ({ p, l: links(p, part), w: safe(p, part) }));
      cand.filter(c => c.w).forEach(c => why.push(c.p.id + ': ' + c.w));
      const ok = cand.filter(c => !c.w).sort((a, b) => (b.p.bytes >= over) - (a.p.bytes >= over) || a.l - b.l ||
        (a.p.bytes >= over ? a.p.bytes - b.p.bytes : b.p.bytes - a.p.bytes) || (a.p.id < b.p.id ? -1 : 1));
      let done = false;
      for (const { p } of ok) {
        const to = [...size.keys()].filter(q => q !== part && parts.has(q) && size.get(q) + p.bytes <= capOf(q) &&
          left.get(bankOf(q)) >= p.bytes && banks.file.get(bankOf(q)) === banks.file.get(bankOf(part)))
          .map(q => ({ q, l: links(p, q) })).sort((a, b) => b.l - a.l || size.get(a.q) - size.get(b.q) || a.q - b.q);
        if (!to.length) why.push(p.id + ' (' + p.bytes + '): no part with room for it');
        for (const { q } of to) {
          const before = depths(all, find, partNow);
          for (const n of p.nodes) at.set(n, q);
          const after = depths(all, find, partNow);
          if (!all.some(n => before.get(n) <= maxDepth && after.get(n) > maxDepth)) {
            p.part = q;
            size.set(part, size.get(part) - p.bytes);
            size.set(q, size.get(q) + p.bytes);
            left.set(bankOf(part), left.get(bankOf(part)) + p.bytes);
            left.set(bankOf(q), left.get(bankOf(q)) - p.bytes);
            moves.push({ piece: p, from: part, to: q });
            done = true;
            break;
          }
          for (const n of p.nodes) at.set(n, part);
          why.push(p.id + ' to ' + q + ': stub calls in calls past ' + maxDepth);
        }
        if (done) break;
      }
      if (!done) throw new Error('part ' + part + ' is ' + over + ' bytes past the swap area (' + capOf(part) + '), and no piece of it can move: ' +
        (why.length ? why.slice(0, 6).join('; ') : 'none free to move, or no part with room') + ' (node games/dq0/pack.js TASK -v: its pieces)');
    }
  }
  return moves;
}

// plan()'s rules held, each build (build.js's swappedParts): part 1 50 past its area; s_b can't leave it (it calls
// s_p, a pointer in its prototype); s_a, enough to make room, leaves before s_c, too small; to part 3, which it
// calls, not to part 2, with more room; but to part 2 when part 3's bank is of another file
function planChecked() {
  const n = (name, part, bytes, calls = [], proto = '') => ({ name, file: 'f', global: true, part, bytes, refs: new Map(calls.map(c => [c, 1])),
    calls: new Set(calls), addrs: new Set(), leaks: new Set(), proto });
  const nodes = [n('_s_a', 1, 60, ['_s_t']), n('_s_b', 1, 50, ['_s_p']), n('_s_c', 1, 30, ['_s_a']), n('_s_p', 1, 10, [], 'void s_p (char* s)'),
    n('_s_u', 2, 20), n('_s_t', 3, 10)];
  const byName = new Map(nodes.map(x => [x.name, x])), find = (x, name) => byName.get(name);
  const ps = nodes.map(x => ({ id: x.name, part: x.part, bytes: x.bytes, nodes: [x], root: x, pinned: /\*/.test(x.proto) ? 'a pointer' : '' }));
  const banks = { at: new Map([[1, 1], [2, 2], [3, 3]]), room: new Map([[1, 0], [2, 500], [3, 500]]), file: new Map([[1, 'o'], [2, 'o'], [3, 'o']]) };
  const m = plan({ pieces: ps, all: nodes, find }, new Map([[1, 150], [2, 20], [3, 10]]), 100, 8, new Set([1, 2, 3]), banks);
  banks.file.set(3, 'r');                              // (Part 3's bank another file's: to part 2)
  for (const p of ps) p.part = p.nodes[0].part;
  const m2 = plan({ pieces: ps, all: nodes, find }, new Map([[1, 150], [2, 20], [3, 10]]), 100, 8, new Set([1, 2, 3]), banks);
  if (m.length !== 1 || m[0].piece.id !== '_s_a' || m[0].to !== 3 || m2.length !== 1 || m2[0].to !== 2)
    throw new Error('games/dq0/pack.js: plan() not as its rules say (planChecked)');
}

// The moves made in the compiled files (their .s, rewritten in place): a function's block, from the segment it's
// begun in to its .endproc, its part's segments the new part's; a table's label put in the new part's segment, the
// old one's after its bytes.  OUT: the files changed
function apply(moves, objdir) {
  const byFile = new Map();
  for (const { piece, from, to } of moves) for (const n of piece.nodes) {
    if (!byFile.has(n.file)) byFile.set(n.file, []);
    byFile.get(n.file).push({ n, from, to });
  }
  const changed = [];
  for (const [file, list] of byFile) {
    const f = path.join(objdir, file + '.s'), t = fs.readFileSync(f, 'latin1'), eol = /\r\n/.test(t) ? '\r\n' : '\n';
    const L = t.split(/\r?\n/), want = new Map(list.map(x => [x.n.name, x]));
    const out = [];
    for (let i = 0; i < L.length; i++) {
      const pm = /^\.proc\s+(\w+)/.exec(L[i]), dm = /^(\w+)\s*:/.exec(L[i]), mv = (pm || dm) && want.get((pm || dm)[1]);
      if (pm && mv) {
        const re = new RegExp('"SW' + mv.from + '(CODE|RODATA)"', 'g'), to = '"SW' + mv.to + '$1"';
        let j = out.length - 1;
        while (j >= 0 && !/^\s*\.segment\b/.test(out[j]) && !/^\.endproc/.test(out[j])) j--;
        if (j >= 0 && /^\s*\.segment\b/.test(out[j])) out[j] = out[j].replace(re, to);
        for (; i < L.length && !/^\.endproc/.test(L[i]); i++) out.push(L[i].replace(re, to));
        out.push(L[i]);
      } else if (dm && mv) {
        out.push('.segment\t"SW' + mv.to + 'RODATA"', L[i]);
        for (i++; i < L.length && /^\s+\.(byte|word|addr|res|dbyt|lobytes|hibytes|asciiz|faraddr|dword)\b/.test(L[i]); i++) out.push(L[i]);
        out.push('.segment\t"SW' + mv.from + 'RODATA"');
        i--;
      } else {
        out.push(L[i]);
      }
    }
    fs.writeFileSync(f, out.join(eol), 'latin1');
    changed.push(f);
  }
  return changed;
}

module.exports = { objectSizes, readListing, readAsm, pieces, route, depths, plan, planChecked, apply };

if (require.main === module) {                          // (The report: node games/dq0/pack.js TASK [-v])
  const { execFileSync } = require('child_process');
  const task = process.argv[2], root = path.join(__dirname, '..', '..'), od = path.join(root, 'obj', 'games', task), dir = path.join(root, 'games', task);
  const src = fs.readdirSync(dir), lsts = src.filter(f => f.endsWith('.c')).map(f => path.join(od, f.replace(/\.c$/, '.lst')));
  const asm = src.filter(f => f.endsWith('.s') && f !== 'bank.s').map(f => path.join(dir, f));
  const od65 = path.join(root, '..', 'base', 'tools', 'cc65', 'win64_hydra', 'bin', 'od65');
  const sizes = objectSizes(execFileSync(od65, ['--dump-segsize', ...lsts.map(f => f.replace(/\.lst$/, '.o'))], { encoding: 'latin1' }));
  const r = pieces({ lsts, asm, sizes });
  let free = 0, pinned = 0;
  for (const p of r.pieces) {
    if (p.pinned) pinned += p.bytes; else free += p.bytes;
    if (process.argv[3] === '-v') console.log('part ' + p.part + ' ' + p.id + ' ' + p.bytes + ' (' + p.nodes.length + ')' + (p.pinned ? ' pinned: ' + p.pinned : ''));
  }
  const n = r.pieces.length, np = r.pieces.filter(p => p.pinned).length;
  console.log(task + ': ' + n + ' pieces in its parts, ' + (n - np) + ' free to move (' + free + ' bytes), ' + np + ' pinned (' + pinned + ' bytes)');
}
