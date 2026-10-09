// page.js - the browser emulator's page (sim/web.js bundles it with web/page.html): the Hydra-16 runs in a Web Worker
// (web/worker.js), and this is the rest of the PC around it.
//   The serial console: a terminal, the VT100 the tests hold the console against (lib/vt.js), drawn on a canvas, its
// keys sent as a PC terminal (xterm's) sends them, its size told as the PC tool tells it (ESC [ 8 ; rows ; columns t,
// when the Hydra asks with ESC [ 18 t, and as it changes), the mouse's reports when the Hydra asks for them (?1000,
// ?1006), and the wheel scrolling back through its lines when it doesn't.  A drag selects text and copies it.
//   The Vera X's screen (Setup: the card in slot 0): its frames as the worker draws them (each line in its time), and
// its keyboard and mouse (the input controller's), when the screen has the focus, as view.js's page gives them.
//   The sound (its button): the worker's stream, played through lib/worklet.js's AudioWorklet.
//   The SD cards (Setup): a new blank HydraFS card, or an image file loaded, for SD device 0 (/sd/0) and the Vera X's
// (/sd/v); kept in this browser's IndexedDB as the Hydra writes them, and saved as image files on asking.
//   The setup and the terminal's size are kept in localStorage.
'use strict';
const { VT } = require('../../../base/sim/lib/vt.js');
const { KEYNUM } = require('../../../base/sim/lib/keynum.js');
const { WORKLET } = require('../../../base/sim/lib/worklet.js');
const { mkfs } = require('./mkfs.js');

const $ = id => document.getElementById(id);
const CLOCK = 3.579545e6;

// ---- The setup, kept

const DEFAULTS = { vera: true, smc: false, rtc: true, modules: 3, size: '80x24', speed: '1' };
const store = {
  get: () => { try { return Object.assign({}, DEFAULTS, JSON.parse(localStorage.getItem('hydra16.setup') || '{}')); } catch (e) { return Object.assign({}, DEFAULTS); } },
  set: s => { try { localStorage.setItem('hydra16.setup', JSON.stringify(s)); } catch (e) { /* (Not kept: a private window) */ } },
};
let setup = store.get();

// ---- The cards: { slot: { name, bytes (ArrayBuffer) } }, kept in IndexedDB

const cards = {};
const SLOTS = { sd0: 'SD card 0 (/sd/0)', verasd: 'The Vera X\'s SD card (/sd/v)' };
const idb = (() => {
  let db = null;
  const open = () => db || (db = new Promise((ok, fail) => {
    const r = indexedDB.open('hydra16', 1);
    r.onupgradeneeded = () => r.result.createObjectStore('cards');
    r.onsuccess = () => ok(r.result);
    r.onerror = () => fail(r.error);
  }));
  const tx = (mode, fn) => open().then(d => new Promise((ok, fail) => {
    const t = d.transaction('cards', mode), s = t.objectStore('cards'), r = fn(s);
    t.oncomplete = () => ok(r && r.result);
    t.onerror = () => fail(t.error);
  }));
  return {
    get: slot => tx('readonly', s => s.get(slot)).catch(() => null),
    keys: () => tx('readonly', s => s.getAllKeys()).catch(() => []),
    put: (slot, v) => tx('readwrite', s => s.put(v, slot)).catch(() => {}),
    del: slot => tx('readwrite', s => s.delete(slot)).catch(() => {}),
  };
})();

// ---- The worker

const worker = new Worker(URL.createObjectURL(new Blob([$('worker-src').textContent], { type: 'text/javascript' })));
const wantCard = {};                                          // (Each slot's card asked for: its promise's resolve)
let saveTimer = null;
const written = new Set();

async function roms() {
  const el = $('roms'), raw = Uint8Array.from(atob(el.textContent.trim()), c => c.charCodeAt(0));
  const all = await new Response(new Blob([raw]).stream().pipeThrough(new DecompressionStream('gzip'))).arrayBuffer();
  const n = +el.dataset.bios;
  return { bios: all.slice(0, n), prom: all.slice(n) };
}
let images = null;

