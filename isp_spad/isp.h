#define TYPE uint8_t
#define IMG_WIDTH 128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define GAMMA 0.416667

// OUT_SPAD_WIDTH and HEIGHT must be even
#define OUT_SPAD_WIDTH 20
#define OUT_SPAD_HEIGHT 12
#define IN_SPAD_WIDTH   (OUT_SPAD_WIDTH  + 2)
#define IN_SPAD_HEIGHT  (OUT_SPAD_HEIGHT + 2)

#define IMG_DIM(x,y) (IMG_WIDTH + 3 + ((x)*(2+IMG_WIDTH)) + (y))
#define IN_SPAD_DIM(x,y)  (((x)*IN_SPAD_WIDTH)  + (y))
#define OUT_SPAD_DIM(x,y) (3*(((x)*OUT_SPAD_WIDTH) + (y)))

void isp(TYPE *input_image_host, TYPE output_image_host[IMG_HEIGHT][IMG_WIDTH][3],
        TYPE *input_image_acc, TYPE output_image_acc[OUT_SPAD_HEIGHT][OUT_SPAD_WIDTH][3]) {
    enum channels {R=0, G, B};
    float ccm[3] = {255/142, 196/255, 1};

    int spad = 0;
    int in_offset = 0, next_in_offset = 0;
    int out_offset = 0, next_out_offset = 0;

    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;
    int num_iters = ((IMG_HEIGHT / OUT_SPAD_HEIGHT) + ((IMG_HEIGHT % OUT_SPAD_HEIGHT) != 0)) *
                    ((IMG_WIDTH  / OUT_SPAD_WIDTH)  + ((IMG_WIDTH  % OUT_SPAD_WIDTH)  != 0));
    int last_iter = num_iters - 1;

    int start_img_i = 0, next_start_img_i = 0;
    int start_img_j = 0, next_start_img_j = 0;
    int out_spad_height, next_out_spad_height = 0;
    int out_spad_width,  next_out_spad_width  = 0;
    int in_spad_width;

    next_out_spad_height = ((next_start_img_i + OUT_SPAD_HEIGHT) > IMG_HEIGHT) ?
                           (IMG_HEIGHT - next_start_img_i) : OUT_SPAD_HEIGHT;
    next_out_spad_width  = ((next_start_img_j + OUT_SPAD_WIDTH) > IMG_WIDTH) ?
                           (IMG_WIDTH - next_start_img_j) : OUT_SPAD_WIDTH;
    in_spad_width = next_out_spad_width + 2;

    // load the upper buffer row
    dmaLoad(&input_image_acc[IN_SPAD_DIM(0,0)], &input_image_host[IMG_DIM(0,0)],
            in_spad_width * sizeof(TYPE));

    for (int i = 0; i < next_out_spad_height; i++) {
        dmaLoad(&input_image_acc[IN_SPAD_DIM(i+1,0)], &input_image_host[IMG_DIM(i+1,0)],
                in_spad_width * sizeof(TYPE));
    }

    // load the lower buffer row
    dmaLoad(&input_image_acc[IN_SPAD_DIM(next_out_spad_height+1,0)],
            &input_image_host[IMG_DIM(next_out_spad_height+1,0)],
            in_spad_width * sizeof(TYPE));

    next_start_img_j += OUT_SPAD_WIDTH;
    if (next_start_img_j >= IMG_WIDTH) {
        next_start_img_i += OUT_SPAD_HEIGHT;
        next_start_img_j = 0;
    }

    out_spad_height = next_out_spad_height;
    out_spad_width  = next_out_spad_width;

    for (int iter = 0; iter < num_iters; iter++) {
        if (iter != last_iter) {
            spad ^= 1;
            next_in_offset = spad * IN_SPAD_WIDTH * IN_SPAD_HEIGHT;
            next_out_offset = spad * OUT_SPAD_HEIGHT;

            next_out_spad_height = ((next_start_img_i + OUT_SPAD_HEIGHT) > IMG_HEIGHT) ?
                                   (IMG_HEIGHT - next_start_img_i) : OUT_SPAD_HEIGHT;
            next_out_spad_width  = ((next_start_img_j + OUT_SPAD_WIDTH) > IMG_WIDTH) ?
                                   (IMG_WIDTH - next_start_img_j) : OUT_SPAD_WIDTH;

            if (next_start_img_j == 0) {
                in_spad_width = next_out_spad_width + 2;
            }
            else {
                in_spad_width = next_out_spad_width;
                if ((next_start_img_j + 1 + next_out_spad_width) > (IMG_WIDTH + 1)) {
                    in_spad_width = IMG_WIDTH - next_start_img_j + 1;
                }
            }

            // load the upper buffer row
            if (next_start_img_j > 0) {
                input_image_acc[next_in_offset + IN_SPAD_DIM(0, 0)] =
                    input_image_acc[in_offset + IN_SPAD_DIM(0, IN_SPAD_WIDTH-2)];
                input_image_acc[next_in_offset + IN_SPAD_DIM(0, 1)] =
                    input_image_acc[in_offset + IN_SPAD_DIM(0, IN_SPAD_WIDTH-1)];
            }
            if (in_spad_width > 0) {
                dmaLoad(&input_image_acc[next_in_offset + IN_SPAD_DIM(0,
                            (next_start_img_j>0) ? 2 : 0)],
                        &input_image_host[IMG_DIM(next_start_img_i-1,
                            next_start_img_j+((next_start_img_j>0) ? 2 : -1))],
                        in_spad_width * sizeof(TYPE));
            }

            for (int i = 0; i < next_out_spad_height; i++) {
                if (next_start_img_j > 0) {
                    input_image_acc[next_in_offset + IN_SPAD_DIM(i+1, 0)] =
                        input_image_acc[in_offset + IN_SPAD_DIM(i+1, IN_SPAD_WIDTH-2)];
                    input_image_acc[next_in_offset + IN_SPAD_DIM(i+1, 1)] =
                        input_image_acc[in_offset + IN_SPAD_DIM(i+1, IN_SPAD_WIDTH-1)];
                }

                if (in_spad_width > 0) {
                    dmaLoad(&input_image_acc[next_in_offset + IN_SPAD_DIM(i+1,
                                (next_start_img_j>1) ? 2 : 0)],
                            &input_image_host[IMG_DIM(next_start_img_i+i,
                                next_start_img_j + ((next_start_img_j>1) ? 2 : -1))],
                            in_spad_width * sizeof(TYPE));
                }
            }

            // load the lower buffer row
            if ((next_start_img_i + next_out_spad_height) < IMG_HEIGHT) {
                if (next_start_img_j > 0) {
                    input_image_acc[next_in_offset + IN_SPAD_DIM(next_out_spad_height+1,0)] =
                        input_image_acc[in_offset +
                                        IN_SPAD_DIM(next_out_spad_height+1, IN_SPAD_WIDTH-2)];
                    input_image_acc[next_in_offset + IN_SPAD_DIM(next_out_spad_height+1,1)] =
                        input_image_acc[in_offset +
                                        IN_SPAD_DIM(next_out_spad_height+1, IN_SPAD_WIDTH-1)];
                }

                if (in_spad_width > 1) {
                    dmaLoad(&input_image_acc[next_in_offset + IN_SPAD_DIM(next_out_spad_height+1,
                                (next_start_img_j>0) ? 2 : 0)],
                            &input_image_host[IMG_DIM(next_start_img_i+next_out_spad_height,
                                next_start_img_j+((next_start_img_j>0) ? 2 : -1))],
                            in_spad_width * sizeof(TYPE));
                }
            }
        }

        for (int i = 0; i < out_spad_height; i += 2) {
            loop: for (int j = 0; j < out_spad_width; j += 2) {
                int x, y;
                float pixel_tl[3], pixel_tr[3], pixel_bl[3], pixel_br[3];

                // top-left
                x = i + 1; y = j + 1;
                pixel_tl[R] = (input_image_acc[in_offset + IN_SPAD_DIM(x-1, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x-1, y+1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y+1)]) >> 2;
                pixel_tl[G] = (input_image_acc[in_offset + IN_SPAD_DIM(x-1, y)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x, y+1)]) >> 2;
                pixel_tl[B] = input_image_acc[in_offset + IN_SPAD_DIM(x, y)];

                pixel_tl[R] = powf((pixel_tl[R] * ccm[R]) / 255, GAMMA) * 255;
                pixel_tl[G] = powf((pixel_tl[G] * ccm[G]) / 255, GAMMA) * 255;
                pixel_tl[B] = powf((pixel_tl[B] * ccm[B]) / 255, GAMMA) * 255;

                x = i; y = j;
                output_image_acc[out_offset + x][y][R] = (pixel_tl[R] > 255) ? 255 : pixel_tl[R];
                output_image_acc[out_offset + x][y][G] = (pixel_tl[G] > 255) ? 255 : pixel_tl[G];
                output_image_acc[out_offset + x][y][B] = (pixel_tl[B] > 255) ? 255 : pixel_tl[B];

                // top-right
                x = i + 1; y = j + 2;
                pixel_tr[R] = (input_image_acc[in_offset + IN_SPAD_DIM(x-1, y)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y)]) >> 1;
                pixel_tr[G] = input_image_acc[in_offset + IN_SPAD_DIM(x, y)];
                pixel_tr[B] = (input_image_acc[in_offset + IN_SPAD_DIM(x, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x, y+1)]) >> 1;

                pixel_tr[R] = powf((pixel_tr[R] * ccm[R]) / 255, GAMMA) * 255;
                pixel_tr[G] = powf((pixel_tr[G] * ccm[G]) / 255, GAMMA) * 255;
                pixel_tr[B] = powf((pixel_tr[B] * ccm[B]) / 255, GAMMA) * 255;

                x = i; y = j + 1;
                output_image_acc[out_offset + x][y][R] = pixel_tr[R] > 255 ? 255 : pixel_tr[R];
                output_image_acc[out_offset + x][y][G] = pixel_tr[G] > 255 ? 255 : pixel_tr[G];
                output_image_acc[out_offset + x][y][B] = pixel_tr[B] > 255 ? 255 : pixel_tr[B];

                // bottom-left
                x = i + 2; y = j + 1;
                pixel_bl[R] = (input_image_acc[in_offset + IN_SPAD_DIM(x, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x, y+1)]) >> 1;
                pixel_bl[G] = input_image_acc[in_offset + IN_SPAD_DIM(x, y)];
                pixel_bl[B] = (input_image_acc[in_offset + IN_SPAD_DIM(x-1, y)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y)]) >> 1;

                pixel_bl[R] = powf((pixel_bl[R] * ccm[R]) / 255, GAMMA) * 255;
                pixel_bl[G] = powf((pixel_bl[G] * ccm[G]) / 255, GAMMA) * 255;
                pixel_bl[B] = powf((pixel_bl[B] * ccm[B]) / 255, GAMMA) * 255;

                x = i + 1; y = j;
                output_image_acc[out_offset + x][y][R] = pixel_bl[R] > 255 ? 255 : pixel_bl[R];
                output_image_acc[out_offset + x][y][G] = pixel_bl[G] > 255 ? 255 : pixel_bl[G];
                output_image_acc[out_offset + x][y][B] = pixel_bl[B] > 255 ? 255 : pixel_bl[B];

                // bottom-right
                x = i + 2; y = j + 2;
                pixel_br[R] = input_image_acc[in_offset + IN_SPAD_DIM(x, y)];
                pixel_br[G] = (input_image_acc[in_offset + IN_SPAD_DIM(x-1, y)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x, y+1)]) >> 2;
                pixel_br[B] = (input_image_acc[in_offset + IN_SPAD_DIM(x-1, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x-1, y+1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y-1)] +
                               input_image_acc[in_offset + IN_SPAD_DIM(x+1, y+1)]) >> 2;

                pixel_br[R] = powf((pixel_br[R] * ccm[R]) / 255, GAMMA) * 255;
                pixel_br[G] = powf((pixel_br[G] * ccm[G]) / 255, GAMMA) * 255;
                pixel_br[B] = powf((pixel_br[B] * ccm[B]) / 255, GAMMA) * 255;

                x = i + 1; y = j + 1;
                output_image_acc[out_offset + x][y][R] = pixel_br[R] > 255 ? 255 : pixel_br[R];
                output_image_acc[out_offset + x][y][G] = pixel_br[G] > 255 ? 255 : pixel_br[G];
                output_image_acc[out_offset + x][y][B] = pixel_br[B] > 255 ? 255 : pixel_br[B];
            }

            dmaStore(&output_image_host[start_img_i + i][start_img_j][0],
                     &output_image_acc[out_offset + i],
                     out_spad_width * sizeof(TYPE) * 3);
            dmaStore(&output_image_host[start_img_i + i + 1][start_img_j][0],
                     &output_image_acc[out_offset + i + 1],
                     out_spad_width * sizeof(TYPE) * 3);
        }

        start_img_i = next_start_img_i;
        start_img_j = next_start_img_j;
        out_spad_height = next_out_spad_height;
        out_spad_width = next_out_spad_width;
        in_offset = next_in_offset;
        out_offset = next_out_offset;

        next_start_img_j += OUT_SPAD_WIDTH;
        if (next_start_img_j >= IMG_WIDTH) {
            next_start_img_i += OUT_SPAD_HEIGHT;
            next_start_img_j = 0;
        }
    }
}
