/**
 * @file rampg.c
 * @brief Implementation of rampg — a linear ramp generator.
 */

/* ================ INCLUDES ================================================ */

#include "rampg.h"

/* ================ DEFINES ================================================= */

/* ================ STRUCTURES ============================================== */

/* ================ TYPEDEFS ================================================ */

/* ================ STATIC PROTOTYPES ======================================= */

/* ================ STATIC VARIABLES ======================================== */

/* ================ MACROS ================================================== */

/* ================ STATIC FUNCTIONS ======================================== */

static float
clamp(float val, float min, float max)
{
        if (val < min) {
                return min;
        }
        if (val > max) {
                return max;
        }
        return val;
}

/* ================ GLOBAL FUNCTIONS ======================================== */

void
rampg_init(rampg_t *ramp, float initial)
{
        ramp->value = initial;
        ramp->target = initial;
        ramp->rise_rate = RAMPG_DEFAULT_RATE;
        ramp->fall_rate = RAMPG_DEFAULT_RATE;
        ramp->limit_min = RAMPG_LIMIT_MIN;
        ramp->limit_max = RAMPG_LIMIT_MAX;
}

void
rampg_set_target(rampg_t *ramp, float target)
{
        ramp->target = target;
}

void
rampg_set_rate(rampg_t *ramp, float rate)
{
        ramp->rise_rate = rate;
        ramp->fall_rate = rate;
}

void
rampg_set_rates(rampg_t *ramp, float rise_rate, float fall_rate)
{
        ramp->rise_rate = rise_rate;
        ramp->fall_rate = fall_rate;
}

void
rampg_set_limits(rampg_t *ramp, float min, float max)
{
        ramp->limit_min = min;
        ramp->limit_max = max;
        ramp->value = clamp(ramp->value, min, max);
}

float
rampg_update(rampg_t *ramp, float dt)
{
        float effective = clamp(ramp->target, ramp->limit_min, ramp->limit_max);
        float diff = effective - ramp->value;

        if (diff > 0.0f) {
                float step = ramp->rise_rate * dt;
                if (step >= diff) {
                        ramp->value = effective;
                } else {
                        ramp->value += step;
                }
        } else if (diff < 0.0f) {
                float step = ramp->fall_rate * dt;
                if (step >= -diff) {
                        ramp->value = effective;
                } else {
                        ramp->value -= step;
                }
        }

        ramp->value = clamp(ramp->value, ramp->limit_min, ramp->limit_max);

        return ramp->value;
}

float
rampg_get(const rampg_t *ramp)
{
        return ramp->value;
}

bool
rampg_at_target(const rampg_t *ramp)
{
        float effective = clamp(ramp->target, ramp->limit_min, ramp->limit_max);
        return ramp->value == effective;
}

void
rampg_reset(rampg_t *ramp, float value)
{
        ramp->value = clamp(value, ramp->limit_min, ramp->limit_max);
        ramp->target = value;
}
