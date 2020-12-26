#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

#define NUM_IMAGES 1
#define NUM_LEVELS (7 + (NUM_IMAGES-1))

typedef struct {
    // input image
    float *input_img;

    // convolution of ut with P
    float *ut_P;

    // division of d by ut_P
    float *d_ut_P;

    // multiplication of d_ut_P with P*
    float *div_P;

    // estimate after one iteration
    float *estimate;
} image_data_t;

task_struct *pipeline[MAX_LEVELS][MAX_REQS];
int req_index[MAX_LEVELS];

void step1(image_data_t *img, int level)
{
    task_struct *task;
    isp_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(isp_args));
    err |= posix_memalign((void**)&img->raw_img, CACHELINE_SIZE, (IMG_HEIGHT+2) * (IMG_WIDTH+2));
    err |= posix_memalign((void**)&img->isp_img, CACHELINE_SIZE, NUM_PIXELS * 3);
    assert(err == 0 && "Failed to allocate memory");

    args->input_img = img->raw_img;
    args->output_img = img->isp_img;

    task->acc_id = ACC_ISP;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void convert_to_grayscale(image_data_t *img, int level)
{
    task_struct *task;
    grayscale_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(grayscale_args));
    err |= posix_memalign((void**)&img->grayscale_img, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

    args->input_image = img->isp_img;
    args->output_image = img->grayscale_img;

    task->acc_id = ACC_GRAYSCALE;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void noise_reduction(image_data_t *img)
{
    int err = 0;

    err |= posix_memalign((void**)&img->denoise_img, CACHELINE_SIZE, NUM_PIXELS*4);
    assert(err == 0 && "Failed to allocate memory");

    // TODO: Convolution
    for (int i = 0; i < NUM_PIXELS; i++) {
        img->denoise_img[i] = (float) img->grayscale_img[i];
    }
}

void gradient_calculation(image_data_t *img, int level)
{
    task_struct *task[5];
    elem_matrix_args *args[5];
    int size = NUM_PIXELS * 4, err = 0;

    for (int i = 0; i < 5; i++) {
        err |= posix_memalign((void**)&task[i], CACHELINE_SIZE, sizeof(task_struct));
        err |= posix_memalign((void**)&args[i], CACHELINE_SIZE, sizeof(elem_matrix_args));
    }
    err |= posix_memalign((void**)&img->I_x,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_y,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xx, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xy, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xx_yy,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->gradient, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->theta,    CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

    // TODO: Convolution
    memset(img->I-x, 0, size);
    memset(img->I-y, 0, size);

    elem_matrix(I_x,  NULL, I_xx,    0, SQR);
    elem_matrix(I_y,  NULL, I_yy,    0, SQR);
    elem_matrix(I_y, I_x, theta, 0, ATAN2);

    elem_matrix(I_xx, I_yy, I_xx_yy, 0, ADD);

    elem_matrix(I_xx_yy, NULL, gradient, 0, SQRT);

    args[0]->arg1 = img->I_x;
    args[0]->arg2 = NULL;
    args[0]->result = img->I_xx;
    args[0]->is_arg2_scalar = 0;
    args[0]->op = SQR;

    args[1]->arg1 = img->I_y;
    args[1]->arg2 = NULL;
    args[1]->result = img->I_yy;
    args[1]->is_arg2_scalar = 0;
    args[1]->op = SQR;

    args[2]->arg1 = img->I_y;
    args[2]->arg2 = img->I_x;
    args[2]->result = img->theta;
    args[2]->is_arg2_scalar = 0;
    args[2]->op = ATAN2;

    args[3]->arg1 = img->I_xx;
    args[3]->arg2 = img->I_yy;
    args[3]->result = img->I_xx_yy;
    args[3]->is_arg2_scalar = 0;
    args[3]->op = ADD;

    args[4]->arg1 = img->I_xx_yy;
    args[4]->arg2 = NULL;
    args[4]->result = img->gradient;
    args[4]->is_arg2_scalar = 0;
    args[4]->op = SQRT;

    for (int i = 0; i < 5; i++) {
        task[i]->acc_id = ACC_ELEM_MATRIX;
        task[i]->acc_args = (void*) args[i];
        task[i]->state = REQ_STATE_WAITING;
    }

    pipeline[level][req_index[level]++] = task[0];
    pipeline[level][req_index[level]++] = task[1];
    pipeline[level][req_index[level]++] = task[2];
    level++;
    pipeline[level][req_index[level]++] = task[3];
    level++;
    pipeline[level][req_index[level]++] = task[4];
}

void non_max_suppression(image_data_t *img, int level)
{
    task_struct *task;
    canny_non_max_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(canny_non_max_args));
    err |= posix_memalign((void**)&img->max_values, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

    args->gradient = img->gradient;
    args->theta = img->theta;
    args->result = img->max_values;

    task->acc_id = ACC_CANNY_NON_MAX;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void thr_and_edge_tracking(image_data_t *img, int level)
{
    task_struct *task;
    edge_tracking_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(edge_tracking_args));
    err |= posix_memalign((void**)&img->final_img, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

    args->input_image = img->max_values;
    args->output_img = img->final_img;

    task->acc_id = ACC_EDGE_TRACKING;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

int main()
{
    for (int i = 0; i < MAX_LEVELS; i++) {
        req_index[i] = 0;
    }

    image_data_t imgs[NUM_IMAGES];

    for (int i = 0; i < NUM_IMAGES; i++) {
        // Step 0: Run raw image through ISP
        process_raw(&imgs[i], i+0);

        // Step 1: Convert image to grayscale
        convert_to_grayscale(&imgs[i], i+1);

        // Step 2: Noise reduction
        noise_reduction(&imgs[i]);

        // Step 3: Gradient calculation
        gradient_calculation(&imgs[i], i+2);

        // Step 4: Non-maximum suppression
        non_max_suppression(&imgs[i], i+5);

        // Steps 5 and 6: Double threshold and edge tracking by hysteresis
        thr_and_edge_tracking(&imgs[i], i+6);
    }

    schedule(NUM_LEVELS, req_index, pipeline);

    return 0;
}
