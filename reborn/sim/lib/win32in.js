// win32in.js - Windows Terminal's win32-input-mode, for the PC tool and the emulator's terminal (--win32-input).  Asked
// for with ESC [ ? 9001 h (ESC [ ? 9001 l ends it), the terminal sends each key as its record, ESC [ Vk ; Sc ; Uc ; Kd
// ; Cs ; Rc _ (the virtual key, the scan code, the character, down or up, the modifiers, the repeat count), which is
// made here into the bytes an xterm sends for it: the Hydra's console decodes those (KEY_*, keys mods's modifiers)
// and takes Ctrl-Tab, which a terminal has no bytes of its own for, as CSI u's ESC [ 9 ; 5 u (6: Ctrl-Shift-Tab).
// (Windows Terminal keeps Ctrl-Tab for its own tabs till that binding is removed from its settings.)  Anything else
// the terminal sends passes as it came.
'use strict';

const ENABLE = '\x1b[?9001h', DISABLE = '\x1b[?9001l';
const VK_BACK = 0x08, VK_TAB = 0x09, VK_F1 = 0x70;
const ALT = 0x0003, CTRL = 0x000C, SHIFT = 0x0010;             // (Right and left Alt, right and left Ctrl, Shift)
const LETTER = { 0x26: 'A', 0x28: 'B', 0x27: 'C', 0x25: 'D', 0x24: 'H', 0x23: 'F' };   // The arrows, Home, End
const TILDE = { 0x2D: 2, 0x2E: 3, 0x21: 5, 0x22: 6 };       // Insert, Delete, Page Up, Page Down: ESC [ n ~
const FKEY = [11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 23, 24];   // F1-F12's n (F1-F4 unmodified: ESC O P-S)

// A key's record as the bytes an xterm sends: [] for none (a key let go, a modifier alone)
function keyBytes(vk, uc, down, cs, rc) {
  if (!down) return [];
  const ctrl = !!(cs & CTRL), alt = !!(cs & ALT), shift = !!(cs & SHIFT);
  const m = 1 + (shift ? 1 : 0) + (alt ? 2 : 0) + (ctrl ? 4 : 0);   // (xterm's modifier parameter)
  const csi = s => Array.from(Buffer.from('\x1b[' + s, 'latin1'));
  let out;
  if (vk === VK_TAB) out = ctrl ? csi('9;' + (shift ? 6 : 5) + 'u') : shift ? csi('Z') : [0x09];
  else if (LETTER[vk]) out = csi((m > 1 ? '1;' + m : '') + LETTER[vk]);
  else if (TILDE[vk]) out = csi(TILDE[vk] + (m > 1 ? ';' + m : '') + '~');
  else if (vk >= VK_F1 && vk < VK_F1 + 12) {
    const n = vk - VK_F1;
    out = n < 4 ? (m > 1 ? csi('1;' + m + 'PQRS'[n]) : [0x1B, 0x4F, 0x50 + n]) : csi(FKEY[n] + (m > 1 ? ';' + m : '') + '~');
  } else if (vk === VK_BACK) out = [ctrl ? 0x08 : 0x7F];        // (Backspace: DEL, as a terminal sends it; Ctrl's: BS)
  else if (uc) {
    const ch = ctrl && !alt && uc === 0x20 ? [0] : Array.from(Buffer.from(String.fromCharCode(uc), 'utf8'));
    out = alt && !ctrl ? [0x1B, ...ch] : ch;                     // (Alt: ESC first, a meta key's; Ctrl and Alt: AltGr's)
  } else return [];
  const all = [];
  for (let i = 0; i < Math.max(1, rc); i++) all.push(...out);
  return all;
}

// A decoder of what the terminal sends: send(bytes) gets the bytes to pass on.  push(byte) each one; flush() what's
// held (an ESC, or a sequence part-way, with nothing after it for a while)
function createWin32Input(send) {
  let st = 0, held = [];                                      // (st: 0 none, 1 ESC, 2 ESC [ and its numbers)
  return {
    push(b) {
      if (st === 0) {
        if (b === 0x1B) { st = 1; held = [b]; } else send([b]);
        return;
      }
      held.push(b);
      if (st === 1) {
        if (b === 0x5B) st = 2;
        else { st = 0; send(held); }
        return;
      }
      if ((b >= 0x30 && b <= 0x39) || b === 0x3B) {
        if (held.length > 48) { st = 0; send(held); }
        return;
      }
      st = 0;
      if (b !== 0x5F) { send(held); return; }                 // (Not a record: as it came)
      const p = String.fromCharCode(...held.slice(2, -1)).split(';').map(x => x === '' ? 0 : +x);
      const out = keyBytes(p[0] || 0, p[2] || 0, p[3] || 0, p[4] || 0, p[5] || 1);
      if (out.length) send(out);
    },
    flush() { if (st) { st = 0; send(held); } },
    pending: () => st !== 0,
  };
}

module.exports = { createWin32Input, keyBytes, ENABLE, DISABLE };
