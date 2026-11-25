// ****************************************************************************
// body.js - Dingus Quest Zero's tests: the body: harm, limbs, tending.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const { typed } = require('../../lib/common.js');
const {
  liveChunks, stratumRec, dq0Story, dq0Prose, inTurn, dq0Turns, dq0Ev, dq0Need, dq0Carried, dq0Thing, DQ0_I, dq0Typed, dq0Game, dq0Rec,
  dq0Scene, dq0SceneNow, dq0PcShock, dq0Grow, dq0OnWords, dq0AnswerTest
} = require('../../lib/dq0.js');

module.exports = {
  tests: [
    {
      name: 'dq0well', near: ['body', 'map'], what: 'Dingus Quest Zero\'s well a real shaft (the user\'s, 2026-11-06: "A real shaft", DECISIONS.md\'s climbing down a well; HOUSE.md\'s a well\'s shaft, for review), by the world\'s events, the referee\'s tests, the item store and the chunks below, at seed 3\'s kitchen: its well comes out in the roof of a cave below (its foot a pool two feet down in the cave\'s floor, under water): the dagger put in it (its words, the splash; at its foot on the stratum below), the rope tied to the curb and let down its shaft, climbed down by it (its tests 3 steps the better on its length), the dagger there and taken, the look at its foot telling the well\'s shaft above; climbed out (beside the curb; from deep water a round with no hold ends the climb, so tried again: three times at most); then the well moved by a poke to a square over the rock below (the stratum grown now), each kind a scene (rc\'s dq0well): a cistern with a way on (the nine squares about its foot under water, out of its depth, a passage up out of it, a rise of two feet a square, to the halls); a sump, down and up unaided (the eight about its foot rock), then jumped into: a fall, told as a plunge into the water, not a hard landing; a dagger thrown down the well ("down": into it, the parser\'s meaning) heard to splash; that game played again with its lines typed late, the same at the turns pace (-same)',
      init: 't_rc', cycles: 12000e6,
      get machine() {
        const g2 = "dq0 -p t -s 3 -c /ram/c.hm -S /ram/s.sc", o3 = ['climb out', 'climb out', 'climb out'], c2 = ['go to the well', 'throw the dagger down the well', 'climb down the well', ...o3, 'climb down the well', 'look', ...o3, 'jump into the well', 'look', 'quit'];
        const typed = l => 'ā' + l + '\r';
        return { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml cli 95 >> /ram/c.hm', 'echo sky 0 >> /ram/s.sc',
          'echo well 3 >> /ram/s.sc', "dq0 -p t -s 3 -c /ram/c.hm -k 'rope,dagger' -S /ram/s.sc", 'go to the well', 'put the dagger in the well', 'tie the rope to the well', 'climb down the well', 'look',
          'take the dagger', ...o3, 'climb down the well', 'look', ...o3, 'quit', 'echo well 0 >> /ram/s.sc', g2, ...c2].map(typed).join('') +
          typed(g2) + c2.map((l, i) => 'ā' + (i ? 'Ā' : 'ĀĀĀ') + l + '\r').join('') + typed('echo $status') };
      },
      expect: ['% echo $status'], sdcard: true,
      start(m) {
        const [TILE, ELEV, WET] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, tile)', 'offsetof (struct chunk, elev)', 'offsetof (struct chunk, wet)']);
        const sq = (sn, x, y) => {                    // (A square of the cache's: its kind, its floor, its water bit; null not there)
          const st = stratumRec(m, sn), c = st && liveChunks(m, sn)[((x - st.x0) >> 5) + ',' + ((y - st.y0) >> 5)];
          if (!c || x - st.x0 >= st.w || y - st.y0 >= st.h) return null;
          const cx = (x - st.x0) & 31, cy = (y - st.y0) & 31;
          return { t: c[TILE + cy * 32 + cx] & 15, z: c[ELEV + cy * 32 + cx] << 24 >> 24, wet: (c[WET + cy * 4 + (cx >> 3)] >> (cx & 7)) & 1 };
        };
        const foot = name => (h, L) => {             // (At the foot: its squares about it, and each way's line of them)
          const sn = h[L.pcSn], x = h[L.pcX] | h[L.pcX + 1] << 8, y = h[L.pcY] | h[L.pcY + 1] << 8;
          const r = { name, sn, x, y, wet: h[L.pcWet], swim: h[L.pcSwim], here: sq(sn, x, y), about: [], ways: [] };
          for (const [dx, dy] of [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]]) r.about.push(sq(sn, x + dx, y + dy));
          for (const [dx, dy] of [[0, -1], [1, 0], [0, 1], [-1, 0]]) { const w = []; for (let k = 1; k <= 14; k++) w.push(sq(sn, x + dx * k, y + dy * k)); r.ways.push(w); }
          (m.feet = m.feet || []).push(r);
        };
        const move = g => (h, L) => {                // (The well moved to a square of its chunk over solid rock below, the
          const per = Math.floor(0x2000 / L.itemSize), cat = m.dq0.bank(L.catBank);   //   character beside it)
          const w16 = (b, o, v) => { b[o] = v & 255; b[o + 1] = v >> 8 & 255; }, r16 = (b, o) => b[o] | b[o + 1] << 8;
          let wb = null, wo = 0;
          for (let i = 0; cat && i < L.nitems && !wb; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (!b || !b[o + L.iKind] || b[o + L.iSn] !== 0) continue;
            let n = '';
            for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
            if (n === 'well') { wb = b; wo = o; }
          }
          if (!wb) return;
          const wx = r16(wb, wo + L.iX), wy = r16(wb, wo + L.iY), st = stratumRec(m, 0);
          for (let d = 2; d < 12; d++) for (let dy = -d; dy <= d; dy++) for (let dx = -d; dx <= d; dx++) {
            const x = wx + dx, y = wy + dy;
            if (m.moved && m.moved.g === g || ((x - st.x0) >> 5) !== ((wx - st.x0) >> 5) || ((y - st.y0) >> 5) !== ((wy - st.y0) >> 5)) continue;
            const a = sq(0, x, y);
            if (!a || a.t !== 1) continue;
            let rock = true;
            for (let j = -1; j <= 1; j++) for (let i = -1; i <= 1; i++) { const b = sq(1, x + i, y + j); if (!b || b.t !== 0) rock = false; }
            if (!rock) continue;
            for (const [i, j] of [[0, 1], [1, 0], [0, -1], [-1, 0]]) {
              const p = sq(0, x + i, y + j);
              if (m.moved && m.moved.g === g || !p || p.t !== 1 || p.z !== a.z) continue;
              w16(wb, wo + L.iX, x);
              w16(wb, wo + L.iY, y);
              w16(h, L.pcX, x + i);
              w16(h, L.pcY, y + j);
              m.moved = { g, from: [wx, wy], to: [x, y], pc: [x + i, y + j] };
            }
          }
        };
        dq0Scene(m, [{ g: 1, line: 'look', n: 1, call: foot('pool') }, { g: 1, line: 'climb down the well', n: 2, call: move(1) }, { g: 1, line: 'look', n: 2, call: foot('cistern') },
          { g: 2, line: 'climb down the well', n: 2, call: move(2) }, { g: 2, line: 'look', n: 1, call: foot('sump') }, { g: 3, line: 'climb down the well', n: 2, call: move(3) }]);
      },
      check(m, out) {
        const T = dq0Turns(m), K = m.dq0.layout().K, f = [], o = dq0Prose(out), g = m.dq0.games;
        const climbs = t => t.replies.filter(r => /^slot/.test(r.req) && r.a === K.P_CLIMB).map(r => r.steps);
        const arr = (t, how) => T.filter(u => u.k >= t.k).flatMap(u => dq0Ev(u, 'E_ARRIVE', {})).find(e => (e.w1 & 0xFF) === how);
        const at = (s, nm) => s ? s.items.filter(i => i.name === nm && i.where === 'IW_SQUARE') : [];
        const feet = n => (m.feet || []).find(r => r.name === n);
        // Into the cave below: the dagger let fall in, the rope, down by it, the pool
        const g1 = T.of("dq0 -p t -s 3 -c /ram/c.hm -k 'rope,dagger' -S /ram/s.sc"), put = T.of('put the dagger in the well', g1.k);
        if (!/put the dagger in the well You reach for the dagger\. You let the dagger fall into the well\. A moment, and you hear a splash far below\./.test(o)) f.push('the dagger put in: not its words');
        const dg = at(put.after, 'dagger');
        if (dg.length !== 1 || dg[0].sn !== 1) f.push('the dagger not at the foot, on the stratum below: ' + JSON.stringify(dg));
        if (!/You tie the rope to the well and let it down the shaft/.test(o)) f.push('the rope not tied to the curb, let down its shaft');
        const tie = T.after(put, 'tie the rope to the well');
        if (at(tie.after, 'tied rope').length !== 1 || !at(tie.after, 'hanging rope').some(i => i.sn === 1)) f.push('the rope in place not at the curb and the foot below');
        const dn = T.after(tie, 'climb down the well'), cs = climbs(dn);
        if (!cs.length || !cs.slice(0, 3).every(s => s === 3)) f.push('the climb down not aided by the rope at first: ' + cs);
        const a1 = arr(dn, K.A_DOWN);
        if (!a1 || ![12, 13].includes(a1.w1 >> 12)) f.push('not come down a well\'s shaft (E_ARRIVE O_WELL): ' + JSON.stringify(a1));
        if (!/You (climb down the well, into the water at its foot|come up in the water at the foot of the well)\./.test(o)) f.push('no words at the foot');
        const p = feet('pool');
        if (!p || p.sn !== 1 || !p.here || p.here.t !== 7 || !p.here.wet || !p.about.some(s => s && (s.t === 1 || s.t === 4) && s.z - p.here.z === 2)) f.push('its foot in the cave not a pool two feet down, under water: ' + JSON.stringify(p && [p.sn, p.here, p.about]));
        if (!/look .{0,400}Above you the well's shaft rises to its mouth, far overhead\./.test(o)) f.push('the look at the foot not telling the well\'s shaft');
        if (!/throw the dagger down the well You reach for the dagger\..{0,80}You let the dagger fall into the well\. A moment, and you hear a splash far below\./.test(o)) f.push('the dagger thrown down the well: not its words');
        if (/You land hard/.test(o.slice(o.indexOf('jump into the well')))) f.push('a fall into the water told as landing hard');
        const tk = T.after(dn, 'take the dagger');
        if (!/take the dagger You reach for the dagger\. Taken\./.test(o)) f.push('the dagger not taken at the foot');
        const up = T.after(tk, 'climb out'), a2 = arr(up, K.A_UP);
        if (!a2 || a2.w1 >> 12 !== 12) f.push('not climbed out of the well: ' + JSON.stringify(a2));
        const top = T.after(up, 'climb down the well');
        if (!top.after || top.after.pc.sn !== 1) f.push('not down the moved well');
        if (!/climb out .*You climb up out of the well\. You come into the/.test(o)) f.push('climbing out not told');
        // The cistern, a way on
        if (!m.moved) f.push('no square over the rock below for the well');
        const c = feet('cistern');
        if (!c) f.push('the cistern\'s foot not seen');
        else {
          if (c.sn !== 1 || !c.here || c.here.t !== 7 || !c.here.wet) f.push('the cistern\'s foot not a way up under water on the stratum below: ' + JSON.stringify([c.sn, c.here]));
          if ((c.wet & 0x7F) < 5 || !c.swim) f.push('not swimming at the foot: ' + JSON.stringify([c.wet, c.swim]));
          if (!c.about.every(s => s && s.t === 4 && s.wet && s.z === c.here.z)) f.push('the cistern\'s eight not under its water: ' + JSON.stringify(c.about));
          const way = c.ways.findIndex(w => w[0] && w[0].t === 4 && w[1] && w[1].t === 4);
          const w = c.ways[way] || [];
          let k = 1;
          while (k < w.length && w[k - 1].wet && w[k] && w[k].t === 4 && w[k].z - w[k - 1].z <= 2 && w[k].z >= w[k - 1].z) k++;
          if (way < 0 || k < 3 || !w[k - 1] || w[k - 1].wet) f.push('no way on up out of its water: ' + JSON.stringify(w.slice(0, k + 1)));
        }
        // The sump, no rope: down and up unaided; jumped into
        const g2 = T.of('dq0 -p t -s 3 -c /ram/c.hm -S /ram/s.sc', g1.k + 1), d2 = T.after(T.after(T.after(g2, 'climb down the well'), 'climb out'), 'climb down the well');
        if (!climbs(d2).length || climbs(d2).some(s => s !== 0)) f.push('the sump\'s climb not unaided: ' + climbs(d2));
        const s = feet('sump');
        if (!s || s.sn !== 1 || !s.here || s.here.t !== 7 || !s.here.wet || !s.about.every(q => q && q.t === 0) || (s.wet & 0x7F) < 5) f.push('the sump\'s foot not one square of deep water in the rock: ' + JSON.stringify(s && [s.sn, s.here, s.wet, s.about]));
        const u2 = T.after(T.after(d2, 'look'), 'climb out');
        if (!climbs(u2).length || !arr(u2, K.A_UP)) f.push('the sump not climbed out of');
        const j = T.after(u2, 'jump into the well'), fall = j.replies.filter(r => /^slot/.test(r.req) && r.rs === 'RS_HARM' && r.ask.cause === 'HC_FALL');   // (By name: msgs.def's)
        if (!fall.length) f.push('the jump not a fall');
        const a3 = arr(j, K.A_DOWN);
        if (!a3 || a3.w1 >> 12 !== 13) f.push('the jump not into the well (O_WELL_FELL): ' + JSON.stringify(a3));
        if (!/jump into the well .*You plunge into the water(:|, shaken).*You come up in the water at the foot of the well\./.test(o)) f.push('the plunge not told');
        this.notes = [JSON.stringify(m.moved) + ' moved; ' + JSON.stringify(fall.map(r => [r.b, r.c, r.w1])) + ' the fall; the climbs ' + [cs, climbs(d2), climbs(u2)].map(x => x.length).join(', ')];
        // The turns pace: the sump's game again, typed late, the same
        if (g.length !== 3) f.push('games played: ' + g.length + ', not 3');
        else {
          const ev = k => g[k].events.concat(g[k].dens, g[k].replies).map(e => JSON.stringify(Object.assign({}, e, { line: undefined })));
          const a = ev(1), b = ev(2);
          let d = 0;
          while (d < a.length && d < b.length && a[d] === b[d]) d++;
          if (d < a.length || d < b.length) f.push('the late game not the same, from its event ' + d + ' (of ' + a.length + ' and ' + b.length + '): ' + (a[d] || 'none') + ' / ' + (b[d] || 'none'));
        }
        return f;
      }
    },
    {
      name: 'dq0swim', near: ['body'], what: 'Dingus Quest Zero\'s water (slice 4.9: DUNGEON.md\'s water, HOUSE.md\'s water: the plan\'s lakes, a chunk\'s water), by the words told and the hub: at seed 2 down the way down (the natural stratum below, a flooded chamber north of its landing, five feet deep), north into it: out of its depth, swimming (its depth in the hub\'s pc_wet, its squares a minute the referee\'s P_SWIM in pc_swim, its breath pc_breath), told "You\'re out of your depth: you swim.", its torch put out ("The water puts out your light."), its swimming told by the referee\'s grade ("You struggle to keep afloat."): in the dark then, the walk stopped',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 2 -c /ram/c.hm -T', 'd', 'n', 'look', 'wait 2 minutes', 'quit', 'echo $status'] })) },
      start(m) {   // (The character's water, as each line's typed: the hub's pc_wet, pc_swim, pc_breath)
        const a = m.acia, read = a.read;
        let typed = '';
        m.wet = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              const h = m.dq0 && m.dq0.bank(0);
              if (h) {
                const { L } = m.dq0.layout();
                m.wet.push({ line: typed, wet: h[L.pcWet], swim: h[L.pcSwim], breath: h[L.pcBreath] });
                if (typed === 'wait 2 minutes') dq0SceneNow(m, (h, L) => { h[L.pcsum + L.sCond] |= require('../../../games/dq0/hubgen.js').load().C.C_BLEEDING; }, null, 'dq0swim\'s bleeding', true);   // (The referee's summary: a scene's, RS_SCENE SCN_HUB)   // (rs.h's C_BLEEDING: its time told as it goes, a minute at a time)
              }
              typed = '';
            } else if (c >= 32) typed += String.fromCharCode(c);
          }
          return read.call(a, r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Prose(out).replace(/\[[.#]{10}\] /g, '');
        const at = line => o.indexOf('> ' + line + ' > ' + line + ' ');
        const n = at('n'), s = at('look');
        const ns = n >= 0 && s > n ? o.slice(n, s) : '';
        if (!ns.includes('You\'re out of your depth: you swim.')) f.push('north into the flooded chamber, not told "You\'re out of your depth: you swim."');
        if (!ns.includes('The water puts out your light.')) f.push('swimming, its torch not put out');
        if (!/You swim strongly\.|You struggle to keep afloat\./.test(ns)) f.push('its swimming not told (the referee\'s test)');
        const w = (m.wet || []).find(e => e.line === 'look');
        if (!w || (w.wet & 0x7F) < 5 || !w.swim || !w.breath) f.push('swimming not in the hub (pc_wet, pc_swim, pc_breath): ' + JSON.stringify(w));
        // (Swimming is vigorous for hunger and thirst, whatever its pace: the bodies' time told so, a 2, wtend.c's; HOUSE.md's want, the check of 2026-11-03)
        const K = m.dq0.layout().K, tm = dq0Turns(m).of('wait 2 minutes').replies.filter(r => /^slot/.test(r.req) && r.rs === 'RS_TIME');
        if (!tm.length || !tm.every(r => r.ask.about === 2)) f.push('a swimmer\'s time not told as vigorous: ' + JSON.stringify(tm.map(r => r.ask.about)));
        this.notes = [JSON.stringify(m.wet)];
        if (f.length) this.notes.push(o.slice(Math.max(0, n - 300), n + 1500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tread', near: ['body'], what: 'Dingus Quest Zero\'s swimming in place (the verbs plan\'s B6, 2026-11-10: DECISIONS.md\'s "Prayer, fasting, swimming and following"; HOUSE.md\'s water), by the words, the world\'s beats and the clock: at seed 2 (dq0swim\'s flooded chamber), "swim" on dry ground and on the landing refused ("There\'s no water deep enough."); north into the water, out of its depth; "swim" alone treads water ("You tread water.", a minute, the world\'s swim test as it treads: I_WAIT\'s WA_TREAD); "dive" goes under on purpose ("You take a deep breath and dive under.", the world\'s WB_UNDER), a minute, then strikes out for the surface ("You come up, gasping.", WB_SWIM); "tread water" the same as "swim"; "swim south" a move, out of the water ("You wade out of the water.")',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 2 -c /ram/c.hm -T', 'swim', 'd', 'swim', 'n', 'swim', 'dive', 'tread water', 'swim south', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), o = dq0Prose(out), f = [];
        inTurn(o, ['> swim There\'s no water deep enough.', '> swim There\'s no water deep enough.', 'You\'re out of your depth: you swim.',
          '> swim You tread water.', '> dive You take a deep breath and dive under.', 'You strike out for the surface.', 'You come up, gasping.',
          '> tread water You tread water.', '> swim south', 'You wade out of the water.'], f);
        const sw = T.filter(t => t.line === 'swim')[2], dv = T.of('dive');
        if (!sw || !sw.at || !sw.after || sw.after.now - sw.at.now < 600) f.push('the tread not a minute');
        dq0Need(f, dv, 'E_BEAT', { beat: 'WB_UNDER' }, 'the dive not under');
        dq0Need(f, dv, 'E_BEAT', { beat: 'WB_SWIM' }, 'the dive not come up');
        if (!dv.at || !dv.after || dv.after.now - dv.at.now < 600) f.push('the dive not a minute under');
        if (/in this game yet/.test(o)) f.push('a "not yet" said');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stillskin', near: ['body'], what: "Dingus Quest Zero's waterskin from still water (the user's, 2026-11-03: the same flux risk as drinking there: HOUSE.md's the diseases): at seed 2 the old kit's empty skin filled in the flooded chamber's still water: the water in it marked (the item's wear), a weak character (its Endurance low) drinking from it, away from the water, catches the flux: told a day on",
      init: 't_rc', cycles: 3500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att sta 1 >> /ram/c.hm', 'echo att str 1 >> /ram/c.hm', 'echo att wil 1 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Vermin | rat | 7-7 | 30 | - | 1 | w | -' > /ram/g/theme.txt",
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g', 'd', 'n', 'fill the waterskin', 's', 'wait 3 hours', 'drink from the waterskin', 'wait 3 hours', 'drink from the waterskin', 'wait 3 hours', 'drink from the waterskin', 'wait 8 hours', 'wait 8 hours', 'wait 8 hours', 'wait 8 hours', 'quit', 'echo $status'] })) },
      start(m) {
        const wearOf = (L) => { const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), r = []; for (let i = 0; cat && i < L.nitems; i++) { const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize; if (!b || !b[o + L.iKind]) continue; let n = ''; for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]); if (n === 'water') r.push(b[o + L.iWear]); } return r; };
        m.foul = [];
        const at = when => (h, L) => m.foul.push({ when, wear: wearOf(L) });
        dq0Scene(m, [{ g: 1, line: 'd', n: 1, call: at('before') }, { g: 1, line: 's', n: 1, call: at('filled') }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out);
        if (!/You fill your waterskin\./.test(s)) f.push('the skin not filled');
        const b = m.foul.find(e => e.when === 'before'), a = m.foul.find(e => e.when === 'filled');
        this.notes = [JSON.stringify(m.foul)];
        if (!b || b.wear.includes(1)) f.push('the kit\'s water marked foul: ' + JSON.stringify(b));
        if (!a || !a.wear.includes(1)) f.push('the still water not marked: ' + JSON.stringify(a));
        if (!/you have the flux\./.test(s)) f.push('the flux not caught from the skin');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0growth', outfit: true, near: ['body', 'save'], what: "Dingus Quest Zero's skills' growth (the user's, 2026-11-03: \"Stress, or 5 routine uses\", HOUSE.md's a skill's growth), by the referee's own record of it (skills.c's, m.grow): hiding a thing, lit, nobody near: a routine use counted, the fifth a roll, the count back to none; a hostile awake beside it: a roll for one use; wounded (its file's injury) with one beside it: a very dangerous use, two; rolls held to three; in the dark without a light a roll; eight hours' sleep spends the rolls, told if one rose, the routine count kept; and it's kept in its file ('body u' lines) and read back as the game's carried on",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml stl 20 >> /ram/c.hm', 'echo ml wpn 20 >> /ram/c.hm', 'mkdir /ram/g', 'mkdir /ram/q', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k chalk,knife,candle,rope,brick -S /ram/s.sc',
        'look', ...Array(5).fill(['hide the chalk', 'take the chalk']).flat(), 'appraise the knife', 'look', 'appraise the knife', 'appraise the knife', 'appraise the knife', 'wait', 'sleep for 8 hours', 'wait', 'quit', 'cat /ram/g/pc.hm', 'echo inj 14 1 2 2 0 0 >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -a -k chalk -S /ram/s.sc', 'look', 'hide the chalk', 'take the chalk', 'put out the torch', 'look', 'hide the chalk', 'take the chalk', 'quit', 'echo $status'] })) },
      start(m) {
        const p = [0, 1, 2].map(k => ({ g: 1, line: 'look', n: 1, k, set: { attitude: 4, doing: 0 } }));
        for (const n of [1, 2, 3, 4]) p.push({ g: 1, line: 'appraise the knife', n, k: 0, set: { attitude: 0, doing: 1 }, beside: 4 });
        for (const k of [0, 1, 2]) p.push({ g: 1, line: 'wait', n: 1, k, set: { attitude: 4, doing: 0 } });
        dq0Scene(m, p);
        dq0Grow(m);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], STL = 7, THR = 88, at = (line, n, g) => m.grow.find(x => x.g === (g || 1) && x.line === line && x.n === n), show = () => m.grow.map(g => g.g + g.line.slice(0, 4) + g.n + ':' + g.rolls(STL) + '/' + g.uses(STL)).join(' ');
        this.notes = [show()];
        const need = (t, r, u, what) => { if (!t || t.rolls(STL) !== r || t.uses(STL) !== u) f.push(what + ': ' + show()); };
        need(at('take the chalk', 4), 0, 4, 'four routine uses not counted, no roll');
        need(at('take the chalk', 5), 1, 0, 'the fifth not a roll, its count back to none');
        const th = at('look', 2), w1 = at('wait', 1), w2 = at('quit', 1);   // (Its rest told the referee a moment after it wakes: the quit's)
        this.notes.push('Weaponcraft ' + th.rolls(THR) + '/' + th.uses(THR) + ', ' + w1.rolls(THR) + '/' + w1.uses(THR) + ', ' + w2.rolls(THR));
        if (!th || th.rolls(THR) !== 1 || th.uses(THR)) f.push('a foe beside it not a stress roll: ' + th.rolls(THR) + '/' + th.uses(THR));
        if (!w1 || w1.rolls(THR) !== 3) f.push('rolls not held at three: ' + w1.rolls(THR));
        if (!w2 || w2.rolls(THR) || w2.rolls(STL)) f.push('a night\'s sleep not spending the rolls');
        need(at('look', 1, 2), 0, 0, 'its growth not carried with its file');
        need(at('put out the torch', 1, 2), 1, 0, 'wounded, a use not a roll');
        need(at('quit', 1, 2), 3, 0, 'wounded in the dark, a very dangerous use not two rolls');
        if (!/\nbody u\d+ /.test(out.slice(out.indexOf('% cat /ram/g/pc.hm')))) f.push('its growth not in its file');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dampwarn', near: ['body'], what: "Dingus Quest Zero's firedamp's warning (the user's, 2026-11-03: \"Warn before bursting\": HOUSE.md's firedamp's burst): dq0air's strong firedamp (dq0air=32), a torch lit, walking in step by step: the first step in the pale blue cap on the flame, no burst; the next step the burst; and backing out after the warning, none",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm',
        'echo air 32 >> /ram/s.sc', 'dq0 -p t -s 1 -c /ram/c.hm -T -S /ram/s.sc', 'open the door', 'south', 'south', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), cap = s.indexOf('a pale blue cap trembling over it'), burst = s.indexOf('The air ignites with a roar');
        this.notes = ['cap at ' + cap + ', burst at ' + burst];
        if (cap < 0) f.push('no warning on its flame: ' + JSON.stringify(s.slice(-700)));
        if (burst < 0) f.push('no burst on after the warning: ' + JSON.stringify(s.slice(-700)));
        else if (cap > burst) f.push('the burst before the warning');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0antidote', near: ['body'], what: "Dingus Quest Zero's remedies (2026-11-02: HOUSE.md's remedies, for review), both referees: dq0venom's hostile cave adders (seed 1, the character tough), the character with an antidote and a remedy (dq0 -k), the antidote stowed; bitten, the venom in it (\"The bite burns: there's venom in it.\"), it drinks the antidote as it's told (dq0OnWords': \"You drink the antidote.\", the things' IO_USE by its key, the referee's RS_TIME RT_TAKEN, its words at once): the toxin's rate the better by its power, dosed by Physician (\"The remedy works\", or \"eases you\"), the antidote gone; the remedy drunk with no sickness on it does nothing (\"It does nothing for you.\"); under the null referee an antidote with nothing to treat the same; and a disease cured (the flux in the character's file's body line, Physician 95, remedies drunk: \"The remedy works: the sickness leaves you.\", feverish no more)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Serpents | adder | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k antidote,remedy', 'stow the antidote', 'open the door', 'south', 'inventory', 'drink the remedy', 'quit',
        'dq0 -r null -p t -s 1 -T -k antidote', 'drink the antidote', 'quit',
        'hmchar -q -s 5 -u feudal -c hunter -o /ram/d.hm', 'echo ml phy 95 >> /ram/d.hm', 'echo body 0 0 0 0 0 4 6 0 0 0 0 >> /ram/d.hm', 'dq0 -p t -s 1 -c /ram/d.hm -T -k remedy,remedy', 'status', 'drink the remedy', 'drink the remedy', 'status', 'quit', 'echo $status'] })) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'south', n: 1, words: "there's venom in it.", text: 'drink the antidote\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), g = s.slice(0, s.indexOf('dq0 -r null')), n = s.slice(s.indexOf('dq0 -r null')), i = g.indexOf('You drink the antidote.');
        if (!/there's venom in it\./.test(g)) f.push('no venom: the scene not set');
        if (i < 0) f.push('the antidote not drunk');
        if (i >= 0 && !/The remedy works|The remedy eases you/.test(g.slice(i, i + 600))) f.push('the antidote did nothing: ' + JSON.stringify(g.slice(i, i + 600)));
        if (!/You drink the remedy\.(?: You've begun[^.]*\.)? It does nothing for you/.test(g)) f.push('the remedy, nothing to treat: ' + JSON.stringify(g.slice(g.indexOf('> drink the remedy'), g.indexOf('> drink the remedy') + 300)));
        if (!/You drink the antidote\. It does nothing for you/.test(n)) f.push('the null referee: ' + JSON.stringify(n.slice(0, 400)));
        // (A disease cured: the flux shown, H4, in the character's file's body line, Physician 95; each remedy a point the
        // better, two at a critical success: one or both cure it)
        const z = s.slice(s.indexOf('-k remedy,remedy')), z1 = z.indexOf('> drink the remedy'), z2 = z.lastIndexOf('> status');
        if (!/feverish/.test(z.slice(0, z1))) f.push('the disease not on it: ' + JSON.stringify(z.slice(0, 500)));
        if (!/You drink the remedy\. The remedy works: the sickness leaves you\./.test(z)) f.push('the remedies not curing it: ' + JSON.stringify(z.slice(z1, z1 + 400)));
        if (/feverish/.test(z.slice(z2))) f.push('still feverish after: ' + JSON.stringify(z.slice(z2, z2 + 300)));
        const inv = T.of('quit');
        if (inv && inv.at && dq0Carried(inv.at).some(j => j.name === 'antidote')) f.push('the antidote still carried');
        if (f.length) f.push(g.slice(g.indexOf('> open the door'), g.indexOf('> open the door') + 2500));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0paceword', near: ['body', 'senses'], what: "Dingus Quest Zero's words tell the pace it's really going (soak seed 192: told \"You jog south\" in shock, \"You jog headlong into a wall\" swimming; HOUSE.md's a run in shock is that walk), by the words: in the dark, a run said in shock a walk (\"You walk south into the dark.\", no run into the wall, the status command's \"held to a walk in shock\"); on one leg no run told, a hop feeling its way (the status command's \"held to a walk on one leg\"); swimming, its hands finding the wall, no run into it; a walk's way told once in the dark; a run whole, a run still",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/a.hm', 'echo inj 12 0 3 4 0 8 >> /ram/a.hm',
        'dq0 -p t -s 7 -c /ram/c.hm -S /ram/s.sc', 'face north', 'open the door behind me', 'douse the torch', 's', 'wait', 'run south', 'status', 'quit',
        'dq0 -p t -s 3 -c /ram/a.hm -S /ram/s.sc', 'douse the torch', 'run east', 'status', 'quit',
        'dq0 -p t -s 17 -c /ram/c.hm -T -S /ram/s.sc', 'd', 'n', 'step east', 'run north', 'quit', 'dq0 -p t -s 7 -c /ram/c.hm -S /ram/s.sc', 'douse the torch', 'run east', 'quit', 'echo $status']) },
      start(m) {
        const off = (h, L, K) => {
          const at = k => L.actor + k * L.actorSize, R = dq0Rec(m), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, due = now + 3000;
          let n = 0;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            const o = R.at(h[at(k) + L.aDen]), x = 32706 + (n & 3), y = 32784 + (n++ >> 2);
            h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8;
            h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_FRIENDLY;
            h[at(k) + L.aDoing] = K.DO_ROAM;
            for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = due >>> (8 * i) & 255;
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: () => dq0PcShock(m, 3000) }, { g: 3, line: 'n', n: 1, call: off }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(out), games = o.split('dq0 -p t -s ').slice(1);
        inTurn(games[0], ['> run south', 'You walk south into the dark.', 'You walk into', '> status', 'Pace: a run, held to a walk in shock'], f, 'in shock: ');
        if (/You run |You jog |headlong/.test(games[0])) f.push('in shock, a run told: ' + games[0].slice(games[0].indexOf('> run south'), games[0].indexOf('> status')));
        if (/You set off south/.test(games[0].slice(games[0].indexOf('> run south'), games[0].indexOf('> status')))) f.push('its way told twice in the dark');
        inTurn(games[1], ['> run east', "You can't run on one leg.", 'You hop along on your good leg.', 'Your hands meet a wall.', '> status', 'held to a walk on one leg'], f, 'on one leg: ');
        if (/You run |headlong/.test(games[1])) f.push('on one leg, a run told: ' + games[1]);
        inTurn(games[2], ["You're out of your depth: you swim.", '> run north', 'Your hands meet a wall.'], f, 'swimming: ');
        if (/You run |headlong/.test(games[2])) f.push('swimming, a run told: ' + games[2]);
        inTurn(games[3], ['> run east', 'You run east into the dark.', 'You run headlong into a wall!'], f, 'a run in the dark, whole: ');
        return f;
      },
      sdcard: true
    },
    // (dq0answer96, the same at 9,600 baud: its times noted, no budget of its own (the user's targets, D17, are 115,200's):
    // only with DQ0ANS96=1, to measure)
    ...(process.env.DQ0ANS96 ? [dq0AnswerTest(9600)] : []),
    {
      name: 'dq0venom', what: "Dingus Quest Zero's wounds over time (slice 4.3: HOUSE.md's wounds over time), a venom's and the bleeding's: a theme's hostile cave adders (written by hand: seed 1, a room of them south of the start), the character tough (STR, STA and WIL 99: Endurance 99): an adder's bite that wounds carries its venom (the HarnWorld Bestiary's heath adder: H3, its first roll at once), told after the bite (\"The bite burns: there's venom in it.\"), and a B1 bleeder (BLEEDING on the status line); its state's words poisoned, where the venom's got to, and weak from the loss of blood; its many bites bleeding a point a minute each, first aid's tourniquet tried a minute at a time (its penalties too great: \"The blood still flows.\", told once), bled to death (\"You bleed to death.\", the referee's RS_TIME, a minute at a time), the game's end",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Serpents | adder | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g', 'open the door', 'south', 'drop the torch', 'south', 'wait', 'flee', 'flee', 'status', 'tend my wounds', 'wait 2 hours', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out);
        if (!/The cave adder bites at you: a [a-z]+ (?:deep )?bite to your [a-z ]+ \([MSG]\d+\)\. The bite burns: there's venom in it\./.test(s))
          f.push('no venom told after a bite');
        if (!/UP \d+ BLEEDING/.test(s)) f.push('not BLEEDING on the status line');
        if (!/Condition: [^.]*; poisoned, the venom in your [a-z ]+;[^.]*from the loss of blood/.test(s))
          f.push('its state not poisoned, nor weak from the loss of blood: ' + JSON.stringify((s.match(/Condition: [^.]*\./) || [''])[0]));
        if (!/You bleed to death\. You are dead\./.test(s)) f.push('not bled to death');
        if (!T.some(t => dq0Ev(t, 'E_BYE', {}).length)) f.push('the game not ended by the death');
        // (Engaged as it walked: its engaging alone, not its closing after it, the user's, 2026-11-18)
        if (/engages you: you stop walking\.\s+The cave adder closes with you/.test(s)) f.push('the engage line and the closing both told');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0air', near: ['body'], what: "Dingus Quest Zero's bad air (2026-10-31, the user's: \"poor air quality (CO or CO2 excess, low oxygen levels, gas pockets--which could explode!)\"; HOUSE.md's bad air, rsair.h's, every ruleset's), its scenes the environment's dq0air (rc's: every chunk a pocket from end to end, the maker's caves.c's air_pocket), the character tough (STR, STA and WIL 99), its torch lit: at seed 1 (dq0 -T) thick blackdamp (dq0air=13) as it steps south through the door, its torch put out by the foul air (the world's, part 73's), its head swimming at once, its legs failing two minutes in, dead of it five minutes in (\"You can't get your breath\"), the game ended; firedamp (dq0air=32) at its naked flame bursting (\"The air ignites with a roar\"), a burn told after (the world's harm: HC_FIRE), its torch out; thin firedamp under the null referee (dq0air=31: its flame's blue cap, the miners' warning, nothing to breathe)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm',
        'echo air 13 >> /ram/s.sc', 'dq0 -p t -s 1 -c /ram/c.hm -T -S /ram/s.sc', 'open the door', 'south', 'wait 10 minutes', 'wait 10 minutes', 'wait 10 minutes',
        'echo air 32 >> /ram/s.sc', 'dq0 -p t -s 1 -c /ram/c.hm -T -S /ram/s.sc', 'open the door', 'south', 'status', 'quit',
        'echo air 31 >> /ram/s.sc', 'dq0 -r null -p t -s 1 -T -S /ram/s.sc', 'open the door', 'south', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), a = s.indexOf('echo air 32 >> /ram/s.sc'), b = s.indexOf('echo air 31 >> /ram/s.sc');
        const g1 = a < 0 ? s : s.slice(0, a), g2 = a < 0 || b < 0 ? '' : s.slice(a, b), g3 = b < 0 ? '' : s.slice(b);
        if (!/The air's too foul to keep your light burning\./.test(g1)) f.push('blackdamp: its torch not put out');
        if (!/Your head swims; your heart pounds\./.test(g1)) f.push('blackdamp: its breath not told at once');
        if (!/Your legs are failing you/.test(g1)) f.push('blackdamp: its legs not failing: ' + JSON.stringify(g1.slice(-1200)));
        if (!/You can't get your breath: the dark takes you\./.test(g1)) f.push('blackdamp: not dead of it');
        if (!/The air ignites with a roar/.test(g2)) f.push('firedamp: no burst at the naked flame');
        if (!/It hurts you: [^.]*burn/.test(g2)) f.push('firedamp: no burn after the burst: ' + JSON.stringify(g2.slice(0, 600)));
        if (!/a pale blue cap trembling over it\./.test(g3)) f.push('thin firedamp, the null referee: no blue cap on its flame');
        if (/The air ignites/.test(g3)) f.push('thin firedamp burst');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tend', what: "Dingus Quest Zero's first aid and healing (slice 4.3: HOUSE.md's wounds over time; Physician 2-4), both referees (its hunger and thirst poked low at each sleep: dq0Poke's, slice 4.9's): at seed 517 the kobold warren's gatehouse, its ledge jumped down onto the gatehouse's hard floor (the referee's GR_HARD: its room's, the passages' till the load guard found s_fall reading its square in a chunk's bank), a minor bruise to the calf (dq0harm's); \"tend my head\" not the grammar's (no such thing); \"tend my wounds\" the book's treatment for a minor bruise, a cold compress (5d6 minutes: a timed work, its long task LT_TEND, its words at once and its outcome at its end), its healing rate on the wound (the sheet's \"tended\", the state's \"all tended\"), \"first aid\" again nothing to tend (one treatment roll a wound); then a night's sleep, again and again (its torch put out first), till five days' rest have gone by: the book's five-day healing roll (its rate times its Endurance), the wound healed, told as it wakes (\"The minor bruise to your right calf (M1) has healed.\"), the sheet's injuries none; and the null referee's simple way, a light wound tended (\"You clean and bind a wound.\") and healed by eight hours' rest (\"A wound has healed.\")",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 517 -c /ram/c.hm -d /ram/hurt', 'ne', 'e', 'u', 'u',
        'turn around', 'jump down', 'tend my head', 'tend my wounds', 'status', 'sheet', 'first aid', 'douse the torch', ...Array(20).fill('sleep'), 'sheet', 'quit',
        'dq0 -r null -p t -s 517 -d /ram/h2', 'ne', 'e', 'u', 'u', 'turn around', 'jump down', 'first aid', 'first aid', 'douse the torch', 'sleep', 'status', 'quit', 'echo $status'] })) },
      // (Each sleep typed at its prompt again: a sleep's last words of its wants, told as it wakes, came after the prompt
      //  at times (the bodies' time run by the lights' minute after the world's E_READY), typed at once falling into the
      //  next answer; the world now brings the body up to its waking first, 2026-11-18)
      start(m) {   // (Its water and food: a five days\' rest without them its death of thirst, slice 4.9\'s; so the scene\'s want
        // set low as each sleep\'s typed, its levels as they are at eight hours, none to rise: the wounds\' healing the test\'s)
        const want = { pcHunger: 0, pcHungerHi: 0, pcThirst: 0, pcThirstHi: 0, pcWant: 0x11 };
        dq0Scene(m, [...Array(20).keys()].map(k => ({ g: 1, line: 'sleep', n: k + 1, hub: want })).concat([{ g: 2, line: 'sleep', n: 1, hub: want },
          { g: 1, line: 'jump down', n: 1, rig: [{ kind: 'HARM', a: 'HC_FALL', uses: 1, faces: [1, 1, 92] }] }]));   // (The fall's dice rigged, dice2: 2 on
          //   the hard floor's two d6, a mid strike's 92 the right calf: the bruise the test tends)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), at = s.indexOf('Dingus, Wanderer'), h = at < 0 ? s : s.slice(0, at), n = at < 0 ? '' : s.slice(at);
        if (!/You land hard: a minor bruise to your right calf \(M1\)\./.test(h)) return ['no bruise from the fall (the seed\'s): ' + JSON.stringify((h.match(/You land hard[^.]*\./) || [''])[0])];
        const jd = dq0Turns(m).of('jump down').replies.find(q => q.rs === 'RS_HARM' && q.ask.cause === 'HC_FALL');   // (The ground its
        if (!jd || jd.ask.ground !== 'GR_HARD') f.push('the fall not onto the gatehouse\'s floor, hard: ' + JSON.stringify(jd && jd.ask));   //   room's floor's)
        if (!/tend my head I don't understand that\./.test(h)) f.push('"tend my head" not refused by the grammar');
        const tw = dq0Turns(m).of('tend my wounds').intents.find(e => e.kind === 'I_WAIT');   // (Its intent by its payload's
        if (!tw || tw.task !== 'LT_TEND' || !tw.quiet || !tw.tenths) f.push('"tend my wounds" not a timed work\'s I_WAIT, LT_TEND: ' + JSON.stringify(tw));   //   names, msgs.def's)
        if (!/You hold a cold compress to the minor bruise to your right calf \(M1\)\.[^>]*It's (?:well tended|tended|tended, roughly|badly done)\./.test(h))
          f.push('no compress, nor its outcome at its end');
        if (!/Condition: hurt: 1 injury, a minor bruise to your right calf \(M1\); all tended/.test(h)) f.push('its state not "all tended"');
        if (!/Injuries: right calf M1 \(bruise\), tended\./.test(h)) f.push('the sheet\'s injury not "tended"');
        if (!/first aid You have no wounds that want tending\./.test(h)) f.push('first aid again not nothing to tend');
        if (!/The minor bruise to your right calf \(M1\) (?:has healed|is mending|is no better)\./.test(h)) f.push('the bruise\'s five-day roll not told as it woke');
        if (/has healed\./.test(h) && !/No injuries/.test(h.slice(h.lastIndexOf('> sheet')))) f.push('the sheet\'s injuries not none once it healed');
        if (!/You land hard: a light wound\./.test(n)) return [...f, 'the null referee\'s fall not a light wound'];
        if (!/first aid You clean and bind a wound\.[^>]*It's (?:tended|badly done)\./.test(n)) f.push('the null referee\'s first aid not its own');
        if (!/first aid You have no wounds that want tending\./.test(n)) f.push('the null referee\'s wound tended twice');
        if (!/A wound has healed\./.test(n)) f.push('the null referee\'s wound not healed by a night\'s rest');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0healaid', what: 'Dingus Quest Zero\'s "heal" without the talent first aid (the user\'s, 2026-11-17: DECISIONS.md\'s "The critique\'s questions: the user\'s answers": "\\"heal\\" without the talent is first aid on wounds, by skill; with the talent it stays the psionic power"; no character has a talent yet, D11\'s): at seed 517 the kobold warren\'s ledge jumped down (dq0tend\'s: a minor bruise to the calf), "heal" the book\'s treatment, a cold compress (a timed work, its long task LT_TEND, as "tend my wounds"), "heal" again nothing to tend, "healing" the same; "heal my shoulder" a part named, nothing there to tend; never "You reach for it; nothing stirs." (the psionic refusal)',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 517 -c /ram/c.hm -d /ram/hurt', 'ne', 'e', 'u', 'u',
        'turn around', 'jump down', 'heal', 'status', 'heal', 'healing', 'heal my shoulder', 'quit', 'echo $status'] })) },
      start(m) {   // (The fall's dice rigged, dice2, dq0tend's: the bruise to the right calf the test tends)
        dq0Scene(m, [{ g: 1, line: 'jump down', n: 1, rig: [{ kind: 'HARM', a: 'HC_FALL', uses: 1, faces: [1, 1, 92] }] }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out);
        if (!/You land hard: a minor bruise to your right calf \(M1\)\./.test(s)) return ['no bruise from the fall (the seed\'s): ' + JSON.stringify((s.match(/You land hard[^.]*\./) || [''])[0])];
        const tw = dq0Turns(m).of('heal').intents.find(e => e.kind === 'I_WAIT');
        if (!tw || tw.task !== 'LT_TEND' || !tw.quiet || !tw.tenths) f.push('"heal" not first aid\'s timed work, I_WAIT LT_TEND: ' + JSON.stringify(tw));
        if (!/> heal You hold a cold compress to the minor bruise to your right calf \(M1\)\.[^>]*It's (?:well tended|tended|tended, roughly|badly done)\./.test(s))
          f.push('"heal" not the compress, nor its outcome at its end');
        if (!/Condition: hurt: 1 injury, a minor bruise to your right calf \(M1\); all tended/.test(s)) f.push('its state not "all tended"');
        inTurn(s, ['> heal', 'You have no wounds that want tending.', '> healing', 'You have no wounds that want tending.', '> heal my shoulder'], f, 'the words: ');
        if (/nothing stirs\./.test(s)) f.push('"heal" refused as the psionic talent');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0limbs', what: 'Dingus Quest Zero\'s limbs (slice 4.1b, the user\'s, 2026-10-21: "if you have a limb injury, it should affect that limbs usefulness"; 2026-10-22: "For an arm or hand, that means no picking things up with that hand"), by where each thing is (the item store\'s), the referee\'s summary (its limbs) and the words: a character with a grievous wound to its left forearm, its limb useless (its file\'s inj line: the book\'s, Combat 14): its summary\'s limbs its left arm useless; at seed 3 (dq0 -T, dq0 -k pick) the kit\'s torch in its right hand (its off hand the useless one), the pick (two-handed) in the pack; the dagger refused its left hand ("Your left arm is useless."), the pick refused ("You can\'t hold the pick in one hand."), the hands not swapped (the torch not into the useless hand)',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['cp /ram/c.hm /ram/a.hm', 'echo inj 6 2 3 4 0 8 >> /ram/a.hm', 'dq0 -p t -s 3 -c /ram/a.hm -T -k pick', 'i',
        'hold the dagger in my left hand', 'hold the pick', 'swap hands', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out);
        const g = T.of('dq0 -p t -s 3 -c /ram/a.hm -T -k pick');
        const thing = (t, n) => t && dq0Thing(t.after, n);
        const tr = thing(g, 'torch'), pk = thing(g, 'pick');
        if (!tr || !(tr.flags & DQ0_I.HELD) || (tr.sn & 3) !== 2) f.push('the kit\'s torch not in its right hand, its left arm useless (' + JSON.stringify(tr) + ')');
        if (!pk || (pk.flags & DQ0_I.HELD)) f.push('the pick held, its left arm useless');
        if (!g.after || !(g.after.sum.limbs & 4)) f.push('the summary\'s limbs not its left arm useless (' + (g.after && g.after.sum.limbs) + ')');
        const d = thing(T.of('hold the dagger in my left hand'), 'dagger');
        if (d && (d.flags & DQ0_I.HELD)) f.push('the dagger taken into its useless hand');
        const sw = thing(T.of('swap hands'), 'torch');
        if (!sw || (sw.sn & 3) !== 2) f.push('the torch moved into its useless hand');
        inTurn(s, ['> i', 'You carry: (a) a torch (lit), in your right hand', '> hold the dagger in my left hand', 'Your left arm is useless.',
          '> hold the pick', 'Your left arm is useless. You can\'t hold the pick in one hand.', '> swap hands', 'Your left arm is useless.'],
          f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0legs', what: 'Dingus Quest Zero\'s legs (slice 4.1b, the user\'s, 2026-10-21: "A serious fracture to the leg should make it more difficult to walk"; 2026-10-22: "You can stand and hop with one good leg.  Hopping would be quite slow (crawl speed), and would tire you at the strenuous work rate.  A crutch could be used to move at walk speed"; "both arms are required if both legs are unusable"), by the referee\'s summary (its limbs, its limp, its conditions), the hub\'s pc_pos and the words, at seed 3 (dq0 -T): a serious wound to the left thigh (its file\'s inj line), its walk a limp (each level a fifth slower: 3, 22 a square for 14, its load within its free pounds by Strength, D13\'s), told ("You limp."); a grievous one, the leg useless: hopping (a crawl\'s pace, three times a walk\'s, strenuous work: "You hop along on your good leg."), no run ("You can\'t run on one leg."), "hop north" said; with a spear in hand (dq0 -k spear, long and sturdy) a crutch at a walk\'s pace ("You lean on the spear and hobble along."), "crawl south" down and crawling ("You crawl.", PRONE on the status line), "walk north" prone getting up first (the book\'s Rise: its test, seriously hurt), "crawl north", "get up" (its test the dice\'s, a round: "You get to your feet.", or "You struggle, but can\'t get up."); both legs useless: falling at its first move and crawling ("You fall.", "You crawl."), "get up" refused ("Your legs won\'t bear you."); both legs and an arm: no move at all ("You can\'t move.")',
      init: 't_rc', cycles: 3600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['cp /ram/c.hm /ram/b.hm', 'echo inj 12 0 2 3 0 0 >> /ram/b.hm', 'dq0 -p t -s 3 -c /ram/b.hm -T', 'n', 'quit',
        'cp /ram/c.hm /ram/a.hm', 'echo inj 12 0 3 4 0 8 >> /ram/a.hm', 'dq0 -p t -s 3 -c /ram/a.hm -T', 'n', 'run south', 'hop north', 'quit',
        'dq0 -p t -s 3 -c /ram/a.hm -T -k spear', 'n', 'crawl south', 'walk north', 'crawl north', 'get up', 'quit',
        'cp /ram/a.hm /ram/d.hm', 'echo inj 140 0 3 4 0 8 >> /ram/d.hm', 'dq0 -p t -s 3 -c /ram/d.hm -T', 'n', 'get up', 'quit',
        'echo inj 6 2 3 4 0 8 >> /ram/d.hm', 'dq0 -p t -s 3 -c /ram/d.hm -T', 'n', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out);
        const game = c => T.of('dq0 -p t -s 3 -c /ram/' + c + ' -T');
        const g1 = game('b.hm'), n1 = T.after(g1, 'n');
        if (!n1.after || n1.after.sum.limp !== 3 || n1.after.sum.gstep[2] !== 22) f.push('a serious thigh wound not a limp, 3 levels, 22 a square (' + (n1.after && [n1.after.sum.limp, n1.after.sum.gstep]) + ')');
        const g2 = game('a.hm'), n2 = T.after(g2, 'n'), r2 = T.after(n2, 'run south'), h2 = T.after(r2, 'hop north');
        if (!n2.after || !(n2.after.sum.limbs & 0x10) || !(n2.after.sum.cond & 0x2000)) f.push('a grievous thigh wound not its left leg useless, crawling (' + (n2.after && [n2.after.sum.limbs, n2.after.sum.cond]) + ')');
        const g3 = T.of('dq0 -p t -s 3 -c /ram/a.hm -T -k spear'), n3 = T.after(g3, 'n'), c3 = T.after(n3, 'crawl south'), u3 = T.after(c3, 'walk north');
        const c4 = T.after(u3, 'crawl north'), up = T.after(c4, 'get up');
        const took = x => x.after && x.at ? x.after.pc.now - x.at.pc.now : -1;
        if (took(n2) < 2 * took(n3)) f.push('the hop not at a crawl\'s pace, three times the crutch\'s walk (' + took(n2) + ', ' + took(n3) + ')');
        if (!c3.after || c3.after.pc.pos !== 1) f.push('"crawl south" on one leg not prone');
        if (!u3.after || u3.after.pc.pos !== 0) f.push('a walk prone not up first');
        if (!c4.after || c4.after.pc.pos !== 1) f.push('"crawl north" not down');
        const rose = up.after && up.after.pc.pos === 0;     // (Its test the dice's: up, or told it struggles, a round either way)
        if (took(up) !== 100) f.push('"get up" not a round (' + took(up) + ')');
        const g4 = game('d.hm'), n4 = T.after(g4, 'n'), u4 = T.after(n4, 'get up');
        if (!n4.after || n4.after.pc.pos !== 1 || !u4.after || u4.after.pc.pos !== 1) f.push('both legs useless not down and crawling, not kept down');
        const g5 = T.after(u4, 'dq0 -p t -s 3 -c /ram/d.hm -T'), n5 = T.after(g5, 'n');
        if (!n5.after || !(n5.after.sum.cond & 0x4000) || n5.after.pc.x !== n5.at.pc.x || n5.after.pc.y !== n5.at.pc.y) f.push('both legs and an arm useless: not stuck');
        inTurn(s, ['> n', 'You limp.', '> quit',
          '> n', 'You hop along on your good leg.', '> run south', 'You can\'t run on one leg.', '> hop north', '> quit',
          '> n', 'You lean on the spear and hobble along.', '> crawl south', 'You crawl.', 'PRONE', '> walk north', 'You get to your feet.',
          '> crawl north', 'You crawl.', '> get up', rose ? 'You get to your feet.' : 'You struggle, but can\'t get up.', '> quit',
          '> n', 'You fall.', 'You crawl.', '> get up', 'Your legs won\'t bear you.', '> quit', '> n', 'You can\'t move.'], f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0loadtire', outfit: true, near: ['body'], what: 'Dingus Quest Zero\'s load and tiring (the user\'s, 2026-11-03, DECISIONS.md\'s: "I seem to acquire fatigue if overburdened even when sleeping", "if you are awake and standing with the burden, you should still get fatigued"; HOUSE.md\'s a load borne, for review): overloaded (dq0 -k), 70 minutes standing tires it (status\'s fatigue), a level or more; lying down, sitting and asleep no more tired, its fatigue less, overloaded or not; a sleeper lies down by itself; heavily burdened standing no rest; hunger and thirst told by status, and their share of the fatigue, after a long sleep; a light load (the kit alone) standing rests as ever',
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', "echo 'faction | Vermin | rat | 7-7 | 30 | - | 1 | w | -' > /ram/g/theme.txt",
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k pick,shovel,rope,sword,bones,pick,shovel,sword,pick,shovel,sword,pick,shovel,sword,pick,shovel,sword', 'douse the torch', 'status', 'wait 70 minutes', 'status',
        'lie down', 'wait 40 minutes', 'status', 'stand up', 'wait 70 minutes', 'status', 'sit', 'wait 40 minutes', 'status', 'stand up', 'wait 70 minutes', 'status', 'sleep 40 minutes', 'status', 'stand up', 'wait 70 minutes', 'status',
        'drop the pack', 'drop the pick', 'drop the sword', 'drop the rope', 'drop the bones', 'drop the sword', 'drop the pick', 'drop the shovel', 'status', 'wait 40 minutes', 'status', 'sleep 20 hours', 'status', 'sleep 20 hours', 'status', 'sleep 20 hours', 'status', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(out).replace(/\[[.#]{10}\] /g, '');
        const st = [...o.matchAll(/Condition: (.*?)Facing:/g)].map(x => x[1]), fat = s => { const x = /fatigue (\d+)/.exec(s); return x ? +x[1] : 0; };
        const F = st.map(fat);
        this.notes = ['fatigue at each status: ' + F.join(', ')];
        if (F.length !== 13) return ['status told ' + F.length + ' times: ' + o.slice(-800)];
        if (F[0] !== 0) f.push('rested at first, fatigue ' + F[0]);
        if (!(F[1] >= 1)) f.push('overloaded, 70 minutes standing: fatigue ' + F[1] + ', not tired by it');
        if (!(F[2] < F[1])) f.push('lying down 40 minutes, overloaded: fatigue ' + F[2] + ' from ' + F[1] + ', not rested');
        if (!(F[3] > F[2])) f.push('standing again half an hour: fatigue ' + F[3] + ' from ' + F[2]);
        if (!(F[4] < F[3])) f.push('sitting 40 minutes, overloaded: fatigue ' + F[4] + ' from ' + F[3] + ', not rested');
        if (!(F[5] > F[4])) f.push('standing again: fatigue ' + F[5] + ' from ' + F[4]);
        if (!(F[6] < F[5])) f.push('asleep 40 minutes, overloaded: fatigue ' + F[6] + ' from ' + F[5] + ', not rested');
        if (!/Position: lying down\./.test(st[6])) f.push('a sleeper not lying down: ' + st[6]);
        if (!(F[7] > F[6])) f.push('awake and standing again: fatigue ' + F[7] + ' from ' + F[6]);
        if (/burdened|overloaded/.test(st[8].replace(/a little burdened/, ''))) f.push('all let fall, still burdened: ' + st[8]);
        if (!(F[9] < F[8])) f.push('all let fall, forty minutes standing: fatigue ' + F[9] + ' from ' + F[8] + ', not rested');
        if (!/of it hunger and thirst, not rested off/.test(st[st.length - 1])) f.push('after long sleeps, the want\'s share not told: ' + st[st.length - 1]);
        if (!/Hunger and thirst: (hungry|very hungry|starving), (very thirsty|parched|dying of thirst)\./.test(st[st.length - 1])) f.push('after long sleeps, hunger and thirst not told: ' + st[st.length - 1]);
        if (!/Hunger and thirst: not hungry, not thirsty\./.test(st[0])) f.push('at first, hunger and thirst not told: ' + st[0]);
        return f;
      },
      sdcard: true
    },
  ],
};
