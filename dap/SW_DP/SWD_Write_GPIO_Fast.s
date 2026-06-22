



    .text
    .align  2
    .global SWD_Write_GPIO_Fast
    .type   SWD_Write_GPIO_Fast, @function

# definition
# uint8_t SWD_Write_GPIO_Fast(uint8_t header, uint8_t turnaround, uint8_t data_phase, uint8_t idle_cycles, uint32_t *data)
#
# parameter:
# a0 -- uint8_t header (already zero-extended)
# a1 -- uint8_t turnaround (already zero-extended)
# a2 -- uint8_t data_phase (already zero-extended)
# a3 -- uint8_t idle_cycles (already zero-extended)
# a4 -- uint32_t *data
#
# return value:
# a0 -- ack (zero-extend to 32-bit)

# Register allocate
# a0 -- header, ack return data
# a1 -- uint8_t turnaround (already zero-extended)
# a2 -- uint8_t data_phase (already zero-extended)
# a3 -- uint8_t idle_cycles (already zero-extended)
# a4 -- uint32_t *data
# a5 -- parity calculate temp
# a6 -- parity calculate temp
# a7 -- Unused
# t0 -- R32_GPIOB_BSHR address
# t1 -- CLK_RISING_ONLY_MASK preset
# t2 -- CLK_FALLING_HIGH_MASK preset
# t3 -- CLK_FALLING_LOW_MASK preset
# t4 -- Temporary
# t5 -- Temporary
# t6 -- Temporary

SWD_Write_GPIO_Fast:
    li      t0, R32_GPIOB_BSHR              # preload BSHR address
    li      t1, CLK_RISING_ONLY_MASK        # clock rising and data hold preset
    li      t2, CLK_FALLING_HIGH_MASK       # clock falling and data high preset
    li      t3, CLK_FALLING_LOW_MASK        # clock falling and data low preset

#=========== send header bit 0 ===========  # bit0 is always 1
    // andi    t5, a0, 1                       # save LSB in t5
    // bnez    t5, .header_bit0_set            # jump to .header_bitx_set
    // sw      t3, 0(t0)                       # generate clock falling and data 0
    // j       .header_bit0_end                # jump to .header_bitx_end
.header_bit0_set:
    sw      t2, 0(t0)                       # generate clock falling and data 1
.header_bit0_end:
    sw      t1, 0(t0)                       # generate clock rising and data holding
    srli    a0, a0, 1                       # logic right shift for next send

#=========== send header bit 1-6 ===========
.rept 6
    andi    t5, a0, 1                       # save LSB in t5
    bnez    t5, 1f            # jump to .header_bitx_set
    sw      t3, 0(t0)                       # generate clock falling and data 0
    j       2f                # jump to .header_bitx_end
1:
    sw      t2, 0(t0)                       # generate clock falling and data 1
2:
    sw      t1, 0(t0)                       # generate clock rising and data holding
    srli    a0, a0, 1                       # logic right shift for next send
.endr

#=========== send header bit 7 ===========  # bit7 is always 1
    // andi    t5, a0, 1                       # save LSB in t5
    // bnez    t5, .header_bit7_set            # jump to .header_bitx_set
    // sw      t3, 0(t0)                       # generate clock falling and data 0
    // j       .header_bit7_end                # jump to .header_bitx_end
.header_bit7_set:
    sw      t2, 0(t0)                       # generate clock falling and data 1
.header_bit7_end:
    sw      t1, 0(t0)                       # generate clock rising and data holding

#=========== turnaround ===========
    li      t4, R32_GPIOB_CFGHR
    li      t5, CFG_INPUT_MASK
    sw      t5, 0(t4)  
    li      t5, DIR_INPUT_MASK
    sw      t5, 0(t0)

    sw      t2, 0(t0)                       # generate clock falling
    sw      t1, 0(t0)                       # generate clock rising and data sampling

