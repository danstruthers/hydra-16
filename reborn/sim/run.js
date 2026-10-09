#!/usr/bin/env node
// ****************************************************************************
// run.js - HydraOS in the emulator: the base's run.js (../../base/sim/run.js: the board, its options, the monitor;
// the top of it lists them all) with this folder's images (bin/: node build.js), its modules' labels (obj/), and /pc
// (--pc-dir: the PC tool's part, sim/lib/pchost.js).
//
// Usage: node sim/run.js [options]      (as the base's: -i, --vera, --sound, --pc-dir DIR, --trace-calls, --break ...)
// From Node: boot(opt) gives the machine, HydraOS's images booted; labels() the kernel's labels; state(m), report(m).
'use strict';
const path = require('path');
const run = require('../../base/sim/run.js');
const { createPcHost } = require('./lib/pchost.js');

const ROOT = path.join(__dirname, '..');

module.exports = Object.assign({}, run, { boot: (opt = {}) => run.boot(Object.assign({ root: ROOT }, opt)), ROOT });

if (require.main === module) run.main(process.argv.slice(2), { root: ROOT, createPcHost });
