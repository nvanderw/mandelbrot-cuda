#ifndef _MANDELBROT_H_
#define _MANDELBROT_H_
#include <stdio.h>

int *create_mandelbrot(int res_x, int res_y, float min_x, float min_y,
        float max_x, float max_y, int iter);
#endif
