// ****************************************************************************
// tests.js - the base's regression tests (sim/test.js runs them; HydraOS's runner too, before its own): each boots
// the base's paged ROM image (modules/rom.txt: ser, wozmon; less its without, with its own modules: tests/mod's), its
// init, and is judged on its output (expect: every one of these in it; or its init's "PASS"/"FAIL" line, and its "ok"
// and "not ok" lines), its budgets, and its own checks (check).  Input: machine.input, as run.js --input's: ā waits
// for a prompt (the output ending with "> "), Ā for 2M cycles.
'use strict';
const fs = require('fs');
const path = require('path');

const IRQ_OFF_MAX = 200;                                      // The longest IRQs-off stretch after the boot, in cycles

// A call's address, from the SDK's hydra.inc (the build's), as hex digits (lo hi): for a program typed into wozmon
function call(name) {
  const inc = fs.readFileSync(path.join(__dirname, '..', 'obj', 'sdk', 'hydra.inc'), 'latin1');
  const m = inc.match(new RegExp('^' + name + '\\s+= \\$([0-9A-F]{4})', 'm'));
  if (!m) throw new Error('hydra.inc: no ' + name);
  return m[1].slice(2) + ' ' + m[1].slice(0, 2);
}

module.exports = {
  IRQ_OFF_MAX,
  tests: [
    {
      name: 'base', what: 'the base boots: POST, ser in task F, wozmon (the monitor) in task 1 as init, its prompt',
      init: 'wozmon', cycles: 20e6,
      expect: ['Hydra-16: kernel 0.1, ABI 1', 'POST ok', 'task F: ser', 'task 1: wozmon', 'Hydra-16 monitor (Woz\'s)', '\nT1 00> '],
    },
    {
      name: 'wozmon', what: 'the monitor on ser (raw): examine (the reset vector), store and a block, L\'s instructions and K\'s bytes, R (a program calling PUTC), Ctrl-C out of a loop and a BRK\'s note (the prompt again), Escape, Backspace, lower case',
      init: 'wozmon', cycles: 120e6,
      // (ā: a prompt.  1040: lda #$48, jsr PUTC, lda #$49, jsr PUTC, rts; 1050: jmp 1050; 1060: brk)
      get machine() {
        return { input: 'āFFFC.FFFD\r' + 'ā1000: 11 22 33\r' + 'ā1000.1002\r' +
          'ā1010: A9 41 8D 00 20 60\r' + 'āL 1010.1015\r' + 'āK 1010\r' +
          'ā1040: A9 48 20 ' + call('PUTC') + ' A9 49 20 ' + call('PUTC') + ' 60\r' + 'ā1040R\r' +
          'ā1050: 4C 50 10\r' + 'ā1050R\rĀ\x03' + 'ā1060: 00 00\r' + 'ā1060R\r' +
          'ā123\x1b' + 'ā1000.100X\b2\r' + 'ā2bcd: 5a\r' + 'ā2BCD\r' };
      },
      expect: ['T1 00> FFFC.FFFD\nFFFC: 00 E0\n', '1000.1002\n1000: 11 22 33\n',
        'L 1010.1015\n1010: A9 41     lda #$41\n1012: 8D 00 20  sta $2000\n1015: 60        rts\n', 'K 1010\n1010: A9\n',
        '1040R\n1040: A9HI\nT1 00> ', '1050R\n1050: 4C\nT1 00> 1060', '1060R\n1060: 00\nT1 00> 123\\\nT1 00> ',
        '\n1000: 11 22 33\nT1 00> 2bcd: 5a', '2BCD\n2BCD: 5A\nT1 00> '],
    },
    {
      name: 'wozpolled', what: 'the monitor with no console driver (no ser): the kernel\'s polled bring-up console; examine, store, a block, R',
      init: 'wozmon', without: ['ser'], cycles: 60e6,
      get machine() {
        return { input: 'āFFFC.FFFD\r' + 'ā1000: 11 22 33\r' + 'ā1000.1002\r' + 'ā1040: A9 48 20 ' + call('PUTC') + ' 60\r' + 'ā1040R\r' + 'ā' };
      },
      expect: ['task 1: wozmon', 'T1 00> FFFC.FFFD\nFFFC: 00 E0\n', '1000.1002\n1000: 11 22 33\n', '1040R\n1040: A9H\nT1 00> '],
      check: (m, out) => out.includes('task F') ? ['a console driver started'] : [],
    },
  ],
};
