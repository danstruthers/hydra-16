// dq0meaning.js - what happened in a Dingus Quest Zero test, read from the hub (Phase 3A: tests cut to meaning, not
// words): the world's events and the denizens' (their rings, as each is written), the replies to the requests that
// change things (the things', the denizens' sway, the world's, the referee's slots), and at each line entered (a CR
// read from the serial port) the state the line finds: the character's place, facing and bearings, light, pace,
// position and summary, the denizens near it, the things in the item store, the names given, what it perceives (the
// senses' record, struct sight), the theme, its long task (the hub's pc_task), the change records in the cache's pages
// (world.h's: a square's, a portal's state, its harm, a trap's, a chunk stocked, a hole dug part way).  hub.h's
// structs as cc65 laid them out for the build (games/dq0/hubgen.js's obj/gen/dq0hub.json: the build's own compiler, so
// the offsets are the game's; nothing compiled at test time), and the headers' enums and constants, so an event's
// numbers come back as names (E_PORTAL, PO_OPENED ...).
//
// createRecorder() gives { hook, attach(m), games, final() }: hook is the machine's opt.sharedWrite (sim/test.js
// chains it with the hub writers' check in every dq0 test), attach(m) once it's booted; games: each game played (a
// hub made: its magic written), { events, dens, replies, snaps, tasks (the character's long task's part done, as it
// changes: { kind, len, done }) }, each record with line, the number of lines entered before it.  final(): the state
// now (the last snapshot, at the test's end).  sim/test.js puts it on the machine as m.dq0; tests.js's dq0Turns reads
// it a line at a time.
'use strict';

