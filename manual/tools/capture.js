#!/usr/bin/env node
// capture.js - the book's examples, from the Hydra itself: HydraOS booted in the emulator (reborn/sim/run.js's
// machine), a script's lines typed at its prompts one by one, and what the console showed, cleaned up as a terminal
// would show it (its escape sequences acted on or dropped; CR LF as a line's end).
//
// Usage: node manual/tools/capture.js [options] SCRIPT|-
//   SCRIPT      a file of lines to type, each after a prompt ("/> ", "% ", "> " ...); "-" reads them from stdin.
//               A line "#! OPTIONS" sets options (as below); "## comment" is skipped; a line starting "!raw " is
//               typed at once, without waiting for a prompt; "!wait N" runs N million cycles; "!quiet" waits for
//               the output to stop; "!line TEXT" types TEXT and Return once the output stops; "!snap NAME" takes a
//               picture of the screen ("!png NAME": of the Vera X's screen, figures/NAME.png).  A prompt is what a program waiting for a line shows: output stopped part-way
//               along a line, and still a while.  In a line, \e is ESC, \r Return, \t Tab, \xHH a byte, ^X a control
//               key (^] is Ctrl-]), \\ a backslash, \^ a caret.
//   --boot      the boot's messages too (otherwise the transcript starts at the first prompt)
//   --vera      a Vera X card in slot 0; --smc its keyboard and mouse; --rtc the clock chip, set to 2026-10-12
//               09:41:00 (so dates in the examples don't change)
//   --modules N RAM modules (3: the book's machine)
//   --no-sd     no SD card (the default: the system's own card, bin/sdcard.img, its writes kept in memory)
//   --cycles M  at most M million cycles in all (default 4000)
//   --raw       the output as it came, escape sequences and all
//   --figs DIR  where !snap NAME writes NAME.svg: the terminal's screen (80 x 24) at that point, as a picture (the
//               default: manual/figures).  A script with snapshots runs each part till its keys are typed and the
//               output has stopped, then takes the picture, then types the next part
'use strict';
const fs = require('fs');
const path = require('path');
const REBORN = path.join(__dirname, '..', '..', 'reborn');
const run = require(path.join(REBORN, 'sim', 'run.js'));
const { VT } = require(path.join(REBORN, '..', 'base', 'sim', 'lib', 'vt.js'));
const { encodePng } = require(path.join(REBORN, '..', 'base', 'sim', 'lib', 'png.js'));

const RTC_TIME = Date.UTC(2026, 9, 12, 9, 41, 0) / 1000;

function parseArgs(argv) {
  const o = { boot: false, vera: false, smc: false, rtc: false, modules: 3, sd: true, cycles: 4000, raw: false, figs: path.join(__dirname, '..', 'figures'), file: null };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--boot') o.boot = true;
    else if (a === '--vera') o.vera = true;
    else if (a === '--smc') o.smc = true;
    else if (a === '--rtc') o.rtc = true;
    else if (a === '--no-sd') o.sd = false;
    else if (a === '--raw') o.raw = true;
    else if (a === '--modules') o.modules = +argv[++i];
    else if (a === '--cycles') o.cycles = +argv[++i];
    else if (a === '--figs') o.figs = argv[++i];
    else o.file = a;
  }
  return o;
}

// The system's SD card in device 0, as run.js has it: the image read, its writes kept in memory
function cardCopy(dev, file) {
  const base = fs.readFileSync(file), written = new Map();
  return { dev, blocks: Math.floor(base.length / 512), file,
    read: n => written.get(n) || Buffer.from(base.subarray(n * 512, n * 512 + 512)),
    write: (n, b) => { written.set(n, Buffer.from(b)); } };
}

// A script line's keys
function keys(line) {
  return line.replace(/\\\\|\\\^|\\e|\\r|\\t|\\x([0-9a-fA-F]{2})|\^([@A-Z\[\\\]^_?])/g, (m, hex, ctl) => {
    if (m === '\\\\') return '\\';
    if (m === '\\^') return '^';
    if (m === '\\e') return '\x1b';
    if (m === '\\r') return '\r';
    if (m === '\\t') return '\t';
    if (hex) return String.fromCharCode(parseInt(hex, 16));
    if (ctl === '?') return '\x7f';
    return String.fromCharCode(ctl.charCodeAt(0) & 0x1f);
  });
}

