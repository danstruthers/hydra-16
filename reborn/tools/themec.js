// ****************************************************************************
// themec.js - Dingus Quest Zero's theme compiler, run (ENGINE-CRITIQUE.md's section 9, step 11; DECISIONS.md's "The meta
// programs' library, their state record, one theme compiler"): games/themec, the game's one compiler of a theme's text
// into its record (comp.c: theme.h's struct theme, then its words) and its checker, a C program of the Hydra's, run in
// the emulator.  The game needs it there (themegen makes a theme's text as a game begins and runs themec on it); the
// build runs it here for the stand-in themes (games/dq0/make.js's after: obj/games/dq0t.dat) and the goal kinds
// (games/themegen/make.js: each one's theme held to a theme's), and the tests for what was compiled on the Hydra.
//
// compile (themes, opt): each theme ({ name, text }) compiled and checked by themec, all in one machine: a paged ROM of
// the modules (modules/rom.txt's, but hysnap) and t_rc (its init), the ROM disk's files, booted with an SD card of
// themec, bestiary.dat, styles.dat (the styles' sets: a theme's styles line) and the themes' texts; "themec -l /sd/0
// -o tN.dat tN.txt" typed for each.  OUT: for each,
// { name, status (themec's: 0, or 1 its faults said), said (what it said: its faults, else its name and its words'
// bytes), data (its record, THEME_SIZE bytes; null if it failed), words (its words' bytes) }.  opt: { root (reborn/),
// out (false: only checked, no record), bios (the BIOS image: default, the built one) }.  Each answer is kept
// (obj/themec, by the hash of themec, the bestiary, the styles, the image's modules and the text), so a test asking
// again doesn't boot the machine.
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const crypto = require('crypto');

const CYCLES = 3000e6;                                        // (A machine's run at most: a theme some 30M cycles)

function compile(themes, opt = {}) {
  const ROOT = opt.root || path.join(__dirname, '..'), at = (...p) => path.join(ROOT, ...p);
  const themec = fs.readFileSync(at('obj', 'games', 'themec.hyx')), beasts = fs.readFileSync(at('obj', 'games', 'bestiary.dat'));
  const styles = fs.readFileSync(at('obj', 'games', 'styles.dat'));
  const { readManifest } = require('../../base/build.js');
  const names = readManifest(at('modules', 'rom.txt')).modules.filter(n => n !== 'hysnap');
  const mods = [...names.map(n => ({ file: n, data: fs.readFileSync(at('obj', 'modules', n + '.bin')) })),
    { file: 't_rc', data: fs.readFileSync(at('obj', 'tests', 't_rc.bin')) }];
  const key = crypto.createHash('sha1').update(themec).update(beasts).update(styles).update(opt.out === false ? 'check' : 'out');
  for (const m of mods) key.update(m.file).update(m.data);
  const base = key.digest('hex'), cache = at('obj', 'themec');
  const keyOf = t => crypto.createHash('sha1').update(base).update(t.text).digest('hex');
  const out = themes.map(t => {                               // (Those asked before: kept)
    try {
      const j = JSON.parse(fs.readFileSync(path.join(cache, keyOf(t) + '.json'), 'utf8'));
      return Object.assign(j, { name: t.name, data: j.data === null ? null : Buffer.from(j.data, 'base64') });
    } catch (e) { return null; }
  });
  const todo = themes.map((t, i) => i).filter(i => !out[i]);
  if (todo.length) run(todo.map(i => themes[i]), opt, { ROOT, at, themec, beasts, styles, mods }).forEach((r, k) => {
    const i = todo[k];
    out[i] = r;
    try {
      fs.mkdirSync(cache, { recursive: true });
      const f = path.join(cache, keyOf(themes[i]) + '.json');
      fs.writeFileSync(f + '.' + process.pid, JSON.stringify(Object.assign({}, r, { data: r.data && r.data.toString('base64') })));
      fs.renameSync(f + '.' + process.pid, f);
    } catch (e) { /* (Not kept: run again next time) */ }
  });
  return out;
}

