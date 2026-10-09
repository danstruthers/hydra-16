#!/usr/bin/env node
// ****************************************************************************
// web.js - the emulator in a browser: one HTML file, the whole of it (the page, web/page.html and web/page.js; the
// machine, sim/lib, in a Web Worker, web/worker.js; the ROM images, bin/bios.bin and bin/prom*.bin, gzipped), that
// runs HydraOS with nothing installed and nothing sent anywhere; and bin/sdcard.img, HydraOS's SD card (the samples, the
// songs), SD card 0 as it starts.  Open the file in a browser (Chrome, Edge, Firefox,
// Safari), or serve it.  The serial console is a terminal in the page; the Vera X's screen, keyboard and mouse, the
// sound, the clock chip, the RAM modules and SD cards (kept in the browser) are the page's Setup.
//
// Usage: node sim/web.js [--out FILE] [--serve [PORT]] [--check]
//   --out FILE      the page (default obj/web/hydra-16.html)
//   --serve [PORT]  then serve it at http://localhost:PORT (8017), till Ctrl-C
//   --check         then run the page's worker (the bundle, as the page starts it) in Node: boot HydraOS in it, type at
//                   its console, and see the answer; and hold web/mkfs.js's new card against sim/tools/hydrafs.js's
// Build first (node build.js): the images are bin/'s.
//   The scripts are bundled as CommonJS modules (each require('./x.js') a path relative to its file, named by its
// path from the repository's root): the board's emulator, base/sim/lib/machine.js and what it needs, and
// base/sim/lib's vt.js, keynum.js and worklet.js, none of which uses Node.js.
'use strict';
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const { execSync } = require('child_process');

const SIM = __dirname, ROOT = path.join(SIM, '..'), REPO = path.join(ROOT, '..');

// The modules entry needs (it and its requires', and theirs ...), as one script that runs entry: each module a
// function of (require, module, exports), by its path under sim/
function bundle(entry) {
  const mods = new Map();
  const add = rel => {
    if (mods.has(rel)) return;
    const src = fs.readFileSync(path.join(REPO, rel), 'utf8').replace(/\r\n/g, '\n');
    mods.set(rel, src);
    for (const m of src.matchAll(/require\('(\.{1,2}\/[^']+)'\)/g)) add(path.posix.normalize(path.posix.join(path.posix.dirname(rel), m[1])));
  };
  add(entry);
  let out = '(function () {\n"use strict";\nconst defs = {};\n';
  for (const [rel, src] of mods) out += 'defs[' + JSON.stringify(rel) + '] = function (require, module, exports) {\n' + src + '\n};\n';
  out += `const cache = {};
function resolve(from, p) {
  const parts = from.split('/'); parts.pop();
  for (const s of p.split('/')) { if (s === '..') parts.pop(); else if (s !== '.') parts.push(s); }
  return parts.join('/');
}
function load(name) {
  if (cache[name]) return cache[name].exports;
  if (!defs[name]) throw new Error('no module ' + name);
  const module = { exports: {} };
  cache[name] = module;
  defs[name](p => load(resolve(name, p)), module, module.exports);
  return module.exports;
}
load(${JSON.stringify(entry)});
})();
`;
  return { code: out, mods: [...mods.keys()] };
}

// The ROM images: the BIOS ROM's, then the paged ROM's chips' (as run.js boots them)
function images() {
  const bios = fs.readFileSync(path.join(ROOT, 'bin', 'bios.bin')), proms = [];
  for (let k = 0; fs.existsSync(path.join(ROOT, 'bin', 'prom' + k + '.bin')); k++) proms.push(fs.readFileSync(path.join(ROOT, 'bin', 'prom' + k + '.bin')));
  if (!proms.length) throw new Error('no bin/prom0.bin: node build.js');
  const card = path.join(ROOT, 'bin', 'sdcard.img');            // (HydraOS's SD card: the samples, the songs, the benchmarks)
  return { bios, prom: Buffer.concat(proms), card: fs.existsSync(card) ? fs.readFileSync(card) : null };
}

// What the page says it was built from: the commit, and whether the tree had changes
function buildInfo() {
  try {
    const opt = { cwd: ROOT, stdio: ['ignore', 'pipe', 'ignore'] };
    const hash = execSync('git rev-parse --short HEAD', opt).toString().trim();
    const dirty = execSync('git status --porcelain -- bin', opt).toString().trim() ? ' (with changed images)' : '';
    const date = execSync('git log -1 --format=%cd --date=short', opt).toString().trim();
    return 'HydraOS images from commit ' + hash + dirty + ', ' + date + '.';
  } catch (e) { return ''; }
}

// A script's text, safe inside a <script> element
const inScript = s => s.replace(/<\/(script)/gi, '<\\/$1').replace(/<!--/g, '<\\!--');
const attr = s => s.replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;');

function build(out) {
  const img = images(), worker = bundle('reborn/sim/web/worker.js'), page = bundle('reborn/sim/web/page.js');
  const roms = zlib.gzipSync(Buffer.concat([img.bios, img.prom]), { level: 9 }).toString('base64');
  let html = fs.readFileSync(path.join(SIM, 'web', 'page.html'), 'utf8').replace(/\r\n/g, '\n');
  const put = (mark, text) => { if (!html.includes(mark)) throw new Error('page.html: no ' + mark); html = html.replace(mark, () => text); };
  put('<!--WORKER-->', '<script id="worker-src" type="text/plain">\n' + inScript(worker.code) + '</script>');
  put('<!--ROMS-->', '<script id="roms" type="text/plain" data-bios="' + img.bios.length + '" data-build="' + attr(buildInfo()) + '">\n' + roms + '\n</script>' +
    (img.card ? '\n<script id="sdcard" type="text/plain">\n' + zlib.gzipSync(img.card, { level: 9 }).toString('base64') + '\n</script>' : ''));
  put('<!--PAGE-->', '<script>\n' + inScript(page.code) + '</script>');
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, html);
  return { size: html.length, worker, page };
}

