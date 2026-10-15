# API reference

Every declaration lives in [`include/rampg.h`](../include/rampg.h), which carries the full Doxygen contract. Configuration macros are listed in the [integration guide](integration.md#configuration), and the profiles are explained in [design.md](design.md).

## Conventions

- The caller owns every `rampg_t`, on the stack, in static storage, or inside a larger struct. The library allocates nothing and keeps no global state.
- `rampg_init` must run before any other call on an instance. The defaults are not all zero, so aggregate initialisation is not enough.
- Preconditions are documented with `@pre` and are not checked at run time. Breaking one is undefined behaviour, as for any C library function; validate at the call site when the application cannot guarantee them.
- `rampg_update` is the exception: it holds the output on invalid inputs, as described under [Updating](#updating).
- The caller supplies the time step in seconds. Every rate is in units per second and every acceleration in units per second squared, in whatever unit the output represents.
- An instance is not thread-safe. Provide mutual exclusion when one is shared between threads or between an interrupt handler and the main loop.

## Types

<!-- snippet: skip -->
```c
typedef enum {
        RAMPG_SHAPE_LINEAR,
        RAMPG_SHAPE_SCURVE,
} rampg_shape_t;

typedef enum {
        RAMPG_STATE_MOVING,
        RAMPG_STATE_AT_TARGET,
        RAMPG_STATE_DISABLED,
} rampg_state_t;

typedef struct {
        float value;         /* current output */
        float target;        /* target, unclamped */
        float rise_rate;     /* units/s */
        float fall_rate;     /* units/s */
        float rise_accel;    /* units/s^2 */
        float fall_accel;    /* units/s^2 */
        float limit_min;     /* output clamp minimum */
        float limit_max;     /* output clamp maximum */
        float vel;           /* internal profile state */
        rampg_shape_t shape;
        bool enabled;
} rampg_t;
```

`RAMPG_SHAPE_LINEAR` moves at a constant rate. `RAMPG_SHAPE_SCURVE` also bounds how fast the rate changes, so the output eases in and out of a move.

`rampg_t` is a plain aggregate with no pointers, so it can be copied with `memcpy`, embedded in another struct, or placed in shared memory. Every field except `vel` is configuration and may be read directly; read the output rate with `rampg_get_rate`, not from `vel`. Write fields only through the functions below.

## Lifecycle

<!-- snippet: skip -->
```c
void rampg_init(rampg_t *ramp, float initial);
void rampg_reset(rampg_t *ramp, float value);
```

`rampg_init` sets the target to `initial`, the output to `initial` clamped to the default limits, and the rate to zero. Both rates, both accelerations, the limits and the shape take their defaults from `rampg_conf.h`, and the ramp is enabled. `ramp` must not be `NULL`.

`rampg_reset` snaps the output to `value` without ramping. It sets the target to `value`, the output to `value` clamped to the current limits, and the rate to zero. The enabled flag is unchanged.

## Configuration

<!-- snippet: skip -->
```c
void rampg_set_target(rampg_t *ramp, float target);
void rampg_set_rate(rampg_t *ramp, float rate);
void rampg_set_rates(rampg_t *ramp, float rise_rate, float fall_rate);
void rampg_set_accel(rampg_t *ramp, float accel);
void rampg_set_accels(rampg_t *ramp, float rise_accel, float fall_accel);
void rampg_set_limits(rampg_t *ramp, float min, float max);
void rampg_set_shape(rampg_t *ramp, rampg_shape_t shape);
```

Every change takes effect at the next `rampg_update`, and the output rate is carried across it, so a change in the middle of a move does not restart the move.

`rampg_set_target` stores the target unclamped. `rampg_set_rate` sets both directions to `rate`, and `rampg_set_rates` sets them independently; each rate must be greater than zero. The direction of the move selects the rate, so a slow rise can be paired with a fast fall.

`rampg_set_accel` and `rampg_set_accels` set the acceleration limit the same way, and each must be greater than zero. Only the S-curve profile uses them.

`rampg_set_limits` sets the output clamp and requires `min <= max`. It clamps the current output to the new range at once, and if that moves the output, resets the rate to zero, because the output was displaced rather than ramped. The stored target is not changed, so widening the limits later recovers it.

`rampg_set_shape` selects the profile. A switch from linear to S-curve starts the S-curve at the rate the linear ramp was running at, so the rate stays continuous. A switch from S-curve to linear resumes at the full configured rate, which is the step in rate that the linear profile always has.

## Updating

<!-- snippet: skip -->
```c
float rampg_update(rampg_t *ramp, float dt);
float rampg_get(const rampg_t *ramp);
```

`rampg_update` advances the output by `dt` seconds toward the effective target, which is the stored target clamped to the limits, and returns the new output. The output never overshoots the effective target, never leaves the limits, and lands exactly on the target. `rampg_get` returns the output without advancing.

The linear profile moves by `rate * dt` per update. When that is below the resolution of the output, it accumulates the movement and holds the visible output until there is enough for the next representable `float`; [design.md](design.md#numerical-range) gives the ratio of output to step over which a move keeps progressing. The S-curve profile is described in [design.md](design.md#s-curve-profile).

`rampg_update` is total. It returns the output unchanged, rather than corrupting the state, when `dt` is not a finite value greater than zero, when the effective target is not finite, or when the rate, or under the S-curve the acceleration, for the direction of the move is not greater than zero. The next update with valid inputs resumes normally. This contains bad values that a control loop derives from the outside world; it does not replace the preconditions on the setters.

## Rate and state

<!-- snippet: skip -->
```c
float rampg_get_rate(const rampg_t *ramp);
bool rampg_at_target(const rampg_t *ramp);
rampg_state_t rampg_get_state(const rampg_t *ramp);
```

`rampg_get_rate` returns the signed output rate in units per second: positive while rising, negative while falling. Under the linear profile it is the configured rise or fall rate while moving and zero at rest; the visible output may hold between updates while sub-resolution movement accumulates. Under the S-curve it is the rate the last update applied. In both it stays correct across a target, rate, limit or shape change, so it can be used directly as a feedforward term. A disabled ramp reads zero.

`rampg_at_target` reports whether the output equals the effective target. The comparison is exact, which works because `rampg_update` lands exactly on the target. It ignores the enabled flag.

`rampg_get_state` returns `RAMPG_STATE_DISABLED` whenever the ramp is disabled, and otherwise `RAMPG_STATE_MOVING` or `RAMPG_STATE_AT_TARGET`. Both functions use the effective target, so a target outside the limits counts as reached at the nearer limit, with a zero rate. There is no separate state for an unreachable target.

## Enable and disable

<!-- snippet: skip -->
```c
void rampg_set_enabled(rampg_t *ramp, bool enabled);
bool rampg_is_enabled(const rampg_t *ramp);
```

A disabled ramp holds its output: `rampg_update` returns it unchanged and `rampg_get_rate` reads zero. Disabling also resets the rate to zero, so re-enabling eases away from rest instead of resuming at a rate the output no longer has. The target, rates, accelerations, limits and shape are kept.

Because `rampg_at_target` ignores the flag, a held ramp short of its target reports `false` there while `rampg_get_state` reports `RAMPG_STATE_DISABLED`. Use `rampg_get_state` to tell a held ramp from a moving one.

## Recipes

### Precharge with a fast trip

A DC bus is raised slowly and dropped fast on a fault. Under the S-curve, both legs ease at their own acceleration limit: the rise from 0 to 400 V takes 8.5 s and the trip back to 0 takes 0.25 s. The trade-off behind these numbers is worked through in [design.md](design.md#asymmetric-legs).

```c
static rampg_t bus;

void
bus_configure(void)
{
        rampg_init(&bus, 0.0f);
        rampg_set_shape(&bus, RAMPG_SHAPE_SCURVE);
        rampg_set_rates(&bus, 50.0f, 2000.0f);    /* V/s up, down */
        rampg_set_accels(&bus, 100.0f, 40000.0f); /* V/s^2 up, down */
        rampg_set_limits(&bus, 0.0f, 800.0f);
}

void
bus_precharge(void)
{
        rampg_set_target(&bus, 400.0f);
}

void
bus_trip(void)
{
        rampg_set_target(&bus, 0.0f);
}
```

### Live setpoint with feedforward

A supervisor may move the target on every tick. The rate is carried across each change, so the ramp re-aims from its current rate without stepping or restarting, including through a reversal. The rate it returns is exact for the step just taken.

```c
float supervisor_setpoint(void);
void apply(float value, float feedforward);

void
control_tick(rampg_t *ramp)
{
        rampg_set_target(ramp, supervisor_setpoint());
        float v = rampg_update(ramp, 0.001f);
        apply(v, rampg_get_rate(ramp));
}
```

### Monitoring

`rampg_get_state` and `rampg_get_rate` read progress without advancing the ramp.

```c
const char *
ramp_status(const rampg_t *ramp)
{
        switch (rampg_get_state(ramp)) {
        case RAMPG_STATE_MOVING:
                return (rampg_get_rate(ramp) > 0.0f) ? "rising" : "falling";
        case RAMPG_STATE_AT_TARGET:
                return "at target";
        case RAMPG_STATE_DISABLED:
        default:
                return "held";
        }
}
```

### Other uses

The same pattern serves voltage or frequency setpoint shaping between operating points, a bounded shutdown as a ramp followed by a trip, motor speed profiles with controlled acceleration and deceleration, and smoothing of any operator setpoint where a sudden jump would be harmful.

## Version

`rampg.h` defines the release it belongs to, so a copied-in header still identifies itself:

<!-- snippet: skip -->
```c
#define RAMPG_VERSION_MAJOR 0
#define RAMPG_VERSION_MINOR 2
#define RAMPG_VERSION_PATCH 1
#define RAMPG_VERSION "0.2.1"
```

`RAMPG_VERSION_NUMBER` is `MAJOR * 10000 + MINOR * 100 + PATCH` as a `long`, so it holds any version where `int` is 16 bits. Compare it in the preprocessor to require a minimum release:

```c
#if RAMPG_VERSION_NUMBER < 201L
#error "rampg 0.2.1 or later is required"
#endif
```

`RAMPG_VERSION_STRING` is the same string as `RAMPG_VERSION`. `rampg_version.h` is a deprecated header that includes `rampg.h`, kept so existing includes compile until the next major release.
