#include <cv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <highgui.h>
#include <unistd.h>
#include <errno.h>
#include "mandelbrot.h"

/* Command-line program options */
struct prog_opts {
    int res_x, res_y;  /* Resolution of each output file */

    int width, height; /* Number of horizontal and vertical images,
                          respectively */

    int num_iter;       /* Number of iterations to use */

    float min_x, min_y; /* Coordinates of lower-left and upper-right corners */
    float max_x, max_y; /* of region in complex plane */

    int color_density;

    char *colors_path; /* Path to colors.bin */
};

static void usage(char *name, FILE *out);
static void default_options(struct prog_opts *opts);
static void parse_options(int argc, char **argv, struct prog_opts *opts);

/* Print usage information to the given file pointer */
static void usage(char *name, FILE *out) {
    /* I acknowledge that the string handling here is pretty untidy. Perhaps
     * some day I will come up with a cleaner way to do this, like associating
     * each option with a metavar name and a description which get printed out
     * and neatly formatted at runtime. */

    struct prog_opts defaults;
    default_options(&defaults);
    fprintf(out, "Usage: %s <options>\n"
                 "Valid options are:\n"
                 "  -s <path-to-colors.bin>   Path to packed color data. This "
                 "is the only required option.\n\n"
                 "  -c (x0,y0),(x1,x1)        Set the respective minimum and "
                 "maximum points in the\n"
                 "                            complex plane to be rendered.\n"
                 "                            Default: (%.2f,%.2f),(%.2f,%.2f)\n"
                 "\n"
                 "  -d <density>              Set the integer color density.\n"
                 "                            Default: %d\n\n"
                 "  -g <width>x<height>       Set the number of horizontal and "
                 "vertical images to generate.\n"
                 "                            Default: %dx%d\n\n"
                 "  -h                        Print out this usage "
                 "information.\n\n"
                 "  -i                        Set the number of iterations.\n"
                 "                            Default: %d\n\n"
                 "  -r                        Set the image resolution.\n"
                 "                            Default: %dx%d\n\n",

                 
                 name,
                 defaults.min_x, defaults.min_y,
                 defaults.max_x, defaults.max_y,
                 defaults.color_density,
                 defaults.width, defaults.height,
                 defaults.num_iter,
                 defaults.res_x, defaults.res_y);
}

/* Initializes options struct to some default values */
static void default_options(struct prog_opts *opts) {
    *opts = (struct prog_opts) {
        .res_x = 32768,
        .res_y = 18432,
        
        .width = 16,
        .height = 16,

        .num_iter = 1<<12,

        .min_x = -0.75,
        .min_y = -1.0,
        .max_x = 1.0,
        .max_y = 1.0,

        .color_density = 80000,

        .colors_path = NULL
    };
}

/* Parses command-line options for this program and populates the opts
 * struct with data */
static void parse_options(int argc, char **argv, struct prog_opts *opts) {
    int c;
    int ret;

    /* Flags for our cleanup handler */
    int error = 0;
    int terminate = 0;

    size_t path_length;

    while((c = getopt(argc, argv, "c:d:g:hi:r:s:")) != -1) {
        switch(c) {
            case 'c':
                /* Coordinate information for region in complex plane */
                ret = sscanf(optarg, "(%f,%f),(%f,%f)", &opts->min_x,
                                                        &opts->min_y,
                                                        &opts->max_x,
                                                        &opts->max_y);

                if(ret < 4) {
                    fprintf(stderr, "Error: argument to -c incorrectly "
                            "formatted.\n");
                    usage(argv[0], stderr);

                    error = 1;
                    goto cleanup;
                }

                break;

            case 'd':
                /* Color density */
                ret = sscanf(optarg, "%d", &opts->color_density);
                if(ret < 1) {
                    fprintf(stderr, "Error: expected integer argument to -d\n");
                    usage(argv[0], stderr);

                    error = 1;
                    goto cleanup;
                }

                break;

            case 'g':
                /* Number of horizontal and vertical images */
                ret = sscanf(optarg, "%dx%d", &opts->width, &opts->height);
                
                if(ret < 2) {
                    fprintf(stderr, "Error: argument to -g incorrectly formatted.\n");
                    usage(argv[0], stderr);

                    error = 1;
                    goto cleanup;
                }
                break;
            
            case 'h':
                /* Usage information */
                usage(argv[0], stdout);
                terminate = 1;
                goto cleanup;

                break;

            case 'i':
                /* Number of iterations */
                ret = sscanf(optarg, "%d", &opts->num_iter);

                if(ret < 1) {
                    fprintf(stderr, "Error: expected integer argument to -i\n");
                    usage(argv[0], stderr);

                    error = 1;
                    goto cleanup;
                }

                break;

            case 'r':
                /* Resolution of each image */
                ret = sscanf(optarg, "%dx%d", &opts->res_x, &opts->res_y);

                if(ret < 2) {
                    fprintf(stderr, "Error: argument to -r incorrectly formatted.\n");
                    usage(argv[0], stderr);

                    error = 1;
                    goto cleanup;
                }
                break;

            case 's':
                /* Path to colors bin */
                path_length = strlen(optarg) + 1;
                opts->colors_path = (char *) malloc(path_length);
                if(opts->colors_path == NULL) {
                    fprintf(stderr, "Memory allocation failure: %s\n",
                            strerror(errno));

                    error = 1;
                    goto cleanup;
                }

                strncpy(opts->colors_path, optarg, path_length);

                break;

        }
    }

    if(opts->colors_path == NULL) {
        fprintf(stderr, "You must pass the -s option.\n\n");
        usage(argv[0], stderr);
        error = 1;
    }

cleanup:
    if(error || terminate)
        free(opts->colors_path);
    if(error)
        exit(1);
    else if(terminate)
        exit(0);
}

int main(int argc, char **argv) {
    struct prog_opts opts;
    default_options(&opts);
    parse_options(argc, argv, &opts);
    exit(0);

#if 0
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
#endif
}
