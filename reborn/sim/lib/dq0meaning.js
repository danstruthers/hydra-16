// dq0meaning.js - what happened in a Dingus Quest Zero test, read from the hub (Phase 3A: tests cut to meaning, not
// words): the world's events and the denizens' (their rings, as each is written), the replies to the requests that
// change things (the things', the denizens' sway, the world's, the referee's slots), and at each line entered (a CR
// read from the serial port) the state the line finds: the character's place, facing and bearings, light, pace,
// position and summary, the denizens near it, the things in the item store, the names given, what it perceives (the
// senses' record, struct sight), the theme, its long task (the hub's pc_task), the change records in the cache's pages
// (world.h's: a square's, a portal's state, its harm, a trap's, a chunk stocked, a hole dug part way).  hub.h's
// structs are laid out by cc65 (hubown.js's evaluate: the build's own compiler, so the offsets are the game's), kept
// in obj/dq0meaning.json as the headers stand; their enums and constants read from the headers, so an event's numbers
// come back as names (E_PORTAL, PO_OPENED ...).
//
// createRecorder() gives { hook, attach(m), games, final() }: hook is the machine's opt.sharedWrite (sim/test.js
// chains it with the hub writers' check in every dq0 test), attach(m) once it's booted; games: each game played (a
// hub made: its magic written), { events, dens, replies, snaps, tasks (the character's long task's part done, as it
// changes: { kind, len, done }) }, each record with line, the number of lines entered before it.  final(): the state
// now (the last snapshot, at the test's end).  sim/test.js puts it on the machine as m.dq0; tests.js's dq0Turns reads
// it a line at a time.
'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const ROOT = path.join(__dirname, '..', '..');                // (reborn/)
const DQ0 = path.join(ROOT, 'games', 'dq0');
const HEADERS = ['hub.h', 'items.h', 'level.h', 'theme.h', 'rs.h', path.join('..', 'dq0w', 'world.h')];

