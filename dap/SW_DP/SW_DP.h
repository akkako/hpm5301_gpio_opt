#ifndef HSLINK_PRO_SW_DP_H
#define HSLINK_PRO_SW_DP_H

#include "DAP_config.h"

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
void IO_SWJ_Sequence(uint32_t count, const uint8_t *data);
#endif

#if (DAP_SWD != 0)
void IO_PORT_SWD_SETUP(void);
void IO_SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi);
uint8_t SWD_Write(uint8_t header, uint32_t *data);
uint8_t SWD_Read(uint8_t header, uint32_t *data);

uint8_t SWD_Write_Opt1(uint8_t header,
                       uint8_t turnaround,
                       uint8_t data_phase,
                       uint8_t idle_cycles,
                       uint32_t *data);
uint8_t SWD_Write_Opt_Asm(uint8_t header,
                      uint8_t turnaround,
                      uint8_t data_phase,
                      uint8_t idle_cycles,
                      uint32_t *data);

#endif

#endif // HSLINK_PRO_SW_DP_H
