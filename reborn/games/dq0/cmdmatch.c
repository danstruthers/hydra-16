/* cmdmatch.c - the command grammar's matcher, for the 65C02 (cc65). Host reference: sim/tools/commandcompile.js; the
   test data and expected results: sim/tools/commandmatch-gen.js; the tables: sim/tools/commandtables.js.

   The matcher returns, for a rule (or a node) at a position, the set of positions where it can end, as a bit mask
   (bit p: the match ends before token p). Nothing is pushed on the stack for a step, so the stack grows with the
   grammar's nesting, not with the length of a parse. Rules are memoised per position (memo[rule][pos]), and a rule the token cannot begin
   is skipped: each rule's first words are worked out once, when first needed (firstbits: about 10 KB of RAM).
   Together with the memo that is about 16 KB of RAM for the full grammar. A line parses when the rule "line" can end after its last token. The grammar is unambiguous on the
   corpus, so no parse is ranked; the first answer is the answer. */
#include <string.h>
#include "cmdmatch.h"

/* Node tags in the grammar table */
#define T_SEQ 1
#define T_ALT 2
#define T_OPT 3
#define T_REP 4
#define T_NT 5
#define T_TERM 6
#define T_QUOTE 7

/* Open-name kinds: what each open name accepts (the host's openOK) */
#define K_NONE (-1)
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

#define MAXRULES 240
#define MAXNAMES 260
#define MAXWORDS 400
#define MAXALIAS 80
#define MAXTOK 14        /* positions 0..14 fit a 16-bit mask; the corpus is at most 12 words */
#define MAXREP 12
#define UNKNOWN 0xFFFF   /* a memo entry not yet computed */

static const unsigned char *rulep[MAXRULES];
static const unsigned char *namep[MAXNAMES];
static unsigned char namelen[MAXNAMES];
static int namekind[MAXNAMES];
static const unsigned char *wordp[MAXWORDS];   /* the packed letters of each word */
static unsigned char wordlen[MAXWORDS];
static int nrules, nnames, nwords, rule_line, rule_adjunct, rule_sequence;
static unsigned int memo[MAXRULES][MAXTOK + 1];
/* The first words of each rule: the words it can begin with (a bit each), its open-name kinds, quoted text, and
   whether it can match nothing. Built from the grammar at start-up; rule_ends skips a rule the token cannot begin. */
static unsigned char firstbits[MAXRULES][(MAXWORDS + 7) / 8];
static unsigned int firstK[MAXRULES];
static unsigned char firstQ[MAXRULES], nullable[MAXRULES];
static int fchanged;
static unsigned char fstate[MAXRULES];   /* 0: not yet, 1: being worked out, 2: done */
static void ensure_first(unsigned r);


static const unsigned char *aliasw[MAXALIAS], *aliasv[MAXALIAS];
static unsigned char aliaswl[MAXALIAS], aliasvl[MAXALIAS], aliasany[MAXALIAS];
static int nalias;

static char tbuf[200];
static int tbn;
static int tokk[MAXTOK + 1], tokw[MAXTOK + 1], tokoff[MAXTOK + 1], toklen[MAXTOK + 1];
static unsigned int tokmask[MAXTOK + 1];   /* bit k: the token fits the open-name kind k */
static int ntok, has_then;
static const char *cur_world, *cur_names, *cur_deities;

/* The letters of a packed word: 5 bits a letter (a to z, then ! = - ') */
static const char PACK[] = "abcdefghijklmnopqrstuvwxyz!=-'";

static int letter_at(const unsigned char *packed, int nbytes, int j)
{
  int bit = 5 * j, b = bit >> 3, sh = bit & 7;
  unsigned int window = packed[b];
  if (b + 1 < nbytes) window |= (unsigned int)packed[b + 1] << 8;
  return PACK[(window >> sh) & 31];
}

/* Does word w spell the len letters of text (the text is lower case)? */
static int word_is(int w, const char *text, int len)
{
  int j, nb = (5 * wordlen[w] + 7) >> 3;
  if (wordlen[w] != len) return 0;
  for (j = 0; j < len; j++)
    if (letter_at(wordp[w], nb, j) != text[j]) return 0;
  return 1;
}

static int same(const unsigned char *a, int alen, const char *b)
{
  int n = strlen(b);
  return alen == n && memcmp(a, b, n) == 0;
}

static unsigned rd16(const unsigned char *p)
{
  return p[0] | ((unsigned)p[1] << 8);
}

