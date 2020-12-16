#define IN_TYPE  float
#define OUT_TYPE uint8_t
#define IMG_WIDTH  129
#define IMG_HEIGHT 129
#define DIM(x,y) (((x)*IMG_WIDTH) + (y))

void harris_non_max(IN_TYPE *harris_response, OUT_TYPE *max_values) {
    for (int i = 0; i < IMG_HEIGHT; i += 3) {
        img_loop: for (int j = 0; j < IMG_WIDTH; j += 3) {
            int max_i = -1, max_j = -1;
            IN_TYPE max_value = 0;

            for (int ii = 0; ii < 3; ii++) {
                for (int jj = 0; jj < 3; jj++) {
                    IN_TYPE value = harris_response[DIM(i+ii, j+jj)];
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
                        max_values[index] = 255;
                    } else {
                        max_values[index] = 0;
                    }
                }
            }
        }
    }
}
