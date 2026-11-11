// ****************************************************************************
// screen.js - Dingus Quest Zero's tests: the screens: the Vera X, the prompt, the answer's time.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const { typed } = require('../../lib/common.js');
const { dq0StatusRows, dq0Typed } = require('../../lib/dq0.js');

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
  ],
};
