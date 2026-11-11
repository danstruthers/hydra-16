// ****************************************************************************
// sound.js - the sound's tests: the YM2151, the PSG, the songs.  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const hydrafs = require('../../sim/tools/hydrafs.js');
const { card, CARD_DIR, OLD_CARDS, imageCard, typed, expected } = require('../lib/common.js');
const TESTS = path.join(__dirname, '..');   // (The tests' folder: their fixtures')

// The C test's lines (as the tools test's): the C SDK's samples, the library's test (ctest: its "ok" lines), and the
// tools in C (sort and grep)
// The chorus sample's lines (sdk/c/samples/chorus), and the round sample's tune: each note's length, in eighths
const CHORUS = ['Row, row, row your boat,', 'Gently down the stream.', 'Merrily, merrily, merrily, merrily,', 'Life is but a dream.'];
const ROUND_LENS = [3, 3, 2, 1, 3, 2, 1, 2, 1, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 6];

// The sound test's lines (as the tools test's): #a's files, the volume, claims (one another program holds), its
// errors, the shadow, the C sample tones, and the bell (no Vera X: 8 channels, the PSG's E_NODEV)
const SND_LINES = [
  ["ls /dev | grep snd; cat /dev/sndctl","snd\nsndctl\nvolume 100\nchannels 8\nclaimed"],
  ["echo volume 150 >/dev/sndctl; cat /dev/sndctl; echo volume 100 >/dev/sndctl","volume 150\nchannels 8\nclaimed"],
  ["{echo claim 5 >[1=3]; cat /dev/sndctl} >[3]/dev/sndctl; cat /dev/sndctl","volume 100\nchannels 8\nclaimed 0 2\nvolume 100\nchannels 8\nclaimed"],
  ["echo frob >/dev/sndctl; echo claim >/dev/sndctl", [
    "echo: write error: invalid argument",
    "echo: write error: invalid argument",
  ].join('\n')],
  ["wc -c /dev/snd","    256 /dev/snd"],
  ["/sd/0/sample/c/tones 0 & sleep 1; echo claim 1 >/dev/sndctl; echo note 0 60 >/dev/sndctl; wait",
    "echo: write error: busy\necho: write error: busy\ntones: patch 0, $20 C4, $28 4C, C#4 61, the tune played, 1000 Hz 5D, mml 0, chord 0"],
  ["echo x >/dev/bell",null],
  // (The channel commands as text: channel 4 a note bent down half a semitone, on the left; 5 a drum on the right;
  // two registers of 6's; their effects on the chip checked in check)
  ["echo patch 4 0 >/dev/sndctl; echo note 4 69 >/dev/sndctl; echo level 4 100 >/dev/sndctl; echo vol 4 90 >/dev/sndctl",null],
  ["echo pan 4 left >/dev/sndctl; echo bend 4 -32 >/dev/sndctl; echo off 4 >/dev/sndctl",null],
  ["echo pan 5 2 >/dev/sndctl; echo drum 5 38 >/dev/sndctl; echo reg 46 74 54 252 >/dev/sndctl",null],
  ["echo note 24 60 >/dev/sndctl; echo patch 0 163 >/dev/sndctl; echo pan 0 up >/dev/sndctl; echo bend 0 128 >/dev/sndctl",
    Array(4).fill('echo: write error: invalid argument').join('\n')],
  ["echo note 8 60 >/dev/sndctl; echo wave 23 saw >/dev/sndctl", Array(2).fill('echo: write error: no such device').join('\n')],
  ["echo bend 0 -129 >/dev/sndctl; echo note 0 >/dev/sndctl; echo reg 46 >/dev/sndctl; echo reg 46 256 >/dev/sndctl",
    Array(4).fill('echo: write error: invalid argument').join('\n')],
  // (Step 3's: a frequency (1000 Hz: B5 and 13 64ths), a glide (C5, no attack), the LFO, a channel's sensitivity
  // to it, the noise; checked in check)
  ["echo freq 2 1000 >/dev/sndctl; echo freq 3 440 >/dev/sndctl; echo glide 3 72 >/dev/sndctl",null],
  ["echo lfo 200 10 20 2 >/dev/sndctl; echo sens 3 5 2 >/dev/sndctl; echo noise 7 >/dev/sndctl",null],
  ["echo noise off >/dev/sndctl; echo noise 9 >/dev/sndctl",null],
  ["echo freq 99 440 >/dev/sndctl; echo lfo 1 2 3 >/dev/sndctl; echo lfo 1 128 0 0 >/dev/sndctl",
    Array(3).fill('echo: write error: invalid argument').join('\n')],
  ["echo sens 0 8 0 >/dev/sndctl; echo noise 32 >/dev/sndctl; echo noise loud >/dev/sndctl",
    Array(3).fill('echo: write error: invalid argument').join('\n')],
];

// The PSG test's lines (with a Vera X: channels 8-23): the state; notes, a waveform, speakers, a level, a frequency,
// a bend, a note off (their registers on the chip checked in check); claims of the PSG's channels (given back
// keyed off: 8's note ends); the master volume on them; errors; /psg and vid's; a note played while the VERA's
// claimed (only kept), on the chip when the claim ends (the claimer's task in the state, then none); hylang's and HyForth's words (snd-wave; claims of the
// PSG's channels: hylang's by number, HyForth's snd-claim-psg); a song of the PSG's alone (play: its PSG writes,
// its PSG channels claimed while it plays)
const PSG_LINES = [
  ["ls /dev | grep psg; ls /dev/vid | grep psg; cat /dev/sndctl", "psg\npsg\nvolume 100\nchannels 24\nclaimed"],
  ["echo note 8 69 >/dev/sndctl; echo wave 9 saw >/dev/sndctl; echo note 9 60 >/dev/sndctl", null],
  ["echo pan 10 left >/dev/sndctl; echo level 10 64 >/dev/sndctl; echo note 10 72 >/dev/sndctl", null],
  ["echo freq 11 1000 >/dev/sndctl; echo wave 12 2 31 >/dev/sndctl; echo patch 13 3 >/dev/sndctl", null],
  ["echo bend 8 -64 >/dev/sndctl; echo off 9 >/dev/sndctl", null],
  ["{echo claim 0 3 >[1=3]; cat /dev/sndctl} >[3]/dev/sndctl; cat /dev/sndctl",
    "volume 100\nchannels 24\nclaimed 8 9\nvolume 100\nchannels 24\nclaimed"],
  ["{echo claim 1 32768 >[1=3]; cat /dev/sndctl} >[3]/dev/sndctl", "volume 100\nchannels 24\nclaimed 0 23"],
  ["echo wave 3 pulse >/dev/sndctl; echo wave 8 pulse 64 >/dev/sndctl", Array(2).fill('echo: write error: invalid argument').join('\n')],
  ["echo wave 8 square >/dev/sndctl; echo sens 8 1 1 >/dev/sndctl", Array(2).fill('echo: write error: invalid argument').join('\n')],
  ["echo volume 50 >/dev/sndctl", null],
  ["wc -c /dev/psg /dev/vid/psg", "     64 /dev/psg\n     64 /dev/vid/psg\n    128 total"],
  ["echo '(do (use \"snd\") (snd-wave 15 :triangle 20) (snd-note 15 69))' | hylang >/dev/null", null],
  ["echo 'lib sound 16 1 40 snd-wave 16 60 snd-note' | forth >/dev/null", null],
  ["echo '(do (use \"snd\") (snd-claim 0 17 23) (run \"cat\" \"/dev/sndctl\"))' | hylang | grep claimed", "claimed 0 17 23"],
  ["echo 'lib sound lib hydra 3 snd-claim-psg s\" cat /dev/sndctl\" sh drop' | forth | grep claimed", "claimed 8 9"],
  ["{echo claim >[1=3]; echo note 14 69 >/dev/sndctl; grep claimed /dev/vid/ctl} >[3]/dev/vid/ctl", null, true],
  ["grep claimed /dev/vid/ctl", "claimed"],
  ["play /sd/0/p.zsm & sleep 1; cat /dev/sndctl; wait", "volume 50\nchannels 24\nclaimed 8 9"],
];

