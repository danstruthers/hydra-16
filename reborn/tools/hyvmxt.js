// hyvmxt.js (node tools/hyvmxt.js modules/hylang/vmxt.inc): writes modules/hylang/vmxt.inc, the native code's templates (vmx.inc's), from the descriptions here.
// A template's code is ca65's, with {B n} (the op's data byte n), {U n} (byte n, bit 0 clear: a fixnum's low byte,
// untagged), {T n} (the native place of the jump target in data bytes n, n + 1), {N} (the next op's entry) where an
// operand goes; @x labels are its own.  Each: .byte len, flags, patches; then (offset, kind, data byte) each; its code
const fs = require('fs');
const out = process.argv[2];
const T = [];
const tpl = (name, flags, code) => T.push({ name, flags, code });
const STUB = 'TF_STUB', NOR = 'TF_NOR';

// ---- Plain ops
tpl('const', '0', `
  lda #{B 1}
  sta ex
  lda #{B 2}
  sta ex + 1`);
const PUSHEX = `
  lda ex
  sta (sp)
  ldy #1
  lda ex + 1
  sta (sp),y
  lda sp
  clc
  adc #2
  sta sp
  bne :+
  jsr vm_page
:`;
tpl('push', '0', PUSHEX);
tpl('cpush', '0', `
  lda #{B 1}
  sta (sp)
  ldy #1
  lda #{B 2}
  sta (sp),y
  lda sp
  clc
  adc #2
  sta sp
  bne :+
  jsr vm_page
:`);
tpl('local', `${STUB} | ${NOR}`, `
  bit vm_mat
  bmi @s
  ldy #{B 1}
  lda (vm_s),y
  sta ex
  iny
  lda (vm_s),y
  sta ex + 1
  jmp {N}
@s:`);
tpl('lpush', `${STUB} | ${NOR}`, `
  bit vm_mat
  bmi @s
  ldy #{B 1}
  lda (vm_s),y
  sta (sp)
  iny
  lda (vm_s),y
  ldy #1
  sta (sp),y
  lda sp
  clc
  adc #2
  sta sp
  bne :+
  jsr vm_page
:
  jmp {N}
@s:`);
tpl('setl', STUB, `
  bit vm_mat
  bmi @s
  ldy #{B 1}
  lda ex
  sta (vm_s),y
  iny
  lda ex + 1
  sta (vm_s),y
  stz ex
  stz ex + 1
  jmp {N}
@s:`);
tpl('jmp', '0', `
  jmp {T 1}`);
tpl('jf', '0', `
  lda ex + 1
  bne @no
  lda ex
  beq @to
  cmp #<SNIL
  bne @no
@to:
  jmp {T 1}
@no:`);
tpl('jt', '0', `
  lda ex + 1
  bne @to
  lda ex
  beq @no
  cmp #<SNIL
  beq @no
@to:
  jmp {T 1}
@no:`);
tpl('loop', STUB, `
  bit intr
  bmi @s
  jmp {T 1}
@s:`);
tpl('doti', '0', `
  lda sp
  sec
  sbc #4
  sta ht
  lda sp + 1
  sbc #0
  sta ht + 1
  ldy #2
  lda (ht),y
  sta ex
  cmp (ht)
  iny
  lda (ht),y
  sta ex + 1
  ldy #1
  sbc (ht),y
  bvc :+
  eor #$80
:
  bmi @in
  jmp {T 1}
@in:`);
tpl('dotinc', '0', `
  lda sp
  sec
  sbc #2
  sta ht
  lda sp + 1
  sbc #0
  sta ht + 1
  lda (ht)
  clc
  adc #2
  sta (ht)
  ldy #1
  lda (ht),y
  adc #0
  sta (ht),y`);
tpl('stt', '0', `
  lda sp
  sec
  sbc #2
  sta ht
  lda sp + 1
  sbc #0
  sta ht + 1
  lda ex
  sta (ht)
  ldy #1
  lda ex + 1
  sta (ht),y`);
tpl('popx', '0', `
  jsr pop
  sta ex
  stx ex + 1`);
tpl('drop', '0', `
  lda #{B 1}
  jsr drop
  stz ex
  stz ex + 1`);

// ---- The fused ops: their tests (true: ex = T, on; false: ex = NIL, to the target), their arithmetic (ex; LQP and
// LL's bit 0: pushed)
const TRUEFALSE = (t) => `
  lda #<T_VAL
  sta ex
  stz ex + 1
  jmp {N}
@f:
  stz ex
  stz ex + 1
  jmp {T ${t}}
@s:`;
// A local's value (data byte yb) to ht, a fixnum (else the stub)
const LOADY = (yb) => `
  bit vm_mat
  bmi @s
  ldy #{B ${yb}}
  lda (vm_s),y
  sta ht
  lsr
  bcc @s
  iny
  lda (vm_s),y
  sta ht + 1`;
const LOADEX = `
  lda ex
  sta ht
  lsr
  bcc @s
  lda ex + 1
  sta ht + 1`;
