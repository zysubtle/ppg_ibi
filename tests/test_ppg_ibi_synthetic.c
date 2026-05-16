#include "ppg_ibi.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static ppg_ibi_input_t make_wave_input(uint32_t ts, bool allow, uint32_t sample_idx, uint32_t samples_per_beat) {
    static const int32_t offsets[PPG_IBI_NUM_CHANNELS] = {0, 300, -250, 150};
    ppg_ibi_input_t in;
    uint32_t phase = sample_idx % samples_per_beat;
    int32_t baseline = 100000;
    int32_t amplitude = 20000;
    int32_t half = (int32_t)(samples_per_beat / 2u);
    int32_t tri;
    in.timestamp_ms = ts;
    in.allow_measure = allow;

    if ((int32_t)phase <= half) tri = (int32_t)phase;
    else tri = (int32_t)samples_per_beat - (int32_t)phase;

    for (unsigned i = 0; i < PPG_IBI_NUM_CHANNELS; ++i) {
        in.ppg[i] = baseline + (amplitude * tri) / (half == 0 ? 1 : half) + offsets[i];
    }
    return in;
}

int main(void) {
    ppg_ibi_context_t ctx;
    ppg_ibi_output_t out;
    ppg_ibi_config_t cfg;
    uint32_t ts;
    uint32_t sample_idx;
    int c60 = 0, c75 = 0;

    ppg_ibi_get_default_config(&cfg);
    assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);

    ts = 0u;
    sample_idx = 0u;
    for (; ts < 15000u; ts += 20u, ++sample_idx) {
        ppg_ibi_input_t in = make_wave_input(ts, true, sample_idx, 50u);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        if (out.valid) {
            assert(out.ibi_ms >= 900u && out.ibi_ms <= 1100u);
            assert(out.confidence >= cfg.confidence_valid_min);
            c60++;
        }
    }
    assert(c60 >= 3);

    assert(ppg_ibi_reset(&ctx) == PPG_IBI_STATUS_OK);
    ts = 0u;
    sample_idx = 0u;
    for (; ts < 15000u; ts += 20u, ++sample_idx) {
        ppg_ibi_input_t in = make_wave_input(ts, true, sample_idx, 40u);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        if (out.valid) {
            assert(out.ibi_ms >= 720u && out.ibi_ms <= 880u);
            c75++;
        }
    }
    assert(c75 >= 3);

    assert(ppg_ibi_reset(&ctx) == PPG_IBI_STATUS_OK);
    for (ts = 0u; ts < 8000u; ts += 20u) {
        ppg_ibi_input_t in;
        in.timestamp_ms = ts;
        in.allow_measure = true;
        for (unsigned i = 0; i < PPG_IBI_NUM_CHANNELS; ++i) in.ppg[i] = 123456;
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        assert(!out.valid);
    }

    puts("All M4 synthetic tests passed.");
    return 0;
}
