// saidgen.js - Dingus Quest Zero's phrase table, made by the build (ENGINE-CRITIQUE.md's §3.2, narration as data): its
// source games/dq0/said.txt (its head says its form) packed into obj/gen/dq0said.h, for the UI's one renderer (said.c,
// overlay 27, the table its #include's: SAID_TABLE defined; its rows in overlay 28's bank, read a byte at a time by
// bank.s's said_b) and the names of the UI's own outcomes (SAID_KIND_NAME, any file's).  build.js's hubNumbers calls
// generate (H) after hubgen.js's: the outcome events (H.outcome: msgs.def's kinds that include E_OUTCOME; and the
// kinds said.txt names with their own outcome field, its `kind` lines), their fields' places in struct msg (H.msgs, as
// cc65 lays them out) and the headers' names (H.C, H.enums) its own.  Made again only when said.txt, this file or the
// headers' numbers change.
//
// The packed form (said.c reads it):
//   said_kind[n], said_at[n]   each outcome kind, and where its rows begin in said_rows
//   said_ko[n]                 each kind's outcome: its field (said_f's)
//   said_f[]                   the fields a row reads, five bytes each: its byte's offset in struct msg, the shift and
//                              the mask that take it out, an operation (SF_*: none, less K, more than K, times K) and K
//   said_rows[]                (overlay 28's) a kind's rows, each: its length (the bytes to the next), its outcome
//                              (SAID_ANY any), its condition's field (0xFF none; | 0x80, the field's one value, else the
//                              mask of its values, bit V for V) and that value or mask, its flags (SR_ONCE), its text
//                              (below), 0; then 0.  After the kinds' rows, the pieces (said.txt's @NAME, each kind's its
//                              own: its fields its), each its text and 0, said_pc[] where each begins
//   said_dw[], said_do[]       the words most said (chosen here: the bytes they save), and where each begins in
//                              said_dw: a text's codes 2-8 and 11-29 the first 26, then SC_WORD and a byte, N, the
//                              26 + N - 1th (the rows' bank's room, and the bytes read in as the game starts)
//   A text's bytes: the letters as they are ('\n' a new line); SC_NL a line of its own; SC_NUM + F the field F's value;
//   SC_CHOOSE + F, SC_CHANGED + F a block: its alternatives' texts between SC_ALT, ended by SC_END; SC_STR + 2K (+ 1
//   capitalised) the UI's word K (obj, instr, text, actor); SC_IF + K a block, a choice by the word K given (actor: a
//   denizen), SC_IF + 4 by the UI's number; SC_S, SC_A, SC_LIGHT, SC_FORGET, SC_ARG, SC_SING; SC_WORD N; SC_ROOM F, SC_REL F,
//   SC_TRAP F G, SC_KIND F, SC_NAME F, SC_PIECE P (each F and P there plus 1: no 0 in a text)
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const ROOT = path.join(__dirname, '..', '..');
const SRC = path.join(__dirname, 'said.txt'), OUT = path.join(ROOT, 'obj', 'gen', 'dq0said.h');

// The codes (said.c's, the same: its SC_*)
const SC = { NL: 0x01, END: 0x1E, ALT: 0x1F, NUM: 0x80, CHOOSE: 0xA0, CHANGED: 0xC0, STR: 0xE0, S: 0xE8, A: 0xE9,
  LIGHT: 0xEA, FORGET: 0xEB, ARG: 0xEC, SING: 0xED, IF: 0xF0, ROOM: 0xF8, TRAP: 0xF9, REL: 0xFA, PIECE: 0xFB,
  KIND: 0xFC, NAME: 0xFD, WORD: 0xEE };
const WORDS_MAX = 200;                                        // (The words said as SC_WORD and a byte, at most)
const ANY = 0xFE;                                             // (A row for any outcome: said.c's SAID_ANY)
const OP = { '': 0, '-': 1, '>': 2, '*': 3 };
const WORDS = ['obj', 'instr', 'text', 'actor'];
const DICT = [2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29];   // (A word's codes)

