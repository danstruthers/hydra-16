// ****************************************************************************
// tiers.js - the tiers of checks (tests/TESTING.md), each a list of steps run one after another, its output as it
// comes and one summary at the end: sim/test.js --tier NAME.
//   report   before an agent reports: the suite's tests what's changed since REF can affect (--changed: the game's
//            alone when only games/ changed; dq0answer at DQ0ANS_WAIT=0 among them), RULES=null's meaning tests of
//            them, and the parser's checks when the grammar's changed (tests/areas.js's GRAMMAR's paths)
//   lead     the lead's, once a round after all the merges: the whole suite (dq0answer at 0), RULES=null's meaning
//            tests, the soaks (soak.js -n 200; -n 100 -fight 0.8), dq0answer at DQ0ANS_WAIT=1 and 2, the parser's
// Usage: node sim/test.js --tier report|lead [--changed REF] [-j N] [--seed N] [--list]
//   --changed REF   report's REF (default: reborn-game, the merge base with it)
//   -j N, --seed N  each step's (the suite's, the soaks')
//   --list          the steps, and each suite step's --list (what it would run, and why), without running
'use strict';
const path = require('path');
const cp = require('child_process');

const ROOT = path.join(__dirname, '..');
const node = (...a) => [process.execPath, ...a];

// The parser's checks: the matcher on sim65 (its default), what each line means (--mean), the grammar and corpus
const PARSER = [
  { what: 'the parser: commandmatch-test.js', cmd: node('sim/tools/commandmatch-test.js') },
  { what: 'the parser: commandmatch-test.js --mean', cmd: node('sim/tools/commandmatch-test.js', '--mean') },
  { what: 'the parser: commandcorpus.js', cmd: node('sim/tools/commandcorpus.js') },
];

// Has the grammar changed since ref (areas.js's GRAMMAR's paths)?  OUT: [the paths] (null: not known: say so, run)
function grammarChanged(ref) {
  try {
    const sel = require('../../base/sim/select.js'), { GRAMMAR } = require('../tests/areas.js');
    return sel.changedFiles(sel.topOf(ROOT), ref).files.filter(f => GRAMMAR.some(re => re.test(f)));
  } catch (e) { return null; }
}

// A tier's steps.  o: { ref, pass: the suite's options passed on (-j, --seed), soak: the soaks' (-j) }
function steps(tier, o) {
  const suite = (what, env, args) => ({ what, env, cmd: node('sim/test.js', ...args, ...o.pass), suite: true });
  if (tier === 'report') {
    const g = grammarChanged(o.ref);
    return [
      suite('the suite: the tests what\'s changed since ' + o.ref + ' can affect', { RULES: '', DQ0ANS_WAIT: '0' }, ['--changed=' + o.ref]),
      suite('RULES=null: the meaning tests of them', { RULES: 'null', DQ0ANS_WAIT: '0' }, ['--changed=' + o.ref]),
      ...(g === null || g.length ? PARSER.map(s => Object.assign({}, s, { what: s.what + (g ? ' (' + g.length + ' grammar file(s) changed)' : ' (the grammar\'s changes not known)') }))
        : [{ what: 'the parser\'s: skipped, no grammar file changed since ' + o.ref, skip: true }]),
    ];
  }
  if (tier === 'lead') return [
    suite('the whole suite (dq0answer at DQ0ANS_WAIT=0)', { RULES: '', DQ0ANS_WAIT: '0' }, []),
    suite('RULES=null: the meaning tests', { RULES: 'null', DQ0ANS_WAIT: '0' }, []),
    { what: 'the soak: 200 seeds', env: { RULES: '' }, cmd: node('sim/tools/soak.js', '-n', '200', ...o.soak) },
    { what: 'the soak: 100 seeds, fights (-fight 0.8)', env: { RULES: '' }, cmd: node('sim/tools/soak.js', '-n', '100', '-fight', '0.8', ...o.soak) },
    suite('dq0answer at DQ0ANS_WAIT=1', { RULES: '', DQ0ANS_WAIT: '1' }, ['dq0answer']),
    suite('dq0answer at DQ0ANS_WAIT=2', { RULES: '', DQ0ANS_WAIT: '2' }, ['dq0answer']),
    ...PARSER,
  ];
  return null;
}

