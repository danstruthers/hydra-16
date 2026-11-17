// ****************************************************************************
// screen.js - Dingus Quest Zero's tests: the screens: the Vera X, the prompt, the answer's time.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const { typed } = require('../../lib/common.js');
const fs = require('fs');
const path = require('path');
const { dq0StatusRows, dq0Typed, DQ0_MAKE, dq0Story } = require('../../lib/dq0.js');
// (dq0played's lines: the play test's first game, 2026-11-16, review/play's g1.json, as typed, but its three "break the
// door" with the lantern named, the tool the game chose for it till then: a bare "break the door" punches since the things
// branch's nothing battered with a light, 2026-11-17, and the weak scribe's fists put it in shock before the door gives, the
// rest of the game never played)
const PLAYED = ['help', 'look', 'i', 'status', 'x room', 'map', 'l', 'z', 'x case', 'open the case', 'look in the case', 'take all from the case', 'get all', 'look under the bed', 'g', 'again', 'undo', 'examine the skylight', 'look at the stairs', 'climb the stairs', 'go to the door', 'open the door', 'look', 'go through the door', 'look', 'map', 'unlock the door', 'i', 'drop the case', 'pick the lock', 'force the door', 'kick the door', 'break the door with the lantern', 'search', 'search the room', 'look', 'x door', 'look around', 'turn around', 'look', 'break the door with the lantern', 'break the door with the lantern', 'status', 'break the door with the knife', 'look', 'go through the door', 'look', 'map', 'f', 'look', 'f', 'listen', 'smell', 'f', 'look', 'map', 'open the door', 'go through the door', 'look', 'f', 'look', 'go back', 'look', 'turn left', 'sw', 'look', 'sw', 'look', 'map', 'w', 'look', 'open the case', 'look in it', 'put the knife in the case', 'look in the case', 'take the knife from the case', 'close the case', 'take E', 'i', 'take C', 'x pit', 'look in the pit', 'go to the pit', 'look down', 'drop the candle stubs in the pit', 'i', 'look', 'drop the stubs into the pit', 'put the stubs in the pit', 'jump into the pit', 'climb down into the pit', 'enter the pit', 'go through the left door', 'look', 'f', 'look', 'open the door', 'go back', 'look', 'go through the right door', 'look', 'f', 'look', 'open the door', '1', 'take F', 'x it', 'break the door', 'go through the door', 'look', 'map'];

