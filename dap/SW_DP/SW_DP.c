#include "SW_DP.h"
#include "DAP.h"
#include "setting.h"

void PORT_SWD_SETUP(void)
{
    IO_PORT_SWD_SETUP();
}

void SWJ_Sequence(uint32_t count, const uint8_t *data)
{
    if (DAP_Data.debug_port == DAP_PORT_JTAG)
    {
        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR),
                       1); // SWDIO 输出
        PIN_SWDIO_TMS_SET();
        // 切换为GPIO控制器
        HPM_IOC->PAD[PIN_TCK_SLV].FUNC_CTL =
            IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0) | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK; /* as gpio*/
        HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);      /* as gpio*/
        gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TCK_SLV), GPIO_GET_PIN_INDEX(PIN_TCK_SLV));
        gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
        IO_SWJ_Sequence(count, data);

        return;
    }

    IO_SWJ_Sequence(count, data);
}

void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
    IO_SWD_Sequence(info, swdo, swdi);
}
