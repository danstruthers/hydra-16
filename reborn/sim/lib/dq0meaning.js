// dq0meaning.js - what happened in a Dingus Quest Zero test, read from the hub (Phase 3A: tests cut to meaning, not
// words): the world's events and the denizens' (their rings, as each is written), the replies to the requests that
// change things (the things', the denizens' sway, the world's, the referee's slots), and at each line entered (a CR
// read from the serial port) the state the line finds: the character's place, facing, light, pace, position and
// summary, the denizens near it, the things in the item store, the names given, what it perceives (the senses'
// record, struct sight), the theme.  hub.h's structs are laid out by cc65 (hubown.js's evaluate: the build's own
// compiler, so the offsets are the game's), kept in obj/dq0meaning.json as the headers stand; their enums and
// constants read from the headers, so an event's numbers come back as names (E_PORTAL, PO_OPENED ...).
//
// createRecorder() gives { hook, attach(m), games, final() }: hook is the machine's opt.sharedWrite (sim/test.js
// chains it with the hub writers' check in every dq0 test), attach(m) once it's booted; games: each game played (a
// hub made: its magic written), { events, dens, replies, snaps }, each record with line, the number of lines entered
// before it.  final(): the state now (the last snapshot, at the test's end).  sim/test.js puts it on the machine as
// m.dq0; tests.js's dq0Turns reads it a line at a time.
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
  rqGrade: F('struct req', 'grade'), rqResult: F('struct req', 'result'),
  now: F('struct hub', 'now'), quit: F('struct hub', 'quit'), seed: F('struct hub', 'seed'), pcstatus: F('struct hub', 'pcstatus'),
  pcSn: F('struct hub', 'pc_stratum'), pcX: F('struct hub', 'pc_x'), pcY: F('struct hub', 'pc_y'), pcFace: F('struct hub', 'pc_face'),
  pcLight: F('struct hub', 'pc_light'), pcLamp: F('struct hub', 'pc_lamp'), pcsum: F('struct hub', 'pcsum'),
  gait: F('struct hub', 'gait'), watch: F('struct hub', 'watch'), pcPos: F('struct hub', 'pc_pos'), pcGoing: F('struct hub', 'pc_going'),
  pcStance: F('struct hub', 'pc_stance'), wayOpen: F('struct hub', 'way_open'), digTool: F('struct hub', 'dig_tool'),
  sCond: F('struct rs_summary', 'cond'), sHealth: F('struct rs_summary', 'health'), sLoad: F('struct rs_summary', 'load'),
  sSight: F('struct rs_summary', 'sight'), sStep: F('struct rs_summary', 'step'), sGstep: F('struct rs_summary', 'gstep'), ngaits: 'NGAITS',
  actor: F('struct hub', 'actor'), actorSize: 'sizeof (struct actor)', nactors: 'NACTORS',
  aState: F('struct actor', 'state'), aSn: F('struct actor', 'sn'), aFace: F('struct actor', 'face'), aPos: F('struct actor', 'pos'),
  aX: F('struct actor', 'x'), aY: F('struct actor', 'y'), aDoing: F('struct actor', 'doing'), aName: F('struct actor', 'name'),
  aAttitude: F('struct actor', 'attitude'), aFaction: F('struct actor', 'faction'), aKind: F('struct actor', 'kind'),
  themeBank: 'THEME_BANK', thName: F('struct theme', 'name'),
  namesAt: '(unsigned) NAMES_AT - 0x8000', tnames: 'TNAMES', tname: 'sizeof (struct tname)', nmWhat: F('struct tname', 'what'),
  nmSn: F('struct tname', 'sn'), nmX: F('struct tname', 'x'), nmY: F('struct tname', 'y'), nmKind: F('struct tname', 'kind'),
  nmName: F('struct tname', 'name'),
  catBank: 'CAT_BANK', kindSize: 'sizeof (struct kind)', nkinds: 'NKINDS', kFlags: F('struct kind', 'flags'), kWeight: F('struct kind', 'weight'),
  item0: 'ITEM_BANK (0)', itemSize: 'sizeof (struct item)', nitems: 'NITEMS', iKind: F('struct item', 'kind'), iFlags: F('struct item', 'flags'),
  iWhere: F('struct item', 'where'), iSn: F('struct item', 'sn'), iX: F('struct item', 'x'), iY: F('struct item', 'y'),
  iQty: F('struct item', 'qty'), iId: F('struct item', 'id'),
  sightBank: 'SIGHT_BANK', tSeq: F('struct sight', 'seq'), tWhere: F('struct sight', 'where'), tDark: F('struct sight', 'dark'),
  tKind: F('struct sight', 'kind'), tSize: F('struct sight', 'size'), tRw: F('struct sight', 'rw'), tRh: F('struct sight', 'rh'),
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
  uiBank: 'UI_BANK', uMap: F('struct uirec', 'map'), mapH: 'MAP_H', mapW: 'MAP_W',
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
  return (LAYOUT = { L: j.L, K: byName, name });
}

