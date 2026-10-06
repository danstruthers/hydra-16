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

// ---- Calls: {D k} the address of the op's data byte k (in its stub), {M k} 2 * data byte k + 2 (the frame's record
// place for k arguments), {C} the function's start, {RL}, {RH} the return place (a fixnum) of the next op's entry
const PUSHAX = `
  sta (sp)
  ldy #1
  txa
  sta (sp),y
  lda sp
  clc
  adc #2
  sta sp
  bne :+
  jsr vm_page
:`;
const DEPTH = `
  lda depth + 1
  cmp #>MAX_DEPTH
  bcc @ok
  bne @s
  lda depth
  cmp #<MAX_DEPTH
  bcs @s
@ok:`;
// A frame's record pushed (its return, no scope), vm_s its function's word (ht); near a page's end, vm_rec's (VPUSH's
// way: vm_page may spill and move the stack down, vm_s from sp then).  @sx: the stub, for a branch from far before
const RECORD = `
  lda sp
  cmp #$FC
  bcs @r2
  lda #{RL}
  sta (sp)
  ldy #1
  lda #{RH}
  sta (sp),y
  iny
  lda #0
  sta (sp),y
  iny
  sta (sp),y
  lda sp
  adc #4
  sta sp
  lda ht
  sta vm_s
  lda ht + 1
  sta vm_s + 1
  bra @r4
@sx:
  jmp {S}
@r2:
  lda #{RL}
  ldx #{RH}
  ldy #{M 1}
  jsr vm_rec
@r4:`;
// SHEAD m c t sym ep v: its cache (the global's value at epoch ep), pushed while it's this epoch and no scope's made
tpl('shead', STUB, `
  bit vm_mat
  bmi @s
  lda {D 8}
  cmp vm_ep
  bne @s
  lda {D 9}
  cmp vm_ep + 1
  bne @s
  lda {D 10}
  ldx {D 11}` + PUSHAX + `
  jmp {N}
@s:`);
// HEAD 1 c t (vx_tpl's, one argument alone): a buffer (given its index) or a function partially applied, pushed
tpl('head', STUB, `
  lda ex
  lsr
  bcs @s
  ldx ex + 1
  cpx #IMM_PAGES
  bcc @s
  lda pk,x
  cmp #PK_BUFFER
  beq @p
  cmp #PK_PARTIAL
  bne @s
@p:` + PUSHEX + `
  jmp {N}
@s:`);
// CALL m ret fn ep code bank idx h r: the function it called last (its cache), at this epoch, under the arguments:
// its frame (the return, no scope) and its code (in this bank: at once; another, through vm_next); missed, its stub
// to op_callm (TF_MISS: a buffer's byte, else the call's way)
tpl('call', `${STUB} | TF_MISS`, `
  lda {D 6}
  cmp vm_ep
  bne @sx
  lda {D 7}
  cmp vm_ep + 1
  bne @sx
  lda sp
  sec
  sbc #{M 1}
  sta ht
  lda sp + 1
  sbc #0
  sta ht + 1
  lda (ht)
  cmp {D 4}
  bne @sx
  ldy #1
  lda (ht),y
  cmp {D 5}
  bne @sx
  bit intr
  bmi @sx
  bvs @sx` + DEPTH + RECORD + `
  lda #{M 1}
  sta vm_rb
  stz vm_mat
  inc depth
  bne :+
  inc depth + 1
:
  lda {D 11}
  cmp vm_idx
  bne @far
  jmp ({D 8})
@far:
  sta vm_idx
  lda {D 10}
  sta vm_bank
  lda {D 8}
  sta vm_ip
  lda {D 9}
  sta vm_ip + 1
  jmp vm_next
@s:`);
// The check of CSELF and TSELF (SELFQ's): as many arguments as the formals, this frame's function under them, no
// Ctrl-C nor a note; ht its place
const SELF = `
  lda vm_rb
  cmp #{M 1}
  bne @s
  lda sp
  sec
  sbc vm_rb
  sta ht
  lda sp + 1
  sbc #0
  sta ht + 1
  ldy #1
  lda (ht),y
  cmp (vm_s),y
  bne @s
  lda (ht)
  cmp (vm_s)
  bne @s
  bit intr
  bmi @s
  bvs @s`;
