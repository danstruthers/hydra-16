// ****************************************************************************
// opm.js - the YM2151's sound: its 8 channels of 4 operators (the 8 algorithms, operator 1's feedback), each
// operator's phase (key code, key fraction, DT1, DT2, MUL) and envelope (attack, decay, sustain, release, key
// scaling), the LFO (its 4 waveforms, AM and PM, each channel's sensitivity), the noise on channel 7's last operator,
// and the YM3012 DAC's 10.3 floating point: a sample at a time, at the chip's rate (its clock / 64: 55,930 a second
// at 3.58 MHz), the left and right outputs as 16-bit numbers.  Only the sound: ym2151.js keeps the chip's timing
// (its busy time, timers and IRQ) and calls this for each register written.
//   It's a port to JavaScript of ymfm's YM2151 (Aaron Giles's: ymfm_opm.cpp and the parts of ymfm_fm.ipp the OPM
// uses, as the X16's emulator has it, C:\source\x16-emulator\src\extern\ymfm), its tables copied from there, so the
// Hydra sounds as the X16's emulator does.  ymfm's licence:
//
//   BSD 3-Clause License
//
//   Copyright (c) 2021, Aaron Giles
//   All rights reserved.
//
//   Redistribution and use in source and binary forms, with or without modification, are permitted provided that
//   the following conditions are met:
//   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
//      following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//      following disclaimer in the documentation and/or other materials provided with the distribution.
//   3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
//      products derived from this software without specific prior written permission.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
//   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Interface: createOpm() gives { write(reg, value), sample(out) (one sample: out[0] left, out[1] right), reset() }.
'use strict';

