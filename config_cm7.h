#ifndef CONFIG_CM7_H_
#define CONFIG_CM7_H_

#define RAPID_FEED_MM_MIN       2000U

#define PROGRAM_MAX_LINES       32U
#define PROGRAM_LINE_LEN        96U

#define USB_RX_LINE_BUF_SIZE        128U
#define USB_TX_LINE_BUF_SIZE        512U
#define USB_TX_RETRY_COUNT          50U
#define USB_TX_RETRY_DELAY_LOOPS    20000U

#define LOGGER_BUF_SIZE         4096U
#define LOGGER_TEMP_SIZE        192U
#define LOG_ARM_ON_RUN_MS       1000U

#endif /* CONFIG_CM7_H_ */
