/*
 * @file test_rampg.c
 * @brief Unit tests for rampg.
 */

#include "rampg.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_ASSERT(expr)                                                      \
        do {                                                                   \
                if (!(expr)) {                                                 \
                        fprintf(stderr, "FAIL  %s:%d  %s\n", __FILE__,         \
                                __LINE__, #expr);                              \
                        exit(EXIT_FAILURE);                                    \
                }                                                              \
        } while (0)

#define TEST_PASS(name) fprintf(stdout, "PASS  %s\n", (name))

#define TEST_CASE(name)                                                        \
        static void name(void);                                                \
        static void name(void)

#define FLOAT_EQ(a, b)        (fabsf((a) - (b)) < 1e-6f)
#define FLOAT_NEAR(a, b, tol) (fabsf((a) - (b)) < (tol))

/* ================ Basic =====================================================
 */

TEST_CASE(test_init_sets_value_and_target)
{
        rampg_t r;
        rampg_init(&r, 42.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 42.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_init_defaults)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        TEST_ASSERT(FLOAT_EQ(r.rise_rate, 100.0f));
        TEST_ASSERT(FLOAT_EQ(r.fall_rate, 100.0f));
        TEST_ASSERT(FLOAT_EQ(r.limit_min, RAMPG_LIMIT_MIN));
        TEST_ASSERT(FLOAT_EQ(r.limit_max, RAMPG_LIMIT_MAX));
}

TEST_CASE(test_ramp_up_basic)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 50.0f);

        /* 100 units/s * 0.1s = 10 units per step */
        float v = rampg_update(&r, 0.1f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
        TEST_ASSERT(!rampg_at_target(&r));
}

TEST_CASE(test_ramp_down_basic)
{
        rampg_t r;
        rampg_init(&r, 100.0f);
        rampg_set_rate(&r, 200.0f);
        rampg_set_target(&r, 0.0f);

        float v = rampg_update(&r, 0.1f);
        TEST_ASSERT(FLOAT_EQ(v, 80.0f));
}

TEST_CASE(test_ramp_reaches_target_exactly)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 10.0f);

        /* step = 100 * 0.5 = 50, but diff is only 10 — should snap to target */
        float v = rampg_update(&r, 0.5f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_ramp_does_not_overshoot)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 1000.0f);
        rampg_set_target(&r, 5.0f);

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 5.0f));
}

/* ================ Asymmetric rates ==========================================
 */

TEST_CASE(test_asymmetric_rates)
{
        rampg_t r;
        rampg_init(&r, 50.0f);
        rampg_set_rates(&r, 10.0f, 100.0f); /* slow up, fast down */

        /* Ramp up: 10 units/s */
        rampg_set_target(&r, 100.0f);
        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 60.0f));

        /* Ramp down: 100 units/s */
        rampg_set_target(&r, 0.0f);
        v = rampg_update(&r, 0.5f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
}

TEST_CASE(test_set_rate_overwrites_both)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rates(&r, 10.0f, 20.0f);
        rampg_set_rate(&r, 50.0f);
        TEST_ASSERT(FLOAT_EQ(r.rise_rate, 50.0f));
        TEST_ASSERT(FLOAT_EQ(r.fall_rate, 50.0f));
}

/* ================ Output clamping ===========================================
 */

TEST_CASE(test_output_clamping_upper)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, -10.0f, 10.0f);
        rampg_set_rate(&r, 1000.0f);
        rampg_set_target(&r, 100.0f);

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
}

TEST_CASE(test_output_clamping_lower)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, -10.0f, 10.0f);
        rampg_set_rate(&r, 1000.0f);
        rampg_set_target(&r, -100.0f);

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, -10.0f));
}

TEST_CASE(test_clamp_applied_after_init)
{
        rampg_t r;
        rampg_init(&r, 500.0f);
        rampg_set_limits(&r, 0.0f, 100.0f);

        /* set_limits should have clamped value immediately */
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 100.0f));
}

/* ================ Target beyond clamp (bug fix) =============================
 */

TEST_CASE(test_target_above_upper_limit)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 100.0f);
        rampg_set_rate(&r, 1000.0f);
        rampg_set_target(&r, 200.0f);

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 100.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_target_below_lower_limit)
{
        rampg_t r;
        rampg_init(&r, 50.0f);
        rampg_set_limits(&r, 0.0f, 100.0f);
        rampg_set_rate(&r, 1000.0f);
        rampg_set_target(&r, -50.0f);

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 0.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_target_beyond_limit_preserves_stored_target)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 100.0f);
        rampg_set_rate(&r, 1000.0f);
        rampg_set_target(&r, 200.0f);
        rampg_update(&r, 1.0f);

        /* Value is at 100 (clamped effective target) */
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 100.0f));
        TEST_ASSERT(rampg_at_target(&r));

        /* Widen limits — original target 200 is now reachable */
        rampg_set_limits(&r, 0.0f, 300.0f);
        TEST_ASSERT(!rampg_at_target(&r));

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 200.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

/* ================ Reset =====================================================
 */

TEST_CASE(test_reset_snaps_value)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_target(&r, 100.0f);
        rampg_update(&r, 0.01f); /* partially ramped */
        TEST_ASSERT(!rampg_at_target(&r));

        rampg_reset(&r, 77.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 77.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_reset_outside_limits_clamps_value)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 100.0f);

        rampg_reset(&r, 500.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 100.0f));

        /* Stored target is 500, so at_target compares against clamped 100 */
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_reset_below_limits_clamps_value)
{
        rampg_t r;
        rampg_init(&r, 50.0f);
        rampg_set_limits(&r, 10.0f, 100.0f);

        rampg_reset(&r, -20.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 10.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

/* ================ get / at_target ===========================================
 */

TEST_CASE(test_get_does_not_advance)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_target(&r, 100.0f);

        float v1 = rampg_get(&r);
        float v2 = rampg_get(&r);
        TEST_ASSERT(FLOAT_EQ(v1, v2));
        TEST_ASSERT(FLOAT_EQ(v1, 0.0f));
}

TEST_CASE(test_already_at_target)
{
        rampg_t r;
        rampg_init(&r, 10.0f);
        TEST_ASSERT(rampg_at_target(&r));

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

/* ================ Edge cases ================================================
 */

TEST_CASE(test_zero_dt_no_change)
{
        rampg_t r;
        rampg_init(&r, 50.0f);
        rampg_set_target(&r, 100.0f);

        float v = rampg_update(&r, 0.0f);
        TEST_ASSERT(FLOAT_EQ(v, 50.0f));
}

TEST_CASE(test_negative_initial_value)
{
        rampg_t r;
        rampg_init(&r, -50.0f);
        rampg_set_rate(&r, 25.0f);
        rampg_set_target(&r, 0.0f);

        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, -25.0f));
}

TEST_CASE(test_negative_to_negative_ramp)
{
        rampg_t r;
        rampg_init(&r, -100.0f);
        rampg_set_rate(&r, 50.0f);
        rampg_set_target(&r, -20.0f);

        /* Ramping up from -100 toward -20 at 50/s for 1s → -50 */
        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, -50.0f));

        /* Continue for 1s more → reaches -20 exactly (step=50 > diff=30) */
        v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, -20.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_multi_step_ramp)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 10.0f);
        rampg_set_target(&r, 100.0f);

        for (int i = 0; i < 10; i++) {
                rampg_update(&r, 1.0f);
        }

        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 100.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_change_target_mid_ramp)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 100.0f);

        rampg_update(&r, 0.5f); /* value = 50 */
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 50.0f));

        rampg_set_target(&r, 30.0f); /* reverse direction */
        float v = rampg_update(&r, 0.1f);
        TEST_ASSERT(FLOAT_EQ(v, 40.0f)); /* 50 - 100*0.1 = 40 */
}

