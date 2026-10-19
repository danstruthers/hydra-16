/* cmdmatch.c - the command grammar's matcher, for the 65C02 (cc65). See cmdmatch.h. Host reference: sim/tools/
   commandcompile.js; the test data and the expected results: sim/tools/commandmatch-gen.js. */
#include <string.h>
#include "cmdmatch.h"

/* Node tags in the table (commandtables.js) */
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
#define MAXTOK 24
#define MAXREP 12
#define STEP_LIMIT 30000

/* Continuations: what remains to be matched after the node in hand */
#define C_END 0
#define C_RULE 1
#define C_SEQ 2
#define C_REP 3

typedef struct Cont Cont;
struct Cont {
  int kind;
  const unsigned char *node;
  int count;
  int start;
  Cont *next;
};

static const unsigned char *rulep[MAXRULES];
static const unsigned char *namep[MAXNAMES];
static unsigned char namelen[MAXNAMES];
static int namekind[MAXNAMES];
static const unsigned char *wordp[MAXWORDS];
static unsigned char wordlen[MAXWORDS];
static int nrules, nnames, nwords, rule_line, rule_adjunct;

static char tbuf[120];
static int tokk[MAXTOK], tokw[MAXTOK], tokoff[MAXTOK], toklen[MAXTOK];
static unsigned tokmask[MAXTOK];   /* bit k set: the token fits the open-name kind k (computed once a line) */
static int ntok;
static const char *cur_world, *cur_names, *cur_deities;
static int adjcount, best, steps, aborted;

static int same(const unsigned char *a, int alen, const char *b)
{
  int n = strlen(b);
  return alen == n && memcmp(a, b, n) == 0;
}

/* The word-list test: is the word (tbuf[off..off+len)) one of the space-separated words in list? */
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

/* Does the token t fit the open name of this kind? (the host's openOK) */
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
    default: /* an object's name: a thing the world names, or a word no grammar rule uses */
      return !in_words(off, len, NOT_OBJECT_WORDS, 3) &&
             (!reserved || in_list(cur_world, off, len) || in_list(cur_names, off, len));
  }
}

/* The table's rule nodes: skip one node, to the byte after it */
static const unsigned char *skip(const unsigned char *p)
{
  unsigned n, i;
  switch (*p) {
    case T_SEQ:
    case T_ALT:
      n = p[1];
      p += 2;
      for (i = 0; i < n; i++) p = skip(p);
      return p;
    case T_OPT:
    case T_REP:
      return skip(p + 1);
    case T_NT:
      return p + 3;
    case T_TERM:
      return p + 2 + 2 * p[1];
    default:
      return p + 1;
  }
}

static unsigned rd16(const unsigned char *p)
{
  return p[0] | ((unsigned)p[1] << 8);
}

/* Read the table: the names (rules first, then open names), the words, and the rule nodes. */
int cmd_init(const unsigned char *t)
{
  const unsigned char *p = t;
  unsigned i, n;
  if (p[0] != 'D' || p[1] != 'Q' || p[2] != '0' || p[3] != 'G' || p[4] != 1) return 0;
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
    p += 1 + p[0];
  }
  {
    const unsigned char *offs = p;
    const unsigned char *rules = p + 2 * nrules;
    for (i = 0; i < nrules; i++) rulep[i] = rules + rd16(offs + 2 * i);
  }
  /* The open names and their kinds; quoted-text is a rule in the grammar but an open name to the host */
  for (i = 0; i < nnames; i++) {
    int k = K_NONE;
    n = namelen[i];
    if (i < nrules && !same(namep[i], n, "quoted-text")) { namekind[i] = K_NONE; continue; }
    if (same(namep[i], n, "unique-name") || same(namep[i], n, "kind-name") || same(namep[i], n, "fixture-name") ||
        same(namep[i], n, "place-name") || same(namep[i], n, "known-alias") || same(namep[i], n, "alias")) k = K_OBJ;
    else if (same(namep[i], n, "adj")) k = K_ADJ;
    else if (same(namep[i], n, "number")) k = K_NUMBER;
    else if (same(namep[i], n, "ordinal")) k = K_ORDINAL;
    else if (same(namep[i], n, "task-name")) k = K_TASK;
    else if (same(namep[i], n, "spell-word")) k = K_SPELL;
    else if (same(namep[i], n, "text") || same(namep[i], n, "meta-topic")) k = K_TEXT;
    else if (same(namep[i], n, "name-word")) k = K_NAMEWORD;
    else if (same(namep[i], n, "deity-name")) k = K_DEITY;
    else if (same(namep[i], n, "pretense-word")) k = K_PRETENSE;
    else if (same(namep[i], n, "quoted-text")) k = K_QUOTED;
    namekind[i] = k;
  }
  for (i = 0; i < nrules; i++) {
    if (same(namep[i], namelen[i], "line")) rule_line = i;
    if (same(namep[i], namelen[i], "adjunct")) rule_adjunct = i;
  }
  return 1;
}

