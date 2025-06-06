.section ".text.boot"

.globl _start
_start:
.option push
.option norelax
  la sp, __stack_top__
  la gp, __global_pointer$
.option pop
  nop
  nop

  # Allow exceptions/interrupts to be handled in S-mode
  # li t0, 0xffff
  # csrw medeleg, t0
  # csrw mideleg, t0

  # Bit   Label
  # 1     SIE
  # 3     MIE
  # 5     SPIE
  # 7     MPIE
  # 11:12 MPP
  # By setting MPP to 0b01, we are saying that the
  # CPU was in S-mode before entering the trap.
  # Note that we are actually not trapping
  # When we return from the "trap," we should be in
  # S-mode
  # li t0, (0b01 << 11) | (1 << 5) | (1 << 1)
  # li t0, (0b01 << 11)
  # csrw mstatus, t0

  csrr t1, mstatus  
  li    t2, ~(3 << 11)      # Clear bits 12:11 = MPP
  and   t1, t1, t2
  li   t0, (1 << 11) # MPP = S-mode
  or   t1, t1, t0
  csrw mstatus, t1

  # Enable interrupts in S-mode, write to MIE
  # Bit   Label
  # 1     SSIE
  # 5     STIE
  # li t0, (1 << 5) | (1 << 1)
  li t0, 0
  csrw mie, t0 # disabling interrupts
  csrw mip, t0 # clear pending interrupts
  
  # Drop from machine mode to supervisor mode, starting at _cstart
  la t0, _cstart
  csrw mepc, t0

  # Most permissive PMP configuration
  li t0, -1
  csrw pmpaddr0, t0
  li t0, 0x1F
  csrw pmpcfg0, t0

  # set up traps to see where we at
  la t0, m_trap_handler
  csrw mtvec, t0

  # dumping stuff
  call dump_csrs

  # ecall

  mret
  call _cstart 


halt:
  nop
  j halt

.align 2
.global m_trap_handler
m_trap_handler:
    call print_in_trap
    call dump_csrs

    # Optional: infinite loop to halt
1:  j 1b


.globl put32
.globl PUT32
put32:
PUT32:
  fence iorw,iorw
  sw a1, 0(a0)
  fence iorw,iorw
  ret

.globl get32
.globl GET32
get32:
GET32:
  fence iorw,iorw
  lw a0, 0(a0)
  ret

.globl put64
.globl PUT64
put64:
PUT64:
  fence iorw,iorw
  sd a1, 0(a0)
  fence iorw,iorw
  ret

