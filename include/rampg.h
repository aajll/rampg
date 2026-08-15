/**
 * SPDX-License-Identifier: MIT
 *
 * @file: rampg.h
 *
 * @brief
 *    Public API for rampg, a linear and S-curve (sigmoid) ramp generator.
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
 * RAMPG_SHAPE_LINEAR: constant-rate step toward the target.
 * RAMPG_SHAPE_SIGMOID: quintic S-curve with a flat start and end,
 * sized so that its peak rate equals the configured ramp rate.
 */
typedef enum {
        RAMPG_SHAPE_LINEAR,  /**< Constant-rate step. */
        RAMPG_SHAPE_SIGMOID, /**< Quintic S-curve at the same peak rate. */
} rampg_shape_t;

/**
 * @brief Ramp generator state.
 *
 * @details
 * - ::RAMPG_STATE_MOVING    output is ramping toward the effective target.
 * - ::RAMPG_STATE_AT_TARGET output is at rest at the effective target.
 */
typedef enum {
        RAMPG_STATE_MOVING,
        RAMPG_STATE_AT_TARGET,
} rampg_state_t;

/**
 * @brief Ramp generator instance.
 *
 * Plain struct; the caller owns the storage (stack, static, or embedded
 * in a larger struct). Initialise with rampg_init() before use.
 */
typedef struct {
        float value;         /**< Current output value. */
        float target;        /**< Target value. */
        float rise_rate;     /**< Rise rate (units/s). */
        float fall_rate;     /**< Fall rate (units/s). */
        float limit_min;     /**< Output clamp minimum. */
        float limit_max;     /**< Output clamp maximum. */
        rampg_shape_t shape; /**< Ramp profile (linear or S-curve). */
        float move_start;    /**< Start value of the planned move. */
        float move_end;      /**< End value of the planned move. */
        float move_duration; /**< Planned move duration in seconds. */
        float move_elapsed;  /**< Elapsed move time in seconds. */
        bool plan_valid;     /**< True when the planned move is current. */
        bool enabled;        /**< True when the ramp is enabled. */
} rampg_t;

/* ================ TYPEDEFS ================================================ */

/* ================ MACROS ================================================== */

/* ================ GLOBAL VARIABLES ======================================== */

/* ================ GLOBAL PROTOTYPES ======================================= */

/**
 * @brief Initialise a ramp generator.
 *
 * Sets both value and target to @p initial. Rates default to
 * RAMPG_DEFAULT_RATE and limits default to RAMPG_LIMIT_MIN / RAMPG_LIMIT_MAX.
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
 * @brief Set output clamp limits.
 *
 * The current value is immediately clamped to the new range.
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
 * Switching between RAMPG_SHAPE_LINEAR and RAMPG_SHAPE_SIGMOID takes
 * effect at the next rampg_update(). A pending move is re-planned from
 * the current value, so no discontinuity is introduced.
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
 * unchanged and rampg_get_rate() reads zero. The target, rates, and
 * limits are preserved, so re-enabling resumes the move from the
 * current value. A ramp is enabled by default after rampg_init().
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
 * Moves the output value toward the effective target (target clamped
 * to limits) using the configured shape, then applies output clamping.
 * The stored target is not modified, so widening limits later recovers
 * the original intent.
 *
 * LINEAR shape steps at the configured rate. SIGMOID shape follows a
 * quintic S-curve sized so that its peak rate equals the configured
 * rate; the move is re-planned from the current value whenever the
 * effective target, limits, or rates change.
 *
 * @pre @p ramp has been initialised with rampg_init().
 * @pre @p dt >= 0.
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
 * @brief Read the current effective rate without advancing.
 *
 * For LINEAR shape this is the configured rise or fall rate while
 * moving, or zero when at rest. For SIGMOID shape this is the
 * instantaneous slope of the planned S-curve: it peaks at the
 * configured rate mid-move and falls to zero at the end. A target,
 * rate, limit, or shape change invalidates the plan, so the rate
 * reads zero until the next rampg_update() re-plans from the
 * current value.
 *
 * @pre @p ramp has been initialised with rampg_init().
 *
 * @param ramp          Pointer to ramp instance.
 * @return              Effective rate in units per second (signed).
 */
float rampg_get_rate(const rampg_t *ramp);

/**
 * @brief Check whether the output has reached the effective target.
 *
 * The effective target is the stored target clamped to the current
 * limits. Returns true when the value equals the effective target.
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
 * RAMPG_STATE_MOVING while the output is ramping toward the effective
 * target, RAMPG_STATE_AT_TARGET when the output is at rest at the
 * effective target (the stored target clamped to the current limits).
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
 * Sets the target to @p value and the output to @p value clamped
 * to the current limits. The unclamped target is preserved.
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
