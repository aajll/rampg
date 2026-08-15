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

/* Peak of the normalized quintic smoothstep derivative s'(u), attained at
 * u = 0.5. The move duration is scaled by this so that the S-curve's peak
 * rate equals the configured rate. See docs/s-curve-profile.md for the
 * derivation. */
#define SIGMOID_PEAK_SLOPE 1.875f

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

/*
 * Normalized quintic smoothstep S(u) = 6u^5 - 15u^4 + 10u^3, mapping
 * u in [0, 1] to [0, 1]. It satisfies S(0) = 0, S(1) = 1 and
 * S'(0) = S'(1) = 0, so the rate is continuous at the start and end
 * of a move. Being a quintic it also has S''(0) = S''(1) = 0, which
 * keeps the acceleration continuous as well (a C^2 S-curve). The
 * u > 0.5 branch computes S(1 - v) = 1 - S(v) with the small
 * v = 1 - u, avoiding the cancellation the direct polynomial suffers
 * near u = 1. See docs/s-curve-profile.md for the full derivation.
 */
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

/*
 * Plans a sigmoid move from the current value to the clamped target.
 * Because the output follows the normalized quintic S(u), the peak rate
 * is (end - start) * SIGMOID_PEAK_SLOPE / duration, and the duration
 * below is chosen so that this peak equals the configured rate.
 * See docs/s-curve-profile.md.
 */
static void
sigmoid_plan(rampg_t *ramp)
{
        float end = clamp(ramp->target, ramp->limit_min, ramp->limit_max);
        float diff = end - ramp->value;
        float dist = (diff >= 0.0f) ? diff : -diff;
        float rate = (diff >= 0.0f) ? ramp->rise_rate : ramp->fall_rate;

        ramp->move_start = ramp->value;
        ramp->move_end = end;
        /*
         * Peak rate of the move is (end - start) * max(s') / duration, with
         * max(s') = SIGMOID_PEAK_SLOPE at u = 0.5. Solving for duration so
         * that the peak rate equals the configured rate gives
         * duration = SIGMOID_PEAK_SLOPE * dist / rate.
         */
        ramp->move_duration = SIGMOID_PEAK_SLOPE * dist / rate;
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
        ramp->enabled = true;
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

void
rampg_set_enabled(rampg_t *ramp, bool enabled)
{
        ramp->enabled = enabled;
}

float
rampg_update(rampg_t *ramp, float dt)
{
        if (!ramp->enabled) {
                return ramp->value;
        }

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

bool
rampg_is_enabled(const rampg_t *ramp)
{
        return ramp->enabled;
}

float
rampg_get_rate(const rampg_t *ramp)
{
        if (!ramp->enabled) {
                return 0.0f;
        }

        float effective = clamp(ramp->target, ramp->limit_min, ramp->limit_max);

        if (ramp->value == effective) {
                return 0.0f;
        }

        if (ramp->shape == RAMPG_SHAPE_SIGMOID) {
                if (!ramp->plan_valid || (ramp->move_duration <= 0.0f)) {
                        return 0.0f;
                }
                float u = ramp->move_elapsed / ramp->move_duration;
                /*
                 * d/du of the quintic S(u) = 6u^5 - 15u^4 + 10u^3 is
                 * S'(u) = 30u^2(1 - u)^2; chain rule gives rate =
                 * (end - start) * S'(u) / duration.
                 */
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
