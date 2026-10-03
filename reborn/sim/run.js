#!/usr/bin/env node
// ****************************************************************************
// run.js - the reborn system in the emulator (sim/lib: the Hydra-16 V1 board, cycle by cycle).  It boots
// bin/bios.bin and bin/prom.bin (node build.js), and either runs for a while and reports (the console's output,
// each task's state, the longest IRQs-off stretches, the stacks' depths), or is the serial console, live.
//
// Usage: node sim/run.js [options]
//   -i, --interactive   the terminal is the Hydra's serial console, in real time.  Ctrl-A x quits, Ctrl-A r resets,
//                       Ctrl-A s shows the state, Ctrl-A h helps
//   --cycles N          stop at cycle N (default 30000000: 8.4 s at 3.58 MHz; interactive: never)
//   --input TEXT        keys to type (\r, \n: Return; \w: wait 2M cycles), one every 20000 cycles from cycle 200000
//   --paste             type them as fast as the line goes (a byte arriving while the last is unread is lost)
//   --speed N           interactive: N times real time (0: as fast as it goes)
//   --clock MHz         the CPU clock (3.579545; 7.15909 with jumper J7: build with --clock 2 too)
//   --acia wdc          a WDC W65C51N in the serial port (build with --acia wdc too)
//   --modules N         RAM modules installed (default 2)
//   --seed N            the power-up's random RAM and registers, repeatable (default: random)
//   --trace N           the last N instructions in the report (default 25)
//   --watch-pc ADDR     log each time the PC reaches ADDR (hex, or a kernel label), on BIOS page 0
//   --bios FILE, --prom FILE   other images
//   --sd FILE           a card image (../sim/tools/hydrafs.js makes them), SD device 0, then 1 ...: read and
//                       written in the file itself, as the Hydra reads and writes it
//   --pc-dir DIR        /pc: the PC tool's part (../sim/tools/hydrapc.js) is played here, serving the folder DIR:
//                       the frames the Hydra sends for /pc are answered, at the line's rate (sim/lib/pchost.js)
//   --pc-read-only      /pc can't be changed: its writes, creates, removes and renames are refused
//   --pc-log            list /pc's requests as they're served (opens, creates, removes, renames, errors)
//   --pc-damage F[,F...]  damage /pc's frames on the line, to try the resends: qN the Nth frame the Hydra sends, rN
//                       the Nth reply (a byte of its body gets bit 6 flipped)
// From Node: boot(opt) gives the machine; labels() the kernel's labels; state(m) each task's state.
'use strict';
const fs = require('fs');
const path = require('path');
const { createMachine } = require('./lib/machine.js');
const { createPcHost } = require('./lib/pchost.js');

const ROOT = path.join(__dirname, '..');
const hx = (v, n = 2) => v.toString(16).toUpperCase().padStart(n, '0');
const CLOCK = 3.579545;
const STATES = ['free', 'ready', 'wait', 'call', 'idle', 'new', 'sleep', 'block', 'event'];
// The OS zero page (include/layout.inc): what the report reads in each task
const TK = { SP: 0x80, STATE: 0x81, FLAGS: 0x82, PREEMPT: 0x83, BUSY: 0x85 }, TA_NAME = 0x0230;

// The kernel's labels, by BIOS ROM page (obj/kernel/bios.dbg: each label's segment, and each segment's place in
// the image): { byName: Map (address), pageOf: Map (page), at(pc, page) -> "NAME+n" }
function labels(file = path.join(ROOT, 'obj', 'kernel', 'bios.dbg')) {
  const byName = new Map(), pageOf = new Map(), lists = [...Array(16)].map(() => []), segPage = new Map();
  if (fs.existsSync(file)) {
    const text = fs.readFileSync(file, 'latin1').split(/\r?\n/);
    for (const line of text) {
      const m = line.match(/^seg\tid=(\d+),.*ooffs=(\d+)/);
      if (m) segPage.set(+m[1], Math.floor(+m[2] / 0x2000));
    }
    for (const line of text) {
      const m = line.match(/^sym\tid=\d+,name="(\w+)",.*val=0x([0-9A-F]+),seg=(\d+),type=lab/);
      if (!m || !segPage.has(+m[3])) continue;
      const v = parseInt(m[2], 16), page = segPage.get(+m[3]);
      lists[page].push([v, m[1]]);
      if (!byName.has(m[1]) || page === 0) { byName.set(m[1], v); pageOf.set(m[1], page); }
    }
  }
  for (const l of lists) l.sort((a, b) => a[0] - b[0]);
  const at = (pc, page = 0) => {
    if (pc < 0xE000) return pc >= 0xA000 ? 'PROM:' + hx(pc, 4) : 'RAM:' + hx(pc, 4);
    let best = null;
    for (const e of lists[pc >= 0xFD00 ? 0 : page & 15]) { if (e[0] <= pc) best = e; else break; }   // (COMMON: page 0's names)
    return best ? best[1] + (pc > best[0] ? '+' + (pc - best[0]) : '') : hx(pc, 4);
  };
  return { byName, pageOf, at };
}

