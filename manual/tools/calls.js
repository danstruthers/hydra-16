#!/usr/bin/env node
// calls.js - Appendix K's entries, from the system calls' one source (base/spec/api.def): each call's address, its
// registers in and out, its errors and whether it waits, with a sentence of the book's own on what it does; written
// as Markdown on stdout, a section for each group.
// Usage: node manual/tools/calls.js [--update manual/app-k-calls.md]   (--update: between the file's markers)
'use strict';
const fs = require('fs');
const path = require('path');
const src = fs.readFileSync(path.join(__dirname, '..', '..', 'base', 'spec', 'api.def'), 'utf8').split(String.fromCharCode(13)).join('');

const GROUPS = {
  system: 'The system', task: 'Tasks', memory: 'Memory', file: 'Files', name: 'Namespaces', cons: 'The console',
  time: 'The clock', server: 'Drivers', dbg: 'Debugging HydraOS',
};
const WHAT = {
  SYSINFO: 'What the Hydra has: the version of the system calls, the RAM modules, and the free tasks.',
  ERRSTR: 'An error code\'s message, as text (Appendix G).',
  KMESG: 'The kernel\'s messages: what it printed as the Hydra started (also `/dev/kmesg`).',
  REBOOT: 'Start the Hydra again, as the RESET button does; or start the hardware test.',
  XCALL: 'Call a routine in another bank of the paged ROM: a library\'s. Every register goes to it and comes back.',
  EXITS: 'End this program, with an exit code and a message (its status). Returning from `main` is `EXITS` with code 0.',
  WAIT: 'Wait for a program this one started to end, and get its exit code and message.',
  SPAWN: 'Start a program, in a task of its own.',
  GETPID: 'This task\'s number (0 to 15).',
  YIELD: 'Let the other tasks have a turn.',
  SLEEP: 'Wait for a number of ticks (200 a second).',
  SLEEP_UNTIL: 'Wait until the tick count reaches a number: for keeping steady time.',
  TICKS: 'The tick count: 200 a second since the Hydra started.',
  PREEMPT_OFF: 'Keep the processor: no other task runs until `PREEMPT_ON`. Use it only for a moment.',
  PREEMPT_ON: 'Let the other tasks run again, after `PREEMPT_OFF`.',
  TASKINFO: 'What the kernel knows about a task: its state, parent, time used, name (as `ps` shows).',
  NOTIFY: 'Set this task\'s note handler (section 27.16), or take it away.',
  NOTE: 'Send a note to a task, or to a note group.',
  MODINFO: 'An entry of the paged ROM\'s module directory: a module\'s banks, its page in its first bank (where its header is, for a small module that shares the bank), its type and its name.',
  ENV_GET: 'Read an environment variable, of this task or another.',
  ENV_PUT: 'Set an environment variable (or part of its value).',
  ENV_DEL: 'Remove an environment variable.',
  ENV_NAME: 'The name of an environment\'s variable number X: for listing them all.',
  TASKREAD: 'Read another task\'s arguments, current directory, environment, registers or open files (as `/proc` does).',
  GETPPID: 'The task that started this one.',
  SEM_NEW: 'Make a **semaphore**: a counter that tasks take and give back, to share something without getting in each other\'s way.',
  SEM_ACQUIRE: 'Take one from a semaphore; if it has none, wait until another task gives one back.',
  SEM_TRY: 'Take one from a semaphore, but don\'t wait if it has none.',
  SEM_RELEASE: 'Give one back to a semaphore, waking a task that\'s waiting for it.',
  SEM_FREE: 'Free a semaphore.',
  SEM_WAIT: 'Take one from a semaphore, waiting a given number of ticks at most for it.',
  RUN: 'Run a bank program in this task, as a subroutine, and wait for it: loaded once into banks of the task\'s own and kept for the next time.',
  TASKFORE: 'Put this task, or one it started, in the **foreground**: the scheduler picks it first whenever it can run (a tick at most), leaving the others a turn after FORE_RUN ticks; or take it out.',
  EXEC: 'Run another program in this task, in this one\'s place.',
  BREAK: 'Move the end of the program\'s memory (its **break**), or ask where it is.',
  PAGES_ALLOC: 'Take some pages (256 bytes each) of the task\'s memory, above the break.',
  PAGES_FREE: 'Give pages back.',
  BANKS: 'How many RAM banks the task has, and on which modules.',
  BANKS_ALLOC: 'Take some of the task\'s RAM banks: A gives the first bank\'s number, to write to $00.',
  BANK_LOAD: 'Find a bank program\'s child (functions a program calls itself, in a bank) in the task\'s code cache, loading it if it isn\'t there: where its entries are.',
  BANK_FLUSH: 'Give up every program and child kept in the task\'s code cache, and free the banks left empty.',
  BANKS_FREE: 'Give banks back.',
  SEG_CREATE: 'Make a shared **segment**: shared RAM banks that other tasks can attach to.',
  SEG_ATTACH: 'Attach to a segment.',
  SEG_DETACH: 'Detach from a segment (the last task to leave frees it).',
  SEG_MAP: 'How to reach a bank of a segment: the values to write to U and to $00.',
  SEGINFO: 'The shared RAM: how much there is, and which segments are in use.',
  BANKS_ALLOC_IN: 'As `BANKS_ALLOC`, from a range of bank numbers only.',
  SEG_CREATE_IN: 'As `SEG_CREATE`, from a range of the shared RAM only.',
  OPEN: 'Open a file, for reading or writing or both.',
  CREATE: 'Make a new file, or a directory, and open it.',
  CLOSE: 'Close a file.',
  READ: 'Read bytes from a file.',
  WRITE: 'Write bytes to a file.',
  SEEK: 'Move the place in a file where the next read or write happens.',
  STAT: 'A file\'s **stat record**: its name, type, length, time and more.',
  FSTAT: 'An open file\'s stat record.',
  WSTAT: 'Change a file: its name, its mode, or its length.',
  FWSTAT: 'Change an open file, as `WSTAT` does.',
  REMOVE: 'Remove a file, or an empty directory.',
  DUP: 'Another fd for an open file.',
  DUP2: 'Make fd X the same as fd A (closing what X was first): how the shell does `>` and `|`.',
  CHDIR: 'Change the current directory.',
  GETCWD: 'The current directory.',
  PIPE: 'Make a **pipe**: two fds, and what\'s written to one is read from the other.',
  FD2PATH: 'The name an open file was opened by.',
  BIND: 'Bind a name to another place in the namespace (section 9.5).',
  MOUNT: 'Mount a device\'s files in the namespace.',
  UNMOUNT: 'Undo a bind or a mount; or, with no old, all of them: an empty namespace of the task\x27s own.',
  NSINFO: 'An entry of a task\'s namespace: what\'s bound or mounted where (as `ns` shows).',
  PUTC: 'Write a byte (a character) to the standard output.',
  PUTS: 'Write a string to the standard output.',
  GETC: 'Read a byte from the standard input, waiting for one.',
  PUTHEX: 'Write a byte to the standard output as two hex digits.',
  TIME: 'The date and time: seconds since the start of 2000.',
  TIME_SET: 'Set the date and time.',
  RTC: 'Read or write the DS1747 clock chip\'s registers.',
  IRQ_OWN: 'A driver: take an interrupt line, so its interrupts run the driver\'s handler.',
  IRQ_RELEASE: 'A driver: give an interrupt line back.',
  WAKE: 'Wake a task that\'s waiting in `PAUSE`.',
  PAUSE: 'Wait until another task (or an interrupt handler) wakes this one.',
  NOTE_POST: 'A driver: send a note.',
  SRV_REGISTER: 'A driver: serve a device letter, so the requests for its files come to this task.',
  SRV_TAKE: 'A driver: take the request being served.',
  SRV_REPLY: 'A driver: answer the request being served.',
  CLIENT_READ: 'A driver: copy bytes from the program whose request it\'s serving.',
  CLIENT_WRITE: 'A driver: copy bytes to the program whose request it\'s serving.',
  NOTE_QUEUE: 'An interrupt handler: send a note to a note group (as the console does for <kbd>CTRL</kbd>-<kbd>C</kbd>).',
  ROMREAD: 'Copy bytes from any bank of the paged ROM.',
  TASKMEM: 'A driver: read or write another task\'s memory (as `/proc`\'s `mem` does).',
  TASKSTOP: 'A driver: stop a task where it is, or start it again.',
  TASKSTEP: 'A driver: the debugger\'s step.',
  SRV_NAP: 'A driver with work of its own between its requests: wait a while (its requests are served meanwhile), then go on.',
  INIT_RUN: 'Start init again in a task of its own, with a number as its argument (the console: a new window\'s shell).',
  DBG_SCALL: 'For testing: call a driver directly.',
  DBG_KCOPY: 'For testing: copy bytes between this task and another.',
  DBG_PS: 'For testing: print a line for each task.',
};

