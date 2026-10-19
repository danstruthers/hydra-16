/* cmdmatch.c - the command grammar's matcher, for the 65C02 (cc65). Host reference: sim/tools/commandcompile.js; the
   tables: sim/tools/commandtables.js (the grammar, the index, the aliases); the test data: sim/tools/commandmatch-gen.js.

   The matcher returns, for a rule (or a node) at a position, the set of positions where it can end, as a bit mask
   (bit p: the match ends before token p). Rules are memoised per position. Nothing is pushed on the stack for a step,
   so the stack grows with the grammar's nesting, not with the length of a parse. The grammar is unambiguous on the
   corpus, so no parse is ranked; the first answer is the answer.

   What makes it quick:
   - A word is found by a hash table (Pearson's hash, 128 buckets, chained): the grammar's words, the lexicon's (the
     words the open names look for, each with its LX_ flags) and the aliases' first words. Each entry heads the chain of
     the aliases that begin with its word, so the aliases are a trie of words: the first by the hash, the rest compared.
   - A rule is entered only if the token can begin it: its first words as a 64-bit Bloom filter, and its first kinds
     (the index table, from the host). An alternation of many options (the phrases) tries only the options the token
     can begin, from its first words, sorted (a binary search), and the options that begin with an open name.
   - The memo is a row a position, and only the rows a line uses are cleared.
   RAM: the memo (positions x rules x 2 bytes, 7 KB at most), the dictionary (about 3 KB), the aliases and tokens. */
#include <string.h>
#include "cmdmatch.h"
#include "cmd_ids.h"

#pragma register-vars (on)
#pragma inline-stdfuncs (on)

/* Node tags in the grammar table */
#define T_SEQ 1
#define T_ALT 2
#define T_OPT 3
#define T_REP 4
#define T_NT 5
#define T_TERM 6
#define T_QUOTE 7

/* Open-name kinds: what each open name accepts (the host's openOK), a bit each in a token's mask */
#define K_OBJ 0
#define K_ADJ 1
#define K_NUMBER 2
#define K_ORDINAL 3
#define K_TASK 4
#define K_SPELL 5
#define K_TEXT 6
#define K_NAMEWORD 7
#define K_DEITY 8
#define K_PRETENSE 9
#define K_QUOTED 10

/* The lexicon's flags (commandtables.js's LX) */
#define LX_ADJ 1
#define LX_TASK 2
#define LX_ORDINAL 4
#define LX_NUMBER 8
#define LX_NOTSPELL 16
#define LX_NOTOBJECT 32

/* The world's marks on the dictionary (cmd_world) */
#define W_WORLD 1     /* the world names a thing by the word */
#define W_NAME 2      /* a name given to a thing */
#define W_DEITY 4     /* a deity's name */

#define MAXRULES 240     /* a rule's number (and a name's) is a byte */
#define MAXWORDS 400
#define MAXEXTRA 200     /* the dictionary's words that are not the grammar's: the lexicon's and the aliases' */
#define MAXLEX (MAXWORDS + MAXEXTRA)
#define MAXALIAS 96      /* the default aliases: 86 of them, and room to add more */
#define MAXINDEX 24
#define MAXTOK 14        /* positions 0..14 fit a 16-bit mask; the corpus is at most 12 words */
#define MAXREP 12
#define NBUCKET 128
#define MAXSET 400      /* the memo cells a line may set before the whole memo is cleared instead */
#define NONE 0xFFFF      /* no word; and a memo entry not yet worked out */

#define RD16(p) (*(const unsigned *)(p))
#define HEAD(p) (((p)[1] & 0x80) ? (p) + 3 : (p) + 2)   /* past a composite's size: its count, or its child */

/* The grammar table */
static const unsigned char *gbase;
static const unsigned char *rulep[MAXRULES];
static const unsigned char *wordp[MAXWORDS];   /* each word's letters (unpacked at start-up, into wordtext) */
#define WORDTEXT 2400
static unsigned char wordtext[WORDTEXT];
static unsigned char wordlen[MAXWORDS];
static const unsigned char *kindp;             /* one kind byte per open name */
static unsigned char nrules, nnames;
static unsigned nwords;

