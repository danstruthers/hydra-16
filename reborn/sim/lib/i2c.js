// i2c.js - the I2C bus on the VIA's port A (PA0 SCL, PA1 SDA: open drain, pulled up on the board): the master
// drives a line low by making its pin an output (ORA's bit 0) and lets it go by making it an input, as the
// driver does.  bus(scl, sda) is told the master's side after each write to port A or its DDR; the levels
// (levels()) are the AND of the master's, the pull-ups' and the devices' (a device pulls SDA low to ack, or to
// send a 0).  The devices (env.devices: { address: size or a device }): a size is a memory, as 24C02-style EEPROMs
// are (a write's first byte sets the address, the rest are written from it on; a read gives bytes from it on, and
// wraps at the size); a device is an object of its own (smc.js): addr(rw) as its address comes (rw 1: a read),
// false not to acknowledge it; put(byte) each byte written to it, false not to acknowledge it; get() each byte
// read from it; end() as its transaction ends (a stop, or a repeated start).  Counted: starts, stops, the bytes the
// devices took and gave (stats).
'use strict';

// A memory of size bytes, as a device
function memory(size) {
  const d = { mem: new Uint8Array(size), ptr: 0, size, first: false };
  d.addr = () => { d.first = true; return true; };
  d.put = v => {
    if (d.first) { d.ptr = v % d.size; d.first = false; }
    else { d.mem[d.ptr] = v; d.ptr = (d.ptr + 1) % d.size; }
    return true;
  };
  d.get = () => { const v = d.mem[d.ptr]; d.ptr = (d.ptr + 1) % d.size; return v; };
  return d;
}

function createI2c(env) {
  const devs = new Map(Object.entries(env.devices || {}).map(([a, d]) => [+a, typeof d === 'number' ? memory(d) : d]));
  const b = { scl: 1, sda: 1, out: 1, state: 'idle', bit: 0, shift: 0, ack: false, dev: null, rw: 0, byte: 0, cur: null,
    stats: { starts: 0, stops: 0, taken: 0, given: 0 } };
  b.devices = devs;
  b.levels = () => ({ scl: b.scl, sda: b.sda & b.out });
  const ended = () => { if (b.cur && b.cur.end) b.cur.end(); b.cur = null; };
  // The master's lines (1: let go, 0: driven low), after each write
  b.bus = (scl, sda) => {
    const lineSda = sda & b.out, wasScl = b.scl, wasSda = b.sda & b.out;
    b.sda = sda;
    if (scl && wasScl && lineSda !== wasSda) {                  // SDA changing while SCL is high: a start or a stop
      ended();
      if (!lineSda) { b.state = 'addr'; b.bit = 0; b.shift = 0; b.ack = false; b.out = 1; b.stats.starts++; }
      else { b.state = 'idle'; b.out = 1; b.stats.stops++; }
      b.scl = scl;
      return;
    }
    b.scl = scl;
    if (scl && !wasScl) rise(b.sda & b.out);
    else if (!scl && wasScl) fall();
  };
  function rise(sda) {                                           // SCL up: a bit taken (or an ack read)
    if (b.state === 'addr' || b.state === 'rx') { if (b.bit < 8) { b.shift = ((b.shift << 1) | sda) & 0xFF; b.bit++; } }
    else if (b.state === 'tx') {
      if (b.bit < 8) b.bit++;
      else if (sda) b.state = 'ignore';                          // (The master's nack: no more)
      else b.bit = 9;                                            // (Its ack: the next byte at the next fall)
    }
  }
  function fall() {                                              // SCL down: the device's next bit, or its ack
    if (b.ack) {                                                 // The ack's clock done: let SDA go
      b.ack = false;
      b.out = 1;
      b.bit = 0;
      b.shift = 0;
      if (b.state === 'tx') send();
      return;
    }
    if ((b.state === 'addr' || b.state === 'rx') && b.bit === 8) {
      if (b.state === 'addr') {
        b.dev = devs.get(b.shift >> 1) || null;
        b.rw = b.shift & 1;
        if (!b.dev || b.dev.addr(b.rw) === false) { b.state = 'ignore'; return; }   // (No ack: the line stays up)
        b.cur = b.dev;
        b.state = b.rw ? 'tx' : 'rx';
      } else {
        b.stats.taken++;
        if (b.dev.put(b.shift) === false) { b.state = 'ignore'; return; }
      }
      b.out = 0;                                                 // Ack: SDA low for the ninth clock
      b.ack = true;
      return;
    }
    if (b.state === 'tx') {
      if (b.bit === 9) send();                                   // (Acked: the next byte)
      else if (b.bit < 8) b.out = (b.byte >> (7 - b.bit)) & 1;
      else b.out = 1;                                            // (Eight sent: SDA let go for the master's ack)
    }
  }
  function send() {                                              // A byte to send: its first bit out
    b.byte = b.dev.get() & 0xFF;
    b.stats.given++;
    b.bit = 0;
    b.out = (b.byte >> 7) & 1;
  }
  return b;
}

module.exports = { createI2c, memory };
