#include <stdint.h>

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

uint8_t SWD_Write_Opt(uint8_t header,
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

    /*********************** Send 8-Bit Header ***********************/
    /*********************** Start Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    header >>= 1;

    /*********************** APnDP Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    header >>= 1;

    /*********************** RnW Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    header >>= 1;

    /*********************** A2 Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    header >>= 1;

    /*********************** A3 Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    header >>= 1;

    /*********************** Parity Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    header >>= 1;

    /*********************** Stop Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");
    header >>= 1;

    /*********************** Park Bit ***********************/
    HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                             ((header & 0x01) << PIN_SWDIO_BITFLD);
    __asm volatile("nop");
    __asm volatile("nop");
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
    __asm volatile("nop");
    __asm volatile("nop");

    /* Turnaround */
    // set SWDIO pin to input mode, level shifter to input mode
    HPM_FGPIO->OE[0].CLEAR = PIN_SWDIO_OFFSET;
    HPM_FGPIO->DO[0].CLEAR = PIN_SWDIR_OFFSET;

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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
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
    bit = HPM_FGPIO->DI[0].VALUE;
    HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
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
        /* Turnaround */
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

        // PIN_SWDIO_OUT_ENABLE();
        // set SWDIO pin to output mode, level shifter to output mode
        HPM_FGPIO->OE[0].SET = PIN_SWDIO_OFFSET;
        HPM_FGPIO->DO[0].SET = PIN_SWDIR_OFFSET;

        /* Write data */
        val = *data;
        parity = GetParity(val);

        /*********************** Data Bit 0 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 1 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 2 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 3 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 4 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 5 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 6 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 7 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 8 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 9 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 10 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 11 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 12 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 13 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 14 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 15 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 16 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 17 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 18 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 19 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 20 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 21 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 22 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 23 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 24 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 25 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 26 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 27 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 28 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 29 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 30 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");
        val >>= 1;

        /*********************** Data Bit 31 ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((val & 0x01) << PIN_SWDIO_BITFLD);
        __asm volatile("nop");
        __asm volatile("nop");
        HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
        __asm volatile("nop");
        __asm volatile("nop");

        /*********************** Parity Bit ***********************/
        HPM_FGPIO->DO[0].VALUE = (1 << PIN_SWDIR_BITFLD) |
                                 ((parity & 0x01) << PIN_SWDIO_BITFLD);
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
                __asm volatile("nop");
                HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
                __asm volatile("nop");
            }
        }
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
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            n--;
        } while (n);
    }
    else
    {
        /* Protocol error */
        for (n = turnaround + 32U + 1U; n; n--)
        {
            // empty cycle
            HPM_FGPIO->DO[0].CLEAR = PIN_SWCLK_OFFSET;
            __asm volatile("nop");
            __asm volatile("nop");
            HPM_FGPIO->DO[0].SET = PIN_SWCLK_OFFSET;
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
