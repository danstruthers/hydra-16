// vt.js - a VT100 (and VT102) terminal's screen, for the tests (docs/plans/WINDOWS.md, W1): what the PC's terminal
// shows of the serial port's stream, and what a window's screen should be after a program's output (onlcr: each LF
// a CR LF too, as the console makes it).  Written apart from the console's engine (modules/cons/vt.s), from the
// VT100's and VT102's user guides, so a test can hold one against the other; xterm.js's headless terminal, when
// it's installed, holds both against a terminal written by others (tests/vt).
//   A screen of rows of cells (a character, the colours: background << 4 | foreground, the rendition), the
// scrollback (the rows that went off the top of the screen, or of a region at its top, by a line feed, IND or SU;
// the newest last; SU's not, and RIS clears it, as xterm's), VT52 mode (DECANM reset), the cursor and its last-column flag, the margins, the modes (DECAWM, DECOM, IRM, LNM, DECTCEM),
// the tab stops, the character sets (G0, G1: B, A, 0), DECSC's saved cursor.  The parser is DEC's state machine (Paul
// Williams'): C0 controls act inside a sequence, CAN and SUB end one, strings (OSC, DCS, SOS, PM, APC) end at ST
// (an OSC also at BEL).  text() is a window's /text: its scrollback, then its screen, a line a row without its
// trailing spaces, the DEC graphics as the console shows them in ASCII (DEC_ASCII).

'use strict';

// The DEC Special Graphics ($5F-$7E) as the console's text shows them (vt.s's dec_ascii; ISO-8859-15)
const DEC_ASCII = [' ', '*', '#', 'H', 'F', 'C', 'L', '\xB0', '\xB1', 'N', 'V', '+', '+', '+', '+', '+',
  '-', '-', '-', '-', '_', '+', '+', '+', '+', '|', '<', '>', 'p', '#', '\xA3', '\xB7'];
const COL_DEF = 0x07;

class VT {
  constructor({ cols = 80, rows = 24, scrollback = 40, onlcr = false } = {}) {
    Object.assign(this, { cols, rows, sbMax: scrollback, onlcr });
    this.sb = [];
    this.reset();
  }

  blankRow() {
    const bg = this.col & 0xF0 | (COL_DEF & 0x0F);
    return Array.from({ length: this.cols }, () => ({ c: 0x20, a: bg, f: 0 }));
  }

  reset() {
    this.sb = []; this.state = 0; this.col = COL_DEF; this.fl = 0;
    this.screen = Array.from({ length: this.rows }, () => this.blankRow());
    this.x = 0; this.y = 0; this.wrap = false;
    this.top = 0; this.bot = this.rows - 1;
    this.awm = true; this.om = false; this.irm = false; this.lnm = false; this.tcem = true;
    this.g = ['B', 'B']; this.gl = 0; this.last = 0x20;
    this.tabs = new Set(); for (let c = 8; c < this.cols; c += 8) this.tabs.add(c);
    this.saved = null; this.vt52 = false;
  }

  soft() {
    this.awm = true; this.om = false; this.irm = false; this.lnm = false; this.tcem = true;
    this.top = 0; this.bot = this.rows - 1; this.col = COL_DEF; this.fl = 0; this.g = ['B', 'B']; this.gl = 0;
    this.saved = null;
  }

  // ---- Output in: a string (each char a byte) or a Buffer
  write(data) {
    const s = typeof data === 'string' ? data : Buffer.from(data).toString('latin1');
    for (let i = 0; i < s.length; i++) this.byte(s.charCodeAt(i) & 0xFF);
    return this;
  }