// The console's output as a terminal shows it: a small screen-less interpreter of what the shells and editors send
// (CR, LF, BS, TAB; CSI's erase and cursor moves within the line), enough for a transcript
function clean(out) {
  const lines = [];
  let cur = [], col = 0;
  const put = ch => { while (cur.length < col) cur.push(' '); cur[col] = ch; col++; };
  const nl = () => { lines.push(cur.join('').replace(/\s+$/, '')); cur = []; col = 0; };
  for (let i = 0; i < out.length; i++) {
    const c = out[i];
    if (c === '\x1b') {
      // CSI sequences, and the two-byte ones
      if (out[i + 1] === '[') {
        let j = i + 2; let params = '';
        while (j < out.length && /[0-9;?<>=!]/.test(out[j])) params += out[j++];
        const fin = out[j];
        const n = parseInt(params.replace(/^\?/, ''), 10);
        if (fin === 'K') { if (!params || params === '0') cur.length = Math.min(cur.length, col); else if (params === '2') cur = []; }
        else if (fin === 'G') col = Math.max(0, (isNaN(n) ? 1 : n) - 1);
        else if (fin === 'C') col += isNaN(n) ? 1 : n;
        else if (fin === 'D') col = Math.max(0, col - (isNaN(n) ? 1 : n));
        else if (fin === 'J' && (params === '2' || params === '')) { /* (a clear: a blank line in a transcript) */ }
        i = j; continue;
      }
      if (out[i + 1] === ']') { const e = out.indexOf('\x07', i); i = e > 0 ? e : i + 1; continue; }   // (OSC: a title)
      if ('()'.includes(out[i + 1])) { i += 2; continue; }
      i += 1; continue;
    }
    if (c === '\r') { col = 0; continue; }
    if (c === '\n') { nl(); continue; }
    if (c === '\b') { col = Math.max(0, col - 1); continue; }
    if (c === '\t') { do put(' '); while (col % 8); continue; }
    if (c === '\x07' || c < ' ' && c !== '\t') continue;
    put(c);
  }
  if (cur.length) lines.push(cur.join('').replace(/\s+$/, ''));
  return lines;
}

// The terminal's colours (the 16 ANSI ones), its background, and the DEC Special Graphics as the PC shows them
const ANSI = ['#14181c', '#d0453a', '#3fae5a', '#d6b43a', '#3d74c9', '#b65cc6', '#3aaec4', '#d9d9d9',
  '#6b6b6b', '#f06a5e', '#62d47c', '#f2d65c', '#6c9cf0', '#d98be6', '#64d2e6', '#ffffff'];
const DEC_UNI = ' ◆▒␉␌␍␊°±␤␋┘┐┌└┼' +
  '⎺⎻─⎼⎽├┤┴┬│≤≥π≠£·';
const LINE_ARMS = { 11: 'lu', 12: 'ld', 13: 'rd', 14: 'ru', 15: 'lrud', 18: 'lr', 21: 'urd', 22: 'uld', 23: 'lru', 24: 'lrd', 25: 'ud' };
const xesc = t =>t.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

