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
    if (output_image[DIM(i,j)] != expected) {
        printf("FAIL: i=%d, j=%d, expected=%f, real=%f\n", i, j, expected,
                output_image[DIM(i,j)]);
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
    TYPE *input_image = NULL;
    TYPE *kernel = NULL;
    TYPE *output_image = NULL;

    const int image_size = sizeof(TYPE) * NUM_PIXELS;
    const int kernel_size = sizeof(TYPE) * 25;

    int err = posix_memalign((void**)&input_image, CACHELINE_SIZE, image_size);
    err |= posix_memalign((void**)&kernel, CACHELINE_SIZE, kernel_size);
    err |= posix_memalign((void**)&output_image, CACHELINE_SIZE, image_size);
    assert(err == 0 && "Failed to allocate memory!");

    // caution: bad code below
    for (int i = 0; i < NUM_PIXELS; i++) {
        input_image[i] = 1;
    }
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            kernel[(i*5) + j] = 2;
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "input_image",  input_image,  image_size);
    mapArrayToAccelerator(0, "kernel",       kernel,       kernel_size);
    mapArrayToAccelerator(0, "output_image", output_image, image_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    convolution(input_image, kernel, output_image, 5, 5);
#endif

    int num_failures = test_output(output_image);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    free(input_image);
    free(kernel);
    free(output_image);
    return 0;
}
