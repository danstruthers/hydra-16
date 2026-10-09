// ****************************************************************************
// view.js - the Vera X's screen, live, in a browser, and the sound: run.js -i --view PORT (the screen), --sound PORT
// (the sound), or both, serves a page at http://localhost:PORT, the terminal staying the serial console.
//   The screen: the VERA's frames as the Hydra runs (about 30 a second).  The VERA draws each line as its time comes
// while the view is on (vera.js's live: raster effects show), and the page fetches the last whole frame: /frame is the
// palette (256 x RGB) then the 640 x 480 palette indexes, /state a line of JSON (the frames so far, DC_VIDEO, the
// gateware's version).
//   The keyboard and mouse (with --smc: what.input): the canvas, clicked, takes the keys and the mouse, for the
// input controller (smc.js): each key's code (KeyboardEvent.code) as the IBM key number the SMC gives, pressed and let
// go (a key held repeats, as a PS/2 keyboard's does), to /key; the mouse's moves (in the screen's 640 x 480), its
// buttons and wheel, gathered and sent to /mouse 60 times a second.  Escape is the keyboard's too: click outside the
// screen to give the keys back to the page.
//   The sound: /audio is a stream of the machine's (audio.js's: 48,000 stereo 16-bit samples a second, as they're
// made), from when it's asked for.  The page's Sound button plays it through an AudioWorklet that keeps some 0.15 s
// in hand: it waits for that much before it starts (and after it runs dry), and drops what's past 0.45 s (the
// emulator ahead of the browser's clock, or not run at --speed 1).  Only on 127.0.0.1.
'use strict';
const http = require('http');
const { WORKLET } = require('./lib/worklet.js');         // (The page's sound: the worklet)
const { KEYNUM } = require('./lib/keynum.js');           // (KeyboardEvent.code: the IBM PC/AT's key numbers)

// The page's keyboard and mouse, for the input controller
const INPUT = `
  const KEYNUM = ${JSON.stringify(KEYNUM)};
  cv.tabIndex = 0;
  let mdx = 0, mdy = 0, mb = 0, mw = 0, moved = false, lastX = null, lastY = null;
  const key = (e, down) => {
    const n = KEYNUM[e.code];
    if (n === undefined) return;
    e.preventDefault();
    fetch('/key?n=' + n + '&d=' + (down ? 1 : 0), { method: 'POST' }).catch(() => {});
  };
  cv.addEventListener('keydown', e => key(e, true));
  cv.addEventListener('keyup', e => key(e, false));
  cv.addEventListener('mousedown', e => { cv.focus(); mb = e.buttons & 7; moved = true; e.preventDefault(); });
  cv.addEventListener('mouseup', e => { mb = e.buttons & 7; moved = true; });
  cv.addEventListener('contextmenu', e => e.preventDefault());
  cv.addEventListener('mousemove', e => {
    const x = e.offsetX * 640 / cv.clientWidth, y = e.offsetY * 480 / cv.clientHeight;
    if (lastX !== null) { mdx += x - lastX; mdy += y - lastY; moved = true; }
    lastX = x; lastY = y; mb = e.buttons & 7;
  });
  cv.addEventListener('mouseleave', () => { lastX = lastY = null; });
  cv.addEventListener('wheel', e => { mw += Math.sign(e.deltaY); moved = true; e.preventDefault(); }, { passive: false });
  setInterval(() => {
    if (!moved) return;
    const dx = Math.trunc(mdx), dy = Math.trunc(mdy), w = Math.max(-8, Math.min(7, mw));
    mdx -= dx; mdy -= dy; mw = 0; moved = false;
    fetch('/mouse?dx=' + dx + '&dy=' + dy + '&b=' + mb + '&w=' + w, { method: 'POST' }).catch(() => {});
  }, 16);`;

