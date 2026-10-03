// ****************************************************************************
// tests.js - the tests sim/test.js runs, in order.  Each boots a paged ROM of the system's modules and its own
// (tests/mod), with one of them as init, and runs till init says "NAME: PASS" or "NAME: FAIL" (or the cycles run
// out).  A test module's lines are "ok - ..." or "not ok - ..." (tests/mod/testlib.inc).
//
//   name, what       the test
//   init             the module started as init (a test module, or the system's init)
//   modules          more modules for the image (the system's are always there, but those in without)
//   without          system modules left out (a test that owns the serial port itself, or needs its driver as
//                    task F)
//   cycles           at most this many (3.58 MHz: 3579545 a second)
//   expect           lines the output must have (for a test without "PASS")
//   machine          the emulator's options for it (sim/lib/machine.js): faults, keys typed (input), modules
//   send             { after: a mark, bytes: [...] }: the PC sends them, back to back, when the mark is out
//   budgets          [{ what, from, to, minus, per, max }]: the cycles between two marks (less those between the
//                    two marks in minus, a baseline), divided by per, at most max
//   check(m, out)    more checks on the machine afterwards: gives a list of failures
// Every test also checks the longest IRQs-off stretch after the boot (IRQ_OFF_MAX).
'use strict';

const IRQ_OFF_MAX = 200;                                      // (docs/reimplementation-from-scratch.md, §8: 115200)
const S1_BYTES = 2000;

module.exports = {
  IRQ_OFF_MAX,
  tests: [
    {
      name: 'boot', what: 'the kernel boots, POST finds nothing wrong; init runs hello and waits for it',
      init: 'init', cycles: 6e6,
      expect: ['Hydra-16 reborn: kernel 0.1, ABI 1', 'POST ZP:0 ST:0 OS:0 HI:0 SH:S W:0',
        'RAM U:0 F0:0/00/0000 F4:0/00/0000 F8:0/00/0000 FC:0/00/0000 00:0/00/0000 10:0/00/0000', 'POST ok', 'RAM modules: 02',
        'task F: cons', 'task 1: init', 'init: up in task 01', 'hello, from init', 'init: hello ended: code $07 (bye)'],
    },
    {
      name: 'post-t', what: 'POST: a T line stuck low (U7)',
      init: 'init', cycles: 3e6, machine: { u7Fault: { mask: 4, high: false } },
      expect: ['POST ZP:4 ST:4 OS:4 HI:4 SH:S W:0', 'POST found faults: T lines'],
    },
    {
      name: 'post-zp', what: 'POST: only the zero page and stack per task (a decoding fault)',
      init: 'init', cycles: 3e6, machine: { model: 'zponly' },
      expect: ['POST ZP:0 ST:0 OS:F HI:F', 'POST found faults: T lines'],
    },
    {
      name: 'post-ram', what: 'POST: an address line of RAM module 1 stuck low; the module is left unused',
      init: 'init', cycles: 3e6, machine: { ramFault: { bank: 0x10, mask: 0x100, high: false } },
      expect: [' 00:0/00/0000 10:0/00/0100', 'POST found faults: RAM (left unused)'],
    },
    {
      name: 'post-sh', what: 'POST: no shared RAM',
      init: 'init', cycles: 3e6, machine: { model: 'noshared' },
      expect: ['SH:X', 'POST found faults: shared RAM RAM (left unused)'],
    },
    {
      name: 'hwtest', what: 'a T typed during POST starts the hardware test (paged ROM bank 1)',
      init: 'init', cycles: 3e6, machine: { input: 'T' },
      expect: ['Hydra-16 hardware test'],
    },
    {
      name: 'task', what: 'tasks and the scheduler: SPAWN, EXITS, WAIT, SLEEP, preemption, PAUSE and WAKE, orphans',
      init: 't_task', modules: ['t_child'], without: ['cons', 'kdev'], cycles: 60e6,
    },
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
      name: 'dev', what: 'the kernel\'s devices (kdev): #/, #n, #t, #m, #p; pipes; a union keeping what was there',
      init: 't_dev', modules: ['t_child'], cycles: 40e6,
    },
    {
      name: 'cons', what: 'the console: lines, editing, history, raw keys, Ctrl-C, windows (shown, repainted, made, gone), 115200',
      init: 't_cons', modules: ['t_child'], cycles: 80e6,
      // (ā: wait for a prompt, "N> ")
      machine: { input: 'āhello\r' + 'āabX\x08c\r' + 'āac\x1b[Db\r' + 'ābc\x1b[Ha\x1b[Fd\r' +
        'āxyz\x15ok\r' + 'āabXc\x1b[D\x1b[D\x1b[3~\r' + 'ā\x1b[A\x1b[A\r' + 'ā\x04' + 'āparts\r' +
        'āx\x1b[A' + 'ā\x03' +
        'ā\x1d1z\r\x1d0' + 'ā\x1d1\x03\x1d0' + 'ā\x1dc' },
      check(m, out) {
        const f = [], a = m.acia, want = a.wdc ? 1 : 2;
        if (!out.includes('\x1b[2J') || !out.includes('w1 hidden text')) f.push('window 1 shown: no repaint of its text');
        this.notes = ['at 115200, the shortest idle time between characters sent: ' + a.gapMin.toFixed(2) + ' bits (at least ' + want + ')'];
        if (!(a.gapMin >= want - 0.05)) f.push('at 115200, characters ' + a.gapMin.toFixed(2) + ' bits apart: less than ' + want);
        if (a.overruns) f.push(a.overruns + ' bytes written to the ACIA while it was still sending');
        return f;
      },
    },
    {
      name: 'mem', what: 'memory: BREAK, pages, banks, a shared segment between tasks (and kcopy from it)',
      init: 't_mem', modules: ['t_child'], cycles: 30e6,
    },
    {
      name: 'scall', what: 'spike S3: calls into a driver\'s task, its errors, a busy driver, the round trip',
      init: 't_scall', modules: ['t_child', 't_drv'], without: ['cons', 'kdev'], cycles: 40e6,
      budgets: [{ what: 'SCALL round trip (DBG_SCALL, less the same loop calling the code in place)', from: '<scall', to: 'scall>',
        minus: ['<base', 'base>'], per: 1000, max: 200 }],
    },
    {
      name: 'kcopy', what: 'spike S2: copying between tasks',
      init: 't_kcopy', cycles: 40e6,
      budgets: [{ what: 'kcopy, 4096 bytes (DBG_KCOPY)', from: '<kc', to: 'kc>', per: 4096, max: 40 }],
    },
    {
      name: 'irq', what: 'spike S1: 115200 received by an irq entry while tasks spin',
      init: 't_irq', modules: ['t_child'], without: ['cons', 'kdev'], cycles: 30e6,
      send: { after: 'ready>', bytes: Array.from({ length: S1_BYTES }, (_, i) => (3 + 7 * i) & 0xFF) },
      check(m) {
        const f = [], l = m.acia.rxLat, char = m.acia.charCycles();
        if (m.acia.pcLost) f.push(m.acia.pcLost + ' bytes lost by the ACIA (one arrived before the last was read)');
        if (l.n) this.notes = ['receive latency (a byte in to its read): ' + l.min + '-' + l.max + ' cycles, ' + Math.round(l.sum / l.n) +
          ' on average, over ' + l.n + ' bytes; the limit is a character: ' + char + ' cycles'];
        if (l.max > char) f.push('a byte waited ' + l.max + ' cycles to be read: over a character\'s time (' + char + ')');
        return f;
      },
    },
  ],
};