// The layout, by name: the build's (games/dq0/hubgen.js, obj/gen/dq0hub.json: hub.h's structs as cc65 lays them out), a
// field's offset (F), size (S), a struct's size (Z), a header's number (K)
function layoutOf(H) {
  const F = (st, f) => H.off(st, f), S = (st, f) => H.size(st, f), Z = st => H.size(st), K = n => H.value(n);
  return {
  magic: K('HUB_MAGIC'), iqHead: F('struct hub', 'iq_head'), iq: F('struct hub', 'iq'), iqSize: K('IQ_SIZE'), eqHead: F('struct hub', 'eq_head'), eq: F('struct hub', 'eq'), eqSize: K('EQ_SIZE'), msg: Z('msg'),
  mKind: F('struct msg', 'kind'), mWho: F('struct msg', 'who'), mA: F('struct msg', 'a'), mB: F('struct msg', 'b'),
  mW1: F('struct msg', 'w1'), mW2: F('struct msg', 'w2'), mT: F('struct msg', 't'), mX: F('struct msg', 'x'),
  dqHead: F('struct hub', 'dq_head'), dqSize: K('DQ_SIZE'), denBank: K('DEN_BANK'),
  ireq: F('struct hub', 'ireq'), irKind: F('struct ireq', 'kind'), irItem: F('struct ireq', 'item'), irTo: F('struct ireq', 'to'),
  irResult: F('struct ireq', 'result'), irName: F('struct ireq', 'name'), irX: F('struct ireq', 'x'), irY: F('struct ireq', 'y'),
  irQty: F('struct ireq', 'qty'), irInto: F('struct ireq', 'into'),
  sway: F('struct hub', 'sway'), swKind: F('struct sway', 'kind'), swWord: F('struct sway', 'word'), swOrder: F('struct sway', 'order'),
  swResult: F('struct sway', 'result'), swGrade: F('struct sway', 'grade'), swWas: F('struct sway', 'was'), swNow: F('struct sway', 'now'),
  swName: F('struct sway', 'name'),
  wreq: F('struct hub', 'wreq'), wqKind: F('struct hub', 'wreq.kind'), wqRes: F('struct hub', 'wreq.res'),
  slot: F('struct hub', 'slot'), slotSize: Z('req'), nslots: K('NSLOTS'), dslot: F('struct hub', 'dslot'),
  rqKind: F('struct req', 'kind'), rqActor: F('struct req', 'actor'), rqOther: F('struct req', 'other'), rqA: F('struct req', 'a'),
  rqB: F('struct req', 'b'), rqC: F('struct req', 'c'), rqW1: F('struct req', 'w1'), rqW2: F('struct req', 'w2'),
  rqGrade: F('struct req', 'grade'), rqResult: F('struct req', 'result'), rqSteps: F('struct req', 'steps'), reqPosted: K('REQ_POSTED'),
  now: F('struct hub', 'now'), quit: F('struct hub', 'quit'), seed: F('struct hub', 'seed'), pcstatus: F('struct hub', 'pcstatus'),
  pcSn: F('struct hub', 'pc_stratum'), pcX: F('struct hub', 'pc_x'), pcY: F('struct hub', 'pc_y'), pcFace: F('struct hub', 'pc_face'),
  pcLight: F('struct hub', 'pc_light'), pcLamp: F('struct hub', 'pc_lamp'), pcsum: F('struct hub', 'pcsum'),
  gait: F('struct hub', 'gait'), watch: F('struct hub', 'watch'), pcPos: F('struct hub', 'pc_pos'), pcHid: F('struct hub', 'pc_hid'), nolurk: F('struct hub', 'nolurk'), hlook: F('struct hub', 'hlook'), pcGoing: F('struct hub', 'pc_going'),
  pcReady: F('struct hub', 'pc_ready'), hubNow: F('struct hub', 'now'),
  pcChart: F('struct hub', 'pc_chart'), pcChartk: F('struct hub', 'pc_chartk'), pcMap: F('struct hub', 'pc_map'),
  pcStance: F('struct hub', 'pc_stance'), pcStrike: F('struct hub', 'pc_strike'), pcShand: F('struct hub', 'pc_shand'), pcSkew: F('struct hub', 'pc_skew'), pcLost: F('struct hub', 'pc_lost'), tbAt: F('struct hub', 'tb.at'), tbSkew: F('struct hub', 'tb.skew'), tbSkewHi: F('struct hub', 'tb.skew') + 1, tbUnc: F('struct hub', 'tb.unc'), tbUncHi: F('struct hub', 'tb.unc') + 1, tbSky: F('struct hub', 'tb.sky'), tbHour0: F('struct hub', 'tb.hour0'), tbHow: F('struct hub', 'tb.how'), tbHours: F('struct hub', 'tb.hours'), tbDue: F('struct hub', 'tb.due'), tbDue1: F('struct hub', 'tb.due') + 1, tbDue2: F('struct hub', 'tb.due') + 2, tbDue3: F('struct hub', 'tb.due') + 3, tsense: F('struct hub', 'pcsum.timesense'), wayOpen: F('struct hub', 'way_open'), digTool: F('struct hub', 'dig_tool'),
  sCond: F('struct rs_summary', 'cond'), sHealth: F('struct rs_summary', 'health'), sLoad: F('struct rs_summary', 'load'),
  sSight: F('struct rs_summary', 'sight'), sStep: F('struct rs_summary', 'step'), sGstep: F('struct rs_summary', 'gstep'), ngaits: K('NGAITS'),
  sLimbs: F('struct rs_summary', 'limbs'), sLimp: F('struct rs_summary', 'limp'), sNorth: F('struct rs_summary', 'northsense'),
  actor: F('struct hub', 'actor'), actorSize: Z('actor'), nactors: K('NACTORS'), diHead: F('struct hub', 'di_head'), uiKeys: F('struct hub', 'ui_keys'),
  diTail: F('struct hub', 'di_tail'), diAt: F('struct hub', 'di'), msgSize: Z('msg'), diSize: K('DI_SIZE'),
  aState: F('struct actor', 'state'), aSn: F('struct actor', 'sn'), aFace: F('struct actor', 'face'), aPos: F('struct actor', 'pos'),
  aX: F('struct actor', 'x'), aY: F('struct actor', 'y'), aDoing: F('struct actor', 'doing'), aName: F('struct actor', 'name'), aArm: F('struct actor', 'arm'), aDropped: F('struct actor', 'dropped'),
  aAttitude: F('struct actor', 'attitude'), aFaction: F('struct actor', 'faction'), aKind: F('struct actor', 'kind'), aDid: F('struct actor', 'did'),
  aReady: F('struct actor', 'ready'), denSeq: F('struct hub', 'den_seq'), agear: F('struct hub', 'agear'), digT: F('struct hub', 'dig_t'),
  // (An actor's every field by its name, its record's and its heart's too: the tests' pokes by name, not by their place
  // in the record, so a field added doesn't move what they poke; the hub's tasks, task[T_*])
  aDen: F('struct actor', 'den'), aGait: F('struct actor', 'gait'), aStep: F('struct actor', 'step'), aMove: F('struct actor', 'move'),
  aHeart: F('struct actor', 'heart'), aLead: F('struct actor', 'lead'), aPower: F('struct actor', 'power'), aMood: F('struct actor', 'mood'),
  aServe: F('struct actor', 'serve'), aContact: F('struct actor', 'contact'), aBonds: F('struct actor', 'bonds'), task: F('struct hub', 'task'),
  // (The bodies by who, the character's actor 0, pc: ONE-PATH.md's stage 1; the denizens' actor[k] is bodies[k + 1])
  bodies: F('struct hub', 'bodies'), pcBody: F('struct hub', 'pc'), lookAt: F('struct hub', 'look'),
  themeBank: K('THEME_BANK'), thName: F('struct theme', 'name'), thStratum: F('struct theme', 'stratum'), tstrata: K('TSTRATA'),
  tSname: F('struct sight', 'sname'),
  namesAt: K('NAMES_AT') - 0x8000, tnames: K('TNAMES'), tname: Z('tname'), nmWhat: F('struct tname', 'what'),
  nmSn: F('struct tname', 'sn'), nmX: F('struct tname', 'x'), nmY: F('struct tname', 'y'), nmKind: F('struct tname', 'kind'),
  nmName: F('struct tname', 'name'),
  catBank: K('KIND_BANK'), kindSize: Z('kind'), nkinds: K('NKINDS'), kFlags: F('struct kind', 'flags'), kWeight: F('struct kind', 'weight'), kForm: F('struct kind', 'form'), kValue: F('struct kind', 'value'),
  item0: K('ITEM_BANK (0)'), itemSize: Z('item'), nitems: K('NITEMS'), iKind: F('struct item', 'kind'), iFlags: F('struct item', 'flags'),
  iWhere: F('struct item', 'where'), iSn: F('struct item', 'sn'), iX: F('struct item', 'x'), iY: F('struct item', 'y'),
  iQty: F('struct item', 'qty'), iId: F('struct item', 'id'), iFuel: F('struct item', 'fuel'), iWear: F('struct item', 'wear'), iShut: F('struct item', 'shut'), iSize: F('struct item', 'size'), iThick: F('struct item', 'thick'),
  iMat: F('struct item', 'mat'), iQuality: F('struct item', 'quality'), iOwner: F('struct item', 'owner'), iDur: F('struct item', 'dur'), iDurMax: F('struct item', 'dur_max'), iHide: F('struct item', 'hide'), pcFuel: F('struct hub', 'pc_fuel'), pcLeft: F('struct hub', 'pc_left'),
  ioStep: K('IO_STEP'), ioName: K('IO_NAME'), ioTheme: K('IO_THEME'), ioBegin: K('IO_BEGIN'), ioPuzzle: K('IO_PUZZLE'), ioThrow: K('IO_THROW'), ioFill: K('IO_FILL'), ioShut: K('IO_SHUT'), ioInside: K('IO_INSIDE'), ioLook: K('IO_LOOK'), ioSeek: K('IO_SEEK'),
  ioHands: K('IO_HANDS'), sightBank: K('SIGHT_BANK'), tSeq: F('struct sight', 'seq'), tWhere: F('struct sight', 'where'), tDark: F('struct sight', 'dark'),
  tKind: F('struct sight', 'kind'), tSize: F('struct sight', 'size'), tRw: F('struct sight', 'rw'), tRh: F('struct sight', 'rh'), tRx: F('struct sight', 'rx'), tRy: F('struct sight', 'ry'),
  tLit: F('struct sight', 'lit'), tCeil: F('struct sight', 'ceil'), tFirst: F('struct sight', 'first'), tZ: F('struct sight', 'z'),
  tNdoors: F('struct sight', 'ndoors'), tDoor: F('struct sight', 'door'), tDoorSize: S('struct sight', 'door[0]'), seeDoors: K('SEE_DOORS'),
  dRel: F('struct sight', 'door[0].rel'), dState: F('struct sight', 'door[0].state'), dX: F('struct sight', 'door[0].x'),
  dY: F('struct sight', 'door[0].y'), dKind: F('struct sight', 'door[0].kind'), dParts: F('struct sight', 'door[0].parts'),
  tNopen: F('struct sight', 'nopen'), tOpen: F('struct sight', 'open'), tOpenSize: S('struct sight', 'open[0]'), seeOpen: K('SEE_OPEN'),
  oRel: F('struct sight', 'open[0].rel'), oKind: F('struct sight', 'open[0].kind'),
  tNways: F('struct sight', 'nways'),
  tNitems: F('struct sight', 'nitems'), tItem: F('struct sight', 'item'), tItemSize: S('struct sight', 'item[0]'), seeItems: K('SEE_ITEMS'),
  sName: F('struct sight', 'item[0].name'), sQty: F('struct sight', 'item[0].qty'), sRel: F('struct sight', 'item[0].rel'),
  sHid: F('struct sight', 'item[0].hid'),
  tStratum: F('struct sight', 'stratum'), tX: F('struct sight', 'x'), tY: F('struct sight', 'y'),
  tNdens: F('struct sight', 'ndens'), tDen: F('struct sight', 'den'), tDenSize: S('struct sight', 'den[0]'), seeDens: K('SEE_DENS'),
  nX: F('struct sight', 'den[0].x'), nY: F('struct sight', 'den[0].y'), nDoing: F('struct sight', 'den[0].doing'), nName: F('struct sight', 'den[0].name'),
  tNtraps: F('struct sight', 'ntraps'), tTrap: F('struct sight', 'trap'), tTrapSize: S('struct sight', 'trap[0]'), seeTraps: K('SEE_TRAPS'),
  pRel: F('struct sight', 'trap[0].rel'), pKind: F('struct sight', 'trap[0].kind'), pTrigger: F('struct sight', 'trap[0].trigger'),
  pState: F('struct sight', 'trap[0].state'),
  tNglows: F('struct sight', 'nglows'), tGlow: F('struct sight', 'glow'), tGlowSize: S('struct sight', 'glow[0]'), seeGlows: K('SEE_GLOWS'),
  gRel: F('struct sight', 'glow[0].rel'), gDist: F('struct sight', 'glow[0].dist'), gHow: F('struct sight', 'glow[0].how'),
  tDazzled: F('struct sight', 'dazzled'),
  wayAt: K('WAY_AT'), wNb: F('struct wayseen', 'nbeyond'), wB: F('struct wayseen', 'beyond'), wBSize: Z('beyond'),
  seeBeyond: K('SEE_BEYOND'), bWhat: F('struct beyond', 'what'), bNoun: F('struct beyond', 'noun'), bRel: F('struct beyond', 'rel'),
  bDist: F('struct beyond', 'dist'), bKind: F('struct beyond', 'kind'), bLit: F('struct beyond', 'lit'),
  wNitems: F('struct wayseen', 'nitems'), wItem: F('struct wayseen', 'item'), wNdens: F('struct wayseen', 'ndens'),
  wDen: F('struct wayseen', 'den'), wNglows: F('struct wayseen', 'nglows'), wGlow: F('struct wayseen', 'glow'),
  wayNearAt: K('WAY_NEAR_AT'), wId: F('struct wayseen', 'id'), wMore: F('struct wayseen', 'more'), wEye: F('struct wayseen', 'eye'),
  uiBank: K('UI_BANK'), uCap: F('struct uirec', 'cap'), ucapSize: Z('ucap'), ucId: F('struct ucap', 'id'), ucWhat: F('struct ucap', 'what'), ucKind: F('struct ucap', 'kind'),
  uIxn: F('struct uirec', 'ixn'), uIx: F('struct uirec', 'ix'), cixSize: Z('cix'), cxKind: F('struct cix', 'kind'), cxUp: F('struct cix', 'up'), cxFlags: F('struct cix', 'flags'), cxLet: F('struct cix', 'let'),
  uIxks: F('struct uirec', 'ixks'), cixkSize: Z('cixk'), ckName: F('struct cixk', 'name'), uLow: F('struct uirec', 'low'), nlet: K('NLET'), uMap: F('struct uirec', 'map'), mapH: K('MAP_H'), mapW: K('MAP_W'), uMapLay: F('struct uirec', 'maplay'),
  uTread: F('struct uirec', 'tread'), uTrtext: F('struct uirec', 'trtext'), trText: K('TR_TEXT'), trHow: F('struct tread', 'how'),
  trResult: F('struct tread', 'result'), trGrade: F('struct tread', 'grade'), trFloor: F('struct tread', 'floor'), trDust: F('struct tread', 'dust'),
  trTrod: F('struct tread', 'trod'), trSmoke: F('struct tread', 'smoke'), trNtr: F('struct tread', 'ntr'), trTr: F('struct tread', 'tr'),
  trailSize: Z('trail'), tlName: F('struct trail', 'name'), tlWhat: F('struct trail', 'what'), tlN: F('struct trail', 'n'),
  tlAgo: F('struct trail', 'ago'), tlHead: F('struct trail', 'head'), tlKind: F('struct trail', 'kind'), tlAge: F('struct trail', 'age'), tlStrength: F('struct trail', 'strength'),
  tjAt: K('TJ_AT') - 0x8000, tjN: F('struct tjournal', 'n'), tjPn: F('struct tjournal', 'pn'),
  tjE: F('struct tjournal', 'e'), tjP: F('struct tjournal', 'p'), tsSize: Z('tstep'), njournal: K('NJOURNAL'), ntrail: K('NTRAIL'),
  tsKind: F('struct tstep', 'kind'), tsSn: F('struct tstep', 'sn'), tsHead: F('struct tstep', 'head'), tsStay: F('struct tstep', 'stay'),
  tsX: F('struct tstep', 'x'), tsY: F('struct tstep', 'y'), tsT: F('struct tstep', 't'),
  worldBank: K('WORLD_BANK'), wCpage: F('struct wrec', 'cpage'), cpSize: Z('cpage'), ncpages: K('NCPAGES'),
  cpUsed: F('struct cpage', 'used'), cpN: F('struct cpage', 'n'), chBank0: K('CHANGE_BANK (0)'), chPage: K('CH_PAGE'),
  chSize: Z('change'), chSn: F('struct change', 'sn'), chX: F('struct change', 'x'), chY: F('struct change', 'y'),
  chTile: F('struct change', 'tile'), chElev: F('struct change', 'elev'), chLink: F('struct change', 'link'),
  dsenseAt: F('struct hub', 'dsense'), dsenseSize: S('struct hub', 'dsense'),
  // (The sound and the air, slices 4.5 and 4.6: dsense's replies, the character's noise, the background, the draught)
  dsWho: F('struct hub', 'dsense.who'), dsNoise: F('struct hub', 'dsense.noise'), dsRes: F('struct hub', 'dsense.res'),
  dsCost: F('struct hub', 'dsense.cost'), pcNoise: F('struct hub', 'pc_noise'), pcMask: F('struct hub', 'pc_mask'),
  pcBg: F('struct hub', 'pc_bg'), airH: F('struct hub', 'air_h'), airS: F('struct hub', 'air_s'),
  // (Hunger and thirst, the water: slice 4.9's; a sixteen-bit field's high byte its name's Hi, for dq0Poke's hub bytes)
  pcHunger: F('struct hub', 'pc_hunger'), pcHungerHi: F('struct hub', 'pc_hunger') + 1, pcThirst: F('struct hub', 'pc_thirst'),
  pcThirstHi: F('struct hub', 'pc_thirst') + 1, pcWant: F('struct hub', 'pc_want'), pcMeals: F('struct hub', 'pc_meals'),
  pcWet: F('struct hub', 'pc_wet'), pcSwim: F('struct hub', 'pc_swim'), pcUnder: F('struct hub', 'pc_under'), pcBreath: F('struct hub', 'pc_breath'),
  pcTask: F('struct hub', 'pc_task'), ltKind: F('struct ltask', 'kind'), ltLen: F('struct ltask', 'len'), ltDone: F('struct ltask', 'done'),
  ltFoe: F('struct ltask', 'foe'), ltMins: F('struct ltask', 'mins'),
  keptAt: K('KEPT_AT') - 0x8000, kpN: F('struct kept', 'n'), kpGen: F('struct kept', 'gen'), kpAt: F('struct kept', 'at'),
  keepSize: Z('keep'), kpSn: F('struct keep', 'sn'), kpRange: F('struct keep', 'range'), kpLive: F('struct keep', 'live'),
  kpCx: F('struct keep', 'cx'), kpCy: F('struct keep', 'cy'), kpLid: F('struct keep', 'lid'),
  wStrata: F('struct wrec', 'strata'), stSize: Z('stratum'), maxstrata: K('MAXSTRATA'), stMade: F('struct stratum', 'made'),
  stN: F('struct stratum', 'n'), stX0: F('struct stratum', 'x0'), stY0: F('struct stratum', 'y0'), stW: F('struct stratum', 'w'),
  stH: F('struct stratum', 'h'),
  // (The chunks' cache, the character's memory of each: slice 4.2's travel's, dq0travel's)
  cacheBank0: K('CACHE_BANK (0)'), ncslots: K('NCSLOTS'), chunkSize: K('CHUNK_SIZE'), wCslot: F('struct wrec', 'cslot'), csSize: Z('cslot'),
  csUsed: F('struct cslot', 'used'), csSn: F('struct cslot', 'sn'), csCx: F('struct cslot', 'cx'), csCy: F('struct cslot', 'cy'),
  ckNrooms: F('struct chunk', 'nrooms'), ckRoom: F('struct chunk', 'room'), rmSize: Z('room'), rmX: F('struct room', 'x'),
  rmY: F('struct room', 'y'), rmW: F('struct room', 'w'), rmH: F('struct room', 'h'), rmFlags: F('struct room', 'flags'), rmKind: F('struct room', 'kind'),
  ckKept: F('struct chunk', 'kept'), kQ: F('struct ckept', 'q'), kM: F('struct ckept', 'm'), kN: F('struct ckept', 'n'), kMap: F('struct ckept', 'map'),
  kDq: F('struct ckept', 'dq'), kDsk: F('struct ckept', 'dsk'), kSk: F('struct ckept', 'sk'), sMapfade: F('struct rs_summary', 'mapfade'),
  sMapmind: F('struct rs_summary', 'mapmind'),
  };
}

