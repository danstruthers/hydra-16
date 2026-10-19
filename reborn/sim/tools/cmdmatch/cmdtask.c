/* cmdtask.c - the command parser's task (games/dq0c) under HydraOS, timed through the hub's protocol as the UI uses
   it (sim/tools/commandmatch-test.js --task builds it and runs it in the emulator, its files in /pc).  It plays the
   UI's part with a hub of its own, two banks (bank 0, its header: struct hub; bank 1, the commands' text), starts
   /pc/dq0c in role T_COMMANDS, and times the round trips: dq0c's first answer (its start: its tables read from
   /pc), empty requests (CQ_NONE: the protocol alone), and the corpus's commands (CQ_MATCH, the world's words before
   them as they change, CQ_WORLD), each result held to the host's (/pc/corpus.txt, as cmdhy.c reads it). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <hydra.h>
#include "hub.h"

#define PINGS 400
#define TEXT_BANK 1

static unsigned char seg, u[2], x[2], rbuf[256];
static char world[420], names[200], deities[100], wtext[730], line[430], num[12], args[24];
static struct hy_regs r;
static int rfd;
static unsigned rpos, rlen;

static void say(const char *s) { fputs(s, stdout); }
static void sayn(unsigned long n) { say(ultoa(n, num, 10)); }

static void map(unsigned char b)
{
  *(volatile unsigned char *)0xFFF1 = u[b];
  *(volatile unsigned char *)0 = x[b];
}

/* The next line of rfd (without its newline), or -1 at the end */
static int getl(char *out, unsigned max)
{
  unsigned n = 0;
  int k;
  for (;;) {
    if (rpos == rlen) {
      k = read(rfd, rbuf, sizeof rbuf);
      if (k <= 0) { out[n] = 0; return n ? (int)n : -1; }
      rlen = k;
      rpos = 0;
    }
    if (rbuf[rpos] == '\n') { rpos++; break; }
    if (n + 1 < max) out[n++] = rbuf[rpos];
    rpos++;
  }
  out[n] = 0;
  return n;
}

/* A request to dq0c: its text (n bytes) into the text bank, posted, its reply waited for (asleep on the UI's
   doorbell, as hub_wait_t does).  OUT: its res */
static signed char ask(unsigned char kind, const char *text, unsigned n)
{
  static unsigned char seq;
  map(TEXT_BANK);
  memcpy(HUB_WINDOW, text, n);
  map(0);
  HUB->cq.kind = kind;
  HUB->cq.len = n;
  HUB->cq.from = T_UI;
  HUB->cq.seq = ++seq;
  HUB->cq.state = REQ_POSTED;
  hy_sem_release(HUB->sem[T_COMMANDS]);
  while (map(0), HUB->cq.state != REQ_DONE)
    hy_sem_wait(HUB->sem[T_UI], 255);
  HUB->cq.state = REQ_FREE;
  return HUB->cq.res;
}

int main(void)
{
  int s, task, want, res;
  unsigned i, n, total = 0, bad = 0, nworld = 0;
  unsigned char t, dirty = 1;
  clock_t t0, tstart, tping, tworld = 0, tmatch = 0;
  if ((s = hy_seg_create(2)) < 0) { say("no segment\n"); return 1; }
  seg = s;
  for (t = 0; t < 2; ++t) {                     /* (Each bank's U and $00: hub.c's find_banks) */
    r.a = seg;
    r.x = t;
    if (hy_call(HY_SEG_MAP, &r)) { say("no bank\n"); return 1; }
    u[t] = r.a;
    x[t] = r.x;
  }
  map(0);
  memset(HUB, 0, sizeof (struct hub));
  HUB->magic = HUB_MAGIC;
  HUB->version = HUB_VERSION;
  HUB->seg = seg;
  for (t = 0; t < NTASKS; ++t) {
    if ((s = hy_sem_new(0, 0)) < 0) { say("no semaphore\n"); return 1; }
    HUB->sem[t] = s;
  }
  HUB->task[T_UI] = hy_task();
  HUB->cq.bank = TEXT_BANK;
  utoa(seg, args, 10);                          /* dq0c SEG ROLE LIBDIR (hub.c's hub_spawn) */
  n = strlen(args) + 1;
  utoa(T_COMMANDS, args + n, 10);
  n += strlen(args + n) + 1;
  strcpy(args + n, "/pc");
  n += 4;
  args[n] = 0;
  t0 = clock();
  r.a = 0;
  r.r[0] = (unsigned)"/pc/dq0c";
  r.r[1] = (unsigned)args;
  if (hy_call(HY_SPAWN, &r)) { say("dq0c not started\n"); return 1; }
  task = r.a;
  map(0);
  ask(CQ_NONE, "", 0);                          /* Its first answer: once its tables are read */
  tstart = clock() - t0;
  t0 = clock();
  for (i = 0; i < PINGS; ++i) ask(CQ_NONE, "", 0);
  tping = clock() - t0;
  if ((rfd = open("/pc/corpus.txt", O_RDONLY)) < 0) { say("no corpus\n"); return 1; }
  while (getl(line, sizeof line) >= 0) {
    if (line[0] == 'W') { strcpy(world, line + 2); dirty = 1; continue; }
    if (line[0] == 'N') { strcpy(names, line + 2); dirty = 1; continue; }
    if (line[0] == 'D') { strcpy(deities, line + 2); dirty = 1; continue; }
    if (line[0] != 'C') continue;
    if (dirty) {                                /* The world's three lists, each 0-ended */
      n = strlen(world) + 1;
      memcpy(wtext, world, n);
      memcpy(wtext + n, names, strlen(names) + 1);
      n += strlen(names) + 1;
      memcpy(wtext + n, deities, strlen(deities) + 1);
      n += strlen(deities) + 1;
      t0 = clock();
      ask(CQ_WORLD, wtext, n);
      tworld += clock() - t0;
      nworld++;
      dirty = 0;
    }
    want = line[2] - '0';
    t0 = clock();
    res = ask(CQ_MATCH, line + 4, strlen(line + 4) + 1);
    tmatch += clock() - t0;
    total++;
    if (res != want) { bad++; say("differs: "); say(line + 4); say("\n"); }
  }
  close(rfd);
  HUB->quit = 1;                                /* Its end (the world's, in the game), and it waited for */
  hy_sem_release(HUB->sem[T_COMMANDS]);
  hy_wait(task, 0);
  sayn(total); say(" commands through dq0c, "); sayn(bad); say(" differ\n");
  say("ticks (200 a second): its start (its first answer) "); sayn(tstart); say("; ");
  sayn(PINGS); say(" empty requests "); sayn(tping); say("; the world set "); sayn(nworld); say(" times, ");
  sayn(tworld); say("; the commands "); sayn(tmatch); say("\n");
  say("cycles (17,898 a tick): an empty request's round trip "); sayn((unsigned long)tping * 17898UL / PINGS);
  say("; a command's "); sayn((unsigned long)tmatch * 17898UL / total); say("\n");
  return bad ? 1 : 0;
}
