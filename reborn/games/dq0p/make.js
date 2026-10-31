// make.js - (dq0p) the senses' data, after its build (build.js calls build): its overlays' file packed (obj/games/
// dq0p.ovp, ../dq0/ovlpack.js's: ld65's cut after the last overlay that has code, each bank's fill left out), so the
// task reads no more than it needs as it starts.
'use strict';

function build({ objdir }) {
  return { overlays: require('../dq0/ovlpack.js').pack(objdir, 'dq0p').banks };
}

module.exports = { build };
