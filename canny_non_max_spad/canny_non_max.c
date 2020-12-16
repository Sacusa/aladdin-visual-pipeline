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
    HYPO_TYPE *hypotenuse_host;
    THTA_TYPE *theta_host;
    OUT_TYPE *result_host;
    HYPO_TYPE *hypotenuse_acc;
    THTA_TYPE *theta_acc;
    OUT_TYPE *result_acc;

    const int hypotenuse_size = sizeof(HYPO_TYPE) * (IMG_WIDTH+2) * (IMG_HEIGHT+2);
    const int theta_size = sizeof(THTA_TYPE) * NUM_PIXELS;
    const int result_size = sizeof(OUT_TYPE) * NUM_PIXELS;

    int err = posix_memalign(
        (void**)&hypotenuse_host, CACHELINE_SIZE, hypotenuse_size);
    err |= posix_memalign(
        (void**)&theta_host, CACHELINE_SIZE, theta_size);
    err |= posix_memalign(
        (void**)&result_host, CACHELINE_SIZE, result_size);
    err |= posix_memalign(
        (void**)&hypotenuse_acc, CACHELINE_SIZE, hypotenuse_size);
    err |= posix_memalign(
        (void**)&theta_acc, CACHELINE_SIZE, theta_size);
    err |= posix_memalign(
        (void**)&result_acc, CACHELINE_SIZE, result_size);
    assert(err == 0 && "Failed to allocate memory!");

    for (int i = 0; i < IMG_HEIGHT; i++) {
        for (int j = 0; j < IMG_WIDTH; j++) {
            hypotenuse_host[HYPO_DIM(i,j)] = 128;
            theta_host[DIM(i,j)] = 0.7854;
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "hypotenuse_host", hypotenuse_host, hypotenuse_size);
    mapArrayToAccelerator(0, "theta_host",      theta_host,      theta_size);
    mapArrayToAccelerator(0, "result_host",     result_host,     result_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    canny_non_max(hypotenuse_host, theta_host, result_host,
            hypotenuse_acc, theta_acc, result_acc,
            hypotenuse_size, theta_size, result_size);
#endif

    int num_failures = test_output(result_host);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    free(hypotenuse_host);
    free(theta_host);
    free(result_host);
    free(hypotenuse_acc);
    free(theta_acc);
    free(result_acc);
    return 0;
}
