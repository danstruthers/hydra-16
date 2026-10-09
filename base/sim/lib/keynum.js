// keynum.js - a browser's keys as the input controller (smc.js) numbers them: KeyboardEvent.code to the IBM PC/AT's
// key numbers (x16-emulator's keyboard.c), for a page that is the Vera X's keyboard (view.js, web/page.js).  No
// Node.js: it runs in a browser too.
'use strict';

const KEYNUM = { Backquote: 1, Minus: 12, Equal: 13, Backspace: 15, Tab: 16, BracketLeft: 27, BracketRight: 28, Backslash: 29,
  CapsLock: 30, Semicolon: 40, Quote: 41, Enter: 43, ShiftLeft: 44, IntlBackslash: 45, Comma: 53, Period: 54, Slash: 55,
  IntlRo: 56, ShiftRight: 57, ControlLeft: 58, MetaLeft: 59, AltLeft: 60, Space: 61, AltRight: 62, MetaRight: 63,
  ControlRight: 64, ContextMenu: 65, Insert: 75, Delete: 76, ArrowLeft: 79, Home: 80, End: 81, ArrowUp: 83, ArrowDown: 84,
  PageUp: 85, PageDown: 86, ArrowRight: 89, NumLock: 90, Numpad7: 91, Numpad4: 92, Numpad1: 93, NumpadDivide: 95,
  Numpad8: 96, Numpad5: 97, Numpad2: 98, Numpad0: 99, NumpadMultiply: 100, Numpad9: 101, Numpad6: 102, Numpad3: 103,
  NumpadDecimal: 104, NumpadSubtract: 105, NumpadAdd: 106, NumpadEnter: 108, Escape: 110, PrintScreen: 124,
  ScrollLock: 125, Pause: 126 };
[...'1234567890'].forEach((c, i) => { KEYNUM['Digit' + c] = 2 + i; });
for (const [first, keys] of [[17, 'QWERTYUIOP'], [31, 'ASDFGHJKL'], [46, 'ZXCVBNM']]) [...keys].forEach((c, i) => { KEYNUM['Key' + c] = first + i; });
for (let i = 1; i <= 12; i++) KEYNUM['F' + i] = 111 + i;

module.exports = { KEYNUM };
