#include <stdint.h>
// #include "hpm_interrupt.h"

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define set_csr(csr_num, bit) __asm volatile("csrs %0, %1" \
                                             :             \
                                             : "i"(csr_num), "r"(bit))
#define read_clear_csr(csr_num, bit) ({ volatile uint32_t v = 0; __asm volatile("csrrc %0, %1, %2" : "=r"(v) : "i"(csr_num), "r"(bit)); v; })

static inline void enable_global_irq(uint32_t mask)
{
    set_csr(0x300, mask);
}

static inline uint32_t disable_global_irq(uint32_t mask)
{
    return read_clear_csr(0x300, mask);
}

#define __R volatile const /* Define "read-only" permission */
#define __RW volatile      /* Define "read-write" permission */
#define __W volatile       /* Define "write-only" permission */

typedef struct
{
    struct
    {
        __R uint32_t VALUE;        /* 0x0: GPIO input value */
        __R uint8_t RESERVED0[12]; /* 0x4 - 0xF: Reserved */
    } DI[15];
    __R uint8_t RESERVED0[16]; /* 0xF0 - 0xFF: Reserved */
    struct
    {
        __RW uint32_t VALUE;  /* 0x100: GPIO output value */
        __RW uint32_t SET;    /* 0x104: GPIO output set */
        __RW uint32_t CLEAR;  /* 0x108: GPIO output clear */
        __RW uint32_t TOGGLE; /* 0x10C: GPIO output toggle */
    } DO[15];
    __R uint8_t RESERVED1[16]; /* 0x1F0 - 0x1FF: Reserved */
    struct
    {
        __RW uint32_t VALUE;  /* 0x200: GPIO direction value */
        __RW uint32_t SET;    /* 0x204: GPIO direction set */
        __RW uint32_t CLEAR;  /* 0x208: GPIO direction clear */
        __RW uint32_t TOGGLE; /* 0x20C: GPIO direction toggle */
    } OE[15];
    __R uint8_t RESERVED2[16]; /* 0x2F0 - 0x2FF: Reserved */
    struct
    {
        __W uint32_t VALUE;        /* 0x300: GPIO interrupt flag value */
        __R uint8_t RESERVED0[12]; /* 0x304 - 0x30F: Reserved */
    } IF[15];
    __R uint8_t RESERVED3[16]; /* 0x3F0 - 0x3FF: Reserved */
    struct
    {
        __RW uint32_t VALUE;  /* 0x400: GPIO interrupt enable value */
        __RW uint32_t SET;    /* 0x404: GPIO interrupt enable set */
        __RW uint32_t CLEAR;  /* 0x408: GPIO interrupt enable clear */
        __RW uint32_t TOGGLE; /* 0x40C: GPIO interrupt enable toggle */
    } IE[15];
    __R uint8_t RESERVED4[16]; /* 0x4F0 - 0x4FF: Reserved */
    struct
    {
        __RW uint32_t VALUE;  /* 0x500: GPIO interrupt polarity value */
        __RW uint32_t SET;    /* 0x504: GPIO interrupt polarity set */
        __RW uint32_t CLEAR;  /* 0x508: GPIO interrupt polarity clear */
        __RW uint32_t TOGGLE; /* 0x50C: GPIO interrupt polarity toggle */
    } PL[15];
    __R uint8_t RESERVED5[16]; /* 0x5F0 - 0x5FF: Reserved */
    struct
    {
        __RW uint32_t VALUE;  /* 0x600: GPIO interrupt type value */
        __RW uint32_t SET;    /* 0x604: GPIO interrupt type set */
        __RW uint32_t CLEAR;  /* 0x608: GPIO interrupt type clear */
        __RW uint32_t TOGGLE; /* 0x60C: GPIO interrupt type toggle */
    } TP[15];
    __R uint8_t RESERVED6[16]; /* 0x6F0 - 0x6FF: Reserved */
    struct
    {
        __RW uint32_t VALUE;  /* 0x700: GPIO interrupt asynchronous value */
        __RW uint32_t SET;    /* 0x704: GPIO interrupt asynchronous set */
        __RW uint32_t CLEAR;  /* 0x708: GPIO interrupt asynchronous clear */
        __RW uint32_t TOGGLE; /* 0x70C: GPIO interrupt asynchronous toggle */
    } AS[15];
    __R uint8_t RESERVED7[16]; /* 0x7F0 - 0x7FF: Reserved */
    struct
    {
        __RW uint32_t VALUE;  /* 0x800: GPIO dual edge interrupt enable value */
        __RW uint32_t SET;    /* 0x804: GPIO dual edge interrupt enable set */
        __RW uint32_t CLEAR;  /* 0x808: GPIO dual edge interrupt enable clear */
        __RW uint32_t TOGGLE; /* 0x80C: GPIO dual edge interrupt enable toggle */
    } PD[15];
} GPIO_Type;

