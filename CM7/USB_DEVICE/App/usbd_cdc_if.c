/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v1.0_Cube
  * @brief          : Usb device for Virtual Com Port.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"

/* USER CODE BEGIN INCLUDE */
#include "gcode_parser.h"
#include "motion_cmd_builder.h"
#include "machine_state.h"
#include "ipc_cm7.h"
#include "ipc_shared.h"
#include <string.h>
#include <stdio.h>
#include "logger.h"
#include "program_manager.h"
#include <stdlib.h>
#include "config.h"
#include "config_cm7.h"

#include "../../app/Inc/app.h"
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
static char rx_line_buf[USB_RX_LINE_BUF_SIZE];
static uint16_t rx_line_idx = 0;
static uint8_t rx_line_ready = 0;

/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_CDC_IF
  * @{
  */

/** @defgroup USBD_CDC_IF_Private_TypesDefinitions USBD_CDC_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Defines USBD_CDC_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Macros USBD_CDC_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Variables USBD_CDC_IF_Private_Variables
  * @brief Private variables.
  * @{
  */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables USBD_CDC_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_FunctionPrototypes USBD_CDC_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);
static int8_t CDC_TransmitCplt_FS(uint8_t *pbuf, uint32_t *Len, uint8_t epnum);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
static void usb_send_text(const char *text);
static void process_received_line(void);
static uint8_t starts_with(const char *text, const char *prefix);

static void usb_send_text(const char *text)
{
    uint8_t ret;
    uint32_t tries = 0;

    if (text == NULL)
    {
        return;
    }

    do
    {
        ret = CDC_Transmit_FS((uint8_t*)text, (uint16_t)strlen(text));
        if (ret == USBD_OK)
        {
            return;
        }

        for (volatile uint32_t d = 0; d < USB_TX_RETRY_DELAY_LOOPS; d++)
        {
            __NOP();
        }

        tries++;
    }
    while (ret == USBD_BUSY && tries < USB_TX_RETRY_COUNT);
}

