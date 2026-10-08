#!/usr/bin/env node
// hysong.js: the Hydra's score compiler.  A score (text: instruments, and a line of MML for each YM2151 channel and
// each of the Vera X's PSG voices) becomes a ZSM song (the Commander X16's format, which the Hydra's player plays: os_rom/sound/player.s), with the
// patches and volumes worked out here, so the song is plain register writes and delays: it plays on an X16 or in
// any ZSM player as well.  Optionally a VGM too (to hear it on the PC), and a ca65 source that puts it in the paged
// ROM (sndtest's song: os_rom/songs/).
//
// Usage: node hysong.js SCORE.mml SONG.zsm [--rom SONG.s] [--vgm SONG.vgm] [--quiet]
//
// The score (';' starts a comment, to the end of the line):
//   #tempo 112              quarter notes a minute (default 120)
//   #rate 200               the song's ticks a second (default 200: the Hydra's own tick)
//   #title Some text        (kept in the report)
//   @name { ... }           an instrument (below)
//   A ...  (to H)           MML for channel 0 (to 7), the YM2151's; a channel's lines are joined in order
//   I ...  (to X)           MML for channel 8 (to 23), the PSG's voices 0-15 (the letter less A is the sound
//                           driver's channel, as play -m takes it)
//
// An instrument: "gm N" (the ROM's patch N: 0-127 General MIDI's, 128-162 drums), or the voice itself:
//   alg 0-7  fb 0-7  pms 0-7  ams 0-3, then each operator (m1 m2 c1 c2), each followed by any of
//   mul 0-15 (0: 1/2)  dt1 0-7  dt2 0-3  tl 0-127 (attenuation)  ks 0-3  ar 0-31  d1r 0-31  d2r 0-31  d1l 0-15
//   rr 0-15  am 0-1 (tremolo on).  Unset: 0 (tl: 127, silent).  Which operators sound: alg 0-3 c2; 4 c1 c2; 5, 6
//   m2 c1 c2; 7 all four.
// A PSG instrument (for channels I-X) is its waveform and envelope instead:
//   wave W [WIDTH]          W pulse, saw, triangle or noise (or 0-3), WIDTH 0-63 (a pulse's duty: 63 a square;
//                           the default)
//   env A D S R             the volume's envelope, in the song's ticks: A ticks rising from 0 to the note's
//                           volume, D falling by S (the PSG's 0.5 dB steps, 0-63: D 0, none) to the level it holds
//                           till the key off, then R falling to silence (0: at once).  Each a straight line in the
//                           volume register (so in dB), written as it changes.  Unset: 0 0 0 0 (a note on, then off)
//
// MML (a channel's line):
//   c d e f g a b [+ # -] [len] [.]   a note (+ or #: sharp, -: flat); len 1 2 4 8 16 32 64 (or 3 6 12 24 48:
//                           triplets), dots add half; ^len ties more on (c4^16)
//   r [len]                 a rest
//   &                       between notes: legato (the next note changes the pitch, no new attack)
//   _                       before a note: slide to it from the last pitch, over its length (no new attack)
//   x N [len]               a General MIDI drum (N: 35 kick, 38 snare, 42 hi-hat, 45 tom, 49 crash ...: the
//                           ROM's drum map and patches)
//   o N  > <                octave (o4 c: middle C, MIDI 60); up, down
//   l N                     the length for notes without one
//   q N                     the part of a note held before its key off, in eighths (1-8; default 7)
//   v N                     volume 0-127 (the carriers' levels, General MIDI's curve; the PSG's: the next note's,
//                           1.5 of its steps for each 0.75 dB of the curve, as the sound driver attenuates it)
//   p l|r|c|0               speakers: left, right, both, none
//   @name                   the instrument, from the next note on
//   I N                     the ROM's patch N as the instrument (0-162), from the next note on; the PSG's: waveform
//                           N (0-3, as wave), width 63
//   k N                     transpose (semitones)      D N    detune (64ths of a semitone)
//   M pms,ams               the channel's LFO sensitivities (vibrato 0-7, tremolo 0-3)
//   L rate,pmd,amd,wave     the LFO (the whole chip): rate 0-255, depths 0-127, wave 0 saw 1 square 2 triangle 3 noise
//   N n  N-                 noise (channel 7's C2) on at frequency n (0-31); off
//   y reg,val               any register (e.g. the timers); the PSG's: its registers, 0-63
//   [ ... ]N                repeat N times (nested)
// On the PSG's channels x, M, L and N (the YM2151's) are errors; a slide's steps write the frequency.  The writes of a
// tick: the PSG's at once (a ZSM's $00-$3F), the YM2151's in chunks of 63 (a chunk out as it fills, the rest at the
// tick's end); the song's end at the longest channel's time, or after the last envelope's last step if that's later.

