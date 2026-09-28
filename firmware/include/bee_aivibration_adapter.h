#ifndef BEE_AIVIBRATION_ADAPTER_H
#define BEE_AIVIBRATION_ADAPTER_H

#include "bee_monitor.h"

#include <stdbool.h>
#include <stdint.h>

typedef bool (*bee_environment_reader_t)(bee_observation_t *observation);

typedef struct {
    uint16_t predictions_per_observation;
    bee_environment_reader_t read_environment;
} bee_aivibration_config_t;

bee_aivibration_config_t bee_aivibration_default_config(void);
void bee_aivibration_init(const bee_aivibration_config_t *adapter_config);
bool bee_aivibration_process_latest(void);
bee_state_t bee_aivibration_get_state(void);
float bee_aivibration_get_fused_score(void);

#endif
