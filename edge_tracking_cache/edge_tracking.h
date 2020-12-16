#define TYPE         uint8_t
#define IMG_WIDTH    128
#define IMG_HEIGHT   128
#define IN_DIM(x,y)  (IMG_WIDTH + 3 + (x*2) + (x*IMG_WIDTH) + y)
#define OUT_DIM(x,y) ((x*IMG_WIDTH) + y)
#define WEAK         25
#define STRONG       255

void edge_tracking(TYPE *input_image, TYPE thr_weak, TYPE thr_strong,
        TYPE *output_image) {
    for (int i = 0; i < IMG_HEIGHT; i++) {
        tr_loop: for (int j = 0; j < IMG_WIDTH; j++) {
            int index = IN_DIM(i,j);

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
            int in_index = IN_DIM(i,j);
            int out_index = OUT_DIM(i,j);

            if (input_image[in_index] == WEAK) {
                if ((input_image[IN_DIM(i-1,j-1)] == STRONG) ||
                    (input_image[IN_DIM(i-1,j  )] == STRONG) ||
                    (input_image[IN_DIM(i-1,j+1)] == STRONG) ||
                    (input_image[IN_DIM(i  ,j-1)] == STRONG) ||
                    (input_image[IN_DIM(i  ,j+1)] == STRONG) ||
                    (input_image[IN_DIM(i+1,j-1)] == STRONG) ||
                    (input_image[IN_DIM(i+1,j  )] == STRONG) ||
                    (input_image[IN_DIM(i+1,j+1)] == STRONG)) {
                    output_image[out_index] = STRONG;
                }
                else {
                    output_image[out_index] = 0;
                }
            }
            else {
                output_image[out_index] = input_image[in_index];
            }
        }
    }
}
