/* TUIK Textual User Interface Kit. A widget library for TUIs using termbox2.
 *
 * Widgets are separate struct types, all must start with the TUIK_WIDGET fields
 * which contains pointer to the vtable and size information.
 *
 */

#include "termbox2.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct tuik_opts_add {
  // constraints for packing (0 is unset)
  uint16_t min_width, max_width, min_height, max_height;
  uint8_t flags;
} tuik_opts_add;

/* fields shared by all widget types */
#define TUIK_WIDGET \
  tuik_vtable *type;                            \
  uint16_t x, y, w, h;

/* You can cast unknown widget type to this */
typedef struct tuik_widget {
  TUIK_BASE_WIDGET
} tuik_widget;


/* widget type vtable entry, points to the implementation types of widgets (render, keypress, etc) */
typedef struct tuik_vtable {
  void (*_tuik_render)(void*);
  bool (*_tuik_keypress)(void*, tb_event*);
  bool (*_tuik_click)(void*, tb_event*);

  /* Only for containers, others should panic */
  void (*_tuik_add)(void*, void*,tuik_opts_add);
  void (*_tuik_pack)(void*);

  enum TUIK_TYPE type;
} tuik_vtable;


#define DO_TYPES(X)                                                            \
  X(textarea)                                                                  \
  X(checkbox)                                                                  \
  X(box)

/* Type is needed for generic containers */
typedef enum TUIK_TYPE {
#define X(t) TUIK_TYPE_##t,
  DO_TYPES(X)
#undef X
  _TUIK_TYPE_COUNT
} tuik_type;

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

typedef struct tuik_opts_textarea {
  tuik_rect rect;
  bool multiline;
  bool show_line_numbers;
  const char *text; // initial text, if any
} tuik_opts_textarea;

typedef struct tuik_checkbox {
  tuik_rect rect;
  const char *label;
  bool checked;
  bool unicode; // if false, use [ ] / [X] instead of ☐ and ☒
} tuik_checkbox;

typedef struct tuik_opts_checkbox {
  tuik_rect rect;
  bool checked; // initially checked?
  bool unicode;
  const char *label;
} tuik_opts_checkbox;

typedef enum tuik_orientation {
  TUIK_ORIENTATION_VERTICAL = 0,
  TUIK_ORIENTATION_TOP_TO_BOTTOM = 0,
  TUIK_ORIENTATION_HORIZONTAL = 1,
  TUIK_ORIENTATION_LEFT_TO_RIGHT = 1
} tuik_orientation;

// If this entry should take up any extra space
#define TUIK_CONTAINER_ENTRY_GROW 1

typedef struct tuik_container_entry {
  tuik_type type;
  void *widget;
  // constraints for packing (0 is unset)
  uint16_t min_width, max_width, min_height, max_height;
  uint8_t flags;
} tuik_container_entry;

typedef struct tuik_box {
  tuik_rect rect;
  size_t widgets_count, widgets_capacity;
  tuik_container_entry *widgets;
  tuik_orientation orientation;
} tuik_box;

typedef struct tuik_opts_box {
  tuik_rect rect;
  tuik_orientation orientation;
} tuik_opts_box;

tuik_textline *tuik_textline_new(const char *cstr);
tuik_box *tuik_new_box(tuik_opts_box opts);

void tuik_textarea_render(tuik_textarea *textarea);
void tuik_checkbox_render(tuik_checkbox *checkbox);
#define tuik_render(widget)                                                    \
  _Generic((widget), tuik_textarea *                                           \
           : tuik_render_textarea, tuik_checkbox *                             \
           : tuik_render_checkbox, tuik_box *                                  \
           : tuik_render_box)(widget)



typedef struct tb_event tb_event;



void tuik_textarea_keypress(tuik_textarea *w, tb_event *e);
void tuik_checkbox_keypress(tuik_checkbox *w, tb_event *e);
#define tuik_keypress(widget,e) _Generic((widget), \
  tuik_textarea* : tuik_textarea_keypress, \
  tuik_checkbox* : tuik_checkbox_keypress)((widget),(e))

void tuik_textarea_click(tuik_textarea *w, tb_event *e);
void tuik_checkbox_click(tuik_textarea *w, tb_event *e);
#define tuik_click(widget, e)                                                  \
  _Generic((widget), tuik_textarea *                                           \
           : tuik_textarea_click, tuik_checkbox *                              \
           : tuik_checkbox_click)((widget), (e))

#define tuik_set_xy(w, _x, _y)                                                 \
  do {                                                                         \
    (w)->rect.x = (_x);                                                        \
    (w)->rect.y = (_y);                                                        \
  } while (false)



#define tuik_xy(w) (w)->rect.x, (w)->rect.y

#define tuik_new(type, ...) (tuik_##type*) tuik_new_##type((tuik_opts_##type){__VA_ARGS__})


