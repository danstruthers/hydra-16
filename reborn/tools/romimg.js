// romimg.js - HydraOS's paged ROM image: the base's (../base/tools/romimg.js: the module directory, the hardware test,
// the modules), with HydraOS's ROM disk after them (tools/romfs.js), so the build, the tests and the tools call it as
// they always have: build({ modules, init, hwtest, bios, romfs: romfs.js's files }).
'use strict';
const romimg = require('../../base/tools/romimg.js');
const romfs = require('./romfs.js');

module.exports = Object.assign({}, romimg, { build: opt => romimg.build(Object.assign({ romfsLib: romfs }, opt)) });
