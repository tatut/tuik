/* TUIK Textual User Interface Kit. A widget library for TUIs using termbox2. */

#include "termbox2.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum BOX_CH {
  BOX_TOP_LEFT,
  BOX_TOP,
  BOX_TOP_RIGHT,
  BOX_RIGHT,
  BOX_BOTTOM_RIGHT,
  BOX_BOTTOM,
  BOX_BOTTOM_LEFT,
  BOX_LEFT
} BOX_CH;

typedef uintattr_t tuik_col;

const char *_tuik_box[] = {
    [BOX_TOP_LEFT]     = "┌",
    [BOX_TOP]          = "─",
    [BOX_TOP_RIGHT]    = "┐",
    [BOX_RIGHT]        = "│",
    [BOX_BOTTOM_RIGHT] = "┘",
    [BOX_BOTTOM]       = "─",
    [BOX_BOTTOM_LEFT]  = "└",
    [BOX_LEFT]         = "│"
};

typedef struct tuik_rect {
  int x,y,w,h;
} tuik_rect;

void tuik_draw_box(int x, int y, int w, int h, tuik_col fg, tuik_col bg);
void tuik_draw_box_rect(tuik_rect c, tuik_col fg, tuik_col bg);

typedef struct tuik_textline {
  char *data;
  size_t len;
  size_t capacity;
} tuik_textline;

typedef struct tuik_textarea {
  tuik_rect rect;
  int row, col; // cursor position (0 based)
  int line;   // what line the display starts from (0 is first)
  tuik_textline **lines; // the text
  size_t lines_count; // current length
  size_t lines_capacity; // how many lines allocated
} tuik_textarea;

typedef struct tuik_textarea_opts {
  bool multiline;
  bool show_line_numbers;
} tuik_textarea_opts;

tuik_textline *tuik_textline_new(const char *cstr);

void tuik_textarea_render(tuik_textarea *textarea);
#define tuik_render(widget) _Generic((widget), tuik_textarea* : tuik_textarea_render((widget)))

typedef struct tb_event tb_event;

void tuik_textarea_keypress(tuik_textarea *w, tb_event *e);
#define tuik_keypress(widget,e) _Generic((widget), tuik_textarea* : tuik_textarea_keypress((widget), (e)))


/* -------------------[ Implementation below ]--------------------------------- */

#ifdef TUIK_IMPLEMENTATION

#include <stdlib.h>

#ifndef TUIK_ALLOC
#define TUIK_ALLOC(size) malloc((size))
#define TUIK_FREE(ptr) free((ptr))
#define TUIK_REALLOC(ptr,new_size) realloc((ptr), (new_size))
#endif

/* if debug output macro not defined, define it as no op */
#ifndef tuik_dbg
#define tuik_dbg(...)
#endif

/* if panic macro not defined, define as shutdown+print+exit */
#define tuik_panic(...)                                                             \
  {                                                                            \
    tb_shutdown();                                                             \
    fprintf(stderr, "[" __FILE__ ":%d] ", __LINE__);                           \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
    exit(1);                                                                   \
  }

/* Helpers to allocate without needing return checks */
void *_tuik_alloc(size_t size) {
  void *mem = TUIK_ALLOC(size);
  if(!mem) {
    tuik_panic("Alloc failed, wanted %zu bytes.", size);
  }
  return mem;
}
void *_tuik_realloc(void *ptr, size_t new_size) {
  void *mem = TUIK_REALLOC(ptr, new_size);
  if(!mem) {
    tuik_panic("Realloc failed, wanted %zu bytes.", new_size);
  }
  return mem;
}

#define TUIK_NEW(type) ((type*) _tuik_alloc(sizeof(type)))
#define TUIK_ANEW(type, count) ((type*) _tuik_alloc(sizeof(type)*(count)))

void tuik_draw_box(int x, int y, int w, int h, tuik_col fg, tuik_col bg) {
  for (int i = x; i <= x + w; i++) {
    const char *b;
    b = _tuik_box[i == x ? BOX_TOP_LEFT
            : (i == (x + w) ? BOX_TOP_RIGHT : BOX_TOP)];
    tb_print(i, y, fg, bg, b);
    b = _tuik_box[i == x ? BOX_BOTTOM_LEFT
            : (i == (x + w) ? BOX_BOTTOM_RIGHT : BOX_BOTTOM)];
    tb_print(i, y+h, fg, bg, b);
  }
  for (int i = y + 1; i <= y + h - 1; i++) {
    tb_print(x, i, fg, bg, _tuik_box[BOX_LEFT]);
    tb_print(x+w, i, fg, bg, _tuik_box[BOX_RIGHT]);
  }
}

