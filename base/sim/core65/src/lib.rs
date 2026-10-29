// core65 - the Hydra-16's W65C02S and its memory map as WebAssembly (base/sim/lib/core.js loads it; machine.js runs
// it): the instructions, cycles and bus accesses of base/sim/lib/cpu65c02.js, in the same order, and machine.js's
// task RAM, bank window, paged ROM, BIOS ROM, T/U/V/W and IRQ vector RAM; the I/O page, the RTC's registers, the
// hub writers' hook and the rest go back to JavaScript (the imports), the devices with them.  The run loop's
// per-instruction work is machine.js's runQuick's, and its trace ring's: the core runs until the devices want a
// tick (devNext), an interrupt is to be taken, the CPU waits (WAI) or stops (STP), or the cycle limit, and
// machine.js does the rest.  Built by build.js with Rust's wasm32-unknown-unknown target (cargo build --release),
// when Rust is there and this file has changed; else the committed base/sim/lib/core65.js (the bytes) is used.
// Also run_flat: the CPU alone on a flat 64K (a benchmark: STP stops it).
#![no_std]
#![allow(static_mut_refs)]
use core::panic::PanicInfo;
#[panic_handler]
fn panic(_: &PanicInfo) -> ! { loop {} }

#[link(wasm_import_module = "env")]
extern "C" {
  fn io_rd(a: u32, io_at: f64, c0: f64) -> u32;
  fn io_wr(a: u32, v: u32, io_at: f64, c0: f64);
  fn shared_write(k: u32, off: u32, v: u32);
  fn rtc_rd(r: u32) -> u32;
  fn rtc_wr(r: u32, v: u32);
  fn reached(kind: u32, k: u32);
  fn stack_low(t: u32, s: u32);
  fn brk_vector() -> u32;
  fn ioff_note(n: f64, from: u32, to: u32, at: f64);
  fn pc_watch(pc: u32);
}

// W65C02S cycles per opcode (WDC's table), +1 for a page crossed by PAGE_X's indexed reads
const CYC: [u8; 256] = {
  const S: &[u8; 256] = b"7621535532216465255154652421646566213355422144652551446524214465662133553221346525514465243184656621335542216465255144652441646526213335222144452651444525214555262133352221444525514445242144452621335522234465255144652433447526213355222144652551446524414475";
  let mut t = [0u8; 256]; let mut i = 0; while i < 256 { t[i] = S[i] - b'0'; i += 1; } t
};
const PAGE_X: [u8; 256] = {
  let mut t = [0u8; 256];
  let l = [0x11, 0x19, 0x1D, 0x31, 0x39, 0x3D, 0x51, 0x59, 0x5D, 0x71, 0x79, 0x7D, 0xB1, 0xB9, 0xBD, 0xD1, 0xD9, 0xDD,
    0xF1, 0xF9, 0xFD, 0xBC, 0xBE, 0x3C, 0x1E, 0x3E, 0x5E, 0x7E];
  let mut i = 0; while i < l.len() { t[l[i]] = 1; i += 1; } t
};
const C: u32 = 1; const Z: u32 = 2; const I: u32 = 4; const D: u32 = 8; const B: u32 = 0x10; const V: u32 = 0x40; const N: u32 = 0x80;

// The state machine.js and the core share (core.js's names): SI's integers, SF's cycles (as f64: exact to 2^53)
const S_A: usize = 0; const S_X: usize = 1; const S_Y: usize = 2; const S_S: usize = 3; const S_P: usize = 4; const S_PC: usize = 5;
const S_LASTPC: usize = 6; const S_WAITING: usize = 7; const S_HALTED: usize = 8; const S_REGT: usize = 9; const S_REGU: usize = 10;
const S_V: usize = 11; const S_REGW: usize = 12; const S_IRQN: usize = 13; const S_TYPED: usize = 14; const S_IOFFFROM: usize = 15;
const S_TRACELEN: usize = 16; const S_RINGAT: usize = 17; const S_RINGN: usize = 18; const S_NWATCH: usize = 19; const S_WATCH: usize = 20;
const S_SKIP: usize = 29; const S_HOOK: usize = 30; const S_RTC: usize = 31; const S_SHAREDU: usize = 32; const S_NOSHARED: usize = 33;
const S_BANKS: usize = 34; const S_PROMLEN: usize = 35; const S_TR: usize = 36; const S_TB: usize = 37; const S_SB: usize = 38;
const S_PROM: usize = 39; const S_OSROM: usize = 40; const S_VEC: usize = 41; const S_RING: usize = 42; const S_FLAGS: usize = 43;
const S_STACKLOW: usize = 48;
const F_CYC: usize = 0; const F_LIMIT: usize = 1; const F_DEVNEXT: usize = 2; const F_LASTSTART: usize = 3; const F_IOFFAT: usize = 4; const F_IOAT: usize = 5;
static mut SI: [i32; 64] = [0; 64];
static mut SF: [f64; 8] = [0.0; 8];
#[no_mangle] pub extern "C" fn state_i() -> *mut i32 { unsafe { SI.as_mut_ptr() } }
#[no_mangle] pub extern "C" fn state_f() -> *mut f64 { unsafe { SF.as_mut_ptr() } }
// (Room for the machine's memory: machine.js grows the memory past this and lays it out, core.js's layout)
extern "C" { static __heap_base: u8; }
#[no_mangle] pub extern "C" fn heap_base() -> u32 { unsafe { core::ptr::addr_of!(__heap_base) as u32 } }

