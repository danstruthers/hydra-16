/* cmdmean.c - the command's meaning (games/dq0c/cmdmean.s) on sim65, cc65's 6502 simulator: each line matched, then
   its struct pcmd made (the character facing the line's number, modulo 8), printed with the cycles cmd_mean took, for
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

/* The words' table (cmd_words.h's), and a line's meaning: the struct pcmd (dq0.h's) at cm_cmd */
int __fastcall__ cmd_winit(const unsigned char *words);
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
  for (i = 0; i < CMD_LINES; i++) {
    r = cmd_match(cmd_line[i]);
    t = clk();
    c = cmd_mean(i & 7);
    t = clk() - t;
    printf("%d %d %lu", i, r, t);
    for (j = 0; j < 7; j++) {
      printf(" %u", c[j]);
    }
    printf(" %u %u |%s|%s|\n", c[23], c[40], (const char *) c + 7, (const char *) c + 24);
  }
  printf("done %d\n", CMD_LINES);
  return 0;
}
