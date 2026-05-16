#include "ppg_ibi.h"

#include <math.h>

#define PPG_IBI_ALPHA_DC (0.01f)
#define PPG_IBI_ALPHA_SMOOTH (0.25f)
#define PPG_IBI_ALPHA_ABS (0.02f)
#define PPG_IBI_SQI_MIN_ABS_EMA (1.0f)

static float ppg_ibi_clampf(float x, float lo, float hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static bool ppg_ibi_is_valid_config(const ppg_ibi_config_t *config) {
    if (config->sample_rate_hz == 0u) return false;
    if (config->expected_dt_ms == 0u) return false;
    if (config->warmup_ms == 0u) return false;
    if (config->dropout_light_ms >= config->dropout_severe_ms) return false;
    if (config->ibi_min_ms >= config->ibi_max_ms) return false;
    if ((config->confidence_valid_min < 0.0f) || (config->confidence_valid_min > 1.0f)) return false;
    return true;
}

static void ppg_ibi_clear_beat_tracking(ppg_ibi_context_t *ctx) {
    ctx->has_last_beat = false;
    ctx->last_beat_timestamp_ms = 0u;
    ctx->beat_count = 0u;
    ctx->valid_ibi_count = 0u;
    ctx->peak_polarity = 0;
    ctx->has_composite_prev1 = false;
    ctx->has_composite_prev2 = false;
}

void ppg_ibi_get_default_config(ppg_ibi_config_t *config) {
    if (config == (ppg_ibi_config_t *)0) {
        return;
    }

    config->sample_rate_hz = 50u;
    config->expected_dt_ms = 20u;
    config->warmup_ms = 5000u;
    config->dropout_light_ms = 40u;
    config->dropout_severe_ms = 100u;
    config->ibi_min_ms = 300u;
    config->ibi_max_ms = 2000u;
    config->confidence_valid_min = 0.6f;
}

ppg_ibi_status_t ppg_ibi_init(ppg_ibi_context_t *ctx, const ppg_ibi_config_t *config) {
    ppg_ibi_config_t cfg;
    unsigned i;

    if (ctx == (ppg_ibi_context_t *)0) {
        return PPG_IBI_STATUS_NULL_ARGUMENT;
    }

    if (config == (const ppg_ibi_config_t *)0) {
        ppg_ibi_get_default_config(&cfg);
        config = &cfg;
    }

    if (!ppg_ibi_is_valid_config(config)) {
        return PPG_IBI_STATUS_INVALID_CONFIG;
    }

    ctx->config = *config;
    ctx->state = PPG_IBI_STATE_INIT;
    ctx->initialized = true;
    ctx->has_last_timestamp = false;
    ctx->last_allow_measure = false;
    ctx->last_timestamp_ms = 0u;
    ctx->state_entry_timestamp_ms = 0u;
    ctx->samples_since_state_entry = 0u;
    ctx->process_count = 0u;
    ctx->dropout_light_count = 0u;
    ctx->dropout_severe_count = 0u;

    for (i = 0u; i < PPG_IBI_NUM_CHANNELS; ++i) {
        ctx->dc[i] = 0.0f;
        ctx->smooth[i] = 0.0f;
    }
    ctx->has_filter_state = false;
    ctx->composite_prev2 = 0.0f;
    ctx->composite_prev1 = 0.0f;
    ctx->timestamp_prev2_ms = 0u;
    ctx->timestamp_prev1_ms = 0u;
    ctx->has_composite_prev1 = false;
    ctx->has_composite_prev2 = false;
    ctx->abs_ema = 0.0f;
    ppg_ibi_clear_beat_tracking(ctx);

    return PPG_IBI_STATUS_OK;
}

ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx) {
    if (ctx == (ppg_ibi_context_t *)0) {
        return PPG_IBI_STATUS_NULL_ARGUMENT;
    }

    if (!ppg_ibi_is_valid_config(&ctx->config)) {
        return PPG_IBI_STATUS_INVALID_CONFIG;
    }

    return ppg_ibi_init(ctx, &ctx->config);
}

