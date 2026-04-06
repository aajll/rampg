/**
 * @file rampg.h
 * @brief Public API for rampg — a linear ramp generator.
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
 * @brief Ramp generator state.
 *
 * Plain struct — caller owns storage (stack, static, or embedded in a
 * larger struct). Initialise with rampg_init() before use.
 */
typedef struct {
        float value;     /**< Current output value. */
        float target;    /**< Target value. */
        float rise_rate; /**< Rise rate (units/s). */
        float fall_rate; /**< Fall rate (units/s). */
        float limit_min; /**< Output clamp minimum. */
        float limit_max; /**< Output clamp maximum. */
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
 * @brief Advance the ramp by @p dt seconds.
 *
 * Moves the output value toward the effective target (target clamped
 * to limits) at the configured rate, then applies output clamping.
 * The stored target is not modified, so widening limits later recovers
 * the original intent.
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
