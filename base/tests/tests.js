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

// A call's address, from the SDK's hydra.inc (sdk/asm: made from spec/), as hex digits (lo hi): for a program typed
// into wozmon
function call(name) {
  const inc = fs.readFileSync(path.join(__dirname, '..', 'sdk', 'asm', 'hydra.inc'), 'latin1');
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
      check: (m, out) => out.includes(': ser') ? ['ser started'] : [],
    },
    // ---- The kernel's (HydraOS's until the base had its own: a test module as init, kdev and ser around it)
    {
      name: 'note', what: 'notes: the defaults, handlers, a note to oneself, WAIT ended by one, note groups',
      init: 't_note', modules: ['t_child'], cycles: 40e6,
    },
    {
      name: 'file', what: 'files and servers: OPEN, READ, WRITE, SEEK, STAT, DUP; text, ctl, data, directories; waiting',
      init: 't_file', modules: ['t_child', 't_srv'], cycles: 40e6,
    },
    {
      name: 'ns', what: 'namespaces: BIND, MOUNT, UNMOUNT, unions and union directories, CHDIR, clean names, inheritance',
      init: 't_ns', modules: ['t_child', 't_srv'], cycles: 40e6,
    },
    {
      name: 'proc', what: '/proc/N\'s mem (its RAM, bank, ROMs, the I/O area), ram (its banks), regs, env, note and fd (FD2PATH, TR_FD); ctl\'s stop and start; the kernel task\'s and a driver\'s refused',
      init: 't_proc', modules: ['t_child'], cycles: 40e6,
    },
    {
      name: 'env', what: 'environments: ENV_GET, ENV_PUT, ENV_DEL, ENV_NAME, a child\'s copy, #e (/env) as files',
      init: 't_env', modules: ['t_child'], cycles: 40e6,
    },
    {
      name: 'kmesg', what: 'the kernel\'s messages: KMESG (the boot\'s banner first, at offsets, the ring full: its last KMESG_SIZE) and /dev/kmesg (kdev\'s #n/kmesg) read in parts',
      init: 't_kmesg', cycles: 60e6,
    },
    {
      name: 'mem', what: 'memory: BREAK, pages, banks, a shared segment between tasks (and kcopy from it); #r (raw RAM, init\'s); #s (a segment by name)',
      init: 't_mem', modules: ['t_child'], cycles: 30e6,
    },
    {
      name: 'sem', what: 'semaphores: counts and mutexes, waits ended by a release, a free and a note, a task\'s end; GETPPID',
      init: 't_sem', cycles: 30e6,
    },
    {
      name: 'xcall', what: 'XCALL: a library module\'s routines (t_lib), registers and flags both ways, its bank and back, a system call from it',
      init: 't_xcall', modules: ['t_lib'], cycles: 10e6,
    },
    {
      name: 'banks', what: 'a module of two banks: calls between them (FAR2, FAR1), registers and C, each bank\'s data',
      init: 't_bank2', cycles: 10e6,
    },
    {
      name: 'banks3', what: 'a module of three banks: calls from any bank to any (FARN), registers and C, each bank\'s data, each bank set again',
      init: 't_bank3', cycles: 10e6,
    },
    {
      name: 'kcopy', what: 'spike S2: copying between tasks',
      init: 't_kcopy', cycles: 40e6,
      budgets: [{ what: 'kcopy, 4096 bytes (DBG_KCOPY)', from: '<kc', to: 'kc>', per: 4096, max: 40 }],
    },
  ],
};
