.globl enable_interrupts
enable_interrupts:
    csrsi mstatus, 1<<3 
    ret

.globl disable_interrupts
disable_interrupts:
    csrci mstatus, 1<<3
    ret

.globl disable_timer_interrupts
    csrci mie, 1<<7
    ret

.globl enable_timer_interrupts
    csrsi mie, 1<<7
    ret