TEST_CASE(test_change_rate_mid_ramp)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 100.0f);

        rampg_update(&r, 0.5f); /* value = 50 at 100/s */
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 50.0f));

        rampg_set_rate(&r, 10.0f); /* slow down */
        float v = rampg_update(&r, 1.0f);
        TEST_ASSERT(FLOAT_EQ(v, 60.0f)); /* 50 + 10*1 = 60 */
}

TEST_CASE(test_repeated_set_target_same_value)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 50.0f);
        rampg_set_target(&r, 50.0f);
        rampg_set_target(&r, 50.0f);

        float v = rampg_update(&r, 0.1f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
}

TEST_CASE(test_limits_narrowed_mid_ramp)
{
        rampg_t r;
        rampg_init(&r, 50.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 200.0f);

        rampg_update(&r, 0.5f); /* value = 100 */
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 100.0f));

        /* Narrow limits — value should clamp immediately */
        rampg_set_limits(&r, 0.0f, 80.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 80.0f));

        /* Target 200 is beyond limit 80 — at_target should be true at clamp */
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_very_large_dt)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 1.0f);
        rampg_set_target(&r, 10.0f);

        /* dt large enough to overshoot — should snap to target */
        float v = rampg_update(&r, 1000.0f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_small_dt_accumulation)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 10.0f);

        /* 10000 steps of 0.001s each = 10s total, rate 100/s → 1000 units
         * moved. Target is only 10, so should snap to target well before the
         * end. */
        for (int i = 0; i < 10000; i++) {
                rampg_update(&r, 0.001f);
        }

        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 10.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_small_dt_precision)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 10.0f);
        rampg_set_limits(&r, 0.0f, 1000.0f);
        rampg_set_target(&r, 1000.0f);

        /* Simulate 1 kHz loop for 100 seconds — value should reach 1000.
         * 100000 increments of 0.01 each. Check final value is close. */
        for (int i = 0; i < 100000; i++) {
                rampg_update(&r, 0.001f);
        }

        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1000.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_linear_subresolution_step_does_not_jump_to_target)
{
        const float start = 1e9f;
        const float target = 2e9f;
        const float rate = 1.0f;
        const float dt = 1e-3f;
        const float step = rate * dt;
        rampg_t r;

        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 1e10f);
        rampg_reset(&r, start);
        rampg_set_rate(&r, rate);
        rampg_set_target(&r, target);

        /* step is far below the ULP of start (~64). The first update holds
         * instead of replacing the requested movement with a target jump. */
        TEST_ASSERT(rampg_update(&r, dt) == start);
        TEST_ASSERT(!rampg_at_target(&r));
        TEST_ASSERT(rampg_get_rate(&r) == rate);
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_MOVING);

        int ticks = 1;
        while ((rampg_get(&r) == start) && (ticks < 100000)) {
                rampg_update(&r, dt);
                ticks++;
        }

        double paid = (double)ticks * (double)step;
        double moved = (double)rampg_get(&r) - (double)start;
        TEST_ASSERT(ticks < 100000);
        TEST_ASSERT(rampg_get(&r) == nextafterf(start, target));
        TEST_ASSERT(moved <= paid);
        TEST_ASSERT(!rampg_at_target(&r));
}

TEST_CASE(test_linear_accumulates_subresolution_steps)
{
        const float start = 1000.0f;
        const float rate = 0.01f;
        const float dt = 0.001f;
        const float target = 1001.0f;
        const float next = nextafterf(start, target);
        rampg_t r;

        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_reset(&r, start);
        rampg_set_rate(&r, rate);
        rampg_set_target(&r, target);

        int ticks = 0;
        while ((rampg_get(&r) == start) && (ticks < 100)) {
                rampg_update(&r, dt);
                ticks++;
        }

        float budget = (float)ticks * rate * dt;
        TEST_ASSERT(ticks > 1 && ticks < 100);
        TEST_ASSERT(rampg_get(&r) == next);
        TEST_ASSERT(budget >= (next - start));
        TEST_ASSERT(!rampg_at_target(&r));
}

TEST_CASE(test_linear_discards_subresolution_budget_on_reversal)
{
        const float start = 1000.0f;
        const float rate = 0.01f;
        const float dt = 0.001f;
        rampg_t r;

        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_reset(&r, start);
        rampg_set_rate(&r, rate);
        rampg_set_target(&r, 1001.0f);

        for (int i = 0; i < 3; i++) {
                TEST_ASSERT(rampg_update(&r, dt) == start);
        }

        rampg_set_target(&r, 999.0f);
        for (int i = 0; i < 6; i++) {
                TEST_ASSERT(rampg_update(&r, dt) == start);
        }
        float lower = nextafterf(start, 999.0f);
        TEST_ASSERT(rampg_update(&r, dt) == lower);

        rampg_set_target(&r, 1001.0f);
        for (int i = 0; i < 6; i++) {
                TEST_ASSERT(rampg_update(&r, dt) == lower);
        }
        TEST_ASSERT(rampg_update(&r, dt) == start);
}

TEST_CASE(test_linear_subresolution_budget_preserves_average_rate)
{
        const float start = 1000.0f;
        const float rate = 0.01f;
        const float dt = 0.001f;
        const float target = 1001.0f;
        const int updates = 10000;
        const float step = rate * dt;
        double budget = 0.0;
        rampg_t r;

        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_reset(&r, start);
        rampg_set_rate(&r, rate);
        rampg_set_target(&r, target);

        for (int i = 0; i < updates; i++) {
                rampg_update(&r, dt);
                budget += (double)step;

                float value = rampg_get(&r);
                double moved = (double)value - (double)start;
                float quantum = nextafterf(value, target) - value;
                TEST_ASSERT(moved <= budget);
                TEST_ASSERT((budget - moved) <= (double)quantum);
        }
        TEST_ASSERT(!rampg_at_target(&r));
}

TEST_CASE(test_linear_subresolution_budget_lands_exactly)
{
        const float start = 1000.0f;
        const float rate = 0.01f;
        const float dt = 0.001f;
        const float target = nextafterf(start, INFINITY);
        rampg_t r;

        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_reset(&r, start);
        rampg_set_rate(&r, rate);
        rampg_set_target(&r, target);

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 100)) {
                rampg_update(&r, dt);
                ticks++;
        }

        TEST_ASSERT(ticks > 1 && ticks < 100);
        TEST_ASSERT(rampg_get(&r) == target);
        TEST_ASSERT(rampg_get_rate(&r) == 0.0f);
}

TEST_CASE(test_linear_rate_product_underflow_holds)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 1.0f);
        rampg_set_rate(&r, FLT_MIN);
        rampg_set_target(&r, 1.0f);

        TEST_ASSERT(rampg_update(&r, FLT_MIN) == 0.0f);
        TEST_ASSERT(!rampg_at_target(&r));
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_MOVING);
}

TEST_CASE(test_linear_step_below_budget_resolution_holds)
{
        /*
         * The gap between representable floats at 1e9 is 64. A step of
         * 2^-21 can only accumulate to about step * 2^24 = 8 before the
         * budget's own resolution stops it growing, so it never pays for
         * a representable step. The documented behaviour is to hold the
         * output and keep reporting a move, never to snap to the target.
         */
        const float start = 1e9f;
        const float dt = 1.0f;
        rampg_t r;

        rampg_init(&r, 0.0f);
        rampg_set_limits(&r, 0.0f, 1e10f);
        rampg_reset(&r, start);
        rampg_set_rate(&r, ldexpf(1.0f, -21));
        rampg_set_target(&r, 2e9f);

        for (int i = 0; i < 1000; i++) {
                TEST_ASSERT(rampg_update(&r, dt) == start);
        }
        TEST_ASSERT(!rampg_at_target(&r));
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_MOVING);
}

