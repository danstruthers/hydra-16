// ****************************************************************************
// senses.js - Dingus Quest Zero's tests: the senses: sight, sound, scent, light.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { typed } = require('../../lib/common.js');
const {
  CHUNK_SIZE, CK_TILE, CK_ROOM, caveReport, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N, CSLOT_AT, liveChunks, stratumRec,
  dq0StatusRows, dq0Story, inTurn, dq0Answers, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need, dq0Never, dq0Carried,
  dq0Here, DQ0_I, passings, dq0Typed, dq0Rec, dq0Alone, dq0Poke, dq0OnWords, dq0Timer, dq0PortalAt
} = require('../../lib/dq0.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')

// The YM2151's key-ons in a machine's log (machine: { ymLog: true }), each its channel and its speakers as it came
// on (the register $20 + ch's RL bits): '4R', '5B', '6L' (B both, - none) in order
function YM_KEYPANS(m) {
  const r = new Uint8Array(256), ons = [];
  for (const [, reg, v] of m.ym.writes) {
    r[reg] = v;
    if (reg === 0x08 && (v & 0x78)) ons.push((v & 7) + '-LRB'[r[0x20 + (v & 7)] >> 6]);
  }
  return ons;
}

// The senses' looks (dq0looks's): at each reply the senses give (dq0p's hub_reply: a look's, a denizen's senses'), the
// request (its first 16 bytes), the senses' record (SIGHT_BANK's first 2K) and each live chunk's seen and visible marks
// (its squares' T_SEEN, T_VIS, its rooms' R_VISITED; a chunk with none left out), hashed: a line each, "game request
// its-hash sight-hash sn:cx,cy=hash ...", game the games' count of "> quit" told before it
const LOOKS_REF = path.join(TESTS, 'dq0', 'looks.ref');
const lookHash = b => require('crypto').createHash('sha1').update(b).digest('hex').slice(0, 8);
function lookSnap(m, ptr, game) {
  const banks = m.sharedBanks(), keys = Object.keys(banks).map(Number).sort((a, b) => a - b);
  const hub = keys.find(k => banks[k][0] === 0x44 && banks[k][1] === 0x51);
  if (hub === undefined) return null;
  const w = banks[hub + WORLD_BANK_N], ch = [];
  for (let k = 0; k < 24; k++) {
    const s = CSLOT_AT + k * 8, b = banks[hub + 5 + Math.floor(k / 3)];
    if (!w[s] || !b) continue;
    const o = (k % 3) * CHUNK_SIZE, t = Buffer.alloc(1024 + 6);
    for (let i = 0; i < 1024; i++) t[i] = b[o + CK_TILE + i] & 0xC0;
    for (let r = 0; r < 6; r++) t[1024 + r] = b[o + CK_ROOM + r * 8 + 4] & 1;
    if (t.some(v => v)) ch.push(w[s + 1] + ':' + (w[s + 2] | w[s + 3] << 8) + ',' + (w[s + 4] | w[s + 5] << 8) + '=' + lookHash(t));
  }
  const { L } = require('../../../sim/lib/dq0meaning.js').layout(), n = ptr - 0x8000 === L.dsenseAt ? L.dsenseSize : 16;   // (The
  // denizens' senses' request its own bytes: the hub's ui_cmd after it, 2026-10-20)
  return [game, ptr.toString(16), lookHash(banks[hub].subarray(ptr - 0x8000, ptr - 0x8000 + n)), lookHash(banks[hub + 19].subarray(0, 0x800)),
    ...ch.sort()].join(' ');
}

// (dq0traces' budget: a square of a walk at the turns pace, its look every third square, the denizens' turn at each
// slice, the world's journal and its store's taking it in: cycles; measured 1,460,275 a square on its walk back across the
// cell, 5 squares; the same walks at seed 3 with no traces, the slice before, within 0.2-0.6% of them: the journal's
// entry a step, the store's taking 24 in a part's swap; 1,800,000 since 2026-10-29: the sound layer's pulse, once a
// denizens' turn after a step, and the denizens' senses' own part, slices 4.5 and 4.6); 1,900,000 since 2026-10-31
// (1,863,195: the minds' steps looking at each square's chunk for its bad air, a swimmer's too, the giant rats')
const TRACE_STEP = 1920000;   // (1950000 for a day, 2026-11-02, part 14's some 30 bytes more, digs told why; 1,901,748 to 1,911,662 since the margins' pass, 2026-11-03, by the tasks' timing and the code's layout)

// dq0lookfar: a look one way into new ground (the delays' round, 2026-11-05: the first look ahead of a game made every chunk its
// grid touched, up to nine, behind it as well, at the normal pace 12M cycles each, 28-55 s on the board, the world and the
// denizens stopped behind it; seed 3's five, 78.5M), by the maker's make_chunk and dq0Timer's times, at the normal pace.  At
// seed 3's start the wall a few squares ahead: nothing made, its whole within budget; asked for again the same; the map drawn
// with the maker first (16.3M before, 9.4M), its first words at once.  Then the character put where the line ahead reaches
// a chunk not made yet past a glance's reach (its sight poked to 0: a glance 4 squares, a look one way 8; a straight run of
// floor to the chunk's edge): that chunk made as the eyes reach it, by the maker in the foreground (its elapsed near its own
// 5.5M, 12M before)
function dq0LookFarTest() {
  const FIRST = 8000000, MAP = 11000000, ACROSS = 24000000;
  const pre = ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl'];
  const cmds = [{ line: 'look ahead', want: 'You peer ahead' }, { line: 'map', want: 'The map' }, { line: 'turn left', want: 'You turn' }, { line: 'look ahead', want: 'You peer ahead' }, { line: 'look ahead', want: 'You peer ahead' }];
  const lines = [...pre, 'dq0 -p n -s 3 -c /ram/c.hm -T', ...cmds.map(c => c.line), 'quit', 'echo $status'];
  return {
    name: 'dq0lookfar', near: ['senses', 'time'], meaning: true, outfit: true,
    what: 'Dingus Quest Zero\'s look one way into new ground (the delays\' round, 2026-11-05): the first look ahead makes only the chunks the eyes reach past a glance\'s: none with a wall in the way (seed 3\'s five before, 78.5M cycles; now within 8M), the map drawn with the maker first (within 11M; 16.3M before), a chunk the line ahead does reach made then',
    init: 't_rc', cycles: 900e6,
    machine: { input: lines.map(l => '\u0101' + l + '\r').join('') },
    expect: ['% echo $status'],
    start(m) {
      this.timer = dq0Timer(m);
      const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0m', 'dq0m.lbl'), 'latin1');
      const make = parseInt(/^al ([0-9A-F]+) \._make_chunk$/m.exec(lbl)[1], 16);
      const cpu = m.cpu, step = cpu.step.bind(cpu), made = this.made = [], enters = this.enters = [], a = m.acia, read = a.read, T = this;
      a.read = (r, c) => { if (r === 0 && a.rdrf && a.rx === 13) enters.push(a.rxAt); return read(r, c); };
      cpu.step = v => {
        if (cpu.PC === make && m.dq0 && m.dq0.bank && m.dq0.bank(0) && m.T === m.dq0.bank(0)[12 + 5]) made.push(cpu.cyc);
        return step(v);
      };
      dq0Poke(m, [{ g: 1, line: 'look ahead', n: 3, call: (h, L) => {
        const sn = h[L.pcSn], S = stratumRec(m, sn), live = liveChunks(m, sn), TILE = 443, FACE = { E: 2, W: 6, N: 0, S: 4 };
        const open = (b, x, y) => { const t = b[TILE + y * 32 + x] & 15; return t === 1 || t === 4; };
        h[L.pcsum + L.sSight] = 0;
        for (const k of Object.keys(live)) {
          const [cx, cy] = k.split(',').map(Number), b = live[k];
          const tries = [['E', 1, 0], ['W', -1, 0], ['S', 0, 1], ['N', 0, -1]];
          for (const [nm, dx, dy] of tries) {
            if (live[(cx + dx) + ',' + (cy + dy)] || cx + dx < 0 || cy + dy < 0 || (cx + dx + 1) * 32 > S.w || (cy + dy + 1) * 32 > S.h) continue;
            for (let o = 2; o < 30 && !T.put; o++) {
              let run = true;
              for (let d = 0; d <= 4 && run; d++) { const x = dx ? (dx > 0 ? 31 - d : d) : o, y = dy ? (dy > 0 ? 31 - d : d) : o; run = open(b, x, y) && (b[1467 + y * 32 + x] === b[1467 + (dy ? (dy > 0 ? 31 : 0) : y) * 32 + (dx ? (dx > 0 ? 31 : 0) : x)]); }
              if (!run) continue;
              const x = S.x0 + cx * 32 + (dx ? (dx > 0 ? 27 : 4) : o), y = S.y0 + cy * 32 + (dy ? (dy > 0 ? 27 : 4) : o);
              h[L.pcX] = x & 255; h[L.pcX + 1] = x >> 8; h[L.pcY] = y & 255; h[L.pcY + 1] = y >> 8; h[L.pcFace] = FACE[nm];
              T.put = { k, to: (cx + dx) + ',' + (cy + dy), x, y, nm };
            }
          }
        }
      } }]);
    },
    check(m, out) {
      const f = [], notes = this.notes = [], r = this.timer(lines, cmds), E = this.enters, within = (a, b) => this.made.filter(c => c >= a && c < b).length;
      const at = i => E[pre.length + 1 + i], over = (x, name, max) => { notes.push(name + ': first letter ' + x.first + ', whole ' + x.whole); if (!(x.whole !== null && x.whole <= max)) f.push(name + ': its whole ' + x.whole + ' cycles from Enter (budget ' + max + ')'); };
      over(r[0], 'the first look ahead', FIRST);
      notes.push('chunks made in it: ' + within(at(0), at(1)));
      if (within(at(0), at(1))) f.push('the first look ahead, a wall in the way, made ' + within(at(0), at(1)) + ' chunks');
      over(r[1], 'the map', MAP);
      if (!(r[1].first !== null && r[1].first <= 1200000)) f.push('the map: its first words ' + r[1].first + ' cycles from Enter');
      const put = this.put;
      notes.push('put: ' + JSON.stringify(put || null) + '; chunks made by the look across: ' + within(at(4), at(5)));
      if (!put) f.push('no straight run to a chunk\'s edge in the scene');
      else if (within(at(4), at(5)) < 1) f.push('the look ahead along a run to a chunk not made: none made');
      over(r[4], 'the look across', ACROSS);
      return f;
    },
    sdcard: true
  };
}

// dq0askheld: a soft question standing as other news of the long task comes (the delays' round, fight soak's seed 358, on the
// tree before it: "Keep waiting?", then more of its news, "go on" taken for a command held till the character was free, the
// world waiting on the answer: no prompt again, a true hang).  At seed 3 digging, "You're tiring.  Keep digging?", a light's
// burning low told (E_TASK's TE_LOW, put in the world's ring as the question's told); "yes" still its answer: on it goes, and the
// next question's "no" stops it, the prompt back
function dq0AskHeldTest() {
  const lines = ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', 'dig down', 'yes', 'no', 'inventory', 'quit', 'echo $status'];
  return {
    name: 'dq0askheld', near: ['time'], asks: true,
    what: 'Dingus Quest Zero\'s soft question standing as other news of the long task comes (seed 358\'s hang in the fight soak, the delays\' round): a torch burning low told as "Keep digging?" waits; "yes" goes on with the dig, "no" stops it, the prompt back',
    init: 't_rc', cycles: 1200e6,
    machine: { input: lines.map(l => '\u0101' + l + '\r').join('') },
    expect: ['% echo $status', 'You carry'],
    start(m) {
      dq0OnWords(m, [{ g: 1, line: 'dig down', n: 1, words: 'Keep digging?', call: () => {
        const h = m.dq0.bank(0), { L, K } = m.dq0.layout(), i = (h[L.eqHead] | h[L.eqHead + 1] << 8) % L.eqSize, o = L.eq + i * L.msg;
        for (let k = 0; k < L.msg; k++) h[o + k] = 0;
        h[o + L.mKind] = K.E_TASK;
        h[o + L.mW1] = K.TE_LOW;
        const n = (h[L.eqHead] | h[L.eqHead + 1] << 8) + 1; h[L.eqHead] = n & 255; h[L.eqHead + 1] = n >> 8 & 255;
        m.poked = true;
      } }]);
    },
    check(m, out) {
      const f = [];
      if (!m.poked) f.push('the question never asked');
      if (!/low/.test(dq0Story(out))) f.push('its news not told');
      return f;
    },
    sdcard: true
  };
}

// dq0restbusy: an eight-hour rest near hostile folk it can't reach or that lie in wait (the delays' round, fight soak's seeds 326 and
// 136 on the tree before it: "hung" after "rest", none hung: robbers lying in wait a second at a time, or a hunter stuck on a scent
// asking each second, 93,000 and 440,000 cycles a time, kept the rest to ten-second slices, 4.9G cycles of it, 23 minutes on the
// board).  At the turns pace (a long wait's the same at the real paces), dq0lurk's bandits about in the dark: its cycles a minute
// of game time within budget.  Since 2026-11-06 (the user's: robbers come and rob a resting character) none lies in wait for one
// at rest, and those that reach it rob it: the scene a sleep, the character's Awareness 5, so one robs it unawares and it sleeps
// on, the rest of the band about it (an awake rest's seen them coming in the cell's light, and stopped, a third of a minute in)
function dq0RestBusyTest() {
  const MAX = 3400000;   // (Cycles a game minute of the rest: 1.63M or 2.87M now, by the power-up; more than 16M before, a game not over in 8,000M)
  const lines = ['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'echo ml awa 5 >> /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
    'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g', 'douse the torch', 'wait', 'wait', 'wait', 'sleep', 'quit', 'echo $status'];
  return {
    name: 'dq0restbusy', near: ['time', 'denizens'], initiative: true,
    what: 'Dingus Quest Zero\'s long rest with hostile folk about that lie in wait or can\'t reach it (seeds 326 and 136 of the fight soak, the delays\' round): robbers about in the dark as an eight-hour sleep goes (since 2026-11-06 one comes and robs it unawares, and it sleeps on): thirty times the while between their decisions, its cycles a game minute within budget',
    init: 't_rc', cycles: 3000e6,
    machine: { input: lines.map(l => '\u0101' + l + '\r').join('') },
    expect: ['% echo $status'],
    start(m) {
      const a = m.acia, read = a.read, R = this.R = {}, cpu = m.cpu;
      let cur = '';
      a.read = (r, c) => {
        if (r === 0 && a.rdrf) {
          const ch = a.rx;
          if (ch === 13) {
            const h = m.dq0 && m.dq0.bank && m.dq0.bank(0);
            if (h && h[0] === 0x44) { const { L } = m.dq0.layout(), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16; if (cur === 'sleep') { R.t0 = cpu.cyc; R.n0 = now; } if (cur === 'quit') { R.t1 = cpu.cyc; R.n1 = now; } }
            cur = '';
          } else if (ch >= 32 && ch < 127) cur += String.fromCharCode(ch);
        }
        return read(r, c);
      };
    },
    check() {
      const f = [], notes = this.notes = [], R = this.R, mins = (R.n1 - R.n0) / 600, per = (R.t1 - R.t0) / (mins || 1);
      notes.push('the rest: ' + Math.round(R.t1 - R.t0) + ' cycles, ' + mins.toFixed(1) + ' game minutes, ' + Math.round(per) + ' cycles a minute');
      if (!(per <= MAX)) f.push('the rest ' + Math.round(per) + ' cycles a game minute (budget ' + MAX + ')');
      return f;
    },
    sdcard: true
  };
}

