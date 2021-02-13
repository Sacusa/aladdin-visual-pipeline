void grayscale(uint8_t *input_image, uint8_t *output_image) {
    loop: for (int i = 0; i < NUM_PIXELS; i++) {
        int ii = i * 3;  // scale the index into the input image
        output_image[i] = (input_image[ii] * 0.2126) +
                          (input_image[ii+1] * 0.7152) +
                          (input_image[ii+2] * 0.0722);
    }
}
