#include "ppg_ibi.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define M5A_MAX_EVENTS 10000u
#define M5A_MATCH_WINDOW_MS 500u
#define SELF_TEST_SAMPLE_RATE_HZ 50u
#define SELF_TEST_DT_MS 20u
#define SELF_TEST_DURATION_MS 16000u

typedef struct {
    uint32_t timestamp_ms;
    uint16_t ibi_ms;
    float confidence;
    uint32_t flags;
    ppg_ibi_state_t state;
} m5a_pred_event_t;

typedef struct {
    uint32_t timestamp_ms;
    uint16_t ibi_ms;
} m5a_truth_event_t;

typedef struct {
    uint32_t truth_count;
    uint32_t pred_count;
    uint32_t matched_count;
    uint32_t miss_count;
    uint32_t extra_count;
    float mae_ms;
    float mean_error_ms;
    float rmse_ms;
    float max_abs_error_ms;
    float p95_abs_error_ms;
} m5a_metrics_t;

static int append_pred_event(m5a_pred_event_t *events, uint32_t *count, const m5a_pred_event_t *event) {
    if (*count >= M5A_MAX_EVENTS) {
        fprintf(stderr, "error: prediction events exceed limit (%u)\n", M5A_MAX_EVENTS);
        return -1;
    }
    events[*count] = *event;
    (*count)++;
    return 0;
}

static int append_truth_event(m5a_truth_event_t *events, uint32_t *count, const m5a_truth_event_t *event) {
    if (*count >= M5A_MAX_EVENTS) {
        fprintf(stderr, "error: truth events exceed limit (%u)\n", M5A_MAX_EVENTS);
        return -1;
    }
    events[*count] = *event;
    (*count)++;
    return 0;
}

static void sort_float_array(float *arr, uint32_t n) {
    for (uint32_t i = 1u; i < n; ++i) {
        float key = arr[i];
        uint32_t j = i;
        while ((j > 0u) && (arr[j - 1u] > key)) {
            arr[j] = arr[j - 1u];
            --j;
        }
        arr[j] = key;
    }
}

static void compute_metrics(const m5a_pred_event_t *pred_events,
                            uint32_t pred_count,
                            const m5a_truth_event_t *truth_events,
                            uint32_t truth_count,
                            m5a_metrics_t *metrics) {
    bool truth_used[M5A_MAX_EVENTS] = {false};
    float abs_errors[M5A_MAX_EVENTS] = {0.0f};
    float sum_abs = 0.0f;
    float sum_err = 0.0f;
    float sum_sq = 0.0f;
    float max_abs = 0.0f;
    uint32_t matched = 0u;

    memset(metrics, 0, sizeof(*metrics));
    metrics->pred_count = pred_count;
    metrics->truth_count = truth_count;

    for (uint32_t i = 0u; i < pred_count; ++i) {
        int32_t best_j = -1;
        uint32_t best_dt = UINT32_MAX;
        for (uint32_t j = 0u; j < truth_count; ++j) {
            if (truth_used[j]) {
                continue;
            }
            uint32_t pred_ts = pred_events[i].timestamp_ms;
            uint32_t truth_ts = truth_events[j].timestamp_ms;
            uint32_t dt = (pred_ts >= truth_ts) ? (pred_ts - truth_ts) : (truth_ts - pred_ts);
            if ((dt <= M5A_MATCH_WINDOW_MS) && (dt < best_dt)) {
                best_dt = dt;
                best_j = (int32_t)j;
            }
        }

        if (best_j >= 0) {
            uint32_t idx = (uint32_t)best_j;
            float err = (float)((int32_t)pred_events[i].ibi_ms - (int32_t)truth_events[idx].ibi_ms);
            float abs_err = fabsf(err);
            truth_used[idx] = true;
            abs_errors[matched] = abs_err;
            sum_abs += abs_err;
            sum_err += err;
            sum_sq += err * err;
            if (abs_err > max_abs) {
                max_abs = abs_err;
            }
            ++matched;
        }
    }

    metrics->matched_count = matched;
    metrics->extra_count = pred_count - matched;
    metrics->miss_count = truth_count - matched;

    if (matched > 0u) {
        metrics->mae_ms = sum_abs / (float)matched;
        metrics->mean_error_ms = sum_err / (float)matched;
        metrics->rmse_ms = sqrtf(sum_sq / (float)matched);
        metrics->max_abs_error_ms = max_abs;
        sort_float_array(abs_errors, matched);
        {
            uint32_t idx = (uint32_t)((95u * (matched - 1u)) / 100u);
            metrics->p95_abs_error_ms = abs_errors[idx];
        }
    }
}

