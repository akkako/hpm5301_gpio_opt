/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        SW_DP.c CMSIS-DAP SW DP I/O
 *
 *---------------------------------------------------------------------------*/

#include "hpm_interrupt.h"
#include "DAP_config.h"
#include "DAP.h"
#include "SW_DP.h"

#define COMPILER_BARRIER() asm volatile("" :: \
                                            : "memory")

#define PIN_SWCLK_BITFLD (27)
#define PIN_SWDIO_IN_BITFLD (28)
#define PIN_SWDIO_BITFLD (29)
#define PIN_SWDIR_BITFLD (30)

#define PIN_SWCLK_OFFSET (1 << PIN_SWCLK_BITFLD)
#define PIN_SWDIO_OFFSET (1 << PIN_SWDIO_BITFLD)

// SW Macros

#define PIN_SWCLK_SET PIN_SWCLK_TCK_SET
#define PIN_SWCLK_CLR PIN_SWCLK_TCK_CLR

// #define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)
#define PIN_DELAY() PIN_DELAY_FAST()

static inline void SW_CLOCK_CYCLE()
{
  COMPILER_BARRIER();
  PIN_GPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
  COMPILER_BARRIER();
  __asm volatile("nop");
  __asm volatile("nop");
  COMPILER_BARRIER();
  PIN_GPIO->DO[0].SET = PIN_SWCLK_OFFSET;
  COMPILER_BARRIER();
  __asm volatile("nop");
  __asm volatile("nop");
}

static inline void SW_WRITE_BIT1(uint8_t bit)
{
  register uint32_t data = bit & 0x01;
  register uint32_t dest_addr = (uint32_t)(&PIN_GPIO->DO[0].CLEAR) - (data)*4;
  *(volatile uint32_t *)dest_addr = PIN_SWDIO_OFFSET;
  COMPILER_BARRIER();
  PIN_GPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
  COMPILER_BARRIER();
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  COMPILER_BARRIER();
  PIN_GPIO->DO[0].SET = PIN_SWCLK_OFFSET;
  COMPILER_BARRIER();
  __asm volatile("nop");
  __asm volatile("nop");
}

static inline void SW_WRITE_BIT(uint8_t bit)
{
  PIN_GPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) | ((bit & 0x01) << PIN_SWDIO_BITFLD);
  __asm volatile("nop");
  __asm volatile("nop");
  // __asm volatile("nop");
  // __asm volatile("nop");
  PIN_GPIO->DO[0].SET = PIN_SWCLK_OFFSET;
  __asm volatile("nop");
  __asm volatile("nop");
  // __asm volatile("nop");
}

static inline uint8_t SW_READ_BIT()
{
  uint32_t bit;
  COMPILER_BARRIER();
  PIN_GPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
  COMPILER_BARRIER();
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  COMPILER_BARRIER();
  bit = PIN_GPIO->DI[0].VALUE;
  COMPILER_BARRIER();
  PIN_GPIO->DO[0].SET = PIN_SWCLK_OFFSET;
  COMPILER_BARRIER();
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  return (bit >> 29) & 1;
}
#if 0
static inline uint8_t SW_READ_BIT_OPT()
{
  uint32_t bit;
  PIN_GPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  bit = PIN_GPIO->DI[0].VALUE;
  PIN_GPIO->DO[0].SET = PIN_SWCLK_OFFSET;

  __asm volatile("nop");
  __asm volatile("nop");
  __asm volatile("nop");
  return (bit >> 28) & 1;
}
#endif

static inline uint8_t GetParity(uint32_t data)
{
  data ^= data >> 16;
  data ^= data >> 8;
  data ^= data >> 4;
  data &= 0x0F;
  return (0x6996 >> data) & 1;
}

