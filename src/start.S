.section ".text.boot"

.globl _start
_start:
    # we dont want the compiler to relax gp setup
    # into an addi which would not be position-
    # independent
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    # setup stack
    # sp does not need to be position independent
    # like the gp
    la sp, __stack_top__

    # we boot in machine mode
    # disable external interrupts
    csrci mstatus, 0x8

    # s0: saved register/frame pointer
    add s0, sp, zero

    # trampoline off to c-land
    jal zero, _cstart

#
# put32(addr,val).  c-code equivalant:
#   - put32(addr, val) { *(uint32_t*)addr = val; }
#
# store value <val> (passed in second argument register r1) 
# into memory address <addr> (passed in first argument 
# register, r0).
#
.globl put32
.globl PUT32
put32:
PUT32:
    fence rw,rw
    sw a1, (a0)
    fence rw,rw
    ret

# get32(addr)
#
# c-code equivalent:
#   - get32(addr) { return *(uint32_t*)addr; }
#
# load value of <addr> passed in first argument register (r0).
#
.globl get32
.globl GET32
get32:
GET32:
    fence rw,rw
    lw a0, (a0)
    ret

#
# put64(addr,val).  c-code equivalant:
#   - put64(addr, val) { *(uint64_t*)addr = val; }
#
# store value <val> (passed in second argument register r1) 
# into memory address <addr> (passed in first argument 
# register, r0).
#
.globl put64
.globl PUT64
put64:
PUT64:
    fence rw,rw
    sd a1, (a0)
    fence rw,rw
    ret

# get64(addr)
#
# c-code equivalent:
#   - get64(addr) { return *(uint64_t*)addr; }
#
# load value of <addr> passed in first argument register (r0).
#
.globl get64
.globl GET64
get64:
GET64:
    fence rw,rw
    ld a0, (a0)
    ret

