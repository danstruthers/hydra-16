// Cross-page reference checker for the BIOS ROM.
//
// Code on one BIOS ROM page ($E000-$FFFF, selected by W) can only call code on the same page, the COMMON
// block ($FD00) and the fixed page-0 areas every page shares (none today besides COMMON and I/O).  A call
// to another page must go through a gate (FAR_GATE_INLINE, FAR_JMP_GATE, TASK_GATE).  Inside a page's
// .scope, a page-0 name that has no local gate silently binds to its page-0 address, so this script
// lists every reference from code on one page to a label on another.
//
// Usage (from os_rom):
//   ca65 -g --cpu 65C02 -o obj.o all.s
//   ld65 -C os_rom_C02.cfg obj.o --dbgfile all.dbg
//   node check_pages.js all.dbg
//
// Not listed: references written with an explicit scope in the source (`::NAME`, `PAGE2::NAME`: a gate's
// target, or an address handed to TASK_CALL or TASK_RUN along with its page), and HyForth's far words'
// headers (def_far).  A reference made by a macro (e.g. PRINT_CHAR's call to WRITE_CHAR) is listed, since
// it's never explicit; so is one from a gate segment (a fast path in a gate, such as WRITE_CHAR's call to
// IO_FLUSH, must call its own page's gate).
//
// Not caught: a pointer to ROM data handed to a routine on another page (e.g. a name string on page 4
// passed to IO_OPEN, which reads it on page 2).  Such data has to be copied to RAM first.

"use strict";
const fs = require("fs");

const dbgName = process.argv[2] || "all.dbg";
const rec = { file: [], line: [], span: [], seg: [], sym: [] };

for (const text of fs.readFileSync(dbgName, "utf8").split(/\r?\n/)) {
    const tab = text.indexOf("\t");
    if (tab < 0) continue;
    const kind = text.slice(0, tab);
    if (!(kind in rec)) continue;
    const o = {};
    for (const m of text.slice(tab + 1).matchAll(/(\w+)=("[^"]*"|[^,]*)/g))
        o[m[1]] = m[2].startsWith('"') ? m[2].slice(1, -1) : m[2];
    rec[kind][+o.id] = o;
}

// ROM page of a segment: BIOS image segments by their offset in the image; HyForth's RAM variables and
// paged ROM are used with W = 1.  null = not BIOS ROM code (RAM, zero page), or shared by every page.
function segPage(seg) {
    if (!seg || !seg.oname) return null;
    if (/^FORTH_(DATA|PAGED_ROM)$/.test(seg.name)) return 1;
    if (/^HWT_/.test(seg.name)) return 'hwtest';                 // (The hardware test: calls no BIOS code)
    if (!/os_rom_C02\.bin$/.test(seg.oname)) return null;
    if (/^(COMMON|RESETVEC|IO_PORTS)/.test(seg.name)) return null;
    return Math.floor(+seg.ooffs / 0x2000);
}

const srcCache = {};
function srcLine(fileName, n) {
    if (!(fileName in srcCache)) {
        try { srcCache[fileName] = fs.readFileSync(fileName, "utf8").split(/\r?\n/); }
        catch (e) { srcCache[fileName] = []; }
    }
    return srcCache[fileName][n - 1] || "";
}
const isExplicit = (text, name) => new RegExp("::" + name + "\\b").test(text.replace(/;.*/, ""));
// A far word's header (def_far, hyforth.s): the address of its code on page A, which FARWORD calls there
const isFarWord = (text) => /^\s*def_far\b/.test(text);

let problems = 0;
for (const sym of rec.sym) {
    if (!sym || sym.type !== "lab" || sym.seg === undefined || !sym.ref) continue;
    const defPage = segPage(rec.seg[+sym.seg]);
    if (defPage === null) continue;
    const val = parseInt(sym.val, 16);
    if (val < 0xE000) continue;
    for (const lineId of new Set(sym.ref.split("+"))) {
        const line = rec.line[+lineId];
        if (!line || line.span === undefined) continue;
        for (const spanId of line.span.split("+")) {
            const seg = rec.seg[+rec.span[+spanId].seg];
            const refPage = segPage(seg);
            if (refPage === null || refPage === defPage) continue;
            const fileName = rec.file[+line.file].name;
            const text = srcLine(fileName, +line.line);
            if (isExplicit(text, sym.name) || isFarWord(text)) continue;
            problems++;
            console.log(`${fileName}:${line.line}: page ${refPage} (${seg.name}) uses ` +
                        `${sym.name} = $${val.toString(16).toUpperCase()} on page ${defPage}`);
        }
    }
}
console.log(problems ? `${problems} cross-page reference(s)` : "No cross-page references");
process.exit(problems ? 1 : 0);