// CSELF m code ret h r: its frame (the return, no scope), its code from its start
tpl('cself', STUB, SELF + DEPTH + RECORD + `
  stz vm_mat
  inc depth
  bne :+
  inc depth + 1
:
  jmp {C}
@s:`);
// TSELF m code 0: the arguments to this frame's, its record's scope none again if one was made, the stack cut back to
// the record's end, its code from its start
tpl('tself', STUB, SELF + `
  ldx #{B 1}
  beq @args
  ldy #2
@arg:
  lda (ht),y
  sta (vm_s),y
  iny
  lda (ht),y
  sta (vm_s),y
  iny
  dex
  bne @arg
@args:
  bit vm_mat
  bpl @kept
  ldy vm_rb
  iny
  iny
  lda #0
  sta (vm_s),y
  iny
  sta (vm_s),y
  stz vm_mat
@kept:
  lda vm_rb
  clc
  adc #4
  adc vm_s
  sta sp
  lda vm_s + 1
  adc #0
  sta sp + 1
  cmp stk_basep
  bne @go
  lda spilled
  beq @go
  jsr unspill
@go:
  jmp {C}
@s:`);
// RET: to its caller's code, in this bank (another, the evaluator's call: its stub), the frame dropped: its return
// pad (past the call's data) finds the caller's frame again; an error, vm_reterr's (returned by the caller too if
// its call's r says)
tpl('ret', STUB, `
  ldy vm_rb
  lda (vm_s),y
  sta ht
  beq @s
  iny
  lda (vm_s),y
  lsr
  tax
  lda ht
  ror
  sta vm_ip
  txa
  and #$60
  cmp vm_idx
  bne @s
  txa
  and #$1F
  ora #$80
  sta vm_ip + 1
  lda depth
  bne :+
  dec depth + 1
:
  dec depth
  lda vm_s
  sta sp
  ldx vm_s + 1
  stx sp + 1
  cpx stk_basep
  bne :+
  lda spilled
  beq :+
  jsr unspill
:
  lda ex
  lsr
  bcs @go
  ldx ex + 1
  cpx #IMM_PAGES
  bcc @go
  lda pk,x
  cmp #PK_ERROR
  bne @go
  jmp vm_reterr
@go:
  jmp (vm_ip)
@s:`);
// A return pad (vx_padt's: past CALL's and CSELF's data, where their returns go, and their code in the machine goes
// on; HEAD's and SHEAD's t, VXK_Q): the caller's frame from the call's h and r, as the machine's RET finds it (vm_s
// h words below the stack's top, vm_rb, vm_mat from its record's scope).  Whichever way it's come to, the stack's
// top is where the call's function was, and the frame what the machine found already, if it did: the same again.
// (Its error's check: RET's, the machine's, the evaluator's resume's)
const PAD = (h, r) => `
  lda sp
  sec
  sbc #{W ${h}}
  sta vm_s
  lda sp + 1
  sbc #0
  sta vm_s + 1
  ldy #{U ${r}}
  sty vm_rb
  iny
  iny
  lda (vm_s),y
  lsr
  bcc :+
  iny
  iny
:
  iny
  lda (vm_s),y
  cmp #IMM_PAGES
  lda #0
  bcc :+
  lda #$80
:
  sta vm_mat`;
tpl('pcall', '0', PAD(12, 13));
tpl('pself', '0', PAD(6, 7));

// ---- Locals and blocks: {DL}, {DH} the place (a fixnum) of the op's data (LOCALS': the record's scope word)
// LOCALS kt kn tbl names: the record's scope word its place; the scope's slot NIL, then kt holes pushed
tpl('locals', STUB, `
  ldy vm_rb
  iny
  iny
  lda #{DL}
  sta (vm_s),y
  iny
  lda #{DH}
  sta (vm_s),y
  lda #0
  tax` + PUSHAX + `
  ldx #{B 1}
@h:
  lda #<UNBOUND
  sta (sp)
  ldy #1
  lda #>UNBOUND
  sta (sp),y
  lda sp
  clc
  adc #2
  sta sp
  bne :+
  jsr vm_page
:
  dex
  bne @h
  jmp {N}
@s:`);
// BLOCK fb k vb parent names: its frame slot NIL, its k variables holes (its data after it: the blocks' table's)
tpl('block', STUB, `
  ldy #{B 1}
  lda #0
  sta (vm_s),y
  iny
  sta (vm_s),y
  ldx #{B 2}
  beq @d
  ldy #{B 3}
@h:
  lda #<UNBOUND
  sta (vm_s),y
  iny
  lda #>UNBOUND
  sta (vm_s),y
  iny
  dex
  bne @h
@d:
  jmp {N}
@s:`);
// A block's variable (LOCALB y fb p sym, SETLB y fb p, SETBLB y fb p sym), in its slot while the block has no scope
const NOSCOPE = `
  ldy #{B 2}
  iny
  lda (vm_s),y
  cmp #IMM_PAGES
  bcs @s`;
