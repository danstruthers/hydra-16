// worlds.js - the worlds (../WORLDS.md): a folder here each, its name the world's (dq0 -w NAME, themegen -w NAME; 8
// letters at most, the save's), its files history.txt (themegen's make.js: history.dat), peoples.txt and deities.txt
// (dq0d's make.js: peoples.dat, deities.dat and gods.txt), on the card in lib/dq0/worlds/NAME.  The default world, a
// game's with no -w: the first (theme.h's WORLD_DEFAULT), its files on the card lib/dq0's own as they were
// (themegen.dat, dq0f.dat, dq0g.dat: sdcard.txt's); its deities' names the catalogue's bank's too (dq0w's make.js:
// obj/games/dq0g.txt), so a game in it opens no file of them.  The build's alone: nothing on the card reads this.
'use strict';
const fs = require('fs');
const path = require('path');

const DEFAULT = /#define\s+WORLD_DEFAULT\s+"(\w+)"/.exec(fs.readFileSync(path.join(__dirname, '..', 'theme.h'), 'latin1'))[1];   // (theme.h's)
const names = () => fs.readdirSync(__dirname, { withFileTypes: true }).filter(d => d.isDirectory()).map(d => d.name).sort()
  .map(n => {
    if (!/^[a-z][a-z0-9]{0,7}$/.test(n)) throw new Error('games/dq0/worlds/' + n + ': a world\'s name 1-8 small letters and digits');
    return n;
  });

module.exports = { DEFAULT, names, dir: n => path.join(__dirname, n) };