/* A slot is a child: its size (one byte under 128; two bytes above, the first with its top bit set), then the child.
   The size lets a reader step over a child without walking it. */
static const unsigned char *slot_body(const unsigned char *s)
{
  return (s[0] & 0x80) ? s + 2 : s + 1;
}

static const unsigned char *slot_next(const unsigned char *s)
{
  unsigned size = (s[0] & 0x80) ? (((unsigned)(s[0] & 0x7F) << 8) | s[1]) : s[0];
  return slot_body(s) + size;
}

/* The first words of a node, into rule r. Returns 1 if the node can match nothing. */
static int first_node(const unsigned char *p, int r)
{
  unsigned i, w, bit, nk, id;
  int nul;
  const unsigned char *s;
  switch (*p) {
    case T_TERM:
      w = rd16(p + 2);
      bit = 1u << (w & 7);
      if (!(firstbits[r][w >> 3] & bit)) { firstbits[r][w >> 3] |= bit; fchanged = 1; }
      return 0;
    case T_QUOTE:
      if (!firstQ[r]) { firstQ[r] = 1; fchanged = 1; }
      return 0;
    case T_NT:
      id = rd16(p + 1);
      if (namekind[id] != K_NONE) {
        nk = 1u << namekind[id];
        if (!(firstK[r] & nk)) { firstK[r] |= nk; fchanged = 1; }
        return 0;
      }
      ensure_first(id);
      for (i = 0; i < sizeof firstbits[r]; i++) {
        unsigned char merged = firstbits[r][i] | firstbits[id][i];
        if (merged != firstbits[r][i]) { firstbits[r][i] = merged; fchanged = 1; }
      }
      if ((firstK[r] | firstK[id]) != firstK[r]) { firstK[r] |= firstK[id]; fchanged = 1; }
      if (firstQ[id] && !firstQ[r]) { firstQ[r] = 1; fchanged = 1; }
      return nullable[id];
    case T_SEQ:
      s = p + 2;
      for (i = 0; i < p[1]; i++) {
        nul = first_node(slot_body(s), r);
        if (!nul) return 0;
        s = slot_next(s);
      }
      return 1;
    case T_ALT:
      nul = 0;
      s = p + 2;
      for (i = 0; i < p[1]; i++) {
        if (first_node(slot_body(s), r)) nul = 1;
        s = slot_next(s);
      }
      return nul;
    case T_OPT:
    case T_REP:
      first_node(slot_body(p + 1), r);
      return 1;
    default:
      return 0;
  }
}

/* The first words of a rule, worked out the first time it is needed (a rule that refers to itself gets what it has so
   far, which is enough for this grammar: no rule starts with itself). */
static void ensure_first(unsigned r)
{
  int nul;
  if (fstate[r]) return;
  fstate[r] = 1;
  memset(firstbits[r], 0, sizeof firstbits[r]);
  firstK[r] = 0;
  firstQ[r] = 0;
  nul = first_node(rulep[r], r);
  nullable[r] = nul ? 1 : 0;
  fstate[r] = 2;
}

/* Read the grammar table: the names (rules first, then open names), the packed words, and the rule bodies. */
int cmd_init(const unsigned char *t)
{
  const unsigned char *p = t;
  unsigned i, n;
  if (p[0] != 'D' || p[1] != 'Q' || p[2] != '0' || p[3] != 'G' || p[4] != 6) return 0;
  p += 5;
  nrules = rd16(p); p += 2;
  nnames = rd16(p); p += 2;
  for (i = 0; i < nnames; i++) {
    namelen[i] = p[0];
    namep[i] = p + 1;
    p += 1 + p[0];
  }
  nwords = rd16(p); p += 2;
  for (i = 0; i < nwords; i++) {
    wordlen[i] = p[0];
    wordp[i] = p + 1;
    p += 1 + ((5 * p[0] + 7) >> 3);
  }
  {
    const unsigned char *offs = p;
    const unsigned char *rules = p + 2 * nrules;
    for (i = 0; i < nrules; i++) rulep[i] = rules + rd16(offs + 2 * i);
  }
  /* The open names and their kinds; quoted-text is a rule in the grammar but an open name to the host */
  for (i = 0; i < nnames; i++) {
    n = namelen[i];
    namekind[i] = K_NONE;
    if (i < (unsigned)nrules && !same(namep[i], n, "quoted-text")) continue;
    if (same(namep[i], n, "name")) namekind[i] = K_OBJ;
    else if (same(namep[i], n, "alias")) namekind[i] = K_OBJ;
    else if (same(namep[i], n, "adj")) namekind[i] = K_ADJ;
    else if (same(namep[i], n, "number")) namekind[i] = K_NUMBER;
    else if (same(namep[i], n, "ordinal")) namekind[i] = K_ORDINAL;
    else if (same(namep[i], n, "task-name")) namekind[i] = K_TASK;
    else if (same(namep[i], n, "spell-word")) namekind[i] = K_SPELL;
    else if (same(namep[i], n, "text") || same(namep[i], n, "meta-topic")) namekind[i] = K_TEXT;
    else if (same(namep[i], n, "name-word")) namekind[i] = K_NAMEWORD;
    else if (same(namep[i], n, "deity-name")) namekind[i] = K_DEITY;
    else if (same(namep[i], n, "pretense-word")) namekind[i] = K_PRETENSE;
    else if (same(namep[i], n, "quoted-text")) namekind[i] = K_QUOTED;
  }
  for (i = 0; i < (unsigned)nrules; i++) {
    if (same(namep[i], namelen[i], "line")) rule_line = i;
    if (same(namep[i], namelen[i], "adjunct")) rule_adjunct = i;
    if (same(namep[i], namelen[i], "sequence")) rule_sequence = i;
  }
  return 1;
}

