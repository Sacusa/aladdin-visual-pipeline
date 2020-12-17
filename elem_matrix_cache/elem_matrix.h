#define TYPE float
#define MAT_WIDTH  128
#define MAT_HEIGHT 128
#define NUM_ELEMS (MAT_WIDTH * MAT_HEIGHT)

enum operation {
    ADD, SUB,
    MUL, DIV,
    SQR, SQRT,
    ATAN2
};

void elem_matrix(TYPE *arg1, TYPE *arg2, TYPE *result,
        uint8_t is_arg2_scalar, uint8_t op) {
    const float threehalfs = 1.5F;

    union sqrt_t {
        float f;
        uint32_t i;
    };

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
                float x2 = arg1[i] * 0.5F;
                union sqrt_t conv = { .f = arg1[i] };
                conv.i = 0x5f3759df - (conv.i >> 1);
                conv.f *= threehalfs - (x2 * conv.f * conv.f);
                conv.f *= threehalfs - (x2 * conv.f * conv.f);
                result[i] = 1/conv.f;
                break;
            }

            case ATAN2:
                result[i] = atan2(arg2[i], arg1[i]);
                break;
        }
    }
}
