#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

#define NUM_IMAGES 1
#define NUM_LEVELS (10 + (NUM_IMAGES-1))

typedef struct {
    // ISP
    uint8_t *raw_img;
    uint8_t *isp_img;

    // Grayscale
    float *grayscale_img;

    // Spatial derivative
    float *K_x, *K_y;
    float *I_x, *I_y;

    // Structure tensor
    float *I_xx, *I_xy, *I_yy;
    float *gauss_kernel;
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
    err |= posix_memalign((void**)&img->grayscale_img, CACHELINE_SIZE, NUM_PIXELS*4);
    assert(err == 0 && "Failed to allocate memory");

    args->input_image = img->isp_img;
    args->output_image = img->grayscale_img;

    task->acc_id = ACC_GRAYSCALE;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void spatial_derivative_calc(image_data_t *img, int level)
{
    task_struct *task[2];
    convolution_args *args[2];
    int size = NUM_PIXELS * 4, err = 0;

    for (int i = 0; i < 2; i++) {
        err |= posix_memalign((void**)&task[i], CACHELINE_SIZE, sizeof(task_struct));
        err |= posix_memalign((void**)&args[i], CACHELINE_SIZE, sizeof(convolution_args));
    }
    err |= posix_memalign((void**)&img->K_x, CACHELINE_SIZE, 36);
    err |= posix_memalign((void**)&img->K_y, CACHELINE_SIZE, 36);
    err |= posix_memalign((void**)&img->I_x, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_y, CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

    args[0]->input_image = img->grayscale_img;
    args[0]->kernel = img->K_x;
    args[0]->output_image = img->I_x;
    args[0]->kern_width = 3;
    args[0]->kern_height = 3;

    args[1]->input_image = img->grayscale_img;
    args[1]->kernel = img->K_y;
    args[1]->output_image = img->I_y;
    args[1]->kern_width = 3;
    args[1]->kern_height = 3;

    for (int i = 0; i < 2; i++) {
        task[i]->acc_id = ACC_CONVOLUTION;
        task[i]->acc_args = (void*) args[i];
        task[i]->state = REQ_STATE_WAITING;
        pipeline[level][req_index[level]++] = task[i];
    }
}

void structure_tensor_setup(image_data_t *img, int level)
{
    task_struct *task[6];
    elem_matrix_args *em_args[3];
    convolution_args *c_args[3];
    int size = NUM_PIXELS * 4, err = 0;

    for (int i = 0; i < 6; i++) {
        err |= posix_memalign((void**)&task[i], CACHELINE_SIZE, sizeof(task_struct));
    }
    for (int i = 0; i < 3; i++) {
        err |= posix_memalign((void**)&em_args[i], CACHELINE_SIZE, sizeof(elem_matrix_args));
        err |= posix_memalign((void**)&c_args[i],  CACHELINE_SIZE, sizeof(convolution_args));
    }
    err |= posix_memalign((void**)&img->I_xx,   CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xy,   CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_yy,   CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xx_g, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xy_g, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_yy_g, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->gauss_kernel, CACHELINE_SIZE, 100);
    assert(err == 0 && "Failed to allocate memory");

    img->gauss_kernel[0]  =  1; img->gauss_kernel[1]  =  4; img->gauss_kernel[2]  =  7;
    img->gauss_kernel[3]  =  4; img->gauss_kernel[4]  =  1;
    img->gauss_kernel[5]  =  4; img->gauss_kernel[6]  = 16; img->gauss_kernel[7]  = 26;
    img->gauss_kernel[8]  = 16; img->gauss_kernel[9]  =  4;
    img->gauss_kernel[10] =  7; img->gauss_kernel[11] = 26; img->gauss_kernel[12] = 41;
    img->gauss_kernel[13] = 26; img->gauss_kernel[14] =  7;
    img->gauss_kernel[15] =  4; img->gauss_kernel[16] = 16; img->gauss_kernel[17] = 26;
    img->gauss_kernel[18] = 16; img->gauss_kernel[19] =  4;
    img->gauss_kernel[20] =  1; img->gauss_kernel[21] =  4; img->gauss_kernel[22] =  7;
    img->gauss_kernel[23] =  4; img->gauss_kernel[24] =  1;
    for (int i = 0; i < 25; i++) {
        img->gauss_kernel[i] /= 273;
    }

    em_args[0]->arg1 = img->I_x;
    em_args[0]->arg2 = NULL;
    em_args[0]->result = img->I_xx;
    em_args[0]->is_arg2_scalar = 0;
    em_args[0]->op = SQR;

    em_args[1]->arg1 = img->I_y;
    em_args[1]->arg2 = NULL;
    em_args[1]->result = img->I_yy;
    em_args[1]->is_arg2_scalar = 0;
    em_args[1]->op = SQR;

    em_args[2]->arg1 = img->I_x;
    em_args[2]->arg2 = img->I_y;
    em_args[2]->result = img->I_xy;
    em_args[2]->is_arg2_scalar = 0;
    em_args[2]->op = MUL;

    c_args[0]->input_image = img->I_xx;
    c_args[0]->kernel = img->gauss_kernel;
    c_args[0]->output_image = img->I_xx_g;
    c_args[0]->kern_width = 5;
    c_args[0]->kern_height = 5;

    c_args[1]->input_image = img->I_yy;
    c_args[1]->kernel = img->gauss_kernel;
    c_args[1]->output_image = img->I_yy_g;
    c_args[1]->kern_width = 5;
    c_args[1]->kern_height = 5;

    c_args[2]->input_image = img->I_xy;
    c_args[2]->kernel = img->gauss_kernel;
    c_args[2]->output_image = img->I_xy_g;
    c_args[2]->kern_width = 5;
    c_args[2]->kern_height = 5;

    for (int i = 0; i < 3; i++) {
        task[i]->acc_id = ACC_ELEM_MATRIX;
        task[i]->acc_args = (void*) em_args[i];
        task[i]->state = REQ_STATE_WAITING;

        task[i+3]->acc_id = ACC_CONVOLUTION;
        task[i+3]->acc_args = (void*) c_args[i];
        task[i+3]->state = REQ_STATE_WAITING;

        pipeline[level][req_index[level]++]     = task[i];
        pipeline[level+1][req_index[level+1]++] = task[i+3];
    }
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
        spatial_derivative_calc(&imgs[i], i+2);

        // Step 3: Structure tensor setup
        structure_tensor_setup(&imgs[i], i+3);

        // Step 4: Harris response calculation
        harris_response_calc(&imgs[i], i+5);

        // Step 5: Non-max suppression
        non_max_suppression(&imgs[i], i+9);
    }

    schedule(NUM_LEVELS, req_index, pipeline);

    return 0;
}