'use strict';
const fs = require('fs');
const path = require('path');

const UNITS = 192;                                          // A whole note
const SLOTS = { m1: 0, m2: 1, c1: 2, c2: 3 };                // Operator: its register offset / 8
const CARRIERS = [[3], [3], [3], [3], [2, 3], [1, 2, 3], [1, 2, 3], [0, 1, 2, 3]];
const NOTE_CODE = [0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14]; // C# D D# E F F# G G# A A# B C
const atten = v => v <= 0 ? 127 : Math.min(127, Math.round(-40 * Math.log10(Math.min(v, 127) / 127) / 0.75));
const FM = 8;                                               // The YM2151's channels; the PSG's are 8-23
const PSG_FREQ = [22473, 23810, 25226, 26726, 28315, 29998, 31782, 33672, 35674, 37796, 40043, 42424, 44947];
const WAVES = { pulse: 0, saw: 1, triangle: 2, noise: 3 };
// A PSG voice's frequency word (Hz * 2^17 / 48,828.125) for pitch p (64ths of a semitone over MIDI 0), as the sound
// driver works it out (snd.s: psg_pitch): the octave C9-C10's words, the 64ths between semitones a straight line,
// halved to the note's octave, rounded
function psgWord(p) {
  p = Math.max(0, Math.min(131 * 64 + 63, p));
  let n = p >> 6, oct = 0;
  while (n < 120) { n += 12; oct++; }
  const k = n - 120, w = PSG_FREQ[k] + ((((PSG_FREQ[k + 1] - PSG_FREQ[k]) >> 2) * (p & 63)) >> 4);
  return oct ? (w >> oct) + ((w >> (oct - 1)) & 1) : w;
}
const psgVol = v => v <= 0 ? 0 : Math.max(0, 63 - atten(v) - (atten(v) >> 1));

// ---- The sound driver's patches and drum map (modules/snd/patches.s: the old system's os_rom/sound/patches.s)
function romPatches() {
  const src = fs.readFileSync(path.join(__dirname, '../../modules/snd/patches.s'), 'latin1');
  const table = (from, to) => {
    const s = src.slice(src.indexOf(from), to ? src.indexOf(to) : undefined);
    return [...s.matchAll(/\$([0-9A-Fa-f]{2})\b/g)].map(m => parseInt(m[1], 16));
  };
  const all = table('\npatches:', '\n; General MIDI drums');
  const patches = [];
  for (let i = 0; i + 26 <= all.length; i += 26) patches.push(all.slice(i, i + 26));
  return { patches, drumPatch: table('\ndrum_patch:', '\ndrum_kc:'), drumKc: table('\ndrum_kc:', '\n; Attenuation') };
}

// ---- An instrument's tokens as a patch (26 bytes: $20, $38, then $40-$F8 by 8, as the ROM's)
function instrument(name, toks, rom) {
  if (toks.includes('wave') || toks.includes('env')) return psgInstrument(name, toks);
  if (toks[0] === 'gm') {
    const n = +toks[1];
    if (!(n >= 0 && n < rom.patches.length)) throw new Error('@' + name + ': no patch ' + toks[1]);
    return rom.patches[n].slice();
  }
  const g = { alg: 0, fb: 0, pms: 0, ams: 0 };
  const ops = [0, 1, 2, 3].map(() => ({ mul: 0, dt1: 0, dt2: 0, tl: 127, ks: 0, ar: 0, d1r: 0, d2r: 0, d1l: 0, rr: 0, am: 0 }));
  let op = null;
  for (let i = 0; i < toks.length; i++) {
    const k = toks[i];
    if (k in SLOTS) { op = ops[SLOTS[k]]; continue; }
    const v = +toks[++i];
    if (!Number.isInteger(v)) throw new Error('@' + name + ': ' + k + ' needs a number');
    if (op && k in op) op[k] = v;
    else if (k in g) g[k] = v;
    else throw new Error('@' + name + ': what is ' + k + '?');
  }
  const p = [(g.fb & 7) << 3 | (g.alg & 7), (g.pms & 7) << 4 | (g.ams & 3)];
  for (const f of [o => (o.dt1 & 7) << 4 | (o.mul & 15), o => o.tl & 127, o => (o.ks & 3) << 6 | (o.ar & 31),
    o => (o.am & 1) << 7 | (o.d1r & 31), o => (o.dt2 & 3) << 6 | (o.d2r & 31), o => (o.d1l & 15) << 4 | (o.rr & 15)])
    for (const o of ops) p.push(f(o));
  return p;
}