/* Read the default aliases (the alias table, commandtables.js): the word, the verb, and whether it stands anywhere. */
int cmd_init_aliases(const unsigned char *t)
{
  const unsigned char *p = t;
  int i;
  if (p[0] != 'D' || p[1] != 'Q' || p[2] != '0' || p[3] != 'A' || p[4] != 1) return 0;
  nalias = p[5];
  p += 6;
  for (i = 0; i < nalias; i++) {
    aliaswl[i] = p[0];
    aliasw[i] = p + 1;
    p += 1 + p[0];
    aliasvl[i] = p[0];
    aliasv[i] = p + 1;
    p += 1 + p[0];
    aliasany[i] = *p;
    p++;
  }
  return 1;
}

static int word_index(const char *t, int len)
{
  int w;
  for (w = 0; w < nwords; w++)
    if (word_is(w, t, len)) return w;
  return -1;
}

/* Is the word (tbuf[off..off+len)) one of the space-separated words in list? */
static int in_list(const char *list, int off, int len)
{
  const char *p = list;
  while (*p) {
    const char *s = p;
    while (*p && *p != ' ') p++;
    if (p - s == len && memcmp(s, tbuf + off, len) == 0) return 1;
    while (*p == ' ') p++;
  }
  return 0;
}

static int in_words(int off, int len, const char *const *words, int n)
{
  int i;
  for (i = 0; i < n; i++)
    if ((int)strlen(words[i]) == len && memcmp(words[i], tbuf + off, len) == 0) return 1;
  return 0;
}

static const char *const ADJ_WORDS[] = {"right", "far", "wooden", "iron", "large", "small", "closed", "open", "old",
  "rusty", "bottom", "top", "heavy", "near", "left"};
static const char *const TASK_WORDS[] = {"digging", "crawling", "waiting", "searching", "climbing", "sleeping",
  "resting", "working", "pretending"};
static const char *const SPELL_STOP_WORDS[] = {"at", "toward", "towards", "to", "spell", "the", "a", "an", "my", "this",
  "that", "these", "those"};
static const char *const NOT_OBJECT_WORDS[] = {"you", "character", "yourself"};
static const char *const ORDINAL_WORDS[] = {"first", "second", "third", "fourth", "fifth"};
static const char *const NUM_WORDS[] = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten"};

static int is_number(int off, int len)
{
  int i;
  if (len == 0) return 0;
  for (i = 0; i < len; i++)
    if (tbuf[off + i] < '0' || tbuf[off + i] > '9') break;
  if (i == len) return 1;
  return in_words(off, len, NUM_WORDS, 10);
}

