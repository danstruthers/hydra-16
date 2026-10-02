// ****************************************************************************
// tests/storage.js - regression tests (sim/regress.js runs them): SD cards and HydraFS: reading, writing, checking, formatting, partitions, stamps, sparse files.  A test's fields are described at
// the top of regress.js.
// ****************************************************************************
'use strict';
const { fs, path, hydrafs, hyx, W, BOOT, num, SPARSE_WRITES, SPARSE_SCRIPT, P } = require('./common.js');

module.exports = [
  {
    name: 'sd', about: '/dev/sd/0/data: write a byte at offset 512, read it back, and it\'s in the card image',
    sd: true,
    args: ['--cycles', '90000000', '--input', BOOT + 'q^/dev/sd/0/data^ 3 open .\\r90 here @ c!\\r' +
      '3 512 0 seek 3 here @ 1 write . ioerr .\\r91 here @ c!\\r3 512 0 seek 3 here @ 1 read . here @ c@ .\\r3 close\\r'],
    expect: ['open .\n' + num(3) + '\n', 'write . ioerr .\n' + num(1) + num(0) + '\n',
      'read . here @ c@ .\n' + num(1) + num(90) + '\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const img = fs.readFileSync(files.sd);
      if (img[512] !== 90) return 'the card image has $' + img[512].toString(16) + ' at 512, not $5A';
      if (img.some((b, i) => b && i !== 512)) return 'the card image changed somewhere else too';
    },
  },
  {
    name: 'sd-speed', about: 'SD read throughput: 4K in 256-byte reads, inside a cycle budget (the bit-banged SPI is most of it)',
    sd: true,
    args: ['--cycles', '200000000', '--mark', '> go', '--mark', '> ', '--input', BOOT +
      'ftrain autoload\\rq^/dev/sd/0/data^ 1 open .\\r' +
      ': go lit [ 16 , ] 0 do 3 here @ lit [ 256 , ] read drop loop ;\\r' + P + 'go\\r' + W(2)],
    expect: ['open .\n' + num(3) + '\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report) => {                                   // 298 cycles/byte now; SPI_RECV is about 60% of it
      const at = +/mark: "> go" at cycle (\d+)/.exec(report)[1];
      const took = [...report.matchAll(/mark: "> " at cycle (\d+)/g)].map(m => +m[1]).find(c => c > at) - at;
      const per = Math.round(took / 4096);
      if (per > 330) return 'an SD read took ' + per + ' cycles a byte, over the 330 budget';
    },
  },
  {
    name: 'sd-cards', about: 'SD cards on devices 0 (SDHC) and 1 (SDSC, CSD v1), none on 3: ctl files, SDSC data, init, bad names',
    sd: [{ dev: 0 }, { dev: 1, mb: 3, sdsc: true, fill: img => img.write('SDSC-BLK1', 512) }],
    args: ['--cycles', '150000000', '--input', BOOT +
      'q^/dev/sd/0/ctl^ 1 open 0 fdup2 cat | cat\\rq^/dev/sd/1/ctl^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/dev/sd/3/ctl^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/dev/sd/1/data^ 3 open .\\r3 512 0 seek 3 here @ 9 read . here @ 5 + c@ .\\r' +
      '66 here @ c! 3 1024 0 seek 3 here @ 1 write .\\r3 close\\r' +
      'q^/dev/sd/9/data^ 1 open\\rioerr .\\rq^/dev/sd/0/nope^ 1 open\\rioerr .\\rq^/dev/sd/3/data^ 1 open\\rioerr .\\r' +
      'q^/dev/sd/1/ctl^ 3 open .\\r105 here @ c! 110 here @ 1 + c! 105 here @ 2 + c! 116 here @ 3 + c!\\r' +
      '3 here @ 4 write .\\r3 here @ 2 write\\rioerr .\\r3 close\\r'],
    expect: ['| cat\nsdhc 1 MB 2048 blocks\n', '| cat\nsdsc 3 MB 6144 blocks\n', '| cat\nnone\n',
      'open .\n' + num(3) + '\n', 'c@ .\n' + num(9) + num(0x42) + '\n', 'write .\n' + num(1) + '\n',
      '!IO ERR!', '/ram> ioerr .\n' + num(0x70) + '\n', '!IO ERR!', '/ram> ioerr .\n' + num(0x70) + '\n',
      '!IO ERR!', '/ram> ioerr .\n' + num(0x79) + '\n',
      'open .\n' + num(3) + '\n', '4 write .\n' + num(4) + '\n', '!IO ERR!', '/ram> ioerr .\n' + num(0x78) + '\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const img = fs.readFileSync(files.sds[1]);
      if (img[1024] !== 66) return 'the SDSC card image has $' + img[1024].toString(16) + ' at 1024, not $42';
    },
  },
  {
    name: 'hydrafs', about: 'HydraFS reading: /sd/N, walking, "." and "..", file and directory reads, stat records, errors',
    sd: [{ dev: 0, mb: 2, label: 'TESTS', hfs: v => {
      v.put('hello.txt', Buffer.from('hello hydra\r\n'));
      v.mkdir('games');
      v.put('games/star.frt', Buffer.from(': star 42 . ;\r\n'));
      v.put('big.bin', Buffer.from(Array.from({ length: 5000 }, (_, i) => i & 0xFF)));
      v.mkdir('many');                                          // More entries than one block holds (8)
      for (let i = 0; i < 10; i++) v.put('many/f' + i, Buffer.from(String(i)));
      for (const n of 'abcdefg') v.put(n, Buffer.alloc(4096, n.charCodeAt(0)));
      for (const n of 'bdf') v.remove(n);                       // So "a" grows into three separate runs:
      v.write(v.walk('a'), 4096, Buffer.alloc(12288, 0x5A));    //   three extents, one in an extent block
    } }, { dev: 1 }],                                           // And a card with no HydraFS on it
    args: ['--cycles', '400000000', '--input', BOOT +
      'q^/sd/0^ 1 open 0 fdup2 cat | cat\\rq^/sd/0/many^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/sd/0/./games/../hello.txt^ 1 open 0 fdup2 cat | cat\\rq^/sd/0/games/..^ 1 open 0 fdup2 cat | cat\\r' +
      'q^/sd/0/a^ 1 open .\\r3 0 0 seek 3 here @ 1 read . here @ c@ .\\r' +
      '3 4096 0 seek 3 here @ 1 read . here @ c@ .\r3 16383 0 seek 3 here @ 4 read . here @ c@ .\r3 close\r' +
      'q^/sd/0/big.bin^ 1 open .\\r3 4095 0 seek 3 here @ 4 read . here @ c@ . here @ 1 + c@ .\\r3 close\\r' +
      'q^/sd/0/many^ 5 open .\\r3 here @ 48 read . here @ c@ . here @ 32 + c@ . here @ 40 + @ .\\r3 close\\r' +
      'q^/sd/1/x^ 1 open\\rioerr .\\rq^/sd/3/x^ 1 open\\rioerr .\\rq^/sd/8/x^ 1 open\\rioerr .\\r' +
      'q^/sd^ 1 open\\rioerr .\\rq^/sd/0/games^ 2 open\\rioerr .\\rq^/sd/0/hello.txt/x^ 1 open\\rioerr .\\r'],
    expect: [
      '| cat\nhello.txt 13\ngames/\nbig.bin 5000\nmany/\na 16384\nc 4096\ne 4096\ng 4096\n',
      '| cat\nf0 1\nf1 1\nf2 1\nf3 1\nf4 1\nf5 1\nf6 1\nf7 1\nf8 1\nf9 1\n',
      '| cat\nhello hydra\n',                                   // "." and ".." on the way (worked out by the IO layer)
      '| cat\nhello.txt 13\n',                                  // ".." back to the root
      'open .\n' + num(3) + '\n',                               // The three-extent file, a byte at a time
      'c@ .\n' + num(1) + num(0x61) + '\n',                     // Its first extent ("a")
      'c@ .\n' + num(1) + num(0x5A) + '\n',                     // The second (what was written at 4096)
      'c@ .\n' + num(1) + num(0x5A) + '\n',                     // The third, and 1 byte at the end of file
      'open .\n' + num(3) + '\n',                               // big.bin, over a cluster boundary
      '1 + c@ .\n' + num(4) + num(0xFF) + num(0) + '\n',
      'open .\n' + num(3) + '\n',                               // A directory as stat records (IO_MODE_STAT)
      '@ .\n' + num(48) + num(0x66) + num(0) + num(1) + '\n',   // "f0": 48 bytes, mode 0, size 1
      '0:/> ioerr .\n' + num(0x80) + '\n',                        // No HydraFS on the card (ERR_IO_NOT_FS)
      '0:/> ioerr .\n' + num(0x79) + '\n',                        // No card at all
      '0:/> ioerr .\n' + num(0x70) + '\n',                        // Card 8
      '0:/> ioerr .\n' + num(0x70) + '\n',                        // /sd, with no card in it
      '0:/> ioerr .\n' + num(0x72) + '\n',                        // A directory opened for writing
      '0:/> ioerr .\n' + num(0x70) + '\n',                        // A path through a file
    ],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const v = new hydrafs.Volume(files.sds[0]);               // Nothing on the card changed
      try { const p = v.check(); if (p.length) return 'the card is damaged: ' + p[0]; } finally { v.close(); }
    },
  },
  {
    name: 'hydrafs-write', about: 'HydraFS writing: a file grown into pieces (an extent block), truncate, append-only, create; the shell\'s mkdir, rm, rmdir, mv, ls; errors, format, a full card',
    sd: [{ dev: 0, mb: 2, label: 'WRITES', hfs: v => {
      for (const n of 'abcdefg') v.put(n, Buffer.alloc(4096, n.charCodeAt(0)));
      const at = v.extents(v.walk('b'))[0].start;
      for (const n of 'bdf') v.remove(n);
      v.hint = at;                                              // So the Hydra's clusters fill the holes first:
    } },                                                        //   a file in four pieces, one in an extent block
    { dev: 1 },                                                 // Blank: the Hydra formats it
    { dev: 2, label: 'TINY', blocks: 34, hfs: () => {} }],      // 4 clusters: it fills up
    args: ['--cycles', '220000000', '--input', BOOT + 'ftrain autoload\\r' + [
      ': wr lit [ 80 , ] 0 do 3 here @ lit [ 256 , ] write drop loop ;\\r' +
      ': fl lit [ 60 , ] 0 do 3 here @ lit [ 256 , ] write drop loop ;\\r' + P,
      'q^/sd/0/big^ 0 create .\\rwr 3 close\\r' + W(18) + 'ls /sd/0\\r',           // (20 KB: type-ahead would overflow)
      'q^/sd/0/t^ 0 create .\\r3 here @ 200 write . 3 close\\rq^/sd/0/t^ 10 open . 3 close\\r',
      'q^/sd/0/log^ 64 create .\\r3 here @ 4 write . 3 0 0 seek 3 here @ 4 write . 3 close\\r',
      'q^/sd/0/t^ 2 open .\\r3 100 0 seek 3 here @ 4 write\\rioerr .\\r3 close\\r',
      'q^/sd/0/ro^ 1 create . 3 here @ 4 write . 3 close\\rq^/sd/0/ro^ 2 open\\rioerr .\\r',
      'mkdir /sd/0/log\\rioerr .\\rmkdir sub\\rq^/sd/0/sub^ 0 create\\rioerr .\\r',   // (Names relative to /sd/0)
      'q^/sd/0/log^ 1 open .\\rrm log\\rioerr .\\r3 close\\r',
      'mv ro log\\rioerr .\\rmv log log2\\r',
      'q^/sd/0/sub/x^ 0 create . 3 close\\rrmdir sub\\rioerr .\\rrm c\\rls\\r',
      'q^/dev/sd/1/ctl^ 2 open .\\r3 q^format TEST^ @ 3 + 11 write . 3 close\\r' + P,
      'q^/sd/1/hi^ 0 create .\\r3 here @ 5 write . 3 close\\rls /sd/1\\r',
      'q^/sd/2/fill^ 0 create .\\rfl\\r' + W(14) + 'ioerr .\\r3 close\\rls /sd/2\\r'].join(W(2))],
    expect: [
      'ls /sd/0\na 4096\nbig 20480\nc 4096\ne 4096\ng 4096\n',   // 20 KB, in the holes and after
      '200 write . 3 close\n' + num(200) + '\n',
      '4 write . 3 close\n' + num(4) + num(4) + '\n',          // Append-only: the second write after the first
      '0:/> ioerr .\n' + num(0) + '\n',                           // A write past the end: zeros before it
      '0:/> ioerr .\n' + num(0x72) + '\n',                        // A read-only file (written as it was made)
      '0:/> ioerr .\n' + num(0x82) + '\n',                        // mkdir where there's a file
      '0:/> ioerr .\n' + num(0x82) + '\n',                        // A file where there's a directory
      '0:/> ioerr .\n' + num(0x84) + '\n',                        // Removing an open file
      '0:/> ioerr .\n' + num(0x82) + '\n',                        // Renaming to a name that's taken
      '0:/> ioerr .\n' + num(0x83) + '\n',                        // Removing a directory with a file in it
      '0:/> ls\na 4096\nbig 20480\nt 104\ne 4096\nlog2 8\ng 4096\nro 4\nsub/\n',   // (Free entries used first)
      '11 write . 3 close\n' + num(11) + '\n',                  // format TEST
      'ls /sd/1\nhi 5\n',
      '0:/> ioerr .\n' + num(0x81) + '\n',                        // The card is full
      'ls /sd/2\nfill 12288\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {                            // What the host tool finds on the cards
      const open = (dev, f) => { const v = new hydrafs.Volume(files.sds[dev]); try { return f(v); } finally { v.close(); } };
      for (const dev of [0, 1, 2]) {
        const p = open(dev, v => v.check());
        if (p.length) return 'card ' + dev + ': ' + p[0];
      }
      const e = open(0, v => {
        const big = v.walk('big'), d = v.read(big), ext = v.extents(big);
        if (ext.length < 3 || !big.extBlock) return 'big is in ' + ext.length + ' extents, with no extent block';
        for (let i = 256; i < d.length; i += 256) if (!d.subarray(i, i + 256).equals(d.subarray(0, 256))) return 'big\'s data is wrong at ' + i;
        const t = v.read(v.walk('t'));
        if (v.walk('log2').size !== 8 || t.length !== 104 || t.subarray(0, 100).some(b => b) || v.walk('ro').mode !== hydrafs.MODE_RO) return 'log2, t or ro is wrong';
      });
      if (e) return e;
      if (open(1, v => v.label) !== 'TEST') return 'card 1\'s label isn\'t TEST';
      if (open(2, v => v.freeCount) !== 0) return 'card 2 isn\'t full';
    },
  },
  {
    name: 'hydrafs-check', about: 'HydraFS check on /dev/sd/0/ctl: lost, unmarked and doubly used clusters over two passes, check fix, the ctl file\'s lines',
    sd: [{ dev: 0, label: 'CHECK', blocks: 600000, hfs: v => {    // 74997 clusters (two passes): a 1 MB image
      v.put('a', Buffer.alloc(100, 1)); v.put('b', Buffer.alloc(100, 2)); v.put('c', Buffer.alloc(5000, 3));
      v.mkdir('d'); v.put('d/e', Buffer.alloc(10, 4));
      v.setUsed(v.extents(v.walk('c'))[0].start + 1, false);  // Unmarked: c's second cluster
      const a = v.walk('a'), b = v.walk('b');
      b.setExt(0, a.ext(0)); v.writeEntry(b);                 // Twice: b uses a's cluster (b's own is lost)
      v.setUsed(70000, true);                                 // Lost, in the second pass
      const e = v.walk('d/e');
      e.setExt(0, { start: 70001, len: 1 }); v.writeEntry(e); // Unmarked in the second pass (e's own is lost)
      v.freeCount = 7;                                        // And the free count is wrong
    } }],
    args: ['--cycles', '120000000', '--input', BOOT + ['ls /dev/sd/0/ctl\\r', 'q^/dev/sd/0/ctl^ q^check^ ctl\\r' + W(3), 'ls /dev/sd/0/ctl\\r',
      'q^/dev/sd/0/ctl^ q^check fix^ ctl\\r' + W(3), 'ls /dev/sd/0/ctl\\r', 'q^/dev/sd/0/ctl^ q^check^ ctl\\r' + W(3), 'ls /dev/sd/0/ctl\\r'].join('')],
    expect: ['ctl\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 28 KB of 299988 KB\n',
      'ctl\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 299960 KB of 299988 KB\ncheck: lost 3, unmarked 2, twice 1\n',
      'ctl\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 299964 KB of 299988 KB\ncheck: lost 3, unmarked 2, twice 1, fixed\n',
      'ctl\nsdhc 1 MB 2048 blocks\nhydrafs label=CHECK\nfree 299964 KB of 299988 KB\ncheck: lost 0, unmarked 0, twice 1\n'],
    forbid: ['!IO ERR!', '!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {                            // The host tool agrees: only the shared cluster
      const v = new hydrafs.Volume(files.sds[0]);               //   is wrong (it needs a person), and the free
      try {                                                     //   count is right
        const p = v.check();
        if (p.length !== 1 || !/ too$/.test(p[0])) return 'the card: ' + JSON.stringify(p);
      } finally { v.close(); }
    },
  },
  {
    name: 'volumes', about: 'HyForth card words: vols, mkfs on a blank card, relabel, fsck, fsfix, a bad card number',
    sd: [{ dev: 0 }],
    args: ['--cycles', '100000000', '--input', BOOT + 'vols\\r' + P + '0 q^GAMES^ mkfs\\r' + P +
      'q^/sd/0/x^ 0 create . 3 close\\r0 q^TOYS^ relabel\\r0 fsck\\r0 fsfix\\r9 fsck\\rioerr .\\rvols\\r'],
    expect: ['/ram> vols\n0: sdhc 1 MB 2048 blocks\n1: none\n', '7: none\n',
      'mkfs\nsdhc 1 MB 2048 blocks\nhydrafs label=GAMES\nfree 1020 KB of 1020 KB\n',
      'relabel\nsdhc 1 MB 2048 blocks\nhydrafs label=TOYS\nfree 1016 KB of 1020 KB\n',
      'fsck\nsdhc 1 MB 2048 blocks\nhydrafs label=TOYS\nfree 1016 KB of 1020 KB\ncheck: lost 0, unmarked 0, twice 0\n',
      'check: lost 0, unmarked 0, twice 0, fixed\n', '/ram> ioerr .\n' + num(0x70) + '\n',
      '/ram> vols\n0: sdhc 1 MB 2048 blocks\nhydrafs label=TOYS\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {                            // The card the Hydra made: as the PC tool makes them
      const v = new hydrafs.Volume(files.sds[0]);
      try {
        const p = v.check();
        if (p.length) return 'the card: ' + p[0];
        if (v.label !== 'TOYS' || !v.tryWalk('x')) return 'the card has label "' + v.label + '", and x ' + (v.tryWalk('x') ? '' : 'isn\'t ') + 'on it';
      } finally { v.close(); }
    },
  },
  {
    name: 'partitions', about: 'HydraFS in a partition: one the PC tool made (read, written, checked), mkfs-part next to a FAT partition, format -p on a blank card, mkfs again in a partition',
    sd: [{ dev: 0, mb: 4, blocks: 2048, part: 1, label: 'PARTED', hfs: v => v.put('hello.txt', Buffer.from('in a partition\r\n')) },
      { dev: 1, mb: 4, fill: img => {                             // A card a PC partitioned: a FAT partition
        img.set([0xFE, 0xFF, 0xFF, 0x0C, 0xFE, 0xFF, 0xFF], 447);   //   (blocks 2048-5047), and its data
        img.writeUInt32LE(2048, 454); img.writeUInt32LE(3000, 458); img[510] = 0x55; img[511] = 0xAA;
        img.write('FAT DATA', 2048 * 512);
      } }, { dev: 2, mb: 4 }],
    args: ['--cycles', '150000000', '--input', BOOT + ['ls /dev/sd/0/ctl\\r', 'cat hello.txt\\recho more > new.txt\\rls\\r', '0 fsck\\r',
      '1 "NEWP" mkfs-part\\r', 'echo hi > /sd/1/a.txt\\rls /sd/1\\r', '1 "AGAIN" mkfs\\r',
      'echo format -p BLANKP > /dev/sd/2/ctl\\r', 'ls /dev/sd/2/ctl\\r'].join(P)],
    expect: ['hydrafs 0\n', '0:/> ls /dev/sd/0/ctl\nsdhc 3 MB 6144 blocks\nhydrafs label=PARTED\npartition at block 4096\nfree 1012 KB of 1020 KB\n',
      'cat hello.txt\nin a partition\n', '0:/> ls\nhello.txt 16\nnew.txt 6\n', 'check: lost 0, unmarked 0, twice 0\n',
      'mkfs-part\nsdhc 4 MB 8192 blocks\nhydrafs label=NEWP\npartition at block 6144\nfree 1020 KB of 1020 KB\n',
      'ls /sd/1\na.txt 4\n', 'mkfs\nsdhc 4 MB 8192 blocks\nhydrafs label=AGAIN\npartition at block 6144\nfree 1020 KB of 1020 KB\n',
      'ls /dev/sd/2/ctl\nsdhc 4 MB 8192 blocks\nhydrafs label=BLANKP\npartition at block 2048\nfree 3068 KB of 3068 KB\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!', '!IO ERR!'],
    check: (out, report, files) => {
      const open = (dev, f) => { const v = new hydrafs.Volume(files.sds[dev]); try { return f(v); } finally { v.close(); } };
      const e = open(0, v => v.check()[0] || (v.base !== 4096 ? 'at ' + v.base : '') || (v.tryWalk('new.txt') ? '' : 'no new.txt'));
      if (e) return 'card 0: ' + e;
      const img = fs.readFileSync(files.sds[1]), parts = hydrafs.mbrParts(img.subarray(0, 512));
      if (!parts || parts[0].type !== 0x0C || parts[0].start !== 2048 || parts[0].blocks !== 3000 ||
          img.toString('latin1', 2048 * 512, 2048 * 512 + 8) !== 'FAT DATA') return 'card 1\'s FAT partition changed';
      if (parts[1].type !== hydrafs.PART_TYPE || parts[1].start !== 6144 || parts[1].blocks !== 2048) return 'card 1\'s HydraFS partition: ' + JSON.stringify(parts[1]);
      const e1 = open(1, v => v.check()[0] || (v.label !== 'AGAIN' ? 'label ' + v.label : ''));
      if (e1) return 'card 1: ' + e1;
      const p2 = hydrafs.mbrParts(fs.readFileSync(files.sds[2]).subarray(0, 512));
      if (!p2 || p2[0].type !== hydrafs.PART_TYPE || p2[0].start !== 2048 || p2[0].blocks !== 6144 || p2[1].type) return 'card 2\'s table: ' + JSON.stringify(p2);
      return open(2, v => v.check()[0]);
    },
  },
  {
    name: 'clock', about: 'the clock: /dev/time from power-up, set and read; into a leap day and past one 2100 hasn\'t; bad dates; files\' stamps (ls -l, and the PC tool\'s dates)',
    sd: [{ dev: 0, label: 'CLOCK', hfs: () => {} }],
    args: ['--cycles', '120000000', '--input', BOOT + ['cat /dev/time\\r', 'echo 2026-09-29 18:05:30 > /dev/time\\recho a > a.txt\\rls -l a.txt\\r',
      'echo 2024-02-28 23:59:59 > /dev/time\\r' + W(2) + 'echo b > b.txt\\rcat /dev/time\\r',
      'echo 2100-02-28 23:59:59 > /dev/time\\r' + W(2) + 'cat /dev/time\\r',
      '"/dev/time" "2023-02-29" ctl\\rioerr .\\r"/dev/time" "2026-13-01" ctl\\r"/dev/time" "2026-01-01 24:00" ctl\\r"/dev/time" "1999-12-31" ctl\\r',
      '"/dev/time" "2135-12-31 23:59:59" ctl\\rcat /dev/time\\r'].join(P)],
    expect: ['0:/> cat /dev/time\n2000-01-01 00:00:0', 'ls -l a.txt\na.txt 3 2026-09-29 18:05:3', 'cat /dev/time\n2024-02-29 00:00:0',
      'cat /dev/time\n2100-03-01 00:00:0', 'ioerr .\n' + num(0x78) + '\n', '!IO ERR!', '!IO ERR!', '!IO ERR!', 'cat /dev/time\n2135-12-31 23:59:59\n'],
    forbid: ['!DS PTR ERROR!', '!UNK WORD!'],
    check: (out, report, files) => {                            // The stamps, as the PC tool reads them
      const v = new hydrafs.Volume(files.sds[0]);
      try {
        const a = hydrafs.stampText(v.walk('a.txt').stamp), b = hydrafs.stampText(v.walk('b.txt').stamp);
        if (!a.startsWith('2026-09-29 18:05:3')) return 'a.txt\'s stamp: ' + a;
        if (!b.startsWith('2024-02-29 00:00:0')) return 'b.txt\'s stamp: ' + b;
      } finally { v.close(); }
    },
  },
  {
    name: 'sparse', about: 'sparse files: writes past the end (a hole), into holes (split, the extents after moving along, into an extent block), across blocks and clusters; a 64 MB file on a 2 MB card; emptied',
    sd: [{ dev: 0, mb: 2, label: 'SPARSE', hfs: v => v.put('fill.hys', Buffer.from(SPARSE_SCRIPT)) }],
    args: ['--cycles', '400000000', '--input', BOOT + ['include fill.hys\\r' + W(60) + 'ls\\r', '0 fsck\\r',
      '"big" 0 create .\\r3 0 $400 seek 3 here @ 4 write .\\r3 close\\rls\\r0 fsck\\r',
      '"big" 0 create .\\r3 close\\r0 fsck\\r'].join(W(2))],
    expect: ['0:/> ls\nfill.hys ', 's ' + SPARSE_WRITES.reduce((m, o) => Math.max(m, o + 4), 0) + '\n',
      'check: lost 0, unmarked 0, twice 0\n', 'big 67108868\n', 'check: lost 0, unmarked 0, twice 0\n',
      'check: lost 0, unmarked 0, twice 0\n'],                  // (Emptied: big's cluster back, the PC tool checks)
    forbid: ['!DS PTR ERROR!', '!UNK WORD!', '!IO ERR!', '!LOW MEM!'],
    check: (out, report, files) => {                            // The file, as the PC tool reads it
      const v = new hydrafs.Volume(files.sds[0]);
      try {
        const p = v.check();
        if (p.length) return 'the card: ' + p[0];
        const big = v.walk('big');
        if (big.size || v.extents(big).length) return 'big wasn\'t emptied';
        const e = v.walk('s'), d = v.read(e), want = Buffer.alloc(d.length);
        for (const o of SPARSE_WRITES) want.write('WXYZ', o, 'latin1');
        if (!d.equals(want)) { let i = 0; while (d[i] === want[i]) i++; return 's differs at ' + i; }
        const x = v.extents(e);
        if (!x.some(y => y.start === hydrafs.HOLE) || !e.extBlock) return 's has no hole, or no extent block: ' + JSON.stringify(x);
      } finally { v.close(); }
    },
  },
  {
    name: 'cards', about: 'the fixture card images (sim/cards): a version 1 HydraFS and a quick-formatted version 2 one, read, written and checked',
    sd: [{ dev: 0, image: 'tests-v1.img', claim: 131072 }, { dev: 1, image: 'quick-v2.img', claim: 131072 }],
    args: ['--cycles', '150000000', '--input', BOOT + W(1) + ['ls\\r', 'cat hello.txt\\rcat games/star.frt\\r', 'ls many\\r',
      '"big.bin" 1 open .\\r3 4095 0 seek 3 here @ 4 read . here @ c@ . here @ 1 + c@ .\\r3 close\\r',
      'mkdir new\\rcp hello.txt new/copy\\rcat new/copy\\r', '0 fsck\\r', 'cd /sd/1\\rls\\rcat note.txt\\rcat docs/list.txt\\r',
      '"data.bin" 1 open .\\r3 8999 0 seek 3 here @ 1 read . here @ c@ .\\r3 close\\r', 'cp note.txt n2\\rcat n2\\r1 fsck\\r'].join(W(1))],
    expect: ['hydrafs 0 1\n',
      '0:/> ls\nhello.txt 13\ngames/\nbig.bin 5000\nmany/\na 16384\nc 4096\ne 4096\ng 4096\n',
      'cat hello.txt\nhello hydra\n', 'cat games/star.frt\n: star 42 . ;\n',
      'ls many\n' + Array.from({ length: 10 }, (_, i) => 'f' + i + ' 1\n').join(''),
      '@ 1 + c@ .\n' + num(4) + num(0xFF) + num(0) + '\n',            // big.bin, over a cluster boundary
      'cat new/copy\nhello hydra\n',
      '0 fsck\nsdhc 64 MB 131072 blocks\nhydrafs label=TESTS\nfree 65424 KB of 65532 KB\ncheck: lost 0, unmarked 0, twice 0\n',
      '1:/> ls\nnote.txt 14\ndocs/\ndata.bin 9000\n', 'cat note.txt\na quick card\n', 'cat docs/list.txt\none\ntwo\nthree\n',
      '8999 0 seek 3 here @ 1 read . here @ c@ .\n' + num(1) + num(0x11) + '\n',   // (8999 * 7) & $FF
      'cat n2\na quick card\n',
      '1 fsck\nsdhc 64 MB 131072 blocks\nhydrafs label=QUICK\nfree 65500 KB of 65532 KB\ncheck: lost 0, unmarked 0, twice 0\n'],
    forbid: ['!DS PTR ERROR!', '!IO ERR!', '!UNK WORD!'],
    check: (out, report, files) => {                            // The copies, as the PC tool sees them now
      for (const [dev, label, version, made] of [[0, 'TESTS', 1, 'new/copy'], [1, 'QUICK', 2, 'n2']]) {
        const v = new hydrafs.Volume(files.sds[dev]);
        try {
          const p = v.check();
          if (p.length) return 'card ' + dev + ': ' + p[0];
          if (v.label !== label || v.version !== version) return 'card ' + dev + ': ' + v.label + ', version ' + v.version;
          if (!v.tryWalk(made)) return 'card ' + dev + ': no ' + made;
        } finally { v.close(); }
      }
    },
  },
  {
    name: 'hydrafs-quick', about: 'HydraFS quick format (a 244 GB card in a moment), a volume\'s size, a full format\'s and fsck\'s progress; a free map written as it\'s used',
    sd: [{ dev: 0, claim: 500170752, fill: img => img.fill(0xA5) },   // A 244 GB card, with junk on it
      { dev: 1, mb: 64, label: 'LAZY', quick: true, hfs: v => {  // A quick-formatted card, junk in its free map,
        for (let j = 0; j < v.mapBlocks; j++) v.writeBlock(v.mapStart + j, Buffer.alloc(512, 0xFF));
        v.hint = 9000;                                            //   and the next cluster in its third map block
      } }],
    args: ['--cycles', '120000000', '--mark', 'QUICK', '--mark', 'of 250077740 KB', '--input', BOOT + [
      '0 "QUICK" mkfs\\r', '"/sd/0/a" 0 create .\\r3 here @ 5 write . 3 close\\rls /sd/0\\r', '0 "SMALL" 4096 mkfs-size\\r',
      '0 fsck\\r' + W(2), '"/dev/sd/0/ctl" "format -f -s 1G FULL" ctl\\r' + W(8) + 'ls /dev/sd/0/ctl\\r',
      '"/sd/1/f" 0 create .\\r3 here @ 5 write . 3 close\\r1 fsck\\r'].join(P)],
    expect: ['hydrafs 1\n',                                      // (Card 0: junk, no HydraFS)
      '"QUICK" mkfs\nsdhc 244224 MB 500170752 blocks\nhydrafs label=QUICK\nfree 250077740 KB of 250077740 KB\n',
      'ls /sd/0\na 5\n', 'mkfs-size\nsdhc 244224 MB 500170752 blocks\nhydrafs label=SMALL\nfree 4194172 KB of 4194172 KB\n',
      '0 fsck\n10% 20% 30% 40% 50% 60% 70% 80% 90% 100%\nsdhc', 'check: lost 0, unmarked 0, twice 0\n',   // 16 passes
      'ctl\n10% 20% 30% 40% 50% 60% 70% 80% 90% 100%\n',       // A full format's 64 map blocks
      'ls /dev/sd/0/ctl\nsdhc 244224 MB 500170752 blocks\nhydrafs label=FULL\nfree 1048540 KB of 1048540 KB\n',
      '1 fsck\nsdhc 64 MB 131072 blocks\nhydrafs label=LAZY\nfree 65524 KB of 65532 KB\ncheck: lost 0, unmarked 0, twice 0\n'],   // (f, and the root's first cluster)
    forbid: ['!DS PTR ERROR!', '!IO ERR!', '!UNK WORD!'],
    check: (out, report, files) => {
      const marks = [...report.matchAll(/mark: "([^"]*)" at cycle (\d+)/g)];
      const at = t => +(marks.find(m => m[1] === t) || [0, 0, NaN])[2];
      const took = at('of 250077740 KB') - at('QUICK');   // (From the command's echo)
      if (!(took < 2000000)) return 'the quick format of 244 GB took ' + took + ' cycles, not under 2M';
      const open = (dev, f) => { const v = new hydrafs.Volume(files.sds[dev]); try { return f(v); } finally { v.close(); } };
      return open(0, v => {
        const p = v.check();
        if (p.length) return 'card 0: ' + p[0];
        if (v.version !== 1 || v.label !== 'FULL' || v.mapInit !== 64) return 'card 0: version ' + v.version + ', ' + v.label + ', map ' + v.mapInit;
      }) || open(1, v => {                                       // Map blocks 0-2 written (the first two zeros),
        const p = v.check();                                      //   and 3 still the junk it had
        if (p.length) return 'card 1: ' + p[0];
        if (v.mapInit !== 3 || !v.used(9000)) return 'card 1: map ' + v.mapInit + ' blocks written, cluster 9000 ' + (v.used(9000) ? 'used' : 'free');
        if (v.readBlock(v.mapStart)[0] !== 0 || v.readBlock(v.mapStart + 3)[0] !== 0xFF) return 'card 1: the map blocks on the card are wrong';
      });
    },
  },
  {
    name: 'ram-disks', about: 'the RAM disks at boot: the RAM disk (r: the storage task\'s banks) and the shared one (s: shared banks), their ctl files (sizes, banks, HydraFS), /ram and /sram mounted (mount hfs /ram r/1, mount hfs /sram s), the caches\' directories; a file bigger than a bank through each, back to a card as it was',
    sd: [{ dev: 0, label: 'CARD', hfs: () => {} }],
    args: ['--cycles', '150000000', '--input', BOOT + ['cat /dev/sd/r/ctl\\r', 'cat /dev/sd/s/ctl\\r', 'ns\\r', 'mount hfs /a r\\r', 'ls /a\\r', 'ls /ram\\r', 'ls /sram\\r',
      'cp /rom/songs/test.zsm /sram/t.zsm\\r', 'cp /sram/t.zsm s.zsm\\r', 'cp /rom/songs/test.zsm /ram/t.zsm\\r', 'cp /ram/t.zsm r.zsm\\r', 'ls /sram\\r'].join(P) + P],
    expect: ['cat /dev/sd/r/ctl\nram 256 KB 512 blocks\nbanks $10-$2F\nhydrafs label=RAM\nfree 244 KB of 252 KB\n',
      'cat /dev/sd/s/ctl\nsram 512 KB 1024 blocks\nbanks $40-$7F\nhydrafs label=SRAM\nfree 504 KB of 508 KB\n',
      '> ns\nmount -s hfs /sd\nmount -s env /env\nmount -s proc /proc\nmount -s hfs /rom x\nmount -s hfs /sram s\nbind -cs /ram/bin /bin\nbind -as /sram/bin /bin\nbind -as /sd/0/bin /bin\nbind -as /rom/bin /bin\nbind -cs /ram/lib /lib\nbind -as /sram/lib /lib\nbind -as /sd/0/lib /lib\nbind -as /rom/lib /lib\nmount -s pc /pc\nmount hfs /ram r/1\n', '> ls /a\n1/\n', '> ls /ram\nbin/\nlib/\n', '> ls /sram\nbin/\nlib/\n',
      '> ls /sram\nbin/\nlib/\nt.zsm 14075\n'],
    forbid: ['!IO ERR!', '!UNK WORD!', 'No card:'],
    check: (out, report, files) => {
      const song = fs.readFileSync(path.join(__dirname, '../../os_rom/songs/test.zsm'));
      const v = new hydrafs.Volume(files.sd);
      try {
        for (const n of ['s.zsm', 'r.zsm']) {
          const e = v.walk(n);
          if (!e) return n + ' wasn\'t copied back to the card';
          if (!v.read(e).equals(song)) return n + ' (the song through the ' + (n[0] === 's' ? 'shared ' : '') + 'RAM disk) isn\'t the song';
        }
      } finally { v.close(); }
    },
  },
  {
    name: 'ram-ctl', about: 'the RAM disks\' ctl files: stop (not with a file open: busy), start with a size (banks, K, M) and where from (modules; shared bank IDs, hex), started already (busy), no room (disk full), a bad size or range or text after it, the ROM disk (not supported); a small machine (1 module, 1 shared macro-page) gets less at boot',
    args: ['--cycles', '200000000', '--input', BOOT + ['echo x > /sram/f\\r', 'q^/sram/f^ 1 open .\\r', 'q^/dev/sd/s/ctl^ q^stop^ ctl\\r', '3 close\\r',
      'q^/dev/sd/s/ctl^ q^stop^ ctl\\r', 'cat /dev/sd/s/ctl\\r', 'q^/dev/sd/s/ctl^ q^start 1M $10-$9F^ ctl\\r', 'cat /dev/sd/s/ctl\\r',
      'q^/dev/sd/r/ctl^ q^start 2^ ctl\\r', 'q^/dev/sd/r/ctl^ q^stop^ ctl\\r', 'q^/dev/sd/r/ctl^ q^start 255^ ctl\\r', 'q^/dev/sd/r/ctl^ q^start 3Q^ ctl\\r',
      'q^/dev/sd/r/ctl^ q^start 2 1-2 x^ ctl\\r', 'q^/dev/sd/r/ctl^ q^start 1 15-15^ ctl\\r', 'q^/dev/sd/x/ctl^ q^start 1^ ctl\\r',
      'q^/dev/sd/r/ctl^ q^start 16k 1-1^ ctl\\r', 'cat /dev/sd/r/ctl\\r', 'echo y > /ram/g\\r', 'cat /ram/g\\r'].join(P) + P],
    expect: ['open .\n' + num(3) + '\n', 'q^stop^ ctl\n\n !IO ERR! busy\n', '3 close\n', 'q^stop^ ctl\n\n/ram> cat /dev/sd/s/ctl\nnone\n',
      'cat /dev/sd/s/ctl\nsram 1024 KB 2048 blocks\nbanks $20-$9F\nhydrafs label=SRAM\n', 'q^start 2^ ctl\n\n !IO ERR! busy\n',
      'q^start 255^ ctl\n\n !IO ERR! disk full\n', 'q^start 3Q^ ctl\n\n !IO ERR! not supported\n', 'q^start 2 1-2 x^ ctl\n\n !IO ERR! not supported\n',
      'q^start 1 15-15^ ctl\n\n !IO ERR! not supported\n', 'q^start 1^ ctl\n\n !IO ERR! not supported\n', 'q^start 16k 1-1^ ctl\n\n/ram> cat /dev/sd/r/ctl\nram 16 KB 32 blocks\nbanks $1E-$1F\n',
      '> cat /ram/g\n\n !IO ERR! not found\n'],
    forbid: ['!UNK WORD!'],
  },
  {
    name: 'ram-speed', about: 'a program loads faster from the shared RAM disk than from a card: 16K (it only returns), run by its full path from the card, then copied to /sram/bin and run from there, at least 2.5 times as fast (3.5 now: every byte is copied from its bank to the block cache, to the transfer area, to the program, four bytes a turn of each loop)',
    sd: [{ dev: 0, label: 'SPEED', hfs: v => { v.put('big.hyx', hyx(0x0800, [0x60, ...new Array(16383).fill(0)])); } }],
    args: ['--cycles', '200000000', '--mark', '> /sd/0/big', '--mark', '> /sram/bin/fast', '--mark', '> ', '--input', BOOT +
      ['cp big.hyx /sram/bin/fast.hyx\\r', '/sd/0/big\\r', '/sram/bin/fast\\r'].join(P) + P],
    forbid: ['!IO ERR!', '!UNK WORD!'],
    check: (out, report) => {
      const prompts = [...report.matchAll(/mark: "> " at cycle (\d+)/g)].map(m => +m[1]);
      const took = name => {
        const m = new RegExp('mark: "> ' + name.replace(/\//g, '\\/') + '" at cycle (\\d+)').exec(report);
        if (!m) return null;
        const after = prompts.find(c => c > +m[1]);
        return after ? after - +m[1] : null;
      };
      const card = took('/sd/0/big'), ram = took('/sram/bin/fast');
      if (!card || !ram) return 'a load wasn\'t timed (card ' + card + ', RAM ' + ram + ')';
      if (ram * 2.5 > card) return 'from the RAM disk: ' + ram + ' cycles; from the card: ' + card + ' (not 2.5 times as fast)';
    },
  },
  {
    name: 'ram-small', about: 'the RAM disks on a small machine (1 RAM module, 1 shared macro-page): less than io.inc\'s sizes, half at a time, rather than none',
    args: ['--modules', '1', '--shared-u', '1', '--cycles', '60000000', '--input', BOOT + 'cat /dev/sd/r/ctl\\r' + P + 'cat /dev/sd/s/ctl\\r'],
    expect: ['ram 128 KB 256 blocks\nbanks $00-$0F\n', 'sram 64 KB 128 blocks\nbanks $01-$08\n'],
  },
];