const page = (screen, sound, input) => `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Hydra-16 ${screen ? 'screen' : 'sound'}</title>
<style>
  :root { --bg: #1b1b1f; --fg: #d8d8dc; --dim: #8a8a93; --edge: #3a3a42; --on: #2e6b3a; }
  body { margin: 0; background: var(--bg); color: var(--fg); font: 13px/1.4 system-ui, sans-serif;
    display: flex; flex-direction: column; align-items: center; gap: 8px; padding: 16px; }
  canvas { width: min(960px, 100%); aspect-ratio: 4 / 3; image-rendering: pixelated; background: #000; }
  .row { display: flex; gap: 12px; align-items: center; flex-wrap: wrap; justify-content: center; }
  #state, #sstate { color: var(--dim); font-variant-numeric: tabular-nums; }
  button { font: inherit; color: var(--fg); background: transparent; border: 1px solid var(--edge); border-radius: 4px;
    padding: 4px 12px; cursor: pointer; }
  button[aria-pressed="true"] { background: var(--on); border-color: var(--on); }
</style></head>
<body>
${screen ? '<canvas id="screen" width="640" height="480"></canvas>' : '<h1 style="font-size:15px;font-weight:600;margin:0">Hydra-16</h1>'}
<div class="row">
${screen ? '<div id="state">waiting for the Hydra</div>' : ''}
${screen && input ? '<div id="kstate">click the screen for its keyboard and mouse</div>' : ''}
${sound ? '<button id="sound" aria-pressed="false">Sound</button><div id="sstate">sound off</div>' : ''}
</div>
<script>
${screen ? `
  const cv = document.getElementById('screen'), cx = cv.getContext('2d'), img = cx.createImageData(640, 480), st = document.getElementById('state');
  async function frame() {
    try {
      const b = new Uint8Array(await (await fetch('/frame', { cache: 'no-store' })).arrayBuffer()), d = img.data;
      for (let i = 0, o = 0; i < 640 * 480; i++, o += 4) { const p = b[768 + i] * 3; d[o] = b[p]; d[o + 1] = b[p + 1]; d[o + 2] = b[p + 2]; d[o + 3] = 255; }
      cx.putImageData(img, 0, 0);
      const s = await (await fetch('/state', { cache: 'no-store' })).json();
      st.textContent = (s.ready ? 'VERA ' + s.version + ' \\u00b7 frame ' + s.frames + ' \\u00b7 DC_VIDEO $' + s.dcVideo.toString(16).toUpperCase().padStart(2, '0') : 'the VERA is configuring');
    } catch (e) { st.textContent = 'the emulator stopped'; return; }
    setTimeout(frame, 33);
  }
  frame();` : ''}
${screen && input ? INPUT : ''}
${sound ? `
  const WORKLET = ${JSON.stringify(WORKLET)};
  const btn = document.getElementById('sound'), sst = document.getElementById('sstate');
  let ctx = null, stop = null;
  async function soundOn() {
    ctx = new AudioContext({ sampleRate: 48000 });
    await ctx.audioWorklet.addModule(URL.createObjectURL(new Blob([WORKLET], { type: 'application/javascript' })));
    const node = new AudioWorkletNode(ctx, 'hydra-sound', { outputChannelCount: [2] });
    node.connect(ctx.destination);
    node.port.onmessage = e => { sst.textContent = 'sound on \\u00b7 ' + Math.round(e.data / 48) + ' ms in hand'; };
    stop = new AbortController();
    btn.setAttribute('aria-pressed', 'true'); sst.textContent = 'sound on';
    try {
      const rd = (await fetch('/audio', { cache: 'no-store', signal: stop.signal })).body.getReader();
      let carry = new Uint8Array(0);
      for (;;) {
        const { value, done } = await rd.read();
        if (done) break;
        const b = new Uint8Array(carry.length + value.length);    // (A sample's 4 bytes may come in two parts)
        b.set(carry); b.set(value, carry.length);
        const whole = b.length - (b.length & 3);
        carry = b.slice(whole);
        if (whole) { const part = b.slice(0, whole); node.port.postMessage(part.buffer, [part.buffer]); }
      }
      sst.textContent = 'the emulator stopped';
    } catch (e) { if (stop && !stop.signal.aborted) sst.textContent = 'the emulator stopped'; }
  }
  function soundOff() {
    if (stop) stop.abort();
    if (ctx) ctx.close();
    ctx = stop = null;
    btn.setAttribute('aria-pressed', 'false'); sst.textContent = 'sound off';
  }
  btn.addEventListener('click', () => { if (ctx) soundOff(); else soundOn().catch(e => { sst.textContent = 'no sound: ' + e.message; soundOff(); }); });` : ''}
</script></body></html>`;

// Serve machine m's screen (what.screen) and sound (what.sound: m.audio's) on port; OUT: the server
function startView(m, port, what = { screen: true }) {
  const vera = what.screen ? m.vera : null, audio = what.sound ? m.audio : null, smc = vera && what.input ? m.smc : null;
  if (vera) vera.live = true;
  const html = page(!!vera, !!audio, !!smc);
  const server = http.createServer((req, res) => {
    if (vera && req.url === '/frame') {
      const f = vera.lastFrame || vera.frame(), body = Buffer.alloc(768 + f.pixels.length);
      for (let i = 0; i < 256; i++) { body[3 * i] = f.rgb[i] >> 16; body[3 * i + 1] = (f.rgb[i] >> 8) & 0xFF; body[3 * i + 2] = f.rgb[i] & 0xFF; }
      body.set(f.pixels, 768);
      res.writeHead(200, { 'Content-Type': 'application/octet-stream', 'Cache-Control': 'no-store' });
      res.end(body);
    } else if (vera && req.url === '/state') {
      res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      res.end(JSON.stringify({ ready: vera.ready, frames: vera.frames, dcVideo: vera.dcVideo, version: vera.version ? vera.version.join('.') : '0.9' }));
    } else if (smc && req.method === 'POST' && (req.url.startsWith('/key?') || req.url.startsWith('/mouse?'))) {
      const q = new URL(req.url, 'http://localhost').searchParams, n = k => parseInt(q.get(k), 10) || 0;
      if (req.url.startsWith('/key?')) smc.key(n('n'), n('d') === 1);
      else smc.move(n('dx'), n('dy'), n('b'), n('w'));
      res.writeHead(204);
      res.end();
    } else if (audio && req.url === '/audio') {                 // (The sound from now on, as it's made)
      res.writeHead(200, { 'Content-Type': 'application/octet-stream', 'Cache-Control': 'no-store' });
      if (res.socket) res.socket.setNoDelay(true);
      const send = s => res.write(Buffer.from(s.buffer, s.byteOffset, s.byteLength));
      audio.on(send);
      req.on('close', () => audio.off(send));
    } else {
      res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
      res.end(html);
    }
  });
  server.listen(port, '127.0.0.1');
  return server;
}

module.exports = { startView };
