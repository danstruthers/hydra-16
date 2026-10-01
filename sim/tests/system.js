// ****************************************************************************
// tests/system.js - regression tests (sim/regress.js runs them): the machine: CPU cycles, boot and POST, the self tests, the hardware test.  A test's fields are described at
// the top of regress.js.
// ****************************************************************************
'use strict';
const { W, BOOT, TO_MON, cycleTestRom, P } = require('./common.js');

module.exports = [
  {
    name: 'cpu-cycles', about: 'the W65C02S cycle counts: page crossing, branches, decimal mode, RMW abs,X, JSR/RTS, BBR',
    romImage: cycleTestRom,
    args: ['--cycles', '1000'],
    halts: /STP at [0-9A-F]:E10E/,                              // (On whichever page W powered up as)
    check: (out, report, files) => {
      const c = +/--- cycles (\d+)/.exec(report)[1];
      if (c !== files.expectCycles) return 'the program took ' + c + ' cycles, not ' + files.expectCycles;
    },
  },
  {
    name: 'boot', about: 'POST (task mapping, shared RAM, ROM page 1, RAM lines), drivers, HyForth banner',
    args: ['--cycles', '30000000'],
    expect: ['POST ZP:T ST:T LO:T 7D:T SH:S P1:4C\n',
      /RAM U:0( [0-9A-F]{2}:0\/00\/0000)+\n/,
      'Welcome to the HYDRA-16!', /HyForth \d/, '/ram/1> '],
  },
  {
    name: 'selftest', about: 'the ROM self tests from WOZMON: MMU (F833), scheduler (F869), IO (F88A)',
    args: ['--cycles', '60000000', '--input', TO_MON + 'F833R\\r' + W(2) + 'F869R\\r' + W(4) + 'F88AR\\r' + P],
    expect: ['T1 00:00>F833R', 'MMU test: ok',
      'Sched test:\n', /^(?=[abm]*a)(?=[abm]*b)(?=[abm]*m)[abm]+\n/m,  // 1. a, b and m interleave
      /^m*\[c{10}\]m+\n/m,                                      // 2. NO_PREEMPT: the c's all together
      'wW\n', 'done',                                           // 3. TASK_WAIT, IO_WAKE
      'IO test: cons ok'],
    forbid: ['FAIL'],
  },
  {
    name: 'selftest-small', about: 'the self tests with 1 RAM module and 1 shared RAM macro-page',
    args: ['--cycles', '60000000', '--modules', '1', '--shared-u', '1',
      '--input', BOOT + 'mmtest\\r' + P + 'bye\\r' + P + 'F869R\\r' + W(4) + 'F88AR\\r' + P],
    expect: [/RAM U:F F0:0\/00\/0000 F4:0\/00\/0000 F8:0\/00\/0000 FC:0\/00\/0000 00:0\/00\/0000\n/,
      'MMU test: ok', 'Sched test:', 'done', 'IO test: cons ok'],
    forbid: ['FAIL'],
  },
  {
    name: 'post-ram-fault', about: 'POST reports a stuck address line (A0 high on the chip holding shared bank $F0)',
    args: ['--cycles', '8000000', '--ram-fault', 'F0:A0:high'],
    expect: [/RAM U:0 F0:0\/00\/0001 F4:0\/00\/0000 /],
  },
  {
    name: 'post-no-shared', about: 'POST with no shared RAM: SH:X, and the drivers that need it fail',
    args: ['--cycles', '30000000', '--model', 'noshared'],
    expect: ['SH:X', 'SOUND FAIL', '/> '],
    bootFailOk: true,
  },
  {
    name: 'hwtest', about: 'the hardware test (paged ROM bank 1) from HyForth\'s hwtest: all its tests pass (an SD card on device 0), then R resets into POST and the OS',
    sd: true,
    args: ['--cycles', '90000000', '--input', BOOT + 'hwtest\\r' + P + 'AR'],
    expect: ['Hydra-16 hardware test', 'CPU ................ ok', 'T U V W registers .. ok', 'shared RAM ......... ok',
      'RAM bank registers . ok', 'task RAM ........... ok', 'RAM modules ........ 0 1 2 ok', 'BIOS ROM ........... ok',
      'paged ROM .......... ok', /interrupts \.+ +ok/,'VIA ................ ok', 'sound chip (YM2151) . ok',
      /CPU clock \.+ 3\.58 MHz \(first \d+, next \d+\) ok/,'serial port (ACIA) .    9600 baud ok', 'SPI devices ........ SD cards 0 ok',
      'I2C bus ............ no devices ok', 'slot cards ......... all empty ok', 'hwtest: all passed\n',
      '> R\n', 'POST ZP:T', 'Welcome to the HYDRA-16!', '/ram/1> '],
    forbid: ['FAIL'],
  },
  {
    name: 'hwtest-faults', about: 'the hardware test from POST (a T typed) finds faults: a shared RAM address line, the ACIA on the wrong IRQ line, the CPU clock (and so the serial timing)',
    args: ['--cycles', '40000000', '--input', 'T' + P + '3IKS', '--acia-line', '3', '--clock', '7.16', '--ram-fault', 'F4:A3:high'],
    expect: ['Hydra-16 hardware test', 'shared RAM ......... FAIL bank 04 8000 bits 08\n',
      /interrupts \.+ +FAIL ACIA IRQ on line 3 +all at once: ACIA IRQ on line 3\n/,          // (Its spaces: the ACIA's test)
      /CPU clock \.+ 7\.16 MHz \(first \d+, next \d+\) FAIL \(the ROM's built for 3\.58 MHz\)\n/,
      /serial port \(ACIA\) \. +FAIL a character took 76\d\d cycles\n/],
  },
  {
    name: 'hwtest-u7', about: 'the hardware test from POST with task RAM line A17 stuck low at U7 (tasks F and B one): test 5 says which, and the pin',
    args: ['--cycles', '20000000', '--input', 'T' + P + '5', '--u7-fault', 'A17:low'],
    expect: ['task RAM ........... FAIL task F has task B\'s mark: U7 A17 (pin 30)\n'],
  },
  {
    name: 'hwtest-irq', about: 'the hardware test from POST with IRQ line 9 held active (the OS can\'t run): the interrupts test says so',
    args: ['--cycles', '20000000', '--input', 'T' + P + 'I', '--stuck-irq', '9'],
    expect: ['interrupts ......... FAIL an IRQ line is held active\n', 'hwtest: failed: 1\n'],
  },
];
