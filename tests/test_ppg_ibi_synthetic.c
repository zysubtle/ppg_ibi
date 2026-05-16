#include "ppg_ibi.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static ppg_ibi_input_t synth_input(uint32_t ts, bool allow, float freq_hz, float amp, bool invert, bool flat) {
    ppg_ibi_input_t in;
    in.timestamp_ms = ts;
    in.allow_measure = allow;
    {
        float t = (float)ts * 0.001f;
        float s = flat ? 0.0f : sinf(2.0f * 3.14159265358979f * freq_hz * t);
        float base = 2000.0f + (invert ? -1.0f : 1.0f) * amp * s;
        for (unsigned i = 0; i < PPG_IBI_NUM_CHANNELS; ++i) in.ppg[i] = (int32_t)(base + (float)(i * 3));
    }
    return in;
}

static void run_rate_test(float freq_hz, uint16_t low, uint16_t high, bool invert) {
    ppg_ibi_context_t ctx; ppg_ibi_config_t cfg; ppg_ibi_output_t out; uint32_t valid_cnt = 0;
    ppg_ibi_get_default_config(&cfg); assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);
    for (uint32_t ts = 0; ts < 16000u; ts += 20u) {
        ppg_ibi_input_t in = synth_input(ts, true, freq_hz, 220.0f, invert, false);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        if (out.valid) { valid_cnt++; assert(out.ibi_ms >= low && out.ibi_ms <= high); assert(out.confidence >= cfg.confidence_valid_min); assert((out.flags & PPG_IBI_FLAG_NO_BEAT) == 0u); }
    }
    assert(valid_cnt >= 3u);
}

static void run_guard_tests(void) {
    ppg_ibi_context_t ctx; ppg_ibi_config_t cfg; ppg_ibi_output_t out;
    ppg_ibi_get_default_config(&cfg); assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);
    for (uint32_t ts = 0; ts < 8000u; ts += 20u) {
        ppg_ibi_input_t in = synth_input(ts, true, 1.0f, 0.0f, false, true);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        assert(!out.valid);
    }
    assert((out.flags & PPG_IBI_FLAG_LOW_SQI) != 0u);

    assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);
    for (uint32_t ts = 0; ts < 7000u; ts += 20u) {
        ppg_ibi_input_t in = synth_input(ts, true, 1.0f, 220.0f, false, false);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    }
    ppg_ibi_input_t hold = synth_input(7020u, false, 1.0f, 220.0f, false, false);
    assert(ppg_ibi_process(&ctx, &hold, &out) == PPG_IBI_STATUS_OK);
    assert(!out.valid);
    ppg_ibi_input_t resume = synth_input(7040u, true, 1.0f, 220.0f, false, false);
    assert(ppg_ibi_process(&ctx, &resume, &out) == PPG_IBI_STATUS_OK);
    assert(out.state == PPG_IBI_STATE_REACQUIRE);
    assert(!out.valid);

    assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);
    for (uint32_t ts = 0; ts < 7000u; ts += 20u) {
        ppg_ibi_input_t in = synth_input(ts, true, 1.0f, 220.0f, false, false);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    }
    ppg_ibi_input_t sev = synth_input(7160u, true, 1.0f, 220.0f, false, false);
    assert(ppg_ibi_process(&ctx, &sev, &out) == PPG_IBI_STATUS_OK);
    assert((out.flags & PPG_IBI_FLAG_DROPOUT_SEVERE) != 0u);
    assert(out.state == PPG_IBI_STATE_REACQUIRE);
    assert(!out.valid);
}

int main(void) {
    run_rate_test(1.0f, 900u, 1100u, false);
    run_rate_test(1.25f, 720u, 880u, true);
    run_guard_tests();
    puts("Synthetic M4 tests passed.");
    return 0;
}