// ---- A PSG instrument's tokens: its waveform register (3: the waveform, the width) and its envelope
function psgInstrument(name, toks) {
  const p = { psg: true, wave: 63, env: [0, 0, 0, 0] };
  const bad = () => { throw new Error('@' + name + ': an instrument it can\'t read'); };
  for (let i = 0; i < toks.length; i++) {
    if (toks[i] === 'wave') {
      const w = toks[++i], n = w in WAVES ? WAVES[w] : /^\d+$/.test(w || '') ? +w : -1;
      if (!(n >= 0 && n <= 3)) bad();
      let width = 63;
      if (/^\d+$/.test(toks[i + 1] || '')) { width = +toks[++i]; if (width > 63) bad(); }
      p.wave = n << 6 | width;
    } else if (toks[i] === 'env') {
      for (let k = 0; k < 4; k++) {
        const v = toks[++i];
        if (!/^\d+$/.test(v || '') || +v > (k === 2 ? 63 : 255)) bad();
        p.env[k] = +v;
      }
    } else bad();
  }
  return p;
}

// ---- The score
function parseScore(text, rom) {
  const score = { tempo: 120, rate: 200, title: '', inst: {}, tracks: new Array(24).fill('') };
  text = text.replace(/;[^\n]*/g, '');
  text = text.replace(/@(\w+)\s*\{([^}]*)\}/g, (m, name, body) => { score.inst[name] = instrument(name, body.trim().split(/\s+/), rom); return ''; });
  for (const line of text.split(/\r?\n/)) {
    const s = line.trim();
    if (!s) continue;
    let m;
    if ((m = /^#(\w+)\s*(.*)$/.exec(s))) {
      if (m[1] === 'tempo') score.tempo = +m[2];
      else if (m[1] === 'rate') score.rate = +m[2];
      else if (m[1] === 'title') score.title = m[2];
      else throw new Error('#' + m[1] + '?');
    } else if ((m = /^([A-X])\s+(.*)$/.exec(s))) score.tracks[m[1].charCodeAt(0) - 65] += ' ' + m[2];
    else throw new Error('what is this line? ' + s);
  }
  return score;
}

// Repeats: [ ... ]N, expanded (nested)
function expand(s) {
  for (;;) {
    const m = /\[([^\[\]]*)\](\d*)/.exec(s);
    if (!m) return s;
    s = s.slice(0, m.index) + (' ' + m[1] + ' ').repeat(m[2] === '' ? 2 : +m[2]) + s.slice(m.index + m[0].length);
  }
}

