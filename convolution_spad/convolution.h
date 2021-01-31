#define TYPE float
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define KERN_DIM(x,y) (((x)*kern_width) + (y))
#define IMG_DIM(x,y)  (((x)*IMG_WIDTH)  + (y))

#define OUT_SPAD_HEIGHT 10
#define OUT_SPAD_WIDTH 10
#define OUT_SPAD_DIM(x,y) (((x)*OUT_SPAD_WIDTH) + (y))

#define IN_SPAD_HEIGHT (OUT_SPAD_HEIGHT + 4)
#define IN_SPAD_WIDTH (OUT_SPAD_WIDTH + 4)
#define IN_SPAD_DIM(x,y) (((x)*IN_SPAD_WIDTH) + (y))

/**
 * No padded required
 *
 * Max. kernel size = 5x5
 */

void convolution(TYPE *input_image_host, TYPE *kernel_host, TYPE *output_image_host,
        TYPE *input_image_acc, TYPE *kernel_acc, TYPE *output_image_acc,
        int kern_width, int kern_height) {
    int spad = 0;
    int in_offset = 0, next_in_offset = 0;
    int out_offset = 0, next_out_offset = 0;

    int padding = (kern_height - 1) / 2;

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
    in_spad_width = next_out_spad_width + padding;

    // load the kernel
    dmaLoad(kernel_acc, kernel_host, kern_width * kern_height * 4);

    // load the image
    for (int i = 0; i < next_out_spad_height; i++) {
        dmaLoad(&input_image_acc[IN_SPAD_DIM(i+padding,padding)],
                &input_image_host[IMG_DIM(i,0)],
                in_spad_width * sizeof(TYPE));
    }

    // load the lower buffer row(s)
    for (int i = 0; i < padding; i++) {
        dmaLoad(&input_image_acc[IN_SPAD_DIM(next_out_spad_height+padding+i, padding)],
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
                in_spad_width = next_out_spad_width + padding;
            }
            else {
                in_spad_width = next_out_spad_width;
                if ((next_start_img_j + next_out_spad_width + padding) > IMG_WIDTH) {
                    in_spad_width = IMG_WIDTH - next_start_img_j - padding;
                }
            }

            // load the upper buffer row(s)
            //int start_i = padding -
            //    (((next_start_img_i - padding) > 0) ? padding : next_start_img_i);
            int start_i = (next_start_img_i == 0) ? padding : 0;
            for (int i = start_i; i < padding; i++) {
                if (next_start_img_j > 0) {
                    int max_j = padding<<1;
                    for (int j = 0; j < max_j; j++) {
                        input_image_acc[next_in_offset + IN_SPAD_DIM(i,j)] =
                            input_image_acc[in_offset + IN_SPAD_DIM(i,IN_SPAD_WIDTH-max_j+j)];
                    }
                }

                if (in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset + IN_SPAD_DIM(i,
                                padding + ((next_start_img_j > 0) ? padding : 0))],
                            &input_image_host[IMG_DIM(next_start_img_i-padding+i,
                                next_start_img_j + ((next_start_img_j>0) ? padding : 0))],
                            in_spad_width * sizeof(TYPE));
                }
            }

            for (int i = 0; i < next_out_spad_height; i++) {
                if (next_start_img_j > 0) {
                    int max_j = padding<<1;
                    for (int j = 0; j < max_j; j++) {
                        input_image_acc[next_in_offset + IN_SPAD_DIM(i+padding,j)] =
                            input_image_acc[in_offset + IN_SPAD_DIM(i+padding,
                                                                    IN_SPAD_WIDTH-max_j+j)];
                    }
                }

                if (in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset + IN_SPAD_DIM(i+padding,
                                padding + ((next_start_img_j > 0) ? padding : 0))],
                            &input_image_host[IMG_DIM(next_start_img_i+i,
                                next_start_img_j + ((next_start_img_j>0) ? padding : 0))],
                            in_spad_width * sizeof(TYPE));
                }
            }

            // load the lower buffer row(s)
            int end_i = IMG_HEIGHT - next_start_img_i - next_out_spad_height;
            end_i = (end_i > padding) ? padding : end_i;
            for (int i = 0; i < end_i; i++) {
                if (next_start_img_j > 0) {
                    int max_j = padding<<1;
                    for (int j = 0; j < max_j; j++) {
                        input_image_acc[next_in_offset + IN_SPAD_DIM(next_out_spad_height+padding+i,j)] =
                            input_image_acc[in_offset + IN_SPAD_DIM(next_out_spad_height+padding+i,
                                    IN_SPAD_WIDTH-max_j+j)];
                    }
                }

                if (in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset +
                                IN_SPAD_DIM(next_out_spad_height + i + padding,
                                    padding + ((next_start_img_j > 0) ? padding : 0))],
                            &input_image_host[IMG_DIM(next_start_img_i+next_out_spad_height+i,
                                next_start_img_j + ((next_start_img_j>0) ? padding : 0))],
                            in_spad_width * sizeof(TYPE));
                }
            }
        }

        for (int img_i = start_img_i-padding, in_i = (2-padding), out_i = 0;
                out_i < out_spad_height; img_i++, in_i++, out_i++) {
            loop: for (int img_j = start_img_j-padding, in_j = (2-padding), out_j = 0;
                    out_j < out_spad_width; img_j++, in_j++, out_j++) {
                float partial_sum = 0;

                for (int ki = 0; ki < kern_height; ki++) {
                    bool valid_row = ((img_i+ki) >= 0) && ((img_i+ki) < IMG_HEIGHT);

                    for (int kj = 0; kj < kern_width; kj++) {
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
