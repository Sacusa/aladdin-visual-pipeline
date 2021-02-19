#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

#define NUM_IMAGES 1
#define NUM_LEVELS (4 + (NUM_IMAGES-1))

typedef struct {
    // ISP
    uint8_t *raw_img;
    uint8_t *isp_img;

    // input image
    float *input_img;

    // convolution of ut with P
    float *conv_psf;

    // division of d by ut_P
    float *div_ut_psf;

    // convolution of d_ut_P with P*
    float *conv_psf_flip;

    // estimate after each iteration
    float *estimate;
} image_data_t;

float *psf;
float *psf_flip;

task_struct *pipeline[MAX_LEVELS][MAX_REQS];
int req_index[MAX_LEVELS];

void init_img(image_data_t *img)
{
    int size = NUM_PIXELS * 4, err = 0;

    err |= posix_memalign((void**)&img->conv_psf,      CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->div_ut_psf,    CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->conv_psf_flip, CACHELINE_SIZE, size);
    err |= posix_memalign((void**)&img->estimate,      CACHELINE_SIZE, size);
    assert(err == 0 && "Failed to allocate memory");
}

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
    err |= posix_memalign((void**)&img->input_img, CACHELINE_SIZE, NUM_PIXELS*4);
    assert(err == 0 && "Failed to allocate memory");

    args->input_image = img->isp_img;
    args->output_image = img->input_img;

    task->acc_id = ACC_GRAYSCALE;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void run_conv_psf(image_data_t *img, int level)
{
    task_struct *task;
    convolution_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(convolution_args));
    assert(err == 0 && "Failed to allocate memory");

    args->input_image = img->estimate;
    args->kernel = psf;
    args->output_image = img->conv_psf;
    args->kern_width = 5;
    args->kern_height = 5;

    task->acc_id = ACC_CONVOLUTION;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void run_div_ut_psf(image_data_t *img, int level)
{
    task_struct *task;
    elem_matrix_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(elem_matrix_args));
    assert(err == 0 && "Failed to allocate memory");

    args->arg1 = img->input_img;
    args->arg2 = img->conv_psf;
    args->result = img->div_ut_psf;
    args->is_arg2_scalar = 0;
    args->op = DIV;

    task->acc_id = ACC_ELEM_MATRIX;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void run_conv_psf_flip(image_data_t *img, int level)
{
    task_struct *task;
    convolution_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(convolution_args));
    assert(err == 0 && "Failed to allocate memory");

    args->input_image = img->div_ut_psf;
    args->kernel = psf_flip;
    args->output_image = img->conv_psf_flip;
    args->kern_width = 5;
    args->kern_height = 5;

    task->acc_id = ACC_CONVOLUTION;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

void run_mult_psf_flip(image_data_t *img, int level)
{
    task_struct *task;
    elem_matrix_args *args;
    int err = 0;

    err |= posix_memalign((void**)&task, CACHELINE_SIZE, sizeof(task_struct));
    err |= posix_memalign((void**)&args, CACHELINE_SIZE, sizeof(elem_matrix_args));
    assert(err == 0 && "Failed to allocate memory");

    args->arg1 = img->estimate;
    args->arg2 = img->conv_psf_flip;
    args->result = img->estimate;
    args->is_arg2_scalar = 0;
    args->op = MUL;

    task->acc_id = ACC_ELEM_MATRIX;
    task->acc_args = (void*) args;
    task->state = REQ_STATE_WAITING;

    pipeline[level][req_index[level]++] = task;
}

int main()
{
    int err = 0;
    image_data_t imgs[NUM_IMAGES];

    err = posix_memalign((void**)&psf, CACHELINE_SIZE, 100);
    assert(err == 0 && "Failed to allocate memory");
    psf[0]  =  1; psf[1]  =  4; psf[2]  =  7; psf[3]  =  4; psf[4]  =  1;
    psf[5]  =  4; psf[6]  = 16; psf[7]  = 26; psf[8]  = 16; psf[9]  =  4;
    psf[10] =  7; psf[11] = 26; psf[12] = 41; psf[13] = 26; psf[14] =  7;
    psf[15] =  4; psf[16] = 16; psf[17] = 26; psf[18] = 16; psf[19] =  4;
    psf[20] =  1; psf[21] =  4; psf[22] =  7; psf[23] =  4; psf[24] =  1;
    for (int i = 0; i < 25; i++) {
        psf[i] /= 273;
    }

    err = posix_memalign((void**)&psf_flip, CACHELINE_SIZE, 100);
    assert(err == 0 && "Failed to allocate memory");
    psf_flip[0]  =  1; psf_flip[1]  =  4; psf_flip[2]  =  7; psf_flip[3]  =  4; psf_flip[4]  =  1;
    psf_flip[5]  =  4; psf_flip[6]  = 16; psf_flip[7]  = 26; psf_flip[8]  = 16; psf_flip[9]  =  4;
    psf_flip[10] =  7; psf_flip[11] = 26; psf_flip[12] = 41; psf_flip[13] = 26; psf_flip[14] =  7;
    psf_flip[15] =  4; psf_flip[16] = 16; psf_flip[17] = 26; psf_flip[18] = 16; psf_flip[19] =  4;
    psf_flip[20] =  1; psf_flip[21] =  4; psf_flip[22] =  7; psf_flip[23] =  4; psf_flip[24] =  1;
    for (int i = 0; i < 25; i++) {
        psf_flip[i] /= 273;
    }

    /**
     * Initialize image struct and preprocess image
     */
    for (int i = 0; i < MAX_LEVELS; i++) {
        req_index[i] = 0;
    }

    for (int i = 0; i < NUM_IMAGES; i++) {
        init_img(&imgs[i]);
        process_raw(&imgs[i], i+0);
        convert_to_grayscale(&imgs[i], i+1);
    }

    schedule(NUM_LEVELS, req_index, pipeline);

    /**
     * Initialize the estimate
     */
    for (int i = 0; i < NUM_IMAGES; i++) {
        memcpy(imgs[i].estimate, imgs[i].input_img, NUM_PIXELS*4);
    }

    /**
     * Run the rest of the stages
     */
    for (int i = 0; i < MAX_LEVELS; i++) {
        req_index[i] = 0;
    }

    for (int i = 0; i < NUM_IMAGES; i++) {
        for (int j = 0; j < 2; j++) {
            int index = i + (j*4);

            run_conv_psf(&imgs[i], index + 0);
            run_div_ut_psf(&imgs[i], index + 1);
            run_conv_psf_flip(&imgs[i], index + 2);
            run_mult_psf_flip(&imgs[i], index + 3);
        }
    }

    schedule(NUM_LEVELS, req_index, pipeline);

    return 0;
}
