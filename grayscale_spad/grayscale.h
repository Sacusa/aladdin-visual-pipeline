#define TYPE uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

#define SPAD_WIDTH 16
#define SPAD_HEIGHT 16

#define IMG_DIM(x,y)      (((x)*IMG_WIDTH)    +  (y))
#define OUT_SPAD_DIM(x,y) (((x)*SPAD_WIDTH)   +  (y))
#define IN_SPAD_DIM(x,y)  (3 * OUT_SPAD_DIM(x,y))

void grayscale(TYPE *input_image_host, TYPE *output_image_host,
               TYPE *input_image_acc,  TYPE *output_image_acc) {
    int num_iters = ((IMG_HEIGHT / SPAD_HEIGHT) + ((IMG_HEIGHT % SPAD_HEIGHT) != 0)) *
                    ((IMG_WIDTH  / SPAD_WIDTH)  + ((IMG_WIDTH  % SPAD_WIDTH)  != 0));
    int last_iter = num_iters - 1;

    int spad = 0;
    int spad_offset = 0, next_spad_offset = 0;

    int start_img_i = 0, next_start_img_i = 0;
    int start_img_j = 0, next_start_img_j = 0;
    int spad_height = 0, next_spad_height = 0;
    int spad_width  = 0, next_spad_width  = 0;

    next_spad_height = ((next_start_img_i + SPAD_HEIGHT) > IMG_HEIGHT) ?
                       (IMG_HEIGHT - next_start_img_i) : SPAD_HEIGHT;
    next_spad_width  = ((next_start_img_j + SPAD_WIDTH) > IMG_WIDTH) ?
                       (IMG_WIDTH - next_start_img_j) : SPAD_WIDTH;

    for (int i = 0; i < next_spad_height; i++) {
        dmaLoad(&input_image_acc[IN_SPAD_DIM(i,0)], &input_image_host[IMG_DIM(i,0)],
                next_spad_width * 3 * sizeof(TYPE));
    }

    next_start_img_j += SPAD_WIDTH;
    if (next_start_img_j >= IMG_WIDTH) {
        next_start_img_i += SPAD_HEIGHT;
        next_start_img_j = 0;
    }

    spad_height = next_spad_height;
    spad_width  = next_spad_width;

    for (int iter = 0; iter < num_iters; iter++) {
        if (iter != last_iter) {
            spad ^= 1;
            next_spad_offset = spad * SPAD_WIDTH * SPAD_HEIGHT;

            next_spad_height = ((next_start_img_i + SPAD_HEIGHT) > IMG_HEIGHT) ?
                               (IMG_HEIGHT - next_start_img_i) : SPAD_HEIGHT;
            next_spad_width  = ((next_start_img_j + SPAD_WIDTH) > IMG_WIDTH) ?
                               (IMG_WIDTH - next_start_img_j) : SPAD_WIDTH;

            for (int i = 0; i < next_spad_height; i++) {
                dmaLoad(&input_image_acc[next_spad_offset + IN_SPAD_DIM(i,0)],
                        &input_image_host[IMG_DIM(next_start_img_i + i, next_start_img_j)],
                        next_spad_width * 3 * sizeof(TYPE));
            }
        }

        for (int i = 0; i < spad_height; i++) {
            loop: for (int j = 0; j < spad_width; j++) {
                int in_index = IN_SPAD_DIM(i, j);
                int out_index = OUT_SPAD_DIM(i, j);

                output_image_acc[spad_offset + OUT_SPAD_DIM(i, j)] =
                    (input_image_acc[spad_offset + in_index]   * 0.2126) +
                    (input_image_acc[spad_offset + in_index+1] * 0.7152) +
                    (input_image_acc[spad_offset + in_index+2] * 0.0722);
            }

            dmaStore(&output_image_host[IMG_DIM(start_img_i + i, start_img_j)],
                     &output_image_acc[spad_offset + OUT_SPAD_DIM(i,0)],
                     spad_width * sizeof(TYPE));
        }

        start_img_i = next_start_img_i;
        start_img_j = next_start_img_j;
        spad_width  = next_spad_width;
        spad_height = next_spad_height;
        spad_offset = next_spad_offset;

        next_start_img_j += SPAD_WIDTH;
        if (next_start_img_j >= IMG_WIDTH) {
            next_start_img_i += SPAD_HEIGHT;
            next_start_img_j = 0;
        }
    }
}
