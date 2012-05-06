#include <stdio.h>
#define ABS(X) X < 0 ? -X : X


extern "C" float *create_mandelbrot(int res_x, int res_y, float min_x, float min_y,
        float max_x, float max_y, int iter);

__global__ void mandelbrot(float *region, int offset, int2 res, float4 boundary, int iter);


/* We define a maximum number of iterations the kernel can do, so that
 * it isn't rudely interrupted by the watchdog timer */
static const long max_iterations = 1L<<30;
static const int block_size = 128;

extern "C" float *create_mandelbrot(int res_x, int res_y, float min_x, float min_y,
        float max_x, float max_y, int iter) {

    float *region, *d_region;
    dim3 threadsPerBlock(block_size);
    dim3 blocks;
    float4 boundary;

    int size = res_x * res_y;
    size_t host_bytes = (size_t) size * sizeof(float);
    size_t job_bytes;
    size_t jobsize;
    cudaStream_t copy_stream; // Stream for copying between host and device
    if(cudaStreamCreate(&copy_stream) != cudaSuccess)
        return NULL;

    region = (float*) malloc(host_bytes);
    if(!region)
        goto cleanup3;

    /* Find some number of pixels that is a power of two, evenly divides up
     * the work, and causes the kernel to compute at most max_iterations */

    jobsize = block_size;
    for(;!(size & (jobsize - 1)) && (jobsize * iter <= max_iterations); jobsize <<= 1);
    jobsize >>= 1;
    if(jobsize < block_size)
        goto cleanup3;

    fprintf(stderr, "%s: %d. %s: %d.\n", "Number of pixels", size,
            "Number of pixels per job", jobsize);

    blocks = dim3(jobsize / threadsPerBlock.x);
    job_bytes = jobsize * sizeof(float);

    boundary.x = min_x;
    boundary.y = min_y;
    boundary.z = max_x;
    boundary.w = max_y;

    if(cudaMalloc(&d_region, job_bytes) != cudaSuccess)
        goto cleanup2;


    for(int start = 0; start <= (size - jobsize); start += jobsize) {
        mandelbrot<<<blocks, threadsPerBlock>>>(d_region, start, make_int2(res_x, res_y), boundary, iter);
        if(cudaMemcpy(&region[start], d_region, job_bytes, cudaMemcpyDeviceToHost) != cudaSuccess)
            goto cleanup1;
    }

    cudaFree(d_region);
    cudaStreamDestroy(copy_stream);
    return region;

cleanup1:
    cudaFree(d_region);
cleanup2:
    free(region);
cleanup3:
    cudaStreamDestroy(copy_stream);
    return NULL;
}


/* Calculates the mandelbrot set and stores the results in region, which
 * should be of length gridDim.x * blockDim.x * gridDim.y * blockDim.y
 * The float4 boundary specifies the region of the complex plane to test
 * for divergence, with the x, y, z, and w components representing the minimum
 * x, minimum y, maximum x, and maximum y values of the rectangular region. */
__global__ void mandelbrot(float *region, int offset, int2 res, float4 boundary, int iter) {
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int pixel = index + offset;
    int pixel_x = pixel % res.x;
    int pixel_y = pixel / res.x;

    double2 c, z;
    int i;

    c.x = ((boundary.z - boundary.x) * (((double) pixel_x) + 0.5))/(res.x) + boundary.x;
    c.y = ((boundary.y - boundary.w) * (((double) pixel_y) + 0.5))/(res.y) + boundary.w;
    z = c;

    for(i=1;(i<iter) && (z.x*z.x+z.y*z.y <= 4);i++)
        z = make_double2(z.x * z.x - z.y*z.y + c.x,
                         2*z.x*z.y + c.y);

    region[index] = (i >= iter) ? (-1) : i + 2 - log2f(log2f(z.x*z.x+z.y*z.y));
}
