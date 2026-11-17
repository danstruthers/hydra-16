// ****************************************************************************
// trends.js - the tests' cycle measures as trends, not budgets (the user's, 2026-11-17: DECISIONS.md's "Budgets",
// GAME-CRITIQUE.md's 6.3 and E9): only the user's targets (D17: a command's first letter within 720,000 cycles, a look
// within 1.79M), the RAM floors and a silent start fail the suite.  Every other cycle budget is a trend:
//   trend(t, name, value)       the measure kept (obj/trends/NAME.json, this run's) and held to tests/trends.json's
//                               value for it: a rise of more than RISE over it said (a note: "trend ..."), never a
//                               failure.  OUT: the note, or null
//   target(t, name, value, max, notes)  a D17 target's measure kept the same way: true if it's over its target (the
//                               test fails), unless its trend (the same name's, tests/trends.json's) was over it
//                               already when the budgets became trends: then said in notes, not failed (the user's
//                               targets not met yet: GAME-CRITIQUE.md's 6.3; met and kept as a trend, it's held to
//                               them from then on)
// A test's measures are named by the test (its line, its pace ...).  The file is made again only on purpose, from the
// last run's measures: node sim/tools/trends.js --update [NAME ...] (sim/tools/trends.js: the measures and their
// trends, each rise).  TRENDS_RUN (sim/test.js's: the run's start) marks a run's records, for its summary.
'use strict';
const fs = require('fs');
const path = require('path');

const ROOT = path.join(__dirname, '..', '..');
const FILE = path.join(ROOT, 'tests', 'trends.json'), DIR = path.join(ROOT, 'obj', 'trends');
const RISE = 0.05, HEADROOM = 0.9;
const D17 = { first: 720000, look: 1790000 };                 // (The user's targets: a command's first letter, a look's)

let kept = null;
const trends = () => { if (!kept) { try { kept = JSON.parse(fs.readFileSync(FILE, 'utf8')); } catch (e) { kept = {}; } } return kept; };
const n = v => Math.round(v).toLocaleString('en-US');

// A test's record of this run (obj/trends/NAME.json): { run, trends: { name: value }, targets: { name: [value, max] } }
const recs = new Map();
function record(t) {
  const run = process.env.TRENDS_RUN || '';
  let r = recs.get(t.name);
  if (!r || r.run !== run) recs.set(t.name, r = { run, trends: {}, targets: {} });
  return r;
}
function save(t, r) {
  try { fs.mkdirSync(DIR, { recursive: true }); fs.writeFileSync(path.join(DIR, t.name + '.json'), JSON.stringify(r)); } catch (e) { /* (Not kept) */ }
}

function trend(t, name, value) {
  const r = record(t);
  if (value === null || value === undefined || !isFinite(value)) return null;
  r.trends[name] = Math.round(value);
  save(t, r);
  const was = (trends()[t.name] || {})[name];
  if (was === undefined) return 'trend ' + name + ': ' + n(value) + ' cycles (none kept: tests/trends.json)';
  return value > was * (1 + RISE) ? 'trend ' + name + ': ' + n(value) + ' cycles, ' + ((value / was - 1) * 100).toFixed(1) + '% over its ' + n(was) : null;
}

function target(t, name, value, max, notes) {
  const r = record(t), v = value === null || value === undefined ? null : Math.round(value);
  const was = r.targets[name];
  if (!was || v === null || (was[0] !== null && v > was[0])) r.targets[name] = [v, max];   // (A name met twice: its worst)
  save(t, r);
  if (v !== null && v <= max) return false;
  const kept = (trends()[t.name] || {})[name];
  if (v === null || !(kept > max)) return true;
  if (notes) notes.push(name + ': ' + n(v) + ' cycles, over the user\'s ' + n(max) + ' (its trend ' + n(kept) + ' over it already: not failed)');
  return false;
}

// A run's records (sim/test.js's summary): each test's that this run made
function records(names) {
  const run = process.env.TRENDS_RUN || '', out = {};
  for (const name of names) {
    try { const r = JSON.parse(fs.readFileSync(path.join(DIR, name + '.json'), 'utf8')); if (r.run === run) out[name] = r; } catch (e) { /* (None) */ }
  }
  return out;
}

// After the tests: the trends that rose past RISE
function summary(names, say = console.log) {
  const R = records(names), T = trends(), rose = [];
  for (const [test, r] of Object.entries(R)) {
    for (const [name, v] of Object.entries(r.trends)) {
      const was = (T[test] || {})[name];
      if (was === undefined) rose.push(test + ' ' + name + ': ' + n(v) + ' (none kept)');
      else if (v > was * (1 + RISE)) rose.push(test + ' ' + name + ': ' + n(v) + ', ' + ((v / was - 1) * 100).toFixed(1) + '% over its ' + n(was));
    }
  }
  if (rose.length) say('trends: ' + rose.length + ' measure(s) more than ' + RISE * 100 + '% over their trend (tests/trends.json; to take one as it is now: node sim/tools/trends.js --update NAME):\n  ' + rose.join('\n  '));
}

module.exports = { trend, target, records, summary, trends, D17, RISE, HEADROOM, FILE, DIR };
