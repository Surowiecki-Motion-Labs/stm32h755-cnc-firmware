#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>

#define AXIS_COUNT              3
#define PERCENT_SCALE           100U

static const uint32_t UM_PER_MM = 1000U;
static const uint32_t SECONDS_PER_MINUTE = 60U;

#define X_DRIVER_PULSES_PER_REV 6400
#define Y_DRIVER_PULSES_PER_REV 6400
#define Z_DRIVER_PULSES_PER_REV 6400

#define X_TRAVEL_UM_PER_REV     5000
#define Y_TRAVEL_UM_PER_REV     5000
#define Z_TRAVEL_UM_PER_REV     5000

#define FEED_OVERRIDE_DEFAULT       100U
#define FEED_OVERRIDE_MIN           5U
#define FEED_OVERRIDE_MAX           150U

#define SPINDLE_OVERRIDE_DEFAULT    100U
#define SPINDLE_OVERRIDE_MIN        50U
#define SPINDLE_OVERRIDE_MAX        150U

#define IPC_MAILBOX_ADDR_CM7    (0x30044000UL)
#define IPC_MAILBOX_ADDR_CM4    (0x10044000UL)

#endif /* CONFIG_H_ */