TEST_CASE(test_sc_precharge_scenario)
{
        /* Simulate DC bus precharge: 0 → 400V at 50V/s, limits [0, 800] */
        rampg_t vbus;
        rampg_init(&vbus, 0.0f);
        rampg_set_rates(&vbus, 50.0f, 200.0f); /* slow up, fast trip down */
        rampg_set_limits(&vbus, 0.0f, 800.0f);
        rampg_set_target(&vbus, 400.0f);

        /* 10 ms tick, 8 seconds to reach 400V at 50V/s */
        int ticks = 0;
        while (!rampg_at_target(&vbus)) {
                rampg_update(&vbus, 0.01f);
                ticks++;
                /* Safety: don't loop forever */
                TEST_ASSERT(ticks < 10000);
        }

        TEST_ASSERT(FLOAT_EQ(rampg_get(&vbus), 400.0f));
        /* 400V / 50(V/s) = 8s = 800 ticks at 10ms */
        TEST_ASSERT(ticks == 800);

        /* Fault: trip down to 0V — should be 4x faster */
        rampg_set_target(&vbus, 0.0f);
        ticks = 0;
        while (!rampg_at_target(&vbus)) {
                rampg_update(&vbus, 0.01f);
                ticks++;
                TEST_ASSERT(ticks < 10000);
        }

        TEST_ASSERT(FLOAT_EQ(rampg_get(&vbus), 0.0f));
        /* 400V / 200(V/s) = 2s = 200 ticks at 10ms */
        TEST_ASSERT(ticks == 200);
}

TEST_CASE(test_sc_frequency_ramp_scenario)
{
        /* Simulate output frequency ramp: 0 → 50Hz at 5Hz/s */
        rampg_t freq;
        rampg_init(&freq, 0.0f);
        rampg_set_rate(&freq, 5.0f);
        rampg_set_limits(&freq, 0.0f, 60.0f);
        rampg_set_target(&freq, 50.0f);

        /* 10 ms tick, 10 seconds to reach 50Hz */
        int ticks = 0;
        while (!rampg_at_target(&freq)) {
                rampg_update(&freq, 0.01f);
                ticks++;
                TEST_ASSERT(ticks < 20000);
        }

        TEST_ASSERT(FLOAT_EQ(rampg_get(&freq), 50.0f));
        /* 50Hz / 5(Hz/s) = 10s = 1000 ticks; allow ±1 for float accumulation */
        TEST_ASSERT(ticks >= 999 && ticks <= 1001);
}

/* ================ S-curve helpers ===========================================
 */

/*
 * Closed-form duration of an S-curve move of `dist` at `rate` and `accel`.
 * Trapezoidal when the move is long enough to reach the rate limit,
 * triangular otherwise.
 */
static float
scurve_duration(float dist, float rate, float accel)
{
        if ((rate * rate / accel) <= dist) {
                return (dist / rate) + (rate / accel);
        }
        return 2.0f * sqrtf(dist / accel);
}

/* Drive a ramp to its target, asserting the invariants on every update. */
static int
drive_checked(rampg_t *r, float dt, int max_ticks)
{
        float rate_limit =
            (r->rise_rate > r->fall_rate) ? r->rise_rate : r->fall_rate;
        float accel_limit =
            (r->rise_accel > r->fall_accel) ? r->rise_accel : r->fall_accel;
        float bound = accel_limit * dt;
        float prev = rampg_get_rate(r);
        int ticks = 0;

        while (!rampg_at_target(r) && (ticks < max_ticks)) {
                rampg_update(r, dt);
                float now = rampg_get_rate(r);

                if (!rampg_at_target(r)) {
                        /* The rate changes by at most one acceleration step,
                         * allowing for float rounding near the envelope. */
                        TEST_ASSERT(fabsf(now - prev)
                                    <= (bound * 2.0f) + 1e-6f);
                }
                /* The rate never exceeds the configured limit. */
                TEST_ASSERT(fabsf(now) <= rate_limit + 1e-4f);
                /* The output stays inside the limits. */
                TEST_ASSERT(rampg_get(r) >= r->limit_min - 1e-4f);
                TEST_ASSERT(rampg_get(r) <= r->limit_max + 1e-4f);

                prev = now;
                ticks++;
        }
        TEST_ASSERT(ticks < max_ticks);
        return ticks;
}

/* ================ S-curve ===================================================
 */

TEST_CASE(test_scurve_reaches_target_exactly)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_target(&r, 100.0f);

        drive_checked(&r, 0.001f, 100000);

        TEST_ASSERT(rampg_at_target(&r));
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 100.0f));
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
}

TEST_CASE(test_scurve_trapezoidal_duration)
{
        /* 1000 units at 100 u/s and 1000 u/s^2 reaches the rate limit:
         * t = dist/rate + rate/accel = 10 + 0.1 = 10.1 s. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        int ticks = drive_checked(&r, 0.001f, 100000);
        float expected = scurve_duration(1000.0f, 100.0f, 1000.0f);

        TEST_ASSERT(FLOAT_NEAR(expected, 10.1f, 1e-4f));
        TEST_ASSERT(FLOAT_NEAR((float)ticks * 0.001f, expected, 0.05f));
}

TEST_CASE(test_scurve_triangular_duration)
{
        /* 1 unit at 100 u/s and 1000 u/s^2 never reaches the rate limit:
         * t = 2*sqrt(dist/accel) = 2*sqrt(0.001) = 0.0632 s. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_target(&r, 1.0f);

        int ticks = drive_checked(&r, 0.0001f, 100000);
        float expected = scurve_duration(1.0f, 100.0f, 1000.0f);

        TEST_ASSERT(FLOAT_NEAR(expected, 0.06325f, 1e-4f));
        TEST_ASSERT(FLOAT_NEAR((float)ticks * 0.0001f, expected, 0.005f));
}

TEST_CASE(test_scurve_reaches_configured_peak_rate)
{
        /* A move long enough to cruise must reach the configured rate and
         * must never exceed it. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        float peak = 0.0f;
        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 100000)) {
                rampg_update(&r, 0.001f);
                float rate = rampg_get_rate(&r);
                TEST_ASSERT(rate <= 100.0f + 1e-4f);
                if (rate > peak) {
                        peak = rate;
                }
                ticks++;
        }
        TEST_ASSERT(FLOAT_NEAR(peak, 100.0f, 1e-3f));
}

TEST_CASE(test_scurve_eases_in_and_out)
{
        /* The first and last updates of a move run well below the configured
         * rate: the profile eases in and out rather than stepping. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 100.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        rampg_update(&r, 0.001f);
        TEST_ASSERT(rampg_get_rate(&r) < 1.0f);

        float last = 0.0f;
        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 200000)) {
                last = rampg_get_rate(&r);
                rampg_update(&r, 0.001f);
                ticks++;
        }
        TEST_ASSERT(rampg_at_target(&r));
        /* The rate on the update before arrival is a small multiple of one
         * acceleration step, not a hard stop from the cruise rate. */
        TEST_ASSERT(last < 100.0f * 0.001f * 4.0f);
}

TEST_CASE(test_scurve_no_overshoot)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 1000.0f);
        rampg_set_accel(&r, 10000.0f);
        rampg_set_limits(&r, 0.0f, 5.0f);
        rampg_set_target(&r, 5.0f);

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 100000)) {
                rampg_update(&r, 0.001f);
                TEST_ASSERT(rampg_get(&r) >= 0.0f);
                TEST_ASSERT(rampg_get(&r) <= 5.0f);
                ticks++;
        }
        TEST_ASSERT(rampg_at_target(&r));
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 5.0f));
}

