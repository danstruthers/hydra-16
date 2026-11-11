// ****************************************************************************
// screen.js - the screen's tests: the Vera X, the windows, the terminal's VT100.  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { VT, DEC_ASCII } = require('../../../base/sim/lib/vt.js');
const { createWin32Input } = require('../../../base/sim/lib/win32in.js');
const { card, CARD_DIR, OLD_CARDS, imageCard, hylangCard, typed, expected, pcReport } = require('../lib/common.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

// The console on the Vera X's screen (phase 8: cons's second terminal, vid's /term), at rc: /dev/vid; consctl's
// terminal; the serial port alone (the screen left as it was: a regexp that doesn't match itself, z[z]z, counts
// zzz on the screen), then both (the screen repainted from the window's text); colours (SGR, a file on the PC);
// a font from the ROM disk; a bad command
const SCREEN_LINES = [
  ["ls /dev/vid", "ctl\nterm\nvram\npal\nsprites\nfont\nframe\npsg\npcm\npcmctl\nmouse\nmousein\nmousectl\ndraw"],
  ["cat /dev/vid/ctl", "vera 47.0.2\nmode 80x60\ncursor blink\nborder 0\nbitmap off\noutput vga\nclaimed"],
  ["grep terminal /dev/consctl", "terminal both"],
  ["echo serial >/dev/consctl; echo z^zz; grep -c 'z[z]z' /dev/vid/term; echo both >/dev/consctl", "zzz\n0"],
  ["grep -c 'z[z]z' /dev/vid/term", "1"],
  ["cat /lib/font/cp437 >/dev/vid/font", null],
  ["echo flash >/dev/vid/ctl", "echo: write error: invalid argument"],
  ["echo output ntsc mono 240p >/dev/vid/ctl; grep output /dev/vid/ctl; echo output vga >/dev/vid/ctl", "output ntsc mono 240p"],
  ["echo output vga 240p >/dev/vid/ctl; grep output /dev/vid/ctl", "echo: write error: invalid argument\noutput vga"],
  ["cat /pc/box", "\x1b(0lqk\x1b(B"],
  ["cat /pc/colours", "\x1b[31;44mR\x1b[0mn\x1b[1;32mG\x1b[0;7mV\x1b[m"],
  ["cat /pc/reverse", "\x1b[?5h", true],
];
const SCREEN_COLOURS = '\x1b[31;44mR\x1b[0mn\x1b[1;32mG\x1b[0;7mV\x1b[m\n';

// The console's VT100 (docs/design/plans/WINDOWS.md, W1): what a program writes, each into a window not shown (window 1),
// its /text read back (the vt test); and a window painted on both terminals (vtpaint).  Each expected screen is
// sim/lib/vt.js's; xterm.js's headless terminal, if it's installed (npm install, in reborn/), is held against vt.js
// too, for each fixture but those it differs on by design (false: SUB's error character, DECCOLM's clearing, which
// xterm.js leaves out)
const VT_FIXTURES = [
  ['text', 'Hello, world.\r\nsecond line\nthird\tTAB\tTAB2\x08X\r\n' + 'A'.repeat(85) + '\ndone'],
  ['moves', '\x1b[5;10Hfive-ten\x1b[2Aup2\x1b[3Bdown3\x1b[4Cright4\x1b[20Dleft20\x1b[2Ecnl\x1b[1Fcpl\x1b[30Gcha30' +
    '\x1b[12dvpa12\x1b[40`hpa40\x1b[3aR\x1b[2eE\x1b[99;75Hcorner\x1b[1;1H\x1b[0;0fhome'],
  ['erase', 'aaaaaaaaaa\r\nbbbbbbbbbb\r\ncccccccccc\r\ndddddddddd\r\neeeeeeeeee\r\nffffffffff\r\ngggggggggg' +
    '\x1b[2;5H\x1b[K\x1b[3;5H\x1b[1K\x1b[4;5H\x1b[2K\x1b[1;3H\x1b[4X\x1b[6;3H\x1b[J'],
  ['erase1', 'aaaaaaaaaa\r\nbbbbbbbbbb\r\ncccccccccc\x1b[2;4H\x1b[1Jz'],
  ['insdel', Array.from({ length: 10 }, (v, i) => 'line ' + (i + 1)).join('\r\n') +
    '\x1b[3;1H\x1b[2L\x1b[7;1H\x1b[1M\x1b[1;3H\x1b[2@\x1b[2;2H\x1b[3Pafter'],
  ['region', 'top\r\nsecond\x1b[3;8r\x1b[8;1Hr1\nr2\nr3\n\x1b[3;1H\x1bMri\x1b[5;1H\x1bDind\x1bEnel\x1b[2S\x1b[1T\x1b[rafter'],
  ['scrollback', Array.from({ length: 30 }, (v, i) => 'L' + (i + 1)).join('\n') + '\x1b[1;20r\x1b[20;1H\n\n\nin region'],
  ['tabs', '\tA\tB\x1b[3g\x1b[1;5H\x1bH\x1b[1;13H\x1bH\r\n\tC\tD\tE\x1b[1;30H\x1b[2ZF\x1b[3;1H\x1b[2IG\x1b[0gH'],
  ['wrap', 'X'.repeat(80) + '\rY\n\x1b[?7l' + 'Z'.repeat(85) + '\x1b[?7h\r\n' + 'W'.repeat(80) + 'V'],
  ['insert', 'abcdef\r\x1b[4hXY\x1b[4l\r\nghijkl\x1b[2;3H\x1b[4h12\x1b[4l'],
  ['charsets', '\x1b(0lqqqqk\r\nx    x\r\nmqqqqj\x1b(B\r\n\x1b)0A\x0eqqq\x0fB\r\n\x1b(A#1 pound\x1b(B #2'],
  ['savecursor', '\x1b[3;5Hhere\x1b7\x1b[10;10Hthere\x1b8!\x1b[s\x1b[12;1Hscosc\x1b[u?' +
    '\x1b[5;10r\x1b[?6h\x1b[1;1Horigin\x1b[2;3Hom\x1b[?6l\x1b[r'],
  ['align', 'garbage\x1b#8\x1b[12;35Hcentre'],
  ['ris', 'old text\x1b[5;10r\x1b[4h\r\nmore\x1bcnew'],
  ['rep', 'ab\x1b[5bc\r\n\x1b[1;79H' + 'x\x1b[3b'],
  ['junk', 'A\x1b[?1234hB\x1b[5xC\x1b]0;title\x07D\x1bPq#0;1\x1b\\E\x1b[1\x18F\x1b[2\x1b[1;9HG' + 'y'.repeat(240) +
    '\x1b[31mred\x1b[0m'],
  ['scrolls', Array.from({ length: 30 }, (v, i) => 'S' + (i + 1)).join('\n') + '\x1b[2S\x1b[1;1H\x1b[2Mtop'],
  ['ris2', Array.from({ length: 30 }, (v, i) => 'R' + (i + 1)).join('\n') + '\x1bcnew'],
  ['ed3', Array.from({ length: 30 }, (v, i) => 'E' + (i + 1)).join('\n') + '\x1b[3Jkept'],
  ['alt', Array.from({ length: 28 }, (v, i) => 'M' + (i + 1)).join('\n') + '\x1b[?1049halt text\x1b[5;5Hthere\n\n\n' +
    Array.from({ length: 30 }, (v, i) => 'A' + i).join('\n') + '\x1b[?1049lback'],
  ['alt47', 'main\x1b[?47hon the alternate\x1b[?47l\x1b[2;1Hmain again'],
  ['sgr', '\x1b[1;31mbold red\x1b[0m \x1b[38;5;196mx256\x1b[48;2;0;0;255mrgb\x1b[m \x1b[7mrev\x1b[27m end'],
  ['sub', 'one\x1b[2\x1athree', false],
  ['dwide', 'single\r\n\x1b#6double width, long enough to wrap past forty columns\r\n\x1b#3top\r\n\x1b#4bottom\r\nx\x1b[70G\x1b#6y\r\n' +
    '\x1b#6z\x1b#5back to single', false],
  ['vt52', 'a\x1b[?2lb\x1bAc\x1bBd\x1bY(#xy\x1bFlqk\x1bG\x1bH\x1bIrv\x1b<\x1b[3;1Hansi', false],
  ['colm', 'text\x1b[?3hafter', false],
];
const VT_PAINT = '\x1b[1;31mRed bold\x1b[0m plain \x1b[7mrev\x1b[27m\r\n\x1b(0lqqqk\x1b(B box\r\n' + 'W'.repeat(100) +
  '\r\n\x1b[44;33m blue bg \x1b[0m\r\n\x1b[10;5Hmiddle\x1b[12;1H\x1b#6wide\x1b[3;20r\x1b[15;7Hend';
// (vtpaint's steps: window 1 made, written, shown (a read of its /text a request at a time paints the serial port,
// as a reader waiting for keys would), the screen read; then window 0 shown, and the screen's rows printed)
const VT_PAINT_RC = [
  'echo new >/dev/wctl',
  '{',
  '  cat /pc/vt/paint >[1=3]',
  '  echo current 1 >/dev/wctl',
  '  for(i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16) cat \'#c1/text\' >/dev/null',
  '  cat /dev/vid/term >/ram/scr',
  '  echo current 0 >/dev/wctl',
  '} >[3]\'#c1/cons\'',
  'echo painted',
  'cat /ram/scr',
  'echo done',
].join('\n') + '\n';
// (vtsize's: 30 lines into window 1, every third longer than 60 columns, the cursor then on row 5; then its steps,
// each the bytes written first, the console's size set (terminal size: both terminals on, the screen's bigger), and a
// mark written at the cursor; its /text after each, held against sim/lib/vt.js's resize)
const VT_SIZE = Array.from({ length: 30 }, (v, i) => 'line ' + String(i + 1).padStart(2, '0') + (i % 3 ? '' : ' ' + 'abcdefghij'.repeat(7))).join('\n') + '\n\x1b[5;1H';
const VT_SIZE_STEPS = [
  ['', 80, 10, '<a>'],                                       // shorter, the cursor high: the bottom rows dropped
  ['', 80, 30, '<b>'],                                       // taller: the scrollback's rows down, then blank ones
  ['', 60, 30, '<c>'],                                       // narrower: the long lines cut
  ['', 80, 24, '<d>'],                                       // wider again (what was cut stays gone), shorter
  ['\x1b[?1049h' + Array.from({ length: 20 }, (v, i) => 'alt ' + (i + 1)).join('\n'), 80, 12, '<e>'], // the alternate screen
  ['\x1b[?1049l', 80, 12, '<f>'],                            // the main one again: its cursor where it was saved
  ['', 80, 24, '<g>'],
];
const VT_SIZE_RC = [
  'echo new >/dev/wctl',
  '{',
  '  cat /pc/vt/size >[1=3]',
  ...VT_SIZE_STEPS.flatMap(([pre, c, r, mark], i) => [
    ...(pre ? ['  cat /pc/vt/size' + i + ' >[1=3]'] : []),
    '  echo terminal size ' + c + ' ' + r + ' >/dev/consctl',
    "  echo -n '" + mark + "' >[1=3]",
    "  echo '[" + i + "]'; cat '#c1/text'; echo '[/" + i + "]'",
  ]),
  "} >[3]'#c1/cons'",
  'echo terminal size 80 24 >/dev/consctl',
  'echo done',
].join('\n') + '\n';
const vtSizeFiles = () => {
  const files = { 'vt/size': vtFile(VT_SIZE), 'vt/size.rc': VT_SIZE_RC };
  VT_SIZE_STEPS.forEach(([pre], i) => { if (pre) files['vt/size' + i] = vtFile(pre); });
  return files;
};
// (vtmode's: the screen alone, vid's mode changed under the console; its sizes, and the screen as painted at 40x30)
const VT_MODE_RC = [
  'echo screen >/dev/consctl',
  'grep size /dev/consctl >/ram/s1',
  'echo mode 40x30 >/dev/vid/ctl',
  'echo after 40x30',
  'grep size /dev/consctl >/ram/s2',
  'cat /dev/vid/term >/ram/t2',
  'echo mode 80x30 >/dev/vid/ctl',
  'echo after 80x30',
  'grep size /dev/consctl >/ram/s3',
  'echo both >/dev/consctl',
  'grep size /dev/consctl >/ram/s4',
  'cat /ram/s1 /ram/s2 /ram/s3 /ram/s4',
  "echo '[t2]'",
  'cat /ram/t2',
  "echo '[/t2]'",
  'echo mode 80x60 >/dev/vid/ctl',
  'echo done',
].join('\n') + '\n';
// (vtedit's: two lines edited in a window 20 columns wide, each longer than its row: moved over (Left across the
// rows), inserted into, deleted from, Home and End; then the first recalled (Up) and cut back; a third made 30 wide
// as it's typed (the terminal's report, typed).  edSim: what the line editor makes of keys, the lines before for Up)
const VT_EDIT = [
  'echo 0123456789abcdefghijklmnopqrstuvwxyz' + '\x1b[D'.repeat(25) + 'XY' + '\x01' + '\x1b[C'.repeat(7) + '\x1b[3~\x1b[3~' + '\x05' + '!',
  '\x1b[A' + '\x7f'.repeat(30) + ' ok',
  'echo abcdefghijklmnopqrstuvwxyz0123' + '\x1b[8;24;30t' + '\x1b[D'.repeat(3) + 'Z',
];
function edSim(keys, hist) {
  let s = '', p = 0;
  for (const k of keys.match(/\x1b\[8;24;30t|\x1b\[3~|\x1b\[[A-D]|[\s\S]/g)) {
    if (k === '\x1b[D') { if (p > 0) p--; }
    else if (k === '\x1b[C') { if (p < s.length) p++; }
    else if (k === '\x01') p = 0;
    else if (k === '\x05') p = s.length;
    else if (k === '\x7f') { if (p > 0) { s = s.slice(0, p - 1) + s.slice(p); p--; } }
    else if (k === '\x1b[3~') { if (p < s.length) s = s.slice(0, p) + s.slice(p + 1); }
    else if (k === '\x1b[A') { s = hist[hist.length - 1]; p = s.length; }
    else if (k === '\x1b[8;24;30t') { }
    else { s = s.slice(0, p) + k + s.slice(p); p++; }
  }
  return s;
}
// (winchrome's: the chrome on the screen alone, its rows read back from /dev/vid/term at each step: the bar's and
// header's (the screen's first two), the footer's (its last, or with the bar at the bottom the two last))
const CHROME_RC = [
  'echo screen >/dev/consctl',
  'echo -n mywin >/dev/label',
  'cat /dev/label >/ram/l0',
  'echo status hello there >/dev/wctl',
  'cat /dev/vid/term >/ram/s1',
  'cat /pc/vt/sasd',
  "echo 'footer %[7]%s%=%c x %r %m' >/dev/wctl",
  "echo 'header [%p] %l%=%n' >/dev/wctl",
  'cat /dev/vid/term >/ram/s2',
  'echo bar bottom >/dev/wctl',
  'cat /dev/vid/term >/ram/s3',
  'echo chrome screen off header >/dev/wctl',
  'grep size /dev/consctl >/ram/z1',
  'echo bar off >/dev/wctl',
  'grep size /dev/consctl >>/ram/z1',
  'echo chrome screen off >/dev/wctl',
  'grep size /dev/consctl >>/ram/z1',
  'echo chrome screen on >/dev/wctl',
  'echo bar top >/dev/wctl',
  'grep size /dev/consctl >>/ram/z1',
  'cat /pc/vt/title',
  'cat /dev/label >/ram/l1',
  'echo new >/dev/wctl',
  '{',
  "  echo monitor on >'#c1/wctl'",
  '  echo hidden >[1=3]',
  '  cat /dev/vid/term >/ram/s4',
  '  cat /pc/vt/bel >[1=3]',
  '  cat /dev/vid/term >/ram/s5',
  "} >[3]'#c1/cons'",
  'echo >/dev/label',
  'cat /dev/vid/term >/ram/s6',
  'echo both >/dev/consctl',
  "echo '[l]'; cat /ram/l0; echo; cat /ram/l1; echo",
  "echo '[z]'; cat /ram/z1",
  "for (n in 1 2 4 5 6) { echo '[s'^$n^']'; head -2 /ram/s^$n; tail -1 /ram/s^$n }",
  "echo '[s3]'; head -1 /ram/s3; tail -2 /ram/s3",
  'echo done',
].join('\n') + '\n';
// (vtjump's: the ROM disk's api.md, 38K, cat to the window shown with scroll jump)
const vtJump = () => fs.readFileSync(path.join(TESTS, '..', 'obj', 'gen', 'api.md'), 'latin1');
const vtModel = bytes => new VT({ onlcr: true }).write(bytes);
const vtText = bytes => vtModel(bytes).text().replace(/\n$/, '');
const vtFile = bytes => () => Buffer.from(bytes, 'latin1');
const VT_LINES = VT_FIXTURES.map(([n, b]) => ["echo new >/dev/wctl; {cat /pc/vt/" + n + " >[1=3]; cat '#c1/text'} >[3]'#c1/cons'", vtText(b)]);
// xterm.js's headless terminal's screen for bytes (as a window's /text: its scrollback, then its screen; the DEC
// graphics as the console's ASCII), or null if it isn't installed
const XTERM_DEC = '◆▒␉␌␍␊°±␤␋┘┐┌└┼' +
  '⎺⎻─⎼⎽├┤┴┬│≤≥π≠£·';
function xtermText(bytes) {
  let Terminal;
  try { ({ Terminal } = require('@xterm/headless')); } catch (e) { return null; }
  const t = new Terminal({ cols: 80, rows: 24, scrollback: 40, convertEol: true, allowProposedApi: true });
  const warn = console.warn, log = console.log;
  console.warn = console.log = () => {};                    // (writeSync's warning: it's a test's, not a program's)
  try { t._core.writeSync(bytes); } finally { console.warn = warn; console.log = log; }
  const b = t.buffer.active, lines = [];
  for (let i = 0; i < b.length; i++) {
    lines.push([...b.getLine(i).translateToString(true)].map(ch => {
      const k = XTERM_DEC.indexOf(ch);
      return k >= 0 && ch !== '£' && ch !== '°' && ch !== '±' && ch !== '·' ? DEC_ASCII[k + 1] : ch;
    }).join('').replace(/ +$/, ''));
  }
  t.dispose();
  return lines.join('\n');
}
// The vt test's check: vt.js held against xterm.js (if it's there), fixture by fixture
function vtXterm() {
  const f = [];
  let checked = 0;
  for (const [n, b, xt] of VT_FIXTURES) {
    if (xt === false) continue;
    const x = xtermText(b);
    if (x === null) return { f, note: 'xterm.js not installed (npm install in reborn/): vt.js not checked against it' };
    checked++;
    const v = vtText(b);
    if (x !== v) {
      const xl = x.split('\n'), vl = v.split('\n');
      const i = xl.findIndex((l, k) => l !== vl[k]);
      f.push('vt.js and xterm.js differ on ' + n + ', line ' + (i + 1) + ': ' + JSON.stringify(vl[i]) + ' and ' + JSON.stringify(xl[i]));
    }
  }
  return { f, note: 'vt.js held against xterm.js: ' + checked + ' fixtures' };
}

// The ramw test's lines, at the login shell (HyForth): a file of 70 writes of 10 bytes, then written over in its
// first block, across its first block's end and at its end (one byte more after it), on the RAM disk, the shared one
// and a card, each read back; then 100 writes of 16 bytes to /ram, and to a card, timed (the file made before them and
// closed after; the marks [rw A] and [rw B] made by emit and .(, so the line typed isn't one); then two files on the
// card left open, each written twice (the first write allocating: its data goes to the card before the map does), the
// one's second write synced, the other's left kept back (nothing after it reaches the storage driver); and its card
// (blank HydraFS)
const RAMW_LINES = ['variable fd', ': w ( a u -- ) w/o create-file throw fd ! ;', ': p ( a u -- ) fd @ write-file throw ;',
  ': at ( n -- ) s>d fd @ reposition-file throw ;', ': c fd @ close-file throw ;',
  ': t ( a u -- ) w 70 0 do s" 0123456789" p loop 5 at s" abc" p 508 at s" XYZWV" p 699 at s" !+" p c ;',
  ': n ( -- ) 100 0 do s" 0123456789abcdef" p loop ;',
  's" /ram/w" t', 's" /sram/w" t', 's" /sd/0/w" t', 'cat /ram/w; echo; cat /sram/w; echo; cat /sd/0/w; echo',
  ': h ( a u -- ) w s" ab" p 3000 at s" Z" p c ;', 's" /ram/h" h s" /sram/h" h s" /sd/0/h" h', 'ls -l /ram/h /sram/h /sd/0/h',
  's" /ram/n" w', '91 emit .( b0 A])', '91 emit .( b0 B])', '91 emit .( rw A])', 'n', '91 emit .( rw B])', 'c',
  's" /sd/0/n" w', '91 emit .( cw A])', 'n', '91 emit .( cw B])', 'c', 'ls -l /ram/n /sd/0/n',
  's" /sd/0/k" w s" first part, written. " p s" then synced" p', 'echo sync >/dev/sd/0/ctl',
  's" /sd/0/u" w s" first part, written. " p s" second part: kept back" p', '91 emit .( ramw done.)'];
const RAMW_TEXT = (() => { const b = [...'0123456789'.repeat(70)]; b.splice(5, 3, ...'abc'); b.splice(508, 5, ...'XYZWV'); b.splice(699, 1, '!', '+'); return b.join(''); })();
function ramwCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const img = path.join(CARD_DIR, 'ramw.img');
  fs.rmSync(img, { force: true });
  hydrafs.mkfs(img, 8, 'RAMW', undefined, true);
  return imageCard(0, img, Math.floor(fs.statSync(img).size / 512));
}

// The draw test's lines at HyForth (lib video) and hylang (video.hl), and what each prints ('': nothing): vid's
// /dev/vid/draw on the bitmap at each depth, read back by vpeek; the turtle; rc's lines to the file
const DRAW_FORTH = [
  ['echo serial >/dev/consctl', ''], ['lib video', ''], ['320 8 bitmap  0 pen clear  5 pen 10 20 plot  20 320 * 10 + 0 vpeek .', '5 '],
  ['0 0 9 0 line  4 0 vpeek .', '5 '], ['20 20 29 29 box  20 320 * 25 + 0 vpeek .  25 320 * 25 + 0 vpeek .', '5 0 '],
  ['40 40 44 42 bar  41 320 * 42 + 0 vpeek .  43 320 * 42 + 0 vpeek .', '5 0 '],
  ['100 100 10 circle  100 320 * 110 + 0 vpeek .  100 320 * 100 + 0 vpeek .', '5 0 '],
  ['200 100 5 disc  100 320 * 203 + 0 vpeek .', '5 '], ['-5 -5 plot  5000 0 plot', 'plot: invalid argument'],
  ['320 4 bitmap  0 pen clear  7 pen 1 0 plot  0 0 vpeek .  12 pen 0 0 plot  0 0 vpeek .', '7 199 '],
  ['320 2 bitmap  0 pen clear  2 pen 2 0 plot  0 0 vpeek .', '8 '],
  ['640 1 bitmap  0 pen clear  1 pen 3 0 plot  0 0 vpeek .  0 479 639 479 line  479 80 * 0 vpeek .', '16 255 '],
  ['640 4 bitmap', 'invalid argument'], ['320 8 bitmap  cs 7 pen 50 fd 90 rt 40 fd heading .', '90 '],
  ['95 320 * 160 + 0 vpeek .  70 320 * 180 + 0 vpeek .', '7 7 '], ['200 $F00 palette!  2 100 50 sprite-at', ''],
  ['0 pen clear  6 pen 0 0 s" Hi" text  1 0 vpeek .  0 0 vpeek .  3 320 * 3 + 0 vpeek .', '6 0 6 '],
  ['echo pen 4 >/dev/vid/draw; cat /dev/vid/draw', 'pen 4'],
];
const DRAW_HY = [['(use "video")', 'NIL'], ['(pen 9)', 'NIL'], ['(plot 30 30)', 'NIL'], ['(vpeek (+ (* 30 320) 30))', '9'], ['(cs)', 'NIL'],
  ['(pen 11)', 'NIL'], ['(fd 30)', 'NIL'], ['(vpeek (+ (* 95 320) 160))', '11'], ['(heading)', '0'],
  ['(text 0 10 "H")', 'NIL'], ['(vpeek (+ (* 10 320) 1))', '11']];

// The vsd test's card: a HydraFS volume (hello.txt) on the VERA's own SD port, its writes kept; and its lines
function veraCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  const img = path.join(CARD_DIR, 'vsd.img');
  hydrafs.setNow(0x1000);
  hydrafs.mkfs(img, 8, 'VERASD', undefined, true);
  const v = new hydrafs.Volume(img);
  v.put('hello.txt', Buffer.from('hello from the vera\n'));
  v.close();
  return imageCard(0, img, Math.floor(fs.statSync(img).size / 512));
}
const VSD_LINES = [
  ['cat /dev/sd/v/ctl', 'sdhc 8 MB 16384 blocks\nhydrafs label=VERASD\nfree 8180 KB of 8188 KB'],
  ['cat /sd/v/hello.txt', 'hello from the vera'],
  ['echo written >/sd/v/new.txt; cat /sd/v/new.txt', 'written'],
  ['ls /sd', 'v/'],
];

module.exports = {
  tests: [
    {
      name: 'hydev', what: 'hylang\'s device libraries (the plan\'s phase 11: /lib/hylang\'s, loaded by use, over the devices\' files), devices.hl as a script: gpio (pins, the port, ctl as a hash, CA1\'s edge), i2c (a memory written and read at a register, the devices, one that doesn\'t answer), spi (an echo device\'s transactions, mode 3), cons (the window, the windows, the bell), proc (a task\'s args, cwd, regs, memory, banks; its environment, its namespace), clock (the chip, the time set), disk (the disks, the cards: this one and one on SPI device 5; the ROM disk\'s room), pc (the PC tool answers; a file of its read), snd (note-of; a tune, its notes on the YM2151 in time; a channel\'s settings; the registers read back: a bent note\'s key code and fraction; a frequency, a glide, the LFO, a sensitivity, the noise; a line of MML and a chord, by play)',
      init: 't_rc', cycles: 400e6, pc: { files: { 'hi.txt': 'hi from the PC\n' } },
      get machine() {
        return { gpioIn: 0xA5, ca1: Array.from({ length: 60 }, (_, i) => 100e6 + i * 50e6), i2c: { 0x50: 256, 0x68: 16 }, spiEcho: [3],
          sd: [...hylangCard(this.name), card(5, 2048, false, () => 0)], rtc: Date.UTC(2026, 9, 3, 15, 4, 5) / 1000,
          input: '\u0101cd /sd/0; hylang devices.hl; echo status $status\r' };
      },
      expect: ['72 checks, 0 failed\nstatus\n%'],
      // (The tune: C4, E4 a beat on (a tenth of a second at 600 a minute), a rest, G4 two beats after E4)
      check(m) {
        const f = pcReport(m, 1, 0, 0), mult = JSON.parse(fs.readFileSync(path.join(TESTS, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const on = m.ym.keyOns.filter(k => k.startsWith('ch 0 ')).map(k => +k.match(/at cycle (\d+)/)[1]);
        const beat = 0.1 * 3579545 * mult, slack = 2 * 3579545 * mult / 200;
        if (on.length !== 3) return [...f, 'tune: ' + on.length + ' key-ons on channel 0, not 3: ' + m.ym.keyOns.join(', ')];
        [1, 2].forEach((beats, k) => { if (Math.abs(on[k + 1] - on[k] - beats * beat) > slack)
          f.push('tune: key-on ' + (k + 1) + ' came ' + (on[k + 1] - on[k]) + ' cycles after the last, not ' + beats + ' beat(s) (' + Math.round(beats * beat) + ')'); });
        return f;
      },
    },
    // ---- Phase 8: the Vera X (the emulator's VERA: sim/lib/vera.js; the driver: modules/vid)
    {
      name: 'vera', what: 'the emulator\'s Vera X (sim/lib/vera.js), the chip as a program sees it (no vid): the version register; ADDR0 and ADDR1, their steps, a data port\'s byte fetched ahead; the display\'s registers at the start; VSYNC (59.5 a second), LINE and SCANLINE (bit 8 too); sprites colliding; the PCM FIFO (empty, full, AFLOW and its interrupt\'s time); a PSG voice; the SPI port with no card; FX (the cache\'s writes and fill, transparency, the multiplier, the line helper, the fill length); CTRL\'s reset',
      init: 't_vera', without: ['vid'], cycles: 40e6, machine: { vera: true },
      check(m) {
        const f = [];
        if (!m.vera.psgOns.some(k => k.startsWith('voice 0 '))) f.push('PSG voice 0 never came on');
        if (m.vera.pcmOut < 977) f.push('the PCM FIFO drained ' + m.vera.pcmOut + ' bytes (977 at least)');
        this.notes = ['the VERA: ' + m.vera.frames + ' frames, ' + m.vera.pcmIn + ' PCM bytes in, ' + m.vera.pcmOut + ' out, ' + m.vera.pcmLost + ' lost (full)'];
        return f;
      },
    },
    {
      name: 'vid', what: 'the Vera X\'s driver (vid: #v), through its files: ctl\'s state; the terminal (/term): text written and read back, a CSI move, a line erased, wrapping, BS and TAB, 70 lines scrolled, SGR\'s colours (in the map\'s cells), the cursor\'s sprite; /frame (a frame a read, 59.5 a second); /vram, /pal, /font, the files\' lengths; ctl\'s commands (mode, cursor, border, bitmap, bad ones); claims: a write to the terminal E_BUSY meanwhile, shown after the release; claim all (the font back); another task\'s (E_BUSY), ended by its end',
      init: 't_vid', cycles: 80e6, machine: { vera: true },
    },
    {
      name: 'vid-none', what: 'vid with no card: its init looks for DETECT_TICKS, then ends; no #v (E_NODEV)',
      init: 't_vid', cycles: 30e6, expect: ['ok - no card: #v isn\'t there (E_NODEV)', 't_vid: PASS'],
    },
    {
      name: 'screen', what: 'the console on the Vera X\'s screen (cons\'s second terminal: vid\'s /term), at rc: /dev/vid; consctl\'s terminal both, serial (the screen left as it was), both again (repainted); the boot\'s lines whole (the console\'s ESC [ 18 t not mixed into the kernel\'s next); a font written to /dev/vid/font; ctl\'s output (NTSC, mono, 240p; VGA has neither); colours from a file (SGR, in the cells); DECSCNM (every cell reversed); the DEC graphics (ESC ( 0) as the font\'s glyphs; what rc shows, on the screen as on the serial port',
      init: 't_rc', cycles: 150e6, pc: { files: { colours: SCREEN_COLOURS, box: '\x1b(0lqk\x1b(B\n', reverse: '\x1b[?5h' } },
      get machine() { return { input: typed(SCREEN_LINES), vera: true }; },
      get expect() { return expected(SCREEN_LINES); },
      check(m, out) {
        const f = [], c = m.vera.cells(), text = m.vera.text();
        if (!c) return ['no text layer on the screen'];
        // (The console's question to the terminal, ESC [ 18 t, sent whole before the kernel's next line: cons's init waits for it)
        if (!out.includes('\x1b[18ttask 1: t_rc')) f.push('the boot\'s "task 1: t_rc" isn\'t whole after the console\'s ESC [ 18 t');
        // (The window shown is the smaller terminal's size, the serial port's 80 x 24: its rows below the screen's
        // chrome, the bar and its header, then its footer: W4; rc's line looked for one its last 24 rows still have)
        if (!text.some(l => l.startsWith('% echo flash >/dev/vid/ctl'))) f.push('the screen lacks rc\'s line "% echo flash >/dev/vid/ctl"');
        if (text.slice(27).some(l => l)) f.push('the screen has text below the window\'s 24 rows and its footer');
        if (!/^ 0 \S+ .* \d\d:\d\d$/.test(text[0] || '')) f.push('the screen\'s bar (row 1) isn\'t " 0 label ... HH:MM": ' + JSON.stringify(text[0]));
        if (!/^0 \S+ .* 0 \S+$/.test(text[1] || '')) f.push('the window\'s header (row 2) isn\'t "0 label ... 0 label": ' + JSON.stringify(text[1]));
        if (!text.some(l => l === 'terminal both')) f.push('the screen lacks consctl\'s "terminal both"');
        // (The DEC graphics, ESC ( 0's lqk, as the font's glyphs $0D $12 $0C: tools/decfont.js's)
        let box = false;
        for (let i = 0; i + 2 < c.chars.length; i++) if (c.chars[i] === 0x0D && c.chars[i + 1] === 0x12 && c.chars[i + 2] === 0x0C) box = true;
        if (!box) f.push('the screen lacks the DEC graphics\' corner, line and corner (glyphs $0D $12 $0C)');
        const row = text.findIndex(l => l === 'RnGV');
        if (row < 0) f.push('the screen lacks the colours\' line RnGV');
        else {
          const at = row * c.cols, attrs = Array.from(c.attrs.subarray(at, at + 4)).map(a => '$' + a.toString(16).toUpperCase().padStart(2, '0')).join(' ');
          // (DECSCNM set at the end: each cell's colours reversed; V's, reversed itself, plain)
          if (attrs !== '$14 $70 $A0 $07') f.push('the colours\' cells, the screen reversed: ' + attrs + ' ($14 $70 $A0 $07 wanted)');
        }
        const font = fs.readFileSync(path.join(TESTS, '..', 'romfs', 'lib', 'font', 'cp437'));
        if (!Buffer.from(m.vera.vram.subarray(0x1F000, 0x1F800)).equals(font)) f.push('VRAM\'s font isn\'t /lib/font/cp437');
        this.notes = ['the screen\'s last rows: ' + JSON.stringify(text.filter(l => l).slice(-3))];
        return f;
      },
    },
    {
      name: 'kbinclock', what: 'the console\'s keyboard (the Vera X\'s, the SMC\'s: kdev\'s input writing #c/kbin) typed at slowly as the bar\'s clock turns a minute (the user\'s, 2026-10-16, in the browser: "only printed about 3/4 of the text and then froze"): the chrome drawn again in the pump after a /kbin write read #t/time, a call to kdev, busy with its look and waiting for that write\'s answer, the two waiting for each other for good; now the console makes the time from the kernel\'s clock (TIME), no call to kdev.  And consctl\'s keyboard screen once the keyboard\'s typed',
      init: 't_rc', cycles: 260e6,
      // (The clock at 15:04:52 as it boots: its minute turns as the keys come, one each 2M cycles)
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2026, 9, 16, 15, 4, 52) / 1000,
        input: 'ācat /dev/consctl\rāĂecho ' + 'kĀ'.repeat(48) + '\răācat /dev/consctl\rāecho do^ne\r' }; },
      expect: ['\n' + 'k'.repeat(48) + '\n', '\nkeyboard screen\n', '\ndone\n'],
      check(m, out) {
        const f = [], first = out.slice(0, out.indexOf('echo k'));
        if (first.includes('keyboard screen')) f.push('consctl said keyboard screen before the keyboard had typed');
        if (!/ 15:0[5-9]$/.test(m.vera.text()[0] || '')) f.push('the bar\'s clock didn\'t turn a minute: ' + JSON.stringify(m.vera.text()[0]));
        return f;
      },
    },
    {
      name: 'kbinstatus', what: 'the console\'s keyboard (kdev\'s input writing #c/kbin) typed at while a loop writes the window\'s status (wctl\'s status, its footer %s %t: each write\'s pump draws the chrome again, the time with it): the console, serving rc\'s write, read #t/time (a call to kdev) while kdev\'s input, busy with its look, waited to write /kbin to it, each waiting for the other for good (the second freeze, 2026-10-16: dq0\'s consctl write as hmchar began; the first, kbinclock\'s, the pump after the /kbin write itself); the time the kernel\'s now (TIME), no call to kdev at all',
      init: 't_rc', cycles: 400e6,
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2026, 9, 16, 15, 4, 0) / 1000,
        input: '\u0101echo footer %s %t >/dev/wctl\r\u0101d=(0 1 2 3 4 5 6 7 8 9); for(i in 0 1 2) for(j in $d) for(k in $d) echo status $i$j$k >/dev/wctl\r' +
          '\u0102echo ' + 'k\u0100'.repeat(40) + '\r\u0103' + '\u0100'.repeat(20) + 'echo do^ne\r' }; },
      expect: ['\n' + 'k'.repeat(40) + '\n', '\ndone\n'],
      check(m) {
        const foot = m.vera.text().filter(l => l).pop() || '';
        return /^299 15:0\d$/.test(foot) ? [] : ['the footer, the loop done: ' + JSON.stringify(foot) + ' ("299 15:0N" wanted)'];
      },
    },
    {
      name: 'chrometime', what: 'the bar\'s date and time (%d %t) made by the console from the kernel\'s clock (TIME), not #t/time (a call to kdev, which could wait for good on kdev\'s input writing #c/kbin): its calendar kdev\'s, a leap day\'s last seconds and the next day\'s first, as /dev/time has them',
      init: 't_rc', cycles: 120e6,
      // (SNAP: the screen's bar, as the keyboard's typing comes to it; nothing typed)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push(m.vera.text()[0]) : type(ch);
      },
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2028, 1, 29, 23, 59, 50) / 1000,
        input: 'āecho bar %d %t >/dev/wctl\rācat /dev/time\rāĂЀă' + 'Ā'.repeat(20) + 'cat /dev/time\rāĂЀăecho do^ne\r' }; },
      expect: ['\n2028-02-29 23:59:5', '\n2028-03-01 00:00:', '\ndone\n'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 2) return ['the snapshots: ' + s.length + ' (2 wanted)'];
        if (!/^2028-02-29 23:59 *$/.test(s[0])) f.push('the bar before midnight: ' + JSON.stringify(s[0]));
        if (!/^2028-03-01 00:00 *$/.test(s[1])) f.push('the bar after midnight: ' + JSON.stringify(s[1]));
        return f;
      },
    },
    {
      name: 'chrometime2', what: 'the bar\'s date and time (chrometime\'s) in 2100, not a leap year (a century): its February\'s last seconds and March\'s first; the date and time (%d %t) made by the console from the kernel\'s clock (TIME), not #t/time (a call to kdev, which could wait for good on kdev\'s input writing #c/kbin): its calendar kdev\'s, a leap day\'s last seconds and the next day\'s first, as /dev/time has them',
      init: 't_rc', cycles: 120e6,
      // (SNAP: the screen's bar, as the keyboard's typing comes to it; nothing typed)
      start(m) {
        const type = m.smc.type, snaps = this.snaps = [];
        m.smc.type = ch => ch === 'Ѐ' ? snaps.push(m.vera.text()[0]) : type(ch);
      },
      get machine() { return { vera: true, smc: true, rtc: Date.UTC(2100, 1, 28, 23, 59, 50) / 1000,
        input: 'āecho bar %d %t >/dev/wctl\rācat /dev/time\rāĂЀă' + 'Ā'.repeat(20) + 'cat /dev/time\rāĂЀăecho do^ne\r' }; },
      expect: ['\n2100-02-28 23:59:5', '\n2100-03-01 00:00:', '\ndone\n'],
      check(m, out) {
        const f = [], s = this.snaps || [];
        if (s.length < 2) return ['the snapshots: ' + s.length + ' (2 wanted)'];
        if (!/^2100-02-28 23:59 *$/.test(s[0])) f.push('the bar before midnight: ' + JSON.stringify(s[0]));
        if (!/^2100-03-01 00:00 *$/.test(s[1])) f.push('the bar after midnight: ' + JSON.stringify(s[1]));
        return f;
      },
    },
    {
      name: 'vt', what: 'the console\'s VT100 (W1): sequences into a window not shown, its /text read back: text, the cursor\'s moves, erasing, inserting and deleting, the scrolling region, the scrollback, tabs, autowrap, insert mode, the character sets, DECSC and origin mode, DECALN, RIS, REP, SGR, VT52 mode, the alternate screen, double width and height, sequences dropped, cancelled and split; each as sim/lib/vt.js has it, and vt.js as xterm.js has it (if it\'s installed)',
      init: 't_rc', cycles: 600e6,
      pc: { files: () => Object.fromEntries(VT_FIXTURES.map(([n, b]) => ['vt/' + n, vtFile(b)])) },
      get machine() { return { input: typed(VT_LINES) }; },
      get expect() { return expected(VT_LINES); },
      check() { const r = vtXterm(); this.notes = [r.note]; return r.f; },
    },
    {
      name: 'vtpaint', what: 'a window painted (W1): text in colours, a box in DEC graphics, a line autowrapped, a double-width row, a region and the cursor, written into a window not shown, which is then shown: what the serial port\'s terminal shows (sim/lib/vt.js: its characters and colours) and what the screen shows, as the window has it',
      init: 't_rc', cycles: 200e6, pc: { files: { 'vt/paint': vtFile(VT_PAINT), 'vt/paint.rc': VT_PAINT_RC } },
      get machine() {
        return { vera: true, input: typed([['rc /pc/vt/paint.rc']]) };
      },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], want = vtModel(VT_PAINT), raw = m.out;
        const clear = '\x1b[0m\x1b(B\x1b)B\x0f', at = raw.indexOf('rc /pc/vt/paint.rc');
        const a = raw.indexOf(clear, at), b = raw.indexOf(clear, a + 1);
        if (a < 0 || b < 0) return ['the serial port: no paint of window 1 (and of window 0 after it)'];
        const ser = new VT().write(raw.slice(a, b));
        for (let r = 0; r < 24; r++) {
          for (let c = 0; c < 80; c++) {
            const x = want.screen[r][c], y = ser.screen[r][c];
            if (x.c !== y.c || x.a !== y.a || x.f !== y.f) { f.push('the serial port\'s terminal: row ' + (r + 1) + ', column ' + (c + 1) + ': ' + JSON.stringify(y) + ', not ' + JSON.stringify(x)); break; }
          }
          if (f.length) break;
        }
        if (ser.y !== want.y || ser.x !== want.x) f.push('the serial port\'s cursor at ' + (ser.y + 1) + ';' + (ser.x + 1) + ', not ' + (want.y + 1) + ';' + (want.x + 1));
        if (ser.top !== want.top || ser.bot !== want.bot) f.push('the serial port\'s region ' + (ser.top + 1) + '-' + (ser.bot + 1) + ', not ' + (want.top + 1) + '-' + (want.bot + 1));
        const out = raw.replace(/\r/g, ''), s0 = out.indexOf('\npainted\n');
        const scr = s0 < 0 ? [] : out.slice(s0 + 9).split('\n').map(l => l.replace(/ +$/, ''));
        const rows = want.screen.map(r => r.dw ? [...VT.rowText(r)].map(c => c + ' ').join('').replace(/ +$/, '') : VT.rowText(r));
        for (let r = 0; r < 24; r++) if (scr[r + 2] !== rows[r]) { f.push('the screen\'s row ' + (r + 3) + ' (the window\'s ' + (r + 1) + ', below the bar and its header): ' + JSON.stringify(scr[r + 2]) + ', not ' + JSON.stringify(rows[r])); break; }
        return f;
      },
    },
    {
      name: 'vtjump', what: 'scroll jump (consctl): the ROM disk\'s api.md (38K) cat to the window shown, its writer not waiting for the serial line, the terminal painted as it can: far fewer bytes sent than written, the file\'s last line shown at the end; scroll smooth again',
      init: 't_rc', cycles: 300e6,
      get machine() { return { input: typed([['echo scroll jump >/dev/consctl; cat /rom/doc/api.md; echo scroll smooth >/dev/consctl; echo after']]) }; },
      expect: ['\nafter\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), at = out.indexOf('echo after\n'), end = out.indexOf('\nafter\n', at);
        if (at < 0 || end < 0) return ['no output between the command and its end'];
        const big = vtJump(), sent = end - at, last = big.trim().split('\n').pop();
        this.notes = ['sent ' + sent + ' bytes for the file\'s ' + big.length];
        if (sent > big.length * 0.6) f.push('scroll jump sent ' + sent + ' bytes of the file\'s ' + big.length + ' (more than 60%)');
        if (!out.slice(at, end).includes(last)) f.push('the file\'s last line not shown: ' + last);
        return f;
      },
    },
    {
      name: 'vtsize', what: 'a window\'s size (W3: terminal size, both terminals on): a window not shown with 30 lines in it, made shorter (the cursor high: rows dropped at the bottom), taller (the scrollback\'s rows down onto it, then blank ones), narrower (the long lines cut), wider; its alternate screen in use, shorter (rows off its top dropped), then the main one again (the saved cursor where its row went); its /text after each, a mark at its cursor, as sim/lib/vt.js\'s resize has it',
      init: 't_rc', cycles: 150e6, pc: { files: vtSizeFiles() },
      get machine() { return { input: typed([['rc /pc/vt/size.rc']]) }; },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), want = new VT({ onlcr: true }).write(VT_SIZE);
        VT_SIZE_STEPS.forEach(([pre, c, r, mark], i) => {
          want.write(pre).resize(c, r).write(mark);
          const a = out.indexOf('[' + i + ']\n'), b = out.indexOf('[/' + i + ']', a);
          if (a < 0 || b < 0) { f.push('step ' + i + ': no /text read'); return; }
          const got = out.slice(a + 3 + String(i).length, b).replace(/\n$/, ''), exp = want.text().replace(/\n$/, '');
          if (got !== exp) {
            const g = got.split('\n'), e = exp.split('\n'), k = e.findIndex((l, j) => l !== g[j]);
            f.push('step ' + i + ' (' + c + ' x ' + r + '): /text row ' + (k < 0 ? g.length : k + 1) + ' ' + JSON.stringify(g[k < 0 ? e.length : k]) + ', not ' + JSON.stringify(e[k]) + ' (' + g.length + ' rows, not ' + e.length + ')');
          }
        });
        return f;
      },
    },
    {
      name: 'vtmode', what: 'a window\'s size from the screen\'s (W3): the screen alone (80 x 60, less its chrome\'s 3 rows: the bar, the header, the footer), vid\'s mode changed under the console (40x30: its next write refused once, the size looked at, the windows resized and the screen painted again; 80x30), then both terminals (the smaller: the serial port\'s 80 x 24)',
      init: 't_rc', cycles: 150e6, pc: { files: { 'vt/mode.rc': VT_MODE_RC } },
      get machine() { return { vera: true, input: typed([['rc /pc/vt/mode.rc']]) }; },
      expect: ['size 80 57\nsize 40 27\nsize 80 27\nsize 80 24\n', '\ndone\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), a = out.indexOf('[t2]\n'), b = out.indexOf('[/t2]', a);
        if (a < 0 || b < 0) return ['no screen read at 40x30'];
        const rows = out.slice(a + 5, b).replace(/\n$/, '').split('\n');
        if (rows.length !== 30 || rows.some(r => r.length !== 40)) f.push('the screen at 40x30 read as ' + rows.length + ' rows of ' + [...new Set(rows.map(r => r.length))].join(', ') + ' columns');
        if (!rows.some(r => r.startsWith('after 40x30'))) f.push('the screen at 40x30: not painted again (no "after 40x30" on it)');
        return f;
      },
    },
    {
      name: 'vtedit', what: 'the line editor at the window\'s width (W3): rc\'s lines typed in a window 20 columns wide (terminal size 20 24), each longer than a row: Left back across the rows, characters inserted, Home, Right, Delete, End; then that line again (Up), cut back with Backspace across the rows; a third made 30 columns wide as it\'s typed (the terminal\'s report among the keys: the line drawn again), then edited: what rc read, and what the serial port\'s terminal shows of each line (sim/lib/vt.js, 20 and 30 columns), as typed, the rest of its last row blank',
      init: 't_rc', cycles: 150e6,
      get machine() { return { input: typed([['echo terminal size 20 24 >/dev/consctl'], [VT_EDIT[0]], [VT_EDIT[1]], [VT_EDIT[2]], ['echo terminal size 80 24 >/dev/consctl'], ['echo done']]) }; },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], raw = m.out, out = raw.replace(/\r/g, ''), hist = [];
        const lines = VT_EDIT.map(k => { const l = edSim(k, hist); hist.push(l); return l; });
        lines.forEach((l, i) => { if (!out.includes('\n' + l.slice(5) + '\n')) f.push('line ' + (i + 1) + ': rc didn\'t echo ' + JSON.stringify(l.slice(5))); });
        const clear = '\x1b[0m\x1b(B\x1b)B', a = raw.indexOf(clear, raw.indexOf('size 20 24')), b = raw.indexOf(clear, a + 1), c = raw.indexOf(clear, b + 1);
        if (a < 0 || b < 0 || c < 0) return f.concat(['no paints at 20 columns, then 30, then 80']);
        const shownAt = (seg, cols, start, l, i) => {             // (The line on the terminal, and the rest of its last row)
          const rows = new VT({ cols, rows: 24 }).write(seg).lines().map(r => r.padEnd(cols)), k = rows.findIndex(r => r.startsWith(start));
          const n = Math.ceil((l.length + 2) / cols) * cols, shown = k < 0 ? '' : rows.slice(k).join('').slice(0, n);
          if (shown !== ('% ' + l).padEnd(n)) f.push('line ' + (i + 1) + ' on the terminal (' + cols + ' columns): ' + JSON.stringify(shown) + ', not ' + JSON.stringify(('% ' + l).padEnd(n)));
        };
        const seg20 = raw.slice(a, b), mid = seg20.indexOf(lines[0].slice(5) + '\r\n');
        shownAt(seg20, 20, '% echo 01', lines[0], 0);
        shownAt(seg20.slice(mid), 20, '% echo 01', lines[1], 1);
        shownAt(raw.slice(b, c), 30, '% echo abc', lines[2], 2);
        this.notes = lines.map((l, i) => 'line ' + (i + 1) + ': ' + l);
        return f;
      },
    },
    {
      name: 'winsize', what: 'a window\'s size in each language (W3): terminal size 100 30, then rc (consctl\'s size line), HyForth (form, k-resize), hylang (cons.hl\'s window-size), C (conio: the keys sample\'s screensize, and CH_RESIZE as the terminal\'s report is typed, 120 x 40); the editor drawn again as its window changes (its help lines on the new last rows)',
      init: 't_rc', cycles: 120e6,
      get machine() {
        return { input: 'āecho terminal size 100 30 >/dev/consctl; grep size /dev/consctl\r' +
          'āforth\rĀĀ' + 'lib facility form . . k-resize .\rĀ' + 'bye\r' +
          'āhylang\rĀĀ' + '(use "cons")\r' + 'ā(window-size)\r' + 'ā(exit)\r' +
          'ā/sd/0/sample/c/keys\rĀĀ' + '\x1b[8;40;120t' + 'Ā' + 'q' + 'āgrep size /dev/consctl\r' +
          'āedit /ram/w\rĀĀ' + '\x1b[8;30;100t' + 'ĀĀ' + '\x18' + 'āecho terminal size 80 24 >/dev/consctl\r' + 'āecho done\r' };
      },
      expect: ['/dev/consctl\nsize 100 30\n%', 'k-resize .\n100 30 150  ok', '(window-size)\n=> {100 30}', 'keys: a 100x30 screen', ' 96\nended at',
        '% grep size /dev/consctl\nsize 120 40\n%', '\ndone\n%'],
      check(m) {
        const out = m.out, a = out.indexOf('edit /ram/w'), b = out.indexOf('echo terminal size 80 24', a);
        if (a < 0 || b < 0) return ['the editor: no output'];
        const ed = out.slice(a, b), r = ed.indexOf('\x1b[0m\x1b(B\x1b)B');            // (The resize's paint)
        return ed.lastIndexOf('\x1b[30;1H') > r && r > 0 ? [] : ['the editor: not drawn again at 100 x 30 (no help line on row 30 after the resize\'s paint)'];
      },
      sdcard: true
    },
    {
      name: 'winchrome', what: 'the chrome on the screen (W4): its label (#c0/label, OSC 2, empty: its program\'s name), its status line (wctl\'s status, and DECSASD\'s, after DECSSDT 2), the header\'s and footer\'s formats (%p, %l, %n, %s, %c, %r, %m, %[7], %=), the bar (its defaults: the windows, the time; at the bottom; off), a window\'s chrome rows turned off (its size grows by each), activity in a window not shown (monitor on: +; a bell: !); read back from vid\'s screen',
      init: 't_rc', cycles: 260e6,
      pc: { files: { 'vt/chrome.rc': CHROME_RC, 'vt/sasd': '\x1b[2$~\x1b[1$}\x1b[2Kfrom vt\x1b[0$}', 'vt/title': '\x1b]2;titled\x07', 'vt/bel': '\x07' } },
      get machine() { return { vera: true, input: typed([['rc /pc/vt/chrome.rc']]) }; },
      expect: ['\ndone\n%'],
      check(m) {
        const f = [], out = m.out.replace(/\r/g, ''), part = n => { const a = out.indexOf('[' + n + ']\n'); return a < 0 ? [] : out.slice(a + n.length + 3).split('\n'); };
        const row = (l, at, want) => { if (!want.test(l[at] || '')) f.push(at + ': ' + JSON.stringify(l[at]) + ' isn\'t ' + want); };
        const l = part('l'), z = part('z');
        if (l[0] !== 'mywin' || l[1] !== 'titled') f.push('the label read back: ' + JSON.stringify(l.slice(0, 2)) + ', not mywin, titled');
        if (z.slice(0, 4).join('|') !== 'size 80 58|size 80 59|size 80 60|size 80 57') f.push('the sizes as the chrome went: ' + JSON.stringify(z.slice(0, 4)));
        const s1 = part('s1'), s2 = part('s2'), s3 = part('s3'), s4 = part('s4'), s5 = part('s5'), s6 = part('s6');
        row(s1, 0, /^ 0 mywin +00:00$/); row(s1, 1, /^0 mywin +0 mywin$/); row(s1, 2, /^hello there *$/);
        row(s2, 1, /^\[rc\] mywin +0$/); row(s2, 2, /^from vt +80 x 57 cooked$/);
        row(s3, 0, /^\[rc\] mywin +0$/); row(s3, 1, /^from vt +80 x 57 cooked$/); row(s3, 2, /^ 0 mywin +00:00$/);
        row(s4, 0, /^ 0\+ titled +00:00$/); row(s5, 0, /^ 0! titled +00:00$/); row(s6, 0, /^ 0 rc +00:00$/);
        return f.map(x => 'winchrome: ' + x);
      },
    },
    {
      name: 'winser', what: 'the chrome on the serial port (W4b: chrome serial on): its bar, header and footer drawn there, the window 80 x 21 below the bar and header (its margins sent offset); 25 lines scrolling the window\'s rows alone; a clear (ED 2) then the chrome drawn again; a CUP and DECSTBM from a program offset; chrome serial off (80 x 24 again): what the PC\'s terminal shows (sim/lib/vt.js)',
      init: 't_rc', cycles: 260e6,
      // (One script: a chrome redraw after a prompt (the status changed) would hide it from the harness's wait)
      pc: { files: { clr: '\x1b[2J\x1b[Hcleared\n\x1b[5;10Hat 5,10\x1b[2;4r\x1b[20;1H', 'ser.rc': ['echo chrome serial on >/dev/wctl', 'grep size /dev/consctl',
        'echo -n sertitle >/dev/label', 'echo status st1 >/dev/wctl', 'for (i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25) echo line $i',
        'cat /pc/clr', 'echo chrome serial off >/dev/wctl', 'grep size /dev/consctl', 'echo done', ''].join('\n') } },
      get machine() { return { input: typed([['rc /pc/ser.rc']]) }; },
      expect: ['size 80 21\n', 'size 80 24\n', '\ndone\n%'],
      check(m) {
        const f = [], raw = m.out, clear = '\x1b[0m\x1b(B\x1b)B';
        const a = raw.indexOf(clear, raw.indexOf('rc /pc/ser.rc')), b = raw.indexOf(clear, raw.indexOf('at 5,10'));
        if (a < 0 || b < 0) return ['no paints for chrome serial on and off'];
        const look = (upto, what, rows) => {                     // (The terminal's rows, as far as upto)
          const t = new VT({ cols: 80, rows: 24 }).write(raw.slice(a, upto)), l = t.lines();
          for (const [r, re] of rows) if (!re.test(l[r] || '')) f.push(what + ': row ' + (r + 1) + ' ' + JSON.stringify(l[r]) + ' isn\'t ' + re);
          return t;
        };
        const lines = raw.indexOf('\x1b[2J', raw.indexOf('line 25'));
        look(lines, 'after 25 lines', [[0, /^ 0 sertitle +00:00$/], [1, /^0 sertitle +0 sertitle$/], [2, /^line 6$/], [21, /^line 25$/], [22, /^$/], [23, /^st1$/]]);
        const t = look(b, 'after the clear', [[0, /^ 0 sertitle +00:00$/], [1, /^0 sertitle +0 sertitle$/], [2, /^cleared$/], [6, /^ +at 5,10$/], [23, /^st1$/]]);
        if (t.top !== 3 || t.bot !== 5) f.push('the program\'s region (2;4) on the terminal: rows ' + (t.top + 1) + '-' + (t.bot + 1) + ', not 4-6');
        return f;
      },
    },
    {
      name: 'winwords', what: 'the window\'s chrome in HyForth and hylang (W4c; C\'s are ctest\'s): window-label (read back from /dev/label; hylang\'s read too), window-status (the footer\'s %s, on the screen\'s footer row: both terminals on, the window 80 x 24 below the bar and header), window-ctl',
      init: 't_rc', cycles: 200e6,
      get machine() {
        return { vera: true, input: 'āforth\rĀĀ' + 'lib cons s" fth" window-label s" st-fth" window-status s" monitor off" window-ctl\rĀ' + 'bye\r' +
          'ācat /dev/label; echo; head -27 /dev/vid/term | tail -1\r' +
          'āhylang\rĀĀ' + '(use "cons")\r' + 'ā(window-label "hyl")\r' + 'ā(window-label)\r' + 'ā(window-status "st-hyl")\r' + 'ā(window-ctl "monitor off")\r' + 'ā(exit)\r' +
          'āhead -27 /dev/vid/term | tail -1; echo done\r' };
      },
      expect: ['monitor off" window-ctl\n ok', 'tail -1\nfth\nst-fth', '(window-label)\n=> "hyl"', '(window-ctl "monitor off")\n=> NIL', 'echo done\nst-hyl', '\ndone\n%'],
    },
    {
      name: 'newwin', what: 'new-window (W5c): a command run in a window made and shown, in this one\'s group ($window its number; the window gone when it ends, this one shown again), -g\'s in a group of its own, the shell (/lib/shell\'s: HyForth) with none; HyForth\'s new-window and new-group, hylang\'s',
      init: 't_rc', cycles: 200e6,
      // (Each window shown as it's made: its output on the terminal then, the window it came from painted again
      // when it goes, the prompt last.  The shell's window: a line typed there, then bye)
      get machine() {
        return { input: 'ānew-window \'echo hi from $window; sleep 1\'\r' + 'ānew-window -g sleep 2; cat /dev/wctl\r' +
          'ānew-window\r' + 'āecho shell in $window; cat /dev/wctl\r' + 'ābye\r' +
          'āforth -l\r' + 'ālib cons s" echo fth $window; sleep 1" new-window\r' + 'ās" echo fgrp; cat /dev/wctl; sleep 1" new-group\r' + 'ābye\r' +
          'āhylang\r' + 'ā(use "cons")\r' + 'ā(new-window "echo hyl $window; sleep 1")\r' + 'ā(new-group "echo hgrp; cat /dev/wctl; sleep 1")\r' + 'ā(exit)\r' +
          'āecho done\r' };
      },
      expect: ['hi from 1\n', 'cat /dev/wctl\n0 0 80 24\n1 1 80 24 *\n', 'shell in $window; cat /dev/wctl\nshell in 1\n0 0 80 24\n1 0 80 24 *\n',
        'fth 1', 'fgrp\n0 0 80 24\n1 1 80 24 *', 'hyl 1', 'hgrp\n0 0 80 24\n1 1 80 24 *', 'echo done\ndone\n%'],
    },
    {
      name: 'winkeys', what: 'the windows\' keys (W5d): wctl\'s key lines (the prefix Ctrl-A, keys after it, Ctrl-Tab\'s action; a digit, an unknown action, Ctrl-C as the prefix refused); Ctrl-] w\'s list (a line a window: its key, number, label and group; chosen with the arrows and Enter, a key, q, Escape twice; the window shown each time it opens marked; activity, +, as monitor on and off say); Ctrl-Tab and Ctrl-Shift-Tab as Windows Terminal\'s win32-input-mode sends them, made into CSI u\'s by sim/lib/win32in.js (the PC tool\'s --win32-input); keys mods (HyForth\'s ekey: Ctrl-Up, Up, Shift-F2, the k- masks or\'d in)',
      init: 't_rc', cycles: 200e6,
      // (Windows 0, 1 in its group, 2 in a group of its own, each held by a sleep; monitor on in 1, on then off in 2,
      // each written to: the first list marks 1's activity, +, not 2's.  The lists, in order, with the window shown before each
      // marked: 0; 2 (the first's down, down, Enter); 1 (its 0, then Ctrl-Tab); 0 (its q, then Ctrl-Shift-Tab); 2
      // (its Escape twice, then Ctrl-Tab bound to next-group; the list by the key g, its 1, then Ctrl-A 0)
      get machine() {
        const w32 = s => { const o = []; const d = createWin32Input(b => o.push(...b)); for (const c of Buffer.from(s, 'latin1')) d.push(c); d.flush(); return String.fromCharCode(...o); };
        const rec = (vk, uc, cs) => '\x1b[' + vk + ';15;' + uc + ';1;' + cs + ';1_';
        const CTAB = w32(rec(9, 9, 8)), CSTAB = w32(rec(9, 9, 0x18)), L = '\u0100', W = '\x01w' + L;
        return { input: 'āecho b115200 >/dev/serctl\r' + 'āecho new >/dev/wctl; echo new group >/dev/wctl\r' + 'āecho key prefix ctrl-a >/dev/wctl\r' +
          'āsleep 60 >\'#c1/cons\' &\r' + 'āsleep 60 >\'#c2/cons\' &\r' + 'āecho monitor on >\'#c1/wctl\'\r' +
          'āecho monitor on >\'#c2/wctl\'\r' + 'āecho monitor off >\'#c2/wctl\'\r' + 'āecho x >\'#c1/cons\'; echo x >\'#c2/cons\'\r' +
          'āecho key 5 list >/dev/wctl\r' + 'āecho key z bogus >/dev/wctl\r' + 'āecho key prefix ctrl-c >/dev/wctl\r' +
          'ā' + W + '\x1b[B' + L + '\x1b[B' + L + '\r' + L + W + '0' +
          'ā' + CTAB + L + W + 'q' + L + CSTAB + L + W + '\x1b\x1b' +
          'āecho key ctrl-tab next-group >/dev/wctl\r' + 'āecho key g list >/dev/wctl\r' + 'ā' + CTAB + L + '\x01g' + L + '1' + L + '\x010' +
          'ācat /dev/wctl\r' + 'āforth\r' + L + L + 'require facility.fl\r' + L +
          ': t s" /dev/consctl" w/o open-file throw >r s" keys mods" r@ write-file throw\r' + L +
          '  ekey . ekey . ekey . ekey . r> close-file throw ;\r' + L + 't\r' + L + '\x1b[1;5A' + L + '\x1b[A' + L + '\x1b[1;2Q' + L + 'x' + L +
          'bye\r' + 'āecho done\r' };
      },
      expect: ['5 list >/dev/wctl\necho: write error: invalid argument', 'z bogus >/dev/wctl\necho: write error: invalid argument',
        'prefix ctrl-c >/dev/wctl\necho: write error: invalid argument', '> 0  0 rc  (group 0)', '  1  1+   (group 0)', '  2  2   (group 1)',
        'cat /dev/wctl\n0 0 80 24 *\n1 0 80 24\n2 1 80 24\n', '640 128 395 120  ok', 'echo done\ndone\n%'],
      check(m) {
        const out = m.out.replace(/\r/g, ''), marked = [];
        for (const part of out.split('A window\'s key, or the arrows').slice(1)) { const k = part.match(/> ([0-9a-f])  \d/); marked.push(k ? k[1] : '?'); }
        return marked.join(' ') === '0 2 1 0 2' ? [] : ['the lists marked ' + marked.join(' ') + ', not 0 2 1 0 2'];
      },
    },
    {
      name: 'snarf', what: '/dev/snarf (W6a): written and read back; a file through it, the same (cmp); 8K at most (past them, disk full); Ctrl-] y\'s paste into rc as its keys (two lines: each LF a CR); hylang\'s snarf! and snarf; bracketed paste (?2004) to a keys vt reader, HyForth\'s ekey: CSI 200 ~, the text, CSI 201 ~',
      init: 't_rc', cycles: 120e6,
      get machine() {
        const L = '\u0100';
        return { input: 'āecho hello snarf >/dev/snarf; cat /dev/snarf\r' +
          'ācat /rom/lib/windows >/dev/snarf; cmp /rom/lib/windows /dev/snarf; echo cmp $status\r' +
          'ācat /rom/doc/api.md >/dev/snarf; wc -c /dev/snarf\r' + 'ā{echo echo one; echo echo two} >/dev/snarf\r' + 'ā\x1dy' +
          'āhylang\r' + 'ā(use "cons")\r' + 'ā(snarf! "from hylang")\r' + 'ā(snarf)\r' + 'ā(exit)\r' +
          'āecho -n ab >/dev/snarf\r' + 'āforth\r' + L + L + 'require facility.fl\r' + L +
          ': t s" /dev/consctl" w/o open-file throw >r s" keys vt" r@ write-file throw\r' + L +
          '  27 emit ." [?2004h" 14 0 do ekey . loop r> close-file throw ;\r' + L + 't\r' + L + '\x1dy' + L + L + 'bye\r' +
          'āecho done\r' };
      },
      expect: ['cat /dev/snarf\nhello snarf\n', 'echo cmp $status\ncmp\n', 'cat: write error: disk full\n   8192 /dev/snarf\n',
        '% echo one\none\n% echo two\ntwo\n', '(snarf)\n=> "from hylang"', '27 91 50 48 48 126 97 98 27 91 50 48 49 126  ok',
        'echo done\ndone\n%'],
    },
    {
      name: 'scrollview', what: 'the scrollback\'s view (W6b): Ctrl-] [ (its end), PgUp, the arrows, Space\'s mark and Enter\'s copy (the lines marked to the cursor\'s, into /dev/snarf); Enter alone (the cursor\'s line); Escape twice; Shift-PgUp (a page up) while a program writes on (its output there after: the view left with q), Home and End; its footer\'s %y on the serial port (default chrome serial on)',
      init: 't_rc', cycles: 200e6,
      // (30 lines and the command's two rows: the view a page up from its end, its top the command's first row; up
      // from the last row (line 22) to line 21, marked, down twice (the top a line down): lines 21-23 copied.  Then
      // the view at the end, up twice: the line two above the prompt's, line 22, copied alone)
      get machine() {
        const L = '\u0100', UP = '\x1b[A', DOWN = '\x1b[B', PGUP = '\x1b[5~';
        return { input: 'āfor(i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30) echo line $i\r' +
          'ā\x1d[' + L + PGUP + L + UP + L + ' ' + L + DOWN + L + DOWN + L + '\r' + 'ācat /dev/snarf\r' +
          'ā\x1d[' + L + UP + L + UP + L + '\r' + 'ācat /dev/snarf\r' + 'ā\x1d[' + L + '\x1b\x1b' +
          'āsleep 1; echo after the view\r' + '\x1b[5;2~' + L + '\x1b[H' + L + '\x1b[F' + L + L + 'q' +
          'āecho default chrome serial on >/dev/wctl\r' + 'ā\x1b[5;2~' + L + 'q' + 'āecho done\r' };
      },
      expect: ['% cat /dev/snarf\nline 21\nline 22\nline 23\n%', '% cat /dev/snarf\nline 22\n%', 'after the view\n%',
        ' 1-21/', '  Space: mark, Enter: copy, q: leave', 'echo done\ndone\n%'],
    },
    {
      name: 'history', what: 'a window\'s history (W6c: wctl\'s history N): 64 rows past its scrollback\'s, a hundred lines and more kept (/dev/text has them all: 105 lines, 64 without), the view\'s Home the oldest of them (copied into /dev/snarf, then a page down and back); history 0 (its rows gone: 64 lines again); history 200 refused',
      init: 't_rc', cycles: 200e6,
      get machine() {
        const L = '\u0100';
        return { input: 'āecho history 64 >/dev/wctl\r' + 'āfor(i in 1 2 3 4 5 6 7 8 9 10) for(j in 0 1 2 3 4 5 6 7 8 9) echo n $i$j\r' +
          'āgrep -c n /dev/text; wc -l /dev/text\r' + 'ā\x1d[' + L + '\x1b[H' + L + '\x1b[6~' + L + '\x1b[5~' + L + '\r' + 'ācat /dev/snarf\r' +
          'āecho history 0 >/dev/wctl; grep -c n /dev/text; wc -l /dev/text\r' + 'āecho history 200 >/dev/wctl\r' + 'āecho done\r' };
      },
      expect: ['wc -l /dev/text\n102\n    105 /dev/text\n', 'cat /dev/snarf\n% echo history 64 >/dev/wctl\n%',
        'wc -l /dev/text\n60\n     64 /dev/text\n', 'history 200 >/dev/wctl\necho: write error: invalid argument', 'echo done\ndone\n%'],
    },
    {
      name: 'tiles', what: 'tiles (W7a: wctl\'s layout): three windows of a group in rows (80 x 7 each on the serial port\'s 80 x 24, a header row each), columns (26 x 23, a border between), a grid (40 x 11, 39 x 11, the third 80 x 11), zoomed and back (Ctrl-] z: the focus alone, 80 x 24, the others keeping theirs), tabs; then rows again, the focus moved by Ctrl-] and the arrows and Ctrl-] Shift-Tab (back to window 0); the screen at the end: the three tiles, their headers (the focused one\'s reversed), each window\'s text in its tile',
      init: 't_rc', cycles: 300e6,
      // (Windows 1 and 2 held open by sleeps, a line written to each.  The sizes read from each window's consctl.  The
      // waits are a time's, not the prompt's: a tile painted again ends with its blanks)
      get machine() {
        const L = '\u0100', W = L + L + L;
        const size = 'grep size /dev/consctl; grep size \'#c1/consctl\'; grep size \'#c2/consctl\'\r';
        return { input: 'āecho b115200 >/dev/serctl\r' + 'āecho new >/dev/wctl; echo new >/dev/wctl\r' + 'āsleep 1000 >\'#c1/cons\' &\r' + 'āsleep 1000 >\'#c2/cons\' &\r' +
          'āecho one >\'#c1/cons\'; echo two >\'#c2/cons\'\r' + 'āecho layout rows >/dev/wctl\r' + W + size + W +
          'echo layout columns >/dev/wctl\r' + W + size + W + 'echo layout grid >/dev/wctl\r' + W + size + W +
          '\x1dz' + W + 'grep size /dev/consctl\r' + W + '\x1dz' + W + 'echo layout tabs >/dev/wctl\r' + W + 'grep size /dev/consctl\r' + W +
          'echo layout rows >/dev/wctl\r' + W + '\x1d\x1b[B' + W + '\x1d\x1b[Z' + W + '\x1d\x1b[C' + W + '\x1d\x1b[D' + W +
          'echo done\r' + W };
      },
      expect: ['size 80 7\n', 'size 26 23', 'size 40 11', 'size 39 11', 'size 80 11', 'done\n%'],
      check(m) {
        const f = [], out = m.out;
        const sizes = [...new Set(out.replace(/\x1b\[[0-9;]*[A-Za-z]/g, '\n').match(/size \d+ \d+/g) || [])];
        const want = ['size 80 7', 'size 26 23', 'size 40 11', 'size 39 11', 'size 80 11', 'size 80 24'];
        if (sizes.join(',') !== want.join(',')) f.push('the sizes read (each the first time): ' + sizes.join(', ') + '; not ' + want.join(', '));
        const t = new VT({ cols: 80, rows: 24 }).write(out);
        const lines = t.lines();
        const rev = r => t.screen[r][0].f & 16;
        if (!/^0 rc/.test(lines[0]) || !/^1/.test(lines[8]) || !/^2/.test(lines[16])) f.push('the headers at rows 1, 9, 17: ' + [lines[0], lines[8], lines[16]].join(' | '));
        if (!rev(0) || rev(8) || rev(16)) f.push('the focused tile\'s header (window 0\'s) reversed, the others not');
        if (!lines.slice(9, 16).some(l => l.startsWith('one'))) f.push('window 1\'s line (one) in its tile, rows 10-16');
        if (!lines.slice(17, 24).some(l => l.startsWith('two'))) f.push('window 2\'s line (two) in its tile, rows 18-24');
        if (!lines.slice(1, 8).some(l => l === 'done')) f.push('window 0\'s last line (done) in its tile, rows 2-8');
        return f;
      },
    },
    {
      name: 'tilesplit', what: 'splits (W7a): Ctrl-] s, a window with a shell (init\'s, HyForth) below in the group, rows; a Forth line there; Ctrl-] Up back to window 0, its wctl (two windows, each 80 x 11: the serial port\'s tiles, the smaller); Ctrl-] v, a third; the Vera X\'s screen read back: the bar, then window 0\'s tile\'s header',
      init: 'init', cycles: 600e6,
      get machine() {
        const L = '\u0100', W = L + L + L + L + L;
        return { vera: true, input: 'ā\x1ds' + W + W + '2 3 + .\r' + W + '\x1d\x1b[A' + W + 'cat /dev/wctl\r' + W + '\x1dv' + W + W +
          '\x1d\x1b[A\x1d\x1b[A' + W + 'head -2 /dev/vid/term | tail -1\r' + W };
      },
      expect: ['2 3 + .\n5 ', 'cat /dev/wctl\n0 0 80 11 *\n1 0 80 11\n', 'tail -1\n0 forth'],
    },
    {
      name: 'popups', what: 'popups (W7b: wctl\'s float X Y C R): window 1 floating at 20, 6 (30 x 8: its consctl\'s size), boxed, shown over window 0 as twelve lines are written there (painted around the box); Ctrl-] 0 and window 0 alone again; Ctrl-] ? (the keys, a popup: the bindings) and q; then the popup over the group\'s two tiles (rows: windows 0 and 2; window 1, floating, no tile), the screen at the end checked: the tiles\' headers, the box, the text in each',
      init: 't_rc', cycles: 300e6,
      get machine() {
        const L = '\u0100', W = L + L + L;
        return { input: 'āecho b115200 >/dev/serctl\r' + 'āecho new >/dev/wctl\r' + 'āsleep 1000 >\'#c1/cons\' &\r' +
          'āecho float 20 6 30 8 >\'#c1/wctl\'; echo popup text >\'#c1/cons\'\r' + 'āgrep size \'#c1/consctl\'\r' +
          'ā{sleep 2; for(i in 1 2 3 4 5 6 7 8 9 10 11 12) echo under the popup, a long line, $i} &\r' +
          'āecho current 1 >/dev/wctl\r' + W + W + W + '\x1d0' + W + 'echo keys\r' + W + '\x1d?' + W + 'q' + W +
          'echo new >/dev/wctl\r' + W + 'sleep 1000 >\'#c2/cons\' &\r' + W + 'echo two >\'#c2/cons\'; echo layout rows >/dev/wctl\r' + W +
          '{sleep 3; echo fin^ish >\'#c1/cons\'} &\r' + W + 'echo current 1 >/dev/wctl\r' + W + W + W };
      },
      expect: ['size 30 8', ' w', 'the windows', ' ?', 'these keys', 'finish'],
      check(m) {
        const f = [];
        const t = new VT({ cols: 80, rows: 24 }).write(m.out);
        const lines = t.lines();
        if (!/^0 rc/.test(lines[0]) || !/^2/.test(lines[12])) f.push('the tiles\' headers at rows 1 and 13: ' + lines[0] + ' | ' + lines[12]);
        if (lines[5].slice(19, 21) !== '+-' || lines[5][50] !== '+' || lines[14][19] !== '+' || lines[14][50] !== '+')
          f.push('the box from row 6, column 20 to row 15, column 51: ' + lines[5] + ' | ' + lines[14]);
        if (!lines[6].slice(20).startsWith('popup text') || lines[6][19] !== '|') f.push('the popup\'s text in its box: ' + lines[6]);
        if (!lines[7].slice(20).startsWith('finish')) f.push('the line written into it last: ' + lines[7]);
        if (!lines.slice(13, 24).some(l => l.startsWith('two'))) f.push('window 2\'s text in its tile, rows 14-24');
        return f;
      },
    },
    {
      name: 'seats', what: 'seats (W8a: consctl\'s seats): the screen and the serial port each a seat; Ctrl-] c at the keyboard, a group of its own (init\'s shell) shown on the screen alone, sized to it, the serial port still showing window 0 (both marked in wctl); the keyboard\'s keys to it (its text read back from the serial port), its Ctrl-C a note to its shell, not window 0\'s; consctl reads terminal seats; both, one seat again (the screen showing window 0)',
      init: 'init', cycles: 400e6,
      get machine() {
        const P = '\u0100', W = P + P + P, CRB = '\u033A\u021C\u03BA';   // (Ctrl down, ], Ctrl up: Ctrl-] at the keyboard)
        return { vera: true, smc: true, input: 'āecho seats >/dev/consctl\r' + 'āgrep terminal /dev/consctl\r' + 'ā' +
          '\u0102' + CRB + 'c' + W + W + W + W + '.( on the scr^een) cr\r' + W + ': spin begin again ;\r' + W + 'spin\r' + W + '\x03' + W + '\u0103' +
          W + 'cat /dev/wctl\r' + 'ācat \'#c1/text\'\r' + 'āhead -2 /dev/vid/term | tail -1\r' + 'āecho both >/dev/consctl\r' + 'ācat /dev/wctl\r' +
          'āhead -3 /dev/vid/term\r' + 'ā' };
      },
      expect: ['terminal seats', 'cat /dev/wctl\n0 0 80 24 *\n1 1 80 57 *\n', 'on the scr^een\n', 'spin\ninterrupt\n', 'tail -1\n1 forth',
        'cat /dev/wctl\n0 0 80 24 *\n1 1 80 24\n', 'head -3 /dev/vid/term\n 0 forth 1 forth', '\n0 forth'],
      check(m) {                                              // (The serial port not painted again till both: its window
        const o = m.out, a = o.indexOf('terminal seats'), b = o.indexOf('echo both');   //   the same throughout)
        return a < 0 || b < 0 || o.slice(a, b).includes('\x1b[2J') ? ['the serial port cleared while the keyboard\'s seat had its window'] : [];
      },
    },
    {
      name: 'winmouse', what: 'the mouse in the windows (W8b): the input controller\'s reports (kdev\'s) (CSI < B ; X ; Y M and m) to a program that asks for them (?1000, ?1006: HyForth\'s ekey, keys vt), at its own cell; a click in a tile not focused (Ctrl-] s\'s rows: window 0\'s, above) focuses it; the wheel over a window that doesn\'t ask, its scrollback\'s view (Enter: its cursor\'s line into /dev/snarf, the view left); the PC terminal\'s reports from the serial port (the console\'s ?1000 and ?1006 sent it as its window asks), its tile\'s cell (row 3, its header row 1: the window\'s 2)',
      init: 'init', cycles: 600e6,
      get machine() {
        const P = '\u0100', W = P + P + P, M = '\u0400';
        return { vera: true, smc: { moves: [[0, -160, 0], [0, 0, 1], [0, 0, 0], [0, 0, 1], [0, 0, 0], [0, 0, 0, -1]] },
          input: 'ārequire facility.fl\r' + 'ā: t s" /dev/consctl" w/o open-file throw >r s" keys vt" r@ write-file throw\r' +
            W + '  27 emit ." [?1000h" 27 emit ." [?1006h" 0 do ekey . loop\r' + W + '  27 emit ." [?1000l" r> close-file throw ;\r' +
            W + '20 t\r' + W + '\u0102' + M + P + M + P + M + P + '\u0103' + W + '\x1ds' + W + W + W + W + 'cat /dev/wctl\r' + W +
            '\u0102' + M + P + M + P + '\u0103' + W + 'cat /dev/wctl\r' + W + '\u0102' + M + P + '\u0103' + W + '\r' + W +
            'cat /dev/snarf\r' + W + '18 t\r' + W + '\x1b[<0;5;3M\x1b[<0;5;3m' + W };
      },
      // (The pointer from the screen's middle up 160: its row 10, the window's 9 below the bar and header; its column 41)
      expect: ['/> 20 t\n\x1b[?1000;1006h27 91 60 48 59 52 49 59 57 77 27 91 60 48 59 52 49 59 57 109 \x1b[?1000l\n',
        'cat /dev/wctl\n0 0 80 11\n1 0 80 11 *\n', 'cat /dev/wctl\n0 0 80 11 *\n1 0 80 11\n',
        '18 t\n\x1b[?1000;1006h', '27 91 60 48 59 53 59 50 77 27 91 60 48 59 53 59 50 109 \x1b[?1000l'],
      // (/dev/snarf's line: the boot's task table's cons line (its state 08: waiting for its own events, its
      // init's loop), its CPU column the boot's own, then the prompt)
      check(m) {
        const out = m.out.replace(/\r/g, ''), line = (out.match(/^F 08 01 FF [0-9A-F]{6} cons$/m) || [])[0];
        return line && out.includes(line + '\n/> ') ? [] : ['cat /dev/snarf: not the task table\'s cons line' + (line ? ' (' + line + ')' : '')];
      },
    },
    {
      name: 'mouse', what: 'the mouse (VIDEO.md step 6): vid\'s /mouse, /mousein and /mousectl (its state; /mouse\'s first read at once, Plan 9\'s 49 bytes; a non-blocking read\'s E_AGAIN; moves, the pointer\'s sprite at them, kept on the screen; swap; the buttons\' changes queued and read in turn; the pointer off and on; a write to /mouse; bad lines; a claim and its release; mode 40x30\'s size), then the input controller (kdev\'s, on the emulator\'s SMC): a move, a click and the wheel from its PS/2 packets',
      init: 't_mouse', cycles: 45e6,
      // (The SMC's mouse: a move, then the left button pressed and let go, then the wheel up, each 2M cycles apart,
      // from 12M cycles on: by then kdev has asked for the mouse's mode and read it)
      machine: { vera: true, smc: { moves: [[30, 20, 0], [0, 0, 1], [0, 0, 0], [0, 0, 0, -1]] },
        input: '\u0102' + '\u0100'.repeat(6) + '\u0400\u0100\u0400\u0100\u0400\u0100\u0400\u0103' },
      check(m) {
        const s = m.smc, f = [];
        if (s.mouseId !== 3) f.push('the SMC\'s mouse in mode ' + s.mouseId + ' (3, a wheel\'s, asked for)');
        if (s.mouseLost) f.push(s.mouseLost + ' packets lost on the SMC');
        this.notes = ['the SMC: ' + s.reads + ' reads, ' + s.nacks + ' with nothing (unanswered); the I2C bus: ' + m.i2c.stats.starts + ' starts'];
        return f;
      },
    },
    {
      name: 'kbd', what: 'the keyboard (VIDEO.md step 6): the input controller (kdev\'s), the emulator\'s SMC typed at, the keys into the console (#c/kbin) as a PC terminal sends them, at the login shell (HyForth): Shift and punctuation; Left to edit a line; Up, its history; Caps Lock; the keypad\'s digits, and its cursor keys with Num Lock off; Ctrl-C, a note to the window\'s shell; the keyboard\'s LEDs following the locks',
      init: 'init', cycles: 120e6,
      get machine() {
        const tap = n => String.fromCharCode(0x200 + n), W = '\u0101';  // (smc.js's key n pressed and let go; a prompt)
        const LEFT = tap(79), UP = tap(83), CAPS = tap(30), NUM = tap(90), KP1 = tap(93), KP2 = tap(98), KP4 = tap(92);
        return { vera: true, smc: true, input: '\u0102' + W + '.( Hello, World!) cr\r' + W + '.( ac)' + LEFT + LEFT + 'b\r' + W + UP + '\r' +
          W + CAPS + '.( shout)' + CAPS + '\r' + W + '.( ' + KP1 + KP2 + ')\r' + W + '.( xz)' + NUM + KP4 + KP4 + 'y' + NUM + '\r' +
          W + ': spin begin again ;\r' + W + 'spin\r\u0100\x03' + W + '.( back)\r' + '\u0103' };
      },
      expect: ['/> .( Hello, World!) cr\nHello, World!\n/> ', '\nabc\n/> .( abc)\nabc\n/> .( SHOUT)\nSHOUT\n/> .( 12)\n12\n/> .( xz)',
        '\nxyz\n/> : spin begin again ;\n/> spin\ninterrupt\n/> .( back)\nback\n/> '],
      check(m) {
        const s = m.smc, f = [], leds = s.commands.filter(c => c[0] === 0xED).map(c => c[1]).join(' ');
        if (leds !== '2 6 2 0 2') f.push('the LEDs: ' + leds + ' (2 6 2 0 2 wanted: Num Lock, Caps Lock on and off, Num Lock off and on)');
        if (s.lost) f.push(s.lost + ' key codes lost on the SMC');
        if (s.keys.length) f.push(s.keys.length + ' key codes left unread');
        return f;
      },
    },
    {
      name: 'draw', what: 'the graphics words (VIDEO.md step 5): vid\'s /dev/vid/draw (pen, plot, line, box, bar, circle, disc, clear) on the bitmap at 8, 4, 2 and 1 bits a pixel (640 across), what falls off it, bad numbers, a bitmap too big (the console on the serial port alone meanwhile: a bitmap 320 across makes the screen\'s text 40 columns, and the windows with it); HyForth\'s lib video (the drawing, vpeek, the turtle, text, the palette, a sprite) and rc\'s lines to the file; hylang\'s video.hl (the drawing, vpeek, the turtle, text); the C SDK\'s shapes sample (cc65\'s TGI on hydra_tgi: a line, a bar, a circle, an ellipse and text, read back) and sketch (vera.h, drawing after the emulator\'s mouse till a key)',
      init: 'init', cycles: 260e6,
      // (The mouse for sketch: to the top left, onto the strip's colour 2 and clicked, then to (150, 120), pressed,
      // dragged 20 right and 20 down, let go; then a key, x, ends it, and a while after it (not a prompt: rc's may come
      // out before the keyboard's input has ended, as the test runner sees it), echo $status)
      get machine() {
        const W = '\u0101', M = '\u0400', P = '\u0100';
        return { vera: true, smc: { moves: [[-400, -400, 0], [50, 5, 0], [0, 0, 1], [0, 0, 0], [100, 115, 0], [0, 0, 1], [20, 0, 1], [0, 20, 1], [0, 0, 0]] },
          input: DRAW_FORTH.map(l => W + l[0] + '\r').join('') + W + 'hylang\r' + DRAW_HY.map(l => W + l[0] + '\r').join('') + W + 'exit\r' +
            W + '/sd/0/sample/c/shapes\r' + W + '/sd/0/sample/c/sketch\r' + '\u0102' + P + P + (M + P).repeat(9) + 'x\u0103' + P + 'echo $status\r' };
      },
      get expect() {
        return [DRAW_FORTH.map(l => '/> ' + l[0] + '\n' + (l[1] ? l[1] + '\n' : '')).join('') + '/> hylang\n',
          DRAW_HY.map(l => 'hylang> ' + l[0] + '\n=> ' + l[1] + '\n').join(''), '/> /sd/0/sample/c/shapes\n320x240, 256 colours: 4 4 2 14 11, text 165 dots\n/> /sd/0/sample/c/sketch\n/> echo $status\n0\n'];
      },
      check(m) {
        const v = m.vera.vram, f = [];
        if (v[0x1FA00 + 400] !== 0 || v[0x1FA00 + 401] !== 15) f.push('palette entry 200 isn\'t $F00 (red)');
        const sp = Array.from(v.subarray(0x1FC00 + 18, 0x1FC00 + 22)).join(' ');
        if (sp !== '100 0 50 0') f.push('sprite 2 at ' + sp + ' (100 0 50 0 wanted: 100, 50)');
        let picked = 0;                                       // (sketch's lines, in the strip's colour 2)
        for (let y = 10; y < 240; y++) for (let x = 0; x < 320; x++) if (v[y * 320 + x] === 2) picked++;
        if (picked < 40 || v[120 * 320 + 160] !== 2 || v[130 * 320 + 170] !== 2) f.push('sketch\'s drag: ' + picked + ' pixels in colour 2 (40 or more wanted, through (160, 120) and (170, 130))');
        if (m.smc.lost || m.smc.mouseLost) f.push('codes lost on the SMC');
        return f;
      },
      sdcard: true
    },
    {
      name: 'ramw', what: 'small writes (HydraFS on a RAM disk writes back only the part of a block a write changed; a card\'s block is kept back): a file of 70 writes, written over in its first block, across a block\'s end and at its end, on /ram, /sram and a card, read back; 100 writes of 16 bytes to /ram and to a card, a write\'s time; a card\'s block kept back (its file open: not on the card) and synced (on it)',
      init: 'init', cycles: 120e6,
      get machine() { this.card = ramwCard(); return { sd: [this.card], input: RAMW_LINES.map(l => '\u0101' + l + '\r').join('') + '\u0101' }; },
      expect: ['[ramw done.'],
      // (October 2026: 15,400, from 24,200 when a RAM disk wrote back the whole block)
      budgets: [{ what: 'a write of 16 bytes to /ram (HyForth\'s write-file, 100 of them: its request to the storage driver, HydraFS, the RAM disk)',
        from: '[rw A]', to: '[rw B]', minus: ['[b0 A]', '[b0 B]'], per: 100, max: 18000 },
        // (October 2026: 25,000 with a card's block kept back, from some 225,000 when each write wrote its block)
        { what: 'a write of 16 bytes to a card (the same: its block kept back, written as the next is wanted)',
          from: '[cw A]', to: '[cw B]', minus: ['[b0 A]', '[b0 B]'], per: 100, max: 40000 }],
      check(m, out) {
        const f = [], n = out.split(RAMW_TEXT).length - 1;
        if (n !== 3) f.push('the file as written, read back ' + n + ' times (3 wanted: /ram, /sram, the card)');
        for (const p of ['/ram/n', '/sd/0/n']) if (!new RegExp('-rw\\S*\\s.*\\b1600\\b.*' + p).test(out.replace(/\r/g, ''))) f.push(p + ' isn\'t 1600 bytes long');
        // (A write past a file's end: the file that long, with a hole.  A RAM disk's clusters aren't a card's 4K: its
        // file came out too long, and the write failed, end of file)
        for (const p of ['/ram/h', '/sram/h', '/sd/0/h']) if (!new RegExp('-rw\\S*\\s.*\\b3001\\b.*' + p).test(out.replace(/\r/g, ''))) f.push(p + ', written at 3000 past its 2 bytes, isn\'t 3001 bytes long');
        this.card.save();
        const raw = fs.readFileSync(this.card.file, 'latin1');
        if (!raw.includes('then synced')) f.push('the card: a block synced, not on it');
        if (raw.includes('second part: kept back')) f.push('the card: a block kept back (its file open, not synced) on it already');
        const v = new hydrafs.Volume(this.card.file), e = v.tryWalk('w'), got = e ? v.read(e).toString('latin1') : '';
        for (const p of v.check()) f.push('the card: ' + p);
        v.close();
        if (got !== RAMW_TEXT) f.push('the card\'s w: ' + got.length + ' bytes, not as written');
        return f;
      },
    },
    {
      name: 'vsd', what: 'the Vera X\'s SD card (the storage driver\'s disk v, on the VERA\'s own SPI controller: the emulator\'s card there), at the login shell: its ctl (an SDHC card, its HydraFS), a file read, one written, /sd listing it',
      init: 'init', cycles: 120e6,
      get machine() { this.card = veraCard(); return { vera: { sd: this.card }, input: VSD_LINES.map(l => '\u0101' + l[0] + '\r').join('') + '\u0101' }; },
      get expect() { return VSD_LINES.map(l => '/> ' + l[0] + '\n' + l[1] + '\n/> '); },
      check() {
        this.card.save();
        const v = new hydrafs.Volume(this.card.file);
        const e = v.tryWalk('new.txt'), got = e ? v.read(e).toString('latin1') : '';
        v.close();
        return got === 'written\n' ? [] : ['the card\'s new.txt: ' + JSON.stringify(got) + ' ("written\\n" wanted)'];
      },
    },
  ],
};
