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
#include "grayscale.h"

#define CACHELINE_SIZE 64

int test_output(TYPE *output_image) {
    int num_failures = 0;
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (output_image[i] != 128) {
            num_failures++;
        }
    }
    return num_failures;
}

int main() {
    TYPE *input_image_host = NULL;
    TYPE *output_image_host = NULL;
    TYPE *input_image_acc = NULL;
    TYPE *output_image_acc = NULL;

    const int input_image_size = sizeof(TYPE) * NUM_PIXELS * 3;
    const int output_image_size = sizeof(TYPE) * NUM_PIXELS;
    const int output_spad_size = sizeof(TYPE) * SPAD_WIDTH * SPAD_HEIGHT * 2;

    int err = 0;
    err |= posix_memalign((void**)&input_image_host,  CACHELINE_SIZE, input_image_size);
    err |= posix_memalign((void**)&output_image_host, CACHELINE_SIZE, output_image_size);
    err |= posix_memalign((void**)&input_image_acc,   CACHELINE_SIZE, output_spad_size * 3);
    err |= posix_memalign((void**)&output_image_acc,  CACHELINE_SIZE, output_spad_size);
    assert(err == 0 && "Failed to allocate memory!");

    memset(input_image_host, 128, input_image_size);

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "input_image_host",  input_image_host,  input_image_size);
    mapArrayToAccelerator(0, "output_image_host", output_image_host, output_image_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    grayscale(input_image_host, output_image_host, input_image_acc, output_image_acc);
#endif

    int num_failures = test_output(output_image_host);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    free(input_image_host);
    free(output_image_host);
    free(input_image_acc);
    free(output_image_acc);
    return 0;
}