// The machine, booted from the images.  opt: createMachine's (sim/lib/machine.js), and bios, prom (files or bytes)
function boot(opt = {}) {
  const img = (v, f) => v instanceof Uint8Array ? v : fs.readFileSync(v || path.join(ROOT, 'bin', f));
  return createMachine(Object.assign({
    osrom: img(opt.bios, 'bios.bin'), pagedrom: img(opt.prom, 'prom.bin'),
    modules: 2, sharedU: 16, aciaLine: 1, stuckIrq: -1, acia: 'rockwell', clock: CLOCK, trace: 25,
    pcWatches: [], watches: [], marks: [], log: s => console.log('[sim] ' + s),
  }, opt, { osrom: img(opt.bios, 'bios.bin'), pagedrom: img(opt.prom, 'prom.bin') }));
}

// Each task in use: { task, state, flags, preempt, guest, sp, name }
function state(m) {
  const out = [];
  for (let t = 0; t < 16; t++) {
    const r = m.taskRam[t], st = r[TK.STATE];
    if (st === 0 && t !== 0) continue;
    let name = '';
    for (let i = 0; i < 16 && r[TA_NAME + i]; i++) name += String.fromCharCode(r[TA_NAME + i]);
    out.push({ task: t, state: STATES[st] || '$' + hx(st), flags: r[TK.FLAGS], preempt: r[TK.PREEMPT], busy: r[TK.BUSY], sp: r[TK.SP], name });
  }
  return out;
}

function report(m, lbl) {
  const cpu = m.cpu;
  console.log('--- stopped at cycle ' + cpu.cyc + ' (' + (cpu.cyc / (CLOCK * 1e6)).toFixed(2) + ' s), task ' + hx(m.T, 1) +
    ', page ' + hx(m.W, 1) + ', PC ' + hx(cpu.PC, 4) + ' (' + lbl.at(cpu.PC, m.W) + ')' + (cpu.waiting ? ', idle (WAI)' : '') + (cpu.halted ? ', HALTED: ' + cpu.halted : ''));
  console.log('--- tasks: T STATE  FLAGS PREEMPT BUSY  SP  NAME   (stack low water)');
  for (const s of state(m))
    console.log('           ' + hx(s.task, 1) + ' ' + s.state.padEnd(6) + ' ' + hx(s.flags) + '    ' + hx(s.preempt) + '      ' + hx(s.busy) + '    ' +
      hx(s.sp) + '  ' + s.name.padEnd(12) + ' $' + hx(m.stackLow[s.task]));
  if (m.iOffTop.length) {
    console.log('--- longest IRQs-off stretches (from the first key typed): cycles, from, to');
    for (const [n, from, to] of m.iOffTop.slice(0, 5)) console.log('   ' + String(n).padStart(6) + '  ' + from + ' - ' + to);
  }
  console.log('--- the last instructions: page:PC (label) T A X Y S P');
  for (const [w, t, pc, a, x, y, s, p] of m.trace)
    console.log('   ' + hx(w, 1) + ':' + hx(pc, 4) + ' ' + lbl.at(pc, w).padEnd(24) + ' T' + hx(t, 1) + ' A=' + hx(a) + ' X=' + hx(x) + ' Y=' + hx(y) + ' S=' + hx(s) + ' P=' + hx(p));
}

function interactive(m, opt) {
  const cpu = m.cpu, acia = m.acia, cps = opt.clock * 1e6, stdin = process.stdin, stdout = process.stdout, tty = stdin.isTTY;
  const now = () => Number(process.hrtime.bigint()) / 1e9;
  const limit = opt.cyclesSet ? opt.cycles : Infinity;
  let sent = 0, prefix = false, quit = '', eof = false, stopAt = Infinity, baseT = now(), baseC = cpu.cyc;
  const say = t => stdout.write('\r\n[sim] ' + t + '\r\n');
  const status = () => 'cycle ' + cpu.cyc + ' (' + (cpu.cyc / cps).toFixed(1) + ' s), task ' + hx(m.T, 1) + ', page ' + hx(m.W, 1) +
    ', PC ' + hx(cpu.PC, 4) + (cpu.waiting ? ' (WAI: idle)' : '') + '; tasks: ' + state(m).map(s => hx(s.task, 1) + ' ' + s.name + ' ' + s.state).join(', ');
  const help = () => say('Ctrl-A then: x quit, r reset (the reset button), s status, h this help, Ctrl-A a Ctrl-A.');
  function onKey(b) {
    if (prefix) {
      prefix = false;
      const k = String.fromCharCode(b).toLowerCase();
      if (b === 1) acia.type('\x01');
      else if (k === 'x' || k === 'q') quit = 'quit (Ctrl-A x)';
      else if (k === 'r') { m.hwReset(); say('reset'); }
      else if (k === 's') say(status());
      else help();
      return;
    }
    if (b === 1) { prefix = true; return; }
    if (!tty && b === 0x0A) b = 0x0D;                           // (Piped text: a line ends in CR, as Enter sends)
    acia.type(String.fromCharCode(b));
  }
  if (tty) stdin.setRawMode(true);
  stdin.on('data', buf => { for (const b of buf) if (!(!tty && b === 0x0D)) onKey(b); });
  stdin.on('end', () => { eof = true; });
  stdin.resume();
  const flush = () => {
    if (sent < m.out.length) { stdout.write(m.out.slice(sent)); sent = m.out.length; }
    if (m.out.length > 1 << 16) { m.out = m.out.slice(-1024); sent = m.out.length; }
  };
  const finish = why => { flush(); say('stopped: ' + why + '; ' + status()); if (tty) stdin.setRawMode(false); process.exit(cpu.halted ? 1 : 0); };
  function tick() {
    const t = now();
    if (opt.speed > 0) {
      let target = baseC + (t - baseT) * cps * opt.speed;
      if (target - cpu.cyc > cps * opt.speed * 0.25) { baseT = t; baseC = cpu.cyc; target = cpu.cyc + cps * opt.speed * 0.01; }
      m.run(Math.min(target, limit, stopAt));
    } else while (now() - t < 0.02 && !cpu.halted && cpu.cyc < Math.min(limit, stopAt)) m.run(Math.min(cpu.cyc + 200000, limit, stopAt));
    flush();
    if (eof && !acia.rxQueue.length && stopAt === Infinity) stopAt = cpu.cyc + cps * 3;
    if (cpu.halted) return finish('halted: ' + cpu.halted);
    if (quit) return finish(quit);
    if (cpu.cyc >= limit) return finish('--cycles reached');
    if (cpu.cyc >= stopAt) return finish('end of input');
    setTimeout(tick, opt.speed > 0 ? 4 : 0);
  }
  say('the Hydra\'s serial console.  Ctrl-A x quits, Ctrl-A h for help.');
  tick();
}

