#define HYPO_TYPE     uint8_t
#define THTA_TYPE     float
#define OUT_TYPE      uint8_t
#define IMG_WIDTH     128
#define IMG_HEIGHT    128
#define PI            3.141592653589793238462643
#define HYPO_DIM(x,y) (IMG_WIDTH + 3 + ((x)*2) + ((x)*IMG_WIDTH) + (y))
#define DIM(x,y)      (((x)*IMG_WIDTH) + (y))

void canny_non_max(HYPO_TYPE *hypotenuse_host, THTA_TYPE *theta_host, OUT_TYPE *result_host,
        HYPO_TYPE *hypotenuse_acc, THTA_TYPE *theta_acc, OUT_TYPE *result_acc,
        int hypotenuse_size, int theta_size, int result_size) {
    dmaLoad(hypotenuse_acc, hypotenuse_host, hypotenuse_size);
    dmaLoad(theta_acc, theta_host, theta_size);

    for (int i = 0; i < IMG_HEIGHT; i++) {
        loop: for (int j = 0; j < IMG_WIDTH; j++) {
            HYPO_TYPE q = 255, r = 255;
            int index = DIM(i,j), hypo_index = HYPO_DIM(i,j);

            THTA_TYPE angle = theta_acc[index] * 180 / PI;
            if (angle < 0) { angle += 180; }

            // angle 0
            if (((angle >= 0)     && (angle < 22.5)) ||
                ((angle >= 157.5) && (angle <= 180))) {
                q = hypotenuse_acc[HYPO_DIM(i,j+1)];
                r = hypotenuse_acc[HYPO_DIM(i,j-1)];
            }

            // angle 45
            else if ((angle >= 22.5) && (angle < 67.5)) {
                q = hypotenuse_acc[HYPO_DIM(i+1,j-1)];
                r = hypotenuse_acc[HYPO_DIM(i-1,j+1)];
            }

            // angle 90
            else if ((angle >= 67.5) && (angle < 112.5)) {
                q = hypotenuse_acc[HYPO_DIM(i+1,j)];
                r = hypotenuse_acc[HYPO_DIM(i-1,j)];
            }

            // angle 135
            else if ((angle >= 112.5) && (angle < 157.5)) {
                q = hypotenuse_acc[HYPO_DIM(i-1,j-1)];
                r = hypotenuse_acc[HYPO_DIM(i+1,j+1)];
            }

            if ((hypotenuse_acc[hypo_index] >= q) && (hypotenuse_acc[hypo_index] >= r)) {
                result_acc[index] = hypotenuse_acc[hypo_index];
            }
            else {
                result_acc[index] = 0;
            }
        }
    }

    dmaStore(result_host, result_acc, result_size);
}