// ---- A channel's MML as register writes at times (units): { u, seq, reg, val }
function compileTrack(ch, mml, score, rom, out) {
  let seq = out.length;
  const put = (u, reg, val) => out.push({ u, seq: seq++, reg, val: val & 255 });
  const st = { oct: 4, len: 4, gate: 7, vol: 100, pan: 0xC0, inst: null, instName: '', pending: null, lfo: null,
    tr: 0, det: 0, pitch: null, legato: false, slide: false };
  let u = 0;
  const s = expand(mml);
  let i = 0;
  const ws = () => { while (i < s.length && /\s/.test(s[i])) i++; };
  const num = () => {                                       // A number: decimal, or $hex
    ws();
    const m = /^(-?)(?:\$([0-9A-Fa-f]+)|(\d+))/.exec(s.slice(i));
    if (!m) return null;
    i += m[0].length;
    return (m[1] ? -1 : 1) * (m[2] !== undefined ? parseInt(m[2], 16) : +m[3]);
  };
  const len = () => {                                       // A length (default l), dots, ^ ties
    let n = num(), d = UNITS / (n === null ? st.len : n);
    if (!Number.isInteger(d)) throw new Error('channel ' + ch + ': length ' + n);
    let add = d;
    while (s[i] === '.') { add /= 2; d += add; i++; }
    ws();
    if (s[i] === '^') { i++; d += len(); }
    return d;
  };
  const kcf = p => {                                        // Pitch (64ths of a semitone, MIDI) as key code and fraction
    let n = Math.floor(p / 64), kf = p - n * 64;
    if (n < 13) { n = 13; kf = 0; } else if (n > 108) { n = 108; kf = 63; }
    return [(Math.floor((n - 13) / 12) << 4) | NOTE_CODE[(n - 13) % 12], kf << 2];
  };
  const patch = at => {                                     // The instrument's registers, with the channel's settings
    const p = st.pending;
    st.inst = p; st.pending = null;
    put(at, 0x20 + ch, st.pan | (p[0] & 0x3F));
    put(at, 0x38 + ch, st.lfo === null ? p[1] : st.lfo);
    for (let r = 0; r < 24; r++) {
      const slot = r & 3, reg = 0x40 + (r >> 2) * 0x20 + slot * 8 + ch;
      let v = p[2 + r];
      if (reg >= 0x60 && reg < 0x80 && CARRIERS[p[0] & 7].includes(slot)) v = Math.min(127, v + atten(st.vol));
      put(at, reg, v);
    }
  };
  const levels = at => {                                    // The carriers' levels again (a new volume)
    if (!st.inst) return;
    for (const slot of CARRIERS[st.inst[0] & 7]) put(at, 0x60 + slot * 8 + ch, Math.min(127, st.inst[6 + slot] + atten(st.vol)));
  };
  const play = (pitch, dur, drum) => {                      // A note: its attack (or a legato or slide), its key off
    const glide = st.slide && st.pitch !== null && !drum;
    const attack = !(st.legato || glide) || drum;
    if (st.pending) patch(u);
    if (glide) {                                            // A slide: the pitch every 2 ticks of the note
      const t0 = tick(u, score), t1 = tick(u + dur, score), from = st.pitch;
      for (let t = t0; t < t1; t += 2) {
        const [kc, kf] = kcf(Math.round(from + (pitch - from) * (t - t0) / Math.max(1, t1 - t0)));
        const at = u + (t - t0) * dur / Math.max(1, t1 - t0);
        put(at, 0x28 + ch, kc); put(at, 0x30 + ch, kf);
      }
    } else {
      const [kc, kf] = drum ? [drum.kc, 0] : kcf(pitch);
      put(u, 0x28 + ch, kc); put(u, 0x30 + ch, kf);
    }
    if (attack) { put(u, 0x08, ch); put(u, 0x08, 0x78 | ch); }
    st.pitch = drum ? null : pitch;
    ws();
    st.legato = s[i] === '&';
    if (st.legato) i++;
    st.slide = false;
    if (!st.legato) put(u + Math.max(1, Math.round(dur * st.gate / 8)), 0x08, ch);
    u += dur;
  };
  let notes = 0;
  while (i < s.length) {
    ws();
    if (i >= s.length) break;
    const c = s[i++];
    const at = i;
    if ('cdefgab'.includes(c)) {
      let n = { c: 0, d: 2, e: 4, f: 5, g: 7, a: 9, b: 11 }[c];
      while ('+#-'.includes(s[i]) && s[i]) n += s[i++] === '-' ? -1 : 1;
      const dur = len();
      if (!st.inst && !st.pending) throw new Error('channel ' + ch + ': a note before an instrument');
      play(((st.oct + 1) * 12 + n + st.tr) * 64 + st.det, dur);
      notes++;
    } else if (c === 'r') u += len();
    else if (c === 'x') {
      const d = num();
      if (!(d >= 0 && d < rom.drumPatch.length)) throw new Error('channel ' + ch + ': no drum ' + d);
      st.pending = rom.patches[rom.drumPatch[d]];
      st.slide = false; st.legato = false;
      play(0, len(), { kc: rom.drumKc[d] });
      st.inst = null;                                       // (The next note loads its instrument again)
      st.pending = score.inst[st.instName] || null;
      notes++;
    } else if (c === 'o') st.oct = num();
    else if (c === '>') st.oct++;
    else if (c === '<') st.oct--;
    else if (c === 'l') st.len = num();
    else if (c === 'q') st.gate = num();
    else if (c === 'v') { st.vol = num(); levels(u); }
    else if (c === 'p') {
      ws(); const w = s[i++];
      st.pan = { l: 0x40, r: 0x80, c: 0xC0, 0: 0 }[w];
      if (st.pan === undefined) throw new Error('channel ' + ch + ': p' + w);
      if (st.inst) put(u, 0x20 + ch, st.pan | (st.inst[0] & 0x3F));
    } else if (c === '@') {
      const m = /^\w+/.exec(s.slice(i)); i += m[0].length;
      if (!score.inst[m[0]]) throw new Error('channel ' + ch + ': no instrument @' + m[0]);
      if (score.inst[m[0]].psg) throw new Error('channel ' + ch + ': the other chip\'s instrument');
      st.pending = score.inst[m[0]]; st.instName = m[0];
    } else if (c === 'I') {
      const n = num();
      if (!(n >= 0 && n < rom.patches.length)) throw new Error('channel ' + ch + ': no patch ' + n);
      score.inst['#' + n] = rom.patches[n];
      st.pending = rom.patches[n]; st.instName = '#' + n;
    } else if (c === 'k') st.tr = num();
    else if (c === 'D') st.det = num();
    else if (c === 'M') { const pms = num(); i++; const ams = num(); st.lfo = (pms & 7) << 4 | (ams & 3); put(u, 0x38 + ch, st.lfo); }
    else if (c === 'L') {
      const rate = num(); i++; const pmd = num(); i++; const amd = num(); i++; const wave = num();
      put(u, 0x18, rate); put(u, 0x19, 0x80 | pmd); put(u, 0x19, amd); put(u, 0x1B, wave & 3);
    } else if (c === 'N') {
      if (s[i] === '-') { i++; put(u, 0x0F, 0); } else put(u, 0x0F, 0x80 | (num() & 31));
    } else if (c === 'y') { const reg = num(); i++; put(u, reg, num()); }
    else if (c === '_') st.slide = true;
    else if (c === '&') st.legato = true;
    else if (c === '|') { /* (A bar line: ignored) */ }
    else throw new Error('channel ' + ch + ': what is "' + c + '" at ' + s.slice(at - 1, at + 20));
  }
  return { units: u, notes };
}

