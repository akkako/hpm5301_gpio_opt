#ifndef SETTING_H
#define SETTING_H

#include <stdbool.h>
#include <stdint.h>
#include "board.h"

typedef struct {
    double vref;
    bool power_on;
    bool port_on;
} Setting_Power_t;

typedef enum {
    RESET_NRST = 0,
    RESET_POR,
    RESET_ARM_SWD_SOFT,
} Setting_ResetBit_t;

#define SETTING_CLEAR_RESET_MODE(reset, mode) (reset &= ~(1 << mode))
#define SETTING_SET_RESET_MODE(reset, mode) (reset |= (1 << mode))
#define SETTING_GET_RESET_MODE(reset, mode) (reset & (1 << mode))

typedef struct {
    uint32_t magic; // 这个段未初始化，使用magic来判断是否已经初始化
    union {
        uint8_t data0;
        struct {
            uint8_t keep_bootloader : 1; // 判断是否需要进入bootloader
            uint8_t fail_cnt: 3;    // 启动失败次数
        };
    };
} BL_Setting_t;

typedef struct {
    uint32_t magic;
    Setting_Power_t power;
    uint8_t reset; //这是一个Bitmap，用来存储多种设置，每一位的功能见Setting_ResetBit_t
    bool jtag_20pin_compatible;
} HSLink_Setting_t;

typedef struct {
    bool reset_level;
} HSLink_Lazy_t;

static const uint32_t SETTING_MAGIC = 0xB7B7B7B7;

extern HSLink_Setting_t HSLink_Setting;
extern BL_Setting_t bl_setting;

extern HSLink_Lazy_t HSLink_Global;

#ifdef __cplusplus

extern "C"
{
#endif

void Setting_Init(void);

#ifdef __cplusplus
}
#endif

#endif //SETTING_H