// said.txt read: its entries ({ head, texts: [{ text, on }], line }), a line begun with a space going on with the last
function entries(text) {
  const out = [];
  text.split(/\r?\n/).forEach((l, i) => {
    if (!l.trim() || /^\s*#/.test(l)) return;
    if (/^\s/.test(l)) {
      if (!out.length) throw new Error('said.txt:' + (i + 1) + ': goes on from nothing');
      out[out.length - 1].s += ' ' + l.trim();
      return;
    }
    out.push({ s: l.trim(), line: i + 1 });
  });
  return out;
}

// An entry's quoted pieces put together: { head (before the first quote), text, on (a + before it) }
function quoted(e) {
  const q = e.s.indexOf('"');
  if (q < 0) return { head: e.s, text: null, on: false };
  let head = e.s.slice(0, q).trim(), on = false;
  if (head.endsWith('+')) { on = true; head = head.slice(0, -1).trim(); }
  let text = '', i = q;
  while (i < e.s.length) {
    if (e.s[i] === ' ') { i++; continue; }
    if (e.s[i] !== '"') throw new Error('said.txt:' + e.line + ': after its text, "' + e.s.slice(i) + '"');
    for (i++; i < e.s.length && e.s[i] !== '"'; i++) {
      if (e.s[i] === '\\') {
        const c = e.s[++i];
        text += c === 'n' ? '\n' : c === '"' || c === '\\' ? c : (() => { throw new Error('said.txt:' + e.line + ': \\' + c + '?'); })();
      } else text += e.s[i];
    }
    if (i >= e.s.length) throw new Error('said.txt:' + e.line + ': a quote not closed');
    i++;
  }
  return { head, text, on };
}

function generate(H, say = () => {}) {
  const src = fs.readFileSync(SRC, 'latin1');
  const key = crypto.createHash('sha1').update(src).update(fs.readFileSync(__filename, 'latin1'))
    .update(JSON.stringify([H.outcome, H.msgs, H.C])).digest('hex');
  if (fs.existsSync(OUT) && fs.readFileSync(OUT, 'latin1').includes('key ' + key)) return;
  const list = entries(src).map(e => ({ ...e, ...quoted(e) }));
  const formsOf = k => Object.keys(H.msgs).filter(n => n === k || n.startsWith(k + ':')).map(n => H.msgs[n]);
  const kind = (k, of, line) => {                             // (An outcome kind: its outcome field, its enum)
    if (!of || !of[5]) throw new Error('saidgen: ' + k + ': no outcome field with its enum (msgs.def\'s @)' + (line ? ' (said.txt:' + line + ')' : ''));
    const pre = of[5].slice(0, of[5].indexOf('_') + 1), e = H.enums.find(x => x.includes(of[5])).filter(n => n.startsWith(pre));
    return { k, short: k.replace(/^E_/, ''), forms: formsOf(k), of: of[0], pre, members: e, own: [], rows: [], v: H.C[k] };
  };
  const kinds = H.outcome.map(k => {                          // (The head's kinds: their field in the head's place)
    const head = H.msgs.E_OUTCOME.fields.find(f => f[0] === 'outcome');
    return kind(k, formsOf(k)[0].fields.find(f => f[1] === head[1] && f[2] === head[2] && f[3] === head[3] && f[4] === head[4]));
  });
  for (const e of list) {                                     // (`kind SHORT FIELD`: a kind worded by its own outcome
    const m = e.text === null && e.head.match(/^kind\s+(\w+)\s+(\w+)$/);   //   field, its bytes where they are)
    if (!m) continue;
    const k = 'E_' + m[1];
    if (!H.msgs[k]) throw new Error('said.txt:' + e.line + ': ' + k + '?');
    if (kinds.some(K => K.k === k)) throw new Error('said.txt:' + e.line + ': ' + k + ' an outcome kind already');
    kinds.push(kind(k, formsOf(k).map(f => f.fields.find(x => x[0] === m[2])).find(x => x), e.line));
  }
  const byShort = Object.fromEntries(kinds.map(x => [x.short, x]));
  const pieces = {};
  for (const e of list) {                                     // (The pieces and the own outcomes first: used anywhere)
    if (e.head.startsWith('@')) { pieces[e.head.slice(1)] = e.text; continue; }
    const m = e.head.match(/^own\s+(\w+)\s+(.+)$/);
    if (m) {
      const K = byShort[m[1]];
      if (!K) throw new Error('said.txt:' + e.line + ': ' + m[1] + ', not an outcome event\'s kind');
      K.own.push(...m[2].trim().split(/\s+/));
    }
  }
  const outVal = (K, name, line) => {
    if (name === 'any') return ANY;
    const c = K.pre + name.toUpperCase();
    if (K.members.includes(c)) return H.C[c];
    const i = K.own.indexOf(name);
    if (i < 0) throw new Error('said.txt:' + line + ': ' + K.short + '.' + name + '? (no ' + c + ', nor an own one)');
    return Math.max(...K.members.map(n => H.C[n])) + 1 + i;
  };
  const fields = [], fieldIx = new Map();
  const field = (K, expr, line) => {                          // (A field's descriptor's place, made if new)
    const m = expr.match(/^([a-z]\w*)(?:([-*>])(\w+))?$/);
    if (!m) throw new Error('said.txt:' + line + ': {' + expr + '}?');
    const at = new Set();
    for (const f of K.forms) for (const x of f.fields) if (x[0] === m[1]) at.add(x.slice(1, 5).join(','));
    if (at.size !== 1) throw new Error('said.txt:' + line + ': ' + K.k + '\'s ' + m[1] + (at.size ? ', in two places' : '?'));
    const [off, , lo, hi] = [...at][0].split(',').map(Number);
    if (lo >> 3 !== hi >> 3) throw new Error('said.txt:' + line + ': ' + m[1] + ' across two bytes');
    let k = 0;
    if (m[2]) {
      k = /^\d+$/.test(m[3]) ? +m[3] : H.C[m[3]];
      if (k === undefined || k > 255) throw new Error('said.txt:' + line + ': ' + m[3] + '?');
    }
    const d = [off + (lo >> 3), lo & 7, 2 ** (hi - lo + 1) - 1, OP[m[2] || ''], k], s = d.join(',');
    if (!fieldIx.has(s)) { fieldIx.set(s, fields.length); fields.push(d); }
    if (fieldIx.get(s) >= 32) throw new Error('saidgen: more than 32 fields');
    return fieldIx.get(s);
  };
  const pc = [], pcIx = new Map();
  const piece = (K, p, line) => {                             // (A piece's place, compiled for the kind the first time)
    if (pieces[p] === undefined) throw new Error('said.txt:' + line + ': no @' + p);
    const key = K.k + '@' + p;
    if (!pcIx.has(key)) {
      pcIx.set(key, -1);
      const b = compile(K, pieces[p], line);
      pcIx.set(key, pc.length);
      pc.push({ c: '@' + p + ' (' + K.short + ')', b: [...b, 0] });
    }
    if (pcIx.get(key) < 0) throw new Error('said.txt:' + line + ': @' + p + ' in itself');
    if (pcIx.get(key) >= 254) throw new Error('saidgen: more than 254 pieces');
    return pcIx.get(key);
  };
  const compile = (K, text, line) => {                        // (A text's bytes)
    let i = 0;
    const seq = (stops) => {
      const b = [];
      while (i < text.length && !(stops && stops.includes(text[i]))) {
        const c = text[i];
        if (c !== '{') {
          if (c !== '\n' && (c < ' ' || c > '~')) throw new Error('said.txt:' + line + ': a letter that isn\'t plain: ' + JSON.stringify(c));
          b.push(c.charCodeAt(0)); i++; continue;
        }
        const m = text.slice(i + 1).match(/^([^?!}]*)([?!}])/);
        if (!m) throw new Error('said.txt:' + line + ': a brace not closed');
        i += 1 + m[0].length;
        const [name, ...args] = m[1].trim().split(/\s+/), lc = name.toLowerCase(), w = WORDS.indexOf(lc);
        const block = (code) => {                             // (Alternatives, or a text said only if, to its })
          b.push(code);
          for (;;) {
            b.push(...seq(m[2] === '?' ? '|}' : '}'));
            if (text[i] === '}') { i++; break; }
            if (text[i] !== '|') throw new Error('said.txt:' + line + ': a brace not closed');
            i++; b.push(SC.ALT);
          }
          b.push(SC.END);
        };
        if (name[0] === '@') { b.push(SC.PIECE, piece(K, name.slice(1), line) + 1); continue; }
        if (m[2] === '?') {
          if (w >= 0 && name === lc) block(SC.IF + w);
          else if (name === 'arg') block(SC.IF + 4);
          else block(SC.CHOOSE + field(K, name, line));
          continue;
        }
        if (m[2] === '!') { block(SC.CHANGED + field(K, name, line)); continue; }
        if (w >= 0) b.push(SC.STR + 2 * w + (name === lc ? 0 : 1));
        else if (name === 'nl') b.push(SC.NL);
        else if (name === 's') b.push(SC.S);
        else if (name === 'a') b.push(SC.A);
        else if (name === '-s') b.push(SC.SING);
        else if (name === 'light') b.push(SC.LIGHT);
        else if (name === 'forget') b.push(SC.FORGET);
        else if (name === 'arg') b.push(SC.ARG);
        else if (name === 'room' || name === 'rel' || name === 'kind' || name === 'name') {
          b.push(SC[name.toUpperCase()], field(K, args[0], line) + 1);
        } else if (name === 'trap') b.push(SC.TRAP, field(K, args[0], line) + 1, field(K, args[1], line) + 1);
        else b.push(SC.NUM + field(K, name, line));
      }
      return b;
    };
    const b = seq(null);
    if (i < text.length) throw new Error('said.txt:' + line + ': "' + text.slice(i) + '"?');
    return b;
  };
  for (const e of list) {                                     // (The rows)
    if (e.head.startsWith('@') || /^(own|kind)\s/.test(e.head)) continue;
    const m = e.head.match(/^(\w+)\.(\w+)(?:\[([^\]]+)\])?(\s+once)?$/);
    if (!m || e.text === null) throw new Error('said.txt:' + e.line + ': "' + e.s + '"?');
    const K = byShort[m[1]];
    if (!K) throw new Error('said.txt:' + e.line + ': ' + m[1] + ', not an outcome event\'s kind (msgs.def\'s < E_OUTCOME, or said.txt\'s kind)');
    let cf = 0xFF, mask = 0;
    if (m[3]) {
      const c = m[3].match(/^([^=]+)=(.+)$/);
      if (!c) throw new Error('said.txt:' + e.line + ': [' + m[3] + ']?');
      cf = field(K, c[1].trim(), e.line);
      const vs = c[2].split(',').map(s => s.trim()).map(v => /^\d+$/.test(v) ? +v : H.C[v]);
      if (vs.some(n => n === undefined || n > 255)) throw new Error('said.txt:' + e.line + ': [' + m[3] + ']: a value?');
      if (vs.every(n => n < 8)) for (const n of vs) mask |= 1 << n;
      else if (vs.length === 1) { cf |= 0x80; mask = vs[0]; }
      else throw new Error('said.txt:' + e.line + ': [' + m[3] + ']: values past 7, more than one');
    }
    const out = outVal(K, m[2], e.line);
    K.rows.push({ out, cf, mask, flags: m[4] ? 1 : 0, bytes: [...(e.on ? [] : [SC.NL]), ...compile(K, e.text, e.line)],
      name: m[1] + '.' + m[2] + (m[3] ? '[' + m[3] + ']' : ''), cond: !!m[3] });
  }
  // The C: the kinds, the fields, the rows (each kind's with a condition first, in their order, then those without;
  // the rows for any outcome after them, the same way)
  const hex = v => '0x' + v.toString(16).toUpperCase().padStart(2, '0');
  const rows = [], at = [];
  for (const K of kinds) {
    at.push(rows.length);
    const seen = new Set(), mine = K.rows.filter(r => r.out !== ANY), any = K.rows.filter(r => r.out === ANY);
    for (const r of [...mine.filter(r => r.cond), ...mine.filter(r => !r.cond), ...any.filter(r => r.cond), ...any.filter(r => !r.cond)]) {
      if (!r.cond && seen.has(r.out)) throw new Error('said.txt: ' + r.name + ' twice');
      if (!r.cond) seen.add(r.out);
      rows.push({ c: r.name, b: [r.out, r.cf, r.mask, r.flags, ...r.bytes, 0], row: true });
    }
    rows.push({ c: K.short + ': end', b: [0] });
  }
  const pfirst = rows.length;
  rows.push(...pc);                                           // (The pieces after the kinds' rows)
  // The words most said, a code each (DICT's): the texts' letters between their codes, the word that saves the most
  // bytes chosen, then the next, as long as one saves any; each run of letters then made of them, the longest first
  const runs = (b, from) => {                                 // (Its runs of letters: [start, end])
    const out = [];
    let i = from, a = -1;
    for (; i < b.length; i++) {
      const c = b[i], letter = c === 0x0A || (c >= 0x20 && c < 0x7F);
      if (letter && a < 0) a = i;
      if (!letter && a >= 0) { out.push([a, i]); a = -1; }
      if ((c >= SC.ROOM && c <= SC.NAME) || c === SC.WORD) i += c === SC.TRAP ? 2 : 1;
    }
    if (a >= 0) out.push([a, i]);
    return out;
  };
  const texts = rows.filter(r => r.b.length > 1).map(r => ({ r, from: rows.indexOf(r) < pfirst ? 4 : 0 }));
  let segs = texts.flatMap(t => runs(t.r.b, t.from).map(([a, e]) => String.fromCharCode(...t.r.b.slice(a, e))));
  // (Then, the codes spent, more words said as SC_WORD and a byte, as long as one saves any: overlay 27's room, the
  // rows' bank's bytes; WORDS_MAX at most)
  const dict = [];
  while (dict.length < DICT.length + WORDS_MAX) {
    const cnt = new Map(), one = dict.length < DICT.length;
    for (const s of segs) for (let a = 0; a < s.length; a++) for (let l = 3; l <= 24 && a + l <= s.length; l++) {
      const w = s.slice(a, a + l);
      cnt.set(w, (cnt.get(w) || 0) + 1);
    }
    let best = null, bv = 0;
    for (const [w, c] of cnt) {
      const v = one ? c * (w.length - 1) - (w.length + 2) : c * (w.length - 2) - (w.length + 3);
      if (v > bv || (v === bv && best && w < best)) { bv = v; best = w; }
    }
    if (!best) break;
    dict.push(best);
    segs = segs.flatMap(s => s.split(best));
  }
  const order = dict.map((w, i) => i).sort((p, q) => dict[q].length - dict[p].length);
  for (const t of texts) {
    const b = t.r.b, out = [];
    let last = 0;
    for (const [a, e] of runs(b, t.from)) {
      out.push(...b.slice(last, a));
      const s = String.fromCharCode(...b.slice(a, e));
      for (let i = 0; i < s.length; ) {
        const k = order.find(j => s.startsWith(dict[j], i));
        if (k === undefined) { out.push(s.charCodeAt(i)); i++; continue; }
        out.push(...(k < DICT.length ? [DICT[k]] : [SC.WORD, k - DICT.length + 1]));
        i += dict[k].length;
      }
      last = e;
    }
    out.push(...b.slice(last));
    t.r.b = out;
  }
  for (const r of rows) if (r.row) {                          // (A row's length first: the next row's found at once)
    if (r.b.length + 1 > 255) throw new Error('saidgen: ' + r.c + ', past 254 bytes packed');
    r.b.unshift(r.b.length + 1);
  }
  let n = 0;
  const offs = rows.map(r => { const o = n; n += r.b.length; return o; });
  if (n > 0x2000) throw new Error('saidgen: the rows ' + n + ' bytes, past their bank\'s 8,192');
  let dn = 0;
  const dofs = dict.map(w => { const o = dn; dn += w.length + 1; return o; });

  const lines = ['/* dq0said.h - made by the build (games/dq0/saidgen.js) from games/dq0/said.txt: not to be edited.  The UI\'s',
    '** phrase table (said.c\'s, SAID_TABLE defined: its rows overlay 28\'s), and the names of its own outcomes (SAID_KIND_NAME) */',
    '/* key ' + key + ' */', '', '#ifndef _DQ0SAID_H', '#define _DQ0SAID_H', ''];
  for (const K of kinds) K.own.forEach((o, i) => lines.push(('#define SAID_' + K.short + '_' + o.toUpperCase()).padEnd(40) + (Math.max(...K.members.map(x => H.C[x])) + 1 + i)));
  lines.push(('#define SAID_NKINDS').padEnd(40) + kinds.length, ('#define SAID_ANY').padEnd(40) + ANY, '', '#ifdef SAID_TABLE',
    'static const unsigned char said_kind[SAID_NKINDS] = { ' + kinds.map(K => K.v + ' /* ' + K.k + ' */').join(', ') + ' };',
    'static const unsigned char said_ko[SAID_NKINDS] = { ' + kinds.map(K => field(K, K.of, 0) + ' /* ' + K.of + ' */').join(', ') + ' };',
    'static const unsigned said_at[SAID_NKINDS] = { ' + kinds.map((K, i) => offs[at[i]]).join(', ') + ' };',
    'static const unsigned said_pc[] = { ' + (pc.length ? pc.map((p, i) => offs[pfirst + i]).join(', ') : '0') + ' };',
    'static const unsigned said_do[] = { ' + (dict.length ? dofs.join(', ') : '0') + ' };',
    'static const char said_dw[] = ' + (dict.length ? dict.map(w => JSON.stringify(w + '\0').replace(/\\u0000/g, '\\0')).join(' ') : '""') + ';');
  const fl = lines.length;
  lines.push('static const unsigned char said_f[] = {');
  fields.forEach((d, i) => lines.push('    ' + d.map(hex).join(', ') + ',' + '    /* ' + i + ' */'));
  lines.push('};', '#pragma rodata-name (push, "OV28RODATA")      /* (Overlay 28\'s: the rows\' bank, read by said_b) */',
    'const unsigned char said_rows[] = {');
  for (const r of rows) {
    lines.push('    /* ' + r.c + ' */');
    for (let i = 0; i < r.b.length; i += 16) lines.push('    ' + r.b.slice(i, i + 16).map(hex).join(', ') + ',');
  }
  lines.push('};', '#pragma rodata-name (pop)', '#endif', '', '#endif', '');
  if (fields.length > 32) throw new Error('saidgen: more than 32 fields');
  void fl;
  fs.mkdirSync(path.dirname(OUT), { recursive: true });
  fs.writeFileSync(OUT, lines.join('\r\n'), 'latin1');
  say('dq0said.h: ' + kinds.reduce((s, K) => s + K.rows.length, 0) + ' rows of ' + kinds.length + ' kinds, ' + n + ' bytes (overlay 28), ' + pc.length + ' pieces, ' + fields.length + ' fields');
}

module.exports = { generate };
