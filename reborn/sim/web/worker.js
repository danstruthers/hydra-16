// worker.js - the Hydra-16 in a browser's Web Worker (sim/web.js bundles it into the page, web/page.js starts it):
// the machine (sim/lib/machine.js) run in real time, or as fast as it goes, as run.js -i runs it, while the page draws
// the terminal and the Vera X's screen and plays the sound.  Off the page's thread, so neither slows the other, and a
// tab in the background still runs (its timers aren't held back as a page's are).
//   The page's messages (each { type, ... }):
//     boot { bios, prom, opt, cards }   a new machine (power on): bios and prom the images (ArrayBuffers: the
//                                       paged ROM's chips one after another), opt createMachine's (vera, smc, modules,
//                                       rtc, seed), cards { sd0, verasd } (ArrayBuffers: card images, or none)
//     keys { s }          bytes typed at the serial console (a string, a byte a character)
//     key { n, down }     a key (its IBM number: lib/keynum.js) pressed or let go at the Vera X's keyboard (--smc's)
//     mouse { dx, dy, b, w }   the Vera X's mouse moved, its buttons and wheel
//     reset               the reset button
//     pause { on }, speed { x }   stopped or going; x times real time (0: as fast as it goes)
//     sound { on }        the sound given out (as 'audio' messages) or not
//     frame               the Vera X's last whole frame, please (the page asks again once it has drawn one)
//     card { slot }       a copy of a card's image as it is now (slot: sd0 or verasd)
//   Its messages: out { s } (the serial port's output since the last), frame { pixels, rgb, ready, frames, dcVideo,
//   version } (or { same: true }: no new one yet), audio { s } (an Int16Array: 48,000 stereo samples a second),
//   status { cyc, rate, clock } (twice a second: the cycle, and the cycles a second lately), card { slot, bytes },
//   written { slot } (a card was written: its image has changed since it was last asked for), halted { why }.
'use strict';
const { createMachine } = require('../../../base/sim/lib/machine.js');

const CLOCK = 3.579545;
const now = () => performance.now() / 1000;
let m = null, cps = CLOCK * 1e6, timer = null, paused = false, speed = 1, baseT = 0, baseC = 0, sent = 0;
let soundOn = false, lastFrame = null, rateT = 0, rateC = 0, statusAt = 0;
const cards = {};                                            // By slot: { bytes, written }

const post = (msg, transfer) => self.postMessage(msg, transfer || []);
const sendAudio = s => post({ type: 'audio', s }, [s.buffer]);

// A card from an image: SD device dev's blocks, read and written in its bytes
function card(slot, dev, buf) {
  const bytes = new Uint8Array(buf), c = { bytes, written: false };
  cards[slot] = c;
  return { dev, blocks: Math.floor(bytes.length / 512),
    read: n => bytes.slice(n * 512, n * 512 + 512),
    write: (n, b) => { bytes.set(b, n * 512); if (!c.written) { c.written = true; post({ type: 'written', slot }); } } };
}

function boot(msg) {
  const opt = msg.opt || {};
  for (const k of Object.keys(cards)) delete cards[k];
  const sd = msg.cards && msg.cards.sd0 ? [card('sd0', 0, msg.cards.sd0)] : [];
  let vera = null;
  if (opt.vera) {
    vera = { version: [47, 0, 2] };
    if (msg.cards && msg.cards.verasd) vera.sd = card('verasd', 0, msg.cards.verasd);
  }
  m = createMachine({ modules: opt.modules || 2, sharedU: 16, aciaLine: 1, stuckIrq: -1, acia: 'rockwell', clock: CLOCK, trace: 0,
    pcWatches: [], watches: [], marks: [], log: () => {}, seed: opt.seed, rtc: opt.rtc ? 'now' : undefined,
    vera, smc: !!(opt.vera && opt.smc), sound: true, sd,
    osrom: new Uint8Array(msg.bios), pagedrom: new Uint8Array(msg.prom) });
  if (m.vera) m.vera.live = true;                            // (Each line drawn as its time comes: raster effects)
  if (soundOn) m.audio.on(sendAudio);
  sent = 0; lastFrame = null;
  baseT = rateT = now(); baseC = rateC = 0;
  go();
}

function go() {
  clearTimeout(timer);
  timer = null;
  if (m && !paused && !m.cpu.halted) timer = setTimeout(tick, 0);
}

function flush() {
  if (sent < m.out.length) { post({ type: 'out', s: m.out.slice(sent) }); sent = m.out.length; }
  if (m.out.length > 1 << 16) { m.out = ''; sent = 0; }
}

function tick() {
  timer = null;
  const cpu = m.cpu, t = now();
  if (speed > 0) {
    let target = baseC + (t - baseT) * cps * speed;
    if (target - cpu.cyc > cps * speed * 0.25) { baseT = t; baseC = cpu.cyc; target = cpu.cyc + cps * speed * 0.01; }   // (Behind: catch up no more)
    m.run(target);
  } else while (now() - t < 0.03 && !cpu.halted) m.run(cpu.cyc + 200000);
  flush();
  if (t - statusAt >= 0.5) {
    post({ type: 'status', cyc: cpu.cyc, rate: (cpu.cyc - rateC) / (t - rateT), clock: cps, frames: m.vera ? m.vera.frames : 0 });
    statusAt = rateT = t; rateC = cpu.cyc;
  }
  if (cpu.halted) { post({ type: 'halted', why: String(cpu.halted) }); return; }
  timer = setTimeout(tick, speed > 0 ? 4 : 0);
}

function frame() {
  const v = m && m.vera;
  if (!v) return post({ type: 'frame', none: true });
  const f = v.lastFrame || v.frame();
  if (f === lastFrame) return post({ type: 'frame', same: true, ready: v.ready, frames: v.frames });
  lastFrame = f;
  const pixels = f.pixels.slice();                           // (A copy: lastFrame is the VERA's)
  post({ type: 'frame', pixels, rgb: f.rgb, ready: v.ready, frames: v.frames, dcVideo: v.dcVideo,
    version: v.version ? v.version.join('.') : '0.9' }, [pixels.buffer]);
}

self.onmessage = e => {
  const msg = e.data;
  switch (msg.type) {
    case 'boot': boot(msg); break;
    case 'keys': if (m) for (const c of msg.s) if (c.charCodeAt(0) < 0x100) m.acia.type(c); break;   // (A key each: bytes)
    case 'key': if (m && m.smc) m.smc.key(msg.n, !!msg.down); break;
    case 'mouse': if (m && m.smc) m.smc.move(msg.dx | 0, msg.dy | 0, msg.b | 0, msg.w | 0); break;
    case 'reset': if (m) { m.hwReset(); go(); } break;
    case 'pause': paused = !!msg.on; if (!paused) { baseT = now(); baseC = m ? m.cpu.cyc : 0; } go(); break;
    case 'speed': speed = +msg.x || 0; baseT = now(); baseC = m ? m.cpu.cyc : 0; break;
    case 'sound':
      if (!!msg.on !== soundOn && m) { if (msg.on) m.audio.on(sendAudio); else m.audio.off(sendAudio); }
      soundOn = !!msg.on;
      break;
    case 'frame': frame(); break;
    case 'card': {
      const c = cards[msg.slot];
      if (!c) { post({ type: 'card', slot: msg.slot, bytes: null }); break; }
      c.written = false;
      const bytes = c.bytes.slice();
      post({ type: 'card', slot: msg.slot, bytes }, [bytes.buffer]);
      break;
    }
  }
};
