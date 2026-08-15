/**
 * SPDX-License-Identifier: MIT
 *
 * @file: rampg.h
 *
 * @brief
 *    Public API for rampg, a linear and S-curve ramp generator.
 */

#ifndef RAMPG_H_
#define RAMPG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ================ INCLUDES ================================================ */

#include "rampg_conf.h"

#include <stdbool.h>

/* ================ DEFINES ================================================= */

/* ================ STRUCTURES ============================================== */

/**
 * @brief Ramp profile shape.
 *
 * @details
 * - ::RAMPG_SHAPE_LINEAR steps toward the target at the configured rate.
 *   The output rate is discontinuous at both ends of a move.
 * - ::RAMPG_SHAPE_SCURVE limits how fast the output rate may change, so
 *   the output eases in and out. The rate is bounded by the configured
 *   rate and its rate of change by the configured acceleration.
 */
typedef enum {
        RAMPG_SHAPE_LINEAR, /**< Constant-rate step. */
        RAMPG_SHAPE_SCURVE, /**< Acceleration-limited S-curve. */
} rampg_shape_t;

/**
 * @brief Ramp generator state.
 *
 * @details
 * - ::RAMPG_STATE_MOVING    output is ramping toward the effective target.
 * - ::RAMPG_STATE_AT_TARGET output is at rest at the effective target.
 * - ::RAMPG_STATE_DISABLED  ramp is disabled and holds its output.
 */
typedef enum {
        RAMPG_STATE_MOVING,
        RAMPG_STATE_AT_TARGET,
        RAMPG_STATE_DISABLED,
} rampg_state_t;

/**
 * @brief Ramp generator instance.
 *
 * @details
 * Plain struct; the caller owns the storage (stack, static, or embedded
 * in a larger struct). It must be initialised with rampg_init() before
 * any other call: aggregate initialisation is not sufficient, because
 * the defaults are not all zero.
 *
 * Every field except @c vel is caller-facing configuration and may be
 * read directly. @c vel is internal and is maintained by rampg_update();
 * read the output rate with rampg_get_rate() rather than reading it.
 */
typedef struct {
        float value;         /**< Current output value. */
        float target;        /**< Target value, unclamped. */
        float rise_rate;     /**< Rise rate (units/s). */
        float fall_rate;     /**< Fall rate (units/s). */
        float accel;         /**< Acceleration limit (units/s^2). */
        float limit_min;     /**< Output clamp minimum. */
        float limit_max;     /**< Output clamp maximum. */
        float vel;           /**< Internal: current output rate. */
        rampg_shape_t shape; /**< Ramp profile. */
        bool enabled;        /**< True when the ramp is enabled. */
} rampg_t;

/* ================ TYPEDEFS ================================================ */

/* ================ MACROS ================================================== */

/* ================ GLOBAL VARIABLES ======================================== */

/* ================ GLOBAL PROTOTYPES ======================================= */

/**
 * @brief Initialise a ramp generator.
 *
 * Sets the target to @p initial and the output rate to zero. Rates
 * default to RAMPG_DEFAULT_RATE, the acceleration limit to
 * RAMPG_DEFAULT_ACCEL, limits to RAMPG_LIMIT_MIN / RAMPG_LIMIT_MAX, the
 * shape to RAMPG_DEFAULT_SHAPE, and the ramp is enabled.
 *
 * The output value is @p initial clamped to the default limits; the
 * stored target is left unclamped, so widening the limits later recovers
 * the original intent.
 *
 * @pre @p ramp is not NULL.
 *
 * @param ramp          Pointer to ramp instance.
 * @param initial       Initial output value.
 */
void rampg_init(rampg_t *ramp, float initial);

/**
 * @brief Set the target value.
 *
 * Takes effect at the next rampg_update(). The output rate is carried
 * across the change, so a mid-move retarget does not restart the move.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @param target        Desired target value.
 */
void rampg_set_target(rampg_t *ramp, float target);

/**
 * @brief Set a symmetric ramp rate (same for rise and fall).
 *
 * @pre @p ramp has been initialised with rampg_init().
 * @pre @p rate > 0.
 *
 * @param ramp          Pointer to ramp instance.
 * @param rate          Rate in units per second (must be > 0).
 */
void rampg_set_rate(rampg_t *ramp, float rate);

/**
 * @brief Set asymmetric ramp rates.
 *
 * @pre @p ramp has been initialised with rampg_init().
 * @pre @p rise_rate > 0.
 * @pre @p fall_rate > 0.
 *
 * @param ramp          Pointer to ramp instance.
 * @param rise_rate     Rate when ramping up (units/s, must be > 0).
 * @param fall_rate     Rate when ramping down (units/s, must be > 0).
 */
void rampg_set_rates(rampg_t *ramp, float rise_rate, float fall_rate);

