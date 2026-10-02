// ****************************************************************************
// tests/forth.js - regression tests (sim/regress.js runs them): HyForth: the language, its line editor, machine code, libraries, pipelines.  A test's fields are described at
// the top of regress.js.
// ****************************************************************************
'use strict';
const { W, BOOT, romSym, num, P } = require('./common.js');

module.exports = [
  {
    name: 'forth', about: 'HyForth: arithmetic (decimal in, hex out), negatives, $ and % prefixes, typed wc (Ctrl-D ends it)',
    args: ['--cycles', '60000000', '--input', BOOT + '1 2 + .\\r1000 24 - .\\r-1 . -2 . -9 . -10 . $B . $1F . %101 .\\rwc\\rab c\\r\\x04. . .\\r'],
    expect: ['/ram> 1 2 + .\n' + num(3) + '\n', num(1000 - 24),
      ' FFFF FFFE FFF7 FFF6' + num(0xB) + num(0x1F) + num(5) + '\n',
      '/ram> . . .\n' + num(5) + num(2) + num(1) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'forth-numbers', about: 'HyForth\'s numbers: compiled into a definition (and lit [ n , ] still), decimal and hex output (decimal, hex, u.), decimal input\'s range (-32768 to 32767; too big is no number)',
    args: ['--cycles', '60000000', '--input', BOOT + [': x 65 -300 $1F %101 . . . . ;\\r', 'x x\\r', ': y lit [ 66 , ] . ;\\ry\\r',
      'decimal 65 . -264 . 0 . 32767 . -32768 . $FFFF . $FFFF u. x\\r', 'hex 65 . $FFFF u.\\r', '32768\\r', '70000\\r', '1 2 + .\\r'].join(P)],
    expect: ['/ram> x x\n' + num(5) + num(0x1F) + ' FED4' + num(65) + num(5) + num(0x1F) + ' FED4' + num(65) + '\n',
      '/ram> y\n' + num(66) + '\n', ' 65 -264 0 32767 -32768 -1 65535 5 31 -300 65\n', '/ram> hex 65 . $FFFF u.\n' + num(65) + ' FFFF\n',
      '/ram> 32768\n\n !UNK WORD!\n', '/ram> 70000\n\n !UNK WORD!\n', '/ram> 1 2 + .\n' + num(3) + '\n'],
    forbid: ['!DS PTR ERROR!'],
  },
  {
    name: 'line-edit', about: 'the console\'s line editor: Left, Right, Home, End (and their Ctrl keys), Backspace and Delete in the middle of a line, history (Up, Down, Ctrl-P, Ctrl-N; the same line twice kept once), Ctrl-U; Ctrl-C while editing, then a program reading the console as before (wc)',
    args: ['--cycles', '90000000', '--input', BOOT + [
      '12\\x1b[D\\x1b[D9\\x1b[F .\\r',                 // 12, Left Left, 9, End: 912 .
      '\\x1b[A\\x1b[A\\r',                              // Up twice: 912 . (the only line: Up again does nothing)
      '1 + .\\x01\\x06 12\\x05\\r',                      // Ctrl-A, Ctrl-F, " 12", Ctrl-E: 1 12 + .
      '\\x10\\x10\\x10\\x0e\\x0e\\r',                    // Ctrl-P three times, Ctrl-N twice: past the newest, an empty line
      '\\x10\\x10\\x10\\x10\\r',                         // Ctrl-P four times: the oldest, 912 . (the empty line isn't kept)
      'abc\\x15 5 6 + .\\r',                           // Ctrl-U
      '7 8\\x1b[H\\x1b[3~9\\x1b[4~ * .\\r',              // Home, Delete (the 7), 9, End: 9 8 * .
      '1x2\\x02\\x7f\\x1b[C 3 + .\\r',                   // Ctrl-B, Backspace (the x), Right: 12 3 + .
      'abc\\x03', '3 4 + .\\r', 'wc\\r', 'ab c\\r\\x04', '. . .\\r'].join(W(1))],
    expect: ['\n' + num(912) + '\n/ram> ', '\n' + num(912) + '\n/ram> ', '\n' + num(13) + '\n/ram> ', '\n/ram> ', '\n' + num(912) + '\n/ram> ',
      '\n' + num(11) + '\n/ram> ', '\n' + num(72) + '\n/ram> ', '\n' + num(15) + '\n/ram> ',
      '\n !BREAK!\n', '\n' + num(7) + '\n', '/ram> wc\nab c\n', '/ram> . . .\n' + num(5) + num(2) + num(1) + '\n'],
    forbid: ['!DS PTR ERROR!'],
  },
  {
    name: 'syscall', about: 'machine code from HyForth: syscall and sys call thunks on ROM page 0 (TICKS_GET, an IO_OPEN that fails: / opened for writing); a bload word on page 1 calls a thunk page 1 has no gate for (TICKS_GET: page 1\'s table goes on to page 0\'s)',
    args: () => {                         // The bload word, TKX: jsr TICKS_GET, sta TEMP1, sty TEMP1 + 1, jsr spush_0, jmp next
      const t1 = romSym('TEMP1', 'PAGE1'), push = romSym('spush_0', 'PAGE1'), next = romSym('next', 'PAGE1');
      const code = [3, 84, 75, 88, 0x22, 0, 0x20, 0xEA, 0xF8, 0x85, t1, 0x84, t1 + 1, 0x20, push & 255, push >> 8, 0x4C, next & 255, next >> 8, 0, 0, 69, 78, 68];
      return ['--cycles', '80000000', '--input', BOOT + ['$F806 $41 0 syscall drop\\r', '$F8EA 0 0 0 sys . . . .\\r', 'cd /\\r', '$F86C 0 2 0 sys . . . .\\r',
        '$5000\\r' + code.map(b => 'dup ' + b + ' swap c! 1 +\\r').join('') + 'drop\\r', '$5000 bload\\r' + W(1), 'TKX .\\r', '1 2 + .\\r'].join(W(1))];
    },
    expect: ['/ram> $F806 $41 0 syscall drop\n41\n', /\/ram> \$F8EA 0 0 0 sys \. \. \. \.\n [0-9A-F]{4} 000[0-9A-F] [0-9A-F]{4} [0-9A-F]{4}\n/,
      /\/> \$F86C 0 2 0 sys \. \. \. \.\n [0-9A-F]{3}[13579BDF] 0000 0002 0072\n/, /\/> TKX \.\n [0-9A-F]{4}\n/, '/> 1 2 + .\n' + num(3)],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!', '!SYS ERR!'],
  },
  {
    name: 'libs', about: 'HyForth\'s libraries: all loaded at boot (libs); -lib and lib (and what one needs); without the shell\'s, a plain prompt, and no pipelines or programs by name; unknown names',
    args: ['--cycles', '60000000', '--input', BOOT + 'libs\\r-lib sound\\rlibs\\rsndinit\\rlib sound\\rsndinit\\r' +
      '-lib io\\r-lib files\\rlibs\\rlib shell\\rlibs\\r-lib shell\\r1 2 + .\\rwords | wc\\rfoo\\rlib shell\\r-lib bogus\\rlib\\r'],
    expect: ['/ram> libs\nforth io files shell tasks sound mem tools term\n',
      '/ram> libs\nforth io files shell tasks (sound) mem tools term\n', '/ram> sndinit\n\n !UNK WORD!\n', '/ram> sndinit\n\n/ram> ',
      '/ram> libs\nforth (io) (files) shell tasks sound mem tools term\n',
      '/ram> libs\nforth io files shell tasks sound mem tools term\n',            // (lib shell: io and files too)
      '\n> 1 2 + .\n' + num(3) + '\n', '> words | wc\n', '\n> foo\n\n !UNK WORD!\n', '> lib shell\n\n/ram> -lib bogus\n\n !UNK WORD!\n',
      '/ram> lib\n\n !UNK WORD!\n'],
  },
  {
    name: 'libfiles', about: 'libraries from files (lib name: name.hyl in /lib): one loading another, an error dropping one, a missing one, -lib and lib again, lib all, words, out of slots',
    sd: [{ dev: 0, label: 'LIBS', hfs: v => {
      v.mkdir('lib');
      v.put('lib/greet.hyl', Buffer.from('9 .\r\n: greet 7 . ;\r\n: twice dup + ;\r\n'));
      v.put('lib/m2.hyl', Buffer.from('1 .\r\nlib greet\r\n: m2w greet greet ;\r\n2 .\r\n'));
      v.put('lib/bad.hyl', Buffer.from(': oops nosuchword ;\r\n'));
      v.put('lib/c.hyl', Buffer.from(': cw 3 . ;\r\n'));
      v.put('lib/d.hyl', Buffer.from(': dw 4 . ;\r\n'));
      v.put('lib/e.hyl', Buffer.from(': ew 5 . ;\r\n'));
    } }],
    args: ['--cycles', '150000000', '--input', BOOT + [': mine 6 . ;\\rlib m2\\r', 'm2w\\r3 twice .\\rmine\\rlibs\\r', 'lib bad\\rlibs\\roops\\r',
      'lib nope\\r', '-lib greet\\rlibs\\rgreet\\rlib greet\\rgreet\\r', '-lib all\\rlibs\\rlib all\\rlibs\\r', 'words\\r',
      'lib c\\rlib d\\rlib e\\rcw dw\\rlibs\\r'].join(P)],
    expect: ['/> lib m2\n' + num(1) + num(9) + num(2) + '\n',             // (greet, loaded by m2: once)
      '/> m2w\n' + num(7) + num(7) + '\n', '/> 3 twice .\n' + num(6) + '\n', '/> mine\n' + num(6) + '\n',
      '/> libs\nforth io files shell tasks sound mem tools term m2 greet\n',
      '/> lib bad\n\n !UNK WORD!\nline 0001\n', '/> libs\nforth io files shell tasks sound mem tools term m2 greet\n',
      '/> oops\n\n !UNK WORD!\n', '/> lib nope\n\n !IO ERR! ',
      '/> libs\nforth io files shell tasks sound mem tools term m2 (greet)\n', '/> greet\n\n !UNK WORD!\n',
      '/> lib greet\n\n0:/> greet\n' + num(7) + '\n',                // (searched again: not read again)
      '> libs\nforth (io) (files) (shell) (tasks) (sound) (mem) (tools) (term) (m2) (greet)\n',
      '/> libs\nforth io files shell tasks sound mem tools term m2 greet\n',
      /: mine +\| [0-9A-F]{4}: m2w +\| [0-9A-F]{4}: twice +\| [0-9A-F]{4}: greet +\|/,
      '/> lib e\n\n !LOW MEM!\n', '/> cw dw\n' + num(3) + num(4) + '\n',
      '/> libs\nforth io files shell tasks sound mem tools term m2 greet c d\n'],
    forbid: ['!DS PTR ERROR!', num(5)],
  },
  {
    name: 'forth-bare', about: 'a bare Forth (forth): its own task, only the base loaded, lib loads what it needs; -lib all and lib all',
    args: ['--cycles', '60000000', '--input', BOOT + 'forth .\\r' + W(1) + '\\x1dB' + W(1) + '\\rlibs\\r1 2 + .\\rls\\rlib files\\rlibs\\r' +
      'lib all\\rlibs\\r-lib all\\rlibs\\r'],
    expect: [/\/ram> forth \.\n\n? 000B\n/, '[B]',                   // (The new task's first CR LF may come first: both write) '> libs\nforth (io) (files) (shell) (tasks) (sound) (mem) (tools) (term)\n',
      '> 1 2 + .\n' + num(3) + '\n', '> ls\n\n !UNK WORD!\n', '> libs\nforth io files (shell) (tasks) (sound) (mem) (tools) (term)\n',
      '/ram> libs\nforth io files shell tasks sound mem tools term\n',             // (lib all: the shell's prompt again)
      '> libs\nforth (io) (files) (shell) (tasks) (sound) (mem) (tools) (term)\n'],
  },
  {
    name: 'pipes', about: 'pipelines: words | wc, and through cat (a copy of the shell in the middle) gives the same',
    args: ['--cycles', '150000000', '--input', BOOT + 'words | wc . . .\\rwords | cat | wc . . .\\rwords | cat | cat | wc . . .\\r1 2 + .\\r'],
    expect: [/\/ram> words \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/, /\/ram> words \| cat \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/,
      /\/ram> words \| cat \| cat \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/, '/ram> 1 2 + .\n' + num(3)],
    forbid: ['!DS PTR ERROR!', '!IO ERR!'],
    check: out => {
      const counts = [...out.matchAll(/wc \. \. \.\n((?: [0-9A-F]{4}){3})\n/g)].map(m => m[1]);
      if (counts.length !== 3 || counts.some(c => c !== counts[0])) return 'the pipelines counted differently: ' + counts.join(' /');
      if (/ 0000 0000 0000/.test(counts[0])) return 'wc counted nothing';
    },
  },
];
