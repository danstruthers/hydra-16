// vera.js - the Vera X card in slot 0 (docs/design/plans/VIDEO.md): the VERA, an iCE40UP5K with 128K of video RAM, as the
// X16 community's gateware has it (v47.0.2 by default: X16Community/vera-module; the programmer's reference is the
// X16's, chapter 9), its 32 registers at $FF20-$FF3F (slot 0's I/O ports 2 and 3), its IRQ# on line 2 (slot 0's
// IRQ A).  C:\source\x16-emulator's video.c (BSD 2-clause) was the guide to the details.  Modelled:
//   * the registers: ADDR0/ADDR1 (CTRL's ADDRSEL), their increments (and DECR), the data ports with the byte each
//     has fetched ahead (a read gives it, then steps the address and fetches the next; a write, or setting the
//     address, fetches too: a write through one port leaves the other's fetched byte as it was, as on the chip);
//     DCSEL's register sets (0: DC_VIDEO, the scales, the border; 1: the active area; 2-6: FX's, below; 63: the
//     version, "V" and its three numbers); the layers'; IEN, ISR, IRQLINE,
//     SCANLINE; the audio's and the SPI controller's;
//   * VRAM, random at power-up, and the write-only registers it shadows: the PSG ($1F9C0), the palette ($1FA00:
//     the chip's default at reset) and the sprite attributes ($1FC00), written with VRAM;
//   * the scan: VGA's 800 x 525 at 25 MHz (59.5 Hz), or NTSC's and RGB's (output modes 2 and 3: two fields of 525
//     half-lines of 794), in the CPU's cycles; SCANLINE; the LINE and VSYNC interrupts at their line's start;
//     sprite collisions, worked out at VSYNC for the frame (the sprite renderer's budget of 800 clocks a line
//     too); IRQ# while (ISR | AFLOW) & IEN isn't 0;
//   * the PCM FIFO (4K: 4095 bytes held at most): its bytes, drained at the sample rate (AUDIO_RATE / 128 of
//     48828 Hz, a sample 1, 2 or 4 bytes), AFLOW while it's under 1024, its full and empty flags; the PCM bytes and
//     the PSG's voices counted (pcmIn, pcmOut, pcmLost; psgOns, as ym2151.js's key-ons);
//   * FX (the X16's Programmer's Reference, chapter 10; video.c's model): ADDR1's modes (line draw: ADDR0's step
//     each time the X position's fraction carries; polygon fill: ADDR1 at ADDR0 + X, the fill's length readable;
//     affine: ADDR1 from a tile map, X and Y stepping), 4-bit mode (the nibble bit and its increment, nibble
//     writes), the 16-bit hop, the 32-bit cache (filled by reads, written 4 bytes at a time with a nibble mask, or
//     byte by byte cycling), transparent writes (0 left alone), the multiplier and its accumulator, 2-bit polygon
//     poking;
//   * the sound, if it's asked for (sound(fn): audio.js's): the PSG's 16 voices and the PCM's samples, a stereo
//     sample at 48828 Hz each given to fn(left, right), as the X16's emulator makes them (its vera_psg.c and
//     vera_pcm.c, Frank van den Hoef's, BSD 2-clause), made up to a cycle as the machine runs (before a PSG register
//     changes, as the FIFO's drained) and as soundTo(t) asks.  Then the FIFO's drained a sample at a time, as those
//     files do; without it, as many samples at once as time has passed (the same reads, the same level);
//     pcmUnderruns, the times the FIFO ran dry while it played (a sample wanted, none there, since bytes came);
//     with env.pcmLog, pcmLog the bytes it took (the first PCM_LOG_MAX), for a test to compare;
//   * the SPI controller (SPI_DATA, SPI_CTRL: busy for 8 bits at 12.5 MHz, or 390 kHz with the slow clock: a byte
//     clocked out whether or not the card's selected, the card answering only while it is), env.sd's card on it if
//     there's one (sd.js's createCard: a block device), else no
//     card on it: it reads $FF (the Vera X brings its SD card's lines to a header);
//   * the FPGA configuring itself after power-up, the reset button (RESB: the card's RES#) and CTRL's reset bit:
//     env.configCycles (0.1 s by default) of no answer (reads float, $FF; writes are lost), then every register
//     as the gateware starts it (the screen off: DC_VIDEO 0); VRAM keeps what it had.
// The picture is drawn only when asked for (frame(): the whole screen as it is now, palette indexes and the
// palette's colours), so a test that doesn't look costs nothing; with live set, each line is drawn as its time
// comes (raster effects shown), and lastFrame is the last whole frame.  text() reads the text layer's characters.
// Interface: read(reg, t) (-1: no answer), write(reg, v, t), tick(t), irqActive(), nextEvent(devCyc), reset(t);
// sound(fn), soundTo(t).
'use strict';
const { createCard } = require('./sd.js');

// The palette at reset (the gateware's: 12 bits, $0RGB)
const DEFAULT_PALETTE = [
  0x000, 0xfff, 0x800, 0xafe, 0xc4c, 0x0c5, 0x00a, 0xee7, 0xd85, 0x640, 0xf77, 0x333, 0x777, 0xaf6, 0x08f, 0xbbb,
  0x000, 0x111, 0x222, 0x333, 0x444, 0x555, 0x666, 0x777, 0x888, 0x999, 0xaaa, 0xbbb, 0xccc, 0xddd, 0xeee, 0xfff,
  0x211, 0x433, 0x644, 0x866, 0xa88, 0xc99, 0xfbb, 0x211, 0x422, 0x633, 0x844, 0xa55, 0xc66, 0xf77, 0x200, 0x411,
  0x611, 0x822, 0xa22, 0xc33, 0xf33, 0x200, 0x400, 0x600, 0x800, 0xa00, 0xc00, 0xf00, 0x221, 0x443, 0x664, 0x886,
  0xaa8, 0xcc9, 0xfeb, 0x211, 0x432, 0x653, 0x874, 0xa95, 0xcb6, 0xfd7, 0x210, 0x431, 0x651, 0x862, 0xa82, 0xca3,
  0xfc3, 0x210, 0x430, 0x640, 0x860, 0xa80, 0xc90, 0xfb0, 0x121, 0x343, 0x564, 0x786, 0x9a8, 0xbc9, 0xdfb, 0x121,
  0x342, 0x463, 0x684, 0x8a5, 0x9c6, 0xbf7, 0x120, 0x241, 0x461, 0x582, 0x6a2, 0x8c3, 0x9f3, 0x120, 0x240, 0x360,
  0x480, 0x5a0, 0x6c0, 0x7f0, 0x121, 0x343, 0x465, 0x686, 0x8a8, 0x9ca, 0xbfc, 0x121, 0x242, 0x364, 0x485, 0x5a6,
  0x6c8, 0x7f9, 0x020, 0x141, 0x162, 0x283, 0x2a4, 0x3c5, 0x3f6, 0x020, 0x041, 0x061, 0x082, 0x0a2, 0x0c3, 0x0f3,
  0x122, 0x344, 0x466, 0x688, 0x8aa, 0x9cc, 0xbff, 0x122, 0x244, 0x366, 0x488, 0x5aa, 0x6cc, 0x7ff, 0x022, 0x144,
  0x166, 0x288, 0x2aa, 0x3cc, 0x3ff, 0x022, 0x044, 0x066, 0x088, 0x0aa, 0x0cc, 0x0ff, 0x112, 0x334, 0x456, 0x668,
  0x88a, 0x9ac, 0xbcf, 0x112, 0x224, 0x346, 0x458, 0x56a, 0x68c, 0x79f, 0x002, 0x114, 0x126, 0x238, 0x24a, 0x35c,
  0x36f, 0x002, 0x014, 0x016, 0x028, 0x02a, 0x03c, 0x03f, 0x112, 0x334, 0x546, 0x768, 0x98a, 0xb9c, 0xdbf, 0x112,
  0x324, 0x436, 0x648, 0x85a, 0x96c, 0xb7f, 0x102, 0x214, 0x416, 0x528, 0x62a, 0x83c, 0x93f, 0x102, 0x204, 0x306,
  0x408, 0x50a, 0x60c, 0x70f, 0x212, 0x434, 0x646, 0x868, 0xa8a, 0xc9c, 0xfbe, 0x211, 0x423, 0x635, 0x847, 0xa59,
  0xc6b, 0xf7d, 0x201, 0x413, 0x615, 0x826, 0xa28, 0xc3a, 0xf3c, 0x201, 0x403, 0x604, 0x806, 0xa08, 0xc09, 0xf0b,
];
// ADDRx_H's bits 3-7 (the increment, and DECR in bit 3): the step
const STEP = [0, 0, 1, -1, 2, -2, 4, -4, 8, -8, 16, -16, 32, -32, 64, -64, 128, -128, 256, -256, 512, -512,
  40, -40, 80, -80, 160, -160, 320, -320, 640, -640];
