/*
 * Copyright (c) 2024 RCSN
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "board.h"
#include "hpm_spi_drv.h"
#include "swd_common.h"
#ifdef HPMSOC_HAS_HPMSDK_DMAV2
#include "hpm_dmav2_drv.h"
#else
#include "hpm_dma_drv.h"
#endif
#include "hpm_dmamux_drv.h"
#include "DAP_config.h"
#include "DAP.h"

#define SWD_SPI_SCLK_FREQ (20000000UL)

#define SWD_SPI_DMA BOARD_APP_HDMA
#define SWD_SPI_DMAMUX BOARD_APP_DMAMUX
#define SWD_SPI_RX_DMA_REQ BOARD_APP_SPI_RX_DMA
#define SWD_SPI_TX_DMA_REQ BOARD_APP_SPI_TX_DMA
#define SWD_SPI_RX_DMA_CH 0
#define SWD_SPI_TX_DMA_CH 1
#define SWD_SPI_RX_DMAMUX_CH DMA_SOC_CHN_TO_DMAMUX_CHN(SWD_SPI_DMA, SWD_SPI_RX_DMA_CH)
#define SWD_SPI_TX_DMAMUX_CH DMA_SOC_CHN_TO_DMAMUX_CHN(SWD_SPI_DMA, SWD_SPI_TX_DMA_CH)

extern host_ops_t host_ops_table[];
ATTR_PLACE_AT(".ahb_sram")
uint8_t target_ack[4];

static void swd_emulation_init(void);

static inline uint8_t GetParity(uint32_t data)
{
    data ^= data >> 16;
    data ^= data >> 8;
    data ^= data >> 4;
    data &= 0x0F;
    return (0x6996 >> data) & 1;
}

void SPI_PORT_SWD_SETUP(void)
{
    board_init_spi_pins(SWD_SPI_BASE);
    HPM_IOC->PAD[PIN_TCK].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3);
    HPM_IOC->PAD[PIN_TMS].PAD_CTL = IOC_PAD_PAD_CTL_SR_MASK | IOC_PAD_PAD_CTL_SPD_SET(3) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);

    HPM_IOC->PAD[PIN_SRST].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[PIN_SRST].PAD_CTL = IOC_PAD_PAD_CTL_PRS_SET(2) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(1) | IOC_PAD_PAD_CTL_SPD_SET(3);
    gpiom_configure_pin_control_setting(PIN_SRST);

    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_SRST), GPIO_GET_PIN_INDEX(PIN_SRST));
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(PIN_SRST), GPIO_GET_PIN_INDEX(PIN_SRST), !HSLink_Global.reset_level);

    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1); // 默认SWDIO为输出

    swd_emulation_init();
}

// Generate SWJ Sequence
//   count:  sequence bit count
//   data:   pointer to sequence bit data
//   return: none
#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
void SPI_SWJ_Sequence(uint32_t count, const uint8_t *data)
{
    uint32_t n;
    uint32_t integer_val = (count / 8);
    uint32_t remaind_val = (count % 8);

    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
    spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_write_only);
    SWD_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
    while (SWD_SPI_BASE->STATUS & (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
    {
    };
    if (integer_val > 0)
    {
        spi_set_data_bits(SWD_SPI_BASE, 8);
        spi_set_write_data_count(SWD_SPI_BASE, integer_val);
        SWD_SPI_BASE->CMD = 0xFF;
        for (n = 0; n < integer_val; n++)
        {
            SWD_SPI_BASE->DATA = *(data + n);
        }
        while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
        {
        }
    }

    if (remaind_val > 0)
    {
        spi_set_write_data_count(SWD_SPI_BASE, 1);
        spi_set_data_bits(SWD_SPI_BASE, remaind_val);
        SWD_SPI_BASE->DATA = *(data + integer_val);
        SWD_SPI_BASE->CMD = 0xFF;
        while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
        {
        };
    }
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}
#endif

// Generate SWD Sequence
//   info:   sequence information
//   swdo:   pointer to SWDIO generated data
//   swdi:   pointer to SWDIO captured data
//   return: none
#if (DAP_SWD != 0)
void SPI_SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
    uint32_t count, integer_val, remaind_val, n = 0;

    count = info & SWD_SEQUENCE_CLK;
    if (count == 0U)
    {
        count = 64U;
    }
    SWD_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
    while (SWD_SPI_BASE->STATUS & (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
    {
    };
    integer_val = (count / 8);
    remaind_val = (count % 8);
    // log_d("SWD_Sequence info 0x%x integer_val %d, remaind_val %d", info, integer_val, remaind_val);
    if (info & SWD_SEQUENCE_DIN)
    {
        spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_read_only);
        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 0);
        if (integer_val)
        {
            spi_set_read_data_count(SWD_SPI_BASE, integer_val);
            spi_set_data_bits(SWD_SPI_BASE, 8);
            SWD_SPI_BASE->CMD = 0xFF;
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
            {
            };
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) != SPI_STATUS_RXEMPTY_MASK)
            {
                *(swdi + n) = SWD_SPI_BASE->DATA;
                n++;
            }
            while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
            {
            };
        }
        if (remaind_val)
        {
            spi_set_read_data_count(SWD_SPI_BASE, 1);
            spi_set_data_bits(SWD_SPI_BASE, remaind_val);
            SWD_SPI_BASE->CMD = 0xFF;
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
            {
            };
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) != SPI_STATUS_RXEMPTY_MASK)
            {
                *(swdi + integer_val) = SWD_SPI_BASE->DATA;
            }
        }
    }
    else
    {
        spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_write_only);
        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        if (integer_val)
        {
            spi_set_write_data_count(SWD_SPI_BASE, integer_val);
            spi_set_data_bits(SWD_SPI_BASE, 8);
            SWD_SPI_BASE->CMD = 0xFF;
            for (n = 0; n < integer_val; n++)
            {
                SWD_SPI_BASE->DATA = *(swdo + n);
            }
            while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
            {
            };
        }
        if (remaind_val)
        {
            spi_set_write_data_count(SWD_SPI_BASE, 1);
            spi_set_data_bits(SWD_SPI_BASE, remaind_val);
            SWD_SPI_BASE->CMD = 0xFF;
            SWD_SPI_BASE->DATA = *(swdo + integer_val);
            while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
            {
            };
        }
    }
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
}

uint8_t SPI_SWD_Write(uint8_t header, uint32_t *data)
{
    uint8_t ack = 0;
    uint32_t parity = 0;
    uint8_t i = 0;
    uint8_t ack_width = 0;

    // gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);

    // SWD_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
    // while (SWD_SPI_BASE->STATUS & (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
    // {
    // };

    spi_set_write_data_count(SWD_SPI_BASE, 1);
    spi_set_read_data_count(SWD_SPI_BASE, 1);
    spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_write_only);
    spi_set_data_bits(SWD_SPI_BASE, 8);

    SWD_SPI_BASE->CMD = 0xFF;
    SWD_SPI_BASE->DATA = header;

    ack_width = DAP_Data.swd_conf.turnaround + 3 + DAP_Data.swd_conf.turnaround; /* trn + ack + trn*/

    while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
    {
    };

    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 0);
    spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_read_only);
    spi_set_data_bits(SWD_SPI_BASE, ack_width);

    SWD_SPI_BASE->CMD = 0xFF;

    while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
    {
    };
    ack = SWD_SPI_BASE->DATA;
    // while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
    // {
    // };

    ack >>= DAP_Data.swd_conf.turnaround;
    ack = ack & 0x07;

    if (ack == DAP_TRANSFER_OK)
    { /* OK response */
        /* Data transfer */

        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        SWD_SPI_BASE->TRANSCTRL = 0x1000000; /* only write mode*/
        SWD_SPI_BASE->TRANSFMT = 0x1F18;     /* datalen = 32bit, mosibidir = 1, lsb=1 */
        SWD_SPI_BASE->CMD = 0xFF;
        SWD_SPI_BASE->DATA = (*data);

        parity = GetParity(*data);

        while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
        {
        };
        spi_set_data_bits(SWD_SPI_BASE, 1);
        SWD_SPI_BASE->CMD = 0xFF;
        SWD_SPI_BASE->DATA = parity;
        while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
        {
        };

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //     DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        if (DAP_Data.transfer.idle_cycles > 0)
        {
            SWD_SPI_BASE->TRANSCTRL = 0x01000000; /* only write mode*/
            SWD_SPI_BASE->TRANSFMT = 0x0018;      /* datalen = 1bit, mosibidir = 1, lsb=1 */
            spi_set_write_data_count(SWD_SPI_BASE, DAP_Data.transfer.idle_cycles);
            SWD_SPI_BASE->CMD = 0xFF;
            for (int i = 0; i < DAP_Data.transfer.idle_cycles; i++)
            {
                SWD_SPI_BASE->DATA = 0;
                while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
                {
                };
            }
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
            {
            };
            while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
            {
            };
        }
        return ack;
    }
    /* WAIT or FAULT response */
    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* Turnaround */
        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        SWD_SPI_BASE->TRANSCTRL = 0x01000000; /* only write mode*/
        SWD_SPI_BASE->TRANSFMT = 0x0018;      /* datalen = 1bit, mosibidir = 1, lsb=1 */
        spi_set_write_data_count(SWD_SPI_BASE, DAP_Data.swd_conf.turnaround);
        SWD_SPI_BASE->CMD = 0xFF;
        SWD_SPI_BASE->DATA = 0;
        while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
        {
        };
        while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
        {
        };

        /* Dummy Write WDATA[0:31] + Parity */
        if (DAP_Data.swd_conf.data_phase)
        {
            SWD_SPI_BASE->TRANSCTRL = 0x01000000; /* only write mode*/
            SWD_SPI_BASE->TRANSFMT = 0x0018;      /* datalen = 1bit, mosibidir = 1, lsb=1 */
            spi_set_write_data_count(SWD_SPI_BASE, 33);
            SWD_SPI_BASE->CMD = 0xFF;
            for (i = 0; i < 33; i++)
            {
                SWD_SPI_BASE->DATA = 0;
                while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
                {
                };
            }
            while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
            {
            };
        }
        return ack;
    }
    else
    {
        /* Protocol error */
        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_write_only);
        spi_set_data_bits(SWD_SPI_BASE, 1);
        spi_set_write_data_count(SWD_SPI_BASE, DAP_Data.swd_conf.turnaround + 32U + 1U);
        SWD_SPI_BASE->CMD = 0xFF;
        for (i = 0; i < DAP_Data.swd_conf.turnaround + 32U + 1U; i++)
        {
            SWD_SPI_BASE->DATA = 0;
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
            {
            };
        }
        return ack;
    }
}

