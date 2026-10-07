#!/usr/bin/env node
// image.js - a test's paged ROM image (as sim/test.js builds it: the system's modules and the test's own, its init),
// written to a file for the danlang emulator (sim/dl/run.dl).
//
// Usage: node sim/dl/image.js TEST FILE
'use strict';
const fs = require('fs');
const path = require('path');
const { image } = require('../test.js');
const { tests } = require('../../tests/tests.js');

const [name, file] = process.argv.slice(2);
const t = tests.find(t => t.name === name);
if (!t || !file) { console.error('usage: node sim/dl/image.js TEST FILE' + (name && !t ? '  (no test ' + name + ')' : '')); process.exit(2); }
fs.mkdirSync(path.dirname(path.resolve(file)), { recursive: true });
fs.writeFileSync(file, image(t));
