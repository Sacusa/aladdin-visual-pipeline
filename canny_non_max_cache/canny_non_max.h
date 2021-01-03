#define HYPO_TYPE     uint8_t
#define THTA_TYPE     float
#define OUT_TYPE      uint8_t
#define IMG_WIDTH     128
#define IMG_HEIGHT    128
#define PI            3.141592653589793238462643
#define DIM(x,y)      (((x)*IMG_WIDTH) + (y))

void canny_non_max(HYPO_TYPE *hypotenuse, THTA_TYPE *theta, OUT_TYPE *result) {
    int max_height = IMG_HEIGHT - 1, max_width = IMG_WIDTH - 1;

    for (int i = 0; i < IMG_HEIGHT; i++) {
        loop: for (int j = 0; j < IMG_WIDTH; j++) {
            float q = 0, r = 0;
            int index = DIM(i,j);

            float angle = theta[index] * 180 / PI;
            if (angle < 0) { angle += 180; }

            // angle 0
            if (((angle >= 0)     && (angle < 22.5)) ||
                ((angle >= 157.5) && (angle <= 180))) {
                q = (j == max_width) ? 0 : hypotenuse[DIM(i,j+1)];
                r = (j == 0)         ? 0 : hypotenuse[DIM(i,j-1)];
            }

            // angle 45
            else if ((angle >= 22.5) && (angle < 67.5)) {
                q = ((i == max_height) || (j == 0)) ? 0 : hypotenuse[DIM(i+1,j-1)];
                r = ((i == 0) || (j == max_width))  ? 0 : hypotenuse[DIM(i-1,j+1)];
            }

            // angle 90
            else if ((angle >= 67.5) && (angle < 112.5)) {
                q = (i == max_height) ? 0 : hypotenuse[DIM(i+1,j)];
                r = (i == 0)          ? 0 : hypotenuse[DIM(i-1,j)];
            }

            // angle 135
            else if ((angle >= 112.5) && (angle < 157.5)) {
                q = ((i == 0)          || (j == 0))         ? 0 : hypotenuse[DIM(i-1,j-1)];
                r = ((i == max_height) || (j == max_width)) ? 0 : hypotenuse[DIM(i+1,j+1)];
            }

            if ((hypotenuse[index] >= q) && (hypotenuse[index] >= r)) {
                result[index] = (uint8_t) hypotenuse[index];
            }
            else {
                result[index] = 0;
            }
        }
    }
}