/* FGPIO base address */
#define HPM_FGPIO_BASE (0xC0000UL)
/* FGPIO base pointer */
#define HPM_FGPIO ((GPIO_Type *)HPM_FGPIO_BASE)

#define PIN_SWCLK_BITFLD (27)
#define PIN_SWDIO_IN_BITFLD (28)
#define PIN_SWDIO_BITFLD (29)
#define PIN_SWDIR_BITFLD (30)

#define PIN_SWCLK_OFFSET (1 << PIN_SWCLK_BITFLD)
#define PIN_SWDIO_OFFSET (1 << PIN_SWDIO_BITFLD)
#define PIN_SWDIR_OFFSET (1 << PIN_SWDIR_BITFLD)

#define DAP_TRANSFER_OK (1U << 0)
#define DAP_TRANSFER_WAIT (1U << 1)
#define DAP_TRANSFER_FAULT (1U << 2)

static inline uint8_t GetParity(uint32_t data)
{
    data ^= data >> 16;
    data ^= data >> 8;
    data ^= data >> 4;
    data &= 0x0F;
    return (0x6996 >> data) & 1;
}

uint8_t SWD_Write_Opt_10M(uint8_t header,
                          uint8_t turnaround,
                          uint8_t data_phase,
                          uint8_t idle_cycles,
                          uint32_t *data)
{
    uint32_t ack = 0;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

    /*********************** Send 8-Bit Header (Inline ASM) ***********************/
    __asm__ volatile(
        /* 加载常量 */
        "li   t0, 0x000C0000          \n\t"
        "li   t1, 0x40000000          \n\t"
        "li   t3, 0x08000000          \n\t"

        /* ===== Bit 0（首周期，需从 hdr 取最低位）===== */
        "andi t2, %[hdr], 1           \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "sw   t2, 0x100(t0)           \n\t" /* SWCLK↓ + DATA */
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t" /* 流水线：提前提取 Bit 1 */
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t" /* SWCLK↑ */

        /* ===== Bit 1~6（.rept 展开，共6次）===== */
        ".rept 6                      \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        ".endr                        \n\t"

        /* ===== Bit 7（尾周期，t2 已在上一轮提取好）===== */
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"

        /* Turnaround */
        "li   t2, 0x20000000          \n\t"
        "sw   t1, 0x108(t0)           \n\t"
        "sw   t2, 0x208(t0)           \n\t"

        : [hdr] "+r"(header)
        :
        : "t0", "t1", "t2", "t3", "memory");

    n = turnaround;
    do
    {
        // empty cycle
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        n--;
    } while (n);

    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");

    /* Acknowledge response */
    // bit = SW_READ_BIT();
    // ack = bit << 0;
    // bit = SW_READ_BIT();
    // ack |= bit << 1;
    // bit = SW_READ_BIT();
    // ack |= bit << 2;

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    ack >>= 29;

    if (ack == DAP_TRANSFER_OK)
    {
        // PIN_SWDIO_OUT_ENABLE();
        // set SWDIO pin to output mode, level shifter to output mode
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            __asm volatile("nop");
            n--;
        } while (n);

        /* Write data */
        val = *data;
        parity = GetParity(val);

        __asm__ volatile(
            /* 加载 FGPIO 基址和常量掩码 */

            "andi t2, %[hdr], 1           \n\t"

            /* Bit 0 */
            "slli t2, t2, 29              \n\t" /* SWDIO_SHIFT */
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t" /* FGPIO_DO_VAL_OFFSET: SWCLK↓ + DATA */
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t" /* FGPIO_DO_SET_OFFSET: SWCLK↑ */

            /* ===== Bit 1~30（.rept 展开，共30次）===== */
            ".rept 30                     \n\t"
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"
            ".endr                        \n\t"

            /* Bit 31 */
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"

            : [hdr] "+r"(val)                  /* 读写操作数：val 会被右移耗尽 */
            :                                  /* 无纯输入操作数 */
            : "t0", "t1", "t2", "t3", "memory" /* 显式 clobber，避免编译器复用这些寄存器 */
        );

        /*********************** Parity Bit ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((parity & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //   DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = idle_cycles;
        if (n)
        {
            // PIN_SWDIO_OUT(0U);
            HPM_FGPIO->DO[0].CLEAR = PIN_SWDIO_OFFSET;
            for (; n; n--)
            {
                // empty cycle
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
                __asm volatile("nop");
            }
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }

    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* WAIT or FAULT response */
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            n--;
        } while (n);
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (n = turnaround + 32U + 1U; n; n--)
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
}