// A VT's screen as an SVG picture: its cells, their colours and renditions, and the cursor
function screenSvg(vt, { cw = 8.4, ch = 17, pad = 12, cursor = true } = {}) {
  const W = Math.round(vt.cols * cw + 2 * pad), H = vt.rows * ch + 2 * pad;
  const o = [`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${W} ${H}" font-family="Courier New, monospace" font-size="14">`,
    `<rect width="${W}" height="${H}" rx="8" fill="${ANSI[0]}"/>`];
  for (let y = 0; y < vt.rows; y++) {
    const row = vt.screen[y], ty = pad + y * ch;
    for (let x = 0; x < vt.cols;) {
      const c0 = row[x];
      let x2 = x + 1;
      while (x2 < vt.cols && row[x2].a === c0.a && row[x2].f === c0.f) x2++;
      let fg = c0.a & 15, bg = c0.a >> 4;
      if (c0.f & 16) [fg, bg] = [bg, fg];
      if (c0.f & 1 && fg < 8) fg += 8;
      const tx = pad + x * cw;
      if (bg !== 0) o.push(`<rect x="${tx.toFixed(1)}" y="${ty}" width="${((x2 - x) * cw).toFixed(1)}" height="${ch}" fill="${ANSI[bg]}"/>`);
      // (The DEC line-drawing characters as lines, joined cell to cell: their arms, as left, right, up, down)
      for (let i = x; i < x2; i++) {
        const arms = LINE_ARMS[row[i].c];
        if (row[i].c >= 0x20 || !arms) continue;
        const cx = pad + i * cw + cw / 2, cy = ty + ch / 2, l = pad + i * cw, r = l + cw;
        const seg = [];
        if (arms.includes('l')) seg.push(`M${l.toFixed(1)} ${cy}H${cx.toFixed(1)}`);
        if (arms.includes('r')) seg.push(`M${cx.toFixed(1)} ${cy}H${r.toFixed(1)}`);
        if (arms.includes('u')) seg.push(`M${cx.toFixed(1)} ${ty}V${cy}`);
        if (arms.includes('d')) seg.push(`M${cx.toFixed(1)} ${cy}V${ty + ch}`);
        o.push(`<path d="${seg.join('')}" stroke="${ANSI[fg]}" stroke-width="1.2" fill="none"/>`);
      }
      const text = row.slice(x, x2).map(k => k.c < 0x20 ? (LINE_ARMS[k.c] ? ' ' : DEC_UNI[k.c]) : String.fromCharCode(k.c)).join('');
      if (!(c0.f & 32) && text.trim()) {
        o.push(`<text x="${tx.toFixed(1)}" y="${ty + 13}" fill="${ANSI[fg]}"${c0.f & 1 ? ' font-weight="bold"' : ''} xml:space="preserve">${xesc(text)}</text>`);
      }
      if (c0.f & 4) o.push(`<line x1="${tx.toFixed(1)}" y1="${ty + 15}" x2="${(tx + (x2 - x) * cw).toFixed(1)}" y2="${ty + 15}" stroke="${ANSI[fg]}"/>`);
      x = x2;
    }
  }
  if (cursor && vt.tcem !== false) {
    o.push(`<rect x="${(pad + vt.x * cw).toFixed(1)}" y="${pad + vt.y * ch + 13}" width="${cw}" height="3" fill="${ANSI[7]}"/>`);
  }
  o.push('</svg>');
  return o.join('\n') + '\n';
}

// Waiting for the Hydra: a prompt is output that has stopped part-way along a line (its escape sequences aside) and
// stayed still a while, the keys typed so far taken
const QUIET = 2e6;                                              // Still this many cycles: it's waiting
const QUIET_SNAP = 10e6;                                        // (Before a picture, or !quiet: programs loading)
const STUCK = 60e6;                                             // Still this long, and no prompt: on regardless
const BUSY_SNAP = 40e6;                                         // (A picture's wait at most, the keys typed)
const STEP = 250000;

