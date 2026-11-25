// ****************************************************************************
// hmchar.js - hmchar's tests (the HarnMaster character maker).  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

// The hmchar tests' checks (hmcharhl, hmcharfs: games/hmchar's hylang and HyForth generators, C's hmchar's
// companions): the last profile printed, and the file written (harnfile.c's lines), each as it should be and the two
// agreeing (the skills' names, keys and which are automatic from harn.hl's Skill Data table, a line each): a list of
// failures
const HM_SKILLS = fs.readFileSync(path.join(TESTS, '..', 'romfs', 'lib', 'hylang', 'harn.hl'), 'latin1').split(/\r?\n/)
  .filter(l => l.length === 41 && /^[a-z]{3,4} +[A-Z]/.test(l) && l[27] === ' ' && l[36] === ' ')
  .map(l => ({ key: l.slice(0, 5).trim(), name: l.slice(5, 24).trim(), auto: l[38] === '1' }));
const HM_NAMES = {
  sp: ['Human', 'Sindarin', 'Khuzdul', 'Other'], sx: ['Female', 'Male'],
  month: ['Nuzyael', 'Peonu', 'Kelen', 'Nolus', 'Larane', 'Agrazhar', 'Azura', 'Halane', 'Savor', 'Ilvin', 'Navek', 'Morgat'],
  culture: ['Tribal', 'Viking', 'Feudal', 'Imperial', 'Khuzan', 'Sindarin'], social: ['Slave', 'Serf', 'Unguilded', 'Guilded', 'Noble'],
  parent: ['Offspring', 'Fostered', 'Adopted', 'Bastard', 'Orphan'], estrange: ['Outcast', 'Unpopular', 'Average', 'Popular', 'Favorite'],
  clanhead: ['Distant Relation', 'Aunt or Uncle', 'Cousin', 'Father or Mother'], fr: ['Scant', 'Light', 'Medium', 'Heavy', 'Massive'],
  complexion: ['Fair', 'Medium', 'Dark'], hair: ['Brown', 'Black', 'Red', 'Silver', 'Blond'], eyes: ['Hazel', 'Gray', 'Violet', 'Green', 'Blue'],
};
const HM_DEITIES = ['Agrik', 'Halea', 'Ilvir', 'Larani', 'Morgath', 'Naveh', 'Peoni', 'Sarajin', 'Save-K' + "'" + 'nor', 'Siem'];
const HM_ATTRS = ['sp', 'sx', 'bd', 'ss', 'ht', 'fr', 'owt', 'wt', 'cml', 'str', 'sta', 'dex', 'agl', 'eye', 'hrg', 'sml', 'voi',
  'int', 'aur', 'wil', 'mor', 'dty', 'pty'];
const HM_OCCS = fs.readFileSync(path.join(TESTS, '..', 'games', 'hmchar', 'occupations.txt'), 'latin1').split(/\r?\n/)
  .filter(l => /^[^;[\s][^|]*\|/.test(l)).map(l => l.split('|')[0].trim()).filter(n => n !== 'all');
function hmcharCheck(out, file) {
  const f = [], end = out.lastIndexOf('\nWritten to ');
  if (HM_SKILLS.length !== 91) return ['harn.hl\'s Skill Data table: ' + HM_SKILLS.length + ' skills read, not 91'];
  if (end < 0) return ['no "Written to"'];
  const lines = out.slice(0, end).split('\n'), at = lines.length - 1 - [...lines].reverse().findIndex(l => / born \d+ /.test(l));
  const p = lines.slice(at), fl = file.split('\n').filter(l => l);
  const m1 = p[0].match(/^(.+), (\w+) (\w+), born (\d+) (\w+) \((.+)\); (\w+) (\w+), (.+)$/);
  const m2 = (p[1] || '').match(/^(\d+) of (\d+); (\w+), (\w+); clanhead: (.+)$/);
  const m3 = (p[2] || '').match(/^(\d+)", (\w+), (\d+) lb \(size (\d+)\); (\w+) (\d+); (\w+), (\w+) hair, (\w+) eyes$/);
  const m6 = (p[5] || '').match(/^Endurance (\d+), Move (\d+), Piety (\d+)(?: \((.+)\))?; medical:(.*)$/);
  if (!m1 || !m2 || !m3 || !m6) return ['the profile isn\'t as hmchar prints it: ' + JSON.stringify(p.slice(0, 7))];
  // The file: its lines in harnfile.c's order
  const a = {};
  if (fl[0] !== 'harn 1') f.push('the file\'s first line: ' + fl[0]);
  HM_ATTRS.forEach((k, i) => { const m = (fl[1 + i] || '').match(/^att (\w+) (\d+)$/); if (!m || m[1] !== k) f.push('att ' + k + ': ' + fl[1 + i]); else a[k] = +m[2]; });
  const nums = (l, key, n) => { const w = (l || '').split(' '); if (w[0] !== key || w.length - 1 < n) { f.push(key + ': ' + l); return []; } return w.slice(1).map(Number); };
  const fam = nums(fl[24], 'fam', 7), look = nums(fl[25], 'look', 3), med = nums(fl[26], 'med', 0);
  let k = 27;
  while (/^psy /.test(fl[k] || '')) k++;
  if (fl[k++] !== 'sexuality 0') f.push('sexuality: ' + fl[k - 1]);
  const ml = new Map();
  while (/^ml /.test(fl[k] || '')) { const w = fl[k++].split(' '); ml.set(w[1], +w[2]); }
  if (fl[k++] !== 'vol 0 0 0 0 0') f.push('vol: ' + fl[k - 1]);
  if (fl[k++] !== 'name ' + m1[1]) f.push('name: ' + fl[k - 1] + ', the profile\'s ' + m1[1]);
  if (fl[k++] !== 'occupation ' + m1[9]) f.push('occupation: ' + fl[k - 1] + ', the profile\'s ' + m1[9]);
  if (/^wealth /.test(fl[k] || '')) {                   // (C's hmchar's: the family's wealth, Character 17's by its class)
    const w = +fl[k++].slice(7), cls = [[40, 40, 40], [40, 80, 160], [60, 120, 240], [90, 180, 360], [600, 1200, 2400]][fam[1]] || [];
    if (!cls.includes(w)) f.push('wealth: ' + w + ', not the book\'s for its class (' + cls.join(', ') + ')');
  }
  const tongues = [], scripts = [];                     // (Its timesense, its tongues and scripts: the plan's step 8, hmchar's last)
  if (fl[k] === 'timesense') k++;
  for (let m; (m = /^language ([a-z]+)=(\d+)$/.exec(fl[k] || '')); k++) tongues.push(m[1] + ' ' + m[2]);
  for (let m; (m = /^script ([a-z]+)(?:=(\d+))?$/.exec(fl[k] || '')); k++) scripts.push(m[1] + ' ' + (m[2] || ''));
  if (fl.length !== k) f.push('the file\'s lines after occupation: ' + JSON.stringify(fl.slice(k)));
  if (f.length) return f;
  // The profile agreeing with it
  const eq = (what, x, y) => { if (x !== y) f.push(what + ': ' + x + ' printed, ' + y + ' in the file'); };
  eq('species', m1[2], HM_NAMES.sp[a.sp]); eq('sex', m1[3], HM_NAMES.sx[a.sx]);
  eq('birthday', +m1[4], a.bd & 255); eq('month', m1[5], HM_NAMES.month[(a.bd >> 8) - 1]);
  eq('culture', m1[7], HM_NAMES.culture[fam[0]]); eq('class', m1[8], HM_NAMES.social[fam[1]]);
  if (!HM_OCCS.includes(m1[9])) f.push('occupation ' + m1[9]);
  eq('sibling rank', +m2[1], fam[2]); eq('family', +m2[2], fam[3]); eq('parent', m2[3], HM_NAMES.parent[fam[4]]);
  eq('estrangement', m2[4], HM_NAMES.estrange[fam[5]]); eq('clanhead', m2[5], HM_NAMES.clanhead[fam[6]]);
  eq('comeliness', m3[5], ['Ugly', 'Plain', 'Average', 'Attractive', 'Handsome'][a.cml <= 5 ? 0 : a.cml <= 8 ? 1 : a.cml <= 12 ? 2 : a.cml <= 15 ? 3 : 4]);
  eq('height', +m3[1], a.ht); eq('frame', m3[2], HM_NAMES.fr[a.fr]); eq('weight', +m3[3], a.wt); eq('comeliness', +m3[6], a.cml);
  eq('complexion', m3[7], HM_NAMES.complexion[look[0]]); eq('hair', m3[8], HM_NAMES.hair[look[1]]); eq('eyes', m3[9], HM_NAMES.eyes[look[2]]);
  eq('physical', p[3], `Strength ${a.str}, Stamina ${a.sta}, Dexterity ${a.dex}, Agility ${a.agl}, Eyesight ${a.eye}, Hearing ${a.hrg}, Smell ${a.sml}, Voice ${a.voi}`);
  eq('personality', p[4], `Intelligence ${a.int}, Aura ${a.aur}, Will ${a.wil}, Morality ${a.mor}`);
  eq('endurance', +m6[1], Math.floor(((a.str + a.sta + a.wil) * 2 + 3) / 6)); eq('move', +m6[2], a.agl); eq('piety', +m6[3], a.pty);
  const church = HM_DEITIES.findIndex(d => m1[9] === 'Cleric (' + d + ')');     // (A cleric's deity its church's; else the
  if (church >= 0 ? a.dty !== church : !(a.dty <= 10)) f.push('a deity: ' + a.dty + ', ' + m1[9]);   //   player's, or none)
  eq('deity', m6[4] || 'none', a.dty === 10 ? 'none' : HM_DEITIES[a.dty]);
  if (a.dty < 10 && !ml.has(HM_SKILLS[21 + a.dty].key)) f.push('the deity\'s ritual not open: ' + HM_SKILLS[21 + a.dty].name);
  eq('medical traits', m6[5] === ' none' ? 0 : m6[5].trim().split(' ').length, med.length);
  const sk = (p.slice(6).join('\n').match(/^Skills \(ML\):([^]*?)(?:\nTongues: |$)/) || [])[1];
  if (!sk) return [...f, 'no skills line'];
  const said = (p.slice(6).join('\n').match(/\nTongues: common (\d+)((?:, [a-z]+ \d+)*)\. Scripts: (none|[a-z]+ \d+(?:, [a-z]+ \d+)*)\.(?:\n|$)/) || []);
  if (!said[0]) f.push('no tongues and scripts said: ' + JSON.stringify(p.slice(6)));
  else {
    if (+said[1] !== ml.get('lan')) f.push('its own tongue said at ' + said[1] + ', Language ' + ml.get('lan'));
    if (said[2].slice(2) !== tongues.join(', ')) f.push('its tongues said, ' + said[2] + ', not the file\'s: ' + tongues);
    const sc = scripts.map((s, i) => i ? s : s + ml.get('scr'));
    if ((said[3] === 'none' ? '' : said[3]) !== sc.join(', ')) f.push('its scripts said, ' + said[3] + ', not the file\'s: ' + sc);
    if (!ml.get('scr') !== !scripts.length) f.push('Script open ' + ml.get('scr') + ', its scripts ' + scripts);
  }
  const shown = new Map([...sk.matchAll(/ ([A-Z][A-Za-z':\- ]*?) (\d+)(?= |\n|$)/g)].map(m => [m[1].trim(), +m[2]]));
  for (const s of HM_SKILLS) {
    if (s.auto && !ml.has(s.key)) f.push('an automatic skill not open: ' + s.name);
    if (ml.get(s.key) !== shown.get(s.name)) f.push(s.name + ': ' + shown.get(s.name) + ' printed, ' + ml.get(s.key) + ' in the file');
  }
  if (ml.size !== shown.size) f.push(ml.size + ' skills in the file, ' + shown.size + ' printed');
  return f;
}

// The hmcharhl test's runs: hmchar.hl -q, its file; then as a player answers: a roll again, kept; a deity (a beginning
// that isn't enough, one there isn't, then Peoni); an occupation out of
// range, the Gladiator's skills shown (? and a beginning of its name), then it chosen so; weapons (a beginning, one that isn't enough, one twice); option points (Climbing three times: the second and third refused, two points a skill at most,
// a skill there isn't, Enter); a name; its file
const HM_PLAY = 'r\rk\rs\rxyz\rpeo\ru\r99\r?glad\rglad\rsw\rs\rshield\rshield\rdag\rclimb\rclimb\rclimb\rzzz\r\rDorn\r';
const HM_DEITY_SAID = ['Which (a beginning enough; Enter alone, none)? s\nWhich? More than one begins so.\n',
  'Which (a beginning enough; Enter alone, none)? xyz\nThere\'s no such deity.\n', 'Which (a beginning enough; Enter alone, none)? peo\nRitual: Peoni is '];
// The hmcharfs test's runs as a player answers, each C's and HyForth's the same (the book's limits, the user's,
// 2026-10-20): seed 64, a Feudal noblewoman: the nobles listed (the Herald, the Lady), the Lady's skills shown, then
// her chosen; seed 91, a Feudal nobleman: the nobles listed (the Herald alone), no Lady, a knight a beginning that
// isn't enough, then the Knight MH; seed 5, a Feudal serf (Tordan): no nobles, the Herald and the Lady not his, a
// beginning (g) that's the unguilded Gladiator's and the soldiers' guardsmen, no knight, the Clothier not his (a
// guilded occupation), then a farmer
const HM_LIMITS = [
  ['64', 'k\r\rn\r?lady\rlady\r\rAyla\r', 'Lady'],
  ['91', 'k\r\rn\rlady\rknight\rknight mh\r\rBran\r', 'Knight MH'],
  ['5', 'k\r\rn\rherald\rlady\rg\rknight\rclothier\rfarmer\r\rTor\r', 'Farmer'],
];
const HM_FILE = f => 'echo ==file; cat ' + f + '; echo ==end';
// A run's part of the output (from its command line), and the file it cat'd (between ==file and ==end)
function hmRun(out, cmd) {
  const a = out.indexOf('% ' + cmd + '\n'), b = out.indexOf('\n==file\n', a), e = out.indexOf('==end\n', b);
  return a < 0 || b < 0 || e < 0 ? null : { out: out.slice(a, b + 1), file: out.slice(b + 8, e) };
}

module.exports = {
  tests: [
    {
      name: 'hmchartongues', what: "hmchar's tongues and scripts (the plan for the game between dungeons, its step 8; occupations.txt's tongues N and scripts N) from its three makers: a Feudal scribe (seed 5) from C's hmchar and hmchar.fs the same, its profile and file as hmcharCheck has them (a 2nd Language, the native script ardic and two more, none twice); a Viking lexigrapher from both the same (two scripts, the native delvic, no second tongue); hmchar.hl's scribe (its dice its own, seeded: -s 5, random-seed's, the same scribe every run) a language line and three scripts, the native ardic, none twice",
      init: 't_rc', cycles: 1600e6,
      machine: { input: 'ābind -a /sd/0/games /bin\r' + 'ācd /ram; hmchar -q -s 5 -u feudal -c scribe -o x.hm >s.out; cp x.hm s.hm; hmchar.fs -q -s 5 -u feudal -c scribe -o x.hm >f.out\r' +
        'ācmp s.out f.out && cmp s.hm x.hm && echo scribe same; cat s.out; ' + HM_FILE('s.hm') + '\r' +
        'āhmchar -q -s 7 -u viking -c lexi -o x.hm >l.out; cp x.hm l.hm; hmchar.fs -q -s 7 -u viking -c lexi -o x.hm >g.out\r' +
        'ācmp l.out g.out && cmp l.hm x.hm && echo lexigrapher same; cat l.out; ' + HM_FILE('l.hm') + '\r' +
        'āhmchar.hl -q -s 5 -u feudal -c scribe -o h.hm >h.out; cat h.out; ' + HM_FILE('h.hm') + '\r' + 'āecho =DONE\r' },
      expect: ['\n=DONE\n'],
      check(m, out) {
        const f = [], o = out.replace(/\r/g, '');
        const c1 = 'cmp s.out f.out && cmp s.hm x.hm && echo scribe same; cat s.out; ' + HM_FILE('s.hm'), c2 = 'cmp l.out g.out && cmp l.hm x.hm && echo lexigrapher same; cat l.out; ' + HM_FILE('l.hm'), c3 = 'hmchar.hl -q -s 5 -u feudal -c scribe -o h.hm >h.out; cat h.out; ' + HM_FILE('h.hm');
        const sc = s => [...s.matchAll(/\nscript ([a-z]+)(?:=\d+)?(?=\n)/g)].map(x => x[1]), lg = s => [...s.matchAll(/\nlanguage [a-z]+=\d+(?=\n)/g)].length;
        if (!/\nscribe same\n/.test(o)) f.push('hmchar.fs\'s scribe not C\'s');
        if (!/\nlexigrapher same\n/.test(o)) f.push('hmchar.fs\'s lexigrapher not C\'s');
        for (const [n, c, nl, ns, nat] of [['the scribe', c1, 1, 3, 'ardic'], ['the lexigrapher', c2, 0, 2, 'delvic'], ['hmchar.hl\'s scribe', c3, 1, 3, 'ardic']]) {
          const p = hmRun(o, c);
          if (!p) { f.push(n + ': no file'); continue; }
          const x = sc('\n' + p.file);
          // (hmchar.hl's scripts in its draw's order: the native among them, 2026-11-10, the images' change drew "starric" first;
          // its dice unseeded changed with every image, 2026-11-21's a Khuzdul scribe: -s 5 since)
          if (lg('\n' + p.file) !== nl || x.length !== ns || (c === c3 ? !x.includes(nat) : x[0] !== nat) || new Set(x).size !== ns) f.push(n + '\'s tongues and scripts: ' + JSON.stringify(p.file.split('\n').filter(z => /^(language|script) /.test(z))));
          f.push(...hmcharCheck(p.out, p.file).map(z => n + ': ' + z));
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'hmcharhl', what: 'hmchar.hl (games/hmchar: hmchar in hylang, harn.hl in the ROM disk\'s /lib/hylang) at rc: -q, its profile and its file (harnfile.c\'s lines) agreeing, the automatic skills open; then as a player answers (a roll again, a deity, Peoni, chosen after a beginning that isn\'t enough and one there isn\'t, its ritual opened; an occupation out of range, weapons by a beginning of their names, one ambiguous, one twice, an option point\'s second improvement and a third, a skill there isn\'t), the skills it says as they should be; -s with a seed that isn\'t a whole number above 0 refused; harntest.hl\'s tests of the character it read back; -p elf, a Sindarin (Character 2: not rolled), its profile and file as they should be; -u viking, a human of the Viking culture (not rolled: the user\'s, 2026-10-20)',
      init: 't_rc', cycles: 1400e6,
      machine: { input: 'ābind -a /sd/0/games /bin\r' + 'ācd /ram; hmchar.hl -q -o q.hm; ' + HM_FILE('q.hm') + '\r' + 'āhmchar.hl\r' + HM_PLAY + 'ā' + HM_FILE('Dorn.hm') + '\r' +
        'ācd /ram; hmchar.hl -q -p elf -o e.hm; ' + HM_FILE('e.hm') + '\r' + 'ācd /ram; hmchar.hl -q -p h -u viking -o v.hm; ' + HM_FILE('v.hm') + '\r' +
        'āhmchar.hl -s x; echo $status\r' + 'āecho \'(do (use "harntest") (def {c} (harn.read "Dorn.hm")) (list (c :other) (harn.eml c 1)))\' | hylang\r' },
      expect: ['Written to q.hm\n', ...HM_DEITY_SAID, 'Which? 99\nThere\'s no such occupation.\nWhich? ?glad\nGladiator, 4 years: Initiative/6, weapons 3/2, Unarmed/6, Physician/2\nWhich? glad\n', 'The second: s\nWhich? More than one begins so.\n',
        'The third: shield\nThat one\'s chosen already.\n', 'Points left: 4. A skill: climb\nNo more than two points in a skill: that one\'s had its share.\n',
        'A skill: zzz\nThere\'s no such skill.\n', 'Written to Dorn.hm\n',
        'usage: hmchar.hl [-o file] [-q] [-s seed] [-p species] [-c occupation] [-u culture]\n1\n%',
        '=> {{"name Dorn" "occupation Gladiator" "wealth ', 'hylang> => exit'],
      check(m, out) {
        const f = [], q = hmRun(out, 'cd /ram; hmchar.hl -q -o q.hm; ' + HM_FILE('q.hm')), d = hmRun(out, HM_FILE('Dorn.hm'));
        if (!q) return ['-q: its output or its file not found'];
        f.push(...hmcharCheck(q.out, q.file).map(e => '-q: ' + e));
        const play = out.slice(out.indexOf('% hmchar.hl\n'), out.indexOf('% ' + HM_FILE('Dorn.hm')));
        if (!d) return [...f, 'Dorn.hm not found'];
        f.push(...hmcharCheck(play, d.file).map(e => 'played: ' + e));
        // (Climbing: the kept roll's, then improved by its SB once, the second and third refused: two points a skill
        // at most; the Gladiator's weapons; the eml in harntest's test the file's Climbing; Peoni the deity, its ritual
        // open)
        const kept = play.slice(0, play.indexOf('roll again (k or r)? k')), c0 = +((kept.match(/Climbing (\d+)/g) || []).pop() || ' 0').split(' ')[1];
        const c = [...play.matchAll(/Climbing is (\d+)/g)].map(m => +m[1]);
        if (c.length !== 1 || c[0] - c0 <= 0) f.push('Climbing: ' + c0 + ', then ' + c.join(', ') + ': not improved by its SB once');
        if (!/occupation Gladiator\n/.test(d.file) || !['ml dag ', 'ml shd ', 'ml swd '].every(k => d.file.includes(k))) f.push('Dorn.hm: not a Gladiator with Dagger, Shield and Sword');
        if (!/^att dty 6$/m.test(d.file) || !/^ml rpeo \d+$/m.test(d.file) || !/, Piety \d+ \(Peoni\);/.test(play)) f.push('Dorn.hm: Peoni not the deity, or Ritual: Peoni not open');
        const eml = out.match(/=> \{\{"name Dorn" "occupation Gladiator" "wealth \d+"\} (\d+)\}/);
        if (!eml || +eml[1] !== c[0]) f.push('harntest.hl\'s eml of Climbing: ' + (eml && eml[1]) + ', not ' + c[0]);
        const e = hmRun(out, 'cd /ram; hmchar.hl -q -p elf -o e.hm; ' + HM_FILE('e.hm'));
        if (!e) return [...f, '-p elf: its output or its file not found'];
        f.push(...hmcharCheck(e.out, e.file).map(e => '-p elf: ' + e));
        if (!/, Sindarin (Male|Female), .*; Sindarin /.test(e.out) || !/^att sp 1$/m.test(e.file)) f.push('-p elf: not a Sindarin of the Sindarin culture');
        const v = hmRun(out, 'cd /ram; hmchar.hl -q -p h -u viking -o v.hm; ' + HM_FILE('v.hm'));
        if (!v) return [...f, '-u viking: its output or its file not found'];
        f.push(...hmcharCheck(v.out, v.file).map(e => '-u viking: ' + e));
        if (!/, Human (Male|Female), .*; Viking /.test(v.out) || !/^fam 1 /m.test(v.file)) f.push('-u viking: not a human of the Viking culture');
        this.notes = ['-q: ' + q.out.split('\n')[2]];
        return f;
      },
      sdcard: true
    },
    {
      name: 'hmcharfs', what: 'hmchar.fs (games/hmchar: hmchar in HyForth, harn.fs in the ROM disk\'s /lib/forth) at rc, against C\'s hmchar: -q from a seed, its profile and its file the same as C\'s from it (the dice in the same order, the culture rolled too), the two agreeing and as they should be; then the same seed answered as a player would (a roll again, a deity chosen, an occupation out of range, weapons by a beginning, one ambiguous, one twice, an option point\'s improvements, a skill there isn\'t), its questions, profile and file the same as C\'s; a bad option; harn.fs\'s harn-read of the file, harn-eml and harn-end; a dwarf (-p: a species not rolled, Character 2), the same from C and HyForth, a Khuzdul of the Khuzan culture, its file as it should be; -p refused a species there isn\'t; -u viking (the culture not rolled), the same from both; the occupations by class and sex (the user\'s, 2026-10-20: the book\'s limits), a noblewoman\'s, a nobleman\'s and a serf\'s, the same from both: the Lady hers alone, the nobles and the guilded not the serf\'s, nor a knight',
      init: 't_rc', cycles: 2700e6,
      machine: { input: 'ābind -a /sd/0/games /bin\r' + 'ācd /ram; hmchar.fs -q -s 5 -o x.hm >f.out; cp x.hm f.hm\r' +
        'āhmchar -q -s 5 -o x.hm >c.out; cmp f.out c.out && cmp f.hm x.hm && echo same\r' +
        'ācat f.out; ' + HM_FILE('f.hm') + '\r' +
        'āhmchar.fs -s 5 -o x.hm >fi.out\r' + HM_PLAY + 'ācp x.hm fi.hm\r' +
        'āhmchar -s 5 -o x.hm >ci.out\r' + HM_PLAY + 'ācmp fi.out ci.out && cmp fi.hm x.hm && echo the same\r' +
        'ācat fi.out; ' + HM_FILE('fi.hm') + '\r' + 'āhmchar.fs -s; echo $status\r' +
        'āecho \'require harn.fs s" fi.hm" r/o open-file throw harn-read . 1 harn-eml . harn-end .\' | forth\r' +
        'āhmchar.fs -q -s 9 -p dwarf -o x.hm >fd.out; cp x.hm fd.hm\r' +
        'āhmchar -q -s 9 -p k -o x.hm >cd.out; cmp fd.out cd.out && cmp fd.hm x.hm && echo the dwarf the same\r' +
        'ācat cd.out; ' + HM_FILE('fd.hm') + '\r' + 'āhmchar -p z; echo $status\r' +
        'āhmchar.fs -q -s 9 -u vik -o x.hm >fu.out; cp x.hm fu.hm\r' +
        'āhmchar -q -s 9 -u viking -o x.hm >cu.out; cmp fu.out cu.out && cmp fu.hm x.hm && echo the viking the same\r' +
        'ācat cu.out; ' + HM_FILE('fu.hm') + '\r' + 'āhmchar.fs -u zz; echo $status\r' +
        HM_LIMITS.map(([s, a]) => 'āhmchar.fs -s ' + s + ' -u feudal -o x.hm >f' + s + '.out\r' + a + 'ācp x.hm f' + s + '.hm\r' +
          'āhmchar -s ' + s + ' -u feudal -o x.hm >c' + s + '.out\r' + a + 'ācmp f' + s + '.out c' + s + '.out && cmp f' + s + '.hm x.hm && echo seed ' + s + ' the same\r' +
          'ācat c' + s + '.out; ' + HM_FILE('f' + s + '.hm') + '\r').join('') +
        'āecho end of test\r' },
      expect: ['\nsame\n%', '\nthe same\n%', 'usage: hmchar.fs [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]\n1\n%', '\nthe dwarf the same\n%',
        'usage: hmchar [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]\n1\n%', '\nthe viking the same\n%',
        '% hmchar.fs -u zz; echo $status\nusage: hmchar.fs [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]\n1\n%',
        ...HM_LIMITS.map(([s]) => '\nseed ' + s + ' the same\n%'), '% echo end of test\nend of test\n'],
      check(m, out) {
        const f = [], q = hmRun(out, 'cat f.out; ' + HM_FILE('f.hm')), d = hmRun(out, 'cat fi.out; ' + HM_FILE('fi.hm'));
        if (!q || !d) return ['the runs\' output or files not found'];
        f.push(...hmcharCheck(q.out, q.file).map(e => '-q: ' + e), ...hmcharCheck(d.out, d.file).map(e => 'played: ' + e));
        for (const t of ['Which? More than one begins so.', 'That one\'s chosen already.', 'No more than two points in a skill: that one\'s had its share.', 'There\'s no such skill.'])
          if (!d.out.includes(t)) f.push('played: "' + t + '" not said');
        const DQ = 'Which (a beginning enough; Enter alone, none)? ';     // (Its answers not in its output, a file)
        if (!d.out.includes(DQ + 'Which? More than one begins so.\n' + DQ + 'There\'s no such deity.\n' + DQ + 'Ritual: Peoni is ')) f.push('played: the questions of the deity not as they should be');
        if (!/occupation Gladiator\n/.test(d.file) || !['ml dag ', 'ml shd ', 'ml swd '].every(k => d.file.includes(k))) f.push('fi.hm: not a Gladiator with Dagger, Shield and Sword');
        if (!/^att dty 6$/m.test(d.file) || !/^ml rpeo \d+$/m.test(d.file)) f.push('fi.hm: Peoni not the deity, or Ritual: Peoni not open');
        // (harn-read's ior, then Climbing's EML: its ML, no penalty; Endurance: the profile's)
        const r = out.match(/\| forth\n0 (\d+) (\d+) /), cli = d.file.match(/^ml cli (\d+)$/m), end = d.out.match(/.*Endurance (\d+),/s);
        if (!r || !cli || !end || r[1] !== cli[1] || r[2] !== end[1]) f.push('harn-read, harn-eml, harn-end: ' + (r ? r.slice(1).join(' ') : 'nothing') + ', not ' + (cli && cli[1]) + ' ' + (end && end[1]));
        const w = hmRun(out, 'cat cd.out; ' + HM_FILE('fd.hm'));
        if (!w) return [...f, '-p dwarf: its output or its file not found'];
        f.push(...hmcharCheck(w.out, w.file).map(e => '-p dwarf: ' + e));
        if (!/, Khuzdul (Male|Female), .*; Khuzan /.test(w.out) || !/^att sp 2$/m.test(w.file)) f.push('-p dwarf: not a Khuzdul of the Khuzan culture');
        const u = hmRun(out, 'cat cu.out; ' + HM_FILE('fu.hm'));
        if (!u) return [...f, '-u viking: its output or its file not found'];
        f.push(...hmcharCheck(u.out, u.file).map(e => '-u viking: ' + e));
        if (!/, Human (Male|Female), .*; Viking /.test(u.out) || !/^fam 1 /m.test(u.file)) f.push('-u viking: not a human of the Viking culture');
        // (The limits: each run's menu, its answers' replies, the occupation taken)
        const W = 'Which? ', NO = W + 'There\'s no such occupation.\n', TWO = W + 'Which? More than one begins so.\n';
        const NOBLES = W + 'Noble (Character 15; a knight is a soldier):\n';
        for (const [s, , occ] of HM_LIMITS) {
          const l = hmRun(out, 'cat c' + s + '.out; ' + HM_FILE('f' + s + '.hm'));
          if (!l) { f.push('seed ' + s + ': its output or its file not found'); continue; }
          f.push(...hmcharCheck(l.out, l.file).map(e => 'seed ' + s + ': ' + e));
          if (!l.file.includes('\noccupation ' + occ + '\n')) f.push('seed ' + s + ': not a ' + occ);
          const noble = /^  noble \d+/m.test(l.out), guilded = /^  guilded \d+/m.test(l.out), at = l.out.indexOf(NOBLES), list = l.out.slice(at, l.out.indexOf('\n' + W, at) + 1);
          if (s === '64' && !(noble && guilded && /^  \d+ Herald +\d+ Lady\n$/m.test(list) && l.out.includes(W + 'Lady, 7 years:') &&
            /, Human Female, .*; Feudal Noble, Lady\n/.test(l.out))) f.push('seed 64: a noblewoman not offered the nobles, the guilded and the Lady');
          if (s === '91' && !(noble && guilded && /^  \d+ Herald\n$/m.test(list) && l.out.includes(list + NO + TWO + W + 'Every soldier:') && l.out.includes('Knight MH, 7 years:') &&
            /, Human Male, .*; Feudal Noble, Knight MH\n/.test(l.out))) f.push('seed 91: a nobleman not offered the nobles and a knight, or offered the Lady');
          if (s === '5' && !(!noble && !guilded && at < 0 && l.out.includes(NO + NO + NO + TWO + NO + NO + W + 'Farmer, 4 years:') &&
            /, Human Male, .*; Feudal Serf, Farmer\n/.test(l.out))) f.push('seed 5: a serf offered a noble\'s, a guilded or a knight\'s occupation');
        }
        this.notes = ['-s 5 -q: ' + q.out.split('\n')[2], '-s 9 -p dwarf: ' + w.out.split('\n')[2], '-s 9 -u viking: ' + u.out.split('\n')[2]];
        return f;
      },
      sdcard: true
    },
  ],
};
