#define TYPE float
#define MAT_WIDTH  128
#define MAT_HEIGHT 128
#define NUM_ELEMS (MAT_WIDTH * MAT_HEIGHT)

enum operation {
    ADD, SUB,
    MUL, DIV,
    SQR, ATAN2
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

            case ATAN2:
                result[i] = atan2(arg2[i], arg1[i]);
                break;
        }
    }
}
