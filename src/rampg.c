/**
 * SPDX-License-Identifier: MIT
 *
 * @file: rampg.c
 *
 * @brief
 *    Implementation of rampg, a linear and S-curve ramp generator.
 */

/* ================ INCLUDES ================================================ */

#include "rampg.h"

#include <math.h>

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
absf(float val)
{
        return (val < 0.0f) ? -val : val;
}

/*
 * Effective target: the stored target clamped to the active limits. The
 * stored target is left alone so that widening the limits later recovers
 * the caller's original intent.
 */
static float
effective_target(const rampg_t *ramp)
{
        return clamp(ramp->target, ramp->limit_min, ramp->limit_max);
}

/*
 * Braking envelope: the greatest speed the output may have with `dist`
 * left to travel and still come to rest exactly on the target without
 * ever changing the rate by more than `accel * dt` per update.
 *
 * rampg_update() applies the new rate to the value within the same
 * update, so decelerating from v advances the value by v, v - accel*dt,
 * ... down to zero. Summing those steps gives the distance needed to
 * stop, dist(v) = v^2 / (2 * accel) + v * dt / 2, and inverting it for v
 * gives the envelope. The rationalised form below is algebraically the
 * positive root but avoids the cancellation the direct root suffers as
 * dist approaches zero. Its denominator is at least 2 * accel * dt, so
 * it is well defined for any accel > 0 and dt > 0.
 *
 * The envelope is applied as a clamp and never as a rate to track. A
 * rate-limited follower chasing it is unstable: the tracking error obeys
 * de/dstep = accel * dt * e / vb and so grows without bound as vb falls
 * to zero, which lands the ramp on the target at speed. Clamping keeps
 * the state on or inside the envelope, where no error can accumulate.
 * See docs/s-curve-profile.md for the derivation.
 */
static float
brake_envelope(float dist, float accel, float dt)
{
        float adt = accel * dt;
        float root = sqrtf((adt * adt) + (8.0f * accel * dist));

        return (4.0f * accel * dist) / (adt + root);
}

/*
 * One acceleration-limited step toward `target`. Returns the value the
 * ramp should take, and updates ramp->vel to the rate applied.
 */
static float
scurve_step(rampg_t *ramp, float target, float rate, float dt)
{
        float diff = target - ramp->value;
        float dist = absf(diff);
        float adt = ramp->accel * dt;

        /* Approach the cruise rate, no faster than the acceleration limit. */
        float want = (diff >= 0.0f) ? rate : -rate;
        float delta = clamp(want - ramp->vel, -adt, adt);
        float vel = ramp->vel + delta;

        /* Never exceed the speed from which the ramp can still stop. */
        float envelope = brake_envelope(dist, ramp->accel, dt);
        vel = clamp(vel, -envelope, envelope);
        ramp->vel = vel;

        float next = ramp->value + (vel * dt);
        bool reached = (diff >= 0.0f) ? (next >= target) : (next <= target);
        if (reached) {
                /* The step reaches or passes the target: stop on it. */
                ramp->vel = 0.0f;
                return target;
        }

        if (next == ramp->value) {
                /*
                 * The step is below the resolution of the value. If the
                 * envelope is what bounds the rate then the rate can only
                 * fall from here and the target can never be reached by
                 * accumulation, so land on it. Otherwise the rate is still
                 * rising and will clear the resolution floor, so hold.
                 */
                if (absf(vel) >= envelope) {
                        ramp->vel = 0.0f;
                        return target;
                }
                return ramp->value;
        }

        return next;
}

/*
 * One constant-rate step toward `target`, snapping rather than
 * overshooting.
 */
static float
linear_step(const rampg_t *ramp, float target, float rate, float dt)
{
        float diff = target - ramp->value;
        float step = rate * dt;

        if (absf(diff) <= step) {
                return target;
        }
        return (diff > 0.0f) ? (ramp->value + step) : (ramp->value - step);
}

/* ================ GLOBAL FUNCTIONS ======================================== */

