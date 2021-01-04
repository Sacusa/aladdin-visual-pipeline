#define TYPE       uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define DIM(x,y)   (((x)*IMG_WIDTH) + (y))
#define WEAK       25
#define STRONG     255

// every row requires 0.25 KB of storage
// make sure there is at least 0.5 KB of scratchpad
#define SPAD_SIZE_IN_KB 4
#define SPAD_ROWS ((int)((SPAD_SIZE_IN_KB - 0.25) / 0.25))

void edge_tracking(TYPE *input_image_host, TYPE *input_image_acc,
        TYPE thr_weak, TYPE thr_strong,
        TYPE *output_image_host, TYPE *output_image_acc) {
    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;
    int num_iters = (IMG_HEIGHT / SPAD_ROWS) + ((IMG_HEIGHT % SPAD_ROWS) != 0);
    int last_iter = num_iters - 1;

    for (int iter = 0; iter < num_iters; iter++) {
        int in_block_offset = (iter * SPAD_ROWS * IMG_WIDTH) - IMG_WIDTH;
        int in_block_size = (SPAD_ROWS + 2) * IMG_WIDTH;
        int out_block_offset = iter * SPAD_ROWS * IMG_WIDTH;
        int out_block_size = SPAD_ROWS * IMG_WIDTH;

        int start_img_i = iter * SPAD_ROWS;
        int end_img_i = start_img_i + SPAD_ROWS;

        if (in_block_offset < 0) {
            in_block_offset = 0;
            in_block_size -= IMG_WIDTH;
        }

        if (end_img_i >= IMG_HEIGHT) {
            end_img_i = IMG_HEIGHT;
            in_block_size = (end_img_i - start_img_i + 1) * IMG_WIDTH;
            out_block_size = (end_img_i - start_img_i) * IMG_WIDTH;
        }

        dmaLoad(input_image_acc, &input_image_host[in_block_offset], in_block_size * sizeof(TYPE));

        for (int img_i = start_img_i, i = (iter == 0) ? 0 : 1; img_i < end_img_i; img_i++, i++) {
            tr_loop: for (int j = 0; j < IMG_WIDTH; j++) {
                int index = DIM(i,j);

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

        for (int img_i = start_img_i, i = (iter == 0) ? 0 : 1; img_i < end_img_i; img_i++, i++) {
            h_loop: for (int j = 0; j < IMG_WIDTH; j++) {
                int i_min = img_i == 0, i_max = img_i == max_height;
                int j_min = j == 0, j_max = j == max_width;
                int in_index = DIM(i,j);
                int out_index = (iter == 0) ? in_index : DIM(i-1,j);

                if (input_image_acc[in_index] == WEAK) {
                    if (((i_min || j_min) ? 0 : (input_image_acc[DIM(i-1,j-1)] == STRONG)) ||
                        ((i_min)          ? 0 : (input_image_acc[DIM(i-1,j  )] == STRONG)) ||
                        ((i_min || j_max) ? 0 : (input_image_acc[DIM(i-1,j+1)] == STRONG)) ||
                        ((j_min)          ? 0 : (input_image_acc[DIM(i  ,j-1)] == STRONG)) ||
                        ((j_max)          ? 0 : (input_image_acc[DIM(i  ,j+1)] == STRONG)) ||
                        ((i_max || j_min) ? 0 : (input_image_acc[DIM(i+1,j-1)] == STRONG)) ||
                        ((i_max)          ? 0 : (input_image_acc[DIM(i+1,j  )] == STRONG)) ||
                        ((i_max || j_max) ? 0 : (input_image_acc[DIM(i+1,j+1)] == STRONG))) {
                        output_image_acc[in_index] = STRONG;
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

        dmaStore(&output_image_host[out_block_offset], output_image_acc,
                out_block_size * sizeof(TYPE));
    }
}