// The worker's bundle in Node, as a page would run it: boot, then a line typed and its answer
function check(out) {
  const vm = require('vm');
  const html = fs.readFileSync(out, 'utf8');
  const code = html.match(/<script id="worker-src" type="text\/plain">\n([\s\S]*?)<\/script>/)[1].replace(/<\\\/(script)/gi, '</$1').replace(/<\\!--/g, '<!--');
  const b64 = html.match(/<script id="roms"[^>]*data-bios="(\d+)"[^>]*>\n([^<]*)\n<\/script>/);
  const all = zlib.gunzipSync(Buffer.from(b64[2], 'base64')), n = +b64[1];
  let text = '', fail = '';
  const self = { postMessage: msg => { if (msg.type === 'out') text += msg.s; if (msg.type === 'halted') fail = 'halted: ' + msg.why; } };
  const ctx = vm.createContext({ self, performance, setTimeout, clearTimeout, console, Math, Date });
  vm.runInContext(code, ctx);
  const ab = b => b.buffer.slice(b.byteOffset, b.byteOffset + b.length);
  self.onmessage({ data: { type: 'boot', bios: ab(all.subarray(0, n)), prom: ab(all.subarray(n)), opt: { vera: true, smc: true, rtc: true, modules: 3 }, cards: {} } });
  self.onmessage({ data: { type: 'speed', x: 0 } });
  const t0 = Date.now();
  return new Promise(done => {
    let typed = false;
    const poll = () => {
      if (!typed && /> ?$|% ?$/.test(text.replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '').trimEnd() + ' ')) { typed = true; self.onmessage({ data: { type: 'keys', s: '1 2 + .\r' } }); }
      const ok = typed && /\b3 +ok|\b3\s/.test(text.split('1 2 + .').slice(1).join(''));
      if (ok || fail || Date.now() - t0 > 60000) {
        self.onmessage({ data: { type: 'pause', on: true } });
        done({ ok: ok && !fail, text, fail });
      } else setTimeout(poll, 100);
    };
    poll();
  });
}

// web/mkfs.js's card against hydrafs.js's, the same stamp
function checkMkfs() {
  const hfs = require('./tools/hydrafs.js'), { mkfs } = require('./web/mkfs.js'), os = require('os');
  const f = path.join(os.tmpdir(), 'hydra-web-mkfs-' + process.pid + '.img');
  hfs.setNow(123456789);
  hfs.mkfs(f, 4, 'LABEL');
  const a = fs.readFileSync(f), b = Buffer.from(mkfs(4, 'LABEL', 123456789));
  fs.unlinkSync(f);
  return a.equals(b);
}

function serve(out, port) {
  const http = require('http');
  http.createServer((req, res) => {
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8', 'Cache-Control': 'no-store' });
    res.end(fs.readFileSync(out));
  }).listen(port, '127.0.0.1');
  console.log('the emulator: http://localhost:' + port + '  (Ctrl-C stops serving it)');
}

async function main(argv) {
  let out = path.join(ROOT, 'obj', 'web', 'hydra-16.html'), port = 0, doCheck = false;
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--out') out = path.resolve(argv[++i]);
    else if (a === '--serve') port = /^\d+$/.test(argv[i + 1] || '') ? +argv[++i] : 8017;
    else if (a === '--check') doCheck = true;
    else { console.error('web.js: ' + a + '?  (see the top of sim/web.js)'); process.exit(2); }
  }
  const r = build(out);
  console.log(path.relative(process.cwd(), out) + ': ' + (r.size / 1048576).toFixed(1) + ' MB (the worker: ' + r.worker.mods.join(', ') + '; the page: ' + r.page.mods.join(', ') + ')');
  if (doCheck) {
    const mk = checkMkfs();
    console.log('web/mkfs.js: ' + (mk ? 'the same card as hydrafs.js makes' : 'NOT the card hydrafs.js makes'));
    const c = await check(out);
    console.log('the worker: ' + (c.ok ? 'booted HydraOS, and 1 2 + . gave 3' : 'FAILED' + (c.fail ? ' (' + c.fail + ')' : '') + '; its console:\n' + c.text));
    if (!mk || !c.ok) process.exit(1);
  }
  if (port) serve(out, port);
}

if (require.main === module) main(process.argv.slice(2));
module.exports = { build, bundle };
