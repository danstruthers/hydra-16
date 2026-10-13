#!/usr/bin/env node
// joints.js - how much soldering a board is: the pads of each part in a layout file (.kicad_pcb), counted, and
// summed by building step (Chapter 4's steps, as figures.sh colours them).  A through-hole pad is a joint; so is a
// surface-mount pad.  Holes with no copper (mounting holes) aren't counted.
//
// Usage: node manual/tools/joints.js PCB [--steps] [--parts]
//   --steps   the main board's seven steps (the default for hydra-16.kicad_pcb)
//   --parts   each part's count too
'use strict';
const fs = require('fs');

// Chapter 4's steps: the parts of each (as figures.sh has them)
const STEPS = [
  ['1 The shared RAM chips (surface-mount)', 'U25,U27-U29'],
  ['2 The resistors and resistor networks', 'R1-R7,R13,R19-R33,RN1-RN4'],
  ['3 The ceramic capacitors', 'C1-C50,C52,C57-C85'],
  ['4 The chip sockets', 'U1-U24,U26,U30-U42,U44-U61,IC1-IC9,J28'],
  ['5 The crystal, the reset chip, the pin headers and sockets', 'Y1,U43,J5-J10,J27,J29,J30,J1,J2,J18-J25'],
  ['6 The electrolytic capacitors', 'C51,C53-C56,C86-C93'],
  ['7 The connectors', 'J3,J11,J12-J17,J26'],
];
function expand(spec) {
  const out = new Set();
  for (const p of spec.split(',')) {
    const m = /^([A-Z]+)(\d+)-([A-Z]+)(\d+)$/.exec(p);
    if (m) for (let n = +m[2]; n <= +m[4]; n++) out.add(m[1] + n); else out.add(p);
  }
  return out;
}

// Each footprint's reference and pads, from the file's text: a footprint's block runs to the next one's start
function parts(file) {
  const t = fs.readFileSync(file, 'utf8');
  const out = [];
  const starts = [...t.matchAll(/^\s*\((?:footprint|module) /gm)].map(m => m.index);
  starts.forEach((s, i) => {
    const block = t.slice(s, i + 1 < starts.length ? starts[i + 1] : t.length);
    const ref = (/\(property "Reference" "([^"]+)"/.exec(block) || /\(fp_text reference "?([^"\s)]+)/.exec(block) || [])[1];
    if (!ref || /^H\d+$/.test(ref)) return;                     // (H1 ...: the mounting holes, not soldered)
    const pads = new Set(), kinds = new Set();
    for (const m of block.matchAll(/\(pad "?([^"\s)]*)"? (thru_hole|smd)\b/g)) { pads.add(m[1] + '/' + pads.size * (m[1] === '' ? 1 : 0)); kinds.add(m[2]); }
    if (pads.size) out.push({ ref, pads: pads.size, smd: kinds.has('smd') && !kinds.has('thru_hole') });
  });
  return out;
}

function main() {
  const args = process.argv.slice(2);
  const file = args.find(a => !a.startsWith('--'));
  if (!file) { console.error('usage: joints.js PCB [--steps] [--parts]'); process.exit(2); }
  const ps = parts(file);
  const total = ps.reduce((n, p) => n + p.pads, 0);
  if (args.includes('--parts')) for (const p of ps.slice().sort((a, b) => a.ref.localeCompare(b.ref, 'en', { numeric: true }))) console.log(p.ref.padEnd(6) + String(p.pads).padStart(4) + (p.smd ? '  smd' : ''));
  if (args.includes('--steps') || /hydra-16\.kicad_pcb$/.test(file)) {
    const seen = new Set();
    for (const [name, spec] of STEPS) {
      const refs = expand(spec);
      const mine = ps.filter(p => refs.has(p.ref));
      mine.forEach(p => seen.add(p.ref));
      console.log(String(mine.reduce((n, p) => n + p.pads, 0)).padStart(5) + ' joints, ' + String(mine.length).padStart(3) + ' parts  step ' + name);
    }
    const rest = ps.filter(p => !seen.has(p.ref));
    if (rest.length) console.log(String(rest.reduce((n, p) => n + p.pads, 0)).padStart(5) + ' joints, ' + String(rest.length).padStart(3) + ' parts  in no step: ' + rest.map(p => p.ref).join(' '));
  }
  console.log(String(total).padStart(5) + ' joints, ' + String(ps.length).padStart(3) + ' parts  in all (' + ps.filter(p => p.smd).reduce((n, p) => n + p.pads, 0) + ' surface-mount)');
}
main();