// The bus: the machine's (Mach) or a flat 64K (Flat)
trait Bus {
  fn rd(&mut self, a: u32) -> u32;
  fn wr(&mut self, a: u32, v: u32);
  fn pushed(&mut self, _s: u32) {}
  fn vector(&mut self) -> u32 { 0 }
  fn set_regs(&mut self, _c: &Regs) {}
}
struct Regs { a: u32, x: u32, y: u32, s: u32, p: u32, pc: u32, cyc: u64, extra: u32, crossed: u32, last_pc: u32, io_at: u64 }

struct Cpu<'b, M: Bus> { r: Regs, m: &'b mut M }
impl<'b, M: Bus> Cpu<'b, M> {
  #[inline(always)] fn rd(&mut self, a: u32) -> u32 { self.m.set_regs(&self.r); self.m.rd(a) }
  #[inline(always)] fn wr(&mut self, a: u32, v: u32) { self.m.set_regs(&self.r); self.m.wr(a, v) }
  #[inline(always)] fn nz(&mut self, v: u32) -> u32 { self.r.p = (self.r.p & !(N | Z)) | (v & 0x80) | (if v == 0 { Z } else { 0 }); v }
  #[inline(always)] fn fetch(&mut self) -> u32 { let v = self.rd(self.r.pc); self.r.pc = (self.r.pc + 1) & 0xFFFF; v }
  #[inline(always)] fn fetch16(&mut self) -> u32 { let l = self.rd(self.r.pc); let h = self.rd((self.r.pc + 1) & 0xFFFF); self.r.pc = (self.r.pc + 2) & 0xFFFF; l | h << 8 }
  #[inline(always)] fn rd16(&mut self, a: u32) -> u32 { let l = self.rd(a); let h = self.rd((a + 1) & 0xFFFF); l | h << 8 }
  #[inline(always)] fn zp16(&mut self, a: u32) -> u32 { let l = self.rd(a & 0xFF); let h = self.rd((a + 1) & 0xFF); l | h << 8 }
  #[inline(always)] fn push(&mut self, v: u32) { self.wr(0x100 + self.r.s, v); self.r.s = (self.r.s.wrapping_sub(1)) & 0xFF; self.m.set_regs(&self.r); self.m.pushed(self.r.s); }
  #[inline(always)] fn pull(&mut self) -> u32 { self.r.s = (self.r.s + 1) & 0xFF; self.rd(0x100 + self.r.s) }
  #[inline(always)] fn idx(&mut self, b: u32, i: u32) -> u32 { let r = (b + i) & 0xFFFF; self.r.crossed = (b ^ r) >> 8; r }
  #[inline(always)] fn zp(&mut self) -> u32 { self.fetch() }
  #[inline(always)] fn zpx(&mut self) -> u32 { (self.fetch() + self.r.x) & 0xFF }
  #[inline(always)] fn zpy(&mut self) -> u32 { (self.fetch() + self.r.y) & 0xFF }
  #[inline(always)] fn abs(&mut self) -> u32 { self.fetch16() }
  #[inline(always)] fn absx(&mut self) -> u32 { let b = self.fetch16(); let x = self.r.x; self.idx(b, x) }
  #[inline(always)] fn absy(&mut self) -> u32 { let b = self.fetch16(); let y = self.r.y; self.idx(b, y) }
  #[inline(always)] fn indx(&mut self) -> u32 { let z = self.fetch() + self.r.x; self.zp16(z) }
  #[inline(always)] fn indy(&mut self) -> u32 { let z = self.fetch(); let b = self.zp16(z); let y = self.r.y; self.idx(b, y) }
  #[inline(always)] fn indz(&mut self) -> u32 { let z = self.fetch(); self.zp16(z) }
  #[inline(always)] fn br(&mut self, c: bool) {
    let o = self.fetch();
    if c { let t = (self.r.pc.wrapping_add(((o ^ 0x80) as i32 - 0x80) as u32)) & 0xFFFF; self.r.extra += if (t ^ self.r.pc) >> 8 != 0 { 2 } else { 1 }; self.r.pc = t; }
  }
  #[inline(always)] fn adc(&mut self, v: u32) {
    if self.r.p & D != 0 { self.r.extra = 1; }
    let a = self.r.a as i32; let vi = v as i32; let c = (self.r.p & C) as i32; let mut r = a + vi + c;
    self.r.p = (self.r.p & !(C | V)) | (if r > 0xFF { C } else { 0 }) | (if (!(a ^ vi) & (a ^ r) & 0x80) != 0 { V } else { 0 });
    if self.r.p & D != 0 {
      let mut lo = (a & 15) + (vi & 15) + c; let mut hi = (a >> 4) + (vi >> 4);
      if lo > 9 { lo += 6; hi += 1; } if hi > 9 { hi += 6; }
      r = (hi << 4) | (lo & 15); self.r.p = (self.r.p & !C) | (if hi > 15 { C } else { 0 });
    }
    self.r.a = self.nz((r & 0xFF) as u32);
  }
  #[inline(always)] fn sbc(&mut self, v: u32) {
    if self.r.p & D == 0 { self.adc(v ^ 0xFF); return; }
    self.r.extra = 1;
    let a = self.r.a as i32; let vi = v as i32; let b = 1 - (self.r.p & C) as i32; let r = a - vi - b;
    let mut lo = (a & 15) - (vi & 15) - b; let mut hi = (a >> 4) - (vi >> 4);
    if lo < 0 { lo -= 6; hi -= 1; } if hi < 0 { hi -= 6; }
    self.r.p = (self.r.p & !C) | (if r >= 0 { C } else { 0 });
    self.r.a = self.nz((((hi << 4) | (lo & 15)) & 0xFF) as u32);
  }
  #[inline(always)] fn cmp(&mut self, r: u32, v: u32) { let t = r as i32 - v as i32; self.r.p = (self.r.p & !C) | (if t >= 0 { C } else { 0 }); self.nz((t & 0xFF) as u32); }
  #[inline(always)] fn asl(&mut self, v: u32) -> u32 { self.r.p = (self.r.p & !C) | (v >> 7); self.nz((v << 1) & 0xFF) }
  #[inline(always)] fn lsr(&mut self, v: u32) -> u32 { self.r.p = (self.r.p & !C) | (v & 1); self.nz(v >> 1) }
  #[inline(always)] fn rol(&mut self, v: u32) -> u32 { let c = self.r.p & C; self.r.p = (self.r.p & !C) | (v >> 7); self.nz(((v << 1) | c) & 0xFF) }
  #[inline(always)] fn ror(&mut self, v: u32) -> u32 { let c = self.r.p & C; self.r.p = (self.r.p & !C) | (v & 1); self.nz((v >> 1) | (c << 7)) }
  #[inline(always)] fn bit(&mut self, v: u32) { self.r.p = (self.r.p & !(N | V | Z)) | (v & 0xC0) | (if self.r.a & v != 0 { 0 } else { Z }); }
  #[inline(always)] fn dec(&mut self, v: u32) -> u32 { self.nz((v.wrapping_sub(1)) & 0xFF) }
  #[inline(always)] fn inc(&mut self, v: u32) -> u32 { self.nz((v + 1) & 0xFF) }
  #[inline(always)] fn irq(&mut self, vec: u32, brk: bool) {
    let pc = self.r.pc; self.push(pc >> 8); let pc = self.r.pc; self.push(pc & 0xFF);
    let p = (self.r.p | 0x20) & (if brk { 0xFF } else { !B }); self.push(p);
    self.r.p = (self.r.p | I) & !D; self.r.pc = vec;
  }

  // One instruction; WAI and STP say so: 1 waiting, 2 stopped
  #[inline(always)]
  fn step(&mut self) -> u32 {
    let c = self;
    c.r.last_pc = c.r.pc;
    let op = c.fetch();
    c.r.crossed = 0; c.r.extra = 0;
    c.r.io_at = c.r.cyc + CYC[op as usize] as u64 - 1;
    let mut why = 0;
    macro_rules! rmw { ($mode:ident, $f:ident) => {{ let a = c.$mode(); let v = c.rd(a); let r = c.$f(v); c.wr(a, r); }} }
    macro_rules! ld { ($reg:ident, $mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); c.r.$reg = c.nz(v); }} }
    macro_rules! st { ($reg:ident, $mode:ident) => {{ let a = c.$mode(); let v = c.r.$reg; c.wr(a, v); }} }
    macro_rules! ora { ($mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); let r = c.r.a | v; c.r.a = c.nz(r); }} }
    macro_rules! and { ($mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); let r = c.r.a & v; c.r.a = c.nz(r); }} }
    macro_rules! eor { ($mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); let r = c.r.a ^ v; c.r.a = c.nz(r); }} }
    macro_rules! adc { ($mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); c.adc(v); }} }
    macro_rules! sbc { ($mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); c.sbc(v); }} }
    macro_rules! cmpa { ($reg:ident, $mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); let r = c.r.$reg; c.cmp(r, v); }} }
    macro_rules! bit { ($mode:ident) => {{ let a = c.$mode(); let v = c.rd(a); c.bit(v); }} }
    macro_rules! stz { ($mode:ident) => {{ let a = c.$mode(); c.wr(a, 0); }} }
    match op {
      0x00 => { c.r.pc = (c.r.pc + 1) & 0xFFFF; c.m.set_regs(&c.r); let v = c.m.vector(); c.irq(v, true); }
      0x40 => { let p = c.pull(); c.r.p = p | 0x30; let l = c.pull(); c.r.pc = l; let h = c.pull(); c.r.pc |= h << 8; }
      0x60 => { let l = c.pull(); c.r.pc = l; let h = c.pull(); c.r.pc |= h << 8; c.r.pc = (c.r.pc + 1) & 0xFFFF; }
      0x20 => { let a = c.fetch16(); let t = (c.r.pc.wrapping_sub(1)) & 0xFFFF; c.push(t >> 8); c.push(t & 0xFF); c.r.pc = a; }
      0x4C => { c.r.pc = c.fetch16(); }
      0x6C => { let a = c.fetch16(); c.r.pc = c.rd16(a); }
      0x7C => { let a = c.absx(); c.r.pc = c.rd16(a); }
      0x08 => { let p = c.r.p | 0x30; c.push(p); } 0x28 => { c.r.p = c.pull() | 0x30; }
      0x48 => { let v = c.r.a; c.push(v); } 0x68 => { let v = c.pull(); c.r.a = c.nz(v); }
      0xDA => { let v = c.r.x; c.push(v); } 0xFA => { let v = c.pull(); c.r.x = c.nz(v); }
      0x5A => { let v = c.r.y; c.push(v); } 0x7A => { let v = c.pull(); c.r.y = c.nz(v); }
      0x18 => { c.r.p &= !C; } 0x38 => { c.r.p |= C; } 0x58 => { c.r.p &= !I; } 0x78 => { c.r.p |= I; }
      0xB8 => { c.r.p &= !V; } 0xD8 => { c.r.p &= !D; } 0xF8 => { c.r.p |= D; }
      0xAA => { let v = c.r.a; c.r.x = c.nz(v); } 0xA8 => { let v = c.r.a; c.r.y = c.nz(v); }
      0x8A => { let v = c.r.x; c.r.a = c.nz(v); } 0x98 => { let v = c.r.y; c.r.a = c.nz(v); }
      0xBA => { let v = c.r.s; c.r.x = c.nz(v); } 0x9A => { c.r.s = c.r.x; }
      0xE8 => { let v = (c.r.x + 1) & 0xFF; c.r.x = c.nz(v); } 0xCA => { let v = (c.r.x.wrapping_sub(1)) & 0xFF; c.r.x = c.nz(v); }
      0xC8 => { let v = (c.r.y + 1) & 0xFF; c.r.y = c.nz(v); } 0x88 => { let v = (c.r.y.wrapping_sub(1)) & 0xFF; c.r.y = c.nz(v); }
      0x1A => { let v = (c.r.a + 1) & 0xFF; c.r.a = c.nz(v); } 0x3A => { let v = (c.r.a.wrapping_sub(1)) & 0xFF; c.r.a = c.nz(v); }
      0xEA => {}
      0xCB => { why = 1; }
      0xDB => { why = 2; }
      0x0A => { let v = c.r.a; c.r.a = c.asl(v); } 0x4A => { let v = c.r.a; c.r.a = c.lsr(v); }
      0x2A => { let v = c.r.a; c.r.a = c.rol(v); } 0x6A => { let v = c.r.a; c.r.a = c.ror(v); }
      0x10 => { let f = c.r.p & N == 0; c.br(f); } 0x30 => { let f = c.r.p & N != 0; c.br(f); }
      0x50 => { let f = c.r.p & V == 0; c.br(f); } 0x70 => { let f = c.r.p & V != 0; c.br(f); }
      0x90 => { let f = c.r.p & C == 0; c.br(f); } 0xB0 => { let f = c.r.p & C != 0; c.br(f); }
      0xD0 => { let f = c.r.p & Z == 0; c.br(f); } 0xF0 => { let f = c.r.p & Z != 0; c.br(f); }
      0x80 => { c.br(true); }
      0x64 => stz!(zp), 0x74 => stz!(zpx), 0x9C => stz!(abs), 0x9E => stz!(absx),
      0x89 => { let v = c.fetch(); c.r.p = (c.r.p & !Z) | (if c.r.a & v != 0 { 0 } else { Z }); }
      0x24 => bit!(zp), 0x34 => bit!(zpx), 0x2C => bit!(abs), 0x3C => bit!(absx),
      0x04 | 0x0C => { let a = if op == 0x04 { c.zp() } else { c.abs() }; let v = c.rd(a); c.r.p = (c.r.p & !Z) | (if c.r.a & v != 0 { 0 } else { Z }); let r = v | c.r.a; c.wr(a, r); }
      0x14 | 0x1C => { let a = if op == 0x14 { c.zp() } else { c.abs() }; let v = c.rd(a); c.r.p = (c.r.p & !Z) | (if c.r.a & v != 0 { 0 } else { Z }); let r = v & !c.r.a; c.wr(a, r); }
      0x92 => st!(a, indz), 0xB2 => ld!(a, indz),
      0x12 => ora!(indz), 0x32 => and!(indz), 0x52 => eor!(indz), 0x72 => adc!(indz), 0xD2 => cmpa!(a, indz), 0xF2 => sbc!(indz),
      0xA2 => { let v = c.fetch(); c.r.x = c.nz(v); } 0xA0 => { let v = c.fetch(); c.r.y = c.nz(v); }
      0xA6 => ld!(x, zp), 0xB6 => ld!(x, zpy), 0xAE => ld!(x, abs), 0xBE => ld!(x, absy),
      0xA4 => ld!(y, zp), 0xB4 => ld!(y, zpx), 0xAC => ld!(y, abs), 0xBC => ld!(y, absx),
      0x86 => st!(x, zp), 0x96 => st!(x, zpy), 0x8E => st!(x, abs),
      0x84 => st!(y, zp), 0x94 => st!(y, zpx), 0x8C => st!(y, abs),
      0xE0 => { let v = c.fetch(); let r = c.r.x; c.cmp(r, v); } 0xE4 => cmpa!(x, zp), 0xEC => cmpa!(x, abs),
      0xC0 => { let v = c.fetch(); let r = c.r.y; c.cmp(r, v); } 0xC4 => cmpa!(y, zp), 0xCC => cmpa!(y, abs),
      0x02 | 0x22 | 0x42 | 0x62 | 0x82 | 0xC2 | 0xE2 | 0x44 | 0x54 | 0xD4 | 0xF4 => { c.fetch(); }
      0x5C | 0xDC | 0xFC => { c.fetch16(); }
      0x03 | 0x13 | 0x23 | 0x33 | 0x43 | 0x53 | 0x63 | 0x73 | 0x83 | 0x93 | 0xA3 | 0xB3 | 0xC3 | 0xD3 | 0xE3 | 0xF3 |
      0x0B | 0x1B | 0x2B | 0x3B | 0x4B | 0x5B | 0x6B | 0x7B | 0x8B | 0x9B | 0xAB | 0xBB | 0xEB | 0xFB => {}
      0x01 => ora!(indx), 0x05 => ora!(zp), 0x09 => { let v = c.fetch(); let r = c.r.a | v; c.r.a = c.nz(r); }, 0x0D => ora!(abs),
      0x11 => ora!(indy), 0x15 => ora!(zpx), 0x19 => ora!(absy), 0x1D => ora!(absx),
      0x21 => and!(indx), 0x25 => and!(zp), 0x29 => { let v = c.fetch(); let r = c.r.a & v; c.r.a = c.nz(r); }, 0x2D => and!(abs),
      0x31 => and!(indy), 0x35 => and!(zpx), 0x39 => and!(absy), 0x3D => and!(absx),
      0x41 => eor!(indx), 0x45 => eor!(zp), 0x49 => { let v = c.fetch(); let r = c.r.a ^ v; c.r.a = c.nz(r); }, 0x4D => eor!(abs),
      0x51 => eor!(indy), 0x55 => eor!(zpx), 0x59 => eor!(absy), 0x5D => eor!(absx),
      0x61 => adc!(indx), 0x65 => adc!(zp), 0x69 => { let v = c.fetch(); c.adc(v); }, 0x6D => adc!(abs),
      0x71 => adc!(indy), 0x75 => adc!(zpx), 0x79 => adc!(absy), 0x7D => adc!(absx),
      0x81 => st!(a, indx), 0x85 => st!(a, zp), 0x8D => st!(a, abs), 0x91 => st!(a, indy), 0x95 => st!(a, zpx), 0x99 => st!(a, absy), 0x9D => st!(a, absx),
      0xA1 => ld!(a, indx), 0xA5 => ld!(a, zp), 0xA9 => { let v = c.fetch(); c.r.a = c.nz(v); }, 0xAD => ld!(a, abs),
      0xB1 => ld!(a, indy), 0xB5 => ld!(a, zpx), 0xB9 => ld!(a, absy), 0xBD => ld!(a, absx),
      0xC1 => cmpa!(a, indx), 0xC5 => cmpa!(a, zp), 0xC9 => { let v = c.fetch(); let r = c.r.a; c.cmp(r, v); }, 0xCD => cmpa!(a, abs),
      0xD1 => cmpa!(a, indy), 0xD5 => cmpa!(a, zpx), 0xD9 => cmpa!(a, absy), 0xDD => cmpa!(a, absx),
      0xE1 => sbc!(indx), 0xE5 => sbc!(zp), 0xE9 => { let v = c.fetch(); c.sbc(v); }, 0xED => sbc!(abs),
      0xF1 => sbc!(indy), 0xF5 => sbc!(zpx), 0xF9 => sbc!(absy), 0xFD => sbc!(absx),
      0x06 => rmw!(zp, asl), 0x0E => rmw!(abs, asl), 0x16 => rmw!(zpx, asl), 0x1E => rmw!(absx, asl),
      0x26 => rmw!(zp, rol), 0x2E => rmw!(abs, rol), 0x36 => rmw!(zpx, rol), 0x3E => rmw!(absx, rol),
      0x46 => rmw!(zp, lsr), 0x4E => rmw!(abs, lsr), 0x56 => rmw!(zpx, lsr), 0x5E => rmw!(absx, lsr),
      0x66 => rmw!(zp, ror), 0x6E => rmw!(abs, ror), 0x76 => rmw!(zpx, ror), 0x7E => rmw!(absx, ror),
      0xC6 => rmw!(zp, dec), 0xCE => rmw!(abs, dec), 0xD6 => rmw!(zpx, dec), 0xDE => rmw!(absx, dec),
      0xE6 => rmw!(zp, inc), 0xEE => rmw!(abs, inc), 0xF6 => rmw!(zpx, inc), 0xFE => rmw!(absx, inc),
      _ => {                                                   // BBR, BBS (xF), RMB, SMB (x7)
        let n = (op >> 4) & 7; let z = c.fetch(); let v = c.rd(z);
        if op & 0x0F == 0x0F { let set = (v >> n) & 1 != 0; c.br(if op & 0x80 != 0 { set } else { !set }); }
        else { c.wr(z, if op & 0x80 != 0 { v | (1 << n) } else { v & !(1 << n) }); }
      }
    }
    c.r.cyc += CYC[op as usize] as u64 + c.r.extra as u64 + (if c.r.crossed != 0 && PAGE_X[op as usize] != 0 { 1 } else { 0 });
    why
  }
}

