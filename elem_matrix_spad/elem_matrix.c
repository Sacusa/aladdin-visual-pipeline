/* Test stores performed in the kernel.
 *
 * The values stored should be able to be loaded by the CPU after the kernel is
 * finished.
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem5_harness.h"
#include "elem_matrix.h"

#define CACHELINE_SIZE 64

int main() {
    TYPE *mat_arg1_host = NULL;
    TYPE *mat_arg2_host = NULL;
    TYPE *mat_res_host  = NULL;
    TYPE *mat_arg1_acc = NULL;
    TYPE *mat_arg2_acc = NULL;
    TYPE *mat_res_acc  = NULL;

    const int mat_size = NUM_ELEMS * sizeof(TYPE);

    int err = posix_memalign(
        (void**)&mat_arg1_host, CACHELINE_SIZE, mat_size);
    err |= posix_memalign(
        (void**)&mat_arg2_host, CACHELINE_SIZE, mat_size);
    err |= posix_memalign(
        (void**)&mat_res_host,  CACHELINE_SIZE, mat_size);
    err |= posix_memalign(
        (void**)&mat_arg1_acc,  CACHELINE_SIZE, mat_size);
    err |= posix_memalign(
        (void**)&mat_arg2_acc,  CACHELINE_SIZE, mat_size);
    err |= posix_memalign(
        (void**)&mat_res_acc,   CACHELINE_SIZE, mat_size);
    assert(err == 0 && "Failed to allocate memory!");

    for (int i = 0; i < NUM_ELEMS; i++) {
        mat_arg1_host[i] = 128;
        mat_arg2_host[i] = 64;
    }

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(0, "arg1_host",   mat_arg1_host, mat_size);
    mapArrayToAccelerator(0, "arg2_host",   mat_arg2_host, mat_size);
    mapArrayToAccelerator(0, "result_host", mat_res_host,  mat_size);

    fprintf(stdout, "Invoking accelerator!\n");
    invokeAcceleratorAndBlock(0);
    fprintf(stdout, "Accelerator finished!\n");
#else
    elem_matrix(mat_arg1_host, mat_arg2_host, mat_res_host,
        mat_arg1_acc, mat_arg2_acc, mat_res_acc, mat_size, 0, MUL);
#endif

    int num_errors = 0;
    for (int i = 0; i < NUM_ELEMS; i++) {
        if (mat_res_host[i] != 8192) {
            num_errors++;
        }
    }
    if (num_errors) {
        printf("Number of errors = %d\n", num_errors);
    }
    else {
        printf("No errors!\n");
    }

    free(mat_arg1_host);
    free(mat_arg2_host);
    free(mat_res_host);
    free(mat_arg1_acc);
    free(mat_arg2_acc);
    free(mat_res_acc);
    return 0;
}
