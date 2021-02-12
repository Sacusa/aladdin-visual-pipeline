#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"
#include "convolution.h"

#define CACHELINE_SIZE 64

int check_val(int i, int j, TYPE expected, TYPE *output_image) {
    if (output_image[IMG_DIM(i,j)] != expected) {
        printf("FAIL: i=%3d, j=%3d, expected=%3.1f, real=%3.1f\n", i, j, expected,
                output_image[IMG_DIM(i,j)]);
        return 1;
    }
    return 0;
}

int test_output(TYPE *output_image) {
    int num_failures = 0;

    // test corners
    num_failures += check_val(0,   0,   18, output_image);
    num_failures += check_val(0,   127, 18, output_image);
    num_failures += check_val(127, 0,   18, output_image);
    num_failures += check_val(127, 127, 18, output_image);

    num_failures += check_val(0,   1,   24, output_image);
    num_failures += check_val(0,   126, 24, output_image);
    num_failures += check_val(1,   0,   24, output_image);
    num_failures += check_val(1,   127, 24, output_image);
    num_failures += check_val(126, 0,   24, output_image);
    num_failures += check_val(126, 127, 24, output_image);
    num_failures += check_val(127, 1,   24, output_image);
    num_failures += check_val(127, 126, 24, output_image);

    num_failures += check_val(1,   1,   32, output_image);
    num_failures += check_val(1,   126, 32, output_image);
    num_failures += check_val(126, 1,   32, output_image);
    num_failures += check_val(126, 126, 32, output_image);

    // edges
    for (int i = 2; i < (IMG_HEIGHT-2); i++) {
        num_failures += check_val(0,   i,   30, output_image);
        num_failures += check_val(1,   i,   40, output_image);
        num_failures += check_val(126, i,   40, output_image);
        num_failures += check_val(127, i,   30, output_image);

        num_failures += check_val(i,   0,   30, output_image);
        num_failures += check_val(i,   1,   40, output_image);
        num_failures += check_val(i,   126, 40, output_image);
        num_failures += check_val(i,   127, 30, output_image);
    }

    // rest
    for (int i = 2; i < (IMG_HEIGHT-2); i++) {
        for (int j = 2; j < (IMG_WIDTH-2); j++) {
            num_failures += check_val(i, j, 50, output_image);
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

    const int image_size = sizeof(TYPE) * NUM_PIXELS;
    const int kernel_size = sizeof(TYPE) * KERN_WIDTH * KERN_HEIGHT;

    int err = 0;
    err |= posix_memalign((void**)&kernel_host, CACHELINE_SIZE, kernel_size);
    err |= posix_memalign((void**)&input_image_host, CACHELINE_SIZE, image_size);
    err |= posix_memalign((void**)&output_image_host, CACHELINE_SIZE, image_size);
    err |= posix_memalign((void**)&kernel_acc, CACHELINE_SIZE, kernel_size);
    err |= posix_memalign((void**)&input_image_acc, CACHELINE_SIZE,
            IN_SPAD_HEIGHT * IN_SPAD_WIDTH * sizeof(TYPE) * 2);
    err |= posix_memalign((void**)&output_image_acc, CACHELINE_SIZE,
            OUT_SPAD_HEIGHT * OUT_SPAD_WIDTH * sizeof(TYPE) * 2);
    assert(err == 0 && "Failed to allocate memory!");

    for (int i = 0; i < NUM_PIXELS; i++) {
        input_image_host[i] = 1;
        output_image_host[i] = -1;
    }
    for (int i = 0; i < KERN_HEIGHT; i++) {
        for (int j = 0; j < KERN_WIDTH; j++) {
            kernel_host[(i*KERN_WIDTH) + j] = 2;
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "input_image_host",  input_image_host,  image_size);
    mapArrayToAccelerator(0, "kernel_host",       kernel_host,       kernel_size);
    mapArrayToAccelerator(0, "output_image_host", output_image_host, image_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    convolution(input_image_host, kernel_host, output_image_host,
            input_image_acc, kernel_acc, output_image_acc);
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