/* The index table */
static const unsigned char *pearson;           /* the hash's permutation */
static const unsigned char *bloomrow[8];       /* the rules' first words: row k is bits 8k..8k+7 */
static const unsigned char *kindlo, *kindhi;   /* the rules' first kinds; kindhi's top bit: the rule can match nothing */
static unsigned char nindex;
static const unsigned char *ixnode[MAXINDEX], *ixalts[MAXINDEX], *ixent[MAXINDEX], *ixwild[MAXINDEX];
static unsigned ixnent[MAXINDEX];
static unsigned char ixnwild[MAXINDEX];

/* The dictionary: entries 0..nwords-1 are the grammar's words, the rest the lexicon's and the aliases' */
static unsigned lhead[NBUCKET];
static unsigned lnext[MAXLEX];
static unsigned char lhash[MAXLEX], lflags[MAXLEX], lalias[MAXLEX];   /* lalias: the first alias for the word, + 1 */
static unsigned char lmark[MAXLEX];   /* W_*: the world's marks */
static const unsigned char *xtext[MAXEXTRA];
static unsigned char xlen[MAXEXTRA];
static unsigned nlex, nlex_fixed, then_word;   /* nlex_fixed: the entries before the world's own words */

/* The aliases: the key (aliasfl: its first word's length), the verb (aliasve: its entry), and the next for the word */
static const unsigned char *aliasw[MAXALIAS], *aliasv[MAXALIAS];
static unsigned char aliaswl[MAXALIAS], aliasvl[MAXALIAS], aliasany[MAXALIAS], aliasfl[MAXALIAS], anext[MAXALIAS];
static unsigned aliasve[MAXALIAS];
static unsigned char nalias;

/* The line's tokens */
static char tbuf[200];
static unsigned char tbn, ntok, has_then, too_long;
static unsigned toke[MAXTOK + 1], tokw[MAXTOK + 1];   /* the dictionary entry, the grammar word (or NONE) */
static unsigned char tokwl[MAXTOK + 1], tokwh[MAXTOK + 1];   /* tokw's bytes */
static unsigned char tokq[MAXTOK + 1];                /* 1: quoted text */
static unsigned char tokoff[MAXTOK + 1], toklen[MAXTOK + 1];
static unsigned tokmask[MAXTOK + 1];                  /* bit k: the token fits the open-name kind k */
static unsigned char tokmlo[MAXTOK + 1], tokmhi[MAXTOK + 1];
static unsigned char tokbk[MAXTOK + 1], tokbb[MAXTOK + 1];   /* the token's Bloom byte and bit (bit 0 for no word) */

static unsigned char line_ref[3] = { T_NT, CMD_LINE, 0 };   /* the rule "line", as a node */
static unsigned memo[(MAXTOK + 1) * MAXRULES];        /* a row a position: each rule's ends from there */
static unsigned *memorow[MAXTOK + 1];
static unsigned *memoset[MAXSET];                     /* the cells this line set */
static unsigned nset;

static const unsigned bitv[16] = { 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
  0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000 };
