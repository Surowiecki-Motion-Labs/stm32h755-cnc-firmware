#include "stepgen.h"
#include "axis.h"
#include "safety.h"
#include "main.h"
#include "config.h"
#include "config_cm4.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;

/*
 * Axis pulse mask:
 *
 * bit 0 -> X -> 001
 * bit 1 -> Y -> 010
 * bit 2 -> Z -> 100
 *
 * Combinations:
 *
 * X           -> 001
 * Y           -> 010
 * Z           -> 100
 * X + Y       -> 011
 * X + Z       -> 101
 * Y + Z       -> 110
 * X + Y + Z   -> 111
 */

typedef struct
{
    GPIO_TypeDef *dir_port;
    uint16_t dir_pin;
    GPIO_TypeDef *step_port;
    uint16_t step_pin;
} stepgen_axis_hw_t;

static const stepgen_axis_hw_t stepgen_axis_hw[AXIS_COUNT] =
{
    { X_DIR_PORT, X_DIR_PIN, X_STEP_PORT, X_STEP_PIN },
    { Y_DIR_PORT, Y_DIR_PIN, Y_STEP_PORT, Y_STEP_PIN },
    { Z_DIR_PORT, Z_DIR_PIN, Z_STEP_PORT, Z_STEP_PIN }
};

static const uint8_t stepgen_axis_positive_dir[AXIS_COUNT] =
{
    [AXIS_X] = X_DEFAULT_DIR,
    [AXIS_Y] = Y_DEFAULT_DIR,
    [AXIS_Z] = Z_DEFAULT_DIR
};

typedef struct
{
    uint8_t busy;
    uint8_t held;

    uint32_t feed_hz;
    uint32_t base_feed_hz;
    uint16_t feed_override_percent;

    int32_t target_steps[AXIS_COUNT];
    int32_t delta_steps[AXIS_COUNT];
    int32_t remaining_steps[AXIS_COUNT];
    uint8_t direction[AXIS_COUNT];

    axis_id_t major_axis;
    uint32_t major_steps_total;
    uint32_t major_steps_done;

    uint32_t error_accum[AXIS_COUNT];
} stepgen_state_t;

static stepgen_state_t sg;
static volatile uint8_t sg_pending_step_low_mask = 0U;
static uint8_t sg_enable_active = 0U;
static uint32_t sg_enable_last_active_ms = 0U;

static void stepgen_hw_set_step_low_all(void);
static void stepgen_hw_timer_stop(void);
static void stepgen_hw_pulse_timer_stop(void);
static void stepgen_state_clear(void);

static uint32_t stepgen_abs32(int32_t v)
{
    return (v >= 0) ? (uint32_t)v : (uint32_t)(-v);
}

static void stepgen_hw_init_outputs(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio_init.Pin = AXIS_ENABLE_PIN;
    HAL_GPIO_Init(AXIS_ENABLE_PORT, &gpio_init);
    HAL_GPIO_WritePin(
        AXIS_ENABLE_PORT,
        AXIS_ENABLE_PIN,
        STEPGEN_ENABLE_INACTIVE_STATE
    );

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        HAL_GPIO_WritePin(
            stepgen_axis_hw[i].step_port,
            stepgen_axis_hw[i].step_pin,
            STEPGEN_STEP_INACTIVE_STATE
        );

        gpio_init.Pin = stepgen_axis_hw[i].step_pin;
        HAL_GPIO_Init(stepgen_axis_hw[i].step_port, &gpio_init);

        HAL_GPIO_WritePin(
            stepgen_axis_hw[i].dir_port,
            stepgen_axis_hw[i].dir_pin,
            STEPGEN_DIR_NEGATIVE_STATE
        );

        gpio_init.Pin = stepgen_axis_hw[i].dir_pin;
        HAL_GPIO_Init(stepgen_axis_hw[i].dir_port, &gpio_init);
    }
}

static void stepgen_hw_set_enable_all(uint8_t enable)
{
    HAL_GPIO_WritePin(
        AXIS_ENABLE_PORT,
        AXIS_ENABLE_PIN,
        enable ? STEPGEN_ENABLE_ACTIVE_STATE : STEPGEN_ENABLE_INACTIVE_STATE
    );
}

static void stepgen_enable_set(uint8_t enable)
{
    stepgen_hw_set_enable_all(enable);
    sg_enable_active = enable ? 1U : 0U;

    if (sg_enable_active)
    {
        sg_enable_last_active_ms = HAL_GetTick();
    }
}

