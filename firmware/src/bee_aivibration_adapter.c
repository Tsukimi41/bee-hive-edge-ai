#include "bee_aivibration_adapter.h"

#include "AI.h"
#include "ConfigData.h"
#include "irq.h"

#include <stddef.h>

#define BEE_DEFAULT_PREDICTIONS_PER_OBSERVATION (7u)

static bee_monitor_t monitor;
static bee_monitor_config_t monitor_config;
static bee_aivibration_config_t adapter_config;
static float maximum_anomaly;
static uint16_t prediction_count;

static float clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static float read_normalized_anomaly(void)
{
    float anomaly;
    float full_scale = 1.0f;

    __disable_irq();
    anomaly = AIGetFloatAnomalyValue();
    __enable_irq();

    if (ConfigDataGetFloatValue(EN_CONFIG_MAX_ABNORMAL, &full_scale)
            != CONFIG_DATA_RESULT_NORMAL
        || full_scale <= 0.0f) {
        full_scale = 1.0f;
    }
    return clamp01(anomaly / full_scale);
}

bee_aivibration_config_t bee_aivibration_default_config(void)
{
    bee_aivibration_config_t config = {
        .predictions_per_observation = BEE_DEFAULT_PREDICTIONS_PER_OBSERVATION,
        .read_environment = NULL
    };
    return config;
}

void bee_aivibration_init(const bee_aivibration_config_t *config)
{
    adapter_config = config != NULL ? *config : bee_aivibration_default_config();
    if (adapter_config.predictions_per_observation == 0u) {
        adapter_config.predictions_per_observation = 1u;
    }

    monitor_config = bee_monitor_default_config();
    bee_monitor_init(&monitor);
    maximum_anomaly = 0.0f;
    prediction_count = 0u;
}

bool bee_aivibration_process_latest(void)
{
    bee_observation_t observation = {0};
    const float anomaly = read_normalized_anomaly();

    if (anomaly > maximum_anomaly) maximum_anomaly = anomaly;
    prediction_count++;
    if (prediction_count < adapter_config.predictions_per_observation) return false;

    if (adapter_config.read_environment != NULL) {
        (void)adapter_config.read_environment(&observation);
    }
    observation.ai_anomaly = maximum_anomaly;
    (void)bee_monitor_update(&monitor, &monitor_config, &observation);

    maximum_anomaly = 0.0f;
    prediction_count = 0u;
    return true;
}

bee_state_t bee_aivibration_get_state(void)
{
    return monitor.state;
}

float bee_aivibration_get_fused_score(void)
{
    return monitor.fused_score;
}
