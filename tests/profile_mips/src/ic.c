
#include "voice.h"
#include "profile.h"

void test_ic(int32_t (*input)[IC_FRAME_ADVANCE]) {
    static int framenum = 0;
    static ic_state_t DWORD_ALIGNED ic_state;
    static int32_t output[IC_FRAME_ADVANCE];
    static float_s32_t input_vnr_pred;

    if(!framenum) {
        ic_init(&ic_state);
    }

    prof(0, "start_ic_process_frame");
    ic_process_frame(&ic_state, input[0], input[1], output, &input_vnr_pred);
    prof(1, "end_ic_process_frame");
    print_prof(0, 2, framenum);
    framenum += 1;
}
