#ifndef PPG_IBI_H
#define PPG_IBI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPG_IBI_NUM_CHANNELS (4u)

#define PPG_IBI_FLAG_NONE             (0u)
#define PPG_IBI_FLAG_WARMUP           (1u << 0)
#define PPG_IBI_FLAG_MOTION_HOLD      (1u << 1)
#define PPG_IBI_FLAG_LOW_SQI          (1u << 2)
#define PPG_IBI_FLAG_DROPOUT_LIGHT    (1u << 3)
#define PPG_IBI_FLAG_DROPOUT_SEVERE   (1u << 4)
#define PPG_IBI_FLAG_IBI_OUT_OF_RANGE (1u << 5)
#define PPG_IBI_FLAG_NO_BEAT          (1u << 6)

typedef enum {
    PPG_IBI_STATUS_OK = 0,
    PPG_IBI_STATUS_NULL_ARGUMENT = 1,
    PPG_IBI_STATUS_INVALID_CONFIG = 2
} ppg_ibi_status_t;

typedef enum {
    PPG_IBI_STATE_INIT = 0,
    PPG_IBI_STATE_ACQUIRE = 1,
    PPG_IBI_STATE_TRACK = 2,
    PPG_IBI_STATE_HOLD = 3,
    PPG_IBI_STATE_REACQUIRE = 4
} ppg_ibi_state_t;

typedef struct {
    uint16_t sample_rate_hz;
    uint16_t expected_dt_ms;
    uint16_t warmup_ms;
    uint16_t dropout_light_ms;
    uint16_t dropout_severe_ms;
    uint16_t ibi_min_ms;
    uint16_t ibi_max_ms;
    float confidence_valid_min;
} ppg_ibi_config_t;

typedef struct {
    uint32_t timestamp_ms;
    int32_t ppg[PPG_IBI_NUM_CHANNELS];
    bool allow_measure;
} ppg_ibi_input_t;

typedef struct {
    bool valid;
    uint16_t ibi_ms;
    float confidence;
    ppg_ibi_state_t state;
    uint32_t flags;
} ppg_ibi_output_t;

typedef struct {
    ppg_ibi_config_t config;
    ppg_ibi_state_t state;
    bool initialized;
    bool has_last_timestamp;
    bool last_allow_measure;
    uint32_t last_timestamp_ms;
    uint32_t state_entry_timestamp_ms;
    uint32_t samples_since_state_entry;
    uint32_t process_count;
    uint32_t dropout_light_count;
    uint32_t dropout_severe_count;

    float dc[PPG_IBI_NUM_CHANNELS];
    float smooth[PPG_IBI_NUM_CHANNELS];
    bool has_filter_state;
    float composite_prev2;
    float composite_prev1;
    uint32_t timestamp_prev2_ms;
    uint32_t timestamp_prev1_ms;
    bool has_composite_prev1;
    bool has_composite_prev2;
    float abs_ema;
    int8_t peak_polarity;
    bool has_last_beat;
    uint32_t last_beat_timestamp_ms;
    uint32_t beat_count;
    uint32_t valid_ibi_count;
} ppg_ibi_context_t;

void ppg_ibi_get_default_config(ppg_ibi_config_t *config);

ppg_ibi_status_t ppg_ibi_init(ppg_ibi_context_t *ctx,
                              const ppg_ibi_config_t *config);

ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx);

ppg_ibi_status_t ppg_ibi_process(ppg_ibi_context_t *ctx,
                                 const ppg_ibi_input_t *input,
                                 ppg_ibi_output_t *output);

#ifdef __cplusplus
}
#endif

#endif
