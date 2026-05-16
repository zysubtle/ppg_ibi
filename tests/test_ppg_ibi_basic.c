#include "ppg_ibi.h"

#include <assert.h>
#include <stdio.h>

static ppg_ibi_input_t make_input(uint32_t ts, bool allow_measure) {
    ppg_ibi_input_t in;
    in.timestamp_ms = ts;
    in.allow_measure = allow_measure;
    for (unsigned i = 0; i < PPG_IBI_NUM_CHANNELS; ++i) {
        in.ppg[i] = 100000;
    }
    return in;
}

int main(void) {
    ppg_ibi_context_t ctx;
    ppg_ibi_output_t out;
    ppg_ibi_config_t cfg;
    ppg_ibi_input_t in;

    ppg_ibi_get_default_config(&cfg);
    assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);
    assert(ppg_ibi_reset(&ctx) == PPG_IBI_STATUS_OK);

    in = make_input(0u, true);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert(out.state == PPG_IBI_STATE_ACQUIRE);
    assert((out.flags & PPG_IBI_FLAG_WARMUP) != 0u);
    assert(!out.valid);

    for (uint32_t ts = 20u; ts <= 5000u; ts += 20u) {
        in = make_input(ts, true);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        assert(!out.valid);
    }
    assert(out.state == PPG_IBI_STATE_TRACK);

    in = make_input(5020u, false);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert(out.state == PPG_IBI_STATE_HOLD);
    assert((out.flags & PPG_IBI_FLAG_MOTION_HOLD) != 0u);
    assert(!out.valid);

    in = make_input(5040u, true);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert(out.state == PPG_IBI_STATE_REACQUIRE);
    assert((out.flags & PPG_IBI_FLAG_WARMUP) != 0u);

    in = make_input(5180u, true);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert((out.flags & PPG_IBI_FLAG_DROPOUT_SEVERE) != 0u);
    assert(out.state == PPG_IBI_STATE_REACQUIRE);
    assert(!out.valid);

    puts("All M4 basic regression tests passed.");
    return 0;
}