TEST_CASE(test_scurve_asymmetric_rates)
{
        /* The fall leg at twice the rise rate takes correspondingly less
         * time, once the fixed accel ramp is accounted for. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rates(&r, 100.0f, 200.0f);
        rampg_set_accel(&r, 10000.0f);
        rampg_set_limits(&r, 0.0f, 1000.0f);

        rampg_set_target(&r, 1000.0f);
        int up = drive_checked(&r, 0.0001f, 1000000);

        rampg_set_target(&r, 0.0f);
        int down = drive_checked(&r, 0.0001f, 1000000);

        float t_up = scurve_duration(1000.0f, 100.0f, 10000.0f);
        float t_down = scurve_duration(1000.0f, 200.0f, 10000.0f);

        TEST_ASSERT(FLOAT_NEAR((float)up * 0.0001f, t_up, 0.01f));
        TEST_ASSERT(FLOAT_NEAR((float)down * 0.0001f, t_down, 0.01f));
        TEST_ASSERT(down < up);
}

TEST_CASE(test_scurve_asymmetric_accels)
{
        /*
         * The acceleration limit is selected by direction, like the rate, so
         * each leg keeps its own shape. The fall leg here eases four times
         * faster than the rise leg.
         */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accels(&r, 250.0f, 1000.0f);
        rampg_set_limits(&r, 0.0f, 1000.0f);

        rampg_set_target(&r, 500.0f);
        int up = drive_checked(&r, 0.0001f, 1000000);

        rampg_set_target(&r, 0.0f);
        int down = drive_checked(&r, 0.0001f, 1000000);

        /* Both legs cover the same distance at the same rate limit, so the
         * difference is exactly the two acceleration overheads. */
        float t_up = scurve_duration(500.0f, 100.0f, 250.0f);
        float t_down = scurve_duration(500.0f, 100.0f, 1000.0f);

        TEST_ASSERT(FLOAT_NEAR((float)up * 0.0001f, t_up, 0.01f));
        TEST_ASSERT(FLOAT_NEAR((float)down * 0.0001f, t_down, 0.01f));
        TEST_ASSERT(down < up);
}

TEST_CASE(test_scurve_asymmetric_accel_bound_per_direction)
{
        /* Each leg must respect its own acceleration limit, not the other. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accels(&r, 200.0f, 4000.0f);
        rampg_set_limits(&r, 0.0f, 1000.0f);

        rampg_set_target(&r, 500.0f);
        float prev = 0.0f;
        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 200000)) {
                rampg_update(&r, 0.001f);
                float now = rampg_get_rate(&r);
                if (!rampg_at_target(&r)) {
                        /* Rising: bounded by rise_accel, well under
                         * fall_accel. */
                        TEST_ASSERT(fabsf(now - prev) <= 0.4f + 1e-4f);
                }
                prev = now;
                ticks++;
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 500.0f));
}

TEST_CASE(test_scurve_set_accel_sets_both)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_accels(&r, 1.0f, 2.0f);
        rampg_set_accel(&r, 42.0f);
        TEST_ASSERT(FLOAT_EQ(r.rise_accel, 42.0f));
        TEST_ASSERT(FLOAT_EQ(r.fall_accel, 42.0f));
}

TEST_CASE(test_scurve_precharge_asymmetric_scenario)
{
        /*
         * The motivating case: a DC bus eased up slowly, then tripped down
         * hard on a fault. A single symmetric acceleration limit cannot serve
         * both, so assert each leg against its own closed form.
         */
        rampg_t vbus;
        rampg_init(&vbus, 0.0f);
        rampg_set_shape(&vbus, RAMPG_SHAPE_SCURVE);
        rampg_set_rates(&vbus, 50.0f, 2000.0f);
        rampg_set_accels(&vbus, 100.0f, 40000.0f);
        rampg_set_limits(&vbus, 0.0f, 800.0f);

        rampg_set_target(&vbus, 400.0f);
        int up = drive_checked(&vbus, 0.0001f, 2000000);

        rampg_set_target(&vbus, 0.0f);
        int down = drive_checked(&vbus, 0.0001f, 2000000);

        float t_up = scurve_duration(400.0f, 50.0f, 100.0f);
        float t_down = scurve_duration(400.0f, 2000.0f, 40000.0f);

        TEST_ASSERT(FLOAT_NEAR((float)up * 0.0001f, t_up, 0.02f));
        TEST_ASSERT(FLOAT_NEAR((float)down * 0.0001f, t_down, 0.02f));
        /* The rise eases over 8.5 s; the trip completes inside 0.3 s. */
        TEST_ASSERT((float)up * 0.0001f > 8.0f);
        TEST_ASSERT((float)down * 0.0001f < 0.3f);
}

TEST_CASE(test_scurve_retarget_carries_the_rate)
{
        /* The defect this profile exists to avoid: a mid-move retarget must
         * not drop the output rate to zero. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 500; i++) {
                rampg_update(&r, 0.001f);
        }
        float before = rampg_get_rate(&r);
        TEST_ASSERT(FLOAT_NEAR(before, 100.0f, 1e-3f));

        rampg_set_target(&r, 1200.0f);

        /* The rate is unchanged by the setter, and the next update moves it
         * by no more than one acceleration step. */
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), before));
        rampg_update(&r, 0.001f);
        TEST_ASSERT(fabsf(rampg_get_rate(&r) - before) <= 1.0f + 1e-4f);

        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1200.0f));
}

TEST_CASE(test_scurve_progresses_under_a_moving_setpoint)
{
        /*
         * A setpoint re-issued with a small change on every update must not
         * stall the ramp. Drive for the undisturbed duration of the move at a
         * range of re-issue cadences and require the output to have arrived.
         *
         * The profile this replaced re-planned from rest on every target
         * change and so never left the flat start of its curve: under the
         * 1 kHz cadence below it covered under one unit in sixty seconds.
         */
        static const int cadences[] = {1, 2, 10, 100, 500};
        float expected = scurve_duration(1000.0f, 100.0f, 1000.0f);
        int budget = (int)((expected / 0.001f) + 200.0f);

        for (unsigned c = 0; c < 5u; c++) {
                rampg_t r;
                rampg_init(&r, 0.0f);
                rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
                rampg_set_rate(&r, 100.0f);
                rampg_set_accel(&r, 1000.0f);
                rampg_set_limits(&r, 0.0f, 2000.0f);

                int ticks = 0;
                while (ticks < budget) {
                        int phase = (ticks / cadences[c]) % 2;
                        rampg_set_target(&r, phase ? 1000.0f : 1000.01f);
                        rampg_update(&r, 0.001f);
                        TEST_ASSERT(fabsf(rampg_get_rate(&r))
                                    <= 100.0f + 1e-4f);
                        ticks++;
                }

                /* Arrived, within the dither band, in about the normal time. */
                TEST_ASSERT(FLOAT_NEAR(rampg_get(&r), 1000.0f, 0.05f));
        }
}

TEST_CASE(test_scurve_reversal_crosses_zero_smoothly)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, -1000.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 500; i++) {
                rampg_update(&r, 0.001f);
        }
        TEST_ASSERT(rampg_get_rate(&r) > 90.0f);

        /* Reverse to a target far behind the current value. */
        rampg_set_target(&r, -500.0f);

        float prev = rampg_get_rate(&r);
        int ticks = 0;
        int seen_negative = 0;
        while (!rampg_at_target(&r) && (ticks < 200000)) {
                rampg_update(&r, 0.001f);
                float now = rampg_get_rate(&r);
                if (!rampg_at_target(&r)) {
                        TEST_ASSERT(fabsf(now - prev) <= 2.0f + 1e-4f);
                }
                if (now < -1.0f) {
                        seen_negative = 1;
                }
                prev = now;
                ticks++;
        }
        TEST_ASSERT(seen_negative);
        TEST_ASSERT(rampg_at_target(&r));
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), -500.0f));
}