/* Does token t fit the open-name kind k? (the host's openOK) */
static int open_ok(int kind, int t)
{
  int off = tokoff[t], len = toklen[t];
  int reserved = tokw[t] >= 0;
  if (kind == K_QUOTED) return tokk[t] == 1;
  if (tokk[t] != 0) return 0;
  switch (kind) {
    case K_NUMBER: return is_number(off, len);
    case K_ORDINAL: return in_words(off, len, ORDINAL_WORDS, 5);
    case K_ADJ: return in_words(off, len, ADJ_WORDS, 15);
    case K_TASK: return in_words(off, len, TASK_WORDS, 9);
    case K_SPELL: return !in_words(off, len, SPELL_STOP_WORDS, 13);
    case K_TEXT: return 1;
    case K_NAMEWORD: return !reserved || in_list(cur_world, off, len);
    case K_DEITY: return in_list(cur_deities, off, len);
    case K_PRETENSE: return !reserved;
    default:
      return !in_words(off, len, NOT_OBJECT_WORDS, 3) &&
             (!reserved || in_list(cur_world, off, len) || in_list(cur_names, off, len));
  }
}

/* Does the alias key (words separated by single spaces) match the tokens from t on? n: the tokens it takes. */
static int key_match(int t, const unsigned char *key, int klen, int *n)
{
  int pos = 0, count = 0, j = t, s0, wl;
  while (pos < klen) {
    s0 = pos;
    while (pos < klen && key[pos] != 32) pos++;
    wl = pos - s0;
    if (j >= ntok || tokk[j] != 0 || toklen[j] != wl || memcmp(tbuf + tokoff[j], key + s0, wl) != 0) return 0;
    j++;
    count++;
    if (pos < klen) pos++;
  }
  *n = count;
  return count > 0;
}

/* Remove count tokens from t on: the line is shorter. */
static void drop_tokens(int t, int count)
{
  int k;
  for (k = t; k + count < ntok; k++) {
    tokk[k] = tokk[k + count];
    tokw[k] = tokw[k + count];
    tokoff[k] = tokoff[k + count];
    toklen[k] = toklen[k + count];
  }
  ntok -= count;
}

/* The aliases: a command's verb at the start of a command or of a part after "then"; a direction word anywhere. The
   longest key wins; a key may be several words ("look closely"). */
static void apply_aliases(void)
{
  int t, i, n, bi, bn, vl, j;
  for (t = 0; t < ntok; t++) {
    int seg = (t == 0) || (tokk[t - 1] == 0 && toklen[t - 1] == 4 && memcmp(tbuf + tokoff[t - 1], "then", 4) == 0);
    if (tokk[t] != 0) continue;
    bi = -1;
    bn = 0;
    for (i = 0; i < nalias; i++) {
      if (!seg && !aliasany[i]) continue;
      if (key_match(t, aliasw[i], aliaswl[i], &n) && n > bn) { bi = i; bn = n; }
    }
    if (bi < 0) continue;
    vl = aliasvl[bi];
    tokoff[t] = tbn;
    toklen[t] = vl;
    for (j = 0; j < vl; j++) tbuf[tbn++] = aliasv[bi][j];
    tbuf[tbn++] = 0;
    tokw[t] = word_index(tbuf + tokoff[t], vl);
    drop_tokens(t + 1, bn - 1);
  }
}

/* Tokenize a line: words (lower case, with ! and =), quoted text (with a w or y), nothing else; then the aliases, then
   the open-name kinds of each token. */
static void tokenize(const char *s)
{
  int nb = 0, t, w;
  ntok = 0;
  while (*s && ntok < MAXTOK) {
    char c = *s;
    int start, len;
    if (c == ' ') { s++; continue; }
    if (c == '"' || ((c == 'w' || c == 'y') && s[1] == '"')) {
      if (c != '"') s++;
      s++;
      t = ntok++;
      tokk[t] = 1;
      tokw[t] = -1;
      tokoff[t] = nb;
      while (*s && *s != '"' && nb < 180) tbuf[nb++] = *s++;
      if (*s == '"') s++;
      toklen[t] = nb - tokoff[t];
      tbuf[nb++] = 0;
      continue;
    }
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      start = nb;
      while ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '\'' || *s == '-') {
        if (nb < 180) tbuf[nb++] = (*s >= 'A' && *s <= 'Z') ? *s + 32 : *s;
        s++;
      }
    } else if (c == '!' || c == '=') {
      start = nb;
      tbuf[nb++] = c;
      s++;
    } else {
      s++;
      continue;
    }
    len = nb - start;
    t = ntok++;
    tokk[t] = 0;
    tokoff[t] = start;
    toklen[t] = len;
    tokw[t] = word_index(tbuf + start, len);
    tbuf[nb++] = 0;
  }
  tbn = nb;
  has_then = 0;
  for (t = 0; t < ntok; t++)
    if (tokk[t] == 0 && toklen[t] == 4 && memcmp(tbuf + tokoff[t], "then", 4) == 0) has_then = 1;
  apply_aliases();
  for (t = 0; t < ntok; t++) {
    int k;
    tokmask[t] = 0;
    for (k = 0; k <= K_QUOTED; k++)
      if (open_ok(k, t)) tokmask[t] |= 1u << k;
  }
}

