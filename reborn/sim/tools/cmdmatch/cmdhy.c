/* cmdhy.c - Dingus Quest Zero's command matcher (games/dq0c/cmdmatch.s) under HydraOS, timed (built and run in the
   emulator by sim/tools/commandmatch-test.js --hydra, its files in /pc): the grammar table in a RAM bank (at $8000
   while it matches), the index and the aliases in the main RAM, the corpus read a line at a time.
   /pc/grammar.bin, /pc/index.bin, /pc/aliases.bin, /pc/corpus.txt (W, N and D lines: the world's words when they
   change; C lines: the expected result, a space, the command).  It prints each command that differs, and the times.
   The files by open and read (stdio's buffers come from the heap, and with too little of it stdio reads a byte at a
   time); no printf. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <hydra.h>
#include "cmdmatch.h"

static unsigned char index_t[8400], alias_t[1024], rbuf[256];
static char world[420], names[200], deities[100], line[430], num[12];
static int rfd;
static unsigned rpos, rlen;

static void say(const char *s) { fputs(s, stdout); }
static void sayn(unsigned long n) { say(ultoa(n, num, 10)); }

static int openr(const char *name)
{
  int fd = open(name, O_RDONLY);
  if (fd < 0) { say("no "); say(name); say("\n"); exit(1); }
  return fd;
}

/* A file into memory, 256 bytes a read (copied: the window is never read into) */
static unsigned load(const char *name, unsigned char *to, unsigned max)
{
  int fd = openr(name), r;
  unsigned n = 0;
  while (n < max && (r = read(fd, rbuf, sizeof rbuf)) > 0) {
    if (n + r > max) r = max - n;
    memcpy(to + n, rbuf, r);
    n += r;
  }
  close(fd);
  say(name); say(" "); sayn(n); say("\n");
  return n;
}

/* The next line of rfd (without its newline), or -1 at the end */
static int getl(char *out, unsigned max)
{
  unsigned n = 0;
  int r;
  for (;;) {
    if (rpos == rlen) {
      r = read(rfd, rbuf, sizeof rbuf);
      if (r <= 0) { out[n] = 0; return n ? (int)n : -1; }
      rlen = r;
      rpos = 0;
    }
    if (rbuf[rpos] == '\n') { rpos++; break; }
    if (n + 1 < max) out[n++] = rbuf[rpos];
    rpos++;
  }
  out[n] = 0;
  return n;
}

int main(void)
{
  int bank, r, want;
  unsigned total = 0, nworld = 0, bad = 0;
  unsigned char *window = (unsigned char *)0x8000;
  clock_t t0, tinit, tworld = 0, tmatch = 0;
  char dirty = 1;
  bank = hy_banks_alloc(1);
  if (bank < 0) { say("no bank\n"); return 1; }
  hy_bank(bank);
  load("/pc/grammar.bin", window, 8192);
  load("/pc/index.bin", index_t, sizeof index_t);
  load("/pc/aliases.bin", alias_t, sizeof alias_t);
  t0 = clock();
  if (!cmd_init(window, index_t, alias_t)) { say("the tables are not right\n"); return 1; }
  tinit = clock() - t0;
  rfd = openr("/pc/corpus.txt");
  while (getl(line, sizeof line) >= 0) {
    if (line[0] == 'W') { strcpy(world, line + 2); dirty = 1; continue; }
    if (line[0] == 'N') { strcpy(names, line + 2); dirty = 1; continue; }
    if (line[0] == 'D') { strcpy(deities, line + 2); dirty = 1; continue; }
    if (line[0] != 'C') continue;
    if (dirty) {
      t0 = clock();
      cmd_world(world, names, deities);
      tworld += clock() - t0;
      nworld++;
      dirty = 0;
    }
    want = line[2] - '0';
    t0 = clock();
    r = cmd_match(line + 4);
    tmatch += clock() - t0;
    total++;
    if (r != want) { bad++; say("differs: "); say(line + 4); say("\n"); }
  }
  close(rfd);
  sayn(total); say(" commands, "); sayn(bad); say(" differ\n");
  say("ticks (200 a second): init "); sayn(tinit); say("; the world set "); sayn(nworld); say(" times, ");
  sayn(tworld); say("; matching "); sayn(tmatch); say("\n");
  say("a command: "); sayn((unsigned long)tmatch * 17898UL / total); say(" cycles at 3.58 MHz (17,898 a tick)\n");
  return bad ? 1 : 0;
}
