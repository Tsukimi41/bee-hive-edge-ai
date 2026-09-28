#ifndef BEE_MONITOR_H
#define BEE_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BEE_STATE_LEARNING = 0,
    BEE_STATE_NORMAL,
    BEE_STATE_WARNING,
    BEE_STATE_ALERT
} bee_state_t;

typedef struct {
    float audio_weight;
    float temperature_weight;
    float humidity_weight;
    float weight_weight;
    float temperature_delta_c;
    float humidity_delta_rh;
    float weight_drop_ratio;
    float warning_threshold;
    float alert_threshold;
    float clear_threshold;
    float baseline_alpha;
    uint16_t baseline_samples;
    uint16_t warning_consecutive;
    uint16_t alert_consecutive;
    uint16_t clear_consecutive;
} bee_monitor_config_t;

typedef struct {
    float ai_anomaly;       /* normalized 0.0 .. 1.0 */
    float temperature_c;
    float humidity_rh;
    float weight_kg;
    bool temperature_valid;
    bool humidity_valid;
    bool weight_valid;
} bee_observation_t;

typedef struct {
    bee_state_t state;
    float fused_score;
    float environmental_score;
    float baseline_temperature_c;
    float baseline_humidity_rh;
    float baseline_weight_kg;
    uint32_t samples_seen;
    uint16_t warning_count;
    uint16_t alert_count;
    uint16_t clear_count;
} bee_monitor_t;

bee_monitor_config_t bee_monitor_default_config(void);
void bee_monitor_init(bee_monitor_t *monitor);
bee_state_t bee_monitor_update(
    bee_monitor_t *monitor,
    const bee_monitor_config_t *config,
    const bee_observation_t *observation);
void bee_monitor_acknowledge(bee_monitor_t *monitor);
const char *bee_monitor_state_name(bee_state_t state);

#endif