// ---- ymfm's tables (ymfm_fm.ipp's, as they are)
// abs_sin_attenuation's: |sin| over a quarter wave, as 4.8 logarithmic attenuation
const SIN = new Uint16Array([
  0x859, 0x6C3, 0x607, 0x58B, 0x52E, 0x4E4, 0x4A6, 0x471, 0x443, 0x41A, 0x3F5, 0x3D3, 0x3B5, 0x398, 0x37E, 0x365,
  0x34E, 0x339, 0x324, 0x311, 0x2FF, 0x2ED, 0x2DC, 0x2CD, 0x2BD, 0x2AF, 0x2A0, 0x293, 0x286, 0x279, 0x26D, 0x261,
  0x256, 0x24B, 0x240, 0x236, 0x22C, 0x222, 0x218, 0x20F, 0x206, 0x1FD, 0x1F5, 0x1EC, 0x1E4, 0x1DC, 0x1D4, 0x1CD,
  0x1C5, 0x1BE, 0x1B7, 0x1B0, 0x1A9, 0x1A2, 0x19B, 0x195, 0x18F, 0x188, 0x182, 0x17C, 0x177, 0x171, 0x16B, 0x166,
  0x160, 0x15B, 0x155, 0x150, 0x14B, 0x146, 0x141, 0x13C, 0x137, 0x133, 0x12E, 0x129, 0x125, 0x121, 0x11C, 0x118,
  0x114, 0x10F, 0x10B, 0x107, 0x103, 0x0FF, 0x0FB, 0x0F8, 0x0F4, 0x0F0, 0x0EC, 0x0E9, 0x0E5, 0x0E2, 0x0DE, 0x0DB,
  0x0D7, 0x0D4, 0x0D1, 0x0CD, 0x0CA, 0x0C7, 0x0C4, 0x0C1, 0x0BE, 0x0BB, 0x0B8, 0x0B5, 0x0B2, 0x0AF, 0x0AC, 0x0A9,
  0x0A7, 0x0A4, 0x0A1, 0x09F, 0x09C, 0x099, 0x097, 0x094, 0x092, 0x08F, 0x08D, 0x08A, 0x088, 0x086, 0x083, 0x081,
  0x07F, 0x07D, 0x07A, 0x078, 0x076, 0x074, 0x072, 0x070, 0x06E, 0x06C, 0x06A, 0x068, 0x066, 0x064, 0x062, 0x060,
  0x05E, 0x05C, 0x05B, 0x059, 0x057, 0x055, 0x053, 0x052, 0x050, 0x04E, 0x04D, 0x04B, 0x04A, 0x048, 0x046, 0x045,
  0x043, 0x042, 0x040, 0x03F, 0x03E, 0x03C, 0x03B, 0x039, 0x038, 0x037, 0x035, 0x034, 0x033, 0x031, 0x030, 0x02F,
  0x02E, 0x02D, 0x02B, 0x02A, 0x029, 0x028, 0x027, 0x026, 0x025, 0x024, 0x023, 0x022, 0x021, 0x020, 0x01F, 0x01E,
  0x01D, 0x01C, 0x01B, 0x01A, 0x019, 0x018, 0x017, 0x017, 0x016, 0x015, 0x014, 0x014, 0x013, 0x012, 0x011, 0x011,
  0x010, 0x00F, 0x00F, 0x00E, 0x00D, 0x00D, 0x00C, 0x00C, 0x00B, 0x00A, 0x00A, 0x009, 0x009, 0x008, 0x008, 0x007,
  0x007, 0x007, 0x006, 0x006, 0x005, 0x005, 0x005, 0x004, 0x004, 0x004, 0x003, 0x003, 0x003, 0x002, 0x002, 0x002,
  0x002, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x001, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
]);
// attenuation_to_volume's: 2^-x as 10-bit mantissas (the leading 1 in, shifted left 2), in reverse
const POWER = new Uint16Array([
  0x1FE8, 0x1FD4, 0x1FBC, 0x1FA8, 0x1F90, 0x1F7C, 0x1F68, 0x1F50, 0x1F3C, 0x1F24, 0x1F10, 0x1EFC,
  0x1EE4, 0x1ED0, 0x1EB8, 0x1EA4, 0x1E90, 0x1E7C, 0x1E64, 0x1E50, 0x1E3C, 0x1E28, 0x1E10, 0x1DFC,
  0x1DE8, 0x1DD4, 0x1DC0, 0x1DA8, 0x1D94, 0x1D80, 0x1D6C, 0x1D58, 0x1D44, 0x1D30, 0x1D1C, 0x1D08,
  0x1CF4, 0x1CE0, 0x1CCC, 0x1CB8, 0x1CA4, 0x1C90, 0x1C7C, 0x1C68, 0x1C54, 0x1C40, 0x1C2C, 0x1C18,
  0x1C08, 0x1BF4, 0x1BE0, 0x1BCC, 0x1BB8, 0x1BA4, 0x1B94, 0x1B80, 0x1B6C, 0x1B58, 0x1B48, 0x1B34,
  0x1B20, 0x1B10, 0x1AFC, 0x1AE8, 0x1AD4, 0x1AC4, 0x1AB0, 0x1AA0, 0x1A8C, 0x1A78, 0x1A68, 0x1A54,
  0x1A44, 0x1A30, 0x1A20, 0x1A0C, 0x19FC, 0x19E8, 0x19D8, 0x19C4, 0x19B4, 0x19A0, 0x1990, 0x197C,
  0x196C, 0x195C, 0x1948, 0x1938, 0x1924, 0x1914, 0x1904, 0x18F0, 0x18E0, 0x18D0, 0x18C0, 0x18AC,
  0x189C, 0x188C, 0x1878, 0x1868, 0x1858, 0x1848, 0x1838, 0x1824, 0x1814, 0x1804, 0x17F4, 0x17E4,
  0x17D4, 0x17C0, 0x17B0, 0x17A0, 0x1790, 0x1780, 0x1770, 0x1760, 0x1750, 0x1740, 0x1730, 0x1720,
  0x1710, 0x1700, 0x16F0, 0x16E0, 0x16D0, 0x16C0, 0x16B0, 0x16A0, 0x1690, 0x1680, 0x1670, 0x1664,
  0x1654, 0x1644, 0x1634, 0x1624, 0x1614, 0x1604, 0x15F8, 0x15E8, 0x15D8, 0x15C8, 0x15BC, 0x15AC,
  0x159C, 0x158C, 0x1580, 0x1570, 0x1560, 0x1550, 0x1544, 0x1534, 0x1524, 0x1518, 0x1508, 0x14F8,
  0x14EC, 0x14DC, 0x14D0, 0x14C0, 0x14B0, 0x14A4, 0x1494, 0x1488, 0x1478, 0x146C, 0x145C, 0x1450,
  0x1440, 0x1430, 0x1424, 0x1418, 0x1408, 0x13FC, 0x13EC, 0x13E0, 0x13D0, 0x13C4, 0x13B4, 0x13A8,
  0x139C, 0x138C, 0x1380, 0x1370, 0x1364, 0x1358, 0x1348, 0x133C, 0x1330, 0x1320, 0x1314, 0x1308,
  0x12F8, 0x12EC, 0x12E0, 0x12D4, 0x12C4, 0x12B8, 0x12AC, 0x12A0, 0x1290, 0x1284, 0x1278, 0x126C,
  0x1260, 0x1250, 0x1244, 0x1238, 0x122C, 0x1220, 0x1214, 0x1208, 0x11F8, 0x11EC, 0x11E0, 0x11D4,
  0x11C8, 0x11BC, 0x11B0, 0x11A4, 0x1198, 0x118C, 0x1180, 0x1174, 0x1168, 0x115C, 0x1150, 0x1144,
  0x1138, 0x112C, 0x1120, 0x1114, 0x1108, 0x10FC, 0x10F0, 0x10E4, 0x10D8, 0x10CC, 0x10C0, 0x10B4,
  0x10A8, 0x10A0, 0x1094, 0x1088, 0x107C, 0x1070, 0x1064, 0x1058, 0x1050, 0x1044, 0x1038, 0x102C,
  0x1020, 0x1018, 0x100C, 0x1000,
]);
// attenuation_increment's: each rate's 8 envelope steps, 4 bits each
const INCREMENT = new Uint32Array([
  0x00000000, 0x00000000, 0x10101010, 0x10101010,
  0x10101010, 0x10101010, 0x11101110, 0x11101110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x10101010, 0x10111010, 0x11101110, 0x11111110,
  0x11111111, 0x21112111, 0x21212121, 0x22212221,
  0x22222222, 0x42224222, 0x42424242, 0x44424442,
  0x44444444, 0x84448444, 0x84848484, 0x88848884,
  0x88888888, 0x88888888, 0x88888888, 0x88888888,
]);
// detune_adjustment's: DT1's steps for each 5-bit keycode (4 a keycode: DT1 0-3)
const DETUNE = new Uint8Array([
   0,  0,  1,  2,  0,  0,  1,  2,  0,  0,  1,  2,  0,  0,  1,  2,
   0,  1,  2,  2,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,
   0,  1,  2,  4,  0,  1,  3,  4,  0,  1,  3,  4,  0,  1,  3,  5,
   0,  2,  4,  5,  0,  2,  4,  6,  0,  2,  4,  6,  0,  2,  5,  7,
   0,  2,  5,  8,  0,  3,  6,  8,  0,  3,  6,  9,  0,  3,  7, 10,
   0,  4,  8, 11,  0,  4,  8, 12,  0,  4,  9, 13,  0,  5, 10, 14,
   0,  5, 11, 16,  0,  6, 12, 17,  0,  6, 13, 19,  0,  7, 14, 20,
   0,  8, 16, 22,  0,  8, 16, 22,  0,  8, 16, 22,  0,  8, 16, 22,
]);
// opm_key_code_to_phase_step's: an octave's phase steps (block 7), 64 a note (David Viens's, from a chip)
const PHASE_STEP = new Uint32Array([
  41568, 41600, 41632, 41664, 41696, 41728, 41760, 41792, 41856, 41888, 41920, 41952, 42016, 42048, 42080, 42112,
  42176, 42208, 42240, 42272, 42304, 42336, 42368, 42400, 42464, 42496, 42528, 42560, 42624, 42656, 42688, 42720,
  42784, 42816, 42848, 42880, 42912, 42944, 42976, 43008, 43072, 43104, 43136, 43168, 43232, 43264, 43296, 43328,
  43392, 43424, 43456, 43488, 43552, 43584, 43616, 43648, 43712, 43744, 43776, 43808, 43872, 43904, 43936, 43968,
  44032, 44064, 44096, 44128, 44192, 44224, 44256, 44288, 44352, 44384, 44416, 44448, 44512, 44544, 44576, 44608,
  44672, 44704, 44736, 44768, 44832, 44864, 44896, 44928, 44992, 45024, 45056, 45088, 45152, 45184, 45216, 45248,
  45312, 45344, 45376, 45408, 45472, 45504, 45536, 45568, 45632, 45664, 45728, 45760, 45792, 45824, 45888, 45920,
  45984, 46016, 46048, 46080, 46144, 46176, 46208, 46240, 46304, 46336, 46368, 46400, 46464, 46496, 46528, 46560,
  46656, 46688, 46720, 46752, 46816, 46848, 46880, 46912, 46976, 47008, 47072, 47104, 47136, 47168, 47232, 47264,
  47328, 47360, 47392, 47424, 47488, 47520, 47552, 47584, 47648, 47680, 47744, 47776, 47808, 47840, 47904, 47936,
  48032, 48064, 48096, 48128, 48192, 48224, 48288, 48320, 48384, 48416, 48448, 48480, 48544, 48576, 48640, 48672,
  48736, 48768, 48800, 48832, 48896, 48928, 48992, 49024, 49088, 49120, 49152, 49184, 49248, 49280, 49344, 49376,
  49440, 49472, 49504, 49536, 49600, 49632, 49696, 49728, 49792, 49824, 49856, 49888, 49952, 49984, 50048, 50080,
  50144, 50176, 50208, 50240, 50304, 50336, 50400, 50432, 50496, 50528, 50560, 50592, 50656, 50688, 50752, 50784,
  50880, 50912, 50944, 50976, 51040, 51072, 51136, 51168, 51232, 51264, 51328, 51360, 51424, 51456, 51488, 51520,
  51616, 51648, 51680, 51712, 51776, 51808, 51872, 51904, 51968, 52000, 52064, 52096, 52160, 52192, 52224, 52256,
  52384, 52416, 52448, 52480, 52544, 52576, 52640, 52672, 52736, 52768, 52832, 52864, 52928, 52960, 52992, 53024,
  53120, 53152, 53216, 53248, 53312, 53344, 53408, 53440, 53504, 53536, 53600, 53632, 53696, 53728, 53792, 53824,
  53920, 53952, 54016, 54048, 54112, 54144, 54208, 54240, 54304, 54336, 54400, 54432, 54496, 54528, 54592, 54624,
  54688, 54720, 54784, 54816, 54880, 54912, 54976, 55008, 55072, 55104, 55168, 55200, 55264, 55296, 55360, 55392,
  55488, 55520, 55584, 55616, 55680, 55712, 55776, 55808, 55872, 55936, 55968, 56032, 56064, 56128, 56160, 56224,
  56288, 56320, 56384, 56416, 56480, 56512, 56576, 56608, 56672, 56736, 56768, 56832, 56864, 56928, 56960, 57024,
  57120, 57152, 57216, 57248, 57312, 57376, 57408, 57472, 57536, 57568, 57632, 57664, 57728, 57792, 57824, 57888,
  57952, 57984, 58048, 58080, 58144, 58208, 58240, 58304, 58368, 58400, 58464, 58496, 58560, 58624, 58656, 58720,
  58784, 58816, 58880, 58912, 58976, 59040, 59072, 59136, 59200, 59232, 59296, 59328, 59392, 59456, 59488, 59552,
  59648, 59680, 59744, 59776, 59840, 59904, 59936, 60000, 60064, 60128, 60160, 60224, 60288, 60320, 60384, 60416,
  60512, 60544, 60608, 60640, 60704, 60768, 60800, 60864, 60928, 60992, 61024, 61088, 61152, 61184, 61248, 61280,
  61376, 61408, 61472, 61536, 61600, 61632, 61696, 61760, 61824, 61856, 61920, 61984, 62048, 62080, 62144, 62208,
  62272, 62304, 62368, 62432, 62496, 62528, 62592, 62656, 62720, 62752, 62816, 62880, 62944, 62976, 63040, 63104,
  63200, 63232, 63296, 63360, 63424, 63456, 63520, 63584, 63648, 63680, 63744, 63808, 63872, 63904, 63968, 64032,
  64096, 64128, 64192, 64256, 64320, 64352, 64416, 64480, 64544, 64608, 64672, 64704, 64768, 64832, 64896, 64928,
  65024, 65056, 65120, 65184, 65248, 65312, 65376, 65408, 65504, 65536, 65600, 65664, 65728, 65792, 65856, 65888,
  65984, 66016, 66080, 66144, 66208, 66272, 66336, 66368, 66464, 66496, 66560, 66624, 66688, 66752, 66816, 66848,
  66944, 66976, 67040, 67104, 67168, 67232, 67296, 67328, 67424, 67456, 67520, 67584, 67648, 67712, 67776, 67808,
  67904, 67936, 68000, 68064, 68128, 68192, 68256, 68288, 68384, 68448, 68512, 68544, 68640, 68672, 68736, 68800,
  68896, 68928, 68992, 69056, 69120, 69184, 69248, 69280, 69376, 69440, 69504, 69536, 69632, 69664, 69728, 69792,
  69920, 69952, 70016, 70080, 70144, 70208, 70272, 70304, 70400, 70464, 70528, 70560, 70656, 70688, 70752, 70816,
  70912, 70976, 71040, 71104, 71136, 71232, 71264, 71360, 71424, 71488, 71552, 71616, 71648, 71744, 71776, 71872,
  71968, 72032, 72096, 72160, 72192, 72288, 72320, 72416, 72480, 72544, 72608, 72672, 72704, 72800, 72832, 72928,
  72992, 73056, 73120, 73184, 73216, 73312, 73344, 73440, 73504, 73568, 73632, 73696, 73728, 73824, 73856, 73952,
  74080, 74144, 74208, 74272, 74304, 74400, 74432, 74528, 74592, 74656, 74720, 74784, 74816, 74912, 74944, 75040,
  75136, 75200, 75264, 75328, 75360, 75456, 75488, 75584, 75648, 75712, 75776, 75840, 75872, 75968, 76000, 76096,
  76224, 76288, 76352, 76416, 76448, 76544, 76576, 76672, 76736, 76800, 76864, 76928, 77024, 77120, 77152, 77248,
  77344, 77408, 77472, 77536, 77568, 77664, 77696, 77792, 77856, 77920, 77984, 78048, 78144, 78240, 78272, 78368,
  78464, 78528, 78592, 78656, 78688, 78784, 78816, 78912, 78976, 79040, 79104, 79168, 79264, 79360, 79392, 79488,
  79616, 79680, 79744, 79808, 79840, 79936, 79968, 80064, 80128, 80192, 80256, 80320, 80416, 80512, 80544, 80640,
  80768, 80832, 80896, 80960, 80992, 81088, 81120, 81216, 81280, 81344, 81408, 81472, 81568, 81664, 81696, 81792,
  81952, 82016, 82080, 82144, 82176, 82272, 82304, 82400, 82464, 82528, 82592, 82656, 82752, 82848, 82880, 82976,
]);

