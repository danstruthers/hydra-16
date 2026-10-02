// ****************************************************************************
// tests/shell.js - regression tests (sim/regress.js runs them): the shell: directories, running programs, exit statuses, C programs, redirection, the editor, the environment.  A test's fields are described at
// the top of regress.js.
// ****************************************************************************
'use strict';
const { fs, path, hydrafs, hyx, W, C_SAMPLES, BOOT, num, P } = require('./common.js');

module.exports = [
  {
    name: 'shell', about: 'the boot shell: the lowest HydraFS volume selected, boot.hys (and a script it includes); cd, pwd, the prompt\'s format, cp, cat, include\'s error line',
    sd: [{ dev: 0 }, { dev: 1, label: 'ONE', hfs: v => {        // No HydraFS on card 0: card 1 is selected
      v.put('boot.hys', Buffer.from(': hi 7 . ;\r\nhi\r\ninclude lib.hys\r\n'));
      v.put('lib.hys', Buffer.from('q^lib^ drop 9 .\r\n'));
      v.put('err.hys', Buffer.from('1 .\r\nnope\r\n2 .\r\n'));
    } }, { dev: 2, label: 'TWO', hfs: () => {} }],
    args: ['--cycles', '150000000', '--input', BOOT + W(1) + ['hi\\r', 'mkdir games\\rcd games\\rpwd\\r', 'cd ..\\rcd /sd/2\\rpwd\\r',
      'cd\\rq^%p %t$ ^ prompt\\r', 'q^%v%d> ^ prompt\\rcp /sd/1/lib.hys lib2.hys\\rcat lib2.hys\\r', 'include /sd/1/err.hys\\r',
      'cd nowhere\\rioerr .\\rcd /sd/1/boot.hys\\rioerr .\\r',
      '"hello world" .sz\\r"a | b" .sz\\r"" .sz 3 .\\r"x^y" .sz q^p"q^ .sz\\rmkdir "my dir"\\rcd "my dir"\\rpwd\\r"%t> " prompt\\r',
      '"[%l] %v%d> " prompt\\rcd /\\rcd /sd/1\\r2 "SECOND" relabel\\rcd /sd/2\\r'].join(W(1))],
    expect: ['hydrafs 1 2\n', num(7) + num(9) + '\n1:/> hi\n' + num(7),   // boot.hys, and the script it includes
      '1:/games> pwd\n/sd/1/games\n', '2:/> pwd\n/sd/2\n', '/sd/2 1$ q^%v%d> ^ prompt\n',
      '2:/> cat lib2.hys\nq^lib^ drop 9 .\n', '2:/> include /sd/1/err.hys\n' + num(1) + '\n !UNK WORD!\nline 0002\n2:/> ',
      '2:/> ioerr .\n' + num(0x70) + '\n', '2:/> ioerr .\n' + num(0x85) + '\n',   // (Not a directory)
      '2:/> "hello world" .sz\nhello world\n', '2:/> "a | b" .sz\na | b\n',   // "..." strings: not a pipeline
      '2:/> "" .sz 3 .\n' + num(3) + '\n', 'q^ .sz\nx^yp"q\n',             // (Empty; each delimiter inside the other)
      '2:/my dir> pwd\n/sd/2/my dir\n', '1> ',                           // A quoted name, spaces and all
      '[TWO] 2:/my dir> cd /\n', '[] /> cd /sd/1\n', '[ONE] 1:/> 2 "SECOND" relabel\n',   // The label (%l), off the
      '[ONE] 1:/> cd /sd/2\n', '[SECOND] 2:/> '],                          //   cards none; a relabel is seen
    forbid: ['!DS PTR ERROR!', /\n 0002\n/],                    // (err.hys stops at its error)
    check: (out, report, files) => {
      const v = new hydrafs.Volume(files.sds[1]);
      try { if (!v.tryWalk('games') || !v.tryWalk('games').isDir) return 'no games directory on card 1'; } finally { v.close(); }
    },
  },
  {
    name: 'run', about: 'running programs: .hyx executables (run, by name, from /bin), .hys scripts in a copy of the shell, a bad header, a script\'s error, Ctrl-C',
    sd: [{ dev: 0, label: 'PROGS', hfs: v => {
      // ldx #0 / lda msg,X / beq +6 / jsr WRITE_CHAR ($F803) / inx / bne -11 / rts / msg: "hyx ok" CR LF 0
      const hello = hyx(0x0800, [0xA2, 0x00, 0xBD, 0x0E, 0x08, 0xF0, 0x06, 0x20, 0x03, 0xF8, 0xE8, 0xD0, 0xF5, 0x60,
        ...Buffer.from('hyx ok\r\n', 'latin1'), 0]);
      v.put('hello.hyx', hello);
      v.put('loop.hyx', hyx(0x0800, [0x4C, 0x00, 0x08]));     // jmp * (Ctrl-C ends it)
      const bad = hyx(0x0800, [0x60]);
      bad.writeUInt16LE(0x0400, 4);                             // Loads below $0800
      v.put('bad.hyx', bad);
      v.put('add.hys', Buffer.from('+ .\r\n: sq dup * ; 5 sq .\r\n'));
      v.put('err.hys', Buffer.from('1 .\r\nnope\r\n2 .\r\n'));
      v.mkdir('bin');
      v.put('bin/hi.hyx', hello);
      v.put('bin/greet.hys', Buffer.from('7 .\r\n'));
    } }],
    args: ['--cycles', '200000000', '--input', BOOT + ['run hello.hyx\\r', 'hello\\r', 'mkdir sub\\rcd sub\\rhi\\rgreet\\rcd ..\\r',
      '3 4 run add.hys\\r. .\\rsq\\r', 'q^hello.hyx^ (run)\\r', 'run bad.hyx\\rioerr .\\r', 'run err.hys\\r5 .\\r', 'run hello.hyx | cat\\r',
      'run loop.hyx\\r' + W(2) + '\\x03' + P + 'ps\\r', 'nosuch\\r'].join(P)],
    expect: ['0:/> run hello.hyx\nhyx ok\n', '0:/> hello\nhyx ok\n',
      '0:/sub> hi\nhyx ok\n', '0:/sub> greet\n' + num(7) + '\n',   // From /bin, with the current directory elsewhere
      '0:/> 3 4 run add.hys\n' + num(7) + num(0x19) + '\n',     // A copy of the shell, with a copy of the stack ...
      '0:/> . .\n' + num(4) + num(3) + '\n', '0:/> sq\n\n !UNK WORD!\n',   // ... and its definitions go with it
      '0:/> q^hello.hyx^ (run)\nhyx ok\n', '0:/> ioerr .\n' + num(0x87) + '\n',
      '0:/> run err.hys\n' + num(1) + '\n !UNK WORD!\nline 0002\n0:/> 5 .\n' + num(5) + '\n',
      '0:/> run hello.hyx | cat\nhyx ok\n',
      /0:\/> ps\n0 R -\n1 R 0 \*\n[C-F] D -\n/,                // Nothing left of the programs (the loop, killed)
      '0:/> nosuch\n\n !UNK WORD!\n'],
    forbid: ['!DS PTR ERROR!', /\n 0002\n/],
  },
  {
    name: 'exit-status', about: 'exit statuses (Plan 9\'s exits): a script\'s exits, errors\', a C program\'s code and message; status, $status; exits at the boot shell; a program started with & ([B], $apid), wait for it',
    sd: [{ dev: 0, label: 'STATUS', hfs: v => {
      v.mkdir('bin');
      for (const p of ['code', 'upper']) v.put('bin/' + p + '.hyx', fs.readFileSync(path.join(__dirname, '../../programs/c/bin/' + p + '.hyx')));
      v.put('ex.hys', Buffer.from('1 .\r\n7 exits\r\n2 .\r\n'));
      v.put('err.hys', Buffer.from('nope\r\n3 .\r\n'));
    } }],
    args: ['--cycles', '150000000', '--input', BOOT + ['run ex.hys\\r', 'status .\\rcat /env/status\\r', 'nosuch\\rstatus .\\rcat /env/status\\r',
      'code 3\\rstatus .\\r', 'code oops\\rstatus .\\rcat /env/status\\r', 'code\\rstatus .\\r', 'run err.hys\\rstatus .\\r',
      '9 exits status .\\r', 'upper &\\r' + W(1) + 'ps\\r', '$B wait\\r' + W(1) + 'hi\\r' + W(1) + '\\x04' + W(1) + 'status .\\rcat /env/apid\\r'].join(W(1))],
    expect: ['0:/> run ex.hys\n' + num(1) + '\n', '0:/> status .\n' + num(7) + '\n', '0:/> cat /env/status\n7\n',
      '0:/> nosuch\n\n !UNK WORD!\n', '0:/> status .\n' + num(5) + '\n', '0:/> cat /env/status\nUNK WORD\n',
      '0:/> code 3\n\n0:/> status .\n' + num(3) + '\n', '0:/> status .\n' + num(1) + '\n', '0:/> cat /env/status\noops\n',
      '0:/> code\n\n0:/> status .\n' + num(0) + '\n',
      '0:/> run err.hys\n\n !UNK WORD!\nline 0001\n0:/> status .\n' + num(5) + '\n',       // (The script's error: its status)
      '0:/> 9 exits status .\n' + num(9) + '\n',                                          // (The boot shell stays)
      '0:/> upper &\n[B]\n', /0:\/> ps\n0 R -\n1 R 0 \*\nB W 1\n/,                          // (Waiting for the console)
      '0:/> $B wait\nhi\nHI\n', '0:/> status .\n' + num(0) + '\n', '0:/> cat /env/apid\nB\n'],  // (Enter: a new line, read.c)
    forbid: [/\n 0002\n/],
  },
  {
    name: 'c-programs', about: 'C programs (cc65 and programs/c\'s library): hello (arguments, long arithmetic, the heap, the clock), ctest (argv[0], stdio and the file calls, directories, errno, the heap, time, semaphores, the environment, stat, dirent, system and exit statuses, clock, isatty), one into a pipe, upper (stdin: a pipe from another, a file with <), keys (conio: the screen, raw keys, the terminal\'s key sequences); hydra.inc agrees with the ROM',
    sd: [{ dev: 0, label: 'CPROGS', hfs: v => {
      v.mkdir('bin');
      for (const p of C_SAMPLES) v.put('bin/' + p + '.hyx', fs.readFileSync(path.join(__dirname, '../../programs/c/bin/' + p + '.hyx')));
    } }],
    args: ['--cycles', '300000000', '--rtc', '2026-09-30T14:05:00', '--input', W(3) + ['hello one "two three"\\r', 'ctest a "b c"\\r' + W(12),
      'hello | wc . . .\\r', 'ls\\r', 'hello x | upper\\r' + W(1), 'echo abc def > t.txt\\rupper < t.txt\\r',
      'keys\\r' + W(1) + 'a' + W(1) + '\\x1b[A' + W(1) + '\\x1b[3~' + W(1) + '\\x1bOP' + W(1) + 'q'].join(W(1))],
    expect: ['0:/> hello one "two three"\nHello from C on the Hydra-16!\n2 arguments: [one] [two three]\n1^2 + ... + 1000^2 = 333833500\n',
      /\nThe clock says 2026-09-30 14:05:\d\d\n/, '0:/> ctest a "b c"\nok arguments\n', '\nctest: 0 failed\n',
      /\/> hello \| wc \. \. \.\n( [0-9A-F]{4}){3}\n/,
      '0:/> ls\nbin/\n',                                                   // (ctest's files and directory: gone)
      '0:/> hello x | upper\nHELLO FROM C ON THE HYDRA-16!\n1 ARGUMENTS: [X]\n', '0:/> upper < t.txt\nABC DEF\n',
      '0:/> keys\n<ESC>[2J<ESC>[1;1H<ESC>[33mkeys: a 80x24 screen; type keys, q to end\n',     // (No echo: raw)
      '<ESC>[37m<ESC>[7m<ESC>[3;1Hcodes:<ESC>[27m 61 80 89 8A\nended at 18,2\n'],
    forbid: ['FAIL', '!IO ERR!'],
    check: () => {                        // programs/c/lib/hydra.inc: the thunks, ZP parameters and constants as the ROM has them
      const lbl = fs.readFileSync(path.join(__dirname, '../../os_rom/obj/os_rom_C02.lbl'), 'latin1');
      const addr = n => { const m = new RegExp('^al ([0-9A-F]{6}) \\.' + n + '$', 'm').exec(lbl); return m ? parseInt(m[1], 16) : undefined; };
      const incs = ['include/kernel.inc', 'include/io.inc', 'include/shell.inc', 'include/hw.inc']
        .map(n => fs.readFileSync(path.join(__dirname, '../../os_rom', n), 'latin1')).join('\n');
      const rom = n => { const m = new RegExp('^' + n + '\\s*=\\s*(\\$[0-9A-Fa-f]+|\\d+)\\s*(;|$)', 'm').exec(incs);
        return m ? (m[1][0] === '$' ? parseInt(m[1].slice(1), 16) : +m[1]) : undefined; };
      const inc = fs.readFileSync(path.join(__dirname, '../../programs/c/lib/hydra.inc'), 'latin1');
      let n = 0;
      for (const m of inc.matchAll(/^([A-Z_0-9]+)\s*=\s*(\$[0-9A-Fa-f]+|\d+)/gm)) {
        const v = m[2][0] === '$' ? parseInt(m[2].slice(1), 16) : +m[2];
        const want = v >= 0xF800 ? addr('TH_' + m[1]) : m[1].startsWith('ZP_') ? addr(m[1]) : rom(m[1]);
        if (want === undefined) continue;                              // (Not the ROM's: TICKS_PER_SEC, UNIX_2000 ...)
        if (want !== v) return 'hydra.inc: ' + m[1] + ' is $' + v.toString(16) + ', the ROM has $' + want.toString(16);
        n++;
      }
      if (n < 60) return 'hydra.inc: only ' + n + ' of its names found in the ROM';
    },
  },
  {
    name: 'redirect', about: 'redirection (>, >>, <, a quoted name, in a script, a bad name), a program\'s arguments (.hyx, by name, a script\'s args), echo',
    sd: [{ dev: 0, label: 'REDIR', hfs: v => {
      v.put('s.hys', Buffer.from('"in script" .sz > s.txt\r\n2 .\r\nwc < s.txt . . .\r\n3 .\r\n'));
      v.put('args.hys', Buffer.from('args .sz\r\n'));
      // sta $F0 / sty $F1 / ldy #0 / lda ($F0),Y / beq +6 / jsr WRITE_CHAR / iny / bne -10 / CR LF: its arguments
      v.put('pargs.hyx', hyx(0x0800, [0x85, 0xF0, 0x84, 0xF1, 0xA0, 0x00, 0xB1, 0xF0, 0xF0, 0x06, 0x20, 0x03, 0xF8, 0xC8,
        0xD0, 0xF6, 0xA9, 0x0D, 0x20, 0x03, 0xF8, 0xA9, 0x0A, 0x4C, 0x03, 0xF8]));
    } }],
    args: ['--cycles', '120000000', '--input', BOOT + ['"hello" .sz > h.txt\\rcat h.txt\\r', '"more" .sz >> h.txt\\rcat h.txt\\r',
      'wc < h.txt . . .\\r', 'words | wc . . . > c.txt\\rcat c.txt\\r', 'cat < nofile\\r1 .\\r', '"x" .sz > "a b.txt"\\rls\\r',
      'include s.hys\\rcat s.txt\\r', '"b" .sz >> new.txt\\rcat new.txt\\r', 'run pargs.hyx one two\\r', 'pargs three "four five"\\r',
      'run args.hys x y z\\r', 'echo hello there > e.txt\\rcat e.txt\\recho "quoted  text"\\r'].join(P)],
    expect: ['cat h.txt\nhello\n', 'cat h.txt\nhellomore\n', 'wc < h.txt . . .\n' + num(9) + num(1) + num(0) + '\n',
      /cat c\.txt\n( [0-9A-F]{4}){3}\n/,                          // (A pipeline's last command, into a file)
      'cat < nofile\n\n !IO ERR! ', '1 .\n' + num(1) + '\n', 'a b.txt 1\n',
      'include s.hys\n' + num(2) + num(9) + num(2) + num(0) + num(3) + '\n',   // (The script reads on after its <)
      'cat s.txt\nin script\n', 'cat new.txt\nb\n',
      'run pargs.hyx one two\none two\n', 'pargs three "four five"\nthree "four five"\n', 'run args.hys x y z\nx y z\n',
      'cat e.txt\nhello there\n', 'echo "quoted  text"\nquoted  text\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const v = new hydrafs.Volume(files.sds[0]);
      try {
        const text = n => v.read(v.walk(n)).toString('latin1');
        if (text('h.txt') !== 'hellomore' || text('e.txt') !== 'hello there\r\n') return 'h.txt or e.txt is wrong';
        const p = v.check(); if (p.length) return p[0];
      } finally { v.close(); }
    },
  },
  {
    name: 'editor', about: 'edit: add, insert, change, delete, print, write, q (twice with changes), a new unnamed file, errors, help, Ctrl-C, commands from a file',
    sd: [{ dev: 0, label: 'EDIT', hfs: v => v.put('cmds.txt', Buffer.from('a\r\none\r\ntwo\r\n.\r\n1d\r\nw\r\nq\r\n')) }],
    args: ['--cycles', '200000000', '--input', BOOT + ['edit notes.txt\\r', 'a\\rfirst line\\rsecond line\\r.\\rp\\r', 'i 1\\rzero\\r.\\r2p\\r',
      'c 2\\rFIRST\\r.\\rd 3\\rp\\rw\\rq\\r', 'cat notes.txt\\r', 'edit notes.txt\\r', 'a\\rmore\\r.\\rq\\rq\\r', 'edit\\ra\\rx\\r.\\rw\\rw other.txt\\rQ\\r',
      'edit notes.txt\\rp 9\\rd\\rz\\r$p\\r1,$p\\ra\\rthree\\r' + W(1) + '\\x03', 'p\\rQ\\r', 'edit s.txt < cmds.txt\\r' + W(2) + 'cat s.txt\\r'].join(W(1))],
    expect: ['edit notes.txt\nnotes.txt: new file\n', '*p\n   1 first line\n   2 second line\n', '*2p\n   2 first line\n',
      '*p\n   1 zero\n   2 FIRST\n*w\n 13 bytes\n*q\n', 'cat notes.txt\nzero\nFIRST\n',
      'notes.txt: 2 lines\n', '*q\n? not written: q again to quit anyway\n*q\n',
      '*w\n? no file name (w name)\n*w other.txt\n 3 bytes\n',
      '*p 9\n? no such line\n*d\n? which lines\n*z\n? h: help\n*$p\n   2 FIRST\n*1,$p\n   1 zero\n   2 FIRST\n',
      '?\n*p\n   1 zero\n   2 FIRST\n   3 three\n*Q\n',                // Ctrl-C: back to the prompt, the line kept
      'cat s.txt\ntwo\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!', /POST[\s\S]*POST/],  // (POST again: a crash, and the machine starting again)
    check: (out, report, files) => {
      const v = new hydrafs.Volume(files.sds[0]);
      try {
        const text = n => v.read(v.walk(n)).toString('latin1');
        if (text('notes.txt') !== 'zero\r\nFIRST\r\n' || text('other.txt') !== 'x\r\n' || text('s.txt') !== 'two\r\n') return 'a file the editor wrote is wrong';
      } finally { v.close(); }
    },
  },
  {
    name: 'env', about: '/env (make, read, add to, list, remove, errors), a copy for each task (a script\'s changes stay its own), PATH, HOME, /dev/proc/N/cwd, env and mem',
    sd: [{ dev: 0, label: 'ENV', hfs: v => {
      v.mkdir('tools'); v.put('tools/seven.hys', Buffer.from('7 .\r\n'));
      v.mkdir('bin'); v.put('bin/eight.hys', Buffer.from('8 .\r\n'));
      v.put('e.hys', Buffer.from('cat /env/A\r\necho two > /env/B\r\nls /env\r\n'));
    } }],
    args: ['--cycles', '120000000', '--input', BOOT + ['ls /env\\recho one > /env/A\\rcat /env/A\\r', 'echo x >> /env/A\\rcat /env/A\\r',
      'run e.hys\\r', 'ls /env\\r', 'cat /env/NOPE\\rioerr .\\recho y > /env/a=b\\rioerr .\\r', 'rm /env/A\\rls /env\\r',
      'seven\\r', 'echo /sd/0/tools > /env/PATH\\rseven\\r', 'eight\\r', 'echo /sd/0/bin:/sd/0/tools > /env/PATH\\reight\\rseven\\r',
      'echo /sd/0/tools > /env/HOME\\rcd /\\rcd\\rpwd\\r', 'cat /dev/proc/1/cwd\\rcat /dev/proc/1/env\\rcat /proc/1/pages\\rcat /proc/F/pages\\rcat /proc/9/pages\\r'].join(P)],
    expect: ['0:/> ls /env\n\n0:/> echo one > /env/A\n', 'cat /env/A\none\n', 'cat /env/A\nx\n',   // (>> at the start: replaced)
      'run e.hys\nxA=x\nB=two\n', '0:/> ls /env\nA=x\nstatus=\n\n',     // The script's B: its own copy's; $status ""
      'cat /env/NOPE\n\n !IO ERR! ', 'ioerr .\n' + num(0x70) + '\n', 'ioerr .\n' + num(0x77) + '\n',   // (= in a name)
      'rm /env/A\n', '0:/> ls /env\nstatus=IO ERR\n\n',             // ($status: the last error's) '0:/> seven\n\n !UNK WORD!\n',
      'PATH\n\n0:/> seven\n' + num(7) + '\n', '0:/> eight\n\n !UNK WORD!\n',   // (PATH, not the card's /bin)
      'eight\n' + num(8) + '\n0:/> seven\n' + num(7) + '\n',
      '0:/tools> pwd\n/sd/0/tools\n',                                   // cd alone: HOME
      'cat /dev/proc/1/cwd\n/sd/0/tools\n', 'cat /dev/proc/1/env\nPATH=/sd/0/bin:/sd/0/tools\nstatus=UNK WORD\nHOME=/sd/0/tools\n',
      /cat \/proc\/1\/pages\npages 08 floor [0-9A-F]{2}\n/, 'cat /proc/F/pages\npages 00 floor 08\n', 'cat /proc/9/pages\n-\n'],
    forbid: ['!DS PTR ERROR!'],
  },
  {
    name: 'sleep', about: 'TASK_SLEEP (HyForth sleep): 400 ticks take 2 s, and Ctrl-C ends a long one',
    args: ['--cycles', '40000000', '--mark', '400 sleep', '--mark', '> ', '--input', BOOT + '400 sleep\\r' + W(5) + '30000 sleep\\r' + W(1) + '\\x03' + W(1) + '1 2 + .\\r'],
    expect: ['/ram/1> 400 sleep\n', '/ram/1> 30000 sleep\n', '!BREAK!', '/ram/1> 1 2 + .\n' + num(3)],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {                                   // The line typed to the prompt: 2 s is 7.16M cycles at 3.58 MHz
      const typed = +/mark: "400 sleep" at cycle (\d+)/.exec(report)[1];
      const took = [...report.matchAll(/mark: "> " at cycle (\d+)/g)].map(m => +m[1]).find(c => c > typed) - typed;
      if (!(took > 7150000 && took < 7400000)) return '400 sleep took ' + took + ' cycles, not about 7.16M';
    },
  },
  {
    name: 'sd-shared', about: 'two shells read /dev/sd at once: the storage server is switched out mid-request, and the other waits for it',
    sd: true,
    args: ['--cycles', '90000000', '--input', BOOT + 'shell\\r' + W(1) + '\\x1dB' + W(1) + '\\rq^/dev/sd/0/data^ 1 open .\\r' + W(1) +
      '6 here @ 600 read '.repeat(12) + '\\r\\x1d1q^/dev/sd/0/data^ 1 open .\\r' + '3 here @ 600 read . '.repeat(6) + '\\r' + W(12) +
      '\\x1dB' + W(1) + '\\r' + '+ '.repeat(11) + '.\\r'],             // (B's 12 counts, added up: 7200 = $1C20)
    expect: ['/ram/1> q^/dev/sd/0/data^ 1 open .\n' + num(6), '[1]q^/dev/sd/0/data^ 1 open .\n' + num(3), '[B]', '+ .\n' + num(7200) + '\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: out => {                                             // Shell 1's 6 reads (B's prompt can come out among them)
      const n = (out.slice(out.indexOf('[1]'), out.lastIndexOf('[B]')).match(/0258/g) || []).length;
      if (n !== 6) return 'shell 1 read 600 bytes ($0258) ' + n + ' times, not 6';
    },
  },
];
