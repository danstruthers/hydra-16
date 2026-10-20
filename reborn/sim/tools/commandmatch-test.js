// commandmatch-test.js - Dingus Quest Zero's command matcher (games/dq0c/cmdmatch.s, 65C02 assembly) against the
// host's results on the corpus (commandcompile.js's, as commandmatch-gen.js writes them):
//   node sim/tools/commandmatch-test.js           on sim65, cc65's 6502 simulator (cmdmatch/cmdtest.c): each command's
//                                                 result and cycles, the slowest, cmd_init's and cmd_world's
//   node sim/tools/commandmatch-test.js --hydra   on the Hydra emulator under HydraOS (cmdmatch/cmdhy.c): the grammar
//                                                 in a RAM bank, the index and the aliases in the main RAM, the corpus
//                                                 read from /pc, timed by the tick (the system's interrupts with it)
//   node sim/tools/commandmatch-test.js --mean    what each line means (games/dq0c/cmdmean.s, on sim65: cmdmean.c), the
//                                                 corpus's lines and cmdmatch/lines.txt's, each struct cmd the host's
//                                                 (commandwords.js's scan), the character facing the line's number mod 8
//   node sim/tools/commandmatch-test.js --task    the parser's task (games/dq0c) under HydraOS, through the hub's
//                                                 protocol (cmdmatch/cmdtask.c the UI's part): its start, an empty
//                                                 request's round trip, a command's, each result the host's
// Its files go in obj/cmdmatch.  Exit status 0 if every command agrees.
'use strict';
const fs = require('fs');
const path = require('path');
const cp = require('child_process');
const { tool } = require('../../../base/build.js');

const REB = path.join(__dirname, '..', '..');
const OUT = path.join(REB, 'obj', 'cmdmatch');
const GAME = path.join(REB, 'games', 'dq0c');
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
  run(tool('cl65'), ['--cpu', '65C02', '-t', 'sim65c02', '-O', '-I', OUT, '-I', HERE, '-o', bin,
    path.join(HERE, 'cmdtest.c'), path.join(GAME, 'cmdmatch.s'), path.join(HERE, 'cmdmatch_c.s')], { cwd: OUT, stdio: ['ignore', 'pipe', 'pipe'] });   // (Its warnings: the test data's unused arrays)
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

// What each line means on sim65 (cmdmean.c), against the host's (commandwords.js's scan, the default aliases)
function mean() {
  const words = require('./commandwords.js');
  const { DEFAULT_ALIASES, DEFAULT_ANYWHERE } = require('./commandcompile.js');
  const lines = require('./commandmatch-gen.js').meaningLines(), w = words.load();
  const bin = path.join(OUT, 'cmdmean.bin');
  run(tool('cl65'), ['--cpu', '65C02', '-t', 'sim65c02', '-O', '-I', OUT, '-I', HERE, '-o', bin, path.join(HERE, 'cmdmean.c'),
    path.join(GAME, 'cmdmatch.s'), path.join(GAME, 'cmdmean.s'), path.join(HERE, 'cmdmatch_c.s'), path.join(HERE, 'cmdmean_c.s')],
    { cwd: OUT, stdio: ['ignore', 'pipe', 'pipe'] });
  const out = run(tool('sim65'), ['--cpu', '65C02', '-x', '4000000000', bin], { cwd: OUT }).split(/\r?\n/);
  const F = ['action', 'actor', 'flags', 'dir', 'how', 'prep', 'aux', 'manner', 'order', 'dflt', 'skill', 'task', 'n',
    'obj.kind', 'obj.sel', 'obj.n', 'obj.lies', 'other.kind', 'other.sel', 'other.n', 'other.lies'];
  const field = (c, k) => k.includes('.') ? c[k.split('.')[0]][k.split('.')[1]] : c[k];
  let n = 0, bad = 0, long = 0, sum = 0, most = 0;
  for (const l of out) {
    const m = l.match(/^(\d+) (-?\d+) (\d+) ([\d ]+) \|([^|]*)\|([^|]*)\|$/);
    if (!m) { if (/not right/.test(l)) console.log(l); continue; }
    const i = +m[1], v = m[4].split(' ').map(Number);
    n++;
    sum += +m[3];
    most = Math.max(most, +m[3]);
    if (+m[2] < 0) { long++; continue; }          // (Too long for the matcher: its tokens cut short)
    const c = words.scan(words.tokens(lines[i], DEFAULT_ALIASES, DEFAULT_ANYWHERE), w, i & 7);
    const d = F.filter((k, j) => field(c, k) !== v[j]);
    if (c.obj.word !== m[5]) d.push('obj.word');
    if (c.other.word !== m[6]) d.push('other.word');
    if (d.length) {
      bad++;
      if (bad <= 30) console.log('differs: ' + JSON.stringify(lines[i]) + ' (facing ' + (i & 7) + '): ' +
        d.map(k => k + ' ' + (k === 'obj.word' ? m[5] : k === 'other.word' ? m[6] : v[F.indexOf(k)]) + ', the host ' + field(c, k)).join('; '));
    }
  }
  if (n !== lines.length) { console.log('only ' + n + ' of ' + lines.length + ' lines ran'); return false; }
  console.log(n + ' lines, ' + bad + ' differ from the host' + (long ? ' (' + long + ' too long, not compared)' : ''));
  console.log('cycles: cm_mean ' + Math.round(sum / n) + ' on average, ' + most + ' the most');
  return bad === 0;
}

