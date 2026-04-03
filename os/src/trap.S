.section .text
.align 2
.global trap_entry

trap_entry:
    # save caller-saved registers (callee-saved preserved by trap_handler per ABI)
    addi  sp, sp, -64
    sw    ra,  0(sp)
    sw    a0,  4(sp)
    sw    a1,  8(sp)
    sw    a2, 12(sp)
    sw    a3, 16(sp)
    sw    a4, 20(sp)
    sw    a5, 24(sp)
    sw    a6, 28(sp)
    sw    a7, 32(sp)
    sw    t0, 36(sp)
    sw    t1, 40(sp)
    sw    t2, 44(sp)
    sw    t3, 48(sp)
    sw    t4, 52(sp)
    sw    t5, 56(sp)
    sw    t6, 60(sp)

    csrr  a0, mcause
    csrr  a1, mepc
    csrr  a2, mtval
    mv    a3, sp        # 4th arg: pointer to saved caller-saved frame
    call  trap_handler

    # advance mepc past ecall instruction (mepc += 4 for ecall causes)
    csrr  t0, mcause
    li    t1, 8         # CAUSE_ECALL_U
    beq   t0, t1, 1f
    li    t1, 9         # CAUSE_ECALL_S
    beq   t0, t1, 1f
    li    t1, 11        # CAUSE_ECALL_M
    bne   t0, t1, 2f
1:  csrr  t0, mepc
    addi  t0, t0, 4
    csrw  mepc, t0
2:  lw    ra,  0(sp)
    lw    a0,  4(sp)
    lw    a1,  8(sp)
    lw    a2, 12(sp)
    lw    a3, 16(sp)
    lw    a4, 20(sp)
    lw    a5, 24(sp)
    lw    a6, 28(sp)
    lw    a7, 32(sp)
    lw    t0, 36(sp)
    lw    t1, 40(sp)
    lw    t2, 44(sp)
    lw    t3, 48(sp)
    lw    t4, 52(sp)
    lw    t5, 56(sp)
    lw    t6, 60(sp)
    addi  sp, sp, 64

    mret
