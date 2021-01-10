#define HYPO_TYPE  uint8_t
#define THTA_TYPE  float
#define OUT_TYPE   uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define PI         3.141592653589793238462643

#define TR_SPAD_DIM 100
#define H_SPAD_DIM  (TR_SPAD_DIM  + 2)

#define IMG_DIM(x,y) (((x)*IMG_WIDTH)   + (y))
#define H_DIM(x,y)   (((x)*H_SPAD_DIM)  + (y))
#define TR_DIM(x,y)  (((x)*TR_SPAD_DIM) + (y))

void canny_non_max(HYPO_TYPE *hypotenuse_host, THTA_TYPE *theta_host, OUT_TYPE *result_host,
        HYPO_TYPE *hypotenuse_acc, THTA_TYPE *theta_acc, OUT_TYPE *result_acc) {
    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;
    int num_iters = ((IMG_HEIGHT / TR_SPAD_DIM) + ((IMG_HEIGHT % TR_SPAD_DIM) != 0)) *
                    ((IMG_WIDTH  / TR_SPAD_DIM) + ((IMG_WIDTH  % TR_SPAD_DIM) != 0));

    int start_img_i = 0, start_img_j = 0;

    for (int iter = 0; iter < num_iters; iter++) {
        int tr_spad_height = ((start_img_i + TR_SPAD_DIM) > IMG_HEIGHT) ?
                             (IMG_HEIGHT - start_img_i) : TR_SPAD_DIM;
        int tr_spad_width  = ((start_img_j + TR_SPAD_DIM) > IMG_WIDTH) ?
                             (IMG_WIDTH - start_img_j) : TR_SPAD_DIM;

        int h_spad_width = tr_spad_width + (start_img_j == 0) +
                           ((start_img_j+tr_spad_width) < max_width);
        h_spad_width -= (start_img_j == 0) ? 0 : 1;

        for (int i = 0; i < tr_spad_height; i++) {
            if (start_img_j > 0) {
                hypotenuse_acc[H_DIM(i+1,0)] = hypotenuse_acc[H_DIM(i+1,H_SPAD_DIM-2)];
                hypotenuse_acc[H_DIM(i+1,1)] = hypotenuse_acc[H_DIM(i+1,H_SPAD_DIM-1)];
            }

            if (h_spad_width > 0) {
                dmaLoad(&hypotenuse_acc[H_DIM(i+1, 1 + (start_img_j > 0))],
                        &hypotenuse_host[IMG_DIM(start_img_i+i, start_img_j)],
                        h_spad_width * sizeof(HYPO_TYPE));
            }
            dmaLoad(&theta_acc[i*TR_SPAD_DIM],
                    &theta_host[IMG_DIM(start_img_i+i, start_img_j)],
                    tr_spad_width * sizeof(THTA_TYPE));
        }

        for (int img_i = start_img_i, h_i = 1, tr_i = 0; tr_i < tr_spad_height;
                img_i++, h_i++, tr_i++) {
            loop: for (int img_j = start_img_j, h_j = 1, tr_j = 0; tr_j < tr_spad_width;
                    img_j++, h_j++, tr_j++) {
                float q = 0, r = 0;
                int h_index = H_DIM(h_i, h_j);
                int tr_index = TR_DIM(tr_i, tr_j);

                float angle = theta_acc[tr_index] * 180 / PI;
                if (angle < 0) { angle += 180; }

                // angle 0
                if (((angle >= 0)     && (angle < 22.5)) ||
                    ((angle >= 157.5) && (angle <= 180))) {
                    q = (img_j == max_width) ? 0 : hypotenuse_acc[H_DIM(h_i,h_j+1)];
                    r = (img_j == 0)         ? 0 : hypotenuse_acc[H_DIM(h_i,h_j-1)];
                }

                // angle 45
                else if ((angle >= 22.5) && (angle < 67.5)) {
                    q = ((img_i == max_height) || (img_j == 0)) ?
                        0 : hypotenuse_acc[H_DIM(h_i+1,h_j-1)];
                    r = ((img_i == 0) || (img_j == max_width)) ?
                        0 : hypotenuse_acc[H_DIM(h_i-1,h_j+1)];
                }

                // angle 90
                else if ((angle >= 67.5) && (angle < 112.5)) {
                    q = (img_i == max_height) ? 0 : hypotenuse_acc[H_DIM(h_i+1,h_j)];
                    r = (img_i == 0)          ? 0 : hypotenuse_acc[H_DIM(h_i-1,h_j)];
                }

                // angle 135
                else if ((angle >= 112.5) && (angle < 157.5)) {
                    q = ((img_i == 0) || (img_j == 0)) ? 0 : hypotenuse_acc[H_DIM(h_i-1,h_j-1)];
                    r = ((img_i == max_height) || (img_j == max_width)) ?
                        0 : hypotenuse_acc[H_DIM(h_i+1,h_j+1)];
                }

                if ((hypotenuse_acc[h_index] >= q) && (hypotenuse_acc[h_index] >= r)) {
                    result_acc[tr_index] = (uint8_t) hypotenuse_acc[h_index];
                }
                else {
                    result_acc[tr_index] = 0;
                }
            }
        }

        for (int i = 0; i < tr_spad_height; i++) {
            dmaStore(&result_host[IMG_DIM(start_img_i+i, start_img_j)],
                     &result_acc[i*TR_SPAD_DIM],
                     tr_spad_width * sizeof(OUT_TYPE));
        }

        start_img_j += TR_SPAD_DIM;
        if (start_img_j >= IMG_WIDTH) {
            start_img_i += TR_SPAD_DIM;
            start_img_j = 0;
        }
    }
}