// The cards as the worker has them now (each one written since it was last asked for), into cards and IndexedDB
async function syncCards() {
  const slots = [...written];
  written.clear();
  for (const slot of slots) {
    const bytes = await new Promise(ok => { wantCard[slot] = ok; worker.postMessage({ type: 'card', slot }); });
    if (bytes && cards[slot]) { cards[slot].bytes = bytes.buffer; await idb.put(slot, cards[slot]); }
  }
}

async function power() {
  await syncCards();
  images = images || await roms();
  const c = {};
  for (const slot of Object.keys(cards)) if (slot !== 'verasd' || setup.vera) c[slot] = cards[slot].bytes.slice(0);
  term.reset();
  halted = '';
  worker.postMessage({ type: 'boot', bios: images.bios.slice(0), prom: images.prom.slice(0), cards: c,
    opt: { vera: setup.vera, smc: setup.smc, rtc: setup.rtc, modules: +setup.modules } }, Object.values(c));
  worker.postMessage({ type: 'speed', x: +setup.speed });
  $('screenbox').hidden = !setup.vera;
  $('kstate').textContent = setup.vera && setup.smc ? 'click the screen for its keyboard and mouse' : '';
  if (setup.vera) askFrame();
}

let halted = '', frameAsked = false, status = { cyc: 0, rate: 0 };
worker.onmessage = e => {
  const msg = e.data;
  switch (msg.type) {
    case 'out': term.write(msg.s); break;
    case 'frame': frameAsked = false; drawFrame(msg); break;
    case 'audio': if (sound.node) sound.node.port.postMessage(msg.s.buffer, [msg.s.buffer]); break;
    case 'status': status = msg; showStatus(); break;
    case 'written':
      written.add(msg.slot);
      clearTimeout(saveTimer);
      saveTimer = setTimeout(() => syncCards(), 3000);       // (Kept a few seconds after the last write)
      break;
    case 'card': if (wantCard[msg.slot]) { wantCard[msg.slot](msg.bytes); delete wantCard[msg.slot]; } break;
    case 'halted': halted = msg.why; showStatus(); break;
  }
};
worker.onerror = e => { console.error('the emulator: ' + e.message + ' (' + e.filename + ':' + e.lineno + ')'); };
window.addEventListener('pagehide', () => { if (written.size) syncCards(); });

function showStatus() {
  const s = status.cyc / CLOCK, pct = Math.round(status.rate / CLOCK * 100);
  const time = s < 60 ? s.toFixed(1) + ' s' : Math.floor(s / 60) + ' min ' + Math.floor(s % 60) + ' s';
  const el = $('status');
  el.textContent = halted ? 'halted: ' + halted : paused ? 'paused · ' + time
    : (status.rate / 1e6).toFixed(2) + ' MHz (' + pct + '% of real time) · ' + time +
      (sound.ctx ? ' · sound ' + (sound.held ? Math.round(sound.held / 48) + ' ms ahead' : 'starting') : '');
  el.style.color = halted ? 'var(--warn)' : '';
}

// ---- The terminal

// The 16 colours (Windows Terminal's Campbell), and ISO-8859-15's characters where they aren't Latin-1's
const COLOURS = ['#0c0c0c', '#c50f1f', '#13a10e', '#c19c00', '#0037da', '#881798', '#3a96dd', '#cccccc',
  '#767676', '#e74856', '#16c60c', '#f9f1a5', '#3b78ff', '#b4009e', '#61d6d6', '#f2f2f2'];
const LATIN9 = { 0xA4: '€', 0xA6: 'Š', 0xA8: 'š', 0xB4: 'Ž', 0xB8: 'ž', 0xBC: 'Œ', 0xBD: 'œ', 0xBE: 'Ÿ' };
const TO_LATIN9 = Object.fromEntries(Object.entries(LATIN9).map(([k, v]) => [v, +k]));
// The DEC Special Graphics ($5F-$7E: vt.js keeps them as 0-31)
const DEC = ' ◆▒␉␌␍␊°±␤␋┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥π≠£·';
const glyph = c => c < 0x20 ? DEC[c] : LATIN9[c] || String.fromCharCode(c);

