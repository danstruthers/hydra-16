#!/usr/bin/env node
// ****************************************************************************
// sentdiff.js - two runs' transcripts compared sentence by sentence (sim/test.js's TRANSCRIPTS=DIR: each test's screen
// in DIR/NAME.txt), so a change's every difference to what the player reads is seen, and the known noise told apart
// (tests/noise.txt: the tests whose words move with the tasks' timing, and the sentences that do in any test, each with
// its reason).  A transcript's text: what the serial line drew, the status row and the typing row left out, wrapped
// lines joined, cut into sentences; then each sentence counted, so that order doesn't matter (the timing noise moves
// whole sentences).  Each test whose sentences differ: its sentences only in one run, with their counts; a test of the
// noise list's, or a difference only in the noise list's sentences, marked as known noise.  A test listed with a
// reason that begins "@real" (lock1's, 2026-11-24: the lockstep turns pace) is noise only in its games at a real pace:
// its games at the turns pace (a game from its title, "D I N G U S ...", to the next; its pace its dq0 line's, "-p t",
// those lines in the raw output in order, one a game) are held exactly, and a difference there is past the noise.
//
// Usage: node sim/tools/sentdiff.js BEFORE AFTER [NAME ...] [--noise FILE] [--quiet]
//   BEFORE, AFTER  two TRANSCRIPTS folders (a test missing from AFTER said; one only in AFTER, new)
//   NAME     only these tests
//   --noise FILE   the noise list (default: tests/noise.txt)
//   --quiet  the known noise's differences counted, not shown
// The exit code: 1 if a test differs past the known noise, else 0.
'use strict';
const fs = require('fs');
const path = require('path');

const NOISE = path.join(__dirname, '..', '..', 'tests', 'noise.txt');