uint8_t SWD_Write_Opt_20M(uint8_t header,
                          uint8_t turnaround,
                          uint8_t data_phase,
                          uint8_t idle_cycles,
                          uint32_t *data)
{
    uint32_t ack = 0;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

    /*********************** Send 8-Bit Header (Inline ASM) ***********************/
    __asm__ volatile(
        /* 加载常量 */
        "li   t0, 0x000C0000          \n\t"
        "li   t1, 0x40000000          \n\t"
        "li   t3, 0x08000000          \n\t"

        /* ===== Bit 0（首周期，需从 hdr 取最低位）===== */
        "andi t2, %[hdr], 1           \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "sw   t2, 0x100(t0)           \n\t" /* SWCLK↓ + DATA */
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t" /* 流水线：提前提取 Bit 1 */
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t" /* SWCLK↑ */

        /* ===== Bit 1~6（.rept 展开，共6次）===== */
        ".rept 6                      \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        ".endr                        \n\t"

        /* ===== Bit 7（尾周期，t2 已在上一轮提取好）===== */
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"

        /* Turnaround */
        "li   t2, 0x20000000          \n\t"
        "sw   t1, 0x108(t0)           \n\t"
        "sw   t2, 0x208(t0)           \n\t"

        : [hdr] "+r"(header)
        :
        : "t0", "t1", "t2", "t3", "memory");

    n = turnaround;
    do
    {
        // empty cycle
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
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

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    ack >>= 29;

    if (ack == DAP_TRANSFER_OK)
    {
        // PIN_SWDIO_OUT_ENABLE();
        // set SWDIO pin to output mode, level shifter to output mode
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            n--;
        } while (n);

        /* Write data */
        val = *data;
        parity = GetParity(val);

        __asm__ volatile(
            /* 加载 FGPIO 基址和常量掩码 */

            "andi t2, %[hdr], 1           \n\t"

            /* Bit 0 */
            "slli t2, t2, 29              \n\t" /* SWDIO_SHIFT */
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t" /* FGPIO_DO_VAL_OFFSET: SWCLK↓ + DATA */
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t" /* FGPIO_DO_SET_OFFSET: SWCLK↑ */

            /* ===== Bit 1~30（.rept 展开，共30次）===== */
            ".rept 30                     \n\t"
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"
            ".endr                        \n\t"

            /* Bit 31 */
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"

            : [hdr] "+r"(val)                  /* 读写操作数：val 会被右移耗尽 */
            :                                  /* 无纯输入操作数 */
            : "t0", "t1", "t2", "t3", "memory" /* 显式 clobber，避免编译器复用这些寄存器 */
        );

        /*********************** Parity Bit ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((parity & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //   DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = idle_cycles;
        if (n)
        {
            // PIN_SWDIO_OUT(0U);
            HPM_FGPIO->DO[0].CLEAR = PIN_SWDIO_OFFSET;
            for (; n; n--)
            {
                // empty cycle
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
            }
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }

    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* WAIT or FAULT response */
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            n--;
        } while (n);
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (n = turnaround + 32U + 1U; n; n--)
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
}

