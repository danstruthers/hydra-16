#!/usr/bin/env node
// ****************************************************************************
// run.js - the reborn system in the emulator (sim/lib: the Hydra-16 V1 board, cycle by cycle).  It boots
// bin/bios.bin and the paged ROM's chips, bin/prom0.bin ... (node build.js), and either runs for a while and
// reports (the console's output, each task's state, the longest IRQs-off stretches, the stacks' depths), or is the
// serial console, live.
//
// Usage: node sim/run.js [options]
//   -i, --interactive   the terminal is the Hydra's serial console, in real time.  Ctrl-A x quits, Ctrl-A r resets,
//                       Ctrl-A s shows the state, Ctrl-A b stops it (the monitor: below), Ctrl-A v shows the Vera X's
//                       screen as text, Ctrl-A p saves it as a PNG, Ctrl-A h helps
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
//   --watch ADDR[:T]    log each write to ADDR ($0000-$7FFF: hex, or a module's label, MODULE:LABEL), by any task or
//                       task T's; --watch-read ADDR[:T] each read
//   --trace-calls [L]   log each system call a program makes, by name, its registers (r0's string, if it's one), and
//                       what it gives back; L: only these, a list (OPEN,READ,t1: a name, or tN, task N's)
//   --break SPEC        stop before the instruction at SPEC: a kernel label (K_OPEN), a module's (rc:main: while the
//                       task has its bank), page:ADDR (0:E000), or ADDR (any page or bank).  The report says where;
//                       with -i, the monitor
//   --log FILE          the log's lines (the watches, the call trace, the marks) into FILE, not the terminal
// The monitor (-i: Ctrl-A b, or a break): the Hydra stopped, a command line of its own: c continue; n [N] the next N
// instructions, each shown; r the registers; t the tasks; m ADDR [N] [T] N bytes (64) at ADDR in task T's view (the
// task running's); b [SPEC] a break (none: the list); d N the Nth break gone; w ADDR[:T] a write watch; x quit
//   --bios FILE, --prom FILE   other images (--prom: the whole paged ROM, its sockets' images one after another)
//   --sd FILE           a card image (sim/tools/hydrafs.js makes them), SD device 0, then 1 ...: read and
//                       written in the file itself, as the Hydra reads and writes it
//   --pc-dir DIR        /pc: the PC tool's part (sim/tools/hydrapc.js) is played here, serving the folder DIR:
//                       the frames the Hydra sends for /pc are answered, at the line's rate (sim/lib/pchost.js)
//   --pc-read-only      /pc can't be changed: its writes, creates, removes and renames are refused
//   --pc-log            list /pc's requests as they're served (opens, creates, removes, renames, errors)
//   --pc-damage F[,F...]  damage /pc's frames on the line, to try the resends: qN the Nth frame the Hydra sends, rN
//                       the Nth reply (a byte of its body gets bit 6 flipped)
//   --vera [V]          a Vera X card in slot 0 (sim/lib/vera.js): the VERA, its gateware version V (47.0.2, the X16
//                       community's, by default; 0.9: fvdhoef's, without FX's registers or the version)
//   --vera-config MS    the VERA's FPGA configuring itself after power-up and a reset: MS milliseconds (100)
//   --screen            after the report, the VERA's text layer as text (its characters as ISO-8859-1)
//   --frame-png FILE    at the end, the VERA's screen as a PNG (640 x 480); with -i, Ctrl-A p's file (screen-N.png)
//   --view [PORT]       with -i: the VERA's screen live in a browser, at http://localhost:PORT (8016) (sim/view.js)
//   --sound [PORT]      with -i: the sound (the YM2151's, and the Vera X's PSG and PCM with --vera: sim/lib/audio.js)
//                       in a browser, at http://localhost:PORT (8016; --view's page, if there's one), its Sound
//                       button to hear it, some 0.15 s behind; in time at --speed 1
//   --wav FILE          the sound into FILE (48,000 stereo 16-bit samples a second), as the Hydra's time passes
// From Node: boot(opt) gives the machine; labels() the kernel's labels; state(m) each task's state.
'use strict';
const fs = require('fs');
const path = require('path');
const { createMachine, romBank } = require('./lib/machine.js');
const { createPcHost } = require('./lib/pchost.js');
const { encodePng } = require('./lib/png.js');

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