// The headers' enums and #defines with a number: { names: { NAME: value }, blocks: [[NAME ...] an enum's members] }
// (the build's: hubgen.js's load ())
function constants() {
  const H = require('../../games/dq0/hubgen.js').load();
  return { names: H.C, blocks: H.enums };
}

// The layout and the constants
let LAYOUT = null;
function layout() {
  if (LAYOUT) return LAYOUT;
  const H = require('../../games/dq0/hubgen.js').load(), j = { L: layoutOf(H), C: constants() };
  const C = j.C, byName = C.names;
  // (A value's name in the enum a member of which is given, or the value: name('PO_NONE', 1) -> 'PO_OPENED')
  const blockOf = {};
  for (const b of C.blocks) for (const n of b) if (!blockOf[n]) blockOf[n] = b;
  const name = (member, v) => { const b = blockOf[member] || []; const n = b.find(x => byName[x] === v); return n === undefined ? v : n; };
  // (A things' request's kind: items.h's IO_*, the enum's or those #defined after it, IO_NAME ... IO_FILL)
  const D = j.L, ioDefs = { [D.ioStep]: 'IO_STEP', [D.ioName]: 'IO_NAME', [D.ioTheme]: 'IO_THEME', [D.ioBegin]: 'IO_BEGIN', [D.ioPuzzle]: 'IO_PUZZLE', [D.ioThrow]: 'IO_THROW', [D.ioFill]: 'IO_FILL',
    [D.ioShut]: 'IO_SHUT', [D.ioInside]: 'IO_INSIDE', [D.ioLook]: 'IO_LOOK', [D.ioSeek]: 'IO_SEEK', [D.ioHands]: 'IO_HANDS' };
  const ioName = v => { const n = name('IO_KIT', v); return typeof n === 'number' && ioDefs[v] ? ioDefs[v] : n; };
  // (An event's record keeps its slots raw beside its fields by name: no field of an event's named as a slot is)
  for (const [k, p] of Object.entries(H.msgs)) for (const [n] of k.startsWith('E_') ? p.fields : [])
    if (['kind', 'who', 'a', 'b', 'w1', 'w2', 't', 'x', 'line', 'den'].includes(n)) throw new Error('msgs.def: ' + k + '\'s ' + n + ', a slot\'s name in its record');
  return (LAYOUT = { L: j.L, K: byName, name, ioName, H });
}

