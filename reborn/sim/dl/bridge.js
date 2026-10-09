// bridge.js - a test (tests/tests.js) run in the danlang emulator (sim/dl/hydra.dl) instead of sim/lib's: its spec
// written (the images, the cards, the keys, the machine's options, the marks), danlang run on it, and what came out
// read back as the JS machine's would be, for sim/test.js to judge (its marks, budgets, IRQs-off stretch and checks):
// a Vera X's VRAM and registers into a JS one (sim/lib/vera.js's load), so a check that looks at the screen draws
// it as the JS machine's would (frame, cells, text); its card's blocks written into the test's card, as the VIA's.
// The PC's end of /pc or of XMODEM (sim/lib/pchost.js, xmpeer.js) stays here, in the harness: each byte the Hydra's
// serial port sends comes over danlang's stdout, and the console's bytes and the PC's go back over its stdin.
//
// The danlang interpreter: $DANLANG, or the Release build beside this repository (../danlang: dotnet build -c Release).
// The emulator: $HYDRA_DL (its hydra.dl, the others beside it: a copy, to go on changing these while a run uses it), or
// this folder's.
'use strict';
const fs = require('fs');
const path = require('path');
const { spawn, spawnSync } = require('child_process');
const { createVera } = require('../../../base/sim/lib/vera.js');

const ROOT = path.join(__dirname, '..', '..');
const HYDRA_DL = process.env.HYDRA_DL || path.join(__dirname, 'hydra.dl');

function danlang() {
  if (process.env.DANLANG) return process.env.DANLANG;
  const exe = path.join(ROOT, '..', '..', 'danlang', 'bin', 'Release', 'net6.0', process.platform === 'win32' ? 'danlang.exe' : 'danlang');
  if (fs.existsSync(exe)) return exe;
  throw new Error('no danlang: set DANLANG, or build ../danlang (dotnet build -c Release)');
}

// A string as danlang reads it (its bytes; escapes for the others)
const dlStr = s => '"' + [...s].map(c => {
  const n = c.charCodeAt(0);
  return c === '"' ? '\\"' : c === '\\' ? '\\\\' : n < 32 || n > 126 ? '\\x' + n.toString(16).padStart(2, '0') : c;
}).join('') + '"';
const dlList = l => '{' + l.join(' ') + '}';
const dlPath = p => dlStr(p.split(path.sep).join('/'));

// The keys typed: a code each (256: wait 2M cycles, 257: wait for a prompt: sim/lib/acia.js)
const keys = input => [...(input || '')].map(c => c.charCodeAt(0));

