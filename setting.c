#include "setting.h"
#include "board.h"
#include <hpm_romapi.h>

#include "usb2uart.h"

const HSLink_Setting_t default_setting = {
    .power = {
        .power_on = false,
        .port_on = false,
    },
    .reset = 1 << RESET_NRST,
};

HSLink_Setting_t HSLink_Setting = {
    .power = {
        .power_on = false,
        .port_on = false,
    },
    .reset = 0,
};

HSLink_Lazy_t HSLink_Global;

ATTR_PLACE_AT(".bl_setting")
BL_Setting_t bl_setting;


static void load_settings();

static void update_settings();

static void load_settings()
{
    memcpy(&HSLink_Setting, &default_setting, sizeof(HSLink_Setting_t));
    return;
}

static void update_settings()
{
    uartx_io_init();
}

void Setting_Init(void)
{
    load_settings();

    update_settings();

    HSLink_Global.reset_level = 0; // level when reset is active
}
