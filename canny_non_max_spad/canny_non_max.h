#define HYPO_TYPE  uint8_t
#define THTA_TYPE  float
#define OUT_TYPE   uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define PI         3.141592653589793238462643

//#define TR_SPAD_DIM 37
//#define H_SPAD_DIM  (TR_SPAD_DIM + 2)

#define TR_SPAD_WIDTH 95
#define TR_SPAD_HEIGHT 57
#define H_SPAD_WIDTH  (TR_SPAD_WIDTH  + 2)
#define H_SPAD_HEIGHT (TR_SPAD_HEIGHT + 2)

#define IMG_DIM(x,y) (((x)*IMG_WIDTH)     + (y))
#define H_DIM(x,y)   (((x)*H_SPAD_WIDTH)  + (y))
#define TR_DIM(x,y)  (((x)*TR_SPAD_WIDTH) + (y))

void canny_non_max(HYPO_TYPE *hypotenuse_host, THTA_TYPE *theta_host, OUT_TYPE *result_host,
        HYPO_TYPE *hypotenuse_acc, THTA_TYPE *theta_acc, OUT_TYPE *result_acc) {
    int spad = 0;
    int curr_h_offset, next_h_offset = 0;
    int curr_tr_offset, next_tr_offset = 0;

    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;
    int num_iters = ((IMG_HEIGHT / TR_SPAD_HEIGHT) + ((IMG_HEIGHT % TR_SPAD_HEIGHT) != 0)) *
                    ((IMG_WIDTH  / TR_SPAD_WIDTH)  + ((IMG_WIDTH  % TR_SPAD_WIDTH)  != 0));
    int last_iter = num_iters - 1;

    int start_img_i = 0, next_start_img_i = 0;
    int start_img_j = 0, next_start_img_j = 0;
    int tr_spad_height, next_tr_spad_height;
    int tr_spad_width,  next_tr_spad_width;
    int h_spad_width;

    next_tr_spad_height = ((next_start_img_i + TR_SPAD_HEIGHT) > IMG_HEIGHT) ?
                          (IMG_HEIGHT - next_start_img_i) : TR_SPAD_HEIGHT;
    next_tr_spad_width  = ((next_start_img_j + TR_SPAD_WIDTH) > IMG_WIDTH) ?
                          (IMG_WIDTH - next_start_img_j) : TR_SPAD_WIDTH;
    h_spad_width = next_tr_spad_width + 1;

    for (int i = 0; i < next_tr_spad_height; i++) {
        dmaLoad(&hypotenuse_acc[H_DIM(i+1, 1)], &hypotenuse_host[IMG_DIM(i, 0)],
                h_spad_width * sizeof(HYPO_TYPE));

        dmaLoad(&theta_acc[TR_DIM(i,0)], &theta_host[IMG_DIM(i, 0)],
                next_tr_spad_width * sizeof(THTA_TYPE));
    }

    // load the lower buffer row
    if (next_tr_spad_height < max_height) {
        dmaLoad(&hypotenuse_acc[H_DIM(next_tr_spad_height+1, 1)],
                &hypotenuse_host[IMG_DIM(next_tr_spad_height, 0)],
                h_spad_width * sizeof(HYPO_TYPE));
    }

    next_start_img_j += TR_SPAD_WIDTH;
    if (next_start_img_j >= IMG_WIDTH) {
        next_start_img_i += TR_SPAD_HEIGHT;
        next_start_img_j = 0;
    }

    tr_spad_height = next_tr_spad_height;
    tr_spad_width  = next_tr_spad_width;
    curr_h_offset = next_h_offset;
    curr_tr_offset = next_tr_offset;

    for (int iter = 0; iter < num_iters; iter++) {
        if (iter != last_iter) {
            spad ^= 1;
            next_h_offset = spad * H_SPAD_WIDTH * H_SPAD_HEIGHT;
            next_tr_offset = spad * TR_SPAD_WIDTH * TR_SPAD_HEIGHT;

            next_tr_spad_height = ((next_start_img_i + TR_SPAD_HEIGHT) > IMG_HEIGHT) ?
                                  (IMG_HEIGHT - next_start_img_i) : TR_SPAD_HEIGHT;
            next_tr_spad_width  = ((next_start_img_j + TR_SPAD_WIDTH) > IMG_WIDTH) ?
                                  (IMG_WIDTH - next_start_img_j) : TR_SPAD_WIDTH;

            if (next_start_img_j == 0) {
                h_spad_width = next_tr_spad_width + 1;
            }
            else {
                h_spad_width = next_tr_spad_width;
                if ((next_start_img_j + 1 + h_spad_width) > IMG_WIDTH) {
                    h_spad_width = IMG_WIDTH - next_start_img_j - 1;
                }
            }

            // load the upper buffer row
            if (next_start_img_i > 0) {
                if (next_start_img_j > 0) {
                    hypotenuse_acc[next_h_offset + H_DIM(0,0)] =
                        hypotenuse_acc[curr_h_offset + H_DIM(0,H_SPAD_WIDTH-2)];
                    hypotenuse_acc[next_h_offset + H_DIM(0,1)] =
                        hypotenuse_acc[curr_h_offset + H_DIM(0,H_SPAD_WIDTH-1)];
                }

                if (h_spad_width > 0) {
                    dmaLoad(&hypotenuse_acc[next_h_offset + H_DIM(0, 1 + (next_start_img_j>0))],
                            &hypotenuse_host[IMG_DIM(next_start_img_i-1,
                                next_start_img_j+(next_start_img_j>0))],
                            h_spad_width * sizeof(HYPO_TYPE));
                }
            }

            for (int i = 0; i < next_tr_spad_height; i++) {
                if (next_start_img_j > 0) {
                    hypotenuse_acc[next_h_offset + H_DIM(i+1,0)] =
                        hypotenuse_acc[curr_h_offset + H_DIM(i+1,H_SPAD_WIDTH-2)];
                    hypotenuse_acc[next_h_offset + H_DIM(i+1,1)] =
                        hypotenuse_acc[curr_h_offset + H_DIM(i+1,H_SPAD_WIDTH-1)];
                }

                if (h_spad_width > 0) {
                    dmaLoad(&hypotenuse_acc[next_h_offset + H_DIM(i+1, 1 + (next_start_img_j>0))],
                            &hypotenuse_host[IMG_DIM(next_start_img_i+i,
                                next_start_img_j+(next_start_img_j>0))],
                            h_spad_width * sizeof(HYPO_TYPE));
                }

                dmaLoad(&theta_acc[next_tr_offset + TR_DIM(i,0)],
                        &theta_host[IMG_DIM(next_start_img_i+i, next_start_img_j)],
                        next_tr_spad_width * sizeof(THTA_TYPE));
            }

            // load the lower buffer row
            if ((next_start_img_i + next_tr_spad_height) < max_height) {
                if (next_start_img_j > 0) {
                    hypotenuse_acc[next_h_offset + H_DIM(next_tr_spad_height+1,0)] =
                        hypotenuse_acc[curr_h_offset +
                                       H_DIM(next_tr_spad_height+1,H_SPAD_WIDTH-2)];
                    hypotenuse_acc[next_h_offset + H_DIM(next_tr_spad_height+1,1)] =
                        hypotenuse_acc[curr_h_offset +
                                       H_DIM(next_tr_spad_height+1,H_SPAD_WIDTH-1)];
                }

                if (h_spad_width > 0) {
                    dmaLoad(&hypotenuse_acc[next_h_offset + H_DIM(next_tr_spad_height+1,
                                1 + (next_start_img_j > 0))],
                            &hypotenuse_host[IMG_DIM(next_start_img_i+next_tr_spad_height,
                                next_start_img_j+(next_start_img_j>0))],
                            h_spad_width * sizeof(HYPO_TYPE));
                }
            }
        }

        for (int img_i = start_img_i, h_i = 1, tr_i = 0; tr_i < tr_spad_height;
                img_i++, h_i++, tr_i++) {
            loop: for (int img_j = start_img_j, h_j = 1, tr_j = 0; tr_j < tr_spad_width;
                    img_j++, h_j++, tr_j++) {
                float q = 0, r = 0;
                int h_index = H_DIM(h_i, h_j);
                int tr_index = TR_DIM(tr_i, tr_j);

                float angle = theta_acc[curr_tr_offset + tr_index] * 180 / PI;
                if (angle < 0) { angle += 180; }

                // angle 0
                if (((angle >= 0)     && (angle < 22.5)) ||
                    ((angle >= 157.5) && (angle <= 180))) {
                    q = (img_j == max_width) ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i,h_j+1)];
                    r = (img_j == 0)         ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i,h_j-1)];
                }

                // angle 45
                else if ((angle >= 22.5) && (angle < 67.5)) {
                    q = ((img_i == max_height) || (img_j == 0)) ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i+1,h_j-1)];
                    r = ((img_i == 0) || (img_j == max_width)) ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i-1,h_j+1)];
                }

                // angle 90
                else if ((angle >= 67.5) && (angle < 112.5)) {
                    q = (img_i == max_height) ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i+1,h_j)];
                    r = (img_i == 0)          ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i-1,h_j)];
                }

                // angle 135
                else if ((angle >= 112.5) && (angle < 157.5)) {
                    q = ((img_i == 0) || (img_j == 0)) ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i-1,h_j-1)];
                    r = ((img_i == max_height) || (img_j == max_width)) ?
                        0 : hypotenuse_acc[curr_h_offset + H_DIM(h_i+1,h_j+1)];
                }

                if ((hypotenuse_acc[curr_h_offset + h_index] >= q) &&
                    (hypotenuse_acc[curr_h_offset + h_index] >= r)) {
                    result_acc[curr_tr_offset + tr_index] =
                        (uint8_t) hypotenuse_acc[curr_h_offset + h_index];
                }
                else {
                    result_acc[curr_tr_offset + tr_index] = 0;
                }
            }

            dmaStore(&result_host[IMG_DIM(start_img_i+tr_i, start_img_j)],
                     &result_acc[curr_tr_offset + TR_DIM(tr_i,0)],
                     tr_spad_width * sizeof(OUT_TYPE));
        }

        start_img_i = next_start_img_i;
        start_img_j = next_start_img_j;
        tr_spad_height = next_tr_spad_height;
        tr_spad_width = next_tr_spad_width;
        curr_h_offset = next_h_offset;
        curr_tr_offset = next_tr_offset;

        next_start_img_j += TR_SPAD_WIDTH;
        if (next_start_img_j >= IMG_WIDTH) {
            next_start_img_i += TR_SPAD_HEIGHT;
            next_start_img_j = 0;
        }
    }
}
