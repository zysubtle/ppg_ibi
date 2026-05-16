#include "ppg_ibi.h"

#include <math.h>

#define PPG_IBI_DC_ALPHA (0.05f)
#define PPG_IBI_LP_ALPHA (0.35f)
#define PPG_IBI_SQI_EMA_ALPHA (0.05f)
#define PPG_IBI_SQI_MIN_ABS (1.5f)
#define PPG_IBI_PEAK_THRESHOLD_SCALE (0.8f)
#define PPG_IBI_REFRACTORY_MS (280u)

static bool ppg_ibi_is_valid_config(const ppg_ibi_config_t *config) {
    if (config->sample_rate_hz == 0u) return false;
    if (config->expected_dt_ms == 0u) return false;
    if (config->warmup_ms == 0u) return false;
    if (config->dropout_light_ms >= config->dropout_severe_ms) return false;
    if (config->ibi_min_ms >= config->ibi_max_ms) return false;
    if ((config->confidence_valid_min < 0.0f) || (config->confidence_valid_min > 1.0f)) return false;
    return true;
}

static void ppg_ibi_clear_tracking(ppg_ibi_context_t *ctx) {
    ctx->filter_initialized = false;
    ctx->have_prev1 = false;
    ctx->have_prev2 = false;
    ctx->signal_abs_ema = 0.0f;
    ctx->polarity_locked = false;
    ctx->locked_polarity = 0;
    ctx->has_last_beat = false;
    ctx->last_beat_timestamp_ms = 0u;
    ctx->refractory_until_ms = 0u;
}

void ppg_ibi_get_default_config(ppg_ibi_config_t *config) { if (config == 0) return;
    config->sample_rate_hz = 50u; config->expected_dt_ms = 20u; config->warmup_ms = 5000u;
    config->dropout_light_ms = 40u; config->dropout_severe_ms = 100u;
    config->ibi_min_ms = 300u; config->ibi_max_ms = 2000u; config->confidence_valid_min = 0.6f;
}

ppg_ibi_status_t ppg_ibi_init(ppg_ibi_context_t *ctx, const ppg_ibi_config_t *config) {
    ppg_ibi_config_t cfg;
    if (ctx == 0) return PPG_IBI_STATUS_NULL_ARGUMENT;
    if (config == 0) { ppg_ibi_get_default_config(&cfg); config = &cfg; }
    if (!ppg_ibi_is_valid_config(config)) return PPG_IBI_STATUS_INVALID_CONFIG;
    ctx->config = *config; ctx->state = PPG_IBI_STATE_INIT; ctx->initialized = true;
    ctx->has_last_timestamp = false; ctx->last_allow_measure = false; ctx->last_timestamp_ms = 0u;
    ctx->state_entry_timestamp_ms = 0u; ctx->samples_since_state_entry = 0u; ctx->process_count = 0u;
    ctx->dropout_light_count = 0u; ctx->dropout_severe_count = 0u; ppg_ibi_clear_tracking(ctx);
    return PPG_IBI_STATUS_OK;
}

ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx) {
    if (ctx == 0) return PPG_IBI_STATUS_NULL_ARGUMENT;
    if (!ppg_ibi_is_valid_config(&ctx->config)) return PPG_IBI_STATUS_INVALID_CONFIG;
    ctx->state = PPG_IBI_STATE_INIT; ctx->initialized = true; ctx->has_last_timestamp = false;
    ctx->last_allow_measure = false; ctx->last_timestamp_ms = 0u; ctx->state_entry_timestamp_ms = 0u;
    ctx->samples_since_state_entry = 0u; ctx->process_count = 0u; ctx->dropout_light_count = 0u;
    ctx->dropout_severe_count = 0u; ppg_ibi_clear_tracking(ctx); return PPG_IBI_STATUS_OK;
}

static void ppg_ibi_enter_state(ppg_ibi_context_t *ctx, ppg_ibi_state_t state, uint32_t timestamp_ms) {
    ctx->state = state; ctx->state_entry_timestamp_ms = timestamp_ms; ctx->samples_since_state_entry = 0u;
}

static float ppg_ibi_process_signal(ppg_ibi_context_t *ctx, const ppg_ibi_input_t *input) {
    float composite = 0.0f;
    for (unsigned i = 0; i < PPG_IBI_NUM_CHANNELS; ++i) {
        float x = (float)input->ppg[i];
        if (!ctx->filter_initialized) { ctx->dc_estimate[i] = x; ctx->lp_estimate[i] = 0.0f; }
        else {
            ctx->dc_estimate[i] += PPG_IBI_DC_ALPHA * (x - ctx->dc_estimate[i]);
            ctx->lp_estimate[i] += PPG_IBI_LP_ALPHA * ((x - ctx->dc_estimate[i]) - ctx->lp_estimate[i]);
        }
        composite += ctx->lp_estimate[i];
    }
    ctx->filter_initialized = true;
    composite *= 0.25f;
    {
        float abs_comp = fabsf(composite);
        if (ctx->signal_abs_ema <= 0.0f) ctx->signal_abs_ema = abs_comp;
        else ctx->signal_abs_ema += PPG_IBI_SQI_EMA_ALPHA * (abs_comp - ctx->signal_abs_ema);
    }
    return composite;
}

