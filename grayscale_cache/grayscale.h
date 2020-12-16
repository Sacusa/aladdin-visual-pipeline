#define TYPE uint8_t
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

void grayscale(TYPE *input_image, TYPE *output_image) {
    loop: for (int i = 0; i < NUM_PIXELS; i++) {
        int ii = i * 3;  // scale the index into the input image
        output_image[i] = (input_image[ii] * 0.2126) +
                          (input_image[ii+1] * 0.7152) +
                          (input_image[ii+2] * 0.0722);
    }
}