static void stepgen_enable_prepare_for_motion(void)
{
    if (!sg_enable_active)
    {
        stepgen_enable_set(1U);
        HAL_Delay(STEPGEN_ENABLE_SETTLE_MS);
        return;
    }

    sg_enable_last_active_ms = HAL_GetTick();
}

static void stepgen_finish_segment(void)
{
    stepgen_hw_timer_stop();
    stepgen_hw_pulse_timer_stop();

    sg_pending_step_low_mask = 0U;

    stepgen_hw_set_step_low_all();
    sg_enable_last_active_ms = HAL_GetTick();

    stepgen_state_clear();
}

static void stepgen_hw_set_dir(axis_id_t axis, uint8_t dir)
{
    uint8_t hw_dir;

    if (!axis_check(axis))
    {
        return;
    }

    hw_dir = dir ? stepgen_axis_positive_dir[axis] : (uint8_t)!stepgen_axis_positive_dir[axis];

    HAL_GPIO_WritePin(
        stepgen_axis_hw[axis].dir_port,
        stepgen_axis_hw[axis].dir_pin,
        hw_dir ? STEPGEN_DIR_POSITIVE_STATE : STEPGEN_DIR_NEGATIVE_STATE
    );
}

static void stepgen_hw_set_step_low_all(void)
{
    for (int i = 0; i < AXIS_COUNT; i++)
    {
        HAL_GPIO_WritePin(
            stepgen_axis_hw[i].step_port,
            stepgen_axis_hw[i].step_pin,
            STEPGEN_STEP_INACTIVE_STATE
        );
    }
}

static void stepgen_hw_step_high_mask(uint8_t mask)
{
    for (int i = 0; i < AXIS_COUNT; i++)
    {
        if (mask & (1U << i))
        {
            HAL_GPIO_WritePin(
                stepgen_axis_hw[i].step_port,
                stepgen_axis_hw[i].step_pin,
                STEPGEN_STEP_ACTIVE_STATE
            );
        }
    }
}

static void stepgen_hw_step_low_mask(uint8_t mask)
{
    for (int i = 0; i < AXIS_COUNT; i++)
    {
        if (mask & (1U << i))
        {
            HAL_GPIO_WritePin(
                stepgen_axis_hw[i].step_port,
                stepgen_axis_hw[i].step_pin,
                STEPGEN_STEP_INACTIVE_STATE
            );
        }
    }
}

static void stepgen_hw_set_frequency(uint32_t freq_hz)
{
    uint32_t arr;

    if (freq_hz == 0U)
    {
        freq_hz = STEPGEN_DEFAULT_FEED_HZ;
    }

    arr = STEPGEN_TIMER_BASE_HZ / freq_hz;

    if (arr == 0U)
    {
        arr = 1U;
    }

    __HAL_TIM_SET_AUTORELOAD(&htim1, arr - 1U);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);
}

static void stepgen_hw_timer_start(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);

    HAL_TIM_Base_Start_IT(&htim1);
}

static void stepgen_hw_timer_stop(void)
{
    HAL_TIM_Base_Stop_IT(&htim1);
    __HAL_TIM_SET_COUNTER(&htim1, 0U);
}

static void stepgen_hw_pulse_timer_start(void)
{
    __HAL_TIM_SET_AUTORELOAD(&htim2, STEP_PULSE_WIDTH_US - 1U);
    __HAL_TIM_SET_COUNTER(&htim2, 0U);
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);

    HAL_TIM_Base_Start_IT(&htim2);
}

static void stepgen_hw_pulse_timer_stop(void)
{
    HAL_TIM_Base_Stop_IT(&htim2);
    __HAL_TIM_SET_COUNTER(&htim2, 0U);
}

static void stepgen_hw_finish_pulse(void)
{
    uint8_t mask = sg_pending_step_low_mask;

    if (mask != 0U)
    {
        stepgen_hw_step_low_mask(mask);
        sg_pending_step_low_mask = 0U;
    }

    stepgen_hw_pulse_timer_stop();
}

static void stepgen_state_clear(void)
{
    sg.busy = 0U;
    sg.held = 0U;
    sg.feed_hz = 0U;
    sg.base_feed_hz = 0U;
    sg.major_axis = AXIS_X;
    sg.major_steps_total = 0U;
    sg.major_steps_done = 0U;

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        sg.target_steps[i] = 0;
        sg.delta_steps[i] = 0;
        sg.remaining_steps[i] = 0;
        sg.direction[i] = 1U;
        sg.error_accum[i] = 0U;
    }
}