static const unsigned char bitb[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

/* The letters of a packed word: 5 bits a letter (a to z, then ! = - ') */
static const char PACK[] = "abcdefghijklmnopqrstuvwxyz!=-'";

/* The node after the one at p: a composite is stepped over by its size, a leaf by its length. */
static const unsigned char *node_next(const unsigned char *p)
{
  switch (*p) {
    case T_SEQ:
    case T_ALT:
    case T_OPT:
    case T_REP:
      return p + ((p[1] & 0x80) ? (((unsigned)(p[1] & 0x7F) << 8) | p[2]) : p[1]);
    case T_NT: return p + 3;
    case T_TERM: return p + 2 + 2 * p[1];
    default: return p + 1;   /* T_QUOTE */
  }
}

/* None of the functions from here to the matcher calls itself: their locals can be static (quicker in cc65). */
#pragma static-locals (push, on)

static char letter_at(const unsigned char *packed, unsigned char nbytes, unsigned char j)
{
  unsigned bit, window;
  unsigned char b;
  bit = 5 * j;
  b = bit >> 3;
  window = packed[b];
  if (b + 1 < nbytes) window |= (unsigned)packed[b + 1] << 8;
  return PACK[(window >> (bit & 7)) & 31];
}

/* Does grammar word w spell the len letters of text? */
static unsigned char word_is(unsigned w, const char *text, unsigned char len)
{
  return wordlen[w] == len && memcmp(wordp[w], text, len) == 0;
}

/* Pearson's hash of a word: a byte at a time through the permutation, then its length (the host's wordHash) */
static unsigned char hash_of(const char *t, unsigned char len)
{
  unsigned char h, i;
  h = 0;
  for (i = 0; i < len; i++) h = pearson[h ^ (unsigned char)t[i]];
  return pearson[h ^ len];
}

static unsigned char entry_is(unsigned e, const char *t, unsigned char len)
{
  if (e < nwords) return word_is(e, t, len);
  e -= nwords;
  return xlen[e] == len && memcmp(xtext[e], t, len) == 0;
}

/* The dictionary entry for a word (h: its hash), or NONE */
static unsigned lookup(const char *t, unsigned char len, unsigned char h)
{
  unsigned e;
  e = lhead[h & (NBUCKET - 1)];
  while (e != NONE) {
    if (lhash[e] == h && entry_is(e, t, len)) return e;
    e = lnext[e];
  }
  return NONE;
}

static void link_entry(unsigned e, unsigned char h)
{
  unsigned char b;
  b = h & (NBUCKET - 1);
  lhash[e] = h;
  lflags[e] = 0;
  lalias[e] = 0;
  lmark[e] = 0;
  lnext[e] = lhead[b];
  lhead[b] = e;
}

/* The entry for a word, added if it is new (its letters stay where they are: in a table) */
static unsigned find_or_add(const unsigned char *t, unsigned char len)
{
  unsigned char h;
  unsigned e;
  h = hash_of((const char *)t, len);
  e = lookup((const char *)t, len, h);
  if (e != NONE) return e;
  if (nlex >= nwords + MAXEXTRA) return NONE;
  e = nlex++;
  xtext[e - nwords] = t;
  xlen[e - nwords] = len;
  link_entry(e, h);
  return e;
}

/* The grammar table: the counts, the packed words, the kind of each open name, and the rule bodies in order. */
static unsigned char read_grammar(const unsigned char *t)
{
  const unsigned char *p;
  unsigned i, e;
  unsigned char len, nb, j;
  p = t;
  if (p[0] != 'D' || p[1] != 'Q' || p[2] != '0' || p[3] != 'G' || p[4] != 7) return 0;
  if (RD16(p + 5) > MAXRULES || RD16(p + 7) > 255 || RD16(p + 9) > MAXWORDS) return 0;
  gbase = t;
  nrules = p[5];
  nnames = p[7];
  nwords = RD16(p + 9);
  p += 11;
  for (i = 0, e = 0; i < nwords; i++) {   /* each word unpacked: 5 bits a letter */
    len = p[0];
    nb = (5 * len + 7) >> 3;
    if (e + len > WORDTEXT) return 0;
    wordlen[i] = len;
    wordp[i] = wordtext + e;
    for (j = 0; j < len; j++) wordtext[e++] = letter_at(p + 1, nb, j);
    p += 1 + nb;
  }
  kindp = p;
  p += nnames - nrules;
  for (i = 0; i < nrules; i++) {   /* the rules follow one after another: each starts where the one before it ends */
    rulep[i] = p;
    p = node_next(p);
  }
  return 1;
}

/* The index table: the hash's permutation, the words' hashes (into the dictionary), the rules' first words and kinds,
   the lexicon (into the dictionary, with its flags), and the indexed alternations. */
static unsigned char read_index(const unsigned char *t)
{
  const unsigned char *p;
  unsigned i, n, e;
  p = t;
  if (p[0] != 'D' || p[1] != 'Q' || p[2] != '0' || p[3] != 'I' || p[4] != 1) return 0;
  if (RD16(p + 5) != nrules || RD16(p + 7) != nwords) return 0;
  p += 9;
  pearson = p;
  p += 256;
  for (i = 0; i < NBUCKET; i++) lhead[i] = NONE;
  for (i = 0; i < nwords; i++) link_entry(i, p[i]);
  nlex = nwords;
  p += nwords;
  for (i = 0; i < 8; i++) {
    bloomrow[i] = p;
    p += nrules;
  }
  kindlo = p;
  kindhi = p + nrules;
  p += 2 * nrules;
  n = *p++;
  for (i = 0; i < n; i++) {
    e = find_or_add(p + 1, p[0]);
    if (e == NONE) return 0;
    lflags[e] |= p[1 + p[0]];
    p += 2 + p[0];
  }
  nindex = *p++;
  if (nindex > MAXINDEX) return 0;
  for (i = 0; i < nindex; i++) {
    ixnode[i] = gbase + RD16(p);
    n = p[2];
    ixalts[i] = p + 3;
    p += 3 + 2 * n;
    ixnent[i] = RD16(p);
    ixent[i] = p + 2;
    p += 2 + 3 * ixnent[i];
    ixnwild[i] = *p;
    ixwild[i] = p + 1;
    p += 1 + 3 * ixnwild[i];
  }
  then_word = lookup("then", 4, hash_of("then", 4));
  return then_word < nwords;
}

/* The default aliases: each key's first word into the dictionary (its alias chain), and each verb's entry. */
static unsigned char read_aliases(const unsigned char *t)
{
  const unsigned char *p;
  unsigned char i, f;
  unsigned e;
  p = t;
  if (p[0] != 'D' || p[1] != 'Q' || p[2] != '0' || p[3] != 'A' || p[4] != 1) return 0;
  nalias = p[5];
  if (nalias > MAXALIAS) return 0;
  p += 6;
  for (i = 0; i < nalias; i++) {
    aliaswl[i] = p[0];
    aliasw[i] = p + 1;
    p += 1 + p[0];
    aliasvl[i] = p[0];
    aliasv[i] = p + 1;
    p += 1 + p[0];
    aliasany[i] = *p++;
    for (f = 0; f < aliaswl[i] && aliasw[i][f] != ' '; f++)
      ;
    aliasfl[i] = f;
    e = find_or_add(aliasw[i], f);
    if (e == NONE) return 0;
    anext[i] = lalias[e];
    lalias[e] = i + 1;
    aliasve[i] = lookup((const char *)aliasv[i], aliasvl[i], hash_of((const char *)aliasv[i], aliasvl[i]));
  }
  return 1;
}

int cmd_init(const unsigned char *grammar, const unsigned char *index, const unsigned char *aliases)
{
  unsigned char i;
  if (!read_grammar(grammar) || !read_index(index) || !read_aliases(aliases)) return 0;
  for (i = 0; i <= MAXTOK; i++) memorow[i] = memo + i * nrules;
  memset(memo, 0xFF, sizeof memo);
  nset = 0;
  nlex_fixed = nlex;
  return 1;
}

/* Mark the words of a space-separated list; a deity's name that is no word of the dictionary's is added for the while */
static void mark_list(const char *list, unsigned char flag)
{
  const char *w;
  unsigned char len;
  unsigned e;
  while (*list) {
    while (*list == ' ') list++;
    w = list;
    while (*list && *list != ' ') list++;
    len = list - w;
    if (!len) continue;
    e = flag == W_DEITY ? find_or_add((const unsigned char *)w, len) : lookup(w, len, hash_of(w, len));
    if (e != NONE) lmark[e] |= flag;
  }
}

/* The world's words: what it names things by, the names given, the deities. The world's own words added last time come
   out first: they were added after every other entry, so they are at the front of their chains. */
void cmd_world(const char *world, const char *names, const char *deities)
{
  unsigned char b;
  for (b = 0; b < NBUCKET; b++)
    while (lhead[b] != NONE && lhead[b] >= nlex_fixed) lhead[b] = lnext[lhead[b]];
  nlex = nlex_fixed;
  memset(lmark, 0, nlex_fixed);
  mark_list(world, W_WORLD);
  mark_list(names, W_NAME);
  mark_list(deities, W_DEITY);
}

/* Does the rest of an alias key (the words after its first) match the tokens from t on? The tokens the key takes, with
   its first word's (1 for a key of one word), or 0. */
static unsigned char key_rest(unsigned char t, const unsigned char *key, unsigned char klen)
{
  unsigned char pos, count, s0, wl;
  pos = 0;
  count = 1;
  while (pos < klen) {
    s0 = pos;
    while (pos < klen && key[pos] != ' ') pos++;
    wl = pos - s0;
    if (t >= ntok || tokq[t] || toklen[t] != wl || memcmp(tbuf + tokoff[t], key + s0, wl) != 0) return 0;
    t++;
    count++;
    if (pos < klen) pos++;
  }
  return count;
}

/* Remove count tokens from t on: the line is shorter. */
static void drop_tokens(unsigned char t, unsigned char count)
{
  unsigned char k;
  for (k = t; k + count < ntok; k++) {
    toke[k] = toke[k + count];
    tokw[k] = tokw[k + count];
    tokq[k] = tokq[k + count];
    tokoff[k] = tokoff[k + count];
    toklen[k] = toklen[k + count];
  }
  ntok -= count;
}

/* The aliases: a command's verb at the start of a command or of a part after "then"; a direction word anywhere. The
   longest key wins; a key may be several words ("look closely"). The token's entry heads its word's aliases. */
static void apply_aliases(void)
{
  unsigned char t, a, ai, n, bn, bi, vl, j, seg;
  for (t = 0; t < ntok; t++) {
    if (tokq[t] || toke[t] == NONE) continue;
    seg = t == 0 || tokw[t - 1] == then_word;
    bn = 0;
    bi = 0;
    for (ai = lalias[toke[t]]; ai; ai = anext[a]) {
      a = ai - 1;
      if (!seg && !aliasany[a]) continue;
      n = aliaswl[a] > aliasfl[a] ? key_rest(t + 1, aliasw[a] + aliasfl[a] + 1, aliaswl[a] - aliasfl[a] - 1) : 1;
      if (n > bn) {
        bn = n;
        bi = a;
      }
    }
    vl = aliasvl[bi];
    if (!bn || tbn + vl + 1 > sizeof tbuf) continue;
    tokoff[t] = tbn;
    toklen[t] = vl;
    for (j = 0; j < vl; j++) tbuf[tbn++] = aliasv[bi][j];
    tbuf[tbn++] = 0;
    toke[t] = aliasve[bi];
    tokw[t] = toke[t] < nwords ? toke[t] : NONE;
    drop_tokens(t + 1, bn - 1);
  }
}

static unsigned char all_digits(unsigned char off, unsigned char len)
{
  unsigned char i;
  if (!len) return 0;
  for (i = 0; i < len; i++)
    if (tbuf[off + i] < '0' || tbuf[off + i] > '9') return 0;
  return 1;
}

/* What open names the token fits (the host's openOK): from its lexicon flags, and the world's marks. */
static unsigned token_kinds(unsigned char t)
{
  unsigned m;
  unsigned char f, w;
  if (tokq[t]) return bitv[K_QUOTED];
  f = w = 0;
  if (toke[t] != NONE) {
    f = lflags[toke[t]];
    w = lmark[toke[t]];
  }
  m = bitv[K_TEXT];
  if (!(f & LX_NOTSPELL)) m |= bitv[K_SPELL];
  if (f & LX_ADJ) m |= bitv[K_ADJ];
  if (f & LX_TASK) m |= bitv[K_TASK];
  if (f & LX_ORDINAL) m |= bitv[K_ORDINAL];
  if ((f & LX_NUMBER) || all_digits(tokoff[t], toklen[t])) m |= bitv[K_NUMBER];
  if (tokw[t] == NONE) {             /* not a word of the grammar's: a name, a word to pretend */
    m |= bitv[K_PRETENSE] | bitv[K_NAMEWORD];
    if (!(f & LX_NOTOBJECT)) m |= bitv[K_OBJ];
  } else if (w & W_WORLD) {   /* a grammar word the world names a thing by */
    m |= bitv[K_NAMEWORD];
    if (!(f & LX_NOTOBJECT)) m |= bitv[K_OBJ];
  } else if (!(f & LX_NOTOBJECT) && (w & W_NAME)) m |= bitv[K_OBJ];
  if (w & W_DEITY) m |= bitv[K_DEITY];
  return m;
}

#define IS_WORD_CHAR(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || ((c) >= '0' && (c) <= '9'))

/* Tokenize a line: words (lower case, with ! and =), quoted text (with a w or y), nothing else; each word looked up
   as it is read. Then the aliases, then each token's open-name kinds and Bloom bit. */
static void tokenize(const char *s)
{
  unsigned char nb, t, start, len, h;
  unsigned m;
  char c;
  nb = 0;
  ntok = 0;
  too_long = 0;
  while (*s) {
    c = *s;
    if (c == '"' || ((c == 'w' || c == 'y') && s[1] == '"')) {
      if (ntok == MAXTOK) { too_long = 1; break; }
      if (c != '"') s++;
      s++;
      t = ntok++;
      tokq[t] = 1;
      toke[t] = tokw[t] = NONE;
      tokoff[t] = nb;
      while (*s && *s != '"' && nb < 180) tbuf[nb++] = *s++;
      if (*s == '"') s++;
      toklen[t] = nb - tokoff[t];
      tbuf[nb++] = 0;
      continue;
    }
    if (IS_WORD_CHAR(c)) {
      if (ntok == MAXTOK) { too_long = 1; break; }
      start = nb;
      h = 0;
      while (IS_WORD_CHAR(*s) || *s == '\'' || *s == '-') {
        c = *s++;
        if (c >= 'A' && c <= 'Z') c += 32;
        if (nb < 180) {
          tbuf[nb++] = c;
          h = pearson[h ^ (unsigned char)c];
        }
      }
    } else if (c == '!' || c == '=') {
      if (ntok == MAXTOK) { too_long = 1; break; }
      start = nb;
      tbuf[nb++] = c;
      h = pearson[(unsigned char)c];
      s++;
    } else {
      s++;
      continue;
    }
    len = nb - start;
    t = ntok++;
    tokq[t] = 0;
    tokoff[t] = start;
    toklen[t] = len;
    toke[t] = lookup(tbuf + start, len, pearson[h ^ len]);
    tokw[t] = toke[t] < nwords ? toke[t] : NONE;
    tbuf[nb++] = 0;
  }
  tbn = nb;
  has_then = 0;
  for (t = 0; t < ntok; t++)
    if (tokw[t] == then_word) has_then = 1;
  apply_aliases();
  for (t = 0; t < ntok; t++) {
    tokwl[t] = tokw[t];
    tokwh[t] = tokw[t] >> 8;
    m = token_kinds(t);
    tokmask[t] = m;
    tokmlo[t] = m;
    tokmhi[t] = m >> 8;
    if (tokw[t] != NONE) {
      tokbk[t] = (tokw[t] >> 3) & 7;
      tokbb[t] = bitb[tokw[t] & 7];
    } else {
      tokbk[t] = 0;
      tokbb[t] = 0;
    }
  }
}

#pragma static-locals (pop)

/* The positions a node can end at, from pos: a bit mask (bit p). */
static unsigned ends(const unsigned char *p, unsigned char pos);

/* An indexed alternation: only the options the token's word begins (a binary search of the sorted first words), and
   those that begin with an open name the token fits, or can match nothing. */
static unsigned alt_indexed(unsigned char i, unsigned char pos)
{
  const unsigned char *e, *alts = ixalts[i];
  unsigned out = 0, w, lo, hi, mid, n;
  if (pos < ntok && (w = tokw[pos]) != NONE) {
    e = ixent[i];
    n = ixnent[i];
    lo = 0;
    hi = n;
    while (lo < hi) {
      mid = (lo + hi) >> 1;
      if (RD16(e + 3 * mid) < w) lo = mid + 1;
      else hi = mid;
    }
    for (e += 3 * lo; lo < n && RD16(e) == w; lo++, e += 3)
      out |= ends(gbase + RD16(alts + 2 * e[2]), pos);
  }
  e = ixwild[i];
  for (n = ixnwild[i]; n; n--, e += 3)
    if ((e[2] & 0x80) || (pos < ntok && ((tokmlo[pos] & e[1]) || (tokmhi[pos] & e[2]))))
      out |= ends(gbase + RD16(alts + 2 * e[0]), pos);
  return out;
}

/* The ends of a node from pos. A rule is memoised per position, and entered only if the token can begin it (or it can
   match nothing); one entered again at the same position (left recursion) gives nothing the second time. In a
   sequence, a word or an open name is checked where it stands, without a call. */
static unsigned ends(const unsigned char *p, unsigned char pos)
{
  const unsigned char *s;
  unsigned mm, *cell;
  unsigned m, out;
  unsigned char n, q, k;
  switch (*p) {
    case T_TERM:
      n = p[1];
      if (pos + n > ntok) return 0;
      for (s = p + 2; n; n--, s += 2, pos++)
        if (tokwl[pos] != s[0] || tokwh[pos] != s[1]) return 0;
      return bitv[pos];
    case T_QUOTE:
      return (pos < ntok && tokq[pos]) ? bitv[pos + 1] : 0;
    case T_NT:
      n = p[1];
      if (n < nrules && n != CMD_QUOTED) {
        cell = memorow[pos] + n;
        if (*cell != NONE) return *cell;
        *cell = 0;
        if (nset < MAXSET) memoset[nset] = cell;
        nset++;
        if (n == CMD_SEQUENCE && !has_then) return 0;   /* a sequence needs a then */
        if (!(kindhi[n] & 0x80)) {
          if (pos >= ntok) return 0;
          if (!(bloomrow[tokbk[pos]][n] & tokbb[pos]) && !(tokmlo[pos] & kindlo[n]) && !(tokmhi[pos] & kindhi[n])) return 0;
        }
        return *cell = ends(rulep[n], pos);
      }
      if (pos >= ntok) return 0;
      return (tokmask[pos] & bitv[n == CMD_QUOTED ? K_QUOTED : kindp[n - nrules]]) ? bitv[pos + 1] : 0;
    case T_SEQ:
      /* the positions so far, passed through each item in turn */
      s = HEAD(p);
      n = *s++;
      m = bitv[pos];
      while (n--) {
        out = 0;
        if (*s == T_TERM && s[1] == 1) {        /* a word */
          for (q = pos, mm = m >> pos; mm && q < ntok; q++, mm >>= 1)
            if ((mm & 1) && tokwl[q] == s[2] && tokwh[q] == s[3]) out |= bitv[q + 1];
        } else if (*s == T_NT && s[1] >= nrules) {   /* an open name */
          k = kindp[s[1] - nrules];
          for (q = pos, mm = m >> pos; mm && q < ntok; q++, mm >>= 1)
            if ((mm & 1) && (tokmask[q] & bitv[k])) out |= bitv[q + 1];
        } else
          for (q = pos, mm = m >> pos; mm; q++, mm >>= 1)
            if (mm & 1) out |= ends(s, q);
        if (!out) return 0;
        m = out;
        while (!(m & bitv[pos])) pos++;   /* where the next item can start, at the soonest */
        s = node_next(s);
      }
      return m;
    case T_ALT:
      s = HEAD(p);
      n = *s++;
      if (n >= 8)   /* (commandtables.js's ALT_INDEX_MIN: only so large an alternation can be indexed) */
        for (q = 0; q < nindex; q++)
          if (ixnode[q] == p) return alt_indexed(q, pos);
      out = 0;
      while (n--) {
        out |= ends(s, pos);
        s = node_next(s);
      }
      return out;
    case T_OPT:
      return bitv[pos] | ends(HEAD(p), pos);
    case T_REP: {
      /* zero or more: the positions reached by repeating the child, each repetition moving on */
      unsigned frontier = bitv[pos], seen = frontier;
      s = HEAD(p);
      for (n = 0; n < MAXREP && frontier; n++) {
        out = 0;
        for (q = pos, mm = frontier >> pos; mm; q++, mm >>= 1)
          if (mm & 1) out |= ends(s, q);
        out &= ~seen;
        seen |= out;
        frontier = out;
      }
      return seen;
    }
    default:
      return 0;
  }
}

/* Match a line (in the world cmd_world last set): 1 if it parses, 0 if not, -1 if it is too long for the matcher. */
int cmd_match(const char *line)
{
  unsigned m;
  tokenize(line);
  if (too_long) return -1;
  m = ends(line_ref, 0);
  if (nset > MAXSET) memset(memo, 0xFF, sizeof memo);   /* the memo ready for the next line */
  else
    while (nset) *memoset[--nset] = NONE;
  nset = 0;
  return (m & bitv[ntok]) ? 1 : 0;
}
