#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"
#include "canny_non_max.h"

#define CACHELINE_SIZE 64
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

int test_output(OUT_TYPE *result) {
    int num_failures = 0;

    for (int i = 0; i < NUM_PIXELS; i++) {
        if (result[i] != 128) {
            num_failures++;
        }
    }

    return num_failures;
}

int main() {
    HYPO_TYPE *hypotenuse;
    THTA_TYPE *theta;
    OUT_TYPE *result;

    const int hypotenuse_size = sizeof(HYPO_TYPE) * (IMG_WIDTH+2) * (IMG_HEIGHT+2);
    const int theta_size = sizeof(THTA_TYPE) * NUM_PIXELS;
    const int result_size = sizeof(OUT_TYPE) * NUM_PIXELS;

    int err = posix_memalign(
        (void**)&hypotenuse, CACHELINE_SIZE, hypotenuse_size);
    err |= posix_memalign(
        (void**)&theta, CACHELINE_SIZE, theta_size);
    err |= posix_memalign(
        (void**)&result, CACHELINE_SIZE, result_size);
    assert(err == 0 && "Failed to allocate memory!");

    for (int i = 0; i < IMG_HEIGHT; i++) {
        for (int j = 0; j < IMG_WIDTH; j++) {
            hypotenuse[HYPO_DIM(i,j)] = 128;
            theta[DIM(i,j)] = 0.7854;
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "hypotenuse", hypotenuse, hypotenuse_size);
    mapArrayToAccelerator(0, "theta",      theta,      theta_size);
    mapArrayToAccelerator(0, "result",     result,     result_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    canny_non_max(hypotenuse, theta, result);
#endif

    int num_failures = test_output(result);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    free(hypotenuse);
    free(theta);
    free(result);
    return 0;
}
