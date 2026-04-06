/*
 * @file test_rampg.c
 * @brief Unit tests for rampg.
 */

#include "rampg.h"
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

        /* Target beyond clamp (bug fix) */
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

        /* SC integration scenarios */
        run_test(test_sc_precharge_scenario, "test_sc_precharge_scenario");
        run_test(test_sc_frequency_ramp_scenario,
                 "test_sc_frequency_ramp_scenario");

        fprintf(stdout, "\n=== All %d tests passed ===\n\n", test_count);
        return EXIT_SUCCESS;
}
