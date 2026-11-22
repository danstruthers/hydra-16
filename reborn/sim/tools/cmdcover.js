#!/usr/bin/env node
// ****************************************************************************
// cmdcover.js - Dingus Quest Zero's commands covered by the tests (savecrash's, 2026-11-19: "save" went untested four
// weeks, the suite's only "save" the quit's): every line the tests type in a game (between a "dq0 ..." line and its
// quit, from each test's machine input as tests/tests.js has it; a test's own lines typed from its start() not seen),
// made a command as the game's parser makes it (sim/tools/commandwords.js's meaning, the host's reference), and each
// action of the game's (actions.h's VB_*, commands.actions) and each verb of the grammar (commands.verbs) counted by
// the tests that type one of it.  Those no test types are listed: dq0cover (tests/suites/dq0/parser.js) types each.
//   node sim/tools/cmdcover.js [--lines]     the actions and verbs no test types; --lines: each line's action too
// From Node: cover() -> { actions: Map(VB_* -> [tests]), verbs: Map(verb -> [tests]), lines: [{ test, line, action, verb }] }
'use strict';
const fs = require('fs');
const path = require('path');
const cw = require('./commandwords.js');

const GAME = path.join(__dirname, '..', '..', 'games', 'dq0');

// The lines a machine's input types (ā: a prompt waited for; Ā: a pause; Ă/ă: the keyboard's on and off)
const typedLines = input => input.replace(/[Ā-ă]/g, '').split(/\r/).map(l => l.trim()).filter(Boolean);

// The verbs of commands.verbs: { name, words: [the words that say it, its [words]'s] }
function verbs() {
  const txt = fs.readFileSync(path.join(GAME, 'commands.verbs'), 'latin1').split(/\r?\n/);
  const out = new Map();
  let sec = '';
  for (const l of txt) {
    const s = l.replace(/#.*/, '').trim();
    if (!s) continue;
    const m = /^\[(\w+)\]$/.exec(s);
    if (m) { sec = m[1]; continue; }
    if (sec === 'verbs') { const name = s.split(/\s+/)[0]; if (!out.has(name)) out.set(name, []); }
    else if (sec === 'words') { const [w, v] = s.split('=').map(x => x.trim()); if (v && out.has(v)) out.get(v).push(w); }
  }
  return out;
}

function cover() {
  const { tests } = require('../../tests/tests.js');
  const w = cw.load(), V = cw.names(), vs = verbs();
  const actions = new Map(Object.keys(V).filter(k => /^VB_/.test(k) && k !== 'VB_NONE' && !/^VB_(FIRST|LAST|COUNT|MAX)/.test(k)).map(k => [k, []]));
  const verbsC = new Map([...vs.keys()].map(k => [k, []]));
  const byWord = new Map();
  for (const [v, ws] of vs) for (const x of ws) byWord.set(x, v);
  const lines = [];
  for (const t of tests) {
    if (!/^dq0/.test(t.name) || !t.machine || typeof t.machine.input !== 'string') continue;
    let inGame = false;
    for (const l of typedLines(t.machine.input)) {
      if (/^dq0(\s|$)/.test(l)) { inGame = true; continue; }
      if (!inGame) continue;
      if (/^(quit|q|exit)$/.test(l)) { inGame = false; continue; }
      let c;
      try { c = cw.meaning(l.toLowerCase(), w); } catch (e) { continue; }
      const action = cw.actionName(c.action), first = l.toLowerCase().split(/\s+/);
      const verb = byWord.get(first.slice(0, 3).join(' ')) || byWord.get(first.slice(0, 2).join(' ')) || byWord.get(first[0]) || '';
      lines.push({ test: t.name, line: l, action, verb });
      if (actions.has(action) && !actions.get(action).includes(t.name)) actions.get(action).push(t.name);
      if (verb && verbsC.has(verb) && !verbsC.get(verb).includes(t.name)) verbsC.get(verb).push(t.name);
    }
  }
  return { actions, verbs: verbsC, lines };
}

module.exports = { cover, typedLines };

if (require.main === module) {
  const r = cover();
  if (process.argv.includes('--lines')) for (const x of r.lines) console.log(x.test + '\t' + x.action + '\t' + x.verb + '\t' + x.line);
  const noA = [...r.actions].filter(([, t]) => !t.length).map(([k]) => k), noV = [...r.verbs].filter(([, t]) => !t.length).map(([k]) => k);
  console.log(r.lines.length + ' lines typed in games; ' + (r.actions.size - noA.length) + ' of ' + r.actions.size + ' actions typed, ' +
    (r.verbs.size - noV.length) + ' of ' + r.verbs.size + ' verbs');
  console.log('actions no test types (' + noA.length + '): ' + noA.join(' '));
  console.log('verbs no test types (' + noV.length + '): ' + noV.join(' '));
}
