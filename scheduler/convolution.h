#define KERN_DIM(x,y) (((x)*kern_width) + (y))

void convolution(float *input_image, float *kernel, float *output_image,
        int kern_width, int kern_height) {
    const int start_in_i = -((kern_height - 1) / 2);
    const int start_in_j = -((kern_width  - 1) / 2);

    for (int in_i = start_in_i, out_i = 0; out_i < IMG_HEIGHT; in_i++, out_i++) {
        loop: for (int in_j = start_in_j, out_j = 0; out_j < IMG_WIDTH; in_j++, out_j++) {
            float partial_sum = 0;

            for (int ki = 0; ki < kern_height; ki++) {
                bool valid_row = ((in_i+ki) >= 0) && ((in_i+ki) < IMG_HEIGHT);

                for (int kj = 0; kj < kern_width; kj++) {
                    bool valid_col = ((in_j+kj) >= 0) && ((in_j+kj) < IMG_WIDTH);

                    float in_val = (valid_row && valid_col) ?
                            input_image[DIM(in_i+ki, in_j+kj)] : 0;

                    partial_sum += in_val * kernel[KERN_DIM(ki, kj)];
                }
            }

            output_image[DIM(out_i, out_j)] = partial_sum;
        }
    }
}