// The layout's expressions (each a C expression cc65 makes a number of), by name
const F = (st, f) => 'offsetof (' + st + ', ' + f + ')';
const S = (st, f) => 'sizeof (((' + st + '*) 0)->' + f + ')';
const EXPRS = {
  magic: 'HUB_MAGIC', eqHead: F('struct hub', 'eq_head'), eq: F('struct hub', 'eq'), eqSize: 'EQ_SIZE', msg: 'sizeof (struct msg)',
  mKind: F('struct msg', 'kind'), mWho: F('struct msg', 'who'), mA: F('struct msg', 'a'), mB: F('struct msg', 'b'),
  mW1: F('struct msg', 'w1'), mW2: F('struct msg', 'w2'), mT: F('struct msg', 't'), mX: F('struct msg', 'x'),
  dqHead: F('struct hub', 'dq_head'), dqSize: 'DQ_SIZE', denBank: 'DEN_BANK',
  ireq: F('struct hub', 'ireq'), irKind: F('struct ireq', 'kind'), irItem: F('struct ireq', 'item'), irTo: F('struct ireq', 'to'),
  irResult: F('struct ireq', 'result'), irName: F('struct ireq', 'name'), irX: F('struct ireq', 'x'), irY: F('struct ireq', 'y'),
  irQty: F('struct ireq', 'qty'), irInto: F('struct ireq', 'into'),
  sway: F('struct hub', 'sway'), swKind: F('struct sway', 'kind'), swWord: F('struct sway', 'word'), swOrder: F('struct sway', 'order'),
  swResult: F('struct sway', 'result'), swGrade: F('struct sway', 'grade'), swWas: F('struct sway', 'was'), swNow: F('struct sway', 'now'),
  swName: F('struct sway', 'name'),
  wreq: F('struct hub', 'wreq'), wqKind: F('struct hub', 'wreq.kind'), wqRes: F('struct hub', 'wreq.res'),
  slot: F('struct hub', 'slot'), slotSize: 'sizeof (struct req)', nslots: 'NSLOTS', dslot: F('struct hub', 'dslot'),
  rqKind: F('struct req', 'kind'), rqActor: F('struct req', 'actor'), rqOther: F('struct req', 'other'), rqA: F('struct req', 'a'),
  rqB: F('struct req', 'b'), rqC: F('struct req', 'c'), rqW1: F('struct req', 'w1'), rqW2: F('struct req', 'w2'),
  rqGrade: F('struct req', 'grade'), rqResult: F('struct req', 'result'), rqSteps: F('struct req', 'steps'),
  now: F('struct hub', 'now'), quit: F('struct hub', 'quit'), seed: F('struct hub', 'seed'), pcstatus: F('struct hub', 'pcstatus'),
  pcSn: F('struct hub', 'pc_stratum'), pcX: F('struct hub', 'pc_x'), pcY: F('struct hub', 'pc_y'), pcFace: F('struct hub', 'pc_face'),
  pcLight: F('struct hub', 'pc_light'), pcLamp: F('struct hub', 'pc_lamp'), pcsum: F('struct hub', 'pcsum'),
  gait: F('struct hub', 'gait'), watch: F('struct hub', 'watch'), pcPos: F('struct hub', 'pc_pos'), pcGoing: F('struct hub', 'pc_going'),
  pcReady: F('struct hub', 'pc_ready'), hubNow: F('struct hub', 'now'),
  pcChart: F('struct hub', 'pc_chart'), pcMap: F('struct hub', 'pc_map'),
  pcStance: F('struct hub', 'pc_stance'), pcStrike: F('struct hub', 'pc_strike'), pcShand: F('struct hub', 'pc_shand'), pcSkew: F('struct hub', 'pc_skew'), pcLost: F('struct hub', 'pc_lost'), wayOpen: F('struct hub', 'way_open'), digTool: F('struct hub', 'dig_tool'),
  sCond: F('struct rs_summary', 'cond'), sHealth: F('struct rs_summary', 'health'), sLoad: F('struct rs_summary', 'load'),
  sSight: F('struct rs_summary', 'sight'), sStep: F('struct rs_summary', 'step'), sGstep: F('struct rs_summary', 'gstep'), ngaits: 'NGAITS',
  sLimbs: F('struct rs_summary', 'limbs'), sLimp: F('struct rs_summary', 'limp'), sNorth: F('struct rs_summary', 'northsense'),
  actor: F('struct hub', 'actor'), actorSize: 'sizeof (struct actor)', nactors: 'NACTORS',
  aState: F('struct actor', 'state'), aSn: F('struct actor', 'sn'), aFace: F('struct actor', 'face'), aPos: F('struct actor', 'pos'),
  aX: F('struct actor', 'x'), aY: F('struct actor', 'y'), aDoing: F('struct actor', 'doing'), aName: F('struct actor', 'name'),
  aAttitude: F('struct actor', 'attitude'), aFaction: F('struct actor', 'faction'), aKind: F('struct actor', 'kind'), aDid: F('struct actor', 'did'),
  themeBank: 'THEME_BANK', thName: F('struct theme', 'name'), thStratum: F('struct theme', 'stratum'), tstrata: 'TSTRATA',
  tSname: F('struct sight', 'sname'),
  namesAt: '(unsigned) NAMES_AT - 0x8000', tnames: 'TNAMES', tname: 'sizeof (struct tname)', nmWhat: F('struct tname', 'what'),
  nmSn: F('struct tname', 'sn'), nmX: F('struct tname', 'x'), nmY: F('struct tname', 'y'), nmKind: F('struct tname', 'kind'),
  nmName: F('struct tname', 'name'),
  catBank: 'KIND_BANK', kindSize: 'sizeof (struct kind)', nkinds: 'NKINDS', kFlags: F('struct kind', 'flags'), kWeight: F('struct kind', 'weight'),
  item0: 'ITEM_BANK (0)', itemSize: 'sizeof (struct item)', nitems: 'NITEMS', iKind: F('struct item', 'kind'), iFlags: F('struct item', 'flags'),
  iWhere: F('struct item', 'where'), iSn: F('struct item', 'sn'), iX: F('struct item', 'x'), iY: F('struct item', 'y'),
  iQty: F('struct item', 'qty'), iId: F('struct item', 'id'), iFuel: F('struct item', 'fuel'), iShut: F('struct item', 'shut'), iThick: F('struct item', 'thick'),
  iMat: F('struct item', 'mat'), iQuality: F('struct item', 'quality'), iOwner: F('struct item', 'owner'), iHide: F('struct item', 'hide'), pcFuel: F('struct hub', 'pc_fuel'), pcLeft: F('struct hub', 'pc_left'),
  ioName: 'IO_NAME', ioTheme: 'IO_THEME', ioBegin: 'IO_BEGIN', ioPuzzle: 'IO_PUZZLE', ioThrow: 'IO_THROW', ioFill: 'IO_FILL', ioShut: 'IO_SHUT', ioInside: 'IO_INSIDE', ioLook: 'IO_LOOK', ioSeek: 'IO_SEEK',
  ioHands: 'IO_HANDS', sightBank: 'SIGHT_BANK', tSeq: F('struct sight', 'seq'), tWhere: F('struct sight', 'where'), tDark: F('struct sight', 'dark'),
  tKind: F('struct sight', 'kind'), tSize: F('struct sight', 'size'), tRw: F('struct sight', 'rw'), tRh: F('struct sight', 'rh'), tRx: F('struct sight', 'rx'), tRy: F('struct sight', 'ry'),
  tLit: F('struct sight', 'lit'), tCeil: F('struct sight', 'ceil'), tFirst: F('struct sight', 'first'), tZ: F('struct sight', 'z'),
  tNdoors: F('struct sight', 'ndoors'), tDoor: F('struct sight', 'door'), tDoorSize: S('struct sight', 'door[0]'), seeDoors: 'SEE_DOORS',
  dRel: F('struct sight', 'door[0].rel'), dState: F('struct sight', 'door[0].state'), dX: F('struct sight', 'door[0].x'),
  dY: F('struct sight', 'door[0].y'), dKind: F('struct sight', 'door[0].kind'), dParts: F('struct sight', 'door[0].parts'),
  tNopen: F('struct sight', 'nopen'), tOpen: F('struct sight', 'open'), tOpenSize: S('struct sight', 'open[0]'), seeOpen: 'SEE_OPEN',
  oRel: F('struct sight', 'open[0].rel'), oKind: F('struct sight', 'open[0].kind'),
  tNways: F('struct sight', 'nways'),
  tNitems: F('struct sight', 'nitems'), tItem: F('struct sight', 'item'), tItemSize: S('struct sight', 'item[0]'), seeItems: 'SEE_ITEMS',
  sName: F('struct sight', 'item[0].name'), sQty: F('struct sight', 'item[0].qty'), sRel: F('struct sight', 'item[0].rel'),
  sHid: F('struct sight', 'item[0].hid'),
  tStratum: F('struct sight', 'stratum'), tX: F('struct sight', 'x'), tY: F('struct sight', 'y'),
  tNdens: F('struct sight', 'ndens'), tDen: F('struct sight', 'den'), tDenSize: S('struct sight', 'den[0]'), seeDens: 'SEE_DENS',
  nX: F('struct sight', 'den[0].x'), nY: F('struct sight', 'den[0].y'), nDoing: F('struct sight', 'den[0].doing'), nName: F('struct sight', 'den[0].name'),
  tNtraps: F('struct sight', 'ntraps'), tTrap: F('struct sight', 'trap'), tTrapSize: S('struct sight', 'trap[0]'), seeTraps: 'SEE_TRAPS',
  pRel: F('struct sight', 'trap[0].rel'), pKind: F('struct sight', 'trap[0].kind'), pTrigger: F('struct sight', 'trap[0].trigger'),
  pState: F('struct sight', 'trap[0].state'),
  tNglows: F('struct sight', 'nglows'), tGlow: F('struct sight', 'glow'), tGlowSize: S('struct sight', 'glow[0]'), seeGlows: 'SEE_GLOWS',
  gRel: F('struct sight', 'glow[0].rel'), gDist: F('struct sight', 'glow[0].dist'), gHow: F('struct sight', 'glow[0].how'),
  tDazzled: F('struct sight', 'dazzled'),
  wayAt: 'WAY_AT', wNb: F('struct wayseen', 'nbeyond'), wB: F('struct wayseen', 'beyond'), wBSize: 'sizeof (struct beyond)',
  seeBeyond: 'SEE_BEYOND', bWhat: F('struct beyond', 'what'), bNoun: F('struct beyond', 'noun'), bRel: F('struct beyond', 'rel'),
  bDist: F('struct beyond', 'dist'), bKind: F('struct beyond', 'kind'), bLit: F('struct beyond', 'lit'),
  wNitems: F('struct wayseen', 'nitems'), wItem: F('struct wayseen', 'item'), wNdens: F('struct wayseen', 'ndens'),
  wDen: F('struct wayseen', 'den'), wNglows: F('struct wayseen', 'nglows'), wGlow: F('struct wayseen', 'glow'),
  uiBank: 'UI_BANK', uMap: F('struct uirec', 'map'), mapH: 'MAP_H', mapW: 'MAP_W', uMapLay: F('struct uirec', 'maplay'),
  uTread: F('struct uirec', 'tread'), uTrtext: F('struct uirec', 'trtext'), trText: 'TR_TEXT', trHow: F('struct tread', 'how'),
  trResult: F('struct tread', 'result'), trGrade: F('struct tread', 'grade'), trFloor: F('struct tread', 'floor'), trDust: F('struct tread', 'dust'),
  trTrod: F('struct tread', 'trod'), trSmoke: F('struct tread', 'smoke'), trNtr: F('struct tread', 'ntr'), trTr: F('struct tread', 'tr'),
  trailSize: 'sizeof (struct trail)', tlName: F('struct trail', 'name'), tlWhat: F('struct trail', 'what'), tlN: F('struct trail', 'n'),
  tlAgo: F('struct trail', 'ago'), tlHead: F('struct trail', 'head'), tlKind: F('struct trail', 'kind'), tlAge: F('struct trail', 'age'), tlStrength: F('struct trail', 'strength'),
  tjAt: '(unsigned) TJ_AT - 0x8000', tjN: F('struct tjournal', 'n'), tjPn: F('struct tjournal', 'pn'),
  tjE: F('struct tjournal', 'e'), tjP: F('struct tjournal', 'p'), tsSize: 'sizeof (struct tstep)', njournal: 'NJOURNAL', ntrail: 'NTRAIL',
  tsKind: F('struct tstep', 'kind'), tsSn: F('struct tstep', 'sn'), tsHead: F('struct tstep', 'head'), tsStay: F('struct tstep', 'stay'),
  tsX: F('struct tstep', 'x'), tsY: F('struct tstep', 'y'), tsT: F('struct tstep', 't'),
  worldBank: 'WORLD_BANK', wCpage: F('struct wrec', 'cpage'), cpSize: 'sizeof (struct cpage)', ncpages: 'NCPAGES',
  cpUsed: F('struct cpage', 'used'), cpN: F('struct cpage', 'n'), chBank0: 'CHANGE_BANK (0)', chPage: 'CH_PAGE',
  chSize: 'sizeof (struct change)', chSn: F('struct change', 'sn'), chX: F('struct change', 'x'), chY: F('struct change', 'y'),
  chTile: F('struct change', 'tile'), chElev: F('struct change', 'elev'), chLink: F('struct change', 'link'),
  dsenseAt: F('struct hub', 'dsense'), dsenseSize: S('struct hub', 'dsense'),
  pcTask: F('struct hub', 'pc_task'), ltKind: F('struct ltask', 'kind'), ltLen: F('struct ltask', 'len'), ltDone: F('struct ltask', 'done'),
  ltFoe: F('struct ltask', 'foe'),
  keptAt: '(unsigned) KEPT_AT - 0x8000', kpN: F('struct kept', 'n'), kpGen: F('struct kept', 'gen'), kpAt: F('struct kept', 'at'),
  keepSize: 'sizeof (struct keep)', kpSn: F('struct keep', 'sn'), kpRange: F('struct keep', 'range'), kpLive: F('struct keep', 'live'),
  kpCx: F('struct keep', 'cx'), kpCy: F('struct keep', 'cy'), kpLid: F('struct keep', 'lid'),
  wStrata: F('struct wrec', 'strata'), stSize: 'sizeof (struct stratum)', maxstrata: 'MAXSTRATA', stMade: F('struct stratum', 'made'),
  stN: F('struct stratum', 'n'), stX0: F('struct stratum', 'x0'), stY0: F('struct stratum', 'y0'), stW: F('struct stratum', 'w'),
  stH: F('struct stratum', 'h'),
  // (The chunks' cache, the character's memory of each: slice 4.2's travel's, dq0travel's)
  cacheBank0: 'CACHE_BANK (0)', ncslots: 'NCSLOTS', chunkSize: 'CHUNK_SIZE', wCslot: F('struct wrec', 'cslot'), csSize: 'sizeof (struct cslot)',
  csUsed: F('struct cslot', 'used'), csSn: F('struct cslot', 'sn'), csCx: F('struct cslot', 'cx'), csCy: F('struct cslot', 'cy'),
  ckNrooms: F('struct chunk', 'nrooms'), ckRoom: F('struct chunk', 'room'), rmSize: 'sizeof (struct room)', rmX: F('struct room', 'x'),
  rmY: F('struct room', 'y'), rmW: F('struct room', 'w'), rmH: F('struct room', 'h'), rmFlags: F('struct room', 'flags'), rmKind: F('struct room', 'kind'),
  ckKept: F('struct chunk', 'kept'), kQ: F('struct ckept', 'q'), kM: F('struct ckept', 'm'), kN: F('struct ckept', 'n'), kMap: F('struct ckept', 'map'),
  kDq: F('struct ckept', 'dq'), kDsk: F('struct ckept', 'dsk'), kSk: F('struct ckept', 'sk'), sMapfade: F('struct rs_summary', 'mapfade'),
  sMapmind: F('struct rs_summary', 'mapmind'),
};

