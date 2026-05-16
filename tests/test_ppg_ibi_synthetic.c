#include "ppg_ibi.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static int32_t synth_ppg(uint32_t sample_idx, uint32_t samples_per_beat) {
    const uint32_t phase = sample_idx % samples_per_beat;
    const float x = (float)phase / (float)samples_per_beat;
    const float tri = (x < 0.5f) ? (x * 2.0f) : ((1.0f - x) * 2.0f);
    return (int32_t)(100000.0f + 5000.0f * tri);
}

static ppg_ibi_input_t make_input(uint32_t ts, bool allow, int32_t base) {
    ppg_ibi_input_t in;
    unsigned i;
    in.timestamp_ms = ts;
    in.allow_measure = allow;
    for (i = 0; i < PPG_IBI_NUM_CHANNELS; ++i) {
        in.ppg[i] = base + (int32_t)i;
    }
    return in;
}

int main(void) {
    ppg_ibi_context_t ctx;
    ppg_ibi_output_t out;
    ppg_ibi_config_t cfg;
    uint32_t ts = 0u;
    uint32_t i;
    bool seen_track_flat = false;
    bool seen_low_sqi = false;
    bool seen_hold = false;
    bool seen_reacquire = false;
    bool severe_seen = false;

    ppg_ibi_get_default_config(&cfg);
    assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);

    for (i = 0u; i < 700u; ++i) {
        ppg_ibi_input_t in = make_input(ts, true, 1000);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        if (out.state == PPG_IBI_STATE_TRACK) {
            seen_track_flat = true;
            if ((out.flags & PPG_IBI_FLAG_LOW_SQI) != 0u) seen_low_sqi = true;
        }
        assert(!out.valid);
        ts += 20u;
    }
    assert(seen_track_flat);
    assert(seen_low_sqi);

    for (i = 0u; i < 80u; ++i) {
        ppg_ibi_input_t in = make_input(ts, false, 1000);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        assert(out.state == PPG_IBI_STATE_HOLD);
        assert((out.flags & PPG_IBI_FLAG_MOTION_HOLD) != 0u);
        assert(!out.valid);
        ts += 20u;
    }
    seen_hold = true;

    for (i = 0u; i < 249u; ++i) {
        const int32_t p = synth_ppg(i, 50u);
        ppg_ibi_input_t in = make_input(ts, true, p);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        if (i == 0u) {
            assert(out.state == PPG_IBI_STATE_REACQUIRE);
            seen_reacquire = true;
        }
        if (out.state != PPG_IBI_STATE_TRACK) {
            assert(!out.valid);
        }
        ts += 20u;
    }
    assert(out.state == PPG_IBI_STATE_REACQUIRE || out.state == PPG_IBI_STATE_TRACK);
    assert(seen_reacquire);

    {
        ppg_ibi_input_t in = make_input(ts + 200u, true, synth_ppg(0u, 50u));
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        assert((out.flags & PPG_IBI_FLAG_DROPOUT_SEVERE) != 0u);
        assert(out.state == PPG_IBI_STATE_REACQUIRE);
        assert(!out.valid);
        severe_seen = true;
        ts += 220u;
    }

    for (i = 1u; i < 250u; ++i) {
        const int32_t p = synth_ppg(i, 50u);
        ppg_ibi_input_t in = make_input(ts, true, p);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
        if (out.state != PPG_IBI_STATE_TRACK) {
            assert(!out.valid);
        }
        ts += 20u;
    }

    assert(seen_hold);
    assert(severe_seen);
    puts("Synthetic M4 tests passed.");
    return 0;
}
