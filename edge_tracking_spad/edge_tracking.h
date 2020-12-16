#define TYPE         uint8_t
#define IMG_WIDTH    128
#define IMG_HEIGHT   128
#define IN_DIM(x,y)  (IMG_WIDTH + 3 + ((x)*2) + (x*IMG_WIDTH) + (y))
#define OUT_DIM(x,y) (((x)*IMG_WIDTH) + (y))
#define WEAK         25
#define STRONG       255

void edge_tracking(TYPE *input_image_host, TYPE *input_image_acc,
        TYPE thr_weak, TYPE thr_strong,
        TYPE *output_image_host, TYPE *output_image_acc,
        int input_image_size, int output_image_size) {
    dmaLoad(input_image_acc, input_image_host, input_image_size);

    for (int i = 0; i < IMG_HEIGHT; i++) {
        tr_loop: for (int j = 0; j < IMG_WIDTH; j++) {
            int index = IN_DIM(i,j);

            if (input_image_acc[index] < thr_weak) {
                input_image_acc[index] = 0;
            }
            else if (input_image_acc[i] >= thr_strong) {
                input_image_acc[index] = STRONG;
            }
            else {
                input_image_acc[index] = WEAK;
            }
        }
    }

    for (int i = 0; i < IMG_HEIGHT; i++) {
        h_loop: for (int j = 0; j < IMG_WIDTH; j++) {
            int in_index = IN_DIM(i,j);
            int out_index = OUT_DIM(i,j);

            if (input_image_acc[in_index] == WEAK) {
                if ((input_image_acc[IN_DIM(i-1,j-1)] == STRONG) ||
                    (input_image_acc[IN_DIM(i-1,j  )] == STRONG) ||
                    (input_image_acc[IN_DIM(i-1,j+1)] == STRONG) ||
                    (input_image_acc[IN_DIM(i  ,j-1)] == STRONG) ||
                    (input_image_acc[IN_DIM(i  ,j+1)] == STRONG) ||
                    (input_image_acc[IN_DIM(i+1,j-1)] == STRONG) ||
                    (input_image_acc[IN_DIM(i+1,j  )] == STRONG) ||
                    (input_image_acc[IN_DIM(i+1,j+1)] == STRONG)) {
                    output_image_acc[out_index] = STRONG;
                }
                else {
                    output_image_acc[out_index] = 0;
                }
            }
            else {
                output_image_acc[out_index] = input_image_acc[in_index];
            }
        }
    }

    dmaStore(output_image_host, output_image_acc, output_image_size);
}