// DO_TYPES didn't work recursively here
#define tuik_typeof(w)                                                         \
  _Generic((w), tuik_textarea *                                                \
           : TUIK_TYPE_textarea, tuik_checkbox *                               \
           : TUIK_TYPE_checkbox, tuik_box *                                    \
           : TUIK_TYPE_box)


#define tuik_add(container, w, ...)                                            \
  _Generic((container), tuik_box *                                             \
           : tuik_add_box)(                                                    \
      (container), (tuik_container_entry){                                     \
                       .widget = (w), .type = tuik_typeof(w), __VA_ARGS__})


#define tuik_pack(container) \
  _Generic((container), tuik_box * : tuik_pack_box)((container))

tuik_rect tuik_fullscreen();




// PENDING: add tuik_<type>_free

/* -------------------[ Implementation below ]--------------------------------- */

#ifdef TUIK_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

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

tuik_textline *tuik_new_textline(const char *cstr) {
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

#define RECT_FMT "x:%d, y:%d, w: %d, h: %d"
#define RECT_ARG(r) (r).x, (r).y, (r).w, (r).h

void tuik_render_textarea(tuik_textarea *w) {
  tuik_dbg("render textarea, "RECT_FMT, RECT_ARG(w->rect));

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
      if(line->data) {
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
      } else {
        tuik_dbg("line is empty %d", l);
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
      w->col = w->lines[w->row]->len;
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
    if (w->row < w->lines_count - 1)
      w->row += 1;
    // ensure col is within bounds
    if(w->col > w->lines[w->row]->len)
      w->col = w->lines[w->row]->len;
    break;
  case TB_KEY_ARROW_UP:
    if (w->row > 0)
      w->row -= 1;
    // ensure col is within bounds
    if(w->col > w->lines[w->row]->len)
      w->col = w->lines[w->row]->len;
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
    w->lines_count = new_lines_count;
    break;
  }
  default: {
    if(e->ch) {
      // regular letter, append it at position

      tuik_textline *line = w->lines[w->row];
      if(line->len == line->capacity) {
        tuik_dbg("increase line cap %zu => %zu", line->capacity, line->capacity * 2);
        line->capacity *= 2;
        line->data = (char*)TUIK_REALLOC(line->data, line->capacity); // double line capacity
      }
      tuik_dbg("before append, line len: %zu, col: %d", line->len, w->col);
      line->len += 1;
      for(int i = line->len-1; i >= 0; i--) {
        line->data[i] = (i > w->col ? line->data[i-1]
                         : (i == w->col ? e->ch : line->data[i]));
      }
      tuik_dbg("after append, line len: %zu, col: %d", line->len, w->col);
      w->col += 1;
    }

  }

  }
}

void tuik_textarea_click(tuik_textarea *w, tb_event *e) {
  int new_col = e->x - w->rect.x - 4; // check line count to determine how many to reserve for line numbers
  int new_row = e->y - w->rect.y - 1; // BOUNDS!
  w->col = new_col;
  w->row = new_row;
  if(w->row < 0) w->row = 0;
  if(w->row > w->lines_count - 1) w->row = w->lines_count - 1;
  if(w->col < 0) w->col = 0;
  if(w->col > w->lines[w->row]->len) w->col = w->lines[w->row]->len;
  tuik_dbg("new col: %d, row: %d", new_col, new_row);


}

const char *_tuik_strdup(const char *str, size_t *len) {
  if(!str) {
    *len = 0;
    return NULL;
  }
  *len = strlen(str);
  char *dup =  TUIK_ANEW(char, *len + 1);
  strcpy(dup, str);
  return (const char*) dup;
}

const char *_tuik_strndup(const char *str, size_t len) {
  char *dup = TUIK_ANEW(char, len + 1);
  memcpy(dup, str, len);
  dup[len] = 0;
  return (const char*)dup;
}

tuik_textarea *tuik_new_textarea(tuik_opts_textarea opts) {
  tuik_textarea *t = TUIK_NEW(tuik_textarea);
  t->col = 0;
  t->row = 0;
  t->line = 0;
  t->lines_count = 0;
  if(opts.text) {
    t->lines_count++;
    for(char *at = (char*)opts.text; *at; at++) {
      if(*at == '\n') t->lines_count++;
    }
  }
  if(t->lines_count > 0) {
    t->lines = TUIK_ANEW(tuik_textline*, t->lines_count);
    t->lines_capacity = t->lines_count;
    int line = 0;
    char *start = (char*) opts.text;
    while(1) {
      char *end = start;
      while(*end != 0 && *end != '\n') { tuik_dbg("at %c", *end); end++; }
      tuik_dbg("RIVI KÄYTY LÄPI %d (%zu)", line, t->lines_count );
      t->lines[line] = TUIK_NEW(tuik_textline);
      size_t len = end-start;
      tuik_dbg("rivi pituus %zu", len);
      t->lines[line]->data = TUIK_ANEW(char, len);
      t->lines[line]->capacity = len;
      t->lines[line]->len = len;
      tuik_dbg("got line (%zu) bytes: %.*s", len, (int)len, start);
      memcpy(t->lines[line]->data, start, len);
      tuik_dbg(" loppu on %d", *end);
      if(!*end) break;
      start = end + 1;
      line++;
    }
  } else {
    t->lines = NULL;
    t->lines_capacity = 0;
  }
  tuik_dbg("textarea luotu!");
  return t;
}