// ---- The machine's bus (machine.js's rd and wr, for the plain model: no what-ifs, no watches)
struct Mach {
  t: u32, u: u32, w: u32, vreg: u32, reg_t: u32, reg_u: u32, reg_w: u32,
  tr: usize, tb: usize, sb: usize, prom: usize, osrom: usize, vec: usize, flags: usize,
  banks: u32, prom_len: u32, shared_on: bool, shared_u: u32, noshared: bool, hook: bool, rtc: bool,
  c0: u64,                                                    // (The instruction's first cycle)
  cur: *const Regs,
}
impl Mach {
  #[inline(always)] fn b(&self, o: usize) -> u32 { unsafe { *(o as *const u8) as u32 } }
  #[inline(always)] fn setb(&self, o: usize, v: u32) { unsafe { *(o as *mut u8) = v as u8; } }
  // The state out before an import (the registers as the JS CPU has them mid-instruction: cyc its first cycle), and
  // what JS may have changed back after it
  fn out(&self) {
    unsafe {
      let r = &*self.cur;
      SI[S_A] = r.a as i32; SI[S_X] = r.x as i32; SI[S_Y] = r.y as i32; SI[S_S] = r.s as i32; SI[S_P] = r.p as i32; SI[S_PC] = r.pc as i32;
      SI[S_LASTPC] = r.last_pc as i32; SI[S_REGT] = self.reg_t as i32; SI[S_REGU] = self.reg_u as i32; SI[S_V] = self.vreg as i32; SI[S_REGW] = self.reg_w as i32;
      SF[F_CYC] = self.c0 as f64; SF[F_IOAT] = r.io_at as f64;
    }
  }
  fn reach(&mut self, kind: u32, k: u32) {
    let f = self.flags + if kind == 0 { k as usize } else { 16 * 256 + k as usize };
    if self.b(f) == 0 { self.out(); unsafe { reached(kind, k); } }
  }
  fn set_u(&mut self, v: u32) { self.reg_u = v; self.u = v & 15; self.shared_on = !self.noshared && self.u < self.shared_u; }
}
impl Bus for Mach {
  #[inline(always)] fn set_regs(&mut self, c: &Regs) { self.cur = c as *const Regs; }
  #[inline(always)]
  fn rd(&mut self, a: u32) -> u32 {
    if a < 0x8000 {
      if self.rtc && a >= 0x7FF8 && self.t == 15 { self.out(); return unsafe { rtc_rd(a - 0x7FF8) }; }
      return self.b(self.tr + (self.t as usize) * 0x8000 + a as usize);
    }
    if a < 0xA000 {
      let b = self.b(self.tr + (self.t as usize) * 0x8000);
      if b >= 0xF0 {
        if !self.shared_on { return a >> 8; }
        let k = self.u * 16 + (b & 15);
        self.reach(1, k);
        return self.b(self.sb + (k as usize) * 0x2000 + (a - 0x8000) as usize);
      }
      if b >= self.banks { return a >> 8; }
      self.reach(0, self.t * 256 + b);
      return self.b(self.tb + ((self.t * self.banks + b) as usize) * 0x2000 + (a - 0x8000) as usize);
    }
    if a < 0xE000 {
      let r = self.b(self.tr + (self.t as usize) * 0x8000 + 1);
      let off = ((r & 0xF3) | ((r & 0x04) << 1) | ((r & 0x08) >> 1)) * 0x4000 + ((a - 0xA000) ^ 0x2000);
      return if off < self.prom_len { self.b(self.prom + off as usize) } else { 0xFF };
    }
    if a < 0xFF00 { return self.b(self.osrom + (self.w as usize) * 0x2000 + (a - 0xE000) as usize); }
    if a < 0xFFF0 { self.out(); let io_at = unsafe { (*self.cur).io_at } as f64; return unsafe { io_rd(a, io_at, self.c0 as f64) }; }
    if a == 0xFFF0 { return self.reg_t; } if a == 0xFFF1 { return self.reg_u; } if a == 0xFFF2 { return self.vreg; } if a == 0xFFF3 { return self.reg_w; }
    if a == 0xFFFE || a == 0xFFFF {
      let o = self.vec + ((self.vreg & 15) as usize) * 2;
      return if a == 0xFFFE { self.b(o) } else { self.b(o + 1) };
    }
    self.b(self.osrom + (self.w as usize) * 0x2000 + (a - 0xE000) as usize)
  }
  #[inline(always)]
  fn wr(&mut self, a: u32, v: u32) {
    let v = v & 0xFF;
    if a < 0x8000 {
      if self.rtc && a >= 0x7FF8 && self.t == 15 { self.out(); unsafe { rtc_wr(a - 0x7FF8, v); } return; }
      self.setb(self.tr + (self.t as usize) * 0x8000 + a as usize, v); return;
    }
    if a < 0xA000 {
      let b = self.b(self.tr + (self.t as usize) * 0x8000);
      if b >= 0xF0 {
        if !self.shared_on { return; }
        let k = self.u * 16 + (b & 15);
        if self.hook { self.out(); unsafe { shared_write(k, a - 0x8000, v); } }
        self.reach(1, k);
        self.setb(self.sb + (k as usize) * 0x2000 + (a - 0x8000) as usize, v); return;
      }
      if b >= self.banks { return; }
      self.reach(0, self.t * 256 + b);
      self.setb(self.tb + ((self.t * self.banks + b) as usize) * 0x2000 + (a - 0x8000) as usize, v); return;
    }
    if a < 0xFF00 { return; }
    if a < 0xFFF0 { self.out(); let io_at = unsafe { (*self.cur).io_at } as f64; unsafe { io_wr(a, v, io_at, self.c0 as f64); } return; }
    if a == 0xFFF0 { self.reg_t = v; self.t = v & 15; return; }
    if a == 0xFFF1 { self.set_u(v); return; }
    if a == 0xFFF2 { self.vreg = v; return; }
    if a == 0xFFF3 { self.reg_w = v; self.w = v & 15; return; }
    if a == 0xFFFE || a == 0xFFFF {
      let o = self.vec + ((self.vreg & 15) as usize) * 2;
      self.setb(if a == 0xFFFE { o } else { o + 1 }, v);
    }
  }
  fn pushed(&mut self, s: u32) {
    unsafe {
      let t = (self.t & 15) as usize;
      if (s as i32) < SI[S_STACKLOW + t] { SI[S_STACKLOW + t] = s as i32; self.out(); stack_low(t as u32, s); }
    }
  }
  fn vector(&mut self) -> u32 { self.out(); unsafe { brk_vector() } }
}

