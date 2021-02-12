#define IN_TYPE  float
#define OUT_TYPE uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

// both dimensions should be multiple of 3
#define SPAD_WIDTH 27
#define SPAD_HEIGHT 15

#define IMG_DIM(x,y)  (((x)*IMG_WIDTH)  + (y))
#define SPAD_DIM(x,y) (((x)*SPAD_WIDTH) + (y))

void harris_non_max(IN_TYPE *harris_response_host, OUT_TYPE *max_values_host,
                    IN_TYPE *harris_response_acc,  OUT_TYPE *max_values_acc) {
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
        dmaLoad(&harris_response_acc[SPAD_DIM(i,0)], &harris_response_host[IMG_DIM(i,0)],
                next_spad_width * sizeof(IN_TYPE));
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
                dmaLoad(&harris_response_acc[next_spad_offset + SPAD_DIM(i,0)],
                        &harris_response_host[IMG_DIM(next_start_img_i+i, next_start_img_j)],
                        next_spad_width * sizeof(IN_TYPE));
            }
        }

        for (int img_i = start_img_i, i = 0; i < spad_height; img_i += 3, i += 3) {
            img_loop: for (int img_j = 0, j = 0; j < spad_width; img_j += 3, j += 3) {
                int max_i = -1, max_j = -1;
                IN_TYPE max_value = 0;

                for (int ii = 0; ii < 3; ii++) {
                    for (int jj = 0; jj < 3; jj++) {
                        if (((img_i+ii) < IMG_HEIGHT) && ((img_j+jj) < IMG_WIDTH)) {
                            IN_TYPE value = harris_response_acc[spad_offset +
                                            SPAD_DIM(i+ii, j+jj)];
                            if (value > max_value) {
                                max_i = ii;
                                max_j = jj;
                                max_value = value;
                            }
                        }
                    }
                }

                w_loop1: for (int ii = 0; ii < 3; ii++) {
                    w_loop2: for (int jj = 0; jj < 3; jj++) {
                        if (((img_i+ii) < IMG_HEIGHT) && ((img_j+jj) < IMG_WIDTH)) {
                            int index = spad_offset + SPAD_DIM(i+ii, j+jj);

                            if ((ii == max_i) && (jj == max_j)) {
                                max_values_acc[index] = 255;
                            } else {
                                max_values_acc[index] = 0;
                            }
                        }
                    }
                }
            }

            dmaStore(&max_values_host[IMG_DIM(start_img_i+i, start_img_j)],
                     &max_values_acc[spad_offset + SPAD_DIM(i,0)],
                     spad_width * sizeof(OUT_TYPE));
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
