// ****************************************************************************
// numbers.js - the numbers' tests (the library, C's).  (tests.js, the index: the suites in
// their order; ../lib: the helpers they share)
'use strict';
const hydrafs = require('../../sim/tools/hydrafs.js');
const numtest = require('../numtest.js');
const { card, imageCard, typed } = require('../lib/common.js');

// The numbers in C (cnum's lines, as the C test's): num.h's test (ntest: its "ok" lines), calc at rc (rc's own
// characters quoted: ^ * ( ) #), and the assembly sample nsum (numbers.inc's macros, numlib.s)
const CNUM_LINES = [
  ["/sd/0/sample/c/ntest","ok - num_init: the libraries",true],
  ["calc 2/3 + 0.5; calc sqrt 2; calc -b x 255","7/6\n1.41421356237\nFF"],
  ["calc '2^100'; calc -d 30 pi; calc -b '#b' 0.75","1267650600228229401496703205376\n3.14159265358979323846264338328\n#b0.11"],
  ["calc '(1+2)*3'; calc 'log(e)^2'; calc 'sqrt 2^2'; calc 'gcd(12, 18)'","9\n1\n2\n6"],
  ["calc -17 % 5; calc 'fixed(1/3, 5)'; calc '#xFF + 1'; calc sqrt -4; calc -b c 5","-2\n0.33333\n256\n2i\n--+"],
  ["calc '4/3 - 8/3i'; calc '3.9 + 1/2i'; calc 1/2i; calc '1/(2i)'; calc '4/3+3.7i'","4/3-8/3i\n3.9+1/2i\n1/2i\n-1/2i\n4/3+3.7i"],
  ["calc 1/0; echo $status; calc foo; calc 2 +; calc -b zz 5", [
    "calc: division by zero",
    "division by zero",
    "calc: foo: unknown",
    "calc: an operand is missing",
    "calc: zz: not a base",
  ].join('\n')],
  ["echo 1/3+1/6 >/ram/e; echo x >>/ram/e; echo '0.1 + 0.2' >>/ram/e; calc </ram/e; echo $status","1/2\ncalc: x: unknown\n0.3\nx: unknown"],
  ["calc 'fib 2000' | wc -c; calc -b b '2^1000' | wc -c","    419\n   1002"],
  ["/sd/0/sample/nsum 1/3 0.5 2; /sd/0/sample/nsum 1 x; echo $status","17/6\nsqrt 1.68325082306\nnot a number"],
];

module.exports = {
  tests: [
    {
      name: 'cnum', what: 'the numbers in C and assembly: num.h\'s test (ntest: the number libraries from C, printf\'s and scanf\'s %N and %{base}), calc at rc (its operators, functions, bases, digits, errors, its input a line at a time, long results), the assembly sample nsum (numbers.inc, numlib.s)',
      init: 't_rc', cycles: 150e6,
      // (Each line typed at its prompt, as the C test's)
      machine: { input: CNUM_LINES.map(l => 'ā' + l[0] + '\r').join('') },
      expect: [...CNUM_LINES.map(l => '% ' + l[0] + '\n' + (l[2] ? l[1] : l[1] + '\n%')), '\nntest: 0 failed\n%'],
      sdcard: true
    },
    {
      name: 'numbers', what: 'the numbers library (modules/numbers): its calls as a card\'s file has them (tests/numtest.js), each checked against the reference (sim/tools/numfmt.js), and to keep the caller\'s bank, zero page and r0-r3',
      init: 't_num', cycles: 3000e6,
      get machine() { this.calls = numtest.calls(); this.file = numtest.card(this.calls); this.sd = [imageCard(0, this.file, 16384)]; return { sd: this.sd }; },
      check() {
        this.sd[0].save();
        const v = new hydrafs.Volume(this.file), out = v.read(v.walk('num.out'));
        v.close();
        this.notes = [this.calls.length + ' calls'];
        return numtest.check(this.calls, out);
      },
    },
  ],
};