  byte(b) {
    if (b === 0x7F) return;
    if (b < 0x20) {
      if (b === 0x1B) {
        if (this.state === 'osc' || this.state === 'str') { this.state += 'E'; return; }
        this.state = 'esc'; this.inter = ''; this.priv = ''; this.params = ['']; return;
      }
      if (b === 0x18) { this.state = 0; return; }
      if (b === 0x1A) { this.state = 0; this.glyph(2); return; }
      if (this.state === 'osc' || this.state === 'oscE') { if (b === 0x07) this.state = 0; return; }
      if (this.state === 'str' || this.state === 'strE') return;
      return this.c0(b);
    }
    switch (this.state) {
      case 0: return this.print(b);
      case 'esc':
        if (this.vt52) { this.state = 0; return this.vt52Do(String.fromCharCode(b)); }
        if (b < 0x30) { this.inter += String.fromCharCode(b); this.state = 'escI'; return; }
        if (b >= 0x80) { this.state = 0; return; }
        if (b === 0x5B) { this.state = 'csi'; this.params = ['']; this.priv = ''; this.inter = ''; this.seen = false; return; }
        if (b === 0x5D) { this.state = 'osc'; return; }
        if ([0x50, 0x58, 0x5E, 0x5F].includes(b)) { this.state = 'str'; return; }
        this.state = 0; return this.escDo(String.fromCharCode(b));
      case 'escI':
        if (b < 0x30) { this.inter += String.fromCharCode(b); return; }
        this.state = 0; if (b < 0x80) this.escDo(String.fromCharCode(b)); return;
      case 'csi':
        if (b >= 0x30 && b <= 0x39) { this.seen = true; this.params[this.params.length - 1] += String.fromCharCode(b); return; }
        if (b === 0x3B) { this.seen = true; if (this.params.length < 16) this.params.push(''); return; }
        if (b === 0x3A) { this.state = 'csiX'; return; }
        if (b >= 0x3C && b <= 0x3F) { if (this.seen || this.priv) this.state = 'csiX'; else this.priv = String.fromCharCode(b); return; }
        if (b < 0x30) { this.inter += String.fromCharCode(b); this.state = 'csiI'; return; }
        this.state = 0; if (b < 0x7F) this.csiDo(String.fromCharCode(b)); return;
      case 'csiI':
        if (b < 0x30) { this.inter += String.fromCharCode(b); return; }
        if (b < 0x40) { this.state = 'csiX'; return; }
        this.state = 0; if (b < 0x7F) this.csiDo(String.fromCharCode(b)); return;
      case 'csiX': if (b >= 0x40 && b < 0x7F) this.state = 0; return;
      case 'osc': case 'str': return;
      case 'y1': this.y52 = Math.max(1, b - 31); this.state = 'y2'; return;
      case 'y2': this.state = 0; this.params = [String(this.y52), String(Math.max(1, b - 31))]; this.priv = ''; this.inter = ''; return this.csiDo('H');
      case 'oscE': case 'strE':
        if (b === 0x5C) { this.state = 0; return; }
        this.state = 'esc'; this.inter = ''; return this.byte(b);
    }
  }

  c0(b) {
    switch (b) {
      case 0x0A: if (this.onlcr || this.lnm) this.x = 0; this.index(); break;
      case 0x0B: case 0x0C: if (this.lnm) this.x = 0; this.index(); break;
      case 0x0D: this.x = 0; this.wrap = false; break;
      case 0x08: this.wrap = false; if (this.x > 0) this.x--; break;
      case 0x09: this.tab(); break;
      case 0x0E: this.gl = 1; break;
      case 0x0F: this.gl = 0; break;
    }
  }

  num(i, def = 1) { const v = this.params[i] === undefined || this.params[i] === '' ? 0 : Math.min(65535, +this.params[i]); return v || def; }
  raw(i) { return this.params[i] === undefined || this.params[i] === '' ? 0 : Math.min(65535, +this.params[i]); }