// ht against the constant at data bytes c, c + 1 (signed): N = 1 if ht < c (lt), c < ht (gt)
const CMPC = (kind, c) => kind === 'lt' ? `
  lda ht
  cmp #{B ${c}}
  lda ht + 1
  sbc #{B ${c + 1}}
  bvc :+
  eor #$80
:` : `
  lda #{B ${c}}
  cmp ht
  lda #{B ${c + 1}}
  sbc ht + 1
  bvc :+
  eor #$80
:`;
const TESTC = (load, c, t, kind) => {
  if (kind === 'zerop' || kind === 'onep' || kind === 'eqc') {
    const lo = kind === 'zerop' ? '<FIX(0)' : kind === 'onep' ? '<FIX(1)' : `{B ${c}}`;
    const hi = kind === 'zerop' || kind === 'onep' ? '0' : `{B ${c + 1}}`;
    return load + `
  lda ht
  cmp #${lo}
  bne @f
  lda ht + 1
  cmp #${hi}
  bne @f` + TRUEFALSE(t);
  }
  const m = { ltc: ['lt', 'bpl'], gec: ['lt', 'bmi'], gtc: ['gt', 'bpl'], lec: ['gt', 'bmi'] }[kind];
  return load + CMPC(m[0], c) + `
  ${m[1]} @f` + TRUEFALSE(t);
};
for (const k of ['zerop', 'onep', 'ltc', 'gtc', 'lec', 'gec', 'eqc']) {
  tpl('jlq_' + k, STUB, TESTC(LOADY(2), 3, 5, k));
  tpl('jq_' + k, STUB, TESTC(LOADEX, 2, 4, k));
}
// LQ's (and LQP's) arithmetic: ex = ht + or - the constant (data 3, 4), 1+, 1-
const ARITH = {
  addc: `
  lda ht
  clc
  adc #{U 3}
  sta ex
  lda ht + 1
  adc #{B 4}
  bvs @s
  sta ex + 1`,
  subc: `
  lda ht
  sec
  sbc #{U 3}
  sta ex
  lda ht + 1
  sbc #{B 4}
  bvs @s
  sta ex + 1`,
  inc: `
  lda ht
  clc
  adc #2
  sta ex
  lda ht + 1
  adc #0
  bvs @s
  sta ex + 1`,
  dec: `
  lda ht
  sec
  sbc #2
  sta ex
  lda ht + 1
  sbc #0
  bvs @s
  sta ex + 1`,
};
for (const k of Object.keys(ARITH)) {
  tpl('lq_' + k, STUB, LOADY(2) + ARITH[k] + `
  jmp {N}
@s:`);
  tpl('lqp_' + k, STUB, LOADY(2) + ARITH[k] + PUSHEX + `
  jmp {N}
@s:`);
}
// LL's: the two locals (data 2, 3) to ht and hp, fixnums
const LOAD2 = `
  bit vm_mat
  bmi @s
  ldy #{B 2}
  lda (vm_s),y
  sta ht
  iny
  lda (vm_s),y
  sta ht + 1
  ldy #{B 3}
  lda (vm_s),y
  sta hp
  and ht
  lsr
  bcc @s
  iny
  lda (vm_s),y
  sta hp + 1`;
const LLA = {
  add: `
  lda hp
  and #$FE
  clc
  adc ht
  sta ex
  lda hp + 1
  adc ht + 1
  bvs @s
  sta ex + 1`,
  sub: `
  lda hp
  and #$FE
  sta hn
  lda ht
  sec
  sbc hn
  sta ex
  lda ht + 1
  sbc hp + 1
  bvs @s
  sta ex + 1`,
};
for (const k of Object.keys(LLA)) {
  tpl('ll_' + k, STUB, LOAD2 + LLA[k] + `
  jmp {N}
@s:`);
  tpl('llp_' + k, STUB, LOAD2 + LLA[k] + PUSHEX + `
  jmp {N}
@s:`);
}
// JLL's: ht against hp
const CMPV = (kind) => kind === 'lt' ? `
  lda ht
  cmp hp
  lda ht + 1
  sbc hp + 1
  bvc :+
  eor #$80
:` : `
  lda hp
  cmp ht
  lda hp + 1
  sbc ht + 1
  bvc :+
  eor #$80
:`;
for (const [k, m] of Object.entries({ lt: ['lt', 'bpl'], ge: ['lt', 'bmi'], gt: ['gt', 'bpl'], le: ['gt', 'bmi'] }))
  tpl('jll_' + k, STUB, LOAD2 + CMPV(m[0]) + `
  ${m[1]} @f` + TRUEFALSE(4));
tpl('jll_eq', STUB, LOAD2 + `
  lda ht
  cmp hp
  bne @f
  lda ht + 1
  cmp hp + 1
  bne @f` + TRUEFALSE(4));

// ---- The source
const lines = ['; ****************************************************************************',
  '; vmxt.inc - the native code\'s templates (vmx.inc\'s), made by tools/hyvmxt.js: each its length, its flags (TF_STUB: its',
  '; op\'s stub after it, its slow way; TF_NOR: not for an op with VM_R), its patches (each its offset, its kind: TPK_B',
  '; a data byte, TPK_U one untagged, TPK_T a target\'s native place, TPK_N the next op\'s; and the data byte), its code', ''];
