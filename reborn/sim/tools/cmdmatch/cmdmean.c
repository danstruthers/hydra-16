/* cmdmean.c - the command's meaning (games/dq0c/cmdmean.s) on sim65, cc65's 6502 simulator: each line matched, then
   its roles and its struct cmd made (the character facing the line's number, modulo 8), printed with the cycles the two
   took (cmd_roles, cmd_mean), for
   sim/tools/commandmatch-test.js --mean, which holds each to the host's (commandwords.js's scan).  The cmd_*.h files
   are commandmatch-gen.js's. */
#include <stdio.h>
#include <string.h>
#include <sim65.h>
#include "cmdmatch.h"
#include "cmd_table.h"
#include "cmd_index.h"
#include "cmd_alias.h"
#include "cmd_words.h"
#include "cmd_lines.h"

/* The words' table (cmd_words.h's), and a line's meaning: the struct cmd (hub.h's) at cm_cmd */
int __fastcall__ cmd_winit(const unsigned char *words);
void __fastcall__ cmd_roles(unsigned char ok);
void cmd_wthings(void);
const unsigned char *__fastcall__ cmd_mean(unsigned char face);

static unsigned long clk(void)
{
  peripherals.counter.latch = 0;
  return peripherals.counter.value32[0];
}

int main(void)
{
  int i, r, j;
  unsigned long t;
  const unsigned char *c;
  if (!cmd_init(cmd_table, cmd_index, cmd_alias) || !cmd_winit(cmd_words)) {
    printf("the tables are not right\n");
    return 1;
  }
  cmd_world(cmd_world_words, "", "");           /* (The world's words, as the game's UI tells dq0c them; the things'
                                                   marked: dq0c.s's) */
  cmd_wthings();
  for (i = 0; i < CMD_LINES; i++) {
    r = cmd_match(cmd_line[i]);
    t = clk();
    cmd_roles((unsigned char) r);
    c = cmd_mean(i & 7);
    t = clk() - t;
    printf("%d %d %lu", i, r, t);
    for (j = 0; j < 13; j++) {                  /* (Its fields, action to n; then each thing's kind, sel, n, lies) */
      printf(" %u", c[j]);
    }
    for (j = 0; j < 4; j++) {
      printf(" %u", c[13 + j]);
    }
    for (j = 0; j < 4; j++) {
      printf(" %u", c[37 + j]);
    }
    printf(" %u %u |%s|%s|\n", c[61], c[62], (const char *) c + 21, (const char *) c + 45);
  }
  printf("done %d\n", CMD_LINES);
  return 0;
}
