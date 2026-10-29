// core.js - the WebAssembly core (base/sim/core65: the CPU and the memory map; core65.js its bytes) for machine.js:
// createCore(env, sizes) gives { x: its exports, si, sf: the state it shares (Int32Array, Float64Array: core65's
// lib.rs names them), buf: its memory, and where the machine's memory is in it: TR (task RAM, 16 x 32K), TB (the
// tasks' RAM banks, 16 x banks x 8K), SB (the shared banks, 256 x 8K), PROM, OSROM, VEC (IRQ vector RAM, 16 x 2),
// RING (the trace, ringLen entries of 8 x 2 bytes), FLAGS (a bank reached: 16 x 256, then 256) }.  env: the imports
// (lib.rs's: io_rd, io_wr, shared_write, rtc_rd, rtc_wr, reached, stack_low, brk_vector, ioff_note, pc_watch).
// sizes: { banks, promLen, osromLen, ringLen }.  available(): WebAssembly is here.  No Node.js: it runs in a browser
// too (the web page's worker).
'use strict';
const { WASM } = require('./core65.js');

let module_ = null;
function bytes() {                                         // (Its base64: no atob or Buffer needed)
  const A = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/', val = new Uint8Array(128);
  for (let i = 0; i < 64; i++) val[A.charCodeAt(i)] = i;
  const s = WASM.replace(/=+$/, ''), out = new Uint8Array(Math.floor(s.length * 3 / 4));
  let o = 0, acc = 0, bits = 0;
  for (let i = 0; i < s.length; i++) { acc = acc << 6 | val[s.charCodeAt(i)]; bits += 6; if (bits >= 8) { bits -= 8; out[o++] = (acc >> bits) & 0xFF; } }
  return out;
}
const available = () => typeof WebAssembly === 'object' && typeof WebAssembly.Module === 'function';

function createCore(env, sizes) {
  if (!module_) module_ = new WebAssembly.Module(bytes());
  const x = new WebAssembly.Instance(module_, { env }).exports;
  const up = (n, a) => Math.ceil(n / a) * a;
  const L = {};
  let at = up(x.heap_base(), 16);
  const put = (name, n) => { L[name] = at; at = up(at + n, 16); };
  put('TR', 16 * 0x8000); put('TB', 16 * sizes.banks * 0x2000); put('SB', 256 * 0x2000); put('PROM', sizes.promLen);
  put('OSROM', sizes.osromLen); put('VEC', 32); put('RING', Math.max(1, sizes.ringLen) * 16); put('FLAGS', 16 * 256 + 256);
  const pages = Math.ceil(at / 65536) - x.memory.buffer.byteLength / 65536;
  if (pages > 0) x.memory.grow(pages);
  const buf = x.memory.buffer;
  const si = new Int32Array(buf, x.state_i(), 64), sf = new Float64Array(buf, x.state_f(), 8);
  si[34] = sizes.banks; si[35] = sizes.promLen;
  si[36] = L.TR; si[37] = L.TB; si[38] = L.SB; si[39] = L.PROM; si[40] = L.OSROM; si[41] = L.VEC; si[42] = L.RING; si[43] = L.FLAGS;
  return Object.assign({ x, buf, si, sf }, L);
}

module.exports = { createCore, available };
