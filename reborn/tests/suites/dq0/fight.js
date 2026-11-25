// ****************************************************************************
// fight.js - Dingus Quest Zero's tests: fighting and the referee.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { typed } = require('../../lib/common.js');
const {
  liveChunks, stratumRec, dq0Story, dq0Prose, inTurn, dq0Answers, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need, dq0Never,
  dq0Carried, dq0Named, dq0Thing, DQ0_I, dq0Typed, dq0Game, dq0Rec, dq0Alone, dq0Scene, dq0SceneNow, dq0Body, dq0PcShock, dq0Grow,
  dq0Aware, dq0OnWords, WORN, dq0Ad, dq0Timer, dq0Asks, dq0Paces, dq0HeldPaces, dq0FightPause, dq0Root
} = require('../../lib/dq0.js');
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')
const MID_MARK = '\u0104', NO_ROUND = 60e6, MID_GAP = 3e6;   // (dq0fightmid's: a line typed as the character's own blow is being worked, 3M cycles after the line before at least; none in 60M cycles, typed anyway)
const NO_ROUND_MID = 400e6;   // (dq0fightmid's own wait for a round, 2026-11-21, the coordinator's: its rat fights on, so a round comes; 60M let a line go with none met as the tasks' timing moved)
const FIGHTMID = ['d', 'wait', 'go right', 'inventory', MID_MARK + 'look', MID_MARK + 'wait', MID_MARK + 'inventory', MID_MARK + 'go left'];   // (dq0fightmid's lines: dq0fightcmd's way to the rats, a rat engaged as inventory's typed, then lines typed mid-round: a look and what's carried told at once, the fight going on; a wait and a step held till its round's out, the fight stopped for them, and taken up again as the rat bites)
const FELLED = ['d', 'wait', 'go right', 'inventory', MID_MARK + 'look', MID_MARK + 'look', MID_MARK + 'look', MID_MARK + 'wait', MID_MARK + 'go left'];   // (dq0felled's: dq0fightmid's way to the rats, a rat engaged and hurt as inventory's typed, then lines typed mid-round, a blow felling it as one waits)
const PCFELLED = ['d', 'wait', 'go right', 'inventory', 'wait 1 minute'];   // (dq0pcfelled's: dq0fightcmd's way to the rats, a rat engaged as inventory's typed, then a line typed at the prompt as it fights on, meeting the bite that fells the character; the game quit as that's told)
const FIGHTCMD = ['d', 'wait', 'go right', 'attack the rat', 'inventory', 'wait', 'look', 'wait', 'inventory', 'wait', 'look'];   // (dq0fightcmd's lines: dq0fight's way to the rats, the fight begun, a rat engaged as the first command after's typed, then commands typed as it fights, waits between for its blows: every one timed, gw3's)

// At each line's Enter, each actor's body (m.bodies: { g, line, name, den, lv: its levels, wfl, ex, at }) after the scene set then
// (each { g: the game, 1 on, line, n: its nth time typed in it, name: the actor's, lv: its levels, ex, at: its body's set: the
// referee's, a scene's sc.tire, its own the rest), taken as the scene's in place
function dq0BodyPoke(m, pokes) {
  const a = m.acia, read = a.read;
  let typed = '', g = 0, seen = {};
  m.bodies = [];
  a.read = (r, t) => {
    if (r === 0 && a.rdrf) {
      const c = a.rx;
      if (c === 13) {
        if (/^dq0 /.test(typed)) { ++g; seen = {}; }
        seen[typed] = (seen[typed] || 0) + 1;
        const h = m.dq0 && m.dq0.bank(0);
        if (h && g) {
          const line = typed, gn = g, nn = seen[typed], poked = new Set();
          const body = mem => (q, i) => i === 0 ? mem[q] : i === 1 ? mem[q + 1] : i === 2 ? mem[q + 2] | mem[q + 3] << 8 : (mem[q + 4] | mem[q + 5] << 8 | mem[q + 6] << 16 | mem[q + 7] << 24) >>> 0;
          const take = () => {                  // (Each actor's body as the line's scene left it)
            const h = m.dq0.bank(0), { L } = m.dq0.layout(), B = dq0Body(m), v = body(B.mem);
            for (let k = 0; k < L.nactors; k++) {
              const o = L.actor + k * L.actorSize;
              let n = '';
              for (let i = 0; h[o + L.aName + i] && i < 12; i++) n += String.fromCharCode(h[o + L.aName + i]);
              if (!n) continue;
              const den = h[o + L.aDen], q = B.at(den);
              m.bodies.push({ g: gn, line, name: n, den, lv: v(q, 0), wfl: v(q, 1), ex: v(q, 2), at: v(q, 3), poked: poked.has(k) });
            }
          };
          dq0SceneNow(m, (h, L, K, at, named, sc) => {
            const B = dq0Body(m), v = body(B.mem);
            for (let k = 0; k < L.nactors; k++) for (const p of pokes) if (p.g === gn && p.line === line && p.n === nn && named(k) && p.name === named(k)) {
              const q = B.at(h[at(k) + L.aDen]);
              sc.tire(k, { lv: p.lv !== undefined ? p.lv : v(q, 0), ex: p.ex !== undefined ? p.ex : v(q, 2), at: p.at !== undefined ? p.at : v(q, 3) });
              poked.add(k);
            }
          }, take, 'dq0BodyPoke\'s "' + line + '" #' + nn, true);
        }
        typed = '';
      } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
    }
    return read(r, t);
  };
}

