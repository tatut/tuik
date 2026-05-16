# TUIK: Textual User Interface Kit

TUIK is a C library for retained mode terminal widgets.
It depends on [termbox2](https://github.com/termbox/termbox2).

Widgets:
* textarea (simple single or multi line text editor)
* checkbox
* select
* split (horizontal or vertical splitter)
* label
* table
* button

## API

All widgets have a constructor named: `<type>* tuik_<type>_new(struct tuik_<type>_options opts);`

Widgets also have a similarly named `_render` and event handlers.
Generic macro variants that dispatch on type are also provided.

## Memory management

Widgets don't have a reference to any allocator but all allocations
are done with user (re)definable macros:

* `TUIK_ALLOC(size)` allocate `size` bytes (defaults to `malloc`)
* `TUIK_REALLOC(ptr, new_size)` reallocate existing `ptr` to hold `new_size` bytes (defaults to `reallloc`)
* `TUIK_FREE(ptr)` free previously allocated `ptr`

User can define memory management macros in the file that defines `TUIK_IMPLEMENTATION` before including `tuik.h`.
Note: If you define `TUIK_ALLOC` you must define all three.
