// ****************************************************************************
// areas.js - the base's tests' areas, for sim/test.js's --changed (sim/select.js: a path's rule, the first that
// matches, and the areas it affects).  The base's own: its tests file by entries, a test module by the tests that
// boot it (mod:NAME: their init or modules), its README none; the rest of it (the kernel, the modules, the emulator,
// the tools) every test.  A system on the base (reborn/tests/areas.js) puts these after its own.
'use strict';

const rules = [
  ['base/tests/tests.js', ['entries', 'tiers']],
  ['base/tests/areas.js', ['*']],
  [/^base\/tests\/mod\/([^/]+)\//, ['mod:$1']],
  [/^base\/(README\.md|.*\.md)$/, ['none']],
  ['base/bin/', ['none']],                                    // (The build's images, from the sources: theirs the rules)
  ['base/', ['*']],
];

// A test's areas: its modules' (mod:NAME), the base's
function areasOf(t) {
  const a = new Set(['base']);
  for (const m of [t.init, ...(t.modules || [])]) if (m) a.add('mod:' + m);
  for (const x of [].concat(t.area || [])) a.add(x);
  return a;
}

module.exports = { rules, areasOf };
