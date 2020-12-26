#define PDIM(x,y) (IMG_WIDTH + 3 + ((x)*(2+IMG_WIDTH)) + (y))

/**
 * Padding
 * I = Image, P = Padding
 *
 * P P P P
 * P I I P
 * P I I P
 * P P P P
 */

void isp(uint8_t *input_image_host, uint8_t output_image_host[IMG_HEIGHT][IMG_WIDTH][3],
        uint8_t *input_image_acc, uint8_t output_image_acc[IMG_HEIGHT][IMG_WIDTH][3]) {
    enum channels { R=0, G, B };
    float ccm[3] = {255/142, 196/255, 1};
    float gamma = 0.416667;

    dmaLoad(input_image_acc, input_image_host, (IMG_HEIGHT+2) * (IMG_WIDTH+2));

    for (int i = 0; i < IMG_HEIGHT; i += 2) {
        loop: for (int j = 0; j < IMG_WIDTH; j += 2) {
            int x, y;
            float pixel_tl[3], pixel_tr[3], pixel_bl[3], pixel_br[3];

            // top-left
            x = i; y = j;
            pixel_tl[R] = (input_image_acc[PDIM(x-1, y-1)] + input_image_acc[PDIM(x-1, y+1)] +
                           input_image_acc[PDIM(x+1, y-1)] + input_image_acc[PDIM(x+1, y+1)]) >> 2;
            pixel_tl[G] = (input_image_acc[PDIM(x-1, y)] + input_image_acc[PDIM(x, y-1)] +
                           input_image_acc[PDIM(x+1, y)] + input_image_acc[PDIM(x, y+1)]) >> 2;
            pixel_tl[B] = input_image_acc[PDIM(x, y)];

            pixel_tl[R] = powf((pixel_tl[R] * ccm[R]) / 255, gamma) * 255;
            pixel_tl[G] = powf((pixel_tl[G] * ccm[G]) / 255, gamma) * 255;
            pixel_tl[B] = powf((pixel_tl[B] * ccm[B]) / 255, gamma) * 255;

            output_image_acc[x][y][R] = (pixel_tl[R] > 255) ? 255 : pixel_tl[R];
            output_image_acc[x][y][G] = (pixel_tl[G] > 255) ? 255 : pixel_tl[G];
            output_image_acc[x][y][B] = (pixel_tl[B] > 255) ? 255 : pixel_tl[B];

            // top-right
            x = i; y = j+1;
            pixel_tr[R] = (input_image_acc[PDIM(x-1, y)] + input_image_acc[PDIM(x+1, y)]) >> 1;
            pixel_tr[G] = input_image_acc[PDIM(x, y)];
            pixel_tr[B] = (input_image_acc[PDIM(x, y-1)] + input_image_acc[PDIM(x, y+1)]) >> 1;

            pixel_tr[R] = powf((pixel_tr[R] * ccm[R]) / 255, gamma) * 255;
            pixel_tr[G] = powf((pixel_tr[G] * ccm[G]) / 255, gamma) * 255;
            pixel_tr[B] = powf((pixel_tr[B] * ccm[B]) / 255, gamma) * 255;

            output_image_acc[x][y][R] = pixel_tr[R] > 255 ? 255 : pixel_tr[R];
            output_image_acc[x][y][G] = pixel_tr[G] > 255 ? 255 : pixel_tr[G];
            output_image_acc[x][y][B] = pixel_tr[B] > 255 ? 255 : pixel_tr[B];

            // bottom-left
            x = i+1; y = j;
            pixel_bl[R] = (input_image_acc[PDIM(x, y-1)] + input_image_acc[PDIM(x, y+1)]) >> 1;
            pixel_bl[G] = input_image_acc[PDIM(x, y)];
            pixel_bl[B] = (input_image_acc[PDIM(x-1, y)] + input_image_acc[PDIM(x+1, y)]) >> 1;

            pixel_bl[R] = powf((pixel_bl[R] * ccm[R]) / 255, gamma) * 255;
            pixel_bl[G] = powf((pixel_bl[G] * ccm[G]) / 255, gamma) * 255;
            pixel_bl[B] = powf((pixel_bl[B] * ccm[B]) / 255, gamma) * 255;

            output_image_acc[x][y][R] = pixel_bl[R] > 255 ? 255 : pixel_bl[R];
            output_image_acc[x][y][G] = pixel_bl[G] > 255 ? 255 : pixel_bl[G];
            output_image_acc[x][y][B] = pixel_bl[B] > 255 ? 255 : pixel_bl[B];

            // bottom-right
            x = i+1; y = j+1;
            pixel_br[R] = input_image_acc[PDIM(x, y)];
            pixel_br[G] = (input_image_acc[PDIM(x-1, y)] + input_image_acc[PDIM(x, y-1)] +
                           input_image_acc[PDIM(x+1, y)] + input_image_acc[PDIM(x, y+1)]) >> 2;
            pixel_br[B] = (input_image_acc[PDIM(x-1, y-1)] + input_image_acc[PDIM(x-1, y+1)] +
                        input_image_acc[PDIM(x+1, y-1)] + input_image_acc[PDIM(x+1, y+1)]) >> 2;

            pixel_br[R] = powf((pixel_br[R] * ccm[R]) / 255, gamma) * 255;
            pixel_br[G] = powf((pixel_br[G] * ccm[G]) / 255, gamma) * 255;
            pixel_br[B] = powf((pixel_br[B] * ccm[B]) / 255, gamma) * 255;

            output_image_acc[x][y][R] = pixel_br[R] > 255 ? 255 : pixel_br[R];
            output_image_acc[x][y][G] = pixel_br[G] > 255 ? 255 : pixel_br[G];
            output_image_acc[x][y][B] = pixel_br[B] > 255 ? 255 : pixel_br[B];
        }
    }

    dmaStore(output_image_host, output_image_acc, IMG_HEIGHT * IMG_WIDTH * 3);
}