uint8_t SWD_Write_Opt_30M(uint8_t header,
                          uint8_t turnaround,
                          uint8_t data_phase,
                          uint8_t idle_cycles,
                          uint32_t *data)
{
    uint32_t ack = 0;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

    /*********************** Send 8-Bit Header (Inline ASM) ***********************/
    __asm__ volatile(
        /* 加载常量 */
        "li   t0, 0x000C0000          \n\t"
        "li   t1, 0x40000000          \n\t"
        "li   t3, 0x08000000          \n\t"

        /* ===== Bit 0（首周期，需从 hdr 取最低位）===== */
        "andi t2, %[hdr], 1           \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "sw   t2, 0x100(t0)           \n\t" /* SWCLK↓ + DATA */
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t" /* 流水线：提前提取 Bit 1 */
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t" /* SWCLK↑ */

        /* ===== Bit 1~6（.rept 展开，共6次）===== */
        ".rept 6                      \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        ".endr                        \n\t"

        /* ===== Bit 7（尾周期，t2 已在上一轮提取好）===== */
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"

        /* Turnaround */
        "li   t2, 0x20000000          \n\t"
        "sw   t1, 0x108(t0)           \n\t"
        "sw   t2, 0x208(t0)           \n\t"

        : [hdr] "+r"(header)
        :
        : "t0", "t1", "t2", "t3", "memory");

    n = turnaround;
    do
    {
        // empty cycle
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
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

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    ack >>= 29;

    if (ack == DAP_TRANSFER_OK)
    {
        // PIN_SWDIO_OUT_ENABLE();
        // set SWDIO pin to output mode, level shifter to output mode
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            n--;
        } while (n);

        /* Write data */
        val = *data;
        parity = GetParity(val);

        __asm__ volatile(
            /* 加载 FGPIO 基址和常量掩码 */

            "andi t2, %[hdr], 1           \n\t"

            /* Bit 0 */
            "slli t2, t2, 29              \n\t" /* SWDIO_SHIFT */
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t" /* FGPIO_DO_VAL_OFFSET: SWCLK↓ + DATA */
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t" /* FGPIO_DO_SET_OFFSET: SWCLK↑ */

            /* ===== Bit 1~30（.rept 展开，共30次）===== */
            ".rept 30                     \n\t"
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"
            ".endr                        \n\t"

            /* Bit 31 */
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"

            : [hdr] "+r"(val)                  /* 读写操作数：val 会被右移耗尽 */
            :                                  /* 无纯输入操作数 */
            : "t0", "t1", "t2", "t3", "memory" /* 显式 clobber，避免编译器复用这些寄存器 */
        );

        /*********************** Parity Bit ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((parity & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //   DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = idle_cycles;
        if (n)
        {
            // PIN_SWDIO_OUT(0U);
            HPM_FGPIO->DO[0].CLEAR = PIN_SWDIO_OFFSET;
            for (; n; n--)
            {
                // empty cycle
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
            }
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }

    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* WAIT or FAULT response */
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            n--;
        } while (n);
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (n = turnaround + 32U + 1U; n; n--)
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
}

