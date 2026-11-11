// ****************************************************************************
// common.js - the tests' shared helpers: the cards, the lines typed and expected, the PC folders, the system's fixtures (the suites' and the runner's: sim/test.js).
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { defaultModules } = require('../../../base/sim/lib/machine.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

const MODULES = defaultModules();                              // (The tests' RAM modules: 3, the board's, or HYDRA_MODULES's)

const IRQ_OFF_MAX = 200;                                      // (docs/design/reimplementation-from-scratch.md, §8: 115200)
// An SD card for the emulator (sim/lib/sd.js) on SPI device dev, its blocks in memory: byte i of block n is fill(n, i)
function card(dev, blocks, sdsc, fill) {
  const data = new Uint8Array(blocks * 512);
  for (let n = 0; n < blocks; n++) for (let i = 0; i < 512; i++) data[n * 512 + i] = fill(n, i) & 0xFF;
  return { dev, blocks, sdsc, data, read: n => data.slice(n * 512, n * 512 + 512), write: (n, b) => data.set(b, n * 512) };
}
// An SD card on SPI device dev from an image file, claiming blocks (those past the file's end read as zeros); its
// writes kept, and save() puts them in the file (for the PC tool to look at)
const CARD_DIR = path.join(TESTS, '..', 'obj', 'cards'), OLD_CARDS = path.join(TESTS, '..', '..', 'old', 'sim', 'cards');
// The SD card's image (bin/sdcard.img: the samples, the songs, the benchmarks: tools/sdcard.js), for the tests that run a sample
// (sdcard: true): in SD device 0, its writes kept apart (imageCard's), so the image stays as it was built
const SDCARD = path.join(TESTS, '..', 'bin', 'sdcard.img');
function sdcardOf(t) {
  const d = Object.getOwnPropertyDescriptor(t, 'machine');
  Object.defineProperty(t, 'machine', { configurable: true, enumerable: true, get() {
    const m = d ? (d.get ? d.get.call(this) : d.value) : {};
    if ((m.sd || []).some(c => c.dev === 0)) throw new Error(t.name + ': its own card in SD device 0, and sdcard');
    return Object.assign({}, m, { sd: [imageCard(0, SDCARD, fs.statSync(SDCARD).size / 512), ...(m.sd || [])] });
  } });
}

function imageCard(dev, file, blocks) {
  const base = fs.readFileSync(file), written = new Map();
  return { dev, blocks, file,
    read: n => written.get(n) || Buffer.concat([n * 512 < base.length ? base.subarray(n * 512, n * 512 + 512) : Buffer.alloc(0)], 512),
    write: (n, b) => written.set(n, Buffer.from(b)),
    save() { const fd = fs.openSync(file, 'r+'); for (const [n, b] of written) fs.writeSync(fd, b, 0, 512, n * 512); fs.closeSync(fd); } };
}

// The rc test's lines, and what each says
const RC_LINES = [
  ["echo hello","hello"],
  ["cat /env/prog; echo","/bin/cat"],
  ["x=(a b c); echo $x $#x $x(2)","a b c 3 b"],
  ["echo 'a  b' 'it''s'","a  b it's"],
  ["echo $\"x","a b c"],
  ["echo x^$x $x^1 $x^$x","xa xb xc a1 b1 c1 aa bb cc"],
  ["y=1 echo $y; echo $#y","1\n0"],
  ["echo $x(2-) $x(1-2)","b c a b"],
  ["echo one >/ram/f; echo two >>/ram/f; cat /ram/f","one\ntwo"],
  ["cat </ram/f >[2=1]","one\ntwo"],
  ["wc <[0=]","wc: -: bad file descriptor\n      0       0       0"],
  ["{echo b >[1=3]} >[3]/ram/y >/dev/null; cat /ram/y","b"],
  ["echo piped | cat","piped"],
  ["echo a b | cat | cat","a b"],
  ["echo a b c d | cat | cat | cat | wc -w","      4"],
  ["rc -c 'i=(); while() {i=($i x); echo $#i; ~ $#i 3 && exit}'","1\n2\n3"],
  ["if() echo always","always"],
  ["if(~ a a) echo yes; if not echo no","yes"],
  ["if(~ a b) echo yes; if not echo no","no"],
  ["for(i in 1 2 3) echo $i","1\n2\n3"],
  ["n=(); while(! ~ $#n 3) n=($n x); echo $#n","3"],
  ["switch(b){case a; echo A; case b c; echo B}","B"],
  ["fn greet {echo hi $1 $#*}; greet you there","hi you 2"],
  ["echo `{echo inner} after","inner after"],
  ["~ a a && echo and; ~ a b || echo or","and\nor"],
  ["cat /nothing; echo status $status","cat: /nothing: not found\nstatus 1"],
  ["echo /rom/lib/n*","/rom/lib/namespace"],
  ["echo /rom/lib/*","/rom/lib/as /rom/lib/bank /rom/lib/basic /rom/lib/edit /rom/lib/font /rom/lib/forth /rom/lib/hylang /rom/lib/namespace /rom/lib/profile /rom/lib/shell /rom/lib/windows"],
  ["echo 'no*match'*","no*match*"],
  ["cd /rom/lib; pwd; cd","/rom/lib"],
  ["rc -c 'echo sub $x'","sub a b c"],
  ["{echo in a block} >/ram/g; cat /ram/g","in a block"],
  ["whatis greet","fn greet {echo hi $1 $#*}"],
  ["{echo back >/ram/bg} & wait; cat /ram/bg; echo $#apid","back\n1"],
  ["echo 'echo script $1 $0' >/ram/s; rc /ram/s arg","script arg /ram/s"],
  ["echo 'z=sourced' >/ram/d; . /ram/d; echo $z","sourced"],
  ["echo x >/dev/sd/x/data; whatis status","echo: write error: read-only\nstatus='write error'"],
  ["eval echo evaled $x(1)","evaled a"],
  ["fn sh {shift; echo $*}; sh a b c","b c"],
  ["rc -c 'exit oops'; echo $status","oops"],
  ["echo (a","rc: syntax error"],
  ["whatis echo x; q=('it''s' '' a.b); whatis q","/bin/echo\nx=(a b c)\nq=('it''s' '' a.b)"],
  ["bind '#n' /mnt; ls /mnt","null\nzero\nkmesg"],
  ["{mount /mnt}","rc: usage: mount [-abc] '#x' old [spec]"],
  ["{bind /mnt}","rc: usage: bind [-abc] new old"],
  ["ls /rom/lib","as/\nbank/\nbasic/\nedit/\nfont/\nforth/\nhylang/\nnamespace\nprofile\nshell\nwindows"],
  ["cat /bin/echo >/ram/hi; cd /ram; hi from dot; cd","from dot"],
  ["cat /nothing >[2]/ram/e; cat /ram/e","cat: /nothing: not found"],
  ["cat /nothing |[2] cat >/ram/p; echo -n 'p: '; cat /ram/p","p: cat: /nothing: not found"],
  ["echo $task $#path $path # a comment","2 2 . /bin"],
  ["path=(); ls; path=(. /bin); ls /rom/lib","rc: ls: not found\nas/\nbank/\nbasic/\nedit/\nfont/\nforth/\nhylang/\nnamespace\nprofile\nshell\nwindows"],
  ["! ~ a b && echo not; echo $status","not\n"],
];

// The lshell test's card: /lib/shell, HyForth as the shell (or the shell given: the hywin test's, hylang)
function shellCard(line = '/bin/forth -l', name = 'shell0') {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, name + '.img');
  hydrafs.mkfs(f, 8, 'SHELL', undefined, true);
  const v = new hydrafs.Volume(f);
  v.mkdir('lib');
  v.put('lib/shell', Buffer.from(line + '\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// hylang's card (the hysuite and hyhydra tests'): danlang's suite's files (tests/hylang), hylang's own checks
// (tests/hyhydra), and the test's own files ({ name: text }); its library (danlang's) is the ROM disk's, /lib/hylang.
// The image is the test's own (obj/cards/hylang-TEST.img), as sim/test.js -j runs tests side by side
function hylangCard(test, files = {}) {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'hylang-' + test + '.img');
  fs.rmSync(f, { force: true });
  hydrafs.mkfs(f, 8, 'HYLANG', undefined, true);
  const v = new hydrafs.Volume(f);
  const dir = path.join(TESTS, 'hylang');
  const put = (from, to) => v.put(to, fs.readFileSync(from));
  for (const n of fs.readdirSync(dir).filter(n => /\.(dl|hl)$/.test(n))) put(path.join(dir, n), n);
  const own = path.join(TESTS, 'hyhydra');                // (hylang's own checks: hydra.hl)
  for (const n of fs.readdirSync(own).filter(n => /\.hl$/.test(n))) put(path.join(own, n), n);
  for (const [n, text] of Object.entries(files)) v.put(n, Buffer.from(text, 'latin1'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// A test's lines typed, each at its prompt, and its expect (as the tools test's)
const typed = lines => lines.map(l => 'ā' + l[0] + '\r').join('');
const expected = lines => lines.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%'));
// The PC host's report (its note), checked: attaches, damaged frames, repeats
function pcReport(m, attaches, naks, repeats) {
  const h = m.pc.host, f = [];
  if (h.attaches !== attaches) f.push('/pc: ' + h.attaches + ' attach(es), not ' + attaches);
  if (h.naks !== naks) f.push('/pc: ' + h.naks + ' damaged, not ' + naks);
  if (h.fsrv.stats.repeats !== repeats) f.push('/pc: ' + h.fsrv.stats.repeats + ' repeated, not ' + repeats);
  return f;
}

module.exports = {
  MODULES, IRQ_OFF_MAX, card, CARD_DIR, OLD_CARDS, SDCARD, sdcardOf, imageCard, RC_LINES, shellCard, hylangCard,
  typed, expected, pcReport,
};
