// rom_space.js - the space left on each BIOS ROM page, from the link map: printed by every build, so the pages
// that are filling up (page 0, the kernel's, and the COMMON block every page carries) are seen before they're full.
//
// Usage (from os_rom): node tools/rom_space.js obj/os_rom_C02.map os_rom_C02.cfg [--table]
//   --table   a line per page, with the free pieces (otherwise one line for all of them)
//
// A page's free space is what its segments leave of $E000-$FEFF ($FF00-$FFF9 is the I/O space; the vectors
// are at $FFFA), less COMMON ($FD00-$FDFF), whose room is counted on its own.  A warning when page 0 or COMMON
// is nearly full.
'use strict';
const fs = require('fs');

const [mapFile, cfgFile] = process.argv.slice(2).filter(a => !a.startsWith('--'));
const table = process.argv.includes('--table');
const map = fs.readFileSync(mapFile || 'obj/os_rom_C02.map', 'utf8');
const cfg = fs.readFileSync(cfgFile || 'os_rom_C02.cfg', 'utf8');
const WARN_PAGE0 = 64, WARN_COMMON = 8;
const BASE = 0xE000, TOP = 0xFF00, COMMON = 0xFD00;

const list = map.split('Segment list:')[1].split('Exports list')[0];
const segs = [...list.matchAll(/^(\S+)\s+([0-9A-F]{6})\s+([0-9A-F]{6})\s+([0-9A-F]{6})/gm)]
  .map(m => ({ name: m[1], start: parseInt(m[2], 16), end: parseInt(m[3], 16), size: parseInt(m[4], 16) }));
const memOf = {};
for (const m of cfg.matchAll(/^\s*(\w+):\s*load\s*=\s*(\w+)/gm)) memOf[m[1]] = m[2];

const used = {};                                    // Page -> a byte per address in $E000-$FEFF: in use?
for (const s of segs) {
  const m = /^OS_ROM_P([0-9A-F])$/.exec(memOf[s.name] || '');
  if (!m || s.size === 0) continue;
  const u = used[m[1]] = used[m[1]] || new Uint8Array(TOP - BASE);
  for (let a = s.start; a <= s.end && a < TOP; a++) u[a - BASE] = 1;
}
const hex = n => '$' + n.toString(16).toUpperCase();
let common = 0x100;
const rows = Object.keys(used).sort().map(p => {
  const u = used[p], gaps = [];
  for (let a = BASE; a < TOP;) {
    if (u[a - BASE] || (a >= COMMON && a < COMMON + 0x100)) { a++; continue; }
    const from = a;
    while (a < TOP && !u[a - BASE] && !(a >= COMMON && a < COMMON + 0x100)) a++;
    gaps.push([from, a - from]);
  }
  let c = 0;
  for (let a = COMMON; a < COMMON + 0x100; a++) if (!u[a - BASE]) c++;
  common = Math.min(common, c);
  return { page: p, free: gaps.reduce((n, [, len]) => n + len, 0), gaps };
});
if (table) {
  for (const r of rows) console.log('page ' + r.page + ': ' + String(r.free).padStart(5) + ' free  ' + r.gaps.map(([a, n]) => n + ' at ' + hex(a)).join(', '));
  console.log('COMMON: ' + common + ' free');
} else {
  console.log('ROM space free (bytes): ' + rows.map(r => r.page + ':' + r.free).join(' ') + ', COMMON:' + common + '  (tools/rom_space.js --table: where)');
}
const p0 = rows.find(r => r.page === '0');
if (p0 && p0.free < WARN_PAGE0) console.log('*** WARNING: page 0 (the kernel) has ' + p0.free + ' bytes left: put new code on another page');
if (common < WARN_COMMON) console.log('*** WARNING: COMMON has ' + common + ' bytes left (every page carries it)');