const HOLE = (yb) => `
  ldy #{B ${yb}}
  iny
  lda (vm_s),y
  bne @set
  dey
  lda (vm_s),y
  cmp #<UNBOUND
  beq @s
@set:`;
const STOREY = (yb) => `
  ldy #{B ${yb}}
  lda ex
  sta (vm_s),y
  iny
  lda ex + 1
  sta (vm_s),y
  stz ex
  stz ex + 1
  jmp {N}
@s:`;
const LOADHOLE = (yb) => `
  ldy #{B ${yb}}
  lda (vm_s),y
  sta ex
  iny
  lda (vm_s),y
  sta ex + 1
  bne @ok
  lda ex
  cmp #<UNBOUND
  beq @s
@ok:
  jmp {N}
@s:`;
tpl('localb', STUB, NOSCOPE + LOADHOLE(1));
tpl('setlb', STUB, NOSCOPE + STOREY(1));
tpl('setblb', STUB, NOSCOPE + HOLE(1) + STOREY(1));
// A local (LOCALH y sym, SETBL y sym), in its slot while no scope's made
tpl('localh', STUB, `
  bit vm_mat
  bmi @s` + LOADHOLE(1));
tpl('setbl', STUB, `
  bit vm_mat
  bmi @s` + HOLE(1) + STOREY(1));
// TRYE t: ex not an error, to t
tpl('trye', '0', `
  jsr vm_iserr
  bcs @on
  jmp {T 1}
@on:`);
// The quick ops of two values (ADD ... NEQ): the first popped (not past the stack's page), fixnums
const POP2 = `
  lda sp
  beq @s
  sec
  sbc #2
  sta sp
  lda (sp)
  sta ht
  and ex
  lsr
  bcc @u
  ldy #1
  lda (sp),y
  sta ht + 1`;
const UNPOP = `
@u:
  lda sp
  clc
  adc #2
  sta sp
@s:`;
tpl('add', STUB, POP2 + `
  lda ex
  and #$FE
  clc
  adc ht
  tax
  lda ex + 1
  adc ht + 1
  bvs @u
  sta ex + 1
  stx ex
  jmp {N}` + UNPOP);
tpl('sub', STUB, POP2 + `
  lda ex
  and #$FE
  sta hn
  lda ht
  sec
  sbc hn
  tax
  lda ht + 1
  sbc ex + 1
  bvs @u
  sta ex + 1
  stx ex
  jmp {N}` + UNPOP);
const CMP2 = (k) => k === 'eq' ? `
  lda ht
  cmp ex
  bne @f
  lda ht + 1
  cmp ex + 1
  bne @f` : (k === 'lt' || k === 'ge' ? `
  lda ht
  cmp ex
  lda ht + 1
  sbc ex + 1` : `
  lda ex
  cmp ht
  lda ex + 1
  sbc ht + 1`) + `
  bvc :+
  eor #$80
:
  ${k === 'lt' || k === 'gt' ? 'bpl' : 'bmi'} @f`;
for (const k of ['lt', 'gt', 'le', 'ge', 'eq'])
  tpl('q_' + k, STUB, POP2 + CMP2(k) + `
  lda #<T_VAL
  sta ex
  stz ex + 1
  jmp {N}
@f:
  stz ex
  stz ex + 1
  jmp {N}` + UNPOP);

// ---- The source
const lines = ['; ****************************************************************************',
  '; vmxt.inc - the native code\'s templates (vmx.inc\'s), made by tools/hyvmxt.js: each its length, its flags (TF_STUB: its',
  '; op\'s stub after it, its slow way; TF_NOR: not for an op with VM_R), its patches (each its offset, its kind: TPK_B',
  '; a data byte, TPK_U one untagged, TPK_T a target\'s native place, TPK_N the next op\'s; and the data byte), its code', ''];
