#include "bee_monitor.h"

#include <stddef.h>

static float clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

static float positive_ratio(float value, float threshold)
{
    if (threshold <= 0.0f || value <= 0.0f) return 0.0f;
    return clamp01(value / threshold);
}

static uint16_t increment_saturated(uint16_t value)
{
    return value < UINT16_MAX ? (uint16_t)(value + 1u) : UINT16_MAX;
}

bee_monitor_config_t bee_monitor_default_config(void)
{
    bee_monitor_config_t c = {
        .audio_weight = 0.75f,
        .temperature_weight = 0.45f,
        .humidity_weight = 0.15f,
        .weight_weight = 0.40f,
        .temperature_delta_c = 2.0f,
        .humidity_delta_rh = 15.0f,
        .weight_drop_ratio = 0.05f,
        .warning_threshold = 0.45f,
        .alert_threshold = 0.70f,
        .clear_threshold = 0.30f,
        .baseline_alpha = 0.001f,
        .baseline_samples = 60u,
        .warning_consecutive = 5u,
        .alert_consecutive = 3u,
        .clear_consecutive = 30u
    };
    return c;
}

void bee_monitor_init(bee_monitor_t *m)
{
    if (m == NULL) return;
    m->state = BEE_STATE_LEARNING;
    m->fused_score = 0.0f;
    m->environmental_score = 0.0f;
    m->baseline_temperature_c = 0.0f;
    m->baseline_humidity_rh = 0.0f;
    m->baseline_weight_kg = 0.0f;
    m->samples_seen = 0u;
    m->warning_count = 0u;
    m->alert_count = 0u;
    m->clear_count = 0u;
}

static void initialize_baseline(bee_monitor_t *m, const bee_observation_t *o)
{
    const float n = (float)m->samples_seen;
    const float denominator = n + 1.0f;
    if (o->temperature_valid) {
        m->baseline_temperature_c = (m->baseline_temperature_c * n + o->temperature_c) / denominator;
    }
    if (o->humidity_valid) {
        m->baseline_humidity_rh = (m->baseline_humidity_rh * n + o->humidity_rh) / denominator;
    }
    if (o->weight_valid) {
        m->baseline_weight_kg = (m->baseline_weight_kg * n + o->weight_kg) / denominator;
    }
}

static void track_baseline(bee_monitor_t *m, const bee_monitor_config_t *c, const bee_observation_t *o)
{
    const float a = clamp01(c->baseline_alpha);
    if (o->temperature_valid) m->baseline_temperature_c += a * (o->temperature_c - m->baseline_temperature_c);
    if (o->humidity_valid) m->baseline_humidity_rh += a * (o->humidity_rh - m->baseline_humidity_rh);
    if (o->weight_valid) m->baseline_weight_kg += a * (o->weight_kg - m->baseline_weight_kg);
}

static float environmental_score(const bee_monitor_t *m, const bee_monitor_config_t *c, const bee_observation_t *o)
{
    float score = 0.0f;
    float used_weight = 0.0f;

    if (o->temperature_valid) {
        score += c->temperature_weight * positive_ratio(
            o->temperature_c - m->baseline_temperature_c, c->temperature_delta_c);
        used_weight += c->temperature_weight;
    }
    if (o->humidity_valid) {
        float delta = o->humidity_rh - m->baseline_humidity_rh;
        if (delta < 0.0f) delta = -delta;
        score += c->humidity_weight * positive_ratio(delta, c->humidity_delta_rh);
        used_weight += c->humidity_weight;
    }
    if (o->weight_valid && m->baseline_weight_kg > 0.01f) {
        const float drop = (m->baseline_weight_kg - o->weight_kg) / m->baseline_weight_kg;
        score += c->weight_weight * positive_ratio(drop, c->weight_drop_ratio);
        used_weight += c->weight_weight;
    }
    return used_weight > 0.0f ? clamp01(score / used_weight) : 0.0f;
}

bee_state_t bee_monitor_update(bee_monitor_t *m, const bee_monitor_config_t *c, const bee_observation_t *o)
{
    if (m == NULL || c == NULL || o == NULL) return BEE_STATE_ALERT;

    if (m->samples_seen < c->baseline_samples) {
        initialize_baseline(m, o);
        m->samples_seen++;
        if (m->samples_seen >= c->baseline_samples) m->state = BEE_STATE_NORMAL;
        return m->state;
    }

    m->environmental_score = environmental_score(m, c, o);
    m->fused_score = clamp01(
        c->audio_weight * clamp01(o->ai_anomaly) +
        (1.0f - c->audio_weight) * m->environmental_score);

    m->warning_count = m->fused_score >= c->warning_threshold ? increment_saturated(m->warning_count) : 0u;
    m->alert_count = m->fused_score >= c->alert_threshold ? increment_saturated(m->alert_count) : 0u;
    m->clear_count = m->fused_score <= c->clear_threshold ? increment_saturated(m->clear_count) : 0u;

    if (m->alert_count >= c->alert_consecutive) {
        m->state = BEE_STATE_ALERT;
    } else if (m->state != BEE_STATE_ALERT && m->warning_count >= c->warning_consecutive) {
        m->state = BEE_STATE_WARNING;
    } else if (m->state == BEE_STATE_WARNING && m->clear_count >= c->clear_consecutive) {
        m->state = BEE_STATE_NORMAL;
    }

    if (m->state == BEE_STATE_NORMAL && m->fused_score < c->warning_threshold) {
        track_baseline(m, c, o);
    }
    m->samples_seen++;
    return m->state;
}

void bee_monitor_acknowledge(bee_monitor_t *m)
{
    if (m == NULL) return;
    if (m->state == BEE_STATE_ALERT && m->clear_count > 0u) {
        m->state = BEE_STATE_WARNING;
        m->alert_count = 0u;
    }
}

const char *bee_monitor_state_name(bee_state_t state)
{
    switch (state) {
        case BEE_STATE_LEARNING: return "LEARNING";
        case BEE_STATE_NORMAL: return "NORMAL";
        case BEE_STATE_WARNING: return "WARNING";
        case BEE_STATE_ALERT: return "ALERT";
        default: return "UNKNOWN";
    }
}
