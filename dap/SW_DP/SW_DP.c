#include "SW_DP.h"
#include "DAP.h"
#include "setting.h"

void PORT_SWD_SETUP(void)
{
  SPI_PORT_SWD_SETUP();
  // IO_PORT_SWD_SETUP();
}

void SWJ_Sequence(uint32_t count, const uint8_t *data)
{
  if (DAP_Data.debug_port == DAP_PORT_JTAG)
  {
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1); // SWDIO 输出
    PIN_SWDIO_TMS_SET();
    // 切换为GPIO控制器
    HPM_IOC->PAD[PIN_TCK_SLV].FUNC_CTL =
        IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0) | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK; /* as gpio*/
    HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);      /* as gpio*/
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TCK_SLV), GPIO_GET_PIN_INDEX(PIN_TCK_SLV));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
    IO_SWJ_Sequence(count, data);

    // 切换为JTAG控制器
    HPM_IOC->PAD[PIN_TCK].FUNC_CTL =
        IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5) | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1); /* as spi sck*/
    return;
  }

  SPI_SWJ_Sequence(count, data);
  // IO_SWJ_Sequence(count, data);
}

void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
  SPI_SWD_Sequence(info, swdo, swdi);
  // IO_SWD_Sequence(info, swdo, swdi);
}

uint8_t SWD_Read(uint8_t header, uint32_t *data)
{
#if 0
  if (DAP_Data.fast_clock)
  {
    return SWD_Read_Opt(header, data);
  }
  else
  {
    return SWD_Read_Opt(header, data);
  }
#endif
  return SPI_SWD_Read(header, data);
}

uint8_t SWD_Write(uint8_t header, uint32_t *data)
{
#if 0
  if (DAP_Data.clock_speed >= 60000000)
  {
    return SWD_Write_Opt_60M(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
  else if (DAP_Data.clock_speed >= 45000000)
  {
    return SWD_Write_Opt_45M(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
  else if (DAP_Data.clock_speed >= 36000000)
  {
    return SWD_Write_Opt_36M(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
  else if (DAP_Data.clock_speed >= 30000000)
  {
    return SWD_Write_Opt_30M(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
  else if (DAP_Data.clock_speed >= 20000000)
  {
    return SWD_Write_Opt_20M(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
  else
  {
    return SWD_Write_Opt_10M(header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
  }
#endif
  return SPI_SWD_Write(header, data);
}