module.exports = {
  tests: [
    {
      // (The by-seed sky left on since 2026-11-03: it passes with it, its step's cost budget measured with the sky's looks)
      name: 'dq0traces', meaning: true, what: 'Dingus Quest Zero\'s traces on the floor (slice 4.1, DUNGEON.md\'s traces on the floor, HOUSE.md\'s light and traces; the user\'s, 2026-10-16 and 21), by the world\'s reply (the UI bank\'s tread), its words and the denizens\' bodies: at seed 3 by the way down (a flagged cell, the stratum\'s dust deep), the floor examined (no test), the character\'s own prints read back after a walk across the room and back (its trodden dust; Tracking 80, its own bare feet), the giant rats\' tracks a while on (small clawed feet), their musky smell sniffed, the smoke its torch left on its trail smelt once it\'s put out and gone three hours on (a scent fading with time: its strength halving as its age doubles); a game kept (dq0 -d), its prints in the save folder\'s region file (trSXXYY.dat) and read back as it\'s carried on; a step\'s cost within its budget; at seed 4 with a cult in the theme, its lair\'s floor swept clean, no tracks there (dq0scent: the denizens following a scent)',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml trk 80 >> /ram/c.hm', 'echo ml awa 80 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'examine the floor', 'e', 'w', 'examine the floor', 'track', 'wait 5 minutes', 'track', 'sniff',
        'douse the torch', 'sniff', 'wait 3 hours', 'sniff', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/kept', 'e', 'w', 'quit',
        'ls /ram/kept/d1', 'dq0 -d /ram/kept', 'examine the floor', 'quit',
        'mkdir /ram/g', "echo 'faction | Cult | cultist | 0-0 | 90 | priest | 0 | i | -' > /ram/g/theme.txt", 'dq0 -p t -s 4 -c /ram/c.hm -T -d /ram/g',
        'examine the floor', 'track', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], rd = t => (t.after && t.after.tread) || {};
        const has = (t, k) => (rd(t).trails || []).some(e => e.kind === k);
        const x0 = T.of('examine the floor'), r0 = rd(x0);
        if (r0.how !== 'TRC_FLOOR' || r0.result !== 'TRR_OK' || r0.floor !== 'MAT_FLAGS' || r0.dust < 3 || !/^The floor here is flagged, (grey|deep) /.test(r0.text || ''))
          f.push('the floor examined: ' + JSON.stringify(r0));
        if (x0.after && x0.at && x0.after.now - x0.at.now !== 50) f.push('the floor examined in ' + (x0.after.now - x0.at.now) + ' tenths, not 50');
        const x1 = T.of('examine the floor', x0.k + 1), tr1 = T.after(x1, 'track'), r1 = rd(tr1);
        if (!rd(x1).trod || !/It's been trodden/.test(rd(x1).text || '')) f.push('its own walk not seen in the dust: ' + JSON.stringify(rd(x1)));
        if (!has(tr1, 0) || (r1.grade > 0 && !/your own/.test(r1.text || ''))) f.push('its own prints not read back: ' + JSON.stringify(r1));
        if (!tr1.replies.some(r => /^slot/.test(r.req) && r.a === m.dq0.layout().K.P_TRACK)) f.push('no Tracking test asked: ' + JSON.stringify(tr1.replies.map(r => [r.req, r.kind, r.a])));
        if (tr1.after && tr1.at && tr1.after.now - tr1.at.now !== 600) f.push('a track in ' + (tr1.after.now - tr1.at.now) + ' tenths, not a minute');
        const tr2 = T.after(tr1, 'track'), r2 = rd(tr2), rat = (rd(tr2).trails || []).find(e => e.kind === 1);
        if (!rat || rat.name !== 'giant rat' || rat.what !== 'small clawed feet' || (r2.grade > 0 && !/giant rat/.test(r2.text || ''))) f.push('the rats\' tracks not read: ' + JSON.stringify(r2));
        const sn = T.after(tr2, 'sniff'), rs = rd(sn), musk = (rs.trails || []).find(e => e.kind === 1);
        if (rs.how !== 'TRC_SMELL' || (rs.grade !== -2 && (!musk || musk.what !== 'a musky smell' || !/musky smell/i.test(rs.text || '')))) f.push('the rats\' smell not smelt: ' + JSON.stringify(rs));
        const s1 = T.after(sn, 'sniff'), s2 = T.after(s1, 'sniff');
        if (rd(s1).grade !== -2 && (!rd(s1).smoke || !/smoke/.test(rd(s1).text || ''))) f.push('its torch\'s smoke not smelt on its trail, put out: ' + JSON.stringify(rd(s1)));
        if (rd(s2).smoke) f.push('its torch\'s smoke still smelt three hours on: ' + JSON.stringify(rd(s2)));
        const e = T.of('w'), sq = e.at && e.after ? Math.abs(e.after.pc.x - e.at.pc.x) + Math.abs(e.after.pc.y - e.at.pc.y) : 0;
        const per = sq ? (e.after.cyc - e.at.cyc) / sq : Infinity;
        this.notes = ['a step\'s cost ' + Math.round(per) + ' cycles (' + sq + ' squares walked, budget ' + TRACE_STEP + ')'];
        if (!(per <= TRACE_STEP)) f.push(this.notes[0]);
        const k0 = T.of('dq0 -d /ram/kept'), kx = T.after(k0, 'examine the floor');
        if (!/tr0[0-9a-f]{4}\.da[t12]/.test(out)) f.push('no traces\' region file in the save folder');
        if (!rd(kx).trod) f.push('the game carried on, its walk\'s prints not read back from the save: ' + JSON.stringify(rd(kx)));
        const c0 = T.of('dq0 -p t -s 4 -c /ram/c.hm -T -d /ram/g'), cx = T.after(c0, 'examine the floor'), ct = T.after(cx, 'track');
        if (rd(cx).dust !== 7 || !/swept clean/.test(rd(cx).text || '')) f.push('the cult\'s lair\'s floor not swept clean: ' + JSON.stringify(rd(cx)));
        if ((rd(ct).trails || []).length) f.push('tracks read on a swept floor: ' + JSON.stringify(rd(ct)));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0scent', what: 'Dingus Quest Zero\'s denizens following a scent (slice 4.1, DENIZENS.md\'s a scent followed; the user\'s, 2026-10-21: keen noses follow a scent far longer than an hour), by their bodies: at seed 3 with the hand-written theme\'s hostile giant rats (a keen nose, 7: K_NOSE), the scene set (2026-11-03: the dice no longer fall on one; dq0Poke\'s): the rats held in the room, beside the squares the character walks, as it goes out through the doorway and round the corner along the passage west, there its torch put out, still there five minutes: the rats let go come after it through the doorway along the squares it walked (the world\'s journal of its squares, its store\'s older trail: from the room none of them can see it in the dark, nor go straight at it through the wall), two of them on the passage within four squares of it before the end (before the traces, they went back to the room and their lair); and the same walk with the rats held at the room\'s far end, none beside a square of its trail: there they stay, five minutes on (their noses its trail\'s, no sound or sight of it fetching them) (Club 60 in its file: its fights with them not the end of it)',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt', 'echo ml clu 60 >> /ram/c.hm',
        'mkdir /ram/h', 'cp /ram/g/theme.txt /ram/h/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'step west', 's', 'go west', 'go west', 'douse the lit torch', ...Array(5).fill('wait 1 minute'), 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/h', 'step west', 's', 'go west', 'go west', 'douse the lit torch', ...Array(5).fill('wait 1 minute'), 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {      // (The scene set, 2026-11-03: the rats put in the room and held, till it's round the corner, then on its trail)
        const hold = (h, L) => { const now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, t = now + 900; for (let k = 0; k < L.nactors; k++) { const o = L.actor + k * L.actorSize + L.aReady; if (h[L.actor + k * L.actorSize + L.aName]) { h[o] = t & 255; h[o + 1] = t >> 8 & 255; h[o + 2] = t >> 16 & 255; h[o + 3] = 0; } } };
        const room = sq => (h, L) => { const R = dq0Rec(m); let n = 0; for (let k = 0; k < L.nactors; k++) if (h[L.actor + k * L.actorSize + L.aName]) { const a = L.actor + k * L.actorSize, o = R.at(h[a + L.aName + 12]), [x, y] = sq[n++ & 3]; h[a + L.aX] = R.mem[o + R.X] = x & 255; h[a + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8; h[a + L.aY] = R.mem[o + R.Y] = y & 255; h[a + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8; } h[L.denSeq] = h[L.denSeq] + 1 & 255; hold(h, L); };
        const held = g => [{ g, line: 'douse the lit torch', n: 1, call: hold }, { g, line: 's', n: 1, call: hold }, { g, line: 'go west', n: 1, call: hold }, { g, line: 'go west', n: 2, call: hold }];
        dq0Poke(m, [{ g: 1, line: 'step west', n: 1, call: room([[32722, 32721], [32722, 32722], [32721, 32721], [32721, 32722]]) }, ...held(1),     // (Beside its trail)
          { g: 2, line: 'step west', n: 1, call: room([[32719, 32721], [32720, 32721], [32719, 32722], [32720, 32722]]) }, ...held(2)]);              // (Not)
      },
      check(m) {
        const T = dq0Turns(m), f = [];
        const h0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g'), waits = T.filter(t => t.k > h0.k && t.line === 'wait 1 minute'), w = waits.slice(0, 5), w1 = waits.slice(5), end = w.length ? w[w.length - 1].after : null;
        if (!end) return f.concat(['the hostile rats\' game not played']);
        // (The passage it walked, west along its row from the doorway, to the corner and round it: the map's, seed 3's)
        const walked = a => a.y === 32724 && a.x >= 32715 && a.x <= 32723;
        const nearAt = e => e.actors.filter(a => a.state && a.name === 'giant rat' && a.attitude === 'AT_HOSTILE' && a.doing !== 'DO_DOWN' &&
          Math.max(Math.abs(a.x - e.pc.x), Math.abs(a.y - e.pc.y)) <= 4 && walked(a));
        const near = w.map(t => t.at ? nearAt(t.at) : []).concat([nearAt(end)]).reduce((b, n) => n.length > b.length ? n : b, []);   // (As they come up; a fight may scatter them after)
        if (end.pc.light) f.push('the character not in the dark');
        // (Slices 4.5 and 4.6: the rats on its scent stalk it, their Stealth tested, the denizens' slot's, their steps
        // the quieter; every sound's way from the character to a rat no shorter than the line, round the corner longer)
        const g = m.dq0.games[0] || { replies: [], dens: [] }, rs = g.replies.filter(r => r.req === 'dsense' && r.cost !== 255);
        if (!g.replies.some(r => r.req === 'dslot' && r.kind === m.dq0.layout().K.RS_TEST && r.a === 4 && r.actor)) f.push("no rat's Stealth tested, stalking");
        if (!g.dens.some(e => e.kind === 'E_DEN' && e.de === 'DE_MOVED' && e.b === 0)) f.push("no rat's step silent, stalking");
        const short = rs.filter(r => r.cost < Math.max(Math.abs(r.x - r.pc.x), Math.abs(r.y - r.pc.y)));
        if (short.length) f.push("a sound's way shorter than the line: " + JSON.stringify(short[0]));
        if (!rs.some(r => r.cost > Math.max(Math.abs(r.x - r.pc.x), Math.abs(r.y - r.pc.y)))) f.push('no sound round the corner, the long way');
        if (near.length < 2) f.push('the rats not come after it along the passage: the character at ' + end.pc.x + ',' + end.pc.y + ', the rats ' + JSON.stringify(end.actors.map(a => [a.x, a.y, a.doing])));
        if (!w.some(t => t.at && t.at.actors.concat(t.after ? t.after.actors : []).some(a => a.name === 'giant rat' && a.doing === 'DO_COMING' && walked(a)))) f.push('no rat coming on along the passage (DO_COMING): ' + JSON.stringify(end.actors.map(a => [a.x, a.y, a.doing])));
        // (Held where no square of its trail's beside them: still in the room, its trail their way out)
        if (w1.length !== 5 || !w1.every(t => t.after)) f.push('the second game not played');
        else {
          const out = w1.flatMap(t => t.after.actors).filter(a => a.name === 'giant rat' && (a.y > 32722 || a.x > 32721));
          if (out.length) f.push('rats with no trail beside them come out all the same: ' + JSON.stringify(out.map(a => [a.x, a.y, a.doing])));
        }
        return f;
      },
      sdcard: true
    },

    {
      name: 'dq0pose', meaning: true, what: 'Dingus Quest Zero\'s body positions (slice 4.6, HOUSE.md\'s body position, for review), by the hub\'s state and the words: at seed 3 by the way down the character crouches ("You crouch.", a second, the status line\'s CROUCHED), is told it\'s crouching already, kneels (the grammar\'s "kneel", the UI\'s by the word), sits, stands ("You get to your feet."), is told it\'s on its feet already, lies down, gets up (the book\'s Rise), crouches and steps forward crouched (a low walk: still crouched, a slow walk\'s pace), sits and steps forward (up first, a moment: "You get to your feet.", standing after)',
      init: 't_rc', cycles: 440e6,   // (400e6 till 2026-10-30: 398.6M with the answers' work, the budget some 10% over it)
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'crouch', 'crouch', 'kneel',
        'sit', 'stand', 'stand', 'lie down', 'get up', 'crouch', 'step forward', 'sit', 'step forward', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out), at = l => T.of(l), pos = t => t.after ? t.after.pc.pos : -1;
        const want = [['crouch', 2], ['kneel', 3], ['sit', 4], ['stand', 0], ['lie down', 1], ['get up', 0]];
        for (const [l, p] of want) if (pos(at(l)) !== p) f.push(l + ': the position ' + pos(at(l)) + ', not ' + p);
        const c2 = T.of('crouch', at('crouch').k + 1), s2 = T.of('stand', at('stand').k + 1), c3 = T.of('crouch', at('get up').k);
        if (pos(c2) !== 2) f.push('crouch again: not crouching');
        if (pos(s2) !== 0) f.push('stand again: not standing');
        const st1 = T.of('step forward', c3.k), si2 = T.of('sit', st1.k), st2 = T.of('step forward', si2.k);
        if (pos(st1) !== 2) f.push('a step crouched: not crouching after it (' + pos(st1) + ')');
        if (st1.after && st1.at && st1.after.pc.x === st1.at.pc.x && st1.after.pc.y === st1.at.pc.y) f.push('a step crouched: no step');
        if (pos(st2) !== 0) f.push('a step sitting: not standing after it (' + pos(st2) + ')');
        if (at('crouch').after && at('crouch').at && at('crouch').after.now - at('crouch').at.now < 10) f.push('crouching took no time');
        for (const w of ['You crouch.', 'You are crouching already.', 'You kneel.', 'You sit down.', 'You get to your feet.',
          'You are on your feet already.', 'You lie down.'])
          if (!o.includes(w)) f.push('not said: ' + w);
        if (!out.includes('CROUCHED')) f.push('the status line never CROUCHED');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0quiet', meaning: true, what: 'Dingus Quest Zero\'s moving quietly (slice 4.6, D15: Stealth tested as it starts, again as something changes; HOUSE.md\'s sound and stealth, for review), by the referee\'s replies and the hub\'s state: at seed 3 by the way down, a step at a walk as loud as its pace (the senses\' to reckon, the hub\'s pc_noise 0xFF, no Stealth test); creeping, its first step tested (the world\'s slot: RS_TEST, P_STEALTH), its loudness the grade\'s (a critical success 0, a marginal 1, a marginal failure 2, a critical 3), its next not tested again, the same loudness; crouched, tested again (its position changed); a walk again, the senses\' to reckon, no test',
      init: 't_rc', cycles: 700e6,   // (400e6 till 2026-10-30: 399.4M at the morale's merge, over it with succession's parts; 2026-11-03, a creep again and a stair)
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'step forward', 'creep',
        'step back', 'step forward', 'crouch', 'step back', 'stand', 'walk', 'step forward', 'wait', 'creep', 'step back', 'step forward', 'd', 'step forward', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], STEALTH = 4, RS_TEST = m.dq0.layout().K.RS_TEST;
        const tests = t => dq0Rep(t, 'slot0', { kind: RS_TEST, a: STEALTH }), loud = g => g >= 2 ? 0 : g === 1 ? 1 : g === -1 ? 2 : 3;
        const s0 = T.of('step forward'), cr = T.of('creep'), s1 = T.of('step back', cr.k), s2 = T.of('step forward', s1.k), cd = T.of('crouch', s2.k);
        const s3 = T.of('step back', cd.k), wk = T.of('walk', s3.k), s4 = T.of('step forward', wk.k), w = T.of('wait', s4.k), nz = t => t.after ? t.after.pc.noise : -1;
        if (nz(s0) !== 255 || tests(s0).length) f.push('a walk\'s step: its noise ' + nz(s0) + ', ' + tests(s0).length + ' Stealth tests');
        const t1 = tests(s1);
        if (t1.length !== 1) f.push('the first creeping step: ' + t1.length + ' Stealth tests');
        else if (nz(s1) !== loud(t1[0].grade)) f.push('the first creeping step: its noise ' + nz(s1) + ', its grade ' + t1[0].grade);
        if (tests(s2).length) f.push('the second creeping step tested again');
        if (t1.length && nz(s2) !== nz(s1)) f.push('the second creeping step not as loud as the first');
        const t3 = tests(s3);
        if (t3.length !== 1) f.push('the crouched creeping step: ' + t3.length + ' Stealth tests (its position changed)');
        else if (t3[0].steps !== 1) f.push('the crouched creeping step: its test ' + t3[0].steps + ' steps, not 1');
        if (nz(s4) !== 255 || tests(s4).length) f.push('a walk again: its noise ' + nz(s4) + ' (not the senses), ' + tests(s4).length + ' Stealth tests');
        const c2 = T.of('creep', w.k), s5 = T.of('step back', c2.k), s6 = T.of('step forward', s5.k), dn = T.of('d', s6.k), s7 = T.of('step forward', dn.k);   // (HOUSE.md's D15: as it starts after a walk, and on a new stratum, the check of 2026-11-03)
        if (tests(s5).length !== 1) f.push('creeping again after a walk: ' + tests(s5).length + ' Stealth tests, not 1');
        if (tests(s6).length) f.push('the next creeping step tested again');
        if (tests(dn).length + tests(s7).length < 1) f.push('creeping on a new stratum: not tested again');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hide', meaning: true, near: ['senses'], what: 'Dingus Quest Zero\'s hiding oneself (A12: HOUSE.md\'s hiding oneself; the book\'s Stealth, Skills 9), the character\'s side, by the hub and its words: with a light on it, refused, no time; a lit torch laid down, no cover, refused; in the dark, begun with its words at once, the time its ten seconds, hidden then and crouched, said by "status", its first words before it\'s done; a look, a listen, a wait, an inventory don\'t end it; a creep keeps it, a walk ends it; a thing handled ends it; standing up ends it; "stop hiding" ends it; a hide stopped as it\'s begun isn\'t hidden; a cask, a heap of straw named and walked to, behind it in the light, its steps the cover\'s; lit afterwards ends it; kept with the game',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g',
        "dq0 -p t -s 7 -c /ram/c.hm -T -d /ram/g -k 'heap of straw'", 'hide', 'drop the torch', 'hide', 'drop the straw', 'hide', 'status', 'look', 'hide', 'stand', 'take the torch', 'douse the torch', 'hide', 'look', 'listen', 'wait', 'i', 'search', 'creep', 'step forward', 'step back', 'walk', 'step forward', 'hide',
        'stop hiding', 'hide', 'drop the dagger', 'hide', 'stand', 'hide', 'status', 'quit', 'dq0 -d /ram/g', 'status', 'quit', "dq0 -p t -s 7 -c /ram/c.hm -T -k cask", 'drop the torch', 'drop the cask', 'hide', 'creep', 'step forward', 'step forward', 'hide behind the sofa', 'walk', 'hide behind the cask', 'status', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        // (The third game's scene its creeping: the giant rats' looks spent, none to find it)
        const none = (h, L) => { for (let k = 0; k < L.nactors; k++) h[L.hlook + k] = 4; };
        dq0Poke(m, [{ g: 3, line: 'creep', n: 1, call: none }, { g: 3, line: 'step forward', n: 1, call: none }, { g: 3, line: 'step forward', n: 2, call: none }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), hid = t => t && t.after ? t.after.pc.hid : -1, pos = t => t.after ? t.after.pc.pos : -1, now = t => t.after ? t.after.now : 0;
        const h0 = T.of('hide');
        if (hid(h0) !== 0) f.push('hidden with a light on it');
        if (!/Not with a light on you/.test(s)) f.push('with a light: not told why');
        const h1 = T.after(T.of('drop the torch'), 'hide');
        if (hid(h1) !== 0) f.push('hidden on a lit square, no cover');
        if (!/nothing here to hide behind/.test(s)) f.push('no cover, lit: not told why');
        const h2 = T.after(T.of('drop the straw'), 'hide');
        if (hid(h2) !== 3) f.push('in a heap, lit: its hiding ' + hid(h2) + ', not 3 (2 steps)');
        if (!/You set to hiding/.test(s)) f.push('its first words not told');
        if (!/You get down into the straw, out of sight/.test(s)) f.push('in the heap: not told');
        if (pos(h2) !== 2) f.push('hidden, not crouched: ' + pos(h2));
        if (!/You are hidden/.test(s)) f.push('status: not told it\'s hidden');
        if (hid(T.after(h2, 'look')) !== 3) f.push('a look ended it');
        if (!/You're hidden already/.test(s)) f.push('hidden already: not told');
        const st = T.after(h2, 'stand');
        if (hid(st) !== 0) f.push('standing up: still hidden');
        const dk = T.after(st, 'douse the torch'), h3 = T.after(dk, 'hide');
        if (hid(h3) !== 5) f.push('in the heap in the dark: its hiding ' + hid(h3) + ', not 5 (2 + 2 steps)');
        for (const l of ['look', 'listen', 'wait', 'i', 'search']) if (hid(T.after(h3, l)) !== 5) f.push('"' + l + '" ended it');
        const cr = T.after(T.after(h3, 'creep'), 'step forward');
        if (hid(cr) !== 3) f.push('crept on in the dark: its hiding ' + hid(cr) + ', not 3');
        const wk = T.after(T.after(cr, 'walk'), 'step forward');
        if (hid(wk) !== 0) f.push('a walk: still hidden');
        const h4 = T.after(wk, 'hide'), sp = T.after(h4, 'stop hiding');
        if (hid(h4) !== 3) f.push('in the dark alone: its hiding ' + hid(h4) + ', not 3');
        if (!/You keep still in the dark, out of sight/.test(s)) f.push('in the dark: not told');
        if (hid(sp) !== 0) f.push('stop hiding: still hidden');
        const h5 = T.after(sp, 'hide'), dr = T.after(h5, 'drop the dagger');
        if (hid(h5) === 0) f.push('hide 5: not hidden');
        if (hid(dr) !== 0) f.push('a thing handled: still hidden');
        if (!/You come out of hiding/.test(s)) f.push('out of hiding: not told');
        const h6 = T.after(dr, 'hide');
        if (hid(h6) === 0) f.push('hide 6: not hidden');
        const h7 = T.after(T.after(h6, 'stand'), 'hide'), g3 = T.after(T.after(h7, 'status'), 'status');
        if (!hid(h7) || hid(g3) !== hid(h7)) f.push('kept with the game: hidden ' + hid(h7) + ', then ' + hid(g3));
        const h8 = T.after(T.of('drop the cask'), 'hide');
        if (hid(h8) !== 3) f.push('behind a cask, lit: its hiding ' + hid(h8) + ', not 3');
        if (!/You crouch behind the cask, out of sight/.test(s)) f.push('behind the cask: not told');
        const c1 = T.after(T.after(h8, 'creep'), 'step forward'), c2 = T.after(c1, 'step forward');
        if (hid(c1) !== 3) f.push('crept to beside the cask: its hiding ' + hid(c1) + ', not 3');
        if (hid(c2) !== 0) f.push('crept out into the light: still hidden');
        if (!/There's no cover here/.test(s)) f.push('crept into the open: not told');
        if (!/You see no(ne of those| [a-z ]+) here/.test(s)) f.push('a thing not there named: not told');
        const c4 = T.after(c2, 'hide behind the cask');
        if (hid(c4) !== 3) f.push('behind the cask, walked over to: its hiding ' + hid(c4) + ', not 3');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hidelook', near: ['senses', 'denizens'], what: 'Dingus Quest Zero\'s hiding oneself, the denizens\' looks (A12: HOUSE.md\'s; the book\'s Stealth, its EML by the distance and the cover, any failure detection, a marginal failure the observer\'s Awareness), by the denizens\' slot\'s replies and the hub: a lair of hostile bandits (dq0kit\'s theme) about, the character hidden in a doorway-less dark behind a cask, one bandit put in its line three squares off: its look tests the character\'s Stealth (the denizens\' slot, actor 0, P_STEALTH) at the cover\'s steps, near harder; good at it (Stealth 95): not found, the bandit doesn\'t come at it or engage it; bad at it (Stealth 5): found, told ("has seen you"), the character no longer hidden; a blow from hiding at a bandit unaware (RS_ATTACK\'s w2 RS_ASLEEP, the book\'s Ignore), out of hiding, told; a watched character can\'t hide (the bandit\'s watching it); a denizen hidden isn\'t among those seen, and it leaps out with a blow from hiding as the character stands beside it (its w2 RS_ASLEEP; told)',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/d.hm', 'echo ml stl 95 >> /ram/c.hm', 'echo ml stl 5 >> /ram/d.hm', 'mkdir /ram/g', 'mkdir /ram/h', 'mkdir /ram/i', 'mkdir /ram/j',
        'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'cp /ram/g/theme.txt /ram/h/theme.txt', 'cp /ram/g/theme.txt /ram/i/theme.txt', 'cp /ram/g/theme.txt /ram/j/theme.txt',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'cask'", 'drop the cask', 'drop the torch', 'hide', 'wait 1 minute', 'wait 1 minute', 'status', 'quit',
        "dq0 -p t -s 1 -c /ram/d.hm -T -d /ram/h -k 'cask'", 'drop the cask', 'drop the torch', 'hide', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'status', 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/i -k 'cask,sword'", 'drop the cask', 'drop the torch', 'hold the sword', 'hide', 'attack', 'status', 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/j", 'look', 'wait 1 minute', 'look', 'status', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        // (A bandit put where the scene wants it, a square or three off the way the character faces, unaware; its doing poked)
        const put = (d, dist, extra = {}) => (h, L, K, at, named) => {
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; }, u16 = o => h[o] | h[o + 1] << 8, DX = [0, 1, 1, 1, 0, -1, -1, -1], DY = [-1, -1, 0, 1, 1, 1, 0, -1];
          const fc = h[L.pcFace] & 7;
          w16(at(k) + L.aX, u16(L.pcX) + DX[fc] * dist);
          w16(at(k) + L.aY, u16(L.pcY) + DY[fc] * dist);
          h[at(k) + L.aDoing] = extra.doing || 0;
          h[L.hlook + k] = 0;                  // (A fresh look)
          if (extra.attitude !== undefined) h[at(k) + L.aAttitude] = extra.attitude;
          if (extra.state) h[at(k) + L.aState] = extra.state;
        };
        dq0Poke(m, [{ g: 1, line: 'wait 1 minute', n: 1, call: put(0, 2) }, ...[1, 2, 3, 4, 5, 6].map(n => ({ g: 2, line: 'wait', n, call: put(0, n & 1 ? 1 : 2) })), ...[0, 1, 2, 3, 4, 5].map(k => ({ g: 3, line: 'drop the cask', n: 1, k, set: { attitude: 2 } })), { g: 3, line: 'attack', n: 1, call: put(0, 1, { attitude: 0 }) }, { g: 4, line: 'wait 1 minute', n: 1, call: put(0, 1, { state: 3 }) }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), STEALTH = 4, RS_TEST = m.dq0.layout().K.RS_TEST, RS_ATTACK = m.dq0.layout().K.RS_ATTACK;
        const hid = t => t && t.after ? t.after.pc.hid : -1, gs = T.filter(t => /^dq0 -p t/.test(t.line));
        const from = n => gs[n] ? gs[n].k : 0, tests = (a, b) => T.filter(t => t.k >= a && t.k < b).flatMap(t => dq0Rep(t, 'dslot', { kind: RS_TEST, a: STEALTH, actor: 0 }));
        const t0 = tests(from(0), from(1)), t1 = tests(from(1), from(2));
        if (!t0.length) f.push('Stealth 95: its look tested no Stealth');
        if (t0.length > 3) f.push('Stealth 95: ' + t0.length + ' tests, not one a nearness');
        if (hid(T.of('status', from(0))) === 0) f.push('Stealth 95: found');
        if (!t1.length) f.push('Stealth 5: its look tested no Stealth');
        if (hid(T.of('status', from(1))) !== 0) f.push('Stealth 5: not found');
        if (!/has seen you/.test(s)) f.push('found: not told');
        const at = T.of('attack', from(2));
        if (hid(at) !== 0) f.push('a blow from hiding: still hidden');
        if (!dq0Rep(at, 'slot0', { kind: RS_ATTACK, w2: v => v & 0x80 }).length) f.push('a blow from hiding: not the book\'s Ignore (RS_ASLEEP)');
        if (!/You strike from hiding/.test(s)) f.push('a blow from hiding: not told');
        const lk = T.of('look', from(3)), l2 = T.after(lk, 'look');
        const wt = T.after(lk, 'wait 1 minute');
        if (!dq0Rep(wt, 'slot0', { kind: RS_ATTACK, w2: v => v & 0x80 }).length) f.push('a blow from hiding at the character: not the book\'s Ignore (RS_ASLEEP)');
        if (!/leaps out of hiding/.test(s)) f.push('one hidden beside it: no blow from hiding told');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stay', meaning: true, what: 'Dingus Quest Zero\'s scents strengthened by passing and by staying (the user\'s, 2026-10-22: "repeated passage or occupancy should enhance prior smells"; HOUSE.md\'s light and traces, DUNGEON.md\'s a stay counts as passings), by the world\'s reply (the UI bank\'s tread), its words and the traces\' journal: at seed 3 the giant rats\' lair by the way down where the character starts, its keepers\' scent journalled where they live as it\'s placed (each rat a passing at its home square, with the most stays: the denizens\' ring), sniffed at once and heavy ("A musky smell hangs heavy here: giant rats, by the smell of it.": its strength 6, TR_HEAVY); the rats standing about as the character waits 50 minutes a step off, their stays journalled where each stood (I_TROD\'s TS_STAY, a TQ each); and the character\'s own stay there, its torch lit: once it\'s stepped back (and looked for tracks: the journal taken in as its torch burns, the store\'s smoke a torch\'s) and put its torch out, its smoke still smelt three hours on (faint), where a passing\'s is gone by then (dq0traces)',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml awa 80 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'sniff', 'step forward', 'wait 50 minutes', 'step back', 'track', 'douse the torch', 'wait 3 hours', 'sniff', 'sniff', 'sniff', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], rd = t => (t.after && t.after.tread) || {}, RAT = 1;
        const s0 = T.of('sniff'), r0 = rd(s0), j0 = s0.after && s0.after.journal, pc0 = s0.after && s0.after.pc;
        if (!j0 || !pc0) return ['the game not played'];
        const lair = j0.e.filter(e => e.kind === RAT && !e.stay && e.stays === 255);
        if (lair.length < 2 || lair.some(e => e.x !== pc0.x || e.y !== pc0.y))
          f.push('the rats\' lair\'s scent not journalled where they live, at the way down (' + pc0.x + ',' + pc0.y + '): ' + JSON.stringify(j0.e.filter(e => e.stays)));
        const musk = (r0.trails || []).find(e => e.kind === RAT);
        if (r0.grade !== -2 && (!musk || musk.strength < 6 || !/^A musky smell hangs heavy here/.test(r0.text || '') || (r0.grade > 0 && !/giant rats, by the smell of it/.test(r0.text || ''))))
          f.push('the rats\' lair not smelt heavy at once: ' + JSON.stringify(r0));
        const w = T.of('wait 50 minutes'), jw = w.after && w.after.journal, t0 = w.at ? w.at.now : 0;
        const stays = jw ? jw.e.filter(e => e.kind === RAT && e.stay && e.stays >= 1 && e.t >= t0) : [];
        if (stays.length < 6) f.push('the rats standing about, their stays not journalled: ' + JSON.stringify(jw ? jw.e.slice(-12) : null));
        const b = T.of('step back'), jb = b.after && b.after.journal, last = jb && jb.p[jb.p.length - 1], before = jb && jb.p[jb.p.length - 2];
        if (!last || !before || !last.lit || last.x !== pc0.x - 1 || last.t - before.t < 11 * 2400)
          f.push('its stay a step off with its torch lit not in its ring: ' + JSON.stringify(jb ? jb.p.slice(-2) : null));
        const ends = T.filter(t => t.line === 'sniff' && t.k > b.k).map(rd), told = ends.filter(r => r.grade !== -2);
        if (!told.length) f.push('no sniff three hours on but a critical failure: ' + JSON.stringify(ends.map(r => r.grade)));
        for (const r of told) if (!r.smoke || !/A faint smell of smoke/.test(r.text || '')) f.push('its torch\'s smoke where it stayed not smelt three hours on: ' + JSON.stringify(r));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0prints', outfit: true, near: ['senses'], what: "Dingus Quest Zero's prints by footwear (the user's, 2026-11-03, \"Build it\": HOUSE.md's the traces): the same walk at seed 3's flagged cell read back (the world's tread): barefoot its faintest and told bare feet, in sandals clearer, in hobnailed boots the clearest and told hobnailed boots; and a floor in a young stratum (the user's: a place of no age has little or no dust; seed 5's): a skin of dust",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml trk 95 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take off the sandals', 'e', 'w', 'track', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take off the sandals', 'wear the sandals', 'e', 'w', 'track', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'hobnail boots\'', 'take off the sandals', 'wear the hobnail boots', 'e', 'w', 'track', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'examine the floor', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], rd = t => (t.after && t.after.tread) || {}, tracks = T.filter(t => t.line === 'track'), own = t => (rd(t).trails || []).find(e => e.kind === 0);
        if (tracks.length !== 3 || tracks.some(t => !own(t))) return ['its own prints not read back: ' + JSON.stringify(tracks.map(t => rd(t)))];
        const [b, s, h] = tracks.map(own);
        this.notes = [JSON.stringify([b, s, h])];
        if (!/bare feet/.test(b.what)) f.push('barefoot: ' + b.what);
        if (!/sandalled feet/.test(s.what)) f.push('in sandals: ' + s.what);
        if (!/hobnailed boots/.test(h.what)) f.push('in hobnails: ' + h.what);
        if (!(b.strength < s.strength && s.strength < h.strength)) f.push('the prints not clearer by footwear: ' + [b.strength, s.strength, h.strength]);
        const fl = rd(T.of('examine the floor'));
        if (fl.dust > 2 || !fl.dust || !/under a skin of dust/.test(fl.text || '')) f.push('no skin of dust in a place of no age: ' + JSON.stringify(fl));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0sleepsenses', outfit: true, asks: true, near: ['senses', 'time'], what: "Dingus Quest Zero's sleeping senses (the user's, 2026-11-03: \"half awareness asleep\": HOUSE.md's sleep and rest): a hostile bandit put in the character's sight as it sleeps: asleep it sees nothing, so nothing stops or asks; and as it wakes what's there is told; at rest (awake), the same bandit in sight stops it or asks, a rest lying down all the same; the senses' look blind as it sleeps (the bandit not among what it sees), its eyes open after",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g', 'look', 'close the door', 'sleep for an hour', 'look', 'rest for an hour', 'look', 'quit', 'echo $status']) },
      start(m) {
        // (Each bandit friendly and away from the sleeper; as the rest's begun one comes into sight)
        const away = line => [0, 1, 2].map(k => ({ g: 1, line, n: 1, k, set: { attitude: 4, doing: 0 } }));
        dq0Poke(m, [...away('look'), ...away('close the door'), ...away('sleep for an hour'),
          { g: 1, line: 'look', n: 2, k: 0, call: (h, L) => { h[L.actor + L.aSn] = h[L.pcSn] + 1; h[L.denSeq] = h[L.denSeq] + 1 & 255; } }]);   // (Out of its sight before the rest, so it's new)
        // (As it's told it's settled down: a friendly bandit put beside it, a denizen come into sight)
        const near = () => {
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), o = L.actor, R = dq0Rec(m), r = R.at(h[o + L.aName + 12]), w = (q, v) => { h[q] = v & 255; h[q + 1] = v >> 8 & 255; };
          const x = (h[L.pcX] | h[L.pcX + 1] << 8) + 1, y = h[L.pcY] | h[L.pcY + 1] << 8;
          w(o + L.aX, x); w(o + L.aY, y); h[o + L.aSn] = h[L.pcSn]; R.mem[r + R.X] = x & 255; R.mem[r + R.X + 1] = x >> 8; R.mem[r + R.Y] = y & 255; R.mem[r + R.Y + 1] = y >> 8; R.mem[r + R.SN] = h[L.pcSn];
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        dq0OnWords(m, [{ g: 1, line: 'sleep for an hour', n: 1, words: 'You settle down to sleep', call: near }, { g: 1, line: 'rest for an hour', n: 1, words: 'You settle down to rest', call: near }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), a = s.indexOf('> sleep for an hour'), b = s.indexOf('> rest for an hour');
        const sleep = s.slice(a, b), rest = s.slice(b);
        this.notes = [JSON.stringify(sleep.slice(0, 300)), JSON.stringify(rest.slice(0, 300))];
        if (/Keep sleeping\?/.test(sleep)) f.push('asleep, a denizen seen: asked');
        if (!/A bandit[^.]*\./.test(sleep)) f.push('woken, what\'s beside it not told: ' + JSON.stringify(sleep.slice(0, 500)));
        if (!/Keep (resting|waiting)\?/.test(rest)) f.push('at rest, awake, a denizen seen not asked: ' + JSON.stringify(rest.slice(0, 500)));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stirtyped', near: ['senses', 'denizens'], what: "Dingus Quest Zero's stir as a line's being typed at the turns pace (the life soak's seed 5, 2026-11-04: \"> look at the rat\" answered \"Something stirs.  A giant rat is to your right, 5 feet away, keeping its distance.\" then the look, saying it again: the stir's words after the echo, the told list begun again as the line was taken): its doing changed as the line's first keys come: held for the answer, the look saying it once; a line that doesn't say it (inventory) gets the stir after its answer",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Vermin | rat | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g', 'look', 'wait', 'look', 'inventory', 'quit', 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '', looks = 0, invs = 0;
        const stirred = doing => {
          const h = m.dq0.bank(0), { L, K } = m.dq0.layout(), at = k => L.actor + k * L.actorSize;
          h[at(0) + L.aDoing] = K[doing];
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        dq0Poke(m, [{ g: 1, line: 'wait', n: 1, call: (h, L, K, at) => dq0Alone(m, h, L, K, at, 0) }, { g: 1, line: 'wait', n: 1, k: 0, beside: 2, set: { attitude: 3, doing: 0 } }]);
        const read2 = a.read;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) typed = ''; else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
            if (typed === 'l' && m.dq0.games.length && ++looks === 2) stirred('DO_WATCH');
            if (typed === 'i' && m.dq0.games.length && !invs++) stirred('DO_SHY');
          }
          return read2(r, t);
        };
        void read;
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], A = dq0Answers(m.out), look = A.filter(x => x.cmd === 'look').pop(), inv = A.find(x => x.cmd === 'inventory');
        this.notes = [look ? look.text.slice(0, 400) : 'no look', inv ? inv.text.slice(0, 400) : 'no inventory'];
        void out;
        if (!look || !/giant rat/.test(look.text)) f.push('the look doesn\'t say the rat: the scene not set');
        else if ((look.text.match(/A giant rat is /g) || []).length !== 1) f.push('the rat told more than once in the look\'s answer: ' + look.text);
        if (look && /Something stirs/.test(look.text.slice(0, look.text.indexOf('You are ') < 0 ? 0 : look.text.indexOf('You are ')))) f.push('the stir told before the look: ' + look.text);
        if (!inv || !/You carry[\s\S]*Something stirs/.test(inv.text)) f.push('the stir not told after the inventory: ' + (inv ? inv.text : ''));
        return f;
      },
      sdcard: true
    },
    dq0LookFarTest(),
    dq0AskHeldTest(),
    dq0RestBusyTest(),
    (() => {
      const LINES = ['walk to the door ahead to my left', 'open the door', 'close the door', 'listen', 'wait 1 minute', 'take the sack from the backpack', 'put the rope in the sack', 'take the rope from the sack',
        'stow the rope', 'drop the sack', 'take the sack', 'take the pumpkin', 'dig down', 'stop', 'look at the floor', 'douse the torch', 'dig down', 'stop', 'quit'];
      return {
        name: 'dq0firstword', outfit: true, near: ['parser', 'time'], what: "Dingus Quest Zero's slow commands saying a word as they begin (the user's, 2026-11-04: \"Word as they begin.\"; HOUSE.md's the first word), by the words: each told once, in its order, before what comes of it, true whatever comes of it: a door tried and found open, closed; a listen; a thing reached for, taken from a sack, put in it, dropped, taken up; a dig's floor; a wait told at once and not again; no reach for a thing it doesn't see lying about; in the dark, felt, not looked at",
        init: 't_rc', cycles: 1500e6,
        machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', ...LINES, 'echo $status']) },
        expect: ['% echo $status'],
        check(m, out) {
          const f = [], o = dq0Story(out), said = (l, want, from = 0) => {
            const i = o.indexOf('> ' + l + ' ', from), e = o.indexOf('> ' + LINES[LINES.indexOf(l) + 1] + ' ', i + 1), s = o.slice(i, e < 0 ? undefined : e);
            inTurn(s, want, f, l + ': ');
            if (s.split(want[0]).length !== 2) f.push(l + ': "' + want[0] + '" told ' + (s.split(want[0]).length - 1) + ' times: ' + s);
            return s;
          };
          said('walk to the door ahead to my left', ['You head for the door.']);
          said('open the door', ['You try the door.', 'It stands open already.']);
          said('close the door', ['You reach for the door.', 'You close the door.']);
          said('listen', ['You listen.', 'You hear']);
          said('wait 1 minute', ['You wait a minute.']);
          said('take the sack from the backpack', ['You reach for your backpack.', 'You take the sack in your right hand.']);
          said('put the rope in the sack', ['You reach for your backpack.', 'You put the rope in the sack.']);
          said('take the rope from the sack', ['You reach into the sack.']);
          if (/Put down\./.test(said('drop the sack', ['You put down the sack.']))) f.push('drop the sack: Put down. after its first words');
          said('take the sack', ['You reach for the sack.', 'Taken.'], o.indexOf('> drop the sack'));
          const none = said('take the pumpkin', ['You look for the pumpkin.', 'You see no pumpkin here.']);
          if (/You reach/.test(none)) f.push('a thing it doesn\'t see reached for: ' + none);
          said('dig down', ['You look at the floor.', 'flagstones']);
          said('look at the floor', ['You look at the floor.', 'The floor here']);
          inTurn(o.slice(o.indexOf('> douse the torch')), ['> dig down', 'You feel the floor.'], f, 'in the dark: ');
          return f;
        },
        sdcard: true
      };
    })(),
    {
      scene: false,   // (Its watchers keep the looks' replies from the game's first: none taken up, sim/lib/scenes.js)
      name: 'dq0looks', what: 'Dingus Quest Zero\'s looks the same as they were (the look quickened, reborn-game-look: its grid kept and moved, its marks put only where they reach, the store gone through once): eight games at the turns pace (seeds 3, 5, 7, 12, 4, 517: turns, steps, walks, a walk\'s chosen door, a stairway down, a torch put out and lit again, one put down and taken up, a portcullis lowered and raised, a window broken (punched bare-handed, as the old code\'s run struck it: the torch in hand would strike it now, the user\'s, 2026-10-22) and climbed through, a weak-eyed character; each dungeon dry, dq0 -W: the looks before the water, slice 4.9\'s), every reply the senses give (a look\'s, the denizens\' senses\') with the same request, the same record of what the character perceives and the same squares seen and visible in every live chunk as tests/dq0/looks.ref, the old code\'s (DQ0LOOKS=write: written afresh from this run)',
      init: 't_rc', cycles: 3000e6,
      machine: { input: 'ābind -a /sd/0/games /bin\rāhmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\rādq0furn=0\rācp /ram/c.hm /ram/w.hm\rāecho att eye 4 >> /ram/w.hm\r' +
        [['dq0 -p t -s 3 -c /ram/c.hm -T -W -k sword',['look','turn left','turn right','walk to the door ahead to my left','2','look','turn around','turn left','turn right','map','d','look','sw','look']],['dq0 -p t -s 5 -c /ram/c.hm -T -W',['d','wait','go right','turn left','look','turn around','wait','look']],['dq0 -p t -s 7 -c /ram/w.hm -W',['open the door behind me','s','move south 4 squares','face north','look','douse the torch','z','z','z','look','light the torch','z','look','turn around','n']],['dq0 -p t -s 3 -c /ram/c.hm -T -W',['drop the torch','turn around','step forward','look','turn around','step forward','get torch','look']],['dq0 -p t -s 12 -c /ram/c.hm -T -W',['e','wait','e','wait','e','turn around','lower portcullis','w','look','raise portcullis','w','turn left']],['dq0 -p t -s 12 -c /ram/c.hm -W',['n','turn around','punch the window','climb through window','turn around','look','turn right']],['dq0 -p t -s 4 -c /ram/c.hm -W',['open door','3','look','go through the door','look','turn left']],['dq0 -p t -s 517 -c /ram/c.hm -W',['look','turn left','turn right','turn around','look']]].map(([g, c]) => 'ā' + g + '\r' + c.concat('quit').map(l => 'ā' + l + '\r').join('')).join('') + 'āecho $status\r' },
      expect: ['% echo $status'],
      // (The senses' task found by its code, dq0p.hyx's at its main; its hub_reply's calls snapped, as the game it's in
      // the quits told so far)
      start(m) {
        const dir = path.join(TESTS, '..', 'obj', 'games'), hyx = fs.readFileSync(path.join(dir, 'dq0p.hyx'));
        const lbl = fs.readFileSync(path.join(dir, 'dq0p', 'dq0p.lbl'), 'latin1'), at = n => parseInt(new RegExp('^al ([0-9A-F]+) \\.' + n + '$', 'm').exec(lbl)[1], 16);
        const main = at('_main'), reply = at('_hub_reply'), lines = this.lines = [], cpu = m.cpu, step = cpu.step.bind(cpu);
        let lastT = -1, isP = false, game = 0, qpos = 0, out = m.out;
        cpu.step = v => {
          const T = m.T;
          if (T !== lastT) { lastT = T; const r = m.taskRam[T]; isP = true; for (let i = 0; i < 24; i++) if (r[main + i] !== hyx[main - 0x800 + i]) { isP = false; break; } }
          if (isP && cpu.PC === reply) { const l = lookSnap(m, cpu.A | cpu.X << 8, game); if (l) lines.push(l); }
          step(v);
        };
        Object.defineProperty(m, 'out', { get: () => out, set: v => { let i; while ((i = v.indexOf('> quit', qpos)) >= 0) { game++; qpos = i + 6; } out = v; } });
      },
      check(m, out) {
        const now = this.lines || [];
        if (process.env.DQ0LOOKS === 'write') { fs.writeFileSync(LOOKS_REF, now.join('\n') + '\n'); return []; }
        const ref = fs.readFileSync(LOOKS_REF, 'utf8').split(/\r?\n/).filter(l => l), f = [];
        if (now.length !== ref.length) f.push(now.length + ' replies, ' + ref.length + ' in tests/dq0/looks.ref');
        for (let i = 0; i < Math.min(now.length, ref.length) && f.length < 4; i++) {
          if (now[i] !== ref[i]) f.push('reply ' + i + ': ' + now[i].slice(0, 120) + ' (looks.ref: ' + ref[i].slice(0, 120) + ')');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0heard', what: 'Dingus Quest Zero\'s denizens\' events perceived by some and not others (DENIZENS.md: events), by the denizens\' events, what the character perceives and the YM2151\'s key-ons: down the ladder at seed 5, the torch put out: in the dark the giant rats move about unseen (their steps told, none on a square the character sees; none seen, nothing told as stirring) but heard, near enough for the character\'s hearing (the referee\'s), by the way they lie (SOUND.md: the UI\'s "You hear a scurrying to your right."), and played as they\'re heard (the sound task\'s: the YM2151\'s channels 4-6, on the side they lie: a rat to the right on the right speaker); the turns pace, the denizens kept up a slice at a time; its cave, seed 5\'s small stratum, irregular, every square reached',
      init: 't_rc', cycles: 700e6,
      machine: { ymLog: true, input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -T', 'd',
        'douse the torch', 'wait', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], x = T.of('douse the torch'), w = T.of('wait'), ons = YM_KEYPANS(m);
        if (!x.after || !x.after.sight || !x.after.sight.dark) f.push('not dark, the torch put out');
        const dark = [x, w], steps = dark.flatMap(t => dq0Ev(t, 'E_DEN', { de: 'DE_MOVED' }));
        if (!steps.length) f.push('the rats not moving about in the dark');
        if (steps.some(e => e.seen)) f.push('a rat\'s step on a square the character sees, in the dark');
        for (const t of dark) {
          if (t.after && t.after.sight && t.after.sight.dens.length) f.push(t.line + ': a rat seen in the dark');
          dq0Never(f, t, 'E_VIEW', { why: 'V_STIR' }, t.line + ': a stirring told in the dark');
        }
        const o = dq0Story(out), seg = o.slice(o.indexOf('> douse the torch'), o.indexOf('> quit'));
        if (!/You hear [^.]* to your right\./.test(seg)) f.push('the rats not heard to the right');
        if (o.includes('You hear something')) f.push('a sound told without its words (the sound task\'s table not in the hub)');
        if (!ons.some(k => k === '4R' || k === '5R' || k === '6R')) f.push('no rat played on the right: ' + ons.join(' '));
        if (ons.some(k => !/^[456]/.test(k))) f.push('a key-on off the sound task\'s channels: ' + ons.join(' '));
        this.notes = ['the YM2151\'s key-ons: ' + ons.join(' ')];
        f.push(...caveReport(m, 1, 1));
        return f;
      },
      sdcard: true
    },
    {
      // (Seed 7: the kitchen the character begins in lit by its own torches; its south door's dart trap set off as
      // it's opened, then down the passage into a dark hall and to its far side, 14 squares from the kitchen's door,
      // facing back up the passage: a weak-eyed character's look reaches 4)
      name: 'dq0glow', what: 'Dingus Quest Zero\'s lights seen far off and eyes adapting to the dark (DUNGEON.md: perception as layers; the user\'s, 2026-10-16: "if I am standing in the dark, I should be able to see other light sources from much further away"; eyes take minutes to the dark), by what the character perceives (the senses\' record: the lights far off, the eyes dazzled): at seed 7, a weak-eyed character (Eyesight 4: its look reaches 4 squares) in a dark hall 14 squares from the lit kitchen\'s open door (lit by its sconce\'s torch, the character\'s first: nobody keeps it lit, slice 4.1\'s), up a straight passage: holding its torch, no light far off (its eyes used to the torch\'s light); its torch put out: at once nothing far off (its eyes still used to the torch), after 2 minutes still nothing, after 3 minutes and 20 seconds a faint glow far off ahead, after 5 minutes a light (16 b / (4 + a): 19 squares, its eyes used to the dark; the eyes the senses\', a 16th of a light level each 1.9 s darker); its torch lit again, dazzled a moment, and the light lost (dq0dark\'s, in it)',
      init: 't_rc', cycles: 700e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/w.hm', 'echo att eye 4 >> /ram/w.hm',
        'dq0 -p t -s 7 -c /ram/w.hm', 'light the torch in the sconce', 'face north', 'open the door behind me', 's', 'move south 4 squares', 'face north', 'look', 'douse the torch', 'where',
        ...Array(12).fill('z'), 'where', ...Array(8).fill('z'), 'where', ...Array(10).fill('z'), 'where', 'light the torch', 'z', 'look', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], looks = T.filter(t => t.line === 'look' || t.line === 'where'), x = T.of('douse the torch'), lt = T.of('light the torch');
        if (looks.length !== 6) return ['the looks not typed'];
        const glows = t => t.after && t.after.sight ? t.after.sight.glows : [{ how: '?' }], ahead = (t, how) => glows(t).some(g => g.rel === 0 && g.how === how);
        const at = ['holding the torch', 'at once in the dark', 'after 2 minutes'];
        looks.slice(0, 3).forEach((t, i) => { if (glows(t).length) f.push('a light far off seen ' + at[i] + ': ' + JSON.stringify(glows(t))); });
        if (!x.after || !x.after.sight || !x.after.sight.dark) f.push('not dark, the torch put out');
        if (!ahead(looks[3], 'GL_GLOW')) f.push('no faint glow ahead after 3 minutes 20: ' + JSON.stringify(glows(looks[3])));
        if (!ahead(looks[4], 'GL_LIGHT')) f.push('no light far off ahead after 5 minutes: ' + JSON.stringify(glows(looks[4])));
        if (!lt.after || !lt.after.sight || !lt.after.sight.dazzled) f.push('not dazzled, the torch lit again');
        if (glows(lt).length || glows(looks[5]).length) f.push('the light far off still seen, the torch lit');
        if (!looks[5].after || looks[5].after.sight.dazzled) f.push('still dazzled a round after');
        return f;
      },
      sdcard: true
    },
    {
      // (The sound task's scene, dq0s -t: the character faces north, its hearing 5; then the YM2151's key-ons, each
      // with its speakers: the rat's three hits on the right, the squeak on the left, the growl on both, the
      // yapping on the right, the bear's tread in its place on the left; nothing on 0-3 or 7, the music's and bell's)
      name: 'dq0sound', what: 'Dingus Quest Zero\'s sound (SOUND.md, the user: "if I am facing N and the sound is coming from the E, then I hear it in my right ear"; dq0s -t, its scene): a giant rat\'s step two squares east of a character facing north played on the right speaker (the YM2151\'s channel 4, its RL bits); then five sources at once, more than the three channels: the three loudest played (the squeak to the left on the left, the growl ahead on both, the yapping behind a shut door muffled, on the right), the two quietest not; and a bear\'s tread beside it, louder, in the quietest\'s place; the music\'s and the bell\'s channels left alone',
      meaning: true,
      init: 't_rc', cycles: 200e6,
      machine: { ymLog: true, input: 'ā/sd/0/games/lib/dq0/dq0s -t /sd/0/games/lib/dq0\rāecho $status\r' },
      expect: ['1 a scurrying: channel 4, right, level 48\n3 a squeak: channel 4, left, level 76\n' +
        '2 a low growl: channel 5, both, level 64\n6 a yapping: channel 6, right, muffled, level 52\n' +
        '4 a shuffling tread: not played, level 39\n5 a dry clatter: not played, level 36\n' +
        '7 a heavy tread: channel 6, left, in place of 6, level 97\n%', '% echo $status\n\n%'],
      check(m) {
        const ons = YM_KEYPANS(m), f = [], want = ['4R', '4L', '5B', '6R', '6L'];
        let i = 0;
        for (const o of ons) if (o === want[i]) i++;
        if (ons[0] !== '4R') f.push('the rat to the east not first, on the right: ' + ons.join(' '));
        if (i < want.length) f.push('the key-ons not ' + want.join(', ') + ' in turn: ' + ons.join(' '));
        if (ons.some(o => !/^[456]/.test(o))) f.push('a key-on off the sound task\'s channels: ' + ons.join(' '));
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        this.notes = ['the YM2151\'s key-ons: ' + ons.join(' ')];
        return f;
      },
      sdcard: true
    },
    {
      // (With a Vera X: the noises on the PSG's voices first (20-23: voices 12-15), so all five played and the
      // bear's tread on a voice free; the voices' speakers as they were last: the rat east's 12, then the
      // tread's behind (both); the clatter's (east: right) on 13, the bear's tread (west: left) on 14)
      name: 'dq0soundx', what: 'Dingus Quest Zero\'s sound with a Vera X (dq0s -t): the PSG\'s voices 12-15 the sound task\'s too, seven channels: a rat\'s scurrying to the east on voice 12 on the right, the five sources at once all played (the noises on the PSG, the tones on the YM2151), the bear\'s tread on a voice free, on the left',
      meaning: true,
      init: 't_rc', cycles: 200e6,
      machine: { ymLog: true, vera: true, input: 'ā/sd/0/games/lib/dq0/dq0s -t /sd/0/games/lib/dq0\rāecho $status\r' },
      expect: ['1 a scurrying: channel 20, right, level 48\n3 a squeak: channel 4, left, level 76\n' +
        '2 a low growl: channel 5, both, level 64\n6 a yapping: channel 6, right, muffled, level 52\n' +
        '4 a shuffling tread: channel 20, both, level 39\n5 a dry clatter: channel 21, right, level 36\n' +
        '7 a heavy tread: channel 22, left, level 97\n%', '% echo $status\n\n%'],
      check(m) {
        const f = [], lr = v => '-LRB'[m.vera.psg[4 * v + 2] >> 6], ons = v => m.vera.psgOns.filter(k => k.startsWith('voice ' + v + ' ')).length;
        for (const [v, n, s] of [[12, 4, 'B'], [13, 3, 'R'], [14, 2, 'L']]) {
          if (ons(v) < n) f.push('PSG voice ' + v + ' on ' + ons(v) + ' times, not ' + n);
          if (lr(v) !== s) f.push('PSG voice ' + v + '\'s speakers ' + lr(v) + ', not ' + s);
        }
        if (ons(15)) f.push('PSG voice 15 on: nothing played there');
        this.notes = ['the PSG\'s voices on: ' + m.vera.psgOns.map(k => k.split(' ')[1]).join(' ')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0darklook', asks: true, initiative: true, reach: true, bearings: true, outfit: true, near: ['senses', 'gear'], what: 'Dingus Quest Zero\'s look in the dark (the user\'s, 2026-11-03, DECISIONS.md\'s: "if I say `look around` it should really fell around, looking for things in the environment (on the floor, the walls)"; asked, the same as feel around, in the light as they are, blind as dark, things on walls as well as the floor, and what it finds taken next): at seed 3 in a kitchen, its torch put out: a look and a look around each what its hands tell at once, then the search nearby, its minute, finding the pot and the well on the floor and the sconce on the wall; "look for the pot" by touch; taken next, the sconce\'s torch to a free hand, a second, no pack off (the user\'s, any thing from a fixture), the pack only with both hands full.  Blind in the light the same; in the light a look takes no time; in shock or busy, not here',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'look', 'look around', 'put out the torch', 'look', 'look around', 'look for the pot', 'take the torch from the sconce', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'look', 'blinded', 'turn around', 'look', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a -k sword', 'take the torch from the sconce', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'take the torch', 'inventory', 'quit',
        'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '', on = 0;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            if (a.rx === 13) { if (typed === 'blinded') on = 1; if (/^quit/.test(typed)) on = 0; typed = ''; } else if (a.rx >= 32) typed += String.fromCharCode(a.rx);
            const h = on && m.dq0 && m.dq0.bank(0);
            if (h) { const { L } = m.dq0.layout(); h[L.pcsum + L.sCond + 1] |= require('../../../games/dq0/hubgen.js').load().C.C_BLINDED >> 8; }   // (rs.h's C_BLINDED, its high byte: held on, as dq0blind's)
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out);
        const games = []; for (let i = s.indexOf('% dq0 '); i >= 0; i = s.indexOf('% dq0 ', i + 1)) games.push(i);
        const seg = (g, line, n = 1) => { const e = '> ' + line + ' > ' + line; let i = games[g - 1]; for (let k = 0; k < n && i >= 0; k++) i = s.indexOf(e, i + 1);
          if (i < 0) { f.push('game ' + g + ': "' + line + '" not answered'); return ''; } const j = s.indexOf(' > ', i + e.length); return s.slice(i + e.length, j < 0 ? s.length : j); };
        const has = (g, line, w, n) => { if (!(w.test ? w.test(seg(g, line, n)) : seg(g, line, n).includes(w))) f.push('game ' + g + ', ' + line + ': not told ' + w + ': ' + seg(g, line, n).slice(0, 400)); };
        const never = (g, line, w, n) => { if (seg(g, line, n).includes(w)) f.push('game ' + g + ', ' + line + ': told ' + w); };
        const gk = {}; T.forEach((t, k) => { if (/^dq0 /.test(t.line)) gk[Object.keys(gk).length + 1] = k; });
        const dt = (g, line, tenths, n = 1) => { let k = gk[g], t; for (let i = 0; i < n; i++) { t = T.of(line, k); k = t.k + 1; } const d = t.at && t.after ? t.after.now - t.at.now : null; if (d !== tenths) f.push('game ' + g + ', ' + line + ' (' + n + '): ' + d + ' tenths, not ' + tenths); };
        // (In the light, as they were)
        for (const l of ['look', 'look around']) { never(1, l, 'You feel about'); dt(1, l, 0); }
        // (In the dark, feel around: the hands' words, then the search; floor and wall)
        for (const [l, n] of [['look', 2], ['look around', 2]]) {
          inTurn(seg(1, l, n), ['You feel a wall', 'You feel about you in the dark...', 'Your hands find', 'iron pot', 'well', 'sconce'], f, 'dark ' + l + ': ');
          dt(1, l, 600, n);
        }
        has(1, 'look for the pot', 'You feel about in the dark for the pot');
        // (Taken next: a free hand, the sconce's)
        has(1, 'take the torch from the sconce', 'It\'s in your hand.');
        never(1, 'take the torch from the sconce', 'backpack');
        has(1, 'inventory', 'in your right hand');
        // (Blind, in a lit room)
        inTurn(seg(2, 'look', 2), ['You can\'t see.', 'You feel about you in the dark...', 'Your hands find'], f, 'blind: ');
        // (Both hands full: the pack)
        has(3, 'take the torch from the sconce', 'You take off the backpack');
        has(3, 'take the torch from the sconce', 'in your backpack');
        // (A plain take of the sconce's: the hand too)
        never(4, 'take the torch', 'backpack');
        return f;
      },
      sdcard: true
    },
    {
      // (Seed 7, dq0glow's dungeon: the kitchen the character begins in, its torch put out (nobody keeps the kitchen lit:
      // its sconce's torch cold), the kitchen's south door seen by the torch as the game began, then down the straight
      // passage into the dark hall (its doorway square 32785,32851; its floor 32781-32786 by 32851-32855; a table at
      // 32784,32853; its east door, closed, at 32787,32855); seed 517's kobold gatehouse, its ledge 10 feet up, dark
      // with the torch out (dq0harm's); seed 3's passage east from the first room, its bend (dq0tasks' walk east))
      // (dq0sky=0, 2026-11-03, a scene's: seed 7's kitchen has a skylight and a window, its daylight lights it with the torch out: the dark's what it's about)
      name: 'dq0dark', meaning: true, what: 'Dingus Quest Zero\'s walking in the dark (the user\'s playtest, 2026-10-22: "you should not auto-adjust your movement direction toward an opening you cannot see ... half-walk speed; anything faster can be explicitly stated, but stumbles over large objects in your path, or running headlong into a pit, a wall, a door, or off a cliff are real dangers"; HOUSE.md\'s walking and searching in the dark), by the world\'s events and where the character ends: at seed 7 the kitchen\'s south door opened (its dart trap set off) and its torch put out, south through the door it saw (a walk to a door it knows of: "You feel your way south.", WB_FEEL at the cautious pace) into the dark hall; east, not turned toward the hall\'s east door it hasn\'t seen, but straight across to the wall (WB_FELT, FA_WALL: "Your hands meet a wall."); two steps west and south, its foot finding the table it didn\'t know of a square short of it (WB_THING, the table known after: "Your foot finds something on the floor: a table."); south again past it, known (gone round, as in the light), to the far wall; east along the far side to the closed east door (FA_DOOR: "You feel a door before you."); the status line "by feel", no "You cross the room." in the dark; then another game the same way to the table, and "run south" (said outright in the dark: WB_FEEL at a run, the status line "blind", the status command\'s pace a run, not held to a slow walk): the stumble roll over the table (the referee\'s P_STUMBLE), kept its feet and on into the far wall (WB_HIT, a blow, HC_STRIKE), or fallen short of it (WB_TRIP); at seed 517 the ledge up felt, not walked into (FA_RISE), climbed, and run back off it: stopped short at the edge (WB_SHORT) or over it (WB_OFF, a fall of its 10 feet, the character at its foot); at seed 3 the passage east walked in the dark straight on to its bend, unknown, and stopped there (FA_WALL, no WB_TURN), where the lit walk (dq0tasks\') goes round it',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 7 -c /ram/c.hm', 'face north', 'open the door behind me', 'douse the torch', 's', 'e', 'step west', 'step west', 's', 's', 'e',
        'quit', 'dq0 -p t -s 7 -c /ram/c.hm', 'face north', 'open the door behind me', 'douse the torch', 's', 'step west', 'run south', 'status', 'quit',
        'dq0 -p t -s 517 -c /ram/c.hm', 'ne', 'douse the torch', 'e', 'u', 'step east', 'step east', 'turn around', 'run west', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm', 'douse the torch', 'e', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), K = m.dq0.layout().K;
        const beats = (t, b) => dq0Ev(t, 'E_BEAT', { beat: b });
        const at = (t, x, y, what) => { if (!t.after || t.after.pc.x !== x || t.after.pc.y !== y) f.push(what + ': at ' + (t.after ? t.after.pc.x + ',' + t.after.pc.y : '?') + ', not ' + x + ',' + y); };
        const g1 = T.of('dq0 -p t -s 7 -c /ram/c.hm'), s1 = T.after(g1, 's'), e1 = T.after(s1, 'e'), w2 = T.after(T.after(e1, 'step west'), 'step west');
        const s2 = T.after(w2, 's'), s3 = T.after(s2, 's'), e2 = T.after(s3, 'e');
        if (!beats(s1, 'WB_FEEL').some(e => e.head === 4 && !e.gait)) f.push('the walk south not set out on feeling its way (' + dq0Evs(s1) + ')');
        dq0Need(f, s1, 'E_BEAT', { beat: 'WB_DOOR' }, 'the walk south not through the door it knew of');
        at(s1, 32785, 32851, 'the walk south into the dark hall');
        if (!beats(e1, 'WB_FELT').some(e => e.fa === 'FA_WALL')) f.push('the walk east not stopped by the wall, felt (' + dq0Evs(e1) + ')');
        at(e1, 32786, 32851, 'the walk east: straight across, not to the east door it hadn\'t seen');
        if (!beats(s2, 'WB_THING').some(e => e.thing === K.KI_TABLE)) f.push('the table in the way not found by its foot (' + dq0Evs(s2) + ')');
        at(s2, 32784, 32852, 'the walk south stopped short of the table');
        const table = s2.after && s2.after.items.find(i => i.name === 'table' && i.x === 32784 && i.y === 32853);
        if (!table || !(table.flags & 0x20)) f.push('the table not known after its foot found it: ' + JSON.stringify(table));
        if (!beats(s3, 'WB_FELT').some(e => e.fa === 'FA_WALL') || beats(s3, 'WB_THING').length) f.push('the walk south past the known table not to the far wall (' + dq0Evs(s3) + ')');
        at(s3, 32784, 32855, 'the walk south past the table');
        if (!beats(e2, 'WB_FELT').some(e => e.fa === 'FA_DOOR')) f.push('the closed east door not felt (' + dq0Evs(e2) + ')');
        at(e2, 32786, 32855, 'the walk east to the closed door');
        for (const t of [e1, s2, s3, e2]) if (dq0Ev(t, 'E_ARRIVE', { how: 'A_CROSSED' }).length) f.push('"' + t.line + '" told as a room crossed, in the dark');
        const g = s.slice(s.indexOf('-s 7 -c /ram/c.hm'), s.indexOf('> quit'));
        inTurn(g, ['You feel your way south.', 'Your hands meet a wall.', 'Your foot finds something on the floor: a table.', 'Your hands meet a wall.',
          'You feel a door before you.'], f, 'the dark walk\'s words: ');
        if (/You cross the room\./.test(g.slice(g.indexOf('> e')))) f.push('"You cross the room." said in the dark');
        if (!/by feel \[/.test(out)) f.push('the status line not "by feel" as it felt its way');
        const g2 = T.after(e2, 'dq0 -p t -s 7 -c /ram/c.hm'), r = T.after(g2, 'run south'), st = T.after(r, 'status');
        if (!beats(r, 'WB_FEEL').some(e => e.head === 4 && e.gait === K.GA_RUN)) f.push('"run south" in the dark not at a run (' + dq0Evs(r) + ')');
        if (!r.replies.some(q => /^slot/.test(q.req) && q.kind === K.RS_TEST && q.a === K.P_STUMBLE)) f.push('no stumble roll over the table (P_STUMBLE)');
        const kept = beats(r, 'WB_STUMBLE').length, fell = beats(r, 'WB_TRIP').length;
        if (kept + fell !== 1) f.push('not stumbled over the table once, kept or fallen (' + dq0Evs(r) + ')');
        if (kept && (!beats(r, 'WB_HIT').some(e => e.fa === 'FA_WALL' && e.gait === K.GA_RUN) || !dq0Ev(r, 'E_HARM', { hc: 'HC_STRIKE' }).length)) f.push('kept its feet, but not run into the far wall, a blow (' + dq0Evs(r) + ')');
        if (kept) at(r, 32784, 32855, 'the run on to the far wall');
        if (fell) at(r, 32784, 32852, 'fallen short of the table');
        if (fell && (!r.after || r.after.pc.pos !== 1)) f.push('fallen over the table, not lying prone (slice 4.1b)');
        if (!/blind \[/.test(out)) f.push('the status line not "blind" as it ran');
        const sw = s.slice(s.indexOf('> status', s.indexOf('> run south')));
        if (!/Pace: a run/.test(sw) || /held to a slow walk in the dark/.test(sw.slice(0, 200))) f.push('the status not a run, said outright in the dark: ' + sw.slice(0, 160));
        const g3 = T.of('dq0 -p t -s 517 -c /ram/c.hm'), e3 = T.after(g3, 'e'), rw = T.after(g3, 'run west');
        if (!beats(e3, 'WB_FELT').some(e => e.fa === 'FA_RISE')) f.push('the ledge up not felt in the dark (' + dq0Evs(e3) + ')');
        if (e3.at && e3.after && (e3.at.pc.x !== e3.after.pc.x || e3.at.pc.y !== e3.after.pc.y)) f.push('moved, the ledge up met in the dark');
        dq0Never(f, e3, 'E_LEDGE', {}, 'the ledge\'s height told, unseen');
        const short = beats(rw, 'WB_SHORT').some(e => e.fa === 'FA_DROP'), over = beats(rw, 'WB_OFF').some(e => e.fa === 'FA_DROP');
        if (short === over) f.push('the run west at the ledge\'s edge not stopped short of it, nor over it (' + dq0Evs(rw) + ')');
        if (!rw.replies.some(q => /^slot/.test(q.req) && q.kind === K.RS_TEST && q.a === K.P_DODGE_HAZARD)) f.push('no dodge at the edge (P_DODGE_HAZARD)');
        if (over && !dq0Ev(rw, 'E_HARM', { hc: 'HC_FALL' }).length) f.push('over the edge, no fall');
        if (rw.at && rw.after && over && rw.after.pc.x >= rw.at.pc.x - 2) f.push('over the edge, not at its foot');
        const g4 = T.of('dq0 -p t -s 3 -c /ram/c.hm'), e4 = T.after(g4, 'e');
        if (beats(e4, 'WB_TURN').length) f.push('the walk east in the dark turned round the unknown bend (' + dq0Evs(e4) + ')');
        if (!beats(e4, 'WB_FELT').some(e => e.fa === 'FA_WALL')) f.push('the walk east in the dark not stopped at the bend, its wall felt (' + dq0Evs(e4) + ')');
        return f;
      },
      sdcard: true
    },
    {
      // (Seed 7's kitchen, as dq0dark's, its torch put out: two iron pots beside the character, at 32785,32839 and 32784,32839,
      // the south door's dart trap; the hall's table at 32784,32853, walked over to in the light)
      // (dq0sky=0, 2026-11-03, a scene's: as dq0dark's)
      name: 'dq0darksearch', meaning: true, what: 'Dingus Quest Zero\'s searching in the dark and the search\'s forms (the user\'s playtest, 2026-10-22: "search around in the dark with your hands ... search nearby or search the east wall, or search around for a/an item ... You fumble around in the dark and find nothing ... search around on the floor/table"; HOUSE.md\'s walking and searching in the dark), by the world\'s events, the things\' replies and the words: at seed 7 in the dark kitchen a search by touch, nearby (its reach 3, E_PORTAL\'s b 1: "You feel about you in the dark...", the iron pots within reach felt and known: "Your hands find an iron pot"); a search for the sack not within reach ("Your hands find no sack here.", a minute); the floor nearby; the north wall out of reach (no search: "Your hands find no wall within reach that way."); a search for the iron pot, stopped as it comes to it (the things\' IO_SEEK: 3 seconds and 3.2 more for its distance squared, 6.4 seconds, "Your hands find an iron pot"); the pot searched, closed, gone over for traps by examine\'s check ("It\'s closed", "shows no sign of a trap"); then in the light the hall\'s table searched (walked over to first: IO_SEEK\'s IR_NOT_HERE, "There\'s nothing on the table.", its trap examine\'s check), "search the table for traps" examine\'s alone (IO_LOOK, LM_TRAPS), the east wall searched (a wall\'s, its reach), nearby (reach 3); the pouch hidden where it stands, the torch put out, a step away, and a search by touch finding it (known, hidden still), and a search for it by touch found as it comes to it',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 7 -c /ram/c.hm', 'douse the torch', 'search', 'search for the sack', 'search the floor', 'search the north wall',
        'search around for the iron pot', 'search the pot', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm', 'face north', 'open the door behind me', 's', 'step west', 'search the table', 'search the table for traps',
        'search the east wall', 'search nearby', 'hide the pouch', 'step east', 'douse the torch', 'search', 'search for the pouch', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), K = m.dq0.layout().K;
        const portal = t => dq0Ev(t, 'E_PORTAL', {}).filter(e => ['PO_NOTHING', 'PO_FOUND', 'PO_FOUND_TRAP'].includes(e.po));
        const took = t => t.at && t.after ? t.after.now - t.at.now : -1;
        const g1 = T.of('dq0 -p t -s 7 -c /ram/c.hm'), se = T.after(g1, 'search'), sk = T.after(se, 'search for the sack'), fl = T.after(sk, 'search the floor');
        const nw = T.after(fl, 'search the north wall'), fp = T.after(nw, 'search around for the iron pot'), sp = T.after(fp, 'search the pot');
        for (const [t, what] of [[se, 'search'], [fl, 'the floor']]) {
          const p = portal(t);
          if (p.length !== 1 || p[0].a !== 3 || p[0].b !== 1) f.push(what + ' in the dark not by touch, within reach (E_PORTAL a 3, b 1): ' + JSON.stringify(p.map(e => [e.po, e.a, e.b])));
        }
        const pots = se.after ? se.after.items.filter(i => i.name === 'iron pot' && i.y === 32839 && (i.x === 32785 || i.x === 32784)) : [];
        if (pots.length !== 2 || !pots.every(i => i.flags & 0x20)) f.push('the iron pots within reach not felt and known: ' + JSON.stringify(pots));
        if (took(sk) !== 600) f.push('the search for the sack not a whole minute: ' + took(sk));
        if (!dq0Rep(sk, 'ireq', { kind: 'IO_SEEK' }).length) f.push('the sack not sought (IO_SEEK)');
        if (portal(nw).length || !dq0Ev(nw, 'E_BEAT', { beat: 'WB_FELT', fa: 'FA_NONE' }).length) f.push('the north wall out of reach searched (' + dq0Evs(nw) + ')');
        if (took(fp) !== 62) f.push('the search for the iron pot not stopped as it came to it (6.2 seconds): ' + took(fp));
        if (took(sp) !== 600) f.push('the pot\'s search not a minute: ' + took(sp));
        const g = s.slice(s.indexOf('> search'), s.indexOf('> quit'));
        inTurn(g, ['You feel about you in the dark...', 'Your hands find an iron pot', 'You feel about in the dark for the sack...', 'Your hands find no sack here.',
          'You feel over the floor about you in the dark...', 'Your hands find no wall within reach that way.', 'Your hands find an iron pot',
          'It\'s closed: you go over the outside of it.'], f, 'the dark search\'s words: ');
        if (g.includes('shows no sign of a trap')) f.push('a thing searched, no traps asked for, told of traps');
        const g2 = T.after(sp, 'dq0 -p t -s 7 -c /ram/c.hm'), tb = T.after(g2, 'search the table'), tt = T.after(tb, 'search the table for traps');
        const ew = T.after(tt, 'search the east wall'), nb = T.after(ew, 'search nearby'), sd = T.after(nb, 'search'), sf = T.after(sd, 'search for the pouch');
        if (!dq0Rep(tb, 'ireq', { kind: 'IO_SEEK', result: 'IR_NOT_HERE' }).length || !dq0Ev(tb, 'E_ARRIVE', { how: 'A_WENT_TO' }).length) f.push('the table out of reach not walked over to first (' + dq0Evs(tb) + ')');
        if (!dq0Rep(tt, 'ireq', { kind: 'IO_LOOK' }).length || portal(tt).length) f.push('"search the table for traps" not examine\'s check alone');
        const pe = portal(ew), pn = portal(nb);
        if (pe.length !== 1 || pe[0].b) f.push('the east wall not searched, in the light: ' + JSON.stringify(pe));
        if (pn.length !== 1 || pn[0].a !== 3 || pn[0].b) f.push('nearby not searched within reach (a 3), in the light: ' + JSON.stringify(pn));
        const pouch = st => st && st.items.find(i => i.name === 'pouch' && i.where === 'IW_SQUARE');
        const p1 = pouch(sd.after);
        if (!p1 || !(p1.flags & 0x08)) f.push('the pouch not hidden where it was: ' + JSON.stringify(p1));
        if (portal(sd).length !== 1 || portal(sd)[0].b !== 1) f.push('the search after the torch went out not by touch');
        if (took(sf) !== 62) f.push('the search for the pouch by touch not stopped as it came to it (6.2 seconds): ' + took(sf));
        const p2 = pouch(sf.after);
        if (!p2 || !(p2.flags & 0x08) || !(p2.flags & 0x20)) f.push('the pouch it hid, felt, not hidden still: ' + JSON.stringify(p2));
        if (!/Your hands find [^.]*\bpouch\b/.test(s.slice(s.indexOf('> step east')))) f.push('the search by touch not finding the pouch it hid, in words');
        const g3 = s.slice(s.indexOf('> search the table'));
        inTurn(g3, ['You search the table...', 'There\'s nothing on the table.', 'The table shows no sign of a trap', 'You search the east wall...',
          'You search about you, within reach...', 'You feel about you in the dark...', 'You feel about in the dark for the pouch...',
          'Your hands find a pouch.'], f, 'the searches\' words: ');
        return f;
      },
      sdcard: true
    },
    {
      // (dq0sky=0, 2026-11-03, a scene's: as dq0dark's)
      name: 'dq0roomsearch', meaning: true, near: ['senses'], what: 'Dingus Quest Zero\'s searching a thing, and searching a whole room (the user\'s playtest, 2026-11-02: "When searching a pile of straw, it says "no sign of a trap", but I am not looking for traps, but for anything hidden in the straw.  Also, I would like `search the room` to take time to search around the whole room, poking through piles of straw, taking a closer look at the items, etc., not just searching for hidden doors or traps.  It should report incrementally what you are doing as well."; HOUSE.md\'s the search\'s forms and a room searched whole), by the world\'s events, the clock and the words.  At seed 7 (Awareness 95, a heap of straw and an iron key to hand, the key hidden under the straw where it lies and made unknown, dq0Poke\'s): the straw searched, a minute, "You poke through the heap of straw...", the key found in it and no words of traps; searched again, "You find nothing in the straw."; the table, the dagger the same, no traps; then the hall searched whole: its stages in order, its words a line each (along the wall to its left, round the corners by the compass), each thing in turn (its look over, the straw poked through), the clock on as much as the stops\' and things\' times and a slow walk, the character on another square at each corner, a long task LT_SEARCH (its bar\'s done counting the clock, never the squares); stopped at the first corner ("You stop searching."), a second search goes on from there ("You go on searching the room."), the things before the stop not gone through again, to its end ("You\'ve been over the whole room."); a third says it\'s done, in no time; kept with the game (dq0 -d: quit, carried on, still done).  In the dark (the first room, its torch doused) by touch, a thing\'s time twice, the block\'s 30 seconds.  In the light, the same first room, its door\'s hidden dart trap, its hiding 0 (the chunk poked), tested as a stop comes to it.  With a ghoul (seed 7, dq0 -T) put beside it, engaging it, stopped hard.  In a passage, stopped at the door behind it, a search about it.  In a great cavern (dq0 -V), refused kindly, no time.  And in the hall again with hidden plates poked onto each stop\'s middle but its first, its doors made secret (dq0Poke\'s traps): each plate noticed underfoot at a search\'s chance and gone round, or sprung and the search stopped there; a secret door found as a stop comes to it, told as found',
      init: 't_rc', cycles: 4500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml awa 95 >> /ram/c.hm', 'mkdir /ram/g',
        "dq0 -p t -s 7 -c /ram/c.hm -d /ram/g -k 'heap of straw,iron key'", 'face north', 'open the door behind me', 's', 'drop the straw', 'hide the key', 'search the straw', 'search the straw',
        'search the table', 'search the dagger', 'search the room', 'search the room', 'search the room', 'search the whole room', 'search this room', 'search the entire room', 'search the chamber', 'search the hall', 'quit', 'dq0 -d /ram/g', 'search the room', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm', 'douse the torch', 'search the room', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm', 'search the room', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -T', 'search the room', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -V', 'd', 'search the room', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm', 'open the door behind me', 's', 'search the room', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm', 'open the door behind me', 's', 'search the room', 'quit',
        'mkdir /ram/h', 'dq0 -p t -s 7 -c /ram/c.hm -d /ram/h', 'search the room', 'dq0 -d /ram/h', 'status', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 1, line: 'search the straw', n: 1, item: { 'iron key': { clear: 0x20, hide: 0 } } }, { g: 4, line: 'search the room', n: 1, traps: { hide: 0 } },
          { g: 5, line: 'search the room', n: 1, name: 'ghoul', beside: 'face', engage: true }, { g: 8, line: 'search the room', n: 1, traps: { hide: 0, plates: true } }]);
        dq0OnWords(m, [{ g: 1, line: 'search the room', n: 1, words: 'You turn the corner', text: 'stop\r' }, { g: 7, line: 's', n: 1, words: 'You go through a doorway', text: 'stop\r' },
          { g: 9, line: 'search the room', n: 1, words: 'You work along', text: 'quit\r' }]);   // (Typed as it's told)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), K = m.dq0.layout().K;
        const took = t => t.at && t.after ? t.after.now - t.at.now : -1;
        const said = (a, b) => { const i = s.indexOf(a), j = b ? s.indexOf(b, i + a.length) : -1; return i < 0 ? '' : s.slice(i, j < 0 ? undefined : j); };
        const g1 = T.of("dq0 -p t -s 7 -c /ram/c.hm -d /ram/g -k 'heap of straw,iron key'");
        const st1 = T.after(g1, 'search the straw'), st2 = T.after(st1, 'search the straw'), tb = T.after(st2, 'search the table'), dg = T.after(tb, 'search the dagger');
        const r1 = T.after(dg, 'search the room'), r2 = T.after(r1, 'search the room'), r3 = T.after(r2, 'search the room');
        const key = st => st && st.items.find(i => i.name === 'iron key' && i.where === 'IW_SQUARE');
        if (!key(st1.at) || !(key(st1.at).flags & 0x08)) f.push('the key not hidden under the straw: ' + JSON.stringify(key(st1.at)));
        if (took(st1) !== 600 || took(st2) !== 600) f.push('a heap\'s search not a minute: ' + took(st1) + ', ' + took(st2));
        const k1 = key(st1.after);
        const bookish = !process.env.RULES;      // (HarnMaster's Awareness 95; another referee's chance its own: the things' test made, its find not looked for)
        if (bookish && (!k1 || (k1.flags & 0x08) || !(k1.flags & 0x20))) f.push('the key in the straw not found and known: ' + JSON.stringify(k1));
        const things = said('> search the straw', '> search the room');
        if (/trap/.test(things)) f.push('a thing searched told of traps: ' + things.slice(0, 300));
        inTurn(things, ['You poke through the heap of straw...', bookish ? 'You find an iron key in the straw.' : ' the straw.', '> search the straw', 'You poke through the heap of straw...', bookish ? 'You find nothing in the straw.' : ' the straw.',
          '> search the table', 'You search the table...', 'There\'s nothing on the table.', '> search the dagger', 'You look the dagger over...', 'You find nothing hidden about the dagger.'], f, 'a thing searched: ');
        // (The hall searched whole, stopped at its first corner, gone on with, done)
        const ww = t => dq0Ev(t, 'E_BEAT', { beat: 'WB_SEARCH' });
        const bar = T.after(r1, 'stop');
        if (bar.k < 0 || bar.at.task && bar.at.task.kind !== 'LT_SEARCH') f.push('the search not a long task as it\'s stopped: ' + JSON.stringify(bar.at && bar.at.task));
        inTurn(s.slice(s.indexOf('> search the room')), ['You set about searching the whole room.', 'You work along the wall to your left.', 'You turn the corner, and work along the',
          'You stop searching.', '> search the room', 'You go on searching the room.', 'You work along the wall to your left.', 'You poke through the straw.', 'You look over the', 'You\'ve been over the whole room.',
          '> search the room', 'You\'ve been all over this room already', '> quit'], f, 'the room\'s stages: ');
        if (took(r1) + took(bar) < 100 || took(r2) < 300) f.push('the room\'s search no time: ' + took(r1) + ', ' + took(r2));
        if (took(r2) > 3000) f.push('the room\'s search too long: ' + took(r2));
        if (took(r3) !== 0) f.push('a room done, its search not told at once: ' + took(r3));
        if (r2.at && r2.after && r2.at.pc.x === r2.after.pc.x && r2.at.pc.y === r2.after.pc.y) f.push('the search of the room walked nowhere');
        const rs = T.of('search the room', r3.k + 1);
        inTurn(s.slice(s.indexOf('> search the room', s.indexOf('dq0 -d /ram/g'))), ['You\'ve been all over this room already'], f, 'kept with the game: ');
        if (took(rs) !== 0) f.push('kept with the game, its search not told at once: ' + took(rs));
        // (In the dark, by touch)
        const g2 = T.after(rs, 'dq0 -p t -s 7 -c /ram/c.hm'), d2 = T.after(g2, 'search the room');
        const dw = s.slice(s.indexOf('> douse the torch'), s.indexOf('> quit', s.indexOf('> douse the torch')));
        inTurn(dw, ['You set about feeling your way round the whole room.', 'You feel along the wall to your left.', 'You feel over the', 'You\'ve been over the whole room.'], f, 'by touch: ');
        if (/You work along|You look over|You poke through/.test(dw)) f.push('words of sight in the dark');
        // (In the light: a door's trap found as it comes to it)
        const g3 = T.after(d2, 'dq0 -p t -s 7 -c /ram/c.hm'), l3 = T.after(g3, 'search the room');
        const found = dq0Ev(l3, 'E_TRAP', { to: 'TO_FOUND' });
        if (!l3.replies.some(e => e.req === 'slot0' && e.a === K.P_SEARCH && e.steps === 0)) f.push('the door\'s hidden trap not tested: ' + JSON.stringify(l3.replies.slice(0, 3)));
        if (took(d2) < took(l3) + 300) f.push('by touch, not the longer: ' + took(d2) + ' against ' + took(l3));
        if (found.length && !/You find a[^.]*trap[^]*You've been over the whole room/.test(s.slice(s.indexOf('> search the room', s.indexOf('> quit', s.indexOf('> douse the torch')))))) f.push('the trap\'s find not told before the search\'s end');
        // (Hard, a ghoul)
        const g4 = T.after(l3, 'dq0 -p t -s 7 -c /ram/c.hm -T'), h4 = T.after(g4, 'search the room');
        if (!/You stop searching\./.test(s.slice(s.indexOf('-T -a'), s.indexOf('-T -V'))) && !dq0Ev(h4, 'E_TASK', {}).some(e => e.task === 'LT_SEARCH' && e.te !== 'TE_STOPPED')) f.push('the ghoul not stopping the search: ' + dq0Evs(h4));
        // (A great cavern, refused; a passage, about it)
        const g5 = T.after(h4, 'dq0 -p t -s 3 -c /ram/c.hm -T -V'), c5 = T.after(g5, 'search the room');
        if (took(c5) !== 0 || !s.includes('This cavern is too vast to go over from end to end')) f.push('a great cavern not refused: ' + took(c5));
        const g6 = T.after(c5, 'dq0 -p t -s 7 -c /ram/c.hm'), p6 = T.after(g6, 'search the room');
        if (took(p6) !== 600 || ww(p6).length) f.push('a passage\'s not a search about it: ' + took(p6) + ', ' + dq0Evs(p6));
        // (Traps underfoot: hidden plates about it, noticed as it comes to one and gone round, or set off, and it stops there)
        const g7 = T.after(p6, 'dq0 -p t -s 7 -c /ram/c.hm'), u7 = T.after(g7, 'search the room'), tu = dq0Ev(u7, 'E_TRAP', {}), su = dq0Ev(u7, 'E_TRAP', { to: 'TO_SPRUNG' });
        if (!tu.some(e => e.to === 'TO_NOTICED' || e.to === 'TO_SPRUNG')) f.push('no plate met underfoot: ' + dq0Evs(u7));
        if (bookish && !dq0Ev(u7, 'E_PORTAL', { po: 'PO_FOUND' }).length) f.push('no hidden door found as a stop came to it: ' + dq0Evs(u7));
        const uw2 = s.slice(s.lastIndexOf('> search the room', s.lastIndexOf('mkdir /ram/h')));
        if (su.length ? !uw2.includes('You stop searching.') : !uw2.includes('You\'ve been over the whole room.')) f.push('after the plates: ' + uw2.slice(0, 500));
        // (Quit part way, the watch as it was, carried on)
        if (!/> status[^>]*Watching: nothing in particular/.test(s)) f.push('the search\'s watch kept with the game: ' + s.slice(s.lastIndexOf('> status'), s.lastIndexOf('> status') + 200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0light', what: 'Dingus Quest Zero\'s lights burning down (slice 4.1: the world\'s, a game minute at a time, each its fuel left in the item store, struct item\'s fuel; HOUSE.md\'s light and traces): the kit\'s spare torch and flask of oil (D5), a lantern more (dq0 -k); at seed 3 its torch lit, an hour\'s (the house rule\'s), over three waits: none told at 45 minutes, its minutes left the status line\'s ("torch 15m"); "Your torch is burning low." at its last ten (TE_LOW); "Your torch gutters and goes out: you stop waiting." at the hour (TE_OUT, its only light: the wait stopped hard, at the hour), the character in the dark (the look: "It is dark."), the torch kept, spent; the lantern lit, burnt 20 minutes, filled from the flask (the oil moved: the lantern whole again, the flask the less), its six hours the status line\'s; the spent torch put down, the spare lit (the spent one passed over), the spent one taken up and dipped in the oil (renewed, an hour\'s again, the flask the less); at seed 7 a chapel\'s candle stubs found with some life left (the maker\'s, by the stratum\'s age: less than a whole one\'s), lit, put down, and seen to gutter and go out (TE_OUT, one seen lying there: E_LIGHT at its square, the stubs gone, burnt away; the wait not stopped)',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -k lantern',
        'wait 45 minutes', 'wait 10 minutes', 'wait 10 minutes', 'look', 'light the lantern', 'wait 20 minutes', 'fill the lantern', 'drop the torch',
        'light torch', 'get the torch', 'dip the torch in the oil', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -T', 'look', 'wait 2 minutes', 'get the candle stubs', 'light the candle stubs', 'drop the candle stubs', 'wait 10 minutes', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out), K = m.dq0.layout().K, I = DQ0_I;
        const named = (st, n) => dq0Carried(st).filter(i => i.name === n);
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k lantern');
        if (!g.after) return ['the first game not begun'];
        const kit = g.after, torches = named(kit, 'torch'), flask = named(kit, 'flask of oil')[0], lantern = named(kit, 'lantern')[0];
        if (torches.length !== 2 || torches.filter(i => i.flags & I.LIT).length !== 1 || !torches.every(i => i.fuel === 60)) f.push('the kit not a torch lit and a spare, an hour each: ' + JSON.stringify(torches.map(i => [i.flags, i.fuel])));
        if (!flask || flask.fuel !== 360) f.push('the kit\'s flask of oil not a lantern\'s fill (360 minutes): ' + (flask && flask.fuel));
        if (!lantern || lantern.fuel !== 360) f.push('the lantern not full: ' + (lantern && lantern.fuel));
        const w1 = T.of('wait 45 minutes'), w2 = T.after(w1, 'wait 10 minutes'), w3 = T.after(w2, 'wait 10 minutes');
        if (dq0Ev(w1, 'E_TASK', { te: v => v === 'TE_LOW' || v === 'TE_OUT' }).length) f.push('a light told before its last ten minutes (' + dq0Evs(w1) + ')');
        if (!w1.after || w1.after.pc.fuel !== 15) f.push('the torch\'s minutes left not 15 at 45 minutes: ' + (w1.after && w1.after.pc.fuel));
        dq0Need(f, w2, 'E_TASK', { te: 'TE_LOW', lamp: K.KI_TORCH, seen: false }, 'the torch not told burning low');
        dq0Never(f, w2, 'E_TASK', { te: 'TE_OUT' }, 'the torch out before the hour');
        const out1 = dq0Ev(w3, 'E_TASK', { te: 'TE_OUT', lamp: K.KI_TORCH, task: 'LT_WAIT' });
        if (out1.length !== 1) f.push('the torch not gone out at the hour, the wait stopped with it (' + dq0Evs(w3) + ')');
        else if (out1[0].t < 36000 || out1[0].t > 36010) f.push('the torch out at ' + out1[0].t + ' tenths, not the hour');   // (Within a
        // second of it: the turns pace moves the clock a slice at a time, the denizens' turns its slices, 36003 with their
        // dice by who asks, 2026-10-30)
        dq0Need(f, w3, 'E_VIEW', { why: 'V_DARK' }, 'no look in the dark as it went out');
        const spent = w3.after && named(w3.after, 'torch').find(i => !i.fuel);
        if (!w3.after || w3.after.pc.light || !w3.after.sight || !w3.after.sight.dark) f.push('the character not in the dark, its only light gone');
        if (!spent || (spent.flags & I.LIT)) f.push('the torch not kept, spent and out');
        if (w3.after && w3.after.now >= 36600) f.push('the wait not stopped as the torch went out: the clock at ' + w3.after.now);
        inTurn(s, ['> wait 10 minutes', 'Your torch is burning low.', '> wait 10 minutes', 'Your torch gutters and goes out: you stop waiting.', 'It is dark.',
          '> look', 'It is dark.'], f, 'the torch\'s words: ');
        const lit = T.of('light the lantern'), w4 = T.after(lit, 'wait 20 minutes'), fl = T.of('fill the lantern');
        const lan = st => st && named(st, 'lantern')[0], fla = st => st && named(st, 'flask of oil')[0];
        if (!lan(w4.after) || lan(w4.after).fuel !== 340 || !(lan(w4.after).flags & I.LIT)) f.push('the lantern not burnt 20 minutes: ' + JSON.stringify(lan(w4.after)));
        if (!dq0Rep(fl, 'ireq', { kind: 'IO_FILL', result: 'IR_OK' }).length) f.push('the lantern not filled (' + JSON.stringify(fl.replies.filter(r => r.req === 'ireq').map(r => r.kind + ' ' + r.result)) + ')');
        if (!lan(fl.after) || lan(fl.after).fuel !== 360 || !fla(fl.after) || fla(fl.after).fuel !== 340) f.push('the oil not moved to the lantern: ' + JSON.stringify([lan(fl.after), fla(fl.after)].map(i => i && i.fuel)));
        inTurn(s, ['> fill the lantern', 'You fill the lantern from the flask of oil.'], f, 'the fill\'s words: ');
        const st = dq0StatusRows(out);
        if (!/lantern 6h00m/.test(st)) f.push('the status line not the lantern\'s six hours ("lantern 6h00m")');
        if (!/torch 1h00m/.test(st) || !/torch 15m/.test(st)) f.push('the status line not the torch\'s minutes ("torch 1h00m", "torch 15m")');
        const dr = T.of('drop the torch'), lt = T.of('light torch'), gt = T.of('get the torch'), dip = T.of('dip the torch in the oil');
        if (!dq0Here(dr.after).some(i => i.name === 'torch' && !i.fuel)) f.push('the spent torch not put down');
        const lt2 = lt.after && named(lt.after, 'torch')[0];
        if (!lt2 || !(lt2.flags & I.LIT) || lt2.fuel < 59) f.push('the spare torch not lit: ' + JSON.stringify(lt2));   // (Its
        // minute's tick may fall in its reach, got out of the pack: HOUSE.md's reach, 2026-10-26)
        if (!dq0Rep(gt, 'ireq', { kind: 'IO_GET', result: 'IR_OK' }).length) f.push('the spent torch not taken up');
        // (The spent one renewed, a whole hour's; the spare lit burning meanwhile, its minute or two: the reach's, a torch
        // got out of the pack to light it, and the spent one stowed, HOUSE.md's reach, 2026-10-26)
        const both = dip.after ? named(dip.after, 'torch') : [];
        if (!dq0Rep(dip, 'ireq', { kind: 'IO_FILL', result: 'IR_OK' }).length || both.length !== 2 ||
          !both.some(i => !(i.flags & I.LIT) && i.fuel === 60) || !both.some(i => (i.flags & I.LIT) && i.fuel >= 57) ||
          !fla(dip.after) || fla(dip.after).fuel !== 280) f.push('the spent torch not renewed from the oil: ' + JSON.stringify([...both, fla(dip.after)].map(i => i && i.fuel)));
        inTurn(s, ['> dip the torch in the oil', 'You soak the torch in the flask of oil.'], f, 'the dip\'s words: ');
        const g2 = T.of('dq0 -p t -s 7 -c /ram/c.hm -T'), lk = T.after(g2, 'look'), stub = st => st && st.items.find(i => i.name === 'candle stubs' && (i.where !== 'IW_SQUARE' || i.sn === st.pc.sn));   // (A held thing's sn its hand: slice 4.1b's)
        const found = stub(lk.after);
        if (!found || !found.id || found.where !== 'IW_SQUARE' || !(found.fuel > 0 && found.fuel < 25)) f.push('the chapel\'s candle stubs not found with some life left, less than a whole one\'s: ' + JSON.stringify(found));
        const ls = T.of('light the candle stubs'), ds = T.of('drop the candle stubs'), w5 = T.after(ds, 'wait 10 minutes');
        if (!stub(ls.after) || !(stub(ls.after).flags & I.LIT)) f.push('the stubs not lit');
        if (!dq0Here(ds.after).some(i => i.name === 'candle stubs' && (i.flags & I.LIT))) f.push('the stubs not put down lit');
        dq0Need(f, w5, 'E_TASK', { te: 'TE_OUT', lamp: K.KI_STUBS, seen: true }, 'the stubs lying there not seen to go out');
        dq0Need(f, w5, 'E_LIGHT', { w1: ds.after && ds.after.pc.x, w2: ds.after && ds.after.pc.y }, 'no E_LIGHT at their square, the light layer out of date');
        if (stub(w5.after)) f.push('the stubs not burnt away: ' + JSON.stringify(stub(w5.after)));
        if (!w5.after || w5.after.now - ds.after.now < 6000) f.push('the wait stopped by a light it doesn\'t carry');
        inTurn(s, ['> wait 10 minutes', 'The candle stubs gutter and go out.'], f, 'the stubs\' words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0relight', what: 'Dingus Quest Zero\'s look as a light is lit or put out, only if the light it sees by has changed (the user\'s, 2026-11-14: "When you light a new torch from a burning one, it should not re-look since the light state has not changed"; DECISIONS.md\'s "No look when the light\'s the same"): at seed 3 (dq0 -T, the cell dark but for its torch) a second torch held and lit from the burning one, its words and no look; one of the two put out, no look; the other put out, the dark told ("It is dark."); one lit again in the dark, the place seen by it; "light a new torch" and "light the unlit torch with the lit torch" no look; one of two lit burning out ("One of your torches gutters and goes out."), no look; a lantern lit beside the torch, a greater light (its reach, the catalogue\'s: 6 squares to the torch\'s 4), the place seen by it; two torches lit and put down, burning out as it rests, the dark told',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'hold another torch', 'light the torch', 'douse the torch', 'douse the torch', 'light the torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'light a new torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'hold another torch', 'light the unlit torch with the lit torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'wait 20 minutes', 'hold another torch', 'light the torch', 'wait 45 minutes', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k lantern', 'light the lantern', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'hold another torch', 'light the torch', 'drop the torch', 'drop the torch', 'rest for 2 hours', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out), LOOK = 'You can see part of a cell';
        const games = []; for (let i = s.indexOf('% dq0 '); i >= 0; i = s.indexOf('% dq0 ', i + 1)) games.push(i);
        const seg = (g, line, n = 1) => { const e = '> ' + line + ' > ' + line; let i = games[g - 1]; for (let k = 0; k < n && i >= 0; k++) i = s.indexOf(e, i + 1);
          if (i < 0) { f.push('game ' + g + ': "' + line + '" not answered'); return ''; } const j = s.indexOf(' > ', i + e.length); return s.slice(i + e.length, j < 0 ? s.length : j); };
        const has = (g, line, w, n) => { if (!seg(g, line, n).includes(w)) f.push('game ' + g + ', ' + line + ' (' + (n || 1) + '): not told ' + w + ': ' + seg(g, line, n).slice(0, 400)); };
        const never = (g, line, w, n) => { if (seg(g, line, n).includes(w)) f.push('game ' + g + ', ' + line + ' (' + (n || 1) + '): told ' + w); };
        // (A torch lit from a burning one: its words, no look)
        has(1, 'light the torch', 'You light the unlit torch.', 1);
        never(1, 'light the torch', LOOK, 1);
        // (One of two put out: no look; the last: the dark)
        has(1, 'douse the torch', 'You put out the torch.', 1);
        never(1, 'douse the torch', LOOK, 1);
        has(1, 'douse the torch', 'It is dark.', 2);
        never(1, 'douse the torch', LOOK, 2);
        // (Lit in the dark: the place seen by it)
        has(1, 'light the torch', 'You light the torch.', 2);
        has(1, 'light the torch', LOOK + ', by the light of your torch.', 2);
        // (A new torch, one lit with another: no look)
        has(2, 'light a new torch', 'You light the unlit torch.');
        never(2, 'light a new torch', LOOK);
        has(3, 'light the unlit torch with the lit torch', 'You light the unlit torch.');
        never(3, 'light the unlit torch with the lit torch', LOOK);
        // (One of two burnt out: no look)
        never(4, 'light the torch', LOOK);
        has(4, 'wait 45 minutes', 'One of your torches gutters and goes out.');
        never(4, 'wait 45 minutes', LOOK);
        // (A greater light: looked about by it)
        has(5, 'light the lantern', 'You are at the near end of a long, narrow cell, by the light of your lantern.');
        // (Lights lying on the floor burnt out, none carried: the dark told)
        has(6, 'rest for 2 hours', 'It is dark.');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0felt', meaning: true, near: ['senses'], what: 'Dingus Quest Zero feeling its way (the user\'s playtest, 2026-11-02, DECISIONS.md: "It might be better to just say You feel a wall to your left, and nothing to your right"; HOUSE.md\'s feeling your way), by the words: at seed 3 its torch put out in the kitchen it has seen, what its hands find, and the kitchen remembered; walked out of it by the door it knows, along a passage it has never seen: its hands\' words, its feet\'s ("The way turns ..."), nothing of a passage named, no ways told, no slope ahead, no fork; a look and "where am I" the same; a look one way; come back into the kitchen, remembered again; a way not felt never told',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'douse the torch', 'look',
        'go through the door to the east', 'look', 'where am i', 'look north', 'go back to the kitchen', 'look', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], ans = dq0Answers(out), a = l => (ans.filter(y => y.cmd === l)[0] || { text: '' }).text, g = {}, nth = (l, n) => (ans.filter(y => y.cmd === l)[n] || { text: '' }).text;
        const leak = /You are in a passage|It goes |The passage |You go on down|forks here|comes to a door|You can see|rising|falling|by the light/;
        const felt = /You feel [^.]*\./;
        if (!felt.test(a('douse the torch')) || !a('douse the torch').includes('This should be the kitchen.')) f.push('its torch out: not its hands\' words, the kitchen remembered: ' + a('douse the torch').slice(0, 160));
        for (const [l, n] of [['go through the door to the east', 0], ['look', 1], ['where am i', 0]]) {
          const x = nth(l, n);
          if (!felt.test(x) && l !== 'go through the door to the east') f.push(l + ': not its hands\' words: ' + x.slice(0, 160));
          if (leak.test(x)) f.push(l + ': the place told in the dark: ' + x.slice(0, 200));
          if (l !== 'go through the door to the east' && x.includes('This should be')) f.push(l + ': a place it has never been named: ' + x.slice(0, 160));
        }
        if (!a('look north').includes('You peer ahead into the dark.')) f.push('look north not into the dark: ' + a('look north').slice(0, 120));
        const back = nth('look', 2);
        if (!felt.test(back) || !back.includes('This should be the kitchen.')) f.push('back in the kitchen: not remembered: ' + back.slice(0, 160));
        if (!a('go through the door to the east').includes('The wall to your left ends.')) f.push('the walk\'s hands not told as the wall to its left ended (WB_SIDES): ' + a('go through the door to the east').slice(0, 200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0blind', meaning: true, near: ['senses'], what: 'Dingus Quest Zero\'s blind character (HOUSE.md\'s in the dark: the referee\'s summary, C_BLINDED, held on by the test, as nothing in play sets it yet), its lit torch no help: at seed 2 "You can\'t see." and its hands in a look; the walks over told at once and felt, to a door and to a thing remembered; nothing read; the status "by feel"',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0furn=0', 'dq0 -p t -s 2 -c /ram/c.hm -T', 'look', 'blinded', 'go to the door', 'look', 'go to the sack', 'read the sconce', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        const a = m.acia, rd = a.read;
        let on = 0, typed = '';
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            if (a.rx === 13) { if (typed === 'blinded') on = 1; typed = ''; } else if (a.rx >= 32) typed += String.fromCharCode(a.rx);
          }
          const h = on && m.dq0 && m.dq0.bank(0);
          if (h) { const { L } = m.dq0.layout(); h[L.pcsum + L.sCond + 1] |= require('../../../games/dq0/hubgen.js').load().C.C_BLINDED >> 8; }   // (rs.h's C_BLINDED, its high byte)
          return rd.call(a, r, t);
        };
      },
      check(m, out) {
        const f = [], ans = dq0Answers(out), x = (l, n) => (ans.filter(y => y.cmd === l)[n || 0] || { text: '' }).text;
        const has = (l, n, w) => { if (!x(l, n).includes(w)) f.push(l + ': not "' + w + '": ' + x(l, n).slice(0, 200)); };
        if (!x('look').includes('by the light of your torch')) f.push('not seeing to begin with: ' + x('look').slice(0, 120));
        has('go to the door', 0, 'You head for the door.');
        has('go to the door', 0, 'You feel your way');
        has('look', 1, 'You can\'t see. You feel ');
        if (/by the light|It is dark/.test(x('look', 1))) f.push('seen, or told of the dark, blind: ' + x('look', 1).slice(0, 160));
        has('go to the sack', 0, 'You go over to the sack.');
        has('go to the sack', 0, 'You feel your way');
        has('read the sconce', 0, 'You can\'t see to read.');
        if (!/by feel \[/.test(out)) f.push('the status line not "by feel" as it felt its way');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0watch', what: 'Dingus Quest Zero\'s watchfulness and fatigue (the user\'s, 2026-10-16), by the world\'s events and the referee\'s summary (HarnMaster\'s fatigue, the book\'s): a keen-eyed character (Awareness 95, hmchar\'s file) at seed 16 (dq0 -T) searching for traps as it goes, the search where it stops finding the cave\'s hidden plate first (dq0traps\' walk onto it, not searching, sets it off), and the walk going round it; then a character a level tired and nearly another (pc.hm\'s vol and pace lines: the pace a run) running down a passage, tired a level more (its universal penalty one more); one two levels tired resting ten minutes (61 rounds waited), a level gone',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/r.hm', 'cp /ram/c.hm /ram/t.hm', 'echo ml awa 95 >> /ram/c.hm',
        'dq0 -p t -s 16 -c /ram/c.hm -T', 'face east', 'force', 'e', 'force', 'force', 'search for traps as I go', 'e', 'e', 'quit',
        'echo vol 1 0 0 0 0 >> /ram/t.hm', 'echo pace 4 0 5990 >> /ram/t.hm', 'dq0 -p t -s 16 -c /ram/t.hm -T', 'face east', 'force', 'e', 'quit',
        'echo vol 2 0 0 0 0 >> /ram/r.hm', 'dq0 -p t -s 16 -c /ram/r.hm -T', ...Array(61).fill('z'), 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], s = T.of('search for traps as I go'), e1 = T.after(s, 'e'), e2 = T.after(e1, 'e');
        if (!s.after || !(s.after.pc.watch & 1)) f.push('not searching for traps as it goes');
        dq0Need(f, e1, 'E_TRAP', { tk: 'TK_BLADE', tt: 'TT_PLATE', to: 'TO_FOUND' }, 'the plate not found by the search as it went');
        for (const t of [e1, e2]) dq0Never(f, t, 'E_TRAP', { to: 'TO_SPRUNG' }, 'the plate set off, searching');
        dq0Need(f, e2, 'E_ARRIVE', {}, 'the walk not gone on round the plate');
        // (The referee's universal penalty, the status line's "UP n": fatigue's here, no wounds)
        const up = x => x ? +((x.status.match(/UP (\d+)/) || [])[1]) : NaN;
        const t0 = T.of('dq0 -p t -s 16 -c /ram/t.hm -T'), te = T.after(t0, 'e');
        if (!t0.after || t0.after.pc.gait !== 4) f.push('the tired character\'s pace not its file\'s run');
        if (!(up(t0.after) === 1 && up(te.after) === 2)) f.push('running not tiring a level more: UP ' + up(t0.after) + ', then ' + up(te.after));
        const r0 = T.of('dq0 -p t -s 16 -c /ram/r.hm -T'), rz = T.of('quit', r0.k + 1);
        if (!(up(r0.after) === 2 && up(rz.at) === 1)) f.push('resting ten minutes not a level less tired: UP ' + up(r0.after) + ', then ' + up(rz.at));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0grille', near: ['senses'], what: 'Dingus Quest Zero\'s looking through a door\'s little window (the user\'s, 2026-11-14: "I still can\'t look through a little window in a door"; DECISIONS.md\'s looking through a door\'s little window): at seed 1 the door ahead of the start, closed, given a little window with a grille (its record\'s feat, PF_GRILLE: the chunk poked as "look" is typed), examined; looked through by its names ("the little window", "peer through the grille", "the door", "the window" with no window in the room, "the grille"): its eye at it ("look south" beside it a look that way as ever, not through it), what\'s beyond told ("Through the little window in the door you see ..."), not the room\'s other doors; from the far side the same, walked up to first; then given a spy-hole, its cover on the passage\'s side (its bar\'s, the record\'s): looked through, opened (looked through), closed ("The spy-hole\'s cover is closed."); its cover then on the far side: not seen through ("The spy-hole\'s cover is on the far side: you can\'t see through it."); a spy-hole asked for where there\'s none: "You haven\'t seen a spy-hole here."',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 1 -c /ram/c.hm -T',
        'look through the spy-hole', 'close the door', 'look', 'turn around', 'turn around', 'examine the door', 'look through the little window',
        'peer through the grille', 'look through the door', 'look through the window', 'look south', 'close the little window', 'open the door',
        'go south', 'wait', 'go north', 'look through the grille', 'look through the little window', 'turn left', 'examine the door',
        'look through the spy-hole', 'open the spy-hole', 'close the spy-hole', 'turn right', 'look through the spy-hole', 'examine the door',
        'quit', 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read, [FEAT, BAR, STATE, PARTS] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct portal, feat)',
          'offsetof (struct portal, bar)', 'offsetof (struct portal, state)', 'offsetof (struct portal, parts)']), PF_GRILLE = 1, PF_PEEP = 2;
        let typed = '', n = 0;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (['look', 'wait', 'turn left', 'turn right'].includes(typed) && m.dq0 && !(m.grille && m.grille.set.includes(typed))) {   // (The door's record poked, as the line's typed: the next look's)
                const { L } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
                if (!m.grille) {                // (The door ahead, closed, the start's: the cell's south door)
                  const px = u16(L.pcX), py = u16(L.pcY), f = h[L.pcFace];
                  m.grille = { sn: h[L.pcSn], x: px + [0, 1, 1, 1, 0, -1, -1, -1][f], y: py + [-1, -1, 0, 1, 1, 1, 0, -1][f], f, set: [] };
                }
                const g = m.grille, P = dq0PortalAt(m, g.sn, g.x, g.y), k = n++;
                if (P) {
                  if (k === 0) P.b[P.o + FEAT] = PF_GRILLE;   // (A little window, a grille across it)
                  if (k === 1) {                // (In the guardroom: shut behind it, not known, so the walk back stops at it)
                    P.b[P.o + STATE] &= ~1;
                    P.b[P.o + PARTS] &= ~0x80;
                  }
                  if (k >= 2) {                 // (A spy-hole, its cover on the passage's side, then the cell's)
                    P.b[P.o + FEAT] = PF_PEEP;
                    P.b[P.o + BAR] = k === 2 ? g.f >> 1 : ((g.f >> 1) + 2) & 3;
                  }
                  g.set.push(typed);
                }
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out), g = m.grille;
        if (!g || g.set.length !== 4 || g.f & 1) return ['the scene not set: no door ahead at the start, faced square on: ' + JSON.stringify(g)];
        // (Each answer, in order of the lines, its words: what it must say, what it mustn't)
        const A = dq0Answers(out).filter(x => x.cmd), see = 'Through the little window in the door you see ', hole = 'Through the spy-hole in the door you see ';
        const want = [
          ['look through the spy-hole', ["You haven't seen a spy-hole here."]],
          ['examine the door', ['A little window is cut in it, a grille across it.']],
          ['look through the little window', ['You peer ahead.', see + 'a passage, 5 feet of it.']],
          ['peer through the grille', [see + 'a passage, 5 feet of it.']],
          ['look through the door', [see + 'a passage, 5 feet of it.']],
          ['look through the window', [see + 'a passage, 5 feet of it.']],   // (No window in the cell: the door's little one)
          ['look south', ["You can't see past the door ahead: it's closed."]],   // (A look that way, not asked through it: as ever)
          ['close the little window', ['The little window has no shutter: only its grille.']],
          ['look through the grille', ['You turn to face north.', see + 'into the cell.']],   // (The far side: turned to it first)
          ['look through the little window', [see + 'into the cell.']],
          ['examine the door', ['A spy-hole is cut in it, its cover on this side.']],
          ['look through the spy-hole', [hole + 'into the cell.']],
          ['open the spy-hole', [hole + 'into the cell.']],
          ['close the spy-hole', ["The spy-hole's cover is closed."]],
          ['look through the spy-hole', ["The spy-hole's cover is on the far side: you can't see through it."]],
          ['examine the door', ['A spy-hole is cut in it, its cover on the far side.']]
        ];
        let at = 0;
        for (const [cmd, words] of want) {
          const i = A.findIndex((x, k) => k >= at && x.cmd === cmd);
          if (i < 0) { f.push('no answer to "' + cmd + '" (after answer ' + at + ')'); continue; }
          at = i + 1;
          inTurn(A[i].text, words, f, cmd + ': ');
          if (/^(look|peer)/.test(cmd) && cmd !== 'look south' && /can't see past the door/.test(A[i].text)) f.push(cmd + ": the door's little window not looked through: " + A[i].text);
          if (/^(look|peer)/.test(cmd) && /Through the (little window|spy-hole)/.test(A[i].text) && /You can't see past|Down the passage|Through the (open|door)/.test(A[i].text))
            f.push(cmd + ': more than the door looked through told: ' + A[i].text);
        }
        if (process.env.DQ0SHOW) f.push(s.slice(s.indexOf('> look through the spy-hole')));
        return f;
      },
      sdcard: true
    },
  ],
};
