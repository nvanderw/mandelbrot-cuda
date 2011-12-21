#include <stdio.h>
#include <stdlib.h>
#include <cv.h>
#include <highgui.h>
#include "mandelbrot.h"

#if 1
#define RES_X 32768
#define RES_Y 18432
#define NUM_ITER 1<<15
#else
#define RES_X 4096
#define RES_Y 2304
#define NUM_ITER 1<<10
#endif

#define COLOR_DENSITY_NUM 80000
#define COLOR_DENSITY_DEN 1

int main(int argc, char **argv) {
    IplImage *img;
    float *region;
    size_t i;

    char *colors;
    FILE *colorsfile = fopen("colors.bin", "r");
    if(!colorsfile) {
        fprintf(stderr, "Error opening file.\n");
        exit(1);
    }

    fseek(colorsfile, 0, SEEK_END);
    int numcomps = ftell(colorsfile);
    fprintf(stderr, "Components: %d\n", numcomps);
    int numcolors = numcomps/3;

    fseek(colorsfile, 0, SEEK_SET);
    colors = (char*) malloc(numcomps);
    if(!colors) {
        fprintf(stderr, "Error allocating memory.\n");
        exit(1);
    }
    fread(colors, 3, numcolors, colorsfile);
    // Create an 8-bit 1-channel image
    img = cvCreateImage(cvSize(RES_X, RES_Y), IPL_DEPTH_8U, 3);
    region = (float*) create_mandelbrot(RES_X, RES_Y, -0.651172, 0.59668, -0.43584, 0.711035, NUM_ITER);
    if(!region || !img) {
        fprintf(stderr, "Memory allocation error.\n");
        exit(1);
    }
    printf("Writing file...\n");
    for(i=0;i<RES_X * RES_Y;i++) {
        if(region[i] < 0) { // Convergence
            img->imageData[3*i] = 0;
            img->imageData[3*i+1] = 0;
            img->imageData[3*i+2] = 0;
        }
        else {
            size_t j = (size_t) ((COLOR_DENSITY_NUM*region[i]/COLOR_DENSITY_DEN)) % numcolors;
            img->imageData[3*i] = colors[3*j];
            img->imageData[3*i+1] = colors[3*j+1];
            img->imageData[3*i+2] = colors[3*j+2];
        }
    }
    free(region);
    cvSaveImage("foo.png", img, 0);
    cvReleaseImage(&img);
}
