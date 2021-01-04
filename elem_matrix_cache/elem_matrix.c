#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"
#include "elem_matrix.h"

#define CACHELINE_SIZE 64

int main() {
    TYPE *mat_arg1 = NULL;
    TYPE *mat_arg2 = NULL;
    TYPE *mat_res  = NULL;

    const int mat_size = sizeof(TYPE) * NUM_ELEMS;

    int err = posix_memalign(
        (void**)&mat_arg1, CACHELINE_SIZE, mat_size);
    err |= posix_memalign(
        (void**)&mat_arg2, CACHELINE_SIZE, mat_size);
    err |= posix_memalign(
        (void**)&mat_res,  CACHELINE_SIZE, mat_size);
    assert(err == 0 && "Failed to allocate memory!");

    for (int i = 0; i < NUM_ELEMS; i++) {
        mat_arg1[i] = 1;
        mat_arg2[i] = 1;
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "arg1",   mat_arg1, mat_size);
    mapArrayToAccelerator(0, "arg2",   mat_arg2, mat_size);
    mapArrayToAccelerator(0, "result", mat_res,  mat_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    elem_matrix(mat_arg1, mat_arg2, mat_res, 0, ATAN2);
#endif

    int num_errors = 0;
    for (int i = 0; i < NUM_ELEMS; i++) {
        if (fabs(mat_res[i] - 0.785398) > 0.0001) {  // ATAN2
        //if (fabs(mat_res[i] - 1.0) > 0.0001) {  // MUL
            num_errors++;
        }
    }
    if (num_errors) {
        printf("Number of errors = %d\n", num_errors);
    }
    else {
        printf("No errors!\n");
    }

    free(mat_arg1);
    free(mat_arg2);
    free(mat_res);
    return 0;
}
