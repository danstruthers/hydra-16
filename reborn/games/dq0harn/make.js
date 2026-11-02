// make.js - (dq0harn) the referee's data files, after its build (build.js calls build): its overlays' file packed
// (obj/games/dq0harn.ovp, ../dq0/ovlpack.js's: ld65's cut after the last overlay that has code, each bank's fill left
// out), so the referee reads no more than it needs as it starts.
'use strict';

function build({ objdir, say = () => {} }) {
  const n = require('../dq0/ovlpack.js').pack(objdir, 'dq0harn').banks;
  return { overlays: n };
}

module.exports = { build };
