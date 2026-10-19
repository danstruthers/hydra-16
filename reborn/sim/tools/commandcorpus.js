// commandcorpus.js - checks Dingus Quest Zero's command grammar and its test corpus (games/dq0/commands.*).
// It doesn't parse commands yet: it checks that the grammar is well formed (every nonterminal defined or open, rules
// with balanced brackets), that the corpus is well formed (five fields a line, known actions and result codes), and that
// every action in the catalogue has at least one case. Run: node sim/tools/commandcorpus.js  (from reborn/).
'use strict';
const fs = require('fs');
const path = require('path');

const dir = path.join(__dirname, '..', '..', 'games', 'dq0');
const read = f => fs.readFileSync(path.join(dir, f), 'utf8').replace(/\r\n/g, '\n');
const errors = [], warnings = [];
const fail = (f, n, msg) => errors.push(`${f}${n ? ':' + n : ''}: ${msg}`);

// ---- the grammar ----
const ebnf = read('commands.ebnf');
const open = new Set();
const openLine = ebnf.match(/open:\s*([^\n*]*)/);
if (!openLine) fail('commands.ebnf', 0, 'no "open:" list in the header comment');
else for (const w of openLine[1].trim().split(/\s+/)) if (w) open.add(w);

const text = ebnf.replace(/\(\*[\s\S]*?\*\)/g, ' ');          // drop comments (they may span lines)
const rules = new Map();                                        // name -> body text
let cur = null;
text.split('\n').forEach((line, i) => {
  const m = line.match(/^\s*<([^>]+)>\s*::=(.*)$/);
  if (m) {
    if (rules.has(m[1])) fail('commands.ebnf', i + 1, `rule <${m[1]}> defined twice`);
    cur = m[1];
    rules.set(cur, m[2]);
  } else if (cur && line.trim() && (/^\s/.test(line))) {
    rules.set(cur, rules.get(cur) + ' ' + line);             // a continuation line
  } else if (!line.trim()) {
    cur = null;
  }
});
const defined = new Set(rules.keys());
for (const [name, body] of rules) {
  const bare = body.replace(/'[^']*'/g, ' ').replace(/"[^"]*"/g, ' ');
  for (const [open_, close] of [['(', ')'], ['[', ']'], ['{', '}']]) {
    const a = bare.split(open_).length - 1, b = bare.split(close).length - 1;
    if (a !== b) fail('commands.ebnf', 0, `<${name}>: unbalanced '${open_}${close}' (${a} open, ${b} close)`);
  }
  for (const m of bare.matchAll(/<([^>]+)>/g)) {
    if (!defined.has(m[1]) && !open.has(m[1])) fail('commands.ebnf', 0, `<${name}> uses undefined <${m[1]}>`);
  }
}
const used = new Set();
for (const body of rules.values()) for (const m of body.matchAll(/<([^>]+)>/g)) used.add(m[1]);
for (const name of defined) if (!used.has(name) && name !== 'line') warnings.push(`commands.ebnf: <${name}> is defined but never used`);

// ---- the catalogue ----
const actionsText = read('commands.actions');
const actions = new Set(), results = new Set();
let section = null;
actionsText.split('\n').forEach((line, i) => {
  const t = line.replace(/#.*$/, '').trim();
  if (!t) return;
  if (t === '[actions]' || t === '[results]') { section = t; return; }
  if (section === '[actions]') {
    const name = t.split(/\s+/)[0];
    if (!/^VB_[A-Z_]+$/.test(name)) fail('commands.actions', i + 1, `bad action name ${name}`);
    if (actions.has(name)) fail('commands.actions', i + 1, `action ${name} listed twice`);
    actions.add(name);
  } else if (section === '[results]') {
    if (!/^(ERROR|QUESTION|ALIAS):[A-Z_]+$/.test(t)) fail('commands.actions', i + 1, `bad result code ${t}`);
    results.add(t);
  } else fail('commands.actions', i + 1, 'line before a [section]');
});

// ---- the corpus ----
const corpus = read('commands.corpus');
const covered = new Set(), usedResults = new Set();
let cases = 0;
corpus.split('\n').forEach((line, i) => {
  if (!line.trim() || line.startsWith('#')) return;
  const f = line.split('\t');
  if (f.length !== 5) { fail('commands.corpus', i + 1, `${f.length} fields, want 5 (input, expected, roles, context, note)`); return; }
  const [input, expect, roles, ctx] = f;
  cases++;
  if (!input.trim()) fail('commands.corpus', i + 1, 'empty input');
  if (expect.startsWith('VB_')) {
    if (!actions.has(expect)) fail('commands.corpus', i + 1, `unknown action ${expect}`);
    else covered.add(expect);
  } else if (/^(ERROR|QUESTION|ALIAS):/.test(expect)) {
    if (!results.has(expect)) fail('commands.corpus', i + 1, `unknown result code ${expect}`);
    else usedResults.add(expect);
  } else fail('commands.corpus', i + 1, `expected "${expect}" is neither an action nor a result code`);
  for (const [label, v] of [['roles', roles], ['context', ctx]]) {
    if (v === '-') continue;
    for (const pair of v.split(';')) {
      if (!/^[A-Za-z_-]+=\S/.test(pair) && !(label === 'roles' && /^[A-Za-z_-]+=/.test(pair)))
        fail('commands.corpus', i + 1, `${label} "${pair}" is not key=value`);
    }
  }
});
for (const a of actions) if (!covered.has(a)) fail('commands.actions', 0, `${a} has no corpus case`);
for (const r of results) if (!usedResults.has(r)) warnings.push(`commands.actions: result ${r} has no corpus case`);

// ---- report ----
console.log(`grammar: ${rules.size} rules, ${open.size} open names`);
console.log(`catalogue: ${actions.size} actions, ${results.size} result codes`);
console.log(`corpus: ${cases} cases, ${covered.size}/${actions.size} actions covered`);
for (const w of warnings) console.log('warning: ' + w);
for (const e of errors) console.log('error: ' + e);
console.log(errors.length ? `FAILED: ${errors.length} error(s)` : 'ok');
process.exit(errors.length ? 1 : 0);
