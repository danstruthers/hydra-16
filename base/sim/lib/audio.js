// ****************************************************************************
// audio.js - the sound the simulator makes: the YM2151's samples (ym2151.js, with opm.js: 55,930 a second at its
// 3.58 MHz) and the Vera X's (vera.js: its PSG's and PCM's, 48,828 a second) mixed into one stream of 48,000 stereo
// 16-bit samples a second, as the X16's emulator mixes them (its audio.c, Frank van den Hoef's, BSD 2-clause): each
// source resampled through a 4-tap windowed sinc (its table, below), the Vera's at twice the YM2151's level (audio.c
// says so, "according to the Developer Board": a PSG voice at its loudest is an eighth of the output's range), and a
// limiter that turns the sum down when it would clip and eases back over some 0.7 s.
//   The chips make their samples in emulated time, the CPU's cycles: each before a register write changes what it
// sounds, and as pump(t) asks.  pump(t) (machine.js's run calls it as it ends) mixes what both have made up to
// cycle t, and gives it to each listener (on(fn): fn(samples), an Int16Array, left then right): the browser's
// stream (view.js's /audio) and a WAV file (run.js --wav).  No Node.js: it runs in a browser too.
// createAudio({ ym, vera }) (vera: null, no card) gives { on(fn), off(fn), pump(t), made }: made, the samples given
// out so far.
'use strict';

const RATE = 48000;                                           // The stream's samples a second
// audio.c's filter: a windowed sinc, its 4 taps at 256 fractions of a sample (taps i + 1 and i + 2 at 0 - 255,
// i and i + 3 at 256 - 511)
const FILTER = new Int16Array([
   32767, 32765, 32761, 32755, 32746, 32736, 32723, 32707, 32690, 32670, 32649, 32625, 32598, 32570, 32539, 32507,
   32472, 32435, 32395, 32354, 32310, 32265, 32217, 32167, 32115, 32061, 32004, 31946, 31885, 31823, 31758, 31691,
   31623, 31552, 31479, 31404, 31327, 31248, 31168, 31085, 31000, 30913, 30825, 30734, 30642, 30547, 30451, 30353,
   30253, 30151, 30048, 29943, 29835, 29726, 29616, 29503, 29389, 29273, 29156, 29037, 28916, 28793, 28669, 28544,
   28416, 28288, 28157, 28025, 27892, 27757, 27621, 27483, 27344, 27204, 27062, 26918, 26774, 26628, 26481, 26332,
   26182, 26031, 25879, 25726, 25571, 25416, 25259, 25101, 24942, 24782, 24621, 24459, 24296, 24132, 23967, 23801,
   23634, 23466, 23298, 23129, 22959, 22788, 22616, 22444, 22271, 22097, 21923, 21748, 21572, 21396, 21219, 21042,
   20864, 20686, 20507, 20328, 20148, 19968, 19788, 19607, 19426, 19245, 19063, 18881, 18699, 18517, 18334, 18152,
   17969, 17786, 17603, 17420, 17237, 17054, 16871, 16688, 16505, 16322, 16139, 15957, 15774, 15592, 15409, 15227,
   15046, 14864, 14683, 14502, 14321, 14141, 13961, 13781, 13602, 13423, 13245, 13067, 12890, 12713, 12536, 12360,
   12185, 12010, 11836, 11663, 11490, 11317, 11146, 10975, 10804, 10635, 10466, 10298, 10131,  9964,  9799,  9634,
    9470,  9306,  9144,  8983,  8822,  8662,  8504,  8346,  8189,  8033,  7878,  7724,  7571,  7419,  7268,  7118,
    6969,  6822,  6675,  6529,  6385,  6241,  6099,  5958,  5818,  5679,  5541,  5405,  5269,  5135,  5002,  4870,
    4739,  4610,  4482,  4355,  4229,  4104,  3981,  3859,  3738,  3619,  3500,  3383,  3268,  3153,  3040,  2928,
    2817,  2708,  2600,  2493,  2388,  2284,  2181,  2079,  1979,  1880,  1783,  1686,  1591,  1498,  1405,  1314,
    1225,  1136,  1049,   963,   879,   795,   714,   633,   554,   476,   399,   323,   249,   176,   105,    34,
     -34,  -102,  -168,  -234,  -298,  -361,  -422,  -482,  -542,  -599,  -656,  -712,  -766,  -819,  -871,  -922,
    -971, -1020, -1067, -1113, -1158, -1202, -1244, -1286, -1326, -1366, -1404, -1441, -1477, -1512, -1546, -1579,
   -1611, -1642, -1671, -1700, -1728, -1755, -1781, -1806, -1830, -1852, -1874, -1896, -1916, -1935, -1953, -1971,
   -1987, -2003, -2018, -2032, -2045, -2058, -2069, -2080, -2090, -2099, -2108, -2116, -2123, -2129, -2134, -2139,
   -2143, -2147, -2150, -2152, -2153, -2154, -2154, -2154, -2153, -2151, -2149, -2146, -2143, -2139, -2135, -2130,
   -2124, -2118, -2112, -2105, -2098, -2090, -2082, -2073, -2064, -2054, -2045, -2034, -2024, -2012, -2001, -1989,
   -1977, -1965, -1952, -1939, -1926, -1912, -1898, -1884, -1870, -1855, -1840, -1825, -1810, -1794, -1778, -1762,
   -1746, -1730, -1714, -1697, -1680, -1663, -1646, -1629, -1612, -1595, -1577, -1560, -1542, -1525, -1507, -1489,
   -1471, -1453, -1435, -1418, -1400, -1382, -1364, -1346, -1328, -1310, -1292, -1274, -1256, -1238, -1220, -1203,
   -1185, -1167, -1150, -1132, -1115, -1097, -1080, -1063, -1046, -1029, -1012,  -995,  -978,  -962,  -945,  -929,
    -912,  -896,  -880,  -864,  -849,  -833,  -817,  -802,  -787,  -772,  -757,  -742,  -727,  -713,  -699,  -684,
    -670,  -656,  -643,  -629,  -616,  -603,  -589,  -577,  -564,  -551,  -539,  -526,  -514,  -502,  -491,  -479,
    -468,  -456,  -445,  -434,  -423,  -413,  -402,  -392,  -381,  -371,  -361,  -352,  -342,  -333,  -323,  -314,
    -305,  -296,  -288,  -279,  -270,  -262,  -254,  -246,  -238,  -230,  -222,  -215,  -207,  -200,  -193,  -186,
    -179,  -172,  -165,  -158,  -152,  -145,  -139,  -133,  -127,  -120,  -114,  -108,  -103,   -97,   -91,   -85,
     -80,   -74,   -69,   -63,   -58,   -53,   -47,   -42,   -37,   -32,   -27,   -22,   -17,   -12,    -7,    -2,
]);

