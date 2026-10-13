#!/usr/bin/env node
// ****************************************************************************
// hydrapc.js - the PC tool for /pc: a terminal on the Hydra-16's serial port that also serves a folder on this PC to
// it, so the Hydra sees the folder at /pc (docs/design/plans/PC.md).  What the Hydra prints shows here and the keys typed go
// to it, as with any terminal; the frames /pc's requests and replies travel in (lib/pcproto.js) go between those
// bytes, and don't show.  The folder's files are served as tools/pcfs.js says; nothing outside it is reached.  It
// serves either system: the old one (the protocol's version 1) or HydraOS (version 2), as the Hydra's attach says.
//
// Usage: node hydrapc.js PORT FOLDER [options]
//          PORT     the serial port (COM3, /dev/ttyUSB0 ...); node hydrapc.js --list shows them
//          FOLDER   the folder the Hydra sees at /pc
//   --read-only     the Hydra can't change the folder: its writes, creates, removes and renames are refused
//   --baud N        the line's rate (default 9600, the Hydra's at boot)
//   --log           list the requests as they're served (opens, creates, removes, renames, errors) on stderr
//   --list          list the serial ports, and stop
//   --no-size       don't tell the Hydra this window's size
//   --win32-input   Windows Terminal's win32-input-mode: each key's record made into an xterm's bytes here
//                   (lib/win32in.js), so Ctrl-Tab and Ctrl-Shift-Tab reach the Hydra (ESC [ 9 ; 5 u, 6), its
//                   windows' keys.  Windows Terminal keeps Ctrl-Tab for its own tabs: take that binding out of its
//                   settings (its actions' nextTab and prevTab keys)
// Keys: Ctrl-A is this tool's prefix: Ctrl-A x quits, Ctrl-A l turns the log on or off, Ctrl-A s says /pc's counts so
// far (requests, attaches, frames from the Hydra that came damaged, replies asked for again: said at the end too),
// Ctrl-A Ctrl-A sends a Ctrl-A.  A typed $1E (Ctrl-^, /pc's frame mark) goes as $1E $1F, which the Hydra takes as the key.
// The window's size: told to the Hydra as xterm answers ESC [ 18 t (ESC [ 8 ; rows ; columns t), as the tool starts
// and whenever the window changes, so the Hydra's windows are its size; and the Hydra's ESC [ 18 t (its console
// asks as it starts, and consctl's terminal size) is answered here, not shown.
//
// Flow control: HydraOS's console sends XOFF when it has more keys than it can keep, and XON when it has read them
// (serctl's x1: XON/XOFF).  They aren't shown: after an XOFF the keys typed or pasted here wait for the XON (the
// /pc replies go on: the Hydra is waiting for those).  A paste goes out a few bytes at a time (CHUNK, then the
// port's drain and a pause: 2,000 keys a second at most, more than the Hydra reads), so few are on their way when the
// XOFF comes.  If no XON comes in HELD_MS the keys go on: an XOFF in a program's own output (a binary file shown)
// mustn't lock the keyboard for good.  Ctrl-C and Ctrl-\ never wait: the Hydra acts on them as they come, and the
// program they stop may be the one that isn't reading.
//
// It needs the serialport package: npm install, in reborn/sim (its package.json).  It asserts RTS (and DTR) once the port
// is open: the Hydra's ACIA sends only while its ~CTS is asserted, which is the PC's RTS (DE-9 pin 7), and serialport
// leaves RTS off without hardware flow control.  (It doesn't wait on CTS itself: the Hydra's ACIA can't raise its RTS
// without turning its own transmitter off, so its flow control is XON/XOFF.)
// ****************************************************************************
'use strict';
const path = require('path');
const P = require('../lib/pcproto.js');
const W32 = require('../../../base/sim/lib/win32in.js');
const { createPcFs } = require('./pcfs.js');

const QUIET_MS = 100;                                         // A frame that stops this long isn't one: its bytes show
const CHUNK = 16, CHUNK_MS = 8;                               // The keys' way out: this many, then a pause (a paste)
const HELD_MS = 10000;                                        // After an XOFF with no XON this long, the keys go on