uint8_t SWD_Write_Opt_36M(uint8_t header,
                          uint8_t turnaround,
                          uint8_t data_phase,
                          uint8_t idle_cycles,
                          uint32_t *data)
{
    uint32_t ack = 0;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

    /*********************** Send 8-Bit Header (Inline ASM) ***********************/
    __asm__ volatile(
        /* 加载常量 */
        "li   t0, 0x000C0000          \n\t"
        "li   t1, 0x40000000          \n\t"
        "li   t3, 0x08000000          \n\t"

        /* ===== Bit 0（首周期，需从 hdr 取最低位）===== */
        "andi t2, %[hdr], 1           \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t" /* SWCLK↓ + DATA */
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t" /* 流水线：提前提取 Bit 1 */
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t" /* SWCLK↑ */

        /* ===== Bit 1~6（.rept 展开，共6次）===== */
        ".rept 6                      \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        ".endr                        \n\t"

        /* ===== Bit 7（尾周期，t2 已在上一轮提取好）===== */
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"

        /* Turnaround */
        "li   t2, 0x20000000          \n\t"
        "sw   t1, 0x108(t0)           \n\t"
        "sw   t2, 0x208(t0)           \n\t"

        : [hdr] "+r"(header)
        :
        : "t0", "t1", "t2", "t3", "memory");

    n = turnaround;
    do
    {
        // empty cycle
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
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

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    ack >>= 29;

    if (ack == DAP_TRANSFER_OK)
    {
        // PIN_SWDIO_OUT_ENABLE();
        // set SWDIO pin to output mode, level shifter to output mode
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            n--;
        } while (n);

        /* Write data */
        val = *data;
        parity = GetParity(val);

        __asm__ volatile(
            /* 加载 FGPIO 基址和常量掩码 */

            "andi t2, %[hdr], 1           \n\t"

            /* Bit 0 */
            "slli t2, t2, 29              \n\t" /* SWDIO_SHIFT */
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t" /* FGPIO_DO_VAL_OFFSET: SWCLK↓ + DATA */
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t" /* FGPIO_DO_SET_OFFSET: SWCLK↑ */

            /* ===== Bit 1~30（.rept 展开，共30次）===== */
            ".rept 30                     \n\t"
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"
            ".endr                        \n\t"

            /* Bit 31 */
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"

            : [hdr] "+r"(val)                  /* 读写操作数：val 会被右移耗尽 */
            :                                  /* 无纯输入操作数 */
            : "t0", "t1", "t2", "t3", "memory" /* 显式 clobber，避免编译器复用这些寄存器 */
        );

        /*********************** Parity Bit ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((parity & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //   DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = idle_cycles;
        if (n)
        {
            // PIN_SWDIO_OUT(0U);
            HPM_FGPIO->DO[0].CLEAR = PIN_SWDIO_OFFSET;
            for (; n; n--)
            {
                // empty cycle
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
            }
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }

    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* WAIT or FAULT response */
        /* Turnaround */
        n = turnaround;
        do
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            n--;
        } while (n);
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (n = turnaround + 32U + 1U; n; n--)
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
}

