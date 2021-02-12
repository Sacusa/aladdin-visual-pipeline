#define TYPE float
#define IMG_WIDTH  128
#define IMG_HEIGHT 128
#define NUM_PIXELS (IMG_WIDTH * IMG_HEIGHT)

#define SPAD_WIDTH 12
#define SPAD_HEIGHT 7

#define IMG_DIM(x,y)  (((x)*IMG_WIDTH)  + (y))
#define SPAD_DIM(x,y) (((x)*SPAD_WIDTH) + (y))

// Definitions for atan2
#define HI(x) *(1+(int*)&x)
#define LO(x) *(int*)&x
#define PI_4  0.785398163397448309616

// Helper method declarations
float em_sqrt(float x);
double em_atan(double x);
double em_atan2(double y, double x);

enum operation {
    ADD, SUB,
    MUL, DIV,
    SQR, SQRT,
    ATAN2
};

void elem_matrix(TYPE *arg1_host, TYPE *arg2_host, TYPE *result_host,
        TYPE *arg1_acc, TYPE *arg2_acc, TYPE *result_acc,
        uint8_t is_arg2_scalar, uint8_t op) {
    int max_width = IMG_WIDTH - 1, max_height = IMG_HEIGHT - 1;
    int num_iters = ((IMG_HEIGHT / SPAD_HEIGHT) + ((IMG_HEIGHT % SPAD_HEIGHT) != 0)) *
                    ((IMG_WIDTH  / SPAD_WIDTH)  + ((IMG_WIDTH  % SPAD_WIDTH)  != 0));
    int last_iter = num_iters - 1;

    int spad = 0;
    int spad_offset = 0, next_spad_offset = 0;

    if (is_arg2_scalar) {
        dmaLoad(arg2_acc, arg2_host, sizeof(TYPE));
    }

    int start_img_i = 0, next_start_img_i = 0;
    int start_img_j = 0, next_start_img_j = 0;
    int spad_height = 0, next_spad_height = 0;
    int spad_width  = 0, next_spad_width  = 0;

    next_spad_height = ((next_start_img_i + SPAD_HEIGHT) > IMG_HEIGHT) ?
                       (IMG_HEIGHT - next_start_img_i) : SPAD_HEIGHT;
    next_spad_width  = ((next_start_img_j + SPAD_WIDTH) > IMG_WIDTH) ?
                       (IMG_WIDTH - next_start_img_j) : SPAD_WIDTH;

    for (int i = 0; i < next_spad_height; i++) {
        dmaLoad(&arg1_acc[SPAD_DIM(i,0)], &arg1_host[IMG_DIM(i, 0)],
                next_spad_width * sizeof(TYPE));

        if (!is_arg2_scalar) {
            dmaLoad(&arg2_acc[SPAD_DIM(i,0)], &arg2_host[IMG_DIM(i, 0)],
                    next_spad_width * sizeof(TYPE));
        }
    }

    next_start_img_j += SPAD_WIDTH;
    if (next_start_img_j >= IMG_WIDTH) {
        next_start_img_i += SPAD_HEIGHT;
        next_start_img_j = 0;
    }

    spad_height = next_spad_height;
    spad_width  = next_spad_width;

    for (int iter = 0; iter < num_iters; iter++) {
        if (iter != last_iter) {
            spad ^= 1;
            next_spad_offset = spad * SPAD_WIDTH * SPAD_HEIGHT;

            next_spad_height = ((next_start_img_i + SPAD_HEIGHT) > IMG_HEIGHT) ?
                               (IMG_HEIGHT - next_start_img_i) : SPAD_HEIGHT;
            next_spad_width  = ((next_start_img_j + SPAD_WIDTH) > IMG_WIDTH) ?
                               (IMG_WIDTH - next_start_img_j) : SPAD_WIDTH;

            for (int i = 0; i < next_spad_height; i++) {
                dmaLoad(&arg1_acc[next_spad_offset + SPAD_DIM(i,0)],
                        &arg1_host[IMG_DIM(next_start_img_i + i, next_start_img_j)],
                        next_spad_width * sizeof(TYPE));

                if (!is_arg2_scalar) {
                    dmaLoad(&arg2_acc[next_spad_offset + SPAD_DIM(i,0)],
                            &arg2_host[IMG_DIM(next_start_img_i + i, next_start_img_j)],
                            next_spad_width * sizeof(TYPE));
                }
            }
        }

        for (int i = 0; i < spad_height; i++) {
            loop: for (int j = 0; j < spad_width; j++) {
                int index = SPAD_DIM(i, j);

                switch (op) {
                    case ADD:
                        if (is_arg2_scalar) {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] + arg2_acc[0];
                        }
                        else {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] + arg2_acc[spad_offset + index];
                        }
                        break;

                    case SUB:
                        if (is_arg2_scalar) {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] - arg2_acc[0];
                        }
                        else {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] - arg2_acc[spad_offset + index];
                        }
                        break;

                    case MUL:
                        if (is_arg2_scalar) {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] * arg2_acc[0];
                        }
                        else {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] * arg2_acc[spad_offset + index];
                        }
                        break;

                    case DIV:
                        if (is_arg2_scalar) {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] / arg2_acc[0];
                        }
                        else {
                            result_acc[spad_offset + index] =
                                arg1_acc[spad_offset + index] / arg2_acc[spad_offset + index];
                        }
                        break;

                    case SQR:
                        result_acc[spad_offset + index] =
                            arg1_acc[spad_offset + index] * arg1_acc[spad_offset + index];
                        break;

                    case SQRT:
                        result_acc[spad_offset + index] =
                            em_sqrt(arg1_acc[spad_offset + index]);
                        break;

                    case ATAN2:
                        result_acc[spad_offset + index] = em_atan2(arg2_acc[spad_offset + index],
                                arg1_acc[spad_offset + index]);
                        break;
                }
            }

            dmaStore(&result_host[IMG_DIM(start_img_i + i, start_img_j)],
                     &result_acc[spad_offset + SPAD_DIM(i,0)],
                     spad_width * sizeof(TYPE));
        }

        start_img_i = next_start_img_i;
        start_img_j = next_start_img_j;
        spad_width  = next_spad_width;
        spad_height = next_spad_height;
        spad_offset = next_spad_offset;

        next_start_img_j += SPAD_WIDTH;
        if (next_start_img_j >= IMG_WIDTH) {
            next_start_img_i += SPAD_HEIGHT;
            next_start_img_j = 0;
        }
    }
}