// Run (machine.js's runWasm): from the state, to cycle limit or until machine.js is wanted.  The first instruction's
// start (the devices' sync, the IRQ lines, a WAI) machine.js has seen to (skip).  OUT: why it stopped: 0 the limit,
// 1 the devices (devNext), 2 an interrupt to take, 3 waiting (WAI), 4 stopped (STP)
#[no_mangle]
pub extern "C" fn run(skip: u32) -> u32 {
  unsafe {
    let mut m = Mach {
      t: (SI[S_REGT] & 15) as u32, u: 0, w: (SI[S_REGW] & 15) as u32, vreg: SI[S_V] as u32, reg_t: SI[S_REGT] as u32, reg_u: 0, reg_w: SI[S_REGW] as u32,
      tr: SI[S_TR] as usize, tb: SI[S_TB] as usize, sb: SI[S_SB] as usize, prom: SI[S_PROM] as usize, osrom: SI[S_OSROM] as usize,
      vec: SI[S_VEC] as usize, flags: SI[S_FLAGS] as usize, banks: SI[S_BANKS] as u32, prom_len: SI[S_PROMLEN] as u32,
      shared_on: false, shared_u: SI[S_SHAREDU] as u32, noshared: SI[S_NOSHARED] != 0, hook: SI[S_HOOK] != 0, rtc: SI[S_RTC] != 0,
      c0: 0, cur: core::ptr::null(),
    };
    m.set_u(SI[S_REGU] as u32);
    let regs = Regs { a: SI[S_A] as u32, x: SI[S_X] as u32, y: SI[S_Y] as u32, s: SI[S_S] as u32, p: SI[S_P] as u32, pc: SI[S_PC] as u32,
      cyc: SF[F_CYC] as u64, extra: 0, crossed: 0, last_pc: SI[S_LASTPC] as u32, io_at: SF[F_IOAT] as u64 };
    let limit = SF[F_LIMIT] as u64;
    let trace_len = SI[S_TRACELEN] as u32; let ring = SI[S_RING] as usize;
    let mut ring_at = SI[S_RINGAT] as u32; let mut ring_n = SI[S_RINGN] as u32;
    let nwatch = SI[S_NWATCH] as usize;
    let mut ioff_at = SF[F_IOFFAT]; let mut ioff_from = SI[S_IOFFFROM] as u32;
    let mut first = skip != 0;
    let mut last_start = SF[F_LASTSTART];
    let mut c = Cpu { r: regs, m: &mut m };
    let why;
    loop {
      let cyc = c.r.cyc;
      if !first {
        if cyc >= limit { why = 0; break; }
        if (cyc as f64) >= SF[F_DEVNEXT] { why = 1; break; }
        if SI[S_IRQN] >= 0 && c.r.p & I == 0 { why = 2; break; }
      }
      first = false;
      last_start = cyc as f64;
      c.m.c0 = cyc;
      if SI[S_TYPED] != 0 {                                    // IRQs-off stretches, from the first key typed
        if c.r.p & I != 0 { if ioff_at < 0.0 { ioff_at = cyc as f64; ioff_from = c.m.w << 16 | c.r.pc; } }
        else if ioff_at >= 0.0 {
          let n = cyc as f64 - ioff_at; let (f, at) = (ioff_from, ioff_at); ioff_at = -1.0;
          c.m.cur = &c.r as *const Regs; c.m.out(); ioff_note(n, f, c.m.w << 16 | c.r.last_pc, at);
        }
      }
      if trace_len != 0 {
        let o = ring + (ring_at as usize) * 16;
        let e = [c.m.w, c.m.t, c.r.pc, c.r.a, c.r.x, c.r.y, c.r.s, c.r.p];
        for i in 0..8 { *((o + i * 2) as *mut u16) = e[i] as u16; }
        ring_at = (ring_at + 1) % trace_len; if ring_n < trace_len { ring_n += 1; }
      }
      if nwatch != 0 {
        for i in 0..nwatch {
          if SI[S_WATCH + 2 * i] as u32 == c.r.pc && (SI[S_WATCH + 2 * i + 1] < 0 || SI[S_WATCH + 2 * i + 1] as u32 == c.m.w) {
            c.m.cur = &c.r as *const Regs; c.m.out(); SI[S_RINGAT] = ring_at as i32; SI[S_RINGN] = ring_n as i32; pc_watch(c.r.pc);
          }
        }
      }
      let w = c.step();
      if w == 1 { SI[S_WAITING] = 1; why = 3; break; }
      if w == 2 { SI[S_HALTED] = 1; why = 4; break; }
    }
    let m = &*c.m; let regs = &c.r;
    SI[S_A] = regs.a as i32; SI[S_X] = regs.x as i32; SI[S_Y] = regs.y as i32; SI[S_S] = regs.s as i32; SI[S_P] = regs.p as i32; SI[S_PC] = regs.pc as i32;
    SI[S_LASTPC] = regs.last_pc as i32; SF[F_CYC] = regs.cyc as f64; SF[F_IOAT] = regs.io_at as f64; SF[F_LASTSTART] = last_start;
    SI[S_REGT] = m.reg_t as i32; SI[S_REGU] = m.reg_u as i32; SI[S_V] = m.vreg as i32; SI[S_REGW] = m.reg_w as i32;
    SI[S_RINGAT] = ring_at as i32; SI[S_RINGN] = ring_n as i32; SF[F_IOFFAT] = ioff_at; SI[S_IOFFFROM] = ioff_from as i32;
    why
  }
}

