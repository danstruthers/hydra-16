// worlds.js - the worlds (../WORLDS.md): a folder here each, its name the world's (dq0 -w NAME, themegen -w NAME; 8
// letters at most, the save's), its files history.txt (themegen's make.js: lib/dq0/worlds/NAME/history.dat), peoples.txt
// and deities.txt (dq0d's make.js: peoples.dat, deities.dat and gods.txt).  The default world, a game's with no -w, the
// first; its deities' names the catalogue's bank's too (dq0w's make.js: obj/games/dq0g.txt), so a game in it opens no
// file of them.  The build's alone: nothing on the card reads this.
'use strict';
const fs = require('fs');
const path = require('path');

const DEFAULT = 'first';
const names = () => fs.readdirSync(__dirname, { withFileTypes: true }).filter(d => d.isDirectory()).map(d => d.name).sort()
  .map(n => {
    if (!/^[a-z][a-z0-9]{0,7}$/.test(n)) throw new Error('games/dq0/worlds/' + n + ': a world\'s name 1-8 small letters and digits');
    return n;
  });

module.exports = { DEFAULT, names, dir: n => path.join(__dirname, n) };
