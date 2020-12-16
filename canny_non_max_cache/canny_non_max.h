#define HYPO_TYPE     uint8_t
#define THTA_TYPE     float
#define OUT_TYPE      uint8_t
#define IMG_WIDTH     128
#define IMG_HEIGHT    128
#define PI            3.141592653589793238462643
#define HYPO_DIM(x,y) (IMG_WIDTH + 3 + ((x)*2) + ((x)*IMG_WIDTH) + (y))
#define DIM(x,y)      (((x)*IMG_WIDTH) + (y))

void canny_non_max(HYPO_TYPE *hypotenuse, THTA_TYPE *theta, OUT_TYPE *result) {
    for (int i = 0; i < IMG_HEIGHT; i++) {
        loop: for (int j = 0; j < IMG_WIDTH; j++) {
            HYPO_TYPE q = 255, r = 255;
            int index = DIM(i,j), hypo_index = HYPO_DIM(i,j);

            THTA_TYPE angle = theta[index] * 180 / PI;
            if (angle < 0) { angle += 180; }

            // angle 0
            if (((angle >= 0)     && (angle < 22.5)) ||
                ((angle >= 157.5) && (angle <= 180))) {
                q = hypotenuse[HYPO_DIM(i,j+1)];
                r = hypotenuse[HYPO_DIM(i,j-1)];
            }

            // angle 45
            else if ((angle >= 22.5) && (angle < 67.5)) {
                q = hypotenuse[HYPO_DIM(i+1,j-1)];
                r = hypotenuse[HYPO_DIM(i-1,j+1)];
            }

            // angle 90
            else if ((angle >= 67.5) && (angle < 112.5)) {
                q = hypotenuse[HYPO_DIM(i+1,j)];
                r = hypotenuse[HYPO_DIM(i-1,j)];
            }

            // angle 135
            else if ((angle >= 112.5) && (angle < 157.5)) {
                q = hypotenuse[HYPO_DIM(i-1,j-1)];
                r = hypotenuse[HYPO_DIM(i+1,j+1)];
            }

            if ((hypotenuse[hypo_index] >= q) && (hypotenuse[hypo_index] >= r)) {
                result[index] = hypotenuse[hypo_index];
            }
            else {
                result[index] = 0;
            }
        }
    }
}