// The psg test's song (PSG_SONG: a ZSM of the PSG's writes alone, its voices 0 and 1 claimed): A4 on voice 0, C4 on
// a sawtooth on voice 1 half a second later (30 ticks at 60 Hz), voice 0 off half a second after that, then two
// seconds more; and its card
function PSG_SONG() {
  const hdr = [0x7A, 0x6D, 1, 0, 0, 0, 0, 0, 0, 0x00, 0x03, 0x00, 60, 0, 0, 0];
  return Buffer.from([...hdr, 0, 0x9D, 1, 0x04, 3, 0x3F, 2, 0xFF, 0x80 | 30, 4, 0xBE, 5, 0x02, 7, 0x7F, 6, 0xFF, 0x80 | 30, 2, 0xC0,
    0x80 | 120, 0x80]);
}
function psgCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'psg0.img');
  hydrafs.mkfs(f, 8, 'SONGS', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('p.zsm', PSG_SONG());
  v.close();
  return [imageCard(0, f, 16384)];
}

// The psgmml test's card: scores on the PSG's channels (I-X) and hysong.js's ZSMs of them: the ROM disk's
// songs/vera.mml (both chips, each waveform and envelope) as v.mml and vpc.zsm, tests/scores/edges.mml and edges2.mml
// as e and f; and scores that are wrong (an instrument on the other chip's channel, both ways; x on the PSG; y past
// its registers; three instruments it can't read; a note before an instrument on channel 23)
const PSG_SCORES = { v: path.join(TESTS, '..', 'sdcard', 'songs', 'vera.mml'), e: path.join(TESTS, 'scores', 'edges.mml'), f: path.join(TESTS, 'scores', 'edges2.mml') };
const PSG_BAD = ['@w { wave saw }\nA @w c\n', '@g { gm 0 }\nI @g c\n', '@w { wave saw }\nI @w x36\n', '@w { wave saw }\nJ @w y 64,1 c\n',
  '@w { wave square }\nI @w c\n', '@w { env 1 2 64 3 }\nI @w c\n', '@w { wave saw alg 3 }\nI @w c\n', '@w { wave saw }\nX c\n'];
function psgScoreCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'psgmml0.img');
  fs.rmSync(f, { force: true });
  hydrafs.mkfs(f, 8, 'SCORES', undefined, true);
  const v = new hydrafs.Volume(f);
  for (const [n, score] of Object.entries(PSG_SCORES)) {
    const zsm = path.join(CARD_DIR, 'psgmml-' + n + '.zsm');
    require('child_process').execFileSync(process.execPath, [path.join(TESTS, '..', 'sim', 'tools', 'hysong.js'), score, zsm, '--quiet']);
    v.put(n + '.mml', fs.readFileSync(score));
    v.put(n + 'pc.zsm', fs.readFileSync(zsm));
  }
  PSG_BAD.forEach((t, i) => v.put('b' + (i + 1) + '.mml', Buffer.from(t)));
  v.close();
  return [imageCard(0, f, 16384)];
}
const PSG_MML_LINES = [
  ['cd /sd/0; play -o v.mml /ram/v.zsm; cmp /ram/v.zsm vpc.zsm && echo same', 'same'],
  ['play -o e.mml /ram/e.zsm; cmp /ram/e.zsm epc.zsm && echo same', 'same'],
  ['play -o f.mml /ram/f.zsm; cmp /ram/f.zsm fpc.zsm && echo same', 'same'],
  ["echo '[b0' 'A]'", '[b0 A]'], ["echo '[b0' 'B]'", '[b0 B]'], ["echo '[po' 'A]'", '[po A]'], ['play -o v.mml /ram/p.zsm', null],
  ["echo '[po' 'B]'", '[po B]'],
  ['play b1.mml; play b2.mml; play b3.mml; play b4.mml', 'play: b1.mml: channel 0: the other chip\'s instrument\nplay: b2.mml: channel 8: the other chip\'s instrument\nplay: b3.mml: channel 8: the YM2151\'s only\nplay: b4.mml: channel 9: a PSG register is 0-63'],
  ['play b5.mml; play b6.mml; play b7.mml; play b8.mml', 'play: b5.mml: an instrument it can\'t read\nplay: b6.mml: an instrument it can\'t read\nplay: b7.mml: an instrument it can\'t read\nplay: b8.mml: channel 23: a note before an instrument'],
  ['echo reset >/dev/sndctl; play v.mml; echo played', 'played'],
  ['play -m 8 o4 l8 c d e; play -x -m 9 I128 V40 O5 L4 C; play -c 13 o4 l4 c e g; play -c 21 c d e f', 'play: c: more notes than channels'],
];
// A ZSM's PSG voices' starts (a volume from 0 to more, a speaker on: vera.js's psgOns), each voice's count and its
// first one's tick
function zsmPsgOns(b) {
  const vol = new Array(16).fill(0), n = new Array(16).fill(0), first = new Array(16).fill(-1);
  let i = 16, t = 0;
  while (i < b.length) {
    const c = b[i++];
    if (c < 0x40) {
      const v = b[i++];
      if ((c & 3) === 2) {
        const k = c >> 2;
        if (!(vol[k] & 0x3F) && (v & 0x3F) && (v & 0xC0)) { n[k]++; if (first[k] < 0) first[k] = t; }
        vol[k] = v;
      }
    } else if (c === 0x40) i += b[i++] & 0x3F;
    else if (c < 0x80) i += 2 * (c & 0x3F);
    else if (c === 0x80) break;
    else t += c & 0x7F;
  }
  return { n, first };
}