const W = 640, H = 480;                                       // The screen
const PSG = 0x1F9C0, PAL = 0x1FA00, SPR = 0x1FC00;            // The registers VRAM shadows
const FIFO = 4096, AFLOW = 1024;                              // The PCM FIFO, and its low mark
const PCM_HZ = 25e6 / 512;                                    // The audio's sample rate
const GROUP = [1, 2, 2, 4];                                   // AUDIO_CTRL's bits 4-5: a sample's bytes
const PCM_LOG_MAX = 1 << 20;                                  // pcmLog's bytes at most
// The PSG's volume (6 bits) and the PCM's (4 bits), as the X16's emulator has them (vera_psg.c, vera_pcm.c)
const PSG_VOLUME = [
  0, 4, 8, 12, 16, 17, 18, 20, 21, 22, 23, 25, 26, 28, 30, 31, 33, 35, 37, 40, 42, 45, 47, 50, 53, 56, 60, 63, 67, 71, 75, 80,
  85, 90, 95, 101, 107, 113, 120, 127, 135, 143, 151, 160, 170, 180, 191, 202, 214, 227, 241, 255, 270, 286, 303, 321, 341, 361,
  382, 405, 429, 455, 482, 511];
const PCM_VOLUME = [0, 1, 2, 3, 4, 5, 6, 8, 11, 14, 18, 23, 30, 38, 49, 64];

