#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"
#include "edge_tracking.h"

#define CACHELINE_SIZE 64
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

int test_output(TYPE *output_image) {
    int num_failures = 0;
    for (int i = 0; i < NUM_PIXELS; i++) {
        if (output_image[i] != 0) {
            printf("%d: %d\n", i, output_image[i]);
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

    // input image needs to be padded, hence the +2
    const int input_image_size = sizeof(TYPE) * (IMG_WIDTH+2) * (IMG_HEIGHT+2);
    const int output_image_size = sizeof(TYPE) * NUM_PIXELS;

    int err = posix_memalign(
        (void**)&input_image_host, CACHELINE_SIZE, input_image_size);
    err |= posix_memalign(
        (void**)&output_image_host, CACHELINE_SIZE, output_image_size);
    err |= posix_memalign(
        (void**)&input_image_acc, CACHELINE_SIZE, input_image_size);
    err |= posix_memalign(
        (void**)&output_image_acc, CACHELINE_SIZE, output_image_size);
    assert(err == 0 && "Failed to allocate memory!");

    // the value in the padding doesn't matter as long as it is not STRONG
    memset(input_image_host, 128, input_image_size);

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "input_image_host",  input_image_host,  input_image_size);
    mapArrayToAccelerator(0, "output_image_host", output_image_host, output_image_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    edge_tracking(input_image_host, input_image_acc, 75, 150,
            output_image_host, output_image_acc,
            input_image_size, output_image_size);
#endif

    int num_failures = test_output(output_image_host);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
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
