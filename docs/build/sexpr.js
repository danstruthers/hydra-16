// sexpr.js - an S-expression reader, for KiCad's files (docs/build/bom.js, layout.js): parse(text) gives a list as an
// array, an atom as a string (a quoted one unquoted).
'use strict';

function parse(text) {
  let i = 0;
  const ws = () => { while (i < text.length && /\s/.test(text[i])) i++; };
  function value() {
    ws();
    if (text[i] === '(') {
      i++;
      const list = [];
      for (;;) {
        ws();
        if (text[i] === ')') { i++; return list; }
        if (i >= text.length) throw new Error('unbalanced');
        list.push(value());
      }
    }
    if (text[i] === '"') {
      let s = '';
      for (i++; text[i] !== '"'; i++) {
        if (text[i] !== '\\') { s += text[i]; continue; }
        const e = text[++i];                                    // (An escape: \n a new line, else the character)
        s += e === 'n' ? '\n' : e;
      }
      i++;
      return s;
    }
    const start = i;
    while (i < text.length && !/[\s()]/.test(text[i])) i++;
    return text.slice(start, i);
  }
  return value();
}

module.exports = { parse };