uint8_t SWD_Write_Opt_45M(uint8_t header,
                          uint8_t turnaround,
                          uint8_t data_phase,
                          uint8_t idle_cycles,
                          uint32_t *data)
{
    uint32_t ack = 0;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

    /*********************** Send 8-Bit Header (Inline ASM) ***********************/
    __asm__ volatile(
        /* 加载常量 */
        "li   t0, 0x000C0000          \n\t"
        "li   t1, 0x40000000          \n\t"
        "li   t3, 0x08000000          \n\t"

        /* ===== Bit 0（首周期，需从 hdr 取最低位）===== */
        "andi t2, %[hdr], 1           \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t" /* SWCLK↓ + DATA */
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t" /* 流水线：提前提取 Bit 1 */
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t" /* SWCLK↑ */

        /* ===== Bit 1~6（.rept 展开，共6次）===== */
        ".rept 6                      \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        ".endr                        \n\t"

        /* ===== Bit 7（尾周期，t2 已在上一轮提取好）===== */
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "nop                          \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        "nop                          \n\t"

        /* Turnaround */
        "li   t2, 0x20000000          \n\t"
        "sw   t1, 0x108(t0)           \n\t"
        "sw   t2, 0x208(t0)           \n\t"

        : [hdr] "+r"(header)
        :
        : "t0", "t1", "t2", "t3", "memory");

    // ===== 热路径：n=1，纯顺序执行，零分支 =====
    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;

    // ===== 冷路径：n>1，极少执行 =====
    if (UNLIKELY(turnaround > 1))
    {
        uint32_t n = turnaround - 1;
        do
        {
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        } while (--n);
    }

    __asm volatile("nop");
    __asm volatile("nop");

    /* Acknowledge response */
    // bit = SW_READ_BIT();
    // ack = bit << 0;
    // bit = SW_READ_BIT();
    // ack |= bit << 1;
    // bit = SW_READ_BIT();
    // ack |= bit << 2;

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    ack >>= 29;

    if (LIKELY(ack == DAP_TRANSFER_OK))
    {
        // PIN_SWDIO_OUT_ENABLE();
        // set SWDIO pin to output mode, level shifter to output mode
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        /* Turnaround */
        // ===== 热路径：n=1，纯顺序执行，零分支 =====
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;

        // ===== 冷路径：n>1，极少执行 =====
        if (UNLIKELY(turnaround > 1))
        {
            uint32_t n = turnaround - 1;
            do
            {
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            } while (--n);
        }

        /* Write data */
        val = *data;
        parity = GetParity(val);

        __asm__ volatile(
            /* 加载 FGPIO 基址和常量掩码 */

            "andi t2, %[hdr], 1           \n\t"

            /* Bit 0 */
            "slli t2, t2, 29              \n\t" /* SWDIO_SHIFT */
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t" /* FGPIO_DO_VAL_OFFSET: SWCLK↓ + DATA */
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t" /* FGPIO_DO_SET_OFFSET: SWCLK↑ */

            /* ===== Bit 1~30（.rept 展开，共30次）===== */
            ".rept 30                     \n\t"
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"
            ".endr                        \n\t"

            /* Bit 31 */
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "nop                          \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"

            : [hdr] "+r"(val)                  /* 读写操作数：val 会被右移耗尽 */
            :                                  /* 无纯输入操作数 */
            : "t0", "t1", "t2", "t3", "memory" /* 显式 clobber，避免编译器复用这些寄存器 */
        );

        /*********************** Parity Bit ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((parity & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        __asm volatile("nop");

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //   DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = idle_cycles;
        if (UNLIKELY(n))
        {
            // PIN_SWDIO_OUT(0U);
            HPM_FGPIO->DO[0].CLEAR = PIN_SWDIO_OFFSET;
            for (; n; n--)
            {
                // empty cycle
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
            }
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }

    else if (UNLIKELY((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)))
    {
        /* WAIT or FAULT response */
        /* Turnaround */
        // ===== 热路径：n=1，纯顺序执行，零分支 =====
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;

        // ===== 冷路径：n>1，极少执行 =====
        if (UNLIKELY(turnaround > 1))
        {
            uint32_t n = turnaround - 1;
            do
            {
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            } while (--n);
        }

        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (n = turnaround + 32U + 1U; n; n--)
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
}

uint8_t SWD_Write_Opt_60M(uint8_t header,
                          uint8_t turnaround,
                          uint8_t data_phase,
                          uint8_t idle_cycles,
                          uint32_t *data)
{
    uint32_t ack = 0;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;
    uint32_t n;

    /*********************** Send 8-Bit Header (Inline ASM) ***********************/
    __asm__ volatile(
        /* 加载常量 */
        "li   t0, 0x000C0000          \n\t"
        "li   t1, 0x40000000          \n\t"
        "li   t3, 0x08000000          \n\t"

        /* ===== Bit 0（首周期，需从 hdr 取最低位）===== */
        "andi t2, %[hdr], 1           \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "sw   t2, 0x100(t0)           \n\t" /* SWCLK↓ + DATA */
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t" /* 流水线：提前提取 Bit 1 */
        "sw   t3, 0x104(t0)           \n\t" /* SWCLK↑ */

        /* ===== Bit 1~6（.rept 展开，共6次）===== */
        ".rept 6                      \n\t"
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "srli %[hdr], %[hdr], 1       \n\t"
        "andi t2, %[hdr], 1           \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        ".endr                        \n\t"

        /* ===== Bit 7（尾周期，t2 已在上一轮提取好）===== */
        "slli t2, t2, 29              \n\t"
        "or   t2, t2, t1              \n\t"
        "sw   t2, 0x100(t0)           \n\t"
        "nop                          \n\t"
        "nop                          \n\t"
        "sw   t3, 0x104(t0)           \n\t"
        "nop                          \n\t"

        /* Turnaround */
        "li   t2, 0x20000000          \n\t"
        "sw   t1, 0x108(t0)           \n\t"
        "sw   t2, 0x208(t0)           \n\t"

        : [hdr] "+r"(header)
        :
        : "t0", "t1", "t2", "t3", "memory");

    // ===== 热路径：n=1，纯顺序执行，零分支 =====
    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;

    // ===== 冷路径：n>1，极少执行 =====
    if (UNLIKELY(turnaround > 1))
    {
        uint32_t n = turnaround - 1;
        do
        {
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        } while (--n);
    }

    // uint32_t irq_status = disable_global_irq(0x8U);
    __asm volatile("nop");
    __asm volatile("nop");

    /* Acknowledge response */
    // bit = SW_READ_BIT();
    // ack = bit << 0;
    // bit = SW_READ_BIT();
    // ack |= bit << 1;
    // bit = SW_READ_BIT();
    // ack |= bit << 2;

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm__ __volatile__(
        "srli  %0, %0, 28\n\t" /* bit = bit >> 28 */
        "andi  %0, %0, 1\n\t"  /* bit = bit & 1 */
        "srli  %1, %1, 1\n\t"  /* val = val >> 1 */
        "slli  %0, %0, 31\n\t" /* bit = bit << 31 */
        "or    %1, %1, %0\n\t" /* val = val | bit */
        : "+r"(bit), "+r"(ack) /* bit 和 val 都是读写 */
        :
        : "memory");

    ack >>= 29;
    // enable_global_irq(irq_status);

    if (LIKELY(ack == DAP_TRANSFER_OK))
    {
        // PIN_SWDIO_OUT_ENABLE();
        // set SWDIO pin to output mode, level shifter to output mode
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        /* Turnaround */
        // ===== 热路径：n=1，纯顺序执行，零分支 =====
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;

        // ===== 冷路径：n>1，极少执行 =====
        if (UNLIKELY(turnaround > 1))
        {
            uint32_t n = turnaround - 1;
            do
            {
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            } while (--n);
        }

        /* Write data */
        val = *data;
        parity = GetParity(val);

        __asm__ volatile(
            /* 加载 FGPIO 基址和常量掩码 */

            "andi t2, %[hdr], 1           \n\t"

            /* Bit 0 */
            "slli t2, t2, 29              \n\t" /* SWDIO_SHIFT */
            "or   t2, t2, t1              \n\t"
            "sw   t2, 0x100(t0)           \n\t" /* FGPIO_DO_VAL_OFFSET: SWCLK↓ + DATA */
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "sw   t3, 0x104(t0)           \n\t" /* FGPIO_DO_SET_OFFSET: SWCLK↑ */

            /* ===== Bit 1~30（.rept 展开，共30次）===== */
            ".rept 30                     \n\t"
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "srli %[hdr], %[hdr], 1       \n\t"
            "andi t2, %[hdr], 1           \n\t"
            "sw   t3, 0x104(t0)           \n\t"
            ".endr                        \n\t"

            /* Bit 31 */
            "slli t2, t2, 29              \n\t"
            "or   t2, t2, t1              \n\t"
            "sw   t2, 0x100(t0)           \n\t"
            "nop                          \n\t"
            "nop                          \n\t"
            "sw   t3, 0x104(t0)           \n\t"

            : [hdr] "+r"(val)                  /* 读写操作数：val 会被右移耗尽 */
            :                                  /* 无纯输入操作数 */
            : "t0", "t1", "t2", "t3", "memory" /* 显式 clobber，避免编译器复用这些寄存器 */
        );

        /*********************** Parity Bit ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((parity & 0x01) << PIN_SWDIO_BITFLD);
        // __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // __asm volatile("nop");
        __asm volatile("nop");

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //   DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = idle_cycles;
        if (UNLIKELY(n))
        {
            // PIN_SWDIO_OUT(0U);
            HPM_FGPIO->DO[0].CLEAR = PIN_SWDIO_OFFSET;
            for (; n; n--)
            {
                // empty cycle
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
            }
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }

    else if (UNLIKELY((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)))
    {
        /* WAIT or FAULT response */
        /* Turnaround */
        // ===== 热路径：n=1，纯顺序执行，零分支 =====
        HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        // ===== 冷路径：n>1，极少执行 =====
        if (UNLIKELY(turnaround > 1))
        {
            uint32_t n = turnaround - 1;
            do
            {
                HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            } while (--n);
        }

        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (n = turnaround + 32U + 1U; n; n--)
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
        }
        // PIN_SWDIO_OUT_ENABLE();
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;
        // PIN_SWDIO_OUT(1U);
        HPM_FGPIO->DO[0].SET = PIN_SWDIO_OFFSET;
        return ((uint8_t)ack);
    }
}