#if (DAP_SWD != 0)
void IO_PORT_SWD_SETUP(void)
{
  // set pin as gpio
  HPM_IOC->PAD[PIN_TCK_SLV].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0) |
                                       IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
  HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
  HPM_IOC->PAD[PIN_SRST].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

  // set driver strenth and pull up
  HPM_IOC->PAD[PIN_TCK_SLV].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(0) | // hysteresis disable
                                      IOC_PAD_PAD_CTL_PRS_SET(0) | // pull-up 100k
                                      IOC_PAD_PAD_CTL_PS_SET(1) |  // pull-up
                                      IOC_PAD_PAD_CTL_PE_SET(1) |  // pull enable
                                      IOC_PAD_PAD_CTL_KE_SET(0) |  // keeper disable
                                      IOC_PAD_PAD_CTL_OD_SET(0) |  // open drain disable
                                      IOC_PAD_PAD_CTL_SR_SET(1) |  // Fast slew rate
                                      IOC_PAD_PAD_CTL_SPD_SET(3) | // Fastest slew rate
                                      IOC_PAD_PAD_CTL_DS_SET(4);   // drive strength 39 ohm(3.3V)

  HPM_IOC->PAD[PIN_TMS].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(0) | // hysteresis disable
                                  IOC_PAD_PAD_CTL_PRS_SET(0) | // pull-up 100k
                                  IOC_PAD_PAD_CTL_PS_SET(1) |  // pull-up
                                  IOC_PAD_PAD_CTL_PE_SET(1) |  // pull enable
                                  IOC_PAD_PAD_CTL_KE_SET(0) |  // keeper disable
                                  IOC_PAD_PAD_CTL_OD_SET(0) |  // open drain disable
                                  IOC_PAD_PAD_CTL_SR_SET(1) |  // Fast slew rate
                                  IOC_PAD_PAD_CTL_SPD_SET(3) | // Fastest slew rate
                                  IOC_PAD_PAD_CTL_DS_SET(4);   // drive strength 39 ohm(3.3V)

  HPM_IOC->PAD[PIN_TMS_IN].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(0) | // hysteresis disable
                                     IOC_PAD_PAD_CTL_PRS_SET(0) | // pull-up 100k
                                     IOC_PAD_PAD_CTL_PS_SET(1) |  // pull-up
                                     IOC_PAD_PAD_CTL_PE_SET(1) |  // pull enable
                                     IOC_PAD_PAD_CTL_KE_SET(0) |  // keeper disable
                                     IOC_PAD_PAD_CTL_OD_SET(0) |  // open drain disable
                                     IOC_PAD_PAD_CTL_SR_SET(1) |  // Fast slew rate
                                     IOC_PAD_PAD_CTL_SPD_SET(3) | // Fastest slew rate
                                     IOC_PAD_PAD_CTL_DS_SET(4);   // drive strength 39 ohm(3.3V)

  HPM_IOC->PAD[PIN_SRST].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2) |
                                   IOC_PAD_PAD_CTL_PE_SET(1) |
                                   IOC_PAD_PAD_CTL_PS_SET(1) |
                                   IOC_PAD_PAD_CTL_SPD_SET(3);

  gpiom_configure_pin_control_setting(PIN_TCK_SLV);
  gpiom_configure_pin_control_setting(PIN_TMS_IN);
  gpiom_configure_pin_control_setting(PIN_TMS);
  gpiom_configure_pin_control_setting(PIN_SRST);

  gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TCK_SLV), GPIO_GET_PIN_INDEX(PIN_TCK_SLV));
  gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
  gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_SRST), GPIO_GET_PIN_INDEX(PIN_SRST));

  gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1); // 默认SWDIO为输出
  gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_SRST), GPIO_GET_PIN_INDEX(PIN_SRST), !HSLink_Global.reset_level);
}
#endif

// Generate SWJ Sequence
//   count:  sequence bit count
//   data:   pointer to sequence bit data
//   return: none
#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
void IO_SWJ_Sequence(uint32_t count, const uint8_t *data)
{
  uint32_t val;
  uint32_t n;

  val = 0U;
  n = 0U;
  while (count--)
  {
    if (n == 0U)
    {
      val = *data++;
      n = 8U;
    }
    if (val & 1U)
    {
      PIN_SWDIO_TMS_SET();
    }
    else
    {
      PIN_SWDIO_TMS_CLR();
    }
    SW_CLOCK_CYCLE();
    val >>= 1;
    n--;
  }
}
#endif

// Generate SWD Sequence
//   info:   sequence information
//   swdo:   pointer to SWDIO generated data
//   swdi:   pointer to SWDIO captured data
//   return: none
#if (DAP_SWD != 0)
void IO_SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
  uint32_t val;
  uint32_t bit;
  uint32_t n, k;

  n = info & SWD_SEQUENCE_CLK;
  if (n == 0U)
  {
    n = 64U;
  }

  if (info & SWD_SEQUENCE_DIN)
  {
    while (n)
    {
      val = 0U;
      for (k = 8U; k && n; k--, n--)
      {
        bit = SW_READ_BIT();
        val >>= 1;
        val |= bit << 7;
      }
      val >>= k;
      *swdi++ = (uint8_t)val;
    }
  }
  else
  {
    while (n)
    {
      val = *swdo++;
      for (k = 8U; k && n; k--, n--)
      {
        SW_WRITE_BIT(val);
        val >>= 1;
      }
    }
  }
}
#endif

