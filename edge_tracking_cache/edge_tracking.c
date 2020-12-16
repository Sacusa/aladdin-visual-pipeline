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
    TYPE *input_image = NULL;
    TYPE *output_image = NULL;

    // input image needs to be padded, hence the +2
    const int input_image_size = sizeof(TYPE) * (IMG_WIDTH+2) * (IMG_HEIGHT+2) * 3;
    const int output_image_size = sizeof(TYPE) * NUM_PIXELS;

    int err = posix_memalign(
        (void**)&input_image, CACHELINE_SIZE, input_image_size);
    err |= posix_memalign(
        (void**)&output_image, CACHELINE_SIZE, output_image_size);
    assert(err == 0 && "Failed to allocate memory!");

    // the value in the padding doesn't matter as long as it is not STRONG
    memset(input_image, 128, input_image_size);

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "input_image",  input_image,  input_image_size);
    mapArrayToAccelerator(0, "output_image", output_image, output_image_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    edge_tracking(input_image, 75, 150, output_image);
#endif

    int num_failures = test_output(output_image);
    if (num_failures) {
        fprintf(stdout, "Test failed with %d errors.\n", num_failures);
    }
    else {
        fprintf(stdout, "Test passed!\n");
    }

    free(input_image);
    free(output_image);
    return 0;
}
