/**
 * SPDX-License-Identifier: MIT
 *
 * @file: rampg.c
 *
 * @brief
 *    Implementation of rampg, a linear and S-curve (sigmoid) ramp generator.
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

static float
sigmoid_s(float u)
{
        if (u > 0.5f) {
                float v = 1.0f - u;
                return 1.0f
                       - (v * v * v * (10.0f - (v * (15.0f - (6.0f * v)))));
        }
        return u * u * u * (10.0f - (u * (15.0f - (6.0f * u))));
}

static void
sigmoid_plan(rampg_t *ramp)
{
        float end = clamp(ramp->target, ramp->limit_min, ramp->limit_max);
        float diff = end - ramp->value;
        float dist = (diff >= 0.0f) ? diff : -diff;
        float rate = (diff >= 0.0f) ? ramp->rise_rate : ramp->fall_rate;

        ramp->move_start = ramp->value;
        ramp->move_end = end;
        /* Peak slope of the quintic is 1.875, so this makes the peak
         * rate equal to the configured rate. */
        ramp->move_duration = 1.875f * dist / rate;
        ramp->move_elapsed = 0.0f;
        ramp->plan_valid = true;
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
        ramp->shape = RAMPG_DEFAULT_SHAPE;
        ramp->move_start = initial;
        ramp->move_end = initial;
        ramp->move_duration = 0.0f;
        ramp->move_elapsed = 0.0f;
        ramp->plan_valid = false;
}

void
rampg_set_target(rampg_t *ramp, float target)
{
        if (target != ramp->target) {
                ramp->plan_valid = false;
        }
        ramp->target = target;
}

void
rampg_set_rate(rampg_t *ramp, float rate)
{
        if ((ramp->rise_rate != rate) || (ramp->fall_rate != rate)) {
                ramp->plan_valid = false;
        }
        ramp->rise_rate = rate;
        ramp->fall_rate = rate;
}

void
rampg_set_rates(rampg_t *ramp, float rise_rate, float fall_rate)
{
        if ((ramp->rise_rate != rise_rate) || (ramp->fall_rate != fall_rate)) {
                ramp->plan_valid = false;
        }
        ramp->rise_rate = rise_rate;
        ramp->fall_rate = fall_rate;
}

void
rampg_set_limits(rampg_t *ramp, float min, float max)
{
        if ((ramp->limit_min != min) || (ramp->limit_max != max)) {
                ramp->plan_valid = false;
        }
        ramp->limit_min = min;
        ramp->limit_max = max;
        ramp->value = clamp(ramp->value, min, max);
}

void
rampg_set_shape(rampg_t *ramp, rampg_shape_t shape)
{
        if (ramp->shape != shape) {
                ramp->plan_valid = false;
        }
        ramp->shape = shape;
}

float
rampg_update(rampg_t *ramp, float dt)
{
        float effective = clamp(ramp->target, ramp->limit_min, ramp->limit_max);

        if (ramp->shape == RAMPG_SHAPE_SIGMOID) {
                if (!ramp->plan_valid || (effective != ramp->move_end)) {
                        sigmoid_plan(ramp);
                }

                if ((ramp->move_elapsed + dt) >= ramp->move_duration) {
                        ramp->move_elapsed = ramp->move_duration;
                        ramp->value = ramp->move_end;
                } else {
                        ramp->move_elapsed += dt;
                        float u = ramp->move_elapsed / ramp->move_duration;
                        ramp->value = ramp->move_start
                                      + (ramp->move_end - ramp->move_start)
                                            * sigmoid_s(u);
                }
        } else {
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
                } else {
                        /* diff == 0.0f: already at target, nothing to do */
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

float
rampg_get_rate(const rampg_t *ramp)
{
        float effective = clamp(ramp->target, ramp->limit_min, ramp->limit_max);

        if (ramp->value == effective) {
                return 0.0f;
        }

        if (ramp->shape == RAMPG_SHAPE_SIGMOID) {
                if (!ramp->plan_valid || (ramp->move_duration <= 0.0f)) {
                        return 0.0f;
                }
                float u = ramp->move_elapsed / ramp->move_duration;
                float dsdu = 30.0f * u * u * (1.0f - u) * (1.0f - u);
                return (ramp->move_end - ramp->move_start) * dsdu
                       / ramp->move_duration;
        }

        if (effective > ramp->value) {
                return ramp->rise_rate;
        }
        return -ramp->fall_rate;
}

bool
rampg_at_target(const rampg_t *ramp)
{
        float effective = clamp(ramp->target, ramp->limit_min, ramp->limit_max);
        return ramp->value == effective;
}

rampg_state_t
rampg_get_state(const rampg_t *ramp)
{
        float effective = clamp(ramp->target, ramp->limit_min, ramp->limit_max);

        if (ramp->value == effective) {
                return RAMPG_STATE_AT_TARGET;
        }
        return RAMPG_STATE_MOVING;
}

void
rampg_reset(rampg_t *ramp, float value)
{
        ramp->value = clamp(value, ramp->limit_min, ramp->limit_max);
        ramp->target = value;
        ramp->plan_valid = false;
}
