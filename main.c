#include <stdio.h>
#include <stdlib.h>
#include <cv.h>
#include <highgui.h>
#include "mandelbrot.h"

#define RES_X 32768
#define RES_Y 18432
#define NUM_ITER 1<<12

#define COLOR_DENSITY_NUM 80000
#define COLOR_DENSITY_DEN 1

int main(int argc, char **argv) {
    IplImage *img, *scaled;
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
    scaled = cvCreateImage(cvSize(8192, 4608), IPL_DEPTH_8U, 3);
    if(!img | !scaled) {
        fprintf(stderr, "Memory allocation error.\n");
        exit(1);
    }

    float width = 3.2/256;
    float height = 1.8/256;
    char filename[128];
    int imageno=0;
    for(float min_y=-0.45; min_y < -0.3375; min_y += height) {
        for(float min_x=-0.8; min_x < -0.6; min_x += width) {
            region = (float*) create_mandelbrot(RES_X, RES_Y, min_x, min_y, min_x + width, min_y + height, NUM_ITER);
            if(!region) {
                fprintf(stderr, "Memory allocation error.\n");
                exit(1);
            }
            int converged = 0;
            int diverged = 0;
            for(i=0;i<RES_X * RES_Y;i++) {
                if(region[i] < 0) { // Convergence
                    img->imageData[3*i] = 0;
                    img->imageData[3*i+1] = 0;
                    img->imageData[3*i+2] = 0;
                    converged++;
                }
                else {
                    size_t j = (size_t) ((COLOR_DENSITY_NUM*region[i]/COLOR_DENSITY_DEN)) % numcolors;
                    img->imageData[3*i] = colors[3*j];
                    img->imageData[3*i+1] = colors[3*j+1];
                    img->imageData[3*i+2] = colors[3*j+2];
                    diverged++;
                }
            }
            free(region);

            // Only write out interesting parts of the Mandelbrot set
            if((diverged != RES_X * RES_Y) && (converged != RES_X * RES_Y)) {
                snprintf(filename, 128, "image-%d.png", imageno);
                cvResize(img, scaled, CV_INTER_AREA);
                cvSaveImage(filename, scaled, 0);
                printf("File %s written. Starts at coordinates (%f, %f)\n", filename, min_x, min_y);
            }
            imageno++;
        }
    }
    cvReleaseImage(&scaled);
    cvReleaseImage(&img);
}