#=========== sampling ack ===========
    li      a0, 0 
    li      t6, R32_GPIOB_INDR              # R32_GPIOB_INDR address preset

    sw      t2, 0(t0)                       # generate clock falling
    lw      t5, 0(t6)                       # read INDR to t5
    sw      t1, 0(t0)                       # generate clock rising
    srli    t5, t5, 14                      # move useful bit to bit 0
    andi    t5, t5, 1                       # clear other bit
    or      a0, a0, t5                      # save to a0

    sw      t2, 0(t0)                       # generate clock falling
    lw      t5, 0(t6)                       # read INDR to t5
    sw      t1, 0(t0)                       # generate clock rising
    srli    t5, t5, 13                      # move useful bit to bit 1
    andi    t5, t5, 2                       # clear other bit
    or      a0, a0, t5                      # save to a0

    sw      t2, 0(t0)                       # generate clock falling
    lw      t5, 0(t6)                       # read INDR to t5
    sw      t1, 0(t0)                       # generate clock rising
    srli    t5, t5, 12                      # move useful bit to bit 2
    andi    t5, t5, 4                       # clear other bit
    or      a0, a0, t5                      # save to a0

#=========== turnaround ===========
    sw      t2, 0(t0)                       # generate clock falling
    sw      t1, 0(t0)                       # generate clock rising

    li      t5, CFG_OUTPUT_MASK             # swdio config input
    sw      t5, 0(t4)  
    li      t5, DIR_OUTPUT_MASK             # swdir switch input
    sw      t5, 0(t0)

#=========== check ack and branch ===========
    li      t6, ACK_OK_MASK                 # ACK_OK
    beq     a0, t6, .lable_ack_ok
    li      t6, ACK_WAIT_MASK               # ACK_WAIT
    beq     a0, t6, .lable_ack_wait
    li      t6, ACK_FAULT_MASK              # ACK_FAULT
    beq     a0, t6, .lable_ack_fault
    j       .lable_ack_error

.lable_ack_ok:
#=========== load data ===========
    lw      t4, 0(a4)      # load 32-bit data in t4
    mv      a5, t4
#=========== calc parity bit ===========
    srli    a6, a5, 16
    xor     a5, a5, a6      # 32 -- 16
    srli    a6, a5, 8
    xor     a5, a5, a6      # 16 -- 8
    srli    a6, a5, 4
    xor     a5, a5, a6      # 8 -- 4
    srli    a6, a5, 2
    xor     a5, a5, a6      # 4 -- 2
    srli    a6, a5, 1
    xor     a5, a5, a6      # 2 -- 1
#    andi    a5, a5, 1       # clear
# parity bit store in a5

.lable_send_data:
#=========== send data bit 0-30 ===========
.rept 31
    andi    t5, t4, 1                       # save LSB in t5
    bnez    t5, 1f                          # jump to bit set
    sw      t3, 0(t0)                       # clock falling and data 0
    j       2f                              # jump to bit end
1:
    sw      t2, 0(t0)                       # clock falling and data 1
2:
    sw      t1, 0(t0)                       # clock rising and data holding
    srli    t4, t4, 1                       # logic right shift for next send
.endr

#=========== send data bit 31 ===========
    andi    t5, t4, 1                       # save LSB in t5
    bnez    t5, 1f                          # jump to bit set
    sw      t3, 0(t0)                       # clock falling and data 0
    j       2f                              # jump to bit end
1:
    sw      t2, 0(t0)                       # clock falling and data 1
2:
    sw      t1, 0(t0)                       # clock rising and data holding

#=========== send parity bit ===========
    andi    a5, a5, 1
    bnez    a5, 1f                          # jump to bit set
    sw      t3, 0(t0)                       # clock falling and data 0
    j       2f                              # jump to bit end
1:
    sw      t2, 0(t0)                       # clock falling and data 1
2:
    sw      t1, 0(t0)                       # clock rising and data holding

.lable_ack_wait:
.lable_ack_fault:
.lable_ack_error:
    li      t4, DATA_OUT_HIGH_MASK
    sw      t4, 0(t0)                       # output data high

    ret