const kinds = { B: 'TPK_B', U: 'TPK_U', T: 'TPK_T', N: 'TPK_N' };
for (const t of T) {
  const n = 'vxt_' + t.name;
  const patches = [], code = [];
  let pi = 0;
  for (let raw of t.code.split('\n')) {
    let l = raw.trim();
    if (!l) continue;
    l = l.replace(/@(\w+)/g, (_, x) => n + '_' + x);
    const m = l.match(/\{([BUTN])\s*(\d*)\}/);
    if (m) {
      const k = m[1], arg = m[2] ? +m[2] : 0, word = k === 'T' || k === 'N';
      l = l.replace(m[0], word ? '$0000' : '0');
      const p = n + '_p' + pi++;
      code.push(l.endsWith(':') ? l : '            ' + l.replace(/^(\w+)\s+/, (a, op) => op.padEnd(12)));
      code.push(p + ' = * - ' + (word ? 2 : 1));
      patches.push(`            .byte       ${p} - ${n}_c, ${kinds[k]}, ${arg}`);
    } else code.push(l.endsWith(':') ? l : '            ' + l.replace(/^(\w+)\s+/, (a, op) => op.padEnd(12)));
  }
  lines.push(`${n}:`.padEnd(12) + `.byte       ${n}_e - ${n}_c, ${t.flags}, ${patches.length}`);
  lines.push(...patches, `${n}_c:`, ...code, `${n}_e:`, '');
}
// ---- The tables: each op's template (by its number / 2), and the fused ones' by s
const main = Array(64).fill('0');
const set = (op, t) => { main[op] = t; };
const OPI = { CONST: 1, LOCAL: 2, PUSH: 5, JF: 6, JT: 7, JMP: 8, LPUSH: 35, CPUSH: 36, SETL: 42, LOOP: 48, DOTI: 54, DOTINC: 55, STT: 46, POPX: 47, DROP: 58 };
set(OPI.CONST, 'vxt_const'); set(OPI.LOCAL, 'vxt_local'); set(OPI.PUSH, 'vxt_push'); set(OPI.JF, 'vxt_jf'); set(OPI.JT, 'vxt_jt');
set(OPI.JMP, 'vxt_jmp'); set(OPI.LPUSH, 'vxt_lpush'); set(OPI.CPUSH, 'vxt_cpush'); set(OPI.SETL, 'vxt_setl'); set(OPI.LOOP, 'vxt_loop');
set(OPI.DOTI, 'vxt_doti'); set(OPI.DOTINC, 'vxt_dotinc'); set(OPI.STT, 'vxt_stt'); set(OPI.POPX, 'vxt_popx'); set(OPI.DROP, 'vxt_drop');
lines.push('; Each op\'s template (0: its stub alone; the fused ones: by s, below)');
for (let i = 0; i < 64; i += 8) lines.push((i ? '            ' : 'vx_tmain:   ') + '.word       ' + main.slice(i, i + 8).join(', '));
const S = ['ADD', 'SUB', 'LT', 'GT', 'LE', 'GE', 'NEQ', 'INC', 'DEC', 'ZEROP', 'ONEP', 'ADDC', 'SUBC', 'LTC', 'GTC', 'LEC', 'GEC', 'EQC'];
const fused = {
  vx_tlq: { INC: 'lq_inc', DEC: 'lq_dec', ADDC: 'lq_addc', SUBC: 'lq_subc' },
  vx_tlqp: { INC: 'lqp_inc', DEC: 'lqp_dec', ADDC: 'lqp_addc', SUBC: 'lqp_subc' },
  vx_tjlq: { ZEROP: 'jlq_zerop', ONEP: 'jlq_onep', LTC: 'jlq_ltc', GTC: 'jlq_gtc', LEC: 'jlq_lec', GEC: 'jlq_gec', EQC: 'jlq_eqc' },
  vx_tjq: { ZEROP: 'jq_zerop', ONEP: 'jq_onep', LTC: 'jq_ltc', GTC: 'jq_gtc', LEC: 'jq_lec', GEC: 'jq_gec', EQC: 'jq_eqc' },
  vx_tll: { ADD: 'll_add', SUB: 'll_sub' },
  vx_tllp: { ADD: 'llp_add', SUB: 'llp_sub' },
  vx_tjll: { LT: 'jll_lt', GT: 'jll_gt', LE: 'jll_le', GE: 'jll_ge', NEQ: 'jll_eq' },
};
lines.push('', '; The fused ops\' templates, by s (VO_ADD\'s order: (s - VO_ADD) / 2)');
for (const [tn, m] of Object.entries(fused)) {
  const row = S.map(s => m[s] ? 'vxt_' + m[s] : '0');
  lines.push(tn + ':'.padEnd(12 - tn.length) + '.word       ' + row.slice(0, 9).join(', '));
  lines.push('            .word       ' + row.slice(9).join(', '));
}
fs.writeFileSync(out, lines.join('\r\n') + '\r\n');
console.log(T.length + ' templates');