function createAudio(env) {
  const listeners = [];
  // A source: its samples (left, right) as they're made, how many, and where the next output sample falls among
  // them (a fraction: its rate over the stream's)
  function source(rate) { return { step: rate / RATE, buf: new Int32Array(1 << 16), n: 0, rd: 0 }; }
  const ys = source(3579545 / 64), vs = env.vera ? source(25e6 / 512) : null;
  function push(s, l, r) {
    if (2 * s.n + 2 > s.buf.length) { const b = new Int32Array(s.buf.length * 2); b.set(s.buf); s.buf = b; }
    s.buf[2 * s.n] = l; s.buf[2 * s.n + 1] = r; s.n++;
  }
  env.ym.sound((l, r) => push(ys, l, r));
  if (vs) env.vera.sound((l, r) => push(vs, l, r));
  // The output samples a source can give: each needs 4 of its samples
  const avail = s => s.n - 4 < s.rd ? 0 : Math.floor((s.n - 4 - s.rd) / s.step) + 1;
  const tap = [0, 0];
  // A source's output sample (scaled by the filter's 32767) at its read place, into tap; the place moved on
  function take(s) {
    const p = Math.floor(s.rd), i = Math.floor((s.rd - p) * 256), b = s.buf, o = 2 * p;
    const f0 = FILTER[256 + i], f1 = FILTER[i], f2 = FILTER[255 - i], f3 = FILTER[511 - i];
    tap[0] = b[o] * f0 + b[o + 2] * f1 + b[o + 4] * f2 + b[o + 6] * f3;
    tap[1] = b[o + 1] * f0 + b[o + 3] * f1 + b[o + 5] * f2 + b[o + 7] * f3;
    s.rd += s.step;
  }
  // A source's used samples dropped
  function compact(s) {
    const p = Math.floor(s.rd);
    if (!p) return;
    s.buf.copyWithin(0, 2 * p, 2 * s.n); s.n -= p; s.rd -= p;
  }
  let limiter = 65536, made = 0;
  // The samples up to cycle t made, mixed and given out
  function pump(t) {
    env.ym.soundTo(t);
    if (vs) env.vera.soundTo(t);
    const count = vs ? Math.min(avail(ys), avail(vs)) : avail(ys);
    if (count <= 0) return;
    const out = new Int16Array(2 * count);
    for (let k = 0; k < count; k++) {
      take(ys);
      let l = Math.floor(tap[0] / 32768), r = Math.floor(tap[1] / 32768);
      if (vs) { take(vs); l += Math.floor(tap[0] / 16384); r += Math.floor(tap[1] / 16384); }
      const amp = Math.max(Math.abs(l), Math.abs(r));
      if (amp > 32767) limiter = Math.min(limiter, Math.floor(32767 * 65536 / amp));
      out[2 * k] = Math.floor(l * limiter / 65536); out[2 * k + 1] = Math.floor(r * limiter / 65536);
      if (limiter < 65536) limiter++;
    }
    compact(ys);
    if (vs) compact(vs);
    made += count;
    for (const fn of listeners) fn(out);
  }
  return {
    on: fn => { listeners.push(fn); }, off: fn => { const i = listeners.indexOf(fn); if (i >= 0) listeners.splice(i, 1); },
    pump, get made() { return made; },
  };
}

module.exports = { createAudio, RATE };
