// The ROMs' checksums, for the hardware test (hwtest/hwt_rom.s): a CRC-16 of each BIOS ROM page and of each
// paged ROM bank the image uses, written into the table at the end of the hardware test's paged ROM bank
// (HWT_SUMS, $DF00 in bank 1).  Run by makeC02.bat after the link.
//
// Usage (from os_rom): node tools/romsum.js bin/os_rom_C02.bin bin/paged_rom_C02.bin
//
// The CRC is CRC-16/CCITT-FALSE (polynomial $1021, from $FFFF, no reflection), over:
//   a BIOS ROM page: $E000-$FEFF as the CPU sees it (the page's first $1F00 bytes)
//   a paged ROM bank: $A000-$DEFF as the CPU sees it (its halves are swapped in the file: $A000 is at
//                     offset $2000 of the bank), so not the table's 256 bytes at $DF00-$DFFF
// The table: the BIOS pages (1 byte), the paged banks (1), then a CRC for each page, then for each bank
// (2 bytes each, low byte first): 127 CRCs at most (the hardware test indexes them by 2 * the entry, in a byte),
// so 16 pages and 111 banks.

"use strict";
const fs = require("fs");

const BANK = 0x4000, PAGE = 0x2000, HWT_BANK = 1, SUMS = 0xDF00, SUMS_SIZE = 256;
const [biosName, pagedName] = process.argv.slice(2);
const bios = fs.readFileSync(biosName), paged = fs.readFileSync(pagedName);

function crc16(get, n) {
    let c = 0xFFFF;
    for (let i = 0; i < n; i++) {
        c ^= get(i) << 8;
        for (let k = 0; k < 8; k++) c = c & 0x8000 ? ((c << 1) ^ 0x1021) & 0xFFFF : (c << 1) & 0xFFFF;
    }
    return c;
}
// (The halves swapped; and on the V1 board a bank number's bits 2 and 3, and 6 and 7, trade places before they reach
// the chips: bank b, as the CPU selects it, is in the image at bank swap(b)'s place: sim/tools/mkromdisk.js)
const swap = b => (b & 0x33) | ((b & 0x04) << 1) | ((b & 0x08) >> 1) | ((b & 0x40) << 1) | ((b & 0x80) >> 1);
const pagedOfs = (bank, a) => swap(bank) * BANK + ((a - 0xA000) ^ 0x2000);

const pages = bios.length / PAGE, banks = paged.length / BANK;
if (2 + 2 * (pages + banks) > SUMS_SIZE) throw new Error("too many pages and banks for the table");
const table = Buffer.alloc(SUMS_SIZE);
table[0] = pages;
table[1] = banks;
let at = 2;
for (let p = 0; p < pages; p++, at += 2) table.writeUInt16LE(crc16(i => bios[p * PAGE + i], 0x1F00), at);
// (A bank the CPU selects can be past the image's end in the chips' order, where the chips are erased: $FF)
const pagedByte = o => o < paged.length ? paged[o] : 0xFF;
for (let b = 0; b < banks; b++, at += 2) table.writeUInt16LE(crc16(i => pagedByte(pagedOfs(b, 0xA000 + i)), SUMS - 0xA000), at);
for (let i = 0; i < SUMS_SIZE; i++) paged[pagedOfs(HWT_BANK, SUMS + i)] = table[i];
fs.writeFileSync(pagedName, paged);
console.log("ROM checksums: " + pages + " BIOS pages, " + banks + " paged ROM banks");
