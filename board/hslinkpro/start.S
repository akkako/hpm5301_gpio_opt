/*
 * Copyright (c) 2021-2022 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "hpm_csr_regs.h"

    .section .start, "ax"

    .global _start
    .type _start,@function

_start:
    /* Initialize global pointer */
    .option push
    .option norelax
    la gp, __global_pointer$
    la tp, __thread_pointer$
    .option pop

    /* reset mstatus to 0*/
    csrrw x0, mstatus, x0

#ifdef __riscv_flen
    /* Enable FPU */
    li t0, CSR_MSTATUS_FS_MASK
    csrrs t0, mstatus, t0

    /* Initialize FCSR */
    fscsr zero
#endif

#ifdef INIT_EXT_RAM_FOR_DATA
    la t0, _stack_safe
    mv sp, t0
    call _init_ext_ram
#endif

    /* Initialize stack pointer */
    la t0, _stack
    mv sp, t0

    /*
     * Copy .vectors and .text from Flash (LMA) to ILM (VMA).
     * These sections are linked to run from ILM, but their initial
     * values are stored in Flash. The copy must be performed before
     * calling any function that resides in .text.
     *
     * All copy loops are placed in the .start section (running from
     * Flash) and do not rely on any .text function.
     */

    /* Copy .vectors */
    la a0, __vector_load_addr__
    la a1, __vector_ram_start__
    la a2, __vector_ram_end__
    beq a1, a2, 2f
1:
    lb t0, 0(a0)
    sb t0, 0(a1)
    addi a0, a0, 1
    addi a1, a1, 1
    blt a1, a2, 1b
2:

    /* Copy .text */
    la a0, __text_load_addr__
    la a1, __text_ram_start__
    la a2, __text_ram_end__
    beq a1, a2, 4f
3:
    lb t0, 0(a0)
    sb t0, 0(a1)
    addi a0, a0, 1
    addi a1, a1, 1
    blt a1, a2, 3b
4:

#ifdef CONFIG_NOT_ENABLE_ICACHE
    call l1c_ic_disable
#else
    call l1c_ic_enable
#endif
#ifdef CONFIG_NOT_ENABLE_DCACHE
    call l1c_dc_invalidate_all
    call l1c_dc_disable
#else
    call l1c_dc_enable
    call l1c_dc_invalidate_all
#endif

    /*
     * Initialize LMA/VMA sections.
     * Relocation for any sections that need to be copied from LMA to VMA.
     */
    call c_startup

#if defined(__SES_RISCV)
    /* Initialize the heap */
    la a0, __heap_start__
    la a1, __heap_end__
    sub a1, a1, a0
    la t1, __SEGGER_RTL_init_heap
    jalr t1
#endif

    /* Do global constructors */
    call __libc_init_array

#ifndef NO_CLEANUP_AT_START
    /* clean up */
    call _clean_up
#endif

#ifdef __nds_execit
    /* Initialize EXEC.IT table */
    la t0, _ITB_BASE_
    csrw uitb, t0
#endif

#if defined(CONFIG_FREERTOS) && CONFIG_FREERTOS
    #define HANDLER_TRAP freertos_risc_v_trap_handler
    #define HANDLER_S_TRAP freertos_risc_v_trap_handler

    /* Use mscratch to store isr level */
    csrw mscratch, 0
#elif defined(CONFIG_UCOS_III) && CONFIG_UCOS_III
    #define HANDLER_TRAP ucos_risc_v_trap_handler
    #define HANDLER_S_TRAP ucos_risc_v_trap_handler

    /* Use mscratch to store isr level */
    csrw mscratch, 0
#elif defined(CONFIG_THREADX) && CONFIG_THREADX
    #define HANDLER_TRAP tx_risc_v_trap_handler
    #define HANDLER_S_TRAP tx_risc_v_trap_handler

    /* Use mscratch to store isr level */
    csrw mscratch, 0

#elif defined(CONFIG_RTTHREAD) && CONFIG_RTTHREAD
    #define HANDLER_TRAP rtt_risc_v_trap_handler
    #define HANDLER_S_TRAP rtt_risc_v_trap_handler

    /* Use mscratch to store isr level */
    csrw mscratch, 0

#else
    #define HANDLER_TRAP irq_handler_trap
    #define HANDLER_S_TRAP irq_handler_s_trap
#endif

#if !defined(USE_NONVECTOR_MODE) || (USE_NONVECTOR_MODE == 0)
    /* Initial machine trap-vector Base */
    la t0, __vector_table
    csrw mtvec, t0

#if defined (USE_S_MODE_IRQ)
    la t0, __vector_s_table
    csrw stvec, t0
#endif
    /* Enable vectored external PLIC interrupt */
    csrsi CSR_MMISC_CTL, 2
#else
    /* Initial machine trap-vector Base */
    la t0, HANDLER_TRAP
    csrw mtvec, t0
#if defined (USE_S_MODE_IRQ)
    la t0, HANDLER_S_TRAP
    csrw stvec, t0
#endif

    /* Disable vectored external PLIC interrupt */
    csrci CSR_MMISC_CTL, 2
#endif

    /* System reset handler */
    call reset_handler

    /* Infinite loop, if returned accidentally */
1:    j 1b

    .weak exit
exit:
1:    j 1b

    .section .isr_vector, "ax"
    .weak nmi_handler
nmi_handler:
1:    j 1b

#include "vectors.h"