// The recorder (a test's machine's, sim/test.js's instrument)
function createRecorder() {
  const { L, K, name, ioName, H } = layout();
  let m = null, key0 = -1, game = null, enters = 0;
  const games = [];
  const bank = n => m && key0 >= 0 ? m.sharedBanks()[key0 + n] : null;
  const rqAt = off => off === L.dslot || (off >= L.slot && off < L.slot + L.nslots * L.slotSize && (off - L.slot) % L.slotSize === 0);   // (A slot of the referee's)
  const w16 = (b, o) => b[o] | b[o + 1] << 8, s8 = v => v << 24 >> 24;
  const w32 = (b, o) => (b[o] | b[o + 1] << 8 | b[o + 2] << 16 | b[o + 3] << 24) >>> 0;
  const str = (b, o, n) => { let s = ''; for (let i = 0; i < n && b[o + i]; i++) s += String.fromCharCode(b[o + i]); return s; };
  // An event's record (a ring's entry: struct msg at o of bank b; den, the denizens' ring's): its slots as they are, and
  // its payload's fields by name (games/dq0/msgs.def's, the build's decoder: the form its selector chooses, E_TASK's by its
  // TE_*, else the kind's), beside them (none named as a slot is: layout's check)
  function decode(b, o, den) {
    const e = { line: enters, kind: name('E_NONE', b[o + L.mKind]), who: b[o + L.mWho], a: b[o + L.mA], b: b[o + L.mB],
      w1: w16(b, o + L.mW1), w2: w16(b, o + L.mW2), t: w32(b, o + L.mT), x: Array.from(b.subarray(o + L.mX, o + L.mX + 4)), den };
    return Object.assign(e, H.decode(b, o, e.kind, ''));
  }
  // An intent's record (struct msg at o of bank b, the ring's: 'ui', the hub's iq, or 'den', its di): its kind, who,
  // and its payload's fields by name (games/dq0/msgs.def's, the build's decoder: a denizen's its :den form's, if its kind
  // has one), its slots as they are in raw
  function intent(b, o, ring) {
    const kind = name('I_NONE', b[o + L.mKind]);
    return Object.assign({ line: enters, ring, kind, who: b[o + L.mWho] }, H.decode(b, o, kind, ring === 'den' ? 'den' : ''),
      { raw: { a: b[o + L.mA], b: b[o + L.mB], w1: w16(b, o + L.mW1), w2: w16(b, o + L.mW2), t: w32(b, o + L.mT),
        x: Array.from(b.subarray(o + L.mX, o + L.mX + 4)) } });
  }
  // The state as it is now (the hub's records): see the comment at the top
  function snap() {
    const h = bank(0);
    if (!h || h[0] !== (L.magic & 0xFF) || h[1] !== L.magic >> 8) return null;
    const s = { line: enters, game: games.length - 1, cyc: m.cpu.cyc, now: w32(h, L.now), quit: h[L.quit], wayOpen: h[L.wayOpen],
      status: str(h, L.pcstatus, 16), digTool: [0, 1, 2, 3].map(i => str(h, L.digTool + i * 8, 8)),
      pc: { sn: h[L.pcSn], x: w16(h, L.pcX), y: w16(h, L.pcY), face: h[L.pcFace], light: h[L.pcLight], lamp: str(h, L.pcLamp, 14), fuel: w16(h, L.pcFuel), left: str(h, L.pcLeft, 20),
        pos: h[L.pcPos], hid: h[L.pcHid], gait: h[L.gait], watch: h[L.watch], going: h[L.pcGoing], stance: name('D_NONE', h[L.pcStance]), strike: name('SK_BEST', h[L.pcStrike]), shand: h[L.pcShand],
        tb: { at: w16(h, L.tbAt), due: (w16(h, L.tbDue) | w16(h, L.tbDue + 2) * 65536), skew: w16(h, L.tbSkew) << 16 >> 16, unc: w16(h, L.tbUnc), sky: h[L.tbSky], hour0: h[L.tbHour0], how: h[L.tbHow], hours: w16(h, L.tbHours), tsense: h[L.tsense] },
        skew: h[L.pcSkew], lost: h[L.pcLost], noise: h[L.pcNoise], mask: h[L.pcMask], bg: h[L.pcBg], airH: h[L.airH], airS: h[L.airS],
        task: { kind: name('LT_NONE', h[L.pcTask]), asking: h[L.pcTask + 1], len: w16(h, L.pcTask + 2), done: w16(h, L.pcTask + 4), foe: h[L.pcTask + L.ltFoe] - 1, mins: h[L.pcTask + L.ltMins] },
        ready: w16(h, L.pcReady) + w16(h, L.pcReady + 2) * 65536, now: w16(h, L.hubNow) + w16(h, L.hubNow + 2) * 65536,
        chart: h[L.pcChart], chartk: h[L.pcChartk], map: w16(h, L.pcMap), state: h[L.pcBody + L.aState] },   // (state: its body's, actor 0's)
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
        kflags: cat ? cat[b[o + L.iKind] * L.kindSize + L.kFlags] : 0, kform: cat ? cat[b[o + L.iKind] * L.kindSize + L.kForm] : 0,
        kvalue: cat ? w16(cat, b[o + L.iKind] * L.kindSize + L.kValue) : 0, size: b[o + L.iSize], flags: b[o + L.iFlags], where: name('IW_SQUARE', b[o + L.iWhere]),
        sn: b[o + L.iSn], x: w16(b, o + L.iX), y: w16(b, o + L.iY), qty: b[o + L.iQty], id: w16(b, o + L.iId), fuel: w16(b, o + L.iFuel),
        thick: b[o + L.iThick], mat: b[o + L.iMat], quality: b[o + L.iQuality], shut: b[o + L.iShut],
        dur: w16(b, o + L.iDur), durMax: w16(b, o + L.iDurMax),
        did: b[o + L.iWhere] === K.IW_DEN ? b[o + L.iOwner] | b[o + L.iHide] << 8 : 0, owner: b[o + L.iOwner], hide: b[o + L.iHide],
        fits: b[o + L.iSize] | b[o + L.iThick] << 8 });   // (A key's lock, the thing's number: the belongings', 2026-10-27)
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
    s.task = { kind: name('LT_NONE', h[L.pcTask + L.ltKind]), len: w16(h, L.pcTask + L.ltLen), done: w16(h, L.pcTask + L.ltDone), foe: h[L.pcTask + L.ltFoe] - 1, mins: h[L.pcTask + L.ltMins] };
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
        known: Array.from({ length: L.seeDoors - Math.min(t[L.tNdoors], L.seeDoors) }, (_, i) => (i + Math.min(t[L.tNdoors], L.seeDoors)) * L.tDoorSize)   // (Those it remembers, after them: KNOWN_REL's, 0x40)
          .filter(o => t[L.dRel + o] & 0x40).map(o => ({ rel: t[L.dRel + o], state: t[L.dState + o], x: w16(t, L.dX + o), y: w16(t, L.dY + o), kind: name('PK_DOOR', t[L.dKind + o]) })),
        open: list(L.tNopen, L.seeOpen, o0, L.tOpenSize, o => ({ rel: t[L.oRel + o], kind: name('O_STAIRS_UP', t[L.oKind + o]) })),
        items: list(L.tNitems, L.seeItems, i0, L.tItemSize, o => ({ name: str(t, L.sName + o, 14), qty: t[L.sQty + o], rel: t[L.sRel + o],
          hid: t[L.sHid + o] })),
        dens: list(L.tNdens, L.seeDens, n0, L.tDenSize, o => ({ x: w16(t, L.nX + o), y: w16(t, L.nY + o), doing: name('DO_ROAM', t[L.nDoing + o]),
          name: str(t, L.nName + o, 12) })),
        traps: list(L.tNtraps, L.seeTraps, p0, L.tTrapSize, o => ({ rel: t[L.pRel + o], kind: name('TK_PIT', t[L.pKind + o]),
          trigger: name('TT_PLATE', t[L.pTrigger + o]), state: t[L.pState + o] })),
        glows: list(L.tNglows, L.seeGlows, g0, L.tGlowSize, o => ({ rel: t[L.gRel + o], dist: t[L.gDist + o], how: name('GL_GLOW', t[L.gHow + o]) })) };
      // (A look one way's own, after it in the bank (level.h's struct wayseen): its openings ahead and what's beyond
      // them, the things, the denizens and the lights far off ahead; valid, the record's own: the last look one way's.
      // And its near part's, told first (WAY_NEAR: the user's, 2026-11-17), its own: an opening held back for the far
      // part, its what's held true (WB_HOLD); more WM_*'s, id the look one way it's of)
      const way = (u, valid) => { const n = (k, max) => Math.min(u[k], max); return { valid, id: u[L.wId], more: u[L.wMore], eye: u[L.wEye],
        beyond: Array.from({ length: n(L.wNb, L.seeBeyond) }, (_, i) => { const o = L.wB + i * L.wBSize;
          return { what: name('WB_DARK', u[o + L.bWhat] & 0x7F), held: !!(u[o + L.bWhat] & 0x80), noun: name('BN_PASSAGE', u[o + L.bNoun]), rel: u[o + L.bRel], dist: u[o + L.bDist],
            kind: u[o + L.bKind], lit: u[o + L.bLit] }; }),
        items: Array.from({ length: n(L.wNitems, L.seeItems) }, (_, i) => { const o = L.wItem + i * L.tItemSize - L.tItem;
          return { name: str(u, L.sName + o, 14), qty: u[L.sQty + o], rel: u[L.sRel + o], hid: u[L.sHid + o] }; }),
        dens: Array.from({ length: n(L.wNdens, L.seeDens) }, (_, i) => { const o = L.wDen + i * L.tDenSize - L.tDen;
          return { x: w16(u, L.nX + o), y: w16(u, L.nY + o), doing: name('DO_ROAM', u[L.nDoing + o]), name: str(u, L.nName + o, 12) }; }),
        glows: Array.from({ length: n(L.wNglows, L.seeGlows) }, (_, i) => { const o = L.wGlow + i * L.tGlowSize - L.tGlow;
          return { rel: u[L.gRel + o], dist: u[L.gDist + o], how: name('GL_GLOW', u[L.gHow + o]) }; }) }; };
      s.way = way(t.subarray(L.wayAt), t[L.wayAt] === s.sight.seq);
      s.wayNear = way(t.subarray(L.wayNearAt), !!(t[L.wayNearAt + L.wMore] & 1));
    }
    return s;
  }
  function hook(k, off, v) {
    if (off === 1 && v === (L.magic >> 8) && m) {                // (A hub made: a game begun)
      const b = m.sharedBanks()[k];
      if (b && b[0] === (L.magic & 0xFF)) {
        key0 = k;
        games.push(game = { line: enters, events: [], dens: [], replies: [], snaps: [], tasks: [], intents: [], asked: {}, eq: 0, dq: 0, iq: 0, di: 0 });
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
    } else if (off === L.iqHead || off === L.diHead) {          // (An intent shown, the UI's or the denizens': its entry
      const ui = off === L.iqHead, n = ui ? L.iqSize : L.diSize, was = ui ? game.iq : game.di;   //   written whole before)
      if (v !== (was + 1) % n) return;
      if (ui) game.iq = v; else game.di = v;
      game.intents.push(intent(h, (ui ? L.iq : L.diAt) + ((v + n - 1) % n) * L.msg, ui ? 'ui' : 'den'));
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
      else if (off === L.dsenseAt) game.replies.push({ line: enters, req: 'dsense', who: h[L.dsWho], noise: h[L.dsNoise], res: h[L.dsRes],
        cost: h[L.dsCost], now: w16(h, L.hubNow) + w16(h, L.hubNow + 2) * 65536, pc: { x: w16(h, L.pcX), y: w16(h, L.pcY), noise: h[L.pcNoise] },
        x: w16(h, L.actor + h[L.dsWho] * L.actorSize + L.aX), y: w16(h, L.actor + h[L.dsWho] * L.actorSize + L.aY) });
      else if (rqAt(off)) {
        const rs = name('RS_HELLO', h[off + L.rqKind]);       // (Its request's fields as it was asked, and its reply's, by
        game.replies.push({ line: enters, req: off === L.dslot ? 'dslot' : 'slot' + (off - L.slot) / L.slotSize, kind: h[off + L.rqKind],   //   name)
          actor: h[off + L.rqActor], other: h[off + L.rqOther], a: h[off + L.rqA], b: h[off + L.rqB], c: h[off + L.rqC],
          w1: w16(h, off + L.rqW1), w2: w16(h, off + L.rqW2), grade: s8(h[off + L.rqGrade]), result: h[off + L.rqResult], steps: s8(h[off + L.rqSteps]),
          rs, ask: game.asked[off] || {}, reply: H.payload(rs, 'reply') !== H.payload(rs) ? H.decode(h, off, rs, 'reply') : {} });
      }
    } else if (v === L.reqPosted && rqAt(off)) {               // (A request posted, the referee's: its fields by name, kept
      game.asked[off] = H.decode(h, off, name('RS_HELLO', h[off + L.rqKind]), '');   //   for its reply's record)
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
  // (Its state, and put back: a test scene's, sim/lib/scenes.js; games and lines kept the same arrays)
  const state = () => ({ key0, enters, typed, game: game ? games.indexOf(game) : -1, games, lines });
  const restore = s => {
    key0 = s.key0; enters = s.enters; typed = s.typed;
    games.splice(0, games.length, ...s.games); lines.splice(0, lines.length, ...s.lines);
    game = s.game >= 0 ? games[s.game] : null;
  };
  return { hook, attach, games, lines, final: snap, layout: () => layout(), bank, state, restore };   // (bank: one of the hub's, a tool's)
}

module.exports = { createRecorder, layout, constants };
