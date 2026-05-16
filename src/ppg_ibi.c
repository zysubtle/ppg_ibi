#include "ppg_ibi.h"

static bool ppg_ibi_is_valid_config(const ppg_ibi_config_t *config) {
    if (config->sample_rate_hz == 0u) return false;
    if (config->expected_dt_ms == 0u) return false;
    if (config->warmup_ms == 0u) return false;
    if (config->dropout_light_ms >= config->dropout_severe_ms) return false;
    if (config->ibi_min_ms >= config->ibi_max_ms) return false;
    if ((config->confidence_valid_min < 0.0f) || (config->confidence_valid_min > 1.0f)) return false;
    return true;
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

    return PPG_IBI_STATUS_OK;
}

ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx) {
    if (ctx == (ppg_ibi_context_t *)0) {
        return PPG_IBI_STATUS_NULL_ARGUMENT;
    }

    if (!ppg_ibi_is_valid_config(&ctx->config)) {
        return PPG_IBI_STATUS_INVALID_CONFIG;
    }

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

    return PPG_IBI_STATUS_OK;
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
        flags |= PPG_IBI_FLAG_MOTION_HOLD;
    } else {
        if (ctx->state == PPG_IBI_STATE_HOLD) {
            ppg_ibi_enter_state(ctx, PPG_IBI_STATE_REACQUIRE, input->timestamp_ms);
            flags |= PPG_IBI_FLAG_WARMUP;
            ctx->has_last_timestamp = false;
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
            } else if (dt_ms > ctx->config.dropout_light_ms) {
                ctx->dropout_light_count++;
                flags |= PPG_IBI_FLAG_DROPOUT_LIGHT;
            }
        }

        if ((ctx->state == PPG_IBI_STATE_ACQUIRE) || (ctx->state == PPG_IBI_STATE_REACQUIRE)) {
            uint32_t elapsed_ms = input->timestamp_ms - ctx->state_entry_timestamp_ms;
            if (elapsed_ms >= ctx->config.warmup_ms) {
                ppg_ibi_enter_state(ctx, PPG_IBI_STATE_TRACK, input->timestamp_ms);
            } else {
                flags |= PPG_IBI_FLAG_WARMUP;
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
