#!/usr/bin/env node
// ****************************************************************************
// sdwrite.js - an SD card image onto a card in the PC's reader: HydraOS's (bin/sdcard.img: the samples and songs), or
// any (sim/tools/hydrafs.js makes them; one read from a card, the run.js --sd file a test left).  It writes the image
// to the card's first blocks, raw, as a disk imager would, then reads them back and compares.  For the card's own
// files, without writing it whole, hydrafs.js puts and gets them in an image.
//
// Usage: node sim/tools/sdwrite.js --list
//        node sim/tools/sdwrite.js IMAGE DISK [--yes] [--force] [--no-verify] [--dry-run]
//   --list        the PC's disks: number or device, size, bus, model, and what it's mounted as; the removable ones
//                 marked (an SD card in a reader is one)
//   IMAGE         the image (bin/sdcard.img)
//   DISK          the card: on Windows its disk number (--list's 3, or \\.\PhysicalDrive3); on Linux its device
//                 (/dev/sdb, /dev/mmcblk0); on macOS its disk (/dev/disk4).  Or a file (another image), to try it
//   --yes         don't ask (it asks for the disk's number, or its name, typed again)
//   --force       a disk that isn't removable too (an external drive: never the system's)
//   --no-verify   don't read it back
//   --dry-run     the checks, and what it would do, but nothing written
// It refuses the disk the system is on, one smaller than the image, and (without --force) one that isn't removable.
// It needs the right to write a disk: on Windows an administrator's prompt (it takes the disk offline for the write,
// so Windows lets go of its volumes, and brings it back online after); on Linux root (sudo), the card's partitions
// unmounted first (umount); on macOS an administrator (sudo: it unmounts the disk itself, diskutil unmountDisk).
'use strict';
const fs = require('fs');
const path = require('path');
const readline = require('readline');
const { execFileSync } = require('child_process');

const CHUNK = 1 << 20;                                       // A write's bytes (whole blocks)
const win = process.platform === 'win32', mac = process.platform === 'darwin';
const mb = n => (n / 1048576).toFixed(n < 1048576 * 10 ? 1 : 0) + ' MB';
const gb = n => n >= 1e9 ? (n / 1e9).toFixed(1) + ' GB' : mb(n);

// ---- The disks: [{ id, dev, model, bus, size, removable, system, mounts }]

function disksWindows() {
  const ps = `$sys = $env:SystemDrive
$out = @(Get-CimInstance Win32_DiskDrive | ForEach-Object {
  $d = $_
  $letters = @($d | Get-CimAssociatedInstance -ResultClassName Win32_DiskPartition |
    Get-CimAssociatedInstance -ResultClassName Win32_LogicalDisk | ForEach-Object { $_.DeviceID })
  [pscustomobject]@{ index = $d.Index; model = $d.Model; bus = $d.InterfaceType; media = $d.MediaType;
    size = [int64]$d.Size; letters = $letters; system = ($letters -contains $sys) }
})
ConvertTo-Json -InputObject $out -Compress -Depth 3`;
  const text = execFileSync('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', ps], { encoding: 'utf8' });
  return JSON.parse(text || '[]').map(d => ({ id: String(d.index), dev: '\\\\.\\PhysicalDrive' + d.index, model: (d.model || '').trim(),
    bus: d.bus || '', size: +d.size || 0, removable: /removable/i.test(d.media || ''), system: !!d.system,
    mounts: d.letters || [] })).sort((a, b) => a.id - b.id);
}

function disksLinux() {
  const j = JSON.parse(execFileSync('lsblk', ['-J', '-b', '-o', 'NAME,PATH,SIZE,MODEL,TRAN,RM,TYPE,MOUNTPOINT'], { encoding: 'utf8' }));
  const mounts = d => [d.mountpoint, ...(d.children || []).flatMap(mounts)].filter(Boolean);
  return j.blockdevices.filter(d => d.type === 'disk').map(d => {
    const m = mounts(d);
    return { id: d.path, dev: d.path, model: (d.model || '').trim(), bus: d.tran || '', size: +d.size, removable: d.rm === true || d.rm === '1' || /usb|mmc/.test(d.tran || ''),
      system: m.some(x => x === '/' || x === '/boot' || x === '/boot/efi' || x === '[SWAP]'), mounts: m };
  });
}

