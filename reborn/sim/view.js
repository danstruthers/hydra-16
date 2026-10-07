// ****************************************************************************
// view.js - the Vera X's screen, live, in a browser: run.js -i --view PORT serves a page at http://localhost:PORT
// that shows the VERA's frames as the Hydra runs (about 30 a second), the terminal staying the serial console.
// The VERA draws each line as its time comes while the view is on (vera.js's live: raster effects show), and the
// page fetches the last whole frame: /frame is the palette (256 x RGB) then the 640 x 480 palette indexes, /state a
// line of JSON (the frames so far, DC_VIDEO, the gateware's version).  Only on 127.0.0.1.
'use strict';
const http = require('http');

const PAGE = `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Hydra-16 screen</title>
<style>
  :root { --bg: #1b1b1f; --fg: #d8d8dc; --dim: #8a8a93; }
  body { margin: 0; background: var(--bg); color: var(--fg); font: 13px/1.4 system-ui, sans-serif;
    display: flex; flex-direction: column; align-items: center; gap: 8px; padding: 16px; }
  canvas { width: min(960px, 100%); aspect-ratio: 4 / 3; image-rendering: pixelated; background: #000; }
  #state { color: var(--dim); font-variant-numeric: tabular-nums; }
</style></head>
<body>
<canvas id="screen" width="640" height="480"></canvas>
<div id="state">waiting for the Hydra</div>
<script>
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
  frame();
</script></body></html>`;

// Serve machine m's screen on port; OUT: the server
function startView(m, port) {
  const vera = m.vera;
  vera.live = true;
  const server = http.createServer((req, res) => {
    if (req.url === '/frame') {
      const f = vera.lastFrame || vera.frame(), body = Buffer.alloc(768 + f.pixels.length);
      for (let i = 0; i < 256; i++) { body[3 * i] = f.rgb[i] >> 16; body[3 * i + 1] = (f.rgb[i] >> 8) & 0xFF; body[3 * i + 2] = f.rgb[i] & 0xFF; }
      body.set(f.pixels, 768);
      res.writeHead(200, { 'Content-Type': 'application/octet-stream', 'Cache-Control': 'no-store' });
      res.end(body);
    } else if (req.url === '/state') {
      res.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      res.end(JSON.stringify({ ready: vera.ready, frames: vera.frames, dcVideo: vera.dcVideo, version: vera.version ? vera.version.join('.') : '0.9' }));
    } else {
      res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
      res.end(PAGE);
    }
  });
  server.listen(port, '127.0.0.1');
  return server;
}

module.exports = { startView };
