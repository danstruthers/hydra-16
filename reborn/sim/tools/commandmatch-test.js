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
//   node sim/tools/commandmatch-test.js --phrases the phrasing test: cmdmatch/phrases.txt's cases and the corpus's with a
//                                                 context, each line made a command as the game makes it on the 65C02
//                                                 (dq0c's meaning, then the UI's resolver, games/dq0/resolve.c: on
//                                                 sim65, cmdmatch/cmdres.c), held to what each case expects
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

// The phrasing test (--phrases): cmdmatch/phrases.txt's cases, and the corpus's with a context, each line made a
// command on the 65C02 as the game makes it (dq0c's meaning, then the UI's resolver: cmdmatch/cmdres.c on sim65, a run a
// case), held to what the case expects (phrases.txt's; the corpus's: the grammar's reference, commandcompile.js's
// action and its object)
const DENS = new Set(['goblin', 'goblins', 'rat', 'rats', 'orc', 'troll', 'dragon', 'linda', 'grub', 'kobold', 'giant rat']);
// The corpus's cases the game reads otherwise, by design: "under" a way (the things put in or on only); "follow" an ally's
// request (the game's: it follows the character); a list of attacks (lists to come)
const KNOWN = new Set(['put the apple under the table', 'follow the goblin', 'attack the rat and the goblin']);
function phraseCases() {
  const cc = require('./commandcompile.js');
  const comp = new cc.Compiler(cc.loadRules(path.join(REB, 'games', 'dq0', 'commands.ebnf')));
  const read = f => fs.readFileSync(f, 'utf8').split(/\r?\n/).filter(l => l.trim() && !l.startsWith('#'));
  const cases = [];
  for (const l of read(path.join(HERE, 'phrases.txt'))) {
    const [lines, seen, want] = l.split('\t');
    const things = seen === '-' ? [] : seen.split(' ').filter(s => !s.startsWith('face='));
    const face = (seen.match(/face=(\d)/) || [])[1] || 0;
    cases.push({ what: lines, lines: lines.split(' | ').map(s => s.trim()), face: +face, things: things.map(s => {
      const m = s.match(/^(\*?)([a-z_']+)@(-?\d+),(-?\d+)(?::(\w+))?$/);
      if (!m) throw new Error('phrases.txt: ' + s);
      return { name: m[2].replace(/_/g, ' '), dx: +m[3], dy: +m[4], size: m[5] && m[5] !== 'down' ? +m[5] : 3,
        den: m[1] ? (m[5] === 'down' ? 2 : 1) : 0 };
    }), want: want.trim() });
  }
  // The corpus's: a context of things near (denizens by their names) and the last thing named (said first: "x" it)
  for (const l of read(path.join(REB, 'games', 'dq0', 'commands.corpus'))) {
    const [input, expect, roles, context] = l.split('\t');
    if (!context || context === '-' || /engaged=|pending=|aliases=|names=|person=|target=|distances=|deities=/.test(context) ||
      !/^VB_/.test(expect) || expect === 'VB_SEQUENCE' || /[A-Za-z]+=[a-z]/.test(context.replace(/(near|last|facing)=[^;]*/g, ''))) continue;
    const ctx = Object.fromEntries(context.split(';').map(p => p.split('=')));
    const r = comp.compile(input, cc.buildContext(context, cc.DEFAULT_WORLD));
    if (r.action !== expect) continue;
    const obj = (roles.match(/(?:^|;)object=([^;]*)/) || [])[1];
    const near = [...new Set([...(ctx.near || '').split(','), ...(ctx.last ? [ctx.last] : [])])].filter(Boolean);   // (The
                                                // last thing named one it perceives: a bare take's)
    if (KNOWN.has(input)) continue;
    cases.push({ what: input + '  (the corpus\'s)', lines: [...(ctx.last ? ['x ' + ctx.last] : []), input], face: 0,
      things: near.map((n, i) => ({ name: n.replace(/\d+$/, ''), dx: (i % 3) - 1, dy: -1 - Math.floor(i / 3), size: 3,
        den: DENS.has(n.replace(/\d+$/, '')) ? 1 : 0 })),
      want: expect + (obj && !/^all|^me$|^way out$|^fire$/.test(obj) ? ' obj~' + obj.replace(/\d+$/, '') : '') });
  }
  return cases;
}

function phrases() {
  const words = require('./commandwords.js'), V = words.names(), wd = words.load();
  const cases = phraseCases();
  const bin = path.join(OUT, 'cmdres.bin');
  run(tool('cl65'), ['--cpu', '65C02', '-t', 'sim65c02', '-C', path.join(HERE, 'cmdres.cfg'), '-O', '-I', OUT, '-I', HERE, '-I',
    path.join(REB, 'games', 'dq0'), '-o', bin, path.join(HERE, 'cmdres.c'), path.join(GAME, 'cmdmatch.s'), path.join(GAME, 'cmdmean.s'),
    path.join(REB, 'games', 'dq0', 'resolve.c'), path.join(HERE, 'cmdmatch_c.s'), path.join(HERE, 'cmdmean_c.s')],
    { cwd: OUT, stdio: ['ignore', 'pipe', 'pipe'] });
  const N = (k, n) => n === undefined ? undefined : /^\d+$/.test(n) ? +n : V[n] !== undefined ? V[n] : NaN;
  let bad = 0, mine = 0;
  cases.forEach((c, k) => {
    const out = run(tool('sim65'), ['--cpu', '65C02', '-x', '400000000', bin, c.lines.join('|'), String(c.face),
      ...c.things.map(t => [t.den, t.dx, t.dy, t.size, t.name].join(' '))], { cwd: OUT }).split(/\r?\n/);
    const says = out.filter(l => l.startsWith('S ')).map(l => l.slice(2));
    const m = (out.find(l => l.startsWith('R ')) || '').match(/^R (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) \|([^|]*)\|(-?\d+),(-?\d+)\|([^|]*)\|(-?\d+),(-?\d+)\|$/);
    const f = [];
    if (!m) f.push('no command (' + out.join(' / ') + ')');
    else {
      const got = { action: +m[1], flags: +m[2], dflt: +m[3], prep: +m[4], manner: +m[5], sel: +m[6], kind: +m[7], obj: m[8],
        objat: m[9] === '99' ? '-' : m[9] + ',' + m[10], other: m[11], otherat: m[12] === '99' ? '-' : m[12] + ',' + m[13] };
      const ws = [];                           // (Its words: a value of several words, the words after its key's)
      for (const s of c.want.split(' ')) if (/^VB_|^[a-z]+[=@~]/.test(s) || !ws.length) ws.push(s); else ws[ws.length - 1] += ' ' + s;
      for (const w of ws) {
        if (/^VB_/.test(w)) { if (words.actionName(got.action) !== w) f.push('action ' + words.actionName(got.action) + ' (want ' + w + ')'); continue; }
        const [, key, op, val] = w.match(/^([a-z]+)(=|@|~)(.*)$/) || [];
        if (key === 'says') { if (!says.some(s => s.startsWith(val))) f.push('says ' + JSON.stringify(says) + ' (want ' + val + ')'); continue; }
        const g = op === '@' ? got[key + 'at'] : got[key];
        const ok = op === '~' ? g === val || g.split(' ').pop() === val || (g && val.startsWith(g)) ||   // (A thing by its
          ((wd.things.get(val) & 0x7F) === got.kind && got.kind > 0) :         //   word, or its kind: the door)
          key === 'prep' || key === 'manner' || key === 'sel' ? g === N(key, val) : g === val;
        if (!ok) f.push(key + op + JSON.stringify(g) + ' (want ' + val + ')');
      }
    }
    mine += !c.what.endsWith('(the corpus\'s)');
    if (f.length) { bad++; if (bad <= 40) console.log('differs: ' + c.what + ': ' + f.join('; ')); }
  });
  console.log(cases.length + ' cases (' + mine + ' phrases.txt\'s, ' + (cases.length - mine) + ' the corpus\'s), ' + bad + ' differ');
  return bad === 0;
}

process.exit((process.argv.includes('--mean') ? mean() : process.argv.includes('--phrases') ? phrases() : process.argv.includes('--task') ? task() : process.argv.includes('--hydra') ? hydra() : sim65()) ? 0 : 1);
