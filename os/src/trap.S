.section .text
.align 2
.global trap_entry

trap_entry:
    # temporarily save t0 so we can use it as scratch
    csrw  mscratch, t0
    addi  sp, sp, -128        # allocate full frame: x1-x31 (31 regs × 4 = 124, padded to 128)
    sw    ra,   0(sp)         # x1  ra
    addi  t0, sp, 128
    sw    t0,   4(sp)         # x2  sp (original, before addi above)
    sw    gp,   8(sp)         # x3  gp
    sw    tp,  12(sp)         # x4  tp
    csrr  t0, mscratch
    sw    t0,  16(sp)         # x5  t0 (original)
    sw    t1,  20(sp)         # x6  t1
    sw    t2,  24(sp)         # x7  t2
    sw    s0,  28(sp)         # x8  s0
    sw    s1,  32(sp)         # x9  s1
    sw    a0,  36(sp)         # x10 a0
    sw    a1,  40(sp)         # x11 a1
    sw    a2,  44(sp)         # x12 a2
    sw    a3,  48(sp)         # x13 a3
    sw    a4,  52(sp)         # x14 a4
    sw    a5,  56(sp)         # x15 a5
    sw    a6,  60(sp)         # x16 a6
    sw    a7,  64(sp)         # x17 a7
    sw    s2,  68(sp)         # x18 s2
    sw    s3,  72(sp)         # x19 s3
    sw    s4,  76(sp)         # x20 s4
    sw    s5,  80(sp)         # x21 s5
    sw    s6,  84(sp)         # x22 s6
    sw    s7,  88(sp)         # x23 s7
    sw    s8,  92(sp)         # x24 s8
    sw    s9,  96(sp)         # x25 s9
    sw    s10,100(sp)         # x26 s10
    sw    s11,104(sp)         # x27 s11
    sw    t3, 108(sp)         # x28 t3
    sw    t4, 112(sp)         # x29 t4
    sw    t5, 116(sp)         # x30 t5
    sw    t6, 120(sp)         # x31 t6

    csrr  a0, mcause
    csrr  a1, mepc
    csrr  a2, mtval
    mv    a3, sp              # 4th arg: pointer to full register frame
    call  trap_handler

    # advance mepc past ecall instruction (ecall causes only)
    csrr  t0, mcause
    li    t1, 8               # CAUSE_ECALL_U
    beq   t0, t1, 1f
    li    t1, 9               # CAUSE_ECALL_S
    beq   t0, t1, 1f
    li    t1, 11              # CAUSE_ECALL_M
    bne   t0, t1, 2f
1:  csrr  t0, mepc
    addi  t0, t0, 4
    csrw  mepc, t0
2:  lw    ra,   0(sp)
    # sp restored last via addi - skip frame[1]
    lw    gp,   8(sp)
    lw    tp,  12(sp)
    lw    t0,  16(sp)
    lw    t1,  20(sp)
    lw    t2,  24(sp)
    lw    s0,  28(sp)
    lw    s1,  32(sp)
    lw    a0,  36(sp)
    lw    a1,  40(sp)
    lw    a2,  44(sp)
    lw    a3,  48(sp)
    lw    a4,  52(sp)
    lw    a5,  56(sp)
    lw    a6,  60(sp)
    lw    a7,  64(sp)
    lw    s2,  68(sp)
    lw    s3,  72(sp)
    lw    s4,  76(sp)
    lw    s5,  80(sp)
    lw    s6,  84(sp)
    lw    s7,  88(sp)
    lw    s8,  92(sp)
    lw    s9,  96(sp)
    lw    s10,100(sp)
    lw    s11,104(sp)
    lw    t3, 108(sp)
    lw    t4, 112(sp)
    lw    t5, 116(sp)
    lw    t6, 120(sp)
    addi  sp, sp, 128

    mret