module.exports = {
  tests: [
    // ---- (The rules' round, 2026-11-03)
    {
      name: 'dq0clock', near: ['screen'], what: "Dingus Quest Zero's status line and the hour (the user's, 2026-11-03, DECISIONS.md's the round after HOUSE.md's check: \"Hide the minutes\"; HOUSE.md's the time of day believed): no minutes gone on the status line, by the screen's rows as the clock goes; dq0 -m, the tests' and debugging's, puts them back",
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'wait 2 minutes', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -m', 'wait 2 minutes', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], at = out.indexOf('-T -m');
        if (at < 0) return ['the second game not begun'];
        const one = dq0StatusRows(out.slice(0, at)), two = dq0StatusRows(out.slice(at));
        if (!/facing /.test(one)) f.push('no status line in the first game');
        if (/\d+ min\b/.test(one)) f.push('the minutes gone on the status line: ' + (one.match(/.*\d+ min\b.*/) || [''])[0]);
        if (!/\b\d+ min\b/.test(two)) f.push('dq0 -m: no minutes on the status line');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0vera',what: 'Dingus Quest Zero on the Vera X (the user\'s, 2026-10-15: "If the Vera is available, this should be running there, and keyboard would come from the SMC"; 2026-10-16: "Full screen is fine"): started with the console\'s terminals both, the game takes the screen alone (consctl\'s terminal screen: its window 57 rows) and puts them back as it ends; typed at the SMC\'s keyboard: the status line reversed on the window\'s first row, the line typed on its last, the map, Backspace, Escape pausing and going on, the cursor key up recalling the last line, the help\'s word on the serial port\'s rates',
      init: 't_rc', cycles: 1400e6,
      // (Snapshots of the screen as the keyboard's typing comes to SNAP: smc.type's, wrapped; P a wait, W a prompt: the
      // serial port's, so W only while it shows the window)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push({ text: m.vera.text(), cells: m.vera.cells() }) : type(ch);
      },
      get machine() {
        const W = 'ā', P = 'Ā', SNAP = 'Ѐ', UP = String.fromCharCode(0x200 + 83), C = P.repeat(30);
        return { vera: true, smc: true, input: W + 'bind -a /sd/0/games /bin\r' + W + 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm\r' + 'Ă' +
          W + 'dq0 -p t -s 3 -c /ram/c.hm\r' + P.repeat(150) + 'look\r' + C + 'mapx\b\r' + C + SNAP + '\x1b' + P + SNAP + '\x1b' + P +
          'lo' + UP + '\r' + C + SNAP + 'help\r' + C + SNAP + 'quit\r' + P.repeat(20) + 'ă' + 'cat /dev/consctl\r' + W + 'echo do^ne\r' };
      },
      expect: ['\ndone\n', 'terminal both'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 4 || s.some(p => !p.cells)) return ['the screen snapshots: ' + s.length + ' (4 wanted, each with a text layer)'];
        const row = (p, r) => p.text[r] || '', rev = (p, r) => Array.from(p.cells.attrs.subarray(r * 80, r * 80 + 80)).every(a => a === 0x70);
        const [map, paused, again, help] = s;
        // (The screen alone, as the game made it: the window rows 2-58 below the bar and the header, its footer row 59;
        // the status line its first, the line typed its last)
        if (!/^ Tordan +(?:torch \S+ +)?UP 0 +facing /.test(row(map, 2))) f.push('the status line (row 2): ' + JSON.stringify(row(map, 2)));
        if (!rev(map, 2)) f.push('the status line isn' + Q + 't reversed all the way across');
        if (row(map, 58) !== '>') f.push('the line typed (row 58): ' + JSON.stringify(row(map, 58)));
        if (!map.text.some(l => l.startsWith('The map (you face'))) f.push('no map on the screen');
        if (!map.text.some(l => l.startsWith('> map')) || map.text.some(l => l.startsWith('> mapx'))) f.push('Backspace: the line entered isn' + Q + 't "> map"');
        if (!/PAUSED/.test(row(paused, 2))) f.push('Escape: the status line doesn' + Q + 't say PAUSED: ' + JSON.stringify(row(paused, 2)));
        if (/PAUSED/.test(row(again, 2))) f.push('Escape again: still PAUSED');
        if (again.text.filter(l => l.startsWith('The map (you face')).length < 2) f.push('the cursor key up: the last line ("map") not entered again');
        const h = help.text.join(' ').replace(/ +/g, ' ');
        if (!h.includes('19200 or 115200 baud')) f.push('the help doesn' + Q + 't give the serial port' + Q + 's rates');
        if (!help.text.slice(27, 58).some(l => l.trim())) f.push('the screen alone: nothing below row 26 (the window not 57 rows)');
        // (The map in colour on the screen, slice 4.2's: the character's @ and a floor it sees now each a colour of
        // their own, neither the words' own)
        const fg = (r, c) => map.cells.attrs[r * 80 + c] & 15, tr = map.text.findIndex(l => l.startsWith('The map (you face'));
        const ar = map.text.findIndex((l, r) => r > tr && l.slice(0, 64).includes('@')), ac = ar < 0 ? -1 : map.text[ar].indexOf('@');
        const dc = ar < 0 ? -1 : map.text[ar].slice(0, 64).search(/[.]/);
        if (tr < 0 || ar < 0 || dc < 0) f.push('the map' + Q + 's @ or a floor beside it not found on the screen');
        else if (fg(ar, ac) === fg(tr, 0) || fg(ar, dc) === fg(tr, 0) || fg(ar, ac) === fg(ar, dc))
          f.push('the map not in its colours on the screen: @ ' + fg(ar, ac) + ', a floor seen now ' + fg(ar, dc) + ', the words ' + fg(tr, 0));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0veraplain', what: 'Dingus Quest Zero on the Vera X as the browser page has it with its keyboard on (the user\'s, 2026-10-16: plain dq0 typed at the Vera X\'s keyboard "switched (on its own) back to the serial terminal"): the terminals chosen once as dq0 starts (terminal both, and consctl\'s keyboard screen: the screen alone) and kept through hmchar\'s questions and the game, put back (terminal both) only as it ends; typed at the SMC\'s keyboard, the normal pace',
      meaning: true,
      init: 't_rc', cycles: 2000e6,
      // (SNAP: the screen and the serial port's output so far, as the keyboard's typing comes to it: smc.type's, wrapped)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push({ text: m.vera.text(), out: m.out }) : type(ch);
      },
      get machine() {
        const P = 'Ā', SNAP = 'Ѐ', hm = ['k', '', 'glad', 'sw', 'shield', 'dag', 'climb', '', 'Dorn'];
        return { vera: true, smc: true, input: 'ābind -a /sd/0/games /bin\rāĂdq0 -s 3\r' + P.repeat(60) + 'n\r' + P.repeat(60) +
          SNAP + hm.map(l => l + '\r' + P.repeat(30)).join('') + P.repeat(150) + SNAP + 'look\r' + P.repeat(40) + SNAP + 'quit\r' + P.repeat(40) +
          'ăcat /dev/consctl\rāecho do^ne\r' };
      },
      expect: ['\ndone\n', '\nterminal both\n'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 3) return ['the snapshots: ' + s.length + ' (3 wanted)'];
        const [hm, game, last] = s, at = out.indexOf('dq0 -s 3');
        if (at < 0) return ['no "dq0 -s 3" on the serial port'];
        // (hmchar's questions on the screen alone: none of them on the serial port, as it asks or after)
        if (!hm.text.some(l => l.startsWith('Keep this character'))) f.push('hmchar\'s first question isn\'t on the screen');
        const ser = last.out.slice(at);
        if (/Keep this character|An occupation|Three weapon skills/.test(ser)) f.push('hmchar\'s questions went to the serial port (the terminals both again)');
        if (/UP 0 +facing|> look/.test(ser)) f.push('the game went to the serial port (the terminals both again)');
        // (The game on the screen alone: the window 57 rows, its status line the first, the line typed its last)
        if (!/^ \S+ +(?:torch \S+ +)?UP 0 +facing /.test(game.text[2] || '')) f.push('the status line (row 2): ' + JSON.stringify(game.text[2]));
        if ((game.text[58] || '') !== '>') f.push('the line typed (row 58), the screen alone: ' + JSON.stringify(game.text[58]));
        // (Put back as it ends: the serial port's terminal painted again, consctl's terminal both)
        if (!out.slice(at).includes('\nterminal both\n')) f.push('the terminals not both after the game');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0veraserial', what: 'Dingus Quest Zero on the Vera X with no keyboard there (the browser page\'s defaults: the screen, no SMC), typed on the serial port: plain dq0 (hmchar inside it, the character it makes the one the game plays: dq0\'s, before) stays on both terminals (no consctl keyboard screen: the serial terminal, the only keyboard, isn\'t left blind), hmchar\'s questions and the game on the serial port and the screen, the terminals both as it ends',
      init: 't_rc', cycles: 2000e6,
      // (When the game's first question and hmchar's first came out on the serial port: the first as it was asked, not
      // painted at once with what the window had as the serial port's terminal was turned on again for hmchar)
      start(m) {
        const run = m.run, at = this.at = {};
        m.run = limit => {
          run(limit);
          if (!at.pace && m.out.includes('How fast does the dungeon move')) at.pace = m.cpu.cyc;
          if (!at.hm && m.out.includes('Keep this character')) at.hm = m.cpu.cyc;
        };
      },
      get machine() {
        const P = 'Ā', hm = ['k', '', 'glad', 'sw', 'shield', 'dag', 'climb', '', 'Dorn'];
        return { vera: true, input: 'ābind -a /sd/0/games /bin\rādq0 -s 3\r' + P.repeat(60) + 'n\r' + P.repeat(60) +
          hm.map(l => l + '\r' + P.repeat(30)).join('') + P.repeat(150) + 'look\r' + P.repeat(40) + 'quit\r' + P.repeat(40) +
          'cat /dev/consctl\rāecho do^ne\r' };
      },
      expect: ['\ndone\n', '\nterminal both\n'],
      check(m, out) {
        const f = [], at = out.indexOf('dq0 -s 3'), text = m.vera.text();
        if (at < 0) return ['no "dq0 -s 3" on the serial port'];
        const ser = out.slice(at);
        const a = this.at || {};
        if (!a.pace || !a.hm || a.hm - a.pace < 50e6) f.push('the game\'s first question not on the serial port as it was asked (the screen taken alone): at ' + a.pace + ', hmchar\'s at ' + a.hm);
        if (!ser.includes('Keep this character')) f.push('hmchar\'s questions not on the serial port');
        if (!/UP 0 +facing/.test(ser)) f.push('the game\'s status line not on the serial port');
        if (!/> look/.test(ser)) f.push('the line typed not on the serial port');
        if (!/Dorn, [A-Za-z/]+, enters the dungeon\./.test(ser.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, ' ').replace(/\s+/g, ' '))) f.push('the character hmchar made in the game (Dorn) not the one it plays (dq0\'s, before)');
        if (/keyboard screen/.test(ser)) f.push('consctl said keyboard screen with no keyboard');
        if (!text.some(l => l.startsWith('> look'))) f.push('the game not on the screen too');
        return f;
      },
      sdcard: true
    },
    // ---- The help (GAME-CRITIQUE.md's 5.6; DECISIONS.md's answers of 2026-11-17: "l" stays left, the help's first steps say so)
    {
      name: 'dq0help', near: ['screen'], what: "Dingus Quest Zero's help (help.c's o_help, games/dq0/help.txt; GAME-CRITIQUE.md's 5.6: three lines past help.c's 400-byte buffer were said as two paragraphs, two cut mid-word, \"the w\" \"ell\"): each of help.txt's lines said as one paragraph, whole, wrapped as the story wraps one (term.c's say: a word on the next line only where it won't fit the 80 columns), the long ones read in parts and said on at a space (games/dq0/make.js's helpParts, the build's check of every line); the first steps saying l is left, not look (the user's, 2026-11-17)",
      init: 't_rc', cycles: 500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'help', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], parts = DQ0_MAKE().helpParts(fs.readFileSync(path.join(__dirname, '..', '..', '..', 'games', 'dq0', 'help.txt'), 'latin1'));
        const o = out.replace(/\r/g, ''), at = o.indexOf('> help\n'), end = at < 0 ? -1 : o.indexOf('\x1b', at);
        if (end < 0) return ['no help said'];
        const said = o.slice(at + 7, end).split('\n'), want = [];
        while (said.length && !said[said.length - 1]) said.pop();
        for (const p of parts) {                // (Each paragraph as say wraps it: a word on the next line only where it won't
          let line = '';                        //   fit, its column, a space and it 80 or more; "@own " the ruleset's word and
          for (const w of p.join('').replace(/@own /g, 'counter, ').split(' ').filter(Boolean)) {    // a comma, HarnMaster's)
            if (line && line.length + 1 + w.length >= 80) {
              want.push(line);
              line = '';
            }
            line += (line ? ' ' : '') + w;
          }
          want.push(line);
        }
        const k = want.findIndex((l, i) => said[i] !== l);
        if (k >= 0 || said.length !== want.length) f.push('the help not said a paragraph a line of help.txt, whole, at line ' + (k < 0 ? want.length : k + 1) + ': ' +
          JSON.stringify(said[k < 0 ? want.length : k]) + ' (wanted ' + JSON.stringify(want[k < 0 ? want.length : k]) + ')');
        if (parts.filter(p => p.length > 1).length < 3) f.push('fewer than three of help.txt\'s lines read in parts: none longer than help.c\'s buffer tried');
        if (!/\(f, b, l, r: as you face; l is left, not look\)/.test(said.join(' '))) f.push('the first steps not saying l is left');
        return f;
      },
      sdcard: true
    },
    // ---- The words the player reads (2026-11-17: GAME-CRITIQUE.md's 5.6 and section 1's visible bugs)
    {
      name: 'dq0pitwords', near: ['screen'], outfit: true, what: "Dingus Quest Zero's refusals and measures in words (2026-11-17, the play test's): a thing let fall down a shaft with none near (the world's wwell2.c, IR_NONE: its words none, so the UI said the things' text as it was left, \"drop the candle stubs in the pit\" an inventory before it, cut short), said why, \"There's no well or shaft here for the torch to fall down.\", the inventory before it not said again, nor at the pit; a weapon's look (the soak's cleric's dagger, light: its item's size its version, WV_*, read as its measure, \"1 feet long\") without a measure",
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'i',
        'drop the torch down the shaft', 'i', 'drop the torch into the pit', 'quit', 'hmchar -q -s 1 -o /ram/d.hm', 'dq0 -p t -s 3 -c /ram/d.hm -T', 'examine the dagger', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out), turn = (a, b) => { const i = s.indexOf('> ' + a), j = s.indexOf('> ' + b, i + 1); return i < 0 ? '' : s.slice(i, j < 0 ? undefined : j); };
        const sh = turn('drop the torch down the shaft', 'i'), pt = turn('drop the torch into the pit', 'quit'), dg = turn('examine the dagger', 'quit');
        if (!sh.includes("There's no well or shaft here for the torch to fall down.")) f.push('no shaft near: not said why: ' + JSON.stringify(sh));
        for (const [w, t] of [['down the shaft', sh], ['into the pit', pt]]) if (/You carry|In all,|\(a\) /.test(t)) f.push(w + ': the inventory before it said again: ' + JSON.stringify(t));
        if (!/Your dagger is (?:in|on) your [a-z]+\. It's made of iron[;.]/.test(dg)) f.push('the dagger looked at: ' + JSON.stringify(dg));
        if (/feet long|foot long/.test(dg)) f.push('a weapon\'s version said as its measure: ' + JSON.stringify(dg));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0played', near: ['screen'], outfit: true, fit: true, reach: true, bearings: true, initiative: true, hours: true, what: "Dingus Quest Zero's play test's first game replayed (2026-11-16, GAME-CRITIQUE.md's: a weak scribe, seed 271828's generated dungeon, its 118 lines as typed, the door broken with the lantern named, as the game broke it then): a thing too far to make out told as one, whatever it is (\"On the floor ahead to your right lies (F) something small.\": a plural kind's candle stubs through a window, \"lie\" till 2026-11-17: seen.c's verb by its words); the candle stubs dropped in the pit, the inventory taken between, its words never said again (\"You reach for your backpack.\" and the inventory before it, cut short, till 2026-11-17)",
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 77 -u feudal -c scribe -o /ram/c.hm', 'dq0 -p t -s 271828 -c /ram/c.hm',
        ...PLAYED, 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out), far = s.match(/ (lie|lies) \([A-Z]\) something (small|large)/g) || [];
        if (!far.length) f.push('no thing too far to make out told (the scene moved?)');
        for (const x of far) if (!/ lies /.test(x)) f.push('a thing too far to make out said with a plural verb: ' + JSON.stringify(x));
        const pit = s.slice(s.indexOf('> drop the stubs into the pit'), s.indexOf('> jump into the pit'));
        if (!pit) f.push('the pit not come to (the scene moved?)');
        else if (/You carry|In all,|\(a\) clothes/.test(pit)) f.push('the inventory before it said again at the pit: ' + JSON.stringify(pit.slice(0, 300)));
        return f;
      },
      sdcard: true
    },
  ],
};