function disksMac() {
  const text = execFileSync('diskutil', ['list'], { encoding: 'utf8' }), out = [];
  for (const block of text.split(/\n(?=\/dev\/)/)) {
    const h = block.match(/^(\/dev\/disk\d+) \(([^)]*)\):/);
    if (!h || !/physical/.test(h[2])) continue;
    const s = block.match(/0:\s+\S+.*?\*([\d.]+) (GB|MB|TB)/), unit = { MB: 1e6, GB: 1e9, TB: 1e12 };
    out.push({ id: h[1], dev: h[1].replace('/dev/disk', '/dev/rdisk'), model: '', bus: h[2], size: s ? Math.round(+s[1] * unit[s[2]]) : 0,
      removable: /external/.test(h[2]), system: /internal/.test(h[2]) && h[1] === '/dev/disk0', mounts: [] });
  }
  return out;
}

const disks = () => win ? disksWindows() : mac ? disksMac() : disksLinux();

function list() {
  const all = disks();
  console.log('disk'.padEnd(18) + 'size'.padStart(10) + '  bus       ' + 'model');
  for (const d of all)
    console.log((d.id + (d.removable ? ' *' : '')).padEnd(18) + gb(d.size).padStart(10) + '  ' + d.bus.padEnd(10) + d.model +
      (d.mounts.length ? '  (' + d.mounts.join(' ') + ')' : '') + (d.system ? '  [the system\'s]' : ''));
  console.log('* removable: a card in a reader is one.  Write one: node sim/tools/sdwrite.js IMAGE ' + (win ? 'NUMBER' : 'DEVICE'));
}

// ---- Writing

// The image, as it is: what kind (a partition table, a HydraFS), for the user to see
function describe(img) {
  const magic = img.toString('latin1', 0, 8);
  if (magic === 'HYDRAFS1') return 'a HydraFS (no partition table)';
  if (img[510] === 0x55 && img[511] === 0xAA) {
    const types = [0, 1, 2, 3].map(i => img[0x1BE + i * 16 + 4]).filter(t => t);
    return 'a partition table (' + types.map(t => t === 0x7F ? 'HydraFS' : t === 0x0C ? 'FAT32' : '$' + t.toString(16)).join(', ') + ')';
  }
  return 'no partition table or HydraFS that sdwrite knows';
}

function ask(q) {
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
  return new Promise(ok => rl.question(q, a => { rl.close(); ok(a.trim()); }));
}

function ps(cmd) {
  execFileSync('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', cmd], { stdio: ['ignore', 'pipe', 'pipe'] });
}

// The disk let go of, for the write (Windows: offline; macOS: unmounted), and taken back after
function release(d) {
  if (win) {
    try { ps('Set-Disk -Number ' + d.id + ' -IsOffline $true'); }
    catch (e) { throw new Error('the disk can\'t be taken offline for the write: run this in an administrator\'s prompt (' + String(e.stderr || e.message).trim().split('\n')[0] + ')'); }
  } else if (mac) execFileSync('diskutil', ['unmountDisk', d.id], { stdio: 'inherit' });
  else if (d.mounts.length) throw new Error(d.dev + ' is mounted (' + d.mounts.join(' ') + '): unmount it first (sudo umount ' + d.dev + '*)');
}
function restore(d) {
  if (win) { try { ps('Set-Disk -Number ' + d.id + ' -IsOffline $false'); } catch (e) { console.error('sdwrite: the disk is still offline: Set-Disk -Number ' + d.id + ' -IsOffline $false, or Disk Management'); } }
}