// ---- A PSG channel's MML (8-23: voice ch - 8) as writes at ticks: { t, seq, reg, val, psg }.  As play makes them
// (modules/play/mml.inc): a note's attack, its pitch and volume, at once; a slide's steps, its key off and its
// envelope's steps as they come due, before each command (the envelope's to the track's time: its steps are the
// earliest of the three, a slide's first at a tie, then a key off), and the rest after the last command
function compilePsg(ch, mml, score, rom, out) {
  const R = 4 * (ch - FM);                                  // The voice's registers: frequency (2), volume, waveform
  const put = (t, reg, val) => out.push({ t, seq: out.length, reg, val: val & 255, psg: true });
  const st = { oct: 4, len: 4, gate: 7, vol: 100, pk: psgVol(100), pan: 0xC0, inst: null, instName: '', pending: null,
    tr: 0, det: 0, pitch: null, legato: false, slide: false };
  let u = 0, cur = 0, slide = null, koff = null, env = null, ep = { a: 0, d: 0, s: 0, r: 0, p: 0 };
  const s = expand(mml);
  let i = 0;
  const fail = why => { throw new Error('channel ' + ch + ': ' + why); };
  const ws = () => { while (i < s.length && /\s/.test(s[i])) i++; };
  const num = () => {
    ws();
    const m = /^(-?)(?:\$([0-9A-Fa-f]+)|(\d+))/.exec(s.slice(i));
    if (!m) return null;
    i += m[0].length;
    return (m[1] ? -1 : 1) * (m[2] !== undefined ? parseInt(m[2], 16) : +m[3]);
  };
  const len = () => {
    let n = num(), d = UNITS / (n === null ? st.len : n);
    if (!Number.isInteger(d)) fail('length ' + n);
    let add = d;
    while (s[i] === '.') { add /= 2; d += add; i++; }
    ws();
    if (s[i] === '^') { i++; d += len(); }
    return d;
  };
  // The envelope: a segment from `from` by d (signed) over n ticks from tick T, its changes numbered 1 to |d| (the
  // jth at tick T + ceil (j n / |d|), its value from + floor (|d| k / n)); j the next one's
  const segment = (phase, from, to, n, T) => { env = { phase, from, d: to - from, n, T, j: 1 }; if (to === from) envDone(); };
  const envDone = () => {
    const { phase, T, n } = env;
    env = null;
    const fall = Math.min(ep.s, ep.p);
    if (phase === 'A' && ep.d > 0 && fall > 0) segment('D', ep.p, ep.p - fall, ep.d, T + n);
  };
  const envNext = () => {
    const ad = Math.abs(env.d), k = Math.floor((env.j * env.n + ad - 1) / ad), q = Math.floor(ad * k / env.n);
    return { t: env.T + k, q, val: env.from + Math.sign(env.d) * q };
  };
  const keyOff = () => {
    const t = koff;
    koff = null; env = null;
    if (ep.r > 0 && cur > 0) segment('R', cur, 0, ep.r, t);
    else { cur = 0; put(t, R + 2, st.pan); }
  };
  const fill = end => {
    for (;;) {
      let best = null, bt = Infinity, step = null, e = null;
      if (slide && slide.t >= slide.t1) slide = null;
      if (slide) {
        const q = Math.max(1, slide.t1 - slide.t0), k = slide.t - slide.t0;
        step = { at: tick(slide.u + k * slide.dur / q, score), pitch: Math.round(slide.from + (slide.to - slide.from) * k / q) };
        best = 'slide'; bt = step.at;
      }
      if (koff !== null && koff < bt) { best = 'koff'; bt = koff; }
      if (env) { e = envNext(); if (e.t < bt && (end || e.t <= tick(u, score))) best = 'env'; }
      if (best === 'slide') { const w = psgWord(step.pitch); put(step.at, R, w & 255); put(step.at, R + 1, w >> 8); slide.t += 2; }
      else if (best === 'koff') keyOff();
      else if (best === 'env') {
        put(e.t, R + 2, st.pan | e.val); cur = e.val; env.j = e.q + 1;
        if (e.q === Math.abs(env.d)) envDone();
      } else return;
    }
  };
  const play = (pitch, dur) => {
    const glide = st.slide && st.pitch !== null, T = tick(u, score);
    if (st.pending) { st.inst = st.pending; st.pending = null; put(T, R + 3, st.inst.wave); }
    if (glide) slide = { t: T, t0: T, t1: tick(u + dur, score), from: st.pitch, to: pitch, u, dur };
    else { const w = psgWord(pitch); put(T, R, w & 255); put(T, R + 1, w >> 8); }
    if (!(st.legato || glide)) {                            // The attack: the envelope from its start
      const [a, d, sus, r] = st.inst.env;
      ep = { a, d, s: sus, r, p: st.pk };
      env = null;
      if (a > 0) { cur = 0; put(T, R + 2, st.pan); segment('A', 0, ep.p, a, T); }
      else {
        cur = ep.p; put(T, R + 2, st.pan | cur);
        const fall = Math.min(ep.s, ep.p);
        if (ep.d > 0 && fall > 0) segment('D', ep.p, ep.p - fall, ep.d, T);
      }
    }
    st.pitch = pitch;
    ws();
    st.legato = s[i] === '&';
    if (st.legato) i++;
    st.slide = false;
    if (!st.legato) koff = tick(u + Math.max(1, Math.round(dur * st.gate / 8)), score);
    u += dur;
  };
  let notes = 0;
  while (i < s.length) {
    ws();
    if (i >= s.length) break;
    fill(false);
    const c = s[i++];
    const at = i;
    if ('cdefgab'.includes(c)) {
      let n = { c: 0, d: 2, e: 4, f: 5, g: 7, a: 9, b: 11 }[c];
      while ('+#-'.includes(s[i]) && s[i]) n += s[i++] === '-' ? -1 : 1;
      const dur = len();
      if (!st.inst && !st.pending) fail('a note before an instrument');
      play(((st.oct + 1) * 12 + n + st.tr) * 64 + st.det, dur);
      notes++;
    } else if (c === 'r') u += len();
    else if (c === 'o') st.oct = num();
    else if (c === '>') st.oct++;
    else if (c === '<') st.oct--;
    else if (c === 'l') st.len = num();
    else if (c === 'q') st.gate = num();
    else if (c === 'v') { st.vol = num(); st.pk = psgVol(st.vol); }
    else if (c === 'p') {
      ws(); const w = s[i++];
      st.pan = { l: 0x40, r: 0x80, c: 0xC0, 0: 0 }[w];
      if (st.pan === undefined) fail('p' + w);
      if (st.inst) put(tick(u, score), R + 2, st.pan | cur);
    } else if (c === '@') {
      const m = /^\w+/.exec(s.slice(i)); i += m[0].length;
      if (!score.inst[m[0]]) fail('no instrument @' + m[0]);
      if (!score.inst[m[0]].psg) fail('the other chip\'s instrument');
      st.pending = score.inst[m[0]]; st.instName = m[0];
    } else if (c === 'I') {
      const n = num();
      if (!(n >= 0 && n <= 3)) fail('no such patch ' + n);
      score.inst['%' + n] = score.inst['%' + n] || { psg: true, wave: n << 6 | 63, env: [0, 0, 0, 0] };
      st.pending = score.inst['%' + n]; st.instName = '%' + n;
    } else if (c === 'k') st.tr = num();
    else if (c === 'D') st.det = num();
    else if (c === 'y') {
      const reg = num(); i++; const val = num();
      if (!(reg >= 0 && reg < 64)) fail('y: a PSG register is 0-63');
      put(tick(u, score), reg, val);
    } else if (c === '_') st.slide = true;
    else if (c === '&') st.legato = true;
    else if (c === '|') { /* (A bar line: ignored) */ }
    else if ('xMLN'.includes(c)) fail(c + ' is the YM2151\'s');
    else fail('what is "' + c + '" at ' + s.slice(at - 1, at + 20));
  }
  fill(true);
  return { units: u, notes, pan: st.pan };
}

