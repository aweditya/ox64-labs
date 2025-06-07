ENTRY(_start)

MEMORY
{
  OCRAM (rwx) : ORIGIN = 0x22020000, LENGTH = 64K
  WRAM (rwx) : ORIGIN = 0x22030000, LENGTH = 160K
  DRAM (rwx) : ORIGIN = 0x3ef80000, LENGTH = 16K
  VRAM (rwx) : ORIGIN = 0x3f000000, LENGTH = 16K
  XRAM (rwx) : ORIGIN = 0x40000000, LENGTH = 16K

  PSRAM (rwx) : ORIGIN = 0x50000000, LENGTH = 64M

  FLASH (rx) : ORIGIN = 0x58000000, LENGTH = 64M

  ROM (rx) : ORIGIN = 0x90000000, LENGTH = 128K
}

SECTIONS
{
    . = ORIGIN(PSRAM);
    __stack_size = 1M;
    __pt_size = 4K;

    .text : {
        _kcode_start = .;
        KEEP(*(.text.boot))  
        *(.text*) 
        _kcode_end = .;
        . = ALIGN(8);
    } > PSRAM

    .rodata : {
        . = ALIGN(8);
        _krodata_start = .;
        *(.rodata*)
        *(.srodata*)
        . = ALIGN(8);
        _krodata_end = .;
    } > PSRAM

    .data : { 
        . = ALIGN(8);
        _kdata_start = .;
        _kdata_start_load = LOADADDR(.data);
        __global_pointer$ = . + 0x800;
        *(.sdata*)
        *(.data*)
        . = ALIGN(8);
        _kdata_end = .;
    } > PSRAM

    .bss : {
        . = ALIGN(8);
        _kbss_start = .;
        *(.sbss*)
        *(.bss*)
        *(COMMON)
        . = ALIGN(8);
        _kbss_end = .;
    } > PSRAM

    .pg1 : {
      . = ALIGN(0x1000);
      *(.pg1)
      . += __pt_size;
      . = ALIGN(0x1000);
    } > PSRAM

    .pg2 : {
      . = ALIGN(0x1000);
      *(.pg2)
      . += __pt_size;
      . = ALIGN(0x1000);
    } > PSRAM

    .pg3 : {
      . = ALIGN(0x1000);
      *(.pg3)
      . += __pt_size;
      . = ALIGN(0x1000);
    } > PSRAM

    .stack : {
      __stack_bottom__ = .;
      . += __stack_size;
      __stack_top__ = .;
    } > PSRAM

    /DISCARD/ : {
      *(.comment)
      *(.riscv.attributes)
      *(.note)
      *(.eh_frame)
    }
}