static uint8_t stepgen_all_done(void)
{
    for (int i = 0; i < AXIS_COUNT; i++)
    {
        if (sg.remaining_steps[i] > 0)
        {
            return 0U;
        }
    }

    return 1U;
}

static axis_id_t stepgen_find_major_axis(void)
{
    axis_id_t major = AXIS_X;
    int32_t max_steps = sg.remaining_steps[AXIS_X];

    for (int i = 1; i < AXIS_COUNT; i++)
    {
        if (sg.remaining_steps[i] > max_steps)
        {
            max_steps = sg.remaining_steps[i];
            major = (axis_id_t)i;
        }
    }

    return major;
}

static uint32_t stepgen_apply_feed_override(uint32_t base_feed_hz, uint16_t percent)
{
    uint32_t out;

    out = (base_feed_hz * (uint32_t)percent) / PERCENT_SCALE;

    if (out == 0U)
    {
        out = 1U;
    }

    return out;
}

void sg_init(void)
{
    stepgen_state_clear();

    sg_pending_step_low_mask = 0U;

    stepgen_hw_init_outputs();

    stepgen_hw_timer_stop();
    stepgen_hw_pulse_timer_stop();

    stepgen_hw_set_step_low_all();
    stepgen_enable_set(0U);

    sg.feed_override_percent = FEED_OVERRIDE_DEFAULT;
}

uint8_t sg_start(const m_segment_t *segment)
{
    uint8_t any_motion = 0U;

    if (segment == 0 || sg.busy || safety_get_estop() || safety_get_alarm())
    {
        return 0U;
    }

    stepgen_state_clear();

    sg.base_feed_hz = (segment->feed != 0U) ? segment->feed : STEPGEN_DEFAULT_FEED_HZ;
    sg.feed_hz = stepgen_apply_feed_override(sg.base_feed_hz, sg.feed_override_percent);

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        const int32_t current = axis_get_pos((axis_id_t)i);
        const int32_t target = segment->target_steps[i];
        const int32_t delta = target - current;

        sg.target_steps[i] = target;
        sg.delta_steps[i] = delta;

        if (delta >= 0)
        {
            sg.direction[i] = 1U;
            sg.remaining_steps[i] = delta;
        }
        else
        {
            sg.direction[i] = 0U;
            sg.remaining_steps[i] = -delta;
        }

        axis_set_target((axis_id_t)i, target);
        stepgen_hw_set_dir((axis_id_t)i, sg.direction[i]);

        if (sg.remaining_steps[i] > 0)
        {
            any_motion = 1U;
        }
    }

    if (!any_motion)
    {
        return 0U;
    }

    sg.major_axis = stepgen_find_major_axis();
    sg.major_steps_total = (uint32_t)sg.remaining_steps[sg.major_axis];
    sg.major_steps_done = 0U;

    sg.busy = 1U;
    sg.held = 0U;

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        sg.error_accum[i] = 0U;
    }

    sg_pending_step_low_mask = 0U;

    stepgen_hw_set_frequency(sg.feed_hz);
    stepgen_hw_pulse_timer_stop();
    stepgen_hw_set_step_low_all();
    stepgen_enable_prepare_for_motion();
    HAL_Delay(STEPGEN_DIR_SETUP_MS);

    stepgen_hw_timer_start();

    return 1U;
}

uint8_t sg_hold(void)
{
    if (!sg.busy || sg.held)
    {
        return 0U;
    }

    sg.held = 1U;

    stepgen_hw_timer_stop();
    stepgen_hw_pulse_timer_stop();

    sg_pending_step_low_mask = 0U;

    stepgen_hw_set_step_low_all();
    stepgen_enable_set(0U);

    return 1U;
}

uint8_t sg_resume(void)
{
    if (!sg.busy || !sg.held)
    {
        return 0U;
    }

    if (safety_get_estop() || safety_get_alarm())
    {
        return 0U;
    }

    sg.held = 0U;

    sg_pending_step_low_mask = 0U;

    stepgen_hw_set_frequency(sg.feed_hz);
    stepgen_hw_pulse_timer_stop();
    stepgen_hw_set_step_low_all();
    stepgen_enable_prepare_for_motion();
    HAL_Delay(STEPGEN_DIR_SETUP_MS);

    stepgen_hw_timer_start();

    return 1U;
}

