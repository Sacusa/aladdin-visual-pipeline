#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gem5_harness.h"

#define PI 3.141592653589793238462643
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define DIM(x,y) (((x)*IMG_WIDTH) + (y))

#define CACHELINE_SIZE 64
#define MAX_ACC_INSTANCES 10
#define MAX_LEVELS 50
#define MAX_REQS 50

/**
 * Bookkeeping for accelerators
 */
enum acc_ids_t {
    ACC_CANNY_NON_MAX = 0,
    ACC_CONVOLUTION,
    ACC_EDGE_TRACKING,
    ACC_ELEM_MATRIX,
    ACC_GRAYSCALE,
    ACC_HARRIS_NON_MAX,
    ACC_ISP,
    NUM_ACCS
};

enum acc_state_t {
    ACC_STATE_IDLE = 0,
    ACC_STATE_RUNNING
};

enum req_state_t {
    REQ_STATE_WAITING = 0,
    REQ_STATE_COMPLETED
};

typedef struct {
    // user provides the following fields
    int acc_id;       // ID of the accelerator needed
    void *acc_args;   // pointer to accelerator arguments
    bool free_inputs; // free input data after execution?

    // the following fields are for use by the scheduler
    int state;
} task_struct;

int acc_instances[NUM_ACCS];

/**
 * Accelerator arguments structs
 */
typedef struct {
    float *hypotenuse;
    float *theta;
    uint8_t *result;
} canny_non_max_args;

typedef struct {
    float *input_image;
    float *kernel;
    float *output_image;
    int kern_width;
    int kern_height;
} convolution_args;

typedef struct {
    uint8_t *input_image;
    uint8_t thr_weak;
    uint8_t thr_strong;
    uint8_t *output_image;
} edge_tracking_args;

typedef struct {
    float *arg1;
    float *arg2;
    float *result;
    uint8_t is_arg2_scalar;
    uint8_t op;
} elem_matrix_args;

typedef struct {
    uint8_t *input_image;
    float *output_image;
} grayscale_args;

typedef struct {
    float *harris_response;
    uint8_t *max_values;
} harris_non_max_args;

typedef struct {
    uint8_t *input_img;
    uint8_t *output_img;
} isp_args;

/**
 * Functions for running each accelerator
 */
volatile int *run_accelerator(int acc_id, task_struct *req, int device_id);
volatile int *run_canny_non_max(task_struct *req, int device_id);
volatile int *run_convolution(task_struct *req, int device_id);
volatile int *run_edge_tracking(task_struct *req, int device_id);
volatile int *run_elem_matrix(task_struct *req, int device_id);
volatile int *run_grayscale(task_struct *req, int device_id);
volatile int *run_harris_non_max(task_struct *req, int device_id);
volatile int *run_isp(task_struct *req, int device_id);

/**
 * Scheduling functions
 */
void schedule(int num_levels, int *num_reqs, task_struct *acc_params[MAX_LEVELS][MAX_REQS]);

#endif /* _SCHEDULER_H_ */
