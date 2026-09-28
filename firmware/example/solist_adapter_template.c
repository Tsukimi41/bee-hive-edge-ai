/*
 * AISignalInference integration template.
 *
 * Copy bee_monitor.c/.h into the supplied LEXIDE project, then replace the
 * three TODO(SOLIST) calls below with the real symbols in that exact version
 * of the vendor sample. The public manuals document behavior, but not the C
 * API names, so guessing those names here would make the project unsafe.
 */
#include "bee_monitor.h"

static bee_monitor_t g_bee;
static bee_monitor_config_t g_config;

void bee_app_init(void)
{
    g_config = bee_monitor_default_config();
    bee_monitor_init(&g_bee);
}

void bee_app_tick_1hz(void)
{
    bee_observation_t o;

    /* TODO(SOLIST): normalize the sample's current AI anomaly to 0.0..1.0. */
    o.ai_anomaly = 0.0f;

    /* TODO(SOLIST): read BME280/SHT3x and HX711 through the board drivers. */
    o.temperature_c = 0.0f;
    o.humidity_rh = 0.0f;
    o.weight_kg = 0.0f;
    o.temperature_valid = false;
    o.humidity_valid = false;
    o.weight_valid = false;

    const bee_state_t state = bee_monitor_update(&g_bee, &g_config, &o);

    /* TODO(SOLIST): map state to LCD, LEDs, SSR OUT0 and minimal UART flag. */
    (void)state;
}

