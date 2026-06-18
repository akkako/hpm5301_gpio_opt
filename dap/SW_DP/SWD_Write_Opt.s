






    .text
    .align  2
    .global SWD_Write_Opt
    .type   SWD_Write_Opt, @function

# definition
# uint8_t SWD_Write_Opt(uint8_t header, uint32_t *data)
#
# parameter:
# a0 -- uint8_t header (already zero-extended)
# a1 -- uint32_t *data
#
# return value:
# a0 -- ack (zero-extend to 32-bit)

# Register allocate
# a0 -- header, ack return data
# a1 -- Temporary
# a2 -- Temporary
# a3 -- Temporary
# a4 -- uint32_t *data
# a5 -- parity calculate temp
# a6 -- parity calculate temp
# a7 -- Temporary
# t0 -- Temporary
# t1 -- Temporary
# t2 -- Temporary
# t3 -- Temporary
# t4 -- Temporary
# t5 -- Temporary
# t6 -- Temporary

SWD_Write_Opt:
    