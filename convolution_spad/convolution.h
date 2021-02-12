#define TYPE float
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define KERN_WIDTH  5
#define KERN_HEIGHT 5
#define PADDING (((KERN_HEIGHT - 1) >> 1))
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define KERN_DIM(x,y) (((x)*KERN_WIDTH) + (y))
#define IMG_DIM(x,y)  (((x)*IMG_WIDTH)  + (y))

#define OUT_SPAD_HEIGHT 28
#define OUT_SPAD_WIDTH 32
#define OUT_SPAD_DIM(x,y) (((x)*OUT_SPAD_WIDTH) + (y))

#define IN_SPAD_HEIGHT (OUT_SPAD_HEIGHT + 4)
#define IN_SPAD_WIDTH (OUT_SPAD_WIDTH + 4)
#define IN_SPAD_DIM(x,y) (((x)*IN_SPAD_WIDTH) + (y))

void convolution(TYPE *input_image_host, TYPE *kernel_host, TYPE *output_image_host,
        TYPE *input_image_acc, TYPE *kernel_acc, TYPE *output_image_acc) {
    int spad = 0;
    int in_offset = 0, next_in_offset = 0;
    int out_offset = 0, next_out_offset = 0;

    int max_height = IMG_HEIGHT - 1;
    int num_iters = ((IMG_HEIGHT / OUT_SPAD_HEIGHT) + ((IMG_HEIGHT % OUT_SPAD_HEIGHT) != 0)) *
                    ((IMG_WIDTH  / OUT_SPAD_WIDTH)  + ((IMG_WIDTH  % OUT_SPAD_WIDTH)  != 0));
    int last_iter = num_iters - 1;

    int start_img_i = 0, next_start_img_i = 0;
    int start_img_j = 0, next_start_img_j = 0;
    int out_spad_height, next_out_spad_height;
    int out_spad_width,  next_out_spad_width;
    int in_spad_width;

    next_out_spad_height = ((next_start_img_i + OUT_SPAD_HEIGHT) > IMG_HEIGHT) ?
                           (IMG_HEIGHT - next_start_img_i) : OUT_SPAD_HEIGHT;
    next_out_spad_width  = ((next_start_img_j + OUT_SPAD_WIDTH) > IMG_WIDTH) ?
                           (IMG_WIDTH - next_start_img_j) : OUT_SPAD_WIDTH;
    in_spad_width = next_out_spad_width + PADDING;

    // load the kernel
    dmaLoad(kernel_acc, kernel_host, KERN_WIDTH * KERN_HEIGHT * 4);

    // load the image
    for (int i = 0; i < next_out_spad_height; i++) {
        dmaLoad(&input_image_acc[IN_SPAD_DIM(i+PADDING,PADDING)],
                &input_image_host[IMG_DIM(i,0)],
                in_spad_width * sizeof(TYPE));
    }

    // load the lower buffer row(s)
    for (int i = 0; i < PADDING; i++) {
        dmaLoad(&input_image_acc[IN_SPAD_DIM(next_out_spad_height+PADDING+i, PADDING)],
                &input_image_host[IMG_DIM(next_out_spad_height+i, 0)],
                in_spad_width * sizeof(TYPE));
    }

    next_start_img_j += OUT_SPAD_WIDTH;
    if (next_start_img_j >= IMG_WIDTH) {
        next_start_img_i += OUT_SPAD_HEIGHT;
        next_start_img_j = 0;
    }

    out_spad_height = next_out_spad_height;
    out_spad_width  = next_out_spad_width;

    for (int iter = 0; iter < num_iters; iter++) {
        if (iter != last_iter) {
            spad ^= 1;
            next_in_offset = spad * IN_SPAD_WIDTH * IN_SPAD_HEIGHT;
            next_out_offset = spad * OUT_SPAD_WIDTH * OUT_SPAD_HEIGHT;

            next_out_spad_height = ((next_start_img_i + OUT_SPAD_HEIGHT) > IMG_HEIGHT) ?
                                   (IMG_HEIGHT - next_start_img_i) : OUT_SPAD_HEIGHT;
            next_out_spad_width  = ((next_start_img_j + OUT_SPAD_WIDTH) > IMG_WIDTH) ?
                                   (IMG_WIDTH - next_start_img_j) : OUT_SPAD_WIDTH;

            if (next_start_img_j == 0) {
                in_spad_width = next_out_spad_width + PADDING;
            }
            else {
                in_spad_width = next_out_spad_width;
                if ((next_start_img_j + next_out_spad_width + PADDING) > IMG_WIDTH) {
                    in_spad_width = IMG_WIDTH - next_start_img_j - PADDING;
                }
            }

            // load the upper buffer row(s)
            int start_i = (next_start_img_i == 0) ? PADDING : 0;
            for (int i = start_i; i < PADDING; i++) {
                if (next_start_img_j > 0) {
                    int max_j = PADDING<<1;
                    for (int j = 0; j < max_j; j++) {
                        input_image_acc[next_in_offset + IN_SPAD_DIM(i,j)] =
                            input_image_acc[in_offset + IN_SPAD_DIM(i,IN_SPAD_WIDTH-max_j+j)];
                    }
                }

                if (in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset + IN_SPAD_DIM(i,
                                PADDING + ((next_start_img_j > 0) ? PADDING : 0))],
                            &input_image_host[IMG_DIM(next_start_img_i-PADDING+i,
                                next_start_img_j + ((next_start_img_j>0) ? PADDING : 0))],
                            in_spad_width * sizeof(TYPE));
                }
            }

            for (int i = 0; i < next_out_spad_height; i++) {
                if (next_start_img_j > 0) {
                    int max_j = PADDING<<1;
                    for (int j = 0; j < max_j; j++) {
                        input_image_acc[next_in_offset + IN_SPAD_DIM(i+PADDING,j)] =
                            input_image_acc[in_offset + IN_SPAD_DIM(i+PADDING,
                                                                    IN_SPAD_WIDTH-max_j+j)];
                    }
                }

                if (in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset + IN_SPAD_DIM(i+PADDING,
                                PADDING + ((next_start_img_j > 0) ? PADDING : 0))],
                            &input_image_host[IMG_DIM(next_start_img_i+i,
                                next_start_img_j + ((next_start_img_j>0) ? PADDING : 0))],
                            in_spad_width * sizeof(TYPE));
                }
            }

            // load the lower buffer row(s)
            int end_i = IMG_HEIGHT - next_start_img_i - next_out_spad_height;
            end_i = (end_i > PADDING) ? PADDING : end_i;
            for (int i = 0; i < end_i; i++) {
                if (next_start_img_j > 0) {
                    int max_j = PADDING<<1;
                    for (int j = 0; j < max_j; j++) {
                        input_image_acc[next_in_offset + IN_SPAD_DIM(next_out_spad_height+PADDING+i,j)] =
                            input_image_acc[in_offset + IN_SPAD_DIM(next_out_spad_height+PADDING+i,
                                    IN_SPAD_WIDTH-max_j+j)];
                    }
                }

                if (in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset +
                                IN_SPAD_DIM(next_out_spad_height + i + PADDING,
                                    PADDING + ((next_start_img_j > 0) ? PADDING : 0))],
                            &input_image_host[IMG_DIM(next_start_img_i+next_out_spad_height+i,
                                next_start_img_j + ((next_start_img_j>0) ? PADDING : 0))],
                            in_spad_width * sizeof(TYPE));
                }
            }
        }

        for (int img_i = start_img_i-PADDING, in_i = (2-PADDING), out_i = 0;
                out_i < out_spad_height; img_i++, in_i++, out_i++) {
            loop: for (int img_j = start_img_j-PADDING, in_j = (2-PADDING), out_j = 0;
                    out_j < out_spad_width; img_j++, in_j++, out_j++) {
                float partial_sum = 0;

                for (int ki = 0; ki < KERN_HEIGHT; ki++) {
                    bool valid_row = ((img_i+ki) >= 0) && ((img_i+ki) < IMG_HEIGHT);

                    for (int kj = 0; kj < KERN_WIDTH; kj++) {
                        bool valid_col = ((img_j+kj) >= 0) && ((img_j+kj) < IMG_WIDTH);

                        float in_val = (valid_row && valid_col) ? input_image_acc[in_offset +
                            IN_SPAD_DIM(in_i+ki, in_j+kj)] : 0;

                        partial_sum += in_val * kernel_acc[KERN_DIM(ki, kj)];
                    }
                }

                output_image_acc[out_offset + OUT_SPAD_DIM(out_i, out_j)] = partial_sum;
            }

            dmaStore(&output_image_host[IMG_DIM(start_img_i + out_i, start_img_j)],
                     &output_image_acc[out_offset + OUT_SPAD_DIM(out_i, 0)],
                     out_spad_width * sizeof(TYPE));
        }

        start_img_i = next_start_img_i;
        start_img_j = next_start_img_j;
        out_spad_height = next_out_spad_height;
        out_spad_width = next_out_spad_width;
        in_offset = next_in_offset;
        out_offset = next_out_offset;

        next_start_img_j += OUT_SPAD_WIDTH;
        if (next_start_img_j >= IMG_WIDTH) {
            next_start_img_i += OUT_SPAD_HEIGHT;
            next_start_img_j = 0;
        }
    }
}
