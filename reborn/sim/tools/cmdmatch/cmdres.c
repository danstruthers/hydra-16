/* cmdres.c - the phrasing test's harness on sim65 (cc65's 6502 simulator): a case's lines (its arguments, from
   sim/tools/commandmatch-test.js --phrases: cmdmatch/phrases.txt's cases and the corpus's), each matched by the grammar
   (games/dq0c/cmdmatch.s), made a command (cmdmean.s: hub.h's struct cmd) and resolved by the UI's resolver
   (games/dq0/resolve.c, compiled here as it is in the game, its overlay's segments in the main RAM: cmdres.cfg),
   among the things and denizens the case says the character perceives (a struct sight of the case's), as main.c's
   s_command does: the ways made the compass's, an answer to a question taken first, then resolve.  What it says
   ("S ..."), and after the case's last line its command ("R ..."), for commandmatch-test.js to hold to what the
   case expects.  Its arguments: the lines (| between them), the facing (0-7), then each thing it perceives, "den dx dy
   size name" (den 0 a thing lying about, 1 a denizen, 2 one down): each case a run of its own, the resolver's state
   fresh. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "cmdmatch.h"
#include "cmd_table.h"
#include "cmd_index.h"
#include "cmd_alias.h"
#include "cmd_words.h"
#include "dq0.h"
#include "hub.h"
#include "level.h"

int __fastcall__ cmd_winit(const unsigned char *words);
const unsigned char *__fastcall__ cmd_mean(unsigned char face);
unsigned char __fastcall__ o_answer(unsigned char chosen);     /* (resolve.c's, the overlay's own: no stub) */
unsigned char o_resolve(void);

/* What the resolver uses of the UI's: its command, the senses' record, the words' buffer, the facing, the target's
   letting go */
struct cmd cmd;
struct sight sight_copy;
char lb[400];
unsigned char pc_face;
unsigned char untarget;
static struct sight seen;                       /* (The case's: what hub_mem copies for the senses' bank) */

void __fastcall__ hub_mem(unsigned char bank, void *to, const void *from, unsigned n)
{
  (void) bank;
  (void) from;
  memcpy(to, &seen, n);
}

void __fastcall__ msg(const char *s)
{
  printf("S %s\n", s);
}

void sayf(const char *fmt, ...)
{
  va_list ap;
  printf("S ");
  va_start(ap, fmt);
  vprintf(*fmt == '\n' ? fmt + 1 : fmt, ap);
  va_end(ap);
  printf("\n");
}

const signed char head_x[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };    /* (level.c's) */
const signed char head_y[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

/* (level.c's) The heading nearest the way (dx, dy) goes */
unsigned char __fastcall__ bearing(int dx, int dy)
{
  int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;

  if (ax * 5 < ay * 2) {
    return dy < 0 ? H_N : H_S;
  }
  if (ay * 5 < ax * 2) {
    return dx > 0 ? H_E : H_W;
  }
  return dx > 0 ? (dy < 0 ? H_NE : H_SE) : (dy > 0 ? H_SW : H_NW);
}

#define HERE 100                                /* (The character's square: 100, 100) */

int main(int argc, char *argv[])
{
  static char world[200];
  const char *l, *e, *name;
  char line[120];
  unsigned char i, f, d, n, den, size;
  int dx, dy;
  signed char ok;

  if (argc < 3) {
    printf("no case\n");
    return 1;
  }
  if (!cmd_init(cmd_table, cmd_index, cmd_alias) || !cmd_winit(cmd_words)) {
    printf("the tables are not right\n");
    return 1;
  }
  memset(&seen, 0, sizeof seen);
  seen.x = seen.y = HERE;
  for (i = 3; i < argc; ++i) {                 /* (The things it sees lying about, and the denizens) */
    den = atoi(strtok(argv[i], " "));
    dx = atoi(strtok(0, " "));
    dy = atoi(strtok(0, " "));
    size = atoi(strtok(0, " "));
    name = strtok(0, "");
    strcat(world, strrchr(name, ' ') ? strrchr(name, ' ') + 1 : name);   /* (The world names it by its last word) */
    strcat(world, " ");
    if (den) {
      n = seen.ndens++;
      strcpy(seen.den[n].name, name);
      seen.den[n].x = HERE + dx;
      seen.den[n].y = HERE + dy;
      seen.den[n].doing = den == 2 ? DO_DOWN : DO_WATCH;
    } else {
      n = seen.nitems++;
      strcpy(seen.item[n].name, name);
      seen.item[n].x = HERE + dx;
      seen.item[n].y = HERE + dy;
      seen.item[n].hid = size << 4;
    }
  }
  cmd_world(world, "", "");
  memcpy(&sight_copy, &seen, sizeof seen);
  pc_face = f = atoi(argv[2]);
  for (l = argv[1]; *l; l = *e ? e + 1 : e) {  /* (Each line, | between them) */
    for (e = l; *e && *e != '|'; ++e) {
    }
    memcpy(line, l, e - l);
    line[e - l] = 0;
    ok = cmd_match(line);
    memcpy(&cmd, cmd_mean(f), sizeof cmd);
    if (cmd.dir != 0xFF && (cmd.dir & CQ_REL)) {         /* (main.c's s_cq_cmd: the ways the compass's) */
      cmd.dir = (f + cmd.dir) & 7;
    }
    if (cmd.obj.lies != 0xFF && (cmd.obj.lies & CQ_REL)) {
      cmd.obj.lies = (f + cmd.obj.lies) & 7;
    }
    if (cmd.action == VB_FACE_DIRECTION && (cmd.flags & CF_HALF) && cmd.dir < GO_UP) {
      d = (cmd.dir - f) & 7;
      cmd.dir = d == 6 ? (f + 7) & 7 : d == 2 ? (f + 1) & 7 : cmd.dir;
    }
    d = o_answer(0);                            /* (main.c's s_command: an answer first, then the grammar's verdict,
                                                   then resolved) */
    if (!d && !ok) {
      printf("S I don't understand that.\n");
      continue;
    }
    if (o_resolve() != RV_ON) {
      continue;
    }
  }
  printf("R %u %u %u %u %u %u %u |%s|%d,%d|%s|%d,%d|\n", cmd.action, cmd.flags, cmd.dflt, cmd.prep, cmd.manner, cmd.obj.sel,
    cmd.obj.kind,
    cmd.obj.word, cmd.obj.x ? (int) cmd.obj.x - HERE : 99, cmd.obj.y ? (int) cmd.obj.y - HERE : 99, cmd.other.word,
    cmd.other.x ? (int) cmd.other.x - HERE : 99, cmd.other.y ? (int) cmd.other.y - HERE : 99);
  return 0;
}