// An operator's waveform: |sin| as a 4.8 logarithmic attenuation, over 10 bits of phase (bit 15: negative)
const WAVE = new Uint16Array(1024);
for (let i = 0; i < 1024; i++) WAVE[i] = SIN[((i & 0x100) ? ~i : i) & 0xFF] | ((i >> 9) & 1) << 15;
// The LFO's waveforms: AM in the low 8 bits, PM (signed) in the upper 8; 3 (noise) filled in as it runs
const LFO_WAVE = [0, 1, 2, 3].map(() => new Int16Array(256));
for (let i = 0; i < 256; i++) {
  let am = i ^ 0xFF, pm = i;                                  // Sawtooth
  LFO_WAVE[0][i] = am | (pm << 8);
  am = (i & 0x80) ? 0 : 0xFF; pm = am ^ 0x80;                 // Square
  LFO_WAVE[1][i] = am | (pm << 8);
  am = ((i & 0x80) ? (i << 1) : ((i ^ 0xFF) << 1)) & 0xFF;     // Triangle
  pm = (i & 0x40) ? am : ~am & 0xFF;
  LFO_WAVE[2][i] = am | (pm << 8);
}
const DT2_DELTA = [0, ((600 * 64 + 50) / 100) | 0, ((781 * 64 + 50) / 100) | 0, ((950 * 64 + 50) / 100) | 0];
const ATTACK = 0, DECAY = 1, SUSTAIN = 2, RELEASE = 3;       // Envelope states
const EG_QUIET = 0x380;
const DYNAMIC = -1;                                           // (A phase step that the LFO's PM changes)
// The operators of channel c in their order of connection (1-4): the register map's M1, C1, M2, C2
const OPS = c => [c, c + 16, c + 8, c + 24];
// The algorithms: each operator's input (opout's index: 0 none, 1-3 an operator, 5 O1+O2, 6 O1+O3, 7 O2+O3) and
// which of operators 1-3 are summed into the output with operator 4
const ALGORITHMS = [
  [1, 2, 3, 0, 0, 0], [0, 5, 3, 0, 0, 0], [0, 2, 6, 0, 0, 0], [1, 0, 7, 0, 0, 0],
  [1, 0, 3, 0, 1, 0], [1, 1, 1, 0, 1, 1], [1, 0, 0, 0, 1, 1], [0, 0, 0, 1, 1, 1],
];