  escDo(f) {
    const i = this.inter;
    if (i === '#') { if (f === '8') this.align(); return; }
    if (i === '(' || i === ')') { this.g[i === '(' ? 0 : 1] = f === '1' ? 'B' : f === '2' ? '0' : 'A0'.includes(f) ? f : 'B'; return; }
    if (i) return;
    switch (f) {
      case '7': this.save(); break;
      case '8': this.restore(); break;
      case 'D': this.index(); break;
      case 'E': this.x = 0; this.index(); break;
      case 'H': this.tabs.add(this.x); break;
      case 'M': this.rindex(); break;
      case 'c': this.reset(); break;
    }
  }

  csiDo(f) {
    if (this.inter) { if (this.inter === '!' && f === 'p' && !this.priv) this.soft(); return; }
    if (this.priv === '?') {
      if (f === 'h' || f === 'l') { for (let i = 0; i < this.params.length; i++) this.decMode(this.raw(i), f === 'h'); return; }
      if (f !== 'J' && f !== 'K') return;
    } else if (this.priv) return;
    const n = this.num(0), last = this.cols - 1;
    switch (f) {
      case '@': this.ich(n); break;
      case 'A': this.y = Math.max(this.y - n, this.y >= this.top ? this.top : 0); this.moved(); break;
      case 'B': this.y = Math.min(this.y + n, this.y <= this.bot ? this.bot : this.rows - 1); this.moved(); break;
      case 'C': case 'a': this.x = Math.min(this.x + n, last); this.moved(); break;
      case 'D': this.x = Math.max(this.x - n, 0); this.moved(); break;
      case 'E': this.x = 0; this.csiDo('B'); break;
      case 'F': this.x = 0; this.csiDo('A'); break;
      case 'G': case '`': this.x = Math.min(n - 1, last); this.moved(); break;
      case 'H': case 'f': this.y = this.rowOf(n); this.x = Math.min(this.num(1) - 1, last); this.moved(); break;
      case 'I': for (let k = 0; k < n; k++) this.tab(); break;
      case 'Z': for (let k = 0; k < n; k++) this.backtab(); this.wrap = false; break;
      case 'J': this.ed(this.raw(0)); break;
      case 'K': this.el(this.raw(0)); break;
      case 'L': if (this.y >= this.top && this.y <= this.bot) { this.scrollDown(this.y, this.bot, n); this.x = 0; this.wrap = false; } break;
      case 'M': if (this.y >= this.top && this.y <= this.bot) { this.scrollUp(this.y, this.bot, n, false); this.x = 0; this.wrap = false; } break;
      case 'P': this.dch(n); break;
      case 'S': this.scrollUp(this.top, this.bot, n, false); break;
      case 'T': this.scrollDown(this.top, this.bot, n); break;
      case 'X': { const row = this.screen[this.y], bg = this.blankRow()[0]; for (let c = this.x; c < Math.min(this.x + n, this.cols); c++) row[c] = { ...bg }; this.wrap = false; break; }
      case 'b': for (let k = 0; k < n; k++) this.glyph(this.last); break;
      case 'd': this.y = this.rowOf(n); this.moved(); break;
      case 'e': this.y = Math.min(this.y + n, this.rows - 1); this.moved(); break;
      case 'g': if (this.raw(0) === 0) this.tabs.delete(this.x); else if (this.raw(0) === 3) this.tabs.clear(); break;
      case 'h': case 'l': for (let i = 0; i < this.params.length; i++) {
        const m = this.raw(i); if (m === 4) this.irm = f === 'h'; else if (m === 20) this.lnm = f === 'h';
      } break;
      case 'm': this.sgr(); break;
      case 'r': {
        const t = this.num(0), b = this.raw(1) || this.rows;
        if (t < b && b <= this.rows) { this.top = t - 1; this.bot = b - 1; this.home(); }
        break;
      }
      case 's': this.save(); break;
      case 'u': this.restore(); break;
    }
  }