const reg = s => s.replace(/\.([AXY])\b/g, '$1');
const calls = [];
let cur = null;
for (const l of src.split('\n')) {
  let m = /^call\s+(\w+)\s+(\w+)/.exec(l);
  if (m) { cur = { name: m[1], group: m[2], in: [], out: [], errors: [], blocks: '', hl: false }; calls.push(cur); continue; }
  if (!cur || !/^\s/.test(l)) { if (!/^\s/.test(l)) cur = null; continue; }
  m = /^\s+(in|out|errors|blocks|hl):\s*(.*)$/.exec(l);
  if (!m) continue;
  if (m[1] === 'blocks') cur.blocks = m[2].trim();
  else if (m[1] === 'hl') cur.hl = true;
  else cur[m[1]].push(m[2].trim());
}
// The table's addresses: each group's base, its calls 3 bytes apart in order
const base = {};
for (const l of src.split('\n')) { const m = /^group\s+(\w+)\s+\$([0-9A-F]+)/.exec(l); if (m) base[m[1]] = parseInt(m[2], 16); }
const next = {};
let out = '', group = '', n = 0;
for (const c of calls) {
  if (c.group !== group) {
    group = c.group;
    out += '\n## K.' + (++n) + ' ' + GROUPS[group] + '\n\n';
  }
  const addr = (next[group] = (next[group] ?? base[group]) + 3) - 3;
  if (!WHAT[c.name]) throw new Error('no WHAT for ' + c.name);
  const ins = c.in.join(' '), outs = c.out.join(' ');
  const errs = [...new Set(c.errors.join(' ').match(/\bE_[A-Z]+/g) || [])].join(', ');
  out += '**`' + c.name + '`** ($' + addr.toString(16).toUpperCase() + '). ' + WHAT[c.name] +
    (ins && ins !== '-' ? ' *In:* ' + reg(ins) + '.' : '') +
    (outs && outs !== '-' ? ' *Out:* ' + reg(outs) + '.' : '') +
    (errs && errs !== '-' ? ' *Errors:* ' + errs.replace(/\.$/, '') + '.' : '') +
    (c.blocks === 'yes' ? ' *It can wait.*' : '') +
    (c.hl ? '' : ' *(Not in the languages.)*') + '\n\n';
}
emit(out.replace(/\.\./g, '.').replace(/^\n+/, ''), 'tools/calls.js');

// --update FILE: the text between FILE's markers (<!-- generated: TAG --> and <!-- end generated: TAG -->) made this
// output; else it's written on stdout
function emit(text, tag) {
  const i = process.argv.indexOf('--update');
  if (i < 0) { process.stdout.write(text); return; }
  const f = process.argv[i + 1], a = '<!-- generated: ' + tag + ' -->\n\n', b = '\n<!-- end generated: ' + tag + ' -->';
  const s = fs.readFileSync(f, 'utf8').split(String.fromCharCode(13)).join(''), p = s.indexOf(a), q = s.indexOf(b, p);
  if (p < 0 || q < 0) throw new Error(f + ': no markers for ' + tag);
  fs.writeFileSync(f, s.slice(0, p + a.length) + text.replace(/\n+$/, '\n') + s.slice(q));
}