const tick = (u, score) => Math.round(u * score.rate * 60 * 4 / (score.tempo * UNITS));

// ---- The ZSM: the writes by tick (the same value twice in a row is left out, but for key on and off), then delays
const tickOf = (w, score) => w.t !== undefined ? w.t : tick(w.u, score);
function zsm(score, writes, mask, pmask) {
  writes.sort((a, b) => tickOf(a, score) - tickOf(b, score) || a.seq - b.seq);
  const shadow = new Array(256).fill(-1), pshadow = new Array(64).fill(-1);
  const data = [];
  let now = 0, n = 0, pairs = [];
  const flush = () => {
    for (let k = 0; k < pairs.length; k += 63 * 2) {
      const part = pairs.slice(k, k + 63 * 2);
      data.push(0x40 | part.length / 2, ...part);
    }
    pairs = [];
  };
  for (const w of writes) {
    const t = tickOf(w, score);
    if (t > now) {
      flush();
      for (let d = t - now; d > 0; d -= 127) data.push(0x80 | Math.min(d, 127));
      now = t;
    }
    if (w.psg) {                                            // The PSG's: at once
      if (pshadow[w.reg] === w.val) continue;
      pshadow[w.reg] = w.val;
      data.push(w.reg, w.val);
      n++;
      continue;
    }
    if (w.reg !== 0x08 && w.reg !== 0x01 && w.reg !== 0x19 && shadow[w.reg] === w.val) continue;
    shadow[w.reg] = w.val;
    pairs.push(w.reg, w.val);
    n++;
    if (pairs.length === 63 * 2) flush();
  }
  flush();
  data.push(0x80);
  const hdr = [0x7A, 0x6D, 1, 0, 0, 0, 0, 0, 0, mask, pmask & 255, pmask >> 8, score.rate & 255, score.rate >> 8, 0, 0];
  return { bytes: Buffer.from([...hdr, ...data]), writes: n, ticks: now };
}

