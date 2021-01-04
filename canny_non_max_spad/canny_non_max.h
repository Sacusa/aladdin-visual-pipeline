#define HYPO_TYPE  uint8_t
#define THTA_TYPE  float
#define OUT_TYPE   uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define PI         3.141592653589793238462643
#define DIM(x,y)   (((x)*IMG_WIDTH) + (y))

// every row requires 0.75 KB of storage
// make sure there is at least 1 KB of scratchpad
#define SPAD_SIZE_IN_KB 1.75
#define SPAD_ROWS ((int)((SPAD_SIZE_IN_KB - 0.25) / 0.75))

void canny_non_max(HYPO_TYPE *hypotenuse_host, THTA_TYPE *theta_host, OUT_TYPE *result_host,
        HYPO_TYPE *hypotenuse_acc, THTA_TYPE *theta_acc, OUT_TYPE *result_acc) {
    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;
    int num_iters = (IMG_HEIGHT / SPAD_ROWS) + ((IMG_HEIGHT % SPAD_ROWS) != 0);

    for (int iter = 0; iter < num_iters; iter++) {
        int h_block_offset = (iter * SPAD_ROWS * IMG_WIDTH) - IMG_WIDTH;
        int h_block_size = (SPAD_ROWS + 2) * IMG_WIDTH;
        int tr_block_offset = iter * SPAD_ROWS * IMG_WIDTH;
        int tr_block_size = SPAD_ROWS * IMG_WIDTH;

        int start_img_i = iter * SPAD_ROWS;
        int end_img_i = start_img_i + SPAD_ROWS;

        if (h_block_offset < 0) {
            h_block_offset = 0;
            h_block_size -= IMG_WIDTH;
        }

        if (end_img_i >= IMG_HEIGHT) {
            end_img_i = IMG_HEIGHT;
            h_block_size = (end_img_i - start_img_i + 1) * IMG_WIDTH;
            tr_block_size = h_block_size - IMG_WIDTH;
        }

        dmaLoad(hypotenuse_acc, &hypotenuse_host[h_block_offset],
                h_block_size * sizeof(HYPO_TYPE));
        dmaLoad(theta_acc, &theta_host[tr_block_offset],
                tr_block_size * sizeof(THTA_TYPE));

        for (int img_i = start_img_i, h_i = (iter == 0) ? 0 : 1, tr_i = 0; img_i < end_img_i;
                img_i++, h_i++, tr_i++) {
            loop: for (int j = 0; j < IMG_WIDTH; j++) {
                float q = 0, r = 0;
                int h_index = DIM(h_i,j);
                int tr_index = DIM(tr_i,j);

                float angle = theta_acc[tr_index] * 180 / PI;
                if (angle < 0) { angle += 180; }

                // angle 0
                if (((angle >= 0)     && (angle < 22.5)) ||
                    ((angle >= 157.5) && (angle <= 180))) {
                    q = (j == max_width) ? 0 : hypotenuse_acc[DIM(h_i,j+1)];
                    r = (j == 0)         ? 0 : hypotenuse_acc[DIM(h_i,j-1)];
                }

                // angle 45
                else if ((angle >= 22.5) && (angle < 67.5)) {
                    q = ((img_i == max_height) || (j == 0)) ? 0 : hypotenuse_acc[DIM(h_i+1,j-1)];
                    r = ((img_i == 0) || (j == max_width))  ? 0 : hypotenuse_acc[DIM(h_i-1,j+1)];
                }

                // angle 90
                else if ((angle >= 67.5) && (angle < 112.5)) {
                    q = (img_i == max_height) ? 0 : hypotenuse_acc[DIM(h_i+1,j)];
                    r = (img_i == 0)          ? 0 : hypotenuse_acc[DIM(h_i-1,j)];
                }

                // angle 135
                else if ((angle >= 112.5) && (angle < 157.5)) {
                    q = ((img_i == 0) || (j == 0)) ? 0 : hypotenuse_acc[DIM(h_i-1,j-1)];
                    r = ((img_i == max_height) || (j == max_width)) ?
                        0 : hypotenuse_acc[DIM(h_i+1,j+1)];
                }

                if ((hypotenuse_acc[h_index] >= q) && (hypotenuse_acc[h_index] >= r)) {
                    result_acc[tr_index] = (uint8_t) hypotenuse_acc[h_index];
                }
                else {
                    result_acc[tr_index] = 0;
                }
            }
        }

        dmaStore(&result_host[tr_block_offset], result_acc, tr_block_size * sizeof(OUT_TYPE));
    }
}
