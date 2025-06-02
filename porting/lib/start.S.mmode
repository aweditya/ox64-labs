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