#if (DAP_SWD != 0)

#define WRITE_BIT(x) \
  SW_WRITE_BIT(x);   \
  x >>= 1

#define READ_ACK()                                          \
  COMPILER_BARRIER();                                       \
  PIN_GPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;                 \
  COMPILER_BARRIER();                                       \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  COMPILER_BARRIER();                                       \
  bit = PIN_GPIO->DI[0].VALUE;                              \
  COMPILER_BARRIER();                                       \
  COMPILER_BARRIER();                                       \
  PIN_GPIO->DO[0].SET = PIN_SWCLK_OFFSET;                   \
  COMPILER_BARRIER();                                       \
  __asm__ __volatile__(                                     \
      "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */          \
      "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */            \
      "srli  %1, %1, 1\n\t"  /* val = val >> 1 */           \
      "slli  %0, %0, 31\n\t" /* bit = bit << 31 */          \
      "or    %1, %1, %0\n\t" /* val = val | bit */          \
      : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */ \
      :                                                     \
      : "memory")

#define READ_BIT()                                          \
  COMPILER_BARRIER();                                       \
  PIN_GPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;                 \
  COMPILER_BARRIER();                                       \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  __asm volatile("nop");                                    \
  COMPILER_BARRIER();                                       \
  bit = PIN_GPIO->DI[0].VALUE;                              \
  COMPILER_BARRIER();                                       \
  COMPILER_BARRIER();                                       \
  PIN_GPIO->DO[0].SET = PIN_SWCLK_OFFSET;                   \
  COMPILER_BARRIER();                                       \
  __asm__ __volatile__(                                     \
      "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */          \
      "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */            \
      "srli  %1, %1, 1\n\t"  /* val = val >> 1 */           \
      "slli  %0, %0, 31\n\t" /* bit = bit << 31 */          \
      "or    %1, %1, %0\n\t" /* val = val | bit */          \
      : "+r"(bit), "+r"(val) /* bit 和 val 都是读写 */ \
      :                                                     \
      : "memory")

#define REPEAT_8(x) \
  x;                \
  x;                \
  x;                \
  x;                \
  x;                \
  x;                \
  x;                \
  x

#define REPEAT_7(x) \
  x;                \
  x;                \
  x;                \
  x;                \
  x;                \
  x;                \
  x;

