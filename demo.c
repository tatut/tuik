#include <stdio.h>

FILE *dbg_log; // debug log
#define tuik_dbg(...)                                                               \
  {                                                                            \
    fprintf(dbg_log, "[" __FILE__ ":%d] ", __LINE__);                          \
    fprintf(dbg_log, __VA_ARGS__);                                             \
    fprintf(dbg_log, "\n");                                                    \
    fflush(dbg_log);                                                           \
  }


#define TUIK_IMPLEMENTATION
#include "tuik.h"

#define TB_IMPL
#include "termbox2.h"


int main(int argc, char **argv) {
  dbg_log = fopen("demo.log", "w+");
  if(!dbg_log) tuik_panic("Can't open demo.log to write!");

  tb_init();
  tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
  struct tb_event e = {0};

  tuik_textarea w = {.rect = {.x = 5, .y = 13, .w = 45, .h = 7 },
                     .lines_count = 4,
                     .lines_capacity = 4,
                     .lines = TUIK_ANEW(tuik_textline*, 4)};
  w.lines[0] = tuik_textline_new("SELECT *");
  w.lines[1] = tuik_textline_new("  FROM foo f");
  w.lines[2] = tuik_textline_new(" WHERE f.created > '1981-04-08' *");
  w.lines[3] = tuik_textline_new("   AND f.deceased IS NULL");

  while (e.ch != 'q') {
    tuik_dbg("loop!");
    tb_clear();
    tuik_render(&w);
    tb_present();

    int ret = tb_poll_event(&e);
    if(ret == TB_ERR_POLL && tb_last_errno() == EINTR) continue;
    else if(e.type == TB_EVENT_KEY) {
      tuik_keypress(&w, &e);
    }
  }
  tb_shutdown();
  return 0;
}
