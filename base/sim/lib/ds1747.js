// ds1747.js - a DS1747 in U7: a 512K task RAM with a clock in its top 8 bytes, task F's $7FF8-$7FFF.  Its clock
// counts seconds (base, at cycle 'at'; as UTC, for the fields) while its oscillator runs.  Its registers show the
// time as it is, or as it was when R or W was set (a snapshot: 'held'); while W is set, writes go to the snapshot,
// and clearing W starts the clock from it, with the century that write gives.  Writes with W clear go nowhere (the
// datasheet doesn't say what they do).  BF (the day's bit 7) can't be written.
//
// createRtc({ time, batteryLow, clock, now, rnd }): time is seconds since 1970 (UTC fields), or 'now' (this
// computer's local time), 'stopped' (its oscillator off, at 2000-01-01) or 'unset' (junk in its registers: rnd);
// now() gives the CPU's cycle count.
'use strict';

const RTC_REGS = 0x7FF8, RTC_TASK = 15;
const bcd = n => ((n / 10) | 0) << 4 | n % 10, unbcd = b => (b >> 4) * 10 + (b & 15);

function createRtc(env) {
  const { time, clock, now } = env;
  const d = new Date();                                       // (now: this computer's local time, as the clock's fields)
  const rtc = { base: typeof time === 'number' ? time : time === 'now' ? Date.UTC(d.getFullYear(), d.getMonth(), d.getDate(),
    d.getHours(), d.getMinutes(), d.getSeconds()) / 1000 : Date.UTC(2000, 0, 1) / 1000, at: 0, osc: time !== 'stopped',
    ctl: 0, held: null, dow: 0, junk: time === 'unset' ? Array.from({ length: 8 }, () => env.rnd(256)) : null };
  rtc.dow = 4;                                                // (Day 1 = Sunday: 1970-01-01 was a Thursday, day 5)
  const secs = () => rtc.osc ? rtc.base + Math.floor((now() - rtc.at) / (clock * 1e6)) : rtc.base;
  rtc.regs = () => {                                          // The registers, from the clock as it is now
    if (rtc.junk) return rtc.junk.slice();
    const s = secs(), t = new Date(s * 1000), y = t.getUTCFullYear(), days = Math.floor(s / 86400);
    return [bcd(Math.floor(y / 100)), (rtc.osc ? 0 : 0x80) | bcd(t.getUTCSeconds()), bcd(t.getUTCMinutes()), bcd(t.getUTCHours()),
      ((days + rtc.dow) % 7 + 7) % 7 + 1, bcd(t.getUTCDate()), bcd(t.getUTCMonth() + 1), bcd(y % 100)];
  };
  rtc.read = r => {
    const v = (rtc.held || rtc.regs())[r];
    if (r === 0) return rtc.ctl | (v & 0x3F);
    return r === 4 ? (env.batteryLow ? 0 : 0x80) | (v & 0x7F) : v;
  };
  rtc.write = (r, v) => {
    if (r > 0) { if (rtc.ctl & 0x80) rtc.held[r] = v; return; }
    if ((v & 0xC0) && !(rtc.ctl & 0xC0)) rtc.held = rtc.regs();   // R or W set: updates halt
    if (!(v & 0x80) && (rtc.ctl & 0x80)) {                      // W cleared: the clock from the registers
      const h = rtc.held, y = unbcd(v & 0x3F) * 100 + unbcd(h[7]);
      rtc.base = Date.UTC(y, unbcd(h[6] & 0x1F) - 1, unbcd(h[5] & 0x3F), unbcd(h[3] & 0x3F), unbcd(h[2] & 0x7F), unbcd(h[1] & 0x7F)) / 1000;
      rtc.at = now(); rtc.osc = !(h[1] & 0x80); rtc.junk = null;
      rtc.dow = (((h[4] & 7) - 1 - Math.floor(rtc.base / 86400)) % 7 + 7) % 7;
    }
    rtc.ctl = v & 0xC0;
    if (!rtc.ctl) rtc.held = null;
  };
  return rtc;
}

module.exports = { createRtc, RTC_REGS, RTC_TASK };
