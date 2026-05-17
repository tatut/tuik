demo: tuik.h demo.c
	cc -fsanitize=address -o demo demo.c
