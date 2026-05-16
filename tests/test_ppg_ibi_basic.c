#include "ppg_ibi.h"

#include <assert.h>
#include <stdio.h>

static ppg_ibi_input_t make_input(uint32_t ts, bool allow_measure) {
    ppg_ibi_input_t in;
    in.timestamp_ms = ts;
    in.allow_measure = allow_measure;
    for (unsigned i = 0; i < PPG_IBI_NUM_CHANNELS; ++i) {
        in.ppg[i] = (int32_t)(1000 + (int32_t)i);
    }
    return in;
}

int main(void) {
    ppg_ibi_context_t ctx;
    ppg_ibi_output_t out;
    ppg_ibi_config_t cfg;
    ppg_ibi_input_t in;

    ppg_ibi_get_default_config(&cfg);
    assert(cfg.sample_rate_hz == 50u);
    assert(cfg.expected_dt_ms == 20u);
    assert(cfg.warmup_ms == 5000u);
    assert(cfg.dropout_light_ms == 40u);
    assert(cfg.dropout_severe_ms == 100u);
    assert(cfg.ibi_min_ms == 300u);
    assert(cfg.ibi_max_ms == 2000u);
    assert(cfg.confidence_valid_min == 0.6f);

    assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_OK);
    assert(ctx.state == PPG_IBI_STATE_INIT);

    assert(ppg_ibi_reset(&ctx) == PPG_IBI_STATUS_OK);
    assert(ctx.state == PPG_IBI_STATE_INIT);

    in = make_input(0u, true);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert(out.state == PPG_IBI_STATE_ACQUIRE);
    assert((out.flags & PPG_IBI_FLAG_WARMUP) != 0u);
    assert(!out.valid);

    for (uint32_t ts = 20u; ts <= 5000u; ts += 20u) {
        in = make_input(ts, true);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    }
    assert(out.state == PPG_IBI_STATE_TRACK);
    assert(!out.valid);

    in = make_input(5020u, false);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert(out.state == PPG_IBI_STATE_HOLD);
    assert((out.flags & PPG_IBI_FLAG_MOTION_HOLD) != 0u);
    assert(!out.valid);

    in = make_input(5040u, true);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert(out.state == PPG_IBI_STATE_REACQUIRE);
    assert((out.flags & PPG_IBI_FLAG_WARMUP) != 0u);
    assert(!out.valid);

    for (uint32_t ts = 5060u; ts <= 10040u; ts += 20u) {
        in = make_input(ts, true);
        assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    }
    assert(out.state == PPG_IBI_STATE_TRACK);
    assert(!out.valid);

    in = make_input(10090u, true);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert((out.flags & PPG_IBI_FLAG_DROPOUT_LIGHT) != 0u);
    assert(out.state == PPG_IBI_STATE_TRACK);
    assert(!out.valid);

    in = make_input(10220u, true);
    assert(ppg_ibi_process(&ctx, &in, &out) == PPG_IBI_STATUS_OK);
    assert((out.flags & PPG_IBI_FLAG_DROPOUT_SEVERE) != 0u);
    assert(out.state == PPG_IBI_STATE_REACQUIRE);
    assert(!out.valid);

    assert((out.flags & PPG_IBI_FLAG_NO_BEAT) != 0u);

    assert(ppg_ibi_init((ppg_ibi_context_t *)0, &cfg) == PPG_IBI_STATUS_NULL_ARGUMENT);
    assert(ppg_ibi_reset((ppg_ibi_context_t *)0) == PPG_IBI_STATUS_NULL_ARGUMENT);
    in = make_input(0u, true);
    assert(ppg_ibi_process((ppg_ibi_context_t *)0, &in, &out) == PPG_IBI_STATUS_NULL_ARGUMENT);
    assert(ppg_ibi_process(&ctx, (const ppg_ibi_input_t *)0, &out) == PPG_IBI_STATUS_NULL_ARGUMENT);
    assert(ppg_ibi_process(&ctx, &in, (ppg_ibi_output_t *)0) == PPG_IBI_STATUS_NULL_ARGUMENT);

    cfg.sample_rate_hz = 0u;
    assert(ppg_ibi_init(&ctx, &cfg) == PPG_IBI_STATUS_INVALID_CONFIG);

    puts("All M3 basic tests passed.");
    return 0;
}