// The PCM test's lines (with a Vera X: vid's /pcm and /pcmctl): its files and state; the rate (the VERA's nearest)
// and volume; raw samples from a card, drained; bad commands; /pcm one task's (another's pcmctl command: busy);
// WAV files played (8 bits mono, made signed; 16 bits stereo, past a chunk of an odd size; a float one, not a song);
// a ZSM's PCM instruments (one, then a looped one, stopped by the FIFO emptied: from RAM), its claim of the PCM as
// it plays, the state it leaves; one too big for RAM (from the file), stopped after half a second.  (A redirection is
// its rc's: another task's command comes through rc -c.)  The FIFO's bytes, and its runs dry, checked in check
const PCM_LINES = [
  ["ls /dev/vid | grep pcm; cat /dev/vid/pcmctl", "pcm\npcmctl\nrate 0\nbits 8\nmono\nvolume 15\nclaimed"],
  ["echo rate 22050 >/dev/vid/pcmctl; echo volume 12 >/dev/vid/pcmctl; grep -v claimed /dev/vid/pcmctl", "rate 22126\nbits 8\nmono\nvolume 12"],
  ["echo rate 3800 >/dev/vid/pcmctl; grep rate /dev/vid/pcmctl", "rate 3815"],
  ["cat /sd/0/tone.raw >/dev/vid/pcm; echo drain >/dev/vid/pcmctl; echo drained", "drained"],
  ["echo bits 12 >/dev/vid/pcmctl; echo volume 16 >/dev/vid/pcmctl; echo rate fast >/dev/vid/pcmctl",
    Array(3).fill('echo: write error: invalid argument').join('\n')],
  ["{rc -c 'echo bits 16 >/dev/vid/pcmctl'; grep -c 'claimed [0-9]' /dev/vid/pcmctl} >[3]/dev/vid/pcm; grep claimed /dev/vid/pcmctl",
    "echo: write error: busy\n1\nclaimed"],
  ["play /sd/0/t.wav; echo $status", ""],
  ["play /sd/0/s.wav; grep bits /dev/vid/pcmctl", "bits 16"],
  ["play /sd/0/f.wav", "play: /sd/0/f.wav: not a song"],
  ["play /sd/0/p.zsm & sleep 1; grep -c 'claimed [0-9]' /dev/vid/pcmctl; wait", "1"],
  ["cat /dev/vid/pcmctl", "rate 3815\nbits 8\nmono\nvolume 10\nclaimed"],
  ["play /sd/0/q.zsm; grep -c claimed /dev/vid/pcmctl", "1"],
];

// The pcm test's samples (bytes a generator's, so the parts can't be mistaken for each other), its WAV files and its
// ZSMs: p.zsm two PCM instruments (8 bits mono), the second looped from 100; the stream sets the rate (AUDIO_RATE 10:
// 3,815 Hz) and volume (10), starts the first, 30 ticks on the second, 60 ticks on empties the FIFO (its volume 10
// again).  q.zsm one of 17,000 bytes (past play's RAM for them, 16K), started, 30 ticks on stopped
const pcmBytes = (n, seed) => { const b = Buffer.alloc(n); let x = seed >>> 0; for (let i = 0; i < n; i++) { x = (Math.imul(x, 1103515245) + 12345) >>> 0; b[i] = x >>> 24; } return b; };
const PCM_DATA = { tone: pcmBytes(6000, 1), t: pcmBytes(4000, 2), s: pcmBytes(2400, 3), i0: pcmBytes(1500, 4), i1: pcmBytes(600, 5),
  big: pcmBytes(17000, 6) };
function wavFile(rate, channels, bits, data, opt = {}) {
  const fmt = Buffer.alloc(16);
  fmt.writeUInt16LE(opt.tag || 1, 0); fmt.writeUInt16LE(channels, 2); fmt.writeUInt32LE(rate, 4);
  fmt.writeUInt32LE(rate * channels * bits / 8, 8); fmt.writeUInt16LE(channels * bits / 8, 12); fmt.writeUInt16LE(bits, 14);
  const chunk = (id, b) => { const h = Buffer.alloc(8); h.write(id, 0, 'latin1'); h.writeUInt32LE(b.length, 4); return Buffer.concat([h, b, Buffer.alloc(b.length & 1)]); };
  const body = Buffer.concat([Buffer.from('WAVE', 'latin1'), chunk('fmt ', fmt), ...(opt.list ? [chunk('LIST', Buffer.from('hydra', 'latin1'))] : []), chunk('data', data)]);
  const h = Buffer.alloc(8); h.write('RIFF', 0, 'latin1'); h.writeUInt32LE(body.length, 4);
  return Buffer.concat([h, body]);
}
function pcmSong(stream, insts, data) {
  const at = 16 + stream.length;
  const hdr = [0x7A, 0x6D, 1, 0, 0, 0, at & 255, at >> 8, 0, 0, 0, 0, 60, 0, 0, 0];
  const inst = ([idx, off, len, loop, lp]) => {
    const b = Buffer.alloc(16); b[0] = idx; b.writeUIntLE(off, 2, 3); b.writeUIntLE(len, 5, 3); b[8] = loop ? 0x80 : 0; b.writeUIntLE(lp, 9, 3); return b;
  };
  return Buffer.concat([Buffer.from(hdr), Buffer.from(stream), Buffer.from('PCM', 'latin1'), Buffer.from([insts.length - 1]), ...insts.map(inst), ...data]);
}
function PCM_SONG() {
  const { i0, i1 } = PCM_DATA;
  return pcmSong([0x40, 0x04, 1, 10, 0, 10, 0x40, 0x02, 2, 0, 0x80 | 30, 0x40, 0x02, 2, 1, 0x80 | 60, 0x40, 0x02, 0, 0x8A, 0x80 | 10, 0x80],
    [[0, 0, i0.length, false, 0], [1, i0.length, i1.length, true, 100]], [i0, i1]);
}
function PCM_BIG() {
  return pcmSong([0x40, 0x04, 1, 10, 2, 0, 0x80 | 30, 0x40, 0x02, 0, 0x8A, 0x80], [[0, 0, PCM_DATA.big.length, false, 0]], [PCM_DATA.big]);
}
function pcmCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'pcm0.img');
  hydrafs.mkfs(f, 8, 'PCM', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('tone.raw', PCM_DATA.tone);
  v.put('t.wav', wavFile(3819, 1, 8, PCM_DATA.t));
  v.put('s.wav', wavFile(1144, 2, 16, PCM_DATA.s, { list: true }));
  v.put('f.wav', wavFile(3819, 1, 8, PCM_DATA.t, { tag: 3 }));
  v.put('p.zsm', PCM_SONG());
  v.put('q.zsm', PCM_BIG());
  v.close();
  return [imageCard(0, f, 16384)];
}

