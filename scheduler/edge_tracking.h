#define WEAK       25
#define STRONG     255

#define ET_OUT_SPAD_WIDTH  50
#define ET_OUT_SPAD_HEIGHT 39
#define ET_IN_SPAD_WIDTH   (ET_OUT_SPAD_WIDTH  + 2)
#define ET_IN_SPAD_HEIGHT  (ET_OUT_SPAD_HEIGHT + 2)

#define ET_OUT_DIM(x,y) (((x)*ET_OUT_SPAD_WIDTH) + (y))
#define ET_IN_DIM(x,y)  (((x)*ET_IN_SPAD_WIDTH)  + (y))

void edge_tracking(uint8_t *input_image_host, uint8_t *input_image_acc,
        uint8_t thr_weak, uint8_t thr_strong,
        uint8_t *output_image_host, uint8_t *output_image_acc) {
    int spad = 0;
    int out_offset = 0, next_out_offset = 0;
    int in_offset  = 0, next_in_offset  = 0;

    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;
    int num_iters = ((IMG_HEIGHT/ET_OUT_SPAD_HEIGHT) + ((IMG_HEIGHT%ET_OUT_SPAD_HEIGHT) != 0)) *
                    ((IMG_WIDTH /ET_OUT_SPAD_WIDTH)  + ((IMG_WIDTH %ET_OUT_SPAD_WIDTH)  != 0));
    int last_iter = num_iters - 1;

    int start_img_i = 0, next_start_img_i = 0;
    int start_img_j = 0, next_start_img_j = 0;
    int out_spad_height = 0, next_out_spad_height = 0;
    int out_spad_width  = 0, next_out_spad_width  = 0;
    int in_spad_width   = 0, next_in_spad_width   = 0;

    next_out_spad_height = ((next_start_img_i + ET_OUT_SPAD_HEIGHT) > IMG_HEIGHT) ?
                           (IMG_HEIGHT - next_start_img_i) : ET_OUT_SPAD_HEIGHT;
    next_out_spad_width  = ((next_start_img_j + ET_OUT_SPAD_WIDTH) > IMG_WIDTH) ?
                           (IMG_WIDTH - next_start_img_j) : ET_OUT_SPAD_WIDTH;
    next_in_spad_width   = next_out_spad_width + 1;

    for (int i = 0; i < next_out_spad_height; i++) {
        dmaLoad(&input_image_acc[ET_IN_DIM(i+1,1)], &input_image_host[DIM(i,0)],
                next_in_spad_width * sizeof(uint8_t));
    }

    // load the lower buffer row
    if (next_out_spad_height < max_height) {
        dmaLoad(&input_image_acc[ET_IN_DIM(next_out_spad_height+1,1)],
                &input_image_host[DIM(next_out_spad_height,0)],
                next_in_spad_width * sizeof(uint8_t));
    }

    next_start_img_j += ET_OUT_SPAD_WIDTH;
    if (next_start_img_j >= IMG_WIDTH) {
        next_start_img_i += ET_OUT_SPAD_HEIGHT;
        next_start_img_j = 0;
    }

    out_spad_height = next_out_spad_height;
    out_spad_width  = next_out_spad_width;
    in_spad_width   = next_in_spad_width;

    for (int iter = 0; iter < num_iters; iter++) {
        if (iter != last_iter) {
            spad ^= 1;
            next_out_offset = spad * ET_OUT_SPAD_WIDTH * ET_OUT_SPAD_HEIGHT;
            next_in_offset  = spad * ET_IN_SPAD_WIDTH  * ET_IN_SPAD_HEIGHT;

            next_out_spad_height = ((next_start_img_i + ET_OUT_SPAD_HEIGHT) > IMG_HEIGHT) ?
                                   (IMG_HEIGHT - next_start_img_i) : ET_OUT_SPAD_HEIGHT;
            next_out_spad_width  = ((next_start_img_j + ET_OUT_SPAD_WIDTH) > IMG_WIDTH) ?
                                   (IMG_WIDTH - next_start_img_j) : ET_OUT_SPAD_WIDTH;

            if (next_start_img_j == 0) {
                next_in_spad_width = next_out_spad_width + 1;
            }
            else {
                if ((next_start_img_j + 1 + next_out_spad_width) > IMG_WIDTH) {
                    next_in_spad_width = IMG_WIDTH - next_start_img_j - 1;
                }
                else {
                    next_in_spad_width = next_out_spad_width;
                }
            }

            // load the upper buffer row
            if (next_start_img_i > 0) {
                if (next_start_img_j > 0) {
                    input_image_acc[next_in_offset + ET_IN_DIM(0,0)] =
                        input_image_acc[in_offset + ET_IN_DIM(0,ET_IN_SPAD_WIDTH-2)];
                    input_image_acc[next_in_offset + ET_IN_DIM(0,1)] =
                        input_image_acc[in_offset + ET_IN_DIM(0,ET_IN_SPAD_WIDTH-1)];
                }

                if (next_in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset +
                                ET_IN_DIM(0, 1 + (next_start_img_j > 0))],
                            &input_image_host[DIM(next_start_img_i-1,
                                next_start_img_j + (next_start_img_j > 0))],
                            next_in_spad_width * sizeof(uint8_t));
                }
            }

            for (int i = 0; i < next_out_spad_height; i++) {
                if (next_start_img_j > 0) {
                    input_image_acc[next_in_offset + ET_IN_DIM(i+1,0)] =
                        input_image_acc[in_offset + ET_IN_DIM(i+1,ET_IN_SPAD_WIDTH-2)];
                    input_image_acc[next_in_offset + ET_IN_DIM(i+1,1)] =
                        input_image_acc[in_offset + ET_IN_DIM(i+1,ET_IN_SPAD_WIDTH-1)];
                }

                if (next_in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset +
                                ET_IN_DIM(i+1, 1 + (next_start_img_j > 0))],
                            &input_image_host[DIM(next_start_img_i+i,
                                next_start_img_j + (next_start_img_j > 0))],
                            next_in_spad_width * sizeof(uint8_t));
                }
            }

            // load the lower buffer row
            if ((next_start_img_i + next_out_spad_height) < max_height) {
                if (next_start_img_j > 0) {
                    input_image_acc[next_in_offset + ET_IN_DIM(next_out_spad_height+1,0)] =
                        input_image_acc[in_offset + ET_IN_DIM(next_out_spad_height+1,
                                                           ET_IN_SPAD_WIDTH-2)];
                    input_image_acc[next_in_offset + ET_IN_DIM(next_out_spad_height+1,1)] =
                        input_image_acc[in_offset + ET_IN_DIM(next_out_spad_height+1,
                                                           ET_IN_SPAD_WIDTH-1)];
                }

                if (next_in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset + ET_IN_DIM(next_out_spad_height+1,
                                1 + (next_start_img_j > 0))],
                            &input_image_host[DIM(next_start_img_i+next_out_spad_height,
                                next_start_img_j + (next_start_img_j > 0))],
                            next_in_spad_width * sizeof(uint8_t));
                }
            }
        }

        for (int i = 1; i <= out_spad_height; i++) {
            tr_loop: for (int j = 1; j <= out_spad_width; j++) {
                int index = ET_IN_DIM(i,j);

                if (input_image_acc[in_offset + index] < thr_weak) {
                    input_image_acc[in_offset + index] = 0;
                }
                else if (input_image_acc[in_offset + i] >= thr_strong) {
                    input_image_acc[in_offset + index] = STRONG;
                }
                else {
                    input_image_acc[in_offset + index] = WEAK;
                }
            }
        }

        for (int img_i = start_img_i, in_i = 1, out_i = 0; out_i < out_spad_height;
                img_i++, in_i++, out_i++) {
            h_loop: for (int img_j = start_img_j, in_j = 1, out_j = 0; out_j < out_spad_width;
                            img_j++, in_j++, out_j++) {
                int i_min = img_i == 0, i_max = img_i == max_height;
                int j_min = img_j == 0, j_max = img_j == max_width;
                int in_index = ET_IN_DIM(in_i,in_j);
                int out_index = ET_OUT_DIM(out_i,out_j);

                if (input_image_acc[in_offset + in_index] == WEAK) {
                    if (((i_min || j_min) ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i-1,in_j-1)] == STRONG)) ||
                        ((i_min)          ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i-1,in_j  )] == STRONG)) ||
                        ((i_min || j_max) ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i-1,in_j+1)] == STRONG)) ||
                        ((j_min)          ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i  ,in_j-1)] == STRONG)) ||
                        ((j_max)          ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i  ,in_j+1)] == STRONG)) ||
                        ((i_max || j_min) ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i+1,in_j-1)] == STRONG)) ||
                        ((i_max)          ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i+1,in_j  )] == STRONG)) ||
                        ((i_max || j_max) ? 0 :
                            (input_image_acc[in_offset + ET_IN_DIM(in_i+1,in_j+1)] == STRONG))) {
                        output_image_acc[out_offset + out_index] = STRONG;
                    }
                    else {
                        output_image_acc[out_offset + out_index] = 0;
                    }
                }
                else {
                    output_image_acc[out_offset + out_index] =
                        input_image_acc[in_offset + in_index];
                }
            }

            dmaStore(&output_image_host[DIM(start_img_i+out_i, start_img_j)],
                     &output_image_acc[out_offset + ET_OUT_DIM(out_i, 0)],
                     out_spad_width * sizeof(uint8_t));
        }

        start_img_i = next_start_img_i;
        start_img_j = next_start_img_j;
        out_spad_width = next_out_spad_width;
        out_spad_height = next_out_spad_height;
        in_spad_width = next_in_spad_width;
        out_offset = next_out_offset;
        in_offset = next_in_offset;

        next_start_img_j += ET_OUT_SPAD_WIDTH;
        if (next_start_img_j >= IMG_WIDTH) {
            next_start_img_i += ET_OUT_SPAD_HEIGHT;
            next_start_img_j = 0;
        }
    }
}