const kinds = { W: 'TPK_W', B: 'TPK_B', U: 'TPK_U', T: 'TPK_T', N: 'TPK_N', D: 'TPK_D', M: 'TPK_M', C: 'TPK_C', RL: 'TPK_RL', RH: 'TPK_RH', S: 'TPK_S', DL: 'TPK_DL', DH: 'TPK_DH' };
// The long templates (their stub out of a branch's reach): each branch to @s an inverted one past a jmp {S}
const FAR = new Set([]);
const INV = { bne: 'beq', beq: 'bne', bcc: 'bcs', bcs: 'bcc', bmi: 'bpl', bpl: 'bmi', bvc: 'bvs', bvs: 'bvc' };
for (const t of T) {
  const n = 'vxt_' + t.name;
  const patches = [], code = [];
  let pi = 0;
  let src = t.code.split('\n');
  if (FAR.has(t.name)) src = src.flatMap(r => { const b = r.trim().match(/^(b\w\w)\s+@s$/); return b ? [INV[b[1]] + ' :+', 'jmp {S}', ':'] : [r]; });
  for (let raw of src) {
    let l = raw.trim();
    if (!l) continue;
    l = l.replace(/@(\w+)/g, (_, x) => n + '_' + x);
    const m = l.match(/\{(RL|RH|DL|DH|[BUTNDMCSW])\s*(\d*)\}/);
    if (m) {
      const k = m[1], arg = m[2] ? +m[2] : 0, word = k === 'T' || k === 'N' || k === 'D' || k === 'C' || k === 'S';
      l = l.replace(m[0], !word ? '0' : /^jmp\b/.test(l) ? '$0000' : 'a:$0000');    // (Absolute: not $00, page zero)
      const p = n + '_p' + pi++;
      code.push(l.endsWith(':') ? l : '            ' + l.replace(/^(\w+)\s+/, (a, op) => op.padEnd(12)));
      code.push(p + ' = * - ' + (word ? 2 : 1));
      patches.push(`            .byte       ${p} - ${n}_c, ${kinds[k]}, ${arg}`);
    } else code.push(l.endsWith(':') ? l : '            ' + l.replace(/^(\w+)\s+/, (a, op) => op.padEnd(12)));
  }
  lines.push(`${n}:`.padEnd(12) + `.byte       ${n}_e - ${n}_c, ${t.flags}, ${patches.length}`);
  lines.push(...patches, `${n}_c:`, ...code, `${n}_e:`, '');
  if (!/^p(call|self)$/.test(t.name)) lines.push(`.assert ${n}_e - ${n}_c + VX_STUB + 48 < 256, error, "${n}: an op's code is at most 255 bytes"`, '');
}
lines.push('; A return pad\'s length (CALL\'s and CSELF\'s the same: VXK_Q\'s)', 'VXT_PADL        = vxt_pcall_e - vxt_pcall_c',
  '.assert vxt_pself_e - vxt_pself_c = VXT_PADL, error, "the return pads are of a length"',
  '.assert vxt_call_e - vxt_call_c + VX_STUB + 14 + VXT_PADL < 256, error, "CALL\'s code is at most 255 bytes"',
  '.assert vxt_cself_e - vxt_cself_c + VX_STUB + 8 + VXT_PADL < 256, error, "CSELF\'s code is at most 255 bytes"', '');
// ---- The tables: each op's template (by its number / 2), and the fused ones' by s
const main = Array(64).fill('0');
const skip = new Set((process.env.HYVMXT_SKIP || '').split(',').filter(Boolean));   // (Templates left out: a test's)
const set = (op, t) => { if (!skip.has(t.replace('vxt_', ''))) main[op] = t; };
const OPI = { RET: 0, HEAD: 10, CALL: 11, SHEAD: 37, CSELF: 38, TSELF: 39, CONST: 1, LOCAL: 2, PUSH: 5, JF: 6, JT: 7, JMP: 8, LPUSH: 35, CPUSH: 36, SETL: 42, LOOP: 48, DOTI: 54, DOTINC: 55, STT: 46, POPX: 47, DROP: 58, LOCALS: 40, LOCALH: 41, SETBL: 43, BLOCK: 49, LOCALB: 50, SETLB: 51, SETBLB: 52, TRYE: 59, ADD: 17, SUB: 18, LT: 19, GT: 20, LE: 21, GE: 22, NEQ: 23 };
set(OPI.RET, 'vxt_ret'); set(OPI.HEAD, 'vxt_head'); set(OPI.CALL, 'vxt_call'); set(OPI.SHEAD, 'vxt_shead'); set(OPI.CSELF, 'vxt_cself'); set(OPI.TSELF, 'vxt_tself');
set(OPI.LOCALS, 'vxt_locals'); set(OPI.LOCALH, 'vxt_localh'); set(OPI.SETBL, 'vxt_setbl'); set(OPI.BLOCK, 'vxt_block'); set(OPI.LOCALB, 'vxt_localb');
set(OPI.SETLB, 'vxt_setlb'); set(OPI.SETBLB, 'vxt_setblb'); set(OPI.TRYE, 'vxt_trye'); set(OPI.ADD, 'vxt_add'); set(OPI.SUB, 'vxt_sub'); set(OPI.LT, 'vxt_q_lt');
set(OPI.GT, 'vxt_q_gt'); set(OPI.LE, 'vxt_q_le'); set(OPI.GE, 'vxt_q_ge'); set(OPI.NEQ, 'vxt_q_eq');
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
lines.push('', '; BLOCK\'s template\'s length (0: none): its data is past it and its stub (the blocks\' table\'s, a block\'s parent\'s)',
  'VXT_BLOCK_D     = ' + (main[OPI.BLOCK] === '0' ? '0' : 'vxt_block_e - vxt_block_c'));
fs.writeFileSync(out, lines.join('\r\n') + '\r\n');
console.log(T.length + ' templates');
