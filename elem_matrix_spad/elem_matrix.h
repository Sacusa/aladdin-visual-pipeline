#define TYPE float
#define MAT_WIDTH  128
#define MAT_HEIGHT 128
#define NUM_ELEMS (MAT_WIDTH * MAT_HEIGHT)

enum operation {
    ADD, SUB,
    MUL, DIV,
    SQR, ATAN2
};

void elem_matrix(TYPE *arg1_host, TYPE *arg2_host, TYPE *result_host,
        TYPE *arg1_acc, TYPE *arg2_acc, TYPE *result_acc,
        int mat_size, uint8_t is_arg2_scalar, uint8_t op) {
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

            case ATAN2:
                result_acc[i] = atan2(arg2_acc[i], arg1_acc[i]);
                break;
        }
    }

    dmaStore(result_host, result_acc, mat_size);
}