TEST_CASE(test_scurve_rate_change_mid_move)
{
        /*
         * Lowering the rate limit below the rate the ramp is already running
         * at cannot take effect instantly: that would be an unbounded
         * deceleration. The ramp eases down to the new limit at the
         * acceleration limit instead, and the move still completes.
         */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 500; i++) {
                rampg_update(&r, 0.001f);
        }
        float before = rampg_get_rate(&r);
        TEST_ASSERT(FLOAT_NEAR(before, 100.0f, 1e-3f));

        rampg_set_rate(&r, 50.0f);
        /* The setter does not move the output rate. */
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), before));

        /* It eases down to the new limit within one acceleration step per
         * update, and takes about (100 - 50) / 1000 s to get there. */
        float prev = before;
        int ticks = 0;
        while ((rampg_get_rate(&r) > 50.0f) && (ticks < 1000)) {
                rampg_update(&r, 0.001f);
                float now = rampg_get_rate(&r);
                TEST_ASSERT(fabsf(now - prev) <= 1.0f + 1e-4f);
                prev = now;
                ticks++;
        }
        TEST_ASSERT(ticks >= 45 && ticks <= 55);

        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1000.0f));
}

TEST_CASE(test_scurve_accel_change_mid_move)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 500; i++) {
                rampg_update(&r, 0.001f);
        }
        rampg_set_accel(&r, 200.0f);

        drive_checked(&r, 0.001f, 200000);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1000.0f));
}

TEST_CASE(test_scurve_limit_narrowed_mid_move)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 500; i++) {
                rampg_update(&r, 0.001f);
        }
        TEST_ASSERT(rampg_get(&r) > 20.0f);

        /* Narrowing the limits below the current value displaces the output,
         * so the rate is reset rather than carried. */
        rampg_set_limits(&r, 0.0f, 10.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 10.0f));
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_scurve_limit_widened_mid_move)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 50.0f);
        rampg_set_target(&r, 200.0f);

        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 50.0f));

        /* Widening the limits makes the original target reachable again. */
        rampg_set_limits(&r, 0.0f, 500.0f);
        TEST_ASSERT(!rampg_at_target(&r));

        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 200.0f));
}

TEST_CASE(test_scurve_target_beyond_limit)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 30.0f);
        rampg_set_target(&r, 200.0f);

        drive_checked(&r, 0.001f, 100000);

        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 30.0f));
        TEST_ASSERT(rampg_at_target(&r));
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_AT_TARGET);
}

TEST_CASE(test_scurve_zero_distance_move)
{
        rampg_t r;
        rampg_init(&r, 42.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_target(&r, 42.0f);

        TEST_ASSERT(rampg_at_target(&r));
        TEST_ASSERT(FLOAT_EQ(rampg_update(&r, 0.01f), 42.0f));
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
}

TEST_CASE(test_scurve_zero_dt_no_progress)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_target(&r, 100.0f);

        for (int i = 0; i < 10; i++) {
                TEST_ASSERT(FLOAT_EQ(rampg_update(&r, 0.0f), 0.0f));
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
}

TEST_CASE(test_scurve_very_large_dt)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 1.0f);
        rampg_set_accel(&r, 10.0f);
        rampg_set_target(&r, 10.0f);

        /* A single step larger than the whole move snaps without overshoot. */
        float v = rampg_update(&r, 1000.0f);
        TEST_ASSERT(FLOAT_EQ(v, 10.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

TEST_CASE(test_scurve_long_slow_move_completes)
{
        /* The old profile accumulated elapsed time in a float and stalled on
         * long moves. This profile has no time accumulator; a move two orders
         * of magnitude longer than the tick still completes. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 50.0f);
        rampg_set_accel(&r, 200.0f);
        rampg_set_limits(&r, 0.0f, 1000.0f);
        rampg_set_target(&r, 500.0f);

        int ticks = drive_checked(&r, 0.0001f, 500000);
        float expected = scurve_duration(500.0f, 50.0f, 200.0f);

        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 500.0f));
        TEST_ASSERT(FLOAT_NEAR((float)ticks * 0.0001f, expected, 0.05f));
}

TEST_CASE(test_scurve_shape_change_from_linear_carries_the_rate)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 100; i++) {
                rampg_update(&r, 0.001f);
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 100.0f));

        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        /* The S-curve starts at the rate the linear ramp was running at. */
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 100.0f));

        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1000.0f));
}

TEST_CASE(test_scurve_shape_change_to_linear_mid_move)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 50; i++) {
                rampg_update(&r, 0.001f);
        }
        float mid = rampg_get(&r);
        TEST_ASSERT(mid > 0.0f);

        float rate_before = rampg_get_rate(&r);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        TEST_ASSERT(rampg_get_rate(&r) == rate_before);

        rampg_set_shape(&r, RAMPG_SHAPE_LINEAR);
        /* LINEAR resumes at the full configured rate. */
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 100.0f));

        float first = rampg_update(&r, 0.001f);
        TEST_ASSERT(FLOAT_NEAR(first - mid, 0.1f, 1e-4f));

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 100000)) {
                rampg_update(&r, 0.001f);
                ticks++;
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1000.0f));
}

TEST_CASE(test_scurve_reset_clears_the_rate)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 500; i++) {
                rampg_update(&r, 0.001f);
        }
        TEST_ASSERT(rampg_get_rate(&r) > 50.0f);

        rampg_reset(&r, 42.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 42.0f));
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
        TEST_ASSERT(rampg_at_target(&r));
}

/* ================ Containment ===============================================
 *
 * rampg_update() is total: a value it cannot act on holds the output rather
 * than corrupting the state, and the ramp recovers on the next valid input.
 */

/* Put a ramp mid-move in the requested shape. */
static void
containment_setup(rampg_t *r, rampg_shape_t shape)
{
        rampg_init(r, 0.0f);
        rampg_set_shape(r, shape);
        rampg_set_rate(r, 100.0f);
        rampg_set_accel(r, 1000.0f);
        rampg_set_limits(r, 0.0f, 1000.0f);
        rampg_set_target(r, 500.0f);
        rampg_update(r, 0.05f);
        TEST_ASSERT(rampg_get(r) > 0.0f);
}

/* Assert the ramp still completes a fresh move after a bad input. */
static void
containment_recovers(rampg_t *r)
{
        rampg_set_rate(r, 100.0f);
        rampg_set_accel(r, 1000.0f);
        rampg_set_target(r, 50.0f);

        int ticks = 0;
        while (!rampg_at_target(r) && (ticks < 200000)) {
                rampg_update(r, 0.001f);
                ticks++;
        }
        TEST_ASSERT(rampg_at_target(r));
        TEST_ASSERT(FLOAT_EQ(rampg_get(r), 50.0f));
}

TEST_CASE(test_containment_bad_dt)
{
        const float bad[] = {NAN, INFINITY, -INFINITY, -0.01f};
        const rampg_shape_t shapes[] = {RAMPG_SHAPE_LINEAR, RAMPG_SHAPE_SCURVE};

        for (unsigned s = 0; s < 2u; s++) {
                for (unsigned i = 0; i < 4u; i++) {
                        rampg_t r;
                        containment_setup(&r, shapes[s]);
                        float held = rampg_get(&r);

                        TEST_ASSERT(FLOAT_EQ(rampg_update(&r, bad[i]), held));
                        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), held));
                        containment_recovers(&r);
                }
        }
}

