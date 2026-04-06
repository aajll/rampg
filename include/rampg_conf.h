/**
 * @file rampg_conf.h
 * @brief Public configuration header for rampg.
 *
 * @details
 *    This header provides compile-time configuration for the rampg module.
 *    It is included automatically by rampg.h. Override any option by
 *    defining it before including this file.
 *
 *    @code
 *    #define RAMPG_DEFAULT_RATE 500.0f
 *    #include "rampg.h"
 *    @endcode
 */
#ifndef RAMPG_CONF_H_
#define RAMPG_CONF_H_

/* ================ CONFIGURATION =========================================== */

#ifndef RAMPG_LIMIT_MIN
/**
 * @def RAMPG_LIMIT_MIN
 * @brief Default minimum output clamp.
 */
#define RAMPG_LIMIT_MIN (-1000000.0f)
#endif

#ifndef RAMPG_LIMIT_MAX
/**
 * @def RAMPG_LIMIT_MAX
 * @brief Default maximum output clamp.
 */
#define RAMPG_LIMIT_MAX (1000000.0f)
#endif

#ifndef RAMPG_DEFAULT_RATE
/**
 * @def RAMPG_DEFAULT_RATE
 * @brief Default ramp rate in units per second.
 */
#define RAMPG_DEFAULT_RATE (100.0f)
#endif

#endif /* RAMPG_CONF_H_ */