// ---- A VGM of the song (to hear it on the PC), and the ROM's source
function vgm(song, rate) {
  const b = song, data = [];
  let i = 16, owed = 0, samples = 0;
  while (i < b.length) {
    const c = b[i++];
    if (c < 0x40) i++;
    else if (c === 0x40) i += b[i++] & 0x3F;
    else if (c < 0x80) for (let k = c & 0x3F; k > 0; k--, i += 2) data.push(0x54, b[i], b[i + 1]);
    else if (c === 0x80) break;
    else {
      owed += (c & 0x7F) * 44100 / rate;
      let w = Math.floor(owed); owed -= w; samples += w;
      while (w > 0) { const x = Math.min(w, 65535); data.push(0x61, x & 255, x >> 8); w -= x; }
    }
  }
  data.push(0x66);
  const v = Buffer.alloc(0x100 + data.length);
  v.write('Vgm ', 0, 'latin1');
  v.writeUInt32LE(v.length - 4, 0x04);
  v.writeUInt32LE(0x151, 0x08);
  v.writeUInt32LE(samples, 0x18);
  v.writeUInt32LE(3579545, 0x30);
  v.writeUInt32LE(0x100 - 0x34, 0x34);
  Buffer.from(data).copy(v, 0x100);
  return v;
}