// The song player's test lines (as the tools test's): play's errors; a file run by its name that isn't a program
// (no #!); a song from a card (PLAY_SONG, at 60 Hz: its key-ons timed in check) with its channels claimed while it
// plays and given back when it's stopped; scom, a song that runs by its name (an rc script); the C sample jukebox
// (snd_play)
const PLAY_LINES = [
  ["play; echo $status","usage: play [-l] song [n]; play -o score.mml song.zsm; play [-lx] -m|-c ch mml\nusage"],
  ["/rom/README; whatis scom","rc: /rom/README: not a program\n/bin/scom"],
  ["play /rom/README; echo $status","play: /rom/README: not a song\nnot a song"],
  ["play /rom/nosuch; echo $status","play: /rom/nosuch: not found\n1"],
  ["play /sd/0/t.zsm & sleep 4; cat /dev/sndctl; kill $apid; wait; cat /dev/sndctl", [
    "volume 100",
    "channels 8",
    "claimed 0 1 2 3 4 5",
    "volume 100",
    "channels 8",
    "claimed",
  ].join('\n')],
  ["scom & sleep 1; cat /dev/sndctl; slay play; wait; cat /dev/sndctl","volume 100\nchannels 8\nclaimed 0 1\nvolume 100\nchannels 8\nclaimed"],
  ["/sd/0/sample/c/jukebox /rom/songs/scom.zsm 2","2\n1\nstopped: 137"],
];
// The play test's song: 60 Hz, six FM channels (a voice each, set up in tick 0), then a note every 2 ticks, round
// the channels, 240 of them (as dense as the X16's tunes: songs come from cards); and its card, SD device 0
function PLAY_SONG() {
  const fm = pairs => {                                       // (63 pairs a command at most)
    const out = [];
    for (let i = 0; i < pairs.length; i += 126) out.push(0x40 | pairs.slice(i, i + 126).length / 2, ...pairs.slice(i, i + 126));
    return out;
  };
  const voices = [];
  for (let ch = 0; ch < 6; ch++) {
    voices.push(0x20 + ch, 0xC7, 0x38 + ch, 0x00);
    for (const op of [0x00, 0x08, 0x10, 0x18]) voices.push(0x40 + op + ch, 0x01, 0x60 + op + ch, 0x10, 0x80 + op + ch, 0x1F, 0xA0 + op + ch, 0x00,
      0xC0 + op + ch, 0x00, 0xE0 + op + ch, 0x0F);
  }
  const notes = [];
  for (let k = 0; k < 240; k++) { const ch = k % 6; notes.push(...fm([0x08, ch, 0x28 + ch, 0x30 + k % 12, 0x08, 0x78 | ch]), 0x82); }
  const hdr = [0x7A, 0x6D, 1, 0, 0, 0, 0, 0, 0, 0x3F, 0, 0, 60, 0, 0, 0];
  return Buffer.from([...hdr, ...fm(voices), 0x81, ...notes, 0x80]);
}
function playCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'play0.img');
  hydrafs.mkfs(f, 8, 'SONGS', undefined, true);
  const v = new hydrafs.Volume(f);
  v.put('t.zsm', PLAY_SONG());
  v.mkdir('sample/c');                                        // (jukebox, as the SD card has it: bin/sdcard.img's)
  v.put('sample/c/jukebox', fs.readFileSync(path.join(TESTS, '..', 'obj', 'samples', 'c', 'jukebox.hyx')));
  v.close();
  return [imageCard(0, f, 16384)];
}

// The mml test's card: scores (play's: modules/play/mml.inc, hysong.js's language) and hysong.js's ZSMs of them (the
// PC's compiler, sim/tools/hysong.js): the old system's test song (os_rom/songs/test.mml: all 8 channels and
// algorithms, the LFO's 4 waveforms, noise, slides, legato, drums, repeats, the timers) as t.mml and tpc.zsm, the
// riff scom (programs/songs) as s.mml and spc.zsm; and two that are wrong (a note before an instrument, a line
// that isn't one)
const MML_SCORES = { t: path.join(TESTS, '..', '..', 'old', 'os_rom', 'songs', 'test.mml'), s: path.join(TESTS, '..', '..', 'old', 'programs', 'songs', 'scom.mml') };
function mmlCard() {
  fs.mkdirSync(CARD_DIR, { recursive: true });
  hydrafs.setNow(0x1000);
  const f = path.join(CARD_DIR, 'mml0.img');
  fs.rmSync(f, { force: true });
  hydrafs.mkfs(f, 8, 'SCORES', undefined, true);
  const v = new hydrafs.Volume(f);
  for (const [n, score] of Object.entries(MML_SCORES)) {
    const zsm = path.join(CARD_DIR, 'mml-' + n + '.zsm');
    require('child_process').execFileSync(process.execPath, [path.join(TESTS, '..', 'sim', 'tools', 'hysong.js'), score, zsm, '--quiet']);
    v.put(n + '.mml', fs.readFileSync(score));
    v.put(n + 'pc.zsm', fs.readFileSync(zsm));
  }
  v.put('bad.mml', Buffer.from('#tempo 100\nA o4 c d e\n'));
  v.put('bad2.mml', Buffer.from('@p { gm 0 }\nA @p c\nZ c d e\n'));
  v.close();
  return [imageCard(0, f, 16384)];
}

// A ZSM song's key-ons (a file, or its bytes): { rate, ticks: [the song tick of each] }
function ZSM_KEYONS(song) {
  const b = Buffer.isBuffer(song) ? song : fs.readFileSync(song), ticks = [];
  let i = 16, tick = 0;
  while (i < b.length) {
    const c = b[i++];
    if (c < 0x40) i++;
    else if (c === 0x40) i += 1 + (b[i] & 0x3F);
    else if (c < 0x80) { for (let k = 0; k < (c & 0x3F); k++, i += 2) if (b[i] === 8 && (b[i + 1] & 0x78)) ticks.push(tick); }
    else if (c === 0x80) break;
    else tick += c & 0x7F;
  }
  return { rate: (b[12] | b[13] << 8) || 60, ticks };
}