// SWD Transfer I/O
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
uint8_t SPI_SWD_Read(uint8_t header, uint32_t *data)
{
    uint8_t ack = 0;
    uint32_t parity = 0;
    uint8_t calc_parity = 0;
    uint8_t i = 0;
    uint32_t dummy = 0;
    uint8_t ack_width = 0;

    // gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);

    // SWD_SPI_BASE->CTRL |= SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK;
    // while (SWD_SPI_BASE->STATUS & (SPI_CTRL_RXFIFORST_MASK | SPI_CTRL_TXFIFORST_MASK))
    // {
    // };

    spi_set_write_data_count(SWD_SPI_BASE, 1);
    spi_set_read_data_count(SWD_SPI_BASE, 1);
    spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_write_only);
    spi_set_data_bits(SWD_SPI_BASE, 8);

    SWD_SPI_BASE->CMD = 0xFF;
    SWD_SPI_BASE->DATA = header;

    ack_width = 3 + DAP_Data.swd_conf.turnaround; /* trn max 4 periods */

    while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
    {
    };

    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 0);

    spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_read_only);
    spi_set_data_bits(SWD_SPI_BASE, ack_width);
    SWD_SPI_BASE->CMD = 0xFF;
    while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
    {
    };
    ack = SWD_SPI_BASE->DATA;
    // while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
    // {
    // };

    ack >>= DAP_Data.swd_conf.turnaround;
    ack = ack & 0x07;

    if (ack == DAP_TRANSFER_OK)
    { /* OK response */
        /* Data transfer */

        /* Read data */
        // gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 0);
        SWD_SPI_BASE->TRANSCTRL = 0x2000000; /* only read mode*/
        SWD_SPI_BASE->TRANSFMT = 0x1F18;     /* datalen = 32bit, mosibidir = 1, lsb=1 */
        SWD_SPI_BASE->CMD = 0xFF;
        // while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
        // {
        // };
        while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
        {
        };
        dummy = SWD_SPI_BASE->DATA;
        spi_set_data_bits(SWD_SPI_BASE, 1 + DAP_Data.swd_conf.turnaround);
        SWD_SPI_BASE->CMD = 0xFF;

        calc_parity = GetParity(dummy);
        while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
        {
        };
        parity = ((SWD_SPI_BASE->DATA) >> DAP_Data.swd_conf.turnaround) & 0x01;

        if (parity != (calc_parity & 0x01))
        {
            ack = DAP_TRANSFER_ERROR;
        }
        if (data != NULL)
        {
            (*data) = dummy;
        }

        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //     DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        if (DAP_Data.transfer.idle_cycles > 0)
        {
            SWD_SPI_BASE->TRANSCTRL = 0x01000000; /* only write mode*/
            SWD_SPI_BASE->TRANSFMT = 0x0018;      /* datalen = 1bit, mosibidir = 1, lsb=1 */
            spi_set_write_data_count(SWD_SPI_BASE, DAP_Data.transfer.idle_cycles);
            SWD_SPI_BASE->CMD = 0xFF;

            for (int i = 0; i < DAP_Data.transfer.idle_cycles; i++)
            {
                SWD_SPI_BASE->DATA = 0;
                while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
                {
                };
            }

            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
            {
            };
            while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
            {
            };
        }
        return ack;
    }
    /* WAIT or FAULT response */
    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        SWD_SPI_BASE->TRANSCTRL = 0x02000000; /* only read mode*/
        SWD_SPI_BASE->TRANSFMT = 0x0018;      /* datalen = 1bit, mosibidir = 1, lsb=1 */
        /* Dummy Read RDATA[0:31] + Parity */
        if (DAP_Data.swd_conf.data_phase)
        {
            spi_set_write_data_count(SWD_SPI_BASE, 33);
            SWD_SPI_BASE->CMD = 0xFF;
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_RXEMPTY_MASK) == SPI_STATUS_RXEMPTY_MASK)
            {
            };
            parity = (SWD_SPI_BASE->DATA) & 0x01;
            while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
            {
            };
        }
        /* Turnaround */
        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        SWD_SPI_BASE->TRANSCTRL = 0x01000000; /* only write mode*/
        SWD_SPI_BASE->TRANSFMT = 0x0018;      /* datalen = 1bit, mosibidir = 1, lsb=1 */
        spi_set_write_data_count(SWD_SPI_BASE, DAP_Data.swd_conf.turnaround);
        SWD_SPI_BASE->CMD = 0xFF;
        SWD_SPI_BASE->DATA = 0;
        while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
        {
        };
        while (SWD_SPI_BASE->STATUS & SPI_STATUS_SPIACTIVE_MASK)
        {
        };

        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        return ack;
    }
    else
    {
        /* Protocol error */
        spi_set_transfer_mode(SWD_SPI_BASE, spi_trans_write_only);
        spi_set_data_bits(SWD_SPI_BASE, 1);
        spi_set_write_data_count(SWD_SPI_BASE, DAP_Data.swd_conf.turnaround + 32U + 1U);
        SWD_SPI_BASE->CMD = 0xFF;
        for (i = 0; i < DAP_Data.swd_conf.turnaround + 32U + 1U; i++)
        {
            SWD_SPI_BASE->DATA = 0;
            while ((SWD_SPI_BASE->STATUS & SPI_STATUS_TXFULL_MASK) == SPI_STATUS_TXFULL_MASK)
            {
            };
        }
        spi_set_data_bits(SWD_SPI_BASE, 1);
        gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1);
        return ack;
    }
}
#endif

