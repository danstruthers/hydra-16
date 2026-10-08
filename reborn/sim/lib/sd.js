// sd.js - SPI on VIA port B (PB0 SCLK, PB1 /CS enable, PB2 MOSI, PB3-PB5 device 0-7, PB6 = 1 for devices 8-15
// (the slots'), PB7 MISO; devices sample on SCLK's rising edge: modes 0 and 3), with up to 8 SD cards (SPI mode)
// on devices 0-7, and test devices (echo: for /dev/spi) on any of the 16.  An echo device answers each byte with
// the one it got before it; its first after a select is $A0 if SCLK was low when it was selected (mode 0), $A3 if
// it was high (mode 3).  A card is SDHC (block addresses),
// or standard capacity (sdsc: byte addresses, CSD v1); its blocks come from a block device: { blocks, read(n) ->
// 512 bytes, write(n, bytes) } (a file, in hydrasim.js; anything, elsewhere).  createCard(d) is one card alone, a
// byte at a time (the VERA's SPI controller's: vera.js): xfer(b) gives the byte the card sends while b goes in (its
// answer to the byte before), deselect() as its select goes.
'use strict';

function cardReset(sd) {                                    // Deselected: it forgets the transfer
  sd.bit = 0; sd.cmd = []; sd.q = []; sd.wr = null; sd.writeAt = -1; sd.cur = 0xFF;
}
function command(sd, c) {                                   // One 6-byte command: its answer bytes
  const idx = c[0] & 0x3F, idle = sd.idle ? 1 : 0;
  let arg = ((c[1] << 24) | (c[2] << 16) | (c[3] << 8) | c[4]) >>> 0;
  if (sd.sdsc && (idx === 17 || idx === 24)) { if (arg % 512) return [0x20 | idle]; arg /= 512; }   // SDSC: byte addresses (address error)
  if (sd.app) {
    sd.app = false;
    if (idx === 41) { if (++sd.acmd41 >= 3) sd.idle = false; return [sd.idle ? 1 : 0]; }
    return [0x04 | idle];
  }
  switch (idx) {
    case 0: sd.idle = true; sd.acmd41 = 0; return [0x01];
    case 8: return [idle, 0x00, 0x00, c[3] & 0x0F, c[4]];      // R7: voltage accepted, the pattern back
    case 55: sd.app = true; return [idle];
    case 58: return [idle, sd.sdsc ? 0x80 : 0xC0, 0xFF, 0x80, 0x00];   // OCR: powered up, CCS (SDHC)
    case 9: return [idle, 0xFF, 0xFE, ...csd(sd), 0x12, 0x34];  // CSD: R1, a wait, the token, 16 bytes, CRC
    case 16: return [idle];
    case 17:
      if (sd.idle || arg >= sd.blocks) return [0x40 | idle];   // (Parameter error)
      return [0x00, 0xFF, 0xFF, 0xFE, ...sd.store.read(arg), 0x12, 0x34];   // R1, a wait, the token, data, CRC
    case 24:
      if (sd.idle || arg >= sd.blocks) return [0x40 | idle];
      sd.writeAt = arg; return [0x00];                         // Then: the data block
    default: return [0x04 | idle];                             // Illegal command
  }
}
function csd(sd) {                                          // The CSD register: the card's size
  const b = [0x40, 0x0E, 0x00, 0x32, 0x5B, 0x59, 0x00, 0, 0, 0, 0x7F, 0x80, 0x0A, 0x40, 0x00, 0x01];
  if (!sd.sdsc) {                                             // v2: (C_SIZE + 1) * 1024 blocks
    const c = Math.max(1, Math.floor(sd.blocks / 1024)) - 1;
    b[7] = (c >> 16) & 0x3F; b[8] = (c >> 8) & 0xFF; b[9] = c & 0xFF;
  } else {                                                    // v1: (C_SIZE + 1) << (C_SIZE_MULT + 2) blocks of 2^READ_BL_LEN
    const len = sd.blocks <= 4096 * 512 ? 9 : sd.blocks <= 4096 * 1024 ? 10 : 11;
    const c = Math.max(1, Math.floor(sd.blocks / (512 << (len - 9)))) - 1;   // (C_SIZE_MULT 7: * 512)
    b[0] = 0x00; b[5] = 0x50 | len; b[6] = (c >> 10) & 3; b[7] = (c >> 2) & 0xFF; b[8] = ((c & 3) << 6) | 0x3F;
    b[9] = 0xE0 | 3; b[10] = 0x80 | 0x7F;
  }
  return b;
}
function byteIn(sd, b) {                                    // Byte b came in: the next byte out
  if (sd.echo) return b;
  if (sd.wr) {                                                // A block for CMD24: token, 512 bytes, CRC
    if (sd.wr.data.length === 0 && b !== 0xFE) return 0xFF;
    sd.wr.data.push(b);
    if (sd.wr.data.length === 515) {
      sd.store.write(sd.wr.at, Uint8Array.from(sd.wr.data.slice(1, 513)));
      sd.wr = null; sd.q = [0x00, 0x00, 0x00, 0xFF];           // (Busy a while, then ready)
      return 0x05;                                             // Data accepted
    }
    return 0xFF;
  }
  if (sd.cmd.length || (b & 0xC0) === 0x40) {
    sd.cmd.push(b);
    if (sd.cmd.length === 6) { sd.q = [0xFF, ...command(sd, sd.cmd)]; sd.cmd = []; return sd.q.shift(); }
    return 0xFF;
  }
  if (sd.q.length) return sd.q.shift();
  if (sd.writeAt >= 0) { sd.wr = { at: sd.writeAt, data: [] }; sd.writeAt = -1; }   // (R1 out: data next)
  return 0xFF;
}