module.exports = {
  tests: [
    {
      name: 'chorus', what: 'the chorus sample (sdk/c/samples/chorus: the console shared) at rc: four tasks sing a line each, a letter at a time; with nothing between them the letters tangle; with a mutex each line is whole (in the order they took it); with a baton (a semaphore each, passed round) whole and in turn',
      init: 't_rc', cycles: 200e6,
      machine: { input: 'ā/sd/0/sample/c/chorus\r' },
      expect: ['With a baton passed round, a semaphore each (wait for yours, sing, pass it on):\n' + CHORUS.join('\n') + '\n'],
      check(m, out) {
        const f = [], a = out.indexOf('With a mutex, held for a whole line:\n'), b = out.indexOf('\n\nWith a baton');
        const mutex = out.slice(a, b).split('\n').slice(1);
        if (JSON.stringify([...mutex].sort()) !== JSON.stringify([...CHORUS].sort())) f.push('with a mutex, not the four lines whole: ' + JSON.stringify(mutex));
        const tangled = out.slice(out.indexOf('With nothing to keep them apart:\n'), a);
        if (CHORUS.every(l => tangled.includes(l))) f.push('with nothing between them, the lines came out whole');
        this.notes = ['with a mutex, in the order ' + mutex.map(l => CHORUS.indexOf(l) + 1).join(' ')];
        return f;
      },
      sdcard: true
    },
    {
      name: 'round', what: 'the round sample (sdk/c/samples/round: a barrier, then each task its own time) at rc, as a copy named /ram/rnd (the build\'s, through /pc: only $prog can find it): four tasks sing a round on YM2151 channels 0-3, an eighth 12 ticks; each voice\'s 27 notes in time (hy_sleep_until: within 5 ticks), each voice two bars after the one before',
      init: 't_rc', cycles: 220e6,
      pc: { files: () => ({ 'round.hyx': fs.readFileSync(path.join(TESTS, '..', 'obj', 'samples', 'c', 'round.hyx')) }) },
      machine: { input: 'ācp /pc/round.hyx /ram/rnd; /ram/rnd 1 12\r' },
      expect: ['Each voice sang it 1 time, its latest note late by', ' ticks.\n%'],
      check(m, out) {
        // (Each channel's key-ons against the tune's: its note lengths, an eighth 12 ticks, from voice 0's first,
        // voice v's 2 bars (12 eighths) on: each within 5 ticks, a task woken waiting its turn for the CPU)
        const f = [], tick = 3579545 / 200, lens = ROUND_LENS, on = [];
        for (let v = 0; v < 4; v++) on.push(m.ym.keyOns.filter(k => k.startsWith('ch ' + v + ' ')).map(k => +k.match(/at cycle (\d+)/)[1]));
        if (on.some(o => o.length !== lens.length)) return ['key-ons on channels 0-3: ' + on.map(o => o.length).join(', ') + ', not ' + lens.length + ' each'];
        let worst = 0;
        for (let v = 0; v < 4; v++) {
          let at = on[0][0] + v * 12 * 12 * tick;
          lens.forEach((len, k) => {
            const off = (on[v][k] - at) / tick;
            worst = Math.max(worst, Math.abs(off));
            if (Math.abs(off) > 5) f.push('voice ' + (v + 1) + '\'s note ' + (k + 1) + ': ' + off.toFixed(1) + ' ticks from its time');
            at += len * 12 * tick;
          });
        }
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        this.notes = ['the notes within ' + worst.toFixed(1) + ' ticks of their times'];
        return f.slice(0, 5);
      },
      sdcard: true
    },
    {
      name: 'sound', what: 'the simulator\'s sound (sim/lib/audio.js: run.js --sound, --wav), at rc: the YM2151\'s (opm.js, ymfm\'s) A4 on channel 4, then the Vera X\'s PSG\'s A5 on a sawtooth (channel 8), each heard at its pitch; the stream 48,000 samples a second of the Hydra\'s time',
      init: 't_rc', cycles: 60e6, jsOnly: 'the danlang emulator has no sound',
      machine: {
        vera: true, sound: true,
        input: ['echo patch 4 0 >/dev/sndctl; echo note 4 69 >/dev/sndctl', 'sleep 1; echo off 4 >/dev/sndctl; echo wave 8 saw >/dev/sndctl; echo note 8 81 >/dev/sndctl',
          'sleep 1; echo off 8 >/dev/sndctl'].map(l => 'ā' + l + '\r').join(''),
      },
      start(m) { this.heard = []; m.audio.on(s => this.heard.push(s)); },
      expect: ['% sleep 1; echo off 8 >/dev/sndctl\n%'],
      check(m) {
        const f = [], n = this.heard.reduce((k, s) => k + s.length / 2, 0), L = new Float64Array(n);
        let k = 0;
        for (const s of this.heard) for (let i = 0; i < s.length; i += 2) L[k++] = s[i];
        // Each 0.1 s that sounds: its pitch, the shortest period (60-2000 Hz) the samples repeat at (a correlation
        // over 0.9 at a local peak)
        const pitches = [];
        for (let s = 0; s + 4800 <= n; s += 4800) {
          let e = 0;
          for (let i = s; i < s + 4800; i++) e += L[i] * L[i];
          if (Math.sqrt(e / 4800) < 300) continue;
          const corr = lag => { let c = 0, e1 = 0, e2 = 0; for (let i = s; i < s + 2400; i++) { c += L[i] * L[i + lag]; e1 += L[i] * L[i]; e2 += L[i + lag] * L[i + lag]; } return c / Math.sqrt(e1 * e2); };
          for (let lag = 24, prev = corr(23); lag < 800; lag++) {
            const c = corr(lag);
            if (c > 0.9 && c >= prev && c >= corr(lag + 1)) { pitches.push(48000 / lag); break; }
            prev = c;
          }
        }
        const near = hz => pitches.filter(p => Math.abs(p - hz) < hz * 0.01).length;
        if (near(440) < 3) f.push('the YM2151\'s A4 not heard: ' + pitches.map(p => p.toFixed(1)).join(', '));
        if (near(880) < 3) f.push('the PSG\'s A5 not heard: ' + pitches.map(p => p.toFixed(1)).join(', '));
        const want = m.cpu.cyc / 3.579545e6 * 48000;
        if (Math.abs(m.audio.made - want) > 16) f.push('the stream: ' + m.audio.made + ' samples for ' + Math.round(want) + ' of the Hydra\'s time');
        this.notes = [n + ' samples heard; 0.1 s pitches near A4 ' + near(440) + ', near A5 ' + near(880)];
        return f;
      },
    },
    {
      name: 'snd', what: 'sound (#a): snd, sndctl and bell; the volume, claims (one another program holds), the shadow, tones (C, snd.h); sndctl\'s channel commands as text (patch, note, level and vol, pan by word and number, bend below 0, off, drum, reg: their registers on the chip; a channel another program has; numbers out of range, or missing); freq (a note by its frequency, its 64ths), glide, lfo, sens, noise',
      init: 't_rc', cycles: 140e6,
      get machine() { return { input: SND_LINES.map(l => '\u0101' + l[0] + '\r').join('') }; },
      get expect() { return SND_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')); },
      // (Channel 4: A4 bent down 32 64ths, G#4 and a half: key code $49 (octave 4, G#), fraction $80; on the left
      // (RL 01).  5: on the right (RL 10), a drum's patch kept it so.  6: $2E and $36 as written)
      check(m) {
        const f = [], keys = m.ym.keyOns.join(', '), r = m.ym.regs, hx = v => '$' + v.toString(16).toUpperCase();
        for (const ch of [0, 1, 2, 3, 4, 5, 7]) if (!m.ym.keyOns.some(k => k.startsWith('ch ' + ch + ' '))) f.push('no key-on on channel ' + ch + ': ' + keys);
        for (const [reg, want, mask, what] of [[0x2C, 0x49, 0xFF, 'note 4 69, bend 4 -32: its key code'], [0x34, 0x80, 0xFC, 'its fraction'],
          [0x24, 0x40, 0xC0, 'pan 4 left'], [0x25, 0x80, 0xC0, 'pan 5 2 (a drum after it)'], [0x2E, 0x4A, 0xFF, 'reg 46 74'], [0x36, 0xFC, 0xFF, 'reg 54 252'],
          [0x2A, 0x5D, 0xFF, 'freq 2 1000: its key code (B5)'], [0x32, 0x34, 0xFC, 'its fraction (13 64ths)'], [0x2B, 0x4E, 0xFF, 'glide 3 72 (C5)'],
          [0x33, 0x00, 0xFC, 'its fraction'], [0x18, 200, 0xFF, 'lfo: the rate'], [0x19, 20, 0xFF, 'its amplitude depth (the last written)'],
          [0x1B, 2, 0x03, 'its waveform'], [0x3B, 0x52, 0xFF, 'sens 3 5 2'], [0x0F, 0x89, 0xFF, 'noise 9']])
          if ((r[reg] & mask) !== want) f.push(what + ': register ' + hx(reg) + ' is ' + hx(r[reg]) + ', not ' + hx(want) + (mask !== 0xFF ? ' (mask ' + hx(mask) + ')' : ''));
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        this.notes = ['the YM2151: ' + m.ym.keyOns.length + ' key-ons'];
        return f;
      },
      sdcard: true
    },
    {
      name: 'mml', what: 'scores (play\'s, modules/play/mml.inc: hysong.js\'s language compiled as it plays): play -o\'s ZSM of the old test song (every channel, algorithm and LFO waveform, noise, slides, legato, drums, repeats, the timers) and of scom, each the same as hysong.js\'s byte for byte; scom played as a score and as hysong.js\'s ZSM, the chip\'s writes the same, in the same order, and in time; play -m (a line on a channel, its own instrument), -c (a chord, a note a channel), -x (the X16\'s MML: T, upper-case notes, S0 legato, K), I (a patch by number); a score\'s errors, the lines\'',
      init: 't_rc', cycles: 700e6,
      get machine() {
        return { sd: mmlCard(), ymLog: true, input: ['cd /sd/0', 'play -o t.mml /ram/t.zsm; cmp /ram/t.zsm tpc.zsm && echo same',
          'play -o s.mml /ram/s.zsm; cmp /ram/s.zsm spc.zsm && echo same', 'play bad.mml; echo $status', 'play bad2.mml',
          'echo reset >/dev/sndctl; play spc.zsm; echo reset >/dev/sndctl; play s.mml; echo played',
          'echo patch 0 0 >/dev/sndctl; echo patch 1 0 >/dev/sndctl; play -m 0 o4 l8 c d e; play -c 0 o4 l2 I0 c e g',
          'play -x -m 1 T240 O4 L8 CDE S0 CD K E; echo lines', 'play -m 24 c; play -c 0 c d e f g a b c d',
          'play -m 0 I0 c t100; play -x -m 0 I0 c Z'].map(l => '\u0101' + l + '\r').join('') };
      },
      expect: ['cmp /ram/t.zsm tpc.zsm && echo same\nsame\n%', 'cmp /ram/s.zsm spc.zsm && echo same\nsame\n%',
        'play bad.mml; echo $status\nplay: bad.mml: channel 0: a note before an instrument\nchannel 0: a note before an ins\n%',
        'play bad2.mml\nplay: bad2.mml: what is this line?\n%', 'echo played\nplayed\n%', 'echo lines\nlines\n%',
        'play -c 0 c d e f g a b c d\nusage: play [-l] song [n]; play -o score.mml song.zsm; play [-lx] -m|-c ch mml\nplay: c: more notes than channels\n%',
        'play -x -m 0 I0 c Z\nplay: I0: channel 0: t is a tempo, before any notes\nplay: I0: channel 0: what is Z\n%'],
      // (The lines' key-ons, the last 11, each with its channel's key code then: -m's C4 D4 E4 an eighth apart (50
      // ticks at 120), -c's C4 E4 G4 on 0-2 at once, -x's C D E an eighth apart at 240 (25 ticks), then after S0 a
      // C keyed, the D after it not (legato), and K's E)
      lineKeys: [[0, 0x3E, 0], [0, 0x41, 50], [0, 0x44, 100], [0, 0x3E], [1, 0x44], [2, 0x48], [1, 0x3E, 0], [1, 0x41, 25], [1, 0x44, 50], [1, 0x3E, 75], [1, 0x44, 125]],
      // (The two plays' writes: each from the reset before it (its $14, then $01-$FF and the channels' $20s), the
      // song's after it; the same registers and values in the same order, each one's time from the song's first
      // within 4 ticks of the other's)
      check(m) {
        const w = m.ym.writes, starts = [];
        for (let i = 0; i < w.length; i++) if (w[i][1] === 0x14 && w[i][2] === 0x30 && w[i + 1] && w[i + 1][1] === 0x01) starts.push(i);
        if (starts.length < 2) return ['the resets before the two plays: ' + starts.length + ' found'];
        const RESET = 256 + 8, s0 = starts[starts.length - 2], s1 = starts[starts.length - 1];
        const a = w.slice(s0 + RESET, s1), b = w.slice(s1 + RESET, s1 + RESET + a.length), tick = 3579545 * (JSON.parse(fs.readFileSync(path.join(TESTS, '..', 'obj', 'build.json'), 'utf8')).clock || 1) / 200;
        if (!a.length || a.length !== b.length) return ['the ZSM played ' + a.length + ' writes, the score ' + b.length];
        let worst = 0;
        for (let i = 0; i < a.length; i++) {
          if (a[i][1] !== b[i][1] || a[i][2] !== b[i][2]) return ['write ' + i + ': the ZSM\'s $' + a[i][1].toString(16) + ' = ' + a[i][2] + ', the score\'s $' + b[i][1].toString(16) + ' = ' + b[i][2]];
          worst = Math.max(worst, Math.abs((b[i][0] - b[0][0]) - (a[i][0] - a[0][0])) / tick);
        }
        this.notes = ['scom: ' + a.length + ' writes, the same both ways; their times within ' + worst.toFixed(2) + ' ticks of each other'];
        if (worst > 4) return ['the score\'s writes ' + worst.toFixed(2) + ' ticks from the ZSM\'s'];
        const kc = [], ons = [];
        for (const [t, r, v] of w) { if (r >= 0x28 && r < 0x30) kc[r & 7] = v; if (r === 0x08 && (v & 0x78)) ons.push([v & 7, kc[v & 7], t]); }
        const last = ons.slice(-this.lineKeys.length), f = [];
        let at0 = 0;
        this.lineKeys.forEach(([ch, code, ticks], i) => {
          const [c, k, t] = last[i] || [];
          if (ticks === 0) at0 = t;
          if (c !== ch || k !== code) f.push('the lines\' key-on ' + i + ': channel ' + c + ', key code $' + (k || 0).toString(16) + ' (not ' + ch + ', $' + code.toString(16) + ')');
          else if (ticks !== undefined && Math.abs((t - at0) / tick - ticks) > 1) f.push('the lines\' key-on ' + i + ': at tick ' + ((t - at0) / tick).toFixed(1) + ', not ' + ticks);
        });
        return f;
      },
    },
    {
      name: 'play', what: 'the song player: its errors; a song timed (its key-ons against its stream), its channels claimed and given back; scom; jukebox',
      init: 't_rc', cycles: 150e6,
      get machine() { return { input: PLAY_LINES.map(l => '\u0101' + l[0] + '\r').join(''), sd: playCard() }; },
      get expect() { return PLAY_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : (l[1] === null ? '' : l[1] + '\n') + '%')); },
      // (The card's song's key-ons from its 20th song tick to its 60th key-on, each against its song tick: they keep
      // time to within two system ticks, so the tempo neither drifts nor jitters more.  The first ticks are left out:
      // tick 0 sets six voices up, hundreds of writes, and its key-ons go out late)
      check(m) {
        const f = [], mult = JSON.parse(fs.readFileSync(path.join(TESTS, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const song = ZSM_KEYONS(PLAY_SONG());
        const cyc = m.ym.keyOns.map(k => +k.match(/at cycle (\d+)/)[1]), N = 60, K0 = song.ticks.findIndex(t => t >= 20);
        const perTick = 3579545 * mult / song.rate, slack = 2 * 3579545 * mult / 200;
        if (cyc.length < N) return ['the song: ' + cyc.length + ' key-ons (' + N + ' wanted)'];
        const e = [];
        for (let k = K0; k < N; k++) e.push(cyc[k] - song.ticks[k] * perTick);
        const spread = Math.max(...e) - Math.min(...e), span = (song.ticks[N - 1] - song.ticks[K0]) * perTick;
        this.notes = ['the song (60 Hz) timed: key-ons ' + K0 + '-' + (N - 1) + ' over ' + Math.round(span) + ' cycles, off their times by ' +
          Math.round(spread) + ' cycles at most from each other (at most ' + Math.round(slack) + ': two system ticks)'];
        if (spread > slack) f.push('the song: its key-ons ' + Math.round(spread) + ' cycles apart from their times (two system ticks: ' + Math.round(slack) + ')');
        if (m.ym.lost) f.push(m.ym.lost + ' writes to the YM2151 while it was busy');
        return f;
      },
    },
    {
      name: 'pcm', what: 'the Vera X\'s PCM (vid\'s /pcm and /pcmctl), at rc: its files and state; the rate (the VERA\'s nearest) and volume; raw samples from a card into the FIFO, drained; bad commands; /pcm one task\'s (another\'s pcmctl: busy); WAV files played (8 bits mono, made signed; 16 bits stereo past an odd chunk; a float one, not a song); a ZSM\'s PCM instruments (one, then one looped, stopped by the FIFO emptied: from RAM) and its claim of the PCM; one too big for RAM (from the file); the FIFO\'s bytes in order, none lost, its runs dry only at the ends',
      init: 't_rc', cycles: 150e6,
      get machine() { return { input: typed(PCM_LINES), vera: { pcmLog: true }, sd: pcmCard() }; },
      get expect() { return expected(PCM_LINES); },
      // (The FIFO's bytes: the raw samples, the 8-bit WAV file's made signed, the 16-bit one's as they are, the first
      // instrument's, then the looped one's: whole, then its loop (bytes 100-599) again and again, a second's worth and
      // the FIFO's at most; then the big one's first, half a second's and the FIFO's.  It runs dry four times: at the
      // raw samples' end, each WAV file's, and the first instrument's; the FIFO emptied isn't one)
      check(m) {
        const f = [], log = m.vera.pcmLog, d = PCM_DATA;
        const want = Buffer.concat([d.tone, Buffer.from(d.t.map(b => b ^ 0x80)), d.s, d.i0]);
        const got = Buffer.from(log.slice(0, want.length));
        if (!got.equals(want)) {
          const at = [...want].findIndex((b, i) => got[i] !== b);
          f.push('the FIFO\'s bytes differ from byte ' + at + ' (of ' + want.length + ': the raw 6000, the WAV files\' 4000 and 2400, the first instrument\'s 1500)');
        }
        const rest = log.slice(want.length), i1 = d.i1, big = d.big;
        let k = 0;                                            // (The looped one's, till the big one's start)
        while (k < rest.length && rest[k] === (k < 600 ? i1[k] : i1[100 + (k - 600) % 500]) && !(k >= 600 && rest[k] === big[0] && rest[k + 1] === big[1] && rest[k + 2] === big[2])) k++;
        if (k < 3000 || k > 9000) f.push('the looped instrument: ' + k + ' bytes to the FIFO (3000-9000 wanted)');
        const tail = rest.slice(k);
        if (!Buffer.from(tail).equals(big.subarray(0, tail.length))) f.push('the big instrument\'s bytes (' + tail.length + ') aren\'t its first');
        if (tail.length < 1500 || tail.length > 7000) f.push('the big instrument: ' + tail.length + ' bytes to the FIFO (1500-7000 wanted)');
        if (m.vera.pcmLost) f.push(m.vera.pcmLost + ' bytes written to a full FIFO');
        if (m.vera.pcmUnderruns !== 4) f.push('the FIFO ran dry ' + m.vera.pcmUnderruns + ' times (4 wanted: the ends)');
        this.notes = ['the FIFO: ' + log.length + ' bytes taken, ' + m.vera.pcmOut + ' played; the looped instrument ' + k + ', the big one ' + tail.length + '; dry ' + m.vera.pcmUnderruns + ' times'];
        return f;
      },
    },
    {
      name: 'psg', what: 'the Vera X\'s PSG as sound channels 8-23 (snd, through vid\'s /psg), at rc: sndctl\'s state (24 channels); a note (its frequency word), a waveform by name, by number and as a patch, speakers, a level, a frequency, a bend, a note off; claims of the PSG\'s channels (sndctl\'s second mask); the master volume on their volumes; errors; /psg and /dev/vid/psg; a note while the VERA\'s claimed, on the chip as the claim ends; hylang\'s and HyForth\'s snd-wave and claims of the PSG\'s channels; a ZSM of PSG writes played (play: its PSG channels claimed, its voices in time)',
      init: 't_rc', cycles: 180e6,
      get machine() { return { input: typed(PSG_LINES), vera: true, sd: psgCard() }; },
      get expect() {
        const claim = PSG_LINES.findIndex(l => l[2]);
        return expected(PSG_LINES.slice(0, claim)).concat(['% ' + PSG_LINES[claim][0] + '\nclaimed '], expected(PSG_LINES.slice(claim + 1)));
      },
      // (Each voice's four registers on the chip: its frequency word (Hz * 2^17 / 48,828.125), its speakers and
      // volume (63 less its attenuation * 1.5: level 64's 16 TL steps, the master volume 50's 17), its waveform and
      // width.  0: A4, the song's (bent down a semitone before, then off as its claim ended), off; 1: C4 on a sawtooth,
      // the song's too, off as it ended; 2: C5
      // on the left, at level 64; 3: 1000 Hz (B5 and 13 64ths); 4: a triangle of width 31; 5: patch 3, noise; 6: A4,
      // played while the chip was claimed; 7: hylang's A4 on a triangle of width 20; 8: HyForth's C4 on a sawtooth of
      // width 40)
      voices: [[1181, 0xC0, 0x3F], [702, 0xC0, 0x7F], [1405, 0x40 | 14, 0x3F], [2683, 0xC0 | 38, 0x3F], [0, 0xC0, 0x9F], [0, 0xC0, 0xFF], [1181, 0xC0 | 38, 0x3F], [1181, 0xC0 | 38, 0x94], [702, 0xC0 | 38, 0x68]],
      check(m) {
        const f = [], p = m.vera.psg, hx = v => '$' + v.toString(16).toUpperCase();
        this.notes = ['the PSG\'s voices on: ' + m.vera.psgOns.join(', ')];
        this.voices.forEach(([word, vol, wave], v) => {
          const w = p[v * 4] | p[v * 4 + 1] << 8;
          if (w !== word) f.push('voice ' + v + ': frequency word ' + w + ', not ' + word);
          if (p[v * 4 + 2] !== vol) f.push('voice ' + v + ': speakers and volume ' + hx(p[v * 4 + 2]) + ', not ' + hx(vol));
          if (p[v * 4 + 3] !== wave) f.push('voice ' + v + ': waveform and width ' + hx(p[v * 4 + 3]) + ', not ' + hx(wave));
        });
        for (let v = this.voices.length; v < 16; v++) if (p[v * 4 + 2] & 0x3F) f.push('voice ' + v + ': its volume ' + (p[v * 4 + 2] & 0x3F) + ' (none played)');
        // (The song's voices on: its voice 1 30 ticks after its voice 0, within two system ticks)
        const mult = JSON.parse(fs.readFileSync(path.join(TESTS, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const at = v => { const o = m.vera.psgOns.filter(k => k.startsWith('voice ' + v + ' ')).pop(); return o ? +o.match(/at cycle (\d+)/)[1] : NaN; };
        const gap = (at(1) - at(0)) / (3579545 * mult / 60);
        if (!(Math.abs(gap - 30) <= 2 * 60 / 200)) f.push('the song: its voice 1 on ' + gap.toFixed(2) + ' ticks after its voice 0 (30 wanted)');
        this.notes.push('the song: its voice 1 on ' + gap.toFixed(2) + ' song ticks after its voice 0 (30)');
        return f;
      },
    },
    {
      name: 'psgmml', what: 'the PSG in scores (play\'s, mml.inc, and hysong.js\'s: channels I-X, sound channels 8-23): play -o\'s ZSMs of the ROM disk\'s songs/vera.mml (both chips; each waveform; envelopes; slides, legato, triplets, ties; I, y, k, D, v, q, p) and of two scores of edge cases, each the same as hysong.js\'s byte for byte; the errors (the other chip\'s instrument, both ways; x; y past 63; instruments it can\'t read; channel 23\'s number); vera.mml played on the Vera X (its voices\' starts as many as its ZSM\'s, the lead\'s first 751 song ticks after the hats\'); a line (-m 8), the X16\'s (-x: I as the waveform register, V), a chord (-c 13), one with more notes than the PSG\'s channels left',
      init: 't_rc', cycles: 600e6,
      get machine() { return { input: typed(PSG_MML_LINES), vera: true, sd: psgScoreCard() }; },
      get expect() { return expected(PSG_MML_LINES); },
      // (October 2026: 36M, from 129M when play -o wrote what each step of the score made, a byte or two at a time)
      budgets: [{ what: 'play -o of songs/vera.mml into /ram (10.5K, 256 bytes a write)', from: '[po A]', to: '[po B]', minus: ['[b0 A]', '[b0 B]'],
        per: 1, max: 45e6 }],
      // (Each voice's registers at the end (its frequency word, its speakers and volume, its waveform; null: any).
      // 0: -m 8's E4 (MIDI 64), on the song's lead's pulse of width 24, off; 1: -x's C5 on its triangle of width 0
      // (I128), off; 2, 3, 4: the song's hats (noise), pad (saw) and chirp (a square), off; 5-7: the chord's C4 E4 G4,
      // off; 12: the chirp's y 51,191 (its waveform register))
      voices: [[64, 0xC0, 0x18], [72, 0xC0, 0x80], [null, 0xC0, 0xFF], [null, 0xC0, 0x7F], [null, 0xC0, 0x3F], [60, 0xC0, null], [64, 0xC0, null], [67, 0xC0, null],
        [null, null, null], [null, null, null], [null, null, null], [null, null, null], [null, null, 0xBF]],
      check(m) {
        const f = [], p = m.vera.psg, hx = v => '$' + v.toString(16).toUpperCase();
        const { psgWord } = require('../../sim/tools/hysong.js');
        this.voices.forEach(([note, vol, wave], v) => {
          const w = p[v * 4] | p[v * 4 + 1] << 8;
          if (note !== null && w !== psgWord(note * 64)) f.push('voice ' + v + ': frequency word ' + w + ', not ' + psgWord(note * 64));
          if (vol !== null && p[v * 4 + 2] !== vol) f.push('voice ' + v + ': speakers and volume ' + hx(p[v * 4 + 2]) + ', not ' + hx(vol));
          if (wave !== null && p[v * 4 + 3] !== wave) f.push('voice ' + v + ': waveform and width ' + hx(p[v * 4 + 3]) + ', not ' + hx(wave));
        });
        for (let v = 0; v < 16; v++) if (p[v * 4 + 2] & 0x3F) f.push('voice ' + v + ': its volume ' + (p[v * 4 + 2] & 0x3F) + ' at the end');
        // (The song's voices' starts: the ZSM's, and the lines' after it (-m's 3 on voice 0, -x's 1 on voice 1, the
        // chord's on 5-7); the lead's first (voice 0) 751 song ticks after the hats' (voice 2), within two system ticks)
        const z = zsmPsgOns(fs.readFileSync(path.join(CARD_DIR, 'psgmml-v.zsm'))), ons = new Array(16).fill(0), at = new Array(16).fill(-1);
        for (const o of m.vera.psgOns) { const [, v, c] = o.match(/voice (\d+) at cycle (\d+)/).map(Number); ons[v]++; if (at[v] < 0) at[v] = c; }
        const lines = [3, 1, 0, 0, 0, 1, 1, 1];
        for (let v = 0; v < 16; v++) if (ons[v] !== z.n[v] + (lines[v] || 0)) f.push('voice ' + v + ': ' + ons[v] + ' starts, not ' + (z.n[v] + (lines[v] || 0)));
        const mult = JSON.parse(fs.readFileSync(path.join(TESTS, '..', 'obj', 'build.json'), 'utf8')).clock || 1;
        const gap = (at[0] - at[2]) / (3579545 * mult / 200);
        if (!(Math.abs(gap - (z.first[0] - z.first[2])) <= 2 * 200 / 200)) f.push('the song: its lead on ' + gap.toFixed(2) + ' ticks after its hats (' + (z.first[0] - z.first[2]) + ' wanted)');
        this.notes = ['the song: ' + z.n.slice(0, 5).join(', ') + ' starts on voices 0-4; its lead on ' + gap.toFixed(2) + ' song ticks after its hats (' + (z.first[0] - z.first[2]) + ')'];
        return f;
      },
    },
  ],
};
