#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"

#define PI 3.141592653589793238462643
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define DIM(x,y) (((x)*IMG_WIDTH) + (y))
#define CACHELINE_SIZE 64

#include "canny_non_max.h"
#include "edge_tracking.h"
#include "elem_matrix.h"
#include "grayscale.h"
#include "isp.h"

enum acc_ids {
    ACC_ISP = 0,
    ACC_GRAYSCALE,
    ACC_ELEM_MATRIX,
    ACC_NON_MAX,
    ACC_EDGE_TRACKING
};

void run_isp(uint8_t *input_img, uint8_t **ret_output_img)
{
    uint8_t *output_img = NULL;
    int input_img_size = (IMG_WIDTH+2) * (IMG_HEIGHT+2);
    int output_img_size = NUM_PIXELS * 3;

    int err = posix_memalign((void**)&output_img, CACHELINE_SIZE, output_img_size);
    assert(err == 0 && "Failed to allocate memory");

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_ISP, "input_image_host",  input_img,  input_img_size);
    mapArrayToAccelerator(ACC_ISP, "output_image_host", output_img, output_img_size);
    invokeAcceleratorAndBlock(ACC_ISP);
#else
    uint8_t *input_img_acc = NULL, *output_img_acc = NULL;

    err = posix_memalign(
        (void**)&input_img_acc, CACHELINE_SIZE, input_img_size);
    err |= posix_memalign(
        (void**)&output_img_acc, CACHELINE_SIZE, output_img_size);
    assert(err == 0 && "Failed to allocate memory");

    isp(input_img, output_img, input_img_acc, output_img_acc, input_img_size, output_img_size);

    free(input_img_acc);
    free(output_img_acc);
#endif

    *ret_output_img = output_img;
}

void convert_to_grayscale(uint8_t *input_img, uint8_t **ret_output_img)
{
    uint8_t *output_img = NULL;

    int err = posix_memalign((void**)&output_img, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_GRAYSCALE, "input_image",  input_img,  NUM_PIXELS);
    mapArrayToAccelerator(ACC_GRAYSCALE, "output_image", output_img, NUM_PIXELS);
    invokeAcceleratorAndBlock(ACC_GRAYSCALE);
#else
    grayscale(input_img, output_img);
#endif

    *ret_output_img = output_img;
}

void noise_reduction(uint8_t *input_img, float **ret_output_img)
{
    float *output_img = NULL;
    int size = NUM_PIXELS * sizeof(float);

    int err = posix_memalign((void**)&output_img, CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

    // TODO: Convolution
    for (int i = 0; i < NUM_PIXELS; i++) {
        output_img[i] = (float) input_img[i];
    }

    *ret_output_img = output_img;
}

void gradient_calculation(float *input_img, float **ret_gradient, float **ret_theta)
{
    float *I_x = NULL, *I_y = NULL;
    float *I_xx = NULL, *I_yy = NULL, *I_xx_yy = NULL;
    float *gradient = NULL, *theta = NULL;
    int size = NUM_PIXELS * sizeof(float);

    int err = posix_memalign((void**)&I_x, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&I_y, CACHELINE_SIZE, size);

    err |= posix_memalign((void**)&I_xx, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&I_yy, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&I_xx_yy, CACHELINE_SIZE, size);

    err |= posix_memalign((void**)&gradient, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&theta, CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

    // TODO: Convolution
    memcpy(I_x, input_img, size);
    memcpy(I_y, input_img, size);

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_x,  size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", I_xx, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_y,  size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", I_yy, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_xx,    size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   I_yy,    size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", I_xx_yy, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_xx_yy,  size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", gradient, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_y,   size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   I_x,   size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", theta, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);
#else
    elem_matrix(I_x,  NULL, I_xx,    0, SQR);
    elem_matrix(I_y,  NULL, I_yy,    0, SQR);
    elem_matrix(I_xx, I_yy, I_xx_yy, 0, ADD);

    elem_matrix(I_xx_yy, NULL, gradient, 0, SQRT);
    elem_matrix(I_y, I_x, theta, 0, ATAN2);
#endif

    free(I_x);
    free(I_y);
    free(I_xx);
    free(I_yy);
    free(I_xx_yy);

    *ret_gradient = gradient;
    *ret_theta = theta;
}

void non_max_suppression(float *gradient, float *theta, uint8_t **ret_max_values)
{
    uint8_t *max_values = NULL;
    int size = NUM_PIXELS * sizeof(float);

    int err = posix_memalign((void**)&max_values, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_NON_MAX, "hypotenuse", gradient,   size);
    mapArrayToAccelerator(ACC_NON_MAX, "theta",      theta,      size);
    mapArrayToAccelerator(ACC_NON_MAX, "result",     max_values, NUM_PIXELS);
    invokeAcceleratorAndBlock(ACC_NON_MAX);
#else
    canny_non_max(gradient, theta, max_values);
#endif

    *ret_max_values = max_values;
}


void thr_and_edge_tracking(uint8_t *input_img, uint8_t **ret_output_img)
{
    uint8_t *output_img = NULL;

    int err = posix_memalign((void**)&output_img, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_EDGE_TRACKING, "input_image",  input_img,  NUM_PIXELS);
    mapArrayToAccelerator(ACC_EDGE_TRACKING, "output_image", output_img, NUM_PIXELS);
    invokeAcceleratorAndBlock(ACC_EDGE_TRACKING);
#else
    // the thresholds below are arbitrary right now
    edge_tracking(input_img, 75, 150, output_img);
#endif

    *ret_output_img = output_img;
}

int main()
{
    uint8_t *raw_img = NULL;
    uint8_t *isp_img = NULL;
    uint8_t *grayscale_img = NULL;
    float *clean_img = NULL;
    float *gradient = NULL, *theta = NULL;
    uint8_t *max_values = NULL;
    uint8_t *final_img = NULL;
    int err;

    int raw_img_size = (IMG_WIDTH+2) * (IMG_HEIGHT+2);
    err = posix_memalign((void**)&raw_img, CACHELINE_SIZE, raw_img_size);
    assert(err == 0 && "Failed to allocate memory");
    memset(raw_img, 128, NUM_PIXELS);

    // Step 0: Run raw image through ISP
    run_isp(raw_img, &isp_img);
    free(raw_img);

    // Step 1: Convert image to grayscale
    convert_to_grayscale(isp_img, &grayscale_img);
    free(isp_img);

    // Step 2: Noise reduction
    noise_reduction(grayscale_img, &clean_img);
    free(grayscale_img);

    // Step 3: Gradient calculation
    gradient_calculation(clean_img, &gradient, &theta);
    free(clean_img);

    // Step 4: Non-maximum suppression
    non_max_suppression(gradient, theta, &max_values);
    free(gradient);
    free(theta);

    // Steps 5 and 6: Double threshold and edge tracking by hysteresis
    thr_and_edge_tracking(max_values, &final_img);
    free(max_values);

    return 0;
}