// A card from an image file, SD device dev: its blocks read and written in the file
function cardFile(dev, file) {
  const fd = fs.openSync(file, 'r+'), blocks = Math.floor(fs.fstatSync(fd).size / 512);
  return { dev, blocks, file,
    read: n => { const b = Buffer.alloc(512); fs.readSync(fd, b, 0, 512, n * 512); return b; },
    write: (n, b) => { fs.writeSync(fd, Buffer.from(b), 0, 512, n * 512); } };
}

function main(argv) {
  const opt = { cycles: 30000000, speed: 1, clock: CLOCK, pcWatches: [] }, lbl = labels();
  const unescape = s => s.replace(/\\r|\\n/g, '\r').replace(/\\w/g, 'Ā').replace(/\\t/g, '\t');
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i], next = () => argv[++i];
    if (a === '-i' || a === '--interactive') opt.interactive = true;
    else if (a === '--cycles') { opt.cycles = +next(); opt.cyclesSet = true; }
    else if (a === '--input') opt.input = unescape(next());
    else if (a === '--paste') opt.paste = true;
    else if (a === '--speed') opt.speed = +next();
    else if (a === '--clock') opt.clock = +next();
    else if (a === '--acia') opt.acia = next();
    else if (a === '--modules') opt.modules = +next();
    else if (a === '--seed') opt.seed = +next();
    else if (a === '--trace') opt.trace = +next();
    else if (a === '--bios') opt.bios = next();
    else if (a === '--prom') opt.prom = next();
    else if (a === '--sd') { opt.sd = opt.sd || []; opt.sd.push(cardFile(opt.sd.length, next())); }
    else if (a === '--pc-dir') opt.pcDir = next();
    else if (a === '--pc-read-only') opt.pcReadOnly = true;
    else if (a === '--pc-log') opt.pcLog = true;
    else if (a === '--pc-damage') opt.pcDamage = next().split(',').map(s => s.trim().toLowerCase());
    else if (a === '--watch-pc') {
      const w = next(), pc = lbl.byName.has(w) ? lbl.byName.get(w) : parseInt(w.replace(/^\$/, ''), 16);
      if (!(pc >= 0)) { console.error('--watch-pc: ' + w + '?'); process.exit(2); }
      opt.pcWatches.push({ pc, page: lbl.pageOf.get(w) || 0 });
    } else { console.error('run.js: ' + a + '?  (see the top of sim/run.js)'); process.exit(2); }
  }
  if (opt.pcDir) opt.pcHost = createPcHost({ dir: opt.pcDir, readOnly: !!opt.pcReadOnly, damage: opt.pcDamage,
    log: opt.pcLog ? t => (opt.interactive ? process.stdout.write('\r\n[pc] ' + t + '\r\n') : console.log('[pc] ' + t)) : undefined });
  const m = boot(opt);
  if (opt.interactive) return interactive(m, opt);
  m.run(opt.cycles);
  if (opt.pcHost) for (const b of opt.pcHost.flush()) m.out += String.fromCharCode(b);   // (A frame cut short: the output's)
  process.stdout.write(m.out.replace(/\r\n/g, '\n').replace(/\r/g, '\n'));
  if (!m.out.endsWith('\n')) console.log();
  report(m, lbl);
  if (opt.pcHost) console.log('--- ' + opt.pcHost.report() + '; ' + m.acia.pcLost + ' reply byte(s) lost (they came while the last was unread)');
  process.exit(m.cpu.halted ? 1 : 0);
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { boot, labels, state, report, CLOCK };
