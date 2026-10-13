// kwsrc.js KEYWORD ...: what HyBASIC's source says of each keyword, for the Technical Reference's writer: the comment
// blocks of reborn/modules/basic that name the keyword in capitals as a word, each with its file and line (run from the
// repository's root; KW_MAX=n: at most n blocks a keyword, 14 if it isn't set)
'use strict';
const fs = require('fs');
const path = require('path');
const dir = path.join(process.cwd(), 'reborn', 'modules', 'basic');
const files = fs.readdirSync(dir).filter(f => /\.(inc|s)$/.test(f));
const src = files.map(f => ({ f, L: fs.readFileSync(path.join(dir, f), 'utf8').split(/\r?\n/) }));
const max = +(process.env.KW_MAX || 14);
for (const kw of process.argv.slice(2)) {
  const re = new RegExp('(^|[^A-Za-z_$])' + kw.replace(/[$.*+?^(){}|[\]\\]/g, '\\$&') + '($|[^A-Za-z_$0-9])');
  const out = [];
  for (const { f, L } of src) {
    for (let i = 0; i < L.length; i++) {
      if (!/^\s*;/.test(L[i]) || !re.test(L[i])) continue;
      // the whole comment block around it (full-line comments only)
      let a = i, b = i;
      while (a > 0 && /^;/.test(L[a - 1])) a--;
      while (b + 1 < L.length && /^;/.test(L[b + 1])) b++;
      if (/^;/.test(L[i])) { out.push(f + ':' + (a + 1) + '  ' + L.slice(a, b + 1).map(s => s.replace(/^;\s?/, '')).join(' ').replace(/\s+/g, ' ').slice(0, 700)); i = b; }
      else out.push(f + ':' + (i + 1) + '  ' + L[i].trim().replace(/\s+/g, ' ').slice(0, 200));
    }
  }
  console.log('=== ' + kw + ' (' + out.length + ')');
  for (const o of [...new Set(out)].slice(0, max)) console.log('  ' + o);
}
