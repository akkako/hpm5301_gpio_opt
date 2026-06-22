    
    .equ FGPIO_BASE,                0x000C0000

    .equ FGPIO_DI_VAL_OFFSET,       0x000

    .equ FGPIO_DO_VAL_OFFSET,       0x100
    .equ FGPIO_DO_SET_OFFSET,       0x104
    .equ FGPIO_DO_CLR_OFFSET,       0x108

    .equ FGPIO_OE_VAL_OFFSET,       0x200
    .equ FGPIO_OE_SET_OFFSET,       0x204
    .equ FGPIO_OE_CLR_OFFSET,       0x208

    .equ SWDIR_SHIFT,               30
    .equ SWCLK_SHIFT,               27
    .equ SWDIO_SHIFT,               29

    .equ SWDIR_OFFSET,              0x40000000
    .equ SWCLK_OFFSET,              0x08000000
    .equ SWDIO_OFFSET,              0x20000000

    .equ ACK_OK_MASK,               1
    .equ ACK_WAIT_MASK,             2
    .equ ACK_FAULT_MASK,            4

    .text
    .align  2
    .global SWD_Write_Opt
    .type   SWD_Write_Opt, @function

# definition
# uint8_t SWD_Write_Opt(uint8_t header, 
#                       uint8_t turnaround, 
#                       uint8_t data_phase, 
#                       uint8_t idle_cycles, 
#                       uint32_t *data);
#
# parameter:
# a0 -- uint8_t header (already zero-extended)
# a1 -- uint8_t turnaround (already zero-extended)
# a2 -- uint8_t data_phase (already zero-extended)
# a3 -- uint8_t idle_cycles (already zero-extended)
# a4 -- uint32_t *data
#
# return value:
# a0 -- ack (need zero-extend to 32-bit)

# Register allocate
# a0 -- param header, return ack
# a1 -- param turnaround
# a2 -- param data_phase
# a3 -- param idle_cycles
# a4 -- param *data
# a5 -- Temporary
# a6 -- Temporary
# a7 -- Temporary
# t0 -- Temporary
# t1 -- Temporary
# t2 -- Temporary
# t3 -- Temporary
# t4 -- Temporary
# t5 -- Temporary
# t6 -- Temporary

SWD_Write_Opt:
# 加载预设值和地址
    li      t0, FGPIO_BASE              # t0 = FGPIO 基地址
    li      t1, SWDIR_OFFSET            # SWDIR 偏移位定义
    li      t3, SWCLK_OFFSET            # SWCLK 偏移位定义

# 发送 8 位包头
# used register
# a0 -- header
# t0 -- FGPIO_BASE
# t1 -- SWDIR_OFFSET
# t2 -- next DO.VALUE calculate
# t3 -- SWCLK_OFFSET
    ori     t2, a0, 1                   # t2 = 包头 LSB
#---------------- bit 0 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    ori     t2, a0, 1                   # t2 = 包头 LSB
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
#---------------- bit 1 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    ori     t2, a0, 1                   # t2 = 包头 LSB
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
#---------------- bit 2 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    ori     t2, a0, 1                   # t2 = 包头 LSB
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
#---------------- bit 3 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    ori     t2, a0, 1                   # t2 = 包头 LSB
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
#---------------- bit 4 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    ori     t2, a0, 1                   # t2 = 包头 LSB
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
#---------------- bit 5 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    ori     t2, a0, 1                   # t2 = 包头 LSB
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
#---------------- bit 6 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    ori     t2, a0, 1                   # t2 = 包头 LSB
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
#---------------- bit 7 ----------------
    slli    t2, t2, SWDIO_SHIFT         # t2 = 数据偏移到 SWDIO 数据位
    and     t2, t2, t1                  # t2 = DO[0].VALUE 预设值
    sw      t2, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    li      t2, SWDIO_OFFSET            # t2 = SWDIO 偏移位定义
    mv      a0, zero                    # a0 = 0
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据

# turnaround 周期
# used register:
# t0 -- FGPIO_BASE
# t1 -- SWDIR_OFFSET
# t2 -- SWDIO_OFFSET
# t3 -- SWCLK_OFFSET
# t4 -- turnaround


    ret