// The paged ROM's chips as build.js writes them (bin/prom0.bin, prom1.bin ...: 512K each, socket 0 on), one image
function chips() {
  const out = [];
  for (let k = 0; fs.existsSync(path.join(ROOT, 'bin', 'prom' + k + '.bin')); k++) out.push(fs.readFileSync(path.join(ROOT, 'bin', 'prom' + k + '.bin')));
  if (!out.length) throw new Error('no bin/prom0.bin: node build.js');
  return Buffer.concat(out);
}

// The machine, booted from the images.  opt: createMachine's (sim/lib/machine.js), and bios, prom (files or bytes;
// the paged ROM a whole image, the sockets' in order; none: the chips' images)
function boot(opt = {}) {
  const img = (v, f) => v instanceof Uint8Array ? v : fs.readFileSync(v || path.join(ROOT, 'bin', f));
  const prom = opt.prom instanceof Uint8Array ? opt.prom : opt.prom ? fs.readFileSync(opt.prom) : chips();
  return createMachine(Object.assign({
    modules: 2, sharedU: 16, aciaLine: 1, stuckIrq: -1, acia: 'rockwell', clock: CLOCK, trace: 25,
    pcWatches: [], watches: [], marks: [], log: s => console.log('[sim] ' + s),
  }, opt, { osrom: img(opt.bios, 'bios.bin'), pagedrom: prom }));
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

// The calls' names by their jump table slots, and the errors' by their codes (obj/gen/api.json: tools/apigen.js)
function api() {
  const f = path.join(ROOT, 'obj', 'gen', 'api.json');
  const a = fs.existsSync(f) ? JSON.parse(fs.readFileSync(f, 'utf8')) : { calls: [], errors: [] };
  return { calls: new Map(a.calls.map(c => [c.addr, c.name])), errNames: new Map(a.errors.map(e => [e.code, e.name])) };
}

// The module directory (paged ROM bank 0 at $A200, the image's): name -> { bank, banks }
function modDir(prom) {
  const at = a => prom[romBank(0) * 0x4000 + ((a - 0xA000) ^ 0x2000)], out = new Map();
  for (let e = 0; e < at(0xA205); e++) {
    const b = 0xA208 + e * 16;
    let name = '';
    for (let i = 0; i < 12 && at(b + 4 + i); i++) name += String.fromCharCode(at(b + 4 + i));
    out.set(name, { bank: at(b), banks: at(b + 1) });
  }
  return out;
}

// A module's label (obj/modules/NAME/NAME.lbl, or obj/tests/NAME/: ld65's -Ln): its address, or undefined
function modLabel(mod, label) {
  for (const d of ['modules', 'tests']) {
    const f = path.join(ROOT, 'obj', d, mod, mod + '.lbl');
    if (!fs.existsSync(f)) continue;
    for (const line of fs.readFileSync(f, 'latin1').split(/\r?\n/)) {
      const m = line.match(/^al ([0-9A-F]+) \.(\w+)$/);
      if (m && m[2] === label) return parseInt(m[1], 16);
    }
  }
  return undefined;
}

// A break's place: a kernel label, MODULE:LABEL (with its banks), PAGE:ADDR, or ADDR.  { pc, page, bank, banks }, or
// null
function breakSpec(spec, lbl, prom) {
  let m;
  if (lbl.byName.has(spec)) return { pc: lbl.byName.get(spec), page: lbl.pageOf.get(spec) || 0, bank: -1 };
  if ((m = spec.match(/^([0-9a-f]):\$?([0-9a-f]{1,4})$/i))) return { pc: parseInt(m[2], 16), page: parseInt(m[1], 16), bank: -1 };
  if ((m = spec.match(/^\$?([0-9a-f]{1,4})$/i))) return { pc: parseInt(m[1], 16), page: -1, bank: -1 };
  if ((m = spec.match(/^(\w+):(\w+)$/))) {
    const pc = modLabel(m[1], m[2]), md = modDir(prom).get(m[1]);
    if (pc === undefined) return null;
    return pc >= 0xA000 && pc < 0xE000 && md ? { pc, page: -1, bank: md.bank, banks: md.banks } : { pc, page: -1, bank: -1 };
  }
  return null;
}

// A watch's place: ADDR or MODULE:LABEL, then :T for one task's.  { addr, task }, or null
function watchSpec(spec) {
  let m = spec.match(/^(.*?)(?::([0-9a-f]))?$/i), task = -1, where = spec;
  if (m && m[2] !== undefined && !/^\w+:\w+$/.test(spec) || /^\w+:\w+:[0-9a-f]$/i.test(spec)) { where = m[1]; task = parseInt(m[2], 16); }
  let addr;
  if ((m = where.match(/^\$?([0-9a-f]{1,4})$/i))) addr = parseInt(m[1], 16);
  else if ((m = where.match(/^(\w+):(\w+)$/))) addr = modLabel(m[1], m[2]);
  return addr >= 0 && addr < 0x8000 ? { addr, task } : null;
}

// The VERA's text layer as lines, a heading first (--screen, Ctrl-A v)
function screenLines(m) {
  if (!m.vera) return ['--- no Vera X (--vera)'];
  const c = m.vera.cells();
  if (!c) return ['--- the screen: ' + (m.vera.ready ? 'no text layer shown' : 'the VERA is configuring')];
  return ['--- the screen (layer ' + c.layer + ', ' + c.cols + ' x ' + c.rows + '):', ...m.vera.text().map(l => '   |' + l)];
}

// The VERA's screen into a PNG file
function savePng(m, file) {
  const f = m.vera.frame();
  fs.writeFileSync(file, encodePng({ width: 640, height: 480, pixels: f.pixels, rgb: f.rgb }, require('zlib').deflateSync));
}

function interactive(m, opt) {
  const cpu = m.cpu, acia = m.acia, cps = opt.clock * 1e6, stdin = process.stdin, stdout = process.stdout, tty = stdin.isTTY;
  const now = () => Number(process.hrtime.bigint()) / 1e9;
  const limit = opt.cyclesSet ? opt.cycles : Infinity;
  let sent = 0, prefix = false, quit = '', eof = false, stopAt = Infinity, baseT = now(), baseC = cpu.cyc;
  let mon = false, line = '';                                   // (The monitor: stopped, and its command line)
  const say = t => stdout.write('\r\n[sim] ' + t + '\r\n');
  const status = () => 'cycle ' + cpu.cyc + ' (' + (cpu.cyc / cps).toFixed(1) + ' s), task ' + hx(m.T, 1) + ', page ' + hx(m.W, 1) +
    ', PC ' + hx(cpu.PC, 4) + (cpu.waiting ? ' (WAI: idle)' : '') + '; tasks: ' + state(m).map(s => hx(s.task, 1) + ' ' + s.name + ' ' + s.state).join(', ');
  const help = () => say('Ctrl-A then: x quit, r reset (the reset button), s status, b the monitor, v the screen as text, p the screen as a PNG, h this help, Ctrl-A a Ctrl-A.');
  let pngs = 0;
  const lbl = opt.lbl;
  const regs = () => 'T' + hx(m.T, 1) + ' ' + hx(m.W, 1) + ':' + hx(cpu.PC, 4) + ' ' + lbl.at(cpu.PC, m.W).padEnd(24) + ' A=' + hx(cpu.A) +
    ' X=' + hx(cpu.X) + ' Y=' + hx(cpu.Y) + ' S=' + hx(cpu.S) + ' P=' + hx(cpu.P) + ' U=' + hx(m.U, 1) + ' RAM=' + hx(m.taskRam[m.T][0]) +
    ' ROM=' + hx(m.taskRam[m.T][1]) + ' cycle ' + cpu.cyc;
  const monSay = t => stdout.write(t.replace(/\n/g, '\r\n') + '\r\n');
  function monitor(why) {                                       // The Hydra stopped: the monitor's prompt
    flush();
    mon = true; line = '';
    say('the monitor (' + why + '): c continue, n [N] step, r registers, t tasks, m ADDR [N] [T], b [SPEC], d N, w ADDR[:T], x quit');
    monSay(regs());
    stdout.write('mon> ');
  }
  function command(text) {
    const w = text.trim().split(/\s+/), c = w[0] || '';
    if (c === 'c') { mon = false; m.skipBreak = true; baseT = now(); baseC = cpu.cyc; say('continued'); setTimeout(tick, 0); return; }
    if (c === 'x' || c === 'q') { quit = 'quit (the monitor)'; mon = false; setTimeout(tick, 0); return; }
    if (c === 'n') {
      const n = Math.max(1, +w[1] || 1);
      for (let i = 0; i < n && !cpu.halted; i++) {
        m.skipBreak = true;
        m.run(cpu.cyc + 1);
        if (n <= 32 || i >= n - 4) monSay(regs());
      }
      flush();
    } else if (c === 'r') monSay(regs());
    else if (c === 't') monSay(status());
    else if (c === 'm') {
      const a = parseInt((w[1] || '').replace(/^\$/, ''), 16), n = Math.min(+w[2] || 64, 1024), t = w[3] !== undefined ? parseInt(w[3], 16) & 15 : m.T;
      if (!(a >= 0)) monSay('m ADDR [N] [T]');
      else for (let o = 0; o < n; o += 16) {
        let hex = '', asc = '';
        for (let i = 0; i < 16 && o + i < n; i++) {
          const x = (a + o + i) & 0xFFFF, v = x < 0x8000 ? m.taskRam[t][x] : m.rd(x);
          hex += hx(v) + ' '; asc += v >= 0x20 && v < 0x7F ? String.fromCharCode(v) : '.';
        }
        monSay(hx((a + o) & 0xFFFF, 4) + '  ' + hex.padEnd(48) + ' ' + asc);
      }
    } else if (c === 'b') {
      if (!w[1]) opt.breaks.forEach((b, i) => monSay(i + ': ' + (b.page >= 0 ? hx(b.page, 1) + ':' : '') + hx(b.pc, 4) + ' ' + (b.spec || '')));
      else { const b = breakSpec(w[1], lbl, opt.promImage); if (b) { b.spec = w[1]; opt.breaks.push(b); monSay('break ' + (opt.breaks.length - 1)); } else monSay('b: ' + w[1] + '?'); }
    } else if (c === 'd') { const i = +w[1]; if (i >= 0 && i < opt.breaks.length) opt.breaks.splice(i, 1); else monSay('d N'); }
    else if (c === 'w') { const x = watchSpec(w[1] || ''); if (x) { opt.watches.push(x); monSay('watching $' + hx(x.addr, 4)); } else monSay('w ADDR[:T]'); }
    else if (c) monSay(c + '?');
    stdout.write('mon> ');
  }
  function onMonKey(b) {
    if (b === 0x0D || b === 0x0A) { stdout.write('\r\n'); const t = line; line = ''; command(t); }
    else if (b === 0x08 || b === 0x7F) { if (line) { line = line.slice(0, -1); stdout.write('\b \b'); } }
    else if (b === 0x03) { line = ''; stdout.write('^C\r\nmon> '); }
    else if (b >= 0x20 && b < 0x7F) { line += String.fromCharCode(b); stdout.write(String.fromCharCode(b)); }
  }
  function onKey(b) {
    if (mon) return onMonKey(b);
    if (prefix) {
      prefix = false;
      const k = String.fromCharCode(b).toLowerCase();
      if (b === 1) acia.type('\x01');
      else if (k === 'x' || k === 'q') quit = 'quit (Ctrl-A x)';
      else if (k === 'r') { m.hwReset(); say('reset'); }
      else if (k === 's') say(status());
      else if (k === 'b') monitor('Ctrl-A b');
      else if (k === 'v') { flush(); stdout.write('\r\n' + screenLines(m).join('\r\n') + '\r\n'); }
      else if (k === 'p') { if (!m.vera) say('no Vera X (--vera)'); else { const f = opt.framePng || 'screen-' + (++pngs) + '.png'; savePng(m, f); say('the screen: ' + f); } }
      else help();
      return;
    }
    if (b === 1) { prefix = true; return; }
    if (!tty && b === 0x0A) b = 0x0D;                           // (Piped text: a line ends in CR, as Enter sends)
    acia.type(String.fromCharCode(b));
  }
  if (tty) stdin.setRawMode(true);
  stdin.on('data', buf => { for (const b of buf) if (!(!tty && b === 0x0D)) onKey(b); });
  stdin.on('end', () => { eof = true; if (mon) { mon = false; quit = 'end of input (the monitor)'; setTimeout(tick, 0); } });
  stdin.resume();
  const flush = () => {
    if (sent < m.out.length) { stdout.write(m.out.slice(sent)); sent = m.out.length; }
    if (m.out.length > 1 << 16) { m.out = m.out.slice(-1024); sent = m.out.length; }
  };
  const finish = why => { flush(); say('stopped: ' + why + '; ' + status()); if (tty) stdin.setRawMode(false); process.exit(cpu.halted ? 1 : 0); };
  function tick() {
    if (mon) return;                                            // (Stopped: the monitor's)
    const t = now();
    if (opt.speed > 0) {
      let target = baseC + (t - baseT) * cps * opt.speed;
      if (target - cpu.cyc > cps * opt.speed * 0.25) { baseT = t; baseC = cpu.cyc; target = cpu.cyc + cps * opt.speed * 0.01; }
      m.run(Math.min(target, limit, stopAt));
    } else while (now() - t < 0.02 && !cpu.halted && cpu.cyc < Math.min(limit, stopAt)) m.run(Math.min(cpu.cyc + 200000, limit, stopAt));
    flush();
    if (m.breakHit) { const b = m.breakHit; m.breakHit = null; return monitor('break ' + opt.breaks.indexOf(b) + (b.spec ? ': ' + b.spec : '')); }
    if (eof && !acia.rxQueue.length && stopAt === Infinity) stopAt = cpu.cyc + cps * 3;
    if (cpu.halted) return finish('halted: ' + cpu.halted);
    if (quit) return finish(quit);
    if (cpu.cyc >= limit) return finish('--cycles reached');
    if (cpu.cyc >= stopAt) return finish('end of input');
    setTimeout(tick, opt.speed > 0 ? 4 : 0);
  }
  if (opt.view || opt.soundPort) {
    const port = opt.view || opt.soundPort;
    require('./view.js').startView(m, port, { screen: !!opt.view, sound: !!opt.soundPort });
    say((opt.view ? 'the screen' : 'the sound') + ': http://localhost:' + port + (opt.soundPort ? ' (its Sound button)' : ''));
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

// A WAV file of the sound (48,000 stereo 16-bit samples a second), written as it comes: write(samples), and close()
// to give its header the length
function wavFile(file, rate) {
  const fd = fs.openSync(file, 'w');
  let bytes = 0;
  const header = () => {
    const h = Buffer.alloc(44);
    h.write('RIFF', 0); h.writeUInt32LE(36 + bytes, 4); h.write('WAVEfmt ', 8); h.writeUInt32LE(16, 16);
    h.writeUInt16LE(1, 20); h.writeUInt16LE(2, 22); h.writeUInt32LE(rate, 24); h.writeUInt32LE(rate * 4, 28);
    h.writeUInt16LE(4, 32); h.writeUInt16LE(16, 34); h.write('data', 36); h.writeUInt32LE(bytes, 40);
    fs.writeSync(fd, h, 0, 44, 0);
  };
  header();
  return {
    write: samples => { const b = Buffer.from(samples.buffer, samples.byteOffset, samples.byteLength); fs.writeSync(fd, b, 0, b.length, 44 + bytes); bytes += b.length; },
    close: () => { header(); fs.closeSync(fd); },
  };
}

function main(argv) {
  const opt = { cycles: 30000000, speed: 1, clock: CLOCK, pcWatches: [], watches: [], readWatches: [], breaks: [] }, lbl = labels();
  const breakArgs = [];
  opt.lbl = lbl;
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
    else if (a === '--vera') {
      opt.vera = Object.assign(opt.vera || {}, { version: [47, 0, 2] });
      if (/^\d+(\.\d+)*$/.test(argv[i + 1] || '')) { const v = next(); opt.vera.version = v === '0.9' ? null : v.split('.').map(Number).concat([0, 0]).slice(0, 3); }
    } else if (a === '--vera-config') opt.veraConfigMs = +next();
    else if (a === '--screen') opt.screen = true;
    else if (a === '--frame-png') opt.framePng = next();
    else if (a === '--view') opt.view = /^\d+$/.test(argv[i + 1] || '') ? +next() : 8016;
    else if (a === '--sound') { opt.sound = true; opt.soundPort = /^\d+$/.test(argv[i + 1] || '') ? +next() : 8016; }
    else if (a === '--wav') { opt.sound = true; opt.wav = next(); }
    else if (a === '--watch' || a === '--watch-read') {
      const w = watchSpec(next() || '');
      if (!w) { console.error(a + ': ADDR[:T] ($0000-$7FFF)?'); process.exit(2); }
      (a === '--watch' ? opt.watches : opt.readWatches).push(w);
    } else if (a === '--trace-calls') {
      const { calls, errNames } = api();
      opt.calls = calls; opt.errNames = errNames;
      if (argv[i + 1] && !argv[i + 1].startsWith('-')) opt.callFilter = new Set(next().split(',').map(x => x.trim()).map(x => /^t[0-9a-f]$/i.test(x) ? 't' + parseInt(x.slice(1), 16) : x.toUpperCase()));
    } else if (a === '--break') breakArgs.push(next());
    else if (a === '--log') { const fd = fs.openSync(next(), 'w'); opt.log = t => fs.writeSync(fd, '[sim] ' + t + '\n'); }
    else if (a === '--watch-pc') {
      const w = next(), pc = lbl.byName.has(w) ? lbl.byName.get(w) : parseInt(w.replace(/^\$/, ''), 16);
      if (!(pc >= 0)) { console.error('--watch-pc: ' + w + '?'); process.exit(2); }
      opt.pcWatches.push({ pc, page: lbl.pageOf.get(w) || 0 });
    } else { console.error('run.js: ' + a + '?  (see the top of sim/run.js)'); process.exit(2); }
  }
  if (opt.pcDir) opt.pcHost = createPcHost({ dir: opt.pcDir, readOnly: !!opt.pcReadOnly, damage: opt.pcDamage,
    log: opt.pcLog ? t => (opt.interactive ? process.stdout.write('\r\n[pc] ' + t + '\r\n') : console.log('[pc] ' + t)) : undefined });
  opt.promImage = opt.prom ? fs.readFileSync(opt.prom) : chips();
  if (opt.veraConfigMs !== undefined) { if (!opt.vera) { console.error('--vera-config: with --vera'); process.exit(2); } opt.vera.configCycles = Math.round(opt.veraConfigMs * opt.clock * 1e3); }
  if ((opt.screen || opt.framePng || opt.view) && !opt.vera) { console.error('--screen, --frame-png and --view: with --vera'); process.exit(2); }
  if (opt.soundPort && !opt.interactive) { console.error('--sound: with -i (--wav FILE keeps it in a file)'); process.exit(2); }
  if (opt.view && opt.soundPort) opt.soundPort = opt.view;    // (One page for both)
  for (const spec of breakArgs) {
    const b = breakSpec(spec, lbl, opt.promImage);
    if (!b) { console.error('--break: ' + spec + '?'); process.exit(2); }
    b.spec = spec; opt.breaks.push(b);
  }
  if (opt.callFilter) for (const x of opt.callFilter) if (!x.startsWith('t') && ![...opt.calls.values()].includes(x)) { console.error('--trace-calls: no call ' + x); process.exit(2); }
  const m = boot(Object.assign({}, opt, { prom: opt.promImage }));
  if (opt.wav) {
    const w = wavFile(opt.wav, require('./lib/audio.js').RATE);
    m.audio.on(w.write);
    process.on('exit', () => w.close());
  }
  if (opt.interactive) return interactive(m, opt);
  if (opt.sound) while (m.cpu.cyc < opt.cycles && !m.cpu.halted && !m.breakHit) m.run(Math.min(opt.cycles, m.cpu.cyc + 2000000));   // (The sound: given out as it goes)
  else m.run(opt.cycles);
  if (m.breakHit) console.log('--- break ' + opt.breaks.indexOf(m.breakHit) + ': ' + m.breakHit.spec);
  if (opt.pcHost) for (const b of opt.pcHost.flush()) m.out += String.fromCharCode(b);   // (A frame cut short: the output's)
  process.stdout.write(m.out.replace(/\r\n/g, '\n').replace(/\r/g, '\n'));
  if (!m.out.endsWith('\n')) console.log();
  report(m, lbl);
  if (opt.screen) console.log(screenLines(m).join('\n'));
  if (opt.framePng) { savePng(m, opt.framePng); console.log('--- the screen: ' + opt.framePng); }
  if (opt.pcHost) console.log('--- ' + opt.pcHost.report() + '; ' + m.acia.pcLost + ' reply byte(s) lost (they came while the last was unread)');
  process.exit(m.cpu.halted ? 1 : 0);
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { boot, labels, state, report, CLOCK };