function capture(script, o) {
  const items = [];
  for (const raw of script.replace(/\r\n?/g, '\n').split('\n')) {
    if (raw.startsWith('#!')) {                               // (Only the options it names: the rest as they were)
      const d = parseArgs([]), p = parseArgs(raw.slice(2).trim().split(/\s+/).filter(Boolean));
      for (const k of Object.keys(p)) if (k !== 'file' && p[k] !== d[k]) o[k] = p[k];
      continue;
    }
    if (raw.startsWith('##')) continue;
    items.push(raw);
  }
  while (items.length && !items[items.length - 1].trim()) items.pop();
  const opt = { input: '', modules: o.modules, seed: 1, log: () => {} };
  if (o.vera) opt.vera = true;
  if (o.smc) opt.smc = true;
  if (o.rtc) opt.rtc = RTC_TIME;
  opt.sd = o.sd ? [cardCopy(0, path.join(REBORN, 'bin', 'sdcard.img'))] : [];
  const m = run.boot(opt);
  const limit = o.cycles * 1e6;
  const type = s => { for (const c of s) m.acia.rxQueue.push(c); };
  // Run till the keys are typed and the output has been still QUIET cycles; with prompt, till it waits at one too
  const settle = prompt => {
    let last = m.out.length, since = m.cpu.cyc;
    const begun = m.cpu.cyc;
    while (m.cpu.cyc < limit && !m.cpu.halted) {
      m.run(m.cpu.cyc + STEP);
      // (A screen that never stops changing, top's: a picture once the keys are typed and a while has passed)
      if (!prompt && !m.acia.rxQueue.length && m.cpu.cyc - begun >= BUSY_SNAP) return;
      if (m.out.length !== last || m.acia.rxQueue.length) { last = m.out.length; since = m.cpu.cyc; continue; }
      const still = m.cpu.cyc - since;
      if (still < (prompt ? QUIET : QUIET_SNAP)) continue;
      if (!prompt) return;
      const tail = m.out.slice(-200).replace(/(\x1b\[[0-9;?]*[ -\/]*[@-~]|\x1b[()][0-9A-Za-z]|\x1b[=>78])+$/, '');
      if (tail.length && !/[\r\n]$/.test(tail)) return;
      if (still >= STUCK) return;
    }
  };
  const vt = new VT({ cols: 80, rows: 24 });
  let fed = 0, snaps = 0;
  for (const it of items) {
    if (it.startsWith('!raw ')) type(keys(it.slice(5)));
    else if (it.startsWith('!wait ')) { const end = m.cpu.cyc + +it.slice(6) * 1e6; while (m.cpu.cyc < end && m.cpu.cyc < limit) m.run(Math.min(end, m.cpu.cyc + 4e6)); }
    else if (it === '!quiet') settle(false);
    else if (it.startsWith('!line ')) { settle(false); type(keys(it.slice(6)) + '\r'); }
    else if (it.startsWith('!png ')) {                          // (The Vera X's screen, as the emulator draws it)
      settle(false);
      if (!m.vera) throw new Error('!png: with --vera');
      const f = m.vera.frame(), file = path.join(o.figs, it.slice(5).trim() + '.png');
      fs.writeFileSync(file, encodePng({ width: 640, height: 480, pixels: f.pixels, rgb: f.rgb }, require('zlib').deflateSync));
      console.error('wrote ' + file);
      snaps++;
    }
    else if (it.startsWith('!snap ')) {
      settle(false);
      vt.write(m.out.slice(fed));
      fed = m.out.length;
      const file = path.join(o.figs, it.slice(6).trim() + '.svg');
      fs.writeFileSync(file, screenSvg(vt));
      console.error('wrote ' + file);
      snaps++;
    } else { settle(true); type(keys(it) + '\r'); }
  }
  settle(!snaps);
  if (o.raw) return m.out;
  if (snaps) return '';                                       // (A screen's pictures: no transcript)
  let lines = clean(m.out);
  if (!o.boot) {
    const k = lines.findIndex(l => /^(\/[^ ]*> |% )/.test(l) || /^\/[^ ]*>$/.test(l));
    if (k > 0) lines = lines.slice(k);
  }
  while (lines.length && !lines[lines.length - 1].trim()) lines.pop();
  return lines.join('\n');
}

if (require.main === module) {
  const o = parseArgs(process.argv.slice(2));
  if (!o.file) { console.error('usage: node manual/tools/capture.js [--boot] [--vera] [--smc] [--rtc] [--modules N] [--no-sd] [--cycles M] [--raw] SCRIPT|-'); process.exit(2); }
  const script = o.file === '-' ? fs.readFileSync(0, 'utf8') : fs.readFileSync(o.file, 'utf8');
  process.stdout.write(capture(script, o) + '\n');
}
module.exports = { capture, clean };