function usage(msg) {
  if (msg) console.error('hydrapc: ' + msg);
  console.error('Usage: node hydrapc.js PORT FOLDER [--read-only] [--baud N] [--log] [--no-size] [--win32-input]   (node hydrapc.js --list: the ports)');
  process.exit(1);
}

const opt = { baud: 9600, readOnly: false, log: false, list: false, size: true, win32: false, args: [] };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const a = argv[i];
  if (a === '--read-only') opt.readOnly = true;
  else if (a === '--log') opt.log = true;
  else if (a === '--list') opt.list = true;
  else if (a === '--no-size') opt.size = false;
  else if (a === '--win32-input') opt.win32 = true;
  else if (a === '--baud') { opt.baud = +argv[++i]; if (!(opt.baud > 0)) usage('--baud N'); }
  else if (a.startsWith('--')) usage('unknown option ' + a);
  else opt.args.push(a);
}

let SerialPort;
try { ({ SerialPort } = require('serialport')); }
catch (e) { console.error('hydrapc: the serialport package is missing: run npm install in ' + path.join(__dirname, '..')); process.exit(1); }

if (opt.list) {
  SerialPort.list().then(ports => {
    if (!ports.length) console.log('No serial ports');
    for (const p of ports) console.log(p.path + (p.manufacturer ? '  ' + p.manufacturer : '') + (p.friendlyName ? '  ' + p.friendlyName : ''));
  });
} else {
  if (opt.args.length !== 2) usage();
  run(opt.args[0], opt.args[1]);
}