class Term extends VT {
  constructor(canvas) {
    super({ cols: 80, rows: 24, scrollback: 1000 });
    this.cv = canvas; this.cx = canvas.getContext('2d');
    this.modes = {}; this.back = 0; this.sel = null; this.dirty = true; this.blink = true;
    this.font = '15px "Cascadia Mono", "Cascadia Code", Consolas, "DejaVu Sans Mono", Menlo, monospace';
  }
  reset() { super.reset(); this.modes = {}; this.back = 0; this.sel = null; this.dirty = true; }
  decMode(m, on) { this.modes[m] = on; super.decMode(m, on); }
  csiDo(f) {
    if (f === 't' && !this.priv && this.raw(0) === 18) return tellSize();   // (The Hydra asking the terminal's size)
    super.csiDo(f);
  }
  write(s) { super.write(s); this.back = 0; this.dirty = true; }
  size(cols, rows) {
    this.resize(cols, rows);
    const cx = this.cx, dpr = window.devicePixelRatio || 1;
    cx.font = this.font;
    this.cw = Math.ceil(cx.measureText('M').width); this.ch = 19;
    this.cv.width = cols * this.cw * dpr; this.cv.height = rows * this.ch * dpr;
    this.cv.style.width = cols * this.cw + 'px';
    this.dpr = dpr; this.dirty = true;
  }
  allLines() { return [...this.sb, ...this.screen]; }
  // The rows shown: the screen, or back rows up into the scrollback
  view() { const all = this.allLines(), top = all.length - this.rows - this.back; return all.slice(top, top + this.rows); }
  draw() {
    if (!this.dirty) return;
    this.dirty = false;
    const cx = this.cx, cw = this.cw, ch = this.ch, rows = this.view(), focused = document.activeElement === this.cv;
    cx.setTransform(this.dpr, 0, 0, this.dpr, 0, 0);
    cx.font = this.font; cx.textBaseline = 'middle';
    const top = this.sb.length - this.back;
    for (let y = 0; y < this.rows; y++) {
      const row = rows[y] || [], dw = row.dw;
      cx.save();
      cx.beginPath(); cx.rect(0, y * ch, this.cols * cw, ch); cx.clip();
      cx.fillStyle = COLOURS[0]; cx.fillRect(0, y * ch, this.cols * cw, ch);
      const w = dw ? cw * 2 : cw, n = dw ? this.cols >> 1 : this.cols;
      for (let x = 0; x < n; x++) {
        const k = row[x] || { c: 0x20, a: 7, f: 0 };
        let fg = k.a & 15, bg = k.a >> 4;
        if (k.f & 1 && fg < 8) fg += 8;                       // (Bold: the bright colour)
        let rev = !!(k.f & 16) !== this.selected(top + y, x);
        if (rev) [fg, bg] = [bg, fg];
        if (bg) { cx.fillStyle = COLOURS[bg]; cx.fillRect(x * w, y * ch, w, ch); }
        if (k.c === 0x20 || k.f & 32) continue;
        cx.fillStyle = COLOURS[fg];
        cx.globalAlpha = k.f & 2 ? 0.6 : 1;
        if (dw) {
          cx.save();
          const half = dw === 'DHT' ? 0 : dw === 'DHB' ? -ch : -ch / 2;
          cx.translate(x * w, y * ch + (dw === 'DW' ? 0 : half));
          cx.scale(2, dw === 'DW' ? 1 : 2);
          cx.fillText(glyph(k.c), 0, ch / 2);
          cx.restore();
        } else cx.fillText(glyph(k.c), x * cw, y * ch + ch / 2 + 1);
        if (k.f & 4) cx.fillRect(x * w, y * ch + ch - 2, w, 1);
        cx.globalAlpha = 1;
      }
      cx.restore();
    }
    if (this.tcem && !this.back && this.blink) {             // The cursor: a block (an outline without the focus)
      const x = Math.min(this.x, this.cols - 1) * cw, y = this.y * ch;
      cx.fillStyle = cx.strokeStyle = '#e8e8e8';
      if (focused) { cx.globalAlpha = 0.65; cx.fillRect(x, y, cw, ch); cx.globalAlpha = 1; }
      else cx.strokeRect(x + 0.5, y + 0.5, cw - 1, ch - 1);
    }
  }
  // The selection: from one (line, column) to another, in all the lines' numbering
  selected(line, x) {
    const s = this.sel;
    if (!s) return false;
    let [a, b] = [s.from, s.to];
    if (a.line > b.line || a.line === b.line && a.x > b.x) [a, b] = [b, a];
    if (line < a.line || line > b.line) return false;
    return (line > a.line || x >= a.x) && (line < b.line || x <= b.x);
  }
  selText() {
    let [a, b] = [this.sel.from, this.sel.to];
    if (a.line > b.line || a.line === b.line && a.x > b.x) [a, b] = [b, a];
    const all = this.allLines(), out = [];
    for (let l = a.line; l <= b.line; l++) {
      const row = all[l] || [];
      out.push(row.slice(l === a.line ? a.x : 0, l === b.line ? b.x + 1 : row.length).map(k => glyph(k.c)).join('').replace(/ +$/, ''));
    }
    return out.join('\n');
  }
  allText() { return this.allLines().map(r => r.map(k => glyph(k.c)).join('').replace(/ +$/, '')).join('\n').replace(/\n+$/, '\n'); }
  cell(e) {                                                   // The cell under the mouse: { x, y } (0-based)
    const r = this.cv.getBoundingClientRect();
    return { x: Math.max(0, Math.min(this.cols - 1, Math.floor((e.clientX - r.left) / r.width * this.cols))),
      y: Math.max(0, Math.min(this.rows - 1, Math.floor((e.clientY - r.top) / r.height * this.rows))) };
  }
}

