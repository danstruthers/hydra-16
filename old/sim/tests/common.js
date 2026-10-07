// ****************************************************************************
// tests/common.js - what the regression tests (sim/tests/*.js) share: the typed input's waits, Forth numbers as
// they print, the ROM build's symbols, songs, scripts and ROM images some tests make.
// ****************************************************************************
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const hydrafs = require('../tools/hydrafs.js');            // For the tests that want a HydraFS card
const { hyx } = require('../tools/mkhyx.js');              // ... and Hydra executables on it

const CARDS = path.join(__dirname, '..', 'cards');                    // Fixture card images (cards/README.md)
const W = n => '\\w'.repeat(n);                                 // Wait n * ~2M cycles before the next key
const P = '\\p';                                                 // Wait for a prompt (a new one: "> ", or WOZMON's ">")
const C_SAMPLES = ['hello', 'ctest', 'upper', 'code', 'keys', 'tones', 'jukebox'];         // programs/c/bin's (make.bat builds them)

// A ZSM song (the player's format: sound/player.s) for the tests: channel 0, a sine (algorithm 7), an intro note
// (C4) and a loop of one note (E4), each 30 ticks on and 6 off at 60 Hz; a PSG write and an extension to skip
function zsmSong() {
  const fm = pairs => [0x40 | pairs.length / 2, ...pairs];
  const voice = [0x20, 0xC7, 0x38, 0x00];
  for (const op of [0x00, 0x08, 0x10, 0x18]) voice.push(0x40 + op, 0x01, 0x60 + op, 0x10, 0x80 + op, 0x1F, 0xA0 + op, 0x00, 0xC0 + op, 0x00, 0xE0 + op, 0x0F);
  const note = kc => [...fm([0x28, kc, 0x30, 0x00, 0x08, 0x78]), 0x80 + 30, ...fm([0x08, 0x00]), 0x80 + 6];
  const intro = [...fm(voice), 0x05, 0x3F, 0x40, 0x82, 0x12, 0x34, ...note(0x3E)];    // (A PSG write; an extension)
  const loop = [...note(0x44)];
  const loopAt = 16 + intro.length;
  const hdr = [0x7A, 0x6D, 1, loopAt & 255, loopAt >> 8 & 255, loopAt >> 16, 0, 0, 0, 0x01, 0, 0, 60, 0, 0, 0];
  return Buffer.from([...hdr, ...intro, ...loop, 0x80]);
}

const BOOT = P;                                                // Before the first key: to the HyForth prompt
const TO_MON = BOOT + 'bye\\r' + P;                             // To WOZMON (its prompt: "T1 00:00>")

// A symbol's value from the ROM build's debug info, in a scope (e.g. 'PAGE1': page 1's 'next', not page A's)
function romSym(name, scope) {
  const dbg = fs.readFileSync(path.join(__dirname, '../../os_rom/obj/os_rom_C02.dbg'), 'latin1');
  const sc = new RegExp('^scope\\tid=(\\d+),name="' + scope + '"', 'm').exec(dbg);
  const m = sc && new RegExp('^sym\\t.*name="' + name + '",.*scope=' + sc[1] + ',.*val=0x([0-9A-F]+)', 'm').exec(dbg);
  if (!m) throw new Error('romSym: no ' + scope + '::' + name + ' in os_rom_C02.dbg');
  return parseInt(m[1], 16);
}

// A Forth number as "." prints it: " 0003"
const num = n => ' ' + n.toString(16).toUpperCase().padStart(4, '0');
// The sparse test's writes: 4 bytes ("WXYZ") at each offset, from a script on the card.  Some at the edges
// (a block's end, a cluster's end, past the end, a hole's first and last clusters), then pseudo-random ones
// (always the same) up to 200000: each makes a hole, or fills one in
const SPARSE_WRITES = (() => {
  const list = [0, 511, 4094, 200000, 12288, 8190, 196606, 102400, 106494, 61441];
  for (let s = 12345, i = 0; i < 30; i++) { s = (s * 1103515245 + 12345) % 2147483648; list.push(Math.floor(s / 32768) % 200000); }
  return list;
})();
const SPARSE_SCRIPT = '"s" 0 create .\r\n"WXYZ" @ 3 + cons wxyz drop\r\n: w seek 3 wxyz 4 write drop ;\r\n' +
  SPARSE_WRITES.map(o => '3 $' + (o & 0xFFFF).toString(16).toUpperCase() + ' $' + (o >>> 16).toString(16).toUpperCase() + ' w\r\n').join('') + '3 close\r\n';
// The CPU cycle test's program: at $E000 on every BIOS page (W powers up random), then STP.  Each
// instruction with its W65C02S cycles (WDC's table and extras); the test checks the total.
function cycleTestRom() {
  const code = [], at = n => 0xE000 + code.length + n;
  let cycles = 0;
  const op = (c, ...bytes) => { code.push(...bytes); cycles += c; };
  op(2, 0xA2, 0xFF);                                            // LDX #$FF
  op(2, 0xA0, 0x01);                                            // LDY #$01
  op(5, 0xBD, 0xF0, 0x10);                                      // LDA $10F0,X: crosses a page, +1
  op(4, 0xB9, 0x00, 0x10);                                      // LDA $1000,Y: doesn't
  op(5, 0x9D, 0xF0, 0x10);                                      // STA $10F0,X: a store, always 5
  op(7, 0x1E, 0xF0, 0x10);                                      // ASL $10F0,X: 6, +1 across a page (65C02)
  op(6, 0x1E, 0x00, 0x10);                                      // ASL $1000,X
  op(7, 0xFE, 0x00, 0x10);                                      // INC $1000,X: always 7
  op(2, 0xF8);                                                  // SED
  op(3, 0x69, 0x01);                                            // ADC #1: +1 in decimal mode
  op(2, 0xD8);                                                  // CLD
  op(2, 0xA0, 0x01);                                            // LDY #1 (Z = 0)
  op(3, 0xD0, 0x00);                                            // BNE: taken, +1
  op(2, 0xF0, 0x00);                                            // BEQ: not taken
  const jsr = code.length; op(6 + 6, 0x20, 0, 0);               // JSR sub (and its RTS)
  op(3, 0x64, 0x10);                                            // STZ $10
  op(6, 0x0F, 0x10, 0x00);                                      // BBR0 $10: 5, taken +1
  op(3, 0x4C, 0xFC, 0xE0);                                      // JMP $E0FC
  const sub = at(0); code.push(0x60);                           // sub: RTS
  code[jsr + 1] = sub & 0xFF; code[jsr + 2] = sub >> 8;
  while (code.length < 0xFC) code.push(0xEA);
  op(4, 0xD0, 0x10);                                            // $E0FC BNE $E10E: taken, to another page, +2
  while (code.length < 0x10E) code.push(0xEA);
  op(3, 0xDB);                                                  // $E10E STP
  const page = Buffer.alloc(0x2000, 0xEA); Buffer.from(code).copy(page);
  page[0x1FFC] = 0x00; page[0x1FFD] = 0xE0;                     // RESET: $E000
  return { bios: Buffer.concat(Array(16).fill(page)), cycles };
}


module.exports = { fs, os, path, hydrafs, hyx, CARDS, W, P, C_SAMPLES, zsmSong, BOOT, TO_MON, romSym, num, SPARSE_WRITES, SPARSE_SCRIPT, cycleTestRom };