function createVera(env) {
  const clock = env.clock || 3.579545;                        // The CPU's (MHz)
  const K = 25 / clock;                                       // The VERA's clocks a CPU cycle
  const rnd = env.rnd || (n => (Math.random() * n) | 0);
  const version = env.version === undefined ? [47, 0, 2] : env.version;   // (null: v0.9, fvdhoef's: no FX, no
  const dcselMask = version ? 0x3F : 0x01;                                  //   version, DCSEL one bit)
  const verBytes = version ? Uint8Array.from([0x56, ...version]) : null;    // (DCSEL 63's: "V" and the version)
  const configCycles = env.configCycles === undefined ? Math.round(0.1 * clock * 1e6) : env.configCycles;
  const v = {
    vram: new Uint8Array(0x20000).map(() => rnd(256)),
    palette: new Uint8Array(512), sprites: new Uint8Array(1024), psg: new Uint8Array(64),
    lastFrame: null, frames: 0, psgOns: [], pcmIn: 0, pcmOut: 0, pcmLost: 0, pcmUnderruns: 0, version,
    pcmLog: env.pcmLog ? [] : null,
  };
  let live = false, curT = 0, fb = null;                      // Each line drawn as it comes; the bus's cycle; (live)
                                                              //   the frame being drawn, palette indexes
  const vram = v.vram, pal = v.palette, spr = v.sprites, psg = v.psg;
  // The registers (reset() sets them)
  const addr = [0, 0], inc = [0, 0], nib = [0, 0], rd = [0, 0];   // The ports: address, step index, the nibble
  let addrsel = 0, dcsel = 0, ien = 0, isr = 0, irqLine = 0;      //   bits (FX's: kept), the byte fetched ahead
  const dc = new Uint8Array(256);                             // The DCSEL sets: set n's registers at 4n
  const layer = [new Uint8Array(7), new Uint8Array(7)];
  let actl = 0, arate = 0, loop = false, fcnt = 0, fwr = 0, frd = 0;   // The PCM FIFO ...
  const fifo = new Uint8Array(FIFO);                          //   its bytes
  let pcmPhase = 0, pcmAt = 0;                                // The audio's sample count, as of cycle pcmAt
  let pcmFed = false;                                         // (Bytes came since the FIFO last ran dry, or was reset)
  // The sound (sound(fn)): the samples made so far, the first that sounds (the FPGA configured), the PSG's voices'
  // phases and noise, its noise generator, the PCM's sample (left, right)
  let sink = null, made = 0, readySample = 0, psgNoiseState = 1, pcmL = 0, pcmR = 0;
  const psgPhase = new Int32Array(16), psgNoise = new Uint8Array(16);
  let ss = 0, autotx = 0, slow = 0, spiBusyTo = 0, spiIn = 0xFF;
  const card = env.sd ? createCard(env.sd) : null;            // (The SD card on the SPI port, if there's one)
  const spiByte = b => (card && ss ? card.xfer(b) : 0xFF);   // A byte clocked: what comes back
  // The scan: units (a VGA line, or half an NTSC one) since t0, the cycle it started at; lastU the last unit done
  let readyAt = configCycles, t0 = configCycles, unitLen = 800, perFrame = 525, lastU = -1, nextU = 0, nextCyc = 0;
  let collisions = 0;                                          // (The sprite renderer's, this frame: live)
  // FX: ADDR1's mode (0 normal, 1 line draw, 2 polygon fill, 3 affine), the switches, the positions and increments
  // (X and Y: 32 bits, the pixel in bits 16-26, the fraction below), the cache and the accumulator
  const fx = { mode: 0, nib4: false, hop: false, cycle: false, fill: false, cwrite: false, trans: false, poly2: false,
    poking: false, cincMode: false, cnib: 0, cbyte: 0, mult: false, sub: false, clip: false, hopAlign: 0,
    nibBit: [0, 0], nibInc: [0, 0], cache: new Uint8Array(4), acc: 0, xinc: 0, yinc: 0, xpos: 0, ypos: 0, fillLen: 0,
    tileBase: 0, mapBase: 0, mapSize: 2 };
  function fxReset() {
    Object.assign(fx, { mode: 0, nib4: false, hop: false, cycle: false, fill: false, cwrite: false, trans: false, poly2: false,
      poking: false, cincMode: false, cnib: 0, cbyte: 0, mult: false, sub: false, clip: false, hopAlign: 0, acc: 0,
      xinc: 0, yinc: 0, xpos: 0x8000, ypos: 0x8000, fillLen: 0, tileBase: 0, mapBase: 0, mapSize: 2 });
    fx.nibBit[0] = fx.nibBit[1] = 0; fx.nibInc[0] = fx.nibInc[1] = 0; fx.cache.fill(0);
  }

  function reset(t) {                                         // The FPGA configured: the gateware's start
    addr[0] = addr[1] = 0; inc[0] = inc[1] = 0; nib[0] = nib[1] = 0;
    addrsel = 0; dcsel = 0; ien = 0; isr = 0; irqLine = 0;
    dc.fill(0); dc[1] = 128; dc[2] = 128; dc[5] = 640 >> 2; dc[7] = 480 >> 1;
    layer[0].fill(0); layer[1].fill(0);
    for (let i = 0; i < 256; i++) { pal[2 * i] = DEFAULT_PALETTE[i] & 0xFF; pal[2 * i + 1] = DEFAULT_PALETTE[i] >> 8; }
    spr.fill(0); psg.fill(0);
    actl = 0; arate = 0; loop = false; fcnt = fwr = frd = 0; pcmPhase = 0; pcmAt = t; pcmFed = false;
    psgPhase.fill(0); psgNoise.fill(0); psgNoiseState = 1; pcmL = pcmR = 0; readySample = Math.ceil(t * PCM_HZ / (clock * 1e6));
    ss = 0; autotx = 0; slow = 0; spiBusyTo = 0; spiIn = 0xFF;
    fxReset();
    rd[0] = rd[1] = vram[0];
    t0 = t; unitLen = 800; perFrame = 525; lastU = -1; collisions = 0; plan();
  }

  // ---- The scan
  const ntsc = () => (dc[0] & 3) > 1;
  const unitAt = t => Math.floor((t - t0) * K / unitLen);    // The unit cycle t is in
  const cycOf = u => Math.ceil(t0 + u * unitLen / K + 1e-6);  // The cycle unit u starts at
  // The screen line a unit starts (-1: none), and the line the LINE interrupt compares
  function lineOf(uf) {
    if (perFrame === 525) return uf;
    const y = uf < 525 ? uf - 42 : uf - 568;
    return y >= 0 ? y : -1;
  }
  // The next unit after lastU with something to do (a VSYNC, a LINE): nextU, and its cycle nextCyc.  Live, every
  // unit is (its line drawn)
  function plan() {
    nextU = live ? lastU + 1 : after(true, true); nextCyc = cycOf(nextU);
  }
  // The first unit after lastU that starts a VSYNC (vs) or the LINE interrupt's line (ln).  (No allocation: it's
  // planned as the machine runs)
  function after(vs, ln) {
    const base = Math.floor((lastU + 1) / perFrame) * perFrame, il = irqLine & ~1;
    if (perFrame === 525) return Math.min(vs ? unitOf(480, base) : Infinity, ln ? unitOf(irqLine, base) : Infinity);
    return Math.min(vs ? Math.min(unitOf(522, base), unitOf(1048, base)) : Infinity,
      ln ? Math.min(unitOf(il + 42, base), unitOf(il + 568, base)) : Infinity);
  }
  // The first unit after lastU at place c of a frame (base: the frame lastU + 1 is in); Infinity if there's none
  function unitOf(c, base) {
    if (c >= perFrame) return Infinity;
    const u = base + c;
    return u <= lastU ? u + perFrame : u;
  }
  // A mode's timing changed (VGA and NTSC's): the scan starts again now
  function retime(t) {
    const n = ntsc();
    if ((perFrame === 1050) === n) return;
    t0 = t; unitLen = n ? 794 : 800; perFrame = n ? 1050 : 525; lastU = -1; plan();
  }
  // The units up to cycle t done: their interrupts' flags (and lines, live)
  function scan(t) {
    if (t < readyAt) return;
    if (t < nextCyc) return;
    const to = unitAt(t);
    while (nextU <= to) {
      const u = nextU, uf = u % perFrame, y = lineOf(uf);
      if (live) drawUnit(uf);
      if (y === 480) vsync(u, to);
      if (y >= 0 && y === (perFrame === 525 ? irqLine : irqLine & ~1)) isr |= 2;
      lastU = u; plan();
    }
    lastU = to; plan();
  }
  // VSYNC at unit u (the scan's at unit to): ISR's flag, the frame's collisions (worked out now, if it isn't live:
  // only for the last frame passed), the frame kept (live)
  function vsync(u, to) {
    isr |= 1; v.frames++;
    if (!live && to - u < perFrame) collisions = (dc[0] & 0x40) ? frameCollisions() : 0;
    isr = (isr & 0x0F) | collisions;
    if (collisions) isr |= 4;
    collisions = 0;
    if (live && fb) { v.lastFrame = { pixels: fb.slice(), rgb: rgbPalette() }; }
  }

  // ---- The PCM FIFO's level, as of cycle t
  const groupLen = () => GROUP[(actl >> 4) & 3];              // (A sample's bytes: 8 or 16 bits, mono or stereo)
  function pcm(t) {
    if (sink) { soundTo(t); return; }
    if (t <= pcmAt) return;
    const s0 = Math.floor(pcmAt * PCM_HZ / (clock * 1e6)), s1 = Math.floor(t * PCM_HZ / (clock * 1e6));
    pcmAt = t;
    if (!arate || s1 <= s0) return;
    const p0 = pcmPhase; pcmPhase += (s1 - s0) * arate;
    let reads = Math.floor(pcmPhase / 128) - Math.floor(p0 / 128);
    pcmPhase %= 256 * 128;
    const g = groupLen();
    while (reads > 0 && fcnt > 0) {                           // (Each read a sample's bytes; part of one: dropped)
      const n = Math.min(reads, Math.floor(fcnt / g));
      if (n === 0) { v.pcmOut += fcnt; fcnt = 0; frd = fwr; reads--; }
      else { fcnt -= n * g; frd = (frd + n * g) % FIFO; v.pcmOut += n * g; reads -= n; }
      if (loop && fcnt === 0) {                               //   (Looping: the FIFO again from its start, as many
        const per = Math.floor(fwr / g);                      //   times as the reads go round)
        if (!per) break;
        reads %= per; frd = 0; fcnt = fwr;
      }
    }
    if (reads > 0 && fcnt === 0 && pcmFed && !loop) { v.pcmUnderruns++; pcmFed = false; }
  }
  // ---- The sound: the samples up to cycle t made, each the PSG's and the PCM's (silent while configuring)
  function soundTo(t) {
    if (!sink) return;
    const n = Math.floor(t * PCM_HZ / (clock * 1e6));
    for (; made < n; made++) {
      if (made < readySample) { sink(0, 0); continue; }
      let l = 0, r = 0;
      for (let i = 0; i < 16; i++) {                          // The PSG's voices (vera_psg.c's render)
        psgNoiseState = ((psgNoiseState << 1) | (((psgNoiseState >> 1) ^ (psgNoiseState >> 2) ^ (psgNoiseState >> 4) ^ (psgNoiseState >> 15)) & 1)) & 0xFFFF;
        const b = i * 4, lr = psg[b + 2] >> 6, pw = psg[b + 3] & 0x3F, old = psgPhase[i];
        const ph = lr ? (old + (psg[b] | psg[b + 1] << 8)) & 0x1FFFF : 0;
        if ((old & 0x10000) && !(ph & 0x10000)) psgNoise[i] = (psgNoiseState >> 1) & 0x3F;
        psgPhase[i] = ph;
        let w;
        switch (psg[b + 3] >> 6) {
          case 0: w = (ph >> 10) > pw ? 0 : 0x3F; break;                                       // Pulse
          case 1: w = (ph >> 11) ^ (pw ^ 0x3F); break;                                         // Sawtooth
          case 2: w = ((ph & 0x10000) ? ~(ph >> 10) & 0x3F : (ph >> 10) & 0x3F) ^ (pw ^ 0x3F); break;   // Triangle
          default: w = psgNoise[i];                                                           // Noise
        }
        const val = (((w ^ 0x20) << 26) >> 26) * PSG_VOLUME[psg[b + 2] & 0x3F];
        if (lr & 1) l += val >> 3;
        if (lr & 2) r += val >> 3;
      }
      const old = pcmPhase;                                   // The PCM (vera_pcm.c's render): a read as bit 7 of
      pcmPhase = (pcmPhase + arate) & 0x7FFF;                 //   its phase turns
      if ((old ^ pcmPhase) & 0x80) pcmRead();
      sink(l + Math.trunc(pcmL * PCM_VOLUME[actl & 15] / 64), r + Math.trunc(pcmR * PCM_VOLUME[actl & 15] / 64));
    }
    pcmAt = t;
  }
  // A sample's bytes from the FIFO (none: silence; part of one: dropped), into pcmL and pcmR
  function pcmRead() {
    if (fcnt === 0) { if (pcmFed && !loop) { v.pcmUnderruns++; pcmFed = false; } pcmL = pcmR = 0; return; }
    const g = groupLen();
    if (fcnt < g) { v.pcmOut += fcnt; fcnt = 0; frd = fwr; }
    else {
      const b = () => { const x = fifo[frd]; frd = (frd + 1) % FIFO; fcnt--; v.pcmOut++; return x; };
      switch ((actl >> 4) & 3) {
        case 0: pcmL = pcmR = (b() << 24) >> 16; break;                             // 8 bits, mono
        case 1: pcmL = (b() << 24) >> 16; pcmR = (b() << 24) >> 16; break;          //   stereo
        case 2: { const lo = b(); pcmL = pcmR = ((lo | b() << 8) << 16) >> 16; break; }   // 16 bits, mono
        default: { let lo = b(); pcmL = ((lo | b() << 8) << 16) >> 16; lo = b(); pcmR = ((lo | b() << 8) << 16) >> 16; }
      }
    }
    if (loop && fcnt === 0) { frd = 0; fcnt = fwr; }
  }
  // Cycles from cycle t till the FIFO goes under its low mark (Infinity: it won't by itself)
  function aflowIn(t) {
    if (!arate || fcnt < AFLOW || loop) return Infinity;
    const reads = Math.floor((fcnt - AFLOW) / groupLen()) + 1;
    const samples = Math.ceil((reads * 128 - (pcmPhase % 128)) / arate);
    return Math.max(1, Math.ceil(samples * clock * 1e6 / PCM_HZ));
  }

  // ---- VRAM
  function put(a, b) {                                         // A write: VRAM, and the register it shadows
    a &= 0x1FFFF; vram[a] = b;
    if (a < PSG) return;
    if (a < PAL) {
      const r = a - PSG, ch = r >> 2;
      if (sink) soundTo(curT);                                // (What it's sounded till now, before it changes)
      const was = psg[r]; psg[r] = b;
      if ((r & 3) === 2 && !(was & 0x3F) && (b & 0x3F) && (b & 0xC0)) v.psgOns.push('voice ' + ch + ' at cycle ' + curT);
    } else if (a < SPR) pal[a - PAL] = b;
    else spr[a - SPR] = b;
  }
  const fetch = s => { rd[s] = vram[addr[s]]; };
  const step = s => { addr[s] = (addr[s] + STEP[inc[s]]) & 0x1FFFF; };
  // FX: a data port's access: its address, then the port stepped (the nibble, the hop, ADDR1's modes)
  function fxStep(s, write) {
    const a = addr[s];
    let n = STEP[inc[s]];
    if (fx.nib4 && fx.nibInc[s] && !n) {
      if (fx.nibBit[s]) { if ((inc[s] & 1) === 0) addr[s]++; fx.nibBit[s] = 0; }
      else { if (inc[s] & 1) addr[s]--; fx.nibBit[s] = 1; }
    }
    if (s === 1 && fx.hop) {
      if (n === 4) n = fx.hopAlign === (a & 3) ? 1 : 3;
      else if (n === 320) n = fx.hopAlign === (a & 3) ? 1 : 319;
    }
    addr[s] = (addr[s] + n) & 0x1FFFF;
    if (s === 1 && fx.mode === 1) {                           // Line draw: ADDR0's step when X's fraction carries
      fx.xpos = (fx.xpos + fx.xinc) >>> 0;
      if (fx.xpos & 0x10000) {
        fx.xpos = (fx.xpos & ~0x10000) >>> 0;
        if (fx.nib4 && fx.nibInc[0]) {
          if (fx.nibBit[1]) { if ((inc[0] & 1) === 0) addr[1]++; fx.nibBit[1] = 0; }
          else { if (inc[0] & 1) addr[1]--; fx.nibBit[1] = 1; }
        }
        addr[1] = (addr[1] + STEP[inc[0]]) & 0x1FFFF;
      }
    } else if (fx.mode === 2 && !write) {                     // Polygon fill: X and Y step, ADDR1 at ADDR0 + X
      fx.xpos = (fx.xpos + fx.xinc) >>> 0;
      fx.ypos = (fx.ypos + fx.yinc) >>> 0;
      fx.fillLen = (((fx.ypos | 0) >> 16) - ((fx.xpos | 0) >> 16)) & 0xFFFF;
      if (s === 0 && fx.cycle && !fx.fill) fx.cbyte = (fx.cbyte + 1) & 3;
      if (s === 1) {
        if (fx.nib4) { addr[1] = (addr[0] + (fx.xpos >>> 17)) & 0x1FFFF; fx.nibBit[1] = (fx.xpos >>> 16) & 1; }
        else addr[1] = (addr[0] + (fx.xpos >>> 16)) & 0x1FFFF;
      }
    } else if (s === 1 && fx.mode === 3 && !write) {         // Affine: X and Y step
      fx.xpos = (fx.xpos + fx.xinc) >>> 0;
      fx.ypos = (fx.ypos + fx.yinc) >>> 0;
    }
    return a;
  }
  // FX's affine mode: ADDR1 at the tile map's pixel under (X, Y), its byte fetched
  function fxAffine() {
    if (fx.mode !== 3) return;
    let tx = (fx.xpos >>> 19) & 0xFF, ty = (fx.ypos >>> 19) & 0xFF;
    const sx = (fx.xpos >>> 16) & 7, sy = (fx.ypos >>> 16) & 7, n4 = fx.nib4 ? 1 : 0;
    if (!fx.clip) { tx &= fx.mapSize - 1; ty &= fx.mapSize - 1; }
    let a;
    if (tx >= fx.mapSize || ty >= fx.mapSize) a = fx.tileBase + (sy << (3 - n4)) + (sx >> n4);
    else {
      const tile = vram[(fx.mapBase + ty * fx.mapSize + tx) & 0x1FFFF];
      a = fx.tileBase + (tile << (6 - n4)) + (sy << (3 - n4)) + (sx >> n4);
    }
    fx.nibBit[1] = (sx & 1) >> (1 - n4);
    addr[1] = a & 0x1FFFF;
    rd[1] = vram[addr[1]];
  }
  // A byte written as FX writes it (4-bit mode: a nibble; transparent writes: 0 left alone)
  function fxPut(a, nibble, b) {
    a &= 0x1FFFF;
    if (fx.nib4) {
      if (nibble) { if (!fx.trans || (b & 0x0F)) b = (vram[a] & 0xF0) | (b & 0x0F); else b = vram[a]; }
      else if (!fx.trans || (b & 0xF0)) b = (vram[a] & 0x0F) | (b & 0xF0);
      else b = vram[a];
    } else if (fx.trans && !b) return;
    put(a, b);
  }
  // A cache write's byte: mask 0 the whole byte, 1 its high nibble, 2 its low, 3 none
  function fxCachePut(a, b, mask) {
    if (fx.trans && !b) return;
    a &= 0x1FFFF;
    if (mask === 0) put(a, b);
    else if (mask === 1) put(a, (vram[a] & 0x0F) | (b & 0xF0));
    else if (mask === 2) put(a, (vram[a] & 0xF0) | (b & 0x0F));
  }
  const fxProduct = () => ((((fx.cache[1] << 8) | fx.cache[0]) << 16) >> 16) * ((((fx.cache[3] << 8) | fx.cache[2]) << 16) >> 16);
  // A data port's write, FX's way (version 47 on: FX there)
  function fxWrite(s, b) {
    if (fx.poking && fx.mode) {                               // 2-bit poking: two bits of the cache's byte at ADDR1
      fx.poking = false;
      const m = b >> 6, keep = [0x3F, 0xCF, 0xF3, 0xFC][m];
      vram[addr[1]] = (fx.cache[fx.cbyte] & ~keep & 0xFF) | (rd[1] & keep);
      return;
    }
    const nibble = fx.nibBit[s];
    let a = fxStep(s, true);
    const cache = fx.mult ? (() => {
      const r = (fx.sub ? fx.acc - fxProduct() : fx.acc + fxProduct()) | 0;
      return [r & 0xFF, (r >> 8) & 0xFF, (r >> 16) & 0xFF, (r >>> 24) & 0xFF];
    })() : Array.from(fx.cache);
    const data = fx.cycle ? fx.cache[fx.cbyte] : b;
    const bytes = fx.cwrite && !fx.cycle ? cache : [data, data, data, data];
    if (fx.cwrite) {
      a &= 0x1FFFC;
      for (let i = 0; i < 4; i++) {
        let mask;
        if (fx.trans) mask = fx.nib4 ? (((bytes[i] & 0xF0) === 0) << 1) | ((bytes[i] & 0x0F) === 0) : (bytes[i] ? 0 : 3);
        else mask = (b >> (2 * i)) & 3;
        fxCachePut(a + i, bytes[i], mask);
      }
    } else fxPut(a, nibble, data);
    fetch(s);
  }
  // A data port's read, FX's way: the byte fetched ahead, the port stepped, the cache filled
  function fxRead(s) {
    const nibble = fx.nibBit[s];
    fxStep(s, false);
    const b = rd[s];
    if (s === 1 && fx.mode === 3) fxAffine(); else fetch(s);
    if (fx.fill) {
      if (fx.nib4) {
        const n = nibble ? (b & 0x0F) << 4 : b & 0xF0;
        if (fx.cnib) { fx.cache[fx.cbyte] = (fx.cache[fx.cbyte] & 0xF0) | (n >> 4); fx.cnib = 0; fx.cbyte = (fx.cbyte + 1) & 3; }
        else { fx.cache[fx.cbyte] = (fx.cache[fx.cbyte] & 0x0F) | n; fx.cnib = 1; }
      } else {
        fx.cache[fx.cbyte] = b;
        fx.cbyte = fx.cincMode ? (fx.cbyte & 2) | ((fx.cbyte + 1) & 1) : (fx.cbyte + 1) & 3;
      }
    }
    return b;
  }
  // FX's increments: DCSEL 3's two registers, a signed 15-bit step (x 32 with bit 15)
  const fxIncrement = (lo, hi) => {
    let v = (((hi & 0x7F) << 15) + (lo << 7)) | ((hi & 0x40) ? 0xFFC00000 | 0 : 0);
    if (hi & 0x80) v <<= 5;
    return v >>> 0;
  };
  // A write to DCSEL 2-6's registers (i: DCSEL * 4 + the register)
  function fxReg(i, b) {
    switch (i) {
      case 0x08: fx.mode = b & 3; fx.nib4 = !!(b & 4); fx.hop = !!(b & 8); fx.cycle = !!(b & 0x10); fx.fill = !!(b & 0x20);
        fx.cwrite = !!(b & 0x40); fx.trans = !!(b & 0x80); return;
      case 0x09: fx.tileBase = (b & 0xFC) << 9; fx.clip = !!(b & 2); fx.poly2 = !!(b & 1); return;
      case 0x0A: fx.mapBase = (b & 0xFC) << 9; fx.mapSize = 2 << ((b & 3) << 1); return;
      case 0x0B:
        fx.cincMode = !!(b & 1); fx.cnib = (b >> 1) & 1; fx.cbyte = (b >> 2) & 3; fx.mult = !!(b & 0x10); fx.sub = !!(b & 0x20);
        if (b & 0x40) fx.acc = (fx.sub ? fx.acc - fxProduct() : fx.acc + fxProduct()) | 0;
        if (b & 0x80) fx.acc = 0;
        return;
      case 0x0C: fx.xinc = fxIncrement(dc[0x0C], dc[0x0D]); return;
      case 0x0D: fx.xinc = fxIncrement(dc[0x0C], dc[0x0D]);
        if (fx.mode === 1 || fx.mode === 2) fx.xpos = ((fx.xpos & 0x07FF0000) | 0x8000) >>> 0;
        return;
      case 0x0E: fx.yinc = fxIncrement(dc[0x0E], dc[0x0F]); return;
      case 0x0F: fx.yinc = fxIncrement(dc[0x0E], dc[0x0F]);
        if (fx.mode === 1 || fx.mode === 2) fx.ypos = ((fx.ypos & 0x07FF0000) | 0x8000) >>> 0;
        return;
      case 0x10: fx.xpos = ((fx.xpos & 0x0700FF80) | (b << 16)) >>> 0; fxAffine(); return;
      case 0x11: fx.xpos = ((fx.xpos & 0x00FFFF00) | ((b & 7) << 24) | (b & 0x80)) >>> 0; fxAffine(); return;
      case 0x12: fx.ypos = ((fx.ypos & 0x0700FF80) | (b << 16)) >>> 0; fxAffine(); return;
      case 0x13: fx.ypos = ((fx.ypos & 0x00FFFF00) | ((b & 7) << 24) | (b & 0x80)) >>> 0; fxAffine(); return;
      case 0x14: fx.xpos = ((fx.xpos & 0x07FF0080) | (b << 8)) >>> 0; return;
      case 0x15: fx.ypos = ((fx.ypos & 0x07FF0080) | (b << 8)) >>> 0; return;
      case 0x18: case 0x19: case 0x1A: case 0x1B: fx.cache[i - 0x18] = b; return;
    }
  }
  // A read of DCSEL 5's fill length (0x16, 0x17), or 6's accumulator's side effects (0x18 reset, 0x19 accumulate)
  function fxRegRead(i) {
    if (i === 0x16) {
      if (fx.fillLen >= 768) return fx.poly2 && fx.mode === 2 ? 0 : 0x80;
      if (fx.nib4) {
        if (fx.poly2 && fx.mode === 2) return ((fx.ypos & 0x8000) >> 8) | ((fx.xpos >>> 11) & 0x60) | ((fx.xpos >>> 14) & 0x10) | ((fx.fillLen & 7) << 1) | ((fx.xpos & 0x8000) >> 15);
        return ((fx.fillLen & 0xFFF8 ? 1 : 0) << 7) | ((fx.xpos >>> 11) & 0x60) | ((fx.xpos >>> 14) & 0x10) | ((fx.fillLen & 7) << 1);
      }
      return ((fx.fillLen & 0xFFF0 ? 1 : 0) << 7) | ((fx.xpos >>> 11) & 0x60) | ((fx.fillLen & 0x0F) << 1);
    }
    if (i === 0x17) return (fx.fillLen & 0x03F8) >> 2;
    if (i === 0x18) fx.acc = 0;
    else if (i === 0x19) fx.acc = (fx.sub ? fx.acc - fxProduct() : fx.acc + fxProduct()) | 0;
    return -1;
  }

  // ---- The bus
  function read(r, t) {
    curT = t;
    if (t < readyAt) return -1;
    scan(t);
    switch (r) {
      case 0x00: return addr[addrsel] & 0xFF;
      case 0x01: return (addr[addrsel] >> 8) & 0xFF;
      case 0x02: return (addr[addrsel] >> 16) | (version ? (fx.nibBit[addrsel] << 1) | (fx.nibInc[addrsel] << 2) : nib[addrsel] << 1) | (inc[addrsel] << 3);
      case 0x03: case 0x04: {
        const s = r - 3;
        if (version) return fxRead(s);
        const b = rd[s]; step(s); fetch(s); return b;
      }
      case 0x05: return (dcsel << 1) | addrsel;
      case 0x06: return ((irqLine & 0x100) >> 1) | ((scanline(t) & 0x100) >> 2) | ien;
      case 0x07: pcm(t); return isr | (fcnt < AFLOW ? 8 : 0);
      case 0x08: return scanline(t) & 0xFF;
      case 0x09: case 0x0A: case 0x0B: case 0x0C: {
        const i = dcsel * 4 + r - 9;
        if (i === 0) return (dc[0] & 0x7F) | (field(t) << 7);
        if (i < 8 || i === 8 && version) return dc[i];
        if (version && i >= 0x16 && i <= 0x19) { const b = fxRegRead(i); if (b >= 0) return b; }
        return verBytes ? verBytes[i & 3] : 0;              // (Write-only: the version's bytes)
      }
      case 0x1B: pcm(t); return actl | (fcnt >= FIFO - 1 ? 0x80 : 0) | (fcnt === 0 ? 0x40 : 0);
      case 0x1C: return arate;
      case 0x1D: return 0;
      case 0x1E: {
        const b = spiIn;
        if (autotx && t >= spiBusyTo) { spiBusyTo = t + spiTime(); spiIn = spiByte(0xFF); }
        return b;
      }
      case 0x1F: return (t < spiBusyTo ? 0x80 : 0) | (autotx << 2) | (slow << 1) | ss;
      default: return r < 0x14 ? layer[0][r - 0x0D] : layer[1][r - 0x14];
    }
  }
  function write(r, b, t) {
    curT = t;
    if (t < readyAt) return;
    scan(t);
    switch (r) {
      case 0x00:
        if (version && fx.poly2 && fx.nib4 && fx.mode === 2 && addrsel === 1) { fx.poking = true; addr[1] = (addr[1] & 0x1FFFC) | (b & 3); }
        else { addr[addrsel] = (addr[addrsel] & 0x1FF00) | b; if (fx.hop && addrsel === 1) fx.hopAlign = b & 3; }
        fetch(addrsel); return;
      case 0x01: addr[addrsel] = (addr[addrsel] & 0x100FF) | (b << 8); fetch(addrsel); return;
      case 0x02: addr[addrsel] = (addr[addrsel] & 0x0FFFF) | ((b & 1) << 16); nib[addrsel] = (b >> 1) & 3; inc[addrsel] = b >> 3;
        fx.nibBit[addrsel] = (b >> 1) & 1; fx.nibInc[addrsel] = (b >> 2) & 1;
        fetch(addrsel); return;
      case 0x03: case 0x04: {
        const s = r - 3;
        if (version) { fxWrite(s, b); return; }
        put(addr[s], b); step(s); fetch(s); return;
      }
      case 0x05:
        if (b & 0x80) { reconfigure(t); return; }
        dcsel = (b >> 1) & dcselMask; addrsel = b & 1; return;
      case 0x06: irqLine = (irqLine & 0xFF) | ((b & 0x80) << 1); ien = b & 0x0F; plan(); return;
      case 0x07: isr &= ~b; return;
      case 0x08: irqLine = (irqLine & 0x100) | b; plan(); return;
      case 0x09: case 0x0A: case 0x0B: case 0x0C: {
        const i = dcsel * 4 + r - 9;
        if (version && i >= 0xFC) return;                     // (DCSEL 63: read-only)
        if (i === 0) { dc[0] = b & 0x7F; retime(t); return; }
        dc[i] = b;
        if (version && i >= 8) fxReg(i, b);
        return;
      }
      case 0x1B:
        pcm(t);
        if ((b & 0xC0) === 0xC0) loop = true;
        else { loop = false; if (b & 0x80) { fcnt = fwr = frd = 0; pcmFed = false; } }
        if (b & 0x40) { frd = 0; fcnt = fwr; }
        actl = b & 0x3F; return;
      case 0x1C: pcm(t); arate = b > 128 ? 256 - b : b; return;
      case 0x1D: pcm(t);
        if (fcnt < FIFO - 1) {
          fifo[fwr] = b; fwr = (fwr + 1) % FIFO; fcnt++; v.pcmIn++; pcmFed = true;
          if (v.pcmLog && v.pcmLog.length < PCM_LOG_MAX) v.pcmLog.push(b);
        } else v.pcmLost++;
        return;
      case 0x1E: if (t >= spiBusyTo) { spiBusyTo = t + spiTime(); spiIn = spiByte(b); } return;
      case 0x1F:
        if (ss && !(b & 1) && card) card.deselect();
        ss = b & 1; slow = (b >> 1) & 1; autotx = (b >> 2) & 1; return;
      default: if (r < 0x14) layer[0][r - 0x0D] = b; else layer[1][r - 0x14] = b;
    }
  }
  const spiTime = () => Math.max(1, Math.round(8 * clock / (slow ? 0.390625 : 12.5)));
  function scanline(t) {
    const uf = Math.max(0, unitAt(t)) % perFrame, s = perFrame === 525 ? uf : uf % 525;
    return s >= 512 ? 0x1FF : s;
  }
  const field = t => { const uf = Math.max(0, unitAt(t)) % perFrame; return perFrame === 525 ? uf & 1 : (uf >= 525 ? 1 : 0); };
  // CTRL's reset bit, or RESB: the FPGA configures itself again
  function reconfigure(t) { readyAt = t + configCycles; reset(readyAt); }

  // ---- The interrupt, and the next event
  function irqActive() {
    if (curT < readyAt) return false;
    return ((isr | (fcnt < AFLOW ? 8 : 0)) & ien) !== 0;
  }
  function tick(t) { curT = t; if (t >= readyAt) { scan(t); if (ien & 8) pcm(t); } }
  function nextEvent(devCyc) {
    if (devCyc < readyAt) return Math.max(1, readyAt - devCyc);
    let n = Infinity;                                         // (Only an interrupt that's on: VSYNC's and SPRCOL's
    if (ien & 7) {                                            //   at VSYNC, LINE's at its line)
      const u = after(!!(ien & 5), !!(ien & 2));
      if (u < Infinity) n = Math.max(1, cycOf(u) - devCyc);
    }
    if (ien & 8) n = Math.min(n, aflowIn(devCyc));
    return n;
  }

  // ---- The picture
  const lineL = [new Uint8Array(W), new Uint8Array(W)], sprCol = new Uint8Array(W), sprZ = new Uint8Array(W), sprMask = new Uint8Array(W);
  // A layer's properties from its registers
  function props(n) {
    const L = layer[n], depth = L[0] & 3, bitmap = !!(L[0] & 4);
    const p = { depth, bitmap, text: depth === 0 && !bitmap, t256: !!(L[0] & 8), mapBase: L[1] << 9, tileBase: (L[2] & 0xFC) << 9,
      hscroll: bitmap ? 0 : L[3] | (L[4] & 0xF) << 8, vscroll: bitmap ? 0 : L[5] | (L[6] & 0xF) << 8, palOfs: L[4] & 0xF };
    if (bitmap) { p.tilew = (L[2] & 1) ? 640 : 320; p.tileh = H; }
    else {
      p.mapwLog = 5 + ((L[0] >> 4) & 3); p.maphLog = 5 + ((L[0] >> 6) & 3);
      p.tilewLog = 3 + (L[2] & 1); p.tilehLog = 3 + ((L[2] >> 1) & 1);
      p.tilew = 1 << p.tilewLog; p.tileh = 1 << p.tilehLog;
      p.wMask = (1 << (p.mapwLog + p.tilewLog)) - 1; p.hMask = (1 << (p.maphLog + p.tilehLog)) - 1;
      p.tileLog = p.tilewLog + p.tilehLog + depth - 3;         // (A tile's bytes, log 2)
    }
    return p;
  }
  // Layer n's line y (of its own: after the scale) into lineL[n]
  function layerLine(n, y) {
    const p = props(n), out = lineL[n], bpp = 1 << p.depth;
    if (p.bitmap) {
      const rowBytes = (p.tilew * bpp) >> 3, base = p.tileBase + (y % p.tileh) * rowBytes;
      for (let x = 0; x < W; x++) {
        const xx = x % p.tilew, s = vram[(base + ((xx * bpp) >> 3)) & 0x1FFFF];
        let c = (s >> (8 - bpp - ((xx & ((8 >> p.depth) - 1)) << p.depth))) & ((1 << bpp) - 1);
        if (c > 0 && c < 16) { c += p.palOfs << 4; if (p.t256) c |= 0x80; }
        out[x] = c;
      }
      return;
    }
    const ey = (y + p.vscroll) & p.hMask, row = ey >> p.tilehLog, yy = ey & (p.tileh - 1);
    const mapRow = p.mapBase + ((row << p.mapwLog) << 1);
    for (let x = 0; x < W; x++) {
      const ex = (x + p.hscroll) & p.wMask, col = ex >> p.tilewLog;
      const m = (mapRow + (col << 1)) & 0x1FFFF, b0 = vram[m], b1 = vram[(m + 1) & 0x1FFFF];
      let xx = ex & (p.tilew - 1);
      if (p.text) {
        const s = vram[(p.tileBase + (b0 << p.tileLog) + ((yy << p.tilewLog) >> 3) + (xx >> 3)) & 0x1FFFF];
        const on = (s >> (7 - (xx & 7))) & 1;
        out[x] = p.t256 ? (on ? b1 : 0) : (on ? b1 & 15 : b1 >> 4);
      } else {
        const tile = b0 | ((b1 & 3) << 8), hflip = b1 & 4, vflip = b1 & 8, yr = vflip ? yy ^ (p.tileh - 1) : yy;
        if (hflip) xx ^= p.tilew - 1;
        const s = vram[(p.tileBase + (tile << p.tileLog) + ((yr << p.tilewLog) * bpp >> 3) + ((xx * bpp) >> 3)) & 0x1FFFF];
        let c = (s >> (8 - bpp - ((xx & ((8 >> p.depth) - 1)) << p.depth))) & ((1 << bpp) - 1);
        if (c > 0 && c < 16) { c += b1 & 0xF0; if (p.t256) c |= 0x80; }
        out[x] = c;
      }
    }
  }
  // The sprites' line y into sprCol, sprZ, sprMask (as the renderer has it: in order, 800 clocks a line at most);
  // OUT: the collisions on it (the masks, bits 4-7)
  function spriteLine(y) {
    sprCol.fill(0); sprZ.fill(0); sprMask.fill(0);
    let budget = 801, coll = 0;
    for (let i = 0; i < 128; i++) {
      if (--budget === 0) break;
      const a = i * 8, z = (spr[a + 6] >> 2) & 3;
      if (!z) continue;
      const wLog = ((spr[a + 7] >> 4) & 3) + 3, hLog = (spr[a + 7] >> 6) + 3, w = 1 << wLog, h = 1 << hLog;
      let sx = spr[a + 2] | (spr[a + 3] & 3) << 8, sy = spr[a + 4] | (spr[a + 5] & 3) << 8;
      if (sx >= 0x400 - w) sx -= 0x400;
      if (sy >= 0x400 - h) sy -= 0x400;
      if (y < sy || y >= sy + h) continue;
      const mode8 = spr[a + 1] >> 7, base = (spr[a] << 5) | ((spr[a + 1] & 0xF) << 13), mask = spr[a + 6] & 0xF0;
      const hflip = spr[a + 6] & 1, vflip = spr[a + 6] & 2, palOfs = (spr[a + 7] & 0xF) << 4;
      const ry = vflip ? h - 1 - (y - sy) : y - sy, rowAt = base + (ry << (wLog - (1 - mode8))), fetchMask = ((2 - mode8) << 2) - 1;
      for (let px = 0; px < w; px++) {
        const lx = sx + px;
        if (lx < 0 || lx >= W) continue;
        if (!(px & fetchMask) && --budget === 0) break;
        if (--budget === 0) break;
        const ix = hflip ? w - 1 - px : px;
        let c = mode8 ? vram[(rowAt + ix) & 0x1FFFF] : (vram[(rowAt + (ix >> 1)) & 0x1FFFF] >> ((ix & 1) ? 0 : 4)) & 15;
        if (!c) continue;
        coll |= sprMask[lx] & mask; sprMask[lx] |= mask;
        if (z > sprZ[lx]) { if (c < 16) c += palOfs; sprCol[lx] = c; sprZ[lx] = z; }
      }
      if (budget <= 0) break;
    }
    return coll;
  }
  // The frame's sprite collisions (VSYNC's, not live): each line the composer would show, once
  function frameCollisions() {
    let any = false;
    for (let i = 0; i < 128 && !any; i++) if ((spr[i * 8 + 6] & 0x0C) && (spr[i * 8 + 6] & 0xF0)) any = true;
    if (!any) return 0;
    let coll = 0, last = -1;
    const vs = dc[6] << 1, ve = Math.min(H, dc[7] << 1);
    for (let y = vs; y < ve; y++) {
      const ey = effY(y);
      if (ey === last) continue;
      last = ey; coll |= spriteLine(ey);
    }
    return coll;
  }
  const effY = y => { const ey = ((y - (dc[6] << 1)) * dc[2]) >> 7; return ey >= H ? H - 1 : ey; };
  // Screen line y (0-479) as palette indexes into out (640), from the registers as they are
  function drawLine(y, out) {
    const mode = dc[0] & 3;
    if (!mode) { out.fill(0); return 0; }
    const vs = dc[6] << 1, ve = dc[7] << 1, border = dc[3];
    if (y < vs || y >= ve) { out.fill(border); return 0; }
    const ey = effY(y), en0 = dc[0] & 0x10, en1 = dc[0] & 0x20, enS = dc[0] & 0x40;
    let coll = 0;
    if (en0) layerLine(0, ey); else lineL[0].fill(0);
    if (en1) layerLine(1, ey); else lineL[1].fill(0);
    if (enS) coll = spriteLine(ey); else { sprCol.fill(0); sprZ.fill(0); }
    const hs = Math.min(W, dc[4] << 2), he = Math.min(W, dc[5] << 2), sc = dc[1], l0 = lineL[0], l1 = lineL[1];
    for (let x = 0; x < W; x++) {
      if (x < hs || x >= he) { out[x] = border; continue; }
      const ex = ((x - hs) * sc) >> 7;
      if (ex >= W) { out[x] = 0; continue; }
      const z = sprZ[ex], s = sprCol[ex], a = l0[ex], b = l1[ex];
      out[x] = z === 3 ? (s || b || a) : z === 2 ? (b || s || a) : z === 1 ? (b || a || s) : (b || a);
    }
    return coll;
  }
  // The palette as 0xRRGGBB (greyscale with chroma off in NTSC)
  function rgbPalette() {
    const out = new Uint32Array(256), grey = (dc[0] & 7) === 6;
    for (let i = 0; i < 256; i++) {
      const e = pal[2 * i] | pal[2 * i + 1] << 8;
      let r = ((e >> 8) & 15) * 17, g = ((e >> 4) & 15) * 17, b = (e & 15) * 17;
      if (grey) r = g = b = Math.round((r + g + b) / 3);
      out[i] = (r << 16) | (g << 8) | b;
    }
    return out;
  }
  // Live: unit uf's line drawn into fb
  const lineBuf = new Uint8Array(W);
  function drawUnit(uf) {
    let y;
    if (perFrame === 525) y = uf;
    else { y = lineOf(uf); if (y < 0 || (y & 1)) return; if (uf >= 525) y |= 1; }
    if (y < 0 || y >= H) return;
    if (!fb) fb = new Uint8Array(W * H);
    collisions |= drawLine(y, lineBuf);
    fb.set(lineBuf, y * W);
  }
  // The screen now: { pixels: 640 x 480 palette indexes, rgb: the palette (0xRRGGBB) }
  function frame() {
    const pixels = new Uint8Array(W * H);
    for (let y = 0; y < H; y++) { drawLine(y, lineBuf); pixels.set(lineBuf, y * W); }
    return { pixels, rgb: rgbPalette() };
  }
  // The text layer's screen (layer 1's if it's text and on, else layer 0's): { rows, cols, chars (character
  // indexes), attrs (the map's second bytes), layer }, the rows and columns the active area shows; or null
  function cells() {
    const on = [dc[0] & 0x10, dc[0] & 0x20];
    let n = -1;
    for (const k of [1, 0]) { const p = props(k); if (on[k] && p.text) { n = k; break; } }
    if (n < 0 || !(dc[0] & 3)) return null;
    const p = props(n);
    const hpx = Math.ceil((Math.min(W, dc[5] << 2) - Math.min(W, dc[4] << 2)) * dc[1] / 128);
    const vpx = Math.ceil((Math.min(H, dc[7] << 1) - (dc[6] << 1)) * dc[2] / 128);
    const cols = Math.max(0, Math.ceil(hpx / p.tilew)), rows = Math.max(0, Math.ceil(vpx / p.tileh));
    const chars = new Uint8Array(rows * cols), attrs = new Uint8Array(rows * cols);
    for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++) {
      const ey = (r * p.tileh + p.vscroll) & p.hMask, ex = (c * p.tilew + p.hscroll) & p.wMask;
      const m = p.mapBase + ((((ey >> p.tilehLog) << p.mapwLog) + (ex >> p.tilewLog)) << 1);
      chars[r * cols + c] = vram[m & 0x1FFFF]; attrs[r * cols + c] = vram[(m + 1) & 0x1FFFF];
    }
    return { rows, cols, chars, attrs, layer: n };
  }
  // The text layer as text: a string a row, its trailing spaces gone (characters as ISO-8859-1, the console's
  // font: below space, and $7F-$9F, as '.'); [] if there's none
  function text() {
    const c = cells();
    if (!c) return [];
    const out = [];
    for (let r = 0; r < c.rows; r++) {
      let s = '';
      for (let k = 0; k < c.cols; k++) { const ch = c.chars[r * c.cols + k]; s += (ch < 32 || (ch >= 0x7F && ch < 0xA0)) ? '.' : String.fromCharCode(ch); }
      out.push(s.replace(/ +$/, ''));
    }
    return out;
  }
  // A register as it reads, without a read's side effects (the data ports' fetched bytes, no step)
  function peek(r) {
    if (curT < readyAt) return -1;
    if (r === 3 || r === 4) return rd[r - 3];
    if (r === 0x1E) return spiIn;
    return read(r, curT);
  }
  reset(configCycles);                                        // (Power-up: configured at configCycles)
  Object.defineProperties(v, {
    live: { get: () => live, set: x => { live = !!x; plan(); } },
    ready: { get: () => curT >= readyAt },
    fifo: { get: () => fcnt },
    ien: { get: () => ien }, isr: { get: () => isr },
    dcVideo: { get: () => dc[0] }, fx: { get: () => fx }, layers: { get: () => [Array.from(layer[0]), Array.from(layer[1])] },
    addr: { get: () => [addr[0], addr[1]] },
  });
  // The sound on: each sample given to fn(left, right) (16 bits each), from cycle 0
  function sound(fn) { sink = fn; made = 0; }
  Object.assign(v, { read, write, tick, irqActive, nextEvent, reset: t => reconfigure(t), frame, cells, text, peek, rgbPalette, sound, soundTo });
  return v;
}

module.exports = { createVera, DEFAULT_PALETTE };
