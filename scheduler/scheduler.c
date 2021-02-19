#include "scheduler.h"
#include "canny_non_max.h"
#include "convolution.h"
#include "edge_tracking.h"
#include "elem_matrix.h"
#include "grayscale.h"
#include "harris_non_max.h"
#include "isp.h"

int device_id[NUM_ACCS][MAX_ACC_INSTANCES];

/**
 * Functions for running each accelerator
 */

volatile int *run_accelerator(int acc_id, task_struct *req, int device_id)
{
    switch (acc_id) {
        case ACC_CANNY_NON_MAX:  return run_canny_non_max(req, device_id);
        case ACC_CONVOLUTION:    return run_convolution(req, device_id);
        case ACC_EDGE_TRACKING:  return run_edge_tracking(req, device_id);
        case ACC_ELEM_MATRIX:    return run_elem_matrix(req, device_id);
        case ACC_GRAYSCALE:      return run_grayscale(req, device_id);
        case ACC_HARRIS_NON_MAX: return run_harris_non_max(req, device_id);
        case ACC_ISP:            return run_isp(req, device_id);
    }

    // should not reach here
    return NULL;
}

volatile int *run_canny_non_max(task_struct *req, int device_id)
{
    volatile int *retval = NULL;
    canny_non_max_args *args = (canny_non_max_args*) req->acc_args;

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(device_id, "hypotenuse", args->hypotenuse, NUM_PIXELS*4);
    mapArrayToAccelerator(device_id, "theta",      args->theta,      NUM_PIXELS*4);
    mapArrayToAccelerator(device_id, "result",     args->result,     NUM_PIXELS);
    retval = invokeAcceleratorAndReturn(device_id);
#else
#ifdef LLVM_TRACE
    char *buffer = malloc(50);
    sprintf(buffer, "canny_non_max_%d_trace.gz", device_id % MAX_ACC_INSTANCES);
    llvmtracer_set_trace_name(buffer);
#endif
    canny_non_max(args->hypotenuse, args->theta, args->result);
#endif

    return retval;
}

volatile int *run_convolution(task_struct *req, int device_id)
{
    volatile int *retval = NULL;
    convolution_args *args = (convolution_args*) req->acc_args;

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(device_id, "input_image",  args->input_image,  NUM_PIXELS*4);
    mapArrayToAccelerator(device_id, "kernel",       args->kernel,
            args->kern_width * args->kern_height * 4);
    mapArrayToAccelerator(device_id, "output_image", args->output_image, NUM_PIXELS*4);
    retval = invokeAcceleratorAndReturn(device_id);
#else
#ifdef LLVM_TRACE
    char *buffer = malloc(50);
    sprintf(buffer, "convolution_%d_trace.gz", device_id % MAX_ACC_INSTANCES);
    llvmtracer_set_trace_name(buffer);
#endif
    convolution(args->input_image, args->kernel, args->output_image,
            args->kern_width, args->kern_height);
#endif

    return retval;
}