  // VT52 mode's sequences (after ESC): as the ANSI ones that do the same
  vt52Do(f) {
    this.params = ['']; this.priv = ''; this.inter = '';
    if ('ABCDHJK'.includes(f)) return this.csiDo(f);
    if (f === 'I') return this.rindex();
    if (f === 'F') this.g[0] = '0';
    else if (f === 'G') this.g[0] = 'B';
    else if (f === 'Y') this.state = 'y1';
    else if (f === '<') this.vt52 = false;
  }

  decMode(m, on) {
    switch (m) {
      case 2: this.vt52 = !on; break;
      case 3: this.top = 0; this.bot = this.rows - 1; this.home(); this.ed(2); break;
      case 6: this.om = on; this.home(); break;
      case 7: this.awm = on; this.wrap = false; break;
      case 25: this.tcem = on; break;
    }
  }

  sgr() {
    const p = this.params.map((v, i) => this.raw(i));
    for (let i = 0; i < p.length; i++) {
      const v = p[i];
      if (v === 0) { this.col = COL_DEF; this.fl = 0; }
      else if (v >= 1 && v <= 9) this.fl |= [0, 1, 2, 0, 4, 8, 8, 16, 32, 0][v];
      else if (v === 21) this.fl |= 4;
      else if (v >= 22 && v <= 29) this.fl &= [~3, -1, ~4, ~8, -1, ~16, ~32, -1][v - 22];
      else if (v >= 30 && v <= 37) this.col = this.col & 0xF0 | v - 30;
      else if (v === 39) this.col = this.col & 0xF0 | COL_DEF & 0x0F;
      else if (v >= 40 && v <= 47) this.col = this.col & 0x0F | v - 40 << 4;
      else if (v === 49) this.col = this.col & 0x0F | COL_DEF & 0xF0;
      else if (v >= 90 && v <= 97) this.col = this.col & 0xF0 | v - 82;
      else if (v >= 100 && v <= 107) this.col = this.col & 0x0F | v - 92 << 4;
      else if (v === 38 || v === 48) {
        let c = null;
        if (p[i + 1] === 5) { c = VT.from256(p[i + 2]); i += 2; }
        else if (p[i + 1] === 2) { c = VT.fromRGB(p[i + 2], p[i + 3], p[i + 4]); i += 4; }
        else i++;
        if (c !== null) this.col = v === 38 ? this.col & 0xF0 | c : this.col & 0x0F | c << 4;
      }
    }
  }

  static from256(n) {
    if (n === undefined) return null;
    if (n < 16) return n;
    if (n >= 232) return [0, 0, 8, 8, 7, 15][(n - 232) >> 2];
    n -= 16;
    const r = Math.floor(n / 36), g = Math.floor(n / 6) % 6, b = n % 6;
    const bits = (r > 2 ? 1 : 0) | (g > 2 ? 2 : 0) | (b > 2 ? 4 : 0);
    return Math.max(r, g, b) >= 5 ? bits | 8 : bits;
  }

  static fromRGB(r = 0, g = 0, b = 0) {
    const bits = (r > 127 ? 1 : 0) | (g > 127 ? 2 : 0) | (b > 127 ? 4 : 0);
    return Math.max(r, g, b) >= 192 ? bits | 8 : bits;
  }

  rowOf(n) {
    if (this.om) return Math.min(this.top + n - 1, this.bot);
    return Math.min(n - 1, this.rows - 1);
  }

  moved() { this.wrap = false; }
  home() { this.x = 0; this.wrap = false; this.y = this.om ? this.top : 0; }

  print(b) {
    const set = this.g[this.gl];
    if (set === '0' && b >= 0x5F && b <= 0x7E) b -= 0x5F;
    else if (set === 'A' && b === 0x23) b = 0xA3;
    this.glyph(b);
  }

  glyph(b) {
    this.last = b;
    if (this.wrap) { this.wrap = false; this.x = 0; this.index(); }
    if (this.irm) this.ich(1);
    this.screen[this.y][this.x] = { c: b, a: this.col, f: this.fl };
    if (this.x < this.cols - 1) this.x++;
    else if (this.awm) this.wrap = true;
  }