static void ppg_ibi_enter_state(ppg_ibi_context_t *ctx, ppg_ibi_state_t state, uint32_t timestamp_ms) {
    ctx->state = state;
    ctx->state_entry_timestamp_ms = timestamp_ms;
    ctx->samples_since_state_entry = 0u;
}

ppg_ibi_status_t ppg_ibi_process(ppg_ibi_context_t *ctx,
                                 const ppg_ibi_input_t *input,
                                 ppg_ibi_output_t *output) {
    uint32_t flags = PPG_IBI_FLAG_NO_BEAT;
    bool low_sqi = false;
    bool beat_candidate = false;
    float candidate_amp = 0.0f;
    float peak_threshold = 0.0f;

    if ((ctx == (ppg_ibi_context_t *)0) || (input == (const ppg_ibi_input_t *)0) || (output == (ppg_ibi_output_t *)0)) {
        return PPG_IBI_STATUS_NULL_ARGUMENT;
    }
    if (!ctx->initialized) {
        return PPG_IBI_STATUS_INVALID_CONFIG;
    }

    output->valid = false;
    output->ibi_ms = 0u;
    output->confidence = 0.0f;
    ctx->process_count++;

    if (!input->allow_measure) {
        ppg_ibi_enter_state(ctx, PPG_IBI_STATE_HOLD, input->timestamp_ms);
        ctx->has_last_timestamp = false;
        ppg_ibi_clear_beat_tracking(ctx);
        flags |= PPG_IBI_FLAG_MOTION_HOLD;
    } else {
        if (ctx->state == PPG_IBI_STATE_HOLD) {
            ppg_ibi_enter_state(ctx, PPG_IBI_STATE_REACQUIRE, input->timestamp_ms);
            flags |= PPG_IBI_FLAG_WARMUP;
            ctx->has_last_timestamp = false;
            ppg_ibi_clear_beat_tracking(ctx);
        } else if (ctx->state == PPG_IBI_STATE_INIT) {
            ppg_ibi_enter_state(ctx, PPG_IBI_STATE_ACQUIRE, input->timestamp_ms);
            flags |= PPG_IBI_FLAG_WARMUP;
        }

        if (ctx->has_last_timestamp) {
            uint32_t dt_ms = input->timestamp_ms - ctx->last_timestamp_ms;
            if (dt_ms > ctx->config.dropout_severe_ms) {
                ctx->dropout_severe_count++;
                flags |= PPG_IBI_FLAG_DROPOUT_SEVERE;
                ppg_ibi_enter_state(ctx, PPG_IBI_STATE_REACQUIRE, input->timestamp_ms);
                flags |= PPG_IBI_FLAG_WARMUP;
                ppg_ibi_clear_beat_tracking(ctx);
            } else if (dt_ms > ctx->config.dropout_light_ms) {
                ctx->dropout_light_count++;
                flags |= PPG_IBI_FLAG_DROPOUT_LIGHT;
            }
        }

        if ((ctx->state == PPG_IBI_STATE_ACQUIRE) || (ctx->state == PPG_IBI_STATE_REACQUIRE)) {
            uint32_t elapsed_ms = input->timestamp_ms - ctx->state_entry_timestamp_ms;
            if (elapsed_ms >= ctx->config.warmup_ms) {
                ppg_ibi_enter_state(ctx, PPG_IBI_STATE_TRACK, input->timestamp_ms);
                ppg_ibi_clear_beat_tracking(ctx);
            } else {
                flags |= PPG_IBI_FLAG_WARMUP;
            }
        }

        if (ctx->state == PPG_IBI_STATE_TRACK) {
            float composite = 0.0f;
            unsigned ch;

            if (!ctx->has_filter_state) {
                for (ch = 0u; ch < PPG_IBI_NUM_CHANNELS; ++ch) {
                    ctx->dc[ch] = (float)input->ppg[ch];
                    ctx->smooth[ch] = 0.0f;
                }
                ctx->has_filter_state = true;
            }

            for (ch = 0u; ch < PPG_IBI_NUM_CHANNELS; ++ch) {
                float raw = (float)input->ppg[ch];
                float ac;
                ctx->dc[ch] += PPG_IBI_ALPHA_DC * (raw - ctx->dc[ch]);
                ac = raw - ctx->dc[ch];
                ctx->smooth[ch] += PPG_IBI_ALPHA_SMOOTH * (ac - ctx->smooth[ch]);
                composite += ctx->smooth[ch];
            }
            composite /= (float)PPG_IBI_NUM_CHANNELS;

            ctx->abs_ema += PPG_IBI_ALPHA_ABS * (fabsf(composite) - ctx->abs_ema);
            low_sqi = (ctx->abs_ema < PPG_IBI_SQI_MIN_ABS_EMA);
            if (low_sqi) {
                flags |= PPG_IBI_FLAG_LOW_SQI;
            }

            if (ctx->has_composite_prev2 && ctx->has_composite_prev1) {
                bool pos_peak = (ctx->composite_prev1 > ctx->composite_prev2) && (ctx->composite_prev1 >= composite);
                bool neg_peak = (ctx->composite_prev1 < ctx->composite_prev2) && (ctx->composite_prev1 <= composite);

                candidate_amp = fabsf(ctx->composite_prev1);
                peak_threshold = 0.8f * ctx->abs_ema;

                if ((pos_peak || neg_peak) && !low_sqi && (candidate_amp >= peak_threshold)) {
                    int polarity = pos_peak ? 1 : -1;
                    if ((ctx->peak_polarity == 0) || (ctx->peak_polarity == polarity)) {
                        if (ctx->peak_polarity == 0) {
                            ctx->peak_polarity = (int8_t)polarity;
                        }
                        beat_candidate = true;
                    }
                }
            }

            if (beat_candidate) {
                uint32_t beat_ts = ctx->timestamp_prev1_ms;
                ctx->beat_count++;

                if (!ctx->has_last_beat) {
                    ctx->last_beat_timestamp_ms = beat_ts;
                    ctx->has_last_beat = true;
                } else {
                    uint32_t ibi_ms = beat_ts - ctx->last_beat_timestamp_ms;
                    if ((ibi_ms < ctx->config.ibi_min_ms) || (ibi_ms > ctx->config.ibi_max_ms)) {
                        flags |= PPG_IBI_FLAG_IBI_OUT_OF_RANGE;
                        ctx->last_beat_timestamp_ms = beat_ts;
                    } else {
                        float amp_ratio = candidate_amp / (peak_threshold + 1e-6f);
                        float confidence = 0.6f + 0.2f * ppg_ibi_clampf(amp_ratio - 1.0f, 0.0f, 1.0f);
                        confidence = ppg_ibi_clampf(confidence, ctx->config.confidence_valid_min, 1.0f);
                        output->valid = true;
                        output->ibi_ms = (uint16_t)ibi_ms;
                        output->confidence = confidence;
                        ctx->last_beat_timestamp_ms = beat_ts;
                        ctx->valid_ibi_count++;
                        flags &= ~PPG_IBI_FLAG_NO_BEAT;
                    }
                }
            }

            if (!ctx->has_composite_prev1) {
                ctx->composite_prev1 = composite;
                ctx->timestamp_prev1_ms = input->timestamp_ms;
                ctx->has_composite_prev1 = true;
            } else if (!ctx->has_composite_prev2) {
                ctx->composite_prev2 = ctx->composite_prev1;
                ctx->timestamp_prev2_ms = ctx->timestamp_prev1_ms;
                ctx->composite_prev1 = composite;
                ctx->timestamp_prev1_ms = input->timestamp_ms;
                ctx->has_composite_prev2 = true;
            } else {
                ctx->composite_prev2 = ctx->composite_prev1;
                ctx->timestamp_prev2_ms = ctx->timestamp_prev1_ms;
                ctx->composite_prev1 = composite;
                ctx->timestamp_prev1_ms = input->timestamp_ms;
            }
        }
    }

    ctx->samples_since_state_entry++;
    ctx->last_allow_measure = input->allow_measure;
    ctx->last_timestamp_ms = input->timestamp_ms;
    ctx->has_last_timestamp = input->allow_measure;

    output->state = ctx->state;
    output->flags = flags;

    return PPG_IBI_STATUS_OK;
}
