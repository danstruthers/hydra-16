// ****************************************************************************
// save.js - Dingus Quest Zero's tests: saving and the life between dungeons.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { card, typed } = require('../../lib/common.js');
const {
  xdBytes, saidTheme, DQ0_MAKE, dq0Life, dq0Turns, dq0Rep, dq0Need, dq0Carried, dq0Here, dq0Holder, dq0View, dq0TypedAsk,
  dq0Typed, dq0Game, dq0Scene, dq0SceneNow
} = require('../../lib/dq0.js');

// (A game with a save's folder: the rc variable dq0auto, so the time between two dungeons, dq0acct's, waits for nothing; dq0TypedAsk has it wait)
// (LINETIME=1: each line typed's cycles, Enter to the next prompt: a carry-on's start, the board's cost: told on stdout)
function dq0LineTime(m) {
  if (!process.env.LINETIME) return;
  const a = m.acia, read = a.read, shown = a.shown;
  let typed = '', t0 = 0, line = '', tail = '';
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) { line = typed; t0 = m.cpu.cyc; typed = ''; tail = ''; } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
  a.shown = (v, t) => {
    shown(v, t);
    tail = (tail + String.fromCharCode(v)).slice(-2);
    if (t0 && (tail === '> ' || tail === '% ')) { console.log('LINETIME ' + ((m.cpu.cyc - t0) / 1e6).toFixed(1) + 'M ' + line); t0 = 0; }
  };
}