const term = new Term($('term'));
const send = s => { if (s) worker.postMessage({ type: 'keys', s }); };
function tellSize() { send('\x1b[8;' + term.rows + ';' + term.cols + 't'); }
function setSize(v) {
  const [c, r] = v.split('x').map(Number);
  term.size(c, r);
  setup.size = v; store.set(setup);
  tellSize();
}
$('size').value = setup.size;
setSize(setup.size);
$('size').addEventListener('change', e => { setSize(e.target.value); term.cv.focus(); });

// A key as xterm sends it (the Hydra's console reads a PC terminal's keys: modules/cons)
const FKEYS = { F1: 'OP', F2: 'OQ', F3: 'OR', F4: 'OS', F5: '[15~', F6: '[17~', F7: '[18~', F8: '[19~', F9: '[20~', F10: '[21~', F11: '[23~', F12: '[24~' };
const TILDE = { Insert: 2, Delete: 3, PageUp: 5, PageDown: 6 };
const CURSOR = { ArrowUp: 'A', ArrowDown: 'B', ArrowRight: 'C', ArrowLeft: 'D', Home: 'H', End: 'F' };
function keyText(e) {
  const mod = 1 + (e.shiftKey ? 1 : 0) + (e.altKey ? 2 : 0) + (e.ctrlKey ? 4 : 0);
  if (CURSOR[e.key]) return mod > 1 ? '\x1b[1;' + mod + CURSOR[e.key] : (term.modes[1] ? '\x1bO' : '\x1b[') + CURSOR[e.key];
  if (TILDE[e.key]) return '\x1b[' + TILDE[e.key] + (mod > 1 ? ';' + mod : '') + '~';
  if (FKEYS[e.key]) return '\x1b' + FKEYS[e.key];
  const alt = e.altKey ? '\x1b' : '';
  switch (e.key) {
    case 'Enter': return alt + '\r';
    case 'Backspace': return alt + (e.ctrlKey ? '\x08' : '\x7f');
    case 'Tab': return e.shiftKey ? '\x1b[Z' : alt + '\t';
    case 'Escape': return '\x1b';
  }
  if (e.ctrlKey && !e.altKey) {                               // Ctrl and a key: its control character
    if (e.code.startsWith('Key')) return String.fromCharCode(e.code.charCodeAt(3) & 0x1F);
    const c = { Space: 0, Digit2: 0, BracketLeft: 27, Backslash: 28, BracketRight: 29, Digit6: 30, Minus: 31, Slash: 31 }[e.code];
    return c === undefined ? null : String.fromCharCode(c);
  }
  if (e.key.length !== 1 || e.metaKey) return null;
  const b = latin9(e.key);
  return b === null ? null : alt + b;
}
const latin9 = s => {
  let out = '';
  for (const ch of s) {
    const c = ch.codePointAt(0), b = TO_LATIN9[ch] !== undefined ? TO_LATIN9[ch] : c < 256 && !LATIN9[c] ? c : -1;
    if (b >= 0) out += String.fromCharCode(b);
  }
  return out || null;
};