  index() {
    this.wrap = false;
    if (this.y === this.bot) this.scrollUp(this.top, this.bot, 1, true);
    else if (this.y < this.rows - 1) this.y++;
  }

  rindex() {
    this.wrap = false;
    if (this.y === this.top) this.scrollDown(this.top, this.bot, 1);
    else if (this.y > 0) this.y--;
  }

  scrollUp(t, b, n, keep) {
    n = Math.max(1, Math.min(n, b - t + 1));
    for (let k = 0; k < n; k++) {
      const out = this.screen.splice(t, 1)[0];
      this.screen.splice(b, 0, this.blankRow());
      if (keep && t === 0) { this.sb.push(out); if (this.sb.length > this.sbMax) this.sb.shift(); }
    }
  }

  scrollDown(t, b, n) {
    n = Math.max(1, Math.min(n, b - t + 1));
    for (let k = 0; k < n; k++) { this.screen.splice(b, 1); this.screen.splice(t, 0, this.blankRow()); }
  }

  tab() {
    this.wrap = false;
    do { this.x++; } while (this.x < this.cols - 1 && !this.tabs.has(this.x));
    if (this.x > this.cols - 1) this.x = this.cols - 1;
  }

  backtab() { while (this.x > 0) { this.x--; if (this.tabs.has(this.x)) break; } }

  el(m) {
    const row = this.screen[this.y], blank = this.blankRow()[0];
    const [a, b] = m === 0 ? [this.x, this.cols] : m === 1 ? [0, this.x + 1] : m === 2 ? [0, this.cols] : [0, 0];
    for (let c = a; c < b; c++) row[c] = { ...blank };
  }

  ed(m) {
    if (m === 3) { this.sb = []; return; }
    if (m > 3) return;
    this.el(m);
    const [a, b] = m === 0 ? [this.y + 1, this.rows] : m === 1 ? [0, this.y] : [0, this.rows];
    for (let r = a; r < b; r++) this.screen[r] = this.blankRow();
  }

  ich(n) {
    const row = this.screen[this.y]; n = Math.min(n, this.cols - this.x);
    const bg = this.blankRow()[0];
    row.splice(this.x, 0, ...Array.from({ length: n }, () => ({ ...bg }))); row.length = this.cols;
  }

  dch(n) {
    const row = this.screen[this.y]; n = Math.min(n, this.cols - this.x);
    const bg = this.blankRow()[0];
    row.splice(this.x, n); row.push(...Array.from({ length: n }, () => ({ ...bg })));
    this.wrap = false;
  }

  align() {
    this.top = 0; this.bot = this.rows - 1;
    this.screen = Array.from({ length: this.rows }, () => Array.from({ length: this.cols }, () => ({ c: 0x45, a: COL_DEF, f: 0 })));
    this.home();
  }

  save() { this.saved = { x: this.x, y: this.y, col: this.col, fl: this.fl, g: [...this.g], gl: this.gl, wrap: this.wrap, om: this.om }; }
  restore() {
    const s = this.saved || { x: 0, y: 0, col: COL_DEF, fl: 0, g: ['B', 'B'], gl: 0, wrap: false, om: false };
    Object.assign(this, { x: s.x, y: s.y, col: s.col, fl: s.fl, g: [...s.g], gl: s.gl, wrap: s.wrap, om: s.om });
  }

  // ---- What's shown
  static rowText(row) {
    return row.map(k => k.c < 0x20 ? DEC_ASCII[k.c] : String.fromCharCode(k.c)).join('').replace(/ +$/, '');
  }
  lines() { return this.screen.map(VT.rowText); }
  text() { return [...this.sb, ...this.screen].map(VT.rowText).join('\n') + '\n'; }
}

module.exports = { VT, DEC_ASCII };
