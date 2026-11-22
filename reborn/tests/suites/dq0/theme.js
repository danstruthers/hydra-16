// ****************************************************************************
// theme.js - Dingus Quest Zero's tests: the themes, their making and the quests.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { card, typed } = require('../../lib/common.js');
const {
  xdBytes, saidTheme, PK_PORTCULLIS, DQ0_MAKE, dq0Life, THEMEC, CHUNK_BYTES, CHUNK_KEPT, WORLD_BANK_N, CSLOT_AT, O_WAY_OUT, O_NOWHERE, stratumRec,
  dq0Made, dq0Story, dq0Prose, dq0Turns, dq0Ev, dq0Rep, dq0Need, dq0Carried, dq0Here, DQ0_I, dq0Typed, dq0Poke
} = require('../../lib/dq0.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')
const CUT_SEEDS = [271828, 1000003, ...Array.from({ length: 22 }, (_, k) => 70000 + k)];   // (dq0cut's: the clock-sized, the play test's)

module.exports = {
  tests: [
    {
      name: 'dq0peoplegen', near: ['theme'], what: "Dingus Quest Zero's peoples in a region's history (Phase 4's, 2026-10-29: themegen's histgen and peoples (), HISTORY.md's inhabitants, HOUSE.md's the peoples): at seed 4 the dwarves' works lost in the hills, a tribe of gargun (brown orcs ruled by black ones, their king their leader) among the inhabitants, placed first (the peoples before the rest, as many as there's room for); at seed 22 men holding their keep still, its soldiers and their captain, and the story's who has it now theirs (\"Soldiers hold it still\"); each theme passing the theme checker (themec, in themegen's place: its status none)",
      init: 't_rc', cycles: 1500e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'themegen -q -s 4 -o /ram/a', 'echo seed 4 status $status', 'grep faction /ram/a/theme.txt',
        'themegen -q -s 22 -o /ram/b', 'echo seed 22 status $status', 'grep faction /ram/b/theme.txt', 'grep still /ram/b/theme.txt', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['seed 4 status\n', 'seed 22 status\n', '% echo done\ndone\n'],
      check(m, out) {
        const o = out.replace(/\r/g, ''), f = [];
        const part = (a, b) => o.slice(o.indexOf(a), o.indexOf(b));
        const fl = s => s.split('\n').filter(l => /^faction\s*\|/.test(l)).map(l => l.split('|').map(v => v.trim()));
        const g = fl(part('% grep faction /ram/a', '% themegen -q -s 22')), s = fl(part('% grep faction /ram/b', '% grep still'));
        if (!g.length || g[0][1] !== 'gargun') f.push('seed 4: the gargun not the first faction: ' + JSON.stringify(g.map(x => x[1])));
        else if (!g[0][2].split(' ').every(k => /^(arak|hyeka|khanu|kyani|viasal)$/.test(k)) || g[0][5] !== 'garking') f.push('seed 4: the gargun\'s kinds or king: ' + g[0].join(' | '));
        if (!s.length || s[0][1] !== 'soldiers' || s[0][2] !== 'manarms' || s[0][5] !== 'captain') f.push('seed 22: no soldiers first, men-at-arms and their captain: ' + JSON.stringify(s[0]));
        if (!/Soldiers hold it still/.test(part('% grep still /ram/b', '% echo done').replace(/\s+/g, ' '))) f.push('seed 22: the story not the soldiers\': ' + part('% grep still /ram/b', '% echo done').replace(/\s+/g, ' '));
        if (g.length > 8 || s.length > 8) f.push('more factions than a theme has room for');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0theme', meaning: true, what: 'Dingus Quest Zero\'s themes (theme.h: data, games/dq0/themes, made dq0t.dat), by the theme bank, the world\'s events and the things\' records: the theme picked by the seed (its second byte: seed 259 the second theme, the drowned temple; seed 3 the first, the crypt), kept in the save and the game carried on in it, its strata named by it (the senses\' record\'s stratum, the theme\'s names); the way out shut by its lock; then the crypt\'s puzzle\'s pieces (dq0 -k: the sunstone and the moonstone to hand) combined into the king\'s seal (its rule\'s: the stones gone, the seal carried), the seal used on the way out (it opens: the world\'s way_open), and taken: the game ended (escaped, E_LEFT), and the save marked done (a new game begun in its folder, not carried on) (its words, the theme\'s: the themes\' own data)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 259 -c /ram/c.hm -d /ram/t', 'u', 'quit', 'dq0 -d /ram/t',
        'map', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -d /ram/q', 'u', 'combine the sunstone with the moonstone', 'use the seal on the way out',
        'u', 'dq0 -p t -c /ram/c.hm -d /ram/q', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], theme = t => t.after ? t.after.theme : '?';
        const g0 = T.of('dq0 -p t -s 259 -c /ram/c.hm -d /ram/t'), g1 = T.of('dq0 -d /ram/t'), g2 = T.of('dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone -d /ram/q');
        if (theme(g0) !== 'the Drowned Temple') f.push('seed 259 not the second theme: ' + theme(g0));
        dq0Need(f, T.after(g0, 'u'), 'E_NOWAY', { why: 'N_SHUT' }, 'the way out not shut by its lock');
        dq0Need(f, g1, 'E_ARRIVE', { how: 'A_RESUMED' }, 'the game not carried on');
        if (theme(g1) !== 'the Drowned Temple') f.push('the theme not kept with the game: ' + theme(g1));
        const mp = T.after(g1, 'map');
        const st = mp.after && mp.after.sight ? mp.after.strata[mp.after.sight.sname] : '?';
        if (st !== 'Temple Steps') f.push('the stratum not named by the theme: ' + st);
        if (theme(g2) !== 'the Crypt of the Dead King') f.push('seed 3 not the first theme: ' + theme(g2));
        dq0Need(f, T.after(g2, 'u'), 'E_NOWAY', { why: 'N_SHUT' }, 'the crypt\'s way out not shut');
        const c = T.of('combine the sunstone with the moonstone'), has = (t, n) => dq0Carried(t.after).some(i => i.name === n);
        if (!dq0Rep(c, 'ireq', { kind: 'IO_COMBINE', result: 'IR_OK' }).length) f.push('the stones not combined');
        if (!has(c, 'king\'s seal') || has(c, 'sunstone') || has(c, 'moonstone')) f.push('the stones not made the king\'s seal');
        const u = T.of('use the seal on the way out');
        if (!u.after || !u.after.wayOpen || (u.at && u.at.wayOpen)) f.push('the way out not opened by the seal');
        const l = T.after(u, 'u');
        dq0Need(f, l, 'E_LEFT', {}, 'the way out not taken (escaped)');
        dq0Need(f, T.and(l), 'E_BYE', {}, 'the game not ended');
        dq0Need(f, T.of('dq0 -p t -c /ram/c.hm -d /ram/q'), 'E_ARRIVE', { how: 'A_START' }, 'the done save carried on, not a new game begun');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0why', near: ['theme', 'save', 'money'], what: "Dingus Quest Zero's why you came (the plan's step 7: themegen's goals.txt why lines, dq0acct's quest.c; DUNGEON.md's why you came, HOUSE.md's), by scenes: themegen's three rumours (-r) the same for the same seed and another for another, each a place and a reason of a goal kind of its own; at seed 70000 (an artifact) the reason told in the story where \"you came down to try\" was (its patron, the theme's why line), the cup to hand (dq0 -k) and walked out solved; carried on, the patron's reward told as it comes into town (the theme's reward line's words and pennies), its standing in the life's record and told after the account, and the three rumours heard in town, the life's seed's, the first followed (the tests' dq0auto): the next dungeon's region and reason the rumour's (quest.txt in its folder, themegen's part-written theme), its patron in its story; the same life again, the second rumour typed: its region and reason; and the two goal kinds only heard of played through, each by a why line given (dq0 -t), its thing to hand and walked out solved: lore (a codex: carried on, its scholar pays) and a scourge (the kobolds' chief's war standard, in the hills given); and each passing the theme checker at seeds 1-3",
      init: 't_rc', cycles: 9000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'themegen -q -r 3 -s 12345 -o /ram/q', 'themegen -q -r 3 -s 12345 -o /ram/r', 'cmp /ram/q/rumours.txt /ram/r/rumours.txt', 'echo same $status',
        'themegen -q -r 3 -s 12346 -o /ram/r', 'cmp /ram/q/rumours.txt /ram/r/rumours.txt', 'echo other $status', 'cat /ram/q/rumours.txt', 'rm -r /ram/q', 'rm -r /ram/r',
        'dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\' -d /ram/b', 'u', 'echo =NEXT',
        'dq0 -p t -d /ram/b', 'quit', 'cat /ram/b/d1/theme.txt', 'cat /ram/b/rumours.txt', 'cat /ram/b/d2/quest.txt', 'cat /ram/b/d2/theme.txt', 'xd /ram/b/life.dat', 'rm -r /ram/b',
        'dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\' -d /ram/e', 'u', 'dq0auto=()', 'echo =SECOND', 'dq0 -p t -d /ram/e', '6', '2', '', 'quit', 'dq0auto=1',
        'cat /ram/e/rumours.txt', 'cat /ram/e/d2/quest.txt', 'cat /ram/e/d2/theme.txt', 'rm -r /ram/e',
        "echo 'why      | lore | scholar | Haldan' > /ram/l.txt",
        "echo 'why      | scourge | reeve | Osric' > /ram/s.txt", "echo 'region   | the Grey Fells | hills | granite | springs | cold' >> /ram/s.txt",
        'echo =LORE', 'dq0 -p t -s 70000 -c /ram/c.hm -t /ram/l.txt -k codex -d /ram/l', 'u', 'xd /ram/l/d1/game.dat', 'cat /ram/l/d1/theme.txt', 'dq0 -p t -d /ram/l', 'quit', 'rm -r /ram/l',
        'echo =SCOURGE', 'dq0 -p t -s 70000 -c /ram/c.hm -t /ram/s.txt -k \'war standard\' -d /ram/s', 'u', 'xd /ram/s/d1/game.dat', 'cat /ram/s/d1/theme.txt', 'rm -r /ram/s',
        ...[1, 2, 3].flatMap(s => ['themegen -q -s ' + s + ' -t /ram/l.txt -o /ram/x', 'echo lore ' + s + ' st $status', 'grep goal /ram/x/theme.txt',
          'themegen -q -s ' + s + ' -t /ram/s.txt -o /ram/x', 'echo scourge ' + s + ' st $status', 'grep goal /ram/x/theme.txt']),
        'echo done']) },
      expect: ['% echo done'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), f = [], words = s => s.replace(/\s+/g, ' ').trim();
        const lines = text => DQ0_MAKE().lines(text, 'theme');                          // (A theme's lines, the format's)
        const key = (text, k) => lines(text).filter(l => l.key === k);
        const blocks = text => {                                                        // (Rumours: each a rumour line, then its region and why)
          const out = [];
          for (const l of lines(text)) { if (l.key === 'rumour') out.push({ said: l.v[0] }); else if (out.length) out[out.length - 1][l.key] = l.v; }
          return out;
        };
        const between = (a, b) => { const i = o.indexOf(a), j = i < 0 ? -1 : o.indexOf(b, i + a.length); return i < 0 ? '' : o.slice(i, j < 0 ? undefined : j); };
        // (Rumours by seed)
        if (!/% echo same \$status\nsame *\n/.test(o)) f.push('the same seed\'s rumours not the same');
        if (/% echo other \$status\nother *\n/.test(o)) f.push('another seed\'s rumours the same');
        const R = blocks(saidTheme(out, 'cat /ram/q/rumours.txt'));
        if (R.length !== 3) f.push(R.length + ' rumours made, not three');
        if (new Set(R.map(r => r.why && r.why[0])).size !== R.length) f.push('two rumours of one goal kind: ' + R.map(r => r.why && r.why[0]).join(' '));
        for (const r of R) {
          if (!r.region || r.region.length !== 5 || !r.why || r.why.length !== 3) f.push('a rumour without its region or why: ' + JSON.stringify(r));
          if (/%/.test(r.said) || !r.region || !r.said.includes(r.region[0])) f.push('a rumour\'s words without its place, or a slot unfilled: ' + r.said);
        }
        // (Seed 70000: the reason told, the reward and the standing, the rumours, the first followed)
        const t1 = saidTheme(out, 'cat /ram/b/d1/theme.txt'), why1 = key(t1, 'why')[0], rw1 = key(t1, 'reward')[0];
        if (!why1 || !rw1) return f.concat(['seed 70000\'s theme without its why or reward line']);
        const story1 = words(between('% dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\' -d /ram/b', 'enters the dungeon'));
        if (!story1.includes(why1.v[2] + ', a collector of old things, has promised good money for the Cup of')) f.push('the reason not told in the story: ' + story1.slice(-300));
        if (/came down to try|came down to find it/.test(story1)) f.push('"you came down to ..." still told');
        const c1 = THEMEC([{ name: 'theme.txt', text: t1 }])[0];
        if (c1.status) f.push('its theme.txt isn\'t a theme: ' + c1.said);
        const acct = words(between('% dq0 -p t -d /ram/b', 'The place\'s history'));
        if (!acct.includes(words(rw1.v[3]))) f.push('the reward not told: ' + words(rw1.v[3]));
        if (!acct.includes('pays you ' + rw1.v[0] + ' pennies')) f.push('not paid ' + rw1.v[0] + ' pennies');
        if (!acct.includes('A few folk in the inns know your name (standing ' + rw1.v[1] + ').')) f.push('the standing not told: ' + acct.slice(0, 600));
        const life = dq0Life(xdBytes(out, 'xd /ram/b/life.dat'));
        if (!life || life.h.reputation !== +rw1.v[1]) f.push('the record\'s reputation ' + (life && life.h.reputation) + ', not ' + rw1.v[1]);
        const B = blocks(saidTheme(out, 'cat /ram/b/rumours.txt'));
        if (B.length !== 3) f.push(B.length + ' rumours heard, not three');
        B.forEach((r, i) => { if (!acct.includes((i + 1) + '. ' + words(r.said))) f.push('rumour ' + (i + 1) + ' not told: ' + r.said); });
        if (!acct.includes('Which will you follow, 1, 2 or 3?')) f.push('not asked which to follow');
        const follows = (folder, k) => {                                               // (The next dungeon, the k'th rumour's)
          const q = blocks(saidTheme(out, 'cat ' + folder + '/d2/quest.txt'))[0], t2 = saidTheme(out, 'cat ' + folder + '/d2/theme.txt'), r = B[k];
          if (!q || !r || q.said !== r.said) return f.push(folder + ': quest.txt not rumour ' + (k + 1) + '\'s');
          const reg = key(t2, 'region')[0], why = key(t2, 'why')[0];
          if (!reg || reg.v.join('|') !== r.region.join('|')) f.push(folder + ': the next dungeon\'s region not the rumour\'s: ' + (reg && reg.v.join('|')));
          if (!why || why.v.join('|') !== r.why.join('|')) f.push(folder + ': the next dungeon\'s why not the rumour\'s: ' + (why && why.v.join('|')));
          if (!acct2(folder).includes('You set out for ' + r.region[0] + ', ')) f.push(folder + ': not told where it sets out for');
          const s2 = words(between('You set out for ' + r.region[0], 'enters the dungeon'));
          if (r.why[1] !== 'found' && !s2.includes(r.why[2])) f.push(folder + ': the patron not in the next story: ' + s2.slice(-300));
        };
        const acct2 = folder => words(between('% dq0 -p t -d ' + folder, 'The place\'s history'));
        follows('/ram/b', 0);
        if (saidTheme(out, 'cat /ram/e/rumours.txt') !== saidTheme(out, 'cat /ram/b/rumours.txt')) f.push('the same life\'s rumours not the same');
        follows('/ram/e', 1);
        // (The heard kinds played through: solved, walked out; lore carried on and paid)
        const solved = cmd => { const b = xdBytes(out, cmd); if (b[20] !== 1 || b[22] !== 1 || !b[23]) f.push(cmd + ': not walked out solved (outcome ' + b[20] + ', done ' + b[22] + ', solved ' + b[23] + ')'); };
        solved('xd /ram/l/d1/game.dat');
        solved('xd /ram/s/d1/game.dat');
        const tl = saidTheme(out, 'cat /ram/l/d1/theme.txt'), ts = saidTheme(out, 'cat /ram/s/d1/theme.txt');
        if (!key(tl, 'goal')[0] || key(tl, 'goal')[0].v[0] !== 'lore') f.push('-t\'s why lore not a lore goal');
        if (!key(ts, 'goal')[0] || key(ts, 'goal')[0].v[0] !== 'scourge') f.push('-t\'s why scourge not a scourge goal');
        const lore = words(between('% echo =LORE', '% echo =SCOURGE')), sc = words(between('% echo =SCOURGE', '% echo done'));
        if (!/Haldan, a scholar, will pay well for the Codex of/.test(lore)) f.push('lore\'s reason not told');
        if (!/You lift the Codex of \w+ from where it has lain/.test(lore)) f.push('the codex not told taken up');
        const rwl = key(tl, 'reward')[0];
        if (!rwl || !lore.includes(words(rwl.v[3]))) f.push('lore\'s scholar not told paying');
        if (!/Osric, the reeve of the farms by the Grey Fells, has put a price on the raiders' chief/.test(sc)) f.push('the scourge\'s reason not told');
        if (!/You take up the Standard of \w+\. Without it the kobolds will scatter/.test(sc)) f.push('the standard not told taken up');
        for (const k of ['lore', 'scourge']) for (const s of [1, 2, 3]) {
          if (!new RegExp('% echo ' + k + ' ' + s + ' st \\$status\\n' + k + ' ' + s + ' st *\\n% grep goal /ram/x/theme.txt\\ngoal +\\| ' + k + ' ').test(o)) f.push(k + ' at seed ' + s + ': not made, or the checker refused it');
        }
        if (/you (win|lose)|you've (won|lost)/i.test(o)) f.push('words of winning or losing');
        this.notes = ['rumours: ' + R.map(r => r.why.join('/')).join(', ') + '; 70000\'s: ' + B.map(r => r.why.join('/')).join(', ') + '; paid ' + rw1.v[0] + 'd'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pieces', meaning: true, what: 'Dingus Quest Zero\'s puzzle\'s pieces placed with the plan (theme.h: each one\'s stratum from the seed, its chunk in the stratum\'s halls as its plan is grown, put down as the chunk is made, by the maker\'s stock), by what the character perceives and the things\' records and replies: the crypt\'s stone tablet on the top stratum (dq0 -T -T: the character begins on it), seen at its feet, taken and read (the parser\'s read: a piece of the puzzle found to hand), a dagger not (nothing written on it); then the crown and the throne (dq0 -k), the throne put down and not taken up again (fixed where it stands: refused), the crown put on it: the place ruled, the game ended (the theme\'s ending; its words, the theme\'s data)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -T', 'take the tablet', 'read the tablet',
        'read the dagger', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -k crown,throne', 'drop the throne', 'take the throne', 'put the crown on the throne', 'u', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], rep = (l, kind, result, what) => { if (!dq0Rep(T.of(l), 'ireq', { kind, result }).length) f.push(what + ' (' + JSON.stringify(T.of(l).replies.filter(r => r.req === 'ireq').map(r => r.kind + ' ' + r.result)) + ')'); };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -T').after;
        if (!s0 || !s0.sight || !s0.sight.items.some(i => i.name === 'stone tablet' && i.rel === 255)) f.push('the stone tablet not seen at its feet as it begins');
        rep('take the tablet', 'IO_GET', 'IR_OK', 'the tablet not taken');
        if (!dq0Carried(T.of('take the tablet').after).some(i => i.name === 'stone tablet')) f.push('the tablet not carried');
        rep('read the tablet', 'IO_FIND', 'IR_OK', 'the tablet not read (a piece found to hand)');
        rep('read the dagger', 'IO_FIND', 'IR_NONE', 'the dagger read as a piece');
        const d = T.of('drop the throne');
        if (!dq0Here(d.after).some(i => i.name === 'throne')) f.push('the throne not put down');
        rep('take the throne', 'IO_GET', 'IR_NO', 'the throne taken up again (not fixed where it stands)');
        const p = T.of('put the crown on the throne');
        rep('put the crown on the throne', 'IO_PUT', 'IR_OK', 'the crown not put on the throne');
        if (dq0Ev(p, 'E_BYE', {}).length) f.push('the game ended as the place is ruled, not walked out of');
        if (!p.after || !p.after.wayOpen) f.push('the way out not opened, the place ruled');
        dq0Need(f, T.after(p, 'u'), 'E_LEFT', {}, 'not out, the place ruled');
        dq0Need(f, T.and(T.after(p, 'u')), 'E_BYE', {}, 'the game not ended, walked out of');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0themegen', what: 'Dingus Quest Zero\'s theme generator (Phase 3D; games/themegen, its bank programs bank/histgen, goalgen, linegen and namegen: HISTORY.md): "themegen -s 3" says a region\'s history and its theme (the region, its peoples and eras, the strata as layers, the inhabitants, the goal, its clues, the story), the same each time (made twice into folders, the files the same, cmp); its record (themec, run in its place, theme.dat) held to games/dq0/make.js\'s compile of the theme it said, byte for byte, and that theme to the theme checker (every goal reachable, every faction placed)',
      init: 't_rc', cycles: 900e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'themegen -s 3', 'themegen -q -s 3 -o /ram/a', 'echo $status',
        'themegen -q -s 3 -o /ram/b', 'cmp /ram/a/theme.txt /ram/b/theme.txt', 'cmp /ram/a/theme.dat /ram/b/theme.dat', 'echo $status',
        'xd /ram/a/theme.dat', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['seed     | 3', 'region   | the ', 'era      | ', 'layer    | ', 'faction  | ', 'goal     | ', 'story    | Under the',
        '% cmp /ram/a/theme.dat /ram/b/theme.dat\n% echo $status\n\n', '% echo done\ndone\n'],
      check(m, out) {
        const f = [], text = saidTheme(out, 'themegen -s 3'), dq0 = DQ0_MAKE(), r = THEMEC([{ name: 'themegen -s 3', text }])[0];
        if (r.status) return ['its theme isn\'t one, or fails the theme checker: ' + r.said];
        const dat = xdBytes(out, 'xd /ram/a/theme.dat');
        if (dat.length !== dq0.THEME_SIZE) f.push('theme.dat: ' + dat.length + ' bytes, not ' + dq0.THEME_SIZE);
        else for (let i = 0; i < dat.length; i++) if (dat[i] !== r.data[i]) { f.push('theme.dat: byte ' + i + ' is ' + dat[i] + ', themec\'s ' + r.data[i]); break; }
        if (/^\s+\S/m.test(text.split('\n').filter(l => /^[a-z]/.test(l)).join('\n'))) f.push('a line not in the theme format');
        this.notes = [(text.match(/^theme +\| (.*)/m) || [])[1] + ': ' + text.split('\n').filter(l => /^[a-z]/.test(l)).length + ' lines, ' + r.words + ' bytes of words'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0world', near: ['theme'], what: "Dingus Quest Zero's worlds (WORLDS.md: the user's world plug-in, 2026-11-17; the bare world, games/dq0/worlds/bare, the proof it swaps as the null referee is the rules'): themegen -w bare's histories (seeds 1-6, each passing the theme checker, themec in its place: its status) its peoples, lands, tongues, deities and words alone, none of the first world's (no dwarves, kobolds, Hills or Peaks; its region the Vales, Meads, Uplands or Tors; its eras its hearthfolk's and delvers'; its open line's story; no prisoner's goal nor scourge's, as it has no raiders); a world that isn't one refused (its history.dat not found, status 1); a game in it (dq0 -w bare, the null referee, a generated seed, kept in a folder): its story the bare world's, its deity's name the command parser's (\"beseech Wend\" understood), the first world's not (\"beseech Imma\" not); carried on with no -w, in the bare world still (its save's); and the same game in the default world as ever (\"beseech Imma\" understood)",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'themegen -w bare -s 3', 'echo said $status',
        ...[1, 2, 3, 4, 5, 6].flatMap(s => ['themegen -q -w bare -s ' + s + ' -o /ram/w' + s, 'echo seed ' + s + ' status $status', 'cat /ram/w' + s + '/theme.txt']),
        'themegen -q -w nosuch -s 3', 'echo nosuch $status',
        'dq0 -w bare -r null -p t -s 70000 -d /ram/bw', 'beseech Wend for help', 'beseech Imma for help', 'quit',
        'dq0 -r null -p t -d /ram/bw', 'beseech Wend for help', 'quit',
        'dq0 -r null -p t -s 70000', 'beseech Imma for help', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const o = out.replace(/\r/g, ''), f = [], first = /\b(dwarves|elves|kobolds|hill-men|sun-priests|gargun|Khuzdul|Hills|Peaks|Crags|Downs|Fells|Woods|Wealds|Fens|Cliffs|Plains|Heaths)\b/;
        const said = saidTheme(out, 'themegen -w bare -s 3');
        if (!/^story +\| Under the \w+ (Vales|Meads|Uplands|Tors) there is an old place\./m.test(said)) f.push('seed 3: its story not the bare world\'s: ' + JSON.stringify((said.match(/^story.*$/m) || [''])[0].slice(0, 200)));
        if (!o.includes('% echo said $status\nsaid\n')) f.push('themegen -w bare -s 3 failed');   // (rc's status none: 0)
        for (let s = 1; s <= 6; s++) {
          if (!o.includes('seed ' + s + ' status\n')) f.push('seed ' + s + ': the checker refused it (or it wasn\'t made)');
          const L = DQ0_MAKE().lines(saidTheme(out, 'cat /ram/w' + s + '/theme.txt'), 'seed ' + s), v = k => L.filter(l => l.key === k).map(l => l.v);
          for (const p of v('people')) if (!/^(hearthfolk|delvers)$/.test(p[1])) f.push('seed ' + s + ': a people not the bare world\'s: ' + p.join(' | '));
          const g = v('goal')[0];
          if (g && /^(prisoner|scourge)$/.test(g[0])) f.push('seed ' + s + ': a ' + g[0] + '\'s goal, and the bare world has no raiders');
          for (const x of v('faction')) if (!/^(vermin|crawlers|dead|beasts|outlaws)$/.test(x[0]) && !/^(vermin|crawlers|dead|beasts|outlaws)$/.test(x[1] || '')) f.push('seed ' + s + ': a faction not of the bare world\'s: ' + x.join(' | '));
          const t = saidTheme(out, 'cat /ram/w' + s + '/theme.txt'), b = t.match(first);
          if (b) f.push('seed ' + s + ': the first world\'s "' + b[0] + '" in it: ' + JSON.stringify(t.slice(Math.max(0, b.index - 100), b.index + 60)));
        }
        if (!/% echo nosuch \$status\nnosuch 1\n/.test(o) || !/its world's data \(\S*\/lib\/worlds\/nosuch\/history\.dat\) not found/.test(o)) f.push('a world that isn\'t one not refused: ' + JSON.stringify(o.slice(o.indexOf('nosuch'), o.indexOf('nosuch') + 200)));
        const p = dq0Prose(out), bare = p.slice(p.indexOf('dq0 -w bare'), p.indexOf('dq0 -r null -p t -d /ram/bw')), kept = p.slice(p.indexOf('dq0 -r null -p t -d /ram/bw'), p.indexOf('dq0 -r null -p t -s 70000')), dflt = p.slice(p.indexOf('dq0 -r null -p t -s 70000'));
        if (!/there is an old place\./.test(bare)) f.push('the game in the bare world: not its story: ' + JSON.stringify(bare.slice(0, 600)));
        const after = (s, cmd) => ((new RegExp('> ' + cmd + '\\s*(?:> ' + cmd + ')?\\s*([^>]*)').exec(s) || [])[1] || '');   // (Its words: the line's echo once)
        const understood = s => /You know no invocations/.test(s);
        if (!understood(after(bare, 'beseech Wend for help'))) f.push('the bare world: "beseech Wend" not understood: ' + JSON.stringify(after(bare, 'beseech Wend for help')));
        if (understood(after(bare, 'beseech Imma for help'))) f.push('the bare world: "beseech Imma", the first world\'s, understood');
        if (!understood(after(kept, 'beseech Wend for help'))) f.push('the bare world\'s game carried on: its world not its save\'s ("beseech Wend" not understood): ' + JSON.stringify(after(kept, 'beseech Wend for help')));
        if (!understood(after(dflt, 'beseech Imma for help'))) f.push('the default world: "beseech Imma" not understood: ' + JSON.stringify(after(dflt, 'beseech Imma for help')));
        if (first.test(bare.slice(0, bare.indexOf('> beseech')))) f.push('the game in the bare world: the first world\'s words in its story: ' + JSON.stringify(bare.slice(0, 600)));
        this.notes = [(said.match(/^theme +\| (.*)/m) || [])[1] + ': ' + ((said.match(/^region +\| ([^|]*)/m) || [])[1] || '').trim()];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0goals', what: 'Dingus Quest Zero\'s goal kinds (G5: data, games/themegen/goals.txt): themegen\'s histories for seeds 1-12, each passing the theme checker (themec, in themegen\'s place: its status), all four kinds among them (an artifact found, the evil cleared out, a prisoner rescued, a far exit won through to), each goal\'s stratum one of its history\'s and its guardians a faction there',
      init: 't_rc', cycles: 3000e6,
      machine: { input: ['bind -a /sd/0/games /bin', ...[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12].flatMap(s => ['themegen -q -s ' + s + ' -o /ram/g',
        'echo seed ' + s + ' status $status', 'grep goal /ram/g/theme.txt']), 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['seed 12 status\n', '% echo done\ndone\n'],
      check(m, out) {
        const o = out.replace(/\r/g, ''), f = [], kinds = new Set();
        for (let s = 1; s <= 12; s++) {
          if (!o.includes('seed ' + s + ' status\n')) f.push('seed ' + s + ': the checker refused it (or it wasn\'t made)');
          const g = new RegExp('% grep goal /ram/g/theme.txt\\n(goal +\\|.*)').exec(o.slice(o.indexOf('seed ' + s + ' status')));
          if (!g) f.push('seed ' + s + ': no goal line'); else kinds.add(g[1].split('|')[1].trim());
        }
        for (const k of ['artifact', 'evil', 'prisoner', 'through']) if (!kinds.has(k)) f.push('no ' + k + ' goal among seeds 1-12');
        this.notes = ['goal kinds: ' + [...kinds].join(', ')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0cut', near: ['theme'], what: "Dingus Quest Zero's generated words whole (GAME-CRITIQUE.md's 5.6, 2026-11-17): themegen's histories said (themegen -s, its theme to read) for 24 seeds, the clock-sized ones 70000 on, the play test's 271828 and 1000003: no words cut short to fit their room (themegen's tcut, fill's and linegen's: \"themegen: words cut short\" said; linegen's 240-byte buffer cut every cursed tomb's clue at 239, \"...and their dead would not lie s\", seed 70001's grave slab), every clue's and era's words ending in a full stop; the story's capitals (an era's first letter's place past 255 bytes an unsigned char's: \"aNd Wearag held it\", \"ago. the Khulim dwarves\" at seed 1000003): every sentence begun with a capital, none inside a word",
      init: 't_rc', cycles: 2400e6,
      get machine() {
        return { input: ['bind -a /sd/0/games /bin', ...CUT_SEEDS.flatMap(s => ['themegen -s ' + s, 'echo seed ' + s + ' status $status']), 'echo done']
          .map(l => '\u0101' + l + '\r').join('') };
      },
      expect: ['% echo done\ndone\n'],
      check(m, out) {
        const o = out.replace(/\r/g, ''), f = [];
        if (/words cut short/.test(o)) f.push('words cut short: ' + JSON.stringify(o.slice(Math.max(0, o.indexOf('words cut short') - 600), o.indexOf('words cut short') + 40)));
        for (const s of CUT_SEEDS) {
          if (!o.includes('seed ' + s + ' status\n')) f.push('seed ' + s + ': themegen failed');   // (rc's status none: 0)
          const L = DQ0_MAKE().lines(saidTheme(out, 'themegen -s ' + s), 'seed ' + s), v = k => L.filter(l => l.key === k).map(l => l.v);
          for (const r of v('rule').filter(r => r[0] === 'read')) if (!/[.!?]"?$/.test(r[r.length - 1])) f.push('seed ' + s + ': a clue cut: ' + JSON.stringify(r[r.length - 1].slice(-60)));
          for (const e of v('era')) if (!/\.$/.test(e[e.length - 1])) f.push('seed ' + s + ': an era\'s words cut: ' + JSON.stringify(e[e.length - 1].slice(-60)));
          const story = (v('story')[0] || [''])[0];
          if (!story) { f.push('seed ' + s + ': no story'); continue; }
          for (const x of story.split(/(?<=[.!?]"?)\s+/)) if (!/^"?[A-Z0-9]/.test(x)) f.push('seed ' + s + ': a sentence of its story not begun with a capital: ' + JSON.stringify(x.slice(0, 40)));
          const mid = story.match(/\b[a-z]+[A-Z]\w*/);
          if (mid) f.push('seed ' + s + ': a capital inside a word of its story: ' + JSON.stringify(mid[0]));
        }
        const s3 = DQ0_MAKE().lines(saidTheme(out, 'themegen -s 1000003'), '1000003').find(l => l.key === 'story');
        if (!s3 || !/called Gruog, and Wearag held it/.test(s3.v[0]) || !/centuries ago\. The Khulim dwarves came after/.test(s3.v[0]))
          f.push('seed 1000003: its story not as the history has it: ' + JSON.stringify(s3 && s3.v[0].slice(0, 500)));
        const c1 = DQ0_MAKE().lines(saidTheme(out, 'themegen -s 70001'), '70001').find(l => l.key === 'rule' && l.v[1] === 'grave slab');
        if (!c1 || !/their dead would not lie still\.$/.test(c1.v[2])) f.push('seed 70001: its grave slab\'s clue not whole: ' + JSON.stringify(c1 && c1.v[2]));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0themec', what: 'Dingus Quest Zero\'s theme compiler (games/themec): the stand-in themes (the card\'s lib/dq0/themes) compiled on the Hydra into the same records as make.js makes them at build time (dq0t.dat\'s, but for their count), byte for byte; and the theme checker refusing a theme whose way out can\'t be opened (no rule opens it), that has no way out to be had unsolved and no words for it, and whose faction can\'t live where it\'s said to (kobolds on a built stratum), said why',
      init: 't_rc', cycles: 900e6,
      machine: { input: ['bind -a /sd/0/games /bin', ...['1crypt', '2temple', '3warren'].flatMap(n => ['themec -o /ram/' + n + '.dat /sd/0/games/lib/dq0/themes/' + n + '.txt',
        'xd /ram/' + n + '.dat']), 'echo \'theme    | the Bad Place\' >/ram/bad.txt', 'echo \'mission  | escape\' >>/ram/bad.txt',
        'echo \'stratum  | Top | b | halls\' >>/ram/bad.txt', 'echo \'faction  | kobolds | kobold | 0-0 | 9 | - | 0 | u | -\' >>/ram/bad.txt',
        'themec /ram/bad.txt', 'echo $status'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['the Crypt of the Dead King: ', 'the Drowned Temple: ', 'the Kobold Warren: ', 'the theme checker: no rule that opens the way out can be done', 'the theme checker: no way out: neither wayin nor wayout', 'the theme checker: no ending for leaving',
        'the theme checker: kobolds: none of its kinds lives in its strata', '% echo $status\n1\n'],
      check(m, out) {
        const f = [], built = DQ0_MAKE().standIns(path.join(TESTS, '..', 'obj', 'games'));
        for (const [k, n] of ['1crypt', '2temple', '3warren'].entries()) {
          const r = built[k];
          const dat = xdBytes(out, 'xd /ram/' + n + '.dat');
          if (dat.length !== r.length) { f.push(n + ': ' + dat.length + ' bytes, not ' + r.length); continue; }
          for (let i = 0; i < r.length; i++) if (dat[i] !== r[i]) { f.push(n + ': byte ' + i + ' is ' + dat[i] + ', the build\'s ' + r[i]); break; }
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0history', what: 'Dingus Quest Zero\'s own theme (theme.h): a game whose seed is past the stand-ins\' (65,536 and on, as a clock\'s is) has its region\'s history and its theme made as it begins (themegen, run by the UI before the world: "The place\'s history is being made"), into its save\'s folder (theme.txt, theme.dat), its story told the theme\'s word for word (its name, then its story: the region, its eras, who has it now, the goal), the top stratum named by it; quit and carried on (dq0 -d), the same theme read back from the folder, not made again',
      init: 't_rc', cycles: 1500e6,
      machine: { input: ['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 70001 -c /ram/c.hm -d /ram/h', 'map', 'quit',
        'ls /ram/h/d1', 'cat /ram/h/d1/theme.txt', 'dq0 -d /ram/h', 'quit', 'echo done'].map(l => '\u0101' + l + '\r').join('') },
      expect: ['The place\'s history is being made', 'Under the ', 'theme.dat', 'theme.txt', 'takes up where they left off.', '% echo done\ndone\n'],
      check(m, out) {
        const o = out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\r/g, ''), f = [], dq0 = DQ0_MAKE();
        const text = saidTheme(out, 'cat /ram/h/d1/theme.txt');
        let r;
        if ((r = THEMEC([{ name: 'theme.txt', text }])[0]).status) return ['its theme.txt isn\'t a theme: ' + r.said];
        const words = s => s.replace(/\s+/g, ' ').trim(), name = (text.match(/^theme +\| (.*)/m) || [])[1] || '?';
        const story = words(r.data.toString('latin1', r.data.readUInt16LE(776), r.data.indexOf(0, r.data.readUInt16LE(776))));
        const game = words(o.slice(o.indexOf('history is being made'), o.indexOf('enters the dungeon.')));
        const title = name[0].toUpperCase() + name.slice(1) + '.';
        if (!game.includes(title)) f.push('its name not told: ' + title);
        if (!game.includes(story)) f.push('its story not told word for word: ' + story.slice(0, 80) + ' ...');
        const top = (text.match(/^stratum +\| ([^|]*)\|/m) || [])[1].trim();
        if (!o.includes(top.slice(0, 14))) f.push('the top stratum not named by it: ' + top);
        const again = o.slice(o.indexOf('% dq0 -d /ram/h'));
        if (again.includes('history is being made')) f.push('carried on, its history made again');
        if (!again.includes(title)) f.push('carried on, its name not told: ' + title);
        this.notes = [name + ': ' + text.split('\n').filter(l => /^[a-z]/.test(l)).length + ' lines; ' + (story.length) + ' letters of story'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0quest', meaning: true, what: 'Dingus Quest Zero\'s goals from a region\'s history played to their ends (Phase 3D: G5, the user\'s: "Each dungeon needs a goal, and it may be to find a specific artifact, clean out the evil denizens, rescue a prisoner, or just find a way through to a distant exit"; themegen\'s goals.txt kinds): at seed 70000 the Cup of Rowon (an artifact): dq0 -T -T begins by the goal\'s own thing, the jade cup lying hidden where the history put it; with it to hand (dq0 -k) and the seeker\'s journal read, the way out shut, the cup held up to it (open), and up it: escaped; at seed 70007 the curse of Yiskeek (an evil): by the bone idol (dq0 -T -T), the silver bell and the old scroll to hand, the scroll read, the bell laid on the idol: the curse broken (ruled, the game ended); at seed 70012 Voven, the kobolds\' prisoner: the shackle key put to the chained woman, the way out open (hers), and up it: escaped; at seed 70008 the Far Door (a way through): dq0 -T -T -T (four strata grown below the top) the goal\'s stratum (the fourth) with a way out of its own, a far exit, and none on the strata between',
      init: 't_rc', cycles: 2200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 70000 -c /ram/c.hm -T -T', 'quit',
        'dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\'', 'read the journal', 'u',
        'dq0 -p t -s 70007 -c /ram/c.hm -T -T -k \'silver bell,old scroll\'', 'read the scroll', 'put the bell on the idol', 'quit',
        'dq0 -p t -s 70012 -c /ram/c.hm -k \'shackle key,chained woman\'', 'drop the woman', 'put the key on the woman', 'u',
        'dq0 -p t -s 70008 -c /ram/c.hm -T -T -T', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) { dq0Made(this, m); },
      check(m) {
        const T = dq0Turns(m), f = [], has = (t, n) => dq0Carried(t.after).some(i => i.name === n);
        const g1 = T.of('dq0 -p t -s 70000 -c /ram/c.hm -T -T');
        if (!g1.after || !dq0Here(g1.after).some(i => i.name === 'jade cup' && (i.flags & DQ0_I.HIDDEN))) f.push('dq0 -T -T not begun by the jade cup, the goal\'s, hidden');
        const g2 = T.of('dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup,journal\'');
        if (!has(g2, 'jade cup') || !has(g2, 'journal')) f.push('the cup and the journal not to hand (dq0 -k)');
        if (!dq0Rep(T.after(g2, 'read the journal'), 'ireq', { kind: 'IO_FIND', result: 'IR_OK' }).length) f.push('the journal not read (a piece found to hand)');
        if (!g2.after || !g2.after.wayOpen) f.push('the artifact\'s way in not open behind it');
        const l = T.after(g2, 'u');
        dq0Need(f, l, 'E_LEFT', {}, 'the way out not taken (escaped)');
        dq0Need(f, T.and(l), 'E_BYE', {}, 'the artifact\'s game not ended');
        const g3 = T.of('dq0 -p t -s 70007 -c /ram/c.hm -T -T -k \'silver bell,old scroll\'');
        if (!g3.after || !dq0Here(g3.after).some(i => i.name === 'bone idol')) f.push('dq0 -T -T not begun by the bone idol, the curse\'s');
        if (!dq0Rep(T.after(g3, 'read the scroll'), 'ireq', { kind: 'IO_FIND', result: 'IR_OK' }).length) f.push('the old scroll not read (a piece found to hand)');
        const p = T.after(g3, 'put the bell on the idol');
        if (!dq0Rep(p, 'ireq', { kind: 'IO_PUT', result: 'IR_OK' }).length) f.push('the bell not laid on the idol');
        if (!p.after || !p.after.wayOpen || (p.at && p.at.wayOpen)) f.push('the way out not opened, the curse broken');
        if (dq0Ev(p, 'E_BYE', {}).length) f.push('the game ended as the curse is broken, not walked out of');
        const g4 = T.of('dq0 -p t -s 70012 -c /ram/c.hm -k \'shackle key,chained woman\''), k = T.after(g4, 'put the key on the woman');
        if (!k.after || !k.after.wayOpen || (k.at && k.at.wayOpen)) f.push('the prisoner\'s way out not opened by the key');
        const l4 = T.after(k, 'u');
        dq0Need(f, l4, 'E_LEFT', {}, 'the prisoner\'s way out not taken (escaped)');
        dq0Need(f, T.and(l4), 'E_BYE', {}, 'the prisoner\'s game not ended');
        // (Seed 70008, the strata's records at its quit: the goal's stratum, the fourth, with a far exit)
        const st = (this.games || [])[4] ? this.games[4].strata : [];
        const outs = sn => st[sn] ? st[sn].links.filter(l => l.kind === O_WAY_OUT && l.to === O_NOWHERE).length : -1;
        if (outs(3) !== 1) f.push('the Far Door\'s stratum (the fourth) with ' + outs(3) + ' ways out of its own, not one');
        for (const sn of [1, 2]) if (outs(sn) !== 0) f.push('stratum ' + sn + ' with ' + outs(sn) + ' ways out, not the goal\'s');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0ends', near: ['theme', 'save'], what: "Dingus Quest Zero's three ends (the between-dungeons plan's steps 1 and 2, the user's Q1 \"Every dungeon\" and Q2 \"No, walk out\"; HOUSE.md's how a dungeon ends), each goal kind and each way out a scene, by the world's events, the words told and game.dat's outcome (xd: Q.done, Q.solved, the save's last bytes but plain): a sky shaft climbed unsolved (the crypt, seed 2, its roof poked in reach, no grating): left, its own words, then a carry-on into a new dungeon; an artifact's way in open: up it once told what that does, again left; the artifact itself to hand (dq0 -k): told it's done, walked out solved at the first asking, in its words; the thing taken up in play told once; the evil's way out shut, the curse broken told and the game going on, kept through a quit and walked out (ruled); the prisoner's way out shut, its other way out on the seed's stratum (dq0 -T -T -T, put by a poke at the foot of that stair) left unsolved; a through goal (a hand-written theme: the checker's way out its far exit) out by its far exit solved, with no asking; and a hard-mode death after solving still solved, in its words",
      init: 't_rc', cycles: 5000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0sky=1', 'dq0hour=12',
        'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/a', 'go to the shaft', 'u', 'xd /ram/a/d1/game.dat', 'dq0 -p t -s 259 -d /ram/a', 'quit', 'rm -r /ram/a',
        'dq0 -p t -s 70000 -c /ram/c.hm -d /ram/b', 'examine the way out', 'u', 'look', 'u', 'xd /ram/b/d1/game.dat', 'rm -r /ram/b',
        'dq0 -p t -s 70000 -c /ram/c.hm -k \'jade cup\' -d /ram/c', 'look', 'u', 'xd /ram/c/d1/game.dat', 'rm -r /ram/c', 'cp /ram/c.hm /ram/s.hm', 'echo ml awa 99 >> /ram/s.hm',
        'dq0 -p t -s 70000 -c /ram/s.hm -T -T', 'search', 'search', 'search', 'search', 'take the cup', 'drop the cup', 'take the cup', 'quit',
        'dq0 -p t -s 70007 -c /ram/c.hm -k \'silver bell,bone idol\' -d /ram/e', 'drop the idol', 'u', 'examine the way out', 'put the bell on the idol', 'quit', 'dq0 -p t -d /ram/e', 'u', 'xd /ram/e/d1/game.dat', 'rm -r /ram/e',
        'dq0 -p t -s 70012 -c /ram/c.hm -T -T -T -d /ram/f', 'look', 'u', 'examine the way out', 'u', 'xd /ram/f/d1/game.dat', 'rm -r /ram/f',
        'echo \'theme    | the Long Road\' > /ram/t.txt', 'echo \'mission  | escape\' >> /ram/t.txt', 'echo \'stratum  | the Upper Road | b | halls\' >> /ram/t.txt', 'echo \'stratum  | the Lower Road | n\' >> /ram/t.txt',
        'echo \'goal     | through | the Far Door | 1 | -\' >> /ram/t.txt', 'echo \'lock     | It has fallen in.\' >> /ram/t.txt',
        'echo \'ending   | escaped | You come up through the Far Door.\' >> /ram/t.txt', 'echo \'ending   | left | You climb out, the road untried.\' >> /ram/t.txt', 'echo \'ending   | died | The road keeps you.\' >> /ram/t.txt',
        'dq0 -p t -s 5 -c /ram/c.hm -t /ram/t.txt -T -T -T -d /ram/g', 'look', 'u', 'xd /ram/g/d1/game.dat', 'rm -r /ram/g',
        'cp /ram/c.hm /ram/w.hm', 'echo \'inj 8 0 3 4 0 1\' >> /ram/w.hm', 'echo \'vol 0 9 0 0 0\' >> /ram/w.hm',
        'dq0 -p t -s 3 -c /ram/w.hm -H -k crown,throne -d /ram/h', 'drop the throne', 'put the crown on the throne', 'wait 10 minutes', 'xd /ram/h/d1/game.dat', 'echo $status']) },
      expect: ['% echo $status'], sdcard: true,
      start(m) {
        const roof = ft => () => {                       // (The roof of the character's chunk's rooms: a shaft's mouth in reach)
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), w = m.dq0.bank(WORLD_BANK_N), st = stratumRec(m, h[L.pcSn]);
          const ox = (h[L.pcX] | h[L.pcX + 1] << 8) - st.x0, oy = (h[L.pcY] | h[L.pcY + 1] << 8) - st.y0;
          const [ZONE, ZSIZE, CMID, NZ] = require('../../../sim/lib/hubown.js').evaluate(['offsetof (struct chunk, zone)', 'sizeof (struct zone)', 'offsetof (struct zone, cmid)', 'offsetof (struct chunk, nzones)']);
          for (let c = 0; c < 24; c++) {
            const sl = CSLOT_AT + c * 8, b = m.dq0.bank(5 + Math.floor(c / 3)), o = (c % 3) * 0xAA0;
            if (w[sl] && b && w[sl + 1] === h[L.pcSn] && (w[sl + 2] | w[sl + 3] << 8) === ox >> 5 && (w[sl + 4] | w[sl + 5] << 8) === oy >> 5) {
              for (let z = 0; z < b[o + NZ]; z++) b[o + ZONE + z * ZSIZE + CMID] = ft;
            }
          }
        };
        const toExit = () => {                           // (At the foot of the stair out on the character's stratum)
          const h = m.dq0.bank(0), { L } = m.dq0.layout(), k = stratumRec(m, h[L.pcSn]).links.find(l => l.kind === O_WAY_OUT && l.to === O_NOWHERE);
          (m.exits = m.exits || []).push(k ? h[L.pcSn] : -1);
          if (k) { h[L.pcX] = k.x & 255; h[L.pcX + 1] = k.x >> 8; h[L.pcY] = k.y & 255; h[L.pcY + 1] = k.y >> 8; }
        };
        dq0Poke(m, [{ g: 1, line: 'u', n: 1, call: roof(8) }, { g: 8, line: 'look', n: 1, call: toExit }, { g: 9, line: 'look', n: 1, call: toExit }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], st = dq0Prose(out), G = st.split('enters the dungeon'), ok = (g, re, what) => { if (!re.test(G[g] || '')) f.push(what + ' (game ' + g + ')'); };
        const no = (g, re, what) => { if (re.test(G[g] || '')) f.push(what + ' (game ' + g + ')'); };
        const q = cmd => {                   // (game.dat: its head 20 bytes, the outcome, then Q: theme, done, solved)
          const b = xdBytes(out, cmd);
          if (b.length && b[20] !== (b[23] ? 1 : b[22] === LEFT ? 2 : b[22] ? 3 : 0)) f.push(cmd + ': its outcome ' + b[20] + ', done ' + b[22] + ', solved ' + b[23]);
          return b.length ? [b[22], b[23]] : [];
        };
        const eq = (a, b, what) => { if (JSON.stringify(a) !== JSON.stringify(b)) f.push(what + ': done, solved ' + JSON.stringify(a)); };
        const ESCAPED = 1, RULED = 2, DIED = 3, LEFT = 4;
        // A sky shaft, unsolved, then a carry-on
        ok(1, /You set to climbing: 20 feet of it\.[^]*You climb out into the light of day with Aldric's door still shut/, 'out by the shaft not left, in its words');
        ok(1, /Your character carries on/, 'left, not said to carry on');
        no(1, /You have escaped|grating/, 'the old words of a shaft');
        eq(q('xd /ram/a/d1/game.dat'), [LEFT, 0], 'left by the shaft');
        dq0Need(f, T.of('dq0 -p t -s 259 -d /ram/a'), 'E_ARRIVE', { how: 'A_START' }, 'no new dungeon after leaving');
        const g2 = T.of('dq0 -p t -s 259 -d /ram/a');
        if (!g2.after || g2.after.theme !== 'the Drowned Temple') f.push('the carry-on not in its new theme: ' + (g2.after && g2.after.theme));
        // The artifact's way in: open, asked once
        ok(3, /The way up stands open: go up to leave\./, 'the way in not told open');
        ok(3, /That's the way out, and what you came for undone\. Go up again to leave it so\./, 'not told what going up would do');
        ok(3, /with empty hands/, 'its left words not told');
        no(3, /in your hands/, 'its solved words told, left');
        eq(q('xd /ram/b/d1/game.dat'), [LEFT, 0], 'left by the way in');
        // The artifact to hand: told, out solved with no asking
        ok(4, /So this is the Cup of [A-Za-z]+, where they said it would be\.[^]*That's what you came to do\. Now to get out alive\./, 'the artifact to hand not told done');
        no(4, /That's the way out, and/, 'asked, solved');
        ok(4, /You climb out into the daylight with the Cup of [A-Za-z]+ in your hands\./, 'its solved words not told');
        eq(q('xd /ram/c/d1/game.dat'), [ESCAPED, ESCAPED], 'solved by the artifact');
        // The artifact taken up in play: told once
        if ((G[5] || '').split("That's what you came to do").length !== 2) f.push('the artifact taken up in play not told once: ' + ((G[5] || '').split("That's what you came to do").length - 1));
        // The evil: shut, then told, kept, walked out
        ok(6, /The way out is shut fast\. The curse holds it till the curse is broken; if there's another way up, it's somewhere below\./, 'the curse\'s lock not told');
        dq0Need(f, T.after(T.of('dq0 -p t -s 70007 -c /ram/c.hm -k \'silver bell,bone idol\' -d /ram/e'), 'u'), 'E_NOWAY', { why: 'N_SHUT' }, 'the curse\'s way out not shut');
        ok(6, /You lay the Bell of [A-Za-z]+ upon the idol of [A-Za-z]+\.[^]*That's what you came to do\./, 'the curse broken not told done');
        ok(6, /You climb out into the daylight, and behind you the dead of [^.]* sleep at last\./, 'its ruled words not told, kept through a quit');
        eq(q('xd /ram/e/d1/game.dat'), [RULED, RULED], 'solved by the curse broken');
        // The prisoner's other way out, unsolved
        ok(7, /A stair climbs to the open air: go up to leave\./, 'the other way out not told, examined');
        ok(7, /That's the way out, and what you came for undone/, 'asked at the other way out');
        ok(7, /You climb out into the daylight alone\./, 'its left words not told');
        eq(q('xd /ram/f/d1/game.dat'), [LEFT, 0], 'left by the other way out');
        // A through goal's far exit: solved by going out
        ok(8, /You come up through the Far Door\./, 'the far exit not solved');
        no(8, /That's the way out, and/, 'asked at the far exit');
        eq(q('xd /ram/g/d1/game.dat'), [ESCAPED, ESCAPED], 'solved by the far exit');
        if (JSON.stringify(m.exits) !== '[1,1]') f.push('the stairs out not on the strata looked at: ' + JSON.stringify(m.exits));
        // Died, solved
        ok(9, /You did what you came to do, and you won't be climbing out to tell of it\./, 'a death after solving not worded so');
        no(9, /The crypt keeps you/, 'its plain death\'s words told');
        eq(q('xd /ram/h/d1/game.dat'), [DIED, RULED], 'died, solved');
        return f;
      }
    },
    {
      name: 'dq0night', near: ['theme', 'time'], what: "Dingus Quest Zero's words by the light outside (the theme format's {day/night/dawn/dusk}: theme.c's words_at, time.c's light_words, by the true hour as the sky shows it; GAME-CRITIQUE.md's 5.6, the play test's \"Above, it's dark: it's night.\", then the crypt's ending \"You climb the stair into the light of day\"): the crypt (seed 3) at ten at night (dq0hour=22), its seal made whole and used on the way out: its door grinding open on the stair up to the night sky, the way out looked at the same, and up it into the dark of night, the light of day not said; at six in the morning the grey of dawn, and the grey light of dawn",
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', ...['22', '6'].flatMap(h => ['dq0hour=' + h,
        'dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone', 'drop the hatchet', 'combine the sunstone with the moonstone', 'use the seal on the way out',
        'examine the way out', 'u']), 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), at = s.indexOf('dq0hour=6'), g = [s.slice(0, at < 0 ? s.length : at), at < 0 ? '' : s.slice(at)];
        const has = (k, w, what) => { if (!g[k].includes(w)) f.push(what + ': ' + JSON.stringify(w) + ' not said'); };
        has(0, 'and the door grinds open on the stair up to the night sky.', 'at night, the door opened');
        has(0, 'The black door stands open on the stair up to the night sky: go up to leave.', 'at night, the way out looked at');
        has(0, 'You climb the stair into the dark of night, and behind you the hill closes again over the Crypt of the Dead King.', 'at night, the ending');
        if (/light of day|daylight|\{/.test(g[0])) f.push('at night, the day\'s words said (or a choice\'s braces)');
        has(1, 'and the door grinds open on the stair up to the grey of dawn.', 'at dawn, the door opened');
        has(1, 'You climb the stair into the grey light of dawn, and behind you', 'at dawn, the ending');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0handtheme', meaning: true, what: 'Dingus Quest Zero\'s hand-written theme (dq0 -t FILE: games/dq0/themes/1crypt.txt\'s format, its history\'s groups): a theme giving its name, its region and its two strata, made into a game\'s own by themegen (its seed\'s history filling the rest: the head\'s mission and story made, line by line; the eras, the inhabitants, the goal and its puzzle), written into the save\'s folder and compiled (theme.txt, theme.dat), even at a small seed (5: a stand-in\'s, but for -t); the game in it: the theme\'s name, its top stratum the one given',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'echo \'theme    | the Pit of Echoes\' > /ram/t.txt', 'echo \'region   | the Grey Fells | hills | granite | springs | cold\' >> /ram/t.txt',
        'echo \'stratum  | the Upper Pit | bx | keep\' >> /ram/t.txt', 'echo \'stratum  | the Echoing Deep | n\' >> /ram/t.txt',
        'dq0 -p t -s 5 -c /ram/c.hm -t /ram/t.txt -d /ram/y', 'quit', 'cat /ram/y/d1/theme.txt', 'echo done']) },
      expect: ['% echo done\ndone\n'],
      check(m, out) {
        const T = dq0Turns(m), f = [], text = saidTheme(out, 'cat /ram/y/d1/theme.txt'), keyed = k => text.split('\n').filter(l => l.startsWith(k.padEnd(9) + '|'));
        const c = THEMEC([{ name: 'theme.txt', text }])[0];
        if (c.status) f.push('its theme.txt isn\'t a theme: ' + c.said);
        const want = { theme: ['theme    | the Pit of Echoes'], region: ['region   | the Grey Fells | hills | granite | springs | cold'],
          stratum: ['stratum  | the Upper Pit | bx | keep', 'stratum  | the Echoing Deep | n'] };
        for (const [k, w] of Object.entries(want)) if (keyed(k).join('\n') !== w.join('\n')) f.push('its ' + k + ' lines not the ones given: ' + keyed(k).join(' / '));
        for (const k of ['mission', 'story', 'era', 'faction', 'goal', 'piece']) if (!keyed(k).length) f.push('no ' + k + ' line made');
        const s = T.of('dq0 -p t -s 5 -c /ram/c.hm -t /ram/t.txt -d /ram/y').after;
        if (!s || s.theme !== 'the Pit of Echoes') f.push('the game not in the hand-written theme: ' + (s && s.theme));
        if (!s || !s.strata || s.strata[0] !== 'the Upper Pit') f.push('the top stratum not the one given: ' + (s && s.strata && s.strata[0]));
        this.notes = [text.split('\n').filter(l => /^[a-z]/.test(l)).length + ' lines, ' + keyed('era').length + ' eras, ' + keyed('faction').length + ' factions made'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0styles', near: ['theme'], meaning: true, what: 'Dingus Quest Zero\'s styles\' proportions in sets (games/dq0/styles.txt, the build\'s lib/dq0/styles.dat; the user\'s, 2026-11-12): a theme naming another set gets that set\'s proportions.  The same hand-written theme (dq0 -t FILE: its name and one built stratum of halls) twice at seed 5, once as it is (the first set, the house\'s: a built stratum\'s every way out a door) and once with "styles | open" (the open set: every way out a doorway); its styles line kept through themegen (theme.txt) and compiled by themec into the record\'s mission (TM_STYLES: the set\'s number, 1); the maker\'s chunks of the top stratum in the hub\'s cache as each game quits: doors (and portcullises, a door\'s) in the first, none in the second (each way out a doorway: its square T_DOOR, but no portal)',
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'echo \'theme    | the Open Halls\' > /ram/t.txt', 'echo \'stratum  | the Halls | b | halls\' >> /ram/t.txt',
        'dq0 -p t -s 5 -c /ram/c.hm -t /ram/t.txt -d /ram/a', 'look', 'quit',
        'echo \'styles   | open\' >> /ram/t.txt', 'dq0 -p t -s 5 -c /ram/c.hm -t /ram/t.txt -d /ram/b', 'look', 'quit',
        'cat /ram/b/d1/theme.txt', 'echo done']) },
      expect: ['% echo done\ndone\n'],
      start(m) { dq0Made(this, m); },
      check(m, out) {
        const PK_DOOR = 0, f = [], g = this.games || [], doors = c => c.portals.filter(p => p.kind === PK_DOOR || p.kind === PK_PORTCULLIS).length;
        const text = saidTheme(out, 'cat /ram/b/d1/theme.txt'), styles = text.split('\n').filter(l => /^styles\s*\|/.test(l));
        if (styles.length !== 1 || !/\|\s*open\s*$/.test(styles[0])) f.push('the styles line not kept in theme.txt: ' + JSON.stringify(styles));
        const c = THEMEC([{ name: 'theme.txt', text }])[0];
        if (c.status || !c.data) f.push('its theme.txt isn\'t a theme: ' + c.said);
        else if ((c.data[1] & 0x3C) >> 2 !== 1) f.push('the record\'s set of styles ' + ((c.data[1] & 0x3C) >> 2) + ', not 1 (open)');
        const [a, b] = [g[0], g[1]].map(x => x ? x.chunks : []);
        if (a.length < 2 || b.length < 2) f.push('built chunks of the top stratum in the cache: ' + a.length + ' and ' + b.length + ', not two or more each');
        const nhouse = a.reduce((n, x) => n + doors(x), 0), nopen = b.reduce((n, x) => n + doors(x), 0);
        if (!nhouse) f.push('no doors in the house\'s set\'s chunks');
        if (nopen) f.push(nopen + ' door(s) in the open set\'s chunks');
        this.notes = [a.length + ' and ' + b.length + ' built chunks, ' + nhouse + ' and ' + nopen + ' doors'];
        return f;
      },
      sdcard: true
    },
  ],
};