TEST_CASE(test_containment_bad_target)
{
        const float bad[] = {NAN, INFINITY, -INFINITY};
        const rampg_shape_t shapes[] = {RAMPG_SHAPE_LINEAR, RAMPG_SHAPE_SCURVE};

        for (unsigned s = 0; s < 2u; s++) {
                for (unsigned i = 0; i < 3u; i++) {
                        rampg_t r;
                        containment_setup(&r, shapes[s]);
                        /* Limits must be infinite too, otherwise the target
                         * clamps to a finite limit and the move is valid. */
                        rampg_set_limits(&r, -INFINITY, INFINITY);
                        float held = rampg_get(&r);

                        rampg_set_target(&r, bad[i]);
                        TEST_ASSERT(FLOAT_EQ(rampg_update(&r, 0.01f), held));

                        rampg_set_limits(&r, 0.0f, 1000.0f);
                        containment_recovers(&r);
                }
        }
}

TEST_CASE(test_containment_infinite_target_clamps_to_limit)
{
        /* An infinite target inside finite limits is not an error: it
         * resolves to the limit and the ramp moves there normally. */
        rampg_t r;
        containment_setup(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_target(&r, INFINITY);

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 200000)) {
                rampg_update(&r, 0.001f);
                ticks++;
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1000.0f));
}

TEST_CASE(test_containment_bad_rate)
{
        const float bad[] = {0.0f, -100.0f, NAN};
        const rampg_shape_t shapes[] = {RAMPG_SHAPE_LINEAR, RAMPG_SHAPE_SCURVE};

        for (unsigned s = 0; s < 2u; s++) {
                for (unsigned i = 0; i < 3u; i++) {
                        rampg_t r;
                        containment_setup(&r, shapes[s]);
                        float held = rampg_get(&r);

                        rampg_set_rate(&r, bad[i]);
                        /* Holds; in particular a negative rate must not snap
                         * to the target or run the ramp backwards. */
                        for (int k = 0; k < 10; k++) {
                                TEST_ASSERT(
                                    FLOAT_EQ(rampg_update(&r, 0.01f), held));
                        }
                        containment_recovers(&r);
                }
        }
}

TEST_CASE(test_containment_bad_accel)
{
        const float bad[] = {0.0f, -1000.0f, NAN};

        for (unsigned i = 0; i < 3u; i++) {
                rampg_t r;
                containment_setup(&r, RAMPG_SHAPE_SCURVE);
                float held = rampg_get(&r);

                rampg_set_accel(&r, bad[i]);
                for (int k = 0; k < 10; k++) {
                        TEST_ASSERT(FLOAT_EQ(rampg_update(&r, 0.01f), held));
                }
                containment_recovers(&r);

                /* A bad limit in the direction of travel holds; a bad one in
                 * the other direction must not. */
                rampg_t d;
                containment_setup(&d, RAMPG_SHAPE_SCURVE);
                float held_d = rampg_get(&d);
                rampg_set_accels(&d, bad[i], 1000.0f);
                TEST_ASSERT(FLOAT_EQ(rampg_update(&d, 0.01f), held_d));

                rampg_set_accels(&d, 1000.0f, bad[i]);
                TEST_ASSERT(rampg_update(&d, 0.01f) > held_d);
        }
}

TEST_CASE(test_containment_inverted_limits)
{
        /*
         * min > max violates the contract, but the ramp must still settle
         * deterministically rather than run away or report a rate for an
         * output that is not moving.
         */
        const rampg_shape_t shapes[] = {RAMPG_SHAPE_LINEAR, RAMPG_SHAPE_SCURVE};

        for (unsigned s = 0; s < 2u; s++) {
                rampg_t r;
                rampg_init(&r, 0.0f);
                rampg_set_shape(&r, shapes[s]);
                rampg_set_rate(&r, 100.0f);
                rampg_set_accel(&r, 1000.0f);
                rampg_set_limits(&r, 100.0f, -100.0f);
                rampg_set_target(&r, 0.0f);

                for (int i = 0; i < 200; i++) {
                        rampg_update(&r, 0.001f);
                }

                TEST_ASSERT(!isnan(rampg_get(&r)));
                TEST_ASSERT(!isinf(rampg_get(&r)));
                /* Settled, and reporting no rate while it is not moving. */
                float settled = rampg_get(&r);
                rampg_update(&r, 0.001f);
                TEST_ASSERT(FLOAT_EQ(rampg_get(&r), settled));
                TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
        }
}

TEST_CASE(test_containment_linear_ignores_accel)
{
        /* LINEAR does not use the acceleration limit, so a bad one must not
         * stop it. */
        rampg_t r;
        containment_setup(&r, RAMPG_SHAPE_LINEAR);
        rampg_set_accel(&r, -1.0f);

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 200000)) {
                rampg_update(&r, 0.001f);
                ticks++;
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 500.0f));
}

/* ================ Introspection =============================================
 */

TEST_CASE(test_get_rate_linear_rising)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 42.0f);
        rampg_set_target(&r, 100.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 42.0f));
}

TEST_CASE(test_get_rate_linear_falling)
{
        rampg_t r;
        rampg_init(&r, 100.0f);
        rampg_set_rates(&r, 10.0f, 42.0f);
        rampg_set_target(&r, 0.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), -42.0f));
}

TEST_CASE(test_get_rate_linear_at_rest)
{
        rampg_t r;
        rampg_init(&r, 10.0f);
        TEST_ASSERT(rampg_at_target(&r));
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
}

TEST_CASE(test_get_rate_scurve_tracks_the_applied_step)
{
        /* The reported rate is the one the last update actually applied. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 500.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 2000; i++) {
                float before = rampg_get(&r);
                rampg_update(&r, 0.001f);
                float measured = (rampg_get(&r) - before) / 0.001f;
                if (!rampg_at_target(&r)) {
                        TEST_ASSERT(
                            FLOAT_NEAR(rampg_get_rate(&r), measured, 0.05f));
                }
        }
}

TEST_CASE(test_get_rate_scurve_zero_at_target)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_target(&r, 10.0f);

        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
}

TEST_CASE(test_get_state_linear)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_AT_TARGET);

        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 10.0f);
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_MOVING);

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 10000)) {
                rampg_update(&r, 0.001f);
                ticks++;
        }
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_AT_TARGET);
}

TEST_CASE(test_get_state_scurve_clamped_target)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 25.0f);
        rampg_set_target(&r, 500.0f);

        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_MOVING);
        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_AT_TARGET);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 25.0f));
}

/* ================ Enabled / disabled ========================================
 */

TEST_CASE(test_enabled_by_default)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        TEST_ASSERT(rampg_is_enabled(&r));
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_AT_TARGET);
}

TEST_CASE(test_disabled_holds_value_and_reports_disabled)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 100.0f);
        rampg_update(&r, 0.1f);
        float held = rampg_get(&r);
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_MOVING);

        rampg_set_enabled(&r, false);
        TEST_ASSERT(!rampg_is_enabled(&r));

        for (int i = 0; i < 3; i++) {
                rampg_update(&r, 0.1f);
                TEST_ASSERT(FLOAT_EQ(rampg_get(&r), held));
                TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));
                /* A frozen ramp reports DISABLED, not MOVING. */
                TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_DISABLED);
        }
        /* at_target stays purely positional. */
        TEST_ASSERT(!rampg_at_target(&r));
}

TEST_CASE(test_disabled_at_target_still_reports_disabled)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 10.0f);

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 10000)) {
                rampg_update(&r, 0.01f);
                ticks++;
        }
        rampg_set_enabled(&r, false);

        TEST_ASSERT(rampg_at_target(&r));
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_DISABLED);

        rampg_set_enabled(&r, true);
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_AT_TARGET);
}