module.exports = {
  tests: [
    {
      name: 'dq0save', meaning: true, what: 'Dingus Quest Zero kept (dq0 -d: the save on a card, here /ram), by the game\'s state: a thing put down, down a stairway, quit (kept as it ends: the world\'s save), the folder listed (the UI\'s game, the things, the world\'s change records, the denizens\', the maker\'s seen maps), then the game carried on: in the stratum below, where it stood and as it faced, seeing what it saw, carrying what it carried, each thing where it was (worn, held, in what)',
      // (850M, not 800M, 2026-10-29: two games begun, the referee's and the world's code the more for the shields and
      // the dens (some 4M more to load); it was at 799.4M; 940M, 2026-10-30: 851.0M with the answers' work, the budget
      // some 10% over it)
      init: 't_rc', cycles: 940e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/kept', 'drop the dagger',
        'd', 'quit', 'ls /ram/kept/d1', 'dq0 -d /ram/kept', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], dr = T.of('drop the dagger'), d = T.of('d'), q = T.and(T.of('quit')), c = T.of('dq0 -d /ram/kept');
        if (!dq0Here(dr.after).some(i => i.name === 'dagger')) f.push('the dagger not put down');
        dq0Need(f, d, 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        dq0Need(f, q, 'E_BYE', {}, 'the game not ended');
        if (!dq0Rep(q, 'wreq', { kind: 'WQ_SAVE' }).length) f.push('the game not kept as it ended (the world\'s WQ_SAVE)');
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '), ls = o.slice(o.indexOf('% ls /ram/kept/d1'), o.indexOf('% dq0 -d'));
        for (const n of ['changes.dat', 'dens.dat', 'game.dat', 'items.dat', 'seen.dat']) if (!ls.includes(n)) f.push('no ' + n + ' in the save folder');
        dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        const a = d.after, b = c.after;
        if (!a || !b || a.pc.sn !== 1 || b.pc.sn !== a.pc.sn || b.pc.x !== a.pc.x || b.pc.y !== a.pc.y || b.pc.face !== a.pc.face)
          f.push('not carried on where it stood, as it faced: ' + JSON.stringify(b && b.pc) + ', kept ' + JSON.stringify(a && a.pc));
        if (dq0View(b) !== dq0View(a)) f.push('not seeing what it saw: ' + dq0View(b) + ', kept ' + dq0View(a));
        const kit = s => dq0Carried(s).map(i => i.name + '/' + (dq0Holder(s, i) || { name: '' }).name + '/' + (i.flags & 7)).sort().join(', ');
        if (!kit(b) || kit(b) !== kit(a)) f.push('not carrying what it carried: ' + kit(b) + ', kept ' + kit(a));
        if (/dagger\//.test(kit(b))) f.push('the dagger carried, put down before');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0carry', meaning: true, outfit: true, near: ['save'], what: 'Dingus Quest Zero\'s character carrying on (slice 4.4: D8, the character carries on into a new dungeon; HOUSE.md\'s the character carries on), by the world\'s events and the words told: the crypt (seed 3, dq0 -k: its puzzle\'s pieces and a pick to hand) escaped by its way out, the seal made and used (dq0theme\'s way): the ending\'s words say it carries on, and its sheet is told as the game ends; its sheet kept (pc.hm, a second "body" line: its skills used since their development rolls); then the game begun again with the same folder and no character file: a new dungeon (seed 259, the drowned temple: begun, not carried on), the same character in it (its name, its skills as they were), carrying what it carried out (the world\'s kit.dat, its outfit: a pick it brought, not the hatchet it put down, its outfit not drawn anew; not the old theme\'s seal)',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone,pick -d /ram/q',
        'drop the hatchet', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u', 'cat /ram/q/pc.hm', 'dq0 -p t -s 259 -d /ram/q', 'sheet', 'inventory', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], theme = t => t.after ? t.after.theme : '?';
        const g2 = T.of('dq0 -p t -s 259 -d /ram/q');
        const l = T.of('u');
        dq0Need(f, l, 'E_LEFT', {}, 'the way out not taken (escaped)');
        const end = out.slice(out.indexOf('> u'), out.indexOf('% cat /ram/q/pc.hm'));
        if (!/Your character carries on/.test(end)) f.push('the ending\'s words don\'t say it carries on');
        const harn = !process.env.RULES || process.env.RULES === 'harn', sk = /Skills \(EML\):[^\n]*/.exec(end);   // (HarnMaster's sheet: the null ruleset's has no skills)
        if (harn && !sk) f.push('its sheet not told as the game ends');
        const pc = out.slice(out.indexOf('% cat /ram/q/pc.hm'), out.indexOf('% dq0 -p t -s 259'));
        if (harn && !/\nbody u\d+ /.test(pc)) f.push('its skills used not kept with its sheet (pc.hm\'s second body line)');
        dq0Need(f, g2, 'E_ARRIVE', { how: 'A_START' }, 'the second game not begun anew (a new dungeon)');
        const kit = dq0Carried(g2.after).map(i => i.name);
        if (!kit.includes('pick') || !kit.includes('backpack')) f.push('its things not carried on: ' + kit.join(', '));
        if (kit.includes('hatchet')) f.push('the hatchet it put down drawn anew, not its own things: ' + kit.join(', '));
        if (kit.includes('king\'s seal') || kit.includes('full chalice')) f.push('the old theme\'s own thing carried on: ' + kit.join(', '));
        if (theme(g2) !== 'the Drowned Temple') f.push('the new dungeon not seed 259\'s: ' + theme(g2));
        const who = s => (/\n([^\n,]+), [^\n,]+, enters the dungeon\./.exec(s) || [])[1];
        const n1 = who(out.slice(out.indexOf('% dq0 -p t -s 3'))), n2 = who(out.slice(out.indexOf('% dq0 -p t -s 259')));
        if (!n1 || n1 !== n2) f.push('not the same character: ' + n1 + ', ' + n2);
        const sk2 = /Skills \(EML\):[^\n]*/.exec(out.slice(out.indexOf('> sheet')));
        if (harn && (!sk || !sk2 || sk[0] !== sk2[0])) f.push('its skills not carried on: ' + (sk && sk[0]) + ' / ' + (sk2 && sk2[0]));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0life', outfit: true, meaning: true, near: ['save', 'gear', 'money'], what: "Dingus Quest Zero's record of a life (the plan for the game between dungeons, its step 3: life.h, dq0life; DUNGEON.md's a character's life, HOUSE.md's carrying on), by the files and the world's things: the crypt (seed 3) solved and walked out of after a long wait, in two folders: then each folder's next dungeon (no -s), the two the same dungeon, their records the same to the byte (the same seed and commands make the same life): the head's life seed, its dungeon 2, its date on past the days the first took, the first dungeon's seed, outcome and days; its second dungeon's seed under 65,536 (a stand-in's). Each dungeon's own folder, the sheet, the things and the record in the top one. Quit and begun again, the same dungeon going on, the record as it was. 'Made new' stops: a dulled blade stays dull and its life as it was unless the account (dq0acct, dq0auto's: its days on the date) says it's paid for, its version as it was; a torch's fuel as it was; the pennies what the account said, the money-changer's 15% taken. A seed over 65,536 goes on over it; and dq0life's date and age figures",
      init: 't_rc', cycles: 6000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone,sword -d /ram/a', 'look', 'wait 1500 minutes', 'combine the sunstone with the moonstone', 'inventory', 'use the seal on the way out', 'u',
        'dq0 -p t -d /ram/a', 'look', 'inventory', 'appraise the sword', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone,sword -d /ram/b', 'look', 'wait 1500 minutes', 'combine the sunstone with the moonstone', 'inventory', 'use the seal on the way out', 'u',
        'dq0 -p t -d /ram/b', 'look', 'inventory', 'appraise the sword', 'quit',
        'cmp /ram/a/life.dat /ram/b/life.dat', 'echo same $status', 'xd /ram/a/life.dat', 'ls /ram/a', 'ls /ram/a/d1', 'ls /ram/a/d2', 'cp /ram/a/life.dat /ram/l.dat',
        'dq0 -p t -d /ram/a', 'look', 'quit', 'cmp /ram/a/life.dat /ram/l.dat', 'echo kept $status',
        '/sd/0/games/lib/dq0/dq0life -d /ram/x -s 123456789', 'xd /ram/x/life.dat', 'cp /ram/a/d1/game.dat /ram/x/d1/game.dat', '/sd/0/games/lib/dq0/dq0life -d /ram/x', 'echo second', 'xd /ram/x/life.dat', 'cp /ram/x/life.dat /ram/y.dat', '/sd/0/games/lib/dq0/dq0life -d /ram/x', 'cmp /ram/x/life.dat /ram/y.dat', 'echo again $status', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {
        const sword = (i, h, L, g) => {                 // (The sword: its wear and life set, read back as the next dungeon's first looked at)
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), r = {};
          for (let k = 0; cat && k < L.nitems; k++) {
            const b = m.dq0.bank(L.item0 + Math.floor(k / per)), o = (k % per) * L.itemSize;
            if (!b || !b[o + L.iKind]) continue;
            let n = '';
            for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
            if (n === 'sword' && !r.sword) {
              if (i) { b[o + L.iWear] = 200; b[o + L.iDur] = 7; b[o + L.iDur + 1] = 0; b[o + L.iSize] = 2; }
              r.sword = { wear: b[o + L.iWear], dur: b[o + L.iDur] | b[o + L.iDur + 1] << 8, durMax: b[o + L.iDurMax] | b[o + L.iDurMax + 1] << 8, size: b[o + L.iSize] };
            }
            if (n === 'torch' && !r.torch) {
              if (i) { b[o + L.iFuel] = 33; b[o + L.iFuel + 1] = 0; }
              r.torch = b[o + L.iFuel] | b[o + L.iFuel + 1] << 8;
            }
          }
          m.lifeSeen.push({ g, ...r });
        };
        m.lifeSeen = [];
        dq0LineTime(m);
        dq0Scene(m, [1, 3].flatMap(g => [{ g, line: 'look', n: 1, fuel: { torch: 3000 } }, { g, line: 'use the seal on the way out', n: 1, call: (h, L) => sword(1, h, L, g) }]).concat(
          [2, 4].map(g => ({ g, line: 'look', n: 1, call: (h, L) => sword(0, h, L, g) }))));
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], g2 = T.of('dq0 -p t -d /ram/a'), g2b = T.of('dq0 -p t -d /ram/b');
        const a = xdBytes(out, 'xd /ram/a/life.dat'), la = dq0Life(a);
        const ao = out.indexOf('% dq0 -p t -d /ram/a'), acctText = out.slice(ao, out.indexOf('Enter to go on', ao)).replace(/\s+/g, ' ');
        if (a.length !== dq0Life.size()) f.push('the record not ' + dq0Life.size() + ' bytes: ' + a.length);
        if (!/\nsame\s*\n/.test(out.replace(/\r/g, ''))) f.push('the same seed and commands, the records not the same');
        if (!/\nkept\s*\n/.test(out.replace(/\r/g, ''))) f.push('quit and begun again, the record changed');
        if (la) {
          const { h, d } = la;
          if (!la.ok) f.push('the record\'s head: ' + a.slice(0, 4).toString('hex'));
          if (h.seed !== 3) f.push('the life\'s seed not 3: ' + h.seed);
          if (h.dungeon !== 2) f.push('not its second dungeon: ' + h.dungeon);
          const ds = h.dseed;
          if (ds >= 65536 || ds === 3) f.push('the second dungeon\'s seed: ' + ds);
          if (d[0].seed !== 3 || d[1].seed !== ds) f.push('the dungeons\' seeds: ' + d[0].seed + ', ' + d[1].seed);
          if (d[0].outcome !== dq0Life.C('LO_SOLVED') || d[1].outcome !== dq0Life.C('LO_NONE')) f.push('the outcomes: ' + d[0].outcome + ', ' + d[1].outcome + ' (solved, then none yet)');
          if (d[0].days !== 1) f.push('the first dungeon\'s days: ' + d[0].days + ' (a wait of 1500 minutes)');
          const d0 = 720 * 360 + 3, date = h.date, days = date - d0;   // (Seed 3: its hour 3, 25 hours on)
          const between = /(\d+) days? pass/.exec(acctText), told = between ? +between[1] : 0;   // (The time between's own, told: dq0acct's, HarnMaster's alone; another ruleset's, none)
          if (days !== 2 + told) f.push('the date not on two days (one crossed, the day after it came out) and the days told: ' + days + ', ' + told);
          const born = h.born;
          if (born > 720 - 16 || born < 720 - 25) f.push('its birth year: ' + born);
        }
        // (The folders)
        const ls = c => { const o = out.replace(/\r/g, ''), i = o.indexOf('% ' + c + '\n'); return i < 0 ? [] : o.slice(i + c.length + 3, o.indexOf('\n% ', i + 1)).split(/\s+/); };
        const top = ls('ls /ram/a'), d1 = ls('ls /ram/a/d1'), d2 = ls('ls /ram/a/d2');
        for (const n of ['life.dat', 'pc.hm', 'kit.dat', 'd1', 'd2']) if (!top.includes(n) && !top.includes(n + '/')) f.push('the top folder has no ' + n + ': ' + top);
        if (top.includes('game.dat')) f.push('a game.dat in the top folder');
        for (const [w, l] of [['d1', d1], ['d2', d2]]) for (const n of ['game.dat', 'world.dat', 'items.dat']) if (!l.includes(n)) f.push(w + ' has no ' + n + ': ' + l);
        // (The same dungeon, the second)
        dq0Need(f, g2, 'E_ARRIVE', { how: 'A_START' }, 'the second game not a new dungeon');
        const th = t => t.after ? t.after.theme : '?';
        if (th(g2) !== th(g2b)) f.push('the same life, two second dungeons: ' + th(g2) + ', ' + th(g2b));
        const look2 = g => dq0View(T.after(g, 'look').after);
        if (!look2(g2) || look2(g2) !== look2(g2b)) f.push('the same life, the second dungeons not seen the same: ' + look2(g2) + ' / ' + look2(g2b));
        dq0Need(f, T.after(g2, 'dq0 -p t -d /ram/a'), 'E_ARRIVE', { how: 'A_RESUMED' }, 'begun again, its second dungeon not going on');
        // (Made new stops)
        const seen = m.lifeSeen, was = seen.find(s => s.g === 1), now = seen.find(s => s.g === 2);
        if (!was || !now || !now.sword) f.push('the sword not seen: ' + JSON.stringify(seen));
        else {
          const mended = /your [^.(]*sword[^.(]* mended/.test(acctText), whetted = /your [^.(]*sword[^.(]* whetted/.test(acctText);   // (Paid for, as told; else as it was)
          if (now.sword.wear !== (mended || whetted ? 0 : 200)) f.push('a dulled blade not dull in the next dungeon, or not whetted as told: wear ' + now.sword.wear + ': ' + acctText);
          if (now.sword.dur !== (mended ? was.sword.durMax : 7) || now.sword.durMax !== was.sword.durMax) f.push('its life made new, or not mended as told: ' + JSON.stringify(now.sword));
          if (now.sword.size !== 2) f.push('its version not as it was: ' + now.sword.size);
          if (!(now.torch <= 33)) f.push('a torch\'s fuel renewed: ' + now.torch);
        }
        const pennies = t => dq0Carried(t && t.after).filter(i => i.name === 'pennies').reduce((n, i) => n + i.qty, 0);
        const i1 = T.of('inventory'), p1 = pennies(i1), p2 = pennies(T.after(i1, 'inventory'));
        const changed = /\((\d+) penn(?:y|ies)[^)]*once it's changed/.exec(acctText), have = /You have (\d+) penn/.exec(acctText);   // (What the account said it'd come to: its 15% each pile)
        if (!have ? !p1 || Math.abs(p2 - p1 * 0.85) > 2 : (changed ? +changed[1] : +have[1]) !== p2) f.push('the pennies not as the account said, at the 15%: ' + p1 + ' then ' + p2 + ': ' + acctText);
        // (A seed over 65,536)
        const x1 = xdBytes(out, 'xd /ram/x/life.dat'), l1 = dq0Life(x1);
        if (l1 && (l1.h.seed !== 123456789 || l1.h.dseed !== 123456789)) f.push('a new life\'s seed: ' + l1.h.seed + ', ' + l1.h.dseed);
        const x2 = xdBytes(out.slice(out.indexOf('% echo second')), 'xd /ram/x/life.dat'), l2 = dq0Life(x2);
        if (!/\nagain\s*\n/.test(out.replace(/\r/g, ''))) f.push('a dungeon past the first with no game kept: its record not left as it is');
        if (x1.length !== dq0Life.size() || x2.length !== dq0Life.size()) f.push('the record, run alone: ' + x1.length + ', ' + x2.length + ' bytes');
        if (l2 && (l2.h.dungeon !== 2 || l2.h.dseed < 65536 || l2.h.dseed === 123456789 || l2.h.seed !== 123456789)) f.push('a seed over 65,536, its second dungeon: ' + l2.h.dseed);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0account', outfit: true, near: ['save', 'gear', 'money', 'body'], what: "Dingus Quest Zero's time between two dungeons (the plan's steps 4 to 6: dq0acct; DUNGEON.md's a character's life, HOUSE.md's between dungeons), by scenes on the saved files (a sheet and a kit from a game begun and quit; dq0life for the record, dq0acct run alone): a wounded character comes back as the rolls say (a physician paid, nothing worse than minor left, the days on the record's date, fatigue clear); one dies of an infection (status 2, the record says so); a 68-day gap gives six rolls and a weapon stops at 70, the filled-in answer taken by Enter, three named by the player; a rich account (mended, whetted, restocked from what it set out with) and a broke one (spare goods sold, what it named never, rough, work with the days added and its bonus practice, no debt); the same seed the same account",
      init: 't_rc', cycles: 4000e6,
      machine: { input: (() => {
        const L = '/sd/0/games/lib/dq0';
        const begin = (d, seed, from) => [L + '/dq0life -d /ram/' + d + ' -s ' + seed, 'cp /ram/' + from + '/kit.dat /ram/' + d + '/kit.dat', 'cp /ram/' + from + '/kit0.dat /ram/' + d + '/kit0.dat', 'cp /ram/' + from + '/pc.hm /ram/' + d + '/pc.hm'];
        const acct = d => '/sd/0/games/lib/acct/dq0acct -d /ram/' + d + ' -l ' + L;
        const add = (d, ...l) => l.map(x => "echo '" + x + "' >> /ram/" + d + '/pc.hm');
        return dq0TypedAsk(['bind -a /sd/0/games /bin', 'dq0auto=1', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', "echo 'wealth 3000' >> /ram/c.hm", "echo 'ml swd 69' >> /ram/c.hm",
          'dq0 -p t -s 3 -c /ram/c.hm -k sword -d /ram/a', 'drop the flask of oil', 'quit',
          'hmchar -q -s 5 -u feudal -c hunter -o /ram/p.hm', 'dq0 -p t -s 3 -c /ram/p.hm -d /ram/p', 'name the rope Bob', 'quit',
          'echo =WOUND', ...begin('w', 100003, 'a'), ...add('w', 'inj 6 1 3 5 0 0', 'inj 13 2 2 3 0 0', 'dealt 00'), acct('w'), 'echo st $status', 'cat /ram/w/pc.hm', 'xd /ram/w/life.dat',
          'echo =WOUND2', ...begin('v', 100003, 'a'), ...add('v', 'inj 6 1 3 5 0 0', 'inj 13 2 2 3 0 0', 'dealt 00'), acct('v'), 'cmp /ram/v/pc.hm /ram/w/pc.hm', 'echo same $status',
          'echo =DIES', ...begin('d', 100001, 'a'), ...add('d', 'inj 6 1 3 5 1 2', 'dealt 0'), acct('d'), 'echo st $status', 'xd /ram/d/life.dat',
          'echo =RICH', ...begin('r', 100077, 'a'), acct('r'), 'echo st $status', 'xd /ram/r/kit.dat',
          'echo =BROKE', ...begin('b', 100021, 'p'), 'mkdir /ram/b/d0', 'cp /ram/p/d1/game.dat /ram/b/d0/game.dat', acct('b'), 'echo st $status',
          'echo =ASK', 'dq0auto=()', ...begin('q', 100077, 'a'), acct('q'), '', '', '', 'cat /ram/q/pc.hm',
          'echo =NAMED', ...begin('n', 100077, 'a'), acct('n'), '', 'swimming', 'sword', '', 'cat /ram/n/pc.hm', 'echo =END', 'echo $status']);
      })() },
      expect: ['% echo $status'],
      start(m) {                                        // (The sword the rich scene has mended: half its life gone, dulled)
        dq0LineTime(m);
        dq0Scene(m, [{ g: 1, line: 'drop the flask of oil', n: 1, call: (h, L) => {
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);
          for (let k = 0; cat && k < L.nitems; k++) {
            const b = m.dq0.bank(L.item0 + Math.floor(k / per)), o = (k % per) * L.itemSize;
            if (!b || !b[o + L.iKind]) continue;
            let n = '';
            for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
            if (n === 'sword') { const d = b[o + L.iDurMax] | b[o + L.iDurMax + 1] << 8; b[o + L.iWear] = 200; b[o + L.iDur] = (d >> 1) & 255; b[o + L.iDur + 1] = d >> 9; break; }
          }
        } }]);
      },
      check(m, out) {
        const f = [], o = out.replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const text = n => sect(n).replace(/\s+/g, ' '), days = n => { const r = /(\d+) days? pass/.exec(text(n)); return r ? +r[1] : -1; };
        const need = (n, re, w) => { if (!re.test(text(n))) f.push(w + ': ' + text(n).slice(0, 700)); };
        // A wounded character
        need('WOUND', /A physician sees to your left forearm and left knee \(12d\)/, 'no physician (a grievous wound 9d, a serious 3d)');
        need('WOUND', /The grievous cut to your left forearm/, 'the wound not told');
        const w = sect('WOUND'), inj = [...w.matchAll(/\ninj (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)/g)];
        for (const j of inj) if (+j[4] > 1) f.push('a wound worse than minor left: ' + j[0]);
        if (!/\nvol 0 /.test(w)) f.push('its fatigue not clear');
        if (!/\npace \d \d 0\n/.test(w)) f.push('its exertion not clear: ' + w.slice(0, 200));
        if (!/\nst\s*\n/.test(w)) f.push('its status not 0: ' + text('WOUND').slice(0, 800));
        const wr = xdBytes(sect('WOUND'), 'xd /ram/w/life.dat'), lw = dq0Life(wr);
        if (lw && days('WOUND') > 0) {
          const d0 = 720 * 360 + 100003 % 360;
          if (lw.h.date !== d0 + days('WOUND')) f.push('the date not on by the days: ' + lw.h.date + ', ' + d0 + ', ' + days('WOUND'));
          if (lw.h.flags & dq0Life.C('LF_BETWEEN')) f.push('the record left as between');
        } else f.push('the record: ' + wr.length + ' bytes, days ' + days('WOUND'));
        if (!/\nsame\s*\n/.test(sect('WOUND2'))) f.push('the same seed, not the same sheet after');
        // One dies of it
        need('DIES', /The infection kills you\. You die on \d+ \w+ \d+, at \d+\./, 'not told as it died');
        if (!/st 2/.test(sect('DIES'))) f.push('its status not 2: ' + text('DIES').slice(0, 300));
        const ld = dq0Life(xdBytes(sect('DIES'), 'xd /ram/d/life.dat')), over = dq0Life.C('LF_OVER') | dq0Life.C('LF_ABROAD');
        if (!ld || (ld.h.flags & over) !== over) f.push('the record not over: flags ' + (ld && ld.h.flags));
        // The rich: what it dropped bought again, no selling
        need('RICH', /You buy a flask of oil/, 'what it dropped (of what it set out with) not restocked');
        if (/You sell/.test(text('RICH'))) f.push('the rich sold things: ' + text('RICH').slice(0, 400));
        need('RICH', /You have your [^.(]*sword[^.(]* mended/, 'the sword not mended');
        const kit = xdBytes(sect('RICH'), 'xd /ram/r/kit.dat');
        let sw = null;
        for (let i = 0; i + 23 <= kit.length; i += 23) if (kit[i] === 25) sw = { wear: kit[i + 7], dur: kit[i + 13] | kit[i + 14] << 8, max: kit[i + 15] | kit[i + 16] << 8 };
        if (!sw || sw.wear !== 0 || !sw.max || sw.dur !== sw.max) f.push('the sword in the kit not made whole: ' + JSON.stringify(sw));
        // The broke: spare goods, never the named
        need('BROKE', /You sell /, 'nothing sold');
        need('BROKE', /you work as a Hunter\/Trapper, your keep found, for half a master's wage \(/, 'no work');
        need('BROKE', /The work teaches you:/, 'no bonus practice');
        if (/You sell[^.]*rope/.test(text('BROKE'))) f.push('what it named sold: ' + text('BROKE').slice(0, 500));
        if (/ -\d/.test(text('BROKE'))) f.push('a debt: ' + text('BROKE').slice(0, 500));
        // Practice
        need('ASK', /In the spare hours you could practise [A-Z]/, 'no question');
        need('ASK', /You practise for 2 months \(6 rolls\): /, 'not six rolls for 68 days');
        if (days('ASK') !== 68) f.push('not the 68 days: ' + days('ASK'));
        need('NAMED', /You can't practise swimming/, 'a skill not open taken');
        need('NAMED', /Sword 69 to 70[,.]/, 'Sword named: ' + text('NAMED').slice(-500));
        if (!/\nml swd 70\n/.test(sect('NAMED'))) f.push('a weapon past 70 by practice, or not there: ' + (/\nml swd \d+/.exec(sect('NAMED')) || [])[0]);
        if (/you (win|lose)|you've (won|lost)/i.test(o)) f.push('words of winning or losing');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0acctgone', outfit: true, near: ['save', 'gear', 'money'], what: "Dingus Quest Zero's time between two dungeons spending nothing on what isn't to hand (the user's, 2026-11-04: \"Skip it\"; HOUSE.md's between dungeons), by scenes on the saved files: a broke character with a spare sword half gone, the knife in its hand, and a quill: the sword sold, no work for the mending of what it sold; it can't practise the sword it sold, its filled-in answer leaving it out, named refused; the quill kept, its Script's (the plan's step 11: never sold unasked, what its practice needs), Script practised by name; one that set out with a bow and arrows buys its arrows back with the bow to shoot them, and none without",
      init: 't_rc', cycles: 2400e6,
      machine: { input: (() => {
        const L = '/sd/0/games/lib/dq0';
        const begin = (d, seed, from) => [L + '/dq0life -d /ram/' + d + ' -s ' + seed, 'cp /ram/' + from + '/kit.dat /ram/' + d + '/kit.dat', 'cp /ram/' + from + '/kit0.dat /ram/' + d + '/kit0.dat', 'cp /ram/' + from + '/pc.hm /ram/' + d + '/pc.hm'];
        return dq0TypedAsk(['bind -a /sd/0/games /bin', 'dq0auto=1', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', "echo 'ml swd 40' >> /ram/c.hm", "echo 'ml scr 40' >> /ram/c.hm",
          'hmchar -q -s 5 -u feudal -c hunter -o /ram/r.hm',
          'dq0 -p t -s 3 -c /ram/c.hm -k sword,quill -d /ram/a', 'put the sword in the pack', 'draw the knife', 'inventory', 'quit',
          'echo =SOLD', 'dq0auto=()', ...begin('z', 100077, 'a'), '/sd/0/games/lib/acct/dq0acct -d /ram/z -l ' + L, '', 'sword', 'script', '', '', '', 'echo =BOW', 'dq0auto=1',
          'dq0 -p t -s 3 -c /ram/r.hm -k shortbow,arrows -d /ram/b', 'quit', 'dq0 -p t -d /ram/b', 'drop the arrows', 'quit', ...begin('y', 100077, 'b'), '/sd/0/games/lib/acct/dq0acct -d /ram/y -l ' + L,
          'echo =NOBOW', 'dq0 -p t -s 3 -c /ram/r.hm -k shortbow,arrows -d /ram/n', 'quit', 'dq0 -p t -d /ram/n', 'drop the arrows', 'drop the shortbow', 'quit', ...begin('x', 100077, 'n'), '/sd/0/games/lib/acct/dq0acct -d /ram/x -l ' + L, 'echo =END', 'echo $status']);
      })() },
      expect: ['% echo $status'],
      start(m) {                                        // (The sword half gone, dulled: a mending that costs the whole price)
        dq0LineTime(m);
        dq0Scene(m, [{ g: 1, line: 'put the sword in the pack', n: 1, call: (h, L) => {
          const per = Math.floor(0x2000 / L.itemSize);
          const cat = m.dq0.bank(L.catBank);
          for (let k = 0; cat && k < L.nitems; k++) {
            const b = m.dq0.bank(L.item0 + Math.floor(k / per)), o = (k % per) * L.itemSize;
            if (!b || !b[o + L.iKind]) continue;
            let n = '';
            for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
            if (n === 'sword') { const d = b[o + L.iDurMax] | b[o + L.iDurMax + 1] << 8; b[o + L.iWear] = 200; b[o + L.iDur] = (d >> 1) & 255; b[o + L.iDur + 1] = d >> 9; break; }
          }
        } }]);
      },
      check(m, out) {
        const f = [], o = out.replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j).replace(/\s+/g, ' '); }, t = sect('SOLD');
        if (!/You sell [^.]*a sword/.test(t)) f.push('the scene not set: the sword not sold: ' + t.slice(0, 500));
        if (/you work as/.test(t) && !/You have (no money|\d farthings?)\b/.test(t)) f.push('work for the mending of the sword it sold, money over: ' + t.slice(0, 900));
        if (/For (\d\d) of the days you work/.test(t)) f.push('weeks of work for the mending of the sword it sold: ' + t.slice(0, 900));
        if (/mended/.test(t)) f.push('something mended: ' + t.slice(0, 500));
        if (!/You have nothing to practise sword with./.test(t)) f.push('the sword it sold named, not refused: ' + t.slice(0, 700));
        if (!/In the spare hours you could practise (?![^.]*Sword)[^.]*./.test(t)) f.push('its filled-in answer the sword it sold: ' + t.slice(0, 500));
        if (/You practise for [^.]*Sword/.test(t)) f.push('it practised the sword it sold: ' + t.slice(0, 700));
        if (/You sell [^.]*quill/.test(t)) f.push('the quill sold, its Script\'s writing thing: ' + t.slice(0, 500));
        if (/You have nothing to practise script with\./.test(t) || !/You practise for [^.]*Script/.test(t)) f.push('Script named, its quill kept, not practised: ' + t.slice(0, 700));
        const bow = sect('BOW'), nobow = sect('NOBOW');
        if (!/You buy [^.]*arrows/.test(bow)) f.push('the scene not set: arrows not bought with the bow kept: ' + bow.slice(-600));
        if (/arrows/.test(nobow.slice(nobow.indexOf('days pass')))) f.push('arrows bought with nothing to shoot them: ' + nobow.slice(-600));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0town', outfit: true, near: ['save', 'money', 'gear', 'theme'], what: "Dingus Quest Zero's town between two dungeons, a menu (the plan's step 11: games/dq0town, dq0acct's town.c; HOUSE.md's between dungeons, the town), by scenes: seed 70000 walked out solved, its knife half gone, carried on with the player asked (no dq0auto): the town's name the name maker's, a paragraph and the list, the temple shut (the scholar's house open: dq0scholar); the inn (a day; the rumours heard, Intrigue's one more the same themegen -r 4 makes; three days' rest), the date on by each errand's day; the market (three torches bought at their whole worth, the whetstone sold at half, its money told), the smith (the knife mended at the account's price), who's asking (a patron's offer taken, its rumour followed: quest.txt, the road's days), the account leaving alone what the player saw to, the torches in the next dungeon's pack; the same life again, the same town, left by a rumour named; a broke scribe's account (dq0auto) selling what's spare, never its quill or parchment, and practising its Script",
      init: 't_rc', cycles: 9000e6,
      machine: { input: (() => {
        const L = '/sd/0/games/lib/dq0';
        const seedOf = (s, n) => { let x = (s + Math.imul(n, 2654435761 | 0)) >>> 0; x = (x ^ (x >>> 15)) >>> 0; x = Math.imul(x, 40503) >>> 0; x = (x ^ (x >>> 13)) >>> 0; return n <= 1 ? s : s < 65536 ? x & 0xFFFF : (x | 0x10000) >>> 0; };
        const rs = ((seedOf(70000, 2) ^ 0x52554D52) >>> 0).toString();
        return dq0TypedAsk(['bind -a /sd/0/games /bin', 'dq0auto=1', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'themegen -q -r 4 -s ' + rs + ' -o /ram/q4',
          'echo =A', 'dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\' -d /ram/b', 'inventory', 'u', 'dq0auto=()',
          'dq0 -p t -d /ram/b', '1', '3', '', 'market', 'buy 3 torches', 'sell the whetstone', '', 'mend my knife', '', '4', '2', '', '', 'inventory', 'quit', 'dq0auto=1',
          'cat /ram/b/rumours.txt', 'cat /ram/q4/rumours.txt', 'cat /ram/b/d2/quest.txt', 'rm -r /ram/b', 'rm -r /ram/q4',
          'echo =B', 'dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\' -d /ram/e', 'u', 'dq0auto=()', 'dq0 -p t -d /ram/e', 'leave', 'viash', '', 'quit', 'dq0auto=1',
          'cat /ram/e/d2/quest.txt', 'rm -r /ram/e',
          'echo =SCRIBE', 'hmchar -q -s 5 -u feudal -c scribe -o /ram/s.hm', 'dq0 -p t -s 3 -c /ram/s.hm -k sword,quill,parchment -d /ram/a', 'quit',
          L + '/dq0life -d /ram/z -s 100077', 'cp /ram/a/kit.dat /ram/z/kit.dat', 'cp /ram/a/kit0.dat /ram/z/kit0.dat', 'cp /ram/a/pc.hm /ram/z/pc.hm',
          '/sd/0/games/lib/acct/dq0acct -d /ram/z -l ' + L, 'echo =END', 'echo $status']);
      })() },
      expect: ['% echo $status'],
      start(m) {                                        // (The knife half gone: the smith's to mend)
        dq0LineTime(m);
        dq0Scene(m, [{ g: 1, line: 'inventory', n: 1, call: (h, L) => {
          const per = Math.floor(0x2000 / L.itemSize), cat = m.dq0.bank(L.catBank);
          for (let k = 0; cat && k < L.nitems; k++) {
            const b = m.dq0.bank(L.item0 + Math.floor(k / per)), o = (k % per) * L.itemSize;
            if (!b || !b[o + L.iKind]) continue;
            let n = '';
            for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
            if (n === 'knife') { const d = b[o + L.iDurMax] | b[o + L.iDurMax + 1] << 8; b[o + L.iDur] = (d >> 1) & 255; b[o + L.iDur + 1] = d >> 9; break; }
          }
        } }]);
      },
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), f = [], words = s => s.replace(/\s+/g, ' ').trim();
        const sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : words(o.slice(i, j < 0 ? o.length : j)); };
        const lines = text => DQ0_MAKE().lines(text, 'theme');
        const blocks = text => { const r = []; for (const l of lines(text)) { if (l.key === 'rumour') r.push({ said: l.v[0] }); else if (r.length) r[r.length - 1][l.key] = l.v; } return r; };
        const d = s => { const x = /It is (\d+) (\w+) (\d+)\./.exec(s); return x ? +x[3] * 360 + ['Nuzyael', 'Peonu', 'Kelen', 'Nolus', 'Larane', 'Agrazhar', 'Azura', 'Halane', 'Savor', 'Ilvin', 'Navek', 'Morgat'].indexOf(x[2]) * 30 + +x[1] - 1 : -1; };
        const money = s => { const x = /You have (\d+)d(?: (\d)f)?;/.exec(s); return x ? +x[1] * 4 + +(x[2] || 0) : -1; };
        const A = sect('A'), town = A.slice(A.indexOf('% dq0 -p t -d /ram/b'));
        // The town: a name, a paragraph, the list, what's shut
        const head = /You walk to (\w+)\. .*?\1, (a [^.]+\.) ([^.]+\.)/.exec(town);
        if (!head) return f.concat(['no town: ' + town.slice(0, 800)]);
        for (const p of ['1 The inn: rest; hear what\'s said', '2 The market: buy, sell, haggle', '3 The smith: mend, whet, a new blade', '4 Who\'s asking: patrons and what they offer', '5 The scholar\'s house: a writing read; the library; a master', '6 Leave for: the places you\'ve heard of', 'The temple is shut to you for now.'])
          if (!town.includes(p)) f.push('the list without "' + p + '"');
        // The days: each errand's day on the date, the rest's three
        const at = [...town.matchAll(/It is \d+ \w+ \d+\. You have [^;]+; your keep so far comes to [^.]+\./g)].map(x => x[0]);
        if (at.length < 4) return f.concat(['the street not come back to four times: ' + at.length]);
        if (d(at[1]) - d(at[0]) !== 4) f.push('the inn and three days\' rest not four days on: ' + at[0] + ' / ' + at[1]);
        if (d(at[2]) - d(at[1]) !== 1 || d(at[3]) - d(at[2]) !== 1) f.push('the market\'s and the smith\'s not a day each: ' + at.slice(1, 4).join(' / '));
        if (!/You rest 3 days at the [A-Z]/.test(town)) f.push('no rest');
        if (!/Your time in \w+ comes to 7 days\./.test(town)) f.push('the town\'s days not told as 7: ' + (/Your time in [^.]+\./.exec(town) || [''])[0]);
        // The inn: the rumours, Intrigue's one more the same as themegen -r 4's
        const R = blocks(saidTheme(out, 'cat /ram/b/rumours.txt')), Q4 = blocks(saidTheme(out, 'cat /ram/q4/rumours.txt'));
        if (!/There's talk of other places: 1\. /.test(town)) f.push('no talk at the inn');
        if (/one more place/.test(town)) {
          if (R.length !== 4 || Q4.length !== 4 || JSON.stringify(R[3]) !== JSON.stringify(Q4[3])) f.push('Intrigue\'s rumour not themegen -r 4\'s fourth: ' + JSON.stringify(R[3]) + ' / ' + JSON.stringify(Q4[3]));
          if (JSON.stringify(R.slice(0, 3)) !== JSON.stringify(Q4.slice(0, 3))) f.push('the first three not -r 4\'s');
        } else if (!/You hear nothing more worth the hearing/.test(town)) f.push('Intrigue\'s listening not told');
        // The market: bought at the whole worth, sold at half, the money told
        const torch = /a torch (\d+)d/.exec(town), whet = /a whetstone (\d+)d/.exec(town);
        if (!torch || !town.includes('You buy 3 torches (' + 3 * torch[1] + 'd)')) f.push('three torches not bought at their worth: ' + town.slice(town.indexOf('For sale'), town.indexOf('For sale') + 300));
        if (!whet || !town.includes('You sell your whetstone (' + Math.floor(whet[1] / 2) + 'd)')) f.push('the whetstone not sold at half: ' + (/You sell[^.]*\./.exec(town) || [''])[0]);
        if (torch && whet && money(at[1]) - money(at[2]) !== 4 * (3 * torch[1] - Math.floor(whet[1] / 2))) f.push('the money not on by the market\'s: ' + at[1] + ' / ' + at[2]);
        // The smith: the knife mended (the account's price: a tenth of its worth a tenth of its life, at most half: its worth then)
        const mend = /Your knife wants mending, (\d+)d/.exec(town), mended = /The smith mends your knife \((\d+)d\)\./.exec(town);
        if (!mend || !mended || mend[1] !== mended[1]) f.push('the knife not mended at its price: ' + town.slice(town.indexOf('forge') - 200, town.indexOf('forge') + 300));
        if (mended && money(at[2]) - money(at[3]) !== 4 * mended[1]) f.push('the mending not paid: ' + at[2] + ' / ' + at[3]);
        // Who's asking: the patron's offer taken, its rumour followed, the road's days
        const r2 = R[1];
        if (!r2 || !r2.why || r2.why[1] === 'found') return f.concat(['the scene not set: rumour 2 not a patron\'s: ' + JSON.stringify(r2)]);
        if (!town.includes('(' + r2.why[2] + ' asks; ' + r2.region[0] + ',')) f.push('the patron not among who\'s asking: ' + r2.why[2]);
        if (!town.includes('You give ' + r2.why[2] + ' your word.')) f.push('the offer not taken');
        const q = blocks(saidTheme(out, 'cat /ram/b/d2/quest.txt'))[0];
        if (!q || q.said !== r2.said) f.push('quest.txt not the patron\'s rumour: ' + JSON.stringify(q));
        const road = /You set out for ([^,]+), (\d+) days on the road\./.exec(town);
        if (!road || road[1] !== r2.region[0]) f.push('not told setting out for the patron\'s place: ' + (road || [''])[0]);
        // The account after: nothing sold or bought for it (the market seen), nothing mended (the smith), the torches carried
        const after = town.slice(town.indexOf('Your time in'), town.indexOf('Enter to go on'));
        if (/You (sell|buy)|mended/.test(after)) f.push('the account did what the player saw to: ' + after.slice(0, 400));
        const inv = /> inventory You carry:? ([^>]*)/g, invs = [...o.replace(/\s+/g, ' ').matchAll(inv)].map(x => x[1]);
        const n = s => { const x = /(\w+) torches/.exec(s || ''); return x ? ['', 'one', 'two', 'three', 'four', 'five', 'six', 'seven', 'eight'].indexOf(x[1]) : 0; };
        if (invs.length < 2 || n(invs[1]) !== n(invs[0]) + 3) f.push('the torches bought not in the next dungeon\'s pack: ' + invs.slice(0, 2).join(' / '));
        // The same life again: the same town; left by a rumour named
        const B = sect('B'), tb = B.slice(B.indexOf('% dq0 -p t -d /ram/e'));
        if (!tb.includes(head[0].slice(head[0].indexOf(head[1] + ',')))) f.push('the same life\'s town not the same: ' + tb.slice(0, 300));
        const r3 = R.find(r => /viash/i.test(r.region[0])), qb = blocks(saidTheme(out, 'cat /ram/e/d2/quest.txt'))[0];
        if (!r3 || !qb || qb.said !== r3.said || !tb.includes('You set out for ' + r3.region[0] + ', ' + (2 + [...r3.region[0]].reduce((h, c) => (h * 31 + c.charCodeAt(0)) & 0xFFFF, 0) % 11) + ' days on the road.'))
          f.push('not left by the rumour named, its road: ' + JSON.stringify(qb) + ' ' + (/You set out[^.]*\./.exec(tb) || [''])[0]);
        // The broke scribe: its writing things kept
        const S = sect('SCRIBE');
        if (!/You sell /.test(S)) f.push('the scene not set: nothing sold: ' + S.slice(-900));
        if (/You sell [^.]*(quill|parchment)/.test(S)) f.push('the scribe\'s writing things sold: ' + (/You sell[^.]*\./.exec(S) || [''])[0]);
        if (!/You practise for [^.]*Script/.test(S)) f.push('the scribe not practising its Script: ' + S.slice(-600));
        if (/you (win|lose)|you've (won|lost)/i.test(o)) f.push('words of winning or losing');
        this.notes = [head[1] + ' ' + head[2] + '; ' + (/one more place/.test(town) ? 'Intrigue found one more' : 'Intrigue found nothing') + '; ' + (road || [''])[0]];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0scholar', outfit: true, near: ['save', 'money', 'body', 'theme'], what: "Dingus Quest Zero's scholar's house in the town between two dungeons (the plan's step 12: games/dq0schol, run by dq0town; HOUSE.md's the scholar's house), by scenes (the rc variable dq0sg setting a scribe's Morality and its grade, a week's, a skill's): a scribe who reads ostari and sunglyph carries the Cup of Rowon's journal out unread (seed 70000, its copy too), and in the town (the player asked) studies it in the library, a penny a day, a test a week, four failed weeks: a piece of it, heard.txt's rumour a piece's, the month's practice on sunglyph first; the journal then read by sages (its tongue's old form, 26 centuries: a sage's, three times a scribe's day as the day's sage of the town's asks it (price.h's: 10d here), only in a big town), each grade's reading told and none its grade (a success's place; a failure's as confident, its land right and another realm; a critical failure's another place and reason; a critical success's line more: whose hand, how old, the road), a better reading the clue's and heard.txt's rumour, a worse one undoing nothing; a second opinion asked at once, an unscrupulous one's half again, the copy it sold on talked of in the street at the next visit; a diabolical one's wrong reading told of the copy; Astrology asked once, its centuries the clue's; a small town with no sage for it, a master teaching only ardic; a soldier with no script taught ardic by a master in the town's real menu, a month, the day's master's price (price.h's: 83d here, the base 84d): its sheet's script ardic at 70 + its skill base, the month's practice forfeit; and the tongues' store (the referee's, 56 bytes till now): a sheet with eight tongues and scripts past its own read and written back whole, a ninth refused aloud, and a master refusing a ninth",
      init: 't_rc', cycles: 14000e6,
      machine: { input: (() => {
        const L = '/sd/0/games/lib/dq0', S = d => '/sd/0/games/lib/acct/dq0schol -d /ram/' + d + ' -l ' + L + ' -t Velmor';
        return dq0TypedAsk(['bind -a /sd/0/games /bin', 'dq0auto=1', 'hmchar -q -s 5 -u feudal -c scribe -o /ram/s.hm', 'cp /ram/s.hm /ram/q.hm',
          "echo 'language ostari=60' >> /ram/q.hm", "echo 'script sunglyph=60' >> /ram/q.hm", 'hmchar -q -s 5 -u feudal -c militia -o /ram/m.hm',
          'echo =LIB', "dq0 -p t -s 70000 -c /ram/q.hm -d /ram/a -k 'jade cup,journal,paper,charcoal'", 'copy the journal', 'u',
          'dq0auto=()', "dq0sg='1 1 1 1'", 'dq0 -p t -d /ram/a', '5', 'study 1', '', '', '', '', '', 'quit', 'dq0auto=1', 'cat /ram/a/heard.txt',
          'echo =G2', "dq0sg='12 2'", S('a') + ' -b 1 -c 4000 -w 1', 'read 1', '', 'cat /ram/a/heard.txt',
          'echo =G1', "dq0sg='12 1'", S('a') + ' -b 1 -c 4000 -w 1', 'read 1', '', 'cat /ram/a/heard.txt',
          'echo =G0', "dq0sg='12 0'", S('a') + ' -b 1 -c 4000 -w 1', 'read 1', '', 'cat /ram/a/heard.txt',
          'echo =G3', "dq0sg='12 3 6 2'", S('a') + ' -b 1 -c 4000 -w 1', 'read 1', 'read the journal', '', 'cat /ram/a/heard.txt',
          'echo =LIE', "dq0sg='4 3'", S('a') + ' -b 1 -c 4000 -w 16385', 'read 1', '', 'cat /ram/a/heard.txt',
          'echo =STARS', "dq0sg='2'", S('a') + ' -b 1 -c 4000 -w 1', 'astrology 1', 'astrology the journal', '',
          'echo =SMALL', 'dq0sg=()', S('a') + ' -b 0 -c 4000 -w 1', 'read 1', 'learn starric', '',
          'echo =TEACH', "dq0 -p t -s 70000 -c /ram/m.hm -d /ram/b -k 'jade cup'", 'u', 'dq0auto=()', 'dq0 -p t -d /ram/b', 'learn ardic', '', '', '', '', 'quit',
          'dq0auto=1', 'cat /ram/b/pc.hm', 'cp /ram/a/life.dat /ram/life.dat', 'rm -r /ram/a', 'rm -r /ram/b',
          'echo =STORE', 'cp /ram/s.hm /ram/t8.hm', "echo 'language sindarin=50' >> /ram/t8.hm", "echo 'script starric=100' >> /ram/t8.hm",
          "echo 'script sunglyph=100' >> /ram/t8.hm", "echo 'script clawmark=100' >> /ram/t8.hm", 'cp /ram/t8.hm /ram/t9.hm', "echo 'language kobold=40' >> /ram/t9.hm",
          'dq0 -p t -s 3 -c /ram/t8.hm -d /ram/c', 'sheet', 'quit', 'cat /ram/c/pc.hm', 'echo =NINE', 'dq0 -p t -s 3 -c /ram/t9.hm', 'echo =FULL', 'mkdir /ram/t', 'cp /ram/life.dat /ram/t/life.dat',
          'cp /ram/t8.hm /ram/t/pc.hm', S('t') + ' -b 1 -c 4000', 'learn kobold', '', 'echo =END', 'echo $status']);
      })() },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), f = [], words = s => s.replace(/\s+/g, ' ').trim();
        const sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : words(o.slice(i, j < 0 ? o.length : j)); };
        const heard = n => (/rumour \| ([^%]*?) region \| ([^|]+) \|/.exec(sect(n).slice(sect(n).lastIndexOf('cat /ram/a/heard.txt'))) || []).slice(1).map(x => x.trim());
        const grade = /\b(success|failure|critical|succeed|fail)/i;
        // The library: four weeks, a piece; its rumour a piece's; the month's practice on sunglyph first
        const lib = sect('LIB');
        if (!/You study the journal in the library, a penny a day: after 4 weeks you've made out only a piece of it: The journal is a seeker's[^.]* \.\.\. The rest you can't make out, but it speaks of some place (in|on) the \w+\. That's 28 days, 28d\./.test(lib))
          f.push('the library\'s four weeks not a piece: ' + lib.slice(lib.indexOf('> study'), lib.indexOf('> study') + 600));
        const days = /Your time in \w+ comes to (\d+) days\./.exec(lib);
        if (!days || +days[1] !== 29) f.push('the town\'s days not the visit and four weeks: ' + (days || [''])[0]);
        if (!/In the spare hours you could practise sunglyph,/.test(lib)) f.push('the month\'s practice not on the script studied first: ' + lib.slice(lib.indexOf('spare hours') - 40, lib.indexOf('spare hours') + 200));
        const h0 = heard('LIB');
        if (!/^What you made out of the journal speaks of some place (in|on) the \w+\.$/.test(h0[0] || '')) f.push('heard.txt not the piece\'s rumour: ' + JSON.stringify(h0));
        // The grades, hidden: each a reading, none its grade
        const read = n => { const s = sect(n), i = s.indexOf('> read'); return s.slice(i, s.indexOf('>', i + 3) < 0 ? s.length : s.indexOf('>', i + 3)); };
        const place = n => (/It tells of (the \w+ \w+), where the Houshani ([a-z ]+)\./.exec(read(n)) || []).slice(1);
        const [p2, r2] = place('G2'), [p1, r1] = place('G1'), [p0, r0] = place('G0'), [p3, r3] = place('G3');
        if (!p2 || r2 !== 'laid others of their dead') f.push('a success not the place and the reason: ' + read('G2'));
        if (!p1 || p1 === p2 || p1.split(' ')[2] !== p2.split(' ')[2] || r1 !== r2) f.push('a failure not as confident, its land right and another realm: ' + read('G1'));
        if (!p0 || p0 === p2 || r0 === r2) f.push('a critical failure not another place and reason: ' + read('G0'));
        if (p3 !== p2 || !/says too that the hand is the Houshani's, of some (\d+) centuries ago, and the place it names lies some \d+ days' road from here\./.test(read('G3')))
          f.push('a critical success not its line more: ' + read('G3'));
        for (const n of ['G2', 'G1', 'G0', 'G3']) {
          if (!/^> read 1 \w+, a sage, keeps the journal \d days? and asks \d+d; then reads it to you: The journal is a seeker's/.test(read(n))) f.push(n + ': not a sage\'s reading: ' + read(n));
          if (grade.test(read(n)) || /can't make out/.test(read(n))) f.push(n + ': its grade told: ' + read(n));
        }
        // A better reading the clue's and its rumour's; a worse one undoing nothing
        const h2 = heard('G2'), h1 = heard('G1'), hz = heard('G0'), h3 = heard('G3');
        if (h2[0] !== 'The journal you had read tells of ' + p2 + ', where the Houshani laid others of their dead.' || h2[1] !== p2) f.push('heard.txt not the better reading\'s: ' + JSON.stringify(h2));
        if (JSON.stringify(h1) !== JSON.stringify(h2) || JSON.stringify(hz) !== JSON.stringify(h2) || JSON.stringify(h3) !== JSON.stringify(h2)) f.push('a worse reading undid a better: ' + JSON.stringify([h1, hz, h3]));
        // A second opinion at once: another sage, half again (unscrupulous); the copy it sold on talked of at the next visit
        const g3 = sect('G3'), asks = [...g3.matchAll(/(\w+), a sage, keeps the journal (\d) days? and asks (\d+)d/g)];
        if (asks.length !== 2 || asks[0][1] === asks[1][1]) f.push('no second opinion from another: ' + g3.slice(0, 900));
        else if (+asks[0][3] !== 10 * asks[0][2] || +asks[1][3] !== 15 * asks[1][2]) f.push('a sage\'s day not 10d (this town\'s, price.h\'s), an unscrupulous one\'s not 15d: ' + asks.map(x => x[0]).join(' / '));
        const lie = sect('LIE');
        if (!/Your writings: 1 a copy of the journal\. There's talk in the street of an old journal like yours: someone has been selling copies of it\./.test(lie)) f.push('the copy sold on not talked of: ' + lie.slice(0, 500));
        if (!lie.includes('It tells of ' + p0 + ', where the Houshani ' + r0 + '.')) f.push('a diabolical sage\'s reading not the wrong one: ' + lie.slice(lie.indexOf('> read'), lie.indexOf('> read') + 500));
        if (JSON.stringify(heard('LIE')) !== JSON.stringify(h2)) f.push('a lie undid a better reading: ' + JSON.stringify(heard('LIE')));
        // Astrology, once
        const st = sect('STARS'), cent = /of some (\d+) centuries ago/.exec(read('G3'));
        if (!cent || !st.includes('By the signs on it, it was made some ' + cent[1] + ' centuries ago.')) f.push('Astrology not the clue\'s age: ' + st.slice(0, 600));
        if (!st.includes('You\'ve asked that of the journal already.')) f.push('Astrology asked twice');
        // A small town: no sage for a dead tongue, its masters ardic's alone
        const sm = sect('SMALL');
        if (!sm.includes('No one in Velmor reads the old ostari of the journal: the sages of a bigger town might.')) f.push('a small town read a dead tongue: ' + sm.slice(0, 500));
        if (!sm.includes('No master in Velmor teaches that: a bigger town\'s might.')) f.push('a small town taught starric');
        // A master's month: ardic opened, the sheet's, no practice that month
        const t = sect('TEACH'), ml = /you can read and write it now \(ardic (\d+)\)/.exec(t);
        if (!/You study ardic for a month with Master \w+ \(83d\)/.test(t) || !ml) f.push('the soldier not taught ardic: ' + t.slice(t.indexOf('learn ardic'), t.indexOf('learn ardic') + 500));
        if (!ml || !new RegExp('script ardic=' + ml[1] + ' ').test(t.slice(t.lastIndexOf('cat /ram/b/pc.hm')))) f.push('ardic not on the sheet at its mastery: ' + t.slice(t.lastIndexOf('cat /ram/b/pc.hm')));
        if (ml && +ml[1] < 70) f.push('ardic under 70 + its skill base: ' + ml[1]);
        if (!/Your time in \w+ comes to 31 days\./.test(t) || /You practise for/.test(t)) f.push('the month with a master not the month\'s practice: ' + t.slice(t.indexOf('Your time'), t.indexOf('Your time') + 400));
        // The tongues' store: eight kept, a ninth refused aloud, a master refusing a ninth
        const sto = sect('STORE');
        if (!/Tongues: common \d+, khuzdul \d+, sindarin 50\. Scripts: ardic \d+, \w+ \d+, \w+ \d+, starric 100, sunglyph 100, clawmark 100\./.test(sto)) f.push('eight tongues and scripts not held: ' + sto.slice(sto.indexOf('Tongues:'), sto.indexOf('Tongues:') + 200));
        for (const l of ['language sindarin=50', 'script starric=100', 'script sunglyph=100', 'script clawmark=100'])
          if (!sto.slice(sto.lastIndexOf('cat /ram/c/pc.hm')).includes(l)) f.push('written back without ' + l);
        if (!sect('NINE').includes('dq0: too many tongues and scripts in its file')) f.push('a ninth not refused aloud: ' + sect('NINE').slice(0, 300));
        if (!sect('FULL').includes('Your sheet holds all the tongues and scripts the game can keep, eight past your own: you can learn no more.')) f.push('a master taught a ninth: ' + sect('FULL').slice(0, 600));
        if (/you (win|lose)|you've (won|lost)/i.test(o)) f.push('words of winning or losing');
        this.notes = [p2 + ' (' + p1 + ', ' + p0 + ')'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tongues', outfit: true, near: ['save', 'body'], what: "Dingus Quest Zero's tongues and scripts on the sheet (the plan for the game between dungeons, its step 8; HOUSE.md's characters, a character's tongues and scripts), by scenes: a scribe's sheet (hmchar's, seed 5, a Feudal one: the book's 2nd Language and Three Scripts, Character 15: a language line at its skill base once, its native script by its culture, ardic, and two more of the game's own, each at 70 + SB, none twice); a sage's (a language, two scripts); a soldier's (one tongue, no script: \"Scripts: none\") (its other makers': hmchartongues); the scribe's sheet read by the referee (its sheet in the game telling its tongues and scripts at their ML, and its age, the life's record's) and written back with them as the game's quit; the account reading it and writing it back, a script and the second tongue practised by name (named by the player: the book's development rolls, Skills 7; a script with something to write with)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: (() => {
        const L = '/sd/0/games/lib/dq0';
        return dq0TypedAsk(['bind -a /sd/0/games /bin', 'dq0auto=1',
          'echo =SCRIBE', 'hmchar -q -s 5 -u feudal -c scribe -o /ram/s.hm', 'cat /ram/s.hm',
          'echo =SAGE', 'hmchar -q -s 5 -u feudal -c sage -o /ram/g.hm', 'cat /ram/g.hm',
          'echo =SOLDIER', 'hmchar -q -s 5 -u feudal -c militia -o /ram/m.hm', 'cat /ram/m.hm',
          'echo =GAME', 'cp /ram/s.hm /ram/r.hm', "echo 'wealth 3000' >> /ram/r.hm", 'dq0 -p t -s 3 -c /ram/r.hm -k quill,parchment -d /ram/a', 'sheet', 'quit', 'cat /ram/a/pc.hm',
          'echo =ACCT', 'dq0auto=()', L + '/dq0life -d /ram/b -s 100077', 'cp /ram/a/kit.dat /ram/b/kit.dat', 'cp /ram/a/kit0.dat /ram/b/kit0.dat',
          'cp /ram/a/pc.hm /ram/b/pc.hm', '/sd/0/games/lib/acct/dq0acct -d /ram/b -l ' + L, '', 'delvic, khuzdul', '', '', 'cat /ram/b/pc.hm', 'echo =END', 'echo $status']);
      })() },
      expect: ['% echo $status'],
      start(m) { dq0LineTime(m); },
      check(m, out) {
        const f = [], o = out.replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const SCRIPTS = ['delvic', 'ardic', 'starric', 'hethric', 'clawmark', 'sunglyph'];
        const lines = (s, re) => [...s.matchAll(re)];
        const sheetOf = s => ({ lang: lines(s, /\nlanguage ([a-z]+)=(\d+)(?=\n)/g).map(x => [x[1], +x[2]]),
          scr: lines(s, /\nscript ([a-z]+)(?:=(\d+))?(?=\n)/g).map(x => [x[1], x[2] === undefined ? 0 : +x[2]]),
          ml: k => +((new RegExp('\nml ' + k + ' (\\d+)\n').exec(s) || [])[1] || 0) });
        // The scribe: a 2nd language, the native script (a Feudal's ardic) and two more, none twice, each at 70 + SB or more
        const s = sheetOf(sect('SCRIBE'));
        if (s.lang.length !== 1 || !['khuzdul', 'sindarin'].includes(s.lang[0][0])) f.push('the scribe\'s second tongue: ' + JSON.stringify(s.lang));
        if (s.scr.length !== 3 || s.scr[0][0] !== 'ardic' || s.scr[0][1] || new Set(s.scr.map(x => x[0])).size !== 3 ||
          s.scr.slice(1).some(x => !SCRIPTS.includes(x[0]) || x[1] < 70)) f.push('the scribe\'s scripts: ' + JSON.stringify(s.scr));
        if (!s.ml('scr')) f.push('the scribe\'s Script not open');
        const said = /Tongues: common (\d+), ([a-z]+) (\d+)\. Scripts: ardic (\d+), ([a-z]+) (\d+), ([a-z]+) (\d+)\./.exec(sect('SCRIBE'));
        if (!said || +said[1] !== s.ml('lan') || said[2] !== (s.lang[0] || [])[0] || +said[4] !== s.ml('scr') || said[5] !== (s.scr[1] || [])[0])
          f.push('the scribe\'s tongues and scripts said: ' + (said || [])[0]);
        const g = sheetOf(sect('SAGE'));
        if (g.lang.length !== 1 || g.scr.length !== 2 || g.scr[0][0] !== 'ardic') f.push('the sage\'s: ' + JSON.stringify(g));
        const so = sheetOf(sect('SOLDIER'));
        if (so.lang.length || so.scr.length || so.ml('scr')) f.push('the soldier\'s: ' + JSON.stringify(so));
        if (!/Tongues: common \d+\. Scripts: none\./.test(sect('SOLDIER'))) f.push('the soldier\'s not said: ' + sect('SOLDIER').slice(0, 300));
        // The referee's sheet, and the save written back
        const game = sect('GAME').replace(/\s+/g, ' ');
        const gs = /Tongues: common (\d+), ([a-z]+) (\d+)\. Scripts: ardic (\d+), ([a-z]+) (\d+), ([a-z]+) (\d+)\./.exec(game);
        if (!gs || s.scr.some((x, i) => i && (gs[3 + 2 * i] !== x[0] || +gs[4 + 2 * i] !== x[1])) || gs[2] !== (s.lang[0] || [])[0] || +gs[3] !== (s.lang[0] || [])[1])
          f.push('the sheet in the game: ' + (gs ? gs[0] : game.slice(game.indexOf('> sheet'), game.indexOf('> sheet') + 900)));
        const age = /born \d+ \w+ \([^)]*\), aged (\d+)\./.exec(game);
        if (!age || +age[1] < 15 || +age[1] > 26) f.push('its age not on the sheet: ' + (age || [])[0]);
        const a = sheetOf(sect('GAME'));
        if (JSON.stringify([a.lang, a.scr]) !== JSON.stringify([s.lang, s.scr])) f.push('the save\'s tongues and scripts: ' + JSON.stringify([a.lang, a.scr]));
        // The account: read back, two practised by name, written back
        const acct = sect('ACCT'), ac = acct.replace(/\s+/g, ' '), b = sheetOf(acct.slice(acct.lastIndexOf('cat /ram/b/pc.hm')));
        const pr = /You practise for \d+ months? \(\d+ rolls\): ([^.]*)\./.exec(ac);
        if (!pr) f.push('no practice: ' + ac.slice(0, 900));
        for (const [nm, list, was] of [['delvic', b.scr, a.scr], ['khuzdul', b.lang, a.lang]]) {
          const now = list.find(x => x[0] === nm), before = was.find(x => x[0] === nm), m2 = pr && new RegExp(nm + ' (\\d+) to (\\d+)|' + nm + ' no better').exec(pr[1]);
          if (!before) { f.push('the scene not set: the scribe has no ' + nm + ': ' + JSON.stringify(was)); continue; }
          if (!m2 || !now) f.push(nm + ' not practised: ' + (pr || [])[1] + ' ' + JSON.stringify(list));
          else if (m2[1] ? +m2[1] !== before[1] || +m2[2] !== now[1] : now[1] !== before[1]) f.push(nm + ' told ' + m2[0] + ', its line ' + JSON.stringify(now) + ' (was ' + before[1] + ')');
        }
        if (b.scr.length !== 3 || b.lang.length !== 1) f.push('the account\'s sheet: ' + JSON.stringify([b.lang, b.scr]));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0reading', outfit: true, near: ['save', 'body'], what: "Dingus Quest Zero's writings read, copied and carried out (the plan for the game between dungeons, its steps 9 and 10; HOUSE.md's reading and clues), by scenes: the crypt's stone tablet (the hill-kings', hethrin in hethric) at a soldier's feet (no script: it can't read), a scribe's (the script, not the tongue), a hill-man's (the tongue, not the script); one with both (the referee's test at their masteries' average and the condition, the book's Skills 11 and Treasure 8) in four games, each grade's words by its dice (a success the words, the place and the reason; a failure half of it, the land, the condition told; a critical success a line more, what the puzzle wants; a critical failure the words' names swapped about and another place, told as confidently), read again the same, untested; a copy made on paper with charcoal (Drawing's test, its words), read, the tablet read after it the same, nothing left to copy onto, nothing copied in the dark; the soldier's puzzle done unread (the crown on the throne: no puzzle needs reading); the Cup of Rowon's journal (seed 70000, the Houshani's, ostari in sunglyph) read and copied and the dungeon left with them in a save's folder: dq0life telling what was read as the next begins, the clue's rumour (heard.txt: its place's region), the old writing and the copy read in the next dungeon by their clues",
      init: 't_rc', cycles: 6000e6,
      machine: { input: dq0TypedAsk(['bind -a /sd/0/games /bin', 'dq0auto=1',
        'hmchar -q -s 5 -u feudal -c militia -o /ram/m.hm', 'hmchar -q -s 5 -u feudal -c scribe -o /ram/s.hm',
        'cp /ram/m.hm /ram/t.hm', "echo 'language hethrin=60' >> /ram/t.hm", "echo 'script ardic' >> /ram/t.hm",
        'cp /ram/m.hm /ram/r.hm', "echo 'language hethrin=50' >> /ram/r.hm", "echo 'script hethric=50' >> /ram/r.hm",
        'cp /ram/m.hm /ram/r9.hm', "echo 'language hethrin=95' >> /ram/r9.hm", "echo 'script hethric=95' >> /ram/r9.hm",
        'echo =SOLDIER', 'dq0 -p t -s 3 -c /ram/m.hm -T -T', 'read the tablet', 'quit',
        'echo =SCRIBE', 'dq0 -p t -s 3 -c /ram/s.hm -T -T', 'read the tablet', 'quit',
        'echo =HILLMAN', 'dq0 -p t -s 3 -c /ram/t.hm -T -T', 'read the tablet', 'quit',
        ...[8, 12, 17, 21].flatMap(n => ['echo =G' + n, 'rm /ram/clues.dat', 'dq0 -p t -s ' + n + ' -c /ram/' + (n === 8 ? 'r9' : 'r') + '.hm -T -T', 'read the tablet', 'read the tablet', 'quit']),
        'echo =COPY', 'rm /ram/clues.dat', "dq0 -p t -s 3 -c /ram/r.hm -T -T -k 'paper,charcoal'", 'copy the tablet', 'read the copy', 'read the tablet', 'copy the tablet',
        'douse the torch', 'copy the tablet', 'quit',
        'echo =UNREAD', "dq0 -p t -s 3 -c /ram/m.hm -k 'iron crown,throne'", 'drop the throne', 'put the crown on the throne', 'quit',
        'echo =QUEST', 'cp /ram/m.hm /ram/q.hm', "echo 'language ostari=90' >> /ram/q.hm", "echo 'script sunglyph=90' >> /ram/q.hm",
        "dq0 -p t -s 70000 -c /ram/q.hm -d /ram/a -k 'journal,paper,charcoal'", 'read the journal', 'copy the journal', 'u', 'u',
        'echo =NEXT', 'dq0 -p t -c /ram/q.hm -d /ram/a', 'read the old writing', 'read the copy', 'quit',
        'echo =HEARD', 'cat /ram/a/heard.txt', 'echo =END', 'echo $status']) },
      start(m) {   // (Each grade's reading rigged, dice2: G8 a success, G12 a failure, G17 a critical success, G21 a critical failure
        //   (games 4 to 7), the first read's test alone, the second read's left to the dice (untested, the same); the copy's
        //   game's readings failures, as the dice had them, the tablet read after the copy the same)
        dq0Scene(m, [['pass', 4], ['fail', 5], ['well', 6], ['badly', 7]].map(([grade, g]) => ({ g, line: 'read the tablet', n: 1,
          rig: [{ kind: 'TEST', a: 'P_LORE', uses: 1, grade }] })).concat([{ g: 8, line: 'copy the tablet', n: 1, rig: [{ kind: 'TEST', a: 'P_LORE', grade: 'fail' }] }]));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '').replace(/\r/g, '');
        const sect = n => { const i = o.indexOf('% echo =' + n + '\n'), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j).replace(/\s+/g, ' '); };
        const has = (n, s, what) => { if (!(s instanceof RegExp ? s.test(sect(n)) : sect(n).includes(s))) f.push(what + ': ' + sect(n).slice(-600)); };
        const not = (n, s, what) => { if (s instanceof RegExp ? s.test(sect(n)) : sect(n).includes(s)) f.push(what + ': ' + sect(n).slice(-600)); };
        const TABLET = 'Here lies Aldric, king under the hill. His seal was broken at his death';
        // Who can't read it
        has('SOLDIER', "You can't read: the marks on the stone tablet mean nothing to you.", 'the soldier (no script) read it');
        has('SCRIBE', "You know the letters on the stone tablet, but the words they spell are in a tongue you don't speak.", 'the scribe (the script, not the tongue)');
        has('HILLMAN', "The stone tablet is written in letters you've never learnt.", 'the hill-man (the tongue, not the script)');
        for (const n of ['SOLDIER', 'SCRIBE', 'HILLMAN']) not(n, TABLET, n.toLowerCase() + ' read the words');
        // The four grades, each twice the same (read once)
        const twice = (n, w) => { if (sect('G' + n).split(w).length < 3) f.push('G' + n + ' read again not the same: ' + sect('G' + n).slice(-900)); };
        has('G8', new RegExp(TABLET.replace(/[.]/g, '\\.') + '[^>]*It tells of the \\w+ \\w+, where the hill-kings laid others of their dead\\.'), 'a success: the words, the place and the reason');
        not('G8', 'smaller hand', 'a success told a line more');
        has('G12', 'The marks are worn almost away.', 'the condition not told');
        has('G12', /the sun's \.\.\. The rest you can't make out, but it speaks of some place (in|on) the \w+\./, 'a failure: half of it and the land');
        not('G12', 'It tells of', 'a failure told the place');
        has('G17', /It tells of the \w+ \w+, where the hill-kings laid others of their dead\. Under it, in a smaller hand, a line most would miss: the king's seal is what matters here\./, 'a critical success: the line more');
        has('G21', /It tells of the \w+ \w+, where the hill-kings built another town\./, 'a critical failure: another place, another reason');
        not('G21', TABLET, 'a critical failure: the words as they are');
        has('G21', 'Here lies Aldric', 'a critical failure: not the tablet\'s words at all');
        twice(8, 'It tells of the');
        twice(12, "can't make out");
        twice(17, 'smaller hand');
        twice(21, 'built another town');
        // The copy
        has('COPY', /You copy the marks of the (stone )?tablet onto your sheet, line by line: (a fair copy, every mark as it stands|a good copy|a rough copy, some of its marks guessed at|a poor copy, much of it botched)\./, 'the copy not made');
        const cp = sect('COPY').split('> read the copy')[2] || '', tb = sect('COPY').split('> read the tablet')[2] || '';
        if (!cp.includes('Here lies Aldric') && !/can't make out/.test(cp)) f.push('the copy not read: ' + cp.slice(0, 400));
        const words = x => (/The tablet is cut[^>]*/.exec(x) || [''])[0].replace(/[\s\d]+$/, '');   // (Less the status line's figures after it)
        if (!words(cp) || words(tb) !== words(cp)) f.push('the tablet read after its copy not the same: ' + tb.slice(0, 400));
        has('COPY', 'You have nothing to copy it onto: a sheet, and charcoal, a pencil, or a quill and ink.', 'a second copy, no sheet left');
        has('COPY', "You can't see to copy anything.", 'copied in the dark');
        // A puzzle done unread
        has('UNREAD', 'You set the iron crown upon the throne.', 'the soldier\'s crown not set on the throne (a puzzle done unread)');
        // Carried out
        has('QUEST', 'The journal is a seeker\'s, Efost\'s', 'the journal not read by an ostari reader in sunglyph');
        has('QUEST', /You copy the marks of the journal onto your sheet/, 'the journal not copied');
        has('NEXT', /You came out (knowing what the journal says: it tells of the \w+ \w+, where (the Houshani|its makers) [a-z ]+\.|with a piece of what the journal says: some place (in|on) the \w+\.)/, 'dq0life not telling the clue carried out');
        has('NEXT', 'Efost', 'the old writing not read in the next dungeon');
        if ((sect('NEXT').match(/Efost/g) || []).length < 2) f.push('the copy not read in the next dungeon: ' + sect('NEXT').slice(-800));
        has('HEARD', /rumour \| (The journal you read tells of the \w+ \w+, where|What you made out of the journal speaks of some place) [^|]*region \| the \w+ \w+ \| (mountains|hills|forest|marsh|coast|plain) \|/, 'the clue\'s rumour (heard.txt)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0nosave', near: ['save', 'body'], what: "Dingus Quest Zero's words at a death telling the truth about the save (the user's, 2026-11-04: \"Change the words\", no save on arriving; HOUSE.md's death, and the autosave; dq0end's, by what's in the folder), a hunter (hmchar's, seed 5) dying of a grievous bleeding wound with nothing saved: in the first dungeon, nothing to go back to and a new game next (the life told as a life, not so far); in a later dungeon, that dungeon begun again by the same seed, the record not moved on; in hard mode, its own words; none of them saying a save's kept",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['cp /ram/c.hm /ram/q.hm', 'echo \'inj 8 0 3 4 0 1\' >> /ram/c.hm', 'echo \'vol 0 9 0 0 0\' >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -d /ram/e', 'wait 20 minutes', 'echo =NEXT', 'dq0 -p t -s 3 -c /ram/c.hm -m -d /ram/e', 'quit',
        'echo =HARD', 'dq0 -p t -s 3 -c /ram/c.hm -H -d /ram/h', 'wait 20 minutes',
        'echo =OUT', 'dq0 -p t -s 3 -c /ram/q.hm -k sunstone,moonstone -d /ram/q', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u',
        'dq0 -p t -d /ram/q', 'quit', 'rm /ram/q/d2/game.dat', 'echo \'inj 8 0 3 4 0 1\' >> /ram/q/pc.hm', 'echo \'vol 0 9 0 0 0\' >> /ram/q/pc.hm', 'xd /ram/q/life.dat',
        'echo =LATER', 'dq0 -m -p t -d /ram/q', 'wait 20 minutes', 'xd /ram/q/life.dat', 'cat /ram/q/pc.hm',
        'echo =AGAIN', 'dq0 -N -p t -d /ram/q', 'quit', 'xd /ram/q/life.dat', 'echo =END', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = out.replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const flat = t => t.replace(/\s+/g, ' ');
        const first = flat(o.slice(o.indexOf('% dq0 -p t -s 3 -c /ram/c.hm -d /ram/e'), o.indexOf('% echo =NEXT')));
        if (!/You are dead/.test(first)) f.push('not dead of its wound: ' + first.slice(-400));
        if (/last save is kept/.test(first)) f.push('the first dungeon: says a save is kept: ' + first.slice(-500));
        if (!/Nothing was saved before you died, so there's no save to go back to: start the game again with its folder, and a new character begins\./.test(first)) f.push('the first dungeon: not its words: ' + first.slice(-500));
        if (/The life so far/.test(first) || !/The life of Tordan, Hunter\/Trapper, born in the year \d+: No dungeon finished\./.test(first)) f.push('the first dungeon: not told as a life: ' + first.slice(-500));
        dq0Need(f, T.of('dq0 -p t -s 3 -c /ram/c.hm -m -d /ram/e'), 'E_ARRIVE', { how: 'A_START' }, 'the first dungeon: its next game not a new one');
        const hard = flat(sect('HARD'));
        if (!/You are dead/.test(hard)) f.push('hard mode: not dead of its wound');
        if (!/Hard mode: there's no save to go back to\./.test(hard)) f.push('hard mode: not its words: ' + hard.slice(-400));
        if (/last save is kept|Nothing was saved/.test(hard)) f.push('hard mode: another\'s words: ' + hard.slice(-400));
        const later = flat(sect('LATER'));
        if (!/You are dead/.test(later)) f.push('a later dungeon: not dead of its wound: ' + later.slice(-400));
        if (/last save is kept/.test(later)) f.push('a later dungeon: says a save is kept: ' + later.slice(-500));
        if (!/Nothing was saved in this dungeon before you died: start the game again with its folder to begin the dungeon again, as you came to it\./.test(later)) f.push('a later dungeon: not its words: ' + later.slice(-500));
        if (!/The life so far of [^:]*: .*solved/.test(later)) f.push('a later dungeon: not the life so far: ' + later.slice(-500));
        dq0Need(f, T.of('dq0 -N -p t -d /ram/q'), 'E_ARRIVE', { how: 'A_START' }, 'a later dungeon: not begun again');
        const rec = n => dq0Life(xdBytes(sect(n), 'xd /ram/q/life.dat')), head = l => l.h;
        const a = rec('OUT'), b = rec('LATER'), c = rec('AGAIN');
        if (!a || !b || !c) f.push('the records not read');
        else {
          if (head(a).dungeon !== 2 || head(c).dungeon !== 2) f.push('the record moved on: ' + head(a).dungeon + ', ' + head(c).dungeon);
          if (head(a).dseed !== head(b).dseed || head(a).dseed !== head(c).dseed) f.push('not the same dungeon\'s seed: ' + JSON.stringify([head(a), head(b), head(c)]));
        }
        if (/you (win|lose)/i.test(o)) f.push('words of winning or losing');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hardon', near: ['save', 'body'], what: "Dingus Quest Zero's hard mode going on with the life (the fight soak's find, 2026-11-04: it was lost at a carry-on, a death in the next dungeon going back to a save): the crypt solved and left in hard mode (dq0 -H), the next dungeon begun without saying so: the record says so (LF_HARD), and the new dungeon's save (its pace's top bit), so its death is hard mode's",
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -H -d /ram/q', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u',
        'dq0 -p t -d /ram/q', 'quit', 'echo =LIFE', 'xd /ram/q/life.dat', 'echo =GAME', 'xd /ram/q/d2/game.dat', 'echo =END', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = out.replace(/\r/g, ''), first = n => { const i = o.indexOf('% echo =' + n), r = /\n[0-9a-f]+ +((?:[0-9a-f]{2} ){4})/.exec(o.slice(i, i + 200)); return r ? r[1].trim().split(' ').map(x => parseInt(x, 16)) : []; };
        const life = dq0Life(xdBytes(out, 'xd /ram/q/life.dat'));
        if (!life || !(life.h.flags & dq0Life.C('LF_HARD'))) f.push('the record not hard mode\'s: ' + first('LIFE'));
        if (!(first('GAME')[3] & 0x80)) f.push('the next dungeon\'s save not hard mode\'s: ' + first('GAME'));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lifeend', outfit: true, near: ['save', 'body'], what: "Dingus Quest Zero's life's account (dq0end; the plan's section 1): the crypt solved and walked out of with a wound that's infected, a carry-on: the character dies between the dungeons, told, the life's account after it (solved), the game not begun, status 0; the record says it's over; the next start a new life, the old record kept in lives.dat; and hard mode's death recorded, the account told",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -d /ram/q', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u',
        "echo 'inj 6 1 3 5 1 2' >> /ram/q/pc.hm", "echo 'dealt 0' >> /ram/q/pc.hm", 'echo =OFF', 'dq0 -p t -d /ram/q', 'echo st $status', 'xd /ram/q/life.dat', 'echo =NEW',
        'dq0 -p t -s 3 -c /ram/c.hm -d /ram/q', 'quit', 'ls /ram/q', 'xd /ram/q/life.dat', 'wc /ram/q/lives.dat', 'echo =END', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = out.replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const off = sect('OFF'), tx = off.replace(/\s+/g, ' ');
        if (!/The infection kills you\./.test(tx)) f.push('not told as it died: ' + tx.slice(0, 500));
        if (!/The life of Tordan, Hunter\/Trapper, born in the year \d+:/.test(tx)) f.push('no life\'s account: ' + tx.slice(0, 500));
        if (!/: solved, \d+ days?/.test(tx)) f.push('the dungeon not told as solved: ' + tx.slice(0, 500));
        if (!/Dead between dungeons, on /.test(tx)) f.push('its end not told: ' + tx.slice(0, 500));
        if (/The dungeon is being made ready|enters the dungeon/.test(tx)) f.push('the game begun all the same');
        if (!/st\s*\n/.test(off)) f.push('dq0\'s status not 0');
        const a = dq0Life(xdBytes(off, 'xd /ram/q/life.dat')), over = dq0Life.C('LF_OVER') | dq0Life.C('LF_ABROAD');
        if (!a || (a.h.flags & over) !== over || a.d[0].outcome !== dq0Life.C('LO_SOLVED')) f.push('the record: flags ' + (a && a.h.flags) + ', outcome ' + (a && a.d[0].outcome));
        const n = sect('NEW'), bb = xdBytes(n, 'xd /ram/q/life.dat'), b = dq0Life(bb);
        if (!b || b.h.flags !== 0 || b.h.dungeon !== 1) f.push('not a new life: ' + bb.slice(0, 16).toString('hex'));
        if (!/lives\.dat/.test(n)) f.push('the life that was not kept in lives.dat');
        if (!n.includes(String(dq0Life.size()))) f.push('lives.dat not one record: ' + n.slice(-200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dressbetween', near: ['save', 'body'], what: "Dingus Quest Zero's dressings soiled by the days between dungeons (the user's, 2026-11-11, DECISIONS.md's \"Cloth, bandages and dressings\"): a serious stab to the right forearm, tended, dressed in clean linen (its file's \"dressed :igs\": grade 9, its cloth s), the sheet's \"dressed in linen, clean\"; the crypt solved and walked out of, the game kept (the referee's line written back with its cloth); then (dq0lifeend's way) an infected grievous stab to the left forearm added to the file, a carry-on: the days between (dq0acct's) soil the dressing as the dungeon's do (three grades a day on a serious wound) till the infection kills the character, and the file's line is written back soiled, its cloth kept (a wound the account heals, any it can, is gone by the next dungeon, ninety days at most, so its death is where its sheet is kept)",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo inj 134 2 2 2 5 4 >> /ram/c.hm', 'echo dressed :igs >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -d /ram/q', 'sheet', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u',
        'echo =KEPT', 'cat /ram/q/pc.hm', "echo 'inj 6 1 3 5 1 2' >> /ram/q/pc.hm", 'echo =OFF', 'dq0 -p t -d /ram/q', 'echo =END', 'cat /ram/q/pc.hm', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const first = o.slice(o.indexOf('% dq0 -p t -s 3'), o.indexOf('% echo =KEPT')).replace(/\s+/g, ' '), off = sect('OFF').replace(/\s+/g, ' ');
        if (!/right forearm S2 \(stab\), tended, dressed in linen, clean/.test(first)) f.push('the first sheet: ' + first.slice(first.indexOf('Injuries'), first.indexOf('Injuries') + 200));
        if (!/\ndressed :igs\n/.test(sect('KEPT'))) f.push('the line not written back with its cloth: ' + sect('KEPT').slice(0, 200));
        if (!/The infection kills you\./.test(off) || !/ days? pass/.test(off)) f.push('not dead of it between the dungeons: ' + off.slice(0, 400));
        if (!/\ndressed [B-K]igs\n/.test(sect('END'))) f.push('the line after the days between not soiled, its cloth kept: ' + sect('END').slice(sect('END').indexOf('\ndealt'), sect('END').indexOf('\ndealt') + 80));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0nullbetween', near: ['save', 'money'], what: "Dingus Quest Zero's time between two dungeons under the null ruleset (the user's, 2026-11-12, DECISIONS.md's \"dq0acct split\": the engine's account, its ruleset's hooks the null's, games/dq0null/acct; rsmeta.h's table its dq0nacct): seed 70000's artifact to hand (dq0 -r null -k), walked out solved; its sheet given three wounds; carried on: the days pass and are told (the road and the lull alone: nothing heals, no physician, no practice), the reward paid and the standing told, the lodging the middle station's at 2d a day or less, the money told, the rumours heard and the first followed; the record on to its second dungeon, its date on by the days told and the road, the time between done; the next dungeon begun under the null referee, its sheet the null's with its wounds as they were",
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0 -r null -p t -s 70000 -k \'jade cup\' -d /ram/n', 'look', 'u', 'xd /ram/n/life.dat', "echo 'wounds 3' >> /ram/n/pc.hm",
        'echo =CARRY', 'dq0 -r null -p t -d /ram/n', 'look', 'quit', 'echo =AFTER', 'xd /ram/n/life.dat', 'cat /ram/n/pc.hm', 'ls /ram/n/d2', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const carry = sect('CARRY'), acct = carry.slice(0, carry.indexOf('Enter to go on') < 0 ? carry.length : carry.indexOf('Enter to go on')).replace(/\s+/g, ' ');
        const days = /(\d+) days? pass\. You walk to [A-Z]/.exec(acct);
        if (!days) return ['no time between told: ' + acct.slice(0, 600)];
        if (/dq0n?acct: /.test(o)) f.push('the account failed: ' + acct.slice(0, 300));
        if (/physician|festers|heals|mends|You practise|could practise|The work teaches|You die/.test(acct)) f.push('the null ruleset\'s account healed, practised or killed: ' + acct.slice(0, 600));
        if (!/You (take a place in the common room at the [A-Z][a-z]+( [A-Z][a-z]+)* \(a bed and a hot meal and a cold: 2d a day|sleep in a hayloft and eat market bread \(1d a day)/.test(acct)) f.push('not the middle station\'s lodging: ' + acct.slice(0, 600));
        if (!/You have [^.]*\. It is \d+ [A-Z][a-z]+ \d+, and you are \d+\. [^(]*\(standing [1-9]\d*\)\./.test(acct)) f.push('the money, the date, the age or the standing not told: ' + acct.slice(0, 900));
        if (!/there's talk of other places:[^]*You set out for [^,]+, (\d+) days on the road\./.test(acct)) f.push('no rumour followed: ' + acct.slice(-500));
        const l1 = dq0Life(xdBytes(o.slice(0, o.indexOf('% echo =CARRY')), 'xd /ram/n/life.dat')), l2 = dq0Life(xdBytes(sect('AFTER'), 'xd /ram/n/life.dat'));
        const road = /You set out for [^,]+, (\d+) days on the road\./.exec(acct);
        if (!l1 || !l2) f.push('no record');
        else {
          if (l2.h.dungeon !== 2 || l2.d[0].outcome !== dq0Life.C('LO_SOLVED')) f.push('not on to its second dungeon, the first solved: ' + l2.h.dungeon + ', ' + l2.d[0].outcome);
          if (l2.h.flags & (dq0Life.C('LF_BETWEEN') | dq0Life.C('LF_OVER'))) f.push('the record\'s flags: ' + l2.h.flags);
          if (road && l2.h.date - l1.h.date < +days[1] + +road[1]) f.push('the date not on by the days told and the road: ' + l1.h.date + ' to ' + l2.h.date + ', ' + days[1] + ' and ' + road[1]);
          if (!(l2.h.reputation > 0)) f.push('no standing from the reward: ' + l2.h.reputation);
        }
        const T = dq0Turns(m);
        dq0Need(f, T.of('dq0 -r null -p t -d /ram/n'), 'E_ARRIVE', { how: 'A_START' }, 'the next dungeon not begun');
        const sheet = sect('AFTER');
        if (!/\nnull 1\n/.test(sheet) || !/\nwounds 3\n/.test(sheet)) f.push('the sheet not the null\'s, its wounds as they were: ' + sheet.slice(sheet.indexOf('null'), sheet.indexOf('null') + 160));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0nulltown', near: ['save', 'money'], what: "Dingus Quest Zero's town and scholar's house under the null ruleset (the user's, 2026-11-13, DECISIONS.md's \"The town and the scholar's house: the ruleset's hooks\": the engine's town and scholar's house, games/dq0/town, linked with the null's rules, games/dq0null/meta, as dq0ntown and dq0nschol, the null account's): seed 70000's artifact and its journal to hand (dq0 -r null -k), walked out solved, carried on with the player asked: its account runs its own town, the inn's listening and the market's haggling tested; the scholar's house its own (a folklore asked of the journal done by the character itself at its flat chance, for a penny: the null ruleset has no masteries, every skill its flat chance; a master's month opening ardic with no mastery, said so, and its sheet's line \"script ardic\", no figure); then the scholar's house run alone, a sage set to the same scene by dq0sg (HarnMaster's Morality 4, a success) under each ruleset: the null's sage honest (no Morality: its price a sage's, the journal read right), HarnMaster's diabolical (half again, a wrong reading told as confidently)",
      init: 't_rc', cycles: 6000e6,
      machine: { input: (() => {
        const L = '/sd/0/games/lib/dq0', S = p => '/sd/0/games/lib/acct/' + p + ' -d /ram/n -l ' + L + ' -t Velmor -b 1 -c 4000 -w 1';
        return dq0TypedAsk(['bind -a /sd/0/games /bin', 'dq0auto=1', "dq0 -r null -p t -s 70000 -k 'jade cup,journal' -d /ram/n", 'look', 'u', 'dq0auto=()',
          'echo =TOWN', 'dq0 -r null -p t -d /ram/n', '1', '', '2', 'haggle for a torch', '', 'learn ardic', 'folklore 1', '', '', '', '', 'quit', 'dq0auto=1',
          'echo =SHEET', 'cat /ram/n/pc.hm', 'rm /ram/n/between.dat',
          'echo =NULL', "dq0sg='4 2'", S('dq0nschol'), 'read 1', '', 'rm /ram/n/between.dat',
          'echo =HARN', "dq0sg='4 2'", S('dq0schol'), 'read 1', '', 'dq0sg=()', 'echo =END', 'echo $status']);
      })() },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j).replace(/\s+/g, ' '); };
        const town = sect('TOWN'), sheet = o.slice(o.indexOf('% echo =SHEET'), o.indexOf('% echo =NULL')), nul = sect('NULL'), harn = sect('HARN');
        if (/dq0n?(acct|town|schol|ntown|nschol): /.test(o)) f.push('a program failed: ' + (/dq0n?\w+: [^\n]*/.exec(o) || [''])[0]);
        if (!/Where to\?/.test(town)) return ['no town: ' + town.slice(0, 800)];
        if (!/You keep your ears open a while\.+ ?(Someone in a corner speaks of one more place|You hear nothing more worth the hearing)/.test(town)) f.push('no listening at the inn: ' + town.slice(town.indexOf('talk of'), town.indexOf('talk of') + 600));
        if (!/> haggle for a torch (The trader takes offence|The trader won't move on the price|You talk the price round a little|You talk the price round a good deal)/.test(town)) f.push('no haggling: ' + town.slice(town.indexOf('> haggle'), town.indexOf('> haggle') + 300));
        // The null's rules in its scholar's house: its own lore at its flat chance, a script opened with no mastery
        if (!/> folklore 1 You spend a day in the library over it \(1d\)\./.test(town)) f.push('the folklore not the character\'s own (the null\'s flat chance): ' + town.slice(town.indexOf('> folklore'), town.indexOf('> folklore') + 300));
        if (!/You study ardic for a month with Master [A-Z][a-z]+ \(83d\): you can read and write it now\./.test(town)) f.push('ardic not taught with no mastery: ' + town.slice(town.indexOf('> learn'), town.indexOf('> learn') + 400));
        if (!/\nscript ardic\n/.test(sheet) || /script ardic=/.test(sheet)) f.push('the sheet not given "script ardic", no figure: ' + sheet.slice(-300));
        // A sage set to Morality 4 and a success: the null's honest, HarnMaster's diabolical
        const sage = s => /a sage, keeps the journal (\d+) days? and asks (\d+)d(?: (\d)f)?; then reads it to you: .*? It tells of ([^,]+), where/.exec(s);
        const a = sage(nul), b = sage(harn);
        if (!a || !b) return f.concat(['no sage\'s reading: ' + nul.slice(0, 600) + ' / ' + harn.slice(0, 600)]);
        const fa = +a[2] * 4 + +(a[3] || 0), fb = +b[2] * 4 + +(b[3] || 0);
        if (a[1] !== b[1] || fa !== +a[1] * 10 * 4) f.push('the null\'s sage not a sage\'s price (this town\'s, price.h\'s: 10d a day): ' + a[0]);
        if (fb !== 60 * +b[1]) f.push('HarnMaster\'s diabolical sage not half again: ' + a[0] + ' / ' + b[0]);
        if (a[4] === b[4]) f.push('the null\'s honest reading the same place as HarnMaster\'s wrong one: ' + a[4]);
        this.notes = [a[0], b[0]];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fastkept', near: ['save', 'body'], what: "Dingus Quest Zero's vow of fasting kept (the user's, 2026-11-11, DECISIONS.md's \"Prayer, fasting, swimming and following\"): a vow made, 20 minutes waited, the game quit and carried on: eating still refused (\"You're fasting, with 24 hours of your vow to go\"); the crypt solved and walked out of, dq0life run with no time between told (no -l) and the next dungeon's seed 1 (its hour 1, before the first's 3): the record's fast (life.h's, the head's byte 26) its end after the new date's midnight, 15 to 17 steps of 12 minutes; the next dungeon begun, eating refused with 2 to 4 hours of the vow to go; \"stop\" ends it, eating then not refused",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -d /ram/v', 'fast', 'wait 20 minutes', 'quit', 'echo =KEPT',
        'dq0 -p t -d /ram/v', 'eat food', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u',
        '/sd/0/games/lib/dq0/dq0life -d /ram/v -s 1', 'echo =LIFE', 'xd /ram/v/life.dat', 'echo =NEXT',
        'dq0 -p t -d /ram/v', 'eat food', 'stop', 'eat food', 'quit', 'echo =END', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), sect = n => { const i = o.indexOf('% echo =' + n), j = o.indexOf('% echo =', i + 3); return i < 0 ? '' : o.slice(i, j < 0 ? o.length : j); };
        const kept = sect('KEPT').replace(/\s+/g, ' '), next = sect('NEXT').replace(/\s+/g, ' ');
        if (!/> eat food You're fasting, with 24 hours of your vow to go: you leave the food be\./.test(kept)) f.push('the vow not kept in the save: ' + kept.slice(kept.indexOf('> eat food'), kept.indexOf('> eat food') + 200));
        const a = dq0Life(xdBytes(sect('LIFE'), 'xd /ram/v/life.dat'));
        if (!a || a.h.dungeon !== 2 || a.h.fast < 15 || a.h.fast > 17) f.push('the record\'s fast: dungeon ' + (a && a.h.dungeon) + ', fast ' + (a && a.h.fast));
        const r = /> eat food You're fasting, with (\d+) hours? of your vow to go: you leave the food be\./.exec(next);
        if (!r || +r[1] < 2 || +r[1] > 4) f.push('the vow not carried into the next dungeon: ' + next.slice(next.indexOf('> eat food'), next.indexOf('> eat food') + 200));
        const e = next.slice(next.indexOf('> stop'));
        if (!/^> stop(?: > stop)? You break your fast: you may eat again\./.test(e) || /fasting/.test(e.slice(e.indexOf('> eat food')))) f.push('the vow carried in not ended by "stop": ' + e.slice(0, 300));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hard', near: ['save', 'body'], what: 'Dingus Quest Zero\'s death and its autosave (slice 4.4: D7, death permanent with autosaves before danger as a setting, off hard mode; HOUSE.md\'s the character carries on), by the world\'s events and the words told: a hunter (hmchar\'s, seed 5) given a grievous bleeding wound and its blood near spent (its file\'s lines added), begun by the stairway down (dq0 -T) and down it (kept as it comes into the stratum: the autosave), then a wait: dead of it, the ending\'s words saying its last save is kept, and the game begun again with the folder carried on from that save (resumed, not a new game); the same in hard mode (dq0 -H): no save to go back to, the game begun again a new one (its folder\'s save not taken up)',
      init: 't_rc', cycles: 1700e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['cat /ram/c.hm', 'echo \'inj 8 0 3 4 0 1\' >> /ram/c.hm', 'echo \'vol 0 9 0 0 0\' >> /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/d', 'd', 'wait 10 minutes', 'dq0 -p t -d /ram/d', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -H -d /ram/h', 'd', 'wait 10 minutes', 'dq0 -p t -s 3 -c /ram/c.hm -d /ram/h', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [];
        const o1 = out.slice(out.indexOf('% dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/d'), out.indexOf('% dq0 -p t -d /ram/d'));
        if (!/You are dead/.test(o1)) f.push('not dead of its wound');
        if (!/Your last save is kept: start the game again with its folder to go back to it\./.test(o1.replace(/\s+/g, ' '))) f.push('the ending\'s words don\'t say its last save is kept');
        dq0Need(f, T.of('dq0 -p t -d /ram/d'), 'E_ARRIVE', { how: 'A_RESUMED' }, 'not carried on from its last save');
        const o2 = out.slice(out.indexOf('% dq0 -p t -s 3 -c /ram/c.hm -T -H'), out.indexOf('% dq0 -p t -s 3 -c /ram/c.hm -d /ram/h'));
        if (!/You are dead/.test(o2)) f.push('not dead of its wound in hard mode');
        if (!/Hard mode: there's no save to go back to/.test(o2)) f.push('the ending\'s words don\'t say hard mode keeps no save');
        if (!/The life of [^:]*:[^]*died in it, \d+ days?[^]*Dead, on /.test(o2)) f.push('hard mode\'s death: no life\'s account: ' + o2.slice(-500));
        if (!/The life so far of [^]*Your last save is kept/.test(o1.replace(/\s+/g, ' '))) f.push('a death with a save: no life so far: ' + o1.slice(-500));
        dq0Need(f, T.of('dq0 -p t -s 3 -c /ram/c.hm -d /ram/h'), 'E_ARRIVE', { how: 'A_START' }, 'hard mode\'s death carried on from a save');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0savebytes', meaning: true, near: ['save', 'body'], what: 'Dingus Quest Zero\'s save byte for byte (ONE-PATH.md\'s stage 1, the character as actor 0: its record moved in the hub, the save\'s bytes as they were), over five seeds (dq0 -T, the turns pace): the character turned, a step, a jog, crouched, its torch put out and hidden, quit (the world\'s world.dat, the denizens\' dens.dat dumped), carried on and quit again (world.dat dumped again; the folder gone after, so the RAM disk keeps its room): the first world.dat the hub\'s state as it quit (its stratum, square, facing, position, hiding, pace and watchfulness), the game carried on from it, and written back the same (all but the time believed\'s next reckoning, made again as it goes on); the dumps the old build\'s and the new\'s alike (their transcripts compared); and the character\'s body actor 0, the hub\'s bodies[0] (its old names, pc_x ..., on its fields; the denizens\' entries the bodies\' from 1), in the dungeon (its state 1) as each game begins and is carried on',
      init: 't_rc', cycles: 6000e6,
      machine: { input: dq0Typed(dq0Game({ lines: [...[1, 2, 3, 6, 21].flatMap(s => [
        'dq0 -p t -s ' + s + ' -c /ram/c.hm -T -d /ram/k' + s, 'turn right', 'step forward', 'jog', 'crouch', 'douse the torch', 'hide', 'quit',
        'xd /ram/k' + s + '/d1/world.dat', 'xd /ram/k' + s + '/d1/dens.dat', 'cp /ram/k' + s + '/d1/world.dat /ram/k' + s + '/w1.dat',
        'dq0 -p t -d /ram/k' + s, 'quit', 'cp /ram/k' + s + '/d1/world.dat /ram/k' + s + '/w2.dat', 'xd /ram/k' + s + '/w2.dat',
        'cmp /ram/k' + s + '/w1.dat /ram/k' + s + '/w2.dat', 'echo same ' + s + ' $status', 'rm -r /ram/k' + s]), 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], H = require('../../../games/dq0/hubgen.js').load(), o = n => H.off('wsave', n), z = n => H.size('wsave', n);
        const num = (b, n) => { let v = 0; for (let i = z(n) - 1; i >= 0; i--) v = v * 256 + b[o(n) + i]; return v; };
        const KEPT = ['sn', 'x', 'y', 'face', 'pos', 'hid', 'gait', 'watch', 'skew', 'lost', 'hunger', 'thirst', 'meals', 'ropes'];
        const PC = { sn: 'sn', x: 'x', y: 'y', face: 'face', pos: 'pos', hid: 'hid', gait: 'gait', watch: 'watch', skew: 'skew', lost: 'lost' };
        const L = m.dq0.layout().L;                       // (Actor 0: the character's body the bodies' first, its old names on its fields,
        if (L.pcBody !== L.bodies || L.actor !== L.bodies + L.actorSize)   //   the denizens' entries the bodies' from 1)
          f.push('the bodies: actor 0 at ' + L.pcBody + ', the denizens at ' + L.actor + ', the table at ' + L.bodies);
        for (const [o, a] of [['pcSn', 'aSn'], ['pcX', 'aX'], ['pcY', 'aY'], ['pcFace', 'aFace'], ['pcPos', 'aPos'], ['gait', 'aGait'], ['pcReady', 'aReady']])
          if (L[o] !== L.pcBody + L[a]) f.push('the old name ' + o + ' at ' + L[o] + ', not actor 0\'s ' + a + ' at ' + (L.pcBody + L[a]));
        let from = 0;
        for (const s of [1, 2, 3, 6, 21]) {
          const w1 = xdBytes(out, 'xd /ram/k' + s + '/d1/world.dat'), w2 = xdBytes(out, 'xd /ram/k' + s + '/w2.dat');
          const q = T.of('quit', from), c = T.of('dq0 -p t -d /ram/k' + s, from), q2 = T.of('quit', c.k + 1);
          from = q2.k + 1;
          if (w1.length !== H.size('wsave') + H.size('wrec', 'strata')) { f.push(s + ': world.dat ' + w1.length + ' bytes, not ' + (H.size('wsave') + H.size('wrec', 'strata'))); continue; }
          if (num(w1, 'magic') !== H.C.WSAVE_MAGIC || num(w1, 'version') !== 4) f.push(s + ': world.dat\'s head ' + num(w1, 'magic') + ' ' + num(w1, 'version'));
          const pc = q.at && q.at.pc;                     // (The hub's, as the quit was typed: the character as it was kept)
          for (const [k, n] of Object.entries(PC)) if (!pc || num(w1, n) !== pc[k]) f.push(s + ': world.dat\'s ' + n + ' ' + num(w1, n) + ', the hub\'s ' + (pc && pc[k]));
          if (!pc || pc.state !== 1) f.push(s + ': the character\'s body, actor 0, not in the dungeon: its state ' + (pc && pc.state));
          dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, s + ': not carried on');
          const b = q2.at && q2.at.pc;                    // (The hub's as the game was carried on: the save read back)
          for (const [k, n] of Object.entries(PC)) if (!b || num(w1, n) !== b[k]) f.push(s + ': carried on, the hub\'s ' + k + ' ' + (b && b[k]) + ', world.dat\'s ' + num(w1, n));
          if (!b || b.state !== 1) f.push(s + ': carried on, the character\'s body, actor 0, not in the dungeon: its state ' + (b && b.state));
          for (const n of KEPT) if (num(w2, n) !== num(w1, n)) f.push(s + ': written back, world.dat\'s ' + n + ' ' + num(w2, n) + ', not ' + num(w1, n));
          if (!xdBytes(out, 'xd /ram/k' + s + '/d1/dens.dat').length) f.push(s + ': no dens.dat');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0saveout', near: ['save', 'fight'], what: "Dingus Quest Zero's shock rolls kept with the game (the user's K16, 2026-11-21: a game loaded mid black-out goes on where it was): at seed 3 the character knocked senseless (a scene's: the referee's sheet senseless, its fall now, its time out five minutes on, the world's out), quit as it lies there (the quit's save), carried on: the referee's fall and rolls (ko_at, ko_n) and its time out (pc_out) read back as they were kept (its file's body line), the world's character senseless still (the hub's pc_out); and an actor's kept by its sheet (its fall and rolls set as it quit: saved in the sheets past the denizens' records, KOSAVE) read back the same, its sheet's entry in the fb bank's KOS as RS_NEW_ACTOR finds it",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/o', 'turn left', 'quit', 'dq0 -p t -d /ram/o', 'save', 'look', 'quit', 'echo $status'] })) },
      start(m) {
        const lbl = () => fs.readFileSync(path.join(__dirname, '..', '..', '..', 'obj', 'games', 'dq0harn', 'dq0harn.lbl'), 'latin1');
        const ad = n => parseInt(new RegExp('^al ([0-9A-F]+) \\._' + n + '$', 'm').exec(lbl())[1], 16);
        const H = fs.readFileSync(path.join(__dirname, '..', '..', '..', 'sdk', 'c', 'include', 'harn.h'), 'latin1'), def = n => +new RegExp('#define\\s+' + n + '\\s+(\\d+)').exec(H)[1];
        const STATE = 2 * 24 + 10 + def('HARN_MEDMAX') + 2 * def('HARN_PSYMAX') + 1 + def('HARN_MAXSKILLS') + 3;
        const PCOUT = require('../../../games/dq0/hubgen.js').load().off('struct hub', 'pc_out');
        const KOS = n => 0x800 + 0x100 + 6 * n;   // (Sheet n's entry: dq0harn.h's KOS, by sheet)
        const ram = () => m.taskRam[m.dq0.bank(0)[12 + 3]], u32 = (r, a) => (r[a] | r[a + 1] << 8 | r[a + 2] << 16 | r[a + 3] << 24) >>> 0;
        const fbMem = () => { const r = ram(), t = m.dq0.bank(0)[12 + 3]; return m.taskBankMem(t, r[ad('fb')]); };
        m.kept = [];
        const read = () => { const r = ram(), fb = fbMem(); return { at: u32(r, ad('ko_at')), n: r[ad('ko_n')] | r[ad('ko_n') + 1] << 8, out: u32(r, ad('pc_out')),
          state: r[ad('pc') + STATE], hubOut: m.dq0.bank(0)[PCOUT], kos: fb ? Array.from(fb.subarray(KOS(m.kosN), KOS(m.kosN) + 6)) : null }; };
        dq0Scene(m, [{ g: 1, line: 'quit', n: 1, call: () => {
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), r = ram(), now = (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16 | h[L.now + 3] << 24) >>> 0;
          const w = (a, v, n) => { for (let i = 0; i < n; i++) r[a + i] = v >>> (8 * i) & 255; };
          r[ad('pc') + STATE] = 3;                  // (HARN_UNCONSCIOUS)
          w(ad('ko_at'), now, 4); w(ad('ko_n'), 2, 2); w(ad('pc_out'), (now + 3000) >>> 0, 4);
          h[PCOUT] = 1;
          m.kosN = r[ad('aden')]; w(ad('ko_at') + 4, 0x1234, 4); w(ad('ko_n') + 2, 5, 2);   // (Actor 0's, its sheet aden[0]'s: kept as the save's, KOSAVE)
          m.kept.push(read());
          m.kept[0].kos = [...r.subarray(ad('ko_at') + 4, ad('ko_at') + 8), ...r.subarray(ad('ko_n') + 2, ad('ko_n') + 4)];   // (Its KOS to come
                                                //   as kept: the referee's ko_at[1] and ko_n[1] as set, all the scene's (its
                                                //   RS_SCENE SCN_RAM), no raw poke of its fb bank's, lock2's, 2026-11-25)
        } }, { g: 2, line: 'look', n: 1, call: () => m.kept.push(read()) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], [a, b] = m.kept || [];
        if (!a || !b) return ['the scene not set or not read back: ' + JSON.stringify(m.kept)];
        if (!a.at) f.push('the fall not set as it quit: ' + JSON.stringify(a));
        if (b.at !== a.at) f.push('its fall not read back as kept: ' + a.at + ' then ' + b.at);
        if (b.n !== a.n) f.push('its shock rolls made not read back as kept: ' + a.n + ' then ' + b.n);
        if (b.out !== a.out) f.push('its time out not read back as kept: ' + a.out + ' then ' + b.out);
        if (b.state !== 3) f.push('not senseless still, carried on: its state ' + b.state);
        if (!b.hubOut) f.push('the world not holding it senseless, carried on (pc_out)');
        if (JSON.stringify(b.kos) !== JSON.stringify(a.kos)) f.push('a creature\'s fall and rolls (KOS) not read back from beasts.dat: ' + JSON.stringify(a.kos) + ' then ' + JSON.stringify(b.kos));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0savemid', near: ['save'], what: "Dingus Quest Zero's \"save\" typed in the middle of a game (2026-11-19: it halted the UI in every game, overlay 7's o_meta calling part 14's save, which left the hub's bank in the window, the overlay's code returned to run in it; main.c's s_save_game keeps the window now), in a new game and in one carried on, with the caches small (dq0 -C) and not, at seeds 3 and 5: each game played twice, the same commands, one with \"save\" typed among them and one without, both quit (the quit's save): the game kept (\"The game is kept.\") and going on, and the two folders' game.dat and world.dat byte for byte the same (a save typed changes nothing the quit keeps); then both carried on (resumed, not a new game), played on the same way, a \"save\" typed in one, quit and compared again; and the saved one carried on once more",
      init: 't_rc', cycles: 12000e6,
      machine: { input: dq0Typed(dq0Game({ lines: [...[[3, ''], [5, '-C '], [5, '']].flatMap(([s, c]) => {
        const d = '/ram/m' + s + (c ? 'c' : ''), e = d + 'n';
        const cmp = t => ['cmp ' + d + '/d1/game.dat ' + e + '/d1/game.dat && echo same game ' + d + ' ' + t,
          'cmp ' + d + '/d1/world.dat ' + e + '/d1/world.dat && echo same world ' + d + ' ' + t];
        const play = (dir, save, first) => [first ? 'dq0 -p t -s ' + s + ' -c /ram/c.hm -T ' + c + '-d ' + dir : 'dq0 -p t ' + c + '-d ' + dir,
          ...(first ? ['turn right', 'step forward'] : ['turn left']), ...(save ? ['save'] : []), 'look', 'quit'];
        return [...play(d, 1, 1), ...play(e, 0, 1), ...cmp('new'), ...play(d, 1, 0), ...play(e, 0, 0), ...cmp('carried'),
          'dq0 -p t ' + c + '-d ' + d, 'quit', 'rm -r ' + d, 'rm -r ' + e];
      }), 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], T = dq0Turns(m), kept = (out.match(/The game is kept\./g) || []).length;
        if (kept !== 6) f.push('"The game is kept." told ' + kept + ' times, not 6 (each "save" typed)');
        for (const d of ['/ram/m3', '/ram/m5c', '/ram/m5'])
          for (const t of ['new', 'carried'])
            for (const w of ['game', 'world'])
              if (!new RegExp('\\nsame ' + w + ' ' + d + ' ' + t + '\\r?\\n').test(out)) f.push(d + ', ' + t + ': its ' + w + '.dat with a "save" typed not byte for byte the one without (cmp)');
        let from = 0;
        for (const d of ['/ram/m3', '/ram/m5c', '/ram/m5'])
          for (let k = 0; k < 2; k++) {
            const c = T.of(d.endsWith('c') ? 'dq0 -p t -C -d ' + d : 'dq0 -p t -d ' + d, from);
            if (c.k < 0) { f.push(d + ': carried on, its ' + (k + 1) + ' time, not found'); break; }
            dq0Need(f, c, 'E_ARRIVE', { how: 'A_RESUMED' }, d + ': carried on (' + (k + 1) + '), not resumed');
            from = c.k + 1;
          }
        return f;
      },
      sdcard: true
    },
  ],
};