// The recorder (a test's machine's, sim/test.js's instrument)
function createRecorder() {
  const { L, K, name } = layout();
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
      case 'E_PORTAL': e.po = name('PO_NONE', w & 0xFF); e.pk = name('PK_DOOR', w >> 8); break;
      case 'E_HARM': e.hc = name('HC_FALL', w & 0xFF); e.band = w >> 8 & 7; e.out = !!(w & 0x4000); e.caught = !!(w & 0x8000);
        e.stopped = e.hc === 'HC_STRUCK' && e.caught; break;          // (E_CAUGHT a fall's, E_STOPPED a blow's)
      case 'E_TRAP': e.tk = name('TK_PIT', w & 15); e.tt = name('TT_PLATE', w >> 4 & 15); e.to = name('TO_SPRUNG', w >> 8); break;
      case 'E_TAKEN': e.ir = name('IR_OK', w & 0xFF); break;
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
      pc: { sn: h[L.pcSn], x: w16(h, L.pcX), y: w16(h, L.pcY), face: h[L.pcFace], light: h[L.pcLight], lamp: str(h, L.pcLamp, 14),
        pos: h[L.pcPos], gait: h[L.gait], watch: h[L.watch], going: h[L.pcGoing], stance: h[L.pcStance] },
      sum: { cond: w16(h, L.pcsum + L.sCond), health: h[L.pcsum + L.sHealth], load: h[L.pcsum + L.sLoad], sight: h[L.pcsum + L.sSight],
        step: h[L.pcsum + L.sStep], gstep: Array.from(h.subarray(L.pcsum + L.sGstep, L.pcsum + L.sGstep + L.ngaits)) },
      actors: [], items: [], names: [], sight: null, theme: '', map: [] };
    for (let k = 0; k < L.nactors; k++) {
      const o = L.actor + k * L.actorSize, a = { k, state: h[o + L.aState], sn: h[o + L.aSn], face: h[o + L.aFace], pos: h[o + L.aPos],
        x: w16(h, o + L.aX), y: w16(h, o + L.aY), doing: name('DO_ROAM', h[o + L.aDoing]), name: str(h, o + L.aName, 12),
        attitude: name('AT_HOSTILE', h[o + L.aAttitude]), faction: h[o + L.aFaction], kind: h[o + L.aKind] };
      if (a.state || a.name) s.actors.push(a);
    }
    const cat = bank(L.catBank), kname = k => cat ? str(cat, k * L.kindSize, 14) : '';     // (CATALOGUE[kind])
    const per = Math.floor(0x2000 / L.itemSize);
    for (let i = 0; i < L.nitems; i++) {
      const b = bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
      if (!b || !b[o + L.iKind]) continue;
      s.items.push({ n: i, kind: b[o + L.iKind], name: kname(b[o + L.iKind]), flags: b[o + L.iFlags], where: name('IW_SQUARE', b[o + L.iWhere]),
        sn: b[o + L.iSn], x: w16(b, o + L.iX), y: w16(b, o + L.iY), qty: b[o + L.iQty], id: w16(b, o + L.iId) });
    }
    const th = bank(L.themeBank);
    if (th) {
      s.theme = str(th, L.thName, 32);
      for (let i = 0; i < L.tnames; i++) {
        const o = L.namesAt + i * L.tname;
        if (!th[o + L.nmWhat]) continue;
        s.names.push({ what: name('NM_NONE', th[o + L.nmWhat]), sn: th[o + L.nmSn], x: w16(th, o + L.nmX), y: w16(th, o + L.nmY),
          kind: str(th, o + L.nmKind, 14), name: str(th, o + L.nmName, 20) });
      }
    }
    const ui = bank(L.uiBank);                                   // (The map's rows, as the maker last drew them)
    if (ui) for (let r = 0; r < L.mapH; r++) s.map.push(str(ui, L.uMap + r * (L.mapW + 1), L.mapW));
    const t = bank(L.sightBank);
    if (t) {
      const list = (n, max, at, size, f) => Array.from({ length: Math.min(t[n], max) }, (_, i) => f(at + i * size - at, i));
      const d0 = L.tDoor, o0 = L.tOpen, i0 = L.tItem, n0 = L.tDen, p0 = L.tTrap, g0 = L.tGlow;
      s.sight = { seq: t[L.tSeq], where: t[L.tWhere], dark: t[L.tDark], kind: t[L.tKind], size: t[L.tSize], rw: t[L.tRw], rh: t[L.tRh],
        lit: t[L.tLit], ceil: t[L.tCeil], first: t[L.tFirst], z: w16(t, L.tZ) << 16 >> 16, stratum: t[L.tStratum], x: w16(t, L.tX), y: w16(t, L.tY), nways: t[L.tNways], dazzled: t[L.tDazzled],
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
    }
    return s;
  }
  function hook(k, off, v) {
    if (off === 1 && v === (L.magic >> 8) && m) {                // (A hub made: a game begun)
      const b = m.sharedBanks()[k];
      if (b && b[0] === (L.magic & 0xFF)) {
        key0 = k;
        games.push(game = { line: enters, events: [], dens: [], replies: [], snaps: [], eq: 0, dq: 0 });
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
    } else if (v === 2) {                                       // (A reply: its state REQ_DONE, the rest written)
      if (off === L.ireq) game.replies.push({ line: enters, req: 'ireq', kind: name('IO_KIT', h[off + L.irKind]), result: name('IR_OK', h[off + L.irResult]),
        item: w16(h, off + L.irItem), to: w16(h, off + L.irTo), name: str(h, off + L.irName, 16), into: str(h, off + L.irInto, 16),
        x: w16(h, off + L.irX), y: w16(h, off + L.irY), qty: h[off + L.irQty] });
      else if (off === L.sway) game.replies.push({ line: enters, req: 'sway', kind: name('SW_LOOK', h[off + L.swKind]), word: str(h, off + L.swWord, 16),
        order: name('OR_NONE', h[off + L.swOrder]), result: name('SR_OK', h[off + L.swResult]), grade: s8(h[off + L.swGrade]),
        was: name('AT_HOSTILE', h[off + L.swWas]), now: name('AT_HOSTILE', h[off + L.swNow]), name: str(h, off + L.swName, 12) });
      else if (off === L.wreq) game.replies.push({ line: enters, req: 'wreq', kind: name('WQ_SAVE', h[L.wqKind]), res: h[L.wqRes] });
      else if (off === L.dslot || (off >= L.slot && off < L.slot + L.nslots * L.slotSize && (off - L.slot) % L.slotSize === 0)) {
        game.replies.push({ line: enters, req: off === L.dslot ? 'dslot' : 'slot' + (off - L.slot) / L.slotSize, kind: h[off + L.rqKind],
          actor: h[off + L.rqActor], other: h[off + L.rqOther], a: h[off + L.rqA], b: h[off + L.rqB], c: h[off + L.rqC],
          w1: w16(h, off + L.rqW1), w2: w16(h, off + L.rqW2), grade: s8(h[off + L.rqGrade]), result: h[off + L.rqResult] });
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
  return { hook, attach, games, lines, final: snap, layout: () => layout() };
}

module.exports = { createRecorder, layout, constants };