term.cv.addEventListener('keydown', e => {
  if (e.ctrlKey && e.shiftKey && e.code === 'KeyC') { e.preventDefault(); copy(term.sel ? term.selText() : term.allText()); return; }
  if (e.ctrlKey && e.shiftKey && e.code === 'KeyV') { e.preventDefault(); pasteClipboard(); return; }
  const s = keyText(e);
  if (s === null) return;
  e.preventDefault();
  term.sel = null; term.back = 0; term.dirty = true;
  send(s);
});
function pasteText(t) {
  const s = latin9(t.replace(/\r\n?/g, '\n').replace(/\n/g, '\r'));
  if (!s) return;
  send(term.modes[2004] ? '\x1b[200~' + s + '\x1b[201~' : s);
}
async function pasteClipboard() {
  try { pasteText(await navigator.clipboard.readText()); } catch (e) { flash('the browser keeps the clipboard: paste with Ctrl+V here'); pasteWanted = true; }
}
let pasteWanted = false;
document.addEventListener('paste', e => {
  if (document.activeElement !== term.cv && !pasteWanted) return;
  pasteWanted = false;
  e.preventDefault();
  pasteText(e.clipboardData.getData('text'));
});
function copy(t) {
  if (navigator.clipboard) navigator.clipboard.writeText(t).then(() => flash('copied'), () => flash('the browser kept the clipboard'));
}
let flashTimer = null;
function flash(t) {
  const el = $('copy'), was = 'Copy text';
  el.textContent = t; clearTimeout(flashTimer);
  flashTimer = setTimeout(() => { el.textContent = was; }, 1500);
}
$('copy').addEventListener('click', () => copy(term.allText()));
$('paste').addEventListener('click', () => { pasteClipboard(); term.cv.focus(); });

// The mouse: the Hydra's (?1000, its reports SGR's with ?1006), or the selection and the scrollback
const report = (b, c, up) => send(term.modes[1006] ? '\x1b[<' + b + ';' + (c.x + 1) + ';' + (c.y + 1) + (up ? 'm' : 'M')
  : '\x1b[M' + String.fromCharCode(32 + (up ? 3 : b), 33 + c.x, 33 + c.y));
let dragging = false;
term.cv.addEventListener('mousedown', e => {
  term.cv.focus();
  const c = term.cell(e);
  if (term.modes[1000] && !e.shiftKey) { e.preventDefault(); report(e.button, c, false); return; }
  if (e.button !== 0) return;
  e.preventDefault();
  const line = term.sb.length - term.back + c.y;
  term.sel = { from: { line, x: c.x }, to: { line, x: c.x } }; dragging = true; term.dirty = true;
});
term.cv.addEventListener('mousemove', e => {
  if (!dragging) return;
  const c = term.cell(e);
  term.sel.to = { line: term.sb.length - term.back + c.y, x: c.x }; term.dirty = true;
});
window.addEventListener('mouseup', e => {
  if (term.modes[1000] && e.target === term.cv && !e.shiftKey) { report(e.button, term.cell(e), true); return; }
  if (!dragging) return;
  dragging = false;
  const s = term.sel;
  if (s && (s.from.line !== s.to.line || s.from.x !== s.to.x)) copy(term.selText()); else term.sel = null;
  term.dirty = true;
});
term.cv.addEventListener('contextmenu', e => { if (term.modes[1000]) e.preventDefault(); });
term.cv.addEventListener('wheel', e => {
  e.preventDefault();
  const d = Math.sign(e.deltaY);
  if (term.modes[1000]) { report(d < 0 ? 64 : 65, term.cell(e), false); return; }
  term.back = Math.max(0, Math.min(term.sb.length, term.back - d * 3)); term.dirty = true;
}, { passive: false });
term.cv.addEventListener('focus', () => { term.dirty = true; });
term.cv.addEventListener('blur', () => { term.dirty = true; });
setInterval(() => { term.blink = !term.blink; term.dirty = true; }, 530);