// The corpus for a run under HydraOS (cmdhy.c and cmdtask.c read it): the world's words as they change (W, N, D),
// each command with the host's result (C)
function corpusFile(pc) {
  const lines = [];
  let w, n, d;
  text.forEach((t, i) => {
    if (world[i] !== w) lines.push('W ' + (w = world[i]));
    if (names[i] !== n) lines.push('N ' + (n = names[i]));
    if (deities[i] !== d) lines.push('D ' + (d = deities[i]));
    lines.push('C ' + ok[i] + ' ' + t);
  });
  fs.writeFileSync(path.join(pc, 'corpus.txt'), lines.join('\n') + '\n');
}

// A program built (build.js prog) from its files in a folder of its own under OUT, into pc as NAME
function prog(name, files, pc, env = {}) {
  const dir = path.join(OUT, name);
  fs.rmSync(dir, { recursive: true, force: true });
  fs.mkdirSync(dir, { recursive: true });
  for (const f of files) fs.copyFileSync(f, path.join(dir, path.basename(f)));
  run(process.execPath, ['build.js', 'prog', dir], { cwd: REB, env: Object.assign({}, process.env, env) });
  fs.copyFileSync(path.join(dir, name + '.hyx'), path.join(pc, name));
}

// pc/NAME run in the emulator, at the shell once it's up; what it says (its lines up to the next prompt)
function emulate(pc, name) {
  const B = String.fromCharCode(92), keys = B + 'w' + B + 'w' + B + 'w/pc/' + name + B + 'r';   // (Typed once the shell is up)
  const r = cp.spawnSync(process.execPath, ['sim/run.js', '--pc-dir', pc, '--input', keys, '--cycles', '900000000'],
    { cwd: REB, encoding: 'utf8', maxBuffer: 1 << 26 });
  const o = r.stdout || '', at = o.indexOf('/pc/' + name), end = o.indexOf('\n/>', at);
  const said = o.slice(at, end < 0 ? at + 4000 : end).split('\n').slice(1).filter(l => /^[ -~]*$/.test(l));
  console.log(said.join('\n') || '(nothing: ' + (o.split('\n').find(l => /stopped/.test(l)) || 'no run') + ')');
  return said;
}

// The matcher alone under HydraOS (cmdhy.c: the grammar in a RAM bank)
function hydra() {
  const pc = path.join(OUT, 'pc');
  fs.mkdirSync(pc, { recursive: true });
  run(process.execPath, [path.join(__dirname, 'commandtables.js'), path.join(pc, 'grammar.bin'), path.join(pc, 'aliases.bin'), path.join(pc, 'index.bin')]);
  corpusFile(pc);
  prog('cmdhy', [path.join(HERE, 'cmdhy.c'), path.join(HERE, 'cmdmatch.h'), path.join(HERE, 'cmdmatch_c.s'), path.join(GAME, 'cmdmatch.s')], pc);
  return emulate(pc, 'cmdhy').some(l => / 0 differ$/.test(l));
}

// The parser's task (dq0c) under HydraOS, through the hub's protocol (cmdtask.c the UI's part): its round trips
function task() {
  const pc = path.join(OUT, 'pc');
  fs.mkdirSync(pc, { recursive: true });
  require(path.join(GAME, 'make.js')).build({ dir: GAME, objdir: path.join(pc, 'dq0c-obj') });   // (pc/dq0c.dat)
  corpusFile(pc);
  prog('dq0c', ['dq0c.s', 'cmdmatch.s', 'cmdmean.s', 'hub.inc'].map(f => path.join(GAME, f)), pc);
  prog('cmdtask', [path.join(HERE, 'cmdtask.c')], pc, { HYC_CFLAGS: '-I ' + path.join(REB, 'games', 'dq0') });
  return emulate(pc, 'cmdtask').some(l => / 0 differ$/.test(l));
}

process.exit((process.argv.includes('--mean') ? mean() : process.argv.includes('--task') ? task() : process.argv.includes('--hydra') ? hydra() : sim65()) ? 0 : 1);
