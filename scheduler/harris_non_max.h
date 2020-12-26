/**
 * No padding needed
 */

void harris_non_max(float *harris_response_host, uint8_t *max_values_host,
                    float *harris_response_acc,  uint8_t *max_values_acc) {
    int height = IMG_HEIGHT - (IMG_HEIGHT % 3);
    int width  = IMG_WIDTH  - (IMG_WIDTH  % 3);

    dmaLoad(harris_response_acc, harris_response_host, height*width*4);

    for (int i = 0; i < height; i += 3) {
        img_loop: for (int j = 0; j < width; j += 3) {
            int max_i = -1, max_j = -1;
            float max_value = 0;

            for (int ii = 0; ii < 3; ii++) {
                for (int jj = 0; jj < 3; jj++) {
                    float value = harris_response_acc[DIM(i+ii, j+jj)];
                    if (value > max_value) {
                        max_i = ii;
                        max_j = jj;
                        max_value = value;
                    }
                }
            }

            w_loop1: for (int ii = 0; ii < 3; ii++) {
                w_loop2: for (int jj = 0; jj < 3; jj++) {
                    int index = DIM(i+ii, j+jj);
                    if ((ii == max_i) && (jj == max_j)) {
                        max_values_acc[index] = 255;
                    } else {
                        max_values_acc[index] = 0;
                    }
                }
            }
        }
    }

    // TODO: handle last row/column

    dmaStore(max_values_host, max_values_acc, height*width);
}