const ROM_A = 0x2000, ROM_MAX = 0x3FC0;                     // Bank 2's $A000-$BFFF, then $C000-$DFBF (romsum.js)
function romSource(song, scoreName) {
  if (song.length > ROM_MAX) throw new Error('the song is ' + song.length + ' bytes: the ROM bank holds ' + ROM_MAX);
  const rows = bytes => {
    const out = [];
    for (let k = 0; k < bytes.length; k += 16) out.push('            .byte       ' + [...bytes.slice(k, k + 16)].map(x => '$' + x.toString(16).toUpperCase().padStart(2, '0')).join(','));
    return out.join('\n');
  };
  return `; ****************************************************************************
; sndtest's song, in paged ROM bank 2 (SND_SONG_BANK, at $A000: the song player reads it there, sound/player.s).
; Made by sim/tools/hysong.js from ${scoreName}: don't edit it, edit the score.  ${song.length} bytes.

.segment "SONG_A"
${rows(song.slice(0, ROM_A))}
` + (song.length > ROM_A ? `
.segment "SONG_C"
${rows(song.slice(ROM_A))}
` : '');
}

// ---- The command line
function main() {
  const args = process.argv.slice(2);
  const opt = { rom: null, vgm: null, quiet: false }, files = [];
  for (let k = 0; k < args.length; k++) {
    if (args[k] === '--rom') opt.rom = args[++k];
    else if (args[k] === '--vgm') opt.vgm = args[++k];
    else if (args[k] === '--quiet') opt.quiet = true;
    else files.push(args[k]);
  }
  if (files.length !== 2) { console.error('Usage: node hysong.js SCORE.mml SONG.zsm [--rom SONG.s] [--vgm SONG.vgm] [--quiet]'); process.exit(1); }
  const rom = romPatches();
  const score = parseScore(fs.readFileSync(files[0], 'latin1'), rom);
  const writes = [];
  let mask = 0, pmask = 0, longest = 0;
  const report = [], pans = [];
  score.tracks.forEach((mml, ch) => {
    if (!mml.trim()) return;
    const r = ch < FM ? compileTrack(ch, mml, score, rom, writes) : compilePsg(ch, mml, score, rom, writes);
    if (ch < FM) mask |= 1 << ch;
    else { pmask |= 1 << (ch - FM); pans.push([ch - FM, r.pan]); }
    longest = Math.max(longest, r.units);
    report.push(String.fromCharCode(65 + ch) + ': ' + r.notes + ' notes, ' + (r.units / UNITS) + ' bars');
  });
  let end = tick(longest, score);                           // The end: every channel off (the PSG's used), at the
  for (const w of writes) end = Math.max(end, tickOf(w, score));   //   longest's time or the last write's
  for (let ch = 0; ch < 8; ch++) writes.push({ t: end, seq: writes.length, reg: 0x08, val: ch });
  for (const [v, pan] of pans) writes.push({ t: end, seq: writes.length, reg: 4 * v + 2, val: pan, psg: true });
  const song = zsm(score, writes, mask, pmask);
  fs.writeFileSync(files[1], song.bytes);
  if (opt.vgm) fs.writeFileSync(opt.vgm, vgm(song.bytes, score.rate));
  if (opt.rom) fs.writeFileSync(opt.rom, romSource(song.bytes, path.basename(files[0])).replace(/\n/g, '\r\n'));
  if (!opt.quiet) console.log((score.title ? score.title + ': ' : '') + song.bytes.length + ' bytes, ' + song.writes + ' writes, '
    + (song.ticks / score.rate).toFixed(1) + ' s at ' + score.rate + ' Hz; ' + report.join('; '));
}

if (require.main === module) {
  try { main(); } catch (e) { console.error('hysong: ' + e.message); process.exit(1); }
}
module.exports = { parseScore, romPatches, psgWord, psgVol };