void tuik_draw_box_rect(tuik_rect c, tuik_col fg, tuik_col bg) {
  tuik_draw_box(c.x, c.y, c.w, c.h, fg, bg);
}

tuik_textline *tuik_textline_new(const char *cstr) {
  tuik_textline *l = TUIK_NEW(tuik_textline);
  if (!l)
    return NULL;
  size_t len = strlen(cstr);
  l->capacity = len;
  l->len = len;
  l->data = TUIK_ANEW(char, len);
  if (!l->data) {
    free(l);
    return NULL;
  }
  memcpy(l->data, cstr, len);

  return l;
}

void tuik_textarea_render(tuik_textarea *w) {
  tuik_draw_box_rect(w->rect, TB_WHITE, TB_BLACK);
  // draw each line, start with the linenum
  int x = w->rect.x + 1;
  int y = w->rect.y + 1;

  int row = 0;
  for (int l = w->line; l < w->line + w->rect.h - 1; l++) {
    tb_printf(x, y, TB_WHITE, TB_MAGENTA, "%02d", (l + 1));
    if (l >= 0 && l < w->lines_count) {
      int col = 0;
      tuik_textline *line = w->lines[l];
      int tx = x + 3; // after the line number
      int chars = 0;
      while (chars < w->rect.w - 4) { // PENDING: long line support
        char ch = ' ';
        if (chars < line->len) {
          ch = line->data[chars];
        }
        if (w->row == row && w->col == col) {
          tb_printf(tx, y, TB_WHITE&TB_BLINK, TB_CYAN, "%c", ch);
        } else {
          tb_printf(tx, y, TB_WHITE, TB_BLACK, "%c", ch);
        }
        tx++;
        chars++;
        col++;
      }
    }
    row++;
    y++;
  }
  //tb_printf(x, y, TB_WHITE, TB_GREEN, "r: %d, c: %d", w->row, w->col);
}

void _tuik_textarea_ensure_lines_capacity(tuik_textarea *w, size_t desired_capacity) {
  if (w->lines_capacity < desired_capacity) {
    size_t new_capacity = w->lines_capacity * 2;
    if (new_capacity < desired_capacity)
      new_capacity = desired_capacity;
    w->lines =
      _tuik_realloc(w->lines, sizeof(tuik_textline *) * new_capacity);
    if (!w->lines)
      tuik_panic("Unable to allocate textarea lines.");
    for(size_t i = w->lines_capacity; i<new_capacity; i++) {
      w->lines[i] = TUIK_NEW(tuik_textline);
      w->lines[i]->capacity = 0;
      w->lines[i]->data = NULL;
      w->lines[i]->len = 0;
    }
    w->lines_capacity = new_capacity;
  }
}

void tuik_textarea_keypress(tuik_textarea *w, tb_event *e) {

  switch (e->key) {
  case TB_KEY_CTRL_A:
    if(e->mod & TB_MOD_CTRL)
      w->col = 0;
    break;
  case TB_KEY_CTRL_E:
    if(e->mod & TB_MOD_CTRL)
      w->col = w->lines[w->row]->len + 1;
    break;
  case TB_KEY_ARROW_LEFT:
    if (w->col > 0)
      w->col -= 1; // wrap to prev line
    break;
  case TB_KEY_ARROW_RIGHT:
    if (w->col < w->lines[w->row]->len)
      w->col += 1; // wrap around
    break;
  case TB_KEY_ARROW_DOWN:
    if (w->row < w->lines_count)
      w->row += 1;
    break;
  case TB_KEY_ARROW_UP:
    if (w->row > 0)
      w->row -= 1;
    break;
  case TB_KEY_ENTER: {
    // insert new line, possibly moving characters after cursor
    // to the new line
    tuik_dbg("old lines_count %zu", w->lines_count);
    size_t new_lines_count = w->lines_count + 1;
    _tuik_textarea_ensure_lines_capacity(w, new_lines_count);
    if(w->col == 0) {
      // at first column, just move lines below us down one
      for(int i=new_lines_count-1; i > w->row; i--) {
        tuik_dbg("swap line %d and %d", i, i-1);
        tuik_textline *tmp = w->lines[i];
        w->lines[i] = w->lines[i-1];
        w->lines[i-1] = tmp;
        if(i > 100) tuik_panic("bug!");
      }
    }
    break;
  }

  }
}


#endif // TUIK_IMPLEMENTATION
