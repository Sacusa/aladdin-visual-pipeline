#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

#define NUM_IMAGES 10
#define NUM_LEVELS (8 + (NUM_IMAGES-1))

typedef struct {
    // ISP
    uint8_t *raw_img;
    uint8_t *isp_img;

    // Grayscale
    uint8_t *grayscale_img;

    // Spatial derivative
    float *I_x, *I_y;

    // Structure tensor
    float *I_xx, *I_xy, *I_yy;
    float *I_xx_g, *I_xy_g, *I_yy_g;

    // Harris response
    float *detA1, *detA2, *detA, *traceA;
    float *hr1, *hr2, *hr, *k;
    float *harris_response;

    // Non-max suppression
    uint8_t *max_values;
} image_data_t;

task_struct *pipeline[MAX_LEVELS][MAX_REQS];
int req_index[MAX_LEVELS];

void process_raw(image_data_t *img, int level)
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

void spatial_derivative_calc(image_data_t *img)
{
    int size = NUM_PIXELS * 4, err = 0;

    err |= posix_memalign((void**)&img->I_x, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_y, CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

    // TODO: Convolution
    memset(img->I_x, 0, size);
    memset(img->I_y, 0, size);
}

void structure_tensor_setup(image_data_t *img, int level)
{
    task_struct *task[3];
    elem_matrix_args *args[3];
    int size = NUM_PIXELS * 4, err = 0;

    for (int i = 0; i < 3; i++) {
        err |= posix_memalign((void**)&task[i], CACHELINE_SIZE, sizeof(task_struct));
        err |= posix_memalign((void**)&args[i], CACHELINE_SIZE, sizeof(elem_matrix_args));
    }
    err |= posix_memalign((void**)&img->I_xx,   CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xy,   CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_yy,   CACHELINE_SIZE, size);
    // TODO: we will need below after integrating convolution
    //err |= posix_memalign((void**)&img->I_xx_g, CACHELINE_SIZE, size);
    //err |= posix_memalign((void**)&img->I_xy_g, CACHELINE_SIZE, size);
    //err |= posix_memalign((void**)&img->I_yy_g, CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

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

    args[2]->arg1 = img->I_x;
    args[2]->arg2 = img->I_y;
    args[2]->result = img->I_xy;
    args[2]->is_arg2_scalar = 0;
    args[2]->op = MUL;

    for (int i = 0; i < 3; i++) {
        task[i]->acc_id = ACC_ELEM_MATRIX;
        task[i]->acc_args = (void*) args[i];
        task[i]->state = REQ_STATE_WAITING;
        pipeline[level][req_index[level]++] = task[i];
    }

    // TODO: Convolution
    img->I_xx_g = img->I_xx;
    img->I_yy_g = img->I_yy;
    img->I_xy_g = img->I_xy;
}

void harris_response_calc(image_data_t *img, int level)
{
    task_struct *task[7];
    elem_matrix_args *args[7];
    int size = NUM_PIXELS * 4,  err = 0;

    for (int i = 0; i < 7; i++) {
        err |= posix_memalign((void**)&task[i], CACHELINE_SIZE, sizeof(task_struct));
        err |= posix_memalign((void**)&args[i], CACHELINE_SIZE, sizeof(elem_matrix_args));
    }
    err |= posix_memalign((void**)&img->detA1,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->detA2,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->detA,   CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->traceA, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->hr1,    CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->hr2,    CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->hr,     CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->k,      CACHELINE_SIZE, 4);
    err |= posix_memalign((void**)&img->harris_response, CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

    *img->k = 0.05;

    args[0]->arg1 = img->I_xx;
    args[0]->arg2 = img->I_yy;
    args[0]->result = img->detA1;
    args[0]->is_arg2_scalar = 0;
    args[0]->op = MUL;

    args[1]->arg1 = img->I_xy;
    args[1]->arg2 = NULL;
    args[1]->result = img->detA2;
    args[1]->is_arg2_scalar = 0;
    args[1]->op = SQR;

    args[2]->arg1 = img->I_xx;
    args[2]->arg2 = img->I_yy;
    args[2]->result = img->traceA;
    args[2]->is_arg2_scalar = 0;
    args[2]->op = ADD;

    args[3]->arg1 = img->detA1;
    args[3]->arg2 = img->detA2;
    args[3]->result = img->detA;
    args[3]->is_arg2_scalar = 0;
    args[3]->op = SUB;

    args[4]->arg1 = img->traceA;
    args[4]->arg2 = NULL;
    args[4]->result = img->hr1;
    args[4]->is_arg2_scalar = 0;
    args[4]->op = SQR;

    args[5]->arg1 = img->hr1;
    args[5]->arg2 = img->k;
    args[5]->result = img->hr2;
    args[5]->is_arg2_scalar = 1;
    args[5]->op = MUL;

    args[6]->arg1 = img->detA;
    args[6]->arg2 = img->hr2;
    args[6]->result = img->hr;
    args[6]->is_arg2_scalar = 0;
    args[6]->op = SUB;

    for (int i = 0; i < 7; i++) {
        task[i]->acc_id = ACC_ELEM_MATRIX;
        task[i]->acc_args = (void*) args[i];
        task[i]->state = REQ_STATE_WAITING;
    }

    pipeline[level][req_index[level]++] = task[0];
    pipeline[level][req_index[level]++] = task[1];
    pipeline[level][req_index[level]++] = task[2];
    level++;
    pipeline[level][req_index[level]++] = task[3];
    pipeline[level][req_index[level]++] = task[4];
    level++;
    pipeline[level][req_index[level]++] = task[5];
    level++;
    pipeline[level][req_index[level]++] = task[6];
}

void non_max_suppression(image_data_t *img, int level)
{
    task_struct *task;
    harris_non_max_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(harris_non_max_args));
    err |= posix_memalign((void**)&img->max_values, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

    args->harris_response = img->harris_response;
    args->max_values = img->max_values;

    task->acc_id = ACC_HARRIS_NON_MAX;
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

        // Step 2: Spatial derivative calculation
        spatial_derivative_calc(&imgs[i]);

        // Step 3: Structure tensor setup
        structure_tensor_setup(&imgs[i], i+2);

        // Step 4: Harris response calculation
        harris_response_calc(&imgs[i], i+3);

        // Step 5: Non-max suppression
        non_max_suppression(&imgs[i], i+7);
    }

    schedule(NUM_LEVELS, req_index, pipeline);

    return 0;
}
