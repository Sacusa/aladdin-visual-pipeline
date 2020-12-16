#define TYPE uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

void grayscale(TYPE *input_image_host, TYPE *output_image_host,
               TYPE *input_image_acc,  TYPE *output_image_acc,
               int input_image_size, int output_image_size) {
    dmaLoad(input_image_acc, input_image_host, input_image_size);

    loop: for (int i = 0; i < NUM_PIXELS; i++) {
        int ii = i * 3;  // scale the index into the input image
        output_image_acc[i] = (input_image_acc[ii]   * 0.2126) +
                              (input_image_acc[ii+1] * 0.7152) +
                              (input_image_acc[ii+2] * 0.0722);
    }

    dmaStore(output_image_host, output_image_acc, output_image_size);
}
