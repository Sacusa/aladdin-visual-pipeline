// both dimensions should be multiple of 3
#define HNM_SPAD_WIDTH 27
#define HNM_SPAD_HEIGHT 15
#define HNM_SPAD_DIM(x,y) (((x)*HNM_SPAD_WIDTH) + (y))

void harris_non_max(float *harris_response_host, uint8_t *max_values_host,
                    float *harris_response_acc,  uint8_t *max_values_acc) {
    int num_iters = ((IMG_HEIGHT / HNM_SPAD_HEIGHT) + ((IMG_HEIGHT % HNM_SPAD_HEIGHT) != 0)) *
                    ((IMG_WIDTH  / HNM_SPAD_WIDTH)  + ((IMG_WIDTH  % HNM_SPAD_WIDTH)  != 0));
    int last_iter = num_iters - 1;

    int spad = 0;
    int spad_offset = 0, next_spad_offset = 0;

    int start_img_i = 0, next_start_img_i = 0;
    int start_img_j = 0, next_start_img_j = 0;
    int spad_height = 0, next_spad_height = 0;
    int spad_width  = 0, next_spad_width  = 0;

    next_spad_height = ((next_start_img_i + HNM_SPAD_HEIGHT) > IMG_HEIGHT) ?
                       (IMG_HEIGHT - next_start_img_i) : HNM_SPAD_HEIGHT;
    next_spad_width  = ((next_start_img_j + HNM_SPAD_WIDTH) > IMG_WIDTH) ?
                       (IMG_WIDTH - next_start_img_j) : HNM_SPAD_WIDTH;

    for (int i = 0; i < next_spad_height; i++) {
        dmaLoad(&harris_response_acc[HNM_SPAD_DIM(i,0)], &harris_response_host[DIM(i,0)],
                next_spad_width * sizeof(float));
    }

    next_start_img_j += HNM_SPAD_WIDTH;
    if (next_start_img_j >= IMG_WIDTH) {
        next_start_img_i += HNM_SPAD_HEIGHT;
        next_start_img_j = 0;
    }

    spad_height = next_spad_height;
    spad_width  = next_spad_width;

    for (int iter = 0; iter < num_iters; iter++) {
        if (iter != last_iter) {
            spad ^= 1;
            next_spad_offset = spad * HNM_SPAD_WIDTH * HNM_SPAD_HEIGHT;

            next_spad_height = ((next_start_img_i + HNM_SPAD_HEIGHT) > IMG_HEIGHT) ?
                               (IMG_HEIGHT - next_start_img_i) : HNM_SPAD_HEIGHT;
            next_spad_width  = ((next_start_img_j + HNM_SPAD_WIDTH) > IMG_WIDTH) ?
                               (IMG_WIDTH - next_start_img_j) : HNM_SPAD_WIDTH;

            for (int i = 0; i < next_spad_height; i++) {
                dmaLoad(&harris_response_acc[next_spad_offset + HNM_SPAD_DIM(i,0)],
                        &harris_response_host[DIM(next_start_img_i+i, next_start_img_j)],
                        next_spad_width * sizeof(float));
            }
        }

        for (int img_i = start_img_i, i = 0; i < spad_height; img_i += 3, i += 3) {
            img_loop: for (int img_j = 0, j = 0; j < spad_width; img_j += 3, j += 3) {
                int max_i = -1, max_j = -1;
                float max_value = 0;

                for (int ii = 0; ii < 3; ii++) {
                    for (int jj = 0; jj < 3; jj++) {
                        if (((img_i+ii) < IMG_HEIGHT) && ((img_j+jj) < IMG_WIDTH)) {
                            float value = harris_response_acc[spad_offset +
                                            HNM_SPAD_DIM(i+ii, j+jj)];
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
                            int index = spad_offset + HNM_SPAD_DIM(i+ii, j+jj);

                            if ((ii == max_i) && (jj == max_j)) {
                                max_values_acc[index] = 255;
                            } else {
                                max_values_acc[index] = 0;
                            }
                        }
                    }
                }
            }

            dmaStore(&max_values_host[DIM(start_img_i+i, start_img_j)],
                     &max_values_acc[spad_offset + HNM_SPAD_DIM(i,0)],
                     spad_width * sizeof(uint8_t));
        }

        start_img_i = next_start_img_i;
        start_img_j = next_start_img_j;
        spad_width  = next_spad_width;
        spad_height = next_spad_height;
        spad_offset = next_spad_offset;

        next_start_img_j += HNM_SPAD_WIDTH;
        if (next_start_img_j >= IMG_WIDTH) {
            next_start_img_i += HNM_SPAD_HEIGHT;
            next_start_img_j = 0;
        }
    }
}
