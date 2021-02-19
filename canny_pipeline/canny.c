#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

#define NUM_IMAGES 1
#define NUM_LEVELS (9 + (NUM_IMAGES-1))

typedef struct {
    // ISP
    uint8_t *raw_img;
    uint8_t *isp_img;

    // Grayscale
    float *grayscale_img;

    // Noise reduction
    float *gauss_kernel;
    float *denoise_img;

    // Gradient calculation
    float *K_x, *K_y;
    float *I_x, *I_y, *I_xx, *I_yy, *I_xx_yy;
    float *gradient, *theta;

    // Non-maxmimum suppression
    uint8_t *max_values;

    // Edge tracking
    uint8_t *final_img;
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

void noise_reduction(image_data_t *img, int level)
{
    task_struct *task;
    convolution_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(convolution_args));
    err |= posix_memalign((void**)&img->gauss_kernel, CACHELINE_SIZE, 100);
    err |= posix_memalign((void**)&img->denoise_img,  CACHELINE_SIZE, NUM_PIXELS*4);
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

    args->input_image = img->grayscale_img;
    args->kernel = img->gauss_kernel;
    args->output_image = img->denoise_img;
    args->kern_width = 5;
    args->kern_height = 5;

    task->acc_id = ACC_CONVOLUTION;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void gradient_calculation(image_data_t *img, int level)
{
    task_struct *task[7];
    convolution_args *c_args[2];
    elem_matrix_args *em_args[5];
    int size = NUM_PIXELS * 4, err = 0;

    for (int i = 0; i < 7; i++) {
        err |= posix_memalign((void**)&task[i], CACHELINE_SIZE, sizeof(task_struct));
    }
    for (int i = 0; i < 2; i++) {
        err |= posix_memalign((void**)&c_args[i], CACHELINE_SIZE, sizeof(convolution_args));
    }
    for (int i = 0; i < 5; i++) {
        err |= posix_memalign((void**)&em_args[i], CACHELINE_SIZE, sizeof(elem_matrix_args));
    }
    err |= posix_memalign((void**)&img->K_x,  CACHELINE_SIZE, 36);
    err |= posix_memalign((void**)&img->K_y,  CACHELINE_SIZE, 36);
    err |= posix_memalign((void**)&img->I_x,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_y,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xx, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_yy, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->I_xx_yy,  CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->gradient, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->theta,    CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");

    img->K_x[0] = -1; img->K_x[1] = 0; img->K_x[2] = 1;
    img->K_x[3] = -2; img->K_x[4] = 0; img->K_x[5] = 2;
    img->K_x[6] = -1; img->K_x[7] = 0; img->K_x[8] = 1;

    img->K_y[0] =  1; img->K_y[1] =  2; img->K_y[2] =  1;
    img->K_y[3] =  0; img->K_y[4] =  0; img->K_y[5] =  0;
    img->K_y[6] = -1; img->K_y[7] = -2; img->K_y[8] = -1;

    c_args[0]->input_image = img->denoise_img;
    c_args[0]->kernel = img->K_x;
    c_args[0]->output_image = img->I_x;
    c_args[0]->kern_width = 3;
    c_args[0]->kern_height = 3;

    c_args[1]->input_image = img->denoise_img;
    c_args[1]->kernel = img->K_y;
    c_args[1]->output_image = img->I_y;
    c_args[1]->kern_width = 3;
    c_args[1]->kern_height = 3;

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

    em_args[2]->arg1 = img->I_y;
    em_args[2]->arg2 = img->I_x;
    em_args[2]->result = img->theta;
    em_args[2]->is_arg2_scalar = 0;
    em_args[2]->op = ATAN2;

    em_args[3]->arg1 = img->I_xx;
    em_args[3]->arg2 = img->I_yy;
    em_args[3]->result = img->I_xx_yy;
    em_args[3]->is_arg2_scalar = 0;
    em_args[3]->op = ADD;

    em_args[4]->arg1 = img->I_xx_yy;
    em_args[4]->arg2 = NULL;
    em_args[4]->result = img->gradient;
    em_args[4]->is_arg2_scalar = 0;
    em_args[4]->op = SQRT;

    for (int i = 0; i < 2; i++) {
        task[i]->acc_id = ACC_CONVOLUTION;
        task[i]->acc_args = (void*) c_args[i];
        task[i]->state = REQ_STATE_WAITING;
    }
    for (int i = 2; i < 7; i++) {
        task[i]->acc_id = ACC_ELEM_MATRIX;
        task[i]->acc_args = (void*) em_args[i-2];
        task[i]->state = REQ_STATE_WAITING;
    }

    pipeline[level][req_index[level]++] = task[0];
    pipeline[level][req_index[level]++] = task[1];
    level++;
    pipeline[level][req_index[level]++] = task[2];
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
    canny_non_max_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(canny_non_max_args));
    err |= posix_memalign((void**)&img->max_values, CACHELINE_SIZE, NUM_PIXELS);
    assert(err == 0 && "Failed to allocate memory");

    args->hypotenuse = img->gradient;
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
    args->output_image = img->final_img;

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
        noise_reduction(&imgs[i], i+2);

        // Step 3: Gradient calculation
        gradient_calculation(&imgs[i], i+3);

        // Step 4: Non-maximum suppression
        non_max_suppression(&imgs[i], i+7);

        // Steps 5 and 6: Double threshold and edge tracking by hysteresis
        thr_and_edge_tracking(&imgs[i], i+8);
    }

    schedule(NUM_LEVELS, req_index, pipeline);

    return 0;
}