/* The positions a node can end at, from pos: a bit mask (bit p). */
static unsigned ends(const unsigned char *p, int pos);

/* Can the rule begin with the token at pos (or match nothing)? */
static int can_start(unsigned id, int pos)
{
  int w;
  ensure_first(id);
  if (nullable[id]) return 1;
  if (pos >= ntok) return 0;
  if (tokk[pos] == 1) return firstQ[id] || ((tokmask[pos] & firstK[id]) != 0);   /* quoted text is an open name too */
  w = tokw[pos];
  if (w >= 0 && (firstbits[id][w >> 3] >> (w & 7)) & 1) return 1;
  return (tokmask[pos] & firstK[id]) != 0;
}

/* A rule's ends from pos, memoised. A rule that is entered again at the same position (left recursion) gives nothing
   the second time, so the search always finishes. */
static unsigned rule_ends(int id, int pos)
{
  unsigned m;
  if (memo[id][pos] != UNKNOWN) return memo[id][pos];
  if ((int)id == rule_sequence && !has_then) return memo[id][pos] = 0;   /* a sequence needs a then */
  if (!can_start(id, pos)) return memo[id][pos] = 0;
  memo[id][pos] = 0;
  m = ends(rulep[id], pos);
  memo[id][pos] = m;
  return m;
}

static unsigned ends(const unsigned char *p, int pos)
{
  unsigned m = 0, out = 0;
  int i, q, n;
  const unsigned char *s;
  switch (*p) {
    case T_TERM:
      n = p[1];
      if (pos + n > ntok) return 0;
      for (i = 0; i < n; i++)
        if (tokk[pos + i] != 0 || tokw[pos + i] != (int)rd16(p + 2 + 2 * i)) return 0;
      return 1u << (pos + n);
    case T_QUOTE:
      return (pos < ntok && tokk[pos] == 1) ? 1u << (pos + 1) : 0;
    case T_NT: {
      unsigned id = rd16(p + 1);
      if (namekind[id] == K_NONE) return rule_ends(id, pos);
      return (pos < ntok && ((tokmask[pos] >> namekind[id]) & 1u)) ? 1u << (pos + 1) : 0;
    }
    case T_SEQ:
      /* the positions so far, passed through each item in turn */
      m = 1u << pos;
      s = p + 2;
      for (i = 0; i < p[1] && m; i++) {
        out = 0;
        for (q = 0; q <= ntok; q++)
          if (m & (1u << q)) out |= ends(slot_body(s), q);
        m = out;
        s = slot_next(s);
      }
      return m;
    case T_ALT:
      s = p + 2;
      for (i = 0; i < p[1]; i++) {
        out |= ends(slot_body(s), pos);
        s = slot_next(s);
      }
      return out;
    case T_OPT:
      return (1u << pos) | ends(slot_body(p + 1), pos);
    case T_REP: {
      /* zero or more: the positions reached by repeating the child, each repetition moving on */
      unsigned frontier = 1u << pos, seen = 1u << pos, nxt;
      for (n = 0; n < MAXREP && frontier; n++) {
        nxt = 0;
        for (q = 0; q <= ntok; q++)
          if (frontier & (1u << q)) nxt |= ends(slot_body(p + 1), q);
        nxt &= ~seen;
        seen |= nxt;
        frontier = nxt;
      }
      return seen;
    }
    default:
      return 0;
  }
}

/* Match a line: 1 if it parses, 0 if not, -1 if it is too long for the matcher. *fewest is always 0 now (no adjuncts). */
int cmd_match(const char *line, const char *world, const char *names, const char *deities, int *fewest)
{
  unsigned m;
  int r, i;
  cur_world = world;
  cur_names = names;
  cur_deities = deities;
  tokenize(line);
  if (ntok > MAXTOK) return -1;
  for (r = 0; r < nrules; r++)
    for (i = 0; i <= MAXTOK; i++) memo[r][i] = UNKNOWN;
  m = rule_ends(rule_line, 0);
  *fewest = 0;
  return (m >> ntok) & 1u ? 1 : 0;
}