// ---- The Vera X's screen

let veraVersion = '';
const scv = $('screen'), scx = scv.getContext('2d'), img = scx.createImageData(640, 480);
function askFrame() { if (!frameAsked && setup.vera) { frameAsked = true; worker.postMessage({ type: 'frame' }); } }
function drawFrame(f) {
  if (f.none) return;
  if (f.version) veraVersion = f.version;
  $('vstate').textContent = f.ready ? 'Vera X (VERA ' + veraVersion + ') · frame ' + f.frames : 'Vera X: configuring';
  if (f.same || !f.pixels) return;
  const d = img.data, p = f.pixels, rgb = f.rgb;
  for (let i = 0, o = 0; i < 640 * 480; i++, o += 4) { const c = rgb[p[i]]; d[o] = c >> 16; d[o + 1] = (c >> 8) & 255; d[o + 2] = c & 255; d[o + 3] = 255; }
  scx.putImageData(img, 0, 0);
}
// Its keyboard and mouse (the input controller's), while the screen has the focus
let mdx = 0, mdy = 0, mb = 0, mw = 0, moved = false, lastX = null, lastY = null;
const vkey = (e, down) => {
  if (!setup.smc) return;
  const n = KEYNUM[e.code];
  if (n === undefined) return;
  e.preventDefault();
  worker.postMessage({ type: 'key', n, down });
};
scv.addEventListener('keydown', e => vkey(e, true));
scv.addEventListener('keyup', e => vkey(e, false));
scv.addEventListener('blur', () => { $('kstate').textContent = setup.vera && setup.smc ? 'click the screen for its keyboard and mouse' : ''; });
scv.addEventListener('focus', () => { if (setup.smc) $('kstate').textContent = 'the keyboard and mouse are the Hydra\'s: click elsewhere to give them back'; });
scv.addEventListener('mousedown', e => { scv.focus(); mb = e.buttons & 7; moved = true; e.preventDefault(); });
scv.addEventListener('mouseup', e => { mb = e.buttons & 7; moved = true; });
scv.addEventListener('contextmenu', e => e.preventDefault());
scv.addEventListener('mousemove', e => {
  const x = e.offsetX * 640 / scv.clientWidth, y = e.offsetY * 480 / scv.clientHeight;
  if (lastX !== null) { mdx += x - lastX; mdy += y - lastY; moved = true; }
  lastX = x; lastY = y; mb = e.buttons & 7;
});
scv.addEventListener('mouseleave', () => { lastX = lastY = null; });
scv.addEventListener('wheel', e => { if (document.activeElement !== scv) return; mw += Math.sign(e.deltaY); moved = true; e.preventDefault(); }, { passive: false });
setInterval(() => {
  if (!moved || !setup.smc || document.activeElement !== scv) { moved = false; return; }
  const dx = Math.trunc(mdx), dy = Math.trunc(mdy), w = Math.max(-8, Math.min(7, mw));
  mdx -= dx; mdy -= dy; mw = 0; moved = false;
  worker.postMessage({ type: 'mouse', dx, dy, b: mb, w });
}, 16);

// ---- The sound

const sound = { ctx: null, node: null, held: 0 };
async function soundOn() {
  sound.ctx = new AudioContext({ sampleRate: 48000 });
  await sound.ctx.audioWorklet.addModule('data:text/javascript,' + encodeURIComponent(WORKLET));   // (Not a Blob's URL: a file:// page can't load one)
  sound.node = new AudioWorkletNode(sound.ctx, 'hydra-sound', { outputChannelCount: [2] });
  sound.node.connect(sound.ctx.destination);
  sound.node.port.onmessage = e => { sound.held = e.data; };   // (The samples it holds: some 7,200 when all's well)
  worker.postMessage({ type: 'sound', on: true });
  $('sound').setAttribute('aria-pressed', 'true');
}
function soundOff() {
  worker.postMessage({ type: 'sound', on: false });
  if (sound.ctx) sound.ctx.close();
  sound.ctx = sound.node = null;
  $('sound').setAttribute('aria-pressed', 'false');
}
$('sound').addEventListener('click', () => { if (sound.ctx) soundOff(); else soundOn().catch(e => { soundOff(); alert('No sound: ' + e.message); }); });