TEST_CASE(test_disable_clears_the_rate)
{
        /* A held output is at rest, so re-enabling must ease away from rest
         * rather than resume at a rate the output no longer has. */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rate(&r, 100.0f);
        rampg_set_accel(&r, 1000.0f);
        rampg_set_limits(&r, 0.0f, 2000.0f);
        rampg_set_target(&r, 1000.0f);

        for (int i = 0; i < 500; i++) {
                rampg_update(&r, 0.001f);
        }
        TEST_ASSERT(rampg_get_rate(&r) > 50.0f);

        rampg_set_enabled(&r, false);
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));

        rampg_set_enabled(&r, true);
        TEST_ASSERT(FLOAT_EQ(rampg_get_rate(&r), 0.0f));

        rampg_update(&r, 0.001f);
        /* One acceleration step away from rest, not back at cruise. */
        TEST_ASSERT(rampg_get_rate(&r) <= 1.0f + 1e-4f);

        drive_checked(&r, 0.001f, 100000);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 1000.0f));
}

TEST_CASE(test_reenable_resumes_from_current_value)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 100.0f);
        rampg_update(&r, 0.1f);
        float held = rampg_get(&r);

        rampg_set_enabled(&r, false);
        rampg_update(&r, 0.5f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), held));

        rampg_set_enabled(&r, true);
        rampg_update(&r, 0.1f);
        TEST_ASSERT(rampg_get(&r) > held);

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 10000)) {
                rampg_update(&r, 0.01f);
                ticks++;
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 100.0f));
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_AT_TARGET);
}

TEST_CASE(test_retarget_while_disabled_applies_on_resume)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_rate(&r, 100.0f);
        rampg_set_target(&r, 100.0f);
        rampg_update(&r, 0.1f);

        rampg_set_enabled(&r, false);
        rampg_update(&r, 0.5f);
        float held = rampg_get(&r);

        rampg_set_target(&r, 50.0f);
        rampg_set_enabled(&r, true);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), held));

        int ticks = 0;
        while (!rampg_at_target(&r) && (ticks < 10000)) {
                rampg_update(&r, 0.01f);
                ticks++;
        }
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 50.0f));
}

TEST_CASE(test_reset_preserves_disabled_state)
{
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_target(&r, 100.0f);
        rampg_set_enabled(&r, false);

        rampg_reset(&r, 42.0f);
        TEST_ASSERT(FLOAT_EQ(rampg_get(&r), 42.0f));
        TEST_ASSERT(!rampg_is_enabled(&r));
        TEST_ASSERT(rampg_get_state(&r) == RAMPG_STATE_DISABLED);
}

/* ================ Property sweep ============================================
 */

TEST_CASE(test_scurve_invariants_over_many_configurations)
{
        /*
         * Sweep a spread of rates, accelerations, tick lengths and distances,
         * including direction reversals, asserting on every update that the
         * rate stays within its limit, that it changes by at most one
         * acceleration step, and that the output stays inside the limits.
         */
        static const float rates[] = {0.5f, 10.0f, 100.0f, 900.0f};
        static const float accels[] = {5.0f, 250.0f, 20000.0f};
        static const float dts[] = {0.0001f, 0.001f, 0.02f};
        static const float targets[] = {0.05f, 7.0f, 250.0f, -180.0f};

        const int tick_cap = 400000;
        int covered = 0;

        for (unsigned a = 0; a < 4u; a++) {
                for (unsigned b = 0; b < 3u; b++) {
                        for (unsigned c = 0; c < 3u; c++) {
                                for (unsigned d = 0; d < 4u; d++) {
                                        float dist = fabsf(targets[d]);
                                        float dur = scurve_duration(
                                            dist, rates[a], accels[b]);

                                        /* Keep the sweep bounded: skip
                                         * combinations whose move is longer
                                         * than the tick budget allows. */
                                        if ((dur / dts[c]) > (float)tick_cap) {
                                                continue;
                                        }

                                        rampg_t r;
                                        rampg_init(&r, 0.0f);
                                        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
                                        rampg_set_rate(&r, rates[a]);
                                        rampg_set_accel(&r, accels[b]);
                                        rampg_set_limits(&r, -500.0f, 500.0f);

                                        rampg_set_target(&r, targets[d]);
                                        drive_checked(&r, dts[c], tick_cap);
                                        TEST_ASSERT(FLOAT_NEAR(
                                            rampg_get(&r), targets[d], 1e-3f));

                                        /* Reverse straight back. */
                                        rampg_set_target(&r, 0.0f);
                                        drive_checked(&r, dts[c], tick_cap);
                                        TEST_ASSERT(
                                            FLOAT_EQ(rampg_get(&r), 0.0f));
                                        covered++;
                                }
                        }
                }
        }

        /* Guard against the sweep silently skipping everything. */
        TEST_ASSERT(covered >= 80);
}

TEST_CASE(test_scurve_retarget_sweep_stays_bounded)
{
        /*
         * Retarget repeatedly mid-move, including inside the braking
         * distance, and assert the output never leaves the limits and the
         * rate never exceeds its limit. The acceleration bound is not
         * asserted here: a target moved inside the braking distance cannot be
         * reached under it, and the generator stops on the target instead.
         */
        rampg_t r;
        rampg_init(&r, 0.0f);
        rampg_set_shape(&r, RAMPG_SHAPE_SCURVE);
        rampg_set_rates(&r, 120.0f, 300.0f);
        rampg_set_accel(&r, 800.0f);
        rampg_set_limits(&r, -100.0f, 100.0f);

        unsigned seed = 1u;
        for (int i = 0; i < 200000; i++) {
                if ((i % 37) == 0) {
                        seed = (seed * 1103515245u) + 12345u;
                        float t = (float)((seed >> 16) % 20001u) / 100.0f;
                        rampg_set_target(&r, t - 100.0f);
                }
                rampg_update(&r, 0.001f);

                TEST_ASSERT(rampg_get(&r) >= -100.0f);
                TEST_ASSERT(rampg_get(&r) <= 100.0f);
                TEST_ASSERT(fabsf(rampg_get_rate(&r)) <= 300.0f + 1e-4f);
                TEST_ASSERT(!isnan(rampg_get(&r)));
        }
}

/* ================ Runner ===================================================
 */

static int test_count;

static void
run_test(void (*test_func)(void), const char *name)
{
        test_func();
        TEST_PASS(name);
        test_count++;
}

