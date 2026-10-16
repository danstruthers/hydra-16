// make.js - (dq0s) the sound task's data file, after its build (build.js calls build): sounds.txt (a stand-in theme's
// sounds) made obj/games/dq0s.dat, 32 bytes a sound (dq0s.c's struct dsnd): each kind's two (its step, its noise) in
// the denizens' kinds' order (dq0d/kinds.txt: the first none), SND_KINDS of them (hub.h), then the world's (WS_*).
'use strict';
const fs = require('fs');
const path = require('path');

const SND_KINDS = 32, SND_WORLD = 16, SND_REC = 32, SND_WORDS = 20;    // hub.h's
const WORLD = ['*dig', '*open', '*shut', '*blow', '*smash', '*chains', '*trap'];   // dq0s.c's WS_*
const WAVES = { p: 0, s: 1, t: 2, n: 3 };       // snd.h's SND_WAVE_*
const S_USED = 1, S_NOISY = 2;                  // dq0s.c's S_*

const lines = f => fs.readFileSync(f, 'utf8').split(/\r?\n/).filter(l => l.trim() && !l.startsWith('#'))
  .map(l => l.split('|').map(s => s.trim()));

function build({ dir, objdir, say = () => {} }) {
  const keys = lines(path.join(dir, '..', 'dq0d', 'kinds.txt')).map(f => f[1]);
  if (keys.length >= SND_KINDS) throw new Error('dq0s: more kinds than SND_KINDS');
  const out = Buffer.alloc((2 * SND_KINDS + SND_WORLD) * SND_REC);
  lines(path.join(dir, 'sounds.txt')).forEach((f, i) => {
    const bad = () => { throw new Error(`dq0s/sounds.txt: line ${i + 1}: ${f.join(' | ')}`); };
    if (f.length !== 8) bad();
    const [key, act, words, ym, psg, hits, hold, level] = f;
    const n = key.startsWith('*') ? 2 * SND_KINDS + WORLD.indexOf(key) : 2 * (keys.indexOf(key) + 1) + (act === 'noise');
    if (key.startsWith('*') ? WORLD.indexOf(key) < 0 : keys.indexOf(key) < 0 || !/^(step|noise)$/.test(act)) bad();
    const w = words === '-' ? '' : words, y = ym.split(/\s+/), p = psg.split(/\s+/), h = hits.split('-').map(Number);
    if (w.length >= SND_WORDS || y.length !== 2 || p.length !== 3 || !(p[0] in WAVES) || h.length !== 2) bad();
    const at = n * SND_REC;
    out.write(w, at, 'latin1');
    const drum = y[0] === 'd';
    [drum ? 1 : 0, drum ? +y[1] : +y[0], drum ? 0 : +y[1], WAVES[p[0]] << 6 | (+p[1] & 63), +p[2], h[0], h[1], +hold,
      +level, S_USED | (p[0] === 'n' ? S_NOISY : 0)].forEach((v, k) => { out[at + SND_WORDS + k] = v; });
  });
  fs.writeFileSync(path.join(objdir, '..', 'dq0s.dat'), out);
  return { sounds: out.length / SND_REC };
}

module.exports = { build };