void
rampg_init(rampg_t *ramp, float initial)
{
        ramp->target = initial;
        ramp->rise_rate = RAMPG_DEFAULT_RATE;
        ramp->fall_rate = RAMPG_DEFAULT_RATE;
        ramp->accel = RAMPG_DEFAULT_ACCEL;
        ramp->limit_min = RAMPG_LIMIT_MIN;
        ramp->limit_max = RAMPG_LIMIT_MAX;
        /* Establish the invariant the update relies on: the value is always
         * inside the limits. The target is left unclamped, as in
         * rampg_reset(), so widening the limits recovers the caller's
         * intent. */
        ramp->value = clamp(initial, ramp->limit_min, ramp->limit_max);
        ramp->vel = 0.0f;
        ramp->shape = RAMPG_DEFAULT_SHAPE;
        ramp->enabled = true;
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
rampg_set_accel(rampg_t *ramp, float accel)
{
        ramp->accel = accel;
}

void
rampg_set_limits(rampg_t *ramp, float min, float max)
{
        float clamped = clamp(ramp->value, min, max);

        ramp->limit_min = min;
        ramp->limit_max = max;

        if (clamped != ramp->value) {
                /* The output was displaced, not ramped: it has no rate. */
                ramp->value = clamped;
                ramp->vel = 0.0f;
        }
}

void
rampg_set_shape(rampg_t *ramp, rampg_shape_t shape)
{
        if ((ramp->shape == RAMPG_SHAPE_LINEAR)
            && (shape == RAMPG_SHAPE_SCURVE)) {
                /*
                 * Seed the S-curve with the rate the linear ramp was
                 * running at, so the output rate is continuous across the
                 * switch instead of restarting from rest.
                 */
                ramp->vel = rampg_get_rate(ramp);
        }
        ramp->shape = shape;
}

void
rampg_set_enabled(rampg_t *ramp, bool enabled)
{
        if (!enabled) {
                /* A held output is at rest, whatever it was doing before. */
                ramp->vel = 0.0f;
        }
        ramp->enabled = enabled;
}

float
rampg_update(rampg_t *ramp, float dt)
{
        if (!ramp->enabled) {
                return ramp->value;
        }

        /* Reject a step that is not a finite, positive duration. */
        if (!isfinite(dt) || (dt <= 0.0f)) {
                return ramp->value;
        }

        float target = effective_target(ramp);
        if (!isfinite(target)) {
                return ramp->value;
        }

        float rate =
            (target >= ramp->value) ? ramp->rise_rate : ramp->fall_rate;
        if (!(rate > 0.0f)) {
                return ramp->value;
        }

        if (ramp->shape == RAMPG_SHAPE_SCURVE) {
                if (!(ramp->accel > 0.0f)) {
                        return ramp->value;
                }
                ramp->value = scurve_step(ramp, target, rate, dt);
        } else {
                ramp->value = linear_step(ramp, target, rate, dt);
        }

        /*
         * No clamp is needed here. Every entry point that writes the value
         * leaves it inside the limits, and a step moves monotonically toward
         * the already-clamped target without overshooting it, so the value
         * cannot leave the range it started in.
         */
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

        if (ramp->shape == RAMPG_SHAPE_SCURVE) {
                return ramp->vel;
        }

        float target = effective_target(ramp);
        if (ramp->value == target) {
                return 0.0f;
        }
        return (target > ramp->value) ? ramp->rise_rate : -ramp->fall_rate;
}

bool
rampg_at_target(const rampg_t *ramp)
{
        return ramp->value == effective_target(ramp);
}

rampg_state_t
rampg_get_state(const rampg_t *ramp)
{
        if (!ramp->enabled) {
                return RAMPG_STATE_DISABLED;
        }
        if (ramp->value == effective_target(ramp)) {
                return RAMPG_STATE_AT_TARGET;
        }
        return RAMPG_STATE_MOVING;
}

void
rampg_reset(rampg_t *ramp, float value)
{
        ramp->value = clamp(value, ramp->limit_min, ramp->limit_max);
        ramp->target = value;
        ramp->vel = 0.0f;
}
