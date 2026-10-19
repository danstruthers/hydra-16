// commandmatch-test.js - Dingus Quest Zero's command matcher (games/dq0/cmdmatch.s, 65C02 assembly) against the
// host's results on the corpus (commandcompile.js's, as commandmatch-gen.js writes them):
//   node sim/tools/commandmatch-test.js           on sim65, cc65's 6502 simulator (cmdmatch/cmdtest.c): each command's
//                                                 result and cycles, the slowest, cmd_init's and cmd_world's
//   node sim/tools/commandmatch-test.js --hydra   on the Hydra emulator under HydraOS (cmdmatch/cmdhy.c): the grammar
//                                                 in a RAM bank, the index and the aliases in the main RAM, the corpus
//                                                 read from /pc, timed by the tick (the system's interrupts with it)
// Its files go in obj/cmdmatch.  Exit status 0 if every command agrees.
'use strict';
const fs = require('fs');
const path = require('path');
const cp = require('child_process');
const { tool } = require('../../../base/build.js');

const REB = path.join(__dirname, '..', '..');
const OUT = path.join(REB, 'obj', 'cmdmatch');
const GAME = path.join(REB, 'games', 'dq0');
const HERE = path.join(__dirname, 'cmdmatch');
const run = (exe, args, opt = {}) => cp.execFileSync(exe, args, Object.assign({ encoding: 'utf8', maxBuffer: 1 << 26 }, opt));

// The test data, and the corpus from it: each command, its world, and the host's result
fs.mkdirSync(OUT, { recursive: true });
run(process.execPath, [path.join(__dirname, 'commandmatch-gen.js'), OUT]);
const h = fs.readFileSync(path.join(OUT, 'cmd_corpus.h'), 'utf8');
const block = name => { const a = h.indexOf(' ' + name + '['), b = h.indexOf('{', a); return h.slice(b + 1, h.indexOf('};', b)); };
const strs = name => block(name).split('\n').map(s => s.trim()).filter(s => s.startsWith('"')).map(s => JSON.parse(s.replace(/,$/, '')));
const text = strs('cmd_text'), world = strs('cmd_case_world'), names = strs('cmd_case_names'), deities = strs('cmd_case_deities');
const ok = block('cmd_ok').split(',').map(s => s.trim()).filter(Boolean).map(Number);

function sim65() {
  const bin = path.join(OUT, 'cmdtest.bin');
  run(tool('cl65'), ['--cpu', '65C02', '-t', 'sim65c02', '-O', '-I', OUT, '-I', GAME, '-o', bin,
    path.join(HERE, 'cmdtest.c'), path.join(GAME, 'cmdmatch.s')], { cwd: OUT, stdio: ['ignore', 'pipe', 'pipe'] });   // (Its warnings: the test data's unused arrays)
  const out = run(tool('sim65'), ['--cpu', '65C02', '-x', '4000000000', bin], { cwd: OUT }).split(/\r?\n/);
  let init = 0, worlds = [], bad = 0;
  const each = [];
  for (const l of out) {
    let m;
    if ((m = l.match(/^init (\d+)$/))) init = +m[1];
    else if ((m = l.match(/^world (\d+)$/))) worlds.push(+m[1]);
    else if ((m = l.match(/^(\d+) (-?\d+) (\d+)$/))) {
      const i = +m[1], r = +m[2];
      each.push({ i, c: +m[3] });
      if (r !== ok[i]) { bad++; console.log('differs: ' + text[i] + ' (' + r + ', the host ' + ok[i] + ')'); }
    } else if (/not right/.test(l)) console.log(l);
  }
  if (each.length !== text.length) { console.log('only ' + each.length + ' of ' + text.length + ' commands ran'); return false; }
  const sum = each.reduce((a, e) => a + e.c, 0);
  console.log(each.length + ' commands, ' + bad + ' differ from the host');
  console.log('cycles: a command ' + Math.round(sum / each.length) + ' on average; cmd_init ' + init + '; cmd_world ' +
    Math.round(worlds.reduce((a, b) => a + b, 0) / Math.max(1, worlds.length)) + ' (' + worlds.length + ' times)');
  console.log('the slowest:');
  for (const e of [...each].sort((a, b) => b.c - a.c).slice(0, 5)) console.log('  ' + String(e.c).padStart(7) + '  ' + text[e.i]);
  return bad === 0;
}

function hydra() {
  const pc = path.join(OUT, 'pc'), prog = path.join(OUT, 'cmdhy');
  fs.mkdirSync(pc, { recursive: true });
  fs.mkdirSync(prog, { recursive: true });
  run(process.execPath, [path.join(__dirname, 'commandtables.js'), path.join(pc, 'grammar.bin'), path.join(pc, 'aliases.bin'), path.join(pc, 'index.bin')]);
  const lines = [];
  let w, n, d;
  text.forEach((t, i) => {
    if (world[i] !== w) lines.push('W ' + (w = world[i]));
    if (names[i] !== n) lines.push('N ' + (n = names[i]));
    if (deities[i] !== d) lines.push('D ' + (d = deities[i]));
    lines.push('C ' + ok[i] + ' ' + t);
  });
  fs.writeFileSync(path.join(pc, 'corpus.txt'), lines.join('\n') + '\n');
  fs.copyFileSync(path.join(HERE, 'cmdhy.c'), path.join(prog, 'cmdhy.c'));
  for (const f of ['cmdmatch.s', 'cmdmatch.h']) fs.copyFileSync(path.join(GAME, f), path.join(prog, f));
  run(process.execPath, ['build.js', 'prog', prog], { cwd: REB });
  fs.copyFileSync(path.join(prog, 'cmdhy.hyx'), path.join(pc, 'cmdhy'));
  const B = String.fromCharCode(92), keys = B + 'w' + B + 'w' + B + 'w/pc/cmdhy' + B + 'r';   // (Typed once the shell is up)
  const r = cp.spawnSync(process.execPath, ['sim/run.js', '--pc-dir', pc, '--input', keys, '--cycles', '900000000'],
    { cwd: REB, encoding: 'utf8', maxBuffer: 1 << 26 });
  const o = r.stdout || '', at = o.indexOf('/pc/cmdhy'), end = o.indexOf('\n/>', at);
  const said = o.slice(at, end < 0 ? at + 4000 : end).split('\n').slice(1).filter(l => /^[ -~]*$/.test(l));
  console.log(said.join('\n') || '(nothing: ' + (o.split('\n').find(l => /stopped/.test(l)) || 'no run') + ')');
  return said.some(l => / 0 differ$/.test(l));
}

process.exit((process.argv.includes('--hydra') ? hydra() : sim65()) ? 0 : 1);
