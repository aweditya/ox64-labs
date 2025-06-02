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
  li t0, 0xffff
  csrw medeleg, t0
  csrw mideleg, t0

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
  li t0, (0b01 << 11) | (1 << 5) | (1 << 1)
  csrw mstatus, t0

  # Enable interrupts in S-mode
  # Bit   Label
  # 1     SSIE
  # 5     STIE
  li t0, (1 << 5) | (1 << 1)

  call _cstart
halt:
  nop
  j halt

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

# .globl handler_trampoline
# handler_trampoline:
#   j handler
#   ret
