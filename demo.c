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

  tuik_box *box = tuik_new(box, .orientation = TUIK_ORIENTATION_TOP_TO_BOTTOM);
  tuik_textarea *edit = tuik_new(textarea,
                                 .multiline = true,
                                 .show_line_numbers = true,
                                 .text =
                                 "SELECT *\n"
                                 "  FROM foo f\n"
                                 " WHERE f.created > '1981-04-08'\n"
                                 "   AND f.deceased IS NULL");


  tuik_checkbox *cb = tuik_new(checkbox,
      .checked = true,
      .unicode = false,
      .label = "Frobnicate");

  tuik_type tt = tuik_typeof(cb);
  tuik_dbg("type: %d\n", tt);

  tuik_add(box, cb, .min_height = 1);
  tuik_add(box, edit, .min_height = 5, .flags = TUIK_CONTAINER_ENTRY_GROW);
  tuik_dbg("lisätty komponentit!");
  box->rect = tuik_fullscreen();
  tuik_dbg("fs x:%d,y:%d,w:%d,h:%d", box->rect.x, box->rect.y, box->rect.w, box->rect.h);
  tuik_pack(box);

  while (e.ch != 'q') {
    tuik_dbg("loop!");
    tb_clear();
    tuik_render(box);
    tb_present();

    int ret = tb_poll_event(&e);
    if(ret == TB_ERR_POLL && tb_last_errno() == EINTR) continue;
    else if(e.type == TB_EVENT_KEY) {
      tuik_keypress(edit, &e);
    } else if(e.type == TB_EVENT_MOUSE) {
      tuik_click(edit, &e);
    } else if(e.type == TB_EVENT_RESIZE) {
      box->rect = tuik_fullscreen();
      tuik_pack(box);
    }

  }
  tb_shutdown();
  return 0;
}
