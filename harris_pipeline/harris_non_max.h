#define HDIM(x,y) (((x)*width) + (y))
#define MDIM(x,y) (((x)*IMG_WIDTH) + (y))

/**
 * Padding
 * Each dimension needs to be a multiple of 3
 */

void harris_non_max(float *harris_response_host, uint8_t *max_values_host,
                    float *harris_response_acc,  uint8_t *max_values_acc,
                    int harris_response_size, int max_values_size) {
    dmaLoad(harris_response_acc, harris_response_host, harris_response_size);

    int height = IMG_HEIGHT + 3 - (IMG_HEIGHT % 3);
    int width  = IMG_WIDTH  + 3 - (IMG_WIDTH  % 3);

    for (int i = 0; i < height; i += 3) {
        img_loop: for (int j = 0; j < width; j += 3) {
            int max_i = -1, max_j = -1;
            float max_value = 0;

            for (int ii = 0; ii < 3; ii++) {
                    if (value > max_value) {
                        max_i = ii;
                        max_j = jj;
                        max_value = value;
                    }
                }
            }

            w_loop1: for (int ii = 0; ii < 3; ii++) {
                w_loop2: for (int jj = 0; jj < 3; jj++) {
                    int index = MDIM(i+ii, j+jj);
                    if ((ii == max_i) && (jj == max_j)) {
                        max_values_acc[index] = 255;
                    } else {
                        max_values_acc[index] = 0;
                    }
                }
            }
        }
    }

    dmaStore(max_values_host, max_values_acc, max_values_size);
}
