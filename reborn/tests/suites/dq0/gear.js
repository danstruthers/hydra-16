// ****************************************************************************
// gear.js - Dingus Quest Zero's tests: the things, the kit and money.  (tests.js, the index: the suites in
// their order; ../../lib: the helpers they share)
'use strict';
const fs = require('fs');
const path = require('path');
const { typed } = require('../../lib/common.js');
const {
  dq0Story, dq0Prose, inTurn, dq0Answers, dq0Turns, dq0Ev, dq0Rep, dq0Evs, dq0Need, dq0Never, dq0Carried, dq0Here, dq0Thing,
  dq0Holder, DQ0_I, dq0Typed, dq0Rec, dq0Poke, dq0Aware, dq0PortalAt, dq0OnWords, dq0Timer, WORN, dq0Paces, dq0Own, dq0Asks, dq0PlayLetters,
  dq0HeldPaces, dq0Alone
} = require('../../lib/dq0.js');
const { target, D17 } = require('../../lib/trends.js');   // (The user's targets, D17, at 115,200 baud: each measure kept, those over and past 90% listed after the tests)
// (dq0stopsweep's stops: 0.5M, 1.5M ... 23.5M cycles after the reach's Enter, its whole sequence, a step at a time)
const STOPS = Array.from({ length: 24 }, (_, i) => 500000 + i * 1000000);
const TESTS = path.join(__dirname, '..', '..');   // (The tests' folder: their fixtures')

const TORCHX = ['inventory', 'drop torch', 'take torch', 'inventory', 'wait 2 hours', 'light a new torch', 'take the spent torch from the ground', 'drop the spent torch',
  'drop the lit torch', 'take the spent torch from the floor', 'drop torch', 'take the lit torch', 'hold the tinderbox', 'take torch', 'inventory', 'douse the torch', 'light the torch',
  'inventory', 'take spent torch from backpack', 'put the rope in the backpack', 'put the tinderbox in the backpack', 'dip the spent torch in the oil', 'inventory'];   // (dq0torchwords' lines: the user's playtest, 2026-11-06)
const EMPTYPLACE = ['drop the chest', 'drop the table', 'unlock the chest with the lockpicks', 'unlock the chest', 'put the key in the sack', 'empty the sack', 'empty the sack', 'take the key', 'put the key in the sack', 'empty the sack onto the table', 'open the flask of oil', 'empty the flask of oil', 'empty the torch', 'take the key from the table', 'put the key in the sack', 'tie the sack', 'empty the sack', 'untie the sack', 'take the key from the sack', 'put the key in the corner', 'put the dagger in the middle of the room', 'put the key on the ledge', 'look'];   // (dq0emptyplace's: P4, P5, P6, 2026-11-18)
const POUR = ['d', 'n', 'fill the waterskin', 's', 'light the torch', 'drop the iron pot', 'open the waterskin', 'open the vial', 'pour the water into the vial', 'pour out the vial', 'empty the waterskin into the vial', 'empty the waterskin into the iron pot', 'empty the vial into the iron pot', 'empty the waterskin into the vial', 'empty the iron pot into the sack', 'close the vial', 'empty the iron pot into the vial', 'pour the water', 'look'];   // (dq0pour's: H11, 2026-11-19)
const OIL = ['drop the iron pot', 'open the flask of oil', 'open the vial', 'pour the oil into the vial', 'empty the vial into the iron pot', 'empty the iron pot into the vial', 'light the lantern', 'wait 1 hour', 'fill the lantern from the vial', 'pour the oil into the flask of oil', 'empty the vial', 'open the waterskin', 'pour the oil into the waterskin', 'fill the waterskin', 'empty the waterskin', 'empty the flask', 'look'];   // (dq0oil's: H14, 2026-11-19)
const LETSIGHT = ['drop j', 'drop c', 'drop b', 'look', 'around', 'take 5F', 'drop c', 'around', 'take 2F', 'drop rations', 'take torch from E', 'drop k',
  'take G', 'around', 'inventory'];   // (dq0letsight's lines: the rats seen first, A, B, C, 2026-11-06's step 4)
const LETCMD = ['inventory', 'drop j', 'inventory', 'take torch', 'put j in b', 'inventory', 'take j from b', 'light j with a', 'drop 2c', 'look d',
  'drop', 'k', 'take off', 'look', 'drop the lit torch', 'inventory'];   // (dq0letcmd's lines)
const THINGSV = ['go to the door ahead', 'back up', 'drop the chest', 'push the chest', 'open the door', 'pull the chest', 'open the door', 'pull the door',
  'push the door', 'pull the portcullis', 'use the iron key on the chest', 'use the rations', 'use the waterskin', 'use the torch', 'use the bandages', 'use the dagger',
  'use the whetstone on the dagger', 'taste the antidote', 'taste the remedy', 'taste the vial', 'use the antidote', 'douse the torch', 'feel the wall', 'touch the chest'];   // (dq0things' first game's lines)
const WEARONLY = ['inventory', 'wear the sword', 'wear the torch', 'wear the sack', 'wear', 'k', 'stow the sword', 'equip the sword', 'inventory',
  'stow the sword', 'wield the sword', 'inventory'];   // (dq0wearonly's lines: k the cloak's letter, the answer to "Wear what? [k]")