static void process_received_line(void)
{
    char tx_buf[USB_TX_LINE_BUF_SIZE];

    if (starts_with(rx_line_buf, "GC:"))
    {
        const char *gline = rx_line_buf + 3;
        app_proc_res_t res = app_process_line(gline);

        if (res.res == APP_RES_OK)
        {
            snprintf(tx_buf, sizeof(tx_buf), "OK:GC [%s]\r\n", gline);
        }
        else
        {
            switch (res.err)
            {
                case APP_ERR_PARSE:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:PARSE [%s]\r\n", gline);
                    break;

                case APP_ERR_NO_FEED:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:NO_FEED [%s]\r\n", gline);
                    break;

                case APP_ERR_NO_SP:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:NO_SP [%s]\r\n", gline);
                    break;

                case APP_ERR_UNSUPP:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:UNSUPP [%s]\r\n", gline);
                    break;

                case APP_ERR_IPC:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:IPC [%s]\r\n", gline);
                    break;

                default:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:BUILD [%s]\r\n", gline);
                    break;
            }
        }

        usb_send_text(tx_buf);
        return;
    }

    if (starts_with(rx_line_buf, "ADD:"))
    {
        const char *gline = rx_line_buf + 4;
        app_proc_res_t res = app_add_line(gline);

        if (res.res == APP_RES_OK)
        {
            snprintf(tx_buf, sizeof(tx_buf), "OK:ADD [%s]\r\n", gline);
        }
        else
        {
            switch (res.err)
            {
                case APP_ERR_PARSE:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:PARSE [%s]\r\n", gline);
                    break;

                case APP_ERR_NO_FEED:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:NO_FEED [%s]\r\n", gline);
                    break;

                case APP_ERR_NO_SP:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:NO_SP [%s]\r\n", gline);
                    break;

                case APP_ERR_UNSUPP:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:UNSUPP [%s]\r\n", gline);
                    break;

                case APP_ERR_FULL:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:FULL [%s]\r\n", gline);
                    break;

                case APP_ERR_STATE:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:STATE [%s]\r\n", gline);
                    break;

                case APP_ERR_IPC:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:IPC [%s]\r\n", gline);
                    break;

                default:
                    snprintf(tx_buf, sizeof(tx_buf), "ERR:BUILD [%s]\r\n", gline);
                    break;
            }
        }

        usb_send_text(tx_buf);
        return;
    }

    if (starts_with(rx_line_buf, "FOVR:"))
    {
        const char *value_str = rx_line_buf + 5;
        int percent = atoi(value_str);

        if (percent < FEED_OVERRIDE_MIN)
            percent = FEED_OVERRIDE_MIN;

        if (percent > FEED_OVERRIDE_MAX)
            percent = FEED_OVERRIDE_MAX;

        app_proc_res_t res = app_set_feed_override((uint16_t)percent);

        if (res.res == APP_RES_OK)
        {
            snprintf(tx_buf, sizeof(tx_buf), "OK:FOVR [%d]\r\n", percent);
        }
        else
        {
            snprintf(tx_buf, sizeof(tx_buf), "ERR:FOVR [%d]\r\n", percent);
        }

        usb_send_text(tx_buf);
        return;
    }
    if (starts_with(rx_line_buf, "SOVR:"))
    {
        const char *value_str = rx_line_buf + 5;
        int percent = atoi(value_str);

        if (percent < SPINDLE_OVERRIDE_MIN)
            percent = SPINDLE_OVERRIDE_MIN;

        if (percent > SPINDLE_OVERRIDE_MAX)
            percent = SPINDLE_OVERRIDE_MAX;

        app_proc_res_t res = app_set_spindle_override((uint16_t)percent);

        if (res.res == APP_RES_OK)
        {
            snprintf(tx_buf, sizeof(tx_buf), "OK:SOVR [%d]\r\n", percent);
        }
        else
        {
            snprintf(tx_buf, sizeof(tx_buf), "ERR:SOVR [%d]\r\n", percent);
        }

        usb_send_text(tx_buf);
        return;
    }

    if (strcmp(rx_line_buf, "RUN;") == 0)
    {
        logger_clear();
        logger_arm_ms(LOG_ARM_ON_RUN_MS);

        app_proc_res_t res = app_run_program();

        if (res.res == APP_RES_OK)
            usb_send_text("OK:RUN\r\n");
        else
            usb_send_text("ERR:STATE [RUN;]\r\n");

        return;
    }

    if (strcmp(rx_line_buf, "PAUSE;") == 0)
    {
        app_proc_res_t res = app_pause_program();

        if (res.res == APP_RES_OK)
            usb_send_text("OK:PAUSE\r\n");
        else
            usb_send_text("ERR:STATE [PAUSE;]\r\n");

        return;
    }

    if (strcmp(rx_line_buf, "RESUME;") == 0)
    {
        app_proc_res_t res = app_resume_program();

        if (res.res == APP_RES_OK)
            usb_send_text("OK:RESUME\r\n");
        else
            usb_send_text("ERR:STATE [RESUME;]\r\n");

        return;
    }

    if (strcmp(rx_line_buf, "STOP;") == 0)
    {
        app_proc_res_t res = app_stop_program();

        if (res.res == APP_RES_OK)
            usb_send_text("OK:STOP\r\n");
        else
            usb_send_text("ERR:STATE [STOP;]\r\n");

        return;
    }

    if (strcmp(rx_line_buf, "CLEAR;") == 0)
    {
        app_proc_res_t res = app_clear_program();

        if (res.res == APP_RES_OK)
            usb_send_text("OK:CLEAR\r\n");
        else
            usb_send_text("ERR:STATE [CLEAR;]\r\n");

        return;
    }

    if (strcmp(rx_line_buf, "STATUS;") == 0)
    {
        app_pm_state_t st = app_get_program_state();
        uint16_t count = app_get_program_count();
        uint16_t free_slots = app_get_program_free();
        uint32_t index = app_get_program_index();

        const char *st_txt = "UNKNOWN";

        switch (st)
        {
            case APP_PM_IDLE:    st_txt = "IDLE";    break;
            case APP_PM_RUNNING: st_txt = "RUNNING"; break;
            case APP_PM_PAUSED:  st_txt = "PAUSED";  break;
            case APP_PM_ERROR:   st_txt = "ERROR";   break;
            default: break;
        }

        snprintf(tx_buf, sizeof(tx_buf),
                 "STATUS:%s COUNT=%u INDEX=%lu FREE=%u\r\n",
                 st_txt,
                 (unsigned)count,
                 (unsigned long)index,
                 (unsigned)free_slots);

        usb_send_text(tx_buf);
        return;
    }

    if (strcmp(rx_line_buf, "LOGCLR;") == 0)
    {
        logger_clear();
        usb_send_text("OK:LOGCLR\r\n");
        return;
    }

    if (strcmp(rx_line_buf, "LOGS;") == 0)
    {
        const char *log_buf = logger_get_buffer();

        if (log_buf == 0 || log_buf[0] == '\0')
        {
            usb_send_text("LOGS:EMPTY\r\n");
        }
        else
        {
            usb_send_text(log_buf);
            usb_send_text("\r\n");
        }

        return;
    }

    if (strcmp(rx_line_buf, "RTSTATUS;") == 0)
    {
        const rt_status_t *rt = ipc_cm7_get_status();

        if (rt == 0)
        {
            usb_send_text("ERR:RTSTATUS\r\n");
            return;
        }

        snprintf(tx_buf, sizeof(tx_buf),
                 "RTSTATUS:"
                 "X=%ld;Y=%ld;Z=%ld;"
                 "XU=%ld;YU=%ld;ZU=%ld;"
                 "TX=%ld;TY=%ld;TZ=%ld;"
                 "TXU=%ld;TYU=%ld;TZU=%ld;"
                 "RXR=%ld;RYR=%ld;RZR=%ld;"
                 "RXRU=%ld;RYRU=%ld;RZRU=%ld;"
                 "BF=%lu;EF=%lu;FOVR=%u;"
                 "SPON=%u;SBRPM=%ld;SERPM=%ld;SOVR=%u;SDIR=%u;"
                 "BUSY=%u;ALARM=%u;ESTOP=%u;HX=%u;HY=%u;HZ=%u;ALL=%u\r\n",

                 (long)rt->position_steps[0],
                 (long)rt->position_steps[1],
                 (long)rt->position_steps[2],

                 (long)rt->position_um[0],
                 (long)rt->position_um[1],
                 (long)rt->position_um[2],

                 (long)rt->target_steps[0],
                 (long)rt->target_steps[1],
                 (long)rt->target_steps[2],

                 (long)rt->target_um[0],
                 (long)rt->target_um[1],
                 (long)rt->target_um[2],

                 (long)rt->remaining_steps[0],
                 (long)rt->remaining_steps[1],
                 (long)rt->remaining_steps[2],

                 (long)rt->remaining_um[0],
                 (long)rt->remaining_um[1],
                 (long)rt->remaining_um[2],

                 (unsigned long)rt->base_feed,
                 (unsigned long)rt->effective_feed,
                 (unsigned)rt->feed_override_percent,

                 (unsigned)rt->spindle_on,
                 (long)rt->spindle_base_rpm,
                 (long)rt->spindle_effective_rpm,
                 (unsigned)rt->spindle_override_percent,
                 (unsigned)rt->spindle_dir_cw,

                 (unsigned)rt->busy,
                 (unsigned)rt->alarm,
                 (unsigned)rt->estop,
                 (unsigned)rt->homed[0],
                 (unsigned)rt->homed[1],
                 (unsigned)rt->homed[2],
                 (unsigned)rt->all_homed);

        usb_send_text(tx_buf);
        return;
    }

    snprintf(tx_buf, sizeof(tx_buf), "ERR:UNKNOWN [%s]\r\n", rx_line_buf);
    usb_send_text(tx_buf);
}