// The noise list: { tests: Map(name -> why), sentences: Map(sentence -> why) } (a line: NAME and its reason, or
// "A SENTENCE." and its reason; # a comment)
function noiseList(file) {
  const tests = new Map(), sentences = new Map();
  if (!file || !fs.existsSync(file)) return { tests, sentences };
  for (const l of fs.readFileSync(file, 'latin1').split(/\r?\n/)) {
    if (!l.trim() || /^\s*#/.test(l)) continue;
    let m;
    if ((m = /^\s*"([^"]+)"\s+(.*)$/.exec(l))) sentences.set(m[1], m[2].trim());
    else if ((m = /^\s*(\S+)\s+(.*)$/.exec(l))) tests.set(m[1], (tests.has(m[1]) ? tests.get(m[1]) + '; ' : '') + m[2].trim());
  }
  return { tests, sentences };
}

// A transcript's bytes as text: the serial line's bytes (sim/test.js writes them as they came); one written as UTF-8
// (the old hook's) read as such
function read(file) {
  const b = fs.readFileSync(file);
  if (b.some(x => x >= 0x80)) try { return new TextDecoder('utf-8', { fatal: true }).decode(b); } catch (e) { /* (Not UTF-8: bytes) */ }
  return b.toString('latin1');
}

// The screen's text: the status row (1) and the typing row (24) left out, a new line at each cursor move
function text(s) {
  let row = 0, o = '';
  const re = /\x1b\[([0-9;?]*)([A-Za-z])|\x1b.|([^\x1b]+)/g;
  let m;
  while ((m = re.exec(s))) {
    if (m[3] !== undefined) { if (row !== 1 && row !== 24) o += m[3]; }
    else if (m[2] === 'H') { row = parseInt(m[1]) || 1; o += '\n'; }
  }
  return o.replace(/\r/g, '');
}
// Its sentences: a line typed whole (> or %), else cut at . ! ? : (a progress dot glued to a sentence's end, "a
// sconce..", the sentence's own)
function sentences(s) {
  const lines = text(s).split('\n').map(l => l.replace(/\s+$/, '')).filter(l => l.length && !/^ ?\.+$/.test(l) && !/^  /.test(l));
  const out = [];
  for (const l of lines) {
    if (/^[>%]/.test(l)) { out.push(l); continue; }
    for (const x of l.replace(/ \.{3,}/g, ' ...').split(/(?<=[.!?:])\s+/)) if (x.trim()) out.push(x.trim().replace(/([^.])\.\.$/, '$1.'));
  }
  return out;
}
// Its sentences in its games at the turns pace alone, and those before its first game (null: its games and their dq0
// lines not one for one, every game taken as a real pace's)
function turnsOnly(raw) {
  const all = sentences(raw), lines = raw.match(/(?<![\w\/.])dq0(?= |\r)[^\x1b\r\n]*/g) || [], out = [];
  const starts = all.map((x, i) => /^D I N G U S/.test(x) ? i : -1).filter(i => i >= 0);
  if (starts.length !== lines.length) return null;
  out.push(...all.slice(0, starts.length ? starts[0] : all.length));
  starts.forEach((a, k) => { if (/ -p t\b/.test(lines[k])) out.push(...all.slice(a, k + 1 < starts.length ? starts[k + 1] : all.length)); });
  return out;
}
const count = a => { const m = new Map(); for (const x of a) m.set(x, (m.get(x) || 0) + 1); return m; };

// Two transcripts' sentences that differ: [{ s, a: count before, b: count after, noise: its reason or null }]
function diff(a, b, noise) {
  const x = count(sentences(a)), y = count(sentences(b)), d = [];
  for (const [k, v] of x) if ((y.get(k) || 0) !== v) d.push({ s: k, a: v, b: y.get(k) || 0, noise: noise.sentences.get(k) || null });
  for (const [k, v] of y) if (!x.has(k)) d.push({ s: k, a: 0, b: v, noise: noise.sentences.get(k) || null });
  return d;
}

function main(argv) {
  const opt = { dirs: [], names: [], noise: NOISE, quiet: false };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--noise') opt.noise = argv[++i];
    else if (argv[i] === '--quiet') opt.quiet = true;
    else if (opt.dirs.length < 2) opt.dirs.push(argv[i]);
    else opt.names.push(argv[i]);
  }
  if (opt.dirs.length < 2) { console.error('usage: node sim/tools/sentdiff.js BEFORE AFTER [NAME ...] [--noise FILE] [--quiet]'); process.exit(2); }
  const [A, B] = opt.dirs, noise = noiseList(opt.noise), want = n => !opt.names.length || opt.names.includes(n);
  const names = [...new Set([...fs.readdirSync(A), ...fs.readdirSync(B)].filter(f => f.endsWith('.txt')).map(f => f.slice(0, -4)))].filter(want).sort();
  let real = 0, known = 0;
  const realNames = [];
  for (const name of names) {
    const a = path.join(A, name + '.txt'), b = path.join(B, name + '.txt');
    if (!fs.existsSync(b)) { console.log('MISSING ' + name + ' (not in ' + B + ')'); real++; realNames.push(name); continue; }
    if (!fs.existsSync(a)) { console.log('NEW ' + name + ' (not in ' + A + ')'); continue; }
    const ra = read(a), rb = read(b), d = diff(ra, rb, noise);
    if (!d.length) continue;
    let why = noise.tests.get(name);
    if (why && /^@real\b/.test(why)) {         // (Noise in its real-pace games alone: its turns-pace games held exactly)
      const ta = turnsOnly(ra), tb = turnsOnly(rb);
      const x = ta && tb ? count(ta) : null, y = ta && tb ? count(tb) : null;
      const same = x && y && x.size === y.size && [...x].every(([k, v]) => y.get(k) === v);
      if (!same) why = null;                    // (Its turns-pace games differ, or its games couldn't be told apart)
    }
    const allNoise = !!why || d.every(e => e.noise);
    if (allNoise) known++; else { real++; realNames.push(name); }
    console.log('== ' + name + (why ? '  (known noise: ' + why + ')' : allNoise ? '  (known noise: its sentences\')' : ''));
    if (allNoise && opt.quiet) { console.log('   ' + d.length + ' sentence(s)'); continue; }
    for (const e of d) console.log((e.a > e.b ? '- ' : '+ ') + e.s + ' (' + e.a + ' -> ' + e.b + ')' + (e.noise && !why ? '  (noise: ' + e.noise + ')' : ''));
  }
  console.log((real + known) + ' differ: ' + real + ' past the known noise' + (realNames.length ? ' (' + realNames.join(', ') + ')' : '') + ', ' + known + ' known noise');
  return real ? 1 : 0;
}

if (require.main === module) process.exit(main(process.argv.slice(2)));
module.exports = { noiseList, sentences, diff, read, main, turnsOnly };
