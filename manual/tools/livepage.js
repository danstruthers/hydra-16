#!/usr/bin/env node
// livepage.js - a picture of a live page (the web emulator, as a reader sees it): Edge or Chrome, headless, driven
// over its DevTools protocol (Node's own WebSocket): the page opened, a wait, keys typed, more waits, then a
// screenshot.
//
// Usage: node manual/tools/livepage.js URL OUT.png [--size WxH] [--wait SECONDS] [--type TEXT] [--wait S] ...
//        [--click SELECTOR] [--clip SELECTOR]
//   The steps run in order.  --type's text: \r is Return.  --clip: the shot is that element's box only.
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawn } = require('child_process');

const BROWSER = process.env.BROWSER || 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe';
const sleep = ms => new Promise(r => setTimeout(r, ms));

async function main() {
  const argv = process.argv.slice(2);
  const url = argv[0], out = argv[1];
  let size = [1280, 900];
  const steps = [];
  for (let i = 2; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--size') size = argv[++i].split('x').map(Number);
    else if (a === '--wait') steps.push({ wait: +argv[++i] });
    else if (a === '--type') steps.push({ type: argv[++i].replace(/\\r/g, '\r').replace(/\\e/g, '\x1b') });
    else if (a === '--click') steps.push({ click: argv[++i] });
    else if (a === '--clip') steps.push({ clip: argv[++i] });
    else if (a === '--eval') steps.push({ evaluate: argv[++i] });
  }
  const prof = fs.mkdtempSync(path.join(os.tmpdir(), 'hydra-live-'));
  const proc = spawn(BROWSER, ['--headless=new', '--disable-gpu', '--no-first-run', '--no-default-browser-check',
    '--remote-debugging-port=0', '--user-data-dir=' + prof, '--window-size=' + size.join(','), 'about:blank'], { stdio: 'ignore' });
  try {
    let port = null;
    for (let k = 0; k < 200 && !port; k++) {
      try { port = fs.readFileSync(path.join(prof, 'DevToolsActivePort'), 'utf8').split('\n')[0].trim(); } catch (e) {}
      if (!port) await sleep(100);
    }
    if (!port) throw new Error('the browser gave no debugging port');
    const targets = await (await fetch('http://127.0.0.1:' + port + '/json/list')).json();
    const page = targets.find(t => t.type === 'page');
    const ws = new WebSocket(page.webSocketDebuggerUrl);
    await new Promise((ok, fail) => { ws.onopen = ok; ws.onerror = fail; });
    let id = 0;
    const pending = new Map();
    ws.onmessage = ev => { const m = JSON.parse(ev.data); if (m.id && pending.has(m.id)) { const p = pending.get(m.id); pending.delete(m.id); m.error ? p.fail(new Error(JSON.stringify(m.error))) : p.ok(m.result); } };
    const send = (method, params = {}) => new Promise((ok, fail) => { const n = ++id; pending.set(n, { ok, fail }); ws.send(JSON.stringify({ id: n, method, params })); });
    await send('Page.enable');
    await send('Emulation.setDeviceMetricsOverride', { width: size[0], height: size[1], deviceScaleFactor: 1, mobile: false });
    await send('Page.navigate', { url });
    await sleep(1500);
    let clip = null;
    for (const s of steps) {
      if (s.wait) await sleep(s.wait * 1000);
      if (s.type) for (const ch of s.type) {
        if (ch === '\r') { await send('Input.dispatchKeyEvent', { type: 'keyDown', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13, text: '\r' }); await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13 }); }
        else {
          await send('Input.dispatchKeyEvent', { type: 'keyDown', key: ch, text: ch, unmodifiedText: ch });
          await send('Input.dispatchKeyEvent', { type: 'keyUp', key: ch });
        }
        await sleep(60);
      }
      if (s.click) await send('Runtime.evaluate', { expression: `document.querySelector(${JSON.stringify(s.click)}).click()` });
      if (s.evaluate) await send('Runtime.evaluate', { expression: s.evaluate });
      if (s.clip) {
        const r = await send('Runtime.evaluate', { expression: `JSON.stringify(document.querySelector(${JSON.stringify(s.clip)}).getBoundingClientRect())`, returnByValue: true });
        const b = JSON.parse(r.result.value); clip = { x: b.x, y: b.y, width: b.width, height: b.height, scale: 1 };
      }
    }
    const shot = await send('Page.captureScreenshot', Object.assign({ format: 'png' }, clip ? { clip } : {}));
    fs.writeFileSync(out, Buffer.from(shot.data, 'base64'));
    ws.close();
    console.log('wrote', out);
  } finally {
    proc.kill();
    await sleep(500);
    try { fs.rmSync(prof, { recursive: true, force: true }); } catch (e) {}
  }
}
main().catch(e => { console.error(e.message); process.exit(1); });
