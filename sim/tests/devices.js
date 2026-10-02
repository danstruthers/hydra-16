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
      'mount zero /z\\rns\\rq^/z^ 1 open here @ 3 read .\\r' +
      'q^/dev/proc^ 1 open here @ 100 read .\\r' +
      'q^/dev/proc/z^ 1 open\\rioerr .\\r'],
    expect: ['read . ioerr .\n' + num(5) + num(0) + '\n',
      '!IO ERR!', '/ram> ioerr .\n' + num(0x70) + '\n',
      'mount zero /z\n', 'read .\n' + num(3) + '\n',
      /\/dev\/proc\^ 1 open here @ 100 read \.\n 00[1-9A-F][0-9A-F]\n/,
      '!IO ERR!', '/ram> ioerr .\n' + num(0x70) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'tasks', about: 'another shell: ps, Ctrl-] to switch the console, kill; Ctrl-C breaks a read',
    args: ['--cycles', '150000000', '--input', BOOT + 'shell\\r' + W(1) + 'ps\\r' + W(1) + '\\x1dB' + W(1) + '\\r1 2 + .\\r' + W(1) + '\\x1d1' + W(1) +
      '\\r11 kill\\r' + W(1) + 'ps\\rcat\\r' + W(1) + '\\x03' + W(1) + '3 4 + .\\r'],   // (kill flags B: it ends when it next runs)
    expect: ['/ram> ps\n0 R -\n1 R 0 *\nB W 1\n', '[B]', '/ram> 1 2 + .\n' + num(3), '[1]',
      '/ram> ps\n0 R -\n1 R 0 *\nC D -\n', '/ram> cat\n', '!BREAK!', '/ram> 3 4 + .\n' + num(7)],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
  },
  {
    name: 'send', about: 'send (/proc/N/cmd): a line for another shell, one this shell started, waiting at its prompt in the background: woken (no break message), it runs the line; its cwd changed so; a line for this shell itself, run at its next prompt; no such task; the other way refused (not allowed)',
    args: ['--cycles', '150000000', '--input', BOOT + 'shell\\r' + W(1) + 'send b 5 6 + .\\r' + W(2) + 'send b cd /rom\\r' + W(2) + 'cat /proc/b/cwd\\r' + W(1) +
      'send 1 7 8 + .\\r' + W(2) + 'send 9 1\\r' + W(1) + '\\x1dB' + W(1) + '\\rsend 1 2 2 + .\\r' + W(1)],
    expect: ['/ram> 5 6 + .\n', num(11), '/ram> cd /rom\n', 'cat /proc/b/cwd\n/rom\n', '/ram> 7 8 + .\n' + num(15), 'send 9 1\n\n !IO ERR! not found\n',
      '[B]', 'send 1 2 2 + .\n\n !IO ERR! not allowed\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!', '!BREAK!'],
  },
  {
    name: 'sound', about: 'sndtest plays in a task of its own while the shell runs; sndstop ends it; the bell (Ctrl-G) first',
    args: ['--cycles', '90000000', '--input', BOOT + '\\x07\\r' + P + 'sndtest\\r' + P + 'ps\\r' + W(2) + 'sndstop\\rps\\r'],
    expect: ['/ram> ps\n0 R -\n1 R 0 *\nB W E\n', '/ram> sndstop\n', '/ram> ps\n0 R -\n1 R 0 *\nC D -\n'],  // (B W: it sleeps between notes)
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
      if (!/^10: \w\w \w\w C[5-7] /m.test(report)) return 'no sound clock: timer B ($12) not at 57-59 units (60 Hz: its short or long period)';
    },
  },
  {
    name: 'rom', about: '/rom, the files in the paged ROM, with no card: its listing (text and stat records: ls -l), a file read, a program run by its name from /rom/bin (through /bin), cd into it, an exit status; read-only (write, remove, create refused), a missing name, a file for cd; the ROM disk as a block device, /dev/sd/x (x: not a digit, so SPI devices 8-f keep theirs), and not under /sd (only through a mount\'s spec: mount hfs /rom x, in ns), /sd/8 no disk; /rom/boot.hys run at boot with no card',
    args: ['--cycles', '90000000', '--input', BOOT + ['ls /rom\\r', 'hello a b\\r' + W(2), 'cd /rom/bin\\rpwd\\rls -l\\r', 'code 3\\r' + P + 'status .\\r',
      'cat /rom/nope\\r', 'rm /rom/README\\r', 'echo x > /rom/x\\r', 'cd /\\rcd /rom/README\\r', 'ns\\r', 'ls /sd/x\\r', 'cat /dev/sd/x/ctl\\r', 'ls /sd/8\\r'].join(P)],
    expect: ['No card: /ram keeps your files until a reset.', '/ram> ls /rom\nREADME 984\nbin/\nboot.hys 83\nlib/\nsongs/\n', '/ram> hello a b\nHello from C on the Hydra-16!\n2 arguments: [a] [b]\n',
      '/ram> cd /rom/bin\n\n/rom/bin> pwd\n/rom/bin\n/rom/bin> ls -l\ncode.hyx 2158 2000-01-01 00:00:00\n', 'scom.zsm 838 2000-01-01 00:00:00\n',
      '/rom/bin> code 3\n\n/rom/bin> status .\n' + num(3) + '\n', '/rom/bin> cat /rom/nope\n\n !IO ERR! not found\n', '/rom/bin> rm /rom/README\n\n !IO ERR! not opened for that\n',
      '/rom/bin> echo x > /rom/x\n\n !IO ERR! not opened for that\n', '/> cd /rom/README\n\n !IO ERR! ', '/> ns\nmount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /rom/lib /lib\nmount hfs /ram r/1\n', '/> ls /sd/x\n\n !IO ERR! not found\n',
      '/> cat /dev/sd/x/ctl\nrom 4 MB 8192 blocks\nhydrafs label=ROM\n', '/> ls /sd/8\n\n !IO ERR! not found\n'],
  },
  {
    name: 'ram-own', about: 'each shell\'s own area at /ram: the boot shell\'s is r/1 (mount hfs /ram r/1; the whole RAM disk at /a), the shared disk is /sram; a shell started with shell gets its own (/a/b, in its ns), so a copy into /bin goes to its own cache, and its files aren\'t the boot shell\'s; when it ends, its area goes',
    args: ['--cycles', '150000000', '--input', BOOT + 'mount hfs /a r\\r' + 'echo one > /ram/f\\r' + W(1) + 'shell\\r' + W(2) + '\\x1dB' + W(1) + ['\\rcat /proc/b/ns | wc . . .\\r', 'echo two > /ram/f\\r',
      'cp /rom/bin/code.hyx /bin/cb.hyx\\r', 'ls /a/b/bin\\r', 'cat /a/1/f\\r', 'cb 6\\r' + P + 'status .\\r'].join(W(1)) + W(1) + '\\x1d1' + W(1) +
      ['\\r11 kill\\r' + W(2), 'cat /ram/f\\r', 'ls /ram/bin\\r', 'ls /a\\r', 'ls /sram\\r'].join(W(1)) + W(1)],
    expect: ['> cat /proc/b/ns | wc . . .\n', '> ls /a/b/bin\ncb.hyx 2158\n', '> cat /a/1/f\none\n', '> status .\n' + num(6) + '\n',
      '/ram> cat /ram/f\none\n', '/ram> ls /ram/bin\n\n', '/ram> ls /a\n1/\n\n', '/ram> ls /sram\nbin/\nlib/\n'],
    forbid: ['!UNK WORD!', '!IO ERR!'],
  },
  {
    name: 'ram-areas', about: 'the RAM disk\'s areas and the program caches: the shell\'s area (/ram) its own, a pipeline stage (a task it started) using it, another task\'s area and a name that isn\'t one refused (not allowed); programs by name from the shell\'s cache (/ram/bin) and the shared one (/sram/bin), a pipeline stage finding the shell\'s cache, the current directory first; /dev/ram (raw RAM) refused: it\'s task 0\'s',
    sd: [{ dev: 0, label: 'AREAS', hfs: v => { v.put('c8.hys', Buffer.from('7 .\r\n')); } }],
    args: ['--cycles', '200000000', '--input', BOOT + 'mount hfs /a r\\r' + ['echo hi > /ram/x\\r', 'cat /ram/x | cat\\r', 'mkdir /a/2\\r', 'ls /a/2\\r', 'mkdir /a/zz\\r',
      'echo no > /a/3\\r', 'cp /rom/bin/code.hyx /ram/bin/c9.hyx\\r', 'c9 4\\r' + P + 'status .\\r', 'cp /rom/bin/hello.hyx /ram/bin/hh.hyx\\r',
      'hh a | cat\\r' + W(2), 'cp /rom/bin/code.hyx /sram/bin/c7.hyx\\r', 'c7 5\\r' + P + 'status .\\r', 'cp /rom/bin/code.hyx /sram/bin/c8.hyx\\r', 'c8\\r', 'cat /dev/ram\\r'].join(P) + P],
    expect: ['cat /ram/x | cat\nhi\n', 'mkdir /a/2\n\n !IO ERR! not allowed\n', 'ls /a/2\n\n !IO ERR! not allowed\n', 'mkdir /a/zz\n\n !IO ERR! not allowed\n',
      'echo no > /a/3\n\n !IO ERR! not allowed\n', '> status .\n' + num(4) + '\n', 'hh a | cat\nHello from C on the Hydra-16!\n1 arguments: [a]\n',
      '> status .\n' + num(5) + '\n', '> c8\n' + num(7) + '\n', '> cat /dev/ram\n\n !IO ERR! not allowed\n'],
    forbid: ['!UNK WORD!'],
  },
  {
    name: 'ram-area-end', about: 'an area removed as its task ends (TASK_ORPHANS, TASK_AREA_END, HFS_AREA_END): a script, a task of its own (B), makes /a/b with directories in directories and files, and leaves one open; when it ends, its area is gone, all its space back, before the shell\'s prompt; the shell\'s area stays',
    sd: [{ dev: 0, label: 'AREAEND', hfs: v => {
      v.put('mk.hys', Buffer.from('$FFF0 c@ .\r\nmkdir /a/b\r\nmkdir /a/b/d\r\nmkdir /a/b/d/e\r\necho hi > /a/b/d/e/f\r\necho yo > /a/b/g\r\n' +
        'q^/a/b/g^ 1 open drop\r\nls /a\r\n'));
    } }],
    args: ['--cycles', '150000000', '--input', BOOT + 'mount hfs /a r\\r' + ['cat /dev/sd/r/ctl\\r', 'mk\\r', 'ls /a\\r', 'cat /dev/sd/r/ctl\\r'].join(P) + P],
    expect: ['free 244 KB of 252 KB\n', '> mk\n' + num(11) + '1/\nb/\n', '> ls /a\n1/\n', 'free 244 KB of 252 KB\n'],
    forbid: ['!IO ERR!', '!UNK WORD!'],
  },
  {
    name: 'ram-area-busy', about: 'an area kept while a task its task started has a file in it open: a script (task B) makes /a/b and starts upper in the background, reading the console (so it waits) and writing to /a/b/out; when the script ends, its area stays, and upper\'s owner is the shell; once upper is killed, the next task B\'s end removes it',
    sd: [{ dev: 0, label: 'AREABUSY', hfs: v => {
      v.put('mk.hys', Buffer.from('mkdir /a/b\r\nupper < /dev/cons > /a/b/out &\r\n'));
      v.put('mk2.hys', Buffer.from('$FFF0 c@ .\r\necho z > /a/b/z\r\n'));
    } }],
    args: ['--cycles', '200000000', '--input', BOOT + 'mount hfs /a r\\r' + ['mk\\r', 'ls /a\\r', 'cat /dev/proc\\r', 'q^/dev/proc/a/ctl^ q^kill^ ctl\\r', 'mk2\\r', 'ls /a\\r'].join(P) + P],
    expect: ['> ls /a\n1/\nb/\n', '\nA W 1\n', '> mk2\n' + num(11), '> ls /a\n1/\n'],
    forbid: ['!IO ERR!', '!UNK WORD!'],
  },
  {
    name: 'rom-none', about: 'a paged ROM image with no ROM disk (banks 0 and 1 only, no partition table): no /rom mounted, so no /rom/boot.hys; the rest as usual',
    pagedRom: image => {
      const paged = Buffer.from(image.subarray(0, 2 * 0x4000));
      paged.fill(0, 0x2000, 0x2200);                            // (Bank 0's $A000 half is at $2000: block 0, the table)
      return paged;
    },
    args: ['--cycles', '60000000', '--input', BOOT + ['ns\\r', 'ls /rom\\r', '1 2 + .\\r'].join(P)],
    expect: ['/ram> ns\nmount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /sram s\nmount hfs /ram r/1\n', '/ram> ls /rom\n\n !IO ERR! not found\n', '/ram> 1 2 + .\n' + num(3)],
    forbid: ['No card:'],
  },
  {
    name: 'ns-unions', about: 'namespaces as Plan 9\'s: a union of two binds (bind, bind -a), a name in its first member and one only in its second, ns\'s lines; its listing (ls, ls -l: each member\'s in turn; a directory only the second has), a file read through it not run on into the next member; a create with no -c member refused, then one to the -bc member; a pipeline stage (a task the shell started) sees the union; a member unmounted (unmount new old), a hide, a bad flag, the whole union unmounted',
    args: ['--cycles', '150000000', '--input', BOOT + ['mkdir /ram/b\\r', 'echo hi > /ram/b/x\\r', 'bind /ram/b /u\\r', 'bind -a /rom /u\\r', 'ns\\r', 'ls /u\\r', 'ls -l /u\\r', 'ls /u/songs\\r',
      'cat /u/x\\r', 'ls -l /u/boot.hys\\r', 'echo y > /u/new\\r', 'mkdir /ram/c\\r', 'bind -bc /ram/c /u\\r', 'echo y > /u/new\\r', 'cat /ram/c/new\\r',
      'cat /u/new | cat\\r', 'ns\\r', 'unmount /rom /u\\r', 'ls -l /u/boot.hys\\r', 'hide /u/x\\r', 'cat /u/x\\r', 'cat /u/new\\r', 'mount -z zero /z\\r',
      'unmount /u\\r', 'ns\\r'].join(P) + P],
    expect: ['/ram> ns\nmount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /rom/lib /lib\nmount hfs /ram r/1\nbind /ram/b /u\nbind -a /rom /u\n',
      '/ram> ls /u\nx 4\nREADME 984\nbin/\nboot.hys 83\nlib/\nsongs/\n\n', '/ram> ls -l /u\nx 4 2000-01-01 00:00:0', '\nREADME 984 2000-01-01 00:00:00\n',
      '/ram> ls /u/songs\ntest.zsm 14075\n\n', '/ram> cat /u/x\nhi\n\n/ram> ls -l /u/boot.hys\nboot.hys 83 2000-01-01 00:00:00\n', '/ram> echo y > /u/new\n\n !IO ERR! not opened for that\n', '/ram> cat /ram/c/new\ny\n', '/ram> cat /u/new | cat\ny\n',
      '/ram> ns\nmount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /rom/lib /lib\nmount hfs /ram r/1\nbind -c /ram/c /u\nbind -a /ram/b /u\nbind -a /rom /u\n',
      '/ram> ls -l /u/boot.hys\n\n !IO ERR! not found\n', '/ram> cat /u/x\n\n !IO ERR! not found\n', '/ram> cat /u/new\ny\n', '/ram> mount -z zero /z\n\n !IO ERR! bad name\n',
      '/ram> ns\nmount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /rom/lib /lib\nmount hfs /ram r/1\nhide /u/x\n'],
    forbid: ['!UNK WORD!'],
  },
  {
    name: 'ns-system', about: 'the system namespace: a bind -s from the boot shell, seen by a shell started before it and by a driver (/proc/f/ns); -s from another shell refused (not allowed); a hide over a system entry is the shell\'s own (unmount /rom in shell B: the boot shell still has it)',
    args: ['--cycles', '150000000', '--input', BOOT + 'shell\\r' + W(2) + ['bind -s /rom/songs /x\\r', 'ls /x\\r', 'cat /proc/f/ns | wc . . .\\r'].join(P) + P + '\\x1dB' + W(1) +
      ['\\rls /x\\r', 'bind -s /rom /y\\r', 'unmount /rom\\r', 'ls /rom\\r'].join(W(1)) + W(1) + '\\x1d1' + W(1) + ['\\rls /rom\\r', 'ns\\r'].join(W(1)) + W(1)],
    expect: ['/ram> ls /x\ntest.zsm 14075\n', '> cat /proc/f/ns | wc . . .\n', '> ls /x\ntest.zsm 14075\n', '> bind -s /rom /y\n\n !IO ERR! not allowed\n',
      '> ls /rom\n\n !IO ERR! not found\n', '[1]', '/ram> ls /rom\nREADME 984\n', 'bind -s /rom/songs /x\nmount hfs /ram r/1\n'],
    forbid: ['!UNK WORD!'],
  },
  {
    name: 'ns-newns', about: 'newns: a fresh namespace, as Plan 9\'s: the shell\'s own entries (a bind, a hide, its copy of /bin\'s union with a member added) go, but its /ram, so it sees the system namespace again; newns file: then the file\'s lines',
    args: ['--cycles', '150000000', '--input', BOOT + ['bind /rom/songs /x', 'hide /sram', 'bind -a /rom/songs /bin', 'ns', 'newns', 'ns', 'ls /sram', 'ls /x',
      'echo bind /rom/songs /y > myns', 'newns myns', 'ns', 'ls /y'].join('\\r' + P) + '\\r' + P],
    expect: (() => {
      const sys = 'mount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /rom/lib /lib\n';
      return ['/ram> ns\n' + sys + 'mount hfs /ram r/1\nbind /rom/songs /x\nhide /sram\n', 'bind -a /rom/songs /bin\n', '/ram> newns\n\n/ram> ns\n' + sys + 'mount hfs /ram r/1\n\n',
        '> ls /sram\nbin/\nlib/\n', '> ls /x\n\n !IO ERR! not found\n', '/ram> ns\n' + sys + 'mount hfs /ram r/1\nbind /rom/songs /y\n\n', '> ls /y\ntest.zsm 14075\n'];
    })(),
    forbid: ['!UNK WORD!'],
  },
  {
    name: 'ns-default', about: 'the default namespace: the boot card\'s bin at /bin, /rom/lib/namespace\'s caches before it and the ROM\'s after, then the card\'s lib/namespace (a # comment, a tools directory after them all); a program by its name from the tools and the ROM through /bin; a copy into /bin goes to the shell\'s cache (-c); ls /bin: every member\'s',
    sd: [{ dev: 0, label: 'NSDEF', hfs: v => {
      v.mkdir('tools'); v.put('tools/seven.hys', Buffer.from('7 .\r\n'));
      v.mkdir('lib'); v.put('lib/namespace', Buffer.from('# The card\'s tools\r\nbind -as /sd/0/tools /bin\r\n'));
    } }],
    args: ['--cycles', '150000000', '--input', BOOT + ['ns\\r', 'seven\\r', 'cp /rom/bin/code.hyx /bin/c5.hyx\\r', 'ls /ram/bin\\r', 'c5 4\\r' + P + 'status .\\r', 'ls /bin\\r'].join(P) + P],
    expect: ['bind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /sd/0/bin /bin\nbind -as /rom/bin /bin\nbind -as /sd/0/tools /bin\nbind -cs /ram/lib /lib\n',
      '0:/> seven\n' + num(7) + '\n', '0:/> ls /ram/bin\nc5.hyx 2158\n', '0:/> status .\n' + num(4) + '\n', '0:/> ls /bin\nc5.hyx 2158\ncode.hyx 2158\n', 'upper.hyx 1819\nseven.hys 5\n'],
    forbid: ['!UNK WORD!', '!IO ERR!'],
  },
  {
    name: 'proc', about: '/proc, mounted as Plan 9 has it: the task list, a task\'s ns file (as ns prints it, past 255 bytes: read in pieces from the offset), the other task\'s; ctl for the family only: the shell\'s own, task 0\'s and a driver\'s refused (not allowed)',
    args: ['--cycles', '120000000', '--input', BOOT + [[1, 2, 3, 4].map(n => 'bind /rom/songs /aaaaaaaaaaa' + n + '\\r').join(''), 'ns\\r', 'cat /proc/1/ns\\r',
      'cat /proc/1/ns | cat\\r', 'cat /proc\\r', 'echo fg > /proc/1/ctl\\r', 'q^/proc/0/ctl^ q^kill^ ctl\\r', 'q^/proc/f/ctl^ q^kill^ ctl\\r', 'cat /proc/f/ns\\r'].join(P) + P],
    expect: (() => {
      const ns = 'mount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /rom/lib /lib\nmount hfs /ram r/1\n' + [1, 2, 3, 4].map(n => 'bind /rom/songs /aaaaaaaaaaa' + n + '\n').join('');
      return ['/ram> ns\n' + ns + '\n', '/ram> cat /proc/1/ns\n' + ns + '\n', '/ram> cat /proc/1/ns | cat\n' + ns + '\n', '/ram> cat /proc\n0 ', '\n1 R 0 *\n',
        '/ram> echo fg > /proc/1/ctl\n\n/ram> q^/proc/0/ctl^ q^kill^ ctl\n\n !IO ERR! not allowed\n', '/ram> q^/proc/f/ctl^ q^kill^ ctl\n\n !IO ERR! not allowed\n',
        '/ram> cat /proc/f/ns\nmount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /rom/lib /lib\n\n/ram> '];
    })(),
    forbid: ['!UNK WORD!'],
  },
  {
    name: 'proc-mem', about: '/proc/N/mem and ram: a shell this shell started (task B) stores a number at $6000 and a 16K bank allocation (its bank $2E: banks are top-down, 3 modules), then waits at its prompt; this one reads the number through mem, writes another that B prints, reads the BIOS ROM ($E000: the reset entry, the same on every page) and the I/O space (zeros), a ROM write refused, past the end nothing; ram reads and writes B\'s bank, a bank it hasn\'t got not found, past the last bank nothing; B can\'t read its parent\'s, nor anyone a driver\'s or a free task\'s; a 64K copy of B\'s mem; the copies with IRQs off kept short',
    args: ['--cycles', '300000000', '--input', BOOT + 'shell\\r' + W(1) + 'send b $1234 $6000 !\\r' + W(2) + 'send b $4000 1 halloc $6004 !\\r' + W(2) + 'send b $6004 @ hlock $ABCD swap ! 0 c@ .\\r' + W(2) + [
      'q^/proc/b/mem^ 3 open .', '3 $6000 0 seek 3 here @ 2 read . here @ @ .', '$5678 here @ ! 3 $6002 0 seek 3 here @ 2 write .'].join('\\r' + P) + '\\r' +
      W(1) + 'send b $6002 @ .\\r' + W(2) + [
      '3 $E000 0 seek 3 here @ 2 read . here @ @ $E000 @ = .', '3 $FF00 0 seek 3 here @ 2 read . here @ @ .', '3 $A000 0 seek 3 here @ 2 write .',
      '3 0 1 seek 3 here @ 2 read .', '3 close', 'q^/proc/b/ram^ 3 open .', '3 $C000 5 seek 3 here @ 2 read . here @ @ .',
      '$1357 here @ ! 3 $C002 5 seek 3 here @ 2 write .', '3 $C002 5 seek 3 here @ 2 read . here @ @ .'].join('\\r' + P) + '\\r' + W(1) +
      'send b $6004 @ hlock 2 + @ .\\r' + W(2) + [
      '3 0 0 seek 3 here @ 2 read .', '3 0 $1E seek 3 here @ 2 read .', '3 close', 'q^/proc/9/mem^ 1 open .', 'cat /proc/f/mem'].join('\\r' + P) + '\\r' + P +
      'send b q^/proc/1/mem^ 1 open .\\r' + W(2) + 'cp /proc/b/mem /sram/core\\r' + P + 'ls /sram\\r' + P],
    expect: ['$ABCD swap ! 0 c@ .\n' + num(0x2E) + '\n', '> q^/proc/b/mem^ 3 open .\n' + num(3), '> 3 $6000 0 seek 3 here @ 2 read . here @ @ .\n' + num(2) + num(0x1234),
      '> $5678 here @ ! 3 $6002 0 seek 3 here @ 2 write .\n' + num(2), '> $6002 @ .\n', num(0x5678),
      '> 3 $E000 0 seek 3 here @ 2 read . here @ @ $E000 @ = .\n' + num(2) + num(0xFFFF), '> 3 $FF00 0 seek 3 here @ 2 read . here @ @ .\n' + num(2) + num(0),
      '> 3 $A000 0 seek 3 here @ 2 write .\n\n !IO ERR! not opened for that\n', '> 3 0 1 seek 3 here @ 2 read .\n' + num(0),
      '> q^/proc/b/ram^ 3 open .\n' + num(3), '> 3 $C000 5 seek 3 here @ 2 read . here @ @ .\n' + num(2) + num(0xABCD),
      '> $1357 here @ ! 3 $C002 5 seek 3 here @ 2 write .\n' + num(2), '> 3 $C002 5 seek 3 here @ 2 read . here @ @ .\n' + num(2) + num(0x1357),
      '> $6004 @ hlock 2 + @ .\n', num(0x1357),
      '> 3 0 0 seek 3 here @ 2 read .\n\n !IO ERR! not found\n', '> 3 0 $1E seek 3 here @ 2 read .\n' + num(0),
      '> q^/proc/9/mem^ 1 open .\n\n !IO ERR! not found\n', '> cat /proc/f/mem\n\n !IO ERR! not allowed\n',
      '> q^/proc/1/mem^ 1 open .\n', ' !IO ERR! not allowed\n', '> ls /sram\n', 'core 65536\n'],
    forbid: ['!UNK WORD!', '!DS PTR ERROR!'],
    check: (out, report) => {
      const m = /longest with IRQs off.*?at cycle\): (\d+): (\S+) -> (\S+)/.exec(report);
      if (m && +m[1] > 5000) return 'IRQs were off for ' + m[1] + ' cycles (from ' + m[2] + ' to ' + m[3] + ')';
    },
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
    expect: [/\nclock 2026-09-30 14:05:0[0-2] battery low\n/, /\/ram> cat \/dev\/time\n2026-09-30 14:05:0[1-3]\n/,
      /\/ram> cat \/dev\/time\n2027-01-02 03:04:0[5-7]\n/],
    check: (out, report) => {
      if (!/--- DS1747: 2027-01-02 03:04:\d\d day 7\n/.test(report)) return 'the DS1747 wasn\'t set (Saturday: day 7): ' + (/--- DS1747.*/.exec(report) || ['none'])[0];
    },
  },
  {
    name: 'rtc-unset', about: 'a DS1747 never set (junk in its registers): "no clock" at boot; setting the time sets it, and it\'s found then',
    args: ['--cycles', '60000000', '--rtc', 'unset', '--input', W(3) + 'echo 2027-01-02 03:04:05 > /dev/time\\r' + W(2) + 'cat /dev/time\\r'],
    expect: ['\nno clock\n', /\/ram> cat \/dev\/time\n2027-01-02 03:04:0[6-9]\n/],
    check: (out, report) => {
      if (!/--- DS1747: 2027-01-02 03:04:\d\d day 7\n/.test(report)) return 'the DS1747 isn\'t running from the time set: ' + (/--- DS1747.*/.exec(report) || ['none'])[0];
    },
  },
  {
    name: 'rtc-none', about: 'no DS1747 (a plain HM628512 in U7): "no clock" at boot, and the clock is set and read as before',
    args: ['--cycles', '40000000', '--input', BOOT + 'echo 2027-01-02 03:04:05 > /dev/time\\r' + P + 'cat /dev/time\\r'],
    expect: ['\nno clock\n\nHyForth', /\/ram> cat \/dev\/time\n2027-01-02 03:04:0[5-9]\n/],
  },
  {
    name: 'semaphores', about: 'semaphores: counts (acquire?, release), a mutex (only its holder releases it), bad ones, a wait another task ends (a pipeline stage), freed and released when a task ends, freed while waited for, Ctrl-C in a wait',
    args: ['--cycles', '120000000', '--input', BOOT + ['2 sem .\\r', '1 acquire? . 1 acquire? . 1 acquire? .\\r', '1 release 1 acquire? .\\r',
      'mutex .\\r', '2 acquire 2 release 2 release\\rioerr .\\r', '9 acquire\\rioerr .\\r', '0 sem .\\r',
      '200 sleep 3 release | 3 acquire 7 .\\r' + W(1), '4 sem . | cat\\r', '4 sem .\\r', '2 acquire 100 sleep | 2 acquire 8 .\\r' + W(1),
      '2 acquire? .\\r', '100 sleep 3 -sem | 3 acquire\\rioerr .\\r', '0 sem .\\r', '3 acquire\\r' + W(1) + '\\x03' + W(1) + '1 2 + .\\r'].join(W(1))],
    expect: ['/ram> 2 sem .\n' + num(1) + '\n', '/ram> 1 acquire? . 1 acquire? . 1 acquire? .\n' + num(0xFFFF) + num(0xFFFF) + num(0) + '\n',
      '/ram> 1 release 1 acquire? .\n' + num(0xFFFF) + '\n', '/ram> mutex .\n' + num(2) + '\n',
      '/ram> 2 acquire 2 release 2 release\n\n !IO ERR! ', '/ram> ioerr .\n' + num(0x63) + '\n',       // (Released already: not held)
      '/ram> 9 acquire\n\n !IO ERR! ', '/ram> ioerr .\n' + num(0x60) + '\n', '/ram> 0 sem .\n' + num(3) + '\n',
      '/ram> 200 sleep 3 release | 3 acquire 7 .\n' + num(7) + '\n',                              // (The shell waits for the stage)
      '/ram> 4 sem . | cat\n' + num(4) + '\n', '/ram> 4 sem .\n' + num(4) + '\n',                     // (The stage's: freed as it ended)
      '/ram> 2 acquire 100 sleep | 2 acquire 8 .\n' + num(8) + '\n',                             // (Released as its holder ended)
      '/ram> 2 acquire? .\n' + num(0) + '\n',                                                    // (The shell holds it now)
      '/ram> 100 sleep 3 -sem | 3 acquire\n\n !IO ERR! ', '/ram> ioerr .\n' + num(0x60) + '\n',       // (Freed while waited for)
      '/ram> 0 sem .\n' + num(3) + '\n', '/ram> 3 acquire\n\n !BREAK!\n', '/ram> 1 2 + .\n' + num(3) + '\n'],
  },
  {
    name: 'serial', about: 'serial settings: 9600 8N1 at boot; stty (/dev/ser/ctl), a refused format, IO_CTL rate and format; the ACIA\'s registers',
    args: ['--cycles', '60000000', '--input', BOOT + 'stty?\\rq^b19200 l7 pe s2^ stty stty?\\rq^l8 pe s2^ stty\\rioerr .\\r' +
      '1 2 6 ioctl stty?\\r1 3 11 ioctl stty?\\r'],
    expect: ['/ram> stty?\nb9600 l8 pn s1\n', 'stty stty?\nb19200 l7 pe s2\n', '!IO ERR!', '/ram> ioerr .\n' + num(0x78) + '\n',
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
    args: ['--cycles', '40000000', '--mark', '/ram> words', '--mark', '/ram> ', '--input', BOOT + 'q^b115200^ stty\\r' + P + 'words\\r'],
    expect: ['/ram> words\n', ': Acls '],
    check: (out, report) => {                                   // (The wire alone, paced: about 1.6M cycles; the old IO path: 3.6M more)
      const at = +/mark: "\/ram> words" at cycle (\d+)/.exec(report)[1];
      const took = [...report.matchAll(/mark: "\/ram> " at cycle (\d+)/g)].map(m => +m[1]).find(c => c > at) - at;
      if (!(took < 3000000)) return 'words took ' + took + ' cycles at 115200, not under 3M';
      const gap = +(/shortest idle between characters sent: ([\d.]+) bits/.exec(report) || [])[1];
      if (!(gap >= 2)) return 'the line idled only ' + gap + ' bits between characters at 115200, not 2 or more';
    },
  },
  {
    name: 'serial-unpaced', about: 'from 115200 (paced by timer 2) back to 9600: sending by the TDRE interrupt again, and the console works',
    args: ['--cycles', '40000000', '--input', BOOT + 'q^b115200^ stty\\r' + P + 'words\\r' + W(2) + 'q^b9600^ stty\\r' + P + '1 2 + .\\r' + P + 'words\\r'],
    expect: ['/ram> q^b9600^ stty\n', '/ram> 1 2 + .\n' + num(3) + '\n', '/ram> words\n', ': Acls '],
  },
  {
    name: 'irqs-off', about: 'no long stretch with IRQs off after boot (tasks starting and ending, a pipeline, sound, files): a serial byte can\'t wait long',
    args: ['--cycles', '90000000', '--input', BOOT + 'words | wc . . .\\rq^/dev/zero^ 1 open here @ 16 read .\\r3 close\\r' +
      'sndtest\\r' + W(2) + 'sndstop\\rshell .\\r' + P + W(2) + 'mmtest\\r'],     // (W: the new shell's start, done)
    expect: ['MMU test: ok'],
    check: (out, report) => {
      const m = /longest with IRQs off.*?at cycle\): (\d+): (\S+) -> (\S+)/.exec(report);
      if (!m) return 'no IRQs-off report';
      if (+m[1] > 5000) return 'IRQs were off for ' + m[1] + ' cycles (from ' + m[2] + ' to ' + m[3] + ')';
    },
  },
];
