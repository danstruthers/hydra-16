// make.js - (dq0p) the senses' data, after its build (build.js calls build): its overlays' file packed
// (obj/games/dq0p.ovp, ../dq0/ovlpack.js's: ld65's cut after the last overlay that has code, each bank's fill left
// out), so the task reads no more than it needs as it starts (../../tools/dq0build.js's packOnly).
'use strict';

module.exports = require('../../tools/dq0build.js').packOnly('dq0p');
