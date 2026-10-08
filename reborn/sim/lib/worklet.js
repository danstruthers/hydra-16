// worklet.js - the browser's side of the sound (audio.js's stream): an AudioWorklet's processor, as text for
// audioWorklet.addModule (a Blob's URL), that plays what it's sent (Int16Arrays' buffers: 48,000 stereo samples a
// second, left then right).  It keeps some 0.15 s in hand: it waits for that much before it starts (and after it runs
// dry), and drops what's past 0.45 s.  It posts how many samples it holds, now and then.  For view.js's page and
// web/page.js.  No Node.js.
'use strict';

const WORKLET = `
class HydraSound extends AudioWorkletProcessor {
  constructor() {
    super();
    this.size = 96000; this.L = new Float32Array(this.size); this.R = new Float32Array(this.size);
    this.rd = 0; this.wr = 0; this.n = 0; this.primed = false; this.target = 7200; this.ticks = 0;
    this.port.onmessage = e => {
      const s = new Int16Array(e.data), k = s.length >> 1;
      for (let i = 0; i < k; i++) { this.L[this.wr] = s[2 * i] / 32768; this.R[this.wr] = s[2 * i + 1] / 32768; this.wr = (this.wr + 1) % this.size; }
      this.n += k;
      if (this.n > 3 * this.target) { const d = this.n - this.target; this.rd = (this.rd + d) % this.size; this.n -= d; }
    };
  }
  process(inputs, outputs) {
    const l = outputs[0][0], r = outputs[0][1] || l;
    if (!this.primed && this.n >= this.target) this.primed = true;
    for (let i = 0; i < l.length; i++) {
      if (this.primed && this.n > 0) { l[i] = this.L[this.rd]; r[i] = this.R[this.rd]; this.rd = (this.rd + 1) % this.size; this.n--; }
      else { l[i] = 0; r[i] = 0; if (this.primed) this.primed = false; }
    }
    if (++this.ticks % 64 === 0) this.port.postMessage(this.n);
    return true;
  }
}
registerProcessor('hydra-sound', HydraSound);
`;

module.exports = { WORKLET };