// ---- The controls

let paused = false;
$('power').addEventListener('click', () => { power(); term.cv.focus(); });
$('reset').addEventListener('click', () => { worker.postMessage({ type: 'reset' }); term.cv.focus(); });
$('pause').addEventListener('click', () => {
  paused = !paused;
  worker.postMessage({ type: 'pause', on: paused });
  $('pause').setAttribute('aria-pressed', String(paused));
  showStatus();
});
$('speed').value = setup.speed;
$('speed').addEventListener('change', e => { setup.speed = e.target.value; store.set(setup); worker.postMessage({ type: 'speed', x: +setup.speed }); });

// The setup's dialog
const dlg = $('setupdlg');
function showCards() {
  for (const el of dlg.querySelectorAll('.card')) {
    const slot = el.dataset.slot, c = cards[slot];
    el.querySelector('.what').textContent = SLOTS[slot] + ': ' + (c ? c.name + ' (' + Math.round(c.bytes.byteLength / 1048576) + ' MB)' : 'none');
    el.querySelector('[data-act=save]').disabled = el.querySelector('[data-act=remove]').disabled = !c;
  }
}
$('setup').addEventListener('click', async () => {
  $('o-vera').checked = setup.vera; $('o-smc').checked = setup.smc; $('o-rtc').checked = setup.rtc; $('o-modules').value = String(setup.modules);
  await syncCards();
  showCards();
  dlg.showModal();
});
let loadSlot = null;
dlg.addEventListener('click', async e => {
  const act = e.target.dataset && e.target.dataset.act;
  if (!act) return;
  const slot = e.target.closest('.card').dataset.slot;
  if (act === 'new') {
    const mb = +$('o-cardmb').value;
    cards[slot] = { name: 'a new ' + mb + ' MB card', bytes: mkfs(mb, '') };
    await idb.put(slot, cards[slot]);
  } else if (act === 'load') { loadSlot = slot; $('cardfile').value = ''; $('cardfile').click(); return; }
  else if (act === 'save') {
    await syncCards();
    const a = document.createElement('a'), c = cards[slot];
    a.href = URL.createObjectURL(new Blob([c.bytes], { type: 'application/octet-stream' }));
    a.download = /\.(img|bin)$/i.test(c.name) ? c.name : (slot === 'sd0' ? 'hydra-sd0' : 'hydra-verasd') + '.img';
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 10000);
  } else if (act === 'remove') {
    if (!confirm('Remove this card from the browser? (Save it first to keep it.)')) return;
    delete cards[slot]; await idb.del(slot);
  }
  showCards();
});
$('cardfile').addEventListener('change', async e => {
  const f = e.target.files[0];
  if (!f || !loadSlot) return;
  if (f.size % 512 || f.size < 64 * 1024) { alert('A card image is a whole number of 512-byte blocks: ' + f.name + ' isn\'t one.'); return; }
  cards[loadSlot] = { name: f.name, bytes: await f.arrayBuffer() };
  await idb.put(loadSlot, cards[loadSlot]);
  showCards();
});
dlg.addEventListener('close', () => {
  if (dlg.returnValue !== 'apply') return;
  Object.assign(setup, { vera: $('o-vera').checked, smc: $('o-smc').checked, rtc: $('o-rtc').checked, modules: +$('o-modules').value });
  store.set(setup);
  power();
  term.cv.focus();
});

// ---- Drawing, and the start

function frame() {
  term.draw();
  if (setup.vera && !document.hidden) askFrame();
  requestAnimationFrame(frame);
}
window.hydra = { term, worker, sound, cards, get setup() { return setup; }, get frameAsked() { return frameAsked; } };   // (For the browser's console)
$('build').textContent = $('roms').dataset.build || '';
(async () => {
  for (const slot of await idb.keys()) {
    const c = await idb.get(slot);
    if (c && c.bytes) cards[slot] = c;
  }
  await power();
  term.cv.focus();
  requestAnimationFrame(frame);
})();
