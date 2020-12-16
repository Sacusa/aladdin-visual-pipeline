#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"
#include "harris_non_max.h"

#define CACHELINE_SIZE 64
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

int test_output(OUT_TYPE *max_values) {
    int num_failures = 0;

    for (int i = 0; i < IMG_HEIGHT; i += 3) {
        for (int j = 0; j < IMG_WIDTH; j += 3) {
            for (int ii = 0; ii < 3; ii++) {
                for (int jj = 0; jj < 3; jj++) {
                    if ((ii == 0) && (jj == 0)) {
                        if (max_values[DIM(i+ii,j+jj)] != 255) {
                            num_failures++;
                        }
                    } else {
                        if (max_values[DIM(i+ii,j+jj)] != 0) {
                            num_failures++;
                        }
                    }
                }
            }
        }
    }

    return num_failures;
}

int main() {
    IN_TYPE *harris_response_host = NULL;
    OUT_TYPE *max_values_host = NULL;
    IN_TYPE *harris_response_acc = NULL;
    OUT_TYPE *max_values_acc = NULL;

    const int harris_response_size = 4 * NUM_PIXELS;
    const int max_values_size = 1 * NUM_PIXELS;

    int err = posix_memalign(
        (void**)&harris_response_host, CACHELINE_SIZE, harris_response_size);
    err |= posix_memalign(
        (void**)&max_values_host, CACHELINE_SIZE, max_values_size);
    err |= posix_memalign(
        (void**)&harris_response_acc, CACHELINE_SIZE, harris_response_size);
    err |= posix_memalign(
        (void**)&max_values_acc, CACHELINE_SIZE, max_values_size);
    assert(err == 0 && "Failed to allocate memory!");

    IN_TYPE values[3][3] = {{10, 5, 5}, {5, 5, 5}, {5, 5, 5}};
    for (int i = 0; i < IMG_HEIGHT; i += 3) {
        for (int j = 0; j < IMG_WIDTH; j += 3) {
            for (int ii = 0; ii < 3; ii++) {
                for (int jj = 0; jj < 3; jj++) {
                    harris_response_host[DIM(i+ii, j+jj)] = values[ii][jj];
                }
            }
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "harris_response_host", harris_response_host, harris_response_size);
    mapArrayToAccelerator(0, "max_values_host",      max_values_host,      max_values_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    harris_non_max(harris_response_host, max_values_host, harris_response_acc, max_values_acc,
            harris_response_size, max_values_size);
#endif

    int num_failures = test_output(max_values_host);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    // PROBLEM: the following frees cause an error
    /*
    free(harris_response_host);
    free(max_values_host);
    free(harris_response_acc);
    free(max_values_acc);
    */
    return 0;
}
