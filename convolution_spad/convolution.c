/* Test stores performed in the kernel.
 *
 * The values stored should be able to be loaded by the CPU after the kernel is
 * finished.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"
#include "convolution.h"

#define CACHELINE_SIZE 64

int check_val(int i, int j, TYPE expected, TYPE *output_image) {
    if (output_image[OUT_DIM(i,j)] != expected) {
        printf("FAIL: i=%d, j=%d, expected=%f, real=%f\n", i, j, expected,
                output_image[OUT_DIM(i,j)]);
        return 1;
    }
    return 0;
}

int test_output(TYPE *output_image) {
    int num_failures = 0;

    // test corners
    num_failures += check_val(0,   0,   8, output_image);
    num_failures += check_val(0,   127, 8, output_image);
    num_failures += check_val(127, 0,   8, output_image);
    num_failures += check_val(127, 127, 8, output_image);

    // edges
    for (int i = 1; i < (IMG_HEIGHT-1); i++) {
        num_failures += check_val(0,   i, 12, output_image);
        num_failures += check_val(127, i, 12, output_image);
        num_failures += check_val(i,   0, 12, output_image);
        num_failures += check_val(i, 127, 12, output_image);
    }

    // rest
    for (int i = 1; i < (IMG_HEIGHT-1); i++) {
        for (int j = 1; j < (IMG_WIDTH-1); j++) {
            num_failures += check_val(i, j, 18, output_image);
        }
    }

    return num_failures;
}

int main() {
    TYPE *input_image_host = NULL;
    TYPE *kernel_host = NULL;
    TYPE *output_image_host = NULL;
    TYPE *input_image_acc = NULL;
    TYPE *kernel_acc = NULL;
    TYPE *output_image_acc = NULL;

    const int input_image_size = sizeof(TYPE) * (IMG_HEIGHT+2) * (IMG_WIDTH+2);
    const int kernel_size = sizeof(TYPE) * 9;
    const int output_image_size = sizeof(TYPE) * NUM_PIXELS;

    int err = posix_memalign((void**)&input_image_host, CACHELINE_SIZE, input_image_size);
    err |= posix_memalign((void**)&kernel_host, CACHELINE_SIZE, kernel_size);
    err |= posix_memalign((void**)&output_image_host, CACHELINE_SIZE, output_image_size);
    err |= posix_memalign((void**)&input_image_acc, CACHELINE_SIZE, input_image_size);
    err |= posix_memalign((void**)&kernel_acc, CACHELINE_SIZE, kernel_size);
    err |= posix_memalign((void**)&output_image_acc, CACHELINE_SIZE, output_image_size);
    assert(err == 0 && "Failed to allocate memory!");

    // caution: bad code below
    memset(input_image_host, 0, input_image_size);
    int in_width = 130;
    for (int i = 1; i < (in_width-1); i++) {
        for (int j = 1; j < (in_width-1); j++) {
            input_image_host[IN_DIM(i, j)] = 1;
        }
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            kernel_host[(i*3) + j] = 2;
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "input_image_host",  input_image_host,  input_image_size);
    mapArrayToAccelerator(0, "kernel_host",       kernel_host,       kernel_size);
    mapArrayToAccelerator(0, "output_image_host", output_image_host, output_image_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    convolution(input_image_host, kernel_host, output_image_host,
            input_image_acc, kernel_acc, output_image_acc, 3, 3);
#endif

    int num_failures = test_output(output_image_host);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    free(input_image_host);
    free(kernel_host);
    free(output_image_host);
    free(input_image_acc);
    free(kernel_acc);
    free(output_image_acc);
    return 0;
}