//
// Created by yekai on 2025/8/5.
//

#include "reset_way.h"
#include "DAP_config.h"
#include "DAP.h"
#include "hpm_spi_drv.h"
#include "hpm_clock_drv.h"
#include "swd_host.h"
#include "expansion.h"

// Use the CMSIS-Core definition if available.
#if !defined(SCB_AIRCR_PRIGROUP_Pos)
#define SCB_AIRCR_PRIGROUP_Pos              8U                                            /*!< SCB AIRCR: PRIGROUP Position */
#define SCB_AIRCR_PRIGROUP_Msk             (7UL << SCB_AIRCR_PRIGROUP_Pos)                /*!< SCB AIRCR: PRIGROUP Mask */
#endif

uint8_t software_reset(void) {
    if (DAP_Data.debug_port != DAP_PORT_SWD) {
        return 1;
    }
    uint8_t ret = 0;
    // uint32_t val;
    // ret |= swd_read_word(NVIC_AIRCR, &val);
    // ret |= swd_write_word(NVIC_AIRCR, VECTKEY | (val & SCB_AIRCR_PRIGROUP_Msk) | SYSRESETREQ);
    return ret;
}