// ---- The CPU alone on a flat 64K (a benchmark): to cycle limit or STP (1)
static mut FLAT: [u8; 65536] = [0; 65536];
struct Flat;
impl Bus for Flat {
  #[inline(always)] fn rd(&mut self, a: u32) -> u32 { unsafe { *FLAT.get_unchecked((a & 0xFFFF) as usize) as u32 } }
  #[inline(always)] fn wr(&mut self, a: u32, v: u32) { unsafe { *FLAT.get_unchecked_mut((a & 0xFFFF) as usize) = v as u8; } }
}
#[no_mangle] pub extern "C" fn flat() -> *mut u8 { unsafe { FLAT.as_mut_ptr() } }
#[no_mangle]
pub extern "C" fn run_flat(limit: f64) -> u32 {
  unsafe {
    let mut f = Flat;
    let mut cpu = Cpu { r: Regs { a: SI[S_A] as u32, x: SI[S_X] as u32, y: SI[S_Y] as u32, s: SI[S_S] as u32, p: SI[S_P] as u32, pc: SI[S_PC] as u32,
      cyc: SF[F_CYC] as u64, extra: 0, crossed: 0, last_pc: 0, io_at: 0 }, m: &mut f };
    let lim = limit as u64; let mut why = 0;
    while cpu.r.cyc < lim { if cpu.step() == 2 { why = 1; break; } }
    SI[S_A] = cpu.r.a as i32; SI[S_X] = cpu.r.x as i32; SI[S_Y] = cpu.r.y as i32; SI[S_S] = cpu.r.s as i32; SI[S_P] = cpu.r.p as i32; SI[S_PC] = cpu.r.pc as i32;
    SF[F_CYC] = cpu.r.cyc as f64;
    why
  }
}