const WORNKIT = [['wear the ring on my left hand', 'wear the ring on my right hand', 'put the scarf over my face', 'inventory', 'quit'],   // (dq0wornkit's: a game,
  ['inventory', 'drop the hatchet', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u'], ['inventory', 'quit']];   //   carried on, the next dungeon)
const RIPLINES = ['inventory', 'wear the robe', 'rip the robe', 'rip the leather vest', 'rip the rope', 'drop the dagger', 'rip the cloak',
  'tear up the surcoat', 'take the dagger', 'cut up the cloak with the dagger', 'inventory', 'examine the bandages',
  'use the dagger on the towel'];   // (dq0rip's lines; the last a use's, things' verbs, routed to rip)
const DRESSLINES = ['inventory', 'put the bandages in the backpack', 'bandage my left arm', 'sheet', 'change the bandage on my right arm', 'inventory', 'sheet',
  'wash the bandages', 'wash the bandages', 'inventory'];   // (dq0dress's lines)
// (A dq0 game's answers, a line's each: from its echo to the next prompt, the story's colours and boxes gone)
const dq0Segs = (out, lines) => {
  const o = dq0Story(out).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
  let at = 0;
  for (const l of lines) {
    let i = o.indexOf('> ' + l + ' ', at);
    if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;
    const e = i < 0 ? -1 : o.indexOf('> ', i + 2);
    seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
    if (i >= 0) at = i + 2;
  }
  return seg;
};
const LETWHICH = ['drop j', 'pick up the torch', 'inventory', 'drop j', 'drop a', 'drop k', 'take the torch', 'F', 'take the torch', '2', 'inventory',
  'drop j', 'take torch', 'lit', 'inventory'];   // (dq0letwhich's lines)
const LETSEVERAL = ['drop j', 'drop h i', 'look', 'take E F', 'inventory', 'letter h as t', 'inventory', 'drop t', 'letter q as r'];   // (dq0letseveral's lines)
const DOFF = ['hold the sword', 'wear the cloak', 'wear the iron helm', 'i', 'take off the cloak', 'take off the iron helm', 'take off the backpack', 'i'];   // (dq0doff's lines)
const LETSTOW = ['drop j', 'hold the dagger', 'stow the dagger', 'f', 'hang l', 'drop 1g', 'take the pennies', 'inventory', 'hold i', 'stow i', 'b',
  'hang the sheath', 'hold the dagger', 'stow the dagger', 'inventory'];   // (dq0letstow's lines)
const LETTERX = [['inventory', 'drop the pick', 'inventory', 'take the pick', 'inventory', 'quit'],   // (dq0letters' lines: a game, carried on, the
  ['inventory', 'drop the hatchet', 'combine the sunstone with the moonstone', 'use the seal on the way out', 'u'], ['inventory', 'quit']];   //   next dungeon)

module.exports = {
  tests: [
    {
      name: 'dq0highkinds', near: ['gear'], sdcard: true, what: "Dingus Quest Zero's kinds past 127 (the margins' pass, 2026-11-03: a kind's number a whole byte everywhere, 60 more in the catalogue's room, items.h's NKINDS; HOUSE.md's digging): a mang in hand, struck with (the hub's pc_strike SK_OF its kind, its name in the words), a mattock kept in a save and read back; a mang in a long task named by its kind; a mattock on a denizen's arm, let fall and taken up (the actor's arm, E_BEAT WB_PICKS); a wood chisel no use on stone, told so, a stone chisel with a hammer a chisel's dig, a spade and a mattock told as a shovel is",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 26 -u feudal -c hunter -o /ram/c.hm', 'echo ml dge 90 >> /ram/c.hm', 'echo ml swd 60 >> /ram/c.hm',
        'dq0 -p t -s 14 -c /ram/c.hm -d /ram/s -k mang,mattock', 'inventory', 'hit the door with the mang', 'quit', 'dq0 -d /ram/s', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k mang,whetstone', 'sharpen the mang', 'quit',
        "dq0 -p t -s 374 -c /ram/c.hm -k 'wood chisel' -T", 'dig north', 'quit',
        "dq0 -p t -s 374 -c /ram/c.hm -k 'stone chisel,hammer' -T", 'dig north', 'quit',
        'dq0 -p t -s 374 -c /ram/c.hm -k spade -T', 'dig north', 'quit',
        'dq0hour=128', 'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 96 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'mattock,gambeson,mail hauberk' -R 21", 'drop the mattock', 'drop the backpack', 'wear the gambeson', 'wear the mail hauberk', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {
        dq0Poke(m, [{ g: 3, line: 'sharpen the mang', n: 1, wear: { mang: 200 } }]);
        const a = m.acia, shown = a.shown;      // (A mattock on the arm of the bandit that strikes first, let fall where it stands: taken up, its next turn)
        let tail = '', armed = false;
        a.shown = (v, t) => {
          shown(v, t);
          tail = (tail + String.fromCharCode(v)).slice(-40);
          if (armed || !tail.includes('strikes at you with their sword')) return;
          const h = m.dq0.bank(0), { L, K } = m.dq0.layout(), per = Math.floor(0x2000 / L.itemSize), at = k => L.actor + k * L.actorSize;
          for (let k = 0; k < L.nactors && !armed; k++) {
            if (!h[at(k) + L.aState] || h[at(k) + L.aDoing] === undefined) continue;
            if (Math.abs((h[at(k) + L.aX] | h[at(k) + L.aX + 1] << 8) - (h[L.pcX] | h[L.pcX + 1] << 8)) > 1 || Math.abs((h[at(k) + L.aY] | h[at(k) + L.aY + 1] << 8) - (h[L.pcY] | h[L.pcY + 1] << 8)) > 1) continue;
            for (let i = 1; i < L.nitems && !armed; i++) {
              const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
              if (!b || b[o + L.iKind] !== K.KI_MATTOCK) continue;
              b[o + L.iWhere] = K.IW_SQUARE; b[o + L.iSn] = h[at(k) + L.aSn];
              b[o + L.iX] = h[at(k) + L.aX]; b[o + L.iX + 1] = h[at(k) + L.aX + 1]; b[o + L.iY] = h[at(k) + L.aY]; b[o + L.iY + 1] = h[at(k) + L.aY + 1];
              h[at(k) + L.aArm] = K.KI_MATTOCK; h[at(k) + L.aDropped] = i & 255; h[at(k) + L.aDropped + 1] = i >> 8;
              armed = true;
            }
          }
        };
      },
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), K = m.dq0.layout().K;
        if (!(K.KI_MATTOCK > 127 && K.KI_SPADE > 127 && K.KI_MANG > 127 && K.KI_WOOD_CHISEL > 127)) f.push('the kinds here not past 127');
        const i0 = T.of('inventory');
        if (!/mattock/.test(s.slice(s.indexOf('You carry')))) f.push('the mattock not carried');
        const h0 = T.of('hit the door with the mang');
        if (!h0.after || h0.after.pc.strike !== K.KI_MANG + 3) f.push('not struck with the mang: pc_strike ' + (h0.after && h0.after.pc.strike));
        if (!/with the mang/.test(s)) f.push('its name not in the blow\'s words');
        const kept = T.of('dq0 -d /ram/s'), i1 = T.after(kept, 'inventory');
        if (!i1.at || !i1.at.items.some(i => i.name === 'mattock')) f.push('the mattock not back from the save');
        if (!/You set to sharpening the mang/.test(s)) f.push('the long task on a mang not named by its kind');
        if (!/Your wood chisel doesn't seem to be working on the /.test(s)) f.push('the wood chisel not told as no use');
        if (!/Your spade only scratches at the /.test(s)) f.push('the spade not told as a shovel is');
        const pick = T.flatMap(t => [...t.events, ...t.dens]).find(e => e.kind === 'E_BEAT' && e.beat === 'WB_PICKS' && e.b > 0);
        if (!pick || pick.thing !== K.KI_MATTOCK) f.push('no mattock taken up from a denizen\'s arm: ' + JSON.stringify(pick));
        if (!/snatches up their mattock/.test(s)) f.push('not told as the bandit snatches up their mattock');
        return f;
      }
    },
    {
      name: 'dq0things', near: ['gear', 'parser'], what: 'Dingus Quest Zero\'s things\' verbs (the verbs plan\'s B4, 2026-11-10: DECISIONS.md\'s "The things\' verbs: use, push, pull, touch, taste"): use routed by the thing\'s kind, after the puzzle\'s (food eaten, a waterskin drunk, a light lit, a bandage tending, a blade wielded, a whetstone sharpening the blade it\'s used on, a key unlocking the chest it\'s used on, an antidote drunk); a chest dropped, shoved a square against a door (Strength against its weight: the character strong), the door refused ("Something heavy stands against it"), the chest dragged back (the character stepping back), the door opened; a door pulled shut and pushed open; a portcullis pulled down; the pantry door\'s bar pulled out of its brackets; a grapple\'s rope pulled from below, shaken loose; touch in the dark (the felt words), a chest touched; taste: an antidote and a remedy named by Lore, an empty vial, still water (the pool it stands in, a waterskin filled there)',
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo att str 20 >> /ram/c.hm', 'echo ml lor 95 >> /ram/c.hm',
        "dq0 -p t -s 3 -c /ram/c.hm -T -k 'chest,iron key,antidote,remedy,vial,whetstone,bandages'", ...THINGSV, 'quit',
        'dq0 -p t -s 15 -c /ram/c.hm', 'go to the door ahead to my left', 'pull the bar', 'quit',
        'echo ml cli 90 >> /ram/c.hm', 'echo ml thr 95 >> /ram/c.hm', 'dq0sky=1', 'dq0hour=12', "dq0 -p t -s 2 -c /ram/c.hm -T -k 'rope,grapple'",
        'go to the shaft', 'throw the grapple up', 'pull the rope', 'quit', 'dq0sky=0',
        'mkdir /ram/g', "dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k 'waterskin'", 'd', 'n', 'taste the water', 'fill the waterskin', 'taste the waterskin', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], ans = dq0Answers(out), g = {};
        const said = (line, want, not) => {           // (Each line's answer, the nth of it in turn)
          const n = g[line] = (g[line] || 0) + 1, a = ans.filter(y => y.cmd === line)[n - 1], x = a ? a.text : '';
          for (const w of [].concat(want)) if (!(w instanceof RegExp ? w.test(x) : x.includes(w))) f.push(line + ' (' + n + '): not ' + w + ': ' + x.slice(0, 240));
          for (const w of [].concat(not || [])) if (x.includes(w)) f.push(line + ' (' + n + '): "' + w + '" told: ' + x.slice(0, 240));
        };
        const no = ['in this game yet', 'Use what', 'can\'t see how', 'Nothing comes of'];
        said('use the rations', 'You\'re not hungry. You leave the rations be.', no);
        said('use the waterskin', /You're not thirsty|You drink/, no);
        said('use the torch', 'The torch is lit already.', no);
        said('use the bandages', /wounds|tend/, no);
        said('use the dagger', 'You take the dagger in your', no);
        said('use the whetstone on the dagger', 'The dagger is as sharp as it will go.', no);
        said('use the iron key on the chest', /chest/, no);
        said('use the antidote', 'You drink the antidote.', no);
        said('push the chest', ['You set your shoulder to the chest and shove it a square away.', 'It stands against the door.']);
        said('open the door', 'Something heavy stands against it: it won\'t open.', 'You open the door');
        said('pull the chest', 'You take hold of the chest and drag it toward you, stepping back.');
        said('open the door', 'You open the door');
        said('pull the door', 'You close the door.');
        said('push the door', 'You open the door');
        said('pull the portcullis', 'the portcullis crashes down');
        said('taste the antidote', 'an antidote against poisons and venoms');
        said('taste the remedy', 'a remedy against disease');
        said('taste the vial', 'There\'s nothing in it to taste.');
        said('feel the wall', ['It is dark.', 'You feel']);
        said('touch the chest', 'The chest is', 'I don\'t understand');
        said('pull the bar', 'You lift the bar out of its brackets');
        said('pull the rope', 'You shake the rope');
        said('taste the water', 'still water, not fresh');
        said('taste the waterskin', 'still water, not fresh');
        // (A sip of still water the flux's risk, the user's, 2026-11-11: the referee told it's drunk foul, no pints (a tenth of
        // a drink's chance, afflict.c's; dq0rules' sips); a taste of the antidote, no time asked with a drink)
        const T = dq0Turns(m), sip = l => T.of(l).replies.filter(r => r.rs === 'RS_TIME' && r.ask.foul && !r.ask.pints && !r.ask.taken);
        for (const l of ['taste the water', 'taste the waterskin']) if (sip(l).length !== 1) f.push(l + ': not a sip of foul water told the referee (' + sip(l).length + ')');
        if (T.of('taste the antidote').replies.some(r => r.rs === 'RS_TIME' && (r.ask.foul || r.ask.pints))) f.push('taste the antidote: a drink told the referee');
        if (/in this game yet/.test(dq0Story(out))) f.push('a "not yet" said');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0outfit', outfit: true, what: 'Dingus Quest Zero\'s starting outfits (games/dq0/outfits.txt, the world\'s draw: outfit.c, outdraw.c, outmake.c; DECISIONS.md\'s outfits, HOUSE.md\'s), by the things each character begins with (the store\'s records) and hmchar\'s files: the family\'s wealth rolled and kept (its "wealth" line, the book\'s by its class); the universal kit (clothes and footwear worn, a backpack and a belt worn, a pouch on the belt, a torch lit in hand, a tinderbox, three rations, a waterskin full, a knife in its sheath, a spare torch and a flask of oil), bought at the book\'s prices within the wealth (seed 5\'s hunter, a serf: sandals; its Spear a staff in hand, all it could pay for); seed 10\'s thief (seed 8\'s till its lantern, 2026-11-03: its money no longer reaches a dagger) its lockpicks, paper and charcoal in its pouch (handy), its Club\'s club and its Dagger\'s dagger in the knife\'s place, in its sheath; a yeoman of the shortbows its unit\'s issue, free (a shortbow worn, a quiver of 12 to 24 arrows on the belt, a falchion in its scabbard, a dagger in its sheath, a buckler, a kurbul helm and a leather vest worn); a cleric of Peoni its temple\'s things, free (a robe, a ring, a staff, its letter, the universal kit) and 6 to 36 pennies; the null ruleset\'s the universal kit alone, with some money; the same character in the same game the same outfit; each thing its own (its number, its make)',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cat /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'inventory', 'quit',
        'hmchar -q -s 10 -u feudal -c thief -o /ram/t.hm', 'cat /ram/t.hm', 'echo att end 40 >> /ram/t.hm', 'dq0 -p t -s 3 -c /ram/t.hm -T', 'quit',
        'hmchar -q -s 11 -u feudal -c \'yeoman sb\' -o /ram/y.hm', 'dq0 -p t -s 3 -c /ram/y.hm -T', 'quit',
        'hmchar -q -s 12 -u feudal -c \'cleric (peoni)\' -o /ram/p.hm', 'dq0 -p t -s 3 -c /ram/p.hm -T', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -r null', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], I = DQ0_I;
        const cat = fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0w.hyx.cat'));
        const kinds = new Map();                        // (The catalogue's names and values (farthings, 2026-10-28), as the world has them)
        for (let k = 1; k < 120; k++) { const r = cat.subarray(k * 37, k * 37 + 37), n = r.subarray(0, 14); kinds.set(n.subarray(0, n.indexOf(0) < 0 ? 14 : n.indexOf(0)).toString('latin1'), { value: r.readUInt16LE(33), form: r[36] }); }
        const price = i => { const k = kinds.get(i.name) || { value: 0, form: 0 }; return Math.floor(k.value * (2 + i.quality) / 6) * (k.form & 0x40 ? i.qty : 1); };
        let from = 0;
        const game = (line, what) => { const t = T.of(line, from); from = t.k + 1; const s = t.after; if (!s) f.push(what + ': no state'); return s; };
        const has = (s, what, name, test = () => true) => { const i = dq0Carried(s).find(j => j.name === name && test(j)); if (!i) f.push(what + ': no ' + name + ' as it should be: ' + JSON.stringify(dq0Carried(s).map(j => j.name))); return i; };
        const inside = (s, i) => i ? s.items.find(j => j.n === i.x) : null;
        const kit = (s, what, foot) => {
          has(s, what, 'clothes', i => i.flags & I.WORN); has(s, what, foot, i => i.flags & I.WORN);
          const pack = has(s, what, 'backpack', i => i.flags & I.WORN), belt = has(s, what, 'belt', i => i.flags & I.WORN);
          has(s, what, 'pouch', i => belt && i.x === belt.n);
          const lit = i => (i.flags & (I.HELD | I.LIT)) === (I.HELD | I.LIT), inPack = n => s ? dq0Carried(s).filter(i => i.name === n && pack && i.x === pack.n).length : 0;
          if (s && dq0Carried(s).some(i => i.name === 'lantern' && lit(i))) {   // (Its light by its money, the user's, 2026-11-03: dq0startlight's)
            if (inPack('flask of oil') !== 2) f.push(what + ': a lantern and ' + inPack('flask of oil') + ' flasks of oil, not two');
          } else {
            has(s, what, 'torch', lit);
            if (inPack('torch') !== 2 || inPack('flask of oil') !== 1) f.push(what + ': a torch lit, ' + inPack('torch') + ' spare and ' + inPack('flask of oil') + ' flasks of oil');
          }
          has(s, what, 'tinderbox', i => pack && i.x === pack.n);
          has(s, what, 'rations', i => pack && i.x === pack.n && i.qty === 3);
          const skin = has(s, what, 'waterskin', i => belt && i.x === belt.n);
          has(s, what, 'water', i => skin && i.x === skin.n && i.qty === 4);
          const sheath = has(s, what, 'sheath', i => belt && i.x === belt.n);
          if (!dq0Carried(s).some(i => (i.name === 'knife' || i.name === 'dagger') && sheath && i.x === sheath.n)) f.push(what + ': no knife or dagger in its sheath');
          if (dq0Carried(s).some(i => !i.id)) f.push(what + ': a thing not its own (no number)');
        };
        const wealth = file => { const w = (file.match(/\nwealth (\d+)/) || [])[1]; return w === undefined ? -1 : +w; };
        const story = dq0Prose(out), hm = (k) => story.split('% cat ')[k] || '';
        // seed 5's hunter: a serf, its wealth rolled and kept, the kit bought within it, its Spear's staff in hand
        const w5 = wealth(out.slice(out.indexOf('% cat /ram/c.hm'), out.indexOf('% dq0 -p t -s 3 -c /ram/c.hm -T')).replace(/\r/g, ''));
        if (![40, 80, 160].includes(w5)) f.push('the hunter\'s file: no serf\'s wealth line (' + w5 + ')');
        const s1 = game('dq0 -p t -s 3 -c /ram/c.hm -T', 'the hunter');
        kit(s1, 'the hunter', 'sandals');

        const free = ['clothes', 'sandals', 'shoes', 'backpack', 'belt', 'pouch', 'water', 'pennies', 'letter'];
        const spent = s => dq0Carried(s).filter(i => !free.includes(i.name)).reduce((a, i) => a + price(i), 0);
        const money = s => dq0Carried(s).filter(i => i.name === 'pennies').reduce((a, i) => a + i.qty, 0);
        if (s1 && spent(s1) + 4 * money(s1) > 4 * w5) f.push('the hunter: spent ' + spent(s1) / 4 + 'd and kept ' + money(s1) + ', more than its wealth ' + w5);
        if (!/You carry: \(a\) clothes, worn \(b\) sandals, worn \(c\) a backpack, worn \(\w+\) two torches/.test(story)) f.push('the hunter\'s inventory not its outfit: ' + story.slice(story.indexOf('You carry'), story.indexOf('You carry') + 200));
        // seed 8's thief: its tools and mapping things handy, its club and dagger
        const s2 = game('dq0 -p t -s 3 -c /ram/t.hm -T', 'the thief');
        kit(s2, 'the thief', 'sandals');

        const pouch = s2 && dq0Carried(s2).find(i => i.name === 'pouch');
        for (const n of ['lockpicks', 'paper', 'charcoal']) has(s2, 'the thief', n, i => pouch && i.x === pouch.n);
        has(s2, 'the thief', 'club'); has(s2, 'the thief', 'dagger', i => inside(s2, i) && inside(s2, i).name === 'sheath');
        const w8 = wealth(out.slice(out.indexOf('% cat /ram/t.hm'), out.indexOf('% dq0 -p t -s 3 -c /ram/t.hm -T')).replace(/\r/g, ''));
        if (s2 && spent(s2) + 4 * money(s2) > 4 * w8) f.push('the thief: spent ' + spent(s2) / 4 + 'd and kept ' + money(s2) + ', more than its wealth ' + w8);
        // the yeoman SB: its unit's issue
        const s3 = game('dq0 -p t -s 3 -c /ram/y.hm -T', 'the yeoman');
        has(s3, 'the yeoman', 'shortbow', i => i.flags & I.WORN);
        const quiver = has(s3, 'the yeoman', 'quiver', i => inside(s3, i) && inside(s3, i).name === 'belt');
        has(s3, 'the yeoman', 'arrows', i => quiver && i.x === quiver.n && i.qty >= 12 && i.qty <= 24);
        has(s3, 'the yeoman', 'falchion', i => inside(s3, i) && inside(s3, i).name === 'scabbard');
        has(s3, 'the yeoman', 'dagger', i => inside(s3, i) && inside(s3, i).name === 'sheath');
        for (const n of ['buckler', 'kurbul helm', 'leather vest']) has(s3, 'the yeoman', n, i => i.flags & I.WORN);
        // the cleric of Peoni: the temple's things, 6d6 pennies
        const s4 = game('dq0 -p t -s 3 -c /ram/p.hm -T', 'the cleric');
        kit(s4, 'the cleric', s4 && dq0Carried(s4).some(i => i.name === 'knee boots') ? 'knee boots' : 'shoes');   // (Boots two times in five)
        for (const n of ['robe', 'ring']) has(s4, 'the cleric', n, i => i.flags & I.WORN);
        has(s4, 'the cleric', 'staff', i => i.flags & I.HELD); has(s4, 'the cleric', 'letter');
        if (s4 && (money(s4) > 36)) f.push('the cleric: ' + money(s4) + ' pennies, more than 6d6');
        // the null ruleset: the universal kit only, with some money
        const s5 = game('dq0 -p t -s 3 -c /ram/c.hm -T', 'the null ruleset\'s');
        kit(s5, 'the null ruleset\'s', 'shoes');
        if (s5 && dq0Carried(s5).some(i => i.name === 'staff')) f.push('the null ruleset\'s: its occupation\'s things');
        if (s5 && !money(s5)) f.push('the null ruleset\'s: no money');
        // the hunter again: the same outfit
        const s6 = game('dq0 -p t -s 3 -c /ram/c.hm -T', 'the hunter again');
        const list = s => s ? JSON.stringify(dq0Carried(s).map(i => [i.name, i.qty, i.quality, i.mat, i.flags & (I.HELD | I.WORN | I.LIT)])) : '';
        if (list(s6) !== list(s1)) f.push('the hunter again: not the same outfit: ' + list(s6) + ' against ' + list(s1));
        return f;
      },
      sdcard: true
    },
    {
      // (dq0sky=0, 2026-11-03, a scene's: seed 1's start room's skylight takes the straw's square; the fight's dice other, the body's words "It's battered" among them; no fault)
      name: 'dq0kit', outfit: true, what: 'Dingus Quest Zero\'s denizens\' gear (the outfits\' plan\'s phase 1: games/dq0/outfits.txt\'s [role bandit], DENIZENS.md\'s their things, HOUSE.md\'s the denizens\' gear), by the item store\'s records and the blows told: a lair of hostile bandits (a theme of them, as dq0disarm\'s) placed as the game begins, each bandit\'s things made (items.h\'s IW_DEN, its number its own: the hub\'s actor\'s did), clothes and a belt worn, a weapon held or on its belt (in its carrier: a sword in its scabbard), two bandits\' things not the same, the same seed\'s game the same things (kinds, makes, materials); a bandit drawing its weapon from its belt before it strikes (WB_DRAWS, told: "The bandit draws its hatchet."), striking with it (its words that weapon\'s), its weapon let fall the same record (WB_DROP: on the floor); one killed, its things on its body ("look at the body": what it wore, its belt and its sheath\'s knife on it), its vest taken from the body; the game saved and carried on, each denizen\'s things the same records (their numbers, makes, places)  (Its scene set by a poke since 2026-10-30, dq0Poke\'s: the bandits wary as the first attack is typed, the one down finished, the other not fighting on; since the denizens\' day, 2026-10-31, with no day: the environment\'s dq0hour=128, rc\'s, the bandits about, none asleep or on watch.)',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm',
        'echo ml swd 95 >> /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'mkdir /ram/h',
        'cp /ram/g/theme.txt /ram/h/theme.txt', "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,mail hauberk,great helm'", 'drop the staff',
        'defend dodge', 'wear the mail hauberk', 'wear the great helm', 'hold the sword', 'drop the backpack', 'drop the torch', 'attack', 'attack', 'look at the body',
        'take the leather vest from the body', 'quit', 'dq0 -d /ram/g', 'quit', "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k 'sword,mail hauberk,great helm'", 'quit',
        'echo $status']) },
      start(m) {   // (The scene set, 2026-10-30: as the first attack's typed, the bandits wary (hub.h's AT_WARY), the one down
        // finished, the other no longer fighting on: its body's vest taken as the test means; since the denizens throw (a
        // bandit's spear in hand, its knife kept) the dice had the other knock the character senseless first)
        dq0Poke(m, [0, 1, 2, 3].map(k => ({ g: 1, line: 'attack', n: 1, k, set: { attitude: 2 } })));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out), I = DQ0_I;
        const g1 = T.of("dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,mail hauberk,great helm'"), s1 = g1.after;
        if (!s1) return ['the game not played'];
        // (Each thing's own: by its record, its holder's chain to its top, IW_DEN's its denizen's)
        const kit = s => {
          const by = new Map(s.items.map(i => [i.n, i])), top = i => { for (let d = 0; i && d < 8 && i.where === 'IW_ITEM'; d++) i = by.get(i.x); return i; };
          const r = new Map();
          for (const i of s.items) { const p = top(i); if (p && p.where === 'IW_DEN') { if (!r.has(p.did)) r.set(p.did, []); r.get(p.did).push(i); } }
          return r;
        };
        const k1 = kit(s1), bandits = s1.actors.filter(a => a.name === 'bandit' && a.did);
        if (bandits.length < 2) return [...f, 'not two bandits about as the game began: ' + JSON.stringify(s1.actors)];
        for (const a of bandits) {
          const t = k1.get(a.did) || [], has = (n, fl) => t.some(i => i.name === n && (!fl || (i.flags & fl)));
          if (!has('clothes', I.WORN) || !has('belt', I.WORN)) f.push('bandit ' + a.did + ': no clothes and belt worn (' + t.map(i => i.name).join(', ') + ')');
          if (!t.some(i => /^(sword|shortsword|falchion|club|hatchet|spear|knife|dagger)$/.test(i.name))) f.push('bandit ' + a.did + ': no weapon of its own');
          if (t.some(i => !i.id)) f.push('bandit ' + a.did + ': a thing not made its own (no number)');
        }
        const sig = d => (k1.get(d) || []).map(i => i.name + i.quality + '/' + i.mat).sort().join(',');
        if (sig(bandits[0].did) === sig(bandits[1].did)) f.push('two bandits\' things the same: ' + sig(bandits[0].did));
        const scab = s1.items.find(i => i.name === 'sword' && i.where === 'IW_ITEM' && (s1.items.find(j => j.n === i.x) || {}).name === 'scabbard');
        if (!scab && ![...k1.values()].flat().some(i => i.name === 'shortsword' || i.name === 'falchion')) f.push('no blade in its scabbard on a bandit\'s belt');
        // (Drawn before it strikes, told; its blow by that weapon)
        const all = T.filter(u => u.k > g1.k && u.game === g1.game).flatMap(u => [...u.events, ...u.dens]);
        const draw = all.find(e => e.kind === 'E_BEAT' && e.beat === 'WB_DRAWS' && e.b > 0);
        const kname = k => (s1.items.find(i => i.kind === k) || {}).name;
        if (!draw) f.push('no bandit drew its weapon (WB_DRAWS)');
        else {
          const w = kname(draw.thing);
          inTurn(o, ['The bandit draws their ' + w + '.', 'The bandit strikes at you with their ' + w], f, 'the drawing\'s words: ');
        }
        // (Its weapon let fall: the same record on the floor)
        const drop = all.find(e => e.kind === 'E_BEAT' && e.beat === 'WB_DROP' && e.b > 0);
        if (drop) {
          const u = T.filter(x => x.k > g1.k && x.game === g1.game).find(x => [...x.events, ...x.dens].includes(drop));
          const was = [...k1.values()].flat().filter(i => i.kind === drop.thing).map(i => i.n);
          const now = u && u.after ? u.after.items.filter(i => i.kind === drop.thing && i.where === 'IW_SQUARE').map(i => i.n) : [];
          if (!now.some(n => was.includes(n))) f.push('the weapon let fall not the bandit\'s own (records ' + was.join(' ') + ', on the floor ' + now.join(' ') + ')');
        }
        // (Killed: its things on its body, looked at, its vest taken from it)
        const lk = T.after(g1, 'look at the body'), sb = lk.after;
        const body = sb && sb.items.find(i => i.name === 'body' && sb.items.some(j => j.where === 'IW_ITEM' && j.x === i.n && j.name === 'clothes'));
        if (!body) f.push('no body with its clothes on it');
        if (!/The body is beside you\. On it lie clothes, (?:shoes|knee boots), a belt/.test(o)) f.push('the body\'s things not told as it\'s looked at');
        const tk = T.after(lk, 'take the leather vest from the body');
        if (!tk.after || !dq0Carried(tk.after).some(i => i.name === 'leather vest')) f.push('the leather vest not taken from the body');
        // (Saved and carried on: each denizen's things the same records; the same seed's game the same things)
        const rec = s => JSON.stringify([...kit(s).entries()].sort((a, b) => a[0] - b[0]).map(([d, t]) => [d, t.map(i => [i.n, i.name, i.quality, i.mat, i.id, i.where, i.x]).sort()]));
        const q1 = T.after(tk, 'quit'), g2 = T.after(q1, 'dq0 -d /ram/g');
        if (!q1.at || !g2.after) f.push('the game not saved and carried on');
        else if (rec(q1.at) !== rec(g2.after)) f.push('carried on, the denizens\' things not the same');
        const g3 = T.of("dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k 'sword,mail hauberk,great helm'"), s3 = g3.after;
        const same = s => JSON.stringify([...kit(s).entries()].sort((a, b) => a[0] - b[0]).map(([d, t]) => [d, t.map(i => [i.name, i.quality, i.mat]).sort()]));
        if (!s3) f.push('the game again not played');
        else if (same(s3) !== same(s1)) f.push('the same seed\'s game not the same things');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0keys', what: 'Dingus Quest Zero\'s keys that fit (the user\'s answer 10: "A key opens its own lock, no test"; HOUSE.md\'s keys that fit, the outfits\' phase 2): a key made after a chest locks it and fits it alone (dq0 -k\'s "chest,iron key": every making\'s rule, the denizens\' boxes\' too): the chest put down locked, opened with its key with no test, a few seconds ("You unlock the chest with its key."), closed and locked again with it ("You lock the chest with its key."); its key put down, no picks: nothing to pick it with; a chest whose key was put down and another key kept ("chest,iron key,iron key"): "Your key doesn\'t fit its lock.", the chest locked still',
      init: 't_rc', cycles: 900e6, outfit: true,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'chest,iron key'",
        'drop the chest', 'open the chest', 'unlock the chest', 'open the chest', 'close the chest', 'lock the chest', 'open the chest', 'drop the iron key',
        'unlock the chest', 'quit', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'chest,iron key,iron key'", 'drop the chest', 'drop the iron key', 'unlock the chest', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out);
        const g1 = T.of("dq0 -p t -s 3 -c /ram/c.hm -T -k 'chest,iron key'");
        if (!g1.after) return ['the game not played'];
        const chest = s => s && s.items.find(i => i.name === 'chest'), keys = s => s ? s.items.filter(i => i.name === 'iron key') : [];
        const c0 = chest(g1.after), k0 = keys(g1.after);
        if (!c0 || !(c0.shut & 2) || k0.length !== 1 || k0[0].fits !== c0.id) f.push('the kit\'s chest not locked, its key not fitting it: ' + JSON.stringify([c0, k0]));
        const u = T.of('unlock the chest', g1.k);
        if (!/You unlock the chest with its key\./.test(o)) f.push('not unlocked with its key');
        if (u.replies.some(r => r.req === 'slot0' || r.req === 'slot1')) f.push('a test asked of the referee as its own key opened it');
        if (!u.after || (chest(u.after).shut & 2)) f.push('the chest locked still, unlocked');
        if (!/You open the chest: it's empty\./.test(o)) f.push('the chest not opened');
        const l = T.of('lock the chest', g1.k);
        if (!/You lock the chest with its key\./.test(o) || !l.after || !(chest(l.after).shut & 2)) f.push('not locked again with its key');
        if (!/You have nothing to pick its lock with\./.test(o)) f.push('unlocked with no key and no picks');
        const g2 = T.of("dq0 -p t -s 3 -c /ram/c.hm -T -k 'chest,iron key,iron key'"), u2 = T.of('unlock the chest', g2.k);
        if (!/Your key doesn't fit its lock\./.test(o)) f.push('another key said to fit');
        if (!u2.after || !(chest(u2.after).shut & 2)) f.push('the chest opened by another key');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lair', outfit: true, what: 'Dingus Quest Zero\'s denizens\' belongings and valuables (the outfits\' plan\'s phases 2 and 3: games/dq0/outfits.txt\'s [role bandit] beds, [lair bandit], [purse bandit], DENIZENS.md\'s their things, HOUSE.md\'s the denizens\' belongings; the user\'s answers 6, 9, 10) and what\'s seen of them (the user\'s three levels, 2026-10-27), by the item store\'s records and the words: dq0kit\'s hostile bandits, each one\'s bed (a blanket, lying in its lair\'s home room: its faction\'s), the lair\'s larder (a sack of rations) made as it\'s placed; each one met, its purse (pennies in its pouch); each box at a bed locked, its key in that bandit\'s pouch alone fitting it; a bandit looked at "armed" (the look\'s general words), examined (what it holds and wears), assessed (each one\'s make); one killed, a thing worn beneath its clothes not told as its body\'s looked at, found as it\'s searched; the same seed\'s game the same belongings.  The scene set (since the denizens\' day, 2026-10-31: bandits keep watches, a third asleep): no day (the environment\'s dq0hour=128, rc\'s, before its games: dq0d\'s), every one about as before',
      init: 't_rc', cycles: 1700e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm',
        'echo ml swd 95 >> /ram/c.hm', 'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'mkdir /ram/h',
        'cp /ram/g/theme.txt /ram/h/theme.txt', 'dq0hour=128', "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,mail hauberk,great helm'", 'drop the staff',
        'wear the mail hauberk', 'wear the great helm', 'hold the sword', 'examine the bandit', 'assess the bandit', 'drop the backpack', 'drop the torch', 'attack',
        'attack', 'look at the body', 'search the body', 'quit', "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k 'sword,mail hauberk,great helm'", 'quit',
        'echo $status']) },
      start(m) {   // (The scene set, 2026-11-03, as dq0kit's: as the first attack's typed, the bandits wary: the one down finished, the rest no longer
        // fighting on, the dice had it dead before it looked; and at the look a thing on the body beneath its clothes, I_HIDDEN and not known, a purse's
        // chance one bandit in seven, so the look and the search are run)
        const beneath = (h, L) => {
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), rec = i => [m.dq0.bank(L.item0 + Math.floor(i / per)), (i % per) * L.itemSize];
          const named = i => { const [b, o] = rec(i); let n = ''; for (let j = 0; b && b[o + L.iKind] && j < 14 && cat[b[o + L.iKind] * L.kindSize + j]; j++) n += String.fromCharCode(cat[b[o + L.iKind] * L.kindSize + j]); return n; };
          for (let i = 0; i < L.nitems; i++) if (named(i) === 'body') for (let j = 0; j < L.nitems; j++) {
            const [b, o] = rec(j);
            if (named(j) === 'leather vest' && b[o + L.iWhere] === m.dq0.layout().K.IW_ITEM && (b[o + L.iX] | b[o + L.iX + 1] << 8) === i) b[o + L.iFlags] = b[o + L.iFlags] & ~0x20 | DQ0_I.HIDDEN;
          }
        };
        dq0Poke(m, [...[0, 1, 2, 3, 4, 5].map(k => ({ g: 1, line: 'attack', n: 1, k, set: { attitude: 2 } })), { g: 1, line: 'look at the body', n: 1, call: beneath }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Story(out), I = DQ0_I;
        const g1 = T.of("dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,mail hauberk,great helm'"), s1 = g1.after, q1 = T.of('quit', g1.k);
        if (!s1 || !q1.at) return ['the game not played'];
        const top = (s, i) => { const by = new Map(s.items.map(j => [j.n, j])); for (let d = 0; i && d < 8 && i.where === 'IW_ITEM'; d++) i = by.get(i.x); return i; };
        const s = q1.at, bandits = [...new Set(s.items.filter(i => i.where === 'IW_DEN').map(i => i.did))];
        const beds = s.items.filter(i => i.name === 'blanket' && i.where === 'IW_SQUARE');
        if (beds.length < bandits.length || beds.some(b => !b.owner)) f.push('not a bed each, its faction\'s: ' + beds.length + ' for ' + bandits.length);
        if (beds.length && (Math.max(...beds.map(b => b.x)) - Math.min(...beds.map(b => b.x)) > 24 || Math.max(...beds.map(b => b.y)) - Math.min(...beds.map(b => b.y)) > 24)) f.push('the beds not in one room');
        if (!s.items.some(i => i.name === 'rations' && i.where === 'IW_ITEM' && (top(s, i) || {}).owner && (top(s, i) || {}).where === 'IW_SQUARE')) f.push('no larder');
        const met = s.actors.filter(a => a.name === 'bandit' && a.did).map(a => a.did);
        const purse = d => s.items.some(i => i.name === 'pennies' && i.where === 'IW_ITEM' && (top(s, i) || {}).did === d);
        if (!met.length || !met.some(purse)) f.push('no bandit met with pennies in its pouch: ' + JSON.stringify(met));
        // (Each box and its key as the game begins, every bandit alive: the key in a pouch of one bandit's alone, 2026-11-03)
        const chests = s1.items.filter(i => i.name === 'chest' && i.where === 'IW_SQUARE' && i.owner);
        if (!chests.length) f.push('no box at a bed');
        const holders = new Set();
        for (const c of chests) {
          const k = s1.items.filter(i => i.name === 'iron key' && i.fits === c.id), held = k.length ? s1.items.find(i => i.n === k[0].x) : null, d = k.length ? (top(s1, k[0]) || {}).did : 0;
          if (!(c.shut & 2) || k.length !== 1 || k[0].where !== 'IW_ITEM' || (held || {}).name !== 'pouch' || !d) f.push('chest ' + c.id + ': not locked, or its key not in one pouch of a bandit: ' + JSON.stringify(k));
          else if (holders.has(d)) f.push('chest ' + c.id + ': a second key on bandit ' + d);
          holders.add(d);
        }
        if (!/A bandit, armed(?: and armoured)?,? is /.test(o)) f.push('no bandit looked at armed');
        if (!/The bandit (?:holds|wears) [a-z]/.test(o)) f.push('the bandit examined: what it holds and wears not told');
        if (!/The bandit (?:holds|wears) (?:an? )?(?:crude|poor|plain|sound|good|fine|master's) /.test(o)) f.push('the bandit assessed: no make told');
        const lk = T.of('look at the body', g1.k), sr = T.of('search the body', g1.k), sb = lk.after;
        const body = sb && sb.items.find(i => i.name === 'body' && sb.items.some(j => j.where === 'IW_ITEM' && j.x === i.n && j.name === 'clothes'));
        const hid = body ? sb.items.filter(i => i.where === 'IW_ITEM' && i.x === body.n && (i.flags & I.HIDDEN) && !(i.flags & 0x20)) : [];
        if (!hid.length) f.push('nothing worn beneath its clothes on the body: the scene not set');
        else {
          const w = o.slice(o.indexOf('> look at the body'), o.indexOf('> search the body')), w2 = o.slice(o.indexOf('> search the body'));
          if (new RegExp('On it lie[^.]*\\b' + hid[0].name + '\\b').test(w)) f.push('a thing worn beneath its clothes told at a look: ' + hid[0].name);
          if (!w2.slice(0, 600).includes(hid[0].name)) f.push('a thing worn beneath its clothes not found by searching: ' + hid[0].name);
        }
        const sig = st => JSON.stringify(st.items.filter(i => i.owner && i.where === 'IW_SQUARE').map(i => [i.name, i.x, i.y, i.quality, i.mat]).sort());
        const g2 = T.of("dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/h -k 'sword,mail hauberk,great helm'");
        if (!g2.after) f.push('the game again not played');
        else if (sig(g2.after) !== sig(s1)) f.push('the same seed\'s game not the same belongings');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0items', meaning: true, what: 'Dingus Quest Zero\'s things (items.h, the world\'s item store), by where each one is (the store\'s records) and what the world said to each request (ireq\'s result): the kit a character begins with (dq0 -k pick: a pick more), its torch lit in hand, its pack and belt worn; what it carries listed; a thing examined, one there isn\'t refused; a thing put down (on its square, seen at its feet), taken up again; one out of reach walked over to first (the world\'s I_TAKE); one it hasn\'t refused; a thing hidden where it stands (on its square, hidden, seen so), taken up again (their words: dq0wordsthings)',
      init: 't_rc', cycles: 800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T', 'inventory',
        'examine the dagger', 'examine the moon', 'drop the dagger', 'look', 'get dagger', 'drop pick', 'walk to the door ahead', 'turn around',
        'take the pick', 'drop rocks', 'hide the pouch', 'look', 'take pouch', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], I = DQ0_I, mine = (s, n) => dq0Carried(s).some(i => i.name === n), here = (s, n) => dq0Here(s).find(i => i.name === n);
        const rep = (l, kind, result) => { const t = T.of(l); if (!dq0Rep(t, 'ireq', { kind, result }).length) f.push(l + ': no ' + kind + ' ' + result + ' ' + JSON.stringify(t.replies.filter(r => r.req === 'ireq'))); return t; };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -k pick -T').after;
        for (const n of ['torch', 'backpack', 'rations', 'tinderbox', 'belt', 'pouch', 'pennies', 'dagger', 'waterskin', 'pick'])
          if (!mine(s0, n)) f.push('the kit without its ' + n);
        const torch = dq0Thing(s0, 'torch');
        if (!torch || (torch.flags & (I.LIT | I.HELD)) !== (I.LIT | I.HELD)) f.push('the torch not lit in hand');
        for (const n of ['backpack', 'belt']) { const i = dq0Thing(s0, n); if (!i || !(i.flags & I.WORN)) f.push('the ' + n + ' not worn'); }
        rep('inventory', 'IO_LIST', 'IR_OK');
        rep('examine the dagger', 'IO_LOOK', 'IR_OK');     // (A closer look, no appraisal: 2026-10-22)
        rep('examine the moon', 'IO_LOOK', 'IR_NONE');
        const d = rep('drop the dagger', 'IO_DROP', 'IR_OK');
        if (!here(d.after, 'dagger') || mine(d.after, 'dagger')) f.push('the dagger not put down where it stands');
        const lk = T.after(d, 'look');
        if (!lk.after || !lk.after.sight || !lk.after.sight.items.some(i => i.name === 'dagger' && i.rel === 255)) f.push('the dagger not seen at its feet');
        if (!mine(rep('get dagger', 'IO_GET', 'IR_OK').after, 'dagger')) f.push('the dagger not taken up again');
        const p = rep('drop pick', 'IO_DROP', 'IR_OK'), at = p.after && p.after.pc;
        if (!here(p.after, 'pick')) f.push('the pick not put down');
        const w = T.of('walk to the door ahead');
        dq0Need(f, w, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'not walked to the door');
        if (at && w.after && Math.max(Math.abs(w.after.pc.x - at.x), Math.abs(w.after.pc.y - at.y)) < 2) f.push('the pick not out of reach after the walk');
        const t = rep('take the pick', 'IO_GET', 'IR_NOT_HERE');
        dq0Need(f, t, 'E_TAKEN', { ir: 'IR_OK' }, 'the pick out of reach not walked over to and taken');
        if (!mine(t.after, 'pick') || !at || t.after.pc.x !== at.x || t.after.pc.y !== at.y) f.push('not at the pick\'s square with it');
        const r = rep('drop rocks', 'IO_DROP', 'IR_NONE');
        if (r.at && r.after && dq0Carried(r.at).length !== dq0Carried(r.after).length) f.push('something put down for rocks it hasn\'t');
        const h = rep('hide the pouch', 'IO_HIDE', 'IR_OK'), hp = here(h.after, 'pouch');
        if (!hp || !(hp.flags & I.HIDDEN) || mine(h.after, 'pouch')) f.push('the pouch not hidden where it stands');
        const lk2 = T.after(h, 'look');
        if (!lk2.after || !lk2.after.sight || !lk2.after.sight.items.some(i => i.name === 'pouch' && (i.hid & 1))) f.push('the pouch not seen hidden at its feet');
        const tp = rep('take pouch', 'IO_GET', 'IR_OK'), pp = dq0Thing(tp.after, 'pouch');
        if (!mine(tp.after, 'pouch') || (pp.flags & I.HIDDEN)) f.push('the pouch not taken up again');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0equip', meaning: true, what: 'Dingus Quest Zero\'s things handled (the world\'s item store, dq0w/things.c), by where each one is and its state (held, worn, lit: the store\'s records) and what the world said to each (ireq\'s result: done or refused): a kit of things more (dq0 -k, commas between: a table held in both hands, the rest in the pack); a table put down and a candle put on it, taken from it again; a cloak worn and taken off; a basket held that won\'t keep sand, a waterskin that won\'t take a dagger, a quiver hung on the belt and arrows put in it, a candle that won\'t hang, a dagger not taken in hand with the hands full, the pack not into the basket that\'s in it; then down to a dark place: the torch put out (dark), a candle lit (out of the pack into a hand: light), the torch lit, put down lit (its light lies about it, the room still seen with the candle out), taken up again (their words: dq0wordsthings)',
      init: 't_rc', cycles: 1200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -k table,basket,sand,cloak,candle,quiver,arrows',
        'drop table', 'put the candle on the table', 'look', 'take the candle from the table', 'stow the candle', 'b', 'wear cloak', 'hold basket', 'put sand in basket',
        'put the dagger in the waterskin', 'hang the quiver on my belt', 'put arrows in quiver', 'hang the candle', 'wield the dagger', 'stow basket',
        'put the backpack in the basket', 'take the cloak off', 'stow cloak', 'd', 'douse the torch', 'light the candle', 'light torch',
        'drop the torch', 'douse candle', 'look', 'get torch', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [], I = DQ0_I;
        const put = (l, kind, result) => { const t = T.of(l); if (!dq0Rep(t, 'ireq', { kind, result }).length) f.push(l + ': no ' + kind + ' ' + result + ' ' + JSON.stringify(t.replies.filter(r => r.req === 'ireq').map(r => r.kind + ' ' + r.result))); return t; };
        const thing = (t, n) => dq0Thing(t.after, n), holder = (t, n) => { const h = dq0Holder(t.after, thing(t, n)); return h ? h.name : ''; };
        const flags = (t, n, on, off, what) => { const i = thing(t, n); if (!i || (i.flags & on) !== on || (i.flags & off)) f.push(t.line + ': the ' + n + ' not ' + what + ' (flags ' + (i && i.flags) + ')'); };
        const s0 = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k table,basket,sand,cloak,candle,quiver,arrows');
        flags(s0, 'table', I.HELD, 0, 'held');
        for (const n of ['basket', 'heap of sand', 'cloak', 'candle', 'quiver', 'arrows']) if (holder(s0, n) !== 'backpack') f.push('the kit\'s ' + n + ' not in the pack');
        const dt = put('drop table', 'IO_DROP', 'IR_OK');
        if (!dq0Here(dt.after).some(i => i.name === 'table')) f.push('the table not put down');
        if (holder(put('put the candle on the table', 'IO_PUT', 'IR_OK'), 'candle') !== 'table') f.push('the candle not on the table');
        if (holder(put('take the candle from the table', 'IO_TAKE', 'IR_OK'), 'candle') === 'table') f.push('the candle not taken from the table');
        put('stow the candle', 'IO_STOW', 'IR_WHICH');   // (The pack or the pouch: asked where, the designators' step 7, its answer the pack)
        if (holder(put('b', 'IO_PUT', 'IR_OK'), 'candle') !== 'backpack') f.push('the candle not stowed in the pack');
        flags(put('wear cloak', 'IO_DON', 'IR_OK'), 'cloak', I.WORN, 0, 'worn');
        flags(put('hold basket', 'IO_HOLD', 'IR_OK'), 'basket', I.HELD, 0, 'held');
        if (holder(put('put sand in basket', 'IO_PUT', 'IR_NO'), 'heap of sand') !== 'backpack') f.push('the sand moved into the basket');
        if (holder(put('put the dagger in the waterskin', 'IO_PUT', 'IR_NO'), 'dagger') === 'waterskin') f.push('the dagger in the waterskin');
        if (holder(put('hang the quiver on my belt', 'IO_PUT', 'IR_OK'), 'quiver') !== 'belt') f.push('the quiver not hung on the belt');
        if (holder(put('put arrows in quiver', 'IO_PUT', 'IR_OK'), 'arrows') !== 'quiver') f.push('the arrows not in the quiver');
        if (holder(put('hang the candle', 'IO_PUT', 'IR_NO'), 'candle') !== 'backpack') f.push('the candle moved, hung');
        flags(put('wield the dagger', 'IO_HOLD', 'IR_NO'), 'dagger', 0, I.HELD, 'left alone (the hands full)');
        if (holder(put('stow basket', 'IO_STOW', 'IR_OK'), 'basket') !== 'backpack') f.push('the basket not stowed');
        flags(put('put the backpack in the basket', 'IO_PUT', 'IR_NO'), 'backpack', I.WORN, 0, 'still worn');
        flags(put('take the cloak off', 'IO_DOFF', 'IR_OK'), 'cloak', I.HELD, I.WORN, 'taken off into a hand');
        if (holder(put('stow cloak', 'IO_STOW', 'IR_OK'), 'cloak') !== 'backpack') f.push('the cloak not stowed');
        dq0Need(f, T.of('d'), 'E_ARRIVE', { how: 'A_DOWN' }, 'not down the stairway');
        const dark = (t, d, what) => { if (!t.after || !t.after.sight || t.after.sight.dark !== d) f.push(t.line + ': ' + what); };
        const dz = put('douse the torch', 'IO_DOUSE', 'IR_OK');
        flags(dz, 'torch', 0, I.LIT, 'put out');
        dark(dz, 1, 'not dark');
        const lc = put('light the candle', 'IO_LIGHT', 'IR_OK');
        flags(lc, 'candle', I.LIT | I.HELD, 0, 'lit in a hand');
        dark(lc, 0, 'dark still, the candle lit');
        flags(put('light torch', 'IO_LIGHT', 'IR_OK'), 'torch', I.LIT, 0, 'lit');
        const dr = put('drop the torch', 'IO_DROP', 'IR_OK'), tr = dq0Here(dr.after).find(i => i.name === 'torch');
        if (!tr || !(tr.flags & I.LIT)) f.push('the torch not put down lit');
        flags(put('douse candle', 'IO_DOUSE', 'IR_OK'), 'candle', 0, I.LIT, 'put out');
        dark(T.after(dr, 'look'), 0, 'dark, the torch lit on the floor');
        flags(put('get torch', 'IO_GET', 'IR_OK'), 'torch', I.LIT, 0, 'taken up lit');
        if (!dq0Carried(T.of('get torch').after).some(i => i.name === 'torch')) f.push('the torch not carried again');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0reach', meaning: true, what: 'Dingus Quest Zero\'s things out of reach walked to first, places in a room, a blow\'s close-in and engagement (slice 4.0; HOUSE.md: out of reach, walked to first, going places, engagement; the user\'s, 2026-10-20), by the hub\'s events, the world\'s replies and where the character ends (the referee\'s dice at these seeds): at seed 3 the pick put down and walked away from, then "stow the pick" walks over to it (the things\' IR_NOT_HERE, I_TAKE\'s walk alone) and stows it as it gets there (the command carried out again: IR_OK); the room\'s centre, its far left corner, its near side and, walking forward, its other side gone to by the facing (I_GOTO: each square the room\'s own); at seed 7 the ghoul down the stairway attacked from two squares off: closed with (I_CLOSE: a walk), its blow back as the character comes beside it not a stop, and the character\'s blow struck; in another game (no sword: its torch to hand) the walk toward the far side stopped as it comes into the ghoul\'s zone (TE_ENGAGED), the ghoul fought back (HOUSE.md\'s fighting on), and the walk again, typed as it fights, taking the fight over: begun in the zone, after a square (TE_ENGAGED, engaged still) (since 2026-10-29: the ghoul made aware of the character as the walk past it, two squares off, is typed: dq0Aware, the test\'s own setting up, an unaware denizen engaging no one; its zone stops the walk into it, the walk begun engaged a square on)',
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k pick -T',
        'drop pick', 'walk to the door ahead', 'turn around', 'stow the pick', 'go to the centre of the room', 'go to the far left corner',
        'face north', 'go to the near side', 'face north', 'walk forward to the other side', 'quit', 'echo ml swd 80 >> /ram/c.hm',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'attack the ghoul', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'go to the far side']) +
        '\u0100go to the far side\r' + dq0Typed(['quit', 'echo $status']) },
      start(m) { dq0Aware(m, 'go to the far side', 'ghoul', 'attack the ghoul'); },   // (Its ghoul aware of the character as the walk past it begins: slice 4.5's, an unaware one engages no one)
      expect: ['% echo $status'],
      check(m) {
        const T = dq0Turns(m), f = [];
        const st = T.of('stow the pick');
        if (!dq0Rep(st, 'ireq', { result: 'IR_NOT_HERE' }).length || !dq0Rep(st, 'ireq', { result: 'IR_OK' }).length) f.push('the pick not walked over to and stowed (' + st.replies.map(r => r.req + ' ' + (r.result || '')).join(', ') + ')');
        dq0Need(f, st, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'no walk over to the pick');
        const pk = dq0Thing(st.after, 'pick');
        if (!pk || pk.where !== 'IW_ITEM') f.push('the pick not in the pack: ' + (pk && pk.where));
        // (The places: the room's floor rx, ry, rw by rh, the character facing north as it ends each)
        const at = (t, what, want) => {
          const s = t.after, r = s && s.sight;
          if (!r || r.where !== 0) { f.push(t.line + ': not in a room'); return; }
          const w = want(r, t.at ? t.at.pc.x : r.rx);   // (A side's square in the column it set out from)
          const on = u => u && u.actors.some(x => x.state && x.sn === u.pc.sn && x.x === w[0] && x.y === w[1]);   // (A denizen on it: beside it)
          if ((s.pc.x !== w[0] || s.pc.y !== w[1]) && !((on (t.at) || on (s)) && Math.abs(s.pc.x - w[0]) <= 1 && Math.abs(s.pc.y - w[1]) <= 1))
            f.push(t.line + ': at ' + s.pc.x + ',' + s.pc.y + ', not ' + what + ' ' + w.join(',') + ' (' + dq0Evs(t) + ')');
        };
        at(T.of('go to the centre of the room'), 'the centre', r => [r.rx + (r.rw - 1 >> 1), r.ry + (r.rh - 1 >> 1)]);
        at(T.of('go to the far left corner'), 'the far left corner', r => [r.rx, r.ry]);
        at(T.of('go to the near side'), 'the near side', (r, x) => [x, r.ry + r.rh - 1]);
        at(T.of('walk forward to the other side'), 'the other side', (r, x) => [x, r.ry]);
        const a = T.of('attack the ghoul'), gh = a.at && a.at.actors.find(x => x.name === 'ghoul');
        if (!gh) return [...f, 'no ghoul'];
        dq0Need(f, a, 'E_ARRIVE', { how: 'A_WENT_TO' }, 'the ghoul not closed with');
        if (!dq0Ev(a, 'E_FIGHT', { who: 0, def: gh.k + 1 }).length) f.push('no blow struck at the ghoul as the character came up to it (' + dq0Evs(a) + ')');
        const g = T.filter(t => t.line === 'go to the far side');
        if (g.length !== 2) return [...f, 'the walks not typed'];
        if (!dq0Ev(g[0], 'E_TASK', { te: 'TE_ENGAGED', still: false }).length) f.push('the walk not stopped by the ghoul\'s zone (' + dq0Evs(g[0]) + ')');
        // (The second walk typed as the ghoul's fought: the two turns' events as one, the fight then taken over)
        const te = [...g[0].events, ...g[1].events].filter(e => e.kind === 'E_TASK');
        const fi = te.findIndex(e => e.te === 'TE_FIGHT'), si = te.findIndex((e, i) => i > fi && e.te === 'TE_STOPPED' && e.task === 'LT_FIGHT');
        if (fi < 0 || si < 0) f.push('the ghoul not fought back, the walk again taking it over (' + dq0Evs(g[1]) + ')');
        else if (!te.slice(si).some(e => e.te === 'TE_ENGAGED' && e.still)) f.push('the walk begun engaged not stopped after a square (' + dq0Evs(g[1]) + ')');
        const p0 = g[0].after && g[0].after.pc, p1 = g[1].after && g[1].after.pc;
        if (p0 && p1 && Math.max(Math.abs(p1.x - p0.x), Math.abs(p1.y - p0.y)) > 1) f.push('more than a square walked engaged');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0toolnamed', meaning: true, near: ['gear'], what: "Dingus Quest Zero's tool named (the user's, 2026-11-03: \"Suggest the better tool if one is available, but allow them to continue with the less effective tool.\": HOUSE.md's digging's a tool named): at seed 1, a shovel and a knife carried, 'dig down with the knife': the better told of, once (E_DIG's D_BETTER, not each spell), the knife digs; none named, the shovel; at seed 374's masonry, a pick carried: the knife refused, told the pick would, no time; sharpening or a lock with the wrong thing named, refused, not passed over for the right one",
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 1 -c /ram/c.hm -T -k shovel,knife', 'dig down with the knife', 'dig down', 'quit',
        'dq0 -p t -s 374 -c /ram/c.hm -T -k pick,knife,whetstone', 'dig north with the knife', 'sharpen the knife with the torch', 'unlock the door with the knife', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'dig down with the knife', n: 1, words: 'You dig with your', text: 'stop\r' }, { g: 1, line: 'dig down', n: 1, words: 'You dig with your', text: 'stop\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), kn = T.of('dig down with the knife');
        const better = T.filter(t => dq0Ev(t, 'E_DIG', { how: 'D_BETTER' }).length);
        if (better.length !== 1 || better[0].k !== kn.k) f.push('the better tool not told once, as the dig began: ' + better.map(t => t.line));
        if (!/Your shovel would do this better, but you set to it with the knife as you said\./.test(s)) f.push('not told of the shovel');
        if (!/You dig with your knife\./.test(s)) f.push('the knife not used');
        if (!/You dig with your shovel\./.test(s)) f.push('none named, the shovel not taken');
        if (!/Your knife won't dig masonry\. Your pick would\./.test(s)) f.push('the knife on masonry not refused, the pick told of');
        const r = T.of('dig north with the knife');
        if (r.after && r.at && r.after.now !== r.at.now) f.push('the refusal took time: ' + (r.after.now - r.at.now));
        if (!/You have nothing to sharpen the knife with\./.test(s)) f.push('sharpening with a torch named not refused');
        if (!/Your knife won't do for a lock\./.test(s)) f.push('a lock with a knife named not refused');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0judge', meaning: true, near: ['gear'], what: "Dingus Quest Zero's appraisals kept (the user's, 2026-11-03: the first answer per thing till the skill improves: HOUSE.md's appraising a thing): asked again, the same words, a thing's value, its make, the whole, a door's quality, no store: the die a hash of the thing, the skill's mastery and the seed; the skill raised (the character's file), another answer for one thing at least; the game's other dice not moved by asking",
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/d.hm', 'echo ml wpn 50 >> /ram/c.hm', 'echo ml wpn 95 >> /ram/d.hm',
        ...[['c', 'value the dagger', 'value the dagger', 'judge the dagger', 'judge the dagger', 'appraise the dagger', 'appraise the dagger', 'value the shield', 'value the shield', 'go to the door', 'judge the door', 'judge the door'],
          ['d', 'value the dagger', 'judge the dagger', 'value the shield']].flatMap(([c, ...l]) => ['dq0 -p t -s 3 -c /ram/' + c + '.hm -T -k \'dagger,round shield\'', ...l, 'quit']), 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out), said = l => [...s.matchAll(new RegExp('> ' + l + ' > ' + l + ' (.*?) (?=Something stirs|Nothing stirs|>)', 'g'))].map(x => x[1]);
        for (const l of ['value the dagger', 'judge the dagger', 'appraise the dagger', 'value the shield', 'judge the door']) {
          const a = said(l);
          if (a.length < 2) f.push(l + ': not asked twice: ' + a.length);
          else if (a[0] !== a[1]) f.push(l + ': another answer asked again: ' + a[0] + ' / ' + a[1]);
        }
        if (!process.env.RULES && ['value the dagger', 'judge the dagger', 'value the shield'].every(l => said(l)[2] === said(l)[0])) f.push('the skill raised, the same answer for all three: ' + said('value the dagger'));
        this.notes = [JSON.stringify(said('value the dagger')), JSON.stringify(said('judge the dagger')), JSON.stringify(said('value the shield'))];
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0farthing', outfit: true, near: ['money', 'denizens'], what: "Dingus Quest Zero's prices under a penny and no money pump (the user's, 2026-11-03: \"Farthing goods\", \"Haggling pump\": HOUSE.md's the market): a hand-written theme's bandit (dq0trade's scene): chalk (a farthing's worth) offered for and priced in farthings, its farthing paid, a character with none a whole penny; and an ally, Rhetoric 95, haggled with both ways over and over: buying back never under what it paid, the character's purse never the fuller for it",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml rhe 95 >> /ram/c.hm', 'echo wealth 1600 >> /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k chalk,chalk,farthings,farthings",
        'inventory', 'name the bandit bob', 'give bob 40 pennies', 'give bob 1 farthings', 'what will you give for the chalk', 'sell the chalk to bob', 'how much for the chalk', 'buy the chalk from bob', 'sell the chalk to bob', 'give the farthings to bob', 'give the farthings to bob', 'give the farthings to bob', 'buy the chalk from bob', 'inventory',
        ...Array(5).fill(['haggle over my rope', 'inventory', 'haggle for the rope', 'inventory']).flat(), 'quit', 'echo $status']) },
      start(m) {
        const lines = ['give the farthings to bob', 'give bob 40 pennies', 'give bob 1 farthings', 'what will you give for the chalk', 'sell the chalk to bob', 'how much for the chalk', 'buy the chalk from bob', 'give the farthings to bob', 'haggle over my rope', 'haggle for the rope'];
        const p = ['inventory', 'name the bandit bob'].map(line => ({ g: 1, line, n: 1, name: 'bandit', set: { attitude: 4, doing: 0 }, beside: 4 }));
        for (const line of lines) for (let n = 1; n <= 5; n++) p.push({ g: 1, line, n, name: 'Bob', set: { attitude: line[0] === 'h' ? 5 : 4, doing: 0 }, beside: 4 });
        dq0Poke(m, p);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const pennies = st => dq0Carried(st).filter(i => i.name === 'pennies').reduce((n, i) => n + i.qty, 0);
        if (!/would give you 1 farthing for your chalk\./.test(s)) f.push('the chalk not offered for in farthings');
        if (!/Bob asks 1 farthing for the chalk\./.test(s)) f.push('the chalk not priced in farthings');
        if (!/agrees: 1 farthing for your chalk\./.test(s)) f.push('the chalk not sold for a farthing');
        if (!/agrees: 1 farthing for the chalk\./.test(s)) f.push('the chalk not bought for a farthing');
        if (!/agrees: 1 penny for the chalk\./.test(s)) f.push('no farthings, not a whole penny');
        const sells = T.filter(t => t.line === 'haggle over my rope'), buys = T.filter(t => t.line === 'haggle for the rope'), got = /agrees: (\d+) penn/;
        const start = T.of('haggle over my rope').at, end = T.of('quit').at;
        this.notes = [(s.match(/agrees: \d+ penn[a-z]* for [a-z]+ rope/g) || []).join('; ')];
        if (start && end && pennies(end) > pennies(start)) f.push('money made by haggling: ' + pennies(start) + ' then ' + pennies(end));
        const sold = [...s.matchAll(/agrees: (\d+) penn[a-z]* for your rope/g)].map(x => +x[1]), bought = [...s.matchAll(/agrees: (\d+) penn[a-z]* for the rope/g)].map(x => +x[1]);
        if (!sold.length || !bought.length) f.push('no trades both ways: ' + sold + ' / ' + bought);
        else if (Math.min(...bought) < Math.max(...sold)) f.push('bought under what it sold for: ' + bought + ' / ' + sold);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0whotrades', outfit: true, near: ['money', 'denizens'], what: "Dingus Quest Zero's who trades (the user's, 2026-11-03: \"Friendly folk only\": HOUSE.md's the market): a hand-written theme's bandit (dq0trade's scene): indifferent, its price, its offer, a sale and a purchase all refused in words (SR_WONT: 'won't trade with you'), nothing changing hands; friendly, the same asked goes through; an antidote carried by the same rule",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo wealth 6000 >> /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k rope,antidote',
        'inventory', 'how much for the rope', 'what will you give for the rope', 'sell the rope to the bandit', 'sell the antidote to the bandit', 'inventory', 'look', 'what will you give for the rope', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, ['inventory', 'how much for the rope', 'what will you give for the rope', 'sell the rope to the bandit', 'sell the antidote to the bandit'].map(line => ({ g: 1, line, n: 1, name: 'bandit', set: { attitude: 3, doing: 0 }, beside: 4 }))
          .concat([{ g: 1, line: 'what will you give for the rope', n: 2, name: 'bandit', set: { attitude: 4, doing: 0 }, beside: 4 }]));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const wont = T.filter(t => /^(how much|what will|sell)/.test(t.line)).map(t => t.replies.filter(r => r.req === 'sway').map(r => r.result).join());
        this.notes = [wont.join(' ')];
        if (wont.slice(0, 4).join() !== 'SR_WONT,SR_WONT,SR_WONT,SR_WONT') f.push('indifferent, not refused every way: ' + wont.join(' '));
        if ((s.match(/The bandit won't trade with you\./g) || []).length !== 4) f.push('its refusal not told each time');
        const inv = T.filter(t => t.line === 'inventory');
        if (inv.length < 2 || !dq0Carried(inv[1].after).some(i => i.name === 'rope') || !dq0Carried(inv[1].after).some(i => i.name === 'antidote')) f.push('things gone though it refused');
        if (wont[4] !== 'SR_OK') f.push('friendly, no offer for the rope: ' + wont[4]);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0strip', outfit: true, near: ['gear', 'denizens'], what: "Dingus Quest Zero's armour off a body (the user's, 2026-11-03: \"Twice taking-off time\": HOUSE.md's dead, its things on its body): dq0kit's bandit killed and looked at; a leather vest (worn) off its body takes ten seconds more than a thing not worn off it, a belt's pouch",
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'dq0sky=0', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm',
        'echo ml swd 95 >> /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt",
        "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,mail hauberk,great helm'", 'drop the staff',
        'wear the mail hauberk', 'wear the great helm', 'hold the sword', 'drop the backpack', 'drop the torch', 'defend dodge', 'attack', 'attack', 'look at the body',
        'take the leather vest from the body', 'take the belt from the body', 'quit', 'echo $status']) },
      start(m) {
        // (The scene set, 2026-10-30: as the first attack's typed, the bandits wary (hub.h's AT_WARY), the one down
        // finished, the other no longer fighting on: its body's vest taken as the test means; since the denizens throw (a
        // bandit's spear in hand, its knife kept) the dice had the other knock the character senseless first)
        dq0Poke(m, [0, 1, 2, 3].map(k => ({ g: 1, line: 'attack', n: 1, k, set: { attitude: 2 } })));
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out);
        const tk = T.of('take the leather vest from the body');
        if (!tk || !tk.after || !dq0Carried(tk.after).some(i => i.name === 'leather vest')) return ['the vest not taken from the body (the scene?): ' + o.slice(-600)];
        const t0 = tk.at.now, t1 = tk.after.now;
        this.notes = ['vest ' + (t1 - t0)];
        if (t1 - t0 < 100) f.push('the vest off a body in under ten seconds: ' + (t1 - t0));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0busyhands', reach: true, outfit: true, near: ['gear'], what: "Dingus Quest Zero's readying with busy hands (the user's, 2026-11-03: \"it should probably put the other things away first (in their appropriate place) then take the tools in hand\": HOUSE.md's every use of a tool, A24, A25, A26): a sword and a shield in hand, a pick to dig with: the sword into its scabbard, the shield slung, each told as a step, the pick got out (truly in hand, both), used; stopped as it digs, the pick stays in hand, told, and a later use finds it there, not got out again; a hammer and chisel: both got out with no telling to hold them; a lit torch and the sword in hand and the pick wanted, both hands: the sword put in its scabbard first, the torch laid on the ground still lit, last (the user's, 2026-11-17: the hand whose thing has a home first, the light last), the look says so, taken up lit; a tinderbox with both hands full: a free hand made first, the sword's",
      init: 't_rc', cycles: 3500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo ml min 40 >> /ram/c.hm', 'dq0sky=0', 'dq0hour=128',
        "dq0 -p t -s 374 -c /ram/c.hm -T -k 'scabbard,pick,hammer,chisel,sword,round shield'", 'hang the scabbard on the belt', 'drop 3 rations', 'drop the flask', 'drop three candles', 'drop the sack', 'drop the whetstone', 'put the chisel in the backpack', 'put the hammer in the backpack', 'put the pick in the backpack',
        'drop the round shield', 'drop the sword', 'drop the torch', 'take the sword', 'take the round shield', 'hold the sword', 'inventory', 'dig north', 'stop', 'inventory', 'dig north', 'stop', 'inventory', 'put on the backpack', 'put the pick in the backpack', 'take the sword', 'hold the round shield', 'inventory',
        'dig north with the chisel', 'stop', 'inventory', 'put on the backpack', 'put the hammer in the backpack', 'put the chisel in the backpack', 'hold the sword', 'take the torch', 'hold the pick', 'look', 'inventory', 'drop the pick', 'take the torch', 'hold the sword', 'put out the torch', 'light the torch', 'inventory', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'dig north', n: 1, words: 'You dig', text: 'stop\r' }, { g: 1, line: 'dig north', n: 2, words: 'You dig', text: 'stop\r' }, { g: 1, line: 'dig north with the chisel', n: 1, words: 'You dig', text: 'stop\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ').replace(/ # /g, ' ').replace(/ UP( \d+)?(?= )/g, '');   // (The status line's pieces, as it's drawn between a task's steps, out of the story)
        this.notes = [s.slice(s.indexOf('> hold the sword')).slice(0, 1200), s.slice(-1200)];
        const after = (w, re, what) => { const i = s.indexOf(w); if (i < 0 || !re.test(s.slice(i))) f.push(what + ': ' + s.slice(i, i + 500)); };
        after('> dig north', /^> dig north( > dig north)? You look at the wall\. You put the sword in the scabbard\. You sling the round shield\. You take off the backpack, and set it down at your feet\. You reach into the backpack\. You get out the pick\./, 'not its hands freed, each told, then the pick got out');
        after('> stop', /You stop digging\. Your backpack lies at your feet\. Your pick is in your hand\./, 'stopped: not told in hand');
        const inv = s.indexOf('You stop digging'), i2 = s.indexOf('> inventory', inv);
        if (!/a pick, in both hands/.test(s.slice(i2, i2 + 400))) f.push('stopped: the pick not truly in its hands');
        const d2 = s.indexOf('> dig north', i2), e2 = s.indexOf('> stop', d2);
        if (/You get out the pick/.test(s.slice(d2, e2))) f.push('a later use got the pick out again');
        const c0 = s.indexOf('> dig north with the chisel'), c1 = s.indexOf('> stop', c0);
        if (!/You get out the chisel\..*You get out the hammer\./.test(s.slice(c0, c1))) f.push('hammer and chisel not both got out: ' + s.slice(c0, c0 + 600));
        if ((s.slice(c0, c1).match(/Your pick would do this better/g) || []).length !== 1) f.push('the better tool not told once');
        if (!/a hammer, in your left hand \(\w+\) a chisel, in your right hand/.test(s.slice(c1, c1 + 500))) f.push('stopped: hammer and chisel not in hand');
        after('> hold the pick', /^> hold the pick( > hold the pick)? [^>]*You put the sword in the scabbard\.[^>]*You lay the torch down too, still lit\./, 'the sword not sheathed first, the lit torch not laid down last');
        after('> look', /At your feet lie a lit torch/, 'the torch laid not told lit');
        const l0 = s.indexOf('> light the torch');
        if (!/You get out the tinderbox\./.test(s.slice(l0)) || !/It catches\./.test(s.slice(l0))) f.push('no tinderbox\'s steps');
        if (!/You put the sword in the scabbard\./.test(s.slice(l0))) f.push('striking a light: no hand freed first: ' + s.slice(l0, l0 + 600));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0busyloop', reach: true, near: ['gear'], what: "Dingus Quest Zero's hands made free, in the states the soak found going round (2026-11-04: a thing being readied never what's put away; HOUSE.md's readying with busy hands): at seed 374 a torch to light in one hand, a spear in the other: the spear put away, never the torch, the tinderbox got out once, lit, put away; the torch in the pack, both hands full: both ways the tinderbox stays till it's lit and the torch comes out, each told once; a pick wanted in the pack, a sword in one hand and no scabbard: the pack off into the other hand is set down, not slung to come off again; a spare torch in the other hand dipping: that one lit, no other got out; two torches lying lit going out in one long rest told once",
      init: 't_rc', cycles: 4500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        "dq0 -p t -s 374 -c /ram/c.hm -T -k spear", 'put out the torch', 'inventory', 'light the torch', 'inventory', 'quit',
        "dq0 -p t -s 374 -c /ram/c.hm -T -k spear,sword", 'put out the torch', 'stow the torch', 'inventory', 'hold the sword', 'inventory', 'light the torch', 'inventory', 'quit',
        "dq0 -p t -s 374 -c /ram/c.hm -T -k pick,sword", 'put the pick in the backpack', 'drop the torch', 'hold the sword', 'inventory', 'dig north', 'stop', 'inventory', 'quit',
        "dq0 -p t -s 3 -c /ram/c.hm -T", 'hold another torch', 'inventory', 'wait 20 minutes', 'dip the torch', 'inventory', 'quit',
        "dq0 -p t -s 3 -c /ram/c.hm -T -k lantern", 'hold another torch', 'light the torch', 'drop the torch', 'drop the torch', 'rest for 2 hours', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 3, line: 'dig north', n: 1, words: 'You dig', text: 'stop\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], all = dq0Story(out).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        const games = all.split(/% dq0 -p t /).slice(1), n = (g, re) => (g.match(re) || []).length;
        this.notes = games.map(g => g.slice(g.indexOf('> '), g.indexOf('> ') + 1100));
        if (games.length !== 5) return ['not five games: ' + games.length];
        const [a, b, c, d, e] = games;
        if (n(a, /You get out the tinderbox\./g) !== 1 || n(a, /put away the tinderbox/g) !== 1) f.push('the tinderbox not out once and back once: ' + a.slice(-900));
        if (/You stow the torch in your backpack\./.test(a)) f.push('the torch to light put away for its tinderbox');
        if (!/It catches\./.test(a) || !/You light the torch\./.test(a)) f.push('not lit');
        if (n(b, /You get out the tinderbox\./g) !== 1) f.push('hands full, the torch in the pack: the tinderbox out more than once: ' + n(b, /You get out the tinderbox\./g));
        if (!/You light the torch\./.test(b)) f.push('hands full, the torch in the pack: not lit: ' + b.slice(-900));
        if (/You sling the backpack/.test(c)) f.push('the pack slung to come off again');
        if (!/You get out the pick\./.test(c)) f.push('the pick not got out: ' + c.slice(-900));
        if (/You get out the new torch/.test(d)) f.push('a spare got out, one in the other hand: ' + d.slice(-900));
        if (n(d, /You set the new torch down at your feet\./g) > 1) f.push('set down twice');
        if (n(e, /The torch gutters and goes out/g) > 1) f.push('two going out in one answer told twice');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0twoalike', reach: true, near: ['gear'], what: "Dingus Quest Zero's two things alike set down for a dipping (the life soak's seed 3, 2026-11-04: \"You set the new torch down at your feet.\" and \"You take up the new torch.\" each twice in one answer, two things, the same words): a torch in each hand and a third burning on the floor, dipped: the second told as the other, each told once (HOUSE.md's reach)",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k torch', 'wait 20 minutes', 'drop the lit torch', 'hold another torch', 'light the torch', 'light the torch', 'inventory', 'dip the torch in the oil', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), a = s.slice(s.indexOf('> dip the torch'));
        this.notes = [a.slice(0, 700)];
        inTurn(a, ['You set the new torch down at your feet.', 'You set the other torch down.', 'You take up the new torch.', 'You take up the other torch.'], f, 'the dipping: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0torchwords', reach: true, near: ['gear'], what: "Dingus Quest Zero's torches by the user's playtest of 2026-11-06 (\"You reach for the torch.\" then \"Put down.\"; \"drop the spent torch\" dropped the lit one; \"You put the torch in your backpack.\" of the spent one; \"You set to filling the torch.\" of a dip; \"You put away the tinderbox.\" of one in hand; a torch taken up went in the pack; \"You see no floor here\"): the first word says what happens (put down; your backpack reached for; looked for, none), a thing named by its state beside another of its kind, spent chooses, a dip's own word, a tool in hand before stays in hand, a torch taken up goes in a free hand and in the pack only with both full (said so), the floor and the ground a place to take from",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', 'dq0 -p t -s 3 -c /ram/c.hm -T -k torch', ...TORCHX, 'quit',
        ...dq0Paces('dq0 -p t -s 3 -c /ram/c.hm -T -k torch', TORCHX), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [3]); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of TORCHX) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;   // (The line as typed, then drawn again)
          const e = o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        const said = (k, re, why) => { const n = TORCHX.indexOf(k, typeof why === 'number' ? why : 0); if (!re.test(seg[n])) f.push(k + ': ' + (typeof why === 'string' ? why : re) + ': ' + seg[n].slice(0, 300)); };
        const not = (k, re, from = 0) => { const n = TORCHX.indexOf(k, from); if (re.test(seg[n])) f.push(k + ': said ' + re + ': ' + seg[n].slice(0, 300)); };
        this.notes = TORCHX.map((l, i) => l + ': ' + seg[i].slice(0, 160));
        said('drop torch', /^You put down the (lit |unlit )?torch\./);
        not('drop torch', /You reach for|Put down\./);
        said('take torch', /Taken\./);
        said('inventory', /backpack, worn \(\w+\) 3 rations \(\w+\) a tinderbox \(\w+\) a torch \(\w+\) a flask of oil.*a torch, in your/, 3);
        said('light a new torch', /^You reach for your backpack\./);
        said('take the spent torch from the ground', /Taken\./);
        said('drop the spent torch', /^You put down the spent torch\./);
        said('drop the lit torch', /^You put down the lit torch\./);
        said('take the spent torch from the floor', /Taken\./);
        said('drop torch', /^You put down the spent torch\./, TORCHX.indexOf('drop torch', 2));
        not('drop torch', /Put down\./, 2);
        said('take torch', /You've no hand free for the torch: you put it in your backpack\./, TORCHX.indexOf('take torch', 3));
        not('light the torch', /put away the tinderbox/);
        said('inventory', /a tinderbox, in your/, TORCHX.indexOf('light the torch'));
        said('take spent torch from backpack', /^You reach for your backpack\./);
        said('put the rope in the backpack', /^You look for the rope\./);
        said('put the tinderbox in the backpack', /^(You reach for your backpack\.|You look for the tinderbox\. The tinderbox is there already\.)/);
        not('put the tinderbox in the backpack', /You reach for your backpack\. The tinderbox is there already/);
        said('dip the spent torch in the oil', /You set to dipping the/);
        not('dip the spent torch in the oil', /filling/);
        // (Every line's first letter within the user's fifth of a second, D17, at 115,200 baud, a thing in hand too: in each
        // typing, the test's own, the menacing pace's and the game as played, dq0Paces')
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], TORCHX.map(l => ({ line: l, want: 'You|Taken|Put|Your|It' })));
        if (/You see no (floor|ground) here/.test(o)) f.push('the floor not a place: ' + o.match(/.{80}You see no (floor|ground) here/)[0]);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0letters', outfit: true, reach: true, near: ['gear', 'save'], what: "Dingus Quest Zero's designators, step 1 (the user's design, 2026-11-06, DECISIONS.md's): the inventory a list, one a line, what's in a thing under it, each thing its letter in brackets, the burden line kept; a thing put down and taken up again its letter back; the letters kept in the save (quit, carried on with dq0 -d) and into the next dungeon (kit.dat); the inventory's first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', 'dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone,pick -d /ram/q', ...LETTERX[0],
        'dq0 -d /ram/q', ...LETTERX[1], 'dq0 -p t -s 259 -d /ram/q', ...LETTERX[2],
        ...dq0Paces('dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone,pick', LETTERX[0].slice(0, -1)), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [5]); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], raw = dq0Own(out).replace(/\x1b\[[0-9;?]*[A-Za-z]/g, '').replace(/\r/g, '');
        const invs = raw.split('You carry:\n').slice(1).map(t => t.slice(0, t.indexOf('In all, ') < 0 ? 0 : t.indexOf('In all, ')));
        const lets = t => { const r = {}; for (const l of t.split('\n')) { const x = /^ *\(([a-z]{1,2})\) ([^(,]*[^(, ])/.exec(l); if (x) r[x[2]] = x[1]; else if (l.trim()) r['?' + l.trim()] = '?'; } return r; };
        this.notes = invs.map(t => t.replace(/\n/g, ' / ').slice(0, 600));
        if (invs.length !== 5) return ['not five inventories: ' + invs.length];
        const L = invs.map(lets);
        for (const [i, l] of L.entries()) for (const k of Object.keys(l)) if (k[0] === '?') f.push('inventory ' + (i + 1) + ': a line with no letter: ' + k.slice(1));
        if (!/\n {2}\(\w+\) /.test(invs[0])) f.push('what\'s in a thing not under it, indented: ' + invs[0].slice(0, 300));
        if (!/a torch \(lit\), in your left hand/.test(invs[0])) f.push('the torch lit not told so: ' + invs[0].slice(0, 300));
        if (!(raw.split('In all, ').length > 5)) f.push('the burden line not kept');
        const same = (a, b, what, keys) => { for (const k of keys || Object.keys(L[a])) if (L[b][k] !== undefined && L[a][k] !== L[b][k]) f.push(what + ': ' + k + ' (' + L[a][k] + ') now (' + L[b][k] + ')'); };
        if (!L[0]['a pick'] || L[1]['a pick'] || L[2]['a pick'] !== L[0]['a pick']) f.push('the pick put down and taken up again not its letter back: ' + [0, 1, 2].map(i => L[i]['a pick']).join(', '));
        same(0, 2, 'taken up again');
        same(2, 3, 'carried on (the save)');
        if (Object.keys(L[3]).length < 10) f.push('the save\'s inventory short: ' + invs[3].slice(0, 300));
        same(3, 4, 'the next dungeon (kit.dat)', ['a pick', 'clothes', 'a backpack', 'a belt', 'a pouch', 'a tinderbox', 'a rope', 'a sack']);
        if (!['a pick', 'clothes', 'a backpack'].every(k => L[4][k])) f.push('the next dungeon\'s inventory not the kit: ' + invs[4].slice(0, 300));
        // (Each inventory's first letter held, D17's: the first game's, and its lines typed again at the menacing pace and as
        // the game is played, dq0Paces', its fourth and fifth games)
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 3, 4], LETTERX[0].filter(l => l === 'inventory').map(l => ({ line: l, want: 'You carry' })));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0letcmd', reach: true, near: ['gear', 'parser'], what: "Dingus Quest Zero's designators, step 2 (the user's design, 2026-11-06, DECISIONS.md's): a thing's letter where its name goes (drop j, put j in b, take j from b, light j with a, drop 2c), that thing and no other (the spare torch in the pack, not the one in hand), its first words its own; a letter not one where it's a way (look d) or an article (take a torch); a command with nothing named asks with the letters that fit (Drop what? [...]), one key answers; a line that isn't an answer lets it go; words still work beside them (drop the lit torch); each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', 'dq0 -p t -s 3 -c /ram/c.hm -T -k torch', ...LETCMD, 'quit',
        ...dq0Paces('dq0 -p t -s 3 -c /ram/c.hm -T -k torch', LETCMD), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [3]); dq0PlayLetters(m, 1, 3); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of LETCMD) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;
          const e = o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        const said = (k, re, from = 0) => { const n = LETCMD.indexOf(k, from); if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        const not = (k, re, from = 0) => { const n = LETCMD.indexOf(k, from); if (re.test(seg[n])) f.push(k + ': said ' + re + ': ' + seg[n].slice(0, 300)); };
        const inv = k => seg[LETCMD.indexOf('inventory', k)] || '';
        this.notes = LETCMD.map((l, i) => l + ': ' + seg[i].slice(0, 200));
        said('inventory', /\(j\) a torch, in your right hand/);
        said('drop j', /^You put down the (unlit )?torch\./);
        if (/\(j\) a torch, in your right hand/.test(inv(2)) || !/\(a\) a torch \(lit/.test(inv(2))) f.push('drop j: not the torch in the right hand put down, the lit one kept: ' + inv(2));
        said('take torch', /Taken\./);
        said('put j in b', /^You reach for your backpack\./);
        if (!/\(b\) a backpack, worn \([a-z]\) 3 rations[^]*\(j\) (a torch|two torches)/.test(inv(5))) f.push('put j in b: the torch not in the pack: ' + inv(5));
        said('take j from b', /^You reach for your backpack\./);
        said('light j with a', /torch/);
        said('drop 2c', /You put down 2 rations\./);
        said('look d', /^You look down|floor|Above|below/i);
        not('look d', /You put|You take/);
        said('drop', /^Drop what\? \[a, b, [a-z, ]+\]/);
        said('k', /^You put down the (unlit )?torch\.|^You reach for your backpack\./);
        said('take off', /^Take off what\? \[b, e\]/);
        not('look', /You take off/);
        // (A count of more than it carries, as played, "drop 2c" its backpack: not counted out, "You have only one.", the
        // world's "You drop it.", 2026-11-18)
        if (/You count out [^.]*\.\s+You drop/.test(dq0Story(out)) || /you put down it/.test(dq0Story(out))) f.push('a count of more than it carries counted out, or "put down it"');
        said('drop the lit torch', /^You put down the (lit )?torch\./);   // (Both lit after "light j with a": the plain name)
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], LETCMD.map(l => ({ line: l, want: 'You|Taken|Put|Your|It|Drop|Take' })));   // (D17's, each typing's: dq0Paces')
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0letsight', reach: true, near: ['gear', 'parser'], what: "Dingus Quest Zero's designators, step 3 (the user's design, 2026-11-06, DECISIONS.md's): a capital for each thing seen, given as it's first told, shown in the look's sentences and in the list of what's in sight (around, one a line with where it is); a capital where a thing's name goes (take D, take D from C), a count before it (take 5E: fewer there, told, the rest taken), a thing put down its old capital again; each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', 'dq0 -p t -s 3 -c /ram/c.hm -T -k torch', ...LETSIGHT, 'quit',
        ...dq0Paces('dq0 -p t -s 3 -c /ram/c.hm -T -k torch', LETSIGHT), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [3]); dq0PlayLetters(m, 1, 3); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of LETSIGHT) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;
          const e = o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        this.notes = LETSIGHT.map((l, i) => l + ': ' + seg[i].slice(0, 400));
        const said = (k, re, from = 0) => { const n = LETSIGHT.indexOf(k, from); if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        said('look', /At your feet lie \(E\) a backpack, \(F\) 3 rations and \(G\) a torch\./);
        said('look', /\(D\) some bones/);
        // (A ... C the giant rats', seen as the game starts: the denizens' capitals first, step 4's)
        said('around', /\(E\) a backpack, at your feet \(F\) 3 rations, at your feet \(G\) a torch, at your feet \(D\) some bones, ahead to your right/);
        said('take 5F', /There are only 3 there; you take them all\./);
        said('drop c', /^You put down/, 6);                   // (Taken up, its letter again)
        said('around', /\(F\) 3 rations, at your feet/, 6);  // (Put down again: its old capital)
        said('take 2F', /You take 2 rations\./);
        said('take torch from E', /torch/);
        if (/You see no|none of those|hands are full/.test(seg[LETSIGHT.indexOf('take torch from E')])) f.push('take torch from E: not taken: ' + seg[LETSIGHT.indexOf('take torch from E')]);
        said('take G', /Taken\./);
        said('around', /\(H\) a torch, at your feet/, 12);  // (The spare put down: a capital of its own)
        if (/\(G\)/.test(seg[LETSIGHT.indexOf('around', 12)])) f.push('the torch taken up still told in sight: ' + seg[LETSIGHT.indexOf('around', 12)]);
        said('inventory', /\(j\) a torch, in your right hand/);
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], LETSIGHT.map(l => ({ line: l, want: 'You|Taken|Put|There|(' })));   // (D17's, each typing's: dq0Paces')
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0letwhich', reach: true, near: ['gear', 'parser'], what: "Dingus Quest Zero's designators, step 5 (the user's playtest, 2026-11-06: \"If there are three torches, one in hand, one in my backpack on my back, and one on the ground, if I say pick up the torch it should take the one I don't already have (from the ground).  But if all three are on the ground, I should get a prompt for which one, with longer descriptions of each\"): the user's three torches, one taken up from the ground; three on the ground, two of them alike, asked about once a description (Which one? (E) a torch (lit), (F) a torch: the world's IR_WHICH, the inventory's words, each its capital), answered by its capital (F), by its number (2), by a word of its state (lit), each re-running the command on that one; no first words before the question; each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', 'dq0 -p t -s 3 -c /ram/c.hm -T -k torch', ...LETWHICH, 'quit',
        ...dq0Paces('dq0 -p t -s 3 -c /ram/c.hm -T -k torch', LETWHICH), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [3]); dq0PlayLetters(m, 1, 3); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of LETWHICH) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;
          const e = o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        this.notes = LETWHICH.map((l, i) => l + ': ' + seg[i].slice(0, 300));
        const said = (k, re, from = 0) => { const n = LETWHICH.indexOf(k, from); if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        said('pick up the torch', /Taken\./);
        said('inventory', /\(a\) a torch \(lit\), in your left hand[^]*\(k\) a torch[^]*\(j\) a torch, in your right hand/);   // (The ground's taken: the pack's and the hand's kept)
        said('take the torch', /^You look them over\. Which one\? \(E\) a torch \(lit\), \(F\) a torch $/);   // (Three: two alike asked of once)
        said('F', /^You reach for the torch\. Taken\./);
        said('take the torch', /^You look them over\. Which one\? \(E\) a torch \(lit\), \(G\) a torch $/, 8);
        said('2', /^You reach for the torch\. Taken\./);
        said('inventory', /\(j\) a torch, in your left hand \([a-z]\) a torch, in your right hand/, 10);
        said('take torch', /^You look them over\. Which one\? \(E\) a torch \(lit\), \(F\) a torch $/);
        said('lit', /^You reach for the torch\. Taken\./);
        said('inventory', /a torch \(lit\), in your left hand/, 14);
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], LETWHICH.map(l => ({ line: l, want: 'You|Taken|Which' })));   // (D17's, each typing's: dq0Paces')
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0letseveral', reach: true, near: ['gear', 'parser'], what: "Dingus Quest Zero's designators, step 6 (the user's design, 2026-11-06): several letters in a line, each its own command in turn (drop h i: the dagger, then the waterskin; take E F, by their capitals); letter h as t swaps two of the character's letters (Your dagger is (t) now.), the inventory after it so, the new letter naming the thing (drop t); a letter of nothing carried told so; each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'torch,sheath,pouch'", ...LETSEVERAL, 'quit',
        ...dq0Paces("dq0 -p t -s 3 -c /ram/c.hm -T -k 'torch,sheath,pouch'", LETSEVERAL), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [3]); dq0PlayLetters(m, 1, 3); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of LETSEVERAL) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;
          const e = o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        this.notes = LETSEVERAL.map((l, i) => l + ': ' + seg[i].slice(0, 300));
        const said = (k, re, from = 0) => { const n = LETSEVERAL.indexOf(k, from); if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        const not = (k, re, from = 0) => { const n = LETSEVERAL.indexOf(k, from); if (re.test(seg[n])) f.push(k + ': said ' + re + ': ' + seg[n].slice(0, 300)); };
        said('drop h i', /You put down the dagger\.[^]*You put down the waterskin\./);
        said('look', /At your feet lie \(E\) a dagger, \(F\) a waterskin and \(G\) a torch\./);
        said('take E F', /You reach for the dagger\.[^]*You reach for the waterskin\./);
        said('inventory', /\(i\) a waterskin[^]*\(h\) a dagger, in your right hand/);   // (Taken up again: their letters back)
        said('letter h as t', /^Your dagger is \(t\) now\. ?$/);
        said('inventory', /\(t\) a dagger/, 6);
        not('inventory', /\(h\)/, 6);
        said('drop t', /Put down\.|You put down the dagger/);
        said('letter q as r', /^You have nothing by that letter\./);
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], LETSEVERAL.map(l => ({ line: l, want: 'You|Taken|Your|Put|Stow' })));   // (D17's, each typing's: dq0Paces')
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0letstow', reach: true, near: ['gear', 'parser'], what: "Dingus Quest Zero's designators, step 7 (the user's, 2026-11-06: \"when stowing something, it should ask where if there is more than one place it could go (pouch(es) or backpack), of course depending on size.  Stowing a weapon with a sheath should choose the sheath if worn.\"): a dagger stowed with the backpack and a pouch on the belt to take it, asked (Stow it where? (b) the backpack, (f) the pouch), one key answering it; a penny dropped and taken up: into the purse (the first pouch on the belt), unasked (the user's, 2026-11-17: coins to the purse); a waterskin too big for any pouch, asked of the pack and its side pocket (a place of its own, the user's, 2026-11-17), the pack answering; the sheath hung on the belt, the dagger stowed in it without asking; each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'torch,sheath,pouch'", ...LETSTOW, 'quit',
        ...dq0Paces("dq0 -p t -s 3 -c /ram/c.hm -T -k 'torch,sheath,pouch'", LETSTOW), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [3]); dq0PlayLetters(m, 1, 3); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of LETSTOW) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;
          const e = o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        this.notes = LETSTOW.map((l, i) => l + ': ' + seg[i].slice(0, 300));
        const said = (k, re, from = 0) => { const n = LETSTOW.indexOf(k, from); if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        const not = (k, re, from = 0) => { const n = LETSTOW.indexOf(k, from); if (re.test(seg[n])) f.push(k + ': said ' + re + ': ' + seg[n].slice(0, 300)); };
        said('stow the dagger', /^You lift the dagger\. Stow it where\? \(b\) the backpack, the side pocket, \(f\) the pouch $/);
        said('f', /You put the dagger in the pouch\./);
        said('take the pennies', /Taken\./);
        said('inventory', /\(f\) a pouch \(g\) 12 pennies \(h\) a dagger \(i\)/);
        said('stow i', /Stow it where\? \(b\) the backpack, the side pocket $/);
        said('b', /You stow the waterskin in your backpack\./);
        said('stow the dagger', /^You lift the dagger\. You put the dagger in the sheath\./, 12);
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], LETSTOW.map(l => ({ line: l, want: 'You|Taken|Your|Put|Stow' })));   // (D17's, each typing's: dq0Paces')
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0doff', near: ['gear'], what: "Dingus Quest Zero's things taken off with both hands full (the user's playtest, 2026-11-06: \"take off the backpack\" refused with its hands full, \"drop backpack\" not): a sword and the torch in hand, a cloak and an iron helm worn (dq0 -k), each taken off with no hand free and no room in the pack set down at the character's feet, said so; the backpack, worn, the same, its things with it; nothing refused; each first letter within the user's 720,000 cycles",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo b115200 >/dev/serctl', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'sword,cloak,iron helm'",
        ...DOFF, 'quit', ...dq0Paces("dq0 -p t -s 3 -c /ram/c.hm -T -k 'sword,cloak,iron helm'", DOFF), 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); dq0Asks(m, [3]); },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(dq0Own(out)).replace(/\s+/g, ' ');
        inTurn(o, ['> take off the cloak', 'Your hands are full: you take off the cloak and set it down at your feet.',
          '> take off the iron helm', 'Your hands are full: you take off the iron helm and set it down at your feet.',
          '> take off the backpack', 'Your hands are full: you take off the backpack and set it down at your feet.'], f, 'the words: ');
        if (/stow something, or put it down/.test(o.slice(o.indexOf('> take off the cloak')))) f.push('a take off refused, its hands full');
        const inv = o.slice(o.lastIndexOf('> i '));
        if (/cloak|helm|backpack/.test(inv.slice(0, inv.indexOf('In all')))) f.push('still carried: ' + inv.slice(0, 300));
        this.notes = [];
        dq0HeldPaces(this, f, this.notes, this.timer, [0, 1, 2], ['take off the cloak', 'take off the iron helm', 'take off the backpack'].map(l => ({ line: l, want: 'You|Your' })));   // (D17's, each typing's: dq0Paces')
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wearonly', near: ['gear', 'parser'], what: "Dingus Quest Zero's wear for what's worn (the user's, 2026-11-09: \"wear\" restricted to articles that can be worn, DECISIONS.md's; commands.verbs' wear, its aux IO_DON_WORN, the world's IO_DON to DN_WORN): a sword and the torch in hand refused (\"You can't wear the sword.\", the catalogue's own words, no weapon taken in hand), as a sack always was; \"wear\" alone asks with the letters of the kinds that are worn, those not worn yet (the cloak's), not the sword's nor the torch's; its letter answers, the cloak put on; wield and equip as they were, the sword taken in hand",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'sword,cloak,sack'",
        ...WEARONLY, 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(out).replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), seg = [];
        let at = 0;
        for (const l of WEARONLY) {
          let i = o.indexOf('> ' + l + ' ', at);
          if (i >= 0 && o.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3;   // (Its echo, then the line as it's drawn)
          const e = i < 0 ? -1 : o.indexOf('> ', i + 2);
          seg.push(i < 0 ? '' : o.slice(i + l.length + 3, e < 0 ? undefined : e));
          if (i >= 0) at = i + 2;
        }
        this.notes = WEARONLY.map((l, i) => l + ': ' + seg[i].slice(0, 300));
        const said = (k, re) => { const n = WEARONLY.indexOf(k); if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        const letter = name => { const r = seg[0].match(new RegExp('\\(([a-z]{1,2})\\) an? ' + name)); return r ? r[1] : null; };
        const [cloak, sword, torch] = [letter('cloak'), letter('sword'), letter('torch')];
        if (!cloak || !sword || !torch) return ['inventory: no cloak, sword or torch: ' + seg[0].slice(0, 400)];
        said('wear the sword', /You can't wear the sword\./);   // (Not held instead: "You hold the sword in your right hand already.")
        said('wear the torch', /You can't wear the (lit )?torch\./);
        said('wear the sack', /You can't wear the sack\./);
        for (const k of ['wear the sword', 'wear the torch']) if (/You (take|hold)/.test(seg[WEARONLY.indexOf(k)])) f.push(k + ': held instead');
        const ask = (seg[WEARONLY.indexOf('wear')].match(/^Wear what\? \[([a-z, ]+)\]/) || [])[1];
        if (!ask) f.push('wear: not asked "Wear what? [...]": ' + seg[WEARONLY.indexOf('wear')].slice(0, 200));
        else {
          const offer = ask.split(', ');
          if (!offer.includes(cloak)) f.push('wear: the cloak (' + cloak + ') not offered: ' + ask);
          if (offer.includes(sword) || offer.includes(torch)) f.push('wear: the sword or the torch offered: ' + ask);
        }
        said('k', /You put on the cloak\./);
        const held = (k, n) => { const s = seg[WEARONLY.indexOf('inventory', n)]; if (!/a sword, in your (left|right) hand/.test(s)) f.push(k + ': the sword not in hand: ' + s.slice(0, 400)); };
        said('equip the sword', /sword/);
        held('equip the sword', WEARONLY.indexOf('equip the sword'));
        said('wield the sword', /sword/);
        held('wield the sword', WEARONLY.indexOf('wield the sword'));
        for (const k of ['equip the sword', 'wield the sword']) if (/can't wear/.test(seg[WEARONLY.indexOf(k)])) f.push(k + ': refused as a wear');
        if (!/a cloak, worn/.test(seg[WEARONLY.lastIndexOf('inventory')])) f.push('the cloak not worn: ' + seg[WEARONLY.lastIndexOf('inventory')].slice(0, 400));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0worn', near: ['gear', 'parser'], what: "Dingus Quest Zero's worn places (the user's, 2026-11-10: \"Wearing should extend to rings, necklaces, earrings, and other jewelry, as well as scarves, and should keep track of where worn\", DECISIONS.md's): four rings on the left hand, a fifth refused there; a ring where it's never worn refused; a ring unsaid on the right hand once the off hand is full; a scarf put on the head, a necklace put around the neck, earrings in the ears by their kind; the inventory's and a look's words by place; \"take off the ring\" asking which by place, \"right\" answering; the scarf worn over the face",
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'ring,ring,ring,ring,ring,ring,necklace,scarf,earrings'",
        ...WORN, 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], seg = dq0Segs(out, WORN);
        this.notes = WORN.map((l, i) => l + ': ' + seg[i].slice(0, 300));
        const said = (i, re) => { if (!re.test(seg[i])) f.push(i + ' ' + WORN[i] + ': not ' + re + ': ' + seg[i].slice(0, 400)); };
        const not = (i, re) => { if (re.test(seg[i])) f.push(i + ' ' + WORN[i] + ': ' + re + ': ' + seg[i].slice(0, 400)); };
        for (const i of [1, 2, 3, 4]) said(i, /You put on the ring\./);
        said(5, /There's no room left on your left hand for the ring\./);
        said(6, /You can't wear the ring about your neck\./);
        said(7, /You put on the ring\./);
        said(8, /\) four rings, on your left hand/);
        said(8, /\) a ring, on your right hand/);
        said(9, /You put on the scarf\./);
        said(10, /You put on the necklace\./);
        said(11, /You put on the earrings\./);
        said(12, /Your scarf is worn on your head\./);
        said(13, /\) a scarf, on your head/);
        said(13, /\) a necklace, about your neck/);
        said(13, /\) earrings, in your ears/);
        said(14, /Which one\? .*\(\w\) a ring, on your left hand/);
        said(14, /Which one\? .*\(\w\) a ring, on your right hand/);
        said(15, /You take off the ring\./);
        said(16, /\) four rings, on your left hand/);
        not(16, /on your right hand/);
        said(18, /You put on the scarf\./);
        said(19, /\) a scarf, over your face/);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wornkit', outfit: true, near: ['gear', 'save'], what: "Dingus Quest Zero's worn places kept (the user's, 2026-11-10): a ring on each hand and a scarf over the face kept in the save (quit, carried on with dq0 -d: the item store's sn) and into the next dungeon (kit.dat: its ofit's in, WP_KIT's)",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -k sunstone,moonstone,ring,ring,scarf -d /ram/q', ...WORNKIT[0],
        'dq0 -d /ram/q', ...WORNKIT[1], 'dq0 -p t -s 259 -d /ram/q', ...WORNKIT[2], 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], invs = dq0Prose(out).split('You carry:').slice(1).map(t => t.slice(0, t.indexOf('In all, ') < 0 ? 600 : t.indexOf('In all, ')));
        this.notes = invs.map(t => t.slice(0, 600));
        if (invs.length !== 3) return ['not three inventories: ' + invs.length];
        ['the first game', 'carried on (the save)', 'the next dungeon (kit.dat)'].forEach((w, i) => {
          for (const re of [/\) a ring, on your left hand/, /\) a ring, on your right hand/, /\) a scarf, over your face/]) if (!re.test(invs[i])) f.push(w + ': not ' + re + ': ' + invs[i].slice(0, 500));
        });
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0wornden', outfit: true, near: ['gear', 'denizens'], what: "Dingus Quest Zero's worn places, a denizen's (the user's, 2026-11-10): its jewelry at its places, drawn by each thing's own number (its sn the stratum it's filed in), told as it's examined: a bandit's clothes made earrings and its shoes a ring (the scene's), \"earrings in its ears\", \"a ring on its left (or right) hand\"",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 23 -u feudal -c hunter -o /ram/c.hm', 'echo ml dge 95 >> /ram/c.hm', 'echo ml swd 95 >> /ram/c.hm',
        'mkdir /ram/g', "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | h | -' > /ram/g/theme.txt", 'dq0hour=128', "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'sword,mail hauberk,great helm'",
        'drop the staff', 'wear the mail hauberk', 'wear the great helm', 'hold the sword', 'examine the bandit', 'quit', 'echo $status']) },
      start(m) {   // (At "examine the bandit"'s Enter: every bandit's clothes worn made earrings, its shoes or boots a ring)
        const jewel = (h, L) => {
          const cat = m.dq0.bank(L.catBank), per = Math.floor(0x2000 / L.itemSize), K = m.dq0.layout().K;
          const kname = k => { let n = ''; for (let j = 0; j < 14 && cat[k * L.kindSize + j]; j++) n += String.fromCharCode(cat[k * L.kindSize + j]); return n; };
          const kind = n => { for (let k = 1; k < 256; k++) if (kname(k) === n) return k; return 0; };
          const ear = kind('earrings'), ring = kind('ring');
          for (let i = 0; i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (!b || !b[o + L.iKind] || b[o + L.iWhere] !== K.IW_DEN || !(b[o + L.iFlags] & DQ0_I.WORN)) continue;
            const n = kname(b[o + L.iKind]);
            if (n === 'clothes') b[o + L.iKind] = ear;
            else if (n === 'shoes' || n === 'knee boots') b[o + L.iKind] = ring;
          }
        };
        dq0Poke(m, [{ g: 1, line: 'examine the bandit', n: 1, call: jewel }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), i = o.indexOf('> examine the bandit'), w = i < 0 ? '' : o.slice(i, i + 700), f = [];
        this.notes = [w];
        if (!/The bandit (?:holds|wears)/.test(w)) return ['the bandit not examined: ' + w];
        if (!/earrings in their ears/.test(w)) f.push('no earrings in their ears: ' + w);
        if (!/a ring on their (?:left|right) hand/.test(w)) f.push('no ring on their hand: ' + w);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0keysteps', reach: true, outfit: true, near: ['gear'], what: "Dingus Quest Zero's door's key by a tool's steps (2026-11-04, the sweep of the tool's routine, C21: HOUSE.md's every use of a tool): at seed 1 (dq0denkey's door, its key the character's, in its pack): 'lock the door' is 'You take off the backpack', 'You reach into', 'You get out the iron key', 'You set to turning a key', 'You lock it with its key', 'You put away the iron key', 'You put on the backpack', each a step, in that order; 'unlock the door' the same; a key that doesn't fit is none of it; and 'lock the door' locked already refused at once, no key got out",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", "dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k 'iron key'",
        'open the door', 'close the door', 'put the key in the backpack', 'lock the door', 'lock the door', 'unlock the door', 'quit', 'echo $status']) },
      start(m) {
        const a = m.acia, read = a.read;
        let typed = '';
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed === 'lock the door' && m.dq0 && !m.keyed) {   // (The door ahead given a lock, the key the character's: dq0denkey's)
                const { L, K } = m.dq0.layout(), h = m.dq0.bank(0), u16 = o => h[o] | h[o + 1] << 8;
                const px = u16(L.pcX), py = u16(L.pcY), sn = h[L.pcSn], f = h[L.pcFace];
                const x = px + [0, 1, 1, 1, 0, -1, -1, -1][f], y = py + [-1, -1, 0, 1, 1, 1, 0, -1][f], P = dq0PortalAt(m, sn, x, y);
                const id = 0x8000 | ((x * 37 + y * 101 + sn * 4099) & 0x7FFF), per = Math.floor(0x2000 / L.itemSize);
                let key = -1;
                for (let i = 1; P && i < L.nitems && key < 0; i++) {
                  const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
                  if (b && b[o + L.iKind] === K.KI_KEY && b[o + L.iWhere] !== K.IW_DEN && b[o + L.iWhere] !== K.IW_SQUARE) {
                    b[o + L.iSize] = id & 255;
                    b[o + L.iThick] = id >> 8;
                    key = i;
                  }
                }
                if (P && key > 0) {
                  if (!P.b[P.o + 4]) P.b[P.o + 4] = 3;
                  m.keyed = { door: [x, y], sn, key };
                }
              }
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
      },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Story(out), f = [];
        if (!m.keyed) return ['the key not made the door\'s: no door ahead, or no iron key carried'];
        const after = (w, from) => { const p = o.indexOf('> ' + w + ' > ' + w, from); return p < 0 ? -1 : p + 2 * w.length + 5; };
        const i = after('lock the door', 0), j = after('lock the door', i), k = after('unlock the door', j), e = o.indexOf('> quit');
        if (process.env.DQ0SHOW) f.push(o.slice(i - 40, e));
        const inOrder = (t, list, what) => { let p = 0; for (const w of list) { const q = t.indexOf(w, p); if (q < 0) { f.push(what + ': no "' + w + '" in its order: ' + t); return; } p = q; } };
        const steps = ['You take off the backpack', 'You reach into the backpack', 'You get out the iron key', 'You set to turning a key', 'with its key', 'You put away the iron key', 'You put on the backpack'];
        inOrder(o.slice(i, j), steps, 'locked');
        inOrder(o.slice(k, e), steps, 'unlocked');
        if (/You get out|take off the backpack/.test(o.slice(j, k))) f.push('locked already, and still got out: ' + o.slice(j, k));
        return f;
      },
    },
    {
      name: 'dq0feedsteps', reach: true, outfit: true, near: ['gear'], what: "Dingus Quest Zero's eating and drinking by a tool's steps (2026-11-04, C21: HOUSE.md's every use of a tool): hungry and thirsty (the referee's clock poked), 'eat' gets its rations out of the worn pack ('You take off the backpack', 'You get out the rations', 'You eat a meal', 'You put away the rations', 'You put on the backpack'); a ration begun, the next meal none of it; 'drink' from the waterskin at its belt ('You get out the waterskin', no pack off, 'You drink from your waterskin', 'You put away the waterskin')",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T',
        'wait 8 hours', 'wait 8 hours', 'eat the rations', 'drink', 'quit', 'echo $status']) },
      start(m) {
      },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [];
        const i = o.indexOf('> eat the rations'), j = o.indexOf('> drink'), e = o.indexOf('> quit');
        const inOrder = (t, list, what) => { let p = 0; for (const w of list) { const q = t.indexOf(w, p); if (q < 0) { f.push(what + ': no "' + w + '" in its order: ' + t); return; } p = q; } };
        inOrder(o.slice(i, j), ['You take off the backpack', 'You get out the rations', 'You eat a meal', 'You put away the rations', 'You put on the backpack'], 'eat');
        inOrder(o.slice(j, e), ['You get out the waterskin', 'You drink from your waterskin', 'You put away the waterskin'], 'drink');
        if (/take off the backpack/.test(o.slice(j, e))) f.push('the pack off for what\'s at its belt: ' + o.slice(j, e));
        return f;
      },
    },
    {
      name: 'dq0blowsteps', reach: true, outfit: true, near: ['gear', 'fight'], what: "Dingus Quest Zero's blow's weapon named from the pack, by the steps (2026-11-04, C21: HOUSE.md's reach, a blow's weapon): the character, a rat beside it (dq0dentake's way), 'attack the rat with the pick', the pick in its worn pack: each step told, a round a step engaged ('You take off the backpack', 'You get out the pick', 'You take the pick'), then the blow, struck with the pick; not 'Your hands are full', nor the blow struck with anything else",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'mkdir /ram/g', 'dq0hour=128',
        "echo 'faction | Brigands | bandit | 0-0 | 30 | - | 0 | w | -' > /ram/g/theme.txt", 'dq0 -p t -s 1 -c /ram/c.hm -T -d /ram/g -k pick',
        'drop the torch', 'wait', 'attack the bandit with the pick', 'wait 30 seconds', 'stop', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 1, line: 'wait', n: 1, call: (h, L, K, at, named) => {
          const u16 = o => h[o] | h[o + 1] << 8, w16 = (o, v) => { h[o] = v & 255; h[o + 1] = v >> 8 & 255; };
          const k = [...Array(L.nactors).keys()].find(j => h[at(j) + L.aState] && named(j) === 'bandit');
          if (k === undefined) return;
          const px = u16(L.pcX), py = u16(L.pcY), x = px + 1, y = py, R = dq0Rec(m), q = R.at(h[at(k) + L.aDen]);
          w16(at(k) + L.aX, x);
          w16(at(k) + L.aY, y);
          h[at(k) + L.aSn] = h[L.pcSn];
          R.mem[q + R.X] = x & 255; R.mem[q + R.X + 1] = x >> 8;
          R.mem[q + R.Y] = y & 255; R.mem[q + R.Y + 1] = y >> 8;
          R.mem[q + R.SN] = h[L.pcSn];
          h[L.denSeq] = h[L.denSeq] + 1 & 255;
          m.bandit = k;
        } }]);
      },
      sdcard: true,
      expect: ['% echo $status'],
      check(m, out) {
        const o = dq0Prose(out), f = [];
        if (m.bandit === undefined) return ['no bandit to set beside it'];
        const i = o.indexOf('> attack the bandit with the pick'), t = o.slice(i, o.indexOf('> quit'));
        const inOrder = list => { let p = 0; for (const w of list) { const q = t.indexOf(w, p); if (q < 0) { f.push('no "' + w + '" in its order: ' + t.slice(0, 900)); return; } p = q; } };
        inOrder(['You take off the backpack', 'You get out the pick']);
        if (/hands are full/.test(t)) f.push('refused: ' + t.slice(0, 500));
        return f;
      },
    },
    {
      name: 'dq0rip', near: ['gear', 'parser'], what: "Dingus Quest Zero's cloth torn up for bandages (the user's, 2026-11-10: \"'rip' and 'tear up' verbs should be allowed to turn a garment, bed sheet, towel, etc. into bandages\"; HOUSE.md's bandages; commands.verbs' rip, its aux IO_RIP, the world's part 107, wcloth.c): a robe worn refused (\"take off the robe first\"), a leather vest refused (not cloth), a rope not torn up; the wool cloak with no blade carried refused (too stout to tear by hand); the linen surcoat torn by hand into 4 bandages (two minutes each: a timed wait of eight minutes and more); the cloak cut up with the dagger taken up again into 6 (a minute each); both lots worn (a garment's cloth, worn at best: its cleanliness kept), each its own lot (linen and wool not joined); the bandages examined, made of wool",
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', "dq0 -p t -s 3 -c /ram/c.hm -T -k 'cloak,surcoat,robe,leather vest,rope,towel'",
        ...RIPLINES, 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], seg = dq0Segs(out, RIPLINES), said = (k, re, n = RIPLINES.indexOf(k)) => { if (!re.test(seg[n])) f.push(k + ': not ' + re + ': ' + seg[n].slice(0, 300)); };
        this.notes = RIPLINES.map((l, i) => l + ': ' + seg[i].slice(0, 400));
        said('rip the robe', /You'd have to take off the robe first\./);
        said('rip the leather vest', /Leather won't do: the leather vest isn't cloth\./);
        said('rip the rope', /You can't tear the rope into bandages\./);
        said('rip the cloak', /The cloak is too stout to tear by hand: you want a blade to cut it\./);
        said('tear up the surcoat', /You tear the surcoat into 4 bandages\./);
        said('cut up the cloak with the dagger', /You cut the cloak into 6 bandages\./);
        const inv = seg[RIPLINES.lastIndexOf('inventory')];
        if (!/4 bandages \(worn\)/.test(inv) || !/6 bandages \(worn\)/.test(inv)) f.push('the two lots not each its own, worn: ' + inv.slice(0, 500));
        if (/cloak|surcoat/.test(inv)) f.push('the cloth still carried: ' + inv.slice(0, 500));
        said('examine the bandages', /wool/);
        said('use the dagger on the towel', /You cut the towel into 2 bandages\./);
        const t = dq0Turns(m), wait = l => ((t.of(l).intents || []).find(e => e.kind === 'I_WAIT') || {}).tenths || 0;
        if (wait('tear up the surcoat') < 4800) f.push('the surcoat torn by hand not eight minutes: ' + wait('tear up the surcoat'));
        if (wait('cut up the cloak with the dagger') < 3600 || wait('cut up the cloak with the dagger') >= 4800) f.push('the cloak cut up not six minutes: ' + wait('cut up the cloak with the dagger'));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0dress', near: ['body', 'parser'], what: "Dingus Quest Zero's dressings (the user's, 2026-11-10: \"A bandage's location should be known\", dressings soil over days and are changed, wash, not boil; HOUSE.md's bandages): a serious stab to the left forearm and one to the right, tended, its dressing soiled (the character's file's: its \"dressed\" line; the days' soiling dq0rules'), five clean linen bandages carried, stowed in the pack (the old dressing kept there; one in hand, at its feet); \"bandage my left arm\" a dressing (its grade the best bandage's, 9: the referee's bonus whole), one bandage used up; the sheet's \"dressed in linen, clean\" at the left forearm (its cloth named: the user's, 2026-11-11), \"dressed, soiled\" at the right (the file's line from before cloths were kept: its grade's words); \"change the bandage on my right arm\" five minutes, the fresh one on, the old kept, a bloody bandage, the sheet's \"dressed in linen, clean\" at both; washing it with no water about refused; at running water (the hub's pc_wet poked) it's washed, worn, not clean, a lot of its own",
      init: 't_rc', cycles: 3000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 3 -u feudal -c hunter -o /ram/c.hm', 'echo inj 6 2 2 2 0 0 >> /ram/c.hm', 'echo inj 134 2 2 2 5 4 >> /ram/c.hm', 'echo dressed Eig >> /ram/c.hm', 'echo ml phy 60 >> /ram/c.hm',
        "dq0 -p t -s 3 -c /ram/c.hm -T -k 'bandages'", ...DRESSLINES, 'quit', 'echo $status']) },
      start(m) {   // (Five bandages, the kit's one made so; the water at the second wash running)
        dq0Poke(m, [{ g: 1, line: 'inventory', n: 1, item: { bandages: { qty: 5 } } },
          { g: 1, line: 'wash the bandages', n: 1, hub: { pcWet: 0 } }, { g: 1, line: 'wash the bandages', n: 2, hub: { pcWet: 0x81 } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], seg = dq0Segs(out, DRESSLINES), s = dq0Prose(out).replace(/\s+/g, ' '), said = (k, re, n = DRESSLINES.indexOf(k)) => { if (!re.test(seg[n])) f.push(k + ' (' + n + '): not ' + re + ': ' + seg[n].slice(0, 300)); };
        this.notes = DRESSLINES.map((l, i) => l + ': ' + seg[i].slice(0, 400));
        said('bandage my left arm', /You clean and dress the serious stab to your left forearm \(S2\)\..*It's (?:well tended|tended|tended, roughly|badly done)\./);
        said('sheet', /left forearm S2 \(stab\), tended, dressed in linen, clean; right forearm S2 \(stab\), tended, dressed, soiled/);
        said('change the bandage on my right arm', /You unwind the old dressing from the serious stab to your right forearm \(S2\)\..*The fresh dressing is on\./);
        said('inventory', /3 bandages(?! \((?:worn|soiled|bloody)\))/, 5);
        said('inventory', /a bandage \(bloody\)/, 5);
        said('sheet', /left forearm S2 \(stab\), tended, dressed in linen, clean; right forearm S2 \(stab\), tended, dressed in linen, clean/, 6);
        said('wash the bandages', /There's no running water here, nor a well, to wash the bandages in\./);
        said('wash the bandages', /You wash the bandages and wring them out: fit to use again, if not new\./, DRESSLINES.lastIndexOf('wash the bandages'));
        said('inventory', /a bandage \(worn\)/, DRESSLINES.lastIndexOf('inventory'));
        if (/bloody/.test(seg[DRESSLINES.lastIndexOf('inventory')])) f.push('the bloody bandage not washed');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0ownseen', meaning: true, near: ['gear'], what: "Dingus Quest Zero's things made their own as they're seen (make_own, the world's owned; 2026-11-04: its loop the main RAM's, part 13 copied only for a thing not yet its own, the swap study's: a thing's number, its quality, its make come as they did): at the turns pace, what a look sees has a number; a look again changes no thing's record and takes no new number (none made twice, none of the store's dice spent); things out of sight have none; one it sees with no number (the scene's: its number cleared) has one when the world next looks about, the next number, the others untouched, and only once",
      init: 't_rc', cycles: 400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'look', 'look', 'turn left', 'turn left', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      start(m) {                                  // (A thing it sees made not its own yet, as the turn's typed: the world looks about after it)
        dq0Poke(m, [{ g: 1, line: 'turn left', n: 1, call: () => {
          const { L, K } = m.dq0.layout(), per = Math.floor(0x2000 / L.itemSize), cat = m.dq0.bank(L.catBank);
          for (let i = 0; i < L.nitems; i++) {
            const b = m.dq0.bank(L.item0 + Math.floor(i / per)), o = (i % per) * L.itemSize;
            if (b[o + L.iKind] && b[o + L.iWhere] === K.IW_SQUARE && (b[o + L.iId] | b[o + L.iId + 1] << 8)) {
              let name = ''; for (let c = b[o + L.iKind] * L.kindSize; cat[c]; c++) name += String.fromCharCode(cat[c]);
              if (name === 'bones' || name === 'potsherds' || name === 'arrows' || name === 'pennies') { m.dq0Cleared = i; b[o + L.iId] = b[o + L.iId + 1] = 0; return; }
            }
          }
        } }]);
      },
      check(m) {
        const T = dq0Turns(m), f = [], rec = i => [i.n, i.kind, i.id, i.quality, i.mat, i.dur, i.durMax, i.where, i.x, i.y].join();
        const look = n => { let t = T.of('look'); for (let k = 1; k < n; k++) t = T.after(t, 'look'); return t.after; };
        const tl = T.of('turn left'), a = look(1), c = tl.after, d = T.after(tl, 'turn left').after, z = m.dq0Cleared;
        if (!a || !c || !d) return ['no state'];
        const others0 = s => lying(s).filter(i => i.n !== z).map(rec).join(';');
        const seen = a.sight.items, top = s => Math.max(...s.items.map(i => i.id)), lying = s => s.items.filter(i => i.where === 'IW_SQUARE');
        if (!seen.length) f.push('nothing seen on the first look: the scene is gone');
        for (const s of seen) if (!a.items.some(i => i.name === s.name && i.id)) f.push('seen, but no number: ' + s.name);
        if (others0(a) !== others0(look(2))) f.push('a look again changed the things lying about');
        const rest = s => Math.max(...s.items.filter(i => i.n !== z).map(i => i.id));   // (Its number cleared as the turn's typed: look(2) is that moment's)
        if (rest(a) !== rest(look(2))) f.push('a look again took a new number: ' + rest(a) + ' to ' + rest(look(2)));
        const ids = a.items.filter(i => i.id).map(i => i.id);
        if (new Set(ids).size !== ids.length) f.push('two things with one number: ' + ids.join());
        if (!lying(a).some(i => !i.id)) f.push('no thing out of sight without a number: the scene is gone');
        if (z === undefined) f.push('no thing to clear: the scene is gone');
        else {
          const th = (s, n) => s.items.find(i => i.n === n);
          const was = th(a, z), got = th(c, z);
          if (!seen.some(s => s.name === was.name)) f.push('the thing cleared not among those seen: ' + was.name + ' of ' + JSON.stringify(seen.map(s => s.name)));
          else if (!got || !got.id || got.id !== top(a) + 1) f.push('a thing seen with no number not made its own at the next look: ' + JSON.stringify(got) + ' top ' + top(a));
          const others = s => lying(s).filter(i => i.n !== z).map(rec).join(';');
          if (others(a) !== others(c)) f.push('another thing changed by that look');
          if (lying(c).map(rec).join(';') !== lying(d).map(rec).join(';') || top(c) !== top(d)) f.push('made its own twice');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0inside', what: 'Dingus Quest Zero\'s things looked into and opened (the user\'s playtest, 2026-10-21: "look in the sack", "look on the table", "open the chest"): at seed 3 (dq0 -T, a kit of things more: a table, two chisels, a sack, a chest, an iron pot) two chisels put on a table told together ("On the table lie two chisels.", seen.c\'s alike: they were one sentence twice, said once); a look on the table, a look at it (what anyone sees of it, no appraisal: "The table is at your feet.", then "On it lie two chisels."), into the sack (the rations put in it), inside the pouch, into the pot (empty), the world\'s IO_INSIDE; the sack tied shut (the catalogue\'s KF_TIE, struct item\'s shut: IS_CLOSED), nothing seen in it, put in or taken out ("The sack is closed."), its contents not listed in what\'s carried, untied; those on one square in one sentence ("At your feet lie a table, a sack and a chest."); the kit\'s chest open, closed, examined closed ("It\'s closed."), its lock no key to, not locked to pick ("pick the lock of the chest"), forced open (one not locked, forced, opened); the flask of oil unstoppered (shown so in what\'s carried) and stoppered; a torch that doesn\'t open; then at seed 3\'s start (the kitchen) its well looked into and its iron pot (found closed, its lid: walked over to first) opened and looked into',
      init: 't_rc', cycles: 2200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'table,chisel,chisel,sack,chest,iron pot\'', 'drop table', 'put the chisel on the table',
        'put the chisel on the table', 'look', 'look on the table', 'look at the table', 'put the rations in the sack', 'look in the sack',
        'look inside the pouch', 'look in the pot', 'close the sack', 'look in the sack', 'put the dagger in the sack', 'take the rations from the sack',
        'i', 'open the sack', 'drop sack', 'drop chest', 'look', 'close the chest', 'examine the chest', 'lock the chest', 'pick the lock of the chest', 'force the chest',
        'open the flask of oil', 'i', 'close the flask', 'open the torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm', 'look into the well', 'look in the pot', 'open the pot', 'look in the pot', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const { IS_CLOSED, IS_LOCK } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's IS_*)
        const thing = (st, n) => st && (dq0Carried(st).find(i => i.name === n) || st.items.find(i => i.name === n && i.sn === st.pc.sn));
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k \'table,chisel,chisel,sack,chest,iron pot\'');
        if (!g.after) return ['the first game not begun'];
        const lk = T.of('look'), tbl = lk.after && lk.after.items.find(i => i.name === 'table');
        const on = lk.after && tbl ? lk.after.items.filter(i => i.name === 'chisel' && i.where === 'IW_ITEM' && i.x === tbl.n) : [];
        if (on.length !== 2) f.push('not two chisels on the table, each its own: ' + on.length);
        const seen = lk.after && lk.after.sight ? lk.after.sight.items.filter(i => i.name === 'chisel') : [];
        if (seen.length !== 2) f.push('the senses\' record not both chisels on the table: ' + JSON.stringify(seen));
        for (const [l, kind] of [['look on the table', 'IO_INSIDE'], ['look in the sack', 'IO_INSIDE'], ['look inside the pouch', 'IO_INSIDE'],
          ['look in the pot', 'IO_INSIDE'], ['close the sack', 'IO_SHUT'], ['open the sack', 'IO_SHUT'], ['close the chest', 'IO_SHUT'],
          ['force the chest', 'IO_SHUT'], ['open the flask of oil', 'IO_SHUT'], ['close the flask', 'IO_SHUT']]) {
          if (!dq0Rep(T.of(l), 'ireq', { kind, result: 'IR_OK' }).length) f.push(l + ': no ' + kind + ' done');
        }
        const cs = T.of('close the sack'), os = T.of('open the sack'), cc = T.of('close the chest'), oc = T.of('force the chest');
        const sk = thing(cs.after, 'sack'), sk2 = thing(os.after, 'sack');
        if (!sk || !(sk.shut & IS_CLOSED)) f.push('the sack not tied shut: ' + JSON.stringify(sk && sk.shut));
        if (!sk2 || (sk2.shut & IS_CLOSED)) f.push('the sack not untied');
        const ch = thing(g.after, 'chest'), ch1 = thing(cc.after, 'chest'), ch2 = thing(oc.after, 'chest');
        if (!ch || (ch.shut & IS_CLOSED) || !(ch.shut & IS_LOCK)) f.push('the kit\'s chest not open, with a lock: ' + JSON.stringify(ch && ch.shut));
        if (!ch1 || !(ch1.shut & IS_CLOSED) || !ch2 || (ch2.shut & IS_CLOSED)) f.push('the chest not closed and opened again');
        const fl = thing(g.after, 'flask of oil'), ws = thing(g.after, 'waterskin'), pk = thing(g.after, 'backpack');
        if (!fl || !(fl.shut & IS_CLOSED) || !ws || !(ws.shut & IS_CLOSED) || !pk || (pk.shut & IS_CLOSED)) f.push('the kit\'s flask and waterskin not stoppered, its pack not open');
        for (const l of ['put the dagger in the sack', 'take the rations from the sack']) {
          if (!dq0Rep(T.of(l), 'ireq', { result: 'IR_NO' }).length) f.push(l + ': not refused, the sack closed');
        }
        const rations = st => st && st.items.find(i => i.name === 'rations');
        const r1 = rations(T.of('take the rations from the sack').after), sack1 = thing(T.of('take the rations from the sack').after, 'sack');
        if (!r1 || !sack1 || r1.where !== 'IW_ITEM' || r1.x !== sack1.n) f.push('the rations taken out of the closed sack');
        inTurn(s, ['> look', 'At your feet lies a table. On the table lie two chisels.', '> look on the table', 'On the table lie two chisels.',
          '> look at the table', 'The table is at your feet.', 'On it lie two chisels.', '> look in the sack', 'In the sack lie 3 rations.',
          '> look inside the pouch', 'In the pouch lie 12 pennies.', '> look in the pot', 'The iron pot is empty.', '> close the sack',
          'You tie the sack shut.', '> look in the sack', 'The sack is closed.', '> put the dagger in the sack', 'The sack is closed.',
          '> take the rations from the sack', 'The sack is closed.', '> i', 'a sack (closed)', '> open the sack', 'You untie the sack, revealing 3 rations.', '> look',
          'At your feet lie a table, a sack and a chest. On the table lie two chisels.', '> close the chest', 'You close the chest.',
          '> examine the chest', 'It\'s closed.', 'It\'s made of wood', '> lock the chest', 'The chest has a lock, but you have no key to it.',
          '> pick the lock of the chest', 'The chest isn\'t locked.', '> force the chest', 'You open the chest: it\'s empty.', '> open the flask of oil',
          'You unstopper the flask of oil: it\'s full.', '> i', 'a flask of oil (unstoppered)',
          '> close the flask', 'You stopper the flask of oil.', '> open the torch', 'You can\'t open the torch.'], f, 'the words: ');
        const inv = s.slice(s.indexOf('> i', s.indexOf('> take the rations from the sack')), s.indexOf('> open the sack'));
        if (/sack \(closed\) \(in it/.test(inv) || /in it: 3 rations/.test(inv)) f.push('what\'s in the closed sack listed: ' + inv);
        if (/On the table lies a chisel/.test(s)) f.push('the chisels on the table told as one');
        // The kitchen: its well, its pot found closed (walked over to each first)
        const g2 = T.of('dq0 -p t -s 3 -c /ram/c.hm');
        const lp = T.after(g2, 'look in the pot'), op = T.after(g2, 'open the pot'), lp2 = T.after(op, 'look in the pot');
        const pot1 = op.after && op.after.items.find(i => i.name === 'iron pot' && i.sn === 0);
        if (!lp.after || !lp.after.items.find(i => i.name === 'iron pot' && i.sn === 0 && (i.shut & IS_CLOSED))) f.push('the kitchen\'s pot not found closed');
        if (!pot1 || (pot1.shut & IS_CLOSED)) f.push('the kitchen\'s pot not opened');
        inTurn(s, ['> look into the well', 'The well\'s shaft drops away into the dark.', '> look in the pot', 'The iron pot is closed.', '> open the pot',
          'You open the iron pot: it\'s empty.', '> look in the pot', 'The iron pot is empty.'], f, 'the kitchen\'s words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0compart', near: ['gear', 'parser'], what: 'Dingus Quest Zero\'s compartments (COMMANDS-CRITIQUE.md\'s step 6b, 2026-11-15: DECISIONS.md\'s "Compartments: containers and surfaces"): the catalogue\'s compartment table, a chest two, its inside and its lid\'s top; at seed 3 (dq0 -T, a kit of a chest, two chisels and a sack more) the chest set down, a chisel put on its open lid refused (the user\'s, 2026-11-17: "The lid is open: nothing would stay on it."); the sack put in it; the chest closed, the chisel put on it (its record in the chest, its y its compartment, IC_MARK | CP_ON << 4 | 1), seen on it in a look ("On the chest lies a chisel."), looked on, looked in (closed); the chisel on it (examined: "On it lies a chisel.", searched: "On the chest you find a chisel."), taken from it, the sack not ("The chest is closed."), put back on it and the other with it ("On the chest lie two chisels."), one taken off it; the chest opened, revealing the sack alone (not what\'s on it), the chisel on its lid sliding off to the floor as it\'s opened (the user\'s, 2026-11-16), looked at, looked on (nothing there now), searched (its inside)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,chisel,chisel,sack\'', 'drop chest', 'put the chisel on the chest', 'put the sack in the chest', 'close the chest',
        'put the chisel on the chest', 'look', 'look on the chest', 'look in the chest', 'examine the chest',
        'search the chest', 'take the chisel from the chest', 'take the sack from the chest', 'put the chisel on the chest', 'put the chisel on the chest',
        'look', 'take the chisel off the chest', 'open the chest', 'look at the chest', 'look on the chest', 'search the chest', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const { IC_MARK, CP_ON } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        const p = T.after(T.of('close the chest'), 'put the chisel on the chest'), st = p.after;
        const chest = st && st.items.find(i => i.name === 'chest' && i.where === 'IW_SQUARE');
        const on = st && chest ? st.items.filter(i => i.name === 'chisel' && i.where === 'IW_ITEM' && i.x === chest.n) : [];
        if (on.length !== 1 || on[0].y !== (IC_MARK | CP_ON << 4 | 1)) f.push('the chisel not on the chest\'s lid, its compartment 1: ' + JSON.stringify(on));
        const sk = T.of('put the sack in the chest').after, sack = sk && chest && sk.items.find(i => i.name === 'sack' && i.x === chest.n);
        if (!sack || sack.y >= IC_MARK) f.push('the sack not in the chest, its compartment 0: ' + JSON.stringify(sack));
        inTurn(s, ['> put the chisel on the chest', 'The lid is open: nothing would stay on it.', '> put the sack in the chest', 'You put the sack in the chest.',
          '> close the chest', 'You close the chest.', '> put the chisel on the chest', 'You put the chisel on the chest.', '> look',
          'At your feet lies a chest. On the chest lies a chisel.', '> look on the chest', 'On the chest lies a chisel.',
          '> look in the chest', 'The chest is closed.', '> examine the chest', 'It\'s closed.', 'On it lies a chisel.', '> search the chest',
          'It\'s closed: you go over the outside of it. On the chest you find a chisel.', '> take the chisel from the chest',
          'You take the chisel from the chest.', '> take the sack from the chest', 'The chest is closed.', '> put the chisel on the chest',
          'You put the chisel on the chest.', '> put the chisel on the chest', 'You put the chisel on the chest.', '> look', 'On the chest lie two chisels.',
          '> take the chisel off the chest', 'You take the chisel from the chest.', '> open the chest', 'You open the chest, revealing a sack. From the lid a chisel slides onto the floor.',
          '> look at the chest', 'In it lies a sack.', '> look on the chest', 'There\'s nothing on the chest.', '> search the chest',
          'In the chest you find a sack.'], f, 'the words: ');
        if (/revealing a sack and a chisel|revealing a chisel/.test(s)) f.push('the chest opened told what\'s on its lid');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0compread', near: ['gear', 'parser'], what: 'Dingus Quest Zero\'s readers of a thing\'s place by its compartment (the user\'s, 2026-11-16: DECISIONS.md\'s "Compartments: the second stage"): at seed 3 (dq0 -T, a kit of a chest, a chisel and a sack) the carried chest closed and a chisel put on its lid: the inventory lists the chisel under it still (on its lid, open to the hand; what\'s inside a closed one not told); the chest set down, the chisel taken from its lid by its name alone (the chooser: a thing on a chest\'s lid lies there as on a table); the chest opened, the chisel on its lid again: "the empty chest" is that one (nothing in it, its lid\'s top not its inside: the chooser\'s state), looked in ("The chest is empty."); the sack put in it, and "the empty chest" none',
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,chisel,sack\'', 'close the chest', 'put the chisel on the chest', 'inventory', 'drop the chest', 'take the chisel',
        'open the chest', 'put the chisel on the chest', 'look in the empty chest', 'put the sack in the chest', 'look in the empty chest', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const inv = T.of('inventory').after, chest = inv && inv.items.find(i => i.name === 'chest'), ch = inv && inv.items.find(i => i.name === 'chisel');
        if (!chest || !ch || ch.where !== 'IW_ITEM' || ch.x !== chest.n) f.push('the chisel not on the carried chest: ' + JSON.stringify(ch));
        if (!/\(j\) a chest \(closed\), in both hands\s+\(k\) a chisel/.test(s)) f.push('the inventory not listing the chisel on the closed chest\'s lid');
        const tk = T.of('take the chisel').after;
        if (!tk || !dq0Carried(tk).some(i => i.name === 'chisel')) f.push('the chisel not taken from the lid by its name');
        inTurn(s, ['> look in the empty chest', 'The chest is empty.', '> put the sack in the chest', 'You put the sack in the chest.', '> look in the empty chest',
          'You see no chest here.'], f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0lidslide', near: ['gear'], what: 'Dingus Quest Zero\'s things on a chest\'s lid slide off as it\'s opened (the user\'s, 2026-11-16: DECISIONS.md\'s "Compartments: the second stage"), told as they fall to the floor: at seed 3 (dq0 -T, a kit of a chest, a chisel and a dagger) the chest set down and closed, the chisel and the dagger put on its lid (its top\'s room by its size: items.h\'s LID_BULK, a chest\'s 20 pints); opened: "You open the chest: it\'s empty. From the lid a dagger and a chisel slide onto the floor.", both lying on its square now (IW_SQUARE), nothing on the lid; the chest closed, the chisel put back on it alone, the chest opened again: "From the lid a chisel slides onto the floor."',
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,chisel,dagger\'', 'drop the chest', 'close the chest', 'put the chisel on the chest', 'put the dagger on the chest',
        'open the chest', 'look on the chest', 'take the chisel', 'close the chest', 'put the chisel on the chest', 'open the chest', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const o = T.of('open the chest').after, chest = o && o.items.find(i => i.name === 'chest' && i.where === 'IW_SQUARE');
        for (const n of ['chisel', 'dagger']) {
          const t = o && o.items.filter(i => i.name === n && i.id && (i.where === 'IW_SQUARE' || (i.where === 'IW_ITEM' && chest && i.x === chest.n)));
          if (!t || !t.length || t.some(i => i.where !== 'IW_SQUARE' || i.x !== chest.x || i.y !== chest.y)) f.push('the ' + n + ' not slid off onto the chest\'s square: ' + JSON.stringify(t));
        }
        inTurn(s, ['> open the chest', 'You open the chest: it\'s empty. From the lid a dagger and a chisel slide onto the floor.', '> look on the chest',
          'There\'s nothing on the chest.', '> close the chest', 'You close the chest.', '> put the chisel on the chest', 'You put the chisel on the chest.',
          '> open the chest', 'You open the chest: it\'s empty. From the lid a chisel slides onto the floor.'], f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0comppocket', near: ['gear', 'parser'], what: 'Dingus Quest Zero\'s backpack\'s side pocket and a compartment named by its own name (the user\'s, 2026-11-16: DECISIONS.md\'s "Compartments: the second stage"): at seed 3 (dq0 -T, a chisel more) the chisel put in the side pocket by its name (the one name match: the catalogue\'s comps\' noun, the chooser\'s; its record in the backpack, its y IC_MARK | CP_IN << 4 | 1), looked in ("In the side pocket lies a chisel."), tied shut, the chisel not taken from it closed ("The side pocket is closed."), untied (what it shows told), the chisel taken out; "take the pocket" not a thing ("You see no pocket here."); the backpack examined, its inside\'s things alone',
      init: 't_rc', cycles: 2000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k chisel', 'put the chisel in the side pocket', 'look in the pocket', 'close the pocket', 'take the chisel from the pocket',
        'open the pocket', 'take the chisel from the pocket', 'take the pocket', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const { IC_MARK, CP_IN } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        const st = T.of('put the chisel in the side pocket').after, pk = st && st.items.find(i => i.name === 'backpack');
        const ch = st && pk && st.items.find(i => i.name === 'chisel' && i.where === 'IW_ITEM' && i.x === pk.n);
        if (!ch || ch.y !== (IC_MARK | CP_IN << 4 | 1)) f.push('the chisel not in the side pocket, the backpack\'s compartment 1: ' + JSON.stringify(ch));
        inTurn(s, ['> put the chisel in the side pocket', 'You stow the chisel in your side pocket.', '> look in the pocket', 'In the side pocket lies a chisel.',
          '> close the pocket', 'You tie the side pocket shut.', '> take the chisel from the pocket', 'The side pocket is closed.', '> open the pocket',
          'You untie the side pocket, revealing a chisel.', '> take the chisel from the pocket', 'You take the chisel in your right hand.', '> take the pocket',
          'You see no pocket here.'], f, 'the words: ');
        const tk = T.of('open the pocket'), t2 = T.after(tk, 'take the chisel from the pocket').after;
        if (!t2 || !dq0Carried(t2).some(i => i.name === 'chisel' && i.where === 'IW_ACTOR')) f.push('the chisel not taken out of the side pocket');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0sidepocket', near: ['gear', 'parser'], what: 'Dingus Quest Zero\'s side pocket named whole, and a look into a side of the room (2026-11-16: DECISIONS.md\'s "A look in from an overlay, the side pocket named whole"): at seed 3 (dq0 -T, a chisel more) "the side pocket" typed whole (the parser\'s "side" a side of the room, OB_SIDE, its word "pocket": the resolver\'s, the backpack\'s compartment named by its own name, a compartment\'s noun before a way) put into, looked in ("In the side pocket lies a chisel."), tied shut ("You tie the side pocket shut."), looked in closed, untied, the chisel taken from it; "look in the far side" (the look one way asked from overlay 1\'s code, hthings.c\'s H_LOOK_IN: the hub\'s bank left in the window ended the game, its words the world\'s raw text) a look about, the game going on to its look and its quit',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k chisel', 'put the chisel in the side pocket', 'look in the side pocket', 'close the side pocket',
        'look in the side pocket', 'open the side pocket', 'take the chisel from the side pocket', 'look in the far side', 'look in the side pocket', 'look',
        'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const { IC_MARK, CP_IN } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        const st = T.of('put the chisel in the side pocket').after, pk = st && st.items.find(i => i.name === 'backpack');
        const ch = st && pk && st.items.find(i => i.name === 'chisel' && i.where === 'IW_ITEM' && i.x === pk.n);
        if (!ch || ch.y !== (IC_MARK | CP_IN << 4 | 1)) f.push('the chisel not in the side pocket, the backpack\'s compartment 1: ' + JSON.stringify(ch));
        inTurn(s, ['> put the chisel in the side pocket', 'You stow the chisel in your side pocket.', '> look in the side pocket', 'In the side pocket lies a chisel.',
          '> close the side pocket', 'You tie the side pocket shut.', '> look in the side pocket', 'The side pocket is closed.', '> open the side pocket',
          'You untie the side pocket, revealing a chisel.', '> take the chisel from the side pocket', 'You take the chisel in your right hand.',
          '> look in the far side', 'You can see part of a cell', '> look in the side pocket', 'The side pocket is empty.', '> look', 'You can see part of a cell'], f, 'the words: ');
        const t2 = T.of('take the chisel from the side pocket').after;
        if (!t2 || !dq0Carried(t2).some(i => i.name === 'chisel' && i.where === 'IW_ACTOR')) f.push('the chisel not taken out of the side pocket');
        const g = s.slice(s.indexOf('> look in the far side'));
        if (/<[a-z]+\d/.test(g)) f.push('the world\'s raw text told: ' + (/<[^>]*>[^ ]*/.exec(g) || [''])[0]);
        if (!/> quit[^%]*% echo/.test(g) || /rc: (look|quit): not found/.test(g)) f.push('the game ended before its quit');
        if (/You look (?:for|at) the far/.test(g)) f.push('a side of the room looked into, its word looked for as a thing\'s (the look\'s first words a thing\'s alone, 2026-11-17)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0handorder', reach: true, near: ['gear'], what: 'Dingus Quest Zero\'s hand freed by its home first, the light last (the user\'s, 2026-11-17: DECISIONS.md\'s "The critique\'s questions: the user\'s answers", freeing a hand: "the hand whose thing has a home goes first (a sword to its scabbard, coins to the purse), the light last; a light left burning is picked up when a walk starts, if a hand is free"; the rule of 2026-11-03 kept, only the order chosen): at seed 374 (dq0 -T, a scabbard, a sword and a pick more) the scabbard hung on the belt, a lit torch in the left hand and the sword in the right: the tinderbox held, the sword put in its scabbard, not the torch laid down; the pennies held, then the dagger: the pennies put in the pouch (the purse), one lot, the torch kept; the pick held (both hands): the torch, the only thing in hand, laid down lit; a step into the wall refused, the torch still there; a walk begun, no hand free: "You leave the torch behind."; in a second game the torch laid down for the pick, the pick stowed, a walk begun: "You take up the torch." first, the torch lit in hand',
      init: 't_rc', cycles: 3600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0sky=0', 'dq0hour=128',
        "dq0 -p t -s 374 -c /ram/c.hm -T -k 'scabbard,sword,pick'", 'hang the scabbard on the belt', 'hold the sword', 'hold the tinderbox', 'inventory',
        'drop the tinderbox', 'hold the pennies', 'hold the dagger', 'inventory', 'drop the dagger', 'hold the pick', 'look', 'turn around', 'f', 'look', 'turn around',
        'f', 'quit', "dq0 -p t -s 374 -c /ram/c.hm -T -k pick", 'hold the pick', 'stow the pick', 'f', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        const games = s.split('% dq0 -p t -s 374').slice(1);
        if (games.length !== 2) return ['not two games: ' + games.length];
        const [a, b] = games, seg = (g, l) => { let i = g.indexOf('> ' + l + ' '); if (i < 0) return ''; if (g.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3; const e = g.indexOf('> ', i + l.length + 3); return g.slice(i, e < 0 ? undefined : e); };
        const tb = seg(a, 'hold the tinderbox');
        if (!/You put the sword in the scabbard\./.test(tb) || /lay the torch/.test(tb)) f.push('the tinderbox held: not the sword sheathed first, the torch kept: ' + tb.slice(0, 300));
        const h1 = T.of('hold the tinderbox').after, torch = h1 && dq0Carried(h1).find(i => i.name === 'torch' && (i.flags & DQ0_I.HELD));
        if (!torch || !(torch.flags & DQ0_I.LIT)) f.push('the tinderbox held: the lit torch not still in hand');
        const dg = seg(a, 'hold the dagger');
        if (!/You put the pennies in the pouch\.[^>]*You take the dagger in your right hand\./.test(dg)) f.push('the dagger held: the pennies not put in the purse first: ' + dg.slice(0, 300));
        const h2 = T.of('hold the dagger').after, pouch = h2 && h2.items.find(i => i.name === 'pouch'), coins = h2 ? h2.items.filter(i => i.name === 'pennies' && i.where === 'IW_ITEM' && pouch && i.x === pouch.n) : [];
        if (coins.length !== 1) f.push('the pennies not one lot in the pouch: ' + JSON.stringify(coins));
        if (!/You lay the torch on the ground, still burning\./.test(seg(a, 'hold the pick'))) f.push('the pick held: the torch not laid down lit, the last: ' + seg(a, 'hold the pick').slice(0, 300));
        const segs = (g, l) => { const r = []; for (let i = 0; (i = g.indexOf('> ' + l + ' ', i)) >= 0; ) { if (g.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3; const e = g.indexOf('> ', i + l.length + 3); r.push(g.slice(i, e < 0 ? undefined : e)); if (e < 0) break; i = e; } return r; };
        const fa = segs(a, 'f');
        if (fa.length !== 2 || !/You can't go that way\./.test(fa[0]) || /leave the torch/.test(fa[0])) f.push('a step refused: the torch told left behind: ' + fa[0]);
        if (!/You set off south\.[^>]*You leave the torch behind\./.test(fa[1] || '')) f.push('the walk begun, no hand free: the torch not told left behind: ' + (fa[1] || '').slice(0, 300));
        const w2 = segs(b, 'f')[0] || '';
        if (!/You take up the torch\.[^>]*You set off south\./.test(w2)) f.push('the walk begun, a hand free: the torch not taken up first: ' + w2.slice(0, 300));
        const iv = T.filter(t => t.line === 'inventory').pop(), lit = iv && iv.after && dq0Carried(iv.after).find(i => i.name === 'torch' && (i.flags & DQ0_I.HELD));
        if (!lit || !(lit.flags & DQ0_I.LIT)) f.push('the torch taken up: not lit in hand');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0litclimb', reach: true, near: ['gear'], what: 'Dingus Quest Zero\'s light left burning taken up as any move away starts (the user\'s, 2026-11-17: DECISIONS.md\'s "The one-path plan and phase A\'s questions: the user\'s answers", "Any move away takes up a light the rule left burning: a walk, a climb, going up or down; else \\"You leave the ... behind.\\".  A light the player put down stays"; dq0handorder\'s walks, a climb now too: things.c\'s s_lit_back, its moves I_GO ... I_GATHER): at seed 3 (dq0 -T, a pick more) the pick held, the torch laid down lit for its hands; "d", no hand free: "You leave the torch behind." before the stairway\'s words; a second game the pick held and stowed again, "d": "You take up the torch." first, the torch lit in hand below; a third the torch put down by the player, "d": it stays, neither taken up nor told left behind',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k pick', 'hold the pick', 'd', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T -k pick', 'hold the pick', 'stow the pick', 'd', 'inventory',
        'quit', 'dq0 -p t -s 3 -c /ram/c.hm -T', 'drop the torch', 'd', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        const games = s.split('% dq0 -p t -s 3').slice(1);
        if (games.length !== 3) return ['not three games: ' + games.length];
        const seg = (g, l) => { let i = g.indexOf('> ' + l + ' '); if (i < 0) return ''; if (g.startsWith('> ' + l + ' ', i + l.length + 3)) i += l.length + 3; const e = g.indexOf('> ', i + l.length + 3); return g.slice(i, e < 0 ? undefined : e); };
        const [a, b, c] = games;
        if (!/You lay the torch on the ground, still burning\./.test(seg(a, 'hold the pick'))) f.push('the pick held: the torch not laid down lit: ' + seg(a, 'hold the pick').slice(0, 300));
        // (The stairway's first words "You start down the stairway." at once after the light's, as the climb's taken:
        // the first words at once, 2026-11-17)
        if (!/^> d You leave the torch behind\. You start down the stairway\. You go down the stairway/.test(seg(a, 'd'))) f.push('down the stairs, no hand free: the torch not told left behind first: ' + seg(a, 'd').slice(0, 300));
        if (!/^> d You take up the torch\. You start down the stairway\. You go down the stairway/.test(seg(b, 'd'))) f.push('down the stairs, a hand free: the torch not taken up first: ' + seg(b, 'd').slice(0, 300));
        if (!/a torch \(lit\), in your left hand/.test(seg(b, 'inventory'))) f.push('the torch taken up: not lit in hand below: ' + seg(b, 'inventory').slice(0, 300));
        if (/take up the torch|leave the torch behind/.test(seg(c, 'd'))) f.push('the torch the player put down taken up, or told left behind: ' + seg(c, 'd').slice(0, 300));
        if (/torch \(lit\), in your/.test(seg(c, 'inventory'))) f.push('the torch the player put down in hand below: ' + seg(c, 'inventory').slice(0, 300));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0purse', reach: true, near: ['gear', 'money'], what: 'Dingus Quest Zero\'s coins put in the purse (the user\'s, 2026-11-17: "coins to the purse"): at seed 3 (dq0 -T, a chest more) three pennies dropped and taken up again: into the pouch on the belt (the purse), one lot with the rest, not the backpack nor a hand; held, then stowed: into the pouch, unasked (not "Stow it where?"); some put in the chest and taken from it: into the pouch; a lot of rations (no coins) still to the pack',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        "dq0 -p t -s 3 -c /ram/c.hm -T -k chest", 'drop the torch', 'drop 3 pennies', 'take the pennies', 'inventory', 'hold the pennies', 'stow the pennies', 'inventory',
        'drop the chest', 'put 5 pennies in the chest', 'take the pennies from the chest', 'inventory', 'drop 2 rations', 'take the rations', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        const purse = (t, why) => {
          const st = t.after, pouch = st && st.items.find(i => i.name === 'pouch'), c = st ? st.items.filter(i => i.name === 'pennies' && dq0Carried(st).includes(i)) : [];
          if (c.length !== 1 || c[0].where !== 'IW_ITEM' || !pouch || c[0].x !== pouch.n) f.push(why + ': the pennies not all in the pouch, one lot: ' + JSON.stringify(c));
        };
        purse(T.of('take the pennies'), 'taken up');
        purse(T.of('stow the pennies'), 'stowed');
        purse(T.of('take the pennies from the chest'), 'taken from the chest');
        if (/Stow it where/.test(s)) f.push('the pennies\' place asked');
        inTurn(s, ['> take the pennies', 'Taken.', '> hold the pennies', 'You take the pennies in your right hand.', '> stow the pennies', 'You put the pennies in the pouch.',
          '> put 5 pennies in the chest', 'You put the 5 pennies in the chest.', '> take the pennies from the chest', 'You take the pennies from the chest. They\'re in your pouch.'], f, 'the words: ');
        const r = T.of('take the rations').after, pack = r && r.items.find(i => i.name === 'backpack'), ra = r ? r.items.filter(i => i.name === 'rations' && dq0Carried(r).includes(i)) : [];
        if (!ra.length || ra.some(i => i.where !== 'IW_ITEM' || !pack || i.x !== pack.n)) f.push('the rations not to the pack: ' + JSON.stringify(ra));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0plurals', reach: true, near: ['gear', 'parser'], what: 'Dingus Quest Zero\'s things\' sentences for a plural name (2026-11-17: the play test\'s "The pennies is there already."; "You\'ve no hand free for the candle stubs: you put it in your backpack."): at seed 3 (dq0 -T, candle stubs and a sword more) the pennies put in the pouch they\'re in: "The pennies are there already." (the SA rows\' pl: the name\'s last letter an s, not two, refuse.c\'s, as the catalogue\'s plural kinds are named); the sword held, the candle stubs put down and taken up again, no hand free: "You\'ve no hand free for the candle stubs: you put them in your backpack." (things.c\'s s_carry, by the same rule)',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        "dq0 -p t -s 3 -c /ram/c.hm -T -k 'candle stubs,sword'", 'put the pennies in the pouch', 'hold the sword', 'drop the candle stubs', 'take the candle stubs', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        inTurn(s, ['> put the pennies in the pouch', 'The pennies are there already.', '> take the candle stubs',
          'You\'ve no hand free for the candle stubs: you put them in your backpack.'], f, 'the words: ');
        if (/pennies is there|candle stubs: you put it in/.test(s)) f.push('a plural said as one: ' + (/[^.]*(pennies is there|candle stubs: you put it in)[^.]*/.exec(s) || [''])[0]);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0keypack', reach: true, near: ['gear'], what: 'Dingus Quest Zero\'s key found in the pack the reach set down (built/2026-11-17-parse.md\'s noticed: "At the full reach, an overloaded hunter\'s \\"unlock the chest\\" (its own key in the worn pack, the pack set down for the reach) says \\"You have nothing to pick its lock with.\\""): at seed 3 (dq0 -T, dq0parsefix\'s kit: a chest and its key, a rope, a case, a chisel, a table, a dagger) the chest put down, "unlock the chest": the hands freed, the pack taken off and set down, the key got out of it (things.c\'s key_fits: what the character carries, and the pack it set down for this handling, rs_pack), "You unlock the chest with its key.", the chest unlocked; never "nothing to pick its lock with"',
      init: 't_rc', cycles: 1800e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        "dq0 -p t -s 3 -c /ram/c.hm -T -k 'chest,key,rope,case,chisel,table,dagger'", 'drop the chest', 'unlock the chest', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        inTurn(s, ['> unlock the chest', 'You take off the backpack', 'You get out the iron key.', 'You unlock the chest with its key.',
          'You keep the iron key in hand.'], f, 'the words: ');
        if (/You put away the iron key\./.test(s)) f.push('the key told put away, the pack (packed past its room) with none for it (H9, 2026-11-18)');
        const iv = T.of('inventory').after, key = iv && iv.items.find(i => i.name === 'iron key');
        if (!key || !(key.flags & DQ0_I.HELD)) f.push('the key not kept in hand: ' + JSON.stringify(key));
        if (/nothing to pick its lock with/.test(s)) f.push('the key in the pack set down not found: "nothing to pick its lock with" said');
        const u = T.of('unlock the chest').after, chest = u && u.items.find(i => i.name === 'chest');
        if (!chest || (chest.shut & 2)) f.push('the chest still locked: ' + JSON.stringify(chest));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0compself', near: ['gear'], what: 'Dingus Quest Zero\'s compartment looked at describes itself, and a thing put on an open lid refused (the user\'s, 2026-11-17: DECISIONS.md\'s "The critique\'s questions: the user\'s answers", the containers\' open questions): at seed 3 (dq0 -T, a kit of a desk, a key after it and a chisel) the desk set down: "look at the drawer" its drawer, not the desk ("The drawer is closed and locked."), unlocked and opened ("The drawer is open. It\'s empty."), the chisel put in it ("The drawer is open. In it lies a chisel."), closed, examined ("x drawer": "The drawer is closed."); the backpack\'s side pocket looked at ("The side pocket is untied. It\'s empty."), tied ("The side pocket is tied shut."); a second game (a chest and a dagger) the chest set down open: the dagger put on it refused ("The lid is open: nothing would stay on it.", the dagger not moved), its lid looked at ("The lid is open. There\'s nothing on it."); the chest closed, the dagger put on it, its lid looked at ("The lid is closed. On it lies a dagger.")',
      init: 't_rc', cycles: 3200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'desk,key,chisel\'', 'drop the desk', 'drop the torch', 'look at the drawer', 'unlock the drawer', 'open the drawer',
        'look at the drawer', 'put the chisel in the drawer', 'look at the drawer', 'close the drawer', 'x drawer', 'look at the pocket', 'close the pocket',
        'look at the pocket', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,dagger\'', 'drop the chest', 'drop the torch', 'put the dagger on the chest', 'look at the lid', 'close the chest',
        'put the dagger on the chest', 'look at the lid', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        inTurn(s, ['> look at the drawer', 'The drawer is closed and locked.', '> unlock the drawer', 'You unlock the drawer with its key.', '> open the drawer',
          '> look at the drawer', 'The drawer is open. It\'s empty.', '> put the chisel in the drawer', 'You put the chisel in the drawer.', '> look at the drawer',
          'The drawer is open. In it lies a chisel.', '> close the drawer', 'You close the drawer.', '> x drawer', 'The drawer is closed.', '> look at the pocket',
          'The side pocket is untied. It\'s empty.', '> close the pocket', 'You tie the side pocket shut.', '> look at the pocket', 'The side pocket is tied shut.',
          '> put the dagger on the chest', 'The lid is open: nothing would stay on it.', '> look at the lid', 'The lid is open. There\'s nothing on it.',
          '> close the chest', 'You close the chest.', '> put the dagger on the chest', 'You put the dagger on the chest.', '> look at the lid',
          'The lid is closed. On it lies a dagger.'], f, 'the words: ');
        if (/The desk is (?:at your feet|beside you)/.test(s.slice(s.indexOf('> look at the drawer'), s.indexOf('> look at the pocket')))) f.push('a drawer looked at told its desk');
        if (/> look at the (?:drawer|pocket|lid) You look for/.test(s)) f.push('a part of a thing looked at, looked for first (the look\'s first words, 2026-11-17)');
        const r = T.of('put the dagger on the chest').after, d = r && dq0Thing(r, 'dagger');
        if (!d || !dq0Carried(r).includes(d)) f.push('the dagger refused the open lid: not still carried: ' + JSON.stringify(d));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stowpocket', reach: true, near: ['gear', 'parser'], what: 'Dingus Quest Zero\'s side pocket offered as a place of its own when a thing\'s stowed (the user\'s, 2026-11-17: "Where to stow offers a pack\'s side pocket as a place of its own"): at seed 374 (dq0 -T) the tinderbox stowed: asked ("Stow it where? (b) the backpack, the side pocket, (f) the pouch"), "pocket" answering it (into the side pocket, the backpack\'s compartment 1: its record\'s y IC_MARK | CP_IN << 4 | 1); the flask of oil stowed, "side pocket" answering it; the side pocket looked in, both there; the side pocket tied: not offered (the rations stowed)',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0sky=0', 'dq0hour=128', 'dq0 -p t -s 374 -c /ram/c.hm -T', 'stow the tinderbox', 'pocket', 'stow the flask', 'side pocket', 'look in the side pocket',
        'close the pocket', 'hold the rations', 'stow the rations', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' ');
        const { IC_MARK, CP_IN } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        inTurn(s, ['> stow the tinderbox', 'Stow it where? (b) the backpack, the side pocket, (f) the pouch', '> pocket', 'You stow the tinderbox in your side pocket.',
          '> stow the flask', 'Stow it where?', 'the side pocket', '> side pocket', 'You stow the flask of oil in your side pocket.', '> look in the side pocket',
          'In the side pocket lie a tinderbox and a flask of oil.', '> close the pocket', 'You tie the side pocket shut.'], f, 'the words: ');
        const st = T.of('look in the side pocket').after, pk = st && st.items.find(i => i.name === 'backpack');
        for (const n of ['tinderbox', 'flask of oil']) {
          const t = st && pk && st.items.find(i => i.name === n && i.where === 'IW_ITEM' && i.x === pk.n);
          if (!t || t.y !== (IC_MARK | CP_IN << 4 | 1)) f.push('the ' + n + ' not in the side pocket: ' + JSON.stringify(t));
        }
        const ch = s.slice(s.indexOf('> stow the rations'));
        if (/side pocket/.test(ch.slice(0, ch.indexOf('> ', 4)))) f.push('the tied side pocket offered: ' + ch.slice(0, 200));
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0compdesk', near: ['gear'], what: 'Dingus Quest Zero\'s desk (furniture, the user\'s, 2026-11-16: DECISIONS.md\'s "Compartments: the second stage"): a lock on a compartment past the first and a hidden one, by the words and the records; at seed 3 (dq0 -T, a kit of a desk, a key after it, a chisel: the key its drawer\'s, the drawer closed and locked, the kit\'s rule) the desk set down: "open the drawer" refused ("The drawer is locked."), unlocked with its key, opened, the chisel put in it and looked in, closed and locked again (its holder\'s fuel\'s CS_LOCKED, its shut\'s closed bit); its secret drawer unnamed till found ("You see no secret drawer here.": its name whole, the parser\'s; "the secret niche" till then: the user\'s, 2026-11-17), found by a search (the referee\'s test: "You find a secret drawer in the desk."), a dagger put in it, looked in, found again by a search ("In the secret drawer you find a dagger."); "the drawer" still the one that locks; "take all from the desk" takes the dagger from the secret drawer once it\'s found (the user\'s, 2026-11-17)',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'desk,key,chisel\'', 'drop the desk', 'drop the torch', 'open the drawer', 'unlock the drawer', 'open the drawer',
        'put the chisel in the drawer', 'look in the drawer', 'close the drawer', 'lock the drawer', 'open the drawer', 'look in the secret drawer', 'search the desk',
        'put the dagger in the secret drawer', 'look in the secret drawer', 'search the desk', 'open the drawer', 'take all from the desk', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const { CS_LOCKED, IS_SHUT_C1 } = Object.assign({ IS_SHUT_C1: 0x20 }, require('../../../games/dq0/hubgen.js').load().C);   // (items.h's)
        const st = T.of('lock the drawer').after, dk = st && st.items.find(i => i.name === 'desk');
        if (!dk || !(dk.fuel & CS_LOCKED) || !(dk.shut & IS_SHUT_C1)) f.push('the desk\'s drawer not closed and locked: ' + JSON.stringify(dk));
        inTurn(s, ['> open the drawer', 'The drawer is locked.', '> unlock the drawer', 'You unlock the drawer with its key.', '> open the drawer',
          'You open the drawer: it\'s empty.', '> put the chisel in the drawer', 'You put the chisel in the drawer.', '> look in the drawer', 'In the drawer lies a chisel.',
          '> close the drawer', 'You close the drawer.', '> lock the drawer', 'You lock the drawer with its key.', '> open the drawer', 'The drawer is locked.',
          '> look in the secret drawer', 'You see no secret drawer here.', '> search the desk', 'You find a secret drawer in the desk.',
          '> put the dagger in the secret drawer', 'You put the dagger in the secret drawer.', '> look in the secret drawer',
          'In the secret drawer lies a dagger.', '> search the desk', 'In the secret drawer you find a dagger.', '> open the drawer',
          'The drawer is locked.', '> take all from the desk', 'You take a dagger from the desk.'], f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0compcase', near: ['gear'], what: 'Dingus Quest Zero\'s case (furniture, the user\'s, 2026-11-16: DECISIONS.md\'s "Compartments: the second stage"): a lid\'s top by its holder\'s size and a hidden compartment, by the words and the records; at seed 3 (dq0 -T, a kit of a case, a chest, a rope and a chisel) the case set down and closed: the rope too big for its lid\'s top (5 pints, a quarter of its 20: items.h\'s LID_BULK), the chisel put there; the case opened, the chisel sliding off its lid; searched till its false bottom\'s found (the referee\'s search test, a hidden compartment\'s: "You find a false bottom in the case.", its record\'s CS_FOUND), after that known; the case closed, the false bottom closed with it (reached through its inside: "The case is closed."), the case opened and the false bottom still closed; the rope put on the chest\'s lid (20 pints, the chest\'s 80)',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'case,chest,rope,chisel\'', 'drop the case', 'drop the torch', 'close the case', 'put the rope on the case', 'put the chisel on the case',
        'open the case', 'search the case', 'search the case', 'search the case', 'close the case', 'open the bottom', 'open the case',
        'look in the bottom', 'close the chest', 'put the rope on the chest', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, '');
        const { CS_FOUND } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        const st = T.of('look in the bottom').after, cs = st && st.items.find(i => i.name === 'case');
        if (!cs || !(cs.fuel >> 4 & CS_FOUND)) f.push('the case\'s false bottom not found, its record\'s: ' + JSON.stringify(cs));
        inTurn(s, ['> close the case', 'You close the case.', '> put the rope on the case', 'There\'s no room there for the rope.', '> put the chisel on the case',
          'You put the chisel on the case.', '> open the case', 'You open the case: it\'s empty. From the lid a chisel slides onto the floor.'], f, 'the words: ');
        if ((s.match(/You find a false bottom in the case\./g) || []).length !== 1) f.push('the false bottom not found once by the searches');
        inTurn(s, ['> close the case', 'You close the case.', '> open the bottom', 'The case is closed.', '> open the case', 'You open the case: it\'s empty.',
          '> look in the bottom', 'The false bottom is closed.', '> close the chest', 'You close the chest.', '> put the rope on the chest', 'You put the rope on the chest.'], f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0furniture', near: ['gear', 'map'], what: 'Dingus Quest Zero\'s furniture placed in the dungeon (the user\'s, 2026-11-16: DECISIONS.md\'s "Compartments: the second stage"): the maker\'s placers\' row (dq0m/place.c\'s o_put_furn, its own dice, DP_FURNITURE) puts a piece in a room whose use has some, beside a wall: at seed 14 the bedchamber the character begins in (quarters: a desk or a case) has a case, seen at once ("On the floor ahead to your right lies a case."), its record the stock\'s (IW_SQUARE, KI_CASE), gone to and searched; the same game with the environment\'s dq0furn=0 (a test\'s scene, as dq0sky\'s) has none, the bedchamber as it was',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 14 -c /ram/c.hm', 'look', 'go to the case', 'open the case', 'search the case', 'quit',
        'dq0furn=0', 'dq0 -p t -s 14 -c /ram/c.hm', 'look', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out).replace(/\([A-Z]{1,2}\) /g, ''), games = s.split('dq0 -p t -s 14').slice(1);
        const { KI_CASE, KI_DESK, KI_CABINET, KI_SHELVES } = require('../../../games/dq0/hubgen.js').load().C;   // (items.h's)
        const furn = st => st ? st.items.filter(i => [KI_CASE, KI_DESK, KI_CABINET, KI_SHELVES].includes(i.kind)) : [];
        const looks = T.filter(t => t.line === 'look');
        const a = furn(looks[0] && looks[0].after), b = furn(looks[1] && looks[1].after);
        if (!a.some(i => i.kind === KI_CASE && i.where === 'IW_SQUARE')) f.push('no case placed at seed 14: ' + JSON.stringify(a));
        if (b.length) f.push('furniture placed with dq0furn=0: ' + JSON.stringify(b));
        if (!games[0] || !/On the floor ahead to your right lies a case\./.test(games[0])) f.push('the case not seen as the game begins');
        if (!games[1] || /a case|a desk|a cabinet|some shelves/.test(games[1])) f.push('furniture seen with dq0furn=0');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0examine', what: 'Dingus Quest Zero\'s things looked at and examined, their aspects asked for, what opening shows, and what a command\'s done with (the user\'s, 2026-10-22: "When examining an item, I want to know about its state ... look [at] item instead of examine should give these things.  Examine gives too many unimportant details (like value)"; "examine value [of] item", "quality [of] item"; "if I open a chest, I should get a \'You open the chest, revealing a pile of gold coins\'"; "hit door with bar hits the door, even if I don\'t have a bar"): at seed 3 (dq0 -T, a kit of a table, two chisels, a sack and a chest more) a look at the table (where, what\'s on it, no appraisal), at the torch (lit), the torch examined (what it\'s made of, its wear, how burnt: no test), the dagger\'s value, its quality, both (the referee\'s appraisal test each, IO_DESCRIBE, only what\'s asked), the backpack hefted (its weight with what\'s in it), the chest looked at closed (with a lock), its lock examined (its make), gone over for traps (none), its condition (the last thing named: "its"); the chest opened (": it\'s empty."), the sack untied (what\'s in it told), the flask unstoppered (how full); a blow with a hammer it hasn\'t got refused ("You have no hammer to hand.", no blow); the door ahead opened (what it shows beyond told with it, the world\'s look after it: "revealing a passage, 30 feet of it"), gone over for traps (the world\'s PA_CHECK: 10 seconds), examined (its plain features, no judgement), judged (its state, the appraisal test), its lock, its condition, its value',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'table,chisel,chisel,sack,chest\'', 'drop table', 'put the chisel on the table',
        'put the chisel on the table', 'look at the table', 'look at the torch', 'examine the torch', 'assess the dagger', 'judge the dagger',
        'appraise the dagger', 'heft the backpack', 'drop chest', 'close the chest', 'look at the chest', 'examine the lock of the chest',
        'check the chest for traps', 'check its condition', 'open the chest', 'put the rations in the sack', 'close the sack', 'look at the sack',
        'look at the rations', 'open the sack', 'open the flask of oil', 'hit the door with the hammer', 'open the door', 'check the door for traps',
        'examine the door', 'judge the door', 'examine the lock of the door', 'condition of the door', 'assess the door', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        for (const l of ['look at the table', 'look at the torch', 'examine the torch', 'heft the backpack', 'look at the chest',
          'examine the lock of the chest', 'check the chest for traps', 'check its condition', 'look at the sack']) {
          if (!dq0Rep(T.of(l), 'ireq', { kind: 'IO_LOOK', result: 'IR_OK' }).length) f.push(l + ': no IO_LOOK done');
          if (dq0Rep(T.of(l), 'ireq', { kind: 'IO_DESCRIBE' }).length) f.push(l + ': appraised (IO_DESCRIBE), no test wanted');
        }
        for (const l of ['assess the dagger', 'judge the dagger', 'appraise the dagger']) {
          if (!dq0Rep(T.of(l), 'ireq', { kind: 'IO_DESCRIBE', result: 'IR_OK' }).length) f.push(l + ': no appraisal (IO_DESCRIBE)');
        }
        for (const l of ['open the chest', 'open the sack', 'open the flask of oil']) {
          if (!dq0Rep(T.of(l), 'ireq', { kind: 'IO_SHUT', result: 'IR_OK' }).length) f.push(l + ': no IO_SHUT done');
        }
        dq0Need(f, T.of('open the door'), 'E_PORTAL', { po: 'PO_OPENED', shows: true }, 'the door not opened, what it shows with it');
        dq0Never(f, T.of('hit the door with the hammer'), 'E_PORTAL', {}, 'a blow struck with a hammer it hasn\'t got');
        dq0Need(f, T.of('check the door for traps'), 'E_TRAP', { to: 'TO_CLEAN' }, 'the door not gone over for traps');
        inTurn(s, ['> look at the table', 'The table is at your feet.', 'On it lie two chisels.', '> look at the torch',
          'Your torch is in your left hand. It\'s lit.', '> examine the torch', 'Your torch is in your left hand. It\'s lit. It\'s made of wood; it\'s',
          '> assess the dagger', 'You judge it worth about', '> judge the dagger', 'You judge it a', 'dagger.', '> appraise the dagger',
          'You judge it', 'it would fetch about', '> heft the backpack', 'Your backpack weighs about', 'with what\'s in it.',
          '> look at the chest', 'The chest is at your feet. It\'s closed, with a lock.', '> examine the lock of the chest', 'The chest has a',
          'lock', '> check the chest for traps', 'The chest shows no sign of a trap', '> check its condition', 'The chest is',
          '> open the chest', 'You open the chest: it\'s empty.', '> look at the sack', 'It\'s tied shut.', '> look at the rations',
          'The sack is closed.', '> open the sack', 'You untie the sack, revealing 3 rations.', '> open the flask of oil',
          'You unstopper the flask of oil: it\'s full.', '> hit the door with the hammer', 'You have no hammer to hand.', '> open the door',
          'You open the door, revealing a passage, 30 feet of it.', '> check the door for traps', 'You go over it carefully, and find no trap.',
          '> examine the door', 'A door of', '> judge the door', 'You judge it', '> examine the lock of the door', '> condition of the door',
          'It shows no damage.', '> assess the door', 'There\'s no price to put on it'], f, 'the words: ');
        for (const l of ['look at the table', 'look at the chest', 'examine the torch', 'examine the door']) {
          const a = s.indexOf('> ' + l), b = s.indexOf('> ', a + 2), w = s.slice(a, b);
          if (/You judge|fetch about|pennies/.test(w)) f.push(l + ': an appraisal in its words: ' + w);
        }
        const od = s.slice(s.indexOf('> open the door'), s.indexOf('> check the door for traps'));
        if ((od.match(/passage, 30 feet/g) || []).length !== 1) f.push('what the door shows told other than once: ' + od);
        // (A thing whose word shares a denizen's first letters, "rations" and "giant rat": its own words, not the rat's
        // seen name, 2026-11-18)
        if (/> look at the rations\s+You look at the giant rat/.test(s)) f.push('the rations looked at told as the giant rat');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0hands', what: 'Dingus Quest Zero\'s two hands (slice 4.1b, the user\'s, 2026-10-21: "is there handedness (right handed versus left versus ambidextrous) in the character profile?  ... There should be a penalty for using a weapon in your off hand unless ambidextrous"), by where each thing is (the item store\'s records: the hand its sn), the hub\'s pc_shand and the words: at seed 3 (dq0 -T, dq0 -k sword,spear: right-handed) the kit\'s torch in its left hand, its sword in its right (a weapon the main hand, a light the other), the spear in the pack; the hands swapped ("You change hands: ..."); the sword refused its right hand, the torch there; the torch put down, the sword put in its other hand; the spear refused both hands, the sword in one; the sword stowed, the spear taken in both hands, looked at ("Your spear is in both hands."); the torch refused its left hand, the spear in both; the spear moved to its left hand, the lit torch taken up into its right; what it carries by hand; a left-handed character (its file\'s med 15) the torch in its right hand, the sword in its left; at seed 5 (dq0 -T, dq0 -k sword: Sword 80, the rats) a rat attacked with the sword in its left hand: the sword moved to its left hand first (its words), struck with (pc_strike the sword, pc_shand its left hand)',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword,spear', 'i', 'swap hands', 'hold the sword in my right hand', 'drop the torch',
        'put the sword in my other hand', 'hold the spear in both hands', 'stow the sword', 'hold the spear in both hands',
        'look at the spear', 'take the lit torch in my left hand', 'hold the spear in my left hand', 'take the lit torch in my right hand',
        'i', 'quit',
        'cp /ram/c.hm /ram/l.hm', 'echo med 15 >> /ram/l.hm', 'dq0 -p t -s 3 -c /ram/l.hm -T -k sword', 'i', 'quit',
        'cp /ram/c.hm /ram/s.hm', 'echo ml swd 80 >> /ram/s.hm', 'dq0 -p t -s 5 -c /ram/s.hm -T -I -k sword', 'd', 'go right',
        'drop the torch', 'attack the rat with the sword in my left hand',
        'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), K = m.dq0.layout().K;
        const H = { L: 1, R: 2, B: 3 }, hn = ['none', 'left', 'right', 'both'];
        const hand = (t, name, h, what) => {
          const i = t && dq0Thing(t.after, name);
          if (!i || !(i.flags & DQ0_I.HELD) || i.where !== 'IW_ACTOR' || (i.sn & 3) !== h) f.push((t ? t.line : '?') + ': the ' + name + ' not in its ' + hn[h] + ' hand (' + (i ? i.where + ' flags ' + i.flags + ' sn ' + i.sn : 'none') + ')' + (what ? ': ' + what : ''));
        };
        const free = (t, name, what) => { const i = t && dq0Thing(t.after, name); if (i && (i.flags & DQ0_I.HELD)) f.push(t.line + ': the ' + name + ' still held (' + what + ')'); };
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k sword,spear');
        hand(g, 'torch', H.L, 'the kit\'s light, its off hand');
        hand(g, 'sword', H.R, 'a weapon, its main hand');
        free(g, 'spear', 'its hands full: in the pack');
        const sw = T.after(g, 'swap hands');
        hand(sw, 'sword', H.L);
        hand(sw, 'torch', H.R);
        hand(T.after(sw, 'hold the sword in my right hand'), 'sword', H.L, 'refused, the torch there');
        const ot = T.after(sw, 'put the sword in my other hand');
        hand(ot, 'sword', H.R);
        free(T.after(ot, 'hold the spear in both hands'), 'spear', 'the sword in one hand');
        const st = T.after(ot, 'stow the sword'), sp = T.after(st, 'hold the spear in both hands');
        hand(sp, 'spear', H.B);
        const lt = T.after(sp, 'take the lit torch in my left hand');
        hand(lt, 'spear', H.B, 'the torch refused');
        const sl = T.after(lt, 'hold the spear in my left hand');
        hand(sl, 'spear', H.L);
        hand(T.after(sl, 'take the lit torch in my right hand'), 'torch', H.R);
        const lg = T.of('dq0 -p t -s 3 -c /ram/l.hm -T -k sword');
        hand(lg, 'torch', H.R, 'left-handed: its off hand the right');
        hand(lg, 'sword', H.L, 'left-handed: its main hand the left');
        const a = T.of('attack the rat with the sword in my left hand');
        if (!dq0Ev(a, 'E_FIGHT', { who: 0 }).length) f.push('the rat not struck at with the sword (' + dq0Evs(a) + ')');
        if (!a.after || a.after.pc.strike !== K.KI_SWORD + 3 || a.after.pc.shand !== H.L) f.push('the blow not the sword in its left hand: pc_strike ' + (a.after && a.after.pc.strike) + ', pc_shand ' + (a.after && a.after.pc.shand));
        inTurn(s, ['> i', 'You carry: (a) a torch (lit), in your left hand', '(j) a sword, in your right hand In all',
          '> swap hands', 'You change hands: the sword in your left, the torch in your right.',
          '> hold the sword in my right hand', 'Your right hand holds the torch.', '> drop the torch', 'You put down the',
          '> put the sword in my other hand', 'You shift the sword to your right hand.',
          '> hold the spear in both hands', 'Your right hand holds the sword.', '> stow the sword', 'You stow the sword in your backpack.',
          '> hold the spear in both hands', 'You take the spear in both hands.', '> look at the spear', 'Your spear is in both hands.',
          '> take the lit torch in my left hand', 'Your left hand holds the spear.',
          '> hold the spear in my left hand', 'You shift the spear to your left hand.',
          '> take the lit torch in my right hand', 'You take the lit torch in your right hand.',
          '> i', 'You carry: (a) a torch (lit), in your right hand', '(k) a spear, in your left hand',
          '> i', 'You carry: (a) a torch (lit), in your right hand', '(j) a sword, in your left hand In all',
          '> attack the rat with the sword in my left hand', 'You lift the sword.', 'You close with the giant rat.', 'You shift the sword to your left hand.', 'You strike at the giant rat with your sword'], f, 'the words: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0handy', reach: true, what: 'Dingus Quest Zero\'s reach, how readily a thing comes to hand (the user\'s, 2026-10-23: "Handy would be readily available, so in a backpack that is being worn is not as readily available as one in a belt pouch.  In a sack, in a backpack is two-containers deep, so should require longer for each level of container.  A backpack on the ground or in hand (not worn) is more readily available ... Searching a worn backpack takes it off first, then if it was on, puts it back on, each taking time.  Just shrugging it off/dropping it before battle should not require time"; HOUSE.md\'s reach, handy tools and things used up), by the clock between one line and the next, the things\' replies (the reach owed in the reply\'s y, tenths) and the words: at seed 3 (dq0 -T, dq0 -k sack: the sack in its main hand) the rations from the worn pack into the sack held (the pack off and on again, 10 seconds, and each level 2: told, "You take off the backpack for it, and put it on again."), the sack stowed in the worn pack, the rations from the sack in it (two deep: 15 seconds), dropped at no time, the pennies from the belt\'s pouch (a quick draw, a second), the worn pack looked in (15 seconds) and searched (its minute: off and on within it), dropped ("drop the pack": no time), the tinderbox taken from it on the ground (a level: 4 seconds), the pack put on ("put on the pack", 5 seconds), its torch put down, the pack taken off and left off ("take off the pack": in both hands), looked in held (5 seconds), put on again; at seed 7 (the ghoul engaging it at once, armoured) a grope in a fight a round a step (the book\'s, Combat 7, a turn): its sword put down first (a hand free: it fumbled it before, a little burdened, till D13\'s free pounds), the dagger drawn from the belt a round, stowed in the worn pack three (off, in, on), the pack dropped, the rations taken from it on the ground a round, each line typed ahead as the wait begins (held, the first taking the fight over as it begins, each after it the fight fought on in between, its round out first), the quit last (the fight taken over by it, not fought to the end): the scene set, the ghoul steady (dq0Poke\'s, at the wait: its mood AM_STEADY, so morale never takes it out of the fight before the lines are done)',
      init: 't_rc', cycles: 3200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sack', 'put the rations in the sack', 'stow the sack', 'take the rations from the sack',
        'drop the rations', 'take the pennies from the pouch', 'drop the pennies', 'look in the backpack', 'search the backpack',
        'drop the pack', 'take the tinderbox from the backpack', 'drop the tinderbox', 'put on the pack', 'drop the torch',
        'take off the pack', 'look in the pack', 'put on the pack', 'quit',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'drop the sword', 'wait']) +
        'hold the dagger\rput the dagger in the backpack\rdrop the backpack\rtake the rations from the backpack\rquit\r' + dq0Typed(['echo $status']) },   // (Put in the pack, said: a stow asks where, the pouch or the pack, the designators' step 7)
      start(m) {
        dq0Poke(m, [{ g: 2, line: 'wait', n: 1, name: 'ghoul', set: { mood: 8 } }]);   // (The ghoul steady: hub.h's AM_STEADY)
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k sack');
        if (g.k < 0) return ['the first game not played'];
        const dt = t => t.at && t.after ? t.after.now - t.at.now : null;
        const rep = (t, kind) => dq0Rep(t, 'ireq', { kind, result: 'IR_OK' })[0];
        const want = (line, kind, time, owed, what) => {
          const t = T.of(line, g.k), r = rep(t, kind);
          if (!r) { f.push(line + ': no ' + kind + ' done (' + JSON.stringify(t.replies.map(e => e.kind + ' ' + e.result)) + ')'); return; }
          if (owed !== undefined && r.y !== owed) f.push(line + ': its reach ' + r.y + ' tenths, not ' + owed + ' (' + what + ')');
          if (time !== undefined && dt(t) !== time) f.push(line + ': ' + dt(t) + ' tenths, not ' + time + ' (' + what + ')');
        };
        // (By the steps since 2026-11-02, dq0packsteps': the time the same, what's owed in the reply none)
        want('put the rations in the sack', 'IO_PUT', 160, undefined, 'from the worn pack, off and on, into the sack held, a level');
        want('stow the sack', 'IO_STOW', 140, undefined, 'into the worn pack');
        want('take the rations from the sack', 'IO_TAKE', 150, undefined, 'from a sack in the worn pack: two deep');
        want('drop the rations', 'IO_DROP', 1, undefined, 'let fall: no time');
        want('take the pennies from the pouch', 'IO_TAKE', 10, undefined, 'the belt\'s pouch: a quick draw');
        want('look in the backpack', 'IO_INSIDE', 150, undefined, 'the worn pack taken off to look in, and put on again');
        want('search the backpack', 'IO_SEEK', 720, undefined, 'searched: off, its level, its minute, on');
        want('drop the pack', 'IO_DROP', 1, undefined, 'a pack worn let fall: no time');
        want('take the tinderbox from the backpack', 'IO_TAKE', 40, undefined, 'from the pack on the ground: a level, no taking off');
        want('put on the pack', 'IO_DON', 50, undefined, 'worn');
        want('take off the pack', 'IO_DOFF', 50, undefined, 'taken off, left off');
        want('look in the pack', 'IO_INSIDE', 50, undefined, 'the pack held: a level');
        const off = T.of('take off the pack', g.k), pk = off.after && dq0Thing(off.after, 'backpack');
        if (!pk || (pk.flags & DQ0_I.WORN) || !(pk.flags & DQ0_I.HELD)) f.push('the pack not left off, in a hand: ' + JSON.stringify(pk));
        const on2 = T.after(T.of('look in the pack', g.k), 'put on the pack'), pk2 = on2.after && dq0Thing(on2.after, 'backpack');
        if (!pk2 || !(pk2.flags & DQ0_I.WORN)) f.push('the pack not put on again: ' + JSON.stringify(pk2));
        inTurn(s, ['> put the rations in the sack', 'You take off the backpack', 'You get out the rations.', 'You hold the sack open.',
          'You put the rations in the sack.', 'You put on the backpack.',
          '> look in the backpack', 'You take off the backpack', 'You hold the backpack open.', 'In the backpack lie', 'You put on the backpack.', '> drop the pack', 'You put down the backpack.',
          '> put on the pack', 'You put on the backpack.', '> take off the pack', 'You take off the backpack.'], f, 'the words: ');
        // A grope in a fight (the referee's round, 100 tenths: HarnMaster's), its reach a round a step
        const g2 = T.of('dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T');
        if (g2.k < 0) return [...f, 'the fight\'s game not played'];
        const w = T.after(g2, 'wait'), late = T.filter(t => t.k >= w.k && t.game === w.game);   // (Typed ahead as it fights: each line
        // entered before it's carried out, its reply in a later turn's)
        const fight = (line, kind, owed, what) => {
          const r = late.map(t => rep(t, kind)).find(Boolean);
          if (!r || r.y !== owed) f.push(line + ', engaged: its reach ' + (r ? r.y : 'none done') + ', not ' + owed + ' (' + what + ')');
        };
        fight('hold the dagger', 'IO_HOLD', 100, 'from the belt: a round');
        // (The worn pack: its steps a round each, dq0packsteps': off, held open, the stowing, on; and from the pack on the ground, a level)
        const steps = late.flatMap(t => dq0Rep(t, 'ireq', { kind: 'IO_STEP' }));
        if (steps.length < 2 || steps.some(r => r.x !== 100)) f.push('engaged, the steps not a round each: ' + JSON.stringify(steps.map(r => [r.qty, r.x])));
        if (!late.some(t => rep(t, 'IO_PUT'))) f.push('the dagger not put in the pack in the fight');
        if (!late.some(t => rep(t, 'IO_TAKE'))) f.push('the rations not taken from the pack on the ground in the fight');
        if (!late.some(t => rep(t, 'IO_DROP'))) f.push('the pack not dropped in the fight');
        const hw = (m.hearts || []).find(h => h.g === 2 && h.line === 'wait');
        if (!hw || !hw.acts.some(a => a.name === 'ghoul' && a.mood === 8)) f.push('the ghoul not set steady');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0housefaults', asks: true, initiative: true, reach: true, bearings: true, outfit: true, near: ['gear', 'map'], what: 'Dingus Quest Zero\'s faults found by HOUSE.md\'s check (2026-11-03), each as it was shown: a journey ended at a shut door told, and nothing written into the chunk in the window (the hub writers\' check); a door round a bend walked up to first, closed, opened and struck; a denizen standing in the door meant said to be in the way, no walk to the wall; a far look\'s words ("is some 10 feet off"); a closed thing searched not held open; a shut door said by "examine the wall", one test of a rock; "sharpen the torch" refused, the torch still lit; "walk normally" and "walk quickly" said in the dark; "stop working" whatever the work; a dropped lit torch no time, and put out a second; a second new torch not the old one at its feet; a candle read by its own life; a mankar named strikes though a better weapon\'s held; thrown hard at a denizen; a lantern filled never put away to free a hand for its flask, the torch laid down (the hands\' rule, the light last but never the thing filled: 2026-11-17)',
      init: 't_rc', cycles: 9000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -o /ram/c.hm',
        'dq0 -p t -s 5 -c /ram/c.hm -a', 's', 's', 'go to the entrance', 'go to the entrance', 'quit',
        'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 1 -c /ram/c.hm -k sword -T -a -I -J -N -K', 'go through the door', 'turn around', 'close the door', 'open the door', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -k sword -T -a -I -J -N -K', 'go through the door', 'turn around', 'kick the door', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -T -a -I -J -N -K', 'go to the door', 'wait', 'go through the door', 'quit',
        'hmchar -q -s 1 -o /ram/c.hm', 'dq0 -p t -s 1 -c /ram/c.hm -a', 'look at the sconce', 'take the sconce', 'quit',
        'hmchar -q -s 3 -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -a', 'search the iron pot', 'sharpen the torch', 'inventory', 'drop the hammer', 'quit',
        'hmchar -q -s 5 -o /ram/c.hm', 'dq0 -p t -s 5 -c /ram/c.hm -a', 's', 's', 'examine the wall', 'quit',
        'hmchar -q -s 3 -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -a', 'examine the wall', 'examine the wall', 'examine the wall', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'put out the torch', 'walk normally', 'status', 'walk quickly', 'status', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'drop the torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'put out the torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'light a new torch', 'light a new torch', 'light a new torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a -k candle', 'examine the candle', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a -k pick', 'dig down', 'quit',
        'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 1 -c /ram/c.hm -k candle -T -a -I -J -N -K', 'wait', 'give the candle to the giant rat', 'quit',
        'dq0 -p t -s 1 -c /ram/c.hm -k mankar -T -a -I -J -N -K', 'throw the mankar hard at the giant rat', 'quit',
        'hmchar -q -s 3 -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -a -k lantern', 'fill the lantern', 'fill the lantern', 'fill the lantern', 'fill the lantern', 'fill the lantern', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -a', 'take the torch from the sconce', 'inventory', 'quit',
        'echo att agl 3 >> /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -a', 'status', 'quit',
        'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 4, line: 'go through the door', n: 1, name: 'giant rat', beside: 'face' }, { g: 15, line: 'give the candle to the giant rat', n: 1, name: 'giant rat', beside: 'face' },
          ...[1, 2, 3, 4, 5].map(n => ({ g: 17, line: 'fill the lantern', n, fuel: { lantern: 10 } }))]);
        dq0OnWords(m, [{ g: 14, line: 'dig down', n: 1, words: 'You dig', text: 'stop working\r' }]);
        // (walk_ahead's mark, sampled as the console's used: a far thing's walk told before the world's asked)
        const ad = parseInt(/^al ([0-9A-F]+) \._said_ahead$/m.exec(fs.readFileSync(path.join(TESTS, '..', 'obj', 'games', 'dq0', 'dq0.lbl'), 'latin1'))[1], 16), c = m.acia, w0 = c.write;
        m.ahead = 0;
        c.write = function (r, v, t) { const h = m.dq0 && m.dq0.bank(0), u = h && m.taskRam[h[12]]; if (u && u[ad]) m.ahead++; return w0.call(c, r, v, t); };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const games = []; for (let i = s.indexOf('% dq0 '); i >= 0; i = s.indexOf('% dq0 ', i + 1)) games.push(i);
        const seg = (g, line, n = 1) => { const e = '> ' + line + ' > ' + line; let i = games[g - 1]; for (let k = 0; k < n && i >= 0; k++) i = s.indexOf(e, i + 1);
          if (i < 0) { f.push('game ' + g + ': "' + line + '" not answered'); return ''; } const j = s.indexOf(' > ', i + e.length); return s.slice(i + e.length, j < 0 ? s.length : j); };
        const has = (g, line, w, n) => { if (!(w.test ? w.test(seg(g, line, n)) : seg(g, line, n).includes(w))) f.push('game ' + g + ', ' + line + ': not told ' + w + ': ' + seg(g, line, n).slice(0, 400)); };
        const never = (g, line, w, n) => { if (seg(g, line, n).includes(w)) f.push('game ' + g + ', ' + line + ': told ' + w); };
        const gk = {}; T.forEach((t, k) => { if (/^dq0 /.test(t.line)) gk[Object.keys(gk).length + 1] = k; });
        const dt = (g, line, tenths, n = 1) => { let k = gk[g], t; for (let i = 0; i < n; i++) { t = T.of(line, k); k = t.k + 1; } const d = t.at && t.after ? t.after.now - t.at.now : null; if (d !== tenths) f.push('game ' + g + ', ' + line + ' (' + n + '): ' + d + ' tenths, not ' + tenths); };
        // (A journey's end at a shut door: told, every time)
        has(1, 'go to the entrance', 'The door is shut: open it first.', 1);
        has(1, 'go to the entrance', 'The door is shut: open it first.', 2);
        // (A door round a bend)
        has(2, 'close the door', 'You head for the door.');
        has(2, 'close the door', 'You close the door.');
        never(2, 'close the door', 'no door that way');
        has(2, 'open the door', 'You open the door');
        has(3, 'kick the door', 'You head for the door.');
        never(3, 'kick the door', 'no door that way');
        // (A denizen in the door meant)
        has(4, 'go through the door', 'is in the way.');
        never(4, 'go through the door', 'You come to the wall');
        // (The words)
        has(5, 'look at the sconce', 'The sconce is some ');
        never(5, 'look at the sconce', ' is is ');
        inTurn(seg(5, 'take the sconce'), ['You go over to the sconce.', 'The sconce is fixed where it stands.'], f, 'a far thing fixed: walked to, then told: ');
        if (!(m.ahead > 0)) f.push('a far thing\'s walk not told before the world was asked (said_ahead never set)');
        never(6, 'search the iron pot', 'You hold the iron pot open.');
        has(6, 'search the iron pot', 'It\'s closed');
        dt(6, 'search the iron pot', 600);
        has(6, 'sharpen the torch', 'can\'t be filled or sharpened');
        has(6, 'inventory', 'a torch (lit');
        dt(6, 'drop the hammer', 1);
        has(7, 'examine the wall', 'That\'s a door');
        never(7, 'examine the wall', 'bare rock');
        const w1 = seg(8, 'examine the wall', 1), w2 = seg(8, 'examine the wall', 2), w3 = seg(8, 'examine the wall', 3);
        if (w1 !== w2 || w2 !== w3) f.push('the same wall looked at again answered otherwise: ' + [w1, w2, w3].join(' | '));
        // (Paces said in the dark)
        never(9, 'walk normally', 'held to a slow walk');
        has(9, 'status', 'a walk', 1);
        has(9, 'status', 'a walk', 2);
        // (A light's change: its handling's time)
        dt(10, 'drop the torch', 1);
        dt(11, 'put out the torch', 10);
        // (A new light: the two spares, then none, the old one at its feet not new)
        has(12, 'light a new torch', 'You light the new torch from the burning one.', 1);
        has(12, 'light a new torch', 'You light the new torch from the burning one.', 2);
        has(12, 'light a new torch', 'You have no other torch.', 3);
        // (A candle by its own life)
        has(13, 'examine the candle', 'hardly burnt');
        never(13, 'examine the candle', 'a quarter burnt down');
        // (Stop working: whatever the work)
        never(14, 'dig down', 'You aren\'t digging');
        // (Food what's eaten: a candle's no gift for a beast, though the appraisal's craft takes it in)
        has(15, 'give the candle to the giant rat', 'has no use for it');
        // (A kind past 127 thrown at a denizen, hard: a throw at it, not at a thing; a named mankar's its own)
        has(16, 'throw the mankar hard at the giant rat', 'You throw your mankar hard at the giant rat');
        // (A fill in a lit room: nothing spilt, every time)
        for (let n = 1; n <= 5; n++) { never(17, 'fill the lantern', 'hands slip', n); never(17, 'fill the lantern', 'oil spills', n); }
        // (The lantern being filled never put away to free a hand for its flask: the lit torch laid down, the hands' rule's
        // light last but never the thing filled, 2026-11-17)
        never(17, 'fill the lantern', 'You stow the lantern', 1);
        has(17, 'fill the lantern', 'You lay the torch on the ground, still burning.', 1);
        // (A sconce's cold torch part burnt, not whole: "nearly spent", its last sixteenth at this stratum's age)
        has(18, 'inventory', 'a torch (nearly spent)');
        // (A Move under 4 walks as Move 4: the status line's feet by the step it walks at)
        has(19, 'status', 'a walk (10 feet a round)');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0fit', fit: true, asks: true, initiative: true, reach: true, bearings: true, outfit: true, near: ['gear'], what: 'Dingus Quest Zero\'s outfit fits the character\'s strength (the brief of 2026-11-03: "An outfit should fit the character\'s strength: leave out the heavy things first"; HOUSE.md\'s an outfit that fits): weak characters and a laden soldier start a little burdened at most, with their clothes, pack, belt, a lit light, a fire-maker, food and water; a whole inventory told past 400 letters (2026-11-03: it was cut off, its weight and all), with a strong one\'s long list',
      init: 't_rc', cycles: 5000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin',
        'hmchar -q -s 102 -u feudal -c thief -o /ram/c.hm', 'dq0 -p t -s 102 -c /ram/c.hm -T -a', 'status', 'inventory', 'quit',
        'hmchar -q -s 103 -u feudal -c thief -o /ram/c.hm', 'dq0 -p t -s 103 -c /ram/c.hm -T -a', 'status', 'inventory', 'quit',
        'hmchar -q -s 102 -u feudal -c miner -o /ram/c.hm', 'dq0 -p t -s 102 -c /ram/c.hm -T -a', 'status', 'inventory', 'quit',
        'hmchar -q -s 100 -u feudal -o /ram/c.hm', 'dq0 -p t -s 100 -c /ram/c.hm -T -a', 'status', 'inventory', 'quit',
        'hmchar -q -s 100 -u feudal -o /ram/c.hm', "dq0 -p t -s 100 -c /ram/c.hm -T -a -k 'sack,rope,candle,chisel,shovel,pick,crowbar,chalk'", 'inventory', 'quit',
        'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], s = dq0Prose(out);
        const enc = [...s.matchAll(/Condition: (.*?)Hunger/g)].map(x => { const e = /encumbrance (\d+)/.exec(x[1]); return e ? +e[1] : 0; });
        if (enc.length !== 4) f.push('status told ' + enc.length + ' times, not 4');
        enc.forEach((e, i) => { if (e > 2) f.push('character ' + (i + 1) + ' starts at encumbrance ' + e); });
        const inv = [...s.matchAll(/> inventory > inventory (.*?) > quit/g)].map(x => x[1]);
        inv.forEach((t, i) => { for (const w of ['clothes', 'backpack', 'belt', 'tinderbox', 'rations', 'waterskin', '(lit', 'In all,']) if (!t.includes(w)) f.push('inventory ' + (i + 1) + ': no ' + w + ': ' + t.slice(0, 500)); });
        if (inv[4] && inv[4].length <= 400) f.push('the long inventory only ' + inv[4].length + ' letters: not the case it means: ' + inv[4]);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0packsteps', reach: true, near: ['gear'], what: 'Dingus Quest Zero\'s reach told and timed a step at a time (the user\'s, 2026-11-02, DECISIONS.md\'s: a worn pack off, the thing got out, the pack back on, each its words and its time; "Applies to any tool use"; light a new torch; lighting by a fire-maker; a torch redipped; HOUSE.md\'s reach, for review), by the words in order and the clock: at seed 3 (dq0 -T) from the worn pack, from a sack in it (a level more), a look in, a search, a pack on the ground (no off, no on), a stop part way (the pack left off, told), both hands free (in a hand), hands full (at its feet); "light a new torch" (the old let fall, a spare by its steps, lit from the old where it lies) and no spare ("You have no other torch.", nothing dropped), "take a fresh torch", "hold another torch"; a pick from the pack (out, the dig, stopped, the pack left off told); a torch lit by the tinderbox from the worn pack and from the belt\'s pouch (no off, no on), its tries told, 20 less in the dark; empty, told at its step, then refused at once; "refresh the torch" lit with a spare (the preferred order), with a sword in the other hand (set down, taken up), "dip the torch" with no spare (blown out, by touch in the dark, the oil spilt or not, lit again by the fire-maker)',
      init: 't_rc', cycles: 12000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sack', 'put the rations in the sack', 'stow the sack', 'take the rations from the sack', 'drop the rations',
        'look in the backpack', 'search the backpack', 'drop the pack', 'take the tinderbox from the backpack', 'drop the tinderbox', 'put on the pack',
        'drop the torch', 'take the flask from the backpack', 'drop the flask', 'take the torch from the backpack', 'inventory', 'put on the pack', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword', 'light a new torch', 'inventory', 'drop the sword', 'hold another torch', 'light a new torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take a fresh torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'hold another torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k pick', 'dig down', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'douse the torch', 'light the torch', 'douse the torch', 'take the tinderbox from the backpack', 'put the tinderbox in the pouch', 'light the torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'douse the torch', 'light the torch', 'inventory', 'light the torch', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'wait 20 minutes', 'refresh the torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword', 'wait 20 minutes', 'redip the torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'drop a fresh torch', 'wait 20 minutes', 'dip the torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword,whetstone', 'sharpen the sword', 'stow the whetstone', 'sharpen the sword', 'sharpen the sword', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword', 'sharpen the sword', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'hold another torch', 'light the torch', 'wait 5 minutes', 'dip the torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'hold another torch', 'light the torch', 'wait 2 minutes', 'dip the torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword', 'drop the sword', 'take the tinderbox from the backpack', 'drop the tinderbox', 'take the rations from the backpack', 'drop the rations', 'take the sword', 'take all', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take the tinderbox from the backpack', 'drop the tinderbox', 'step forward', 'step forward', 'take the rations from the backpack', 'drop the rations', 'hold another torch', 'turn around', 'take all', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k mang,whetstone', 'sharpen the mang', 'inventory', 'drop the mang', 'put the torch in the backpack', 'take the whetstone from the backpack', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take the flask of oil from the backpack', 'put the flask of oil in the pouch', 'hold another torch', 'light the torch', 'wait 5 minutes', 'dip the torch', 'inventory', 'quit',
        'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 7, line: 'douse the torch', n: 1, fuel: { tinderbox: 0 } }, { g: 11, line: 'stow the whetstone', n: 1, wear: { sword: 200 } }, { g: 12, line: 'sharpen the sword', n: 1, wear: { sword: 200 } }, { g: 14, line: 'wait 2 minutes', n: 1, fuel: { torch: 0 } }, { g: 17, line: 'sharpen the mang', n: 1, wear: { mang: 200 } }]);
        dq0OnWords(m, [{ g: 1, line: 'take the torch from the backpack', n: 1, words: 'You get out the', text: 'stop\r' },
          { g: 5, line: 'dig down', n: 1, words: 'You dig', text: 'stop\r' },
          { g: 17, line: 'sharpen the mang', n: 1, words: 'You set to sharpening the mang', text: 'stop\r' },
          { g: 17, line: 'take the whetstone from the backpack', n: 1, words: 'You get out the whetstone', text: 'stop\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        // (The answer to a line: its echo, then its words as far as the next line's echo: the nth such, from a game's start)
        const games = []; for (let i = s.indexOf('% dq0 '); i >= 0; i = s.indexOf('% dq0 ', i + 1)) games.push(i);
        const seg = (g, line, n = 1) => { const e = '> ' + line + ' > ' + line; let i = games[g - 1]; for (let k = 0; k < n && i >= 0; k++) i = s.indexOf(e, i + 1);
          if (i < 0) { f.push('game ' + g + ': "' + line + '" not answered'); return ''; } const j = s.indexOf(' > ', i + e.length); return s.slice(i + e.length, j < 0 ? s.length : j); };
        const order = (g, line, want, n) => inTurn(seg(g, line, n), want, f, 'game ' + g + ', ' + line + ': ');
        const never = (g, line, w, n) => { if (seg(g, line, n).includes(w)) f.push('game ' + g + ', ' + line + ': told ' + w); };
        const has = (g, line, w, n) => { if (!(w.test ? w.test(seg(g, line, n)) : seg(g, line, n).includes(w))) f.push('game ' + g + ', ' + line + ': not told ' + w + ': ' + seg(g, line, n).slice(0, 400)); };
        const gk = {}; T.forEach((t, k) => { if (/^dq0 /.test(t.line)) gk[Object.keys(gk).length + 1] = k; });
        const dt = (g, line, tenths) => { const t = T.of(line, gk[g]); const d = t.at && t.after ? t.after.now - t.at.now : null; if (d !== tenths) f.push('game ' + g + ', ' + line + ': ' + d + ' tenths, not ' + tenths); };
        const steps = 'You take off the backpack';
        order(1, 'put the rations in the sack', ['You take off the backpack, and set it down at your feet.', 'You get out the rations.', 'You hold the sack open.', 'You put the rations in the sack.', 'You put on the backpack.']);
        dt(1, 'put the rations in the sack', 160);
        order(1, 'stow the sack', [steps, 'You hold the backpack open.', 'You stow the sack in your backpack.', 'You put on the backpack.']);
        dt(1, 'stow the sack', 140);
        order(1, 'take the rations from the sack', [steps, 'You reach into the backpack.', 'You get out the rations.', 'You take the rations in your right hand.', 'You put on the backpack.']);
        dt(1, 'take the rations from the sack', 150);
        order(1, 'look in the backpack', [steps, 'You hold the backpack open.', 'In the backpack lie', 'You put on the backpack.']);
        dt(1, 'look in the backpack', 150);
        order(1, 'search the backpack', [steps, 'You hold the backpack open.', 'You search the backpack', 'You put on the backpack.']);
        dt(1, 'search the backpack', 720);
        order(1, 'take the tinderbox from the backpack', ['You get out the tinderbox.', 'You take the tinderbox from the backpack.']);
        never(1, 'take the tinderbox from the backpack', steps);
        dt(1, 'take the tinderbox from the backpack', 40);
        order(1, 'take the flask from the backpack', ['You take off the backpack.', 'You get out the flask of oil.', 'You take the flask of oil in your right hand.', 'You put on the backpack.']);
        dt(1, 'take the flask from the backpack', 130);
        // (Stopped part way: the pack off told, in a hand; "put on the pack" finishes it)
        const st = s.slice(s.indexOf('> take the torch from the backpack'), s.indexOf('> inventory', s.indexOf('> take the torch from the backpack')));
        if (!/Your backpack (is in your hand|lies at your feet)\./.test(st)) f.push('stopped part way, the pack off not told: ' + st.slice(0, 400));
        has(1, 'inventory', 'a backpack, in your', 1);
        has(1, 'put on the pack', 'You put on the backpack.', 2);
        // (A new light)
        order(2, 'light a new torch', ['You set the sword down at your feet.', 'You take off the backpack, and set it down at your feet.', 'You get out the new torch.', 'You light the new torch from the burning one.', 'You put out the old torch.', 'You let the old torch fall.', 'You take up the sword.', 'You put on the backpack.']);
        never(2, 'light a new torch', 'tinderbox');
        has(2, 'light a new torch', 'You light the new torch from the burning one.', 2);   // (The one in its other hand, 2026-11-03)
        never(2, 'light a new torch', 'You get out', 2);
        has(2, 'inventory', 'a sword, in your right hand');
        has(2, 'inventory', /a torch \(lit[^)]*\), in your left hand/, 2);
        has(3, 'inventory', 'a torch, in your right hand');
        has(3, 'inventory', /a torch \(lit[^)]*\), in your left hand/);
        has(4, 'inventory', 'a torch, in your right hand');
        // (A tool: a pick from the pack, the dig, stopped: the pack off told)
        order(5, 'dig down', ['You take off the backpack, and set it down at your feet.', 'You reach into the backpack.', 'You get out the pick.', 'You dig with your pick']);
        const ds = s.slice(s.indexOf('> stop', games[4]), s.indexOf('> inventory', games[4]));
        inTurn(ds, ['You stop digging.', 'Your backpack lies at your feet.'], f, 'the dig stopped: ');
        // (A fire-maker: from the worn pack, its tries in the dark at 20 less)
        order(6, 'light the torch', [steps, 'You reach into the backpack.', 'You get out the tinderbox.', 'You strike a spark.', 'It catches.', 'You light the torch.', 'You put away the tinderbox.', 'You put on the backpack.']);
        const K = m.dq0.layout().K, tries = T.of('light the torch', gk[6]).replies.filter(r => /^slot/.test(r.req) && r.a === K.P_GROPE);
        if (!tries.length || tries.some(r => r.steps !== -4)) f.push('the tries in the dark not at 20 less: ' + JSON.stringify(tries.map(r => r.steps)));
        order(6, 'light the torch', ['You get out the tinderbox.', 'You strike a spark.', 'It catches.', 'You light the torch.', 'You put away the tinderbox.'], 2);
        never(6, 'light the torch', steps, 2);
        never(6, 'light the torch', 'You put on the backpack.', 2);
        // (Empty: told at its step, put away, on; then at once)
        order(7, 'light the torch', [steps, 'You get out the tinderbox.', 'The tinderbox is empty.', 'You put away the tinderbox.', 'You put on the backpack.']);
        never(7, 'light the torch', 'You strike a spark.');
        has(7, 'light the torch', 'The tinderbox is empty.', 2);
        never(7, 'light the torch', steps, 2);
        // (A torch redipped: the preferred order; a sword in the other hand; no spare)
        const pref = ['You get out the new torch.', 'You light the new torch from the burning one.', 'You put out the old torch.', 'You set the new torch down at your feet.', 'You get out the flask of oil.', 'You set to dipping the old torch.', 'You soak the torch in the flask of oil.', 'You put away the flask of oil.', 'You put away the old torch.', 'You take up the new torch.', 'You put on the backpack.'];
        order(8, 'refresh the torch', [steps, ...pref]);
        has(8, 'inventory', /a torch \(lit[^)]*\), in your left hand/);
        order(9, 'redip the torch', ['You set the sword down at your feet.', steps, ...pref.slice(0, 9), 'You take up the new torch.', 'You take up the sword.', 'You put on the backpack.']);
        has(9, 'inventory', 'a sword, in your right hand');
        order(10, 'dip the torch', ['You go to put out the torch: that will leave you in the dark.', steps, 'You get out the flask of oil.', 'You set to dipping the torch.', 'You put away the flask of oil.', 'You get out the tinderbox.', 'You strike a spark.', 'You light the torch.', 'You put away the tinderbox.', 'You put on the backpack.']);
        if (!/You soak the torch|oil spills/.test(seg(10, 'dip the torch'))) f.push('dipped in the dark: neither soaked nor spilt');
        has(10, 'inventory', 'a torch (lit');
        // (Two lit, the one with the most room the old: blown out, set by the new; a spent one: nothing to blow out, kept to its end; each named)
        const dipped = ['You set to dipping the old torch.', 'You soak the torch in the flask of oil.', 'You put away the flask of oil.'];
        order(13, 'dip the torch', ['You put out the old torch.', 'You set the new torch down at your feet.', steps, 'You get out the flask of oil.', ...dipped, 'You put away the old torch.', 'You take up the new torch.', 'You put on the backpack.']);
        has(13, 'inventory', /a torch \(lit[^)]*\), in your left hand/);
        order(14, 'dip the torch', ['You set the new torch down at your feet.', steps, 'You get out the flask of oil.', ...dipped, 'You take up the new torch.', 'You put on the backpack.']);
        if (seg(14, 'dip the torch').includes('You put out')) f.push('game 14: a spent torch blown out');
        has(14, 'inventory', 'a torch, in your left hand');
        has(14, 'inventory', /a torch \(lit[^)]*\), in your right hand/);
        order(18, 'dip the torch', ['You put out the old torch.', 'You set the new torch down at your feet.', 'You get out the flask of oil.', ...dipped, 'You take up the new torch.']);
        if (seg(18, 'dip the torch').includes('backpack')) f.push('game 18: the pack touched, the flask in the pouch');
        has(18, 'inventory', 'a torch, in your');
        has(18, 'inventory', /a torch \(lit[^)]*\), in your/);
        // (A run of handlings on one line: the pack off once, on once; a walk off between: on first, the rest by the lump, untold)
        order(15, 'take all', ['You take off the backpack', 'You hold the backpack open.', 'Taken.', 'Taken.', 'Taken.', 'You put on the backpack.']);
        has(15, 'inventory', 'a backpack, worn');
        order(16, 'take all', [steps, 'You hold the backpack open.', 'Taken.', 'Taken.', 'You put on the backpack.', 'You go over to the tinderbox.', 'Taken.']);
        has(16, 'inventory', /a backpack, worn \(\w+\) 3 rations \(\w+\) a tinderbox/);
        // (A kind past 127, its top bit the UI's: named as it's begun and as it's stopped)
        inTurn(s.slice(games[16], games[17]), ['You set to sharpening the mang.', 'You stop sharpening the mang.', 'Your backpack lies at your feet.'], f, 'game 17: ');
        // (A whetstone: the routine's use, sharpened, a sharp one told; none carried, told)
        has(11, 'sharpen the sword', 'The sword is as sharp as it will go.');
        order(11, 'sharpen the sword', [steps, 'You get out the whetstone.', 'You set to sharpening the sword.', 'You put a fresh edge on the sword.', 'You put away the whetstone.', 'You put on the backpack.'], 2);
        has(12, 'sharpen the sword', 'You have nothing to sharpen the sword with.');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0packgrope', reach: true, initiative: true, near: ['gear', 'fight'], what: 'Dingus Quest Zero\'s reach engaged (the user\'s, 2026-11-02: "Dropping your pack in a fight should be automatic (no grope test), but taking it in hand should be"; each step a grope, a round; HOUSE.md\'s reach): at seed 7 the ghoul engaging it (set steady), the pack taken off a test, dropped none, taken up from the ground a test, and from the worn pack each step a round its own test, the sequence over at a fumble, the world as it stands',
      init: 't_rc', cycles: 3200e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 7 -c /ram/c.hm -k \'sword,gambeson,mail hauberk\' -T', 'wear the gambeson', 'wear the mail hauberk', 'd', 'wait']) +
        'take the rations from the backpack\rtake off the pack\rdrop the pack\rtake the pack\rquit\r' + dq0Typed(['echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 1, line: 'wait', n: 1, name: 'ghoul', set: { mood: 8 } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out);
        const K = m.dq0.layout().K, w = T.of('wait'), late = T.filter(t => t.k >= w.k);
        const seq = late.flatMap(t => t.replies).filter(r => (/^slot/.test(r.req) && r.kind === K.RS_TEST && r.a === K.P_GROPE) || (r.req === 'ireq' && /^IO_(STEP|TAKE|DOFF|DROP|GET)$/.test(r.kind)));
        const k = kind => seq.findIndex(r => r.kind === kind);
        const dr = k('IO_DROP'), gt = k('IO_GET'), off = k('IO_DOFF');
        if (off < 0 || !/^slot/.test((seq[off - 1] || {}).req)) f.push('the pack taken in hand off the back not a grope: ' + JSON.stringify(seq[off - 1]));
        if (dr < 0 || /^slot/.test((seq[dr - 1] || {}).req)) f.push('the pack dropped with a test, or not dropped: ' + JSON.stringify(seq[dr - 1]));
        if (gt < 0 || !/^slot/.test((seq[gt - 1] || {}).req)) f.push('the pack taken up from the ground not a grope: ' + JSON.stringify(seq[gt - 1]));
        const hw = (m.hearts || []).find(h => h.g === 1 && h.line === 'wait');
        if (!hw || !hw.acts.some(a => a.name === 'ghoul' && a.mood === 8)) f.push('the ghoul not set steady');
        if (!/Put down\.|You put down the/.test(s)) f.push('the pack not put down');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0packasks', reach: true, asks: true, near: ['gear'], what: 'Dingus Quest Zero\'s torch redipped with no spare and no other light: asked first, as it would leave it in the dark (HOUSE.md\'s a torch redipped; the user\'s, 2026-11-02): answered no, the torch still lit, nothing dipped',
      init: 't_rc', cycles: 1600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'drop a fresh torch', 'wait 20 minutes', 'dip the torch', 'no', 'inventory', 'quit', 'echo $status']) },
      // (The rats incidental: sent off as the torch is got out, hear1's, 2026-11-20: the minds hearing one another's
      // footsteps by the senses' rule, a giant rat came into sight then and broke off the getting out)
      start(m) { dq0Poke(m, [{ g: 1, line: 'drop a fresh torch', n: 1, call: (h, L, K, at) => dq0Alone(m, h, L, K, at, []) }]); },
      expect: ['% echo $status'],
      check(m, out) {
        const s = dq0Prose(out), f = [];
        inTurn(s, ['> dip the torch', 'You go to put out the torch: that will leave you in the dark.', 'Keep putting out the torch?', 'You stop putting out the torch.', '> inventory', 'a torch (lit'], f);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0stopsweep', reach: true, meaning: true, near: ['gear'], what: 'Dingus Quest Zero\'s reach stopped at every moment tells only what happened (the coordinator\'s, 2026-11-20: "The game must never tell what didn\'t happen"; hear2\'s dq0packsteps, "You put on the backpack." told with the pack left in hand): "take the flask from the backpack" at seed 3 (dq0 -T: the pack off and set down, the flask got out, taken in the right hand, the pack back on, some 20M cycles), a "stop" typed 0.5M, 1.5M ... 23.5M cycles after its Enter, each its own try in one game (the flask put back and the pack put on between them); at each, by the item store as the inventory\'s typed after it: every word told of the pack true (the last of "You take off the backpack", "You put on the backpack", "Your backpack is in your hand", "Your backpack lies at your feet" where the pack is; none of the last two after a "put on" with no "take off" between), the flask in the pack unless "You get out the flask of oil." or "You take the flask of oil in your right hand." was told (got out: out of the pack; taken: in hand).  The stop a race in timing only: a step\'s words told as it\'s done (part 86\'s RO_END), a step stopped part way undone and untold (HOUSE.md\'s: "You stop taking off the backpack.", the pack still on)',
      init: 't_rc', cycles: 2600e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T',
        ...STOPS.flatMap(() => ['take the flask from the backpack', 'inventory', 'put the flask in the backpack', 'put on the pack']), 'quit', 'echo $status']) },
      start(m) {                                // (Each try's stop typed its cycles after its Enter; a later line typed first, none)
        const a = m.acia, read = a.read, cpu = m.cpu, step = cpu.step.bind(cpu);
        let typed = '', k = 0, due = 0;
        a.read = (r, t) => {
          if (r === 0 && a.rdrf) {
            const c = a.rx;
            if (c === 13) {
              if (typed !== 'stop') due = 0;
              if (typed === 'take the flask from the backpack' && k < STOPS.length) due = cpu.cyc + STOPS[k++];
              typed = '';
            } else if (c >= 32 && c < 127) typed += String.fromCharCode(c);
          }
          return read(r, t);
        };
        // (Not into a line being typed: the inventory's typed as the reach is over, its stop then none, as its Enter comes;
        //  bugs1's, 2026-11-21: "inventstop" typed, the stop let in between its keys)
        cpu.step = v => { step(v); if (due && cpu.cyc >= due && typed === '' && !a.rdrf) { due = 0; a.rxQueue.unshift(...'stop\r'); } };
      },
      expect: ['% echo $status'],
      check(m, out) {
        const s = dq0Prose(out).replace(/\s+/g, ' '), f = [], T = dq0Turns(m), LINE = 'take the flask from the backpack';
        const PACK = /You take off the backpack|Your backpack is in your hand|Your backpack lies at your feet|You put on the backpack/g;
        let at = 0, from = 0;
        this.notes = [];
        for (const d of STOPS) {
          const i = s.indexOf('> ' + LINE, at), j = s.indexOf('> inventory', i + 1), u = T.of(LINE, from), inv = T.after(u, 'inventory');
          if (i < 0 || j < 0 || u.k < 0 || !inv.at) { f.push('the try stopped at ' + d + ' not played'); break; }
          at = j + 5;
          from = inv.k + 1;
          const seg = s.slice(i + LINE.length + 2, j), st = inv.at;
          const pack = st.items.find(x => x.name === 'backpack'), fl = st.items.find(x => x.name === 'flask of oil');
          const ps = !pack ? '?' : pack.where === 'IW_ACTOR' && pack.x === 0 ? (pack.flags & DQ0_I.WORN ? 'worn' : pack.flags & DQ0_I.HELD ? 'in hand' : 'carried') :
            pack.where === 'IW_SQUARE' ? 'at its feet' : pack.where;
          const fs_ = !fl ? '?' : fl.where === 'IW_ITEM' && pack && fl.x === pack.n ? 'in the pack' : fl.where === 'IW_ACTOR' && fl.x === 0 && (fl.flags & DQ0_I.HELD) ? 'in hand' : fl.where;
          const pw = seg.match(PACK) || [], last = pw.length ? pw[pw.length - 1] : '';
          const want = !last || /put on/.test(last) ? 'worn' : /in your hand/.test(last) ? 'in hand' : /at your feet/.test(last) ? 'at its feet' : 'not worn';
          this.notes.push(d + ': ' + seg.slice(0, 300) + ' || the pack ' + ps + ', the flask ' + fs_);
          let told = 'worn';
          for (const w of pw) {
            if (/take off/.test(w)) told = 'off';
            else if (/put on/.test(w)) told = 'worn';
            else if (told === 'worn') f.push(d + ': "' + w + '." told with the pack told on');
          }
          if (want === 'not worn' ? ps === 'worn' : ps !== want) f.push(d + ': the pack told "' + (last || 'nothing') + '" but ' + ps);
          const took = /You take the flask of oil in your/.test(seg), got = /You get out the flask of oil/.test(seg);
          if (took ? fs_ !== 'in hand' : got ? fs_ === 'in the pack' : fs_ !== 'in the pack')
            f.push(d + ': the flask told ' + (took ? 'taken' : got ? 'got out' : 'nothing') + ' but ' + fs_);
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0reachstop', reach: true, meaning: true, near: ['gear'], what: 'Dingus Quest Zero\'s lines that break off, acted on at once during any long task and a reach\'s steps, every other line held till its end (the user\'s, 2026-11-06, DECISIONS.md\'s a line typed during a reach: "`stop` and other similar commands should be acted on immediately, but others could be held until after the reach"; HOUSE.md\'s breaking off at once), at seed 3 (dq0 -T), each line typed as a step\'s words are told: an ordinary line ("drop the rations") typed as the pack comes off waits till the pack\'s back on; "halt" (a stop now) as the rations are got out: stopped there, the pack left at its feet, said, no ally\'s words; a flight as the worn pack\'s searched: the search stopped, the pack in hand said, then the flight; "enough" in a search of the room: stopped; a flight as a pick from the pack digs: the dig stopped, the pack at its feet and the pick in hand said; an ordinary line ("turn around") in "take all" runs after its last thing and the pack back on, a stop there ends it, its later things not taken; a flight in a list\'s first part lets its later parts go; an ordinary line as a fire-maker from the pack is struck waits for the torch lit and the pack back on.  And the class kept by the parser\'s meanings: every word commands.verbs gives stop, flee or disengage is one parts.c\'s list takes at once ahead of a list\'s later parts',
      init: 't_rc', cycles: 7000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take the rations from the backpack', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take the rations from the backpack', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'search the backpack', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'search the room', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k pick', 'dig down', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take the tinderbox from the backpack', 'drop the tinderbox', 'take the rations from the backpack', 'drop the rations', 'take all', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take the tinderbox from the backpack', 'drop the tinderbox', 'take the rations from the backpack', 'drop the rations', 'take all', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'take the tinderbox from the backpack and take the flask from the backpack', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'douse the torch', 'light the torch', 'inventory', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'take the rations from the backpack', n: 1, words: 'You take off the backpack', text: 'drop the rations\r' },
          { g: 2, line: 'take the rations from the backpack', n: 1, words: 'You get out the', text: 'halt\r' },
          { g: 3, line: 'search the backpack', n: 1, words: 'You search the backpack', text: 'flee\r' },
          { g: 4, line: 'search the room', n: 1, words: 'You work along', text: 'enough\r' },
          { g: 5, line: 'dig down', n: 1, words: 'You dig with your', text: 'flee\r' },
          { g: 6, line: 'take all', n: 1, words: 'Taken.', text: 'turn around\r' },
          { g: 7, line: 'take all', n: 1, words: 'Taken.', text: 'stop\r' },
          { g: 8, line: 'take the tinderbox from the backpack and take the flask from the backpack', n: 1, words: 'You get out the tinderbox', text: 'flee\r' },
          { g: 9, line: 'light the torch', n: 1, words: 'You strike a spark', text: 'turn left\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const s = dq0Prose(out), f = [], gs = [];
        for (let i = s.indexOf('% dq0 '); i >= 0; i = s.indexOf('% dq0 ', i + 1)) gs.push(i);
        const game = g => s.slice(gs[g - 1], gs[g] === undefined ? s.length : gs[g]);
        // (A game's story from its line to its inventory's answer)
        const upto = (g, line) => { const t = game(g), a = t.indexOf('> ' + line + ' > ' + line), b = t.indexOf('> inventory', a); return a < 0 ? '' : t.slice(a, b < 0 ? t.length : b); };
        const inv = g => { const t = game(g), a = t.indexOf('> inventory > inventory'); return a < 0 ? '' : t.slice(a, t.indexOf('> quit', a)); };
        const order = (g, line, want) => inTurn(upto(g, line), want, f, 'game ' + g + ', ' + line + ': ');
        const never = (g, line, w) => { if (upto(g, line).includes(w)) f.push('game ' + g + ', ' + line + ': told ' + w); };
        // (An ordinary line waits for the pack back on)
        order(1, 'take the rations from the backpack', ['You take off the backpack', 'drop the rations', 'You get out the rations.', 'You take the rations in your right hand.', 'You put on the backpack.', 'You put down the rations.']);
        // (halt: a stop, at once, the pack where it was left; the user's, 2026-11-11: the parse of "halt" and the end of the
        //  short get-out step race at the turns pace, so a stop landing in the step after (the rations taken in hand, then
        //  "You stop there.") passes too, the pack still left at its feet and nothing after it done)
        //  (reach1's, 2026-11-20: a step's words told as it's done, "halt" typed at "You get out the" lands after the get-out,
        //  as the rations are taken in hand: "You stop there.", the rations in hand, the pack at its feet)
        //  (sched1's, 2026-11-21, the coordinator's: a quicker scheduler lands it a step sooner as well, "halt" told between
        //  the get-out and the take; the stop the same, after the take, the rations in hand and the pack at its feet)
        const g2 = upto(2, 'take the rations from the backpack'), sooner = g2.indexOf('halt') >= 0 && g2.indexOf('halt') < g2.indexOf('You take the rations in your right hand.');
        order(2, 'take the rations from the backpack', sooner ? ['You get out the rations.', 'halt', 'You take the rations in your right hand.', 'You stop there.',
          'Your backpack lies at your feet.'] : ['You get out the rations.', 'You take the rations in your right hand.', 'halt', 'You stop there.',
          'Your backpack lies at your feet.']);
        if (!/rations, in your/.test(inv(2))) f.push('game 2: the rations told taken, not in hand: ' + inv(2));
        never(2, 'take the rations from the backpack', 'You put on the backpack.');
        never(2, 'take the rations from the backpack', 'no ally');
        if (/\(b\) a backpack, worn/.test(inv(2))) f.push('game 2: the pack back on after the halt');
        // (A flight in a search of the pack: stopped, the pack in hand told, then the flight)
        order(3, 'search the backpack', ['You search the backpack', 'flee', 'You stop searching.', 'Your backpack is in your hand.', 'You see nothing to flee from.']);
        never(3, 'search the backpack', 'You put on the backpack.');
        if (!inv(3).includes('a backpack, in your right hand')) f.push('game 3: the pack not left in its hand: ' + inv(3));
        order(4, 'search the room', ['You work along', 'enough', 'You stop searching.']);
        never(4, 'search the room', 'over the whole room');
        order(5, 'dig down', ['You dig with your pick', 'flee', 'You stop digging.', 'Your backpack lies at your feet.', 'Your pick is in your hand.', 'You see nothing to flee from.']);
        never(5, 'dig down', 'You put away the pick.');
        // ("take all": an ordinary line after its last thing and the pack on; a stop ends it there)
        order(6, 'take all', ['Taken.', 'turn around', 'Taken.', 'You put on the backpack.', 'You turn to face']);
        // (the user's, 2026-11-12, as game 2's: "stop" typed at the first "Taken." races the reach for the next thing at the
        //  turns pace, so the stop landing one thing later passes too, the tinderbox taken, then stopped, nothing after it)
        if (!upto(7, 'take all').includes('You reach for the tinderbox.')) {
          order(7, 'take all', ['Taken.', 'stop', 'You stop', 'Your backpack is in your hand.']);
          if (inv(7).includes('tinderbox')) f.push('game 7: "take all" went on after the stop: ' + inv(7));
        } else {
          order(7, 'take all', ['Taken.', 'stop', 'You reach for the tinderbox.', 'Taken.', 'You stop', 'Your backpack is in your hand.']);
          if ((upto(7, 'take all').match(/Taken\./g) || []).length > 2) f.push('game 7: "take all" went on past one thing after the stop');
        }
        // (A list's later part let go with a flight)
        // (reach1's, 2026-11-20: a step's words told as it's done, "You get out the tinderbox." the get-out over, the take in
        //  hand after it; the flight typed at those words lands as it's taken, "You stop there.", the pack left at its feet)
        order(8, 'take the tinderbox from the backpack and take the flask from the backpack', ['You get out the tinderbox.', 'You take the tinderbox in your right hand.', 'flee',
          'You stop there.', 'Your backpack lies at your feet.', 'You see nothing to flee from.']);
        if (!/tinderbox, in your/.test(inv(8))) f.push('game 8: the tinderbox told taken, not in hand: ' + inv(8));
        never(8, 'take the tinderbox from the backpack and take the flask from the backpack', 'flask of oil');
        // (A fire-maker from the pack: an ordinary line after the torch lit and the pack on)
        order(9, 'light the torch', ['You strike a spark.', 'turn left', 'You light the torch.', 'You put away the tinderbox.', 'You put on the backpack.', 'You turn to face']);
        // (The class by the parser's meanings: every word for stop, flee or disengage taken at once ahead of a list's parts, parts.c's)
        const root = path.join(TESTS, '..', 'games', 'dq0'), verbs = fs.readFileSync(path.join(root, 'commands.verbs'), 'latin1'), pc = fs.readFileSync(path.join(root, 'parts.c'), 'latin1');
        const said = ['stop', 'flee', 'disengage'];
        for (const l of verbs.slice(verbs.indexOf('[words]')).split(/\r?\n/)) { const w = /^([a-z ]+?) = (stop|flee|disengage)\b/.exec(l); if (w && !said.includes(w[1])) said.push(w[1]); }
        const list = n => { const r = new RegExp('static const char ' + n + '\\[\\] = "([^"]*)"').exec(pc); return r ? r[1].split('\\0').filter(Boolean) : []; };
        const one = list('brks'), two = list('brk2');
        for (const p of said) { const w = p.split(' '); if (w.length === 1 ? !one.includes(p) : !(two.includes(w[0]) && ['away', 'off'].includes(w[1]))) f.push('"' + p + '" breaks off (commands.verbs) but parts.c\'s list doesn\'t take it at once'); }
        if (!said.includes('halt') || !said.includes('enough')) f.push('halt and enough not the stop\'s in commands.verbs');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0reachflee', reach: true, near: ['gear', 'fight'], what: 'Dingus Quest Zero\'s flight and stop typed during a reach engaged (HOUSE.md\'s breaking off at once; the user\'s, 2026-11-06, "`stop` and other similar commands should be acted on immediately"; and 2026-11-17, "\'stop\' ends everything under way, a fight\'s part and the reach it took over"): at seed 7 the ghoul engaging it (set steady, dq0packgrope\'s scene), the sword and the lit torch in hand, "take the rations from the backpack" typed ahead, each step a round; the hand for the rations the torch\'s, laid down lit, the sword kept to fight with (the user\'s, 2026-11-17: "A drawn weapon keeps its hand in a fight"; the home first outside one: its sword not stowed); "flee" typed as the pack comes off: that step stopped, not done (the pack still worn), the rations not got out, then the flight from the ghoul, before any further step; "stop" typed the same: the step stopped, no further step, the reach over; and, a crowbar in hand for the sword (no weapon: its home the pack, so the pack comes off first, as the sword\'s did in a fight before), "stop" typed as the pack comes off and taken as the fight takes a round between the reach\'s steps ("You turn to the ghoul. Fighting the ghoul"): the fight stopped and the reach it took over with it, no further step (the crowbar not stowed, the rations not got out), the pack still worn (built/2026-11-17-things.md\'s open question 7: the reach went on after it)',
      init: 't_rc', cycles: 6000e6,
      machine: { input: (() => {
        const game = ["dq0 -p t -s 7 -c /ram/c.hm -k 'sword,gambeson,mail hauberk' -T", 'wear the gambeson', 'wear the mail hauberk', 'd', 'wait'];
        const tool = ["dq0 -p t -s 7 -c /ram/c.hm -k 'gambeson,mail hauberk,crowbar' -T", 'wear the gambeson', 'wear the mail hauberk', 'hold the crowbar', 'd', 'wait'];
        return dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', ...game]) + 'take the rations from the backpack\r' +
          dq0Typed(['inventory', 'quit', ...game]) + 'take the rations from the backpack\r' + dq0Typed(['inventory', 'quit', ...tool]) +
          'take the rations from the backpack\r' + dq0Typed(['inventory', 'quit', 'echo $status']);
      })() },
      start(m) {
        dq0Poke(m, [1, 2, 3].map(g => ({ g, line: 'wait', n: 1, name: 'ghoul', set: { mood: 8 } })));
        // (reach1's, 2026-11-20: a step's words told as it's done, so typed as the pack comes off is typed in the round after the
        //  step before's words, the torch laid down (LATER cycles on: a line typed just between two steps, a flight's held to
        //  the reach's end, the report's); the crowbar's, as the fight takes its round after "You reach for your backpack.")
        const a = m.acia, cpu = m.cpu, step = cpu.step.bind(cpu), LATER = 3000000;   // (3M cycles: in the pack's round; 1M and
                                                // 2M land as the step changes, the flight held past the inventory, the report's)
        let due = 0, text = '';
        cpu.step = v => { step(v); if (due && cpu.cyc >= due) { due = 0; a.rxQueue.unshift(...text); a.promptFrom = -1; } };
        const later = t => () => { due = cpu.cyc + LATER; text = t; };
        dq0OnWords(m, [{ g: 1, line: 'take the rations from the backpack', n: 1, words: 'You lay the torch on the ground, still burning.', call: later('flee\r') },
          { g: 2, line: 'take the rations from the backpack', n: 1, words: 'You lay the torch on the ground, still burning.', call: later('stop\r') },
          { g: 3, line: 'take the rations from the backpack', n: 1, words: 'You reach for your backpack. You turn to the ghoul.', text: 'stop\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const s = dq0Prose(out), f = [], gs = [];
        for (let i = s.indexOf('% dq0 '); i >= 0; i = s.indexOf('% dq0 ', i + 1)) gs.push(i);
        const game = g => s.slice(gs[g - 1], gs[g] === undefined ? s.length : gs[g]);
        const after = (g, w) => { const t = game(g), a = t.indexOf(w); return a < 0 ? '' : t.slice(a, t.indexOf('> inventory', a)); };
        const fl = after(1, 'flee > flee');
        // (Its words "You stop taking off the backpack." or, landing as the step's begun and not yet noted, "You stop there.":
        //  a race in timing, both true, the pack still worn, reach1's, 2026-11-20)
        const stopped = t => /You stop taking off the backpack\./.test(t) ? 'You stop taking off the backpack.' : 'You stop there.';
        inTurn(fl, [stopped(fl), 'You break away from the ghoul.'], f, 'the flight: ');
        if (/You get out the rations|You put on the backpack/.test(fl.slice(0, fl.indexOf('You break away')))) f.push('a step after the flight was typed: ' + fl.slice(0, 600));
        const st = after(2, 'stop > stop');
        inTurn(st, [stopped(st)], f, 'the stop: ');
        if (/You get out the rations|You put on the backpack/.test(st)) f.push('a step after the stop: ' + st.slice(0, 600));
        const sf = after(3, 'stop > stop');
        inTurn(sf, ['You stop fighting.'], f, 'the stop as the fight took a round: ');
        if (/You reach for the rations|You hold the backpack open|You stow the crowbar|You get out the rations|You put on the backpack/.test(sf)) f.push('the reach went on after the stop that ended its fight: ' + sf.slice(0, 600));
        for (const g of [1, 2, 3]) {
          const t = game(g), i = t.slice(t.indexOf('> inventory > inventory'));
          if (!/\(b\) a backpack, worn/.test(i) || /rations \(\w+\), in your|rations, in your/.test(i)) f.push('game ' + g + ': the pack not still worn, or the rations got out: ' + i.slice(0, 300));
          if (g === 3) {
            if (!/a crowbar, in your/.test(i)) f.push('game 3: the crowbar not still in hand: ' + i.slice(0, 300));
            continue;
          }
          if (!/You lay the torch on the ground, still burning\./.test(t)) f.push('game ' + g + ': the torch not laid down for the rations\' hand, its weapon kept: ' + t.slice(t.indexOf('> take the rations'), t.indexOf('> take the rations') + 400));
          if (/You stow the sword|You put the sword/.test(t)) f.push('game ' + g + ': the sword put away in a fight');
          if (!/a sword, in your/.test(i)) f.push('game ' + g + ': the sword not still in hand: ' + i.slice(0, 300));
        }
        return f;
      },
      sdcard: true
    },
    {
      // (A flight typed between two steps of a reach: reach1's open question 2, 2026-11-20; bugs1's, 2026-11-21)
      name: 'dq0reachflee2', reach: true, near: ['gear', 'fight'], what: 'Dingus Quest Zero\'s flight typed exactly between two steps of a reach engaged (HOUSE.md\'s breaking off at once): dq0reachflee\'s first scene (seed 7, the ghoul engaging it, the sword and the lit torch in hand, "take the rations from the backpack" typed ahead), "flee" typed 1M and 2M cycles after "You lay the torch on the ground, still burning." (as the torch\'s step ends and the pack\'s begins): the reach stopped there, no further step (the pack still worn, the rations not got out), and the flight from the ghoul at once, before the next line typed ("inventory") is answered (reach1\'s: the flight waited till after it)',
      init: 't_rc', cycles: 4000e6,
      machine: { input: (() => {
        const game = ["dq0 -p t -s 7 -c /ram/c.hm -k 'sword,gambeson,mail hauberk' -T", 'wear the gambeson', 'wear the mail hauberk', 'd', 'wait'];
        return dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', ...game]) + 'take the rations from the backpack\r' +
          dq0Typed(['inventory', 'quit', ...game]) + 'take the rations from the backpack\r' + dq0Typed(['inventory', 'quit', 'echo $status']);
      })() },
      start(m) {
        dq0Poke(m, [1, 2].map(g => ({ g, line: 'wait', n: 1, name: 'ghoul', set: { mood: 8 } })));
        const a = m.acia, cpu = m.cpu, step = cpu.step.bind(cpu);
        let due = 0;
        cpu.step = v => { step(v); if (due && cpu.cyc >= due) { due = 0; a.rxQueue.unshift(...'flee\r'); a.promptFrom = -1; } };
        const later = c => () => { due = cpu.cyc + c; };
        dq0OnWords(m, [{ g: 1, line: 'take the rations from the backpack', n: 1, words: 'You lay the torch on the ground, still burning.', call: later(1000000) },
          { g: 2, line: 'take the rations from the backpack', n: 1, words: 'You lay the torch on the ground, still burning.', call: later(2000000) }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const s = dq0Prose(out), f = [], gs = [];
        for (let i = s.indexOf('% dq0 '); i >= 0; i = s.indexOf('% dq0 ', i + 1)) gs.push(i);
        const game = g => s.slice(gs[g - 1], gs[g] === undefined ? s.length : gs[g]);
        for (const g of [1, 2]) {
          const t = game(g), a = t.indexOf('flee > flee'), fl = a < 0 ? '' : t.slice(a, t.indexOf('> inventory', a));
          if (!fl) { f.push('game ' + g + ': no flee typed in the reach: ' + t.slice(t.indexOf('> take the rations'), t.indexOf('> take the rations') + 600)); continue; }
          if (!/You stop (?:there|taking off the backpack)\./.test(fl)) f.push('game ' + g + ': the reach not stopped as the flee came: ' + fl.slice(0, 600));
          if (!/You break away from the ghoul\./.test(fl)) f.push('game ' + g + ': the flight not at once, before the next line\'s answer: ' + fl.slice(0, 600) + ' ... ' + t.slice(t.indexOf('> inventory', a), t.indexOf('> inventory', a) + 400));
          if (/You get out the rations|You put on the backpack|You take off the backpack/.test(fl.slice(0, Math.max(0, fl.indexOf('You break away'))))) f.push('game ' + g + ': a step after the flee: ' + fl.slice(0, 600));
          const i = t.slice(t.indexOf('> inventory > inventory'));
          if (!/\(b\) a backpack, worn/.test(i) || /rations \(\w+\), in your|rations, in your/.test(i)) f.push('game ' + g + ': the pack not still worn, or the rations got out: ' + i.slice(0, 300));
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0usedup', what: 'Dingus Quest Zero\'s handy tools and things used up (the user\'s, 2026-10-23: "Ink, pencils, charcoal, etc., should be used up eventually.  A pot of ink might last a really long time, as might a pencil, but a chunk of charcoal or chalk would be used up faster"; mapping\'s things "handy and equipped"; HOUSE.md\'s reach, handy tools and things used up, and the map kept), by the hub\'s map making (pc_chart, pc_map), the item store (the chalk\'s fuel: minutes of drawing left), the world\'s events and the words: at seed 3 (dq0 -T, dq0 -k paper,chalk: the paper in hand, the chalk in the worn pack) mapping as it goes with its chalk not handy: no map begun, what they\'re in the hub\'s (pc_chart, CH_PACKED: the backpack), told once as it looks ("Your drawing things are in your backpack.", not again the next look); the chalk put in the pouch on its belt, handy: the map begun (pc_chart CH_DRAW, pc_map), each drawing (a walk across the room and back, mapping on paper) two minutes off the chalk (three hours: the catalogue\'s burns, FU_DRAW), till it\'s spent: gone, told ("Your chalk is worn to nothing.", E_BEAT\'s WB_SPENT, its kind), the map making without it (no CH_DRAW)',
      init: 't_rc', cycles: 4000e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k paper,chalk', 'map as I go', 'look', 'look', 'put the chalk in the pouch',
        ...Array(48).fill(['go to the far side', 'go to the near side']).flat(), 'i', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], s = dq0Prose(out), K = m.dq0.layout().K;
        const g = T.of('dq0 -p t -s 3 -c /ram/c.hm -T -k paper,chalk');
        if (g.k < 0) return ['the game not played'];
        const mg = T.after(g, 'map as I go'), l1 = T.after(mg, 'look'), l2 = T.after(l1, 'look'), pu = T.after(l2, 'put the chalk in the pouch');
        if (!mg.after || mg.after.pc.map || (mg.after.pc.chart & 1) || !(mg.after.pc.chart & 2) || mg.after.pc.chartk !== K.KI_BACKPACK) f.push('mapping with the chalk in the worn pack: pc_chart ' + (mg.after && mg.after.pc.chart) + ', pc_map ' + (mg.after && mg.after.pc.map) + ' (not handy, in the backpack, no map begun wanted)');
        const told = (s.match(/Your drawing things are in your backpack\./g) || []).length;
        if (told !== 1) f.push('"Your drawing things are in your backpack." told ' + told + ' times, not once');
        inTurn(s, ['> look', 'Your drawing things are in your backpack.', '> look', '> put the chalk in the pouch'], f, 'told as it looks: ');
        if (!pu.after || !(pu.after.pc.chart & 1) || !pu.after.pc.map) f.push('the chalk handy in the pouch: no map making (pc_chart ' + (pu.after && pu.after.pc.chart) + ', pc_map ' + (pu.after && pu.after.pc.map) + ')');
        const chalk = st => st && st.items.find(i => i.name === 'chalk');
        const c0 = chalk(pu.after);
        if (!c0 || c0.fuel < 176 || c0.fuel > 180) f.push('the chalk not three hours of drawing as it\'s put in the pouch: ' + JSON.stringify(c0));
        const left = T.filter(t => t.k > pu.k && /^go to the/.test(t.line)).map(t => chalk(t.after)).filter(Boolean).map(c => c.fuel);
        if (left.length < 40 || left.some((v, i) => i && ((left[i - 1] - v) & 1 || left[i - 1] - v < 0 || left[i - 1] - v > 4)) ||
          left[0] - left[left.length - 1] < 160) f.push('the chalk not used up two minutes a drawing as it walks: ' + left.join(' '));
        const spent = T.filter(t => t.k > pu.k && dq0Ev(t, 'E_BEAT', { beat: 'WB_SPENT', thing: K.KI_CHALK }).length);
        if (spent.length !== 1) f.push('the chalk not spent once (' + spent.length + ')');
        const end = T.of('i', pu.k);
        if (chalk(end.after)) f.push('the chalk not gone: ' + JSON.stringify(chalk(end.after)));
        if (end.after && (end.after.pc.chart & 1)) f.push('still drawing with no chalk: pc_chart ' + end.after.pc.chart);
        inTurn(s, ['Your chalk is worn to nothing.'], f, 'the chalk spent: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0money', what: 'Dingus Quest Zero\'s money as the HarnMaster books have it (the user\'s, 2026-10-28: DECISIONS.md\'s money, HOUSE.md\'s): the kit\'s 12 silver pennies (a dram each: the pouch\'s weight nothing more than before), examined close their stamps told (the theme\'s mints: 1crypt\'s, four in five the region\'s own, Cendra\'s), their mix kept in the pile\'s record (its groups by mint, fifteenths of it), assessed their worth in pennies (as a local would take them); farthings, a penny cut in four (the user\'s "Please keep farthings"), the money counted ("count the money"), a farthing worth a farthing, one dropped and taken again joined to the other; a stocked old coin\'s mint by its era (themes\' mint lines compiled: dq0themec\'s); the same seed\'s pile the same mix',
      init: 't_rc', cycles: 1100e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm',
        'examine the pennies', 'value the pennies', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm', 'examine the pennies', 'quit', 'dq0 -p t -s 3 -c /ram/c.hm -k farthings,farthings', 'inventory', 'count the money', 'examine the farthings', 'value the farthings', 'drop 1 farthing', 'take the farthing', 'inventory', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], A = dq0Answers(out);
        const said = c => A.filter(a => a.cmd === c).map(a => a.text);
        const ex = said('examine the pennies'), val = said('value the pennies');
        if (ex.length < 2) return ['the pennies not examined twice: ' + JSON.stringify(ex)];
        if (!/Their stamps: (all|most|many|some|a few) of Cendra/.test(ex[0])) f.push('no stamps told, the region\'s own first: ' + ex[0]);
        if (ex[0] !== ex[1]) f.push('the same seed\'s pile not the same mix: ' + ex[0] + ' / ' + ex[1]);
        if (!val.length || !/You judge them worth (about (\d+ pennies|a penny|half a penny|a farthing|three farthings)|nothing)/.test(val[0])) f.push('no worth in pennies: ' + val[0]);
        // (Farthings, the user's, 2026-10-28: a penny cut in four, a coin of its own; the money counted)
        const cnt = said('count the money'), fv = said('value the farthings'), inv = said('inventory');
        if (!cnt.length || !/Your (12 pennies|farthings?) (are|is) in your /.test(cnt[0])) f.push('the money not counted: ' + cnt[0]);
        if (!fv.length || !/worth about (a farthing|half a penny)\./.test(fv[0])) f.push('the farthings not worth a farthing: ' + fv[0]);
        if (inv.length < 2 || !/2 farthings/.test(inv[inv.length - 1])) f.push('the farthing taken again not joined to the other: ' + inv[inv.length - 1]);
        const s = T.of('examine the pennies').after, p = s && dq0Carried(s).find(i => i.name === 'pennies');
        if (!p) f.push('no pennies carried');
        else {
          const g = [p.size, p.thick, p.fuel & 255, p.fuel >> 8].filter(b => b), sum = g.reduce((a, b) => a + (b & 15), 0);
          if (!g.length || (g[0] >> 4) !== 1 || sum !== 15) f.push('the pile\'s groups not kept (the first Cendra\'s, fifteen shares): ' + JSON.stringify(g));
          if (p.qty !== 12 || p.weight !== 1) f.push('the kit\'s pennies: ' + p.qty + ', weight ' + p.weight + ' (a dram each, 12)');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0startkit', outfit: true, near: ['gear'], what: 'Dingus Quest Zero\'s starting light by the family\'s money and its vessels full (the user\'s, 2026-11-03, DECISIONS.md\'s: "Light source by available money", "If you have a waterskin, it should be full to start"; HOUSE.md\'s outfits, for review): a rich serf of 160 pennies a lantern lit in hand and two flasks of oil, no spare torch, its money the less; a serf of 80, a poor serf of 40 and a slave of 40 a torch lit, two spare and a flask; each a tinderbox; its money what\'s left of its wealth at the prices; a waterskin told "(full)" in the list, "full" on a closer look, "full already" to a fill at a well, no time; opened, its 4 pints of water; drunk from when thirsty; told a lantern\'s low words at its last ten minutes, as a torch\'s',
      init: 't_rc', cycles: 2400e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 10 -u feudal -c thief -o /ram/a.hm', 'hmchar -q -s 2 -u feudal -c thief -o /ram/b.hm', 'hmchar -q -s 1 -u feudal -c thief -o /ram/c.hm', 'hmchar -q -s 3 -u feudal -c thief -o /ram/d.hm',
        'dq0 -p t -s 3 -c /ram/a.hm -T', 'inventory', 'examine the waterskin', 'look into the well', 'fill the waterskin', 'open the waterskin', 'look in the waterskin', 'wait 55 minutes', 'wait', 'quit',
        'dq0 -p t -s 3 -c /ram/b.hm -T', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'inventory', 'quit', 'dq0 -p t -s 3 -c /ram/d.hm -T', 'inventory', 'quit', 'echo $status']) },
      start(m) { dq0Poke(m, [{ g: 1, line: 'wait 55 minutes', n: 1, fuel: { lantern: 65 } }]); },
      expect: ['% echo $status'],
      check(m, out) {
        const T = dq0Turns(m), f = [], o = dq0Prose(out).replace(/\[[.#]{10}\] /g, '');
        const games = o.split(/% dq0 -p t -s 3 -c \/ram\/\w\.hm/).slice(1);
        const inv = g => { const i = games[g].indexOf('You carry'); return i < 0 ? '' : games[g].slice(i, games[g].indexOf('In all', i)); };
        const a = inv(0), b = inv(1), c = inv(2), d = inv(3);
        if (!/a lantern \(lit[^)]*\), in your left hand/.test(a) || !/two flasks of oil/.test(a) || /torch/.test(a)) f.push('160 pennies: not a lantern lit and two flasks, no torch: ' + a);
        for (const [s, w] of [[b, 'a poor serf'], [c, 'a slave'], [d, 'a serf of 80 pennies']]) {
          if (!/a torch \(lit[^)]*\), in your left hand/.test(s) || !/two torches/.test(s) || !/a flask of oil/.test(s) || /lantern/.test(s)) f.push(w + ': not a torch lit, two spare and a flask: ' + s);
        }
        for (const [s, w] of [[a, '160 pennies'], [b, 'a poor serf'], [c, 'a slave'], [d, 'a serf of 80 pennies']]) {
          if (!/a tinderbox/.test(s)) f.push(w + ': no tinderbox: ' + s);
          if (!/a waterskin \(full\)/.test(s)) f.push(w + ': its waterskin not told full: ' + s);
        }
        const g = games[0];
        if (!/It's stoppered\..*little worn, full\.|It's stoppered\..*, full\./.test(g)) f.push('a closer look at it not told full');
        if (!g.includes('Your waterskin is full already.')) f.push('a fill of a full waterskin not refused');
        if (!g.includes('revealing 4 pints of water')) f.push('opened, not its 4 pints of water');
        if (!g.includes('Your lantern is burning low.')) f.push('a lantern at its last ten minutes not told');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0newlight', reach: true, near: ['gear'], what: 'Dingus Quest Zero\'s new light lit from the old one (the user\'s, 2026-11-03, DECISIONS.md\'s: "light the new torch from the old one before it is extinguished"; HOUSE.md\'s a new light, for review), by the words in order and the things: "light a new torch" with one burning and a sword in the other hand: the sword set down, the pack off, the new torch got out, lit from the burning one, the old blown out and let fall, the sword taken up, the pack back on; no tinderbox touched; the new one lit in hand, the old unlit at its feet, each step its time (over 6 seconds); with the old one spent, the tinderbox\'s steps as before; a lantern from a torch, by "light the lantern"; and "turn" by its way, right, left, half left, half right, around, and by a compass way: "You turn to face west." always (the user\'s, 2026-11-16: whatever\'s typed after it; "You turn right, to face west." before, its way read from the command queue)',
      init: 't_rc', cycles: 1500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k sword', 'light a new torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'wait 61 minutes', 'light a new torch', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k lantern', 'light the lantern', 'inventory', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -T', 'turn r', 'turn left', 'turn half left', 'turn half right', 'turn around', 'face west', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Prose(out).replace(/\[[.#]{10}\] /g, '');
        const games = o.split(/% dq0 -p t -s 3 -c \/ram\/c\.hm/).slice(1), T = dq0Turns(m);
        if (games.length !== 4) return ['not four games: ' + games.length];
        inTurn(games[0], ['You set the sword down at your feet.', 'You take off the backpack', 'You get out the new torch.', 'You light the new torch from the burning one.',
          'You put out the old torch.', 'You let the old torch fall.', 'You take up the sword.', 'You put on the backpack.'], f, 'a new torch: ');
        if (/tinderbox/.test(games[0].slice(0, games[0].indexOf('You carry')))) f.push('a new torch lit with one burning: the tinderbox touched');
        const inv = games[0].slice(games[0].indexOf('You carry'));
        if (!/a torch \(lit[^)]*\), in your left hand/.test(inv) || !/a sword, in your right hand/.test(inv)) f.push('after it, not a torch lit and the sword each in its own hand: ' + inv.slice(0, 400));
        const t0 = T.of('light a new torch'), secs = t0.at && t0.after ? t0.after.now - t0.at.now : 0;
        if (!(secs >= 60)) f.push('a new torch\'s steps took ' + secs + ' tenths, not their time');
        if (!/You let the torch fall\./.test(games[1]) || !/You strike a spark\./.test(games[1])) f.push('the old one spent: not let fall and the tinderbox\'s steps: ' + games[1].slice(-900));
        if (!/You light the lantern\./.test(games[2]) || /strike a spark/.test(games[2])) f.push('a lantern not lit from the torch burning: ' + games[2].slice(-600));
        inTurn(games[3], ['You turn to face north.', 'You turn to face west.', 'You turn to face southwest.', 'You turn to face west.', 'You turn to face east.', 'You turn to face west.'], f, 'turning: ');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0chiselhands', outfit: false, reach: true, near: ['gear', 'map'], what: 'Dingus Quest Zero\'s hammer and chisel in each hand (the user\'s, 2026-11-03, DECISIONS.md\'s: "Yes, hammer and chisel in each hand", and A25: "it should probably put the other things away first (in their appropriate place) then take the tools in hand"; HOUSE.md\'s digging, readying with busy hands): at seed 374, facing a wall, a lit torch in hand, the hammer and chisel in the pack: "dig north" lays the torch down lit, gets out both, digs by its light; a shovel carried beside them not the tool got out; no hammer, refused as ever',
      init: 't_rc', cycles: 2500e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 374 -c /ram/c.hm -k chisel,hammer,shovel -T', 'dig north', 'stop', 'inventory', 'look', 'quit', 'dq0 -p t -s 374 -c /ram/c.hm -k chisel -T', 'dig north', 'quit', 'echo $status']) },
      start(m) {
        dq0OnWords(m, [{ g: 1, line: 'dig north', n: 1, words: 'You dig', text: 'stop\r' }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Story(out).replace(/\([A-Z]{1,2}\) /g, '').replace(/\[?[#.]{2,}\]? ?/g, ' ').replace(/\s+/g, ' '), T = dq0Turns(m);
        this.notes = [o.slice(o.indexOf('> dig north')).slice(0, 900)];
        if (!/You lay the torch on the ground, still burning\./.test(o)) f.push('the torch in hand not laid down lit');
        if (!/You get out the hammer\./.test(o)) f.push('the hammer not got out');   // (dq0 -k's chisel in hand already)
        if (/get out the shovel/.test(o)) f.push('the shovel got out for a chisel\'s work');
        if (!/(a hammer, in your [a-z]+ hand \(\w+\) a chisel, in your [a-z]+ hand|a chisel, in your [a-z]+ hand \(\w+\) a hammer, in your [a-z]+ hand)/.test(o)) f.push('stopped, not both in hand');
        if (!/At your feet lies? a lit torch/.test(o)) f.push('the torch not told lying lit');
        const digs = T.filter(t => t.line === 'dig north');
        if (!digs[1] || !dq0Ev(digs[1], 'E_DIG', { how: 'D_NO_TOOL' }).length) f.push('no hammer, not refused');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0versions', near: ['gear'], what: 'Dingus Quest Zero\'s weapon versions by weight (D13\'s: the book\'s custom weapons, a light one a quarter lighter, a heavy one a quarter heavier; 2026-11-03, a light one\'s negated weight shifted unsigned made a Strength 5 character\'s sword about 4,098 pounds, unplayable): at seed 3 a warhammer (5 lb, 20 quarter pounds), hefted, and its load on the sheet: average 5 lb, made light (dq0Poke, its size) under 4, made heavy over 6, the sheet\'s load the same way, never thousands; the inventory\'s "In all" the sheet\'s load each time, a version counted, a thing put down off both',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm -T -k warhammer',
        'heft the warhammer', 'sheet', 'inventory', 'wait', 'drop the warhammer', 'take the warhammer', 'heft the warhammer', 'sheet', 'inventory', 'wait', 'drop the warhammer', 'take the warhammer', 'heft the warhammer', 'sheet', 'inventory', 'drop the warhammer', 'sheet', 'inventory', 'open the waterskin', 'quit', 'echo $status']) },
      start(m) {
        dq0Poke(m, [{ g: 1, line: 'wait', n: 1, item: { warhammer: { size: 1 } } }, { g: 1, line: 'wait', n: 2, item: { warhammer: { size: 2 } } }]);
      },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], o = dq0Prose(out);
        const hefts = [...o.matchAll(/Your warhammer weighs about ([\d,]+) pounds?|Your warhammer weighs (under a pound|about a pound)/g)].map(x => +(x[1] || '1').replace(/,/g, ''));
        const loads = [...o.matchAll(/load ([\d,]+) lb/g)].map(x => +x[1].replace(/,/g, ''));
        this.notes = ['hefted ' + hefts.join(', ') + ' lb; the sheet\'s load ' + loads.join(', ') + ' lb'];
        if (hefts.length !== 3) return ['not hefted three times: ' + hefts.join(', ') + ': ' + o.slice(-1500)];
        if (hefts[0] !== 5) f.push('an average warhammer ' + hefts[0] + ' lb, not 5');
        if (!(hefts[1] >= 3 && hefts[1] <= 4)) f.push('a light warhammer ' + hefts[1] + ' lb, not a quarter under 5');
        if (!(hefts[2] >= 6 && hefts[2] <= 7)) f.push('a heavy warhammer ' + hefts[2] + ' lb, not a quarter over 5');
        const alls = [...o.matchAll(/In all, ([\d,]+) lb/g)].map(x => +x[1].replace(/,/g, ''));   // (One path: the inventory's total the sheet's, a version and all, what's put down too)
        if (alls.join() !== loads.join()) f.push('the inventory\'s totals ' + alls.join(', ') + ' lb, the sheet\'s load ' + loads.join(', '));
        if (loads.length !== 4) return f.concat(['the sheet\'s load not told four times: ' + loads.join(', ')]);
        if (!(loads[1] < loads[0] && loads[0] - loads[1] <= 2)) f.push('the sheet\'s load, light: ' + loads[1] + ' lb, average ' + loads[0]);
        if (!(loads[2] > loads[0] && loads[2] - loads[0] <= 2)) f.push('the sheet\'s load, heavy: ' + loads[2] + ' lb, average ' + loads[0]);
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0load', what: 'Dingus Quest Zero\'s load (the user\'s, 2026-10-16), by the things\' weights (the catalogue\'s, all the character carries) and the referee\'s summary (its load: HarnMaster\'s encumbrance, the load over its endurance, as the game\'s bands): the kit alone 9 lb (its spare torch and flask of oil, D5\'s, slice 4.1), no burden (within its free pounds by Strength, D13\'s, slice 4.4); with a pick, a shovel, a rope, a sword and bones (dq0 -k) 35 lb, a little burdened; some put down, 17 lb, a little burdened still (the sheet\'s penalties the referee\'s, its unit tests\'; the words: dq0wordsthings)',
      init: 't_rc', cycles: 700e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'quit',
        'dq0 -p t -s 3 -c /ram/c.hm -k pick,shovel,rope,sword,bones', 'drop rope', 'drop shovel', 'drop sword', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m) {
        // (A thing's weight its kind's, in quarter pounds, times how many but a plural kind's (K_PLURAL: 3 rations one lot;
        // coins by the dram, 64 to the quarter pound, items.h's LOT_WT, 2026-10-28):
        // the catalogue's, as the things weigh them)
        const T = dq0Turns(m), f = [], lb = s => dq0Carried(s).reduce((w, i) => w + ((i.kform & 0x12) === 0x12 ? (i.weight * i.qty + 32) >> 6 : i.weight * (i.kflags & 0x40 ? 1 : Math.max(1, i.qty))), 0) / 4, load = s => s ? s.sum.load : -1;
        const k = T.of('dq0 -p t -s 3 -c /ram/c.hm').after, h = T.of('dq0 -p t -s 3 -c /ram/c.hm -k pick,shovel,rope,sword,bones').after, d = T.of('drop sword').after;
        for (const [s, w, l, what] of [[k, 9, 0, 'the kit alone'], [h, 35, 1, 'with the things more'], [d, 17, 1, 'some put down']]) {
          if (!s || lb(s) !== w) f.push(what + ': ' + (s && lb(s)) + ' lb carried, not ' + w);
          if (load(s) !== l) f.push(what + ': load ' + load(s) + ', not ' + l + ' (1 a little burdened, 2 heavily)');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0heft', outfit: true, near: ['gear', 'fight'], what: 'Dingus Quest Zero\'s weapons\' versions by Strength (D13, the user\'s: Strength sets how heavy a weapon is wielded without penalty, heavier versions hit harder, strong characters made with heavier weapons; the book\'s custom weapons, Combat 3; HOUSE.md\'s the character carries on), by the words told and the referee\'s marks: a hunter (hmchar\'s, seed 5) made strong (Strength 16, its file\'s line added) begins with its weapons heavy (its knife appraised "a heavy one"; RS_EQUIP\'s marks an H after its quality), made weak (Strength 4) with them light ("a light one")',
      init: 't_rc', cycles: 900e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm', 'cp /ram/c.hm /ram/w.hm', 'echo \'att str 16\' >> /ram/c.hm',
        'echo \'att str 4\' >> /ram/w.hm', 'dq0 -p t -s 3 -c /ram/c.hm', 'inventory', 'appraise the knife', 'quit', 'dq0 -p t -s 3 -c /ram/w.hm', 'inventory', 'appraise the knife', 'quit', 'echo $status']) },
      expect: ['% echo $status'],
      check(m, out) {
        const f = [], a = out.indexOf('% dq0 -p t -s 3 -c /ram/c.hm'), b = out.indexOf('% dq0 -p t -s 3 -c /ram/w.hm');
        if (!/It's a heavy one/.test(out.slice(a, b))) f.push('the strong character\'s knife not heavy');
        if (!/It's a light one/.test(out.slice(b))) f.push('the weak character\'s knife not light');
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0emptyplace', near: ['gear'], what: 'Dingus Quest Zero\'s empty, place and named tool (the user\'s P4, P5, P6, 2026-11-18): at seed 3 (-T, a kit of a chest, its key, the lockpicks, a sack and a table) "unlock the chest with the lockpicks" picks it though its key fits, plain "unlock" its key; "empty the sack" puts its things down at the feet, empty again "The sack is empty.", onto the table put on it, a flask\'s oil poured away, a torch refused, a sack tied refused; "put the key in the corner" walks there and puts it down on that square, "in the middle of the room" the same, no ledge here told',
      init: 't_rc', cycles: 600e6 + EMPTYPLACE.length * 150e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'chest,key,lockpicks,sack,table\'', ...EMPTYPLACE, 'quit', 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); },
      expect: ['% echo $status'],
      check(m) {
        const A = dq0Answers(m.out), f = [], a = (l, n = 0) => (A.filter(x => x.cmd === l)[n] || { text: '(no answer)' }).text;
        if (process.env.DQ0_EMPTY) for (const x of A) console.error(x.cmd + ' => ' + x.text);
        const want = (l, re, why, n) => { if (!re.test(a(l, n))) f.push('"' + l + '" ' + why + ': ' + a(l, n).slice(0, 200)); };
        want('unlock the chest with the lockpicks', /You set to picking the lock\./, 'not picked (P6: the named tool wins)');
        if (/with its key/.test(a('unlock the chest with the lockpicks'))) f.push('"unlock the chest with the lockpicks": its key used');
        want('unlock the chest', /You unlock the chest with its key\./, 'not unlocked by its key (plain unlock)');
        want('empty the sack', /You empty the sack out at your feet\./, 'not emptied at its feet');
        want('empty the sack', /The sack is empty\./, 'not told empty', 1);
        want('empty the sack onto the table', /You empty the sack onto the table\./, 'not emptied onto the table');
        want('empty the flask of oil', /You pour the oil out of the flask of oil\./, 'its oil not poured away');
        want('empty the torch', /The torch isn't a thing to empty\./, 'not refused');
        want('empty the sack', /The sack is closed\./, 'a tied sack not refused', 2);
        want('put the key in the corner', /You put the iron key in the corner\./, 'not put in the corner');
        want('put the dagger in the middle of the room', /^You set off across the room\..*You put the dagger in the middle\./,
          'not walked there and put in the middle');
        want('put the key on the ledge', /You haven't seen a ledge here\./, 'no ledge not told');
        // (Each line's first letter held to D17's, the test's own typing; a tool named by its word too, the lockpicks: found
        // in what it carries, the UI's copy, letters.c's o_des_low, 2026-11-18)
        this.notes = [];
        for (const r of this.timer.game(0, EMPTYPLACE.map(l => ({ line: l, want: 'You|The' })))) {
          this.notes.push(r.line + ': first letter ' + r.first);
          if (target(this, r.line + ': first letter', r.first, D17.first)) f.push(r.line + ': its first letter ' + r.first + ' cycles from Enter (the user\'s 720,000)');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0pour', near: ['gear'], what: "Dingus Quest Zero's pouring from one container into another (the user's H11, 2026-11-19): at seed 2 (-T, a kit of an iron pot, a vial and a sack; the waterskin filled in the water down the stair, dq0things' way) 'pour the water into the vial' fills it, a pint, the rest left in the waterskin (told); the vial poured out, then the waterskin emptied into it, the same; emptied into the iron pot (on the floor), all of it, the vial's joined to it; refused: into the sack (holds no liquid), into a closed vial, into a full one; 'pour the water' pours the pot's away; each pour's and empty's first letter held to D17",
      init: 't_rc', cycles: 600e6 + POUR.length * 150e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'mkdir /ram/g', 'dq0 -p t -s 2 -c /ram/c.hm -T -d /ram/g -k \'iron pot,vial,sack\'', ...POUR, 'quit', 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); },
      expect: ['% echo $status'],
      check(m) {
        const A = dq0Answers(m.out), f = [], a = (l, n = 0) => (A.filter(x => x.cmd === l)[n] || { text: '(no answer)' }).text;
        if (process.env.DQ0_POUR) for (const x of A) console.error(x.cmd + ' => ' + x.text);
        const want = (l, re, why, n) => { if (!re.test(a(l, n))) f.push('"' + l + '" ' + why + ': ' + a(l, n).slice(0, 200)); };
        want('pour the water into the vial', /You pour a pint of the water into the vial: it's full, and the rest stays in the waterskin\./, 'not poured in, the rest kept');
        want('empty the waterskin into the vial', /You pour a pint of the water into the vial: it's full, and the rest stays in the waterskin\./, 'not poured in again', 0);
        want('empty the waterskin into the iron pot', /You pour the water into the iron pot\./, 'not poured into the pot');
        want('empty the vial into the iron pot', /You pour the water into the iron pot\./, "the vial's not joined in the pot");
        want('empty the waterskin into the vial', /The waterskin is empty\./, 'an empty waterskin not told', 1);
        want('empty the iron pot into the sack', /The sack won't hold water\./, 'the sack not refused');
        want('empty the iron pot into the vial', /The vial is closed\./, 'a closed vial not refused');
        want('pour the water', /You pour the water out of the iron pot\./, "the pot's water not poured away");
        if (/reach for the waterskin/.test(a('pour the water'))) f.push('"pour the water": its first words the empty waterskin\'s: ' + a('pour the water'));
        this.notes = [];
        for (const r of this.timer.game(0, POUR.filter(l => /^(pour|empty) /.test(l)).map(l => ({ line: l, want: 'You|The' })))) {
          this.notes.push(r.line + ': first letter ' + r.first);
          if (target(this, r.line + ': first letter', r.first, D17.first)) f.push(r.line + ': its first letter ' + r.first + ' cycles from Enter (the user\'s 720,000)');
        }
        return f;
      },
      sdcard: true
    },
    {
      name: 'dq0oil', near: ['gear'], what: "Dingus Quest Zero's lamp oil a liquid of its own (the user's H14, 2026-11-19): at seed 3 (-T, a kit of a lantern, a flask of oil, a vial and an iron pot) the flask's oil poured into the vial (a pint of lamp oil there), the vial emptied into the pot on the floor, the pot's emptied back into the vial, the lantern lit and burnt an hour, then filled from the vial (its oil the less), the rest poured back into the flask, the vial empty after ('The vial is empty.'); the flask's poured into the waterskin, which then won't be filled with water (two liquids don't mix, H13), and poured out; the flask empty; each pour's, empty's and fill's first letter held to D17",
      init: 't_rc', cycles: 600e6 + OIL.length * 150e6,
      machine: { input: dq0Typed(['bind -a /sd/0/games /bin', 'hmchar -q -s 5 -u feudal -c hunter -o /ram/c.hm',
        'dq0 -p t -s 3 -c /ram/c.hm -T -k \'lantern,flask of oil,vial,iron pot\'', ...OIL, 'quit', 'echo $status']) },
      start(m) { this.timer = dq0Timer(m); },
      expect: ['% echo $status'],
      check(m) {
        const A = dq0Answers(m.out), f = [], a = (l, n = 0) => (A.filter(x => x.cmd === l)[n] || { text: '(no answer)' }).text;
        if (process.env.DQ0_OIL) for (const x of A) console.error(x.cmd + ' => ' + x.text);
        const want = (l, re, why, n) => { if (!re.test(a(l, n))) f.push('"' + l + '" ' + why + ': ' + a(l, n).slice(0, 200)); };
        want('pour the oil into the vial', /You pour the oil into the vial\./, "the flask's oil not poured into the vial");
        want('empty the vial into the iron pot', /You pour the oil into the iron pot\./, 'not poured into the pot');
        want('empty the iron pot into the vial', /You pour the oil into the vial\./, "the pot's oil not poured back");
        want('fill the lantern from the vial', /You fill the lantern from the vial\./, 'the lantern not filled from the vial');
        want('pour the oil into the flask of oil', /You pour the oil into the flask of oil\./, 'the rest not poured back into the flask');
        want('empty the vial', /The vial is empty\./, 'the vial not empty after');
        want('pour the oil into the waterskin', /You pour the oil into the waterskin\./, "the flask's oil not poured into the waterskin");
        want('fill the waterskin', /You'd have to empty the waterskin first\./, 'water filled on its oil');
        want('empty the waterskin', /You pour the oil out of the waterskin\./, "the waterskin's oil not poured out");
        want('empty the flask', /The flask of oil is empty\./, 'an empty flask not told');
        this.notes = [];
        for (const r of this.timer.game(0, OIL.filter(l => /^(pour|empty|fill) /.test(l)).map(l => ({ line: l, want: 'You|The' })))) {
          this.notes.push(r.line + ': first letter ' + r.first);
          if (target(this, r.line + ': first letter', r.first, D17.first)) f.push(r.line + ': its first letter ' + r.first + " cycles from Enter (the user's 720,000)");
        }
        return f;
      },
      sdcard: true
    },
  ],
};