ppg_ibi_status_t ppg_ibi_process(ppg_ibi_context_t *ctx, const ppg_ibi_input_t *input, ppg_ibi_output_t *output) {
    uint32_t flags = PPG_IBI_FLAG_NO_BEAT;
    if ((ctx == 0) || (input == 0) || (output == 0)) return PPG_IBI_STATUS_NULL_ARGUMENT;
    if (!ctx->initialized) return PPG_IBI_STATUS_INVALID_CONFIG;
    output->valid = false; output->ibi_ms = 0u; output->confidence = 0.0f;
    ctx->process_count++;

    if (!input->allow_measure) {
        ppg_ibi_enter_state(ctx, PPG_IBI_STATE_HOLD, input->timestamp_ms);
        ctx->has_last_timestamp = false; ppg_ibi_clear_tracking(ctx); flags |= PPG_IBI_FLAG_MOTION_HOLD;
    } else {
        if (ctx->state == PPG_IBI_STATE_HOLD) {
            ppg_ibi_enter_state(ctx, PPG_IBI_STATE_REACQUIRE, input->timestamp_ms); flags |= PPG_IBI_FLAG_WARMUP;
            ctx->has_last_timestamp = false; ppg_ibi_clear_tracking(ctx);
        } else if (ctx->state == PPG_IBI_STATE_INIT) { ppg_ibi_enter_state(ctx, PPG_IBI_STATE_ACQUIRE, input->timestamp_ms); flags |= PPG_IBI_FLAG_WARMUP; }

        if (ctx->has_last_timestamp) {
            uint32_t dt_ms = input->timestamp_ms - ctx->last_timestamp_ms;
            if (dt_ms > ctx->config.dropout_severe_ms) {
                ctx->dropout_severe_count++; flags |= PPG_IBI_FLAG_DROPOUT_SEVERE;
                ppg_ibi_enter_state(ctx, PPG_IBI_STATE_REACQUIRE, input->timestamp_ms); flags |= PPG_IBI_FLAG_WARMUP; ppg_ibi_clear_tracking(ctx);
            } else if (dt_ms > ctx->config.dropout_light_ms) { ctx->dropout_light_count++; flags |= PPG_IBI_FLAG_DROPOUT_LIGHT; }
        }

        if ((ctx->state == PPG_IBI_STATE_ACQUIRE) || (ctx->state == PPG_IBI_STATE_REACQUIRE)) {
            uint32_t elapsed_ms = input->timestamp_ms - ctx->state_entry_timestamp_ms;
            if (elapsed_ms >= ctx->config.warmup_ms) ppg_ibi_enter_state(ctx, PPG_IBI_STATE_TRACK, input->timestamp_ms);
            else flags |= PPG_IBI_FLAG_WARMUP;
        }

        if (ctx->state == PPG_IBI_STATE_TRACK) {
            float composite = ppg_ibi_process_signal(ctx, input);
            bool low_sqi = (ctx->signal_abs_ema < PPG_IBI_SQI_MIN_ABS);
            if (low_sqi) flags |= PPG_IBI_FLAG_LOW_SQI;
            if (ctx->have_prev1 && ctx->have_prev2) {
                float prev1 = ctx->composite_prev1;
                float prev2 = ctx->composite_prev2;
                bool is_pos_peak = (prev1 > prev2) && (prev1 > composite);
                bool is_neg_peak = (prev1 < prev2) && (prev1 < composite);
                bool refractory_over = (input->timestamp_ms >= ctx->refractory_until_ms);
                if (refractory_over && (is_pos_peak || is_neg_peak)) {
                    int8_t candidate_polarity = is_pos_peak ? 1 : -1;
                    float candidate_amp = fabsf(prev1);
                    float peak_threshold = PPG_IBI_PEAK_THRESHOLD_SCALE * ctx->signal_abs_ema;
                    bool amp_ok = (!low_sqi) && (candidate_amp >= peak_threshold);
                    bool polarity_ok = (!ctx->polarity_locked) || (ctx->locked_polarity == candidate_polarity);
                    if (amp_ok && polarity_ok) {
                        uint32_t beat_ts = input->timestamp_ms - ctx->config.expected_dt_ms;
                        if (!ctx->polarity_locked) { ctx->polarity_locked = true; ctx->locked_polarity = candidate_polarity; }
                        if (!ctx->has_last_beat) { ctx->has_last_beat = true; ctx->last_beat_timestamp_ms = beat_ts; }
                        else {
                            uint32_t ibi_ms = beat_ts - ctx->last_beat_timestamp_ms;
                            if (ibi_ms < ctx->config.ibi_min_ms) {
                                flags |= PPG_IBI_FLAG_IBI_OUT_OF_RANGE;
                            } else if (ibi_ms > ctx->config.ibi_max_ms) {
                                flags |= PPG_IBI_FLAG_IBI_OUT_OF_RANGE;
                                ctx->last_beat_timestamp_ms = beat_ts;
                            } else {
                                output->valid = true; output->ibi_ms = (uint16_t)ibi_ms; output->confidence = ctx->config.confidence_valid_min;
                                ctx->last_beat_timestamp_ms = beat_ts; flags &= ~PPG_IBI_FLAG_NO_BEAT;
                            }
                        }
                        ctx->refractory_until_ms = beat_ts + PPG_IBI_REFRACTORY_MS;
                    }
                }
            }
            ctx->composite_prev2 = ctx->composite_prev1;
            ctx->composite_prev1 = composite;
            ctx->have_prev2 = ctx->have_prev1;
            ctx->have_prev1 = true;
        }
    }

    ctx->samples_since_state_entry++; ctx->last_allow_measure = input->allow_measure;
    ctx->last_timestamp_ms = input->timestamp_ms; ctx->has_last_timestamp = input->allow_measure;
    output->state = ctx->state; output->flags = flags; return PPG_IBI_STATUS_OK;
}
