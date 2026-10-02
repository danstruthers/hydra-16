// ****************************************************************************
// tests/pc.js - regression tests (sim/regress.js runs them): /pc, a folder on the PC served over the serial port (the
// emulator plays the PC tool's part: hydrasim.js --pc-dir).  A test's fields are described at the top of regress.js.
// ****************************************************************************
'use strict';
const { fs, path, W, BOOT, num, P, zsmSong } = require('./common.js');

const README = () => fs.readFileSync(path.join(__dirname, '../../os_rom/romfs/README'));
const STAMP = '\\d{4}-\\d\\d-\\d\\d \\d\\d:\\d\\d:\\d\\d';
const lines = cmds => BOOT + cmds.map(c => c + '\\r').join(P);
const big = () => Buffer.from([...Array(120)].map((_, i) => 'line ' + i + ' of the big file on the PC\r\n').join(''));

module.exports = [
  {
    name: 'pc', about: '/pc, mounted at boot (mount -s pc /pc): a folder on the PC through the serial port, its frames between the console\'s bytes: a listing (ls, ls -l), a file read, a file made by a redirect, a copy to it, mkdir, mv, rmdir, a program run from it (by its path, and by its name through bind -a /pc/bin /bin), cd into it and a relative name in a pipeline, a missing name; what the PC\'s folder holds after',
    pc: { files: { 'hello.txt': 'Hello from the PC\r\nline two\r\n', 'sub/x': 'x', 'sub/deep/': '', 'bin/hi.hyx': fs.readFileSync(path.join(__dirname, '../../programs/c/bin/hello.hyx')) } },
    args: ['--cycles', '150000000', '--input', lines(['ls /pc', 'cat /pc/hello.txt', 'ls -l /pc/sub', 'echo hi > /pc/new.txt', 'cat /pc/new.txt', 'cp /rom/README /pc/r',
      'mkdir /pc/d', 'mv /pc/r /pc/rr', 'cp /rom/bin/code.hyx /pc/c.hyx', '/pc/c 5', 'status .', 'bind -a /pc/bin /bin', 'hi a b', 'cd /pc/sub', 'pwd',
      'cat ../hello.txt | wc . . .', 'rmdir /pc/d', 'cat /pc/nope', 'rm /pc/nope', 'ls /pc'])],
    expect: ['/ram> ls /pc\nbin/\nhello.txt 29\nsub/\n', '/ram> cat /pc/hello.txt\nHello from the PC\nline two\n',
      new RegExp('/ram> ls -l /pc/sub\ndeep/ ' + STAMP + '\nx 1 ' + STAMP + '\n'), '/ram> cat /pc/new.txt\nhi\n', '/ram> /pc/c 5\n\n/ram> status .\n' + num(5) + '\n',
      '/ram> hi a b\nHello from C on the Hydra-16!\n2 arguments: [a] [b]\n', '/pc/sub> pwd\n/pc/sub\n', '/pc/sub> cat ../hello.txt | wc . . .\n 001D ',
      '/pc/sub> cat /pc/nope\n\n !IO ERR! not found\n', '/pc/sub> rm /pc/nope\n\n !IO ERR! not found\n',
      '/pc/sub> ls /pc\nbin/\nc.hyx 2158\nhello.txt 29\nnew.txt 4\nrr 984\nsub/\n'],
    forbid: ['no answer'],
    check: (out, report, files) => {
      const f = n => path.join(files.pc, n);
      if (fs.readFileSync(f('new.txt'), 'latin1') !== 'hi\r\n') return 'new.txt on the PC: ' + JSON.stringify(fs.readFileSync(f('new.txt'), 'latin1'));
      if (!fs.readFileSync(f('rr')).equals(README())) return 'rr on the PC isn\'t /rom/README';
      if (fs.existsSync(f('d')) || fs.existsSync(f('r'))) return 'd or r still on the PC';
      if (!/--- \/pc: 1 attach\(es\), \d+ request\(s\), 0 damaged/.test(report)) return 'not one attach, or damaged frames: ' + (/--- \/pc.*/.exec(report) || [''])[0];
    },
  },
  {
    name: 'pc-two', about: '/pc from two tasks at once (a pipeline\'s stages: one reads a file, the other writes its copy): one request at a time, the other waiting its turn; the copy is the file',
    pc: { files: { 'big': big() } },
    args: ['--cycles', '250000000', '--input', lines(['cat /pc/big | cat > /pc/copy', 'ls /pc'])],
    expect: ['/ram> ls /pc\nbig ' + big().length + '\ncopy ' + big().length + '\n'],
    check: (out, report, files) => {
      if (!fs.readFileSync(path.join(files.pc, 'copy')).equals(big())) return 'the copy on the PC isn\'t big';
    },
  },
  {
    name: 'pc-song', about: 'a song played from /pc (play /pc/t.zsm 2), its loop twice more, in time as from a card: the player reads ahead without waiting (IO_MODE_NONBLOCK, asking again each tick: /pc knows the request it has out), so a read\'s third of a second on the line doesn\'t hold a note up',
    pc: { files: { 't.zsm': zsmSong() } },
    args: ['--cycles', '90000000', '--ym-log', '--input', lines(['play /pc/t.zsm 2', 'status .'])],
    expect: ['/ram> play /pc/t.zsm 2\n\n/ram> status .\n' + num(0) + '\n'],
    check: (out, report) => {
      const on = [...report.matchAll(/ch (\d) at cycle (\d+)/g)].map(m => +m[2]);
      if (on.length !== 4) return 'key-ons: ' + on.length + ', not 4';
      const mhz = +(/ s at ([0-9.]+) MHz/.exec(report) || [0, 3.58])[1], want = 0.6 * mhz * 1e6;   // (The CPU's clock: a variant's)
      for (let k = 1; k < 4; k++) {                                // 4 notes, 0.6 s apart
        const gap = on[k] - on[k - 1];
        if (Math.abs(gap - want) > want / 80) return 'key-on ' + k + ' came ' + gap + ' cycles after the last, not 0.6 s';
      }
    },
  },
  {
    name: 'pc-read-only', about: '/pc served read-only (the PC tool\'s --read-only): a read works; a write, a create, a remove, a mkdir refused (not allowed), and the folder as it was',
    pc: { files: { 'hello.txt': 'Hello\r\n' }, readOnly: true },
    args: ['--cycles', '90000000', '--input', lines(['cat /pc/hello.txt', 'echo no > /pc/x', 'echo no > /pc/hello.txt', 'rm /pc/hello.txt', 'mkdir /pc/d'])],
    expect: ['/ram> cat /pc/hello.txt\nHello\n', '/ram> echo no > /pc/x\n\n !IO ERR! not allowed\n', '/ram> echo no > /pc/hello.txt\n\n !IO ERR! not allowed\n',
      '/ram> rm /pc/hello.txt\n\n !IO ERR! not allowed\n', '/ram> mkdir /pc/d\n\n !IO ERR! not allowed\n'],
    check: (out, report, files) => {
      if (fs.readdirSync(files.pc).join() !== 'hello.txt' || fs.readFileSync(path.join(files.pc, 'hello.txt'), 'latin1') !== 'Hello\r\n') return 'the folder changed: ' + fs.readdirSync(files.pc).join();
    },
  },
  {
    name: 'pc-none', about: '/pc with no PC tool: the attach goes unanswered, so no answer (ERR_IO_DEVICE) after a second, each time; the console goes on',
    args: ['--cycles', '90000000', '--input', lines(['ls /pc', 'cat /pc/x', 'echo still here'])],
    expect: ['/ram> ls /pc\n', ' !IO ERR! no answer\n', '/ram> cat /pc/x\n', ' !IO ERR! no answer\n', '/ram> echo still here\nstill here\n'],
  },
  {
    name: 'pc-damage', about: '/pc\'s frames damaged on the line (--pc-damage): a request (the PC tool asks for it again: its NAK), a reply (the Hydra asks again, and the PC tool answers from its last reply, not doing it twice), another request; the answers are right, and the damaged frames don\'t show',
    pc: { files: { 'hello.txt': 'Hello from the PC\r\n' } },
    args: ['--cycles', '90000000', '--pc-damage', 'q3,r4,q6', '--input', lines(['ls /pc', 'cat /pc/hello.txt', 'cat /pc/hello.txt'])],
    expect: ['/ram> ls /pc\nhello.txt 19\n\n/ram> cat /pc/hello.txt\nHello from the PC\n\n/ram> cat /pc/hello.txt\nHello from the PC\n'],
    forbid: ['!IO ERR!'],
    check: (out, report) => {
      if (!/--- \/pc: 1 attach\(es\), \d+ request\(s\), 2 damaged \(asked again\), 1 repeated/.test(report)) return 'not 2 damaged and 1 repeated: ' + (/--- \/pc.*/.exec(report) || [''])[0];
    },
  },
];