volatile int *run_edge_tracking(task_struct *req, int device_id)
{
    volatile int *retval = NULL;
    edge_tracking_args *args = (edge_tracking_args*) req->acc_args;

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(device_id, "input_image_host",  args->input_image,  NUM_PIXELS);
    mapArrayToAccelerator(device_id, "output_image_host", args->output_image, NUM_PIXELS);
    retval = invokeAcceleratorAndReturn(device_id);
#else
    uint8_t *ii_acc = NULL, *oi_acc = NULL;
    int err = 0;

    err |= posix_memalign((void**)&ii_acc, CACHELINE_SIZE, NUM_PIXELS);
    err |= posix_memalign((void**)&oi_acc, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

#ifdef LLVM_TRACE
    char *buffer = malloc(50);
    sprintf(buffer, "edge_tracking_%d_trace.gz", device_id % MAX_ACC_INSTANCES);
    llvmtracer_set_trace_name(buffer);
#endif
    edge_tracking(args->input_image, ii_acc, args->thr_weak, args->thr_strong,
            args->output_image, oi_acc);

    free(ii_acc);
    free(oi_acc);
#endif

    return retval;
}

volatile int *run_elem_matrix(task_struct *req, int device_id)
{
    volatile int *retval = NULL;
    elem_matrix_args *args = (elem_matrix_args*) req->acc_args;

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(device_id, "arg1",   args->arg1,   NUM_PIXELS*4);
    mapArrayToAccelerator(device_id, "arg2",   args->arg2,   NUM_PIXELS*4);
    mapArrayToAccelerator(device_id, "result", args->result, NUM_PIXELS*4);
    retval = invokeAcceleratorAndReturn(device_id);
#else
#ifdef LLVM_TRACE
    char *buffer = malloc(50);
    sprintf(buffer, "elem_matrix_%d_trace.gz", device_id % MAX_ACC_INSTANCES);
    llvmtracer_set_trace_name(buffer);
#endif
    elem_matrix(args->arg1, args->arg2, args->result, args->is_arg2_scalar, args->op);
#endif

    return retval;
}

volatile int *run_grayscale(task_struct *req, int device_id)
{
    volatile int *retval = NULL;
    grayscale_args *args = (grayscale_args*) req->acc_args;

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(device_id, "input_image",  args->input_image,  NUM_PIXELS);
    mapArrayToAccelerator(device_id, "output_image", args->output_image, NUM_PIXELS);
    retval = invokeAcceleratorAndReturn(device_id);
#else
#ifdef LLVM_TRACE
    char *buffer = malloc(50);
    sprintf(buffer, "grayscale_%d_trace.gz", device_id % MAX_ACC_INSTANCES);
    llvmtracer_set_trace_name(buffer);
#endif
    grayscale(args->input_image, args->output_image);
#endif

    return retval;
}

volatile int *run_harris_non_max(task_struct *req, int device_id)
{
    volatile int *retval = NULL;
    harris_non_max_args *args = (harris_non_max_args*) req->acc_args;

    int hr_size = NUM_PIXELS * 4;

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(device_id, "harris_response_host", args->harris_response, hr_size);
    mapArrayToAccelerator(device_id, "max_values_host",      args->max_values,      NUM_PIXELS);
    retval = invokeAcceleratorAndReturn(device_id);
#else
    float   *hr_acc = NULL;
    uint8_t *mv_acc = NULL;
    int err = 0;

    err |= posix_memalign((void**)&hr_acc, CACHELINE_SIZE, hr_size);
    err |= posix_memalign((void**)&mv_acc, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

#ifdef LLVM_TRACE
    char *buffer = malloc(50);
    sprintf(buffer, "harris_non_max_%d_trace.gz", device_id % MAX_ACC_INSTANCES);
    llvmtracer_set_trace_name(buffer);
#endif
    harris_non_max(args->harris_response, args->max_values, hr_acc, mv_acc);

    free(hr_acc);
    free(mv_acc);
#endif

    return retval;
}

volatile int *run_isp(task_struct *req, int device_id)
{
    volatile int *retval = NULL;
    isp_args *args = (isp_args*) req->acc_args;
    int size = (IMG_HEIGHT+2) * (IMG_WIDTH+2);

#ifdef GEM5_HARNESS
    mapArrayToAccelerator(device_id, "input_image_host",  args->input_img,  size);
    mapArrayToAccelerator(device_id, "output_image_host", args->output_img, NUM_PIXELS*3);
    retval = invokeAcceleratorAndReturn(device_id);
#else
    uint8_t *input_img_acc = NULL, *output_img_acc = NULL;
    int err = 0;

    err |= posix_memalign((void**)&input_img_acc,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&output_img_acc, CACHELINE_SIZE, NUM_PIXELS*3);
    assert(err == 0 && "Failed to allocate memory");

#ifdef LLVM_TRACE
    char *buffer = malloc(50);
    sprintf(buffer, "isp_%d_trace.gz", device_id % MAX_ACC_INSTANCES);
    llvmtracer_set_trace_name(buffer);
#endif
    isp(args->input_img, args->output_img, input_img_acc, output_img_acc);

    free(input_img_acc);
    free(output_img_acc);
#endif

    return retval;
}

/**
 * Scheduling functions
 */

int get_device_id(int acc_id, int instance)
{
    return ((acc_id * MAX_ACC_INSTANCES) + instance);
}

void schedule(int num_levels, int *num_reqs, task_struct *acc_params[MAX_LEVELS][MAX_REQS])
{
    acc_instances[ACC_CANNY_NON_MAX]  = 1;
    acc_instances[ACC_CONVOLUTION]    = 1;
    acc_instances[ACC_EDGE_TRACKING]  = 1;
    acc_instances[ACC_ELEM_MATRIX]    = 6;
    acc_instances[ACC_GRAYSCALE]      = 1;
    acc_instances[ACC_HARRIS_NON_MAX] = 1;
    acc_instances[ACC_ISP]            = 1;

    int acc_state[NUM_ACCS][MAX_ACC_INSTANCES];
    volatile int *finish_flag[NUM_ACCS][MAX_ACC_INSTANCES];

    for (int i = 0; i < NUM_ACCS; i++) {
        for (int j = 0; j < acc_instances[i]; j++) {
            acc_state[i][j] = ACC_STATE_IDLE;
        }
    }

    for (int i = 0; i < num_levels;) {
        bool level_complete = true;

        /**
         * Go through all the requests for this level and run a waiting request if we can
         */
        for (int j = 0; j < num_reqs[i]; j++) {
            task_struct *req = acc_params[i][j];

            if (req->state == REQ_STATE_WAITING) {
                // check if we have a free instance
                for (int k = 0; k < acc_instances[req->acc_id]; k++) {
                    if (acc_state[req->acc_id][k] == ACC_STATE_IDLE) {
                        acc_state[req->acc_id][k] = ACC_STATE_RUNNING;
                        finish_flag[req->acc_id][k] = run_accelerator(req->acc_id, req, \
                                get_device_id(req->acc_id, k));
                        req->state = REQ_STATE_COMPLETED;
                        level_complete = false;
                        break;
                    }
                }
            }
        }

        /**
         * Wait on the started accelerators
         */
        for (int j = 0; j < NUM_ACCS; j++) {
            for (int k = 0; k < acc_instances[j]; k++) {
                if (acc_state[j][k] == ACC_STATE_RUNNING) {
                    acc_state[j][k] = ACC_STATE_IDLE;
#ifdef GEM5_HARNESS
                    waitForAccelerator(finish_flag[j][k]);
#endif
                }
            }
        }

        /**
         * Advance to next level if all requests served
         */
        if (level_complete) {
            i++;
        }
    }
}
