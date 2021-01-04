#define TYPE float
#define MAT_WIDTH  128
#define MAT_HEIGHT 128
#define NUM_ELEMS (MAT_WIDTH * MAT_HEIGHT)

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

void elem_matrix(TYPE *arg1, TYPE *arg2, TYPE *result,
        uint8_t is_arg2_scalar, uint8_t op) {
    loop: for (int i = 0; i < NUM_ELEMS; i++) {
        switch (op) {
            case ADD:
                if (is_arg2_scalar) {
                    result[i] = arg1[i] + arg2[0];
                }
                else {
                    result[i] = arg1[i] + arg2[i];
                }
                break;

            case SUB:
                if (is_arg2_scalar) {
                    result[i] = arg1[i] - arg2[0];
                }
                else {
                    result[i] = arg1[i] - arg2[i];
                }
                break;

            case MUL:
                if (is_arg2_scalar) {
                    result[i] = arg1[i] * arg2[0];
                }
                else {
                    result[i] = arg1[i] * arg2[i];
                }
                break;

            case DIV:
                if (is_arg2_scalar) {
                    result[i] = arg1[i] / arg2[0];
                }
                else {
                    result[i] = arg1[i] / arg2[i];
                }
                break;

            case SQR:
                result[i] = arg1[i] * arg1[i];
                break;

            case SQRT: {
                result[i] = em_sqrt(arg1[i]);
                break;
            }

            case ATAN2:
                result[i] = em_atan2(arg2[i], arg1[i]);
                break;
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
