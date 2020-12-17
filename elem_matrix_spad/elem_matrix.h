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

void elem_matrix(TYPE *arg1_host, TYPE *arg2_host, TYPE *result_host,
        TYPE *arg1_acc, TYPE *arg2_acc, TYPE *result_acc,
        int mat_size, uint8_t is_arg2_scalar, uint8_t op) {
    const float threehalfs = 1.5F;

    union sqrt_t {
        float f;
        uint32_t i;
    };

    dmaLoad(arg1_acc, arg1_host, mat_size);
    if (is_arg2_scalar) {
        dmaLoad(arg2_acc, arg2_host, sizeof(TYPE));
    }
    else {
        dmaLoad(arg2_acc, arg2_host, mat_size);
    }

    loop: for (int i = 0; i < NUM_ELEMS; i++) {
        switch (op) {
            case ADD:
                if (is_arg2_scalar) {
                    result_acc[i] = arg1_acc[i] + arg2_acc[0];
                }
                else {
                    result_acc[i] = arg1_acc[i] + arg2_acc[i];
                }
                break;

            case SUB:
                if (is_arg2_scalar) {
                    result_acc[i] = arg1_acc[i] - arg2_acc[0];
                }
                else {
                    result_acc[i] = arg1_acc[i] - arg2_acc[i];
                }
                break;

            case MUL:
                if (is_arg2_scalar) {
                    result_acc[i] = arg1_acc[i] * arg2_acc[0];
                }
                else {
                    result_acc[i] = arg1_acc[i] * arg2_acc[i];
                }
                break;

            case DIV:
                if (is_arg2_scalar) {
                    result_acc[i] = arg1_acc[i] / arg2_acc[0];
                }
                else {
                    result_acc[i] = arg1_acc[i] / arg2_acc[i];
                }
                break;

            case SQR:
                result_acc[i] = arg1_acc[i] * arg1_acc[i];
                break;

            case SQRT: {
                float x2 = arg1_acc[i] * 0.5F;
                union sqrt_t conv = { .f = arg1_acc[i] };
                conv.i = 0x5f3759df - (conv.i >> 1);
                conv.f *= threehalfs - (x2 * conv.f * conv.f);
                conv.f *= threehalfs - (x2 * conv.f * conv.f);
                result_acc[i] = 1/conv.f;
                break;
            }

            case ATAN2:
                result_acc[i] = atan2(arg2_acc[i], arg1_acc[i]);
                break;
        }
    }

    dmaStore(result_host, result_acc, mat_size);
}