/* Tokenize a line: words (lower case, with ! and =), quoted text (with a w or y prefix), and nothing else. */
static void tokenize(const char *s)
{
  int nb = 0, t;
  ntok = 0;
  while (*s && ntok < MAXTOK) {
    int vol = 0, start, len;
    char c = *s;
    if (c == ' ') { s++; continue; }
    if (c == '"' || ((c == 'w' || c == 'y') && s[1] == '"')) {
      if (c != '"') { vol = c; s++; }
      s++;
      t = ntok++;
      tokk[t] = 1;
      tokw[t] = -1;
      tokoff[t] = nb;
      while (*s && *s != '"' && nb < 110) tbuf[nb++] = *s++;
      if (*s == '"') s++;
      toklen[t] = nb - tokoff[t];
      (void)vol;
      tbuf[nb++] = 0;
      continue;
    }
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
      start = nb;
      while ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '\'' || *s == '-') {
        if (nb < 110) tbuf[nb++] = (*s >= 'A' && *s <= 'Z') ? *s + 32 : *s;
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
    tokw[t] = -1;
    {
      int w;
      for (w = 0; w < nwords; w++)
        if (wordlen[w] == len && memcmp(wordp[w], tbuf + start, len) == 0) { tokw[t] = w; break; }
    }
    tbuf[nb++] = 0;
  }
  for (t = 0; t < ntok; t++) {
    int k;
    tokmask[t] = 0;
    for (k = 0; k <= K_QUOTED; k++)
      if (open_ok(k, t)) tokmask[t] |= 1u << k;
  }
}

/* The matcher. match(node, pos, k): the node matched at pos, then k. run(k, pos): k, once something has matched to pos.
   Both return 1 only to stop the whole search (the step limit); a parse is recorded at the end and the search goes on. */
static int run(Cont *k, int pos);

static int match(const unsigned char *p, int pos, Cont *k)
{
  if (++steps > STEP_LIMIT) { aborted = 1; return 1; }
  switch (*p) {
    case T_TERM: {
      unsigned n = p[1], i;
      if (pos + (int)n > ntok) return 0;
      for (i = 0; i < n; i++) {
        int t = pos + i;
        if (tokk[t] != 0 || tokw[t] != (int)rd16(p + 2 + 2 * i)) return 0;
      }
      return run(k, pos + n);
    }
    case T_QUOTE:
      return (pos < ntok && tokk[pos] == 1) ? run(k, pos + 1) : 0;
    case T_NT: {
      unsigned id = rd16(p + 1);
      if (namekind[id] == K_NONE) {
        Cont c;
        int adj = (int)id == rule_adjunct, r;
        c.kind = C_RULE;
        c.node = 0;
        c.count = 0;
        c.start = 0;
        c.next = k;
        if (adj) adjcount++;
        r = match(rulep[id], pos, &c);
        if (adj) adjcount--;
        return r;
      }
      return (pos < ntok && ((tokmask[pos] >> namekind[id]) & 1u)) ? run(k, pos + 1) : 0;
    }
    case T_SEQ: {
      Cont c;
      if (p[1] == 0) return run(k, pos);
      c.kind = C_SEQ;
      c.node = p + 2;
      c.count = p[1];
      c.start = 0;
      c.next = k;
      return run(&c, pos);
    }
    case T_ALT: {
      unsigned n = p[1], i;
      const unsigned char *o = p + 2;
      for (i = 0; i < n; i++) {
        if (match(o, pos, k)) return 1;
        o = skip(o);
      }
      return 0;
    }
    case T_OPT:
      if (run(k, pos)) return 1;
      return match(p + 1, pos, k);
    case T_REP: {
      Cont c;
      if (run(k, pos)) return 1;
      c.kind = C_REP;
      c.node = p + 1;
      c.count = 1;
      c.start = pos;
      c.next = k;
      return match(p + 1, pos, &c);
    }
    default:
      return 0;
  }
}

static int run(Cont *k, int pos)
{
  switch (k->kind) {
    case C_END:
      if (pos == ntok) {
        if (best < 0 || adjcount < best) best = adjcount;
      }
      return 0;
    case C_RULE:
      return run(k->next, pos);
    case C_SEQ: {
      Cont c;
      if (k->count == 0) return run(k->next, pos);
      c.kind = C_SEQ;
      c.node = skip(k->node);
      c.count = k->count - 1;
      c.start = 0;
      c.next = k->next;
      return match(k->node, pos, &c);
    }
    case C_REP: {
      Cont c;
      if (pos == k->start) return 0;          /* an iteration that took nothing: no more */
      if (run(k->next, pos)) return 1;
      if (k->count >= MAXREP) return 0;
      c.kind = C_REP;
      c.node = k->node;
      c.count = k->count + 1;
      c.start = pos;
      c.next = k->next;
      return match(k->node, pos, &c);
    }
    default:
      return 0;
  }
}

int cmd_match(const char *line, const char *world, const char *names, const char *deities, int *fewest)
{
  Cont end;
  cur_world = world;
  cur_names = names;
  cur_deities = deities;
  tokenize(line);
  best = -1;
  adjcount = 0;
  steps = 0;
  aborted = 0;
  end.kind = C_END;
  end.node = 0;
  end.count = 0;
  end.start = 0;
  end.next = 0;
  match(rulep[rule_line], 0, &end);
  if (aborted) return -1;
  if (best < 0) return 0;
  *fewest = best;
  return 1;
}
