const fs = require('fs');
let s = fs.readFileSync('check.js', 'utf8');
const rep = (a, b) => { if (!s.includes(a)) throw new Error('missing: ' + a.slice(0, 60)); s = s.replace(a, b); };
rep("    b.prompts = m[1] ? m[1].split(/\s+/).map(p => new RegExp('^(' + p.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace(/_/g, ' ') + ')')) : PROMPTS;",
    "    b.prompts = (m[1] ? m[1].split(/\s+/).map(p => new RegExp('^(' + p.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace(/_/g, ' ') + ')')) : []).concat(PROMPTS);");
rep("// What a block types: its lines after a prompt\nfunction typed(b) {\n  const t = [];\n  for (const l of b.lines) {\n    if (l.startsWith('\u00ab')) continue;",
    "// What a block types: its lines after a prompt, and its » lines (typed with no prompt: !line)\nfunction typed(b) {\n  const t = [];\n  for (const l of b.lines) {\n    if (l.startsWith('\u00ab')) continue;\n    if (l.startsWith('\u00bb ')) { t.push('!line ' + l.slice(2)); continue; }");
rep("      const want = b.lines.filter(l => !l.startsWith('\u00ab')).map(l => l.replace(/\s+$/, ''));",
    "      const want = b.lines.filter(l => !l.startsWith('\u00ab')).map(l => l.replace(/^\u00bb /, '').replace(/\s+$/, ''));");
rep("      fixes.push({ b, got });",
    "      // (The » lines marked again, in order)\n      const raws = b.lines.filter(l => l.startsWith('\u00bb ')).map(l => l.slice(2).replace(/\s+$/, ''));\n      const marked = got.map(l => raws.length && l === raws[0] ? '\u00bb ' + raws.shift() : l);\n      fixes.push({ b, got: marked });");
rep("//   A block whose fence names its own prompts (```session db>) uses them; a «...» line is a note, not output.",
    "//   A block's fence can name more prompts (```session * for ed's); a «...» line is a note, not output; a line\n//   \"» text\" is typed with no prompt before it (ed's lines being added, a program's input).");
fs.writeFileSync('check.js', s);
let c = fs.readFileSync('capture.js', 'utf8');
const crep = (a, b) => { if (!c.includes(a)) throw new Error('missing: ' + a.slice(0, 60)); c = c.replace(a, b); };
crep("    else if (it === '!quiet') settle(false);", "    else if (it === '!quiet') settle(false);\n    else if (it.startsWith('!line ')) { settle(false); type(keys(it.slice(6)) + '\r'); }");
crep("//               the output to stop; \"!snap NAME\"", "//               the output to stop; \"!line TEXT\" types TEXT and Return once the output stops; \"!snap NAME\"");
fs.writeFileSync('capture.js', c);
