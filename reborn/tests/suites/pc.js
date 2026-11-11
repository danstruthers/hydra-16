// ****************************************************************************
// pc.js - the PC tool's tests (/pc, XMODEM).  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { createXmodemPeer } = require('../../sim/lib/xmpeer.js');
const { card, typed, expected, pcReport } = require('../lib/common.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

// The /pc tests' folder (the emulator as the PC tool: sim/lib/pchost.js), and the pc test's lines (as the tools
// test's): a listing, a file read, a file made by a redirect, a copy to the PC compared (its 512-byte writes go in
// frames of 128 bytes: the rest of each in the same request block, a fid other than the first's, as another /pc
// file is open), a directory made, a rename, a program run from the PC (by its path, and by its name through a
// bind), cd into it and a relative name, a directory and a file removed, missing names, the folder's files
const PC_FILES = () => ({ 'hello.txt': 'Hello from the PC\nline two\n', 'sub/x': 'x', 'sub/deep/': '',
  'bin/hi': fs.readFileSync(path.join(TESTS, '..', 'obj', 'samples', 'hi.hyx')) });
const PC_LINES = [
  ["ls /pc","bin/\nhello.txt\nsub/"],
  ["cat /pc/hello.txt","Hello from the PC\nline two"],
  ["ls -l /pc/sub","d-rw-rw-rw- P-        0 2026-10-03 15:04 deep\n--rw-rw-rw- P-        1 2026-10-03 15:04 x"],
  ["echo hi >/pc/new.txt; cat /pc/new.txt","hi"],
  ["{cp /rom/README /pc/r} </pc/hello.txt; cmp /rom/README /pc/r; echo $status",""],
  ["mkdir /pc/d; mv /pc/r /pc/rr; ls /pc","bin/\nd/\nhello.txt\nnew.txt\nrr\nsub/"],
  ["/pc/bin/hi Ann","Hello, Ann!",true],
  ["bind -a /pc/bin /bin; hi Bob","Hello, Bob!",true],
  ["cd /pc/sub; pwd; cat ../hello.txt | wc -l; cd","/pc/sub\n      2"],
  ["rmdir /pc/d; rm /pc/sub/x; cat /pc/nope; rm /pc/nope","cat: /pc/nope: not found\nrm: /pc/nope: not found"],
  ["ls /pc /pc/sub","bin/\nhello.txt\nnew.txt\nrr\nsub/\ndeep/"],
];
// The pc-img test's lines: /pc a HydraFS image on the PC, not a folder (the card's own order in a directory, and
// its stamps: the test's, for what's written too)
const PC_IMG_LINES = [
  ["echo b115200 >/dev/serctl",null],
  ["ls /pc","big\nhello.txt\nsub/\nbin/"],
  ["cat /pc/hello.txt","Hello from the PC\nline two"],
  ["ls -l /pc/sub","--rw-rw-rw- P-        1 2026-10-03 15:04 x\nd-rw-rw-rw- P-        0 2026-10-03 15:04 deep"],
  ["echo hi >/pc/new.txt; cat /pc/new.txt; ls -l /pc/new.txt","hi\n--rw-rw-rw- P-        3 2026-10-03 15:04 /pc/new.txt"],
  ["{cp /rom/README /pc/r} </pc/hello.txt; cmp /rom/README /pc/r; echo $status",""],
  ["cp /pc/big /pc/copy; cmp /pc/big /pc/copy; echo $status",""],
  ["mkdir /pc/d; mv /pc/r /pc/rr; ls /pc","big\nhello.txt\nsub/\nbin/\nnew.txt\nrr\ncopy\nd/"],
  ["/pc/bin/hi Ann","Hello, Ann!",true],
  ["echo shorter >/pc/copy; cat /pc/copy; ls -l /pc/copy","shorter\n--rw-rw-rw- P-        8 2026-10-03 15:04 /pc/copy"],
  ["echo more >>/pc/hello.txt; cat /pc/hello.txt | wc -l","      3"],
  ["mkdir /pc/d; echo no >/pc/sub; cat /pc/hello.txt/x","mkdir: /pc/d: already exists\nrc: /pc/sub: is a directory\ncat: /pc/hello.txt/x: not a directory"],
  ["rmdir /pc/sub; rmdir /pc/d; rm /pc/sub/x; cat /pc/nope; rm /pc/nope","rmdir: /pc/sub: directory not empty\ncat: /pc/nope: not found\nrm: /pc/nope: not found"],
  ["ls /pc /pc/sub","big\nhello.txt\nsub/\nbin/\nnew.txt\nrr\ncopy\ndeep/"],
];
// hydrafs.js's Volume, what serving an image needs of it beyond what the Hydra's commands ask in pc-img: a file's
// length set (longer: zeros; cut: its clusters given back, mid-cluster and to nothing), a write into a hole (the
// hole's part gets clusters, the rest stays one), a rename (and one refused: the name taken), the read-only bit;
// its errors' codes; and the free map right after all of it.  OUT: what's wrong
function hfsVolumeChecks(dir) {
  const f = [], img = path.join(dir, 'vol.img');
  fs.mkdirSync(dir, { recursive: true });
  hydrafs.mkfs(img, 1, 'VOL', undefined, true);
  const v = new hydrafs.Volume(img, { now: 0x2000 });
  const code = fn => { try { fn(); return 'none'; } catch (e) { return e.code || e.message; } };
  try {
    const C = v.cluster, a = v.create(v.root, 'a', 0);
    v.write(a, 0, Buffer.alloc(C + 100, 0x41));
    v.resize(v.lookup(v.root, 'a'), 3 * C + 7);                           // Longer: zeros
    let e = v.lookup(v.root, 'a'), d = v.read(e);
    if (e.size !== 3 * C + 7 || d[C + 99] !== 0x41 || d[C + 100] !== 0 || d[3 * C + 6] !== 0) f.push('a made longer: ' + e.size + ' bytes, or not zeros past its old end');
    v.resize(e, C + 50);                                                 // Cut, in a cluster
    e = v.lookup(v.root, 'a'); d = v.read(e);
    if (e.size !== C + 50 || d.length !== C + 50 || d[C + 49] !== 0x41 || v.extents(e).reduce((s, x) => s + x.len, 0) !== 2) f.push('a cut to a cluster and 50 bytes: ' + e.size + ' bytes in ' + v.extents(e).reduce((s, x) => s + x.len, 0) + ' clusters');
    v.resize(e, 2 * C); d = v.read(v.lookup(v.root, 'a'));               // Longer again: zeros where its cut bytes were
    if (d[C + 49] !== 0x41 || d[C + 50] !== 0 || d[C + 99] !== 0) f.push('a longer again: its cut bytes are back');
    v.resize(v.lookup(v.root, 'a'), 0);
    if (v.extents(v.lookup(v.root, 'a')).length) f.push('a cut to nothing still has clusters');
    // A hole: h's second to fourth clusters (as the Hydra's HydraFS leaves one), then a write into the third
    const h = v.create(v.root, 'h', 0);
    v.write(h, 0, Buffer.alloc(C, 0x42));
    const x0 = v.extents(v.lookup(v.root, 'h'))[0];
    let he = v.lookup(v.root, 'h');
    v.setExtents(he, [x0, { start: hydrafs.HOLE, len: 3 }]); he.size = 4 * C; v.writeEntry(he);
    v.write(v.lookup(v.root, 'h'), 2 * C + 10, Buffer.from('in the hole'));
    he = v.lookup(v.root, 'h'); d = v.read(he);
    const kinds = v.extents(he).map(x => (x.start === hydrafs.HOLE ? 'hole' : 'clusters') + ' ' + x.len).join(', ');
    if (kinds !== 'clusters 1, hole 1, clusters 1, hole 1') f.push('h\'s extents after a write into its hole: ' + kinds);
    if (d.length !== 4 * C || d[C - 1] !== 0x42 || d[C] !== 0 || d.toString('latin1', 2 * C + 10, 2 * C + 21) !== 'in the hole' || d[2 * C + 9] !== 0 || d[3 * C] !== 0) f.push('h read back after the write into its hole isn\'t right');
    v.resize(v.lookup(v.root, 'h'), 2 * C + 5);                           // Cut in its written cluster: the hole after it goes
    // Names
    v.rename(v.root, v.lookup(v.root, 'h'), 'hole.bin');
    if (!v.lookup(v.root, 'hole.bin') || v.lookup(v.root, 'h')) f.push('h renamed hole.bin: not so');
    const errs = [code(() => v.rename(v.root, v.lookup(v.root, 'a'), 'hole.bin')), code(() => v.rename(v.root, v.lookup(v.root, 'a'), 'x'.repeat(32))),
      code(() => v.create(v.root, 'a', 0)), code(() => v.walk('nope')), code(() => v.walk('a/b')), code(() => v.remove('/')),
      code(() => { v.mkdir('dd'); v.put('dd/f', Buffer.from('f')); v.remove('dd'); })].join(' ');
    if (errs !== 'EEXIST ENAMETOOLONG EEXIST ENOENT ENOTDIR EPERM ENOTEMPTY') f.push('the errors\' codes: ' + errs);
    v.setReadOnly(v.lookup(v.root, 'a'), true);
    if (!(v.lookup(v.root, 'a').mode & hydrafs.MODE_RO)) f.push('a read-only: its bit isn\'t set');
    v.setReadOnly(v.lookup(v.root, 'a'), false);
    if (v.lookup(v.root, 'a').mode & hydrafs.MODE_RO) f.push('a not read-only: its bit is set');
    if (v.lookup(v.root, 'hole.bin').stamp !== 0x2000) f.push('the volume\'s own stamp (now) isn\'t on its files');
    v.flush();
    const problems = v.check();
    if (problems.length) f.push('the free map after it all: ' + problems.slice(0, 3).join('; '));
  } catch (e) { f.push('hydrafs.js\'s Volume: ' + e.message); } finally { v.close(); }
  return f;
}
// The pc-ro test's lines: the folder served read-only
const PC_RO_LINES = [
  ["cat /pc/hello.txt","Hello"],
  ["echo no >/pc/x","rc: /pc/x: read-only"],
  ["echo no >/pc/hello.txt","rc: /pc/hello.txt: read-only"],
  ["rm /pc/hello.txt; mkdir /pc/d; mv /pc/hello.txt /pc/h","rm: /pc/hello.txt: read-only\nmkdir: /pc/d: read-only\nmv: /pc/hello.txt: read-only"],
  ["ls -l /pc","--r--r--r-- P-        6 2026-10-03 15:04 hello.txt"],
];
// A file of 120 lines (pc-two's), and a song (pc-song's: a note, then its loop, another, 0.6 s apart at 60 Hz)
const PC_BIG = () => Buffer.from([...Array(120)].map((_, i) => 'line ' + i + ' of the big file on the PC\n').join(''));
function PC_SONG() {
  const fm = pairs => [0x40 | pairs.length / 2, ...pairs];
  const voice = [0x20, 0xC7, 0x38, 0x00];
  for (const op of [0x00, 0x08, 0x10, 0x18]) voice.push(0x40 + op, 0x01, 0x60 + op, 0x10, 0x80 + op, 0x1F, 0xA0 + op, 0x00, 0xC0 + op, 0x00, 0xE0 + op, 0x0F);
  const note = kc => [...fm([0x28, kc, 0x30, 0x00, 0x08, 0x78]), 0x80 + 30, ...fm([0x08, 0x00]), 0x80 + 6];
  const intro = [...fm(voice), 0x05, 0x3F, 0x40, 0x82, 0x12, 0x34, ...note(0x3E)];    // (A PSG write; an extension)
  const loop = [...note(0x44)];
  const loopAt = 16 + intro.length;
  const hdr = [0x7A, 0x6D, 1, loopAt & 255, loopAt >> 8 & 255, loopAt >> 16, 0, 0, 0, 0x01, 0, 0, 60, 0, 0, 0];
  return Buffer.from([...hdr, ...intro, ...loop, 0x80]);
}
// The xmodem test's file: 3000 bytes, every value (Ctrl-C, Ctrl-], the frame marks ... too), its second 1K all SUB
// (the padding's byte: held back, then written, as data comes after it), its last byte not SUB
const XM_DATA = () => Buffer.from(Array.from({ length: 3000 }, (_, i) => i >= 1024 && i < 2048 ? 0x1A : i === 2999 ? 0x41 : (i * 7 + (i >> 8)) & 0xFF));

module.exports = {
  tests: [
    {
      name: 'pc', what: '/pc (#P, the console driver\'s): a folder on the PC through the serial port, its frames between the console\'s bytes; files read, made, copied, renamed, removed; a program run from it',
      init: 't_rc', cycles: 250e6, pc: { files: PC_FILES },
      get machine() { return { input: typed(PC_LINES) }; },
      get expect() { return expected(PC_LINES); },
      check(m) {
        const f = pcReport(m, 1, 0, 0), at = n => path.join(m.pc.dir, n);
        if (fs.readFileSync(at('new.txt'), 'latin1') !== 'hi\n') f.push('new.txt on the PC: ' + JSON.stringify(fs.readFileSync(at('new.txt'), 'latin1')));
        if (!fs.readFileSync(at('rr')).equals(fs.readFileSync(path.join(TESTS, '..', 'romfs', 'README')))) f.push('rr on the PC isn\'t /rom/README');
        for (const n of ['d', 'r', 'sub/x']) if (fs.existsSync(at(n))) f.push(n + ' is still on the PC');
        return f;
      },
    },
    {
      name: 'pc-img', what: '/pc a HydraFS image file on the PC (the PC tool\'s folder can be an image: sim/tools/pcfs.js, hydrafs.js\'s Volume): its files read, a program run from it; files made, written, added to, made again shorter, copied (17K: several clusters), renamed, removed, a directory made and removed; its errors; then the image on the PC: the files as the Hydra left them, and its free map right (hydrafs.js check).  And the Volume\'s own: a length set, a write into a hole, a rename, the read-only bit, its errors\' codes',
      init: 't_rc', cycles: 400e6,
      pc: { image: { mb: 1, label: 'PCIMG' }, files: () => Object.assign({ big: Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]) }, PC_FILES()) },
      get machine() { return { input: typed(PC_IMG_LINES) }; },
      get expect() { return expected(PC_IMG_LINES); },
      check(m) {
        const f = pcReport(m, 1, 0, 0), v = new hydrafs.Volume(m.pc.image, { readOnly: true });
        try {
          const bytes = n => { const e = v.tryWalk(n); return e && !e.isDir ? v.read(e) : Buffer.from('(no such file)'); };
          const text = n => bytes(n).toString('latin1'), big = Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]);
          if (text('new.txt') !== 'hi\n') f.push('new.txt in the image: ' + JSON.stringify(text('new.txt')));
          if (text('copy') !== 'shorter\n') f.push('copy in the image: ' + JSON.stringify(text('copy').slice(0, 40)));
          if (text('hello.txt') !== 'Hello from the PC\nline two\nmore\n') f.push('hello.txt in the image: ' + JSON.stringify(text('hello.txt')));
          if (!bytes('big').equals(big)) f.push('big in the image isn\'t as it was');
          if (!bytes('rr').equals(fs.readFileSync(path.join(TESTS, '..', 'romfs', 'README')))) f.push('rr in the image isn\'t /rom/README');
          for (const n of ['d', 'r', 'sub/x']) if (v.tryWalk(n)) f.push(n + ' is still in the image');
          const problems = v.check();
          if (problems.length) f.push('the image\'s free map: ' + problems.slice(0, 3).join('; '));
          this.notes = [v.freeCount + ' of ' + v.clusters + ' clusters free'];
        } finally { v.close(); }
        f.push(...hfsVolumeChecks(path.join(path.dirname(m.pc.image), 'pc-img-vol')));
        return f;
      },
    },
    {
      name: 'pc-two', what: '/pc from two tasks at once (a pipeline: one reads a file, the other writes its copy): one request at a time, the other waiting its turn',
      init: 't_rc', cycles: 400e6, pc: { files: { big: PC_BIG } },
      machine: { input: 'ācat /pc/big | cat >/pc/copy; cmp /pc/big /pc/copy; echo $status\r' },
      expect: ['% cat /pc/big | cat >/pc/copy; cmp /pc/big /pc/copy; echo $status\n\n%'],
      check(m) {
        const f = pcReport(m, 1, 0, 0);
        if (!fs.readFileSync(path.join(m.pc.dir, 'copy')).equals(PC_BIG())) f.push('the copy on the PC isn\'t big');
        return f;
      },
    },
    {
      name: 'pc-fast', what: '/pc at 115200: a file copied on the PC through the Hydra and compared, no reply\'s byte lost (none asked for again: the ACIA\'s interrupt goes ahead of the tick\'s and timer 2\'s)',
      // (17K: some 530 requests.  Before, about one in 22 lost a byte and went again)
      init: 't_rc', cycles: 300e6, pc: { files: { big: () => Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]) } },
      machine: { input: 'āecho b115200 >/dev/serctl\r' + 'ācp /pc/big /pc/copy; cmp /pc/big /pc/copy; echo $status\r' },
      expect: ['% cp /pc/big /pc/copy; cmp /pc/big /pc/copy; echo $status\n\n%'],
      check(m) {
        const f = pcReport(m, 1, 0, 0), big = Buffer.concat([PC_BIG(), PC_BIG(), PC_BIG(), PC_BIG()]);
        if (m.acia.pcLost) f.push(m.acia.pcLost + ' reply byte(s) lost (they came while the last was unread)');
        if (!fs.readFileSync(path.join(m.pc.dir, 'copy')).equals(big)) f.push('the copy on the PC isn\'t big');
        return f;
      },
    },
    {
      name: 'pc-song', what: 'a song played from /pc (play /pc/t.zsm 2), its loop twice more, in time: a read on the line doesn\'t hold a note up',
      init: 't_rc', cycles: 120e6, pc: { files: { 't.zsm': PC_SONG } },
      machine: { input: 'āplay /pc/t.zsm 2; echo $status\r' },
      expect: ['% play /pc/t.zsm 2; echo $status\n\n%'],
      check(m) {
        const f = pcReport(m, 1, 0, 0), mult = JSON.parse(fs.readFileSync(path.join(TESTS, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const on = m.ym.keyOns.map(k => +k.match(/at cycle (\d+)/)[1]), want = 0.6 * 3579545 * mult, slack = 2 * 3579545 * mult / 200;
        if (on.length !== 4) return [...f, 'key-ons: ' + on.length + ', not 4'];
        for (let k = 1; k < 4; k++) if (Math.abs(on[k] - on[k - 1] - want) > slack)
          f.push('key-on ' + k + ' came ' + (on[k] - on[k - 1]) + ' cycles after the last, not 0.6 s (' + Math.round(want) + ', give or take two system ticks)');
        return f;
      },
    },
    {
      name: 'pc-ro', what: '/pc served read-only (the PC tool\'s --read-only): files read; a write, a create, a remove, a mkdir, a rename refused; the folder as it was',
      init: 't_rc', cycles: 120e6, pc: { files: { 'hello.txt': 'Hello\n' }, readOnly: true },
      get machine() { return { input: typed(PC_RO_LINES) }; },
      get expect() { return expected(PC_RO_LINES); },
      check(m) {
        const f = pcReport(m, 1, 0, 0);
        if (fs.readdirSync(m.pc.dir).join() !== 'hello.txt' || fs.readFileSync(path.join(m.pc.dir, 'hello.txt'), 'latin1') !== 'Hello\n') f.push('the folder changed: ' + fs.readdirSync(m.pc.dir).join());
        return f;
      },
    },
    {
      name: 'pc-none', what: '/pc with no PC tool: the attach (its bytes on the terminal) unanswered, an error a second on, each time; the console goes on',
      init: 't_rc', cycles: 80e6,
      machine: { input: 'āls /pc\rācat /pc/x\rāecho still here\r' },
      expect: ['% ls /pc\n\x1eA', 'ls: /pc: i/o error\n%', '% cat /pc/x\n\x1eA', 'cat: /pc/x: i/o error\n%', '% echo still here\nstill here\n%'],
    },
    {
      name: 'pc-damage', what: '/pc\'s frames damaged on the line: requests (the PC tool asks for them again: its NAK), a reply (the Hydra asks again, and the PC tool answers from its last reply); the answers right, nothing damaged shown; serctl counts them (2 nak, 1 damaged)',
      init: 't_rc', cycles: 140e6, pc: { files: { 'hello.txt': 'Hello from the PC\n' }, damage: ['q3', 'r4', 'q6'] },
      machine: { input: 'āls /pc\rācat /pc/hello.txt\rācat /pc/hello.txt\rācat /dev/serctl\r' },
      expect: ['% ls /pc\nhello.txt\n%', '% cat /pc/hello.txt\nHello from the PC\n% cat /pc/hello.txt\nHello from the PC\n%', ' req, 0 cut short, 0 unanswered, 1 damaged, 2 nak, 0 gone\nline 0 overruns, 0 framing errors, 0 dropped\n%'],
      check: m => pcReport(m, 1, 2, 1),
    },
    {
      name: 'xmodem', what: 'xmodem: a file received (1K blocks, a CRC; one damaged, one sent twice) and sent back (128-byte blocks, a checksum, one NAKed; 1K ones), the same; Ctrl-C at its start; the PC cancelling; 115200',
      init: 't_rc', cycles: 200e6,
      // (The emulator the PC's end: sim/lib/xmpeer.js.  The -s sessions' start, NAK or C, typed as a key.  At 115200,
      // 1K blocks received: one comes faster than it can be taken, what's behind waiting in the console's receive ring)
      get machine() {
        this.peer = createXmodemPeer([
          { trigger: 'xmodem -r /ram/x\r\n', role: 'send', data: XM_DATA(), k: true, damage: [2], again: [1] },
          { trigger: 'xmodem -s /ram/x\r\n', role: 'receive', crc: false, nak: [3] },
          { trigger: 'xmodem -s -k /ram/x\r\n', role: 'receive', crc: true },
          { trigger: 'xmodem -r /ram/y\r\n', role: 'send', data: XM_DATA(), cancel: 2 },
          { trigger: 'xmodem -r /ram/w\r\n', role: 'send', data: XM_DATA(), k: true },
          { trigger: 'xmodem -s -k /ram/w\r\n', role: 'receive', crc: true },
        ]);
        // (flow 'none': the PC's end takes every byte, XON and XOFF too, as a terminal does in a binary transfer)
        return { pcHost: this.peer, flow: 'none', input: 'āxmodem -r /ram/x\r' + 'āxmodem -s /ram/x\rĀ\x15' + 'āxmodem -s -k /ram/x\rĀC' +
          'āxmodem -r /ram/z\rĀ\x03' + 'āxmodem -r /ram/y\r' + 'āecho $status; xmodem /ram/x\r' +
          'āecho b115200 >/dev/serctl; xmodem -r /ram/w\r' + 'āxmodem -s -k /ram/w\rĀC' };
      },
      expect: ['% xmodem -r /ram/x\n/ram/x: 3000 bytes\n%', '% xmodem -s /ram/x\n/ram/x: 3000 bytes\n%', '% xmodem -s -k /ram/x\n/ram/x: 3000 bytes\n%',
        'xmodem: /ram/z: cancelled\n%', '% xmodem -r /ram/y\nxmodem: /ram/y: cancelled\n%',
        '% echo $status; xmodem /ram/x\ncancelled\nusage: xmodem -r file | -s [-k] file\n%',
        '% echo b115200 >/dev/serctl; xmodem -r /ram/w\n/ram/w: 3000 bytes\n%', '% xmodem -s -k /ram/w\n/ram/w: 3000 bytes\n%'],
      // (Each session's data back as the file, and SUBs to its blocks' end: 128-byte blocks, and 1K ones)
      check() {
        const f = [], [rx, tx, tk, can, rx115, tk115] = this.peer.sessions, data = XM_DATA();
        this.notes = this.peer.sessions.map((s, i) => 'session ' + (i + 1) + ': ' + (s.log || ['never armed']).join(', '));
        if (rx.done !== true || !rx.log.includes('damaged 2') || !rx.log.includes('again 1')) f.push('receiving: ' + (rx.log || []).join(', '));
        if (tx.done !== true || !tx.log.includes('nak 3')) f.push('sending back: ' + (tx.log || []).join(', '));
        if (tk.done !== true || tk.log.filter(l => / 1K$/.test(l)).length !== 3) f.push('sending back, 1K: ' + (tk.log || []).join(', '));
        if (rx115.done !== true) f.push('receiving at 115200: ' + (rx115.log || []).join(', '));
        for (const [s, block] of [[tx, 128], [tk, 1024], [tk115, 1024]]) {
          const back = Buffer.from(s.got || []), pad = Math.ceil(data.length / block) * block;
          if (back.length !== pad || !back.subarray(0, data.length).equals(data) || back.subarray(data.length).some(b => b !== 0x1A))
            f.push('sent back in ' + block + '-byte blocks: ' + back.length + ' bytes, not the file (' + data.length + ') and SUBs to ' + pad);
        }
        if (can.done !== 'cancelled') f.push('the PC\'s cancel: ' + (can.log || []).join(', '));
        return f;
      },
    },
  ],
};
