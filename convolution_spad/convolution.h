#define TYPE float
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)
#define IN_DIM(x,y)   (((x)*in_width)   + (y))
#define KERN_DIM(x,y) (((x)*kern_width) + (y))
#define OUT_DIM(x,y)  (((x)*IMG_WIDTH)  + (y))

/**
 * Padding
 * I = img, P = padding
 * P P P P
 * P I I P
 * P I I P
 * P P P P
 *
 * Padded rows/cols depend on the kernel width and height.
 *
 * Max. kernel size = 5x5
 */

void convolution(TYPE *input_image_host, TYPE *kernel_host, TYPE *output_image_host,
        TYPE *input_image_acc, TYPE *kernel_acc, TYPE *output_image_acc,
        int kern_width, int kern_height) {
    dmaLoad(input_image_acc, input_image_host, 4 * (IMG_HEIGHT+2) * (IMG_WIDTH+2));
    dmaLoad(kernel_acc, kernel_host, 4 * kern_width * kern_height);

    const int in_height = IMG_HEIGHT + kern_height - 1;
    const int in_width = IMG_WIDTH + kern_width - 1;

    for (int in_i = 0, out_i = 0; in_i < IMG_HEIGHT; in_i++, out_i++) {
        loop: for (int in_j = 0, out_j = 0; in_j < IMG_WIDTH; in_j++, out_j++) {
            float partial_sum = 0;

            for (int ki = 0; ki < kern_height; ki++) {
                for (int kj = 0; kj < kern_width; kj++) {
                    partial_sum += input_image_acc[IN_DIM(in_i+ki, in_j+kj)] * \
                                   kernel_acc[KERN_DIM(ki, kj)];
                }
            }

            output_image_acc[OUT_DIM(out_i, out_j)] = partial_sum;
        }
    }

    dmaStore(output_image_host, output_image_acc, 4 * NUM_PIXELS);
}