/**
 * @brief Set the acceleration limit.
 *
 * Bounds how fast the output rate may change under RAMPG_SHAPE_SCURVE,
 * and so sets how sharply the ramp eases in and out. A move long enough
 * to reach the configured rate takes @c rate/accel seconds longer than
 * the equivalent linear move. Ignored by RAMPG_SHAPE_LINEAR.
 *
 * @pre @p ramp has been initialised with rampg_init().
 * @pre @p accel > 0.
 *
 * @param ramp          Pointer to ramp instance.
 * @param accel         Acceleration limit (units/s^2, must be > 0).
 */
void rampg_set_accel(rampg_t *ramp, float accel);

/**
 * @brief Set output clamp limits.
 *
 * The current value is immediately clamped to the new range. If that
 * clamp moves the value, the output rate is reset to zero, because the
 * output has been displaced rather than ramped.
 *
 * @pre @p ramp has been initialised with rampg_init().
 * @pre @p min <= @p max.
 *
 * @param ramp          Pointer to ramp instance.
 * @param min           Minimum output value.
 * @param max           Maximum output value.
 */
void rampg_set_limits(rampg_t *ramp, float min, float max);

/**
 * @brief Set the ramp profile shape.
 *
 * Takes effect at the next rampg_update(). Switching from LINEAR to
 * SCURVE seeds the S-curve with the rate the linear ramp was running at,
 * so the output rate is continuous across the switch. Switching from
 * SCURVE to LINEAR resumes at the full configured rate, which is a step
 * in the output rate inherent to the linear profile.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @param shape         Desired ramp profile.
 */
void rampg_set_shape(rampg_t *ramp, rampg_shape_t shape);

/**
 * @brief Enable or disable the ramp.
 *
 * A disabled ramp holds its output: rampg_update() leaves the value
 * unchanged and rampg_get_rate() reads zero. Disabling resets the output
 * rate to zero, so re-enabling eases away from rest rather than resuming
 * at a rate the output no longer has. The target, rates, acceleration,
 * limits, and shape are preserved. A ramp is enabled after rampg_init().
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @param enabled       true to enable, false to disable.
 */
void rampg_set_enabled(rampg_t *ramp, bool enabled);

/**
 * @brief Advance the ramp by @p dt seconds.
 *
 * Moves the output value toward the effective target, the stored target
 * clamped to the limits. The stored target is not modified, so widening
 * limits later recovers the original intent. The output stays inside the
 * limits without a further clamp, because it starts inside them and a
 * step never overshoots the effective target.
 *
 * LINEAR steps at the configured rate. SCURVE bounds the output rate by
 * the configured rate and its rate of change by the configured
 * acceleration, easing in and out of the move.
 *
 * The call is total. It holds the output unchanged, rather than
 * corrupting the state, when @p dt is not a finite value greater than
 * zero, when the effective target is not finite, or when the governing
 * rate (or, under SCURVE, the acceleration) is not greater than zero.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @param dt            Time step in seconds.
 * @return              Current output value after the update.
 */
float rampg_update(rampg_t *ramp, float dt);

/**
 * @brief Read the current output value without advancing.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @return              Current output value.
 */
float rampg_get(const rampg_t *ramp);

/**
 * @brief Read whether the ramp is enabled without advancing.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @return              true if the ramp is enabled and will move.
 */
bool rampg_is_enabled(const rampg_t *ramp);

/**
 * @brief Read the current output rate without advancing.
 *
 * The value is signed: positive when rising, negative when falling. For
 * LINEAR this is the configured rise or fall rate while moving, and zero
 * at rest. For SCURVE it is the rate the last rampg_update() applied,
 * which eases from zero up to at most the configured rate and back down.
 * A disabled ramp reads zero.
 *
 * Suitable as a feedforward term: it stays correct across a target,
 * rate, limit, or shape change, because the rate is carried across the
 * change rather than being recomputed from rest.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @return              Output rate in units per second (signed).
 */
float rampg_get_rate(const rampg_t *ramp);

/**
 * @brief Check whether the output has reached the effective target.
 *
 * The effective target is the stored target clamped to the current
 * limits. Returns true when the value equals the effective target. This
 * is purely positional and ignores the enabled flag; use
 * rampg_get_state() to distinguish a disabled ramp.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @return              true if value == effective target.
 */
bool rampg_at_target(const rampg_t *ramp);

/**
 * @brief Read the current ramp state without advancing.
 *
 * ::RAMPG_STATE_DISABLED whenever the ramp is disabled, otherwise
 * ::RAMPG_STATE_MOVING while the output is ramping toward the effective
 * target and ::RAMPG_STATE_AT_TARGET when it is at rest there.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @return              Current ramp state.
 */
rampg_state_t rampg_get_state(const rampg_t *ramp);

/**
 * @brief Snap the output to @p value immediately (bypass ramp).
 *
 * Sets the target to @p value, the output to @p value clamped to the
 * current limits, and the output rate to zero. The unclamped target is
 * preserved. The enabled flag is not changed.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @param value         Value to snap to.
 */
void rampg_reset(rampg_t *ramp, float value);

#ifdef __cplusplus
}
#endif

#endif /* RAMPG_H_ */