static void swd_emulation_init(void)
{
    spi_timing_config_t timing_config = {0};
    spi_format_config_t format_config = {0};
    spi_control_config_t control_config = {0};
    uint32_t spi_clcok;
    uint32_t pll_clk = 0, div = 0;
    /* set SPI sclk frequency for master */
    spi_clcok = board_init_spi_clock(SWD_SPI_BASE);
    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.cs2sclk = spi_cs2sclk_half_sclk_1;
    timing_config.master_config.csht = spi_csht_half_sclk_1;
    timing_config.master_config.clk_src_freq_in_hz = spi_clcok;
    timing_config.master_config.sclk_freq_in_hz = SWD_SPI_SCLK_FREQ;
    if (status_success != spi_master_timing_init(SWD_SPI_BASE, &timing_config))
    {
        spi_master_set_sclk_div(SWD_SPI_BASE, 0xFF);
        pll_clk = get_frequency_for_source(clock_source_pll1_clk0);
        div = pll_clk / SWD_SPI_SCLK_FREQ;
        clock_set_source_divider(SWD_SPI_BASE_CLOCK_NAME, clk_src_pll1_clk0, div);
    }

    /* set SPI format config for master */
    spi_master_get_default_format_config(&format_config);
    format_config.master_config.addr_len_in_bytes = 1U;
    format_config.common_config.data_len_in_bits = 1;
    format_config.common_config.data_merge = false;
    format_config.common_config.mosi_bidir = true;
    format_config.common_config.lsb = true;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol = spi_sclk_low_idle;
    format_config.common_config.cpha = spi_sclk_sampling_odd_clk_edges;
    spi_format_init(SWD_SPI_BASE, &format_config);

    /* set SPI control config for master */
    spi_master_get_default_control_config(&control_config);
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    control_config.master_config.addr_phase_fmt = spi_address_phase_format_single_io_mode;
    control_config.common_config.trans_mode = spi_trans_write_dummy_read;
    control_config.common_config.data_phase_fmt = spi_single_io_mode;
    control_config.common_config.dummy_cnt = spi_dummy_count_1;
    spi_control_init(SWD_SPI_BASE, &control_config, 1, 1);
}