function writeAll(target, img, verify) {
  const fd = fs.openSync(target, 'r+');
  try {
    for (let at = 0; at < img.length; at += CHUNK) {
      const n = Math.min(CHUNK, img.length - at);
      fs.writeSync(fd, img, at, n, at);
      process.stdout.write('\rwritten ' + mb(at + n) + ' of ' + mb(img.length) + '   ');
    }
    try { fs.fsyncSync(fd); } catch (e) { /* (A raw disk on Windows: written through) */ }
    process.stdout.write('\n');
    if (!verify) return;
    const buf = Buffer.alloc(CHUNK);
    for (let at = 0; at < img.length; at += CHUNK) {
      const n = Math.min(CHUNK, img.length - at);
      fs.readSync(fd, buf, 0, n, at);
      if (!buf.subarray(0, n).equals(img.subarray(at, at + n))) throw new Error('read back, it differs from the image at block ' + at / 512 + ' on');
      process.stdout.write('\rverified ' + mb(at + n) + ' of ' + mb(img.length) + '   ');
    }
    process.stdout.write('\n');
  } finally { fs.closeSync(fd); }
}

async function main(argv) {
  const opt = { yes: false, force: false, verify: true, dry: false }, args = [];
  for (const a of argv) {
    if (a === '--list') return list();
    if (a === '--yes') opt.yes = true;
    else if (a === '--force') opt.force = true;
    else if (a === '--no-verify') opt.verify = false;
    else if (a === '--dry-run') opt.dry = true;
    else if (a.startsWith('--')) throw new Error(a + '?  (see the top of sim/tools/sdwrite.js)');
    else args.push(a);
  }
  if (args.length !== 2) throw new Error('usage: node sim/tools/sdwrite.js --list, or IMAGE DISK [--yes] [--force] [--no-verify] [--dry-run]');
  const img = fs.readFileSync(args[0]);
  if (!img.length || img.length % 512) throw new Error(args[0] + ': not a disk image (its length isn\'t whole 512-byte blocks)');
  console.log(args[0] + ': ' + mb(img.length) + ', ' + describe(img));

  // A file: another image, written as the card would be
  const target = args[1];
  const isDevice = win ? /^(\d+|\\\\\.\\PhysicalDrive\d+)$/i.test(target) : target.startsWith('/dev/');
  if (!isDevice) {
    if (opt.dry) { console.log('would write it to the file ' + target); return; }
    if (!fs.existsSync(target)) fs.writeFileSync(target, Buffer.alloc(0));
    writeAll(target, img, opt.verify);
    console.log('done: ' + target);
    return;
  }

  // A disk: the checks
  const id = win ? target.replace(/^\\\\\.\\PhysicalDrive/i, '') : target;
  const d = disks().find(x => x.id === id || x.dev === target);
  if (!d) throw new Error('no disk ' + target + ' (node sim/tools/sdwrite.js --list)');
  const what = d.id + ': ' + gb(d.size) + ', ' + (d.bus + ' ' + d.model).trim() + (d.mounts.length ? ' (' + d.mounts.join(' ') + ')' : '');
  if (d.system) throw new Error(what + ' is the disk the system is on: never');
  if (!d.removable && !opt.force) throw new Error(what + ' isn\'t removable (an SD card in a reader is): --force, if you mean it');
  if (d.size && d.size < img.length) throw new Error(what + ' is smaller than the image');
  console.log('the card: ' + what);
  console.log('everything on it will be gone: its first ' + mb(img.length) + ' the image\'s, the rest as it was (the image\'s partition table doesn\'t name it)');
  if (opt.dry) { console.log('would write ' + d.dev); return; }
  if (!opt.yes) {
    const a = await ask('type the disk\'s ' + (win ? 'number' : 'name') + ' (' + d.id + ') to write it: ');
    if (a !== d.id) { console.log('not written'); return; }
  }
  release(d);
  try { writeAll(d.dev, img, opt.verify); }
  finally { restore(d); }
  console.log('done: ' + d.id + (mac ? ' (diskutil eject ' + d.id + ' before you take it out)' : ' (eject it before you take it out)'));
}

if (require.main === module) main(process.argv.slice(2)).catch(e => { console.error('sdwrite: ' + e.message); process.exit(1); });
module.exports = { disks, describe };