static int run_self_test_case(uint16_t target_ibi_ms,
                              const char *label,
                              uint32_t *out_truth,
                              uint32_t *out_pred,
                              uint32_t *out_matched,
                              float *out_mae,
                              float *out_max_abs) {
    ppg_ibi_context_t ctx;
    ppg_ibi_config_t cfg;
    ppg_ibi_output_t output;
    m5a_pred_event_t pred_events[M5A_MAX_EVENTS];
    m5a_truth_event_t truth_events[M5A_MAX_EVENTS];
    uint32_t pred_count = 0u;
    uint32_t truth_count = 0u;

    ppg_ibi_get_default_config(&cfg);
    if (cfg.sample_rate_hz != SELF_TEST_SAMPLE_RATE_HZ) {
        cfg.sample_rate_hz = SELF_TEST_SAMPLE_RATE_HZ;
    }
    if (cfg.expected_dt_ms != SELF_TEST_DT_MS) {
        cfg.expected_dt_ms = SELF_TEST_DT_MS;
    }

    if (ppg_ibi_init(&ctx, &cfg) != PPG_IBI_STATUS_OK) {
        fprintf(stderr, "error: ppg_ibi_init failed in %s\n", label);
        return -1;
    }

    const uint32_t samples = SELF_TEST_DURATION_MS / SELF_TEST_DT_MS;
    const uint32_t samples_per_beat = ((uint32_t)target_ibi_ms + (SELF_TEST_DT_MS / 2u)) / SELF_TEST_DT_MS;
    const int32_t baseline = 100000;
    const int32_t amplitude = 18000;
    const int32_t offsets[PPG_IBI_NUM_CHANNELS] = {0, 300, -200, 150};
    for (uint32_t i = 0u; i < samples; ++i) {
        ppg_ibi_input_t in;
        in.timestamp_ms = i * SELF_TEST_DT_MS;
        in.allow_measure = true;

        uint32_t phase = i % samples_per_beat;
        uint32_t half = samples_per_beat / 2u;
        uint32_t pulse_num = (phase <= half) ? phase : (samples_per_beat - phase);
        int32_t pulse = (int32_t)((2u * pulse_num * (uint32_t)amplitude) / ((half > 0u) ? half : 1u));
        int32_t raw = baseline + pulse;

        for (uint32_t ch = 0u; ch < PPG_IBI_NUM_CHANNELS; ++ch) {
            in.ppg[ch] = raw + offsets[ch];
        }

        if ((i > 0u) && (phase == 0u)) {
            m5a_truth_event_t truth = {in.timestamp_ms, target_ibi_ms};
            if (append_truth_event(truth_events, &truth_count, &truth) != 0) {
                return -1;
            }
        }

        if (ppg_ibi_process(&ctx, &in, &output) != PPG_IBI_STATUS_OK) {
            fprintf(stderr, "error: ppg_ibi_process failed in %s\n", label);
            return -1;
        }

        if (output.valid) {
            m5a_pred_event_t pred = {
                in.timestamp_ms,
                output.ibi_ms,
                output.confidence,
                output.flags,
                output.state
            };
            if (append_pred_event(pred_events, &pred_count, &pred) != 0) {
                return -1;
            }
        }
    }

    m5a_metrics_t metrics;
    compute_metrics(pred_events, pred_count, truth_events, truth_count, &metrics);
    *out_truth = metrics.truth_count;
    *out_pred = metrics.pred_count;
    *out_matched = metrics.matched_count;
    *out_mae = metrics.mae_ms;
    *out_max_abs = metrics.max_abs_error_ms;

    bool pass = (metrics.matched_count >= 3u) && (metrics.mae_ms <= 120.0f) && (metrics.max_abs_error_ms <= 200.0f);
    printf("M5a self-test %s: truth=%u pred=%u matched=%u mae_ms=%.1f max_abs_error_ms=%.1f %s\n",
           label,
           metrics.truth_count,
           metrics.pred_count,
           metrics.matched_count,
           metrics.mae_ms,
           metrics.max_abs_error_ms,
           pass ? "pass" : "fail");

    return pass ? 0 : 1;
}

int main(int argc, char **argv) {
    if ((argc == 2) && (strcmp(argv[1], "--self-test") == 0)) {
        uint32_t truth60, pred60, matched60;
        uint32_t truth75, pred75, matched75;
        float mae60, max60, mae75, max75;

        int rc1 = run_self_test_case(1000u, "60bpm", &truth60, &pred60, &matched60, &mae60, &max60);
        int rc2 = run_self_test_case(800u, "75bpm", &truth75, &pred75, &matched75, &mae75, &max75);
        if ((rc1 != 0) || (rc2 != 0)) {
            return 1;
        }
        printf("M5a self-test passed.\n");
        return 0;
    }

    fprintf(stderr, "CSV evaluation is not implemented in M5a; see M5b.\n");
    return 1;
}