tuik_checkbox *tuik_new_checkbox(tuik_opts_checkbox opts) {
  tuik_checkbox *cb = TUIK_NEW(tuik_checkbox);
  cb->checked = opts.checked;
  cb->unicode = opts.unicode;
  if(!opts.label) tuik_panic("No label specified for tuik_checkbox!");
  size_t len;
  cb->label = _tuik_strdup(opts.label, &len);
  cb->rect.h = 1;
  cb->rect.w = len + (cb->unicode ? 2 : 4);
  return cb;
}

void tuik_render_checkbox(tuik_checkbox *cb) {
  int x = cb->rect.x, y = cb->rect.y;
  if(cb->unicode) {
    tb_print(x, y, TB_WHITE, TB_BLACK, cb->checked ? "☒ " : "☐ ");
    x += 2;
  } else {
    tb_print(x, y, TB_WHITE, TB_BLACK, cb->checked ? "[X] " : "[ ] ");
    x += 4;
  }
  tb_print(x, y, TB_WHITE, TB_BLACK, cb->label);
}

tuik_box *tuik_new_box(tuik_opts_box opts) {
  tuik_box *b = TUIK_NEW(tuik_box);
  b->widgets_count = 0;
  b->widgets_capacity = 2; // start with 2 widget capacity
  b->widgets = TUIK_ANEW(tuik_container_entry, 2);
  b->orientation = opts.orientation;
  b->rect = opts.rect;
  return b;
}

void tuik_render_box(tuik_box *box) {
  for(int i=0;i<box->widgets_count;i++) {
    tuik_container_entry e = box->widgets[i];
    switch(e.type) {
#define TYPE(type) case TUIK_TYPE_##type: tuik_render_##type((tuik_##type*)e.widget); break;
      DO_TYPES(TYPE)
#undef TYPE
    case _TUIK_TYPE_COUNT: tuik_panic("Unreachable!");
    }
  }
}

void _tuik_set_rect(tuik_container_entry *e, tuik_rect r) {
  switch(e->type) {
#define TYPE(t) case TUIK_TYPE_##t: ((tuik_##t*)e->widget)->rect = r; break;
    DO_TYPES(TYPE)
#undef TYPE
  case _TUIK_TYPE_COUNT: tuik_panic("Unreachable!");
      }
}

void tuik_pack_box(tuik_box *box) {
  // split my rect into the children
  if(box->orientation == TUIK_ORIENTATION_TOP_TO_BOTTOM) {
    // make every child my width, divvy up my height to children
    int num_grow = 0; // how many have grow flag
    int min_width = 0;
    for(int i=0;i<box->widgets_count;i++) {
      if(box->widgets[i].flags & TUIK_CONTAINER_ENTRY_GROW) {
        num_grow += 1;
      }
      int minw = box->widgets[i].min_width;
      min_width += minw ? minw : 1;
    }
    // how much space do we have above min width
    int space = box->rect.h - min_width;
    int space_per_grow = 0;
    if(!num_grow && space) {
      tuik_dbg("Box has extra space, but no components want to grow.");
    } else {
      space_per_grow = space / num_grow;
      space %= num_grow; // if there's still left over, we'll give it to the 1st one
    }
    tuik_dbg("have height %d to grow %d", space, space_per_grow);

    // go through widgets and set their rects
    bool first = true;
    int y = box->rect.y;
    for(int i=0;i<box->widgets_count; i++) {
      tuik_rect r = (tuik_rect){.x = box->rect.x, .w = box->rect.w,
                                .y = y,
                                .h = box->widgets[i].min_height};
      if(!r.h) r.h += 1;
      tuik_dbg("widget minh %d", r.h);
      if(box->widgets[i].flags & TUIK_CONTAINER_ENTRY_GROW) {
        r.h = space_per_grow + (first ? space : 0);
        tuik_dbg(" grow to %d", r.h);
        first = false;
      }
      tuik_dbg("pack box, child[%d] rect= "RECT_FMT, i, RECT_ARG(r));
      _tuik_set_rect(&box->widgets[i], r);
      y += r.h;

    }
  }
}

void tuik_add_box(tuik_box *box, tuik_container_entry entry) {
  if(box->widgets_count == box->widgets_capacity) {
    box->widgets_capacity *= 2;
    box->widgets = (tuik_container_entry*)TUIK_REALLOC(box->widgets, box->widgets_capacity);
  }
  box->widgets[box->widgets_count++] = entry;
}

tuik_rect tuik_fullscreen() {
  return (tuik_rect){.x = 0, .y = 0, .w = tb_width()-1, .h = tb_height()};
}

#endif // TUIK_IMPLEMENTATION