/*
 * Helper methods definitions
 */

float em_sqrt(float x) {
    const float threehalfs = 1.5F;

    union sqrt_t {
        float f;
        uint32_t i;
    };

    float x2 = x * 0.5F;
    union sqrt_t conv = { .f = x };
    conv.i = 0x5f3759df - (conv.i >> 1);
    conv.f *= threehalfs - (x2 * conv.f * conv.f);
    conv.f *= threehalfs - (x2 * conv.f * conv.f);
    return 1/conv.f;
}

double em_atan(double x) {
    return PI_4*x - x*(((int)x) - 1)*(0.2447 + 0.0663*((int)x));
}

double em_atan2(double y, double x) {
    const double
        tiny  = 1.0e-300,
        zero  = 0.0,
        pi_o_4  = 7.8539816339744827900E-01, /* 0x3FE921FB, 0x54442D18 */
        pi_o_2  = 1.5707963267948965580E+00, /* 0x3FF921FB, 0x54442D18 */
        pi      = 3.1415926535897931160E+00, /* 0x400921FB, 0x54442D18 */
        pi_lo   = 1.2246467991473531772E-16; /* 0x3CA1A626, 0x33145C07 */
    double z;
    int k, m, hx, hy, ix, iy;
    unsigned lx, ly;

    hx = HI(x); ix = hx&0x7fffffff;
    lx = LO(x);
    hy = HI(y); iy = hy&0x7fffffff;
    ly = LO(y);

    if (((ix|((lx|-lx)>>31)) > 0x7ff00000) ||
        ((iy|((ly|-ly)>>31)) > 0x7ff00000)) {  /* x or y is NaN */
        return x+y;
    }

    if ((hx-0x3ff00000|lx) == 0) {  /* x=1.0 */
        return em_atan(y);
    }

    m = ((hy>>31)&1) | ((hx>>30)&2);  /* 2*sign(x)+sign(y) */

    /* when y = 0 */
    if ((iy|ly) == 0) {
        switch (m) {
            case 0:
            case 1: return y;         /* atan(+-0,+anything)=+-0 */
            case 2: return  pi+tiny;  /* atan(+0,-anything) = pi */
            case 3: return -pi-tiny;  /* atan(-0,-anything) =-pi */
        }
    }

    /* when x = 0 */
    if ((ix|lx) == 0) {
        return (hy < 0) ? -pi_o_2-tiny : pi_o_2+tiny;
    }

    /* when x is INF */
    if (ix == 0x7ff00000) {
        if (iy == 0x7ff00000) {
            switch (m) {
                case 0: return  pi_o_4+tiny;      /* atan(+INF,+INF) */
                case 1: return -pi_o_4-tiny;      /* atan(-INF,+INF) */
                case 2: return  3.0*pi_o_4+tiny;  /* atan(+INF,-INF) */
                case 3: return -3.0*pi_o_4-tiny;  /* atan(-INF,-INF) */
            }
        } else {
            switch (m) {
                case 0: return  zero;     /* atan(+...,+INF) */
                case 1: return -zero;     /* atan(-...,+INF) */
                case 2: return  pi+tiny;  /* atan(+...,-INF) */
                case 3: return -pi-tiny;  /* atan(-...,-INF) */
            }
        }
    }

    /* when y is INF */
    if (iy == 0x7ff00000) {
        return (hy < 0) ? -pi_o_2-tiny : pi_o_2+tiny;
    }

    /* compute y/x */
    k = (iy-ix)>>20;
    if (k > 60) {
        z = pi_o_2+0.5*pi_lo;  /* |y/x| >  2**60 */
    }
    else if (hx < 0 && k < -60) {
        z = 0.0;  /* |y|/x < -2**60 */
    }
    else {
        z = em_atan((double) ((int)(y/x)));  /* safe to do y/x */
    }
    switch (m) {
        case 0:
            return z;  /* atan(+,+) */
        case 1:
            HI(z) ^= 0x80000000;
            return z;  /* atan(-,+) */
        case 2:
            return pi-(z-pi_lo);  /* atan(+,-) */
        default: /* case 3 */
            return (z-pi_lo)-pi;  /* atan(-,-) */
    }
}
