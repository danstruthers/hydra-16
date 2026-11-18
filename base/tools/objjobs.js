// ****************************************************************************
// objjobs.js - tool runs side by side, for the build's cache (objcache.js's ahead: the runs it hasn't, run at once
// before the build asks for each in turn): a worker thread, one a build, the build waiting on it.  Each message
// ({ jobs: [{ exe, args, cwd }], n: how many at a time }) its runs, then on workerData's port a list, a { code, said }
// each (the run's status, and what it wrote: a failure's words, which the build's own run says again), and its flag
// (an Int32Array on shared memory) set to 1 and notified: the build waits on it (Atomics.wait), its event loop held.
'use strict';
const { parentPort, workerData } = require('worker_threads');
const { spawn } = require('child_process');
const { flag, port } = workerData;

parentPort.on('message', ({ jobs, n }) => {
  const out = new Array(jobs.length);
  let next = 0, running = 0, sent = false;
  const done = () => {
    if (sent) return;
    sent = true;
    port.postMessage(out);
    Atomics.store(flag, 0, 1);
    Atomics.notify(flag, 0);
  };
  const start = () => {
    while (running < n && next < jobs.length) {
      const i = next++, j = jobs[i];
      let said = '', ended = false;
      const end = code => {
        if (ended) return;
        ended = true;
        out[i] = { code, said };
        running--;
        if (next >= jobs.length && running === 0) done(); else start();
      };
      running++;
      try {
        const c = spawn(j.exe, j.args, { cwd: j.cwd, stdio: ['ignore', 'pipe', 'pipe'] });
        c.stdout.on('data', d => { said += d; });
        c.stderr.on('data', d => { said += d; });
        c.on('error', e => { said += e.message; end(-1); });
        c.on('close', code => end(code === null ? -1 : code));
      } catch (e) { said += e.message; end(-1); }
    }
  };
  try { if (!jobs.length) done(); else start(); } catch (e) { done(); }
});