module.exports = {
  tests: [
    {
      name: 'dq0harm', what: 'Dingus Quest Zero\'s climbing and harm (the referee\'s: climbing tests, RS_HARM\'s falling, Skills 8, and its injuries, Combat 13-14: HarnMaster\'s at this seed), by the world\'s events and the referee\'s summary: at seed 517 the kobold warren\'s gatehouse (a built stratum\'s floors flat), a ledge 10 feet up met; climbed (u, its words\' though the way out\'s in the room): the climb\'s test at its foot a critical failure, no hold found yet (nothing to fall: the soak\'s, 2026-10-19, "you fall 0 feet" no more), then up; u again the way out\'s, shut; then jumped down it, a fall of 10 feet onto paving, an injury (the character hurt), and kept in the save (pc.hm\'s inj line, hmchar\'s format); a creep, listening as it goes, kept with the sheet too (pc.hm\'s pace line, the referee\'s: dq0pace\'s before)',
      init: 't_rc', cycles: 600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 517 -c /ram/c.hm -d /ram/hurt', 'ne', 'e', 'u', 'u',
        'turn around', 'jump down', 'creep', 'listen as I go', 'quit', 'cat /ram/hurt/pc.hm', 'echo $status'] })) },
      start(m) {   // (Rigged, dice2: the climb's first test a critical failure at its foot; the fall's dice dq0tend's, a minor bruise)
        dq0Scene(m, [{ g: 1, line: 'u', n: 1, rig: [{ kind: 'TEST', a: 'P_CLIMB', actor: -1, uses: 1, grade: 'badly' }] },
          { g: 1, line: 'jump down', n: 1, rig: [{ kind: 'HARM', a: 'HC_FALL', uses: 1, faces: [1, 1, 92] }] }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], e = T.of('e'), u = T.of('u'), u2 = T.after(u, 'u'), j = T.of('jump down');
        dq0Need(f, e, 'E_LEDGE', { rise: 10 }, 'no ledge 10 feet up met');
        dq0Need(f, u, 'E_CLIMB', { how: 'K_BEGUN' }, 'the climb not begun');
        dq0Need(f, u, 'E_CLIMB', { how: 'K_NOHOLD' }, 'the critical failure at its foot not a hold not found');
        dq0Never(f, u, 'E_CLIMB', { how: 'K_FELL' }, 'a fall from the ledge\'s foot');
        dq0Never(f, u, 'E_HARM', {}, 'harm from the ledge\'s foot');
        dq0Need(f, u, 'E_CLIMB', { how: 'K_UP' }, 'not up the ledge');
        dq0Need(f, u2, 'E_NOWAY', { why: 'N_SHUT' }, 'the second u not the way out\'s, shut');
        dq0Need(f, j, 'E_CLIMB', { how: 'K_JUMPED', feet: 10 }, 'not jumped down 10 feet');
        dq0Need(f, j, 'E_HARM', { hc: 'HC_FALL', band: b => b > 0 }, 'the fall did no harm');
        if (!j.after || !(j.after.sum.health > 0)) f.push('the character not hurt by the fall');
        const pc = out.slice(out.indexOf('% cat /ram/hurt/pc.hm'));
        if (!/\ninj /.test(pc)) f.push('the injury not kept in the save (pc.hm\'s inj line)');
        if (!/\npace 0 2 /.test(pc)) f.push('the pace and watchfulness not kept with the sheet (pc.hm\'s pace line: a creep, listening)');
        // (A walk at a ledge not begun: its first words the ledge's, not "You set off east." the UI's at once,
        // 2026-11-17)
        if (/You set off east\.\s+The floor rises/.test(dq0Story(out))) f.push('a walk refused at a ledge told set off first');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rules', what: 'Dingus Quest Zero\'s referee\'s unit tests (Phase 3A: tests/ram/t_rules, its cases in rules.txt beside it): the HarnMaster referee (dq0harn) asked rs.h\'s requests by a program playing the world\'s part, its dice fixed (RS_DICE: dice.h\'s hook), each answer held to the book\'s rules and HOUSE.md\'s, and the dice each rolled to the book\'s: the tests\' grades at their edges (critical and marginal, the 5-95 clamp, the penalty each skill takes), noticing by pace and state, falls by height and ground (a landing on the feet, a hold grabbed), injuries by impact, aspect, location and armour (Combat 13-14), the kill, amputation, shock and stumble rolls, senselessness and coming round, bleeding, encumbrance by load, fatigue by exertion and rest, digging and appraising, the bearings underground (P_BEARING: Awareness, or Mining if higher, by the bends and the dark; one of the Khuzdul never tested; its own dice, the others rolling as they did)',
      init: 't_rc', cycles: 2300e6,
      pc: { files: () => ({ 't_rules.hyx': fs.readFileSync(path.join(TESTS, '..', 'obj', 'tests', 't_rules.hyx')),
        'rules.txt': fs.readFileSync(path.join(TESTS, 'ram', 't_rules', 'rules.txt')) }) },
      machine: { input: '\u0101cp /pc/t_rules.hyx /ram/t_rules.hyx; cp /pc/rules.txt /ram/rules.txt\r\u0101/ram/t_rules.hyx /ram/rules.txt\r' },
      expect: ['t_rules: PASS'],
      check(m, out) {
        const t = out.slice(out.indexOf('% /ram/t_rules.hyx'));
        this.notes = [(t.match(/^ok - /gm) || []).length + ' cases held'];
        return [...t.matchAll(/^not ok - ([^\r\n]*)/gm)].map(k => k[1]);
      },
      sdcard: true
    },
    {
      name: 'dq0nofoe', meaning: true, near: ['fight'], what: "Dingus Quest Zero's flee with no foe it sees (the user's, 2026-11-03, HOUSE.md's disengaging): at seed 3 by the rats, wary and let be: 'You see nothing to flee from.'; the rats made unfriendly to it, near and unseen in the dark: the same words (E_TASK's TE_FREE, no TE_BREAK), no run, nothing given away",
      init: 't_rc', cycles: 1200e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 3 -c /ram/c.hm -T', 'flee', 'douse the torch', 'flee', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'flee', n: 2, call: (h, L, K, at, named) => { m.near = 0; for (let k = 0; k < L.nactors; k++) if (named(k)) { h[at(k) + L.aAttitude] = 1; m.near++; } } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        for (const t of T.filter(t => t.line === 'flee')) dq0Never(f, t, 'E_TASK', { te: 'TE_BREAK' }, 'it ran');
        const n = (s.match(/You see nothing to flee from\./g) || []).length;
        if (n !== 2) f.push('not told in both: ' + n);
        if (!m.near) f.push('no one near to be unseen');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0shieldskill', outfit: true, asks: true, near: ['fight'], what: "Dingus Quest Zero's block improves the thing that blocked (the user's, 2026-11-03: \"A block improves Shield\"): dq0shield's bandit set on the character, sword and round shield held, its stance a block: the block by the shield earns Shield its rolls and Sword none of it; struck at, its own blow, Sword",
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml dge 5 >> /ram/c.hm', 'echo ml shd 90 >> /ram/c.hm',
        'echo ml swd 20 >> /ram/c.hm', 'echo hour 128 >> /ram/s.sc', 'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,round shield,gambeson,mail hauberk' -S /ram/s.sc", 'drop the backpack', 'hold the sword', 'hold the round shield', 'wear the gambeson', 'wear the mail hauberk',
        'defend block', ...Array(30).fill('wait'), 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [0, 1, 2].map(k => k ? { g: 1, line: 'wait', n: 1, k, set: { attitude: 4, doing: 0 } } : { g: 1, line: 'wait', n: 1, k, set: { attitude: 0, doing: 0 }, beside: 4,
          rig: [{ kind: 'ATTACK', actor: 0, uses: 1, faces: [1, 1] }] }));   // (Its first blow rigged, dice2: a marginal success, the shield's block one too: blocked)
        dq0Grow(m);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], SHD = 42, SWD = 45, end = m.grow[m.grow.length - 1], s = dq0Prose(out);
        this.notes = ['Shield ' + end.rolls(SHD) + ', Sword ' + end.rolls(SWD)];
        if (!/fend[^.]*(shield)/.test(s)) f.push('no block with the shield in its scene');
        else if (!end.rolls(SHD)) f.push('a shield\'s block earned Shield nothing');
        if (end.rolls(SWD) && !/You (strike|hit|counter)/.test(s)) f.push('Sword earned rolls with no use of the sword');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0faced', initiative: true, near: ['fight', 'denizens'], what: "Dingus Quest Zero's courage on facing the character (the user's, 2026-11-03: \"Also on first facing you\", \"Reckon it properly\": HOUSE.md's morale): a mighty character (Sword 100) and a giant rat, then a band: each as it first engages tests its courage (RS_MORALE, MR_FACED) with the character's might weighed against it: the steps against a beast by its size and the character's weapon skill, none against a thing as mighty; a band that breaks flees with no blow struck",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 100 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Vermin | rat | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'look', ...Array(14).fill('wait'), 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [...[0, 1, 2, 3].map(k => ({ g: 1, line: 'wait', n: 1, k, set: { attitude: 0, doing: 0 }, beside: [0, 2, 4, 6][k] })),
          // (Rigged, dice2's keyed dice: each courage tested on facing it a critical failure, its breakdown 60, broken: the
          // band flees with no blow struck, as before; the keyed dice had them stand, four felled in the wait's one answer)
          { g: 1, line: 'wait', n: 1, rig: [{ kind: 'MORALE', a: 'MR_FACED', faces: [100, 60] }] }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), C = require('../../../sim/lib/dq0meaning.js').constants().names, f = [], MR_FACED = 4;
        const seen = new Set(), tests = T.flatMap(t => dq0Rep(t, 'dslot', { kind: C.RS_MORALE })).filter(r => r.a === MR_FACED).filter(r => { const k = JSON.stringify(r); return !seen.has(k) && seen.add(k); });
        this.notes = [tests.map(r => r.actor + ':' + r.steps + ':' + r.result).join(' ')];
        if (!tests.length) f.push('no test of courage as it first engaged (MR_FACED)');
        if (tests.some(r => (r.b << 24 >> 24) >= 0)) f.push('the character\'s might not weighing on a rat: ' + tests.map(r => r.b));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dulled', outfit: true, near: ['fight', 'gear'], what: "Dingus Quest Zero's worn blades (the user's, 2026-11-03: \"both: dull edges cut less, and weapons can break by the book\": HOUSE.md's what strikes, a whetstone): a sword's edge impact as a blow's struck, by the referee's side of the fight: keen 5; worn dull (wear 200, 'its edge dull') 3; a whetstone used in the fight, its wear halved (dulled) 4; sharpened again, keen 5, the rat at it all the while",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 95 >> /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Vermin | rat | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k sword,whetstone -S /ram/s.sc', 'put the whetstone in the backpack', 'drop the torch', 'hold the sword', 'look', 'attack the giant rat', 'wait',
        'drop the sword', 'take the sword', 'attack the giant rat', 'wait', 'sharpen the sword', 'attack the giant rat', 'wait', 'sharpen the sword', 'sharpen the sword', 'sharpen the sword', 'attack the giant rat', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const ch = n => [0, 1, 2, 3].map(k => ({ g: 1, line: 'attack the giant rat', n, k, set: { attitude: 0, doing: 1 }, beside: 4 }));
        dq0Scene(m, [...ch(1), { g: 1, line: 'drop the sword', n: 1, wear: { sword: 200 } }, ...ch(2), ...ch(3), ...ch(4)]);
        const a = m.acia, read = a.read;
        let typed = '';
        m.edges = [];
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              const h = m.dq0 && m.dq0.bank(0);
              if (h) {
                const lbl = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0harn', 'dq0harn.lbl'), 'latin1'), at = parseInt(/^al ([0-9A-F]+) \._A$/m.exec(lbl)[1], 16), ram = m.taskRam[h[12 + 3]];
                const at2 = parseInt(/^al ([0-9A-F]+) \._D$/m.exec(lbl)[1], 16), s = ram[at + 2] ? at2 : at;   // (Whichever side's the character's: who, 0)
                m.edges.push({ line: typed, edge: ram[s + 11] << 24 >> 24 });
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), e = m.edges.filter(x => x.line === 'wait').map(x => x.edge);
        this.notes = ['the edge after each blow: ' + e.join(', ')];
        if (e[0] !== 5) f.push('a keen sword\'s edge not 5: ' + e);
        if (e[1] !== 3) f.push('a dull sword\'s edge not 3: ' + e);
        if (e[2] !== 4) f.push('sharpened once (dulled), its edge not 4: ' + e);
        if (e[3] !== 5) f.push('sharpened till keen, its edge not 5: ' + e);
        if (!/You put a fresh edge on the sword\./.test(s)) f.push('its sharpening not told');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0broke', outfit: true, asks: true, initiative: true, near: ['fight', 'gear'], what: "Dingus Quest Zero's weapons broken by the book (the user's, 2026-11-03; HOUSE.md's what strikes: Combat 12's Weapon Damage, 3d6 over its Weapon Quality, on a Block), every actor's: dq0shield's bandit set on the character, sword in hand, its stance a block: a block, weapon against weapon, breaks one of them: told, its pieces a 'broken weapon' on the floor, none of it in hand after; the character's next blow by its fists, or the bandit's",
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml dge 5 >> /ram/c.hm', 'echo ml swd 80 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo hour 128 >> /ram/s.sc', 'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk' -S /ram/s.sc", 'drop the backpack', 'hold the sword', 'wear the gambeson', 'wear the mail hauberk', 'defend block', ...Array(30).fill('wait'), 'inventory', 'look', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [0, 1, 2].map(k => k ? { g: 1, line: 'wait', n: 1, k, set: { attitude: 4, doing: 0 } } : { g: 1, line: 'wait', n: 1, k, set: { attitude: 0, doing: 0 }, beside: 4 }));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out).replace(/\s+/g, ' '), broke = /(Your|The bandit's) ([a-z ]+) breaks!/.exec(s);
        this.notes = [broke ? broke[0] : 'none broke', s.slice(s.lastIndexOf('> inventory')).slice(0, 500)];
        if (!broke) return ['no weapon broke in thirty rounds of blocks'];
        const last = s.slice(s.lastIndexOf('> inventory'));
        if (broke[1] === 'Your' && new RegExp('a ' + broke[2] + ', in your').test(last)) f.push('its pieces still in hand as a ' + broke[2]);
        const T = dq0Turns(m), st = T.of('look').after;
        if (!st || !st.items.some(i => i.name === 'broken weapon')) f.push('no pieces where it fell: ' + (st ? st.items.filter(i => i.where === 'IW_SQUARE').map(i => i.name).join(', ') : ''));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hardblow', initiative: true, near: ['fight', 'gear'], what: "Dingus Quest Zero's weapon broken on something hard (the user's, 2026-11-03; HOUSE.md's what strikes: Combat 12's Weapon Damage): a crude sword (quality 0, the book's table's four under its 12) battering a lowered portcullis (iron): three dice over its quality and it breaks, told, its pieces let fall, no more of it in hand; and a door of wood, no such test",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 514 -c /ram/c.hm -k sword', 'drop the sword', 'take the sword', 'lower portcullis', 'hit the portcullis with the sword', 'inventory', 'look', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'drop the sword', n: 1, item: { sword: { quality: 0 } } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out).replace(/\s+/g, ' '), i = s.lastIndexOf('> inventory'), inv = s.slice(i, s.indexOf('> look', i));
        this.notes = [s.slice(s.indexOf('> hit the portcullis'), s.indexOf('> hit the portcullis') + 400)];
        if (!/Your sword breaks!/.test(s)) f.push('the crude sword not broken on the iron');
        if (/a sword/.test(inv)) f.push('the sword still carried: ' + inv);
        if (!/broken weapon/.test(s.slice(i))) f.push('no pieces where it fell');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0atbay', initiative: true, near: ['fight', 'denizens'], what: "Dingus Quest Zero's denizen broken and cornered beside the character (the life soak's seed 12, 2026-11-04: two shy rats far from their lair, no step of theirs allowed, each deciding again at once with no time gone: 192 tests of courage a round and no blow, the fight's rounds 66M cycles each, so \"hung\"; seed 30 the same, its budget gone before its next game's start): a giant rat beside it, fleeing, its home far off (the mind's own: no step away): at bay it bites, its courage not asked again (DENIZENS.md's morale)",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml dge 95 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Vermin | rat | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'look', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        const lbl = n => dq0Ad(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1'))(n);
        const broken = (h, L, K, at) => {
          const ram = m.taskRam[h[12 + 2]];
          dq0Alone(m, h, L, K, at, 0);
          ram[lbl('fled')] = 40;                // (Its mind's: broken, fleeing, these many turns)
          for (const a of [lbl('ahx'), lbl('ahy')]) { ram[a] = 1; ram[a + 1] = 0; }   // (Its home far off: no step within its reach)
        };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, k: 0, beside: 2, engage: true }, { g: 1, line: 'wait', n: 1, call: broken },
          { g: 1, line: 'wait', n: 2, call: h => { this.left = m.taskRam[h[12 + 2]][lbl('fled')]; } }]);
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), C = require('../../../sim/lib/dq0meaning.js').constants().names, f = [], waits = T.filter(t => t.line === 'wait');
        const tests = waits.flatMap(t => dq0Rep(t, 'dslot', { kind: C.RS_MORALE })).filter(r => r.actor === 1), blows = waits.flatMap(t => t.dens).filter(e => e.kind === 'E_FIGHT' && e.def === 0 && e.who === 1);
        this.notes = ['tests of its courage ' + tests.length + ', its blows ' + blows.length + ', its turns of flight left of 40 after them: ' + this.left];
        if (this.left === undefined) f.push('the second wait not given: the scene not played');
        else if (this.left < 40 - blows.length - 4) f.push('its turns of flight gone with no time gone (decided again at once): ' + this.left + ' of 40 left, ' + blows.length + ' blows');
        if (tests.length) f.push('fleeing, its courage tested again: ' + tests.length);
        if (blows.length < 2) f.push('cornered, too few blows at bay: ' + blows.length);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fighttired', asks: true, near: ['fight', 'body'], what: "Dingus Quest Zero's fight tiring the character at the turns pace (HOUSE.md's fighting on: a level more asked as a hurt is; asked only at the real paces till 2026-11-04, the world's task_word turning it away, so a long fight at the turns pace asked nothing and never gave the prompt back): a character a round short of its next level of fatigue attacks a giant rat: \"You're tiring.  Keep fighting?\", the clock still, and \"no\" ends the fight",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml dge 95 >> /ram/c.hm', 'echo pace 2 0 5990 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Vermin | rat | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'look', 'attack the rat', 'no', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'attack the rat', n: 1, call: (h, L, K, at) => dq0Alone(m, h, L, K, at, 0) }, { g: 1, line: 'attack the rat', n: 1, k: 0, beside: 'face', engage: true }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out).replace(/\s+/g, ' ');
        this.notes = [s.slice(s.indexOf('> attack the rat'), s.indexOf('> attack the rat') + 500)];
        const at = T.filter(t => t.line === 'attack the rat').flatMap(t => t.events).filter(e => e.kind === 'E_TASK');
        if (!at.some(e => e.te === 'TE_ASK_TIRED' && e.task === 'LT_FIGHT')) f.push('tired a level more as it fought, not asked: ' + at.map(e => e.te).join(' '));
        if (!/You're tiring\. Keep fighting\?/.test(s)) f.push('the question not told');
        const after = T.find(t => t.line === 'quit');
        if (!T.some(t => t.line === 'no') || !after) f.push('its answer not taken at a prompt');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0nolegs', near: ['fight', 'body'], what: "Dingus Quest Zero's character down with no leg to stand on (HOUSE.md's prone, 2026-11-04, for review): \"attack\" takes its round and tells that its legs won't bear it, the fight over and the prompt back, a blow never to come (it struggled on each round before, for ever if nothing killed it); the player's to say what next",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'echo inj 12 0 3 4 0 8 >> /ram/c.hm', 'echo inj 140 0 3 4 0 8 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc',
        "echo 'faction | Vermin | rat | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc', 'look', 'lie down', 'attack the rat', 'crawl north', 'quit', 'echo $status']) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'attack the rat', n: 1, call: (h, L, K, at) => dq0Alone(m, h, L, K, at, 0) }, { g: 1, line: 'attack the rat', n: 1, k: 0, beside: 'face', engage: true }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], t = T.find(x => x.line === 'attack the rat'), s = dq0Prose(out).replace(/\s+/g, ' ');
        if (!t) return ['its attack not given'];
        const K = m.dq0.layout().K, rises = t.replies.filter(r => r.kind === K.RS_ATTACK && r.actor === 0);
        if (rises.length !== 1) f.push('rounds spent trying to rise: ' + rises.length + ', not 1');
        if (!/Your legs won't bear you\./.test(s)) f.push("not told its legs won't bear it");
        if (/You struggle, but can't get up/.test(s)) f.push('told it struggles, as if it might');
        if (!T.some(x => x.line === 'crawl north')) f.push('no prompt after it');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0downonce', near: ['fight'], what: "Dingus Quest Zero's creature down, its failed tries to rise told once (2026-11-02, the lead's, a fault found in dq0antidote's scene: \"The cave adder writhes, but can't right itself.\" four times in one answer): dq0venom's hostile cave adders (seed 1, the character tough), two set on the character where it stands, fighting it, waiting beside them, prone, hurt and Agility 1 (the scene's, 2026-11-03, dq0Poke's lame: a Rise is tested only of one hurt); a creature that fails its Rise (the referee's O_RISE, its round spent as ever) told at its first failure (E_BEAT WB_NOT_UP), not again till something changes: it rises, falls again or is wounded",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Serpents | adder | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k antidote,remedy', 'stow the antidote', 'drop the torch', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {        // (The scene set, 2026-11-03: their way to it and a fallen one's hurt the dice's no longer)
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, k: 1, beside: 6, engage: true }, { g: 1, line: 'wait', n: 1, k: 0, beside: 4, engage: true }, { g: 1, line: 'wait', n: 1, k: 0, lame: true }, { g: 1, line: 'wait', n: 1, k: 1, lame: true }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), told = {};
        let tries = 0;
        for (const t of T) for (const e of t.events.concat(t.dens).sort((a, b) => a.t - b.t)) {
          if (e.kind === 'E_FIGHT') {
            told[e.def] = 0;                  // (A blow at it: its wound changes it)
            if (e.out === 'O_RISE') told[e.who] = 0;    // (Risen, in a fight: the referee's own, no beat's)
          }
          if (e.kind !== 'E_BEAT' || !e.b) continue;
          if (e.beat === 'WB_RISEN' || e.beat === 'WB_FALLEN') told[e.b] = 0;
          if (e.beat === 'WB_NOT_UP') {
            ++tries;
            if (told[e.b]) f.push('actor ' + e.b + ' told failing to rise again with nothing changed, at ' + e.t);
            told[e.b] = 1;
          }
        }
        if (!tries || !/writhes, but can't right itself\./.test(s)) f.push('no adder failed to rise: the scene not set');
        const K = m.dq0.layout().K, spent = T.flatMap(t => t.replies).filter(r => r.kind === K.RS_ATTACK && r.result === K.O_RISE && r.a).length;   // (Each a round spent)
        if (spent <= tries) f.push('no failed rise left untold: ' + spent + ' failed, ' + tries + ' told');
        this.notes = [spent + ' rises failed, ' + tries + ' told'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0break', what: 'Dingus Quest Zero\'s portcullises and windows broken (the user\'s, 2026-10-16; DUNGEON.md: portals), by the world\'s events (the referee\'s blows and Strength: HarnMaster\'s, at these seeds): at seed 514\'s gatehouse a portcullis lowered by its winch, then battered with a pick (dq0 -k pick: blows at its iron bars off their body, as a door\'s; its torch put down first, its hands empty, the pick offered and taken, dq0 -a\'s yes) till they break, a gap through for good (battered again, jammed; examined; it won\'t lower), and gone through; then a strong character (Strength 18) bending its bars apart (P_FORCE), and through; then at seed 12 a glazed window not climbed through while its glass is in it, the glass broken (the referee\'s blow at a thing, HC_BLOW at RS_THING), climbed through (its sill, P_CLIMB) into the passage beyond, the window seen from there, its glass gone (dq0portcbreak\'s and dq0window\'s, these together)',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 514 -c /ram/c.hm -k pick', 'lower portcullis',
        'drop the torch', 'break portcullis', 'break', 'x portcullis', 'lower portcullis', 'go through the portcullis', 'quit', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/s.hm', 'echo att str 18 >> /ram/s.hm',
        'dq0 -p t -s 514 -c /ram/s.hm', 'lower portcullis', 'bend the bars', 'bend bars', 'go through the portcullis', 'quit',
        'dq0 -p t -s 12 -c /ram/c.hm', 'go through the door to the north', 'turn around', 'climb through window', 'break window', 'climb through window', 'turn around', 'quit',
        'echo $status'] })) },
      start(m) {        // (Its sill climbed: the climb's test a pass, dice2's keyed dice had it fail)
        dq0Scene(m, [{ g: 3, line: 'climb through window', n: 2, rig: [{ kind: 'TEST', a: 'P_CLIMB', actor: -1, uses: 1, grade: 'pass' }] }]);
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], g1 = T.of('dq0 -p t -s 514 -c /ram/c.hm -k pick'), g2 = T.of('dq0 -p t -s 514 -c /ram/s.hm');
        const lo = T.after(g1, 'lower portcullis'), b = T.of('break portcullis'), b2 = T.of('break'), x = T.of('x portcullis'), lo2 = T.after(x, 'lower portcullis');
        dq0Need(f, lo, 'E_PORTAL', { po: 'PO_LOWERED' }, 'the portcullis not lowered');
        dq0Need(f, b, 'E_PORTAL', { po: 'PO_GAP' }, 'its bars not broken through with the pick');
        dq0Need(f, b2, 'E_PORTAL', { po: 'PO_JAMMED' }, 'battered again, not jammed for good');
        if (!dq0Rep(x, 'wreq', { kind: 'WQ_EXAMINE', res: 0 }).length) f.push('the portcullis not examined');
        dq0Need(f, lo2, 'E_PORTAL', { po: 'PO_JAMMED' }, 'lowered again, not jammed');
        dq0Need(f, T.after(lo2, 'go through the portcullis'), 'E_ARRIVE', {}, 'not through the gap');
        const lo3 = T.after(g2, 'lower portcullis'), bend = [T.of('bend the bars'), T.of('bend bars')];
        dq0Need(f, lo3, 'E_PORTAL', { po: 'PO_LOWERED' }, 'the portcullis not lowered (the strong character)');
        if (!bend.some(t => dq0Ev(t, 'E_PORTAL', { po: 'PO_BENT' }).length)) f.push('its bars not bent apart (' + bend.map(dq0Evs).join('; ') + ')');
        dq0Need(f, T.after(bend[1], 'go through the portcullis'), 'E_ARRIVE', {}, 'not through the bent bars');
        const g3 = T.of('dq0 -p t -s 12 -c /ram/c.hm'), c1 = T.after(g3, 'climb through window'), bw = T.of('break window'), c2 = T.after(bw, 'climb through window');
        dq0Need(f, c1, 'E_PORTAL', { po: 'PO_IN_WAY', pk: 'PK_WINDOW' }, 'climbed through the glass');
        dq0Need(f, bw, 'E_PORTAL', { po: 'PO_SHATTERED', pk: 'PK_WINDOW' }, 'the glass not broken');
        dq0Need(f, c2, 'E_PORTAL', { po: 'PO_CLIMBED' }, 'not climbed through the window');
        dq0Need(f, c2, 'E_ARRIVE', { how: 'A_CLIMBED' }, 'not come through the window');
        const ta = T.after(c2, 'turn around');
        if (!ta.after || !ta.after.sight || ta.after.sight.where !== 1 || !ta.after.sight.doors.some(d => d.kind === 'PK_WINDOW' && !(d.parts & 1)))
          f.push('the window, its glass gone, not seen from the passage beyond');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0batter', what: 'Dingus Quest Zero\'s doors battered (the user\'s, 2026-10-15 and 16: "model it after unarmed combat": a blow at a thing an attack it ignores, by Unarmed, its impact less the door\'s resistance off its body; the striker hurt as a blow goes wrong, less with skill and armour; boots for a kick, a helm for a head butt; the tool to hand that hits hardest; the user\'s, 2026-10-22: a part\'s blow the house\'s Unarmed test, a marginal failure harming neither, a critical failure the part, and a bare part\'s first hurt at a door stopped short, asked), by the world\'s events (the referee\'s, HarnMaster\'s, at these seeds; each game\'s first blow the same roll): at seed 14\'s sound iron-bound door, bare fists making no mark, the first blow that would hurt them stopped short (asked: "Keep at it?", dq0 -a\'s yes), unhurt; punched again, not asked again at that door, and by the third the hand hurt seriously, the blows stopped as the character\'s too hurt; a kick in knee boots and a head butt in an iron helm not stopped short (not bare), hurt by their blows gone wrong; at seed 23 (seed 19\'s rotten door till 2026-11-01, its stratum\'s passages on the diagonal since) a door one of Unarmed 95 (hmchar\'s file) breaks down unhurt; at seed 15 a pick battering a door till it gives way (its torch put down first: its hands empty, the pick offered and taken, dq0 -a\'s yes, the user\'s, 2026-10-22), broken down for good: it won\'t shut (dq0hands\' and dq0kick\'s, and dq0doors\' blows, these together; the injuries\' words the referee\'s)',
      init: 't_rc', cycles: 2300e6,   // (1,393M at slice 4.2's travel, more to start a game since: the world's 16th overlay read in; past 1,500M
                                     // at slice 4.3's, its overlays more: the UI's 15th, the world's 20th and 21st, the referee's 6th)
      machine: { input: dq0Typed(dq0Game({ lines: ['hmchar -q -s 5 -u feudal -c hunter -o /ram/u.hm', 'echo ml una 95 >> /ram/u.hm',
        'dq0 -p t -s 14 -c /ram/c.hm', 'punch the door', 'punch the door', 'punch the door', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -k \'knee boots\'', 'wear the knee boots', 'kick the door', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm -k \'iron helm\'', 'wear the iron helm', 'head butt the door', 'quit', 'dq0 -p t -s 23 -c /ram/u.hm', 'punch the door', 'quit',
        'dq0 -p t -s 15 -c /ram/c.hm -k pick', 'drop the torch', 'break the door ahead to my left', 'close', 'quit', 'echo $status'] })) },
      start(m) {        // (Rigged, dice2's keyed dice: the first punching a critical failure stopped short, then marginal failures,
                        //   no mark, the second's too; the third's first blow gone wrong, a serious hurt; the head butt's first gone wrong)
        dq0Scene(m, [{ g: 1, line: 'punch the door', n: 1, rig: [{ kind: 'ATTACK', actor: -1, uses: 1, grade: 'badly' }, { kind: 'ATTACK', actor: -1, grade: 'fail' }] },
          { g: 1, line: 'punch the door', n: 3, rig: [{ kind: 'ATTACK', actor: -1, uses: 1, faces: [100, 2, 2] }] },
          { g: 3, line: 'head butt the door', n: 1, rig: [{ kind: 'ATTACK', actor: -1, uses: 1, faces: [100, 4, 4] }] }]);
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], hurt = t => Math.max(0, ...dq0Ev(t, 'E_HARM', { hc: 'HC_STRUCK' }).map(e => e.band));
        const p = T.of('punch the door'), p2 = T.after(p, 'punch the door'), p3 = T.after(p2, 'punch the door'), k = T.of('kick the door');
        const h = T.of('head butt the door'), u = T.after(p3, 'punch the door'), short = { te: 'TE_ASK_SHORT', task: 'LT_BATTER' };
        dq0Need(f, p, 'E_PORTAL', { po: 'PO_NO_MARK' }, 'bare fists marked the sound door');
        dq0Need(f, p, 'E_TASK', short, 'the bare fists\' blow that would hurt them not stopped short, asked');
        if (hurt(p)) f.push('the bare hand hurt, stopped short of it (' + dq0Evs(p) + ')');
        for (const t of [p2, p3]) dq0Never(f, t, 'E_TASK', short, 'asked again at the same door');
        if (hurt(p3) < 2) f.push('the bare hand not hurt seriously by the third (' + dq0Evs(p3) + ')');
        dq0Need(f, p3, 'E_HARM', { hc: 'HC_STRUCK', stopped: true }, 'the blows not stopped, the character too hurt');
        for (const [t, w] of [[k, 'the booted foot'], [h, 'the helmed head']]) {
          dq0Never(f, t, 'E_TASK', short, w + ' stopped short (it isn\'t bare)');
          if (hurt(t) < 1) f.push(w + ' not hurt by its blow gone wrong (' + dq0Evs(t) + ')');
        }
        dq0Need(f, u, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'the skilled fists didn\'t break the rotten door down');
        if (hurt(u)) f.push('the skilled fists hurt');
        const b = T.of('break the door ahead to my left');
        dq0Need(f, b, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'the pick didn\'t break the door down');
        dq0Need(f, T.after(b, 'close'), 'E_PORTAL', { po: 'PO_WRECKED' }, 'the broken door shut');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0battering', what: 'Dingus Quest Zero\'s battering a long task (slice 4.0, HOUSE.md\'s long tasks: a blow at a time as the clock goes, its bar the blows struck, stopped by the player or hard, the harm done kept), by the world\'s events and the long task\'s part done as it went (the hub\'s pc_task: the referee\'s, HarnMaster\'s, at these seeds): at seed 14 a pick battering the sound door (its torch put down and the pick taken in hand first: what\'s in its hands strikes, the user\'s, 2026-10-22) a dozen blows, one at a time (LT_BATTER, its length a dozen), leaving it giving, and battered again, broken down; then the same door battered and "stop digging" refused as it goes (it\'s a battering), "stop battering" stopping it part way, how the door stands told, and battered again, broken down in as many blows in all as without the stop (the harm done kept, each blow its own roll as before); with the hand-written theme\'s hostile rats (inhabit.txt) a battering stopped hard by a rat\'s blow ("break the door ahead with the pick": the pick taken in hand by the battering\'s command, the rats coming as it batters); told as it goes (the user\'s, 2026-10-22: "nothing is written to the output until after it is resolved"): its first words at once (E_TASK TE_BEGUN, its what the door and what strikes it), a beat as its blows first tell (WB_TELLS) and as they leave it giving (WB_GIVES); both its ends (the user\'s: "doesn\'t seem to stop kicking a door once started"): the dozenth blow (above), and the fourth that makes no mark (seed 14\'s door punched bare-handed: four blows, no more, its bare fist stopped short once and not again at that door, and punched again: its fist hurt on it, the blows ending there, the door held (its load within its free pounds by Strength since D13\'s, a step better: it marked the door this time, and four blows with no mark were no longer the end)); a kick in knee boots ending of itself (no question: not bare), and a bare-footed kick at seed 15\'s door stopped short once and breaking it down',
      init: 't_rc', cycles: 2800e6,   // (Past 1,900M at slice 4.3's: its hands hurt, the referee told each minute of it, and the overlays more)
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 14 -c /ram/c.hm -k pick', 'drop the torch', 'hold the pick', 'break the door', 'break', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -k pick',
        'drop the torch', 'hold the pick', 'break the door'] })) +
        'stop digging\rstop battering\r' + dq0Typed(['break', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k pick', 'drop the torch', 'break the door ahead with the pick', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm', 'punch the door', 'punch the door', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -k \'knee boots\'', 'wear the knee boots',
        'kick the door', 'quit', 'dq0 -p t -s 15 -c /ram/c.hm', 'kick the door ahead to my left', 'quit', 'echo $status']) },
      start(m) {
        // (The stop typed as the blows are told, not at a fixed gap after "break the door" (2026-11-13: the gap's
        // blows as the timing had them, the stop a blow either side of the door's giving): the lines after it held
        // till "You keep at it." (its fourth blow's words), then typed as they were)
        const a = m.acia, read = a.read;
        let g = 0, typed = '', held = null;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (/^dq0 /.test(typed)) ++g;
              if (g === 2 && typed === 'break the door' && !held) held = a.rxQueue.splice(0);
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
        dq0OnWords(m, [{ g: 2, line: 'break the door', n: 1, words: 'You keep at it.', call: () => { a.rxQueue.unshift(...held); a.promptFrom = -1; } }]);
        // (Rigged, dice2's keyed dice a blow's by its moment: the pick's every blow the same, a marginal success and
        // its three dice 5, its point's 5 past the door's 5: 5 a blow, a dozen leaving it giving, a few more breaking
        // it, so the stop's run and the one without it are alike blow for blow; the first punching a critical failure
        // stopped short then marginal failures, no mark; the second a blow that marks it, then one gone wrong, a
        // serious hurt; the bare-footed kick's first gone wrong, stopped short)
        const pick = [{ kind: 'ATTACK', actor: -1, faces: [1, 1, 1, 3] }];
        dq0Scene(m, [{ g: 1, line: 'break the door', n: 1, rig: pick }, { g: 2, line: 'break the door', n: 1, rig: pick },
          { g: 4, line: 'punch the door', n: 1, rig: [{ kind: 'ATTACK', actor: -1, uses: 1, grade: 'badly' }, { kind: 'ATTACK', actor: -1, grade: 'fail' }] },
          { g: 4, line: 'punch the door', n: 2, rig: [{ kind: 'ATTACK', actor: -1, uses: 1, faces: [1, 3, 3, 3] }, { kind: 'ATTACK', actor: -1, uses: 1, faces: [100, 2, 2] }] },
          { g: 6, line: 'kick the door ahead to my left', n: 1, rig: [{ kind: 'ATTACK', actor: -1, uses: 1, grade: 'badly' }] }]);
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], tasks = m.dq0.games.flatMap(g => g.tasks);
        // (A run's part done as it went, from turn t's line to turn u's: the blows struck, one at a time from none)
        const run = (t, u, what) => {
          const d = tasks.filter(e => e.kind === 'LT_BATTER' && e.line > t.k && e.line <= u.k).map(e => e.done);
          if (!d.length || !d.every((v, i) => v === i)) f.push(what + ': its part done not the blows struck one at a time (' + d.join(' ') + ')');
          return d.length - 1;
        };
        const g1 = T.of('dq0 -p t -s 14 -c /ram/c.hm -k pick'), b1 = T.after(g1, 'break the door'), r1 = T.after(b1, 'break'), q1 = T.after(r1, 'quit');
        const n1 = run(b1, r1, 'the whole run'), m1 = run(r1, q1, 'the run after it');
        if (n1 !== 12) f.push('the whole run not a dozen blows: ' + n1);
        const first = dq0Ev(b1, 'E_TASK', { te: 'TE_BEGUN', task: 'LT_BATTER' })[0];
        if (!first || first !== b1.events.find(e => e.kind !== 'E_VIEW') || first.what !== (1 | 0x40)) f.push('the battering\'s first words not at once, at the door, with the pick (' + dq0Evs(b1) + ')');
        for (const beat of ['WB_TELLS', 'WB_GIVES']) dq0Need(f, b1, 'E_BEAT', { beat }, 'no ' + beat + ' as it went');
        if (!tasks.some(e => e.kind === 'LT_BATTER' && e.len === 12)) f.push('the long task\'s length not a dozen blows');
        dq0Need(f, b1, 'E_PORTAL', { po: 'PO_GIVING' }, 'the pick\'s dozen blows didn\'t leave the door giving');
        dq0Need(f, r1, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'battered again, the door not broken down');
        const g2 = T.after(q1, 'dq0 -p t -s 14 -c /ram/c.hm -k pick'), b2 = T.after(g2, 'break the door'), sd = T.after(b2, 'stop digging');
        const sb = T.after(b2, 'stop battering'), r2 = T.after(sb, 'break'), q2 = T.after(r2, 'quit');
        dq0Need(f, T.and(sd), 'E_TASK', { te: 'TE_NOT', task: 'LT_BATTER', named: 'LT_DIG' }, '"stop digging" not refused as it battered');
        dq0Need(f, sb, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_BATTER' }, '"stop battering" didn\'t stop it');
        const n2 = run(b2, r2, 'the run stopped'), m2 = run(r2, q2, 'the run after the stop');
        if (!(n2 >= 1 && n2 < 12)) f.push('not stopped part way: ' + n2 + ' blows');
        dq0Need(f, sb, 'E_PORTAL', { po: p => p === 'PO_HOLDING' || p === 'PO_GIVING' }, 'how the door stood not told as it stopped');
        dq0Need(f, r2, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'battered again after the stop, the door not broken down');
        if (n2 + m2 !== n1 + m1) f.push('the harm done before the stop not kept: ' + n2 + ' blows, then ' + m2 + ', not ' + (n1 + m1) + ' in all as without it');
        const b3 = T.after(T.of('dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k pick'), 'break the door ahead with the pick');
        dq0Need(f, b3, 'E_TASK', { te: 'TE_HIT', task: 'LT_BATTER' }, 'the battering not stopped hard by the rat\'s blow');
        // (Both ends, and each kick ending of itself: the user's, 2026-10-22)
        const short = { te: 'TE_ASK_SHORT', task: 'LT_BATTER' }, g4 = T.of('dq0 -p t -s 14 -c /ram/c.hm'), p1 = T.after(g4, 'punch the door');
        const p2 = T.after(p1, 'punch the door'), q4 = T.after(p2, 'quit');
        for (const [a, b, w] of [[p1, p2, 'the first punching']]) {
          const n = run(a, b, w);
          if (n !== 4) f.push(w + ': given up not at the fourth blow with no mark: ' + n);
          dq0Need(f, a, 'E_PORTAL', { po: 'PO_NO_MARK' }, w + ': its blows\' marks not told as none');
          dq0Need(f, a, 'E_READY', {}, w + ': the character not free again');
        }
        run(p2, q4, 'the second');                  // (The second: its fist hurt, the blows ended, the door held)
        dq0Need(f, p2, 'E_HARM', { hc: 'HC_STRUCK' }, 'the second: its fist not hurt on the door');
        dq0Need(f, p2, 'E_PORTAL', { po: 'PO_HOLDING' }, 'the second: the door not told as holding');
        dq0Need(f, p2, 'E_READY', {}, 'the second: the character not free again');
        dq0Need(f, p1, 'E_TASK', short, 'the bare fist\'s blow that would hurt it not stopped short');
        dq0Never(f, p2, 'E_TASK', short, 'asked again at the same door');
        if (!dq0Ev(p1, 'E_TASK', { te: 'TE_BEGUN' }).some(e => (e.what >> 4 & 3) === 1)) f.push('the punching\'s first words not the fist\'s');
        const k1 = T.after(T.of('dq0 -p t -s 14 -c /ram/c.hm -k \'knee boots\''), 'kick the door'), k2 = T.after(T.of('dq0 -p t -s 15 -c /ram/c.hm'), 'kick the door ahead to my left');
        const nk1 = run(k1, T.after(k1, 'quit'), 'the booted kicking');
        if (!(nk1 >= 1 && nk1 <= 12)) f.push('the booted kicking not ended of itself: ' + nk1 + ' blows');
        dq0Need(f, k1, 'E_PORTAL', { po: p => /^PO_(NO_MARK|HOLDING|GIVING|BROKE_DOWN)$/.test(p) }, 'how the door stood not told as the kicking ended');
        dq0Never(f, k1, 'E_TASK', short, 'a booted foot stopped short');
        dq0Need(f, k2, 'E_TASK', short, 'the bare foot\'s blow that would hurt it not stopped short');
        dq0Need(f, k2, 'E_PORTAL', { po: 'PO_BROKE_DOWN' }, 'the bare-footed kicking didn\'t break seed 15\'s door down');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fight', what: 'Dingus Quest Zero\'s fights (DENIZENS.md: fights; the referee\'s melee, RS_ATTACK, dq0harn/fight.c), by the blows told (the denizens\' events, E_FIGHT: who struck whom, how it came out, what it left them in; the referee\'s dice the commands\', in their order, at the turns pace: HarnMaster\'s at this seed): a fight with seed 5\'s giant rats won (down its ladder, a sword in hand, dq0 -k sword; the character\'s Sword and Dodge 95, its Stamina and Will 99, its file\'s), one attack fought on (HOUSE.md\'s fighting on, the user\'s, 2026-10-20): the rat struck at, and its lair\'s kin, turn on the character, both striking at it; blows at it till one knocks it senseless (the book\'s rat dodges at 90, 2026-11-04: both set hurt, dq0Poke\'s, so a blow lands); then the other turned to and felled, the fight over; the senseless one finished where it lies, an attack, the next attack finding none left to fight; their bodies left (seen as the world looks about again); the character hurt by their blows (since fighting on, the blows typed one by one before).  The scene set (since morale, 2026-10-29, and the dice by who asks, 2026-10-30, a kin seeing its fellow fall runs off, as a rule): the kin steady (dq0Poke\'s, at the attack: its mood AM_STEADY, its courage never tested), so the fight goes on to it; and the game played again as it stands, the kin running off as its fellow falls senseless beside it (part 6\'s dismay: no blow from it after, none at it, the fight\'s end told), morale\'s own small check',
      init: 't_rc', cycles: 1300e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 95 >> /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -k sword -T',
        'd', 'wait', 'go right', 'attack the rat', 'attack the rat', 'attack the rat', 'attack the rat', 'look', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T', 'd', 'wait', 'go right', 'attack the rat', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'attack the rat', n: 1, k: 0, set: { mood: 8 } }, ...[1, 2].flatMap(g => [0, 1, 2, 3].map(k => ({ g, line: 'attack the rat', n: 1, k, hurt: 8 }))),   // (The kin steady: hub.h's AM_STEADY; the book's rat dodges at 90, 2026-11-04: all set hurt, so a blow lands and their courage is shaken)
          // (Rigged, dice2's keyed dice felling both rats at the second blow, one by its bite met by the sword: the fight
          // fought on as before, the character's first three blows dodged (both a marginal success), the next two a
          // hit of one die, a 1 (felled, not killed); the rats' first bite a hit (it's hurt), the rest marginal failures
          // the character meets (no bite blocked by the sword, which wounds the rat))
          { g: 1, line: 'attack the rat', n: 1, rig: [{ kind: 'ATTACK', actor: -1, uses: 3, faces: [1, 1] }, { kind: 'ATTACK', actor: -1, uses: 2, faces: [1, 99, 1] },
            { kind: 'ATTACK', actor: -1, faces: [] }, { kind: 'ATTACK', uses: 1, faces: [1, 99] }, { kind: 'ATTACK', faces: [99, 1] }] }]);
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], FF_D_DOWN = 4, FF_D_DEAD = 8;
        const mine = t => dq0Ev(t, 'E_FIGHT', { who: 0 }), theirs = t => dq0Ev(t, 'E_FIGHT', { def: 0, who: w => w > 0 });
        const q1 = T.of('quit'), a = T.filter(t => t.line === 'attack the rat' && t.k < q1.k);
        if (a.length !== 4) return ['the attacks not typed'];
        // (Played again, the kin as it is: running off as its fellow falls, morale's)
        const a2 = T.after(T.of('dq0 -p t -s 5 -c /ram/c.hm -k sword -T', q1.k), 'attack the rat'), b2 = mine(a2);
        const down2 = b2.find(e => e.ff & FF_D_DOWN), kin = down2 && theirs(a2).find(e => e.who !== down2.def);
        if (!down2 || !kin) f.push('again: the rat not felled with its kin beside it (' + dq0Evs(a2) + ')');
        else if (theirs(a2).some(e => e.who === kin.who && e.t > down2.t) || b2.some(e => e.def === kin.who) ||
          !dq0Ev(a2, 'E_TASK', { te: 'TE_NO_FOE' }).length) f.push('again: the kin not running off as its fellow fell (' + dq0Evs(a2) + ')');
        const b = mine(a[0]);
        if (b.length < 5) return ['the attack not fought on (' + dq0Evs(a[0]) + ')'];
        if (new Set(theirs(a[0]).map(e => e.who)).size < 2) f.push('the rat and its kin not both striking at the character');
        // (Its blows at the rat in front till it's knocked senseless; the other turned to and felled)
        const down = b.find(e => e.ff & FF_D_DOWN), before = down ? b.slice(0, b.indexOf(down)) : [];
        if (!down || down.def !== b[0].def || down.out !== 'O_HIT') f.push('no blow knocked the rat it fought senseless (' + dq0Evs(a[0]) + ')');
        if (before.some(e => e.def !== b[0].def)) f.push('a blow at another before the first fell (' + dq0Evs(a[0]) + ')');
        const turned = dq0Ev(a[0], 'E_TASK', { te: 'TE_FIGHT', still: true });
        const other = b.slice(b.indexOf(down) + 1).find(e => e.def !== b[0].def && e.out === 'O_HIT' && (e.ff & (FF_D_DEAD | FF_D_DOWN)));
        if (turned.length !== 1 || !other || turned[0].by + 1 !== other.def) f.push('the other rat not turned to and felled (' + dq0Evs(a[0]) + ')');
        dq0Need(f, a[0], 'E_TASK', { te: 'TE_NO_FOE' }, 'the fight not over');
        if (b.some(e => e.out === 'O_DISPATCH')) f.push('a rat finished unasked');
        // (Those left senseless finished where they lie, an attack each: the first, and the other if it fell senseless too
        // (felled1's, 2026-11-21: its shock rolls a round each as the time comes, the dice in another order, the other's blow
        // a serious gash, not a mortal one); the next attack finding none left)
        const lying = other && !(other.ff & FF_D_DEAD) ? 2 : 1, fins = a.slice(1, 1 + lying).map(t => mine(t).find(e => e.out === 'O_DISPATCH' && (e.ff & FF_D_DEAD)));
        if (fins.some(e => !e) || !fins.some(e => e.def === b[0].def)) f.push('the senseless rat not finished where it lies (' + a.slice(1, 1 + lying).map(dq0Evs).join('; ') + ')');
        if (mine(a[1 + lying]).length) f.push('a rat fought again, none left (' + dq0Evs(a[1 + lying]) + ')');
        const lk = T.of('look');
        if (!lk.after || !lk.after.sight || lk.after.sight.items.filter(i => i.name === 'body').length < 2) f.push('their bodies not seen');
        if (!lk.after || !(lk.after.sum.health > 0)) f.push('the character not hurt by their blows');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0begin', near: ['fight', 'gear', 'parser'], what: "Dingus Quest Zero's first words at once for every command (the user's, 2026-11-17: \"You close with the giant rat.\" for an attack, \"You count out a farthing.\" for a counted drop; one path for every actor, a denizen's the same rows the other way round, \"The bandit closes with you.\"): at seed 5 (dq0fight's: a sword, the giant rats down the ladder) pennies counted out before the world splits the lot (\"You count out 2 pennies.\", then \"You put down 2 pennies.\"), the sack looked at (\"You look at the sack.\"), the ladder started down before the stratum below is grown (\"You start down the ladder.\", then as it gets there \"You climb down the ladder.\"), a wait's \"Time passes.\" first, the rat beside it attacked (\"You close with the giant rat.\" before its blow), and its kin closing with the character (\"The giant rat closes with you.\" before its first bite); a second game, a rat attacked out of reach as it's come to below: walked over to first, by the name it's seen by (\"You go over to the giant rat.\", not the word typed)",
      init: 't_rc', cycles: 2100e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 95 >> /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -k sword -T',
        'drop 2 pennies', 'look at the sack', 'd', 'wait', 'go right', 'attack the rat', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T', 'd', 'attack the rat', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'attack the rat', n: 1, k: 0, set: { mood: 8 } }, ...[0, 1, 2, 3].map(k => ({ g: 1, line: 'attack the rat', n: 1, k, hurt: 8 }))]);   // (dq0fight's: the kin steady, all hurt)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Prose(out).replace(/\s+/g, ' '), seg = l => { const i = o.indexOf('> ' + l + ' > ' + l + ' '), e = o.indexOf(' > ', i + 2 * l.length + 6); return i < 0 ? '' : o.slice(i + 2 * l.length + 6, e < 0 ? undefined : e); };
        const first = (l, re) => { const a = seg(l).trim(); if (!re.test(a)) f.push(l + ': its first words not ' + re + ': ' + a.slice(0, 300)); return a; };
        inTurn(first('drop 2 pennies', /^You count out 2 pennies\./), ['You count out 2 pennies.', 'You put down 2 pennies.'], f, 'drop 2 pennies: ');
        first('look at the sack', /^You look at the sack\./);
        inTurn(first('d', /^You start down the ladder\./), ['You start down the ladder.', 'You climb down the ladder.', 'You come into the'], f, 'd: ');
        first('wait', /^Time passes\./);
        const a = first('attack the rat', /^You close with the giant rat\./);
        inTurn(a, ['You close with the giant rat.', 'You strike at the giant rat'], f, 'attack the rat: ');
        const k = a.indexOf('The giant rat closes with you.');
        if (k < 0) f.push('the kin\'s attack begun not told: "The giant rat closes with you.": ' + a.slice(0, 600));
        else if (!/^The giant rat closes with you\.[^.]*?The giant rat bites at you/.test(a.slice(k))) f.push('the kin\'s closing not before its bite: ' + a.slice(k, k + 200));
        // (Game 2: the rat out of reach, walked over to first, by its name as it's seen, not the word typed)
        const j = o.lastIndexOf('> attack the rat > attack the rat '), b = j < 0 ? '' : o.slice(j + 34).trim();
        if (!/^You go over to the giant rat\./.test(b)) f.push('attack the rat out of reach: its first words not "You go over to the giant rat.": ' + b.slice(0, 300));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0grapple', what: 'Dingus Quest Zero\'s grappling (HOUSE.md\'s grappling, 2026-11-09: the user\'s "full HarnMaster grappling rules"; the referee\'s RS_ATTACK RS_GRAPPLE, dq0harn/grapple.c; the world\'s hold, wfight.c\'s grip), by the blows told (E_FIGHT: who, at whom, how it came out, O_HOLD ... O_LETGO; what it left them in), the actors\' places and positions and the words, the referee\'s dice the commands\' at seed 5\'s giant rats (down the ladder, beside one; the character\'s Unarmed 80, its Stamina and Will 99; the rats set hurt, dq0Poke\'s, the book\'s rat dodging at 90): "wrestle the rat" fought on till it\'s held ("You seize the giant rat and hold it fast."), the fight over then (the next move the player\'s: no fist fight after it); the rat, made wary and shy (dq0Poke\'s, as the first wait\'s typed: it would keep its distance), held where it is as it tries to walk away, each try a struggle ("The giant rat struggles in your grip."), till it breaks free ("...breaks free of your grip.") and goes; a second game\'s "throw the rat", a hold and then a throw ("You throw the giant rat to the ground", the fall\'s hurt, laid down), the hold let go as it\'s thrown ("let go": "You aren\'t holding anyone."), then "wrestle the rat" and "let go" ("You let go of the giant rat."); a third\'s "pin the rat" (pinned, laid down), then "choke the rat", the one pinned, till it falls senseless ("You choke the giant rat. It falls senseless.")',
      init: 't_rc', cycles: 2100e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml una 80 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -T',
        'd', 'wait', 'go right', 'wrestle the rat', 'wait', 'wait', 'wait', 'wait', 'wait', 'wait', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'd', 'wait', 'go right', 'throw the rat', 'let go', 'wrestle the rat', 'let go', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'd', 'wait', 'go right', 'pin the rat', 'choke the rat', 'quit', 'echo $status'] })) },
      start(m) {        // (Every rat hurt as it's come beside, each game; the first game's made wary and shy as the first wait after the hold's typed: AT_WARY, DO_SHY;
                        //   its tries to break free rigged then, dice2: the first a marginal failure, a struggle, the second a critical success against
                        //   the character's marginal failure, free)
        dq0Scene(m, [1, 2, 3].flatMap(g => [0, 1, 2, 3].map(k => ({ g, line: 'go right', n: 1, k, hurt: 8 }))).concat([0, 1, 2, 3].map(k => ({ g: 1, line: 'wait', n: 2, k, set: { attitude: 2, doing: 3 } })),
          { g: 1, line: 'wait', n: 2, rig: [{ kind: 'ATTACK', a: 'GM_BREAK', uses: 1, faces: [99] }, { kind: 'ATTACK', a: 'GM_BREAK', uses: 1, faces: [5, 99] }] }));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out), FF_D_DOWN = 4, POS_PRONE = 1, game = 'dq0 -p t -s 5 -c /ram/c.hm -T';
        const fights = t => dq0Ev(t, 'E_FIGHT'), at = (t, k) => t.after && t.after.actors.find(a => a.k === k);
        const g1 = T.of(game), q1 = T.after(g1, 'quit'), g2 = T.after(q1, game), q2 = T.after(g2, 'quit'), g3 = T.after(q2, game);
        if (g3.k < 0) return ['the games not played'];
        // (A hold, the fight over with it; the rat held where it is as it tries to go, struggling, till it's free and goes)
        const w = T.after(g1, 'wrestle the rat'), hold = fights(w).find(e => e.who === 0 && e.out === 'O_HOLD');
        if (!hold) return ['no hold (' + dq0Evs(w) + ')'];
        const k = hold.def - 1, after = fights(w).slice(fights(w).indexOf(hold) + 1);
        if (after.some(e => e.who === 0)) f.push('the character fought on after its hold (' + dq0Evs(w) + ')');
        const waits = T.filter(t => t.k > w.k && t.k < q1.k), free = waits.findIndex(t => fights(t).some(e => e.who === k + 1 && e.out === 'O_FREE'));
        const struggles = waits.slice(0, free < 0 ? waits.length : free + 1).flatMap(fights).filter(e => e.who === k + 1 && e.out === 'O_STRUGGLES');
        const p0 = at(w, k);
        if (!struggles.length) f.push('the held rat not struggling as it tried to go (' + waits.map(dq0Evs).join('; ') + ')');
        if (free < 0) f.push('the rat never broke free (' + waits.map(dq0Evs).join('; ') + ')');
        else {
          if (waits.slice(0, free + 1).some(t => !at(t, k) || !p0 || at(t, k).x !== p0.x || at(t, k).y !== p0.y)) f.push('the held rat moved');
          if (!waits.slice(free + 1).some(t => at(t, k) && (at(t, k).x !== p0.x || at(t, k).y !== p0.y))) f.push('the rat broken free not gone off');
        }
        inTurn(o, ['You seize the giant rat and hold it fast.', 'The giant rat struggles in your grip.', 'The giant rat breaks free of your grip.'], f, 'the hold\'s words: ');
        // (A throw: a hold, then thrown down, hurt, the hold let go with it; a hold on another, let go)
        const th = T.after(g2, 'throw the rat'), thrown = fights(th).find(e => e.who === 0 && e.out === 'O_THROWN');
        if (!thrown || !fights(th).some(e => e.who === 0 && e.out === 'O_HOLD' && e.def === thrown.def)) f.push('no hold and throw (' + dq0Evs(th) + ')');
        else if (!at(th, thrown.def - 1) || at(th, thrown.def - 1).pos !== POS_PRONE) f.push('the rat thrown not lying there');
        const l1 = T.after(th, 'let go'), w2 = T.after(l1, 'wrestle the rat'), l2 = T.after(w2, 'let go');
        dq0Need(f, l1, 'E_FIGHT', { who: 0, out: 'O_LETGO', def: 0 }, 'nothing held let go of, not told so');
        const h2 = fights(w2).find(e => e.who === 0 && e.out === 'O_HOLD');
        if (!h2) f.push('no second hold (' + dq0Evs(w2) + ')');
        else dq0Need(f, l2, 'E_FIGHT', { who: 0, out: 'O_LETGO', def: h2.def }, 'the hold not let go');
        const g2o = o.slice(o.indexOf('> throw the rat'));
        inTurn(g2o, ['You throw the giant rat to the ground: a ', '> let go', 'You aren\'t holding anyone.', '> let go', 'You let go of the giant rat.'], f, 'the throw\'s words: ');
        // (A pin, laid down; a choke of the one pinned, senseless)
        const pn = T.after(g3, 'pin the rat'), pin = fights(pn).find(e => e.who === 0 && e.out === 'O_PINNED');
        if (!pin) return f.concat(['no pin (' + dq0Evs(pn) + ')']);
        if (!at(pn, pin.def - 1) || at(pn, pin.def - 1).pos !== POS_PRONE) f.push('the rat pinned not lying there');
        const ch = T.after(pn, 'choke the rat'), down = fights(ch).find(e => e.who === 0 && e.out === 'O_CHOKED' && (e.ff & FF_D_DOWN));
        if (!down || down.def !== pin.def || fights(ch).some(e => e.who === 0 && e.out === 'O_HOLD')) f.push('the rat pinned not choked senseless, held as it was (' + dq0Evs(ch) + ')');
        inTurn(o.slice(o.lastIndexOf('> pin the rat')), ['You pin the giant rat to the ground.', '> choke the rat', 'You choke the giant rat. It falls senseless.'], f, 'the pin\'s and the choke\'s words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fightverbs', what: 'Dingus Quest Zero\'s fights\' verbs not done till the verbs plan\'s B1 and B7 (2026-11-10, DECISIONS.md\'s "Charge, a throw a way, engage, a split attack, and again"), by the intents sent (I_NOISE, I_CLOSE, I_ATTACK, I_TURN), the blows told (E_FIGHT), the things\' places and the words, at seed 5 (its storeroom, the giant rats down the ladder): "again" with nothing before it ("There\'s nothing to do again."); "charge" with no foe ahead ("There\'s no foe ahead to charge."); a dagger thrown a way, "throw the dagger 1 south" (lands a square south, "it lands 1 square off"), "throw the dagger 3 south" (the storeroom wall two squares south: stopped short by it, "it strikes the wall and falls 1 square off") and "throw the dagger north" (the wall behind: stopped short by it, the world\'s line, "it strikes the wall and falls at your feet"); "turn left", "again", "repeat" (the line run again, three turns: east, north, west); down the ladder, the rats made hostile, turned to face them, "charge" (the war cry, I_NOISE 6; a close-in at a run, I_CLOSE run GA_RUN, or a blow; "\\"Charge!\\""; a blow struck); a second game\'s "engage the rat" (I_CLOSE\'s walk alone, no blow: "You close with the giant rat."), and again ("You\'re already engaged with the giant rat."), then "g" twice (again\'s, Infocom\'s: the user\'s, 2026-11-11), the engage each time (the second not a "g" of the first), no close-in; a third\'s split attack, two rats beside it (one named a giant bat, dq0Poke\'s), "attack the rat and the bat" (one line: I_ATTACK with its split, "You strike at the giant rat and the giant bat by turns."), the blows fought on at both',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm',
        'dq0 -p t -s 5 -c /ram/c.hm -T -k dagger', 'again', 'charge', 'throw the dagger 1 south', 'take the dagger', 'throw the dagger 3 south', 'take the dagger', 'throw the dagger north', 'turn left', 'again', 'repeat', 'd', 'wait', 'turn right', 'charge', 'wait', 'wait', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'd', 'wait', 'engage the rat', 'engage the rat', 'g', 'g', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -T', 'd', 'wait', 'go right', 'attack the rat and the bat', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {        // (The rats hostile as the charge is typed; the third game's second rat named a giant bat and both put beside it, engaged)
        const bat = (h, L, K, at, named) => { const ks = [...Array(L.nactors).keys()].filter(j => named(j) === 'giant rat'); if (ks.length > 1) 'giant bat\0'.split('').forEach((c, i) => { h[at(ks[1]) + L.aName + i] = c.charCodeAt(0); }); };
        dq0Scene(m, [0, 1, 2, 3].map(k => ({ g: 1, line: 'charge', n: 2, k, set: { attitude: 0 }, hurt: 8 }))
          .concat([0, 1, 2, 3].map(k => ({ g: 3, line: 'go right', n: 1, k, hurt: 8 })))
          .concat([{ g: 3, line: 'attack the rat and the bat', n: 1, call: bat }, { g: 3, line: 'attack the rat and the bat', n: 1, name: 'giant rat', beside: 'face', engage: true },
            { g: 3, line: 'attack the rat and the bat', n: 1, name: 'giant bat', beside: 6, engage: true }]));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), o = dq0Prose(out), f = [], game = 'dq0 -p t -s 5 -c /ram/c.hm -T';
        const ui = (t, kind) => t.intents.filter(i => i.ring === 'ui' && i.kind === kind), mine = t => dq0Ev(t, 'E_FIGHT').filter(e => e.who === 0);
        const g1 = T.of(game + ' -k dagger'), q1 = T.after(g1, 'quit'), g2 = T.after(q1, game), q2 = T.after(g2, 'quit'), g3 = T.after(q2, game);
        if (g3.k < 0) return ['the games not played'];
        const o1 = o.slice(o.indexOf('> again'));
        inTurn(o1, ['> again', 'There\'s nothing to do again.', '> charge', 'There\'s no foe ahead to charge.', '> throw the dagger 1 south', 'You throw the dagger south: it lands 1 square off.', '> throw the dagger 3 south', 'You throw the dagger south: it strikes the wall and falls 1 square off.',
          '> throw the dagger north', 'You throw the dagger north: it strikes the wall and falls at your feet.'], f, 'the first game\'s words: ');
        // (A throw a way: two squares along the line; the wall behind, stopped short, at its feet)
        const dag = s => s && s.items.find(i => i.name === 'dagger' && i.where === 'IW_SQUARE'), t1 = T.after(g1, 'throw the dagger 1 south'), t3 = T.after(t1, 'throw the dagger 3 south'), t2 = T.after(t3, 'throw the dagger north');
        const d1 = dag(t1.after), d3 = dag(t3.after), d2 = dag(t2.after), p = t1.after && t1.after.pc;
        if (!d1 || !d3 || !p || d1.x !== p.x || d1.y !== p.y + 1 || d3.x !== p.x || d3.y !== p.y + 1) f.push('the dagger not a square south, thrown one and three (' + JSON.stringify([d1, d3]) + ', ' + JSON.stringify(p) + ')');
        if (!d2 || !t2.after || d2.x !== t2.after.pc.x || d2.y !== t2.after.pc.y) f.push('the dagger thrown at the wall not at its feet (' + JSON.stringify(d2) + ')');
        // (Again: the line before run again, repeat the same)
        const tl = T.after(g1, 'turn left'), a1 = T.after(tl, 'again'), a2 = T.after(a1, 'repeat');
        const faces = [tl, a1, a2].map(t => t.after && t.after.pc.face), turns = [a1, a2].map(t => ui(t, 'I_TURN').length);
        if (faces.join() !== '2,0,6' || turns.join() !== '1,1') f.push('"turn left", "again", "repeat" not three turns (faces ' + faces + ', turns sent ' + turns + ')');
        // (A charge: the war cry, the close-in at a run or the blow, a blow struck)
        const ch = T.after(T.after(g1, 'turn right'), 'charge'), w1 = T.after(ch, 'wait');
        const noise = ui(ch, 'I_NOISE'), close = ui(ch, 'I_CLOSE'), blow = ui(ch, 'I_ATTACK');
        if (noise.length !== 1 || noise[0].loud !== 6) f.push('no war cry (' + JSON.stringify(noise) + ')');
        if (!(close.length === 1 && close[0].walk === 0 && close[0].run === 4) && blow.length !== 1) f.push('no close-in at a run nor a blow (' + JSON.stringify(ch.intents) + ')');
        if (!mine(ch).length && !mine(w1).length) f.push('no blow struck in the charge (' + dq0Evs(ch) + '; ' + dq0Evs(w1) + ')');
        inTurn(o.slice(o.indexOf('> turn right')), ['> charge', '"Charge!"'], f, 'the charge\'s words: ');
        // (Engage: walked up to, no blow; again, already engaged)
        const e1 = T.after(g2, 'engage the rat'), e2 = T.after(e1, 'engage the rat'), ec = ui(e1, 'I_CLOSE');
        if (ec.length !== 1 || ec[0].walk !== 1) f.push('engage not a close-in alone (' + JSON.stringify(e1.intents) + ')');
        if (mine(e1).length || ui(e2, 'I_CLOSE').length) f.push('a blow struck, or a second close-in (' + dq0Evs(e1) + ')');
        inTurn(o.slice(o.indexOf('> engage the rat')), ['You close with the giant rat.', '> engage the rat', 'You\'re already engaged with the giant rat.'], f, 'engage\'s words: ');
        // ("g" again's, the user's, 2026-11-11: the engage run again twice, the line before the first "g", not the "g")
        const g1e = T.after(e2, 'g'), g2e = T.after(g1e, 'g');
        if (!g2e.after || ui(g1e, 'I_CLOSE').length || ui(g2e, 'I_CLOSE').length || mine(g1e).length) f.push('"g" not the engage again (' + dq0Evs(g1e) + ')');
        inTurn(o.slice(Math.max(0, o.search(/> g(?![a-z])/))), ['> g', 'You\'re already engaged with the giant rat.', '> g', 'You\'re already engaged with the giant rat.'], f, '"g"\'s words: ');
        // (A split attack: the blow with its split, both struck at by turns)
        const sp = T.after(g3, 'attack the rat and the bat'), sa = ui(sp, 'I_ATTACK');
        if (sa.length !== 1 || !sa[0].split || sa[0].split === sa[0].foe) f.push('no split attack sent (' + JSON.stringify(sp.intents) + ')');
        else {
          const defs = [sp, T.after(sp, 'wait')].flatMap(mine).map(e => e.def);
          if (!defs.includes(sa[0].foe) || !defs.includes(sa[0].split)) f.push('not both struck at (' + defs + ')');
        }
        inTurn(o.slice(o.indexOf('> attack the rat and the bat')), ['You strike at the giant rat and the giant bat by turns.'], f, 'the split\'s words: ');
        return f;
      },
      sdcard: true
    },
    {
      // (dq0sky=0, 2026-11-03, a scene's: seed 1's straw gone for the skylight, the fight's dice other: another bandit took up the sword the character let fall before its own was taken up; no fault)
      name: 'dq0disarm', asks: true, initiative: true, what: 'Dingus Quest Zero\'s denizens\' limbs: a denizen\'s fumble (slice 4.1b, the brief\'s: "The same rules for every actor with hands and legs: a denizen\'s fumble drops its weapon (the world\'s floor, seen and taken up)"), by the denizens\' events, the item store and the words (HarnMaster\'s dice as they fall here: no -a, no -I, asks and initiative, the game as a search found it): the hand-written theme\'s hostile bandits (dq0robbed\'s, seed 1, south of the start), the character (hmchar seed 23, Dodge 90, Sword 60) its pack dropped (2026-10-26: its armour from it there, the reach\'s times, the dice as a search of the variants found them), in a gambeson and a mail hauberk, blocking, waiting for them: a bandit knocked senseless by its blow ("It falls senseless.", "lying senseless"), doing nothing as it lies there, deaf to words ("talk to the bandit": "The bandit lies senseless: it can\'t hear you.", the denizens\' SR_DOWN, the rouse the talk sends it ignored), its first turn as it comes round getting to its feet (the referee\'s O_RISE, "The bandit gets to its feet.": it fell as it was knocked out, 2026-10-29), and only then fighting on; a bandit\'s sword fumbled (its blow\'s, "The bandit strikes at you with its sword, but fumbles the blow.", or its guard\'s, "; it falters.": since 2026-10-29 its guard\'s, the fight\'s dice another fall), let fall (the world\'s E_BEAT WB_DROP in the denizens\' ring, its kind the sword\'s, its b the bandit\'s entry plus 1: "The bandit drops its sword."), taken up again at its next turn (WB_PICKS, the referee\'s RS_REARM and O_REARM: "The bandit snatches up its sword."), gone from the floor (as many swords in the store after as before).  The scene set (since the dice by who asks, 2026-10-30, the bandits roam with their door open, and come for no one waiting): the character goes into their room, two steps south through the door, and waits there; two games, each in its own folder: -R 25\'s the fumble (thirty waits, the bandit coming in at the twenty-third), -R 48\'s the bandit knocked senseless as it\'s talked to (talked to again as it lies there).  The scene set (since the denizens\' day, 2026-10-31: bandits keep watches, a third asleep): no day (the environment\'s dq0hour=128, rc\'s, before its games: dq0d\'s), every one about as before',
      init: 't_rc', cycles: 2500e6,
      // (hmchar's seed 26, not 23, 2026-10-27: the bandits' lairs in the rooms they like, the belongings', their fight's dice
      // another fall, the fumble in another bout)
      // (The scene set since the dice by who asks, 2026-10-30: the bandits' room gone into, two squares south through the
      // door, their own door opened already as they roam; two games, each in a folder of its own (a game's folder its save:
      // the second would go on with the first), the dice's offsets each its own: -R 24's a fumble (21's till
      // 2026-11-19: act1's door noises, the bandits come at another moment), -R 48's a bandit knocked senseless as it's
      // talked to, talked to again as it lies there)
      // (Game 1's waits 25, not 15, hear1's, 2026-11-20: the minds hear one another's footsteps by the senses' rule, so
      // fewer bandits stand frozen by their fellows' steps and they roam their room; at -R 24 bandit H comes in at the
      // twentieth wait and fumbles in its guard; the cycles 2,200M for the longer game, not 1,500M)
      // (Game 1 -R 25 and 30 waits, hear2's, 2026-11-20: the gambeson worn now, got out of the pack it dropped (to hand
      // in a container set down), the fight's dice another fall; a search of -R 1-42 at 25 waits found 25 and 41; at
      // -R 25 bandit H comes in at the twenty-third wait and falters in its guard, seven waits to spare)
      // (Game 2 -R 16, felled1's, 2026-11-21: a felled one's shock rolls a round each as the time comes, the dice in another
      // order; at -R 48 the bandit came round before a look saw it lie; a search of -R 1-24 found 16)
      // (No -R since dice2's keyed dice, 2026-11-24: both games' scenes rigged and set in start, the dice searched no more)
      machine: { input: (() => {
        const game = (d, talks, w = 15) => ["dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/" + d + " -k 'sword,gambeson,mail hauberk' -S /ram/s.sc", 'drop the backpack', 'wear the gambeson',
          'wear the mail hauberk', 'defend block', 'wait', 'close the door', 'open the door', 'go south', 'go south', ...Array(w).fill('wait'),
          ...Array(talks).fill('talk to the bandit'), ...Array(3).fill('wait'), 'quit'];
        return dq0Typed(['bind -a /sd/0/games /bin', 'echo sky 0 >> /ram/s.sc', 'hmchar -q -s 26 -u feudal -c hunter -o /ram/c.hm', 'echo ml dge 90 >> /ram/c.hm',
          'echo ml swd 60 >> /ram/c.hm', 'echo hour 128 >> /ram/s.sc', 'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
          'mkdir /ram/h', 'cp /ram/g/theme.txt /ram/h/theme.txt', ...game('g', 1, 30), ...game('h', 2), 'echo $status']);
      })() },
      start(m) {        // (Told as it falls, 2026-11-03: the lines typed no longer fall on it)
        dq0Scene(m, [...[1, 2].map(g => ({ g, line: 'drop the backpack', n: 1, call: (h, L) => { h[L.nolurk] = 1; } })),   // (No robber lying in wait, HOUSE.md's hiding oneself: this scene's the bandits at it, 2026-11-04)
          // (Rigged since dice2's keyed dice, no -R: game 1's second bandit blow (its first, the quicker's, as it falls) a
          // critical failure, blocked by a marginal failure (AF: its fumble), its fumble roll two 6s and a die, its sword
          // let fall; the character's own blows kept from it.  Game 2's bandit, felled on the walk in by a grievous wound
          // and a serious one (the keyed dice's: its courage broken by the first, its shock rolls never passed), lies
          // through the waits and the first talk; at the second its wounds one minor one of two levels (dq0Poke's hurt)
          // and its flight forgotten (the minds' md.fled), its shock rolls 1s: it comes round at its next, and its first
          // turn up is the fight's (O_RISE), not a flight on its own legs)
          { g: 1, line: 'go south', n: 2, rig: [{ kind: 'ATTACK', actor: -1, faces: [] }, { kind: 'ATTACK', uses: 1, faces: [] }, { kind: 'ATTACK', uses: 1, faces: [100, 99, 6, 6] }] },
          { g: 2, line: 'talk to the bandit', n: 2, rig: [{ kind: 'ROUSE', faces: [1, 1, 1, 1] }], call: (h, L, K, at, named, sc) => {
            const ad = dq0Ad(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0d', 'dq0d.lbl'), 'latin1')), ram = m.taskRam[h[12 + 2]];
            for (let k = 0; k < L.nactors; k++) if (named(k) === 'bandit' && h[at(k) + L.aDoing] === K.DO_DOWN) { ram[ad('fled') + k] = 0; sc.hurt(k, 2); }
          } }]);
        dq0OnWords(m, Array.from({ length: 15 }, (_, i) => ({ g: 2, line: 'wait', n: i + 2, words: 'They fall senseless.', text: 'talk to the bandit\r' })));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], line = "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk' -S /ram/s.sc";
        const g = T.of(line), end = T.of('quit', g.k), g2 = T.of(line.replace('/ram/g', '/ram/h'), end.k);
        if (g.k < 0 || g2.k < 0) return ['the games not played'];
        const beats = T.filter(u => u.k > g.k && u.k < end.k).flatMap(u => [...u.events, ...u.dens]).filter(e => e.kind === 'E_BEAT');
        const drop = beats.find(e => e.beat === 'WB_DROP' && e.b > 0), pick = beats.find(e => e.beat === 'WB_PICKS' && e.b > 0);
        const w0 = T.after(g, 'wear the gambeson'), sword = w0.after && w0.after.items.find(i => i.name === 'sword');
        if (!drop || !sword || drop.thing !== sword.kind) f.push('no bandit\'s sword let fall (' + JSON.stringify(drop) + ')');
        else if (!pick || pick.b !== drop.b || pick.thing !== drop.thing) f.push('the bandit\'s sword not taken up again (' + JSON.stringify(pick) + ')');
        const swords = s => s ? s.items.filter(i => i.name === 'sword').length : -1;
        if (swords(end.at) !== swords(w0.after)) f.push('a sword left on the floor, taken up again (' + swords(w0.after) + ' swords before, ' + swords(end.at) + ' after)');
        const said = dq0Prose(out), fum = said.search(/The bandit strikes at you with their sword, but fumbles the blow\.|; they falter\./);
        if (fum < 0) f.push('the bandit\'s fumble not told');
        else inTurn(said.slice(fum), ['The bandit drops their sword.', 'The bandit snatches up their sword.'], f, 'the bandit\'s fumble\'s words: ');
        // (Knocked senseless, it lies there doing nothing, and comes round on the floor: its first turn after, up)
        const fights = T.filter(u => u.k > g2.k).flatMap(u => u.dens).filter(e => e.kind === 'E_FIGHT');
        const out1 = fights.findIndex(e => e.who === 0 && e.a > 0 && (e.ff & 4)), by = out1 < 0 ? 0 : fights[out1].a;
        const next = fights.slice(out1 + 1).find(e => e.who === by);
        if (out1 < 0) f.push('no bandit knocked senseless');
        else if (!next || next.out !== 'O_RISE') f.push('the bandit come round not getting to its feet first (' + JSON.stringify(next) + ')');
        inTurn(said.slice(said.lastIndexOf('dq0 -p t -s 1 -c /ram/c.hm')), ['They fall senseless.', 'lying senseless.', 'The bandit lies senseless: they can\'t hear you.', 'The bandit gets to their feet.'], f,
          'the bandit knocked senseless: ');
        // (Its shock rolls as their time comes, a round each, as the character's (the user's, 2026-11-21, felled1): the world's
        // RS_ROUSE asked for it (its actor's) as it lay, senseless still a round on at most, then come round (0))
        const RSR = m.dq0.layout().K.RS_ROUSE, rouses = T.filter(u => u.k > g2.k).flatMap(u => u.replies.filter(r => r.req === 'slot0' && r.kind === RSR && r.actor === by));
        if (out1 >= 0 && (rouses.length < 2 || rouses.slice(0, -1).some(r => !r.w2 || r.w2 > 100) || rouses[rouses.length - 1].w2 !== 0))
          f.push('the bandit\'s shock rolls not asked a round each as it lay (RS_ROUSE: ' + rouses.map(r => r.w2).join(', ') + ')');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0shield', outfit: true, asks: true, initiative: true, what: 'Dingus Quest Zero\'s denizens\' shields (the gear3 agent\'s, 2026-10-29; HOUSE.md\'s the denizens\' gear: a shield held blocks, for review), by the denizens\' events, the referee\'s replies and the words: dq0disarm\'s bandits with their own things (the outfits\'), the character (hmchar seed 23, Dodge 90, Sword 90) in a gambeson and a mail hauberk waiting for them: a bandit with its weapon in hand unslinging the shield on its back into its other hand, its round (the world\'s E_BEAT WB_READIES, its kind a shield\'s: "The bandit unslings its round shield."), then blocking the character\'s blow with it (RS_ATTACK\'s reply: the defence D_BLOCK, its c AF_SHIELD, O_BLOCKED: "it fends off the blow with its shield."); a shield\'s block only ever a block (AF_SHIELD with D_BLOCK alone), and by an actor that holds one.  The scene set (since the denizens\' day and leisure, 2026-10-31: bandits keep watches, and dice and drink at leisure): no day (the environment\'s dq0hour=128, rc\'s), every one about as before',
      init: 't_rc', cycles: 1700e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml dge 90 >> /ram/c.hm',
        'echo ml swd 90 >> /ram/c.hm', 'echo hour 128 >> /ram/s.sc', 'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk' -S /ram/s.sc", 'drop the backpack', 'wear the gambeson', 'wear the mail hauberk',
        'defend block', ...Array(44).fill('wait'), 'quit', 'echo $status'] })) },
      start(m) {
        // (One of the band set on it, hostile, beside it, the rest friendly, a scene's: their way to it the dice's, 2026-11-03; its
        // first blow and the character's rigged, dice2: each a marginal success met by one, blocked, the character's by its shield)
        dq0Scene(m, [0, 1, 2].map(k => k ? { g: 1, line: 'wait', n: 1, k, set: { attitude: 4, doing: 0 } } : { g: 1, line: 'wait', n: 1, k, set: { attitude: 0, doing: 0 }, beside: 4,
          rig: [{ kind: 'ATTACK', actor: 0, uses: 1, faces: [1, 1] }, { kind: 'ATTACK', actor: -1, uses: 1, faces: [1, 1] }] }));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], K = m.dq0.layout().K, o = dq0Prose(out);
        const g = T.of("dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk' -S /ram/s.sc");
        if (g.k < 0) return ['the game not played'];
        const after = T.filter(u => u.k > g.k), names = new Map();
        for (const u of after) for (const s of [u.at, u.after]) if (s) for (const i of s.items) names.set(i.kind, i.name);
        const shields = ['round shield', 'buckler', 'knight shield', 'kite shield', 'tower shield'];
        const ready = after.flatMap(u => [...u.events, ...u.dens]).find(e => e.kind === 'E_BEAT' && e.beat === 'WB_READIES' && e.b > 0);
        if (!ready || !shields.includes(names.get(ready.thing))) f.push('no bandit\'s shield unslung (' + JSON.stringify(ready) + ')');
        const atts = after.flatMap(u => u.replies).filter(r => r.kind === K.RS_ATTACK);
        const blocks = atts.filter(r => r.c & 0x40);
        if (!ready || !blocks.some(r => r.actor === 0 && r.other === ready.b && r.b === 1 && r.result === 2))
          f.push('the character\'s blow not blocked by the bandit\'s shield (D_BLOCK, AF_SHIELD, O_BLOCKED): ' + JSON.stringify(blocks));
        if (blocks.some(r => r.b !== 1 || r.actor !== 0)) f.push('AF_SHIELD not a denizen\'s block: ' + JSON.stringify(blocks));
        const sw = ready && names.get(ready.thing);
        inTurn(o, ['The bandit unslings their ' + sw + '.', 'they fend off the blow with their ' + sw], f, 'the shield\'s words: ');   // (People "they",
                                                //   the user's, 2026-11-17: "its" before)
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0they', outfit: true, asks: true, initiative: true, near: ['fight', 'denizens'], what: "Dingus Quest Zero's people \"they\", beasts \"it\" (the user's, 2026-11-17: \"The elf unslings their hartbow.\"; GAME-CRITIQUE.md's 5.6): the speaking peoples' kinds (kinds.txt's nature f, the hub's folk as the denizens make it an actor: the one source, the referee's words from it, the world's blow's RS_THEY and first aid's w2, its bestiary no flag of its own, 2026-11-17) said \"their\" and \"they\", a pronoun the subject its verb plural (\"they fend off the blow\"), the kind named keeping the singular (\"The bandit unslings their round shield.\"): dq0shield's bandits set on the character, no \"its\" nor \"it\" of one, their shield unslung and its block in their words, the referee's wound and armour words \"their\"; a giant rat still \"it\" (seed 3's, keeping its distance)",
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml dge 90 >> /ram/c.hm',
        'echo ml swd 90 >> /ram/c.hm', 'echo hour 128 >> /ram/s.sc', 'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk' -S /ram/s.sc", 'drop the backpack', 'wear the gambeson', 'wear the mail hauberk',
        'defend block', ...Array(44).fill('wait'), 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -S /ram/s.sc', 'f', 'look', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [0, 1, 2].map(k => k ? { g: 1, line: 'wait', n: 1, k, set: { attitude: 4, doing: 0 } } : { g: 1, line: 'wait', n: 1, k, set: { attitude: 0, doing: 0 }, beside: 4 }));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Prose(out), at = o.indexOf('dq0 -p t -s 3 -c /ram/c.hm -T -S /ram/s.sc'), b = at < 0 ? o : o.slice(0, at), r = at < 0 ? '' : o.slice(at);
        const ss = b.split(/(?<=[.!?])\s+/);
        for (const s of ss.filter(s => /The bandit|the bandit/.test(s))) {
          if (/\bits\b/.test(s)) f.push('a bandit\'s "its": ' + JSON.stringify(s));
          if (/[;,] it (dodges|fends|strikes|stumbles|falters|slips|twists)|\bIt (falls|flees)\b/.test(s)) f.push('a bandit "it": ' + JSON.stringify(s));
        }
        for (const w of [/The bandit unslings their [a-z ]+\./, /they fend off the blow with their [a-z ]+/]) if (!w.test(b)) f.push('not said: ' + w);
        if (/: a [a-z ]+ to its [a-z ]+ \([MSG]\d+\)/.test(b)) f.push('a bandit\'s wound "to its"');
        if (!/keeping its distance/.test(r)) f.push('a giant rat not "keeping its distance" (seed 3): ' + JSON.stringify(r.slice(0, 400)));
        // (One source, the hub's folk: the referee's bestiary no flag of its own, its words by the requests', 2026-11-17)
        if (/BF_THEY/.test(fs.readFileSync(path.join(TESTS, '..', 'games', 'dq0harn', 'beasts.c'), 'latin1'))) f.push('the referee\'s bestiary keeps a flag of its own, BF_THEY');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stray', near: ['fight'], what: "Dingus Quest Zero's miss's stray at its two chances (2026-11-03, the user's \"follow the book\": HOUSE.md's a miss's stray: half the time if the mark's engaged, a fifth if it isn't), by the words: at seed 3 the hand-written theme's giant rats, the room's far end, two held where they are as each shot's typed (dq0Poke's: the mark the closer, a second beside it, their wounds taken off, due later than the shot): shot at by a poor shot many times, no rat in a fight, and again in a second game with the one beside the mark fighting (its doing DO_FIGHT, all the rule asks): of the misses, about a fifth strike the one beside it in the first, about half in the second; its words each time \"Your shot strikes ... instead\"",
      init: 't_rc', cycles: 2400e6,
      machine: { input: (() => {
        const game = d => ["dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/" + d + " -k 'sling,sling stones'", 'drop the torch', ...Array(60).fill('shoot the closest rat'), 'quit'];
        return dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml bow 15 >> /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
          'mkdir /ram/h', 'cp /ram/g/theme.txt /ram/h/theme.txt', ...game('g'), ...game('h'), 'echo $status'] }));
      })() },
      start(m) {
        const set = fight => (h, L, K, at, named, sc) => {
          const R = dq0Rec(m), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, due = now + 6000;
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);
          for (let i = 0; i < L.nitems; i++) {   // (Stones enough, its sling in its hand, as it shoots: a fumble lets it fall)
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize, s = b && b[o + L.iKind] ? String.fromCharCode(...cat.subarray(b[o + L.iKind] * L.kindSize, b[o + L.iKind] * L.kindSize + 11)) : '';
            if (s === 'sling stone') b[o + L.iQty] = 20;
            if (s === 'sling\0\0\0\0\0\0') {
              if (b[o + L.iFlags] & DQ0_I.HELD) m.sling = b.slice(o, o + L.itemSize);
              else if (m.sling) b.set(m.sling, o);
            }
          }
          let n = 0;
          for (let k = 0; k < L.nactors; k++) {
            if (!h[at(k) + L.aName] || !h[at(k) + L.aState]) continue;
            const o = R.at(h[at(k) + L.aDen]), [x, y] = [[32720, 32721], [32719, 32721], [32719, 32722], [32720, 32722]][n++ & 3];
            h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8;
            h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_WARY;
            h[at(k) + L.aDoing] = fight && n === 2 ? K.DO_FIGHT : K.DO_DOWN;
            h[at(k) + L.aPos] = K.POS_PRONE;
            h[at(k) + L.aReady] = due & 255; h[at(k) + L.aReady + 1] = due >> 8 & 255; h[at(k) + L.aReady + 2] = due >> 16 & 255; h[at(k) + L.aReady + 3] = 0;
            sc.well(k);                         // (Senseless, its wounds off: the referee's)
            sc.out(k, due);
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        const pokes = [];
        for (const [g, fight] of [[1, false], [2, true]]) for (let n = 1; n <= 60; n++) pokes.push({ g, line: 'shoot the closest rat', n, call: set(fight) });
        dq0Scene(m, pokes);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [], gs = o.split('% dq0 -p t').slice(1), cnt = s => [(s.match(/Your shot strikes [a-z ]+ instead/g) || []).length, (s.match(/shoot at [a-z ]+, and miss|shoot at [a-z ]+; it dodges/g) || []).length];
        if (gs.length !== 2) return ['not two games'];
        const [s1, m1] = cnt(gs[0]), [s2, m2] = cnt(gs[1]);
        this.notes = ['not engaged: ' + s1 + ' strays of ' + m1 + ' misses; engaged: ' + s2 + ' of ' + m2];
        if (m1 < 30 || m2 < 30) f.push('too few misses to tell: ' + m1 + ', ' + m2);
        if (s1 < 0.08 * m1 || s1 > 0.35 * m1) f.push('not engaged, not about a fifth of the misses astray: ' + s1 + ' of ' + m1);
        if (s2 < 0.33 * m2 || s2 > 0.67 * m2) f.push('engaged, not about half of the misses astray: ' + s2 + ' of ' + m2);
        if (s1 / m1 >= s2 / m2) f.push('no more astray with the mark engaged: ' + s1 + ' of ' + m1 + ', ' + s2 + ' of ' + m2);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hardthrow', near: ['fight'], what: "Dingus Quest Zero's hard throw (HOUSE.md's thrown hard: twice as far, and 10 less; the world's reach doubled, two steps in its request), by the referee's requests: at seed 3 a sword (its short band a square: 1, 2, 4, 8) thrown at the bones from across the room: plain at the band its squares fall in, 20 less a band (P_THROW's steps, four a band); hard at the band twice the reach gives, and two steps worse: the steps its own band's less two; and at a giant rat held senseless two squares off (the scene's, dq0Poke's): RS_ATTACK's steps none plain, two worse hard",
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        "dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k 'sword,sword,sword,sword'", 'step west', 'step west', 'step west', 'face east', 'hold the sword', 'throw the sword at the bones', 'hold the sword', 'throw the sword hard at the bones',
        'hold the sword', 'step north', 'throw the sword at the closest rat', 'hold the sword', 'step north', 'throw the sword hard at the closest rat', 'quit', 'echo $status'] })) },
      start(m) {
        const put = (go, rat) => (h, L, K, at, named, sc) => {
          const w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8; }, R = dq0Rec(m), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, due = now + 6000;
          if (go) { w16(L.pcX, 32705); w16(L.pcY, 32735); }   // (A step from it to a long room's corner)
          let n = 0;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            const o = R.at(h[at(k) + L.aDen]), [x, y] = !n && rat ? [32707, 32734] : [32705 + n, 32741];
            ++n;
            w16(at(k) + L.aX, x); w16(at(k) + L.aY, y);
            R.mem[o + R.X] = x & 255; R.mem[o + R.X + 1] = x >> 8; R.mem[o + R.Y] = y & 255; R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_WARY;
            h[at(k) + L.aDoing] = K.DO_DOWN; h[at(k) + L.aPos] = K.POS_PRONE;
            for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = due >>> (8 * i) & 255;
            sc.well(k);                         // (Senseless, its wounds off: the referee's)
            sc.out(k, due);
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        dq0Scene(m, [{ g: 1, line: 'step west', n: 1, call: put(0, 0) }, { g: 1, line: 'hold the sword', n: 3, call: put(1, 1) }, { g: 1, line: 'step north', n: 1, call: put(1, 1) }, { g: 1, line: 'throw the sword at the closest rat', n: 1, call: put(0, 1) },
          { g: 1, line: 'hold the sword', n: 4, call: put(1, 1) }, { g: 1, line: 'step north', n: 2, call: put(1, 1) }, { g: 1, line: 'throw the sword hard at the closest rat', n: 1, call: put(0, 1) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out);
        const pl = T.of('throw the sword at the bones'), hd = T.of('throw the sword hard at the bones');
        if (!pl.at || !hd.at) return ['the throws not typed'];
        const bones = pl.at.items.find(i => i.name === 'bones'), d = Math.max(Math.abs(bones.x - pl.at.pc.x), Math.abs(bones.y - pl.at.pc.y)), band = c => { let b = 0; while (b < 4 && d > c << b) ++b; return b; };
        const st = t => t.replies.filter(r => r.rs === 'RS_TEST' && r.ask.purpose === 'P_THROW').map(r => r.ask.steps);   // (As asked: msgs.def's)
        const want = [-4 * band(1), -2 - 4 * band(2)];
        this.notes = [d + ' squares from the bones: plain ' + JSON.stringify(st(pl)) + ', hard ' + JSON.stringify(st(hd))];
        if (band(1) === band(2)) f.push('no band to tell the reach by: ' + d + ' squares');
        if (st(pl)[0] !== want[0]) f.push('a plain throw not at its band: ' + JSON.stringify(st(pl)) + ', not ' + want[0]);
        if (st(hd)[0] !== want[1]) f.push('a hard throw not at twice the reach, two steps worse: ' + JSON.stringify(st(hd)) + ', not ' + want[1]);
        const shot = t => t.replies.filter(r => r.rs === 'RS_ATTACK').map(r => r.ask.steps), a = shot(T.of('throw the sword at the closest rat')), b = shot(T.of('throw the sword hard at the closest rat'));
        if (a.length !== 1 || a[0] !== 0) f.push('thrown at the rat plainly, not no steps: ' + JSON.stringify(a));
        if (b.length !== 1 || b[0] !== -2) f.push('thrown hard at the rat, not two steps worse: ' + JSON.stringify(b));
        inTurn(o, ['You throw your sword at the giant rat', 'You throw your sword hard at the giant rat'], f, 'the rat: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0namedblow', near: ['fight', 'gear'], what: "Dingus Quest Zero's blow with a weapon named, a better one held (HOUSE.md's the Gargun's gear, the margins' pass of 2026-11-03: the mankar and the mang kinds of their own; till then \"strike with the mang\" struck with the best held), by the words: at seed 3 a giant rat set beside the character, wary, its sword in one hand, a mang then a mankar in the other, each named: its blow worded that one, not the sword; and with none named, the best it holds",
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 90 >> /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        "dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k 'sword,mang,mankar'", 'drop the torch', 'hold the sword', 'hold the mang', 'i', 'strike the rat with the mang', 'stow the mang', 'hold the mankar', 'i', 'strike the rat with the mankar', 'strike the rat', 'quit', 'echo $status'] })) },
      start(m) {
        const held = (h, L, K, at, named, sc) => {
          const R = dq0Rec(m), now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, due = now + 3000; let n = 0;
          const px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            const o = R.at(h[at(k) + L.aDen]), x = n ? 32719 + n : px - 1, y = n ? 32721 : py;
            ++n;
            h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8; h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = K.AT_WARY; h[at(k) + L.aDoing] = K.DO_WATCH;
            h[at(k) + L.aPos] = K.POS_STANDING;
            for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = due >>> (8 * i) & 255;
            sc.well(k);                         // (Awake, its wounds off: the referee's)
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        dq0Scene(m, ['hold the sword', 'strike the rat with the mang', 'stow the mang', 'strike the rat with the mankar', 'strike the rat'].map(line => ({ g: 1, line, n: 1, call: held })));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [];
        const part = (a, b) => o.slice(o.indexOf('> ' + a), b ? o.indexOf('> ' + b, o.indexOf('> ' + a)) : undefined);
        const m1 = part('strike the rat with the mang', 'stow the mang'), m2 = part('strike the rat with the mankar', 'strike the rat >'), m3 = o.slice(o.lastIndexOf('> strike the rat '));
        if (!/You strike at (?:the giant rat|something) with your mang/.test(m1)) f.push('the mang named, not struck with: ' + m1.slice(0, 300));
        if (/with your sword/.test(m1.split('> ')[1] || '')) f.push('the sword struck with, the mang named');
        if (!/You strike at (?:the giant rat|something) with your mankar/.test(m2)) f.push('the mankar named, not struck with: ' + m2.slice(0, 300));
        if (!/You strike at (?:the giant rat|something) with your sword/.test(m3)) f.push('none named, not the best it holds: ' + m3.slice(0, 300));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0throwheld', near: ['fight', 'gear'], what: "Dingus Quest Zero's thing thrown out of the hand (the user's playtest, 2026-11-20: a dagger thrown at a foe and missed, the foe's bite then blocked with the dagger it no longer had: the referee never told it had gone, nor the UI's copy of what's carried), by the item store, the referee's requests (RS_EQUIP, the character's hands empty), the UI bank's copy (no thing in a hand) and the words: at seed 3 (the hunter's dagger in hand, another at its belt) a giant rat set hostile two squares west, the character set to block, the dagger thrown at the rat: no blow struck nor bite fended off with a dagger after it, what's carried nothing in a hand; then at the normal pace (a game of its own), its torch kept, the belt's dagger gone, a rat beside it senseless and the rest far, senseless too (none roused to fight), the dagger thrown, what's carried (no dagger), taken up from the floor (where it fell out of the light: the scene's, at its feet), held and thrown again, held again (none to hand: gone over to where it lies, or told so; its first words no lifting of the dagger, the UI's copy's)",
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        "dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k dagger", 'drop the torch', 'defend block', 'throw the dagger at the closest rat', 'wait', 'i', 'quit',
        'mkdir /ram/h', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/h/theme.txt', "dq0 -p n -s 3 -c /ram/c.hm -T -d /ram/h -k dagger", 'look', 'throw the dagger at the closest rat', 'i', 'look', 'take the dagger', 'hold the dagger', 'throw the dagger at the closest rat', 'hold the dagger', 'i', 'quit', 'echo $status'] })) },
      start(m) {
        const held = [];
        m.uiHeld = held;
        const put = (d, foe) => (h, L, K, at, named, sc) => {   // (Rat 0 d squares west, hostile or held still; the rest far off, friendly, held still)
          const R = dq0Rec(m), due = (h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16) + 6000; let n = 0;
          const px = h[L.pcX] | h[L.pcX + 1] << 8, py = h[L.pcY] | h[L.pcY + 1] << 8;
          for (let k = 0; k < L.nactors; k++) if (h[at(k) + L.aName] && h[at(k) + L.aState]) {
            const o = R.at(h[at(k) + L.aDen]), far = n > 0, x = far ? 32719 + n : px - d, y = far ? 32721 : py;
            ++n;
            h[at(k) + L.aX] = R.mem[o + R.X] = x & 255; h[at(k) + L.aX + 1] = R.mem[o + R.X + 1] = x >> 8; h[at(k) + L.aY] = R.mem[o + R.Y] = y & 255; h[at(k) + L.aY + 1] = R.mem[o + R.Y + 1] = y >> 8;
            h[at(k) + L.aAttitude] = R.mem[o + R.ATTITUDE] = far ? K.AT_FRIENDLY : foe ? K.AT_HOSTILE : K.AT_WARY; h[at(k) + L.aDoing] = far ? K.DO_ROAM : foe ? K.DO_FIGHT : K.DO_WATCH;
            h[at(k) + L.aPos] = K.POS_STANDING;
            if (far || !foe) for (let i = 0; i < 4; i++) h[at(k) + L.aReady + i] = due >>> (8 * i) & 255;
            sc.well(k);                         // (Awake, its wounds off: the referee's)
            if (far && !foe) {   // (The second game's others senseless till the scene's done: none roused by the noise to come and fight)
              h[at(k) + L.aDoing] = K.DO_DOWN; h[at(k) + L.aPos] = K.POS_PRONE;
              sc.out(k, due);
            }
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        const unbelt = (h, L) => {   // (The dagger not in a hand gone from the store: one dagger in the game)
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize);
          for (let i = 0; cat && i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (!b || !b[o + L.iKind]) continue;
            let n = '';
            for (let j = 0; j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]);
            if (n === 'dagger' && !(b[o + L.iFlags] & 2)) b[o + L.iKind] = 0;
          }
        };
        const near = (h, L) => {   // (The dagger, fallen out of the torch's light, put at the character's feet: to be taken up)
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), px = h[L.pcX] | h[L.pcX + 1] << 8;
          for (let i = 0; cat && i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (b && b[o + L.iKind] === 5 && b[o + L.iWhere] === 0) { b[o + L.iX] = px & 255; b[o + L.iX + 1] = px >> 8; b[o + L.iY] = h[L.pcY]; b[o + L.iY + 1] = h[L.pcY + 1]; }
          }
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
        };
        const look = line => (h, L) => {   // (The UI bank's copy of what's carried: its things in a hand, by kind's name)
          const ui = m.dq0.bank(L.uiBank), names = [];
          for (let i = 0; ui && i < Math.min(ui[L.uIxn], 40); i++) {
            const e = L.uIx + i * L.cixSize, k = ui[e + L.cxKind];
            let n = '';
            for (let j = 0; k !== 0xFF && j < 14 && ui[L.uIxks + k * L.cixkSize + L.ckName + j]; j++) n += String.fromCharCode(ui[L.uIxks + k * L.cixkSize + L.ckName + j]);
            if ((ui[e + L.cxFlags] & 1) && n === 'dagger') names.push(n);
          }
          held.push({ line, names });
        };
        dq0Scene(m, [{ g: 1, line: 'throw the dagger at the closest rat', n: 1, call: put(2, 1) }, { g: 1, line: 'wait', n: 1, call: look('g1') },
          { g: 2, line: 'look', n: 1, call: unbelt }, { g: 2, line: 'throw the dagger at the closest rat', n: 1, call: put(1, 0), name: 'giant rat', senseless: 3000 },
          { g: 2, line: 'i', n: 1, call: look('g2') }, { g: 2, line: 'look', n: 2, call: near }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), o = dq0Prose(out), f = [];
        if (process.env.DQ0SHOW) console.log(o);
        const g1 = T.of("dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g -k dagger"), g2 = T.of("dq0 -p n -s 3 -c /ram/c.hm -T -d /ram/h -k dagger");
        if (g1.k < 0 || g2.k < 0) return ['the games not played'];
        const th = T.after(g1, 'throw the dagger at the closest rat'), t2 = T.after(g2, 'throw the dagger at the closest rat');
        const inHand = s => dq0Carried(s).filter(i => i.name === 'dagger' && (i.flags & DQ0_I.HELD)), lying = s => s ? s.items.filter(i => i.name === 'dagger' && i.where === 'IW_SQUARE') : [];
        for (const [g, t] of [['the first', th], ['the second', t2]]) {
          if (!t.at || inHand(t.at).length !== 1) { f.push(g + ' game\'s dagger not in hand before the throw'); continue; }
          if (inHand(t.after).length || lying(t.after).length !== 1) f.push(g + ' game\'s dagger not on the floor after the throw: ' + JSON.stringify((t.after ? t.after.items : []).filter(i => i.name === 'dagger')));
          const eq = t.replies.filter(r => r.rs === 'RS_EQUIP' && r.actor === 0);
          if (!eq.length || (t === th && eq[eq.length - 1].ask.held !== 0)) f.push(g + ' game\'s referee not told the hands are empty after the throw: ' + JSON.stringify(eq.map(r => r.ask)));
        }
        // (The first: thrown, then fought on: no blow, block nor fend with the dagger it threw)
        const o1 = o.slice(o.indexOf('You throw your dagger at'), o.indexOf('> quit'));
        if (!/You throw your dagger at/.test(o)) f.push('the throw not told');
        if (/with your dagger/.test(o1)) f.push('the dagger used after it was thrown: ' + (o1.match(/[^.]*with your dagger[^.]*\./) || [''])[0]);
        const i1 = o.slice(o.indexOf('> i', o.indexOf('You throw your dagger at')), o.indexOf('> quit'));
        if (/dagger, in your/.test(i1)) f.push('what\'s carried, after the throw, a thing in hand: ' + i1.slice(0, 300));
        for (const u of m.uiHeld) if (u.names.length) f.push('the UI\'s copy of what\'s carried, ' + u.line + ', a thing in hand: ' + u.names.join(', '));
        if (m.uiHeld.length !== 2) f.push('the UI\'s copy not read twice: ' + JSON.stringify(m.uiHeld));
        // (The second: taken up from where it fell, held, thrown again, held again)
        const tk = T.after(g2, 'take the dagger'), h1 = T.after(tk, 'hold the dagger'), t3 = T.after(h1, 'throw the dagger at the closest rat'), h2 = T.after(t3, 'hold the dagger');
        if (!dq0Carried(tk.after).some(i => i.name === 'dagger')) f.push('the dagger not taken up from the floor: ' + JSON.stringify(lying(tk.after)));
        if (inHand(h1.after).length !== 1) f.push('the dagger taken up not held: ' + JSON.stringify((h1.after ? h1.after.items : []).filter(i => i.name === 'dagger')));
        if (inHand(t3.after).length || lying(t3.after).length !== 1) f.push('the dagger thrown again not on the floor');
        const a2 = o.slice(o.lastIndexOf('> hold the dagger'));
        if (/You lift the dagger\./.test(a2.slice(0, 80))) f.push('held after it was thrown again, its first words a dagger lifted (the UI\'s copy of what\'s carried): ' + a2.slice(0, 120));
        if (!/You have no dagger to hand\.|You go over to the dagger\./.test(a2.slice(0, 200))) f.push('held after it was thrown again, neither told it isn\'t carried nor gone over to: ' + a2.slice(0, 200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0denthrow', near: ['fight'], outfit: true, lostLines: true, what: "Dingus Quest Zero's thing thrown out of a denizen's hand, by the same code as the character's (the user's playtest, 2026-11-20: a dagger thrown, then a block made with it; ONE-PATH.md: every actor alike), by the item store, the referee's requests and the words: dq0throws' small orcs (arak, hostile: seed 2), each made a spearman (its club a spear; no bow nor arrows: the scene's), a mail hauberk put on (fought on till it falls: the game may end there): a small orc throwing its spear at the character; the spear on the floor after it, no one's, the referee told what that orc holds now (RS_EQUIP, its own, after the throw, in the same turn), and no blow of its told with a spear after it",
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml dge 90 >> /ram/c.hm', 'echo ml swd 90 >> /ram/c.hm',
        'mkdir /ram/g', "echo 'faction | Tribe | arak | 0-0 | 20 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk'", 'drop the backpack', 'wear the mail hauberk', 'quit', 'echo $status'] })) },
      start(m) {
        const spears = (h, L, K) => {   // (Each orc's club a spear, its bow and arrows gone: a spearman, to throw)
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), name = k => { let n = ''; for (let j = 0; j < 14 && cat[k * L.kindSize + j]; j++) n += String.fromCharCode(cat[k * L.kindSize + j]); return n; };
          let sp = 0;
          for (let k = 1; k < 255 && !sp; k++) if (name(k) === 'spear') sp = k;
          for (let i = 0; cat && sp && i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (!b || !b[o + L.iKind]) continue;
            const n = name(b[o + L.iKind]);
            if (b[o + L.iWhere] === K.IW_DEN && n === 'club') b[o + L.iKind] = sp;
            if (['shortbow', 'arrows', 'quiver'].includes(n) && (b[o + L.iWhere] === K.IW_DEN || b[o + L.iWhere] === K.IW_ITEM)) b[o + L.iKind] = 0;
          }
        };
        dq0Scene(m, [{ g: 1, line: 'drop the backpack', n: 1, call: spears }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), o = dq0Prose(out), f = [];
        if (process.env.DQ0SHOW) console.log(o);
        const g = T.of("dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk'");
        if (g.k < 0) return ['the game not played'];
        const k = o.search(/The small orc throws (?:their|its) spear at you/);
        if (k < 0) return ['the scene not set: no small orc\'s spear thrown'];
        if (/The small orc (?:strikes|thrusts|jabs|stabs|swings)[^.]*with (?:their|its) spear/.test(o.slice(k + 10, o.indexOf('> quit')))) f.push('a blow with a spear after it was thrown: ' + o.slice(k, k + 600));
        const thrownIn = T.filter(u => u.k > g.k && u.at && u.after).find(u => u.after.items.some(i => i.name === 'spear' && i.where === 'IW_SQUARE' && !i.owner) &&
          !u.at.items.some(i => i.name === 'spear' && i.where === 'IW_SQUARE' && !i.owner));
        if (!thrownIn) return f.concat(['the thrown spear not on the floor, no one\'s']);
        // (The throw: a small orc's blow at the character told to the referee, and the next it hears of that orc what it
        // holds now, before any blow more)
        const reps = thrownIn.replies.filter(r => r.rs === 'RS_ATTACK' || r.rs === 'RS_EQUIP');
        const at = reps.findIndex((r, i) => r.rs === 'RS_ATTACK' && r.actor > 0 && r.other === 0 && reps[i + 1] && reps[i + 1].rs === 'RS_EQUIP' && reps[i + 1].actor === r.actor);
        if (at < 0) f.push('the referee not told what the thrower holds after its throw: ' + reps.filter(r => /RS_ATTACK|RS_EQUIP/.test(r.rs)).map(r => r.rs + ' ' + r.actor + '>' + r.other).join(', '));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0missile', outfit: true, asks: true, near: ['fight'], what: 'Dingus Quest Zero\'s missiles (slice 4.7, 2026-10-29: DUNGEON.md\'s missiles, HOUSE.md\'s, for review; the referee\'s RS_MISSILE, HarnMaster\'s Combat 15-17), by the words, the fights told and the item store: the character (hmchar seed 23, its Bow 90: its outfit a shortbow slung, a quiver of arrows at its belt) in a gambeson and a mail hauberk (Dodge 90), its arrows put down and gathered up again ("You gather up 22 arrows."), put back in the quiver; down seed 5\'s ladder by the giant rats, its torch put down, "shoot the rat": its shortbow taken up first (its round\'s, "You take up your shortbow."), then shot (E_FIGHT, "You shoot at the giant rat with your shortbow"), an arrow the fewer; then with the hand-written theme\'s elves (sindwar, hartbows slung: seed 4), waited for: an elf unslinging its hartbow and shooting at the character from further off than beside it (E_FIGHT by it; "The elf shoots at you with its hartbow")',
      init: 't_rc', cycles: 1900e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml bow 90 >> /ram/c.hm',
        'echo ml dge 90 >> /ram/c.hm', 'echo ml swd 90 >> /ram/c.hm',
        "dq0 -p t -s 5 -c /ram/c.hm -T -k 'gambeson,mail hauberk'", 'wear the gambeson', 'wear the mail hauberk', 'drop the arrows',
        'gather the arrows', 'put the arrows in the quiver', 'd', 'drop the torch', 'hold the shortbow', 'shoot the rat', 'quit',
        'mkdir /ram/g', "echo 'faction | Wardens | sindwar | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 4 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk'", 'drop the backpack', 'wear the gambeson', 'wear the mail hauberk',
        'defend block', ...Array(40).fill('wait'), 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out);
        if (process.env.DQ0SHOW) { console.log(o); for (const t of T) console.log(dq0Evs(t)); }
        const g1 = T.of("dq0 -p t -s 5 -c /ram/c.hm -T -k 'gambeson,mail hauberk'"), g2 = T.of("dq0 -p t -s 4 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk'");
        if (g1.k < 0 || g2.k < 0) return ['the games not played'];
        const arrows = s => dq0Carried(s).filter(i => i.name === 'arrows').reduce((a, i) => a + i.qty, 0);
        const sh = T.of('shoot the rat', g1.k), d0 = T.of('drop the torch', g1.k), dr = T.of('drop the arrows', g1.k), ga = T.of('gather the arrows', g1.k);
        if (!(arrows(dr.after) < arrows(dr.at))) f.push('the arrows not put down (' + arrows(dr.at) + ', ' + arrows(dr.after) + ')');
        if (!(arrows(ga.after) > arrows(ga.at))) f.push('the arrows not gathered (' + arrows(ga.at) + ', ' + arrows(ga.after) + ')');
        if (!dq0Ev(sh, 'E_FIGHT', { who: 0 }).length) f.push('no shot at the rat told (' + dq0Evs(sh) + ')');
        inTurn(o, ['You shoot at the giant rat with your shortbow'], f, 'the character\'s shot: ');
        if (!(arrows(sh.after) < arrows(d0.after))) f.push('no arrow the fewer (' + arrows(d0.after) + ' before, ' + arrows(sh.after) + ' after)');
        if (!/You gather up \d+ arrows?\./.test(o)) f.push('the gathering not told');
        const den = T.filter(u => u.k > g2.k).flatMap(u => u.dens).filter(e => e.kind === 'E_FIGHT' && e.who > 0);
        if (!den.length) f.push('no elf\'s blow or shot (E_FIGHT)');
        inTurn(o, ['The elf unslings their hartbow.', 'The elf takes aim at you.', 'The elf shoots at you with their hartbow'], f, 'the elf\'s shot: ');   // (Its aim told as it shoots, the character's "You take aim" the other way round, 2026-11-18)
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0spiderbite', near: ['fight', 'body'], what: "Dingus Quest Zero's cave spider's venom (2026-11-04, A9: the book has no spider, its figures its nearest kin's, the adder's, and its venom with them, HOUSE.md's the creatures' sheets, for review), by the words: a tough character undefended, a hostile cave spider set beside it on the stratum below; a bite that wounds carries the venom, told as the adder's is",
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Crawlers | spider | 1-1 | 3 | - | 0 | h | -' > /ram/g/theme.txt",
        'dq0 -p t -s 1001 -c /ram/c.hm -T -d /ram/g', 'defend none', 'd', 'wait', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) { dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'cave spider', beside: 4, engage: true }]); },
      check(m, out) {
        const o = dq0Prose(out), f = [];
        if (!/The cave spider bites at you/.test(o)) f.push('no bite: the scene not set');
        if (!/The bite burns: there's venom in it\./.test(o)) f.push('no venom in its bite');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wildlost', near: ['fight'], what: "Dingus Quest Zero's wild miss lost (2026-11-04: C18, the user's \"wild misses should possibly be lost\", HOUSE.md's where it falls; how often the house's, one time in two), by the fights told and the item store: a tough character with no skill at the sling (5), hostile cave adders set beside it, shooting on; sling stones never break, so a stone is gone from the store only by a shot gone wild (E_FIGHT O_FUMBLE): none lost in a turn without one, and of the wild ones some lost and some lying near",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml slg 5 >> /ram/c.hm',
        'echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Serpents | adder | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sling,sling stones'", 'drop the torch', 'hold the sling', 'wait', ...Array(30).fill(['shoot the adder', 'take the sling', 'hold the sling']).flat(), 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) { dq0Scene(m, [{ g: 1, line: 'drop the torch', n: 1, item: { 'sling stones': { qty: 200 } } }, { g: 1, line: 'wait', n: 1, k: 0, beside: 4, engage: true }]); },
      check(m, out) {
        const T = dq0Turns(m), f = [];
        if (process.env.DQ0SHOW) { console.log(dq0Story(out)); for (const t of T) console.log(dq0Evs(t)); }
        const stones = s => s.items.filter(i => i.name === 'sling stones').reduce((a, i) => a + i.qty, 0);
        const shots = T.filter(t => t.line === 'shoot the adder');
        let wild = 0, lost = 0, fired = 0;
        for (const t of shots) {
          const w = dq0Ev(t, 'E_FIGHT', { who: 0 }).filter(e => e.out === 'O_FUMBLE').length, l = stones(t.at) - stones(t.after);
          fired += dq0Ev(t, 'E_FIGHT', { who: 0 }).length;
          wild += w;
          lost += l;
          if (l > w) f.push('a stone gone with no shot gone wild (' + l + ' lost, ' + w + ' wild: ' + dq0Evs(t) + ')');
        }
        if (wild < 2) f.push('too few wild shots to tell: ' + wild + ' of ' + fired);
        if (!lost) f.push('no wild stone lost, of ' + wild);
        if (lost >= wild) f.push('every wild stone lost: ' + lost + ' of ' + wild);
        this.notes = [fired + ' shots, ' + wild + ' wild, ' + lost + ' lost'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0throws', outfit: true, asks: true, near: ['fight'], what: 'Dingus Quest Zero\'s fights complete (slice 4.7\'s left, 2026-10-30: DUNGEON.md\'s fights, HOUSE.md\'s missiles, for review), by the words, the fights told and the item store: the character (hmchar seed 23, its Bow 15: its outfit a sling) shooting at seed 5\'s giant rats down the ladder, fought on, each miss half the time striking a rat beside its mark (the book\'s, a stray: "Your shot strikes the giant rat instead"), every fall told as it falls and each rise after a fall told (no rise without its fall); a bowman (Bow 90: a shortbow, a quiver) its arrows put down and gathered back into its quiver, not its pack; then a hand-written theme\'s small orcs (arak, hostile: a mankar, a club or a spear in hand, a dagger at the belt, half of them a shortbow; seed 2), waited for: one throwing its spear, or putting up its weapon and shooting its shortbow ("The small orc throws its spear at you", "The small orc shoots at you with its shortbow"); then the character set to shoot on its own ("autotarget at medium range"), shot at by the elves (sindwar, hartbows: seed 4) and shooting back unbidden (it fights back, its shortbow taken up, the round the book\'s grope: the bowman\'s)',
      init: 't_rc', cycles: 3400e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', lines: ['echo ml bow 15 >> /ram/c.hm',
        'echo ml dge 90 >> /ram/c.hm', 'echo ml swd 90 >> /ram/c.hm',
        'hmchar -q -s 23 -u feudal -c hunter -o /ram/d.hm', 'echo ml bow 90 >> /ram/d.hm', 'echo ml dge 90 >> /ram/d.hm',
        "dq0 -p t -s 5 -c /ram/c.hm -T -k 'gambeson,mail hauberk'", 'wear the gambeson', 'wear the mail hauberk',
        'd', 'drop the torch', 'drop the staff', 'shoot the rat', ...Array(12).fill('wait'), 'quit', 'dq0 -p t -s 5 -c /ram/d.hm -T', 'drop the arrows', 'gather the arrows', 'quit',
        'mkdir /ram/g', "echo 'faction | Tribe | arak | 0-0 | 20 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk'", 'drop the backpack', 'wear the gambeson', 'wear the mail hauberk',
        'defend block', ...Array(4).fill('wait'), 'quit',
        'mkdir /ram/h', "echo 'faction | Wardens | sindwar | 0-0 | 96 | - | 0 | h | -' > /ram/h/theme.txt",
        "dq0 -p t -s 4 -c /ram/d.hm -T -d /ram/h -k 'sword,gambeson,mail hauberk'", 'drop the backpack', 'wear the gambeson', 'wear the mail hauberk',
        'drop the torch', 'drop the staff', 'drop the sword', 'autotarget at medium range', 'defend block', ...Array(3).fill('wait'), 'quit', 'echo $status'] })) },   // (Its sword put down,
                                                //   dice2: its hands free for its shortbow, as an elf's hit to its forearm had them)
      start(m) {   // (A stray's scene: a second rat kept beside the first as each of game 1's rounds is typed, 2026-11-03;
        //   the world's stray roll rigged, dice2: a miss strikes the one beside, the book's half the time made every time)
        const beside = (h, L, K, at, named) => {
          const u = o => h[o] | h[o + 1] << 8, rats = [...Array(L.nactors).keys()].filter(q => h[at(q) + L.aState] && named(q) === 'giant rat');
          if (rats.length > 1) {   // (On open floor beside it: dq0dentake's way)
            const sn = h[at(rats[0]) + L.aSn], S = stratumRec(m, sn), x0 = u(at(rats[0]) + L.aX), y0 = u(at(rats[0]) + L.aY);
            const tile = (x, y) => { const c = S && liveChunks(m, sn)[((x - S.x0) >> 5) + ',' + ((y - S.y0) >> 5)]; return c ? c[443 + ((y - S.y0) & 31) * 32 + ((x - S.x0) & 31)] & 15 : -1; };
            const busy = (x, y) => [...Array(L.nactors).keys()].some(q => h[at(q) + L.aState] && u(at(q) + L.aX) === x && u(at(q) + L.aY) === y) || (u(L.pcX) === x && u(L.pcY) === y);
            const d = [[0, -1], [1, 0], [0, 1], [-1, 0]].find(([dx, dy]) => [1, 4].includes(tile(x0 + dx, y0 + dy)) && !busy(x0 + dx, y0 + dy));
            if (d) { const x = x0 + d[0], y = y0 + d[1], o = at(rats[1]); h[o + L.aX] = x & 255; h[o + L.aX + 1] = x >> 8; h[o + L.aY] = y & 255; h[o + L.aY + 1] = y >> 8; h[o + L.aSn] = sn; const R = dq0Rec(m), q = R.at(h[o + L.aDen]); R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8; R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8; h[L.denSeq] = h[L.denSeq] + 1 & 255; }
          }
        };
        dq0Scene(m, [{ g: 1, line: 'shoot the rat', n: 1, call: beside, rig: [{ kind: 'WORLD', a: 'WD_SHOT', faces: [1] }] }, ...Array.from({ length: 12 }, (_, i) => ({ g: 1, line: 'wait', n: i + 1, call: beside }))]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out);
        if (process.env.DQ0SHOW) { console.log(o); for (const t of T) console.log(dq0Evs(t)); }
        const g1 = T.of("dq0 -p t -s 5 -c /ram/c.hm -T -k 'gambeson,mail hauberk'"), g2 = T.of("dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k 'sword,gambeson,mail hauberk'"),
          g3 = T.of("dq0 -p t -s 4 -c /ram/d.hm -T -d /ram/h -k 'sword,gambeson,mail hauberk'");
        if (g1.k < 0 || g2.k < 0 || g3.k < 0) return ['the games not played'];
        const s1 = o.slice(o.indexOf("> shoot the rat"), o.indexOf('/ram/g -k')), s2 = o.slice(o.indexOf('/ram/g -k'), o.indexOf('/ram/h -k')),
          s3 = o.slice(o.indexOf('/ram/h -k'));
        // (A stray: a miss striking one beside the mark)
        if (!/Your shot strikes (?:the giant rat|something) instead/.test(s1)) f.push('no stray told (a miss striking a rat beside its mark)');
        // (Every fall told, each rise after one)
        const rises = [...s1.matchAll(/The giant rat (?:scrambles to its feet|goes down)\./g)].map(x => x[0]);
        let down = 0;
        for (const r of rises) {
          if (/goes down/.test(r)) down++;
          else if (down-- <= 0) { f.push('a rat\'s rise with no fall told before it: ' + rises.join(' ')); break; }
        }
        // (The arrows gathered into the quiver)
        const ga = T.of('gather the arrows', T.of('dq0 -p t -s 5 -c /ram/d.hm -T').k), inQuiver = s => (s ? s.items : []).filter(i => i.name === 'arrows' && i.where === 'IW_ITEM')
          .filter(i => { const c = s.items.find(j => j.n === i.x || j.i === i.x); return c && c.name === 'quiver'; }).reduce((a, i) => a + i.qty, 0);
        if (!/You gather up \d+ arrows?\./.test(o)) f.push('the gathering not told');
        if (ga.after && !(inQuiver(ga.after) > inQuiver(ga.at))) f.push('the arrows not gathered into the quiver (' + inQuiver(ga.at) + ', ' + inQuiver(ga.after) + ')');
        // (The soldiers: a spear thrown, or a crossbow taken up and shot)
        if (!/The small orc (?:throws their (?:spear|dagger) at you|shoots at you with their shortbow)/.test(s2)) f.push('no small orc\'s throw or shot');
        // (Auto-target: the character shooting back unbidden)
        inTurn(s3, ['You shoot at hostiles within medium range on your own.', 'You fight back against the elf.', 'You take up your shortbow.'], f, 'the auto-target: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dens', outfit: true, what: 'Dingus Quest Zero\'s beasts\' dens and the dead\'s lairs (the outfits\' plan, denizens.md 3; the gear3 agent\'s, 2026-10-29; HOUSE.md\'s the denizens\' belongings, for review), by the item store\'s records: the hand-written theme\'s giant rats (dq0senseless\'s, seed 3) their nests made as their lairs are placed, each a heap of straw with bones on its square, their faction\'s, kept with the game (dq0 -d: the same things where they were, carried on); seed 7\'s crypt below (dq0armour\'s ghoul) a ghoul\'s nest, bones and skulls together, what lies there found (Treasure 1\'s Found column: some of it worn, its durability short of its whole)',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['mkdir /ram/g',
        'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt', 'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'look', 'quit', 'dq0 -d /ram/g', 'look', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -T', 'd', 'look', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [];
        const g1 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g'), q1 = T.of('quit', g1.k), g2 = T.of('dq0 -d /ram/g'), q2 = T.of('quit', g2.k);
        const g3 = T.of('dq0 -p t -s 7 -c /ram/c.hm -T'), q3 = T.of('quit', g3.k);
        if (!q1.at || !q2.at || !q3.at) return ['the games not played'];
        const lying = (s, n) => s.items.filter(i => i.name === n && i.where === 'IW_SQUARE' && i.owner);
        const nests = s => lying(s, 'heap of straw').filter(h => lying(s, 'bones').some(b => b.sn === h.sn && b.x === h.x && b.y === h.y));
        const n1 = nests(q1.at), n2 = nests(q2.at);
        if (!n1.length) f.push('no rats\' nest (a heap of straw, bones on its square, its faction\'s)');
        const sig = l => JSON.stringify(l.map(i => [i.sn, i.x, i.y]).sort());
        if (sig(n1) !== sig(n2)) f.push('the nests not kept with the game: ' + sig(n1) + ' then ' + sig(n2));
        const s3 = q3.at, skulls = lying(s3, 'skull'), nest = skulls.find(k => lying(s3, 'bones').some(b => b.sn === k.sn && b.x === k.x && b.y === k.y));
        if (!nest) f.push('no ghoul\'s nest (skulls and bones together): ' + JSON.stringify(skulls));
        else {
          const there = s3.items.filter(i => i.where === 'IW_SQUARE' && i.owner === nest.owner && i.sn === nest.sn && i.x === nest.x && i.y === nest.y);
          if (!there.some(i => i.durMax && i.dur < i.durMax)) f.push('nothing in the ghoul\'s nest worn (Found): ' + JSON.stringify(there.map(i => [i.name, i.dur, i.durMax])));
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0armour', what: 'Dingus Quest Zero\'s denizens fighting and armour (DENIZENS.md: fights; the catalogue\'s armour kinds, worn: the things tell the referee what\'s worn and how well made, RS_EQUIP\'s marks; HarnMaster\'s protection by location, the layers summed, Combat 4-5), by the blows told and the character\'s state (the referee\'s dice at this seed): a gambeson under a mail hauberk (dq0 -k) worn, the character a poor fighter (Unarmed 5, its torch put down, a lantern lit hung from its belt: the room below dark, nobody keeping it lit since slice 4.1); down the stairway at seed 7 a ghoul (its faction hostile to the character) seen as the character comes down, coming up to it and clawing at it as it waits, fought back with bare fists (HOUSE.md\'s fighting on, the user\'s, 2026-10-20); dodging (defend dodge: its stance), a claw dodged; seed 7\'s rat set on it too (2026-11-04: the book\'s ghoul\'s 7 and a die mostly go through mail), a blow turned by the armour in one game or the other; then a game standing undefended (defend none), wounded, worse, knocked senseless, then clawed till it\'s killed: the character\'s death, the game\'s end (dq0ghoul\'s, in it; since fighting on, two games, the stance set before each, and the character fighting back)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml una 5 >> /ram/c.hm',
        ...['dodge', 'none'].flatMap(d => ['dq0 -p t -s 7 -c /ram/c.hm -k \'gambeson,mail hauberk,lantern\' -T', 'wear the gambeson', 'wear the mail hauberk', 'light the lantern', 'hang the lantern', 'drop the torch', 'd',
          'defend ' + d, 'wait', 'quit']), 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {        // (The book's ghoul's 7 and a die mostly go through mail, 2026-11-04: seed 7's rat set on it too, its small bite the armour's to turn)
        dq0Scene(m, [...[1, 2].map(g => ({ g, line: 'wait', n: 1, name: 'giant rat', beside: 2, engage: true })),
          { g: 1, line: 'wait', n: 1, call: (h, L, K, at, named, sc) => {   // (Dodging: the ghoul's third blow, the first of a round the rat's
            const k = [...Array(L.nactors).keys()].find(j => named(j) === 'ghoul');   //   bite hasn't taken the dodge from, both dice a pass: dodged)
            sc.rig([{ kind: 'ATTACK', actor: k, uses: 2, faces: [] }, { kind: 'ATTACK', actor: k, uses: 1, faces: [1, 1] }]);
          } }]);
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], FF_A_DOWN = 1, FF_A_DEAD = 2, FF_D_DOWN = 4, FF_D_DEAD = 8, FF_STOPPED = 0x40;
        const worn = (t, n) => { const i = dq0Thing(t.after, n); if (!i || !(i.flags & DQ0_I.WORN)) f.push(n + ' not worn'); };
        worn(T.of('wear the gambeson'), 'gambeson');
        worn(T.of('wear the mail hauberk'), 'mail hauberk');
        const d = T.of('d'), ghoul = d.after && d.after.actors.find(a => a.name === 'ghoul');
        if (!d.after || !d.after.sight || !d.after.sight.dens.some(x => x.name === 'ghoul')) f.push('the ghoul not seen as the character came down');
        if (!ghoul) return [...f, 'no ghoul'];
        const claws = t => dq0Ev(t, 'E_FIGHT', { def: 0, who: w => w === ghoul.k + 1 });
        const dd = T.of('defend dodge'), dn = T.of('defend none'), w1 = T.after(dd, 'wait'), w2 = T.after(dn, 'wait');
        if (!dd.after || dd.after.pc.stance !== 'D_DODGE') f.push('the stance not dodging: ' + (dd.after && dd.after.pc.stance));
        if (!dn.after || dn.after.pc.stance !== 'D_NONE') f.push('the stance not none: ' + (dn.after && dn.after.pc.stance));
        if (w2.k < 0) return [...f, 'the waits not typed'];
        // (Dodging: a claw dodged, a blow turned by the armour, the ghoul fought back)
        dq0Need(f, w1, 'E_TASK', { te: 'TE_FIGHT', by: ghoul.k }, 'the ghoul not fought back');
        if (!claws(w1).some(e => e.out === 'O_DODGED')) f.push('no claw dodged (' + dq0Evs(w1) + ')');
        if (![w1, w2].some(w => dq0Ev(w, 'E_FIGHT', { def: 0 }).some(e => e.ff & FF_STOPPED))) f.push('no blow turned by the armour (' + dq0Evs(w1) + ')');
        // (Undefended: wounded, worse, knocked senseless (a claw, or its counterstrike at the character's blow), then
        // clawed as it lies till it's killed: since slice 4.0 the character busy till it comes round, its death in the
        // wait's own turn)
        const all = dq0Ev(w2, 'E_FIGHT', {}), hurt = all.filter(e => e.def === 0 && e.who === ghoul.k + 1 && e.out === 'O_HIT' && !(e.ff & FF_STOPPED));
        if (claws(w2).some(e => e.out === 'O_DODGED' || e.out === 'O_BLOCKED')) f.push('undefended, a claw dodged or blocked');
        if (hurt.length < 2) f.push('undefended, not wounded, then worse (' + dq0Evs(w2) + ')');
        const down = all.findIndex(e => (e.def === 0 && (e.ff & FF_D_DOWN)) || (e.who === 0 && (e.ff & FF_A_DOWN)));
        const dead = all.findIndex(e => (e.def === 0 && (e.ff & FF_D_DEAD)) || (e.who === 0 && (e.ff & FF_A_DEAD)));
        if (down < 0) f.push('undefended, the character not knocked senseless (' + dq0Evs(w2) + ')');
        if (dead < 0 || dead < down) f.push('the character, down, not killed by the ghoul\'s claws');
        else {
          if (!w2.after || !(w2.after.sum.cond & 4)) f.push('the character not dead (C_DEAD)');
          dq0Need(f, w2, 'E_BYE', {}, 'the game not ended by the death');
        }
        // (Its black-out told once, and the ghoul's blows fumbled at it as it lies misses: the Ignore table's tactical
        // advantage none it can take, 2026-10-20; it was "you see an opening")
        const said = dq0Prose(out), g2 = said.slice(said.lastIndexOf('> defend none')), black = g2.indexOf('Everything goes black.');
        if (black < 0 || g2.indexOf('Everything goes black.', black + 1) >= 0) f.push('the black-out not told once');
        else if (g2.slice(black).includes('you see an opening')) f.push('the character, senseless, seeing an opening');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0senseless', initiative: true, what: "Dingus Quest Zero's character knocked senseless (the faults the engage work found, fixed 2026-10-20; since 2026-10-21 a helpless character's foes by their nature, the user's): the hand-written theme's hostile rats (inhabit.txt, seed 3), the character a poor fighter (Unarmed 5) undefended (defend none) fighting back (HOUSE.md's fighting on), with the engage initiative test on: knocked senseless, its black-out told once; the rats then leaving it be (giant rats only defend themselves: kinds.txt's ways), not a blow at it as it lies, nor as it comes round in shock, in no state to fight on, and waits (it was bitten as it lay, and as it waited); no blow at it as it lies seen as an opening (the Ignore table's tactical advantage none it can take: it was 'you see an opening'); every wound's level in digits, its own (the book's M1 to G5: it was \"(G:)\", and past the record's twelve the running total), the rats' worded bites",
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml una 5 >> /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'defend none', ...Array(12).fill('wait'), 'quit',
        'echo $status'] })) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], FF_A_DOWN = 1, FF_D_DOWN = 4;
        const all = T.flatMap(t => dq0Ev(t, 'E_FIGHT', {})), down = e => (e.def === 0 && (e.ff & FF_D_DOWN)) || (e.who === 0 && (e.ff & FF_A_DOWN));
        const first = all.findIndex(down), lain = first < 0 ? [] : all.slice(first + 1).filter(e => e.def === 0 && e.out !== 'O_MISS');
        const out = m.out.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '').replace(/\s+/g, ' ');
        const black = (out.match(/Everything goes black\./g) || []).length;
        const levels = [...out.matchAll(/a (?:minor|serious|grievous) [a-z ]+ to (?:your|its) [a-z ]+ \(([^)]*)\)/g)].map(x => x[1]);
        const bites = [...out.matchAll(/(?:The giant rat|Something) bites at you: a (?:minor|serious|grievous) ([a-z ]+?) to your/g)].map(x => x[1]);
        if (first < 0) f.push('the character not knocked senseless');
        if (lain.length) f.push('struck at as it lay, or in shock after (' + lain.length + ' blows): the rats only defend themselves');
        if (black !== 1) f.push('"Everything goes black." told ' + black + ' times (knocked senseless ' + all.filter(down).length + ' times, struck ' + lain.length + ' times as it lay)');
        const bad = levels.filter(l => !/^[MSG][0-9]+$/.test(l));
        if (bad.length) f.push('injury levels not in digits: ' + bad.join(', '));
        const own = { M: [1], S: [2, 3], G: [4, 5] }, odd = levels.filter(l => !own[l[0]] || !own[l[0]].includes(+l.slice(1)));
        if (odd.length) f.push('a wound not at its own level (the book\'s M1 to G5): ' + odd.join(', '));
        if (!bites.length || bites.some(b => !/^(?:deep )?bite$/.test(b))) f.push('the rats\' wounds not worded bites: ' + bites.join(', '));
        if (out.slice(out.indexOf('Everything goes black.')).includes('you see an opening')) f.push("the character, senseless, seeing an opening (the Ignore table's advantage)");
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0helpless', what: "Dingus Quest Zero's wounds and a foe that only defends itself (the user's adder playtest, 2026-10-21: \"For some reason the wounds just kept getting worse and worse\", and its bites worded stabs; DECISIONS.md's a helpless character): a theme's hostile cave adders (written by hand: a faction of them on the top stratum; seed 1), the character tough (STR, STA and WIL 99: Endurance 99, its shock rolls holding long) and undefended (defend none, Dodge 5, Unarmed 5), its torch put down, two of them set on it where it stands (the scene's, dq0Poke's): each bite worded a bite, never a stab (a creature's own part: HOUSE.md's wounds), each wound's own level (M1 to G5, the book's: it was the running total past the record's twelve, S12 ... S37), more than twelve of them, every level of them in the universal penalty; its shock roll failed at last, senseless (\"Everything goes black.\", OUT on the status line); the adders then going, fleeing, with not another blow (one that only defends itself: kinds.txt's ways, the user's by its nature), one posted as it fell held (the referee's: no killer, RS_KILLER); come round in shock, in no state to fight, left be; its state's words the record's real injuries, the lesser hurts never named the worst (\"in shock: 11 injuries and lesser hurts, the worst a serious bite to ...\"), the same as the game's carried on (what dealt each injury kept with its sheet)",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml dge 5 >> /ram/c.hm', 'echo ml una 5 >> /ram/c.hm',
        'echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Serpents | adder | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g', 'defend none', 'drop the torch', ...Array(24).fill('wait'), 'status', 'quit', 'dq0 -d /ram/g', 'status', 'quit', 'echo $status'] })) },
      start(m) {        // (The scene set, 2026-11-03: two adders set on it where it stands, fighting it: their way to it the dice's; and as it falls senseless,
                        // out a minute and in shock after, not as the shock rolls happen to fall)
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, k: 0, beside: 4, engage: true }, { g: 1, line: 'wait', n: 1, k: 1, beside: 6, engage: true }]);
        dq0OnWords(m, [1, 2, 3, 4, 5, 6, 7, 8].map(n => ({ g: 1, line: 'wait', n, words: 'Everything goes black.', call: () => dq0SceneNow(m, () => dq0PcShock(m, 600), null, 'dq0helpless\'s shock, wait #' + n) })));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], FF_A_DOWN = 1, FF_D_DOWN = 4, s = dq0Story(out);
        const all = T.flatMap(t => dq0Ev(t, 'E_FIGHT', {}));
        const down = all.map(e => !!((e.def === 0 && (e.ff & FF_D_DOWN)) || (e.who === 0 && (e.ff & FF_A_DOWN)))).lastIndexOf(true);   // (The last fall: slice 4.3's venom, more than one)
        const bites = [...s.matchAll(/(?:The cave adder|Something) bites at you: a (minor|serious|grievous) ([a-z ]+?) to your [a-z ]+? \(([MSG])(\d+)\)/g)];
        if (bites.length < 13) f.push('not more than twelve wounds (' + bites.length + ')');
        const odd = bites.filter(b => !/^(?:deep )?bite$/.test(b[2]) || b[1][0].toUpperCase() !== b[3] ||
          !({ M: [1], S: [2, 3], G: [4, 5] })[b[3]].includes(+b[4]));
        if (odd.length) f.push('wounds not worded a bite, at their own levels: ' + odd.map(b => b[0]).join('; '));
        if (/ stab /.test(s)) f.push('a stab told: ' + s.match(/[^.]* stab [^.]*\./)[0]);
        const up = bites.reduce((n, b) => n + +b[4], 0) + (s.match(/It spreads to your/g) || []).length, pen = s.match(/Condition: in shock: (\d+) injuries and lesser hurts, the worst a (serious|grievous) ((?:deep )?bite) to your [a-z ]+? \(([SG])(\d)\)[^.]*penalties: universal (\d+)/);
        if (!pen) f.push('the state not "in shock: N injuries and lesser hurts, the worst a ... bite ...": ' + JSON.stringify((s.match(/Condition: [^.]*\./) || [''])[0]));
        else {
          if (+pen[1] !== 11) f.push('the real injuries not eleven beside the lesser hurts: ' + pen[1]);
          if (+pen[6] !== up) f.push('the universal penalty not every level told (' + up + '): ' + pen[6]);
        }
        if (down < 0) return [...f, 'the character never knocked senseless'];
        if (!/Everything goes black\..*UP \d+ OUT/.test(s)) f.push('OUT not on the status line as it lay senseless');
        const after = all.slice(down + 1).filter(e => e.def === 0);
        if (after.length) f.push('struck at after it fell: ' + after.map(e => e.out).join(', '));
        const ko = all[down], kt = T.find(t => t.dens.includes(ko) || t.events.includes(ko)), pc = kt && kt.after && kt.after.pc;   // (Unseen as it lay senseless, the user's, 2026-11-14: their going off by their moves)
        if (!pc || !T.flatMap(t => t.dens).some(e => e.kind === 'E_DEN' && e.de === 'DE_MOVED' && e.t > ko.t && Math.max(Math.abs(e.sx - pc.x), Math.abs(e.sy - pc.y)) >= 2))
          f.push('the adders not gone off once it was down');
        if (/Everything goes black\.[^>]*fleeing\./.test(s)) f.push('the adders seen fleeing as it lay senseless');
        if (!/You're in no state to fight\./.test(s)) f.push('not in shock after (no state to fight)');
        const cond = [...s.matchAll(/Condition: [^.]*\./g)].map(x => x[0]);   // (Kept: the game carried on, its words the same, a bite a bite)
        if (cond.length !== 2 || cond[0] !== cond[1]) f.push('its state not the same carried on (what dealt each injury kept): ' + JSON.stringify(cond));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0finished', what: "Dingus Quest Zero's helpless character and a killer (the user's, 2026-10-21: by its nature, a killer or a predator finishes it, as the character finishes a senseless creature): seed 7's ghoul (down the way at the start), the character poor (Dodge 5, Unarmed 5) and undefended (defend none): the ghoul's claws worded a rake (a creature's own part: HOUSE.md's wounds), its bite a bite, never a gash or a stab; the character knocked senseless, then finished where it lies (the referee's dispatch for a killer, RS_KILLER: its mortal wound unfelt as it lies senseless since 2026-11-14, the user's, \"Everything goes black. You are dead.\"), dead, the game's end",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml dge 5 >> /ram/c.hm', 'echo ml una 5 >> /ram/c.hm',
        'dq0 -p t -s 7 -c /ram/c.hm -T', 'defend none', 'd', ...Array(12).fill('wait'), 'echo $status'] })) },
      start(m) {   // (The ghoul's first two blows rigged, dice2: each a marginal failure, the Ignore table's one die, a 6, at the thorax (50):
                   //   a grievous rake (G4), no mortal one, its shock the dice's: senseless at the second)
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: (h, L, K, at, named, sc) => {
          const k = [...Array(L.nactors).keys()].find(j => named(j) === 'ghoul');
          if (k !== undefined) sc.rig([{ kind: 'ATTACK', actor: k, uses: 2, faces: [99, 6, 50] }]);
        } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], FF_A_DOWN = 1, FF_D_DOWN = 4, FF_D_DEAD = 8, s = dq0Prose(out);
        const all = T.flatMap(t => dq0Ev(t, 'E_FIGHT', {}));
        const blows = [...s.matchAll(/The ghoul (claws|bites) at you: a (minor|serious|grievous|mortal) ([a-z ]+?) to your/g)];
        if (!blows.length) return ['the ghoul struck no wound'];
        const odd = blows.filter(b => !(b[1] === 'claws' ? /^(?:scratch|rake|deep rake)$/ : /^(?:bite|deep bite)$/).test(b[3]));
        if (odd.length) f.push('the ghoul\'s wounds not worded by its part: ' + odd.map(b => b[0]).join('; '));
        const down = all.findIndex(e => (e.def === 0 && (e.ff & FF_D_DOWN)) || (e.who === 0 && (e.ff & FF_A_DOWN)));
        const dead = all.findIndex(e => e.def === 0 && (e.ff & FF_D_DEAD));
        if (down < 0) f.push('the character never knocked senseless');
        if (dead < 0 || dead < down) return [...f, 'the character, senseless, not finished'];
        if (all[dead].out !== 'O_HIT' || !/Everything goes black\.[^.]*You are dead\./.test(s))   // (Its death blow unfelt as it lay senseless, its death told: the user's, 2026-11-14)
          f.push('not finished where it lay, its death told: ' + all[dead].out + ' ' + JSON.stringify((s.match(/[^.]*black[^.]*\.[^.]*\./) || [''])[0]));
        if (/mortal/.test(s)) f.push('its death blow felt as it lay senseless: ' + s.match(/[^.]*mortal[^.]*\./)[0]);
        const t = T.find(t => dq0Ev(t, 'E_FIGHT', { def: 0 }).some(e => e.ff & FF_D_DEAD));
        if (t) dq0Need(f, t, 'E_BYE', {}, 'the game not ended by the death');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0robbed', what: "Dingus Quest Zero's helpless character and a robber (the user's, 2026-10-21: by its nature, a cunning foe may rob it; HOUSE.md's: what it holds, but a light lit, and its pennies, off to its lair, and it goes): a theme's hostile bandits (written by hand: seed 1, south of the start; the character walking on east toward them, as the dice now fall: they keep to their room, watching), the character poor (Dodge 5, Unarmed 5, Dagger 5) and undefended (defend none), a dagger in hand beside its lit torch: struck senseless, robbed (E_TASK TE_ROBBED, the robber's; unseen as it lies senseless, found as it comes round since 2026-11-14: \"You come round to find someone has been through your things.\"), the dagger and its pennies taken off to one square away from it (the bandits' lair's), its torch still in its hand, still lit; no blow at it after; it lives; the referee told its load left at once (the user's, 2026-10-22: \"Weight should be recounted immediately after a robbery\": the world's RS_EQUIP 2 as it's robbed, its load less the dagger and the pennies, the same as the UI's own once it's come round, puts its torch down and takes it up again (since slice 4.3, come round in shock, a thing not taken up in shock: its torch let fall and not taken up, the load's check that one's alone); and its encumbrance the referee's the same).  The scene set (since the denizens' day, 2026-10-31: bandits keep watches, a third asleep): no day (the environment's dq0hour=128, rc's, before its games: dq0d's), every one about as before",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml dge 5 >> /ram/c.hm', 'echo ml una 5 >> /ram/c.hm',
        'echo ml dag 5 >> /ram/c.hm', 'mkdir /ram/g', 'echo hour 128 >> /ram/s.sc', "echo 'faction | Brigands | bandit | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -S /ram/s.sc',
        'defend none', 'hold the dagger', 'open the door', 'go through it', 'east', ...Array(12).fill('wait'), 'drop the torch', 'take the torch', 'quit', 'echo $status'] })) },
      start(m) {   // (Re-aimed, dice2: the bandits keep off now, watching; one put before it, hostile and fighting it, as the first wait's
                   //   typed, its first two blows rigged: each a marginal failure, the Ignore table's one die (a 6, then a 1) at the thorax
                   //   (50), the shock roll's first die a 6: an S3, then an S2 that fells it, out a few rounds (its robber not home yet))
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, call: (h, L, K, at, named, sc) => {
          const k = [...Array(L.nactors).keys()].find(j => named(j) === 'bandit');
          if (k !== undefined) sc.rig([{ kind: 'ATTACK', actor: k, uses: 1, faces: [99, 6, 50, 6] }, { kind: 'ATTACK', actor: k, uses: 1, faces: [99, 1, 50, 6] }]);
        } }, { g: 1, line: 'wait', n: 1, name: 'bandit', beside: 'face', engage: true }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], FF_A_DOWN = 1, FF_D_DOWN = 4, s = dq0Prose(out), I = DQ0_I;
        const h = T.of('hold the dagger'), q = T.of('quit');
        if (!h.after || !q.at) return ['the game not played'];
        const before = dq0Carried(h.after), dag = dq0Named(before, 'dagger')[0], pen = dq0Named(before, 'pennies')[0];
        if (!dag || !(dag.flags & I.HELD) || !pen) return ['no dagger in hand, or no pennies, to begin with'];
        const all = T.flatMap(t => dq0Ev(t, 'E_FIGHT', {})), robs = T.flatMap(t => dq0Ev(t, 'E_TASK', { te: 'TE_ROBBED' }));
        const down = all.findIndex(e => (e.def === 0 && (e.ff & FF_D_DOWN)) || (e.who === 0 && (e.ff & FF_A_DOWN)));
        if (down < 0) return ['the character never knocked senseless'];
        if (robs.length !== 1) f.push('not robbed once: ' + robs.length);
        else if (!q.at.actors.some(a => a.k === robs[0].by && a.name === 'bandit')) f.push('robbed by no bandit: ' + robs[0].by);
        if (!/You come round to find someone has been through your things\./.test(s)) f.push('the robbery not told as it comes round');   // (Unseen as it lay senseless: the user's, 2026-11-14)
        if (/goes through your things as you lie helpless/.test(s)) f.push('the robbery seen as it lay senseless');
        const now = dq0Carried(q.at), shock = /take the torch You're in shock: you can do no more than rest/.test(s);
        const torch = (shock ? dq0Carried(T.of('drop the torch').at) : now).find(i => i.name === 'torch' && (i.flags & I.HELD));   // (In shock: as it came round)
        if (dq0Named(now, 'dagger').length || dq0Named(now, 'pennies').length) f.push('the dagger or the pennies still carried');
        if (!torch || !(torch.flags & I.LIT)) f.push('its torch not in hand, lit');
        // (The takings carried off on the robber's body, its takings (I_LOOT), home to its lair, put down there as it gets
        //   there: the user's C10, 2026-11-23; till then, so one killed on the way gives them back)
        const rtk = T.find(t => dq0Ev(t, 'E_TASK', { te: 'TE_ROBBED' }).length), rober = robs[0] && rtk && rtk.after && rtk.after.actors.find(a => a.k === robs[0].by);
        const on = (s, i) => { const r = dq0Root(s, i); return r.where === 'IW_DEN' && rober && r.did === rober.did && (i.flags & 0x10); };
        const took = rtk && rtk.after ? rtk.after.items.filter(i => i.n === dag.n || i.n === pen.n) : [];
        if (took.length !== 2 || !took.every(i => on(rtk.after, i))) f.push('the takings not on the robber as it robbed (its own, I_LOOT): ' + JSON.stringify(took.map(i => [i.name, i.where, i.flags, dq0Root(rtk.after, i).did])) + ' ' + JSON.stringify(rober));
        const lay = q.at.items.filter(i => i.where === 'IW_SQUARE' && (i.n === dag.n || i.n === pen.n)), still = q.at.items.filter(i => (i.n === dag.n || i.n === pen.n) && on(q.at, i));
        if (still.length !== 2 && (lay.length !== 2 || lay[0].x !== lay[1].x || lay[0].y !== lay[1].y || (lay[0].x === q.at.pc.x && lay[0].y === q.at.pc.y)))
          f.push('the dagger and the pennies neither on the robber nor put down together at its lair: ' + JSON.stringify(lay.map(i => [i.name, i.x - q.at.pc.x, i.y - q.at.pc.y])));
        this.notes = ['the takings at the end: ' + (still.length === 2 ? 'on the robber' : 'at its lair')];
        if (all.slice(down + 1).some(e => e.def === 0)) f.push('struck at after it fell');
        if (/You are dead\./.test(s)) f.push('the character killed');
        const RSE = m.dq0.layout().K.RS_EQUIP, eq = (t, slot) => t.replies.filter(r => r.kind === RSE && r.a === 2 && r.req === slot);
        const rt = T.find(t => dq0Ev(t, 'E_TASK', { te: 'TE_ROBBED' }).length), tk = T.of('take the torch');
        if (rt && rt.after && !dq0Carried(rt.after).some(i => i.name === 'torch' && (i.flags & I.HELD) && (i.flags & I.LIT))) f.push('its torch not in hand, lit, as it was robbed');
        const was = eq(h, 'slot1').pop(), told = rt ? eq(rt, 'slot0').filter(r => !r.actor) : [], again = eq(tk, 'slot1').pop();   // (The character's: the robber's own told too, 2026-11-23)
        if (told.length !== 1) f.push('the robbed load not told the referee as it was robbed (the world\'s RS_EQUIP 2): ' + told.length);
        else if (!was || (!shock && !again) || told[0].w1 >= was.w1 || (!shock && told[0].w1 !== again.w1))
          f.push('the load told as it was robbed not what was left: before ' + (was && was.w1) + ', robbed ' + told[0].w1 + ', its torch put down and taken up again ' + (again && again.w1));
        if (!shock && rt && tk.after && rt.after && rt.after.sum.load !== tk.after.sum.load) f.push('the referee\'s encumbrance after the robbery ' + rt.after.sum.load + ', not ' + tk.after.sum.load);
        // (Its shock rolls as their time comes, a round each (the user's, 2026-11-21, felled1): the world's RS_ROUSE asked as it
        // lay, at each round's end, senseless still a round on at most (the reply's w2), then come round (0))
        const RSR = m.dq0.layout().K.RS_ROUSE, rouses = T.flatMap(t => t.replies.filter(r => r.req === 'slot0' && r.kind === RSR && r.actor === 0));
        if (rouses.length < 2 || rouses.slice(0, -1).some(r => !r.w2 || r.w2 > 100) || rouses[rouses.length - 1].w2 !== 0)
          f.push('its shock rolls not asked a round each as it lay (RS_ROUSE: ' + rouses.map(r => r.w2).join(', ') + ')');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0disengage', what: "Dingus Quest Zero's disengaging and a fight's tiring (the user's, 2026-10-21: \"For disengaging, try to have it follow the book\"; \"Tiring for combat should happen at the standard strenous work rate\"; HOUSE.md's, the book's Combat 7): seed 7's ghoul, the character weak with its sword (20; its Stamina and Will 30, 2026-11-04, so the book's ghoul doesn't finish it before the pass is told) in a gambeson and a mail hauberk, two of its three rations put down (its load as it was before a ration weighed a pound: slice 4.1's counted lots), two turns about first (2026-10-26: its armour got out of its pack, the reach's times, its dice as a search of the variants found them); disengage and flee before anything engages it refused, told (not engaged; nothing near to flee); the ghoul fought, a disengage typed as it fights (at its first blow's words) taking the fight over (its round out), the character backing away a square and more, no blow of its own after it, the fight not fought on (the ghoul left behind, out of its sight); a flight as the ghoul engages it again, a square back and still engaged (stopped there, told once: \"You stop walking.\"), fought on; pass typed as it fights (at its next blow's words), a round given up, no blow of its own; a flight at its prompt breaking away into a corner, no run on into the wall (since 2026-10-21 a second game's, the character in shock from the start, as the first's was when it fled: in the first, knocked senseless as it fights on after the pass, it's finished, the ghoul a killer, the user's helpless character); every round it fought charged to the referee as a round of strenuous work (RS_EXERT GA_WORK, 100 tenths: the book's second variant of combat fatigue, every round of active combat, whatever is done in it), each blow it struck, each struggle up from the ground, and the pass (begun engaged, the ghoul beside it).  The scene set (since the turns pace waits for a command being given, 2026-10-31, the ghoul no longer came back after the disengage on its own): the ghoul put back beside the character, facing it, hostile and fighting, as the flight is typed at its prompt (dq0Poke's beside and engage), the disengage and the pass typed on the fight's words (dq0OnWords), a look on the pass's (so the fighting on after the pass is the look's)",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 20 >> /ram/c.hm', 'echo att sta 30 >> /ram/c.hm', 'echo att wil 30 >> /ram/c.hm',
        "dq0 -p t -s 7 -c /ram/c.hm -k 'sword,gambeson,mail hauberk' -T", 'turn around', 'turn around', 'wear the gambeson', 'wear the mail hauberk', 'drop 2 rations', 'disengage', 'flee',
        'd', 'wait', 'flee', 'quit', 'cp /ram/c.hm /ram/s.hm', 'echo vol 0 0 0 2 0 >> /ram/s.hm', "dq0 -p t -s 7 -c /ram/s.hm -k 'sword,gambeson,mail hauberk' -T",
          'wear the gambeson', 'wear the mail hauberk', 'drop 2 rations', 'd', 'wait', 'flee', 'look', 'quit', 'echo $status'] })) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'wait', n: 1, words: 'You strike at the ghoul', text: 'disengage\r' },
          { g: 1, line: 'flee', n: 2, words: 'You strike at the ghoul', text: 'pass\r' },
          { g: 1, line: 'pass', n: 1, words: 'Time passes.', text: 'look\r' }]);
        dq0Scene(m, [{ g: 1, line: 'flee', n: 2, name: 'ghoul', beside: 'face', engage: true }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], K = m.dq0.layout().K;
        const mine = t => dq0Ev(t, 'E_FIGHT', { who: 0 }), brk = t => dq0Ev(t, 'E_TASK', { te: 'TE_BREAK' });
        const d1 = T.of('disengage'), fl1 = T.of('flee');
        dq0Need(f, d1, 'E_TASK', { te: 'TE_FREE', none: 'engaged' }, 'a disengage not engaged not refused');
        dq0Need(f, fl1, 'E_TASK', { te: 'TE_FREE', none: 'near' }, 'a flight with nothing near not refused');
        const d2 = T.of('disengage', d1.k + 1), b2 = brk(d2);
        if (b2.length !== 1 || b2[0].how !== 'back') return [...f, 'the disengage as it fought not begun (' + dq0Evs(d2) + ')'];
        dq0Need(f, d2, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_FIGHT' }, 'the fight not taken over by the disengage');
        if (mine(d2).some(e => e.t > b2[0].t)) f.push('a blow struck as it backed away');
        if (dq0Ev(d2, 'E_TASK', { te: 'TE_FIGHT' }).some(e => e.t > b2[0].t)) f.push('fought on after the disengage');
        if (!d2.after || !d2.at || (d2.after.pc.x === d2.at.pc.x && d2.after.pc.y === d2.at.pc.y)) f.push('the disengage moved it nowhere');
        const ps = T.of('pass');
        if (ps.k < 0 || mine(ps).length) f.push('pass not a round given up (' + (ps.k < 0 ? 'not typed' : dq0Evs(ps)) + ')');
        const fl2 = T.of('flee', fl1.k + 1), b3 = brk(fl2);
        if (b3.length !== 1 || b3[0].how !== 'flight') f.push('the flight as it fought not begun (' + dq0Evs(fl2) + ')');
        else {
          dq0Need(f, fl2, 'E_TASK', { te: 'TE_ENGAGED' }, 'the flight not stopped, engaged still');
          if (!dq0Ev(fl2, 'E_TASK', { te: 'TE_FIGHT' }).some(e => e.t >= b3[0].t)) f.push('not fought on, engaged still after the flight');
        }
        const story = dq0Story(out), at2 = story.indexOf('You break away from the ghoul.'), m3 = at2 < 0 ? null : /You (turn to|fight back against) the ghoul[.]/.exec(story.slice(at2)),
          at3 = m3 ? at2 + m3.index : -1;   // (The fight told again after the stop: turned to, or fought back)
        if (at2 < 0 || at3 < 0 || (story.slice(at2, at3).match(/stop walking[.]/gi) || []).length !== 1) f.push('the flight\'s stop not told once: ' + JSON.stringify(story.slice(at2, at3 < 0 ? at2 + 200 : at3)));
        const fl3 = T.of('flee', fl2.k + 1), b4 = brk(fl3);
        if (b4.length !== 1) f.push('the flight at its prompt not begun (' + dq0Evs(fl3) + ')');
        if (dq0Ev(fl3, 'E_NOWAY', {}).length) f.push('a flight cornered run on into the wall (' + dq0Evs(fl3) + ')');
        // (Every round fought charged: each blow (a rise, O_RISE, a blow's turn), each struggle up still down, the pass
        // begun engaged: the ghoul beside it as it was typed, on its feet, fighting it)
        const rounds = t => t.replies.filter(r => /^slot/.test(r.req) && r.kind === K.RS_EXERT && r.a === K.GA_WORK && r.w1 === 100).length;
        const blows = T.reduce((n, t) => n + mine(t).length, 0), ups = T.reduce((n, t) => n + dq0Ev(t, 'E_BEAT', { beat: 'WB_NOT_UP', b: 0 }).length, 0);
        const g = ps.at && ps.at.actors.find(a => a.name === 'ghoul'), engaged = !!g && g.sn === ps.at.pc.sn && g.pos !== K.POS_PRONE && g.doing === 'DO_FIGHT' &&
          Math.max(Math.abs(g.x - ps.at.pc.x), Math.abs(g.y - ps.at.pc.y)) === 1;
        if (!engaged) f.push('the pass not begun engaged: ' + JSON.stringify(g || null));
        else if (rounds(ps) !== 1) f.push('the pass, engaged, not charged as a round of strenuous work: ' + rounds(ps) + ' charged in it');
        const work = T.reduce((n, t) => n + rounds(t), 0), fought = blows + ups + (engaged ? 1 : 0);
        if (!blows || work !== fought) f.push('a fight\'s rounds not charged as strenuous work: ' + work + ' charged, ' + fought + ' rounds fought (' + blows + ' blows, ' + ups + ' struggles up, the pass)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0tiredorc', near: ['fight', 'denizens'], what: "Dingus Quest Zero's black orc tiring, and not breaking off (2026-10-31's every actor's tiring: DENIZENS.md's their bodies; dq0bodies has the ghoul and the rat): dq0succession's tribe at seed 2, its black orc set beside the character, fighting it, its body a breath short of a level more tired each round as each wait's typed (dq0BodyPoke's); folk on two legs: a level more tired (the referee's bank), its blows struck on, and no courage tested for it: it never flees but by a wound",
      init: 't_rc', cycles: 1500e6,
      machine: { input: (() => {
        return dq0Typed(dq0Game({ lines: ['echo ml swd 20 >> /ram/c.hm', 'echo ml dge 100 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'mkdir /ram/b',
          "echo 'faction | Tribe | arak garking | 0-1 | 30 | khanu | 0 | w | -' > /ram/b/theme.txt",
          "dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/b -k 'gambeson,mail hauberk,mail cowl,great helm'", 'wear the gambeson', 'wear the mail hauberk', 'wear the mail cowl', 'wear the great helm', ...Array(6).fill('wait'), 'quit', 'echo $status'] }));
      })() },
      start(m) {
        dq0BodyPoke(m, [1, 2, 3, 4, 5, 6].map(n => ({ g: 1, line: 'wait', n, name: 'black orc', lv: n === 1 ? 2 : undefined, ex: 5990, at: 0 })));
        const off = (h, L, K, at, named) => {   // (The rest of the tribe let be: the black orc's the scene)
          for (let k = 0; k < L.nactors; k++) if (named(k) && named(k) !== 'black orc') { h[at(k) + L.aFaction] = 7; h[at(k) + L.aAttitude] = K.AT_FRIENDLY; h[at(k) + L.aDoing] = K.DO_ROAM; const now = h[L.now] | h[L.now + 1] << 8 | h[L.now + 2] << 16, due = now + 3000; h[at(k) + L.aReady] = due & 255; h[at(k) + L.aReady + 1] = due >> 8 & 255; h[at(k) + L.aReady + 2] = due >> 16 & 255; h[at(k) + L.aReady + 3] = 0; }
        };
        dq0Scene(m, [{ g: 1, line: 'wait', n: 1, name: 'black orc', beside: 'face', engage: true }, ...[1, 2, 3, 4, 5, 6].map(n => ({ g: 1, line: 'wait', n, call: off }))]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], bodies = (m.bodies || []).filter(b => b.name === 'black orc'), L = m.dq0.layout().K;
        if (!bodies.some(b => b.poked)) return ['the black orc not among the actors as the wait was typed'];
        const first = bodies.find(b => b.poked), last = bodies[bodies.length - 1];
        if (!(last.lv > first.lv)) f.push('the black orc not a level more tired: ' + JSON.stringify(bodies));
        const blows = T.flatMap(t => t.dens).filter(e => e.kind === 'E_FIGHT');
        const fled = blows.filter(e => (e.ff & 0x10) && !/HIT/.test(e.out));   // (A wound's courage its own: not tiring's)
        if (blows.length < 4) f.push('its fight too short to tell: ' + blows.length + ' blows');
        if (fled.length) f.push('broken off, tired, unhurt by the blow: ' + JSON.stringify(fled));
        this.notes = [bodies.map(b => b.lv + '/' + b.ex).join(' '), blows.length + ' blows, ' + fled.length + ' fleeing'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0bodies', near: ['fight', 'denizens'], what: "Dingus Quest Zero's every actor tiring in a fight (2026-10-31, the brief's every actor's body: HOUSE.md's fights, the denizens' bodies; the character's rules, the book's second variant of combat fatigue, a level each five minutes of active combat): seed 7's ghoul (dq0disengage's scene: the character weak with its sword in a gambeson and a mail hauberk), the fight as the character waits; the ghoul's body in the referee's bank (dq0harn's fb, tire.c's struct bbody by its sheet's number) charged for its rounds (its exertion, 2 units a tenth, a round 100 tenths at most since it was last reckoned), kept between them.  And a tired beast breaking off: a second game the same, the ghoul's body set a breath short of its thirteenth level (dq0BodyPoke's: twelve levels, 5990 units, never reckoned) as the wait's typed; its next round a level more, its courage tested with its fatigue in it, and it breaks off (an E_FIGHT with FF_FLEES, the ghoul fleeing)",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 20 >> /ram/c.hm',
        "dq0 -p t -s 7 -c /ram/c.hm -k 'sword,gambeson,mail hauberk' -T", 'turn around', 'turn around', 'wear the gambeson', 'wear the mail hauberk', 'drop 2 rations',
        'd', 'wait', 'quit',
        "dq0 -p t -s 7 -c /ram/c.hm -k 'sword,gambeson,mail hauberk' -T", 'turn around', 'turn around', 'wear the gambeson', 'wear the mail hauberk', 'drop 2 rations',
        'd', 'wait', 'look', 'quit',
        "dq0 -p t -s 1 -c /ram/c.hm -k sword -T", 'wait', 'wait', 'wait', 'wait', 'wait', 'quit', 'echo $status'] })) },
      start(m) {
        dq0BodyPoke(m, [{ g: 2, line: 'wait', n: 1, name: 'ghoul', lv: 12, ex: 5990, at: 0 },
          ...[1, 2, 3, 4, 5].map(n => ({ g: 3, line: 'wait', n, name: 'giant rat', lv: 12, ex: 5990, at: 0 }))]);
        dq0Scene(m, [{ g: 3, line: 'wait', n: 1, name: 'giant rat', beside: 'face', engage: true, set: { attitude: 0 } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], bodies = m.bodies || [];
        const gh = g => bodies.filter(b => b.g === g && b.name === 'ghoul');
        // (Game 1: the ghoul's rounds charged: its body reckoned (at), its exertion the more after the fight)
        const b1 = gh(1), last = b1[b1.length - 1];
        if (!b1.length) f.push('game 1: no ghoul among the actors');
        else if (!last.at || last.lv * 6000 + last.ex < 200) f.push('game 1: the ghoul\'s rounds not charged: ' + JSON.stringify(b1));
        const fights1 = T.filter(t => t.game === 0).reduce((n, t) => n + t.dens.filter(e => e.kind === 'E_FIGHT' && e.out !== 'FO_BLED').length, 0);
        if (last && last.lv * 6000 + last.ex > 200 * fights1) f.push('game 1: the ghoul charged more than its rounds (' + fights1 + ' blows): ' + JSON.stringify(last));
        // (Game 2: set a breath short of a level more, its next round tires it: a ghoul's no beast, its sheet two-legged, it fights on, 2026-11-03)
        const p = bodies.find(b => b.g === 2 && b.poked);
        if (!p) f.push('game 2: the ghoul\'s body not set (not an actor as the wait was typed)');
        const flee = T.filter(t => t.game === 1).flatMap(t => t.dens).filter(e => e.kind === 'E_FIGHT' && (e.ff & 0x10));
        if (flee.length) f.push('game 2: the tired ghoul broke off, as a beast does: ' + T.filter(t => t.game === 1).map(dq0Evs).join(' | ').slice(0, 1500));
        // (Game 3: a beast, a giant rat, a breath short each round: short of steady, it breaks off)
        const rflee = T.filter(t => t.game === 2).flatMap(t => t.dens).filter(e => e.kind === 'E_FIGHT' && (e.ff & 0x10));
        if (!rflee.length) f.push('game 3: the tired rat didn\'t break off: ' + T.filter(t => t.game === 2).map(dq0Evs).join(' | ').slice(0, 1500));
        const b2 = gh(2), l2 = b2[b2.length - 1];
        if (p && (!l2 || l2.lv < 13)) f.push('game 2: the ghoul not a level more tired: ' + JSON.stringify(b2));
        this.notes = [JSON.stringify(bodies)];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0strike', what: 'Dingus Quest Zero\'s blows by what strikes (the user\'s playtest, 2026-10-22: "I cannot `hit the door with bar` if I have a `bar` in hand.  It still says I am battering it with my bare hands"; "A bar should just be treated as a club or stick, no?"; a soft thing tried once, "it seems ineffectual"; nothing named, what\'s in hand; HOUSE.md\'s fights, what strikes), by the world\'s events, the hub\'s pc_strike and the words: at seed 14 (dq0 -k bar: the torch and the bar in hand) the door hit with the bar, battered with it (TE_BEGUN\'s what the door and SK_WITH, pc_strike the bar\'s kind: "You set to battering the door with the bar."), no part of the character struck (no stop short, no hurt); hit with nothing named, the bar again (of the two in hand the referee\'s, RS_WIELD: the club over the torch\'s stick); the torch alone in hand, the door hit, not battered with the torch (the user\'s, 2026-11-17: "nothing is battered with a light"): its fist (SK_PUNCH, its hand as if empty: never a better thing carried, unasked); a rope named (soft): one attempt, no battering ("You attempt to batter the door with the rope, but it seems ineffectual."), and a kick said outright a kick still; at seed 5 (dq0 -T) a giant rat attacked with the bar ("You strike at the giant rat with your bar"), and one swatted with a cloak, no blow struck ("You swat at the giant rat with the cloak, but it seems ineffectual.")',
      init: 't_rc', cycles: 2800e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 14 -c /ram/c.hm -k bar', 'hit the door with the bar', 'quit', 'dq0 -p t -s 14 -c /ram/c.hm -k bar', 'hit the door', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm', 'hit the door', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm -k rope', 'batter the door with the rope', 'kick the door', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -k bar -T', 'd', 'wait', 'go right', 'attack the rat with the bar', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -k cloak -T', 'd', 'wait', 'go right', 'attack the rat with the cloak', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), K = m.dq0.layout().K;
        const begun = t => dq0Ev(t, 'E_TASK', { te: 'TE_BEGUN', task: 'LT_BATTER' })[0];
        const g1 = T.of('dq0 -p t -s 14 -c /ram/c.hm -k bar'), h1 = T.after(g1, 'hit the door with the bar');
        const h2 = T.after(T.after(h1, 'dq0 -p t -s 14 -c /ram/c.hm -k bar'), 'hit the door');
        for (const [t, w] of [[h1, 'named'], [h2, 'nothing named, the bar and the torch in hand']]) {
          const b = begun(t);
          if (!b || b.what !== (1 | 0x40)) f.push('the door hit with the bar (' + w + '): not battered with a thing (' + dq0Evs(t) + ')');
          if (!t.after || t.after.pc.strike !== K.KI_BAR + 3) f.push('the door hit with the bar (' + w + '): pc_strike ' + (t.after && t.after.pc.strike));
          dq0Never(f, t, 'E_TASK', { te: 'TE_ASK_SHORT' }, 'the bar\'s blows stopped short (' + w + ': no part of it strikes)');
          dq0Never(f, t, 'E_HARM', { hc: 'HC_STRUCK' }, 'the character hurt by the bar\'s blows (' + w + ')');
          if (!dq0Ev(t, 'E_PORTAL', { po: p => ['PO_BROKE_DOWN', 'PO_GIVING', 'PO_HOLDING', 'PO_NO_MARK'].includes(p) }).length) f.push('the bar\'s battering not ended (' + w + ': ' + dq0Evs(t) + ')');
        }
        const g2 = T.of('dq0 -p t -s 14 -c /ram/c.hm'), h3 = T.after(g2, 'hit the door');
        if (!begun(h3) || begun(h3).what !== (1 | 1 << 4) || !h3.after || h3.after.pc.strike !== 'SK_PUNCH') f.push('the torch in hand, the door not battered with its fist (' + dq0Evs(h3) + ')');
        const r = T.of('batter the door with the rope'), k = T.after(r, 'kick the door');
        dq0Need(f, r, 'E_PORTAL', { po: 'PO_SOFT' }, 'the rope not one attempt, ineffectual');
        if (begun(r)) f.push('the rope\'s attempt a battering (' + dq0Evs(r) + ')');
        if (!begun(k) || begun(k).what !== (1 | 2 << 4)) f.push('a kick said outright not a kick (' + dq0Evs(k) + ')');
        const a = T.of('attack the rat with the bar');
        if (!dq0Ev(a, 'E_FIGHT', { who: 0 }).length) f.push('the rat not struck at with the bar (' + dq0Evs(a) + ')');
        const c = T.of('attack the rat with the cloak'), cq = T.and(c);
        dq0Need(f, cq, 'E_BEAT', { beat: 'WB_SWAT' }, 'the rat not swatted at with the cloak');
        if (dq0Ev(c, 'E_FIGHT', { who: 0 }).length) f.push('a blow struck with the cloak (' + dq0Evs(c) + ')');
        inTurn(s, ['> hit the door with the bar', 'You set to battering the door with the bar.', '> hit the door', 'You set to battering the door with the bar.',
          '> hit the door', 'You set to punching the door.', '> batter the door with the rope',
          'You attempt to batter the door with the rope, but it seems ineffectual.', '> kick the door', 'You set to kicking the door.',
          '> attack the rat with the bar', 'You strike at the giant rat with your bar', '> attack the rat with the cloak',
          'You swat at the giant rat with the cloak, but it seems ineffectual.'], f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0nolight', asks: true, what: 'Dingus Quest Zero\'s door never battered with a light (the user\'s, 2026-11-17: DECISIONS.md\'s "The critique\'s questions: the user\'s answers", freeing a hand: "nothing is battered with a light"; the play test\'s "break the door" with no tool named: "You set to battering the door with the lantern."), by the world\'s events, the hub\'s pc_strike and the words: a light in hand is as if its hand were empty (the UI\'s wield: the things\' IO_HANDS telling which are lights), so the rules of a blow with nothing named go on as ever: at seed 14 (dq0 -k pick: a lit torch in hand, the pick in the pack) the door hit: asked ("... would you rather use your pick?"), no: its fist (SK_PUNCH, "You set to punching the door."), not the torch; at seed 14 (dq0 -k lantern: an unlit lantern in hand, the torch put down) the door broken: its fist again; at seed 14 (dq0 -k bar: the torch and the bar in hand) the door hit: the bar (dq0strike\'s)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 14 -c /ram/c.hm -k pick', 'hit the door', 'no', 'stop', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm -k lantern', 'drop the torch', 'break the door', 'stop', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm -k bar', 'hit the door', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), K = m.dq0.layout().K;
        const begun = t => dq0Ev(t, 'E_TASK', { te: 'TE_BEGUN', task: 'LT_BATTER' })[0];
        const n = T.of('no'), b = T.of('break the door'), h = T.after(b, 'hit the door');
        for (const [t, w] of [[n, 'asked, no'], [b, 'a lantern in hand']]) {
          if (!begun(t) || begun(t).what !== (1 | 1 << 4) || !t.after || t.after.pc.strike !== 'SK_PUNCH') f.push(w + ': not battered with its fist (' + dq0Evs(t) + ', pc_strike ' + (t.after && t.after.pc.strike) + ')');
        }
        if (!begun(h) || begun(h).what !== (1 | 0x40) || !h.after || h.after.pc.strike !== K.KI_BAR + 3) f.push('the torch and the bar: not the bar (' + dq0Evs(h) + ')');
        inTurn(s, ['> hit the door', 'You can try battering it with your hands, but would you rather use your pick?', '> no', 'You set to punching the door.',
          '> break the door', 'You set to punching the door.', '> hit the door', 'You set to battering the door with the bar.'], f, 'the words: ');
        if (/battering the door with the (?:torch|lantern)/.test(s)) f.push('a door battered with a light');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0rather', asks: true, what: 'Dingus Quest Zero\'s blow with nothing named and its hands empty (the user\'s, 2026-10-22: "I don\'t think it should auto-fallback to something better unless you have nothing at hand, and then it should ask for confirmation.  If your hands are empty, but you have an axe at your belt, then something like `You can try battering it with your hands, but would you rather use your axe?`"; HOUSE.md\'s fights, what strikes), by the world\'s events, the hub\'s pc_strike and the words: at seed 14 (dq0 -k pick: the pick in the pack) the torch put down, the door hit: asked ("... would you rather use your pick?", nothing struck), yes taking the pick in hand and battering with it; again, no battering bare-handed ("You set to battering the door."), stopped; asked again, and a look typed instead letting the blow go (no battering, the look done); at seed 5 (dq0 -T, dq0 -k sword: Sword 80) its sword stowed, the rats seen down the ladder, its torch put down, a rat attacked: asked ("You can fight it with your fists, but would you rather draw your sword?"), a look letting it go; and before it, its hands empty, a ghoul attacked with none there to fight: not asked; each answer\'s first words its own ("You reach for your backpack.", "You head for the door."), and the rat\'s question its first words, no walk over to it told before it (the user\'s first words at once, 2026-11-17)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['dq0 -p t -s 14 -c /ram/c.hm -k pick', 'drop the torch', 'hit the door', 'yes', 'quit',
        'dq0 -p t -s 14 -c /ram/c.hm -k pick', 'drop the torch', 'hit the door', 'no', 'stop', 'hit the door', 'look', 'quit',
        'cp /ram/c.hm /ram/s.hm', 'echo ml swd 80 >> /ram/s.hm', 'dq0 -p t -s 5 -c /ram/s.hm -k sword -T', 'stow the sword', 'drop the torch', 'attack the ghoul',
        'take the torch', 'd', 'drop the torch', 'attack the rat', 'look', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), K = m.dq0.layout().K;
        const begun = t => dq0Ev(t, 'E_TASK', { te: 'TE_BEGUN', task: 'LT_BATTER' })[0];
        const g1 = T.of('dq0 -p t -s 14 -c /ram/c.hm -k pick'), h1 = T.after(g1, 'hit the door'), y = T.after(h1, 'yes');
        if (begun(h1)) f.push('battered before it was asked (' + dq0Evs(h1) + ')');
        if (!begun(y) || begun(y).what !== (1 | 0x40) || !y.after || y.after.pc.strike !== K.KI_PICK + 3) f.push('yes: not battered with the pick (' + dq0Evs(y) + ')');
        const g2 = T.after(y, 'dq0 -p t -s 14 -c /ram/c.hm -k pick'), h2 = T.after(g2, 'hit the door'), n = T.after(h2, 'no');
        const h3 = T.after(n, 'hit the door'), l = T.after(h3, 'look');
        if (begun(h2)) f.push('battered before it was asked, again (' + dq0Evs(h2) + ')');
        if (!begun(n) || begun(n).what !== 1) f.push('no: not battered bare-handed (' + dq0Evs(n) + ')');
        if (begun(h3) || begun(l)) f.push('the blow asked about not let go for a look (' + dq0Evs(h3) + '; ' + dq0Evs(l) + ')');
        const a = T.of('attack the rat'), al = T.after(a, 'look');
        if (dq0Ev(a, 'E_FIGHT', { who: 0 }).length || dq0Ev(al, 'E_FIGHT', { who: 0 }).length) f.push('the rat struck at, asked about and let go (' + dq0Evs(a) + ')');
        inTurn(s, ['> hit the door', 'You can try battering it with your hands, but would you rather use your pick?', '> yes', 'You reach for your backpack.',
          'You take the pick in both hands.',
          'You set to battering the door with the pick.', '> hit the door', 'You can try battering it with your hands, but would you rather use your pick?',
          '> no', 'You head for the door.', 'You set to battering the door.', '> hit the door', 'would you rather use your pick?', '> look', '> attack the ghoul', '> take the torch',
          '> attack the rat', 'You can fight it with your fists, but would you rather draw your sword?', '> look'], f, 'the words: ');
        const ag = s.slice(s.indexOf('> attack the ghoul'), s.indexOf('> take the torch'));
        if (/would you rather/.test(ag)) f.push('asked with nothing to fight: ' + ag);
        // (Its first words the question, its hands empty: no walk over to the rat told before it, 2026-11-17; the
        // door's answered, its own first words after it, as ever)
        const ar = s.slice(s.indexOf('> attack the rat'), s.indexOf('would you rather draw'));
        if (/You go over to|You close with/.test(ar)) f.push('the rat closed with before it was asked: ' + ar);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0morale', near: ['fight', 'denizens'], what: "Dingus Quest Zero's morale (the user's, 2026-10-29: every intelligent creature sways the morale field about it, by its rank and its influence skills, for its friends and against its foes; bonds upward to its leaders regardless of distance, growing with time served and contact; a leader's loss, known, a morale check at once with its influence removed; HOUSE.md's morale, for review), by the denizens' morale tests (RS_MORALE in their slot: steps, why, the field, the bonds and the leaders reckoned) and a strong character (its attributes 99) in mail, among a hand-written theme's small orcs (a shy, wary people) at seed 2, their home by the way down where the character starts (popgen's, its leader with its band), another band of them by it, a third on the stratum below.  Their king their leader: struck down, senseless, then dead, every small orc that heard it fall tests at once, its leader's loss (its band's head for its own band, the faction's for the other, wherever it is), its bond to it taken away, the band beside it the worse (the book's bad situation, a friend fallen about it); after its death those that heard it are led by its successor (since 2026-10-30: the most influential of those below it, dq0succession's), while the band below, that didn't hear it, keeps its bond to the dead king, its fall unfelt.  The band hostile, fighting beside its king, a fellow's fall weighed with its king's presence and its bond (the field and the bonds above nothing).  A black orc their band's head (a captain), kings above it in the tribe: its fall the home band's leader's loss, its bond to the kings holding; an old band (long served, often with its leaders) the steadier at its fall than a new one, by those bonds.  An ally of the character's heartened by it; a fearsome foe beside a band (a mighty one of a faction hostile to it) weighing on it.  (The test's own setting up: pokes of the actors' hub fields at a line's Enter, dq0Poke's; since the denizens' water, 2026-10-31, the band below's small orc put beside the character as it's attacked, dq0Poke's beside: the band no longer wades where it did)",
      init: 't_rc', cycles: 4300e6,
      machine: { input: (() => {
        const kit = "-k 'sword,gambeson,mail hauberk,mail cowl,great helm'", dress = ['wear the gambeson', 'wear the mail hauberk', 'wear the mail cowl', 'wear the great helm', 'hold the sword'];
        const game = d => ['dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/' + d + ' ' + kit, ...dress], king = "'faction | Tribe | arak | 0-1 | 20 | garking | 0 | w | -'";
        return dq0Typed(dq0Game({ lines: ['echo ml swd 100 >> /ram/c.hm', 'echo ml ini 100 >> /ram/c.hm',
          'echo ml dge 100 >> /ram/c.hm', 'echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm',
          ...['h', 'p', 'b', 'c', 'd', 'e'].flatMap(d => ['mkdir /ram/' + d, 'echo ' + king + ' > /ram/' + d + '/theme.txt']),
          ...['b', 'c'].map(d => "echo 'faction | Tribe | arak garking | 0-1 | 30 | khanu | 0 | w | -' > /ram/" + d + '/theme.txt'),
          "echo 'faction | Tribe | arak | 0-1 | 20 | garking | 0 | w | Beasts h' > /ram/e/theme.txt", "echo 'faction | Beasts | bear | 1-1 | 1 | - | 1 | w | -' >> /ram/e/theme.txt",
          ...game('h'), 'wait', 'wait', 'attack the orc king', 'wait', 'attack the orc king', 'wait', 'd', 'wait', 'attack the small orc', 'wait', 'wait', 'quit',   // (Its
                                                // first "attack the small orc" gone, dice2: it came to nothing ("You can't get near enough") as the
                                                // minds' dice fell, now it fights the band and the king falls in it)
          ...game('p'), 'wait', 'wait', 'quit', 'rm -r /ram/h',   // (The RAM disk's room: its six folders, each with its dungeon's)
          ...['b', 'c'].flatMap(d => [...game(d), 'attack the black orc', 'wait', 'attack the black orc', 'quit']),
          ...['d', 'e'].flatMap(d => [...game(d), 'attack the orc king', 'quit']), 'echo $status'] }));
      })() },
      start(m) {
        const band = (g, set) => [0, 1].map(k => ({ g, line: 'attack the black orc', n: 1, k, set }));
        dq0Scene(m, [
          ...[0, 1].map(k => ({ g: 2, line: 'hold the sword', n: 1, k, set: { attitude: 0 } })),       // (Game 2: the band hostile, its king standing by)
          ...band(3, { serve: 255, contact: 255 }), ...band(4, { serve: 0, contact: 0 }),             // (Games 3, 4: an old band, a new one)
          { g: 5, line: 'attack the orc king', n: 1, k: 1, set: { attitude: 5 } },                    // (Game 5: one the character's ally)
          { g: 6, line: 'attack the orc king', n: 1, k: 1, set: { faction: 2, power: 250 } },         // (Game 6: one a fearsome foe)
          { g: 1, line: 'attack the small orc', n: 1, name: 'small orc', beside: 'face' }]);          // (Game 1: one of the band below beside
                                                                                                      //   the character as it's attacked, since
                                                                                                      //   their water, 2026-10-31)
      },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), C = require('../../../sim/lib/dq0meaning.js').constants().names, f = [], MR_FELL = 1, MR_LEADER = 2, NONE = 0xFF;
        const tests = g => { const seen = new Set(); return T.filter(t => t.game === g).flatMap(t => dq0Rep(t, 'dslot', { kind: C.RS_MORALE }).map(r => Object.assign({ cmd: t.line, at: t.k }, r)))
          .filter(r => { const k = JSON.stringify(r); return !seen.has(k) && seen.add(k); }); };
        const games = [...new Set(T.map(t => t.game).filter(g => g >= 0))].sort((a, b) => a - b);
        if (games.length !== 6) return ['not six games: ' + games.length];
        const [gk, gp, gb, gc, gd, ge] = games, K = tests(gk), say = r => JSON.stringify({ cmd: r.cmd, actor: r.actor, a: r.a, b: r.b, c: r.c, l1: r.w2 & NONE, l2: r.w2 >> 8, steps: r.steps });
        const l1 = r => r.w2 & NONE, l2 = r => r.w2 >> 8;
        // (The king's fall: every small orc that heard it, its leader's loss, the bond taken away; the band beside it the worse)
        const first = K.filter(r => r.a === MR_LEADER && r.cmd === 'attack the orc king'), fa = first.filter(r => r.w1 === first[0].w1);
        const own = fa.filter(r => l1(r) === r.w1), other = fa.filter(r => l1(r) === NONE && l2(r) === r.w1);
        if (new Set(fa.map(r => r.actor)).size < 4 || !own.length || !other.length) f.push('not every small orc tested at the king\'s fall, the king its band\'s head and the other band\'s faction head: ' + fa.map(say).join(' '));
        if (K.some(r => r.a === MR_LEADER && r.c !== 0)) f.push('a bond to the fallen king not taken away: ' + K.filter(r => r.a === MR_LEADER).map(say).join(' '));
        if (own.length && other.length && Math.max(...own.map(r => r.steps)) >= Math.min(...other.map(r => r.steps))) f.push('the band beside the fallen king not the worse: ' + fa.map(say).join(' '));
        // (After its death: those that heard it led by no one; the band below keeps its bond, the fall unfelt)
        const d = T.find(t => t.game === gk && t.line === 'd'), after = K.filter(r => r.a === MR_FELL && r.w1 !== (fa[0] || {}).w1);
        const kingRec = (fa[0] || {}).w1, lastK = Math.max(...K.filter(r => r.a === MR_LEADER).map(r => r.at)), up = after.filter(r => d && r.at >= lastK && r.at < d.k), below = after.filter(r => d && r.at > d.k);
        if (up.some(r => l1(r) === kingRec || l2(r) === kingRec) || new Set(up.filter(r => l1(r) === NONE && l2(r) === NONE).map(r => r.actor)).size > 1) f.push('a small orc that heard the king die still led by it, or by no successor (but the successor itself): ' + up.map(say).join(' '));
        if (!below.length || below.some(r => l2(r) === NONE || r.c < 1)) f.push('the band below not keeping its bond to the dead king (unheard): ' + below.map(say).join(' '));
        // (The king's presence: a fellow's fall weighed with the king standing beside its band, the field and the bond above nothing)
        const P = tests(gp), kp = P.find(r => r.a === MR_LEADER), pf = P.filter(r => r.a === MR_FELL && l1(r) !== NONE && (!kp || r.at <= kp.at));
        if (!pf.length || pf.some(r => r.b <= 0 || r.c <= 0)) f.push('the king\'s band not steadied by its presence: ' + P.map(say).join(' '));
        // (The black orc's fall, its bond to the kings holding; the old band the steadier, the new the less)
        const at = (g, k, why) => tests(g).find(r => r.a === why && r.actor === k + 1), ob = [0, 1].map(k => at(gb, k, MR_LEADER)), nb = [0, 1].map(k => at(gc, k, MR_LEADER));
        if (ob.some(r => !r || l2(r) === NONE || r.c < 1)) f.push('the black orc\'s fall not its band\'s leader\'s loss, its bond to the kings above holding: ' + tests(gb).map(say).join(' '));
        else if (nb.some(r => !r) || ob.some((r, i) => r.steps <= nb[i].steps || r.c <= nb[i].c)) f.push('the old band not the steadier than the new: ' + [...ob, ...nb].filter(Boolean).map(say).join(' '));
        // (An ally heartened; a fearsome foe weighing)
        const ad = at(gd, 1, MR_LEADER), aa = fa.find(r => r.actor === 2);
        const sb = v => v << 24 >> 24;      // (The field's points, signed: the character's might counts against it now, 2026-11-03)
        if (!ad || !aa || sb(ad.b) <= sb(aa.b)) f.push('the ally not heartened by the character: ' + [ad, aa].filter(Boolean).map(say).join(' '));
        const fe = tests(ge).find(r => r.a === MR_LEADER && r.actor !== 2), ff = fe && fa.find(r => r.actor === fe.actor) || fa[0];   // (Whichever of the band's still there: since their test on facing it, one may have fled, 2026-11-03)
        const near = tests(ge).filter(r => r.a === MR_LEADER && r.actor !== 2 && fa.some(q => q.actor === r.actor && sb(r.b) < sb(q.b) - 3));
        if (!near.length) f.push('the fearsome foe beside the band not weighing on it: ' + [fe, ff].filter(Boolean).map(say).join(' '));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0succession', near: ['fight', 'denizens'], what: "Dingus Quest Zero's succession (the user's, 2026-10-30: \"When a leader dies, the next one below him in the heirarchy (strongest/most influential of the those directly below them) should take over\"; the chain filled all the way down, the coordinator's; HOUSE.md's morale, for review), dq0morale's small orcs at seed 2 and a strong character in mail, each scene set by pokes of the actors' hub fields and the denizens' records at a line's Enter (dq0Poke's, dq0Rec's), its outcome the records' and the hub's, not the dice's: a black orc its band's head, two of its band kept out of the fight (another faction's in the hub for the while: no test of their courage, no fleeing), one the more influential (its skill), the other the mightier: the black orc slain, the more influential at its head (its record's rank a captain's, AM_LEADS), rallying them, told as the character sees it (\"The small orc rallies its fellows.\"); the same in the dark, the torch doused, told as heard (\"You hear a rallying shout ...\"); and a king its tribe's leader, a captain below it (its record's rank) heading a band of its own: the king slain, the captain king after and its band's best its head (the chain filled), the captain's own band's bond to it kept, the king's old band's to it a new leader's",
      init: 't_rc', cycles: 3000e6,
      machine: { input: (() => {
        const kit = "-k 'sword,gambeson,mail hauberk,mail cowl,great helm'", dress = ['wear the gambeson', 'wear the mail hauberk', 'wear the mail cowl', 'wear the great helm', 'hold the sword'];
        const game = d => ['dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/' + d + ' ' + kit, ...dress], head = "'faction | Tribe | arak garking | 0-1 | 30 | khanu | 0 | w | -'";
        return dq0Typed(dq0Game({ lines: ['echo ml swd 100 >> /ram/c.hm', 'echo ml ini 100 >> /ram/c.hm',
          'echo ml dge 100 >> /ram/c.hm', 'echo att str 99 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm',
          ...['b', 'd', 'k'].map(d => 'mkdir /ram/' + d), 'echo ' + head + ' > /ram/b/theme.txt', 'echo ' + head + ' > /ram/d/theme.txt',
          "echo 'faction | Tribe | arak | 0-1 | 20 | garking | 0 | w | -' > /ram/k/theme.txt",
          ...game('b'), 'attack the black orc', ...Array(4).fill('wait'), 'quit',
          ...game('d'), 'douse the torch', 'attack the black orc', ...Array(4).fill('wait'), 'quit',
          ...game('k'), 'attack the orc king', 'wait', 'attack the orc king', 'wait', 'quit', 'echo $status'] }));
      })() },
      start(m) {
        const apart = g => [{ g, line: 'attack the black orc', n: 1, k: 0, set: { faction: 7, lead: 255, power: 0 } },   // (The more influential,
          { g, line: 'attack the black orc', n: 1, k: 1, set: { faction: 7, lead: 0, power: 250 } }];              //   the mightier: out of the fight)
        // (The black orc senseless beside it as it's attacked, finished: its fall the dice's no longer, 2026-11-03)
        const fall = g => ({ g, line: 'attack the black orc', n: 1, name: 'black orc', beside: 'face', senseless: 600 });
        dq0Scene(m, [...apart(1), fall(1), ...apart(2), fall(2),
          { g: 3, line: 'attack the orc king', n: 1, k: 3, set: { serve: 255, contact: 255 }, rec: { or: 0x20 } },   // (A captain, its band's head)
          { g: 3, line: 'attack the orc king', n: 1, k: 4, set: { serve: 255, contact: 255 } },                     // (Its band: long with it)
          ...[0, 1].map(k => ({ g: 3, line: 'attack the orc king', n: 1, k, set: { serve: 255, contact: 255 }, rec: { or: 0x10 } }))]);   // (The king's
                                                // band, long with it too; they know (D_LOST): so they know of its successor)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], DE_RALLY = 4, AM_LEADS = 4, H = m.hearts || [];
        const games = [...new Set(T.map(t => t.game).filter(g => g >= 0))].sort((a, b) => a - b);
        if (games.length !== 3) return ['not three games: ' + games.length];
        const last = g => H.filter(x => x.g === g).slice(-1)[0], st = dq0Prose(out).split('% dq0 -p t').slice(1);
        const rallies = gi => T.filter(t => t.game === games[gi]).flatMap(t => dq0Ev(t, 'E_DEN', { a: DE_RALLY }));
        for (const [gi, g, how] of [[0, 1, 'seen'], [1, 2, 'heard']]) {
          const e = last(g), b0 = e && e.acts.find(a => a.k === 0), b1 = e && e.acts.find(a => a.k === 1), r = rallies(gi);
          if (!b0 || !(b0.mood & AM_LEADS) || b0.rank !== 1 || !b0.prom || (b1 && ((b1.mood & AM_LEADS) || b1.rank))) f.push('game ' + g + ': the more influential not at the black orc\'s post over the mightier: ' + JSON.stringify(e && e.acts));
          if (r.length !== 1 || r[0].who !== 1 || !!r[0].seen !== (how === 'seen')) f.push('game ' + g + ': not one rally by it, ' + how + ': ' + JSON.stringify(r));
          if (!(how === 'seen' ? /The small orc rallies their fellows\./ : /You hear a rallying shout/).test(st[gi] || '')) f.push('game ' + g + ': the rally ' + how + ' not told');
        }
        const e3 = H.find(x => x.g === 3 && x.line === 'wait' && x.n === 2), act = k => e3 && e3.acts.find(a => a.k === k), c3 = act(3), c4 = act(4), c0 = act(0) || act(1);
        if (!c3 || c3.rank !== 2 || !c3.prom) f.push('the captain not king after the king: ' + JSON.stringify(e3 && e3.acts));
        if (!c4 || c4.rank !== 1 || !c4.prom) f.push('the captain\'s band not headed by its best after (the chain not filled): ' + JSON.stringify(e3 && e3.acts));
        if (c4 && c0 && !(c4.bonds >= 8 && c0.bonds <= 4)) f.push('the bonds to the new king not its own band\'s kept, the rest\'s a new leader\'s: ' + JSON.stringify([c4, c0]));
        if (!c0) f.push('none of the king\'s old band left to see');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0flee', what: 'Dingus Quest Zero\'s unarmed blows and morale (the user\'s, 2026-10-16: punching, kicking, head butts, the referee\'s gear.c, by Unarmed; RS_MORALE, HarnMaster\'s Initiative test, Combat 18), by the blows told and the denizens\' state (the referee\'s dice at this seed): at seed 5\'s giant rats (bare fists: the character\'s Unarmed 95, tough; the rats set hurt, dq0Poke\'s, 2026-11-04: the book\'s rat dodges at 90) a punch fought on (HOUSE.md\'s fighting on, the user\'s, 2026-10-20: every blow of it a punch, the hub\'s pc_strike), the rat and its kin turning on the character, a punch felling one; its kin that heard it fall shaken and, shy, fleeing (fighting no more, no foe left beside it), those out of hearing untested (since 2026-10-30, HOUSE.md\'s morale: a fall unfelt till it\'s known); the one down finished with a kick; the other gone out of reach, a head butt at it not struck (dq0unarmed\'s, in it; a kick, a head butt and punches typed one by one before fighting on)',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml una 95 >> /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm', 'echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -T', 'd',
        'wait', 'go right', 'punch the rat', 'kick the rat', 'head butt the rat', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      start(m) {        // (The book's rat dodges at 90, 2026-11-04: the two set hurt, a punch can land and their courage is shaken)
        dq0Scene(m, [0, 1, 2, 3].map(k => ({ g: 1, line: 'punch the rat', n: 1, k, hurt: 8 })));
      },
      check(m) {
        const T = dq0Turns(m), f = [], FF_D_DOWN = 4, FF_D_DEAD = 8, mine = t => dq0Ev(t, 'E_FIGHT', { who: 0 });
        const p = T.of('punch the rat'), k = T.of('kick the rat'), h = T.of('head butt the rat');
        if (h.k < 0) return ['the blows not typed'];
        if (mine(p).length < 2) f.push('the punch not fought on (' + dq0Evs(p) + ')');
        if (!p.after || p.after.pc.strike !== 'SK_PUNCH') f.push('the fight not struck with the fists: ' + (p.after && p.after.pc.strike));
        if (new Set(dq0Ev(p, 'E_FIGHT', { def: 0, who: w => w > 0 }).map(e => e.who)).size < 2) f.push('the rat and its kin not both striking at the character');
        const felled = mine(p).find(e => e.out === 'O_HIT' && (e.ff & FF_D_DOWN)), rat = felled && felled.def;
        if (!felled) f.push('no punch felled a rat (' + dq0Evs(p) + ')');
        // (Its kin that heard it fall tested and fleeing; those out of their hearing of a fight's din, 15 squares, not
        // tested: the fall unfelt till it's known, HOUSE.md's morale, 2026-10-30)
        const C = require('../../../sim/lib/dq0meaning.js').constants().names, tested = new Set(dq0Rep(p, 'dslot', { kind: C.RS_MORALE }).map(r => r.actor));
        const down = p.after && p.after.actors.find(a => a.k + 1 === rat), kin = p.after ? p.after.actors.filter(a => a.k + 1 !== rat) : [];
        const heard = kin.filter(a => tested.has(a.k + 1)), far = a => !down || Math.max(Math.abs(a.x - down.x), Math.abs(a.y - down.y)) > 15;
        if (!heard.length || heard.some(a => a.doing !== 'DO_FLEE')) f.push('its kin not fleeing: ' + kin.map(a => a.doing + (tested.has(a.k + 1) ? ' (tested)' : '')).join(', '));
        if (kin.some(a => !tested.has(a.k + 1) && !far(a))) f.push('a kin within its hearing not tested: ' + JSON.stringify(kin.map(a => [a.k, a.x, a.y])) + ' ' + JSON.stringify(down));
        dq0Need(f, p, 'E_TASK', { te: 'TE_NO_FOE' }, 'the fight not over, no foe left');
        if (!mine(k).some(e => e.def === rat && e.out === 'O_DISPATCH' && (e.ff & FF_D_DEAD))) f.push('the one down not finished (' + dq0Evs(k) + ')');
        if (!k.after || k.after.pc.strike !== 'SK_KICK') f.push('not finished with a kick: ' + (k.after && k.after.pc.strike));
        if (mine(h).length) f.push('a blow struck at the rat fled out of reach');
        if (h.after && h.after.sight && h.after.sight.dens.some(d => d.doing === 'DO_FIGHT')) f.push('a rat seen fighting still');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0engage', initiative: true, what: 'Dingus Quest Zero\'s engage initiative test and a grope while engaged (the user\'s, 2026-10-20; HOUSE.md\'s, as built: the book\'s optional rule, the referee\'s RS_CONTEST of P_INITIATIVE, and P_GROPE), on (initiative: true: the runner gives the other dq0 tests -I), by the referee\'s requests, the hub\'s events and the words (HarnMaster\'s dice at these seeds): at seed 7, in a gambeson and a mail hauberk, the walk toward the far side stopped by the ghoul\'s zone, both tested, the ghoul the quicker: its blow first ("Quicker, the ghoul"), the ghoul fought back (HOUSE.md\'s fighting on), and the walk again taking the fight over, engaged still, a square, no test; the ghoul closed with (attack, a walk), the ghoul the quicker, its blow first, then the character\'s; the sword dropped beside the ghoul as it fights (the commands typed ahead taking the fight over, one by one) and taken up, a grope: fumbled ("You fumble for the sword.", the sword where it lay), and held at last; then a quick character (Initiative 80, no armour) the ghoul comes up to, the character the quicker: its blow first ("Quicker, you strike at the ghoul with your sword"); at seed 21 a wary giant rat closed with, the rat the quicker, backing away (TE_AWAY: "Quicker, the giant rat backs away from you."); and at seed 7 again with dq0 -I, no test at all (since fighting on, the walk again and the gropes typed as the character fights) (since 2026-10-29: the ghoul made aware of the character as the walk past it, two squares off, is typed: dq0Aware, the test\'s own setting up, an unaware denizen engaging no one; the walk into its zone the engage initiative test\'s)',
      init: 't_rc', cycles: 3600e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 80 >> /ram/c.hm',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'go to the far side', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'attack the ghoul', 'quit',
        'cp /ram/c.hm /ram/e.hm', 'echo att dex 1 >> /ram/e.hm', 'dq0 -p t -s 7 -c /ram/e.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'wait'] })) +
        'drop the sword\rtake the sword\rtake the sword\rtake the sword\rhold the sword\r' + dq0Typed(['quit',
        'echo ml ini 80 >> /ram/c.hm', 'dq0 -p t -s 7 -c /ram/c.hm -k sword -T', 'd', 'wait', 'quit',
        'hmchar -q -s 5 -u feudal -c hunter -o /ram/d.hm', 'dq0 -p t -s 21 -c /ram/d.hm -T', 'attack the rat', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -k sword -T -I', 'd', 'wait', 'wait', 'quit', 'echo $status']) },
      start(m) {
        dq0Aware(m, 'go to the far side', 'ghoul');   // (Its ghoul aware of the character as the walk past it begins: slice 4.5's, an unaware one engages no one)
        dq0OnWords(m, [{ g: 1, line: 'go to the far side', n: 1, words: 'Quicker, the ghoul', text: 'go to the far side\r' }]);   // (The walk again
                                                //   typed as it's told, as it fights: 2026-11-02, not at a fixed gap of cycles)
        // (Rigged, dice2's keyed dice: the ghoul the quicker as it's met, the character's d100 a marginal failure, its a
        // success; on the walk its first blow, a marginal failure blocked, an opening, as before, and its courage held
        // (the keyed dice had the character's block wound it, or its courage fail, and the walk typed again met no foe)
        const quick = { kind: 'CONTEST', a: 'P_INITIATIVE', uses: 1, faces: [99, 1] };
        dq0Scene(m, [{ g: 1, line: 'go to the far side', n: 1, call: (h, L, K, at, named, sc) => {
          const k = [...Array(L.nactors).keys()].find(j => named(j) === 'ghoul');
          sc.rig([quick, { kind: 'ATTACK', actor: k, uses: 1, faces: [99, 1] }, { kind: 'MORALE', actor: k, grade: 'pass' }]);
        } },
          { g: 2, line: 'attack the ghoul', n: 1, rig: [quick] }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), C = require('../../../sim/lib/dq0meaning.js').constants().names, f = [];
        const contests = t => t.replies.filter(r => r.req === 'slot0' && r.kind === C.RS_CONTEST);
        const ans = dq0Answers(out), said = (t, w) => { const n = T.filter(u => u.k < t.k && u.line === t.line).length, x = ans.filter(y => y.cmd === t.line)[n]; if (!x || !x.text.includes(w)) f.push(t.line + ': not "' + w + '" (' + (x ? x.text.slice(0, 160) : 'no answer') + ')'); };
        const actor = (t, n) => t.at && t.at.actors.find(x => x.name === n);
        // (The walk into the ghoul's zone: tested, the ghoul the quicker, its blow first, fought back; the walk again,
        // typed as it fights, taking it over: begun engaged, a square, no test.  The two lines' turns as one: the
        // second typed before the first's done)
        const g = T.filter(t => t.line === 'go to the far side'), gh = g.length && actor(g[0], 'ghoul');
        if (g.length !== 2 || !gh) return [...f, 'the walks not typed, or no ghoul'];
        const gg = { k: g[1].k, line: g[1].line, events: [...g[0].events, ...g[1].events], dens: [...g[0].dens, ...g[1].dens], replies: [...g[0].replies, ...g[1].replies] };
        const te = gg.events.filter(e => e.kind === 'E_TASK'), at = (want, from = 0) => te.findIndex((e, i) => i >= from && Object.entries(want).every(([k, v]) => e[k] === v));
        const eng = at({ te: 'TE_ENGAGED', still: false }), fight = at({ te: 'TE_FIGHT', by: gh.k }, eng + 1), stop = at({ te: 'TE_STOPPED', task: 'LT_FIGHT' }, fight + 1);
        if (eng < 0) f.push('the walk not stopped by the ghoul\'s zone (' + dq0Evs(g[0]) + ')');
        else if (fight < 0) f.push('the ghoul not fought back (' + dq0Evs(g[1]) + ')');
        else if (stop < 0) f.push('the walk typed as it fought not taking it over (' + dq0Evs(g[1]) + ')');
        else if (at({ te: 'TE_ENGAGED', still: true }, stop + 1) < 0) f.push('the walk begun engaged not stopped after a square (' + dq0Evs(g[1]) + ')');
        const c0 = contests(gg);
        if (c0.length !== 1 || c0[0].actor !== 0 || c0[0].other !== gh.k + 1 || c0[0].a !== C.P_INITIATIVE || c0[0].result !== 2)
          f.push('the walk into its zone not one test, the ghoul the quicker (and none as the walk begun engaged went on): ' + JSON.stringify(c0));
        dq0Need(f, gg, 'E_FIGHT', { who: gh.k + 1, def: 0 }, 'the ghoul\'s blow first not struck');
        if (!ans.filter(y => y.cmd === 'go to the far side').some(y => y.text.includes('Quicker, the ghoul')))   // (Either
          f.push('go to the far side: not "Quicker, the ghoul"');   //   walk's answer: the second typed ahead, the words where the clock puts them)
        // (The close-in: the ghoul the quicker, its blow first, then the character's)
        const a = T.of('attack the ghoul'), ag = actor(a, 'ghoul'), ac = contests(a);
        if (!ag || ac.length !== 1 || ac[0].actor !== 0 || ac[0].other !== ag.k + 1 || ac[0].result !== 2)
          f.push('the close-in not one test, the ghoul the quicker: ' + JSON.stringify(ac));
        const blows = a.dens.filter(e => e.kind === 'E_FIGHT');
        if (blows.length < 2 || !ag || blows[0].who !== ag.k + 1 || !blows.slice(1).some(e => e.who === 0)) f.push('the ghoul\'s blow not before the character\'s (' + dq0Evs(a) + ')');
        said(a, 'Quicker, the ghoul');
        // (The gropes as it fights the ghoul: the commands typed ahead, each taking the fight over in turn (their turns
        // one, the last line's); the drop no test; a take a grope, fumbled ("You fumble for the sword.": the sword where
        // it lay, not taken): its own clumsy character's)
        const h = T.of('hold the sword'), w0 = T.of('wait', T.of('dq0 -p t -s 7 -c /ram/e.hm -k \'sword,gambeson,mail hauberk\' -T', a.k).k);   // (2026-11-04: its own clumsy character, Dexterity 1: the typed-ahead lines meet the fight's rounds as its timing has it, so a good one's take fumbled or not by the build; the sword taken after a fumble no more this test's: dq0packgrope's a grope's)
        const run = T.filter(t => t.k >= w0.k && t.k <= h.k), reps = run.flatMap(t => t.replies);
        if (h.k < 0 || w0.k < 0) return [...f, 'the gropes not typed'];
        const seq = reps.filter(r => (r.req === 'slot0' && r.kind === C.RS_TEST && r.a === C.P_GROPE) || (r.req === 'ireq' && /^IO_(DROP|GET|HOLD)$/.test(r.kind)));
        const di = seq.findIndex(r => r.kind === 'IO_DROP');
        if (di < 0 || seq[di].result !== 'IR_OK') f.push('the sword not dropped: ' + JSON.stringify(seq));
        else if (di > 0 && seq[di - 1].req === 'slot0') f.push('a drop tested');
        const fumbles = seq.filter((r, i) => r.req === 'ireq' && r.kind === 'IO_GET' && r.result === 'IR_FUMBLED' && i > 0 && seq[i - 1].req === 'slot0' && seq[i - 1].grade <= 0);
        if (!fumbles.length) f.push('no take a grope fumbled: ' + JSON.stringify(seq));
        if (seq.some((r, i) => r.req === 'ireq' && r.result === 'IR_FUMBLED' && !(i > 0 && seq[i - 1].req === 'slot0' && seq[i - 1].grade <= 0))) f.push('a fumble not a grope failed');
        if (!/You fumble for the sword\./.test(dq0Story(out))) f.push('not "You fumble for the sword."');
        if (run.flatMap(t => t.events).filter(e => e.kind === 'E_TASK' && e.te === 'TE_STOPPED' && e.task === 'LT_FIGHT').length < 2) f.push('the commands typed ahead not taking the fight over');

                // (Since slice 4.1b the ghoul, its knee broken by the torch, falls and fails its Rise (the book's test, seriously
        // wounded) as the commands go on: down, it engages no one, the take after the fumble and the hold no gropes; up, the
        // hold a grope, unless the take just before it, a grope, put the sword in hand already: nothing left for the hold to
        // do, 2026-10-31, the commands typed ahead meeting the fight's rounds as its timing has it)
        const hi = seq.findIndex(r => r.kind === 'IO_HOLD'), down = /The ghoul struggles, but can't get up\./.test(dq0Story(out));
        const inhand = hi < 0 || seq[hi].result !== 'IR_OK' || seq.slice(0, hi).some(r => r.kind === 'IO_GET' && r.result === 'IR_OK');   // (2026-11-04: a hold that did nothing, or the sword got before it: no grope to look for)
        if (!down && !inhand && seq[hi - 1].req !== 'slot0') f.push('the hold not a grope, the ghoul up: ' + JSON.stringify(seq));
        // (A quick character the ghoul comes up to: the character the quicker, its blow first)
        const w = T.filter(t => t.line === 'wait'), q = w[1], qg = q && actor(q, 'ghoul'), qc = q ? contests(q) : [];
        if (w.length !== 4 || !qg) return [...f, 'the waits not typed, or no ghoul'];
        if (!qc.length || qc[0].actor !== qg.k + 1 || qc[0].other !== 0 || qc[0].result !== 2)
          f.push('the ghoul coming up not tested, the character the quicker: ' + JSON.stringify(qc));
        const qb = q.dens.filter(e => e.kind === 'E_FIGHT');
        if (!qb.length || qb[0].who !== 0 || qb[0].def !== qg.k + 1) f.push('the character\'s blow not first (' + dq0Evs(q) + ')');
        const qs = dq0Prose(out), qa = qs.indexOf('> wait', qs.indexOf('echo ml ini 80'));     // (Its answer: the story's, the lines
        if (qa < 0 || !qs.slice(qa, qa + 400).includes('Quicker, you strike at the ghoul with your sword')) f.push('wait: not "Quicker, you strike at the ghoul with your sword" (' + qs.slice(qa, qa + 160) + ')');   //   typed ahead before it one answer)
        // (A wary rat closed with: the rat the quicker, backing away)
        const r = T.of('attack the rat'), rc = contests(r);
        if (!rc.length || rc[0].actor !== 0 || rc[0].result !== 2) f.push('the close-in on the rat not tested, the rat the quicker: ' + JSON.stringify(rc));
        dq0Need(f, r, 'E_TASK', { te: 'TE_AWAY' }, 'the rat not backing away');
        said(r, 'Quicker, the giant rat backs away from you.');
        // (dq0 -I: none tested, the ghoul's blows as they come)
        for (const t of w.slice(2)) if (contests(t).length) f.push('dq0 -I, a test all the same (' + t.k + ')');
        if (!w.slice(2).some(t => dq0Ev(t, 'E_FIGHT', { def: 0 }).length)) f.push('dq0 -I: the ghoul not come up and striking');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fighton', asks: true, what: 'Dingus Quest Zero\'s fighting on (the user\'s, 2026-10-20: "once engaged in combat, you should continue to attack your target as you can, without actively having to say "attack" each time"; HOUSE.md\'s, as built: the world\'s long task LT_FIGHT, a blow each round as the character\'s free), by the hub\'s events and the long task\'s record (the referee\'s dice at these seeds): at seed 5 one attack at the giant rats fought on, blow after blow, till its target falls senseless, then turned to the other rat engaging it ("You turn to the giant rat.") and fought till it falls too, then ended ("No foe is left beside you."), the senseless left lying and "attack the rat" finishing one, a blow alone; the status line\'s "Fighting the giant rat" as it fights; at seed 7 the hostile ghoul coming up and striking at the character as it waits fought back ("You fight back against the ghoul."), no attack typed; at seed 5 again a look typed as it fights carried out at once, the fight going on, then a walk taking it over (its round out, "You stop fighting.", the walk), fought on after it while a rat engages it still; and at seed 7 in armour but undefended (defend none), badly hurt as it fights, asked ("You\'re badly hurt.  Keep fighting?", the clock still at the turns pace), "yes" fighting on, "no" stopping it.  The scene set (since morale, 2026-10-29, and the dice by who asks, 2026-10-30, a kin seeing its fellow fall runs off, as a rule): at the first game the other rat steady (dq0Poke\'s, at the attack: its mood AM_STEADY, its courage never tested), so the fight goes on to it (dq0fight\'s the kin running off as it stands); both set hurt, the book\'s rat dodging at 90 (2026-11-04), Sword and Dodge 95; the last games\' Stamina and Will 8 (Endurance 9: badly hurt at three levels) and the ghoul\'s first two bites rigged (dice2, its shock roll\'s first die too), so the book\'s ghoul doesn\'t fell it before it asks',
      init: 't_rc', cycles: 2800e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml swd 95 >> /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T -a', 'd', 'wait', 'go right', 'attack the rat', 'attack the rat', 'look', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T -a', 'wear the gambeson', 'wear the mail hauberk', 'd', 'wait', 'quit',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T -a', 'd', 'wait', 'go right', 'attack the rat'] })) +
        dq0Typed(['quit', 'echo att sta 8 >> /ram/c.hm', 'echo att wil 8 >> /ram/c.hm', ...['yes', 'no'].flatMap(ans => ['dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson',
          'wear the mail hauberk', 'defend none', 'd', 'wait', ans, 'quit']), 'echo $status']) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'attack the rat', n: 1, k: 0, set: { mood: 8 } }, ...[0, 1, 2, 3].map(k => ({ g: 1, line: 'attack the rat', n: 1, k, hurt: 8 })),   // (The other rat steady: hub.h's AM_STEADY; the book's rat dodges at 90, 2026-11-04: both set hurt, so a blow lands)
          { g: 1, line: 'attack the rat', n: 1, rig: [{ kind: 'ATTACK', actor: 0, faces: [100, 99] }] },   // (Rigged, dice2: the other rat's bites each a critical failure met by a marginal
                                                //   failure, a fumble or a stumble, none blocked onto the sword: it isn't felled before it's turned to)
          ...[4, 5].map(g => ({ g, line: 'wait', n: 1, call: (h, L, K, at, named, sc) => {   // (The last games' ghoul rigged, dice2: its first bite a critical
            const k = [...Array(L.nactors).keys()].find(j => named(j) === 'ghoul');      //   failure, the character's opening, fought back; its second a marginal
            if (k !== undefined) sc.rig([{ kind: 'ATTACK', actor: k, uses: 1, faces: [100] }, { kind: 'ATTACK', actor: k, uses: 1, faces: [99, 2, 8, 1] }]);   //   failure's
          } }))]);                                //   one die, a 2, at the face (8): an S3, badly hurt at Endurance 9, its shock roll's first die a 1)
        dq0OnWords(m, [{ g: 3, line: 'attack the rat', n: 1, words: 'You strike at the giant rat', text: 'look\r' },   // (Typed as it fights:
          { g: 3, line: 'look', n: 1, words: 'You ', text: 'go left\r' }]);   //   the look at its first blow, the walk at the look's words, 2026-10-31)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], FF_D_DOWN = 4, FF_D_DEAD = 8;
        const mine = t => dq0Ev(t, 'E_FIGHT', { who: 0 }), tasks = t => dq0Ev(t, 'E_TASK', {});
        // (One attack fought on: its target senseless, the other rat turned to and felled, the fight's end told)
        const a = T.of('attack the rat'), fin = T.after(a, 'attack the rat');
        const b = mine(a), first = b[0];
        if (b.length < 3 || !first) return [...f, 'the attack not fought on (' + dq0Evs(a) + ')'];
        dq0Need(f, a, 'E_TASK', { te: 'TE_FIGHT', quiet: true, by: first.def - 1 }, 'fighting on not begun by the attack');
        const down1 = b.find(e => e.def === first.def && (e.ff & FF_D_DOWN));
        if (!down1) f.push('its target not fought till it fell senseless (' + dq0Evs(a) + ')');
        const turned = dq0Ev(a, 'E_TASK', { te: 'TE_FIGHT', still: true });
        if (turned.length !== 1 || turned[0].by + 1 === first.def) f.push('not turned to the other rat (' + dq0Evs(a) + ')');
        else if (!b.some(e => e.def === turned[0].by + 1 && (e.ff & (FF_D_DOWN | FF_D_DEAD)))) f.push('the other rat not fought till it fell');
        if (b.some(e => e.out === 'O_DISPATCH')) f.push('a senseless rat finished, unasked');   // (The other killed outright since D13's free pounds: no burden)
        dq0Need(f, a, 'E_TASK', { te: 'TE_NO_FOE', task: 'LT_FIGHT' }, 'the fight\'s end not told');
        if (!a.after || a.after.pc.task.kind !== 'LT_NONE') f.push('the fight not over: ' + (a.after && a.after.pc.task.kind));
        if (!mine(fin).some(e => e.out === 'O_DISPATCH' && (e.ff & FF_D_DEAD))) f.push('"attack the rat" not finishing one (' + dq0Evs(fin) + ')');
        if (mine(fin).length !== 1 || dq0Ev(fin, 'E_TASK', { te: 'TE_NO_FOE' }).length) f.push('the finishing blow not alone, untold (' + dq0Evs(fin) + ')');
        if (!/Fighting the giant rat/.test(out)) f.push('no "Fighting the giant rat" on the status line');
        // (The ghoul coming up and striking, fought back: no attack typed)
        const w = T.of('wait', T.of('dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T').k), gh = w.at && w.at.actors.find(x => x.name === 'ghoul');
        if (!gh) return [...f, 'no ghoul'];
        dq0Need(f, w, 'E_FIGHT', { who: gh.k + 1, def: 0 }, 'the ghoul not striking at the character');
        dq0Need(f, w, 'E_TASK', { te: 'TE_FIGHT', quiet: false, still: false, by: gh.k }, 'the ghoul not fought back');
        if (!mine(w).some(e => e.def === gh.k + 1)) f.push('no blow struck back at the ghoul (' + dq0Evs(w) + ')');
        // (A look as it fights, at once; a walk taking it over, then fought on)
        const g3 = T.of('dq0 -p t -s 5 -c /ram/c.hm -k sword -T', w.k), a3 = T.after(g3, 'attack the rat'), lk = T.after(a3, 'look'), gl = T.after(a3, 'go left');
        const run = [a3, lk, gl].filter(t => t.k >= 0), evs = run.flatMap(tasks);
        if (gl.k < 0) return [...f, 'the look and the walk not typed'];
        const stops = evs.filter(e => e.te === 'TE_STOPPED');
        if (stops.length !== 1 || stops[0].task !== 'LT_FIGHT') f.push('the fight not stopped once, by the walk: ' + evs.map(e => e.te + ':' + e.task).join(', '));
        const story = dq0Prose(out), at3 = story.indexOf('> attack the rat', story.indexOf('> go right', story.lastIndexOf('dq0 -p t -s 5')));
        const said = story.slice(at3), seen = said.indexOf('You can see'), stop = said.indexOf('You stop fighting.');
        if (seen < 0 || stop < 0 || seen > stop || said.slice(0, seen).indexOf('You strike at') < 0) f.push('the look not carried out as it fought, before the walk took it over');
        else if (!/You strike at|bites at you/.test(said.slice(seen, stop))) f.push('the fight not going on after the look');
        const after = run.flatMap(t => [...t.events, ...t.dens]).filter(e => e.kind === 'E_TASK' || e.kind === 'E_ARRIVE' || e.kind === 'E_FIGHT');
        const si = after.findIndex(e => e.kind === 'E_TASK' && e.te === 'TE_STOPPED');
        if (si < 0 || !after.slice(si).some(e => e.kind === 'E_ARRIVE' || (e.kind === 'E_TASK' && e.te === 'TE_ENGAGED'))) f.push('the walk not carried out after the stop');
        if (si < 0 || !after.slice(si).some(e => e.kind === 'E_TASK' && e.te === 'TE_FIGHT')) f.push('not fought on after the walk, a rat engaging it still');
        // (Badly hurt as it fights, undefended: asked, the clock still; yes on, no stopped)
        for (const ans of ['yes', 'no']) {
          const t = T.of(ans), q = T.of('wait', T.of('defend none', ans === 'yes' ? 0 : T.of('yes').k).k);
          const ask = dq0Ev(q, 'E_TASK', { te: 'TE_ASK_HURT', task: 'LT_FIGHT' });
          if (!ask.length || ask[0].health !== 'H_BADLY') { f.push(ans + ': badly hurt as it fought, not asked (' + dq0Evs(q) + ')'); continue; }
          if (!t.at || t.at.now !== ask[0].t) f.push(ans + ': the clock not still as it asked (' + (t.at && t.at.now) + ', asked at ' + ask[0].t + ')');
          if (ans === 'yes' && !mine(t).length) f.push('"yes" not fighting on (' + dq0Evs(t) + ')');
          if (ans === 'no') {
            dq0Need(f, t, 'E_TASK', { te: 'TE_STOPPED', task: 'LT_FIGHT' }, '"no" not stopping the fight');
            if (mine(t).length) f.push('"no": fought on (' + dq0Evs(t) + ')');
          }
        }
        return f;
      },
      sdcard: true
    },
    {
      lostLines: true, asks: true, initiative: true, reach: true, outfit: true, bearings: true, fit: true, hours: true, name: 'dq0deadonce', near: ['fight', 'body'], what: "Dingus Quest Zero's death in a run of blows (the fight soak's finds, 2026-11-04; its seed 14's scene, the commands as it played them, as nothing simpler brought it about: re-baseline the commands if the dice shift): small orcs shooting at a character as it walks over to one with a blow in hand, the fatal shot among others of the round: 'You are dead.' and 'The game is over.' told once, no blow told after the end (they were, each ending again, as the round's blows were told out), no blow at the body once it's dead (the world's: it was struck on), and the game gone back to the shell (a look posted after the quit's round, the senses gone, hung the world: dq0w's looked).",
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 14 -o /ram/c.hm', 'echo hour 128 >> /ram/s.sc', 'mkdir /ram/g',
        "echo 'faction | Diggers | khuzmin | 0-1 | 29 | - | 0 | h | -' > /ram/g/theme.txt", "echo 'faction | Tribe | arak | 0-1 | 17 | - | 0 | h | Diggers h' >> /ram/g/theme.txt",
        "dq0 -p t -s 14 -c /ram/c.hm -T -d /ram/g -k 'club' -S /ram/s.sc", 'attack', 'wait', 'fight the orc', 'echo =END', 'echo $status']) },   // (Its "kick the orc" gone, 2026-11-13: typed at a
                                                //   prompt that came as it lay senseless; now none comes till it comes round, and it dies first)
      start(m) {        // (The first shot at it as it goes over a critical success it fails to dodge, three dice, two of them 6s: a
                        //   mortal wound; dice2's keyed dice had a serious stab, it walked on.  Its own blows kept from the rig)
        dq0Scene(m, [{ g: 1, line: 'fight the orc', n: 1, rig: [{ kind: 'ATTACK', actor: -1, faces: [] }, { kind: 'ATTACK', uses: 1, faces: [5, 100, 6, 6] }] }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], T = dq0Turns(m), a = dq0Prose(out), FF_D_DEAD = 8;
        const over = (a.match(/The game is over\./g) || []).length, dead = (a.match(/You are dead\./g) || []).length;
        if (over !== 1 || dead !== 1) f.push('the end told ' + over + ' times, the death ' + dead + ': ' + a.slice(-400));
        const after = a.slice(a.indexOf('The game is over.'));
        if (/ at you\b/.test(after)) f.push('a blow told after the end: ' + after.slice(0, 300));
        const blows = T.filter(t => t.game === 0).flatMap(t => [...t.events, ...t.dens]).filter(e => e.kind === 'E_FIGHT' && e.def === 0 && e.ff !== undefined).sort((x, y) => x.t - y.t);
        const k = blows.findIndex(e => e.ff & FF_D_DEAD);
        if (k < 0) f.push('the scene not set: no fatal blow in ' + blows.length);
        else if (k < blows.length - 1) f.push('blows at the character after it was dead: ' + (blows.length - k - 1));
        this.notes = [blows.length + ' blows at it, the fatal the ' + (k + 1) + 'th'];
        return f;
      },
      sdcard: true
    },
    {
      initiative: true, name: 'dq0outcold', near: ['fight'], what: "Dingus Quest Zero's lines typed as the character lies senseless (the user's, 2026-11-13: \"For knocked out players ... It might be better to just ignore input or not allow it at all when knocked out\"; DECISIONS.md's \"Commands while knocked out\", as built, for review): dq0senseless's rats (seed 3, no defence), a line typed as it's knocked senseless (\"drop the torch\", at \"Everything goes black.\") refused (\"You lie senseless.\"), no intent sent for it, and not carried out as it comes round (the torch still in hand at the quit); and dq0deadonce's orcs (seed 14), a line typed as it falls senseless refused the same, the character dying senseless, the game's end told once and the game gone back to the shell (the latch tried before hung its quit there); and nothing sensed as it lies senseless (the user's, 2026-11-14: DECISIONS.md's \"A knocked-out character senses nothing\"), though the denizens go about and the orcs strike at it: from the black-out to the death only the refusal, then \"You are dead.\"; from the rats' black-out to its coming round only the refusal and, as it comes round, the denizens it sees then",
      init: 't_rc', cycles: 2800e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo ml una 5 >> /ram/c.hm', 'mkdir /ram/g', 'cp /sd/0/games/lib/dq0/inhabit.txt /ram/g/theme.txt',
        'dq0 -p t -s 3 -c /ram/c.hm -T -d /ram/g', 'defend none', 'wait', 'wait', 'wait', 'quit',
        'hmchar -q -s 14 -o /ram/d.hm', 'echo hour 128 >> /ram/s.sc', 'mkdir /ram/h',
        "echo 'faction | Diggers | khuzmin | 0-1 | 29 | - | 0 | h | -' > /ram/h/theme.txt", "echo 'faction | Tribe | arak | 0-1 | 17 | - | 0 | h | Diggers h' >> /ram/h/theme.txt",
        "dq0 -p t -s 14 -c /ram/d.hm -T -d /ram/h -k 'club' -S /ram/s.sc", 'attack', 'wait', 'echo =END', 'echo $status'] })) },
      start(m) {        // (Each line typed as the black-out's told, the character senseless)
        dq0OnWords(m, [{ g: 1, line: 'wait', n: 1, words: 'Everything goes black.', text: 'drop the torch\r' },
          { g: 2, line: 'wait', n: 1, words: 'Everything goes black.', text: 'kick the orc\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], T = dq0Turns(m), a = dq0Prose(out), g1 = a.slice(0, a.indexOf('% hmchar -q -s 14')), g2 = a.slice(g1.length);
        const refused = (g, line) => {
          const i = g.indexOf('> ' + line), next = i < 0 ? '' : g.slice(i + line.length + 2).trim();
          if (i < 0) f.push('"' + line + '" not typed');
          else if (!next.startsWith('You lie senseless.')) f.push('"' + line + '" not refused: ' + next.slice(0, 200));
          else if (g.indexOf('Everything goes black.') > i) f.push('"' + line + '" typed before the black-out');
          const t = T.of(line);
          if (t.k >= 0 && t.intents.some(x => x.ring === 'ui' && x.kind !== 'I_QUIT')) f.push('"' + line + '" sent the world an intent: ' + t.intents.filter(x => x.ring === 'ui').map(x => x.kind).join(', '));
        };
        refused(g1, 'drop the torch');
        if (/You drop|You put down|You let go of the torch/.test(g1)) f.push('the torch dropped as it came round: ' + (g1.match(/[^.]*(?:You drop|You put down|You let go of the torch)[^.]*\./) || [''])[0]);
        const q = T.of('quit');
        if (!q.at || !dq0Carried(q.at).some(i => i.name === 'torch')) f.push('the torch not still carried at the quit');
        if (!/Time passes\./.test(g1.slice(g1.indexOf('You lie senseless.')))) f.push('the waits after it not taken as it came round');
        refused(g2, 'kick the orc');
        const over = (g2.match(/The game is over\./g) || []).length, dead = (g2.match(/You are dead\./g) || []).length;
        if (dead !== 1 || over !== 1) f.push('dying senseless: the death told ' + dead + ' times, the end ' + over);
        if (g2.indexOf('You are dead.') < g2.indexOf('You lie senseless.')) f.push('not dead senseless (the refusal after the death)');
        if (!/% echo =END\s+=END/.test(g2)) f.push('the game not gone back to the shell after the death');
        if (/kick: not found/.test(g2)) f.push('the line typed as it lay senseless left for the shell');
        // (Nothing sensed as it lies senseless, the user's, 2026-11-14: from the orcs' black-out to its death only the line refused,
        // though they strike at it and go about meanwhile; from the rats' to its coming round only the line refused and, as it
        // comes round, the denizens it sees then, once)
        const lay2 = g2.slice(g2.indexOf('Everything goes black.') + 22, g2.indexOf('You are dead.')).replace('> kick the orc', '').replace('You lie senseless.', '').trim();
        if (/[.!?]/.test(lay2)) f.push('told as it lay senseless (the orcs): ' + lay2.slice(0, 200));
        const k2 = T.of('kick the orc');
        if (!dq0Ev(k2, 'E_FIGHT', { def: 0 }).length || !dq0Ev(k2, 'E_DEN', {}).length) f.push('no blow at it nor a denizen\'s doing as it lay senseless (the orcs): the check proves nothing');
        const b1 = g1.indexOf('Everything goes black.'), lay1 = g1.slice(b1 + 22, g1.indexOf('> wait', b1)).replace('> drop the torch', '').replace('You lie senseless.', '').trim();
        if ((lay1.match(/stirs/g) || []).length > 1 || /fighting you|You hear/.test(lay1)) f.push('told as it lay senseless (the rats): ' + lay1.slice(0, 200));
        if (!dq0Ev(T.of('drop the torch'), 'E_DEN', {}).length) f.push('no denizen\'s doing as it lay senseless (the rats): the check proves nothing');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fightcmd', near: ['fight', 'denizens'], what: "Dingus Quest Zero's commands typed during a fight (the coordinator's, gw2, 2026-11-19: the character's fight and its other long tasks take the one give-way, so a command typed mid-fight has its first letter in time; gw1's dq0letdens, a rat fighting the character as attack A was typed, 837,757 cycles at the menacing pace): dq0fight's rats at seed 5, one put before the character, hostile and fighting it, as the first command's typed (dq0Poke's engage), then inventory, look, inventory and look typed as it fights on, a wait between each for its blows (a blow struck at the character, and a rat fighting as one of them was typed), each one's first letter within the user's 720,000 cycles (D17), at the turns pace, the menacing pace and as played (dq0Paces)",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'echo b115200 >/dev/serctl',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T', ...FIGHTCMD, 'quit', ...dq0Paces('dq0 -p t -s 5 -c /ram/c.hm -k sword -T', FIGHTCMD), 'echo $status'] })) },
      start(m) {
        this.timer = dq0Timer(m);
        dq0Asks(m, [3]);
        dq0Scene(m, [1, 2, 3].map(g => ({ g, line: 'inventory', n: 1, k: 0, beside: 'face', engage: true,   // (A rat put before the character, hostile and fighting it, as the first command's typed)
          rig: [{ kind: 'ATTACK', actor: -1, faces: [] }, { kind: 'ATTACK', faces: [99, 1] }] })));   // (The rats' bites a marginal failure
        //   met by a marginal success, dodged, dice2; the character's blows as the keys have them: at the menacing pace a run of
        //   bites felled the character, its shock rolls an hour's, and the played game's start ran out of the test's cycles)
        dq0FightPause(m);                       // (A player's pause before a line typed in the fight: the user's T3, 2026-11-22)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], notes = this.notes = [];
        const mid = T.filter(t => ['inventory', 'look'].includes(t.line) && t.at && t.at.actors.some(a => a.doing === 'DO_FIGHT'));
        if (!T.some(t => dq0Ev(t, 'E_FIGHT', { def: 0 }).length)) f.push('no blow struck at the character: no fight');
        if (!mid.length) f.push('no command typed as a rat fought the character (none fighting as inventory or look was typed): the test proves nothing');
        dq0HeldPaces(this, f, notes, this.timer, [0, 1, 2], FIGHTCMD.map(l => ({ line: l, want: '*' })));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fightmid', near: ['fight', 'denizens'], what: "Dingus Quest Zero's commands typed in the middle of the character's own round (the coordinator's, start2, 2026-11-20: a line typed as the character fights on is held till the round's blow is struck, the blow's referee test then worked in the background, 830,000 cycles in start1's trace, a D17 risk the suite didn't reach): dq0fightcmd's rats at seed 5, one put before the character, hostile and fighting it (dq0Poke's engage, as inventory's typed); then look, wait, inventory and go left (the fight taken up again after the wait as the rat bites), each typed (3M cycles after the line before at least, no prompt waited for: the fight goes on) as the world's referee slot holds the character's own blow (RS_ATTACK, the character striking, not yet answered), each one's first letter within the user's 720,000 cycles (D17), at the normal pace, the menacing pace and as played (dq0Paces)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'echo b115200 >/dev/serctl',
        'dq0 -p n -s 5 -c /ram/c.hm -k sword -T', ...FIGHTMID, 'quit', ...dq0Paces('dq0 -p n -s 5 -c /ram/c.hm -k sword -T', FIGHTMID), 'echo $status'] })).split('\u0101' + MID_MARK).join(MID_MARK) },   // (A line typed mid-round: not after a prompt, the fight going on)
      start(m) {
        this.timer = dq0Timer(m);
        dq0Asks(m, [3]);
        dq0Scene(m, [...[1, 2, 3].map(g => ({ g, line: 'inventory', n: 1, k: 0, beside: 'face', engage: true })),   // (A rat put before the character, hostile and fighting it)
          { g: 2, line: 'inventory', n: 1, rig: [{ kind: 'ATTACK', actor: -1, faces: [99] }] }]);   // (The menacing game's blows rigged, dice2: each a marginal failure,
                                                //   no wound, the rat fighting on through the lines: its first felled it, a gash)
        // (Each line marked MID_MARK typed only as the world's referee slot holds the character's own blow: posted, not yet
        // answered; or, none come in NO_ROUND_MID cycles, typed anyway and said so)
        const H = require(path.join(TESTS, '..', 'games', 'dq0', 'hubgen.js')).load(), a = m.acia, tick = a.tick, self = this;
        const ST = H.off('hub', 'slot.state'), KD = H.off('hub', 'slot.kind'), AC = H.off('hub', 'slot.actor');
        let since = -1;
        const P = dq0FightPause(m);             // (A player's pause between lines mid-fight, the user's T3, 2026-11-22: a line
        this.mids = [];                         //   typed mid-round once the one before is answered and a second on, PLAYER_PAUSE)
        const blow = () => { const h = m.dq0 && m.dq0.bank(0); return !!h && h[ST] === H.C.REQ_POSTED && h[KD] === H.C.RS_ATTACK && h[AC] === 0; };
        a.tick = (d, t) => {
          if (a.rxQueue[0] !== MID_MARK) { since = -1; return tick(d, t); }
          if (since < 0) since = m.cpu.cyc;
          const hit = blow();
          if ((hit && m.cpu.cyc - since >= MID_GAP && P.ready(m.cpu.cyc)) || m.cpu.cyc - since > NO_ROUND_MID) {
            a.rxQueue.shift();
            self.mids.push({ at: m.cpu.cyc, round: hit, line: a.rxQueue.slice(0, a.rxQueue.indexOf('\r')).join('') });
            return tick(d, t);
          }
          const q = a.rxQueue.splice(0);
          try { return tick(d, t); } finally { a.rxQueue.push(...q); }
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], notes = this.notes = [], lines = FIGHTMID.map(l => l.replace(MID_MARK, ''));
        const missed = this.mids.filter(x => !x.round);
        if (this.mids.length !== 3 * FIGHTMID.filter(l => l[0] === MID_MARK).length) f.push(this.mids.length + ' lines typed mid-round, not ' + 3 * FIGHTMID.filter(l => l[0] === MID_MARK).length);
        if (missed.length) f.push(missed.length + ' line(s) typed with no blow of the character\'s under way (' + missed.map(x => x.line).join(', ') + '): no round met');
        dq0HeldPaces(this, f, notes, this.timer, [0, 1, 2], lines.map(l => ({ line: l, want: '*' })));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0felled', near: ['fight', 'denizens'], what: "Dingus Quest Zero's foe felled while a line waits (reach1's find, 2026-11-20: dq0fightmid's \"go left\" once 6,506,445 cycles, the blow knocking the rat senseless as the line waited, the referee's own work 4.52M of it: its rounds out worked by the book's shock rolls, a round each, up to an hour's, each a die a point of its universal penalty), by the words and dq0Timer's times: dq0fightmid's rats at seed 5, one put before the character hostile and fighting it, and hurt (a minor wound of level 8: its every shock roll failed, the most rounds out the referee works: an hour's); then lines typed as the character's own blow is being worked (dq0fightmid's way): looks, a wait and a step; a blow felling the rat as one waits, and every first letter held to D17, at the normal pace, the menacing and as played",
      init: 't_rc', cycles: 3200e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att sta 99 >> /ram/c.hm', 'echo att wil 99 >> /ram/c.hm', 'echo ml swd 90 >> /ram/c.hm', 'echo b115200 >/dev/serctl',
        'dq0 -p n -s 5 -c /ram/c.hm -k sword -T', ...FELLED, 'quit', ...dq0Paces('dq0 -p n -s 5 -c /ram/c.hm -k sword -T', FELLED), 'echo $status'] })).split('ā' + MID_MARK).join(MID_MARK) },
      // (No -R since dice2's keyed dice, 2026-11-24: the character's first blow rigged (start's), the rat felled, not
      // killed, as the first look waits, in every game; its -R pins, scene1's and speed11's, the dice searched for it)
      start(m) {
        this.timer = dq0Timer(m);
        dq0Asks(m, [3]);
        dq0Scene(m, [1, 2, 3].map(g => ({ g, line: 'inventory', n: 1, k: 0, beside: 'face', engage: true, hurt: 8,   // (A rat before it, fighting it, its every shock roll failed)
          rig: [{ kind: 'ATTACK', actor: -1, uses: 1, faces: [1, 99, 1] }] })));   // (The character's first blow a light hit, AH(1) and a 1: the rat felled, not killed, dice2's keyed dice had its counterstrike kill it)
        const H = require(path.join(TESTS, '..', 'games', 'dq0', 'hubgen.js')).load(), a = m.acia, tick = a.tick, self = this;
        const ST = H.off('hub', 'slot.state'), KD = H.off('hub', 'slot.kind'), AC = H.off('hub', 'slot.actor');
        let since = -1;
        const P = dq0FightPause(m);             // (A player's pause between lines mid-fight, the user's T3, 2026-11-22: a line
        this.mids = [];                         //   typed mid-round once the one before is answered and a second on, PLAYER_PAUSE)
        const blow = () => { const h = m.dq0 && m.dq0.bank(0); return !!h && h[ST] === H.C.REQ_POSTED && h[KD] === H.C.RS_ATTACK && h[AC] === 0; };
        a.tick = (d, t) => {   // (dq0fightmid's: each line marked MID_MARK typed as the character's own blow is being worked)
          if (a.rxQueue[0] !== MID_MARK) { since = -1; return tick(d, t); }
          if (since < 0) since = m.cpu.cyc;
          const hit = blow();
          if ((hit && m.cpu.cyc - since >= MID_GAP && P.ready(m.cpu.cyc)) || m.cpu.cyc - since > NO_ROUND) {
            a.rxQueue.shift();
            self.mids.push({ at: m.cpu.cyc, round: hit, line: a.rxQueue.slice(0, a.rxQueue.indexOf('\r')).join('') });
            return tick(d, t);
          }
          const q = a.rxQueue.splice(0);
          try { return tick(d, t); } finally { a.rxQueue.push(...q); }
        };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], notes = this.notes = [], lines = FELLED.map(l => l.replace(MID_MARK, ''));
        // (A blow felling the rat in the answer of a line typed mid-round, each game: its words in that line's answer.  Its
        // shock rolls a round each as its time comes since felled1 (the user's, 2026-11-21), not an hour's as it falls: the
        // felling cheap whichever line meets it; each game's line noted)
        const o = dq0Prose(out), games = o.split(/% dq0 -p [nm] -s 5/).slice(1), felled = [];
        games.forEach((g, i) => {
          const mids = lines.filter((l, k) => FELLED[k][0] === MID_MARK), segs = g.split(/> (?=look|wait|go left|inventory)/);
          const s = segs.find(s => mids.some(l => s.startsWith(l)) && /falls senseless|knocked senseless|goes down/.test(s));
          if (s) felled.push((i + 1) + ': ' + s.split(' ')[0]);
          else f.push('game ' + (i + 1) + ': no blow felling the rat as a line waited');
        });
        notes.push('felled as a line waited, by game: ' + (felled.join(', ') || 'none'));
        if (games.length !== 3) f.push(games.length + ' games, not 3');
        dq0HeldPaces(this, f, notes, this.timer, [0, 1, 2], lines.map(l => ({ line: l, want: '*' })));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pcfelled', near: ['fight', 'denizens'], what: "Dingus Quest Zero's character felled while a line waits (dice2's find, 2026-11-25: dq0fightcmd's \"wait\" typed as a run of keyed bites felled the character, 824,650-915,210 cycles to its first letter; the mirror of dq0felled's foe felled, P2): dq0fightcmd's way to the rats at seed 5, the character frail (STR, STA, WIL and END 1: any wound's shock roll fails), one rat put before it hostile and fighting it as inventory's typed, its bites rigged to land and the character's to miss; then lines typed at the prompt as it fights on (a player's pause before each), one of them meeting the bite that fells it (\"Everything goes black.\"): every first letter held to D17, at the turns pace, the menacing and as played (fix2's: a line held behind the round a command in hand, the news told before the next intent)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att str 1 >> /ram/c.hm', 'echo att sta 1 >> /ram/c.hm', 'echo att wil 1 >> /ram/c.hm', 'echo att end 1 >> /ram/c.hm', 'echo b115200 >/dev/serctl',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T', ...PCFELLED, 'wait 1 minute', ...dq0Paces('dq0 -p t -s 5 -c /ram/c.hm -k sword -T', [...PCFELLED, 'wait 1 minute']).filter(l => l !== 'quit'), 'echo $status'] })) },   // (A wait more each: the bite to come as it's typed, if not before; after the quit, the shell's)
      start(m) {
        this.timer = dq0Timer(m);
        dq0Asks(m, [3]);
        dq0Scene(m, [1, 2, 3].map(g => ({ g, line: 'inventory', n: 1, k: 0, beside: 'face', engage: true,   // (A rat before it, fighting it)
          rig: [{ kind: 'ATTACK', actor: -1, faces: [99, 1] }, { kind: 'ATTACK', faces: [1, 99] }] })));   // (The character's blows miss, the
        dq0FightPause(m);                       //   rat's land: a wound, its shock roll failed, the character felled as it fights on)
        dq0OnWords(m, [1, 2, 3].map(g => ({ g, line: 'inventory', n: 1, words: 'Everything goes black.', text: 'quit\r' })));   // (Quit as it's
        //   told: senseless, the quit taken whatever (the user's, 2026-11-13); at the real paces no prompt comes till it comes round)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], notes = this.notes = [], o = dq0Prose(out), games = o.split(/% dq0 -p [tmn] -s 5/).slice(1), at = [];
        games.forEach((g, i) => {
          const segs = g.split(/> (?=wait|inventory)/), s = segs.findIndex(x => /Everything goes black\./.test(x));
          if (s < 0) f.push('game ' + (i + 1) + ': the character not felled as a line waited');
          else at.push((i + 1) + ': ' + segs[s].split(' ')[0]);
        });
        notes.push('felled in the answer of, by game: ' + (at.join(', ') || 'none'));
        if (games.length !== 3) f.push(games.length + ' games, not 3');
        dq0HeldPaces(this, f, notes, this.timer, [0, 1, 2], PCFELLED.map(l => ({ line: l, want: '*' })));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0numb', near: ['fight', 'time'], what: "Dingus Quest Zero's long task stopped as the character lies senseless, untold (WORLD-MODEL.md's B3: the senseless sense nothing; dice2's find, dq0hoard's \"You stop waiting.\" three times after the black-out, fix3, 2026-11-25): dq0pcfelled's way to the rats at seed 5, the character frail (any wound's shock roll fails), waiting a minute as a rat is put before it, hostile, its bites rigged to land: felled as it waits (\"Everything goes black.\"), its wait stopped by the world (E_TASK's stop, the command dropped), and no stop told after the black-out (sim/lib/dq0inv.js's numb, every test's)",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(dq0Game({ lines: ['echo att str 1 >> /ram/c.hm', 'echo att sta 1 >> /ram/c.hm', 'echo att wil 1 >> /ram/c.hm', 'echo att end 1 >> /ram/c.hm',
        'dq0 -p t -s 5 -c /ram/c.hm -k sword -T', 'd', 'wait', 'go right', 'wait 1 minute', 'wait 1 minute', 'quit', 'echo $status'] })) },
      start(m) {
        dq0Scene(m, [{ g: 1, line: 'wait 1 minute', n: 1, k: 0, beside: 'face', set: { attitude: 0 },
          rig: [{ kind: 'ATTACK', actor: -1, faces: [99, 1] }, { kind: 'ATTACK', faces: [1, 99] }] }]);   // (A rat before it, hostile, its bites landing)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], w = T.of('wait 1 minute'), s = dq0Prose(out);
        if (!w.at) return ['the wait not typed'];
        const a = s.slice(s.indexOf('> wait 1 minute'), s.indexOf('> quit', s.indexOf('> wait 1 minute')));   // (Both waits' answers)
        if (!/Everything goes black./.test(a)) f.push('not felled as it waited: ' + a.slice(0, 500));
        const stops = T.filter(t => t.k >= w.k).flatMap(t => dq0Ev(t, 'E_TASK').filter(e => /^TE_(?:HIT|STOPPED|NEAR|ENGAGED)$/.test(e.te)));
        if (!stops.length) f.push('no stop of its wait by the world (E_TASK): the case not met');
        const after = a.slice(a.indexOf('Everything goes black'));
        if (/[Yy]ou stop [a-z]+ing./.test(after)) f.push('a stop told as it lies senseless: ' + after.slice(0, 400));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0turnback', near: ['denizens', 'senses'], what: "Dingus Quest Zero's turn back to a heading told already in one answer (WORLD-MODEL.md's 7, nothing told twice; dice2's find, dq0follow's turns, fix3, 2026-11-25): dq0follow's second game as it was before dice2 re-aimed it (seed 5, down the ladder, 'follow the rat' with every rat about): the follow looks round for the rat gone behind it (\"You turn to face southeast.\"), walks after it as it's seen again (a step's turn untold), and looks round for it again the same way: \"You turn back to face southeast.\", not the same words twice",
      init: 't_rc', cycles: 1000e6,
      machine: { input: dq0Typed(dq0Game({ char: 'hmchar -q -s 5 -u feudal -c hunter -o /ram/a.hm', lines: ['echo ml ani 90 >> /ram/a.hm',
        'dq0 -p t -s 5 -c /ram/a.hm -T', 'd', 'wait', 'follow the rat', 'quit', 'echo $status'] })) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], a = (dq0Answers(out).find(x => x.cmd === 'follow the rat') || {}).text || '';
        const turns = a.match(/You turn (?:back )?to face [a-z]+\./g) || [];
        const seen = new Set();
        for (const t of turns) { if (seen.has(t)) f.push('told twice in the follow\'s answer: ' + t); seen.add(t); }
        if (!turns.length) f.push('no turn in the follow\'s answer: the case not met: ' + a.slice(0, 300));
        this.notes = ['the follow\'s turns: ' + turns.join(' ')];
        return f;
      },
      sdcard: true
    },
  ],
};