// A card alone, byte by byte
function createCard(d) {
  const sd = { dev: -1, store: d, blocks: d.blocks, sdsc: !!d.sdsc, bit: 0, inB: 0, cur: 0xFF, miso: 1,
    q: [], cmd: [], idle: true, app: false, acmd41: 0, writeAt: -1, wr: null };
  return {
    xfer(b) { const out = sd.cur; sd.cur = byteIn(sd, b & 0xFF); return out; },
    deselect() { cardReset(sd); },
  };
}

function createSpi(devices, echoes = []) {
  const cards = [];                                           // By device: a card, or undefined
  for (const dev of echoes) cards[dev] = { dev, echo: true, bit: 0, inB: 0, cur: 0xFF, miso: 1 };
  for (const d of devices) cards[d.dev] = { dev: d.dev, store: d, blocks: d.blocks, sdsc: !!d.sdsc, bit: 0, inB: 0, cur: 0xFF, miso: 1,
    q: [], cmd: [], idle: true, app: false, acmd41: 0, writeAt: -1, wr: null };
  let sel = null, clk = 0;                                    // The selected card (null: none), SCLK
  return {
    // Port B's output bits changed
    portB(v) {
      const card = !(v & 0x02) ? cards[((v >> 3) & 7) | ((v >> 3) & 8)] || null : null;
      const c = v & 1;
      if (card !== sel) {
        if (sel && !sel.echo) cardReset(sel);
        sel = card;
        if (card && card.echo) { card.bit = 0; card.cur = c ? 0xA3 : 0xA0; card.miso = (card.cur >> 7) & 1; }
      }
      if (c && !clk && card) {                                  // Rising edge: both sides sample
        card.miso = (card.cur >> (7 - card.bit)) & 1;
        card.inB = ((card.inB << 1) | ((v >> 2) & 1)) & 0xFF;
        if (++card.bit === 8) { card.bit = 0; card.cur = byteIn(card, card.inB); }
      }
      clk = c;
    },
    miso: () => (sel ? sel.miso : 1),
  };
}

module.exports = { createSpi, createCard };
