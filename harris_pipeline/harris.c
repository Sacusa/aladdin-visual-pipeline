#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"

#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define DIM(x,y) (((x)*IMG_WIDTH) + (y))
#define CACHELINE_SIZE 64

#include "elem_matrix.h"
#include "grayscale.h"
#include "harris_non_max.h"
#include "isp.h"

enum acc_ids {
    ACC_ISP = 0,
    ACC_GRAYSCALE,
    ACC_NON_MAX,
    ACC_ELEM_MATRIX
};

void run_isp(uint8_t *input_img, uint8_t **ret_output_img)
{
    uint8_t *output_img = NULL;
    int input_img_size = (IMG_WIDTH+2) * (IMG_HEIGHT+2);
    int output_img_size = NUM_PIXELS * 3;

    int err = posix_memalign(
        (void**)&output_img, CACHELINE_SIZE, output_img_size);
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

    int err = posix_memalign(
        (void**)&output_img, CACHELINE_SIZE, NUM_PIXELS);
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

void spatial_derivative_calc(uint8_t *input_img, float **ret_I_x, float **ret_I_y)
{
    float *I_x = NULL, *I_y = NULL;
    int I_size = NUM_PIXELS * sizeof(float);

    int err = posix_memalign(
        (void**)&I_x, CACHELINE_SIZE, I_size);
    err |= posix_memalign(
        (void**)&I_y, CACHELINE_SIZE, I_size);
    assert(err == 0 && "Failed to allocate memory");

    // TODO: Convolution
    // the following is just a placeholder
    memset(I_x, 0, I_size);
    memset(I_y, 0, I_size);

    *ret_I_x = I_x;
    *ret_I_y = I_y;
}

void structure_tensor_setup(float *I_x, float *I_y,
        float **ret_I_xx, float **ret_I_xy, float **ret_I_yy)
{
    float *I_xx = NULL, *I_xy = NULL, *I_yy = NULL;
    float *I_xx_g = NULL, *I_xy_g = NULL, *I_yy_g = NULL;
    int I_size = NUM_PIXELS * sizeof(float);

    int err = posix_memalign(
        (void**)&I_xx, CACHELINE_SIZE, I_size);
    err |= posix_memalign(
        (void**)&I_xy, CACHELINE_SIZE, I_size);
    err |= posix_memalign(
        (void**)&I_yy, CACHELINE_SIZE, I_size);
    assert(err == 0 && "Failed to allocate memory");

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_x,  I_size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", I_xx, I_size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_y,  I_size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", I_yy, I_size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_x,  I_size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   I_y,  I_size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", I_xy, I_size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);
#else
    elem_matrix(I_x, NULL, I_xx, 0, SQR);
    elem_matrix(I_y, NULL, I_yy, 0, SQR);
    elem_matrix(I_x, I_y,  I_xy, 0, MUL);
#endif

    // TODO: Convolution
    // the following is just a placeholder
    I_xx_g = I_xx;
    I_yy_g = I_yy;
    I_xy_g = I_xy;

    *ret_I_xx = I_xx_g;
    *ret_I_yy = I_yy_g;
    *ret_I_xy = I_xy_g;
}

void harris_response_calc(float *I_xx, float *I_xy, float *I_yy,
        float **ret_harris_response)
{
    float *detA1 = NULL, *detA2 = NULL, *detA = NULL;
    float *traceA = NULL;
    float *hr1 = NULL, *hr2 = NULL, *hr = NULL;
    float k = 0.05;
    int size = NUM_PIXELS * sizeof(float);

    int err = posix_memalign(
        (void**)&detA1, CACHELINE_SIZE, size);
    err |= posix_memalign(
        (void**)&detA2, CACHELINE_SIZE, size);
    err |= posix_memalign(
        (void**)&detA, CACHELINE_SIZE, size);

    err |= posix_memalign(
        (void**)&traceA, CACHELINE_SIZE, size);

    err |= posix_memalign(
        (void**)&hr1, CACHELINE_SIZE, size);
    err |= posix_memalign(
        (void**)&hr2, CACHELINE_SIZE, size);
    err |= posix_memalign(
        (void**)&hr, CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_xx,  size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   I_yy,  size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", detA1, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_xy,  size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", detA2, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   detA1, size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   detA2, size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", detA,  size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   I_xx,   size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   I_yy,   size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", traceA, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   traceA, size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", hr1,    size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   hr1, size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   &k,  sizeof(float));
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", hr2, size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);

    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg1",   detA, size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "arg2",   hr2,  size);
    mapArrayToAccelerator(ACC_ELEM_MATRIX, "result", hr,   size);
    invokeAcceleratorAndBlock(ACC_ELEM_MATRIX);
#else
    elem_matrix(I_xx,  I_yy,  detA1, 0, MUL);
    elem_matrix(I_xy,  NULL,  detA2, 0, SQR);
    elem_matrix(detA1, detA2, detA,  0, SUB);

    elem_matrix(I_xx, I_yy, traceA, 0, ADD);

    elem_matrix(traceA, NULL, hr1, 0, SQR);
    elem_matrix(hr1,    &k,   hr2, 1, MUL);
    elem_matrix(detA,   hr2,  hr,  0, SUB);
#endif

    free(detA1);
    free(detA2);
    free(detA);
    free(traceA);
    free(hr1);
    free(hr2);

    *ret_harris_response = hr;
}

void non_max_suppression(float *harris_response, float **ret_max_values)
{
    float *hr_padded = NULL;
    uint8_t *mv_padded = NULL;
    uint8_t *max_values = NULL;

    int hr_size = (IMG_WIDTH+1) * (IMG_HEIGHT+1) * sizeof(float);
    int mv_size = (IMG_WIDTH+1) * (IMG_HEIGHT+1) * sizeof(uint8_t);

    int err = posix_memalign(
        (void**)&hr_padded, CACHELINE_SIZE, hr_size);
    err |= posix_memalign(
        (void**)&mv_padded, CACHELINE_SIZE, mv_size);
    err |= posix_memalign(
        (void**)&max_values, CACHELINE_SIZE, NUM_PIXELS * sizeof(uint8_t));
    assert(err == 0 && "Failed to allocate memory");

    memset(hr_padded, 0, hr_size);
    for (int i = 0; i < IMG_HEIGHT; i++) {
        for (int j = 0; j < IMG_WIDTH; j++) {
            hr_padded[DIM(i,j)] = harris_response[DIM(i,j)];
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(ACC_NON_MAX, "harris_response_host", hr_padded, hr_size);
    mapArrayToAccelerator(ACC_NON_MAX, "max_values_host",      mv_padded, mv_size);
    invokeAcceleratorAndBlock(ACC_NON_MAX);
#else
    float *hr_padded_acc = NULL;
    uint8_t *mv_padded_acc = NULL;

    err = posix_memalign(
        (void**)&hr_padded_acc, CACHELINE_SIZE, hr_size);
    err |= posix_memalign(
        (void**)&mv_padded_acc, CACHELINE_SIZE, mv_size);
    assert(err == 0 && "Failed to allocate memory");

    harris_non_max(hr_padded, mv_padded, hr_padded_acc, mv_padded_acc, hr_size, mv_size);

    // TODO: harris response is doing something weird...
    free(hr_padded_acc);
    free(mv_padded_acc);
#endif

    for (int i = 0; i < IMG_HEIGHT; i++) {
        for (int j = 0; j < IMG_WIDTH; j++) {
            max_values[DIM(i,j)] = mv_padded[DIM(i,j)];
        }
    }
    *ret_max_values = max_values;

    free(hr_padded);
    free(mv_padded);
}

int main()
{
    uint8_t *raw_img = NULL;
    uint8_t *isp_img = NULL;
    uint8_t *grayscale_img = NULL;
    float *I_x = NULL, *I_y = NULL;
    float *I_xx = NULL, *I_xy = NULL, *I_yy = NULL;
    float *harris_response = NULL;
    float *max_values = NULL;
    int err;

    int raw_img_size = (IMG_WIDTH+2) * (IMG_HEIGHT+2);
    err = posix_memalign(
            (void**)&raw_img, CACHELINE_SIZE, raw_img_size);
    assert(err == 0 && "Failed to allocate memory");
    memset(raw_img, 128, NUM_PIXELS);

    // Step 0: Run raw image through ISP
    run_isp(raw_img, &isp_img);
    free(raw_img);

    // Step 1: Convert image to grayscale
    convert_to_grayscale(isp_img, &grayscale_img);
    free(isp_img);

    // Step 2: Spatial derivative calculation
    spatial_derivative_calc(grayscale_img, &I_x, &I_y);
    free(grayscale_img);

    // Step 3: Structure tensor setup
    structure_tensor_setup(I_x, I_y, &I_xx, &I_xy, &I_yy);
    free(I_x);
    free(I_y);

    // Step 4: Harris response calculation
    harris_response_calc(I_xx, I_xy, I_yy, &harris_response);
    free(I_xx);
    free(I_xy);
    free(I_yy);

    // Step 5: Non-max suppression
    non_max_suppression(harris_response, &max_values);
    free(harris_response);

    return 0;
}