int
main(void)
{
        test_count = 0;

        fprintf(stdout, "\n=== Running rampg unit tests ===\n\n");

        /* Basic */
        run_test(test_init_sets_value_and_target,
                 "test_init_sets_value_and_target");
        run_test(test_init_defaults, "test_init_defaults");
        run_test(test_ramp_up_basic, "test_ramp_up_basic");
        run_test(test_ramp_down_basic, "test_ramp_down_basic");
        run_test(test_ramp_reaches_target_exactly,
                 "test_ramp_reaches_target_exactly");
        run_test(test_ramp_does_not_overshoot, "test_ramp_does_not_overshoot");

        /* Asymmetric rates */
        run_test(test_asymmetric_rates, "test_asymmetric_rates");
        run_test(test_set_rate_overwrites_both,
                 "test_set_rate_overwrites_both");

        /* Output clamping */
        run_test(test_output_clamping_upper, "test_output_clamping_upper");
        run_test(test_output_clamping_lower, "test_output_clamping_lower");
        run_test(test_clamp_applied_after_init,
                 "test_clamp_applied_after_init");

        /* Target beyond clamp */
        run_test(test_target_above_upper_limit,
                 "test_target_above_upper_limit");
        run_test(test_target_below_lower_limit,
                 "test_target_below_lower_limit");
        run_test(test_target_beyond_limit_preserves_stored_target,
                 "test_target_beyond_limit_preserves_stored_target");

        /* Reset */
        run_test(test_reset_snaps_value, "test_reset_snaps_value");
        run_test(test_reset_outside_limits_clamps_value,
                 "test_reset_outside_limits_clamps_value");
        run_test(test_reset_below_limits_clamps_value,
                 "test_reset_below_limits_clamps_value");

        /* get / at_target */
        run_test(test_get_does_not_advance, "test_get_does_not_advance");
        run_test(test_already_at_target, "test_already_at_target");

        /* Edge cases */
        run_test(test_zero_dt_no_change, "test_zero_dt_no_change");
        run_test(test_negative_initial_value, "test_negative_initial_value");
        run_test(test_negative_to_negative_ramp,
                 "test_negative_to_negative_ramp");
        run_test(test_multi_step_ramp, "test_multi_step_ramp");
        run_test(test_change_target_mid_ramp, "test_change_target_mid_ramp");
        run_test(test_change_rate_mid_ramp, "test_change_rate_mid_ramp");
        run_test(test_repeated_set_target_same_value,
                 "test_repeated_set_target_same_value");
        run_test(test_limits_narrowed_mid_ramp,
                 "test_limits_narrowed_mid_ramp");
        run_test(test_very_large_dt, "test_very_large_dt");
        run_test(test_small_dt_accumulation, "test_small_dt_accumulation");
        run_test(test_small_dt_precision, "test_small_dt_precision");
        run_test(test_linear_subresolution_step_does_not_jump_to_target,
                 "test_linear_subresolution_step_does_not_jump_to_target");
        run_test(test_linear_accumulates_subresolution_steps,
                 "test_linear_accumulates_subresolution_steps");
        run_test(test_linear_discards_subresolution_budget_on_reversal,
                 "test_linear_discards_subresolution_budget_on_reversal");
        run_test(test_linear_subresolution_budget_preserves_average_rate,
                 "test_linear_subresolution_budget_preserves_average_rate");
        run_test(test_linear_subresolution_budget_lands_exactly,
                 "test_linear_subresolution_budget_lands_exactly");
        run_test(test_linear_rate_product_underflow_holds,
                 "test_linear_rate_product_underflow_holds");
        run_test(test_linear_step_below_budget_resolution_holds,
                 "test_linear_step_below_budget_resolution_holds");

        /* S-curve */
        run_test(test_scurve_reaches_target_exactly,
                 "test_scurve_reaches_target_exactly");
        run_test(test_scurve_trapezoidal_duration,
                 "test_scurve_trapezoidal_duration");
        run_test(test_scurve_triangular_duration,
                 "test_scurve_triangular_duration");
        run_test(test_scurve_reaches_configured_peak_rate,
                 "test_scurve_reaches_configured_peak_rate");
        run_test(test_scurve_eases_in_and_out, "test_scurve_eases_in_and_out");
        run_test(test_scurve_no_overshoot, "test_scurve_no_overshoot");
        run_test(test_scurve_asymmetric_rates, "test_scurve_asymmetric_rates");
        run_test(test_scurve_asymmetric_accels,
                 "test_scurve_asymmetric_accels");
        run_test(test_scurve_asymmetric_accel_bound_per_direction,
                 "test_scurve_asymmetric_accel_bound_per_direction");
        run_test(test_scurve_set_accel_sets_both,
                 "test_scurve_set_accel_sets_both");
        run_test(test_scurve_precharge_asymmetric_scenario,
                 "test_scurve_precharge_asymmetric_scenario");
        run_test(test_scurve_retarget_carries_the_rate,
                 "test_scurve_retarget_carries_the_rate");
        run_test(test_scurve_progresses_under_a_moving_setpoint,
                 "test_scurve_progresses_under_a_moving_setpoint");
        run_test(test_scurve_reversal_crosses_zero_smoothly,
                 "test_scurve_reversal_crosses_zero_smoothly");
        run_test(test_scurve_rate_change_mid_move,
                 "test_scurve_rate_change_mid_move");
        run_test(test_scurve_accel_change_mid_move,
                 "test_scurve_accel_change_mid_move");
        run_test(test_scurve_limit_narrowed_mid_move,
                 "test_scurve_limit_narrowed_mid_move");
        run_test(test_scurve_limit_widened_mid_move,
                 "test_scurve_limit_widened_mid_move");
        run_test(test_scurve_target_beyond_limit,
                 "test_scurve_target_beyond_limit");
        run_test(test_scurve_zero_distance_move,
                 "test_scurve_zero_distance_move");
        run_test(test_scurve_zero_dt_no_progress,
                 "test_scurve_zero_dt_no_progress");
        run_test(test_scurve_very_large_dt, "test_scurve_very_large_dt");
        run_test(test_scurve_long_slow_move_completes,
                 "test_scurve_long_slow_move_completes");
        run_test(test_scurve_shape_change_from_linear_carries_the_rate,
                 "test_scurve_shape_change_from_linear_carries_the_rate");
        run_test(test_scurve_shape_change_to_linear_mid_move,
                 "test_scurve_shape_change_to_linear_mid_move");
        run_test(test_scurve_reset_clears_the_rate,
                 "test_scurve_reset_clears_the_rate");

        /* Containment */
        run_test(test_containment_bad_dt, "test_containment_bad_dt");
        run_test(test_containment_bad_target, "test_containment_bad_target");
        run_test(test_containment_infinite_target_clamps_to_limit,
                 "test_containment_infinite_target_clamps_to_limit");
        run_test(test_containment_bad_rate, "test_containment_bad_rate");
        run_test(test_containment_bad_accel, "test_containment_bad_accel");
        run_test(test_containment_inverted_limits,
                 "test_containment_inverted_limits");
        run_test(test_containment_linear_ignores_accel,
                 "test_containment_linear_ignores_accel");

        /* Introspection */
        run_test(test_get_rate_linear_rising, "test_get_rate_linear_rising");
        run_test(test_get_rate_linear_falling, "test_get_rate_linear_falling");
        run_test(test_get_rate_linear_at_rest, "test_get_rate_linear_at_rest");
        run_test(test_get_rate_scurve_tracks_the_applied_step,
                 "test_get_rate_scurve_tracks_the_applied_step");
        run_test(test_get_rate_scurve_zero_at_target,
                 "test_get_rate_scurve_zero_at_target");
        run_test(test_get_state_linear, "test_get_state_linear");
        run_test(test_get_state_scurve_clamped_target,
                 "test_get_state_scurve_clamped_target");

        /* Enabled / disabled */
        run_test(test_enabled_by_default, "test_enabled_by_default");
        run_test(test_disabled_holds_value_and_reports_disabled,
                 "test_disabled_holds_value_and_reports_disabled");
        run_test(test_disabled_at_target_still_reports_disabled,
                 "test_disabled_at_target_still_reports_disabled");
        run_test(test_disable_clears_the_rate, "test_disable_clears_the_rate");
        run_test(test_reenable_resumes_from_current_value,
                 "test_reenable_resumes_from_current_value");
        run_test(test_retarget_while_disabled_applies_on_resume,
                 "test_retarget_while_disabled_applies_on_resume");
        run_test(test_reset_preserves_disabled_state,
                 "test_reset_preserves_disabled_state");

        /* Property sweeps */
        run_test(test_scurve_invariants_over_many_configurations,
                 "test_scurve_invariants_over_many_configurations");
        run_test(test_scurve_retarget_sweep_stays_bounded,
                 "test_scurve_retarget_sweep_stays_bounded");

        /* Integration scenarios */
        run_test(test_sc_precharge_scenario, "test_sc_precharge_scenario");
        run_test(test_sc_frequency_ramp_scenario,
                 "test_sc_frequency_ramp_scenario");

        fprintf(stdout, "\n=== All %d tests passed ===\n\n", test_count);
        return EXIT_SUCCESS;
}