static uint8_t SWD_Read_Opt(uint8_t header, uint32_t *data)
{
  uint32_t ack = 0;
  uint32_t bit;
  uint32_t val;
  uint32_t parity;

  uint32_t n;

  /* Packet Request */
  SW_WRITE_BIT(header); /* Start Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* APnDP Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* RnW Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* A2 Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* A3 Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* Parity Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* Stop Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* Park Bit */
  __asm volatile("nop");
  /* Turnaround */
  PIN_SWDIO_OUT_DISABLE();
  n = DAP_Data.swd_conf.turnaround;
  do
  {
    SW_CLOCK_CYCLE();
    n--;
  } while (n);

  // uint32_t irq_status = disable_global_irq(CSR_MSTATUS_MIE_MASK);

  /* Acknowledge response */
  bit = SW_READ_BIT();
  ack = bit << 0;
  bit = SW_READ_BIT();
  ack |= bit << 1;
  bit = SW_READ_BIT();
  ack |= bit << 2;
  // READ_ACK();
  // READ_ACK();
  // READ_ACK();
  // ack >>= 29;

  if (ack == DAP_TRANSFER_OK)
    goto label_ok;
  else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    goto label_wait_fault;
  else
    goto label_error;

label_ok:
  val = 0U;
  /* 保存并关闭 Machine 模式总中断 */
  REPEAT_8(READ_BIT());
  REPEAT_8(READ_BIT());
  REPEAT_8(READ_BIT());
  REPEAT_8(READ_BIT());

  bit = SW_READ_BIT(); /* Read Parity */

  parity = GetParity(val);

  if ((parity ^ bit) & 1U)
  {
    printf("data=%x,par=%d,bit=%d\r\n", val, parity, bit);
    ack = DAP_TRANSFER_ERROR;
  }
  if (data)
  {
    *data = val;
  }

  PIN_SWDIO_OUT_ENABLE();

  /* Turnaround */
  for (n = DAP_Data.swd_conf.turnaround; n; n--)
  {
    SW_CLOCK_CYCLE();
  }
  PIN_SWDIO_OUT(1U);

  /* Capture Timestamp */
  // if (request & DAP_TRANSFER_TIMESTAMP)
  // {
  //   DAP_Data.timestamp = TIMESTAMP_GET();
  // }
  /* Idle cycles */
  n = DAP_Data.transfer.idle_cycles;
  if (n)
  {
    PIN_SWDIO_OUT(0U);
    for (; n; n--)
    {
      SW_CLOCK_CYCLE();
    }
  }
  PIN_SWDIO_OUT(1U);
  return ((uint8_t)ack);

label_wait_fault:
  /* WAIT or FAULT response */
  if (DAP_Data.swd_conf.data_phase)
  {
    for (n = 32U + 1U; n; n--)
    {
      SW_CLOCK_CYCLE(); /* Dummy Read RDATA[0:31] + Parity */
    }
  }
  /* Turnaround */
  n = DAP_Data.swd_conf.turnaround;
  do
  {
    SW_CLOCK_CYCLE();
    n--;
  } while (n);
  PIN_SWDIO_OUT_ENABLE();
  if (DAP_Data.swd_conf.data_phase)
  {
    PIN_SWDIO_OUT(0U);
    for (n = 32U + 1U; n; n--)
    {
      SW_CLOCK_CYCLE(); /* Dummy Write WDATA[0:31] + Parity */
    }
  }
  PIN_SWDIO_OUT(1U);
  // printf("ack:%d\r\n", ack);
  return ((uint8_t)ack);

label_error:
  /* Protocol error */
  for (n = DAP_Data.swd_conf.turnaround + 32U + 1U; n; n--)
  {
    SW_CLOCK_CYCLE(); /* Back off data phase */
  }
  PIN_SWDIO_OUT_ENABLE();
  PIN_SWDIO_OUT(1U);
  // printf("ack:%d\r\n", ack);
  return ((uint8_t)ack);
}

static uint8_t SWD_Write_Opt(uint8_t header, uint32_t *data)
{
  uint32_t ack = 0;
  uint32_t bit;
  uint32_t val;
  uint32_t parity;
  uint32_t n;

  /* Packet Request */
  SW_WRITE_BIT(header); /* Start Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* APnDP Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* RnW Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* A2 Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* A3 Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* Parity Bit */
  __asm volatile("nop");
  header >>= 1;
  SW_WRITE_BIT(header); /* Stop Bit */
  __asm volatile("nop");
  header >>= 1;
  SW_WRITE_BIT(header); /* Park Bit */
  __asm volatile("nop");

  /* Turnaround */
  PIN_SWDIO_OUT_DISABLE();
  n = DAP_Data.swd_conf.turnaround;
  do
  {
    SW_CLOCK_CYCLE();
    n--;
  } while (n);
  uint32_t irq_status = disable_global_irq(CSR_MSTATUS_MIE_MASK);

  /* Acknowledge response */
  // bit = SW_READ_BIT();
  // ack = bit << 0;
  // bit = SW_READ_BIT();
  // ack |= bit << 1;
  // bit = SW_READ_BIT();
  // ack |= bit << 2;

  READ_ACK();
  READ_ACK();
  READ_ACK();
  ack >>= 29;
  restore_global_irq(irq_status);

  if (ack == DAP_TRANSFER_OK)
  {
    /* Turnaround */
    n = DAP_Data.swd_conf.turnaround;
    do
    {
      SW_CLOCK_CYCLE();
      n--;
    } while (n);

    PIN_SWDIO_OUT_ENABLE();
    /* Write data */
    val = *data;
    parity = GetParity(val);

    REPEAT_8(WRITE_BIT(val));
    REPEAT_8(WRITE_BIT(val));
    REPEAT_8(WRITE_BIT(val));
    REPEAT_8(WRITE_BIT(val));

    SW_WRITE_BIT(parity); /* Write Parity Bit */
    __asm volatile("nop");

    /* Capture Timestamp */
    // if (request & DAP_TRANSFER_TIMESTAMP)
    // {
    //   DAP_Data.timestamp = TIMESTAMP_GET();
    // }
    /* Idle cycles */
    n = DAP_Data.transfer.idle_cycles;
    if (n)
    {
      PIN_SWDIO_OUT(0U);
      for (; n; n--)
      {
        SW_CLOCK_CYCLE();
      }
    }
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
  }

  else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
  {
    /* WAIT or FAULT response */
    /* Turnaround */
    n = DAP_Data.swd_conf.turnaround;
    do
    {
      SW_CLOCK_CYCLE();
      n--;
    } while (n);
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
  }
  else
  {
    /* Protocol error */
    for (n = DAP_Data.swd_conf.turnaround + 32U + 1U; n; n--)
    {
      SW_CLOCK_CYCLE(); /* Back off data phase */
    }
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
  }
}

