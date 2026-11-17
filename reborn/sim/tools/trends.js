#!/usr/bin/env node
// ****************************************************************************
// trends.js - the tests' cycle trends (tests/lib/trends.js; the user's, 2026-11-17: only the D17 targets, the RAM
// floors and a silent start fail the suite, every other cycle budget a trend, its rise past 5% said): each measure
// the last run kept (obj/trends/NAME.json: each test's, as it ran), its trend (tests/trends.json), how far from it.
// --update [NAME ...]: those tests' measures (or every test's the last runs kept) made their trends, on purpose: a
// rise taken as it is now, a measure new, a gain kept.  A D17 target's measure is a trend too, its rise said; its
// trend never lets it off its target (tests/lib/trends.js's target: over it, the test fails, since 2026-11-17).
//
// Usage: node sim/tools/trends.js [NAME ...]                    the measures and their trends
//        node sim/tools/trends.js --update [NAME ...]
'use strict';
const fs = require('fs');
const path = require('path');
const { FILE, DIR, RISE, trends } = require('../../tests/lib/trends.js');

const n = v => v === null || v === undefined ? '-' : Math.round(v).toLocaleString('en-US');

function lastRuns(names) {                                  // (Each test's record, the last run's that kept one)
  const out = {};
  if (!fs.existsSync(DIR)) return out;
  for (const f of fs.readdirSync(DIR).filter(x => x.endsWith('.json'))) {
    const name = f.replace(/\.json$/, '');
    if (names.length && !names.includes(name)) continue;
    try { out[name] = JSON.parse(fs.readFileSync(path.join(DIR, f), 'utf8')); } catch (e) { /* (Half written: none) */ }
  }
  return out;
}

function show(names) {
  const T = trends(), R = lastRuns(names);
  for (const test of [...new Set([...Object.keys(T), ...Object.keys(R)])].sort()) {
    if (names.length && !names.includes(test)) continue;
    const kept = T[test] || {}, now = (R[test] || {}).trends || {};
    console.log(test + ':');
    for (const name of [...new Set([...Object.keys(kept), ...Object.keys(now)])].sort()) {
      const a = kept[name], b = now[name], d = a && b !== undefined ? (b / a - 1) * 100 : null;
      console.log('  ' + name + ': ' + n(b) + ' (trend ' + n(a) + (d === null ? '' : ', ' + (d >= 0 ? '+' : '') + d.toFixed(1) + '%') + ')' + (d !== null && d > RISE * 100 ? '  ROSE' : ''));
    }
  }
}

function update(names) {
  const T = JSON.parse(JSON.stringify(trends())), R = lastRuns(names), said = [];
  if (names.length) for (const x of names) if (!R[x]) said.push(x + ': no measures kept (run it first)');
  for (const [test, r] of Object.entries(R)) {
    const t = T[test] = T[test] || {};
    for (const [name, v] of Object.entries(r.trends || {})) {
      const g = (r.targets || {})[name];
      if (g && g[0] !== null && g[0] > g[1]) said.push(test + ' ' + name + ': ' + n(v) + ' over its target ' + n(g[1]) + ' (a trend all the same: the test fails till it\'s won back)');
      if (t[name] !== v) said.push(test + ' ' + name + ': ' + n(t[name]) + ' -> ' + n(v));
      t[name] = v;
    }
  }
  const sorted = {};                                        // (By name, a test with none left out: a stable file)
  for (const test of Object.keys(T).sort()) {
    if (!Object.keys(T[test]).length) continue;
    sorted[test] = {};
    for (const name of Object.keys(T[test]).sort()) sorted[test][name] = T[test][name];
  }
  fs.writeFileSync(FILE, JSON.stringify(sorted, null, 2).replace(/\n/g, '\r\n') + '\r\n');
  console.log(said.length ? said.join('\n') : 'no change');
  console.log(path.relative(process.cwd(), FILE) + ': ' + Object.keys(sorted).length + ' tests, ' + Object.values(sorted).reduce((a, x) => a + Object.keys(x).length, 0) + ' measures');
}

if (require.main === module) {
  const argv = process.argv.slice(2), names = argv.filter(a => !a.startsWith('--'));
  if (argv.includes('--update')) update(names);
  else show(names);
}
module.exports = { show, update };