function run(portName, folder) {
  const stdin = process.stdin, stdout = process.stdout, tty = stdin.isTTY;
  let logOn = opt.log, naks = 0;
  const log = t => { if (logOn) process.stderr.write('[pc] ' + t + '\r\n'); };
  const counts = () => { const s = fsrv.stats; return '/pc: ' + s.requests + ' request(s), ' + s.attaches + ' attach(es), ' + naks + ' frame(s) from the Hydra damaged (asked again), ' + s.repeats + ' repl' + (s.repeats === 1 ? 'y' : 'ies') + ' lost (asked for again)'; };
  const say = t => stdout.write('\r\n[hydrapc] ' + t + '\r\n');
  let fsrv;
  try { fsrv = createPcFs({ root: folder, readOnly: opt.readOnly, log }); }
  catch (e) { usage(e.message); }

  const port = new SerialPort({ path: portName, baudRate: opt.baud, dataBits: 8, parity: 'none', stopBits: 1 }, err => {
    if (err) { console.error('hydrapc: ' + portName + ': ' + err.message); process.exit(1); }
    port.set({ rts: true, dtr: true }, e => {                  // (The Hydra sends only while its CTS, our RTS, is on)
      if (e) say('RTS and DTR not set (' + e.message + '): the Hydra may not send');
      say('the Hydra on ' + portName + ' at ' + opt.baud + ' baud; /pc is ' + path.resolve(folder) + (opt.readOnly ? ' (read-only)' : '') +
        '.  Ctrl-A x quits, Ctrl-A l the log, Ctrl-A s the /pc counts.');
      tellSize();
    });
  });

  // The window's size, told (ESC [ 8 ; rows ; columns t); the Hydra's asks (ESC [ 18 t) taken out of what's shown
  const tellSize = () => { if (opt.size && stdout.isTTY && stdout.columns && stdout.rows && port.isOpen) port.write('\x1b[8;' + stdout.rows + ';' + stdout.columns + 't'); };
  stdout.on('resize', tellSize);
  const ASK = [0x1B, 0x5B, 0x31, 0x38, 0x74];
  let askAt = 0;
  const shown = bytes => {
    const out = [];
    for (const b of bytes) {
      if (b === ASK[askAt]) { if (++askAt === ASK.length) { askAt = 0; tellSize(); } continue; }
      if (askAt) { out.push(...ASK.slice(0, askAt)); askAt = b === ASK[0] ? 1 : 0; if (askAt) continue; }
      out.push(b);
    }
    if (out.length) stdout.write(Buffer.from(out));
  };

  // The keys' way to the Hydra: a queue, a chunk at a time, held from the Hydra's XOFF to its XON
  const keyQ = [];
  let sending = false, held = false, heldTimer = null;
  function pumpKeys() {
    if (sending || held || !keyQ.length || !port.isOpen) return;
    sending = true;
    port.write(Buffer.from(keyQ.splice(0, CHUNK)));
    port.drain(() => setTimeout(() => { sending = false; pumpKeys(); }, keyQ.length ? CHUNK_MS : 0));
  }
  const sendKeys = bytes => { keyQ.push(...bytes); pumpKeys(); };
  const flow = b => {
    clearTimeout(heldTimer);
    held = b === P.XOFF;
    if (held) heldTimer = setTimeout(() => { held = false; pumpKeys(); }, HELD_MS);
    else pumpKeys();
  };

  // The Hydra's bytes: frames to the file server, its XOFF and XON to the keys' queue, the rest to the screen
  const reader = P.createReader({ types: [P.T_ATTACH, P.T_REQ], onFlow: flow,
    onFrame: f => port.write(Buffer.from(P.encode(P.T_REPLY, f.tag, f.type === P.T_ATTACH ? fsrv.attach(f.payload) : fsrv.request(f.tag, f.payload)))),
    onBad: f => { naks++; log('a frame from the Hydra came damaged (tag ' + f.tag + '): asked for it again'); port.write(Buffer.from(P.encode(P.T_NAK, f.tag))); } });
  let quiet = null;
  port.on('data', buf => {
    const out = [];
    for (const b of buf) for (const c of reader.push(b)) out.push(c);
    shown(out);
    clearTimeout(quiet);
    if (reader.inFrame()) quiet = setTimeout(() => shown(reader.flush()), QUIET_MS);
  });
  port.on('close', () => finish('the port closed'));
  port.on('error', e => finish(e.message));

  // The keys: to the Hydra (Ctrl-A: this tool's)
  let prefix = false;
  function onKey(b) {
    if (prefix) {
      prefix = false;
      const k = String.fromCharCode(b).toLowerCase();
      if (b === 1) sendKeys([1]);
      else if (k === 'x' || k === 'q') finish('quit');
      else if (k === 'l') { logOn = !logOn; say('log ' + (logOn ? 'on' : 'off')); }
      else if (k === 's') say(counts());
      else say('Ctrl-A then: x quit, l the log on or off, s the /pc counts, Ctrl-A a Ctrl-A');
      return;
    }
    if (b === 1) { prefix = true; return; }
    if (b === 0x03 || b === 0x1C) { port.write(Buffer.from([b])); return; }   // (Ctrl-C, Ctrl-\: at once, ahead of the keys waiting)
    if (!tty && b === 0x0A) b = 0x0D;                           // (Piped text: a line ends in CR, as Enter sends)
    sendKeys(b === P.MARK ? [P.MARK, P.ESC] : [b]);
  }
  if (tty) stdin.setRawMode(true);
  const w32 = opt.win32 && tty && stdout.isTTY ? W32.createWin32Input(bytes => { for (const b of bytes) onKey(b); }) : null;
  let w32wait = null;
  if (w32) stdout.write(W32.ENABLE);
  stdin.on('data', buf => {
    if (w32) {                                                // (A record part-way: the rest soon, or it's bytes)
      for (const b of buf) w32.push(b);
      clearTimeout(w32wait);
      if (w32.pending()) w32wait = setTimeout(() => w32.flush(), 50);
      return;
    }
    for (const b of buf) if (!(!tty && b === 0x0D)) onKey(b);
  });
  stdin.resume();

  let done = false;
  function finish(why) {
    if (done) return;
    done = true;
    say(why);
    if (fsrv.stats.requests || fsrv.stats.attaches || naks) say(counts());
    fsrv.close();
    if (w32) stdout.write(W32.DISABLE);
    if (tty) stdin.setRawMode(false);
    if (port.isOpen) port.close(() => process.exit(0)); else process.exit(0);
  }
}
