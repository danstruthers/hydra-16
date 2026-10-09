// png.js - a PNG of an indexed picture (the VERA's frame: vera.js's frame()), for run.js's --frame-png and the
// tests.  encodePng({ width, height, pixels (palette indexes), rgb (the palette: 0xRRGGBB each) }, deflate):
// deflate(bytes) -> the zlib stream (Node's zlib.deflateSync; in a browser, any zlib), so this needs no Node.
// It writes an 8-bit palette image (colour type 3), 256 entries.
'use strict';

const CRC = new Uint32Array(256);
for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; CRC[n] = c >>> 0; }
function crc32(bytes) {
  let c = 0xFFFFFFFF;
  for (const b of bytes) c = CRC[(c ^ b) & 0xFF] ^ (c >>> 8);
  return (c ^ 0xFFFFFFFF) >>> 0;
}
// A chunk: its length, type, data, and the CRC of the type and data
function chunk(type, data) {
  const out = new Uint8Array(12 + data.length), dv = new DataView(out.buffer);
  dv.setUint32(0, data.length);
  for (let i = 0; i < 4; i++) out[4 + i] = type.charCodeAt(i);
  out.set(data, 8);
  dv.setUint32(8 + data.length, crc32(out.subarray(4, 8 + data.length)));
  return out;
}

function encodePng(img, deflate) {
  const { width, height, pixels, rgb } = img;
  const ihdr = new Uint8Array(13), dv = new DataView(ihdr.buffer);
  dv.setUint32(0, width); dv.setUint32(4, height);
  ihdr[8] = 8; ihdr[9] = 3;                                   // (8 bits, a palette; deflate, filters 0, no interlace)
  const plte = new Uint8Array(768);
  for (let i = 0; i < 256; i++) { plte[3 * i] = rgb[i] >> 16; plte[3 * i + 1] = (rgb[i] >> 8) & 0xFF; plte[3 * i + 2] = rgb[i] & 0xFF; }
  const raw = new Uint8Array(height * (width + 1));           // (Each row: filter 0, then its pixels)
  for (let y = 0; y < height; y++) raw.set(pixels.subarray(y * width, (y + 1) * width), y * (width + 1) + 1);
  const parts = [Uint8Array.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]), chunk('IHDR', ihdr), chunk('PLTE', plte),
    chunk('IDAT', Uint8Array.from(deflate(raw))), chunk('IEND', new Uint8Array(0))];
  const out = new Uint8Array(parts.reduce((n, p) => n + p.length, 0));
  let at = 0;
  for (const p of parts) { out.set(p, at); at += p.length; }
  return out;
}

module.exports = { encodePng };