// The headers' enums and #defines with a number: { names: { NAME: value }, blocks: [[NAME ...] an enum's members] }
function constants() {
  const names = {}, blocks = [];
  for (const h of HEADERS) {
    const text = fs.readFileSync(path.join(DQ0, h), 'latin1').replace(/\/\*[^]*?\*\//g, ' ');
    for (const k of text.matchAll(/^#define\s+(\w+)\s+\(?(0x[0-9A-Fa-f]+|\d+)\)?\s*$/gm)) names[k[1]] = +k[2];
    for (const k of text.matchAll(/\benum\s*\{([^}]*)\}/g)) {
      const block = [];
      let v = 0;
      for (const part of k[1].split(',')) {
        const p = part.trim().match(/^(\w+)(?:\s*=\s*(.+))?$/);
        if (!p) continue;
        if (p[2] !== undefined) { const e = p[2].trim(); v = /^(0x[0-9A-Fa-f]+|\d+)$/.test(e) ? +e : names[e] !== undefined ? names[e] : NaN; }
        names[p[1]] = v;
        block.push(p[1]);
        v++;
      }
      blocks.push(block);
    }
  }
  return { names, blocks };
}

// The layout and the constants (obj/dq0meaning.json, made again as the headers or this file change)
let LAYOUT = null;
function layout() {
  if (LAYOUT) return LAYOUT;
  const hash = crypto.createHash('sha1');
  for (const h of HEADERS) hash.update(fs.readFileSync(path.join(DQ0, h)));
  hash.update(fs.readFileSync(__filename));
  const key = hash.digest('hex'), cache = path.join(ROOT, 'obj', 'dq0meaning.json');
  let j = null;
  try { j = JSON.parse(fs.readFileSync(cache, 'utf8')); if (j.key !== key) j = null; } catch (e) { j = null; }
  if (!j) {
    const ks = Object.keys(EXPRS), v = require('./hubown.js').evaluate(ks.map(k => EXPRS[k]));
    j = { key, L: {}, C: constants() };
    ks.forEach((k, i) => { j.L[k] = v[i]; });
    try { const t = cache + '.' + process.pid; fs.writeFileSync(t, JSON.stringify(j)); fs.renameSync(t, cache); } catch (e) { /* (Made again next time) */ }
  }
  const C = j.C, byName = C.names;
  // (A value's name in the enum a member of which is given, or the value: name('PO_NONE', 1) -> 'PO_OPENED')
  const blockOf = {};
  for (const b of C.blocks) for (const n of b) if (!blockOf[n]) blockOf[n] = b;
  const name = (member, v) => { const b = blockOf[member] || []; const n = b.find(x => byName[x] === v); return n === undefined ? v : n; };
  // (A things' request's kind: items.h's IO_*, the enum's or those #defined after it, IO_NAME ... IO_FILL)
  const D = j.L, ioDefs = { [D.ioName]: 'IO_NAME', [D.ioTheme]: 'IO_THEME', [D.ioBegin]: 'IO_BEGIN', [D.ioPuzzle]: 'IO_PUZZLE', [D.ioThrow]: 'IO_THROW', [D.ioFill]: 'IO_FILL',
    [D.ioShut]: 'IO_SHUT', [D.ioInside]: 'IO_INSIDE', [D.ioLook]: 'IO_LOOK', [D.ioSeek]: 'IO_SEEK', [D.ioHands]: 'IO_HANDS' };
  const ioName = v => { const n = name('IO_KIT', v); return typeof n === 'number' && ioDefs[v] ? ioDefs[v] : n; };
  return (LAYOUT = { L: j.L, K: byName, name, ioName });
}

// The recorder (a test's machine's, sim/test.js's instrument)
function createRecorder() {
  const { L, K, name, ioName } = layout();
  let m = null, key0 = -1, game = null, enters = 0;
  const games = [];
  const bank = n => m && key0 >= 0 ? m.sharedBanks()[key0 + n] : null;
  const w16 = (b, o) => b[o] | b[o + 1] << 8, s8 = v => v << 24 >> 24;
  const w32 = (b, o) => (b[o] | b[o + 1] << 8 | b[o + 2] << 16 | b[o + 3] << 24) >>> 0;
  const str = (b, o, n) => { let s = ''; for (let i = 0; i < n && b[o + i]; i++) s += String.fromCharCode(b[o + i]); return s; };
  // An event's record (a ring's entry: struct msg at o of bank b), its numbers named by its kind
  function decode(b, o, den) {
    const e = { line: enters, kind: name('E_NONE', b[o + L.mKind]), who: b[o + L.mWho], a: b[o + L.mA], b: b[o + L.mB],
      w1: w16(b, o + L.mW1), w2: w16(b, o + L.mW2), t: w32(b, o + L.mT), x: Array.from(b.subarray(o + L.mX, o + L.mX + 4)), den };
    const w = e.w1;
    switch (e.kind) {
      case 'E_ARRIVE': e.how = name('A_WALKED', w & 0xFF); e.first = !!(w & 0x100); e.link = w >> 12; break;
      case 'E_NOWAY': e.why = name('N_NO_EXIT', w & 0xFF); e.actor = w >> 8; break;
      case 'E_VIEW': e.why = name('V_TURNED', w); break;
      case 'E_DIG': e.how = name('D_BEGUN', w >> 8); e.feet = w & 0xFF; break;
      case 'E_CLIMB': e.how = name('K_BEGUN', w >> 8); e.feet = w & 0xFF; break;
      case 'E_LEDGE': e.rise = s8(w & 0xFF); break;
      case 'E_PORTAL': e.po = name('PO_NONE', w & 0xFF); e.pk = name('PK_DOOR', w >> 8 & 0x7F); if (w & 0x8000) e.shows = true; break;
      case 'E_HARM': e.hc = name('HC_FALL', w & 0xFF); e.band = w >> 8 & 7; e.out = !!(w & 0x4000); e.caught = !!(w & 0x8000);
        e.stopped = e.hc === 'HC_STRUCK' && e.caught; break;          // (E_CAUGHT a fall's, E_STOPPED a blow's)
      case 'E_TRAP': e.tk = name('TK_PIT', w & 15); e.tt = name('TT_PLATE', w >> 4 & 3); e.to = name('TO_SPRUNG', w >> 8);
        e.touch = !!(w & 0x80); break;                               // (Found by touch, in the dark: 2026-10-22)
      case 'E_TAKEN': e.ir = name('IR_OK', w & 0xFF); break;
      case 'E_TASK': e.te = name('TE_STOPPED', w & 0xFF); e.task = name('LT_NONE', w >> 8);
        if (e.te === 'TE_ENGAGED' || e.te === 'TE_FAR' || e.te === 'TE_AWAY' || e.te === 'TE_FIGHT') { e.by = (e.b & 0x3F) - 1; e.still = !!(e.b & 0x80); e.quiet = !!(e.b & 0x40); }
        else if (e.te === 'TE_ASK_HURT') e.health = name('H_FINE', e.b);   // (Fighting on's: the band it's come to;
        else if (e.te === 'TE_NO_FOE') e.shock = !!e.b;                    //   its end, in shock or none left)
        else if (e.te === 'TE_BREAK') { e.by = (e.b & 0x3F) - 1; e.how = ['back', 'flight', 'run'][w >> 8]; delete e.task; }   // (A disengage's)
        else if (e.te === 'TE_FREE') e.none = ['engaged', 'near', 'way'][e.b];   // (None: not engaged, nothing near to flee, no way back)
        else if (e.te === 'TE_LOW' || e.te === 'TE_OUT') { e.lamp = e.b & 0x7F; e.seen = !!(e.b & 0x80); }   // (A light's kind; one lying there seen)
        else if (e.te === 'TE_ROBBED') e.by = e.b - 1;                     // (The robber: its entry)
        else if (e.te === 'TE_BEGUN' || e.te === 'TE_ASK_SHORT') e.what = e.b;   // (A timed work's, a battering's: what it's done to)
        else e.named = name('LT_NONE', e.b);
        break;
      case 'E_BEARING': e.brg = name('BRG_NONE', w); break;
      case 'E_BEAT': e.beat = name('WB_QUIET', w & 0xFF); e.side = name('WS_AHEAD', w >> 8);
        if ((w & 0xFF) >= K.WB_FEEL) {                         // (The dark's, 2026-10-22: what's met, FA_*, a thing's
          e.fa = name('FA_NONE', w >> 8 & 7); e.thing = w >> 8; e.head = w >> 8 & 7; e.gait = w >> 12;   //   kind, a heading
        }                                                          //   and its gait faster than feeling its way)
        break;
      case 'E_DEN': e.de = name('DE_MOVED', e.a); e.sn = e.x[0]; e.seen = !!e.x[2]; break;
      case 'E_FIGHT': e.def = e.a; e.sn = e.x[0]; e.out = e.x[1] === 0xFF ? 'FO_GONE' : e.x[1] === 0xFE ? 'FO_BLED' : name('O_MISS', e.x[1]);
        e.ff = e.x[2]; break;
    }
    return e;
  }
  // The state as it is now (the hub's records): see the comment at the top
  function snap() {
    const h = bank(0);
    if (!h || h[0] !== (L.magic & 0xFF) || h[1] !== L.magic >> 8) return null;
    const s = { line: enters, game: games.length - 1, cyc: m.cpu.cyc, now: w32(h, L.now), quit: h[L.quit], wayOpen: h[L.wayOpen],
      status: str(h, L.pcstatus, 16), digTool: [0, 1, 2, 3].map(i => str(h, L.digTool + i * 8, 8)),
      pc: { sn: h[L.pcSn], x: w16(h, L.pcX), y: w16(h, L.pcY), face: h[L.pcFace], light: h[L.pcLight], lamp: str(h, L.pcLamp, 14), fuel: w16(h, L.pcFuel), left: str(h, L.pcLeft, 20),
        pos: h[L.pcPos], gait: h[L.gait], watch: h[L.watch], going: h[L.pcGoing], stance: name('D_NONE', h[L.pcStance]), strike: name('SK_BEST', h[L.pcStrike]), shand: h[L.pcShand],
        skew: h[L.pcSkew], lost: h[L.pcLost],
        task: { kind: name('LT_NONE', h[L.pcTask]), asking: h[L.pcTask + 1], len: w16(h, L.pcTask + 2), done: w16(h, L.pcTask + 4), foe: h[L.pcTask + L.ltFoe] - 1 },
        ready: w16(h, L.pcReady) + w16(h, L.pcReady + 2) * 65536, now: w16(h, L.hubNow) + w16(h, L.hubNow + 2) * 65536,
        chart: h[L.pcChart], map: w16(h, L.pcMap) },
      sum: { cond: w16(h, L.pcsum + L.sCond), health: h[L.pcsum + L.sHealth], load: h[L.pcsum + L.sLoad], sight: h[L.pcsum + L.sSight],
        step: h[L.pcsum + L.sStep], gstep: Array.from(h.subarray(L.pcsum + L.sGstep, L.pcsum + L.sGstep + L.ngaits)),
        limbs: h[L.pcsum + L.sLimbs], limp: h[L.pcsum + L.sLimp], northsense: h[L.pcsum + L.sNorth],
        mapfade: h[L.pcsum + L.sMapfade], mapmind: h[L.pcsum + L.sMapmind] },
      actors: [], items: [], names: [], sight: null, theme: '', map: [] };
    for (let k = 0; k < L.nactors; k++) {
      const o = L.actor + k * L.actorSize, a = { k, state: h[o + L.aState], sn: h[o + L.aSn], face: h[o + L.aFace], pos: h[o + L.aPos],
        x: w16(h, o + L.aX), y: w16(h, o + L.aY), doing: name('DO_ROAM', h[o + L.aDoing]), name: str(h, o + L.aName, 12),
        attitude: name('AT_HOSTILE', h[o + L.aAttitude]), faction: h[o + L.aFaction], kind: h[o + L.aKind], did: w16(h, o + L.aDid) };
      if (a.state || a.name) s.actors.push(a);
    }
    const cat = bank(L.catBank), kname = k => cat ? str(cat, k * L.kindSize, 14) : '';     // (CATALOGUE[kind])
    const per = Math.floor(0x2000 / L.itemSize);
    for (let i = 0; i < L.nitems; i++) {
      const b = bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
      if (!b || !b[o + L.iKind]) continue;
      s.items.push({ n: i, kind: b[o + L.iKind], name: kname(b[o + L.iKind]), weight: cat ? cat[b[o + L.iKind] * L.kindSize + L.kWeight] : 0,
        kflags: cat ? cat[b[o + L.iKind] * L.kindSize + L.kFlags] : 0, flags: b[o + L.iFlags], where: name('IW_SQUARE', b[o + L.iWhere]),
        sn: b[o + L.iSn], x: w16(b, o + L.iX), y: w16(b, o + L.iY), qty: b[o + L.iQty], id: w16(b, o + L.iId), fuel: w16(b, o + L.iFuel),
        thick: b[o + L.iThick], mat: b[o + L.iMat], quality: b[o + L.iQuality], shut: b[o + L.iShut],
        did: b[o + L.iWhere] === K.IW_DEN ? b[o + L.iOwner] | b[o + L.iHide] << 8 : 0 });
    }
    const th = bank(L.themeBank);
    if (th) {
      s.theme = str(th, L.thName, 32);
      s.strata = Array.from({ length: L.tstrata }, (_, i) => str(th, L.thStratum + i * 20, 20));   // (Its strata's names, the top down)
      for (let i = 0; i < L.tnames; i++) {
        const o = L.namesAt + i * L.tname;
        if (!th[o + L.nmWhat]) continue;
        s.names.push({ what: name('NM_NONE', th[o + L.nmWhat]), sn: th[o + L.nmSn], x: w16(th, o + L.nmX), y: w16(th, o + L.nmY),
          kind: str(th, o + L.nmKind, 14), name: str(th, o + L.nmName, 20) });
      }
    }
    s.task = { kind: name('LT_NONE', h[L.pcTask + L.ltKind]), len: w16(h, L.pcTask + L.ltLen), done: w16(h, L.pcTask + L.ltDone), foe: h[L.pcTask + L.ltFoe] - 1 };
    s.changes = [];                                              // (The change records in the cache's pages: world.h's,
    const wb = bank(L.worldBank), cper = Math.floor(0x2000 / L.chPage);  //   the world's, by page)
    if (wb) for (let p = 0; p < L.ncpages; p++) {
      const q = L.wCpage + p * L.cpSize, b = wb[q + L.cpUsed] ? bank(L.chBank0 + Math.floor(p / cper)) : null;
      for (let i = 0; b && i < wb[q + L.cpN]; i++) {
        const o = (p % cper) * L.chPage + i * L.chSize, link = b[o + L.chLink];
        s.changes.push({ sn: b[o + L.chSn], x: w16(b, o + L.chX), y: w16(b, o + L.chY), tile: b[o + L.chTile], elev: s8(b[o + L.chElev]), link,
          what: ['PORTAL_STATE', 'PORTAL_HARM', 'TRAP_STATE', 'STOCKED', 'DUG', 'ROOM_LIT'].find(n => K[n] === link) || (link === 0xFF ? 'SQUARE' : 'OPENING') });
      }
    }
    s.chunks = [];                                               // (The chunks in the cache, each with what the character
    if (wb) for (let k = 0; k < L.ncslots; k++) {               //   keeps of it, its rooms: slice 4.2's)
      const o = L.wCslot + k * L.csSize, cb = bank(L.cacheBank0 + Math.floor(k / 3)), c = (k % 3) * L.chunkSize, kp = c + L.ckKept;
      if (!wb[o + L.csUsed] || !cb) continue;
      s.chunks.push({ sn: wb[o + L.csSn], cx: w16(wb, o + L.csCx), cy: w16(wb, o + L.csCy), q: cb[kp + L.kQ], m: w16(cb, kp + L.kM), n: cb[kp + L.kN],
        map: w16(cb, kp + L.kMap), dq: s8(cb[kp + L.kDq]), dsk: cb[kp + L.kDsk], sk: cb[kp + L.kSk],
        rooms: Array.from({ length: Math.min(cb[c + L.ckNrooms], 6) }, (_, i) => { const r = c + L.ckRoom + i * L.rmSize;
          return { x: cb[r + L.rmX], y: cb[r + L.rmY], w: cb[r + L.rmW], h: cb[r + L.rmH], flags: cb[r + L.rmFlags], kind: cb[r + L.rmKind] }; }) });
    }
    s.slots = [];                                                // (The strata's records in the hub's slots, the world's:
    if (wb) for (let k = 0; k < L.maxstrata; k++) {              //   their bounds, as made)
      const o = L.wStrata + k * L.stSize;
      if (wb[o + L.stMade]) s.slots.push({ n: wb[o + L.stN], x0: w16(wb, o + L.stX0), y0: w16(wb, o + L.stY0), w: w16(wb, o + L.stW), h: w16(wb, o + L.stH) });
    }
    const db = bank(L.denBank);                                  // (The places kept lit, the denizens': hub.h's KEPT_AT)
    if (db) {
      const o = L.keptAt;
      s.kept = { done: Array.from(db.subarray(o, o + 32)), n: db[o + L.kpN], gen: db[o + L.kpGen],
        at: Array.from({ length: Math.min(db[o + L.kpN], 64) }, (_, i) => { const q = o + L.kpAt + i * L.keepSize;
          return { sn: db[q + L.kpSn], range: db[q + L.kpRange], live: db[q + L.kpLive], cx: db[q + L.kpCx], cy: db[q + L.kpCy], lid: w16(db, q + L.kpLid) }; }) };
    }
    const ui = bank(L.uiBank);                                   // (The map's rows, as the maker last drew them)
    if (ui) for (let r = 0; r < L.mapH; r++) s.map.push(str(ui, L.uMap + r * (L.mapW + 1), L.mapW));
    if (ui) s.mapLay = Array.from({ length: L.mapH }, (_, r) => Array.from(ui.subarray(L.uMapLay + r * L.mapW, L.uMapLay + (r + 1) * L.mapW)));   // (Each
    // square's layer, the maker's: ML_NOW, ML_KEPT, ML_DRAWN, ML_PC: slice 4.2's)
    if (ui) {                                                   // (The traces read last: the world's reply, its words;
      const o = L.uTread;                                       //   slice 4.1's)
      s.tread = { how: name('TRC_FLOOR', ui[o + L.trHow]), result: name('TRR_OK', ui[o + L.trResult]), grade: s8(ui[o + L.trGrade]),
        floor: name('MAT_EARTH', ui[o + L.trFloor]), dust: ui[o + L.trDust], trod: ui[o + L.trTrod], smoke: ui[o + L.trSmoke],
        trails: Array.from({ length: Math.min(ui[o + L.trNtr], 3) }, (_, i) => { const q = o + L.trTr + i * L.trailSize;
          return { name: str(ui, q + L.tlName, 12), what: str(ui, q + L.tlWhat, 24), n: name('TP_ONE', ui[q + L.tlN]), ago: name('TA_NOW', ui[q + L.tlAgo]),
            head: ui[q + L.tlHead], kind: ui[q + L.tlKind], age: w16(ui, q + L.tlAge), strength: ui[q + L.tlStrength] }; }),
        text: str(ui, L.uTrtext, L.trText) };
    }
    const jb = bank(L.denBank);                                 // (The traces' journal's counts: the passings so far)
    if (jb) {                                                   // (And the passings in its rings, the oldest first: the
      const ring = (at, size, n) => Array.from({ length: Math.min(n, size) }, (_, i) => {   //   denizens', the character's)
        const o = L.tjAt + at + ((n - Math.min(n, size) + i) % size) * L.tsSize, h = jb[o + L.tsHead];
        return { kind: jb[o + L.tsKind], sn: jb[o + L.tsSn], head: h & 7, noway: !!(h & K.TS_NOWAY), lit: !!(h & K.TS_LIT), stay: !!(h & K.TS_STAY),
          stays: jb[o + L.tsStay], x: w16(jb, o + L.tsX), y: w16(jb, o + L.tsY), t: w32(jb, o + L.tsT) }; });
      s.journal = { n: w16(jb, L.tjAt + L.tjN), pn: w16(jb, L.tjAt + L.tjPn) };
      s.journal.e = ring(L.tjE, L.njournal, s.journal.n);
      s.journal.p = ring(L.tjP, L.ntrail, s.journal.pn);
    }
    const t = bank(L.sightBank);
    if (t) {
      const list = (n, max, at, size, f) => Array.from({ length: Math.min(t[n], max) }, (_, i) => f(at + i * size - at, i));
      const d0 = L.tDoor, o0 = L.tOpen, i0 = L.tItem, n0 = L.tDen, p0 = L.tTrap, g0 = L.tGlow;
      s.sight = { seq: t[L.tSeq], where: t[L.tWhere], dark: t[L.tDark], kind: t[L.tKind], size: t[L.tSize], rw: t[L.tRw], rh: t[L.tRh], rx: w16(t, L.tRx), ry: w16(t, L.tRy),
        lit: t[L.tLit], ceil: t[L.tCeil], sname: t[L.tSname], first: t[L.tFirst], z: w16(t, L.tZ) << 16 >> 16, stratum: t[L.tStratum], x: w16(t, L.tX), y: w16(t, L.tY), nways: t[L.tNways], dazzled: t[L.tDazzled],
        doors: list(L.tNdoors, L.seeDoors, d0, L.tDoorSize, o => ({ rel: t[L.dRel + o], state: t[L.dState + o], x: w16(t, L.dX + o),
          y: w16(t, L.dY + o), kind: name('PK_DOOR', t[L.dKind + o]), parts: t[L.dParts + o] })),
        open: list(L.tNopen, L.seeOpen, o0, L.tOpenSize, o => ({ rel: t[L.oRel + o], kind: name('O_STAIRS_UP', t[L.oKind + o]) })),
        items: list(L.tNitems, L.seeItems, i0, L.tItemSize, o => ({ name: str(t, L.sName + o, 14), qty: t[L.sQty + o], rel: t[L.sRel + o],
          hid: t[L.sHid + o] })),
        dens: list(L.tNdens, L.seeDens, n0, L.tDenSize, o => ({ x: w16(t, L.nX + o), y: w16(t, L.nY + o), doing: name('DO_ROAM', t[L.nDoing + o]),
          name: str(t, L.nName + o, 12) })),
        traps: list(L.tNtraps, L.seeTraps, p0, L.tTrapSize, o => ({ rel: t[L.pRel + o], kind: name('TK_PIT', t[L.pKind + o]),
          trigger: name('TT_PLATE', t[L.pTrigger + o]), state: t[L.pState + o] })),
        glows: list(L.tNglows, L.seeGlows, g0, L.tGlowSize, o => ({ rel: t[L.gRel + o], dist: t[L.gDist + o], how: name('GL_GLOW', t[L.gHow + o]) })) };
      // (A look one way's own, after it in the bank (level.h's struct wayseen): its openings ahead and what's beyond
      // them, the things, the denizens and the lights far off ahead; valid, the record's own: the last look one way's)
      const u = t.subarray(L.wayAt), n = (k, max) => Math.min(u[k], max);
      s.way = { valid: u[0] === s.sight.seq,
        beyond: Array.from({ length: n(L.wNb, L.seeBeyond) }, (_, i) => { const o = L.wB + i * L.wBSize;
          return { what: name('WB_DARK', u[o + L.bWhat]), noun: name('BN_PASSAGE', u[o + L.bNoun]), rel: u[o + L.bRel], dist: u[o + L.bDist],
            kind: u[o + L.bKind], lit: u[o + L.bLit] }; }),
        items: Array.from({ length: n(L.wNitems, L.seeItems) }, (_, i) => { const o = L.wItem + i * L.tItemSize - L.tItem;
          return { name: str(u, L.sName + o, 14), qty: u[L.sQty + o], rel: u[L.sRel + o], hid: u[L.sHid + o] }; }),
        dens: Array.from({ length: n(L.wNdens, L.seeDens) }, (_, i) => { const o = L.wDen + i * L.tDenSize - L.tDen;
          return { x: w16(u, L.nX + o), y: w16(u, L.nY + o), doing: name('DO_ROAM', u[L.nDoing + o]), name: str(u, L.nName + o, 12) }; }),
        glows: Array.from({ length: n(L.wNglows, L.seeGlows) }, (_, i) => { const o = L.wGlow + i * L.tGlowSize - L.tGlow;
          return { rel: u[L.gRel + o], dist: u[L.gDist + o], how: name('GL_GLOW', u[L.gHow + o]) }; }) };
    }
    return s;
  }
  function hook(k, off, v) {
    if (off === 1 && v === (L.magic >> 8) && m) {                // (A hub made: a game begun)
      const b = m.sharedBanks()[k];
      if (b && b[0] === (L.magic & 0xFF)) {
        key0 = k;
        games.push(game = { line: enters, events: [], dens: [], replies: [], snaps: [], tasks: [], eq: 0, dq: 0 });
      }
      return;
    }
    if (!game || k !== key0) return;
    const h = m.sharedBanks()[key0];
    if (off === L.eqHead) {                                     // (An event shown: its entry written whole before)
      if (v !== ((game.eq + 1) & 0xFF)) return;
      game.eq = v;
      game.events.push(decode(h, L.eq + ((v - 1) & (L.eqSize - 1)) * L.msg, false));
    } else if (off === L.dqHead) {
      if (v !== ((game.dq + 1) & 0xFF)) return;
      game.dq = v;
      const b = bank(L.denBank);
      if (b) game.dens.push(decode(b, ((v - 1) & (L.dqSize - 1)) * L.msg, true));
    } else if (off === L.pcTask + L.ltDone + 1) {               // (The long task's part done, its high byte written last)
      const kind = name('LT_NONE', h[L.pcTask + L.ltKind]), done = h[off - 1] | v << 8, t = game.tasks[game.tasks.length - 1];
      if (!t || t.kind !== kind || t.done !== done) game.tasks.push({ line: enters, kind, len: w16(h, L.pcTask + L.ltLen), done, cyc: m.cpu.cyc });
    } else if (v === 2) {                                       // (A reply: its state REQ_DONE, the rest written)
      if (off === L.ireq) game.replies.push({ line: enters, req: 'ireq', kind: ioName(h[off + L.irKind]), result: name('IR_OK', h[off + L.irResult]),
        item: w16(h, off + L.irItem), to: w16(h, off + L.irTo), name: str(h, off + L.irName, 16), into: str(h, off + L.irInto, 16),
        x: w16(h, off + L.irX), y: w16(h, off + L.irY), qty: h[off + L.irQty] });
      else if (off === L.sway) game.replies.push({ line: enters, req: 'sway', kind: name('SW_LOOK', h[off + L.swKind]), word: str(h, off + L.swWord, 16),
        order: name('OR_NONE', h[off + L.swOrder]), result: name('SR_OK', h[off + L.swResult]), grade: s8(h[off + L.swGrade]),
        was: name('AT_HOSTILE', h[off + L.swWas]), now: name('AT_HOSTILE', h[off + L.swNow]), name: str(h, off + L.swName, 12) });
      else if (off === L.wreq) game.replies.push({ line: enters, req: 'wreq', kind: name('WQ_SAVE', h[L.wqKind]), res: h[L.wqRes] });
      else if (off === L.dslot || (off >= L.slot && off < L.slot + L.nslots * L.slotSize && (off - L.slot) % L.slotSize === 0)) {
        game.replies.push({ line: enters, req: off === L.dslot ? 'dslot' : 'slot' + (off - L.slot) / L.slotSize, kind: h[off + L.rqKind],
          actor: h[off + L.rqActor], other: h[off + L.rqOther], a: h[off + L.rqA], b: h[off + L.rqB], c: h[off + L.rqC],
          w1: w16(h, off + L.rqW1), w2: w16(h, off + L.rqW2), grade: s8(h[off + L.rqGrade]), result: h[off + L.rqResult], steps: s8(h[off + L.rqSteps]) });
      }
    }
  }
  // (Each line entered: a CR the CPU reads from the serial port, the state it finds kept first; the lines typed, as
  // the serial port gave them, in lines)
  const lines = [];
  let typed = '';
  function attach(mm) {
    m = mm;
    const a = m.acia, read = a.read;
    a.read = (r, t) => {
      if (r === 0 && a.rdrf && a.rx === 13) {
        enters++;
        lines.push(typed);
        typed = '';
        const s = game ? snap() : null;
        if (s) game.snaps.push(s);
      } else if (r === 0 && a.rdrf && a.rx !== 10) typed += String.fromCharCode(a.rx);
      return read(r, t);
    };
  }
  return { hook, attach, games, lines, final: snap, layout: () => layout(), bank };   // (bank: one of the hub's, a tool's)
}

module.exports = { createRecorder, layout, constants };
