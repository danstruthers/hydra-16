#!/usr/bin/env node
// ****************************************************************************
// hydrapc.js - the PC tool for /pc: a terminal on the Hydra-16's serial port that also serves a folder on this PC to
// it, so the Hydra sees the folder at /pc (docs/plans/PC.md).  What the Hydra prints shows here and the keys typed go
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
// Keys: Ctrl-A is this tool's prefix: Ctrl-A x quits, Ctrl-A l turns the log on or off, Ctrl-A Ctrl-A sends a
// Ctrl-A.  A typed $1E (Ctrl-^, /pc's frame mark) goes as $1E $1F, which the Hydra takes as the key.
// The window's size: told to the Hydra as xterm answers ESC [ 18 t (ESC [ 8 ; rows ; columns t), as the tool starts
// and whenever the window changes, so the Hydra's windows are its size; and the Hydra's ESC [ 18 t (its console
// asks as it starts, and consctl's terminal size) is answered here, not shown.
//
// It needs the serialport package: npm install, in reborn/sim (its package.json).  It asserts RTS (and DTR) once the port
// is open: the Hydra's ACIA sends only while its ~CTS is asserted, which is the PC's RTS (DE-9 pin 7), and serialport
// leaves RTS off without hardware flow control.  (It doesn't wait on CTS itself: the Hydra can't hold the PC back.)
// ****************************************************************************
'use strict';
const path = require('path');
const P = require('../lib/pcproto.js');
const { createPcFs } = require('./pcfs.js');

const QUIET_MS = 100;                                         // A frame that stops this long isn't one: its bytes show

function usage(msg) {
  if (msg) console.error('hydrapc: ' + msg);
  console.error('Usage: node hydrapc.js PORT FOLDER [--read-only] [--baud N] [--log] [--no-size]   (node hydrapc.js --list: the ports)');
  process.exit(1);
}

const opt = { baud: 9600, readOnly: false, log: false, list: false, size: true, args: [] };
const argv = process.argv.slice(2);
for (let i = 0; i < argv.length; i++) {
  const a = argv[i];
  if (a === '--read-only') opt.readOnly = true;
  else if (a === '--log') opt.log = true;
  else if (a === '--list') opt.list = true;
  else if (a === '--no-size') opt.size = false;
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
  let logOn = opt.log;
  const say = t => stdout.write('\r\n[hydrapc] ' + t + '\r\n');
  let fsrv;
  try { fsrv = createPcFs({ root: folder, readOnly: opt.readOnly, log: t => { if (logOn) process.stderr.write('[pc] ' + t + '\r\n'); } }); }
  catch (e) { usage(e.message); }

  const port = new SerialPort({ path: portName, baudRate: opt.baud, dataBits: 8, parity: 'none', stopBits: 1 }, err => {
    if (err) { console.error('hydrapc: ' + portName + ': ' + err.message); process.exit(1); }
    port.set({ rts: true, dtr: true }, e => {                  // (The Hydra sends only while its CTS, our RTS, is on)
      if (e) say('RTS and DTR not set (' + e.message + '): the Hydra may not send');
      say('the Hydra on ' + portName + ' at ' + opt.baud + ' baud; /pc is ' + path.resolve(folder) + (opt.readOnly ? ' (read-only)' : '') +
        '.  Ctrl-A x quits, Ctrl-A l the log.');
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

  // The Hydra's bytes: frames to the file server, the rest to the screen
  const reader = P.createReader({ types: [P.T_ATTACH, P.T_REQ],
    onFrame: f => port.write(Buffer.from(P.encode(P.T_REPLY, f.tag, f.type === P.T_ATTACH ? fsrv.attach(f.payload) : fsrv.request(f.tag, f.payload)))),
    onBad: f => port.write(Buffer.from(P.encode(P.T_NAK, f.tag))) });
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
      if (b === 1) port.write(Buffer.from([1]));
      else if (k === 'x' || k === 'q') finish('quit');
      else if (k === 'l') { logOn = !logOn; say('log ' + (logOn ? 'on' : 'off')); }
      else say('Ctrl-A then: x quit, l the log on or off, Ctrl-A a Ctrl-A');
      return;
    }
    if (b === 1) { prefix = true; return; }
    if (!tty && b === 0x0A) b = 0x0D;                           // (Piped text: a line ends in CR, as Enter sends)
    port.write(Buffer.from(b === P.MARK ? [P.MARK, P.ESC] : [b]));
  }
  if (tty) stdin.setRawMode(true);
  stdin.on('data', buf => { for (const b of buf) if (!(!tty && b === 0x0D)) onKey(b); });
  stdin.resume();

  let done = false;
  function finish(why) {
    if (done) return;
    done = true;
    say(why);
    fsrv.close();
    if (tty) stdin.setRawMode(false);
    if (port.isOpen) port.close(() => process.exit(0)); else process.exit(0);
  }
}
