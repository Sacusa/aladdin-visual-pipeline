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
#include "harris_non_max.h"

#define CACHELINE_SIZE 64
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

int test_output(OUT_TYPE *max_values) {
    int num_failures = 0;

    for (int i = 0; i < IMG_HEIGHT; i += 3) {
        for (int j = 0; j < IMG_WIDTH; j += 3) {
            for (int ii = 0; ii < 3; ii++) {
                for (int jj = 0; jj < 3; jj++) {
                    int x = i+ii, y = j+jj;

                    if ((x < IMG_HEIGHT) && (y < IMG_WIDTH)) {
                        if ((ii == 0) && (jj == 0)) {
                            if (max_values[DIM(x,y)] != 255) {
                                num_failures++;
                            }
                        } else {
                            if (max_values[DIM(x,y)] != 0) {
                                num_failures++;
                            }
                        }
                    }
                }
            }
        }
    }

    return num_failures;
}

int main() {
    IN_TYPE *harris_response = NULL;
    OUT_TYPE *max_values = NULL;

    const int harris_response_size = sizeof(IN_TYPE) * NUM_PIXELS;
    const int max_values_size = sizeof(OUT_TYPE) * NUM_PIXELS;

    int err = 0;
    err |= posix_memalign((void**)&harris_response, CACHELINE_SIZE, harris_response_size);
    err |= posix_memalign((void**)&max_values,      CACHELINE_SIZE, max_values_size);
    assert(err == 0 && "Failed to allocate memory!");

    IN_TYPE values[3][3] = {{10, 5, 5}, {5, 5, 5}, {5, 5, 5}};
    for (int i = 0; i < IMG_HEIGHT; i += 3) {
        for (int j = 0; j < IMG_WIDTH; j += 3) {
            for (int ii = 0; ii < 3; ii++) {
                for (int jj = 0; jj < 3; jj++) {
                    int x = i+ii, y = j+jj;

                    if ((x < IMG_HEIGHT) && (y < IMG_WIDTH)) {
                        harris_response[DIM(x, y)] = values[ii][jj];
                    }
                }
            }
        }
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "harris_response", harris_response, harris_response_size);
    mapArrayToAccelerator(0, "max_values",      max_values,      max_values_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    harris_non_max(harris_response, max_values);
#endif

    int num_failures = test_output(max_values);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    free(harris_response);
    free(max_values);
    return 0;
}