// The spec for test t (its machine's options m), in dir; the cards' files
function writeSpec(t, m, opt, dir, extra) {
  const out = [];
  const set = (name, value) => out.push('(set! {' + name + '} ' + value + ')');
  out.push('(def {spec-bios spec-prom} ' + dlPath(path.join(ROOT, 'bin', 'bios.bin')) + ' ' + dlPath(path.join(dir, 'prom.bin')) + ')');
  set('spec-cycles', t.cycles);
  set('spec-init', t.expect ? '""' : dlStr(t.init));
  set('spec-expect', t.expect ? dlList(t.expect.map(e => dlStr(e))) : 'NIL');
  set('spec-marks', dlList(extra.marks.map(dlStr)));
  set('spec-keys', dlList(keys(m.input)));
  if (t.send) { set('spec-send-after', dlStr(t.send.after)); set('spec-send', dlList(t.send.bytes)); }
  set('spec-boot-done', extra.bootDone === undefined ? -1 : extra.bootDone);
  set('spec-out', dlPath(dir));
  set('spec-peer', extra.peer ? 'T' : 'NIL');
  set('spec-paste', m.paste ? 'T' : 'NIL');
  set('opt-seed', opt.seed === undefined ? 1 : opt.seed);
  if (m.modules !== undefined) set('opt-modules', m.modules);
  if (m.sharedU !== undefined) set('opt-sharedu', m.sharedU);
  if (m.model) set('opt-model', dlStr(m.model));
  if (m.u7Fault) { set('opt-u7', 'T'); set('opt-u7mask', m.u7Fault.mask); set('opt-u7high', m.u7Fault.high ? 'T' : 'NIL'); }
  if (m.ramFault) { set('opt-ramfault', 'T'); set('opt-rfbank', m.ramFault.bank); set('opt-rfmask', m.ramFault.mask); set('opt-rfhigh', m.ramFault.high ? 'T' : 'NIL'); }
  if (m.stuckIrq !== undefined) set('opt-stuckirq', m.stuckIrq);
  if (m.aciaLine !== undefined) set('opt-acialine', m.aciaLine);
  if (m.acia === 'wdc') set('ac-wdc', 'T');
  if (m.clock !== undefined) set('clock-mhz', m.clock);
  if (m.ymResetDelay) set('ym-resetdelay', m.ymResetDelay);
  if (m.ymLog) set('ym-log', 'T');
  if (m.gpioIn !== undefined) set('opt-portain', m.gpioIn);
  if (m.ca1) out.push('(ca1-setup ' + dlList([...m.ca1].sort((a, b) => a - b)) + ')');
  for (const [a, size] of Object.entries(m.i2c || {})) out.push('(i2c-device ' + (+a) + ' ' + size + ')');
  if (m.rtc !== undefined) out.push('(rtc-setup ' + (typeof m.rtc === 'number' ? m.rtc : ':' + m.rtc) + ' ' + (m.rtcBatteryLow ? 'T' : 'NIL') + ')');
  for (const dev of m.spiEcho || []) out.push('(spi-echo ' + dev + ')');
  for (const c of m.sd || []) {
    let file = c.file;
    if (!file) {                                            // (A card in memory: its blocks, a file for danlang)
      file = path.join(dir, 'card' + c.dev + '.img');
      fs.writeFileSync(file, c.data || Buffer.alloc(0));
    }
    out.push('(spi-card ' + c.dev + ' ' + dlPath(file) + ' ' + c.blocks + ' ' + (c.sdsc ? 'T' : 'NIL') + ')');
  }
  if (m.vera) {                                             // (A Vera X: its version, its configuring's cycles, the PCM
    const vo = m.vera === true ? {} : m.vera;               //   log; a card on its SPI controller, spi.dl's device 16)
    const ver = vo.version === undefined ? [47, 0, 2] : vo.version;
    out.push('(vera-setup ' + (ver ? dlList(ver) : 'NIL') + ' ' + (vo.configCycles === undefined ? -1 : vo.configCycles) + ' ' + (vo.pcmLog ? 'T' : 'NIL') + ')');
    if (vo.sd) {
      let file = vo.sd.file;
      if (!file) { file = path.join(dir, 'card16.img'); fs.writeFileSync(file, vo.sd.data || Buffer.alloc(0)); }
      out.push('(spi-card 16 ' + dlPath(file) + ' ' + vo.sd.blocks + ' ' + (vo.sd.sdsc ? 'T' : 'NIL') + ')', '(vera-card)');
    }
  }
  if (m.smc) {                                              // (Its input controller: the mouse, the moves typed)
    const so = m.smc === true ? {} : m.smc;
    out.push('(smc-setup ' + (so.mouse === false ? 'NIL' : 'T') + ' ' + dlList((so.moves || []).map(dlList)) + ')');
  }
  fs.writeFileSync(path.join(dir, 'spec.dl'), out.join('\n') + '\n', 'latin1');
}

