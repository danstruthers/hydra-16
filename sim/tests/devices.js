// ****************************************************************************
// tests/devices.js - regression tests (sim/regress.js runs them): the IO layer and the devices: namespaces, tasks, sound and songs, /rom, the clock chip, semaphores, the serial port.  A test's fields are described at
// the top of regress.js.
// ****************************************************************************
'use strict';
const { fs, path, hydrafs, hyx, W, zsmSong, BOOT, num, P } = require('./common.js');

// The files in /rom: romfs.txt's lines, { path, src, data }
function romFiles() {
  const dir = path.join(__dirname, '../../os_rom');
  return fs.readFileSync(path.join(dir, 'romfs.txt'), 'latin1').split(/\r?\n/).map(l => l.replace(/;.*/, '').trim()).filter(l => l)
    .map(l => { const [p, src] = l.split(/\s+/); return { path: p, src, data: fs.readFileSync(path.join(dir, src)) }; });
}

module.exports = [
  {
    name: 'io', about: 'open/read /dev/zero, a missing file (ERR 70), mount and ns, /dev/proc, a server\'s not found (ERR 70)',
    args: ['--cycles', '90000000', '--input', BOOT +
      'q^/dev/zero^ 1 open here @ 5 read . ioerr .\\r' +
      'q^/dev/nothere^ 1 open\\rioerr .\\r' +
      'q^/z^ q^zero^ mount ns\\rq^/z^ 1 open here @ 3 read .\\r' +
      'q^/dev/proc^ 1 open here @ 100 read .\\r' +
      'q^/dev/proc/z^ 1 open\\rioerr .\\r'],
    expect: ['read . ioerr .\n' + num(5) + num(0) + '\n',
      '!IO ERR!', '/> ioerr .\n' + num(0x70) + '\n',
      '/z -> zero\n', 'read .\n' + num(3) + '\n',
      /\/dev\/proc\^ 1 open here @ 100 read \.\n 00[1-9A-F][0-9A-F]\n/,
      '!IO ERR!', '/> ioerr .\n' + num(0x70) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'tasks', about: 'another shell: ps, Ctrl-] to switch the console, kill; Ctrl-C breaks a read',
    args: ['--cycles', '150000000', '--input', BOOT + 'shell\\r' + W(1) + 'ps\\r' + W(1) + '\\x1dB' + W(1) + '\\r1 2 + .\\r' + W(1) + '\\x1d1' + W(1) +
      '\\r11 kill\\r' + W(1) + 'ps\\rcat\\r' + W(1) + '\\x03' + W(1) + '3 4 + .\\r'],   // (kill flags B: it ends when it next runs)
    expect: ['/> ps\n0 R -\n1 R 0 *\nB W 1\n', '[B]', '/> 1 2 + .\n' + num(3), '[1]',
      '/> ps\n0 R -\n1 R 0 *\nC D -\n', '/> cat\n', '!BREAK!', '/> 3 4 + .\n' + num(7)],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'sound', about: 'sndtest plays in a task of its own while the shell runs; sndstop ends it; the bell (Ctrl-G) first',
    args: ['--cycles', '90000000', '--input', BOOT + '\\x07\\r' + P + 'sndtest\\r' + P + 'ps\\r' + W(2) + 'sndstop\\rps\\r'],
    expect: ['/> ps\n0 R -\n1 R 0 *\nB W E\n', '/> sndstop\n', '/> ps\n0 R -\n1 R 0 *\nC D -\n'],  // (B W: it sleeps between notes)
    check: (out, report) => {
      const m = /--- YM2151 key-ons: (\d+) \((.*)\)/.exec(report);
      if (!m || +m[1] < 5) return 'the tune played ' + (m ? m[1] : 'no') + ' notes';
      if (!/^ch 7 at/.test(m[2])) return 'no bell (the first key-on, on channel 7)';
    },
  },
  {
    name: 'sound-lib', about: 'the YM2151 library (/dev/snd): a C program (snd.h) claims channels 0-3 in the background (patches, notes, volumes, a bend, drums; the registers read back), a second one finds them busy; HyForth\'s patch and note, commands through ywrite (a channel, its volume and speakers); the chip\'s registers: key codes, a carrier\'s level with the volume, the rest as written',
    sd: [{ dev: 0, label: 'SOUND', hfs: v => {
      v.mkdir('bin');
      v.put('bin/tones.hyx', fs.readFileSync(path.join(__dirname, '../../programs/c/bin/tones.hyx')));
    } }],
    args: ['--cycles', '110000000', '--ym-dump', '--input', W(3) + ['tones 29 &\r', 'tones\rstatus .\rcat /env/status\r', '$B wait\r' + W(8),
      '0 5 patch 72 5 note\r', '$0205 ywrite drop $0640 ywrite drop $0702 ywrite drop\r'].join(W(1))],
    expect: ['0:/> tones 29 &\n[B]\n', '0:/> tones\ntones: channels 0-3: Device or resource busy\n', '0:/> status .\n' + num(1) + '\n0:/> cat /env/status\nbusy\n',
      '0:/> $B wait\ntones: patch 29, $20 FA, $28 4C\n'],
    forbid: ['!IO ERR!'],
    check: (out, report) => {
      const m = /--- YM2151 key-ons: (\d+)/.exec(report);
      if (!m || +m[1] < 20) return 'only ' + (m ? m[1] : 'no') + ' key-ons';
      if (/lost on the chip/.test(report)) return 'writes lost on the chip';
      for (const [re, what] of [
        [/^20: FA FA FA FC C0 84 C0 C0 4C 44 48 41 00 4E 00 00$/m, 'patch 29 on 0-2, a drum on 3; 5: patch 0 on the right, C5'],
        [/^60: 10 10 10 \w\w \w\w 35 \w\w \w\w 16 16 16 \w\w \w\w 18 /m, 'patch 0\'s M1 and M2 (not carriers) as written'],
        [/^70: 21 21 21 \w\w \w\w 27 \w\w \w\w 00 00 00 \w\w \w\w 10 /m, 'its C1 and C2 (carriers) 16 steps down at volume 64']])
        if (!re.test(report)) return 'YM2151 registers: not ' + what;
    },
  },
  {
    name: 'songs', about: 'the song player (ZSM): play with a loop count, a song run by its name, played in time (the key-ons 36 ticks of 60 Hz apart); play ... 0 & (forever) in the background, its channel claimed (tones finds it busy), Ctrl-C at wait ends it (status 130, its channel keyed off); a script is no song; C\'s snd_play, stopped with hy_kill',
    sd: [{ dev: 0, label: 'SONGS', hfs: v => {
      v.mkdir('bin');
      for (const p of ['tones', 'jukebox']) v.put('bin/' + p + '.hyx', fs.readFileSync(path.join(__dirname, '../../programs/c/bin/' + p + '.hyx')));
      v.put('t.zsm', zsmSong());
      v.put('x.hys', Buffer.from('1 .\r\n'));
    } }],
    args: ['--cycles', '110000000', '--ym-log', '--ym-dump', '--input', W(3) + ['play t.zsm 2\\r' + W(6), 'status .\\r', 't\\r' + W(3),
      'play t.zsm 0 &\\r', 'tones\\r' + W(1), '$B wait\\r' + W(3) + '\\x03' + W(1), 'status .\\rcat /env/status\\r', 'play x.hys\\r', 'jukebox t.zsm 1\\r' + W(4)].join(W(1))],
    expect: ['0:/> play t.zsm 2\n\n0:/> status .\n' + num(0) + '\n', '0:/> t\n\n0:/> play t.zsm 0 &\n[B]\n',     // (t: t.zsm, by its name)
      '0:/> tones\ntones: channels 0-3: Device or resource busy\n', '0:/> status .\n' + num(130) + '\n0:/> cat /env/status\ninterrupt\n',
      '0:/> play x.hys\n\n !IO ERR! ', '0:/> jukebox t.zsm 1\n1\nstopped: 137\n'],     // (C: snd_play, hy_kill)
    check: (out, report) => {
      const on =[...report.matchAll(/ch (\d) at cycle (\d+)/g)].map(m => [+m[1], +m[2]]);
      if (on.length < 8 || on.some(k => k[0] !== 0)) return 'key-ons: ' + on.length + ', on channel 0 only?';
      for (let k = 1; k < 4; k++) {                                // The first play: 4 notes, 0.6 s (2,147,727 cycles) apart
        const gap = on[k][1] - on[k - 1][1];
        if (Math.abs(gap - 2147727) > 25000) return 'key-on ' + k + ' came ' + gap + ' cycles after the last, not 0.6 s';
      }
      if (on[4][1] - on[3][1] < 2147727) return 'the second play began before the first ended';
      if (!/^00: 00 00 00 00 00 00 00 00 00 /m.test(report)) return 'channel 0 not keyed off at the end ($08)';
      if (!/^10: \w\w \w\w C[67] /m.test(report)) return 'no sound clock: timer B ($12) not at 58 or 59 units (60 Hz)';
    },
  },
  {
    name: 'rom', about: '/rom, the files in the paged ROM, with no card: its listing (text and stat records: ls -l), a file read, a program run by its name from /rom/bin (the PATH fallback), cd into it, an exit status; read-only (write, remove, create refused), a missing name, a file for cd; the ROM disk by its name, /sd/x and /dev/sd/x (x: not a digit, so SPI devices 8-f keep theirs), the bind in ns, /sd/8 no disk',
    args: ['--cycles', '90000000', '--input', BOOT + ['ls /rom\\r', 'hello a b\\r' + W(2), 'cd /rom/bin\\rpwd\\rls -l\\r', 'code 3\\r' + P + 'status .\\r',
      'cat /rom/nope\\r', 'rm /rom/README\\r', 'echo x > /rom/x\\r', 'cd /\\rcd /rom/README\\r', 'ns\\r', 'ls /sd/x\\r', 'cat /dev/sd/x/ctl\\r', 'ls /sd/8\\r'].join(P)],
    expect: ['/> ls /rom\nREADME 984\nbin/\nsongs/\n', '/> hello a b\nHello from C on the Hydra-16!\n2 arguments: [a] [b]\n',
      '/> cd /rom/bin\n\n/rom/bin> pwd\n/rom/bin\n/rom/bin> ls -l\ncode.hyx 2158 2000-01-01 00:00:00\n', 'scom.zsm 838 2000-01-01 00:00:00\n',
      '/rom/bin> code 3\n\n/rom/bin> status .\n' + num(3) + '\n', '/rom/bin> cat /rom/nope\n\n !IO ERR! not found\n', '/rom/bin> rm /rom/README\n\n !IO ERR! not opened for that\n',
      '/rom/bin> echo x > /rom/x\n\n !IO ERR! not opened for that\n', '/> cd /rom/README\n\n !IO ERR! ', '/> ns\n/sd -> hfs\n/rom = /sd/x\n', '/> ls /sd/x\nREADME 984\nbin/\nsongs/\n',
      '/> cat /dev/sd/x/ctl\nrom 4 MB 8192 blocks\nhydrafs label=ROM\n', '/> ls /sd/8\n\n !IO ERR! not found\n'],
  },
  {
    name: 'ram-areas', about: 'the RAM disk\'s areas and the program caches: the shell\'s area (/ram/1) its own, a pipeline stage (a task it started) using it, another task\'s area and a name that isn\'t one refused (not allowed); programs by name from the shell\'s cache (/ram/1/bin) and the shared one (/ram/s/bin), a pipeline stage finding the shell\'s cache, the current directory first',
    sd: [{ dev: 0, label: 'AREAS', hfs: v => { v.put('c8.hys', Buffer.from('7 .\r\n')); } }],
    args: ['--cycles', '200000000', '--input', BOOT + ['echo hi > /ram/1/x\\r', 'cat /ram/1/x | cat\\r', 'mkdir /ram/2\\r', 'ls /ram/2\\r', 'mkdir /ram/zz\\r',
      'echo no > /ram/3\\r', 'cp /rom/bin/code.hyx /ram/1/bin/c9.hyx\\r', 'c9 4\\r' + P + 'status .\\r', 'cp /rom/bin/hello.hyx /ram/1/bin/hh.hyx\\r',
      'hh a | cat\\r' + W(2), 'cp /rom/bin/code.hyx /ram/s/bin/c7.hyx\\r', 'c7 5\\r' + P + 'status .\\r', 'cp /rom/bin/code.hyx /ram/s/bin/c8.hyx\\r', 'c8\\r'].join(P) + P],
    expect: ['cat /ram/1/x | cat\nhi\n', 'mkdir /ram/2\n\n !IO ERR! not allowed\n', 'ls /ram/2\n\n !IO ERR! not allowed\n', 'mkdir /ram/zz\n\n !IO ERR! not allowed\n',
      'echo no > /ram/3\n\n !IO ERR! not allowed\n', '> status .\n' + num(4) + '\n', 'hh a | cat\nHello from C on the Hydra-16!\n1 arguments: [a]\n',
      '> status .\n' + num(5) + '\n', '> c8\n' + num(7) + '\n'],
    forbid: ['!UNK WORD!'],
  },
  {
    name: 'ram-area-end', about: 'an area removed as its task ends (TASK_ORPHANS, TASK_AREA_END, HFS_AREA_END): a script, a task of its own (B), makes /ram/b with directories in directories and files, and leaves one open; when it ends, its area is gone, all its space back, before the shell\'s prompt; the shell\'s area stays',
    sd: [{ dev: 0, label: 'AREAEND', hfs: v => {
      v.put('mk.hys', Buffer.from('$FFF0 c@ .\r\nmkdir /ram/b\r\nmkdir /ram/b/d\r\nmkdir /ram/b/d/e\r\necho hi > /ram/b/d/e/f\r\necho yo > /ram/b/g\r\n' +
        'q^/ram/b/g^ 1 open drop\r\nls /ram\r\n'));
    } }],
    args: ['--cycles', '150000000', '--input', BOOT + ['cat /dev/sd/r/ctl\\r', 'mk\\r', 'ls /ram\\r', 'cat /dev/sd/r/ctl\\r'].join(P) + P],
    expect: ['free 244 KB of 252 KB\n', '> mk\n' + num(11) + '1/\nb/\n', '> ls /ram\n1/\n', 'free 244 KB of 252 KB\n'],
    forbid: ['!IO ERR!', '!UNK WORD!'],
  },
  {
    name: 'ram-area-busy', about: 'an area kept while a task its task started has a file in it open: a script (task B) makes /ram/b and starts upper in the background, writing to /ram/b/out; when the script ends, its area stays; once upper has ended, the next task B\'s end removes it',
    sd: [{ dev: 0, label: 'AREABUSY', hfs: v => {
      v.put('mk.hys', Buffer.from('mkdir /ram/b\r\nupper > /ram/b/out &\r\n'));
      v.put('mk2.hys', Buffer.from('$FFF0 c@ .\r\necho z > /ram/b/z\r\n'));
    } }],
    args: ['--cycles', '200000000', '--input', BOOT + ['mk\\r', 'ls /ram\\r', 'mk2\\r', 'ls /ram\\r'].join(P) + P],
    expect: ['> ls /ram\n1/\nb/\n', '> mk2\n' + num(11), '> ls /ram\n1/\n'],
    forbid: ['!IO ERR!', '!UNK WORD!'],
  },
  {
    name: 'rom-copy', about: 'the ROM disk read back on the machine: every file in /rom (romfs.txt) copied to a card is its source, byte for byte, the ones across a bank boundary too (sd.s: SD_ROM_READ)',
    sd: [{ dev: 0, label: 'COPIES', hfs: () => {} }],
    args: ['--cycles', '400000000', '--input', BOOT + romFiles().map(f => 'cp /rom/' + f.path + ' ' + path.basename(f.path) + '\\r').join(P) + P],
    expect: ['> cp /rom/songs/test.zsm test.zsm\n'],
    forbid: ['!IO ERR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const v = new hydrafs.Volume(files.sd);
      try {
        for (const f of romFiles()) {
          const e = v.walk(path.basename(f.path));
          if (!e) return '/rom/' + f.path + ' wasn\'t copied';
          if (!v.read(e).equals(f.data)) return '/rom/' + f.path + ' doesn\'t read back as ' + f.src;
        }
      } finally { v.close(); }
    },
  },
  {
    name: 'rtc', about: 'a DS1747 in U7 (its battery flat): found at boot, the clock set from it; /dev/time reads it; setting the time sets it too (the day of the week)',
    args: ['--cycles', '60000000', '--rtc', '2026-09-30T14:05:00', '--rtc-battery-low', '--input', W(3) + 'cat /dev/time\\r' + P +
      'echo 2027-01-02 03:04:05 > /dev/time\\r' + P + 'cat /dev/time\\r'],
    expect: [/\nclock 2026-09-30 14:05:0[0-2] battery low\n/, /\/> cat \/dev\/time\n2026-09-30 14:05:0[1-3]\n/,
      /\/> cat \/dev\/time\n2027-01-02 03:04:0[5-7]\n/],
    check: (out, report) => {
      if (!/--- DS1747: 2027-01-02 03:04:\d\d day 7\n/.test(report)) return 'the DS1747 wasn\'t set (Saturday: day 7): ' + (/--- DS1747.*/.exec(report) || ['none'])[0];
    },
  },
  {
    name: 'rtc-unset', about: 'a DS1747 never set (junk in its registers): "no clock" at boot; setting the time sets it, and it\'s found then',
    args: ['--cycles', '60000000', '--rtc', 'unset', '--input', W(3) + 'echo 2027-01-02 03:04:05 > /dev/time\\r' + W(2) + 'cat /dev/time\\r'],
    expect: ['\nno clock\n', /\/> cat \/dev\/time\n2027-01-02 03:04:0[6-9]\n/],
    check: (out, report) => {
      if (!/--- DS1747: 2027-01-02 03:04:\d\d day 7\n/.test(report)) return 'the DS1747 isn\'t running from the time set: ' + (/--- DS1747.*/.exec(report) || ['none'])[0];
    },
  },
  {
    name: 'rtc-none', about: 'no DS1747 (a plain HM628512 in U7): "no clock" at boot, and the clock is set and read as before',
    args: ['--cycles', '40000000', '--input', BOOT + 'echo 2027-01-02 03:04:05 > /dev/time\\r' + P + 'cat /dev/time\\r'],
    expect: ['\nno clock\n\nHyForth', /\/> cat \/dev\/time\n2027-01-02 03:04:0[5-9]\n/],
  },
  {
    name: 'semaphores', about: 'semaphores: counts (acquire?, release), a mutex (only its holder releases it), bad ones, a wait another task ends (a pipeline stage), freed and released when a task ends, freed while waited for, Ctrl-C in a wait',
    args: ['--cycles', '120000000', '--input', BOOT + ['2 sem .\\r', '1 acquire? . 1 acquire? . 1 acquire? .\\r', '1 release 1 acquire? .\\r',
      'mutex .\\r', '2 acquire 2 release 2 release\\rioerr .\\r', '9 acquire\\rioerr .\\r', '0 sem .\\r',
      '200 sleep 3 release | 3 acquire 7 .\\r' + W(1), '4 sem . | cat\\r', '4 sem .\\r', '2 acquire 100 sleep | 2 acquire 8 .\\r' + W(1),
      '2 acquire? .\\r', '100 sleep 3 -sem | 3 acquire\\rioerr .\\r', '0 sem .\\r', '3 acquire\\r' + W(1) + '\\x03' + W(1) + '1 2 + .\\r'].join(W(1))],
    expect: ['/> 2 sem .\n' + num(1) + '\n', '/> 1 acquire? . 1 acquire? . 1 acquire? .\n' + num(0xFFFF) + num(0xFFFF) + num(0) + '\n',
      '/> 1 release 1 acquire? .\n' + num(0xFFFF) + '\n', '/> mutex .\n' + num(2) + '\n',
      '/> 2 acquire 2 release 2 release\n\n !IO ERR! ', '/> ioerr .\n' + num(0x63) + '\n',       // (Released already: not held)
      '/> 9 acquire\n\n !IO ERR! ', '/> ioerr .\n' + num(0x60) + '\n', '/> 0 sem .\n' + num(3) + '\n',
      '/> 200 sleep 3 release | 3 acquire 7 .\n' + num(7) + '\n',                              // (The shell waits for the stage)
      '/> 4 sem . | cat\n' + num(4) + '\n', '/> 4 sem .\n' + num(4) + '\n',                     // (The stage's: freed as it ended)
      '/> 2 acquire 100 sleep | 2 acquire 8 .\n' + num(8) + '\n',                             // (Released as its holder ended)
      '/> 2 acquire? .\n' + num(0) + '\n',                                                    // (The shell holds it now)
      '/> 100 sleep 3 -sem | 3 acquire\n\n !IO ERR! ', '/> ioerr .\n' + num(0x60) + '\n',       // (Freed while waited for)
      '/> 0 sem .\n' + num(3) + '\n', '/> 3 acquire\n\n !BREAK!\n', '/> 1 2 + .\n' + num(3) + '\n'],
  },
  {
    name: 'serial', about: 'serial settings: 9600 8N1 at boot; stty (/dev/ser/ctl), a refused format, IO_CTL rate and format; the ACIA\'s registers',
    args: ['--cycles', '60000000', '--input', BOOT + 'stty?\\rq^b19200 l7 pe s2^ stty stty?\\rq^l8 pe s2^ stty\\rioerr .\\r' +
      '1 2 6 ioctl stty?\\r1 3 11 ioctl stty?\\r'],
    expect: ['/> stty?\nb9600 l8 pn s1\n', 'stty stty?\nb19200 l7 pe s2\n', '!IO ERR!', '/> ioerr .\n' + num(0x78) + '\n',
      '6 ioctl stty?\nb4800 l7 pe s2\n', '11 ioctl stty?\nb4800 l8 pe s1\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {                                   // 4800 ($0C), 8 bits, 1 stop; even parity ($60) on DTR + IRQs ($05)
      if (!/ACIA control 1C command 65,/.test(report)) return 'the ACIA isn\'t at 4800 8E1: ' + (/ACIA control \w+ command \w+/.exec(report) || ['?'])[0];
    },
  },
  {
    name: 'paste', about: 'serial input at the full line rate (--paste): 1000 characters into wc at 57600, none lost',
    args: ['--cycles', '60000000', '--paste', '--input', BOOT + 'q^b57600^ stty\\r' + P + 'wc . . .\\r' +
      Array(20).fill('the quick brown fox jumps over the lazy dog 0123 \\r').join('') + '\\x04'],
    expect: [' 03E8 00C8 0014\n'],                                  // 1000 characters, 200 words, 20 lines
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {
      const m = /ACIA: (\d+) received byte\(s\) lost/.exec(report);
      if (!m || +m[1]) return (m ? m[1] : '?') + ' received byte(s) lost';
    },
  },
  {
    name: 'fast-output', about: 'console output at 115200: words (3.5K characters) in under a second (the fast paths), paced by timer 2 with at least 2 idle bits between characters (SER_PACE_GAP, and the interrupt\'s time)',
    args: ['--cycles', '40000000', '--mark', '/> words', '--mark', '/> ', '--input', BOOT + 'q^b115200^ stty\\r' + P + 'words\\r'],
    expect: ['/> words\n', ': Acls '],
    check: (out, report) => {                                   // (The wire alone, paced: about 1.6M cycles; the old IO path: 3.6M more)
      const at = +/mark: "\/> words" at cycle (\d+)/.exec(report)[1];
      const took = [...report.matchAll(/mark: "\/> " at cycle (\d+)/g)].map(m => +m[1]).find(c => c > at) - at;
      if (!(took < 3000000)) return 'words took ' + took + ' cycles at 115200, not under 3M';
      const gap = +(/shortest idle between characters sent: ([\d.]+) bits/.exec(report) || [])[1];
      if (!(gap >= 2)) return 'the line idled only ' + gap + ' bits between characters at 115200, not 2 or more';
    },
  },
  {
    name: 'serial-unpaced', about: 'from 115200 (paced by timer 2) back to 9600: sending by the TDRE interrupt again, and the console works',
    args: ['--cycles', '40000000', '--input', BOOT + 'q^b115200^ stty\\r' + P + 'words\\r' + W(2) + 'q^b9600^ stty\\r' + P + '1 2 + .\\r' + P + 'words\\r'],
    expect: ['/> q^b9600^ stty\n', '/> 1 2 + .\n' + num(3) + '\n', '/> words\n', ': Acls '],
  },
  {
    name: 'irqs-off', about: 'no long stretch with IRQs off after boot (tasks starting and ending, a pipeline, sound, files): a serial byte can\'t wait long',
    args: ['--cycles', '90000000', '--input', BOOT + 'words | wc . . .\\rq^/dev/zero^ 1 open here @ 16 read .\\r3 close\\r' +
      'sndtest\\r' + W(2) + 'sndstop\\rshell .\\r' + P + 'mmtest\\r'],
    expect: ['MMU test: ok'],
    check: (out, report) => {
      const m = /longest with IRQs off.*?at cycle\): (\d+): (\S+) -> (\S+)/.exec(report);
      if (!m) return 'no IRQs-off report';
      if (+m[1] > 5000) return 'IRQs were off for ' + m[1] + ' cycles (from ' + m[2] + ' to ' + m[3] + ')';
    },
  },
];
