/*
 * Copyright (c) 2022, sakumisu
 * Copyright (c) 2022-2025, HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

#include "board.h"

/* ================ USB common Configuration ================ */

#ifdef __RTTHREAD__
#include <rtthread.h>

#define CONFIG_USB_PRINTF(...) rt_kprintf(__VA_ARGS__)
#else
#define CONFIG_USB_PRINTF(...) //printf(__VA_ARGS__)
#endif

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#endif

#if defined(CONFIG_USB_DEVICE_FS) || defined(CONFIG_USB_DEVICE_FORCE_FULL_SPEED)
#undef CONFIG_USB_HS
#else
#define CONFIG_USB_HS
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* #define CONFIG_USB_DCACHE_ENABLE */

/* data align size when use dma or use dcache */
#ifdef CONFIG_USB_DCACHE_ENABLE
#define CONFIG_USB_ALIGN_SIZE HPM_L1C_CACHELINE_SIZE
#else
#define CONFIG_USB_ALIGN_SIZE 4
#endif

/* descriptor common define */
// #define USBD_VID           0x34B7 /* HPMicro VID */
// #define USBD_PID           0xFFFF
// #define USBD_MAX_POWER     200

/* attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable.non_init")))

/* use usb_memcpy default for high performance but cost more flash memory.
 * And, arm libc has a bug that memcpy() may cause data misalignment when the size is not a multiple of 4.
*/
/* #define CONFIG_USB_MEMCPY_DISABLE */

/* ================= USB Device Stack Configuration ================ */

/* Ep0 in and out transfer buffer */
#ifndef CONFIG_USBDEV_REQUEST_BUFFER_LEN
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512
#endif

/* Setup packet log for debug */
/* #define CONFIG_USBDEV_SETUP_LOG_PRINT */

/* Send ep0 in data from user buffer instead of copying into ep0 reqdata
 * Please note that user buffer must be aligned with CONFIG_USB_ALIGN_SIZE
 */
/* #define CONFIG_USBDEV_EP0_INDATA_NO_COPY */

/* Check if the input descriptor is correct */
/* #define CONFIG_USBDEV_DESC_CHECK */

/* Enable test mode */
#define CONFIG_USBDEV_TEST_MODE

/* enable advance desc register api */
#define CONFIG_USBDEV_ADVANCE_DESC

/* move ep0 setup handler from isr to thread */
/* #define CONFIG_USBDEV_EP0_THREAD */

#ifndef CONFIG_USBDEV_EP0_PRIO
#define CONFIG_USBDEV_EP0_PRIO 4
#endif

#ifndef CONFIG_USBDEV_EP0_STACKSIZE
#define CONFIG_USBDEV_EP0_STACKSIZE 2048
#endif

#ifndef CONFIG_USBDEV_MSC_MAX_LUN
#define CONFIG_USBDEV_MSC_MAX_LUN 1
#endif

#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512
#endif

#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
#define CONFIG_USBDEV_MSC_PRODUCT_STRING ""
#endif

#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
#define CONFIG_USBDEV_MSC_VERSION_STRING "0.01"
#endif

/* move msc read & write from isr to while(1), you should call usbd_msc_polling in while(1) */
/* #define CONFIG_USBDEV_MSC_POLLING */

/* move msc read & write from isr to thread */
/* #define CONFIG_USBDEV_MSC_THREAD */

#ifndef CONFIG_USBDEV_MSC_PRIO
#define CONFIG_USBDEV_MSC_PRIO 4
#endif

#ifndef CONFIG_USBDEV_MSC_STACKSIZE
#define CONFIG_USBDEV_MSC_STACKSIZE 2048
#endif

#ifndef CONFIG_USBDEV_MTP_MAX_BUFSIZE
#define CONFIG_USBDEV_MTP_MAX_BUFSIZE 2048
#endif

#ifndef CONFIG_USBDEV_MTP_MAX_OBJECTS
#define CONFIG_USBDEV_MTP_MAX_OBJECTS 256
#endif

#ifndef CONFIG_USBDEV_MTP_MAX_PATHNAME
#define CONFIG_USBDEV_MTP_MAX_PATHNAME 256
#endif

#define CONFIG_USBDEV_MTP_THREAD

#ifndef CONFIG_USBDEV_MTP_PRIO
#define CONFIG_USBDEV_MTP_PRIO 4
#endif

#ifndef CONFIG_USBDEV_MTP_STACKSIZE
#define CONFIG_USBDEV_MTP_STACKSIZE 4096
#endif

#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
#define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE 156
#endif

/* rndis transfer buffer size, must be a multiple of (1536 + 44)*/
#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
#define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE 1580
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
#define CONFIG_USBDEV_RNDIS_VENDOR_ID 0x0000ffff
#endif

#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
#define CONFIG_USBDEV_RNDIS_VENDOR_DESC "HPMicro"
#endif

#define CONFIG_USBDEV_RNDIS_USING_LWIP
#define CONFIG_USBDEV_CDC_ECM_USING_LWIP


/* ================ USB Device Port Configuration ================*/

#define CONFIG_USBDEV_MAX_BUS USB_SOC_MAX_COUNT

#ifndef CONFIG_USBDEV_EP_NUM
#define CONFIG_USBDEV_EP_NUM USB_SOC_DCD_MAX_ENDPOINT_COUNT
#endif

#ifndef CONFIG_HPM_USBD_BASE
#define CONFIG_HPM_USBD_BASE HPM_USB0_BASE
#endif
#ifndef CONFIG_HPM_USBD_IRQn
#define CONFIG_HPM_USBD_IRQn IRQn_USB0
#endif


/* ================ Addr Convert Configuration ==================*/
#ifndef usb_phyaddr2ramaddr
#define usb_phyaddr2ramaddr(addr) core_local_mem_to_sys_address(BOARD_RUNNING_CORE, addr)
#endif

#ifndef usb_ramaddr2phyaddr
#define usb_ramaddr2phyaddr(addr) sys_address_to_core_local_mem(BOARD_RUNNING_CORE, addr)
#endif

#endif
