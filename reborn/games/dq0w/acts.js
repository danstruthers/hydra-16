// ****************************************************************************
// acts.js - the world's act table made of games/dq0w/acts.txt (GAME-CRITIQUE.md's E6, ONE-PATH.md's 2.1): the
// rows as data for the paged ROM's library (obj/gen/dq0acts.inc: modules/dq0lib/table.s's AC_ROW reads them; no main
// RAM) and their numbers for the world's C (obj/gen/dq0acts.h: AC_* an act's row, struct act its record, ACT_STEP the
// time's flag for "N step").  The build's (tools/dq0build.js's generate, after the hub's numbers: H, hubgen.js's).
// A row's names (P_*, NZ_*, DE_*, LT_*, KI_*, GA_WORK) are the hub's headers', checked here: a name that isn't one
// fails the build.
//
// Usage: node games/dq0w/acts.js    (its table checked and printed: the build's own call is generate (H, say))
'use strict';
const fs = require('fs');
const path = require('path');

const FILE = path.join(__dirname, 'acts.txt');
const GEN = path.join(__dirname, '..', '..', 'obj', 'gen');
const ACT_SIZE = 12;            // (struct act's bytes: time 2, test, steps, loud, noise, den, fatigue, task, tool, hands, spare)
const ACT_STEP = 0x8000;        // (time's flag: so many of the actor's step)

function read(H) {
  const rows = [];
  fs.readFileSync(FILE, 'latin1').split(/\r?\n/).forEach((l, i) => {
    if (!l.trim() || l.startsWith('#')) return;
    const bad = what => { throw new Error('games/dq0w/acts.txt: line ' + (i + 1) + ': ' + what); };
    const v = l.split('|').map(x => x.trim());
    if (v.length !== 11) bad('11 fields, not ' + v.length);
    const [name, time, test, steps, loud, noise, den, fat, task, tool, hands] = v;
    if (!/^[a-z][a-z0-9]*$/.test(name) || rows.some(r => r.name === name)) bad('a name of its own, lower case: ' + name);
    const C = (pre, x) => {
      if (x === '-') return 0;
      const n = H.C[pre + x];
      if (n === undefined) bad('no ' + pre + x + ' in the hub\'s headers');
      return n;
    };
    let t;
    const m = /^(\d+)( step)?$/.exec(time);
    if (!m) bad('a time: tenths, or "N step": ' + time);
    t = +m[1] | (m[2] ? ACT_STEP : 0);
    if ((t & ~ACT_STEP) > 0x7FFF) bad('a time too long');
    const s = +steps, ld = +loud, h = +hands;
    if (!Number.isInteger(s) || s < -128 || s > 127) bad('steps: -128 to 127');
    if (!Number.isInteger(ld) || ld < 0 || ld > 9) bad('loud: 0 to 9');
    if (![0, 1, 2].includes(h)) bad('hands: 0, 1 or 2');
    rows.push({ name, t, p: test === '-' ? 0xFF : C('P_', test), s: s & 255, loud: ld, nz: noise === '-' ? 0xFF : C('NZ_', noise),
      de: C('DE_', den), ga: fat === '-' ? 0 : C('GA_', fat), lt: C('LT_', task), tool: C('KI_', tool), hands: h });
  });
  if (rows.length > 255) throw new Error('games/dq0w/acts.txt: more than 255 rows');
  return rows;
}

function generate(H, say = () => {}) {
  const rows = read(H);
  fs.mkdirSync(GEN, { recursive: true });
  const inc = ['; dq0acts.inc - made by games/dq0w/acts.js of acts.txt: the act table\'s rows (struct act, ' + ACT_SIZE + ' bytes each)',
    'AC_N        = ' + rows.length, 'AC_SIZE     = ' + ACT_SIZE, 'ac_rows:'];
  for (const r of rows) {
    inc.push('            .word       $' + r.t.toString(16).padStart(4, '0') + '                             ; AC_' + r.name.toUpperCase());
    inc.push('            .byte       ' + [r.p, r.s, r.loud, r.nz, r.de, r.ga, r.lt, r.tool, r.hands, 0].join(', '));
  }
  const h = ['/* dq0acts.h - made by games/dq0w/acts.js of acts.txt: the act table\'s rows by their names, and a row\'s record */',
    '#ifndef _DQ0ACTS_H', '#define _DQ0ACTS_H',
    '#define ACT_STEP        0x8000U                 /* (struct act\'s time: so many of the actor\'s step) */',
    '#define ACT_NONE        0xFF                    /* (Its test, its noise: none) */',
    'struct act {',
    '    unsigned        time;                       /* Tenths, or | ACT_STEP so many of the actor\'s step */',
    '    unsigned char   test;                       /* The referee\'s purpose (P_*), ACT_NONE none */',
    '    signed char     steps;                      /* Its test the easier */',
    '    unsigned char   loud;                       /* Its noise, 0 none */',
    '    unsigned char   noise;                      /* The character\'s E_NOISE\'s what (NZ_*), ACT_NONE none */',
    '    unsigned char   den;                        /* An actor\'s E_DEN\'s what (DE_*), 0 none */',
    '    unsigned char   fatigue;                    /* GA_WORK, or 0 */',
    '    unsigned char   task;                       /* A long task\'s kind (LT_*), 0 done at once */',
    '    unsigned char   tool;                       /* The thing it wants in hand (KI_*), 0 none */',
    '    unsigned char   hands;',
    '    unsigned char   spare;',
    '};',
    'typedef char act_fits[sizeof (struct act) == ' + ACT_SIZE + ' ? 1 : -1];',
    'enum {'];
  rows.forEach((r, i) => h.push('    AC_' + r.name.toUpperCase() + (i === rows.length - 1 ? '' : ',')));
  h.push('};', '#define AC_N            ' + rows.length, '#endif', '');
  write(path.join(GEN, 'dq0acts.inc'), inc.join('\n') + '\n');
  write(path.join(GEN, 'dq0acts.h'), h.join('\n'));
  say('dq0acts: ' + rows.length + ' acts, ' + rows.length * ACT_SIZE + ' bytes in the paged ROM\'s library');
  return rows;
}

function write(f, text) {           // (Written only if it's changed: the build's cache keeps its objects)
  if (!fs.existsSync(f) || fs.readFileSync(f, 'latin1') !== text) fs.writeFileSync(f, text, 'latin1');
}

module.exports = { generate, read, ACT_SIZE, ACT_STEP };

if (require.main === module) {
  const H = require(path.join(__dirname, '..', 'dq0', 'hubgen.js')).load();
  for (const r of read(H)) console.log(r.name.padEnd(10), JSON.stringify(r));
}