// Test t run in danlang.  extra: { image (the paged ROM), marks, bootDone, peer (a PC host or an XMODEM peer: push,
// send, flush) }.  OUT: (a promise) { m: the machine as the checks see it, marks: { name: cycle } }
async function runDl(t, machine, opt, extra) {
  const dir = path.join(ROOT, 'obj', 'dl', t.name);
  fs.rmSync(dir, { recursive: true, force: true });
  fs.mkdirSync(dir, { recursive: true });
  fs.writeFileSync(path.join(dir, 'prom.bin'), extra.image);
  writeSpec(t, machine, opt, dir, extra);
  const args = [HYDRA_DL, path.join(dir, 'spec.dl')];
  let errText = '', peerOut = [];
  if (!extra.peer) {
    const r = spawnSync(danlang(), args, { encoding: 'latin1', maxBuffer: 1 << 26 });
    if (r.error) throw r.error;
    errText = (r.stdout || '') + (r.stderr || '');
  } else {
    // The peer: each byte the Hydra sends is pushed to it; what it gives back (the console's bytes) and what it
    // sends (the PC's) go back to danlang at once
    const p = extra.peer;
    let pcBytes = [];
    p.send = bytes => { pcBytes.push(...bytes); };
    await new Promise((resolve, reject) => {
      const child = spawn(danlang(), args, { stdio: ['pipe', 'pipe', 'pipe'] });
      let buf = '';
      child.stdout.setEncoding('latin1');
      child.stdout.on('data', d => {
        buf += d;
        let nl;
        while ((nl = buf.indexOf('\n')) >= 0) {
          const line = buf.slice(0, nl).replace(/\r$/, '');
          buf = buf.slice(nl + 1);
          const k = line.match(/^T (\d+) (\d+)$/);
          if (!k) { errText += line + '\n'; continue; }
          pcBytes = [];
          const console_ = p.push(+k[1], +k[2]);
          const hex = a => Buffer.from(a).toString('hex');
          child.stdin.write('C' + hex(console_) + ';P' + hex(pcBytes) + '\n');
        }
      });
      child.stderr.setEncoding('latin1');
      child.stderr.on('data', d => { errText += d; });
      child.on('error', reject);
      child.on('close', () => resolve());
    });
    peerOut = p.flush ? [...p.flush()] : [];
  }
  const res = path.join(dir, 'result.json');
  if (!fs.existsSync(res)) throw new Error('the danlang emulator gave no result: ' + errText.trim().slice(-2000));
  const r = JSON.parse(fs.readFileSync(res, 'latin1'));
  let out = fs.readFileSync(path.join(dir, 'out.txt'), 'latin1');
  if (peerOut.length) out += String.fromCharCode(...peerOut);
  // The cards' blocks written, back in the cards (as the JS machine's writes go)
  const cards = fs.readFileSync(path.join(dir, 'cards.bin'));
  for (let i = 0; i + 517 <= cards.length; i += 517) {
    const dev = cards[i], n = cards.readUInt32LE(i + 1);
    const card = dev === 16 ? machine.vera && machine.vera.sd : (machine.sd || []).find(c => c.dev === dev);   // (16: the Vera X's)
    if (card) card.write(n, Uint8Array.from(cards.subarray(i + 5, i + 517)));
  }
  const marks = {};
  for (const [i, at] of Object.entries(r.marks)) marks[extra.marks[+i]] = at;
  const viaR = new Array(16).fill(0);
  viaR[0x0C] = r.via.pcr;
  const m = {
    cpu: { cyc: r.cycles, halted: r.halted || '' },
    out,
    iOffTop: r.ioff,
    ym: { keyOns: r.keyOns, lost: r.ymLost, regs: Uint8Array.from(r.ymRegs), writes: r.ymWrites },
    i2c: { devices: new Map(Object.entries(r.i2c.devices).map(([a, mem]) => [+a, { mem: Uint8Array.from(mem) }])), stats: r.i2c.stats },
    via: { ier: r.via.ier, r: viaR },
    rtc: r.rtc ? { regs: () => r.rtc.slice() } : null,
    acia: { gapMin: r.acia.gapMin === null ? Infinity : r.acia.gapMin, overruns: r.acia.overruns, wdc: r.acia.wdc, pcLost: r.acia.pcLost,
      rxLat: Object.assign({}, r.acia.rxLat, { min: r.acia.rxLat.min === null ? Infinity : r.acia.rxLat.min }),
      charCycles: () => r.acia.charCycles, rxLost: r.acia.rxLost },
    dl: { errText },
  };
  if (r.vera) {                                             // The Vera X: a JS one, as danlang's left it (for frame, cells, text)
    const vb = fs.readFileSync(path.join(dir, 'vera.bin')), vo = machine.vera === true ? {} : machine.vera;
    const v = createVera({ clock: machine.clock, rnd: () => 0, version: vo.version });
    let at = 0;
    const take = n => vb.subarray(at, at += n);
    v.load({ vram: take(131072), palette: take(512), sprites: take(1024), psg: take(64), dc: take(256), layers: take(14) });
    Object.assign(v, { frames: r.vera.frames, psgOns: r.vera.psgOns, pcmIn: r.vera.pcmIn, pcmOut: r.vera.pcmOut, pcmLost: r.vera.pcmLost,
      pcmUnderruns: r.vera.pcmUnderruns, pcmLog: r.vera.pcmLog === null ? null : [...take(r.vera.pcmLog)] });
    m.vera = v;
  }
  if (r.smc) m.smc = r.smc;
  return { m, marks };
}

module.exports = { runDl };