const attToVol = x => POWER[x & 0xFF] >> (x >> 8);             // 5.8 attenuation to a 13-bit volume
const attIncrement = (rate, index) => (INCREMENT[rate] >>> (4 * index)) & 15;
const effectiveRate = (raw, ksr) => raw === 0 ? 0 : Math.min(raw + ksr, 63);
function detuneAdjustment(dt, keycode) { const r = DETUNE[keycode * 4 + (dt & 3)]; return (dt & 4) ? -r : r; }
// A block (3 bits), key code (4 bits) and key fraction (6 bits), plus delta (64ths of a semitone): a phase step
function keyCodeToPhaseStep(blockFreq, delta) {
  let block = (blockFreq >> 10) & 7;
  const adjusted = ((blockFreq >> 6) & 15) - ((blockFreq >> 8) & 3);   // (12 notes over the 16 codes)
  let eff = ((adjusted << 6) | (blockFreq & 63)) + delta;
  if (eff < 0 || eff >= 768) {
    if (eff < 0) { eff += 768; if (block === 0) return PHASE_STEP[0] >> 7; block--; }
    else {
      eff -= 768;
      if (eff >= 768) { block++; eff -= 768; }
      if (block >= 7) return PHASE_STEP[767];
      block++;
    }
  }
  return PHASE_STEP[eff] >> (block ^ 7);
}
// The YM3012's 10.3 floating point: the bits it loses, lost
function roundtripFp(v) {
  if (v < -32768) return -32768;
  if (v > 32767) return 32767;
  const scan = v ^ (v >> 31);
  const exponent = Math.max(7 - Math.clz32(scan << 17), 1) - 1;
  return v & ~((1 << exponent) - 1);
}

