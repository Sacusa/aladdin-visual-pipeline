#define WEAK   25
#define STRONG 255

void edge_tracking(uint8_t *input_image, uint8_t thr_weak, uint8_t thr_strong,
        uint8_t *output_image) {
    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;

    for (int i = 0; i < IMG_HEIGHT; i++) {
        tr_loop: for (int j = 0; j < IMG_WIDTH; j++) {
            int index = DIM(i,j);

            if (input_image[index] < thr_weak) {
                input_image[index] = 0;
            }
            else if (input_image[i] >= thr_strong) {
                input_image[index] = STRONG;
            }
            else {
                input_image[index] = WEAK;
            }
        }
    }

    for (int i = 0; i < IMG_HEIGHT; i++) {
        h_loop: for (int j = 0; j < IMG_WIDTH; j++) {
            int i_min = i == 0, i_max = i == max_height;
            int j_min = j == 0, j_max = j == max_width;
            int index = DIM(i,j);

            if (input_image[index] == WEAK) {
                if (((i_min || j_min) ? 0 : (input_image[DIM(i-1,j-1)] == STRONG)) ||
                    ((i_min)          ? 0 : (input_image[DIM(i-1,j  )] == STRONG)) ||
                    ((i_min || j_max) ? 0 : (input_image[DIM(i-1,j+1)] == STRONG)) ||
                    ((j_min)          ? 0 : (input_image[DIM(i  ,j-1)] == STRONG)) ||
                    ((j_max)          ? 0 : (input_image[DIM(i  ,j+1)] == STRONG)) ||
                    ((i_max || j_min) ? 0 : (input_image[DIM(i+1,j-1)] == STRONG)) ||
                    ((i_max)          ? 0 : (input_image[DIM(i+1,j  )] == STRONG)) ||
                    ((i_max || j_max) ? 0 : (input_image[DIM(i+1,j+1)] == STRONG))) {
                    output_image[index] = STRONG;
                }
                else {
                    output_image[index] = 0;
                }
            }
            else {
                output_image[index] = input_image[index];
            }
        }
    }
}