static uint8_t starts_with(const char *text, const char *prefix)
{
    if (text == NULL || prefix == NULL)
    {
        return 0;
    }

    while (*prefix != '\0')
    {
        if (*text != *prefix)
        {
            return 0;
        }

        text++;
        prefix++;
    }

    return 1;
}
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS,
  CDC_TransmitCplt_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  /* USER CODE BEGIN 3 */
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 5 */
  switch(cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:
    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:
    break;

    case CDC_SET_COMM_FEATURE:
    break;

    case CDC_GET_COMM_FEATURE:
    break;

    case CDC_CLEAR_COMM_FEATURE:
    break;

    case CDC_SET_LINE_CODING:
    break;

    case CDC_GET_LINE_CODING:
    break;

    case CDC_SET_CONTROL_LINE_STATE:
    break;

    case CDC_SEND_BREAK:
    break;

    default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 6 */

  for (uint32_t i = 0; i < *Len; i++)
  {
      uint8_t c = Buf[i];

      if (c == '\r' || c == '\n')
      {
          if (rx_line_idx > 0)
          {
              rx_line_buf[rx_line_idx] = '\0';
              rx_line_ready = 1;
          }
      }
      else
      {
          if (rx_line_idx < (sizeof(rx_line_buf) - 1))
          {
              rx_line_buf[rx_line_idx++] = c;
          }
          else
          {
              rx_line_idx = 0;
              rx_line_buf[0] = '\0';
              usb_send_text("ERR:OVF\r\n");
          }
      }
  }

  if (rx_line_ready)
  {
      process_received_line();

      rx_line_idx = 0;
      rx_line_ready = 0;
      rx_line_buf[0] = '\0';
  }

  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  CDC_Transmit_FS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 7 */
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  if (hcdc->TxState != 0)
  {
    return USBD_BUSY;
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  /* USER CODE END 7 */
  return result;
}

/**
  * @brief  CDC_TransmitCplt_FS
  *         Data transmitted callback
  *
  *         @note
  *         This function is IN transfer complete callback used to inform user that
  *         the submitted Data is successfully sent over USB.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 13 */
  UNUSED(Buf);
  UNUSED(Len);
  UNUSED(epnum);
  /* USER CODE END 13 */
  return result;
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */
