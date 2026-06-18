#include "expansion.h"
#include "board.h"
#include "dap_main.h"
#include "setting.h"
#include "usb2uart.h"
#include <hid_comm.h>
#include <hpm_dma_mgr.h>
#include <hpm_ewdg_drv.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include <hpm_romapi.h>

static void serial_number_init(void)
{
#define OTP_CHIP_UUID_IDX_START (88U)
#define OTP_CHIP_UUID_IDX_END (91U)
    uint32_t uuid_words[4];

    uint32_t word_idx = 0;
    for (uint32_t i = OTP_CHIP_UUID_IDX_START; i <= OTP_CHIP_UUID_IDX_END; i++)
    {
        uuid_words[word_idx++] = ROM_API_TABLE_ROOT->otp_driver_if->read_from_shadow(i);
    }

    sprintf(serial_number_dynamic, "%08X%08X%08X%08X", uuid_words[0], uuid_words[1], uuid_words[2], uuid_words[3]);
    printf("Serial number: %s\n", serial_number_dynamic);
}

ATTR_ALWAYS_INLINE
static inline void SWDIO_DIR_Init(void)
{
    HPM_IOC->PAD[SWDIO_DIR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[SWDIO_DIR].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(0) | // hysteresis disable
                                      IOC_PAD_PAD_CTL_PRS_SET(0) | // pull-up 100k
                                      IOC_PAD_PAD_CTL_PS_SET(1) |  // pull-up
                                      IOC_PAD_PAD_CTL_PE_SET(1) |  // pull enable
                                      IOC_PAD_PAD_CTL_KE_SET(0) |  // keeper disable
                                      IOC_PAD_PAD_CTL_OD_SET(0) |  // open drain disable
                                      IOC_PAD_PAD_CTL_SR_SET(1) |  // Fast slew rate
                                      IOC_PAD_PAD_CTL_SPD_SET(3) | // Fastest slew rate
                                      IOC_PAD_PAD_CTL_DS_SET(4);   // drive strength 39 ohm(3.3V)

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), PIN_GPIOM);
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}

static void EWDG_Init()
{
    clock_add_to_group(clock_watchdog0, 0);
    ewdg_config_t config;
    ewdg_get_default_config(HPM_EWDG0, &config);

    config.enable_watchdog = true;
    config.int_rst_config.enable_timeout_reset = true;
    config.ctrl_config.use_lowlevel_timeout = false;
    config.ctrl_config.cnt_clk_sel = ewdg_cnt_clk_src_ext_osc_clk;

    /* Set the EWDG reset timeout to 5 second */
    config.cnt_src_freq = 32768;
    config.ctrl_config.timeout_reset_us = 5 * 1000 * 1000;

    /* Initialize the WDG */
    hpm_stat_t status = ewdg_init(HPM_EWDG0, &config);
    if (status != status_success)
    {
        printf(" EWDG initialization failed, error_code=%d\n", status);
    }
}

int main()
{
    board_init();
    EWDG_Init();
    board_delay_ms(500);
    serial_number_init();
    board_init_usb(HPM_USB0);
    dma_mgr_init();

    SWDIO_DIR_Init();

    Setting_Init();

    HSP_Init();
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 5);
    uartx_preinit();
    chry_dap_init(0, HPM_USB0_BASE);

    bl_setting.fail_cnt = 0;

    while (true)
    {
        ewdg_refresh(HPM_EWDG0);
        chry_dap_handle();
        chry_dap_usb2uart_handle();
        usb2uart_handler();
        HSP_Loop();
    }
}
