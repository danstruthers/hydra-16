// keys.js - the browser emulator's keyboard for its terminal (page.js's): a key as xterm sends it, and text typed
// as the Hydra's console reads it (Latin-9 bytes).  No DOM here: sim/web.js --check holds it against key events as
// browsers give them, a US layout's and a US-International one's (its dead keys: ' " ` ~ ^).
//   A dead key: its keydown's key is 'Dead', and the browser composes it with the next key (an accent's letter: the
// keydown's key may be it already, or composition events give it) into text, which the terminal's hidden textarea
// gets: its input event (insertText) or its compositionend.  So keydown sends only what it knows (a special key,
// Ctrl's, a letter typed on its own), and leaves a dead key's and an IME's keys to the text; and a key held for a
// dead key's text that never came is sent at the next keydown.
//   Text the Hydra's character set lacks: a spacing accent as its ASCII key (´ as '), and a letter with an accent
// as the accent then the letter ("ẽ": ~e), as a terminal without the letter would show them.
'use strict';

// Latin-9's own (ISO 8859-15's 8 bytes that aren't Latin-1's)
const LATIN9 = { 0xA4: '€', 0xA6: 'Š', 0xA8: 'š', 0xB4: 'Ž', 0xB8: 'ž', 0xBC: 'Œ', 0xBD: 'œ', 0xBE: 'Ÿ' };
const TO_LATIN9 = Object.fromEntries(Object.entries(LATIN9).map(([k, v]) => [v, +k]));
// The accents Latin-9 lacks, as their keys (a dead key and space, on some layouts) and their combining marks
const SPACING = { '´': "'", '¨': '"', '˜': '~', 'ˆ': '^', '¸': ',', '˝': '"', 'ˇ': '^', '˘': '^', '˚': 'o', '‘': "'", '’': "'", '“': '"', '”': '"' };
const COMBINING = { 0x300: '`', 0x301: "'", 0x302: '^', 0x303: '~', 0x308: '"', 0x327: ',', 0x30B: '"', 0x30C: '^', 0x306: '^', 0x30A: 'o' };

// One character as a Latin-9 byte (a 1-character string), or null
const byte9 = ch => {
  const c = ch.codePointAt(0);
  const b = TO_LATIN9[ch] !== undefined ? TO_LATIN9[ch] : c < 256 && !LATIN9[c] ? c : -1;
  return b >= 0 ? String.fromCharCode(b) : null;
};
// Text as Latin-9 bytes: what it lacks, as above (and what has no likeness, left out); null for none
function latin9(s) {
  let out = '';
  for (const ch of s) {
    const b = byte9(ch);
    if (b !== null) { out += b; continue; }
    if (SPACING[ch]) { out += SPACING[ch]; continue; }
    const d = [...ch.normalize('NFD')];                       // (A letter and its accents: the accents, then the letter)
    const base = d.length > 1 ? byte9(d[0]) : null;
    if (base === null) continue;
    let acc = '';
    for (const m of d.slice(1)) { const c = m.codePointAt(0); if (COMBINING[c]) acc += COMBINING[c]; }
    out += acc + base;
  }
  return out || null;
}

// A key as xterm sends it, or null (the browser's to do, or text's)
const FKEYS = { F1: 'OP', F2: 'OQ', F3: 'OR', F4: 'OS', F5: '[15~', F6: '[17~', F7: '[18~', F8: '[19~', F9: '[20~', F10: '[21~', F11: '[23~', F12: '[24~' };
const TILDE = { Insert: 2, Delete: 3, PageUp: 5, PageDown: 6 };
const CURSOR = { ArrowUp: 'A', ArrowDown: 'B', ArrowRight: 'C', ArrowLeft: 'D', Home: 'H', End: 'F' };
function keyText(e, appCursor) {
  const altGr = !!(e.getModifierState && e.getModifierState('AltGraph'));   // (AltGr: Windows says Ctrl and Alt too)
  const ctrl = e.ctrlKey && !altGr, alt = e.altKey && !altGr;
  const mod = 1 + (e.shiftKey ? 1 : 0) + (alt ? 2 : 0) + (ctrl ? 4 : 0);
  if (CURSOR[e.key]) return mod > 1 ? '\x1b[1;' + mod + CURSOR[e.key] : (appCursor ? '\x1bO' : '\x1b[') + CURSOR[e.key];
  if (TILDE[e.key]) return '\x1b[' + TILDE[e.key] + (mod > 1 ? ';' + mod : '') + '~';
  if (FKEYS[e.key]) return '\x1b' + FKEYS[e.key];
  const esc = alt ? '\x1b' : '';
  switch (e.key) {
    case 'Enter': return esc + '\r';
    case 'Backspace': return esc + (ctrl ? '\x08' : '\x7f');
    case 'Tab': return e.shiftKey ? '\x1b[Z' : esc + '\t';
    case 'Escape': return '\x1b';
  }
  if (ctrl && !alt) {                                         // Ctrl and a key: its control character
    if (e.code.startsWith('Key')) return String.fromCharCode(e.code.charCodeAt(3) & 0x1F);
    const c = { Space: 0, Digit2: 0, BracketLeft: 27, Backslash: 28, BracketRight: 29, Digit6: 30, Minus: 31, Slash: 31 }[e.code];
    return c === undefined ? null : String.fromCharCode(c);
  }
  if ([...e.key].length !== 1 || e.metaKey) return null;
  const b = latin9(e.key);
  return b === null ? null : esc + b;
}

// The terminal's keys and text: keydown(e) true if it sent the key (the page then prevents the browser's default),
// input(e) and compositionend(e) the textarea's text; send(s) the bytes
class KeyInput {
  constructor(send) { this.send = send; this.dead = false; this.held = null; }
  keydown(e, appCursor) {
    if (this.held !== null) { const h = latin9(this.held); this.held = null; if (h) this.send(h); }   // (No text came for it)
    if (e.key === 'Dead') { this.dead = true; return false; }
    if (e.isComposing || e.keyCode === 229 || e.key === 'Process' || e.key === 'Unidentified') return false;
    const s = keyText(e, appCursor);
    if (s === null) return false;
    if (this.dead && [...e.key].length === 1 && s === latin9(e.key)) {   // (The key after a dead key: its text is the
      this.dead = false; this.held = e.key; return false;     //   browser's to compose)
    }
    this.dead = false;
    this.send(s);
    return true;
  }
  text(t) {
    this.dead = false; this.held = null;
    const s = latin9(t.replace(/\r\n?|\n/g, '\r'));
    if (s) this.send(s);
  }
  input(e) {
    if (e.inputType === 'insertText' || e.inputType === undefined) { if (e.data) this.text(e.data); }
    else if (e.inputType === 'insertLineBreak' || e.inputType === 'insertParagraph') this.text('\r');
  }
  compositionend(e) { if (e.data) this.text(e.data); }
}

module.exports = { LATIN9, TO_LATIN9, latin9, keyText, KeyInput };