// The machine run on themes (not kept yet): its answers, as compile's
function run(themes, opt, { ROOT, at, themec, beasts, styles, mods }) {
  const romimg = require('./romimg.js'), romfs = require('./romfs.js'), hydrafs = require('../sim/tools/hydrafs.js');
  const { hwtest } = require('../../base/build.js'), images = require('../../base/tools/images.js');
  const { boot } = require('../sim/run.js');
  const bios = opt.bios || images.bios(ROOT, images.built(ROOT));
  const { image } = romimg.build({ modules: mods, init: 't_rc', hwtest: hwtest(), bios, romfs: romfs.manifest(at('romfs', 'romfs.txt')) });
  const img = path.join(os.tmpdir(), 'themec-' + process.pid + '-' + Date.now() + '.img');
  hydrafs.mkfs(img, 1, 'THEMEC', undefined, true);
  let v = new hydrafs.Volume(img);
  v.put('themec', themec);
  v.put('bestiary.dat', beasts);
  v.put('styles.dat', styles);
  themes.forEach((t, i) => v.put('t' + i + '.txt', Buffer.from(t.text, 'latin1')));
  v.close();
  const fd = fs.openSync(img, 'r+');
  try {
    const card = { dev: 0, blocks: Math.floor(fs.fstatSync(fd).size / 512), file: img,
      read: n => { const b = Buffer.alloc(512); fs.readSync(fd, b, 0, 512, n * 512); return b; },
      write: (n, b) => { fs.writeSync(fd, Buffer.from(b), 0, 512, n * 512); } };
    const lines = ['bind -a /sd/0 /bin', ...themes.flatMap((t, i) => ['themec -l /sd/0 ' + (opt.out === false ? '' : '-o /sd/0/t' + i + '.dat ') +
      '/sd/0/t' + i + '.txt', 'echo =' + i + ' $status']), 'echo =END'];
    const m = boot({ prom: image, bios, sd: [card], seed: 1, marks: [], log: () => {}, input: lines.map(l => 'ā' + l + '\r').join('') });
    while (!/\n=END\r?\n/.test(m.out) && !m.cpu.halted && m.cpu.cyc < CYCLES) m.run(m.cpu.cyc + 2e6);
    const said = m.out.replace(/\r/g, '');
    if (!/\n=END\n/.test(said)) throw new Error('themec in the emulator: no end' + (m.cpu.halted ? ' (' + m.cpu.halted + ')' : '') + ': ' + JSON.stringify(said.slice(-400)));
    fs.closeSync(fd);
    v = new hydrafs.Volume(img, { readOnly: true });
    return themes.map((t, i) => {
      const a = said.indexOf(' /sd/0/t' + i + '.txt\n'), st = new RegExp('\\n=' + i + '( \\d+)?\\n', 'g');   // (Its status: "" 0)
      st.lastIndex = a;
      const sm = a < 0 ? null : st.exec(said), e = sm ? sm.index : -1, status = sm && sm[1] ? +sm[1] : 0;
      if (a < 0 || e < 0) throw new Error('themec in the emulator: theme ' + i + ' (' + t.name + ') not run: ' + JSON.stringify(said.slice(-400)));
      const text = said.slice(a + ('/sd/0/t' + i + '.txt\n').length + 1, e).replace(/(^|\n)% [^\n]*$/, '').trim();
      const words = /: (\d+) bytes of words, its checks passed$/.exec(text);
      let data = null;
      if (status === 0 && opt.out !== false) {
        const f = v.tryWalk('t' + i + '.dat');
        data = f ? Buffer.from(v.read(f)) : null;
      }
      return { name: t.name, status, said: text, data, words: words ? +words[1] : null };
    });
  } finally {
    try { fs.closeSync(fd); } catch (e) { /* (Closed) */ }
    try { if (v) v.close(); } catch (e) { /* (Closed) */ }
    fs.rmSync(img, { force: true });
  }
}

module.exports = { compile };