const mins = ms => Math.floor(ms / 60000) + 'm ' + String(Math.round(ms / 1000) % 60).padStart(2, '0') + 's';

// One step: its output passed on as it comes, its last line kept.  OUT: (a promise) { code, last, ms }
function run(s) {
  return new Promise(resolve => {
    const t0 = Date.now(), p = cp.spawn(s.cmd[0], s.cmd.slice(1), { cwd: ROOT, env: Object.assign({}, process.env, s.env || {}), stdio: ['ignore', 'pipe', 'inherit'] });
    let tail = '';
    p.stdout.setEncoding('utf8');
    p.stdout.on('data', d => { process.stdout.write(d); tail = (tail + d).slice(-4000); });
    p.on('close', code => {
      const lines = tail.split(/\r?\n/).filter(l => l.trim());
      resolve({ code, last: lines.length ? lines[lines.length - 1].trim() : '', ms: Date.now() - t0 });
    });
  });
}

const shown = s => Object.entries(s.env || {}).filter(([, v]) => v !== '').map(([k, v]) => k + '=' + v).join(' ') + (s.env && Object.values(s.env).some(v => v !== '') ? ' ' : '') +
  'node ' + s.cmd.slice(1).join(' ');

// sim/test.js --tier NAME [...]: the tier's steps, one after another; the summary.  argv: the options but --tier's
async function main(tier, argv) {
  const o = { ref: 'reborn-game', pass: [], soak: [], list: false };
  for (let i = 0; i < argv.length; i++) {
    if (argv[i] === '--changed') o.ref = argv[++i];
    else if (argv[i].startsWith('--changed=')) o.ref = argv[i].slice(10);
    else if (argv[i] === '-j') { o.pass.push('-j', argv[++i]); o.soak.push('-j', argv[i]); }
    else if (argv[i] === '--seed') o.pass.push('--seed', argv[++i]);
    else if (argv[i] === '--list') o.list = true;
    else { console.error('--tier ' + tier + ': not an option of a tier: ' + argv[i] + ' (node sim/test.js --tier report|lead [--changed REF] [-j N] [--seed N] [--list])'); process.exit(2); }
  }
  const list = steps(tier, o);
  if (!list) { console.error('--tier: report or lead, not ' + tier); process.exit(2); }
  if (o.list) {
    console.log('tier ' + tier + ': ' + list.length + ' steps');
    for (const s of list) {
      console.log('\n== ' + s.what + (s.skip ? '' : ': ' + shown(s)));
      if (s.suite) await run(Object.assign({}, s, { cmd: [...s.cmd, '--list'] }));
    }
    process.exit(0);
  }
  const t0 = Date.now(), done = [];
  for (const s of list) {
    if (s.skip) { done.push({ s, skip: true }); continue; }
    console.log('\n== tier ' + tier + ', step ' + (done.length + 1) + ' of ' + list.length + ': ' + s.what + '\n== ' + shown(s));
    done.push(Object.assign({ s }, await run(s)));
  }
  const bad = done.filter(d => !d.skip && d.code !== 0);
  console.log('\n== tier ' + tier + (tier === 'report' ? ' (since ' + o.ref + ')' : '') + ': ' + (bad.length ? bad.length + ' of ' + done.length + ' steps failed' : 'every step passed') + ', ' + mins(Date.now() - t0));
  for (const d of done)
    console.log('  ' + (d.skip ? 'SKIP' : d.code === 0 ? 'PASS' : 'FAIL') + '  ' + d.s.what + (d.skip ? '' : '  (' + mins(d.ms) + '): ' + d.last));
  process.exit(bad.length ? 1 : 0);
}

module.exports = { main, steps, PARSER };