static uint8_t SWD_Write_Opt2(uint8_t header, uint32_t *data)
{
  uint32_t ack = 0;
  uint32_t bit;
  uint32_t val;
  uint32_t parity;
  uint32_t n;

  /* Packet Request */
  SW_WRITE_BIT(header); /* Start Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* APnDP Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* RnW Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* A2 Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* A3 Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* Parity Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* Stop Bit */
  header >>= 1;
  SW_WRITE_BIT(header); /* Park Bit */
  __asm volatile("nop");

  /* Turnaround */
  PIN_SWDIO_OUT_DISABLE();
  n = DAP_Data.swd_conf.turnaround;
  do
  {
    SW_CLOCK_CYCLE();
    n--;
  } while (n);
  __asm volatile("nop");
  __asm volatile("nop");

  /* Acknowledge response */
  // bit = SW_READ_BIT();
  // ack = bit << 0;
  // bit = SW_READ_BIT();
  // ack |= bit << 1;
  // bit = SW_READ_BIT();
  // ack |= bit << 2;

  READ_ACK();
  READ_ACK();
  READ_ACK();
  ack >>= 29;

  if (ack == DAP_TRANSFER_OK)
  {
    /* Turnaround */
    n = DAP_Data.swd_conf.turnaround;
    do
    {
      SW_CLOCK_CYCLE();
      n--;
    } while (n);

    PIN_SWDIO_OUT_ENABLE();
    /* Write data */
    val = *data;
    parity = GetParity(val);

    // REPEAT_8(WRITE_BIT(val));
    // REPEAT_8(WRITE_BIT(val));
    // REPEAT_8(WRITE_BIT(val));
    // REPEAT_8(WRITE_BIT(val));

            /*********************** Data Bit 0 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 1 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 2 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 3 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 4 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 5 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 6 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 7 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 8 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 9 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 10 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 11 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 12 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 13 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 14 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 15 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 16 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 17 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 18 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 19 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 20 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 21 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 22 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 23 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 24 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 25 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 26 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 27 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 28 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 29 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 30 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 31 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        // __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        // __asm volatile("nop");

    SW_WRITE_BIT(parity); /* Write Parity Bit */

    /* Capture Timestamp */
    // if (request & DAP_TRANSFER_TIMESTAMP)
    // {
    //   DAP_Data.timestamp = TIMESTAMP_GET();
    // }
    /* Idle cycles */
    n = DAP_Data.transfer.idle_cycles;
    if (n)
    {
      PIN_SWDIO_OUT(0U);
      for (; n; n--)
      {
        SW_CLOCK_CYCLE();
      }
    }
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
  }

  else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
  {
    /* WAIT or FAULT response */
    /* Turnaround */
    n = DAP_Data.swd_conf.turnaround;
    do
    {
      SW_CLOCK_CYCLE();
      n--;
    } while (n);
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
  }
  else
  {
    /* Protocol error */
    for (n = DAP_Data.swd_conf.turnaround + 32U + 1U; n; n--)
    {
      SW_CLOCK_CYCLE(); /* Back off data phase */
    }
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
  }
}

uint8_t SWD_Read(uint8_t header, uint32_t *data)
{
  if (DAP_Data.fast_clock)
  {
    return SWD_Read_Opt(header, data);
  }
  else
  {
    return SWD_Read_Opt(header, data);
  }
}

uint8_t SWD_Write(uint8_t header, uint32_t *data)
{
  if (DAP_Data.fast_clock)
  {
    // return SWD_Write_Opt(header, data);
    // return SWD_Write_Opt2(header, data);
    return SWD_Write_Opt1(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
  else
  {
    // return SWD_Write_Opt(header, data);
    // return SWD_Write_Opt2(header, data);
    return SWD_Write_Opt1(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
}

#endif /* (DAP_SWD != 0) */
