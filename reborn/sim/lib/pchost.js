// pchost.js - the PC tool's part of /pc, played in the emulator (sim/run.js --pc-dir, a test's pc): the frames the
// Hydra sends on its serial port are taken out of its output and answered by the PC tool's own file server
// (sim/tools/pcfs.js; the frames: sim/lib/pcproto.js), as the PC tool (sim/tools/hydrapc.js) does on a
// real PC; the rest of the output is the console's.  Its replies go back on the serial port at the line's rate.
//
// createPcHost({ dir, readOnly, log, damage }) gives { push(byte) -> the bytes that are the console's, flush() ->
// a frame cut short, as bytes, send (set by the machine: the PC's bytes to the serial port), report() -> a line,
// attaches, requests, naks, fsrv }.  damage: frames damaged on the line, to try the resends: 'qN', the Nth frame the
// Hydra sends (a byte of its body has bit 6 flipped: the PC tool asks for it again, its NAK); 'rN', the Nth reply
// (the Hydra asks again, and the PC tool answers from its last reply, not doing it twice).
'use strict';
const P = require('./pcproto.js');
const { createPcFs } = require('../tools/pcfs.js');

// A byte damaged (bit 6 flipped; bit 0 if that makes a byte the console acts on as it comes in, or a frame's mark)
const hurt = b => [0x03, 0x1C, 0x1D, 0x1E, 0x1F].includes(b ^ 0x40) ? b ^ 0x01 : b ^ 0x40;

function createPcHost({ dir, readOnly = false, log, damage = [] }) {
  const fsrv = createPcFs({ root: dir, readOnly, log });
  const h = { requests: 0, attaches: 0, naks: 0, send: null, fsrv };
  const bad = new Set(damage);
  let sent = 0, replies = 0, at = -1;                          // (Frames from the Hydra, replies; a frame's byte count)
  const reply = bytes => {
    if (bad.has('r' + ++replies)) { bytes = Uint8Array.from(bytes); bytes[6] = hurt(bytes[6]); }
    h.send(bytes);
  };
  const reader = P.createReader({ types: [P.T_ATTACH, P.T_REQ],
    onFrame: f => {
      if (f.type === P.T_ATTACH) h.attaches++; else h.requests++;
      reply(P.encode(P.T_REPLY, f.tag, f.type === P.T_ATTACH ? fsrv.attach(f.payload) : fsrv.request(f.tag, f.payload)));
    },
    onBad: f => { h.naks++; reply(P.encode(P.T_NAK, f.tag)); } });
  h.push = b => {
    if (b === P.MARK) at = bad.has('q' + ++sent) ? 0 : -1;    // (qN: its 6th byte on)
    else if (at >= 0 && ++at === 6) { b = hurt(b); at = -1; }
    return reader.push(b);
  };
  h.flush = () => reader.flush();
  h.report = () => '/pc: ' + h.attaches + ' attach(es), ' + h.requests + ' request(s), ' + h.naks + ' damaged (asked again), ' +
    fsrv.stats.repeats + ' repeated (a reply lost)';
  return h;
}

module.exports = { createPcHost };