function createOpm() {
  const regs = new Uint8Array(256);
  // The operators (32, by register offset)
  const phase = new Int32Array(32), att = new Int32Array(32), state = new Uint8Array(32), keyState = new Uint8Array(32),
    keyonLive = new Uint8Array(32);
  const cBlockFreq = new Int32Array(32), cDetune = new Int32Array(32), cMultiple = new Int32Array(32), cPhaseStep = new Int32Array(32),
    cTotalLevel = new Int32Array(32), cSustain = new Int32Array(32), cRate = new Uint8Array(32 * 4);
  // The channels (8)
  const fb0 = new Int32Array(8), fb1 = new Int32Array(8), fbIn = new Int32Array(8);
  const opout = new Int32Array(8);
  let envCounter = 0, lfoCounter = 0, noiseLfsr = 1, noiseCounter = 0, noiseState = 0, lfoAm = 0;
  let modified = true, prepareCount = 0, activeChannels = 0;

  function reset() {
    regs.fill(0);
    for (let c = 0; c < 8; c++) regs[0x20 + c] = 0xC0;         // (Both outputs on)
    phase.fill(0); att.fill(0x3FF); state.fill(RELEASE); keyState.fill(0); keyonLive.fill(0);
    fb0.fill(0); fb1.fill(0); fbIn.fill(0);
    modified = true;
  }

  // ---- Registers
  function write(reg, v) {
    reg &= 0xFF;
    if (reg === 0x19) regs[0x19 + (v >> 7)] = v;              // (AMD, or PMD: kept at $1A)
    else if (reg !== 0x1A) regs[reg] = v;
    modified = true;
    if (reg === 0x08) {                                       // Key on/off: M1, C1, M2, C2 in bits 3-6
      const ops = OPS(v & 7);
      for (let n = 0; n < 4; n++) keyonLive[ops[n]] = (v >> (3 + n)) & 1;
    }
  }

  // ---- An operator's cache, and its key state, before clocking; OUT: whether it's sounding
  function phaseStep(c, op, lfoRawPm) {
    let delta = DT2_DELTA[regs[0xC0 + op] >> 6];
    const pms = (regs[0x38 + c] >> 4) & 7;
    if (pms) delta += pms < 6 ? lfoRawPm >> (6 - pms) : lfoRawPm << (pms - 5);
    const step = keyCodeToPhaseStep(cBlockFreq[op], delta) + cDetune[op];
    return (step * cMultiple[op]) >> 1;
  }
  function prepareOp(c, op) {
    const blockFreq = cBlockFreq[op] = ((regs[0x28 + c] & 0x7F) << 6) | ((regs[0x30 + c] >> 2) & 0x3F);
    const keycode = (blockFreq >> 8) & 0x1F;
    cDetune[op] = detuneAdjustment((regs[0x40 + op] >> 4) & 7, keycode);
    cMultiple[op] = (regs[0x40 + op] & 15) * 2 || 1;
    cPhaseStep[op] = (regs[0x1A] & 0x7F) === 0 || ((regs[0x38 + c] >> 4) & 7) === 0 ? phaseStep(c, op, 0) : DYNAMIC;
    cTotalLevel[op] = (regs[0x60 + op] & 0x7F) << 3;
    let sl = regs[0xE0 + op] >> 4;
    sl |= (sl + 1) & 0x10;
    cSustain[op] = sl << 5;
    const ksr = keycode >> ((regs[0x80 + op] >> 6) ^ 3);
    cRate[op * 4 + ATTACK] = effectiveRate((regs[0x80 + op] & 31) * 2, ksr);
    cRate[op * 4 + DECAY] = effectiveRate((regs[0xA0 + op] & 31) * 2, ksr);
    cRate[op * 4 + SUSTAIN] = effectiveRate((regs[0xC0 + op] & 31) * 2, ksr);
    cRate[op * 4 + RELEASE] = effectiveRate((regs[0xE0 + op] & 15) * 4 + 2, ksr);
    const ks = keyonLive[op];                                 // The key state
    if (ks !== keyState[op]) {
      keyState[op] = ks;
      if (ks) {                                               // On: the attack, from phase 0
        if (state[op] !== ATTACK) {
          state[op] = ATTACK; phase[op] = 0;
          if (cRate[op * 4 + ATTACK] >= 62) att[op] = 0;
        }
      } else if (state[op] < RELEASE) state[op] = RELEASE;    // Off: the release
    }
    return state[op] !== RELEASE || att[op] < EG_QUIET;
  }

  // ---- Clocking (a sample's)
  function clockEnvelope(op, counter) {
    if (state[op] === ATTACK && att[op] === 0) state[op] = DECAY;
    if (state[op] === DECAY && att[op] >= cSustain[op]) state[op] = SUSTAIN;
    const rate = cRate[op * 4 + state[op]], shift = rate >> 2;
    const c = counter << shift;
    if (c & 0x7FF) return;
    const inc = attIncrement(rate, (c >>> (shift <= 11 ? 11 : shift)) & 7);
    if (state[op] === ATTACK) { if (rate < 62) att[op] += (~att[op] * inc) >> 4; }
    else { att[op] += inc; if (att[op] >= 0x400) att[op] = 0x3FF; }
  }
  function clockNoiseAndLfo() {
    const freq = (regs[0x0F] & 0x1F) ^ 0x1F;
    for (let rep = 0; rep < 2; rep++) {                       // (The noise: counted twice a sample)
      noiseLfsr = (noiseLfsr << 1) & 0x1FFFFFF;
      noiseLfsr |= ((noiseLfsr >> 17) ^ (noiseLfsr >> 14) ^ 1) & 1;
      if (noiseCounter++ >= freq) { noiseCounter = 0; noiseState = (noiseLfsr >> 17) & 1; }
    }
    const rate = regs[0x18];
    lfoCounter = (lfoCounter + ((0x10 | (rate & 15)) << (rate >> 4))) >>> 0;
    if (regs[0x01] & 2) lfoCounter = 0;                       // (The test register's LFO reset)
    const lfo = (lfoCounter >>> 22) & 0xFF, lfoNoise = (noiseLfsr >> 17) & 0xFF;
    LFO_WAVE[3][(lfo + 1) & 0xFF] = lfoNoise | (lfoNoise << 8);
    const ampm = LFO_WAVE[regs[0x1B] & 3][lfo];
    lfoAm = ((ampm & 0xFF) * (regs[0x19] & 0x7F)) >> 7;
    return ((ampm >> 8) * (regs[0x1A] & 0x7F)) >> 7;
  }
  function clock() {
    if (modified || prepareCount++ >= 4096) {                 // (A register written: every operator's cache again)
      activeChannels = 0;
      for (let c = 0; c < 8; c++) {
        const ops = OPS(c);
        let on = false;
        for (let n = 0; n < 4; n++) if (prepareOp(c, ops[n])) on = true;
        if (on) activeChannels |= 1 << c;
      }
      modified = false; prepareCount = 0;
    }
    envCounter = (envCounter + 1) >>> 0;                      // (The envelopes: every third sample)
    if ((envCounter & 3) === 3) envCounter = (envCounter + 1) >>> 0;
    const lfoRawPm = clockNoiseAndLfo();
    for (let c = 0; c < 8; c++) {
      fb0[c] = fb1[c]; fb1[c] = fbIn[c];
      for (let n = 0; n < 4; n++) {
        const op = n === 0 ? c : n === 1 ? c + 16 : n === 2 ? c + 8 : c + 24;
        if ((envCounter & 3) === 0) clockEnvelope(op, envCounter >>> 2);
        const step = cPhaseStep[op] === DYNAMIC ? phaseStep(c, op, lfoRawPm) : cPhaseStep[op];
        phase[op] = (phase[op] + step) & 0xFFFFF;
      }
    }
  }

  // ---- Output
  function envelopeAtt(op, am) {
    let r = att[op];
    if (regs[0xA0 + op] & 0x80) r += am;
    r += cTotalLevel[op];
    return r < 0x3FF ? r : 0x3FF;
  }
  function volume(op, ph, am) {                               // An operator's 14-bit value at phase ph
    if (att[op] > EG_QUIET) return 0;
    const s = WAVE[ph & 0x3FF], r = attToVol((s & 0x7FFF) + (envelopeAtt(op, am) << 2));
    return (s & 0x8000) ? -r : r;
  }
  function channel(c, out) {
    const ams = regs[0x38 + c] & 3, am = ams ? lfoAm << (ams - 1) : 0;
    const fb = (regs[0x20 + c] >> 3) & 7;
    const o1 = c, o2 = c + 16, o3 = c + 8, o4 = c + 24;
    const mod = fb ? (fb0[c] + fb1[c]) >> (10 - fb) : 0;
    const v1 = fbIn[c] = volume(o1, (phase[o1] >> 10) + mod, am);
    const outs = regs[0x20 + c] >> 6;
    if (!outs) return;
    const alg = ALGORITHMS[regs[0x20 + c] & 7];
    opout[0] = 0; opout[1] = v1;
    opout[2] = volume(o2, (phase[o2] >> 10) + (opout[alg[0]] >> 1), am);
    opout[5] = opout[1] + opout[2];
    opout[3] = volume(o3, (phase[o3] >> 10) + (opout[alg[1]] >> 1), am);
    opout[6] = opout[1] + opout[3];
    opout[7] = opout[2] + opout[3];
    let r;
    if ((regs[0x0F] & 0x80) && c === 7) {                     // (Noise: channel 7's operator 4)
      const n = (envelopeAtt(o4, am) ^ 0x3FF) << 1;
      r = noiseState & 1 ? -n : n;
    } else r = volume(o4, (phase[o4] >> 10) + (opout[alg[2]] >> 1), am);
    if (alg[3]) r = Math.max(-32768, Math.min(32767, r + opout[1]));
    if (alg[4]) r = Math.max(-32768, Math.min(32767, r + opout[2]));
    if (alg[5]) r = Math.max(-32768, Math.min(32767, r + opout[3]));
    if (outs & 1) out[0] += r;                                // (Bit 6: left; 7: right)
    if (outs & 2) out[1] += r;
  }
  // A sample: out[0] left, out[1] right (16 bits)
  function sample(out) {
    clock();
    out[0] = 0; out[1] = 0;
    for (let c = 0; c < 8; c++) if (activeChannels & (1 << c)) channel(c, out);
    out[0] = roundtripFp(out[0]); out[1] = roundtripFp(out[1]);
  }

  reset();
  return { write, sample, reset };
}

module.exports = { createOpm };