void sg_time_tick(void)
{
    uint8_t pulse_mask = 0U;

    if (!sg.busy || sg.held)
    {
        return;
    }

    if (safety_get_estop() || safety_get_alarm())
    {
        stepgen_finish_segment();
        return;
    }

    if (sg_pending_step_low_mask != 0U)
    {
        stepgen_hw_finish_pulse();
    }

    if (sg.major_steps_done >= sg.major_steps_total)
    {
        for (int i = 0; i < AXIS_COUNT; i++)
        {
            axis_set_pos((axis_id_t)i, sg.target_steps[i]);
            sg.remaining_steps[i] = 0;
        }

        sg_stop();
        return;
    }

    if (sg.remaining_steps[sg.major_axis] > 0)
    {
        pulse_mask |= (1U << sg.major_axis);
    }

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        if (i == (int)sg.major_axis)
        {
            continue;
        }

        if (sg.remaining_steps[i] <= 0)
        {
            continue;
        }

        sg.error_accum[i] += stepgen_abs32(sg.delta_steps[i]);

        if (sg.error_accum[i] >= sg.major_steps_total)
        {
            pulse_mask |= (1U << i);
            sg.error_accum[i] -= sg.major_steps_total;
        }
    }

    if (pulse_mask != 0U)
    {
        stepgen_hw_step_high_mask(pulse_mask);
        sg_pending_step_low_mask = pulse_mask;
        stepgen_hw_pulse_timer_start();
    }

    for (int i = 0; i < AXIS_COUNT; i++)
    {
        if (!(pulse_mask & (1U << i)))
        {
            continue;
        }

        if (sg.remaining_steps[i] > 0)
        {
            int32_t current = axis_get_pos((axis_id_t)i);

            if (sg.direction[i])
            {
                current++;
            }
            else
            {
                current--;
            }

            axis_set_pos((axis_id_t)i, current);
            sg.remaining_steps[i]--;
        }

        if (sg.remaining_steps[i] == 0)
        {
            axis_set_pos((axis_id_t)i, sg.target_steps[i]);
        }
    }

    if (pulse_mask & (1U << sg.major_axis))
    {
        sg.major_steps_done++;
    }

    if (stepgen_all_done())
    {
        for (int i = 0; i < AXIS_COUNT; i++)
        {
            axis_set_pos((axis_id_t)i, sg.target_steps[i]);
            sg.remaining_steps[i] = 0;
        }

        stepgen_finish_segment();
    }
}

void sg_stop(void)
{
    stepgen_hw_timer_stop();
    stepgen_hw_pulse_timer_stop();

    sg_pending_step_low_mask = 0U;

    stepgen_hw_set_step_low_all();
    stepgen_enable_set(0U);

    stepgen_state_clear();
}

void sg_run_idle(void)
{
    if (!sg_enable_active || sg.busy || sg.held)
    {
        return;
    }

    if ((HAL_GetTick() - sg_enable_last_active_ms) >= STEPGEN_ENABLE_IDLE_TIMEOUT_MS)
    {
        stepgen_enable_set(0U);
    }
}

uint8_t sg_is_busy(void)
{
    return sg.busy;
}

int32_t sg_get_remaining_steps(axis_id_t axis)
{
    if (!axis_check(axis))
    {
        return 0;
    }

    return sg.remaining_steps[axis];
}

void sg_set_f_override(uint16_t percent)
{
    if (percent < FEED_OVERRIDE_MIN)
    {
        percent = FEED_OVERRIDE_MIN;
    }

    if (percent > FEED_OVERRIDE_MAX)
    {
        percent = FEED_OVERRIDE_MAX;
    }

    sg.feed_override_percent = percent;

    if (sg.busy && !sg.held)
    {
        sg.feed_hz = stepgen_apply_feed_override(sg.base_feed_hz, sg.feed_override_percent);
        stepgen_hw_set_frequency(sg.feed_hz);
    }
}

uint16_t sg_get_f_override(void)
{
    return sg.feed_override_percent;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        sg_time_tick();
    }
    else if (htim->Instance == TIM2)
    {
        stepgen_hw_finish_pulse();
    }
}

uint32_t sg_get_base_feed_hz(void)
{
    return sg.base_feed_hz;
}

uint32_t sg_get_effective_feed_hz(void)
{
    return sg.feed_hz;
}
